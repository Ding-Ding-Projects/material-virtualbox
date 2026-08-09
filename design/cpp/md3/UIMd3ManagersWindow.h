/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3ManagersWindow class declaration - the standalone managers and preferences shell.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3ManagersWindow_h
#define FEQT_INCLUDED_SRC_md3_UIMd3ManagersWindow_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QMainWindow>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class QStackedWidget;
class UIMd3SearchField;
class UIMd3TabStrip;

/** QMainWindow extension hosting every standalone manager in one Material 3 window.
  *
  * Virtual Media, Network, Extension Pack, Cloud Profile, Log Viewer, VM
  * Activity Overview and the eighteen preference pages share one window with
  * one tab strip, so an operator never ends up with six loose dialogs. Each
  * embedded manager keeps its existing model class; only its chrome is new. */
class UIMd3ManagersWindow : public QMainWindow
{
    Q_OBJECT;

public:

    /** Shows the window with @a enmTool current. */
    static void show(UIToolType enmTool, QWidget *pParent);

    /** Constructs the window. */
    UIMd3ManagersWindow(QWidget *pParent = 0);

    /** Makes @a enmTool current. */
    void setCurrentTool(UIToolType enmTool);
    /** Makes the preference page @a strPageId current. */
    void setCurrentPreferencePage(const QString &strPageId);

private slots:

    /** Filters the current manager's rows. */
    void sltFilter();
    /** Applies preference changes. */
    void sltApply();
    /** Resets the current manager's search and filters. */
    void sltReset();

private:

    void prepare();
    void preparePreferencePages();

    static UIMd3ManagersWindow *s_pInstance;

    UIMd3TabStrip    *m_pTabStrip;
    UIMd3SearchField *m_pSearchField;
    QStackedWidget   *m_pStack;
    QStackedWidget   *m_pPreferenceStack;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3ManagersWindow_h */
