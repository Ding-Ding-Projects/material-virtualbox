/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Export class declaration - multi-format export of any view.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Export_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Export_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QString>
#include <QStringList>
#include <QVector>

/** A view reduced to exportable rows: a section, a key and a value. */
struct UIMd3ExportRow
{
    QString strSection;
    QString strKey;
    QString strValue;
};

/** Multi-format serialiser used by every "Export view" affordance.
  *
  * Any surface that can describe itself as sections of key/value rows gets all
  * seventeen output formats for free, and no export ever contains a credential:
  * the caller supplies only what the surface already displays. */
class UIMd3Export
{
public:

    /** Returns every supported format extension, in menu order. */
    static QStringList formats();
    /** Returns @a rows serialised as @a strFormat, titled @a strTitle. */
    static QByteArray serialize(const QVector<UIMd3ExportRow> &rows, const QString &strFormat, const QString &strTitle);
    /** Writes @a rows to @a strPath in the format implied by its suffix. Returns false on an I/O error. */
    static bool writeToFile(const QVector<UIMd3ExportRow> &rows, const QString &strPath, const QString &strTitle, QString &strError);
    /** Copies @a rows to the clipboard as @a strFormat. */
    static void copyToClipboard(const QVector<UIMd3ExportRow> &rows, const QString &strFormat, const QString &strTitle);
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Export_h */
