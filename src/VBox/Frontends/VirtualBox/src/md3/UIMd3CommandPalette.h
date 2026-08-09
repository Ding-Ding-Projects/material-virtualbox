/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 command palette.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h
#define FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <functional>

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>

#include "UILibraryDefs.h"

class QKeyEvent;
class QHideEvent;
class QEvent;
class QVBoxLayout;
class UIMd3SearchField;
class UIMd3Button;

/** One command palette entry owned by a live UI surface. */
struct UIMd3Command
{
    UIMd3Command() : pTarget(0) {}
    UIMd3Command(const QString &strTitle, const QString &strSource,
                 const std::function<void()> &handler, QWidget *pTargetWidget = 0,
                 const QString &strCategory = QString(),
                 const QString &strId = QString(),
                 const std::function<bool()> &enabledPredicate = std::function<bool()>(),
                 const QString &strDisabledReason = QString())
        : strTitle(strTitle), strSource(strSource), strCategory(strCategory),
          strId(strId), handler(handler), enabledPredicate(enabledPredicate),
          strDisabledReason(strDisabledReason), pTarget(pTargetWidget) {}

    QString strTitle;
    /** Stable owner identifier used for replacement and unregister. */
    QString strSource;
    /** Localized owner/category shown to the user. */
    QString strCategory;
    /** Stable command identifier used for appearance and replacement. */
    QString strId;
    std::function<void()> handler;
    /** Optional live predicate supplied by the owning action/model. */
    std::function<bool()> enabledPredicate;
    /** Localized explanation shown when the command is unavailable. */
    QString strDisabledReason;
    QPointer<QWidget> pTarget;
};

/** Bounded, searchable command palette shared by the native frontend. */
class SHARED_LIBRARY_STUFF UIMd3CommandPalette : public QDialog
{
    Q_OBJECT;

public:

    static UIMd3CommandPalette *instance();
    static void registerCommand(const UIMd3Command &command);
    static void unregisterSource(const QString &strSource);
    static void showPalette(QWidget *pParent);

protected:

    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;
    virtual void hideEvent(QHideEvent *pEvent) RT_OVERRIDE;
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;

private slots:

    void sltRefresh();

private:

    UIMd3CommandPalette();
    virtual ~UIMd3CommandPalette() RT_OVERRIDE RT_FINAL;
    void prepare();
    static void teleportTo(QWidget *pTarget);
    void restoreOriginFocus();

    static UIMd3CommandPalette *s_pInstance;
    QList<UIMd3Command> m_commands;
    QList<UIMd3Button *> m_pRows;
    UIMd3SearchField *m_pSearchField;
    QVBoxLayout *m_pResultLayout;
    QPointer<QWidget> m_pOrigin;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h */
