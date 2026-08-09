/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3History class declaration - append-only local revision history.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3History_h
#define FEQT_INCLUDED_SRC_md3_UIMd3History_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

/** One recorded revision. */
struct UIMd3Revision
{
    QString   strId;      /**< Stable revision identifier. */
    QString   strAction;  /**< What happened, in the product's own vocabulary. */
    QString   strDetail;  /**< The exact object the action touched. */
    QDateTime when;       /**< When it happened. */
    QByteArray state;     /**< The serialised GUI state, used by restore. */
};

/** QObject extension owning the append-only, Git-backed local history.
  *
  * Restoring never rewrites history: the restored state is appended as a new
  * revision, so the state that was current before the restore stays reachable. */
class UIMd3History : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners that a revision was appended. */
    void sigRevisionAppended(const QString &strId);

public:

    /** Returns the history singleton. */
    static UIMd3History *instance();
    /** Creates the singleton, opening or initialising the repository under the app data directory. */
    static void create();
    /** Destroys the singleton. */
    static void destroy();

    /** Appends a revision for @a strAction on @a strDetail. Returns the new revision id. */
    QString record(const QString &strAction, const QString &strDetail);
    /** Returns every revision, newest first. */
    QList<UIMd3Revision> revisions() const { return m_revisions; }
    /** Returns the revisions matching @a strQuery, optionally filtered by @a actions and a date range. */
    QList<UIMd3Revision> filtered(const QString &strQuery, const QStringList &actions,
                                  const QDate &from, const QDate &to) const;
    /** Restores @a strId and appends the restore itself as a new revision. Returns false when the id is unknown. */
    bool restore(const QString &strId);
    /** Returns every distinct recorded action, for the action filter. */
    QStringList knownActions() const;
    /** Verifies that every revision is readable and hashes match. */
    bool verifyIntegrity(QString &strError) const;

private:

    /** Constructs the history. */
    UIMd3History();
    /** Destructs the history. */
    virtual ~UIMd3History() RT_OVERRIDE;

    /** Loads the repository from disk. */
    void load();
    /** Commits @a revision to the repository. */
    void commit(const UIMd3Revision &revision);

    static UIMd3History  *s_pInstance;
    QString               m_strRepositoryPath;
    QList<UIMd3Revision>  m_revisions;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3History_h */
