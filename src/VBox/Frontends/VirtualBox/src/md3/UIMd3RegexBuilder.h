/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 anchored regular-expression builder.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, in version 3 of the License.
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
#include <QRegularExpression>
#include <QStringList>

/* Forward declarations: */
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTimer;
class QTreeWidget;

/** Non-blocking regex builder owned by one search field. */
class UIMd3RegexBuilder : public QDialog
{
    Q_OBJECT;

signals:

    /** Emitted when the user accepts a valid regex pattern. */
    void sigPatternAccepted(const QString &strPattern, const QString &strFlags);
    /** Emitted when the user explicitly returns to plain-text mode. */
    void sigPlainTextRequested();

public:

    /** Constructs a builder anchored to @a pParent. */
    explicit UIMd3RegexBuilder(QWidget *pParent = 0);

    /** Defines the initial pattern and flags. */
    void setPattern(const QString &strPattern, const QString &strFlags);

private slots:

    /** Debounces preview evaluation after an editor change. */
    void sltSchedulePreview();
    /** Updates validation and the bounded sample match count. */
    void sltUpdatePreview();
    /** Marks the current worker preview as over budget without blocking the UI. */
    void sltPreviewTimedOut();
    /** Accepts the current pattern when valid. */
    void sltAcceptPattern();
    /** Returns the owning field to plain-text mode. */
    void sltUsePlainText();
    /** Copies the raw expression to the clipboard. */
    void sltCopyPattern();
    /** Exports the expression metadata as bounded UTF-8 JSON. */
    void sltExportPattern();

private:

    /** Creates the builder controls. */
    void prepare();
    /** Converts the flags field to Qt options. */
    QRegularExpression::PatternOptions patternOptions() const;
    /** Inserts a guided construct around the selected pattern text. */
    void insertConstruct(const QString &strPrefix, const QString &strSuffix = QString());
    /** Inserts a suffix construct after the selected pattern text. */
    void insertSuffix(const QString &strSuffix);
    /** Inserts the escaped literal editor value at the pattern cursor. */
    void insertLiteral();
    /** Applies one still-current worker result on the UI thread. */
    void applyPreview(int iGeneration, const QStringList &rows,
                      int cMatches, qint64 cElapsedMilliseconds,
                      bool fTruncated, bool fTimedOut);

    QLineEdit *m_pPattern;
    QLineEdit *m_pFlags;
    QLineEdit *m_pLiteral;
    QPlainTextEdit *m_pSample;
    QCheckBox *m_pRegexMode;
    QLabel *m_pStatus;
    QTreeWidget *m_pMatches;
    QTimer *m_pPreviewTimer;
    QTimer *m_pPreviewTimeout;
    int m_iPreviewGeneration;
    bool m_fPreviewPending;
    bool m_fPreviewTimedOut;
    bool m_fWorkerRunning;
    bool m_fPreviewRerunRequested;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3RegexBuilder_h */
