/* $Id$ */
/** @file
 * VBox Qt GUI - persisted Material 3 language presentation settings.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, in version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "UIExtraDataManager.h"
#include "UIMd3Language.h"

static const char *g_pszMode = "GUI/Md3/LanguageMode";
static const char *g_pszEnglishFunny = "GUI/Md3/FunnyLevelEnglish";
static const char *g_pszCantoneseFunny = "GUI/Md3/FunnyLevelCantonese";

static QString md3EnglishStyle(const QString &strText, int iLevel)
{
    static const char * const s_aSuffixes[] =
    {
        "", " · please", " · nice and tidy", " · tiny victory", " · let’s go"
    };
    return strText + QString::fromUtf8(s_aSuffixes[qBound(1, iLevel, 5) - 1]);
}

static QString md3CantoneseStyle(const QString &strText, int iLevel)
{
    static const QStringList s_aSuffixes =
    {
        QString(), QStringLiteral(" · 輕輕鬆鬆"), QStringLiteral(" · 幾醒喎"),
        QStringLiteral(" · 好掂呀"), QStringLiteral(" · 勁到飛起")
    };
    return strText + s_aSuffixes.value(qBound(1, iLevel, 5) - 1);
}

UIMd3Language *UIMd3Language::s_pInstance = 0;

UIMd3Language *UIMd3Language::instance() { return s_pInstance; }

void UIMd3Language::create()
{
    if (s_pInstance)
        return;
    s_pInstance = new UIMd3Language;
    if (gEDataManager)
    {
        s_pInstance->m_enmMode = (UIMd3LanguageMode)qBound(0, gEDataManager->extraDataString(g_pszMode).toInt(), 2);
        s_pInstance->m_iPlayfulness = qBound(1, gEDataManager->extraDataString(g_pszEnglishFunny).toInt(), 5);
        s_pInstance->m_iCantonesePlayfulness = qBound(1, gEDataManager->extraDataString(g_pszCantoneseFunny).toInt(), 5);
    }
}

void UIMd3Language::destroy()
{
    if (!s_pInstance)
        return;
    if (gEDataManager)
    {
        gEDataManager->setExtraDataString(g_pszMode, QString::number((int)s_pInstance->m_enmMode));
        gEDataManager->setExtraDataString(g_pszEnglishFunny, QString::number(s_pInstance->m_iPlayfulness));
        gEDataManager->setExtraDataString(g_pszCantoneseFunny, QString::number(s_pInstance->m_iCantonesePlayfulness));
    }
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3Language::UIMd3Language()
    : m_enmMode(UIMd3LanguageMode_English)
    , m_iPlayfulness(1)
    , m_iCantonesePlayfulness(1)
{
    /* Shared search affordances use the same persisted language service as
     * the surfaces that own them.  Keeping these keys here prevents each
     * search field from growing a subtly different translation path. */
    registerText(QStringLiteral("md3.search.regex-builder"),
                 QStringLiteral("Open the regex builder for this search"),
                 QStringLiteral("開啟呢個搜尋嘅正則表達式建立器"));
    registerText(QStringLiteral("md3.search.regex-builder-name"),
                 QStringLiteral("Open regex builder"),
                 QStringLiteral("開啟正則表達式建立器"));
    registerText(QStringLiteral("md3.search.regex-active"),
                 QStringLiteral("Regular-expression search is active with flags %1."),
                 QStringLiteral("正則表達式搜尋已啟用，旗標係 %1。"));
    registerText(QStringLiteral("md3.search.regex-active-no-flags"),
                 QStringLiteral("Regular-expression search is active without flags."),
                 QStringLiteral("正則表達式搜尋已啟用，冇使用旗標。"));
    registerText(QStringLiteral("md3.settings.customization"),
                 QStringLiteral("Appearance and language"),
                 QStringLiteral("外觀同語言"));
    registerText(QStringLiteral("md3.settings.customization.description"),
                 QStringLiteral("Show or hide global language and appearance controls"),
                 QStringLiteral("顯示或者收起全域語言同外觀控制"));
    registerText(QStringLiteral("md3.settings.customization.panel"),
                 QStringLiteral("Language and appearance customization"),
                 QStringLiteral("語言同外觀自訂"));
    registerText(QStringLiteral("md3.manager.navigation-rail"),
                 QStringLiteral("Navigation rail"),
                 QStringLiteral("導覽列"));
    registerText(QStringLiteral("md3.appearance.edit"),
                 QStringLiteral("Edit appearance…"),
                 QStringLiteral("編輯外觀…"));
    registerText(QStringLiteral("md3.appearance.edit-for"),
                 QStringLiteral("Edit appearance for %1"),
                 QStringLiteral("編輯 %1 嘅外觀"));
    registerText(QStringLiteral("md3.appearance.search-actions"),
                 QStringLiteral("Search appearance actions"),
                 QStringLiteral("搜尋外觀動作"));
    registerText(QStringLiteral("md3.appearance.search-menu"),
                 QStringLiteral("Search this appearance menu"),
                 QStringLiteral("搜尋呢個外觀選單"));
}

UIMd3Language::~UIMd3Language() = default;

void UIMd3Language::setMode(UIMd3LanguageMode enmMode)
{
    if (enmMode < UIMd3LanguageMode_English || enmMode > UIMd3LanguageMode_Bilingual || enmMode == m_enmMode)
        return;
    m_enmMode = enmMode;
    if (gEDataManager)
        gEDataManager->setExtraDataString(g_pszMode, QString::number((int)m_enmMode));
    emit sigLanguageChanged();
}

void UIMd3Language::setPlayfulness(int iLevel)
{
    const int iClamped = qBound(1, iLevel, 5);
    if (iClamped == m_iPlayfulness)
        return;
    m_iPlayfulness = iClamped;
    if (gEDataManager)
        gEDataManager->setExtraDataString(g_pszEnglishFunny, QString::number(m_iPlayfulness));
    emit sigLanguageChanged();
}

void UIMd3Language::setCantonesePlayfulness(int iLevel)
{
    const int iClamped = qBound(1, iLevel, 5);
    if (iClamped == m_iCantonesePlayfulness)
        return;
    m_iCantonesePlayfulness = iClamped;
    if (gEDataManager)
        gEDataManager->setExtraDataString(g_pszCantoneseFunny, QString::number(m_iCantonesePlayfulness));
    emit sigLanguageChanged();
}

QString UIMd3Language::text(const QString &strKey) const
{
    const QPair<QString, QString> pair = m_strings.value(strKey);
    const QString strEnglish = pair.first.isEmpty() ? strKey : pair.first;
    const QString strCantonese = pair.second.isEmpty() ? strEnglish : pair.second;
    const QString strStyledEnglish = md3EnglishStyle(strEnglish, m_iPlayfulness);
    const QString strStyledCantonese = md3CantoneseStyle(strCantonese, m_iCantonesePlayfulness);
    if (m_enmMode == UIMd3LanguageMode_Cantonese)
        return strStyledCantonese;
    if (m_enmMode == UIMd3LanguageMode_Bilingual)
        return QStringLiteral("%1 · %2").arg(strStyledEnglish, strStyledCantonese);
    return strStyledEnglish;
}

void UIMd3Language::registerText(const QString &strKey, const QString &strEnglish, const QString &strCantonese)
{
    if (strKey.isEmpty())
        return;
    m_strings.insert(strKey, qMakePair(strEnglish, strCantonese));
}
