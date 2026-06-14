/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2020 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "qg_blockwidget.h"

#include <QBoxLayout>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QScrollBar>
#include <QToolButton>
#include <algorithm>

#include "lc_actiongroupmanager.h"
#include "lc_flexlayout.h"
#include "lc_mouse_tracking_table_view.h"
#include "lc_widgets_common.h"
#include "qg_actionhandler.h"
#include "rs_blocklist.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"

QG_BlockModel::QG_BlockModel(QObject * parent) : QAbstractTableModel(parent) {
    m_iconBlockVisible = QIcon(":/icons/visible.lci");
    //    blockHidden = QIcon(":/icons/invisible.svg");
    m_iconBlockHidden = QIcon(":/icons/not_visible.lci");
}

int QG_BlockModel::rowCount ( const QModelIndex & /*parent*/ ) const {
    return m_listBlock.size();
}

QModelIndex QG_BlockModel::parent ( const QModelIndex & /*index*/ ) const {
    return QModelIndex();
}

QModelIndex QG_BlockModel::index (const int row, const int column, const QModelIndex & /*parent*/ ) const {
    if ( row >= m_listBlock.size() || row < 0) {
        return QModelIndex();
    }
    return createIndex ( row, column);
}

bool blockLessThan(const RS_Block *s1, const RS_Block *s2) {
    return s1->getName() < s2->getName();
}

void QG_BlockModel::setBlockList(RS_BlockList* bl) {
    /* since 4.6 the recommended way is to use begin/endResetModel()
     * TNick <nicu.tofan@gmail.com>
     */
    beginResetModel();
    m_listBlock.clear();
    if (bl == nullptr){
        endResetModel();
        return;
    }
    for (int i=0; i<bl->count(); ++i) {
        if ( !bl->at(i)->isDeleted() ) {
            m_listBlock.append(bl->at(i));
        }
    }
    setActiveBlock(bl->getActive());
    std::sort( m_listBlock.begin(), m_listBlock.end(), blockLessThan);

    //called to force redraw
    endResetModel();
}


RS_Block *QG_BlockModel::getBlock(const int row) const{
    if ( row >= m_listBlock.size() || row < 0) {
        return nullptr;
    }
    return m_listBlock.at(row);
}

QModelIndex QG_BlockModel::getIndex (RS_Block * blk) const{
    const int row = m_listBlock.indexOf(blk);
    if (row<0) {
        return QModelIndex();
    }
    return createIndex ( row, NAME);
}

QVariant QG_BlockModel::data ( const QModelIndex & index, const int role ) const {
    if (!index.isValid() || index.row() >= m_listBlock.size()) {
        return QVariant();
    }

    const RS_Block* blk = m_listBlock.at(index.row());

    if (role ==Qt::DecorationRole && index.column() == VISIBLE) {
        if (!blk->isFrozen()) {
            return m_iconBlockVisible;
        }
        return m_iconBlockHidden;
    }
    if (role ==Qt::DisplayRole && index.column() == NAME) {
        return blk->getName();
    }
    if (role == Qt::FontRole && index.column() == NAME) {
        if ((m_activeBlock != nullptr) && m_activeBlock == blk) {
            QFont font;
            font.setBold(true);
            return font;
        }
    }
    //Other roles:
    return QVariant();
}

class LC_BlockTableItemDelegate : public LC_TableItemDelegateBase {
public:
    explicit LC_BlockTableItemDelegate(LC_MouseTrackingTableView* parent) : LC_TableItemDelegateBase(parent) {
        auto palette = parent->palette();
        m_hoverRowBackgroundColor = palette.color(QPalette::AlternateBase);
    }

    void doPaint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyledItemDelegate::paint(painter, option, index);
        const bool drawGrid = true; // fixme - add options as part of rework
        if (drawGrid) {
            drawHorizontalGridLine(painter, option);
        }
    }
private:

};


/**
 * Constructor.
 */
QG_BlockWidget::QG_BlockWidget(LC_ActionGroupManager* agm, const QG_ActionHandler* ah, QWidget* parent,
                               const char* name, const Qt::WindowFlags f)
    : LC_GraphicViewAwareWidget(parent, name, f) {
    m_actionGroupManager = agm;
    m_actionHandler = ah;
    m_blockList = nullptr;
    m_lastBlock = nullptr;

    m_blockModel = new QG_BlockModel(this);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_blockModel);
    m_proxyModel->setFilterKeyColumn(QG_BlockModel::NAME);  // Filter only on the NAME column
    m_proxyModel->setDynamicSortFilter(false);  // Disable automatic sorting (source model is already sorted)

    m_blockView = new LC_MouseTrackingTableView(this);
    m_blockView->setModel (m_proxyModel);
    m_blockView->setShowGrid (false);
    m_blockView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_blockView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_blockView->setFocusPolicy(Qt::NoFocus);
    m_blockView->setColumnWidth(QG_BlockModel::VISIBLE, 20);
    m_blockView->verticalHeader()->hide();
    m_blockView->horizontalHeader()->setStretchLastSection(true);
    m_blockView->horizontalHeader()->hide();

#ifndef DONT_FORCE_WIDGETS_CSS
    blockView->setStyleSheet("QWidget {background-color: white;}  QScrollBar{ background-color: none }");
#endif
    auto* lay = new QVBoxLayout(this);
    lay->setSpacing(1);
    lay->setContentsMargins(0, 1, 2, 2);

    const auto layButtons = new LC_FlexLayout(0,3,3);

    addToolbarButton(layButtons, RS2::ActionBlocksDefreezeAll);
    addToolbarButton(layButtons, RS2::ActionBlocksFreezeAll);
    addToolbarButton(layButtons, RS2::ActionBlocksCreate);
    addToolbarButton(layButtons, RS2::ActionBlocksAdd);
    addToolbarButton(layButtons, RS2::ActionBlocksRemove);
    addToolbarButton(layButtons, RS2::ActionBlocksAttributes);
    addToolbarButton(layButtons, RS2::ActionBlocksEdit);
    addToolbarButton(layButtons, RS2::ActionBlocksSave);
    addToolbarButton(layButtons, RS2::ActionBlocksInsert);

    // lineEdit to filter block list with RegEx
    m_matchBlockName = new QLineEdit(this);
    m_matchBlockName->setReadOnly(false);
    m_matchBlockName->setPlaceholderText(tr("Filter"));
    m_matchBlockName->setClearButtonEnabled(true);
    m_matchBlockName->setToolTip(tr("Looking for matching block names"));
    connect(m_matchBlockName, &QLineEdit::textChanged, this, &QG_BlockWidget::slotUpdateBlockList);

    lay->addWidget(m_matchBlockName);
    lay->addLayout(layButtons);
    // lay->addLayout(layButtons2);
    lay->addWidget(m_blockView);

    connect(m_blockView, &QTableView::clicked, this, &QG_BlockWidget::slotActivated);
    connect(m_blockView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &QG_BlockWidget::slotSelectionChanged);
    m_blockView->setTrackingItemDelegate(new LC_BlockTableItemDelegate(m_blockView));
    updateWidgetSettings();
}

void QG_BlockWidget::addToolbarButton(LC_FlexLayout* layButtons, const RS2::ActionType actionType) {
    QAction* action = m_actionGroupManager->getActionByType(actionType);
    if (action != nullptr) {
        const auto button = new QToolButton(this);
        button->setDefaultAction(action);
        layButtons->addWidget(button);
    }
}

QLayout* QG_BlockWidget::getTopLevelLayout() const {
    return layout();
}

/**
 * Updates the block box from the blocks in the graphic.
 */
void QG_BlockWidget::updateWidget() {
    RS_DEBUG->print("QG_BlockWidget::update()");

    if (m_blockList==nullptr) {
        RS_DEBUG->print("QG_BlockWidget::update(): blockList is nullptr");
        m_blockModel->setActiveBlock(nullptr);
        m_blockModel->setBlockList(nullptr);
        m_lastBlock = nullptr;
        return;
    }

    RS_Block* activeBlock =m_blockList->getActive();

    m_blockModel->setBlockList(m_blockList);

    RS_Block* b = m_lastBlock;
    activateBlock(activeBlock);
    m_lastBlock = b;
    m_blockView->resizeRowsToContents();

    restoreSelections();

    RS_DEBUG->print("QG_BlockWidget::update() done");
}


void QG_BlockWidget::restoreSelections() const {
    QItemSelectionModel* selectionModel = m_blockView->selectionModel();
    selectionModel->clearSelection();  // Clear first to avoid duplicates

    for (const auto block : *m_blockList) {
        if (block == nullptr || !block->isSelectedInBlockList()) {
            continue;
        }
        QModelIndex sourceIdx = m_blockModel->getIndex(block);
        if (!sourceIdx.isValid()) {
            continue;
        }
        QModelIndex proxyIdx = m_proxyModel->mapFromSource(sourceIdx);
        if (proxyIdx.isValid()) {
            QItemSelection selection(proxyIdx, proxyIdx);
            selectionModel->select(selection, QItemSelectionModel::Select);
        }
    }
}


/**
 * Activates the given block and makes it the active
 * block in the blocklist.
 */
void QG_BlockWidget::activateBlock(RS_Block* block) {
    RS_DEBUG->print("QG_BlockWidget::activateBlock()");

    if (block == nullptr || m_blockList == nullptr || m_proxyModel == nullptr) {
        return;
    }

    m_lastBlock = m_blockList->getActive();
    m_blockList->activate(block);
    const int yPos = m_blockView->verticalScrollBar()->value();
    const QModelIndex sourceIdx = m_blockModel->getIndex(block);
    if (!sourceIdx.isValid()) {
        return;
    }
    const QModelIndex proxyIdx = m_proxyModel->mapFromSource(sourceIdx);

    // remember selected status of the block
    const bool selected = block->isSelectedInBlockList();

    if (proxyIdx.isValid()) {
        m_blockView->setCurrentIndex(proxyIdx);
    }
    m_blockModel->setActiveBlock(block);
    m_blockView->viewport()->update();

    // restore selected status of the block
    const QItemSelectionModel::SelectionFlag selFlag = selected ? QItemSelectionModel::Select : QItemSelectionModel::Deselect;
    block->selectedInBlockList(selected);
    if (proxyIdx.isValid()) {
        m_blockView->selectionModel()->select(QItemSelection(proxyIdx, proxyIdx), selFlag);
    }
    m_blockView->verticalScrollBar()->setValue(yPos);
}

/**
 * Called when the user activates (highlights) a block.
 */
void QG_BlockWidget::slotActivated(const QModelIndex &blockIdx) {
    if (!blockIdx.isValid() || m_blockList == nullptr || m_proxyModel == nullptr) {
        return;
    }

    const QModelIndex sourceIdx = m_proxyModel->mapToSource(blockIdx);
    if (!sourceIdx.isValid()) {
        return;
    }

    RS_Block* block = m_blockModel->getBlock(sourceIdx.row());
    if (block == nullptr) {
        return;
    }

    if (sourceIdx.column() == QG_BlockModel::VISIBLE) {
        RS_Block* b = m_blockList->getActive();
        m_blockList->activate(block);
        m_actionHandler->setCurrentAction(RS2::ActionBlocksToggleView);
        activateBlock(b);
        return;
    }

    if (sourceIdx.column() == QG_BlockModel::NAME) {
        m_lastBlock = m_blockList->getActive();
        activateBlock(block);
    }
}


/**
 * Called on blocks selection/deselection
 */
void QG_BlockWidget::slotSelectionChanged(
    const QItemSelection &selected,
    const QItemSelection &deselected) const {
    if (m_proxyModel == nullptr) {
        return;
    }

    // Handle selected
    foreach (QModelIndex proxyIdx, selected.indexes()) {
        QModelIndex sourceIdx = m_proxyModel->mapToSource(proxyIdx);
        if (sourceIdx.isValid()) {
            const RS_Block* block = m_blockModel->getBlock(sourceIdx.row());
            if (block != nullptr) {
                block->selectedInBlockList(true);
            }
        }
    }

    // Handle deselected
    foreach (QModelIndex proxyIdx, deselected.indexes()) {
        QModelIndex sourceIdx = m_proxyModel->mapToSource(proxyIdx);
        if (sourceIdx.isValid()) {
            const RS_Block* block = m_blockModel->getBlock(sourceIdx.row());
            if (block != nullptr) {
                block->selectedInBlockList(false);
            }
        }
    }
}


/**
 * Shows a context menu for the block widget. Launched with a right click.
 */
void QG_BlockWidget::contextMenuEvent(QContextMenuEvent *e) {
    // select item (block) in Block List widget first because left-mouse-click event are not to be triggered
    // slotActivated(blockView->currentIndex());
    const auto contextMenu = std::make_unique<QMenu>(this);
    const auto menu = contextMenu.get();
    // Actions for all blocks:
    addMenuItem(menu, RS2::ActionBlocksDefreezeAll);
    addMenuItem(menu, RS2::ActionBlocksFreezeAll);
    contextMenu->addSeparator();
    // Actions for selected blocks or, if nothing is selected, for active block:
    addMenuItem(menu, RS2::ActionBlocksToggleView);
    addMenuItem(menu, RS2::ActionBlocksRemove);
    contextMenu->addSeparator();
    // Single block actions:
    addMenuItem(menu, RS2::ActionBlocksAdd);
    addMenuItem(menu, RS2::ActionBlocksAttributes);
    addMenuItem(menu, RS2::ActionBlocksEdit);
    addMenuItem(menu, RS2::ActionBlocksInsert);
    addMenuItem(menu, RS2::ActionBlocksCreate);
    contextMenu->exec(QCursor::pos());
    e->accept();
}

void QG_BlockWidget::addMenuItem(QMenu* contextMenu, const RS2::ActionType actionType) const {
    const auto action = m_actionGroupManager->getActionByType(actionType);
    if (action != nullptr) {
        contextMenu->addAction(action);
    }
}

/**
 * Escape releases focus.
 */
void QG_BlockWidget::keyPressEvent(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_Escape:
        emit escape();
        break;

    default:
        QWidget::keyPressEvent(e);
        break;
    }
}


void QG_BlockWidget::blockAdded(RS_Block*) {
    updateWidget();
    if (! m_matchBlockName->text().isEmpty()) {
        slotUpdateBlockList();
    }
}


/**
 * Called when reg-expression matchBlockName->text changed
 */
void QG_BlockWidget::slotUpdateBlockList() const {
    if (m_blockList == nullptr || m_proxyModel == nullptr) {
        return;
    }

    QString input = m_matchBlockName->text();
    if (input.isEmpty()) {
        m_proxyModel->setFilterRegularExpression(QRegularExpression(""));  // Clear filter to show all
    } else {
        const bool hasPattern = input.contains('*') || input.contains('?');
        if (!hasPattern) {
            input = "*" + input + "*";  // Wrap for "contains" matching
        }
        const QRegularExpression rx(QRegularExpression::wildcardToRegularExpression(input), QRegularExpression::CaseInsensitiveOption);
        m_proxyModel->setFilterRegularExpression(rx);
    }

    restoreSelections();  // Restore after filter change
}


void QG_BlockWidget::setGraphicView(RS_GraphicView* gv){
    if (gv == nullptr) {
        setBlockList(nullptr);
    }
    else {
        const auto graphic = gv->getGraphic();
        if (graphic == nullptr) {
            setBlockList(nullptr);
        }
        else {
            setBlockList(graphic->getBlockList());
        }
    }
}

void QG_BlockWidget::setBlockList(RS_BlockList* blockList) {
    if (m_blockList != nullptr) {
        m_blockList->removeListener(this);
    }
    m_blockList = blockList;
    if (blockList != nullptr) {
        m_blockList->addListener(this);
    }
    updateWidget();
}
