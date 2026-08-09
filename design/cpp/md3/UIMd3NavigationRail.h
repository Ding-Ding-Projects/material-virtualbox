/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3NavigationRail class declaration - adaptive rail and drawer.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3NavigationRail_h
#define FEQT_INCLUDED_SRC_md3_UIMd3NavigationRail_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QList>

/* GUI includes: */
#include "UIExtraDataDefs.h"
#include "UIMd3Widget.h"

/** UIMd3Widget extension implementing the Material 3 navigation rail.
  *
  * Above 1000px it is a rail with icon-and-label destinations; below that it
  * collapses into a modal drawer opened from the title bar, which is the
  * adaptive behaviour the M3 spec prescribes and what keeps the Manager usable
  * on a small laptop screen. */
class UIMd3NavigationRail : public UIMd3Widget
{
    Q_OBJECT;

signals:

    /** Notifies listeners that @a enmType was activated. */
    void sigDestinationActivated(UIToolType enmType);

public:

    /** Constructs the rail. */
    UIMd3NavigationRail(QWidget *pParent = 0);

    /** Adds a destination for @a enmType showing @a strLabel and @a strIcon. */
    void addDestination(UIToolType enmType, const QString &strLabel, const QString &strIcon);
    /** Makes @a enmType current. */
    void setCurrentDestination(UIToolType enmType);
    /** Returns the current destination. */
    UIToolType currentDestination() const { return m_enmCurrent; }
    /** Switches between rail and drawer presentation. */
    void setDrawerMode(bool fDrawer);

protected:

    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    virtual QSize sizeHint() const RT_OVERRIDE;

private:

    struct Destination { UIToolType enmType; QString strLabel; QIcon icon; QRect rect; };

    QList<Destination> m_destinations;
    UIToolType         m_enmCurrent;
    bool               m_fDrawerMode;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3NavigationRail_h */
