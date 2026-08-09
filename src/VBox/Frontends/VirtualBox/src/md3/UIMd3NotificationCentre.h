/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 persistent notification history.
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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3NotificationCentre_h
#define FEQT_INCLUDED_SRC_md3_UIMd3NotificationCentre_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QDialog;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QWidget;
class UIMd3SearchField;

/** One bounded, reviewable notification-history record. */
struct UIMd3Notice
{
    /** Stable record identifier. */
    QString strId;
    /** User-facing title. */
    QString strTitle;
    /** User-facing detail text. */
    QString strDetail;
    /** Short semantic category such as Information or Warning. */
    QString strCategory;
    /** UTC time at which the record was created. */
    QDateTime when;
    /** Whether the record represents an error or warning. */
    bool fError;
    /** Whether the user has not reviewed the record yet. */
    bool fUnread;
};

/** Persistent Material 3 notification history and review surface. */
class SHARED_LIBRARY_STUFF UIMd3NotificationCentre : public QObject
{
    Q_OBJECT;

signals:

    /** Announces a newly persisted record. */
    void sigNoticePosted(const QString &strId);
    /** Announces any history or read-state change. */
    void sigChanged();

public:

    /** Returns the process-wide notification history singleton. */
    static UIMd3NotificationCentre *instance();
    /** Creates the singleton after UICommon and the theme exist. */
    static void create();
    /** Destroys the singleton and closes its modeless review surface. */
    static void destroy();

    /** Constructs a persistent history service. */
    UIMd3NotificationCentre();
    /** Destructs the history service. */
    virtual ~UIMd3NotificationCentre() RT_OVERRIDE RT_FINAL;

    /**
     * Adds a bounded record and returns its stable identifier.
     *
     * Blocking questions and progress operations are intentionally not routed
     * through this API; the legacy notification center remains their owner.
     */
    QString post(QWidget *pParent,
                 const QString &strTitle,
                 const QString &strDetail,
                 const QString &strCategory = QString(),
                 bool fError = false);

    /** Returns newest-first records. */
    QList<UIMd3Notice> notices() const { return m_notices; }
    /** Marks every retained record as reviewed. */
    void markAllRead();
    /** Returns whether any retained record is unread. */
    bool hasUnread() const;
    /** Clears retained records; the destructive UI gate is a later lane. */
    void clear();
    /** Opens or focuses the modeless review surface. */
    void showCentre(QWidget *pParent = 0);

private slots:

    /** Rebuilds the visible rows from the current query and history. */
    void sltRefreshDialog();
    /** Refreshes the dialog's own language-mode strings. */
    void sltRetranslateUI();
    /** Handles the review action. */
    void sltMarkAllRead();
    /** Selects every row currently visible under the active filter. */
    void sltSelectAllVisible();
    /** Inverts selection for rows currently visible under the active filter. */
    void sltInvertVisibleSelection();
    /** Marks selected history records as read. */
    void sltMarkSelectedRead();
    /** Exports the selected records, or the visible filtered records. */
    void sltExportVisible();

private:

    /** Loads and validates the bounded on-disk schema. */
    void load();
    /** Atomically writes the bounded on-disk schema. */
    void save() const;
    /** Returns the application-data storage path. */
    static QString storagePath();
    /** Trims and bounds an untrusted persisted/user string. */
    static QString boundedString(const QString &strValue, int iMaximum);
    /** Returns IDs matching the active query, newest-first. */
    QStringList visibleNoticeIds() const;
    /** Refreshes selection summary and bulk-action enabled states. */
    void updateBulkActions();

    static UIMd3NotificationCentre *s_pInstance;
    QList<UIMd3Notice> m_notices;
    QDialog *m_pDialog;
    QLabel *m_pHeading;
    UIMd3SearchField *m_pSearchField;
    QVBoxLayout *m_pRowsLayout;
    QPushButton *m_pMarkAllReadButton;
    QPushButton *m_pSelectAllButton;
    QPushButton *m_pInvertSelectionButton;
    QPushButton *m_pMarkSelectedReadButton;
    QPushButton *m_pExportButton;
    QLabel *m_pSelectionSummary;
    QSet<QString> m_selectedIds;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3NotificationCentre_h */
