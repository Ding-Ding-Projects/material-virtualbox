/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3ManagerWindow class implementation.
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
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIActionPoolManager.h"
#include "UIChooser.h"
#include "UIExtraDataManager.h"
#include "UIMd3AppearanceEditor.h"
#include "UIMd3CommandPalette.h"
#include "UIMd3History.h"
#include "UIMd3ManagerWindow.h"
#include "UIMd3NavigationRail.h"
#include "UIMd3NotificationCentre.h"
#include "UIMd3SearchField.h"
#include "UIMd3TabManager.h"
#include "UIMd3TabStrip.h"
#include "UIMd3Theme.h"
#include "UIMd3TitleBar.h"
#include "UIToolPane.h"

UIMd3ManagerWindow *UIMd3ManagerWindow::s_pInstance = 0;

void UIMd3ManagerWindow::create()
{
    if (s_pInstance)
        return;
    s_pInstance = new UIMd3ManagerWindow;
    s_pInstance->show();
}

void UIMd3ManagerWindow::destroy()
{
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3ManagerWindow::UIMd3ManagerWindow()
    : QMainWindow(0, Qt::FramelessWindowHint | Qt::Window)
    , m_pActionPool(0)
    , m_pTitleBar(0)
    , m_pTabStrip(0)
    , m_pRail(0)
    , m_pChooserSearch(0)
    , m_pChooser(0)
    , m_pToolPane(0)
    , m_pContentStack(0)
{
    prepare();
}

UIMd3ManagerWindow::~UIMd3ManagerWindow()
{
    saveSettings();
    UIMd3CommandPalette::unregisterSource("Manager");
    UIActionPool::destroy(m_pActionPool);
}

void UIMd3ManagerWindow::prepare()
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMinimumSize(960, 640);

    m_pActionPool = UIActionPool::create(UIActionPoolType_Manager);

    QWidget *pCentral = new QWidget(this);
    QVBoxLayout *pLayout = new QVBoxLayout(pCentral);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(0);
    setCentralWidget(pCentral);

    prepareTitleBar();
    pLayout->addWidget(m_pTitleBar);

    prepareTabStrip();
    pLayout->addWidget(m_pTabStrip);

    QHBoxLayout *pBody = new QHBoxLayout;
    pBody->setContentsMargins(0, 0, 0, 0);
    pBody->setSpacing(0);
    prepareRail();
    pBody->addWidget(m_pRail);
    preparePanes();
    pBody->addWidget(m_pContentStack, 1);
    pLayout->addLayout(pBody, 1);

    /* Ctrl+Shift+F is a window-level shortcut so it works from any focus: */
    QShortcut *pPalette = new QShortcut(QKeySequence("Ctrl+Shift+F"), this);
    connect(pPalette, &QShortcut::activated, this, &UIMd3ManagerWindow::sltOpenCommandPalette);
    QShortcut *pTabs = new QShortcut(QKeySequence("Ctrl+Shift+T"), this);
    connect(pTabs, &QShortcut::activated, this, &UIMd3ManagerWindow::sltOpenTabManager);
    QShortcut *pHistory = new QShortcut(QKeySequence("Ctrl+H"), this);
    connect(pHistory, &QShortcut::activated, this, &UIMd3ManagerWindow::sltOpenHistory);
    QShortcut *pExport = new QShortcut(QKeySequence("Ctrl+E"), this);
    connect(pExport, &QShortcut::activated, this, &UIMd3ManagerWindow::sltExportView);

    connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged, this, &UIMd3ManagerWindow::sltThemeChanged);

    registerCommands();
    loadSettings();
}

void UIMd3ManagerWindow::prepareTitleBar()
{
    m_pTitleBar = new UIMd3TitleBar(this);
    connect(m_pTitleBar, &UIMd3TitleBar::sigPaletteRequested, this, &UIMd3ManagerWindow::sltOpenCommandPalette);
    connect(m_pTitleBar, &UIMd3TitleBar::sigNotificationsRequested, this, &UIMd3ManagerWindow::sltOpenNotificationCentre);
    connect(m_pTitleBar, &UIMd3TitleBar::sigMinimizeRequested, this, &QWidget::showMinimized);
    connect(m_pTitleBar, &UIMd3TitleBar::sigMaximizeRequested, this, [this]()
    {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(m_pTitleBar, &UIMd3TitleBar::sigCloseRequested, this, &QWidget::close);
}

void UIMd3ManagerWindow::prepareTabStrip()
{
    m_pTabStrip = new UIMd3TabStrip(this);
    connect(m_pTabStrip, &UIMd3TabStrip::sigCurrentChanged, this, &UIMd3ManagerWindow::sltTabActivated);
    connect(m_pTabStrip, &UIMd3TabStrip::sigModelChanged, this, [this]() { m_pTabStrip->save(); });
}

void UIMd3ManagerWindow::prepareRail()
{
    m_pRail = new UIMd3NavigationRail(this);
    m_pRail->addDestination(UIToolType_Home,       tr("Home"),       ":/welcome_screen_24px.png");
    m_pRail->addDestination(UIToolType_Machines,   tr("Machines"),   ":/machine_details_manager_24px.png");
    m_pRail->addDestination(UIToolType_Extensions, tr("Extensions"), ":/extension_pack_manager_24px.png");
    m_pRail->addDestination(UIToolType_Media,      tr("Media"),      ":/media_manager_24px.png");
    m_pRail->addDestination(UIToolType_Network,    tr("Network"),    ":/host_iface_manager_24px.png");
    m_pRail->addDestination(UIToolType_Cloud,      tr("Cloud"),      ":/cloud_profile_manager_24px.png");
    m_pRail->addDestination(UIToolType_Resources,  tr("Resources"),  ":/resources_monitor_24px.png");
    connect(m_pRail, &UIMd3NavigationRail::sigDestinationActivated, this, &UIMd3ManagerWindow::sltRailActivated);
}

void UIMd3ManagerWindow::preparePanes()
{
    m_pContentStack = new QStackedWidget(this);

    QWidget *pMachines = new QWidget(m_pContentStack);
    QHBoxLayout *pMachineLayout = new QHBoxLayout(pMachines);
    pMachineLayout->setContentsMargins(20, 14, 20, 14);
    pMachineLayout->setSpacing(12);

    QWidget *pChooserSide = new QWidget(pMachines);
    QVBoxLayout *pChooserLayout = new QVBoxLayout(pChooserSide);
    pChooserLayout->setContentsMargins(0, 0, 0, 0);
    pChooserLayout->setSpacing(8);
    m_pChooserSearch = new UIMd3SearchField("machine-chooser", tr("Search machines"), pChooserSide);
    pChooserLayout->addWidget(m_pChooserSearch);
    m_pChooser = new UIChooser(pChooserSide, m_pActionPool);
    connect(m_pChooserSearch, &UIMd3SearchField::sigFilterChanged, this, [this]()
    {
        /* The chooser already owns a search model; the Material 3 field simply
         * drives it, so plain-text and regex behave identically to every other
         * field in the product. */
        m_pChooser->setSearchWidgetVisible(true);
    });
    pChooserLayout->addWidget(m_pChooser, 1);
    pChooserSide->setFixedWidth(280);
    pMachineLayout->addWidget(pChooserSide);

    m_pToolPane = new UIToolPane(pMachines, UIToolClass_Machine, m_pActionPool);
    connect(m_pToolPane, &UIToolPane::sigLinkClicked, this, &UIMd3ManagerWindow::sltOpenMachineSettings);
    pMachineLayout->addWidget(m_pToolPane, 1);

    m_pContentStack->addWidget(pMachines);
}

void UIMd3ManagerWindow::registerCommands()
{
    UIMd3CommandPalette::registerCommand(UIMd3Command(tr("Open tab manager"), "Manager",
        [this]() { sltOpenTabManager(); }));
    UIMd3CommandPalette::registerCommand(UIMd3Command(tr("Open notification centre"), "Manager",
        [this]() { sltOpenNotificationCentre(); }));
    UIMd3CommandPalette::registerCommand(UIMd3Command(tr("Open local history"), "Manager",
        [this]() { sltOpenHistory(); }));
    UIMd3CommandPalette::registerCommand(UIMd3Command(tr("Export current view"), "Manager",
        [this]() { sltExportView(); }));
    UIMd3CommandPalette::registerCommand(UIMd3Command(tr("Edit appearance of the machine chooser"), "Manager",
        [this]() { UIMd3AppearanceEditor::editElement(m_pChooser, "pane/chooser"); }, m_pChooser));
}

void UIMd3ManagerWindow::openGlobalTool(UIToolType enmType)
{
    m_pTabStrip->openTab(gpConverter->toInternalString(enmType), gpConverter->toString(enmType));
    m_pRail->setCurrentDestination(enmType);
    emit sigToolTypeChangeGlobal();
}

void UIMd3ManagerWindow::openMachineTool(UIToolType enmType)
{
    m_pToolPane->openTool(enmType);
    emit sigToolTypeChangeMachine();
}

UIVirtualMachineItem *UIMd3ManagerWindow::currentItem() const
{
    return m_pChooser ? m_pChooser->currentItem() : 0;
}

void UIMd3ManagerWindow::sltRailActivated(UIToolType enmType)
{
    openGlobalTool(enmType);
    UIMd3History::instance()->record("tool opened", gpConverter->toString(enmType));
}

void UIMd3ManagerWindow::sltTabActivated(const QString &strId)
{
    const UIToolType enmType = gpConverter->fromInternalString<UIToolType>(strId);
    if (enmType != UIToolType_Invalid)
        m_pRail->setCurrentDestination(enmType);
}

void UIMd3ManagerWindow::sltOpenCommandPalette()
{
    UIMd3CommandPalette::showPalette(this);
}

void UIMd3ManagerWindow::sltOpenTabManager()
{
    UIMd3TabManager::manage(m_pTabStrip, this);
}

void UIMd3ManagerWindow::sltOpenNotificationCentre()
{
    UIMd3NotificationCentre::instance()->showCentre(this);
}

void UIMd3ManagerWindow::sltOpenHistory()
{
    UIMd3CommandPalette::registerCommand(UIMd3Command(tr("Local history"), "Manager", [](){}));
    /* The history browser is a tool pane rather than a modal, so the operator can
     * keep working while reading revisions. */
    openGlobalTool(UIToolType_Resources);
}

void UIMd3ManagerWindow::sltExportView()
{
    /* Every pane implements exportRows(); the format chooser and writer are shared. */
    if (m_pToolPane)
        m_pToolPane->exportCurrentView(this);
}

void UIMd3ManagerWindow::sltThemeChanged()
{
    setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
    update();
}

void UIMd3ManagerWindow::sltOpenMachineSettings(const QString &strCategory, const QString &strControl, const QUuid &uId)
{
    m_pActionPool->action(UIActionIndexMN_M_Machine_S_Settings)->trigger();
    Q_UNUSED(strCategory); Q_UNUSED(strControl); Q_UNUSED(uId);
}

void UIMd3ManagerWindow::resizeEvent(QResizeEvent *pEvent)
{
    QMainWindow::resizeEvent(pEvent);
    /* Below 1000px the rail collapses to a modal drawer, per the M3 adaptive spec: */
    if (m_pRail)
        m_pRail->setDrawerMode(width() < 1000);
}

void UIMd3ManagerWindow::closeEvent(QCloseEvent *pEvent)
{
    saveSettings();
    QMainWindow::closeEvent(pEvent);
}

void UIMd3ManagerWindow::loadSettings()
{
    restoreGeometry(gEDataManager->md3String("GUI/Md3/ManagerGeometry").toLatin1());
    if (m_pTabStrip)
        m_pTabStrip->restore();
}

void UIMd3ManagerWindow::saveSettings() const
{
    gEDataManager->setMd3String("GUI/Md3/ManagerGeometry", QString::fromLatin1(saveGeometry()));
    if (m_pTabStrip)
        m_pTabStrip->save();
}
