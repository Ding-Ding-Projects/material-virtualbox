/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 header for the normal runtime window.
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

/* Qt includes: */
#include <QAccessible>
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
#include <QPointer>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QWindow>

/* GUI includes: */
#include "UICommon.h"
#include "UIIconPool.h"
#include "UIMd3Button.h"
#include "UIMd3Language.h"
#include "UIMd3MenuSearch.h"
#include "UIMd3RuntimeHeader.h"
#include "UIMd3Theme.h"
#include "UITranslationEventListener.h"

/** Creates a theme-coloured icon from existing high-DPI application artwork. */
static QIcon md3RuntimeHeaderIcon(const QIcon &source, const QColor &color, const QWidget *pWidget)
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

/** Creates a theme-coloured icon from an application resource. */
static QIcon md3RuntimeHeaderIcon(const QString &strResource, const QColor &color, const QWidget *pWidget)
{
    return md3RuntimeHeaderIcon(UIIconPool::iconSet(strResource), color, pWidget);
}

UIMd3RuntimeHeader::UIMd3RuntimeHeader(QMainWindow *pWindow, QMenuBar *pMenuBar, QWidget *pParent /* = 0 */)
    : QWidget(pParent)
    , m_pWindow(pWindow)
    , m_pMenuBar(pMenuBar)
    , m_pMachineIcon(0)
    , m_pTitle(0)
    , m_pMenu(0)
    , m_pMinimize(0)
    , m_pMaximize(0)
    , m_pClose(0)
    , m_fDragging(false)
{
    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.menu"),
                                                QStringLiteral("Machine menu"),
                                                QStringLiteral("虛擬機選單"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.menu-description"),
                                                QStringLiteral("Opens the searchable Machine, View, Input, Devices, and Help menus."),
                                                QStringLiteral("開啟可搜尋嘅虛擬機、檢視、輸入、裝置同說明選單。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.search-menu"),
                                                QStringLiteral("Search runtime actions"),
                                                QStringLiteral("搜尋執行中動作"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.search-menu-name"),
                                                QStringLiteral("Search this machine menu"),
                                                QStringLiteral("搜尋呢個虛擬機選單"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.machine-icon"),
                                                QStringLiteral("Virtual machine icon"),
                                                QStringLiteral("虛擬機圖示"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.title-description"),
                                                QStringLiteral("Current virtual machine and session state."),
                                                QStringLiteral("目前虛擬機同工作階段狀態。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.minimize"),
                                                QStringLiteral("Minimize window"),
                                                QStringLiteral("縮小視窗"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.minimize-description"),
                                                QStringLiteral("Hides this virtual machine window on the taskbar."),
                                                QStringLiteral("將呢個虛擬機視窗縮到工作列。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.maximize"),
                                                QStringLiteral("Maximize window"),
                                                QStringLiteral("放大視窗"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.maximize-description"),
                                                QStringLiteral("Expands this virtual machine window to the available desktop."),
                                                QStringLiteral("將呢個虛擬機視窗放大到可用桌面。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.restore"),
                                                QStringLiteral("Restore window"),
                                                QStringLiteral("還原視窗"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.restore-description"),
                                                QStringLiteral("Returns this virtual machine window to its previous size."),
                                                QStringLiteral("將呢個虛擬機視窗還原到之前大小。"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.close"),
                                                QStringLiteral("Close window"),
                                                QStringLiteral("關閉視窗"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.runtime.close-description"),
                                                QStringLiteral("Starts the existing virtual machine close flow."),
                                                QStringLiteral("開始現有嘅虛擬機關閉流程。"));
    }

    setObjectName(QStringLiteral("md3RuntimeHeader"));
    setFixedHeight(48);
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(4, 0, 0, 0);
    pLayout->setSpacing(0);

    m_pMenu = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pMenu->setObjectName(QStringLiteral("md3RuntimeMenuButton"));
    m_pMenu->setAppearanceKey(QStringLiteral("runtime-header/menu"));
    m_pMenu->setFixedSize(QSize(48, 48));
    connect(m_pMenu, &UIMd3Button::sigClicked,
            this, &UIMd3RuntimeHeader::showApplicationMenu);
    pLayout->addWidget(m_pMenu);

    m_pMachineIcon = new QLabel(this);
    m_pMachineIcon->setAlignment(Qt::AlignCenter);
    m_pMachineIcon->setFixedSize(QSize(32, 48));
    m_pMachineIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
    pLayout->addWidget(m_pMachineIcon);
    pLayout->addSpacing(8);

    m_pTitle = new QLabel(this);
    m_pTitle->setMinimumWidth(0);
    m_pTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_pTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
    pLayout->addWidget(m_pTitle, 1);

    m_pMinimize = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pMinimize->setObjectName(QStringLiteral("md3RuntimeMinimizeButton"));
    m_pMinimize->setAppearanceKey(QStringLiteral("runtime-header/minimize"));
    m_pMinimize->setFixedSize(QSize(48, 48));
    connect(m_pMinimize, &UIMd3Button::sigClicked, pWindow, &QWidget::showMinimized);
    pLayout->addWidget(m_pMinimize);

    m_pMaximize = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pMaximize->setObjectName(QStringLiteral("md3RuntimeMaximizeButton"));
    m_pMaximize->setAppearanceKey(QStringLiteral("runtime-header/maximize"));
    m_pMaximize->setFixedSize(QSize(48, 48));
    connect(m_pMaximize, &UIMd3Button::sigClicked,
            this, &UIMd3RuntimeHeader::toggleMaximize);
    pLayout->addWidget(m_pMaximize);

    m_pClose = new UIMd3Button(QString(), UIMd3ButtonVariant_Icon, this);
    m_pClose->setObjectName(QStringLiteral("md3RuntimeCloseButton"));
    m_pClose->setAppearanceKey(QStringLiteral("runtime-header/close"));
    m_pClose->setFixedSize(QSize(48, 48));
    connect(m_pClose, &UIMd3Button::sigClicked, pWindow, &QWidget::close);
    pLayout->addWidget(m_pClose);

    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3RuntimeHeader::sltRetranslateUI);
    if (UIMd3Theme::instance())
        connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
                this, &UIMd3RuntimeHeader::sltUpdateTheme);
    if (UICommon::instance())
        connect(&uiCommon(), &UICommon::sigThemeChange,
                this, &UIMd3RuntimeHeader::sltUpdateTheme);
    if (UITranslationEventListener::instance())
        connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
                this, &UIMd3RuntimeHeader::sltRetranslateUI);
    if (m_pWindow)
        m_pWindow->installEventFilter(this);

    sltUpdateTheme();
    sltRetranslateUI();
    sltUpdateWindowTitle();
}

bool UIMd3RuntimeHeader::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pObject == m_pWindow && pEvent)
    {
        switch (pEvent->type())
        {
            case QEvent::WindowStateChange:
                updateMaximizeButton();
                QTimer::singleShot(0, this, [this]() { updateMaximizeButton(); });
                break;
            case QEvent::WindowTitleChange:
            case QEvent::WindowIconChange:
                sltUpdateWindowTitle();
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

void UIMd3RuntimeHeader::showApplicationMenu()
{
    if (!m_pMenuBar || !m_pMenu)
        return;

    QMenu menu;
    const QString strMenuName = UIMd3Language::instance()
                              ? md3Text(QStringLiteral("md3.runtime.menu"))
                              : tr("Machine menu");
    menu.setTitle(strMenuName);
    menu.setAccessibleName(strMenuName);
    foreach (QAction *pAction, m_pMenuBar->actions())
    {
        if (!pAction)
            continue;
        if (pAction->menu())
        {
            QMenu *pProxyMenu = menu.addMenu(pAction->icon(), pAction->text());
            pProxyMenu->setEnabled(pAction->isEnabled());
            pProxyMenu->setVisible(pAction->isVisible());
            populateProxyMenu(pProxyMenu, pAction->menu(), pAction->text());
        }
        else
        {
            QAction *pProxy = menu.addAction(pAction->icon(), pAction->text());
            pProxy->setCheckable(pAction->isCheckable());
            pProxy->setChecked(pAction->isChecked());
            pProxy->setEnabled(pAction->isEnabled());
            pProxy->setVisible(pAction->isVisible());
            pProxy->setShortcut(pAction->shortcut());
            pProxy->setShortcutContext(pAction->shortcutContext());
            pProxy->setStatusTip(pAction->statusTip());
            pProxy->setToolTip(pAction->toolTip());
            pProxy->setWhatsThis(pAction->whatsThis());
            const QPointer<QAction> original(pAction);
            connect(pProxy, &QAction::triggered, &menu, [original](bool)
            {
                if (original && original->isEnabled())
                    original->trigger();
            });
        }
    }
    md3PrepareSearchableMenu(&menu,
                             QStringLiteral("runtime-application-menu"),
                             UIMd3Language::instance()
                             ? md3Text(QStringLiteral("md3.runtime.search-menu"))
                             : tr("Search runtime actions"),
                             UIMd3Language::instance()
                             ? md3Text(QStringLiteral("md3.runtime.search-menu-name"))
                             : tr("Search this machine menu"));
    if (!menu.actions().isEmpty())
        menu.exec(m_pMenu->mapToGlobal(QPoint(0, m_pMenu->height() + 2)));
}

void UIMd3RuntimeHeader::populateProxyMenu(QMenu *pTarget, QMenu *pSource, const QString &strPath)
{
    if (!pTarget || !pSource)
        return;

    emit pSource->aboutToShow();
    foreach (QAction *pOriginal, pSource->actions())
    {
        if (!pOriginal)
            continue;
        if (pOriginal->isSeparator())
        {
            QAction *pSeparator = pTarget->addSeparator();
            pSeparator->setVisible(pOriginal->isVisible());
            continue;
        }
        if (pOriginal->menu())
        {
            QMenu *pChild = pTarget->addMenu(pOriginal->icon(), pOriginal->text());
            pChild->setEnabled(pOriginal->isEnabled());
            pChild->setVisible(pOriginal->isVisible());
            populateProxyMenu(pChild, pOriginal->menu(),
                              strPath + QStringLiteral(" / ") + pOriginal->text());
            continue;
        }

        QAction *pProxy = pTarget->addAction(pOriginal->icon(), pOriginal->text());
        pProxy->setCheckable(pOriginal->isCheckable());
        pProxy->setChecked(pOriginal->isChecked());
        pProxy->setEnabled(pOriginal->isEnabled());
        pProxy->setVisible(pOriginal->isVisible());
        pProxy->setShortcut(pOriginal->shortcut());
        pProxy->setShortcutContext(pOriginal->shortcutContext());
        pProxy->setStatusTip(strPath.isEmpty()
                           ? pOriginal->statusTip()
                           : QStringLiteral("%1 — %2").arg(strPath, pOriginal->statusTip()));
        pProxy->setToolTip(pOriginal->toolTip());
        pProxy->setWhatsThis(pOriginal->whatsThis());
        const QPointer<QAction> original(pOriginal);
        connect(pProxy, &QAction::triggered, pTarget, [original](bool)
        {
            if (original && original->isEnabled())
                original->trigger();
        });
    }

    QString strFieldKey = strPath;
    strFieldKey.remove(QLatin1Char('&'));
    strFieldKey.replace(QLatin1Char('/'), QLatin1Char('-'));
    const QString strFieldId = QStringLiteral("runtime-submenu/%1").arg(strFieldKey);
    md3PrepareSearchableMenu(pTarget,
                             strFieldId,
                             UIMd3Language::instance()
                             ? md3Text(QStringLiteral("md3.runtime.search-menu"))
                             : tr("Search runtime actions"),
                             UIMd3Language::instance()
                             ? md3Text(QStringLiteral("md3.runtime.search-menu-name"))
                             : tr("Search this machine menu"));
}

void UIMd3RuntimeHeader::toggleMaximize()
{
    if (!m_pWindow)
        return;
    if (m_pWindow->isMaximized())
        m_pWindow->showNormal();
    else
        m_pWindow->showMaximized();
    updateMaximizeButton();
}

void UIMd3RuntimeHeader::sltRetranslateUI()
{
    const QString strMenu = UIMd3Language::instance()
                          ? md3Text(QStringLiteral("md3.runtime.menu"))
                          : tr("Machine menu");
    const QString strMenuDescription = UIMd3Language::instance()
                                     ? md3Text(QStringLiteral("md3.runtime.menu-description"))
                                     : tr("Opens the searchable Machine, View, Input, Devices, and Help menus.");
    updateAccessibleName(m_pMenu, strMenu);
    updateAccessibleDescription(m_pMenu, strMenuDescription);
    m_pMenu->setToolTip(strMenu);

    const QString strMachineIcon = UIMd3Language::instance()
                                 ? md3Text(QStringLiteral("md3.runtime.machine-icon"))
                                 : tr("Virtual machine icon");
    updateAccessibleName(m_pMachineIcon, strMachineIcon);

    const QString strMinimize = UIMd3Language::instance()
                              ? md3Text(QStringLiteral("md3.runtime.minimize"))
                              : tr("Minimize window");
    const QString strMinimizeDescription = UIMd3Language::instance()
                                         ? md3Text(QStringLiteral("md3.runtime.minimize-description"))
                                         : tr("Hides this virtual machine window on the taskbar.");
    updateAccessibleName(m_pMinimize, strMinimize);
    updateAccessibleDescription(m_pMinimize, strMinimizeDescription);
    m_pMinimize->setToolTip(strMinimize);

    const QString strClose = UIMd3Language::instance()
                           ? md3Text(QStringLiteral("md3.runtime.close"))
                           : tr("Close window");
    const QString strCloseDescription = UIMd3Language::instance()
                                      ? md3Text(QStringLiteral("md3.runtime.close-description"))
                                      : tr("Starts the existing virtual machine close flow.");
    updateAccessibleName(m_pClose, strClose);
    updateAccessibleDescription(m_pClose, strCloseDescription);
    m_pClose->setToolTip(strClose);

    updateMaximizeButton();
    sltUpdateWindowTitle();
}

void UIMd3RuntimeHeader::sltUpdateTheme()
{
    QPalette headerPalette = palette();
    headerPalette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    headerPalette.setColor(QPalette::WindowText, md3(UIMd3ColorRole_OnSurface));
    setPalette(headerPalette);
    m_pTitle->setPalette(headerPalette);
    m_pTitle->setFont(md3Theme().font(UIMd3TypeRole_LabelLarge));
    updateIcons();
    sltUpdateWindowTitle();
}

void UIMd3RuntimeHeader::sltUpdateWindowTitle()
{
    if (!m_pWindow || !m_pTitle || !m_pMachineIcon)
        return;

    const QString strTitle = m_pWindow->windowTitle();
    const int iTitleWidth = qMax(0, m_pTitle->width() - 8);
    m_pTitle->setText(iTitleWidth > 0
                    ? QFontMetrics(m_pTitle->font()).elidedText(strTitle, Qt::ElideRight, iTitleWidth)
                    : strTitle);
    updateAccessibleName(m_pTitle, strTitle);
    updateAccessibleDescription(m_pTitle,
                                UIMd3Language::instance()
                                ? md3Text(QStringLiteral("md3.runtime.title-description"))
                                : tr("Current virtual machine and session state."));
    m_pTitle->setToolTip(strTitle);

    QIcon machineIcon = m_pWindow->windowIcon();
    if (machineIcon.isNull())
        machineIcon = UIIconPool::iconSet(QStringLiteral(":/machine_16px.png"));
    m_pMachineIcon->setPixmap(machineIcon.pixmap(QSize(24, 24)));
}

void UIMd3RuntimeHeader::updateMaximizeButton()
{
    if (!m_pWindow || !m_pMaximize)
        return;

    const bool fMaximized = m_pWindow->isMaximized();
    const QString strName = UIMd3Language::instance()
                          ? md3Text(fMaximized
                                    ? QStringLiteral("md3.runtime.restore")
                                    : QStringLiteral("md3.runtime.maximize"))
                          : fMaximized ? tr("Restore window") : tr("Maximize window");
    const QString strDescription = UIMd3Language::instance()
                                 ? md3Text(fMaximized
                                           ? QStringLiteral("md3.runtime.restore-description")
                                           : QStringLiteral("md3.runtime.maximize-description"))
                                 : fMaximized
                                   ? tr("Returns this virtual machine window to its previous size.")
                                   : tr("Expands this virtual machine window to the available desktop.");
    updateAccessibleName(m_pMaximize, strName);
    updateAccessibleDescription(m_pMaximize, strDescription);
    m_pMaximize->setToolTip(strName);
    const QIcon source = fMaximized
                       ? UIIconPool::iconSet(QStringLiteral(":/restore_16px.png"))
                       : QApplication::style()->standardIcon(QStyle::SP_TitleBarMaxButton);
    m_pMaximize->setIcon(md3RuntimeHeaderIcon(source,
                                              md3(UIMd3ColorRole_OnSurfaceVariant),
                                              this));
}

void UIMd3RuntimeHeader::updateIcons()
{
    const QColor color = md3(UIMd3ColorRole_OnSurfaceVariant);
    m_pMenu->setIcon(md3RuntimeHeaderIcon(QStringLiteral(":/tools_menu_24px.png"), color, this));
    m_pMinimize->setIcon(md3RuntimeHeaderIcon(QStringLiteral(":/minimize_16px.png"), color, this));
    m_pClose->setIcon(md3RuntimeHeaderIcon(QStringLiteral(":/close_16px.png"), color, this));
    updateMaximizeButton();
}

/* static */
void UIMd3RuntimeHeader::updateAccessibleName(QWidget *pWidget, const QString &strName)
{
    if (!pWidget || pWidget->accessibleName() == strName)
        return;
    pWidget->setAccessibleName(strName);
    QAccessibleEvent event(pWidget, QAccessible::NameChanged);
    QAccessible::updateAccessibility(&event);
}

/* static */
void UIMd3RuntimeHeader::updateAccessibleDescription(QWidget *pWidget, const QString &strDescription)
{
    if (!pWidget || pWidget->accessibleDescription() == strDescription)
        return;
    pWidget->setAccessibleDescription(strDescription);
    QAccessibleEvent event(pWidget, QAccessible::DescriptionChanged);
    QAccessible::updateAccessibility(&event);
}

void UIMd3RuntimeHeader::mousePressEvent(QMouseEvent *pEvent)
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

void UIMd3RuntimeHeader::resizeEvent(QResizeEvent *pEvent)
{
    QWidget::resizeEvent(pEvent);
    sltUpdateWindowTitle();
}

void UIMd3RuntimeHeader::mouseMoveEvent(QMouseEvent *pEvent)
{
    if (m_fDragging && m_pWindow && !m_pWindow->isMaximized())
    {
        m_pWindow->move(pEvent->globalPosition().toPoint() - m_dragOffset);
        pEvent->accept();
        return;
    }
    QWidget::mouseMoveEvent(pEvent);
}

void UIMd3RuntimeHeader::mouseReleaseEvent(QMouseEvent *pEvent)
{
    m_fDragging = false;
    QWidget::mouseReleaseEvent(pEvent);
}

void UIMd3RuntimeHeader::mouseDoubleClickEvent(QMouseEvent *pEvent)
{
    if (pEvent->button() == Qt::LeftButton)
    {
        toggleMaximize();
        pEvent->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(pEvent);
}
