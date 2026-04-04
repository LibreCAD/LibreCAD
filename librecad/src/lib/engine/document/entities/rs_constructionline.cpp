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

#include "rs_constructionline.h"

#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_painter.h"

RS_ConstructionLineData::RS_ConstructionLineData(const RS_Vector& point1, const RS_Vector& point2) : point1(point1), point2(point2) {
}

std::ostream& operator <<(std::ostream& os, const RS_ConstructionLineData& ld) {
    os << "(" << ld.point1 << "/" << ld.point2 << ")";
    return os;
}

/**
 * Constructor.
 */
RS_ConstructionLine::RS_ConstructionLine(RS_EntityContainer* parent, const RS_ConstructionLineData& d)
    : RS_AtomicEntity(parent), m_data(d) {
    RS_ConstructionLine::calculateBorders();
}

RS_ConstructionLine::RS_ConstructionLine(const RS_Vector& point1, const RS_Vector& point2)
    : RS_AtomicEntity(nullptr), m_data(point1, point2) {
}

RS_Entity* RS_ConstructionLine::clone() const {
    const auto c = new RS_ConstructionLine(*this);
    return c;
}

void RS_ConstructionLine::calculateBorders() {
    m_minV = RS_Vector::minimum(m_data.point1, m_data.point2);
    m_maxV = RS_Vector::maximum(m_data.point1, m_data.point2);
}

RS_Vector RS_ConstructionLine::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    const double dist1 = (m_data.point1 - coord).squared();
    const double dist2 = (m_data.point2 - coord).squared();

    if (dist2 < dist1) {
        if (dist != nullptr) {
            *dist = sqrt(dist2);
        }
        return m_data.point2;
    }
    if (dist != nullptr) {
        *dist = sqrt(dist1);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_ConstructionLine*>(this);
    }
    return m_data.point1;
}

RS_Vector RS_ConstructionLine::doGetNearestPointOnEntity(const RS_Vector& coord, [[maybe_unused]]bool onEntity, [[maybe_unused]]double* dist,
                                                       RS_Entity** entity) const {
    if (entity != nullptr) {
        *entity = const_cast<RS_ConstructionLine*>(this);
    }

    const RS_Vector ae = m_data.point2 - m_data.point1;
    const RS_Vector ea = m_data.point1 - m_data.point2;
    const RS_Vector ap = coord - m_data.point1;
    //    RS_Vector ep = coord-data.point2;

    if (ae.magnitude() < 1.0e-6 || ea.magnitude() < 1.0e-6) {
        return RS_Vector(false);
    }

    // Orthogonal projection from both sides:
    const RS_Vector ba = ae * RS_Vector::dotP(ae, ap) / (ae.magnitude() * ae.magnitude());
    //    RS_Vector be = ea * RS_Vector::dotP(ea, ep)
    //                   / (ea.magnitude()*ea.magnitude());

    auto result = m_data.point1 + ba;
    if (dist != nullptr) {
        *dist = coord.distanceTo(result);
    }
    return result;
}

RS_Vector RS_ConstructionLine::doGetNearestCenter([[maybe_unused]]const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    if (dist != nullptr) {
        *dist = RS_MAXDOUBLE;
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_ConstructionLine*>(this);
    }
    return RS_Vector(false);
}

/** @return Copy of data that defines the line. */
const RS_ConstructionLineData& RS_ConstructionLine::getData() const {
    return m_data;
}

/** @return First definition point. */
const RS_Vector& RS_ConstructionLine::getPoint1() const {
    return m_data.point1;
}

/** @return Second definition point. */
const RS_Vector& RS_ConstructionLine::getPoint2() const {
    return m_data.point2;
}

/** @return Start point of the entity */
RS_Vector RS_ConstructionLine::getStartpoint() const {
    return m_data.point1;
}

/** @return End point of the entity */
RS_Vector RS_ConstructionLine::getEndpoint() const {
    return m_data.point2;
}

/**
 * @return Direction 1. The angle at which the arc starts at
 * the startpoint.
 */
double RS_ConstructionLine::getDirection1() const {
    return RS_Math::correctAngle(m_data.point1.angleTo(m_data.point2));
}

/**
 * @return Direction 2. The angle at which the arc starts at
 * the endpoint.
 */
double RS_ConstructionLine::getDirection2() const {
    return RS_Math::correctAngle(m_data.point2.angleTo(m_data.point1));
}

/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_ConstructionLine::getQuadratic() const {
    std::vector<double> ce(3, 0.);
    const auto dvp = m_data.point2 - m_data.point1;
    const RS_Vector normal(-dvp.y, dvp.x);
    ce[0] = normal.x;
    ce[1] = normal.y;
    ce[2] = -normal.dotP(m_data.point2);
    return LC_Quadratic(ce);
}

RS_Vector RS_ConstructionLine::getMiddlePoint() const {
    return RS_Vector(false);
}

RS_Vector RS_ConstructionLine::doGetNearestMiddle([[maybe_unused]]const RS_Vector& coord, double* dist, [[maybe_unused]]const int middlePoints) const {
    if (dist != nullptr) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

RS_Vector RS_ConstructionLine::doGetNearestDist([[maybe_unused]]double distance, [[maybe_unused]]const RS_Vector& coord, double* dist) const {
    if (dist != nullptr) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

double RS_ConstructionLine::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, [[maybe_unused]] RS2::ResolveLevel level,
                                                 [[maybe_unused]] double solidDist) const {
    RS_DEBUG->print("RS_ConstructionLine::getDistanceToPoint");

    if (entity != nullptr) {
        *entity = const_cast<RS_ConstructionLine*>(this);
    }
    //double dist = RS_MAXDOUBLE;
    RS_Vector se = m_data.point2 - m_data.point1;
    const double d(se.magnitude());
    if (d < RS_TOLERANCE) {
        //line too short
        return RS_MAXDOUBLE;
    }
    se.set(se.x / d, -se.y / d); //normalized
    RS_Vector vpc = coord - m_data.point1;
    vpc.rotate(se); // rotate to use the line as x-axis, and the distance is fabs(y)
    return fabs(vpc.y);
}

void RS_ConstructionLine::move(const RS_Vector& offset) {
    m_data.point1.move(offset);
    m_data.point2.move(offset);
    //calculateBorders();
}

void RS_ConstructionLine::rotate(const RS_Vector& center, const double angle) {
    const RS_Vector angleVector(angle);
    m_data.point1.rotate(center, angleVector);
    m_data.point2.rotate(center, angleVector);
    //calculateBorders();
}

void RS_ConstructionLine::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_data.point1.rotate(center, angleVector);
    m_data.point2.rotate(center, angleVector);
    //calculateBorders();
}

void RS_ConstructionLine::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_data.point1.scale(center, factor);
    m_data.point2.scale(center, factor);
    //calculateBorders();
}

void RS_ConstructionLine::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_data.point1.mirror(axisPoint1, axisPoint2);
    m_data.point2.mirror(axisPoint1, axisPoint2);
}

RS_Entity& RS_ConstructionLine::shear(const double k) {
    m_data.point1.shear(k);
    m_data.point2.shear(k);
    return *this;
}

void RS_ConstructionLine::draw(RS_Painter* painter) {
    painter->drawInfiniteWCS(m_data.point1, m_data.point2);
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_ConstructionLine& l) {
    os << " ConstructionLine: " << l.getData() << "\n";
    return os;
}
