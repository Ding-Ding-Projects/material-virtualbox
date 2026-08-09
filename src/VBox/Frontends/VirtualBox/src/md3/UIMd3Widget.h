/* $Id$ */
/** @file
 * VBox Qt GUI - shared token-aware base for Material 3 widgets.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Widget_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Widget_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QPainter>
#include <QPainterPath>
#include <QString>
#include <QWidget>

#include "UILibraryDefs.h"
#include "UIMd3Theme.h"
#include "UIMd3Tokens.h"

/** Shared state and appearance plumbing for native Material 3 widgets. */
class SHARED_LIBRARY_STUFF UIMd3Widget : public QWidget
{
    Q_OBJECT;

signals:

    /** Requests the owning surface open the element appearance editor. */
    void sigAppearanceEditRequested(const QString &strAppearanceKey);

public:

    /** Constructs a widget with a stable appearance key. */
    explicit UIMd3Widget(QWidget *pParent = 0, const QString &strAppearanceKey = QString());

    QString appearanceKey() const { return m_strAppearanceKey; }
    void setAppearanceKey(const QString &strKey);

    int effectiveRadius() const;
    QFont effectiveFont(UIMd3TypeRole enmRole) const;
    QColor effectiveAccent() const;

protected:

    void paintContainer(QPainter &painter, const QRect &rect, UIMd3ColorRole enmRole,
                        int iRadius = -1) const;
    void paintStateLayer(QPainter &painter, const QRect &rect, UIMd3ColorRole enmRole,
                         int iRadius = -1) const;
    void paintFocusRing(QPainter &painter, const QRect &rect, int iRadius = -1) const;

    virtual void enterEvent(QEnterEvent *pEvent) RT_OVERRIDE;
    virtual void leaveEvent(QEvent *pEvent) RT_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *pEvent) RT_OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) RT_OVERRIDE;

    bool m_fHovered;
    bool m_fPressed;

private slots:

    void sltThemeChanged();

private:

    QString m_strAppearanceKey;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Widget_h */
