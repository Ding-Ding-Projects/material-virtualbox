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
#include <QList>
#include <QString>

#include "UILibraryDefs.h"
#include "UIMd3Widget.h"

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

/** Shared browser-style tab model and native strip renderer. */
class SHARED_LIBRARY_STUFF UIMd3TabStrip : public UIMd3Widget
{
    Q_OBJECT;

signals:

    void sigCurrentChanged(const QString &strId);
    void sigModelChanged();

public:

    explicit UIMd3TabStrip(QWidget *pParent = 0);

    void openTab(const QString &strId, const QString &strLabel);
    void setTabLabel(const QString &strId, const QString &strLabel);
    void setTabEnabled(const QString &strId, bool fEnabled);
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
                                    bool fRegex, bool fIncludePinned) const;
    void save() const;
    void restore();

protected:

    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    virtual void contextMenuEvent(QContextMenuEvent *pEvent) RT_OVERRIDE;

private:

    QList<UIMd3Tab> displayTabs() const;
    QRect tabRect(const QString &strId) const;
    QString tabAt(const QPoint &position) const;
    void showOverflowMenu();
    void announceModelChanged();

    QList<UIMd3Tab>      m_tabs;
    QList<UIMd3TabGroup> m_groups;
    QString              m_strCurrentId;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3TabStrip_h */
