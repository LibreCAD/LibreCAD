/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_coordinates_mapper.h"

#include "rs_math.h"
#include "rs_vector.h"

LC_CoordinatesMapper::LC_CoordinatesMapper() {
    setXAxisAngle(0.0);
}

RS_Vector LC_CoordinatesMapper::doWCS2UCS(const RS_Vector& worldCoordinate) const {
    // the code below is unwrapped equivalent to this
    /*
            RS_Vector wcs = RS_Vector(worldX, worldY);
            RS_Vector newPos = wcs-ucsOrigin;
            newPos.rotate(xAxisAngle);
            uiY = newPos.x;
            uiX = newPos.y;
    */
    return RS_Vector{worldCoordinate}.move(-m_ucsOrigin).rotate(m_ucsRotation);
}

void LC_CoordinatesMapper::doWCS2UCS(const double worldX, const double worldY, double& ucsX, double& ucsY) const {
    // the code below is unwrapped equivalent to this
    /*
            RS_Vector wcs = RS_Vector(worldX, worldY);
            RS_Vector newPos = wcs-m_ucsOrigin;
            newPos.rotate(xAxisAngle);
            uiY = newPos.x;
            uiX = newPos.y;
    */

    const double ucsPositionX = worldX - m_ucsOrigin.x;
    const double ucsPositionY = worldY - m_ucsOrigin.y;

    ucsX = (ucsPositionX * m_cosXAngle) - (ucsPositionY * m_sinXAngle);
    ucsY = (ucsPositionX * m_sinXAngle) + (ucsPositionY * m_cosXAngle);
}

// todo - sand - ucs - inline calculations
RS_Vector LC_CoordinatesMapper::doWCSDelta2UCSDelta(const RS_Vector& worldDelta) const {
    return worldDelta.rotated(m_xAxisAngle);
}

void LC_CoordinatesMapper::doWCSDelta2UCSDelta(const RS_Vector& worldDelta, double& ucsDX, double& ucsDY) const {
    const double magnitude = worldDelta.magnitude();
    const double angle = worldDelta.angle();
    const double ucsAngle = angle + m_xAxisAngle;
    ucsDX = magnitude * cos(ucsAngle);
    ucsDY = magnitude * sin(ucsAngle);
}

RS_Vector LC_CoordinatesMapper::doUCSDelta2WCSDelta(const RS_Vector& ucsDelta) const {
    return ucsDelta.rotated(-m_xAxisAngle);
}

void LC_CoordinatesMapper::doUCSDelta2WCSDelta(const RS_Vector& ucsDelta, double& wcsDX, double& wcsDY) const {
    const double magnitude = ucsDelta.magnitude();
    const double angle = ucsDelta.angle();
    const double ucsAngle = angle - m_xAxisAngle;
    wcsDX = magnitude * cos(ucsAngle);
    wcsDY = magnitude * sin(ucsAngle);
}

RS_Vector LC_CoordinatesMapper::doUCS2WCS(const RS_Vector& ucsCoordinate) const {
    // code is equivalent to
    /*
        RS_Vector newPos = ucsCoordinate;
        newPos.rotate(-xAxisAngle);
        worldCoordinate  = newPos + ucsOrigin;
    */
    return ucsCoordinate.rotated(m_axisNegRotation) + m_ucsOrigin;
}

void LC_CoordinatesMapper::doUCS2WCS(const double ucsX, const double ucsY, double& worldX, double& worldY) const {
    // code is equivalent to
    /*
        RS_Vector ucsCoordinate = RS_Vector(ucsX, ucsY);
        ucsCoordinate.rotate(-xAxisAngle);
        RS_Vector world = ucsCoordinate + ucsOrigin;
        worldX  = world.x;
        worldY = world.y;
    */

    const double wcsX = (ucsX * m_cosNegativeXAngle) - (ucsY * m_sinNegativeXAngle);
    const double wcsY = (ucsX * m_sinNegativeXAngle) + (ucsY * m_cosNegativeXAngle);

    worldX = wcsX + m_ucsOrigin.x;
    worldY = wcsY + m_ucsOrigin.y;
}

void LC_CoordinatesMapper::setXAxisAngle(const double angle) {
    m_xAxisAngle = angle;
    m_xAxisAngleDegrees = RS_Math::rad2deg(angle);
    m_ucsRotation = RS_Vector{angle};
    m_axisNegRotation = m_ucsRotation;
    m_axisNegRotation.y = -m_ucsRotation.y;
}

void LC_CoordinatesMapper::update(const RS_Vector& origin, const double angle) {
    m_ucsOrigin = origin;
    setXAxisAngle(angle);
}

const RS_Vector& LC_CoordinatesMapper::getUcsOrigin() const {
    return m_ucsOrigin;
}

void LC_CoordinatesMapper::setUcsOrigin(const RS_Vector& origin) {
    m_ucsOrigin = origin;
}

double LC_CoordinatesMapper::toWorldAngle(const double ucsAngle) const {
    return m_hasUcs ? ucsAngle - m_xAxisAngle : ucsAngle;
}

double LC_CoordinatesMapper::toWorldAngleDegrees(const double ucsAngle) const {
    return m_hasUcs ? ucsAngle - m_xAxisAngleDegrees : ucsAngle;
}

RS_Vector LC_CoordinatesMapper::restrictHorizontal(const RS_Vector& baseWCSPoint, const RS_Vector& wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);

        return doUCS2WCS({ucsCoord.x, ucsBase.y});
    }
    return RS_Vector(wcsCoord.x, baseWCSPoint.y);
}

RS_Vector LC_CoordinatesMapper::restrictVertical(const RS_Vector& baseWCSPoint, const RS_Vector& wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        return doUCS2WCS({ucsBase.x, ucsCoord.y});
    }
    return RS_Vector(baseWCSPoint.x, wcsCoord.y);
}

void LC_CoordinatesMapper::ucsBoundingBox(const RS_Vector& wcsMin, const RS_Vector& wcsMax, RS_Vector& ucsMin, RS_Vector& ucsMax) const {
    if (m_hasUcs) {
        /* This implementation does not work, too aggressive clipping of entities
                LC_Rect ucsRect{toUCS(wcsMin), toUCS(wcsMax)};
                ucsRect.merge(toUCS({wcsMin.x, wcsMax.y}));
                ucsRect.merge(toUCS({wcsMax.x, wcsMin.y}));

                ucsMin = ucsRect.minP();
                ucsMax = ucsRect.maxP();
         */
        // this implementation works properly and
        const RS_Vector ucsCorner1 = toUCS(wcsMin);
        const RS_Vector ucsCorner3 = toUCS(wcsMax);
        const auto ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
        const auto ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);

        double maxX = std::max(ucsCorner1.x, ucsCorner3.x);
        maxX = std::max(ucsCorner2.x, maxX);
        maxX = std::max(ucsCorner4.x, maxX);

        double minX = std::min(ucsCorner1.x, ucsCorner3.x);
        minX = std::min(ucsCorner2.x, minX);
        minX = std::min(ucsCorner4.x, minX);

        double maxY = std::max(ucsCorner1.y, ucsCorner3.y);
        maxY = std::max(ucsCorner2.y, maxY);
        maxY = std::max(ucsCorner4.y, maxY);

        double minY = std::min(ucsCorner1.y, ucsCorner3.y);
        minY = std::min(ucsCorner2.y, minY);
        minY = std::min(ucsCorner4.y, minY);

        ucsMin = RS_Vector(minX, minY);
        ucsMax = RS_Vector(maxX, maxY);
    }
    else {
        ucsMin = wcsMin;
        ucsMax = wcsMax;
    }
}

void LC_CoordinatesMapper::worldBoundingBox(const RS_Vector& ucsMin, const RS_Vector& ucsMax, RS_Vector& worldMin,
                                            RS_Vector& worldMax) const {
    if (m_hasUcs) {
        /* This does not work right, too aggressive clipping of entities
                LC_Rect worldRect{toWorld(ucsMin), toWorld(ucsMax)};
                        worldRect.merge(toWorld({ucsMin.x, ucsMax.y}));
                        worldRect.merge(toWorld({ucsMax.x, ucsMin.y}));

                        worldMin = worldRect.minP();
                        worldMax = worldRect.maxP();
        */
        // This implementation is correct
        const RS_Vector ucsCorner1 = ucsMin;
        const RS_Vector ucsCorner3 = ucsMax;
        const auto ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
        const auto ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);

        const RS_Vector worldCorner1 = toWorld(ucsCorner1);
        const RS_Vector worldCorner2 = toWorld(ucsCorner2);
        const RS_Vector worldCorner3 = toWorld(ucsCorner3);
        const RS_Vector worldCorner4 = toWorld(ucsCorner4);

        double maxX = std::max(worldCorner1.x, worldCorner3.x);
        maxX = std::max(worldCorner2.x, maxX);
        maxX = std::max(worldCorner4.x, maxX);

        double minX = std::min(worldCorner1.x, worldCorner3.x);
        minX = std::min(worldCorner2.x, minX);
        minX = std::min(worldCorner4.x, minX);

        double maxY = std::max(worldCorner1.y, worldCorner3.y);
        maxY = std::max(worldCorner2.y, maxY);
        maxY = std::max(worldCorner4.y, maxY);

        double minY = std::min(worldCorner1.y, worldCorner3.y);
        minY = std::min(worldCorner2.y, minY);
        minY = std::min(worldCorner4.y, minY);

        worldMin = RS_Vector(minX, minY);
        worldMax = RS_Vector(maxX, maxY);
    }
    else {
        worldMin = ucsMin;
        worldMax = ucsMax;
    }
}

double LC_CoordinatesMapper::toUCSAngle(const double wcsAngle) const {
    return m_hasUcs ? wcsAngle + m_xAxisAngle : wcsAngle;
}

double LC_CoordinatesMapper::toUCSAngleDegrees(const double wcsAngle) const {
    return m_hasUcs ? wcsAngle + m_xAxisAngleDegrees : wcsAngle;
}

void LC_CoordinatesMapper::toUCSDelta(const RS_Vector& worldDelta, double& ucsDX, double& ucsDY) const {
    if (m_hasUcs) {
        doWCSDelta2UCSDelta(worldDelta, ucsDX, ucsDY);
    }
    else {
        ucsDX = worldDelta.x;
        ucsDY = worldDelta.y;
    }
}

RS_Vector LC_CoordinatesMapper::toUCSDelta(const RS_Vector& worldDelta) const {
    return m_hasUcs ? doWCSDelta2UCSDelta(worldDelta) : worldDelta;
}

RS_Vector LC_CoordinatesMapper::toWorldDelta(const RS_Vector& ucsDelta) const {
    return m_hasUcs ? doUCSDelta2WCSDelta(ucsDelta) : ucsDelta;
}

RS_Vector LC_CoordinatesMapper::toUCS(const RS_Vector& wcsPos) const {
    return m_hasUcs ? doWCS2UCS(wcsPos) : wcsPos;
}

void LC_CoordinatesMapper::toUCS(const RS_Vector& wcsPos, double& ucsX, double& ucsY) const {
    if (m_hasUcs) {
        doWCS2UCS(wcsPos.x, wcsPos.y, ucsX, ucsY);
    }
    else {
        ucsX = wcsPos.x;
        ucsY = wcsPos.y;
    }
}

RS_Vector LC_CoordinatesMapper::toWorld(const double ucsX, const double ucsY) const {
    const RS_Vector ucsPosition{ucsX, ucsY};
    return m_hasUcs ? doUCS2WCS(ucsPosition) : ucsPosition;
}

RS_Vector LC_CoordinatesMapper::toWorld(const RS_Vector& ucsPos) const {
    return m_hasUcs ? doUCS2WCS(ucsPos) : ucsPos;
}

/**
 * Transforms absolute angle in UCS (with zero at 3.pm) to angle in UCS basis (with user defined base zero angle)
 * @param ucsAbsAngle
 * @param baseAngle
 * @param counterclockwise
 * @return
 */
double LC_CoordinatesMapper::toUCSBasisAngle(const double ucsAbsAngle, const double baseAngle, const bool counterclockwise) {
    const double ucsBasisAngle = ucsAbsAngle - baseAngle;
    return counterclockwise ? ucsBasisAngle : M_PI * 2 - ucsBasisAngle;
}

/**
 * transforms angle in ucs basis (with user defined base zero angle) to absolute LC-standard angle value (with zero of angles at at 3.pm)
 * @param ucsBasisAngle
 * @param baseAngle
 * @param counterclockwise
 * @param counterclockwise
 * @return
 */
double LC_CoordinatesMapper::toUCSAbsAngle(const double ucsBasisAngle, const double baseAngle, const bool counterclockwise) {
    const double ucsAbsAngle = ucsBasisAngle + baseAngle;
    return counterclockwise ? ucsAbsAngle : M_PI * 2 - ucsAbsAngle;
}

void LC_CoordinatesMapper::apply(const LC_CoordinatesMapper* other) {
    m_hasUcs = other->hasUCS();
    update(other->getUcsOrigin(), other->getXAxisAngle());
}
