/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 wizard shell implementation.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QPaintEvent>
#include <QStyle>
#include <QVBoxLayout>

#include "UIMd3Theme.h"
#include "UIMd3Tokens.h"
#include "UIMd3Wizard.h"

UIMd3Wizard::UIMd3Wizard(QWidget *pParent)
    : QWidget(pParent)
    , m_pPageTitle(0)
    , m_pStepSummary(0)
    , m_pLayout(0)
    , m_pContentLayout(0)
    , m_iCurrentStep(-1)
{
    setObjectName(QStringLiteral("md3WizardShell"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAccessibleName(tr("Wizard"));
    setAccessibleDescription(tr("Material 3 wizard with page progress and the active page content."));

    m_pLayout = new QVBoxLayout(this);
    const int iGutter = UIMd3Theme::instance() ? md3Theme().gutter() : 16;
    m_pLayout->setContentsMargins(iGutter, iGutter, iGutter, iGutter);
    m_pLayout->setSpacing(8);

    m_pPageTitle = new QLabel(this);
    m_pPageTitle->setWordWrap(true);
    m_pPageTitle->setAccessibleName(tr("Current wizard page"));
    if (UIMd3Theme::instance())
        m_pPageTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    m_pLayout->addWidget(m_pPageTitle);

    m_pStepSummary = new QLabel(this);
    m_pStepSummary->setWordWrap(true);
    m_pStepSummary->setAccessibleName(tr("Wizard progress"));
    if (UIMd3Theme::instance())
        m_pStepSummary->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
    m_pLayout->addWidget(m_pStepSummary);

    QWidget *pStepRow = new QWidget(this);
    pStepRow->setObjectName(QStringLiteral("md3WizardStepRow"));
    QHBoxLayout *pStepLayout = new QHBoxLayout(pStepRow);
    pStepLayout->setContentsMargins(0, 0, 0, 0);
    pStepLayout->setSpacing(6);
    m_pLayout->addWidget(pStepRow);

    m_pContentLayout = new QVBoxLayout;
    m_pContentLayout->setContentsMargins(0, 4, 0, 0);
    m_pContentLayout->setSpacing(0);
    m_pLayout->addLayout(m_pContentLayout, 1);

    if (UIMd3Theme::instance())
        connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
                this, &UIMd3Wizard::sltThemeChanged);
}

void UIMd3Wizard::setContentWidget(QWidget *pWidget)
{
    if (!pWidget || !m_pContentLayout)
        return;
    pWidget->setParent(this);
    m_pContentLayout->addWidget(pWidget, 1);
}

void UIMd3Wizard::clearSteps()
{
    QWidget *pStepRow = findChild<QWidget *>(QStringLiteral("md3WizardStepRow"));
    if (!pStepRow)
        return;
    QLayout *pLayout = pStepRow->layout();
    if (!pLayout)
        return;
    while (QLayoutItem *pItem = pLayout->takeAt(0))
    {
        if (pItem->widget())
            delete pItem->widget();
        delete pItem;
    }
    m_steps.clear();
}

void UIMd3Wizard::setStepTitles(const QStringList &titles)
{
    m_stepTitles = titles;
    clearSteps();
    QWidget *pStepRow = findChild<QWidget *>(QStringLiteral("md3WizardStepRow"));
    QHBoxLayout *pStepLayout = pStepRow ? qobject_cast<QHBoxLayout *>(pStepRow->layout()) : 0;
    if (!pStepLayout)
        return;

    for (int i = 0; i < m_stepTitles.size(); ++i)
    {
        QLabel *pStep = new QLabel(this);
        pStep->setWordWrap(true);
        pStep->setAlignment(Qt::AlignCenter);
        pStep->setMinimumHeight(UIMd3Theme::instance() ? md3Theme().controlHeight() : 40);
        pStep->setText(m_stepTitles.at(i));
        pStep->setAccessibleName(tr("Wizard step %1: %2").arg(i + 1).arg(m_stepTitles.at(i)));
        m_steps << pStep;
        pStepLayout->addWidget(pStep, 1);
        if (i + 1 < m_stepTitles.size())
        {
            QLabel *pDivider = new QLabel(QStringLiteral("•"), this);
            pDivider->setAccessibleName(tr("Step separator"));
            pDivider->setEnabled(false);
            pStepLayout->addWidget(pDivider, 0, Qt::AlignCenter);
        }
    }
    setCurrentStep(m_iCurrentStep);
}

void UIMd3Wizard::setCurrentStep(int iIndex)
{
    m_iCurrentStep = iIndex;
    const int cSteps = m_stepTitles.size();
    if (m_pStepSummary)
        m_pStepSummary->setText(cSteps > 0 && iIndex >= 0 && iIndex < cSteps
                                ? tr("Step %1 of %2").arg(iIndex + 1).arg(cSteps)
                                : QString());
    for (int i = 0; i < m_steps.size(); ++i)
    {
        QLabel *pStep = m_steps.at(i);
        const bool fCurrent = i == iIndex;
        QPalette pal = pStep->palette();
        if (UIMd3Theme::instance())
        {
            pal.setColor(QPalette::Window, fCurrent ? md3Theme().color(UIMd3ColorRole_SecondaryContainer)
                                                     : md3Theme().color(UIMd3ColorRole_SurfaceContainerHigh));
            pal.setColor(QPalette::WindowText, fCurrent ? md3Theme().color(UIMd3ColorRole_OnSecondaryContainer)
                                                         : md3Theme().color(UIMd3ColorRole_OnSurfaceVariant));
        }
        pStep->setAutoFillBackground(true);
        pStep->setPalette(pal);
        pStep->setFont(UIMd3Theme::instance()
                       ? md3Theme().font(fCurrent ? UIMd3TypeRole_LabelLarge : UIMd3TypeRole_LabelMedium)
                       : font());
        pStep->setToolTip(m_stepTitles.value(i));
    }
    update();
}

void UIMd3Wizard::setStepComplete(int iIndex, bool fComplete)
{
    if (iIndex < 0 || iIndex >= m_steps.size())
        return;
    QLabel *pStep = m_steps.at(iIndex);
    const QString strTitle = m_stepTitles.value(iIndex);
    pStep->setText(fComplete && iIndex != m_iCurrentStep
                   ? QStringLiteral("✓ ") + strTitle : strTitle);
    pStep->setAccessibleName(tr("Wizard step %1: %2%3")
                             .arg(iIndex + 1)
                             .arg(strTitle)
                             .arg(fComplete ? tr(", complete") : QString()));
}

void UIMd3Wizard::sltThemeChanged()
{
    if (m_pLayout)
    {
        const int iGutter = UIMd3Theme::instance() ? md3Theme().gutter() : 16;
        m_pLayout->setContentsMargins(iGutter, iGutter, iGutter, iGutter);
    }
    if (m_pPageTitle && UIMd3Theme::instance())
        m_pPageTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    if (m_pStepSummary && UIMd3Theme::instance())
        m_pStepSummary->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
    setCurrentStep(m_iCurrentStep);
}

void UIMd3Wizard::paintEvent(QPaintEvent *pEvent)
{
    QPainter painter(this);
    painter.setClipRect(pEvent->rect());
    const QColor background = UIMd3Theme::instance()
                            ? md3Theme().color(UIMd3ColorRole_SurfaceContainerLow)
                            : palette().color(QPalette::Window);
    painter.fillRect(rect(), background);
    painter.setPen(UIMd3Theme::instance()
                   ? md3Theme().color(UIMd3ColorRole_OutlineVariant)
                   : palette().color(QPalette::Mid));
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), UIMd3Shape::Large, UIMd3Shape::Large);
    QWidget::paintEvent(pEvent);
}
