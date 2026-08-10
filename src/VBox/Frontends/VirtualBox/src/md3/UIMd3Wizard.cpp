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

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPalette>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>

#include "UIMd3Language.h"
#include "UIMd3Theme.h"
#include "UIMd3Tokens.h"
#include "UIMd3Wizard.h"

namespace
{
    /** Width below which the page card keeps only the compact progress summary. */
    const int g_iWizardCompactWidth = 720;

    /** Registers text owned by the shared wizard presentation shell. */
    void registerMd3WizardTexts()
    {
        UIMd3Language *pLanguage = UIMd3Language::instance();
        if (!pLanguage)
            return;
#define REGISTER_WIZARD_TEXT(a_pszKey, a_pszEnglish, a_pszCantonese) \
        pLanguage->registerText(QStringLiteral(a_pszKey), QStringLiteral(a_pszEnglish), QStringLiteral(a_pszCantonese))
        REGISTER_WIZARD_TEXT("md3.wizard.name", "Wizard", "精靈");
        REGISTER_WIZARD_TEXT("md3.wizard.description", "Wizard with page progress and active page content.", "有頁面進度同目前內容嘅精靈。");
        REGISTER_WIZARD_TEXT("md3.wizard.current-page", "Current wizard page", "目前精靈頁面");
        REGISTER_WIZARD_TEXT("md3.wizard.current-page-description", "Title of the active wizard page.", "目前精靈頁面嘅標題。");
        REGISTER_WIZARD_TEXT("md3.wizard.progress", "Wizard progress", "精靈進度");
        REGISTER_WIZARD_TEXT("md3.wizard.progress-description", "Progress through the wizard steps.", "精靈各步驟嘅進度。");
        REGISTER_WIZARD_TEXT("md3.wizard.progress-current", "Step %1 of %2", "第 %1 步，共 %2 步");
        REGISTER_WIZARD_TEXT("md3.wizard.progress-unavailable", "Wizard progress is unavailable.", "精靈進度暫時未有。");
        REGISTER_WIZARD_TEXT("md3.wizard.progress-current-description", "Current wizard progress: %1", "目前精靈進度：%1");
        REGISTER_WIZARD_TEXT("md3.wizard.step-name", "Wizard step %1: %2", "精靈第 %1 步：%2");
        REGISTER_WIZARD_TEXT("md3.wizard.step-position", "Wizard step %1 of %2. %3.", "精靈第 %1 步，共 %2 步。%3。");
        REGISTER_WIZARD_TEXT("md3.wizard.step-current", "Current step", "目前步驟");
        REGISTER_WIZARD_TEXT("md3.wizard.step-complete", "Completed step", "已完成步驟");
        REGISTER_WIZARD_TEXT("md3.wizard.step-upcoming", "Upcoming step", "稍後步驟");
        REGISTER_WIZARD_TEXT("md3.wizard.step-complete-suffix", ", complete", "，已完成");
#undef REGISTER_WIZARD_TEXT
    }

    /** Resolves shell text with a safe Qt fallback for early construction. */
    QString wizardText(const char *pszKey, const QString &strFallback)
    {
        if (!UIMd3Language::instance())
            return strFallback;
        const QString strKey = QString::fromLatin1(pszKey);
        const QString strText = md3Text(strKey);
        return strText == strKey || strText.isEmpty() ? strFallback : strText;
    }

    /** Rounded Material card used for the step rail and page content. */
    class UIMd3WizardSurface : public QWidget
    {
    public:

        UIMd3WizardSurface(UIMd3ColorRole enmBackgroundRole,
                          UIMd3ColorRole enmOutlineRole,
                          QWidget *pParent)
            : QWidget(pParent)
            , m_enmBackgroundRole(enmBackgroundRole)
            , m_enmOutlineRole(enmOutlineRole)
        {
            setAttribute(Qt::WA_StyledBackground, true);
        }

    protected:

        virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setClipRect(pEvent->rect());
            const QColor background = UIMd3Theme::instance()
                                      ? md3Theme().color(m_enmBackgroundRole)
                                      : palette().color(QPalette::Window);
            const QColor outline = UIMd3Theme::instance()
                                   ? md3Theme().color(m_enmOutlineRole)
                                   : palette().color(QPalette::Mid);
            painter.setPen(outline);
            painter.setBrush(background);
            painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1),
                                    UIMd3Shape::Large, UIMd3Shape::Large);
            QWidget::paintEvent(pEvent);
        }

    private:

        const UIMd3ColorRole m_enmBackgroundRole;
        const UIMd3ColorRole m_enmOutlineRole;
    };

    /** Eliding, token-colored step chip retaining its full accessible name. */
    class UIMd3WizardStepLabel : public QLabel
    {
    public:

        explicit UIMd3WizardStepLabel(QWidget *pParent)
            : QLabel(pParent)
        {
            setAutoFillBackground(false);
            setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }

    protected:

        virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setClipRect(pEvent->rect());
            painter.setPen(Qt::NoPen);
            painter.setBrush(palette().color(QPalette::Window));
            painter.drawRoundedRect(rect(), UIMd3Shape::Full, UIMd3Shape::Full);
            painter.setPen(palette().color(QPalette::WindowText));
            painter.setFont(font());
            const QRect textRect = rect().adjusted(14, 0, -14, 0);
            const QString strVisible = QFontMetrics(font()).elidedText(text(), Qt::ElideRight,
                                                                       qMax(0, textRect.width()));
            painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, strVisible);
        }
    };
}

UIMd3Wizard::UIMd3Wizard(QWidget *pParent)
    : QWidget(pParent)
    , m_pPageTitle(0)
    , m_pStepSummary(0)
    , m_pStepPanel(0)
    , m_pPagePanel(0)
    , m_pLayout(0)
    , m_pBodyLayout(0)
    , m_pStepLayout(0)
    , m_pContentLayout(0)
    , m_pActionLayout(0)
    , m_iCurrentStep(-1)
{
    registerMd3WizardTexts();
    setObjectName(QStringLiteral("md3WizardShell"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::NoFocus);
    setMinimumSize(QSize(0, 0));

    m_pLayout = new QVBoxLayout(this);
    const int iGutter = UIMd3Theme::instance() ? md3Theme().gutter() : 16;
    m_pLayout->setContentsMargins(iGutter, iGutter, iGutter, iGutter);
    m_pLayout->setSpacing(12);

    m_pBodyLayout = new QHBoxLayout;
    m_pBodyLayout->setContentsMargins(0, 0, 0, 0);
    m_pBodyLayout->setSpacing(12);
    m_pLayout->addLayout(m_pBodyLayout, 1);

    m_pStepPanel = new UIMd3WizardSurface(UIMd3ColorRole_SurfaceContainerLow,
                                          UIMd3ColorRole_OutlineVariant, this);
    m_pStepPanel->setObjectName(QStringLiteral("md3WizardStepRail"));
    m_pStepPanel->setMinimumWidth(176);
    m_pStepPanel->setMaximumWidth(220);
    m_pStepPanel->setFocusPolicy(Qt::NoFocus);
    m_pBodyLayout->addWidget(m_pStepPanel, 0);

    m_pStepLayout = new QVBoxLayout(m_pStepPanel);
    m_pStepLayout->setContentsMargins(12, 12, 12, 12);
    m_pStepLayout->setSpacing(6);

    m_pPagePanel = new UIMd3WizardSurface(UIMd3ColorRole_SurfaceContainer,
                                          UIMd3ColorRole_OutlineVariant, this);
    m_pPagePanel->setObjectName(QStringLiteral("md3WizardPageCard"));
    m_pPagePanel->setMinimumSize(QSize(0, 0));
    m_pBodyLayout->addWidget(m_pPagePanel, 1);

    QVBoxLayout *pPageLayout = new QVBoxLayout(m_pPagePanel);
    pPageLayout->setContentsMargins(iGutter, iGutter, iGutter, iGutter);
    pPageLayout->setSpacing(8);

    m_pPageTitle = new QLabel(m_pPagePanel);
    m_pPageTitle->setWordWrap(true);
    m_pPageTitle->setMinimumWidth(0);
    if (UIMd3Theme::instance())
        m_pPageTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pPageLayout->addWidget(m_pPageTitle);

    m_pStepSummary = new QLabel(m_pPagePanel);
    m_pStepSummary->setWordWrap(true);
    m_pStepSummary->setMinimumWidth(0);
    if (UIMd3Theme::instance())
        m_pStepSummary->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
    pPageLayout->addWidget(m_pStepSummary);

    m_pContentLayout = new QVBoxLayout;
    m_pContentLayout->setContentsMargins(0, 4, 0, 0);
    m_pContentLayout->setSpacing(0);
    pPageLayout->addLayout(m_pContentLayout, 1);

    m_pActionLayout = new QVBoxLayout;
    m_pActionLayout->setContentsMargins(0, 0, 0, 0);
    m_pActionLayout->setSpacing(0);
    m_pLayout->addLayout(m_pActionLayout);

    if (UIMd3Theme::instance())
        connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
                this, &UIMd3Wizard::sltThemeChanged);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3Wizard::sltLanguageChanged);
    sltLanguageChanged();
    updateResponsiveLayout();
}

void UIMd3Wizard::setContentWidget(QWidget *pWidget)
{
    if (!pWidget || !m_pContentLayout)
        return;
    pWidget->setParent(m_pPagePanel ? m_pPagePanel : this);
    pWidget->setMinimumSize(QSize(0, 0));
    m_pContentLayout->addWidget(pWidget, 1);
}

void UIMd3Wizard::setActionWidget(QWidget *pWidget)
{
    if (!pWidget || !m_pActionLayout)
        return;
    pWidget->setParent(this);
    pWidget->setObjectName(QStringLiteral("md3WizardActions"));
    pWidget->setAutoFillBackground(false);
    m_pActionLayout->addWidget(pWidget);
}

void UIMd3Wizard::clearSteps()
{
    if (!m_pStepLayout)
        return;
    while (QLayoutItem *pItem = m_pStepLayout->takeAt(0))
    {
        if (pItem->widget())
            delete pItem->widget();
        delete pItem;
    }
    m_steps.clear();
}

void UIMd3Wizard::setStepTitles(const QStringList &titles)
{
    /* Page changes can submit the same translated titles repeatedly.  Keep the
     * existing labels so navigation does not destroy and rebuild the step rail. */
    if (m_stepTitles == titles && m_steps.size() == titles.size())
        return;
    m_stepTitles = titles;
    clearSteps();
    if (!m_pStepLayout)
        return;

    for (int i = 0; i < m_stepTitles.size(); ++i)
    {
        QLabel *pStep = new UIMd3WizardStepLabel(m_pStepPanel);
        pStep->setMinimumHeight(qMax(48, UIMd3Theme::instance() ? md3Theme().controlHeight() : 40));
        pStep->setText(m_stepTitles.at(i));
        pStep->setToolTip(m_stepTitles.at(i));
        m_steps << pStep;
        m_pStepLayout->addWidget(pStep);
    }
    m_pStepLayout->addStretch(1);
    setCurrentStep(m_iCurrentStep);
    updateResponsiveLayout();
}

void UIMd3Wizard::setCurrentStep(int iIndex)
{
    m_iCurrentStep = iIndex;
    const int cSteps = m_stepTitles.size();
    const QString strSummary = cSteps > 0 && iIndex >= 0 && iIndex < cSteps
                              ? wizardText("md3.wizard.progress-current", tr("Step %1 of %2"))
                                .arg(iIndex + 1).arg(cSteps)
                              : QString();
    if (m_pStepSummary)
    {
        m_pStepSummary->setText(strSummary);
        m_pStepSummary->setAccessibleDescription(strSummary.isEmpty()
                                                  ? wizardText("md3.wizard.progress-unavailable",
                                                               tr("Wizard progress is unavailable."))
                                                  : wizardText("md3.wizard.progress-current-description",
                                                               tr("Current wizard progress: %1"))
                                                    .arg(strSummary));
    }
    for (int i = 0; i < m_steps.size(); ++i)
    {
        QLabel *pStep = m_steps.at(i);
        const bool fCurrent = i == iIndex;
        const bool fComplete = i < iIndex;
        QPalette pal = pStep->palette();
        if (UIMd3Theme::instance())
        {
            pal.setColor(QPalette::Window,
                         fCurrent ? md3Theme().color(UIMd3ColorRole_SecondaryContainer)
                                  : fComplete ? md3Theme().color(UIMd3ColorRole_PrimaryContainer)
                                              : md3Theme().color(UIMd3ColorRole_SurfaceContainerHigh));
            pal.setColor(QPalette::WindowText,
                         fCurrent ? md3Theme().color(UIMd3ColorRole_OnSecondaryContainer)
                                  : fComplete ? md3Theme().color(UIMd3ColorRole_OnPrimaryContainer)
                                              : md3Theme().color(UIMd3ColorRole_OnSurfaceVariant));
        }
        pStep->setPalette(pal);
        pStep->setFont(UIMd3Theme::instance()
                       ? md3Theme().font(fCurrent ? UIMd3TypeRole_LabelLarge : UIMd3TypeRole_LabelMedium)
                       : font());
        const QString strTitle = m_stepTitles.value(i);
        pStep->setText(fComplete ? QStringLiteral("✓ ") + strTitle : strTitle);
        pStep->setAccessibleName(wizardText("md3.wizard.step-name", tr("Wizard step %1: %2"))
                                 .arg(i + 1).arg(strTitle)
                                 + (fComplete
                                    ? wizardText("md3.wizard.step-complete-suffix", tr(", complete"))
                                    : QString()));
        const QString strState = fCurrent
                               ? wizardText("md3.wizard.step-current", tr("Current step"))
                               : fComplete
                               ? wizardText("md3.wizard.step-complete", tr("Completed step"))
                               : wizardText("md3.wizard.step-upcoming", tr("Upcoming step"));
        pStep->setAccessibleDescription(wizardText("md3.wizard.step-position",
                                                    tr("Wizard step %1 of %2. %3."))
                                        .arg(i + 1).arg(cSteps).arg(strState));
        pStep->update();
    }
    updateResponsiveLayout();
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
    pStep->setAccessibleName(wizardText("md3.wizard.step-name", tr("Wizard step %1: %2"))
                             .arg(iIndex + 1).arg(strTitle)
                             + (fComplete
                                ? wizardText("md3.wizard.step-complete-suffix", tr(", complete"))
                                : QString()));
    const QString strState = iIndex == m_iCurrentStep
                           ? wizardText("md3.wizard.step-current", tr("Current step"))
                           : fComplete
                           ? wizardText("md3.wizard.step-complete", tr("Completed step"))
                           : wizardText("md3.wizard.step-upcoming", tr("Upcoming step"));
    pStep->setAccessibleDescription(wizardText("md3.wizard.step-position",
                                                tr("Wizard step %1 of %2. %3."))
                                    .arg(iIndex + 1).arg(m_stepTitles.size()).arg(strState));
    pStep->update();
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
    if (m_pStepPanel)
        m_pStepPanel->update();
    if (m_pPagePanel)
        m_pPagePanel->update();
    setCurrentStep(m_iCurrentStep);
}

void UIMd3Wizard::sltLanguageChanged()
{
    registerMd3WizardTexts();
    setAccessibleName(wizardText("md3.wizard.name", tr("Wizard")));
    setAccessibleDescription(wizardText("md3.wizard.description",
                                        tr("Wizard with page progress and active page content.")));
    if (m_pStepPanel)
    {
        m_pStepPanel->setAccessibleName(wizardText("md3.wizard.progress", tr("Wizard progress")));
        m_pStepPanel->setAccessibleDescription(wizardText("md3.wizard.progress-description",
                                                          tr("Progress through the wizard steps.")));
    }
    if (m_pPageTitle)
    {
        m_pPageTitle->setAccessibleName(wizardText("md3.wizard.current-page",
                                                   tr("Current wizard page")));
        m_pPageTitle->setAccessibleDescription(wizardText("md3.wizard.current-page-description",
                                                          tr("Title of the active wizard page.")));
    }
    if (m_pStepSummary)
    {
        m_pStepSummary->setAccessibleName(wizardText("md3.wizard.progress", tr("Wizard progress")));
        m_pStepSummary->setAccessibleDescription(wizardText("md3.wizard.progress-description",
                                                            tr("Progress through the wizard steps.")));
    }
    setCurrentStep(m_iCurrentStep);
}

void UIMd3Wizard::updateResponsiveLayout()
{
    if (!m_pStepPanel)
        return;
    const bool fShowStepRail = width() >= g_iWizardCompactWidth && m_steps.size() > 1;
    m_pStepPanel->setVisible(fShowStepRail);
}

void UIMd3Wizard::paintEvent(QPaintEvent *pEvent)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setClipRect(pEvent->rect());
    const QColor background = UIMd3Theme::instance()
                            ? md3Theme().color(UIMd3ColorRole_Surface)
                            : palette().color(QPalette::Window);
    painter.setPen(Qt::NoPen);
    painter.setBrush(background);
    painter.drawRoundedRect(rect(), UIMd3Shape::Large, UIMd3Shape::Large);
    QWidget::paintEvent(pEvent);
}

void UIMd3Wizard::resizeEvent(QResizeEvent *pEvent)
{
    QWidget::resizeEvent(pEvent);
    updateResponsiveLayout();
}
