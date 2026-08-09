/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 anchored regular-expression builder.
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3RegexBuilder_h
#define FEQT_INCLUDED_SRC_md3_UIMd3RegexBuilder_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QDialog>
#include <QRegularExpression>

/* Forward declarations: */
class QCheckBox;
class QLabel;
class QLineEdit;

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

    /** Updates validation and the bounded sample match count. */
    void sltUpdatePreview();
    /** Accepts the current pattern when valid. */
    void sltAcceptPattern();
    /** Returns the owning field to plain-text mode. */
    void sltUsePlainText();

private:

    /** Creates the builder controls. */
    void prepare();
    /** Converts the flags field to Qt options. */
    QRegularExpression::PatternOptions patternOptions() const;

    QLineEdit *m_pPattern;
    QLineEdit *m_pFlags;
    QLineEdit *m_pSample;
    QCheckBox *m_pRegexMode;
    QLabel *m_pStatus;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3RegexBuilder_h */
