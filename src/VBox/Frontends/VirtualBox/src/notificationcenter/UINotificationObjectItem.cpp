/* $Id: UINotificationObjectItem.cpp 113791 2026-04-09 18:04:52Z klaus.espenlaub@oracle.com $ */
/** @file
 * VBox Qt GUI - UINotificationObjectItem class implementation.
 */

/*
 * Copyright (C) 2021-2026 Oracle and/or its affiliates.
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
#include <QCheckBox>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "QIRichTextLabel.h"
#include "QIToolButton.h"
#include "UIHelpBrowserDialog.h"
#include "UIIconPool.h"
#include "UIMd3DialogEmoji.h"
#include "UINotificationMessage.h"
#include "UINotificationObject.h"
#include "UINotificationObjectItem.h"
#include "UINotificationQuestion.h"
#include "UITranslationEventListener.h"


/*********************************************************************************************************************************
*   Class UINotificationObjectItem implementation.                                                                               *
*********************************************************************************************************************************/

UINotificationObjectItem::UINotificationObjectItem(QWidget *pParent,
                                                   UINotificationObject *pObject)
    : QWidget(pParent)
    , m_pObject(pObject)
    , m_iMinimumWidthHint(0)
    , m_iInitialItemWidthHint(0)
    , m_iCurrentItemWidthHint(0)
    , m_pLayoutMain(0)
    , m_pLayoutUpper(0)
    , m_pLabelIcon(0)
    , m_pLabelName(0)
    , m_pButtonHelp(0)
    , m_pButtonClose(0)
    , m_pLabelDetails(0)
    , m_fHovered(false)
    , m_fToggled(isCritical())
{
}

bool UINotificationObjectItem::isCritical() const
{
    return internalObject()->isCritical();
}

void UINotificationObjectItem::prepare()
{
    /* Make sure item is opaque. */
    setAutoFillBackground(true);
    /* Make the row itself reachable so details do not depend on a pointer. */
    setFocusPolicy(Qt::StrongFocus);

    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    sltRetranslateUI();
}

void UINotificationObjectItem::setItemWidthHint(int iHint)
{
    /* Make sure something changed: */
    if (m_iCurrentItemWidthHint == iHint)
        return;
    /* Remember new value: */
    m_iCurrentItemWidthHint = iHint;

    /* Acquire details width hint (which is item width hint minus two margins): */
    int iDetailsWidthHint = m_iCurrentItemWidthHint;
    if (iDetailsWidthHint > 0)
    {
        int iL, iT, iR, iB;
        m_pLayoutMain->getContentsMargins(&iL, &iT, &iR, &iB);
        iDetailsWidthHint -= (iL + iR);
    }

    /* Calculate effective width hint on the basis of mimumum and details hints: */
    const int iEffectiveWidthHint = qMax(m_iMinimumWidthHint, iDetailsWidthHint);
    m_pLabelDetails->setMinimumTextWidth(iEffectiveWidthHint);
}

void UINotificationObjectItem::sltRetranslateUI()
{
    /* Keep the row accessible name in sync with its visible primary label. */
    const QString strAccessibleName = m_pLabelName && !m_pLabelName->text().isEmpty()
                                    ? m_pLabelName->text()
                                    : QApplication::translate("UIMessageCenter", "Notification");
    setAccessibleName(strAccessibleName);
    setAccessibleDescription(QApplication::translate("UIMessageCenter",
                                                     "Press Enter, Return, or Space to expand or collapse notification details."));

    /* Translate the details region: */
    if (m_pLabelDetails)
        m_pLabelDetails->setAccessibleName(QApplication::translate("UIMessageCenter", "Notification details"));

    /* Translate help-control: */
    if (m_pButtonHelp)
    {
        m_pButtonHelp->setToolTip(QApplication::translate("UIMessageCenter", "Open help for this notification"));
        m_pButtonHelp->setAccessibleName(QApplication::translate("UIMessageCenter", "Open help for this notification"));
    }

    /* Translate close-button: */
    if (m_pButtonClose)
    {
        m_pButtonClose->setToolTip(QApplication::translate("UIMessageCenter", "Close"));
        m_pButtonClose->setAccessibleName(QApplication::translate("UIMessageCenter", "Close notification"));
    }
}

void UINotificationObjectItem::prepareWidgets()
{
    /* Prepare main layout: */
    m_pLayoutMain = new QVBoxLayout(this);
    if (m_pLayoutMain)
    {
        /* Prepare upper layout: */
        m_pLayoutUpper = new QHBoxLayout;
        if (m_pLayoutUpper)
        {
            /* Prepare icon label: */
            if (internalObject()->objType() != NotificationType_Unknown)
            {
                m_pLabelIcon = new QLabel(this);
                if (m_pLabelIcon)
                {
                    m_pLabelIcon->setPixmap(standardPixmap(internalObject()->objType(), this));
                    m_pLabelIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                    m_pLayoutUpper->addWidget(m_pLabelIcon);
                }
            }

            /* Prepare name label: */
            m_pLabelName = new QLabel(this);
            if (m_pLabelName)
            {
                m_pLabelName->setText(m_pObject->name());
                m_pLayoutUpper->addWidget(m_pLabelName);
            }

            /* Prepare help button: */
            if (!m_pObject->helpKeyword().isEmpty())
                m_pButtonHelp = new QIToolButton(this);
            if (m_pButtonHelp)
            {
                m_pButtonHelp->setIcon(UIIconPool::iconSet(":/help_16px.png"));
                m_pButtonHelp->setIconSize(QSize(10, 10));
                m_pButtonHelp->setProperty("helpkeyword", m_pObject->helpKeyword());
                connect(m_pButtonHelp, &QIToolButton::clicked,
                        this, &UINotificationObjectItem::sltHandleHelpRequest);

                m_pLayoutUpper->addWidget(m_pButtonHelp);
            }

            /* Prepare close button: */
            m_pButtonClose = new QIToolButton(this);
            if (m_pButtonClose)
            {
                m_pButtonClose->setIcon(UIIconPool::iconSet(":/close_16px.png"));
                m_pButtonClose->setIconSize(QSize(10, 10));
                connect(m_pButtonClose, &QIToolButton::clicked,
                        m_pObject, &UINotificationObject::close,
                        Qt::QueuedConnection);

                m_pLayoutUpper->addWidget(m_pButtonClose);
            }

            /* Add to layout: */
            m_pLayoutMain->addLayout(m_pLayoutUpper);
        }

        /* Prepare details label: */
        m_pLabelDetails = new QIRichTextLabel(this);
        if (m_pLabelDetails)
        {
            QFont myFont = m_pLabelDetails->font();
            myFont.setPointSize(myFont.pointSize() - 1);
            m_pLabelDetails->setBrowserFont(myFont);
            m_pLabelDetails->setText(decoratedDetailsText());
            m_pLabelDetails->setVisible(m_fToggled && !m_pLabelDetails->text().isEmpty());
            m_pLabelDetails->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);

            /* Acquire initial item with hint (which is details width hint plus two margins): */
            int iL, iT, iR, iB;
            m_pLayoutMain->getContentsMargins(&iL, &iT, &iR, &iB);
            m_iInitialItemWidthHint = m_pLabelDetails->minimumTextWidth() + iL + iR;

            m_pLayoutMain->addWidget(m_pLabelDetails);
        }
    }

    /* Calculate minimum width hint: */
    if (m_pLabelIcon)
        m_iMinimumWidthHint += m_pLabelIcon->minimumSizeHint().width() + m_pLayoutUpper->spacing();
    if (m_pLabelName)
        m_iMinimumWidthHint += m_pLabelName->minimumSizeHint().width() + m_pLayoutUpper->spacing();
    if (m_pButtonHelp)
        m_iMinimumWidthHint += m_pButtonHelp->minimumSizeHint().width() + m_pLayoutUpper->spacing();
    if (m_pButtonClose)
        m_iMinimumWidthHint += m_pButtonClose->minimumSizeHint().width();
    m_iMinimumWidthHint = qMax(m_iMinimumWidthHint, widthHintForgetControl());
}

void UINotificationObjectItem::prepareConnections()
{
    /* Install translation listener: */
    if (UITranslationEventListener::instance())
        connect(&translationEventListener(), &UITranslationEventListener::sigRetranslateUI,
            this, &UINotificationObjectItem::sltRetranslateUI);

    /* Keep the details text in step with the "Show emojis in dialogs and
     * message boxes" preference for as long as this row stays on screen: */
    if (UIMd3DialogEmoji::instance())
        connect(UIMd3DialogEmoji::instance(), &UIMd3DialogEmoji::sigDialogEmojiChanged,
            this, &UINotificationObjectItem::sltHandleDialogEmojiChanged);
}

bool UINotificationObjectItem::event(QEvent *pEvent)
{
    /* Handle required event types: */
    switch (pEvent->type())
    {
        case QEvent::Enter:
        case QEvent::MouseMove:
        {
            m_fHovered = true;
            update();
            break;
        }
        case QEvent::Leave:
        {
            m_fHovered = false;
            update();
            break;
        }
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        {
            update();
            break;
        }
        case QEvent::KeyPress:
        {
            QKeyEvent *pKeyEvent = static_cast<QKeyEvent *>(pEvent);
            if (   m_pLabelDetails
                && !m_pLabelDetails->text().isEmpty()
                && (pKeyEvent->key() == Qt::Key_Enter
                    || pKeyEvent->key() == Qt::Key_Return
                    || pKeyEvent->key() == Qt::Key_Space))
            {
                m_fToggled = !m_fToggled;
                m_pLabelDetails->setVisible(m_fToggled);
                pKeyEvent->accept();
                update();
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if (m_pLabelDetails)
            {
                m_fToggled = !m_fToggled;
                m_pLabelDetails->setVisible(m_fToggled && !m_pLabelDetails->text().isEmpty());
            }
            break;
        }
        default:
            break;
    }

    /* Call to base-class: */
    return QWidget::event(pEvent);
}

void UINotificationObjectItem::paintEvent(QPaintEvent *pPaintEvent)
{
    /* Prepare painter: */
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipRect(pPaintEvent->rect());
    /* Acquire palette: */
    const bool fActive = isActiveWindow();
    QPalette pal = QApplication::palette();

    /* Prepare effective frame rectangle: */
#ifdef VBOX_WS_WIN
    // WORKAROUND:
    // Fixing off-by-one problem on Windows:
    QRectF adjustedRect = rect().adjusted(0, 0, -1, -1);
    QRectF effectiveRect = adjustedRect.translated(0.5, 0.5);
#else
    QRectF effectiveRect = rect();
#endif

    /* Gather suitable colors: */
    QColor color = pal.color(fActive ? QPalette::Active : QPalette::Inactive, QPalette::Window);
    QColor color1;
    QColor color2;
    if (color.black() > 128)
    {
        color1 = color.lighter(110);
        color2 = color.lighter(105);
    }
    else
    {
        color1 = color.darker(105);
        color2 = color.darker(110);
    }
    /* Prepare background gradient: */
    QLinearGradient grad(QPointF(0, 0), QPointF(width(), height()));
    {
        grad.setColorAt(0, color1);
        grad.setColorAt(1, color2);
    }
    /* Fill background: */
    painter.fillRect(effectiveRect, grad);

    /* If item is hovered: */
    if (m_fHovered || hasFocus())
    {
        /* Gather suitable color: */
        QColor color3 = pal.color(fActive ? QPalette::Active : QPalette::Inactive, QPalette::Highlight);
        /* Make keyboard focus visibly stronger than hover. */
        QPen pen(color3);
        pen.setWidth(hasFocus() ? 2 : 1);
        painter.setPen(pen);
        /* Draw frame: */
        painter.drawRect(hasFocus() ? effectiveRect.adjusted(1, 1, -1, -1) : effectiveRect);
    }
}

/* static */
QPixmap UINotificationObjectItem::standardPixmap(NotificationType enmType, QWidget *pWidget)
{
    /* Prepare standard icon: */
    QIcon icon;
    switch (enmType)
    {
        case NotificationType_Info:
            icon = UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_MessageBoxInformation, pWidget);
            break;
        case NotificationType_Question:
            icon = UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_MessageBoxQuestion, pWidget);
            break;
        case NotificationType_Warning:
            icon = UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_MessageBoxWarning, pWidget);
            break;
        case NotificationType_Critical:
            icon = UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_MessageBoxCritical, pWidget);
            break;
        case NotificationType_GuruMeditation:
            icon = UIIconPool::iconSet(":/meditation_32px.png");
            break;
        default:
            break;
    }

    /* Return empty pixmap if nothing found: */
    if (icon.isNull())
        return QPixmap();

    /* Return pixmap of standard size if possible: */
    QStyle *pStyle = pWidget ? pWidget->style() : QApplication::style();
    const int iSize = pStyle->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, pWidget) / 2;
    return icon.pixmap(iSize, iSize);
}

void UINotificationObjectItem::sltHandleHelpRequest()
{
    UIHelpBrowserDialog::findManualFileAndShow("helpkeyword");
}

void UINotificationObjectItem::sltHandleDialogEmojiChanged()
{
    /* Nothing to refresh before the details label exists: */
    if (!m_pLabelDetails)
        return;

    /* Only the text depends on the preference; visibility (collapsed/toggled,
     * empty details) is independent state we must not disturb here: */
    const bool fWasVisible = m_pLabelDetails->isVisible();
    m_pLabelDetails->setText(decoratedDetailsText());
    m_pLabelDetails->setVisible(fWasVisible);
}

QString UINotificationObjectItem::decoratedDetailsText() const
{
    const QString strDetails = m_pObject->details();

    /* Map the same classification already used for the standard message-box
     * icon (standardPixmap() above) onto a decoration kind.  Progress and
     * downloader items report NotificationType_Unknown -- they carry no
     * message-box icon and are not "a dialog or message box" in the
     * house-contract sense, so their text is deliberately left untouched. */
    UIMd3DialogEmojiKind enmKind;
    switch (internalObject()->objType())
    {
        case NotificationType_Info:           enmKind = UIMd3DialogEmojiKind_Info; break;
        case NotificationType_Question:       enmKind = UIMd3DialogEmojiKind_Question; break;
        case NotificationType_Warning:        enmKind = UIMd3DialogEmojiKind_Warning; break;
        case NotificationType_Critical:       enmKind = UIMd3DialogEmojiKind_Critical; break;
        case NotificationType_GuruMeditation: enmKind = UIMd3DialogEmojiKind_GuruMeditation; break;
        default:                              return strDetails;
    }

    return UIMd3DialogEmoji::instance() ? UIMd3DialogEmoji::instance()->decorate(strDetails, enmKind) : strDetails;
}

/*********************************************************************************************************************************
*   Class UINotificationMessageItem implementation.                                                                              *
*********************************************************************************************************************************/

UINotificationMessageItem::UINotificationMessageItem(QWidget *pParent,
                                                     UINotificationObject *pObject)
    : UINotificationObjectItem(pParent, pObject)
    , m_pButtonForget(0)
{
}

void UINotificationMessageItem::sltRetranslateUI()
{
    /* Call to base-class: */
    UINotificationObjectItem::sltRetranslateUI();

    /* Translate forget-button: */
    if (m_pButtonForget)
        m_pButtonForget->setText(QApplication::translate("UIMessageCenter", "Don't show again"));
}

void UINotificationMessageItem::prepareWidgets()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareWidgets();

    /* Main layout was prepared in base-class: */
    if (m_pLayoutMain)
    {
        /* Prepare forget button: */
        if (!message()->internalName().isEmpty())
        {
            m_pButtonForget = new QPushButton(this);
            if (m_pButtonForget)
            {
                QFont myFont = m_pButtonForget->font();
                myFont.setPointSize(myFont.pointSize() - 2);
                m_pButtonForget->setFont(myFont);
                m_pButtonForget->setIcon(UIIconPool::iconSet(":/close_popup_16px.png"));
                m_pButtonForget->setIconSize(QSize(10, 10));

                m_pLayoutMain->addWidget(m_pButtonForget);
            }
        }
    }
}

void UINotificationMessageItem::prepareConnections()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareConnections();

    /* Connect buttons: */
    if (m_pButtonForget)
        connect(m_pButtonForget, &QIToolButton::clicked,
                m_pObject, &UINotificationObject::dismiss,
                Qt::QueuedConnection);
}

int UINotificationMessageItem::widthHintForgetControl() const
{
    return m_pButtonForget ? m_pButtonForget->minimumSizeHint().width() : 0;
}

UINotificationMessage *UINotificationMessageItem::message() const
{
    return qobject_cast<UINotificationMessage*>(m_pObject);
}


/*********************************************************************************************************************************
*   Class UINotificationQuestionItem implementation.                                                                             *
*********************************************************************************************************************************/

UINotificationQuestionItem::UINotificationQuestionItem(QWidget *pParent,
                                                       UINotificationObject *pObject)
    : UINotificationObjectItem(pParent, pObject)
    , m_pCheckBoxOption(0)
    , m_pCheckBoxForget(0)
    , m_pButtonBox(0)
    , m_fPolished(false)
{
}

void UINotificationQuestionItem::sltRetranslateUI()
{
    /* Call to base-class: */
    UINotificationObjectItem::sltRetranslateUI();

    /* Translate forget-button: */
    if (m_pCheckBoxForget)
        m_pCheckBoxForget->setText(QApplication::translate("UIMessageCenter", "Don't show again"));
}

void UINotificationQuestionItem::prepareWidgets()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareWidgets();

    /* Main layout was prepared in base-class: */
    if (m_pLayoutMain)
    {
        /* Acquire question object: */
        UINotificationQuestion *pQuestion = question();
        AssertPtrReturnVoid(pQuestion);

        /* Prepare options check-box: */
        if (!pQuestion->option().isEmpty())
        {
            m_pCheckBoxOption = new QCheckBox(this);
            if (m_pCheckBoxOption)
            {
                QFont myFont = m_pCheckBoxOption->font();
                myFont.setPointSize(myFont.pointSize() - 2);
                m_pCheckBoxOption->setFont(myFont);
                m_pCheckBoxOption->setText(pQuestion->option());

                m_pLayoutMain->addWidget(m_pCheckBoxOption);
            }
        }

        /* Prepare forget check-box: */
        if (!pQuestion->internalName().isEmpty())
        {
            m_pCheckBoxForget = new QCheckBox(this);
            if (m_pCheckBoxForget)
            {
                QFont myFont = m_pCheckBoxForget->font();
                myFont.setPointSize(myFont.pointSize() - 2);
                m_pCheckBoxForget->setFont(myFont);

                m_pLayoutMain->addWidget(m_pCheckBoxForget);
            }
        }

        /* Acquire button names and check their amount: */
        const QStringList buttonNames = pQuestion->buttonNames();
        const int iButtonAmount = qMax(2, buttonNames.size());
        AssertReturnVoid(iButtonAmount <= 3);

        /* Compose a list of button types: */
        QList<QDialogButtonBox::StandardButton> buttonTypes;
            buttonTypes << QDialogButtonBox::Cancel
                        << QDialogButtonBox::Ok;
        if (iButtonAmount == 3)
            buttonTypes << QDialogButtonBox::Yes;

        /* Compose a flag of button types: */
        QDialogButtonBox::StandardButtons enmButtons = QDialogButtonBox::NoButton;
        foreach (QDialogButtonBox::StandardButton enmButtonType, buttonTypes)
            enmButtons |= enmButtonType;

        /* Preparing button-box: */
        m_pButtonBox = new QIDialogButtonBox(enmButtons, Qt::Horizontal, this);
        if (m_pButtonBox)
        {
            /* Apply button names: */
            for (int i = 0; i < iButtonAmount; ++i)
            {
                const QString strName = buttonNames.value(i);
                if (!strName.isEmpty())
                {
                    QPushButton *pButton = m_pButtonBox->button(buttonTypes.value(i));
                    if (pButton)
                        pButton->setText(strName);
                }
            }

            m_pLayoutMain->addWidget(m_pButtonBox);
        }
    }
}

void UINotificationQuestionItem::prepareConnections()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareConnections();

    /* Connect buttons: */
    if (m_pCheckBoxForget)
        connect(m_pCheckBoxForget, &QCheckBox::checkStateChanged,
                this, &UINotificationQuestionItem::sltHandleCheckStateChanged);
    AssertPtrReturnVoid(m_pButtonBox);
    connect(m_pButtonBox, &QIDialogButtonBox::clicked,
            this, &UINotificationQuestionItem::sltHandleButtonClick);
}

int UINotificationQuestionItem::widthHintForgetControl() const
{
    int iResult = 0;
    if (m_pCheckBoxOption && m_pCheckBoxOption->isVisible())
        iResult = qMax(iResult, m_pCheckBoxOption->minimumSizeHint().width());
    if (m_pCheckBoxForget && m_pCheckBoxForget->isVisible())
        iResult = qMax(iResult, m_pCheckBoxForget->minimumSizeHint().width());
    return iResult;
}

void UINotificationQuestionItem::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    UINotificationObjectItem::showEvent(pEvent);

    /* Polish once: */
    if (!m_fPolished)
    {
        m_fPolished = true;

        /* Make sure critical type question focused: */
        if (isCritical())
        {
            /* Ask question whether Ok button should be default one: */
            UINotificationQuestion *pQuestion = question();
            AssertPtrReturnVoid(pQuestion);
            /* Focus Ok or Cancel button on polishing: */
            AssertPtrReturnVoid(m_pButtonBox);
            QPushButton *pButton = m_pButtonBox->button(  pQuestion->isOkByDefault()
                                                        ? QDialogButtonBox::Ok
                                                        : QDialogButtonBox::Cancel);
            AssertPtrReturnVoid(pButton);
            pButton->setFocus();
        }
    }
}

void UINotificationQuestionItem::keyPressEvent(QKeyEvent *pEvent)
{
    switch (pEvent->key())
    {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        {
            /* Ask question whether Ok button should be default one: */
            UINotificationQuestion *pQuestion = question();
            AssertPtrReturnVoid(pQuestion);
            /* Click Ok or Cancel button on Enter/Return: */
            AssertPtrReturnVoid(m_pButtonBox);
            QPushButton *pButton = m_pButtonBox->button(  pQuestion->isOkByDefault()
                                                        ? QDialogButtonBox::Ok
                                                        : QDialogButtonBox::Cancel);
            AssertPtrReturnVoid(pButton);
            pButton->click();
            pEvent->accept();
            break;
        }
        case Qt::Key_Escape:
        {
            /* Click Cancel button on Escape: */
            AssertPtrReturnVoid(m_pButtonBox);
            QPushButton *pButton = m_pButtonBox->button(QDialogButtonBox::Cancel);
            AssertPtrReturnVoid(pButton);
            pButton->click();
            pEvent->accept();
            break;
        }
        default:
        {
            /* Call to base-class: */
            UINotificationObjectItem::keyPressEvent(pEvent);
            break;
        }
    }
}

void UINotificationQuestionItem::sltHandleCheckStateChanged(Qt::CheckState enmCheckState)
{
    /* Sanity check: */
    UINotificationQuestion *pQuestion = question();
    AssertPtrReturnVoid(pQuestion);

    /* Save result: */
    pQuestion->setDoNotShowAgain(enmCheckState == Qt::Checked);
}

void UINotificationQuestionItem::sltHandleButtonClick(QAbstractButton *pButton)
{
    /* Sanity check: */
    AssertPtrReturnVoid(pButton);
    UINotificationQuestion *pQuestion = question();
    AssertPtrReturnVoid(pQuestion);

    /* Acquire result: */
    AssertPtrReturnVoid(m_pButtonBox);
    QMap<Question::Result, QAbstractButton*> results;
    results[Question::Result_Accept] = m_pButtonBox->button(QDialogButtonBox::Ok);
    results[Question::Result_AcceptAlternative] = m_pButtonBox->button(QDialogButtonBox::Yes);
    Question::Result enmResult = results.key(pButton, Question::Result_Cancel);
    if (   enmResult != Question::Result_Cancel
        && m_pCheckBoxOption && m_pCheckBoxOption->checkState() == Qt::Checked)
        enmResult = (Question::Result)(enmResult | Question::Result_AcceptOption);

    /* Assign result: */
    pQuestion->setResult(enmResult);

    /* Click close button: */
    m_pButtonClose->click();
}

UINotificationQuestion *UINotificationQuestionItem::question() const
{
    return qobject_cast<UINotificationQuestion*>(m_pObject);
}


/*********************************************************************************************************************************
*   Class UINotificationProgressItem implementation.                                                                             *
*********************************************************************************************************************************/

UINotificationProgressItem::UINotificationProgressItem(QWidget *pParent,
                                                       UINotificationObject *pObject)
    : UINotificationObjectItem(pParent, pObject)
    , m_pProgressBar(0)
{
}

void UINotificationProgressItem::prepareWidgets()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareWidgets();

    /* Main layout was prepared in base-class: */
    if (m_pLayoutMain)
    {
        /* Name label was prepared in base-class: */
        if (m_pLabelName)
            m_pLabelName->setText(progress()->name());
        /* Details label was prepared in base-class: */
        if (m_pLabelDetails)
        {
            const int iHint = m_pLabelName->minimumSizeHint().width()
                            + m_pLayoutUpper->spacing()
                            + m_pButtonClose->minimumSizeHint().width();
            m_pLabelDetails->setMinimumTextWidth(iHint);
            updateDetails();
        }

        /* Prepare progress-bar: */
        m_pProgressBar = new QProgressBar(this);
        if (m_pProgressBar)
        {
            m_pProgressBar->setMinimum(0);
            m_pProgressBar->setMaximum(100);
            m_pProgressBar->setValue(progress()->percent());

            m_pLayoutMain->addWidget(m_pProgressBar);
        }
    }
}

void UINotificationProgressItem::prepareConnections()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareConnections();

    /* Prepare progress connections: */
    connect(progress(), &UINotificationProgress::sigProgressStarted,
            this, &UINotificationProgressItem::sltHandleProgressStarted);
    connect(progress(), &UINotificationProgress::sigProgressChange,
            this, &UINotificationProgressItem::sltHandleProgressChange);
    connect(progress(), &UINotificationProgress::sigProgressFinished,
            this, &UINotificationProgressItem::sltHandleProgressFinished);
}

void UINotificationProgressItem::sltHandleProgressStarted()
{
    /* Init close-button and progress-bar states: */
    if (m_pButtonClose)
        m_pButtonClose->setEnabled(progress()->isCancelable());
    if (m_pProgressBar)
        m_pProgressBar->setValue(0);
    /* Update details with fetched stuff if any: */
    if (m_pLabelDetails)
        updateDetails();
}

void UINotificationProgressItem::sltHandleProgressChange(ulong uPercent)
{
    /* Update close-button and progress-bar states: */
    if (m_pButtonClose)
        m_pButtonClose->setEnabled(progress()->isCancelable());
    if (m_pProgressBar)
        m_pProgressBar->setValue(uPercent);
}

void UINotificationProgressItem::sltHandleProgressFinished()
{
    /* Finalize close-button and progress-bar states: */
    if (m_pButtonClose)
        m_pButtonClose->setEnabled(true);
    if (m_pProgressBar)
        m_pProgressBar->setValue(100);
    /* Update details with error text if any: */
    if (m_pLabelDetails)
        updateDetails();
}

UINotificationProgress *UINotificationProgressItem::progress() const
{
    return qobject_cast<UINotificationProgress*>(m_pObject);
}

void UINotificationProgressItem::updateDetails()
{
    AssertPtrReturnVoid(m_pLabelDetails);
    const QString strDetails = progress()->details();
    const QString strError = progress()->error();
    const QString strFullDetails = strError.isNull()
                                 ? strDetails
                                 : QString("%1<br>%2").arg(strDetails, strError);
    m_pLabelDetails->setText(strFullDetails);
    if (!strError.isEmpty())
    {
        m_fToggled = true;
        m_pLabelDetails->setVisible(m_fToggled);
    }
}


#ifdef VBOX_GUI_WITH_NETWORK_MANAGER


/*********************************************************************************************************************************
*   Class UINotificationDownloaderItem implementation.                                                                           *
*********************************************************************************************************************************/

UINotificationDownloaderItem::UINotificationDownloaderItem(QWidget *pParent,
                                                           UINotificationObject *pObject)
    : UINotificationObjectItem(pParent, pObject)
    , m_pProgressBar(0)
{
}

void UINotificationDownloaderItem::prepareWidgets()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareWidgets();

    /* Main layout was prepared in base-class: */
    if (m_pLayoutMain)
    {
        /* Name label was prepared in base-class: */
        if (m_pLabelName)
            m_pLabelName->setText(downloader()->name());
        /* Details label was prepared in base-class: */
        if (m_pLabelDetails)
        {
            const int iHint = m_pLabelName->minimumSizeHint().width()
                            + m_pLayoutUpper->spacing()
                            + m_pButtonClose->minimumSizeHint().width();
            m_pLabelDetails->setMinimumTextWidth(iHint);
            updateDetails();
        }

        /* Prepare progress-bar: */
        m_pProgressBar = new QProgressBar(this);
        if (m_pProgressBar)
        {
            m_pProgressBar->setMinimum(0);
            m_pProgressBar->setMaximum(100);
            m_pProgressBar->setValue(downloader()->percent());

            m_pLayoutMain->addWidget(m_pProgressBar);
        }
    }
}

void UINotificationDownloaderItem::prepareConnections()
{
    /* Call to base-class: */
    UINotificationObjectItem::prepareConnections();

    /* Prepare downloader connections: */
    connect(downloader(), &UINotificationDownloader::sigProgressStarted,
            this, &UINotificationDownloaderItem::sltHandleProgressStarted);
    connect(downloader(), &UINotificationDownloader::sigProgressChange,
            this, &UINotificationDownloaderItem::sltHandleProgressChange);
    connect(downloader(), &UINotificationDownloader::sigProgressFailed,
            this, &UINotificationDownloaderItem::sltHandleProgressFinished);
    connect(downloader(), &UINotificationDownloader::sigProgressCanceled,
            this, &UINotificationDownloaderItem::sltHandleProgressFinished);
    connect(downloader(), &UINotificationDownloader::sigProgressFinished,
            this, &UINotificationDownloaderItem::sltHandleProgressFinished);
}

void UINotificationDownloaderItem::sltHandleProgressStarted()
{
    /* Init progress-bar state: */
    if (m_pProgressBar)
        m_pProgressBar->setValue(0);
    /* Update details with fetched stuff if any: */
    if (m_pLabelDetails)
        updateDetails();
}

void UINotificationDownloaderItem::sltHandleProgressChange(ulong uPercent)
{
    /* Update progress-bar state: */
    if (m_pProgressBar)
        m_pProgressBar->setValue(uPercent);
}

void UINotificationDownloaderItem::sltHandleProgressFinished()
{
    /* Finalize progress-bar state: */
    if (m_pProgressBar)
        m_pProgressBar->setValue(100);
    /* Update details with error text if any: */
    if (m_pLabelDetails)
        updateDetails();
}

UINotificationDownloader *UINotificationDownloaderItem::downloader() const
{
    return qobject_cast<UINotificationDownloader*>(m_pObject);
}

void UINotificationDownloaderItem::updateDetails()
{
    AssertPtrReturnVoid(m_pLabelDetails);
    const QString strDetails = downloader()->details();
    const QString strError = downloader()->error();
    const QString strFullDetails = strError.isNull()
                                 ? strDetails
                                 : QString("%1<br>%2").arg(strDetails, strError);
    m_pLabelDetails->setText(strFullDetails);
    if (!strError.isEmpty())
    {
        m_fToggled = true;
        m_pLabelDetails->setVisible(m_fToggled);
    }
}

#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */


/*********************************************************************************************************************************
*   Namespace UINotificationProgressItem implementation.                                                                         *
*********************************************************************************************************************************/

UINotificationObjectItem *UINotificationItem::create(QWidget *pParent,
                                                     UINotificationObject *pObject)
{
    /* Prepare item: */
    UINotificationObjectItem *pItem = 0;

    /* Handle known types: */
    if (pObject->inherits("UINotificationMessage"))
        pItem = new UINotificationMessageItem(pParent, pObject);
    else if (pObject->inherits("UINotificationQuestion"))
        pItem = new UINotificationQuestionItem(pParent, pObject);
    else if (pObject->inherits("UINotificationProgress"))
        pItem = new UINotificationProgressItem(pParent, pObject);
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    else if (pObject->inherits("UINotificationDownloader"))
        pItem = new UINotificationDownloaderItem(pParent, pObject);
#endif
    /* Handle defaults: */
    else
        pItem = new UINotificationObjectItem(pParent, pObject);

    /* Prepare item: */
    if (pItem)
        pItem->prepare();

    /* Return prepared result: */
    return pItem;
}
