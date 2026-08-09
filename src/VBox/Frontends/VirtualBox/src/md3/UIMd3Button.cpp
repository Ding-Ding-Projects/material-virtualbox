/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 button variants.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QKeyEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

#include "UIMd3Button.h"

UIMd3Button::UIMd3Button(const QString &strText, UIMd3ButtonVariant enmVariant, QWidget *pParent)
    : UIMd3Widget(pParent, QStringLiteral("button/") + strText)
    , m_strText(strText)
    , m_enmVariant(enmVariant)
{
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setAccessibleName(strText);
}

void UIMd3Button::setText(const QString &strText)
{
    m_strText = strText;
    setAccessibleName(strText);
    setAppearanceKey(QStringLiteral("button/") + strText);
    updateGeometry();
    update();
}

void UIMd3Button::setIcon(const QIcon &icon)
{
    m_icon = icon;
    updateGeometry();
    update();
}

void UIMd3Button::setVariant(UIMd3ButtonVariant enmVariant)
{
    if (m_enmVariant == enmVariant)
        return;
    m_enmVariant = enmVariant;
    updateGeometry();
    update();
}

void UIMd3Button::setEnabledState(bool fEnabled)
{
    setEnabled(fEnabled);
    setCursor(fEnabled ? Qt::PointingHandCursor : Qt::ForbiddenCursor);
    update();
}

UIMd3ColorRole UIMd3Button::containerRole() const
{
    switch (m_enmVariant)
    {
        case UIMd3ButtonVariant_Filled:   return UIMd3ColorRole_Primary;
        case UIMd3ButtonVariant_Tonal:    return UIMd3ColorRole_SecondaryContainer;
        case UIMd3ButtonVariant_Elevated: return UIMd3ColorRole_SurfaceContainerLow;
        case UIMd3ButtonVariant_Danger:   return UIMd3ColorRole_ErrorContainer;
        default:                          return UIMd3ColorRole_Surface;
    }
}

UIMd3ColorRole UIMd3Button::labelRole() const
{
    switch (m_enmVariant)
    {
        case UIMd3ButtonVariant_Filled: return UIMd3ColorRole_OnPrimary;
        case UIMd3ButtonVariant_Tonal:  return UIMd3ColorRole_OnSecondaryContainer;
        case UIMd3ButtonVariant_Danger: return UIMd3ColorRole_OnErrorContainer;
        case UIMd3ButtonVariant_Icon:   return UIMd3ColorRole_OnSurfaceVariant;
        default:                        return UIMd3ColorRole_Primary;
    }
}

QSize UIMd3Button::sizeHint() const
{
    const QFontMetrics metrics(effectiveFont(UIMd3TypeRole_LabelLarge));
    if (m_enmVariant == UIMd3ButtonVariant_Icon)
        return QSize(md3Theme().controlHeight(), md3Theme().controlHeight());
    const int iIconWidth = m_icon.isNull() ? 0 : 26;
    return QSize(metrics.horizontalAdvance(m_strText) + 48 + iIconWidth,
                 md3Theme().controlHeight());
}

QSize UIMd3Button::minimumSizeHint() const
{
    const QSize hint = sizeHint();
    return QSize(qMax(48, hint.width()), qMax(48, hint.height()));
}

void UIMd3Button::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const QRect body = rect().adjusted(0, 0, -1, -1);
    const int iRadius = m_enmVariant == UIMd3ButtonVariant_Icon
                      ? height() / 2 : qMin(effectiveRadius() + 4, height() / 2);

    if (m_enmVariant != UIMd3ButtonVariant_Text &&
        m_enmVariant != UIMd3ButtonVariant_Outlined &&
        m_enmVariant != UIMd3ButtonVariant_Icon)
    {
        QColor container = md3(containerRole());
        if (m_enmVariant == UIMd3ButtonVariant_Filled && effectiveAccent().isValid())
            container = effectiveAccent();
        if (!isEnabled())
            container.setAlpha(UIMd3StateLayer::Disabled * 255 / 100);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(body, iRadius, iRadius);
        painter.fillPath(path, container);
    }

    if (m_enmVariant == UIMd3ButtonVariant_Outlined)
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(md3(UIMd3ColorRole_Outline), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(body, iRadius, iRadius);
    }
    if (isEnabled())
        paintStateLayer(painter, body, labelRole(), iRadius);

    QColor label = md3(labelRole());
    if (!isEnabled())
        label.setAlpha(UIMd3StateLayer::Disabled * 255 / 100);
    painter.setPen(label);
    painter.setFont(effectiveFont(UIMd3TypeRole_LabelLarge));
    QRect content = body;
    if (!m_icon.isNull())
    {
        const QRect iconRect(content.left() + 16, content.center().y() - 9, 18, 18);
        m_icon.paint(&painter, iconRect, Qt::AlignCenter,
                     isEnabled() ? QIcon::Normal : QIcon::Disabled);
        content.setLeft(iconRect.right() + 8);
    }
    if (m_enmVariant != UIMd3ButtonVariant_Icon)
        painter.drawText(content, Qt::AlignCenter, m_strText);
    paintFocusRing(painter, body, iRadius);
}

void UIMd3Button::mouseReleaseEvent(QMouseEvent *pEvent)
{
    const bool fInside = rect().contains(pEvent->pos());
    UIMd3Widget::mouseReleaseEvent(pEvent);
    if (fInside && isEnabled() && pEvent->button() == Qt::LeftButton)
        emit sigClicked();
}

void UIMd3Button::keyPressEvent(QKeyEvent *pEvent)
{
    if (isEnabled() && (pEvent->key() == Qt::Key_Space ||
                        pEvent->key() == Qt::Key_Return ||
                        pEvent->key() == Qt::Key_Enter))
    {
        emit sigClicked();
        pEvent->accept();
        return;
    }
    UIMd3Widget::keyPressEvent(pEvent);
}
