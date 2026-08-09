/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 button variants.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Button_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Button_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QIcon>
#include <QString>

#include "UILibraryDefs.h"
#include "UIMd3Widget.h"

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

/** Native token-aware button with keyboard and accessible activation. */
class SHARED_LIBRARY_STUFF UIMd3Button : public UIMd3Widget
{
    Q_OBJECT;

signals:

    void sigClicked();

public:

    explicit UIMd3Button(const QString &strText,
                         UIMd3ButtonVariant enmVariant = UIMd3ButtonVariant_Tonal,
                         QWidget *pParent = 0);

    QString text() const { return m_strText; }
    void setText(const QString &strText);
    void setIcon(const QIcon &icon);
    void setVariant(UIMd3ButtonVariant enmVariant);
    void setEnabledState(bool fEnabled);
    /** Keeps a row focusable while preventing activation (for unavailable commands). */
    void setActivationEnabled(bool fEnabled);
    bool isActivationEnabled() const { return m_fActivationEnabled; }
    /** Activates the button when it is available. */
    void click();

    virtual QSize sizeHint() const RT_OVERRIDE;
    virtual QSize minimumSizeHint() const RT_OVERRIDE;

protected:

    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;

private:

    UIMd3ColorRole containerRole() const;
    UIMd3ColorRole labelRole() const;

    QString m_strText;
    QIcon m_icon;
    UIMd3ButtonVariant m_enmVariant;
    bool m_fActivationEnabled;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Button_h */
