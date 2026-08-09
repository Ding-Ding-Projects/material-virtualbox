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

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPalette>
#include <QSize>

#include "UIMd3ManagerHeader.h"
#include "UIMd3Button.h"
#include "UIMd3Language.h"
#include "UIMd3NotificationCentre.h"
#include "UIMd3Theme.h"

UIMd3ManagerHeader::UIMd3ManagerHeader(QMainWindow *pWindow, QWidget *pParent /* = 0 */)
    : QWidget(pParent)
    , m_pWindow(pWindow)
    , m_pTitle(0)
    , m_pUnread(0)
    , m_pMenu(0)
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
    }

    setObjectName(QStringLiteral("md3ManagerHeader"));
    setFixedHeight(56);
    setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, md3(UIMd3ColorRole_SurfaceContainer));
    setPalette(palette);

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(20, 8, 12, 8);
    pLayout->setSpacing(8);

    m_pTitle = new QLabel(md3Theme().brandName(), this);
    m_pTitle->setFont(md3Theme().font(UIMd3TypeRole_TitleLarge));
    m_pTitle->setAccessibleName(tr("Application name"));
    m_pTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
    pLayout->addWidget(m_pTitle, 1);
    connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged, m_pTitle, [this]()
    {
        m_pTitle->setText(md3Theme().brandName());
        m_pTitle->setFont(md3Theme().font(UIMd3TypeRole_TitleLarge));
    });
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged, m_pTitle, [this]()
        {
            m_pTitle->setText(md3Theme().brandName());
        });

    m_pNotifications = new UIMd3Button(md3Text(QStringLiteral("md3.notifications.bell")), UIMd3ButtonVariant_Text, this);
    m_pNotifications->setToolTip(tr("Open notification history"));
    m_pNotifications->setAccessibleName(tr("Open notification history"));
    m_pNotifications->setMinimumHeight(md3Theme().controlHeight());
    connect(m_pNotifications, &UIMd3Button::sigClicked, this, [pWindow]()
    {
        if (UIMd3NotificationCentre::instance())
            UIMd3NotificationCentre::instance()->showCentre(pWindow);
    });
    pLayout->addWidget(m_pNotifications);

    m_pUnread = new QLabel(QStringLiteral("●"), this);
    m_pUnread->setMinimumSize(QSize(16, 16));
    m_pUnread->setAlignment(Qt::AlignCenter);
    m_pUnread->setAccessibleName(tr("Unread notifications"));
    m_pUnread->setAttribute(Qt::WA_TransparentForMouseEvents);
    pLayout->addWidget(m_pUnread);
    if (UIMd3NotificationCentre::instance())
        connect(UIMd3NotificationCentre::instance(), &UIMd3NotificationCentre::sigChanged,
                this, &UIMd3ManagerHeader::updateNotificationState);
    connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged,
            this, &UIMd3ManagerHeader::updateNotificationState);
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged, this, [this]()
        {
            if (m_pNotifications)
            {
                const QString strNotifications = md3Text(QStringLiteral("md3.notifications.bell"));
                m_pNotifications->setText(strNotifications);
                m_pNotifications->setAccessibleName(strNotifications);
                m_pNotifications->setToolTip(strNotifications);
            }
            if (m_pUnread)
                m_pUnread->setAccessibleName(tr("Unread notifications"));
        });
    updateNotificationState();

    m_pMenu = new UIMd3Button(md3Text(QStringLiteral("md3.menu")), UIMd3ButtonVariant_Text, this);
    m_pMenu->setToolTip(tr("Show or hide the application menu"));
    m_pMenu->setAccessibleName(tr("Show or hide the application menu"));
    m_pMenu->setMinimumHeight(md3Theme().controlHeight());
    connect(m_pMenu, &UIMd3Button::sigClicked, pWindow, [pWindow]()
    {
        if (pWindow->menuBar())
            pWindow->menuBar()->setVisible(!pWindow->menuBar()->isVisible());
    });
    pLayout->addWidget(m_pMenu);

    m_pMinimize = new UIMd3Button(md3Text(QStringLiteral("md3.minimize")), UIMd3ButtonVariant_Text, this);
    m_pMinimize->setAccessibleName(tr("Minimize window"));
    connect(m_pMinimize, &UIMd3Button::sigClicked, pWindow, &QWidget::showMinimized);
    pLayout->addWidget(m_pMinimize);

    if (UIMd3Language::instance())
        UIMd3Language::instance()->registerText(QStringLiteral("md3.maximize"), QStringLiteral("Maximize"), QStringLiteral("放大"));
    m_pMaximize = new UIMd3Button(QString(), UIMd3ButtonVariant_Text, this);
    m_pMaximize->setAccessibleName(tr("Maximize or restore window"));
    connect(m_pMaximize, &UIMd3Button::sigClicked, this, &UIMd3ManagerHeader::toggleMaximize);
    pLayout->addWidget(m_pMaximize);
    updateMaximizeLabel();

    m_pClose = new UIMd3Button(md3Text(QStringLiteral("md3.close")), UIMd3ButtonVariant_Danger, this);
    m_pClose->setAccessibleName(tr("Close window"));
    connect(m_pClose, &UIMd3Button::sigClicked, pWindow, &QWidget::close);
    pLayout->addWidget(m_pClose);

    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3ManagerHeader::updateChromeText);
    updateChromeText();
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
    m_pMaximize->setText(fMaximized ? tr("Restore") : tr("Maximize"));
    m_pMaximize->setToolTip(fMaximized ? tr("Restore window") : tr("Maximize window"));
}

void UIMd3ManagerHeader::updateChromeText()
{
    if (m_pMenu)
    {
        const QString strMenu = md3Text(QStringLiteral("md3.menu"));
        m_pMenu->setText(strMenu);
        m_pMenu->setAccessibleName(tr("Show or hide the application menu"));
        m_pMenu->setToolTip(tr("Show or hide the application menu"));
    }
    if (m_pMinimize)
    {
        const QString strMinimize = md3Text(QStringLiteral("md3.minimize"));
        m_pMinimize->setText(strMinimize);
        m_pMinimize->setAccessibleName(tr("Minimize window"));
        m_pMinimize->setToolTip(tr("Minimize window"));
    }
    if (m_pMaximize)
        updateMaximizeLabel();
    if (m_pClose)
    {
        const QString strClose = md3Text(QStringLiteral("md3.close"));
        m_pClose->setText(strClose);
        m_pClose->setAccessibleName(tr("Close window"));
        m_pClose->setToolTip(tr("Close window"));
    }
}

void UIMd3ManagerHeader::updateNotificationState()
{
    if (!m_pUnread)
        return;
    const bool fUnread = UIMd3NotificationCentre::instance()
                      && UIMd3NotificationCentre::instance()->hasUnread();
    m_pUnread->setVisible(fUnread);
    QPalette palette = m_pUnread->palette();
    palette.setColor(QPalette::WindowText,
                     fUnread ? md3(UIMd3ColorRole_Error) : md3(UIMd3ColorRole_Primary));
    m_pUnread->setPalette(palette);
    m_pUnread->setAccessibleDescription(fUnread ? tr("Unread notifications are available")
                                                : tr("No unread notifications"));
}

void UIMd3ManagerHeader::mousePressEvent(QMouseEvent *pEvent)
{
    if (pEvent->button() == Qt::LeftButton && m_pWindow && !m_pWindow->isMaximized())
    {
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
