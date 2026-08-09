/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 anchored regular-expression builder.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, in version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QAbstractButton>
#include <QAccessible>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTextCursor>
#include <QTimer>
#include <QThreadPool>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QVBoxLayout>

/* C++ includes: */
#include <atomic>

/* GUI includes: */
#include "UIMd3Language.h"
#include "UIMd3RegexBuilder.h"

/** Maximum source characters evaluated by the live preview. */
static const int g_cchRegexPreviewSample = 4096;
/** Maximum result rows produced by one live preview. */
static const int g_cRegexPreviewMatches = 256;
/** Maximum match and capture rows retained by one preview. */
static const int g_cRegexPreviewRows = 2048;
/** Maximum visible characters retained by one match or capture row. */
static const int g_cchRegexPreviewCapture = 512;
/** Cooperative wall-clock budget for one bounded preview. */
static const int g_msRegexPreviewBudget = 300;
/** Process-wide ceiling preventing abandoned hostile previews from exhausting the shared pool. */
static const int g_cRegexPreviewWorkers = 2;
/** Number of regular-expression preview workers currently executing. */
static std::atomic<int> g_cActiveRegexPreviewWorkers(0);

static void md3RegisterRegexBuilderText()
{
    UIMd3Language *pLanguage = UIMd3Language::instance();
    if (!pLanguage)
        return;
#define REGISTER_REGEX_TEXT(a_pszKey, a_pszEnglish, a_pszCantonese) \
    pLanguage->registerText(QStringLiteral(a_pszKey), QStringLiteral(a_pszEnglish), QStringLiteral(a_pszCantonese))
    REGISTER_REGEX_TEXT("md3.regex.title", "Regex builder", "Regex 建構器");
    REGISTER_REGEX_TEXT("md3.regex.dialect", "Engine: Qt QRegularExpression (PCRE2-compatible). Backslash escapes follow PCRE2; supported flags are i, m, s, and x. Preview input stays local and is limited to 4,096 characters.", "引擎：Qt QRegularExpression（兼容 PCRE2）。反斜線跳脫跟 PCRE2；支援旗標 i、m、s、x。預覽內容只留喺本機，最多 4,096 個字元。");
    REGISTER_REGEX_TEXT("md3.regex.guided", "Guided construction", "引導式建構");
    REGISTER_REGEX_TEXT("md3.regex.literal", "Literal", "字面文字");
    REGISTER_REGEX_TEXT("md3.regex.literal-placeholder", "Text to escape", "要跳脫嘅文字");
    REGISTER_REGEX_TEXT("md3.regex.insert-literal", "Insert escaped literal", "插入已跳脫文字");
    REGISTER_REGEX_TEXT("md3.regex.character-class", "Character class", "字元類別");
    REGISTER_REGEX_TEXT("md3.regex.class-digit", "Digit \\d", "數字 \\d");
    REGISTER_REGEX_TEXT("md3.regex.class-word", "Word \\w", "文字字元 \\w");
    REGISTER_REGEX_TEXT("md3.regex.class-space", "Whitespace \\s", "空白字元 \\s");
    REGISTER_REGEX_TEXT("md3.regex.class-any", "Any character .", "任何字元 .");
    REGISTER_REGEX_TEXT("md3.regex.class-custom", "Custom class […]", "自訂字元類別 […]");
    REGISTER_REGEX_TEXT("md3.regex.anchor", "Anchor", "錨點");
    REGISTER_REGEX_TEXT("md3.regex.anchor-start", "Start of subject ^", "內容開頭 ^");
    REGISTER_REGEX_TEXT("md3.regex.anchor-end", "End of subject $", "內容結尾 $");
    REGISTER_REGEX_TEXT("md3.regex.anchor-word", "Word boundary \\b", "文字邊界 \\b");
    REGISTER_REGEX_TEXT("md3.regex.anchor-absolute-start", "Absolute start \\A", "絕對開頭 \\A");
    REGISTER_REGEX_TEXT("md3.regex.anchor-absolute-end", "Absolute end \\z", "絕對結尾 \\z");
    REGISTER_REGEX_TEXT("md3.regex.group", "Group", "群組");
    REGISTER_REGEX_TEXT("md3.regex.group-capturing", "Capturing group (…)", "擷取群組 (…)");
    REGISTER_REGEX_TEXT("md3.regex.group-noncapturing", "Non-capturing group (?:…)", "非擷取群組 (?:…)");
    REGISTER_REGEX_TEXT("md3.regex.group-named", "Named group (?<name>…)", "命名群組 (?<name>…)");
    REGISTER_REGEX_TEXT("md3.regex.group-positive-lookahead", "Positive lookahead (?=…)", "正向前瞻 (?=…)");
    REGISTER_REGEX_TEXT("md3.regex.group-negative-lookahead", "Negative lookahead (?!…)", "負向前瞻 (?!…)");
    REGISTER_REGEX_TEXT("md3.regex.quantifier", "Quantifier", "量詞");
    REGISTER_REGEX_TEXT("md3.regex.quantifier-optional", "Optional ?", "可有可無 ?");
    REGISTER_REGEX_TEXT("md3.regex.quantifier-zero-more", "Zero or more *", "零次或以上 *");
    REGISTER_REGEX_TEXT("md3.regex.quantifier-one-more", "One or more +", "一次或以上 +");
    REGISTER_REGEX_TEXT("md3.regex.quantifier-exact", "Exactly n {n}", "啱啱 n 次 {n}");
    REGISTER_REGEX_TEXT("md3.regex.quantifier-between", "Between n and m {n,m}", "n 至 m 次 {n,m}");
    REGISTER_REGEX_TEXT("md3.regex.minimum-repeats", "Minimum repeats", "最少重複次數");
    REGISTER_REGEX_TEXT("md3.regex.maximum-repeats", "Maximum repeats", "最多重複次數");
    REGISTER_REGEX_TEXT("md3.regex.insert", "Insert", "插入");
    REGISTER_REGEX_TEXT("md3.regex.alternation", "Insert alternation |", "插入或者符號 |");
    REGISTER_REGEX_TEXT("md3.regex.pattern", "Pattern", "模式");
    REGISTER_REGEX_TEXT("md3.regex.flags", "Flags", "旗標");
    REGISTER_REGEX_TEXT("md3.regex.flags-placeholder", "i m s x", "i m s x");
    REGISTER_REGEX_TEXT("md3.regex.sample", "Sample text", "範例文字");
    REGISTER_REGEX_TEXT("md3.regex.use", "Use regex", "使用 regex");
    REGISTER_REGEX_TEXT("md3.regex.validation", "Regex validation status", "Regex 驗證狀態");
    REGISTER_REGEX_TEXT("md3.regex.matches", "Live matches and capture groups", "即時配對同擷取群組");
    REGISTER_REGEX_TEXT("md3.regex.column-group", "Match or group", "配對或群組");
    REGISTER_REGEX_TEXT("md3.regex.column-span", "Span", "範圍");
    REGISTER_REGEX_TEXT("md3.regex.column-text", "Text", "文字");
    REGISTER_REGEX_TEXT("md3.regex.plain-status", "Plain-text mode: special characters are literal.", "純文字模式：特殊字元會當普通文字。");
    REGISTER_REGEX_TEXT("md3.regex.invalid-flags", "Invalid flags: use each of i, m, s, and x at most once.", "旗標無效：i、m、s、x 每個最多用一次。");
    REGISTER_REGEX_TEXT("md3.regex.invalid-pattern", "Invalid pattern: %1", "模式無效：%1");
    REGISTER_REGEX_TEXT("md3.regex.pending", "Evaluating the bounded preview…", "計緊有上限嘅預覽…");
    REGISTER_REGEX_TEXT("md3.regex.match", "Match %1", "配對 %1");
    REGISTER_REGEX_TEXT("md3.regex.valid", "Valid pattern · %1 match(es) · evaluated in %2 ms", "模式有效 · %1 個配對 · 用咗 %2 毫秒");
    REGISTER_REGEX_TEXT("md3.regex.truncated", "Preview stopped after %1 matches.", "預覽喺 %1 個配對後停止。");
    REGISTER_REGEX_TEXT("md3.regex.timeout", "Preview exceeded the 300 ms budget. Shorten the pattern or sample before applying it.", "預覽超過 300 毫秒上限。套用之前請縮短模式或範例。");
    REGISTER_REGEX_TEXT("md3.regex.worker-busy", "Regex preview capacity is busy with earlier bounded evaluations. Close and reopen the builder after they finish, or shorten the input.", "Regex 預覽容量仲處理緊之前有上限嘅計算。等佢完成後重新開啟建構器，或者縮短輸入。");
    REGISTER_REGEX_TEXT("md3.regex.use-plain", "Use plain text", "使用純文字");
    REGISTER_REGEX_TEXT("md3.regex.copy", "Copy pattern", "複製模式");
    REGISTER_REGEX_TEXT("md3.regex.export", "Export JSON…", "匯出 JSON…");
    REGISTER_REGEX_TEXT("md3.regex.apply", "Apply regex", "套用 regex");
    REGISTER_REGEX_TEXT("md3.regex.cancel", "Cancel", "取消");
    REGISTER_REGEX_TEXT("md3.regex.copied", "Pattern copied to the clipboard.", "模式已複製到剪貼簿。");
    REGISTER_REGEX_TEXT("md3.regex.export-title", "Export regular expression", "匯出正規表示式");
    REGISTER_REGEX_TEXT("md3.regex.exported", "Expression exported to %1.", "表示式已匯出到 %1。");
    REGISTER_REGEX_TEXT("md3.regex.export-failed", "Could not export the expression to %1.", "無法將表示式匯出到 %1。");
#undef REGISTER_REGEX_TEXT
}

static QString md3RegexText(const char *pszKey, const QString &strFallback)
{
    if (!UIMd3Language::instance())
        return strFallback;
    const QString strKey = QString::fromLatin1(pszKey);
    const QString strText = md3Text(strKey);
    return strText == strKey || strText.isEmpty() ? strFallback : strText;
}

static void md3AnnounceRegexStatus(QLabel *pStatus, const QString &strText)
{
    if (!pStatus)
        return;
    pStatus->setText(strText);
    pStatus->setToolTip(strText);
    pStatus->setAccessibleDescription(strText);
    if (QAccessible::isActive())
    {
        QAccessibleAnnouncementEvent event(pStatus, strText);
        QAccessible::updateAccessibility(&event);
    }
}

static bool md3ValidateRegexFlags(const QString &strFlags, QString *pstrNormalized = 0)
{
    QString strResult;
    const QString strSupported = QStringLiteral("imsx");
    for (const QChar ch : strFlags.toLower().left(16))
    {
        if (ch.isSpace())
            continue;
        if (!strSupported.contains(ch) || strResult.contains(ch))
            return false;
        strResult += ch;
    }
    if (pstrNormalized)
        *pstrNormalized = strResult;
    return true;
}

static void md3AddConstruct(QComboBox *pCombo, const QString &strLabel,
                            const QString &strPrefix, const QString &strSuffix = QString())
{
    QVariantList data;
    data << strPrefix << strSuffix;
    pCombo->addItem(strLabel, data);
}

UIMd3RegexBuilder::UIMd3RegexBuilder(QWidget *pParent)
    : QDialog(pParent)
    , m_pPattern(0)
    , m_pFlags(0)
    , m_pLiteral(0)
    , m_pSample(0)
    , m_pRegexMode(0)
    , m_pStatus(0)
    , m_pMatches(0)
    , m_pPreviewTimer(0)
    , m_pPreviewTimeout(0)
    , m_iPreviewGeneration(0)
    , m_fPreviewPending(false)
    , m_fPreviewTimedOut(false)
    , m_fWorkerRunning(false)
    , m_fPreviewRerunRequested(false)
{
    md3RegisterRegexBuilderText();
    setWindowTitle(md3RegexText("md3.regex.title", tr("Regex builder")));
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, true);
    prepare();
}

void UIMd3RegexBuilder::setPattern(const QString &strPattern, const QString &strFlags)
{
    if (m_pPattern)
        m_pPattern->setText(strPattern.left(4096));
    if (m_pFlags)
        m_pFlags->setText(strFlags.left(16));
    sltUpdatePreview();
}

QRegularExpression::PatternOptions UIMd3RegexBuilder::patternOptions() const
{
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    QString strFlags;
    md3ValidateRegexFlags(m_pFlags ? m_pFlags->text() : QString(), &strFlags);
    if (strFlags.contains('i')) options |= QRegularExpression::CaseInsensitiveOption;
    if (strFlags.contains('m')) options |= QRegularExpression::MultilineOption;
    if (strFlags.contains('s')) options |= QRegularExpression::DotMatchesEverythingOption;
    if (strFlags.contains('x')) options |= QRegularExpression::ExtendedPatternSyntaxOption;
    return options;
}

void UIMd3RegexBuilder::insertConstruct(const QString &strPrefix, const QString &strSuffix)
{
    if (!m_pPattern)
        return;
    int iStart = m_pPattern->selectionStart();
    const QString strSelection = m_pPattern->selectedText();
    if (iStart < 0)
        iStart = m_pPattern->cursorPosition();
    m_pPattern->setSelection(iStart, strSelection.size());
    m_pPattern->insert(strPrefix + strSelection + strSuffix);
    m_pPattern->setCursorPosition(iStart + strPrefix.size() + strSelection.size());
    m_pPattern->setFocus(Qt::OtherFocusReason);
}

void UIMd3RegexBuilder::insertSuffix(const QString &strSuffix)
{
    if (!m_pPattern)
        return;
    int iStart = m_pPattern->selectionStart();
    const QString strSelection = m_pPattern->selectedText();
    if (iStart < 0)
        iStart = m_pPattern->cursorPosition();
    m_pPattern->setSelection(iStart, strSelection.size());
    m_pPattern->insert(strSelection + strSuffix);
    m_pPattern->setFocus(Qt::OtherFocusReason);
}

void UIMd3RegexBuilder::insertLiteral()
{
    if (!m_pPattern || !m_pLiteral)
        return;
    const QString strLiteral = m_pLiteral->text().left(256);
    if (strLiteral.isEmpty())
    {
        m_pLiteral->setFocus(Qt::OtherFocusReason);
        return;
    }
    const int iStart = m_pPattern->selectionStart() >= 0
                     ? m_pPattern->selectionStart() : m_pPattern->cursorPosition();
    m_pPattern->setSelection(iStart, m_pPattern->selectedText().size());
    m_pPattern->insert(QRegularExpression::escape(strLiteral));
    m_pPattern->setFocus(Qt::OtherFocusReason);
}

void UIMd3RegexBuilder::sltSchedulePreview()
{
    ++m_iPreviewGeneration;
    m_fPreviewPending = true;
    m_fPreviewTimedOut = false;
    if (m_pPreviewTimeout)
        m_pPreviewTimeout->stop();
    if (m_pStatus)
        m_pStatus->setText(md3RegexText("md3.regex.pending",
                                       tr("Evaluating the bounded preview…")));
    if (m_pPreviewTimer)
        m_pPreviewTimer->start();
}

void UIMd3RegexBuilder::sltUpdatePreview()
{
    if (!m_pStatus || !m_pPattern || !m_pRegexMode || !m_pMatches)
        return;
    ++m_iPreviewGeneration;
    if (m_pPreviewTimer)
        m_pPreviewTimer->stop();
    m_fPreviewPending = false;
    m_fPreviewTimedOut = false;
    if (m_pPreviewTimeout)
        m_pPreviewTimeout->stop();
    m_pMatches->clear();
    if (!m_pRegexMode->isChecked())
    {
        m_pStatus->setText(md3RegexText("md3.regex.plain-status",
                                       tr("Plain-text mode: special characters are literal.")));
        return;
    }

    if (!md3ValidateRegexFlags(m_pFlags ? m_pFlags->text() : QString()))
    {
        m_pStatus->setText(md3RegexText("md3.regex.invalid-flags",
                                       tr("Invalid flags: use each of i, m, s, and x at most once.")));
        return;
    }

    const QRegularExpression regex(m_pPattern->text(), patternOptions());
    if (!regex.isValid())
    {
        m_pStatus->setText(md3RegexText("md3.regex.invalid-pattern",
                                       tr("Invalid pattern: %1")).arg(regex.errorString()));
        return;
    }

    const QString strSample = m_pSample ? m_pSample->toPlainText().left(g_cchRegexPreviewSample)
                                         : QString();
    if (m_fWorkerRunning)
    {
        m_fPreviewPending = true;
        m_fPreviewRerunRequested = true;
        m_pStatus->setText(md3RegexText("md3.regex.pending",
                                       tr("Evaluating the bounded preview…")));
        if (m_pPreviewTimeout)
            m_pPreviewTimeout->start(g_msRegexPreviewBudget);
        return;
    }
    const QString strPattern = regex.pattern();
    const QRegularExpression::PatternOptions options = regex.patternOptions();
    const int iGeneration = m_iPreviewGeneration;
    if (g_cActiveRegexPreviewWorkers.load() >= g_cRegexPreviewWorkers)
    {
        m_fPreviewPending = false;
        m_fPreviewTimedOut = true;
        md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.worker-busy",
            tr("Regex preview capacity is busy with earlier bounded evaluations. Close and reopen the builder after they finish, or shorten the input.")));
        return;
    }
    ++g_cActiveRegexPreviewWorkers;
    m_fPreviewPending = true;
    m_fWorkerRunning = true;
    m_fPreviewRerunRequested = false;
    m_pStatus->setText(md3RegexText("md3.regex.pending",
                                   tr("Evaluating the bounded preview…")));
    if (m_pPreviewTimeout)
        m_pPreviewTimeout->start(g_msRegexPreviewBudget);
    const QPointer<UIMd3RegexBuilder> pGuard(this);
    QThreadPool::globalInstance()->start([pGuard, strPattern, options, strSample, iGeneration]()
    {
        const QRegularExpression workerRegex(strPattern, options);
        const QStringList names = workerRegex.namedCaptureGroups();
        QStringList rows;
        QElapsedTimer timer;
        timer.start();
        int cMatches = 0;
        bool fTimedOut = false;
        bool fTruncated = false;
        QRegularExpressionMatchIterator iterator = workerRegex.globalMatch(strSample);
        while (iterator.hasNext())
        {
            if (timer.elapsed() >= g_msRegexPreviewBudget)
            {
                fTimedOut = true;
                break;
            }
            if (   cMatches >= g_cRegexPreviewMatches
                || rows.size() / 4 >= g_cRegexPreviewRows)
            {
                fTruncated = true;
                break;
            }
            const QRegularExpressionMatch match = iterator.next();
            const int iMatchIndex = cMatches++;
            rows << QString()
                 << QString::number(cMatches)
                 << QStringLiteral("%1–%2").arg(match.capturedStart(0)).arg(match.capturedEnd(0))
                 << match.captured(0).left(g_cchRegexPreviewCapture);
            for (int iCapture = 1; iCapture <= workerRegex.captureCount(); ++iCapture)
            {
                if (rows.size() / 4 >= g_cRegexPreviewRows)
                {
                    fTruncated = true;
                    break;
                }
                const QString strName = iCapture < names.size() && !names.at(iCapture).isEmpty()
                                      ? names.at(iCapture) : QStringLiteral("#%1").arg(iCapture);
                const int iCaptureStart = match.capturedStart(iCapture);
                const int iCaptureEnd = match.capturedEnd(iCapture);
                rows << QString::number(iMatchIndex)
                     << strName
                     << (iCaptureStart >= 0
                         ? QStringLiteral("%1–%2").arg(iCaptureStart).arg(iCaptureEnd)
                         : QStringLiteral("—"))
                     << match.captured(iCapture).left(g_cchRegexPreviewCapture);
            }
        }
        const qint64 cElapsedMilliseconds = timer.elapsed();
        fTimedOut |= cElapsedMilliseconds >= g_msRegexPreviewBudget;
        --g_cActiveRegexPreviewWorkers;
        QMetaObject::invokeMethod(qApp,
            [pGuard, iGeneration, rows, cMatches, cElapsedMilliseconds, fTruncated, fTimedOut]()
        {
            if (pGuard)
                pGuard->applyPreview(iGeneration, rows, cMatches,
                                     cElapsedMilliseconds, fTruncated, fTimedOut);
        }, Qt::QueuedConnection);
    });
}

void UIMd3RegexBuilder::sltPreviewTimedOut()
{
    if (!m_fPreviewPending)
        return;
    m_fPreviewPending = false;
    m_fPreviewTimedOut = true;
    if (m_pMatches)
        m_pMatches->clear();
    if (m_pStatus)
        md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.timeout",
            tr("Preview exceeded the 300 ms budget. Shorten the pattern or sample before applying it.")));
}

void UIMd3RegexBuilder::applyPreview(int iGeneration, const QStringList &rows,
                                     int cMatches, qint64 cElapsedMilliseconds,
                                     bool fTruncated, bool fTimedOut)
{
    m_fWorkerRunning = false;
    if (m_fPreviewRerunRequested || iGeneration != m_iPreviewGeneration)
    {
        const bool fShouldRerun = m_fPreviewPending && !m_fPreviewTimedOut;
        m_fPreviewRerunRequested = false;
        if (fShouldRerun)
            sltUpdatePreview();
        return;
    }
    if (   iGeneration != m_iPreviewGeneration
        || !m_fPreviewPending
        || !m_pMatches
        || !m_pStatus)
        return;
    m_fPreviewPending = false;
    if (m_pPreviewTimeout)
        m_pPreviewTimeout->stop();
    if (fTimedOut)
    {
        m_fPreviewTimedOut = true;
        m_pMatches->clear();
        md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.timeout",
            tr("Preview exceeded the 300 ms budget. Shorten the pattern or sample before applying it.")));
        return;
    }
    m_fPreviewTimedOut = false;
    m_pMatches->clear();
    QList<QTreeWidgetItem*> matchItems;
    for (int i = 0; i + 3 < rows.size(); i += 4)
    {
        const QString strParent = rows.at(i);
        if (strParent.isEmpty())
        {
            QTreeWidgetItem *pMatch = new QTreeWidgetItem(m_pMatches,
                QStringList() << md3RegexText("md3.regex.match", tr("Match %1")).arg(rows.at(i + 1))
                              << rows.at(i + 2)
                              << rows.at(i + 3));
            matchItems << pMatch;
        }
        else
        {
            const int iParent = strParent.toInt();
            if (iParent >= 0 && iParent < matchItems.size())
                new QTreeWidgetItem(matchItems.at(iParent),
                    QStringList() << rows.at(i + 1) << rows.at(i + 2) << rows.at(i + 3));
        }
    }
    QString strStatus = md3RegexText("md3.regex.valid",
        tr("Valid pattern · %1 match(es) · evaluated in %2 ms"))
        .arg(cMatches).arg(cElapsedMilliseconds);
    if (fTruncated)
        strStatus += QStringLiteral(" ") + md3RegexText("md3.regex.truncated",
                     tr("Preview stopped after %1 matches.")).arg(g_cRegexPreviewMatches);
    m_pStatus->setText(strStatus);
    m_pMatches->expandAll();
    m_pMatches->resizeColumnToContents(0);
    m_pMatches->resizeColumnToContents(1);
}

void UIMd3RegexBuilder::sltAcceptPattern()
{
    if (!m_pRegexMode || !m_pRegexMode->isChecked() || !m_pPattern)
        return;
    if (m_fPreviewPending)
    {
        md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.pending",
                               tr("Evaluating the bounded preview…")));
        return;
    }
    if (m_fPreviewTimedOut)
    {
        md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.timeout",
            tr("Preview exceeded the 300 ms budget. Shorten the pattern or sample before applying it.")));
        return;
    }
    QString strFlags;
    if (!md3ValidateRegexFlags(m_pFlags ? m_pFlags->text() : QString(), &strFlags))
    {
        sltUpdatePreview();
        md3AnnounceRegexStatus(m_pStatus, m_pStatus->text());
        return;
    }
    const QRegularExpression regex(m_pPattern->text(), patternOptions());
    if (!regex.isValid())
    {
        sltUpdatePreview();
        md3AnnounceRegexStatus(m_pStatus, m_pStatus->text());
        return;
    }
    emit sigPatternAccepted(m_pPattern->text(), strFlags);
    close();
}

void UIMd3RegexBuilder::sltUsePlainText()
{
    emit sigPlainTextRequested();
    close();
}

void UIMd3RegexBuilder::sltCopyPattern()
{
    if (!m_pPattern)
        return;
    QApplication::clipboard()->setText(m_pPattern->text());
    md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.copied",
                           tr("Pattern copied to the clipboard.")));
}

void UIMd3RegexBuilder::sltExportPattern()
{
    if (!m_pPattern || !m_pFlags)
        return;
    QString strPath = QFileDialog::getSaveFileName(this,
        md3RegexText("md3.regex.export-title", tr("Export regular expression")),
        QDir::homePath() + QStringLiteral("/regular-expression.json"),
        QStringLiteral("JSON (*.json)"));
    if (strPath.isEmpty())
        return;
    if (!strPath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
        strPath += QStringLiteral(".json");
    QJsonObject root;
    root.insert(QStringLiteral("schema"), 1);
    root.insert(QStringLiteral("engine"), QStringLiteral("Qt QRegularExpression / PCRE2-compatible"));
    root.insert(QStringLiteral("pattern"), m_pPattern->text());
    root.insert(QStringLiteral("flags"), m_pFlags->text());
    root.insert(QStringLiteral("sampleIncluded"), false);
    QSaveFile file(strPath);
    if (   !file.open(QIODevice::WriteOnly)
        || file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0
        || !file.commit())
    {
        md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.export-failed",
                               tr("Could not export the expression to %1.")).arg(strPath));
        return;
    }
    md3AnnounceRegexStatus(m_pStatus, md3RegexText("md3.regex.exported",
                           tr("Expression exported to %1.")).arg(strPath));
}

void UIMd3RegexBuilder::prepare()
{
    QVBoxLayout *pRootLayout = new QVBoxLayout(this);
    pRootLayout->setContentsMargins(8, 8, 8, 8);
    pRootLayout->setSpacing(8);
    QScrollArea *pScrollArea = new QScrollArea(this);
    pScrollArea->setObjectName(QStringLiteral("md3RegexBuilderScrollArea"));
    pScrollArea->setFrameShape(QFrame::NoFrame);
    pScrollArea->setWidgetResizable(true);
    pScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pScrollArea->setAccessibleName(md3RegexText("md3.regex.title", tr("Regex builder")));
    QWidget *pContent = new QWidget(pScrollArea);
    QVBoxLayout *pLayout = new QVBoxLayout(pContent);
    pLayout->setContentsMargins(16, 16, 16, 16);
    pLayout->setSpacing(10);
    pScrollArea->setWidget(pContent);
    pRootLayout->addWidget(pScrollArea, 1);

    QLabel *pDialect = new QLabel(md3RegexText("md3.regex.dialect",
        tr("Engine: Qt QRegularExpression (PCRE2-compatible). Backslash escapes follow PCRE2; supported flags are i, m, s, and x. Preview input stays local and is limited to 4,096 characters.")), this);
    pDialect->setWordWrap(true);
    pDialect->setAccessibleName(pDialect->text());
    pLayout->addWidget(pDialect);

    QGroupBox *pGuided = new QGroupBox(md3RegexText("md3.regex.guided",
                                                    tr("Guided construction")), this);
    QGridLayout *pGuideLayout = new QGridLayout(pGuided);
    pGuideLayout->setColumnStretch(1, 1);

    m_pLiteral = new QLineEdit(pGuided);
    m_pLiteral->setMaxLength(256);
    m_pLiteral->setPlaceholderText(md3RegexText("md3.regex.literal-placeholder",
                                                tr("Text to escape")));
    m_pLiteral->setAccessibleName(md3RegexText("md3.regex.literal", tr("Literal")));
    QPushButton *pInsertLiteral = new QPushButton(md3RegexText("md3.regex.insert-literal",
                                                               tr("Insert escaped literal")), pGuided);
    pGuideLayout->addWidget(new QLabel(md3RegexText("md3.regex.literal", tr("Literal")), pGuided), 0, 0);
    pGuideLayout->addWidget(m_pLiteral, 0, 1);
    pGuideLayout->addWidget(pInsertLiteral, 0, 2);

    QComboBox *pClasses = new QComboBox(pGuided);
    md3AddConstruct(pClasses, md3RegexText("md3.regex.class-digit", tr("Digit \\d")), QStringLiteral("\\d"));
    md3AddConstruct(pClasses, md3RegexText("md3.regex.class-word", tr("Word \\w")), QStringLiteral("\\w"));
    md3AddConstruct(pClasses, md3RegexText("md3.regex.class-space", tr("Whitespace \\s")), QStringLiteral("\\s"));
    md3AddConstruct(pClasses, md3RegexText("md3.regex.class-any", tr("Any character .")), QStringLiteral("."));
    md3AddConstruct(pClasses, md3RegexText("md3.regex.class-custom", tr("Custom class […]")), QStringLiteral("["), QStringLiteral("]"));
    QPushButton *pInsertClass = new QPushButton(md3RegexText("md3.regex.insert", tr("Insert")), pGuided);
    pGuideLayout->addWidget(new QLabel(md3RegexText("md3.regex.character-class", tr("Character class")), pGuided), 1, 0);
    pGuideLayout->addWidget(pClasses, 1, 1);
    pGuideLayout->addWidget(pInsertClass, 1, 2);

    QComboBox *pAnchors = new QComboBox(pGuided);
    md3AddConstruct(pAnchors, md3RegexText("md3.regex.anchor-start", tr("Start of subject ^")), QStringLiteral("^"));
    md3AddConstruct(pAnchors, md3RegexText("md3.regex.anchor-end", tr("End of subject $")), QStringLiteral("$"));
    md3AddConstruct(pAnchors, md3RegexText("md3.regex.anchor-word", tr("Word boundary \\b")), QStringLiteral("\\b"));
    md3AddConstruct(pAnchors, md3RegexText("md3.regex.anchor-absolute-start", tr("Absolute start \\A")), QStringLiteral("\\A"));
    md3AddConstruct(pAnchors, md3RegexText("md3.regex.anchor-absolute-end", tr("Absolute end \\z")), QStringLiteral("\\z"));
    QPushButton *pInsertAnchor = new QPushButton(md3RegexText("md3.regex.insert", tr("Insert")), pGuided);
    pGuideLayout->addWidget(new QLabel(md3RegexText("md3.regex.anchor", tr("Anchor")), pGuided), 2, 0);
    pGuideLayout->addWidget(pAnchors, 2, 1);
    pGuideLayout->addWidget(pInsertAnchor, 2, 2);

    QComboBox *pGroups = new QComboBox(pGuided);
    md3AddConstruct(pGroups, md3RegexText("md3.regex.group-capturing", tr("Capturing group (…)")), QStringLiteral("("), QStringLiteral(")"));
    md3AddConstruct(pGroups, md3RegexText("md3.regex.group-noncapturing", tr("Non-capturing group (?:…)")), QStringLiteral("(?:"), QStringLiteral(")"));
    md3AddConstruct(pGroups, md3RegexText("md3.regex.group-named", tr("Named group (?<name>…)")), QStringLiteral("(?<name>"), QStringLiteral(")"));
    md3AddConstruct(pGroups, md3RegexText("md3.regex.group-positive-lookahead", tr("Positive lookahead (?=…)")), QStringLiteral("(?="), QStringLiteral(")"));
    md3AddConstruct(pGroups, md3RegexText("md3.regex.group-negative-lookahead", tr("Negative lookahead (?!…)")), QStringLiteral("(?!"), QStringLiteral(")"));
    QPushButton *pInsertGroup = new QPushButton(md3RegexText("md3.regex.insert", tr("Insert")), pGuided);
    pGuideLayout->addWidget(new QLabel(md3RegexText("md3.regex.group", tr("Group")), pGuided), 3, 0);
    pGuideLayout->addWidget(pGroups, 3, 1);
    pGuideLayout->addWidget(pInsertGroup, 3, 2);

    QComboBox *pQuantifiers = new QComboBox(pGuided);
    pQuantifiers->addItem(md3RegexText("md3.regex.quantifier-optional", tr("Optional ?")), QStringLiteral("?"));
    pQuantifiers->addItem(md3RegexText("md3.regex.quantifier-zero-more", tr("Zero or more *")), QStringLiteral("*"));
    pQuantifiers->addItem(md3RegexText("md3.regex.quantifier-one-more", tr("One or more +")), QStringLiteral("+"));
    pQuantifiers->addItem(md3RegexText("md3.regex.quantifier-exact", tr("Exactly n {n}")), QStringLiteral("exact"));
    pQuantifiers->addItem(md3RegexText("md3.regex.quantifier-between", tr("Between n and m {n,m}")), QStringLiteral("range"));
    QSpinBox *pMinimumRepeats = new QSpinBox(pGuided);
    pMinimumRepeats->setRange(0, 9999);
    pMinimumRepeats->setValue(1);
    pMinimumRepeats->setAccessibleName(md3RegexText("md3.regex.minimum-repeats", tr("Minimum repeats")));
    QSpinBox *pMaximumRepeats = new QSpinBox(pGuided);
    pMaximumRepeats->setRange(0, 9999);
    pMaximumRepeats->setValue(3);
    pMaximumRepeats->setAccessibleName(md3RegexText("md3.regex.maximum-repeats", tr("Maximum repeats")));
    QHBoxLayout *pQuantifierLayout = new QHBoxLayout;
    pQuantifierLayout->setContentsMargins(0, 0, 0, 0);
    pQuantifierLayout->addWidget(pQuantifiers, 1);
    pQuantifierLayout->addWidget(pMinimumRepeats);
    pQuantifierLayout->addWidget(pMaximumRepeats);
    QPushButton *pInsertQuantifier = new QPushButton(md3RegexText("md3.regex.insert", tr("Insert")), pGuided);
    pGuideLayout->addWidget(new QLabel(md3RegexText("md3.regex.quantifier", tr("Quantifier")), pGuided), 4, 0);
    pGuideLayout->addLayout(pQuantifierLayout, 4, 1);
    pGuideLayout->addWidget(pInsertQuantifier, 4, 2);

    QPushButton *pAlternation = new QPushButton(md3RegexText("md3.regex.alternation",
                                                             tr("Insert alternation |")), pGuided);
    pGuideLayout->addWidget(pAlternation, 5, 1, 1, 2);
    pLayout->addWidget(pGuided);

    QFormLayout *pForm = new QFormLayout;
    m_pPattern = new QLineEdit(this);
    m_pPattern->setAccessibleName(md3RegexText("md3.regex.pattern",
                                               tr("Regular-expression pattern")));
    m_pPattern->setMaxLength(4096);
    pForm->addRow(md3RegexText("md3.regex.pattern", tr("Pattern")), m_pPattern);
    m_pFlags = new QLineEdit(this);
    m_pFlags->setAccessibleName(md3RegexText("md3.regex.flags",
                                             tr("Regular-expression flags")));
    m_pFlags->setMaxLength(16);
    m_pFlags->setPlaceholderText(md3RegexText("md3.regex.flags-placeholder", tr("i m s x")));
    pForm->addRow(md3RegexText("md3.regex.flags", tr("Flags")), m_pFlags);
    pLayout->addLayout(pForm);

    m_pSample = new QPlainTextEdit(this);
    m_pSample->setAccessibleName(md3RegexText("md3.regex.sample", tr("Regex sample text")));
    m_pSample->setPlaceholderText(md3RegexText("md3.regex.sample", tr("Sample text")));
    m_pSample->setMinimumHeight(88);
    pLayout->addWidget(m_pSample);

    m_pRegexMode = new QCheckBox(md3RegexText("md3.regex.use", tr("Use regex")), this);
    m_pRegexMode->setChecked(true);
    m_pRegexMode->setAccessibleName(m_pRegexMode->text());
    pLayout->addWidget(m_pRegexMode);
    m_pStatus = new QLabel(this);
    m_pStatus->setWordWrap(true);
    m_pStatus->setAccessibleName(md3RegexText("md3.regex.validation",
                                              tr("Regex validation status")));
    pLayout->addWidget(m_pStatus);

    m_pMatches = new QTreeWidget(this);
    m_pMatches->setObjectName(QStringLiteral("md3RegexCaptureTree"));
    m_pMatches->setAccessibleName(md3RegexText("md3.regex.matches",
                                               tr("Live matches and capture groups")));
    m_pMatches->setHeaderLabels(QStringList()
        << md3RegexText("md3.regex.column-group", tr("Match or group"))
        << md3RegexText("md3.regex.column-span", tr("Span"))
        << md3RegexText("md3.regex.column-text", tr("Text")));
    m_pMatches->setMinimumHeight(112);
    pLayout->addWidget(m_pMatches, 1);

    QDialogButtonBox *pButtons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    pButtons->button(QDialogButtonBox::Apply)->setText(md3RegexText("md3.regex.apply", tr("Apply regex")));
    pButtons->button(QDialogButtonBox::Cancel)->setText(md3RegexText("md3.regex.cancel", tr("Cancel")));
    QPushButton *pPlainText = pButtons->addButton(md3RegexText("md3.regex.use-plain",
                                                               tr("Use plain text")), QDialogButtonBox::ActionRole);
    QPushButton *pCopy = pButtons->addButton(md3RegexText("md3.regex.copy",
                                                          tr("Copy pattern")), QDialogButtonBox::ActionRole);
    QPushButton *pExport = pButtons->addButton(md3RegexText("md3.regex.export",
                                                            tr("Export JSON…")), QDialogButtonBox::ActionRole);
    pRootLayout->addWidget(pButtons);

    m_pLiteral->setMinimumHeight(48);
    pClasses->setMinimumHeight(48);
    pAnchors->setMinimumHeight(48);
    pGroups->setMinimumHeight(48);
    pQuantifiers->setMinimumHeight(48);
    pMinimumRepeats->setMinimumHeight(48);
    pMaximumRepeats->setMinimumHeight(48);
    m_pPattern->setMinimumHeight(48);
    m_pFlags->setMinimumHeight(48);
    m_pRegexMode->setMinimumHeight(48);
    for (QPushButton *pButton : findChildren<QPushButton*>())
        pButton->setMinimumHeight(48);

    m_pPreviewTimer = new QTimer(this);
    m_pPreviewTimer->setSingleShot(true);
    m_pPreviewTimer->setInterval(160);
    m_pPreviewTimeout = new QTimer(this);
    m_pPreviewTimeout->setSingleShot(true);

    const auto insertComboConstruct = [this](QComboBox *pCombo)
    {
        const QVariantList data = pCombo->currentData().toList();
        insertConstruct(data.value(0).toString(), data.value(1).toString());
    };
    connect(pInsertLiteral, &QPushButton::clicked, this, &UIMd3RegexBuilder::insertLiteral);
    connect(pInsertClass, &QPushButton::clicked, this, [insertComboConstruct, pClasses]() { insertComboConstruct(pClasses); });
    connect(pInsertAnchor, &QPushButton::clicked, this, [insertComboConstruct, pAnchors]() { insertComboConstruct(pAnchors); });
    connect(pInsertGroup, &QPushButton::clicked, this, [insertComboConstruct, pGroups]() { insertComboConstruct(pGroups); });
    const auto updateQuantifierControls = [pQuantifiers, pMinimumRepeats, pMaximumRepeats]()
    {
        const QString strKind = pQuantifiers->currentData().toString();
        pMinimumRepeats->setEnabled(strKind == QStringLiteral("exact") || strKind == QStringLiteral("range"));
        pMaximumRepeats->setEnabled(strKind == QStringLiteral("range"));
    };
    connect(pQuantifiers, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [updateQuantifierControls](int) { updateQuantifierControls(); });
    connect(pInsertQuantifier, &QPushButton::clicked, this,
            [this, pQuantifiers, pMinimumRepeats, pMaximumRepeats]()
    {
        const QString strKind = pQuantifiers->currentData().toString();
        if (strKind == QStringLiteral("exact"))
            insertSuffix(QStringLiteral("{%1}").arg(pMinimumRepeats->value()));
        else if (strKind == QStringLiteral("range"))
        {
            const int iMinimum = pMinimumRepeats->value();
            const int iMaximum = qMax(iMinimum, pMaximumRepeats->value());
            pMaximumRepeats->setValue(iMaximum);
            insertSuffix(QStringLiteral("{%1,%2}").arg(iMinimum).arg(iMaximum));
        }
        else
            insertSuffix(strKind);
    });
    connect(pAlternation, &QPushButton::clicked, this, [this]() { insertConstruct(QStringLiteral("|")); });
    connect(pButtons, &QDialogButtonBox::clicked, this, [this, pButtons](QAbstractButton *pButton)
    {
        if (pButton == pButtons->button(QDialogButtonBox::Apply))
            sltAcceptPattern();
        else if (pButton == pButtons->button(QDialogButtonBox::Cancel))
            reject();
    });
    connect(pPlainText, &QPushButton::clicked, this, &UIMd3RegexBuilder::sltUsePlainText);
    connect(pCopy, &QPushButton::clicked, this, &UIMd3RegexBuilder::sltCopyPattern);
    connect(pExport, &QPushButton::clicked, this, &UIMd3RegexBuilder::sltExportPattern);
    connect(m_pPreviewTimer, &QTimer::timeout, this, &UIMd3RegexBuilder::sltUpdatePreview);
    connect(m_pPreviewTimeout, &QTimer::timeout, this, &UIMd3RegexBuilder::sltPreviewTimedOut);
    connect(m_pPattern, &QLineEdit::textChanged, this, &UIMd3RegexBuilder::sltSchedulePreview);
    connect(m_pFlags, &QLineEdit::textChanged, this, &UIMd3RegexBuilder::sltSchedulePreview);
    connect(m_pSample, &QPlainTextEdit::textChanged, this, [this]()
    {
        const QString strText = m_pSample->toPlainText();
        if (strText.size() > g_cchRegexPreviewSample)
        {
            const QSignalBlocker blocker(m_pSample);
            QTextCursor cursor = m_pSample->textCursor();
            const int iPosition = qMin(cursor.position(), g_cchRegexPreviewSample);
            m_pSample->setPlainText(strText.left(g_cchRegexPreviewSample));
            cursor = m_pSample->textCursor();
            cursor.setPosition(iPosition);
            m_pSample->setTextCursor(cursor);
        }
        sltSchedulePreview();
    });
    connect(m_pRegexMode, &QCheckBox::toggled, this, &UIMd3RegexBuilder::sltSchedulePreview);
    updateQuantifierControls();
    sltUpdatePreview();
}
