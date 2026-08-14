/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3PersonalVocabulary class declaration - local, user-supplied text replacement.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3PersonalVocabulary_h
#define FEQT_INCLUDED_SRC_md3_UIMd3PersonalVocabulary_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QDialog;
class QLabel;
class QPushButton;
class QWidget;

/**
 * Local, user-supplied personal-vocabulary text replacement service.
 *
 * This class ships with no built-in mappings, samples, templates, guesses, or
 * default vocabulary of any kind, and it never performs a network request.
 * Until the user picks a valid local JSON file, replacement() always returns
 * its argument unchanged, so every surface renders its original shipped
 * wording.
 *
 * The only file this class ever reads is one the user actively picked
 * through the visible file-picker control in showCentre() (plus, on a later
 * launch, a bounded local cache of that same already-validated content -- see
 * cacheFilePath()). The expected file shape is exactly
 * <tt>{ "schemaVersion": 1, "entries": { "ordinary phrase": "replacement" } }</tt>.
 * validateFile() bounds and checks the *complete* payload -- file size,
 * schema version, JSON nesting depth, entry count, key/value length, and
 * value type -- before any of it is used, and never commits a partial
 * result: on any rejection the currently active vocabulary (if any) is left
 * exactly as it was, and the rejection reason is surfaced in the picker.
 *
 * Clearing (sltClear()) purges both the in-memory mapping and the on-disk
 * cache and restores original wording immediately.
 */
class SHARED_LIBRARY_STUFF UIMd3PersonalVocabulary : public QObject
{
    Q_OBJECT;

signals:

    /** Announces every successful load, every rejected pick, and every clear. */
    void sigStateChanged();

public:

    /** Honest, mutually exclusive states the visible control can report. */
    enum State
    {
        /** No file has ever been accepted; nothing is cached either. */
        State_NoFile,
        /** A valid file is currently active and supplying replacements. */
        State_Loaded,
        /** The only pick ever attempted (or the only one since a clear) was rejected. */
        State_Invalid,
    };

    /** Returns the process-wide personal-vocabulary service. */
    static UIMd3PersonalVocabulary *instance();
    /** Creates the singleton, registers its localized copy, and restores the local cache. */
    static void create();
    /** Destroys the singleton and closes its modeless picker. */
    static void destroy();

    /** Returns @a strOriginal unchanged unless a valid file currently defines a
      * replacement for it; with no valid file loaded this is always the
      * identity function, by construction (m_entries stays empty until a
      * validated file is applied). */
    QString replacement(const QString &strOriginal) const;

    /** Returns the current honest state of the control. */
    State state() const { return m_enmState; }
    /** Returns the number of entries currently active (0 unless State_Loaded). */
    int entryCount() const { return m_entries.size(); }
    /** Returns the absolute path most recently accepted, or empty (including
      * when the active vocabulary was restored from the local cache rather
      * than picked again this run). */
    QString sourcePath() const { return m_strSourcePath; }
    /** Returns the bounded, localized reason the last pick was rejected, or empty. */
    QString lastError() const { return m_strLastError; }

    /** Opens or focuses the modeless picker/status surface. */
    void showCentre(QWidget *pParent = 0);

private slots:

    /** Opens the local file picker and validates/applies (or rejects) the choice. */
    void sltChooseFile();
    /** Purges the in-memory mapping and the on-disk cache; restores original wording. */
    void sltClear();
    /** Applies the current Material palette and language strings to the picker. */
    void sltRetranslateUI();

private:

    UIMd3PersonalVocabulary();
    virtual ~UIMd3PersonalVocabulary() RT_OVERRIDE RT_FINAL;

    /** Returns the bounded local cache file path, or empty when unavailable. */
    static QString cacheFilePath();
    /** Returns whether @a data never nests object/array braces beyond the
      * declared schema's depth, scanned byte-by-byte (skipping quoted string
      * content) before any JSON parser walks the payload. */
    static bool payloadNestingWithinLimit(const QByteArray &data);
    /** Validates the *complete* payload at @a strPath against every declared
      * bound and fills @a outEntries only on full success; @a outEntries is
      * left untouched on any rejection, and @a strError names the bounded,
      * localized reason. */
    static bool validateFile(const QString &strPath, QHash<QString, QString> &outEntries, QString &strError);
    /** Commits a fully validated @a entries set as the active vocabulary. */
    void applyEntries(const QHash<QString, QString> &entries, const QString &strSourcePath);
    /** Restores a previously validated vocabulary from the local cache, if any. */
    void loadCache();
    /** Atomically rewrites the local cache from the currently active vocabulary. */
    void saveCache() const;
    /** Removes the local cache file, if any. */
    void clearCache() const;
    /** Refreshes the picker's visible status text and control states. */
    void updateCentre();

    static UIMd3PersonalVocabulary *s_pInstance;

    State m_enmState;
    QHash<QString, QString> m_entries;
    QString m_strSourcePath;
    QString m_strLastError;
    bool m_fRestoredFromCache;

    QPointer<QDialog> m_pDialog;
    QLabel *m_pHeading;
    QLabel *m_pExplanation;
    QLabel *m_pStatus;
    QPushButton *m_pChoose;
    QPushButton *m_pClear;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3PersonalVocabulary_h */
