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

#include "lc_actioninteractivepickangle.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_information.h"
#include "rs_line.h"

LC_ActionInteractivePickAngle::LC_ActionInteractivePickAngle(LC_ActionContext* actionContext)
    :LC_ActionInteractivePickBase("PickAngle", actionContext, RS2::ActionInteractivePickAngle){
}

LC_ActionInteractivePickAngle::~LC_ActionInteractivePickAngle() {
}

void LC_ActionInteractivePickAngle::init(int status) {
    LC_ActionInteractivePickBase::init(status);
}

bool LC_ActionInteractivePickAngle::isInteractiveDataValid() {
   return m_mayTrigger;
}

void LC_ActionInteractivePickAngle::doSetInteractiveInputValue(
    LC_ActionContext::InteractiveInputInfo* interactiveInputInfo) {
    interactiveInputInfo->m_angleRad = m_angle;
}

RS2::CursorType LC_ActionInteractivePickAngle::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionInteractivePickAngle::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent* e) {
    if (status == SetSecondLine) {
        setStatus(SetPoint1);
    }
    else {
        setStatus(getStatus() - 1);
    }
}

void LC_ActionInteractivePickAngle::onMouseMoveEvent(int status, LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetPoint1:{
            auto ent = catchAndDescribe(e, RS2::EntityLine, RS2::ResolveAll);
            if (ent != nullptr) {
                if (e->isControl) {
                    highlightHover(ent);
                    auto point = ent->getNearestPointOnEntity(mouse, true);
                    if (m_showRefEntitiesOnPreview) {
                        RS_Vector endpointToUse = ent->getEndpoint();
                        previewSnapAngleMark(point, endpointToUse);
                    }
                }
                else if (e->isShift) {
                    highlightHover(ent);
                }
            }
            break;
        }
        case SetPoint2:{
            mouse = getSnapAngleAwarePoint(e, m_point1, mouse, true, e->isShift);
            previewRefPoint(m_point1);
            updateInfoCursor(mouse, m_point1);
            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(mouse);
                previewRefLine(m_point1, mouse);
            }
            break;
        }
        case SetPoint3: {
            mouse = getSnapAngleAwarePoint(e, m_point2, mouse, true, e->isShift);
            previewRefPoint(m_point1);
            previewRefPoint(m_point2);
            previewRefSelectablePoint(mouse);
            updateInfoCursor(mouse, m_point2, m_point1);
            if (m_showRefEntitiesOnPreview) {
                previewRefLine(m_point1, m_point2);
                previewRefLine(m_point2, mouse);

                double distance1 = m_point2.distanceTo(m_point1);
                double distance2 = m_point2.distanceTo(mouse);
                if (distance2 < distance1) {
                    previewRefArc(m_point2, mouse, m_point1, true);
                }
                else {
                    previewRefArc(m_point2, m_point1, mouse, true);
                }
            }
            break;
        }
        case SetSecondLine: {
            auto en = catchAndDescribe(e, RS2::ResolveAll);
            highlightSelected(m_entity1);
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_point1);
            }
            if (isLine(en)){ // fixme - support of polyline
                RS_VectorSolutions const &sol = RS_Information::getIntersection(m_entity1, en, false);
                if (sol.hasValid()){
                    highlightHover(en);
                    if (m_showRefEntitiesOnPreview) {
                        RS_Vector p2 = en->getNearestPointOnEntity(mouse);
                        previewRefSelectablePoint(p2);
                        RS_Vector intersection = sol.get(0);
                        updateInfoCursor2(p2,intersection);
                        previewRefArc(intersection, m_point1, p2, true);
                        previewRefPoint(intersection);
                        previewRefLine(intersection, m_point1);
                        previewRefLine(intersection, p2);
                    }
                }
                else{
                    if (m_infoCursorOverlayPrefs->enabled){
                        appendInfoCursorZoneMessage(tr("Lines are parallel"), 2, false);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickAngle::updateInfoCursor2(const RS_Vector &point2, const RS_Vector &intersection) {
    if (m_infoCursorOverlayPrefs->enabled){
        double angle1 = intersection.angleTo(m_point1);
        double angle2 = intersection.angleTo(point2);
        double angle = LC_LineMath::angleFor3Points(m_point1, intersection, point2);

        double angleComplementary, angleSupplementary, angleAlt;
        RS_Math::calculateAngles(angle, angleComplementary, angleSupplementary, angleAlt);

        msgStart().string(tr("Angle Info"))
        .rawAngle(tr("Angle:"), angle)
        .rawAngle(tr("Complementary:"), angleComplementary)
        .rawAngle(tr("Supplementary:"), angleSupplementary)
        .rawAngle(tr("Alternative: "), angleAlt)
        .vector(tr("Intersection:"), intersection)
               .wcsAngle(tr("Line 1 Angle:"), angle1)
               .wcsAngle(tr("Line 2 Angle:"), angle2)
               .toInfoCursorZone2(true);
    }
}

void LC_ActionInteractivePickAngle::onMouseLeftButtonRelease(int status, LC_MouseEvent* e) {
    RS_Vector snapped = e->snapPoint;
    switch (status){
        case SetPoint1:{
            if (e->isControl) {
                auto ent = catchAndDescribe(e, RS2::EntityLine, RS2::ResolveAll);
                if (ent != nullptr) {
                    auto line = dynamic_cast<RS_Line*>(ent);
                    double wcsLineAngle = line->getAngle1();
                    double ucsAngle = toUCSBasisAngle(wcsLineAngle);
                    // m_angle = RS_Math::correctAngle0ToPi(ucsAngle);
                    m_angle = ucsAngle;

                    if (e->isShift) {
                        double angleComplementary, angleSupplementary, angleAlt;
                        RS_Math::calculateAngles(m_angle, angleComplementary, angleSupplementary, angleAlt);
                        m_angle = angleSupplementary;
                    }

                    m_mayTrigger = true;
                    trigger();
                }
            }
            else if (e->isShift) {
                auto ent = catchAndDescribe(e, RS2::EntityLine, RS2::ResolveAll);
                if (ent != nullptr) {
                    m_entity1 = ent;
                    m_point1 = ent->getNearestPointOnEntity(snapped, true);
                    setStatus(SetSecondLine);
                }
            }
            else {
                fireCoordinateEvent(snapped);
            }
            break;
        }
        case SetPoint2: {
            snapped = getSnapAngleAwarePoint(e, m_point1, snapped, false,e->isShift);
            if (e->isControl) {
                double wcsPointAngle = m_point1.angleTo(snapped);
                double ucsAngle = toUCSBasisAngle(wcsPointAngle);
                m_angle = RS_Math::correctAngle0ToPi(ucsAngle);
                m_mayTrigger = true;
                trigger();
            }
            else {
                fireCoordinateEvent(snapped);
            }
            break;
        }
        case SetPoint3:{
            snapped = getSnapAngleAwarePoint(e, m_point2, snapped, false, e->isShift);
            m_pickAlternative = e->isControl;
            fireCoordinateEvent(snapped);
            break;
        }
        case SetSecondLine: {
            auto ent = catchAndDescribe(e, RS2::EntityLine, RS2::ResolveAll);
            if (ent != nullptr) {
                RS_VectorSolutions const &sol = RS_Information::getIntersection(m_entity1, ent, false);
                if (sol.hasValid()) {
                    auto point2 = ent->getNearestPointOnEntity(snapped, true);
                    auto intersection = sol.get(0);
                    double angle = LC_LineMath::angleFor3Points(m_point1, intersection, point2);
                    m_angle = RS_Math::correctAngle0ToPi(angle);
                    double angleComplementary, angleSupplementary, angleAlt;
                    RS_Math::calculateAngles(m_angle, angleComplementary, angleSupplementary, angleAlt);
                    if (e->isShift) {
                        m_angle = angleComplementary;
                    }
                    else if (e->isControl) {
                        m_angle = angleSupplementary;
                    }
                    m_mayTrigger = true;
                    trigger();
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickAngle::onCoordinateEvent(int status,[[maybe_unused]] bool isZero, const RS_Vector& pos) {
    switch (status){
        case SetPoint1:{
            m_point1 = pos;
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2:{
            m_point2 = pos;
            setStatus(SetPoint3);
            break;
        }
        case SetPoint3:{
            m_point3 = pos;
            m_mayTrigger = true;
            m_angle = LC_LineMath::angleFor3Points(m_point1, m_point2, m_point3);
            if (m_pickAlternative) {
                double angleComplementary, angleSupplementary, angleAlt;
                RS_Math::calculateAngles(m_angle, angleComplementary, angleSupplementary, angleAlt);
                m_angle = angleSupplementary;
            }
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionInteractivePickAngle::updateMouseButtonHints() {
    int status = getStatus();
    switch (status){
        case SetPoint1:{
            updateMouseWidgetTRCancel(tr("Select first edge point of angle"), MOD_SHIFT_AND_CTRL(tr("Select first line/Pick Supplementary"), tr("Pick from line")));
            break;
        }
        case SetPoint2:{
            updateMouseWidgetTRCancel(tr("Select second (intersection) point of angle"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Pick angle")));
            break;
        }
        case SetPoint3:{
            updateMouseWidgetTRCancel(tr("Select second edge point of angle"), MOD_SHIFT_AND_CTRL(MSG_ANGLE_SNAP, tr("Pick Supplementary")));
            break;
        }
        case SetSecondLine: {
            updateMouseWidgetTRBack(tr("Specify second line"), MOD_SHIFT_AND_CTRL(tr("Pick Complementary"), tr("Pick Supplementary")));
            break;
        }
        default:
            updateMouseWidget();
    }
}

void LC_ActionInteractivePickAngle::updateInfoCursor(const RS_Vector &mouse, const RS_Vector &point2, const RS_Vector &startPoint) {
    if (m_infoCursorOverlayPrefs->enabled) {

        double angle = LC_LineMath::angleFor3Points(m_point1, point2, mouse);

        double angleComplementary, angleSupplementary, angleAlt;
        RS_Math::calculateAngles(angle, angleComplementary, angleSupplementary, angleAlt);

        msg(tr("Pick Angle"))
            .rawAngle(tr("Angle:"), angle)
            .rawAngle(tr("Complementary:"), angleComplementary)
            .rawAngle(tr("Supplementary:"), angleSupplementary)
            .rawAngle(tr("Alternative: "), angleAlt)
            .vector(tr("From:"), startPoint)
            .vector(tr("Intersection:"), point2)
            .vector(tr("To:"), mouse)
            .linear(tr("Distance1:"), point2.distanceTo(startPoint))
            .linear(tr("Distance2:"), point2.distanceTo(mouse))
            .wcsAngle(tr("Angle 1:"), point2.angleTo(startPoint))
            .wcsAngle(tr("Angle 2:"), point2.angleTo(mouse))
            .toInfoCursorZone2(false);
    }
}

void LC_ActionInteractivePickAngle::updateInfoCursor(const RS_Vector& mouse, const RS_Vector& startPoint) {
    if (m_infoCursorOverlayPrefs->enabled){
        msg(tr("Pick Angle"))
            .linear(tr("Distance:"), startPoint.distanceTo(mouse))
            .wcsAngle(tr("Angle:"), startPoint.angleTo(mouse))
            .vector(tr("From:"), startPoint)
            .vector(tr("To:"), mouse)
            .toInfoCursorZone2(false);
    }
}

void LC_ActionInteractivePickAngle::doTrigger() {
    LC_ActionInteractivePickBase::doTrigger();
}
