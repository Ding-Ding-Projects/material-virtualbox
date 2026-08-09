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
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QHash>
#include <QHBoxLayout>
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
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>
#include <QUuid>

#include "UIExtraDataManager.h"
#include "UIMd3AppearanceEditor.h"
#include "UIMd3SearchField.h"
#include "UIMd3TabStrip.h"
#include "UIMd3Theme.h"

static const char *g_pszTabsExtraData = "GUI/Md3/Tabs";

UIMd3TabStrip::UIMd3TabStrip(QWidget *pParent)
    : UIMd3Widget(pParent, QStringLiteral("tab-strip"))
    , m_pOverflowButton(0)
{
    setObjectName(QStringLiteral("md3TabStrip"));
    setAccessibleName(tr("Workspace tabs"));
    setFocusPolicy(Qt::StrongFocus);
    setMinimumHeight(md3Theme().controlHeight() + 16);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_pOverflowButton = new QToolButton(this);
    m_pOverflowButton->setText(QStringLiteral("…"));
    m_pOverflowButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_pOverflowButton->setAutoRaise(true);
    m_pOverflowButton->setMinimumSize(QSize(48, 48));
    m_pOverflowButton->setAccessibleName(tr("Show more tabs"));
    m_pOverflowButton->setAccessibleDescription(tr("Open the searchable list of tabs that do not fit"));
    m_pOverflowButton->setToolTip(tr("Show more tabs"));
    connect(m_pOverflowButton, &QToolButton::clicked, this, &UIMd3TabStrip::showOverflowMenu);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged, this, [this]()
    {
        setMinimumHeight(md3Theme().controlHeight() + 16);
        update();
    });
    restore();
    setAccessibleDescription(tr("Use Left and Right to change the selected tab."));
    updateOverflowButton();
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
        if (!fCollapsed && tab.fPinned)
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
        if (!fCollapsed && !tab.fPinned)
            result << tab;
    }
    return result;
}

void UIMd3TabStrip::announceModelChanged()
{
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
                setAccessibleDescription(tr("Selected tab: %1. Use Left and Right to change the selected tab.")
                                         .arg(tab.strLabel));
            announceModelChanged();
            return;
        }
}

void UIMd3TabStrip::setTabEnabled(const QString &strId, bool fEnabled)
{
    for (UIMd3Tab &tab : m_tabs)
        if (tab.strId == strId && tab.fEnabled != fEnabled)
        {
            const bool fWasCurrent = tab.strId == m_strCurrentId;
            tab.fEnabled = fEnabled;
            QString strNextId;
            if (fWasCurrent && !fEnabled)
            {
                for (const UIMd3Tab &candidate : displayTabs())
                    if (candidate.fEnabled)
                    {
                        strNextId = candidate.strId;
                        break;
                    }
                m_strCurrentId.clear();
            }
            announceModelChanged();
            if (fWasCurrent && !fEnabled)
            {
                emit sigCurrentChanged(QString());
                if (!strNextId.isEmpty())
                    setCurrentTabId(strNextId);
            }
            return;
        }
}

bool UIMd3TabStrip::closeTab(const QString &strId, bool fForce)
{
    for (int i = 0; i < m_tabs.size(); ++i)
        if (m_tabs.at(i).strId == strId)
        {
            if (m_tabs.at(i).fPinned && !fForce)
                return false;
            const bool fCurrent = m_tabs.at(i).strId == m_strCurrentId;
            m_tabs.removeAt(i);
            if (fCurrent)
                m_strCurrentId.clear();
            announceModelChanged();
            if (fCurrent)
            {
                const QList<UIMd3Tab> aDisplayTabs = displayTabs();
                QList<UIMd3Tab> aCandidates;
                for (const UIMd3Tab &tab : aDisplayTabs)
                    if (tab.fEnabled)
                        aCandidates << tab;
                if (!aCandidates.isEmpty())
                    setCurrentTabId(aCandidates.at(qBound(0, i - 1, aCandidates.size() - 1)).strId);
                else
                    emit sigCurrentChanged(QString());
            }
            return true;
        }
    return false;
}

void UIMd3TabStrip::setCurrentTabId(const QString &strId)
{
    for (const UIMd3Tab &tab : m_tabs)
        if (tab.strId == strId)
        {
            if (m_strCurrentId == strId)
                return;
            if (!tab.fEnabled)
                return;
            m_strCurrentId = strId;
            save();
            setAccessibleDescription(tr("Selected tab: %1. Use Left and Right to change the selected tab.")
                                     .arg(tab.strLabel));
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
            for (UIMd3TabGroup &group : m_groups)
                if (group.strId == strGroupId && group.fCollapsed && tab.strId == m_strCurrentId)
                    group.fCollapsed = false;
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
    root.insert(QStringLiteral("version"), 1);
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
        item.insert(QStringLiteral("enabled"), tab.fEnabled);
        tabs.append(item);
    }
    root.insert(QStringLiteral("tabs"), tabs);
    gEDataManager->setExtraDataString(g_pszTabsExtraData,
                                      QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

void UIMd3TabStrip::restore()
{
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
    if (root.value(QStringLiteral("version")).toInt(1) != 1)
        return;
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
        tab.fEnabled = item.value(QStringLiteral("enabled")).toBool(true);
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
        {
            fCurrentFound = true;
            for (const UIMd3TabGroup &group : m_groups)
                if (group.strId == tab.strGroupId && group.fCollapsed)
                    fCurrentFound = false;
        }
    if (!fCurrentFound)
        m_strCurrentId.clear();
    if (m_strCurrentId.isEmpty() && !displayTabs().isEmpty())
        m_strCurrentId = displayTabs().first().strId;
}

QRect UIMd3TabStrip::tabRect(const QString &strId) const
{
    int iX = 8;
    const QFontMetrics metrics(font());
    for (const UIMd3Tab &tab : displayTabs())
    {
        const int iWidth = qBound(96, metrics.horizontalAdvance(tab.strLabel) + 52, 240);
        const QRect rect(iX, 8, iWidth, height() - 16);
        if (tab.strId == strId)
            return rect;
        iX += iWidth + 4;
    }
    return QRect();
}

QString UIMd3TabStrip::tabAt(const QPoint &position) const
{
    for (const UIMd3Tab &tab : displayTabs())
        if (tabRect(tab.strId).contains(position))
            return tab.strId;
    return QString();
}

void UIMd3TabStrip::resizeEvent(QResizeEvent *pEvent)
{
    UIMd3Widget::resizeEvent(pEvent);
    updateOverflowButton();
}

void UIMd3TabStrip::updateOverflowButton()
{
    if (!m_pOverflowButton)
        return;
    const QList<UIMd3Tab> aDisplayTabs = displayTabs();
    const bool fOverflow = !aDisplayTabs.isEmpty()
                        && tabRect(aDisplayTabs.last().strId).right() > width();
    m_pOverflowButton->setVisible(fOverflow);
    if (fOverflow)
    {
        m_pOverflowButton->setGeometry(width() - 48, 4, 48, qMax(48, height() - 8));
        m_pOverflowButton->raise();
    }
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
    for (const UIMd3Tab &tab : aDisplayTabs)
    {
        const QRect tabGeometry = tabRect(tab.strId);
        if (!tabGeometry.intersects(rect()))
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
        const QString strLabel = metrics.elidedText(tab.strLabel, Qt::ElideRight, tabGeometry.width() - 36);
        painter.drawText(tabGeometry.adjusted(14, 0, -12, 0), Qt::AlignVCenter, strLabel);
        if (tab.fPinned)
        {
            painter.setBrush(md3(UIMd3ColorRole_Primary));
            painter.drawEllipse(QPoint(tabGeometry.right() - 10, tabGeometry.center().y()), 3, 3);
        }
        if (fCurrent)
            paintFocusRing(painter, tabGeometry.adjusted(2, 2, -2, -2), UIMd3Shape::Medium);
        painter.setOpacity(1.0);
    }
    if (!m_pOverflowButton->isVisible() && !aDisplayTabs.isEmpty()
        && tabRect(aDisplayTabs.last().strId).right() > width())
    {
        painter.setPen(md3(UIMd3ColorRole_OnSurface));
        painter.drawText(QRect(width() - 40, 0, 32, height()), Qt::AlignCenter, QStringLiteral("…"));
    }
}

void UIMd3TabStrip::keyPressEvent(QKeyEvent *pEvent)
{
    const QList<UIMd3Tab> aDisplayTabs = displayTabs();
    QList<UIMd3Tab> aNavigationTabs;
    for (const UIMd3Tab &tab : aDisplayTabs)
        if (tab.fEnabled)
            aNavigationTabs << tab;
    if (pEvent->key() == Qt::Key_Down && !aDisplayTabs.isEmpty()
        && tabRect(aDisplayTabs.last().strId).right() > width())
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
        setCurrentTabId(aNavigationTabs.at(iIndex).strId);
        pEvent->accept();
        return;
    }
    QWidget::keyPressEvent(pEvent);
}

void UIMd3TabStrip::showOverflowMenu()
{
    QMenu menu(this);
    UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-overflow"),
                                                     tr("Search tabs"), &menu);
    QWidgetAction *pSearchAction = new QWidgetAction(&menu);
    pSearchAction->setDefaultWidget(pSearch);
    menu.addAction(pSearchAction);
    QHash<QAction *, QString> actionIds;
    for (const UIMd3Tab &tab : displayTabs())
        if (tabRect(tab.strId).right() > width())
        {
            QAction *pAction = menu.addAction(tab.strLabel);
            pAction->setEnabled(tab.fEnabled);
            pAction->setStatusTip(tr("Open tab %1").arg(tab.strLabel));
            pAction->setWhatsThis(tr("Open tab %1").arg(tab.strLabel));
            if (!tab.fEnabled)
                pAction->setStatusTip(tr("Tab %1 is unavailable under the current manager restrictions")
                                       .arg(tab.strLabel));
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
    menu.exec(mapToGlobal(QPoint(width() - 44, height())));
}

void UIMd3TabStrip::showGroupPicker(const QString &strTabId)
{
    QDialog dialog(this, Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setObjectName(QStringLiteral("md3TabGroupPicker"));
    dialog.setWindowTitle(tr("Move tab into group"));
    dialog.setAccessibleName(tr("Move tab into group"));
    dialog.setMinimumSize(QSize(440, 420));
    dialog.setAutoFillBackground(true);
    QPalette palette = dialog.palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Base, md3(UIMd3ColorRole_SurfaceContainer));
    dialog.setPalette(palette);

    QVBoxLayout *pRootLayout = new QVBoxLayout(&dialog);
    pRootLayout->setContentsMargins(md3Theme().gutter(), md3Theme().gutter(),
                                    md3Theme().gutter(), md3Theme().gutter());
    pRootLayout->setSpacing(8);

    QLabel *pHeading = new QLabel(tr("Choose an existing group or create one"), &dialog);
    pHeading->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pRootLayout->addWidget(pHeading);

    UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-group-picker"),
                                                     tr("Search groups"), &dialog);
    pRootLayout->addWidget(pSearch);

    QLabel *pEmpty = new QLabel(tr("No groups yet. Create one below."), &dialog);
    pEmpty->setWordWrap(true);
    pEmpty->setAccessibleName(pEmpty->text());
    pRootLayout->addWidget(pEmpty);

    QListWidget *pGroups = new QListWidget(&dialog);
    pGroups->setObjectName(QStringLiteral("md3TabGroupPickerList"));
    pGroups->setAccessibleName(tr("Move target groups"));
    pGroups->setSelectionMode(QAbstractItemView::SingleSelection);
    pRootLayout->addWidget(pGroups, 1);

    QHBoxLayout *pCreateLayout = new QHBoxLayout;
    pCreateLayout->setContentsMargins(0, 0, 0, 0);
    QLineEdit *pNewGroup = new QLineEdit(&dialog);
    pNewGroup->setPlaceholderText(tr("New group name"));
    pNewGroup->setAccessibleName(tr("New group name"));
    QPushButton *pCreate = new QPushButton(tr("Create group"), &dialog);
    pCreate->setAccessibleName(tr("Create group"));
    pCreateLayout->addWidget(pNewGroup, 1);
    pCreateLayout->addWidget(pCreate);
    pRootLayout->addLayout(pCreateLayout);

    QDialogButtonBox *pButtons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                      Qt::Horizontal, &dialog);
    pButtons->button(QDialogButtonBox::Ok)->setText(tr("Move"));
    pButtons->button(QDialogButtonBox::Ok)->setAccessibleName(tr("Move tab into selected group"));
    pButtons->button(QDialogButtonBox::Cancel)->setAccessibleName(tr("Cancel moving tab"));
    pRootLayout->addWidget(pButtons);

    const auto populate = [this, pGroups, pEmpty, pSearch]()
    {
        const QString strQuery = pSearch->text();
        pGroups->clear();
        QListWidgetItem *pNoGroup = new QListWidgetItem(tr("No group (top level)"), pGroups);
        pNoGroup->setData(Qt::UserRole, QString());
        pNoGroup->setToolTip(tr("Keep this tab outside a group"));
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
                                        tr("%1 members").arg(cMembers));
            QListWidgetItem *pItem = new QListWidgetItem(strLabel, pGroups);
            pItem->setData(Qt::UserRole, group.strId);
            pItem->setToolTip(tr("%1, color %2, %3 members")
                              .arg(group.strName, group.color.name(QColor::HexRgb))
                              .arg(cMembers));
            ++cGroups;
        }
        pEmpty->setVisible(cGroups == 0);
        for (int i = 0; i < pGroups->count(); ++i)
        {
            QListWidgetItem *pItem = pGroups->item(i);
            pItem->setHidden(!strQuery.isEmpty() && !pSearch->matches(pItem->text()));
        }
        if (pGroups->currentRow() < 0)
            pGroups->setCurrentRow(0);
    };
    connect(pSearch, &UIMd3SearchField::sigFilterChanged, &dialog, populate);
    connect(pGroups, &QListWidget::currentRowChanged, &dialog,
            [pButtons](int iRow)
    {
        pButtons->button(QDialogButtonBox::Ok)->setEnabled(iRow >= 0);
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
            if (pGroups->item(i)->data(Qt::UserRole).toString() == strGroupId)
            {
                pGroups->setCurrentRow(i);
                break;
            }
    });
    connect(pButtons, &QDialogButtonBox::accepted, &dialog,
            [this, &dialog, pGroups, strTabId]()
    {
        QListWidgetItem *pItem = pGroups->currentItem();
        if (!pItem)
            return;
        moveToGroup(strTabId, pItem->data(Qt::UserRole).toString());
        dialog.accept();
    });
    connect(pButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    populate();
    pGroups->setCurrentRow(0);
    pSearch->setFocus(Qt::OtherFocusReason);
    dialog.exec();
}

void UIMd3TabStrip::mousePressEvent(QMouseEvent *pEvent)
{
    const QString strId = tabAt(pEvent->position().toPoint());
    if (pEvent->button() == Qt::LeftButton && !strId.isEmpty())
    {
        setCurrentTabId(strId);
        setFocus(Qt::MouseFocusReason);
        pEvent->accept();
        return;
    }
    if (pEvent->button() == Qt::LeftButton && strId.isEmpty() && !displayTabs().isEmpty()
        && tabRect(displayTabs().last().strId).right() > width())
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
    const QString strId = tabAt(pEvent->pos());
    if (strId.isEmpty())
    {
        QMenu menu(this);
        UIMd3SearchField *pSearch = new UIMd3SearchField(QStringLiteral("tab-groups"),
                                                         tr("Search tab strip"), &menu);
        QWidgetAction *pSearchAction = new QWidgetAction(&menu);
        pSearchAction->setDefaultWidget(pSearch);
        menu.addAction(pSearchAction);
        QHash<QAction *, QString> groupIds;
        for (const UIMd3TabGroup &group : m_groups)
        {
            QAction *pGroup = menu.addAction(group.fCollapsed
                                           ? tr("Expand %1").arg(group.strName)
                                           : tr("Collapse %1").arg(group.strName));
            pGroup->setStatusTip(tr("Toggle group %1").arg(group.strName));
            groupIds.insert(pGroup, group.strId);
        }
        QAction *pEdit = menu.addAction(tr("Edit appearance…"));
        pEdit->setStatusTip(tr("Edit appearance for the tab strip"));
        pEdit->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F10));
        connect(pSearch, &UIMd3SearchField::sigFilterChanged, &menu,
                [pSearch, groupIds, pEdit]() mutable
        {
            for (QAction *pGroup : groupIds.keys())
                pGroup->setVisible(pSearch->matches(pGroup->text()));
            pEdit->setVisible(pSearch->matches(pEdit->text()));
        });
        for (QAction *pGroup : groupIds.keys())
            connect(pGroup, &QAction::triggered, this, [this, pGroup, groupIds]()
            {
                toggleGroupCollapsed(groupIds.value(pGroup));
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
                                                     tr("Search tab actions"), &menu);
    QWidgetAction *pSearchAction = new QWidgetAction(&menu);
    pSearchAction->setDefaultWidget(pSearch);
    menu.addAction(pSearchAction);
    QAction *pPin = menu.addAction(selected.fPinned ? tr("Unpin tab") : tr("Pin tab"));
    QAction *pMove = menu.addAction(tr("Move… into group…"));
    QAction *pClose = menu.addAction(tr("Close tab"));
    QAction *pEdit = menu.addAction(tr("Edit tab appearance…"));
    if (selected.fPinned)
    {
        pClose->setEnabled(false);
        pClose->setStatusTip(tr("Unpin this tab before closing it"));
    }
    pEdit->setStatusTip(tr("Edit appearance for %1").arg(selected.strLabel));
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
    menu.exec(pEvent->globalPos());
    pEvent->accept();
}
