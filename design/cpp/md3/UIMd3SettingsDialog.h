/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3SettingsDialog class declaration - Material 3 settings shell.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3SettingsDialog_h
#define FEQT_INCLUDED_SRC_md3_UIMd3SettingsDialog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDialog>
#include <QMap>

/* GUI includes: */
#include "UISettingsDefs.h"

/* Forward declarations: */
class QStackedWidget;
class UIMd3Button;
class UIMd3SearchField;
class UISettingsPage;
class UISettingsSerializer;

/** QDialog extension replacing UIAdvancedSettingsDialog with the Material 3 shell.
  *
  * The page inventory, serializer and validators are unchanged: every existing
  * UISettingsPage subclass is hosted as-is. What the shell adds is a searchable
  * page list, a per-page editor search, live validity dots per page, and the
  * Basic/Expert switch surfaced as an M3 segmented control rather than a
  * hidden mode. */
class UIMd3SettingsDialog : public QDialog
{
    Q_OBJECT;

public:

    /** Constructs the dialog for the machine @a uMachineId. */
    UIMd3SettingsDialog(QWidget *pParent, const QUuid &uMachineId);
    /** Constructs the dialog for the global preferences. */
    UIMd3SettingsDialog(QWidget *pParent);

    /** Adds @a pPage under @a strTitle with @a strIcon. */
    void addPage(UISettingsPage *pPage, const QString &strTitle, const QString &strIcon);
    /** Selects the page named @a strTitle, used by the palette and by details-pane links. */
    void selectPage(const QString &strTitle);

private slots:

    /** Filters the page list against the page search. */
    void sltFilterPages();
    /** Filters the editors of the current page against the page-level search. */
    void sltFilterEditors();
    /** Revalidates every page and repaints the validity dots. */
    void sltRevalidate();
    /** Applies pending changes through the existing serializer. */
    void sltApply();
    /** Reverts pending changes. */
    void sltCancel();
    /** Resets the current page to its defaults. */
    void sltResetPage();
    /** Toggles Basic and Expert mode. */
    void sltToggleExpert();

private:

    void prepare();

    QUuid                          m_uMachineId;
    bool                           m_fGlobal;
    bool                           m_fExpert;
    UIMd3SearchField              *m_pPageSearch;
    UIMd3SearchField              *m_pEditorSearch;
    QStackedWidget                *m_pPageStack;
    QMap<QString, UISettingsPage*> m_pages;
    UISettingsSerializer          *m_pSerializer;
    UIMd3Button                   *m_pApplyButton;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3SettingsDialog_h */
