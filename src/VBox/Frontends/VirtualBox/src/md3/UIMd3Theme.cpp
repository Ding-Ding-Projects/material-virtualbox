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
#include <QPalette>
#include <QStyleHints>
#include <QVariantMap>

/* GUI includes: */
#include "UIExtraDataManager.h"
#include "UIMd3History.h"
#include "UIMd3Theme.h"

/* Extradata keys owned by the Material 3 shell. */
static const char *g_pszKeySeed       = "GUI/Md3/Seed";
static const char *g_pszKeyScheme     = "GUI/Md3/Scheme";
static const char *g_pszKeyScale      = "GUI/Md3/FontScale";
static const char *g_pszKeyCompact    = "GUI/Md3/Compact";
static const char *g_pszKeyFont       = "GUI/Md3/FontFamily";
static const char *g_pszKeyBrand      = "GUI/Md3/BrandName";
static const char *g_pszKeyAppearance = "GUI/Md3/Appearance";
static const char *g_pszKeyThemes     = "GUI/Md3/NamedThemes";
static const int g_iThemeHistorySchema = 1;
static const int g_iMaxHistoryAppearanceEntries = 256;
static const int g_iMaxHistoryKeyLength = 80;

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
    , m_strFontFamily("Roboto Flex")
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
    m_seed = seed;
    regenerate();
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme seed changed"), m_seed.name());
}

void UIMd3Theme::setScheme(UIMd3Scheme enmScheme)
{
    if (enmScheme == m_enmScheme)
        return;
    m_enmScheme = enmScheme;
    regenerate();
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme scheme changed"), QString::number(static_cast<int>(m_enmScheme)));
}

void UIMd3Theme::setFontScale(double dScale)
{
    const double dClamped = qBound(0.75, dScale, 2.0);
    if (qFuzzyCompare(dClamped, m_dFontScale))
        return;
    m_dFontScale = dClamped;
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme font scale changed"), QString::number(m_dFontScale, 'f', 2));
}

void UIMd3Theme::setCompact(bool fCompact)
{
    if (fCompact == m_fCompact)
        return;
    m_fCompact = fCompact;
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
    m_strBrandName = strEffective;
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
    result.setWeight((QFont::Weight)aScale[iIndex].iWeight);
    return result;
}

UIMd3Appearance UIMd3Theme::appearance(const QString &strKey) const
{
    return m_appearances.value(strKey, UIMd3Appearance());
}

void UIMd3Theme::setAppearance(const QString &strKey, const UIMd3Appearance &appearance)
{
    const QString strTrimmedKey = strKey.trimmed();
    if (strTrimmedKey.isEmpty())
        return;
    if (!appearance.fValid)
    {
        clearAppearance(strTrimmedKey);
        return;
    }

    UIMd3Appearance sanitized = appearance;
    sanitized.iRadius = qBound(0, sanitized.iRadius, static_cast<int>(UIMd3Shape::Full));
    sanitized.dScale = qBound(0.50, sanitized.dScale, 2.00);
    if (sanitized.iWeight < 0)
        sanitized.iWeight = -1;
    else
        sanitized.iWeight = qBound(static_cast<int>(QFont::Thin), sanitized.iWeight,
                                   static_cast<int>(QFont::Black));
    if (!sanitized.strFont.isEmpty() && !QFontDatabase::families().contains(sanitized.strFont))
        sanitized.strFont.clear();

    m_appearances[strTrimmedKey] = sanitized;
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme appearance changed"), strTrimmedKey);
}

void UIMd3Theme::clearAppearance(const QString &strKey)
{
    if (m_appearances.remove(strKey))
    {
        saveToExtraData();
        emit sigThemeChanged();
        recordHistory(QStringLiteral("theme appearance reset"), strKey);
    }
}

void UIMd3Theme::saveNamedTheme(const QString &strName)
{
    const QString strTrimmedName = strName.trimmed().left(80);
    if (strTrimmedName.isEmpty())
        return;
    QVariantMap map;
    map["seed"]    = m_seed.name();
    map["scheme"]  = (int)m_enmScheme;
    map["scale"]   = m_dFontScale;
    map["compact"] = m_fCompact;
    map["font"]    = m_strFontFamily;
    m_namedThemes[strTrimmedName] = map;
    saveToExtraData();
    recordHistory(QStringLiteral("theme named preset saved"), strTrimmedName);
}

bool UIMd3Theme::applyNamedTheme(const QString &strName)
{
    if (!m_namedThemes.contains(strName))
        return false;
    const QVariantMap map = m_namedThemes.value(strName);
    m_seed        = QColor(map.value("seed", "#6750A4").toString());
    m_enmScheme   = (UIMd3Scheme)map.value("scheme", (int)UIMd3Scheme_Dark).toInt();
    m_dFontScale  = map.value("scale", 1.0).toDouble();
    m_fCompact    = map.value("compact", false).toBool();
    m_strFontFamily = map.value("font", "Roboto Flex").toString();
    regenerate();
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme named preset applied"), strName);
    return true;
}

QByteArray UIMd3Theme::exportNamedThemes() const
{
    QJsonObject root;
    for (QHash<QString, QVariantMap>::const_iterator it = m_namedThemes.begin(); it != m_namedThemes.end(); ++it)
        root.insert(it.key(), QJsonObject::fromVariantMap(it.value()));
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool UIMd3Theme::importNamedThemes(const QByteArray &data)
{
    if (data.isEmpty() || data.size() > 512 * 1024)
        return false;
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    const QJsonObject root = doc.object();
    const QStringList fontFamilies = QFontDatabase::families();
    int cImported = 0;
    for (QJsonObject::const_iterator it = root.begin(); it != root.end(); ++it)
    {
        if (cImported >= g_iMaxHistoryAppearanceEntries)
            break;
        const QString strName = it.key().trimmed().left(g_iMaxHistoryKeyLength);
        const QJsonObject entry = it.value().toObject();
        const QColor themeSeed(entry.value(QStringLiteral("seed")).toString());
        const int iThemeScheme = entry.value(QStringLiteral("scheme")).toInt(-1);
        const double dThemeScale = entry.value(QStringLiteral("scale")).toDouble(-1.0);
        const QString strThemeFont = entry.value(QStringLiteral("font")).toString()
                                           .trimmed().left(g_iMaxHistoryKeyLength);
        if (strName.isEmpty() || !themeSeed.isValid()
            || iThemeScheme < static_cast<int>(UIMd3Scheme_Dark)
            || iThemeScheme > static_cast<int>(UIMd3Scheme_HighContrastLight)
            || dThemeScale < 0.75 || dThemeScale > 2.0
            || !entry.value(QStringLiteral("compact")).isBool()
            || (!strThemeFont.isEmpty() && !fontFamilies.contains(strThemeFont)))
            continue;
        QVariantMap map;
        map.insert(QStringLiteral("seed"), themeSeed.name());
        map.insert(QStringLiteral("scheme"), iThemeScheme);
        map.insert(QStringLiteral("scale"), dThemeScale);
        map.insert(QStringLiteral("compact"), entry.value(QStringLiteral("compact")).toBool());
        map.insert(QStringLiteral("font"), strThemeFont);
        m_namedThemes[strName] = map;
        ++cImported;
    }
    saveToExtraData();
    emit sigThemeChanged();
    recordHistory(QStringLiteral("theme named preset imported"), QString::number(cImported));
    return true;
}

QByteArray UIMd3Theme::serializeState() const
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), g_iThemeHistorySchema);
    root.insert(QStringLiteral("seed"), m_seed.name());
    root.insert(QStringLiteral("scheme"), static_cast<int>(m_enmScheme));
    root.insert(QStringLiteral("scale"), m_dFontScale);
    root.insert(QStringLiteral("compact"), m_fCompact);
    root.insert(QStringLiteral("font"), m_strFontFamily);
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
        entry.insert(QStringLiteral("seed"), value.seed.isValid() ? value.seed.name() : QString());
        entry.insert(QStringLiteral("font"), value.strFont.left(g_iMaxHistoryKeyLength));
        entry.insert(QStringLiteral("radius"), value.iRadius);
        entry.insert(QStringLiteral("scale"), value.dScale);
        entry.insert(QStringLiteral("weight"), value.iWeight);
        appearances.insert(strBoundedKey, entry);
        ++cEntries;
    }
    root.insert(QStringLiteral("appearances"), appearances);

    const QJsonDocument namedThemes = QJsonDocument::fromJson(exportNamedThemes());
    if (namedThemes.isObject())
        root.insert(QStringLiteral("namedThemes"), namedThemes.object());

    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Compact);
    return data.size() <= 512 * 1024 ? data : QByteArray();
}

bool UIMd3Theme::restoreState(const QByteArray &data)
{
    if (data.isEmpty() || data.size() > 512 * 1024)
        return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("version")).toInt(-1) != g_iThemeHistorySchema)
        return false;

    const QColor seed(root.value(QStringLiteral("seed")).toString());
    const int iScheme = root.value(QStringLiteral("scheme")).toInt(-1);
    const double dScale = root.value(QStringLiteral("scale")).toDouble(-1.0);
    if (!seed.isValid() || iScheme < static_cast<int>(UIMd3Scheme_Dark)
        || iScheme > static_cast<int>(UIMd3Scheme_HighContrastLight)
        || dScale < 0.75 || dScale > 2.0
        || !root.value(QStringLiteral("compact")).isBool())
        return false;

    QHash<QString, UIMd3Appearance> appearances;
    const QJsonObject appearanceObject = root.value(QStringLiteral("appearances")).toObject();
    int cEntries = 0;
    for (QJsonObject::const_iterator it = appearanceObject.begin();
         it != appearanceObject.end() && cEntries < g_iMaxHistoryAppearanceEntries; ++it)
    {
        const QString strKey = it.key().trimmed().left(g_iMaxHistoryKeyLength);
        const QJsonObject entry = it.value().toObject();
        if (strKey.isEmpty() || entry.isEmpty())
            continue;
        UIMd3Appearance value;
        value.fValid = true;
        const QString strAppearanceSeed = entry.value(QStringLiteral("seed")).toString();
        value.seed = QColor(strAppearanceSeed);
        if (!strAppearanceSeed.isEmpty() && !value.seed.isValid())
            continue;
        value.strFont = entry.value(QStringLiteral("font")).toString().left(g_iMaxHistoryKeyLength);
        value.iRadius = qBound(0, entry.value(QStringLiteral("radius")).toInt(UIMd3Shape::Large),
                                static_cast<int>(UIMd3Shape::Full));
        value.dScale = qBound(0.50, entry.value(QStringLiteral("scale")).toDouble(1.0), 2.0);
        value.iWeight = entry.value(QStringLiteral("weight")).toInt(-1);
        if (value.iWeight >= 0)
            value.iWeight = qBound(static_cast<int>(QFont::Thin), value.iWeight,
                                   static_cast<int>(QFont::Black));
        if (!value.strFont.isEmpty() && !QFontDatabase::families().contains(value.strFont))
            value.strFont.clear();
        appearances.insert(strKey, value);
        ++cEntries;
    }

    QHash<QString, QVariantMap> namedThemes;
    const QJsonObject namedThemeObject = root.value(QStringLiteral("namedThemes")).toObject();
    int cNamedThemes = 0;
    const QStringList fontFamilies = QFontDatabase::families();
    for (QJsonObject::const_iterator it = namedThemeObject.begin();
         it != namedThemeObject.end() && cNamedThemes < g_iMaxHistoryAppearanceEntries; ++it)
    {
        const QString strName = it.key().trimmed().left(g_iMaxHistoryKeyLength);
        const QJsonObject entry = it.value().toObject();
        const QColor themeSeed(entry.value(QStringLiteral("seed")).toString());
        const int iThemeScheme = entry.value(QStringLiteral("scheme")).toInt(-1);
        const double dThemeScale = entry.value(QStringLiteral("scale")).toDouble(-1.0);
        const QString strThemeFont = entry.value(QStringLiteral("font")).toString()
                                           .trimmed().left(g_iMaxHistoryKeyLength);
        if (strName.isEmpty() || !themeSeed.isValid()
            || iThemeScheme < static_cast<int>(UIMd3Scheme_Dark)
            || iThemeScheme > static_cast<int>(UIMd3Scheme_HighContrastLight)
            || dThemeScale < 0.75 || dThemeScale > 2.0
            || !entry.value(QStringLiteral("compact")).isBool()
            || (!strThemeFont.isEmpty() && !fontFamilies.contains(strThemeFont)))
            continue;
        QVariantMap map;
        map.insert(QStringLiteral("seed"), themeSeed.name());
        map.insert(QStringLiteral("scheme"), iThemeScheme);
        map.insert(QStringLiteral("scale"), dThemeScale);
        map.insert(QStringLiteral("compact"), entry.value(QStringLiteral("compact")).toBool());
        map.insert(QStringLiteral("font"), strThemeFont);
        namedThemes.insert(strName, map);
        ++cNamedThemes;
    }

    m_fRestoring = true;
    m_seed = seed;
    m_enmScheme = static_cast<UIMd3Scheme>(iScheme);
    m_dFontScale = dScale;
    m_fCompact = root.value(QStringLiteral("compact")).toBool();
    const QString strFont = root.value(QStringLiteral("font")).toString().left(g_iMaxHistoryKeyLength);
    if (!strFont.isEmpty() && fontFamilies.contains(strFont))
        m_strFontFamily = strFont;
    const QString strBrand = root.value(QStringLiteral("brand")).toString().trimmed();
    if (!strBrand.isEmpty())
        m_strBrandName = strBrand.left(80);
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
        UIMd3History::instance()->record(strAction, strDetail, serializeState());
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
    const QString strFont = gEDataManager->extraDataString(g_pszKeyFont);
    if (!strFont.isEmpty())
        m_strFontFamily = strFont;
    const QString strBrand = gEDataManager->extraDataString(g_pszKeyBrand);
    if (!strBrand.trimmed().isEmpty())
        m_strBrandName = strBrand.left(80);

    /* Per-element overrides: */
    m_appearances.clear();
    const QJsonDocument docAppearance = QJsonDocument::fromJson(gEDataManager->extraDataString(g_pszKeyAppearance).toUtf8());
    const QJsonObject objAppearance = docAppearance.object();
    for (QJsonObject::const_iterator it = objAppearance.begin(); it != objAppearance.end(); ++it)
    {
        const QJsonObject entry = it.value().toObject();
        UIMd3Appearance appearance;
        appearance.fValid   = true;
        appearance.seed     = QColor(entry.value("seed").toString());
        appearance.strFont  = entry.value("font").toString();
        appearance.iRadius  = qBound(0, entry.value("radius").toInt(static_cast<int>(UIMd3Shape::Large)),
                                      static_cast<int>(UIMd3Shape::Full));
        const QJsonValue scaleValue = entry.value("scale");
        appearance.dScale   = qBound(0.50, scaleValue.isDouble() ? scaleValue.toDouble() : 1.0, 2.00);
        appearance.iWeight  = entry.value("weight").toInt(-1);
        if (appearance.iWeight >= 0)
            appearance.iWeight = qBound(static_cast<int>(QFont::Thin), appearance.iWeight,
                                        static_cast<int>(QFont::Black));
        if (!appearance.strFont.isEmpty() && !QFontDatabase::families().contains(appearance.strFont))
            appearance.strFont.clear();
        if (!it.key().trimmed().isEmpty())
            m_appearances.insert(it.key().trimmed(), appearance);
    }

    /* Named themes: */
    m_namedThemes.clear();
    importNamedThemes(gEDataManager->extraDataString(g_pszKeyThemes).toUtf8());

    regenerate();
    emit sigThemeChanged();
}

void UIMd3Theme::saveToExtraData() const
{
    gEDataManager->setExtraDataString(g_pszKeySeed, m_seed.name());
    gEDataManager->setExtraDataString(g_pszKeyScheme, QString::number((int)m_enmScheme));
    gEDataManager->setExtraDataString(g_pszKeyScale, QString::number(m_dFontScale));
    gEDataManager->setExtraDataString(g_pszKeyCompact, m_fCompact ? "true" : "false");
    gEDataManager->setExtraDataString(g_pszKeyFont, m_strFontFamily);
    gEDataManager->setExtraDataString(g_pszKeyBrand, m_strBrandName);

    QJsonObject objAppearance;
    for (QHash<QString, UIMd3Appearance>::const_iterator it = m_appearances.begin(); it != m_appearances.end(); ++it)
    {
        QJsonObject entry;
        entry.insert("seed",   it.value().seed.isValid() ? it.value().seed.name() : QString());
        entry.insert("font",   it.value().strFont);
        entry.insert("radius", it.value().iRadius);
        entry.insert("scale",  it.value().dScale);
        entry.insert("weight", it.value().iWeight);
        objAppearance.insert(it.key(), entry);
    }
    gEDataManager->setExtraDataString(g_pszKeyAppearance, QString::fromUtf8(QJsonDocument(objAppearance).toJson(QJsonDocument::Compact)));
    gEDataManager->setExtraDataString(g_pszKeyThemes, QString::fromUtf8(exportNamedThemes()));
}

UIMd3Scheme UIMd3Theme::effectiveScheme() const
{
    if (m_enmScheme != UIMd3Scheme_System)
        return m_enmScheme;
    if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Light)
        return UIMd3Scheme_Light;
    return UIMd3Scheme_Dark;
}

QColor UIMd3Theme::tone(const QColor &base, int iTone)
{
    /* Approximate the HCT tonal palette by holding hue and chroma while
     * driving lightness to the requested tone. This keeps the palette
     * perceptually close to the reference implementation without pulling
     * an extra dependency into the frontend. */
    float h = 0, s = 0, l = 0, a = 0;
    base.getHslF(&h, &s, &l, &a);
    const float dTarget = qBound(0.0f, iTone / 100.0f, 1.0f);
    /* Chroma decays towards the extremes, exactly as the M3 palettes do: */
    const float dChroma = s * (1.0f - qAbs(dTarget - 0.5f) * 0.7f);
    QColor result;
    result.setHslF(h, qBound(0.0f, dChroma, 1.0f), dTarget, a);
    return result.toRgb();
}

void UIMd3Theme::regenerate()
{
    const bool fDark = effectiveScheme() == UIMd3Scheme_Dark || effectiveScheme() == UIMd3Scheme_HighContrastDark;
    const bool fContrast = effectiveScheme() == UIMd3Scheme_HighContrastDark || effectiveScheme() == UIMd3Scheme_HighContrastLight;

    QColor primaryBase = m_seed;
    QColor secondaryBase = QColor::fromHsvF(fmod(m_seed.hueF() + 0.02, 1.0), m_seed.saturationF() * 0.4, m_seed.valueF());
    QColor tertiaryBase = QColor::fromHsvF(fmod(m_seed.hueF() + 0.16, 1.0), m_seed.saturationF() * 0.6, m_seed.valueF());
    QColor neutralBase = QColor::fromHsvF(m_seed.hueF(), m_seed.saturationF() * 0.06, m_seed.valueF());
    QColor errorBase = QColor("#B3261E");

    if (fDark)
    {
        m_colors[UIMd3ColorRole_Primary]                  = tone(primaryBase, fContrast ? 90 : 80);
        m_colors[UIMd3ColorRole_OnPrimary]                = tone(primaryBase, 20);
        m_colors[UIMd3ColorRole_PrimaryContainer]         = tone(primaryBase, 30);
        m_colors[UIMd3ColorRole_OnPrimaryContainer]       = tone(primaryBase, 90);
        m_colors[UIMd3ColorRole_Secondary]                = tone(secondaryBase, 80);
        m_colors[UIMd3ColorRole_OnSecondary]              = tone(secondaryBase, 20);
        m_colors[UIMd3ColorRole_SecondaryContainer]       = tone(secondaryBase, 30);
        m_colors[UIMd3ColorRole_OnSecondaryContainer]     = tone(secondaryBase, 90);
        m_colors[UIMd3ColorRole_Tertiary]                 = tone(tertiaryBase, 80);
        m_colors[UIMd3ColorRole_OnTertiary]               = tone(tertiaryBase, 20);
        m_colors[UIMd3ColorRole_TertiaryContainer]        = tone(tertiaryBase, 30);
        m_colors[UIMd3ColorRole_OnTertiaryContainer]      = tone(tertiaryBase, 90);
        m_colors[UIMd3ColorRole_Error]                    = tone(errorBase, 80);
        m_colors[UIMd3ColorRole_OnError]                  = tone(errorBase, 20);
        m_colors[UIMd3ColorRole_ErrorContainer]           = tone(errorBase, 30);
        m_colors[UIMd3ColorRole_OnErrorContainer]         = tone(errorBase, 90);
        m_colors[UIMd3ColorRole_Surface]                  = tone(neutralBase, 6);
        m_colors[UIMd3ColorRole_OnSurface]                = tone(neutralBase, fContrast ? 100 : 90);
        m_colors[UIMd3ColorRole_OnSurfaceVariant]         = tone(neutralBase, 80);
        m_colors[UIMd3ColorRole_SurfaceContainerLowest]   = tone(neutralBase, 4);
        m_colors[UIMd3ColorRole_SurfaceContainerLow]      = tone(neutralBase, 10);
        m_colors[UIMd3ColorRole_SurfaceContainer]         = tone(neutralBase, 12);
        m_colors[UIMd3ColorRole_SurfaceContainerHigh]     = tone(neutralBase, 17);
        m_colors[UIMd3ColorRole_SurfaceContainerHighest]  = tone(neutralBase, 22);
        m_colors[UIMd3ColorRole_Outline]                  = tone(neutralBase, fContrast ? 80 : 60);
        m_colors[UIMd3ColorRole_OutlineVariant]           = tone(neutralBase, 30);
    }
    else
    {
        m_colors[UIMd3ColorRole_Primary]                  = tone(primaryBase, fContrast ? 30 : 40);
        m_colors[UIMd3ColorRole_OnPrimary]                = tone(primaryBase, 100);
        m_colors[UIMd3ColorRole_PrimaryContainer]         = tone(primaryBase, 90);
        m_colors[UIMd3ColorRole_OnPrimaryContainer]       = tone(primaryBase, 10);
        m_colors[UIMd3ColorRole_Secondary]                = tone(secondaryBase, 40);
        m_colors[UIMd3ColorRole_OnSecondary]              = tone(secondaryBase, 100);
        m_colors[UIMd3ColorRole_SecondaryContainer]       = tone(secondaryBase, 90);
        m_colors[UIMd3ColorRole_OnSecondaryContainer]     = tone(secondaryBase, 10);
        m_colors[UIMd3ColorRole_Tertiary]                 = tone(tertiaryBase, 40);
        m_colors[UIMd3ColorRole_OnTertiary]               = tone(tertiaryBase, 100);
        m_colors[UIMd3ColorRole_TertiaryContainer]        = tone(tertiaryBase, 90);
        m_colors[UIMd3ColorRole_OnTertiaryContainer]      = tone(tertiaryBase, 10);
        m_colors[UIMd3ColorRole_Error]                    = tone(errorBase, 40);
        m_colors[UIMd3ColorRole_OnError]                  = tone(errorBase, 100);
        m_colors[UIMd3ColorRole_ErrorContainer]           = tone(errorBase, 90);
        m_colors[UIMd3ColorRole_OnErrorContainer]         = tone(errorBase, 10);
        m_colors[UIMd3ColorRole_Surface]                  = tone(neutralBase, 98);
        m_colors[UIMd3ColorRole_OnSurface]                = tone(neutralBase, fContrast ? 0 : 10);
        m_colors[UIMd3ColorRole_OnSurfaceVariant]         = tone(neutralBase, 30);
        m_colors[UIMd3ColorRole_SurfaceContainerLowest]   = tone(neutralBase, 100);
        m_colors[UIMd3ColorRole_SurfaceContainerLow]      = tone(neutralBase, 96);
        m_colors[UIMd3ColorRole_SurfaceContainer]         = tone(neutralBase, 94);
        m_colors[UIMd3ColorRole_SurfaceContainerHigh]     = tone(neutralBase, 92);
        m_colors[UIMd3ColorRole_SurfaceContainerHighest]  = tone(neutralBase, 90);
        m_colors[UIMd3ColorRole_Outline]                  = tone(neutralBase, fContrast ? 30 : 50);
        m_colors[UIMd3ColorRole_OutlineVariant]           = tone(neutralBase, 80);
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
