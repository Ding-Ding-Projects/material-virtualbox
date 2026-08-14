/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3ExternalEditor class declaration - open VirtualBox's own files and folders in an external editor.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3ExternalEditor_h
#define FEQT_INCLUDED_SRC_md3_UIMd3ExternalEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QList>
#include <QString>
#include <QStringList>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QWidget;

/** Describes one external editor detected on this host. */
struct UIMd3EditorInfo
{
    /** Stable identifier, e.g. "vscode". */
    QString strId;
    /** Human-readable name shown to the user, e.g. "Visual Studio Code". */
    QString strDisplayName;
    /** Absolute path to the launcher; empty for the operating-system default handler. */
    QString strExecutable;
};

/** Opens VirtualBox's own configuration files and folders in an external editor.
  *
  * This is the native, self-contained core of the "open in external editor"
  * capability every user-facing surface is expected to offer.  It detects the
  * editors actually installed on this host (Visual Studio Code and its Insiders
  * build explicitly, since the house contract names Code, plus the operating
  * system's own default handler as a guaranteed fallback) and opens a given
  * path in the best available one.
  *
  * A folder handed to Visual Studio Code opens as a workspace root, so the whole
  * VirtualBox configuration directory becomes navigable rather than a lone file
  * with no surrounding context.
  *
  * The class is deliberately static-only and free of COM and settings coupling
  * so that it compiles and behaves identically wherever it is used.  A
  * persisted, user-chosen preferred editor and an in-settings picker are the
  * documented follow-up (see doc/md3/ExternalEditor.md); today the best
  * detected editor is used automatically, which is a complete, working path. */
class SHARED_LIBRARY_STUFF UIMd3ExternalEditor
{
public:

    /** Returns every editor detected on this host, best first, always ending with the system default. */
    static QList<UIMd3EditorInfo> availableEditors();

    /** Returns VirtualBox's configuration folder (VBOX_USER_HOME when set, otherwise ~/.VirtualBox). */
    static QString defaultConfigFolder();

    /** Opens @a strPath in the best available editor.
      * @returns true on success; on failure shows a localized message on @a pParent and returns false. */
    static bool openPath(const QString &strPath, QWidget *pParent = 0);

    /** Convenience wrapper opening defaultConfigFolder() in the best available editor. */
    static bool openConfigFolder(QWidget *pParent = 0);

private:

    /** This class is static-only. */
    UIMd3ExternalEditor();

    /** Returns the first existing path among @a paths, or an empty string when none exist. */
    static QString firstExisting(const QStringList &paths);
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3ExternalEditor_h */
