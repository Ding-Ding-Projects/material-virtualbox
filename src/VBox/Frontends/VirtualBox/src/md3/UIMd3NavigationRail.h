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
#include <QList>
#include <QWidget>

/* GUI includes: */
#include "UIExtraDataDefs.h"

/* Forward declarations: */
class QButtonGroup;
class QScrollArea;
class QToolButton;
class QVBoxLayout;

/** Compact, keyboard-accessible Material 3 rail for global manager tools. */
class UIMd3NavigationRail : public QWidget
{
    Q_OBJECT;

signals:

    /** Emitted when the user activates a global tool. */
    void sigToolTypeSelected(UIToolType enmType);
    /** Emitted when the user activates the persistent Preferences destination. */
    void sigPreferencesRequested();

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

protected:

    /** Refreshes density-aware icons after moving between screens. */
    virtual bool event(QEvent *pEvent) override;
    /** Moves between enabled destinations with the vertical arrow keys. */
    virtual bool eventFilter(QObject *pWatched, QEvent *pEvent) override;

private:

    /** Creates one rail button for @a enmType. */
    QToolButton *createButton(UIToolType enmType, const QIcon &icon);
    /** Returns the button for @a enmType. */
    QToolButton *button(UIToolType enmType) const;
    /** Creates the persistent Preferences action at the foot of the rail. */
    void createPreferencesButton();
    /** Returns the localized reason why @a enmType is disabled. */
    QString disabledReason(UIToolType enmType) const;
    /** Refreshes the enabled or disabled explanatory text for @a pButton. */
    void updateButtonStatus(QToolButton *pButton, UIToolType enmType);
    /** Rebuilds every icon from its alpha mask for the active theme and screen density. */
    void updateIcons();
    /** Applies the current Material 3 surface roles. */
    void updatePalette();

    /** Holds the button group. */
    QButtonGroup *m_pButtonGroup;
    /** Holds the scrollable destination region. */
    QScrollArea *m_pScrollArea;
    /** Holds the destination-button layout. */
    QVBoxLayout *m_pDestinationLayout;
    /** Holds the persistent Preferences button. */
    QToolButton *m_pPreferencesButton;
    /** Holds the explicit top-to-bottom focus traversal order. */
    QList<QToolButton *> m_navigationOrder;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3NavigationRail_h */
