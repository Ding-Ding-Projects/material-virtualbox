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
#include <QKeySequence>
#include <QMenu>
#include <QObject>
#include <QPointer>
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

static void md3SynchronizeMenuProxy(QAction *pProxy, QAction *pOriginal)
{
    if (!pProxy || !pOriginal)
        return;
    pProxy->setText(pOriginal->text());
    pProxy->setIcon(pOriginal->icon());
    pProxy->setIconText(pOriginal->iconText());
    pProxy->setToolTip(pOriginal->toolTip());
    pProxy->setStatusTip(pOriginal->statusTip());
    pProxy->setWhatsThis(pOriginal->whatsThis());
    pProxy->setShortcut(pOriginal->shortcut());
    pProxy->setShortcutContext(pOriginal->shortcutContext());
    pProxy->setCheckable(pOriginal->isCheckable());
    pProxy->setChecked(pOriginal->isChecked());
    pProxy->setEnabled(pOriginal->isEnabled());
    pProxy->setMenu(pOriginal->menu());
}

void md3PrepareSearchableMenu(QMenu *pMenu,
                              const QString &strFieldId,
                              const QString &strPlaceholder,
                              const QString &strAccessibleName)
{
    if (!pMenu)
        return;

    UIMd3SearchField *pSearch = new UIMd3SearchField(strFieldId, strPlaceholder, pMenu);
    pSearch->setAccessibleName(strAccessibleName);
    pSearch->setAccessibleDescription(pMenu->title().isEmpty()
                                      ? strAccessibleName
                                      : pMenu->title());
    pSearch->setMinimumWidth(280);

    const QList<QAction*> originalActions = pMenu->actions();
    QList<QAction*> proxyActions;
    for (QAction *pOriginal : originalActions)
    {
        if (!pOriginal)
            continue;
        QAction *pProxy = new QAction(pMenu);
        pProxy->setSeparator(pOriginal->isSeparator());
        md3SynchronizeMenuProxy(pProxy, pOriginal);
        pProxy->setProperty("md3OriginalVisible", pOriginal->isVisible());
        pProxy->setVisible(pOriginal->isVisible());
        pMenu->insertAction(pOriginal, pProxy);
        pMenu->removeAction(pOriginal);
        proxyActions << pProxy;
        const QPointer<QAction> original(pOriginal);
        QObject::connect(pProxy, &QAction::triggered, pMenu,
                         [original](bool)
        {
            if (!original || !original->isEnabled())
                return;
            original->trigger();
        });
        QObject::connect(pOriginal, &QAction::changed, pMenu,
                         [pProxy, original, pSearch]()
        {
            if (original)
            {
                md3SynchronizeMenuProxy(pProxy, original);
                const bool fBaseVisible = original->isVisible();
                pProxy->setProperty("md3OriginalVisible", fBaseVisible);
                const bool fQueryActive = pSearch->isRegexActive()
                                       || !pSearch->text().isEmpty();
                pProxy->setVisible(fBaseVisible
                                && (pProxy->isSeparator()
                                    ? !fQueryActive
                                    : pSearch->matches(md3MenuSearchCandidate(pProxy))));
            }
        });
    }

    QWidgetAction *pSearchAction = new QWidgetAction(pMenu);
    pSearchAction->setDefaultWidget(pSearch);
    QAction *pBefore = proxyActions.isEmpty() ? 0 : proxyActions.first();
    pMenu->insertAction(pBefore, pSearchAction);
    QAction *pSeparator = pMenu->insertSeparator(pBefore);

    QObject::connect(pSearch, &UIMd3SearchField::sigFilterChanged, pMenu,
                     [pSearch, proxyActions, pSeparator]()
    {
        int cVisible = 0;
        const bool fQueryActive = pSearch->isRegexActive()
                               || !pSearch->text().isEmpty();
        for (QAction *pAction : proxyActions)
        {
            if (!pAction)
                continue;
            if (pAction->isSeparator())
            {
                pAction->setVisible(pAction->property("md3OriginalVisible").toBool()
                                 && !fQueryActive);
                continue;
            }
            const bool fVisible = pAction->property("md3OriginalVisible").toBool()
                               && pSearch->matches(md3MenuSearchCandidate(pAction));
            pAction->setVisible(fVisible);
            if (fVisible)
                ++cVisible;
        }
        if (pSeparator)
            pSeparator->setVisible(cVisible > 0);
    });
    pSearch->setFocus(Qt::PopupFocusReason);
}
