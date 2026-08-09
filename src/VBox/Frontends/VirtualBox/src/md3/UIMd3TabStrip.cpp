/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 browser-style tab strip with groups and pinning.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QAbstractItemView>
#include <QAccessibleWidget>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFontMetrics>
#include <QHash>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScreen>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QUuid>

#include "UIExtraDataManager.h"
#include "UIMd3AppearanceEditor.h"
#include "UIMd3Language.h"
#include "UIMd3SearchField.h"
#include "UIMd3TabStrip.h"
#include "UIMd3Theme.h"

static const char *g_pszTabsExtraData = "GUI/Md3/Tabs";

static void md3RegisterTabText()
{
    UIMd3Language *pLanguage = UIMd3Language::instance();
    if (!pLanguage)
        return;
#define REGISTER_TAB_TEXT(a_pszKey, a_pszEnglish, a_pszCantonese) \
    pLanguage->registerText(QStringLiteral(a_pszKey), QStringLiteral(a_pszEnglish), QStringLiteral(a_pszCantonese))
    REGISTER_TAB_TEXT("md3.tabs.workspace", "Workspace tabs", "工作區分頁");
    REGISTER_TAB_TEXT("md3.tabs.show-more", "Show more tabs", "顯示更多分頁");
    REGISTER_TAB_TEXT("md3.tabs.show-more-description", "Open the searchable list of tabs that do not fit", "開啟可搜尋嘅未顯示分頁清單");
    REGISTER_TAB_TEXT("md3.tabs.new", "New workspace tab", "新增工作區分頁");
    REGISTER_TAB_TEXT("md3.tabs.new-description", "Choose a manager destination to open in a tab", "揀一個管理員目的地喺分頁開啟");
    REGISTER_TAB_TEXT("md3.tabs.manager", "Tab manager", "分頁管理員");
    REGISTER_TAB_TEXT("md3.tabs.manager-description", "Search and activate every open workspace tab", "搜尋同啟用所有已開工作區分頁");
    REGISTER_TAB_TEXT("md3.tabs.navigation-hint", "Use Left and Right to change the selected tab.", "用向左同向右鍵切換所選分頁。");
    REGISTER_TAB_TEXT("md3.tabs.selected", "Selected tab: %1. Use Left and Right to change the selected tab.", "已選分頁：%1。用向左同向右鍵切換。");
    REGISTER_TAB_TEXT("md3.tabs.page-description", "Workspace tab %1", "工作區分頁 %1");
    REGISTER_TAB_TEXT("md3.tabs.close-named", "Close tab %1", "關閉分頁 %1");
    REGISTER_TAB_TEXT("md3.tabs.search", "Search tabs", "搜尋分頁");
    REGISTER_TAB_TEXT("md3.tabs.overflow-menu", "Tabs that do not fit", "未能顯示嘅分頁");
    REGISTER_TAB_TEXT("md3.tabs.open-named", "Open tab %1", "開啟分頁 %1");
    REGISTER_TAB_TEXT("md3.tabs.unavailable", "Tab %1 is unavailable under the current manager restrictions", "按目前管理員限制，分頁 %1 暫時不可用");
    REGISTER_TAB_TEXT("md3.tabs.new-menu", "New workspace tab destinations", "新增工作區分頁目的地");
    REGISTER_TAB_TEXT("md3.tabs.search-destinations", "Search destinations", "搜尋目的地");
    REGISTER_TAB_TEXT("md3.tabs.open-destination", "Open %1 in a workspace tab", "喺工作區分頁開啟 %1");
    REGISTER_TAB_TEXT("md3.tabs.destination-unavailable", "%1 is unavailable under the current manager restrictions", "按目前管理員限制，%1 暫時不可用");
    REGISTER_TAB_TEXT("md3.tabs.destinations-open", "All available destinations are already open", "所有可用目的地已經開啟");
    REGISTER_TAB_TEXT("md3.tabs.search-all", "Search all open tabs", "搜尋所有已開分頁");
    REGISTER_TAB_TEXT("md3.tabs.activate-named", "Activate workspace tab %1", "啟用工作區分頁 %1");
    REGISTER_TAB_TEXT("md3.tabs.none-open", "No workspace tabs are open", "未有工作區分頁開啟");
    REGISTER_TAB_TEXT("md3.tabs.group.move-title", "Move tab into group", "將分頁移入群組");
    REGISTER_TAB_TEXT("md3.tabs.group.choose", "Choose an existing group or create one", "揀現有群組或者新增一個");
    REGISTER_TAB_TEXT("md3.tabs.group.search", "Search groups", "搜尋群組");
    REGISTER_TAB_TEXT("md3.tabs.group.empty", "No groups yet. Create one below.", "未有群組，可以喺下面新增。");
    REGISTER_TAB_TEXT("md3.tabs.group.no-match", "No groups match the current search.", "冇群組符合目前搜尋。");
    REGISTER_TAB_TEXT("md3.tabs.group.targets", "Move target groups", "移動目的地群組");
    REGISTER_TAB_TEXT("md3.tabs.group.new-name", "New group name", "新群組名稱");
    REGISTER_TAB_TEXT("md3.tabs.group.create", "Create group", "新增群組");
    REGISTER_TAB_TEXT("md3.tabs.group.name-required", "Enter a group name from 1 to 80 characters.", "輸入 1 至 80 個字元嘅群組名稱。");
    REGISTER_TAB_TEXT("md3.tabs.group.move", "Move", "移動");
    REGISTER_TAB_TEXT("md3.tabs.group.move-selected", "Move tab into selected group", "將分頁移入所選群組");
    REGISTER_TAB_TEXT("md3.tabs.group.cancel-move", "Cancel moving tab", "取消移動分頁");
    REGISTER_TAB_TEXT("md3.tabs.group.none", "No group (top level)", "無群組（頂層）");
    REGISTER_TAB_TEXT("md3.tabs.group.keep-top", "Keep this tab outside a group", "將呢個分頁保留喺群組之外");
    REGISTER_TAB_TEXT("md3.tabs.group.members", "%1 members", "%1 個成員");
    REGISTER_TAB_TEXT("md3.tabs.group.summary", "%1, color %2, %3 members", "%1，顏色 %2，%3 個成員");
    REGISTER_TAB_TEXT("md3.tabs.strip-actions", "Tab strip actions", "分頁列動作");
    REGISTER_TAB_TEXT("md3.tabs.search-strip", "Search tab strip", "搜尋分頁列");
    REGISTER_TAB_TEXT("md3.tabs.group.create-ellipsis", "Create group…", "新增群組…");
    REGISTER_TAB_TEXT("md3.tabs.group.create-description", "Create a new tab group", "新增一個分頁群組");
    REGISTER_TAB_TEXT("md3.tabs.group.create-help", "Create a new tab group with a name from 1 to 80 characters", "新增一個名稱為 1 至 80 個字元嘅分頁群組");
    REGISTER_TAB_TEXT("md3.tabs.group.expand", "Expand %1", "展開 %1");
    REGISTER_TAB_TEXT("md3.tabs.group.collapse", "Collapse %1", "收合 %1");
    REGISTER_TAB_TEXT("md3.tabs.group.toggle", "Toggle group %1", "切換群組 %1");
    REGISTER_TAB_TEXT("md3.tabs.group.toggle-help", "Expand or collapse tab group %1", "展開或收合分頁群組 %1");
    REGISTER_TAB_TEXT("md3.tabs.group.rename-named", "Rename group: %1", "重新命名群組：%1");
    REGISTER_TAB_TEXT("md3.tabs.group.rename-description", "Rename tab group %1", "重新命名分頁群組 %1");
    REGISTER_TAB_TEXT("md3.tabs.group.rename-help", "Rename tab group %1; the name is limited to 80 characters", "重新命名分頁群組 %1；名稱最多 80 個字元");
    REGISTER_TAB_TEXT("md3.tabs.group.edit-named", "Edit group appearance…: %1", "編輯群組外觀…：%1");
    REGISTER_TAB_TEXT("md3.tabs.group.edit-description", "Edit appearance for tab group %1", "編輯分頁群組 %1 嘅外觀");
    REGISTER_TAB_TEXT("md3.tabs.group.rename", "Rename group", "重新命名群組");
    REGISTER_TAB_TEXT("md3.tabs.group.rename-accessible", "Rename tab group", "重新命名分頁群組");
    REGISTER_TAB_TEXT("md3.tabs.group.name", "Group name", "群組名稱");
    REGISTER_TAB_TEXT("md3.tabs.group.rename-button", "Rename", "重新命名");
    REGISTER_TAB_TEXT("md3.tabs.cancel", "Cancel", "取消");
    REGISTER_TAB_TEXT("md3.tabs.group.name-help", "Enter 1 to 80 characters for the group name.", "輸入 1 至 80 個字元作為群組名稱。");
    REGISTER_TAB_TEXT("md3.tabs.group.create-accessible", "Create tab group", "新增分頁群組");
    REGISTER_TAB_TEXT("md3.tabs.group.create-button", "Create", "新增");
    REGISTER_TAB_TEXT("md3.tabs.edit-strip", "Edit appearance…", "編輯外觀…");
    REGISTER_TAB_TEXT("md3.tabs.edit-strip-description", "Edit appearance for the tab strip", "編輯分頁列外觀");
    REGISTER_TAB_TEXT("md3.tabs.search-actions", "Search tab actions", "搜尋分頁動作");
    REGISTER_TAB_TEXT("md3.tabs.unpin", "Unpin tab", "取消固定分頁");
    REGISTER_TAB_TEXT("md3.tabs.pin", "Pin tab", "固定分頁");
    REGISTER_TAB_TEXT("md3.tabs.move-ellipsis", "Move… into group…", "移動…入群組…");
    REGISTER_TAB_TEXT("md3.tabs.close", "Close tab", "關閉分頁");
    REGISTER_TAB_TEXT("md3.tabs.edit-tab", "Edit tab appearance…", "編輯分頁外觀…");
    REGISTER_TAB_TEXT("md3.tabs.actions", "Tab actions", "分頁動作");
    REGISTER_TAB_TEXT("md3.tabs.unpin-before-close", "Unpin this tab before closing it", "關閉前先取消固定呢個分頁");
    REGISTER_TAB_TEXT("md3.tabs.edit-tab-named", "Edit appearance for %1", "編輯 %1 嘅外觀");
    REGISTER_TAB_TEXT("md3.tabs.edit-tab-help", "Edit the appearance of tab %1", "編輯分頁 %1 嘅外觀");
#undef REGISTER_TAB_TEXT
}

static QString md3TabText(const char *pszKey, const QString &strFallback)
{
    if (!UIMd3Language::instance())
        return strFallback;
    const QString strKey = QString::fromLatin1(pszKey);
    const QString strText = md3Text(strKey);
    return strText == strKey || strText.isEmpty() ? strFallback : strText;
}

/** Page-tab-list semantics for the dedicated tab-only child container. */
class UIAccessibilityInterfaceForUIMd3TabList : public QAccessibleWidget
{
public:

    explicit UIAccessibilityInterfaceForUIMd3TabList(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::PageTabList)
    {}
};

/** Page-tab semantics for the transparent, real child controls which expose
  * the painted tab model to keyboard and assistive-technology users. */
class UIAccessibilityInterfaceForUIMd3PageTab : public QAccessibleWidget
{
public:

    static QAccessibleInterface *pFactory(const QString &strClassname, QObject *pObject)
    {
        if (   pObject
            && strClassname == QLatin1String("QToolButton")
            && pObject->property("md3PageTab").toBool())
            return new UIAccessibilityInterfaceForUIMd3PageTab(qobject_cast<QWidget*>(pObject));
        if (pObject && pObject->property("md3PageTabList").toBool())
            return new UIAccessibilityInterfaceForUIMd3TabList(qobject_cast<QWidget*>(pObject));
        return 0;
    }

    explicit UIAccessibilityInterfaceForUIMd3PageTab(QWidget *pWidget)
        : QAccessibleWidget(pWidget, QAccessible::PageTab)
    {}

    virtual QStringList actionNames() const RT_OVERRIDE
    {
        return QStringList(QAccessibleActionInterface::pressAction());
    }

    virtual QAccessible::State state() const RT_OVERRIDE
    {
        QAccessible::State myState = QAccessibleWidget::state();
        const QToolButton *pButton = qobject_cast<const QToolButton*>(widget());
        myState.selectable = true;
        myState.selected = pButton && pButton->isChecked();
        return myState;
    }

    virtual void doAction(const QString &strActionName) RT_OVERRIDE
    {
        QToolButton *pButton = qobject_cast<QToolButton*>(widget());
        if (   strActionName == QAccessibleActionInterface::pressAction()
            && pButton
            && pButton->isEnabled())
            pButton->click();
    }

    virtual QStringList keyBindingsForAction(const QString &strActionName) const RT_OVERRIDE
    {
        if (strActionName != QAccessibleActionInterface::pressAction())
            return QStringList();
        return QStringList() << QKeySequence(Qt::Key_Space).toString(QKeySequence::NativeText)
                             << QKeySequence(Qt::Key_Return).toString(QKeySequence::NativeText);
    }
};

UIMd3TabStrip::UIMd3TabStrip(QWidget *pParent)
    : UIMd3Widget(pParent, QStringLiteral("tab-strip"))
    , m_fRestoredLegacyPersistence(false)
    , m_iFirstVisiblePinned(0)
    , m_iFirstVisibleUnpinned(0)
    , m_pTabList(0)
    , m_pOverflowButton(0)
    , m_pNewTabButton(0)
    , m_pTabManagerButton(0)
{
    static bool s_fAccessibilityFactoryInstalled = false;
    if (!s_fAccessibilityFactoryInstalled)
    {
        QAccessible::installFactory(UIAccessibilityInterfaceForUIMd3PageTab::pFactory);
        s_fAccessibilityFactoryInstalled = true;
    }
    md3RegisterTabText();
    setObjectName(QStringLiteral("md3TabStrip"));
    setFocusPolicy(Qt::StrongFocus);
    setFixedHeight(48);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pTabList = new QWidget(this);
    m_pTabList->setObjectName(QStringLiteral("md3PageTabList"));
    m_pTabList->setProperty("md3PageTabList", true);
    m_pTabList->setAttribute(Qt::WA_TranslucentBackground);
    m_pTabList->setFocusPolicy(Qt::NoFocus);
    m_pOverflowButton = new QToolButton(this);
    m_pOverflowButton->setText(QStringLiteral("…"));
    m_pOverflowButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_pOverflowButton->setAutoRaise(true);
    m_pOverflowButton->setFixedSize(QSize(48, 48));
    connect(m_pOverflowButton, &QToolButton::clicked, this, &UIMd3TabStrip::showOverflowMenu);

    m_pNewTabButton = new QToolButton(this);
    m_pNewTabButton->setObjectName(QStringLiteral("md3NewTabButton"));
    m_pNewTabButton->setText(QStringLiteral("+"));
    m_pNewTabButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_pNewTabButton->setAutoRaise(true);
    m_pNewTabButton->setFixedSize(QSize(48, 48));
    connect(m_pNewTabButton, &QToolButton::clicked, this, &UIMd3TabStrip::showNewTabMenu);

    m_pTabManagerButton = new QToolButton(this);
    m_pTabManagerButton->setObjectName(QStringLiteral("md3TabManagerButton"));
    m_pTabManagerButton->setText(QString(QChar(0x2261)));
    m_pTabManagerButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_pTabManagerButton->setAutoRaise(true);
    m_pTabManagerButton->setFixedSize(QSize(48, 48));
    connect(m_pTabManagerButton, &QToolButton::clicked, this, &UIMd3TabStrip::showTabManagerMenu);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged, this, [this]()
    {
        updateTheme();
    });
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3TabStrip::retranslateUi, Qt::UniqueConnection);
    restore();
    retranslateUi();
    updateTheme();
    updateOverflowButton();
}

void UIMd3TabStrip::retranslateUi()
{
    const QString strWorkspaceTabs = md3TabText("md3.tabs.workspace", tr("Workspace tabs"));
    setAccessibleName(strWorkspaceTabs);
    if (m_pTabList)
        m_pTabList->setAccessibleName(strWorkspaceTabs);
    if (m_pOverflowButton)
    {
        const QString strShowMore = md3TabText("md3.tabs.show-more", tr("Show more tabs"));
        m_pOverflowButton->setAccessibleName(strShowMore);
        m_pOverflowButton->setAccessibleDescription(
            md3TabText("md3.tabs.show-more-description",
                       tr("Open the searchable list of tabs that do not fit")));
        m_pOverflowButton->setToolTip(strShowMore);
    }
    if (m_pNewTabButton)
    {
        const QString strNewTab = md3TabText("md3.tabs.new", tr("New workspace tab"));
        m_pNewTabButton->setAccessibleName(strNewTab);
        m_pNewTabButton->setAccessibleDescription(
            md3TabText("md3.tabs.new-description",
                       tr("Choose a manager destination to open in a tab")));
        m_pNewTabButton->setToolTip(strNewTab);
    }
    if (m_pTabManagerButton)
    {
        const QString strManager = md3TabText("md3.tabs.manager", tr("Tab manager"));
        m_pTabManagerButton->setAccessibleName(strManager);
        m_pTabManagerButton->setAccessibleDescription(
            md3TabText("md3.tabs.manager-description",
                       tr("Search and activate every open workspace tab")));
        m_pTabManagerButton->setToolTip(strManager);
    }
    QString strDescription = md3TabText("md3.tabs.navigation-hint",
                                        tr("Use Left and Right to change the selected tab."));
    for (const UIMd3Tab &tab : m_tabs)
        if (tab.strId == m_strCurrentId)
        {
            strDescription = md3TabText("md3.tabs.selected",
                                        tr("Selected tab: %1. Use Left and Right to change the selected tab."))
                             .arg(tab.strLabel);
            break;
        }
    setAccessibleDescription(strDescription);
    syncAccessibleTabButtons();
}

QList<UIMd3Tab> UIMd3TabStrip::displayTabs() const
{
    QList<UIMd3Tab> result;
    for (const UIMd3Tab &tab : m_tabs)
    {
        bool fCollapsed = false;
        for (const UIMd3TabGroup &group : m_groups)
            if (group.strId == tab.strGroupId)
            {
                fCollapsed = group.fCollapsed;
                break;
            }
        if ((!fCollapsed || tab.strId == m_strCurrentId) && tab.fPinned)
            result << tab;
    }
    for (const UIMd3Tab &tab : m_tabs)
    {
        bool fCollapsed = false;
        for (const UIMd3TabGroup &group : m_groups)
            if (group.strId == tab.strGroupId)
            {
                fCollapsed = group.fCollapsed;
                break;
            }
        if ((!fCollapsed || tab.strId == m_strCurrentId) && !tab.fPinned)
            result << tab;
    }
    return result;
}

QList<UIMd3Tab> UIMd3TabStrip::unpinnedDisplayTabs() const
{
    QList<UIMd3Tab> result;
    for (const UIMd3Tab &tab : displayTabs())
        if (!tab.fPinned)
            result << tab;
    return result;
}

void UIMd3TabStrip::announceModelChanged()
{
    m_fRestoredLegacyPersistence = false;
    int cPinnedTabs = 0;
    for (const UIMd3Tab &tab : displayTabs())
        if (tab.fPinned)
            ++cPinnedTabs;
    m_iFirstVisiblePinned = qBound(0, m_iFirstVisiblePinned,
                                  qMax(0, cPinnedTabs - 1));
    m_iFirstVisibleUnpinned = qBound(0, m_iFirstVisibleUnpinned,
                                    qMax(0, unpinnedDisplayTabs().size() - 1));
    if (!m_strCurrentId.isEmpty())
        ensureTabVisible(m_strCurrentId);
    save();
    emit sigModelChanged();
    updateGeometry();
    update();
    updateOverflowButton();
}

void UIMd3TabStrip::openTab(const QString &strId, const QString &strLabel)
{
    const QString strTrimmedId = strId.trimmed();
    if (strTrimmedId.isEmpty())
        return;
    for (int i = 0; i < m_tabs.size(); ++i)
        if (m_tabs.at(i).strId == strTrimmedId)
        {
            const QString strBoundedLabel = strLabel.trimmed().left(160);
            const bool fLabelChanged = !strBoundedLabel.isEmpty() && m_tabs.at(i).strLabel != strBoundedLabel;
            if (fLabelChanged)
                m_tabs[i].strLabel = strBoundedLabel;
            if (fLabelChanged)
                announceModelChanged();
            setCurrentTabId(strTrimmedId);
            return;
        }
    UIMd3Tab tab;
    tab.strId = strTrimmedId;
    tab.strLabel = strLabel.trimmed().left(160);
    tab.fPinned = false;
    tab.fEnabled = true;
    m_tabs << tab;
    m_strCurrentId = strTrimmedId;
    ensureTabVisible(m_strCurrentId);
    announceModelChanged();
    emit sigCurrentChanged(m_strCurrentId);
}

void UIMd3TabStrip::setTabLabel(const QString &strId, const QString &strLabel)
{
    const QString strBoundedLabel = strLabel.trimmed().left(160);
    if (strBoundedLabel.isEmpty())
        return;
    for (UIMd3Tab &tab : m_tabs)
        if (tab.strId == strId && tab.strLabel != strBoundedLabel)
        {
            tab.strLabel = strBoundedLabel;
            if (tab.strId == m_strCurrentId)
                setAccessibleDescription(md3TabText("md3.tabs.selected",
                                         tr("Selected tab: %1. Use Left and Right to change the selected tab."))
                                         .arg(tab.strLabel));
            announceModelChanged();
            return;
        }
}

void UIMd3TabStrip::setTabEnabled(const QString &strId, bool fEnabled)
{
    QHash<QString, bool> states;
    states.insert(strId, fEnabled);
    setTabsEnabled(states);
}

void UIMd3TabStrip::setTabsEnabled(const QHash<QString, bool> &states)
{
    bool fChanged = false;
    const QString strPreviousId = m_strCurrentId;
    for (UIMd3Tab &tab : m_tabs)
        if (states.contains(tab.strId) && tab.fEnabled != states.value(tab.strId))
        {
            tab.fEnabled = states.value(tab.strId);
            fChanged = true;
        }

    bool fCurrentAvailable = false;
    for (const UIMd3Tab &tab : displayTabs())
        if (tab.strId == m_strCurrentId && tab.fEnabled)
        {
            fCurrentAvailable = true;
            break;
        }
    if (!fCurrentAvailable)
    {
        m_strCurrentId.clear();
        for (const UIMd3Tab &tab : displayTabs())
            if (tab.fEnabled)
            {
                m_strCurrentId = tab.strId;
                break;
            }
    }

    if (!fChanged && strPreviousId == m_strCurrentId)
        return;
    announceModelChanged();
    if (strPreviousId != m_strCurrentId)
        emit sigCurrentChanged(m_strCurrentId);
}

void UIMd3TabStrip::setAvailableTabs(const QList<UIMd3TabChoice> &choices)
{
    QList<UIMd3TabChoice> boundedChoices;
    for (const UIMd3TabChoice &choice : choices)
    {
        UIMd3TabChoice boundedChoice;
        boundedChoice.strId = choice.strId.trimmed().left(128);
        boundedChoice.strLabel = choice.strLabel.trimmed().left(160);
        boundedChoice.fEnabled = choice.fEnabled;
        bool fDuplicate = false;
        for (const UIMd3TabChoice &existing : boundedChoices)
            fDuplicate |= existing.strId == boundedChoice.strId;
        if (   !fDuplicate
            && !boundedChoice.strId.isEmpty()
            && !boundedChoice.strLabel.isEmpty()
            && boundedChoices.size() < 256)
            boundedChoices << boundedChoice;
    }
    m_availableTabs = boundedChoices;
    const QString strPreviousCurrentId = m_strCurrentId;
    bool fModelChanged = false;
    for (UIMd3Tab &tab : m_tabs)
        for (const UIMd3TabChoice &choice : m_availableTabs)
            if (tab.strId == choice.strId)
            {
                if (tab.strLabel != choice.strLabel)
                {
                    tab.strLabel = choice.strLabel;
                    fModelChanged = true;
                }
                if (tab.fEnabled != choice.fEnabled)
                {
                    tab.fEnabled = choice.fEnabled;
                    fModelChanged = true;
                }
                break;
            }
    bool fCurrentAvailable = false;
    for (const UIMd3Tab &tab : displayTabs())
        if (tab.strId == m_strCurrentId && tab.fEnabled)
        {
            fCurrentAvailable = true;
            break;
        }
    if (!fCurrentAvailable)
    {
        m_strCurrentId.clear();
        for (const UIMd3Tab &tab : displayTabs())
            if (tab.fEnabled)
            {
                m_strCurrentId = tab.strId;
                break;
            }
    }
    if (m_pNewTabButton)
        m_pNewTabButton->setEnabled(!m_availableTabs.isEmpty());
    if (fModelChanged || strPreviousCurrentId != m_strCurrentId)
    {
        announceModelChanged();
        retranslateUi();
        if (strPreviousCurrentId != m_strCurrentId)
            emit sigCurrentChanged(m_strCurrentId);
    }
}

bool UIMd3TabStrip::migrateLegacyGeneratedLayout(const QStringList &expectedIds,
                                                  const QString &strInitialId,
                                                  const QString &strInitialLabel)
{
    if (   !m_fRestoredLegacyPersistence
        || !m_groups.isEmpty()
        || expectedIds.size() != m_tabs.size()
        || expectedIds.isEmpty())
        return false;
    for (int i = 0; i < expectedIds.size(); ++i)
    {
        const UIMd3Tab &tab = m_tabs.at(i);
        if (   tab.strId != expectedIds.at(i)
            || tab.fPinned
            || !tab.strGroupId.isEmpty())
            return false;
    }
    if (   !expectedIds.contains(strInitialId)
        || strInitialLabel.trimmed().isEmpty())
        return false;

    const QString strPreviousId = m_strCurrentId;
    UIMd3Tab initialTab;
    initialTab.strId = strInitialId;
    initialTab.strLabel = strInitialLabel.trimmed().left(160);
    initialTab.fPinned = true;
    initialTab.fEnabled = true;
    m_tabs = QList<UIMd3Tab>() << initialTab;
    m_groups.clear();
    m_strCurrentId = initialTab.strId;
    announceModelChanged();
    if (strPreviousId != m_strCurrentId)
        emit sigCurrentChanged(m_strCurrentId);
    return true;
}

bool UIMd3TabStrip::closeTab(const QString &strId, bool fForce)
{
    for (int i = 0; i < m_tabs.size(); ++i)
        if (m_tabs.at(i).strId == strId)
        {
            if (m_tabs.at(i).fPinned && !fForce)
                return false;
            const QString strPreviousCurrentId = m_strCurrentId;
            const QList<UIMd3Tab> aBefore = displayTabs();
            int iDisplayIndex = 0;
            for (int j = 0; j < aBefore.size(); ++j)
                if (aBefore.at(j).strId == strId)
                {
                    iDisplayIndex = j;
                    break;
                }
            const bool fCurrent = m_tabs.at(i).strId == m_strCurrentId;
            QString strFallbackId;
            if (fCurrent)
            {
                for (int j = iDisplayIndex + 1; j < aBefore.size() && strFallbackId.isEmpty(); ++j)
                    if (aBefore.at(j).fEnabled)
                        strFallbackId = aBefore.at(j).strId;
                for (int j = iDisplayIndex - 1; j >= 0 && strFallbackId.isEmpty(); --j)
                    if (aBefore.at(j).fEnabled)
                        strFallbackId = aBefore.at(j).strId;
            }
            m_tabs.removeAt(i);
            if (fCurrent)
                m_strCurrentId = strFallbackId;
            m_iFirstVisibleUnpinned = qBound(0, m_iFirstVisibleUnpinned,
                                            qMax(0, unpinnedDisplayTabs().size() - 1));
            if (!m_strCurrentId.isEmpty())
                ensureTabVisible(m_strCurrentId);
            announceModelChanged();
            if (strPreviousCurrentId != m_strCurrentId)
                emit sigCurrentChanged(m_strCurrentId);
            return true;
        }
    return false;
}

void UIMd3TabStrip::setCurrentTabId(const QString &strId)
{
    for (const UIMd3Tab &tab : m_tabs)
        if (tab.strId == strId)
        {
            if (!tab.fEnabled)
                return;
            if (m_strCurrentId == strId)
            {
                ensureTabVisible(strId);
                return;
            }
            m_strCurrentId = strId;
            ensureTabVisible(strId);
            save();
            setAccessibleDescription(md3TabText("md3.tabs.selected",
                                     tr("Selected tab: %1. Use Left and Right to change the selected tab."))
                                     .arg(tab.strLabel));
            syncAccessibleTabButtons();
            update();
            emit sigCurrentChanged(strId);
            return;
        }
}

QString UIMd3TabStrip::createGroup(const QString &strName)
{
    const QString strTrimmedName = strName.trimmed().left(80);
    if (strTrimmedName.isEmpty())
        return QString();
    UIMd3TabGroup group;
    group.strId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    group.strName = strTrimmedName;
    group.color = md3(UIMd3ColorRole_SecondaryContainer);
    group.fCollapsed = false;
    m_groups << group;
    announceModelChanged();
    return group.strId;
}

void UIMd3TabStrip::renameGroup(const QString &strGroupId, const QString &strName)
{
    for (UIMd3TabGroup &group : m_groups)
        if (group.strId == strGroupId)
        {
            const QString strTrimmedName = strName.trimmed().left(80);
            if (!strTrimmedName.isEmpty() && group.strName != strTrimmedName)
            {
                group.strName = strTrimmedName;
                announceModelChanged();
            }
            return;
        }
}

void UIMd3TabStrip::moveToGroup(const QString &strTabId, const QString &strGroupId)
{
    bool fKnownGroup = strGroupId.isEmpty();
    for (const UIMd3TabGroup &group : m_groups)
        fKnownGroup |= group.strId == strGroupId;
    if (!fKnownGroup)
        return;
    for (UIMd3Tab &tab : m_tabs)
        if (tab.strId == strTabId && tab.strGroupId != strGroupId)
        {
            tab.strGroupId = strGroupId;
            announceModelChanged();
            return;
        }
}

void UIMd3TabStrip::toggleGroupCollapsed(const QString &strGroupId)
{
    for (UIMd3TabGroup &group : m_groups)
        if (group.strId == strGroupId)
        {
            if (!group.fCollapsed)
                for (const UIMd3Tab &tab : m_tabs)
                    if (tab.strId == m_strCurrentId && tab.strGroupId == strGroupId)
                        return;
            group.fCollapsed = !group.fCollapsed;
            announceModelChanged();
            return;
        }
}

void UIMd3TabStrip::togglePinned(const QString &strTabId)
{
    for (UIMd3Tab &tab : m_tabs)
        if (tab.strId == strTabId)
        {
            tab.fPinned = !tab.fPinned;
            announceModelChanged();
            return;
        }
}

QList<UIMd3Tab> UIMd3TabStrip::resolveCloseSet(const QString &strQuery, bool fInverse,
                                               bool fRegex, bool fIncludePinned) const
{
    QList<UIMd3Tab> result;
    const QString strBoundedQuery = strQuery.left(4096);
    if (strBoundedQuery.trimmed().isEmpty())
        return result;
    QRegularExpression expression;
    if (fRegex)
    {
        expression = QRegularExpression(strBoundedQuery);
        if (!expression.isValid())
            return result;
    }
    for (const UIMd3Tab &tab : m_tabs)
    {
        if (tab.fPinned && !fIncludePinned)
            continue;
        const bool fMatch = fRegex
                          ? expression.match(tab.strLabel).hasMatch()
                          : tab.strLabel.contains(strBoundedQuery, Qt::CaseInsensitive);
        if (fMatch != fInverse)
            result << tab;
    }
    return result;
}

void UIMd3TabStrip::save() const
{
    if (!gEDataManager)
        return;
    QJsonObject root;
    root.insert(QStringLiteral("version"), 2);
    root.insert(QStringLiteral("provenance"), QStringLiteral("md3-tab-model-v2"));
    root.insert(QStringLiteral("current"), m_strCurrentId);
    QJsonArray groups;
    for (const UIMd3TabGroup &group : m_groups)
    {
        QJsonObject item;
        item.insert(QStringLiteral("id"), group.strId);
        item.insert(QStringLiteral("name"), group.strName);
        item.insert(QStringLiteral("color"), group.color.name(QColor::HexArgb));
        item.insert(QStringLiteral("collapsed"), group.fCollapsed);
        groups.append(item);
    }
    root.insert(QStringLiteral("groups"), groups);
    QJsonArray tabs;
    for (const UIMd3Tab &tab : m_tabs)
    {
        QJsonObject item;
        item.insert(QStringLiteral("id"), tab.strId);
        item.insert(QStringLiteral("label"), tab.strLabel);
        item.insert(QStringLiteral("group"), tab.strGroupId);
        item.insert(QStringLiteral("pinned"), tab.fPinned);
        tabs.append(item);
    }
    root.insert(QStringLiteral("tabs"), tabs);
    gEDataManager->setExtraDataString(g_pszTabsExtraData,
                                      QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void UIMd3TabStrip::restore()
{
    m_fRestoredLegacyPersistence = false;
    if (!gEDataManager)
        return;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(
        gEDataManager->extraDataString(g_pszTabsExtraData).toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return;
    if (document.toJson(QJsonDocument::Compact).size() > 256 * 1024)
        return;
    const QJsonObject root = document.object();
    const int iVersion = root.value(QStringLiteral("version")).toInt(1);
    if (iVersion != 1 && iVersion != 2)
        return;
    m_fRestoredLegacyPersistence = iVersion == 1
                                 && !root.contains(QStringLiteral("provenance"));
    m_groups.clear();
    m_tabs.clear();
    const QJsonArray groups = root.value(QStringLiteral("groups")).toArray();
    for (int i = 0; i < qMin(groups.size(), 128); ++i)
    {
        const QJsonObject item = groups.at(i).toObject();
        UIMd3TabGroup group;
        group.strId = item.value(QStringLiteral("id")).toString().trimmed().left(128);
        group.strName = item.value(QStringLiteral("name")).toString().trimmed().left(80);
        group.color = QColor(item.value(QStringLiteral("color")).toString());
        if (!group.color.isValid())
            group.color = md3(UIMd3ColorRole_SecondaryContainer);
        group.fCollapsed = item.value(QStringLiteral("collapsed")).toBool();
        bool fDuplicate = false;
        for (const UIMd3TabGroup &existing : m_groups)
            fDuplicate |= existing.strId == group.strId;
        if (!fDuplicate && !group.strId.isEmpty() && !group.strName.isEmpty())
            m_groups << group;
    }
    const QJsonArray tabs = root.value(QStringLiteral("tabs")).toArray();
    for (int i = 0; i < qMin(tabs.size(), 256); ++i)
    {
        const QJsonObject item = tabs.at(i).toObject();
        UIMd3Tab tab;
        tab.strId = item.value(QStringLiteral("id")).toString().trimmed().left(128);
        tab.strLabel = item.value(QStringLiteral("label")).toString().trimmed().left(160);
        tab.strGroupId = item.value(QStringLiteral("group")).toString().trimmed().left(128);
        tab.fPinned = item.value(QStringLiteral("pinned")).toBool();
        /* Availability belongs to the live manager restrictions.  Persisting
         * it made yesterday's transient policy override today's valid state. */
        tab.fEnabled = true;
        bool fDuplicate = false;
        for (const UIMd3Tab &existing : m_tabs)
            fDuplicate |= existing.strId == tab.strId;
        if (!fDuplicate && !tab.strId.isEmpty() && !tab.strLabel.isEmpty())
            m_tabs << tab;
    }
    for (UIMd3Tab &tab : m_tabs)
    {
        bool fKnownGroup = tab.strGroupId.isEmpty();
        for (const UIMd3TabGroup &group : m_groups)
            fKnownGroup |= tab.strGroupId == group.strId;
        if (!fKnownGroup)
            tab.strGroupId.clear();
    }
    m_strCurrentId = root.value(QStringLiteral("current")).toString().trimmed().left(128);
    bool fCurrentFound = false;
    for (const UIMd3Tab &tab : m_tabs)
        if (tab.strId == m_strCurrentId && tab.fEnabled)
            fCurrentFound = true;
    if (!fCurrentFound)
        m_strCurrentId.clear();
    if (m_strCurrentId.isEmpty() && !displayTabs().isEmpty())
        m_strCurrentId = displayTabs().first().strId;
}

QRect UIMd3TabStrip::tabRect(const QString &strId) const
{
    int iX = 8;
    const QList<UIMd3Tab> aDisplayTabs = displayTabs();
    QList<UIMd3Tab> aPinnedTabs;
    for (const UIMd3Tab &tab : aDisplayTabs)
        if (tab.fPinned)
            aPinnedTabs << tab;
    const int iFirstPinned = qBound(0, m_iFirstVisiblePinned,
                                    qMax(0, aPinnedTabs.size() - 1));
    for (int i = iFirstPinned; i < aPinnedTabs.size(); ++i)
    {
        const UIMd3Tab &tab = aPinnedTabs.at(i);
        const QRect rect(iX, 0, tabWidth(tab), 48);
        if (tab.strId == strId)
            return rect;
        iX += rect.width() + 4;
    }
    const QList<UIMd3Tab> aUnpinnedTabs = unpinnedDisplayTabs();
    const int iFirst = qBound(0, m_iFirstVisibleUnpinned,
                              qMax(0, aUnpinnedTabs.size() - 1));
    for (int i = iFirst; i < aUnpinnedTabs.size(); ++i)
    {
        const UIMd3Tab &tab = aUnpinnedTabs.at(i);
        const QRect rect(iX, 0, tabWidth(tab), 48);
        if (tab.strId == strId)
            return rect;
        iX += rect.width() + 4;
    }
    return QRect();
}

int UIMd3TabStrip::tabWidth(const UIMd3Tab &tab) const
{
    const QFontMetrics metrics(font());
    return qBound(120, metrics.horizontalAdvance(tab.strLabel) + 72, 210);
}

bool UIMd3TabStrip::hasOverflow() const
{
    const QList<UIMd3Tab> aDisplayTabs = displayTabs();
    if (aDisplayTabs.isEmpty())
        return false;
    if (m_iFirstVisiblePinned > 0 || m_iFirstVisibleUnpinned > 0)
        return true;
    int iRight = 8;
    for (const UIMd3Tab &tab : displayTabs())
    {
        iRight += tabWidth(tab) + 4;
        if (iRight > width() - 100)
            return true;
    }
    return false;
}

int UIMd3TabStrip::tabContentRight() const
{
    return qMax(0, width() - (hasOverflow() ? 148 : 100));
}

bool UIMd3TabStrip::isTabFullyVisible(const QString &strId) const
{
    const QRect tabGeometry = tabRect(strId);
    return tabGeometry.isValid()
        && tabGeometry.left() >= 0
        && tabGeometry.right() < tabContentRight();
}

void UIMd3TabStrip::ensureTabVisible(const QString &strId)
{
    QList<UIMd3Tab> aPinnedTabs;
    for (const UIMd3Tab &tab : displayTabs())
        if (tab.fPinned)
            aPinnedTabs << tab;
    const QList<UIMd3Tab> aUnpinnedTabs = unpinnedDisplayTabs();
    const int iPreviousPinned = m_iFirstVisiblePinned;
    const int iPrevious = m_iFirstVisibleUnpinned;
    normalizeViewport();
    for (int i = 0; i < aPinnedTabs.size(); ++i)
        if (aPinnedTabs.at(i).strId == strId)
        {
            if (i < m_iFirstVisiblePinned || !isTabFullyVisible(strId))
                m_iFirstVisiblePinned = i;
            while (m_iFirstVisiblePinned > 0)
            {
                --m_iFirstVisiblePinned;
                if (!isTabFullyVisible(strId))
                {
                    ++m_iFirstVisiblePinned;
                    break;
                }
            }
            if (iPreviousPinned != m_iFirstVisiblePinned)
            {
                updateOverflowButton();
                update();
            }
            return;
        }
    int iTarget = -1;
    for (int i = 0; i < aUnpinnedTabs.size(); ++i)
        if (aUnpinnedTabs.at(i).strId == strId)
        {
            iTarget = i;
            break;
        }
    if (iTarget < 0)
    {
        if (iPreviousPinned != m_iFirstVisiblePinned || iPrevious != m_iFirstVisibleUnpinned)
        {
            updateOverflowButton();
            update();
        }
        return;
    }
    if (iTarget < m_iFirstVisibleUnpinned || !isTabFullyVisible(strId))
        m_iFirstVisibleUnpinned = iTarget;
    /* A long pinned region must not make the selected ordinary tab
     * impossible to reveal.  Keep at least one pinned tab in the stable
     * region, then expose earlier pinned tabs again while the target fits. */
    while (   !isTabFullyVisible(strId)
           && m_iFirstVisiblePinned < qMax(0, aPinnedTabs.size() - 1))
        ++m_iFirstVisiblePinned;
    while (m_iFirstVisibleUnpinned > 0)
    {
        --m_iFirstVisibleUnpinned;
        if (!isTabFullyVisible(strId))
        {
            ++m_iFirstVisibleUnpinned;
            break;
        }
    }
    while (m_iFirstVisiblePinned > 0)
    {
        --m_iFirstVisiblePinned;
        if (!isTabFullyVisible(strId))
        {
            ++m_iFirstVisiblePinned;
            break;
        }
    }
    if (iPreviousPinned != m_iFirstVisiblePinned || iPrevious != m_iFirstVisibleUnpinned)
    {
        updateOverflowButton();
        update();
    }
}

void UIMd3TabStrip::normalizeViewport()
{
    int cPinnedTabs = 0;
    for (const UIMd3Tab &tab : displayTabs())
        if (tab.fPinned)
            ++cPinnedTabs;
    m_iFirstVisiblePinned = qBound(0, m_iFirstVisiblePinned,
                                  qMax(0, cPinnedTabs - 1));
    const QList<UIMd3Tab> aUnpinnedTabs = unpinnedDisplayTabs();
    m_iFirstVisibleUnpinned = qBound(0, m_iFirstVisibleUnpinned,
                                    qMax(0, aUnpinnedTabs.size() - 1));
    if (aUnpinnedTabs.isEmpty())
        return;
    const QString strLastId = aUnpinnedTabs.last().strId;
    while (m_iFirstVisibleUnpinned > 0 && isTabFullyVisible(strLastId))
    {
        --m_iFirstVisibleUnpinned;
        if (!isTabFullyVisible(strLastId))
        {
            ++m_iFirstVisibleUnpinned;
            break;
        }
    }
}

QRect UIMd3TabStrip::tabCloseRect(const QString &strId) const
{
    for (const UIMd3Tab &tab : m_tabs)
        if (tab.strId == strId && !tab.fPinned)
        {
            const QRect tabGeometry = tabRect(strId);
            return QRect(tabGeometry.right() - 47, tabGeometry.top(), 48, 48);
        }
    return QRect();
}

QString UIMd3TabStrip::tabAt(const QPoint &position) const
{
    for (const UIMd3Tab &tab : displayTabs())
        if (isTabFullyVisible(tab.strId) && tabRect(tab.strId).contains(position))
            return tab.strId;
    return QString();
}

void UIMd3TabStrip::resizeEvent(QResizeEvent *pEvent)
{
    UIMd3Widget::resizeEvent(pEvent);
    if (!m_strCurrentId.isEmpty())
        ensureTabVisible(m_strCurrentId);
    updateOverflowButton();
}

void UIMd3TabStrip::updateOverflowButton()
{
    if (!m_pOverflowButton || !m_pNewTabButton || !m_pTabManagerButton)
        return;
    const bool fOverflow = hasOverflow();
    if (m_pTabList)
        m_pTabList->setGeometry(0, 0, tabContentRight(), 48);
    m_pOverflowButton->setVisible(fOverflow);
    if (fOverflow)
        m_pOverflowButton->setGeometry(width() - 144, 0, 48, 48);
    m_pNewTabButton->setGeometry(width() - 96, 0, 48, 48);
    m_pTabManagerButton->setGeometry(width() - 48, 0, 48, 48);
    syncAccessibleTabButtons();
    if (fOverflow)
        m_pOverflowButton->raise();
    m_pNewTabButton->raise();
    m_pTabManagerButton->raise();
}

void UIMd3TabStrip::syncAccessibleTabButtons()
{
    QStringList knownIds;
    QStringList knownCloseIds;
    const QString strTabButtonStyle = QStringLiteral(
        "QToolButton { background: transparent; border: 0; border-radius: 12px; }"
        "QToolButton:focus { border: 2px solid %1; }")
        .arg(md3(UIMd3ColorRole_Primary).name(QColor::HexArgb));
    for (const UIMd3Tab &tab : m_tabs)
    {
        knownIds << tab.strId;
        QToolButton *pButton = m_tabButtons.value(tab.strId, 0);
        if (!pButton)
        {
            pButton = new QToolButton(m_pTabList);
            pButton->setProperty("md3PageTab", true);
            pButton->setAutoRaise(true);
            pButton->setCheckable(true);
            pButton->setText(QString());
            pButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
            pButton->setAttribute(Qt::WA_TranslucentBackground);
            pButton->installEventFilter(this);
            const QString strId = tab.strId;
            connect(pButton, &QToolButton::clicked, this, [this, strId]()
            {
                setCurrentTabId(strId);
                syncAccessibleTabButtons();
            });
            m_tabButtons.insert(tab.strId, pButton);
        }
        pButton->setAccessibleName(tab.strLabel);
        pButton->setAccessibleDescription(md3TabText("md3.tabs.page-description",
                                          tr("Workspace tab %1")).arg(tab.strLabel));
        pButton->setToolTip(tab.strLabel);
        pButton->setStyleSheet(strTabButtonStyle);
        pButton->setEnabled(tab.fEnabled);
        pButton->setChecked(tab.strId == m_strCurrentId);
        pButton->setFocusPolicy(tab.strId == m_strCurrentId ? Qt::StrongFocus : Qt::ClickFocus);
        if (isTabFullyVisible(tab.strId))
        {
            QRect buttonGeometry = tabRect(tab.strId);
            if (!tab.fPinned)
                buttonGeometry.adjust(0, 0, -48, 0);
            pButton->setGeometry(buttonGeometry);
            pButton->show();

            if (!tab.fPinned)
            {
                knownCloseIds << tab.strId;
                QToolButton *pCloseButton = m_closeButtons.value(tab.strId, 0);
                if (!pCloseButton)
                {
                    pCloseButton = new QToolButton(this);
                    pCloseButton->setObjectName(QStringLiteral("md3TabClose_%1").arg(tab.strId));
                    pCloseButton->setText(QString(QChar(0x00d7)));
                    pCloseButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
                    pCloseButton->setAutoRaise(true);
                    pCloseButton->setFixedSize(QSize(48, 48));
                    pCloseButton->setFocusPolicy(Qt::StrongFocus);
                    const QString strId = tab.strId;
                    connect(pCloseButton, &QToolButton::clicked, this, [this, strId]()
                    {
                        closeTab(strId);
                    });
                    m_closeButtons.insert(tab.strId, pCloseButton);
                }
                const QString strCloseTab = md3TabText("md3.tabs.close-named",
                                                       tr("Close tab %1")).arg(tab.strLabel);
                pCloseButton->setAccessibleName(strCloseTab);
                pCloseButton->setToolTip(strCloseTab);
                pCloseButton->setEnabled(true);
                pCloseButton->setStyleSheet(strTabButtonStyle);
                pCloseButton->setGeometry(tabCloseRect(tab.strId));
                pCloseButton->show();
                pCloseButton->raise();
            }
        }
        else
        {
            pButton->hide();
            if (QToolButton *pCloseButton = m_closeButtons.value(tab.strId, 0))
                pCloseButton->hide();
        }
    }
    const QStringList staleIds = m_tabButtons.keys();
    for (const QString &strId : staleIds)
        if (!knownIds.contains(strId))
        {
            m_tabButtons.take(strId)->deleteLater();
        }
    const QStringList staleCloseIds = m_closeButtons.keys();
    for (const QString &strId : staleCloseIds)
        if (!knownCloseIds.contains(strId))
        {
            m_closeButtons.take(strId)->deleteLater();
        }
}

void UIMd3TabStrip::updateTheme()
{
    setFont(md3Theme().font(UIMd3TypeRole_LabelMedium));
    const QString strActionStyle = QStringLiteral(
            "QToolButton { color: %1; background: %2; border: 0; border-radius: 14px; }"
            "QToolButton:hover { background: %3; }"
            "QToolButton:focus { border: 2px solid %4; }")
            .arg(md3(UIMd3ColorRole_OnSurfaceVariant).name(QColor::HexArgb))
            .arg(md3(UIMd3ColorRole_SurfaceContainerHigh).name(QColor::HexArgb))
            .arg(md3(UIMd3ColorRole_SecondaryContainer).name(QColor::HexArgb))
            .arg(md3(UIMd3ColorRole_Primary).name(QColor::HexArgb));
    if (m_pOverflowButton)
        m_pOverflowButton->setStyleSheet(strActionStyle);
    if (m_pNewTabButton)
        m_pNewTabButton->setStyleSheet(strActionStyle);
    if (m_pTabManagerButton)
        m_pTabManagerButton->setStyleSheet(strActionStyle);
    updateGeometry();
    updateOverflowButton();
    update();
}

void UIMd3TabStrip::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), md3(UIMd3ColorRole_Surface));
    painter.setPen(md3(UIMd3ColorRole_OutlineVariant));
    painter.drawLine(0, height() - 1, width(), height() - 1);
    const QFontMetrics metrics(font());
    const QList<UIMd3Tab> aDisplayTabs = displayTabs();
    painter.save();
    painter.setClipRect(QRect(0, 0, qMax(0, tabContentRight()), height()));
    for (const UIMd3Tab &tab : aDisplayTabs)
    {
        const QRect tabGeometry = tabRect(tab.strId);
        if (!isTabFullyVisible(tab.strId))
            continue;
        const bool fCurrent = tab.strId == m_strCurrentId;
        painter.setPen(Qt::NoPen);
        QColor tabColor = fCurrent ? md3(UIMd3ColorRole_SecondaryContainer) : Qt::transparent;
        for (const UIMd3TabGroup &group : m_groups)
            if (group.strId == tab.strGroupId && group.color.isValid() && !fCurrent)
                tabColor = QColor(group.color).lighter(115);
        painter.setBrush(tabColor);
        painter.drawRoundedRect(tabGeometry, UIMd3Shape::Medium, UIMd3Shape::Medium);
        painter.setPen(fCurrent ? md3(UIMd3ColorRole_OnSecondaryContainer)
                                : md3(UIMd3ColorRole_OnSurfaceVariant));
        if (!tab.fEnabled)
            painter.setOpacity(0.48);
        const int iTrailingInset = tab.fPinned ? 20 : 54;
        const QString strLabel = metrics.elidedText(tab.strLabel, Qt::ElideRight,
                                                    tabGeometry.width() - 14 - iTrailingInset);
        painter.drawText(tabGeometry.adjusted(14, 0, -iTrailingInset, 0), Qt::AlignVCenter, strLabel);
        if (tab.fPinned)
        {
            painter.setBrush(md3(UIMd3ColorRole_Primary));
            painter.drawEllipse(QPoint(tabGeometry.right() - 10, tabGeometry.center().y()), 3, 3);
        }
        if (fCurrent)
            paintFocusRing(painter, tabGeometry.adjusted(2, 2, -2, -2), UIMd3Shape::Medium);
        painter.setOpacity(1.0);
    }
    painter.restore();
}

void UIMd3TabStrip::keyPressEvent(QKeyEvent *pEvent)
{
    const QList<UIMd3Tab> aDisplayTabs = displayTabs();
    QList<UIMd3Tab> aNavigationTabs;
    for (const UIMd3Tab &tab : aDisplayTabs)
        if (tab.fEnabled)
            aNavigationTabs << tab;
    if (pEvent->key() == Qt::Key_Down && hasOverflow())
    {
        showOverflowMenu();
        pEvent->accept();
        return;
    }
    if ((pEvent->key() == Qt::Key_Left || pEvent->key() == Qt::Key_Right) && !aNavigationTabs.isEmpty())
    {
        int iIndex = 0;
        for (int i = 0; i < aNavigationTabs.size(); ++i)
            if (aNavigationTabs.at(i).strId == m_strCurrentId) { iIndex = i; break; }
        iIndex = (iIndex + (pEvent->key() == Qt::Key_Left ? -1 : 1) + aNavigationTabs.size()) % aNavigationTabs.size();
        const QString strNextId = aNavigationTabs.at(iIndex).strId;
        setCurrentTabId(strNextId);
        if (QToolButton *pButton = m_tabButtons.value(strNextId, 0))
            pButton->setFocus(Qt::ShortcutFocusReason);
        pEvent->accept();
        return;
    }
    if (   (pEvent->matches(QKeySequence::Close) || pEvent->key() == Qt::Key_Delete)
        && !m_strCurrentId.isEmpty())
    {
        if (closeTab(m_strCurrentId))
        {
            pEvent->accept();
            return;
        }
    }
    QWidget::keyPressEvent(pEvent);
}

bool UIMd3TabStrip::eventFilter(QObject *pObject, QEvent *pEvent)
{
    QToolButton *pTabButton = qobject_cast<QToolButton*>(pObject);
    if (m_tabButtons.values().contains(pTabButton))
    {
        if (pEvent->type() == QEvent::KeyPress)
        {
            QKeyEvent *pKeyEvent = static_cast<QKeyEvent*>(pEvent);
            if (   pKeyEvent->key() == Qt::Key_Left
                || pKeyEvent->key() == Qt::Key_Right
                || pKeyEvent->key() == Qt::Key_Down
                || pKeyEvent->key() == Qt::Key_Delete
                || pKeyEvent->matches(QKeySequence::Close))
            {
                keyPressEvent(pKeyEvent);
                return pKeyEvent->isAccepted();
            }
        }
        else if (pEvent->type() == QEvent::ContextMenu)
        {
            QContextMenuEvent *pContextEvent = static_cast<QContextMenuEvent*>(pEvent);
            if (   pContextEvent->reason() == QContextMenuEvent::Mouse
                && (pContextEvent->modifiers() & Qt::ShiftModifier))
            {
                UIMd3AppearanceEditor::open(this,
                    QStringLiteral("tab/") + m_tabButtons.key(pTabButton));
                pContextEvent->accept();
                return true;
            }
            QContextMenuEvent forwardedEvent(pContextEvent->reason(),
                                              pTabButton->mapTo(this, pContextEvent->pos()),
                                              pContextEvent->globalPos(),
                                              pContextEvent->modifiers());
            contextMenuEvent(&forwardedEvent);
            return forwardedEvent.isAccepted();
        }
    }
    return UIMd3Widget::eventFilter(pObject, pEvent);
}

void UIMd3TabStrip::showOverflowMenu()
{
    QMenu menu(this);
    menu.setAccessibleName(md3TabText("md3.tabs.overflow-menu", tr("Tabs that do not fit")));
    UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-overflow"),
                                                     md3TabText("md3.tabs.search", tr("Search tabs")), &menu);
    QWidgetAction *pSearchAction = new QWidgetAction(&menu);
    pSearchAction->setDefaultWidget(pSearch);
    menu.addAction(pSearchAction);
    QHash<QAction *, QString> actionIds;
    for (const UIMd3Tab &tab : displayTabs())
        if (!isTabFullyVisible(tab.strId))
        {
            QAction *pAction = menu.addAction(tab.strLabel);
            pAction->setEnabled(tab.fEnabled);
            pAction->setStatusTip(md3TabText("md3.tabs.open-named", tr("Open tab %1")).arg(tab.strLabel));
            pAction->setWhatsThis(pAction->statusTip());
            if (!tab.fEnabled)
            {
                const QString strReason = md3TabText("md3.tabs.unavailable",
                                          tr("Tab %1 is unavailable under the current manager restrictions"))
                                          .arg(tab.strLabel);
                pAction->setStatusTip(strReason);
                pAction->setToolTip(strReason);
                pAction->setWhatsThis(strReason);
            }
            actionIds.insert(pAction, tab.strId);
        }
    connect(pSearch, &UIMd3SearchField::sigFilterChanged, &menu,
            [pSearch, actionIds]() mutable
    {
        for (QAction *pAction : actionIds.keys())
            pAction->setVisible(pSearch->matches(pAction->text()));
    });
    for (QAction *pAction : actionIds.keys())
        connect(pAction, &QAction::triggered, this, [this, pAction, actionIds]()
        {
            setCurrentTabId(actionIds.value(pAction));
        });
    pSearch->setFocus(Qt::OtherFocusReason);
        menu.exec(mapToGlobal(QPoint(width() - 144, height())));
}

void UIMd3TabStrip::showNewTabMenu()
{
    QMenu menu(this);
    menu.setAccessibleName(md3TabText("md3.tabs.new-menu", tr("New workspace tab destinations")));
    UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("new-tab-destinations"),
                                                     md3TabText("md3.tabs.search-destinations",
                                                                tr("Search destinations")), &menu);
    QWidgetAction *pSearchAction = new QWidgetAction(&menu);
    pSearchAction->setDefaultWidget(pSearch);
    menu.addAction(pSearchAction);
    QHash<QAction *, UIMd3TabChoice> actionChoices;
    for (const UIMd3TabChoice &choice : m_availableTabs)
    {
        bool fOpen = false;
        for (const UIMd3Tab &tab : m_tabs)
            fOpen |= tab.strId == choice.strId;
        if (fOpen)
            continue;
        QAction *pAction = menu.addAction(choice.strLabel);
        pAction->setEnabled(choice.fEnabled);
        pAction->setStatusTip(choice.fEnabled
                            ? md3TabText("md3.tabs.open-destination",
                                         tr("Open %1 in a workspace tab")).arg(choice.strLabel)
                            : md3TabText("md3.tabs.destination-unavailable",
                                         tr("%1 is unavailable under the current manager restrictions")).arg(choice.strLabel));
        pAction->setWhatsThis(pAction->statusTip());
        actionChoices.insert(pAction, choice);
    }
    if (actionChoices.isEmpty())
    {
        QAction *pEmpty = menu.addAction(md3TabText("md3.tabs.destinations-open",
                                                   tr("All available destinations are already open")));
        pEmpty->setEnabled(false);
    }
    connect(pSearch, &UIMd3SearchField::sigFilterChanged, &menu,
            [pSearch, actionChoices]() mutable
    {
        for (QAction *pAction : actionChoices.keys())
            pAction->setVisible(pSearch->matches(pAction->text()));
    });
    for (QAction *pAction : actionChoices.keys())
        connect(pAction, &QAction::triggered, this, [this, pAction, actionChoices]()
        {
            const UIMd3TabChoice choice = actionChoices.value(pAction);
            openTab(choice.strId, choice.strLabel);
        });
    pSearch->setFocus(Qt::OtherFocusReason);
    menu.exec(m_pNewTabButton->mapToGlobal(QPoint(0, m_pNewTabButton->height())));
}

void UIMd3TabStrip::showTabManagerMenu()
{
    QMenu menu(this);
    menu.setAccessibleName(md3TabText("md3.tabs.manager", tr("Tab manager")));
    UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-manager"),
                                                     md3TabText("md3.tabs.search-all",
                                                                tr("Search all open tabs")), &menu);
    QWidgetAction *pSearchAction = new QWidgetAction(&menu);
    pSearchAction->setDefaultWidget(pSearch);
    menu.addAction(pSearchAction);
    QHash<QAction *, QString> actionIds;
    for (const UIMd3Tab &tab : m_tabs)
    {
        QAction *pAction = menu.addAction(tab.strLabel);
        pAction->setCheckable(true);
        pAction->setChecked(tab.strId == m_strCurrentId);
        pAction->setEnabled(tab.fEnabled);
        pAction->setStatusTip(tab.fEnabled
                            ? md3TabText("md3.tabs.activate-named",
                                         tr("Activate workspace tab %1")).arg(tab.strLabel)
                            : md3TabText("md3.tabs.unavailable",
                                         tr("Tab %1 is unavailable under the current manager restrictions")).arg(tab.strLabel));
        pAction->setWhatsThis(pAction->statusTip());
        actionIds.insert(pAction, tab.strId);
    }
    if (actionIds.isEmpty())
    {
        QAction *pEmpty = menu.addAction(md3TabText("md3.tabs.none-open",
                                                   tr("No workspace tabs are open")));
        pEmpty->setEnabled(false);
    }
    connect(pSearch, &UIMd3SearchField::sigFilterChanged, &menu,
            [pSearch, actionIds]() mutable
    {
        for (QAction *pAction : actionIds.keys())
            pAction->setVisible(pSearch->matches(pAction->text()));
    });
    for (QAction *pAction : actionIds.keys())
        connect(pAction, &QAction::triggered, this, [this, pAction, actionIds]()
        {
            setCurrentTabId(actionIds.value(pAction));
        });
    pSearch->setFocus(Qt::OtherFocusReason);
    menu.exec(m_pTabManagerButton->mapToGlobal(QPoint(0, m_pTabManagerButton->height())));
}

void UIMd3TabStrip::showGroupPicker(const QString &strTabId)
{
    QDialog dialog(this, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setObjectName(QStringLiteral("md3TabGroupPicker"));
    dialog.setWindowTitle(md3TabText("md3.tabs.group.move-title", tr("Move tab into group")));
    dialog.setAccessibleName(dialog.windowTitle());
    dialog.setAutoFillBackground(true);
    QPalette palette = dialog.palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Base, md3(UIMd3ColorRole_SurfaceContainer));
    dialog.setPalette(palette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(&dialog);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(8);

    QScrollArea *pScrollArea = new QScrollArea(&dialog);
    pScrollArea->setObjectName(QStringLiteral("md3TabGroupPickerScrollArea"));
    pScrollArea->setFrameShape(QFrame::NoFrame);
    pScrollArea->setWidgetResizable(true);
    pScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *pContent = new QWidget(pScrollArea);
    QVBoxLayout *pContentLayout = new QVBoxLayout(pContent);
    pContentLayout->setContentsMargins(0, 0, 0, 0);
    pContentLayout->setSpacing(8);
    pScrollArea->setWidget(pContent);
    pRootLayout->addWidget(pScrollArea, 1);

    QLabel *pHeading = new QLabel(md3TabText("md3.tabs.group.choose",
                                            tr("Choose an existing group or create one")), pContent);
    pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pContentLayout->addWidget(pHeading);

    UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-group-picker"),
                                                     md3TabText("md3.tabs.group.search", tr("Search groups")), pContent);
    pContentLayout->addWidget(pSearch);

    QLabel *pEmpty = new QLabel(md3TabText("md3.tabs.group.empty",
                                          tr("No groups yet. Create one below.")), pContent);
    pEmpty->setWordWrap(true);
    pEmpty->setAccessibleName(pEmpty->text());
    pContentLayout->addWidget(pEmpty);

    QListWidget *pGroups = new QListWidget(pContent);
    pGroups->setObjectName(QStringLiteral("md3TabGroupPickerList"));
    pGroups->setAccessibleName(md3TabText("md3.tabs.group.targets", tr("Move target groups")));
    pGroups->setSelectionMode(QAbstractItemView::SingleSelection);
    pGroups->setMinimumHeight(96);
    pContentLayout->addWidget(pGroups, 1);

    QHBoxLayout *pCreateLayout = new QHBoxLayout;
    pCreateLayout->setContentsMargins(0, 0, 0, 0);
    QLineEdit *pNewGroup = new QLineEdit(pContent);
    pNewGroup->setPlaceholderText(md3TabText("md3.tabs.group.new-name", tr("New group name")));
    pNewGroup->setAccessibleName(pNewGroup->placeholderText());
    pNewGroup->setMaxLength(80);
    QPushButton *pCreate = new QPushButton(md3TabText("md3.tabs.group.create", tr("Create group")), pContent);
    pCreate->setAccessibleName(pCreate->text());
    pCreate->setEnabled(false);
    pCreate->setToolTip(md3TabText("md3.tabs.group.name-required",
                                  tr("Enter a group name from 1 to 80 characters.")));
    pCreate->setAccessibleDescription(pCreate->toolTip());
    pCreateLayout->addWidget(pNewGroup, 1);
    pCreateLayout->addWidget(pCreate);
    pContentLayout->addLayout(pCreateLayout);

    QDialogButtonBox *pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                      Qt::Horizontal, &dialog);
    pButtons->button(QDialogButtonBox::Ok)->setText(md3TabText("md3.tabs.group.move", tr("Move")));
    pButtons->button(QDialogButtonBox::Ok)->setAccessibleName(
        md3TabText("md3.tabs.group.move-selected", tr("Move tab into selected group")));
    pButtons->button(QDialogButtonBox::Cancel)->setAccessibleName(
        md3TabText("md3.tabs.group.cancel-move", tr("Cancel moving tab")));
    pRootLayout->addWidget(pButtons);

    const auto populate = [this, pGroups, pEmpty, pSearch, pButtons]()
    {
        const bool fHasFilter = pSearch->isRegexActive() || !pSearch->text().isEmpty();
        pGroups->clear();
        QListWidgetItem *pNoGroup = new QListWidgetItem(
            md3TabText("md3.tabs.group.none", tr("No group (top level)")), pGroups);
        pNoGroup->setData(Qt::UserRole, QString());
        pNoGroup->setToolTip(md3TabText("md3.tabs.group.keep-top",
                                       tr("Keep this tab outside a group")));
        int cGroups = 0;
        for (const UIMd3TabGroup &group : m_groups)
        {
            int cMembers = 0;
            for (const UIMd3Tab &tab : m_tabs)
                if (tab.strGroupId == group.strId)
                    ++cMembers;
            const QString strLabel = QStringLiteral("%1 · %2 · %3")
                                   .arg(group.strName,
                                        group.color.name(QColor::HexRgb),
                                        md3TabText("md3.tabs.group.members", tr("%1 members")).arg(cMembers));
            QListWidgetItem *pItem = new QListWidgetItem(strLabel, pGroups);
            pItem->setData(Qt::UserRole, group.strId);
            pItem->setToolTip(md3TabText("md3.tabs.group.summary",
                              tr("%1, color %2, %3 members"))
                              .arg(group.strName, group.color.name(QColor::HexRgb))
                              .arg(cMembers));
            ++cGroups;
        }
        QListWidgetItem *pFirstVisible = 0;
        int cVisible = 0;
        for (int i = 0; i < pGroups->count(); ++i)
        {
            QListWidgetItem *pItem = pGroups->item(i);
            pItem->setHidden(fHasFilter && !pSearch->matches(pItem->text()));
            if (!pItem->isHidden() && !pFirstVisible)
                pFirstVisible = pItem;
            if (!pItem->isHidden())
                ++cVisible;
        }
        const bool fNoMatches = fHasFilter && cVisible == 0;
        pEmpty->setText(fNoMatches
                      ? md3TabText("md3.tabs.group.no-match", tr("No groups match the current search."))
                      : md3TabText("md3.tabs.group.empty", tr("No groups yet. Create one below.")));
        pEmpty->setAccessibleName(pEmpty->text());
        pEmpty->setVisible(fNoMatches || (cGroups == 0 && !fHasFilter));
        pGroups->setCurrentItem(pFirstVisible);
        pButtons->button(QDialogButtonBox::Ok)->setEnabled(pFirstVisible != 0);
        const QString strMoveReason = pFirstVisible
            ? QString()
            : md3TabText("md3.tabs.group.no-match", tr("No groups match the current search."));
        pButtons->button(QDialogButtonBox::Ok)->setToolTip(strMoveReason);
        pButtons->button(QDialogButtonBox::Ok)->setAccessibleDescription(strMoveReason);
    };
    connect(pSearch, &UIMd3SearchField::sigFilterChanged, &dialog, populate);
    connect(pGroups, &QListWidget::currentItemChanged, &dialog,
            [pButtons](QListWidgetItem *pCurrent, QListWidgetItem *)
    {
        const bool fAvailable = pCurrent && !pCurrent->isHidden();
        pButtons->button(QDialogButtonBox::Ok)->setEnabled(fAvailable);
        if (!fAvailable && pButtons->button(QDialogButtonBox::Ok)->toolTip().isEmpty())
        {
            const QString strReason = md3TabText("md3.tabs.group.no-match",
                                      QObject::tr("No groups match the current search."));
            pButtons->button(QDialogButtonBox::Ok)->setToolTip(strReason);
            pButtons->button(QDialogButtonBox::Ok)->setAccessibleDescription(strReason);
        }
    });
    connect(pCreate, &QPushButton::clicked, &dialog,
            [this, pNewGroup, pGroups, populate]()
    {
        const QString strGroupId = createGroup(pNewGroup->text());
        if (strGroupId.isEmpty())
            return;
        pNewGroup->clear();
        populate();
        for (int i = 0; i < pGroups->count(); ++i)
            if (   pGroups->item(i)->data(Qt::UserRole).toString() == strGroupId
                && !pGroups->item(i)->isHidden())
            {
                pGroups->setCurrentRow(i);
                break;
            }
    });
    connect(pNewGroup, &QLineEdit::textChanged, &dialog,
            [pCreate](const QString &strText)
    {
        const bool fValid = !strText.trimmed().isEmpty();
        pCreate->setEnabled(fValid);
        pCreate->setAccessibleDescription(fValid ? QString() : pCreate->toolTip());
    });
    connect(pButtons, &QDialogButtonBox::accepted, &dialog,
            [this, &dialog, pGroups, strTabId]()
    {
        QListWidgetItem *pItem = pGroups->currentItem();
        if (!pItem || pItem->isHidden())
            return;
        moveToGroup(strTabId, pItem->data(Qt::UserRole).toString());
        dialog.accept();
    });
    connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                &dialog, [&dialog, pHeading, pSearch, pEmpty, pGroups, pNewGroup, pCreate, pButtons, populate]()
    {
        dialog.setWindowTitle(md3TabText("md3.tabs.group.move-title", dialog.tr("Move tab into group")));
        dialog.setAccessibleName(dialog.windowTitle());
        pHeading->setText(md3TabText("md3.tabs.group.choose",
                                    dialog.tr("Choose an existing group or create one")));
        pSearch->setPlaceholderText(md3TabText("md3.tabs.group.search", dialog.tr("Search groups")));
        pEmpty->setText(md3TabText("md3.tabs.group.empty", dialog.tr("No groups yet. Create one below.")));
        pEmpty->setAccessibleName(pEmpty->text());
        pGroups->setAccessibleName(md3TabText("md3.tabs.group.targets", dialog.tr("Move target groups")));
        pNewGroup->setPlaceholderText(md3TabText("md3.tabs.group.new-name", dialog.tr("New group name")));
        pNewGroup->setAccessibleName(pNewGroup->placeholderText());
        pCreate->setText(md3TabText("md3.tabs.group.create", dialog.tr("Create group")));
        pCreate->setAccessibleName(pCreate->text());
        pCreate->setToolTip(md3TabText("md3.tabs.group.name-required",
                                      dialog.tr("Enter a group name from 1 to 80 characters.")));
        pCreate->setAccessibleDescription(pCreate->isEnabled() ? QString() : pCreate->toolTip());
        pButtons->button(QDialogButtonBox::Ok)->setText(md3TabText("md3.tabs.group.move", dialog.tr("Move")));
        pButtons->button(QDialogButtonBox::Ok)->setAccessibleName(
            md3TabText("md3.tabs.group.move-selected", dialog.tr("Move tab into selected group")));
        pButtons->button(QDialogButtonBox::Cancel)->setAccessibleName(
            md3TabText("md3.tabs.group.cancel-move", dialog.tr("Cancel moving tab")));
        populate();
    });

    populate();
    pSearch->setFocus(Qt::OtherFocusReason);
    QScreen *pScreen = window() ? window()->screen() : 0;
    const QRect available = pScreen ? pScreen->availableGeometry()
                                    : QRect(0, 0, 640, 360);
    const QSize ownerSize = window() ? window()->size() : QSize(640, 360);
    const int iMaximumWidth = qMax(280, qMin(available.width() - 24, ownerSize.width() - 24));
    const int iMaximumHeight = qMax(220, qMin(available.height() - 48, ownerSize.height() - 48));
    dialog.setMaximumSize(iMaximumWidth, iMaximumHeight);
    dialog.setMinimumSize(qMin(320, iMaximumWidth), qMin(220, iMaximumHeight));
    dialog.resize(qMin(440, iMaximumWidth), qMin(304, iMaximumHeight));
    dialog.exec();
}

void UIMd3TabStrip::mousePressEvent(QMouseEvent *pEvent)
{
    const QString strId = tabAt(pEvent->position().toPoint());
    if (   pEvent->button() == Qt::LeftButton
        && !strId.isEmpty()
        && tabCloseRect(strId).contains(pEvent->position().toPoint()))
    {
        closeTab(strId);
        setFocus(Qt::MouseFocusReason);
        pEvent->accept();
        return;
    }
    if (pEvent->button() == Qt::LeftButton && !strId.isEmpty())
    {
        setCurrentTabId(strId);
        setFocus(Qt::MouseFocusReason);
        pEvent->accept();
        return;
    }
    if (pEvent->button() == Qt::LeftButton && strId.isEmpty() && hasOverflow())
    {
        showOverflowMenu();
        pEvent->accept();
        return;
    }
    if (pEvent->button() == Qt::RightButton && (pEvent->modifiers() & Qt::ShiftModifier) && !strId.isEmpty())
    {
        UIMd3AppearanceEditor::open(this, QStringLiteral("tab/") + strId);
        pEvent->accept();
        return;
    }
    UIMd3Widget::mousePressEvent(pEvent);
}

void UIMd3TabStrip::contextMenuEvent(QContextMenuEvent *pEvent)
{
    const bool fKeyboardContext = pEvent->reason() == QContextMenuEvent::Keyboard;
    const QString strHitId = tabAt(pEvent->pos());
    /* Keyboard context-menu events do not carry a useful tab position.  The
     * strip has one focus/current model, so reuse that stable id; pointer
     * chrome with no tab hit must continue to open the strip-level menu. */
    const QString strId = fKeyboardContext ? m_strCurrentId : strHitId;
    if (strId.isEmpty())
    {
        QMenu menu(this);
        menu.setAccessibleName(md3TabText("md3.tabs.strip-actions", tr("Tab strip actions")));
        UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-groups"),
                                                         md3TabText("md3.tabs.search-strip",
                                                                    tr("Search tab strip")), &menu);
        QWidgetAction *pSearchAction = new QWidgetAction(&menu);
        pSearchAction->setDefaultWidget(pSearch);
        menu.addAction(pSearchAction);
        QAction *pCreateGroup = menu.addAction(md3TabText("md3.tabs.group.create-ellipsis",
                                                         tr("Create group…")));
        pCreateGroup->setStatusTip(md3TabText("md3.tabs.group.create-description",
                                             tr("Create a new tab group")));
        pCreateGroup->setWhatsThis(md3TabText("md3.tabs.group.create-help",
                                             tr("Create a new tab group with a name from 1 to 80 characters")));
        QList<QAction *> groupActions;
        for (const UIMd3TabGroup &group : m_groups)
        {
            const QString strGroupId = group.strId;
            const QString strGroupName = group.strName;
            QAction *pGroup = menu.addAction(group.fCollapsed
                                           ? md3TabText("md3.tabs.group.expand", tr("Expand %1")).arg(group.strName)
                                           : md3TabText("md3.tabs.group.collapse", tr("Collapse %1")).arg(group.strName));
            pGroup->setStatusTip(md3TabText("md3.tabs.group.toggle", tr("Toggle group %1")).arg(group.strName));
            pGroup->setWhatsThis(md3TabText("md3.tabs.group.toggle-help",
                                           tr("Expand or collapse tab group %1")).arg(group.strName));
            groupActions << pGroup;

            QAction *pRename = menu.addAction(md3TabText("md3.tabs.group.rename-named",
                                                         tr("Rename group: %1")).arg(group.strName));
            pRename->setStatusTip(md3TabText("md3.tabs.group.rename-description",
                                            tr("Rename tab group %1")).arg(group.strName));
            pRename->setWhatsThis(md3TabText("md3.tabs.group.rename-help",
                                            tr("Rename tab group %1; the name is limited to 80 characters"))
                                  .arg(group.strName));
            groupActions << pRename;

            QAction *pAppearance = menu.addAction(md3TabText("md3.tabs.group.edit-named",
                                                             tr("Edit group appearance…: %1")).arg(group.strName));
            pAppearance->setStatusTip(md3TabText("md3.tabs.group.edit-description",
                                                tr("Edit appearance for tab group %1")).arg(group.strName));
            pAppearance->setWhatsThis(pAppearance->statusTip());
            groupActions << pAppearance;

            connect(pGroup, &QAction::triggered, this, [this, strGroupId]()
            {
                toggleGroupCollapsed(strGroupId);
            });
            connect(pRename, &QAction::triggered, this,
                    [this, strGroupId, strGroupName]()
            {
                QInputDialog dialog(this);
                dialog.setInputMode(QInputDialog::TextInput);
                dialog.setWindowTitle(md3TabText("md3.tabs.group.rename", tr("Rename group")));
                dialog.setAccessibleName(md3TabText("md3.tabs.group.rename-accessible", tr("Rename tab group")));
                dialog.setLabelText(md3TabText("md3.tabs.group.name", tr("Group name")));
                dialog.setOkButtonText(md3TabText("md3.tabs.group.rename-button", tr("Rename")));
                dialog.setCancelButtonText(md3TabText("md3.tabs.cancel", tr("Cancel")));
                dialog.setTextValue(strGroupName.left(80));
                if (QLineEdit *pEditor = dialog.findChild<QLineEdit *>())
                {
                    pEditor->setMaxLength(80);
                    pEditor->setAccessibleName(md3TabText("md3.tabs.group.name", tr("Group name")));
                    pEditor->setAccessibleDescription(md3TabText("md3.tabs.group.name-help",
                                                      tr("Enter 1 to 80 characters for the group name.")));
                    pEditor->setPlaceholderText(md3TabText("md3.tabs.group.name", tr("Group name")));
                    pEditor->selectAll();
                }
                if (dialog.exec() == QDialog::Accepted)
                    renameGroup(strGroupId, dialog.textValue().trimmed().left(80));
            });
            connect(pAppearance, &QAction::triggered, this,
                    [this, strGroupId]()
            {
                UIMd3AppearanceEditor::open(this, QStringLiteral("group/") + strGroupId);
            });
        }
        connect(pCreateGroup, &QAction::triggered, this, [this]()
        {
            QInputDialog dialog(this);
            dialog.setInputMode(QInputDialog::TextInput);
            dialog.setWindowTitle(md3TabText("md3.tabs.group.create", tr("Create group")));
            dialog.setAccessibleName(md3TabText("md3.tabs.group.create-accessible", tr("Create tab group")));
            dialog.setLabelText(md3TabText("md3.tabs.group.name", tr("Group name")));
            dialog.setOkButtonText(md3TabText("md3.tabs.group.create-button", tr("Create")));
            dialog.setCancelButtonText(md3TabText("md3.tabs.cancel", tr("Cancel")));
            if (QLineEdit *pEditor = dialog.findChild<QLineEdit *>())
            {
                pEditor->setMaxLength(80);
                pEditor->setAccessibleName(md3TabText("md3.tabs.group.name", tr("Group name")));
                pEditor->setAccessibleDescription(md3TabText("md3.tabs.group.name-help",
                                                  tr("Enter 1 to 80 characters for the group name.")));
                pEditor->setPlaceholderText(md3TabText("md3.tabs.group.name", tr("Group name")));
            }
            if (dialog.exec() == QDialog::Accepted)
                createGroup(dialog.textValue().trimmed().left(80));
        });
        QAction *pEdit = menu.addAction(md3TabText("md3.tabs.edit-strip", tr("Edit appearance…")));
        pEdit->setStatusTip(md3TabText("md3.tabs.edit-strip-description",
                                      tr("Edit appearance for the tab strip")));
        pEdit->setWhatsThis(pEdit->statusTip());
        pEdit->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F10));
        connect(pSearch, &UIMd3SearchField::sigFilterChanged, &menu,
                [pSearch, pCreateGroup, groupActions, pEdit]() mutable
        {
            pCreateGroup->setVisible(pSearch->matches(pCreateGroup->text()));
            for (QAction *pGroupAction : groupActions)
                pGroupAction->setVisible(pSearch->matches(pGroupAction->text()));
            pEdit->setVisible(pSearch->matches(pEdit->text()));
        });
        connect(pEdit, &QAction::triggered, this, [this]()
        {
            UIMd3AppearanceEditor::open(this, QStringLiteral("tab-strip"));
        });
        pSearch->setFocus(Qt::OtherFocusReason);
        menu.exec(pEvent->globalPos());
        pEvent->accept();
        return;
    }
    UIMd3Tab selected = {};
    bool fSelected = false;
    for (const UIMd3Tab &tab : m_tabs)
        if (tab.strId == strId) { selected = tab; fSelected = true; break; }
    if (!fSelected)
    {
        pEvent->ignore();
        return;
    }
    QMenu menu(this);
    UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-actions"),
                                                     md3TabText("md3.tabs.search-actions",
                                                                tr("Search tab actions")), &menu);
    QWidgetAction *pSearchAction = new QWidgetAction(&menu);
    pSearchAction->setDefaultWidget(pSearch);
    menu.addAction(pSearchAction);
    QAction *pPin = menu.addAction(selected.fPinned
                                 ? md3TabText("md3.tabs.unpin", tr("Unpin tab"))
                                 : md3TabText("md3.tabs.pin", tr("Pin tab")));
    QAction *pMove = menu.addAction(md3TabText("md3.tabs.move-ellipsis", tr("Move… into group…")));
    QAction *pClose = menu.addAction(md3TabText("md3.tabs.close", tr("Close tab")));
    QAction *pEdit = menu.addAction(md3TabText("md3.tabs.edit-tab", tr("Edit tab appearance…")));
    pClose->setShortcut(QKeySequence::Close);
    menu.setAccessibleName(md3TabText("md3.tabs.actions", tr("Tab actions")));
    if (selected.fPinned)
    {
        pClose->setEnabled(false);
        pClose->setStatusTip(md3TabText("md3.tabs.unpin-before-close",
                                       tr("Unpin this tab before closing it")));
    }
    pEdit->setStatusTip(md3TabText("md3.tabs.edit-tab-named",
                                  tr("Edit appearance for %1")).arg(selected.strLabel));
    pEdit->setWhatsThis(md3TabText("md3.tabs.edit-tab-help",
                                  tr("Edit the appearance of tab %1")).arg(selected.strLabel));
    pEdit->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F10));
    connect(pSearch, &UIMd3SearchField::sigFilterChanged, this,
            [pSearch, pPin, pMove, pClose, pEdit]()
    {
        pPin->setVisible(pSearch->matches(pPin->text()));
        pMove->setVisible(pSearch->matches(pMove->text()));
        pClose->setVisible(pSearch->matches(pClose->text()));
        pEdit->setVisible(pSearch->matches(pEdit->text()));
    });
    connect(pPin, &QAction::triggered, this, [this, strId]() { togglePinned(strId); });
    connect(pMove, &QAction::triggered, this, [this, strId]() { showGroupPicker(strId); });
    connect(pClose, &QAction::triggered, this, [this, strId]() { closeTab(strId); });
    connect(pEdit, &QAction::triggered, this, [this, strId]()
    {
        UIMd3AppearanceEditor::open(this, QStringLiteral("tab/") + strId);
    });
    pSearch->setFocus(Qt::OtherFocusReason);
    const QRect focusRect = tabRect(strId);
    const QPoint menuPosition = fKeyboardContext && focusRect.isValid()
                              ? mapToGlobal(focusRect.center())
                              : pEvent->globalPos();
    menu.exec(menuPosition);
    pEvent->accept();
}
