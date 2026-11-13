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

#include <algorithm>
#include <vector>

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
*   Every remaining settings page, snapshots, media, networks, extension packs, clone, appliances, USB, global properties        *
*********************************************************************************************************************************/
static const char *bridgeBusName(StorageBus_T enmBus)
{
    switch (enmBus)
    {
        case StorageBus_IDE:        return "IDE";
        case StorageBus_SATA:       return "SATA";
        case StorageBus_SCSI:       return "SCSI";
        case StorageBus_Floppy:     return "Floppy";
        case StorageBus_SAS:        return "SAS";
        case StorageBus_USB:        return "USB";
        case StorageBus_PCIe:       return "NVMe";
        case StorageBus_VirtioSCSI: return "VirtioSCSI";
        default:                    return "None";
    }
}

static StorageBus_T bridgeBusFromName(const Utf8Str &str)
{
    if (str == "IDE")        return StorageBus_IDE;
    if (str == "SATA")       return StorageBus_SATA;
    if (str == "SCSI")       return StorageBus_SCSI;
    if (str == "Floppy")     return StorageBus_Floppy;
    if (str == "SAS")        return StorageBus_SAS;
    if (str == "USB")        return StorageBus_USB;
    if (str == "NVMe")       return StorageBus_PCIe;
    if (str == "VirtioSCSI") return StorageBus_VirtioSCSI;
    throw BridgeError(Utf8StrFmt("Unknown storage bus '%s'", str.c_str()), E_INVALIDARG);
}

static const char *bridgeBootName(DeviceType_T enmType)
{
    switch (enmType)
    {
        case DeviceType_HardDisk: return "disk";
        case DeviceType_DVD:      return "optical";
        case DeviceType_Floppy:   return "floppy";
        case DeviceType_Network:  return "network";
        default:                  return "none";
    }
}

static DeviceType_T bridgeBootFromName(const Utf8Str &str)
{
    if (str == "disk")    return DeviceType_HardDisk;
    if (str == "optical") return DeviceType_DVD;
    if (str == "floppy")  return DeviceType_Floppy;
    if (str == "network") return DeviceType_Network;
    return DeviceType_Null;
}

static const char *bridgePortModeName(PortMode_T enmMode)
{
    switch (enmMode)
    {
        case PortMode_HostPipe:   return "hostPipe";
        case PortMode_HostDevice: return "hostDevice";
        case PortMode_RawFile:    return "rawFile";
        case PortMode_TCP:        return "tcp";
        default:                  return "disconnected";
    }
}

static PortMode_T bridgePortModeFromName(const Utf8Str &str)
{
    if (str == "hostPipe")   return PortMode_HostPipe;
    if (str == "hostDevice") return PortMode_HostDevice;
    if (str == "rawFile")    return PortMode_RawFile;
    if (str == "tcp")        return PortMode_TCP;
    return PortMode_Disconnected;
}

static const char *bridgeAudioControllerName(AudioControllerType_T enm)
{
    switch (enm)
    {
        case AudioControllerType_AC97:        return "AC97";
        case AudioControllerType_SB16:        return "SB16";
        case AudioControllerType_HDA:         return "HDA";
        case AudioControllerType_VirtioSound: return "VirtioSound";
        default:                              return "HDA";
    }
}

static AudioControllerType_T bridgeAudioControllerFromName(const Utf8Str &str)
{
    if (str == "AC97")        return AudioControllerType_AC97;
    if (str == "SB16")        return AudioControllerType_SB16;
    if (str == "VirtioSound") return AudioControllerType_VirtioSound;
    return AudioControllerType_HDA;
}

static const char *bridgeClipboardName(ClipboardMode_T enm)
{
    switch (enm)
    {
        case ClipboardMode_HostToGuest:   return "hostToGuest";
        case ClipboardMode_GuestToHost:   return "guestToHost";
        case ClipboardMode_Bidirectional: return "bidirectional";
        default:                          return "disabled";
    }
}

static ClipboardMode_T bridgeClipboardFromName(const Utf8Str &str)
{
    if (str == "hostToGuest")   return ClipboardMode_HostToGuest;
    if (str == "guestToHost")   return ClipboardMode_GuestToHost;
    if (str == "bidirectional") return ClipboardMode_Bidirectional;
    return ClipboardMode_Disabled;
}

static const char *bridgeDnDName(DnDMode_T enm)
{
    switch (enm)
    {
        case DnDMode_HostToGuest:   return "hostToGuest";
        case DnDMode_GuestToHost:   return "guestToHost";
        case DnDMode_Bidirectional: return "bidirectional";
        default:                    return "disabled";
    }
}

static DnDMode_T bridgeDnDFromName(const Utf8Str &str)
{
    if (str == "hostToGuest")   return DnDMode_HostToGuest;
    if (str == "guestToHost")   return DnDMode_GuestToHost;
    if (str == "bidirectional") return DnDMode_Bidirectional;
    return DnDMode_Disabled;
}

/** machine.settings: everything the remaining settings pages show. */
static void bridgeMachineSettings(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    Bstr bstrDesc, bstrOs;
    ptrMachine->COMGETTER(Description)(bstrDesc.asOutParam());
    ptrMachine->COMGETTER(OSTypeId)(bstrOs.asOutParam());
    j.beginObject().member("description", Utf8Str(bstrDesc)).member("osTypeId", Utf8Str(bstrOs));

    FirmwareType_T enmFw = FirmwareType_BIOS;
    ComPtr<IFirmwareSettings> ptrFw;
    if (SUCCEEDED(ptrMachine->COMGETTER(FirmwareSettings)(ptrFw.asOutParam())) && !ptrFw.isNull())
        ptrFw->COMGETTER(FirmwareType)(&enmFw);
    TpmType_T enmTpm = TpmType_None;
    ComPtr<ITrustedPlatformModule> ptrTpm;
    if (SUCCEEDED(ptrMachine->COMGETTER(TrustedPlatformModule)(ptrTpm.asOutParam())) && !ptrTpm.isNull())
        ptrTpm->COMGETTER(Type)(&enmTpm);
    BOOL fNested = FALSE;
    ComPtr<IPlatform> ptrPlatform;
    ComPtr<IPlatformX86> ptrX86;
    if (   SUCCEEDED(ptrMachine->COMGETTER(Platform)(ptrPlatform.asOutParam())) && !ptrPlatform.isNull()
        && SUCCEEDED(ptrPlatform->COMGETTER(X86)(ptrX86.asOutParam())) && !ptrX86.isNull())
        ptrX86->GetCPUProperty(CPUPropertyTypeX86_HWVirt, &fNested);
    j.key("system").beginObject()
     .member("firmware", enmFw == FirmwareType_BIOS ? "BIOS" : "EFI")
     .member("tpm", enmTpm == TpmType_v2_0 ? "2.0" : enmTpm == TpmType_v1_2 ? "1.2" : "none")
     .member("nestedVirt", (bool)fNested);
    j.key("bootOrder").beginArray();
    for (ULONG iPos = 1; iPos <= 4; iPos++)
    {
        DeviceType_T enmDev = DeviceType_Null;
        ptrMachine->GetBootOrder(iPos, &enmDev);
        j.value(bridgeBootName(enmDev));
    }
    j.endArray().endObject();

    ComPtr<IAudioSettings> ptrAudioSettings;
    ComPtr<IAudioAdapter> ptrAudio;
    j.key("audio").beginObject();
    if (   SUCCEEDED(ptrMachine->COMGETTER(AudioSettings)(ptrAudioSettings.asOutParam())) && !ptrAudioSettings.isNull()
        && SUCCEEDED(ptrAudioSettings->COMGETTER(Adapter)(ptrAudio.asOutParam())) && !ptrAudio.isNull())
    {
        BOOL fEnabled = FALSE, fIn = FALSE, fOut = FALSE;
        AudioControllerType_T enmCtl = AudioControllerType_HDA;
        ptrAudio->COMGETTER(Enabled)(&fEnabled);
        ptrAudio->COMGETTER(EnabledIn)(&fIn);
        ptrAudio->COMGETTER(EnabledOut)(&fOut);
        ptrAudio->COMGETTER(AudioController)(&enmCtl);
        j.member("enabled", (bool)fEnabled).member("input", (bool)fIn).member("output", (bool)fOut).member("controller", bridgeAudioControllerName(enmCtl));
    }
    j.endObject();

    j.key("serial").beginArray();
    for (ULONG iSlot = 0; iSlot < 4; iSlot++)
    {
        ComPtr<ISerialPort> ptrPort;
        if (FAILED(ptrMachine->GetSerialPort(iSlot, ptrPort.asOutParam())) || ptrPort.isNull())
            break;
        BOOL fEnabled = FALSE, fServer = FALSE;
        PortMode_T enmMode = PortMode_Disconnected;
        ULONG uIo = 0, uIrq = 0;
        Bstr bstrPath;
        ptrPort->COMGETTER(Enabled)(&fEnabled);
        ptrPort->COMGETTER(HostMode)(&enmMode);
        ptrPort->COMGETTER(Path)(bstrPath.asOutParam());
        ptrPort->COMGETTER(Server)(&fServer);
        ptrPort->COMGETTER(IOAddress)(&uIo);
        ptrPort->COMGETTER(IRQ)(&uIrq);
        j.beginObject().member("slot", (int64_t)iSlot).member("enabled", (bool)fEnabled).member("mode", bridgePortModeName(enmMode))
         .member("path", Utf8Str(bstrPath)).member("server", (bool)fServer).member("ioBase", (int64_t)uIo).member("irq", (int64_t)uIrq).endObject();
    }
    j.endArray();

    j.key("usb").beginObject().key("controllers").beginArray();
    SafeIfaceArray<IUSBController> aUsb;
    ptrMachine->COMGETTER(USBControllers)(ComSafeArrayAsOutParam(aUsb));
    for (size_t i = 0; i < aUsb.size(); i++)
    {
        Bstr bstrName;
        USBControllerType_T enmType = USBControllerType_Null;
        aUsb[i]->COMGETTER(Name)(bstrName.asOutParam());
        aUsb[i]->COMGETTER(Type)(&enmType);
        j.beginObject().member("name", Utf8Str(bstrName))
         .member("type", enmType == USBControllerType_XHCI ? "XHCI" : enmType == USBControllerType_EHCI ? "EHCI" : "OHCI").endObject();
    }
    j.endArray().key("filters").beginArray();
    ComPtr<IUSBDeviceFilters> ptrFilters;
    if (SUCCEEDED(ptrMachine->COMGETTER(USBDeviceFilters)(ptrFilters.asOutParam())) && !ptrFilters.isNull())
    {
        SafeIfaceArray<IUSBDeviceFilter> aFilters;
        ptrFilters->COMGETTER(DeviceFilters)(ComSafeArrayAsOutParam(aFilters));
        for (size_t i = 0; i < aFilters.size(); i++)
        {
            Bstr bstrName, bstrVendor, bstrProduct;
            BOOL fActive = FALSE;
            aFilters[i]->COMGETTER(Name)(bstrName.asOutParam());
            aFilters[i]->COMGETTER(Active)(&fActive);
            aFilters[i]->COMGETTER(VendorId)(bstrVendor.asOutParam());
            aFilters[i]->COMGETTER(ProductId)(bstrProduct.asOutParam());
            j.beginObject().member("index", (int64_t)i).member("name", Utf8Str(bstrName)).member("active", (bool)fActive)
             .member("vendorId", Utf8Str(bstrVendor)).member("productId", Utf8Str(bstrProduct)).endObject();
        }
    }
    j.endArray().endObject();

    j.key("storageControllers").beginArray();
    SafeIfaceArray<IStorageController> aCtls;
    ptrMachine->COMGETTER(StorageControllers)(ComSafeArrayAsOutParam(aCtls));
    for (size_t i = 0; i < aCtls.size(); i++)
    {
        Bstr bstrName;
        StorageBus_T enmBus = StorageBus_Null;
        ULONG cPorts = 0;
        aCtls[i]->COMGETTER(Name)(bstrName.asOutParam());
        aCtls[i]->COMGETTER(Bus)(&enmBus);
        aCtls[i]->COMGETTER(PortCount)(&cPorts);
        j.beginObject().member("name", Utf8Str(bstrName)).member("bus", bridgeBusName(enmBus)).member("ports", (int64_t)cPorts).endObject();
    }
    j.endArray();

    ClipboardMode_T enmClip = ClipboardMode_Disabled;
    ComPtr<IClipboardSettings> ptrClip;
    if (SUCCEEDED(ptrMachine->COMGETTER(Clipboard)(ptrClip.asOutParam())) && !ptrClip.isNull())
        ptrClip->COMGETTER(Mode)(&enmClip);
    DnDMode_T enmDnD = DnDMode_Disabled;
    ptrMachine->COMGETTER(DnDMode)(&enmDnD);
    j.member("clipboardMode", bridgeClipboardName(enmClip)).member("dndMode", bridgeDnDName(enmDnD)).endObject();
}

/** Finds a registered medium of any kind by its UUID. */
static ComPtr<IMedium> bridgeFindMedium(PBRIDGECTX pCtx, const Utf8Str &strId)
{
    SafeIfaceArray<IMedium> aAll[3];
    pCtx->ptrVBox->COMGETTER(HardDisks)(ComSafeArrayAsOutParam(aAll[0]));
    pCtx->ptrVBox->COMGETTER(DVDImages)(ComSafeArrayAsOutParam(aAll[1]));
    pCtx->ptrVBox->COMGETTER(FloppyImages)(ComSafeArrayAsOutParam(aAll[2]));
    for (unsigned k = 0; k < 3; k++)
        for (size_t i = 0; i < aAll[k].size(); i++)
        {
            Bstr bstrId;
            aAll[k][i]->COMGETTER(Id)(bstrId.asOutParam());
            if (Utf8Str(bstrId) == strId)
                return ComPtr<IMedium>(aAll[k][i]);
        }
    throw BridgeError(Utf8StrFmt("No medium with id '%s'", strId.c_str()), E_INVALIDARG);
}

/**
 * machine.settingsUpdate: general, system, audio, serial, USB and storage
 * controllers of a powered-off machine, in one write lock.
 */
static void bridgeMachineSettingsUpdate(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Write), "Locking the machine for changes (power it off first)");
    unsigned cChanges = 0;
    try
    {
        ComPtr<IMachine> m;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(m.asOutParam()), "Session machine");
        int64_t i64 = 0;
        bool f = false;

        Utf8Str str = bridgeParamString(hParams, "description", false);
        if (hParams != NIL_RTJSONVAL)
        {
            RTJSONVAL hTmp = NIL_RTJSONVAL;
            if (RT_SUCCESS(RTJsonValueQueryByName(hParams, "description", &hTmp)))
            {
                RTJsonValueRelease(hTmp);
                BRIDGE_HRC(m->COMSETTER(Description)(Bstr(str).raw()), "Setting the description");
                cChanges++;
            }
        }
        str = bridgeParamString(hParams, "osTypeId", false);
        if (str.isNotEmpty())
        {
            BRIDGE_HRC(m->COMSETTER(OSTypeId)(Bstr(str).raw()), "Setting the OS type");
            cChanges++;
        }

        str = bridgeParamString(hParams, "firmware", false);
        if (str.isNotEmpty())
        {
            ComPtr<IFirmwareSettings> ptrFw;
            BRIDGE_HRC(m->COMGETTER(FirmwareSettings)(ptrFw.asOutParam()), "Firmware settings");
            BRIDGE_HRC(ptrFw->COMSETTER(FirmwareType)(str == "EFI" ? FirmwareType_EFI : FirmwareType_BIOS), "Setting the firmware");
            cChanges++;
        }
        str = bridgeParamString(hParams, "tpm", false);
        if (str.isNotEmpty())
        {
            ComPtr<ITrustedPlatformModule> ptrTpm;
            BRIDGE_HRC(m->COMGETTER(TrustedPlatformModule)(ptrTpm.asOutParam()), "TPM settings");
            BRIDGE_HRC(ptrTpm->COMSETTER(Type)(str == "2.0" ? TpmType_v2_0 : str == "1.2" ? TpmType_v1_2 : TpmType_None), "Setting the TPM");
            cChanges++;
        }
        if (bridgeParamBool(hParams, "nestedVirt", &f))
        {
            ComPtr<IPlatform> ptrPlatform;
            ComPtr<IPlatformX86> ptrX86;
            BRIDGE_HRC(m->COMGETTER(Platform)(ptrPlatform.asOutParam()), "Platform settings");
            BRIDGE_HRC(ptrPlatform->COMGETTER(X86)(ptrX86.asOutParam()), "x86 settings");
            BRIDGE_HRC(ptrX86->SetCPUProperty(CPUPropertyTypeX86_HWVirt, f), "Setting nested virtualization");
            cChanges++;
        }
        {
            RTJSONVAL hBoot = NIL_RTJSONVAL;
            if (RT_SUCCESS(RTJsonValueQueryByName(hParams, "bootOrder", &hBoot)))
            {
                unsigned const cItems = RT_MIN(RTJsonValueGetArraySize(hBoot), 4U);
                for (unsigned i = 0; i < 4; i++)
                {
                    DeviceType_T enmDev = DeviceType_Null;
                    RTJSONVAL hItem = NIL_RTJSONVAL;
                    if (i < cItems && RT_SUCCESS(RTJsonValueQueryByIndex(hBoot, i, &hItem)))
                    {
                        const char *psz = NULL;
                        if (RT_SUCCESS(RTJsonValueQueryString(hItem, &psz)) && psz)
                            enmDev = bridgeBootFromName(psz);
                        RTJsonValueRelease(hItem);
                    }
                    HRESULT hrc = m->SetBootOrder(i + 1, enmDev);
                    if (FAILED(hrc))
                    {
                        RTJsonValueRelease(hBoot);
                        throw BridgeError("Setting the boot order failed", hrc);
                    }
                }
                RTJsonValueRelease(hBoot);
                cChanges++;
            }
        }

        RTJSONVAL hAudio = NIL_RTJSONVAL;
        if (RT_SUCCESS(RTJsonValueQueryByName(hParams, "audio", &hAudio)))
        {
            try
            {
                ComPtr<IAudioSettings> ptrAs;
                ComPtr<IAudioAdapter> ptrA;
                BRIDGE_HRC(m->COMGETTER(AudioSettings)(ptrAs.asOutParam()), "Audio settings");
                BRIDGE_HRC(ptrAs->COMGETTER(Adapter)(ptrA.asOutParam()), "Audio adapter");
                if (bridgeParamBool(hAudio, "enabled", &f)) BRIDGE_HRC(ptrA->COMSETTER(Enabled)(f), "Enabling audio");
                if (bridgeParamBool(hAudio, "input", &f))   BRIDGE_HRC(ptrA->COMSETTER(EnabledIn)(f), "Setting audio input");
                if (bridgeParamBool(hAudio, "output", &f))  BRIDGE_HRC(ptrA->COMSETTER(EnabledOut)(f), "Setting audio output");
                Utf8Str strCtl = bridgeParamString(hAudio, "controller", false);
                if (strCtl.isNotEmpty())
                    BRIDGE_HRC(ptrA->COMSETTER(AudioController)(bridgeAudioControllerFromName(strCtl)), "Setting the audio controller");
                cChanges++;
            }
            catch (...)
            {
                RTJsonValueRelease(hAudio);
                throw;
            }
            RTJsonValueRelease(hAudio);
        }

        bridgeForEachObject(hParams, "serial", [&](RTJSONVAL hPort)
        {
            int64_t iSlot = -1;
            if (!bridgeParamInt(hPort, "slot", &iSlot) || iSlot < 0 || iSlot > 3)
                throw BridgeError("Each serial port needs a slot from 0 to 3", E_INVALIDARG);
            ComPtr<ISerialPort> ptrPort;
            BRIDGE_HRC(m->GetSerialPort((ULONG)iSlot, ptrPort.asOutParam()), "Serial port");
            bool fEn = false, fSrv = false;
            int64_t iVal = 0;
            Utf8Str strPath = bridgeParamString(hPort, "path", false);
            if (strPath.isNotEmpty())
                BRIDGE_HRC(ptrPort->COMSETTER(Path)(Bstr(strPath).raw()), "Setting the port path");
            Utf8Str strMode = bridgeParamString(hPort, "mode", false);
            if (strMode.isNotEmpty())
                BRIDGE_HRC(ptrPort->COMSETTER(HostMode)(bridgePortModeFromName(strMode)), "Setting the port mode");
            if (bridgeParamBool(hPort, "server", &fSrv)) BRIDGE_HRC(ptrPort->COMSETTER(Server)(fSrv), "Setting server mode");
            if (bridgeParamInt(hPort, "ioBase", &iVal))  BRIDGE_HRC(ptrPort->COMSETTER(IOAddress)((ULONG)iVal), "Setting the I/O port");
            if (bridgeParamInt(hPort, "irq", &iVal))     BRIDGE_HRC(ptrPort->COMSETTER(IRQ)((ULONG)iVal), "Setting the IRQ");
            if (bridgeParamBool(hPort, "enabled", &fEn)) BRIDGE_HRC(ptrPort->COMSETTER(Enabled)(fEn), "Enabling the port");
            cChanges++;
        });

        RTJSONVAL hUsb = NIL_RTJSONVAL;
        if (RT_SUCCESS(RTJsonValueQueryByName(hParams, "usb", &hUsb)))
        {
            try
            {
                Utf8Str strType = bridgeParamString(hUsb, "controller", false);
                if (strType.isNotEmpty())
                {
                    SafeIfaceArray<IUSBController> aUsb;
                    m->COMGETTER(USBControllers)(ComSafeArrayAsOutParam(aUsb));
                    for (size_t i = 0; i < aUsb.size(); i++)
                    {
                        Bstr bstrName;
                        aUsb[i]->COMGETTER(Name)(bstrName.asOutParam());
                        BRIDGE_HRC(m->RemoveUSBController(bstrName.raw()), "Removing the USB controller");
                    }
                    ComPtr<IUSBController> ptrNew;
                    if (strType == "XHCI")
                        BRIDGE_HRC(m->AddUSBController(Bstr("xHCI").raw(), USBControllerType_XHCI, ptrNew.asOutParam()), "Adding the USB 3 controller");
                    else if (strType == "EHCI")
                    {
                        BRIDGE_HRC(m->AddUSBController(Bstr("OHCI").raw(), USBControllerType_OHCI, ptrNew.asOutParam()), "Adding the USB 1.1 controller");
                        BRIDGE_HRC(m->AddUSBController(Bstr("EHCI").raw(), USBControllerType_EHCI, ptrNew.asOutParam()), "Adding the USB 2 controller");
                    }
                    else if (strType == "OHCI")
                        BRIDGE_HRC(m->AddUSBController(Bstr("OHCI").raw(), USBControllerType_OHCI, ptrNew.asOutParam()), "Adding the USB 1.1 controller");
                    cChanges++;
                }
                ComPtr<IUSBDeviceFilters> ptrFilters;
                BRIDGE_HRC(m->COMGETTER(USBDeviceFilters)(ptrFilters.asOutParam()), "USB filters");
                /* Removals by index, highest first so earlier indexes stay valid. */
                RTJSONVAL hRemove = NIL_RTJSONVAL;
                if (RT_SUCCESS(RTJsonValueQueryByName(hUsb, "filtersRemove", &hRemove)))
                {
                    unsigned const cItems = RTJsonValueGetArraySize(hRemove);
                    std::vector<int64_t> aIdx;
                    for (unsigned i = 0; i < cItems; i++)
                    {
                        RTJSONVAL hItem = NIL_RTJSONVAL;
                        int64_t idx = -1;
                        if (RT_SUCCESS(RTJsonValueQueryByIndex(hRemove, i, &hItem)))
                        {
                            RTJsonValueQueryInteger(hItem, &idx);
                            RTJsonValueRelease(hItem);
                        }
                        if (idx >= 0)
                            aIdx.push_back(idx);
                    }
                    RTJsonValueRelease(hRemove);
                    std::sort(aIdx.begin(), aIdx.end());
                    for (size_t i = aIdx.size(); i-- > 0;)
                    {
                        ComPtr<IUSBDeviceFilter> ptrGone;
                        BRIDGE_HRC(ptrFilters->RemoveDeviceFilter((ULONG)aIdx[i], ptrGone.asOutParam()), "Removing a USB filter");
                        cChanges++;
                    }
                }
                bridgeForEachObject(hUsb, "filtersAdd", [&](RTJSONVAL hF)
                {
                    Utf8Str strName = bridgeParamString(hF, "name");
                    ComPtr<IUSBDeviceFilter> ptrF;
                    BRIDGE_HRC(ptrFilters->CreateDeviceFilter(Bstr(strName).raw(), ptrF.asOutParam()), "Creating a USB filter");
                    Utf8Str strV = bridgeParamString(hF, "vendorId", false);
                    Utf8Str strP = bridgeParamString(hF, "productId", false);
                    if (strV.isNotEmpty()) BRIDGE_HRC(ptrF->COMSETTER(VendorId)(Bstr(strV).raw()), "Setting the vendor id");
                    if (strP.isNotEmpty()) BRIDGE_HRC(ptrF->COMSETTER(ProductId)(Bstr(strP).raw()), "Setting the product id");
                    BRIDGE_HRC(ptrF->COMSETTER(Active)(TRUE), "Activating the filter");
                    SafeIfaceArray<IUSBDeviceFilter> aExisting;
                    ptrFilters->COMGETTER(DeviceFilters)(ComSafeArrayAsOutParam(aExisting));
                    BRIDGE_HRC(ptrFilters->InsertDeviceFilter((ULONG)aExisting.size(), ptrF), "Adding a USB filter");
                    cChanges++;
                });
            }
            catch (...)
            {
                RTJsonValueRelease(hUsb);
                throw;
            }
            RTJsonValueRelease(hUsb);
        }

        /* Storage: detach first, then remove controllers, add controllers, attach. */
        bridgeForEachObject(hParams, "detach", [&](RTJSONVAL hD)
        {
            Utf8Str strCtl = bridgeParamString(hD, "controller");
            int64_t iPort = 0, iDev = 0;
            bridgeParamInt(hD, "port", &iPort);
            bridgeParamInt(hD, "device", &iDev);
            BRIDGE_HRC(m->DetachDevice(Bstr(strCtl).raw(), (LONG)iPort, (LONG)iDev), "Detaching the device");
            cChanges++;
        });
        bridgeForEachObject(hParams, "controllersRemove", [&](RTJSONVAL hC)
        {
            Utf8Str strName = bridgeParamString(hC, "name");
            BRIDGE_HRC(m->RemoveStorageController(Bstr(strName).raw()), "Removing the controller");
            cChanges++;
        });
        bridgeForEachObject(hParams, "controllersAdd", [&](RTJSONVAL hC)
        {
            Utf8Str strName = bridgeParamString(hC, "name");
            Utf8Str strBus = bridgeParamString(hC, "bus");
            ComPtr<IStorageController> ptrCtl;
            BRIDGE_HRC(m->AddStorageController(Bstr(strName).raw(), bridgeBusFromName(strBus), ptrCtl.asOutParam()), "Adding the controller");
            cChanges++;
        });
        bridgeForEachObject(hParams, "attach", [&](RTJSONVAL hA)
        {
            Utf8Str strCtl = bridgeParamString(hA, "controller");
            Utf8Str strType = bridgeParamString(hA, "type");
            Utf8Str strMediumId = bridgeParamString(hA, "mediumId", false);
            int64_t iPort = 0, iDev = 0;
            bridgeParamInt(hA, "port", &iPort);
            bridgeParamInt(hA, "device", &iDev);
            ComPtr<IMedium> ptrMedium;
            if (strMediumId.isNotEmpty())
                ptrMedium = bridgeFindMedium(pCtx, strMediumId);
            DeviceType_T enmType = strType == "optical" ? DeviceType_DVD : strType == "floppy" ? DeviceType_Floppy : DeviceType_HardDisk;
            BRIDGE_HRC(m->AttachDevice(Bstr(strCtl).raw(), (LONG)iPort, (LONG)iDev, enmType, ptrMedium), "Attaching the device");
            cChanges++;
        });

        BRIDGE_HRC(m->SaveSettings(), "Saving settings");
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    j.beginObject().member("changes", (int64_t)cChanges).endObject();
}

/** machine.runtime: clipboard and drag-and-drop modes; works while running. */
static void bridgeMachineRuntime(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    bool fShared = false;
    if (FAILED(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Write)))
    {
        BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Shared), "Locking the machine");
        fShared = true;
    }
    try
    {
        ComPtr<IMachine> m;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(m.asOutParam()), "Session machine");
        Utf8Str str = bridgeParamString(hParams, "clipboardMode", false);
        if (str.isNotEmpty())
        {
            ComPtr<IClipboardSettings> ptrClip;
            BRIDGE_HRC(m->COMGETTER(Clipboard)(ptrClip.asOutParam()), "Clipboard settings");
            BRIDGE_HRC(ptrClip->COMSETTER(Mode)(bridgeClipboardFromName(str)), "Setting the clipboard mode");
        }
        str = bridgeParamString(hParams, "dndMode", false);
        if (str.isNotEmpty())
            BRIDGE_HRC(m->COMSETTER(DnDMode)(bridgeDnDFromName(str)), "Setting drag and drop");
        BRIDGE_HRC(m->SaveSettings(), "Saving settings");
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    RT_NOREF(fShared);
    j.beginObject().member("done", true).endObject();
}

static ComPtr<ISnapshot> bridgeFindSnapshot(ComPtr<IMachine> &ptrMachine, const Utf8Str &strSnapId)
{
    ComPtr<ISnapshot> ptrSnap;
    HRESULT hrc = ptrMachine->FindSnapshot(Bstr(strSnapId).raw(), ptrSnap.asOutParam());
    if (FAILED(hrc) || ptrSnap.isNull())
        throw BridgeError(Utf8StrFmt("No snapshot '%s'", strSnapId.c_str()), hrc);
    return ptrSnap;
}

/** snapshots.restore, snapshots.delete and snapshots.rename. */
static void bridgeSnapshotAction(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    Utf8Str strAction = bridgeParamString(hParams, "action");
    ComPtr<ISnapshot> ptrSnap = bridgeFindSnapshot(ptrMachine, bridgeParamString(hParams, "snapshotId"));
    if (strAction == "rename")
    {
        Utf8Str strName = bridgeParamString(hParams, "name");
        Utf8Str strDesc = bridgeParamString(hParams, "description", false);
        BRIDGE_HRC(ptrSnap->COMSETTER(Name)(Bstr(strName).raw()), "Renaming the snapshot");
        if (strDesc.isNotEmpty())
            BRIDGE_HRC(ptrSnap->COMSETTER(Description)(Bstr(strDesc).raw()), "Setting the description");
        j.beginObject().member("done", true).endObject();
        return;
    }
    if (FAILED(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Write)))
        BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Shared), "Locking the machine");
    try
    {
        ComPtr<IMachine> m;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(m.asOutParam()), "Session machine");
        ComPtr<IProgress> ptrProgress;
        if (strAction == "restore")
            BRIDGE_HRC(m->RestoreSnapshot(ptrSnap, ptrProgress.asOutParam()), "Restoring the snapshot (power the machine off first)");
        else if (strAction == "delete")
        {
            Bstr bstrId;
            ptrSnap->COMGETTER(Id)(bstrId.asOutParam());
            BRIDGE_HRC(m->DeleteSnapshot(bstrId.raw(), ptrProgress.asOutParam()), "Deleting the snapshot");
        }
        else
            throw BridgeError(Utf8StrFmt("Unknown snapshot action '%s'", strAction.c_str()), E_INVALIDARG);
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

/** media.action: resize, compact, copy, release, remove, open. */
static void bridgeMediaAction(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    Utf8Str strAction = bridgeParamString(hParams, "action");
    if (strAction == "open")
    {
        Utf8Str strLoc = bridgeParamString(hParams, "location");
        Utf8Str strKind = bridgeParamString(hParams, "kind", false);
        DeviceType_T enmType = strKind == "optical" ? DeviceType_DVD : strKind == "floppy" ? DeviceType_Floppy : DeviceType_HardDisk;
        ComPtr<IMedium> ptrMedium;
        BRIDGE_HRC(pCtx->ptrVBox->OpenMedium(Bstr(strLoc).raw(), enmType, enmType == DeviceType_HardDisk ? AccessMode_ReadWrite : AccessMode_ReadOnly,
                                             FALSE, ptrMedium.asOutParam()), "Adding the medium");
        Bstr bstrId;
        ptrMedium->COMGETTER(Id)(bstrId.asOutParam());
        j.beginObject().member("id", Utf8Str(bstrId)).endObject();
        return;
    }

    ComPtr<IMedium> ptrMedium = bridgeFindMedium(pCtx, bridgeParamString(hParams, "mediumId"));
    ComPtr<IProgress> ptrProgress;
    if (strAction == "resize")
    {
        int64_t cGB = 0;
        if (!bridgeParamInt(hParams, "sizeGB", &cGB) || cGB < 1)
            throw BridgeError("A size of at least 1 GB is required", E_INVALIDARG);
        BRIDGE_HRC(ptrMedium->Resize((LONG64)cGB * _1G, ptrProgress.asOutParam()), "Resizing (only growing is supported)");
    }
    else if (strAction == "compact")
        BRIDGE_HRC(ptrMedium->Compact(ptrProgress.asOutParam()), "Compacting");
    else if (strAction == "copy")
    {
        Utf8Str strTarget = bridgeParamString(hParams, "target");
        Utf8Str strFormat = bridgeParamString(hParams, "format", false);
        if (strFormat.isEmpty())
            strFormat = "VDI";
        ComPtr<IMedium> ptrTarget;
        BRIDGE_HRC(pCtx->ptrVBox->CreateMedium(Bstr(strFormat).raw(), Bstr(strTarget).raw(), AccessMode_ReadWrite, DeviceType_HardDisk,
                                               ptrTarget.asOutParam()), "Creating the copy");
        SafeArray<MediumVariant_T> aVariant(1);
        aVariant[0] = MediumVariant_Standard;
        BRIDGE_HRC(ptrMedium->CloneTo(ptrTarget, ComSafeArrayAsInParam(aVariant), NULL, ptrProgress.asOutParam()), "Copying");
    }
    else if (strAction == "release")
    {
        /* Detach the medium from every machine that uses it. */
        Bstr bstrMediumId;
        ptrMedium->COMGETTER(Id)(bstrMediumId.asOutParam());
        SafeArray<BSTR> aMachineIds;
        ptrMedium->COMGETTER(MachineIds)(ComSafeArrayAsOutParam(aMachineIds));
        for (size_t i = 0; i < aMachineIds.size(); i++)
        {
            ComPtr<IMachine> ptrOwner;
            BRIDGE_HRC(pCtx->ptrVBox->FindMachine(aMachineIds[i], ptrOwner.asOutParam()), "Finding the machine using it");
            BRIDGE_HRC(ptrOwner->LockMachine(pCtx->ptrSession, LockType_Write), "Locking the machine using it (power it off first)");
            try
            {
                ComPtr<IMachine> m;
                BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Machine)(m.asOutParam()), "Session machine");
                SafeIfaceArray<IMediumAttachment> aAtt;
                m->COMGETTER(MediumAttachments)(ComSafeArrayAsOutParam(aAtt));
                for (size_t k = 0; k < aAtt.size(); k++)
                {
                    ComPtr<IMedium> ptrAttached;
                    aAtt[k]->COMGETTER(Medium)(ptrAttached.asOutParam());
                    Bstr bstrAttachedId;
                    if (!ptrAttached.isNull())
                        ptrAttached->COMGETTER(Id)(bstrAttachedId.asOutParam());
                    if (bstrAttachedId == bstrMediumId)
                    {
                        Bstr bstrCtl;
                        LONG iPort = 0, iDev = 0;
                        aAtt[k]->COMGETTER(Controller)(bstrCtl.asOutParam());
                        aAtt[k]->COMGETTER(Port)(&iPort);
                        aAtt[k]->COMGETTER(Device)(&iDev);
                        BRIDGE_HRC(m->DetachDevice(bstrCtl.raw(), iPort, iDev), "Detaching the medium");
                    }
                }
                BRIDGE_HRC(m->SaveSettings(), "Saving settings");
            }
            catch (...)
            {
                pCtx->ptrSession->UnlockMachine();
                throw;
            }
            pCtx->ptrSession->UnlockMachine();
        }
    }
    else if (strAction == "remove")
    {
        bool fDeleteFile = false;
        bridgeParamBool(hParams, "deleteFile", &fDeleteFile);
        if (fDeleteFile)
            BRIDGE_HRC(ptrMedium->DeleteStorage(ptrProgress.asOutParam()), "Deleting the file");
        else
            BRIDGE_HRC(ptrMedium->Close(), "Removing from the list");
    }
    else
        throw BridgeError(Utf8StrFmt("Unknown media action '%s'", strAction.c_str()), E_INVALIDARG);
    bridgeWaitProgress(ptrProgress, strAction.c_str());
    j.beginObject().member("done", true).endObject();
}

/** networks.action: host-only and NAT network management. */
static void bridgeNetworksAction(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    Utf8Str strAction = bridgeParamString(hParams, "action");
    ComPtr<IHost> ptrHost;
    BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(Host)(ptrHost.asOutParam()), "IVirtualBox::Host");
    ComPtr<IProgress> ptrProgress;
    if (strAction == "hostOnlyCreate")
    {
        ComPtr<IHostNetworkInterface> ptrIf;
        BRIDGE_HRC(ptrHost->CreateHostOnlyNetworkInterface(ptrIf.asOutParam(), ptrProgress.asOutParam()), "Creating the host-only adapter");
        bridgeWaitProgress(ptrProgress, "Creating the host-only adapter");
        Bstr bstrName;
        if (!ptrIf.isNull())
            ptrIf->COMGETTER(Name)(bstrName.asOutParam());
        j.beginObject().member("name", Utf8Str(bstrName)).endObject();
        return;
    }
    if (strAction == "hostOnlyRemove" || strAction == "hostOnlyConfigure")
    {
        Utf8Str strName = bridgeParamString(hParams, "name");
        ComPtr<IHostNetworkInterface> ptrIf;
        BRIDGE_HRC(ptrHost->FindHostNetworkInterfaceByName(Bstr(strName).raw(), ptrIf.asOutParam()), "Finding the adapter");
        if (strAction == "hostOnlyConfigure")
        {
            Utf8Str strIp = bridgeParamString(hParams, "ip");
            Utf8Str strMask = bridgeParamString(hParams, "mask");
            BRIDGE_HRC(ptrIf->EnableStaticIPConfig(Bstr(strIp).raw(), Bstr(strMask).raw()), "Setting the address");
        }
        else
        {
            Bstr bstrId;
            ptrIf->COMGETTER(Id)(bstrId.asOutParam());
            BRIDGE_HRC(ptrHost->RemoveHostOnlyNetworkInterface(bstrId.raw(), ptrProgress.asOutParam()), "Removing the adapter");
            bridgeWaitProgress(ptrProgress, "Removing the adapter");
        }
        j.beginObject().member("done", true).endObject();
        return;
    }

    Utf8Str strName = bridgeParamString(hParams, "name");
    ComPtr<INATNetwork> ptrNet;
    if (strAction == "natCreate")
    {
        BRIDGE_HRC(pCtx->ptrVBox->CreateNATNetwork(Bstr(strName).raw(), ptrNet.asOutParam()), "Creating the NAT network");
        BRIDGE_HRC(ptrNet->COMSETTER(Enabled)(TRUE), "Enabling the NAT network");
    }
    else
        BRIDGE_HRC(pCtx->ptrVBox->FindNATNetworkByName(Bstr(strName).raw(), ptrNet.asOutParam()), "Finding the NAT network");

    if (strAction == "natRemove")
    {
        BRIDGE_HRC(pCtx->ptrVBox->RemoveNATNetwork(ptrNet), "Removing the NAT network");
        j.beginObject().member("done", true).endObject();
        return;
    }
    if (strAction != "natCreate" && strAction != "natUpdate")
        throw BridgeError(Utf8StrFmt("Unknown network action '%s'", strAction.c_str()), E_INVALIDARG);

    Utf8Str strCidr = bridgeParamString(hParams, "cidr", false);
    if (strCidr.isNotEmpty())
        BRIDGE_HRC(ptrNet->COMSETTER(Network)(Bstr(strCidr).raw()), "Setting the network range");
    bool f = false;
    if (bridgeParamBool(hParams, "dhcp", &f))
        BRIDGE_HRC(ptrNet->COMSETTER(NeedDhcpServer)(f), "Setting DHCP");
    if (bridgeParamBool(hParams, "enabled", &f))
        BRIDGE_HRC(ptrNet->COMSETTER(Enabled)(f), "Enabling the network");
    bridgeForEachObject(hParams, "rulesRemove", [&](RTJSONVAL hR)
    {
        Utf8Str strRule = bridgeParamString(hR, "name");
        BRIDGE_HRC(ptrNet->RemovePortForwardRule(FALSE, Bstr(strRule).raw()), "Removing the rule");
    });
    bridgeForEachObject(hParams, "rulesAdd", [&](RTJSONVAL hR)
    {
        Utf8Str strRule = bridgeParamString(hR, "name");
        Utf8Str strProto = bridgeParamString(hR, "proto", false);
        Utf8Str strHostIp = bridgeParamString(hR, "hostIp", false);
        Utf8Str strGuestIp = bridgeParamString(hR, "guestIp");
        int64_t uHostPort = 0, uGuestPort = 0;
        if (!bridgeParamInt(hR, "hostPort", &uHostPort) || !bridgeParamInt(hR, "guestPort", &uGuestPort)
            || uHostPort < 1 || uHostPort > 65535 || uGuestPort < 1 || uGuestPort > 65535)
            throw BridgeError("Ports must be between 1 and 65535", E_INVALIDARG);
        BRIDGE_HRC(ptrNet->AddPortForwardRule(FALSE, Bstr(strRule).raw(), strProto == "udp" ? NATProtocol_UDP : NATProtocol_TCP,
                                              Bstr(strHostIp).raw(), (USHORT)uHostPort, Bstr(strGuestIp).raw(), (USHORT)uGuestPort),
                   "Adding the rule");
    });
    SafeArray<BSTR> aRules;
    ptrNet->COMGETTER(PortForwardRules4)(ComSafeArrayAsOutParam(aRules));
    j.beginObject().member("name", strName).key("rules").beginArray();
    for (size_t i = 0; i < aRules.size(); i++)
        j.value(Utf8Str(aRules[i]));
    j.endArray().endObject();
}

/** extpacks.action: inspect a file (with its license), install, uninstall. */
static void bridgeExtPacksAction(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    Utf8Str strAction = bridgeParamString(hParams, "action");
    ComPtr<IExtPackManager> ptrMgr;
    BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(ExtensionPackManager)(ptrMgr.asOutParam()), "IVirtualBox::ExtensionPackManager");
    ComPtr<IProgress> ptrProgress;
    if (strAction == "uninstall")
    {
        Utf8Str strName = bridgeParamString(hParams, "name");
        BRIDGE_HRC(ptrMgr->Uninstall(Bstr(strName).raw(), FALSE, Bstr().raw(), ptrProgress.asOutParam()), "Uninstalling");
        bridgeWaitProgress(ptrProgress, "Uninstalling");
        j.beginObject().member("done", true).endObject();
        return;
    }
    Utf8Str strPath = bridgeParamString(hParams, "path");
    ComPtr<IExtPackFile> ptrFile;
    BRIDGE_HRC(ptrMgr->OpenExtPackFile(Bstr(strPath).raw(), ptrFile.asOutParam()), "Opening the extension pack");
    if (strAction == "inspect")
    {
        Bstr bstrName, bstrVersion, bstrDesc, bstrWhy, bstrLicense;
        BOOL fUsable = FALSE;
        ptrFile->COMGETTER(Name)(bstrName.asOutParam());
        ptrFile->COMGETTER(Version)(bstrVersion.asOutParam());
        ptrFile->COMGETTER(Description)(bstrDesc.asOutParam());
        ptrFile->COMGETTER(Usable)(&fUsable);
        ptrFile->COMGETTER(WhyUnusable)(bstrWhy.asOutParam());
        ptrFile->QueryLicense(Bstr().raw(), Bstr().raw(), Bstr("txt").raw(), bstrLicense.asOutParam());
        j.beginObject().member("name", Utf8Str(bstrName)).member("version", Utf8Str(bstrVersion)).member("description", Utf8Str(bstrDesc))
         .member("usable", (bool)fUsable).member("whyUnusable", Utf8Str(bstrWhy)).member("license", Utf8Str(bstrLicense)).endObject();
        return;
    }
    if (strAction != "install")
        throw BridgeError(Utf8StrFmt("Unknown extension pack action '%s'", strAction.c_str()), E_INVALIDARG);
    bool fAccepted = false;
    bridgeParamBool(hParams, "licenseAccepted", &fAccepted);
    if (!fAccepted)
        throw BridgeError("The license must be accepted first", E_INVALIDARG);
    BRIDGE_HRC(ptrFile->Install(TRUE /*fReplace*/, Bstr().raw(), ptrProgress.asOutParam()), "Installing");
    bridgeWaitProgress(ptrProgress, "Installing");
    j.beginObject().member("done", true).endObject();
}

/** machine.clone: full clone of the current state or of all states. */
static void bridgeMachineClone(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<IMachine> ptrSource = bridgeFindMachine(pCtx, hParams);
    Utf8Str strName = bridgeParamString(hParams, "name");
    Utf8Str strMode = bridgeParamString(hParams, "mode", false);
    bool fKeepMacs = false;
    bridgeParamBool(hParams, "keepMacs", &fKeepMacs);

    Bstr bstrOs;
    ptrSource->COMGETTER(OSTypeId)(bstrOs.asOutParam());
    SafeArray<BSTR> aGroups;
    ptrSource->COMGETTER(Groups)(ComSafeArrayAsOutParam(aGroups));
    ComPtr<IMachine> ptrTarget;
    BRIDGE_HRC(pCtx->ptrVBox->CreateMachine(Bstr().raw(), Bstr(strName).raw(), PlatformArchitecture_x86, ComSafeArrayAsInParam(aGroups),
                                            bstrOs.raw(), Bstr().raw(), Bstr().raw(), Bstr().raw(), Bstr().raw(), ptrTarget.asOutParam()),
               "Creating the clone");
    SafeArray<CloneOptions_T> aOptions;
    if (fKeepMacs)
        aOptions.push_back(CloneOptions_KeepAllMACs);
    ComPtr<IProgress> ptrProgress;
    BRIDGE_HRC(ptrSource->CloneTo(ptrTarget, strMode == "all" ? CloneMode_AllStates : CloneMode_MachineState,
                                  ComSafeArrayAsInParam(aOptions), ptrProgress.asOutParam()), "Cloning");
    bridgeWaitProgress(ptrProgress, "Cloning");
    BRIDGE_HRC(pCtx->ptrVBox->RegisterMachine(ptrTarget), "Registering the clone");
    bridgeWriteMachineSummary(j, ptrTarget);
}

/** appliance.export, appliance.inspect and appliance.import (OVF and OVA). */
static void bridgeAppliance(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    Utf8Str strAction = bridgeParamString(hParams, "action");
    Utf8Str strPath = bridgeParamString(hParams, "path");
    ComPtr<IAppliance> ptrApp;
    BRIDGE_HRC(pCtx->ptrVBox->CreateAppliance(ptrApp.asOutParam()), "Creating the appliance");
    ComPtr<IProgress> ptrProgress;
    if (strAction == "export")
    {
        bridgeForEachObject(hParams, "machines", [&](RTJSONVAL hM)
        {
            ComPtr<IMachine> ptrM = bridgeFindMachine(pCtx, hM);
            ComPtr<IVirtualSystemDescription> ptrDesc;
            BRIDGE_HRC(ptrM->ExportTo(ptrApp, Bstr(strPath).raw(), ptrDesc.asOutParam()), "Preparing the machine for export");
        });
        Utf8Str strFormat = bridgeParamString(hParams, "format", false);
        if (strFormat.isEmpty())
            strFormat = "ovf-2.0";
        bool fManifest = true;
        bridgeParamBool(hParams, "manifest", &fManifest);
        SafeArray<ExportOptions_T> aOptions;
        if (fManifest)
            aOptions.push_back(ExportOptions_CreateManifest);
        BRIDGE_HRC(ptrApp->Write(Bstr(strFormat).raw(), ComSafeArrayAsInParam(aOptions), Bstr(strPath).raw(), ptrProgress.asOutParam()), "Writing the appliance");
        bridgeWaitProgress(ptrProgress, "Writing the appliance");
        j.beginObject().member("path", strPath).endObject();
        return;
    }

    BRIDGE_HRC(ptrApp->Read(Bstr(strPath).raw(), ptrProgress.asOutParam()), "Reading the appliance");
    bridgeWaitProgress(ptrProgress, "Reading the appliance");
    BRIDGE_HRC(ptrApp->Interpret(), "Interpreting the appliance");
    if (strAction == "inspect")
    {
        SafeIfaceArray<IVirtualSystemDescription> aDescs;
        ptrApp->COMGETTER(VirtualSystemDescriptions)(ComSafeArrayAsOutParam(aDescs));
        j.beginObject().key("systems").beginArray();
        for (size_t i = 0; i < aDescs.size(); i++)
        {
            SafeArray<VirtualSystemDescriptionType_T> aTypes;
            SafeArray<BSTR> aRefs, aOvf, aVBox, aExtra;
            aDescs[i]->GetDescriptionByType(VirtualSystemDescriptionType_Name, ComSafeArrayAsOutParam(aTypes), ComSafeArrayAsOutParam(aRefs),
                                            ComSafeArrayAsOutParam(aOvf), ComSafeArrayAsOutParam(aVBox), ComSafeArrayAsOutParam(aExtra));
            j.beginObject().member("name", aVBox.size() ? Utf8Str(aVBox[0]) : Utf8Str("Unnamed")).endObject();
        }
        j.endArray().key("warnings").beginArray();
        SafeArray<BSTR> aWarnings;
        ptrApp->GetWarnings(ComSafeArrayAsOutParam(aWarnings));
        for (size_t i = 0; i < aWarnings.size(); i++)
            j.value(Utf8Str(aWarnings[i]));
        j.endArray().endObject();
        return;
    }
    if (strAction != "import")
        throw BridgeError(Utf8StrFmt("Unknown appliance action '%s'", strAction.c_str()), E_INVALIDARG);
    bool fKeepMacs = false;
    bridgeParamBool(hParams, "keepMacs", &fKeepMacs);
    SafeArray<ImportOptions_T> aOptions;
    if (fKeepMacs)
        aOptions.push_back(ImportOptions_KeepAllMACs);
    BRIDGE_HRC(ptrApp->ImportMachines(ComSafeArrayAsInParam(aOptions), ptrProgress.asOutParam()), "Importing");
    bridgeWaitProgress(ptrProgress, "Importing");
    SafeArray<BSTR> aMachines;
    ptrApp->COMGETTER(Machines)(ComSafeArrayAsOutParam(aMachines));
    j.beginObject().key("machines").beginArray();
    for (size_t i = 0; i < aMachines.size(); i++)
        j.value(Utf8Str(aMachines[i]));
    j.endArray().endObject();
}

/** usb.list (host devices, and which a machine holds), usb.attach, usb.detach. */
static void bridgeUsb(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    Utf8Str strAction = bridgeParamString(hParams, "action");
    if (strAction == "list")
    {
        ComPtr<IHost> ptrHost;
        BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(Host)(ptrHost.asOutParam()), "IVirtualBox::Host");
        SafeIfaceArray<IHostUSBDevice> aDevs;
        ptrHost->COMGETTER(USBDevices)(ComSafeArrayAsOutParam(aDevs));
        j.beginArray();
        for (size_t i = 0; i < aDevs.size(); i++)
        {
            Bstr bstrId, bstrProduct, bstrMaker;
            USHORT uVendor = 0, uProduct = 0;
            USBDeviceState_T enmState = USBDeviceState_NotSupported;
            aDevs[i]->COMGETTER(Id)(bstrId.asOutParam());
            aDevs[i]->COMGETTER(Product)(bstrProduct.asOutParam());
            aDevs[i]->COMGETTER(Manufacturer)(bstrMaker.asOutParam());
            aDevs[i]->COMGETTER(VendorId)(&uVendor);
            aDevs[i]->COMGETTER(ProductId)(&uProduct);
            aDevs[i]->COMGETTER(State)(&enmState);
            const char *pszState = enmState == USBDeviceState_Captured ? "captured" : enmState == USBDeviceState_Available ? "available"
                                 : enmState == USBDeviceState_Busy ? "busy" : enmState == USBDeviceState_Held ? "held" : "unavailable";
            j.beginObject().member("id", Utf8Str(bstrId)).member("product", Utf8Str(bstrProduct)).member("manufacturer", Utf8Str(bstrMaker))
             .member("vendorId", Utf8StrFmt("%04x", uVendor)).member("productId", Utf8StrFmt("%04x", uProduct)).member("state", pszState).endObject();
        }
        j.endArray();
        return;
    }
    ComPtr<IMachine> ptrMachine = bridgeFindMachine(pCtx, hParams);
    Utf8Str strDevice = bridgeParamString(hParams, "deviceId");
    BRIDGE_HRC(ptrMachine->LockMachine(pCtx->ptrSession, LockType_Shared), "Locking the running machine");
    try
    {
        ComPtr<IConsole> ptrConsole;
        BRIDGE_HRC(pCtx->ptrSession->COMGETTER(Console)(ptrConsole.asOutParam()), "Session console");
        if (ptrConsole.isNull())
            throw BridgeError("The machine is not running", E_FAIL);
        if (strAction == "attach")
            BRIDGE_HRC(ptrConsole->AttachUSBDevice(Bstr(strDevice).raw(), Bstr().raw()), "Attaching the USB device");
        else if (strAction == "detach")
        {
            ComPtr<IUSBDevice> ptrGone;
            BRIDGE_HRC(ptrConsole->DetachUSBDevice(Bstr(strDevice).raw(), ptrGone.asOutParam()), "Detaching the USB device");
        }
        else
            throw BridgeError(Utf8StrFmt("Unknown USB action '%s'", strAction.c_str()), E_INVALIDARG);
    }
    catch (...)
    {
        pCtx->ptrSession->UnlockMachine();
        throw;
    }
    pCtx->ptrSession->UnlockMachine();
    j.beginObject().member("done", true).endObject();
}

/** system.properties: read, and optionally change, the default machine folder. */
static void bridgeSystemProperties(PBRIDGECTX pCtx, RTJSONVAL hParams, BridgeJson &j)
{
    ComPtr<ISystemProperties> ptrProps;
    BRIDGE_HRC(pCtx->ptrVBox->COMGETTER(SystemProperties)(ptrProps.asOutParam()), "IVirtualBox::SystemProperties");
    Utf8Str strFolder = bridgeParamString(hParams, "defaultMachineFolder", false);
    if (strFolder.isNotEmpty())
        BRIDGE_HRC(ptrProps->COMSETTER(DefaultMachineFolder)(Bstr(strFolder).raw()), "Setting the default machine folder");
    Bstr bstrFolder;
    ptrProps->COMGETTER(DefaultMachineFolder)(bstrFolder.asOutParam());
    j.beginObject().member("defaultMachineFolder", Utf8Str(bstrFolder)).endObject();
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
    { "machine.settings",   bridgeMachineSettings },
    { "machine.settingsUpdate", bridgeMachineSettingsUpdate },
    { "machine.runtime",    bridgeMachineRuntime },
    { "machine.clone",      bridgeMachineClone },
    { "snapshots.action",   bridgeSnapshotAction },
    { "media.action",       bridgeMediaAction },
    { "networks.action",    bridgeNetworksAction },
    { "extpacks.action",    bridgeExtPacksAction },
    { "appliance",          bridgeAppliance },
    { "usb",                bridgeUsb },
    { "system.properties",  bridgeSystemProperties },
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
