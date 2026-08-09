/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3RegexBuilder class implementation.
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
#include <QElapsedTimer>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3Button.h"
#include "UIMd3RegexBuilder.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

/* The guided constructs offered on every field, in the ECMAScript-compatible
 * subset Qt's QRegularExpression accepts. */
static const struct { const char *pszLabel; const char *pszToken; } g_aConstructs[] =
{
    { "Any character", "."      }, { "Digit",       "\\d"   }, { "Word character", "\\w"  },
    { "Whitespace",    "\\s"  }, { "Line start",  "^"       }, { "Line end",       "$"      },
    { "Group",         "()"     }, { "Class",       "[]"      }, { "Alternation",    "|"      },
    { "Zero or more",  "*"      }, { "One or more", "+"       }, { "Optional",       "?"      },
    { "Range",         "{1,3}"  }, { "Word bound",  "\\b"   }, { "Named group",    "(?<name>)" },
    { "Lookahead",     "(?=)"   }, { "Negative lookahead", "(?!)" }
};

void UIMd3RegexBuilder::editField(UIMd3SearchField *pField, const QString &strFieldId)
{
    AssertPtrReturnVoid(pField);
    UIMd3RegexBuilder builder(strFieldId, pField->text(), "iu", pField);
    if (builder.exec() != QDialog::Accepted)
        return;
    if (builder.isCleared())
    {
        pField->clearRegex();
        return;
    }
    if (!pField->applyRegex(builder.pattern(), builder.flags()))
        pField->clearRegex();
}

UIMd3RegexBuilder::UIMd3RegexBuilder(const QString &strFieldId, const QString &strInitialPattern,
                                     const QString &strInitialFlags, QWidget *pParent /* = 0 */)
    : QDialog(pParent)
    , m_strFieldId(strFieldId)
    , m_strInitialPattern(strInitialPattern)
    , m_strInitialFlags(strInitialFlags)
    , m_pPatternEditor(0)
    , m_pFlagsEditor(0)
    , m_pSampleEditor(0)
    , m_pFeedback(0)
    , m_pPreview(0)
    , m_pApplyButton(0)
    , m_fCleared(false)
{
    prepare();
}

void UIMd3RegexBuilder::prepare()
{
    setWindowTitle(tr("Regex builder"));
    resize(620, 520);

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(24, 22, 24, 22);
    pLayout->setSpacing(12);

    QLabel *pTarget = new QLabel(tr("Target field: %1").arg(m_strFieldId), this);
    pTarget->setStyleSheet(QString("color: %1;").arg(md3(UIMd3ColorRole_Outline).name()));
    pLayout->addWidget(pTarget);

    m_pPatternEditor = new QLineEdit(m_strInitialPattern, this);
    m_pPatternEditor->setMaxLength(500);
    m_pPatternEditor->setPlaceholderText(tr("^Win\\d+"));
    connect(m_pPatternEditor, &QLineEdit::textChanged, this, &UIMd3RegexBuilder::sltRevalidate);
    pLayout->addWidget(new QLabel(tr("Pattern"), this));
    pLayout->addWidget(m_pPatternEditor);

    QGridLayout *pConstructs = new QGridLayout;
    prepareConstructs(pConstructs);
    pLayout->addLayout(pConstructs);

    QHBoxLayout *pFlagsLayout = new QHBoxLayout;
    m_pFlagsEditor = new QLineEdit(m_strInitialFlags, this);
    m_pFlagsEditor->setMaxLength(8);
    connect(m_pFlagsEditor, &QLineEdit::textChanged, this, &UIMd3RegexBuilder::sltRevalidate);
    pFlagsLayout->addWidget(new QLabel(tr("Flags"), this));
    pFlagsLayout->addWidget(m_pFlagsEditor, 1);
    pLayout->addLayout(pFlagsLayout);

    pLayout->addWidget(new QLabel(tr("Sample text"), this));
    m_pSampleEditor = new QPlainTextEdit(tr("Win11-Dev / Development"), this);
    m_pSampleEditor->setFixedHeight(70);
    connect(m_pSampleEditor, &QPlainTextEdit::textChanged, this, &UIMd3RegexBuilder::sltRevalidate);
    pLayout->addWidget(m_pSampleEditor);

    m_pFeedback = new QLabel(tr("Plain text stays the default. Applying a pattern switches this one field to regex."), this);
    m_pFeedback->setWordWrap(true);
    pLayout->addWidget(m_pFeedback);

    m_pPreview = new QLabel(this);
    m_pPreview->setWordWrap(true);
    m_pPreview->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pLayout->addWidget(m_pPreview, 1);

    QHBoxLayout *pButtons = new QHBoxLayout;
    UIMd3Button *pClear = new UIMd3Button(tr("Clear pattern"), UIMd3ButtonVariant_Text, this);
    connect(pClear, &UIMd3Button::sigClicked, this, &UIMd3RegexBuilder::sltClear);
    pButtons->addWidget(pClear);
    pButtons->addStretch(1);
    UIMd3Button *pCancel = new UIMd3Button(tr("Cancel"), UIMd3ButtonVariant_Text, this);
    connect(pCancel, &UIMd3Button::sigClicked, this, &QDialog::reject);
    pButtons->addWidget(pCancel);
    m_pApplyButton = new UIMd3Button(tr("Apply pattern"), UIMd3ButtonVariant_Filled, this);
    connect(m_pApplyButton, &UIMd3Button::sigClicked, this, &QDialog::accept);
    pButtons->addWidget(m_pApplyButton);
    pLayout->addLayout(pButtons);

    sltRevalidate();
}

void UIMd3RegexBuilder::prepareConstructs(QLayout *pLayout)
{
    QGridLayout *pGrid = qobject_cast<QGridLayout*>(pLayout);
    AssertPtrReturnVoid(pGrid);
    const int cColumns = 4;
    for (size_t i = 0; i < RT_ELEMENTS(g_aConstructs); ++i)
    {
        UIMd3Button *pChip = new UIMd3Button(tr(g_aConstructs[i].pszLabel), UIMd3ButtonVariant_Outlined, this);
        pChip->setProperty("token", QString::fromUtf8(g_aConstructs[i].pszToken));
        connect(pChip, &UIMd3Button::sigClicked, this, &UIMd3RegexBuilder::sltInsertConstruct);
        pGrid->addWidget(pChip, (int)i / cColumns, (int)i % cColumns);
    }
}

QString UIMd3RegexBuilder::pattern() const
{
    return m_pPatternEditor ? m_pPatternEditor->text() : QString();
}

QString UIMd3RegexBuilder::flags() const
{
    return m_pFlagsEditor ? m_pFlagsEditor->text() : QString("iu");
}

void UIMd3RegexBuilder::sltInsertConstruct()
{
    UIMd3Button *pSender = qobject_cast<UIMd3Button*>(sender());
    AssertPtrReturnVoid(pSender);
    AssertPtrReturnVoid(m_pPatternEditor);
    m_pPatternEditor->insert(pSender->property("token").toString());
    m_pPatternEditor->setFocus();
}

void UIMd3RegexBuilder::sltClear()
{
    m_fCleared = true;
    accept();
}

void UIMd3RegexBuilder::sltRevalidate()
{
    AssertPtrReturnVoid(m_pFeedback);
    AssertPtrReturnVoid(m_pPreview);

    const QString strPattern = pattern();
    if (strPattern.isEmpty())
    {
        m_pFeedback->setText(tr("Plain text stays the default. Applying a pattern switches this one field to regex."));
        m_pFeedback->setStyleSheet(QString("color: %1;").arg(md3(UIMd3ColorRole_OnSurfaceVariant).name()));
        m_pPreview->clear();
        if (m_pApplyButton)
            m_pApplyButton->setEnabledState(false);
        return;
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    const QString strFlags = flags();
    if (strFlags.contains('i'))
        options |= QRegularExpression::CaseInsensitiveOption;
    if (strFlags.contains('m'))
        options |= QRegularExpression::MultilineOption;
    if (strFlags.contains('s'))
        options |= QRegularExpression::DotMatchesEverythingOption;

    const QRegularExpression regex(strPattern, options);
    if (!regex.isValid())
    {
        m_pFeedback->setText(regex.errorString());
        m_pFeedback->setStyleSheet(QString("color: %1;").arg(md3(UIMd3ColorRole_Error).name()));
        m_pPreview->clear();
        if (m_pApplyButton)
            m_pApplyButton->setEnabledState(false);
        return;
    }

    /* Evaluate the sample under a hard 300 ms budget so a catastrophic pattern
     * cannot lock the GUI thread while the user is still typing it: */
    QElapsedTimer timer;
    timer.start();
    QStringList matches;
    QRegularExpressionMatchIterator it = regex.globalMatch(m_pSampleEditor->toPlainText());
    while (it.hasNext() && timer.elapsed() < 300)
        matches << it.next().captured(0);

    if (timer.elapsed() >= 300)
    {
        m_pFeedback->setText(tr("Evaluation exceeded the 300 ms budget; simplify the pattern before applying it."));
        m_pFeedback->setStyleSheet(QString("color: %1;").arg(md3(UIMd3ColorRole_Error).name()));
        if (m_pApplyButton)
            m_pApplyButton->setEnabledState(false);
        return;
    }

    m_pFeedback->setText(tr("Pattern is valid."));
    m_pFeedback->setStyleSheet(QString("color: %1;").arg(md3(UIMd3ColorRole_OnSurfaceVariant).name()));
    m_pPreview->setText(matches.isEmpty() ? tr("No match in the sample text.")
                                          : tr("%n match(es): %1", "", matches.size()).arg(matches.join(", ")));
    if (m_pApplyButton)
        m_pApplyButton->setEnabledState(true);
}
