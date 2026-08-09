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
#include <QPushButton>

#include "UIMd3ManagerHeader.h"
#include "UIMd3Language.h"
#include "UIMd3Theme.h"

UIMd3ManagerHeader::UIMd3ManagerHeader(QMainWindow *pWindow, QWidget *pParent /* = 0 */)
    : QWidget(pParent)
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

    QLabel *pTitle = new QLabel(md3Theme().brandName(), this);
    pTitle->setFont(md3Theme().font(UIMd3TypeRole_TitleLarge));
    pTitle->setAccessibleName(tr("Application name"));
    pLayout->addWidget(pTitle, 1);
    connect(UIMd3Theme::instance(), &UIMd3Theme::sigThemeChanged, pTitle, [pTitle]()
    {
        pTitle->setText(md3Theme().brandName());
    });
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged, pTitle, [pTitle]()
        {
            pTitle->setText(md3Theme().brandName());
        });

    QPushButton *pMenu = new QPushButton(md3Text(QStringLiteral("md3.menu")), this);
    pMenu->setToolTip(tr("Show or hide the application menu"));
    pMenu->setAccessibleName(tr("Show or hide the application menu"));
    pMenu->setMinimumHeight(md3Theme().controlHeight());
    connect(pMenu, &QPushButton::clicked, pWindow, [pWindow]()
    {
        if (pWindow->menuBar())
            pWindow->menuBar()->setVisible(!pWindow->menuBar()->isVisible());
    });
    pLayout->addWidget(pMenu);

    QPushButton *pMinimize = new QPushButton(md3Text(QStringLiteral("md3.minimize")), this);
    pMinimize->setAccessibleName(tr("Minimize window"));
    connect(pMinimize, &QPushButton::clicked, pWindow, &QWidget::showMinimized);
    pLayout->addWidget(pMinimize);

    QPushButton *pClose = new QPushButton(md3Text(QStringLiteral("md3.close")), this);
    pClose->setAccessibleName(tr("Close window"));
    connect(pClose, &QPushButton::clicked, pWindow, &QWidget::close);
    pLayout->addWidget(pClose);
}
