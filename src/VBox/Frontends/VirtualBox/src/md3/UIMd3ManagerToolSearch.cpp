/* $Id$ */
/** @file
 * VBox Qt GUI - Material 3 search and appearance card for manager tools.
 */

/*
 * Copyright (C) 2026 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, in version 3 of the License.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/* Qt includes: */
#include <QAccessible>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QSet>
#include <QStringList>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QTreeView>

/* GUI includes: */
#include "UIIconPool.h"
#include "UIMd3AppearanceEditor.h"
#include "UIMd3Language.h"
#include "UIMd3ManagerToolSearch.h"
#include "UIMd3SearchField.h"
#include "UIMd3Theme.h"

namespace
{
    /** Maximum number of model rows visited by one interactive filter pass. */
    const int g_cMd3MaximumFilterRows = 4096;
    /** Maximum candidate text read from one field. */
    const int g_cMd3MaximumFieldLength = 4096;

    static QString md3ManagerToolText(const char *pszKey, const QString &strFallback)
    {
        const UIMd3Language *pLanguage = UIMd3Language::instance();
        return pLanguage ? pLanguage->text(QString::fromLatin1(pszKey)) : strFallback;
    }

    static QIcon md3ManagerToolIcon(const QIcon &source, const QColor &color,
                                    const QWidget *pWidget)
    {
        const qreal dDevicePixelRatio = qMax<qreal>(1.0,
                                                    pWidget ? pWidget->devicePixelRatioF() : 1.0);
        const int iExtent = qMax(1, qRound(20.0 * dDevicePixelRatio));
        QPixmap normal = source.pixmap(QSize(iExtent, iExtent), QIcon::Normal, QIcon::Off);
        if (normal.isNull())
            return source;
        normal.setDevicePixelRatio(dDevicePixelRatio);
        {
            QPainter painter(&normal);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(normal.rect(), color);
        }

        QPixmap disabled = normal;
        {
            QColor disabledColor = color;
            disabledColor.setAlphaF(disabledColor.alphaF() * 0.38);
            QPainter painter(&disabled);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(disabled.rect(), disabledColor);
        }

        QIcon result;
        result.addPixmap(normal, QIcon::Normal, QIcon::Off);
        result.addPixmap(normal, QIcon::Active, QIcon::Off);
        result.addPixmap(normal, QIcon::Selected, QIcon::Off);
        result.addPixmap(disabled, QIcon::Disabled, QIcon::Off);
        return result;
    }
}

UIMd3ManagerToolSearch::UIMd3ManagerToolSearch(QWidget *pParent /* = 0 */)
    : UIMd3Widget(pParent, QStringLiteral("manager-tool/search-card"))
    , m_pSearchField(0)
    , m_pStatusLabel(0)
    , m_pAppearanceButton(0)
    , m_pRefreshTimer(0)
    , m_fChangingTarget(false)
{
    setMinimumHeight(64);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(tr("Manager tool search"));

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(12, 8, 12, 8);
    pLayout->setSpacing(8);

    m_pSearchField = new UIMd3SearchField(QStringLiteral("manager-tool"),
                                           tr("Search this manager tool"), this);
    m_pSearchField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pLayout->addWidget(m_pSearchField, 1);

    m_pStatusLabel = new QLabel(this);
    m_pStatusLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_pStatusLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard);
    pLayout->addWidget(m_pStatusLabel);

    m_pAppearanceButton = new QToolButton(this);
    m_pAppearanceButton->setAutoRaise(true);
    m_pAppearanceButton->setIconSize(QSize(20, 20));
    m_pAppearanceButton->setFixedSize(QSize(48, 48));
    m_pAppearanceButton->setFocusPolicy(Qt::StrongFocus);
    pLayout->addWidget(m_pAppearanceButton);

    m_pRefreshTimer = new QTimer(this);
    m_pRefreshTimer->setInterval(120);
    m_pRefreshTimer->setSingleShot(true);

    connect(m_pSearchField, &UIMd3SearchField::sigFilterChanged,
            this, [this]() { applyFilter(); });
    connect(m_pAppearanceButton, &QToolButton::clicked, this, [this]()
    {
        if (m_pTarget && !m_strTargetKey.isEmpty())
            UIMd3AppearanceEditor::open(m_pTarget, m_strTargetKey);
    });
    connect(m_pRefreshTimer, &QTimer::timeout, this, [this]() { applyFilter(); });
    connect(&md3Theme(), &UIMd3Theme::sigThemeChanged,
            this, [this]() { updateAppearanceIcon(); });

    if (UIMd3Language::instance())
    {
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager-tool.search-card"),
                                                QStringLiteral("Manager tool search"),
                                                QStringLiteral("管理工具搜尋"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager-tool.search-description"),
                                                QStringLiteral("Filter the existing records in this manager tool"),
                                                QStringLiteral("篩選呢個管理工具現有嘅記錄"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager-tool.edit-appearance"),
                                                QStringLiteral("Edit this manager tool appearance"),
                                                QStringLiteral("編輯呢個管理工具嘅外觀"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager-tool.no-records"),
                                                QStringLiteral("No searchable records"),
                                                QStringLiteral("冇可搜尋嘅記錄"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager-tool.record-count"),
                                                QStringLiteral("%1 of %2 records"),
                                                QStringLiteral("%2 項記錄入面顯示 %1 項"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager-tool.minimum-record-count"),
                                                QStringLiteral("At least %1 records"),
                                                QStringLiteral("最少有 %1 項記錄"));
        UIMd3Language::instance()->registerText(QStringLiteral("md3.manager-tool.limited-record-count"),
                                                QStringLiteral("%1 matches in the first %2 records; additional records remain visible"),
                                                QStringLiteral("頭 %2 項記錄有 %1 項相符；其餘記錄繼續顯示"));
        connect(UIMd3Language::instance(), &UIMd3Language::sigLanguageChanged,
                this, [this]() { retranslateUi(); }, Qt::UniqueConnection);
    }
    retranslateUi();
    updateAppearanceIcon();
    updateResponsiveState();
}

UIMd3ManagerToolSearch::~UIMd3ManagerToolSearch()
{
    disconnectModels();
    restoreRows();
}

void UIMd3ManagerToolSearch::setTarget(QWidget *pTarget, const QString &strTargetKey,
                                       const QString &strToolName, const QString &strPlaceholder)
{
    const bool fTargetChanged = m_pTarget != pTarget || m_strTargetKey != strTargetKey;
    if (fTargetChanged)
    {
        disconnectModels();
        restoreRows();
        m_fChangingTarget = true;
        m_pSearchField->clearRegex();
        m_pSearchField->setText(QString());
        m_fChangingTarget = false;
        m_pTarget = pTarget;
        m_strTargetKey = strTargetKey.left(128);
        setAppearanceKey(QStringLiteral("manager-tool/search-card/%1").arg(m_strTargetKey));
        bindModels();
    }
    m_strToolName = strToolName.left(256);
    m_strPlaceholder = strPlaceholder.left(256);
    retranslateUi();
    applyFilter();
}

void UIMd3ManagerToolSearch::clearTarget()
{
    disconnectModels();
    restoreRows();
    m_fChangingTarget = true;
    m_pSearchField->clearRegex();
    m_pSearchField->setText(QString());
    m_fChangingTarget = false;
    m_pTarget.clear();
    m_strTargetKey.clear();
    m_strToolName.clear();
    m_strPlaceholder.clear();
    setStatusText(QString());
}

void UIMd3ManagerToolSearch::paintEvent(QPaintEvent *pEvent)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    paintContainer(painter, rect().adjusted(0, 0, -1, -1),
                   UIMd3ColorRole_SurfaceContainerLow, effectiveRadius());
    UIMd3Widget::paintEvent(pEvent);
}

void UIMd3ManagerToolSearch::resizeEvent(QResizeEvent *pEvent)
{
    UIMd3Widget::resizeEvent(pEvent);
    updateAppearanceIcon();
    updateResponsiveState();
}

void UIMd3ManagerToolSearch::applyFilter()
{
    if (m_fChangingTarget || !m_pTarget || !m_pSearchField)
        return;

    if (!m_pSearchField->isRegexActive() && m_pSearchField->text().trimmed().isEmpty())
    {
        restoreRows();
        int cRows = 0;
        bool fTruncated = false;
        const QList<QTreeView*> treeViews = m_pTarget->findChildren<QTreeView*>();
        foreach (QTreeView *pView, treeViews)
            if (pView && pView->model())
                cRows += countRows(pView->model(), QModelIndex(),
                                   g_cMd3MaximumFilterRows - cRows, fTruncated);
        const QList<QTableView*> tableViews = m_pTarget->findChildren<QTableView*>();
        foreach (QTableView *pView, tableViews)
        {
            if (!pView || !pView->model())
                continue;
            const int cAvailable = pView->model()->rowCount();
            const int cAccepted = qMin(cAvailable, g_cMd3MaximumFilterRows - cRows);
            cRows += cAccepted;
            if (cAccepted < cAvailable)
                fTruncated = true;
        }
        const QString strStatus = !cRows
                                ? md3ManagerToolText("md3.manager-tool.no-records",
                                                     tr("No searchable records"))
                                : fTruncated
                                ? md3ManagerToolText("md3.manager-tool.minimum-record-count",
                                                     tr("At least %1 records")).arg(cRows)
                                : md3ManagerToolText("md3.manager-tool.record-count",
                                                     tr("%1 of %2 records")).arg(cRows).arg(cRows);
        setStatusText(strStatus);
        return;
    }

    int cRows = 0;
    int cVisibleRows = 0;
    bool fTruncated = false;
    const QList<QTreeView*> treeViews = m_pTarget->findChildren<QTreeView*>();
    foreach (QTreeView *pView, treeViews)
    {
        if (!pView || !pView->model())
            continue;
        const int cTopLevelRows = pView->model()->rowCount();
        int iRow = 0;
        for (; iRow < cTopLevelRows && cRows < g_cMd3MaximumFilterRows; ++iRow)
            filterTreeRow(pView, iRow, QModelIndex(), cRows, cVisibleRows, fTruncated);
        if (iRow < cTopLevelRows)
            fTruncated = true;
    }
    const QList<QTableView*> tableViews = m_pTarget->findChildren<QTableView*>();
    foreach (QTableView *pView, tableViews)
    {
        if (pView && pView->model() && cRows >= g_cMd3MaximumFilterRows)
        {
            if (pView->model()->rowCount() > 0)
                fTruncated = true;
            continue;
        }
        filterTable(pView, cRows, cVisibleRows, fTruncated);
    }
    const QString strStatus = !cRows
                            ? md3ManagerToolText("md3.manager-tool.no-records",
                                                 tr("No searchable records"))
                            : fTruncated
                            ? md3ManagerToolText("md3.manager-tool.limited-record-count",
                                                 tr("%1 matches in the first %2 records; additional records remain visible"))
                              .arg(cVisibleRows).arg(cRows)
                            : md3ManagerToolText("md3.manager-tool.record-count",
                                                 tr("%1 of %2 records")).arg(cVisibleRows).arg(cRows);
    setStatusText(strStatus);
}

void UIMd3ManagerToolSearch::bindModels()
{
    if (!m_pTarget)
        return;

    QSet<QAbstractItemModel*> models;
    const QList<QTreeView*> treeViews = m_pTarget->findChildren<QTreeView*>();
    foreach (QTreeView *pView, treeViews)
        if (pView && pView->model())
            models.insert(pView->model());
    const QList<QTableView*> tableViews = m_pTarget->findChildren<QTableView*>();
    foreach (QTableView *pView, tableViews)
        if (pView && pView->model())
            models.insert(pView->model());

    const auto scheduleRefresh = [this]()
    {
        if (m_pRefreshTimer)
            m_pRefreshTimer->start();
    };
    foreach (QAbstractItemModel *pModel, models)
    {
        m_modelConnections << connect(pModel, &QAbstractItemModel::modelReset,
                                      this, scheduleRefresh);
        m_modelConnections << connect(pModel, &QAbstractItemModel::rowsInserted,
                                      this, scheduleRefresh);
        m_modelConnections << connect(pModel, &QAbstractItemModel::rowsRemoved,
                                      this, scheduleRefresh);
        m_modelConnections << connect(pModel, &QAbstractItemModel::dataChanged,
                                      this, scheduleRefresh);
        m_modelConnections << connect(pModel, &QAbstractItemModel::layoutChanged,
                                      this, scheduleRefresh);
    }
}

void UIMd3ManagerToolSearch::disconnectModels()
{
    if (m_pRefreshTimer)
        m_pRefreshTimer->stop();
    foreach (const QMetaObject::Connection &connection, m_modelConnections)
        disconnect(connection);
    m_modelConnections.clear();
}

bool UIMd3ManagerToolSearch::filterTreeRow(QTreeView *pView, int iRow,
                                           const QModelIndex &parent,
                                           int &cRows, int &cVisibleRows,
                                           bool &fTruncated)
{
    if (!pView || !pView->model() || cRows >= g_cMd3MaximumFilterRows)
        return false;

    QAbstractItemModel *pModel = pView->model();
    const QModelIndex index = pModel->index(iRow, 0, parent);
    if (!index.isValid())
        return false;
    ++cRows;

    bool fChildMatch = false;
    const int cChildren = pModel->rowCount(index);
    int iChild = 0;
    for (; iChild < cChildren && cRows < g_cMd3MaximumFilterRows; ++iChild)
        fChildMatch = filterTreeRow(pView, iChild, index, cRows, cVisibleRows,
                                    fTruncated) || fChildMatch;
    if (iChild < cChildren)
        fTruncated = true;

    const bool fMatch = m_pSearchField->matches(rowText(pModel, iRow, parent)) || fChildMatch;
    const bool fOriginallyHidden = originalHidden(pView, iRow, parent);
    pView->setRowHidden(iRow, parent, fOriginallyHidden || !fMatch);
    if (fMatch && !fOriginallyHidden)
        ++cVisibleRows;
    return fMatch;
}

void UIMd3ManagerToolSearch::filterTable(QTableView *pView, int &cRows,
                                         int &cVisibleRows, bool &fTruncated)
{
    if (!pView || !pView->model())
        return;
    QAbstractItemModel *pModel = pView->model();
    const int cTableRows = pModel->rowCount();
    int iRow = 0;
    for (; iRow < cTableRows && cRows < g_cMd3MaximumFilterRows; ++iRow)
    {
        ++cRows;
        const bool fMatch = m_pSearchField->matches(rowText(pModel, iRow, QModelIndex()));
        const bool fOriginallyHidden = originalHidden(pView, iRow);
        pView->setRowHidden(iRow, fOriginallyHidden || !fMatch);
        if (fMatch && !fOriginallyHidden)
            ++cVisibleRows;
    }
    if (iRow < cTableRows)
        fTruncated = true;
}

QString UIMd3ManagerToolSearch::rowText(const QAbstractItemModel *pModel, int iRow,
                                        const QModelIndex &parent) const
{
    if (!pModel)
        return QString();
    QStringList fields;
    const int cColumns = qMin(pModel->columnCount(parent), 64);
    for (int iColumn = 0; iColumn < cColumns; ++iColumn)
    {
        const QModelIndex index = pModel->index(iRow, iColumn, parent);
        const QString strDisplay = pModel->data(index, Qt::DisplayRole).toString().left(g_cMd3MaximumFieldLength);
        const QString strToolTip = pModel->data(index, Qt::ToolTipRole).toString().left(g_cMd3MaximumFieldLength);
        if (!strDisplay.isEmpty())
            fields << strDisplay;
        if (!strToolTip.isEmpty() && strToolTip != strDisplay)
            fields << strToolTip;
    }
    return fields.join(QLatin1Char(' '));
}

int UIMd3ManagerToolSearch::countRows(const QAbstractItemModel *pModel,
                                      const QModelIndex &parent, int cRemaining,
                                      bool &fTruncated) const
{
    if (!pModel)
        return 0;
    if (cRemaining <= 0)
    {
        if (pModel->rowCount(parent) > 0)
            fTruncated = true;
        return 0;
    }
    int cRows = 0;
    const int cChildren = pModel->rowCount(parent);
    int iRow = 0;
    for (; iRow < cChildren && cRows < cRemaining; ++iRow)
    {
        ++cRows;
        const QModelIndex index = pModel->index(iRow, 0, parent);
        cRows += countRows(pModel, index, cRemaining - cRows, fTruncated);
    }
    if (iRow < cChildren)
        fTruncated = true;
    return cRows;
}

UIMd3ManagerToolSearch::ViewState &UIMd3ManagerToolSearch::viewState(QAbstractItemView *pView)
{
    for (int i = 0; i < m_viewStates.size(); ++i)
        if (m_viewStates[i].m_pView == pView)
            return m_viewStates[i];
    ViewState state;
    state.m_pView = pView;
    m_viewStates << state;
    return m_viewStates.last();
}

bool UIMd3ManagerToolSearch::originalHidden(QTreeView *pView, int iRow,
                                            const QModelIndex &parent)
{
    ViewState &state = viewState(pView);
    const QPersistentModelIndex index(pView->model()->index(iRow, 0, parent));
    if (!state.m_rows.contains(index))
        state.m_rows.insert(index, pView->isRowHidden(iRow, parent));
    return state.m_rows.value(index);
}

bool UIMd3ManagerToolSearch::originalHidden(QTableView *pView, int iRow)
{
    ViewState &state = viewState(pView);
    const QPersistentModelIndex index(pView->model()->index(iRow, 0));
    if (!state.m_rows.contains(index))
        state.m_rows.insert(index, pView->isRowHidden(iRow));
    return state.m_rows.value(index);
}

void UIMd3ManagerToolSearch::restoreRows()
{
    for (int iView = 0; iView < m_viewStates.size(); ++iView)
    {
        ViewState &state = m_viewStates[iView];
        if (!state.m_pView)
            continue;
        QTreeView *pTree = qobject_cast<QTreeView*>(state.m_pView.data());
        QTableView *pTable = qobject_cast<QTableView*>(state.m_pView.data());
        for (QHash<QPersistentModelIndex, bool>::const_iterator it = state.m_rows.constBegin();
             it != state.m_rows.constEnd(); ++it)
        {
            if (!it.key().isValid())
                continue;
            if (pTree)
                pTree->setRowHidden(it.key().row(), it.key().parent(), it.value());
            else if (pTable)
                pTable->setRowHidden(it.key().row(), it.value());
        }
    }
    m_viewStates.clear();
}

void UIMd3ManagerToolSearch::setStatusText(const QString &strText)
{
    if (!m_pStatusLabel)
        return;
    const bool fChanged = m_pStatusLabel->text() != strText;
    m_pStatusLabel->setText(strText);
    m_pStatusLabel->setAccessibleName(strText);
    m_pStatusLabel->setToolTip(strText);
    if (m_pSearchField && m_pSearchField->focusProxy())
        m_pSearchField->focusProxy()->setAccessibleDescription(strText);
    if (fChanged)
    {
        QAccessibleEvent event(m_pStatusLabel, QAccessible::NameChanged);
        QAccessible::updateAccessibility(&event);
    }
}

void UIMd3ManagerToolSearch::retranslateUi()
{
    const QString strName = md3ManagerToolText("md3.manager-tool.search-card",
                                               tr("Manager tool search"));
    setAccessibleName(strName);
    setAccessibleDescription(md3ManagerToolText("md3.manager-tool.search-description",
                                                tr("Filter the existing records in this manager tool")));
    if (m_pSearchField)
        m_pSearchField->setPlaceholderText(m_strPlaceholder.isEmpty()
                                           ? tr("Search this manager tool") : m_strPlaceholder);
    if (m_pAppearanceButton)
    {
        const QString strAppearance = md3ManagerToolText("md3.manager-tool.edit-appearance",
                                                         tr("Edit this manager tool appearance"));
        m_pAppearanceButton->setAccessibleName(strAppearance);
        m_pAppearanceButton->setAccessibleDescription(
            m_strToolName.isEmpty() ? strAppearance : QStringLiteral("%1: %2").arg(m_strToolName, strAppearance));
        m_pAppearanceButton->setToolTip(strAppearance);
        m_pAppearanceButton->setWhatsThis(strAppearance);
    }
    applyFilter();
}

void UIMd3ManagerToolSearch::updateAppearanceIcon()
{
    if (!m_pAppearanceButton)
        return;
    m_pAppearanceButton->setIcon(
        md3ManagerToolIcon(UIIconPool::iconSet(":/global_settings_16px.png"),
                           md3(UIMd3ColorRole_OnSurfaceVariant), this));
}

void UIMd3ManagerToolSearch::updateResponsiveState()
{
    if (m_pStatusLabel)
        m_pStatusLabel->setVisible(width() >= 620);
}
