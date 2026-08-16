/* $Id$ */
/** @file
 * VBox Qt GUI - persisted "Show emojis in dialogs and message boxes" preference.
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

/* Qt includes: */
#include <QRegularExpression>

/* GUI includes: */
#include "UIExtraDataManager.h"
#include "UIMd3DialogEmoji.h"

static const char *g_pszEnabled = "GUI/Md3/DialogEmojis";

UIMd3DialogEmoji *UIMd3DialogEmoji::s_pInstance = 0;

UIMd3DialogEmoji *UIMd3DialogEmoji::instance() { return s_pInstance; }

void UIMd3DialogEmoji::create()
{
    if (s_pInstance)
        return;
    s_pInstance = new UIMd3DialogEmoji;
    if (gEDataManager)
        s_pInstance->m_fEnabled = gEDataManager->extraDataString(QString::fromLatin1(g_pszEnabled))
                                == QStringLiteral("true");
}

void UIMd3DialogEmoji::destroy()
{
    if (!s_pInstance)
        return;
    if (gEDataManager)
        gEDataManager->setExtraDataString(QString::fromLatin1(g_pszEnabled),
                                          s_pInstance->m_fEnabled ? QStringLiteral("true") : QStringLiteral("false"));
    delete s_pInstance;
    s_pInstance = 0;
}

UIMd3DialogEmoji::UIMd3DialogEmoji()
    : m_fEnabled(false)
{
    /* Shipped default is off: the decoration is a discoverable opt-in style
     * choice, not something every dialog should suddenly grow unasked. */
}

UIMd3DialogEmoji::~UIMd3DialogEmoji() = default;

void UIMd3DialogEmoji::setEnabled(bool fEnabled)
{
    if (fEnabled == m_fEnabled)
        return;
    m_fEnabled = fEnabled;
    if (gEDataManager)
        gEDataManager->setExtraDataString(QString::fromLatin1(g_pszEnabled),
                                          m_fEnabled ? QStringLiteral("true") : QStringLiteral("false"));
    emit sigDialogEmojiChanged();
}

/* static */
QString UIMd3DialogEmoji::glyphFor(UIMd3DialogEmojiKind enmKind)
{
    /* Deliberately restricted to the Basic Multilingual Plane: every glyph
     * below is representable as a single UTF-16 code unit (optionally
     * followed by the U+FE0F emoji-presentation selector), so no compiler's
     * handling of surrogate-pair-requiring astral characters inside a
     * QStringLiteral is ever in question here. */
    switch (enmKind)
    {
        case UIMd3DialogEmojiKind_Info:           return QStringLiteral("ℹ️");
        case UIMd3DialogEmojiKind_Question:       return QStringLiteral("❓");
        case UIMd3DialogEmojiKind_Warning:        return QStringLiteral("⚠️");
        case UIMd3DialogEmojiKind_Critical:       return QStringLiteral("⛔");
        case UIMd3DialogEmojiKind_GuruMeditation: return QStringLiteral("☯️");
        default:                                  return QString();
    }
}

QString UIMd3DialogEmoji::decorate(const QString &strText, UIMd3DialogEmojiKind enmKind) const
{
    /* Disabled or nothing to decorate: return the exact, byte-identical text. */
    if (!m_fEnabled || strText.isEmpty())
        return strText;

    const QString strGlyph = glyphFor(enmKind);
    if (strGlyph.isEmpty())
        return strText;

    /* Notification/message bodies throughout this frontend are frequently
     * "<p>...</p>"-wrapped rich text.  Detect a leading paragraph tag and
     * place the decoration inside it, right before the real words, rather
     * than in front of the markup where a rich-text renderer would show it
     * as a lone line above the paragraph. */
    const QRegularExpression leadingParagraph(QStringLiteral("^\\s*<p[^>]*>"),
                                              QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = leadingParagraph.match(strText);
    if (match.hasMatch())
    {
        QString strDecorated = strText;
        strDecorated.insert(match.capturedEnd(), strGlyph + QStringLiteral(" "));
        return strDecorated;
    }

    return strGlyph + QStringLiteral(" ") + strText;
}
