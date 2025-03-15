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

#include "rs_vector.h"
#include "lc_coordinates_mapper.h"
#include "rs_math.h"

LC_CoordinatesMapper::LC_CoordinatesMapper(){
    ucsOrigin = RS_Vector(0, 0, 0); setXAxisAngle(0.0);
}


void LC_CoordinatesMapper::doWCS2UCS(const RS_Vector &worldCoordinate, RS_Vector &ucsCoordinate) const{
    // the code below is unwrapped equivalent to this
/*
        RS_Vector wcs = RS_Vector(worldX, worldY);
        RS_Vector newPos = wcs-ucsOrigin;
        newPos.rotate(xAxisAngle);
        uiY = newPos.x;
        uiX = newPos.y;
*/

    double ucsPositionX = worldCoordinate.x - ucsOrigin.x;
    double ucsPositionY = worldCoordinate.y - ucsOrigin.y;

    double ucsX = ucsPositionX * cosXAngle - ucsPositionY * sinXAngle;
    double ucsY = ucsPositionX * sinXAngle + ucsPositionY * cosXAngle;

    ucsCoordinate.x = ucsX;
    ucsCoordinate.y = ucsY;
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
void LC_CoordinatesMapper::doWCSDelta2UCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const {
    double magnitude = worldDelta.magnitude();
    double angle = worldDelta.angle();
    double ucsAngle = angle + xAxisAngle;
    ucsDX = magnitude*cos(ucsAngle);
    ucsDY = magnitude*sin(ucsAngle);
}

void LC_CoordinatesMapper::doUCSDelta2WCSDelta(const RS_Vector &ucsDelta, double &wcsDX, double &wcsDY) const {
    double magnitude = ucsDelta.magnitude();
    double angle = ucsDelta.angle();
    double ucsAngle = angle - xAxisAngle;
    wcsDX = magnitude*cos(ucsAngle);
    wcsDY = magnitude*sin(ucsAngle);
}


void LC_CoordinatesMapper::doUCS2WCS(const RS_Vector &ucsCoordinate, RS_Vector &worldCoordinate) const{

    // code is equivalent to
/*
    RS_Vector newPos = ucsCoordinate;
    newPos.rotate(-xAxisAngle);
    worldCoordinate  = newPos + ucsOrigin;
*/
    double wcsX = ucsCoordinate.x * cosNegativeXAngle - ucsCoordinate.y * sinNegativeXAngle;
    double wcsY = ucsCoordinate.x * sinNegativeXAngle + ucsCoordinate.y * cosNegativeXAngle;

    worldCoordinate.x = wcsX + ucsOrigin.x;
    worldCoordinate.y = wcsY + ucsOrigin.y;
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
    sinXAngle = sin(angle);
    cosXAngle = cos(angle);
    sinNegativeXAngle = sin(-angle);
    cosNegativeXAngle = cos(-angle);
}

void LC_CoordinatesMapper::update(const RS_Vector &origin, double angle) {
    ucsOrigin = origin;
    setXAxisAngle(angle);
}

const RS_Vector &LC_CoordinatesMapper::getUcsOrigin() const {
    return ucsOrigin;
}

double LC_CoordinatesMapper::toWorldAngle(double ucsAngle) const{
    if (m_hasUcs){
        return ucsAngle - xAxisAngle;
    }
    else{
        return ucsAngle;
    }
}

double LC_CoordinatesMapper::toWorldAngleDegrees(double ucsAngle) const{
    if (m_hasUcs){
        return ucsAngle - xAxisAngleDegrees;
    }
    else{
        return ucsAngle;
    }
}

RS_Vector LC_CoordinatesMapper::restrictHorizontal(const RS_Vector &baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        double resX, resY;
        doUCS2WCS(ucsCoord.x, ucsBase.y, resX, resY);
        return RS_Vector(resX, resY);
    }
    else{
        return RS_Vector(wcsCoord.x, baseWCSPoint.y);
    }
}

RS_Vector LC_CoordinatesMapper::restrictVertical(const RS_Vector &baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        double resX, resY;
        doUCS2WCS(ucsBase.x, ucsCoord.y, resX, resY);
        return RS_Vector(resX, resY);
    }
    else{
        return RS_Vector(baseWCSPoint.x, wcsCoord.y);
    }
}

void LC_CoordinatesMapper::ucsBoundingBox(const RS_Vector& wcsMin, const RS_Vector&wcsMax, RS_Vector& ucsMin, RS_Vector& ucsMax) const{
    if (m_hasUcs) {
        RS_Vector ucsCorner1 = toUCS(wcsMin);
        RS_Vector ucsCorner3 = toUCS(wcsMax);
        RS_Vector ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
        RS_Vector ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);

        double minX, maxX;
        double minY, maxY;

        maxX = std::max(ucsCorner1.x, ucsCorner3.x);
        maxX = std::max(ucsCorner2.x, maxX);
        maxX = std::max(ucsCorner4.x, maxX);

        minX = std::min(ucsCorner1.x, ucsCorner3.x);
        minX = std::min(ucsCorner2.x, minX);
        minX = std::min(ucsCorner4.x, minX);

        maxY = std::max(ucsCorner1.y, ucsCorner3.y);
        maxY = std::max(ucsCorner2.y, maxY);
        maxY = std::max(ucsCorner4.y, maxY);

        minY = std::min(ucsCorner1.y, ucsCorner3.y);
        minY = std::min(ucsCorner2.y, minY);
        minY = std::min(ucsCorner4.y, minY);

        ucsMin = RS_Vector(minX, minY);
        ucsMax = RS_Vector(maxX, maxY);
    }
    else{
        ucsMin = wcsMin;
        ucsMax = wcsMax;
    }
}


void LC_CoordinatesMapper::worldBoundingBox(const RS_Vector& ucsMin, const RS_Vector&ucsMax, RS_Vector& worldMin, RS_Vector& worldMax) const{
    if (m_hasUcs) {
        RS_Vector ucsCorner1 = ucsMin;
        RS_Vector ucsCorner3 = ucsMax;
        RS_Vector ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
        RS_Vector ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);

        RS_Vector worldCorner1 = toWorld(ucsCorner1);
        RS_Vector worldCorner2 = toWorld(ucsCorner2);
        RS_Vector worldCorner3 = toWorld(ucsCorner3);
        RS_Vector worldCorner4 = toWorld(ucsCorner4);

        double minX, maxX;
        double minY, maxY;

        maxX = std::max(worldCorner1.x, worldCorner3.x);
        maxX = std::max(worldCorner2.x, maxX);
        maxX = std::max(worldCorner4.x, maxX);

        minX = std::min(worldCorner1.x, worldCorner3.x);
        minX = std::min(worldCorner2.x, minX);
        minX = std::min(worldCorner4.x, minX);

        maxY = std::max(worldCorner1.y, worldCorner3.y);
        maxY = std::max(worldCorner2.y, maxY);
        maxY = std::max(worldCorner4.y, maxY);

        minY = std::min(worldCorner1.y, worldCorner3.y);
        minY = std::min(worldCorner2.y, minY);
        minY = std::min(worldCorner4.y, minY);

        worldMin = RS_Vector(minX, minY);
        worldMax = RS_Vector(maxX, maxY);
    }
    else{
        worldMin = ucsMin;
        worldMax = ucsMax;
    }
}

double LC_CoordinatesMapper::toUCSAngle(double wcsAngle) const{
    double result;
    if (m_hasUcs){
        result = wcsAngle + xAxisAngle;
    }
    else{
        result = wcsAngle;
    }
    return result;
}

double LC_CoordinatesMapper::toUCSAngleDegrees(double wcsAngle) const{
    double result;
    if (m_hasUcs){
        result = wcsAngle + xAxisAngleDegrees;
    }
    else{
        result = wcsAngle;
    }
    return result;
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
    RS_Vector result;
    if (m_hasUcs){
        double ucsDX, ucsDY;
        doWCSDelta2UCSDelta(worldDelta, ucsDX, ucsDY);
        result = RS_Vector(ucsDX, ucsDY, 0);
    }
    else{
        result = RS_Vector(worldDelta.x, worldDelta.y, 0);
    }
    return result;
}

RS_Vector LC_CoordinatesMapper::toWorldDelta(const RS_Vector& ucsDelta) const {
    RS_Vector result;
    if (m_hasUcs){
        double ucsDX, ucsDY;
        doUCSDelta2WCSDelta(ucsDelta, ucsDX, ucsDY);
        result = RS_Vector(ucsDX, ucsDY, 0);
    }
    else{
        result = RS_Vector(ucsDelta.x, ucsDelta.y, 0);
    }
    return result;
}


RS_Vector LC_CoordinatesMapper::toUCS(const RS_Vector& wcsPos) const{
    RS_Vector result;
    if (m_hasUcs){
        doWCS2UCS(wcsPos, result);
        result.valid = true;
        return result;
    }
    else{
        result = wcsPos;
    }
    return result;
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
    RS_Vector result;
    if (m_hasUcs){
        double wcsX, wcsY;
        doUCS2WCS(ucsX, ucsY, wcsX, wcsY);
        result = RS_Vector(wcsX, wcsY);
    }
    else{
        result = RS_Vector(ucsX, ucsY);
    }
    return result;
}

RS_Vector LC_CoordinatesMapper::toWorld(const RS_Vector& ucsPos) const{
    RS_Vector result;
    if (m_hasUcs){
        doUCS2WCS(ucsPos, result);
        result.valid = true;
        return result;
    }
    else{
        result = ucsPos;
    }
    return result;
}

/**
 * Transforms absolute angle in UCS (with zero at 3.pm) to angle in UCS basis (with user defined base zero angle)
 * @param ucsAbsAngle
 * @param baseAngle
 * @param counterclockwise
 * @return
 */
double LC_CoordinatesMapper::toUCSBasisAngle(double ucsAbsAngle, double baseAngle, bool counterclockwise) {
    double ucsBasisAngle;
    if (counterclockwise){
        ucsBasisAngle = ucsAbsAngle - baseAngle;
    }
    else{
        ucsBasisAngle = M_PI * 2 - ucsAbsAngle + baseAngle;
    }
    return ucsBasisAngle;
}

/**
 * transforms angle in ucs basis (with user defined base zero angle) to absolute LC-standard angle value (with zero of angles at at 3.pm)
 * @param ucsBasisAngle
 * @param baseAngle
 * @param conterclockwise
 * @return
 */
double LC_CoordinatesMapper::toUCSAbsAngle(double ucsBasisAngle, double baseAngle, bool conterclockwise) {
    double ucsAbsAngle;
    if (conterclockwise){
        ucsAbsAngle = ucsBasisAngle + baseAngle;
    }
    else{
        ucsAbsAngle = M_PI * 2 - ucsBasisAngle + baseAngle;
    }
    return ucsAbsAngle;
}


void LC_CoordinatesMapper::apply(LC_CoordinatesMapper *other) {
    m_hasUcs = other->hasUCS();
    update(other->getUcsOrigin(), other->getXAxisAngle());
}
