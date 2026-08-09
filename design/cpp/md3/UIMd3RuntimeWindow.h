/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3RuntimeWindow class declaration - Material 3 runtime window chrome.
 */

/*
 * Copyright (C) 2026 Material Virtual Machine contributors.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3RuntimeWindow_h
#define FEQT_INCLUDED_SRC_md3_UIMd3RuntimeWindow_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QMainWindow>
#include <QUuid>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class UIActionPool;
class UIMachineView;
class UIMd3StatusBar;
class UIMd3TitleBar;
class UIMiniToolBar;

/** QMainWindow extension replacing UIMachineWindowNormal with the Material 3 chrome.
  *
  * The guest surface, capture handling and visual-state machinery stay exactly
  * where they were: this class owns only the chrome around UIMachineView, so a
  * regression in the runtime path cannot originate here. The Machine, View,
  * Input, Devices and Help menus continue to be built from UIActionPoolRuntime,
  * which is what keeps every shortcut and checkable state correct. */
class UIMd3RuntimeWindow : public QMainWindow
{
    Q_OBJECT;

public:

    /** Constructs the runtime window for @a uMachineId on @a uScreenId. */
    UIMd3RuntimeWindow(const QUuid &uMachineId, ulong uScreenId, UIActionPool *pActionPool);

    /** Applies @a enmState, switching between normal, fullscreen, seamless and scaled. */
    void setVisualState(UIVisualStateType enmState);
    /** Shows or hides the menu bar. */
    void setMenuBarVisible(bool fVisible);
    /** Shows or hides the status bar. */
    void setStatusBarVisible(bool fVisible);
    /** Shows or hides the mini toolbar. */
    void setMiniToolbarVisible(bool fVisible);

private slots:

    /** Refreshes the window title for the current machine state. */
    void sltUpdateTitle();
    /** Handles a status-bar indicator click, opening the surface it stands for. */
    void sltIndicatorActivated(IndicatorType enmType);

private:

    void prepare();
    void prepareMenuBar();
    void prepareStatusBar();
    void prepareMiniToolbar();

    QUuid           m_uMachineId;
    ulong           m_uScreenId;
    UIActionPool   *m_pActionPool;
    UIMd3TitleBar  *m_pTitleBar;
    UIMachineView  *m_pMachineView;
    UIMd3StatusBar *m_pStatusBar;
    UIMiniToolBar  *m_pMiniToolbar;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3RuntimeWindow_h */
