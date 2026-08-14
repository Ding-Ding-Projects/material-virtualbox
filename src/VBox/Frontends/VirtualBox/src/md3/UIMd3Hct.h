/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 HCT colour space declaration - CAM16 hue, chroma and L* tone.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Hct_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Hct_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UILibraryDefs.h"

/** Material 3 HCT colour coordinate.
  *
  * HCT is the colour space Material 3 generates every tonal palette from. Hue and
  * chroma come from the CAM16 colour-appearance model under the Material default
  * viewing conditions, while tone is CIE L*, so a tone difference is a predictable
  * contrast difference regardless of hue. That property is why the theme cannot use
  * an HSL stand-in: HSL lightness is not perceptually uniform, so identical tone
  * numbers would produce different contrast for different seeds.
  *
  * This is an independent implementation of the published CAM16 model and of the
  * Material 3 palette rules; it deliberately carries no third-party code so the
  * frontend keeps a single licence. */
struct UIMd3Hct
{
    /** Holds the CAM16 hue in degrees, 0..360. */
    double dHue;
    /** Holds the CAM16 chroma; 0 for a fully neutral colour, unbounded above. */
    double dChroma;
    /** Holds the CIE L* tone, 0..100. */
    double dTone;
};

/** Returns the HCT coordinate of the opaque sRGB colour @a iRed, @a iGreen, @a iBlue (0..255 each). */
SHARED_LIBRARY_STUFF UIMd3Hct md3HctFromRgb(int iRed, int iGreen, int iBlue);

/** Resolves the HCT coordinate @a dHue, @a dChroma, @a dTone to the closest displayable
  * sRGB colour and stores it in @a piRed, @a piGreen and @a piBlue (0..255 each).
  *
  * Tone is honoured exactly wherever sRGB can represent it, because contrast depends on
  * it; chroma is reduced towards the neutral axis when the requested combination falls
  * outside the sRGB gamut, exactly as the Material tonal-palette rules require. */
SHARED_LIBRARY_STUFF void md3HctToRgb(double dHue, double dChroma, double dTone,
                                      int *piRed, int *piGreen, int *piBlue);

/** Returns the CIE L* tone of the opaque sRGB colour @a iRed, @a iGreen, @a iBlue. */
SHARED_LIBRARY_STUFF double md3LstarFromRgb(int iRed, int iGreen, int iBlue);

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Hct_h */
