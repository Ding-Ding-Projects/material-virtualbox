/* $Id$ */
/** @file
 * VBox Qt GUI - compact Material 3 header for the existing manager window.
 */

#ifndef FEQT_INCLUDED_SRC_md3_UIMd3ManagerHeader_h
#define FEQT_INCLUDED_SRC_md3_UIMd3ManagerHeader_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <QWidget>

class QMainWindow;

/** Native manager header that keeps existing actions authoritative. */
class UIMd3ManagerHeader : public QWidget
{
public:
    UIMd3ManagerHeader(QMainWindow *pWindow, QWidget *pParent = 0);
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3ManagerHeader_h */
