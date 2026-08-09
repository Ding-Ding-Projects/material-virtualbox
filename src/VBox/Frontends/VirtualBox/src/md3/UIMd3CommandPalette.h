/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 command palette.
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h
#define FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <functional>

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>

#include "UILibraryDefs.h"

class QKeyEvent;
class QHideEvent;
class QVBoxLayout;
class UIMd3SearchField;

/** One command palette entry owned by a live UI surface. */
struct UIMd3Command
{
    UIMd3Command() : pTarget(0) {}
    UIMd3Command(const QString &strTitle, const QString &strSource,
                 const std::function<void()> &handler, QWidget *pTargetWidget = 0)
        : strTitle(strTitle), strSource(strSource), handler(handler), pTarget(pTargetWidget) {}

    QString strTitle;
    QString strSource;
    std::function<void()> handler;
    QPointer<QWidget> pTarget;
};

/** Bounded, searchable command palette shared by the native frontend. */
class SHARED_LIBRARY_STUFF UIMd3CommandPalette : public QDialog
{
    Q_OBJECT;

public:

    static UIMd3CommandPalette *instance();
    static void registerCommand(const UIMd3Command &command);
    static void unregisterSource(const QString &strSource);
    static void showPalette(QWidget *pParent);

protected:

    virtual void keyPressEvent(QKeyEvent *pEvent) RT_OVERRIDE;
    virtual void hideEvent(QHideEvent *pEvent) RT_OVERRIDE;

private slots:

    void sltRefresh();

private:

    UIMd3CommandPalette();
    void prepare();
    static void teleportTo(QWidget *pTarget);
    void restoreOriginFocus();

    static UIMd3CommandPalette *s_pInstance;
    QList<UIMd3Command> m_commands;
    UIMd3SearchField *m_pSearchField;
    QVBoxLayout *m_pResultLayout;
    QPointer<QWidget> m_pOrigin;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3CommandPalette_h */
