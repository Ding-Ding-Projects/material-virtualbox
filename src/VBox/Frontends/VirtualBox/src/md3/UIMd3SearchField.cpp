/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 search field with an anchored regex builder.
 */

/* Qt includes: */
#include <QApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPoint>
#include <QScreen>
#include <QToolButton>

/* GUI includes: */
#include "UIMd3RegexBuilder.h"
#include "UIMd3SearchField.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"

UIMd3SearchField::UIMd3SearchField(const QString &strFieldId, const QString &strPlaceholder, QWidget *pParent)
    : QWidget(pParent)
    , m_strFieldId(strFieldId)
    , m_strPlaceholder(strPlaceholder)
    , m_pEditor(0)
    , m_pBuilderButton(0)
    , m_pBuilder(0)
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
    if (m_pBuilderButton)
    {
        m_pBuilderButton->setToolTip(tr("Open the regex builder for this search"));
        m_pBuilderButton->setAccessibleName(tr("Open regex builder"));
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
    if (m_pBuilder)
    {
        m_pBuilder->raise();
        m_pBuilder->activateWindow();
        return;
    }
    UIMd3RegexBuilder *pBuilder = new UIMd3RegexBuilder(this);
    if (!pBuilder)
        return;
    m_pBuilder = pBuilder;
    pBuilder->setPattern(m_fRegexActive ? m_regex.pattern() : text(), m_strFlags);
    connect(pBuilder, &UIMd3RegexBuilder::sigPatternAccepted,
            this, [this](const QString &strPattern, const QString &strFlags)
    {
        applyRegex(strPattern, strFlags);
    });
    connect(pBuilder, &UIMd3RegexBuilder::sigPlainTextRequested,
            this, &UIMd3SearchField::clearRegex);
    connect(pBuilder, &QObject::destroyed, this, [this]()
    {
        if (m_pEditor)
            m_pEditor->setFocus(Qt::OtherFocusReason);
    });
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                pBuilder, [this]()
    {
        if (m_pBuilder)
            m_pBuilder->close();
        if (m_pEditor)
            m_pEditor->setFocus(Qt::OtherFocusReason);
    });
    pBuilder->adjustSize();
    const QPoint point = m_pBuilderButton ? m_pBuilderButton->mapToGlobal(QPoint(0, m_pBuilderButton->height()))
                                           : mapToGlobal(QPoint(0, height()));
    QScreen *pScreen = window() ? window()->screen() : 0;
    if (!pScreen)
        pScreen = QApplication::screenAt(point);
    const QRect available = pScreen ? pScreen->availableGeometry()
                                    : (QApplication::primaryScreen()
                                     ? QApplication::primaryScreen()->availableGeometry()
                                     : QRect(0, 0, 1280, 720));
    QSize size = pBuilder->size();
    size.setWidth(qMin(size.width(), available.width()));
    size.setHeight(qMin(size.height(), available.height()));
    pBuilder->resize(size);
    int iX = point.x();
    int iY = point.y();
    if (iX + size.width() > available.right() + 1)
        iX = point.x() - size.width();
    if (iY + size.height() > available.bottom() + 1)
        iY = point.y() - size.height() - (m_pBuilderButton ? m_pBuilderButton->height() : height());
    iX = qBound(available.left(), iX, available.right() - size.width() + 1);
    iY = qBound(available.top(), iY, available.bottom() - size.height() + 1);
    pBuilder->move(iX, iY);
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
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(m_pEditor);
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
