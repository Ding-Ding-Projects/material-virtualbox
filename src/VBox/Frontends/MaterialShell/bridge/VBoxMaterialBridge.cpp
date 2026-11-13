/* $Id$ */
/** @file
 * VBoxMaterialBridge - Stdio front door of the Material shell bridge.
 *
 * Reads one JSON request per line on stdin and writes one JSON response per
 * line on stdout.  The shell uses this helper when its in-process module
 * (VBoxMaterialBridgeNode) cannot be loaded or has crashed repeatedly.
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

#include <VBox/com/com.h>
#include <VBox/version.h>
#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/stream.h>
#include <iprt/string.h>

#include "BridgeCore.h"


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
        hrc = (HRESULT)bridgeCtxInit(&Ctx);
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

            static char s_szLine[_64K];
            for (;;)
            {
                vrc = RTStrmGetLine(g_pStdIn, s_szLine, sizeof(s_szLine));
                if (vrc == VERR_EOF)
                    break;
                if (RT_FAILURE(vrc))
                    continue; /* Oversized line: skip it rather than parse half a request. */
                if (s_szLine[0] == '\0')
                    continue;
                com::Utf8Str strResponse = bridgeHandleRequest(&Ctx, s_szLine);
                RTStrmWrite(g_pStdOut, strResponse.c_str(), strResponse.length());
                RTStrmWrite(g_pStdOut, "\n", 1);
                RTStrmFlush(g_pStdOut);
            }
        }
        bridgeCtxTerm(&Ctx);
    }

    com::Shutdown();
    return rcExit;
}
