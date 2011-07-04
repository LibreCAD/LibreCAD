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

#include "rs_actionblocksattributes.h"

#include "rs_graphic.h"
#include "rs_dialogfactory.h"



RS_ActionBlocksAttributes::RS_ActionBlocksAttributes(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Edit Block Attributes", container, graphicView) {}




QAction* RS_ActionBlocksAttributes::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Rename Block")
    QAction* action = new QAction(tr("&Rename Block"), NULL);
	action->setIcon(QIcon(":/ui/blockattributes.png"));
    //action->zetStatusTip(tr("Rename Block and all Inserts"));
    return action;
}



void RS_ActionBlocksAttributes::trigger() {
    RS_DEBUG->print("editing block attributes");

    if (graphic!=NULL && RS_DIALOGFACTORY!=NULL) {
        RS_Block* block = graphic->getActiveBlock();
        RS_BlockList* blockList = graphic->getBlockList();
        if (blockList!=NULL && block!=NULL) {
            QString oldName = block->getName();

            RS_BlockData d;
            d = RS_DIALOGFACTORY->requestBlockAttributesDialog(
                    blockList);

            if (d.isValid()) {

                RS_String newName = d.name;
                blockList->rename(block, newName);

                // update the name of all inserts:
                graphic->renameInserts(oldName, newName);

                graphic->addBlockNotification();
            }
        }

    }
    finish();
}



void RS_ActionBlocksAttributes::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
