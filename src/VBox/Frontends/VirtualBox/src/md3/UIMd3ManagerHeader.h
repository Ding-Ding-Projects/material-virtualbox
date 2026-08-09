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
#include <QPoint>

class QMainWindow;
class QLabel;
class QPushButton;
class QMouseEvent;

/** Native manager header that keeps existing actions authoritative. */
class UIMd3ManagerHeader : public QWidget
{
public:
    UIMd3ManagerHeader(QMainWindow *pWindow, QWidget *pParent = 0);

protected:
    void mouseDoubleClickEvent(QMouseEvent *pEvent) override;
    void mouseMoveEvent(QMouseEvent *pEvent) override;
    void mousePressEvent(QMouseEvent *pEvent) override;
    void mouseReleaseEvent(QMouseEvent *pEvent) override;

private:
    void toggleMaximize();
    void updateMaximizeLabel();

    QMainWindow  *m_pWindow;
    QLabel       *m_pTitle;
    QPushButton  *m_pMaximize;
    bool          m_fDragging;
    QPoint        m_dragOffset;
};

#endif /* !FEQT_INCLUDED_SRC_md3_UIMd3ManagerHeader_h */
