/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3SearchField class implementation.
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
#include <QHBoxLayout>
#include <QPainter>

/* GUI includes: */
#include "UIMd3Button.h"
#include "UIMd3RegexBuilder.h"
#include "UIMd3SearchField.h"

UIMd3SearchField::UIMd3SearchField(const QString &strFieldId, const QString &strPlaceholder, QWidget *pParent /* = 0 */)
    : UIMd3Widget(pParent, QString("search/%1").arg(strFieldId))
    , m_strFieldId(strFieldId)
    , m_strPlaceholder(strPlaceholder)
    , m_pEditor(0)
    , m_pBuilderButton(0)
    , m_fRegexActive(false)
{
    prepare();
}

void UIMd3SearchField::prepare()
{
    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(12, 0, 4, 0);
    pLayout->setSpacing(8);

    m_pEditor = new QLineEdit(this);
    m_pEditor->setFrame(false);
    m_pEditor->setPlaceholderText(m_strPlaceholder);
    m_pEditor->setClearButtonEnabled(true);
    m_pEditor->setStyleSheet("background: transparent; border: 0;");
    connect(m_pEditor, &QLineEdit::textChanged, this, &UIMd3SearchField::sltTextEdited);
    pLayout->addWidget(m_pEditor, 1);

    m_pBuilderButton = new UIMd3Button(".*", UIMd3ButtonVariant_Tonal, this);
    m_pBuilderButton->setToolTip(tr("Open the regex builder for this field"));
    m_pBuilderButton->setFixedSize(38, 34);
    connect(m_pBuilderButton, &UIMd3Button::sigClicked, this, &UIMd3SearchField::sltOpenBuilder);
    pLayout->addWidget(m_pBuilderButton, 0);

    setFixedHeight(md3Theme().controlHeight());
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

bool UIMd3SearchField::matches(const QString &strCandidate) const
{
    const QString strQuery = text();
    if (strQuery.isEmpty() && !m_fRegexActive)
        return true;
    if (m_fRegexActive && m_regex.isValid())
        return m_regex.match(strCandidate).hasMatch();
    return strCandidate.contains(strQuery, Qt::CaseInsensitive);
}

bool UIMd3SearchField::applyRegex(const QString &strPattern, const QString &strFlags)
{
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (strFlags.contains('i'))
        options |= QRegularExpression::CaseInsensitiveOption;
    if (strFlags.contains('m'))
        options |= QRegularExpression::MultilineOption;
    if (strFlags.contains('s'))
        options |= QRegularExpression::DotMatchesEverythingOption;
    if (strFlags.contains('x'))
        options |= QRegularExpression::ExtendedPatternSyntaxOption;

    const QRegularExpression candidate(strPattern, options);
    if (!candidate.isValid())
        return false;

    m_regex = candidate;
    m_fRegexActive = true;
    emit sigFilterChanged();
    update();
    return true;
}

void UIMd3SearchField::clearRegex()
{
    m_fRegexActive = false;
    m_regex = QRegularExpression();
    emit sigFilterChanged();
    update();
}

void UIMd3SearchField::sltOpenBuilder()
{
    UIMd3RegexBuilder::editField(this, m_strFieldId);
}

void UIMd3SearchField::sltTextEdited()
{
    emit sigFilterChanged();
}
