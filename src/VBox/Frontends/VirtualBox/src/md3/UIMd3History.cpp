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

/* Qt includes: */
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSaveFile>
#include <QStandardPaths>
#include <QProcess>
#include <QStringList>
#include <QUuid>

/* GUI includes: */
#include "UIMd3History.h"

namespace
{
    const int g_iMaxRevisions = 1024;
    const int g_iMaxPayload = 1024 * 1024;
    const int g_iMaxActionLength = 128;
    const int g_iMaxDetailLength = 512;
    const int g_iMaxStateLength = 512 * 1024;

    bool runGit(const QString &strWorkingDirectory, const QStringList &arguments)
    {
        QProcess git;
        git.setWorkingDirectory(strWorkingDirectory);
        git.start(QStringLiteral("git"), arguments);
        if (!git.waitForStarted(1000) || !git.waitForFinished(3000))
            return false;
        return git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0;
    }
}

UIMd3History *UIMd3History::s_pInstance = 0;

UIMd3History *UIMd3History::instance()
{
    return s_pInstance;
}

void UIMd3History::create()
{
    if (!s_pInstance)
        s_pInstance = new UIMd3History;
}

void UIMd3History::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3History::UIMd3History()
    : QObject(0)
    , m_fGitBacked(false)
{
    m_strRepositoryPath = repositoryPath();
    if (m_strRepositoryPath.isEmpty() || !QDir().mkpath(m_strRepositoryPath))
        return;

    QDir repository(m_strRepositoryPath);
    if (!repository.exists(QStringLiteral(".git")))
        runGit(m_strRepositoryPath, QStringList() << QStringLiteral("init") << QStringLiteral("--quiet"));
    m_fGitBacked = QDir(m_strRepositoryPath).exists(QStringLiteral(".git"));
    load();
}

UIMd3History::~UIMd3History()
{
}

QString UIMd3History::repositoryPath()
{
    const QString strLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return strLocation.isEmpty() ? QString() : QDir(strLocation).filePath(QStringLiteral("history"));
}

QString UIMd3History::boundedString(const QString &strValue, int iMaximum)
{
    return strValue.trimmed().left(iMaximum);
}

void UIMd3History::load()
{
    m_revisions.clear();
    QFile file(QDir(m_strRepositoryPath).filePath(QStringLiteral("revisions.jsonl")));
    if (!file.exists() || file.size() <= 0 || file.size() > g_iMaxPayload
        || !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QSet<QString> seenIds;
    while (!file.atEnd() && m_revisions.size() < g_iMaxRevisions)
    {
        const QJsonDocument document = QJsonDocument::fromJson(file.readLine());
        if (!document.isObject())
            continue;
        const QJsonObject object = document.object();
        UIMd3HistoryRevision revision;
        revision.strId = boundedString(object.value(QStringLiteral("id")).toString(), 64);
        revision.strAction = boundedString(object.value(QStringLiteral("action")).toString(), g_iMaxActionLength);
        revision.strDetail = boundedString(object.value(QStringLiteral("detail")).toString(), g_iMaxDetailLength);
        revision.when = QDateTime::fromString(object.value(QStringLiteral("at")).toString(), Qt::ISODateWithMs).toUTC();
        revision.state = QByteArray::fromBase64(object.value(QStringLiteral("state")).toString().toLatin1());
        const QString strExpectedHash = object.value(QStringLiteral("hash")).toString();
        const QString strActualHash = QString::fromLatin1(QCryptographicHash::hash(revision.state,
                                                                                   QCryptographicHash::Sha256).toHex());
        if (revision.strId.isEmpty() || revision.strAction.isEmpty() || !revision.when.isValid()
            || revision.state.size() > g_iMaxStateLength || seenIds.contains(revision.strId)
            || strExpectedHash != strActualHash)
            continue;
        m_revisions << revision;
        seenIds.insert(revision.strId);
    }
}

bool UIMd3History::save() const
{
    if (m_strRepositoryPath.isEmpty())
        return false;

    QJsonArray lines;
    for (int i = 0; i < m_revisions.size() && i < g_iMaxRevisions; ++i)
    {
        const UIMd3HistoryRevision &revision = m_revisions.at(i);
        QJsonObject object;
        object.insert(QStringLiteral("id"), revision.strId);
        object.insert(QStringLiteral("action"), revision.strAction);
        object.insert(QStringLiteral("detail"), revision.strDetail);
        object.insert(QStringLiteral("at"), revision.when.toUTC().toString(Qt::ISODateWithMs));
        object.insert(QStringLiteral("state"), QString::fromLatin1(revision.state.toBase64()));
        object.insert(QStringLiteral("hash"), QString::fromLatin1(QCryptographicHash::hash(revision.state,
                                                                                             QCryptographicHash::Sha256).toHex()));
        lines.append(object);
    }
    const QByteArray data = QJsonDocument(lines).toJson(QJsonDocument::Compact);
    if (data.size() > g_iMaxPayload)
        return false;

    QSaveFile file(QDir(m_strRepositoryPath).filePath(QStringLiteral("revisions.jsonl")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    for (const QJsonValue &value : lines)
    {
        const QByteArray line = QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact) + '\n';
        if (file.write(line) != line.size())
            return false;
    }
    return file.commit();
}

QString UIMd3History::record(const QString &strAction,
                             const QString &strDetail,
                             const QByteArray &state)
{
    const QByteArray boundedState = state.left(g_iMaxStateLength);
    UIMd3HistoryRevision revision;
    revision.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    revision.strAction = boundedString(strAction, g_iMaxActionLength);
    revision.strDetail = boundedString(strDetail, g_iMaxDetailLength);
    revision.when = QDateTime::currentDateTimeUtc();
    revision.state = boundedState;
    if (revision.strAction.isEmpty())
        return QString();

    m_revisions.prepend(revision);
    while (m_revisions.size() > g_iMaxRevisions)
        m_revisions.removeLast();
    if (!save())
    {
        m_revisions.removeFirst();
        return QString();
    }
    commit(revision);
    emit sigRevisionAppended(revision.strId);
    return revision.strId;
}

void UIMd3History::commit(const UIMd3HistoryRevision &revision)
{
    if (!m_fGitBacked)
        return;
    Q_UNUSED(revision);

    /* Keep the UI path non-blocking.  The journal file is already atomic and
     * authoritative; Git is a best-effort local audit copy launched outside
     * the GUI event loop.  The command has no user-controlled shell text. */
    const QString strCommand = QStringLiteral(
        "git -c user.name=VirtualBox -c user.email=virtualbox@localhost "
        "add -- revisions.jsonl && git -c user.name=VirtualBox "
        "-c user.email=virtualbox@localhost commit --quiet -m "
        "\"Material 3 local history update\"");
#ifdef VBOX_WS_WIN
    const bool fStarted = QProcess::startDetached(QStringLiteral("cmd.exe"),
                                                   QStringList() << QStringLiteral("/d")
                                                                 << QStringLiteral("/c")
                                                                 << strCommand,
                                                   m_strRepositoryPath);
#else
    const bool fStarted = QProcess::startDetached(QStringLiteral("sh"),
                                                   QStringList() << QStringLiteral("-c")
                                                                 << strCommand,
                                                   m_strRepositoryPath);
#endif
    if (!fStarted)
        m_fGitBacked = false;
}

UIMd3HistoryRevision UIMd3History::latestRevision() const
{
    return m_revisions.isEmpty() ? UIMd3HistoryRevision() : m_revisions.first();
}

QByteArray UIMd3History::stateFor(const QString &strId) const
{
    for (const UIMd3HistoryRevision &revision : m_revisions)
        if (revision.strId == strId)
            return revision.state;
    return QByteArray();
}
