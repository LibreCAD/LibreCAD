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

#include "rs_actionlayersremove.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphic.h"



RS_ActionLayersRemove::RS_ActionLayersRemove(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Remove Layer", container, graphicView) {}


QAction* RS_ActionLayersRemove::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("&Remove Layer")
    QAction* action = new QAction(tr("Remove Layer"), NULL);
    //action->zetStatusTip(tr("Remove Layer"));
	action->setIcon(QIcon(":/ui/layerremove.png"));
    return action;
}


void RS_ActionLayersRemove::trigger() {
    RS_DEBUG->print("RS_ActionLayersRemove::trigger");

    if (graphic!=NULL) {
        RS_Layer* layer =
            RS_DIALOGFACTORY->requestLayerRemovalDialog(graphic->getLayerList());

        /*
              if (layer!=NULL && layer->getName()!="0") {

                  graphic->startUndoCycle();
                  for (RS_Entity* e=graphic->firstEntity(RS2::ResolveNone);
                          e!=NULL;
                          e=graphic->nextEntity(RS2::ResolveNone)) {

                      if (e->getLayer()!=NULL &&
                              e->getLayer()->getName()==layer->getName()) {

                          e->setUndoState(true);
                          e->setLayer("0");
                          graphic->addUndoable(e);
                      }
                  }


                  graphic->endUndoCycle();
        */

        // Now remove the layer from the layer list:
        graphic->removeLayer(layer);
    }
    finish();
    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}



void RS_ActionLayersRemove::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
