/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
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

#include <QToolTip>
#include <QToolButton>
#include <QMenu>

/**
 * Constructor.
 */
QG_BlockWidget::QG_BlockWidget(QG_ActionHandler* ah, QWidget* parent,
                               const char* name, Qt::WFlags f)
        : QWidget(parent, name, f),
        pxmVisible(":/ui/visibleblock.png"),
        pxmHidden(":/ui/hiddenblock.png"),
        pxmAdd(":/ui/blockadd.png"),
        pxmRemove(":/ui/blockremove.png"),
        pxmAttributes(":/ui/blockattributes.png"),
        pxmEdit(":/ui/blockedit.png"),
        pxmInsert(":/ui/blockinsert.png"),
        pxmDefreezeAll(":/ui/visibleblock.png"),
        pxmFreezeAll(":/ui/hiddenblock.png") {

    actionHandler = ah;
    blockList = NULL;
	lastBlock = NULL;

    listBox = new Q3ListBox(this, "blockbox");
    listBox->setDragSelect(false);
    listBox->setMultiSelection(false);
    listBox->setSmoothScrolling(true);
	listBox->setFocusPolicy(Qt::NoFocus);

    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setSpacing ( 0 );
    /*
	QLabel* caption = new QLabel(tr("Block List"), this, "caption");
    caption->setAlignment(Qt::AlignCenter);
    caption->setPaletteBackgroundColor(black);
    caption->setPaletteForegroundColor(white);
	*/

    QHBoxLayout* layButtons = new QHBoxLayout();
    QHBoxLayout* layButtons2 = new QHBoxLayout();
    QToolButton* but;
    // show all blocks:
    but = new QToolButton(this);
    but->setPixmap(pxmDefreezeAll);
    but->setMinimumSize(QSize(22,22));
    QToolTip::add(but, tr("Show all blocks"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksDefreezeAll()));
    layButtons->addWidget(but);
    // hide all blocks:
    but = new QToolButton(this);
    but->setPixmap(pxmFreezeAll);
    but->setMinimumSize(QSize(22,22));
    QToolTip::add(but, tr("Hide all blocks"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksFreezeAll()));
    layButtons->addWidget(but);
    // add block:
    but = new QToolButton(this);
    but->setPixmap(pxmAdd);
    but->setMinimumSize(QSize(22,22));
    QToolTip::add(but, tr("Add a block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksAdd()));
    layButtons->addWidget(but);
    // remove block:
    but = new QToolButton(this);
    but->setPixmap(pxmRemove);
    but->setMinimumSize(QSize(22,22));
    QToolTip::add(but, tr("Remove the active block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksRemove()));
    layButtons->addWidget(but);
    // edit attributes:
    but = new QToolButton(this);
    but->setPixmap(pxmAttributes);
    but->setMinimumSize(QSize(22,22));
    QToolTip::add(but, tr("Rename the active block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksAttributes()));
    layButtons->addWidget(but);
    // edit block:
    but = new QToolButton(this);
    but->setPixmap(pxmEdit);
    but->setMinimumSize(QSize(22,22));
    QToolTip::add(but, tr("Edit the active block\n"
                          "in a separate window"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksEdit()));
    layButtons2->addWidget(but);
    // insert block:
    but = new QToolButton(this);
    but->setPixmap(pxmInsert);
    but->setMinimumSize(QSize(22,22));
    QToolTip::add(but, tr("Insert the active block"));
    connect(but, SIGNAL(clicked()),
            actionHandler, SLOT(slotBlocksInsert()));
    layButtons2->addWidget(but);

    //lay->addWidget(caption);
    lay->addLayout(layButtons);
    lay->addLayout(layButtons2);
    lay->addWidget(listBox);

    //connect(listBox, SIGNAL(doubleClicked(QListBoxItem*)),
    //        actionHandler, SLOT(slotBlocksToggleView()));

    connect(listBox, SIGNAL(highlighted(const QString&)),
            this, SLOT(slotActivated(const QString&)));
	
	connect(listBox, SIGNAL(mouseButtonClicked(int, Q3ListBoxItem*, 
		const QPoint&)),
		this, SLOT(slotMouseButtonClicked(int, Q3ListBoxItem*, const QPoint&)));

    //boxLayout()->addWidget(listBox);
}



/**
 * Destructor
 */
QG_BlockWidget::~QG_BlockWidget() {
    delete listBox;
}



/**
 * Updates the block box from the blocks in the graphic.
 */
void QG_BlockWidget::update() {
    RS_DEBUG->print("QG_BlockWidget::update()");

    int yPos = listBox->contentsY();
    
	RS_Block* activeBlock;
    if (blockList!=NULL) {
        activeBlock = blockList->getActive();
    } else {
        activeBlock = NULL;
    }

    listBox->clear();

    if (blockList==NULL) {
		RS_DEBUG->print("QG_BlockWidget::update(): blockList is NULL");
        return;
    }

    for (uint i=0; i<blockList->count(); ++i) {
        RS_Block* blk = blockList->at(i);
        if (!blk->isFrozen()) {
            listBox->insertItem(pxmVisible, blk->getName());
        } else {
            listBox->insertItem(pxmHidden, blk->getName());
        }
    }

    listBox->sort();
	
	RS_Block* b = lastBlock;
    highlightBlock(activeBlock);
	lastBlock = b;
    listBox->setContentsPos(0, yPos);

    //highlightBlock(blockList->getActiveBlock());
    //listBox->setContentsPos(0, yPos);
    RS_DEBUG->print("QG_BlockWidget::update() done");
}



/**
 * Highlights (activates) the given block and makes it 
 * the active block in the blocklist.
 */
void QG_BlockWidget::highlightBlock(RS_Block* block) {
    RS_DEBUG->print("QG_BlockWidget::highlightBlock()");

    if (block==NULL || listBox==NULL) {
        return;
    }

    blockList->activate(block);
    QString name = block->getName();

    for (int i=0; i<(int)listBox->count(); ++i) {
        if (listBox->text(i)==name) {
            listBox->setCurrentItem(i);
            break;
        }
    }
}



/**
 * Toggles the view of the given block. This is usually called when 
 * an item is double clicked.
 */
/*
void QG_BlockWidget::slotToggleView(QListBoxItem* item) {
    RS_DEBUG->print("QG_BlockWidget::slotToggleView()");
 
    if (item==NULL || blockList==NULL) {
        return;
    }
 
    int index = listBox->index(item);
    RS_Block* block = blockList->find(item->text());
 
    if (block!=NULL) {
        blockList->toggleBlock(item->text());
        if (!block->isFrozen()) {
        	listBox->changeItem(pxmVisible, item->text(), index);
        } else {
            listBox->changeItem(*pxmHidden, item->text(), index);
        }
 
    }
}
*/



/**
 * Called when the user activates (highlights) a block.
 */
void QG_BlockWidget::slotActivated(const QString& blockName) {
    RS_DEBUG->print("QG_BlockWidget::slotActivated(): %s", blockName.latin1());

    if (blockList==NULL) {
        return;
    }

	lastBlock = blockList->getActive();

    blockList->activate(blockName);
}



/**
 * Called for every mouse click.
 */
void QG_BlockWidget::slotMouseButtonClicked(int /*button*/, 
    Q3ListBoxItem* item, const QPoint& pos) {

	QPoint p = mapFromGlobal(pos);

	RS_Block* b = lastBlock;

	if (p.x()<23) {
		actionHandler->slotBlocksToggleView();
		highlightBlock(b);
	}
	else {
		if (item!=NULL && blockList!=NULL) {
			lastBlock = blockList->find(item->text());
		}
	}
}


/**
 * Shows a context menu for the block widget. Launched with a right click.
 */
void QG_BlockWidget::contextMenuEvent(QContextMenuEvent *e) {

    //QListBoxItem* item = listBox->selectedItem();
    QMenu* contextMenu = new QMenu(this);
    QLabel* caption = new QLabel(tr("Block Menu"), this);
    caption->setPaletteBackgroundColor(RS_Color(0,0,0));
    caption->setPaletteForegroundColor(RS_Color(255,255,255));
    caption->setAlignment( Qt::AlignCenter );
    // RVT_PORT contextMenu->insertItem( caption );
    contextMenu->insertItem( tr("&Defreeze all Blocks"), actionHandler,
                             SLOT(slotBlocksDefreezeAll()), 0);
    contextMenu->insertItem( tr("&Freeze all Blocks"), actionHandler,
                             SLOT(slotBlocksFreezeAll()), 0);
    contextMenu->insertItem( tr("&Add Block"), actionHandler,
                             SLOT(slotBlocksAdd()), 0);
    contextMenu->insertItem( tr("&Remove Block"), actionHandler,
                             SLOT(slotBlocksRemove()), 0);
    contextMenu->insertItem( tr("&Rename Block"), actionHandler,
                             SLOT(slotBlocksAttributes()), 0);
    contextMenu->insertItem( tr("&Edit Block"), actionHandler,
                             SLOT(slotBlocksEdit()), 0);
    contextMenu->insertItem( tr("&Insert Block"), actionHandler,
                             SLOT(slotBlocksInsert()), 0);
    contextMenu->insertItem( tr("&Toggle Visibility"), actionHandler,
                             SLOT(slotBlocksToggleView()), 0);
    contextMenu->insertItem( tr("&Create New Block"), actionHandler,
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

