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
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

/* GUI includes: */
#include "UILibraryDefs.h"

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
    /** Returns the state for @a strId, or an empty array when unknown. */
    QByteArray stateFor(const QString &strId) const;
    /** Returns whether the repository was initialized and Git commits work. */
    bool isGitBacked() const { return m_fGitBacked; }

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

    static UIMd3History *s_pInstance;
    QList<UIMd3HistoryRevision> m_revisions;
    QString m_strRepositoryPath;
    bool m_fGitBacked;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3History_h */
