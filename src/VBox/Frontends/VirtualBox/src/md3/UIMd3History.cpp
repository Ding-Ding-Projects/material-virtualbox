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
#include <QComboBox>
#include <QDateEdit>
#include <QAbstractItemView>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QPalette>
#include <QPushButton>
#include <QProcess>
#include <QSet>
#include <QSaveFile>
#include <QSize>
#include <QStandardPaths>
#include <QStringList>
#include <QUuid>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3History.h"
#include "UIMd3Language.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

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

    QString md3HistoryText(const char *pszKey, const QString &strFallback)
    {
        UIMd3Language *pLanguage = UIMd3Language::instance();
        return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
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
    {
        s_pInstance = new UIMd3History;
        if (UIMd3Language::instance())
        {
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.title"),
                                                     QStringLiteral("Local history"),
                                                     QStringLiteral("本地紀錄"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.search"),
                                                     QStringLiteral("Search revisions by action, detail or time"),
                                                     QStringLiteral("按動作、細節或時間搜尋紀錄"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.action"),
                                                     QStringLiteral("Action"),
                                                     QStringLiteral("動作"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.allActions"),
                                                     QStringLiteral("All actions"),
                                                     QStringLiteral("所有動作"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.from"),
                                                     QStringLiteral("From"),
                                                     QStringLiteral("由"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.to"),
                                                     QStringLiteral("To"),
                                                     QStringLiteral("到"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.verify"),
                                                     QStringLiteral("Verify integrity"),
                                                     QStringLiteral("驗證完整性"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.export"),
                                                     QStringLiteral("Export JSONL"),
                                                     QStringLiteral("匯出 JSONL"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.anyDate"),
                                                     QStringLiteral("Any date"),
                                                     QStringLiteral("任何日期"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.noMatch"),
                                                     QStringLiteral("No revisions match this filter"),
                                                     QStringLiteral("冇紀錄符合呢個篩選"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.integrityOk"),
                                                     QStringLiteral("Integrity verified: every revision hash matches."),
                                                     QStringLiteral("完整性驗證完成：每項紀錄雜湊都吻合。"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.integrityFailed"),
                                                     QStringLiteral("Integrity check failed: %1"),
                                                     QStringLiteral("完整性驗證失敗：%1"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.summary"),
                                                     QStringLiteral("%1 of %2 revisions"),
                                                     QStringLiteral("%1 / %2 項紀錄"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.history.exported"),
                                                     QStringLiteral("Exported %1 revisions."),
                                                     QStringLiteral("已匯出 %1 項紀錄。"));
        }
    }
}

void UIMd3History::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3History::UIMd3History()
    : QObject(0)
    , m_fGitBacked(false)
    , m_pHistorySearch(0)
    , m_pHistoryAction(0)
    , m_pHistoryFrom(0)
    , m_pHistoryTo(0)
    , m_pHistoryActionLabel(0)
    , m_pHistoryFromLabel(0)
    , m_pHistoryToLabel(0)
    , m_pHistoryRows(0)
    , m_pHistoryStatus(0)
    , m_pHistoryVerify(0)
    , m_pHistoryExport(0)
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
     * authoritative; Git is a best-effort local audit copy driven directly
     * through QProcess so no shell or terminal window is created. */
    QProcess *pStage = new QProcess(this);
    connect(pStage, &QProcess::errorOccurred, this, [this](QProcess::ProcessError)
    {
        m_fGitBacked = false;
    });
    connect(pStage, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, pStage](int iExitCode, QProcess::ExitStatus enmStatus)
    {
        pStage->deleteLater();
        if (!m_fGitBacked || enmStatus != QProcess::NormalExit || iExitCode != 0)
        {
            m_fGitBacked = false;
            return;
        }
        QProcess *pCommit = new QProcess(this);
        connect(pCommit, &QProcess::errorOccurred, this, [this](QProcess::ProcessError)
        {
            m_fGitBacked = false;
        });
        connect(pCommit, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, pCommit](int iCommitExitCode, QProcess::ExitStatus enmCommitStatus)
        {
            if (enmCommitStatus != QProcess::NormalExit || iCommitExitCode != 0)
                m_fGitBacked = false;
            pCommit->deleteLater();
        });
        pCommit->start(QStringLiteral("git"), QStringList()
                       << QStringLiteral("-c") << QStringLiteral("user.name=VirtualBox")
                       << QStringLiteral("-c") << QStringLiteral("user.email=virtualbox@localhost")
                       << QStringLiteral("-C") << m_strRepositoryPath
                       << QStringLiteral("commit") << QStringLiteral("--quiet")
                       << QStringLiteral("-m") << QStringLiteral("Material 3 local history update"));
    });
    pStage->start(QStringLiteral("git"), QStringList()
                  << QStringLiteral("-C") << m_strRepositoryPath
                  << QStringLiteral("add") << QStringLiteral("--")
                  << QStringLiteral("revisions.jsonl"));
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

QList<UIMd3HistoryRevision> UIMd3History::filtered(const QString &strQuery,
                                                   const QStringList &actions,
                                                   const QDate &from,
                                                   const QDate &to) const
{
    QList<UIMd3HistoryRevision> result;
    const QString strNeedle = strQuery.trimmed().left(512);
    for (const UIMd3HistoryRevision &revision : m_revisions)
    {
        if (!actions.isEmpty() && !actions.contains(revision.strAction))
            continue;
        if (from.isValid() && revision.when.date() < from)
            continue;
        if (to.isValid() && revision.when.date() > to)
            continue;
        const QString strHaystack = revision.strAction + QLatin1Char(' ')
                                   + revision.strDetail + QLatin1Char(' ')
                                   + revision.when.toString(Qt::ISODateWithMs);
        if (!strNeedle.isEmpty() && !strHaystack.contains(strNeedle, Qt::CaseInsensitive))
            continue;
        result << revision;
    }
    return result;
}

QList<UIMd3HistoryRevision> UIMd3History::visibleCentreRows() const
{
    if (!m_pHistorySearch || !m_pHistoryAction || !m_pHistoryFrom || !m_pHistoryTo)
        return QList<UIMd3HistoryRevision>();

    const QString strAction = m_pHistoryAction->currentData().toString();
    const QDate dateMinimum(1900, 1, 1);
    const QDate from = m_pHistoryFrom->date() == dateMinimum ? QDate() : m_pHistoryFrom->date();
    const QDate to = m_pHistoryTo->date() == dateMinimum ? QDate() : m_pHistoryTo->date();
    QList<UIMd3HistoryRevision> result;
    for (const UIMd3HistoryRevision &revision : m_revisions)
    {
        if (!strAction.isEmpty() && revision.strAction != strAction)
            continue;
        if (from.isValid() && revision.when.date() < from)
            continue;
        if (to.isValid() && revision.when.date() > to)
            continue;
        const QString strHaystack = revision.strAction + QLatin1Char(' ')
                                   + revision.strDetail + QLatin1Char(' ')
                                   + revision.when.toString(Qt::ISODateWithMs);
        if (!m_pHistorySearch->matches(strHaystack))
            continue;
        result << revision;
    }
    return result;
}

QStringList UIMd3History::knownActions() const
{
    QStringList result;
    for (const UIMd3HistoryRevision &revision : m_revisions)
        if (!result.contains(revision.strAction))
            result << revision.strAction;
    result.sort(Qt::CaseInsensitive);
    return result;
}

bool UIMd3History::verifyIntegrity(QString &strError) const
{
    strError.clear();
    if (m_strRepositoryPath.isEmpty())
    {
        strError = tr("The history repository path is unavailable.");
        return false;
    }

    QFile file(QDir(m_strRepositoryPath).filePath(QStringLiteral("revisions.jsonl")));
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        strError = tr("The history repository could not be opened.");
        return false;
    }
    if (file.size() > g_iMaxPayload)
    {
        strError = tr("The history repository exceeds its size limit.");
        return false;
    }

    QSet<QString> seenIds;
    int iLine = 0;
    while (!file.atEnd())
    {
        ++iLine;
        if (iLine > g_iMaxRevisions)
        {
            strError = tr("The history repository exceeds its revision limit.");
            return false;
        }
        const QByteArray line = file.readLine(g_iMaxPayload + 2);
        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject())
        {
            strError = tr("Revision line %1 is malformed.").arg(iLine);
            return false;
        }
        const QJsonObject object = document.object();
        const QString strId = boundedString(object.value(QStringLiteral("id")).toString(), 64);
        const QString strAction = boundedString(object.value(QStringLiteral("action")).toString(),
                                                g_iMaxActionLength);
        const QString strDetail = boundedString(object.value(QStringLiteral("detail")).toString(),
                                                g_iMaxDetailLength);
        const QDateTime when = QDateTime::fromString(object.value(QStringLiteral("at")).toString(),
                                                     Qt::ISODateWithMs);
        const QByteArray state = QByteArray::fromBase64(object.value(QStringLiteral("state"))
                                                         .toString().toLatin1());
        const QString strExpectedHash = object.value(QStringLiteral("hash")).toString();
        const QString strActualHash = QString::fromLatin1(QCryptographicHash::hash(
            state, QCryptographicHash::Sha256).toHex());
        if (strId.isEmpty() || strAction.isEmpty() || strDetail.isNull() || !when.isValid()
            || state.size() > g_iMaxStateLength || strExpectedHash != strActualHash
            || seenIds.contains(strId))
        {
            strError = tr("Revision on line %1 failed validation.").arg(iLine);
            return false;
        }
        seenIds.insert(strId);
    }
    return true;
}

void UIMd3History::showCentre(QWidget *pParent)
{
    if (m_pHistoryDialog)
    {
        m_pHistoryDialog->show();
        m_pHistoryDialog->raise();
        m_pHistoryDialog->activateWindow();
        sltRefreshCentre();
        return;
    }
    if (!UIMd3Theme::instance())
        return;

    m_pHistoryDialog = new QDialog(pParent, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    m_pHistoryDialog->setObjectName(QStringLiteral("md3LocalHistory"));
    m_pHistoryDialog->setMinimumSize(QSize(560, 440));
    QPalette palette = m_pHistoryDialog->palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Base, md3(UIMd3ColorRole_SurfaceContainer));
    m_pHistoryDialog->setAutoFillBackground(true);
    m_pHistoryDialog->setPalette(palette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(m_pHistoryDialog);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(md3Theme().gutter() / 2);

    QLabel *pHeading = new QLabel(m_pHistoryDialog);
    pHeading->setObjectName(QStringLiteral("historyHeading"));
    pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pRootLayout->addWidget(pHeading);

    m_pHistorySearch = new UIMd3SearchField(QStringLiteral("local-history"),
                                             md3HistoryText("md3.history.search",
                                                            tr("Search revisions by action, detail or time")),
                                             m_pHistoryDialog);
    m_pHistorySearch->setAccessibleName(md3HistoryText("md3.history.search",
                                                       tr("Search revisions by action, detail or time")));
    pRootLayout->addWidget(m_pHistorySearch);

    QHBoxLayout *pFilterLayout = new QHBoxLayout;
    pFilterLayout->setContentsMargins(0, 0, 0, 0);
    pFilterLayout->setSpacing(md3Theme().gutter() / 2);
    m_pHistoryActionLabel = new QLabel(m_pHistoryDialog);
    m_pHistoryAction = new QComboBox(m_pHistoryDialog);
    m_pHistoryAction->setMinimumHeight(md3Theme().controlHeight());
    m_pHistoryAction->addItem(QString(), QString());
    for (const QString &strAction : knownActions())
        m_pHistoryAction->addItem(strAction, strAction);
    pFilterLayout->addWidget(m_pHistoryActionLabel);
    pFilterLayout->addWidget(m_pHistoryAction, 1);

    const QDate dateMinimum(1900, 1, 1);
    m_pHistoryFromLabel = new QLabel(m_pHistoryDialog);
    m_pHistoryFrom = new QDateEdit(dateMinimum, m_pHistoryDialog);
    m_pHistoryFrom->setCalendarPopup(true);
    m_pHistoryFrom->setMinimumDate(dateMinimum);
    m_pHistoryToLabel = new QLabel(m_pHistoryDialog);
    m_pHistoryTo = new QDateEdit(dateMinimum, m_pHistoryDialog);
    m_pHistoryTo->setCalendarPopup(true);
    m_pHistoryTo->setMinimumDate(dateMinimum);
    pFilterLayout->addWidget(m_pHistoryFromLabel);
    pFilterLayout->addWidget(m_pHistoryFrom);
    pFilterLayout->addWidget(m_pHistoryToLabel);
    pFilterLayout->addWidget(m_pHistoryTo);
    pRootLayout->addLayout(pFilterLayout);

    m_pHistoryRows = new QListWidget(m_pHistoryDialog);
    m_pHistoryRows->setAccessibleName(md3HistoryText("md3.history.title", tr("Local history")));
    m_pHistoryRows->setSelectionMode(QAbstractItemView::NoSelection);
    pRootLayout->addWidget(m_pHistoryRows, 1);

    m_pHistoryStatus = new QLabel(m_pHistoryDialog);
    m_pHistoryStatus->setAccessibleName(tr("History status"));
    pRootLayout->addWidget(m_pHistoryStatus);

    QHBoxLayout *pButtonLayout = new QHBoxLayout;
    pButtonLayout->setContentsMargins(0, 0, 0, 0);
    m_pHistoryVerify = new QPushButton(m_pHistoryDialog);
    m_pHistoryVerify->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    m_pHistoryExport = new QPushButton(m_pHistoryDialog);
    m_pHistoryExport->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    pButtonLayout->addWidget(m_pHistoryVerify);
    pButtonLayout->addWidget(m_pHistoryExport);
    pButtonLayout->addStretch(1);
    pRootLayout->addLayout(pButtonLayout);

    connect(m_pHistorySearch, &UIMd3SearchField::sigFilterChanged,
            this, &UIMd3History::sltRefreshCentre);
    connect(m_pHistoryAction, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &UIMd3History::sltRefreshCentre);
    connect(m_pHistoryFrom, &QDateEdit::dateChanged,
            this, &UIMd3History::sltRefreshCentre);
    connect(m_pHistoryTo, &QDateEdit::dateChanged,
            this, &UIMd3History::sltRefreshCentre);
    connect(m_pHistoryVerify, &QPushButton::clicked,
            this, &UIMd3History::sltVerifyIntegrity);
    connect(m_pHistoryExport, &QPushButton::clicked,
            this, &UIMd3History::sltExportCentre);
    connect(this, &UIMd3History::sigRevisionAppended,
            this, &UIMd3History::sltRefreshCentre);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3History::retranslateCentre);
    connect(m_pHistoryDialog, &QObject::destroyed, this, [this]()
    {
        m_pHistoryDialog = 0;
        m_pHistorySearch = 0;
        m_pHistoryAction = 0;
        m_pHistoryFrom = 0;
        m_pHistoryTo = 0;
        m_pHistoryActionLabel = 0;
        m_pHistoryFromLabel = 0;
        m_pHistoryToLabel = 0;
        m_pHistoryRows = 0;
        m_pHistoryStatus = 0;
        m_pHistoryVerify = 0;
        m_pHistoryExport = 0;
    });

    retranslateCentre();
    sltRefreshCentre();
    m_pHistoryDialog->resize(760, 560);
    m_pHistoryDialog->show();
    m_pHistoryDialog->raise();
    m_pHistoryDialog->activateWindow();
    m_pHistorySearch->setFocus(Qt::ShortcutFocusReason);
}

void UIMd3History::sltRefreshCentre()
{
    if (!m_pHistoryRows || !m_pHistorySearch || !m_pHistoryAction
        || !m_pHistoryFrom || !m_pHistoryTo)
        return;

    const QList<UIMd3HistoryRevision> rows = visibleCentreRows();

    m_pHistoryRows->clear();
    for (const UIMd3HistoryRevision &revision : rows)
    {
        QListWidgetItem *pItem = new QListWidgetItem(
            QStringLiteral("%1\n%2\n%3")
                .arg(revision.strAction,
                     revision.strDetail,
                     QLocale().toString(revision.when.toLocalTime(), QLocale::ShortFormat)),
            m_pHistoryRows);
        pItem->setToolTip(revision.strId);
        pItem->setData(Qt::UserRole, revision.strId);
    }
    if (rows.isEmpty())
    {
        QListWidgetItem *pEmpty = new QListWidgetItem(
            md3HistoryText("md3.history.noMatch", tr("No revisions match this filter")),
            m_pHistoryRows);
        pEmpty->setFlags(Qt::NoItemFlags);
    }
    if (m_pHistoryStatus)
    {
        m_pHistoryStatus->setText(md3HistoryText("md3.history.summary",
                                                 tr("%1 of %2 revisions"))
                                  .arg(rows.size()).arg(m_revisions.size()));
        m_pHistoryStatus->setAccessibleName(m_pHistoryStatus->text());
    }
}

void UIMd3History::sltVerifyIntegrity()
{
    if (!m_pHistoryStatus)
        return;
    QString strError;
    const bool fValid = verifyIntegrity(strError);
    m_pHistoryStatus->setText(fValid
                              ? md3HistoryText("md3.history.integrityOk",
                                               tr("Integrity verified: every revision hash matches."))
                              : md3HistoryText("md3.history.integrityFailed",
                                               tr("Integrity check failed: %1")).arg(strError));
    m_pHistoryStatus->setAccessibleName(m_pHistoryStatus->text());
}

void UIMd3History::sltExportCentre()
{
    if (!m_pHistoryDialog || !m_pHistorySearch || !m_pHistoryAction
        || !m_pHistoryFrom || !m_pHistoryTo)
        return;

    const QList<UIMd3HistoryRevision> rows = visibleCentreRows();
    if (rows.isEmpty())
        return;

    const QString strDefaultPath = QDir(QStandardPaths::writableLocation(
        QStandardPaths::DocumentsLocation)).filePath(QStringLiteral("virtualbox-history.jsonl"));
    const QString strPath = QFileDialog::getSaveFileName(m_pHistoryDialog,
                                                          tr("Export local history"),
                                                          strDefaultPath,
                                                          tr("JSONL files (*.jsonl);;JSON files (*.json)"));
    if (strPath.isEmpty())
        return;

    QSaveFile file(strPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    int cbWritten = 0;
    for (const UIMd3HistoryRevision &revision : rows)
    {
        QJsonObject object;
        object.insert(QStringLiteral("id"), revision.strId);
        object.insert(QStringLiteral("action"), revision.strAction);
        object.insert(QStringLiteral("detail"), revision.strDetail);
        object.insert(QStringLiteral("at"), revision.when.toUTC().toString(Qt::ISODateWithMs));
        object.insert(QStringLiteral("state"), QString::fromLatin1(revision.state.toBase64()));
        object.insert(QStringLiteral("hash"), QString::fromLatin1(QCryptographicHash::hash(
            revision.state, QCryptographicHash::Sha256).toHex()));
        const QByteArray line = QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
        cbWritten += line.size();
        if (cbWritten > g_iMaxPayload || file.write(line) != line.size())
            return;
    }
    if (file.commit() && m_pHistoryStatus)
    {
        m_pHistoryStatus->setText(md3HistoryText("md3.history.exported",
                                                 tr("Exported %1 revisions.")).arg(rows.size()));
        m_pHistoryStatus->setAccessibleName(m_pHistoryStatus->text());
    }
}

void UIMd3History::retranslateCentre()
{
    if (!m_pHistoryDialog)
        return;
    const QString strTitle = md3HistoryText("md3.history.title", tr("Local history"));
    m_pHistoryDialog->setWindowTitle(strTitle);
    m_pHistoryDialog->setAccessibleName(strTitle);
    if (m_pHistorySearch)
    {
        const QString strSearch = md3HistoryText("md3.history.search",
                                                 tr("Search revisions by action, detail or time"));
        m_pHistorySearch->setPlaceholderText(strSearch);
        m_pHistorySearch->setAccessibleName(strSearch);
    }
    if (m_pHistoryActionLabel)
        m_pHistoryActionLabel->setText(md3HistoryText("md3.history.action", tr("Action")));
    if (m_pHistoryFromLabel)
        m_pHistoryFromLabel->setText(md3HistoryText("md3.history.from", tr("From")));
    if (m_pHistoryToLabel)
        m_pHistoryToLabel->setText(md3HistoryText("md3.history.to", tr("To")));
    if (m_pHistoryAction && m_pHistoryAction->count())
        m_pHistoryAction->setItemText(0, md3HistoryText("md3.history.allActions", tr("All actions")));
    if (m_pHistoryFrom)
        m_pHistoryFrom->setSpecialValueText(md3HistoryText("md3.history.anyDate", tr("Any date")));
    if (m_pHistoryTo)
        m_pHistoryTo->setSpecialValueText(md3HistoryText("md3.history.anyDate", tr("Any date")));
    if (m_pHistoryVerify)
        m_pHistoryVerify->setText(md3HistoryText("md3.history.verify", tr("Verify integrity")));
    if (m_pHistoryExport)
        m_pHistoryExport->setText(md3HistoryText("md3.history.export", tr("Export JSONL")));
    sltRefreshCentre();
}
