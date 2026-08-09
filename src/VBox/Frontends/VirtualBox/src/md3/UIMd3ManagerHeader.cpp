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

#include "UIMd3ManagerHeader.h"
#include "UIMd3Button.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"

UIMd3ManagerHeader::UIMd3ManagerHeader(QMainWindow *pWindow, QWidget *pParent /* = 0 */)
    : QWidget(pParent)
    , m_pWindow(pWindow)
    , m_pTitle(0)
    , m_pMaximize(0)
    , m_fDragging(false)
{
    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.application"), QStringLiteral("Material Virtual Machine"), QStringLiteral("Material Virtual Machine"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.menu"), QStringLiteral("Menu"), QStringLiteral("餐牌"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.minimize"), QStringLiteral("Minimize"), QStringLiteral("收埋"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.close"), QStringLiteral("Close"), QStringLiteral("閂埋"));
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

    UIMd3Button *pMenu = new UIMd3Button(md3Text(QStringLiteral("md3.menu")), UIMd3ButtonVariant_Text, this);
    pMenu->setToolTip(tr("Show or hide the application menu"));
    pMenu->setAccessibleName(tr("Show or hide the application menu"));
    pMenu->setMinimumHeight(md3Theme().controlHeight());
    connect(pMenu, &UIMd3Button::sigClicked, pWindow, [pWindow]()
    {
        if (pWindow->menuBar())
            pWindow->menuBar()->setVisible(!pWindow->menuBar()->isVisible());
    });
    pLayout->addWidget(pMenu);

    UIMd3Button *pMinimize = new UIMd3Button(md3Text(QStringLiteral("md3.minimize")), UIMd3ButtonVariant_Text, this);
    pMinimize->setAccessibleName(tr("Minimize window"));
    connect(pMinimize, &UIMd3Button::sigClicked, pWindow, &QWidget::showMinimized);
    pLayout->addWidget(pMinimize);

    UIMd3Language::instance()->registerText(QStringLiteral("md3.maximize"), QStringLiteral("Maximize"), QStringLiteral("放大"));
    m_pMaximize = new UIMd3Button(QString(), UIMd3ButtonVariant_Text, this);
    m_pMaximize->setAccessibleName(tr("Maximize or restore window"));
    connect(m_pMaximize, &UIMd3Button::sigClicked, this, &UIMd3ManagerHeader::toggleMaximize);
    pLayout->addWidget(m_pMaximize);
    updateMaximizeLabel();

    UIMd3Button *pClose = new UIMd3Button(md3Text(QStringLiteral("md3.close")), UIMd3ButtonVariant_Danger, this);
    pClose->setAccessibleName(tr("Close window"));
    connect(pClose, &UIMd3Button::sigClicked, pWindow, &QWidget::close);
    pLayout->addWidget(pClose);
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
