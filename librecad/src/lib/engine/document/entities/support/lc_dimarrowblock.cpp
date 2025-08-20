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

#include "lc_dimarrowblock.h"

#include "rs_pen.h"

void LC_DimArrow::move(const RS_Vector& offset) {
    // this is just ugly construction that is needed to avoid direct invocation of protected fields.
    // So an additional boilerplate and performance overhead should be created in order to satisfy generic logic
    // (updating borders at the end of move) and performing actual calculation in move() in this and
    // inherited classes.
    // as m_position is not accessible in inherited class, move() for it may not be called directly.
    // Alternatives:
    // 1) exposing reference to the point in class interface - yet that it's actually bad choice, as it leads
    // to encapsulation break and exposing the internals of the class. Plus, it may deliver unwanted subtle
    // side effects that are hard to identify, if modification of the position is performed outsider of setter method.
    // 2) it's possible just to skip implementation of method in base class - and just create as many copies of this
    // logic in code as there will be inherited classes (one copy per inheritor). This is probably even worse approach...
    //
    // So the choice there is between worst and bad choices, and this is just due to incorrect application of the pattern...

    m_position.move(offset);
    doMove(offset);
    calculateBorders();
}

void LC_DimArrow::doMove([[maybe_unused]]const RS_Vector& offset) {
}

void LC_DimArrow::doRotate([[maybe_unused]]const RS_Vector& center, [[maybe_unused]]RS_Vector angleVector) {
}

void LC_DimArrow::rotate(const RS_Vector& center, double angle) {
    RS_Vector angleVector(angle);
    rotate(center, angleVector);
}

void LC_DimArrow::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_position.rotate(angleVector);
    doRotate(center, angleVector);
    calculateBorders();
}

void LC_DimArrow::doScale([[maybe_unused]]const RS_Vector& center, [[maybe_unused]]const RS_Vector& factor) {
}

void LC_DimArrow::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_position.scale(center, factor);
    doScale(center, factor);
    calculateBorders();
}

void LC_DimArrow::doMirror([[maybe_unused]]const RS_Vector& axisPoint1, [[maybe_unused]]const RS_Vector& axisPoint2) {
}

void LC_DimArrow::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_position.mirror(axisPoint1, axisPoint2);
    doMirror(axisPoint1, axisPoint2);
    calculateBorders();
}

RS_Vector LC_DimArrow::getNearestCenter([[maybe_unused]] const RS_Vector& coord,
                                        double* dist /*= nullptr*/) const {
    setDistPtr(dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

RS_Vector LC_DimArrow::getNearestMiddle([[maybe_unused]] const RS_Vector& coord,
                                        double* dist /*= nullptr*/,
                                        [[maybe_unused]] const int middlePoints /*= 1*/) const {
    setDistPtr(dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

RS_Vector LC_DimArrow::getNearestDist([[maybe_unused]] double distance,
                                      [[maybe_unused]] const RS_Vector& coord,
                                      double* dist /*= nullptr*/) const {
    setDistPtr(dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

void LC_DimArrow::doCalculateBorders() {
}

void LC_DimArrow::setDimLinePoint(const RS_Vector& pos) {
    m_dimLinePoint = pos;
}

void LC_DimArrow::positionDimLinePointFromZero(const RS_Vector &angleVector) {
    m_dimLinePoint.move(m_position);
    m_dimLinePoint.rotate(m_position, angleVector);
}

void LC_DimArrow::calculateBorders() {
    resetBorders();
    minV = RS_Vector::minimum(minV, m_position);
    maxV = RS_Vector::maximum(maxV, m_position);
    doCalculateBorders();
}

RS_Vector LC_DimArrow::getNearestEndpoint([[maybe_unused]]const RS_Vector& coord, [[maybe_unused]]double* dist /*= nullptr*/) const {
    return m_position;
}

void LC_DimArrow::setDistPtr(double* dist, double value) const {
    if (nullptr != dist) {
        *dist = value;
    }
}

RS_Vector LC_DimArrow::getNearestPointOnEntity(const RS_Vector& coord,
                                               [[maybe_unused]]bool onEntity /*= true*/,
                                               double* dist /*= nullptr*/,
                                              [[maybe_unused]] RS_Entity** entity /*= nullptr*/) const {
    *dist = m_position.distanceTo(coord);
    return m_position;
}

double LC_DimArrow::getDistanceToPoint(const RS_Vector& coord,
                                    RS_Entity** entity /*= nullptr*/,
                                    [[maybe_unused]] RS2::ResolveLevel level /*= RS2::ResolveNone*/,
                                    [[maybe_unused]] double solidDist /*= RS_MAXDOUBLE*/) const{
    if (nullptr != entity) {
        *entity = const_cast<LC_DimArrow*>(this);
    }

    double ret {0.0};
    getNearestPointOnEntity( coord, true, &ret, entity);
    return ret;
}
