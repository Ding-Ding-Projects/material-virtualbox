/* $Id$ */
/** @file
 * VBox Qt GUI - anchored Material 3 element appearance editor.
 */

#include <QComboBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPoint>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QApplication>
#include <QAccessible>
#include <QScreen>
#include <QSpinBox>
#include <QVBoxLayout>

#include "UIMd3AppearanceEditor.h"
#include "UIMd3Button.h"
#include "UIMd3Language.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

namespace
{
    class UIMd3AppearanceDialog : public QDialog
    {
        enum ThemeStatusKind
        {
            ThemeStatus_None,
            ThemeStatus_NameRequired,
            ThemeStatus_SaveFailed,
            ThemeStatus_Saved,
            ThemeStatus_ApplyFailed,
            ThemeStatus_Applied,
            ThemeStatus_SeedInvalid,
            ThemeStatus_AppearanceFailed
        };

    public:
        UIMd3AppearanceDialog(QWidget *pAnchor, const QString &strKey)
            : QDialog(pAnchor, Qt::Tool)
            , m_pAnchor(pAnchor)
            , m_strKey(strKey)
            , m_pSeed(new QLineEdit(this))
            , m_pFont(new QComboBox(this))
            , m_pRadius(new QSpinBox(this))
            , m_pScale(new QDoubleSpinBox(this))
            , m_pWeight(new QComboBox(this))
            , m_pNamedTheme(new QComboBox(this))
            , m_pThemeName(new QLineEdit(this))
            , m_pSaveTheme(0)
            , m_pApplyTheme(0)
            , m_pPreview(new QLabel(this))
            , m_pThemeStatus(new QLabel(this))
            , m_enmThemeStatus(ThemeStatus_None)
            , m_fThemeStatusError(false)
            , m_pSearch(new UIMd3SearchField(QStringLiteral("appearance-editor"),
                                              tr("Search appearance properties"), this))
            , m_pForm(new QFormLayout)
        {
            setAttribute(Qt::WA_DeleteOnClose);
            setWindowTitle(tr("Edit appearance"));
            setAccessibleName(tr("Appearance editor for %1").arg(strKey));

            m_pSeed->setAccessibleName(tr("Accent seed colour"));
            m_pSeed->setPlaceholderText(QStringLiteral("#6750A4"));
            m_pSeed->setToolTip(tr("Enter a HEX colour used by this element."));
            m_pSeed->setAccessibleDescription(m_pSeed->toolTip());

            m_pFont->setAccessibleName(tr("Typeface"));
            m_pFont->setToolTip(tr("Choose an installed typeface for this element; inherited keeps the theme family."));
            m_pFont->setAccessibleDescription(m_pFont->toolTip());
            m_pFont->addItem(tr("Inherited"), QString());
            const QStringList families = QFontDatabase::families();
            for (QStringList::const_iterator it = families.constBegin(); it != families.constEnd(); ++it)
                m_pFont->addItem(*it, *it);

            m_pRadius->setRange(0, 999);
            m_pRadius->setSuffix(tr(" px"));
            m_pRadius->setAccessibleName(tr("Corner radius"));
            m_pRadius->setToolTip(tr("Set the element corner radius in pixels."));
            m_pRadius->setAccessibleDescription(m_pRadius->toolTip());

            m_pScale->setRange(0.50, 2.00);
            m_pScale->setSingleStep(0.05);
            m_pScale->setDecimals(2);
            m_pScale->setSuffix(QStringLiteral("×"));
            m_pScale->setAccessibleName(tr("Type scale"));
            m_pScale->setToolTip(tr("Scale this element's type from 0.50× to 2.00×."));
            m_pScale->setAccessibleDescription(m_pScale->toolTip());

            m_pWeight->setAccessibleName(tr("Font weight"));
            m_pWeight->setToolTip(tr("Choose the element font weight; inherited follows the theme."));
            m_pWeight->setAccessibleDescription(m_pWeight->toolTip());
            m_pWeight->addItem(tr("Inherited"), -1);
            m_pWeight->addItem(tr("Light"), QFont::Light);
            m_pWeight->addItem(tr("Normal"), QFont::Normal);
            m_pWeight->addItem(tr("Medium"), QFont::Medium);
            m_pWeight->addItem(tr("Demi bold"), QFont::DemiBold);
            m_pWeight->addItem(tr("Bold"), QFont::Bold);

            m_pNamedTheme->setAccessibleName(tr("Saved named theme"));
            m_pNamedTheme->setToolTip(tr("Choose a previously saved complete appearance theme."));
            m_pNamedTheme->setAccessibleDescription(m_pNamedTheme->toolTip());
            m_pThemeName->setAccessibleName(tr("Theme name"));
            m_pThemeName->setPlaceholderText(tr("Name this theme"));
            m_pThemeName->setMaxLength(80);
            m_pThemeName->setToolTip(tr("Use a short name for the current persisted appearance."));
            m_pThemeName->setAccessibleDescription(m_pThemeName->toolTip());
            m_pThemeStatus->setAccessibleName(tr("Named theme status"));
            m_pThemeStatus->setWordWrap(true);

            m_pPreview->setAccessibleName(tr("Live appearance preview"));
            m_pPreview->setMinimumHeight(56);
            m_pPreview->setWordWrap(true);
            m_pPreview->setAlignment(Qt::AlignCenter);
            m_pPreview->setText(tr("Material 3 preview"));

            m_pForm->addRow(tr("Seed"), m_pSeed);
            m_pForm->addRow(tr("Typeface"), m_pFont);
            m_pForm->addRow(tr("Radius"), m_pRadius);
            m_pForm->addRow(tr("Scale"), m_pScale);
            m_pForm->addRow(tr("Weight"), m_pWeight);
            m_pForm->addRow(tr("Saved theme"), m_pNamedTheme);
            m_pForm->addRow(tr("Theme name"), m_pThemeName);

            m_pSaveTheme = new UIMd3Button(tr("Save named theme"), UIMd3ButtonVariant_Text, this);
            m_pSaveTheme->setAccessibleName(tr("Save named theme"));
            m_pSaveTheme->setAccessibleDescription(tr("Save the current complete appearance as a named theme."));
            m_pSaveTheme->setAppearanceKey(QStringLiteral("appearance-editor/save-theme"));
            m_pApplyTheme = new UIMd3Button(tr("Apply named theme"), UIMd3ButtonVariant_Text, this);
            m_pApplyTheme->setAccessibleName(tr("Apply named theme"));
            m_pApplyTheme->setAccessibleDescription(tr("Apply the selected complete appearance theme."));
            m_pApplyTheme->setAppearanceKey(QStringLiteral("appearance-editor/apply-theme"));
            m_pThemeActions = new QHBoxLayout;
            m_pThemeActions->setContentsMargins(0, 0, 0, 0);
            m_pThemeActions->addWidget(m_pSaveTheme);
            m_pThemeActions->addWidget(m_pApplyTheme);
            m_pForm->addRow(QString(), m_pThemeActions);

            m_pReset = new UIMd3Button(tr("Reset element"), UIMd3ButtonVariant_Text, this);
            m_pReset->setAccessibleName(tr("Reset this element appearance"));
            m_pReset->setAccessibleDescription(tr("Remove this element override and return to the theme value."));
            m_pReset->setAppearanceKey(QStringLiteral("appearance-editor/reset"));
            connect(m_pReset, &UIMd3Button::sigClicked, this, [this]()
            {
                md3Theme().clearAppearance(m_strKey);
                close();
            });

            m_pApply = new UIMd3Button(tr("Apply"), UIMd3ButtonVariant_Filled, this);
            m_pApply->setAccessibleName(tr("Apply element appearance"));
            m_pApply->setAccessibleDescription(tr("Validate and persist this element appearance."));
            m_pApply->setAppearanceKey(QStringLiteral("appearance-editor/apply"));
            connect(m_pApply, &UIMd3Button::sigClicked, this, [this]() { apply(); });

            m_pCancel = new UIMd3Button(tr("Cancel"), UIMd3ButtonVariant_Text, this);
            m_pCancel->setAccessibleName(tr("Cancel appearance editing"));
            m_pCancel->setAccessibleDescription(tr("Close without changing this element."));
            m_pCancel->setAppearanceKey(QStringLiteral("appearance-editor/cancel"));
            connect(m_pCancel, &UIMd3Button::sigClicked, this, &QDialog::close);

            QHBoxLayout *pActions = new QHBoxLayout;
            pActions->addWidget(m_pReset);
            pActions->addStretch(1);
            pActions->addWidget(m_pCancel);
            pActions->addWidget(m_pApply);

            QWidget *pContent = new QWidget;
            QVBoxLayout *pLayout = new QVBoxLayout(pContent);
            pLayout->setContentsMargins(16, 16, 16, 16);
            m_pIntro = new QLabel(tr("Changes apply to this element and persist across restarts."), this);
            pLayout->addWidget(m_pIntro);
            pLayout->addWidget(m_pSearch);
            pLayout->addWidget(m_pPreview);
            pLayout->addWidget(m_pThemeStatus);
            pLayout->addLayout(m_pForm);
            pLayout->addLayout(pActions);
            pContent->setLayout(pLayout);
            QScrollArea *pScrollArea = new QScrollArea(this);
            pScrollArea->setFrameShape(QFrame::NoFrame);
            pScrollArea->setWidgetResizable(true);
            pScrollArea->setAccessibleName(tr("Appearance editor content"));
            pScrollArea->setWidget(pContent);
            QVBoxLayout *pOuterLayout = new QVBoxLayout(this);
            pOuterLayout->setContentsMargins(0, 0, 0, 0);
            pOuterLayout->addWidget(pScrollArea);
            setLayout(pOuterLayout);

            connect(m_pSearch, &UIMd3SearchField::sigFilterChanged,
                    this, [this]() { filterControls(); });

            connect(m_pSeed, &QLineEdit::textChanged, this, [this]() { preview(); });
            connect(m_pFont, &QComboBox::currentTextChanged, this, [this]() { preview(); });
            connect(m_pRadius, qOverload<int>(&QSpinBox::valueChanged), this, [this]() { preview(); });
            connect(m_pScale, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() { preview(); });
            connect(m_pWeight, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() { preview(); });
            connect(m_pNamedTheme, qOverload<int>(&QComboBox::currentIndexChanged), this,
                    [this]() { updateThemeActions(); });
            connect(m_pThemeName, &QLineEdit::textChanged, this,
                    [this]() { updateThemeActions(); });
            connect(m_pSaveTheme, &UIMd3Button::sigClicked, this,
                    [this]() { saveNamedTheme(); });
            connect(m_pApplyTheme, &UIMd3Button::sigClicked, this,
                    [this]() { applyNamedTheme(); });
            if (UIMd3Language::instance())
                connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                        this, [this]() { retranslateUi(); });

            refreshNamedThemes();
            loadAppearance();
            resize(420, sizeHint().height());
            preview();
            filterControls();
            updateThemeActions();
        }

        void focusInitialControl()
        {
            if (m_pSearch)
                m_pSearch->setFocus(Qt::OtherFocusReason);
        }

    protected:
        virtual void closeEvent(QCloseEvent *pEvent) RT_OVERRIDE
        {
            if (m_pAnchor)
                m_pAnchor->setFocus(Qt::OtherFocusReason);
            QDialog::closeEvent(pEvent);
        }

    private:
        void refreshNamedThemes(const QString &strSelected = QString())
        {
            const QSignalBlocker blocker(m_pNamedTheme);
            m_pNamedTheme->clear();
            m_pNamedTheme->addItem(tr("Select saved theme"), QString());
            QStringList names = md3Theme().namedThemes();
            names.sort(Qt::CaseInsensitive);
            for (QStringList::const_iterator it = names.constBegin(); it != names.constEnd(); ++it)
                m_pNamedTheme->addItem(*it, *it);
            const int iSelected = strSelected.isEmpty()
                                 ? 0
                                 : m_pNamedTheme->findData(strSelected);
            m_pNamedTheme->setCurrentIndex(iSelected >= 0 ? iSelected : 0);
        }

        void retranslateUi()
        {
            setWindowTitle(tr("Edit appearance"));
            setAccessibleName(tr("Appearance editor for %1").arg(m_strKey));
            m_pIntro->setText(tr("Changes apply to this element and persist across restarts."));
            m_pSearch->setPlaceholderText(tr("Search appearance properties"));
            m_pSeed->setAccessibleName(tr("Accent seed colour"));
            m_pSeed->setToolTip(tr("Enter a HEX colour used by this element."));
            m_pSeed->setAccessibleDescription(m_pSeed->toolTip());
            m_pFont->setAccessibleName(tr("Typeface"));
            m_pFont->setToolTip(tr("Choose an installed typeface; inherited follows the theme family."));
            m_pFont->setAccessibleDescription(m_pFont->toolTip());
            m_pFont->setItemText(0, tr("Inherited"));
            m_pRadius->setSuffix(tr(" px"));
            m_pRadius->setAccessibleName(tr("Corner radius"));
            m_pRadius->setToolTip(tr("Set the corner radius for this element in pixels."));
            m_pRadius->setAccessibleDescription(m_pRadius->toolTip());
            m_pScale->setAccessibleName(tr("Type scale"));
            m_pScale->setToolTip(tr("Scale this element's type from 0.50× to 2.00×."));
            m_pScale->setAccessibleDescription(m_pScale->toolTip());
            m_pWeight->setAccessibleName(tr("Font weight"));
            m_pWeight->setToolTip(tr("Choose the element font weight; inherited follows the theme."));
            m_pWeight->setAccessibleDescription(m_pWeight->toolTip());
            m_pWeight->setItemText(0, tr("Inherited"));
            m_pWeight->setItemText(1, tr("Light"));
            m_pWeight->setItemText(2, tr("Normal"));
            m_pWeight->setItemText(3, tr("Medium"));
            m_pWeight->setItemText(4, tr("Demi bold"));
            m_pWeight->setItemText(5, tr("Bold"));
            m_pNamedTheme->setAccessibleName(tr("Saved named theme"));
            m_pNamedTheme->setToolTip(tr("Choose a previously saved complete appearance theme."));
            m_pNamedTheme->setAccessibleDescription(m_pNamedTheme->toolTip());
            m_pThemeName->setAccessibleName(tr("Theme name"));
            m_pThemeName->setPlaceholderText(tr("Name this theme"));
            m_pThemeName->setToolTip(tr("Use a short name for the current persisted appearance."));
            m_pThemeName->setAccessibleDescription(m_pThemeName->toolTip());
            m_pThemeStatus->setAccessibleName(tr("Named theme status"));
            m_pPreview->setAccessibleName(tr("Live appearance preview"));
            m_pPreview->setText(tr("Material 3 preview"));
            m_pSaveTheme->setText(tr("Save named theme"));
            m_pSaveTheme->setAccessibleName(tr("Save named theme"));
            m_pSaveTheme->setAppearanceKey(QStringLiteral("appearance-editor/save-theme"));
            m_pApplyTheme->setText(tr("Apply named theme"));
            m_pApplyTheme->setAccessibleName(tr("Apply named theme"));
            m_pApplyTheme->setAppearanceKey(QStringLiteral("appearance-editor/apply-theme"));
            m_pReset->setText(tr("Reset element"));
            m_pReset->setAccessibleName(tr("Reset this element appearance"));
            m_pReset->setAppearanceKey(QStringLiteral("appearance-editor/reset"));
            m_pApply->setText(tr("Apply"));
            m_pApply->setAccessibleName(tr("Apply element appearance"));
            m_pApply->setAppearanceKey(QStringLiteral("appearance-editor/apply"));
            m_pCancel->setText(tr("Cancel"));
            m_pCancel->setAccessibleName(tr("Cancel appearance editing"));
            m_pCancel->setAccessibleDescription(tr("Close without changing this element."));
            m_pCancel->setAppearanceKey(QStringLiteral("appearance-editor/cancel"));
            for (int i = 0; i < m_pForm->rowCount(); ++i)
            {
                QLabel *pLabel = qobject_cast<QLabel *>(m_pForm->itemAt(i, QFormLayout::LabelRole)
                                                          ? m_pForm->itemAt(i, QFormLayout::LabelRole)->widget() : 0);
                if (!pLabel)
                    continue;
                switch (i)
                {
                    case 0: pLabel->setText(tr("Seed")); break;
                    case 1: pLabel->setText(tr("Typeface")); break;
                    case 2: pLabel->setText(tr("Radius")); break;
                    case 3: pLabel->setText(tr("Scale")); break;
                    case 4: pLabel->setText(tr("Weight")); break;
                    case 5: pLabel->setText(tr("Saved theme")); break;
                    case 6: pLabel->setText(tr("Theme name")); break;
                    default: break;
                }
            }
            reannounceThemeStatus();
            refreshNamedThemes(m_pNamedTheme->currentData().toString());
            preview();
            filterControls();
            updateThemeActions();
        }

        void loadAppearance()
        {
            const QSignalBlocker seedBlocker(m_pSeed);
            const QSignalBlocker fontBlocker(m_pFont);
            const QSignalBlocker radiusBlocker(m_pRadius);
            const QSignalBlocker scaleBlocker(m_pScale);
            const QSignalBlocker weightBlocker(m_pWeight);
            const UIMd3Appearance appearance = md3Theme().appearance(m_strKey);
            if (appearance.fValid)
            {
                m_pSeed->setText(appearance.seed.isValid() ? appearance.seed.name(QColor::HexArgb) : QString());
                m_pFont->setCurrentIndex(appearance.strFont.isEmpty() ? 0 : m_pFont->findData(appearance.strFont));
                m_pRadius->setValue(appearance.iRadius);
                m_pScale->setValue(appearance.dScale);
                m_pWeight->setCurrentIndex(m_pWeight->findData(appearance.iWeight));
            }
            else
            {
                m_pSeed->clear();
                m_pFont->setCurrentIndex(0);
                m_pRadius->setValue(UIMd3Shape::Large);
                m_pScale->setValue(1.0);
                m_pWeight->setCurrentIndex(0);
            }
        }

        QString themeStatusText() const
        {
            switch (m_enmThemeStatus)
            {
                case ThemeStatus_NameRequired:
                    return tr("Enter a name before saving the current theme.");
                case ThemeStatus_SaveFailed:
                    return tr("The theme was not saved: the name, saved-theme limit, or 512 KiB theme-data limit was rejected.");
                case ThemeStatus_Saved:
                    return tr("Saved the current persisted appearance as %1.").arg(m_strThemeStatusArgument);
                case ThemeStatus_ApplyFailed:
                    return tr("The named theme %1 is unavailable, malformed, or exceeds the 512 KiB history limit.").arg(m_strThemeStatusArgument);
                case ThemeStatus_Applied:
                    return tr("Applied named theme %1.").arg(m_strThemeStatusArgument);
                case ThemeStatus_SeedInvalid:
                    return tr("Seed must be a six- or eight-digit HEX colour, for example #6750A4.");
                case ThemeStatus_AppearanceFailed:
                    return tr("The appearance was not saved: a value or the 512 KiB validated-history limit was rejected.");
                default:
                    return QString();
            }
        }

        void reannounceThemeStatus()
        {
            const QString strStatus = themeStatusText();
            if (strStatus.isEmpty())
                return;
            m_pThemeStatus->setText(strStatus);
            m_pThemeStatus->setToolTip(strStatus);
            m_pThemeStatus->setAccessibleDescription(m_fThemeStatusError
                                                      ? tr("Named theme operation failed: %1").arg(strStatus)
                                                      : strStatus);
            if (QAccessible::isActive())
            {
                QAccessibleAnnouncementEvent event(m_pThemeStatus, strStatus);
                QAccessible::updateAccessibility(&event);
            }
        }

        void setThemeStatus(ThemeStatusKind enmStatus, const QString &strArgument = QString(),
                            bool fError = false)
        {
            m_enmThemeStatus = enmStatus;
            m_strThemeStatusArgument = strArgument;
            m_fThemeStatusError = fError;
            reannounceThemeStatus();
        }

        void updateThemeActions()
        {
            const bool fHasName = !m_pThemeName->text().trimmed().isEmpty();
            const bool fHasSelection = m_pNamedTheme->currentIndex() > 0
                                    && !m_pNamedTheme->currentData().toString().isEmpty();
            m_pSaveTheme->setEnabled(fHasName);
            m_pSaveTheme->setAccessibleDescription(fHasName
                                                   ? tr("Save the current persisted appearance under this name.")
                                                   : tr("Enter a theme name before saving."));
            m_pApplyTheme->setEnabled(fHasSelection);
            m_pApplyTheme->setAccessibleDescription(fHasSelection
                                                   ? tr("Apply the selected complete appearance theme.")
                                                   : tr("Select a saved named theme before applying it."));
        }

        void saveNamedTheme()
        {
            const QString strName = m_pThemeName->text().trimmed();
            if (strName.isEmpty())
            {
                setThemeStatus(ThemeStatus_NameRequired, QString(), true);
                m_pThemeName->setFocus(Qt::OtherFocusReason);
                return;
            }
            if (!md3Theme().saveNamedTheme(strName))
            {
                setThemeStatus(ThemeStatus_SaveFailed, QString(), true);
                m_pThemeName->setFocus(Qt::OtherFocusReason);
                return;
            }
            refreshNamedThemes(strName);
            setThemeStatus(ThemeStatus_Saved, strName);
        }

        void applyNamedTheme()
        {
            const QString strName = m_pNamedTheme->currentData().toString();
            if (strName.isEmpty() || !md3Theme().applyNamedTheme(strName))
            {
                setThemeStatus(ThemeStatus_ApplyFailed, strName, true);
                m_pNamedTheme->setFocus(Qt::OtherFocusReason);
                return;
            }
            loadAppearance();
            preview();
            setThemeStatus(ThemeStatus_Applied, strName);
        }

        bool collectAppearance(UIMd3Appearance &appearance)
        {
            const QString strSeed = m_pSeed->text().trimmed();
            static const QRegularExpression reHex(QStringLiteral("^#[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$"));
            if (!strSeed.isEmpty() && !reHex.match(strSeed).hasMatch())
            {
                setThemeStatus(ThemeStatus_SeedInvalid, QString(), true);
                m_pSeed->setFocus(Qt::OtherFocusReason);
                return false;
            }
            appearance.fValid = true;
            appearance.seed = strSeed.isEmpty() ? QColor() : QColor(strSeed);
            appearance.strFont = m_pFont->currentData().toString();
            appearance.iRadius = m_pRadius->value();
            appearance.dScale = m_pScale->value();
            appearance.iWeight = m_pWeight->currentData().toInt();
            return true;
        }

        void filterControls()
        {
            for (int i = 0; i < m_pForm->rowCount(); ++i)
            {
                QLabel *pLabel = qobject_cast<QLabel *>(m_pForm->itemAt(i, QFormLayout::LabelRole)
                                                          ? m_pForm->itemAt(i, QFormLayout::LabelRole)->widget() : 0);
                QWidget *pField = m_pForm->itemAt(i, QFormLayout::FieldRole)
                                ? m_pForm->itemAt(i, QFormLayout::FieldRole)->widget() : 0;
                QStringList candidates;
                if (pLabel)
                    candidates << pLabel->text();
                if (pField)
                {
                    candidates << pField->accessibleName() << pField->accessibleDescription() << pField->toolTip();
                    if (QComboBox *pCombo = qobject_cast<QComboBox *>(pField))
                        candidates << pCombo->currentText();
                    else if (QLineEdit *pLineEdit = qobject_cast<QLineEdit *>(pField))
                        candidates << pLineEdit->text();
                    else if (QSpinBox *pSpinBox = qobject_cast<QSpinBox *>(pField))
                        candidates << pSpinBox->text() << QString::number(pSpinBox->value());
                    else if (QDoubleSpinBox *pDoubleSpinBox = qobject_cast<QDoubleSpinBox *>(pField))
                        candidates << pDoubleSpinBox->text()
                                   << QString::number(pDoubleSpinBox->value(), 'f', 2);
                }
                const bool fVisible = candidates.isEmpty() || m_pSearch->matches(candidates.join(QLatin1Char(' ')));
                if (pLabel)
                    pLabel->setVisible(fVisible);
                if (pField)
                    pField->setVisible(fVisible);
            }
            const bool fSaveVisible = m_pSearch->matches(m_pSaveTheme->text() + QLatin1Char(' ')
                                                       + m_pSaveTheme->accessibleName()
                                                       + QLatin1Char(' ') + m_pSaveTheme->accessibleDescription());
            const bool fApplyVisible = m_pSearch->matches(m_pApplyTheme->text() + QLatin1Char(' ')
                                                        + m_pApplyTheme->accessibleName()
                                                        + QLatin1Char(' ') + m_pApplyTheme->accessibleDescription());
            m_pSaveTheme->setVisible(fSaveVisible);
            m_pApplyTheme->setVisible(fApplyVisible);
        }

        void preview()
        {
            const QColor seed(m_pSeed->text().trimmed());
            QPalette palette = m_pPreview->palette();
            const QColor background = seed.isValid() ? seed : md3(UIMd3ColorRole_SecondaryContainer);
            const QColor foreground = background.lightnessF() > 0.55
                                    ? md3(UIMd3ColorRole_OnSurface)
                                    : md3(UIMd3ColorRole_OnPrimary);
            palette.setColor(QPalette::Window, background);
            palette.setColor(QPalette::WindowText, foreground);
            m_pPreview->setAutoFillBackground(true);
            m_pPreview->setPalette(palette);
            QFont font = md3Theme().font(UIMd3TypeRole_BodyLarge);
            if (!m_pFont->currentData().toString().isEmpty())
                font.setFamily(m_pFont->currentData().toString());
            font.setPixelSize(qMax(9, qRound(font.pixelSize() * m_pScale->value())));
            if (m_pWeight->currentData().toInt() >= static_cast<int>(QFont::Thin))
                font.setWeight(static_cast<QFont::Weight>(m_pWeight->currentData().toInt()));
            m_pPreview->setFont(font);
            m_pPreview->setText(tr("Material 3 preview · %1").arg(m_strKey));
        }

        void apply()
        {
            UIMd3Appearance appearance;
            if (!collectAppearance(appearance))
                return;
            if (!md3Theme().setAppearance(m_strKey, appearance))
            {
                setThemeStatus(ThemeStatus_AppearanceFailed, QString(), true);
                return;
            }
            close();
        }

        QWidget      *m_pAnchor;
        QString        m_strKey;
        QLineEdit     *m_pSeed;
        QComboBox     *m_pFont;
        QSpinBox      *m_pRadius;
        QDoubleSpinBox*m_pScale;
        QComboBox     *m_pWeight;
        QComboBox     *m_pNamedTheme;
        QLineEdit     *m_pThemeName;
        UIMd3Button    *m_pSaveTheme;
        UIMd3Button    *m_pApplyTheme;
        UIMd3Button    *m_pReset;
        UIMd3Button    *m_pApply;
        UIMd3Button    *m_pCancel;
        QLabel        *m_pIntro;
        QLabel        *m_pPreview;
        QLabel        *m_pThemeStatus;
        ThemeStatusKind m_enmThemeStatus;
        bool            m_fThemeStatusError;
        QString         m_strThemeStatusArgument;
        UIMd3SearchField *m_pSearch;
        QFormLayout  *m_pForm;
        QHBoxLayout  *m_pThemeActions;
    };
}

void UIMd3AppearanceEditor::open(QWidget *pAnchor, const QString &strAppearanceKey)
{
    if (!pAnchor || strAppearanceKey.isEmpty() || strAppearanceKey.size() > 80)
        return;
    UIMd3AppearanceDialog *pDialog = new UIMd3AppearanceDialog(pAnchor, strAppearanceKey);
    pDialog->adjustSize();
    const QPoint point = pAnchor->mapToGlobal(QPoint(pAnchor->width() + 8, 0));
    QScreen *pScreen = pAnchor->screen();
    if (!pScreen)
        pScreen = QApplication::screenAt(point);
    const QRect available = pScreen ? pScreen->availableGeometry()
                                    : (QApplication::primaryScreen()
                                     ? QApplication::primaryScreen()->availableGeometry()
                                     : QRect(0, 0, 1280, 720));
    QSize size = pDialog->size();
    size.setWidth(qMin(size.width(), available.width()));
    size.setHeight(qMin(size.height(), available.height()));
    pDialog->resize(size);
    int iX = point.x();
    int iY = point.y();
    if (iX + size.width() > available.right() + 1)
        iX = pAnchor->mapToGlobal(QPoint(-size.width() - 8, 0)).x();
    if (iY + size.height() > available.bottom() + 1)
        iY = pAnchor->mapToGlobal(QPoint(0, pAnchor->height() - size.height())).y();
    iX = qBound(available.left(), iX, available.right() - size.width() + 1);
    iY = qBound(available.top(), iY, available.bottom() - size.height() + 1);
    pDialog->move(iX, iY);
    pDialog->show();
    pDialog->raise();
    pDialog->activateWindow();
    pDialog->focusInitialControl();
}
