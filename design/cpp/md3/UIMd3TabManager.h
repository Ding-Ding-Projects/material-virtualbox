/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3TabManager class declaration - the four-search tab manager with safe closing.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3TabManager_h
#define FEQT_INCLUDED_SRC_md3_UIMd3TabManager_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDialog>

/* Forward declarations: */
class QCheckBox;
class QVBoxLayout;
class UIMd3Button;
class UIMd3SearchField;
class UIMd3TabStrip;

/** QDialog extension implementing the tab manager.
  *
  * It hosts the four mandatory independent searches and both close tools. The
  * close tools never act directly: they resolve a preview set first, name every
  * matched tab, and only then unlock the authorising button. */
class UIMd3TabManager : public QDialog
{
    Q_OBJECT;

public:

    /** Runs the manager for @a pStrip. */
    static void manage(UIMd3TabStrip *pStrip, QWidget *pParent);

    /** Constructs the manager for @a pStrip. */
    UIMd3TabManager(UIMd3TabStrip *pStrip, QWidget *pParent = 0);

private slots:

    /** Refreshes the tab rows for the four active searches. */
    void sltRefresh();
    /** Resolves and shows the close preview set. */
    void sltPreviewClose();
    /** Applies the previewed close set. */
    void sltApplyClose();
    /** Creates a group from the new-group field. */
    void sltCreateGroup();

private:

    /** Prepares all contents. */
    void prepare();

    UIMd3TabStrip    *m_pStrip;
    UIMd3SearchField *m_pCurrentSearch;
    UIMd3SearchField *m_pGroupNameSearch;
    UIMd3SearchField *m_pMasterSearch;
    UIMd3SearchField *m_pSelectedGroupSearch;
    UIMd3SearchField *m_pCloseContaining;
    UIMd3SearchField *m_pCloseNotContaining;
    QCheckBox        *m_pIncludePinned;
    QVBoxLayout      *m_pRowLayout;
    UIMd3Button      *m_pApplyCloseButton;
    QStringList       m_pendingClose;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3TabManager_h */
