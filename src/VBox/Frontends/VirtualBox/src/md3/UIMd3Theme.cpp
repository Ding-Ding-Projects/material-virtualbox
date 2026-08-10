/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Theme class implementation.
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


/* Qt includes: */
#include <QApplication>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QPalette>
#include <QStyleHints>
#include <QVariantMap>

/* Standard C++ includes: */
#include <cmath>
#include <limits>

/* GUI includes: */
#include "UIExtraDataManager.h"
#include "UIMd3Hct.h"
#include "UIMd3History.h"
#include "UIMd3Theme.h"

/* Extradata keys owned by the Material 3 shell. */
static const char *g_pszKeySeed       = "GUI/Md3/Seed";
static const char *g_pszKeyScheme     = "GUI/Md3/Scheme";
static const char *g_pszKeyScale      = "GUI/Md3/FontScale";
static const char *g_pszKeyCompact    = "GUI/Md3/Compact";
static const char *g_pszKeyFont       = "GUI/Md3/FontFamily";
static const char *g_pszKeyWeight     = "GUI/Md3/FontWeight";
static const char *g_pszKeyBrand      = "GUI/Md3/BrandName";
static const char *g_pszKeyAppearance = "GUI/Md3/Appearance";
static const char *g_pszKeyThemes     = "GUI/Md3/NamedThemes";
static const int g_iThemeHistorySchema = 1;
static const int g_iMaxHistoryAppearanceEntries = 256;
static const int g_iMaxHistoryKeyLength = 80;
static const int g_cbMaxThemePayload = 512 * 1024;

static QString md3DefaultFontFamily()
{
    const QString strPreferred = QStringLiteral("Roboto Flex");
    return QFontDatabase::families().contains(strPreferred)
         ? strPreferred
         : QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
}

static bool md3IsValidFontWeight(int iWeight)
{
    switch (iWeight)
    {
        case -1:
        case QFont::Thin:
        case QFont::ExtraLight:
        case QFont::Light:
        case QFont::Normal:
        case QFont::Medium:
        case QFont::DemiBold:
        case QFont::Bold:
        case QFont::ExtraBold:
        case QFont::Black:
            return true;
        default:
            return false;
    }
}

static QString md3ColorString(const QColor &color)
{
    return color.isValid() ? color.name(QColor::HexArgb) : QString();
}

static bool md3ReadFiniteNumber(const QVariant &value, double &dValue)
{
    const int iType = value.typeId();
    if (iType != QMetaType::Double && iType != QMetaType::Float
        && iType != QMetaType::Int && iType != QMetaType::UInt
        && iType != QMetaType::LongLong && iType != QMetaType::ULongLong)
        return false;
    dValue = value.toDouble();
    return std::isfinite(dValue);
}

static bool md3ReadInteger(const QVariant &value, int &iValue)
{
    double dValue = 0.0;
    if (!md3ReadFiniteNumber(value, dValue)
        || dValue < static_cast<double>(std::numeric_limits<int>::min())
        || dValue > static_cast<double>(std::numeric_limits<int>::max())
        || std::floor(dValue) != dValue)
        return false;
    iValue = static_cast<int>(dValue);
    return true;
}

static bool md3IsValidAppearanceValue(const UIMd3Appearance &value)
{
    if (!value.fValid)
        return true;
    if (value.iRadius < UIMd3Shape::None || value.iRadius > UIMd3Shape::Full
        || !std::isfinite(value.dScale) || value.dScale < 0.50 || value.dScale > 2.00
        || !md3IsValidFontWeight(value.iWeight))
        return false;
    if (value.strFont.size() > g_iMaxHistoryKeyLength
        || (!value.strFont.isEmpty() && !QFontDatabase::families().contains(value.strFont)))
        return false;
    /* An invalid QColor is the explicit inherited-seed state; non-empty text is
     * rejected by the editor before it reaches this value object. */
    return true;
}

static bool md3ReadAppearanceMap(const QVariantMap &map, const QStringList &fontFamilies,
                                 UIMd3Appearance &value)
{
    const QVariant seedValue = map.value(QStringLiteral("seed"));
    const QVariant fontValue = map.value(QStringLiteral("font"));
    const QVariant radiusValue = map.value(QStringLiteral("radius"));
    const QVariant scaleValue = map.value(QStringLiteral("scale"));
    const QVariant weightValue = map.value(QStringLiteral("weight"));
    if (seedValue.typeId() != QMetaType::QString || fontValue.typeId() != QMetaType::QString
        || !radiusValue.isValid() || !scaleValue.isValid() || !weightValue.isValid())
        return false;

    int iRadius = 0;
    int iWeight = 0;
    double dScale = 0.0;
    if (!md3ReadInteger(radiusValue, iRadius) || !md3ReadFiniteNumber(scaleValue, dScale)
        || !md3ReadInteger(weightValue, iWeight))
        return false;

    const QString strSeed = seedValue.toString();
    const QString strFontRaw = fontValue.toString();
    const QString strFont = strFontRaw.trimmed();
    const QColor seed(strSeed);
    if (strFontRaw != strFont
        || (!strSeed.isEmpty() && !seed.isValid())
        || strFont.size() > g_iMaxHistoryKeyLength
        || (!strFont.isEmpty() && !fontFamilies.contains(strFont))
        || iRadius < UIMd3Shape::None || iRadius > UIMd3Shape::Full
        || !std::isfinite(dScale) || dScale < 0.50 || dScale > 2.00
        || !md3IsValidFontWeight(iWeight))
        return false;

    value.fValid = true;
    value.seed = seed;
    value.strFont = strFont;
    value.iRadius = iRadius;
    value.dScale = dScale;
    value.iWeight = iWeight;
    return true;
}

static bool md3ReadNamedThemeMap(const QVariantMap &map, const QStringList &fontFamilies,
                                 const QString &strFallbackBrand, QVariantMap &normalized,
                                 QHash<QString, UIMd3Appearance> *pAppearances = 0)
{
    const QVariant seedValue = map.value(QStringLiteral("seed"));
    const QVariant schemeValue = map.value(QStringLiteral("scheme"));
    const QVariant scaleValue = map.value(QStringLiteral("scale"));
    const QVariant compactValue = map.value(QStringLiteral("compact"));
    if (seedValue.typeId() != QMetaType::QString || !schemeValue.isValid()
        || !scaleValue.isValid() || compactValue.typeId() != QMetaType::Bool)
        return false;

    int iScheme = 0;
    double dScale = 0.0;
    if (!md3ReadInteger(schemeValue, iScheme) || !md3ReadFiniteNumber(scaleValue, dScale))
        return false;

    const QColor seed(seedValue.toString());
    const QString strFontRaw = map.contains(QStringLiteral("font"))
                             ? map.value(QStringLiteral("font")).toString()
                             : md3DefaultFontFamily();
    const QString strFont = strFontRaw.trimmed();
    int iWeight = -1;
    /* Keep the type check separate from the bounded integer conversion above;
     * malformed QVariant payloads must not be silently coerced. */
    if (map.contains(QStringLiteral("font"))
        && map.value(QStringLiteral("font")).typeId() != QMetaType::QString)
        return false;
    if (map.contains(QStringLiteral("weight")))
    {
        if (map.value(QStringLiteral("weight")).typeId() == QMetaType::Bool
            || !map.value(QStringLiteral("weight")).isValid()
            || !md3ReadInteger(map.value(QStringLiteral("weight")), iWeight))
            return false;
    }
    const QString strBrandRaw = map.contains(QStringLiteral("brand"))
                              ? map.value(QStringLiteral("brand")).toString()
                              : strFallbackBrand;
    const QString strBrand = strBrandRaw.trimmed();
    if ((map.contains(QStringLiteral("brand"))
         && map.value(QStringLiteral("brand")).typeId() != QMetaType::QString)
        || strFontRaw != strFont || strBrandRaw != strBrand
        || !seed.isValid()
        || iScheme < static_cast<int>(UIMd3Scheme_Dark)
        || iScheme > static_cast<int>(UIMd3Scheme_HighContrastLight)
        || !std::isfinite(dScale) || dScale < 0.75 || dScale > 2.0
        || strFont.isEmpty() || strFont.size() > g_iMaxHistoryKeyLength
        || !fontFamilies.contains(strFont) || !md3IsValidFontWeight(iWeight)
        || strBrand.isEmpty() || strBrand.size() > g_iMaxHistoryKeyLength)
        return false;

    QHash<QString, UIMd3Appearance> appearances;
    if (map.contains(QStringLiteral("appearances")))
    {
        const QVariant appearancesValue = map.value(QStringLiteral("appearances"));
        if (appearancesValue.typeId() != QMetaType::QVariantMap)
            return false;
        const QVariantMap appearanceMap = appearancesValue.toMap();
        if (appearanceMap.size() > g_iMaxHistoryAppearanceEntries)
            return false;
        for (QVariantMap::const_iterator it = appearanceMap.constBegin(); it != appearanceMap.constEnd(); ++it)
        {
            const QString strKey = it.key().trimmed();
            if (strKey != it.key() || strKey.isEmpty() || strKey.size() > g_iMaxHistoryKeyLength
                || appearances.contains(strKey) || it.value().typeId() != QMetaType::QVariantMap)
                return false;
            UIMd3Appearance value;
            if (!md3ReadAppearanceMap(it.value().toMap(), fontFamilies, value))
                return false;
            appearances.insert(strKey, value);
        }
    }

    normalized.clear();
    normalized.insert(QStringLiteral("seed"), md3ColorString(seed));
    normalized.insert(QStringLiteral("scheme"), iScheme);
    normalized.insert(QStringLiteral("scale"), dScale);
    normalized.insert(QStringLiteral("compact"), compactValue.toBool());
    normalized.insert(QStringLiteral("font"), strFont);
    normalized.insert(QStringLiteral("weight"), iWeight);
    normalized.insert(QStringLiteral("brand"), strBrand);
    if (map.contains(QStringLiteral("appearances")))
    {
        QVariantMap appearanceMap;
        for (QHash<QString, UIMd3Appearance>::const_iterator it = appearances.constBegin();
             it != appearances.constEnd(); ++it)
        {
            QVariantMap entry;
            entry.insert(QStringLiteral("seed"), md3ColorString(it.value().seed));
            entry.insert(QStringLiteral("font"), it.value().strFont);
            entry.insert(QStringLiteral("radius"), it.value().iRadius);
            entry.insert(QStringLiteral("scale"), it.value().dScale);
            entry.insert(QStringLiteral("weight"), it.value().iWeight);
            appearanceMap.insert(it.key(), entry);
        }
        normalized.insert(QStringLiteral("appearances"), appearanceMap);
    }
    if (pAppearances)
        *pAppearances = appearances;
    return true;
}

static QByteArray md3SerializeNamedThemes(const QHash<QString, QVariantMap> &themes)
{
    QJsonObject root;
    for (QHash<QString, QVariantMap>::const_iterator it = themes.constBegin();
         it != themes.constEnd(); ++it)
        root.insert(it.key(), QJsonObject::fromVariantMap(it.value()));
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    return data.size() <= g_cbMaxThemePayload ? data : QByteArray();
}

UIMd3Theme *UIMd3Theme::s_pInstance = 0;

UIMd3Theme *UIMd3Theme::instance() { return s_pInstance; }

void UIMd3Theme::create()
{
    if (s_pInstance)
        return;
    s_pInstance = new UIMd3Theme;
    s_pInstance->loadFromExtraData();
}

void UIMd3Theme::destroy()
{
    if (!s_pInstance)
        return;
    s_pInstance->saveToExtraData();
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3Theme::UIMd3Theme()
    : m_seed(QColor("#6750A4"))
    , m_enmScheme(UIMd3Scheme_Dark)
    , m_dFontScale(1.0)
    , m_fCompact(false)
    , m_strFontFamily(md3DefaultFontFamily())
    , m_iFontWeight(-1)
    , m_strBrandName("Material Virtual Machine")
    , m_fRestoring(false)
{
    regenerate();
}

UIMd3Theme::~UIMd3Theme()
{
}

QColor UIMd3Theme::color(UIMd3ColorRole enmRole) const
{
    if (enmRole < 0 || enmRole >= UIMd3ColorRole_Max)
        return QColor();
    return m_colors[enmRole];
}

QColor UIMd3Theme::stateLayer(UIMd3ColorRole enmRole, int iOpacityPercent) const
{
    QColor result = color(enmRole);
    result.setAlpha(qBound(0, iOpacityPercent, 100) * 255 / 100);
    return result;
}

void UIMd3Theme::setSeed(const QColor &seed)
{
    if (!seed.isValid() || seed == m_seed)
        return;
    const QColor previousSeed = m_seed;
    m_seed = seed;
    regenerate();
    if (serializeState().isEmpty())
    {
        m_seed = previousSeed;
        regenerate();
        return;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme seed changed"), md3ColorString(m_seed));
}

void UIMd3Theme::setScheme(UIMd3Scheme enmScheme)
{
    if (enmScheme < UIMd3Scheme_Dark || enmScheme > UIMd3Scheme_HighContrastLight
        || enmScheme == m_enmScheme)
        return;
    const UIMd3Scheme previousScheme = m_enmScheme;
    m_enmScheme = enmScheme;
    regenerate();
    if (serializeState().isEmpty())
    {
        m_enmScheme = previousScheme;
        regenerate();
        return;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme scheme changed"), QString::number(static_cast<int>(m_enmScheme)));
}

void UIMd3Theme::setFontScale(double dScale)
{
    if (!std::isfinite(dScale) || dScale < 0.75 || dScale > 2.0
        || qFuzzyCompare(dScale, m_dFontScale))
        return;
    const double previousScale = m_dFontScale;
    m_dFontScale = dScale;
    if (serializeState().isEmpty())
    {
        m_dFontScale = previousScale;
        return;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme font scale changed"), QString::number(m_dFontScale, 'f', 2));
}

void UIMd3Theme::setFontFamily(const QString &strFamily)
{
    const QString strTrimmed = strFamily.trimmed();
    if (strTrimmed.size() > g_iMaxHistoryKeyLength)
        return;
    const QString strEffective = strTrimmed.isEmpty() ? md3DefaultFontFamily() : strTrimmed;
    if (!QFontDatabase::families().contains(strEffective) || strEffective == m_strFontFamily)
        return;
    const QString previousFontFamily = m_strFontFamily;
    m_strFontFamily = strEffective;
    if (serializeState().isEmpty())
    {
        m_strFontFamily = previousFontFamily;
        return;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme font family changed"), m_strFontFamily);
}

void UIMd3Theme::setFontWeight(int iWeight)
{
    if (!md3IsValidFontWeight(iWeight) || iWeight == m_iFontWeight)
        return;
    const int previousFontWeight = m_iFontWeight;
    m_iFontWeight = iWeight;
    if (serializeState().isEmpty())
    {
        m_iFontWeight = previousFontWeight;
        return;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme font weight changed"), QString::number(m_iFontWeight));
}

void UIMd3Theme::setCompact(bool fCompact)
{
    if (fCompact == m_fCompact)
        return;
    const bool previousCompact = m_fCompact;
    m_fCompact = fCompact;
    if (serializeState().isEmpty())
    {
        m_fCompact = previousCompact;
        return;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme density changed"), m_fCompact ? QStringLiteral("compact")
                                                                      : QStringLiteral("comfortable"));
}

void UIMd3Theme::setBrandName(const QString &strName)
{
    const QString strTrimmed = strName.trimmed();
    const QString strEffective = strTrimmed.isEmpty() ? QStringLiteral("Material Virtual Machine") : strTrimmed.left(80);
    if (strEffective == m_strBrandName)
        return;
    const QString previousBrandName = m_strBrandName;
    m_strBrandName = strEffective;
    if (serializeState().isEmpty())
    {
        m_strBrandName = previousBrandName;
        return;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme brand changed"), m_strBrandName);
}

QFont UIMd3Theme::font(UIMd3TypeRole enmRole) const
{
    struct { int iSize; int iWeight; } aScale[UIMd3TypeRole_Max] =
    {
        { 57, QFont::Normal }, { 45, QFont::Normal }, { 36, QFont::Normal },
        { 32, QFont::Normal }, { 28, QFont::Normal }, { 24, QFont::Normal },
        { 22, QFont::Medium }, { 16, QFont::Medium }, { 14, QFont::Medium },
        { 16, QFont::Normal }, { 14, QFont::Normal }, { 12, QFont::Normal },
        { 14, QFont::Medium }, { 12, QFont::Medium }, { 11, QFont::Medium }
    };
    QFont result(m_strFontFamily);
    if (!QFontDatabase::families().contains(m_strFontFamily))
        result = QFont("Segoe UI");
    const int iIndex = qBound(0, (int)enmRole, (int)UIMd3TypeRole_Max - 1);
    result.setPixelSize(qMax(9, (int)(aScale[iIndex].iSize * m_dFontScale * (m_fCompact ? 0.92 : 1.0))));
    result.setWeight((QFont::Weight)(m_iFontWeight >= 0 ? m_iFontWeight : aScale[iIndex].iWeight));
    return result;
}

UIMd3Appearance UIMd3Theme::appearance(const QString &strKey) const
{
    return m_appearances.value(strKey, UIMd3Appearance());
}

bool UIMd3Theme::setAppearance(const QString &strKey, const UIMd3Appearance &appearance)
{
    const QString strTrimmedKey = strKey.trimmed();
    if (strTrimmedKey.isEmpty() || strTrimmedKey.size() > g_iMaxHistoryKeyLength
        || (!m_appearances.contains(strTrimmedKey)
            && m_appearances.size() >= g_iMaxHistoryAppearanceEntries))
        return false;
    if (!appearance.fValid)
    {
        clearAppearance(strTrimmedKey);
        return true;
    }

    if (!md3IsValidAppearanceValue(appearance))
        return false;
    const bool fHadAppearance = m_appearances.contains(strTrimmedKey);
    const UIMd3Appearance previousAppearance = m_appearances.value(strTrimmedKey);
    m_appearances[strTrimmedKey] = appearance;
    if (serializeState().isEmpty())
    {
        if (fHadAppearance)
            m_appearances[strTrimmedKey] = previousAppearance;
        else
            m_appearances.remove(strTrimmedKey);
        return false;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme appearance changed"), strTrimmedKey);
    return true;
}

void UIMd3Theme::clearAppearance(const QString &strKey)
{
    if (m_appearances.remove(strKey.trimmed()))
    {
        saveToExtraData();
        emit sigThemeChanged();
        recordHistory(QStringLiteral("theme appearance reset"), strKey);
    }
}

bool UIMd3Theme::saveNamedTheme(const QString &strName)
{
    const QString strTrimmedName = strName.trimmed();
    if (strTrimmedName.isEmpty() || strTrimmedName.size() > g_iMaxHistoryKeyLength
        || (!m_namedThemes.contains(strTrimmedName)
            && m_namedThemes.size() >= g_iMaxHistoryAppearanceEntries))
        return false;
    QVariantMap map;
    map["seed"]    = md3ColorString(m_seed);
    map["scheme"]  = static_cast<int>(m_enmScheme);
    map["scale"]   = m_dFontScale;
    map["compact"] = m_fCompact;
    map["font"]    = m_strFontFamily;
    map["weight"]  = m_iFontWeight;
    map["brand"]   = m_strBrandName;
    QVariantMap appearanceMap;
    for (QHash<QString, UIMd3Appearance>::const_iterator it = m_appearances.constBegin();
         it != m_appearances.constEnd(); ++it)
    {
        if (!md3IsValidAppearanceValue(it.value()))
            return false;
        QVariantMap entry;
        entry["seed"]   = md3ColorString(it.value().seed);
        entry["font"]   = it.value().strFont;
        entry["radius"] = it.value().iRadius;
        entry["scale"]  = it.value().dScale;
        entry["weight"] = it.value().iWeight;
        appearanceMap.insert(it.key(), entry);
    }
    map["appearances"] = appearanceMap;
    QHash<QString, QVariantMap> savedThemes = m_namedThemes;
    savedThemes[strTrimmedName] = map;
    if (md3SerializeNamedThemes(savedThemes).isEmpty())
        return false;
    const QHash<QString, QVariantMap> previousThemes = m_namedThemes;
    m_namedThemes = savedThemes;
    if (serializeState().isEmpty())
    {
        m_namedThemes = previousThemes;
        return false;
    }
    saveToExtraData();
    recordHistory(QStringLiteral("theme named preset saved"), strTrimmedName);
    return true;
}

bool UIMd3Theme::applyNamedTheme(const QString &strName)
{
    const QString strTrimmedName = strName.trimmed();
    if (!m_namedThemes.contains(strTrimmedName))
        return false;
    const QVariantMap map = m_namedThemes.value(strTrimmedName);
    QVariantMap normalized;
    QHash<QString, UIMd3Appearance> appearances;
    if (!md3ReadNamedThemeMap(map, QFontDatabase::families(), m_strBrandName, normalized, &appearances))
        return false;

    /* Validation above completes before any live state is touched. */
    const QColor previousSeed = m_seed;
    const UIMd3Scheme previousScheme = m_enmScheme;
    const double previousScale = m_dFontScale;
    const bool previousCompact = m_fCompact;
    const QString previousFontFamily = m_strFontFamily;
    const int previousFontWeight = m_iFontWeight;
    const QString previousBrandName = m_strBrandName;
    const QHash<QString, UIMd3Appearance> previousAppearances = m_appearances;
    m_seed = QColor(normalized.value(QStringLiteral("seed")).toString());
    m_enmScheme = static_cast<UIMd3Scheme>(normalized.value(QStringLiteral("scheme")).toInt());
    m_dFontScale = normalized.value(QStringLiteral("scale")).toDouble();
    m_fCompact = normalized.value(QStringLiteral("compact")).toBool();
    m_strFontFamily = normalized.value(QStringLiteral("font")).toString();
    m_iFontWeight = normalized.value(QStringLiteral("weight")).toInt();
    m_strBrandName = normalized.value(QStringLiteral("brand")).toString();
    /* A named theme is a complete appearance snapshot.  Older records without
     * an element map therefore mean an explicit empty override set, rather than
     * silently retaining overrides from the previously active theme. */
    m_appearances = normalized.contains(QStringLiteral("appearances"))
                  ? appearances : QHash<QString, UIMd3Appearance>();
    if (serializeState().isEmpty())
    {
        m_seed = previousSeed;
        m_enmScheme = previousScheme;
        m_dFontScale = previousScale;
        m_fCompact = previousCompact;
        m_strFontFamily = previousFontFamily;
        m_iFontWeight = previousFontWeight;
        m_strBrandName = previousBrandName;
        m_appearances = previousAppearances;
        return false;
    }
    regenerate();
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme named preset applied"), strTrimmedName);
    return true;
}

QByteArray UIMd3Theme::exportNamedThemes() const
{
    return md3SerializeNamedThemes(m_namedThemes);
}

bool UIMd3Theme::importNamedThemes(const QByteArray &data)
{
    if (data.isEmpty() || data.size() > g_cbMaxThemePayload)
        return false;
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    const QJsonObject root = doc.object();
    if (root.size() > g_iMaxHistoryAppearanceEntries)
        return false;

    const QStringList fontFamilies = QFontDatabase::families();
    QHash<QString, QVariantMap> imported;
    for (QJsonObject::const_iterator it = root.begin(); it != root.end(); ++it)
    {
        const QString strName = it.key().trimmed();
        if (strName != it.key() || strName.isEmpty() || strName.size() > g_iMaxHistoryKeyLength
            || imported.contains(strName) || !it.value().isObject())
            return false;
        QVariantMap normalized;
        if (!md3ReadNamedThemeMap(it.value().toObject().toVariantMap(), fontFamilies,
                                  m_strBrandName, normalized))
            return false;
        imported.insert(strName, normalized);
    }

    QHash<QString, QVariantMap> merged = m_namedThemes;
    for (QHash<QString, QVariantMap>::const_iterator it = imported.constBegin();
         it != imported.constEnd(); ++it)
    {
        if (!merged.contains(it.key()) && merged.size() >= g_iMaxHistoryAppearanceEntries)
            return false;
        merged.insert(it.key(), it.value());
    }
    if (imported.isEmpty())
        return true;

    if (md3SerializeNamedThemes(merged).isEmpty())
        return false;
    const QHash<QString, QVariantMap> previousThemes = m_namedThemes;
    m_namedThemes = merged;
    if (serializeState().isEmpty())
    {
        m_namedThemes = previousThemes;
        return false;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme named preset imported"), QString::number(imported.size()));
    return true;
}

QByteArray UIMd3Theme::serializeState() const
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), g_iThemeHistorySchema);
    root.insert(QStringLiteral("seed"), md3ColorString(m_seed));
    root.insert(QStringLiteral("scheme"), static_cast<int>(m_enmScheme));
    root.insert(QStringLiteral("scale"), m_dFontScale);
    root.insert(QStringLiteral("compact"), m_fCompact);
    root.insert(QStringLiteral("font"), m_strFontFamily);
    root.insert(QStringLiteral("weight"), m_iFontWeight);
    root.insert(QStringLiteral("brand"), m_strBrandName);

    QJsonObject appearances;
    QStringList keys = m_appearances.keys();
    keys.sort(Qt::CaseSensitive);
    int cEntries = 0;
    for (const QString &strKey : keys)
    {
        if (cEntries >= g_iMaxHistoryAppearanceEntries)
            break;
        const QString strBoundedKey = strKey.trimmed().left(g_iMaxHistoryKeyLength);
        const UIMd3Appearance value = m_appearances.value(strKey);
        if (strBoundedKey.isEmpty() || !value.fValid)
            continue;
        QJsonObject entry;
        entry.insert(QStringLiteral("seed"), md3ColorString(value.seed));
        entry.insert(QStringLiteral("font"), value.strFont.left(g_iMaxHistoryKeyLength));
        entry.insert(QStringLiteral("radius"), value.iRadius);
        entry.insert(QStringLiteral("scale"), value.dScale);
        entry.insert(QStringLiteral("weight"), value.iWeight);
        appearances.insert(strBoundedKey, entry);
        ++cEntries;
    }
    root.insert(QStringLiteral("appearances"), appearances);

    const QByteArray namedThemeData = exportNamedThemes();
    if (!m_namedThemes.isEmpty() && namedThemeData.isEmpty())
        return QByteArray();
    const QJsonDocument namedThemes = QJsonDocument::fromJson(namedThemeData);
    if (namedThemes.isObject())
        root.insert(QStringLiteral("namedThemes"), namedThemes.object());

    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    return data.size() <= g_cbMaxThemePayload ? data : QByteArray();
}

bool UIMd3Theme::restoreState(const QByteArray &data)
{
    if (data.isEmpty() || data.size() > g_cbMaxThemePayload)
        return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;
    const QJsonObject root = document.object();
    const QJsonValue versionValue = root.value(QStringLiteral("version"));
    if (!versionValue.isDouble() || !std::isfinite(versionValue.toDouble())
        || versionValue.toDouble() != g_iThemeHistorySchema)
        return false;
    const QStringList fontFamilies = QFontDatabase::families();
    const QJsonValue appearancesValue = root.value(QStringLiteral("appearances"));
    const QJsonValue namedThemesValue = root.value(QStringLiteral("namedThemes"));
    if (!appearancesValue.isObject() || !namedThemesValue.isObject())
        return false;

    QVariantMap normalized;
    QHash<QString, UIMd3Appearance> appearances;
    if (!md3ReadNamedThemeMap(root.toVariantMap(), fontFamilies, m_strBrandName,
                              normalized, &appearances))
        return false;

    const QJsonObject namedThemeObject = namedThemesValue.toObject();
    if (namedThemeObject.size() > g_iMaxHistoryAppearanceEntries)
        return false;
    QHash<QString, QVariantMap> namedThemes;
    for (QJsonObject::const_iterator it = namedThemeObject.begin(); it != namedThemeObject.end(); ++it)
    {
        const QString strName = it.key().trimmed();
        if (strName != it.key() || strName.isEmpty() || strName.size() > g_iMaxHistoryKeyLength
            || namedThemes.contains(strName) || !it.value().isObject())
            return false;
        QVariantMap theme;
        if (!md3ReadNamedThemeMap(it.value().toObject().toVariantMap(), fontFamilies,
                                  normalized.value(QStringLiteral("brand")).toString(), theme))
            return false;
        namedThemes.insert(strName, theme);
    }

    m_fRestoring = true;
    m_seed = QColor(normalized.value(QStringLiteral("seed")).toString());
    m_enmScheme = static_cast<UIMd3Scheme>(normalized.value(QStringLiteral("scheme")).toInt());
    m_dFontScale = normalized.value(QStringLiteral("scale")).toDouble();
    m_fCompact = normalized.value(QStringLiteral("compact")).toBool();
    m_strFontFamily = normalized.value(QStringLiteral("font")).toString();
    m_iFontWeight = normalized.value(QStringLiteral("weight")).toInt();
    m_strBrandName = normalized.value(QStringLiteral("brand")).toString();
    m_appearances = appearances;
    m_namedThemes = namedThemes;
    regenerate();
    saveToExtraData();
    emit sigThemeChanged();
    m_fRestoring = false;
    return true;
}

void UIMd3Theme::recordHistory(const QString &strAction, const QString &strDetail)
{
    if (!m_fRestoring && UIMd3History::instance())
    {
        const QByteArray state = serializeState();
        if (!state.isEmpty())
            UIMd3History::instance()->record(strAction, strDetail, state);
    }
}

void UIMd3Theme::sltRestoreHistoryRevision(const QString &strRevisionId)
{
    UIMd3History *pHistory = UIMd3History::instance();
    if (!pHistory)
        return;
    for (const UIMd3HistoryRevision &revision : pHistory->revisions())
        if (revision.strId == strRevisionId && revision.strAction.startsWith(QStringLiteral("theme ")))
        {
            const bool fRestored = restoreState(revision.state);
            if (fRestored)
                recordHistory(QStringLiteral("theme revision restored"), strRevisionId);
            emit pHistory->sigRevisionRestoreCompleted(strRevisionId, fRestored);
            return;
        }
}

void UIMd3Theme::loadFromExtraData()
{
    const QString strSeed = gEDataManager->extraDataString(g_pszKeySeed);
    if (!strSeed.isEmpty() && QColor(strSeed).isValid())
        m_seed = QColor(strSeed);
    m_enmScheme     = static_cast<UIMd3Scheme>(qBound(static_cast<int>(UIMd3Scheme_Dark),
                                                       gEDataManager->extraDataString(g_pszKeyScheme).toInt(),
                                                       static_cast<int>(UIMd3Scheme_HighContrastLight)));
    bool fScaleOk = false;
    const double dStoredScale = gEDataManager->extraDataString(g_pszKeyScale).toDouble(&fScaleOk);
    m_dFontScale    = qBound(0.75, fScaleOk ? dStoredScale : 1.0, 2.0);
    m_fCompact      = gEDataManager->extraDataString(g_pszKeyCompact) == "true";
    const QString strFont = gEDataManager->extraDataString(g_pszKeyFont).trimmed();
    if (!strFont.isEmpty() && strFont.size() <= g_iMaxHistoryKeyLength
        && QFontDatabase::families().contains(strFont))
        m_strFontFamily = strFont;
    bool fWeightOk = false;
    const int iStoredWeight = gEDataManager->extraDataString(g_pszKeyWeight).toInt(&fWeightOk);
    if (fWeightOk && md3IsValidFontWeight(iStoredWeight))
        m_iFontWeight = iStoredWeight;
    const QString strBrand = gEDataManager->extraDataString(g_pszKeyBrand).trimmed();
    if (!strBrand.isEmpty())
        m_strBrandName = strBrand.left(80);

    /* Per-element overrides: */
    const QByteArray appearanceData = gEDataManager->extraDataString(g_pszKeyAppearance).toUtf8();
    QJsonParseError appearanceError;
    const QJsonDocument docAppearance = QJsonDocument::fromJson(appearanceData, &appearanceError);
    if (appearanceError.error == QJsonParseError::NoError && docAppearance.isObject())
    {
        const QJsonObject objAppearance = docAppearance.object();
        QHash<QString, UIMd3Appearance> parsedAppearances;
        bool fValidAppearances = objAppearance.size() <= g_iMaxHistoryAppearanceEntries;
        for (QJsonObject::const_iterator it = objAppearance.begin();
             fValidAppearances && it != objAppearance.end(); ++it)
        {
            const QString strKey = it.key().trimmed();
            if (strKey != it.key() || strKey.isEmpty() || strKey.size() > g_iMaxHistoryKeyLength
                || parsedAppearances.contains(strKey) || !it.value().isObject())
            {
                fValidAppearances = false;
                break;
            }
            UIMd3Appearance appearance;
            if (!md3ReadAppearanceMap(it.value().toObject().toVariantMap(),
                                       QFontDatabase::families(), appearance))
            {
                fValidAppearances = false;
                break;
            }
            parsedAppearances.insert(strKey, appearance);
        }
        if (fValidAppearances)
            m_appearances = parsedAppearances;
    }

    /* Named themes: keep the previous validated set when the stored payload is
     * malformed, so a later save cannot erase usable presets. */
    importNamedThemes(gEDataManager->extraDataString(g_pszKeyThemes).toUtf8());

    regenerate();
    emit sigThemeChanged();
}

void UIMd3Theme::saveToExtraData() const
{
    gEDataManager->setExtraDataString(g_pszKeySeed, md3ColorString(m_seed));
    gEDataManager->setExtraDataString(g_pszKeyScheme, QString::number((int)m_enmScheme));
    gEDataManager->setExtraDataString(g_pszKeyScale, QString::number(m_dFontScale));
    gEDataManager->setExtraDataString(g_pszKeyCompact, m_fCompact ? "true" : "false");
    gEDataManager->setExtraDataString(g_pszKeyFont, m_strFontFamily);
    gEDataManager->setExtraDataString(g_pszKeyWeight, QString::number(m_iFontWeight));
    gEDataManager->setExtraDataString(g_pszKeyBrand, m_strBrandName);

    QJsonObject objAppearance;
    for (QHash<QString, UIMd3Appearance>::const_iterator it = m_appearances.begin(); it != m_appearances.end(); ++it)
    {
        QJsonObject entry;
        entry.insert("seed",   md3ColorString(it.value().seed));
        entry.insert("font",   it.value().strFont);
        entry.insert("radius", it.value().iRadius);
        entry.insert("scale",  it.value().dScale);
        entry.insert("weight", it.value().iWeight);
        objAppearance.insert(it.key(), entry);
    }
    gEDataManager->setExtraDataString(g_pszKeyAppearance, QString::fromUtf8(QJsonDocument(objAppearance).toJson(QJsonDocument::Compact)));
    const QByteArray namedThemes = exportNamedThemes();
    if (!namedThemes.isEmpty())
        gEDataManager->setExtraDataString(g_pszKeyThemes, QString::fromUtf8(namedThemes));
}

UIMd3Scheme UIMd3Theme::effectiveScheme() const
{
    if (m_enmScheme != UIMd3Scheme_System)
        return m_enmScheme;
    if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Light)
        return UIMd3Scheme_Light;
    return UIMd3Scheme_Dark;
}

QColor UIMd3Theme::paletteTone(double dHue, double dChroma, int iTone)
{
    int iRed = 0, iGreen = 0, iBlue = 0;
    md3HctToRgb(dHue, dChroma, (double)iTone, &iRed, &iGreen, &iBlue);
    return QColor(iRed, iGreen, iBlue);
}

void UIMd3Theme::regenerate()
{
    const bool fDark = effectiveScheme() == UIMd3Scheme_Dark || effectiveScheme() == UIMd3Scheme_HighContrastDark;
    const bool fContrast = effectiveScheme() == UIMd3Scheme_HighContrastDark || effectiveScheme() == UIMd3Scheme_HighContrastLight;

    /* Derive the Material 3 core palettes from the seed. Every palette keeps the seed
     * hue and fixes its own chroma, so the whole scheme stays a single colour family
     * while each role keeps its intended amount of colour. The tertiary palette is the
     * one deliberate exception: it is rotated a fixed 60 degrees to give the scheme an
     * accent that is related to the seed rather than a shade of it. Tones are CIE L*,
     * so the tone numbers below are contrast decisions, not brightness guesses. */
    const QColor seedColor = m_seed.toRgb();
    const UIMd3Hct seedHct = md3HctFromRgb(seedColor.red(), seedColor.green(), seedColor.blue());
    const double dHue = seedHct.dHue;
    const double dPrimaryChroma = qMax(48.0, seedHct.dChroma);
    const double dSecondaryChroma = 16.0;
    const double dTertiaryChroma = 24.0;
    const double dTertiaryHue = dHue + 60.0;
    const double dNeutralChroma = 6.0;
    const double dNeutralVariantChroma = 8.0;
    /* The error family is fixed rather than seed-derived: a destructive action has to
     * read as dangerous whatever the user themed the rest of the application to. */
    const double dErrorHue = 25.0;
    const double dErrorChroma = 84.0;

    if (fDark)
    {
        m_colors[UIMd3ColorRole_Primary]                  = paletteTone(dHue, dPrimaryChroma, fContrast ? 90 : 80);
        m_colors[UIMd3ColorRole_OnPrimary]                = paletteTone(dHue, dPrimaryChroma, 20);
        m_colors[UIMd3ColorRole_PrimaryContainer]         = paletteTone(dHue, dPrimaryChroma, 30);
        m_colors[UIMd3ColorRole_OnPrimaryContainer]       = paletteTone(dHue, dPrimaryChroma, 90);
        m_colors[UIMd3ColorRole_Secondary]                = paletteTone(dHue, dSecondaryChroma, 80);
        m_colors[UIMd3ColorRole_OnSecondary]              = paletteTone(dHue, dSecondaryChroma, 20);
        m_colors[UIMd3ColorRole_SecondaryContainer]       = paletteTone(dHue, dSecondaryChroma, 30);
        m_colors[UIMd3ColorRole_OnSecondaryContainer]     = paletteTone(dHue, dSecondaryChroma, 90);
        m_colors[UIMd3ColorRole_Tertiary]                 = paletteTone(dTertiaryHue, dTertiaryChroma, 80);
        m_colors[UIMd3ColorRole_OnTertiary]               = paletteTone(dTertiaryHue, dTertiaryChroma, 20);
        m_colors[UIMd3ColorRole_TertiaryContainer]        = paletteTone(dTertiaryHue, dTertiaryChroma, 30);
        m_colors[UIMd3ColorRole_OnTertiaryContainer]      = paletteTone(dTertiaryHue, dTertiaryChroma, 90);
        m_colors[UIMd3ColorRole_Error]                    = paletteTone(dErrorHue, dErrorChroma, 80);
        m_colors[UIMd3ColorRole_OnError]                  = paletteTone(dErrorHue, dErrorChroma, 20);
        m_colors[UIMd3ColorRole_ErrorContainer]           = paletteTone(dErrorHue, dErrorChroma, 30);
        m_colors[UIMd3ColorRole_OnErrorContainer]         = paletteTone(dErrorHue, dErrorChroma, 90);
        m_colors[UIMd3ColorRole_Surface]                  = paletteTone(dHue, dNeutralChroma, 6);
        m_colors[UIMd3ColorRole_OnSurface]                = paletteTone(dHue, dNeutralChroma, fContrast ? 100 : 90);
        m_colors[UIMd3ColorRole_OnSurfaceVariant]         = paletteTone(dHue, dNeutralVariantChroma, fContrast ? 90 : 80);
        m_colors[UIMd3ColorRole_SurfaceContainerLowest]   = paletteTone(dHue, dNeutralChroma, 4);
        m_colors[UIMd3ColorRole_SurfaceContainerLow]      = paletteTone(dHue, dNeutralChroma, 10);
        m_colors[UIMd3ColorRole_SurfaceContainer]         = paletteTone(dHue, dNeutralChroma, 12);
        m_colors[UIMd3ColorRole_SurfaceContainerHigh]     = paletteTone(dHue, dNeutralChroma, 17);
        m_colors[UIMd3ColorRole_SurfaceContainerHighest]  = paletteTone(dHue, dNeutralChroma, 22);
        m_colors[UIMd3ColorRole_Outline]                  = paletteTone(dHue, dNeutralVariantChroma, fContrast ? 80 : 60);
        m_colors[UIMd3ColorRole_OutlineVariant]           = paletteTone(dHue, dNeutralVariantChroma, fContrast ? 60 : 30);
    }
    else
    {
        m_colors[UIMd3ColorRole_Primary]                  = paletteTone(dHue, dPrimaryChroma, fContrast ? 30 : 40);
        m_colors[UIMd3ColorRole_OnPrimary]                = paletteTone(dHue, dPrimaryChroma, 100);
        m_colors[UIMd3ColorRole_PrimaryContainer]         = paletteTone(dHue, dPrimaryChroma, 90);
        m_colors[UIMd3ColorRole_OnPrimaryContainer]       = paletteTone(dHue, dPrimaryChroma, 10);
        m_colors[UIMd3ColorRole_Secondary]                = paletteTone(dHue, dSecondaryChroma, 40);
        m_colors[UIMd3ColorRole_OnSecondary]              = paletteTone(dHue, dSecondaryChroma, 100);
        m_colors[UIMd3ColorRole_SecondaryContainer]       = paletteTone(dHue, dSecondaryChroma, 90);
        m_colors[UIMd3ColorRole_OnSecondaryContainer]     = paletteTone(dHue, dSecondaryChroma, 10);
        m_colors[UIMd3ColorRole_Tertiary]                 = paletteTone(dTertiaryHue, dTertiaryChroma, 40);
        m_colors[UIMd3ColorRole_OnTertiary]               = paletteTone(dTertiaryHue, dTertiaryChroma, 100);
        m_colors[UIMd3ColorRole_TertiaryContainer]        = paletteTone(dTertiaryHue, dTertiaryChroma, 90);
        m_colors[UIMd3ColorRole_OnTertiaryContainer]      = paletteTone(dTertiaryHue, dTertiaryChroma, 10);
        m_colors[UIMd3ColorRole_Error]                    = paletteTone(dErrorHue, dErrorChroma, 40);
        m_colors[UIMd3ColorRole_OnError]                  = paletteTone(dErrorHue, dErrorChroma, 100);
        m_colors[UIMd3ColorRole_ErrorContainer]           = paletteTone(dErrorHue, dErrorChroma, 90);
        m_colors[UIMd3ColorRole_OnErrorContainer]         = paletteTone(dErrorHue, dErrorChroma, 10);
        m_colors[UIMd3ColorRole_Surface]                  = paletteTone(dHue, dNeutralChroma, 98);
        m_colors[UIMd3ColorRole_OnSurface]                = paletteTone(dHue, dNeutralChroma, fContrast ? 0 : 10);
        m_colors[UIMd3ColorRole_OnSurfaceVariant]         = paletteTone(dHue, dNeutralVariantChroma, fContrast ? 10 : 30);
        m_colors[UIMd3ColorRole_SurfaceContainerLowest]   = paletteTone(dHue, dNeutralChroma, 100);
        m_colors[UIMd3ColorRole_SurfaceContainerLow]      = paletteTone(dHue, dNeutralChroma, 96);
        m_colors[UIMd3ColorRole_SurfaceContainer]         = paletteTone(dHue, dNeutralChroma, 94);
        m_colors[UIMd3ColorRole_SurfaceContainerHigh]     = paletteTone(dHue, dNeutralChroma, 92);
        m_colors[UIMd3ColorRole_SurfaceContainerHighest]  = paletteTone(dHue, dNeutralChroma, 90);
        m_colors[UIMd3ColorRole_Outline]                  = paletteTone(dHue, dNeutralVariantChroma, fContrast ? 30 : 50);
        m_colors[UIMd3ColorRole_OutlineVariant]           = paletteTone(dHue, dNeutralVariantChroma, fContrast ? 50 : 80);
    }
    m_colors[UIMd3ColorRole_Scrim] = QColor(0, 0, 0, 153);

    /* Publish an approximate QPalette so any not-yet-migrated widget still reads correctly: */
    QPalette palette = qApp->palette();
    palette.setColor(QPalette::Window,          m_colors[UIMd3ColorRole_Surface]);
    palette.setColor(QPalette::WindowText,      m_colors[UIMd3ColorRole_OnSurface]);
    palette.setColor(QPalette::Base,            m_colors[UIMd3ColorRole_SurfaceContainerLow]);
    palette.setColor(QPalette::AlternateBase,   m_colors[UIMd3ColorRole_SurfaceContainer]);
    palette.setColor(QPalette::Text,            m_colors[UIMd3ColorRole_OnSurface]);
    palette.setColor(QPalette::Button,          m_colors[UIMd3ColorRole_SurfaceContainerHigh]);
    palette.setColor(QPalette::ButtonText,      m_colors[UIMd3ColorRole_OnSurface]);
    palette.setColor(QPalette::Highlight,       m_colors[UIMd3ColorRole_SecondaryContainer]);
    palette.setColor(QPalette::HighlightedText, m_colors[UIMd3ColorRole_OnSecondaryContainer]);
    palette.setColor(QPalette::ToolTipBase,     m_colors[UIMd3ColorRole_SurfaceContainerHighest]);
    palette.setColor(QPalette::ToolTipText,     m_colors[UIMd3ColorRole_OnSurface]);
    qApp->setPalette(palette);
}
