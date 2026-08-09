/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3ManagerWindow class declaration - the frameless Material 3 Manager shell.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3ManagerWindow_h
#define FEQT_INCLUDED_SRC_md3_UIMd3ManagerWindow_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QMainWindow>
#include <QUuid>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class QStackedWidget;
class UIActionPool;
class UIChooser;
class UIMd3NavigationRail;
class UIMd3SearchField;
class UIMd3TabStrip;
class UIMd3TitleBar;
class UIToolPane;
class UIVirtualMachineItem;

/** QMainWindow extension replacing UIVirtualBoxManager with the Material 3 shell.
  *
  * The window keeps the existing manager contract intact: it still owns a
  * UIActionPool, still drives UIChooser and UIToolPane, and still emits the same
  * signals the rest of the frontend listens to. What changes is the chrome — a
  * frameless Material 3 title bar, a workspace tab strip, a navigation rail, and
  * the global-memory layer wired across all of them. */
class UIMd3ManagerWindow : public QMainWindow
{
    Q_OBJECT;

signals:

    /** Notifies about chooser-pane selection change. */
    void sigChooserPaneSelectionChange();
    /** Notifies about global tool type change. */
    void sigToolTypeChangeGlobal();
    /** Notifies about machine tool type change. */
    void sigToolTypeChangeMachine();

public:

    /** Returns the singleton instance. */
    static UIMd3ManagerWindow *instance() { return s_pInstance; }
    /** Creates the singleton. */
    static void create();
    /** Destroys the singleton. */
    static void destroy();

    /** Returns the action-pool instance. */
    UIActionPool *actionPool() const { return m_pActionPool; }
    /** Returns the current chooser item. */
    UIVirtualMachineItem *currentItem() const;

    /** Opens the global tool @a enmType, creating its tab when needed. */
    void openGlobalTool(UIToolType enmType);
    /** Opens the machine tool @a enmType for the current selection. */
    void openMachineTool(UIToolType enmType);

public slots:

    /** Opens the command palette. */
    void sltOpenCommandPalette();
    /** Opens the tab manager. */
    void sltOpenTabManager();
    /** Opens the notification centre. */
    void sltOpenNotificationCentre();
    /** Opens the local history browser. */
    void sltOpenHistory();
    /** Exports the current view. */
    void sltExportView();

private slots:

    /** Handles rail selection. */
    void sltRailActivated(UIToolType enmType);
    /** Handles tab activation. */
    void sltTabActivated(const QString &strId);
    /** Handles theme changes, re-polishing the whole window. */
    void sltThemeChanged();
    /** Handles a machine-settings request from the details pane. */
    void sltOpenMachineSettings(const QString &strCategory, const QString &strControl, const QUuid &uId);

private:

    UIMd3ManagerWindow();
    virtual ~UIMd3ManagerWindow() RT_OVERRIDE;

    /** Prepares everything. */
    void prepare();
    /** Prepares the frameless title bar. */
    void prepareTitleBar();
    /** Prepares the workspace tab strip. */
    void prepareTabStrip();
    /** Prepares the navigation rail and its drawer fallback. */
    void prepareRail();
    /** Prepares the chooser and tool panes. */
    void preparePanes();
    /** Registers every command this window owns with the palette. */
    void registerCommands();
    /** Loads window geometry, tabs and tool selection from extradata. */
    void loadSettings();
    /** Saves window geometry, tabs and tool selection to extradata. */
    void saveSettings() const;

    /** Handles resize events, switching the rail to a drawer below 1000px. */
    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;
    /** Handles close events, saving state first. */
    virtual void closeEvent(QCloseEvent *pEvent) RT_OVERRIDE;

    static UIMd3ManagerWindow *s_pInstance;

    UIActionPool        *m_pActionPool;
    UIMd3TitleBar       *m_pTitleBar;
    UIMd3TabStrip       *m_pTabStrip;
    UIMd3NavigationRail *m_pRail;
    UIMd3SearchField    *m_pChooserSearch;
    UIChooser           *m_pChooser;
    UIToolPane          *m_pToolPane;
    QStackedWidget      *m_pContentStack;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3ManagerWindow_h */
