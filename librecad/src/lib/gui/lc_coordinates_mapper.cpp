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

LC_CoordinatesMapper::LC_CoordinatesMapper() {}

double  LC_CoordinatesMapper::UserCoordinateSystem::toWorldAngle(double angle) const {return angle - xAxisAngle;};
double  LC_CoordinatesMapper::UserCoordinateSystem::toWorldAngleDegrees(double angle) const {return angle - xAxisAngleDegrees;};
double  LC_CoordinatesMapper::UserCoordinateSystem::toUCSAngle(double angle) const {return angle + xAxisAngle;}

// fixme - sand - ucs - unwrapp calculations
void LC_CoordinatesMapper::UserCoordinateSystem::toUCS(const RS_Vector &worldCoordinate, RS_Vector &ucsCoordinate) const{
    // the code below is equivalent to this
    //  RS_Vector newPos = world - ucsOrigin;
    //  RS_Vector rotated = newPos.rotate(ucsOrigin, xAxisAngle);

    /* double x = worldCoordinate.x - ucsOrigin.x;
     double y = worldCoordinate.y - ucsOrigin.y;

     rotate(x,y);

     ucsCoordinate.x = x;
     ucsCoordinate.y = y;*/

    RS_Vector wcs = worldCoordinate;
    RS_Vector newPos = wcs-ucsOrigin;
    ucsCoordinate = newPos.rotate(xAxisAngle);
}
// fixme - sand - ucs - unwrapp calculations
void LC_CoordinatesMapper::UserCoordinateSystem::toUCS(double worldX, double worldY, double &ucsX, double &ucsY) const {
    // the code below is equivalent to this
    //  RS_Vector newPos = world - ucsOrigin;
    //  RS_Vector rotated = newPos.rotate(ucsOrigin, xAxisAngle);

    /*ucsX = worldX - ucsOrigin.x;
    ucsY = worldY - ucsOrigin.y;

    rotate(ucsX,ucsY);*/


    RS_Vector wcs = RS_Vector(worldX, worldY);
    RS_Vector newPos = wcs-ucsOrigin;
    newPos.rotate(xAxisAngle);
    ucsX = newPos.x;
    ucsY = newPos.y;
}
// fixme - sand - ucs - unwrapp calculations
void LC_CoordinatesMapper::UserCoordinateSystem::toUCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const {
    double magnitude = worldDelta.magnitude();
    double angle = worldDelta.angle();
    double ucsAngle = angle + xAxisAngle;
    ucsDX = magnitude*cos(ucsAngle);
    ucsDY = magnitude*sin(ucsAngle);
}
// fixme - sand - ucs - unwrapp calculations
void LC_CoordinatesMapper::UserCoordinateSystem::toWorld(const RS_Vector &ucsCoordinate, RS_Vector &worldCoordinate) const{
    // RS_Vector rotated = ucs.rotate(UCS_Origin, -ucs_Angle);
    // world = rotated + ucsOrigin

/*    double x = ucsCoordinate.x;
    double y = ucsCoordinate.y;

    rotateBack(x,y);

    x = x + ucsOrigin.x;
    y = y + ucsOrigin.y;

    worldCoordinate.x = x;
    worldCoordinate.y = y;*/

//    ******
    RS_Vector newPos = ucsCoordinate;
    newPos.rotate(-xAxisAngle);
    worldCoordinate  = newPos + ucsOrigin;
}
// fixme - sand - ucs - unwrapp calculations
void LC_CoordinatesMapper::UserCoordinateSystem::toWorld(double ucsX, double ucsY, double &worldX, double &worldY) const{
    // RS_Vector rotated = ucs.rotate(UCS_Origin, -ucs_Angle);
    // world = rotated + ucsOrigin

    /* double x = ucsX;
     double y = ucsY;

     rotateBack(x,y);

     x = x + ucsOrigin.x;
     y = y + ucsOrigin.y;

     worldX = x;
     worldY = y;*/

    RS_Vector ucsCoordinate = RS_Vector(ucsX, ucsY);
    ucsCoordinate.rotate(-xAxisAngle);
    RS_Vector world = ucsCoordinate + ucsOrigin;
    worldX  = world.x;
    worldY = world.y;
}


void LC_CoordinatesMapper::UserCoordinateSystem::rotate(double &x, double &y) const{
    double deltaX = x - ucsOrigin.x;
    double deltaY = y - ucsOrigin.y;

    x = ucsOrigin.x + deltaX * cosXAngle - deltaY * sinXAngle;
    y = ucsOrigin.y + deltaX * sinXAngle + deltaY * cosXAngle;
}

void LC_CoordinatesMapper::UserCoordinateSystem::rotateBack(double &x, double &y) const{
    double deltaX = x - ucsOrigin.x;
    double deltaY = y - ucsOrigin.y;

    x = ucsOrigin.x + deltaX * cosNegativeXAngle - deltaY * sinNegativeXAngle;
    y = ucsOrigin.y + deltaX * sinNegativeXAngle + deltaY * cosNegativeXAngle;
}


void LC_CoordinatesMapper::UserCoordinateSystem::setXAxisAngle(double angle){
    xAxisAngle = angle;
    xAxisAngleDegrees = RS_Math::rad2deg(angle);
    sinXAngle = sin(angle);
    cosXAngle = cos(angle);
    sinNegativeXAngle = sin(-angle);
    cosNegativeXAngle = cos(-angle);
}

void LC_CoordinatesMapper::UserCoordinateSystem::update(const RS_Vector &origin, double angle) {
    ucsOrigin = origin;
    setXAxisAngle(angle);
}

double LC_CoordinatesMapper::UserCoordinateSystem::toUCSAngleDegree(double angle) const {
    return angle + xAxisAngleDegrees;
}

const RS_Vector &LC_CoordinatesMapper::UserCoordinateSystem::getUcsOrigin() const {
    return ucsOrigin;
}

double LC_CoordinatesMapper::UserCoordinateSystem::getXAxisAngle() const {
    return xAxisAngle;
}


double LC_CoordinatesMapper::toWorldAngle(double angle) const{
    if (m_hasUcs){
        return ucs.toWorldAngle(angle);
    }
    else{
        return angle;
    }
}

double LC_CoordinatesMapper::toWorldAngleDegrees(double angle) const{
    if (m_hasUcs){
        return ucs.toWorldAngleDegrees(angle);
    }
    else{
        return angle;
    }
}

RS_Vector LC_CoordinatesMapper::restrictHorizontal(const RS_Vector &baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        double resX, resY;
        ucs.toWorld(ucsCoord.x, ucsBase.y, resX, resY);
        return RS_Vector(resX, resY);
    }
    else{
        return RS_Vector(baseWCSPoint.x, wcsCoord.y);
    }
}

RS_Vector LC_CoordinatesMapper::restrictVertical(const RS_Vector &baseWCSPoint, const RS_Vector &wcsCoord) const {
    if (m_hasUcs) {
        RS_Vector ucsBase = toUCS(baseWCSPoint);
        RS_Vector ucsCoord = toUCS(wcsCoord);
        double resX, resY;
        ucs.toWorld(ucsBase.x, ucsCoord.y, resX, resY);
        return RS_Vector(resX, resY);
    }
    else{
        return RS_Vector(wcsCoord.x, baseWCSPoint.y);
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


double LC_CoordinatesMapper::toUCSAngle(double angle) const{
    double result;
    if (m_hasUcs){
        result = ucs.toUCSAngle(angle);
    }
    else{
        result = angle;
    }
    return result;
}

double LC_CoordinatesMapper::toUCSAngleDegrees(double angle) const{
    double result;
    if (m_hasUcs){
        result = ucs.toUCSAngleDegree(angle);
    }
    else{
        result = angle;
    }
    return result;
}

void LC_CoordinatesMapper::toUCSDelta(const RS_Vector &worldDelta, double &ucsDX, double &ucsDY) const {
    if (m_hasUcs){
        ucs.toUCSDelta(worldDelta, ucsDX, ucsDY);
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
        ucs.toUCSDelta(worldDelta, ucsDX, ucsDY);
        result = RS_Vector(ucsDX, ucsDY, 0);
    }
    else{
        result = RS_Vector(worldDelta.x, worldDelta.y, 0);
    }
    return result;
}


RS_Vector LC_CoordinatesMapper::toUCS(const RS_Vector& v) const{
    RS_Vector result;
    if (m_hasUcs){
        ucs.toUCS(v, result);
        result.valid = true;
        return result;
    }
    else{
        result = v;
    }
    return result;
}

void LC_CoordinatesMapper::toUCS(const RS_Vector& v, double& ucsX, double &ucsY) const{
    if (m_hasUcs){
        ucs.toUCS(v.x, v.y, ucsX, ucsY);
    }
    else{
        ucsX = v.x;
        ucsY = v.y;
    }
}

RS_Vector LC_CoordinatesMapper::toWorld(double ucsX, double ucsY) const{
    RS_Vector result;
    if (m_hasUcs){
        double wcsX, wcsY;
        ucs.toWorld(ucsX, ucsY, wcsX, wcsY);
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
        ucs.toWorld(ucsPos, result);
        result.valid = true;
        return result;
    }
    else{
        result = ucsPos;
    }
    return result;
}
