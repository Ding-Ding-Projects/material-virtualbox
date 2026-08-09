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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3TabStrip_h
#define FEQT_INCLUDED_SRC_md3_UIMd3TabStrip_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QColor>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include "UILibraryDefs.h"
#include "UIMd3Widget.h"

class QContextMenuEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QToolButton;
class QWidget;
class UIMd3TabManager;

struct UIMd3Tab
{
    QString strId;
    QString strLabel;
    QString strGroupId;
    bool    fPinned;
    bool    fEnabled;
};

struct UIMd3TabGroup
{
    QString strId;
    QString strName;
    QColor  color;
    bool    fCollapsed;
};

/** A destination offered by the persistent new-tab control. */
struct UIMd3TabChoice
{
    QString strId;
    QString strLabel;
    bool    fEnabled;
};

/** Shared browser-style tab model and native strip renderer. */
class SHARED_LIBRARY_STUFF UIMd3TabStrip : public UIMd3Widget
{
    Q_OBJECT;

signals:

    void sigCurrentChanged(const QString &strId);
    void sigModelChanged();

public:

    explicit UIMd3TabStrip(QWidget *pParent = 0,
                           const QString &strPersistenceScope = QString());
    virtual ~UIMd3TabStrip() RT_OVERRIDE;

    void openTab(const QString &strId, const QString &strLabel);
    void setTabLabel(const QString &strId, const QString &strLabel);
    void setTabEnabled(const QString &strId, bool fEnabled);
    void setTabsEnabled(const QHash<QString, bool> &states);
    void setAvailableTabs(const QList<UIMd3TabChoice> &choices);
    bool migrateLegacyGeneratedLayout(const QStringList &expectedIds,
                                      const QString &strInitialId,
                                      const QString &strInitialLabel);
    bool closeTab(const QString &strId, bool fForce = false);
    QString currentTabId() const { return m_strCurrentId; }
    void setCurrentTabId(const QString &strId);

    QList<UIMd3Tab> tabs() const { return m_tabs; }
    QList<UIMd3TabGroup> groups() const { return m_groups; }

    QString createGroup(const QString &strName);
    void renameGroup(const QString &strGroupId, const QString &strName);
    void moveToGroup(const QString &strTabId, const QString &strGroupId);
    void toggleGroupCollapsed(const QString &strGroupId);
    void togglePinned(const QString &strTabId);

    QList<UIMd3Tab> resolveCloseSet(const QString &strQuery, bool fInverse,
                                    bool fRegex, bool fIncludePinned,
                                    const QString &strRegexFlags = QString()) const;
    void save() const;
    void restore();

protected:

    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    virtual void contextMenuEvent(QContextMenuEvent *pEvent) RT_OVERRIDE;
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;

private:

    friend class UIMd3TabManager;

    QList<UIMd3Tab> displayTabs() const;
    QList<UIMd3Tab> unpinnedDisplayTabs() const;
    bool hasOverflow() const;
    int tabContentRight() const;
    int tabWidth(const UIMd3Tab &tab) const;
    bool isTabFullyVisible(const QString &strId) const;
    void normalizeViewport();
    void ensureTabVisible(const QString &strId);
    QRect tabRect(const QString &strId) const;
    QRect tabCloseRect(const QString &strId) const;
    QString tabAt(const QPoint &position) const;
    void showOverflowMenu();
    void showNewTabMenu();
    void showTabManagerMenu();
    void showTabActions(const QString &strId, const QPoint &globalPosition);
    void showGroupPicker(const QString &strTabId);
    void announceModelChanged();
    void updateOverflowButton();
    void syncAccessibleTabButtons();
    void retranslateUi();
    void updateTheme();

    QList<UIMd3Tab>      m_tabs;
    QList<UIMd3TabGroup> m_groups;
    QList<UIMd3TabChoice> m_availableTabs;
    QString              m_strCurrentId;
    QString              m_strPersistenceKey;
    bool                 m_fRestoredLegacyPersistence;
    int                  m_iFirstVisiblePinned;
    int                  m_iFirstVisibleUnpinned;
    QHash<QString, QToolButton*> m_tabButtons;
    QHash<QString, QToolButton*> m_closeButtons;
    QWidget             *m_pTabList;
    QToolButton         *m_pOverflowButton;
    QToolButton         *m_pNewTabButton;
    QToolButton         *m_pTabManagerButton;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3TabStrip_h */
