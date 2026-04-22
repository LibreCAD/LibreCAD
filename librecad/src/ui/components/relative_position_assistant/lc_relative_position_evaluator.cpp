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

#include "lc_relative_position_evaluator.h"

#include "lc_linemath.h"
#include "rs.h"
#include "rs_math.h"
#include "rs_vector.h"

void LC_RelativePositionEvaluator::update(const RS_Vector& wcsPos, const RS_Vector& baseWCSPoint) {
    m_relativeInputData.wcsBasePoint = baseWCSPoint;
    m_relativeInputData.wcsProjection = wcsPos;
    recalculateLengthAndAngle(baseWCSPoint);
    recalculateDeltas(wcsPos);
    m_relativeInputData.isSingleSolution = true;
    m_relativeInputData.setUnmodified();
}

void LC_RelativePositionEvaluator::recalculateDeltas([[]] const RS_Vector& wcsEndPoint) {
    /*const RS_Vector delta = wcsEndPoint - m_relativeInputData.wcsBasePoint;
    m_relativeInputData.wcsDX = delta.x;
    m_relativeInputData.wcsDY = delta.y;*/
    m_relativeInputData.explicitDX = false;
    m_relativeInputData.explicitDY = false;
}

void LC_RelativePositionEvaluator::recalculateLength(const RS_Vector& wcsBasePoint) {
    m_relativeInputData.length = wcsBasePoint.distanceTo(m_relativeInputData.wcsProjection);
    m_relativeInputData.explicitLength = false;
}

void LC_RelativePositionEvaluator::recalculateAngle(const RS_Vector& wcsBasePoint) {
    m_relativeInputData.wcsAngle = wcsBasePoint.angleTo(m_relativeInputData.wcsProjection);
    m_relativeInputData.explicitAngle = false;
}

void LC_RelativePositionEvaluator::recalculateLengthAndAngle(const RS_Vector& wcsBasePoint) {
    recalculateLength(wcsBasePoint);
    recalculateAngle(wcsBasePoint);
}

void LC_RelativePositionEvaluator::calculatePointForGivenXKeepingLength(double length, double wcsAngle, RS_Vector& ucsProjectedPoint) const {
    const auto ucsBase = m_viewport->toUCS(m_relativeInputData.wcsBasePoint);
    const RS_Vector ucsDelta = ucsProjectedPoint - ucsBase;
    const double ucsDX = ucsDelta.x;

    const double ucsDY = std::sqrt(length * length - ucsDX * ucsDX);

    const RS_Vector proj1 = ucsBase + RS_Vector(ucsDX, ucsDY);
    const RS_Vector proj2 = ucsBase + RS_Vector(ucsDX, -ucsDY);

    const double angle1 = ucsBase.angleTo(proj1);
    const double angle2 = ucsBase.angleTo(proj2);

    const double ucsAngle = m_viewport->toUCSAngle(wcsAngle);

    const double deltaAngles1 = RS_Math::getAngleDifferenceU(angle1, ucsAngle);
    const double deltaAngles2 = RS_Math::getAngleDifferenceU(angle2, ucsAngle);
    if (std::abs(deltaAngles1) < std::abs(deltaAngles2)) {
        // which angle is closer to original one?
        ucsProjectedPoint = proj1;
    }
    else {
        ucsProjectedPoint = proj2;
    }
}

void LC_RelativePositionEvaluator::calculatePointForGivenYKeepingLength(double length, double wcsAngle, RS_Vector& ucsProjectedPoint) const {
    const auto ucsBase = m_viewport->toUCS(m_relativeInputData.wcsBasePoint);
    const RS_Vector ucsDelta = ucsProjectedPoint - ucsBase;
    const double ucsDY = ucsDelta.y;

    const double ucsDX = std::sqrt(length * length - ucsDY * ucsDY);

    const RS_Vector proj1 = ucsBase + RS_Vector(ucsDX, ucsDY);
    const RS_Vector proj2 = ucsBase + RS_Vector(-ucsDX, ucsDY);

    const double angle1 = ucsBase.angleTo(proj1);
    const double angle2 = ucsBase.angleTo(proj2);

    const double ucsAngle = m_viewport->toUCSAngle(wcsAngle);

    const double deltaAngles1 = RS_Math::getAngleDifferenceU(angle1, ucsAngle);
    const double deltaAngles2 = RS_Math::getAngleDifferenceU(angle2, ucsAngle);
    if (std::abs(deltaAngles1) < std::abs(deltaAngles2)) {
        // which angle is closer to original one?
        ucsProjectedPoint = proj1;
    }
    else {
        ucsProjectedPoint = proj2;
    }
}

void LC_RelativePositionEvaluator::calculatePointForGivenXKeepingAngle(RS_Vector& ucsProjectedPoint) {
    // try to keep angle
    const double ucsAngle = m_viewport->toUCSAngle(m_relativeInputData.wcsAngle);
    const auto ucsBase = m_viewport->toUCS(m_relativeInputData.wcsBasePoint);
    RS_Vector ucsDelta = ucsProjectedPoint - ucsBase;

    const double normAngle = RS_Math::correctAngle0ToPi(ucsAngle);

    if (LC_LineMath::isSameAngle(normAngle, M_PI_2)) {
        // should not happen, yet still
        ucsDelta.x = m_relativeInputData.length;
        ucsDelta.y = 0.0;
    }
    else {
        ucsDelta.y = ucsDelta.x * std::tan(ucsAngle);
    }

    ucsProjectedPoint = ucsBase + ucsDelta;
    m_relativeInputData.explicitDY = false;
}

void LC_RelativePositionEvaluator::calculatePointForGivenYKeepingAngle(RS_Vector& ucsProjectedPoint) {
    // try to keep angle
    const double ucsAngle = m_viewport->toUCSAngle(m_relativeInputData.wcsAngle);
    const auto ucsBase = m_viewport->toUCS(m_relativeInputData.wcsBasePoint);
    RS_Vector ucsDelta = ucsProjectedPoint - ucsBase;

    const double normAngle = RS_Math::correctAngle0ToPi(ucsAngle);

    if (LC_LineMath::isSameAngle(normAngle, M_PI_2)) {
        // should not happen, yet still
        ucsDelta.x = 0;
    }
    if (LC_LineMath::isSameAngle(normAngle, 0)) {
        // should not happen, yet still
        ucsDelta.y = 0.0;
    }
    else {
        ucsDelta.x = ucsDelta.y / std::tan(ucsAngle);
    }

    ucsProjectedPoint = ucsBase + ucsDelta;
    m_relativeInputData.explicitDY = false;
}

void LC_RelativePositionEvaluator::calculateForX(const RS_Vector& wcsBasePoint, double length, double wcsAngle, RS_Vector ucsProjectedPoint) {
    bool calcAngle = true;
    bool calcLength = true;

    if (m_relativeInputData.explicitDY) {
        // just set coordinate, as above - so we'll just enter explicit point coordinates
    }
    else if (m_relativeInputData.explicitAngle && !m_relativeInputData.explicitLength) {
        // keep angle
        calculatePointForGivenXKeepingAngle(ucsProjectedPoint);
        calcAngle = false;
    }
    else if (m_relativeInputData.explicitLength && !m_relativeInputData.explicitAngle) { // keep length
        calculatePointForGivenXKeepingLength(length, wcsAngle, ucsProjectedPoint);
        calcLength = false;
    }
    else {
        // just set coordinate, as above
    }

    m_relativeInputData.wcsProjection = m_viewport->toWorld(ucsProjectedPoint);
    m_relativeInputData.explicitDX = true;
    if (calcAngle) {
        recalculateAngle(wcsBasePoint);
    }
    if (calcLength) {
        recalculateLength(wcsBasePoint);
    }
}

void LC_RelativePositionEvaluator::calculateForY(const RS_Vector& wcsBasePoint, double length, double wcsAngle, RS_Vector ucsProjectedPoint) {
    bool calcAngle = true;
    bool calcLength = true;

    if (m_relativeInputData.explicitDX) {
        // just set coordinate, as above - so we'll just enter explicit point coordinates
    }
    else if (m_relativeInputData.explicitAngle && !m_relativeInputData.explicitLength) {
        // keep angle
        calculatePointForGivenYKeepingAngle(ucsProjectedPoint);
        calcAngle = false;
    }
    else if (m_relativeInputData.explicitLength && !m_relativeInputData.explicitAngle) { // keep length
        calculatePointForGivenYKeepingLength(length, wcsAngle, ucsProjectedPoint);
        calcLength = false;
    }
    else {
        // just set coordinate, as above
    }

    m_relativeInputData.wcsProjection = m_viewport->toWorld(ucsProjectedPoint);
    m_relativeInputData.explicitDY = true;
    if (calcAngle) {
        recalculateAngle(wcsBasePoint);
    }
    if (calcLength) {
        recalculateLength(wcsBasePoint);
    }
}

void LC_RelativePositionEvaluator::setPositionParam(RS2::RelativePointParam paramType, double value) {
    const RS_Vector wcsBasePoint = m_relativeInputData.wcsBasePoint;
    const double length = m_relativeInputData.length;
    const double wcsAngle = m_relativeInputData.wcsAngle;
    switch (paramType) {
        case RS2::REL_POINT_LENGTH: {
            const double newLength = value;
            if (!LC_LineMath::isSameLength(newLength, length)) {
                // change length, keep angle, re-calculate deltas
                m_relativeInputData.length = newLength;
                m_relativeInputData.explicitLength = true;
                m_relativeInputData.wcsProjection = wcsBasePoint.relative(newLength, wcsAngle);
                recalculateDeltas(m_relativeInputData.wcsProjection);
                m_relativeInputData.isSingleSolution = true;
            }
            break;
        }
        case RS2::REL_POINT_ANGLE: {
            const double newAngle = value;
            if (!LC_LineMath::isSameAngle(newAngle, wcsAngle)) {
                // change angle, keep length, re-calculate deltas
                m_relativeInputData.wcsAngle = newAngle;
                m_relativeInputData.explicitAngle = true;
                m_relativeInputData.wcsProjection = wcsBasePoint.relative(length, newAngle);
                recalculateDeltas(m_relativeInputData.wcsProjection);
                m_relativeInputData.isSingleSolution = true;
            }
            break;
        }
        case RS2::REL_POINT_DX: {
            const RS_Vector ucsBasePoint = m_viewport->toUCS(wcsBasePoint);
            RS_Vector ucsProjectedPoint = m_viewport->toUCS(m_relativeInputData.wcsProjection);
            ucsProjectedPoint.x = ucsBasePoint.x + value;
            calculateForX(wcsBasePoint, length, wcsAngle, ucsProjectedPoint);
            break;
        }
        case RS2::REL_POINT_DY: {
            const RS_Vector ucsBasePoint = m_viewport->toUCS(wcsBasePoint);
            RS_Vector ucsProjectedPoint = m_viewport->toUCS(m_relativeInputData.wcsProjection);
            ucsProjectedPoint.y = ucsBasePoint.y + value;
            calculateForY(wcsBasePoint, length, wcsAngle, ucsProjectedPoint);
            break;
        }
        case RS2::REL_POINT_X: {
            RS_Vector ucsProjectedPoint = m_viewport->toUCS(m_relativeInputData.wcsProjection);
            ucsProjectedPoint.x = value;
            calculateForX(wcsBasePoint, length, wcsAngle, ucsProjectedPoint);
            break;
        }
        case RS2::REL_POINT_Y: {
            RS_Vector ucsProjectedPoint = m_viewport->toUCS(m_relativeInputData.wcsProjection);
            ucsProjectedPoint.y = value;
            calculateForY(wcsBasePoint, length, wcsAngle, ucsProjectedPoint);
            break;
        }
    }
}
