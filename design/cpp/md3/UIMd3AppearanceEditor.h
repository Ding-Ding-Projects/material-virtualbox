/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3AppearanceEditor class declaration - the anchored per-element appearance editor.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3AppearanceEditor_h
#define FEQT_INCLUDED_SRC_md3_UIMd3AppearanceEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDialog>
#include <QString>

/* GUI includes: */
#include "UIMd3Tokens.h"

/* Forward declarations: */
class QComboBox;
class QLabel;
class QSlider;
class UIMd3Button;

/** QDialog extension implementing the anchored appearance editor.
  *
  * Any UIMd3Widget with an appearance key can be edited by Shift+right-clicking
  * it, from the command palette, or from a pane's ✎ affordance. Changes are
  * previewed live and stored per element key in extradata, so they survive
  * restart and roam with VBoxSVC. */
class UIMd3AppearanceEditor : public QDialog
{
    Q_OBJECT;

public:

    /** Opens the editor anchored on @a pElement for @a strKey. */
    static void editElement(QWidget *pElement, const QString &strKey);

    /** Constructs the editor for @a strKey. */
    UIMd3AppearanceEditor(const QString &strKey, QWidget *pParent = 0);

private slots:

    /** Applies the edited appearance to the element. */
    void sltApply();
    /** Removes the override, returning the element to the inherited theme. */
    void sltReset();
    /** Saves the current values as a named theme. */
    void sltSaveNamedTheme();
    /** Refreshes the live preview. */
    void sltPreview();

private:

    /** Prepares all contents. */
    void prepare();
    /** Returns the appearance currently described by the controls. */
    UIMd3Appearance currentAppearance() const;

    QString      m_strKey;
    QComboBox   *m_pFontCombo;
    QComboBox   *m_pSeedCombo;
    QSlider     *m_pRadiusSlider;
    QSlider     *m_pScaleSlider;
    QSlider     *m_pWeightSlider;
    QLabel      *m_pPreview;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3AppearanceEditor_h */
