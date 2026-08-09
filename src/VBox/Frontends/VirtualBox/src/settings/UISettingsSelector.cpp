/* $Id: UISettingsSelector.cpp 112403 2026-01-11 19:29:08Z knut.osmundsen@oracle.com $ */
/** @file
 * VBox Qt GUI - UISettingsSelector class implementation.
 */

/*
 * Copyright (C) 2008-2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QApplication>
#include <QCursor>
#include <QLayout>
#include <QPainter>
#include <QWindow>

/* GUI includes: */
#include "QIListWidget.h"
#include "UIIconPool.h"
#include "UIMd3Theme.h"
#include "UISettingsPage.h"
#include "UISettingsSelector.h"


/** Simple container of all the selector item data. */
class UISelectorItem
{
public:

    /** Constructs selector item.
      * @param  icon       Brings the item icon.
      * @param  iID        Brings the item ID.
      * @param  strLink    Brings the item link.
      * @param  pPage      Brings the item page reference.
      * @param  iParentID  Brings the item parent ID. */
    UISelectorItem(const QIcon &icon, int iID, const QString &strLink, UISettingsPage *pPage, int iParentID)
        : m_icon(icon)
        , m_iID(iID)
        , m_strLink(strLink)
        , m_pPage(pPage)
        , m_iParentID(iParentID)
    {}

    /** Destructs selector item. */
    virtual ~UISelectorItem() {}

    /** Returns the item icon. */
    QIcon icon() const { return m_icon; }
    /** Returns the item text. */
    QString text() const { return m_strText; }
    /** Defines the item @a strText. */
    void setText(const QString &strText) { m_strText = strText; }
    /** Returns the item ID. */
    int id() const { return m_iID; }
    /** Returns the item link. */
    QString link() const { return m_strLink; }
    /** Returns the item page reference. */
    UISettingsPage *page() const { return m_pPage; }
    /** Returns the item parent ID. */
    int parentID() const { return m_iParentID; }

protected:

    /** Holds the item icon. */
    QIcon m_icon;
    /** Holds the item text. */
    QString m_strText;
    /** Holds the item ID. */
    int m_iID;
    /** Holds the item link. */
    QString m_strLink;
    /** Holds the item page reference. */
    UISettingsPage *m_pPage;
    /** Holds the item parent ID. */
    int m_iParentID;
};


/*********************************************************************************************************************************
*   Class UISettingsSelector implementation.                                                                                     *
*********************************************************************************************************************************/

UISettingsSelector::UISettingsSelector(QWidget *pParent /* = 0 */)
    : QObject(pParent)
{
}

UISettingsSelector::~UISettingsSelector()
{
    qDeleteAll(m_list);
    m_list.clear();
}

void UISettingsSelector::setItemText(int iID, const QString &strText)
{
    if (UISelectorItem *pTtem = findItem(iID))
        pTtem->setText(strText);
}

QString UISettingsSelector::itemTextByPage(UISettingsPage *pPage) const
{
    QString strText;
    if (UISelectorItem *pItem = findItemByPage(pPage))
        strText = pItem->text();
    return strText;
}

QWidget *UISettingsSelector::idToPage(int iID) const
{
    UISettingsPage *pPage = 0;
    if (UISelectorItem *pItem = findItem(iID))
        pPage = pItem->page();
    return pPage;
}

QList<UISettingsPage*> UISettingsSelector::settingPages() const
{
    QList<UISettingsPage*> list;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->page())
            list << pItem->page();
    return list;
}

UISelectorItem *UISettingsSelector::findItem(int iID) const
{
    UISelectorItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->id() == iID)
        {
            pResult = pItem;
            break;
        }
    return pResult;
}

UISelectorItem *UISettingsSelector::findItemByLink(const QString &strLink) const
{
    UISelectorItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->link() == strLink)
        {
            pResult = pItem;
            break;
        }
    return pResult;
}

UISelectorItem *UISettingsSelector::findItemByPage(UISettingsPage *pPage) const
{
    UISelectorItem *pResult = 0;
    foreach (UISelectorItem *pItem, m_list)
        if (pItem->page() == pPage)
        {
            pResult = pItem;
            break;
        }
    return pResult;
}


/** QIListWidget sub-class for settings selector needs. */
class UISelectorListWidget : public QIListWidget
{
    Q_OBJECT;

public:

    /** Constructs selector list-widget passing @a pParent to the base-class. */
    UISelectorListWidget(QWidget *pParent)
        : QIListWidget(pParent, true /* own painting routine */)
    {
        prepare();
    }

    /** Calculates widget minimum size-hint. */
    virtual QSize minimumSizeHint() const RT_OVERRIDE
    {
        /* Calculate largest column width: */
        int iMaximumColumnWidth = 0;
        int iCumulativeColumnHeight = 0;
        for (int i = 0; i < childCount(); ++i)
        {
            QIListWidgetItem *pItem = childItem(i);
            AssertPtrReturn(pItem, QSize());
            const QSize itemSizeHint = pItem->sizeHint();
            const int iHeightHint = itemSizeHint.height();
            iMaximumColumnWidth = qMax(iMaximumColumnWidth, itemSizeHint.width() + iHeightHint /* to get the fancy shape */);
            iCumulativeColumnHeight += iHeightHint;
        }

        /* Return list-widget size-hint: */
        return QSize(iMaximumColumnWidth, iCumulativeColumnHeight);
    }

    /** Calculates widget size-hint. */
    virtual QSize sizeHint() const RT_OVERRIDE
    {
        // WORKAROUND:
        // By default QIListWidget uses own size-hint
        // which we don't like and want to ignore:
        return minimumSizeHint();
    }

private:

    void prepare()
    {
        /* Configure list-widget: */
#ifndef VBOX_WS_MAC
        setFocusPolicy(Qt::TabFocus);
#endif
        setContextMenuPolicy(Qt::PreventContextMenu);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
        setSpacing(2);
        setUniformItemSizes(true);
    }
};


/*********************************************************************************************************************************
*   Class UISettingsSelectorListWidget implementation.                                                                           *
*********************************************************************************************************************************/

UISettingsSelectorListWidget::UISettingsSelectorListWidget(QWidget *pParent /* = 0 */)
    : UISettingsSelector(pParent)
    , m_pListWidget(0)
{
    prepare();
}

UISettingsSelectorListWidget::~UISettingsSelectorListWidget()
{
    cleanup();
}

QWidget *UISettingsSelectorListWidget::widget() const
{
    return m_pListWidget;
}

QWidget *UISettingsSelectorListWidget::addItem(const QString & /* strBigIcon */,
                                               const QString &strMediumIcon ,
                                               const QString & /* strSmallIcon */,
                                               int iID,
                                               const QString &strLink,
                                               UISettingsPage *pPage /* = 0 */,
                                               int iParentID /* = -1 */)
{
    QWidget *pResult = 0;
    if (pPage)
    {
        /* Adjust page a bit: */
        pPage->setContentsMargins(0, 0, 0, 0);
        if (pPage->layout())
            pPage->layout()->setContentsMargins(0, 0, 0, 0);
        pResult = pPage;

        /* Add selector-item object: */
        const QIcon icon = UIIconPool::iconSet(strMediumIcon);
        UISelectorItem *pSelectorItem = new UISelectorItem(icon, iID, strLink, pPage, iParentID);
        if (pSelectorItem)
            m_list.append(pSelectorItem);

        /* Sanity check: */
        AssertPtrReturn(m_pListWidget, pResult);

        /* Add list-widget item: */
        QIListWidgetItem *pItem = new QIListWidgetItem(pSelectorItem->icon(), QString(), m_pListWidget);
        if (pItem)
        {
            pItem->setProperty("p_ID", iID);
            pItem->setProperty("p_link", strLink);

            /* Update list-widget item size accordingly: */
            pItem->setSizeHint(itemSizeHint(pItem));
        }
    }
    return pResult;
}

void UISettingsSelectorListWidget::setItemVisible(int iID, bool fVisible)
{
    /* Sanity check: */
    AssertPtrReturnVoid(m_pListWidget);

    /* Make corresponding item fVisible: */
    if (QIListWidgetItem *pItem = m_pListWidget->findItem("p_ID", iID))
        pItem->setHidden(!fVisible);
}

void UISettingsSelectorListWidget::setItemText(int iID, const QString &strText)
{
    /* Call to base-class: */
    UISettingsSelector::setItemText(iID, strText);

    /* Sanity check: */
    AssertPtrReturnVoid(m_pListWidget);

    /* Assign the strText: */
    if (QIListWidgetItem *pItem = m_pListWidget->findItem("p_ID", iID))
    {
        pItem->setText(strText);

        /* Update list-widget item size accordingly: */
        pItem->setSizeHint(itemSizeHint(pItem));
    }
}

QString UISettingsSelectorListWidget::itemText(int iID) const
{
    /* Sanity check: */
    AssertPtrReturn(m_pListWidget, QString());

    /* Acquire corresponding item: */
    QIListWidgetItem *pItem = m_pListWidget->findItem("p_ID", iID);

    /* Return item text: */
    return pItem ? pItem->text() : QString();
}

int UISettingsSelectorListWidget::currentId() const
{
    /* Sanity check: */
    AssertPtrReturn(m_pListWidget, -1);

    /* Acquire current item: */
    QIListWidgetItem *pItem = QIListWidgetItem::toItem(m_pListWidget->currentItem());

    /* Return item ID: */
    return pItem ? pItem->property("p_ID").toInt() : -1;
}

int UISettingsSelectorListWidget::linkToId(const QString &strLink) const
{
    /* Sanity check: */
    AssertPtrReturn(m_pListWidget, -1);

    /* Acquire corresponding item: */
    QIListWidgetItem *pItem = m_pListWidget->findItem("p_link", strLink);

    /* Return item ID: */
    return pItem ? pItem->property("p_ID").toInt() : -1;
}

void UISettingsSelectorListWidget::selectById(int iID, bool fSilently)
{
    /* Sanity check: */
    AssertPtrReturnVoid(m_pListWidget);

    /* Acquire corresponding item: */
    QIListWidgetItem *pItem = m_pListWidget->findItem("p_ID", iID);

    /* Select item if present: */
    if (pItem)
    {
        if (fSilently)
            m_pListWidget->blockSignals(true);
        m_pListWidget->setCurrentItem(pItem);
        if (fSilently)
            m_pListWidget->blockSignals(false);
    }
}

void UISettingsSelectorListWidget::sltHandleItemPainted(QListWidgetItem *pItem, QPainter *pPainter)
{
    /* Sanity checks: */
    AssertPtrReturnVoid(pItem);
    AssertPtrReturnVoid(pPainter);
    AssertPtrReturnVoid(m_pListWidget);

    /* Do nothing for hidden items: */
    if (pItem->isHidden())
        return;

    const QRect origRect = m_pListWidget->visualItemRect(pItem);
    const QPalette pal = m_pListWidget->palette();
    const bool fSelected = m_pListWidget->currentItem() == pItem;
    const bool fHovered = m_pListWidget->itemAt(
        m_pListWidget->viewport()->mapFromGlobal(QCursor::pos())) == pItem;
    const QRect itemRectangle = origRect.adjusted(4, 2, -4, -2);
    const QColor colorSurface = UIMd3Theme::instance()
                              ? md3Theme().color(UIMd3ColorRole_SurfaceContainerLow)
                              : pal.color(QPalette::Window);
    QColor colorBackground = fSelected && UIMd3Theme::instance()
                           ? md3Theme().color(UIMd3ColorRole_SecondaryContainer)
                           : fSelected ? pal.color(QPalette::Highlight) : colorSurface;
    const QColor colorForeground = fSelected && UIMd3Theme::instance()
                                 ? md3Theme().color(UIMd3ColorRole_OnSecondaryContainer)
                                 : fSelected ? pal.color(QPalette::HighlightedText)
                                             : UIMd3Theme::instance()
                                             ? md3Theme().color(UIMd3ColorRole_OnSurfaceVariant)
                                             : pal.color(QPalette::WindowText);
    QColor colorState = Qt::transparent;
    if (fHovered && !fSelected)
    {
        colorState = UIMd3Theme::instance()
                   ? md3Theme().color(UIMd3ColorRole_OnSurface)
                   : pal.color(QPalette::WindowText);
        colorState.setAlpha(UIMd3StateLayer::Hover * 255 / 100);
    }

    pPainter->save();
    pPainter->setRenderHint(QPainter::Antialiasing);
    pPainter->setPen(Qt::NoPen);
    pPainter->setBrush(colorBackground);
    pPainter->drawRoundedRect(itemRectangle, UIMd3Shape::Medium, UIMd3Shape::Medium);
    if (colorState.alpha() > 0)
    {
        pPainter->setBrush(colorState);
        pPainter->drawRoundedRect(itemRectangle, UIMd3Shape::Medium, UIMd3Shape::Medium);
    }

    if (fSelected && m_pListWidget->hasFocus())
    {
        const QColor colorFocus = UIMd3Theme::instance()
                                ? md3Theme().color(UIMd3ColorRole_Primary)
                                : pal.color(QPalette::Highlight);
        pPainter->setBrush(Qt::NoBrush);
        pPainter->setPen(QPen(colorFocus, 2));
        pPainter->drawRoundedRect(itemRectangle.adjusted(1, 1, -1, -1),
                                  UIMd3Shape::Medium - 1, UIMd3Shape::Medium - 1);
    }

    const int iIconSize = 20;
    const int iMargin = 14;
    const QRect itemPixmapRect(itemRectangle.left() + iMargin,
                               itemRectangle.center().y() - iIconSize / 2,
                               iIconSize, iIconSize);
    const QIcon itemIcon = pItem->icon();
    const qreal fDpr = m_pListWidget->window() && m_pListWidget->window()->windowHandle()
                     ? m_pListWidget->window()->windowHandle()->devicePixelRatio() : 1;
    const QPixmap sourcePixmap = itemIcon.pixmap(QSize(iIconSize, iIconSize), fDpr);
    QPixmap tintedPixmap(sourcePixmap.size());
    tintedPixmap.setDevicePixelRatio(sourcePixmap.devicePixelRatio());
    tintedPixmap.fill(Qt::transparent);
    QPainter iconPainter(&tintedPixmap);
    iconPainter.drawPixmap(0, 0, sourcePixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    iconPainter.fillRect(tintedPixmap.rect(), colorForeground);
    iconPainter.end();
    pPainter->drawPixmap(itemPixmapRect, tintedPixmap);

    const QRect textRect(itemPixmapRect.right() + 10, itemRectangle.top(),
                         qMax(0, itemRectangle.right() - itemPixmapRect.right() - 22),
                         itemRectangle.height());
    const QFontMetrics fm(m_pListWidget->font());
    const QString strName = fm.elidedText(pItem->text(), Qt::ElideRight, textRect.width());
    pPainter->setPen(colorForeground);
    pPainter->setFont(m_pListWidget->font());
    pPainter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, strName);
    pPainter->restore();
}

void UISettingsSelectorListWidget::sltHandleCurrentItemChanged(QListWidgetItem *pCurrentItem, QListWidgetItem *)
{
    /* Notify listeners: */
    if (QIListWidgetItem *pItem = QIListWidgetItem::toItem(pCurrentItem))
        emit sigCategoryChanged(pItem->property("p_ID").toInt());
}

void UISettingsSelectorListWidget::prepare()
{
    /* Prepare the selector list-widget: */
    m_pListWidget = new UISelectorListWidget(qobject_cast<QWidget*>(parent()));
    if (m_pListWidget)
    {
        /* Setup connections: */
        connect(m_pListWidget, &QIListWidget::painted,
                this, &UISettingsSelectorListWidget::sltHandleItemPainted);
        connect(m_pListWidget, &QIListWidget::currentItemChanged,
                this, &UISettingsSelectorListWidget::sltHandleCurrentItemChanged);
    }
}

void UISettingsSelectorListWidget::cleanup()
{
    /* Cleanup the list-widget: */
    delete m_pListWidget;
    m_pListWidget = 0;
}

QSize UISettingsSelectorListWidget::itemSizeHint(QListWidgetItem *pItem) const
{
    /* Sanity check: */
    AssertPtrReturn(m_pListWidget, QSize());

    /* Calculate size-hint for passed pItem: */
    const QFontMetrics fm(m_pListWidget->font());
    const int iMargin = QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin) / 1.5;
    const int iSpacing = qMax(QApplication::style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing), 5) * 2;
    const int iIconSize = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * 1.5;
    const QString strName = pItem->text();
    const int iMinimumContentWidth = iIconSize /* icon width */
                                   + iSpacing
                                   + fm.horizontalAdvance(strName) /* name width */;
    const int iMinimumContentHeight = qMax(fm.height() /* font height */,
                                           iIconSize /* icon height */);
    int iMinimumWidth = iMinimumContentWidth + iMargin * 2;
    int iMinimumHeight = iMinimumContentHeight + iMargin * 2;
#ifndef VBOX_WS_MAC
    iMinimumWidth += 2 /* left */ + 2 /* right */;
    iMinimumHeight += 2 /* top */ + 2 /* bottom */;
#endif

    /* Return item size-hint: */
    return QSize(qMin(220, qMax(180, iMinimumWidth)), qMax(48, iMinimumHeight));
}


#include "UISettingsSelector.moc"
