/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3CommandPalette class declaration - the Ctrl+Shift+F palette.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h
#define FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>

/* Forward declarations: */
class QVBoxLayout;
class UIMd3SearchField;

/** One palette entry. */
struct UIMd3Command
{
    UIMd3Command() : pTarget(0) {}
    UIMd3Command(const QString &strTitle, const QString &strSource, std::function<void()> handler, QWidget *pTargetWidget = 0)
        : strTitle(strTitle), strSource(strSource), handler(handler), pTarget(pTargetWidget) {}

    QString               strTitle;   /**< User visible title. */
    QString               strSource;  /**< Where the command lives, shown on the right. */
    std::function<void()> handler;    /**< What running it does. */
    QPointer<QWidget>     pTarget;    /**< Optional element to reveal, focus and flash. */
};

/** QDialog extension implementing the product-wide command palette.
  *
  * Commands are registered by every surface as it is constructed, so the palette
  * always reflects what actually exists in the running GUI. A result with a
  * target widget does not merely navigate: it raises the owning window, switches
  * to the owning page, scrolls the element into view and flashes it. */
class UIMd3CommandPalette : public QDialog
{
    Q_OBJECT;

public:

    /** Returns the palette singleton, creating it on first use. */
    static UIMd3CommandPalette *instance();
    /** Registers @a command. Registration is idempotent by title and source. */
    static void registerCommand(const UIMd3Command &command);
    /** Removes every command registered by @a strSource. */
    static void unregisterSource(const QString &strSource);
    /** Shows the palette над @a pParent. */
    static void showPalette(QWidget *pParent);

private slots:

    /** Refreshes the result list for the current query. */
    void sltRefresh();

private:

    /** Constructs the palette. */
    UIMd3CommandPalette();

    /** Prepares all contents. */
    void prepare();
    /** Reveals, focuses and flashes @a pTarget. */
    static void teleportTo(QWidget *pTarget);

    static UIMd3CommandPalette *s_pInstance;
    QList<UIMd3Command>         m_commands;
    UIMd3SearchField           *m_pSearchField;
    QVBoxLayout                *m_pResultLayout;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h */
