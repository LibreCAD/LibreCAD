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

#include "qc_actiongetcorner.h"

#include <QPointF>
#include <QMouseEvent>
#include "rs_snapper.h"
#include "rs_graphicview.h"
#include "rs_overlaybox.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct QC_ActionGetCorner::Points {
    RS_MoveData data;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
    QString message;
};

QC_ActionGetCorner::QC_ActionGetCorner(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Get Point",
						   container, graphicView)
        , canceled(false)
		, completed{false}
		, setTargetPoint{false}
		, pPoints(std::make_unique<Points>()){
    pPoints->targetPoint = RS_Vector(0,0);
}

QC_ActionGetCorner::~QC_ActionGetCorner() = default;

void QC_ActionGetCorner::trigger() {
    RS_DEBUG->print("QC_ActionGetCorner::trigger()");
    completed = true;
    updateMouseButtonHints();
}

void QC_ActionGetCorner::mouseMoveEvent(QMouseEvent* e) {
    deletePreview();
    RS_DEBUG->print("QC_ActionGetCorner::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    if(setTargetPoint){
        if (pPoints->referencePoint.valid) {
            pPoints->targetPoint = mouse;

            auto* ob=new RS_OverlayBox(preview.get(), RS_OverlayBoxData(pPoints->referencePoint, mouse));
            preview->addEntity(ob);

            //line->setPen(RS_Pen(RS_Color(0,0,0), RS2::Width00, RS2::DotLine ));

            RS_DEBUG->print("QC_ActionGetCorner::mouseMoveEvent: draw preview");
            preview->addSelectionFrom(*container,graphicView);
        }
    } else {
        pPoints->targetPoint = mouse;
    }

    RS_DEBUG->print("QC_ActionGetCorner::mouseMoveEvent end");
    drawPreview();
}

void QC_ActionGetCorner::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        canceled = true;
        completed = true;
        finish();
    }
}

void QC_ActionGetCorner::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    pPoints->targetPoint = pos;
    moveRelativeZero(pPoints->targetPoint);
    trigger();
}

void QC_ActionGetCorner::updateMouseButtonHints() {
    if (!completed)
        updateMouseWidget(pPoints->message, tr("Cancel"));
    else
        updateMouseWidget();
}

RS2::CursorType QC_ActionGetCorner::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void QC_ActionGetCorner::setBasepoint(QPointF* basepoint){
    pPoints->referencePoint.x = basepoint->x();
    pPoints->referencePoint.y = basepoint->y();
    pPoints->referencePoint.valid = true;
    setTargetPoint = true;
}

void QC_ActionGetCorner::setMessage(QString msg){
    pPoints->message = msg;
}

void QC_ActionGetCorner::getPoint(QPointF *point){
    if (pPoints)    {
        point->setX(pPoints->targetPoint.x);
        point->setY(pPoints->targetPoint.y);
    }
}
