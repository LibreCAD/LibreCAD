/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */
#include "lc_action_draw_arc_3points.h"

#include "lc_creation_arc.h"
#include "rs_arc.h"
#include "rs_document.h"

/**
 * Arc data defined so far.
 */
struct LC_ActionDrawArc3Points::ActionData {
    RS_ArcData data;
    /**
     * 1st point.
     */
    RS_Vector point1;
    /**
     * 2nd point.
     */
    RS_Vector point2;
    /**
     * 3rd point.
     */
    RS_Vector point3;
};

LC_ActionDrawArc3Points::LC_ActionDrawArc3Points(LC_ActionContext *actionContext)
    :LC_ActionDrawCircleBase("ActionDrawArc3P", actionContext,  RS2::ActionDrawArc3P)
    , m_actionData{std::make_unique<ActionData>()}{
}

LC_ActionDrawArc3Points::~LC_ActionDrawArc3Points() = default;

void LC_ActionDrawArc3Points::reset() {
}

void LC_ActionDrawArc3Points::init(const int status) {
    LC_ActionDrawCircleBase::init(status);
}

RS_Entity* LC_ActionDrawArc3Points::doTriggerCreateEntity(){
    preparePreview(m_alternatedPoints);
    if (m_actionData->data.isValid()){
        auto *arc = new RS_Arc{m_document, m_actionData->data};

        RS_Vector rz = arc->getEndpoint();
        if (m_moveRelPointAtCenterAfterTrigger){
            rz = arc->getCenter();
        }
        moveRelativeZero(rz);

        return arc;
    }
    commandMessage(tr("Invalid arc data."));
    return nullptr;
}

void LC_ActionDrawArc3Points::doTriggerCompletion([[maybe_unused]]bool success){
    m_alternatedPoints = false;
    setStatus(SetPoint1);
    reset();
}

void LC_ActionDrawArc3Points::preparePreview(const bool alternatePoints) const {
    if (m_actionData->point1.valid && m_actionData->point2.valid && m_actionData->point3.valid){
        const RS_Vector &middlePoint = m_actionData->point2;
        const RS_Vector &startPoint = m_actionData->point1;
        const RS_Vector &endPoint = m_actionData->point3;
        bool success = false;
        if (alternatePoints){
            success = LC_CreationArc::createFrom3P(startPoint, endPoint, middlePoint, m_actionData->data);
        }
        else {
            success = LC_CreationArc::createFrom3P(startPoint, middlePoint, endPoint,m_actionData->data);
        }
        if (!success){
            m_actionData->data = RS_ArcData{};
        }
    }
}

bool LC_ActionDrawArc3Points::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2) || (status == SetPoint3);
}

void LC_ActionDrawArc3Points::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;

    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = mouse;
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2: {
            mouse = getSnapAngleAwarePoint(e, m_actionData->point1, mouse, true);
            m_actionData->point2 = mouse;
            if (m_actionData->point1.valid) { // todo - redundant check
                previewLine(m_actionData->point1, m_actionData->point2);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->point1);
                    previewRefSelectablePoint(m_actionData->point2);
                }
            }
            break;
        }
        case SetPoint3: {
            // todo - which point (1 or 2) is more suitable there for snap?
            mouse = getSnapAngleAwarePoint(e,m_actionData->point1, mouse, true);
            m_actionData->point3 = mouse;
            const bool alternatePoints = e->isControl || m_alternatedPoints;
            preparePreview(alternatePoints);
            if (m_actionData->data.isValid()){
                previewToCreateArc(m_actionData->data);

                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->data.center);
                    previewRefPoint(m_actionData->point1);
                    previewRefPoint(m_actionData->point2);
                    previewRefSelectablePoint(m_actionData->point3);

                    if (alternatePoints){
                        previewRefLine(m_actionData->point1, m_actionData->point2);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawArc3Points::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetPoint2:{
            snap = getSnapAngleAwarePoint(e, m_actionData->point1, snap);
            break;
        }
        case SetPoint3:{
            snap = getSnapAngleAwarePoint(e, m_actionData->point1, snap);
            if (e->isControl){
               m_alternatedPoints = true;
            }
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snap);
}

void LC_ActionDrawArc3Points::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    setStatus(status-1);
}

void LC_ActionDrawArc3Points::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetPoint1: {
            m_actionData->point1 = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            m_actionData->point2 = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(coord);
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3: {
            m_actionData->point3 = coord;
            addSnappedPointToVisualSnap(coord);
            trigger();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawArc3Points::doProcessCommand([[maybe_unused]]int status, const QString &command) {
    bool accept = false;
    if (checkCommand("center", command, rtti())) {
        accept = true;
        finish();
        switchToAction(RS2::ActionDrawArc);
    }
    // fixme - sand - add these to commands
    else if (checkCommand("altpoint", command, rtti())){
        accept = true;
        m_alternatedPoints = true;
    }
    else if (checkCommand("normpoint", command, rtti())){
        accept = true;
        m_alternatedPoints = false;
    }
    return accept;
}

QStringList LC_ActionDrawArc3Points::getAvailableCommands() {
    return {{"center", "altpoint", "normpoint"}};
}

void LC_ActionDrawArc3Points::updateActionPrompt() {
    switch (getStatus()) {
    case SetPoint1:
        updatePromptTRCancel(tr("Specify startpoint or [center]"), MOD_SHIFT_RELATIVE_ZERO);
        break;
    case SetPoint2:
        updatePromptTRBack(tr("Specify second point"), MOD_SHIFT_ANGLE_SNAP);
        break;
    case SetPoint3:
        updatePromptTRBack(tr("Specify third point"), MOD_SHIFT_AND_CTRL_ANGLE("Second point was endpoint"));
        break;
    default:
        updatePrompt();
        break;
    }
}
