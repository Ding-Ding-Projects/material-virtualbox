/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Widget class implementation.
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
#include <QMouseEvent>

/* GUI includes: */
#include "UIMd3AppearanceEditor.h"
#include "UIMd3Widget.h"

UIMd3Widget::UIMd3Widget(QWidget *pParent /* = 0 */, const QString &strAppearanceKey /* = QString() */)
    : QWidget(pParent)
    , m_fHovered(false)
    , m_fPressed(false)
    , m_strAppearanceKey(strAppearanceKey)
{
    setAttribute(Qt::WA_Hover);
    connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged, this, &UIMd3Widget::sltThemeChanged);
}

void UIMd3Widget::setAppearanceKey(const QString &strKey)
{
    m_strAppearanceKey = strKey;
    update();
}

int UIMd3Widget::effectiveRadius() const
{
    const UIMd3Appearance appearance = md3Theme().appearance(m_strAppearanceKey);
    return appearance.fValid ? appearance.iRadius : UIMd3Shape::Large;
}

QFont UIMd3Widget::effectiveFont(UIMd3TypeRole enmRole) const
{
    QFont result = md3Theme().font(enmRole);
    const UIMd3Appearance appearance = md3Theme().appearance(m_strAppearanceKey);
    if (appearance.fValid)
    {
        if (!appearance.strFont.isEmpty())
            result.setFamily(appearance.strFont);
        result.setPixelSize(qMax(9, (int)(result.pixelSize() * appearance.dScale)));
        result.setWeight((QFont::Weight)appearance.iWeight);
    }
    return result;
}

QColor UIMd3Widget::effectiveAccent() const
{
    const UIMd3Appearance appearance = md3Theme().appearance(m_strAppearanceKey);
    if (appearance.fValid && appearance.seed.isValid())
        return appearance.seed;
    return md3(UIMd3ColorRole_Primary);
}

void UIMd3Widget::paintContainer(QPainter &painter, const QRect &rect, UIMd3ColorRole enmRole, int iRadius /* = -1 */) const
{
    const int iEffective = iRadius >= 0 ? iRadius : effectiveRadius();
    QPainterPath path;
    path.addRoundedRect(rect, iEffective, iEffective);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, md3(enmRole));
}

void UIMd3Widget::paintStateLayer(QPainter &painter, const QRect &rect, UIMd3ColorRole enmRole, int iRadius /* = -1 */) const
{
    int iOpacity = 0;
    if (m_fPressed)
        iOpacity = UIMd3StateLayer::Pressed;
    else if (m_fHovered)
        iOpacity = UIMd3StateLayer::Hover;
    else if (hasFocus())
        iOpacity = UIMd3StateLayer::Focus;
    if (iOpacity == 0)
        return;

    const int iEffective = iRadius >= 0 ? iRadius : effectiveRadius();
    QPainterPath path;
    path.addRoundedRect(rect, iEffective, iEffective);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, md3Theme().stateLayer(enmRole, iOpacity));
}

void UIMd3Widget::paintFocusRing(QPainter &painter, const QRect &rect, int iRadius /* = -1 */) const
{
    if (!hasFocus())
        return;
    const int iEffective = iRadius >= 0 ? iRadius : effectiveRadius();
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(md3(UIMd3ColorRole_Primary));
    pen.setWidth(3);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect.adjusted(2, 2, -2, -2), iEffective, iEffective);
}

void UIMd3Widget::enterEvent(QEnterEvent *pEvent)
{
    m_fHovered = true;
    update();
    QWidget::enterEvent(pEvent);
}

void UIMd3Widget::leaveEvent(QEvent *pEvent)
{
    m_fHovered = false;
    m_fPressed = false;
    update();
    QWidget::leaveEvent(pEvent);
}

void UIMd3Widget::mousePressEvent(QMouseEvent *pEvent)
{
    /* Shift+right-click is the documented direct route into the anchored appearance editor: */
    if (pEvent->button() == Qt::RightButton && (pEvent->modifiers() & Qt::ShiftModifier) && !m_strAppearanceKey.isEmpty())
    {
        UIMd3AppearanceEditor::editElement(this, m_strAppearanceKey);
        pEvent->accept();
        return;
    }
    if (pEvent->button() == Qt::LeftButton)
    {
        m_fPressed = true;
        update();
    }
    QWidget::mousePressEvent(pEvent);
}

void UIMd3Widget::mouseReleaseEvent(QMouseEvent *pEvent)
{
    m_fPressed = false;
    update();
    QWidget::mouseReleaseEvent(pEvent);
}

void UIMd3Widget::sltThemeChanged()
{
    setFont(effectiveFont(UIMd3TypeRole_BodyMedium));
    update();
}
