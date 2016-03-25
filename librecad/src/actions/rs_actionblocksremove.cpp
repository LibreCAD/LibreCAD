/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_debug.h"


RS_ActionBlocksRemove::RS_ActionBlocksRemove(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Remove Block", container, graphicView) {}

void RS_ActionBlocksRemove::trigger() {
	RS_DEBUG->print("RS_ActionBlocksRemove::trigger");

	if (!(graphic && document)) {
		finish(false);
		return;
	}
	RS_Block* block =
			RS_DIALOGFACTORY->requestBlockRemovalDialog(graphic->getBlockList());

	// list of containers that might refer to the block via inserts:
	std::vector<RS_EntityContainer*> containerList;
	containerList.push_back(graphic);
	RS_BlockList* blkLst = graphic->getBlockList();
	for (int bi=0; bi<blkLst->count(); bi++) {
		containerList.push_back(blkLst->at(bi));
	}

	if (!block) {
		finish(false);
		return;
	}
	document->startUndoCycle();
	for(auto cont: containerList){
		// remove all inserts from the graphic:
		bool done;
		do {
			done = true;
			for(auto e: *cont){

				if (e->rtti()==RS2::EntityInsert) {
					RS_Insert* ins = (RS_Insert*)e;
					if (ins->getName()==block->getName() && !ins->isUndone()) {
						document->addUndoable(ins);
						ins->setUndoState(true);
						done = false;
						break;
					}
				}
			}
		} while (!done);
	}

	// close all windows that are editing this block:
	RS_DIALOGFACTORY->closeEditBlockWindow(block);

	// Now remove the block from the block list, but do not delete:
	block->setUndoState(true);
	document->addUndoable(block);
	document->endUndoCycle();
	graphic->addBlockNotification();
	graphic->updateInserts();
	graphicView->redraw(RS2::RedrawDrawing);

	finish(false);
	RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());
}



void RS_ActionBlocksRemove::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
