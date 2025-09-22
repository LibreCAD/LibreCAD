/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "lc_actioninteractivepickdistance.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_division.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_line.h"

namespace {
    const EntityTypeList g_enTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle, RS2::EntityPolyline};
}

LC_ActionInteractivePickDistance::LC_ActionInteractivePickDistance(LC_ActionContext* actionContext):
 LC_ActionInteractivePickBase("PickDistance", actionContext, RS2::ActionInteractivePickLength){
}

LC_ActionInteractivePickDistance::~LC_ActionInteractivePickDistance() = default;

bool LC_ActionInteractivePickDistance::isInteractiveDataValid() {
    return (LC_LineMath::isMeaningful(m_distance) && !std::signbit(m_distance)) || (m_point1.valid && m_point2.valid);
}

void LC_ActionInteractivePickDistance::doSetInteractiveInputValue(
    LC_ActionContext::InteractiveInputInfo* interactiveInputInfo) {
    interactiveInputInfo->m_distance = m_distance;
}

void LC_ActionInteractivePickDistance::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetPoint1: {
            RS_Entity* entity = catchAndDescribe(e, g_enTypeList, RS2::ResolveAll);
            if (entity != nullptr) {
                if (e->isShift) {
                    highlightHover(entity);
                    if (isLine(entity)) {
                        auto startPoint = entity->getStartpoint();
                        auto endPoint = entity->getEndpoint();
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(startPoint);
                            previewRefPoint(endPoint);
                            previewRefLine(startPoint, endPoint);
                        }
                        updateInfoCursorForPoint2(startPoint, endPoint);
                    }
                    if (isArc(entity)) {
                        auto arc = static_cast<RS_Arc*>(entity);
                        auto center = arc->getCenter();
                        auto pointOnEntity = arc->getNearestPointOnEntity(e->graphPoint, true);
                        auto startPoint = center;
                        auto endPoint = pointOnEntity;
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(center);
                            previewRefPoint(pointOnEntity);
                            previewRefLine(center, pointOnEntity);
                        }
                        if (e->isControl) {
                            auto otherPoint = pointOnEntity;
                            otherPoint.mirror(center, center.relative(10, center.angleTo(pointOnEntity) + M_PI_2));
                            if (m_showRefEntitiesOnPreview) {
                                previewRefPoint(otherPoint);
                                previewRefLine(center, otherPoint);
                            }
                            startPoint = otherPoint;
                        }
                        updateInfoCursorForPoint2(startPoint, endPoint);
                    }
                    else if (isCircle(entity)) {
                        auto circle = static_cast<RS_Arc*>(entity);
                        auto center = circle->getCenter();
                        auto pointOnEntity = circle->getNearestPointOnEntity(e->graphPoint, true);
                        auto startPoint = center;
                        auto endPoint = pointOnEntity;
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(center);
                            previewRefPoint(pointOnEntity);
                            previewRefLine(center, pointOnEntity);
                        }
                        if (e->isControl) {
                            auto otherPoint = pointOnEntity;
                            otherPoint.mirror(center, center.relative(10, center.angleTo(pointOnEntity) + M_PI_2));
                            if (m_showRefEntitiesOnPreview) {
                                previewRefPoint(otherPoint);
                                previewRefLine(center, otherPoint);
                            }
                            startPoint = otherPoint;
                        }
                        updateInfoCursorForPoint2(startPoint, endPoint);
                    }
                }
                else if (e->isControl) {
                    if (isLine(entity)) {
                        highlightHover(entity);
                        auto line = static_cast<RS_Line*>(entity);
                        LC_Division division(m_container);
                        LC_Division::LineSegmentData* data = division.findLineSegmentBetweenIntersections(
                            line, e->graphPoint, true);

                        if (data != nullptr) {
                            RS_Vector start = data->snapSegmentStart;
                            RS_Vector end = data->snapSegmentEnd;
                            if (m_showRefEntitiesOnPreview) {
                                previewRefLine(start, end);
                                previewRefPoint(start);
                                previewRefPoint(end);
                            }
                            updateInfoCursorForPoint2(start, end);
                        }
                        delete data;
                    }
                }
            }
            else {
                // if (!trySnapToRelZeroCoordinateEvent(e)) {
                    updateInfoCursorForPoint1(mouse);
                // }
            }
            break;
        }
        case SetPoint2: {
            if (m_point1.valid){
                mouse = getSnapAngleAwarePoint(e, m_point1, mouse, true);
                m_point2 = mouse;
                previewLine(m_point1, m_point2);
                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(m_point1, m_point2);
                    previewRefPoint(m_point1);
                    previewRefSelectablePoint(m_point2);
                }
                RS_Vector &startPoint = m_point1;
                updateInfoCursorForPoint2(mouse, startPoint);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickDistance::updateInfoCursorForPoint1(const RS_Vector &mouse) {
    if (m_infoCursorOverlayPrefs->enabled) {
        msg(tr("Pick Distance"))
            .vector(tr("Absolute:"), mouse)
            .polar(tr("Polar:"), mouse)
            .relative(tr("Relative:"), mouse)
            .relativePolar(tr("Relative Polar:"), mouse)
            .toInfoCursorZone2(false);
    }
}

void LC_ActionInteractivePickDistance::updateInfoCursorForPoint2(const RS_Vector &mouse, const RS_Vector &startPoint) {
    if (m_infoCursorOverlayPrefs->enabled) {
        double distance = startPoint.distanceTo(mouse);
        msg(tr("Pick Distance"))
            .linear(tr("Distance:"), distance)
            .wcsAngle(tr("Angle:"), startPoint.angleTo(mouse))
            .vector(tr("From:"), startPoint)
            .vector(tr("To:"), mouse)
            .toInfoCursorZone2(false);
    }
}

void LC_ActionInteractivePickDistance::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetPoint1:{
            RS_Entity* entity = catchEntity(e->graphPoint, g_enTypeList, RS2::ResolveAll);
            if (entity != nullptr) {
                if (e->isShift) {
                    if (isLine(entity)) {
                        m_distance = entity->getLength();
                    }
                    else if (isCircle(entity)) {
                        auto circle = static_cast<RS_Circle*>(entity);
                        m_distance = circle->getRadius();
                        if (e->isControl) {
                            m_distance = m_distance*2; // diameter
                        }
                    }
                    else if (isArc(entity)) {
                        auto arc = static_cast<RS_Arc*>(entity);
                        m_distance = arc->getRadius();
                        if (e->isControl) {
                            m_distance = m_distance*2; // diameter
                        }
                    }
                }
                else if (e->isControl) {
                    if (isLine(entity)) {
                        auto line = static_cast<RS_Line*>(entity);
                        LC_Division division(m_container);
                        LC_Division::LineSegmentData *data = division.findLineSegmentBetweenIntersections(line, snap, true);
                        if (data != nullptr) {
                            m_distance = data->snapSegmentEnd.distanceTo(data->snapSegmentStart);
                            delete data;
                        }
                    }
                }
            }
            if (LC_LineMath::isMeaningful(m_distance) && !std::signbit(m_distance)) {
                trigger();
            }
            else {
                fireCoordinateEvent(snap);
                moveRelativeZero(m_point1);
            }
            break;
        }
        case (SetPoint2):{
            snap = getSnapAngleAwarePoint(e, m_point1,  snap);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickDistance::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    if (status == SetPoint2) {
        moveRelativeZero(m_savedRelativeZero);
    }
    initPrevious(status);
}


void LC_ActionInteractivePickDistance::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            m_point1 = mouse;
            m_savedRelativeZero = m_viewport->getRelativeZero();
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            if (m_point1.valid){
                if (LC_LineMath::isMeaningfulDistance(m_point1, mouse)) {
                    m_point2 = mouse;
                    m_distance = m_point2.distanceTo(m_point1);
                    moveRelativeZero(m_savedRelativeZero);
                    deletePreview();
                    trigger();
                }
                else {
                    commandMessage(tr("Start and end points are too close to each other to calculate the distance."));
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickDistance::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify first point of distance"), MOD_SHIFT_AND_CTRL(tr("Entity geometry length"), tr("Segment length/Diameter")));
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify second point of distance"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType LC_ActionInteractivePickDistance::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}
