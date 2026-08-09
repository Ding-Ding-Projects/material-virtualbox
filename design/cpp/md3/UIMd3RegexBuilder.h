/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3RegexBuilder class declaration - the shared full regex builder.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3RegexBuilder_h
#define FEQT_INCLUDED_SRC_md3_UIMd3RegexBuilder_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDialog>
#include <QString>

/* Forward declarations: */
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class UIMd3Button;
class UIMd3SearchField;

/** QDialog extension implementing the product-wide regex builder.
  *
  * The builder is deliberately a single implementation: every search field in
  * the GUI opens this dialog, so the guided constructs, flag handling, live
  * highlighting, bounded 300 ms evaluation and the plain-text default behave
  * identically everywhere. */
class UIMd3RegexBuilder : public QDialog
{
    Q_OBJECT;

public:

    /** Opens the builder for @a pField and applies the result on accept. */
    static void editField(UIMd3SearchField *pField, const QString &strFieldId);

    /** Constructs the builder for @a strFieldId. */
    UIMd3RegexBuilder(const QString &strFieldId, const QString &strInitialPattern,
                      const QString &strInitialFlags, QWidget *pParent = 0);

    /** Returns the edited pattern. */
    QString pattern() const;
    /** Returns the edited flags. */
    QString flags() const;
    /** Returns whether the user asked to clear the pattern instead of applying it. */
    bool isCleared() const { return m_fCleared; }

private slots:

    /** Re-validates the pattern and refreshes the preview. */
    void sltRevalidate();
    /** Inserts the guided construct carried by the sender. */
    void sltInsertConstruct();
    /** Handles the clear request. */
    void sltClear();

private:

    /** Prepares all contents. */
    void prepare();
    /** Prepares the guided-construct chip row. */
    void prepareConstructs(QLayout *pLayout);

    QString         m_strFieldId;
    QString         m_strInitialPattern;
    QString         m_strInitialFlags;
    QLineEdit      *m_pPatternEditor;
    QLineEdit      *m_pFlagsEditor;
    QPlainTextEdit *m_pSampleEditor;
    QLabel         *m_pFeedback;
    QLabel         *m_pPreview;
    UIMd3Button    *m_pApplyButton;
    bool            m_fCleared;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3RegexBuilder_h */
