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

#include "rs_actionselectwindow.h"

#include "rs.h"
#include "rs_snapper.h"
#include "rs_selection.h"
#include "rs_overlaybox.h"


/**
 * Constructor.
 *
 * @param select true: select window. false: deselect window
 */
RS_ActionSelectWindow::RS_ActionSelectWindow(RS_EntityContainer& container,
        RS_GraphicView& graphicView,
        bool select)
        : RS_PreviewActionInterface("Select Window",
                            container, graphicView) {

    this->select = select;
}


QAction* RS_ActionSelectWindow::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action;

    if (type==RS2::ActionSelectWindow) {
        action = new QAction(tr("Select Window"),  NULL);
		action->setIcon(QIcon(":/extui/selectwindow.png"));
        action->setStatusTip(tr("Selects all Entities in a given Window"));
    } else {
        action = new QAction(tr("Deselect Window"), NULL);
		action->setIcon(QIcon(":/extui/deselectwindow.png"));
        action->setStatusTip(tr("Deselects all Entities"
                                " in a given Window"));		
	}
    return action;
}


void RS_ActionSelectWindow::init(int status) {
    RS_PreviewActionInterface::init(status);
    v1 = v2 = RS_Vector(false);
    snapMode = RS2::SnapFree;
    snapRes = RS2::RestrictNothing;
}



void RS_ActionSelectWindow::trigger() {
    RS_PreviewActionInterface::trigger();

    if (v1.valid && v2.valid) {
        if (graphicView->toGuiDX(v1.distanceTo(v2))>10) {

            bool cross = (v2.y>v1.y);

            RS_Selection s(*container, graphicView);
            s.selectWindow(v1, v2, select, cross);

            RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());

            init();
        }
    }
}



void RS_ActionSelectWindow::mouseMoveEvent(RS_MouseEvent* e) {
    if (getStatus()==SetCorner2 && v1.valid) {
        v2 = snapPoint(e);
        deletePreview();
		RS_Pen pen_f(RS_Color(50,50,255,40), RS2::Width00, RS2::SolidLine);
		RS_OverlayBox* ob=new RS_OverlayBox(preview, RS_OverlayBoxData(v1, v2));
		ob->setPen(pen_f);	
		preview->addEntity(ob);
		
		RS_Pen pen(RS_Color(218,105,24), RS2::Width00, RS2::SolidLine);
		
		// TODO change to a rs_box sort of entity
		RS_Line* e=new RS_Line(preview, RS_LineData(RS_Vector(v1.x, v1.y),  RS_Vector(v2.x, v1.y)));
		e->setPen(pen);
        preview->addEntity(e);

		e=new RS_Line(preview, RS_LineData(RS_Vector(v2.x, v1.y),  RS_Vector(v2.x, v2.y)));
		e->setPen(pen);
        preview->addEntity(e);

		e=new RS_Line(preview, RS_LineData(RS_Vector(v2.x, v2.y),  RS_Vector(v1.x, v2.y)));
		e->setPen(pen);
        preview->addEntity(e);

		e=new RS_Line(preview, RS_LineData(RS_Vector(v1.x, v2.y),  RS_Vector(v1.x, v1.y)));
		e->setPen(pen);
        preview->addEntity(e);
		 
        drawPreview();
    }
}



void RS_ActionSelectWindow::mousePressEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        switch (getStatus()) {
        case SetCorner1:
            v1 = snapPoint(e);
            setStatus(SetCorner2);
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionSelectWindow::mousePressEvent(): %f %f",
                    v1.x, v1.y);
}



void RS_ActionSelectWindow::mouseReleaseEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionSelectWindow::mouseReleaseEvent()");

    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        if (getStatus()==SetCorner2) {
            v2 = snapPoint(e);
            trigger();
        }
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        if (getStatus()==SetCorner2) {
            deletePreview();
        }
        init(getStatus()-1);
    }
}



void RS_ActionSelectWindow::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetCorner1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Choose first edge"), tr("Cancel"));
        break;
    case SetCorner2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Choose second edge"), tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionSelectWindow::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}



void RS_ActionSelectWindow::updateToolBar() {
    if (!isFinished()) {
        //RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSelect);
    } else {
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSelect);
    }
}

// EOF
