/* $Id$ */
/** @file
 * VBoxMaterialBridge - Line-delimited JSON bridge between the Material shell
 * front end and the VirtualBox Main API.
 *
 * Reads one JSON request per line on stdin and writes one JSON response per
 * line on stdout:
 *
 *      request:  {"id": 7, "method": "machines.list", "params": {...}}
 *      response: {"id": 7, "result": ...}
 *                {"id": 7, "error": {"message": "...", "hrc": -2135228409}}
 *
 * The bridge owns no state of its own: every call goes straight to VBoxSVC,
 * so the Qt manager, VBoxManage and this shell always see the same truth.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/VirtualBox.h>
#include <VBox/version.h>

#include <iprt/initterm.h>
#include <iprt/json.h>
#include <iprt/mem.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/message.h>

using namespace com;


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/** Global Main API handles shared by every request. */
typedef struct BRIDGECTX
{
    ComPtr<IVirtualBoxClient>   ptrClient;
    ComPtr<IVirtualBox>         ptrVBox;
    ComPtr<ISession>            ptrSession;
} BRIDGECTX;
typedef BRIDGECTX *PBRIDGECTX;


/**
 * Minimal JSON writer.  Values are appended in order; the caller is
 * responsible for structure.  Strings are escaped per RFC 8259.
 */
class BridgeJson
{
public:
    BridgeJson() : m_fNeedComma(false) {}

    const Utf8Str &str() const { return m_str; }

    BridgeJson &beginObject() { sep(); m_str.append('{'); m_fNeedComma = false; return *this; }
    BridgeJson &endObject()   { m_str.append('}'); m_fNeedComma = true; return *this; }
    BridgeJson &beginArray()  { sep(); m_str.append('['); m_fNeedComma = false; return *this; }
    BridgeJson &endArray()    { m_str.append(']'); m_fNeedComma = true; return *this; }

    BridgeJson &key(const char *pszKey)
    {
        sep();
        appendString(pszKey);
        m_str.append(':');
        m_fNeedComma = false;
        return *this;
    }

    BridgeJson &value(const char *psz)       { sep(); appendString(psz ? psz : ""); m_fNeedComma = true; return *this; }
    BridgeJson &value(const Utf8Str &s)      { return value(s.c_str()); }
    BridgeJson &value(const Bstr &bstr)      { return value(Utf8Str(bstr)); }
    BridgeJson &value(bool f)                { sep(); m_str.append(f ? "true" : "false"); m_fNeedComma = true; return *this; }
    BridgeJson &value(int64_t i)             { sep(); m_str.appendPrintf("%RI64", i); m_fNeedComma = true; return *this; }
    BridgeJson &value(double rd)             { sep(); m_str.appendPrintf("%u.%03u", (unsigned)rd, (unsigned)((rd - (unsigned)rd) * 1000)); m_fNeedComma = true; return *this; }
    BridgeJson &null()                       { sep(); m_str.append("null"); m_fNeedComma = true; return *this; }

    BridgeJson &raw(const Utf8Str &strJson)  { sep(); m_str.append(strJson); m_fNeedComma = true; return *this; }

    template<typename T> BridgeJson &member(const char *pszKey, T v) { key(pszKey); return value(v); }

private:
    void sep()
    {
        if (m_fNeedComma)
            m_str.append(',');
    }

    void appendString(const char *psz)
    {
        m_str.append('"');
        for (const unsigned char *pb = (const unsigned char *)psz; *pb; pb++)
        {
            switch (*pb)
            {
                case '"':  m_str.append("\\\""); break;
                case '\\': m_str.append("\\\\"); break;
                case '\n': m_str.append("\\n"); break;
                case '\r': m_str.append("\\r"); break;
                case '\t': m_str.append("\\t"); break;
                default:
                    if (*pb < 0x20)
                        m_str.appendPrintf("\\u%04x", *pb);
                    else
                        m_str.append((char)*pb);
                    break;
            }
        }
        m_str.append('"');
    }

    Utf8Str m_str;
    bool    m_fNeedComma;
};


/** Thrown by request handlers to report a failure with its HRESULT. */
class BridgeError
{
public:
    BridgeError(const Utf8Str &strMsg, HRESULT hrc = E_FAIL) : m_strMsg(strMsg), m_hrc(hrc) {}
    Utf8Str m_strMsg;
    HRESULT m_hrc;
};

/** Throws a BridgeError when a Main API call fails. */
#define BRIDGE_HRC(a_Expr, a_pszWhat) \
    do { \
        HRESULT const hrcCheck = (a_Expr); \
        if (FAILED(hrcCheck)) \
            throw BridgeError(Utf8StrFmt("%s failed (%Rhrc)", (a_pszWhat), hrcCheck), hrcCheck); \
    } while (0)


/*********************************************************************************************************************************
*   Helpers                                                                                                                      *
*********************************************************************************************************************************/
static const char *bridgeMachineStateName(MachineState_T enmState)
{
    switch (enmState)
    {
        case MachineState_PoweredOff:   return "poweredOff";
        case MachineState_Saved:        return "saved";
        case MachineState_AbortedSaved: return "abortedSaved";
        case MachineState_Aborted:      return "aborted";
        case MachineState_Running:      return "running";
        case MachineState_Paused:       return "paused";
        case MachineState_Stuck:        return "stuck";
        case MachineState_Teleported:   return "teleported";
        case MachineState_Starting:     return "starting";
        case MachineState_Stopping:     return "stopping";
        case MachineState_Saving:       return "saving";
        case MachineState_Restoring:    return "restoring";
        default:                        return "busy";
    }
}

static Utf8Str bridgeParamString(RTJSONVAL hParams, const char *pszName, bool fRequired = true)
{
    char *psz = NULL;
    int vrc = hParams != NIL_RTJSONVAL ? RTJsonValueQueryStringByName(hParams, pszName, &psz) : VERR_NOT_FOUND;
    if (RT_FAILURE(vrc))
    {
        if (fRequired)
            throw BridgeError(Utf8StrFmt("Missing string parameter '%s'", pszName), E_INVALIDARG);
        return Utf8Str();
    }
    Utf8Str str(psz);
    RTStrFree(psz);
    return str;
}

static bool bridgeParamInt(RTJSONVAL hParams, const char *pszName, int64_t *pi64)
{
    return hParams != NIL_RTJSONVAL && RT_SUCCESS(RTJsonValueQueryIntegerByName(hParams, pszName, pi64));
}

static ComPtr<IMachine> bridgeFindMachine(PBRIDGECTX pCtx, RTJSONVAL hParams)
{
    Utf8Str strId = bridgeParamString(hParams, "id");
    ComPtr<IMachine> ptrMachine;
    HRESULT hrc = pCtx->ptrVBox->FindMachine(Bstr(strId).raw(), ptrMachine.asOutParam());
    if (FAILED(hrc) || ptrMachine.isNull())
        throw BridgeError(Utf8StrFmt("No machine with id '%s'", strId.c_str()), hrc);
    return ptrMachine;
}

/** Waits for a progress object and turns a failed result into a BridgeError. */
static void bridgeWaitProgress(ComPtr<IProgress> &ptrProgress, const char *pszWhat)
{
    if (ptrProgress.isNull())
        return;
    BRIDGE_HRC(ptrProgress->WaitForCompletion(-1), pszWhat);
    LONG iRc = 0;
    BRIDGE_HRC(ptrProgress->COMGETTER(ResultCode)(&iRc), pszWhat);
    if (FAILED(iRc))
    {
        ComPtr<IVirtualBoxErrorInfo> ptrErr;
        ptrProgress->COMGETTER(ErrorInfo)(ptrErr.asOutParam());
        Bstr bstrText;
        if (!ptrErr.isNull())
            ptrErr->COMGETTER(Text)(bstrText.asOutParam());
        throw BridgeError(bstrText.isNotEmpty() ? Utf8Str(bstrText) : Utf8StrFmt("%s failed", pszWhat), iRc);
    }
}

static void bridgeWriteMachineSummary(BridgeJson &j, ComPtr<IMachine> &ptrMachine)
{
    Bstr bstrId, bstrName, bstrOs;
    MachineState_T enmState = MachineState_Null;
    ULONG cMB = 0, cCpus = 0, cSnaps = 0;
    BOOL fAccessible = FALSE;
    LONG64 msLastChange = 0;
    SafeArray<BSTR> aGroups;

    ptrMachine->COMGETTER(Accessible)(&fAccessible);
    ptrMachine->COMGETTER(Id)(bstrId.asOutParam());
    j.beginObject().member("id", Utf8Str(bstrId)).member("accessible", (bool)fAccessible);
    if (!fAccessible)
    {
        j.member("name", "Inaccessible machine").member("state", "inaccessible").endObject();
        return;
    }
    ptrMachine->COMGETTER(Name)(bstrName.asOutParam());
    ptrMachine->COMGETTER(OSTypeId)(bstrOs.asOutParam());
    ptrMachine->COMGETTER(State)(&enmState);
    ptrMachine->COMGETTER(MemorySize)(&cMB);
    ptrMachine->COMGETTER(CPUCount)(&cCpus);
    ptrMachine->COMGETTER(SnapshotCount)(&cSnaps);
    ptrMachine->COMGETTER(LastStateChange)(&msLastChange);
    ptrMachine->COMGETTER(Groups)(ComSafeArrayAsOutParam(aGroups));

    j.member("name", Utf8Str(bstrName))
     .member("osTypeId", Utf8Str(bstrOs))
     .member("state", bridgeMachineStateName(enmState))
     .member("memoryMB", (int64_t)cMB)
     .member("cpus", (int64_t)cCpus)
     .member("snapshotCount", (int64_t)cSnaps)
     .member("lastStateChangeMs", (int64_t)msLastChange);
    j.key("groups").beginArray();
    for (size_t i = 0; i < aGroups.size(); i++)
        j.value(Utf8Str(aGroups[i]));
    j.endArray().endObject();
}


/*********************************************************************************************************************************
*   Request handlers                                                                                                             *
*********************************************************************************************************************************/
static void bridgeHostInfo(PBRIDGECTX pCtx, RTJSONVAL, BridgeJson &j)
{
    ComPtr<IHost> ptrHost;
    BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(Host)(ptrHost.asOutParam()), "IVirtualBox::Host");
    Bstr bstrVersion, bstrOs, bstrOsVer;
    ULONG cCpus = 0, cbMemMB = 0, cbAvailMB = 0;
    pCtx->ptrVBox->COMGETTER(Version)(bstrVersion.asOutParam());
    ptrHost->COMGETTER(OperatingSystem)(bstrOs.asOutParam());
    ptrHost->COMGETTER(OSVersion)(bstrOsVer.asOutParam());
    ptrHost->COMGETTER(ProcessorCount)(&cCpus);
    ptrHost->COMGETTER(MemorySize)(&cbMemMB);
    ptrHost->COMGETTER(MemoryAvailable)(&cbAvailMB);
    j.beginObject()
     .member("version", Utf8Str(bstrVersion))
     .member("os", Utf8Str(bstrOs))
     .member("osVersion", Utf8Str(bstrOsVer))
     .member("cpus", (int64_t)cCpus)
     .member("memoryMB", (int64_t)cbMemMB)
     .member("memoryAvailableMB", (int64_t)cbAvailMB)
     .endObject();
}

static void bridgeMachinesList(PBRIDGECTX pCtx, RTJSONVAL, BridgeJson &j)
{
    SafeIfaceArray<IMachine> aMachines;
    BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(Machines)(ComSafeArrayAsOutParam(aMachines)), "IVirtualBox::Machines");
    j.beginArray();
    for (size_t i = 0; i < aMachines.size(); i++)
    {
        ComPtr<IMachine> ptrMachine = aMachines[i];
        if (!ptrMachine.isNull())
            bridgeWriteMachineSummary(j, ptrMachine);
    }
    j.endArray();
}

static void bridgeMachineDetails(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);

    BridgeJson jSummary;
    bridgeWriteMachineSummary(jSummary, ptrMachine);

    ULONG cVramMB = 0;
    ComPtr<IGraphicsAdapter> ptrGfx;
    if (SUCCEEDED(ptrMachine->COMGETTER(GraphicsAdapter)(ptrGfx.asOutParam())) && !ptrGfx.isNull())
        ptrGfx->COMGETTER(VRAMSize)(&cVramMB);

    FirmwareType_T enmFw = FirmwareType_BIOS;
    ComPtr<IFirmwareSettings> ptrFw;
    if (SUCCEEDED(ptrMachine->COMGETTER(FirmwareSettings)(ptrFw.asOutParam())) && !ptrFw.isNull())
        ptrFw->COMGETTER(FirmwareType)(&enmFw);

    Bstr bstrDesc, bstrFolder;
    ptrMachine->COMGETTER(Description)(bstrDesc.asOutParam());
    ptrMachine->COMGETTER(SettingsFilePath)(bstrFolder.asOutParam());

    j.beginObject().key("summary").raw(jSummary.str())
     .member("vramMB", (int64_t)cVramMB)
     .member("firmware", enmFw == FirmwareType_BIOS ? "BIOS" : "UEFI")
     .member("description", Utf8Str(bstrDesc))
     .member("settingsFile", Utf8Str(bstrFolder));

    /* Storage. */
    SafeIfaceArray<IMediumAttachment> aAttachments;
    ptrMachine->COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(aAttachments));
    j.key("storage").beginArray();
    for (size_t i = 0; i < aAttachments.size(); i++)
    {
        Bstr bstrCtl;
        LONG iPort = 0, iDevice = 0;
        DeviceType_T enmType = DeviceType_Null;
        ComPtr<IMedium> ptrMedium;
        aAttachments[i]->COMGETTER(Controller)(bstrCtl.asOutParam());
        aAttachments[i]->COMGETTER(Port)(&iPort);
        aAttachments[i]->COMGETTER(Device)(&iDevice);
        aAttachments[i]->COMGETTER(Type)(&enmType);
        aAttachments[i]->COMGETTER(Medium)(ptrMedium.asOutParam());
        j.beginObject()
         .member("controller", Utf8Str(bstrCtl))
         .member("port", (int64_t)iPort)
         .member("device", (int64_t)iDevice)
         .member("type", enmType == DeviceType_HardDisk ? "disk" : enmType == DeviceType_DVD ? "optical" : "other");
        if (!ptrMedium.isNull())
        {
            Bstr bstrName;
            LONG64 cbLogical = 0;
            ptrMedium->COMGETTER(Name)(bstrName.asOutParam());
            ptrMedium->COMGETTER(LogicalSize)(&cbLogical);
            j.member("medium", Utf8Str(bstrName)).member("logicalSize", (int64_t)cbLogical);
        }
        j.endObject();
    }
    j.endArray();

    /* Network adapters: walk slots until the machine says there are no more. */
    j.key("network").beginArray();
    for (ULONG iSlot = 0; iSlot < 8; iSlot++)
    {
        ComPtr<INetworkAdapter> ptrNic;
        if (FAILED(ptrMachine->GetNetworkAdapter(iSlot, ptrNic.asOutParam())) || ptrNic.isNull())
            break;
        BOOL fEnabled = FALSE;
        NetworkAttachmentType_T enmAttach = NetworkAttachmentType_Null;
        ptrNic->COMGETTER(Enabled)(&fEnabled);
        if (!fEnabled)
            continue;
        ptrNic->COMGETTER(AttachmentType)(&enmAttach);
        const char *pszAttach = "none";
        switch (enmAttach)
        {
            case NetworkAttachmentType_NAT:        pszAttach = "nat"; break;
            case NetworkAttachmentType_Bridged:    pszAttach = "bridged"; break;
            case NetworkAttachmentType_Internal:   pszAttach = "internal"; break;
            case NetworkAttachmentType_HostOnly:   pszAttach = "hostOnly"; break;
            case NetworkAttachmentType_NATNetwork: pszAttach = "natNetwork"; break;
            default: break;
        }
        j.beginObject().member("slot", (int64_t)iSlot).member("attachment", pszAttach).endObject();
    }
    j.endArray();

    SafeIfaceArray<ISharedFolder> aFolders;
    ptrMachine->COMGETTER(SharedFolders)(ComSafeArrayAsOutParam(aFolders));
    j.key("sharedFolders").beginArray();
    for (size_t i = 0; i < aFolders.size(); i++)
    {
        Bstr bstrName, bstrPath;
        aFolders[i]->COMGETTER(Name)(bstrName.asOutParam());
        aFolders[i]->COMGETTER(HostPath)(bstrPath.asOutParam());
        j.beginObject().member("name", Utf8Str(bstrName)).member("hostPath", Utf8Str(bstrPath)).endObject();
    }
    j.endArray().endObject();
}

static void bridgeMachineStart(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    Utf8Str strType = bridgeParamString(hParams, "type", false);
    if (strType.isEmpty())
        strType = "gui";
    if (strType != "gui" && strType != "headless" && strType != "separate")
        throw BridgeError("Session type must be gui, headless or separate", E_INVALIDARG);

    ComPtr<ISession> ptrSession;
    BRIDGE_HRC(ptrSession.createInprocObject(CLSID_Session), "Session");
    SafeArray<BSTR> aEnv;
    ComPtr<IProgress> ptrProgress;
    HRESULT hrc = ptrMachine->LaunchVMProcess(ptrSession, Bstr(strType).raw(), ComSafeArrayAsInParam(aEnv), ptrProgress.asOutParam());
    if (FAILED(hrc))
    {
        com::ErrorInfo Info(ptrMachine, COM_IIDOF(IMachine));
        throw BridgeError(Info.isFullAvailable() ? Utf8Str(Info.getText()) : Utf8Str("Could not start the machine"), hrc);
    }
    bridgeWaitProgress(ptrProgress, "Starting the machine");
    ptrSession->UnlockMachine();
    j.beginObject().member("started", true).endObject();
}

static void bridgeMachineControl(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    Utf8Str strAction = bridgeParamString(hParams, "action");

    BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Shared), "Locking the machine");
    try
    {
        ComPtr<IConsole> ptrConsole;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Console)(ptrConsole.asOutParam()), "Session console");
        if (ptrConsole.isNull())
            throw BridgeError("The machine is not running", E_FAIL);

        ComPtr<IProgress> ptrProgress;
        if (strAction == "pause")
            BRIDGE_HRC(ptrConsole->Pause(), "Pause");
        else if (strAction == "resume")
            BRIDGE_HRC(ptrConsole->Resume(), "Resume");
        else if (strAction == "reset")
            BRIDGE_HRC(ptrConsole->Reset(), "Reset");
        else if (strAction == "acpiShutdown")
            BRIDGE_HRC(ptrConsole->PowerButton(), "ACPI shutdown");
        else if (strAction == "powerOff")
            BRIDGE_HRC(ptrConsole->PowerDown(ptrProgress.asOutParam()), "Power off");
        else if (strAction == "saveState")
        {
            ComPtr<IMachine> ptrSessionMachine;
            BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(ptrSessionMachine.asOutParam()), "Session machine");
            BRIDGE_HRC(ptrSessionMachine->SaveState(ptrProgress.asOutParam()), "Save state");
        }
        else
            throw BridgeError(Utf8StrFmt("Unknown action '%s'", strAction.c_str()), E_INVALIDARG);
        bridgeWaitProgress(ptrProgress, strAction.c_str());
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    j.beginObject().member("done", true).endObject();
}

static void bridgeMachineUpdate(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Write), "Locking the machine for changes");
    try
    {
        ComPtr<IMachine> ptrMutable;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(ptrMutable.asOutParam()), "Session machine");
        int64_t i64 = 0;
        if (bridgeParamInt(hParams, "memoryMB", &i64))
            BRIDGE_HRC(ptrMutable->COMSETTER(MemorySize)((ULONG)i64), "Setting memory");
        if (bridgeParamInt(hParams, "cpus", &i64))
            BRIDGE_HRC(ptrMutable->COMSETTER(CPUCount)((ULONG)i64), "Setting processors");
        Utf8Str strName = bridgeParamString(hParams, "name", false);
        if (strName.isNotEmpty())
            BRIDGE_HRC(ptrMutable->COMSETTER(Name)(Bstr(strName).raw()), "Renaming");
        Utf8Str strDesc = bridgeParamString(hParams, "description", false);
        if (strDesc.isNotEmpty())
            BRIDGE_HRC(ptrMutable->COMSETTER(Description)(Bstr(strDesc).raw()), "Setting the description");
        BRIDGE_HRC(ptrMutable->SaveSettings(), "Saving settings");
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    bridgeWriteMachineSummary(j, ptrMachine);
}

static void bridgeMachineDelete(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    /* The front end only sends this after its two-key confirmation, and it
       must repeat the machine name so a stale id can never delete another. */
    Utf8Str strConfirm = bridgeParamString(hParams, "confirmName");
    Bstr bstrName;
    ptrMachine->COMGETTER(Name)(bstrName.asOutParam());
    if (Utf8Str(bstrName) != strConfirm)
        throw BridgeError("Confirmation name does not match the machine", E_INVALIDARG);

    SafeIfaceArray<IMedium> aMedia;
    BRIDGE_HRC(ptrMachine->Unregister(CleanupMode_DetachAllReturnHardDisksOnly, ComSafeArrayAsOutParam(aMedia)), "Unregistering");
    ComPtr<IProgress> ptrProgress;
    BRIDGE_HRC(ptrMachine->DeleteConfig(ComSafeArrayAsInParam(aMedia), ptrProgress.asOutParam()), "Deleting files");
    bridgeWaitProgress(ptrProgress, "Deleting the machine");
    j.beginObject().member("deleted", true).member("media", (int64_t)aMedia.size()).endObject();
}

static void bridgeWriteSnapshot(BridgeJson &j, ComPtr<ISnapshot> &ptrSnap, int iDepth)
{
    Bstr bstrId, bstrName, bstrDesc;
    LONG64 msTime = 0;
    BOOL fOnline = FALSE;
    ptrSnap->COMGETTER(Id)(bstrId.asOutParam());
    ptrSnap->COMGETTER(Name)(bstrName.asOutParam());
    ptrSnap->COMGETTER(Description)(bstrDesc.asOutParam());
    ptrSnap->COMGETTER(TimeStamp)(&msTime);
    ptrSnap->COMGETTER(Online)(&fOnline);
    j.beginObject()
     .member("id", Utf8Str(bstrId)).member("name", Utf8Str(bstrName)).member("description", Utf8Str(bstrDesc))
     .member("timeMs", (int64_t)msTime).member("online", (bool)fOnline);
    j.key("children").beginArray();
    if (iDepth < 64)
    {
        SafeIfaceArray<ISnapshot> aChildren;
        ptrSnap->COMGETTER(Children)(ComSafeArrayAsOutParam(aChildren));
        for (size_t i = 0; i < aChildren.size(); i++)
        {
            ComPtr<ISnapshot> ptrChild = aChildren[i];
            bridgeWriteSnapshot(j, ptrChild, iDepth + 1);
        }
    }
    j.endArray().endObject();
}

static void bridgeSnapshotsList(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    ComPtr<ISnapshot> ptrRoot, ptrCurrent;
    ptrMachine->FindSnapshot(Bstr().raw(), ptrRoot.asOutParam());
    ptrMachine->COMGETTER(CurrentSnapshot)(ptrCurrent.asOutParam());
    Bstr bstrCurrent;
    if (!ptrCurrent.isNull())
        ptrCurrent->COMGETTER(Id)(bstrCurrent.asOutParam());
    j.beginObject().member("currentId", Utf8Str(bstrCurrent)).key("root");
    if (ptrRoot.isNull())
        j.null();
    else
        bridgeWriteSnapshot(j, ptrRoot, 0);
    j.endObject();
}

static void bridgeSnapshotTake(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    Utf8Str strName = bridgeParamString(hParams, "name");
    Utf8Str strDesc = bridgeParamString(hParams, "description", false);
    BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Shared), "Locking the machine");
    Bstr bstrSnapId;
    try
    {
        ComPtr<IMachine> ptrSessionMachine;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(ptrSessionMachine.asOutParam()), "Session machine");
        ComPtr<IProgress> ptrProgress;
        BRIDGE_HRC(ptrSessionMachine->TakeSnapshot(Bstr(strName).raw(), Bstr(strDesc).raw(), TRUE /*fPause*/,
                                                   bstrSnapId.asOutParam(), ptrProgress.asOutParam()), "Taking the snapshot");
        bridgeWaitProgress(ptrProgress, "Taking the snapshot");
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    j.beginObject().member("id", Utf8Str(bstrSnapId)).endObject();
}

static void bridgeMediaList(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    Utf8Str strKind = bridgeParamString(hParams, "kind", false);
    SafeIfaceArray<IMedium> aMedia;
    if (strKind == "optical")
        BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(DVDImages)(ComSafeArrayAsOutParam(aMedia)), "IVirtualBox::DVDImages");
    else if (strKind == "floppy")
        BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(FloppyImages)(ComSafeArrayAsOutParam(aMedia)), "IVirtualBox::FloppyImages");
    else
        BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(HardDisks)(ComSafeArrayAsOutParam(aMedia)), "IVirtualBox::HardDisks");

    j.beginArray();
    for (size_t i = 0; i < aMedia.size(); i++)
    {
        Bstr bstrId, bstrName, bstrLoc, bstrFmt;
        LONG64 cbLogical = 0, cbActual = 0;
        SafeArray<BSTR> aMachineIds;
        aMedia[i]->COMGETTER(Id)(bstrId.asOutParam());
        aMedia[i]->COMGETTER(Name)(bstrName.asOutParam());
        aMedia[i]->COMGETTER(Location)(bstrLoc.asOutParam());
        aMedia[i]->COMGETTER(Format)(bstrFmt.asOutParam());
        aMedia[i]->COMGETTER(LogicalSize)(&cbLogical);
        aMedia[i]->COMGETTER(Size)(&cbActual);
        aMedia[i]->COMGETTER(MachineIds)(ComSafeArrayAsOutParam(aMachineIds));
        j.beginObject()
         .member("id", Utf8Str(bstrId)).member("name", Utf8Str(bstrName)).member("location", Utf8Str(bstrLoc))
         .member("format", Utf8Str(bstrFmt)).member("logicalSize", (int64_t)cbLogical).member("size", (int64_t)cbActual);
        j.key("attachedTo").beginArray();
        for (size_t k = 0; k < aMachineIds.size(); k++)
        {
            ComPtr<IMachine> ptrOwner;
            Bstr bstrOwner;
            if (SUCCEEDED(pCtx->ptrVBox->FindMachine(aMachineIds[k], ptrOwner.asOutParam())) && !ptrOwner.isNull())
                ptrOwner->COMGETTER(Name)(bstrOwner.asOutParam());
            j.value(bstrOwner.isNotEmpty() ? Utf8Str(bstrOwner) : Utf8Str(aMachineIds[k]));
        }
        j.endArray().endObject();
    }
    j.endArray();
}

static void bridgeNetworksList(PBRIDGECTX pCtx, RTJSONVAL, BridgeJson &j)
{
    j.beginObject().key("natNetworks").beginArray();
    SafeIfaceArray<INATNetwork> aNat;
    pCtx->ptrVBox->COMGETTER(NATNetworks)(ComSafeArrayAsOutParam(aNat));
    for (size_t i = 0; i < aNat.size(); i++)
    {
        Bstr bstrName, bstrCidr;
        BOOL fEnabled = FALSE, fDhcp = FALSE;
        aNat[i]->COMGETTER(NetworkName)(bstrName.asOutParam());
        aNat[i]->COMGETTER(Network)(bstrCidr.asOutParam());
        aNat[i]->COMGETTER(Enabled)(&fEnabled);
        aNat[i]->COMGETTER(NeedDhcpServer)(&fDhcp);
        j.beginObject().member("name", Utf8Str(bstrName)).member("cidr", Utf8Str(bstrCidr))
         .member("enabled", (bool)fEnabled).member("dhcp", (bool)fDhcp).endObject();
    }
    j.endArray();

    j.key("hostInterfaces").beginArray();
    ComPtr<IHost> ptrHost;
    if (SUCCEEDED(pCtx->ptrVBox->COMGETTER(Host)(ptrHost.asOutParam())) && !ptrHost.isNull())
    {
        SafeIfaceArray<IHostNetworkInterface> aIfs;
        ptrHost->COMGETTER(NetworkInterfaces)(ComSafeArrayAsOutParam(aIfs));
        for (size_t i = 0; i < aIfs.size(); i++)
        {
            Bstr bstrName, bstrIp, bstrMask;
            HostNetworkInterfaceType_T enmType = HostNetworkInterfaceType_Bridged;
            BOOL fDhcp = FALSE;
            aIfs[i]->COMGETTER(Name)(bstrName.asOutParam());
            aIfs[i]->COMGETTER(IPAddress)(bstrIp.asOutParam());
            aIfs[i]->COMGETTER(NetworkMask)(bstrMask.asOutParam());
            aIfs[i]->COMGETTER(InterfaceType)(&enmType);
            aIfs[i]->COMGETTER(DHCPEnabled)(&fDhcp);
            j.beginObject().member("name", Utf8Str(bstrName)).member("ip", Utf8Str(bstrIp)).member("mask", Utf8Str(bstrMask))
             .member("kind", enmType == HostNetworkInterfaceType_HostOnly ? "hostOnly" : "bridged")
             .member("dhcp", (bool)fDhcp).endObject();
        }
    }
    j.endArray().endObject();
}

static void bridgeLogsRead(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    int64_t iIdx = 0, offStart = 0;
    bridgeParamInt(hParams, "index", &iIdx);
    bridgeParamInt(hParams, "offset", &offStart);
    Bstr bstrFile;
    ptrMachine->QueryLogFilename((ULONG)iIdx, bstrFile.asOutParam());

    SafeArray<BYTE> aData;
    BRIDGE_HRC(ptrMachine->ReadLog((ULONG)iIdx, offStart, _512K, ComSafeArrayAsOutParam(aData)), "Reading the log");
    /* Logs are mostly UTF-8 but are not guaranteed to be; purge anything invalid. */
    Utf8Str strText((const char *)aData.raw(), aData.size());
    RTStrPurgeEncoding(strText.mutableRaw());
    j.beginObject().member("file", Utf8Str(bstrFile)).member("offset", offStart)
     .member("nextOffset", offStart + (int64_t)aData.size()).member("text", strText).endObject();
}

static void bridgeExtPacksList(PBRIDGECTX pCtx, RTJSONVAL, BridgeJson &j)
{
    ComPtr<IExtPackManager> ptrMgr;
    BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(ExtensionPackManager)(ptrMgr.asOutParam()), "IVirtualBox::ExtensionPackManager");
    SafeIfaceArray<IExtPack> aPacks;
    ptrMgr->COMGETTER(InstalledExtPacks)(ComSafeArrayAsOutParam(aPacks));
    j.beginArray();
    for (size_t i = 0; i < aPacks.size(); i++)
    {
        Bstr bstrName, bstrVersion, bstrDesc, bstrWhy;
        BOOL fUsable = FALSE;
        aPacks[i]->COMGETTER(Name)(bstrName.asOutParam());
        aPacks[i]->COMGETTER(Version)(bstrVersion.asOutParam());
        aPacks[i]->COMGETTER(Description)(bstrDesc.asOutParam());
        aPacks[i]->COMGETTER(Usable)(&fUsable);
        aPacks[i]->COMGETTER(WhyUnusable)(bstrWhy.asOutParam());
        j.beginObject().member("name", Utf8Str(bstrName)).member("version", Utf8Str(bstrVersion))
         .member("description", Utf8Str(bstrDesc)).member("usable", (bool)fUsable)
         .member("whyUnusable", Utf8Str(bstrWhy)).endObject();
    }
    j.endArray();
}

static void bridgeMetricsSample(PBRIDGECTX pCtx, RTJSONVAL, BridgeJson &j)
{
    ComPtr<IPerformanceCollector> ptrPerf;
    BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(PerformanceCollector)(ptrPerf.asOutParam()), "IVirtualBox::PerformanceCollector");

    com::SafeArray<BSTR> aNames(2);
    Bstr("CPU/Load/User").detachTo(&aNames[0]);
    Bstr("RAM/Usage/Used").detachTo(&aNames[1]);
    com::SafeIfaceArray<IUnknown> aObjects;
    com::SafeIfaceArray<IPerformanceMetric> aAffected;
    /* Idempotent: re-arming every call keeps a single one-second sample ready. */
    ptrPerf->SetupMetrics(ComSafeArrayAsInParam(aNames), ComSafeArrayAsInParam(aObjects), 1, 1,
                          ComSafeArrayAsOutParam(aAffected));

    com::SafeArray<BSTR>    aRetNames;
    com::SafeIfaceArray<IUnknown> aRetObjects;
    com::SafeArray<BSTR>    aRetUnits;
    com::SafeArray<ULONG>   aRetScales;
    com::SafeArray<ULONG>   aRetSequenceNumbers;
    com::SafeArray<ULONG>   aRetIndices;
    com::SafeArray<ULONG>   aRetLengths;
    com::SafeArray<LONG>    aRetData;
    BRIDGE_HRC(ptrPerf->QueryMetricsData(ComSafeArrayAsInParam(aNames), ComSafeArrayAsInParam(aObjects),
                                         ComSafeArrayAsOutParam(aRetNames), ComSafeArrayAsOutParam(aRetObjects),
                                         ComSafeArrayAsOutParam(aRetUnits), ComSafeArrayAsOutParam(aRetScales),
                                         ComSafeArrayAsOutParam(aRetSequenceNumbers), ComSafeArrayAsOutParam(aRetIndices),
                                         ComSafeArrayAsOutParam(aRetLengths), ComSafeArrayAsOutParam(aRetData)),
               "Querying metrics");

    j.beginArray();
    for (size_t i = 0; i < aRetNames.size(); i++)
    {
        ComPtr<IMachine> ptrMachine;
        Bstr bstrObjId;
        if (!aRetObjects[i].isNull())
        {
            ComPtr<IUnknown> ptrObj = aRetObjects[i];
            if (SUCCEEDED(ptrObj.queryInterfaceTo(ptrMachine.asOutParam())) && !ptrMachine.isNull())
                ptrMachine->COMGETTER(Id)(bstrObjId.asOutParam());
        }
        double rdValue = 0;
        if (aRetLengths[i] > 0)
            rdValue = (double)aRetData[aRetIndices[i] + aRetLengths[i] - 1] / (aRetScales[i] ? aRetScales[i] : 1);
        j.beginObject()
         .member("metric", Utf8Str(aRetNames[i]))
         .member("object", bstrObjId.isNotEmpty() ? Utf8Str(bstrObjId) : Utf8Str("host"))
         .member("unit", Utf8Str(aRetUnits[i]))
         .member("value", rdValue)
         .endObject();
    }
    j.endArray();
}


/*********************************************************************************************************************************
*   Dispatcher                                                                                                                   *
*********************************************************************************************************************************/
typedef void FNBRIDGEHANDLER(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j);

static const struct
{
    const char      *pszMethod;
    FNBRIDGEHANDLER *pfnHandler;
} g_aHandlers[] =
{
    { "host.info",          bridgeHostInfo },
    { "machines.list",      bridgeMachinesList },
    { "machine.details",    bridgeMachineDetails },
    { "machine.start",      bridgeMachineStart },
    { "machine.control",    bridgeMachineControl },
    { "machine.update",     bridgeMachineUpdate },
    { "machine.delete",     bridgeMachineDelete },
    { "snapshots.list",     bridgeSnapshotsList },
    { "snapshots.take",     bridgeSnapshotTake },
    { "media.list",         bridgeMediaList },
    { "networks.list",      bridgeNetworksList },
    { "logs.read",          bridgeLogsRead },
    { "extpacks.list",      bridgeExtPacksList },
    { "metrics.sample",     bridgeMetricsSample },
};

static void bridgeHandleLine(PBRIDGECTX pCtx, const char *pszLine)
{
    RTJSONVAL hReq = NIL_RTJSONVAL;
    int64_t   idReq = -1;
    BridgeJson j;
    j.beginObject();

    int vrc = RTJsonParseFromString(&hReq, 0 /*fFlags*/, pszLine, NULL);
    if (RT_FAILURE(vrc))
    {
        j.key("id").null().key("error").beginObject().member("message", "Request is not valid JSON").endObject().endObject();
        RTPrintf("%s\n", j.str().c_str());
        return;
    }
    RTJsonValueQueryIntegerByName(hReq, "id", &idReq);
    j.member("id", idReq);

    RTJSONVAL hParams = NIL_RTJSONVAL;
    RTJsonValueQueryByName(hReq, "params", &hParams);

    try
    {
        Utf8Str strMethod = bridgeParamString(hReq, "method");
        FNBRIDGEHANDLER *pfn = NULL;
        for (size_t i = 0; i < RT_ELEMENTS(g_aHandlers); i++)
            if (strMethod == g_aHandlers[i].pszMethod)
                pfn = g_aHandlers[i].pfnHandler;
        if (!pfn)
            throw BridgeError(Utf8StrFmt("Unknown method '%s'", strMethod.c_str()), E_NOTIMPL);
        BridgeJson jResult;
        pfn(pCtx, hParams, jResult);
        j.key("result").raw(jResult.str());
    }
    catch (BridgeError &e)
    {
        j.key("error").beginObject().member("message", e.m_strMsg).member("hrc", (int64_t)e.m_hrc).endObject();
    }
    catch (...)
    {
        j.key("error").beginObject().member("message", "Unexpected failure in the bridge").endObject();
    }
    j.endObject();

    if (hParams != NIL_RTJSONVAL)
        RTJsonValueRelease(hParams);
    RTJsonValueRelease(hReq);

    RTPrintf("%s\n", j.str().c_str());
    RTStrmFlush(g_pStdOut);
}


int main(int argc, char **argv)
{
    int vrc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(vrc))
        return RTMsgInitFailure(vrc);

    HRESULT hrc = com::Initialize();
    if (FAILED(hrc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to initialize COM (%Rhrc)", hrc);

    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    {
        BRIDGECTX Ctx;
        hrc = Ctx.ptrClient.createInprocObject(CLSID_VirtualBoxClient);
        if (SUCCEEDED(hrc))
            hrc = Ctx.ptrClient->COMGETTER(VirtualBox)(Ctx.ptrVBox.asOutParam());
        if (SUCCEEDED(hrc))
            hrc = Ctx.ptrSession.createInprocObject(CLSID_Session);
        if (FAILED(hrc))
        {
            /* Tell the front end why, in its own protocol, before leaving. */
            RTPrintf("{\"id\":null,\"error\":{\"message\":\"Could not reach the VirtualBox service\",\"hrc\":%d}}\n", (int)hrc);
            rcExit = RTEXITCODE_FAILURE;
        }
        else
        {
            RTPrintf("{\"id\":0,\"result\":{\"ready\":true,\"version\":\"%s\"}}\n", VBOX_VERSION_STRING);
            RTStrmFlush(g_pStdOut);

            char szLine[_64K];
            for (;;)
            {
                vrc = RTStrmGetLine(g_pStdIn, szLine, sizeof(szLine));
                if (vrc == VERR_EOF)
                    break;
                if (RT_FAILURE(vrc))
                    continue; /* Oversized line: skip it rather than parse half a request. */
                if (szLine[0] != '\0')
                    bridgeHandleLine(&Ctx, szLine);
            }
        }
    }

    com::Shutdown();
    return rcExit;
}
