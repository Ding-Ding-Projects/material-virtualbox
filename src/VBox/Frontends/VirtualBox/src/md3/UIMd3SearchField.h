/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 search field with an anchored regex builder.
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3SearchField_h
#define FEQT_INCLUDED_SRC_md3_UIMd3SearchField_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QRegularExpression>
#include <QPointer>
#include <QString>
#include <QWidget>

class QLineEdit;
class QToolButton;
class UIMd3RegexBuilder;

/** Plain-text-first search field with independent regex state. */
class UIMd3SearchField : public QWidget
{
    Q_OBJECT;

signals:

    /** Notifies listeners that query or regex state changed. */
    void sigFilterChanged();

public:

    /** Constructs a field with a stable @a strFieldId and placeholder. */
    UIMd3SearchField(const QString &strFieldId, const QString &strPlaceholder, QWidget *pParent = 0);

    /** Returns the plain query text. */
    QString text() const;
    /** Defines the query text. */
    void setText(const QString &strText);
    /** Defines the translated placeholder shown in the field. */
    void setPlaceholderText(const QString &strText);
    /** Returns whether regex mode is active. */
    bool isRegexActive() const { return m_fRegexActive; }
    /** Returns the active regex, invalid when plain-text mode is active. */
    QRegularExpression regex() const { return m_regex; }
    /** Applies a valid regex pattern and flags. */
    bool applyRegex(const QString &strPattern, const QString &strFlags);
    /** Returns to plain-text mode. */
    void clearRegex();
    /** Tests one candidate against the current independent filter. */
    bool matches(const QString &strCandidate) const;

private slots:

    /** Opens the anchored builder. */
    void sltOpenBuilder();
    /** Emits a filter change for plain-text edits. */
    void sltTextChanged(const QString &strText);

private:

    /** Creates the child controls. */
    void prepare();
    /** Converts flags to Qt options. */
    static QRegularExpression::PatternOptions patternOptions(const QString &strFlags);

    QString m_strFieldId;
    QString m_strPlaceholder;
    QLineEdit *m_pEditor;
    QToolButton *m_pBuilderButton;
    QPointer<UIMd3RegexBuilder> m_pBuilder;
    bool m_fRegexActive;
    QString m_strFlags;
    QRegularExpression m_regex;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3SearchField_h */
