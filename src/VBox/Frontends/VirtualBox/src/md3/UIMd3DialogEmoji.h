/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3DialogEmoji class declaration - persisted "Show emojis in
 * dialogs and message boxes" preference and its decoration helper.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3DialogEmoji_h
#define FEQT_INCLUDED_SRC_md3_UIMd3DialogEmoji_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QObject>
#include <QString>

/* GUI includes: */
#include "UILibraryDefs.h"

/** Relevant, non-semantic emoji families offered to a dialog or message box.
  * These mirror the standard message-box icon families already used by
  * QIMessageBox::AlertIconType and UINotificationObjectItem::standardPixmap()
  * (NotificationType), so a decorated dialog is always styled by the same
  * classification as its real, non-emoji icon rather than a fourth taxonomy
  * nobody asked for. */
enum UIMd3DialogEmojiKind
{
    UIMd3DialogEmojiKind_Info,
    UIMd3DialogEmojiKind_Question,
    UIMd3DialogEmojiKind_Warning,
    UIMd3DialogEmojiKind_Critical,
    UIMd3DialogEmojiKind_GuruMeditation
};

/** QObject extension persisting the "Show emojis in dialogs and message
  * boxes" preference and decorating dialog/message-box body copy with it.
  *
  * The decoration is purely cosmetic.  The real semantic meaning of a dialog
  * is always carried by its native icon and its exact wording, both left
  * completely unaffected by this class -- disabling the preference restores
  * byte-identical text.  Callers must only ever pass this class a message
  * *body*; it must never be applied to a button, an action label, a field
  * label, or anything an accessible name is derived from. */
class SHARED_LIBRARY_STUFF UIMd3DialogEmoji : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies every surface that the enabled state changed. */
    void sigDialogEmojiChanged();

public:

    /** Returns the singleton. */
    static UIMd3DialogEmoji *instance();
    /** Creates the singleton. */
    static void create();
    /** Destroys the singleton. */
    static void destroy();

    /** Returns whether dialogs and message boxes should carry an emoji decoration. */
    bool enabled() const { return m_fEnabled; }
    /** Defines whether dialogs and message boxes should carry an emoji decoration. */
    void setEnabled(bool fEnabled);

    /** Returns @a strText decorated with the emoji relevant to @a enmKind when
      * the preference is enabled, or @a strText completely unchanged when it
      * is disabled.  Safe to call with plain text or with the
      * "<p>...</p>"-wrapped rich text used throughout the notification
      * centre: a leading paragraph tag is detected and the emoji is placed
      * inside it rather than pushed out in front of the markup. */
    QString decorate(const QString &strText, UIMd3DialogEmojiKind enmKind) const;

private:

    UIMd3DialogEmoji();
    virtual ~UIMd3DialogEmoji() override;

    /** Returns the non-semantic emoji glyph for @a enmKind, or an empty string. */
    static QString glyphFor(UIMd3DialogEmojiKind enmKind);

    static UIMd3DialogEmoji *s_pInstance;
    bool                     m_fEnabled;
};

/** Convenience reference used at every call site. */
inline UIMd3DialogEmoji &md3DialogEmoji() { return *UIMd3DialogEmoji::instance(); }

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3DialogEmoji_h */
