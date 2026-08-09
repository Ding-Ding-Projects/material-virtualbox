/* $Id$ */
/** @file
 * VBox Qt GUI - shared token-aware base for Material 3 widgets.
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

#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QWidgetAction>

#include "UIMd3AppearanceEditor.h"
#include "UIMd3Widget.h"

UIMd3Widget::UIMd3Widget(QWidget *pParent, const QString &strAppearanceKey)
    : QWidget(pParent)
    , m_fHovered(false)
    , m_fPressed(false)
    , m_strAppearanceKey(strAppearanceKey)
{
    setAttribute(Qt::WA_Hover);
    if (UIMd3Theme::instance())
        connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
                this, &UIMd3Widget::sltThemeChanged);
}

void UIMd3Widget::setAppearanceKey(const QString &strKey)
{
    if (m_strAppearanceKey == strKey)
        return;
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
        result.setPixelSize(qMax(9, qRound(result.pixelSize() * appearance.dScale)));
        if (appearance.iWeight >= static_cast<int>(QFont::Thin))
            result.setWeight(static_cast<QFont::Weight>(qBound(static_cast<int>(QFont::Thin),
                                                                appearance.iWeight,
                                                                static_cast<int>(QFont::Black))));
    }
    return result;
}

QColor UIMd3Widget::effectiveAccent() const
{
    const UIMd3Appearance appearance = md3Theme().appearance(m_strAppearanceKey);
    return appearance.fValid && appearance.seed.isValid()
         ? appearance.seed : md3(UIMd3ColorRole_Primary);
}

void UIMd3Widget::paintContainer(QPainter &painter, const QRect &rect,
                                 UIMd3ColorRole enmRole, int iRadius) const
{
    const int iEffective = iRadius >= 0 ? iRadius : effectiveRadius();
    QPainterPath path;
    path.addRoundedRect(rect, iEffective, iEffective);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, md3(enmRole));
}

void UIMd3Widget::paintStateLayer(QPainter &painter, const QRect &rect,
                                  UIMd3ColorRole enmRole, int iRadius) const
{
    int iOpacity = 0;
    if (m_fPressed)
        iOpacity = UIMd3StateLayer::Pressed;
    else if (m_fHovered)
        iOpacity = UIMd3StateLayer::Hover;
    else if (hasFocus())
        iOpacity = UIMd3StateLayer::Focus;
    if (!iOpacity)
        return;

    const int iEffective = iRadius >= 0 ? iRadius : effectiveRadius();
    QPainterPath path;
    path.addRoundedRect(rect, iEffective, iEffective);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, md3Theme().stateLayer(enmRole, iOpacity));
}

void UIMd3Widget::paintFocusRing(QPainter &painter, const QRect &rect, int iRadius) const
{
    if (!hasFocus())
        return;
    const int iEffective = iRadius >= 0 ? iRadius : effectiveRadius();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(md3(UIMd3ColorRole_Primary), 3));
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
    if (pEvent->button() == Qt::RightButton &&
        (pEvent->modifiers() & Qt::ShiftModifier) && !m_strAppearanceKey.isEmpty())
    {
        emit sigAppearanceEditRequested(m_strAppearanceKey);
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

void UIMd3Widget::contextMenuEvent(QContextMenuEvent *pEvent)
{
    if (m_strAppearanceKey.isEmpty())
    {
        QWidget::contextMenuEvent(pEvent);
        return;
    }

    QMenu menu(this);
    QLineEdit *pSearch = new QLineEdit(&menu);
    pSearch->setPlaceholderText(tr("Search menu"));
    pSearch->setAccessibleName(tr("Search appearance menu"));
    QWidgetAction *pSearchAction = new QWidgetAction(&menu);
    pSearchAction->setDefaultWidget(pSearch);
    menu.addAction(pSearchAction);
    pSearch->setFocus(Qt::OtherFocusReason);
    QAction *pEdit = menu.addAction(tr("Edit appearance…"));
    pEdit->setStatusTip(tr("Edit appearance for %1").arg(m_strAppearanceKey));
    pEdit->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F10));
    connect(pSearch, &QLineEdit::textChanged, this, [pEdit](const QString &strText)
    {
        pEdit->setVisible(strText.trimmed().isEmpty()
                          || pEdit->text().contains(strText, Qt::CaseInsensitive));
    });
    connect(pEdit, &QAction::triggered, this, [this]()
    {
        UIMd3AppearanceEditor::open(this, m_strAppearanceKey);
    });
    menu.exec(pEvent->globalPos());
    pEvent->accept();
}

void UIMd3Widget::sltThemeChanged()
{
    setFont(effectiveFont(UIMd3TypeRole_BodyMedium));
    updateGeometry();
    update();
}
