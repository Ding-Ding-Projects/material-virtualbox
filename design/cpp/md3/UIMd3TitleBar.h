/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3TitleBar class declaration - frameless Material 3 title bar.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3TitleBar_h
#define FEQT_INCLUDED_SRC_md3_UIMd3TitleBar_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* GUI includes: */
#include "UIMd3Widget.h"

/* Forward declarations: */
class QLabel;
class UIMd3Button;

/** UIMd3Widget extension implementing the frameless window chrome.
  *
  * It carries the user-renamable wordmark and application name, the global
  * search entry into the command palette, the notification bell with its unread
  * dot, and the three window buttons. The bar itself is the drag region, so the
  * window still moves the way an operator expects without a native frame. */
class UIMd3TitleBar : public UIMd3Widget
{
    Q_OBJECT;

signals:

    void sigPaletteRequested();
    void sigNotificationsRequested();
    void sigMinimizeRequested();
    void sigMaximizeRequested();
    void sigCloseRequested();

public:

    /** Constructs the title bar. */
    UIMd3TitleBar(QWidget *pParent = 0);

    /** Defines the window @a strTitle shown next to the wordmark. */
    void setTitleText(const QString &strTitle);
    /** Re-reads the user's application name and wordmark letter from preferences. */
    void refreshBranding();

protected:

    /** Handles paint events. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    /** Handles mouse-press events, to begin a system move. */
    virtual void mousePressEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    /** Handles double clicks, to toggle maximised. */
    virtual void mouseDoubleClickEvent(QMouseEvent *pEvent) RT_OVERRIDE;

private:

    /** Prepares all contents. */
    void prepare();

    QLabel      *m_pWordmark;
    QLabel      *m_pTitle;
    UIMd3Button *m_pSearchButton;
    UIMd3Button *m_pBellButton;
    UIMd3Button *m_pMinimizeButton;
    UIMd3Button *m_pMaximizeButton;
    UIMd3Button *m_pCloseButton;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3TitleBar_h */
