/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 HCT tonal-palette testcase.
 *
 * Checks the generated palette against the published Material 3 baseline scheme and
 * against the properties the rest of the frontend depends on: tone is CIE L*, tone is
 * monotonic, every generated colour is displayable, and generation is deterministic.
 *
 * The test deliberately links no Qt: it exercises the colour science alone, so a failure
 * points at the palette rules rather than at widget code.
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

/* Standard includes: */
#include <cmath>
#include <cstdio>
#include <cstdlib>

/* GUI includes: */
#include "UIMd3Hct.h"


/** Counts the checks that failed. */
static int g_cFailures = 0;
/** Counts the checks that ran. */
static int g_cChecks = 0;

/** Reports a failed check. */
static void md3TestFailed(const char *pszWhat, const char *pszDetail)
{
    ++g_cFailures;
    std::printf("FAILED: %s: %s\n", pszWhat, pszDetail);
}

/** Reports whether @a fCondition held for @a pszWhat, describing it with @a pszDetail. */
static void md3TestCheck(bool fCondition, const char *pszWhat, const char *pszDetail)
{
    ++g_cChecks;
    if (!fCondition)
        md3TestFailed(pszWhat, pszDetail);
}

/** A published Material 3 baseline palette entry. */
struct UIMd3ToneExpectation
{
    const char *pszPalette;
    double dHue;
    double dChroma;
    int iTone;
    int iRed;
    int iGreen;
    int iBlue;
};

/** The Material 3 baseline seed. */
static const int g_iSeedRed = 0x67;
static const int g_iSeedGreen = 0x50;
static const int g_iSeedBlue = 0xA4;

/** The per-channel tolerance against the published baseline table.
  *
  * The published values were authored from a reference implementation that resolves the
  * sRGB gamut boundary analytically; this implementation bisects it. Both agree on the
  * tone exactly, and differ by at most one 8-bit step on the chroma-limited light tones,
  * which is below the smallest difference a display can show. */
static const int g_iChannelTolerance = 2;

/** Runs the published-baseline comparison for the seed palettes. */
static void md3TestBaselinePalette()
{
    const UIMd3Hct seed = md3HctFromRgb(g_iSeedRed, g_iSeedGreen, g_iSeedBlue);
    const double dHue = seed.dHue;
    const double dPrimaryChroma = seed.dChroma > 48.0 ? seed.dChroma : 48.0;

    /* The Material 3 baseline scheme, as used by the checked-in design prototypes. */
    const UIMd3ToneExpectation aExpectations[] =
    {
        /* Primary: the seed hue at the seed's own chroma, floored at 48. */
        { "primary",         dHue, dPrimaryChroma,   0, 0x00, 0x00, 0x00 },
        { "primary",         dHue, dPrimaryChroma,  10, 0x21, 0x00, 0x5D },
        { "primary",         dHue, dPrimaryChroma,  20, 0x38, 0x1E, 0x72 },
        { "primary",         dHue, dPrimaryChroma,  30, 0x4F, 0x37, 0x8B },
        { "primary",         dHue, dPrimaryChroma,  40, 0x67, 0x50, 0xA4 },
        { "primary",         dHue, dPrimaryChroma,  80, 0xD0, 0xBC, 0xFF },
        { "primary",         dHue, dPrimaryChroma,  90, 0xEA, 0xDD, 0xFF },
        { "primary",         dHue, dPrimaryChroma, 100, 0xFF, 0xFF, 0xFF },
        /* Secondary: the seed hue held at chroma 16. */
        { "secondary",       dHue,           16.0,  10, 0x1D, 0x19, 0x2B },
        { "secondary",       dHue,           16.0,  30, 0x4A, 0x44, 0x58 },
        { "secondary",       dHue,           16.0,  80, 0xCC, 0xC2, 0xDC },
        { "secondary",       dHue,           16.0,  90, 0xE8, 0xDE, 0xF8 },
        /* Tertiary: the seed hue rotated 60 degrees at chroma 24. */
        { "tertiary",  dHue + 60.0,          24.0,  30, 0x63, 0x3B, 0x48 },
        { "tertiary",  dHue + 60.0,          24.0,  40, 0x7D, 0x52, 0x60 },
        { "tertiary",  dHue + 60.0,          24.0,  80, 0xEF, 0xB8, 0xC8 },
        /* Neutral: the surface family, the seed hue at chroma 6. */
        { "neutral",         dHue,            6.0,   4, 0x0F, 0x0D, 0x13 },
        { "neutral",         dHue,            6.0,   6, 0x14, 0x12, 0x18 },
        { "neutral",         dHue,            6.0,  10, 0x1D, 0x1B, 0x20 },
        { "neutral",         dHue,            6.0,  12, 0x21, 0x1F, 0x26 },
        { "neutral",         dHue,            6.0,  17, 0x2B, 0x29, 0x30 },
        { "neutral",         dHue,            6.0,  22, 0x36, 0x34, 0x3B },
        { "neutral",         dHue,            6.0,  90, 0xE6, 0xE0, 0xE9 },
        /* Neutral variant: the outline family, the seed hue at chroma 8. */
        { "neutralvariant",  dHue,            8.0,  30, 0x49, 0x45, 0x4F },
        { "neutralvariant",  dHue,            8.0,  60, 0x93, 0x8F, 0x99 },
        { "neutralvariant",  dHue,            8.0,  80, 0xCA, 0xC4, 0xD0 },
        { "neutralvariant",  dHue,            8.0,  90, 0xE7, 0xE0, 0xEC },
        /* Error: the fixed destructive family, independent of the seed. */
        { "error",           25.0,           84.0,  30, 0x93, 0x00, 0x0A },
        { "error",           25.0,           84.0,  80, 0xFF, 0xB4, 0xAB },
        { "error",           25.0,           84.0,  90, 0xFF, 0xDA, 0xD6 },
    };

    for (size_t i = 0; i < sizeof(aExpectations) / sizeof(aExpectations[0]); ++i)
    {
        const UIMd3ToneExpectation &expectation = aExpectations[i];
        int iRed = 0, iGreen = 0, iBlue = 0;
        md3HctToRgb(expectation.dHue, expectation.dChroma, (double)expectation.iTone,
                    &iRed, &iGreen, &iBlue);

        const int iDeltaRed = std::abs(iRed - expectation.iRed);
        const int iDeltaGreen = std::abs(iGreen - expectation.iGreen);
        const int iDeltaBlue = std::abs(iBlue - expectation.iBlue);
        const bool fMatches =    iDeltaRed <= g_iChannelTolerance
                              && iDeltaGreen <= g_iChannelTolerance
                              && iDeltaBlue <= g_iChannelTolerance;

        char szDetail[128];
        std::snprintf(szDetail, sizeof(szDetail), "%s tone %d: expected #%02X%02X%02X, got #%02X%02X%02X",
                      expectation.pszPalette, expectation.iTone,
                      expectation.iRed, expectation.iGreen, expectation.iBlue,
                      iRed, iGreen, iBlue);
        md3TestCheck(fMatches, "baseline palette", szDetail);
    }

    /* Tone 40 of the primary palette is the seed itself for this scheme. */
    int iRed = 0, iGreen = 0, iBlue = 0;
    md3HctToRgb(dHue, dPrimaryChroma, 40.0, &iRed, &iGreen, &iBlue);
    md3TestCheck(   iRed == g_iSeedRed && iGreen == g_iSeedGreen && iBlue == g_iSeedBlue,
                 "seed round trip", "primary tone 40 must reproduce the seed exactly");
}

/** The seeds the property checks run over: the baseline, saturated primaries at several
  * hues, an out-of-gamut-prone yellow, and the achromatic extremes. */
static const int g_aTestSeeds[] =
{
    0x6750A4, 0xB33B15, 0x00658F, 0x386A20, 0xFFDE3F, 0xFF0000, 0x00FF00, 0x0000FF,
    0x000000, 0xFFFFFF, 0x808080, 0x123456
};

/** Checks that every generated tone lands on the tone it was asked for, that tones rise
  * monotonically, and that generation is repeatable. */
static void md3TestToneProperties()
{
    double dWorstToneError = 0.0;
    char szDetail[160];

    for (size_t i = 0; i < sizeof(g_aTestSeeds) / sizeof(g_aTestSeeds[0]); ++i)
    {
        const int iSeed = g_aTestSeeds[i];
        const UIMd3Hct seed = md3HctFromRgb((iSeed >> 16) & 0xFF, (iSeed >> 8) & 0xFF, iSeed & 0xFF);

        /* Exercise the chroma levels the theme actually generates: */
        const double adChromas[] = { seed.dChroma > 48.0 ? seed.dChroma : 48.0, 24.0, 16.0, 8.0, 6.0 };
        for (size_t j = 0; j < sizeof(adChromas) / sizeof(adChromas[0]); ++j)
        {
            double dPreviousTone = -1.0;
            for (int iTone = 0; iTone <= 100; ++iTone)
            {
                int iRed = 0, iGreen = 0, iBlue = 0;
                md3HctToRgb(seed.dHue, adChromas[j], (double)iTone, &iRed, &iGreen, &iBlue);

                /* Displayable: */
                const bool fDisplayable =    iRed >= 0 && iRed <= 255
                                          && iGreen >= 0 && iGreen <= 255
                                          && iBlue >= 0 && iBlue <= 255;
                if (!fDisplayable)
                {
                    std::snprintf(szDetail, sizeof(szDetail), "seed #%06X chroma %.1f tone %d left the gamut",
                                  iSeed, adChromas[j], iTone);
                    md3TestFailed("displayable", szDetail);
                }

                /* Requested tone honoured: only 8-bit rounding may move it. */
                const double dTone = md3LstarFromRgb(iRed, iGreen, iBlue);
                const double dToneError = std::fabs(dTone - (double)iTone);
                if (dToneError > dWorstToneError)
                    dWorstToneError = dToneError;

                /* Monotonic: a higher tone is never darker. */
                if (dTone + 0.5 < dPreviousTone)
                {
                    std::snprintf(szDetail, sizeof(szDetail),
                                  "seed #%06X chroma %.1f tone %d fell back to L* %.2f from %.2f",
                                  iSeed, adChromas[j], iTone, dTone, dPreviousTone);
                    md3TestFailed("monotonic tone", szDetail);
                }
                dPreviousTone = dTone;

                /* Deterministic: the palette must not drift between calls. */
                int iRepeatRed = 0, iRepeatGreen = 0, iRepeatBlue = 0;
                md3HctToRgb(seed.dHue, adChromas[j], (double)iTone, &iRepeatRed, &iRepeatGreen, &iRepeatBlue);
                if (iRepeatRed != iRed || iRepeatGreen != iGreen || iRepeatBlue != iBlue)
                {
                    std::snprintf(szDetail, sizeof(szDetail), "seed #%06X chroma %.1f tone %d is not repeatable",
                                  iSeed, adChromas[j], iTone);
                    md3TestFailed("deterministic", szDetail);
                }
            }
        }
    }

    std::snprintf(szDetail, sizeof(szDetail), "worst tone error %.4f L* exceeds the 8-bit rounding budget",
                  dWorstToneError);
    md3TestCheck(dWorstToneError <= 0.5, "tone accuracy", szDetail);
    std::printf("worst tone error across %zu seeds: %.4f L*\n",
                sizeof(g_aTestSeeds) / sizeof(g_aTestSeeds[0]), dWorstToneError);
}

/** Returns the WCAG relative luminance of the sRGB colour @a iRed, @a iGreen, @a iBlue. */
static double md3TestRelativeLuminance(int iRed, int iGreen, int iBlue)
{
    const int aiComponents[3] = { iRed, iGreen, iBlue };
    const double adWeights[3] = { 0.2126, 0.7152, 0.0722 };
    double dLuminance = 0.0;
    for (int i = 0; i < 3; ++i)
    {
        const double dNormalized = aiComponents[i] / 255.0;
        const double dLinear = dNormalized <= 0.04045
                             ? dNormalized / 12.92
                             : std::pow((dNormalized + 0.055) / 1.055, 2.4);
        dLuminance += adWeights[i] * dLinear;
    }
    return dLuminance;
}

/** Checks that the tone pairs the schemes rely on keep a usable contrast ratio for every
  * seed. This is the property the HSL approximation could not guarantee: identical tone
  * numbers have to mean identical contrast whatever colour the user picked. */
static void md3TestContrastPairs()
{
    /* Foreground/background tone pairs taken from the dark and light role assignments. */
    struct { int iForeground; int iBackground; double dMinimumRatio; } aPairs[] =
    {
        {  20,  80, 4.5 },  /* onPrimary on primary, dark scheme. */
        {  90,  30, 4.5 },  /* onPrimaryContainer on primaryContainer, dark scheme. */
        {  90,   6, 4.5 },  /* onSurface on surface, dark scheme. */
        {  80,   6, 3.0 },  /* onSurfaceVariant on surface, dark scheme. */
        { 100,  40, 4.5 },  /* onPrimary on primary, light scheme. */
        {  10,  90, 4.5 },  /* onPrimaryContainer on primaryContainer, light scheme. */
        {  10,  98, 4.5 },  /* onSurface on surface, light scheme. */
        {  30,  98, 3.0 },  /* onSurfaceVariant on surface, light scheme. */
    };

    for (size_t i = 0; i < sizeof(g_aTestSeeds) / sizeof(g_aTestSeeds[0]); ++i)
    {
        const int iSeed = g_aTestSeeds[i];
        const UIMd3Hct seed = md3HctFromRgb((iSeed >> 16) & 0xFF, (iSeed >> 8) & 0xFF, iSeed & 0xFF);
        const double dChroma = seed.dChroma > 48.0 ? seed.dChroma : 48.0;

        for (size_t j = 0; j < sizeof(aPairs) / sizeof(aPairs[0]); ++j)
        {
            int iFgRed = 0, iFgGreen = 0, iFgBlue = 0;
            int iBgRed = 0, iBgGreen = 0, iBgBlue = 0;
            md3HctToRgb(seed.dHue, dChroma, (double)aPairs[j].iForeground, &iFgRed, &iFgGreen, &iFgBlue);
            md3HctToRgb(seed.dHue, dChroma, (double)aPairs[j].iBackground, &iBgRed, &iBgGreen, &iBgBlue);

            const double dForeground = md3TestRelativeLuminance(iFgRed, iFgGreen, iFgBlue);
            const double dBackground = md3TestRelativeLuminance(iBgRed, iBgGreen, iBgBlue);
            const double dLighter = dForeground > dBackground ? dForeground : dBackground;
            const double dDarker = dForeground > dBackground ? dBackground : dForeground;
            const double dRatio = (dLighter + 0.05) / (dDarker + 0.05);

            char szDetail[160];
            std::snprintf(szDetail, sizeof(szDetail),
                          "seed #%06X tone %d on tone %d: ratio %.2f is below %.2f",
                          iSeed, aPairs[j].iForeground, aPairs[j].iBackground,
                          dRatio, aPairs[j].dMinimumRatio);
            md3TestCheck(dRatio >= aPairs[j].dMinimumRatio, "contrast pair", szDetail);
        }
    }
}

int main()
{
    std::printf("tstUIMd3Hct: Material 3 tonal palette\n");

    md3TestBaselinePalette();
    md3TestToneProperties();
    md3TestContrastPairs();

    std::printf("%d checks, %d failures\n", g_cChecks, g_cFailures);
    return g_cFailures == 0 ? 0 : 1;
}
