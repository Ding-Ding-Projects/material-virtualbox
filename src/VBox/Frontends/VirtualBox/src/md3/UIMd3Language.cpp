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
}

UIMd3Language::~UIMd3Language() = default;

void UIMd3Language::setMode(UIMd3LanguageMode enmMode)
{
    if (enmMode < UIMd3LanguageMode_English || enmMode > UIMd3LanguageMode_Bilingual || enmMode == m_enmMode)
        return;
    m_enmMode = enmMode;
    emit sigLanguageChanged();
}

void UIMd3Language::setPlayfulness(int iLevel)
{
    const int iClamped = qBound(1, iLevel, 5);
    if (iClamped == m_iPlayfulness)
        return;
    m_iPlayfulness = iClamped;
    emit sigLanguageChanged();
}

void UIMd3Language::setCantonesePlayfulness(int iLevel)
{
    const int iClamped = qBound(1, iLevel, 5);
    if (iClamped == m_iCantonesePlayfulness)
        return;
    m_iCantonesePlayfulness = iClamped;
    emit sigLanguageChanged();
}

QString UIMd3Language::text(const QString &strKey) const
{
    const QPair<QString, QString> pair = m_strings.value(strKey);
    const QString strEnglish = pair.first.isEmpty() ? strKey : pair.first;
    const QString strCantonese = pair.second.isEmpty() ? strEnglish : pair.second;
    if (m_enmMode == UIMd3LanguageMode_Cantonese)
        return strCantonese;
    if (m_enmMode == UIMd3LanguageMode_Bilingual)
        return QStringLiteral("%1 · %2").arg(strEnglish, strCantonese);
    return strEnglish;
}

void UIMd3Language::registerText(const QString &strKey, const QString &strEnglish, const QString &strCantonese)
{
    if (strKey.isEmpty())
        return;
    m_strings.insert(strKey, qMakePair(strEnglish, strCantonese));
}
