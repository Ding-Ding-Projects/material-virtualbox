/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 local menu search helper.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QAction>
#include <QHash>
#include <QKeySequence>
#include <QMenu>
#include <QObject>
#include <QWidgetAction>

/* GUI includes: */
#include "UIMd3MenuSearch.h"
#include "UIMd3SearchField.h"

static QString md3MenuSearchCandidate(QAction *pAction)
{
    if (!pAction)
        return QString();
    QString strResult = pAction->text();
    strResult.remove('&');
    strResult += QLatin1Char(' ');
    strResult += pAction->statusTip();
    strResult += QLatin1Char(' ');
    strResult += pAction->toolTip();
    strResult += QLatin1Char(' ');
    strResult += pAction->whatsThis();
    strResult += QLatin1Char(' ');
    strResult += pAction->shortcut().toString(QKeySequence::NativeText);
    return strResult;
}

void md3PrepareSearchableMenu(QMenu *pMenu,
                              const QString &strFieldId,
                              const QString &strPlaceholder,
                              const QString &strAccessibleName)
{
    if (!pMenu)
        return;

    const QList<QAction*> actions = pMenu->actions();
    QHash<QAction*, bool> originalVisibility;
    for (QAction *pAction : actions)
        if (pAction)
            originalVisibility.insert(pAction, pAction->isVisible());
    UIMd3SearchField *pSearch = new UIMd3SearchField(strFieldId, strPlaceholder, pMenu);
    pSearch->setAccessibleName(strAccessibleName);
    pSearch->setAccessibleDescription(pMenu->title().isEmpty()
                                      ? strAccessibleName
                                      : pMenu->title());
    pSearch->setMinimumWidth(280);

    QWidgetAction *pSearchAction = new QWidgetAction(pMenu);
    pSearchAction->setDefaultWidget(pSearch);
    QAction *pBefore = actions.isEmpty() ? 0 : actions.first();
    pMenu->insertAction(pBefore, pSearchAction);
    QAction *pSeparator = pMenu->insertSeparator(pBefore);

    QObject::connect(pSearch, &UIMd3SearchField::sigFilterChanged, pMenu,
                     [pSearch, actions, originalVisibility, pSeparator]()
    {
        int cVisible = 0;
        for (QAction *pAction : actions)
        {
            if (!pAction || pAction->isSeparator())
                continue;
            const bool fVisible = originalVisibility.value(pAction, false)
                               && pSearch->matches(md3MenuSearchCandidate(pAction));
            pAction->setVisible(fVisible);
            if (fVisible)
                ++cVisible;
        }
        if (pSeparator)
            pSeparator->setVisible(cVisible > 0);
    });
    QObject::connect(pMenu, &QMenu::aboutToHide, pMenu,
                     [actions, originalVisibility]()
    {
        /* Several callers intentionally reuse action-pool or menu-bar actions.
         * Menu-local filtering must not leak visibility changes back to those
         * authoritative surfaces after this popup closes. */
        for (QAction *pAction : actions)
            if (pAction)
                pAction->setVisible(originalVisibility.value(pAction, false));
    });

    pSearch->setFocus(Qt::PopupFocusReason);
}
