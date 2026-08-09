/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Language class declaration - English, playful Cantonese and compact bilingual modes.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Language_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Language_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QObject>
#include <QHash>
#include <QString>

/** The three app language modes required of every user-facing surface. */
enum UIMd3LanguageMode
{
    UIMd3LanguageMode_English,
    UIMd3LanguageMode_Cantonese,
    UIMd3LanguageMode_Bilingual
};

/** QObject extension resolving user-facing strings for the active language mode.
  *
  * This sits beside Qt's own translation system rather than replacing it: Qt
  * handles locale translation, while this class handles the product's own
  * register — plain English, playful Hong Kong Cantonese, or a compact bilingual
  * pairing — and the playfulness level that goes with it. */
class UIMd3Language : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies every surface that the mode or playfulness changed. */
    void sigLanguageChanged();

public:

    /** Returns the singleton. */
    static UIMd3Language *instance();
    /** Creates the singleton. */
    static void create();
    /** Destroys the singleton. */
    static void destroy();

    /** Returns the active mode. */
    UIMd3LanguageMode mode() const { return m_enmMode; }
    /** Defines the active @a enmMode. */
    void setMode(UIMd3LanguageMode enmMode);
    /** Returns the playfulness level, 1 to 5. */
    int playfulness() const { return m_iPlayfulness; }
    /** Defines the playfulness level. */
    void setPlayfulness(int iLevel);
    /** Returns the independent Cantonese playfulness level, 1 to 5. */
    int cantonesePlayfulness() const { return m_iCantonesePlayfulness; }
    /** Defines the independent Cantonese playfulness level. */
    void setCantonesePlayfulness(int iLevel);

    /** Returns the string registered for @a strKey in the active mode.
      * Falls back to the English string, then to @a strKey itself. */
    QString text(const QString &strKey) const;
    /** Registers @a strEnglish and @a strCantonese for @a strKey. */
    void registerText(const QString &strKey, const QString &strEnglish, const QString &strCantonese);

private:

    UIMd3Language();
    virtual ~UIMd3Language() override;

    static UIMd3Language *s_pInstance;
    UIMd3LanguageMode     m_enmMode;
    int                   m_iPlayfulness;
    int                   m_iCantonesePlayfulness;
    QHash<QString, QPair<QString, QString> > m_strings;
};

/** Convenience accessor used at every call site. */
inline QString md3Text(const QString &strKey) { return UIMd3Language::instance()->text(strKey); }

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Language_h */
