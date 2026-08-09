/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 navigation rail for the manager shell.
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3NavigationRail_h
#define FEQT_INCLUDED_SRC_md3_UIMd3NavigationRail_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QIcon>
#include <QWidget>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class QButtonGroup;
class QToolButton;

/** Compact, keyboard-accessible Material 3 rail for global manager tools. */
class UIMd3NavigationRail : public QWidget
{
    Q_OBJECT;

signals:

    /** Emitted when the user activates a global tool. */
    void sigToolTypeSelected(UIToolType enmType);

public:

    /** Constructs a navigation rail with @a pParent as the owner. */
    explicit UIMd3NavigationRail(QWidget *pParent = 0);

    /** Selects @a enmType without emitting a user activation signal. */
    void setCurrentToolType(UIToolType enmType);
    /** Enables or disables @a enmType while retaining its visible label. */
    void setToolEnabled(UIToolType enmType, bool fEnabled);

private slots:

    /** Refreshes colors and typography after the global theme changes. */
    void sltUpdateTheme();
    /** Refreshes translated labels. */
    void sltRetranslateUI();

private:

    /** Creates one rail button for @a enmType. */
    QToolButton *createButton(UIToolType enmType, const QIcon &icon);
    /** Returns the button for @a enmType. */
    QToolButton *button(UIToolType enmType) const;
    /** Applies the current Material 3 surface roles. */
    void updatePalette();

    /** Holds the button group. */
    QButtonGroup *m_pButtonGroup;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3NavigationRail_h */
