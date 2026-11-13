/* $Id$ */
/** @file
 * BridgeCore - Request handlers shared by VBoxMaterialBridge (stdio helper)
 * and VBoxMaterialBridgeNode (in-process module of the Material shell).
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
#include <iprt/path.h>
#include <iprt/base64.h>

#include "BridgeCore.h"

using namespace com;


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
        /* Copy runs of plain characters in one go: frames are megabytes of base64. */
        const char *pszRun = psz;
        for (const unsigned char *pb = (const unsigned char *)psz; ; pb++)
        {
            unsigned char const ch = *pb;
            if (ch >= 0x20 && ch != '"' && ch != '\\')
                continue;
            if ((const char *)pb > pszRun)
                m_str.append(pszRun, (size_t)((const char *)pb - pszRun));
            if (ch == '\0')
                break;
            switch (ch)
            {
                case '"':  m_str.append("\\\""); break;
                case '\\': m_str.append("\\\\"); break;
                case '\n': m_str.append("\\n"); break;
                case '\r': m_str.append("\\r"); break;
                case '\t': m_str.append("\\t"); break;
                default:   m_str.appendPrintf("\\u%04x", ch); break;
            }
            pszRun = (const char *)pb + 1;
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

static const char *bridgeAttachmentName(NetworkAttachmentType_T enmAttach)
{
    switch (enmAttach)
    {
        case NetworkAttachmentType_NAT:        return "nat";
        case NetworkAttachmentType_Bridged:    return "bridged";
        case NetworkAttachmentType_Internal:   return "internal";
        case NetworkAttachmentType_HostOnly:   return "hostOnly";
        case NetworkAttachmentType_NATNetwork: return "natNetwork";
        default:                               return "none";
    }
}

static NetworkAttachmentType_T bridgeAttachmentFromName(const Utf8Str &str)
{
    if (str == "nat")        return NetworkAttachmentType_NAT;
    if (str == "bridged")    return NetworkAttachmentType_Bridged;
    if (str == "internal")   return NetworkAttachmentType_Internal;
    if (str == "hostOnly")   return NetworkAttachmentType_HostOnly;
    if (str == "natNetwork") return NetworkAttachmentType_NATNetwork;
    if (str == "none")       return NetworkAttachmentType_Null;
    throw BridgeError(Utf8StrFmt("Unknown network attachment '%s'", str.c_str()), E_INVALIDARG);
}

static const char *bridgeGraphicsName(GraphicsControllerType_T enmType)
{
    switch (enmType)
    {
        case GraphicsControllerType_VBoxVGA:  return "VBoxVGA";
        case GraphicsControllerType_VMSVGA:   return "VMSVGA";
        case GraphicsControllerType_VBoxSVGA: return "VBoxSVGA";
        default:                              return "None";
    }
}

static GraphicsControllerType_T bridgeGraphicsFromName(const Utf8Str &str)
{
    if (str == "VBoxVGA")  return GraphicsControllerType_VBoxVGA;
    if (str == "VMSVGA")   return GraphicsControllerType_VMSVGA;
    if (str == "VBoxSVGA") return GraphicsControllerType_VBoxSVGA;
    throw BridgeError(Utf8StrFmt("Unknown graphics controller '%s'", str.c_str()), E_INVALIDARG);
}

static bool bridgeParamBool(RTJSONVAL hParams, const char *pszName, bool *pf)
{
    return hParams != NIL_RTJSONVAL && RT_SUCCESS(RTJsonValueQueryBooleanByName(hParams, pszName, pf));
}

/** Calls pfnItem for every object in the named array parameter. */
template<typename Fn> static void bridgeForEachObject(RTJSONVAL hParams, const char *pszName, Fn fnItem)
{
    RTJSONVAL hArray = NIL_RTJSONVAL;
    if (hParams == NIL_RTJSONVAL || RT_FAILURE(RTJsonValueQueryByName(hParams, pszName, &hArray)))
        return;
    unsigned const cItems = RTJsonValueGetArraySize(hArray);
    for (unsigned i = 0; i < cItems && i < 64; i++)
    {
        RTJSONVAL hItem = NIL_RTJSONVAL;
        if (RT_FAILURE(RTJsonValueQueryByIndex(hArray, i, &hItem)))
            continue;
        try
        {
            fnItem(hItem);
        }
        catch (...)
        {
            RTJsonValueRelease(hItem);
            RTJsonValueRelease(hArray);
            throw;
        }
        RTJsonValueRelease(hItem);
    }
    RTJsonValueRelease(hArray);
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

    ULONG cVramMB = 0, cMonitors = 1;
    BOOL f3D = FALSE;
    GraphicsControllerType_T enmGfx = GraphicsControllerType_Null;
    ComPtr<IGraphicsAdapter> ptrGfx;
    if (SUCCEEDED(ptrMachine->COMGETTER(GraphicsAdapter)(ptrGfx.asOutParam())) && !ptrGfx.isNull())
    {
        ptrGfx->COMGETTER(VRAMSize)(&cVramMB);
        ptrGfx->COMGETTER(MonitorCount)(&cMonitors);
        ptrGfx->COMGETTER(GraphicsControllerType)(&enmGfx);
        ptrGfx->IsFeatureEnabled(GraphicsFeature_Acceleration3D, &f3D);
    }

    FirmwareType_T enmFw = FirmwareType_BIOS;
    ComPtr<IFirmwareSettings> ptrFw;
    if (SUCCEEDED(ptrMachine->COMGETTER(FirmwareSettings)(ptrFw.asOutParam())) && !ptrFw.isNull())
        ptrFw->COMGETTER(FirmwareType)(&enmFw);

    Bstr bstrDesc, bstrFolder;
    ptrMachine->COMGETTER(Description)(bstrDesc.asOutParam());
    ptrMachine->COMGETTER(SettingsFilePath)(bstrFolder.asOutParam());

    j.beginObject().key("summary").raw(jSummary.str())
     .member("vramMB", (int64_t)cVramMB)
     .member("graphicsController", bridgeGraphicsName(enmGfx))
     .member("monitors", (int64_t)cMonitors)
     .member("accel3d", (bool)f3D)
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
            Bstr bstrLoc;
            ptrMedium->COMGETTER(Name)(bstrName.asOutParam());
            ptrMedium->COMGETTER(Location)(bstrLoc.asOutParam());
            ptrMedium->COMGETTER(LogicalSize)(&cbLogical);
            j.member("medium", Utf8Str(bstrName)).member("location", Utf8Str(bstrLoc)).member("logicalSize", (int64_t)cbLogical);
        }
        j.endObject();
    }
    j.endArray();

    /* Network adapters: "network" lists the enabled ones, "adapters" the first four slots in full. */
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
        j.beginObject().member("slot", (int64_t)iSlot).member("attachment", bridgeAttachmentName(enmAttach)).endObject();
    }
    j.endArray();

    j.key("adapters").beginArray();
    for (ULONG iSlot = 0; iSlot < 4; iSlot++)
    {
        ComPtr<INetworkAdapter> ptrNic;
        if (FAILED(ptrMachine->GetNetworkAdapter(iSlot, ptrNic.asOutParam())) || ptrNic.isNull())
            break;
        BOOL fEnabled = FALSE, fCable = FALSE;
        NetworkAttachmentType_T enmAttach = NetworkAttachmentType_Null;
        Bstr bstrMac, bstrBridged, bstrHostOnly, bstrInternal, bstrNatNet;
        ptrNic->COMGETTER(Enabled)(&fEnabled);
        ptrNic->COMGETTER(AttachmentType)(&enmAttach);
        ptrNic->COMGETTER(CableConnected)(&fCable);
        ptrNic->COMGETTER(MACAddress)(bstrMac.asOutParam());
        ptrNic->COMGETTER(BridgedInterface)(bstrBridged.asOutParam());
        ptrNic->COMGETTER(HostOnlyInterface)(bstrHostOnly.asOutParam());
        ptrNic->COMGETTER(InternalNetwork)(bstrInternal.asOutParam());
        ptrNic->COMGETTER(NATNetwork)(bstrNatNet.asOutParam());
        j.beginObject()
         .member("slot", (int64_t)iSlot).member("enabled", (bool)fEnabled).member("attachment", bridgeAttachmentName(enmAttach))
         .member("cableConnected", (bool)fCable).member("mac", Utf8Str(bstrMac))
         .member("bridgedInterface", Utf8Str(bstrBridged)).member("hostOnlyInterface", Utf8Str(bstrHostOnly))
         .member("internalNetwork", Utf8Str(bstrInternal)).member("natNetwork", Utf8Str(bstrNatNet))
         .endObject();
    }
    j.endArray();

    SafeIfaceArray<ISharedFolder> aFolders;
    ptrMachine->COMGETTER(SharedFolders)(ComSafeArrayAsOutParam(aFolders));
    j.key("sharedFolders").beginArray();
    for (size_t i = 0; i < aFolders.size(); i++)
    {
        Bstr bstrName, bstrPath, bstrMountPoint;
        BOOL fWritable = FALSE, fAutoMount = FALSE;
        aFolders[i]->COMGETTER(Name)(bstrName.asOutParam());
        aFolders[i]->COMGETTER(HostPath)(bstrPath.asOutParam());
        aFolders[i]->COMGETTER(Writable)(&fWritable);
        aFolders[i]->COMGETTER(AutoMount)(&fAutoMount);
        aFolders[i]->COMGETTER(AutoMountPoint)(bstrMountPoint.asOutParam());
        j.beginObject().member("name", Utf8Str(bstrName)).member("hostPath", Utf8Str(bstrPath))
         .member("writable", (bool)fWritable).member("autoMount", (bool)fAutoMount).member("autoMountPoint", Utf8Str(bstrMountPoint)).endObject();
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

    if (strAction == "show")
    {
        /* Brings the native VirtualBoxVM window of a running machine to the front. */
        BOOL fCanShow = FALSE;
        BRIDGE_HRC(ptrMachine->CanShowConsoleWindow(&fCanShow), "Checking the machine window");
        if (!fCanShow)
            throw BridgeError("This machine has no window to show (it may be running headless)", E_FAIL);
        LONG64 idWindow = 0;
        BRIDGE_HRC(ptrMachine->ShowConsoleWindow(&idWindow), "Showing the machine window");
        j.beginObject().member("done", true).member("windowId", (int64_t)idWindow).endObject();
        return;
    }

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

/**
 * Creates and registers a machine with the OS type's defaults, then, when
 * diskGB is given, creates a dynamically allocated VDI next to the settings
 * file and attaches it to the first free port of the default controller.
 */
static void bridgeMachineCreate(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    Utf8Str strName  = bridgeParamString(hParams, "name");
    Utf8Str strOs    = bridgeParamString(hParams, "osTypeId");
    Utf8Str strGroup = bridgeParamString(hParams, "group", false);
    int64_t cMB = 0, cCpus = 0, cGB = 0;
    bridgeParamInt(hParams, "memoryMB", &cMB);
    bridgeParamInt(hParams, "cpus", &cCpus);
    bridgeParamInt(hParams, "diskGB", &cGB);

    SafeArray<BSTR> aGroups;
    if (strGroup.isNotEmpty())
    {
        Utf8Str strPath = strGroup;
        if (strPath.c_str()[0] != '/')
            strPath = Utf8StrFmt("/%s", strGroup.c_str());
        aGroups.resize(1);
        Bstr(strPath).detachTo(&aGroups[0]);
    }

    ComPtr<IMachine> ptrMachine;
    BRIDGE_HRC(pCtx->ptrVBox->CreateMachine(Bstr().raw(), Bstr(strName).raw(), PlatformArchitecture_x86,
                                            ComSafeArrayAsInParam(aGroups), Bstr(strOs).raw(), Bstr().raw(),
                                            Bstr().raw(), Bstr().raw(), Bstr().raw(), ptrMachine.asOutParam()),
               "Creating the machine");
    BRIDGE_HRC(ptrMachine->SaveSettings(), "Saving the new machine");
    BRIDGE_HRC(pCtx->ptrVBox->RegisterMachine(ptrMachine), "Registering the machine");

    BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Write), "Locking the new machine");
    Bstr bstrDisk;
    try
    {
        ComPtr<IMachine> ptrMutable;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(ptrMutable.asOutParam()), "Session machine");
        BRIDGE_HRC(ptrMutable->ApplyDefaults(Bstr().raw()), "Applying defaults for the OS type");
        if (cMB > 0)
            BRIDGE_HRC(ptrMutable->COMSETTER(MemorySize)((ULONG)cMB), "Setting memory");
        if (cCpus > 0)
            BRIDGE_HRC(ptrMutable->COMSETTER(CPUCount)((ULONG)cCpus), "Setting processors");

        if (cGB > 0)
        {
            Bstr bstrSettings;
            ptrMutable->COMGETTER(SettingsFilePath)(bstrSettings.asOutParam());
            Utf8Str strDir(bstrSettings);
            strDir.stripFilename();
            Utf8Str strDisk = strDir;
            strDisk.append(RTPATH_SLASH_STR).append(strName).append(".vdi");

            ComPtr<IMedium> ptrMedium;
            BRIDGE_HRC(pCtx->ptrVBox->CreateMedium(Bstr("VDI").raw(), Bstr(strDisk).raw(), AccessMode_ReadWrite,
                                                   DeviceType_HardDisk, ptrMedium.asOutParam()), "Creating the disk");
            SafeArray<MediumVariant_T> aVariant(1);
            aVariant[0] = MediumVariant_Standard;
            ComPtr<IProgress> ptrProgress;
            BRIDGE_HRC(ptrMedium->CreateBaseStorage((LONG64)cGB * _1G, ComSafeArrayAsInParam(aVariant), ptrProgress.asOutParam()),
                       "Allocating the disk");
            bridgeWaitProgress(ptrProgress, "Allocating the disk");

            /* First controller the defaults created, then its first port with nothing on it. */
            SafeIfaceArray<IStorageController> aCtls;
            ptrMutable->COMGETTER(StorageControllers)(ComSafeArrayAsOutParam(aCtls));
            if (aCtls.size() == 0)
                throw BridgeError("The OS defaults created no storage controller", E_FAIL);
            Bstr bstrCtl;
            aCtls[0]->COMGETTER(Name)(bstrCtl.asOutParam());
            SafeIfaceArray<IMediumAttachment> aAtt;
            ptrMutable->COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(aAtt));
            LONG iPort = 0;
            for (bool fBusy = true; fBusy && iPort < 30; )
            {
                fBusy = false;
                for (size_t i = 0; i < aAtt.size(); i++)
                {
                    Bstr bstrAttCtl;
                    LONG iAttPort = -1;
                    aAtt[i]->COMGETTER(Controller)(bstrAttCtl.asOutParam());
                    aAtt[i]->COMGETTER(Port)(&iAttPort);
                    if (bstrAttCtl == bstrCtl && iAttPort == iPort)
                        fBusy = true;
                }
                if (fBusy)
                    iPort++;
            }
            BRIDGE_HRC(ptrMutable->AttachDevice(bstrCtl.raw(), iPort, 0, DeviceType_HardDisk, ptrMedium), "Attaching the disk");
            bstrDisk = Bstr(strDisk);
        }
        BRIDGE_HRC(ptrMutable->SaveSettings(), "Saving settings");
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();

    j.beginObject().key("machine");
    bridgeWriteMachineSummary(j, ptrMachine);
    j.member("disk", Utf8Str(bstrDisk)).endObject();
}

/**
 * Changes display, network adapters and shared folders of a powered-off
 * machine in one write lock, then saves.  Every part is optional:
 *  display: { vramMB, controller, accel3d, monitors }
 *  adapters: [{ slot, enabled, attachment, bridgedInterface, hostOnlyInterface, internalNetwork, natNetwork, cableConnected }]
 *  sharedFoldersAdd: [{ name, hostPath, writable, autoMount, autoMountPoint }]
 *  sharedFoldersRemove: [{ name }]
 */
static void bridgeMachineConfigure(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Write), "Locking the machine for changes (power it off first)");
    unsigned cChanges = 0;
    try
    {
        ComPtr<IMachine> ptrMutable;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(ptrMutable.asOutParam()), "Session machine");

        RTJSONVAL hDisplay = NIL_RTJSONVAL;
        if (RT_SUCCESS(RTJsonValueQueryByName(hParams, "display", &hDisplay)))
        {
            try
            {
                ComPtr<IGraphicsAdapter> ptrGfx;
                BRIDGE_HRC(ptrMutable->COMGETTER(GraphicsAdapter)(ptrGfx.asOutParam()), "Graphics adapter");
                int64_t i64 = 0;
                bool f = false;
                Utf8Str strCtl = bridgeParamString(hDisplay, "controller", false);
                if (strCtl.isNotEmpty())
                {
                    BRIDGE_HRC(ptrGfx->COMSETTER(GraphicsControllerType)(bridgeGraphicsFromName(strCtl)), "Setting the graphics controller");
                    cChanges++;
                }
                if (bridgeParamInt(hDisplay, "vramMB", &i64))
                {
                    BRIDGE_HRC(ptrGfx->COMSETTER(VRAMSize)((ULONG)i64), "Setting video memory");
                    cChanges++;
                }
                if (bridgeParamInt(hDisplay, "monitors", &i64))
                {
                    BRIDGE_HRC(ptrGfx->COMSETTER(MonitorCount)((ULONG)i64), "Setting the monitor count");
                    cChanges++;
                }
                if (bridgeParamBool(hDisplay, "accel3d", &f))
                {
                    BRIDGE_HRC(ptrGfx->SetFeature(GraphicsFeature_Acceleration3D, f), "Setting 3D acceleration");
                    cChanges++;
                }
            }
            catch (...)
            {
                RTJsonValueRelease(hDisplay);
                throw;
            }
            RTJsonValueRelease(hDisplay);
        }

        bridgeForEachObject(hParams, "adapters", [&](RTJSONVAL hNic)
        {
            int64_t iSlot = -1;
            if (!bridgeParamInt(hNic, "slot", &iSlot) || iSlot < 0 || iSlot > 7)
                throw BridgeError("Each adapter needs a slot from 0 to 7", E_INVALIDARG);
            ComPtr<INetworkAdapter> ptrNic;
            BRIDGE_HRC(ptrMutable->GetNetworkAdapter((ULONG)iSlot, ptrNic.asOutParam()), "Network adapter");
            bool f = false;
            if (bridgeParamBool(hNic, "enabled", &f))
                BRIDGE_HRC(ptrNic->COMSETTER(Enabled)(f), "Enabling the adapter");
            Utf8Str strAttach = bridgeParamString(hNic, "attachment", false);
            if (strAttach.isNotEmpty())
                BRIDGE_HRC(ptrNic->COMSETTER(AttachmentType)(bridgeAttachmentFromName(strAttach)), "Setting the attachment");
            Utf8Str str = bridgeParamString(hNic, "bridgedInterface", false);
            if (str.isNotEmpty())
                BRIDGE_HRC(ptrNic->COMSETTER(BridgedInterface)(Bstr(str).raw()), "Setting the bridged interface");
            str = bridgeParamString(hNic, "hostOnlyInterface", false);
            if (str.isNotEmpty())
                BRIDGE_HRC(ptrNic->COMSETTER(HostOnlyInterface)(Bstr(str).raw()), "Setting the host-only interface");
            str = bridgeParamString(hNic, "internalNetwork", false);
            if (str.isNotEmpty())
                BRIDGE_HRC(ptrNic->COMSETTER(InternalNetwork)(Bstr(str).raw()), "Setting the internal network");
            str = bridgeParamString(hNic, "natNetwork", false);
            if (str.isNotEmpty())
                BRIDGE_HRC(ptrNic->COMSETTER(NATNetwork)(Bstr(str).raw()), "Setting the NAT network");
            if (bridgeParamBool(hNic, "cableConnected", &f))
                BRIDGE_HRC(ptrNic->COMSETTER(CableConnected)(f), "Setting the cable");
            cChanges++;
        });

        bridgeForEachObject(hParams, "sharedFoldersRemove", [&](RTJSONVAL hFolder)
        {
            Utf8Str strName = bridgeParamString(hFolder, "name");
            BRIDGE_HRC(ptrMutable->RemoveSharedFolder(Bstr(strName).raw()), "Removing the shared folder");
            cChanges++;
        });

        bridgeForEachObject(hParams, "sharedFoldersAdd", [&](RTJSONVAL hFolder)
        {
            Utf8Str strName = bridgeParamString(hFolder, "name");
            Utf8Str strPath = bridgeParamString(hFolder, "hostPath");
            Utf8Str strMountPoint = bridgeParamString(hFolder, "autoMountPoint", false);
            bool fWritable = true, fAutoMount = true;
            bridgeParamBool(hFolder, "writable", &fWritable);
            bridgeParamBool(hFolder, "autoMount", &fAutoMount);
            BRIDGE_HRC(ptrMutable->CreateSharedFolder(Bstr(strName).raw(), Bstr(strPath).raw(), fWritable, fAutoMount,
                                                     Bstr(strMountPoint).raw()), "Adding the shared folder");
            cChanges++;
        });

        BRIDGE_HRC(ptrMutable->SaveSettings(), "Saving settings");
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    j.beginObject().member("changes", (int64_t)cChanges).endObject();
}

/**
 * Inserts an image into, or ejects one from, an optical or floppy drive.
 * Works on running machines too: a write lock is tried first, a shared lock
 * when the machine is running.  An empty location ejects.
 */
static void bridgeStorageMount(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    Utf8Str strController = bridgeParamString(hParams, "controller");
    Utf8Str strLocation   = bridgeParamString(hParams, "location", false);
    int64_t iPort = 0, iDevice = 0;
    bridgeParamInt(hParams, "port", &iPort);
    bridgeParamInt(hParams, "device", &iDevice);

    ComPtr<IMedium> ptrMedium;
    if (strLocation.isNotEmpty())
        BRIDGE_HRC(pCtx->ptrVBox->OpenMedium(Bstr(strLocation).raw(), DeviceType_DVD, AccessMode_ReadOnly, FALSE,
                                             ptrMedium.asOutParam()), "Opening the image");

    bool fShared = false;
    if (FAILED(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Write)))
    {
        BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Shared), "Locking the machine");
        fShared = true;
    }
    try
    {
        ComPtr<IMachine> ptrMutable;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(ptrMutable.asOutParam()), "Session machine");
        BRIDGE_HRC(ptrMutable->MountMedium(Bstr(strController).raw(), (LONG)iPort, (LONG)iDevice, ptrMedium, TRUE /*fForce*/),
                   strLocation.isNotEmpty() ? "Inserting the image" : "Ejecting the image");
        if (!fShared)
            BRIDGE_HRC(ptrMutable->SaveSettings(), "Saving settings");
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    j.beginObject().member("mounted", strLocation.isNotEmpty()).endObject();
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
*   Machine screen                                                                                                               *
*                                                                                                                                *
*   The shell has no native machine window: machines run headless and their screen is streamed through these calls.  The   *
*   front end runs one bridge process per open screen so frame traffic never queues behind library calls.                  *
*********************************************************************************************************************************/
static void bridgeScreenRequire(PBRIDGECTX pCtx, RTJSONVAL hParams)
{
    Utf8Str strId = bridgeParamString(hParams, "id");
    if (pCtx->Screen.ptrDisplay.isNull() || strId != pCtx->Screen.strMachineId)
        throw BridgeError("This screen is not attached; call screen.attach first", E_FAIL);
}

static void bridgeScreenDetachInternal(PBRIDGECTX pCtx)
{
    pCtx->Screen.ptrMouse.setNull();
    pCtx->Screen.ptrKeyboard.setNull();
    pCtx->Screen.ptrDisplay.setNull();
    if (!pCtx->Screen.ptrSession.isNull())
        pCtx->Screen.ptrSession->UnlockMachine();
    pCtx->Screen.ptrSession.setNull();
    pCtx->Screen.strMachineId.setNull();
}

static void bridgeScreenAttach(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    bridgeScreenDetachInternal(pCtx);

    ComPtr<ISession> ptrSession;
    BRIDGE_HRC(ptrSession.createInprocObject(CLSID_Session), "Session");
    BRIDGE_HRC(ptrMachine->LockMachine(ptrSession, LockType_Shared), "Attaching to the machine");
    ComPtr<IConsole> ptrConsole;
    ptrSession->COMGETTER(Console)(ptrConsole.asOutParam());
    if (ptrConsole.isNull())
    {
        ptrSession->UnlockMachine();
        throw BridgeError("The machine is not running", E_FAIL);
    }
    pCtx->Screen.ptrSession = ptrSession;
    ptrConsole->COMGETTER(Display)(pCtx->Screen.ptrDisplay.asOutParam());
    ptrConsole->COMGETTER(Keyboard)(pCtx->Screen.ptrKeyboard.asOutParam());
    ptrConsole->COMGETTER(Mouse)(pCtx->Screen.ptrMouse.asOutParam());
    pCtx->Screen.strMachineId = bridgeParamString(hParams, "id");
    if (pCtx->Screen.ptrDisplay.isNull())
    {
        bridgeScreenDetachInternal(pCtx);
        throw BridgeError("The machine has no display", E_FAIL);
    }

    ULONG cx = 0, cy = 0, cBits = 0;
    LONG xOrigin = 0, yOrigin = 0;
    GuestMonitorStatus_T enmStatus = GuestMonitorStatus_Enabled;
    pCtx->Screen.ptrDisplay->GetScreenResolution(0, &cx, &cy, &cBits, &xOrigin, &yOrigin, &enmStatus);
    j.beginObject().member("width", (int64_t)cx).member("height", (int64_t)cy).endObject();
}

static void bridgeScreenFrame(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    bridgeScreenRequire(pCtx, hParams);
    ULONG cx = 0, cy = 0, cBits = 0;
    LONG xOrigin = 0, yOrigin = 0;
    GuestMonitorStatus_T enmStatus = GuestMonitorStatus_Enabled;
    BRIDGE_HRC(pCtx->Screen.ptrDisplay->GetScreenResolution(0, &cx, &cy, &cBits, &xOrigin, &yOrigin, &enmStatus), "Reading the screen size");
    if (cx == 0 || cy == 0)
    {
        j.beginObject().member("width", (int64_t)0).member("height", (int64_t)0).member("format", "none").member("data", "").endObject();
        return;
    }

    SafeArray<BYTE> aPng;
    BRIDGE_HRC(pCtx->Screen.ptrDisplay->TakeScreenShotToArray(0, cx, cy, BitmapFormat_PNG, ComSafeArrayAsOutParam(aPng)), "Capturing the screen");

    if (pCtx->fBinaryFrames)
    {
        /* In-process callers take the PNG bytes as they are: no base64, no JSON text. */
        pCtx->abFrame.resize(aPng.size());
        if (aPng.size())
            memcpy(&pCtx->abFrame[0], aPng.raw(), aPng.size());
        j.beginObject().member("width", (int64_t)cx).member("height", (int64_t)cy).member("format", "png").member("data", "").endObject();
        return;
    }

    size_t const cchB64 = RTBase64EncodedLengthEx(aPng.size(), RTBASE64_FLAGS_NO_LINE_BREAKS);
    char *pszB64 = (char *)RTMemAlloc(cchB64 + 1);
    if (!pszB64)
        throw BridgeError("Out of memory encoding the frame", E_OUTOFMEMORY);
    size_t cchActual = 0;
    int vrc = RTBase64EncodeEx(aPng.raw(), aPng.size(), RTBASE64_FLAGS_NO_LINE_BREAKS, pszB64, cchB64 + 1, &cchActual);
    if (RT_FAILURE(vrc))
    {
        RTMemFree(pszB64);
        throw BridgeError("Could not encode the frame", E_FAIL);
    }
    j.beginObject().member("width", (int64_t)cx).member("height", (int64_t)cy).member("format", "png").member("data", pszB64).endObject();
    RTMemFree(pszB64);
}

static void bridgeScreenKeys(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    bridgeScreenRequire(pCtx, hParams);
    RTJSONVAL hCodes = NIL_RTJSONVAL;
    if (RT_FAILURE(RTJsonValueQueryByName(hParams, "scancodes", &hCodes)))
        throw BridgeError("Missing array parameter 'scancodes'", E_INVALIDARG);
    unsigned const cCodes = RTJsonValueGetArraySize(hCodes);
    SafeArray<LONG> aCodes(RT_MIN(cCodes, 256U));
    for (unsigned i = 0; i < aCodes.size(); i++)
    {
        RTJSONVAL hCode = NIL_RTJSONVAL;
        int64_t i64 = 0;
        if (RT_SUCCESS(RTJsonValueQueryByIndex(hCodes, i, &hCode)))
        {
            RTJsonValueQueryInteger(hCode, &i64);
            RTJsonValueRelease(hCode);
        }
        aCodes[i] = (LONG)(i64 & 0xff);
    }
    RTJsonValueRelease(hCodes);
    ULONG cStored = 0;
    BRIDGE_HRC(pCtx->Screen.ptrKeyboard->PutScancodes(ComSafeArrayAsInParam(aCodes), &cStored), "Sending keys");
    j.beginObject().member("stored", (int64_t)cStored).endObject();
}

static void bridgeScreenMouse(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    bridgeScreenRequire(pCtx, hParams);
    int64_t x = 0, y = 0, dz = 0, dw = 0, fButtons = 0;
    bridgeParamInt(hParams, "x", &x);
    bridgeParamInt(hParams, "y", &y);
    bridgeParamInt(hParams, "dz", &dz);
    bridgeParamInt(hParams, "dw", &dw);
    bridgeParamInt(hParams, "buttons", &fButtons);
    /* The Main API counts absolute positions from 1. */
    BRIDGE_HRC(pCtx->Screen.ptrMouse->PutMouseEventAbsolute((LONG)x + 1, (LONG)y + 1, (LONG)dz, (LONG)dw, (LONG)(fButtons & 0x1f)), "Moving the mouse");
    j.beginObject().member("done", true).endObject();
}

static void bridgeScreenResize(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    bridgeScreenRequire(pCtx, hParams);
    int64_t cx = 0, cy = 0;
    bridgeParamInt(hParams, "width", &cx);
    bridgeParamInt(hParams, "height", &cy);
    if (cx < 640 || cy < 480 || cx > 8192 || cy > 8192)
        throw BridgeError("Screen size must be between 640x480 and 8192x8192", E_INVALIDARG);
    BRIDGE_HRC(pCtx->Screen.ptrDisplay->SetVideoModeHint(0, TRUE, FALSE, 0, 0, (ULONG)cx, (ULONG)cy, 32, TRUE), "Asking the guest to resize");
    j.beginObject().member("requested", true).endObject();
}

static void bridgeScreenDetach(PBRIDGECTX pCtx, RTJSONVAL, BridgeJson &j)
{
    bridgeScreenDetachInternal(pCtx);
    j.beginObject().member("detached", true).endObject();
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
    { "machine.create",     bridgeMachineCreate },
    { "machine.configure",  bridgeMachineConfigure },
    { "storage.mount",      bridgeStorageMount },
    { "machine.delete",     bridgeMachineDelete },
    { "snapshots.list",     bridgeSnapshotsList },
    { "snapshots.take",     bridgeSnapshotTake },
    { "media.list",         bridgeMediaList },
    { "networks.list",      bridgeNetworksList },
    { "logs.read",          bridgeLogsRead },
    { "extpacks.list",      bridgeExtPacksList },
    { "metrics.sample",     bridgeMetricsSample },
    { "screen.attach",      bridgeScreenAttach },
    { "screen.frame",       bridgeScreenFrame },
    { "screen.keys",        bridgeScreenKeys },
    { "screen.mouse",       bridgeScreenMouse },
    { "screen.resize",      bridgeScreenResize },
    { "screen.detach",      bridgeScreenDetach },
};

Utf8Str bridgeHandleRequest(PBRIDGECTX pCtx, const char *pszLine)
{
    RTJSONVAL hReq = NIL_RTJSONVAL;
    int64_t   idReq = -1;
    BridgeJson j;
    j.beginObject();

    int vrc = RTJsonParseFromString(&hReq, 0 /*fFlags*/, pszLine, NULL);
    if (RT_FAILURE(vrc))
    {
        j.key("id").null().key("error").beginObject().member("message", "Request is not valid JSON").endObject().endObject();
        return j.str();
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

    return j.str();
}




int bridgeCtxInit(PBRIDGECTX pCtx)
{
    HRESULT hrc = pCtx->ptrClient.createInprocObject(CLSID_VirtualBoxClient);
    if (SUCCEEDED(hrc))
        hrc = pCtx->ptrClient->COMGETTER(VirtualBox)(pCtx->ptrVBox.asOutParam());
    if (SUCCEEDED(hrc))
        hrc = pCtx->ptrSession.createInprocObject(CLSID_Session);
    return (int)hrc;
}

void bridgeCtxTerm(PBRIDGECTX pCtx)
{
    bridgeScreenDetachInternal(pCtx);
    pCtx->ptrSession.setNull();
    pCtx->ptrVBox.setNull();
    pCtx->ptrClient.setNull();
}
