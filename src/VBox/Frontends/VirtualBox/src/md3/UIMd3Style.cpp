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

#include <QApplication>
#include <QPainter>
#include <QStyleOption>

#include "UIMd3Style.h"
#include "UIMd3Theme.h"

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
}

void UIMd3Style::drawPrimitive(PrimitiveElement enmElement, const QStyleOption *pOption,
                               QPainter *pPainter, const QWidget *pWidget) const
{
    if (pOption && (enmElement == PE_PanelLineEdit || enmElement == PE_FrameLineEdit))
    {
        pPainter->save();
        pPainter->setRenderHint(QPainter::Antialiasing);
        pPainter->setPen(QPen(md3(UIMd3ColorRole_Outline), 1));
        pPainter->setBrush(md3(UIMd3ColorRole_SurfaceContainerHighest));
        pPainter->drawRoundedRect(pOption->rect.adjusted(0, 0, -1, -1), UIMd3Shape::Small, UIMd3Shape::Small);
        pPainter->restore();
        return;
    }
    QProxyStyle::drawPrimitive(enmElement, pOption, pPainter, pWidget);
}

void UIMd3Style::drawControl(ControlElement enmElement, const QStyleOption *pOption,
                             QPainter *pPainter, const QWidget *pWidget) const
{
    QProxyStyle::drawControl(enmElement, pOption, pPainter, pWidget);
}

int UIMd3Style::pixelMetric(PixelMetric enmMetric, const QStyleOption *pOption,
                            const QWidget *pWidget) const
{
    if (enmMetric == PM_DefaultFrameWidth)
        return 1;
    if (enmMetric == PM_ButtonMargin)
        return md3Theme().gutter();
    return QProxyStyle::pixelMetric(enmMetric, pOption, pWidget);
}

int UIMd3Style::styleHint(StyleHint enmHint, const QStyleOption *pOption, const QWidget *pWidget,
                          QStyleHintReturn *pReturnData) const
{
    if (enmHint == SH_UnderlineShortcut)
        return 0;
    return QProxyStyle::styleHint(enmHint, pOption, pWidget, pReturnData);
}
