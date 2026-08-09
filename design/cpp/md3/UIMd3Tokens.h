/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Tokens declarations - Material 3 design tokens.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Tokens_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Tokens_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QColor>
#include <QFont>
#include <QMetaType>
#include <QString>

/** Material 3 tonal role. Every colour the GUI paints resolves through one of these. */
enum UIMd3ColorRole
{
    UIMd3ColorRole_Primary,
    UIMd3ColorRole_OnPrimary,
    UIMd3ColorRole_PrimaryContainer,
    UIMd3ColorRole_OnPrimaryContainer,
    UIMd3ColorRole_Secondary,
    UIMd3ColorRole_OnSecondary,
    UIMd3ColorRole_SecondaryContainer,
    UIMd3ColorRole_OnSecondaryContainer,
    UIMd3ColorRole_Tertiary,
    UIMd3ColorRole_OnTertiary,
    UIMd3ColorRole_TertiaryContainer,
    UIMd3ColorRole_OnTertiaryContainer,
    UIMd3ColorRole_Error,
    UIMd3ColorRole_OnError,
    UIMd3ColorRole_ErrorContainer,
    UIMd3ColorRole_OnErrorContainer,
    UIMd3ColorRole_Surface,
    UIMd3ColorRole_OnSurface,
    UIMd3ColorRole_OnSurfaceVariant,
    UIMd3ColorRole_SurfaceContainerLowest,
    UIMd3ColorRole_SurfaceContainerLow,
    UIMd3ColorRole_SurfaceContainer,
    UIMd3ColorRole_SurfaceContainerHigh,
    UIMd3ColorRole_SurfaceContainerHighest,
    UIMd3ColorRole_Outline,
    UIMd3ColorRole_OutlineVariant,
    UIMd3ColorRole_Scrim,
    UIMd3ColorRole_Max
};

/** Material 3 type role. */
enum UIMd3TypeRole
{
    UIMd3TypeRole_DisplayLarge,  UIMd3TypeRole_DisplayMedium, UIMd3TypeRole_DisplaySmall,
    UIMd3TypeRole_HeadlineLarge, UIMd3TypeRole_HeadlineMedium, UIMd3TypeRole_HeadlineSmall,
    UIMd3TypeRole_TitleLarge,    UIMd3TypeRole_TitleMedium,   UIMd3TypeRole_TitleSmall,
    UIMd3TypeRole_BodyLarge,     UIMd3TypeRole_BodyMedium,    UIMd3TypeRole_BodySmall,
    UIMd3TypeRole_LabelLarge,    UIMd3TypeRole_LabelMedium,   UIMd3TypeRole_LabelSmall,
    UIMd3TypeRole_Max
};

/** Material 3 shape scale, in device independent pixels. */
namespace UIMd3Shape
{
    enum { None = 0, ExtraSmall = 4, Small = 8, Medium = 12, Large = 16, ExtraLarge = 28, Full = 999 };
}

/** Material 3 elevation levels, expressed as shadow blur radii. */
namespace UIMd3Elevation
{
    enum { Level0 = 0, Level1 = 3, Level2 = 6, Level3 = 8, Level4 = 10, Level5 = 12 };
}

/** Material 3 state layer opacities, in percent. */
namespace UIMd3StateLayer
{
    enum { Hover = 8, Focus = 10, Pressed = 10, Dragged = 16, Disabled = 38 };
}

/** Material 3 motion durations, in milliseconds. */
namespace UIMd3Motion
{
    enum { Short2 = 100, Short4 = 200, Medium2 = 300, Medium4 = 400, Long2 = 500 };
}

/** Colour scheme variants supported by the shell. */
enum UIMd3Scheme
{
    UIMd3Scheme_Dark,
    UIMd3Scheme_Light,
    UIMd3Scheme_System,
    UIMd3Scheme_HighContrastDark,
    UIMd3Scheme_HighContrastLight
};

/** Per-element appearance override, persisted in extradata. */
struct UIMd3Appearance
{
    UIMd3Appearance()
        : fValid(false), iRadius(UIMd3Shape::Large), dScale(1.0), iWeight(QFont::Normal) {}

    bool     fValid;      /**< Whether any field of this override is in use. */
    QColor   seed;        /**< Optional per-element seed colour. */
    QString  strFont;     /**< Optional font family. */
    int      iRadius;     /**< Corner radius in pixels. */
    double   dScale;      /**< Font scale factor. */
    int      iWeight;     /**< QFont weight. */
};
Q_DECLARE_METATYPE(UIMd3Appearance);

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Tokens_h */
