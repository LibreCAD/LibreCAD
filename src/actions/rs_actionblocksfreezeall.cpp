/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actionblocksfreezeall.h"

#include "rs_graphic.h"



RS_ActionBlocksFreezeAll::RS_ActionBlocksFreezeAll(bool freeze,
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Freeze all Blocks",
                    container, graphicView) {

    this->freeze = freeze;
}

QAction* RS_ActionBlocksFreezeAll::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action = NULL;
	
	if (type==RS2::ActionBlocksFreezeAll) {
		// tr("Freeze all")
		action= new QAction(tr("&Freeze all"), NULL);
    	//action->zetStatusTip(tr("Freeze all blocks"));
		action->setIcon(QIcon(":/ui/blockfreeze.png"));
	}
	else if (type==RS2::ActionBlocksDefreezeAll) {
		// tr("&Defreeze all")
		action = new QAction(tr("&Defreeze all"), NULL); 
        //action->zetStatusTip(tr("Defreeze all blocks"));
		action->setIcon(QIcon(":/ui/blockdefreeze.png"));

	}

    return action;
}


void RS_ActionBlocksFreezeAll::trigger() {
    RS_DEBUG->print("RS_ActionBlocksFreezeAll::trigger");
    if (graphic!=NULL) {
        graphic->freezeAllBlocks(freeze);
    }
	graphicView->redraw(RS2::RedrawDrawing); 

    finish();
}



void RS_ActionBlocksFreezeAll::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
