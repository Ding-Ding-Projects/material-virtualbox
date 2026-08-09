/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Widget class declaration - shared base for every Material 3 widget.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Widget_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Widget_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QPainter>
#include <QPainterPath>
#include <QString>
#include <QWidget>

/* GUI includes: */
#include "UIMd3Theme.h"
#include "UIMd3Tokens.h"

/** Base class of every Material 3 widget in the frontend.
  *
  * It provides three things every M3 surface needs:
  *   - an appearance key, so the anchored appearance editor can address it;
  *   - state-layer painting (hover, focus, pressed) on top of any container;
  *   - automatic repaint when the theme, density or an override changes. */
class UIMd3Widget : public QWidget
{
    Q_OBJECT;

public:

    /** Constructs the widget passing @a pParent to the base-class.
      * @param  strAppearanceKey  Stable identifier used by the appearance editor. */
    UIMd3Widget(QWidget *pParent = 0, const QString &strAppearanceKey = QString());

    /** Returns the appearance key of this element. */
    QString appearanceKey() const { return m_strAppearanceKey; }
    /** Defines the appearance key of this element. */
    void setAppearanceKey(const QString &strKey);

    /** Returns the effective corner radius, honouring any override. */
    int effectiveRadius() const;
    /** Returns the effective font for @a enmRole, honouring any override. */
    QFont effectiveFont(UIMd3TypeRole enmRole) const;
    /** Returns the effective accent, honouring any override. */
    QColor effectiveAccent() const;

protected:

    /** Paints a rounded container of @a rect filled with @a role. */
    void paintContainer(QPainter &painter, const QRect &rect, UIMd3ColorRole enmRole, int iRadius = -1) const;
    /** Paints the hover/focus/pressed state layer over @a rect. */
    void paintStateLayer(QPainter &painter, const QRect &rect, UIMd3ColorRole enmRole, int iRadius = -1) const;
    /** Paints the Material 3 focus ring around @a rect when this widget has keyboard focus. */
    void paintFocusRing(QPainter &painter, const QRect &rect, int iRadius = -1) const;

    /** Handles enter events, to raise the hover state. */
    virtual void enterEvent(QEnterEvent *pEvent) RT_OVERRIDE;
    /** Handles leave events, to drop the hover state. */
    virtual void leaveEvent(QEvent *pEvent) RT_OVERRIDE;
    /** Handles mouse-press events, to raise the pressed state and open the appearance editor on Shift+right-click. */
    virtual void mousePressEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    /** Handles mouse-release events, to drop the pressed state. */
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) RT_OVERRIDE;

    bool m_fHovered;   /**< Whether the pointer is inside this widget. */
    bool m_fPressed;   /**< Whether a mouse button is held on this widget. */

private slots:

    /** Handles theme changes. */
    void sltThemeChanged();

private:

    QString m_strAppearanceKey;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Widget_h */
