/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 semantic tokens shared by the native frontend.
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Tokens_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Tokens_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QColor>
#include <QFont>
#include <QMetaType>
#include <QString>

enum UIMd3ColorRole
{
    UIMd3ColorRole_Primary, UIMd3ColorRole_OnPrimary,
    UIMd3ColorRole_PrimaryContainer, UIMd3ColorRole_OnPrimaryContainer,
    UIMd3ColorRole_Secondary, UIMd3ColorRole_OnSecondary,
    UIMd3ColorRole_SecondaryContainer, UIMd3ColorRole_OnSecondaryContainer,
    UIMd3ColorRole_Tertiary, UIMd3ColorRole_OnTertiary,
    UIMd3ColorRole_TertiaryContainer, UIMd3ColorRole_OnTertiaryContainer,
    UIMd3ColorRole_Error, UIMd3ColorRole_OnError,
    UIMd3ColorRole_ErrorContainer, UIMd3ColorRole_OnErrorContainer,
    UIMd3ColorRole_Surface, UIMd3ColorRole_OnSurface,
    UIMd3ColorRole_OnSurfaceVariant, UIMd3ColorRole_SurfaceContainerLowest,
    UIMd3ColorRole_SurfaceContainerLow, UIMd3ColorRole_SurfaceContainer,
    UIMd3ColorRole_SurfaceContainerHigh, UIMd3ColorRole_SurfaceContainerHighest,
    UIMd3ColorRole_Outline, UIMd3ColorRole_OutlineVariant,
    UIMd3ColorRole_Scrim, UIMd3ColorRole_Max
};

enum UIMd3TypeRole
{
    UIMd3TypeRole_DisplayLarge, UIMd3TypeRole_DisplayMedium, UIMd3TypeRole_DisplaySmall,
    UIMd3TypeRole_HeadlineLarge, UIMd3TypeRole_HeadlineMedium, UIMd3TypeRole_HeadlineSmall,
    UIMd3TypeRole_TitleLarge, UIMd3TypeRole_TitleMedium, UIMd3TypeRole_TitleSmall,
    UIMd3TypeRole_BodyLarge, UIMd3TypeRole_BodyMedium, UIMd3TypeRole_BodySmall,
    UIMd3TypeRole_LabelLarge, UIMd3TypeRole_LabelMedium, UIMd3TypeRole_LabelSmall,
    UIMd3TypeRole_Max
};

namespace UIMd3Shape { enum { None = 0, ExtraSmall = 4, Small = 8, Medium = 12, Large = 16, ExtraLarge = 28, Full = 999 }; }
namespace UIMd3Elevation { enum { Level0 = 0, Level1 = 3, Level2 = 6, Level3 = 8, Level4 = 10, Level5 = 12 }; }
namespace UIMd3StateLayer { enum { Hover = 8, Focus = 10, Pressed = 10, Dragged = 16, Disabled = 38 }; }
namespace UIMd3Motion { enum { Short2 = 100, Short4 = 200, Medium2 = 300, Medium4 = 400, Long2 = 500 }; }

enum UIMd3Scheme
{
    UIMd3Scheme_Dark, UIMd3Scheme_Light, UIMd3Scheme_System,
    UIMd3Scheme_HighContrastDark, UIMd3Scheme_HighContrastLight
};

struct UIMd3Appearance
{
    UIMd3Appearance()
        : fValid(false), iRadius(UIMd3Shape::Large), dScale(1.0), iWeight(QFont::Normal) {}
    bool fValid;
    QColor seed;
    QString strFont;
    int iRadius;
    double dScale;
    int iWeight;
};
Q_DECLARE_METATYPE(UIMd3Appearance);

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Tokens_h */
