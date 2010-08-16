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

#include "rs_actionlayersadd.h"

#include "rs_graphic.h"



RS_ActionLayersAdd::RS_ActionLayersAdd(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_ActionInterface("Add Layer", container, graphicView) {}


QAction* RS_ActionLayersAdd::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Add Layer"), tr("&Add Layer"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Add Layer"), NULL);
    action->setStatusTip(tr("Add Layer"));

    return action;
}

void RS_ActionLayersAdd::trigger() {
    RS_DEBUG->print("add layer");

    if (RS_DIALOGFACTORY!=NULL) {
        if (graphic) {
            RS_Layer* layer = RS_DIALOGFACTORY->requestNewLayerDialog(
                                  graphic->getLayerList());
            if (layer!=NULL)
                graphic->addLayer(layer);
        }
    }
    finish();
}



void RS_ActionLayersAdd::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
