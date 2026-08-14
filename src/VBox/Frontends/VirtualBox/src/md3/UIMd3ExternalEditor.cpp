/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3ExternalEditor class implementation - open VirtualBox's own files and folders in an external editor.
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

/* Qt includes: */
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

/* GUI includes: */
#include "UIMd3ExternalEditor.h"


/* static */
QString UIMd3ExternalEditor::firstExisting(const QStringList &paths)
{
    foreach (const QString &strPath, paths)
        if (!strPath.isEmpty() && QFileInfo::exists(strPath))
            return strPath;
    return QString();
}

/* static */
QList<UIMd3EditorInfo> UIMd3ExternalEditor::availableEditors()
{
    QList<UIMd3EditorInfo> editors;

#ifdef VBOX_WS_WIN
    const QString strLocalApps = qEnvironmentVariable("LOCALAPPDATA");
    const QString strProgramFiles = qEnvironmentVariable("ProgramFiles");
#endif

    /* Visual Studio Code (stable) -- the editor the house contract names explicitly.
     * Look on PATH first (covers the 'code' shim and portable builds on PATH),
     * then the usual per-user and machine install locations. */
    QStringList codeCandidates;
    codeCandidates << QStandardPaths::findExecutable("code");
#ifdef VBOX_WS_WIN
    if (!strLocalApps.isEmpty())
        codeCandidates << strLocalApps + "\\Programs\\Microsoft VS Code\\Code.exe";
    if (!strProgramFiles.isEmpty())
        codeCandidates << strProgramFiles + "\\Microsoft VS Code\\Code.exe";
#endif
    const QString strCode = firstExisting(codeCandidates);
    if (!strCode.isEmpty())
    {
        UIMd3EditorInfo info;
        info.strId = "vscode";
        info.strDisplayName = "Visual Studio Code";
        info.strExecutable = strCode;
        editors << info;
    }

    /* Visual Studio Code (Insiders). */
    QStringList insidersCandidates;
    insidersCandidates << QStandardPaths::findExecutable("code-insiders");
#ifdef VBOX_WS_WIN
    if (!strLocalApps.isEmpty())
        insidersCandidates << strLocalApps + "\\Programs\\Microsoft VS Code Insiders\\Code - Insiders.exe";
#endif
    const QString strInsiders = firstExisting(insidersCandidates);
    if (!strInsiders.isEmpty())
    {
        UIMd3EditorInfo info;
        info.strId = "vscode-insiders";
        info.strDisplayName = "Visual Studio Code - Insiders";
        info.strExecutable = strInsiders;
        editors << info;
    }

    /* The operating system's own default handler is always available as a fallback. */
    UIMd3EditorInfo systemDefault;
    systemDefault.strId = "system";
    systemDefault.strDisplayName = "System default application";
    systemDefault.strExecutable = QString();
    editors << systemDefault;

    return editors;
}

/* static */
QString UIMd3ExternalEditor::defaultConfigFolder()
{
    const QString strEnv = qEnvironmentVariable("VBOX_USER_HOME");
    if (!strEnv.isEmpty())
        return QDir::toNativeSeparators(strEnv);
    return QDir::toNativeSeparators(QDir::homePath() + "/.VirtualBox");
}

/* static */
bool UIMd3ExternalEditor::openPath(const QString &strPath, QWidget *pParent)
{
    if (strPath.isEmpty())
        return false;

    /* Prefer a real editor (Visual Studio Code) over the system default. A folder
     * handed to Code opens as a workspace root rather than a single file. */
    const QList<UIMd3EditorInfo> editors = availableEditors();
    foreach (const UIMd3EditorInfo &info, editors)
    {
        if (info.strExecutable.isEmpty())
            continue; /* The system default is tried below, after the real editors. */
        if (QProcess::startDetached(info.strExecutable, QStringList() << strPath))
            return true;
    }

    /* Fall back to the operating system's default handler for this path. */
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(strPath)))
        return true;

    if (pParent)
        QMessageBox::warning(pParent, "VirtualBox",
                             QString("Could not open \"%1\" in an external editor.\n\n"
                                     "Install Visual Studio Code, or make sure a default application "
                                     "is associated with this folder.").arg(strPath));
    return false;
}

/* static */
bool UIMd3ExternalEditor::openConfigFolder(QWidget *pParent)
{
    return openPath(defaultConfigFolder(), pParent);
}
