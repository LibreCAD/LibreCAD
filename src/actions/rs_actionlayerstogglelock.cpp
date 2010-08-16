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

#include "rs_actionlayerstogglelock.h"

#include "rs_graphic.h"



RS_ActionLayersToggleLock::RS_ActionLayersToggleLock(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Toggle Layer Visibility",
                    container, graphicView) {}


QAction* RS_ActionLayersToggleLock::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Toggle Layer Lock"),
                                  tr("&Toggle Lock"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Toggle Layer Lock"), NULL);
    action->setStatusTip(tr("Toggle Lock"));
    return action;
}

void RS_ActionLayersToggleLock::trigger() {
    RS_DEBUG->print("toggle layer");
    if (graphic!=NULL) {
        RS_Layer* layer = graphic->getActiveLayer();
        if (layer!=NULL) {
            graphic->toggleLayerLock(layer);

            // deselect entities on locked layer:
            if (layer->isLocked()) {
                for (RS_Entity* e=container->firstEntity(); e!=NULL;
                        e=container->nextEntity()) {
                    if (e!=NULL && e->isVisible() && e->getLayer()==layer) {

                        if (graphicView!=NULL) {
                            graphicView->deleteEntity(e);
                        }

                        e->setSelected(false);

                        if (graphicView!=NULL) {
                            graphicView->drawEntity(e);
                        }
                    }
                }
            }
        }

    }
    finish();
}



void RS_ActionLayersToggleLock::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
