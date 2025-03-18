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
#include "lc_rect.h"
#include "rs_math.h"
#include "rs_vector.h"

LC_CoordinatesMapper::LC_CoordinatesMapper(){
    setXAxisAngle(0.0);
}


RS_Vector LC_CoordinatesMapper::doWCS2UCS(const RS_Vector &worldCoordinate) const {
    // the code below is unwrapped equivalent to this
/*
        RS_Vector wcs = RS_Vector(worldX, worldY);
        RS_Vector newPos = wcs-ucsOrigin;
        newPos.rotate(xAxisAngle);
        uiY = newPos.x;
        uiX = newPos.y;
*/
    return RS_Vector{worldCoordinate}.move(-ucsOrigin).rotate(m_ucsRotation);
}

void LC_CoordinatesMapper::doWCS2UCS(double worldX, double worldY, double &ucsX, double &ucsY) const {
    // the code below is unwrapped equivalent to this
/*
        RS_Vector wcs = RS_Vector(worldX, worldY);
        RS_Vector newPos = wcs-ucsOrigin;
        newPos.rotate(xAxisAngle);
        uiY = newPos.x;
        uiX = newPos.y;
*/

    double ucsPositionX = worldX - ucsOrigin.x;
    double ucsPositionY = worldY - ucsOrigin.y;

    ucsX = ucsPositionX * cosXAngle - ucsPositionY * sinXAngle;
    ucsY = ucsPositionX * sinXAngle + ucsPositionY * cosXAngle;
}

// todo - sand - ucs - inline calculations
RS_Vector LC_CoordinatesMapper::doWCSDelta2UCSDelta(const RS_Vector &worldDelta) const {
    return worldDelta.rotated(xAxisAngle);
}

void LC_CoordinatesMapper::doWCSDelta2UCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const {
    double magnitude = worldDelta.magnitude();
    double angle = worldDelta.angle();
    double ucsAngle = angle + xAxisAngle;
    ucsDX = magnitude*cos(ucsAngle);
    ucsDY = magnitude*sin(ucsAngle);
}

RS_Vector LC_CoordinatesMapper::doUCSDelta2WCSDelta(const RS_Vector &ucsDelta) const {
    return ucsDelta.rotated(-xAxisAngle);
}

void LC_CoordinatesMapper::doUCSDelta2WCSDelta(const RS_Vector &ucsDelta, double &wcsDX, double &wcsDY) const {
    double magnitude = ucsDelta.magnitude();
    double angle = ucsDelta.angle();
    double ucsAngle = angle - xAxisAngle;
    wcsDX = magnitude*cos(ucsAngle);
    wcsDY = magnitude*sin(ucsAngle);
}


RS_Vector LC_CoordinatesMapper::doUCS2WCS(const RS_Vector &ucsCoordinate) const {

    // code is equivalent to
/*
    RS_Vector newPos = ucsCoordinate;
    newPos.rotate(-xAxisAngle);
    worldCoordinate  = newPos + ucsOrigin;
*/
    return ucsCoordinate.rotated(m_AxisNegRotation) + ucsOrigin;
}

void LC_CoordinatesMapper::doUCS2WCS(double ucsX, double ucsY, double &worldX, double &worldY) const{

    // code is equivalent to
/*
    RS_Vector ucsCoordinate = RS_Vector(ucsX, ucsY);
    ucsCoordinate.rotate(-xAxisAngle);
    RS_Vector world = ucsCoordinate + ucsOrigin;
    worldX  = world.x;
    worldY = world.y;
*/

    double wcsX = ucsX * cosNegativeXAngle - ucsY * sinNegativeXAngle;
    double wcsY = ucsX * sinNegativeXAngle + ucsY * cosNegativeXAngle;

    worldX = wcsX + ucsOrigin.x;
    worldY = wcsY + ucsOrigin.y;
}

void LC_CoordinatesMapper::setXAxisAngle(double angle){
    xAxisAngle = angle;
    xAxisAngleDegrees = RS_Math::rad2deg(angle);
    m_ucsRotation = RS_Vector{angle};
    m_AxisNegRotation = m_ucsRotation;
    m_AxisNegRotation.y = - m_ucsRotation.y;
}

void LC_CoordinatesMapper::update(const RS_Vector &origin, double angle) {
    ucsOrigin = origin;
    setXAxisAngle(angle);
}

const RS_Vector &LC_CoordinatesMapper::getUcsOrigin() const {
    return ucsOrigin;
}

double LC_CoordinatesMapper::toWorldAngle(double ucsAngle) const{
    return m_hasUcs ? ucsAngle - xAxisAngle : ucsAngle;
}

double LC_CoordinatesMapper::toWorldAngleDegrees(double ucsAngle) const{
    return m_hasUcs ? ucsAngle - xAxisAngleDegrees : ucsAngle;
}

RS_Vector LC_CoordinatesMapper::restrictHorizontal(const RS_Vector &baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);

        return doUCS2WCS({ucsCoord.x, ucsBase.y});
    }
    else{
        return RS_Vector(wcsCoord.x, baseWCSPoint.y);
    }
}

RS_Vector LC_CoordinatesMapper::restrictVertical(const RS_Vector &baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        return doUCS2WCS({ucsBase.x, ucsCoord.y});
    }
    else{
        return RS_Vector(baseWCSPoint.x, wcsCoord.y);
    }
}

void LC_CoordinatesMapper::ucsBoundingBox(const RS_Vector& wcsMin, const RS_Vector&wcsMax, RS_Vector& ucsMin, RS_Vector& ucsMax) const{
    if (m_hasUcs) {
        LC_Rect ucsRect{toUCS(wcsMin), toUCS(wcsMax)};
        ucsRect.merge(toUCS({wcsMin.x, wcsMax.y}));
        ucsRect.merge(toUCS({wcsMax.x, wcsMin.y}));

        ucsMin = ucsRect.minP();
        ucsMax = ucsRect.maxP();
    }
    else{
        ucsMin = wcsMin;
        ucsMax = wcsMax;
    }
}


void LC_CoordinatesMapper::worldBoundingBox(const RS_Vector& ucsMin, const RS_Vector&ucsMax, RS_Vector& worldMin, RS_Vector& worldMax) const{
    if (m_hasUcs) {
        LC_Rect worldRect{toWorld(ucsMin), toWorld(ucsMax)};
        worldRect.merge(toWorld({ucsMin.x, ucsMax.y}));
        worldRect.merge(toWorld({ucsMax.x, ucsMin.y}));

        worldMin = worldRect.minP();
        worldMax = worldRect.maxP();
    }
    else{
        worldMin = ucsMin;
        worldMax = ucsMax;
    }
}

double LC_CoordinatesMapper::toUCSAngle(double wcsAngle) const{
    return m_hasUcs ? wcsAngle + xAxisAngle : wcsAngle;
}

double LC_CoordinatesMapper::toUCSAngleDegrees(double wcsAngle) const{
    return m_hasUcs ? wcsAngle + xAxisAngleDegrees : wcsAngle;
}


void LC_CoordinatesMapper::toUCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const {
    if (m_hasUcs){
        doWCSDelta2UCSDelta(worldDelta, ucsDX, ucsDY);
    }
    else{
        ucsDX = worldDelta.x;
        ucsDY = worldDelta.y;
    }
}

RS_Vector LC_CoordinatesMapper::toUCSDelta(const RS_Vector& worldDelta) const {
    return  m_hasUcs ? doWCSDelta2UCSDelta(worldDelta) : worldDelta;
}

RS_Vector LC_CoordinatesMapper::toWorldDelta(const RS_Vector& ucsDelta) const {
    return  m_hasUcs ? doUCSDelta2WCSDelta(ucsDelta) : ucsDelta;
}


RS_Vector LC_CoordinatesMapper::toUCS(const RS_Vector& wcsPos) const{
    return  m_hasUcs ? doWCS2UCS(wcsPos) : wcsPos;
}

void LC_CoordinatesMapper::toUCS(const RS_Vector& wcsPos, double& ucsX, double &ucsY) const{
    if (m_hasUcs){
        doWCS2UCS(wcsPos.x, wcsPos.y, ucsX, ucsY);
    }
    else{
        ucsX = wcsPos.x;
        ucsY = wcsPos.y;
    }
}

RS_Vector LC_CoordinatesMapper::toWorld(double ucsX, double ucsY) const{
    const RS_Vector ucsPosition{ucsX, ucsY};
    return m_hasUcs ? doUCS2WCS(ucsPosition) : ucsPosition;
}

RS_Vector LC_CoordinatesMapper::toWorld(const RS_Vector& ucsPos) const{
    return m_hasUcs ? doUCS2WCS(ucsPos) : ucsPos;
}

/**
 * Transforms absolute angle in UCS (with zero at 3.pm) to angle in UCS basis (with user defined base zero angle)
 * @param ucsAbsAngle
 * @param baseAngle
 * @param counterclockwise
 * @return
 */
double LC_CoordinatesMapper::toUCSBasisAngle(double ucsAbsAngle, double baseAngle, bool counterclockwise) {
    const double ucsBasisAngle = ucsAbsAngle - baseAngle;
    return counterclockwise ? ucsBasisAngle : M_PI * 2 - ucsBasisAngle;
}

/**
 * transforms angle in ucs basis (with user defined base zero angle) to absolute LC-standard angle value (with zero of angles at at 3.pm)
 * @param ucsBasisAngle
 * @param baseAngle
 * @param conterclockwise
 * @return
 */
double LC_CoordinatesMapper::toUCSAbsAngle(double ucsBasisAngle, double baseAngle, bool counterclockwise) {
    const double ucsAbsAngle = ucsBasisAngle + baseAngle;
    return counterclockwise ? ucsAbsAngle : M_PI * 2 - ucsAbsAngle;
}


void LC_CoordinatesMapper::apply(LC_CoordinatesMapper *other) {
    m_hasUcs = other->hasUCS();
    update(other->getUcsOrigin(), other->getXAxisAngle());
}
