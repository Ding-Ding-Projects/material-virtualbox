/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3DestructiveGate class declaration - the two-key destructive-action gate.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3DestructiveGate_h
#define FEQT_INCLUDED_SRC_md3_UIMd3DestructiveGate_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDialog>
#include <QString>

/* Forward declarations: */
class QSlider;
class UIMd3Button;

/** QDialog extension implementing the two-key plus slider gate that guards
  * every irreversible action: machine removal, snapshot deletion, saved-state
  * discard, medium removal and extension-pack uninstall.
  *
  * Both keys must be held before the slider unlocks, and the slider must reach
  * its far end before the authorising button becomes usable. The emergency exit
  * is always reachable by keyboard and is the default button. */
class UIMd3DestructiveGate : public QDialog
{
    Q_OBJECT;

public:

    /** Runs the gate for @a strTitle with @a strConsequence and returns whether the user authorised it. */
    static bool authorize(QWidget *pParent, const QString &strTitle, const QString &strConsequence);

    /** Constructs the gate. */
    UIMd3DestructiveGate(const QString &strTitle, const QString &strConsequence, QWidget *pParent = 0);

private slots:

    /** Handles a key toggle. */
    void sltKeyToggled();
    /** Handles slider movement. */
    void sltSliderMoved(int iValue);

private:

    /** Prepares all contents. */
    void prepare();
    /** Re-evaluates whether the action may be authorised. */
    void reevaluate();

    QString      m_strTitle;
    QString      m_strConsequence;
    UIMd3Button *m_pLeftKey;
    UIMd3Button *m_pRightKey;
    QSlider     *m_pSlider;
    UIMd3Button *m_pAuthorizeButton;
    bool         m_fLeftHeld;
    bool         m_fRightHeld;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3DestructiveGate_h */
