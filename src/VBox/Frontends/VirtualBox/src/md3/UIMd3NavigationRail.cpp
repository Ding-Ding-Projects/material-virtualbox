/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 navigation rail for the manager shell.
 */

/* Qt includes: */
#include <QApplication>
#include <QButtonGroup>
#include <QColor>
#include <QFrame>
#include <QFontMetrics>
#include <QIcon>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3NavigationRail.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"

/** Paints one compact rail destination without coloring its outer hit tile. */
class UIMd3NavigationButton : public QToolButton
{
public:

    /** Constructs a navigation button with @a pParent as the owner. */
    explicit UIMd3NavigationButton(QWidget *pParent)
        : QToolButton(pParent)
    {
        setAttribute(Qt::WA_Hover, true);
    }

    /** Returns a height which continues to fit the configured UI font. */
    virtual QSize sizeHint() const override
    {
        return QSize(80, qMax(54, 38 + fontMetrics().height()));
    }

protected:

    /** Paints the 54x30 icon pill and a bounded visual label. */
    virtual void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const int iPillWidth = qMin(54, qMax(0, width() - 8));
        const QRect pillRect((width() - iPillWidth) / 2, 3, iPillWidth, 30);
        QColor pillColor = Qt::transparent;
        if (isChecked())
            pillColor = md3(UIMd3ColorRole_SecondaryContainer);
        else if (isDown())
            pillColor = md3(UIMd3ColorRole_SurfaceContainerHighest);
        else if (underMouse() && isEnabled())
            pillColor = md3(UIMd3ColorRole_SurfaceContainerHigh);
        if (pillColor.alpha() > 0)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(pillColor);
            painter.drawRoundedRect(pillRect, 15, 15);
        }

        if (hasFocus())
        {
            QPen focusPen(md3(UIMd3ColorRole_Primary), 2);
            painter.setPen(focusPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(pillRect.adjusted(1, 1, -1, -1), 14, 14);
        }

        const QRect iconRect(pillRect.center().x() - 11, pillRect.center().y() - 11, 22, 22);
        const QIcon::Mode enmIconMode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
        const QIcon::State enmIconState = isChecked() ? QIcon::On : QIcon::Off;
        icon().paint(&painter, iconRect, Qt::AlignCenter, enmIconMode, enmIconState);

        QColor textColor = isChecked()
                         ? md3(UIMd3ColorRole_OnSecondaryContainer)
                         : md3(UIMd3ColorRole_OnSurfaceVariant);
        if (!isEnabled())
            textColor.setAlphaF(0.38f);
        painter.setFont(font());
        painter.setPen(textColor);
        const QRect textRect(3, 35, qMax(0, width() - 6), qMax(0, height() - 36));
        const QString strVisualText = fontMetrics().elidedText(text(), Qt::ElideRight, textRect.width());
        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextSingleLine, strVisualText);
    }
};

/** Property carrying each button's uncolored source icon. */
static const char *g_pszMd3SourceIconProperty = "md3SourceIcon";

static QIcon md3RailSourceIcon(const char *pszName, const QIcon &fallback)
{
    const QIcon icon(QStringLiteral(":/md3/icons/") + QString::fromLatin1(pszName));
    return icon.isNull() ? fallback : icon;
}

static QPixmap md3TintedRailPixmap(const QIcon &source, const QColor &color, qreal dDpr)
{
    const int iPixelExtent = qMax(1, qRound(22.0 * dDpr));
    const QPixmap sourcePixmap = source.pixmap(QSize(iPixelExtent, iPixelExtent),
                                               QIcon::Normal, QIcon::Off);
    if (sourcePixmap.isNull())
        return QPixmap();

    QPixmap tintedPixmap(sourcePixmap.size());
    tintedPixmap.fill(Qt::transparent);
    QPainter painter(&tintedPixmap);
    painter.drawPixmap(0, 0, sourcePixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tintedPixmap.rect(), color);
    painter.end();
    tintedPixmap.setDevicePixelRatio(dDpr);
    return tintedPixmap;
}

static QIcon md3TintedRailIcon(const QIcon &source, qreal dDpr)
{
    QColor disabledColor = md3(UIMd3ColorRole_OnSurfaceVariant);
    disabledColor.setAlphaF(0.38f);
    const QPixmap normalPixmap = md3TintedRailPixmap(source,
                                                      md3(UIMd3ColorRole_OnSurfaceVariant), dDpr);
    const QPixmap selectedPixmap = md3TintedRailPixmap(source,
                                                        md3(UIMd3ColorRole_OnSecondaryContainer), dDpr);
    const QPixmap disabledPixmap = md3TintedRailPixmap(source, disabledColor, dDpr);

    QIcon icon;
    icon.addPixmap(normalPixmap, QIcon::Normal, QIcon::Off);
    icon.addPixmap(selectedPixmap, QIcon::Normal, QIcon::On);
    icon.addPixmap(normalPixmap, QIcon::Active, QIcon::Off);
    icon.addPixmap(selectedPixmap, QIcon::Active, QIcon::On);
    icon.addPixmap(disabledPixmap, QIcon::Disabled, QIcon::Off);
    icon.addPixmap(disabledPixmap, QIcon::Disabled, QIcon::On);
    return icon;
}

static void md3RegisterRailText()
{
    UIMd3Language *pLanguage = UIMd3Language::instance();
    if (!pLanguage)
        return;
    pLanguage->registerText(QStringLiteral("md3.tool.home"),
                            QStringLiteral("Home"), QStringLiteral("主頁"));
    pLanguage->registerText(QStringLiteral("md3.tool.machines"),
                            QStringLiteral("Machines"), QStringLiteral("虛擬機"));
    pLanguage->registerText(QStringLiteral("md3.tool.extensions"),
                            QStringLiteral("Extensions"), QStringLiteral("擴充功能"));
    pLanguage->registerText(QStringLiteral("md3.tool.media"),
                            QStringLiteral("Media"), QStringLiteral("媒體"));
    pLanguage->registerText(QStringLiteral("md3.tool.network"),
                            QStringLiteral("Network"), QStringLiteral("網絡"));
    pLanguage->registerText(QStringLiteral("md3.tool.cloud"),
                            QStringLiteral("Cloud"), QStringLiteral("雲端"));
    pLanguage->registerText(QStringLiteral("md3.tool.resources"),
                            QStringLiteral("Resources"), QStringLiteral("資源"));
    pLanguage->registerText(QStringLiteral("md3.tool.preferences"),
                            QStringLiteral("Preferences"), QStringLiteral("偏好設定"));
    pLanguage->registerText(QStringLiteral("md3.manager.disabled.machines"),
                            QStringLiteral("No virtual machine is available."),
                            QStringLiteral("沒有可用的虛擬機。"));
    pLanguage->registerText(QStringLiteral("md3.manager.disabled.expert"),
                            QStringLiteral("Expert mode is required."),
                            QStringLiteral("需要啟用專家模式。"));
    pLanguage->registerText(QStringLiteral("md3.manager.disabled.generic"),
                            QStringLiteral("This destination is currently unavailable."),
                            QStringLiteral("這個目的地目前不可用。"));
}

static QString md3RailText(const char *pszKey, const QString &strFallback)
{
    const UIMd3Language *pLanguage = UIMd3Language::instance();
    return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
}

UIMd3NavigationRail::UIMd3NavigationRail(QWidget *pParent)
    : QWidget(pParent)
    , m_pButtonGroup(new QButtonGroup(this))
    , m_pScrollArea(0)
    , m_pDestinationLayout(0)
    , m_pPreferencesButton(0)
{
    setObjectName("md3NavigationRail");
    setAccessibleName(md3RailText("md3.manager.navigation-rail", tr("Navigation rail")));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(92);

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 6);
    pLayout->setSpacing(2);

    m_pScrollArea = new QScrollArea(this);
    m_pScrollArea->setObjectName(QStringLiteral("md3NavigationScroll"));
    m_pScrollArea->setFrameShape(QFrame::NoFrame);
    m_pScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_pScrollArea->setWidgetResizable(true);
    m_pScrollArea->setFocusPolicy(Qt::NoFocus);

    QWidget *pDestinationContainer = new QWidget(m_pScrollArea);
    pDestinationContainer->setObjectName(QStringLiteral("md3NavigationDestinations"));
    m_pDestinationLayout = new QVBoxLayout(pDestinationContainer);
    m_pDestinationLayout->setContentsMargins(6, 10, 6, 4);
    m_pDestinationLayout->setSpacing(3);
    m_pScrollArea->setWidget(pDestinationContainer);
    pLayout->addWidget(m_pScrollArea, 1);

    createButton(UIToolType_Home, md3RailSourceIcon("welcome_screen_24px.png",
                                                     QApplication::style()->standardIcon(QStyle::SP_DirHomeIcon)));
    createButton(UIToolType_Machines, md3RailSourceIcon("machine_details_manager_24px.png",
                                                         QApplication::style()->standardIcon(QStyle::SP_ComputerIcon)));
    createButton(UIToolType_Extensions, md3RailSourceIcon("extension_pack_manager_24px.png",
                                                           QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView)));
    createButton(UIToolType_Media, md3RailSourceIcon("media_manager_24px.png",
                                                      QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon)));
    createButton(UIToolType_Network, md3RailSourceIcon("host_iface_manager_24px.png",
                                                        QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon)));
    createButton(UIToolType_Cloud, md3RailSourceIcon("cloud_profile_manager_24px.png",
                                                      QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon)));
    createButton(UIToolType_Resources, md3RailSourceIcon("resources_monitor_24px.png",
                                                          QApplication::style()->standardIcon(QStyle::SP_FileDialogContentsView)));
    m_pDestinationLayout->addStretch(1);
    createPreferencesButton();

    connect(m_pButtonGroup, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *pButton) {
                if (pButton)
                    emit sigToolTypeSelected(static_cast<UIToolType>(pButton->property("UIToolType").toInt()));
            });
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged,
            this, &UIMd3NavigationRail::sltUpdateTheme);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3NavigationRail::sltRetranslateUI, Qt::UniqueConnection);
    connect(qApp, &QApplication::fontChanged,
            this, &UIMd3NavigationRail::sltRetranslateUI);

    md3RegisterRailText();
    sltRetranslateUI();
    updatePalette();
    updateIcons();
}

QToolButton *UIMd3NavigationRail::createButton(UIToolType enmType, const QIcon &icon)
{
    if (!m_pDestinationLayout)
        return 0;
    UIMd3NavigationButton *pButton = new UIMd3NavigationButton(m_pDestinationLayout->parentWidget());
    pButton->setProperty("UIToolType", static_cast<int>(enmType));
    pButton->setProperty(g_pszMd3SourceIconProperty, icon);
    pButton->setCheckable(true);
    pButton->setAutoExclusive(true);
    pButton->setIconSize(QSize(22, 22));
    pButton->setFont(md3Theme().font(UIMd3TypeRole_LabelSmall));
    pButton->setMinimumSize(QSize(54, 54));
    pButton->setMaximumWidth(80);
    pButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    pButton->setFocusPolicy(Qt::StrongFocus);
    pButton->installEventFilter(this);
    m_pButtonGroup->addButton(pButton);
    m_navigationOrder << pButton;
    m_pDestinationLayout->addWidget(pButton);
    return pButton;
}

void UIMd3NavigationRail::createPreferencesButton()
{
    UIMd3NavigationButton *pButton = new UIMd3NavigationButton(this);
    QIcon icon(QStringLiteral(":/performance_monitor_preferences_24px.png"));
    if (icon.isNull())
        icon = QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView);
    pButton->setProperty(g_pszMd3SourceIconProperty, icon);
    pButton->setIconSize(QSize(22, 22));
    pButton->setFont(md3Theme().font(UIMd3TypeRole_LabelSmall));
    pButton->setMinimumSize(QSize(54, 54));
    pButton->setMaximumWidth(80);
    pButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    pButton->setFocusPolicy(Qt::StrongFocus);
    pButton->installEventFilter(this);
    m_pPreferencesButton = pButton;
    m_navigationOrder << pButton;
    if (QVBoxLayout *pLayout = qobject_cast<QVBoxLayout *>(layout()))
        pLayout->addWidget(pButton, 0, Qt::AlignHCenter);
    connect(pButton, &QToolButton::clicked,
            this, &UIMd3NavigationRail::sigPreferencesRequested);
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
    {
        pButton->setChecked(true);
        if (m_pScrollArea)
            m_pScrollArea->ensureWidgetVisible(pButton, 0, 4);
    }
}

void UIMd3NavigationRail::setToolEnabled(UIToolType enmType, bool fEnabled)
{
    if (QToolButton *pButton = button(enmType))
    {
        pButton->setEnabled(fEnabled);
        if (!fEnabled)
            pButton->setChecked(false);
        updateButtonStatus(pButton, enmType);
    }
}

void UIMd3NavigationRail::sltUpdateTheme()
{
    updatePalette();
    updateIcons();
    foreach (QAbstractButton *pAbstractButton, m_pButtonGroup->buttons())
        if (QToolButton *pButton = qobject_cast<QToolButton *>(pAbstractButton))
        {
            pButton->setFont(md3Theme().font(UIMd3TypeRole_LabelSmall));
            pButton->update();
        }
    if (m_pPreferencesButton)
    {
        m_pPreferencesButton->setFont(md3Theme().font(UIMd3TypeRole_LabelSmall));
        m_pPreferencesButton->update();
    }
}

void UIMd3NavigationRail::sltRetranslateUI()
{
    setAccessibleName(md3RailText("md3.manager.navigation-rail", tr("Navigation rail")));
    const struct
    {
        UIToolType enmType;
        const char *pszKey;
        const char *pszFallback;
    } aLabels[] =
    {
        { UIToolType_Home,       "md3.tool.home",       QT_TRANSLATE_NOOP("UIMd3NavigationRail", "Home") },
        { UIToolType_Machines,   "md3.tool.machines",   QT_TRANSLATE_NOOP("UIMd3NavigationRail", "Machines") },
        { UIToolType_Extensions, "md3.tool.extensions", QT_TRANSLATE_NOOP("UIMd3NavigationRail", "Extensions") },
        { UIToolType_Media,      "md3.tool.media",      QT_TRANSLATE_NOOP("UIMd3NavigationRail", "Media") },
        { UIToolType_Network,    "md3.tool.network",    QT_TRANSLATE_NOOP("UIMd3NavigationRail", "Network") },
        { UIToolType_Cloud,      "md3.tool.cloud",      QT_TRANSLATE_NOOP("UIMd3NavigationRail", "Cloud") },
        { UIToolType_Resources,  "md3.tool.resources",  QT_TRANSLATE_NOOP("UIMd3NavigationRail", "Resources") }
    };
    for (size_t i = 0; i < RT_ELEMENTS(aLabels); ++i)
        if (QToolButton *pButton = button(aLabels[i].enmType))
        {
            const QString strText = md3RailText(aLabels[i].pszKey, tr(aLabels[i].pszFallback));
            pButton->setText(strText);
            pButton->setAccessibleName(strText);
            updateButtonStatus(pButton, aLabels[i].enmType);
        }
    if (m_pPreferencesButton)
    {
        const QString strText = md3RailText("md3.tool.preferences", tr("Preferences"));
        m_pPreferencesButton->setText(strText);
        m_pPreferencesButton->setAccessibleName(strText);
        m_pPreferencesButton->setToolTip(strText);
        m_pPreferencesButton->setAccessibleDescription(QString());
        m_pPreferencesButton->setStatusTip(QString());
    }
}

bool UIMd3NavigationRail::event(QEvent *pEvent)
{
    const bool fUpdateIcons =    pEvent->type() == QEvent::ScreenChangeInternal
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
                              || pEvent->type() == QEvent::DevicePixelRatioChange
#endif
                              ;
    const bool fResult = QWidget::event(pEvent);
    if (fUpdateIcons)
        updateIcons();
    return fResult;
}

bool UIMd3NavigationRail::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    QToolButton *pCurrent = qobject_cast<QToolButton *>(pWatched);
    if (pCurrent && pEvent->type() == QEvent::KeyPress)
    {
        QKeyEvent *pKeyEvent = static_cast<QKeyEvent *>(pEvent);
        if (pKeyEvent->key() == Qt::Key_Up || pKeyEvent->key() == Qt::Key_Down)
        {
            int iIndex = m_navigationOrder.indexOf(pCurrent);
            const int iStep = pKeyEvent->key() == Qt::Key_Up ? -1 : 1;
            if (iIndex < 0 || m_navigationOrder.isEmpty())
                return QWidget::eventFilter(pWatched, pEvent);
            for (int i = 0; i < m_navigationOrder.size(); ++i)
            {
                iIndex = (iIndex + iStep + m_navigationOrder.size()) % m_navigationOrder.size();
                QToolButton *pNext = m_navigationOrder.at(iIndex);
                if (pNext && pNext->isEnabled() && pNext->isVisibleTo(this))
                {
                    pNext->setFocus(Qt::TabFocusReason);
                    if (m_pScrollArea && pNext != m_pPreferencesButton)
                        m_pScrollArea->ensureWidgetVisible(pNext, 0, 4);
                    pKeyEvent->accept();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(pWatched, pEvent);
}

QString UIMd3NavigationRail::disabledReason(UIToolType enmType) const
{
    switch (enmType)
    {
        case UIToolType_Machines:
            return md3RailText("md3.manager.disabled.machines",
                               tr("No virtual machine is available."));
        case UIToolType_Media:
        case UIToolType_Network:
            return md3RailText("md3.manager.disabled.expert",
                               tr("Expert mode is required."));
        default:
            return md3RailText("md3.manager.disabled.generic",
                               tr("This destination is currently unavailable."));
    }
}

void UIMd3NavigationRail::updateButtonStatus(QToolButton *pButton, UIToolType enmType)
{
    if (!pButton)
        return;
    const QString strReason = pButton->isEnabled() ? QString() : disabledReason(enmType);
    pButton->setToolTip(strReason.isEmpty() ? pButton->text() : strReason);
    pButton->setAccessibleDescription(strReason);
    pButton->setStatusTip(strReason);
}

void UIMd3NavigationRail::updateIcons()
{
    const qreal dDpr = qMax<qreal>(1.0, devicePixelRatioF());
    for (int i = 0; i < m_navigationOrder.size(); ++i)
    {
        QToolButton *pButton = m_navigationOrder.at(i);
        if (!pButton)
            continue;
        const QIcon source = pButton->property(g_pszMd3SourceIconProperty).value<QIcon>();
        if (!source.isNull())
            pButton->setIcon(md3TintedRailIcon(source, dDpr));
    }
}

void UIMd3NavigationRail::updatePalette()
{
    const QString strStyle = QStringLiteral(
        "QWidget#md3NavigationRail { background: %1; border-right: 1px solid %2; }"
        "QScrollArea#md3NavigationScroll, QWidget#md3NavigationDestinations { background: transparent; border: 0; }"
        "QScrollBar:vertical { background: transparent; width: 6px; margin: 2px 0; }"
        "QScrollBar::handle:vertical { background: %3; border-radius: 3px; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QToolButton { background: transparent; border: 0; padding: 0; }")
        .arg(md3(UIMd3ColorRole_SurfaceContainerLow).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_OutlineVariant).name(QColor::HexArgb))
        .arg(md3(UIMd3ColorRole_Outline).name(QColor::HexArgb));
    setStyleSheet(strStyle);
    update();
}
