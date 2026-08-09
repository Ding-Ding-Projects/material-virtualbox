/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 four-scope tab manager.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, in version 3 of the License.
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
#include <QHash>
#include <QPointer>
#include <QStringList>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class UIMd3SearchField;
class UIMd3TabStrip;

/** Modeless tab manager with four independent discovery scopes and safe close previews. */
class SHARED_LIBRARY_STUFF UIMd3TabManager : public QDialog
{
    Q_OBJECT;

public:

    /** Opens or raises the manager associated with @a pStrip. */
    static void manage(UIMd3TabStrip *pStrip, QWidget *pParent);
    /** Registers one live strip for the cross-window master search. */
    static void registerStrip(UIMd3TabStrip *pStrip);
    /** Removes one strip from the cross-window master search. */
    static void unregisterStrip(UIMd3TabStrip *pStrip);

    /** Constructs the manager associated with @a pStrip. */
    UIMd3TabManager(UIMd3TabStrip *pStrip, QWidget *pParent = 0);

private slots:

    /** Refreshes all result models after tab state changes. */
    void sltRefresh();
    /** Builds a reviewable containing-query close preview. */
    void sltPreviewContaining();
    /** Builds a reviewable inverse-query close preview. */
    void sltPreviewNotContaining();
    /** Applies the still-current reviewed close set. */
    void sltApplyClose();
    /** Creates a named group from the guided group-name field. */
    void sltCreateGroup();

private:

    /** One preserved independent search state while group pages are rebuilt. */
    struct SearchState
    {
        QString strText;
        QString strPattern;
        QString strFlags;
        bool fRegex = false;
    };

    /** Creates the complete surface. */
    void prepare();
    /** Rebuilds the independently searchable page owned by every group. */
    void rebuildGroupPages();
    /** Updates the current-strip result list. */
    void refreshCurrentRows();
    /** Updates the group-name result list. */
    void refreshGroupNames();
    /** Updates one group's independently filtered tab rows. */
    void refreshGroupRows(const QString &strGroupId);
    /** Updates the all-window master result list. */
    void refreshMasterRows();
    /** Populates @a pList with an honest empty state when needed. */
    void addEmptyResult(QListWidget *pList, const QString &strText) const;
    /** Activates the exact tab represented by @a pItem. */
    void activateItem(QListWidgetItem *pItem);
    /** Opens the owning strip's complete tab-actions menu for @a pItem. */
    void showItemActions(QListWidget *pList, const QPoint &position);
    /** Builds one close preview from @a pSearch and @a fInverse. */
    void previewClose(UIMd3SearchField *pSearch, bool fInverse);
    /** Invalidates a preview after its model or options change. */
    void invalidateClosePreview(const QString &strReason = QString());

    QPointer<UIMd3TabStrip> m_pStrip;
    UIMd3SearchField *m_pCurrentSearch;
    QListWidget *m_pCurrentList;
    UIMd3SearchField *m_pGroupNameSearch;
    QListWidget *m_pGroupList;
    QStackedWidget *m_pGroupStack;
    QHash<QString, QPointer<UIMd3SearchField> > m_groupSearches;
    QHash<QString, QPointer<QListWidget> > m_groupLists;
    QHash<QString, SearchState> m_groupSearchStates;
    QString m_strGroupSignature;
    QLineEdit *m_pNewGroupName;
    QPushButton *m_pCreateGroup;
    UIMd3SearchField *m_pMasterSearch;
    QListWidget *m_pMasterList;
    UIMd3SearchField *m_pCloseContaining;
    UIMd3SearchField *m_pCloseNotContaining;
    QCheckBox *m_pIncludePinned;
    QLabel *m_pCloseStatus;
    QListWidget *m_pClosePreview;
    QPushButton *m_pApplyClose;
    QPointer<UIMd3SearchField> m_pPendingSearch;
    QStringList m_pendingCloseIds;
    QString m_strPendingQuery;
    QString m_strPendingFlags;
    bool m_fPendingInverse;
    bool m_fPendingRegex;
    bool m_fPendingIncludePinned;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3TabManager_h */
