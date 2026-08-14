/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3PersonalVocabulary class implementation - local, user-supplied text replacement.
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
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSaveFile>
#include <QSize>
#include <QStandardPaths>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3PersonalVocabulary.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"

namespace
{
    /* Every bound below applies to the *complete* payload, before any of it
     * is used -- see UIMd3PersonalVocabulary::validateFile(). */

    /** Hard file-size ceiling for the picked file (and the local cache). */
    const qint64 g_iMaxFileSizeBytes = 512 * 1024;
    /** The only schema version this control understands. */
    const int g_iSupportedSchemaVersion = 1;
    /** Deepest brace/bracket nesting the declared schema allows: the root
      * object (depth 1) containing the "entries" object (depth 2). Replacement
      * values must be plain strings, so nothing legitimate ever nests deeper. */
    const int g_iMaxNestingDepth = 2;
    /** Bounded entry count, independent of the size and length limits below. */
    const int g_iMaxEntries = 2000;
    /** Bounded key length, in characters. */
    const int g_iMaxKeyLength = 200;
    /** Bounded replacement-value length, in characters. */
    const int g_iMaxValueLength = 1000;

    QString md3VocabularyText(const char *pszKey, const QString &strFallback)
    {
        UIMd3Language *pLanguage = UIMd3Language::instance();
        return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
    }
}

UIMd3PersonalVocabulary *UIMd3PersonalVocabulary::s_pInstance = 0;

UIMd3PersonalVocabulary *UIMd3PersonalVocabulary::instance()
{
    return s_pInstance;
}

void UIMd3PersonalVocabulary::create()
{
    if (s_pInstance)
        return;
    s_pInstance = new UIMd3PersonalVocabulary;
    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.title"),
                                                 QStringLiteral("Personal vocabulary"),
                                                 QStringLiteral("個人詞彙"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.explanation"),
                                                 QStringLiteral("Pick a local JSON file to replace specific words or "
                                                                 "phrases everywhere they appear. The file never leaves "
                                                                 "this device -- nothing here is ever sent over the "
                                                                 "network. Until you pick a valid file, everything shows "
                                                                 "its original wording."),
                                                 QStringLiteral("揀一個本機 JSON 檔案，就可以將指定嘅字詞喺成個介面度全部"
                                                                 "換走。呢個檔案永遠唔會傳出呢部機，呢度嘅嘢絕對唔會經"
                                                                 "網絡送出去。喺未揀到有效檔案之前，成個介面都會顯示"
                                                                 "原本嘅字眼。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.choose"),
                                                 QStringLiteral("Choose file..."),
                                                 QStringLiteral("揀選檔案..."));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.replace"),
                                                 QStringLiteral("Replace file..."),
                                                 QStringLiteral("更換檔案..."));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.clear"),
                                                 QStringLiteral("Clear"),
                                                 QStringLiteral("清除"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.filter"),
                                                 QStringLiteral("JSON files (*.json)"),
                                                 QStringLiteral("JSON 檔案 (*.json)"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.statusNoFile"),
                                                 QStringLiteral("No personal vocabulary file is loaded. Original wording is shown everywhere."),
                                                 QStringLiteral("未有揀選個人詞彙檔案。成個介面都會顯示原本嘅字眼。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.statusLoaded"),
                                                 QStringLiteral("Loaded %1 entries from %2."),
                                                 QStringLiteral("已經由 %2 載入 %1 項替換。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.statusLoadedCache"),
                                                 QStringLiteral("Restored %1 entries from the local cache of your last accepted file."),
                                                 QStringLiteral("已經由本機快取還原上次接受嘅檔案，共 %1 項替換。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.statusInvalid"),
                                                 QStringLiteral("That file was rejected: %1 Original wording is shown everywhere."),
                                                 QStringLiteral("嗰個檔案已被拒絕：%1 成個介面都會顯示原本嘅字眼。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.statusLoadedButRejected"),
                                                 QStringLiteral("Loaded %1 entries from %2. Your last replace attempt was rejected: %3"),
                                                 QStringLiteral("已經由 %2 載入 %1 項替換。上次嘗試更換已被拒絕：%3"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.cleared"),
                                                 QStringLiteral("Cleared. Original wording is shown everywhere."),
                                                 QStringLiteral("已清除。成個介面都會顯示原本嘅字眼。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorMissing"),
                                                 QStringLiteral("The selected file could not be found or read."),
                                                 QStringLiteral("搵唔到或者讀唔到揀選嘅檔案。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorSize"),
                                                 QStringLiteral("The file is empty or larger than the %1 KB limit."),
                                                 QStringLiteral("檔案係空嘅，或者超過 %1 KB 上限。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorDepth"),
                                                 QStringLiteral("The file is structured more deeply than this control supports."),
                                                 QStringLiteral("檔案嘅結構層數超出呢個功能支援嘅範圍。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorJson"),
                                                 QStringLiteral("The file is not valid JSON."),
                                                 QStringLiteral("檔案唔係有效嘅 JSON。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorFields"),
                                                 QStringLiteral("The file must contain exactly \"schemaVersion\" and \"entries\", and nothing else."),
                                                 QStringLiteral("檔案一定要淨係有「schemaVersion」同「entries」呢兩個欄位。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorVersion"),
                                                 QStringLiteral("This file's schema version is not supported."),
                                                 QStringLiteral("呢個檔案嘅結構版本未受支援。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorEntriesShape"),
                                                 QStringLiteral("\"entries\" must be an object of phrase-to-replacement pairs."),
                                                 QStringLiteral("「entries」一定要係一個「字詞 對 替換文字」嘅物件。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorCount"),
                                                 QStringLiteral("The file defines more than %1 entries."),
                                                 QStringLiteral("檔案定義咗超過 %1 項替換。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorKey"),
                                                 QStringLiteral("An entry key is empty or longer than %1 characters."),
                                                 QStringLiteral("有一個項目嘅字詞係空嘅，或者超過 %1 個字符。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorValueType"),
                                                 QStringLiteral("Every replacement value must be plain text."),
                                                 QStringLiteral("每一個替換值都一定要係純文字。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.vocabulary.errorValueLength"),
                                                 QStringLiteral("A replacement value is longer than %1 characters."),
                                                 QStringLiteral("有一個替換值超過 %1 個字符。"));
    }
    s_pInstance->loadCache();
}

void UIMd3PersonalVocabulary::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3PersonalVocabulary::UIMd3PersonalVocabulary()
    : QObject(0)
    , m_enmState(State_NoFile)
    , m_fRestoredFromCache(false)
    , m_pHeading(0)
    , m_pExplanation(0)
    , m_pStatus(0)
    , m_pChoose(0)
    , m_pClear(0)
{
}

UIMd3PersonalVocabulary::~UIMd3PersonalVocabulary()
{
}

QString UIMd3PersonalVocabulary::replacement(const QString &strOriginal) const
{
    if (m_enmState != State_Loaded || m_entries.isEmpty())
        return strOriginal;
    const QHash<QString, QString>::const_iterator it = m_entries.constFind(strOriginal);
    return it == m_entries.constEnd() ? strOriginal : it.value();
}

QString UIMd3PersonalVocabulary::cacheFilePath()
{
    const QString strLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (strLocation.isEmpty())
        return QString();
    QDir dir(strLocation);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
        return QString();
    return dir.filePath(QStringLiteral("personal-vocabulary-cache.json"));
}

/* static */
bool UIMd3PersonalVocabulary::payloadNestingWithinLimit(const QByteArray &data)
{
    bool fInString = false;
    bool fEscaped = false;
    int iDepth = 0;
    for (int i = 0; i < data.size(); ++i)
    {
        const char ch = data.at(i);
        if (fInString)
        {
            if (fEscaped)
                fEscaped = false;
            else if (ch == '\\')
                fEscaped = true;
            else if (ch == '"')
                fInString = false;
            continue;
        }
        if (ch == '"')
        {
            fInString = true;
            continue;
        }
        if (ch == '{' || ch == '[')
        {
            ++iDepth;
            if (iDepth > g_iMaxNestingDepth)
                return false;
        }
        else if (ch == '}' || ch == ']')
        {
            --iDepth;
        }
    }
    return true;
}

/* static */
bool UIMd3PersonalVocabulary::validateFile(const QString &strPath, QHash<QString, QString> &outEntries, QString &strError)
{
    strError.clear();

    const QFileInfo info(strPath);
    if (!info.exists() || !info.isFile())
    {
        strError = md3VocabularyText("md3.vocabulary.errorMissing", QStringLiteral("The selected file could not be found or read."));
        return false;
    }
    if (info.size() <= 0 || info.size() > g_iMaxFileSizeBytes)
    {
        strError = md3VocabularyText("md3.vocabulary.errorSize",
                                     QStringLiteral("The file is empty or larger than the %1 KB limit."))
                  .arg(g_iMaxFileSizeBytes / 1024);
        return false;
    }

    QFile file(strPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        strError = md3VocabularyText("md3.vocabulary.errorMissing", QStringLiteral("The selected file could not be found or read."));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();
    /* Re-check the bytes actually read: QFileInfo::size() can race with a file
     * that changes between the stat above and this read. Never trust a single
     * size observation for a hard limit. */
    if (data.isEmpty() || data.size() > g_iMaxFileSizeBytes)
    {
        strError = md3VocabularyText("md3.vocabulary.errorSize",
                                     QStringLiteral("The file is empty or larger than the %1 KB limit."))
                  .arg(g_iMaxFileSizeBytes / 1024);
        return false;
    }

    /* Bound nesting depth by scanning the raw bytes *before* any JSON parser
     * builds a tree from them, so a pathologically deep payload is rejected
     * up front rather than after being fully parsed into memory. */
    if (!payloadNestingWithinLimit(data))
    {
        strError = md3VocabularyText("md3.vocabulary.errorDepth",
                                     QStringLiteral("The file is structured more deeply than this control supports."));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        strError = md3VocabularyText("md3.vocabulary.errorJson", QStringLiteral("The file is not valid JSON."));
        return false;
    }

    const QJsonObject root = document.object();
    if (root.size() != 2
        || !root.contains(QStringLiteral("schemaVersion"))
        || !root.contains(QStringLiteral("entries")))
    {
        strError = md3VocabularyText("md3.vocabulary.errorFields",
                                     QStringLiteral("The file must contain exactly \"schemaVersion\" and \"entries\", and nothing else."));
        return false;
    }

    const QJsonValue versionValue = root.value(QStringLiteral("schemaVersion"));
    if (!versionValue.isDouble() || versionValue.toDouble() != double(g_iSupportedSchemaVersion))
    {
        strError = md3VocabularyText("md3.vocabulary.errorVersion", QStringLiteral("This file's schema version is not supported."));
        return false;
    }

    const QJsonValue entriesValue = root.value(QStringLiteral("entries"));
    if (!entriesValue.isObject())
    {
        strError = md3VocabularyText("md3.vocabulary.errorEntriesShape",
                                     QStringLiteral("\"entries\" must be an object of phrase-to-replacement pairs."));
        return false;
    }
    const QJsonObject entriesObject = entriesValue.toObject();
    if (entriesObject.size() > g_iMaxEntries)
    {
        strError = md3VocabularyText("md3.vocabulary.errorCount", QStringLiteral("The file defines more than %1 entries.")).arg(g_iMaxEntries);
        return false;
    }

    QHash<QString, QString> parsed;
    parsed.reserve(entriesObject.size());
    for (QJsonObject::const_iterator it = entriesObject.constBegin(); it != entriesObject.constEnd(); ++it)
    {
        const QString strKey = it.key();
        if (strKey.isEmpty() || strKey.length() > g_iMaxKeyLength)
        {
            strError = md3VocabularyText("md3.vocabulary.errorKey",
                                         QStringLiteral("An entry key is empty or longer than %1 characters.")).arg(g_iMaxKeyLength);
            return false;
        }
        const QJsonValue value = it.value();
        if (!value.isString())
        {
            strError = md3VocabularyText("md3.vocabulary.errorValueType", QStringLiteral("Every replacement value must be plain text."));
            return false;
        }
        const QString strValue = value.toString();
        if (strValue.length() > g_iMaxValueLength)
        {
            strError = md3VocabularyText("md3.vocabulary.errorValueLength",
                                         QStringLiteral("A replacement value is longer than %1 characters.")).arg(g_iMaxValueLength);
            return false;
        }
        parsed.insert(strKey, strValue);
    }

    /* Only now, after every bound above passed for the *complete* payload,
     * does anything reach the caller -- a rejected file never applies
     * partially. */
    outEntries = parsed;
    return true;
}

void UIMd3PersonalVocabulary::applyEntries(const QHash<QString, QString> &entries, const QString &strSourcePath)
{
    m_entries = entries;
    m_strSourcePath = strSourcePath;
    m_strLastError.clear();
    m_enmState = State_Loaded;
    m_fRestoredFromCache = strSourcePath.isEmpty();
    if (!m_fRestoredFromCache)
        saveCache();
    updateCentre();
    emit sigStateChanged();
}

void UIMd3PersonalVocabulary::loadCache()
{
    const QString strCachePath = cacheFilePath();
    if (strCachePath.isEmpty() || !QFileInfo::exists(strCachePath))
        return;

    QHash<QString, QString> parsed;
    QString strError;
    if (!validateFile(strCachePath, parsed, strError))
    {
        /* A corrupted or stale cache is never trusted; fall back to the
         * honest no-file state rather than guessing. */
        return;
    }
    /* Restored from cache, not freshly picked: leave sourcePath empty so the
     * status text is honest about where this run's mapping actually came
     * from, and avoid rewriting the cache with a no-op save. */
    applyEntries(parsed, QString());
}

void UIMd3PersonalVocabulary::saveCache() const
{
    const QString strCachePath = cacheFilePath();
    if (strCachePath.isEmpty())
        return;

    QJsonObject entriesObject;
    for (QHash<QString, QString>::const_iterator it = m_entries.constBegin(); it != m_entries.constEnd(); ++it)
        entriesObject.insert(it.key(), it.value());
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), g_iSupportedSchemaVersion);
    root.insert(QStringLiteral("entries"), entriesObject);
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    if (data.size() > g_iMaxFileSizeBytes)
        return; /* Should not happen for an already-validated set; never write an unbounded cache. */

    QSaveFile file(strCachePath);
    if (!file.open(QIODevice::WriteOnly))
        return;
    if (file.write(data) == data.size())
        file.commit();
}

void UIMd3PersonalVocabulary::clearCache() const
{
    const QString strCachePath = cacheFilePath();
    if (!strCachePath.isEmpty())
        QFile::remove(strCachePath);
}

void UIMd3PersonalVocabulary::showCentre(QWidget *pParent)
{
    if (m_pDialog)
    {
        m_pDialog->show();
        m_pDialog->raise();
        m_pDialog->activateWindow();
        updateCentre();
        return;
    }
    if (!UIMd3Theme::instance())
        return;

    m_pDialog = new QDialog(pParent, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    m_pDialog->setObjectName(QStringLiteral("md3PersonalVocabulary"));
    m_pDialog->setMinimumSize(QSize(420, 260));
    QPalette palette = m_pDialog->palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Base, md3(UIMd3ColorRole_SurfaceContainer));
    m_pDialog->setAutoFillBackground(true);
    m_pDialog->setPalette(palette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(m_pDialog);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(md3Theme().gutter() / 2);

    m_pHeading = new QLabel(m_pDialog);
    m_pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pRootLayout->addWidget(m_pHeading);

    m_pExplanation = new QLabel(m_pDialog);
    m_pExplanation->setWordWrap(true);
    m_pExplanation->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
    QPalette explanationPalette = m_pExplanation->palette();
    explanationPalette.setColor(QPalette::WindowText, md3(UIMd3ColorRole_OnSurfaceVariant));
    m_pExplanation->setPalette(explanationPalette);
    pRootLayout->addWidget(m_pExplanation);

    /* The picker control is always visible here, even before any file has
     * ever been picked -- it is never hidden behind an intermediate step. */
    QHBoxLayout *pPickerLayout = new QHBoxLayout;
    pPickerLayout->setContentsMargins(0, 0, 0, 0);
    pPickerLayout->setSpacing(md3Theme().gutter() / 2);
    m_pChoose = new QPushButton(m_pDialog);
    m_pChoose->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    m_pClear = new QPushButton(m_pDialog);
    m_pClear->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    pPickerLayout->addWidget(m_pChoose);
    pPickerLayout->addWidget(m_pClear);
    pPickerLayout->addStretch(1);
    pRootLayout->addLayout(pPickerLayout);

    m_pStatus = new QLabel(m_pDialog);
    m_pStatus->setWordWrap(true);
    m_pStatus->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
    pRootLayout->addWidget(m_pStatus);
    pRootLayout->addStretch(1);

    connect(m_pChoose, &QPushButton::clicked, this, &UIMd3PersonalVocabulary::sltChooseFile);
    connect(m_pClear, &QPushButton::clicked, this, &UIMd3PersonalVocabulary::sltClear);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3PersonalVocabulary::sltRetranslateUI, Qt::UniqueConnection);
    connect(m_pDialog, &QObject::destroyed, this, [this]()
    {
        m_pDialog = 0;
        m_pHeading = 0;
        m_pExplanation = 0;
        m_pStatus = 0;
        m_pChoose = 0;
        m_pClear = 0;
    });

    sltRetranslateUI();
    m_pDialog->resize(560, 340);
    m_pDialog->show();
    m_pDialog->raise();
    m_pDialog->activateWindow();
    m_pChoose->setFocus(Qt::ShortcutFocusReason);
}

void UIMd3PersonalVocabulary::sltChooseFile()
{
    if (!m_pDialog)
        return;

    const QString strPath = QFileDialog::getOpenFileName(
        m_pDialog,
        m_enmState == State_Loaded
            ? md3VocabularyText("md3.vocabulary.replace", QStringLiteral("Replace file..."))
            : md3VocabularyText("md3.vocabulary.choose", QStringLiteral("Choose file...")),
        QDir::homePath(),
        md3VocabularyText("md3.vocabulary.filter", QStringLiteral("JSON files (*.json)")));
    if (strPath.isEmpty())
        return;

    QHash<QString, QString> parsed;
    QString strError;
    if (!validateFile(strPath, parsed, strError))
    {
        m_strLastError = strError;
        /* A rejected pick never applies partially, and it never discards an
         * already-active, previously validated vocabulary either -- only the
         * error annotation changes. If nothing was active yet, the honest
         * state becomes Invalid rather than silently staying NoFile. */
        if (m_enmState != State_Loaded)
            m_enmState = State_Invalid;
        updateCentre();
        emit sigStateChanged();
        return;
    }

    applyEntries(parsed, strPath);
}

void UIMd3PersonalVocabulary::sltClear()
{
    m_entries.clear();
    m_strSourcePath.clear();
    m_strLastError.clear();
    m_fRestoredFromCache = false;
    m_enmState = State_NoFile;
    clearCache();
    updateCentre();
    emit sigStateChanged();
}

void UIMd3PersonalVocabulary::updateCentre()
{
    if (!m_pStatus || !m_pChoose || !m_pClear)
        return;

    QString strStatus;
    switch (m_enmState)
    {
        case State_Loaded:
            strStatus = m_strLastError.isEmpty()
                ? (m_fRestoredFromCache
                       ? md3VocabularyText("md3.vocabulary.statusLoadedCache",
                                          QStringLiteral("Restored %1 entries from the local cache of your last accepted file."))
                             .arg(m_entries.size())
                       : md3VocabularyText("md3.vocabulary.statusLoaded", QStringLiteral("Loaded %1 entries from %2."))
                             .arg(m_entries.size()).arg(QDir::toNativeSeparators(m_strSourcePath)))
                : md3VocabularyText("md3.vocabulary.statusLoadedButRejected",
                                    QStringLiteral("Loaded %1 entries from %2. Your last replace attempt was rejected: %3"))
                      .arg(m_entries.size()).arg(QDir::toNativeSeparators(m_strSourcePath)).arg(m_strLastError);
            break;
        case State_Invalid:
            strStatus = md3VocabularyText("md3.vocabulary.statusInvalid",
                                          QStringLiteral("That file was rejected: %1 Original wording is shown everywhere."))
                       .arg(m_strLastError);
            break;
        case State_NoFile:
        default:
            strStatus = md3VocabularyText("md3.vocabulary.statusNoFile",
                                          QStringLiteral("No personal vocabulary file is loaded. Original wording is shown everywhere."));
            break;
    }
    m_pStatus->setText(strStatus);
    m_pStatus->setAccessibleName(strStatus);

    const QString strChooseLabel = m_enmState == State_Loaded
        ? md3VocabularyText("md3.vocabulary.replace", QStringLiteral("Replace file..."))
        : md3VocabularyText("md3.vocabulary.choose", QStringLiteral("Choose file..."));
    m_pChoose->setText(strChooseLabel);
    m_pChoose->setAccessibleName(strChooseLabel);

    const QString strClearLabel = md3VocabularyText("md3.vocabulary.clear", QStringLiteral("Clear"));
    m_pClear->setText(strClearLabel);
    m_pClear->setAccessibleName(strClearLabel);
    m_pClear->setEnabled(m_enmState == State_Loaded);
}

void UIMd3PersonalVocabulary::sltRetranslateUI()
{
    if (!m_pDialog)
        return;

    const QString strTitle = md3VocabularyText("md3.vocabulary.title", QStringLiteral("Personal vocabulary"));
    m_pDialog->setWindowTitle(strTitle);
    m_pDialog->setAccessibleName(strTitle);
    if (m_pHeading)
    {
        m_pHeading->setText(strTitle);
        m_pHeading->setAccessibleName(strTitle);
    }
    if (m_pExplanation)
    {
        const QString strExplanation = md3VocabularyText("md3.vocabulary.explanation",
            QStringLiteral("Pick a local JSON file to replace specific words or phrases everywhere they appear. "
                           "The file never leaves this device -- nothing here is ever sent over the network. "
                           "Until you pick a valid file, everything shows its original wording."));
        m_pExplanation->setText(strExplanation);
        m_pExplanation->setAccessibleName(strExplanation);
    }
    updateCentre();
}
