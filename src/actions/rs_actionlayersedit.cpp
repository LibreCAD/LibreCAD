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

#include "rs_actionlayersedit.h"

#include "rs_graphic.h"



RS_ActionLayersEdit::RS_ActionLayersEdit(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Edit Layer", container, graphicView) {}


QAction* RS_ActionLayersEdit::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Edit Layer")
    QAction* action = new QAction(tr("&Edit Layer"), NULL);
    //action->zetStatusTip(tr("Edit Layer"));
	action->setIcon(QIcon(":/ui/layeredit.png"));
    return action;
}

void RS_ActionLayersEdit::trigger() {
    RS_DEBUG->print("RS_ActionLayersEdit::trigger");

    RS_Layer* layer = NULL;

    if (graphic!=NULL) {
        layer =
            RS_DIALOGFACTORY->requestEditLayerDialog(graphic->getLayerList());

        if (layer!=NULL) {
            graphic->editLayer(graphic->getActiveLayer(), *layer);

            // update updateable entities on the layer that has changed
            for (RS_Entity* e=graphic->firstEntity(RS2::ResolveNone);
                    e!=NULL;
                    e=graphic->nextEntity(RS2::ResolveNone)) {

                RS_Layer* l = e->getLayer();
                if (l!=NULL && l->getName()==layer->getName()) {
                    e->update();
                }
            }
        }
    }
    finish();

	graphicView->redraw(RS2::RedrawDrawing); 

}



void RS_ActionLayersEdit::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
