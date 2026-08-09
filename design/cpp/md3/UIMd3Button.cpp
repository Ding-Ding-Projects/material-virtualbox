/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Button class implementation.
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
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>

/* GUI includes: */
#include "UIMd3Button.h"

UIMd3Button::UIMd3Button(const QString &strText, UIMd3ButtonVariant enmVariant /* = UIMd3ButtonVariant_Tonal */, QWidget *pParent /* = 0 */)
    : UIMd3Widget(pParent, QString("button/%1").arg(strText))
    , m_strText(strText)
    , m_enmVariant(enmVariant)
{
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}

void UIMd3Button::setText(const QString &strText)
{
    m_strText = strText;
    setAppearanceKey(QString("button/%1").arg(strText));
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
    m_enmVariant = enmVariant;
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
        default: break;
    }
    return UIMd3ColorRole_Surface;
}

UIMd3ColorRole UIMd3Button::labelRole() const
{
    switch (m_enmVariant)
    {
        case UIMd3ButtonVariant_Filled:   return UIMd3ColorRole_OnPrimary;
        case UIMd3ButtonVariant_Tonal:    return UIMd3ColorRole_OnSecondaryContainer;
        case UIMd3ButtonVariant_Danger:   return UIMd3ColorRole_OnErrorContainer;
        case UIMd3ButtonVariant_Icon:     return UIMd3ColorRole_OnSurfaceVariant;
        default: break;
    }
    return UIMd3ColorRole_Primary;
}

QSize UIMd3Button::sizeHint() const
{
    const QFontMetrics metrics(effectiveFont(UIMd3TypeRole_LabelLarge));
    const int iIconWidth = m_icon.isNull() ? 0 : 18 + 8;
    if (m_enmVariant == UIMd3ButtonVariant_Icon)
        return QSize(md3Theme().controlHeight(), md3Theme().controlHeight());
    return QSize(metrics.horizontalAdvance(m_strText) + 48 + iIconWidth, md3Theme().controlHeight());
}

QSize UIMd3Button::minimumSizeHint() const
{
    /* Never go below the documented 48px touch target on either axis: */
    const QSize hint = sizeHint();
    return QSize(qMax(48, hint.width()), qMax(40, hint.height()));
}

void UIMd3Button::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    const QRect body = rect().adjusted(0, 0, -1, -1);
    const int iRadius = m_enmVariant == UIMd3ButtonVariant_Icon ? height() / 2 : qMin(effectiveRadius() + 4, height() / 2);

    /* Container: */
    if (m_enmVariant != UIMd3ButtonVariant_Text && m_enmVariant != UIMd3ButtonVariant_Outlined && m_enmVariant != UIMd3ButtonVariant_Icon)
    {
        QColor container = md3(containerRole());
        if (m_enmVariant == UIMd3ButtonVariant_Filled)
        {
            const QColor accent = effectiveAccent();
            if (accent.isValid())
                container = accent;
        }
        if (!isEnabled())
            container.setAlpha(UIMd3StateLayer::Disabled * 255 / 100);
        QPainterPath path;
        path.addRoundedRect(body, iRadius, iRadius);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillPath(path, container);
    }

    /* Outline: */
    if (m_enmVariant == UIMd3ButtonVariant_Outlined)
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(md3(UIMd3ColorRole_Outline), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(body, iRadius, iRadius);
    }

    /* State layer: */
    if (isEnabled())
        paintStateLayer(painter, body, labelRole(), iRadius);

    /* Icon and label: */
    QColor label = md3(labelRole());
    if (!isEnabled())
        label.setAlpha(UIMd3StateLayer::Disabled * 255 / 100);
    painter.setPen(label);
    painter.setFont(effectiveFont(UIMd3TypeRole_LabelLarge));

    QRect content = body;
    if (!m_icon.isNull())
    {
        const QRect iconRect(content.left() + 16, content.center().y() - 9, 18, 18);
        m_icon.paint(&painter, iconRect, Qt::AlignCenter, isEnabled() ? QIcon::Normal : QIcon::Disabled);
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
    if (isEnabled() && (pEvent->key() == Qt::Key_Space || pEvent->key() == Qt::Key_Return || pEvent->key() == Qt::Key_Enter))
    {
        emit sigClicked();
        pEvent->accept();
        return;
    }
    UIMd3Widget::keyPressEvent(pEvent);
}
