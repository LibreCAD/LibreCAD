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

#include "rs_actionselectcontour.h"

#include "rs_selection.h"
#include "rs_snapper.h"



RS_ActionSelectContour::RS_ActionSelectContour(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Select Contours", container, graphicView) {

    en = NULL;
}

QAction* RS_ActionSelectContour::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("(De-)Select Contour")
    QAction* action = new QAction(tr("(De-)Select &Contour"),  NULL);
	action->setIcon(QIcon(":/extui/selectcontour.png"));
	//action->zetStatusTip(tr("(De-)Selects connected entities"));	
    return action;
}


void RS_ActionSelectContour::trigger() {
    if (en!=NULL) {
        if (en->isAtomic()) {
            RS_Selection s(*container, graphicView);
            s.selectContour(en);

            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
            }
        } else {
            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->commandMessage(
                    tr("Entity must be an Atomic Entity."));
            }
        }
    } else {
        RS_DEBUG->print("RS_ActionSelectContour::trigger: Entity is NULL\n");
    }
}



void RS_ActionSelectContour::mouseReleaseEvent(RS_MouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
        en = catchEntity(e);
        trigger();
    }
}



void RS_ActionSelectContour::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
