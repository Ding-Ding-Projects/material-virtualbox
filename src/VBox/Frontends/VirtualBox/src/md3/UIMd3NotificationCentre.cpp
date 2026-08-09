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
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSet>
#include <QSize>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QUuid>
#include <QWidget>

/* GUI includes: */
#include "UIMd3NotificationCentre.h"
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
{
    load();
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

void UIMd3NotificationCentre::load()
{
    m_notices.clear();

    const QString strPath = storagePath();
    if (strPath.isEmpty())
        return;

    QFile file(strPath);
    if (!file.exists() || file.size() <= 0 || file.size() > g_iMaxPayload)
        return;
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QByteArray data = file.read(g_iMaxPayload + 1);
    if (data.isEmpty() || data.size() > g_iMaxPayload)
        return;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return;

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("version")).toInt(-1) != g_iSchemaVersion)
        return;

    const QJsonArray records = root.value(QStringLiteral("notices")).toArray();
    if (records.isEmpty())
        return;

    QSet<QString> seenIds;
    const int iCount = qMin(records.size(), g_iMaxJsonArrayEntries);
    for (int i = 0; i < iCount && m_notices.size() < g_iMaxNotices; ++i)
    {
        const QJsonObject record = records.at(i).toObject();
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
        m_notices << notice;
        seenIds.insert(strId);
    }
}

void UIMd3NotificationCentre::save() const
{
    const QString strPath = storagePath();
    if (strPath.isEmpty())
        return;

    const QFileInfo fileInfo(strPath);
    if (!QDir().mkpath(fileInfo.absolutePath()))
        return;

    QJsonArray records;
    for (int i = 0; i < m_notices.size() && i < g_iMaxNotices; ++i)
    {
        const UIMd3Notice &notice = m_notices.at(i);
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
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (data.size() > g_iMaxPayload)
        return;
    if (file.write(data) != data.size())
        return;
    file.commit();
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

    m_notices.prepend(notice);
    while (m_notices.size() > g_iMaxNotices)
        m_notices.removeLast();
    save();
    emit sigNoticePosted(notice.strId);
    emit sigChanged();
    if (m_pDialog)
        sltRefreshDialog();
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
    emit sigChanged();
    if (m_pDialog)
        sltRefreshDialog();
}

void UIMd3NotificationCentre::clear()
{
    if (m_notices.isEmpty())
        return;
    m_notices.clear();
    save();
    emit sigChanged();
    if (m_pDialog)
        sltRefreshDialog();
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
    connect(this, &UIMd3NotificationCentre::sigChanged,
            this, &UIMd3NotificationCentre::sltRefreshDialog);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3NotificationCentre::sltRetranslateUI);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged,
            this, &UIMd3NotificationCentre::sltRefreshDialog);
    connect(m_pDialog, &QObject::destroyed, this, [this]()
    {
        m_pDialog = 0;
        m_pSearchField = 0;
        m_pRowsLayout = 0;
        m_pMarkAllReadButton = 0;
    });

    m_pDialog->resize(600, 560);
    sltRetranslateUI();
    sltRefreshDialog();
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
        pRowLayout->addWidget(pTitle);

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
}
