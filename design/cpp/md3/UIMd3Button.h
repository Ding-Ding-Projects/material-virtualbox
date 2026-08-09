/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Button class declaration - the five Material 3 button variants.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Button_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Button_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QIcon>
#include <QString>

/* GUI includes: */
#include "UIMd3Widget.h"

/** The Material 3 button variants. */
enum UIMd3ButtonVariant
{
    UIMd3ButtonVariant_Filled,
    UIMd3ButtonVariant_Tonal,
    UIMd3ButtonVariant_Outlined,
    UIMd3ButtonVariant_Text,
    UIMd3ButtonVariant_Elevated,
    UIMd3ButtonVariant_Icon,
    UIMd3ButtonVariant_Danger
};

/** UIMd3Widget extension implementing every Material 3 button variant.
  * Replaces QPushButton and QToolButton across the frontend. */
class UIMd3Button : public UIMd3Widget
{
    Q_OBJECT;

signals:

    /** Notifies listeners that the button was activated by mouse or keyboard. */
    void sigClicked();

public:

    /** Constructs a button showing @a strText with @a enmVariant. */
    UIMd3Button(const QString &strText, UIMd3ButtonVariant enmVariant = UIMd3ButtonVariant_Tonal, QWidget *pParent = 0);

    /** Returns the button text. */
    QString text() const { return m_strText; }
    /** Defines the button @a strText. */
    void setText(const QString &strText);
    /** Defines the leading @a icon. */
    void setIcon(const QIcon &icon);
    /** Defines the button @a enmVariant. */
    void setVariant(UIMd3ButtonVariant enmVariant);
    /** Defines whether the button is @a fEnabled, repainting the disabled state layer. */
    void setEnabledState(bool fEnabled);

    /** Returns the size hint for the active density. */
    virtual QSize sizeHint() const RT_OVERRIDE;
    /** Returns the minimum size hint, never below the 48px touch target. */
    virtual QSize minimumSizeHint() const RT_OVERRIDE;

protected:

    /** Handles paint events. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    /** Handles mouse-release events, to emit sigClicked(). */
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    /** Handles key-press events, so Space and Return activate the button. */
    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;

private:

    /** Returns the container colour role for the active variant. */
    UIMd3ColorRole containerRole() const;
    /** Returns the label colour role for the active variant. */
    UIMd3ColorRole labelRole() const;

    QString            m_strText;
    QIcon              m_icon;
    UIMd3ButtonVariant m_enmVariant;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Button_h */
