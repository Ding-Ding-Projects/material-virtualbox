/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 anchored regular-expression builder.
 */

/* Qt includes: */
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3RegexBuilder.h"

static bool md3ValidateRegexFlags(const QString &strFlags, QString *pstrNormalized = 0)
{
    QString strResult;
    const QString strSupported = QStringLiteral("imsx");
    for (const QChar ch : strFlags.toLower().left(16))
    {
        if (ch.isSpace())
            continue;
        if (!strSupported.contains(ch) || strResult.contains(ch))
            return false;
        strResult += ch;
    }
    if (pstrNormalized)
        *pstrNormalized = strResult;
    return true;
}

UIMd3RegexBuilder::UIMd3RegexBuilder(QWidget *pParent)
    : QDialog(pParent)
    , m_pPattern(0)
    , m_pFlags(0)
    , m_pSample(0)
    , m_pRegexMode(0)
    , m_pStatus(0)
{
    setWindowTitle(tr("Regex builder"));
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, true);
    prepare();
}

void UIMd3RegexBuilder::setPattern(const QString &strPattern, const QString &strFlags)
{
    if (m_pPattern)
        m_pPattern->setText(strPattern);
    if (m_pFlags)
        m_pFlags->setText(strFlags);
    sltUpdatePreview();
}

QRegularExpression::PatternOptions UIMd3RegexBuilder::patternOptions() const
{
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    QString flags;
    md3ValidateRegexFlags(m_pFlags ? m_pFlags->text() : QString(), &flags);
    if (flags.contains('i')) options |= QRegularExpression::CaseInsensitiveOption;
    if (flags.contains('m')) options |= QRegularExpression::MultilineOption;
    if (flags.contains('s')) options |= QRegularExpression::DotMatchesEverythingOption;
    if (flags.contains('x')) options |= QRegularExpression::ExtendedPatternSyntaxOption;
    return options;
}

void UIMd3RegexBuilder::sltUpdatePreview()
{
    if (!m_pStatus || !m_pPattern || !m_pRegexMode)
        return;
    if (!m_pRegexMode->isChecked())
    {
        m_pStatus->setText(tr("Plain-text mode: special characters are literal."));
        return;
    }

    if (!md3ValidateRegexFlags(m_pFlags ? m_pFlags->text() : QString()))
    {
        m_pStatus->setText(tr("Invalid flags: use each of i, m, s, and x at most once."));
        return;
    }

    const QRegularExpression regex(m_pPattern->text(), patternOptions());
    if (!regex.isValid())
    {
        m_pStatus->setText(tr("Invalid pattern: %1").arg(regex.errorString()));
        return;
    }

    const QString sample = m_pSample ? m_pSample->text().left(4096) : QString();
    int cMatches = 0;
    QRegularExpressionMatchIterator iterator = regex.globalMatch(sample);
    while (iterator.hasNext() && cMatches < 256)
    {
        iterator.next();
        ++cMatches;
    }
    m_pStatus->setText(tr("Valid pattern · %1 match(es) in the sample").arg(cMatches));
}

void UIMd3RegexBuilder::sltAcceptPattern()
{
    if (!m_pRegexMode || !m_pRegexMode->isChecked() || !m_pPattern)
        return;
    QString strFlags;
    if (!md3ValidateRegexFlags(m_pFlags ? m_pFlags->text() : QString(), &strFlags))
    {
        sltUpdatePreview();
        return;
    }
    const QRegularExpression regex(m_pPattern->text(), patternOptions());
    if (!regex.isValid())
        return;
    emit sigPatternAccepted(m_pPattern->text(), strFlags);
    close();
}

void UIMd3RegexBuilder::sltUsePlainText()
{
    emit sigPlainTextRequested();
    close();
}

void UIMd3RegexBuilder::prepare()
{
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    QFormLayout *pForm = new QFormLayout;
    m_pPattern = new QLineEdit(this);
    m_pPattern->setAccessibleName(tr("Regular-expression pattern"));
    m_pPattern->setMaxLength(4096);
    pForm->addRow(tr("Pattern"), m_pPattern);
    m_pFlags = new QLineEdit(this);
    m_pFlags->setAccessibleName(tr("Regular-expression flags"));
    m_pFlags->setMaxLength(16);
    m_pFlags->setPlaceholderText(tr("i m s x"));
    pForm->addRow(tr("Flags"), m_pFlags);
    m_pSample = new QLineEdit(this);
    m_pSample->setAccessibleName(tr("Regex sample text"));
    m_pSample->setMaxLength(4096);
    pForm->addRow(tr("Sample"), m_pSample);
    pLayout->addLayout(pForm);

    m_pRegexMode = new QCheckBox(tr("Use regex"), this);
    m_pRegexMode->setChecked(true);
    m_pRegexMode->setAccessibleName(tr("Use regular-expression mode"));
    pLayout->addWidget(m_pRegexMode);
    m_pStatus = new QLabel(this);
    m_pStatus->setWordWrap(true);
    m_pStatus->setAccessibleName(tr("Regex validation status"));
    pLayout->addWidget(m_pStatus);

    QDialogButtonBox *pButtons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    QPushButton *pPlainText = pButtons->addButton(tr("Use plain text"), QDialogButtonBox::ActionRole);
    connect(pButtons, &QDialogButtonBox::clicked, this, [this, pButtons](QAbstractButton *pButton)
    {
        if (pButton == pButtons->button(QDialogButtonBox::Apply))
            sltAcceptPattern();
        else if (pButton == pButtons->button(QDialogButtonBox::Cancel))
            reject();
    });
    connect(pPlainText, &QPushButton::clicked, this, &UIMd3RegexBuilder::sltUsePlainText);
    pLayout->addWidget(pButtons);

    connect(m_pPattern, &QLineEdit::textChanged, this, &UIMd3RegexBuilder::sltUpdatePreview);
    connect(m_pFlags, &QLineEdit::textChanged, this, &UIMd3RegexBuilder::sltUpdatePreview);
    connect(m_pSample, &QLineEdit::textChanged, this, &UIMd3RegexBuilder::sltUpdatePreview);
    connect(m_pRegexMode, &QCheckBox::toggled, this, &UIMd3RegexBuilder::sltUpdatePreview);
    sltUpdatePreview();
}
