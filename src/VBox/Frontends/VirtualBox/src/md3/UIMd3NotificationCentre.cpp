/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 persistent notification history.
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
#include <QDateTime>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QPointer>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSet>
#include <QSize>
#include <QSlider>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QUuid>
#include <QWidget>

/* GUI includes: */
#include "UIMd3NotificationCentre.h"
#include "UIMd3History.h"
#include "UIMd3Language.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

namespace
{
    const int g_iMaxNotices = 256;
    const int g_iMaxPayload = 512 * 1024;
    const int g_iMaxIdLength = 64;
    const int g_iMaxTitleLength = 240;
    const int g_iMaxDetailLength = 4096;
    const int g_iMaxCategoryLength = 96;
    const int g_iMaxJsonArrayEntries = 512;
    const int g_iRowSpacing = 8;
    const int g_iSchemaVersion = 1;

    QString md3NotificationText(const char *pszKey, const QString &strFallback)
    {
        UIMd3Language *pLanguage = UIMd3Language::instance();
        return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
    }
}

UIMd3NotificationCentre *UIMd3NotificationCentre::s_pInstance = 0;

UIMd3NotificationCentre *UIMd3NotificationCentre::instance()
{
    return s_pInstance;
}

void UIMd3NotificationCentre::create()
{
    if (!s_pInstance)
    {
        s_pInstance = new UIMd3NotificationCentre;
        if (UIMd3History::instance())
            connect(UIMd3History::instance(), &UIMd3History::sigRevisionRestoreRequested,
                    s_pInstance, &UIMd3NotificationCentre::sltRestoreHistoryRevision);
        if (UIMd3Language::instance())
        {
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.history"),
                                                     QStringLiteral("Notification history"),
                                                     QStringLiteral("通知紀錄"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.search"),
                                                     QStringLiteral("Search notifications"),
                                                     QStringLiteral("搜尋通知"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.markRead"),
                                                     QStringLiteral("Mark all as read"),
                                                     QStringLiteral("全部標記為已讀"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.selectAll"),
                                                     QStringLiteral("Select visible"),
                                                     QStringLiteral("揀晒目前顯示"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.invertSelection"),
                                                     QStringLiteral("Invert selection"),
                                                     QStringLiteral("反轉選取"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.markSelectedRead"),
                                                     QStringLiteral("Mark selected as read"),
                                                     QStringLiteral("將選取標記為已讀"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.export"),
                                                     QStringLiteral("Export view"),
                                                     QStringLiteral("匯出目前檢視"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.selectedSummary"),
                                                     QStringLiteral("%1 selected"),
                                                     QStringLiteral("已選取 %1 項"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clear"),
                                                     QStringLiteral("Clear history"),
                                                     QStringLiteral("清除紀錄"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearTitle"),
                                                     QStringLiteral("Clear notification history?"),
                                                     QStringLiteral("清除通知紀錄？"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearCount"),
                                                     QStringLiteral("This removes %1 retained notification records from md3-notifications.json."),
                                                     QStringLiteral("呢個動作會由 md3-notifications.json 清除 %1 項保留通知紀錄。"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearAckRecords"),
                                                     QStringLiteral("I understand these records will be removed."),
                                                     QStringLiteral("我明白呢啲紀錄會被移除。"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearAckUndo"),
                                                     QStringLiteral("I understand this dialog does not make the action reversible."),
                                                     QStringLiteral("我明白呢個對話框唔會令動作可以復原。"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearSlider"),
                                                     QStringLiteral("Slide fully to authorize"),
                                                     QStringLiteral("將滑桿推到底先可以授權"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearReady"),
                                                     QStringLiteral("Ready to clear"),
                                                     QStringLiteral("準備清除"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearEmergency"),
                                                     QStringLiteral("Emergency exit"),
                                                     QStringLiteral("緊急退出"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.clearAuthorize"),
                                                     QStringLiteral("Clear records"),
                                                     QStringLiteral("清除紀錄"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.undoClear"),
                                                     QStringLiteral("Undo last clear"),
                                                     QStringLiteral("復原上次清除"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.empty"),
                                                     QStringLiteral("No local notifications"),
                                                     QStringLiteral("暫時冇通知"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.noMatch"),
                                                     QStringLiteral("No notifications match this search"),
                                                     QStringLiteral("冇通知符合呢個搜尋"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.notification"),
                                                     QStringLiteral("Notification"),
                                                     QStringLiteral("通知"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.general"),
                                                     QStringLiteral("General"),
                                                     QStringLiteral("一般"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.information"),
                                                     QStringLiteral("Information"),
                                                     QStringLiteral("資訊"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.warning"),
                                                     QStringLiteral("Warning"),
                                                     QStringLiteral("警告"));
            UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.error"),
                                                     QStringLiteral("Error"),
                                                     QStringLiteral("錯誤"));
        }
    }
}

void UIMd3NotificationCentre::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3NotificationCentre::UIMd3NotificationCentre()
    : QObject(0)
    , m_pDialog(0)
    , m_pHeading(0)
    , m_pSearchField(0)
    , m_pRowsLayout(0)
    , m_pMarkAllReadButton(0)
    , m_pSelectAllButton(0)
    , m_pInvertSelectionButton(0)
    , m_pMarkSelectedReadButton(0)
    , m_pExportButton(0)
    , m_pClearButton(0)
    , m_pUndoClearButton(0)
    , m_pSelectionSummary(0)
    , m_fUndoAvailable(false)
{
    load();
    QList<UIMd3Notice> recovery;
    m_fUndoAvailable = loadRecordsAtPath(undoStoragePath(), recovery);
    if (UIMd3History::instance())
    {
        const UIMd3HistoryRevision latest = UIMd3History::instance()->latestRevision();
        if (latest.strAction == QStringLiteral("notification history cleared")
            && !latest.state.isEmpty())
        {
            QList<UIMd3Notice> historyRecovery;
            if (loadRecordsFromData(latest.state, historyRecovery))
            {
                m_strLastClearRevisionId = latest.strId;
                m_fUndoAvailable = true;
            }
        }
    }
}

UIMd3NotificationCentre::~UIMd3NotificationCentre()
{
    if (m_pDialog)
    {
        m_pDialog->close();
        delete m_pDialog;
        m_pDialog = 0;
        m_pHeading = 0;
    }
}

QString UIMd3NotificationCentre::boundedString(const QString &strValue, int iMaximum)
{
    return strValue.trimmed().left(iMaximum);
}

QString UIMd3NotificationCentre::storagePath()
{
    const QString strLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strLocation.isEmpty())
        return QString();
    return QDir(strLocation).filePath(QStringLiteral("md3-notifications.json"));
}

QString UIMd3NotificationCentre::undoStoragePath()
{
    const QString strLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strLocation.isEmpty())
        return QString();
    return QDir(strLocation).filePath(QStringLiteral("md3-notifications-undo.json"));
}

void UIMd3NotificationCentre::load()
{
    m_notices.clear();
    loadRecordsAtPath(storagePath(), m_notices);
}

void UIMd3NotificationCentre::save() const
{
    saveRecordsAtPath(storagePath(), m_notices);
}

bool UIMd3NotificationCentre::loadRecordsAtPath(const QString &strPath,
                                                QList<UIMd3Notice> &records)
{
    records.clear();
    if (strPath.isEmpty())
        return false;

    QFile file(strPath);
    if (!file.exists() || file.size() <= 0 || file.size() > g_iMaxPayload)
        return false;
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QByteArray data = file.read(g_iMaxPayload + 1);
    if (data.isEmpty() || data.size() > g_iMaxPayload)
        return false;

    return loadRecordsFromData(data, records);
}

bool UIMd3NotificationCentre::loadRecordsFromData(const QByteArray &data,
                                                  QList<UIMd3Notice> &records)
{
    records.clear();
    if (data.isEmpty() || data.size() > g_iMaxPayload)
        return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("version")).toInt(-1) != g_iSchemaVersion)
        return false;

    const QJsonArray recordsArray = root.value(QStringLiteral("notices")).toArray();
    if (recordsArray.isEmpty())
        return false;

    QSet<QString> seenIds;
    const int iCount = qMin(recordsArray.size(), g_iMaxJsonArrayEntries);
    for (int i = 0; i < iCount && records.size() < g_iMaxNotices; ++i)
    {
        const QJsonObject record = recordsArray.at(i).toObject();
        const QString strId = boundedString(record.value(QStringLiteral("id")).toString(), g_iMaxIdLength);
        const QString strTitle = boundedString(record.value(QStringLiteral("title")).toString(), g_iMaxTitleLength);
        const QString strDetail = boundedString(record.value(QStringLiteral("detail")).toString(), g_iMaxDetailLength);
        const QString strCategory = boundedString(record.value(QStringLiteral("category")).toString(), g_iMaxCategoryLength);
        const QDateTime when = QDateTime::fromString(record.value(QStringLiteral("when")).toString(), Qt::ISODateWithMs);
        if (strId.isEmpty() || strTitle.isEmpty() || !when.isValid() || seenIds.contains(strId))
            continue;

        UIMd3Notice notice;
        notice.strId = strId;
        notice.strTitle = strTitle;
        notice.strDetail = strDetail;
        notice.strCategory = strCategory.isEmpty() ? QStringLiteral("General") : strCategory;
        notice.when = when.toUTC();
        notice.fError = record.value(QStringLiteral("error")).toBool(false);
        notice.fUnread = record.value(QStringLiteral("unread")).toBool(true);
        records << notice;
        seenIds.insert(strId);
    }
    return !records.isEmpty();
}

bool UIMd3NotificationCentre::saveRecordsAtPath(const QString &strPath,
                                                 const QList<UIMd3Notice> &records)
{
    if (strPath.isEmpty())
        return false;

    const QFileInfo fileInfo(strPath);
    if (!QDir().mkpath(fileInfo.absolutePath()))
        return false;

    QJsonArray recordsArray;
    for (int i = 0; i < records.size() && i < g_iMaxNotices; ++i)
    {
        const UIMd3Notice &notice = records.at(i);
        QJsonObject record;
        record.insert(QStringLiteral("id"), notice.strId);
        record.insert(QStringLiteral("title"), notice.strTitle);
        record.insert(QStringLiteral("detail"), notice.strDetail);
        record.insert(QStringLiteral("category"), notice.strCategory);
        record.insert(QStringLiteral("when"), notice.when.toUTC().toString(Qt::ISODateWithMs));
        record.insert(QStringLiteral("error"), notice.fError);
        record.insert(QStringLiteral("unread"), notice.fUnread);
        recordsArray.append(record);
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), g_iSchemaVersion);
    root.insert(QStringLiteral("notices"), recordsArray);

    QSaveFile file(strPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (data.size() > g_iMaxPayload)
        return false;
    if (file.write(data) != data.size())
        return false;
    return file.commit();
}

QByteArray UIMd3NotificationCentre::currentState() const
{
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    const QByteArray state = file.read(g_iMaxPayload + 1);
    return state.size() <= g_iMaxPayload ? state : QByteArray();
}

void UIMd3NotificationCentre::saveUndoSnapshot()
{
    const bool fSnapshotSaved = saveRecordsAtPath(undoStoragePath(), m_notices);
    QString strRevisionId;
    if (fSnapshotSaved && UIMd3History::instance())
    {
        QFile file(undoStoragePath());
        if (file.open(QIODevice::ReadOnly))
        {
            const QByteArray state = file.read(g_iMaxPayload + 1);
            if (!state.isEmpty() && state.size() <= g_iMaxPayload)
                strRevisionId = UIMd3History::instance()->record(
                    QStringLiteral("notification history cleared"),
                    QStringLiteral("%1 notification records").arg(m_notices.size()),
                    state);
        }
    }
    m_strLastClearRevisionId = strRevisionId;
    m_fUndoAvailable = fSnapshotSaved || !strRevisionId.isEmpty();
}

bool UIMd3NotificationCentre::restoreUndoSnapshot()
{
    QList<UIMd3Notice> restored;
    if (!m_fUndoAvailable)
        return false;

    bool fRestored = false;
    if (UIMd3History::instance() && !m_strLastClearRevisionId.isEmpty())
    {
        const QByteArray state = UIMd3History::instance()->stateFor(m_strLastClearRevisionId);
        fRestored = loadRecordsFromData(state, restored);
    }
    if (!fRestored)
        fRestored = loadRecordsAtPath(undoStoragePath(), restored);
    if (!fRestored)
        return false;

    m_notices = restored;
    m_fUndoAvailable = false;
    const QString strRevisionId = m_strLastClearRevisionId;
    m_strLastClearRevisionId.clear();
    QFile::remove(undoStoragePath());
    save();
    if (UIMd3History::instance())
    {
        QFile file(storagePath());
        if (file.open(QIODevice::ReadOnly))
        {
            const QByteArray state = file.read(g_iMaxPayload + 1);
            if (!state.isEmpty() && state.size() <= g_iMaxPayload)
                UIMd3History::instance()->record(
                    QStringLiteral("notification history restored"),
                    QStringLiteral("%1 notification records; restored %2")
                        .arg(m_notices.size()).arg(strRevisionId),
                    state);
        }
    }
    emit sigChanged();
    return true;
}

QString UIMd3NotificationCentre::post(QWidget *pParent,
                                      const QString &strTitle,
                                      const QString &strDetail,
                                      const QString &strCategory,
                                      bool fError)
{
    Q_UNUSED(pParent);

    UIMd3Notice notice;
    notice.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    notice.strTitle = boundedString(strTitle, g_iMaxTitleLength);
    notice.strDetail = boundedString(strDetail, g_iMaxDetailLength);
    notice.strCategory = boundedString(strCategory, g_iMaxCategoryLength);
    if (notice.strTitle.isEmpty())
        notice.strTitle = md3NotificationText("md3.notifications.notification", tr("Notification"));
    if (notice.strCategory.isEmpty())
        notice.strCategory = md3NotificationText("md3.notifications.general", tr("General"));
    notice.when = QDateTime::currentDateTimeUtc();
    notice.fError = fError;
    notice.fUnread = true;

    if (m_fUndoAvailable)
    {
        QFile::remove(undoStoragePath());
        m_fUndoAvailable = false;
        m_strLastClearRevisionId.clear();
    }
    m_notices.prepend(notice);
    while (m_notices.size() > g_iMaxNotices)
        m_notices.removeLast();
    save();
    if (UIMd3History::instance())
        UIMd3History::instance()->record(
            QStringLiteral("notification history changed"),
            QStringLiteral("New notification %1").arg(notice.strId),
            currentState());
    emit sigNoticePosted(notice.strId);
    emit sigChanged();
    return notice.strId;
}

bool UIMd3NotificationCentre::hasUnread() const
{
    for (const UIMd3Notice &notice : m_notices)
        if (notice.fUnread)
            return true;
    return false;
}

void UIMd3NotificationCentre::markAllRead()
{
    bool fChanged = false;
    for (UIMd3Notice &notice : m_notices)
    {
        if (notice.fUnread)
        {
            notice.fUnread = false;
            fChanged = true;
        }
    }
    if (!fChanged)
        return;
    save();
    if (UIMd3History::instance())
        UIMd3History::instance()->record(
            QStringLiteral("notification history changed"),
            QStringLiteral("Marked all notification records as read"),
            currentState());
    emit sigChanged();
}

QStringList UIMd3NotificationCentre::visibleNoticeIds() const
{
    QStringList result;
    for (const UIMd3Notice &notice : m_notices)
    {
        const QString strSearchText = notice.strTitle + QLatin1Char('\n')
                                     + notice.strDetail + QLatin1Char('\n')
                                     + notice.strCategory;
        if (!m_pSearchField || m_pSearchField->matches(strSearchText))
            result << notice.strId;
    }
    return result;
}

void UIMd3NotificationCentre::updateBulkActions()
{
    const QStringList visibleIds = visibleNoticeIds();
    int cSelectedVisible = 0;
    for (const QString &strId : visibleIds)
        if (m_selectedIds.contains(strId))
            ++cSelectedVisible;

    if (m_pSelectionSummary)
    {
        const QString strSummary = md3NotificationText("md3.notifications.selectedSummary",
                                                        tr("%1 selected"));
        m_pSelectionSummary->setText(strSummary.arg(cSelectedVisible));
        m_pSelectionSummary->setAccessibleName(m_pSelectionSummary->text());
    }
    if (m_pSelectAllButton)
        m_pSelectAllButton->setEnabled(!visibleIds.isEmpty());
    if (m_pInvertSelectionButton)
        m_pInvertSelectionButton->setEnabled(!visibleIds.isEmpty());
    if (m_pMarkSelectedReadButton)
        m_pMarkSelectedReadButton->setEnabled(cSelectedVisible > 0);
    if (m_pExportButton)
        m_pExportButton->setEnabled(!visibleIds.isEmpty());
    if (m_pClearButton)
        m_pClearButton->setEnabled(!m_notices.isEmpty());
    if (m_pUndoClearButton)
        m_pUndoClearButton->setEnabled(m_fUndoAvailable);
}

void UIMd3NotificationCentre::sltSelectAllVisible()
{
    const QStringList visibleIds = visibleNoticeIds();
    for (const QString &strId : visibleIds)
        m_selectedIds.insert(strId);
    sltRefreshDialog();
}

void UIMd3NotificationCentre::sltInvertVisibleSelection()
{
    const QStringList visibleIds = visibleNoticeIds();
    for (const QString &strId : visibleIds)
    {
        if (m_selectedIds.contains(strId))
            m_selectedIds.remove(strId);
        else
            m_selectedIds.insert(strId);
    }
    sltRefreshDialog();
}

void UIMd3NotificationCentre::sltMarkSelectedRead()
{
    bool fChanged = false;
    for (UIMd3Notice &notice : m_notices)
    {
        if (m_selectedIds.contains(notice.strId) && notice.fUnread)
        {
            notice.fUnread = false;
            fChanged = true;
        }
    }
    if (!fChanged)
        return;
    save();
    if (UIMd3History::instance())
        UIMd3History::instance()->record(
            QStringLiteral("notification history changed"),
            QStringLiteral("Marked selected notification records as read"),
            currentState());
    emit sigChanged();
}

void UIMd3NotificationCentre::sltExportVisible()
{
    if (!m_pDialog)
        return;

    const QStringList visibleIds = visibleNoticeIds();
    if (visibleIds.isEmpty())
        return;

    QStringList exportIds;
    for (const QString &strId : visibleIds)
        if (m_selectedIds.contains(strId))
            exportIds << strId;
    if (exportIds.isEmpty())
        exportIds = visibleIds;

    const QString strDefaultPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                 .filePath(QStringLiteral("virtualbox-notification-history.json"));
    const QString strPath = QFileDialog::getSaveFileName(m_pDialog,
                                                          tr("Export notification history"),
                                                          strDefaultPath,
                                                          tr("JSON files (*.json)"));
    if (strPath.isEmpty())
        return;

    QJsonArray records;
    for (const UIMd3Notice &notice : m_notices)
    {
        if (!exportIds.contains(notice.strId))
            continue;
        QJsonObject record;
        record.insert(QStringLiteral("id"), notice.strId);
        record.insert(QStringLiteral("title"), notice.strTitle);
        record.insert(QStringLiteral("detail"), notice.strDetail);
        record.insert(QStringLiteral("category"), notice.strCategory);
        record.insert(QStringLiteral("when"), notice.when.toUTC().toString(Qt::ISODateWithMs));
        record.insert(QStringLiteral("error"), notice.fError);
        record.insert(QStringLiteral("unread"), notice.fUnread);
        records.append(record);
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), g_iSchemaVersion);
    root.insert(QStringLiteral("notices"), records);
    QSaveFile file(strPath);
    if (!file.open(QIODevice::WriteOnly))
        return;
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (data.size() > g_iMaxPayload || file.write(data) != data.size())
        return;
    file.commit();
}

void UIMd3NotificationCentre::sltRequestClear()
{
    if (!m_pDialog || m_notices.isEmpty())
        return;

    QDialog dialog(m_pDialog, Qt::Dialog | Qt::WindowTitleHint);
    dialog.setObjectName(QStringLiteral("md3NotificationClearConfirmation"));
    dialog.setModal(true);
    dialog.setWindowModality(Qt::WindowModal);
    const QString strTitle = md3NotificationText("md3.notifications.clearTitle",
                                                  tr("Clear notification history?"));
    dialog.setWindowTitle(strTitle);
    dialog.setAccessibleName(strTitle);
    dialog.setMinimumSize(QSize(460, 360));

    QPalette palette = dialog.palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Base, md3(UIMd3ColorRole_SurfaceContainer));
    dialog.setAutoFillBackground(true);
    dialog.setPalette(palette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(&dialog);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(g_iRowSpacing);

    QLabel *pHeading = new QLabel(strTitle, &dialog);
    pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pHeading->setAccessibleName(strTitle);
    pRootLayout->addWidget(pHeading);

    QLabel *pCount = new QLabel(md3NotificationText("md3.notifications.clearCount",
                                                     tr("This removes %1 retained notification records from md3-notifications.json."))
                                .arg(m_notices.size()),
                                &dialog);
    pCount->setWordWrap(true);
    pCount->setAccessibleName(pCount->text());
    pRootLayout->addWidget(pCount);

    QCheckBox *pAcknowledgeRecords = new QCheckBox(
        md3NotificationText("md3.notifications.clearAckRecords",
                            tr("I understand these records will be removed.")),
        &dialog);
    pAcknowledgeRecords->setObjectName(QStringLiteral("md3ClearAcknowledgeRecords"));
    pAcknowledgeRecords->setAccessibleName(pAcknowledgeRecords->text());
    pAcknowledgeRecords->setAccessibleDescription(
        tr("First acknowledgement required before the authorization slider is enabled"));
    pRootLayout->addWidget(pAcknowledgeRecords);

    QCheckBox *pAcknowledgeUndo = new QCheckBox(
        md3NotificationText("md3.notifications.clearAckUndo",
                            tr("I understand this dialog does not make the action reversible.")),
        &dialog);
    pAcknowledgeUndo->setObjectName(QStringLiteral("md3ClearAcknowledgeUndo"));
    pAcknowledgeUndo->setAccessibleName(pAcknowledgeUndo->text());
    pAcknowledgeUndo->setAccessibleDescription(
        tr("Second acknowledgement required before the authorization slider is enabled"));
    pRootLayout->addWidget(pAcknowledgeUndo);

    QLabel *pSliderHeading = new QLabel(
        md3NotificationText("md3.notifications.clearSlider",
                            tr("Slide fully to authorize")),
        &dialog);
    pSliderHeading->setAccessibleName(pSliderHeading->text());
    pRootLayout->addWidget(pSliderHeading);

    QSlider *pSlider = new QSlider(Qt::Horizontal, &dialog);
    pSlider->setObjectName(QStringLiteral("md3ClearAuthorizationSlider"));
    pSlider->setRange(0, 100);
    pSlider->setValue(0);
    pSlider->setSingleStep(10);
    pSlider->setPageStep(25);
    pSlider->setTickInterval(25);
    pSlider->setTickPosition(QSlider::TicksBelow);
    pSlider->setEnabled(false);
    pSlider->setAccessibleName(pSliderHeading->text());
    pSlider->setAccessibleDescription(
        tr("The clear action remains unavailable until the slider reaches 100 percent"));
    pRootLayout->addWidget(pSlider);

    QProgressBar *pProgress = new QProgressBar(&dialog);
    pProgress->setObjectName(QStringLiteral("md3ClearAuthorizationProgress"));
    pProgress->setRange(0, 100);
    pProgress->setValue(0);
    pProgress->setTextVisible(true);
    pProgress->setFormat(QStringLiteral("%p%"));
    pProgress->setAccessibleName(tr("Clear authorization progress"));
    pProgress->setAccessibleDescription(
        tr("Animated progress follows the authorization slider and completes at 100 percent"));
    pRootLayout->addWidget(pProgress);

    QLabel *pStatus = new QLabel(tr("Both acknowledgements are required."), &dialog);
    pStatus->setWordWrap(true);
    pStatus->setAccessibleName(pStatus->text());
    pRootLayout->addWidget(pStatus);
    QGraphicsOpacityEffect *pCompletionEffect = new QGraphicsOpacityEffect(pStatus);
    pCompletionEffect->setOpacity(1.0);
    pStatus->setGraphicsEffect(pCompletionEffect);

    QDialogButtonBox *pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal,
                                                       &dialog);
    QPushButton *pAuthorize = pButtons->button(QDialogButtonBox::Ok);
    QPushButton *pEmergencyExit = pButtons->button(QDialogButtonBox::Cancel);
    pAuthorize->setObjectName(QStringLiteral("md3ClearAuthorizeButton"));
    pAuthorize->setText(md3NotificationText("md3.notifications.clearAuthorize",
                                            tr("Clear records")));
    pAuthorize->setAccessibleName(pAuthorize->text());
    pAuthorize->setEnabled(false);
    pEmergencyExit->setObjectName(QStringLiteral("md3ClearEmergencyExitButton"));
    pEmergencyExit->setText(md3NotificationText("md3.notifications.clearEmergency",
                                                 tr("Emergency exit")));
    pEmergencyExit->setAccessibleName(pEmergencyExit->text());
    pEmergencyExit->setToolTip(pEmergencyExit->text());
    pRootLayout->addWidget(pButtons);

    QPointer<QPropertyAnimation> pProgressAnimation;
    QPointer<QPropertyAnimation> pCompletionAnimation;
    const auto animateProgress = [&pProgressAnimation, pProgress](int iValue)
    {
        if (pProgressAnimation)
            pProgressAnimation->stop();
        pProgressAnimation = new QPropertyAnimation(pProgress, "value", pProgress);
        pProgressAnimation->setDuration(140);
        pProgressAnimation->setEasingCurve(QEasingCurve::OutCubic);
        pProgressAnimation->setStartValue(pProgress->value());
        pProgressAnimation->setEndValue(iValue);
        pProgressAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    };

    const auto animateCompletion = [&pCompletionAnimation, pCompletionEffect]()
    {
        if (pCompletionAnimation)
            pCompletionAnimation->stop();
        pCompletionAnimation = new QPropertyAnimation(pCompletionEffect, "opacity", pCompletionEffect);
        pCompletionAnimation->setDuration(220);
        pCompletionAnimation->setEasingCurve(QEasingCurve::OutCubic);
        pCompletionAnimation->setStartValue(0.35);
        pCompletionAnimation->setEndValue(1.0);
        pCompletionAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    };

    const auto updateAuthorizationState = [&]()
    {
        const bool fAcknowledged = pAcknowledgeRecords->isChecked()
                                 && pAcknowledgeUndo->isChecked();
        pSlider->setEnabled(fAcknowledged);
        if (!fAcknowledged)
        {
            pSlider->setValue(0);
            animateProgress(0);
            pProgress->setFormat(QStringLiteral("%p%"));
            pStatus->setText(tr("Both acknowledgements are required."));
        }
        pAuthorize->setEnabled(fAcknowledged && pSlider->value() == 100);
    };

    connect(pAcknowledgeRecords, &QCheckBox::toggled, &dialog,
            [&updateAuthorizationState](bool) { updateAuthorizationState(); });
    connect(pAcknowledgeUndo, &QCheckBox::toggled, &dialog,
            [&updateAuthorizationState](bool) { updateAuthorizationState(); });
    connect(pSlider, &QSlider::valueChanged, &dialog,
            [&, pProgress, pStatus, pAuthorize](int iValue)
    {
        animateProgress(iValue);
        if (iValue == 100)
        {
            const QString strReady = md3NotificationText("md3.notifications.clearReady",
                                                         tr("Ready to clear"));
            pProgress->setFormat(strReady);
            pStatus->setText(strReady);
            animateCompletion();
        }
        else
        {
            pProgress->setFormat(QStringLiteral("%p%"));
            pStatus->setText(tr("Keep sliding until the authorization reaches 100 percent."));
        }
        pStatus->setAccessibleName(pStatus->text());
        pAuthorize->setEnabled(pAcknowledgeRecords->isChecked()
                               && pAcknowledgeUndo->isChecked()
                               && iValue == 100);
    });
    connect(pButtons, &QDialogButtonBox::accepted, &dialog,
            [this, &dialog, pAcknowledgeRecords, pAcknowledgeUndo, pSlider,
             pProgress, pStatus, pAuthorize]()
    {
        if (!pAcknowledgeRecords->isChecked() || !pAcknowledgeUndo->isChecked()
            || pSlider->value() != 100)
            return;
        pAuthorize->setEnabled(false);
        pProgress->setValue(100);
        pProgress->setFormat(md3NotificationText("md3.notifications.clearReady",
                                                 tr("Ready to clear")));
        pStatus->setText(md3NotificationText("md3.notifications.clearReady",
                                             tr("Ready to clear")));
        pStatus->setAccessibleName(pStatus->text());
        saveUndoSnapshot();
        clear();
        dialog.accept();
    });
    connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.resize(560, 420);
    dialog.exec();
    if (m_pClearButton)
        m_pClearButton->setFocus(Qt::OtherFocusReason);
}

void UIMd3NotificationCentre::sltUndoClear()
{
    if (!restoreUndoSnapshot())
        return;
    if (m_pUndoClearButton)
        m_pUndoClearButton->setFocus(Qt::OtherFocusReason);
}

void UIMd3NotificationCentre::sltRestoreHistoryRevision(const QString &strRevisionId)
{
    bool fRestored = false;
    bool fApplicable = false;
    UIMd3History *pHistory = UIMd3History::instance();
    if (pHistory)
    {
        const QList<UIMd3HistoryRevision> revisions = pHistory->revisions();
        for (const UIMd3HistoryRevision &revision : revisions)
            if (revision.strId == strRevisionId
                && (revision.strAction == QStringLiteral("notification history cleared")
                    || revision.strAction == QStringLiteral("notification history changed")
                    || revision.strAction == QStringLiteral("notification history restored")))
            {
                fApplicable = true;
                QList<UIMd3Notice> restored;
                if (loadRecordsFromData(revision.state, restored)
                    && saveRecordsAtPath(storagePath(), restored))
                {
                    m_notices = restored;
                    m_selectedIds.clear();
                    m_fUndoAvailable = false;
                    m_strLastClearRevisionId.clear();
                    QFile::remove(undoStoragePath());
                    fRestored = true;
                    QFile file(storagePath());
                    if (file.open(QIODevice::ReadOnly))
                    {
                        const QByteArray state = file.read(g_iMaxPayload + 1);
                        if (!state.isEmpty() && state.size() <= g_iMaxPayload)
                            pHistory->record(
                                QStringLiteral("notification history restored"),
                                QStringLiteral("%1 notification records; restored %2")
                                    .arg(m_notices.size()).arg(strRevisionId),
                                state);
                    }
                    emit sigChanged();
                }
                break;
            }
    }
    if (pHistory && fApplicable)
        emit pHistory->sigRevisionRestoreCompleted(strRevisionId, fRestored);
}

void UIMd3NotificationCentre::clear()
{
    if (m_notices.isEmpty())
        return;
    m_notices.clear();
    save();
    emit sigChanged();
}

void UIMd3NotificationCentre::showCentre(QWidget *pParent)
{
    if (m_pDialog)
    {
        m_pDialog->show();
        m_pDialog->raise();
        m_pDialog->activateWindow();
        return;
    }

    /* UIMd3SearchField reads the process-wide theme during construction. */
    if (!UIMd3Theme::instance())
        return;

    m_pDialog = new QDialog(pParent, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    m_pDialog->setObjectName(QStringLiteral("md3NotificationCentre"));
    m_pDialog->setAccessibleName(md3NotificationText("md3.notifications.history", tr("Notification history")));
    m_pDialog->setWindowTitle(md3NotificationText("md3.notifications.history", tr("Notification history")));
    m_pDialog->setMinimumSize(QSize(440, 420));

    QPalette palette = m_pDialog->palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Base, md3(UIMd3ColorRole_SurfaceContainer));
    m_pDialog->setAutoFillBackground(true);
    m_pDialog->setPalette(palette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(m_pDialog);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(g_iRowSpacing);

    m_pHeading = new QLabel(m_pDialog);
    m_pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pRootLayout->addWidget(m_pHeading);

    QHBoxLayout *pActionLayout = new QHBoxLayout;
    pActionLayout->setContentsMargins(0, 0, 0, 0);
    m_pSearchField = new UIMd3SearchField(QStringLiteral("notifications-history"),
                                           md3NotificationText("md3.notifications.search", tr("Search notifications")),
                                           m_pDialog);
    pActionLayout->addWidget(m_pSearchField, 1);
    m_pMarkAllReadButton = new QPushButton(m_pDialog);
    m_pMarkAllReadButton->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    pActionLayout->addWidget(m_pMarkAllReadButton);
    pRootLayout->addLayout(pActionLayout);

    QHBoxLayout *pBulkLayout = new QHBoxLayout;
    pBulkLayout->setContentsMargins(0, 0, 0, 0);
    pBulkLayout->setSpacing(g_iRowSpacing);
    m_pSelectAllButton = new QPushButton(m_pDialog);
    m_pInvertSelectionButton = new QPushButton(m_pDialog);
    m_pMarkSelectedReadButton = new QPushButton(m_pDialog);
    m_pExportButton = new QPushButton(m_pDialog);
    for (QPushButton *pButton : { m_pSelectAllButton, m_pInvertSelectionButton,
                                  m_pMarkSelectedReadButton, m_pExportButton })
    {
        pButton->setMinimumSize(QSize(48, md3Theme().controlHeight()));
        pBulkLayout->addWidget(pButton);
    }
    m_pSelectionSummary = new QLabel(m_pDialog);
    m_pSelectionSummary->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pBulkLayout->addWidget(m_pSelectionSummary, 1);
    pRootLayout->addLayout(pBulkLayout);

    QHBoxLayout *pDestructiveLayout = new QHBoxLayout;
    pDestructiveLayout->setContentsMargins(0, 0, 0, 0);
    m_pClearButton = new QPushButton(m_pDialog);
    m_pClearButton->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    m_pClearButton->setProperty("destructive", true);
    pDestructiveLayout->addWidget(m_pClearButton);
    m_pUndoClearButton = new QPushButton(m_pDialog);
    m_pUndoClearButton->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    pDestructiveLayout->addWidget(m_pUndoClearButton);
    pDestructiveLayout->addStretch(1);
    pRootLayout->addLayout(pDestructiveLayout);

    QScrollArea *pScrollArea = new QScrollArea(m_pDialog);
    pScrollArea->setWidgetResizable(true);
    pScrollArea->setFrameShape(QFrame::NoFrame);
    QWidget *pRowsWidget = new QWidget(pScrollArea);
    m_pRowsLayout = new QVBoxLayout(pRowsWidget);
    m_pRowsLayout->setContentsMargins(0, 0, 0, 0);
    m_pRowsLayout->setSpacing(g_iRowSpacing);
    pScrollArea->setWidget(pRowsWidget);
    pRootLayout->addWidget(pScrollArea, 1);

    connect(m_pSearchField, &UIMd3SearchField::sigFilterChanged,
            this, &UIMd3NotificationCentre::sltRefreshDialog);
    connect(m_pMarkAllReadButton, &QPushButton::clicked,
            this, &UIMd3NotificationCentre::sltMarkAllRead);
    connect(m_pSelectAllButton, &QPushButton::clicked,
            this, &UIMd3NotificationCentre::sltSelectAllVisible);
    connect(m_pInvertSelectionButton, &QPushButton::clicked,
            this, &UIMd3NotificationCentre::sltInvertVisibleSelection);
    connect(m_pMarkSelectedReadButton, &QPushButton::clicked,
            this, &UIMd3NotificationCentre::sltMarkSelectedRead);
    connect(m_pExportButton, &QPushButton::clicked,
            this, &UIMd3NotificationCentre::sltExportVisible);
    connect(m_pClearButton, &QPushButton::clicked,
            this, &UIMd3NotificationCentre::sltRequestClear);
    connect(m_pUndoClearButton, &QPushButton::clicked,
            this, &UIMd3NotificationCentre::sltUndoClear);
    connect(this, &UIMd3NotificationCentre::sigChanged,
            this, &UIMd3NotificationCentre::sltRefreshDialog,
            Qt::UniqueConnection);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3NotificationCentre::sltRetranslateUI,
                Qt::UniqueConnection);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged,
            this, &UIMd3NotificationCentre::sltRefreshDialog,
            Qt::UniqueConnection);
    connect(m_pDialog, &QObject::destroyed, this, [this]()
    {
        m_pDialog = 0;
        m_pSearchField = 0;
        m_pRowsLayout = 0;
        m_pMarkAllReadButton = 0;
        m_pSelectAllButton = 0;
        m_pInvertSelectionButton = 0;
        m_pMarkSelectedReadButton = 0;
        m_pExportButton = 0;
        m_pClearButton = 0;
        m_pUndoClearButton = 0;
        m_pSelectionSummary = 0;
    });

    m_pDialog->resize(600, 560);
    sltRetranslateUI();
    m_pDialog->show();
    m_pDialog->raise();
    m_pDialog->activateWindow();
}

void UIMd3NotificationCentre::sltMarkAllRead()
{
    markAllRead();
}

void UIMd3NotificationCentre::sltRetranslateUI()
{
    const QString strHistory = md3NotificationText("md3.notifications.history", tr("Notification history"));
    if (m_pDialog)
    {
        m_pDialog->setAccessibleName(strHistory);
        m_pDialog->setWindowTitle(strHistory);
    }
    if (m_pHeading)
    {
        m_pHeading->setText(strHistory);
        m_pHeading->setAccessibleName(strHistory);
    }
    if (m_pSearchField)
        m_pSearchField->setPlaceholderText(md3NotificationText("md3.notifications.search", tr("Search notifications")));
    if (m_pMarkAllReadButton)
    {
        const QString strMarkRead = md3NotificationText("md3.notifications.markRead", tr("Mark all as read"));
        m_pMarkAllReadButton->setText(strMarkRead);
        m_pMarkAllReadButton->setAccessibleName(strMarkRead);
    }
    if (m_pSelectAllButton)
    {
        const QString strSelectAll = md3NotificationText("md3.notifications.selectAll", tr("Select visible"));
        m_pSelectAllButton->setText(strSelectAll);
        m_pSelectAllButton->setAccessibleName(strSelectAll);
    }
    if (m_pInvertSelectionButton)
    {
        const QString strInvert = md3NotificationText("md3.notifications.invertSelection", tr("Invert selection"));
        m_pInvertSelectionButton->setText(strInvert);
        m_pInvertSelectionButton->setAccessibleName(strInvert);
    }
    if (m_pMarkSelectedReadButton)
    {
        const QString strMarkSelected = md3NotificationText("md3.notifications.markSelectedRead",
                                                             tr("Mark selected as read"));
        m_pMarkSelectedReadButton->setText(strMarkSelected);
        m_pMarkSelectedReadButton->setAccessibleName(strMarkSelected);
    }
    if (m_pExportButton)
    {
        const QString strExport = md3NotificationText("md3.notifications.export", tr("Export view"));
        m_pExportButton->setText(strExport);
        m_pExportButton->setAccessibleName(strExport);
    }
    if (m_pClearButton)
    {
        const QString strClear = md3NotificationText("md3.notifications.clear", tr("Clear history"));
        m_pClearButton->setText(strClear);
        m_pClearButton->setAccessibleName(strClear);
        m_pClearButton->setToolTip(strClear);
    }
    if (m_pUndoClearButton)
    {
        const QString strUndo = md3NotificationText("md3.notifications.undoClear", tr("Undo last clear"));
        m_pUndoClearButton->setText(strUndo);
        m_pUndoClearButton->setAccessibleName(strUndo);
        m_pUndoClearButton->setToolTip(tr("Restore the most recent recovery snapshot"));
    }
    sltRefreshDialog();
}

void UIMd3NotificationCentre::sltRefreshDialog()
{
    if (!m_pDialog || !m_pRowsLayout)
        return;

    while (QLayoutItem *pItem = m_pRowsLayout->takeAt(0))
    {
        if (QWidget *pWidget = pItem->widget())
            delete pWidget;
        delete pItem;
    }

    QSet<QString> validIds;
    for (const UIMd3Notice &notice : m_notices)
        validIds.insert(notice.strId);
    m_selectedIds.intersect(validIds);

    const QString strQuery = m_pSearchField ? m_pSearchField->text() : QString();
    int iVisibleCount = 0;
    for (const UIMd3Notice &notice : m_notices)
    {
        const QString strSearchText = notice.strTitle + QLatin1Char('\n')
                                     + notice.strDetail + QLatin1Char('\n')
                                     + notice.strCategory;
        if (m_pSearchField && !m_pSearchField->matches(strSearchText))
            continue;

        QWidget *pRow = new QFrame(m_pRowsLayout->parentWidget());
        pRow->setObjectName(QStringLiteral("md3NoticeRow_%1").arg(notice.strId));
        pRow->setAccessibleName(notice.strTitle);
        pRow->setAccessibleDescription(notice.strDetail);
        pRow->setAutoFillBackground(true);
        QPalette rowPalette = pRow->palette();
        rowPalette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainerHighest));
        pRow->setPalette(rowPalette);

        QVBoxLayout *pRowLayout = new QVBoxLayout(pRow);
        pRowLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                       md3Theme().gutter(), md3Theme().gutter());
        pRowLayout->setSpacing(4);

        QHBoxLayout *pTitleLayout = new QHBoxLayout;
        pTitleLayout->setContentsMargins(0, 0, 0, 0);
        QCheckBox *pCheckBox = new QCheckBox(pRow);
        pCheckBox->setChecked(m_selectedIds.contains(notice.strId));
        pCheckBox->setAccessibleName(notice.strTitle);
        pCheckBox->setToolTip(tr("Select this notification"));
        pTitleLayout->addWidget(pCheckBox);

        QLabel *pTitle = new QLabel(notice.strTitle, pRow);
        QFont titleFont = md3Theme().font(UIMd3TypeRole_TitleMedium);
        titleFont.setBold(notice.fUnread);
        pTitle->setFont(titleFont);
        if (notice.fError)
        {
            QPalette titlePalette = pTitle->palette();
            titlePalette.setColor(QPalette::WindowText, md3(UIMd3ColorRole_Error));
            pTitle->setPalette(titlePalette);
        }
        pTitleLayout->addWidget(pTitle, 1);
        pRowLayout->addLayout(pTitleLayout);
        const QString strNoticeId = notice.strId;
        connect(pCheckBox, &QCheckBox::toggled, this, [this, strNoticeId](bool fChecked)
        {
            if (fChecked)
                m_selectedIds.insert(strNoticeId);
            else
                m_selectedIds.remove(strNoticeId);
            updateBulkActions();
        });

        QLabel *pMetadata = new QLabel(QStringLiteral("%1 | %2")
                                       .arg(notice.strCategory,
                                            notice.when.toLocalTime().toString(Qt::ISODate)),
                                       pRow);
        pMetadata->setFont(md3Theme().font(UIMd3TypeRole_LabelMedium));
        pMetadata->setStyleSheet(QStringLiteral("color: %1;")
                                 .arg(md3(UIMd3ColorRole_OnSurfaceVariant).name()));
        pRowLayout->addWidget(pMetadata);

        if (!notice.strDetail.isEmpty())
        {
            QLabel *pDetail = new QLabel(notice.strDetail, pRow);
            pDetail->setTextFormat(Qt::PlainText);
            pDetail->setWordWrap(true);
            pDetail->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
            pRowLayout->addWidget(pDetail);
        }

        m_pRowsLayout->addWidget(pRow);
        ++iVisibleCount;
    }

    if (!iVisibleCount)
    {
        QLabel *pEmpty = new QLabel(strQuery.isEmpty()
                                    ? md3NotificationText("md3.notifications.empty", tr("No local notifications"))
                                    : md3NotificationText("md3.notifications.noMatch", tr("No notifications match this search")),
                                    m_pRowsLayout->parentWidget());
        pEmpty->setAlignment(Qt::AlignCenter);
        pEmpty->setWordWrap(true);
        pEmpty->setAccessibleName(pEmpty->text());
        pEmpty->setFont(md3Theme().font(UIMd3TypeRole_BodyLarge));
        m_pRowsLayout->addWidget(pEmpty);
    }
    m_pRowsLayout->addStretch(1);
    if (m_pMarkAllReadButton)
        m_pMarkAllReadButton->setEnabled(hasUnread());
    updateBulkActions();
}
