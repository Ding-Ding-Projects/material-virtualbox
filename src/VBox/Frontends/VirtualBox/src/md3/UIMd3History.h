/* $Id$ */
/** @file
 * VBox Qt GUI - append-only local Material 3 history service.
 */

/*
 * Copyright (C) 2026 Material Virtual Machine contributors.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3 of the
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3History_h
#define FEQT_INCLUDED_SRC_md3_UIMd3History_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QComboBox;
class QDateEdit;
class QDialog;
class QListWidget;
class QLabel;
class QPushButton;
class QWidget;
class UIMd3SearchField;

/** One bounded append-only local revision. */
struct UIMd3HistoryRevision
{
    /** Stable revision identifier. */
    QString strId;
    /** Action recorded by the owning surface. */
    QString strAction;
    /** Human-readable detail describing the affected data. */
    QString strDetail;
    /** UTC time at which the revision was recorded. */
    QDateTime when;
    /** Bounded serialized state associated with the revision. */
    QByteArray state;
};

/** Git-backed local history journal shared by Material 3 surfaces. */
class SHARED_LIBRARY_STUFF UIMd3History : public QObject
{
    Q_OBJECT;

signals:

    /** Announces an appended revision identifier. */
    void sigRevisionAppended(const QString &strId);
    /** Requests a surface-owned restore for a selected revision. */
    void sigRevisionRestoreRequested(const QString &strId);
    /** Announces whether the owning surface applied a requested restore. */
    void sigRevisionRestoreCompleted(const QString &strId, bool fRestored);

public:

    /** Returns the process-wide history journal. */
    static UIMd3History *instance();
    /** Creates the journal beneath the application-data directory. */
    static void create();
    /** Destroys the journal. */
    static void destroy();

    /** Appends a bounded revision and returns its identifier. */
    QString record(const QString &strAction,
                   const QString &strDetail,
                   const QByteArray &state = QByteArray());
    /** Returns the newest revision, or an empty revision when none exists. */
    UIMd3HistoryRevision latestRevision() const;
    /** Returns every valid revision, newest first. */
    QList<UIMd3HistoryRevision> revisions() const { return m_revisions; }
    /** Returns revisions matching bounded text, action, and inclusive dates. */
    QList<UIMd3HistoryRevision> filtered(const QString &strQuery,
                                        const QStringList &actions = QStringList(),
                                        const QDate &from = QDate(),
                                        const QDate &to = QDate()) const;
    /** Returns distinct actions currently present in the journal. */
    QStringList knownActions() const;
    /** Verifies every on-disk revision and reports a bounded human-readable error. */
    bool verifyIntegrity(QString &strError) const;
    /** Returns the state for @a strId, or an empty array when unknown. */
    QByteArray stateFor(const QString &strId) const;
    /** Returns whether the repository was initialized and Git commits work. */
    bool isGitBacked() const { return m_fGitBacked; }
    /** Opens or focuses the modeless local-history browser. */
    void showCentre(QWidget *pParent = 0);

private slots:

    /** Refreshes the filtered revision list. */
    void sltRefreshCentre();
    /** Verifies the journal and announces the result in the browser. */
    void sltVerifyIntegrity();
    /** Exports the current filtered revision list as bounded JSONL. */
    void sltExportCentre();
    /** Requests a restore through the owning surface adapter. */
    void sltRestoreCentre();
    /** Updates the browser after the owning surface reports restore status. */
    void sltRestoreResult(const QString &strId, bool fRestored);
    /** Updates Restore availability for the current row selection. */
    void sltUpdateRestoreState();

private:

    /** Constructs the journal. */
    UIMd3History();
    /** Destructs the journal. */
    virtual ~UIMd3History() RT_OVERRIDE RT_FINAL;

    /** Returns the isolated repository path. */
    static QString repositoryPath();
    /** Bounds an action/detail value before it reaches disk or Git. */
    static QString boundedString(const QString &strValue, int iMaximum);
    /** Loads valid revisions from the journal file. */
    void load();
    /** Atomically writes the bounded journal file. */
    bool save() const;
    /** Commits the journal file without inheriting user repository identity. */
    void commit(const UIMd3HistoryRevision &revision);
    /** Applies the current Material palette and language strings to the browser. */
    void retranslateCentre();
    /** Returns browser rows after its action/date and plain/regex filter state. */
    QList<UIMd3HistoryRevision> visibleCentreRows() const;

    static UIMd3History *s_pInstance;
    QList<UIMd3HistoryRevision> m_revisions;
    QString m_strRepositoryPath;
    bool m_fGitBacked;
    QPointer<QDialog> m_pHistoryDialog;
    UIMd3SearchField *m_pHistorySearch;
    QComboBox *m_pHistoryAction;
    QDateEdit *m_pHistoryFrom;
    QDateEdit *m_pHistoryTo;
    QLabel *m_pHistoryActionLabel;
    QLabel *m_pHistoryFromLabel;
    QLabel *m_pHistoryToLabel;
    QListWidget *m_pHistoryRows;
    QLabel *m_pHistoryStatus;
    QPushButton *m_pHistoryVerify;
    QPushButton *m_pHistoryExport;
    QPushButton *m_pHistoryRestore;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3History_h */
