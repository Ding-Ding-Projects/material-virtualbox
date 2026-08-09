/* $Id$ */
/** @file
 * VBox Qt GUI - anchored Material 3 element appearance editor.
 */

#include <QComboBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPoint>
#include <QSpinBox>
#include <QVBoxLayout>

#include "UIMd3AppearanceEditor.h"
#include "UIMd3Button.h"
#include "UIMd3Theme.h"

namespace
{
    class UIMd3AppearanceDialog : public QDialog
    {
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
            , m_pPreview(new QLabel(this))
        {
            setAttribute(Qt::WA_DeleteOnClose);
            setWindowTitle(tr("Edit appearance"));
            setAccessibleName(tr("Appearance editor for %1").arg(strKey));

            m_pSeed->setAccessibleName(tr("Accent seed colour"));
            m_pSeed->setPlaceholderText(QStringLiteral("#6750A4"));
            m_pSeed->setToolTip(tr("Enter a HEX colour used by this element."));

            m_pFont->setAccessibleName(tr("Typeface"));
            m_pFont->addItem(tr("Inherited"), QString());
            const QStringList families = QFontDatabase::families();
            for (QStringList::const_iterator it = families.constBegin(); it != families.constEnd(); ++it)
                m_pFont->addItem(*it, *it);

            m_pRadius->setRange(0, 999);
            m_pRadius->setSuffix(tr(" px"));
            m_pRadius->setAccessibleName(tr("Corner radius"));

            m_pScale->setRange(0.50, 2.00);
            m_pScale->setSingleStep(0.05);
            m_pScale->setDecimals(2);
            m_pScale->setSuffix(QStringLiteral("×"));
            m_pScale->setAccessibleName(tr("Type scale"));

            m_pWeight->setAccessibleName(tr("Font weight"));
            m_pWeight->addItem(tr("Inherited"), -1);
            m_pWeight->addItem(tr("Light"), QFont::Light);
            m_pWeight->addItem(tr("Normal"), QFont::Normal);
            m_pWeight->addItem(tr("Medium"), QFont::Medium);
            m_pWeight->addItem(tr("Demi bold"), QFont::DemiBold);
            m_pWeight->addItem(tr("Bold"), QFont::Bold);

            m_pPreview->setAccessibleName(tr("Live appearance preview"));
            m_pPreview->setMinimumHeight(56);
            m_pPreview->setAlignment(Qt::AlignCenter);
            m_pPreview->setText(tr("Material 3 preview"));

            QFormLayout *pForm = new QFormLayout;
            pForm->addRow(tr("Seed"), m_pSeed);
            pForm->addRow(tr("Typeface"), m_pFont);
            pForm->addRow(tr("Radius"), m_pRadius);
            pForm->addRow(tr("Scale"), m_pScale);
            pForm->addRow(tr("Weight"), m_pWeight);

            UIMd3Button *pReset = new UIMd3Button(tr("Reset element"), UIMd3ButtonVariant_Text, this);
            pReset->setAccessibleName(tr("Reset this element appearance"));
            connect(pReset, &UIMd3Button::sigClicked, this, [this]()
            {
                md3Theme().clearAppearance(m_strKey);
                close();
            });

            UIMd3Button *pApply = new UIMd3Button(tr("Apply"), UIMd3ButtonVariant_Filled, this);
            pApply->setAccessibleName(tr("Apply element appearance"));
            connect(pApply, &UIMd3Button::sigClicked, this, [this]() { apply(); });

            UIMd3Button *pCancel = new UIMd3Button(tr("Cancel"), UIMd3ButtonVariant_Text, this);
            connect(pCancel, &UIMd3Button::sigClicked, this, &QDialog::close);

            QHBoxLayout *pActions = new QHBoxLayout;
            pActions->addWidget(pReset);
            pActions->addStretch(1);
            pActions->addWidget(pCancel);
            pActions->addWidget(pApply);

            QVBoxLayout *pLayout = new QVBoxLayout(this);
            pLayout->setContentsMargins(16, 16, 16, 16);
            pLayout->addWidget(new QLabel(tr("Changes apply to this element and persist across restarts."), this));
            pLayout->addWidget(m_pPreview);
            pLayout->addLayout(pForm);
            pLayout->addLayout(pActions);
            setLayout(pLayout);

            connect(m_pSeed, &QLineEdit::textChanged, this, [this]() { preview(); });
            connect(m_pFont, &QComboBox::currentTextChanged, this, [this]() { preview(); });
            connect(m_pRadius, qOverload<int>(&QSpinBox::valueChanged), this, [this]() { preview(); });
            connect(m_pScale, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this]() { preview(); });
            connect(m_pWeight, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() { preview(); });

            const UIMd3Appearance appearance = md3Theme().appearance(strKey);
            if (appearance.fValid)
            {
                m_pSeed->setText(appearance.seed.name(QColor::HexArgb));
                m_pFont->setCurrentText(appearance.strFont);
                m_pRadius->setValue(appearance.iRadius);
                m_pScale->setValue(appearance.dScale);
                m_pWeight->setCurrentIndex(m_pWeight->findData(appearance.iWeight));
            }
            else
            {
                m_pRadius->setValue(UIMd3Shape::Large);
                m_pScale->setValue(1.0);
            }
            resize(420, sizeHint().height());
            preview();
        }

    protected:
        virtual void closeEvent(QCloseEvent *pEvent) RT_OVERRIDE
        {
            if (m_pAnchor)
                m_pAnchor->setFocus(Qt::OtherFocusReason);
            QDialog::closeEvent(pEvent);
        }

    private:
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
            const QColor seed(m_pSeed->text().trimmed());
            if (!seed.isValid() && !m_pSeed->text().trimmed().isEmpty())
            {
                m_pSeed->setFocus();
                return;
            }
            UIMd3Appearance appearance;
            appearance.fValid = true;
            appearance.seed = seed;
            appearance.strFont = m_pFont->currentData().toString();
            appearance.iRadius = m_pRadius->value();
            appearance.dScale = m_pScale->value();
            appearance.iWeight = m_pWeight->currentData().toInt();
            md3Theme().setAppearance(m_strKey, appearance);
            close();
        }

        QWidget      *m_pAnchor;
        QString        m_strKey;
        QLineEdit     *m_pSeed;
        QComboBox     *m_pFont;
        QSpinBox      *m_pRadius;
        QDoubleSpinBox*m_pScale;
        QComboBox     *m_pWeight;
        QLabel        *m_pPreview;
    };
}

void UIMd3AppearanceEditor::open(QWidget *pAnchor, const QString &strAppearanceKey)
{
    if (!pAnchor || strAppearanceKey.isEmpty())
        return;
    UIMd3AppearanceDialog *pDialog = new UIMd3AppearanceDialog(pAnchor, strAppearanceKey);
    const QPoint point = pAnchor->mapToGlobal(QPoint(pAnchor->width() + 8, 0));
    pDialog->move(point);
    pDialog->show();
    pDialog->raise();
    pDialog->activateWindow();
}
