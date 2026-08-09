/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 search field with an anchored regex builder.
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
#include <QApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollArea>
#include <QScreen>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3RegexBuilder.h"
#include "UIMd3SearchField.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"

static QString md3SearchText(const QString &strKey, const QString &strFallback)
{
    if (!UIMd3Language::instance())
        return strFallback;
    const QString strText = md3Text(strKey);
    return strText == strKey || strText.isEmpty() ? strFallback : strText;
}

static bool md3NormalizeRegexFlags(const QString &strFlags, QString &strResult)
{
    strResult.clear();
    if (strFlags.size() > 16)
        return false;
    const QString strSupported = QStringLiteral("imsx");
    for (const QChar ch : strFlags.toLower())
    {
        if (ch.isSpace())
            continue;
        if (!strSupported.contains(ch) || strResult.contains(ch))
            return false;
        strResult += ch;
    }
    return true;
}

UIMd3SearchField::UIMd3SearchField(const QString &strFieldId, const QString &strPlaceholder, QWidget *pParent)
    : QWidget(pParent)
    , m_strFieldId(strFieldId)
    , m_strPlaceholder(strPlaceholder)
    , m_pEditor(0)
    , m_pBuilderButton(0)
    , m_pLayout(0)
    , m_pBuilderScrollArea(0)
    , m_pBuilder(0)
    , m_fRegexActive(false)
{
    setObjectName(QStringLiteral("md3SearchField_%1").arg(strFieldId));
    setAccessibleName(strPlaceholder);
    prepare();
}

QString UIMd3SearchField::text() const
{
    return m_pEditor ? m_pEditor->text() : QString();
}

void UIMd3SearchField::setText(const QString &strText)
{
    if (m_pEditor)
        m_pEditor->setText(strText);
}

void UIMd3SearchField::setPlaceholderText(const QString &strText)
{
    m_strPlaceholder = strText;
    if (m_pEditor)
    {
        m_pEditor->setPlaceholderText(strText);
        m_pEditor->setAccessibleName(strText);
        setAccessibleName(strText);
    }
    updateRegexPresentation();
}

QRegularExpression::PatternOptions UIMd3SearchField::patternOptions(const QString &strFlags)
{
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    const QString flags = strFlags.toLower();
    if (flags.contains('i')) options |= QRegularExpression::CaseInsensitiveOption;
    if (flags.contains('m')) options |= QRegularExpression::MultilineOption;
    if (flags.contains('s')) options |= QRegularExpression::DotMatchesEverythingOption;
    if (flags.contains('x')) options |= QRegularExpression::ExtendedPatternSyntaxOption;
    return options;
}

bool UIMd3SearchField::applyRegex(const QString &strPattern, const QString &strFlags)
{
    if (strPattern.size() > 4096)
        return false;
    QString strNormalizedFlags;
    if (!md3NormalizeRegexFlags(strFlags, strNormalizedFlags))
        return false;
    const QRegularExpression candidate(strPattern, patternOptions(strNormalizedFlags));
    if (!candidate.isValid())
        return false;
    m_regex = candidate;
    m_strFlags = strNormalizedFlags;
    m_fRegexActive = true;
    if (m_pEditor && m_pEditor->text() != strPattern)
    {
        const QSignalBlocker blocker(m_pEditor);
        m_pEditor->setText(strPattern);
    }
    updateRegexPresentation();
    emit sigFilterChanged();
    return true;
}

void UIMd3SearchField::clearRegex()
{
    m_fRegexActive = false;
    m_strFlags.clear();
    m_regex = QRegularExpression();
    updateRegexPresentation();
    emit sigFilterChanged();
}

void UIMd3SearchField::updateRegexPresentation()
{
    if (!m_pBuilderButton)
        return;
    const QString strBuilderName = md3SearchText(QStringLiteral("md3.search.regex-builder-name"),
                                                  tr("Open regex builder"));
    m_pBuilderButton->setAccessibleName(strBuilderName);
    m_pBuilderButton->setChecked(m_fRegexActive);
    m_pBuilderButton->setText(m_fRegexActive && !m_strFlags.isEmpty()
                            ? QStringLiteral(".* %1").arg(m_strFlags)
                            : QStringLiteral(".*"));
    if (m_fRegexActive)
    {
        const QString strDescription = m_strFlags.isEmpty()
            ? md3SearchText(QStringLiteral("md3.search.regex-active-no-flags"),
                            tr("Regular-expression search is active without flags."))
            : md3SearchText(QStringLiteral("md3.search.regex-active"),
                            tr("Regular-expression search is active with flags %1.")).arg(m_strFlags);
        m_pBuilderButton->setToolTip(strDescription);
        m_pBuilderButton->setAccessibleDescription(strDescription);
        setAccessibleDescription(strDescription);
    }
    else
    {
        m_pBuilderButton->setToolTip(md3SearchText(QStringLiteral("md3.search.regex-builder"),
                                                    tr("Open the regex builder for this search")));
        m_pBuilderButton->setAccessibleDescription(QString());
        setAccessibleDescription(QString());
    }
}

bool UIMd3SearchField::matches(const QString &strCandidate) const
{
    if (m_fRegexActive && m_regex.isValid())
        return m_regex.match(strCandidate).hasMatch();
    return text().isEmpty() || strCandidate.contains(text(), Qt::CaseInsensitive);
}

void UIMd3SearchField::sltOpenBuilder()
{
    if (m_pBuilder)
    {
        if (!m_pBuilder->isVisible())
            m_pBuilder->setPattern(m_fRegexActive ? m_regex.pattern() : text(), m_strFlags);
        m_pBuilder->show();
        if (m_pBuilderScrollArea)
            m_pBuilderScrollArea->show();
        if (m_pBuilder->isWindow())
        {
            m_pBuilder->raise();
            m_pBuilder->activateWindow();
        }
        else
        {
            adjustSize();
            if (QMenu *pMenu = qobject_cast<QMenu*>(window()))
                pMenu->adjustSize();
        }
        return;
    }

    QMenu *pHostMenu = 0;
    for (QWidget *pAncestor = parentWidget(); pAncestor && !pHostMenu; pAncestor = pAncestor->parentWidget())
        pHostMenu = qobject_cast<QMenu*>(pAncestor);

    UIMd3RegexBuilder *pBuilder = new UIMd3RegexBuilder(this);
    if (!pBuilder)
        return;
    m_pBuilder = pBuilder;
    if (pHostMenu)
    {
        /* A popup menu owns its widget actions only for the duration of exec().
         * Keep the complete builder inside the same action so interacting with
         * it does not close the menu and destroy a stack-owned popup window. */
        pBuilder->setWindowFlags(Qt::Widget);
        pBuilder->setAttribute(Qt::WA_DeleteOnClose, false);
        pBuilder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
    pBuilder->setPattern(m_fRegexActive ? m_regex.pattern() : text(), m_strFlags);
    connect(pBuilder, &UIMd3RegexBuilder::sigPatternAccepted,
            this, [this](const QString &strPattern, const QString &strFlags)
    {
        applyRegex(strPattern, strFlags);
    });
    connect(pBuilder, &UIMd3RegexBuilder::sigPlainTextRequested,
            this, &UIMd3SearchField::clearRegex);
    connect(pBuilder, &QObject::destroyed, this, [this]()
    {
        if (m_pEditor)
            m_pEditor->setFocus(Qt::OtherFocusReason);
    });
    connect(pBuilder, &QDialog::finished, this, [this, pHostMenu](int)
    {
        if (m_pEditor)
            m_pEditor->setFocus(Qt::OtherFocusReason);
        if (pHostMenu)
        {
            if (m_pBuilderScrollArea)
                m_pBuilderScrollArea->hide();
            adjustSize();
            pHostMenu->adjustSize();
        }
    });
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                pBuilder, [this]()
    {
        if (m_pBuilderScrollArea)
        {
            m_pBuilderScrollArea->hide();
            m_pBuilderScrollArea->deleteLater();
            m_pBuilderScrollArea = 0;
            m_pBuilder = 0;
        }
        else if (m_pBuilder)
            m_pBuilder->close();
        if (m_pEditor)
            m_pEditor->setFocus(Qt::OtherFocusReason);
    });
    pBuilder->adjustSize();
    if (pHostMenu)
    {
        const QPoint point = mapToGlobal(QPoint(0, height()));
        QScreen *pScreen = pHostMenu->screen();
        if (!pScreen)
            pScreen = QApplication::screenAt(point);
        const QRect available = pScreen ? pScreen->availableGeometry()
                                        : QRect(0, 0, 1280, 720);
        const int iMaximumWidth = qMax(240, available.width() - 24);
        const int iMaximumHeight = qMax(180, available.height() - 96);
        pBuilder->setMinimumSize(pBuilder->sizeHint());
        m_pBuilderScrollArea = new QScrollArea(this);
        m_pBuilderScrollArea->setObjectName(QStringLiteral("md3InlineRegexBuilderScrollArea"));
        m_pBuilderScrollArea->setFrameShape(QFrame::NoFrame);
        m_pBuilderScrollArea->setWidgetResizable(true);
        m_pBuilderScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_pBuilderScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_pBuilderScrollArea->setMaximumSize(iMaximumWidth, iMaximumHeight);
        m_pBuilderScrollArea->setWidget(pBuilder);
        if (m_pLayout)
            m_pLayout->addWidget(m_pBuilderScrollArea);
        setMaximumWidth(iMaximumWidth);
        pBuilder->show();
        adjustSize();
        pHostMenu->adjustSize();
        return;
    }
    const QPoint point = m_pBuilderButton ? m_pBuilderButton->mapToGlobal(QPoint(0, m_pBuilderButton->height()))
                                           : mapToGlobal(QPoint(0, height()));
    QScreen *pScreen = window() ? window()->screen() : 0;
    if (!pScreen)
        pScreen = QApplication::screenAt(point);
    const QRect available = pScreen ? pScreen->availableGeometry()
                                    : (QApplication::primaryScreen()
                                     ? QApplication::primaryScreen()->availableGeometry()
                                     : QRect(0, 0, 1280, 720));
    QSize size = pBuilder->size();
    size.setWidth(qMin(size.width(), available.width()));
    size.setHeight(qMin(size.height(), available.height()));
    pBuilder->resize(size);
    int iX = point.x();
    int iY = point.y();
    if (iX + size.width() > available.right() + 1)
        iX = point.x() - size.width();
    if (iY + size.height() > available.bottom() + 1)
        iY = point.y() - size.height() - (m_pBuilderButton ? m_pBuilderButton->height() : height());
    iX = qBound(available.left(), iX, available.right() - size.width() + 1);
    iY = qBound(available.top(), iY, available.bottom() - size.height() + 1);
    pBuilder->move(iX, iY);
    pBuilder->show();
}

void UIMd3SearchField::sltTextChanged(const QString &)
{
    if (m_fRegexActive)
        clearRegex();
    else
        emit sigFilterChanged();
}

void UIMd3SearchField::prepare()
{
    m_pLayout = new QVBoxLayout(this);
    m_pLayout->setContentsMargins(0, 0, 0, 0);
    m_pLayout->setSpacing(4);
    QHBoxLayout *pLayout = new QHBoxLayout;
    pLayout->setContentsMargins(12, 4, 4, 4);
    pLayout->setSpacing(6);
    m_pEditor = new QLineEdit(this);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(m_pEditor);
    m_pEditor->setClearButtonEnabled(true);
    m_pEditor->setMaxLength(4096);
    m_pEditor->setPlaceholderText(m_strPlaceholder);
    m_pEditor->setAccessibleName(m_strPlaceholder);
    pLayout->addWidget(m_pEditor, 1);
    m_pBuilderButton = new QToolButton(this);
    m_pBuilderButton->setText(QStringLiteral(".*"));
    m_pBuilderButton->setCheckable(true);
    m_pBuilderButton->setToolTip(md3SearchText(QStringLiteral("md3.search.regex-builder"),
                                                tr("Open the regex builder for this search")));
    m_pBuilderButton->setAccessibleName(md3SearchText(QStringLiteral("md3.search.regex-builder-name"),
                                                       tr("Open regex builder")));
    m_pBuilderButton->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    pLayout->addWidget(m_pBuilderButton);
    m_pLayout->addLayout(pLayout);
    setMinimumHeight(md3Theme().controlHeight() + 8);
    connect(m_pEditor, &QLineEdit::textChanged, this, &UIMd3SearchField::sltTextChanged);
    connect(m_pBuilderButton, &QToolButton::clicked, this, &UIMd3SearchField::sltOpenBuilder);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged, this, [this]()
    {
        if (m_pBuilderButton)
            m_pBuilderButton->setMinimumHeight(md3Theme().controlHeight());
    });
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, [this]()
    {
        updateRegexPresentation();
    });
    updateRegexPresentation();
}
