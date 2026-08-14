/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 HCT colour space implementation - CAM16 hue, chroma and L* tone.
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

/* GUI includes: */
#include "UIMd3Hct.h"

/** Portable value of pi; the common platform macro is not part of standard C++. */
static const double g_dMd3Pi = 3.141592653589793238462643383279502884;

/*********************************************************************************************************************************
*   sRGB and CIE L* helpers                                                                                                      *
*********************************************************************************************************************************/

/** Returns the sign of @a dValue as -1, 0 or 1. */
static double md3Signum(double dValue)
{
    return dValue < 0 ? -1.0 : (dValue > 0 ? 1.0 : 0.0);
}

/** Converts the 0..255 sRGB component @a iComponent to its 0..100 linear counterpart. */
static double md3Linearized(int iComponent)
{
    const double dNormalized = iComponent / 255.0;
    if (dNormalized <= 0.040449936)
        return dNormalized / 12.92 * 100.0;
    return std::pow((dNormalized + 0.055) / 1.055, 2.4) * 100.0;
}

/** Converts the 0..100 linear component @a dComponent to its rounded 0..255 sRGB counterpart. */
static int md3Delinearized(double dComponent)
{
    const double dNormalized = dComponent / 100.0;
    double dDelinearized = 0.0;
    if (dNormalized <= 0.0031308)
        dDelinearized = dNormalized * 12.92;
    else
        dDelinearized = 1.055 * std::pow(dNormalized, 1.0 / 2.4) - 0.055;
    const int iResult = (int)std::lround(dDelinearized * 255.0);
    return iResult < 0 ? 0 : (iResult > 255 ? 255 : iResult);
}

/** Returns the CIE L* of the CIE Y value @a dY (0..100). */
static double md3LstarFromY(double dY)
{
    const double dNormalized = dY / 100.0;
    if (dNormalized <= 216.0 / 24389.0)
        return dNormalized * 24389.0 / 27.0;
    return 116.0 * std::cbrt(dNormalized) - 16.0;
}

/** Returns the CIE Y value (0..100) of the CIE L* @a dLstar. */
static double md3YFromLstar(double dLstar)
{
    const double dFt = (dLstar + 16.0) / 116.0;
    const double dFt3 = dFt * dFt * dFt;
    if (dFt3 > 216.0 / 24389.0)
        return dFt3 * 100.0;
    return dLstar / (24389.0 / 27.0) * 100.0;
}

double md3LstarFromRgb(int iRed, int iGreen, int iBlue)
{
    const double dY =   0.2126 * md3Linearized(iRed)
                      + 0.7152 * md3Linearized(iGreen)
                      + 0.0722 * md3Linearized(iBlue);
    return md3LstarFromY(dY);
}

/** Stores the neutral sRGB colour of tone @a dTone in @a piRed, @a piGreen and @a piBlue. */
static void md3GreyFromTone(double dTone, int *piRed, int *piGreen, int *piBlue)
{
    const int iComponent = md3Delinearized(md3YFromLstar(dTone));
    *piRed = iComponent;
    *piGreen = iComponent;
    *piBlue = iComponent;
}

/*********************************************************************************************************************************
*   CAM16 viewing conditions                                                                                                     *
*********************************************************************************************************************************/

/** The CAM16 viewing conditions Material 3 generates its palettes under: the sRGB D65
  * white point, an average surround and a mid-grey background. They are constant for the
  * whole process, so they are computed once. */
struct UIMd3ViewingConditions
{
    double adRgbD[3];
    double dAw;
    double dNbb;
    double dNcb;
    double dC;
    double dNc;
    double dN;
    double dFl;
    double dFlRoot;
    double dZ;
};

/** Returns the process-wide CAM16 viewing conditions. */
static const UIMd3ViewingConditions &md3ViewingConditions()
{
    static UIMd3ViewingConditions s_conditions;
    static bool s_fPrepared = false;
    if (!s_fPrepared)
    {
        /* The sRGB D65 white point in CIE XYZ, scaled to Y = 100: */
        const double adWhitePoint[3] = { 95.047, 100.0, 108.883 };

        /* CAM16 works in a sharpened cone-response space; transform the white point into it: */
        const double adRgbW[3] =
        {
             0.401288 * adWhitePoint[0] + 0.650173 * adWhitePoint[1] - 0.051461 * adWhitePoint[2],
            -0.250268 * adWhitePoint[0] + 1.204414 * adWhitePoint[1] + 0.045854 * adWhitePoint[2],
            -0.002079 * adWhitePoint[0] + 0.048952 * adWhitePoint[1] + 0.953127 * adWhitePoint[2]
        };

        /* Average surround: f = 1.0 gives c = 0.69 and nc = 1.0. */
        const double dF = 1.0;
        s_conditions.dC = 0.69;
        s_conditions.dNc = dF;

        /* A mid-grey (L* 50) background against a display-typical adapting luminance: */
        const double dAdaptingLuminance = (200.0 / g_dMd3Pi) * md3YFromLstar(50.0) / 100.0;
        const double dBackgroundY = md3YFromLstar(50.0);
        s_conditions.dN = dBackgroundY / adWhitePoint[1];
        s_conditions.dZ = 1.48 + std::sqrt(s_conditions.dN);
        s_conditions.dNbb = 0.725 / std::pow(s_conditions.dN, 0.2);
        s_conditions.dNcb = s_conditions.dNbb;

        /* Degree of chromatic adaptation to the illuminant: */
        double dD = dF * (1.0 - (1.0 / 3.6) * std::exp((-dAdaptingLuminance - 42.0) / 92.0));
        dD = dD < 0.0 ? 0.0 : (dD > 1.0 ? 1.0 : dD);
        for (int i = 0; i < 3; ++i)
            s_conditions.adRgbD[i] = dD * (adWhitePoint[1] / adRgbW[i]) + 1.0 - dD;

        /* Luminance-level adaptation factor: */
        const double dK = 1.0 / (5.0 * dAdaptingLuminance + 1.0);
        const double dK4 = dK * dK * dK * dK;
        const double dK4F = 1.0 - dK4;
        s_conditions.dFl =   dK4 * dAdaptingLuminance
                           + 0.1 * dK4F * dK4F * std::cbrt(5.0 * dAdaptingLuminance);
        s_conditions.dFlRoot = std::pow(s_conditions.dFl, 0.25);

        /* Achromatic response to the white point: */
        double adRgbA[3];
        for (int i = 0; i < 3; ++i)
        {
            const double dFactor = std::pow(s_conditions.dFl * s_conditions.adRgbD[i] * adRgbW[i] / 100.0, 0.42);
            adRgbA[i] = 400.0 * dFactor / (dFactor + 27.13);
        }
        s_conditions.dAw = (2.0 * adRgbA[0] + adRgbA[1] + 0.05 * adRgbA[2]) * s_conditions.dNbb;

        s_fPrepared = true;
    }
    return s_conditions;
}

/*********************************************************************************************************************************
*   CAM16 forward and inverse model                                                                                              *
*********************************************************************************************************************************/

/** A CAM16 appearance correlate set, reduced to what tonal-palette generation needs. */
struct UIMd3Cam16
{
    double dHue;
    double dChroma;
    /** Holds the lightness correlate J. */
    double dJ;
};

/** Returns the CAM16 correlates of the opaque sRGB colour @a iRed, @a iGreen, @a iBlue. */
static UIMd3Cam16 md3Cam16FromRgb(int iRed, int iGreen, int iBlue)
{
    const UIMd3ViewingConditions &vc = md3ViewingConditions();

    const double dLinR = md3Linearized(iRed);
    const double dLinG = md3Linearized(iGreen);
    const double dLinB = md3Linearized(iBlue);

    const double dX = 0.41233895 * dLinR + 0.35762064 * dLinG + 0.18051042 * dLinB;
    const double dY = 0.21260000 * dLinR + 0.71520000 * dLinG + 0.07220000 * dLinB;
    const double dZ = 0.01932141 * dLinR + 0.11916382 * dLinG + 0.95034478 * dLinB;

    const double adCone[3] =
    {
         0.401288 * dX + 0.650173 * dY - 0.051461 * dZ,
        -0.250268 * dX + 1.204414 * dY + 0.045854 * dZ,
        -0.002079 * dX + 0.048952 * dY + 0.953127 * dZ
    };

    double adAdapted[3];
    for (int i = 0; i < 3; ++i)
    {
        const double dDiscounted = vc.adRgbD[i] * adCone[i];
        const double dFactor = std::pow(vc.dFl * std::fabs(dDiscounted) / 100.0, 0.42);
        adAdapted[i] = md3Signum(dDiscounted) * 400.0 * dFactor / (dFactor + 27.13);
    }

    const double dA = (11.0 * adAdapted[0] - 12.0 * adAdapted[1] + adAdapted[2]) / 11.0;
    const double dB = (adAdapted[0] + adAdapted[1] - 2.0 * adAdapted[2]) / 9.0;
    const double dU = (20.0 * adAdapted[0] + 20.0 * adAdapted[1] + 21.0 * adAdapted[2]) / 20.0;
    const double dP2 = (40.0 * adAdapted[0] + 20.0 * adAdapted[1] + adAdapted[2]) / 20.0;

    double dHueDegrees = std::atan2(dB, dA) * 180.0 / g_dMd3Pi;
    if (dHueDegrees < 0.0)
        dHueDegrees += 360.0;
    else if (dHueDegrees >= 360.0)
        dHueDegrees -= 360.0;

    const double dAc = dP2 * vc.dNbb;
    const double dJ = 100.0 * std::pow(dAc / vc.dAw, vc.dC * vc.dZ);

    /* The eccentricity factor is discontinuous at hue 20.14 degrees: */
    const double dHuePrime = dHueDegrees < 20.14 ? dHueDegrees + 360.0 : dHueDegrees;
    const double dEHue = 0.25 * (std::cos(dHuePrime * g_dMd3Pi / 180.0 + 2.0) + 3.8);
    const double dP1 = 50000.0 / 13.0 * dEHue * vc.dNc * vc.dNcb;
    const double dT = dP1 * std::sqrt(dA * dA + dB * dB) / (dU + 0.305);
    const double dAlpha = std::pow(dT, 0.9) * std::pow(1.64 - std::pow(0.29, vc.dN), 0.73);

    UIMd3Cam16 cam;
    cam.dHue = dHueDegrees;
    cam.dJ = dJ;
    cam.dChroma = dAlpha * std::sqrt(dJ / 100.0);
    return cam;
}

/** Stores the linear sRGB components (nominally 0..100) of the CAM16 coordinate
  * @a dJ, @a dChroma, @a dHueDegrees in @a adLinearRgb.
  *
  * The result is deliberately left unclamped: a component outside 0..100 is how the
  * caller learns that the requested coordinate is not displayable. */
static void md3LinearRgbFromJch(double dJ, double dChroma, double dHueDegrees,
                                double adLinearRgb[3])
{
    const UIMd3ViewingConditions &vc = md3ViewingConditions();
    const double dHueRadians = dHueDegrees * g_dMd3Pi / 180.0;

    const double dAlpha = (dChroma == 0.0 || dJ == 0.0) ? 0.0 : dChroma / std::sqrt(dJ / 100.0);
    const double dT = std::pow(dAlpha / std::pow(1.64 - std::pow(0.29, vc.dN), 0.73), 1.0 / 0.9);
    const double dEHue = 0.25 * (std::cos(dHueRadians + 2.0) + 3.8);
    const double dAc = vc.dAw * std::pow(dJ / 100.0, 1.0 / (vc.dC * vc.dZ));
    const double dP1 = dEHue * (50000.0 / 13.0) * vc.dNc * vc.dNcb;
    const double dP2 = dAc / vc.dNbb;

    const double dHSin = std::sin(dHueRadians);
    const double dHCos = std::cos(dHueRadians);
    const double dGamma = 23.0 * (dP2 + 0.305) * dT
                        / (23.0 * dP1 + 11.0 * dT * dHCos + 108.0 * dT * dHSin);
    const double dA = dGamma * dHCos;
    const double dB = dGamma * dHSin;

    const double adAdapted[3] =
    {
        (460.0 * dP2 + 451.0 * dA + 288.0 * dB) / 1403.0,
        (460.0 * dP2 - 891.0 * dA - 261.0 * dB) / 1403.0,
        (460.0 * dP2 - 220.0 * dA - 6300.0 * dB) / 1403.0
    };

    double adCone[3];
    for (int i = 0; i < 3; ++i)
    {
        const double dAbsolute = std::fabs(adAdapted[i]);
        double dBase = 27.13 * dAbsolute / (400.0 - dAbsolute);
        if (dBase < 0.0)
            dBase = 0.0;
        const double dDiscounted = md3Signum(adAdapted[i]) * (100.0 / vc.dFl) * std::pow(dBase, 1.0 / 0.42);
        adCone[i] = dDiscounted / vc.adRgbD[i];
    }

    const double dX =  1.86206786 * adCone[0] - 1.01125463 * adCone[1] + 0.14918677 * adCone[2];
    const double dY =  0.38752654 * adCone[0] + 0.62144744 * adCone[1] - 0.00897398 * adCone[2];
    const double dZ = -0.01584150 * adCone[0] - 0.03412294 * adCone[1] + 1.04996444 * adCone[2];

    adLinearRgb[0] =  3.2413774792388685 * dX - 1.5376652402851851 * dY - 0.49885366846268053 * dZ;
    adLinearRgb[1] = -0.9691452513005321 * dX + 1.8758853451067872 * dY + 0.04156585616912061 * dZ;
    adLinearRgb[2] =  0.05562093689691305 * dX - 0.20395524564742123 * dY + 1.0571799111220335 * dZ;
}

/** Returns whether every component of @a adLinearRgb lies inside the sRGB gamut. */
static bool md3IsInGamut(const double adLinearRgb[3])
{
    for (int i = 0; i < 3; ++i)
        if (adLinearRgb[i] < 0.0 || adLinearRgb[i] > 100.0)
            return false;
    return true;
}

/*********************************************************************************************************************************
*   HCT resolution                                                                                                               *
*********************************************************************************************************************************/

UIMd3Hct md3HctFromRgb(int iRed, int iGreen, int iBlue)
{
    const UIMd3Cam16 cam = md3Cam16FromRgb(iRed, iGreen, iBlue);
    UIMd3Hct hct;
    hct.dHue = cam.dHue;
    hct.dChroma = cam.dChroma;
    hct.dTone = md3LstarFromRgb(iRed, iGreen, iBlue);
    return hct;
}

/** Solves the CAM16 lightness J that puts the colour @a dHueDegrees / @a dChroma on
  * exactly tone @a dTone, and stores its linear sRGB components in @a adLinearRgb.
  *
  * CIE Y grows monotonically with J at a fixed hue and chroma, so a plain bisection
  * converges on the requested tone to full double precision. That exactness is the point:
  * tone is the contrast axis of the palette, so it must not be traded for search speed. */
static void md3SolveToneAtChroma(double dHueDegrees, double dChroma, double dTone,
                                 double adLinearRgb[3])
{
    const double dTargetY = md3YFromLstar(dTone);

    double dLow = 0.0;
    double dHigh = 100.0;
    for (int i = 0; i < 64; ++i)
    {
        const double dMid = dLow + (dHigh - dLow) / 2.0;
        md3LinearRgbFromJch(dMid, dChroma, dHueDegrees, adLinearRgb);
        const double dY =   0.2126 * adLinearRgb[0]
                          + 0.7152 * adLinearRgb[1]
                          + 0.0722 * adLinearRgb[2];
        if (dY < dTargetY)
            dLow = dMid;
        else
            dHigh = dMid;
    }
    md3LinearRgbFromJch(dLow + (dHigh - dLow) / 2.0, dChroma, dHueDegrees, adLinearRgb);
}

void md3HctToRgb(double dHue, double dChroma, double dTone,
                 int *piRed, int *piGreen, int *piBlue)
{
    dTone = dTone < 0.0 ? 0.0 : (dTone > 100.0 ? 100.0 : dTone);

    /* Black, white and the neutral axis have no hue to preserve: */
    if (dChroma < 1.0 || std::lround(dTone) <= 0 || std::lround(dTone) >= 100)
    {
        md3GreyFromTone(dTone, piRed, piGreen, piBlue);
        return;
    }

    dHue = std::fmod(dHue, 360.0);
    if (dHue < 0.0)
        dHue += 360.0;

    /* Take the requested chroma when sRGB can display it at this tone: */
    double adLinearRgb[3] = { 0.0, 0.0, 0.0 };
    md3SolveToneAtChroma(dHue, dChroma, dTone, adLinearRgb);
    if (md3IsInGamut(adLinearRgb))
    {
        *piRed = md3Delinearized(adLinearRgb[0]);
        *piGreen = md3Delinearized(adLinearRgb[1]);
        *piBlue = md3Delinearized(adLinearRgb[2]);
        return;
    }

    /* Otherwise walk the chroma axis towards neutral for the most colourful displayable
     * colour that still has the requested tone. Tone is never sacrificed, chroma is. */
    double adAnswer[3] = { 0.0, 0.0, 0.0 };
    md3GreyFromTone(dTone, piRed, piGreen, piBlue);
    bool fHaveAnswer = false;

    double dLow = 0.0;
    double dHigh = dChroma;
    while (dHigh - dLow > 0.0001)
    {
        const double dMid = dLow + (dHigh - dLow) / 2.0;
        md3SolveToneAtChroma(dHue, dMid, dTone, adLinearRgb);
        if (md3IsInGamut(adLinearRgb))
        {
            fHaveAnswer = true;
            adAnswer[0] = adLinearRgb[0];
            adAnswer[1] = adLinearRgb[1];
            adAnswer[2] = adLinearRgb[2];
            dLow = dMid;
        }
        else
            dHigh = dMid;
    }

    if (fHaveAnswer)
    {
        *piRed = md3Delinearized(adAnswer[0]);
        *piGreen = md3Delinearized(adAnswer[1]);
        *piBlue = md3Delinearized(adAnswer[2]);
    }
}
