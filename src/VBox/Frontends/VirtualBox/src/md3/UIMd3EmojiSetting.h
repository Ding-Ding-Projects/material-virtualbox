/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3EmojiSetting class declaration - persisted "show emojis in dialogs and message boxes" toggle.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3EmojiSetting_h
#define FEQT_INCLUDED_SRC_md3_UIMd3EmojiSetting_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QString>

/* GUI includes: */
#include "UILibraryDefs.h"

/** The broad, decorative categories a dialog or message box can request an
  * emoji for.
  *
  * These name a *tone*, not a meaning: the emoji chosen for each kind is
  * topically relevant but never the only thing that conveys the dialog's
  * actual significance -- the dialog's real icon, wording, and button choices
  * still carry the full semantics on their own, exactly as they do when the
  * toggle is switched off. That is what keeps the decoration "non-semantic". */
enum UIMd3EmojiKind
{
    UIMd3EmojiKind_General = 0,
    UIMd3EmojiKind_Information,
    UIMd3EmojiKind_Question,
    UIMd3EmojiKind_Warning,
    UIMd3EmojiKind_Error,
    UIMd3EmojiKind_Success,
    UIMd3EmojiKind_Destructive,
    UIMd3EmojiKind_Progress
};

/** Persisted, user-controllable "show emojis in dialogs and message boxes" toggle.
  *
  * When enabled, decorate() prefixes a dialog or message box's own displayed
  * message copy with one relevant, purely decorative emoji chosen from
  * UIMd3EmojiKind; when disabled it hands back the caller's text completely
  * unchanged, so the exact same factual copy ships either way. The state
  * survives restarts through this frontend's ordinary extra-data mechanism
  * (see UIExtraDataManager / gEDataManager), the same store UIMd3Theme uses
  * for its own persisted settings -- no separate persistence layer is
  * introduced here.
  *
  * decorate() must only ever be called on a dialog's own displayed message
  * string. It must never be applied to button text, action
  * labels, field labels, or accessible names -- nothing in this class touches
  * control text, and callers are expected to keep it that way.
  *
  * Static-only, like UIMd3ExternalEditor: no COM coupling, no owned widgets,
  * no constructed instance, safe to call from anywhere without construction
  * or lifecycle management. */
class SHARED_LIBRARY_STUFF UIMd3EmojiSetting
{
public:

    /** Returns whether the toggle is currently enabled (persisted). Disabled by default. */
    static bool isEnabled();
    /** Persists @a fEnabled as the new toggle state. */
    static void setEnabled(bool fEnabled);
    /** Flips the persisted state and returns the new value. */
    static bool toggle();

    /** Returns the single relevant, non-semantic emoji for @a enmKind, regardless
      * of whether the toggle is currently enabled. */
    static QString emojiFor(UIMd3EmojiKind enmKind);

    /** Returns @a strText unchanged when the toggle is disabled or @a strText is
      * empty; otherwise returns @a strText prefixed with a decorative emoji for
      * @a enmKind. Leading rich-text container markup is preserved and the
      * prefix is inserted inside its first tag. @a strText must be a displayed
      * dialog/message-box message --
      * never button text, an action label, a field label, or an accessible name. */
    static QString decorate(const QString &strText, UIMd3EmojiKind enmKind = UIMd3EmojiKind_General);

private:

    /** This class is static-only. */
    UIMd3EmojiSetting();
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3EmojiSetting_h */
