/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Style class declaration - QProxyStyle bridging not-yet-migrated widgets.
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
  * Every screen in this rewrite is built from UIMd3* widgets, but third-party
  * and platform dialogs (file chooser, message boxes, printer dialogs) still
  * come from Qt. This style keeps those consistent instead of leaving a
  * platform-styled hole in the middle of a Material 3 window. */
class SHARED_LIBRARY_STUFF UIMd3Style : public QProxyStyle
{
    Q_OBJECT;

public:

    /** Installs the style on the application. */
    static void install();

    /** Constructs the style. */
    UIMd3Style();

    /** Draws the primitive @a element. */
    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *pOption,
                               QPainter *pPainter, const QWidget *pWidget = 0) const override;
    /** Draws the control @a element. */
    virtual void drawControl(ControlElement element, const QStyleOption *pOption,
                             QPainter *pPainter, const QWidget *pWidget = 0) const override;
    /** Returns the pixel metric for @a metric. */
    virtual int pixelMetric(PixelMetric metric, const QStyleOption *pOption = 0,
                            const QWidget *pWidget = 0) const override;
    /** Returns the style hint for @a hint. */
    virtual int styleHint(StyleHint hint, const QStyleOption *pOption = 0, const QWidget *pWidget = 0,
                          QStyleHintReturn *pReturnData = 0) const override;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Style_h */
