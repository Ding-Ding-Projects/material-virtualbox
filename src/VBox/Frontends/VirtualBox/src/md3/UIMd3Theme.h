/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Theme class declaration - tonal palette generation and theme distribution.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Theme_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Theme_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QByteArray>
#include <QColor>
#include <QFont>
#include <QHash>
#include <QObject>
#include <QVariantMap>

/* IPRT compatibility macros: */
#include <iprt/cdefs.h>

/* GUI includes: */
#include "UILibraryDefs.h"
#include "UIMd3Tokens.h"

/** QObject extension owning the Material 3 palette for the whole process.
  * The theme derives every colour role from a single seed colour using the
  * HCT tonal-palette rules, applies the active scheme, and republishes itself
  * whenever the seed, scheme, density or per-element overrides change. */
class SHARED_LIBRARY_STUFF UIMd3Theme : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies every widget that the palette, type scale or density changed. */
    void sigThemeChanged();

public:

    /** Returns the process-wide theme singleton. */
    static UIMd3Theme *instance();
    /** Creates the singleton. Called once from main(). */
    static void create();
    /** Destroys the singleton. Called once on shutdown. */
    static void destroy();

    /** @name Palette
      * @{ */
        /** Returns the colour for @a enmRole. */
        QColor color(UIMd3ColorRole enmRole) const;
        /** Returns the colour for @a enmRole blended with a state layer of @a iOpacityPercent. */
        QColor stateLayer(UIMd3ColorRole enmRole, int iOpacityPercent) const;
        /** Returns the current seed colour. */
        QColor seed() const { return m_seed; }
        /** Defines the seed colour and regenerates every tonal palette. */
        void setSeed(const QColor &seed);
        /** Returns the active scheme. */
        UIMd3Scheme scheme() const { return m_enmScheme; }
        /** Defines the active scheme. */
        void setScheme(UIMd3Scheme enmScheme);
    /** @} */

    /** @name Typography and density
      * @{ */
        /** Returns the font for @a enmRole, already scaled. */
        QFont font(UIMd3TypeRole enmRole) const;
        /** Returns the global font scale factor. */
        double fontScale() const { return m_dFontScale; }
        /** Defines the global font scale factor. */
        void setFontScale(double dScale);
        /** Returns the selected global font family, or the shipped fallback. */
        QString fontFamily() const { return m_strFontFamily; }
        /** Defines the global font family; unavailable families are rejected. */
        void setFontFamily(const QString &strFamily);
        /** Returns the global font weight override, or -1 for role weights. */
        int fontWeight() const { return m_iFontWeight; }
        /** Defines the global font weight override; -1 restores role weights. */
        void setFontWeight(int iWeight);
        /** Returns whether the compact density is active. */
        bool isCompact() const { return m_fCompact; }
        /** Defines the compact density. */
        void setCompact(bool fCompact);
        /** Returns the user-facing display brand without changing technical identity. */
        QString brandName() const { return m_strBrandName; }
        /** Defines the user-facing display brand; empty values reset to the shipped name. */
        void setBrandName(const QString &strName);
        /** Returns the vertical size of a standard control for the active density. */
        int controlHeight() const { return m_fCompact ? 36 : 40; }
        /** Returns the standard content gutter for the active density. */
        int gutter() const { return m_fCompact ? 12 : 16; }
    /** @} */

    /** @name Per-element appearance overrides
      * @{ */
        /** Returns the override stored for @a strKey, invalid when none exists. */
        UIMd3Appearance appearance(const QString &strKey) const;
        /** Stores @a appearance for @a strKey after strict validation and persists it.
          * Returns false without changing the live state when a value is malformed or
          * outside the bounded Material 3 element contract. */
        bool setAppearance(const QString &strKey, const UIMd3Appearance &appearance);
        /** Removes the override stored for @a strKey. */
        void clearAppearance(const QString &strKey);
        /** Returns every stored override key. */
        QStringList appearanceKeys() const { return m_appearances.keys(); }
    /** @} */

    /** @name Named themes
      * @{ */
        /** Saves the current palette, typography, density, brand, and element overrides
          * as @a strName. Returns false without changing the saved set when the name is
          * empty, oversized, or would exceed the bounded preset count. */
        bool saveNamedTheme(const QString &strName);
        /** Applies the named theme @a strName. Returns false when it does not exist. */
        bool applyNamedTheme(const QString &strName);
        /** Returns every saved theme name. */
        QStringList namedThemes() const { return m_namedThemes.keys(); }
        /** Serialises every named theme to JSON for export. */
        QByteArray exportNamedThemes() const;
        /** Merges named themes from the JSON blob @a data transactionally. Returns false
          * on a parse error or malformed/out-of-range entry without changing saved themes. */
        bool importNamedThemes(const QByteArray &data);
    /** @} */

    /** Serialises the validated appearance state for local history. */
    QByteArray serializeState() const;
    /** Applies a validated local-history appearance state without recording a duplicate change. */
    bool restoreState(const QByteArray &data);

    /** Reloads seed, scheme, density and overrides from extradata. */
    void loadFromExtraData();
    /** Writes seed, scheme, density and overrides to extradata. */
    void saveToExtraData() const;

public slots:

    /** Applies a selected theme revision requested by the history browser. */
    void sltRestoreHistoryRevision(const QString &strRevisionId);

private:

    /** Constructs the theme. */
    UIMd3Theme();
    /** Destructs the theme. */
    virtual ~UIMd3Theme() override;

    /** Rebuilds every tonal palette from the current seed and scheme. */
    void regenerate();
    /** Returns the tone @a iTone (0..100) of the tonal palette built from @a base. */
    static QColor tone(const QColor &base, int iTone);
    /** Returns the effective scheme, resolving UIMd3Scheme_System against the host. */
    UIMd3Scheme effectiveScheme() const;
    /** Records a validated theme change without making history persistence fatal. */
    void recordHistory(const QString &strAction, const QString &strDetail);

    static UIMd3Theme            *s_pInstance;
    QColor                        m_seed;
    UIMd3Scheme                   m_enmScheme;
    double                        m_dFontScale;
    bool                          m_fCompact;
    QString                       m_strFontFamily;
    int                           m_iFontWeight;
    QString                       m_strBrandName;
    QColor                        m_colors[UIMd3ColorRole_Max];
    QHash<QString, UIMd3Appearance> m_appearances;
    QHash<QString, QVariantMap>   m_namedThemes;
    bool                          m_fRestoring;
};

/** Convenience accessor mirroring uiCommon(). */
inline UIMd3Theme &md3Theme() { return *UIMd3Theme::instance(); }
/** Convenience colour accessor. */
inline QColor md3(UIMd3ColorRole enmRole) { return UIMd3Theme::instance()->color(enmRole); }

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Theme_h */
