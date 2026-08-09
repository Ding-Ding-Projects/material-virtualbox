/* $Id: UIGlobalToolsWidget.cpp 112403 2026-01-11 19:29:08Z knut.osmundsen@oracle.com $ */
/** @file
 * VBox Qt GUI - UIGlobalToolsWidget class implementation.
 */

/*
 * Copyright (C) 2006-2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QAction>
#include <QColor>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QResizeEvent>
#include <QToolButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIToolBar.h"
#include "UIActionPool.h"
#include "UIChooser.h"
#include "UICommon.h"
#include "UIExtraDataManager.h"
#include "UIGlobalToolsWidget.h"
#include "UIMd3Language.h"
#include "UIMd3MenuSearch.h"
#include "UIMd3NavigationRail.h"
#include "UIMd3Theme.h"
#include "UIMachineToolsWidget.h"
#include "UIToolPane.h"
#include "UITools.h"
#include "UIVirtualBoxEventHandler.h"
#include "UIVirtualMachineItem.h"

/* Other VBox includes: */
#include "iprt/assert.h"

static QString md3ManagerText(const char *pszKey, const QString &strFallback)
{
    const UIMd3Language *pLanguage = UIMd3Language::instance();
    return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
}


UIGlobalToolsWidget::UIGlobalToolsWidget(QWidget *pParent, UIActionPool *pActionPool)
    : QWidget(pParent)
    , m_pActionPool(pActionPool)
    , m_pLayout(0)
    , m_pPageHeaderLayout(0)
    , m_pPageHeader(0)
    , m_pPageEyebrow(0)
    , m_pPageTitle(0)
    , m_pNavigationDrawerButton(0)
    , m_pMenu(0)
    , m_pNavigationRail(0)
    , m_pPane(0)
{
    prepare();
}

void UIGlobalToolsWidget::addToolBar(QIToolBar *pToolBar)
{
    AssertPtrReturnVoid(m_pPageHeaderLayout);
    m_pPageHeaderLayout->addWidget(pToolBar, 0, Qt::AlignVCenter);
}

UIToolPane *UIGlobalToolsWidget::toolPane() const
{
    return m_pPane;
}

UIMachineToolsWidget *UIGlobalToolsWidget::machineToolsWidget() const
{
    return toolPane()->machineToolsWidget();
}

UIToolType UIGlobalToolsWidget::menuToolType() const
{
    AssertPtrReturn(toolMenu(), UIToolType_Invalid);
    return toolMenu()->toolsType();
}

void UIGlobalToolsWidget::setMenuToolType(UIToolType enmType)
{
    /* Sanity check: */
    AssertReturnVoid(enmType != UIToolType_Invalid);

    AssertPtrReturnVoid(toolMenu());
    toolMenu()->setToolsType(enmType);
}

bool UIGlobalToolsWidget::isMenuToolEnabled(UIToolType enmType) const
{
    if (!chooser() || !gEDataManager)
        return enmType == UIToolType_Home;
    if (enmType == UIToolType_Machines && chooser()->isNavigationListEmpty())
        return false;
    if (!gEDataManager->isSettingsInExpertMode()
        && (enmType == UIToolType_Media || enmType == UIToolType_Network))
        return false;
    return true;
}

UIToolType UIGlobalToolsWidget::toolType() const
{
    AssertPtrReturn(toolPane(), UIToolType_Invalid);
    return toolPane()->currentTool();
}

bool UIGlobalToolsWidget::isToolOpened(UIToolType enmType) const
{
    /* Sanity check: */
    AssertReturn(enmType != UIToolType_Invalid, false);
    /* Make sure new tool type is of Global class: */
    AssertReturn(UIToolStuff::isTypeOfClass(enmType, UIToolClass_Global), false);

    AssertPtrReturn(toolPane(), false);
    return toolPane()->isToolOpened(enmType);
}

void UIGlobalToolsWidget::switchToolTo(UIToolType enmType)
{
    /* Sanity check: */
    AssertReturnVoid(enmType != UIToolType_Invalid);
    /* Make sure new tool type is of Global class: */
    AssertReturnVoid(UIToolStuff::isTypeOfClass(enmType, UIToolClass_Global));

    /* Open corresponding tool: */
    AssertPtrReturnVoid(toolPane());
    toolPane()->openTool(enmType);

    /* Notify corresponding tool-pane it's active: */
    toolPane()->setActive(enmType != UIToolType_Machines);
    toolPaneMachine()->setActive(enmType == UIToolType_Machines);

    /* Let the parent know: */
    emit sigToolTypeChange();
}

void UIGlobalToolsWidget::closeTool(UIToolType enmType)
{
    /* Sanity check: */
    AssertReturnVoid(enmType != UIToolType_Invalid);
    /* Make sure new tool type is of Global class: */
    AssertReturnVoid(UIToolStuff::isTypeOfClass(enmType, UIToolClass_Global));

    AssertPtrReturnVoid(toolPane());
    toolPane()->closeTool(enmType);
}

QString UIGlobalToolsWidget::currentHelpKeyword() const
{
    if (toolType() == UIToolType_Machines)
    {
        AssertPtrReturn(machineToolsWidget(), QString());
        return machineToolsWidget()->currentHelpKeyword();
    }

    AssertPtrReturn(toolPane(), QString());
    return toolPane()->currentHelpKeyword();
}

void UIGlobalToolsWidget::sltHandleCommitData()
{
    cleanupConnections();
}

void UIGlobalToolsWidget::sltHandleMachineRegistrationChanged(const QUuid &, const bool fRegistered)
{
    /* On any VM registered switch from Home to Machines: */
    AssertPtrReturnVoid(toolMenu());
    if (fRegistered && toolMenu()->toolsType() == UIToolType_Home)
        setMenuToolType(UIToolType_Machines);
}

void UIGlobalToolsWidget::sltHandleSettingsExpertModeChange()
{
    /* Update tools restrictions: */
    emit sigToolMenuUpdate();
}

void UIGlobalToolsWidget::sltHandleChooserPaneNavigationListChange()
{
    /* Update tools restrictions: */
    emit sigToolMenuUpdate();
    sltHandleCurrentMachineLabelChange();
}

void UIGlobalToolsWidget::sltHandleCurrentMachineLabelChange()
{
    if (toolMenu() && toolMenu()->toolsType() == UIToolType_Machines)
        updatePageHeader(UIToolType_Machines);
}

void UIGlobalToolsWidget::sltHandleCloudProfileStateChange(const QString &, const QString &)
{
    /* If Global Resources tool is currently chosen: */
    AssertPtrReturnVoid(toolPane());
    if (toolPane()->currentTool() == UIToolType_Resources)
    {
        /* Propagate a set of cloud machine items to Global tool-pane: */
        toolPane()->setCloudMachineItems(chooser()->cloudMachineItems());
    }
}

void UIGlobalToolsWidget::sltHandleToolMenuUpdate()
{
    /* Prepare tool restrictions: */
    QSet<UIToolType> restrictedTypes;

    /* Make sure Machines tool is hidden for empty Chooser-pane: */
    if (chooser()->isNavigationListEmpty())
        restrictedTypes << UIToolType_Machines;

    /* Restrict some types for Basic mode: */
    const bool fExpertMode = gEDataManager->isSettingsInExpertMode();
    if (!fExpertMode)
        restrictedTypes << UIToolType_Media
                        << UIToolType_Network;

    /* Make sure no restricted tool is selected: */
    if (restrictedTypes.contains(toolMenu()->toolsType()))
        setMenuToolType(UIToolType_Home);

    if (m_pNavigationRail)
    {
        m_pNavigationRail->setToolEnabled(UIToolType_Home, true);
        m_pNavigationRail->setToolEnabled(UIToolType_Machines, !restrictedTypes.contains(UIToolType_Machines));
        m_pNavigationRail->setToolEnabled(UIToolType_Extensions, !restrictedTypes.contains(UIToolType_Extensions));
        m_pNavigationRail->setToolEnabled(UIToolType_Media, !restrictedTypes.contains(UIToolType_Media));
        m_pNavigationRail->setToolEnabled(UIToolType_Network, !restrictedTypes.contains(UIToolType_Network));
        m_pNavigationRail->setToolEnabled(UIToolType_Cloud, !restrictedTypes.contains(UIToolType_Cloud));
        m_pNavigationRail->setToolEnabled(UIToolType_Resources, !restrictedTypes.contains(UIToolType_Resources));
        m_pNavigationRail->setCurrentToolType(toolMenu()->toolsType());
    }

    /* Hide restricted tools in the menu: */
    const QList restrictions(restrictedTypes.begin(), restrictedTypes.end());
    toolMenu()->setRestrictedToolTypes(restrictions);

    /* Close all restricted tools (besides the Machines): */
    foreach (const UIToolType &enmRestrictedType, restrictedTypes)
        if (enmRestrictedType != UIToolType_Machines)
            toolPane()->closeTool(enmRestrictedType);
}

void UIGlobalToolsWidget::sltHandleToolsMenuIndexChange(UIToolType enmType)
{
    /* Determine tool class of passed tool type: */
    const UIToolClass enmClass = UIToolStuff::castTypeToClass(enmType);

    /* For Global tool class: */
    if (enmClass == UIToolClass_Global)
    {
        updatePageHeader(enmType);
        /* Switch tool-pane accordingly: */
        switchToolTo(enmType);

        /* Special handling for Global Resources tool,
         * start unconditionally updating all cloud VMs: */
        if (enmType == UIToolType_Resources)
        {
            chooser()->setKeepCloudNodesUpdated(true);
            toolPane()->setCloudMachineItems(chooser()->cloudMachineItems());
        }
        /* Otherwise, stop unconditionally updating all cloud VMs,
         * (tho they will still be updated if selected) */
        else
            chooser()->setKeepCloudNodesUpdated(false);
    }
}

void UIGlobalToolsWidget::sltRetranslateUI()
{
    if (m_pNavigationDrawerButton)
    {
        const QString strLabel = md3ManagerText("md3.manager.navigation", tr("Navigation"));
        m_pNavigationDrawerButton->setAccessibleName(strLabel);
        m_pNavigationDrawerButton->setToolTip(strLabel);
        m_pNavigationDrawerButton->setAccessibleDescription(
            md3ManagerText("md3.manager.open-navigation-drawer",
                           tr("Open the searchable manager destination drawer")));
    }
    if (m_pPageEyebrow)
        m_pPageEyebrow->setAccessibleDescription(
            md3ManagerText("md3.manager.destination-category", tr("Destination category")));
    if (m_pPageTitle)
        m_pPageTitle->setAccessibleDescription(
            md3ManagerText("md3.manager.current-destination", tr("Current destination")));
    updatePageHeader(toolMenu() ? toolMenu()->toolsType() : UIToolType_Home);
}

void UIGlobalToolsWidget::sltShowNavigationDrawer()
{
    if (!m_pNavigationDrawerButton)
        return;

    QMenu menu(this);
    menu.setAccessibleName(md3ManagerText("md3.manager.destinations", tr("Manager destinations")));
    const struct
    {
        UIToolType enmType;
        const char *pszKey;
        const char *pszFallback;
    } aDestinations[] =
    {
        { UIToolType_Home,       "md3.tab.home",       QT_TRANSLATE_NOOP("UIGlobalToolsWidget", "Home") },
        { UIToolType_Machines,   "md3.tab.machines",   QT_TRANSLATE_NOOP("UIGlobalToolsWidget", "Machines") },
        { UIToolType_Extensions, "md3.tab.extensions", QT_TRANSLATE_NOOP("UIGlobalToolsWidget", "Extensions") },
        { UIToolType_Media,      "md3.tab.media",      QT_TRANSLATE_NOOP("UIGlobalToolsWidget", "Media") },
        { UIToolType_Network,    "md3.tab.network",    QT_TRANSLATE_NOOP("UIGlobalToolsWidget", "Network") },
        { UIToolType_Cloud,      "md3.tab.cloud",      QT_TRANSLATE_NOOP("UIGlobalToolsWidget", "Cloud") },
        { UIToolType_Resources,  "md3.tab.resources",  QT_TRANSLATE_NOOP("UIGlobalToolsWidget", "Resources") }
    };
    const UIMd3Language *pLanguage = UIMd3Language::instance();
    for (size_t i = 0; i < RT_ELEMENTS(aDestinations); ++i)
    {
        const QString strLabel = pLanguage
                               ? pLanguage->text(QString::fromLatin1(aDestinations[i].pszKey))
                               : tr(aDestinations[i].pszFallback);
        QAction *pAction = menu.addAction(strLabel);
        pAction->setCheckable(true);
        pAction->setChecked(menuToolType() == aDestinations[i].enmType);
        const bool fEnabled = isMenuToolEnabled(aDestinations[i].enmType);
        pAction->setEnabled(fEnabled);
        if (!fEnabled)
        {
            const QString strReason = disabledReason(aDestinations[i].enmType);
            pAction->setToolTip(strReason);
            pAction->setStatusTip(strReason);
            pAction->setWhatsThis(strReason);
            pAction->setProperty("accessibleDescription", strReason);
        }
        const UIToolType enmType = aDestinations[i].enmType;
        connect(pAction, &QAction::triggered, this, [this, enmType]()
        {
            setMenuToolType(enmType);
        });
    }
    menu.addSeparator();
    if (actionPool() && actionPool()->action(UIActionIndex_M_Application_S_Preferences))
        menu.addAction(actionPool()->action(UIActionIndex_M_Application_S_Preferences));
    md3PrepareSearchableMenu(&menu,
                             QStringLiteral("manager-navigation-drawer"),
                             md3ManagerText("md3.manager.search-destinations",
                                            tr("Search manager destinations")),
                             md3ManagerText("md3.manager.search-destinations-description",
                                            tr("Search this navigation drawer")));
    menu.exec(m_pNavigationDrawerButton->mapToGlobal(
        QPoint(0, m_pNavigationDrawerButton->height())));
}

void UIGlobalToolsWidget::updatePageHeader(UIToolType enmType)
{
    if (!m_pPageEyebrow || !m_pPageTitle)
        return;
    const UIMd3Language *pLanguage = UIMd3Language::instance();
    QString strEyebrow;
    QString strTitle;
    if (enmType == UIToolType_Machines)
    {
        strEyebrow = pLanguage
                    ? pLanguage->text(QStringLiteral("md3.manager.machine-tools"))
                    : tr("Machine tools");
        UIVirtualMachineItem *pItem = machineToolsWidget() ? machineToolsWidget()->currentItem() : 0;
        strTitle = pItem
                 ? pItem->name()
                 : pLanguage
                 ? pLanguage->text(QStringLiteral("md3.manager.no-machine-selected"))
                 : tr("No machine selected");
    }
    else
    {
        strEyebrow = pLanguage
                    ? pLanguage->text(QStringLiteral("md3.manager.global-tool"))
                    : tr("Global tool");
        switch (enmType)
        {
            case UIToolType_Home:       strTitle = pLanguage ? pLanguage->text(QStringLiteral("md3.tab.home")) : tr("Home"); break;
            case UIToolType_Extensions: strTitle = pLanguage ? pLanguage->text(QStringLiteral("md3.tab.extensions")) : tr("Extensions"); break;
            case UIToolType_Media:      strTitle = pLanguage ? pLanguage->text(QStringLiteral("md3.tab.media")) : tr("Media"); break;
            case UIToolType_Network:    strTitle = pLanguage ? pLanguage->text(QStringLiteral("md3.tab.network")) : tr("Network"); break;
            case UIToolType_Cloud:      strTitle = pLanguage ? pLanguage->text(QStringLiteral("md3.tab.cloud")) : tr("Cloud"); break;
            case UIToolType_Resources:  strTitle = pLanguage ? pLanguage->text(QStringLiteral("md3.tab.resources")) : tr("Resources"); break;
            default:                    strTitle = tr("Manager"); break;
        }
    }
    m_pPageEyebrow->setText(strEyebrow.toUpper());
    m_pPageEyebrow->setAccessibleName(strEyebrow);
    m_pPageEyebrow->setToolTip(strEyebrow);
    m_pPageTitle->setText(strTitle);
    m_pPageTitle->setAccessibleName(strTitle);
    m_pPageTitle->setToolTip(strTitle);
}

void UIGlobalToolsWidget::updatePageHeaderTheme()
{
    if (!m_pPageHeader || !m_pPageEyebrow || !m_pPageTitle)
        return;
    m_pPageHeader->setStyleSheet(QStringLiteral(
        "QWidget#md3DestinationHeader { background: %1; border-bottom: 1px solid %2; }")
        .arg(md3(UIMd3ColorRole_Surface).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_OutlineVariant).name(QColor::HexArgb)));
    m_pPageEyebrow->setFont(md3Theme().font(UIMd3TypeRole_LabelSmall));
    m_pPageEyebrow->setStyleSheet(QStringLiteral("color:%1;")
                                  .arg(md3(UIMd3ColorRole_Primary).name(QColor::HexArgb)));
    m_pPageTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    m_pPageTitle->setStyleSheet(QStringLiteral("color:%1;")
                                .arg(md3(UIMd3ColorRole_OnSurface).name(QColor::HexArgb)));
    if (m_pNavigationDrawerButton)
        m_pNavigationDrawerButton->setStyleSheet(QStringLiteral(
            "QToolButton { color:%1; background:%2; border:0; border-radius:16px; }"
            "QToolButton:hover { background:%3; }"
            "QToolButton:focus { border:2px solid %4; }")
            .arg(md3(UIMd3ColorRole_OnSecondaryContainer).name(QColor::HexArgb))
            .arg(md3(UIMd3ColorRole_SecondaryContainer).name(QColor::HexArgb))
            .arg(md3(UIMd3ColorRole_SurfaceContainerHighest).name(QColor::HexArgb))
            .arg(md3(UIMd3ColorRole_Primary).name(QColor::HexArgb)));
}

void UIGlobalToolsWidget::sltSwitchToResourcesTool()
{
    setMenuToolType(UIToolType_Resources);
}

void UIGlobalToolsWidget::sltSwitchToResourceUseTool(const QUuid &uMachineId)
{
    AssertPtrReturnVoid(chooser());
    chooser()->setCurrentMachine(uMachineId);
    setMenuToolType(UIToolType_Machines);
    machineToolsWidget()->setMenuToolType(UIToolType_ResourceUse);
}

void UIGlobalToolsWidget::prepare()
{
    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.subtitle"), QStringLiteral("Manager"), QStringLiteral("管理員"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.global-tool"), QStringLiteral("Global tool"), QStringLiteral("全域工具"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.machine-tools"), QStringLiteral("Machine tools"), QStringLiteral("虛擬機工具"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.no-machine-selected"), QStringLiteral("No machine selected"), QStringLiteral("未揀虛擬機"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.navigation"), QStringLiteral("Navigation"), QStringLiteral("導覽"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.open-navigation-drawer"), QStringLiteral("Open the searchable manager destination drawer"), QStringLiteral("打開可搜尋的管理員目的地抽屜"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.destinations"), QStringLiteral("Manager destinations"), QStringLiteral("管理員目的地"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.search-destinations"), QStringLiteral("Search manager destinations"), QStringLiteral("搜尋管理員目的地"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.search-destinations-description"), QStringLiteral("Search this navigation drawer"), QStringLiteral("搜尋這個導覽抽屜"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.destination-category"), QStringLiteral("Destination category"), QStringLiteral("目的地類別"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.current-destination"), QStringLiteral("Current destination"), QStringLiteral("目前目的地"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.disabled.machines"), QStringLiteral("No virtual machine is available."), QStringLiteral("沒有可用的虛擬機。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.disabled.expert"), QStringLiteral("Expert mode is required."), QStringLiteral("需要啟用專家模式。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.disabled.generic"), QStringLiteral("This destination is currently unavailable."), QStringLiteral("這個目的地目前不可用。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.tab.home"), QStringLiteral("Home"), QStringLiteral("主頁"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.tab.machines"), QStringLiteral("Machines"), QStringLiteral("虛擬機"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.tab.extensions"), QStringLiteral("Extensions"), QStringLiteral("擴充功能"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.tab.media"), QStringLiteral("Media"), QStringLiteral("媒體"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.tab.network"), QStringLiteral("Network"), QStringLiteral("網絡"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.tab.cloud"), QStringLiteral("Cloud"), QStringLiteral("雲端"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.tab.resources"), QStringLiteral("Resources"), QStringLiteral("資源"));
    }

    /* Load settings: */
    loadSettings();
    updatePageHeaderTheme();
    sltRetranslateUI();
    updateResponsiveNavigation();
}

void UIGlobalToolsWidget::prepareWidgets()
{
    /* Create layout: */
    m_pLayout = new QGridLayout(this);
    if (m_pLayout)
    {
        /* Configure layout: */
        m_pLayout->setContentsMargins(0, 0, 0, 0);
        m_pLayout->setSpacing(0);

        /* Create tool-menu: */
        m_pMenu = new UITools(this, UIToolClass_Global);
        if (toolMenu())
        {
            /* Keep the existing model-backed menu alive for selection and
             * restriction logic, while presenting the Material 3 rail. */
            toolMenu()->hide();
            m_pNavigationRail = new UIMd3NavigationRail(this);
            if (m_pNavigationRail)
                m_pLayout->addWidget(m_pNavigationRail, 0, 0, 2, 1);
        }

        /* Create a compact destination heading.  It owns the contextual
         * action toolbar, so the body no longer starts with a detached
         * legacy toolbar row. */
        m_pPageHeader = new QWidget(this);
        if (m_pPageHeader)
        {
            m_pPageHeader->setObjectName(QStringLiteral("md3DestinationHeader"));
            m_pPageHeader->setAttribute(Qt::WA_StyledBackground, true);
            m_pPageHeader->setMinimumHeight(64);
            m_pPageHeaderLayout = new QHBoxLayout(m_pPageHeader);
            m_pPageHeaderLayout->setContentsMargins(20, 6, 20, 2);
            m_pPageHeaderLayout->setSpacing(12);

            m_pNavigationDrawerButton = new QToolButton(m_pPageHeader);
            m_pNavigationDrawerButton->setObjectName(QStringLiteral("md3NavigationDrawerButton"));
            m_pNavigationDrawerButton->setText(QString(QChar(0x2630)));
            m_pNavigationDrawerButton->setAutoRaise(true);
            m_pNavigationDrawerButton->setFixedSize(QSize(48, 48));
            m_pNavigationDrawerButton->setFocusPolicy(Qt::StrongFocus);
            m_pPageHeaderLayout->addWidget(m_pNavigationDrawerButton, 0, Qt::AlignVCenter);

            QVBoxLayout *pTitleLayout = new QVBoxLayout;
            pTitleLayout->setContentsMargins(0, 0, 0, 0);
            pTitleLayout->setSpacing(0);
            m_pPageEyebrow = new QLabel(m_pPageHeader);
            m_pPageTitle = new QLabel(m_pPageHeader);
            m_pPageEyebrow->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
            m_pPageTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
            m_pPageEyebrow->setWordWrap(true);
            m_pPageTitle->setWordWrap(true);
            pTitleLayout->addWidget(m_pPageEyebrow);
            pTitleLayout->addWidget(m_pPageTitle);
            m_pPageHeaderLayout->addLayout(pTitleLayout, 1);
            m_pLayout->addWidget(m_pPageHeader, 0, 1);
        }

        /* Create tool-pane: */
        m_pPane = new UIToolPane(this, UIToolClass_Global, actionPool());
        if (toolPane())
        {
            /* Add into layout: */
            m_pLayout->addWidget(toolPane(), 1, 1);
        }
    }
}

void UIGlobalToolsWidget::prepareConnections()
{
    /* UICommon connections: */
    connect(&uiCommon(), &UICommon::sigAskToCommitData,
            this, &UIGlobalToolsWidget::sltHandleCommitData);

    /* Global COM event handlers: */
    connect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineRegistered,
            this, &UIGlobalToolsWidget::sltHandleMachineRegistrationChanged);
    connect(gEDataManager, &UIExtraDataManager::sigSettingsExpertModeChange,
            this, &UIGlobalToolsWidget::sltHandleSettingsExpertModeChange);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIGlobalToolsWidget::sltRetranslateUI, Qt::UniqueConnection);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged,
            this, &UIGlobalToolsWidget::updatePageHeaderTheme);
    if (m_pNavigationDrawerButton)
        connect(m_pNavigationDrawerButton, &QToolButton::clicked,
                this, &UIGlobalToolsWidget::sltShowNavigationDrawer);

    /* Chooser-pane connections: */
    connect(chooser(), &UIChooser::sigNavigationListChanged,
            this, &UIGlobalToolsWidget::sltHandleChooserPaneNavigationListChange);
    connect(chooser(), &UIChooser::sigCloudProfileStateChange,
            this, &UIGlobalToolsWidget::sltHandleCloudProfileStateChange);

    /* Tools-menu connections: */
    connect(toolMenu(), &UITools::sigSelectionChanged,
            this, &UIGlobalToolsWidget::sltHandleToolsMenuIndexChange);
    connect(toolMenu(), &UITools::sigSelectionChanged,
            m_pNavigationRail, &UIMd3NavigationRail::setCurrentToolType);
    connect(m_pNavigationRail, &UIMd3NavigationRail::sigToolTypeSelected,
            this, &UIGlobalToolsWidget::setMenuToolType);
    connect(machineToolsWidget(), &UIMachineToolsWidget::sigCurrentMachineLabelChange,
            this, &UIGlobalToolsWidget::sltHandleCurrentMachineLabelChange);
    if (actionPool() && actionPool()->action(UIActionIndex_M_Application_S_Preferences))
        connect(m_pNavigationRail, &UIMd3NavigationRail::sigPreferencesRequested,
                actionPool()->action(UIActionIndex_M_Application_S_Preferences), &QAction::trigger);

    /* Tools-pane connections: */
    connect(this, &UIGlobalToolsWidget::sigToolMenuUpdate,
            this, &UIGlobalToolsWidget::sltHandleToolMenuUpdate);
    connect(toolPane(), &UIToolPane::sigSwitchToMachineActivityPane,
            this, &UIGlobalToolsWidget::sltSwitchToResourceUseTool);
    connect(toolPaneMachine(), &UIToolPane::sigSwitchToActivityOverviewPane,
            this, &UIGlobalToolsWidget::sltSwitchToResourcesTool);
}

void UIGlobalToolsWidget::loadSettings()
{
    /* Acquire & select tool currently chosen in the menu: */
    sltHandleToolsMenuIndexChange(toolMenu()->toolsType());

    /* Update tools restrictions: */
    sltHandleToolMenuUpdate();
}

void UIGlobalToolsWidget::resizeEvent(QResizeEvent *pEvent)
{
    QWidget::resizeEvent(pEvent);
    updateResponsiveNavigation();
}

void UIGlobalToolsWidget::updateResponsiveNavigation()
{
    const bool fUseDrawer = width() > 0 && width() < 1000;
    if (m_pNavigationRail)
        m_pNavigationRail->setVisible(!fUseDrawer);
    if (m_pNavigationDrawerButton)
        m_pNavigationDrawerButton->setVisible(fUseDrawer);
}

QString UIGlobalToolsWidget::disabledReason(UIToolType enmType) const
{
    switch (enmType)
    {
        case UIToolType_Machines:
            return md3ManagerText("md3.manager.disabled.machines",
                                  tr("No virtual machine is available."));
        case UIToolType_Media:
        case UIToolType_Network:
            return md3ManagerText("md3.manager.disabled.expert",
                                  tr("Expert mode is required."));
        default:
            return md3ManagerText("md3.manager.disabled.generic",
                                  tr("This destination is currently unavailable."));
    }
}

void UIGlobalToolsWidget::cleanupConnections()
{
    /* Global COM event handlers: */
    disconnect(gVBoxEvents, &UIVirtualBoxEventHandler::sigMachineRegistered,
               this, &UIGlobalToolsWidget::sltHandleMachineRegistrationChanged);
    disconnect(gEDataManager, &UIExtraDataManager::sigSettingsExpertModeChange,
               this, &UIGlobalToolsWidget::sltHandleSettingsExpertModeChange);
    if (UIMd3Language::instance())
        disconnect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                   this, &UIGlobalToolsWidget::sltRetranslateUI);
    disconnect(&md3Theme(), &UIMd3Theme::sigThemeChanged,
               this, &UIGlobalToolsWidget::updatePageHeaderTheme);
    if (m_pNavigationDrawerButton)
        disconnect(m_pNavigationDrawerButton, &QToolButton::clicked,
                   this, &UIGlobalToolsWidget::sltShowNavigationDrawer);

    /* Chooser-pane connections: */
    disconnect(chooser(), &UIChooser::sigNavigationListChanged,
               this, &UIGlobalToolsWidget::sltHandleChooserPaneNavigationListChange);
    disconnect(chooser(), &UIChooser::sigCloudProfileStateChange,
               this, &UIGlobalToolsWidget::sltHandleCloudProfileStateChange);

    /* Tools-menu connections: */
    disconnect(toolMenu(), &UITools::sigSelectionChanged,
               this, &UIGlobalToolsWidget::sltHandleToolsMenuIndexChange);
    disconnect(machineToolsWidget(), &UIMachineToolsWidget::sigCurrentMachineLabelChange,
               this, &UIGlobalToolsWidget::sltHandleCurrentMachineLabelChange);

    /* Tools-pane connections: */
    disconnect(this, &UIGlobalToolsWidget::sigToolMenuUpdate,
               this, &UIGlobalToolsWidget::sltHandleToolMenuUpdate);
    disconnect(toolPane(), &UIToolPane::sigSwitchToMachineActivityPane,
               this, &UIGlobalToolsWidget::sltSwitchToResourceUseTool);
    disconnect(toolPaneMachine(), &UIToolPane::sigSwitchToActivityOverviewPane,
               this, &UIGlobalToolsWidget::sltSwitchToResourcesTool);
}

UITools *UIGlobalToolsWidget::toolMenu() const
{
    return m_pMenu;
}

UIChooser *UIGlobalToolsWidget::chooser() const
{
    return machineToolsWidget()->chooser();
}

UIToolPane *UIGlobalToolsWidget::toolPaneMachine() const
{
    return machineToolsWidget()->toolPane();
}
