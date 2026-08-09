/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Style class declaration - QProxyStyle bridging not-yet-migrated widgets.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Style_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Style_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QProxyStyle>

/* IPRT compatibility macros: */
#include <iprt/cdefs.h>

/* GUI includes: */
#include "UILibraryDefs.h"

/** QProxyStyle extension that paints stock Qt controls in Material 3.
  *
  * Existing VirtualBox pages still contain many stock Qt controls.  This
  * shared style gives those application-owned controls the same semantic
  * colors, shape, state, focus and sizing rules as the UIMd3 widgets without
  * replacing their models, signals or validation behavior. */
class SHARED_LIBRARY_STUFF UIMd3Style : public QProxyStyle
{
    Q_OBJECT;

public:

    using QProxyStyle::unpolish;

    /** Installs the style on the application. */
    static void install();

    /** Constructs the style. */
    UIMd3Style();

    /** Applies Material typography and palette defaults to @a pApplication. */
    virtual void polish(QApplication *pApplication) RT_OVERRIDE;
    /** Applies Material semantic colors to @a palette. */
    virtual void polish(QPalette &palette) RT_OVERRIDE;
    /** Enables state-layer tracking for supported stock @a pWidget controls. */
    virtual void polish(QWidget *pWidget) RT_OVERRIDE;
    /** Restores stock-widget attributes changed by polish(). */
    virtual void unpolish(QWidget *pWidget) RT_OVERRIDE;

    /** Draws the primitive @a element. */
    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *pOption,
                               QPainter *pPainter, const QWidget *pWidget = 0) const RT_OVERRIDE;
    /** Draws the control @a element. */
    virtual void drawControl(ControlElement element, const QStyleOption *pOption,
                             QPainter *pPainter, const QWidget *pWidget = 0) const RT_OVERRIDE;
    /** Draws Material combo-box and slider complex controls. */
    virtual void drawComplexControl(ComplexControl control, const QStyleOptionComplex *pOption,
                                    QPainter *pPainter, const QWidget *pWidget = 0) const RT_OVERRIDE;
    /** Returns Material minimum anatomy for stock control @a contentsType. */
    virtual QSize sizeFromContents(ContentsType contentsType, const QStyleOption *pOption,
                                   const QSize &contentSize,
                                   const QWidget *pWidget = 0) const RT_OVERRIDE;
    /** Returns the pixel metric for @a metric. */
    virtual int pixelMetric(PixelMetric metric, const QStyleOption *pOption = 0,
                            const QWidget *pWidget = 0) const RT_OVERRIDE;
    /** Returns the style hint for @a hint. */
    virtual int styleHint(StyleHint hint, const QStyleOption *pOption = 0, const QWidget *pWidget = 0,
                          QStyleHintReturn *pReturnData = 0) const RT_OVERRIDE;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Style_h */
