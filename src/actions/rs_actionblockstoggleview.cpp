/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actionblockstoggleview.h"

#include "rs_graphic.h"



RS_ActionBlocksToggleView::RS_ActionBlocksToggleView(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Toggle Block Visibility",
                    container, graphicView) {}


QAction* RS_ActionBlocksToggleView::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
// RVT_PORT	QAction* action = new QAction(tr("Toggle Block Visibility"),
//								  tr("&Toggle Block"),
//								  QKeySequence(), NULL);
	
	QAction* action = new QAction(tr("Toggle Block Visibility"),  NULL);
	action->setStatusTip(tr("Toggle Block"));

		return action;
}


void RS_ActionBlocksToggleView::trigger() {
    RS_DEBUG->print("toggle block");
    if (graphic!=NULL) {
        RS_Block* block = graphic->getActiveBlock();
        graphic->toggleBlock(block);
    }
	graphicView->redraw(RS2::RedrawDrawing); 

    finish();
}



void RS_ActionBlocksToggleView::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
