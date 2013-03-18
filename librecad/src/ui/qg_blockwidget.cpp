/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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

#include <QScrollBar>
#include <QTableView>
#include <QHeaderView>
#include <QToolButton>
#include <QMenu>
#include <QBoxLayout>
#include <QLabel>
#include "rs_blocklist.h"
#include "qg_actionhandler.h"
//#include <QtAlgorithms>

QG_BlockModel::QG_BlockModel(QObject * parent) : QAbstractTableModel(parent) {
    blockVisible = QIcon(":/ui/visibleblock.png");
    blockHidden = QIcon(":/ui/hiddenblock.png");
}

QG_BlockModel::~QG_BlockModel() {

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
    listBlock.clear();
    if (bl == NULL){
        reset();
        return;
    }
    for (int i=0; i<bl->count(); ++i) {
        if ( !bl->at(i)->isUndone() )
            listBlock.append(bl->at(i));
    }
    qSort ( listBlock.begin(), listBlock.end(), blockLessThan );
//called to force redraw
    reset();
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
    blockView->setSelectionMode(QAbstractItemView::SingleSelection);
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
    // show all blocks:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/ui/visibleblock.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Show all blocks"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksDefreezeAll()));
    layButtons->addWidget(but);
    // hide all blocks:
    but = new QToolButton(this);
    but->setIcon( QIcon(":/ui/hiddenblock.png") );
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Hide all blocks"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksFreezeAll()));
    layButtons->addWidget(but);
    // create block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/extui/menublock.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Create Block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksCreate()));
    layButtons->addWidget(but);
    // add block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/ui/blockadd.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Add an empty block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksAdd()));
    layButtons->addWidget(but);
    // remove block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/ui/blockremove.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Remove the active block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksRemove()));
    layButtons->addWidget(but);
    // edit attributes:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/ui/blockattributes.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Rename the active block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksAttributes()));
    layButtons2->addWidget(but);
    // edit block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/ui/blockedit.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Edit the active block\n"
                          "in a separate window"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksEdit()));
    layButtons2->addWidget(but);
    // save block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/main/filesave.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("save the active block to a file"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksSave()));
    layButtons2->addWidget(but);
    // insert block:
    but = new QToolButton(this);
    but->setIcon(QIcon(":/ui/blockinsert.png"));
    but->setMinimumSize(QSize(22,22));
    but->setToolTip(tr("Insert the active block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksInsert()));
    layButtons2->addWidget(but);

    lay->addLayout(layButtons);
    lay->addLayout(layButtons2);
    lay->addWidget(blockView);

    connect(blockView, SIGNAL(clicked(QModelIndex)), this, SLOT(slotActivated(QModelIndex)));
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

    if (blockList!=NULL) {
        activeBlock = blockList->getActive();
    } else {
        activeBlock = NULL;
    }

    blockModel->setBlockList(blockList);

    if (blockList==NULL) {
        RS_DEBUG->print("QG_BlockWidget::update(): blockList is NULL");
        return;
    }

    RS_Block* b = lastBlock;
    activateBlock(activeBlock);
    lastBlock = b;
    blockView->resizeRowsToContents();
    blockView->verticalScrollBar()->setValue(yPos);

    RS_DEBUG->print("QG_BlockWidget::update() done");
}



/**
 * Activates the given block and makes it the active
 * block in the blocklist.
 */
void QG_BlockWidget::activateBlock(RS_Block* block) {
    RS_DEBUG->print("QG_BlockWidget::activateBlock()");

    if (block==NULL || blockList==NULL) {
        return;
    }

    lastBlock = blockList->getActive();
    blockList->activate(block);
    QModelIndex idx = blockModel->getIndex (block);
    blockView->setCurrentIndex ( idx );
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
        blockList->activate(block);
    }
}

/**
 * Shows a context menu for the block widget. Launched with a right click.
 */
void QG_BlockWidget::contextMenuEvent(QContextMenuEvent *e) {

    QMenu* contextMenu = new QMenu(this);
    QLabel* caption = new QLabel(tr("Block Menu"), this);
    QPalette palette;
    palette.setColor(caption->backgroundRole(), RS_Color(0,0,0));
    palette.setColor(caption->foregroundRole(), RS_Color(255,255,255));
    caption->setPalette(palette);
    caption->setAlignment( Qt::AlignCenter );
    contextMenu->addAction( tr("&Defreeze all Blocks"), actionHandler,
                             SLOT(slotBlocksDefreezeAll()), 0);
    contextMenu->addAction( tr("&Freeze all Blocks"), actionHandler,
                             SLOT(slotBlocksFreezeAll()), 0);
    contextMenu->addAction( tr("&Add Block"), actionHandler,
                             SLOT(slotBlocksAdd()), 0);
    contextMenu->addAction( tr("&Remove Block"), actionHandler,
                             SLOT(slotBlocksRemove()), 0);
    contextMenu->addAction( tr("&Rename Block"), actionHandler,
                             SLOT(slotBlocksAttributes()), 0);
    contextMenu->addAction( tr("&Edit Block"), actionHandler,
                             SLOT(slotBlocksEdit()), 0);
    contextMenu->addAction( tr("&Insert Block"), actionHandler,
                             SLOT(slotBlocksInsert()), 0);
    contextMenu->addAction( tr("&Toggle Visibility"), actionHandler,
                             SLOT(slotBlocksToggleView()), 0);
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

