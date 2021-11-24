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

#include <QScrollBar>
#include <QTableView>
#include <QHeaderView>
#include <QToolButton>
#include <QMenu>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>
#include <QContextMenuEvent>

#include <algorithm>

#include "qg_blockwidget.h"
#include "rs_blocklist.h"
#include "qg_actionhandler.h"
#include "rs_debug.h"
#include "qc_applicationwindow.h"

QG_BlockModel::QG_BlockModel(QObject * parent) : QAbstractTableModel(parent) {
    blockVisible = QIcon(":/icons/visible.svg");
    blockHidden = QIcon(":/icons/invisible.svg");
}

int QG_BlockModel::rowCount ( const QModelIndex & /*parent*/ ) const {
    return listBlock.size();
}

QModelIndex QG_BlockModel::parent ( const QModelIndex & /*index*/ ) const {
    return QModelIndex();
}

QModelIndex QG_BlockModel::index ( int row, int column, const QModelIndex & /*parent*/ ) const {
    if ( row >= listBlock.size() || row < 0)
        return QModelIndex();
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
    listBlock.clear();
    if (bl == NULL){
        endResetModel();
        return;
    }
    for (int i=0; i<bl->count(); ++i) {
        if ( !bl->at(i)->isUndone() )
            listBlock.append(bl->at(i));
    }
    setActiveBlock(bl->getActive());
    std::sort( listBlock.begin(), listBlock.end(), blockLessThan);

    //called to force redraw
    endResetModel();
}


RS_Block *QG_BlockModel::getBlock( int row ){
    if ( row >= listBlock.size() || row < 0)
        return NULL;
    return listBlock.at(row);
}

QModelIndex QG_BlockModel::getIndex (RS_Block * blk){
    int row = listBlock.indexOf(blk);
    if (row<0)
        return QModelIndex();
    return createIndex ( row, NAME);
}

QVariant QG_BlockModel::data ( const QModelIndex & index, int role ) const {
    if (!index.isValid() || index.row() >= listBlock.size())
        return QVariant();

    RS_Block* blk = listBlock.at(index.row());

    if (role ==Qt::DecorationRole && index.column() == VISIBLE) {
        if (!blk->isFrozen()) {
            return blockVisible;
        } else {
            return blockHidden;
        }
    }
    if (role ==Qt::DisplayRole && index.column() == NAME) {
        return blk->getName();
    }
    if (role == Qt::FontRole && index.column() == NAME) {
        if (activeBlock && activeBlock == blk) {
            QFont font;
            font.setBold(true);
            return font;
        }
    }
//Other roles:
    return QVariant();
}

 /**
 * Constructor.
 */
QG_BlockWidget::QG_BlockWidget(QG_ActionHandler* ah, QWidget* parent,
                               const char* name, Qt::WindowFlags f)
        : QWidget(parent, f) {

    setObjectName(name);
    actionHandler = ah;
    blockList = NULL;
    lastBlock = NULL;

    blockModel = new QG_BlockModel;
    blockView = new QTableView(this);
    blockView->setModel (blockModel);
    blockView->setShowGrid (false);
    blockView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    blockView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    blockView->setFocusPolicy(Qt::NoFocus);
    blockView->setColumnWidth(QG_BlockModel::VISIBLE, 20);
    blockView->verticalHeader()->hide();
    blockView->horizontalHeader()->setStretchLastSection(true);
    blockView->horizontalHeader()->hide();

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setSpacing ( 0 );
    lay->setContentsMargins(2, 2, 2, 2);

    QHBoxLayout* layButtons = new QHBoxLayout();
    QHBoxLayout* layButtons2 = new QHBoxLayout();
    QToolButton* but;
    const QSize button_size(28,28);
    // show all blocks:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/visible.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Show all blocks"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksDefreezeAll);
    layButtons->addWidget(but);
    // hide all blocks:
    but = new QToolButton(this);
    but->setIcon( QIcon(":/icons/invisible.svg") );
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Hide all blocks"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksFreezeAll);
    layButtons->addWidget(but);
    // create block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/create_block.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Create Block"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksCreate);
    layButtons->addWidget(but);
    // add block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/add.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Add an empty block"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksAdd);
    layButtons->addWidget(but);
    // remove block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/remove.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Remove block"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksRemove);
    layButtons->addWidget(but);
    // edit attributes:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/rename_active_block.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Rename the active block"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksAttributes);
    layButtons2->addWidget(but);
    // edit block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/properties.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Edit the active block\n"
                          "in a separate window"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksEdit);
    layButtons2->addWidget(but);
    // save block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/save.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("save the active block to a file"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksSave);
    layButtons2->addWidget(but);
    // insert block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/icons/insert_active_block.svg"));
    but->setMinimumSize(button_size);
    but->setToolTip(tr("Insert the active block"));
    connect(but, &QToolButton::clicked, actionHandler, &QG_ActionHandler::slotBlocksInsert);
    layButtons2->addWidget(but);

    // lineEdit to filter block list with RegEx
    matchBlockName = new QLineEdit(this);
    matchBlockName->setReadOnly(false);
    matchBlockName->setPlaceholderText(tr("Filter"));
    matchBlockName->setClearButtonEnabled(true);
    matchBlockName->setToolTip(tr("Looking for matching block names"));
    connect(matchBlockName, &QLineEdit::textChanged, this, &QG_BlockWidget::slotUpdateBlockList);

    lay->addWidget(matchBlockName);
    lay->addLayout(layButtons);
    lay->addLayout(layButtons2);
    lay->addWidget(blockView);

    connect(blockView, &QTableView::clicked, this, &QG_BlockWidget::slotActivated);
    connect(blockView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &QG_BlockWidget::slotSelectionChanged);
}

/**
 * Destructor
 */
QG_BlockWidget::~QG_BlockWidget() {
    delete blockView;
    delete blockModel;
}

/**
 * Updates the block box from the blocks in the graphic.
 */
void QG_BlockWidget::update() {
    RS_DEBUG->print("QG_BlockWidget::update()");

    int yPos = blockView->verticalScrollBar()->value();
    RS_Block* activeBlock;

    if (blockList) {
        activeBlock = blockList->getActive();
    } else {
        activeBlock = NULL;
    }

    blockModel->setBlockList(blockList);

    if (blockList==NULL) {
        RS_DEBUG->print("QG_BlockWidget::update(): blockList is NULL");
        blockModel->setActiveBlock(nullptr);
        return;
    }

    RS_Block* b = lastBlock;
    activateBlock(activeBlock);
    lastBlock = b;
    blockView->resizeRowsToContents();
    blockView->verticalScrollBar()->setValue(yPos);

    restoreSelections();

    RS_DEBUG->print("QG_BlockWidget::update() done");
}


void QG_BlockWidget::restoreSelections() {

    QItemSelectionModel* selectionModel = blockView->selectionModel();

    for (auto block: *blockList) {
        if (!block) continue;
        if (!block->isVisibleInBlockList()) continue;
        if (!block->isSelectedInBlockList()) continue;

        QModelIndex idx = blockModel->getIndex(block);
        QItemSelection selection(idx, idx);
        selectionModel->select(selection, QItemSelectionModel::Select);
    }
}


/**
 * Activates the given block and makes it the active
 * block in the blocklist.
 */
void QG_BlockWidget::activateBlock(RS_Block* block)
{
    RS_DEBUG->print("QG_BlockWidget::activateBlock()");

    if ((block == nullptr) || (blockList == nullptr))
    {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QG_BlockWidget::activateBlock: nullptr block or blockList");
        return;
    }

    if (block->isSelectedInBlockList())
    {
        QC_ApplicationWindow::getAppWindow()->statusBar()
                                            ->showMessage( QString("Block '%1' selected").arg(block->getName()), 
                                                           QC_ApplicationWindow::DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT);
    }

    lastBlock = blockList->getActive();
    blockList->activate(block);
    QModelIndex idx = blockModel->getIndex (block);

    // remember selected status of the block
    bool selected = block->isSelectedInBlockList();

    blockView->setCurrentIndex ( idx );
    blockModel->setActiveBlock(block);
    blockView->viewport()->update();

    // restore selected status of the block
    QItemSelectionModel::SelectionFlag selFlag;
    if (selected) {
        selFlag = QItemSelectionModel::Select;
    } else {
        selFlag = QItemSelectionModel::Deselect;
    }
    block->selectedInBlockList(selected);
    blockView->selectionModel()->select(QItemSelection(idx, idx), selFlag);
}

/**
 * Called when the user activates (highlights) a block.
 */
void QG_BlockWidget::slotActivated(QModelIndex blockIdx) {
    if (!blockIdx.isValid() || blockList==NULL)
        return;

    RS_Block * block = blockModel->getBlock( blockIdx.row() );
    if (block == 0)
        return;

    if (blockIdx.column() == QG_BlockModel::VISIBLE) {
        RS_Block* b = blockList->getActive();
        blockList->activate(block);
        actionHandler->slotBlocksToggleView();
        activateBlock(b);
        return;
    }

    if (blockIdx.column() == QG_BlockModel::NAME) {
        lastBlock = blockList->getActive();
        activateBlock(block);
    }
}


/**
 * Called on blocks selection/deselection
 */
void QG_BlockWidget::slotSelectionChanged(
    const QItemSelection &selected,
    const QItemSelection &deselected)
{
    QModelIndex index;
    QItemSelectionModel *selectionModel {blockView->selectionModel()};

    foreach (index, selected.indexes()) {
        auto block = blockModel->getBlock(index.row());
        if (block) {
            block->selectedInBlockList(true);
            selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Select);
        }
    }

    foreach (index, deselected.indexes()) {
        auto block = blockModel->getBlock(index.row());
        if (block && block->isVisibleInBlockList()) {
            block->selectedInBlockList(false);
            selectionModel->select(QItemSelection(index, index), QItemSelectionModel::Deselect);
        }
    }
}


/**
 * Shows a context menu for the block widget. Launched with a right click.
 */
void QG_BlockWidget::contextMenuEvent(QContextMenuEvent *e) {

    // select item (block) in Block List widget first because left-mouse-click event are not to be triggered
    // slotActivated(blockView->currentIndex());

    QMenu* contextMenu = new QMenu(this);
    QLabel* caption = new QLabel(tr("Block Menu"), this);
    QPalette palette;
    palette.setColor(caption->backgroundRole(), RS_Color(0,0,0));
    palette.setColor(caption->foregroundRole(), RS_Color(255,255,255));
    caption->setPalette(palette);
    caption->setAlignment( Qt::AlignCenter );
    // Actions for all blocks:
    contextMenu->addAction( tr("&Defreeze all Blocks"), actionHandler,
                             SLOT(slotBlocksDefreezeAll()), 0);
    contextMenu->addAction( tr("&Freeze all Blocks"), actionHandler,
                             SLOT(slotBlocksFreezeAll()), 0);
    contextMenu->addSeparator();
    // Actions for selected blocks or,
    // if nothing is selected, for active block:
    contextMenu->addAction( tr("&Toggle Visibility"), actionHandler,
                             SLOT(slotBlocksToggleView()), 0);
    contextMenu->addAction( tr("&Remove Block"), actionHandler,
                             SLOT(slotBlocksRemove()), 0);
    contextMenu->addSeparator();
    // Single block actions:
    contextMenu->addAction( tr("&Add Block"), actionHandler,
                             SLOT(slotBlocksAdd()), 0);
    contextMenu->addAction( tr("&Rename Block"), actionHandler,
                             SLOT(slotBlocksAttributes()), 0);
    contextMenu->addAction( tr("&Edit Block"), actionHandler,
                             SLOT(slotBlocksEdit()), 0);
    contextMenu->addAction( tr("&Insert Block"), actionHandler,
                             SLOT(slotBlocksInsert()), 0);
    contextMenu->addAction( tr("&Create New Block"), actionHandler,
                             SLOT(slotBlocksCreate()), 0);
    contextMenu->exec(QCursor::pos());
    delete contextMenu;

    e->accept();
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
    update();
    if (! matchBlockName->text().isEmpty()) {
        slotUpdateBlockList();
    }
}


/**
 * Called when reg-expression matchBlockName->text changed
 */
void QG_BlockWidget::slotUpdateBlockList() {

    if (!blockList) {
        return;
    }

    QRegExp rx("");
    int pos = 0;
    QString s, n;

    n = matchBlockName->text();
    rx.setPattern(n);
    rx.setPatternSyntax(QRegExp::WildcardUnix);

    for (int i = 0; i < blockList->count(); i++) {
        RS_Block* block = blockModel->getBlock(i);
        if (!block) continue;
        s = block->getName();
        int f = rx.indexIn(s, pos);
        if (!f) {
            blockView->showRow(i);
            blockModel->getBlock(i)->visibleInBlockList(true);
        } else {
            blockView->hideRow(i);
            blockModel->getBlock(i)->visibleInBlockList(false);
        }
    }

    restoreSelections();
}
