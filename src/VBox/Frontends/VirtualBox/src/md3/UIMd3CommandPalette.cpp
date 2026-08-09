/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 command palette.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <QApplication>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
#include <QMap>
#include <QScrollArea>
#include <QScreen>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "UIMd3Button.h"
#include "UIMd3CommandPalette.h"
#include "UIMd3Language.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

static void registerMd3PaletteTexts()
{
    if (!UIMd3Language::instance())
        return;
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.title"),
                                            QStringLiteral("Command palette"),
                                            QStringLiteral("指令調色盤"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.search"),
                                            QStringLiteral("Search commands, machines, tools and settings"),
                                            QStringLiteral("搜尋指令、虛擬機、工具同設定"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.search-name"),
                                            QStringLiteral("Command palette search"),
                                            QStringLiteral("指令調色盤搜尋"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.category"),
                                            QStringLiteral("Command category"),
                                            QStringLiteral("指令分類"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.unavailable"),
                                            QStringLiteral("This command is unavailable on the current surface."),
                                            QStringLiteral("呢個指令喺目前畫面未能使用。"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.activate"),
                                            QStringLiteral("Activate %1 from %2."),
                                            QStringLiteral("由 %2 啟用 %1。"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.unavailable-format"),
                                            QStringLiteral("%1 Unavailable: %2"),
                                            QStringLiteral("%1 未能使用：%2"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.empty"),
                                            QStringLiteral("No command matches this search."),
                                            QStringLiteral("搵唔到符合呢個搜尋嘅指令。"));
    UIMd3Language::instance()->registerText(QStringLiteral("md3.palette.empty-name"),
                                            QStringLiteral("No command matches this search"),
                                            QStringLiteral("搵唔到符合呢個搜尋嘅指令"));
}

static QString md3PaletteText(const QString &strKey, const QString &strFallback)
{
    return UIMd3Language::instance() ? md3Text(strKey) : strFallback;
}

UIMd3CommandPalette *UIMd3CommandPalette::s_pInstance = 0;

UIMd3CommandPalette *UIMd3CommandPalette::instance()
{
    if (!s_pInstance)
        s_pInstance = new UIMd3CommandPalette;
    return s_pInstance;
}

void UIMd3CommandPalette::registerCommand(const UIMd3Command &command)
{
    UIMd3Command bounded = command;
    bounded.strTitle = bounded.strTitle.trimmed().left(256);
    bounded.strSource = bounded.strSource.trimmed().left(64);
    bounded.strCategory = bounded.strCategory.trimmed().left(128);
    bounded.strId = bounded.strId.trimmed().left(96);
    if (bounded.strId.isEmpty())
        bounded.strId = bounded.strTitle.left(96);
    bounded.strDisabledReason = bounded.strDisabledReason.trimmed().left(256);
    if (bounded.strTitle.isEmpty() || bounded.strSource.isEmpty() || !bounded.handler)
        return;
    UIMd3CommandPalette *pPalette = instance();
    for (int i = 0; i < pPalette->m_commands.size(); ++i)
        if (pPalette->m_commands.at(i).strId == bounded.strId
            && pPalette->m_commands.at(i).strSource == bounded.strSource)
        {
            pPalette->m_commands[i] = bounded;
            if (pPalette->isVisible())
                pPalette->sltRefresh();
            return;
        }
    pPalette->m_commands << bounded;
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
    QScreen *pScreen = pParent ? pParent->screen() : 0;
    if (!pScreen)
        pScreen = QGuiApplication::primaryScreen();
    const QRect available = pScreen ? pScreen->availableGeometry() : QRect(0, 0, 1280, 720);
    const int iAvailableWidth = qMax(1, available.width() - 24);
    const int iAvailableHeight = qMax(1, available.height() - 24);
    pPalette->setMinimumSize(qMin(420, iAvailableWidth), qMin(280, iAvailableHeight));
    pPalette->adjustSize();
    QSize boundedSize = pPalette->size();
    boundedSize.setWidth(qMin(boundedSize.width(), iAvailableWidth));
    boundedSize.setHeight(qMin(boundedSize.height(), iAvailableHeight));
    pPalette->resize(boundedSize);
    QPoint position = available.center() - QPoint(pPalette->width() / 2, pPalette->height() / 2);
    if (pParent)
    {
        const QRect parentRect = pParent->frameGeometry();
        position = parentRect.center() - QPoint(pPalette->width() / 2, pPalette->height() / 2);
    }
    position.setX(qBound(available.left() + 12, position.x(),
                         available.right() - pPalette->width() - 11));
    position.setY(qBound(available.top() + 12, position.y(),
                         available.bottom() - pPalette->height() - 11));
    pPalette->move(position);
    pPalette->show();
    pPalette->raise();
    pPalette->activateWindow();
    pPalette->m_pSearchField->setFocus(Qt::ShortcutFocusReason);
}

UIMd3CommandPalette::UIMd3CommandPalette()
    : QDialog(0)
    , m_pTitle(0)
    , m_pSearchField(0)
    , m_pResultLayout(0)
{
    registerMd3PaletteTexts();
    setWindowTitle(md3PaletteText(QStringLiteral("md3.palette.title"), tr("Command palette")));
    setAccessibleName(md3PaletteText(QStringLiteral("md3.palette.title"), tr("Command palette")));
    setAccessibleDescription(md3PaletteText(QStringLiteral("md3.palette.search"),
                                             tr("Search and activate commands from the current VirtualBox surface.")));
    setWindowFlag(Qt::Tool, true);
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    prepare();
    if (UIMd3Language::instance())
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, &UIMd3CommandPalette::sltRetranslateUI, Qt::UniqueConnection);
}

UIMd3CommandPalette::~UIMd3CommandPalette()
{
    if (s_pInstance == this)
        s_pInstance = 0;
}

void UIMd3CommandPalette::prepare()
{
    resize(720, 520);
    setMinimumSize(420, 280);
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(24, 22, 24, 22);
    pLayout->setSpacing(12);

    m_pTitle = new QLabel(md3PaletteText(QStringLiteral("md3.palette.title"), tr("Command palette")), this);
    m_pTitle->setObjectName(QStringLiteral("md3CommandPaletteTitle"));
    m_pTitle->setFont(md3Theme().font(UIMd3TypeRole_HeadlineSmall));
    pLayout->addWidget(m_pTitle);

    m_pSearchField = new UIMd3SearchField(QStringLiteral("command-palette"),
                                           md3PaletteText(QStringLiteral("md3.palette.search"),
                                                          tr("Search commands, machines, tools and settings")), this);
    m_pSearchField->setAccessibleName(md3PaletteText(QStringLiteral("md3.palette.search-name"),
                                                     tr("Command palette search")));
    m_pSearchField->installEventFilter(this);
    if (m_pSearchField->focusProxy())
        m_pSearchField->focusProxy()->installEventFilter(this);
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

void UIMd3CommandPalette::sltRetranslateUI()
{
    registerMd3PaletteTexts();
    const QString strTitle = md3PaletteText(QStringLiteral("md3.palette.title"), tr("Command palette"));
    setWindowTitle(strTitle);
    setAccessibleName(strTitle);
    setAccessibleDescription(md3PaletteText(QStringLiteral("md3.palette.search"),
                                             tr("Search and activate commands from the current VirtualBox surface.")));
    if (m_pTitle)
        m_pTitle->setText(strTitle);
    if (m_pSearchField)
    {
        m_pSearchField->setPlaceholderText(md3PaletteText(QStringLiteral("md3.palette.search"),
                                                          tr("Search commands, machines, tools and settings")));
        m_pSearchField->setAccessibleName(md3PaletteText(QStringLiteral("md3.palette.search-name"),
                                                          tr("Command palette search")));
    }
    sltRefresh();
}

void UIMd3CommandPalette::sltRefresh()
{
    if (!m_pResultLayout || !m_pSearchField)
        return;
    while (QLayoutItem *pItem = m_pResultLayout->takeAt(0))
    {
        if (pItem->widget())
        {
            pItem->widget()->hide();
            pItem->widget()->setEnabled(false);
            pItem->widget()->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            pItem->widget()->deleteLater();
        }
        delete pItem;
    }

    m_pRows.clear();
    int cShown = 0;
    UIMd3Button *pFirst = 0;
    QMap<QString, QList<UIMd3Command> > groupedCommands;
    foreach (const UIMd3Command &command, m_commands)
    {
        const QString strCandidate = command.strTitle + QLatin1Char(' ')
                                   + command.strCategory + QLatin1Char(' ')
                                   + command.strSource + QLatin1Char(' ')
                                   + command.strId;
        if (!m_pSearchField->matches(strCandidate))
            continue;
        const QString strCategory = command.strCategory.isEmpty() ? command.strSource : command.strCategory;
        groupedCommands[strCategory] << command;
    }
    for (QMap<QString, QList<UIMd3Command> >::const_iterator it = groupedCommands.constBegin();
         it != groupedCommands.constEnd(); ++it)
    {
        QLabel *pCategory = new QLabel(it.key(), this);
        pCategory->setFont(md3Theme().font(UIMd3TypeRole_LabelLarge));
        pCategory->setAccessibleName(md3PaletteText(QStringLiteral("md3.palette.category"),
                                                     tr("Command category")).arg(it.key()));
        m_pResultLayout->addWidget(pCategory);
        foreach (const UIMd3Command &command, it.value())
        {
            UIMd3Button *pRow = new UIMd3Button(command.strTitle, UIMd3ButtonVariant_Tonal, this);
            pRow->setAppearanceKey(QStringLiteral("command/%1/%2").arg(command.strSource, command.strId));
            const bool fEnabled = !command.enabledPredicate || command.enabledPredicate();
            pRow->setEnabledState(true);
            pRow->setActivationEnabled(fEnabled);
            const QString strCategory = command.strCategory.isEmpty() ? command.strSource : command.strCategory;
            const QString strDisabledReason = command.strDisabledReason.isEmpty()
                                            ? md3PaletteText(QStringLiteral("md3.palette.unavailable"),
                                                             tr("This command is unavailable on the current surface."))
                                            : command.strDisabledReason;
            pRow->setToolTip(strCategory);
            pRow->setAccessibleName(QStringLiteral("%1 — %2").arg(command.strTitle, strCategory));
            pRow->setAccessibleDescription(fEnabled
                                          ? md3PaletteText(QStringLiteral("md3.palette.activate"),
                                                           tr("Activate %1 from %2.")).arg(command.strTitle, strCategory)
                                          : md3PaletteText(QStringLiteral("md3.palette.unavailable-format"),
                                                           tr("%1 Unavailable: %2")).arg(command.strTitle, strDisabledReason));
            pRow->setProperty("md3Unavailable", !fEnabled);
            pRow->installEventFilter(this);
            const UIMd3Command captured = command;
            connect(pRow, &UIMd3Button::sigClicked, this, [this, captured]()
            {
                if (captured.enabledPredicate && !captured.enabledPredicate())
                {
                    sltRefresh();
                    return;
                }
                hide();
                if (captured.handler)
                    captured.handler();
                if (captured.pTarget)
                    teleportTo(captured.pTarget);
            });
            m_pResultLayout->addWidget(pRow);
            m_pRows << pRow;
            if (!pFirst)
                pFirst = pRow;
            ++cShown;
        }
    }

    if (!cShown)
    {
        QLabel *pEmpty = new QLabel(md3PaletteText(QStringLiteral("md3.palette.empty"),
                                                    tr("No command matches this search.")), this);
        pEmpty->setAccessibleName(md3PaletteText(QStringLiteral("md3.palette.empty-name"),
                                                  tr("No command matches this search")));
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
    if (pTarget->graphicsEffect())
        return;
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

bool UIMd3CommandPalette::eventFilter(QObject *pObject, QEvent *pEvent)
{
    if (pEvent->type() == QEvent::KeyPress)
    {
        QKeyEvent *pKeyEvent = static_cast<QKeyEvent *>(pEvent);
        if (pKeyEvent->key() == Qt::Key_Escape)
        {
            hide();
            pKeyEvent->accept();
            return true;
        }
        if (pKeyEvent->key() == Qt::Key_Up || pKeyEvent->key() == Qt::Key_Down)
        {
            const bool fForward = pKeyEvent->key() == Qt::Key_Down;
            int iCurrent = -1;
            for (int i = 0; i < m_pRows.size(); ++i)
                if (m_pRows.at(i) == pObject || m_pRows.at(i) == QApplication::focusWidget())
                {
                    iCurrent = i;
                    break;
                }
            int iNext = fForward ? iCurrent + 1 : iCurrent - 1;
            if (iCurrent < 0)
                iNext = fForward ? 0 : m_pRows.size() - 1;
            for (int c = 0; c < m_pRows.size(); ++c)
            {
                if (iNext < 0)
                    iNext = m_pRows.size() - 1;
                if (iNext >= m_pRows.size())
                    iNext = 0;
                if (m_pRows.at(iNext)->isEnabled() && m_pRows.at(iNext)->isVisible())
                {
                    m_pRows.at(iNext)->setFocus(Qt::OtherFocusReason);
                    pKeyEvent->accept();
                    return true;
                }
                iNext += fForward ? 1 : -1;
            }
        }
    }
    return QDialog::eventFilter(pObject, pEvent);
}

void UIMd3CommandPalette::hideEvent(QHideEvent *pEvent)
{
    QDialog::hideEvent(pEvent);
    restoreOriginFocus();
}
