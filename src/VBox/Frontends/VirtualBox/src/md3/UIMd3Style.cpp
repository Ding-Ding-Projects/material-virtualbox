/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Style implementation.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, in version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QProgressBar>
#include <QScrollBar>
#include <QSlider>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionHeader>
#include <QStyleOptionMenuItem>
#include <QStyleOptionProgressBar>
#include <QStyleOptionSpinBox>
#include <QStyleOptionSlider>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QTabBar>
#include <QToolButton>
#include <QWidget>

/* GUI includes: */
#include "UIMd3Style.h"
#include "UIMd3Theme.h"

/** Returns whether @a pOption represents an enabled control. */
static bool md3StyleIsEnabled(const QStyleOption *pOption)
{
    return pOption && pOption->state.testFlag(QStyle::State_Enabled);
}

/** Returns the Material state-layer opacity represented by @a pOption. */
static int md3StyleStateOpacity(const QStyleOption *pOption)
{
    if (!pOption)
        return 0;
    if (pOption->state.testFlag(QStyle::State_Sunken))
        return UIMd3StateLayer::Pressed;
    if (pOption->state.testFlag(QStyle::State_MouseOver))
        return UIMd3StateLayer::Hover;
    if (pOption->state.testFlag(QStyle::State_HasFocus))
        return UIMd3StateLayer::Focus;
    return 0;
}

/** Returns @a color with Material disabled opacity applied. */
static QColor md3StyleEnabledColor(const QColor &color, bool fEnabled)
{
    QColor result(color);
    if (!fEnabled)
        result.setAlphaF(result.alphaF() * UIMd3StateLayer::Disabled / 100.0);
    return result;
}

/** Returns the opaque result of applying a Material state layer over a base role. */
static QColor md3StyleStateColor(UIMd3ColorRole enmBaseRole, UIMd3ColorRole enmLayerRole,
                                 int iOpacityPercent)
{
    const QColor base = md3(enmBaseRole);
    const QColor layer = md3(enmLayerRole);
    const qreal dOpacity = qBound(0, iOpacityPercent, 100) / 100.0;
    return QColor(qRound(base.red()   * (1.0 - dOpacity) + layer.red()   * dOpacity),
                  qRound(base.green() * (1.0 - dOpacity) + layer.green() * dOpacity),
                  qRound(base.blue()  * (1.0 - dOpacity) + layer.blue()  * dOpacity),
                  base.alpha());
}

/** Draws a rounded Material surface in @a rect. */
static void md3StyleDrawSurface(QPainter *pPainter, const QRectF &rect, const QColor &fill,
                                const QColor &outline = QColor(), qreal dRadius = UIMd3Shape::Small)
{
    pPainter->save();
    pPainter->setRenderHint(QPainter::Antialiasing);
    pPainter->setBrush(fill);
    pPainter->setPen(outline.isValid() ? QPen(outline, 1) : Qt::NoPen);
    pPainter->drawRoundedRect(rect, dRadius, dRadius);
    pPainter->restore();
}

/** Draws one Material focus ring around @a rect. */
static void md3StyleDrawFocus(QPainter *pPainter, const QRectF &rect, qreal dRadius)
{
    pPainter->save();
    pPainter->setRenderHint(QPainter::Antialiasing);
    pPainter->setBrush(Qt::NoBrush);
    pPainter->setPen(QPen(md3(UIMd3ColorRole_Primary), 2));
    pPainter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), dRadius, dRadius);
    pPainter->restore();
}

/** Draws a compact chevron in @a rect. */
static void md3StyleDrawChevron(QPainter *pPainter, const QRect &rect, Qt::ArrowType enmArrow,
                                const QColor &color)
{
    const QPointF center = rect.center();
    QPainterPath path;
    switch (enmArrow)
    {
        case Qt::UpArrow:
            path.moveTo(center.x() - 4, center.y() + 2);
            path.lineTo(center.x(), center.y() - 2);
            path.lineTo(center.x() + 4, center.y() + 2);
            break;
        case Qt::DownArrow:
            path.moveTo(center.x() - 4, center.y() - 2);
            path.lineTo(center.x(), center.y() + 2);
            path.lineTo(center.x() + 4, center.y() - 2);
            break;
        case Qt::LeftArrow:
            path.moveTo(center.x() + 2, center.y() - 4);
            path.lineTo(center.x() - 2, center.y());
            path.lineTo(center.x() + 2, center.y() + 4);
            break;
        case Qt::RightArrow:
            path.moveTo(center.x() - 2, center.y() - 4);
            path.lineTo(center.x() + 2, center.y());
            path.lineTo(center.x() - 2, center.y() + 4);
            break;
        default:
            return;
    }
    pPainter->save();
    pPainter->setRenderHint(QPainter::Antialiasing);
    pPainter->setBrush(Qt::NoBrush);
    pPainter->setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    pPainter->drawPath(path);
    pPainter->restore();
}

UIMd3Style::UIMd3Style()
    : QProxyStyle(QApplication::style())
{
}

void UIMd3Style::install()
{
    if (!qApp || qApp->style()->objectName() == QStringLiteral("md3"))
        return;
    UIMd3Style *pStyle = new UIMd3Style;
    pStyle->setObjectName(QStringLiteral("md3"));
    qApp->setStyle(pStyle);
    if (UIMd3Theme::instance())
        QObject::connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
                         pStyle, [pStyle]()
        {
            if (!qApp)
                return;
            QPalette palette = qApp->palette();
            pStyle->polish(palette);
            qApp->setPalette(palette);
            qApp->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
            foreach (QWidget *pWidget, qApp->allWidgets())
            {
                pWidget->updateGeometry();
                pWidget->update();
            }
        });
}

void UIMd3Style::polish(QApplication *pApplication)
{
    QProxyStyle::polish(pApplication);
    if (!pApplication || !UIMd3Theme::instance())
        return;
    QPalette palette = pApplication->palette();
    polish(palette);
    pApplication->setPalette(palette);
    pApplication->setFont(md3Theme().font(UIMd3TypeRole_BodyMedium));
}

void UIMd3Style::polish(QPalette &palette)
{
    QProxyStyle::polish(palette);
    if (!UIMd3Theme::instance())
        return;

    palette.setColor(QPalette::Window,          md3(UIMd3ColorRole_Surface));
    palette.setColor(QPalette::WindowText,      md3(UIMd3ColorRole_OnSurface));
    palette.setColor(QPalette::Base,            md3(UIMd3ColorRole_SurfaceContainerLow));
    palette.setColor(QPalette::AlternateBase,   md3(UIMd3ColorRole_SurfaceContainer));
    palette.setColor(QPalette::Text,            md3(UIMd3ColorRole_OnSurface));
    palette.setColor(QPalette::Button,          md3(UIMd3ColorRole_SurfaceContainerHigh));
    palette.setColor(QPalette::ButtonText,      md3(UIMd3ColorRole_OnSurface));
    palette.setColor(QPalette::BrightText,      md3(UIMd3ColorRole_OnError));
    palette.setColor(QPalette::Highlight,       md3(UIMd3ColorRole_SecondaryContainer));
    palette.setColor(QPalette::HighlightedText, md3(UIMd3ColorRole_OnSecondaryContainer));
    palette.setColor(QPalette::ToolTipBase,     md3(UIMd3ColorRole_SurfaceContainerHighest));
    palette.setColor(QPalette::ToolTipText,     md3(UIMd3ColorRole_OnSurface));
    palette.setColor(QPalette::Link,            md3(UIMd3ColorRole_Primary));
    palette.setColor(QPalette::LinkVisited,     md3(UIMd3ColorRole_Tertiary));
    palette.setColor(QPalette::PlaceholderText, md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurfaceVariant), false));

    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurface), false));
    palette.setColor(QPalette::Disabled, QPalette::Text,
                     md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurface), false));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurface), false));
    palette.setColor(QPalette::Disabled, QPalette::Highlight,
                     md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurface), false));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurface), false));
}

void UIMd3Style::polish(QWidget *pWidget)
{
    QProxyStyle::polish(pWidget);
    if (   qobject_cast<QAbstractButton*>(pWidget)
        || qobject_cast<QAbstractItemView*>(pWidget)
        || qobject_cast<QComboBox*>(pWidget)
        || qobject_cast<QScrollBar*>(pWidget)
        || qobject_cast<QSlider*>(pWidget)
        || qobject_cast<QTabBar*>(pWidget))
        pWidget->setAttribute(Qt::WA_Hover, true);
}

void UIMd3Style::unpolish(QWidget *pWidget)
{
    if (pWidget)
        pWidget->setAttribute(Qt::WA_Hover, false);
    QProxyStyle::unpolish(pWidget);
}

void UIMd3Style::drawPrimitive(PrimitiveElement enmElement, const QStyleOption *pOption,
                               QPainter *pPainter, const QWidget *pWidget) const
{
    if (!pOption || !pPainter || !UIMd3Theme::instance())
    {
        QProxyStyle::drawPrimitive(enmElement, pOption, pPainter, pWidget);
        return;
    }

    const bool fEnabled = md3StyleIsEnabled(pOption);
    const int iStateOpacity = md3StyleStateOpacity(pOption);

    if (enmElement == PE_PanelLineEdit || enmElement == PE_FrameLineEdit)
    {
        QColor fill = md3(UIMd3ColorRole_SurfaceContainerHighest);
        if (iStateOpacity)
            fill = md3StyleStateColor(UIMd3ColorRole_SurfaceContainerHighest,
                                      UIMd3ColorRole_OnSurface, iStateOpacity);
        const QColor outline = pOption->state.testFlag(State_HasFocus)
                             ? md3(UIMd3ColorRole_Primary)
                             : md3StyleEnabledColor(md3(UIMd3ColorRole_Outline), fEnabled);
        md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                            fill, outline, UIMd3Shape::Small);
        return;
    }
    if (enmElement == PE_PanelButtonCommand)
    {
        const QStyleOptionButton *pButton = qstyleoption_cast<const QStyleOptionButton*>(pOption);
        const bool fPrimary = pButton && pButton->features.testFlag(QStyleOptionButton::DefaultButton);
        const bool fFlat = pButton && pButton->features.testFlag(QStyleOptionButton::Flat);
        QColor fill = fPrimary ? md3(UIMd3ColorRole_Primary)
                               : md3(UIMd3ColorRole_SurfaceContainerHigh);
        if (fFlat && !iStateOpacity && !pOption->state.testFlag(State_On))
            fill = Qt::transparent;
        else if (iStateOpacity)
            fill = md3StyleStateColor(fPrimary ? UIMd3ColorRole_Primary
                                               : UIMd3ColorRole_SurfaceContainerHigh,
                                      fPrimary ? UIMd3ColorRole_OnPrimary
                                               : UIMd3ColorRole_OnSurface,
                                      iStateOpacity);
        const QColor outline = fFlat ? QColor()
                                     : md3StyleEnabledColor(md3(UIMd3ColorRole_OutlineVariant), fEnabled);
        md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                            md3StyleEnabledColor(fill, fEnabled), outline, UIMd3Shape::Full);
        if (pOption->state.testFlag(State_HasFocus))
            md3StyleDrawFocus(pPainter, pOption->rect, UIMd3Shape::Full);
        return;
    }
    if (enmElement == PE_PanelButtonTool)
    {
        QColor fill = Qt::transparent;
        if (pOption->state.testFlag(State_On))
            fill = md3(UIMd3ColorRole_SecondaryContainer);
        else if (iStateOpacity)
            fill = md3StyleStateColor(UIMd3ColorRole_SurfaceContainerHigh,
                                      UIMd3ColorRole_OnSurface, iStateOpacity);
        md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(1, 1, -1, -1),
                            md3StyleEnabledColor(fill, fEnabled), QColor(), UIMd3Shape::Full);
        if (pOption->state.testFlag(State_HasFocus))
            md3StyleDrawFocus(pPainter, pOption->rect, UIMd3Shape::Full);
        return;
    }
    if (enmElement == PE_IndicatorCheckBox)
    {
        const QRectF box = QRectF(pOption->rect).adjusted(1.5, 1.5, -1.5, -1.5);
        const bool fChecked = pOption->state.testFlag(State_On) || pOption->state.testFlag(State_NoChange);
        const QColor fill = fChecked ? md3(UIMd3ColorRole_Primary) : Qt::transparent;
        const QColor outline = fChecked ? md3(UIMd3ColorRole_Primary)
                                        : md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurfaceVariant), fEnabled);
        md3StyleDrawSurface(pPainter, box, md3StyleEnabledColor(fill, fEnabled), outline, UIMd3Shape::ExtraSmall);
        if (fChecked)
        {
            pPainter->save();
            pPainter->setRenderHint(QPainter::Antialiasing);
            pPainter->setPen(QPen(md3StyleEnabledColor(md3(UIMd3ColorRole_OnPrimary), fEnabled), 2,
                                  Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            if (pOption->state.testFlag(State_NoChange))
                pPainter->drawLine(box.left() + 4, box.center().y(), box.right() - 4, box.center().y());
            else
            {
                QPainterPath path;
                path.moveTo(box.left() + 4, box.center().y());
                path.lineTo(box.center().x() - 1, box.bottom() - 4);
                path.lineTo(box.right() - 3, box.top() + 4);
                pPainter->drawPath(path);
            }
            pPainter->restore();
        }
        return;
    }
    if (enmElement == PE_IndicatorRadioButton)
    {
        const QRectF outer = QRectF(pOption->rect).adjusted(1.5, 1.5, -1.5, -1.5);
        const bool fChecked = pOption->state.testFlag(State_On);
        pPainter->save();
        pPainter->setRenderHint(QPainter::Antialiasing);
        pPainter->setBrush(Qt::NoBrush);
        pPainter->setPen(QPen(md3StyleEnabledColor(fChecked ? md3(UIMd3ColorRole_Primary)
                                                            : md3(UIMd3ColorRole_OnSurfaceVariant), fEnabled), 2));
        pPainter->drawEllipse(outer);
        if (fChecked)
        {
            pPainter->setPen(Qt::NoPen);
            pPainter->setBrush(md3StyleEnabledColor(md3(UIMd3ColorRole_Primary), fEnabled));
            pPainter->drawEllipse(outer.adjusted(5, 5, -5, -5));
        }
        pPainter->restore();
        return;
    }
    if (enmElement == PE_PanelItemViewItem)
    {
        QColor fill = Qt::transparent;
        if (pOption->state.testFlag(State_Selected))
            fill = md3(UIMd3ColorRole_SecondaryContainer);
        else if (pOption->state.testFlag(State_MouseOver))
            fill = md3StyleStateColor(UIMd3ColorRole_SurfaceContainer,
                                      UIMd3ColorRole_OnSurface, UIMd3StateLayer::Hover);
        if (fill.alpha() > 0)
            md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(2, 2, -2, -2),
                                fill, QColor(), UIMd3Shape::Medium);
        return;
    }
    if (enmElement == PE_PanelMenu || enmElement == PE_PanelTipLabel)
    {
        md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                            md3(UIMd3ColorRole_SurfaceContainerHigh),
                            md3(UIMd3ColorRole_OutlineVariant), UIMd3Shape::Medium);
        return;
    }
    if (enmElement == PE_FrameMenu)
        return;
    if (enmElement == PE_FrameGroupBox)
    {
        md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                            md3(UIMd3ColorRole_SurfaceContainerLow),
                            md3StyleEnabledColor(md3(UIMd3ColorRole_OutlineVariant), fEnabled),
                            UIMd3Shape::Medium);
        return;
    }
    if (enmElement == PE_PanelToolBar || enmElement == PE_PanelStatusBar)
    {
        pPainter->fillRect(pOption->rect, md3(UIMd3ColorRole_SurfaceContainer));
        return;
    }
    if (enmElement == PE_IndicatorToolBarSeparator)
    {
        pPainter->save();
        pPainter->setPen(QPen(md3(UIMd3ColorRole_OutlineVariant), 1));
        if (pOption->rect.height() > pOption->rect.width())
            pPainter->drawLine(pOption->rect.center().x(), pOption->rect.top() + 8,
                               pOption->rect.center().x(), pOption->rect.bottom() - 8);
        else
            pPainter->drawLine(pOption->rect.left() + 8, pOption->rect.center().y(),
                               pOption->rect.right() - 8, pOption->rect.center().y());
        pPainter->restore();
        return;
    }
    if (enmElement == PE_FrameFocusRect)
    {
        md3StyleDrawFocus(pPainter, pOption->rect, UIMd3Shape::Small);
        return;
    }
    QProxyStyle::drawPrimitive(enmElement, pOption, pPainter, pWidget);
}

void UIMd3Style::drawControl(ControlElement enmElement, const QStyleOption *pOption,
                             QPainter *pPainter, const QWidget *pWidget) const
{
    if (!pOption || !pPainter || !UIMd3Theme::instance())
    {
        QProxyStyle::drawControl(enmElement, pOption, pPainter, pWidget);
        return;
    }

    if (enmElement == CE_PushButtonLabel)
    {
        const QStyleOptionButton *pButton = qstyleoption_cast<const QStyleOptionButton*>(pOption);
        if (pButton)
        {
            QStyleOptionButton option(*pButton);
            const bool fPrimary = pButton->features.testFlag(QStyleOptionButton::DefaultButton);
            option.palette.setColor(QPalette::ButtonText,
                                    md3StyleEnabledColor(md3(fPrimary ? UIMd3ColorRole_OnPrimary
                                                                     : UIMd3ColorRole_OnSurface),
                                                         md3StyleIsEnabled(pOption)));
            QProxyStyle::drawControl(enmElement, &option, pPainter, pWidget);
            return;
        }
    }
    if (enmElement == CE_ToolButtonLabel)
    {
        const QStyleOptionToolButton *pToolButton = qstyleoption_cast<const QStyleOptionToolButton*>(pOption);
        if (pToolButton)
        {
            QStyleOptionToolButton option(*pToolButton);
            option.palette.setColor(QPalette::ButtonText,
                                    md3StyleEnabledColor(md3(pOption->state.testFlag(State_On)
                                                            ? UIMd3ColorRole_OnSecondaryContainer
                                                            : UIMd3ColorRole_OnSurfaceVariant),
                                                         md3StyleIsEnabled(pOption)));
            QProxyStyle::drawControl(enmElement, &option, pPainter, pWidget);
            return;
        }
    }
    if (enmElement == CE_TabBarTabShape)
    {
        const QStyleOptionTab *pTab = qstyleoption_cast<const QStyleOptionTab*>(pOption);
        QColor fill = pOption->state.testFlag(State_Selected)
                    ? md3(UIMd3ColorRole_SecondaryContainer) : Qt::transparent;
        if (pOption->state.testFlag(State_MouseOver) && !pOption->state.testFlag(State_Selected))
            fill = md3StyleStateColor(UIMd3ColorRole_SurfaceContainer,
                                      UIMd3ColorRole_OnSurface, UIMd3StateLayer::Hover);
        md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(2, 4, -2, -4), fill,
                            QColor(), UIMd3Shape::Full);
        if (pTab && pOption->state.testFlag(State_HasFocus))
            md3StyleDrawFocus(pPainter, QRectF(pOption->rect).adjusted(1, 3, -1, -3), UIMd3Shape::Full);
        return;
    }
    if (enmElement == CE_TabBarTabLabel)
    {
        const QStyleOptionTab *pTab = qstyleoption_cast<const QStyleOptionTab*>(pOption);
        if (pTab)
        {
            QStyleOptionTab option(*pTab);
            option.palette.setColor(QPalette::WindowText,
                                    md3StyleEnabledColor(md3(pOption->state.testFlag(State_Selected)
                                                            ? UIMd3ColorRole_OnSecondaryContainer
                                                            : UIMd3ColorRole_OnSurfaceVariant),
                                                         md3StyleIsEnabled(pOption)));
            QProxyStyle::drawControl(enmElement, &option, pPainter, pWidget);
            return;
        }
    }
    if (enmElement == CE_MenuItem)
    {
        const QStyleOptionMenuItem *pMenuItem = qstyleoption_cast<const QStyleOptionMenuItem*>(pOption);
        if (pMenuItem)
        {
            QStyleOptionMenuItem option(*pMenuItem);
            if (pOption->state.testFlag(State_Selected))
                md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(4, 2, -4, -2),
                                    md3(UIMd3ColorRole_SecondaryContainer), QColor(), UIMd3Shape::Small);
            option.palette.setColor(QPalette::Highlight, Qt::transparent);
            option.palette.setColor(QPalette::HighlightedText,
                                    md3(UIMd3ColorRole_OnSecondaryContainer));
            option.palette.setColor(QPalette::Text,
                                    md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurface),
                                                         md3StyleIsEnabled(pOption)));
            QProxyStyle::drawControl(enmElement, &option, pPainter, pWidget);
            return;
        }
    }
    if (enmElement == CE_HeaderSection)
    {
        pPainter->fillRect(pOption->rect, md3(UIMd3ColorRole_SurfaceContainerHigh));
        pPainter->save();
        pPainter->setPen(QPen(md3(UIMd3ColorRole_OutlineVariant), 1));
        pPainter->drawLine(pOption->rect.bottomLeft(), pOption->rect.bottomRight());
        pPainter->restore();
        return;
    }
    if (enmElement == CE_HeaderLabel)
    {
        const QStyleOptionHeader *pHeader = qstyleoption_cast<const QStyleOptionHeader*>(pOption);
        if (pHeader)
        {
            QStyleOptionHeader option(*pHeader);
            option.palette.setColor(QPalette::ButtonText,
                                    md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurfaceVariant),
                                                         md3StyleIsEnabled(pOption)));
            QProxyStyle::drawControl(enmElement, &option, pPainter, pWidget);
            return;
        }
    }
    if (enmElement == CE_ProgressBarGroove)
    {
        const QProgressBar *pProgressBar = qobject_cast<const QProgressBar*>(pWidget);
        const bool fHorizontal = !pProgressBar || pProgressBar->orientation() == Qt::Horizontal;
        const QRectF groove = fHorizontal
                            ? QRectF(pOption->rect.left() + 1, pOption->rect.center().y() - 3,
                                     qMax(0, pOption->rect.width() - 2), 6)
                            : QRectF(pOption->rect.center().x() - 3, pOption->rect.top() + 1,
                                     6, qMax(0, pOption->rect.height() - 2));
        md3StyleDrawSurface(pPainter, groove,
                            md3(UIMd3ColorRole_SurfaceContainerHighest), QColor(), UIMd3Shape::Full);
        return;
    }
    if (enmElement == CE_ProgressBarContents)
    {
        const QStyleOptionProgressBar *pProgress = qstyleoption_cast<const QStyleOptionProgressBar*>(pOption);
        if (pProgress && pProgress->maximum > pProgress->minimum)
        {
            const QProgressBar *pProgressBar = qobject_cast<const QProgressBar*>(pWidget);
            const bool fHorizontal = !pProgressBar || pProgressBar->orientation() == Qt::Horizontal;
            const qreal dRatio = qBound<qreal>(0, qreal(pProgress->progress - pProgress->minimum)
                                                / qreal(pProgress->maximum - pProgress->minimum), 1);
            QRectF fillRect = fHorizontal
                            ? QRectF(pOption->rect.left() + 1, pOption->rect.center().y() - 3,
                                     qMax(0, pOption->rect.width() - 2), 6)
                            : QRectF(pOption->rect.center().x() - 3, pOption->rect.top() + 1,
                                     6, qMax(0, pOption->rect.height() - 2));
            if (fHorizontal)
            {
                const qreal dWidth = fillRect.width() * dRatio;
                const bool fReverse = pProgress->invertedAppearance != (pOption->direction == Qt::RightToLeft);
                if (fReverse)
                    fillRect.setLeft(fillRect.right() - dWidth);
                else
                    fillRect.setWidth(dWidth);
            }
            else
            {
                const qreal dHeight = fillRect.height() * dRatio;
                if (pProgress->invertedAppearance != pProgress->bottomToTop)
                    fillRect.setHeight(dHeight);
                else
                    fillRect.setTop(fillRect.bottom() - dHeight);
            }
            if (fillRect.width() > 0 && fillRect.height() > 0)
                md3StyleDrawSurface(pPainter, fillRect, md3(UIMd3ColorRole_Primary),
                                    QColor(), UIMd3Shape::Full);
        }
        return;
    }
    QProxyStyle::drawControl(enmElement, pOption, pPainter, pWidget);
}

void UIMd3Style::drawComplexControl(ComplexControl enmControl, const QStyleOptionComplex *pOption,
                                    QPainter *pPainter, const QWidget *pWidget) const
{
    if (!pOption || !pPainter || !UIMd3Theme::instance())
    {
        QProxyStyle::drawComplexControl(enmControl, pOption, pPainter, pWidget);
        return;
    }

    if (enmControl == CC_ComboBox)
    {
        const QStyleOptionComboBox *pCombo = qstyleoption_cast<const QStyleOptionComboBox*>(pOption);
        if (pCombo)
        {
            const bool fEnabled = md3StyleIsEnabled(pOption);
            QColor fill = md3(UIMd3ColorRole_SurfaceContainerHighest);
            const int iStateOpacity = md3StyleStateOpacity(pOption);
            if (iStateOpacity)
                fill = md3StyleStateColor(UIMd3ColorRole_SurfaceContainerHighest,
                                          UIMd3ColorRole_OnSurface, iStateOpacity);
            md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                fill,
                                pOption->state.testFlag(State_HasFocus)
                                ? md3(UIMd3ColorRole_Primary)
                                : md3StyleEnabledColor(md3(UIMd3ColorRole_Outline), fEnabled),
                                UIMd3Shape::Small);

            const QRect arrowRect = subControlRect(CC_ComboBox, pCombo, SC_ComboBoxArrow, pWidget);
            pPainter->save();
            pPainter->setRenderHint(QPainter::Antialiasing);
            pPainter->setPen(Qt::NoPen);
            pPainter->setBrush(md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurfaceVariant), fEnabled));
            QPainterPath arrow;
            arrow.moveTo(arrowRect.center().x() - 4, arrowRect.center().y() - 2);
            arrow.lineTo(arrowRect.center().x() + 4, arrowRect.center().y() - 2);
            arrow.lineTo(arrowRect.center().x(), arrowRect.center().y() + 3);
            arrow.closeSubpath();
            pPainter->drawPath(arrow);
            pPainter->restore();

            const QRect textRect = subControlRect(CC_ComboBox, pCombo, SC_ComboBoxEditField, pWidget);
            pPainter->save();
            pPainter->setPen(md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurface), fEnabled));
            const QString strText = QFontMetrics(pCombo->fontMetrics).elidedText(pCombo->currentText,
                                                                                Qt::ElideRight,
                                                                                textRect.width());
            pPainter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, strText);
            pPainter->restore();
            return;
        }
    }
    if (enmControl == CC_Slider)
    {
        const QStyleOptionSlider *pSlider = qstyleoption_cast<const QStyleOptionSlider*>(pOption);
        if (pSlider)
        {
            const bool fHorizontal = pSlider->orientation == Qt::Horizontal;
            const bool fEnabled = md3StyleIsEnabled(pOption);
            QRect groove = subControlRect(CC_Slider, pSlider, SC_SliderGroove, pWidget);
            QRect handle = subControlRect(CC_Slider, pSlider, SC_SliderHandle, pWidget);
            QRectF inactive;
            QRectF active;
            if (fHorizontal)
            {
                inactive = QRectF(groove.left(), groove.center().y() - 2, groove.width(), 4);
                if (pSlider->upsideDown)
                    active = QRectF(handle.center().x(), inactive.top(),
                                    qMax<qreal>(0, inactive.right() - handle.center().x()), inactive.height());
                else
                    active = QRectF(inactive.left(), inactive.top(),
                                    qMax<qreal>(0, handle.center().x() - inactive.left()), inactive.height());
            }
            else
            {
                inactive = QRectF(groove.center().x() - 2, groove.top(), 4, groove.height());
                if (pSlider->upsideDown)
                    active = QRectF(inactive.left(), inactive.top(), inactive.width(),
                                    qMax<qreal>(0, handle.center().y() - inactive.top()));
                else
                    active = QRectF(inactive.left(), handle.center().y(), inactive.width(),
                                    qMax<qreal>(0, inactive.bottom() - handle.center().y()));
            }
            md3StyleDrawSurface(pPainter, inactive,
                                md3StyleEnabledColor(md3(UIMd3ColorRole_SurfaceContainerHighest), fEnabled),
                                QColor(), UIMd3Shape::Full);
            md3StyleDrawSurface(pPainter, active,
                                md3StyleEnabledColor(md3(UIMd3ColorRole_Primary), fEnabled),
                                QColor(), UIMd3Shape::Full);
            pPainter->save();
            pPainter->setRenderHint(QPainter::Antialiasing);
            pPainter->setPen(Qt::NoPen);
            pPainter->setBrush(md3StyleEnabledColor(md3(UIMd3ColorRole_Primary), fEnabled));
            pPainter->drawEllipse(QRectF(handle).adjusted(2, 2, -2, -2));
            pPainter->restore();
            if (pOption->state.testFlag(State_HasFocus))
                md3StyleDrawFocus(pPainter, QRectF(handle).adjusted(-2, -2, 2, 2), UIMd3Shape::Full);
            return;
        }
    }
    if (enmControl == CC_SpinBox)
    {
        const QStyleOptionSpinBox *pSpinBox = qstyleoption_cast<const QStyleOptionSpinBox*>(pOption);
        if (pSpinBox)
        {
            const bool fEnabled = md3StyleIsEnabled(pOption);
            QColor fill = md3(UIMd3ColorRole_SurfaceContainerHighest);
            const int iStateOpacity = md3StyleStateOpacity(pOption);
            if (iStateOpacity)
                fill = md3StyleStateColor(UIMd3ColorRole_SurfaceContainerHighest,
                                          UIMd3ColorRole_OnSurface, iStateOpacity);
            md3StyleDrawSurface(pPainter, QRectF(pOption->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                fill,
                                pOption->state.testFlag(State_HasFocus)
                                ? md3(UIMd3ColorRole_Primary)
                                : md3StyleEnabledColor(md3(UIMd3ColorRole_Outline), fEnabled),
                                UIMd3Shape::Small);
            if (pSpinBox->buttonSymbols != QAbstractSpinBox::NoButtons)
            {
                const QRect upRect = subControlRect(CC_SpinBox, pSpinBox, SC_SpinBoxUp, pWidget);
                const QRect downRect = subControlRect(CC_SpinBox, pSpinBox, SC_SpinBoxDown, pWidget);
                pPainter->save();
                pPainter->setPen(QPen(md3(UIMd3ColorRole_OutlineVariant), 1));
                pPainter->drawLine(upRect.topLeft(), upRect.bottomLeft());
                pPainter->drawLine(upRect.bottomLeft(), upRect.bottomRight());
                pPainter->restore();
                const QColor glyph = md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurfaceVariant), fEnabled);
                if (pSpinBox->buttonSymbols == QAbstractSpinBox::PlusMinus)
                {
                    pPainter->save();
                    pPainter->setPen(QPen(glyph, 2, Qt::SolidLine, Qt::RoundCap));
                    pPainter->drawLine(upRect.center().x() - 4, upRect.center().y(),
                                       upRect.center().x() + 4, upRect.center().y());
                    pPainter->drawLine(upRect.center().x(), upRect.center().y() - 4,
                                       upRect.center().x(), upRect.center().y() + 4);
                    pPainter->drawLine(downRect.center().x() - 4, downRect.center().y(),
                                       downRect.center().x() + 4, downRect.center().y());
                    pPainter->restore();
                }
                else
                {
                    md3StyleDrawChevron(pPainter, upRect, Qt::UpArrow, glyph);
                    md3StyleDrawChevron(pPainter, downRect, Qt::DownArrow, glyph);
                }
            }
            return;
        }
    }
    if (enmControl == CC_ScrollBar)
    {
        const QStyleOptionSlider *pScrollBar = qstyleoption_cast<const QStyleOptionSlider*>(pOption);
        if (pScrollBar)
        {
            const bool fEnabled = md3StyleIsEnabled(pOption);
            const QRect groove = subControlRect(CC_ScrollBar, pScrollBar, SC_ScrollBarGroove, pWidget);
            const QRect slider = subControlRect(CC_ScrollBar, pScrollBar, SC_ScrollBarSlider, pWidget);
            const QRect subLine = subControlRect(CC_ScrollBar, pScrollBar, SC_ScrollBarSubLine, pWidget);
            const QRect addLine = subControlRect(CC_ScrollBar, pScrollBar, SC_ScrollBarAddLine, pWidget);
            pPainter->fillRect(pOption->rect, md3(UIMd3ColorRole_Surface));
            md3StyleDrawSurface(pPainter, QRectF(groove).adjusted(4, 4, -4, -4),
                                md3(UIMd3ColorRole_SurfaceContainer), QColor(), UIMd3Shape::Full);
            QColor handle = md3(UIMd3ColorRole_OnSurfaceVariant);
            handle.setAlphaF(fEnabled ? 0.62 : UIMd3StateLayer::Disabled / 100.0);
            if (pOption->activeSubControls.testFlag(SC_ScrollBarSlider))
                handle = md3(UIMd3ColorRole_Primary);
            md3StyleDrawSurface(pPainter, QRectF(slider).adjusted(3, 3, -3, -3),
                                handle, QColor(), UIMd3Shape::Full);

            const QColor glyph = md3StyleEnabledColor(md3(UIMd3ColorRole_OnSurfaceVariant), fEnabled);
            if (pScrollBar->orientation == Qt::Horizontal)
            {
                md3StyleDrawChevron(pPainter, subLine,
                                    pScrollBar->upsideDown ? Qt::RightArrow : Qt::LeftArrow, glyph);
                md3StyleDrawChevron(pPainter, addLine,
                                    pScrollBar->upsideDown ? Qt::LeftArrow : Qt::RightArrow, glyph);
            }
            else
            {
                md3StyleDrawChevron(pPainter, subLine,
                                    pScrollBar->upsideDown ? Qt::DownArrow : Qt::UpArrow, glyph);
                md3StyleDrawChevron(pPainter, addLine,
                                    pScrollBar->upsideDown ? Qt::UpArrow : Qt::DownArrow, glyph);
            }
            if (pOption->state.testFlag(State_HasFocus))
                md3StyleDrawFocus(pPainter, pOption->rect, UIMd3Shape::Small);
            return;
        }
    }
    QProxyStyle::drawComplexControl(enmControl, pOption, pPainter, pWidget);
}

QSize UIMd3Style::sizeFromContents(ContentsType enmContentsType, const QStyleOption *pOption,
                                   const QSize &contentSize, const QWidget *pWidget) const
{
    QSize result = QProxyStyle::sizeFromContents(enmContentsType, pOption, contentSize, pWidget);
    const int iControlHeight = UIMd3Theme::instance() ? qMax(48, md3Theme().controlHeight()) : 48;
    switch (enmContentsType)
    {
        case CT_PushButton:
            result.setHeight(qMax(result.height(), iControlHeight));
            result.setWidth(qMax(result.width() + 16, 64));
            break;
        case CT_ToolButton:
            result.setHeight(qMax(result.height(), 48));
            result.setWidth(qMax(result.width(), 48));
            break;
        case CT_ComboBox:
        case CT_LineEdit:
        case CT_SpinBox:
            result.setHeight(qMax(result.height(), iControlHeight));
            break;
        case CT_CheckBox:
        case CT_RadioButton:
            result.setHeight(qMax(result.height(), 48));
            break;
        case CT_MenuItem:
            result.setHeight(qMax(result.height(), 48));
            result.rwidth() += 16;
            break;
        case CT_TabBarTab:
            result.setHeight(qMax(result.height(), 48));
            result.rwidth() += 16;
            break;
        case CT_ItemViewItem:
            result.setHeight(qMax(result.height(), 48));
            break;
        case CT_HeaderSection:
            result.setHeight(qMax(result.height(), 48));
            break;
        case CT_ProgressBar:
            result.setHeight(qMax(result.height(), 16));
            break;
        case CT_Slider:
        {
            const QSlider *pSlider = qobject_cast<const QSlider*>(pWidget);
            if (pSlider && pSlider->orientation() == Qt::Vertical)
                result.setWidth(qMax(result.width(), 48));
            else
                result.setHeight(qMax(result.height(), 48));
            break;
        }
        default:
            break;
    }
    return result;
}

int UIMd3Style::pixelMetric(PixelMetric enmMetric, const QStyleOption *pOption,
                            const QWidget *pWidget) const
{
    switch (enmMetric)
    {
        case PM_DefaultFrameWidth:       return 1;
        case PM_ButtonMargin:            return UIMd3Theme::instance() ? md3Theme().gutter() : 16;
        case PM_ButtonIconSize:          return 20;
        case PM_SmallIconSize:           return 20;
        case PM_ToolBarIconSize:         return 24;
        case PM_IndicatorWidth:
        case PM_IndicatorHeight:
        case PM_ExclusiveIndicatorWidth:
        case PM_ExclusiveIndicatorHeight:return 20;
        case PM_ScrollBarExtent:         return 16;
        case PM_ScrollBarSliderMin:      return 40;
        case PM_SliderThickness:         return 24;
        case PM_SliderLength:            return 24;
        case PM_TabBarTabHSpace:         return 24;
        case PM_TabBarTabVSpace:         return 12;
        case PM_MenuHMargin:
        case PM_MenuVMargin:             return 4;
        case PM_MenuPanelWidth:          return 1;
        case PM_MenuButtonIndicator:     return 20;
        case PM_LayoutLeftMargin:
        case PM_LayoutTopMargin:
        case PM_LayoutRightMargin:
        case PM_LayoutBottomMargin:      return UIMd3Theme::instance() ? md3Theme().gutter() : 16;
        case PM_LayoutHorizontalSpacing:
        case PM_LayoutVerticalSpacing:   return UIMd3Theme::instance() && md3Theme().isCompact() ? 8 : 12;
        default:                         break;
    }
    return QProxyStyle::pixelMetric(enmMetric, pOption, pWidget);
}

int UIMd3Style::styleHint(StyleHint enmHint, const QStyleOption *pOption, const QWidget *pWidget,
                          QStyleHintReturn *pReturnData) const
{
    switch (enmHint)
    {
        case SH_UnderlineShortcut:              return 0;
        case SH_ItemView_ShowDecorationSelected:return 1;
        case SH_Menu_Scrollable:                return 1;
        case SH_ScrollBar_Transient:             return 1;
        case SH_DialogButtonBox_ButtonsHaveIcons:return 0;
        default:                                break;
    }
    return QProxyStyle::styleHint(enmHint, pOption, pWidget, pReturnData);
}
