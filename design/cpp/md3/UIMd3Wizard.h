/* $Id$ */
/** @file
 * VBox Qt GUI - UIMd3Wizard class declaration - Material 3 wizard shell.
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

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3Wizard_h
#define FEQT_INCLUDED_SRC_md3_UIMd3Wizard_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/* Qt includes: */
#include <QDialog>
#include <QList>

/* Forward declarations: */
class QStackedWidget;
class UIMd3Button;
class UINativeWizardPage;

/** QDialog extension replacing UINativeWizard with the Material 3 shell.
  *
  * Every existing UINativeWizardPage subclass is hosted unchanged, so New VM,
  * Clone, Export, Import and Create Virtual Hard Disk keep their real page
  * logic and validation. The shell adds the step list, the per-step validity
  * message, and a Basic/Expert switch that unlocks direct navigation instead of
  * silently swapping the page set. */
class UIMd3Wizard : public QDialog
{
    Q_OBJECT;

public:

    /** Constructs a wizard titled @a strTitle. */
    UIMd3Wizard(QWidget *pParent, const QString &strTitle);

    /** Appends @a pPage titled @a strTitle with @a strSubtitle. */
    void addPage(UINativeWizardPage *pPage, const QString &strTitle, const QString &strSubtitle);
    /** Defines the label of the finishing button, for example "Import". */
    void setFinishButtonText(const QString &strText);
    /** Returns whether expert mode is active. */
    bool isExpert() const { return m_fExpert; }

protected:

    /** Performs the wizard's real work; subclasses override it. */
    virtual bool performFinish() { return true; }

private slots:

    void sltNext();
    void sltBack();
    void sltStepSelected(int iIndex);
    void sltToggleExpert();
    void sltRevalidate();

private:

    void prepare();
    void updateStepList();

    QString                     m_strTitle;
    QString                     m_strFinishText;
    bool                        m_fExpert;
    int                         m_iCurrentStep;
    QStackedWidget             *m_pStack;
    QList<UINativeWizardPage*>  m_pages;
    QStringList                 m_stepTitles;
    QStringList                 m_stepSubtitles;
    UIMd3Button                *m_pNextButton;
    UIMd3Button                *m_pBackButton;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3Wizard_h */
