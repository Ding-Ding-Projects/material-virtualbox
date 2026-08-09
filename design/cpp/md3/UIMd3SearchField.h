/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3SearchField class declaration - search field with a mandatory regex builder.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3SearchField_h
#define FEQT_INCLUDED_SRC_md3_UIMd3SearchField_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QLineEdit>
#include <QRegularExpression>
#include <QString>

/* GUI includes: */
#include "UIMd3Widget.h"

class UIMd3Button;

/** UIMd3Widget extension implementing the one search field the whole GUI uses.
  *
  * Every search bar in the product is an instance of this class, which is what
  * guarantees the two global rules: plain-text matching is the default, and the
  * full regex builder is one click away on every field. The field owns its own
  * pattern state, so a pattern applied to one field never leaks into another. */
class UIMd3SearchField : public UIMd3Widget
{
    Q_OBJECT;

signals:

    /** Notifies listeners that the effective filter changed. */
    void sigFilterChanged();

public:

    /** Constructs a search field.
      * @param  strFieldId     Stable identifier; the applied pattern is persisted under it.
      * @param  strPlaceholder Placeholder shown while the field is empty. */
    UIMd3SearchField(const QString &strFieldId, const QString &strPlaceholder, QWidget *pParent = 0);

    /** Returns the raw text typed into the field. */
    QString text() const;
    /** Defines the field @a strText. */
    void setText(const QString &strText);

    /** Returns whether @a strCandidate passes the current filter. */
    bool matches(const QString &strCandidate) const;

    /** Returns whether a regex pattern is applied to this field. */
    bool isRegexActive() const { return m_fRegexActive; }
    /** Applies @a strPattern with @a strFlags to this field. Returns false when the pattern is invalid. */
    bool applyRegex(const QString &strPattern, const QString &strFlags);
    /** Drops any applied pattern and returns the field to plain-text matching. */
    void clearRegex();

private slots:

    /** Handles the builder button. */
    void sltOpenBuilder();
    /** Handles text edits. */
    void sltTextEdited();

private:

    /** Prepares all contents. */
    void prepare();

    QString             m_strFieldId;
    QString             m_strPlaceholder;
    QLineEdit          *m_pEditor;
    UIMd3Button        *m_pBuilderButton;
    bool                m_fRegexActive;
    QRegularExpression  m_regex;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3SearchField_h */
