/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 wizard shell declaration.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Wizard_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Wizard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QStringList>
#include <QList>
#include <QWidget>

#include "UILibraryDefs.h"

class QLabel;
class QPaintEvent;
class QResizeEvent;
class QHBoxLayout;
class QVBoxLayout;

/** Token-aware shell for the existing native wizard page stack.
  *
  * The shell owns presentation only.  Page construction, validation,
  * navigation and notification progress remain owned by UINativeWizard. */
class SHARED_LIBRARY_STUFF UIMd3Wizard : public QWidget
{
    Q_OBJECT;

public:

    /** Constructs the shell passing @a pParent to the base class. */
    explicit UIMd3Wizard(QWidget *pParent = 0);

    /** Returns the title label used by UINativeWizard for the active page. */
    QLabel *pageTitleLabel() const { return m_pPageTitle; }

    /** Adds the existing page stack to the shell. */
    void setContentWidget(QWidget *pWidget);

    /** Adds the existing native wizard action row to the shell. */
    void setActionWidget(QWidget *pWidget);

    /** Rebuilds the visible stepper from translated page titles. */
    void setStepTitles(const QStringList &titles);

    /** Marks @a iIndex as the current page. */
    void setCurrentStep(int iIndex);

    /** Marks @a iIndex as complete or incomplete. */
    void setStepComplete(int iIndex, bool fComplete);

protected:

    /** Paints the Material 3 surface and focus ring. */
    virtual void paintEvent(QPaintEvent *pEvent) RT_OVERRIDE;
    /** Switches between the full step rail and compact progress presentation. */
    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;

private:

    /** Reapplies live Material 3 tokens after a theme change. */
    void sltThemeChanged();
    /** Retranslates shell-owned text after a Material language change. */
    void sltLanguageChanged();

    /** Clears all step widgets while retaining the layout. */
    void clearSteps();
    /** Updates the full or compact step presentation for the current width. */
    void updateResponsiveLayout();

    QLabel     *m_pPageTitle;
    QLabel     *m_pStepSummary;
    QWidget    *m_pStepPanel;
    QWidget    *m_pPagePanel;
    QVBoxLayout *m_pLayout;
    QHBoxLayout *m_pBodyLayout;
    QVBoxLayout *m_pStepLayout;
    QVBoxLayout *m_pContentLayout;
    QVBoxLayout *m_pActionLayout;
    QList<QLabel*> m_steps;
    QStringList m_stepTitles;
    int         m_iCurrentStep;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Wizard_h */
