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

#include "lc_action_draw_circle_2points_radius.h"

#include "lc_creation_circle.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_preview.h"

struct LC_ActionDrawCircle2PointsRadius::Points {
    RS_Vector point1;
    RS_Vector point2;
};

LC_ActionDrawCircle2PointsRadius::LC_ActionDrawCircle2PointsRadius(LC_ActionContext* actionContext)
    : LC_ActionDrawCircleCenterRadius("ActionDrawCircle2PointsRadius", actionContext, RS2::ActionDrawCircle2PointsRadius),
      m_actionData(std::make_unique<Points>()) {
    LC_ActionDrawCircle2PointsRadius::reset();
}

LC_ActionDrawCircle2PointsRadius::~LC_ActionDrawCircle2PointsRadius() = default;

void LC_ActionDrawCircle2PointsRadius::reset() {
    deletePreview();
    m_actionData->point1 = {};
    m_actionData->point2 = {};
}

void LC_ActionDrawCircle2PointsRadius::init(const int status) {
    LC_ActionDrawCircleCenterRadius::init(status);
    if (status <= 0) {
        reset();
    }
}

RS_Entity* LC_ActionDrawCircle2PointsRadius::doTriggerCreateEntity() {
    auto* circle = new RS_Circle(m_document, RS_CircleData(m_center, m_radius));
    if (m_moveRelPointAtCenterAfterTrigger) {
        moveRelativeZero(circle->getCenter());
    }
    return circle;
}

bool LC_ActionDrawCircle2PointsRadius::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2);
}

void LC_ActionDrawCircle2PointsRadius::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetPoint1);
    reset();
}

bool LC_ActionDrawCircle2PointsRadius::preparePreview(const RS_Vector& mouse, RS_Vector& altCenter) {
    RS_CircleData circleData(m_center, m_radius);
    const bool result = LC_CreationCircle::create2PRadius(m_actionData->point1, m_actionData->point2, m_radius, altCenter, circleData);
    if (result) {
        m_center = circleData.center;
        const double ds = mouse.squaredTo(m_center) - mouse.squaredTo(altCenter);
        if (ds > 0.) {
            const RS_Vector center = m_center;
            m_center = altCenter;
            altCenter = center;
        }
    }
    return result;
}

void LC_ActionDrawCircle2PointsRadius::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint1:
            m_actionData->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;

        case SetPoint2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->point1, mouse, true);
            if (mouse.distanceTo(m_actionData->point1) <= 2. * m_radius) {
                m_actionData->point2 = mouse;
            }

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_actionData->point1);
                previewRefSelectablePoint(m_actionData->point2);
                previewRefLine(m_actionData->point1, m_actionData->point2);
            }

            RS_Vector altCenter;
            if (preparePreview(mouse, altCenter)) {
                if (m_center.valid) {
                    previewCircle(RS_CircleData(m_center, m_radius));
                    previewRefSelectablePoint(m_center);
                    previewRefSelectablePoint(altCenter);
                }
            }
            break;
        }
        case SelectCenter: {
            deleteSnapper();
            RS_Vector altCenter;
            if (preparePreview(mouse, altCenter)) {
                previewToCreateCircle(RS_CircleData(m_center, m_radius));
                previewRefSelectablePoint(m_center);
                previewRefSelectablePoint(altCenter);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->point1);
                    previewRefPoint(m_actionData->point2);
                    previewRefLine(m_actionData->point1, m_actionData->point2);
                }
            }
            else {
                const bool dataValid = m_center.valid && m_radius > RS_TOLERANCE;
                if (dataValid) {
                    trigger();
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawCircle2PointsRadius::onMouseLeftButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    RS_Vector coord = e->snapPoint;
    if (status == SetPoint2) {
        coord = getSnapAngleAwarePoint(e, m_actionData->point1, coord);
    }
    else if (status == SelectCenter) {
        invalidateSnapSpot();
    }
    fireCoordinateEvent(coord);
}

void LC_ActionDrawCircle2PointsRadius::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawCircle2PointsRadius::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            const double distance = coord.distanceTo(m_actionData->point1);
            if (distance <= 2. * m_radius) {
                m_actionData->point2 = coord;
                addSnappedPointToVisualSnap(coord);
                moveRelativeZero(coord);
                setStatus(SelectCenter);
            }
            else {
                commandMessage(
                    tr("radius=%1 is too small for points selected\ndistance between points=%2 is larger than diameter=%3").arg(m_radius).
                    arg(distance).arg(2. * m_radius));
            }
            break;
        }
        case SelectCenter: {
            RS_Vector altCenter;
            const bool showPreview = preparePreview(coord, altCenter);
            bool circleValid = m_center.valid && m_radius < RS_TOLERANCE;
            if (showPreview || circleValid) {
                trigger();
            }
            else {
                commandMessage(tr("Select from two possible circle centers"));
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawCircle2PointsRadius::doProcessCommand([[maybe_unused]] int status, [[maybe_unused]] const QString& c) {
    // fixme - support commands
    return false;
}

QStringList LC_ActionDrawCircle2PointsRadius::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}

void LC_ActionDrawCircle2PointsRadius::updateActionPrompt() {
    switch (getStatus()) {
        case SetPoint1:
            updatePromptTRCancel(tr("Specify first point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updatePromptTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SelectCenter:
            updatePromptTRBack(tr("Select circle center"));
            break;
        default:
            updatePrompt();
            break;
    }
}
