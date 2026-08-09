/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 navigation rail for the manager shell.
 */

/* Qt includes: */
#include <QApplication>
#include <QButtonGroup>
#include <QIcon>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3NavigationRail.h"
#include "UIMd3Theme.h"

UIMd3NavigationRail::UIMd3NavigationRail(QWidget *pParent)
    : QWidget(pParent)
    , m_pButtonGroup(new QButtonGroup(this))
{
    setObjectName("md3NavigationRail");
    setAccessibleName(tr("Navigation rail"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setMinimumWidth(176);
    setMaximumWidth(224);

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(12, 12, 12, 12);
    pLayout->setSpacing(8);

    createButton(UIToolType_Home, QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon));
    createButton(UIToolType_Machines, QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    createButton(UIToolType_Media, QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon));
    createButton(UIToolType_Network, QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon));
    createButton(UIToolType_Cloud, QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    createButton(UIToolType_Resources, QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView));
    createButton(UIToolType_Extensions, QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView));
    pLayout->addStretch(1);

    connect(m_pButtonGroup, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *pButton) {
                if (pButton)
                    emit sigToolTypeSelected(static_cast<UIToolType>(pButton->property("UIToolType").toInt()));
            });
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged,
            this, &UIMd3NavigationRail::sltUpdateTheme);
    connect(qApp, &QApplication::fontChanged,
            this, &UIMd3NavigationRail::sltRetranslateUI);

    sltRetranslateUI();
    updatePalette();
}

QToolButton *UIMd3NavigationRail::createButton(UIToolType enmType, const QIcon &icon)
{
    QToolButton *pButton = new QToolButton(this);
    pButton->setProperty("UIToolType", static_cast<int>(enmType));
    pButton->setCheckable(true);
    pButton->setAutoExclusive(true);
    pButton->setIcon(icon);
    pButton->setIconSize(QSize(22, 22));
    pButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pButton->setMinimumHeight(md3Theme().controlHeight());
    pButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pButton->setFocusPolicy(Qt::StrongFocus);
    m_pButtonGroup->addButton(pButton);
    qobject_cast<QVBoxLayout *>(layout())->insertWidget(m_pButtonGroup->buttons().size() - 1, pButton);
    return pButton;
}

QToolButton *UIMd3NavigationRail::button(UIToolType enmType) const
{
    foreach (QAbstractButton *pButton, m_pButtonGroup->buttons())
        if (pButton->property("UIToolType").toInt() == static_cast<int>(enmType))
            return qobject_cast<QToolButton *>(pButton);
    return 0;
}

void UIMd3NavigationRail::setCurrentToolType(UIToolType enmType)
{
    if (QToolButton *pButton = button(enmType))
        pButton->setChecked(true);
}

void UIMd3NavigationRail::setToolEnabled(UIToolType enmType, bool fEnabled)
{
    if (QToolButton *pButton = button(enmType))
    {
        pButton->setEnabled(fEnabled);
        if (!fEnabled)
            pButton->setChecked(false);
    }
}

void UIMd3NavigationRail::sltUpdateTheme()
{
    updatePalette();
    foreach (QAbstractButton *pAbstractButton, m_pButtonGroup->buttons())
        if (QToolButton *pButton = qobject_cast<QToolButton *>(pAbstractButton))
            pButton->setMinimumHeight(md3Theme().controlHeight());
}

void UIMd3NavigationRail::sltRetranslateUI()
{
    setAccessibleName(tr("Navigation rail"));
    if (QToolButton *pButton = button(UIToolType_Home)) { pButton->setText(tr("Home")); pButton->setAccessibleName(tr("Home")); }
    if (QToolButton *pButton = button(UIToolType_Machines)) { pButton->setText(tr("Machines")); pButton->setAccessibleName(tr("Machines")); }
    if (QToolButton *pButton = button(UIToolType_Media)) { pButton->setText(tr("Media")); pButton->setAccessibleName(tr("Media")); }
    if (QToolButton *pButton = button(UIToolType_Network)) { pButton->setText(tr("Network")); pButton->setAccessibleName(tr("Network")); }
    if (QToolButton *pButton = button(UIToolType_Cloud)) { pButton->setText(tr("Cloud")); pButton->setAccessibleName(tr("Cloud")); }
    if (QToolButton *pButton = button(UIToolType_Resources)) { pButton->setText(tr("Resources")); pButton->setAccessibleName(tr("Resources")); }
    if (QToolButton *pButton = button(UIToolType_Extensions)) { pButton->setText(tr("Extensions")); pButton->setAccessibleName(tr("Extensions")); }
}

void UIMd3NavigationRail::updatePalette()
{
    const QString strStyle = QStringLiteral(
        "QWidget#md3NavigationRail { background: %1; border-right: 1px solid %2; }"
        "QToolButton { color: %3; background: transparent; border: 0; border-radius: 20px; padding: 8px 16px; text-align: left; }"
        "QToolButton:hover { background: %4; }"
        "QToolButton:focus { border: 2px solid %5; padding: 6px 14px; }"
        "QToolButton:checked { color: %6; background: %7; font-weight: 600; }"
        "QToolButton:disabled { color: %8; }")
        .arg(md3(UIMd3ColorRole_SurfaceContainerLow).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_OutlineVariant).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_OnSurfaceVariant).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_SurfaceContainerHigh).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_Primary).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_OnSecondaryContainer).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_SecondaryContainer).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_OnSurfaceVariant).name(QColor::HexArgb));
    setStyleSheet(strStyle);
}
