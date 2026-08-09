/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 four-scope tab manager.
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
#include <QAbstractItemView>
#include <QAccessible>
#include <QApplication>
#include <QCheckBox>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolTip>
#include <QUuid>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3Language.h"
#include "UIMd3SearchField.h"
#include "UIMd3TabManager.h"
#include "UIMd3TabStrip.h"
#include "UIMd3Theme.h"

enum
{
    TabIdRole = Qt::UserRole,
    StripTokenRole,
    GroupIdRole,
    SearchCandidateRole,
    TabEnabledRole
};

static QList<QPointer<UIMd3TabStrip> > g_liveTabStrips;
static QList<QPointer<UIMd3TabManager> > g_liveTabManagers;

static void md3RegisterTabManagerText()
{
    UIMd3Language *pLanguage = UIMd3Language::instance();
    if (!pLanguage)
        return;
#define REGISTER_TAB_MANAGER_TEXT(a_pszKey, a_pszEnglish, a_pszCantonese) \
    pLanguage->registerText(QStringLiteral(a_pszKey), QStringLiteral(a_pszEnglish), QStringLiteral(a_pszCantonese))
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.title", "Tab manager", "分頁管理員");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.heading", "Find, organize, and safely close workspace tabs", "尋找、整理同安全關閉工作區分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.current", "Current strip", "目前分頁列");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.search-current", "Search this tab strip", "搜尋目前分頁列");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.current-results", "Tabs in the current strip", "目前分頁列嘅分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.groups", "Groups", "群組");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.search-groups", "Search tab groups by name", "按名稱搜尋分頁群組");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.group-results", "Tab groups", "分頁群組");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.ungrouped", "No group (top level)", "無群組（頂層）");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.search-group", "Search tabs in %1", "搜尋 %1 入面嘅分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.group-tabs", "Tabs in group %1", "群組 %1 嘅分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.new-group", "New group name", "新群組名稱");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.create-group", "Create group", "新增群組");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.group-name-required", "Enter a group name from 1 to 80 characters.", "輸入 1 至 80 個字元嘅群組名稱。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.master", "All windows", "所有視窗");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.search-master", "Search every open tab", "搜尋所有已開分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.master-results", "Tabs in all application windows", "所有應用程式視窗嘅分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.close-tools", "Bulk close", "批量關閉");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.close-scope", "Close preview applies only to the current strip. Pinned tabs stay protected unless explicitly included.", "關閉預覽只套用目前分頁列。除非明確包括，固定分頁會繼續受保護。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.containing", "Close tabs containing text", "關閉名稱包含文字嘅分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.not-containing", "Close tabs not containing text", "關閉名稱唔包含文字嘅分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.query-containing", "Text tabs must contain", "分頁名稱必須包含嘅文字");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.query-not-containing", "Text tabs must not contain", "分頁名稱唔可以包含嘅文字");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.preview", "Preview", "預覽");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.include-pinned", "Include pinned tabs in this preview", "今次預覽包括固定分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.preview-results", "Tabs reviewed for closing", "已檢視準備關閉嘅分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.apply-close", "Close reviewed tabs", "關閉已檢視分頁");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.cancel", "Close tab manager", "關閉分頁管理員");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.no-tabs", "No tabs are open in this scope.", "呢個範圍未有分頁開啟。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.no-match", "No tabs match this search.", "冇分頁符合呢個搜尋。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.no-group-match", "No tab groups match this search.", "冇分頁群組符合呢個搜尋。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.window", "Window", "視窗");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.pinned", "Pinned", "已固定");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.unpinned", "Not pinned", "未固定");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.current-state", "Current", "目前");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.unavailable", "Unavailable under the current manager restrictions", "按目前管理員限制暫時不可用");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.activate", "Activate %1 in %2, group %3, %4", "喺 %2 啟用 %1，群組 %3，%4");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.query-required", "Enter a non-empty plain-text query or apply a valid regular expression before previewing.", "預覽之前，輸入非空白純文字搜尋，或者套用有效正規表示式。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.preview-required", "Preview the current query before closing any tabs.", "關閉任何分頁之前，先預覽目前搜尋。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.preview-none", "The current query matches no closable tabs.", "目前搜尋冇符合任何可關閉分頁。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.preview-summary", "%1 tab(s) will close. Review every row, then use Close reviewed tabs.", "%1 個分頁會關閉。逐行檢視之後，再用「關閉已檢視分頁」。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.preview-changed", "The tab model or close options changed. Preview again before closing anything.", "分頁模型或者關閉選項有變。關閉任何分頁之前請再預覽。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.closed", "Closed %1 reviewed tab(s); %2 tab(s) were skipped because their state changed.", "已關閉 %1 個已檢視分頁；有 %2 個因狀態改變而略過。");
    REGISTER_TAB_MANAGER_TEXT("md3.tab-manager.group-members", "%1 member(s)", "%1 個成員");
#undef REGISTER_TAB_MANAGER_TEXT
}

static QString md3TabManagerText(const char *pszKey, const QString &strFallback)
{
    if (!UIMd3Language::instance())
        return strFallback;
    const QString strKey = QString::fromLatin1(pszKey);
    const QString strText = md3Text(strKey);
    return strText == strKey || strText.isEmpty() ? strFallback : strText;
}

static void md3AnnounceTabManagerStatus(QLabel *pStatus, const QString &strText)
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

static QString md3StripToken(UIMd3TabStrip *pStrip)
{
    if (!pStrip)
        return QString();
    QString strToken = pStrip->property("md3LiveStripToken").toString();
    if (strToken.isEmpty())
    {
        strToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
        pStrip->setProperty("md3LiveStripToken", strToken);
    }
    return strToken;
}

static UIMd3TabStrip *md3FindStrip(const QString &strToken)
{
    for (const QPointer<UIMd3TabStrip> &pStrip : g_liveTabStrips)
        if (pStrip && md3StripToken(pStrip) == strToken)
            return pStrip;
    return 0;
}

static QString md3StripWindowName(UIMd3TabStrip *pStrip)
{
    if (pStrip && pStrip->window() && !pStrip->window()->windowTitle().trimmed().isEmpty())
        return pStrip->window()->windowTitle().trimmed();
    return md3TabManagerText("md3.tab-manager.window", QObject::tr("Window"));
}

static QString md3GroupName(UIMd3TabStrip *pStrip, const QString &strGroupId)
{
    if (pStrip)
        for (const UIMd3TabGroup &group : pStrip->groups())
            if (group.strId == strGroupId)
                return group.strName;
    return md3TabManagerText("md3.tab-manager.ungrouped", QObject::tr("No group (top level)"));
}

static QListWidgetItem *md3AddTabResult(QListWidget *pList,
                                        UIMd3TabStrip *pStrip,
                                        const UIMd3Tab &tab)
{
    if (!pList || !pStrip)
        return 0;
    const QString strWindow = md3StripWindowName(pStrip);
    const QString strGroup = md3GroupName(pStrip, tab.strGroupId);
    const QString strPin = tab.fPinned
                         ? md3TabManagerText("md3.tab-manager.pinned", QObject::tr("Pinned"))
                         : md3TabManagerText("md3.tab-manager.unpinned", QObject::tr("Not pinned"));
    QString strState;
    if (pStrip->currentTabId() == tab.strId)
        strState = md3TabManagerText("md3.tab-manager.current-state", QObject::tr("Current"))
                 + QStringLiteral(" · ");
    const QString strSecondary = QStringLiteral("%1%2 · %3 · %4")
                               .arg(strState, strWindow, strGroup, strPin);
    QListWidgetItem *pItem = new QListWidgetItem(tab.strLabel + QLatin1Char('\n') + strSecondary, pList);
    pItem->setData(TabIdRole, tab.strId);
    pItem->setData(StripTokenRole, md3StripToken(pStrip));
    pItem->setData(GroupIdRole, tab.strGroupId);
    pItem->setData(TabEnabledRole, tab.fEnabled);
    pItem->setData(SearchCandidateRole,
                   QStringLiteral("%1 %2 %3 %4 %5")
                   .arg(tab.strLabel, strWindow, strGroup, strPin, strState));
    const QString strDescription = tab.fEnabled
        ? md3TabManagerText("md3.tab-manager.activate",
                            QObject::tr("Activate %1 in %2, group %3, %4"))
          .arg(tab.strLabel, strWindow, strGroup, strPin)
        : md3TabManagerText("md3.tab-manager.unavailable",
                            QObject::tr("Unavailable under the current manager restrictions"));
    pItem->setToolTip(strDescription);
    pItem->setStatusTip(strDescription);
    if (!tab.fEnabled)
        pItem->setForeground(pList->palette().brush(QPalette::Disabled, QPalette::Text));
    return pItem;
}

void UIMd3TabManager::manage(UIMd3TabStrip *pStrip, QWidget *pParent)
{
    if (!pStrip)
        return;
    for (int i = g_liveTabManagers.size() - 1; i >= 0; --i)
        if (!g_liveTabManagers.at(i))
            g_liveTabManagers.removeAt(i);
    for (const QPointer<UIMd3TabManager> &pManager : g_liveTabManagers)
        if (pManager && pManager->m_pStrip == pStrip)
        {
            pManager->show();
            pManager->raise();
            pManager->activateWindow();
            return;
        }
    UIMd3TabManager *pManager = new UIMd3TabManager(pStrip, pParent);
    pManager->show();
    pManager->raise();
    pManager->activateWindow();
}

void UIMd3TabManager::registerStrip(UIMd3TabStrip *pStrip)
{
    if (!pStrip)
        return;
    md3StripToken(pStrip);
    for (int i = g_liveTabStrips.size() - 1; i >= 0; --i)
        if (!g_liveTabStrips.at(i))
            g_liveTabStrips.removeAt(i);
    bool fKnown = false;
    for (const QPointer<UIMd3TabStrip> &pKnown : g_liveTabStrips)
        fKnown |= pKnown == pStrip;
    if (!fKnown)
        g_liveTabStrips << QPointer<UIMd3TabStrip>(pStrip);
    for (int i = g_liveTabManagers.size() - 1; i >= 0; --i)
    {
        UIMd3TabManager *pManager = g_liveTabManagers.at(i);
        if (!pManager)
        {
            g_liveTabManagers.removeAt(i);
            continue;
        }
        QObject::connect(pStrip, &UIMd3TabStrip::sigModelChanged,
                         pManager, &UIMd3TabManager::sltRefresh, Qt::UniqueConnection);
        pManager->sltRefresh();
    }
}

void UIMd3TabManager::unregisterStrip(UIMd3TabStrip *pStrip)
{
    for (int i = g_liveTabStrips.size() - 1; i >= 0; --i)
        if (!g_liveTabStrips.at(i) || g_liveTabStrips.at(i) == pStrip)
            g_liveTabStrips.removeAt(i);
    for (int i = g_liveTabManagers.size() - 1; i >= 0; --i)
    {
        UIMd3TabManager *pManager = g_liveTabManagers.at(i);
        if (!pManager)
            g_liveTabManagers.removeAt(i);
        else if (pManager->m_pStrip == pStrip)
            pManager->close();
        else
            pManager->sltRefresh();
    }
}

UIMd3TabManager::UIMd3TabManager(UIMd3TabStrip *pStrip, QWidget *pParent)
    : QDialog(pParent, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
    , m_pStrip(pStrip)
    , m_pCurrentSearch(0)
    , m_pCurrentList(0)
    , m_pGroupNameSearch(0)
    , m_pGroupList(0)
    , m_pGroupStack(0)
    , m_pNewGroupName(0)
    , m_pCreateGroup(0)
    , m_pMasterSearch(0)
    , m_pMasterList(0)
    , m_pCloseContaining(0)
    , m_pCloseNotContaining(0)
    , m_pIncludePinned(0)
    , m_pCloseStatus(0)
    , m_pClosePreview(0)
    , m_pApplyClose(0)
    , m_fPendingInverse(false)
    , m_fPendingRegex(false)
    , m_fPendingIncludePinned(false)
{
    md3RegisterTabManagerText();
    setAttribute(Qt::WA_DeleteOnClose, true);
    setObjectName(QStringLiteral("md3TabManager"));
    g_liveTabManagers << QPointer<UIMd3TabManager>(this);
    prepare();
    for (const QPointer<UIMd3TabStrip> &pLiveStrip : g_liveTabStrips)
        if (pLiveStrip)
            connect(pLiveStrip, &UIMd3TabStrip::sigModelChanged,
                    this, &UIMd3TabManager::sltRefresh, Qt::UniqueConnection);
    sltRefresh();
}

void UIMd3TabManager::prepare()
{
    setWindowTitle(md3TabManagerText("md3.tab-manager.title", tr("Tab manager")));
    setAccessibleName(windowTitle());
    setAutoFillBackground(true);
    QPalette surfacePalette = palette();
    surfacePalette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    surfacePalette.setColor(QPalette::Base, md3(UIMd3ColorRole_Surface));
    setPalette(surfacePalette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(this);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(8);
    QLabel *pHeading = new QLabel(md3TabManagerText("md3.tab-manager.heading",
                                                   tr("Find, organize, and safely close workspace tabs")), this);
    pHeading->setWordWrap(true);
    pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pRootLayout->addWidget(pHeading);

    QTabWidget *pScopes = new QTabWidget(this);
    pScopes->setObjectName(QStringLiteral("md3TabManagerScopes"));
    pScopes->setAccessibleName(windowTitle());
    pRootLayout->addWidget(pScopes, 1);

    QWidget *pCurrentPage = new QWidget(pScopes);
    QVBoxLayout *pCurrentLayout = new QVBoxLayout(pCurrentPage);
    m_pCurrentSearch = new UIMd3SearchField(QStringLiteral("tab-manager-current-strip"),
        md3TabManagerText("md3.tab-manager.search-current", tr("Search this tab strip")), pCurrentPage);
    pCurrentLayout->addWidget(m_pCurrentSearch);
    m_pCurrentList = new QListWidget(pCurrentPage);
    m_pCurrentList->setAccessibleName(md3TabManagerText("md3.tab-manager.current-results",
                                                       tr("Tabs in the current strip")));
    m_pCurrentList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pCurrentList->setContextMenuPolicy(Qt::CustomContextMenu);
    pCurrentLayout->addWidget(m_pCurrentList, 1);
    pScopes->addTab(pCurrentPage, md3TabManagerText("md3.tab-manager.current", tr("Current strip")));

    QWidget *pGroupsPage = new QWidget(pScopes);
    QVBoxLayout *pGroupsLayout = new QVBoxLayout(pGroupsPage);
    m_pGroupNameSearch = new UIMd3SearchField(QStringLiteral("tab-manager-group-names"),
        md3TabManagerText("md3.tab-manager.search-groups", tr("Search tab groups by name")), pGroupsPage);
    pGroupsLayout->addWidget(m_pGroupNameSearch);
    m_pGroupList = new QListWidget(pGroupsPage);
    m_pGroupList->setAccessibleName(md3TabManagerText("md3.tab-manager.group-results", tr("Tab groups")));
    m_pGroupList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pGroupList->setMaximumHeight(132);
    pGroupsLayout->addWidget(m_pGroupList);
    m_pGroupStack = new QStackedWidget(pGroupsPage);
    pGroupsLayout->addWidget(m_pGroupStack, 1);
    QHBoxLayout *pCreateLayout = new QHBoxLayout;
    m_pNewGroupName = new QLineEdit(pGroupsPage);
    m_pNewGroupName->setMaxLength(80);
    m_pNewGroupName->setPlaceholderText(md3TabManagerText("md3.tab-manager.new-group",
                                                          tr("New group name")));
    m_pNewGroupName->setAccessibleName(m_pNewGroupName->placeholderText());
    m_pCreateGroup = new QPushButton(md3TabManagerText("md3.tab-manager.create-group",
                                                       tr("Create group")), pGroupsPage);
    m_pCreateGroup->setMinimumHeight(48);
    m_pCreateGroup->setEnabled(false);
    m_pCreateGroup->setToolTip(md3TabManagerText("md3.tab-manager.group-name-required",
                                                tr("Enter a group name from 1 to 80 characters.")));
    m_pCreateGroup->setAccessibleDescription(m_pCreateGroup->toolTip());
    pCreateLayout->addWidget(m_pNewGroupName, 1);
    pCreateLayout->addWidget(m_pCreateGroup);
    pGroupsLayout->addLayout(pCreateLayout);
    pScopes->addTab(pGroupsPage, md3TabManagerText("md3.tab-manager.groups", tr("Groups")));

    QWidget *pMasterPage = new QWidget(pScopes);
    QVBoxLayout *pMasterLayout = new QVBoxLayout(pMasterPage);
    m_pMasterSearch = new UIMd3SearchField(QStringLiteral("tab-manager-master-all-windows"),
        md3TabManagerText("md3.tab-manager.search-master", tr("Search every open tab")), pMasterPage);
    pMasterLayout->addWidget(m_pMasterSearch);
    m_pMasterList = new QListWidget(pMasterPage);
    m_pMasterList->setAccessibleName(md3TabManagerText("md3.tab-manager.master-results",
                                                      tr("Tabs in all application windows")));
    m_pMasterList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pMasterList->setContextMenuPolicy(Qt::CustomContextMenu);
    pMasterLayout->addWidget(m_pMasterList, 1);
    pScopes->addTab(pMasterPage, md3TabManagerText("md3.tab-manager.master", tr("All windows")));

    QScrollArea *pCloseScroll = new QScrollArea(pScopes);
    pCloseScroll->setFrameShape(QFrame::NoFrame);
    pCloseScroll->setWidgetResizable(true);
    QWidget *pClosePage = new QWidget(pCloseScroll);
    QVBoxLayout *pCloseLayout = new QVBoxLayout(pClosePage);
    QLabel *pScope = new QLabel(md3TabManagerText("md3.tab-manager.close-scope",
        tr("Close preview applies only to the current strip. Pinned tabs stay protected unless explicitly included.")), pClosePage);
    pScope->setWordWrap(true);
    pCloseLayout->addWidget(pScope);

    QGroupBox *pContainingBox = new QGroupBox(md3TabManagerText("md3.tab-manager.containing",
                                                               tr("Close tabs containing text")), pClosePage);
    QVBoxLayout *pContainingLayout = new QVBoxLayout(pContainingBox);
    m_pCloseContaining = new UIMd3SearchField(QStringLiteral("tab-manager-close-containing"),
        md3TabManagerText("md3.tab-manager.query-containing", tr("Text tabs must contain")), pContainingBox);
    QPushButton *pPreviewContaining = new QPushButton(md3TabManagerText("md3.tab-manager.preview", tr("Preview")), pContainingBox);
    pPreviewContaining->setMinimumHeight(48);
    pContainingLayout->addWidget(m_pCloseContaining);
    pContainingLayout->addWidget(pPreviewContaining);
    pCloseLayout->addWidget(pContainingBox);

    QGroupBox *pNotContainingBox = new QGroupBox(md3TabManagerText("md3.tab-manager.not-containing",
                                                                  tr("Close tabs not containing text")), pClosePage);
    QVBoxLayout *pNotContainingLayout = new QVBoxLayout(pNotContainingBox);
    m_pCloseNotContaining = new UIMd3SearchField(QStringLiteral("tab-manager-close-not-containing"),
        md3TabManagerText("md3.tab-manager.query-not-containing", tr("Text tabs must not contain")), pNotContainingBox);
    QPushButton *pPreviewNotContaining = new QPushButton(md3TabManagerText("md3.tab-manager.preview", tr("Preview")), pNotContainingBox);
    pPreviewNotContaining->setMinimumHeight(48);
    pNotContainingLayout->addWidget(m_pCloseNotContaining);
    pNotContainingLayout->addWidget(pPreviewNotContaining);
    pCloseLayout->addWidget(pNotContainingBox);

    m_pIncludePinned = new QCheckBox(md3TabManagerText("md3.tab-manager.include-pinned",
                                                       tr("Include pinned tabs in this preview")), pClosePage);
    pCloseLayout->addWidget(m_pIncludePinned);
    m_pCloseStatus = new QLabel(pClosePage);
    m_pCloseStatus->setWordWrap(true);
    m_pCloseStatus->setFocusPolicy(Qt::StrongFocus);
    m_pCloseStatus->setAccessibleName(md3TabManagerText("md3.tab-manager.preview-results",
                                                        tr("Tabs reviewed for closing")));
    m_pCloseStatus->setText(md3TabManagerText("md3.tab-manager.preview-required",
                                              tr("Preview the current query before closing any tabs.")));
    pCloseLayout->addWidget(m_pCloseStatus);
    m_pClosePreview = new QListWidget(pClosePage);
    m_pClosePreview->setAccessibleName(md3TabManagerText("md3.tab-manager.preview-results",
                                                        tr("Tabs reviewed for closing")));
    m_pClosePreview->setSelectionMode(QAbstractItemView::NoSelection);
    m_pClosePreview->setMinimumHeight(96);
    pCloseLayout->addWidget(m_pClosePreview, 1);
    m_pApplyClose = new QPushButton(md3TabManagerText("md3.tab-manager.apply-close",
                                                      tr("Close reviewed tabs")), pClosePage);
    m_pApplyClose->setMinimumHeight(48);
    m_pApplyClose->setEnabled(false);
    pCloseLayout->addWidget(m_pApplyClose);
    pCloseScroll->setWidget(pClosePage);
    pScopes->addTab(pCloseScroll, md3TabManagerText("md3.tab-manager.close-tools", tr("Bulk close")));

    QDialogButtonBox *pButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    pButtons->button(QDialogButtonBox::Close)->setText(
        md3TabManagerText("md3.tab-manager.cancel", tr("Close tab manager")));
    pRootLayout->addWidget(pButtons);

    connect(m_pCurrentSearch, &UIMd3SearchField::sigFilterChanged,
            this, &UIMd3TabManager::refreshCurrentRows);
    connect(m_pMasterSearch, &UIMd3SearchField::sigFilterChanged,
            this, &UIMd3TabManager::refreshMasterRows);
    connect(m_pGroupNameSearch, &UIMd3SearchField::sigFilterChanged,
            this, &UIMd3TabManager::refreshGroupNames);
    connect(m_pCurrentList, &QListWidget::itemActivated,
            this, &UIMd3TabManager::activateItem);
    connect(m_pMasterList, &QListWidget::itemActivated,
            this, &UIMd3TabManager::activateItem);
    connect(m_pCurrentList, &QListWidget::customContextMenuRequested, this,
            [this](const QPoint &position) { showItemActions(m_pCurrentList, position); });
    connect(m_pMasterList, &QListWidget::customContextMenuRequested, this,
            [this](const QPoint &position) { showItemActions(m_pMasterList, position); });
    connect(m_pGroupList, &QListWidget::currentRowChanged,
            m_pGroupStack, &QStackedWidget::setCurrentIndex);
    connect(m_pNewGroupName, &QLineEdit::textChanged, this,
            [this](const QString &strText)
    {
        const bool fValid = !strText.trimmed().isEmpty();
        m_pCreateGroup->setEnabled(fValid);
        m_pCreateGroup->setAccessibleDescription(fValid ? QString() : m_pCreateGroup->toolTip());
    });
    connect(m_pCreateGroup, &QPushButton::clicked,
            this, &UIMd3TabManager::sltCreateGroup);
    connect(pPreviewContaining, &QPushButton::clicked,
            this, &UIMd3TabManager::sltPreviewContaining);
    connect(pPreviewNotContaining, &QPushButton::clicked,
            this, &UIMd3TabManager::sltPreviewNotContaining);
    connect(m_pCloseContaining, &UIMd3SearchField::sigFilterChanged, this,
            [this]() { invalidateClosePreview(); });
    connect(m_pCloseNotContaining, &UIMd3SearchField::sigFilterChanged, this,
            [this]() { invalidateClosePreview(); });
    connect(m_pIncludePinned, &QCheckBox::toggled, this,
            [this](bool) { invalidateClosePreview(); });
    connect(m_pApplyClose, &QPushButton::clicked,
            this, &UIMd3TabManager::sltApplyClose);
    connect(pButtons, &QDialogButtonBox::rejected, this, &QDialog::close);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &QDialog::close, Qt::UniqueConnection);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged, this, [this]()
    {
        QPalette updatedPalette = palette();
        updatedPalette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
        updatedPalette.setColor(QPalette::Base, md3(UIMd3ColorRole_Surface));
        setPalette(updatedPalette);
    });

    QScreen *pScreen = parentWidget() && parentWidget()->window()
                     ? parentWidget()->window()->screen() : QApplication::primaryScreen();
    const QRect available = pScreen ? pScreen->availableGeometry() : QRect(0, 0, 640, 360);
    const int iMaximumWidth = qMax(1, available.width() - 24);
    const int iMaximumHeight = qMax(1, available.height() - 48);
    setMaximumSize(iMaximumWidth, iMaximumHeight);
    setMinimumSize(qMin(520, iMaximumWidth), qMin(320, iMaximumHeight));
    resize(qMin(760, iMaximumWidth), qMin(560, iMaximumHeight));
    m_pCurrentSearch->setFocus(Qt::OtherFocusReason);
}

void UIMd3TabManager::addEmptyResult(QListWidget *pList, const QString &strText) const
{
    if (!pList)
        return;
    QListWidgetItem *pItem = new QListWidgetItem(strText, pList);
    pItem->setFlags(Qt::ItemIsEnabled);
    pItem->setData(TabEnabledRole, false);
}

void UIMd3TabManager::refreshCurrentRows()
{
    if (!m_pCurrentList || !m_pCurrentSearch || !m_pStrip)
        return;
    m_pCurrentList->clear();
    int cMatches = 0;
    for (const UIMd3Tab &tab : m_pStrip->tabs())
    {
        QListWidgetItem *pItem = md3AddTabResult(m_pCurrentList, m_pStrip, tab);
        if (!pItem)
            continue;
        const bool fMatches = m_pCurrentSearch->matches(pItem->data(SearchCandidateRole).toString());
        if (!fMatches)
            delete m_pCurrentList->takeItem(m_pCurrentList->row(pItem));
        else
            ++cMatches;
    }
    if (!cMatches)
        addEmptyResult(m_pCurrentList, m_pStrip->tabs().isEmpty()
                     ? md3TabManagerText("md3.tab-manager.no-tabs", tr("No tabs are open in this scope."))
                     : md3TabManagerText("md3.tab-manager.no-match", tr("No tabs match this search.")));
}

void UIMd3TabManager::rebuildGroupPages()
{
    if (!m_pStrip || !m_pGroupStack || !m_pGroupList)
        return;
    for (auto it = m_groupSearches.constBegin(); it != m_groupSearches.constEnd(); ++it)
        if (it.value())
        {
            SearchState state;
            state.strText = it.value()->text();
            state.fRegex = it.value()->isRegexActive();
            state.strPattern = state.fRegex ? it.value()->regex().pattern() : QString();
            state.strFlags = state.fRegex ? it.value()->regexFlags() : QString();
            m_groupSearchStates.insert(it.key(), state);
        }
    while (m_pGroupStack->count())
    {
        QWidget *pPage = m_pGroupStack->widget(0);
        m_pGroupStack->removeWidget(pPage);
        pPage->deleteLater();
    }
    m_groupSearches.clear();
    m_groupLists.clear();
    m_pGroupList->clear();

    QList<UIMd3TabGroup> groups = m_pStrip->groups();
    UIMd3TabGroup ungrouped;
    ungrouped.strId = QString();
    ungrouped.strName = md3TabManagerText("md3.tab-manager.ungrouped", tr("No group (top level)"));
    ungrouped.color = md3(UIMd3ColorRole_SurfaceContainer);
    ungrouped.fCollapsed = false;
    groups.prepend(ungrouped);
    for (const UIMd3TabGroup &group : groups)
    {
        QListWidgetItem *pGroupItem = new QListWidgetItem(group.strName, m_pGroupList);
        pGroupItem->setData(GroupIdRole, group.strId);
        pGroupItem->setData(SearchCandidateRole,
                            group.strName + QLatin1Char(' ') + group.color.name(QColor::HexArgb));
        QWidget *pPage = new QWidget(m_pGroupStack);
        QVBoxLayout *pLayout = new QVBoxLayout(pPage);
        pLayout->setContentsMargins(0, 0, 0, 0);
        UIMd3SearchField *pSearch = new UIMd3SearchField(
            QStringLiteral("tab-manager-group/%1").arg(group.strId.isEmpty()
                                                        ? QStringLiteral("ungrouped") : group.strId),
            md3TabManagerText("md3.tab-manager.search-group", tr("Search tabs in %1")).arg(group.strName), pPage);
        QListWidget *pList = new QListWidget(pPage);
        pList->setAccessibleName(md3TabManagerText("md3.tab-manager.group-tabs",
                                                  tr("Tabs in group %1")).arg(group.strName));
        pList->setSelectionMode(QAbstractItemView::SingleSelection);
        pList->setContextMenuPolicy(Qt::CustomContextMenu);
        pLayout->addWidget(pSearch);
        pLayout->addWidget(pList, 1);
        m_pGroupStack->addWidget(pPage);
        m_groupSearches.insert(group.strId, pSearch);
        m_groupLists.insert(group.strId, pList);
        connect(pSearch, &UIMd3SearchField::sigFilterChanged, this,
                [this, group]() { refreshGroupRows(group.strId); });
        connect(pList, &QListWidget::itemActivated,
                this, &UIMd3TabManager::activateItem);
        connect(pList, &QListWidget::customContextMenuRequested, this,
                [this, pList](const QPoint &position) { showItemActions(pList, position); });
        const SearchState state = m_groupSearchStates.value(group.strId);
        if (state.fRegex)
            pSearch->applyRegex(state.strPattern, state.strFlags);
        else if (!state.strText.isEmpty())
            pSearch->setText(state.strText);
    }
    m_pGroupList->setCurrentRow(m_pGroupList->count() ? 0 : -1);
    refreshGroupNames();
    for (auto it = m_groupLists.constBegin(); it != m_groupLists.constEnd(); ++it)
        refreshGroupRows(it.key());
}

void UIMd3TabManager::refreshGroupNames()
{
    if (!m_pGroupList || !m_pGroupNameSearch || !m_pStrip)
        return;
    QListWidgetItem *pFirstVisible = 0;
    for (int i = 0; i < m_pGroupList->count(); ++i)
    {
        QListWidgetItem *pItem = m_pGroupList->item(i);
        const QString strGroupId = pItem->data(GroupIdRole).toString();
        const QString strName = md3GroupName(m_pStrip, strGroupId);
        int cMembers = 0;
        for (const UIMd3Tab &tab : m_pStrip->tabs())
            if (tab.strGroupId == strGroupId)
                ++cMembers;
        pItem->setText(QStringLiteral("%1 · %2").arg(strName,
            md3TabManagerText("md3.tab-manager.group-members", tr("%1 member(s)")).arg(cMembers)));
        pItem->setData(SearchCandidateRole, strName);
        const bool fMatches = m_pGroupNameSearch->matches(strName);
        pItem->setHidden(!fMatches);
        if (fMatches && !pFirstVisible)
            pFirstVisible = pItem;
    }
    if (!m_pGroupList->currentItem() || m_pGroupList->currentItem()->isHidden())
        m_pGroupList->setCurrentItem(pFirstVisible);
    m_pGroupStack->setVisible(pFirstVisible != 0);
}

void UIMd3TabManager::refreshGroupRows(const QString &strGroupId)
{
    QListWidget *pList = m_groupLists.value(strGroupId);
    UIMd3SearchField *pSearch = m_groupSearches.value(strGroupId);
    if (!pList || !pSearch || !m_pStrip)
        return;
    pList->clear();
    int cMembers = 0;
    int cMatches = 0;
    for (const UIMd3Tab &tab : m_pStrip->tabs())
        if (tab.strGroupId == strGroupId)
        {
            ++cMembers;
            QListWidgetItem *pItem = md3AddTabResult(pList, m_pStrip, tab);
            if (!pItem)
                continue;
            if (!pSearch->matches(pItem->data(SearchCandidateRole).toString()))
                delete pList->takeItem(pList->row(pItem));
            else
                ++cMatches;
        }
    if (!cMatches)
        addEmptyResult(pList, cMembers
                     ? md3TabManagerText("md3.tab-manager.no-match", tr("No tabs match this search."))
                     : md3TabManagerText("md3.tab-manager.no-tabs", tr("No tabs are open in this scope.")));
}

void UIMd3TabManager::refreshMasterRows()
{
    if (!m_pMasterList || !m_pMasterSearch)
        return;
    m_pMasterList->clear();
    int cTabs = 0;
    int cMatches = 0;
    for (const QPointer<UIMd3TabStrip> &pStrip : g_liveTabStrips)
        if (pStrip)
            for (const UIMd3Tab &tab : pStrip->tabs())
            {
                ++cTabs;
                QListWidgetItem *pItem = md3AddTabResult(m_pMasterList, pStrip, tab);
                if (!pItem)
                    continue;
                if (!m_pMasterSearch->matches(pItem->data(SearchCandidateRole).toString()))
                    delete m_pMasterList->takeItem(m_pMasterList->row(pItem));
                else
                    ++cMatches;
            }
    if (!cMatches)
        addEmptyResult(m_pMasterList, cTabs
                     ? md3TabManagerText("md3.tab-manager.no-match", tr("No tabs match this search."))
                     : md3TabManagerText("md3.tab-manager.no-tabs", tr("No tabs are open in this scope.")));
}

void UIMd3TabManager::activateItem(QListWidgetItem *pItem)
{
    if (!pItem || pItem->data(TabIdRole).toString().isEmpty())
        return;
    if (!pItem->data(TabEnabledRole).toBool())
    {
        QToolTip::showText(QCursor::pos(), pItem->toolTip(), this);
        return;
    }
    UIMd3TabStrip *pStrip = md3FindStrip(pItem->data(StripTokenRole).toString());
    if (!pStrip)
        return;
    pStrip->setCurrentTabId(pItem->data(TabIdRole).toString());
    pStrip->setFocus(Qt::OtherFocusReason);
    QWidget *pWindow = pStrip->window();
    close();
    if (pWindow)
    {
        pWindow->show();
        pWindow->raise();
        pWindow->activateWindow();
    }
}

void UIMd3TabManager::showItemActions(QListWidget *pList, const QPoint &position)
{
    if (!pList)
        return;
    QListWidgetItem *pItem = pList->itemAt(position);
    if (!pItem || pItem->data(TabIdRole).toString().isEmpty())
        return;
    UIMd3TabStrip *pStrip = md3FindStrip(pItem->data(StripTokenRole).toString());
    if (pStrip)
        pStrip->showTabActions(pItem->data(TabIdRole).toString(), pList->viewport()->mapToGlobal(position));
}

void UIMd3TabManager::invalidateClosePreview(const QString &strReason)
{
    m_pendingCloseIds.clear();
    m_pPendingSearch = 0;
    if (m_pApplyClose)
    {
        m_pApplyClose->setEnabled(false);
        m_pApplyClose->setAccessibleDescription(QString());
    }
    if (m_pClosePreview)
        m_pClosePreview->clear();
    if (m_pCloseStatus)
    {
        const QString strStatus = strReason.isEmpty()
            ? md3TabManagerText("md3.tab-manager.preview-required",
                                tr("Preview the current query before closing any tabs."))
            : strReason;
        if (strReason.isEmpty())
        {
            m_pCloseStatus->setText(strStatus);
            m_pCloseStatus->setToolTip(strStatus);
            m_pCloseStatus->setAccessibleDescription(strStatus);
        }
        else
            md3AnnounceTabManagerStatus(m_pCloseStatus, strStatus);
    }
}

void UIMd3TabManager::previewClose(UIMd3SearchField *pSearch, bool fInverse)
{
    if (!m_pStrip || !pSearch)
        return;
    invalidateClosePreview();
    const QString strQuery = pSearch->text();
    if (strQuery.trimmed().isEmpty())
    {
        md3AnnounceTabManagerStatus(m_pCloseStatus,
            md3TabManagerText("md3.tab-manager.query-required",
            tr("Enter a non-empty plain-text query or apply a valid regular expression before previewing.")));
        pSearch->setFocus(Qt::OtherFocusReason);
        return;
    }
    const bool fRegex = pSearch->isRegexActive();
    const QString strFlags = fRegex ? pSearch->regexFlags() : QString();
    const bool fIncludePinned = m_pIncludePinned->isChecked();
    const QList<UIMd3Tab> matches = m_pStrip->resolveCloseSet(strQuery, fInverse, fRegex,
                                                              fIncludePinned, strFlags);
    if (matches.isEmpty())
    {
        md3AnnounceTabManagerStatus(m_pCloseStatus,
            md3TabManagerText("md3.tab-manager.preview-none",
                              tr("The current query matches no closable tabs.")));
        pSearch->setFocus(Qt::OtherFocusReason);
        return;
    }
    for (const UIMd3Tab &tab : matches)
    {
        const QString strPin = tab.fPinned
            ? md3TabManagerText("md3.tab-manager.pinned", tr("Pinned"))
            : md3TabManagerText("md3.tab-manager.unpinned", tr("Not pinned"));
        QListWidgetItem *pItem = new QListWidgetItem(
            QStringLiteral("%1 · %2 · %3").arg(tab.strLabel, md3GroupName(m_pStrip, tab.strGroupId), strPin),
            m_pClosePreview);
        pItem->setData(TabIdRole, tab.strId);
        m_pendingCloseIds << tab.strId;
    }
    m_pPendingSearch = pSearch;
    m_strPendingQuery = strQuery;
    m_strPendingFlags = strFlags;
    m_fPendingInverse = fInverse;
    m_fPendingRegex = fRegex;
    m_fPendingIncludePinned = fIncludePinned;
    md3AnnounceTabManagerStatus(m_pCloseStatus,
        md3TabManagerText("md3.tab-manager.preview-summary",
                          tr("%1 tab(s) will close. Review every row, then use Close reviewed tabs."))
        .arg(matches.size()));
    m_pApplyClose->setEnabled(true);
    m_pApplyClose->setAccessibleDescription(m_pCloseStatus->text());
    m_pCloseStatus->setFocus(Qt::OtherFocusReason);
}

void UIMd3TabManager::sltPreviewContaining()
{
    previewClose(m_pCloseContaining, false);
}

void UIMd3TabManager::sltPreviewNotContaining()
{
    previewClose(m_pCloseNotContaining, true);
}

void UIMd3TabManager::sltApplyClose()
{
    if (!m_pStrip || !m_pPendingSearch || m_pendingCloseIds.isEmpty())
        return;
    const QList<UIMd3Tab> currentMatches = m_pStrip->resolveCloseSet(
        m_pPendingSearch->text(), m_fPendingInverse, m_pPendingSearch->isRegexActive(),
        m_pIncludePinned->isChecked(), m_pPendingSearch->regexFlags());
    QStringList currentIds;
    for (const UIMd3Tab &tab : currentMatches)
        currentIds << tab.strId;
    QStringList expectedIds = m_pendingCloseIds;
    currentIds.sort();
    expectedIds.sort();
    if (   currentIds != expectedIds
        || m_pPendingSearch->text() != m_strPendingQuery
        || m_pPendingSearch->isRegexActive() != m_fPendingRegex
        || m_pPendingSearch->regexFlags() != m_strPendingFlags
        || m_pIncludePinned->isChecked() != m_fPendingIncludePinned)
    {
        invalidateClosePreview(md3TabManagerText("md3.tab-manager.preview-changed",
            tr("The tab model or close options changed. Preview again before closing anything.")));
        m_pCloseStatus->setFocus(Qt::OtherFocusReason);
        return;
    }
    const QStringList closeIds = m_pendingCloseIds;
    const bool fIncludePinned = m_fPendingIncludePinned;
    m_pendingCloseIds.clear();
    m_pPendingSearch = 0;
    m_pApplyClose->setEnabled(false);
    int cClosed = 0;
    for (const QString &strId : closeIds)
        if (m_pStrip && m_pStrip->closeTab(strId, fIncludePinned))
            ++cClosed;
    m_pClosePreview->clear();
    md3AnnounceTabManagerStatus(m_pCloseStatus,
        md3TabManagerText("md3.tab-manager.closed",
            tr("Closed %1 reviewed tab(s); %2 tab(s) were skipped because their state changed."))
        .arg(cClosed).arg(closeIds.size() - cClosed));
    m_pCloseStatus->setFocus(Qt::OtherFocusReason);
}

void UIMd3TabManager::sltCreateGroup()
{
    if (!m_pStrip || !m_pNewGroupName)
        return;
    const QString strName = m_pNewGroupName->text().trimmed().left(80);
    if (strName.isEmpty())
    {
        m_pNewGroupName->setFocus(Qt::OtherFocusReason);
        return;
    }
    const QString strGroupId = m_pStrip->createGroup(strName);
    if (strGroupId.isEmpty())
        return;
    m_pNewGroupName->clear();
    sltRefresh();
    for (int i = 0; i < m_pGroupList->count(); ++i)
        if (m_pGroupList->item(i)->data(GroupIdRole).toString() == strGroupId)
        {
            m_pGroupList->setCurrentRow(i);
            break;
        }
}

void UIMd3TabManager::sltRefresh()
{
    if (!m_pStrip)
    {
        close();
        return;
    }
    QStringList signatureParts;
    for (const UIMd3TabGroup &group : m_pStrip->groups())
        signatureParts << QStringLiteral("%1|%2|%3|%4")
                          .arg(group.strId, group.strName, group.color.name(QColor::HexArgb))
                          .arg(group.fCollapsed);
    const QString strSignature = signatureParts.join(QLatin1Char('\n'));
    refreshCurrentRows();
    if (m_groupLists.isEmpty() || strSignature != m_strGroupSignature)
    {
        m_strGroupSignature = strSignature;
        rebuildGroupPages();
    }
    else
    {
        refreshGroupNames();
        for (auto it = m_groupLists.constBegin(); it != m_groupLists.constEnd(); ++it)
            refreshGroupRows(it.key());
    }
    refreshMasterRows();
    if (!m_pendingCloseIds.isEmpty())
        invalidateClosePreview(md3TabManagerText("md3.tab-manager.preview-changed",
            tr("The tab model or close options changed. Preview again before closing anything.")));
}
