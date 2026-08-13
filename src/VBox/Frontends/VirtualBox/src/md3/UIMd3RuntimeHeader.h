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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3RuntimeHeader_h
#define FEQT_INCLUDED_SRC_md3_UIMd3RuntimeHeader_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QPoint>
#include <QWidget>

/* Other VBox includes: */
#include <iprt/cdefs.h>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QLabel;
class QMainWindow;
class QMenu;
class QMenuBar;
class QEvent;
class QMouseEvent;
class QResizeEvent;
class UIMd3Button;

/** Material 3 title and menu header for a normal machine window.
  *
  * The runtime menu bar remains the owner of every existing QAction and QMenu;
  * this widget only provides a compact searchable entry point to those actions
  * and native-equivalent top-level window controls. */
class SHARED_LIBRARY_STUFF UIMd3RuntimeHeader : public QWidget
{
    Q_OBJECT;

public:

    /** Constructs a runtime header for @a pWindow using @a pMenuBar. */
    UIMd3RuntimeHeader(QMainWindow *pWindow, QMenuBar *pMenuBar, QWidget *pParent = 0);
    /** Opens the same searchable runtime action surface used by the menu button. */
    void showApplicationMenu();

protected:

    /** Tracks title, icon, screen, and window-state changes from the owning window. */
    virtual bool eventFilter(QObject *pObject, QEvent *pEvent) RT_OVERRIDE;
    /** Toggles the owning window between maximized and normal states. */
    virtual void mouseDoubleClickEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    /** Refreshes title elision after the available header width changes. */
    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;
    /** Continues the fallback native move operation. */
    virtual void mouseMoveEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    /** Starts a native system move from the non-interactive header surface. */
    virtual void mousePressEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    /** Ends the fallback move operation. */
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) RT_OVERRIDE;

private slots:

    /** Rebuilds localized labels and descriptions. */
    void sltRetranslateUI();
    /** Rebuilds token-derived colours, fonts, and icons. */
    void sltUpdateTheme();
    /** Synchronizes the visible title with the owning window. */
    void sltUpdateWindowTitle();

private:

    /** Toggles the owning window between maximized and normal states. */
    void toggleMaximize();
    /** Updates the maximize/restore control for the current state. */
    void updateMaximizeButton();
    /** Rebuilds all token-coloured icons. */
    void updateIcons();
    /** Rebuilds one owned searchable proxy menu from @a pSource. */
    void populateProxyMenu(QMenu *pTarget, QMenu *pSource, const QString &strPath);
    /** Updates an accessible name and emits its change event when required. */
    static void updateAccessibleName(QWidget *pWidget, const QString &strName);
    /** Updates an accessible description and emits its change event when required. */
    static void updateAccessibleDescription(QWidget *pWidget, const QString &strDescription);

    QMainWindow *m_pWindow;
    QMenuBar    *m_pMenuBar;
    QLabel      *m_pMachineIcon;
    QLabel      *m_pTitle;
    UIMd3Button *m_pMenu;
    UIMd3Button *m_pMinimize;
    UIMd3Button *m_pMaximize;
    UIMd3Button *m_pClose;
    bool         m_fDragging;
    QPoint       m_dragOffset;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3RuntimeHeader_h */
