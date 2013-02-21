/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
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
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"



QC_ActionGetPoint::QC_ActionGetPoint(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Get Point",
                           container, graphicView) {
    completed = false;
    mesage = tr("Specify a point");
    setTargetPoint = false;
}


void QC_ActionGetPoint::trigger() {

    RS_DEBUG->print("QC_ActionGetPoint::trigger()");
    completed = true;
    updateMouseButtonHints();
}


void QC_ActionGetPoint::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent begin");

        RS_Vector mouse = snapPoint(e);
        if(setTargetPoint){
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
        } else {
            targetPoint = mouse;
        }

    RS_DEBUG->print("QC_ActionGetPoint::mouseMoveEvent end");
}



void QC_ActionGetPoint::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}

void QC_ActionGetPoint::coordinateEvent(RS_CoordinateEvent* e) {

    if (e==NULL) {
        return;
    }

    RS_Vector pos = e->getCoordinate();

        targetPoint = pos;
        graphicView->moveRelativeZero(targetPoint);
        trigger();
}


void QC_ActionGetPoint::updateMouseButtonHints() {
    if (!completed)
        RS_DIALOGFACTORY->updateMouseWidget(mesage, tr("Cancel"));
    else
        RS_DIALOGFACTORY->updateMouseWidget("", "");
}


void QC_ActionGetPoint::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


void QC_ActionGetPoint::setBasepoint(QPointF* basepoint){
    referencePoint.x = basepoint->x();
    referencePoint.y = basepoint->y();
    setTargetPoint = true;
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
