/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3EmojiSetting class implementation - persisted "show emojis in dialogs and message boxes" toggle.
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

/* GUI includes: */
#include "UIMd3EmojiSetting.h"
#include "UIExtraDataManager.h"

/* Qt includes: */
#include <QRegularExpression>

/* Extra-data key this toggle is persisted under, in the same "GUI/Md3/..."
 * namespace UIMd3Theme's own settings (seed, scheme, compact density, ...)
 * already use -- see UIMd3Theme.cpp's g_pszKey* constants. Reusing that same
 * store, rather than inventing a second persistence layer, is deliberate. */
static const char *g_pszEmojiExtraDataKey = "GUI/Md3/EmojiDialogs";

/* static */
bool UIMd3EmojiSetting::isEnabled()
{
    /* Disabled until a user opts in; an empty/unset extra-data value must not
     * read as enabled. */
    return gEDataManager->extraDataString(g_pszEmojiExtraDataKey) == "true";
}

/* static */
void UIMd3EmojiSetting::setEnabled(bool fEnabled)
{
    gEDataManager->setExtraDataString(g_pszEmojiExtraDataKey, fEnabled ? "true" : "false");
}

/* static */
bool UIMd3EmojiSetting::toggle()
{
    const bool fEnabled = !isEnabled();
    setEnabled(fEnabled);
    return fEnabled;
}

/* static */
QString UIMd3EmojiSetting::emojiFor(UIMd3EmojiKind enmKind)
{
    switch (enmKind)
    {
        case UIMd3EmojiKind_Information: return QStringLiteral("📌"); /* pushpin -- "worth noting" */
        case UIMd3EmojiKind_Question:    return QStringLiteral("🤔"); /* thinking face -- "needs an answer" */
        case UIMd3EmojiKind_Warning:     return QStringLiteral("🚧"); /* construction -- "proceed carefully" */
        case UIMd3EmojiKind_Error:       return QStringLiteral("🔧"); /* wrench -- "something needs fixing" */
        case UIMd3EmojiKind_Success:     return QStringLiteral("🎉"); /* party popper -- "it worked" */
        case UIMd3EmojiKind_Destructive: return QStringLiteral("🧨"); /* dynamite -- "this removes something" */
        case UIMd3EmojiKind_Progress:    return QStringLiteral("⌛"); /* hourglass -- "still going" */
        case UIMd3EmojiKind_General:
        default:                         return QStringLiteral("💬"); /* speech balloon -- generic dialog */
    }
}

/* static */
QString UIMd3EmojiSetting::decorate(const QString &strText, UIMd3EmojiKind enmKind)
{
    if (strText.isEmpty() || !isEnabled())
        return strText;

    const QString strPrefix = emojiFor(enmKind) + QStringLiteral("  ");
    const QRegularExpression openingTag(QStringLiteral("^(?:\\s*<(?:html|body|qt|p)(?:\\s[^>]*)?>)+"),
                                        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = openingTag.match(strText);
    if (!match.hasMatch())
        return strPrefix + strText;

    QString strDecorated = strText;
    strDecorated.insert(match.capturedEnd(), strPrefix);
    return strDecorated;
}
