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

#include "rs_actionmodifymirror.h"

#include "rs_snapper.h"



RS_ActionModifyMirror::RS_ActionModifyMirror(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Mirror Entities",
                           container, graphicView) {}

QAction* RS_ActionModifyMirror::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
/* RVT_PORT    QAction* action = new QAction(tr("Mirror"), tr("&Mirror"),
                                  QKeySequence(), NULL); */
    QAction* action = new QAction(tr("Mirror"), NULL);
    action->setStatusTip(tr("Mirror Entities"));
    return action;
}


void RS_ActionModifyMirror::init(int status) {
    RS_ActionInterface::init(status);
}



void RS_ActionModifyMirror::trigger() {

    RS_DEBUG->print("RS_ActionModifyMirror::trigger()");

    RS_Modification m(*container, graphicView);
    m.mirror(data);

    if (RS_DIALOGFACTORY!=NULL) {
        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
    }
}



void RS_ActionModifyMirror::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent begin");

    if (getStatus()==SetAxisPoint1 ||
            getStatus()==SetAxisPoint2) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetAxisPoint1:
            axisPoint1 = mouse;
            break;

        case SetAxisPoint2:
            if (axisPoint1.valid) {
                axisPoint2 = mouse;

                deletePreview();
                preview->addSelectionFrom(*container);
                preview->mirror(axisPoint1, axisPoint2);

                preview->addEntity(new RS_Line(preview,
                                               RS_LineData(axisPoint1,
                                                           axisPoint2)));

                drawPreview();
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyMirror::mouseMoveEvent end");
}



void RS_ActionModifyMirror::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);

    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionModifyMirror::coordinateEvent(RS_CoordinateEvent* e) {
    if (e==NULL) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

        switch (getStatus()) {
        case SetAxisPoint1:
            axisPoint1 = mouse;
            setStatus(SetAxisPoint2);
        	graphicView->moveRelativeZero(mouse);
            break;

        case SetAxisPoint2:
            axisPoint2 = mouse;
            setStatus(ShowDialog);
        	graphicView->moveRelativeZero(mouse);
            if (RS_DIALOGFACTORY!=NULL) {
                if (RS_DIALOGFACTORY->requestMirrorDialog(data)) {
                    data.axisPoint1 = axisPoint1;
                    data.axisPoint2 = axisPoint2;
                    deletePreview();
                    trigger();
                    finish();
                }
            }
            break;

        default:
            break;
        }
}



void RS_ActionModifyMirror::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
            /*case Select:
                RS_DIALOGFACTORY->updateMouseWidget(tr("Pick entities to move"),
                                               tr("Cancel"));
                break;*/
        case SetAxisPoint1:
            RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify first point of mirror line"),
                tr("Cancel"));
            break;
        case SetAxisPoint2:
            RS_DIALOGFACTORY->updateMouseWidget(
                tr("Specify second point of mirror line"),
                tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionModifyMirror::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyMirror::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetAxisPoint1:
        case SetAxisPoint2:
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
            break;
        default:
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
            break;
        }
    }
}


// EOF
