/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 command palette.
 */

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "UIMd3Button.h"
#include "UIMd3CommandPalette.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

UIMd3CommandPalette *UIMd3CommandPalette::s_pInstance = 0;

UIMd3CommandPalette *UIMd3CommandPalette::instance()
{
    if (!s_pInstance)
        s_pInstance = new UIMd3CommandPalette;
    return s_pInstance;
}

void UIMd3CommandPalette::registerCommand(const UIMd3Command &command)
{
    if (command.strTitle.trimmed().isEmpty() || command.strSource.trimmed().isEmpty())
        return;
    UIMd3CommandPalette *pPalette = instance();
    for (int i = 0; i < pPalette->m_commands.size(); ++i)
        if (pPalette->m_commands.at(i).strTitle == command.strTitle
            && pPalette->m_commands.at(i).strSource == command.strSource)
        {
            pPalette->m_commands[i] = command;
            if (pPalette->isVisible())
                pPalette->sltRefresh();
            return;
        }
    pPalette->m_commands << command;
    if (pPalette->isVisible())
        pPalette->sltRefresh();
}

void UIMd3CommandPalette::unregisterSource(const QString &strSource)
{
    if (!s_pInstance)
        return;
    UIMd3CommandPalette *pPalette = s_pInstance;
    for (int i = pPalette->m_commands.size() - 1; i >= 0; --i)
        if (pPalette->m_commands.at(i).strSource == strSource)
            pPalette->m_commands.removeAt(i);
    if (pPalette->isVisible())
        pPalette->sltRefresh();
}

void UIMd3CommandPalette::showPalette(QWidget *pParent)
{
    UIMd3CommandPalette *pPalette = instance();
    pPalette->m_pOrigin = QApplication::focusWidget();
    pPalette->setParent(pParent, Qt::Dialog);
    pPalette->sltRefresh();
    pPalette->adjustSize();
    if (pParent)
    {
        const QRect parentRect = pParent->frameGeometry();
        pPalette->move(parentRect.center() - QPoint(pPalette->width() / 2, pPalette->height() / 2));
    }
    pPalette->show();
    pPalette->raise();
    pPalette->activateWindow();
    pPalette->m_pSearchField->setFocus(Qt::ShortcutFocusReason);
}

UIMd3CommandPalette::UIMd3CommandPalette()
    : QDialog(0)
    , m_pSearchField(0)
    , m_pResultLayout(0)
{
    setWindowTitle(tr("Command palette"));
    setWindowFlag(Qt::Tool, true);
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    prepare();
}

void UIMd3CommandPalette::prepare()
{
    resize(720, 520);
    setMinimumSize(420, 280);
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(24, 22, 24, 22);
    pLayout->setSpacing(12);

    QLabel *pTitle = new QLabel(tr("Command palette"), this);
    pTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pLayout->addWidget(pTitle);

    m_pSearchField = new UIMd3SearchField(QStringLiteral("command-palette"),
                                           tr("Search commands, machines, tools and settings"), this);
    m_pSearchField->setAccessibleName(tr("Command palette search"));
    connect(m_pSearchField, &UIMd3SearchField::sigFilterChanged,
            this, &UIMd3CommandPalette::sltRefresh);
    pLayout->addWidget(m_pSearchField);

    QScrollArea *pScroll = new QScrollArea(this);
    pScroll->setWidgetResizable(true);
    pScroll->setFrameShape(QFrame::NoFrame);
    QWidget *pResults = new QWidget(pScroll);
    m_pResultLayout = new QVBoxLayout(pResults);
    m_pResultLayout->setContentsMargins(0, 0, 0, 0);
    m_pResultLayout->setSpacing(4);
    pScroll->setWidget(pResults);
    pLayout->addWidget(pScroll, 1);
}

void UIMd3CommandPalette::sltRefresh()
{
    if (!m_pResultLayout || !m_pSearchField)
        return;
    while (QLayoutItem *pItem = m_pResultLayout->takeAt(0))
    {
        if (pItem->widget())
            pItem->widget()->deleteLater();
        delete pItem;
    }

    int cShown = 0;
    UIMd3Button *pFirst = 0;
    foreach (const UIMd3Command &command, m_commands)
    {
        if (!m_pSearchField->matches(command.strTitle + QLatin1Char(' ') + command.strSource))
            continue;
        UIMd3Button *pRow = new UIMd3Button(command.strTitle, UIMd3ButtonVariant_Tonal, this);
        pRow->setToolTip(command.strSource);
        pRow->setAccessibleName(tr("%1 — %2").arg(command.strTitle, command.strSource));
        const UIMd3Command captured = command;
        connect(pRow, &UIMd3Button::sigClicked, this, [this, captured]()
        {
            hide();
            if (captured.handler)
                captured.handler();
            if (captured.pTarget)
                teleportTo(captured.pTarget);
        });
        m_pResultLayout->addWidget(pRow);
        if (!pFirst)
            pFirst = pRow;
        ++cShown;
    }

    if (!cShown)
    {
        QLabel *pEmpty = new QLabel(tr("No command matches this search."), this);
        pEmpty->setAccessibleName(tr("No command matches this search"));
        m_pResultLayout->addWidget(pEmpty);
    }
    m_pResultLayout->addStretch(1);
    if (pFirst && hasFocus() && !m_pSearchField->hasFocus())
        pFirst->setFocus(Qt::OtherFocusReason);
}

void UIMd3CommandPalette::teleportTo(QWidget *pTarget)
{
    if (!pTarget)
        return;
    QWidget *pWindow = pTarget->window();
    if (pWindow)
    {
        pWindow->show();
        pWindow->raise();
        pWindow->activateWindow();
    }
    for (QWidget *pAncestor = pTarget->parentWidget(); pAncestor; pAncestor = pAncestor->parentWidget())
        if (QStackedWidget *pStack = qobject_cast<QStackedWidget *>(pAncestor))
        {
            QWidget *pPage = pTarget;
            while (pPage && pPage->parentWidget() != pStack)
                pPage = pPage->parentWidget();
            if (pPage)
                pStack->setCurrentWidget(pPage);
        }
    pTarget->setFocus(Qt::ShortcutFocusReason);
    QGraphicsDropShadowEffect *pFlash = new QGraphicsDropShadowEffect(pTarget);
    pFlash->setColor(md3(UIMd3ColorRole_Primary));
    pFlash->setBlurRadius(40);
    pFlash->setOffset(0);
    pTarget->setGraphicsEffect(pFlash);
    QTimer::singleShot(UIMd3Motion::Long2 * 2, pTarget, [pTarget]() { pTarget->setGraphicsEffect(0); });
}

void UIMd3CommandPalette::restoreOriginFocus()
{
    if (m_pOrigin)
        m_pOrigin->setFocus(Qt::OtherFocusReason);
    m_pOrigin = 0;
}

void UIMd3CommandPalette::keyPressEvent(QKeyEvent *pEvent)
{
    if (pEvent->key() == Qt::Key_Escape)
    {
        hide();
        pEvent->accept();
        return;
    }
    QDialog::keyPressEvent(pEvent);
}

void UIMd3CommandPalette::hideEvent(QHideEvent *pEvent)
{
    QDialog::hideEvent(pEvent);
    restoreOriginFocus();
}
