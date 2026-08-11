/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 search and appearance card for manager tools.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3ManagerToolSearch_h
#define FEQT_INCLUDED_SRC_md3_UIMd3ManagerToolSearch_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPersistentModelIndex>
#include <QPointer>

/* GUI includes: */
#include "UILibraryDefs.h"
#include "UIMd3Widget.h"

/* Forward declarations: */
class QAbstractItemModel;
class QAbstractItemView;
class QLabel;
class QPaintEvent;
class QResizeEvent;
class QTableView;
class QTimer;
class QToolButton;
class QTreeView;
class UIMd3SearchField;

/** Material search and appearance card which filters existing manager item views. */
class SHARED_LIBRARY_STUFF UIMd3ManagerToolSearch : public UIMd3Widget
{
public:

    /** Constructs a manager-tool search card. */
    explicit UIMd3ManagerToolSearch(QWidget *pParent = 0);
    /** Restores any filtered rows before destruction. */
    virtual ~UIMd3ManagerToolSearch() RT_OVERRIDE;

    /** Defines the existing @a pTarget pane and its stable @a strTargetKey. */
    void setTarget(QWidget *pTarget, const QString &strTargetKey,
                   const QString &strToolName, const QString &strPlaceholder);
    /** Clears the active target and restores its original row visibility. */
    void clearTarget();

protected:

    /** Paints the Material container surface. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    /** Hides secondary status copy when the card becomes narrow. */
    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;

private:

    /** Original row visibility for one existing item view. */
    struct ViewState
    {
        QPointer<QAbstractItemView> m_pView;
        QHash<QPersistentModelIndex, bool> m_rows;
    };

    /** Applies the active plain-text or regular-expression filter. */
    void applyFilter();
    /** Tracks live model changes without filtering on every individual update. */
    void bindModels();
    /** Disconnects all live model refresh connections. */
    void disconnectModels();
    /** Filters one tree row and its descendants, returning whether it matches. */
    bool filterTreeRow(QTreeView *pView, int iRow, const QModelIndex &parent,
                       int &cRows, int &cVisibleRows, bool &fTruncated);
    /** Filters all rows of one table view. */
    void filterTable(QTableView *pView, int &cRows, int &cVisibleRows,
                     bool &fTruncated);
    /** Returns searchable display and tooltip data for one model row. */
    QString rowText(const QAbstractItemModel *pModel, int iRow,
                    const QModelIndex &parent) const;
    /** Counts rows recursively up to the interactive filtering bound. */
    int countRows(const QAbstractItemModel *pModel, const QModelIndex &parent,
                  int cRemaining, bool &fTruncated) const;
    /** Returns or creates saved visibility state for @a pView. */
    ViewState &viewState(QAbstractItemView *pView);
    /** Returns the saved hidden state, capturing it on first use. */
    bool originalHidden(QTreeView *pView, int iRow, const QModelIndex &parent);
    /** Returns the saved hidden state, capturing it on first use. */
    bool originalHidden(QTableView *pView, int iRow);
    /** Restores every row hidden state captured for the current target. */
    void restoreRows();
    /** Updates visual and assistive result-status copy together. */
    void setStatusText(const QString &strText);
    /** Refreshes localized and accessible copy without resetting the filter. */
    void retranslateUi();
    /** Rebuilds the appearance icon for the active Material role and scale. */
    void updateAppearanceIcon();
    /** Refreshes compact status visibility. */
    void updateResponsiveState();

    UIMd3SearchField *m_pSearchField;
    QLabel *m_pStatusLabel;
    QToolButton *m_pAppearanceButton;
    QTimer *m_pRefreshTimer;
    QPointer<QWidget> m_pTarget;
    QString m_strTargetKey;
    QString m_strToolName;
    QString m_strPlaceholder;
    QList<ViewState> m_viewStates;
    QList<QMetaObject::Connection> m_modelConnections;
    bool m_fChangingTarget;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3ManagerToolSearch_h */
