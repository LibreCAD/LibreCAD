/****************************************************************************
*
This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2014-2015 Dongxu Li (dongxuli2011 at gmail.com)

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
**********************************************************************/

#include "lc_actiondrawcircle2pr.h"

#include "rs_circle.h"
#include "rs_preview.h"

struct LC_ActionDrawCircle2PR::Points{
	RS_Vector point1;
	RS_Vector point2;
};

LC_ActionDrawCircle2PR::LC_ActionDrawCircle2PR(LC_ActionContext *actionContext)
    :RS_ActionDrawCircleCR(actionContext)
    , m_actionData(std::make_unique<Points>()){
    m_actionType=RS2::ActionDrawCircle2PR;
    reset();
}

LC_ActionDrawCircle2PR::~LC_ActionDrawCircle2PR() = default;

void LC_ActionDrawCircle2PR::reset(){
    deletePreview();
    double radius = m_circleData->radius;
    *m_circleData = {};
    m_circleData->radius = radius;
    m_actionData->point1 = {};
    m_actionData->point2 = {};
}

void LC_ActionDrawCircle2PR::init(int status){
    RS_ActionDrawCircleCR::init(status);
    if (status <= 0){
        reset();
    }
}

void LC_ActionDrawCircle2PR::doTrigger() {
    auto *circle = new RS_Circle(m_container, *m_circleData);
    setPenAndLayerToActive(circle);

    if (m_moveRelPointAtCenterAfterTrigger){
        moveRelativeZero(circle->getCenter());
    }
    undoCycleAdd(circle);
    setStatus(SetPoint1);
    reset();
}

bool LC_ActionDrawCircle2PR::preparePreview(const RS_Vector &mouse, RS_Vector& altCenter){
    const RS_Vector vp = (m_actionData->point1 + m_actionData->point2) * 0.5;
    double const angle = m_actionData->point1.angleTo(m_actionData->point2) + 0.5 * M_PI;
    double const &r0 = m_circleData->radius;
    double const r = sqrt(r0 * r0 - 0.25 * m_actionData->point1.squaredTo(m_actionData->point2));
    const RS_Vector dvp = RS_Vector(angle) * r;
    const RS_Vector &center1 = vp + dvp;
    const RS_Vector &center2 = vp - dvp;

    if (center1.squaredTo(center2) < RS_TOLERANCE){
//no need to select center, as only one solution possible
        m_circleData->center = vp;
        return false;
    }

    const double ds = mouse.squaredTo(center1) - mouse.squaredTo(center2);

    if (ds < 0.){
        m_circleData->center = center1;
        altCenter = center2;
        return true;
    }
    if (ds > 0.){
        m_circleData->center = center2;
        altCenter = center1;
        return true;
    }
    m_circleData->center.valid = false;
    return false;
}


void LC_ActionDrawCircle2PR::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint1:
            m_actionData->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;

        case SetPoint2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->point1, mouse, true);
            if (mouse.distanceTo(m_actionData->point1) <= 2. * m_circleData->radius) {
                m_actionData->point2 = mouse;
            }

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->point1);
                previewRefSelectablePoint(m_actionData->point2);
                previewRefLine(m_actionData->point1, m_actionData->point2);
            }

            RS_Vector altCenter;
            if (preparePreview(mouse, altCenter)){
                if (m_circleData->center.valid){
                    previewCircle(*m_circleData);
                    previewRefSelectablePoint(m_circleData->center);
                    previewRefSelectablePoint(altCenter);
                }
            }
            break;
        }
        case SelectCenter: {
            deleteSnapper();
            RS_Vector altCenter;
            if (preparePreview(mouse, altCenter)){
                // todo - review, what for we're checking for circle there?
                bool existing = false;
                for (auto p: *m_preview) {
                    if (isCircle(p)){
                        if (dynamic_cast<RS_Circle *>(p)->getData() == *m_circleData)
                            existing = true;
                    }
                }
                if (!existing){
                    previewToCreateCircle(*m_circleData);
                    previewRefSelectablePoint(m_circleData->center);
                    previewRefSelectablePoint(altCenter);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefPoint(m_actionData->point1);
                        previewRefPoint(m_actionData->point2);
                        previewRefLine(m_actionData->point1, m_actionData->point2);
                    }
                }
            } else {
                if (m_circleData->isValid()){
                    trigger();
                }
            }
        }
    }
}

void LC_ActionDrawCircle2PR::onMouseLeftButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    RS_Vector coord = e->snapPoint;
    if (status == SetPoint2){
        coord = getSnapAngleAwarePoint(e, m_actionData->point1, coord);
    }
    else if (status == SelectCenter){
        invalidateSnapSpot();
    }
    fireCoordinateEvent(coord);
}

void LC_ActionDrawCircle2PR::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawCircle2PR::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = mouse;
            moveRelativeZero(mouse);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            double distance = mouse.distanceTo(m_actionData->point1);
            if (distance <= 2. * m_circleData->radius){
                m_actionData->point2 = mouse;
                moveRelativeZero(mouse);
                setStatus(SelectCenter);
            } else {
                commandMessage(tr("radius=%1 is too small for points selected\ndistance between points=%2 is larger than diameter=%3").
                    arg(m_circleData->radius).arg(distance).arg(2. * m_circleData->radius));
            }
            break;
        }
        case SelectCenter: {
            RS_Vector altCenter;
            bool showPreview = preparePreview(mouse, altCenter);
            if (showPreview || m_circleData->isValid())
                trigger();
            else
                commandMessage(tr("Select from two possible circle centers"));
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawCircle2PR::doProcessCommand([[maybe_unused]]int status, [[maybe_unused]] const QString& c){
    // fixme - support commands
    return false;
}

QStringList LC_ActionDrawCircle2PR::getAvailableCommands(){
    QStringList cmd;
    return cmd;
}

void LC_ActionDrawCircle2PR::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SelectCenter:
            updateMouseWidgetTRBack(tr("Select circle center"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
