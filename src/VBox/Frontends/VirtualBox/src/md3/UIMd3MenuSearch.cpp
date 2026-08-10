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

/** Applies the current query to @a proxyActions and hides the leading separator when
  * nothing matches. Proxies whose original action has gone are simply skipped. */
static void md3ApplyMenuSearchFilter(UIMd3SearchField *pSearch,
                                     const QList<QPointer<QAction> > &proxyActions,
                                     QAction *pSeparator)
{
    if (!pSearch)
        return;

    int cVisible = 0;
    const bool fQueryActive = pSearch->isRegexActive()
                           || !pSearch->text().isEmpty();
    for (const QPointer<QAction> &action : proxyActions)
    {
        QAction *pAction = action.data();
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
}

void md3PrepareSearchableMenu(QMenu *pMenu,
                              const QString &strFieldId,
                              const QString &strPlaceholder,
                              const QString &strAccessibleName)
{
    /* Preparing the same menu twice would add a second search field and proxy every
     * proxy, so the menu remembers that it has already been prepared: */
    if (!pMenu || pMenu->property("md3SearchPrepared").toBool())
        return;
    pMenu->setProperty("md3SearchPrepared", true);

    UIMd3SearchField *pSearch = new UIMd3SearchField(strFieldId, strPlaceholder, pMenu);
    pSearch->setAccessibleName(strAccessibleName);
    pSearch->setAccessibleDescription(pMenu->title().isEmpty()
                                      ? strAccessibleName
                                      : pMenu->title());
    pSearch->setMinimumWidth(280);

    const QList<QAction*> originalActions = pMenu->actions();
    QList<QPointer<QAction> > originalGuards;
    QList<QPointer<QAction> > proxyActions;
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
        const QPointer<QAction> original(pOriginal);
        const QPointer<QAction> proxy(pProxy);
        originalGuards << original;
        proxyActions << proxy;
        QObject::connect(pProxy, &QAction::triggered, pMenu,
                         [original](bool)
        {
            if (!original || !original->isEnabled())
                return;
            original->trigger();
        });
    }

    QWidgetAction *pSearchAction = new QWidgetAction(pMenu);
    pSearchAction->setDefaultWidget(pSearch);
    QAction *pBefore = proxyActions.isEmpty() ? 0 : proxyActions.first().data();
    pMenu->insertAction(pBefore, pSearchAction);
    QAction *pSeparator = pMenu->insertSeparator(pBefore);

    const QPointer<UIMd3SearchField> search(pSearch);
    const QPointer<QAction> separator(pSeparator);
    const auto refreshFilter = [search, proxyActions, separator]()
    {
        md3ApplyMenuSearchFilter(search.data(), proxyActions, separator.data());
    };

    for (int i = 0; i < originalGuards.size(); ++i)
    {
        const QPointer<QAction> original = originalGuards.at(i);
        const QPointer<QAction> proxy = proxyActions.at(i);
        if (!original)
            continue;
        QObject::connect(original.data(), &QAction::changed, pMenu,
                         [original, proxy, refreshFilter]()
        {
            if (!original || !proxy)
                return;
            md3SynchronizeMenuProxy(proxy.data(), original.data());
            proxy->setProperty("md3OriginalVisible", original->isVisible());
            refreshFilter();
        });
        /* An original action can outlive neither its owner nor this menu: retire the
         * proxy with it so a stale row cannot be triggered. */
        QObject::connect(original.data(), &QObject::destroyed, pMenu,
                         [proxy, refreshFilter]()
        {
            if (proxy)
            {
                proxy->setVisible(false);
                proxy->deleteLater();
            }
            refreshFilter();
        });
    }

    QObject::connect(pSearch, &UIMd3SearchField::sigFilterChanged,
                     pMenu, refreshFilter);
    /* Run once so separator visibility is right before the first keystroke: */
    refreshFilter();
    pSearch->setFocus(Qt::PopupFocusReason);
}
