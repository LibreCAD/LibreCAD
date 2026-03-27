/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include "rs_point.h"

#include<iostream>

#include "rs_circle.h"
#include "rs_painter.h"
#include "lc_quadratic.h"

RS_Point::RS_Point(RS_EntityContainer* parent, const RS_PointData& d)
    : RS_AtomicEntity(parent), m_data(d) {
    RS_Point::calculateBorders();
}

RS_Point::RS_Point(RS_EntityContainer* parent, const RS_Vector& p)
    : RS_AtomicEntity(parent), m_data{RS_PointData(p)} {
    RS_Point::calculateBorders();
}

RS_Entity* RS_Point::clone() const {
    auto* p = new RS_Point(*this);
    return p;
}

RS2::EntityType RS_Point::rtti() const {
    return RS2::EntityPoint;
}

void RS_Point::calculateBorders() {
    m_minV = m_data.pos;
    m_maxV = m_data.pos;
}

RS_VectorSolutions RS_Point::getRefPoints() const {
    return RS_VectorSolutions{m_data.pos};
}

RS_Vector RS_Point::getStartpoint() const {
    return m_data.pos;
}

RS_Vector RS_Point::getEndpoint() const {
    return m_data.pos;
}

RS_PointData RS_Point::getData() const {
    return m_data;
}

RS_Vector RS_Point::getPos() const {
    return m_data.pos;
}

RS_Vector RS_Point::getCenter() const {
    return m_data.pos;
}

double RS_Point::getRadius() const {
    return 0.;
}

bool RS_Point::isTangent(const RS_CircleData& circleData) const {
    const double dist = m_data.pos.distanceTo(circleData.center);
    return fabs(dist - fabs(circleData.radius)) < 50. * RS_TOLERANCE;
}

void RS_Point::setPos(const RS_Vector& pos) {
    m_data.pos = pos;
}

RS_Vector RS_Point::doGetNearestEndpoint(const RS_Vector& coord, double* dist,  RS_Entity** entity) const {
    if (dist != nullptr) {
        *dist = m_data.pos.distanceTo(coord);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_Point*>(this);
    }

    return m_data.pos;
}

RS_Vector RS_Point::doGetNearestPointOnEntity(const RS_Vector& coord, [[maybe_unused]]bool onEntity, double* dist, RS_Entity** entity) const {
    if (dist != nullptr) {
        *dist = m_data.pos.distanceTo(coord);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_Point*>(this);
    }
    return m_data.pos;
}

RS_Vector RS_Point::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    if (dist != nullptr) {
        *dist = m_data.pos.distanceTo(coord);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_Point*>(this);
    }
    return m_data.pos;
}

RS_Vector RS_Point::getMiddlePoint() const {
    return m_data.pos;
}

RS_Vector RS_Point::doGetNearestMiddle(const RS_Vector& coord, double* dist, [[maybe_unused]] const int middlePoints) const {
    if (dist != nullptr) {
        *dist = m_data.pos.distanceTo(coord);
    }
    return m_data.pos;
}

RS_Vector RS_Point::doGetNearestDist([[maybe_unused]]double distance, [[maybe_unused]]const RS_Vector& coord, double* dist) const {
    if (dist != nullptr) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

double RS_Point::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, [[maybe_unused]] RS2::ResolveLevel level,
                                    [[maybe_unused]] double solidDist) const {
    if (entity != nullptr) {
        *entity = const_cast<RS_Point*>(this);
    }
    return m_data.pos.distanceTo(coord);
}

void RS_Point::moveStartpoint(const RS_Vector& pos) {
    m_data.pos = pos;
    calculateBorders();
}

void RS_Point::move(const RS_Vector& offset) {
    m_data.pos.move(offset);
    calculateBorders();
}

void RS_Point::rotate(const RS_Vector& center, const double angle) {
    m_data.pos.rotate(center, angle);
    calculateBorders();
}

void RS_Point::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_data.pos.rotate(center, angleVector);
    calculateBorders();
}

void RS_Point::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_data.pos.scale(center, factor);
    calculateBorders();
}

void RS_Point::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_data.pos.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}

RS_Entity& RS_Point::shear(const double k) {
    m_data.pos.shear(k);
    calculateBorders();
    return *this;
}

void RS_Point::draw(RS_Painter* painter) {
    painter->drawPointEntityWCS(m_data.pos);
}

LC_Quadratic RS_Point::getQuadratic() const {
    const double a = m_data.pos.x;
    const double b = m_data.pos.y;
    return LC_Quadratic({1., 0., 1., -2. * a, -2. * b, a * a + b * b});
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Point& p) {
    os << " Point: " << p.getData() << "\n";
    return os;
}

std::ostream& operator <<(std::ostream& os, const RS_PointData& pd) {
    os << "(" << pd.pos << ")";
    return os;
}
