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
#include "lc_action_draw_arc_tangential.h"

#include "lc_actioncontext.h"
#include "lc_arc_tangential_options_filler.h"
#include "lc_arc_tangential_options_widget.h"
#include "lc_creation_arc.h"
#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_document.h"

LC_ActionDrawArcTangential::LC_ActionDrawArcTangential(LC_ActionContext* actionContext)
    : LC_SingleEntityCreationAction("ActionDrawArcTangential", actionContext, RS2::ActionDrawArcTangential), m_point(false),
      m_arcData(std::make_unique<RS_ArcData>()) {
}

LC_ActionDrawArcTangential::~LC_ActionDrawArcTangential() = default;

void LC_ActionDrawArcTangential::reset() {
    m_baseEntity = nullptr;
    m_isStartPoint = false;
    m_point = RS_Vector(false);
    m_arcData = std::make_unique<RS_ArcData>();
    m_alternateArc = false;
}

void LC_ActionDrawArcTangential::doSaveOptions() {
    save("ByRadius", m_byRadius);
    save("Radius", m_radius);
    save("Angle", m_angleLength);
    if (m_byRadius) {
        setRadius(m_radius);
    }
}

void LC_ActionDrawArcTangential::doLoadOptions() {
    m_byRadius = loadBool("ByRadius", true);
    m_radius = loadDouble("Radius", 1.0);
    m_angleLength = loadDouble("Angle", M_PI_2);
}

bool LC_ActionDrawArcTangential::isInVisualSnapStatus(int status) {
    return status == SetEndAngle;
}

void LC_ActionDrawArcTangential::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPosition) {
    setBaseEntity(contextEntity, m_actionContext->getContextMenuActionClickPosition());
}

RS_Entity* LC_ActionDrawArcTangential::doTriggerCreateEntity() {
    if (!(m_point.valid && (m_baseEntity != nullptr))) {
        RS_DEBUG->print("RS_ActionDrawArcTangential::trigger: conditions not met");
        return nullptr;
    }

    preparePreview();
    if (m_alternateArc) {
        m_arcData->reversed = !m_arcData->reversed;
    }
    auto* arc = new RS_Arc(m_document, *m_arcData);
    moveRelativeZero(arc->getCenter());
    return arc;
}

void LC_ActionDrawArcTangential::doTriggerCompletion([[maybe_unused]] bool success) {
    setStatus(SetBaseEntity);
    const double oldRadius = m_arcData->radius;

    reset();
    if (m_byRadius) {
        m_arcData->radius = oldRadius;
    }
}

bool LC_ActionDrawArcTangential::doUpdateAngleByInteractiveInput(const QString& tag, const double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}

bool LC_ActionDrawArcTangential::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

void LC_ActionDrawArcTangential::preparePreview() {
    if ((m_baseEntity != nullptr) && m_point.valid) {
        RS_Vector startPoint;
        double direction;
        if (m_isStartPoint) {
            startPoint = m_baseEntity->getStartpoint();
            direction = RS_Math::correctAngle(m_baseEntity->getDirection1() + M_PI);
        }
        else {
            startPoint = m_baseEntity->getEndpoint();
            direction = RS_Math::correctAngle(m_baseEntity->getDirection2() + M_PI);
        }

        bool success;
        if (m_byRadius) {
            success = LC_CreationArc::createFrom2PDirectionRadius(startPoint, m_point, direction, m_radius, *m_arcData);
        }
        else {
            success = LC_CreationArc::createFrom2PDirectionAngle(startPoint, m_point, direction, m_angleLength, *m_arcData);
        }
        if (!success) {
            m_arcData.reset(new RS_ArcData());
        }
    }
}

void LC_ActionDrawArcTangential::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    m_point = e->snapPoint;
    switch (status) {
        case SetBaseEntity: {
            deleteSnapper();
            const auto entity = catchAndDescribe(e, RS2::ResolveAll);
            if (isAtomic(entity)) {
                highlightHover(entity);
            }
            break;
        }
        case SetEndAngle: {
            highlightSelected(m_baseEntity);
            if (m_byRadius) {
                if (e->isShift) {
                    // double check for efficiency, eliminate center forecasting calculations if not needed
                    const auto center = forecastArcCenter();
                    m_point = getSnapAngleAwarePoint(e, center, m_point, true);
                }
            }
            preparePreview();
            if (m_arcData->isValid()) {
                RS_Arc* arc;
                const bool alternateArcMode = e->isControl;
                if (alternateArcMode) {
                    auto tmpArcData = *m_arcData;
                    tmpArcData.reversed = !m_arcData->reversed;
                    arc = previewToCreateArc(tmpArcData);
                }
                else {
                    arc = previewToCreateArc(*m_arcData);
                }
                if (m_showRefEntitiesOnPreview) {
                    const auto& center = m_arcData->center;
                    const auto& startPoint = arc->getStartpoint();
                    previewRefPoint(center);
                    previewRefPoint(startPoint);
                    if (m_byRadius) {
                        previewRefLine(center, m_point);
                        previewRefSelectablePoint(arc->getEndpoint());
                        previewRefSelectablePoint(center);
                    }
                    else {
                        previewRefLine(center, arc->getEndpoint());
                        previewRefLine(center, startPoint);
                        const auto nearest = arc->getNearestPointOnEntity(m_point, false);
                        previewRefLine(center, m_point);
                        previewRefSelectablePoint(nearest);
                        auto circleArcData = arc->getData();
                        std::swap(circleArcData.angle1, circleArcData.angle2);
                        previewRefArc(circleArcData);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

RS_Vector LC_ActionDrawArcTangential::forecastArcCenter() const {
    RS_Vector center;
    double direction;
    if (m_isStartPoint) {
        direction = RS_Math::correctAngle(m_baseEntity->getDirection1() + M_PI);
    }
    else {
        direction = RS_Math::correctAngle(m_baseEntity->getDirection2() + M_PI);
    }

    const RS_Vector ortho = RS_Vector::polar(m_arcData->radius, direction + M_PI_2);
    const RS_Vector center1 = m_arcStartPoint + ortho;
    const RS_Vector center2 = m_arcStartPoint - ortho;
    if (center1.distanceTo(m_point) < center2.distanceTo(m_point)) {
        center = center1;
    }
    else {
        center = center2;
    }
    return center;
}

void LC_ActionDrawArcTangential::setBaseEntity(RS_Entity* entity, const RS_Vector& coord) {
    if (isAtomic(entity)) {
        m_baseEntity = static_cast<RS_AtomicEntity*>(entity);
        const RS_Vector& startPoint = m_baseEntity->getStartpoint();
        const RS_Vector& endPoint = m_baseEntity->getEndpoint();
        if (startPoint.distanceTo(coord) < endPoint.distanceTo(coord)) {
            m_isStartPoint = true;
            m_arcStartPoint = startPoint;
        }
        else {
            m_isStartPoint = false;
            m_arcStartPoint = endPoint;
        }
        setStatus(SetEndAngle);
        updateActionPrompt();
    }
}

void LC_ActionDrawArcTangential::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        // set base entity:
        case SetBaseEntity: {
            const RS_Vector coord = e->graphPoint;
            RS_Entity* entity = catchEntity(coord, RS2::ResolveAll);
            setBaseEntity(entity, coord);
            invalidateSnapSpot();
            break;
        }
        case SetEndAngle: {
            // set angle (point that defines the angle)
            if (m_byRadius) {
                if (e->isShift) {
                    // double check for efficiency, eliminate calculations if not needed
                    const RS_Vector center = forecastArcCenter();
                    m_point = getSnapAngleAwarePoint(e, center, m_point);
                }
            }
            else {
                m_point = getSnapAngleAwarePoint(e, m_arcStartPoint, m_point);
            }
            m_alternateArc = e->isControl;
            fireCoordinateEvent(m_point);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawArcTangential::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

// fixme - more intelligent processing
void LC_ActionDrawArcTangential::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetBaseEntity:
            break;
        case SetEndAngle:
            m_point = coord;
            trigger();
            break;
        default:
            break;
    }
}

void LC_ActionDrawArcTangential::updateActionPrompt() {
    switch (getStatus()) {
        case SetBaseEntity:
            updatePromptTRCancel(tr("Specify base entity"));
            break;
        case SetEndAngle:
            if (m_byRadius) {
                updatePromptTRBack(tr("Specify end angle"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Alternate arc")));
            }
            else {
                updatePromptTRBack(tr("Specify end point"), MOD_CTRL(tr("Alternate Arc")));
            }
            break;
        default:
            updatePrompt();
            break;
    }
}

// fixme - add suport of commands?

RS2::CursorType LC_ActionDrawArcTangential::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

void LC_ActionDrawArcTangential::setRadius(const double r) {
    m_radius = std::abs(r);
}

double LC_ActionDrawArcTangential::getRadius() const {
    return m_radius;
}

bool LC_ActionDrawArcTangential::isByRadius() const {
    return m_byRadius;
}

void LC_ActionDrawArcTangential::setByRadius(const bool byRadius) {
    m_byRadius = byRadius;
}

double LC_ActionDrawArcTangential::getAngle() const {
    return m_angleLength;
}

void LC_ActionDrawArcTangential::setAngle(const double angle) {
    m_angleLength = angle;
}

LC_ActionOptionsWidget* LC_ActionDrawArcTangential::createOptionsWidget() {
    return new LC_ArcTangentialOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawArcTangential::createOptionsFiller() {
    return new LC_ArcTangentialOptionsFiller();
}
