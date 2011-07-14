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

#include "rs_actiontoolregeneratedimensions.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_dimension.h"



RS_ActionToolRegenerateDimensions::RS_ActionToolRegenerateDimensions(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Tool Regen Dim",
                    container, graphicView) {}


QAction* RS_ActionToolRegenerateDimensions::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Regenerate Dimension Entities"),
                                  tr("&Regenerate Dimension Entities"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Regenerate Dimension Entities"), NULL);
    //action->zetStatusTip(tr("Regenerates all Dimension Entities"));

    return action;
}

void RS_ActionToolRegenerateDimensions::init(int status) {
    RS_ActionInterface::init(status);

    trigger();
}



void RS_ActionToolRegenerateDimensions::trigger() {

    RS_DEBUG->print("RS_ActionToolRegenerateDimensions::trigger()");

    int num = 0;
    for (RS_Entity* e = container->firstEntity(RS2::ResolveNone);
            e != NULL;
            e = container->nextEntity(RS2::ResolveNone)) {

        if (RS_Information::isDimension(e->rtti()) && e->isVisible()) {
			num++;
			if (((RS_Dimension*)e)->getLabel()==";;") {
				((RS_Dimension*)e)->setLabel("");
			}
            ((RS_Dimension*)e)->update(true);
        }
    }

    if (num>0) {
    	graphicView->redraw();
        RS_DIALOGFACTORY->commandMessage(
            tr("Regenerated %1 dimension entities").arg(num));
    } else {
        RS_DIALOGFACTORY->commandMessage(tr("No dimension entities found"));
    }

    finish();
}


// EOF
