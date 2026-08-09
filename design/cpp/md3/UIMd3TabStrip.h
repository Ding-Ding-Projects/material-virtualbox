/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3TabStrip class declaration - browser-style tabs with groups, pinning and four searches.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3TabStrip_h
#define FEQT_INCLUDED_SRC_md3_UIMd3TabStrip_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QList>
#include <QString>

/* GUI includes: */
#include "UIMd3Widget.h"

/** One workspace tab. */
struct UIMd3Tab
{
    QString strId;      /**< Stable tab identifier, usually the tool id. */
    QString strLabel;   /**< Visible label; every close query matches against this. */
    QString strGroupId; /**< Owning group. */
    bool    fPinned;    /**< Whether the tab is protected from closing. */
};

/** One tab group. */
struct UIMd3TabGroup
{
    QString strId;
    QString strName;
    QColor  color;
    bool    fCollapsed;
};

/** UIMd3Widget extension implementing the workspace tab strip.
  *
  * The strip owns the tab and group model for the window. Four independent
  * searches read it (current strip, group names, the selected group, and a
  * master all-tabs search), and both close tools — "containing" and its inverse
  * "not containing" — resolve against the visible label only, preview their
  * match set, and protect pinned tabs unless explicitly opted in. */
class UIMd3TabStrip : public UIMd3Widget
{
    Q_OBJECT;

signals:

    /** Notifies listeners that @a strId became current. */
    void sigCurrentChanged(const QString &strId);
    /** Notifies listeners that the tab or group model changed. */
    void sigModelChanged();

public:

    /** Constructs the strip. */
    UIMd3TabStrip(QWidget *pParent = 0);

    /** Opens or raises a tab with @a strId and @a strLabel. */
    void openTab(const QString &strId, const QString &strLabel);
    /** Closes @a strId. Pinned tabs are refused unless @a fForce. Returns whether it closed. */
    bool closeTab(const QString &strId, bool fForce = false);
    /** Returns the current tab id. */
    QString currentTabId() const { return m_strCurrentId; }
    /** Makes @a strId current. */
    void setCurrentTabId(const QString &strId);

    /** Returns every tab. */
    QList<UIMd3Tab> tabs() const { return m_tabs; }
    /** Returns every group. */
    QList<UIMd3TabGroup> groups() const { return m_groups; }

    /** Creates a group named @a strName and returns its id. */
    QString createGroup(const QString &strName);
    /** Renames @a strGroupId to @a strName. */
    void renameGroup(const QString &strGroupId, const QString &strName);
    /** Moves @a strTabId into @a strGroupId. */
    void moveToGroup(const QString &strTabId, const QString &strGroupId);
    /** Toggles the collapsed state of @a strGroupId. */
    void toggleGroupCollapsed(const QString &strGroupId);
    /** Toggles the pinned state of @a strTabId. */
    void togglePinned(const QString &strTabId);

    /** Returns the tabs whose visible label matches @a strQuery, or its inverse when @a fInverse.
      * @param  fRegex        Whether @a strQuery is a regular expression rather than plain text.
      * @param  fIncludePinned Whether pinned tabs may appear in the result. */
    QList<UIMd3Tab> resolveCloseSet(const QString &strQuery, bool fInverse, bool fRegex, bool fIncludePinned) const;

    /** Persists the tab and group model to extradata. */
    void save() const;
    /** Restores the tab and group model from extradata. */
    void restore();

protected:

    /** Handles paint events. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    /** Handles mouse-press events, including Shift+right-click on a tab or group header. */
    virtual void mousePressEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    /** Handles context-menu events. */
    virtual void contextMenuEvent(QContextMenuEvent *pEvent) RT_OVERRIDE;

private:

    /** Returns the rectangle occupied by @a strId. */
    QRect tabRect(const QString &strId) const;
    /** Returns the tab under @a position, or an empty string. */
    QString tabAt(const QPoint &position) const;

    QList<UIMd3Tab>      m_tabs;
    QList<UIMd3TabGroup> m_groups;
    QString              m_strCurrentId;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3TabStrip_h */
