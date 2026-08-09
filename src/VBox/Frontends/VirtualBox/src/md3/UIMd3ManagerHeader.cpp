/* $Id$ */
/** @file
 * VBox Qt GUI - compact Material 3 header for the existing manager window.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, in version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QWindow>

#include "UIIconPool.h"
#include "UIMd3ManagerHeader.h"
#include "UIMd3Button.h"
#include "UIMd3CommandPalette.h"
#include "UIMd3Language.h"
#include "UIMd3MenuSearch.h"
#include "UIMd3NotificationCentre.h"
#include "UIMd3Theme.h"

/** Creates a token-coloured icon from existing application artwork.
  * The source icon pool supplies the closest bundled high-DPI variant; the
  * generated icon is rebuilt when the theme or host screen changes. */
static QIcon md3HeaderIcon(const QIcon &source, const QColor &color, const QWidget *pWidget)
{
    const qreal dDevicePixelRatio = qMax<qreal>(1.0, pWidget ? pWidget->devicePixelRatioF() : 1.0);
    const QSize physicalSize(qMax(1, qRound(18 * dDevicePixelRatio)),
                             qMax(1, qRound(18 * dDevicePixelRatio)));
    QPixmap normal = source.pixmap(physicalSize, QIcon::Normal, QIcon::Off);
    if (normal.isNull())
        return source;
    normal.setDevicePixelRatio(dDevicePixelRatio);
    {
        QPainter painter(&normal);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(normal.rect(), color);
    }

    QPixmap disabled = normal;
    {
        QColor disabledColor = color;
        disabledColor.setAlphaF(disabledColor.alphaF() * 0.38);
        QPainter painter(&disabled);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(disabled.rect(), disabledColor);
    }

    QIcon result;
    result.addPixmap(normal, QIcon::Normal, QIcon::Off);
    result.addPixmap(normal, QIcon::Active, QIcon::Off);
    result.addPixmap(normal, QIcon::Selected, QIcon::Off);
    result.addPixmap(disabled, QIcon::Disabled, QIcon::Off);
    return result;
}

static QIcon md3HeaderIcon(const QString &strResource, const QColor &color, const QWidget *pWidget)
{
    return md3HeaderIcon(UIIconPool::iconSet(strResource), color, pWidget);
}

UIMd3ManagerHeader::UIMd3ManagerHeader(QMainWindow *pWindow, QWidget *pParent /* = 0 */)
    : QWidget(pParent)
    , m_pWindow(pWindow)
    , m_pBrandMark(0)
    , m_pTitle(0)
    , m_pSubtitle(0)
    , m_pUnread(0)
    , m_pMenu(0)
    , m_pPalette(0)
    , m_pMinimize(0)
    , m_pMaximize(0)
    , m_pClose(0)
    , m_pNotifications(0)
    , m_fDragging(false)
{
    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.application"), QStringLiteral("Material Virtual Machine"), QStringLiteral("Material Virtual Machine"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.menu"), QStringLiteral("Menu"), QStringLiteral("餐牌"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.minimize"), QStringLiteral("Minimize"), QStringLiteral("收埋"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.close"), QStringLiteral("Close"), QStringLiteral("閂埋"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.bell"), QStringLiteral("Notifications"), QStringLiteral("通知"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.subtitle"), QStringLiteral("Manager"), QStringLiteral("管理員"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.search-everything"), QStringLiteral("Search everything"), QStringLiteral("搜尋全部"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.application-menu"), QStringLiteral("Application menu"), QStringLiteral("應用程式選單"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.search-application-menu"), QStringLiteral("Search application actions"), QStringLiteral("搜尋應用程式動作"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.search-application-menu-name"), QStringLiteral("Search this application menu"), QStringLiteral("搜尋呢個應用程式選單"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.application-mark"), QStringLiteral("Application mark for %1"), QStringLiteral("%1 應用程式標記"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager.shortcut"), QStringLiteral("%1. Shortcut %2"), QStringLiteral("%1。快捷鍵 %2"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.window.maximize"), QStringLiteral("Maximize window"), QStringLiteral("放大視窗"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.window.restore"), QStringLiteral("Restore window"), QStringLiteral("還原視窗"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.unread"), QStringLiteral("Unread notifications are available"), QStringLiteral("有未讀通知"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.notifications.none"), QStringLiteral("No unread notifications"), QStringLiteral("沒有未讀通知"));
    }

    setObjectName(QStringLiteral("md3ManagerHeader"));
    /* A 48-pixel header gives every compact control a full accessible target
     * while retaining the intended low-profile title-bar composition. */
    setFixedHeight(48);
    setAutoFillBackground(true);

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(8, 0, 0, 0);
    pLayout->setSpacing(0);

    m_pMenu = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pMenu->setObjectName(QStringLiteral("md3ApplicationMenuButton"));
    m_pMenu->setAppearanceKey(QStringLiteral("manager-header/menu"));
    m_pMenu->setFixedSize(QSize(48, 48));
    connect(m_pMenu, &UIMd3Button::sigClicked,
            this, &UIMd3ManagerHeader::showApplicationMenu);
    pLayout->addWidget(m_pMenu);

    m_pBrandMark = new QLabel(this);
    m_pBrandMark->setAlignment(Qt::AlignCenter);
    m_pBrandMark->setFixedSize(QSize(28, 28));
    m_pBrandMark->setAttribute(Qt::WA_TransparentForMouseEvents);
    pLayout->addWidget(m_pBrandMark);
    pLayout->addSpacing(8);

    m_pTitle = new QLabel(this);
    m_pTitle->setFont(md3Theme().font(UIMd3TypeRole_LabelLarge));
    m_pTitle->setMinimumWidth(0);
    m_pTitle->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_pTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
    pLayout->addWidget(m_pTitle);
    pLayout->addSpacing(8);

    m_pSubtitle = new QLabel(this);
    m_pSubtitle->setFont(md3Theme().font(UIMd3TypeRole_BodySmall));
    m_pSubtitle->setMinimumWidth(0);
    m_pSubtitle->setAttribute(Qt::WA_TransparentForMouseEvents);
    pLayout->addWidget(m_pSubtitle);
    pLayout->addStretch(1);

    m_pPalette = new UIMd3Button(QString(), UIMd3ButtonVariant_Tonal, this);
    m_pPalette->setObjectName(QStringLiteral("md3CommandPaletteButton"));
    m_pPalette->setAppearanceKey(QStringLiteral("manager-header/palette"));
    m_pPalette->setFixedHeight(48);
    connect(m_pPalette, &UIMd3Button::sigClicked, this, [pWindow]()
    {
        UIMd3CommandPalette::showPalette(pWindow);
    });
    pLayout->addWidget(m_pPalette);

    m_pNotifications = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pNotifications->setObjectName(QStringLiteral("md3NotificationsButton"));
    m_pNotifications->setAppearanceKey(QStringLiteral("manager-header/notifications"));
    m_pNotifications->setFixedSize(QSize(48, 48));
    connect(m_pNotifications, &UIMd3Button::sigClicked, this, [pWindow]()
    {
        if (UIMd3NotificationCentre::instance())
            UIMd3NotificationCentre::instance()->showCentre(pWindow);
    });
    pLayout->addWidget(m_pNotifications);

    m_pUnread = new QLabel(m_pNotifications);
    m_pUnread->setFixedSize(QSize(8, 8));
    m_pUnread->move(32, 6);
    m_pUnread->setAttribute(Qt::WA_TransparentForMouseEvents);
    if (UIMd3NotificationCentre::instance())
        connect(UIMd3NotificationCentre::instance(), &UIMd3NotificationCentre::sigChanged,
                this, &UIMd3ManagerHeader::updateNotificationState);
    connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
            this, &UIMd3ManagerHeader::updateNotificationState);
    m_pMinimize = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pMinimize->setObjectName(QStringLiteral("md3MinimizeButton"));
    m_pMinimize->setAppearanceKey(QStringLiteral("manager-header/minimize"));
    m_pMinimize->setFixedSize(QSize(48, 48));
    connect(m_pMinimize, &UIMd3Button::sigClicked, pWindow, &QWidget::showMinimized);
    pLayout->addWidget(m_pMinimize);

    m_pMaximize = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pMaximize->setObjectName(QStringLiteral("md3MaximizeButton"));
    m_pMaximize->setAppearanceKey(QStringLiteral("manager-header/maximize"));
    m_pMaximize->setFixedSize(QSize(48, 48));
    connect(m_pMaximize, &UIMd3Button::sigClicked, this, &UIMd3ManagerHeader::toggleMaximize);
    pLayout->addWidget(m_pMaximize);

    m_pClose = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pClose->setObjectName(QStringLiteral("md3CloseButton"));
    m_pClose->setAppearanceKey(QStringLiteral("manager-header/close"));
    m_pClose->setFixedSize(QSize(48, 48));
    connect(m_pClose, &UIMd3Button::sigClicked, pWindow, &QWidget::close);
    pLayout->addWidget(m_pClose);

    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3ManagerHeader::updateChromeText);
    connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
            this, &UIMd3ManagerHeader::updateTheme);
    if (m_pWindow)
        m_pWindow->installEventFilter(this);
    updateTheme();
    updateChromeText();
    updateNotificationState();
    updateResponsiveLayout();
}

bool UIMd3ManagerHeader::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pObject == m_pWindow && pEvent)
    {
        switch (pEvent->type())
        {
            case QEvent::WindowStateChange:
                /* Queue one refresh as well: some window managers deliver the
                 * notification while Qt is still finishing the state change. */
                updateMaximizeLabel();
                QTimer::singleShot(0, this, [this]() { updateMaximizeLabel(); });
                break;
            case QEvent::ScreenChangeInternal:
            case QEvent::Show:
                updateIcons();
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(pObject, pEvent);
}

void UIMd3ManagerHeader::showApplicationMenu()
{
    if (!m_pWindow || !m_pWindow->menuBar() || !m_pMenu)
        return;
    QMenu menu;
    menu.setAccessibleName(md3Text(QStringLiteral("md3.manager.application-menu")));
    foreach (QAction *pAction, m_pWindow->menuBar()->actions())
    {
        if (!pAction)
            continue;
        menu.addAction(pAction);
    }
    md3PrepareSearchableMenu(&menu,
                             QStringLiteral("manager-application-menu"),
                             md3Text(QStringLiteral("md3.manager.search-application-menu")),
                             md3Text(QStringLiteral("md3.manager.search-application-menu-name")));
    if (!menu.actions().isEmpty())
        menu.exec(m_pMenu->mapToGlobal(QPoint(0, m_pMenu->height() + 2)));
}

void UIMd3ManagerHeader::toggleMaximize()
{
    if (!m_pWindow)
        return;
    if (m_pWindow->isMaximized())
        m_pWindow->showNormal();
    else
        m_pWindow->showMaximized();
    updateMaximizeLabel();
}

void UIMd3ManagerHeader::updateMaximizeLabel()
{
    if (!m_pMaximize || !m_pWindow)
        return;
    const bool fMaximized = m_pWindow->isMaximized();
    const QString strLabel = md3Text(fMaximized ? QStringLiteral("md3.window.restore")
                                                : QStringLiteral("md3.window.maximize"));
    const QIcon source = fMaximized
                       ? UIIconPool::iconSet(QStringLiteral(":/restore_16px.png"))
                       : QApplication::style()->standardIcon(QStyle::SP_TitleBarMaxButton);
    m_pMaximize->setIcon(md3HeaderIcon(source, md3(UIMd3ColorRole_OnSurfaceVariant), this));
    m_pMaximize->setAccessibleName(strLabel);
    m_pMaximize->setToolTip(strLabel);
}

void UIMd3ManagerHeader::updateBranding()
{
    const QString strBrand = md3Theme().brandName().trimmed();
    const QString strVisibleBrand = strBrand.isEmpty() ? QStringLiteral("Material Virtual Machine") : strBrand;
    if (m_pBrandMark)
    {
        m_pBrandMark->setText(strVisibleBrand.left(1).toUpper());
        m_pBrandMark->setAccessibleName(md3Text(QStringLiteral("md3.manager.application-mark")).arg(strVisibleBrand));
    }
    if (m_pTitle)
    {
        const int iMaximumTitleWidth = qBound(88, width() / 5, 240);
        m_pTitle->setMaximumWidth(iMaximumTitleWidth);
        m_pTitle->setText(QFontMetrics(m_pTitle->font()).elidedText(strVisibleBrand,
                                                                   Qt::ElideRight,
                                                                   iMaximumTitleWidth));
        m_pTitle->setAccessibleName(strVisibleBrand);
        m_pTitle->setToolTip(strVisibleBrand);
    }
}

void UIMd3ManagerHeader::updateChromeText()
{
    if (m_pMenu)
    {
        const QString strMenu = md3Text(QStringLiteral("md3.menu"));
        m_pMenu->setAccessibleName(strMenu);
        m_pMenu->setToolTip(strMenu);
    }
    if (m_pSubtitle)
    {
        const QString strSubtitle = md3Text(QStringLiteral("md3.manager.subtitle"));
        const int iMaximumSubtitleWidth = qBound(64, width() / 10, 120);
        m_pSubtitle->setMaximumWidth(iMaximumSubtitleWidth);
        m_pSubtitle->setText(QFontMetrics(m_pSubtitle->font()).elidedText(strSubtitle,
                                                                         Qt::ElideRight,
                                                                         iMaximumSubtitleWidth));
        m_pSubtitle->setAccessibleName(strSubtitle);
        m_pSubtitle->setToolTip(strSubtitle);
    }
    if (m_pPalette)
    {
        const QString strPalette = md3Text(QStringLiteral("md3.manager.search-everything"));
        m_pPalette->setAccessibleName(md3Text(QStringLiteral("md3.manager.shortcut"))
                                     .arg(strPalette, QStringLiteral("Ctrl+Shift+F")));
        m_pPalette->setToolTip(QStringLiteral("%1 (Ctrl+Shift+F)").arg(strPalette));
    }
    if (m_pNotifications)
    {
        const QString strNotifications = md3Text(QStringLiteral("md3.notifications.bell"));
        m_pNotifications->setAccessibleName(strNotifications);
        m_pNotifications->setToolTip(strNotifications);
    }
    if (m_pMinimize)
    {
        const QString strMinimize = md3Text(QStringLiteral("md3.minimize"));
        m_pMinimize->setAccessibleName(strMinimize);
        m_pMinimize->setToolTip(strMinimize);
    }
    if (m_pMaximize)
        updateMaximizeLabel();
    if (m_pClose)
    {
        const QString strClose = md3Text(QStringLiteral("md3.close"));
        m_pClose->setAccessibleName(strClose);
        m_pClose->setToolTip(strClose);
    }
    updateBranding();
    updateNotificationState();
    updateResponsiveLayout();
}

void UIMd3ManagerHeader::updateNotificationState()
{
    if (!m_pUnread)
        return;
    const bool fUnread = UIMd3NotificationCentre::instance()
                      && UIMd3NotificationCentre::instance()->hasUnread();
    m_pUnread->setVisible(fUnread);
    m_pUnread->setStyleSheet(QStringLiteral("background:%1;border-radius:4px;")
                             .arg(md3(UIMd3ColorRole_Error).name(QColor::HexArgb)));
    if (m_pNotifications)
        m_pNotifications->setAccessibleDescription(md3Text(fUnread
                                                            ? QStringLiteral("md3.notifications.unread")
                                                            : QStringLiteral("md3.notifications.none")));
}

void UIMd3ManagerHeader::updateResponsiveLayout()
{
    if (!m_pPalette)
        return;
    const bool fCompactPalette = width() > 0 && width() < 900;
    const QString strPalette = md3Text(QStringLiteral("md3.manager.search-everything"));
    m_pPalette->setVariant(fCompactPalette ? UIMd3ButtonVariant_Icon : UIMd3ButtonVariant_Tonal);
    if (fCompactPalette)
    {
        m_pPalette->setFixedSize(QSize(48, 48));
        m_pPalette->setText(QString());
    }
    else
    {
        const int iPaletteWidth = qBound(168, width() / 4, 260);
        m_pPalette->setMinimumSize(QSize(168, 48));
        m_pPalette->setMaximumSize(QSize(iPaletteWidth, 48));
        m_pPalette->setText(QFontMetrics(md3Theme().font(UIMd3TypeRole_LabelLarge))
                            .elidedText(strPalette, Qt::ElideRight, iPaletteWidth - 64));
    }
    m_pPalette->setAccessibleName(md3Text(QStringLiteral("md3.manager.shortcut"))
                                 .arg(strPalette, QStringLiteral("Ctrl+Shift+F")));
    m_pPalette->setToolTip(QStringLiteral("%1 (Ctrl+Shift+F)").arg(strPalette));
    if (m_pSubtitle)
        m_pSubtitle->setVisible(width() == 0 || width() >= 960);
    updateBranding();
}

void UIMd3ManagerHeader::updateIcons()
{
    const QColor color = md3(UIMd3ColorRole_OnSurfaceVariant);
    if (m_pMenu)
        m_pMenu->setIcon(md3HeaderIcon(QStringLiteral(":/tools_menu_24px.png"), color, this));
    if (m_pPalette)
        m_pPalette->setIcon(md3HeaderIcon(QStringLiteral(":/search_16px.png"), color, this));
    if (m_pNotifications)
        m_pNotifications->setIcon(md3HeaderIcon(QStringLiteral(":/notification_center_16px.png"), color, this));
    if (m_pMinimize)
        m_pMinimize->setIcon(md3HeaderIcon(QStringLiteral(":/minimize_16px.png"), color, this));
    if (m_pClose)
        m_pClose->setIcon(md3HeaderIcon(QStringLiteral(":/close_16px.png"), color, this));
    updateMaximizeLabel();
}

void UIMd3ManagerHeader::updateTheme()
{
    QPalette headerPalette = palette();
    headerPalette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    headerPalette.setColor(QPalette::WindowText, md3(UIMd3ColorRole_OnSurface));
    setPalette(headerPalette);
    if (m_pBrandMark)
    {
        m_pBrandMark->setFont(md3Theme().font(UIMd3TypeRole_LabelLarge));
        m_pBrandMark->setStyleSheet(QStringLiteral("background:%1;color:%2;border-radius:8px;")
                                    .arg(md3(UIMd3ColorRole_Primary).name(QColor::HexArgb))
                                    .arg(md3(UIMd3ColorRole_OnPrimary).name(QColor::HexArgb)));
    }
    if (m_pTitle)
        m_pTitle->setFont(md3Theme().font(UIMd3TypeRole_LabelLarge));
    if (m_pSubtitle)
        m_pSubtitle->setFont(md3Theme().font(UIMd3TypeRole_BodySmall));
    updateIcons();
    updateBranding();
    updateNotificationState();
}

void UIMd3ManagerHeader::mousePressEvent(QMouseEvent *pEvent)
{
    if (pEvent->button() == Qt::LeftButton && m_pWindow)
    {
        if (m_pWindow->windowHandle() && m_pWindow->windowHandle()->startSystemMove())
        {
            m_fDragging = false;
            pEvent->accept();
            return;
        }
        if (m_pWindow->isMaximized())
        {
            QWidget::mousePressEvent(pEvent);
            return;
        }
        m_fDragging = true;
        m_dragOffset = pEvent->globalPosition().toPoint() - m_pWindow->frameGeometry().topLeft();
        pEvent->accept();
        return;
    }
    QWidget::mousePressEvent(pEvent);
}

void UIMd3ManagerHeader::mouseMoveEvent(QMouseEvent *pEvent)
{
    if (m_fDragging && m_pWindow && !m_pWindow->isMaximized())
    {
        m_pWindow->move(pEvent->globalPosition().toPoint() - m_dragOffset);
        pEvent->accept();
        return;
    }
    QWidget::mouseMoveEvent(pEvent);
}

void UIMd3ManagerHeader::mouseReleaseEvent(QMouseEvent *pEvent)
{
    m_fDragging = false;
    QWidget::mouseReleaseEvent(pEvent);
}

void UIMd3ManagerHeader::resizeEvent(QResizeEvent *pEvent)
{
    QWidget::resizeEvent(pEvent);
    updateResponsiveLayout();
}

void UIMd3ManagerHeader::mouseDoubleClickEvent(QMouseEvent *pEvent)
{
    if (pEvent->button() == Qt::LeftButton)
    {
        toggleMaximize();
        pEvent->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(pEvent);
}
