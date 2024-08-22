/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/



#include <QMouseEvent>

#include "rs_actiondrawcircle3p.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"

struct RS_ActionDrawCircle3P::Points {
	RS_CircleData data;
	/**
	 * 1st point.
	 */
	RS_Vector point1 = RS_Vector(false);
	/**
	 * 2nd point.
	 */
	RS_Vector point2 = RS_Vector(false);
	/**
	 * 3rd point.
	 */
	RS_Vector point3 = RS_Vector(false);
};

RS_ActionDrawCircle3P::RS_ActionDrawCircle3P(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :LC_ActionDrawCircleBase("Draw circles",
						   container, graphicView)
        , pPoints(std::make_unique<Points>()){
	actionType=RS2::ActionDrawCircle3P;
}

RS_ActionDrawCircle3P::~RS_ActionDrawCircle3P() = default;


void RS_ActionDrawCircle3P::reset(){
    pPoints.reset(new Points{});
}

void RS_ActionDrawCircle3P::trigger(){
    RS_PreviewActionInterface::trigger();

    preparePreview();
    if (pPoints->data.isValid()){
        auto *circle = new RS_Circle{container, pPoints->data};
        circle->setLayerToActive();
        circle->setPenToActive();
        container->addEntity(circle);

        addToDocumentUndoable(circle);

        RS_Vector rz = graphicView->getRelativeZero();
        if (moveRelPointAtCenterAfterTrigger){
            rz = pPoints->data.center;
        }
        moveRelativeZero(rz);
        graphicView->redraw(RS2::RedrawDrawing);
        drawSnapper();

        setStatus(SetPoint1);
        reset();
    } else
        RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid circle data."));
}

void RS_ActionDrawCircle3P::preparePreview(){
    pPoints->data = RS_CircleData{};
    if (pPoints->point1.valid && pPoints->point2.valid && pPoints->point3.valid){
        RS_Circle circle{nullptr, pPoints->data};
        bool suc = circle.createFrom3P(pPoints->point1,
                                       pPoints->point2,
                                       pPoints->point3);
        if (suc){
            pPoints->data = circle.getData();
        }
    }
}
// todo - think about preview improving to give it more geometric meaning
void RS_ActionDrawCircle3P::mouseMoveEvent(QMouseEvent *e){
    RS_Vector mouse = snapPoint(e);

    switch (getStatus()) {
        case SetPoint1:
            pPoints->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;

        case SetPoint2: {
            deletePreview();
            mouse = getSnapAngleAwarePoint(e, pPoints->point1, mouse, true);

            pPoints->point2 = mouse;
            RS_Vector center = (mouse + pPoints->point1) / 2;
            double radius = pPoints->point1.distanceTo(center);
            previewCircle(RS_CircleData(center, radius));

            if (showRefEntitiesOnPreview) {
                previewRefPoint(pPoints->point1);
                previewRefLine(pPoints->point1, mouse);
                previewRefSelectablePoint(mouse);
            }

            drawPreview();

            break;
        }
        case SetPoint3: {
            pPoints->point3 = mouse;
            preparePreview();
            deletePreview();
            if (pPoints->data.isValid()) {
                previewCircle(pPoints->data);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->data.center);
                    previewRefPoint(pPoints->point1);
                    previewRefPoint(pPoints->point2);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(pPoints->point1, pPoints->data.center);
                    previewRefLine(pPoints->point2, pPoints->data.center);
                    previewRefLine(mouse, pPoints->data.center);
                }
            }

            drawPreview();
            break;
        }
    }
}

void RS_ActionDrawCircle3P::onMouseLeftButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    RS_Vector coord = snapPoint(e);
    if (status == SetPoint2){
        coord = getSnapAngleAwarePoint(e, pPoints->point1, coord);
    }
    fireCoordinateEvent(coord);
}

void RS_ActionDrawCircle3P::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawCircle3P::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            pPoints->point1 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            pPoints->point2 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3: {
            pPoints->point3 = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawCircle3P::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify first point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetPoint3:
            updateMouseWidgetTRBack(tr("Specify third point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
