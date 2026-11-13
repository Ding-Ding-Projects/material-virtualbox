/* $Id$ */
/** @file
 * VBoxMaterialBridgeNode - In-process Node-API module of the Material shell.
 *
 * Loaded by the shell's main process with process.dlopen().  It runs the same
 * request handlers as VBoxMaterialBridge.exe, without a second process and
 * without base64 for screen frames.
 *
 * Threading: every channel ("main", "screen:<id>") owns one thread that
 * initializes COM, creates its own Main API client and runs every request for
 * that channel, because COM objects belong to the thread that made them.  A
 * JavaScript call becomes Node-API async work; its worker hands the request to
 * the channel thread and waits, so the shell's window thread never blocks.
 *
 * Node-API symbols are resolved at load time from the host executable rather
 * than linked, so this module builds with kBuild alone and works with any
 * Node-API 4 host.
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
#include <VBox/version.h>

#include <iprt/initterm.h>
#include <iprt/critsect.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/string.h>
#include <iprt/mem.h>

#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h>
#else
# include <dlfcn.h>
#endif

#include <map>
#include <list>
#include <string>

#include "BridgeCore.h"


/*********************************************************************************************************************************
*   Minimal Node-API declarations (the subset this module uses; ABI-stable)                                                      *
*********************************************************************************************************************************/
typedef struct napi_env__          *napi_env;
typedef struct napi_value__        *napi_value;
typedef struct napi_deferred__     *napi_deferred;
typedef struct napi_async_work__   *napi_async_work;
typedef struct napi_callback_info__ *napi_callback_info;
typedef int napi_status;            /* napi_ok == 0 */
typedef napi_value (*napi_callback)(napi_env env, napi_callback_info info);
typedef void (*napi_async_execute_callback)(napi_env env, void *data);
typedef void (*napi_async_complete_callback)(napi_env env, napi_status status, void *data);
typedef void (*napi_finalize)(napi_env env, void *data, void *hint);

#define BRIDGE_NAPI_FUNCTIONS(X) \
    X(napi_create_function,        (napi_env, const char *, size_t, napi_callback, void *, napi_value *)) \
    X(napi_set_named_property,     (napi_env, napi_value, const char *, napi_value)) \
    X(napi_get_cb_info,            (napi_env, napi_callback_info, size_t *, napi_value *, napi_value *, void **)) \
    X(napi_get_value_string_utf8,  (napi_env, napi_value, char *, size_t, size_t *)) \
    X(napi_create_string_utf8,     (napi_env, const char *, size_t, napi_value *)) \
    X(napi_create_buffer_copy,     (napi_env, size_t, const void *, void **, napi_value *)) \
    X(napi_create_object,          (napi_env, napi_value *)) \
    X(napi_get_undefined,          (napi_env, napi_value *)) \
    X(napi_create_promise,         (napi_env, napi_deferred *, napi_value *)) \
    X(napi_resolve_deferred,       (napi_env, napi_deferred, napi_value)) \
    X(napi_reject_deferred,        (napi_env, napi_deferred, napi_value)) \
    X(napi_create_error,           (napi_env, napi_value, napi_value, napi_value *)) \
    X(napi_throw_error,            (napi_env, const char *, const char *)) \
    X(napi_create_async_work,      (napi_env, napi_value, napi_value, napi_async_execute_callback, napi_async_complete_callback, void *, napi_async_work *)) \
    X(napi_queue_async_work,       (napi_env, napi_async_work)) \
    X(napi_delete_async_work,      (napi_env, napi_async_work))

#define BRIDGE_NAPI_DECLARE(a_Name, a_Args) typedef napi_status (*PFN_##a_Name) a_Args; static PFN_##a_Name g_pfn_##a_Name = NULL;
BRIDGE_NAPI_FUNCTIONS(BRIDGE_NAPI_DECLARE)

/** Resolves every Node-API function from the host executable. */
static bool bridgeNapiResolve(void)
{
#ifdef RT_OS_WINDOWS
    HMODULE hHost = GetModuleHandleW(NULL);
# define BRIDGE_NAPI_RESOLVE(a_Name, a_Args) \
    g_pfn_##a_Name = (PFN_##a_Name)(void *)GetProcAddress(hHost, #a_Name); \
    if (!g_pfn_##a_Name) return false;
#else
# define BRIDGE_NAPI_RESOLVE(a_Name, a_Args) \
    g_pfn_##a_Name = (PFN_##a_Name)dlsym(RTLD_DEFAULT, #a_Name); \
    if (!g_pfn_##a_Name) return false;
#endif
    BRIDGE_NAPI_FUNCTIONS(BRIDGE_NAPI_RESOLVE)
    return true;
}


/*********************************************************************************************************************************
*   Channels                                                                                                                     *
*********************************************************************************************************************************/
/** One request in flight. */
struct BRIDGEJOB
{
    std::string             strChannel;
    com::Utf8Str            strRequest;
    com::Utf8Str            strResponse;
    std::vector<uint8_t>    abFrame;
    bool                    fFrame;
    RTSEMEVENT              hDone;
    napi_deferred           hDeferred;
    napi_async_work         hWork;
};

/** A channel: its thread, its queue and its Main API context. */
struct BRIDGECHANNEL
{
    RTTHREAD                hThread;
    RTSEMEVENT              hWake;
    RTCRITSECT              CritSect;
    std::list<BRIDGEJOB *>  Queue;
    bool                    fStop;
    int                     hrcInit;
};

static RTCRITSECT                               g_CritSectChannels;
static std::map<std::string, BRIDGECHANNEL *>   g_Channels;

static DECLCALLBACK(int) bridgeChannelThread(RTTHREAD hThreadSelf, void *pvUser)
{
    RT_NOREF(hThreadSelf);
    BRIDGECHANNEL *pChan = (BRIDGECHANNEL *)pvUser;
    HRESULT hrc = com::Initialize();
    {
        BRIDGECTX Ctx;
        Ctx.fBinaryFrames = true;
        pChan->hrcInit = SUCCEEDED(hrc) ? bridgeCtxInit(&Ctx) : (int)hrc;
        for (;;)
        {
            BRIDGEJOB *pJob = NULL;
            RTCritSectEnter(&pChan->CritSect);
            if (!pChan->Queue.empty())
            {
                pJob = pChan->Queue.front();
                pChan->Queue.pop_front();
            }
            bool const fStop = pChan->fStop && pJob == NULL;
            RTCritSectLeave(&pChan->CritSect);
            if (fStop)
                break;
            if (!pJob)
            {
                RTSemEventWait(pChan->hWake, RT_INDEFINITE_WAIT);
                continue;
            }

            if (FAILED((HRESULT)pChan->hrcInit))
                pJob->strResponse = com::Utf8StrFmt("{\"id\":null,\"error\":{\"message\":\"Could not reach the VirtualBox service\",\"hrc\":%d}}",
                                                    pChan->hrcInit);
            else
            {
                Ctx.abFrame.clear();
                pJob->strResponse = bridgeHandleRequest(&Ctx, pJob->strRequest.c_str());
                if (pJob->fFrame)
                    pJob->abFrame.swap(Ctx.abFrame);
            }
            RTSemEventSignal(pJob->hDone);
        }
        bridgeCtxTerm(&Ctx);
    }
    if (SUCCEEDED(hrc))
        com::Shutdown();
    return VINF_SUCCESS;
}

static BRIDGECHANNEL *bridgeChannelGet(const std::string &strName)
{
    RTCritSectEnter(&g_CritSectChannels);
    BRIDGECHANNEL *pChan = NULL;
    std::map<std::string, BRIDGECHANNEL *>::iterator it = g_Channels.find(strName);
    if (it != g_Channels.end())
        pChan = it->second;
    else
    {
        pChan = new BRIDGECHANNEL();
        pChan->fStop   = false;
        pChan->hrcInit = 0;
        RTSemEventCreate(&pChan->hWake);
        RTCritSectInit(&pChan->CritSect);
        int vrc = RTThreadCreate(&pChan->hThread, bridgeChannelThread, pChan, 0, RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "MvbChan");
        if (RT_FAILURE(vrc))
        {
            RTCritSectDelete(&pChan->CritSect);
            RTSemEventDestroy(pChan->hWake);
            delete pChan;
            pChan = NULL;
        }
        else
            g_Channels[strName] = pChan;
    }
    RTCritSectLeave(&g_CritSectChannels);
    return pChan;
}

/** Stops a channel: its screen is detached and its thread ends. */
static void bridgeChannelStop(const std::string &strName)
{
    RTCritSectEnter(&g_CritSectChannels);
    BRIDGECHANNEL *pChan = NULL;
    std::map<std::string, BRIDGECHANNEL *>::iterator it = g_Channels.find(strName);
    if (it != g_Channels.end())
    {
        pChan = it->second;
        g_Channels.erase(it);
    }
    RTCritSectLeave(&g_CritSectChannels);
    if (!pChan)
        return;
    RTCritSectEnter(&pChan->CritSect);
    pChan->fStop = true;
    RTCritSectLeave(&pChan->CritSect);
    RTSemEventSignal(pChan->hWake);
    RTThreadWait(pChan->hThread, 30000, NULL);
    RTCritSectDelete(&pChan->CritSect);
    RTSemEventDestroy(pChan->hWake);
    delete pChan;
}


/*********************************************************************************************************************************
*   Node-API glue                                                                                                                *
*********************************************************************************************************************************/
/** Runs on a Node worker thread: hand the job to its channel and wait. */
static void bridgeJobExecute(napi_env, void *pvData)
{
    BRIDGEJOB *pJob = (BRIDGEJOB *)pvData;
    BRIDGECHANNEL *pChan = bridgeChannelGet(pJob->strChannel);
    if (!pChan)
    {
        pJob->strResponse = "{\"id\":null,\"error\":{\"message\":\"Could not start a bridge thread\"}}";
        return;
    }
    RTCritSectEnter(&pChan->CritSect);
    pChan->Queue.push_back(pJob);
    RTCritSectLeave(&pChan->CritSect);
    RTSemEventSignal(pChan->hWake);
    RTSemEventWait(pJob->hDone, RT_INDEFINITE_WAIT);
}

/** Runs on the JavaScript thread: settle the promise. */
static void bridgeJobComplete(napi_env env, napi_status status, void *pvData)
{
    BRIDGEJOB *pJob = (BRIDGEJOB *)pvData;
    napi_value hJson = NULL;
    g_pfn_napi_create_string_utf8(env, pJob->strResponse.c_str(), pJob->strResponse.length(), &hJson);
    if (status != 0)
    {
        napi_value hMsg = NULL, hErr = NULL;
        g_pfn_napi_create_string_utf8(env, "The bridge call was cancelled", (size_t)-1, &hMsg);
        g_pfn_napi_create_error(env, NULL, hMsg, &hErr);
        g_pfn_napi_reject_deferred(env, pJob->hDeferred, hErr);
    }
    else if (pJob->fFrame)
    {
        napi_value hObj = NULL, hBuf = NULL;
        void *pvIgnored = NULL;
        g_pfn_napi_create_object(env, &hObj);
        g_pfn_napi_set_named_property(env, hObj, "json", hJson);
        g_pfn_napi_create_buffer_copy(env, pJob->abFrame.size(), pJob->abFrame.empty() ? "" : (const void *)&pJob->abFrame[0], &pvIgnored, &hBuf);
        g_pfn_napi_set_named_property(env, hObj, "png", hBuf);
        g_pfn_napi_resolve_deferred(env, pJob->hDeferred, hObj);
    }
    else
        g_pfn_napi_resolve_deferred(env, pJob->hDeferred, hJson);

    g_pfn_napi_delete_async_work(env, pJob->hWork);
    RTSemEventDestroy(pJob->hDone);
    delete pJob;
}

static bool bridgeArgString(napi_env env, napi_value hVal, std::string &rStr)
{
    size_t cch = 0;
    if (g_pfn_napi_get_value_string_utf8(env, hVal, NULL, 0, &cch) != 0)
        return false;
    rStr.resize(cch + 1);
    if (g_pfn_napi_get_value_string_utf8(env, hVal, &rStr[0], cch + 1, &cch) != 0)
        return false;
    rStr.resize(cch);
    return true;
}

/** call(channel, requestJson) and frame(channel, requestJson): both return a Promise. */
static napi_value bridgeQueue(napi_env env, napi_callback_info info, bool fFrame)
{
    size_t cArgs = 2;
    napi_value ahArgs[2] = { NULL, NULL };
    g_pfn_napi_get_cb_info(env, info, &cArgs, ahArgs, NULL, NULL);
    std::string strChannel, strRequest;
    if (cArgs < 2 || !bridgeArgString(env, ahArgs[0], strChannel) || !bridgeArgString(env, ahArgs[1], strRequest))
    {
        g_pfn_napi_throw_error(env, NULL, "Expected (channel: string, request: string)");
        return NULL;
    }

    BRIDGEJOB *pJob = new BRIDGEJOB();
    pJob->strChannel = strChannel;
    pJob->strRequest = strRequest.c_str();
    pJob->fFrame     = fFrame;
    RTSemEventCreate(&pJob->hDone);

    napi_value hPromise = NULL, hName = NULL;
    g_pfn_napi_create_promise(env, &pJob->hDeferred, &hPromise);
    g_pfn_napi_create_string_utf8(env, "VBoxMaterialBridgeNode", (size_t)-1, &hName);
    g_pfn_napi_create_async_work(env, NULL, hName, bridgeJobExecute, bridgeJobComplete, pJob, &pJob->hWork);
    g_pfn_napi_queue_async_work(env, pJob->hWork);
    return hPromise;
}

static napi_value bridgeNapiCall(napi_env env, napi_callback_info info)  { return bridgeQueue(env, info, false); }
static napi_value bridgeNapiFrame(napi_env env, napi_callback_info info) { return bridgeQueue(env, info, true); }

/** close(channel): synchronous; detaches the channel's screen and ends its thread. */
static napi_value bridgeNapiClose(napi_env env, napi_callback_info info)
{
    size_t cArgs = 1;
    napi_value hArg = NULL, hUndef = NULL;
    g_pfn_napi_get_cb_info(env, info, &cArgs, &hArg, NULL, NULL);
    std::string strChannel;
    if (cArgs >= 1 && bridgeArgString(env, hArg, strChannel))
        bridgeChannelStop(strChannel);
    g_pfn_napi_get_undefined(env, &hUndef);
    return hUndef;
}

/** Module entry point looked up by Node when the module is loaded. */
extern "C" DECLEXPORT(napi_value) napi_register_module_v1(napi_env env, napi_value hExports)
{
    if (!bridgeNapiResolve())
        return hExports; /* No Node-API host: leave exports empty so the shell falls back. */

    int vrc = RTR3InitDll(RTR3INIT_FLAGS_UNOBTRUSIVE);
    if (RT_FAILURE(vrc))
    {
        g_pfn_napi_throw_error(env, NULL, "IPRT initialization failed");
        return NULL;
    }
    RTCritSectInit(&g_CritSectChannels);

    static const struct { const char *pszName; napi_callback pfn; } s_aFns[] =
    {
        { "call",  bridgeNapiCall },
        { "frame", bridgeNapiFrame },
        { "close", bridgeNapiClose },
    };
    for (size_t i = 0; i < RT_ELEMENTS(s_aFns); i++)
    {
        napi_value hFn = NULL;
        g_pfn_napi_create_function(env, s_aFns[i].pszName, (size_t)-1, s_aFns[i].pfn, NULL, &hFn);
        g_pfn_napi_set_named_property(env, hExports, s_aFns[i].pszName, hFn);
    }
    napi_value hVersion = NULL;
    g_pfn_napi_create_string_utf8(env, VBOX_VERSION_STRING, (size_t)-1, &hVersion);
    g_pfn_napi_set_named_property(env, hExports, "version", hVersion);
    return hExports;
}
