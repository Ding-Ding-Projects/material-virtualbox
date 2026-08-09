/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3History class implementation.
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
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QUuid>

/* GUI includes: */
#include "UIMd3History.h"
#include "UIMd3Theme.h"

UIMd3History *UIMd3History::s_pInstance = 0;

UIMd3History *UIMd3History::instance() { return s_pInstance; }

void UIMd3History::create()
{
    if (!s_pInstance)
    {
        s_pInstance = new UIMd3History;
        s_pInstance->load();
    }
}

void UIMd3History::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3History::UIMd3History()
{
    const QString strRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_strRepositoryPath = QDir(strRoot).filePath("history");
    QDir().mkpath(m_strRepositoryPath);

    /* Initialise the isolated repository once. It lives beside the app data and is
     * never the user's own repository, so a failure here degrades to a plain
     * append-only file rather than touching anything the user owns. */
    if (!QDir(m_strRepositoryPath).exists(".git"))
    {
        QProcess git;
        git.setWorkingDirectory(m_strRepositoryPath);
        git.start("git", QStringList() << "init" << "--quiet");
        git.waitForFinished(5000);
    }
}

UIMd3History::~UIMd3History()
{
}

void UIMd3History::load()
{
    m_revisions.clear();
    QFile file(QDir(m_strRepositoryPath).filePath("revisions.jsonl"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    while (!file.atEnd())
    {
        const QJsonObject entry = QJsonDocument::fromJson(file.readLine()).object();
        if (entry.isEmpty())
            continue;
        UIMd3Revision revision;
        revision.strId     = entry.value("id").toString();
        revision.strAction = entry.value("action").toString();
        revision.strDetail = entry.value("detail").toString();
        revision.when      = QDateTime::fromString(entry.value("at").toString(), Qt::ISODate);
        revision.state     = entry.value("state").toString().toUtf8();
        m_revisions.prepend(revision);
    }
    file.close();
}

QString UIMd3History::record(const QString &strAction, const QString &strDetail)
{
    UIMd3Revision revision;
    revision.strId     = QUuid::createUuid().toString(QUuid::WithoutBraces);
    revision.strAction = strAction;
    revision.strDetail = strDetail;
    revision.when      = QDateTime::currentDateTime();

    QJsonObject state;
    state.insert("seed", md3Theme().seed().name());
    state.insert("scheme", (int)md3Theme().scheme());
    state.insert("compact", md3Theme().isCompact());
    revision.state = QJsonDocument(state).toJson(QJsonDocument::Compact);

    m_revisions.prepend(revision);
    commit(revision);
    emit sigRevisionAppended(revision.strId);
    return revision.strId;
}

void UIMd3History::commit(const UIMd3Revision &revision)
{
    QFile file(QDir(m_strRepositoryPath).filePath("revisions.jsonl"));
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;
    QJsonObject entry;
    entry.insert("id", revision.strId);
    entry.insert("action", revision.strAction);
    entry.insert("detail", revision.strDetail);
    entry.insert("at", revision.when.toString(Qt::ISODate));
    entry.insert("state", QString::fromUtf8(revision.state));
    entry.insert("hash", QString::fromLatin1(QCryptographicHash::hash(revision.state, QCryptographicHash::Sha256).toHex()));
    file.write(QJsonDocument(entry).toJson(QJsonDocument::Compact) + "\n");
    file.close();

    QProcess git;
    git.setWorkingDirectory(m_strRepositoryPath);
    git.start("git", QStringList() << "add" << "revisions.jsonl");
    git.waitForFinished(5000);
    git.start("git", QStringList() << "commit" << "--quiet" << "-m" << QString("%1: %2").arg(revision.strAction, revision.strDetail));
    git.waitForFinished(5000);
}

QList<UIMd3Revision> UIMd3History::filtered(const QString &strQuery, const QStringList &actions,
                                            const QDate &from, const QDate &to) const
{
    QList<UIMd3Revision> result;
    foreach (const UIMd3Revision &revision, m_revisions)
    {
        if (!strQuery.isEmpty()
            && !QString("%1 %2").arg(revision.strAction, revision.strDetail).contains(strQuery, Qt::CaseInsensitive))
            continue;
        if (!actions.isEmpty() && !actions.contains(revision.strAction))
            continue;
        if (from.isValid() && revision.when.date() < from)
            continue;
        if (to.isValid() && revision.when.date() > to)
            continue;
        result << revision;
    }
    return result;
}

bool UIMd3History::restore(const QString &strId)
{
    foreach (const UIMd3Revision &revision, m_revisions)
        if (revision.strId == strId)
        {
            const QJsonObject state = QJsonDocument::fromJson(revision.state).object();
            md3Theme().setSeed(QColor(state.value("seed").toString()));
            md3Theme().setScheme((UIMd3Scheme)state.value("scheme").toInt());
            md3Theme().setCompact(state.value("compact").toBool());
            /* The restore itself is a new revision. History is append-only. */
            record("revision restored", QString("%1 · %2").arg(revision.strAction, revision.strDetail));
            return true;
        }
    return false;
}

QStringList UIMd3History::knownActions() const
{
    QStringList result;
    foreach (const UIMd3Revision &revision, m_revisions)
        if (!result.contains(revision.strAction))
            result << revision.strAction;
    result.sort();
    return result;
}

bool UIMd3History::verifyIntegrity(QString &strError) const
{
    QFile file(QDir(m_strRepositoryPath).filePath("revisions.jsonl"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        strError = tr("The history repository could not be opened.");
        return false;
    }
    int iLine = 0;
    while (!file.atEnd())
    {
        ++iLine;
        const QJsonObject entry = QJsonDocument::fromJson(file.readLine()).object();
        const QByteArray state = entry.value("state").toString().toUtf8();
        const QString strExpected = entry.value("hash").toString();
        const QString strActual = QString::fromLatin1(QCryptographicHash::hash(state, QCryptographicHash::Sha256).toHex());
        if (strExpected != strActual)
        {
            strError = tr("Revision on line %1 failed its hash check.").arg(iLine);
            file.close();
            return false;
        }
    }
    file.close();
    return true;
}
