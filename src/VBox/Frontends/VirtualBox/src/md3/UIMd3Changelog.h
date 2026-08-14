/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 in-app changelog viewer.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Changelog_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Changelog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QDate>
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
class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QWidget;
class UIMd3SearchField;

/**
 * One real, previously published changelog entry.
 *
 * Every field here is transcribed verbatim from the repository's own
 * @c CHANGELOG.md (never invented), and every @a strCommitSha below was
 * independently verified against this checkout's Git history
 * (<tt>git cat-file -e</tt> / <tt>git log</tt>) before being compiled in --
 * see doc/md3/Changelog.md for the exact verification record.
 */
struct UIMd3ChangelogEntry
{
    UIMd3ChangelogEntry() : fOnDefaultBranch(true) {}

    /** Release version this entry is filed under ("Unreleased" until the
      * first tagged release ships -- see UIMd3Changelog::create()). */
    QString strVersion;
    /** Real calendar date the change was made, taken from the verified commit. */
    QDate date;
    /** Short categorized label ("Fixed", "Known issue", "Documentation", "Related work"). */
    QString strCategory;
    /** Optional section/context line shown under the category, such as the
      * subsystem a batch of fixes belongs to. */
    QString strSection;
    /** One-line human summary of the change. */
    QString strTitle;
    /** Full, unabridged explanation transcribed from CHANGELOG.md. */
    QString strDetail;
    /** Full 40-character commit hash that made the change. */
    QString strCommitSha;
    /** Resolvable URL for that exact commit on the project's forge. */
    QString strCommitUrl;
    /** Whether the cited commit is reachable from the shipped default branch. */
    bool fOnDefaultBranch;
};

/**
 * In-app changelog viewer: every entry CHANGELOG.md records, not only the
 * newest, with a plain-text/regex search over the changelog text, a real
 * calendar date-range filter that also accepts typed dates, a category
 * filter, per-entry clickable commit references, and filtered export/copy.
 *
 * The entry set is compiled in from CHANGELOG.md's own committed content
 * (see UIMd3Changelog::create()) rather than parsed from a file the
 * installed binary does not ship, so every entry, date, and commit link
 * shown here is real and was verified before being written -- this class
 * never fabricates a version, a date, or a change.
 */
class SHARED_LIBRARY_STUFF UIMd3Changelog : public QObject
{
    Q_OBJECT;

public:

    /** Returns the process-wide changelog singleton. */
    static UIMd3Changelog *instance();
    /** Creates the singleton and registers its localized copy. */
    static void create();
    /** Destroys the singleton and closes its modeless viewer. */
    static void destroy();

    /** Returns every known entry, in the order CHANGELOG.md records them. */
    QList<UIMd3ChangelogEntry> entries() const { return m_entries; }
    /** Returns entries matching the bounded @a strQuery/@a strCategory/@a strVersion and inclusive dates. */
    QList<UIMd3ChangelogEntry> filtered(const QString &strQuery,
                                        const QString &strCategory = QString(),
                                        const QString &strVersion = QString(),
                                        const QDate &from = QDate(),
                                        const QDate &to = QDate()) const;
    /** Returns distinct release versions the compiled-in entries belong to. */
    QStringList knownVersions() const;
    /** Returns distinct categories currently present. */
    QStringList knownCategories() const;
    /** Opens or focuses the modeless changelog browser. */
    void showCentre(QWidget *pParent = 0);

private slots:

    /** Rebuilds the visible rows from the current filter state. */
    void sltRefreshCentre();
    /** Exports the current filtered entries as a Markdown file. */
    void sltExportCentre();
    /** Copies the current filtered entries to the clipboard as Markdown. */
    void sltCopyCentre();

private:

    /** Constructs the viewer and its compiled-in entry set. */
    UIMd3Changelog();
    /** Destructs the viewer. */
    virtual ~UIMd3Changelog() RT_OVERRIDE RT_FINAL;

    /** Populates m_entries from CHANGELOG.md's real, verified content. */
    void prepareEntries();
    /** Applies the current Material palette and language strings to the browser. */
    void retranslateCentre();
    /** Returns entries after the browser's live category/version/date/search filter state. */
    QList<UIMd3ChangelogEntry> visibleCentreRows() const;
    /** Renders @a rows as bounded Markdown text honoring the requested @a fFullDetail. */
    static QString formatEntries(const QList<UIMd3ChangelogEntry> &rows, bool fFullDetail);
    /** Returns a short, stable clickable reference for a full commit hash. */
    static QString shortSha(const QString &strSha);

    static UIMd3Changelog *s_pInstance;
    QList<UIMd3ChangelogEntry> m_entries;

    QPointer<QDialog> m_pDialog;
    QLabel *m_pHeading;
    QLabel *m_pUnreleasedNotice;
    UIMd3SearchField *m_pSearch;
    QComboBox *m_pVersion;
    QComboBox *m_pCategory;
    QDateEdit *m_pFrom;
    QDateEdit *m_pTo;
    QLabel *m_pVersionLabel;
    QLabel *m_pCategoryLabel;
    QLabel *m_pFromLabel;
    QLabel *m_pToLabel;
    QScrollArea *m_pScrollArea;
    QVBoxLayout *m_pRowsLayout;
    QLabel *m_pStatus;
    QPushButton *m_pExport;
    QPushButton *m_pCopy;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Changelog_h */
