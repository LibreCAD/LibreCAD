/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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

#include "qc_actiongetpoint.h"

#include "rs_snapper.h"



QC_ActionGetPoint::QC_ActionGetPoint(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Get Point",
                           container, graphicView) {
    completed = false;
    mesage = tr("Specify a point");

}


void QC_ActionGetPoint::init(int status) {
    RS_ActionInterface::init(status);
}


void QC_ActionGetPoint::trigger() {

    RS_DEBUG->print("QC_ActionGetPoint::trigger()");
    completed = true;
}


void QC_ActionGetPoint::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent begin");
    if (getStatus()==SetReferencePoint ||
            getStatus()==SetTargetPoint) {

        RS_Vector mouse = snapPoint(e);
        switch (getStatus()) {
        case SetReferencePoint:
            targetPoint = mouse;
            break;

        case SetTargetPoint:
            if (referencePoint.valid) {
                targetPoint = mouse;
                deletePreview();
                RS_Line *line =new RS_Line(preview,
                                       RS_LineData(referencePoint, mouse));
                line->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));
                preview->addEntity(line);
                RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent: draw preview");
                drawPreview();
                preview->addSelectionFrom(*container);
            }
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent end");
}



void QC_ActionGetPoint::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void QC_ActionGetPoint::coordinateEvent(RS_CoordinateEvent* e) {

    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();

    switch (getStatus()) {
    case SetReferencePoint:
    case SetTargetPoint:
        targetPoint = pos;
        graphicView->moveRelativeZero(targetPoint);
        trigger();
        finish();
        break;

    default:
        break;
    }
}


void QC_ActionGetPoint::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint:
    case SetTargetPoint:
        RS_DIALOGFACTORY->updateMouseWidget(mesage, tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}


void QC_ActionGetPoint::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


void QC_ActionGetPoint::updateToolBar() {
    switch (getStatus()) {
    case SetReferencePoint:
    case SetTargetPoint:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    default:
        RS_DIALOGFACTORY->requestPreviousMenu();
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}


void QC_ActionGetPoint::setBasepoint(QPointF* basepoint){
    referencePoint.x = basepoint->x();
    referencePoint.y = basepoint->y();
    setStatus(SetTargetPoint);
}


void QC_ActionGetPoint::setMesage(QString msg){
    mesage = msg;
}


void QC_ActionGetPoint::getPoint(QPointF *point) {
    if (completed) {
        point->setX(targetPoint.x);
        point->setY(targetPoint.y);
    } else {
        point->setX(0.0);
        point->setY(0.0);
    }
}

// EOF
