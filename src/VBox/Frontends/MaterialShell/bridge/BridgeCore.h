/* $Id$ */
/** @file
 * BridgeCore - Shared state and entry points of the Material shell bridge.
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

#ifndef VBOX_INCLUDED_SRC_MaterialShell_bridge_BridgeCore_h
#define VBOX_INCLUDED_SRC_MaterialShell_bridge_BridgeCore_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/ptr.h>
#include <VBox/com/VirtualBox.h>
#include <vector>

/**
 * One bridge channel: its own Main API client, session and attached screen.
 * Every call on a channel must come from the thread that initialized it,
 * because the COM objects belong to that thread.
 */
typedef struct BRIDGECTX
{
    ComPtr<IVirtualBoxClient>  ptrClient;
    ComPtr<IVirtualBox>        ptrVBox;
    ComPtr<ISession>           ptrSession;
    /** The machine screen this channel is attached to, if any. */
    struct
    {
        com::Utf8Str                strMachineId;
        ComPtr<ISession>       ptrSession;
        ComPtr<IDisplay>       ptrDisplay;
        ComPtr<IKeyboard>      ptrKeyboard;
        ComPtr<IMouse>         ptrMouse;
    } Screen;
    /** When set, screen.frame leaves the PNG in abFrame instead of base64 JSON. */
    bool                            fBinaryFrames;
    std::vector<uint8_t>            abFrame;

    BRIDGECTX() : fBinaryFrames(false) {}
} BRIDGECTX;
typedef BRIDGECTX *PBRIDGECTX;

/** Creates the channel's Main API objects; returns the HRESULT. */
int             bridgeCtxInit(PBRIDGECTX pCtx);
/** Detaches any screen and releases the channel's Main API objects. */
void            bridgeCtxTerm(PBRIDGECTX pCtx);
/** Handles one JSON request and returns the JSON response (no newline). */
com::Utf8Str    bridgeHandleRequest(PBRIDGECTX pCtx, const char *pszRequest);

#endif /* !VBOX_INCLUDED_SRC_MaterialShell_bridge_BridgeCore_h */
