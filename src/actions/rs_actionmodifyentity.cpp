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

#include "rs_actionmodifyentity.h"

#include "rs_snapper.h"



RS_ActionModifyEntity::RS_ActionModifyEntity(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Modify Entity", container, graphicView) {

    en = NULL;
}


QAction* RS_ActionModifyEntity::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Properties")
    QAction* action = new QAction(tr("&Properties"), NULL);
	action->setIcon(QIcon(":/extui/modifyentity.png"));
    action->setStatusTip(tr("Modify Entity Properties"));
    return action;
}

void RS_ActionModifyEntity::trigger() {
    if (en!=NULL) {
        RS_Entity* clone = en->clone();
        if (RS_DIALOGFACTORY->requestModifyEntityDialog(clone)) {
            container->addEntity(clone);

            graphicView->deleteEntity(en);
			en->setSelected(false);

			clone->setSelected(false);
            graphicView->drawEntity(clone);

            if (document!=NULL) {
                document->startUndoCycle();

                document->addUndoable(clone);
                en->setUndoState(true);
                document->addUndoable(en);

                document->endUndoCycle();
            }
            RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
        } else {
            delete clone;
        }

    } else {
        RS_DEBUG->print("RS_ActionModifyEntity::trigger: Entity is NULL\n");
    }
}



void RS_ActionModifyEntity::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        init(getStatus()-1);
    } else {
        en = catchEntity(e);
        trigger();
    }
}



void RS_ActionModifyEntity::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
