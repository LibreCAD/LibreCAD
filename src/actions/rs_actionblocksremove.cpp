/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#include "rs_actionblocksremove.h"

#include "rs_block.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_dialogfactory.h"
#include "rs_ptrlist.h"



RS_ActionBlocksRemove::RS_ActionBlocksRemove(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Remove Block", container, graphicView) {}


QAction* RS_ActionBlocksRemove::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Remove Block")
	QAction* action = new QAction(tr("&Remove Block"), NULL);
    //action->zetStatusTip(tr("Remove Block"));
	action->setIcon(QIcon(":/ui/blockremove.png"));
	return action;
}

void RS_ActionBlocksRemove::trigger() {
    RS_DEBUG->print("RS_ActionBlocksRemove::trigger");

    if (graphic!=NULL) {
        RS_Block* block =
            RS_DIALOGFACTORY->requestBlockRemovalDialog(graphic->getBlockList());

        // list of containers that might refer to the block via inserts:
        RS_PtrList<RS_EntityContainer> containerList;
        containerList.append(graphic);
        RS_BlockList* blkLst = graphic->getBlockList();
        for (uint bi=0; bi<blkLst->count(); bi++) {
            containerList.append(blkLst->at(bi));
        }
        
        if (block!=NULL) {
            
            for (RS_EntityContainer* cont = containerList.first();
                cont!=NULL;
                cont=containerList.next()) {
        
                // remove all inserts from the graphic:
                bool done;
                do {
                    done = true;
                    for (RS_Entity* e=cont->firstEntity(RS2::ResolveNone);
                            e!=NULL;
                            e=cont->nextEntity(RS2::ResolveNone)) {

                        if (e->rtti()==RS2::EntityInsert) {
                            RS_Insert* ins = (RS_Insert*)e;
                            if (ins->getName()==block->getName()) {
                                cont->removeEntity(ins);
                                done = false;
                                break;
                            }
                        }
                    }
                } while (!done);
            }

			// close all windows that are editing this block:
        	if (RS_DIALOGFACTORY!=NULL) {
	            RS_DIALOGFACTORY->closeEditBlockWindow(block);
    	    }

            // Now remove the block from the block list:
            graphic->removeBlock(block);
            graphic->updateInserts();
			graphicView->redraw(RS2::RedrawDrawing); 

        }
    }

    finish();
    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}



void RS_ActionBlocksRemove::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
