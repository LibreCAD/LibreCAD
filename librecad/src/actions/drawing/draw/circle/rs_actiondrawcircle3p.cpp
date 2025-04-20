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


#include "rs_actiondrawcircle3p.h"

#include "rs_circle.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"

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

RS_ActionDrawCircle3P::RS_ActionDrawCircle3P(LC_ActionContext *actionContext)
        :LC_ActionDrawCircleBase("Draw circles",actionContext,RS2::ActionDrawCircle3P)
        , m_actionData(std::make_unique<Points>()){
}

RS_ActionDrawCircle3P::~RS_ActionDrawCircle3P() = default;


void RS_ActionDrawCircle3P::reset(){
    m_actionData.reset(new Points{});
}

void RS_ActionDrawCircle3P::doTrigger() {
    preparePreview();
    if (m_actionData->data.isValid()){
        auto *circle = new RS_Circle{m_container, m_actionData->data};

        setPenAndLayerToActive(circle);

        if (m_moveRelPointAtCenterAfterTrigger){
            moveRelativeZero(m_actionData->data.center);
        }

        undoCycleAdd(circle);

        setStatus(SetPoint1);
        reset();
    } else
        RS_DIALOGFACTORY->requestWarningDialog(tr("Invalid circle data."));
}

void RS_ActionDrawCircle3P::preparePreview(){
    m_actionData->data = RS_CircleData{};
    if (m_actionData->point1.valid && m_actionData->point2.valid && m_actionData->point3.valid){
        RS_Circle circle{nullptr, m_actionData->data};
        bool suc = circle.createFrom3P(m_actionData->point1,
                                       m_actionData->point2,
                                       m_actionData->point3);
        if (suc){
            m_actionData->data = circle.getData();
        }
    }
}
// todo - think about preview improving to give it more geometric meaning
void RS_ActionDrawCircle3P::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint1:
            m_actionData->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;

        case SetPoint2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->point1, mouse, true);
            m_actionData->point2 = mouse;
            RS_Vector center = (mouse + m_actionData->point1) / 2;
            double radius = m_actionData->point1.distanceTo(center);
            previewCircle(RS_CircleData(center, radius));

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->point1);
                previewRefLine(m_actionData->point1, mouse);
                previewRefSelectablePoint(mouse);
            }
            break;
        }
        case SetPoint3: {
            m_actionData->point3 = mouse;
            preparePreview();
            if (m_actionData->data.isValid()) {
                previewToCreateCircle(m_actionData->data);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->data.center);
                    previewRefPoint(m_actionData->point1);
                    previewRefPoint(m_actionData->point2);
                    previewRefSelectablePoint(mouse);
                    previewRefLine(m_actionData->point1, m_actionData->data.center);
                    previewRefLine(m_actionData->point2, m_actionData->data.center);
                    previewRefLine(mouse, m_actionData->data.center);
                }
            }
            break;
        }
    }
}

void RS_ActionDrawCircle3P::onMouseLeftButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    RS_Vector coord = e->snapPoint;
    if (status == SetPoint2){
        coord = getSnapAngleAwarePoint(e, m_actionData->point1, coord);
    }
    fireCoordinateEvent(coord);
}

void RS_ActionDrawCircle3P::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDrawCircle3P::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            m_actionData->point2 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3: {
            m_actionData->point3 = mouse;
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
