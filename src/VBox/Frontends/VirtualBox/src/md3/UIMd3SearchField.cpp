/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 search field with an anchored regex builder.
 */

/* Qt includes: */
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPoint>
#include <QToolButton>

/* GUI includes: */
#include "UIMd3RegexBuilder.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

UIMd3SearchField::UIMd3SearchField(const QString &strFieldId, const QString &strPlaceholder, QWidget *pParent)
    : QWidget(pParent)
    , m_strFieldId(strFieldId)
    , m_strPlaceholder(strPlaceholder)
    , m_pEditor(0)
    , m_pBuilderButton(0)
    , m_fRegexActive(false)
{
    setObjectName(QStringLiteral("md3SearchField_%1").arg(strFieldId));
    setAccessibleName(strPlaceholder);
    prepare();
}

QString UIMd3SearchField::text() const
{
    return m_pEditor ? m_pEditor->text() : QString();
}

void UIMd3SearchField::setText(const QString &strText)
{
    if (m_pEditor)
        m_pEditor->setText(strText);
}

void UIMd3SearchField::setPlaceholderText(const QString &strText)
{
    m_strPlaceholder = strText;
    if (m_pEditor)
    {
        m_pEditor->setPlaceholderText(strText);
        setAccessibleName(strText);
    }
}

QRegularExpression::PatternOptions UIMd3SearchField::patternOptions(const QString &strFlags)
{
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    const QString flags = strFlags.toLower();
    if (flags.contains('i')) options |= QRegularExpression::CaseInsensitiveOption;
    if (flags.contains('m')) options |= QRegularExpression::MultilineOption;
    if (flags.contains('s')) options |= QRegularExpression::DotMatchesEverythingOption;
    if (flags.contains('x')) options |= QRegularExpression::ExtendedPatternSyntaxOption;
    return options;
}

bool UIMd3SearchField::applyRegex(const QString &strPattern, const QString &strFlags)
{
    const QRegularExpression candidate(strPattern.left(4096), patternOptions(strFlags));
    if (!candidate.isValid())
        return false;
    m_regex = candidate;
    m_strFlags = strFlags;
    m_fRegexActive = true;
    emit sigFilterChanged();
    return true;
}

void UIMd3SearchField::clearRegex()
{
    m_fRegexActive = false;
    m_strFlags.clear();
    m_regex = QRegularExpression();
    emit sigFilterChanged();
}

bool UIMd3SearchField::matches(const QString &strCandidate) const
{
    if (m_fRegexActive && m_regex.isValid())
        return m_regex.match(strCandidate).hasMatch();
    return text().isEmpty() || strCandidate.contains(text(), Qt::CaseInsensitive);
}

void UIMd3SearchField::sltOpenBuilder()
{
    UIMd3RegexBuilder *pBuilder = new UIMd3RegexBuilder(this);
    if (!pBuilder)
        return;
    pBuilder->setPattern(m_fRegexActive ? m_regex.pattern() : text(), m_strFlags);
    connect(pBuilder, &UIMd3RegexBuilder::sigPatternAccepted,
            this, [this](const QString &strPattern, const QString &strFlags)
    {
        applyRegex(strPattern, strFlags);
    });
    connect(pBuilder, &UIMd3RegexBuilder::sigPlainTextRequested,
            this, &UIMd3SearchField::clearRegex);
    const QPoint point = m_pBuilderButton ? m_pBuilderButton->mapToGlobal(QPoint(0, m_pBuilderButton->height()))
                                           : mapToGlobal(QPoint(0, height()));
    pBuilder->move(point);
    pBuilder->show();
}

void UIMd3SearchField::sltTextChanged(const QString &)
{
    if (m_fRegexActive)
        clearRegex();
    else
        emit sigFilterChanged();
}

void UIMd3SearchField::prepare()
{
    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(12, 4, 4, 4);
    pLayout->setSpacing(6);
    m_pEditor = new QLineEdit(this);
    m_pEditor->setClearButtonEnabled(true);
    m_pEditor->setMaxLength(4096);
    m_pEditor->setPlaceholderText(m_strPlaceholder);
    pLayout->addWidget(m_pEditor, 1);
    m_pBuilderButton = new QToolButton(this);
    m_pBuilderButton->setText(QStringLiteral(".*"));
    m_pBuilderButton->setToolTip(tr("Open the regex builder for this search"));
    m_pBuilderButton->setAccessibleName(tr("Open regex builder"));
    m_pBuilderButton->setMinimumSize(QSize(48, md3Theme().controlHeight()));
    pLayout->addWidget(m_pBuilderButton);
    setMinimumHeight(md3Theme().controlHeight() + 8);
    connect(m_pEditor, &QLineEdit::textChanged, this, &UIMd3SearchField::sltTextChanged);
    connect(m_pBuilderButton, &QToolButton::clicked, this, &UIMd3SearchField::sltOpenBuilder);
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged, this, [this]()
    {
        if (m_pBuilderButton)
            m_pBuilderButton->setMinimumHeight(md3Theme().controlHeight());
    });
}
