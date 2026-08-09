/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3DestructiveGate class implementation.
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
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3Button.h"
#include "UIMd3DestructiveGate.h"
#include "UIMd3History.h"
#include "UIMd3Theme.h"

bool UIMd3DestructiveGate::authorize(QWidget *pParent, const QString &strTitle, const QString &strConsequence)
{
    UIMd3DestructiveGate gate(strTitle, strConsequence, pParent);
    const bool fAuthorized = gate.exec() == QDialog::Accepted;
    if (fAuthorized)
        UIMd3History::instance()->record("destructive action authorized", strTitle);
    return fAuthorized;
}

UIMd3DestructiveGate::UIMd3DestructiveGate(const QString &strTitle, const QString &strConsequence, QWidget *pParent /* = 0 */)
    : QDialog(pParent)
    , m_strTitle(strTitle)
    , m_strConsequence(strConsequence)
    , m_pLeftKey(0)
    , m_pRightKey(0)
    , m_pSlider(0)
    , m_pAuthorizeButton(0)
    , m_fLeftHeld(false)
    , m_fRightHeld(false)
{
    prepare();
}

void UIMd3DestructiveGate::prepare()
{
    setWindowTitle(m_strTitle);
    resize(560, 380);

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(24, 22, 24, 22);
    pLayout->setSpacing(14);

    QLabel *pTitle = new QLabel(m_strTitle, this);
    pTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pLayout->addWidget(pTitle);

    QLabel *pConsequence = new QLabel(m_strConsequence, this);
    pConsequence->setWordWrap(true);
    pConsequence->setStyleSheet(QString("background: %1; color: %2; padding: 12px; border-radius: 12px;")
                                .arg(md3(UIMd3ColorRole_ErrorContainer).name(), md3(UIMd3ColorRole_OnErrorContainer).name()));
    pLayout->addWidget(pConsequence);

    QHBoxLayout *pKeys = new QHBoxLayout;
    m_pLeftKey = new UIMd3Button(tr("Hold key A"), UIMd3ButtonVariant_Outlined, this);
    m_pRightKey = new UIMd3Button(tr("Hold key L"), UIMd3ButtonVariant_Outlined, this);
    m_pLeftKey->setMinimumHeight(54);
    m_pRightKey->setMinimumHeight(54);
    connect(m_pLeftKey, &UIMd3Button::sigClicked, this, &UIMd3DestructiveGate::sltKeyToggled);
    connect(m_pRightKey, &UIMd3Button::sigClicked, this, &UIMd3DestructiveGate::sltKeyToggled);
    pKeys->addWidget(m_pLeftKey);
    pKeys->addWidget(m_pRightKey);
    pLayout->addLayout(pKeys);

    m_pSlider = new QSlider(Qt::Horizontal, this);
    m_pSlider->setRange(0, 100);
    m_pSlider->setEnabled(false);
    connect(m_pSlider, &QSlider::valueChanged, this, &UIMd3DestructiveGate::sltSliderMoved);
    pLayout->addWidget(m_pSlider);

    pLayout->addStretch(1);

    QHBoxLayout *pButtons = new QHBoxLayout;
    UIMd3Button *pExit = new UIMd3Button(tr("Emergency exit"), UIMd3ButtonVariant_Text, this);
    connect(pExit, &UIMd3Button::sigClicked, this, &QDialog::reject);
    pButtons->addWidget(pExit);
    pButtons->addStretch(1);
    m_pAuthorizeButton = new UIMd3Button(m_strTitle, UIMd3ButtonVariant_Danger, this);
    m_pAuthorizeButton->setEnabledState(false);
    connect(m_pAuthorizeButton, &UIMd3Button::sigClicked, this, &QDialog::accept);
    pButtons->addWidget(m_pAuthorizeButton);
    pLayout->addLayout(pButtons);
}

void UIMd3DestructiveGate::sltKeyToggled()
{
    if (sender() == m_pLeftKey)
        m_fLeftHeld = !m_fLeftHeld;
    else if (sender() == m_pRightKey)
        m_fRightHeld = !m_fRightHeld;

    m_pLeftKey->setVariant(m_fLeftHeld ? UIMd3ButtonVariant_Tonal : UIMd3ButtonVariant_Outlined);
    m_pRightKey->setVariant(m_fRightHeld ? UIMd3ButtonVariant_Tonal : UIMd3ButtonVariant_Outlined);

    const bool fBoth = m_fLeftHeld && m_fRightHeld;
    m_pSlider->setEnabled(fBoth);
    if (!fBoth)
        m_pSlider->setValue(0);
    reevaluate();
}

void UIMd3DestructiveGate::sltSliderMoved(int)
{
    reevaluate();
}

void UIMd3DestructiveGate::reevaluate()
{
    const bool fArmed = m_fLeftHeld && m_fRightHeld && m_pSlider->value() >= m_pSlider->maximum();
    m_pAuthorizeButton->setEnabledState(fArmed);
}
