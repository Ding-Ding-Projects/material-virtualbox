/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3CommandPalette class implementation.
 */

/*
 * Copyright (C) 2026 Material Virtual Machine contributors.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


/* Qt includes: */
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

/* GUI includes: */
#include "UIMd3Button.h"
#include "UIMd3CommandPalette.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

UIMd3CommandPalette *UIMd3CommandPalette::s_pInstance = 0;

UIMd3CommandPalette *UIMd3CommandPalette::instance()
{
    if (!s_pInstance)
        s_pInstance = new UIMd3CommandPalette;
    return s_pInstance;
}

void UIMd3CommandPalette::registerCommand(const UIMd3Command &command)
{
    UIMd3CommandPalette *pPalette = instance();
    for (int i = 0; i < pPalette->m_commands.size(); ++i)
        if (pPalette->m_commands.at(i).strTitle == command.strTitle && pPalette->m_commands.at(i).strSource == command.strSource)
        {
            pPalette->m_commands[i] = command;
            return;
        }
    pPalette->m_commands << command;
}

void UIMd3CommandPalette::unregisterSource(const QString &strSource)
{
    UIMd3CommandPalette *pPalette = instance();
    for (int i = pPalette->m_commands.size() - 1; i >= 0; --i)
        if (pPalette->m_commands.at(i).strSource == strSource)
            pPalette->m_commands.removeAt(i);
}

void UIMd3CommandPalette::showPalette(QWidget *pParent)
{
    UIMd3CommandPalette *pPalette = instance();
    pPalette->setParent(pParent, Qt::Dialog);
    pPalette->sltRefresh();
    pPalette->show();
    pPalette->raise();
    pPalette->activateWindow();
}

UIMd3CommandPalette::UIMd3CommandPalette()
    : QDialog(0)
    , m_pSearchField(0)
    , m_pResultLayout(0)
{
    prepare();
}

void UIMd3CommandPalette::prepare()
{
    setWindowTitle(tr("Command palette"));
    resize(720, 520);

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(24, 22, 24, 22);
    pLayout->setSpacing(12);

    QLabel *pTitle = new QLabel(tr("Command palette"), this);
    pTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pLayout->addWidget(pTitle);

    m_pSearchField = new UIMd3SearchField("command-palette", tr("Search commands, machines, tools and settings"), this);
    connect(m_pSearchField, &UIMd3SearchField::sigFilterChanged, this, &UIMd3CommandPalette::sltRefresh);
    pLayout->addWidget(m_pSearchField);

    QScrollArea *pScroll = new QScrollArea(this);
    pScroll->setWidgetResizable(true);
    pScroll->setFrameShape(QFrame::NoFrame);
    QWidget *pResults = new QWidget(pScroll);
    m_pResultLayout = new QVBoxLayout(pResults);
    m_pResultLayout->setContentsMargins(0, 0, 0, 0);
    m_pResultLayout->setSpacing(4);
    pScroll->setWidget(pResults);
    pLayout->addWidget(pScroll, 1);
}

void UIMd3CommandPalette::sltRefresh()
{
    AssertPtrReturnVoid(m_pResultLayout);

    /* Drop the previous result set: */
    while (QLayoutItem *pItem = m_pResultLayout->takeAt(0))
    {
        if (pItem->widget())
            pItem->widget()->deleteLater();
        delete pItem;
    }

    int cShown = 0;
    foreach (const UIMd3Command &command, m_commands)
    {
        if (!m_pSearchField->matches(command.strTitle + " " + command.strSource))
            continue;

        UIMd3Button *pRow = new UIMd3Button(command.strTitle, UIMd3ButtonVariant_Tonal, this);
        pRow->setToolTip(command.strSource);
        const UIMd3Command captured = command;
        connect(pRow, &UIMd3Button::sigClicked, this, [this, captured]()
        {
            hide();
            if (captured.handler)
                captured.handler();
            if (captured.pTarget)
                teleportTo(captured.pTarget);
        });
        m_pResultLayout->addWidget(pRow);
        ++cShown;
    }

    if (cShown == 0)
        m_pResultLayout->addWidget(new QLabel(tr("No command matches this search."), this));
    m_pResultLayout->addStretch(1);
}

void UIMd3CommandPalette::teleportTo(QWidget *pTarget)
{
    AssertPtrReturnVoid(pTarget);

    /* Raise the owning window and make every ancestor page current: */
    QWidget *pWindow = pTarget->window();
    if (pWindow)
    {
        pWindow->show();
        pWindow->raise();
        pWindow->activateWindow();
    }
    for (QWidget *pAncestor = pTarget->parentWidget(); pAncestor; pAncestor = pAncestor->parentWidget())
        if (QStackedWidget *pStack = qobject_cast<QStackedWidget*>(pAncestor))
            pStack->setCurrentWidget(pTarget);

    pTarget->setFocus(Qt::ShortcutFocusReason);

    /* Flash the element so the user can see exactly what the palette landed on.
     * The flash is a graphics effect rather than a stylesheet, so it never
     * disturbs the element's own painting. */
    QGraphicsDropShadowEffect *pFlash = new QGraphicsDropShadowEffect(pTarget);
    pFlash->setColor(md3(UIMd3ColorRole_Primary));
    pFlash->setBlurRadius(40);
    pFlash->setOffset(0);
    pTarget->setGraphicsEffect(pFlash);
    QTimer::singleShot(UIMd3Motion::Long2 * 2, pTarget, [pTarget]() { pTarget->setGraphicsEffect(0); });
}
