/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "rs_solid.h"

#include<algorithm>
#include<array>
#include<iostream>

#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_painter.h"

namespace {
    /**
     * @brief getLinePointDistance - find the shortest distance from a point to points
     *                               on a line segment
     * @param line - a line
     * @param point - a point
     * @return the shortest distance in between
     */
    double getLinePointDistance(const RS_Line& line, const RS_Vector& point) {
        double distance = RS_MAXDOUBLE;
        line.getNearestPointOnEntity(point, true, &distance);
        return distance;
    }

    /**
     * @brief whether a point is within a contour
     */
    bool isInternalPoint(const RS_Vector& point, const RS_VectorSolutions& contour) {
        if (!point.valid) {
            return false;
        }

        RS_VectorSolutions sol;

        // ignore invalid points
        std::copy_if(contour.begin(), contour.end(), std::back_inserter(sol), [](const RS_Vector& vp) {
            return vp.valid;
        });

        switch (sol.size()) {
            case 0:
                return false;
            case 1:
                return sol.front().distanceTo(point) < RS_TOLERANCE;
            case 2:
                return getLinePointDistance(RS_Line{nullptr, {sol.front(), sol.back()}}, point) < RS_TOLERANCE;
            default:
                break;
        }

        QPolygonF polygon;
        std::transform(sol.begin(), sol.end(), std::back_inserter(polygon), [](const RS_Vector& vp) {
            return QPointF(vp.x, vp.y);
        });
        return polygon.containsPoint({point.x, point.y}, Qt::OddEvenFill);
    }
}

RS_SolidData::RS_SolidData() : corners{{RS_Vector(false), RS_Vector(false), RS_Vector(false), RS_Vector(false)}} {
}

/**
 * Constructor for a solid with 3 corners.
 */
RS_SolidData::RS_SolidData(const RS_Vector& corner1, const RS_Vector& corner2, const RS_Vector& corner3) : corners{
    {corner1, corner2, corner3, RS_Vector(false)}
} {
}

/**
 * Constructor for a solid with 4 corners.
 */
RS_SolidData::RS_SolidData(const RS_Vector& corner1, const RS_Vector& corner2, const RS_Vector& corner3,
                           const RS_Vector& corner4) : corners{{corner1, corner2, corner3, corner4}} {
}

std::ostream& operator <<(std::ostream& os, const RS_SolidData& pd) {
    os << "(";
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; i++) {
        os << pd.corners[i];
    }
    os << ")";

    return os;
}

/**
 * Default constructor.
 */
RS_Solid::RS_Solid(RS_EntityContainer* parent, const RS_SolidData& d) : RS_AtomicEntity(parent), m_data(d) {
    RS_Solid::calculateBorders();
}

RS_Solid::RS_Solid(const RS_SolidData& d) : RS_AtomicEntity(nullptr), m_data(d) {
    RS_Solid::calculateBorders();
}

RS_Entity* RS_Solid::clone() const {
    auto* s = new RS_Solid(*this);
    return s;
}

/**
 * @return Corner number 'num'.
 */
RS_Vector RS_Solid::unsafeGetCorner(const int num) const {
    return m_data.corners[num];
}

RS_Vector RS_Solid::getCorner(const int num) const {
    if (num >= RS_SolidData::FirstCorner && num < RS_SolidData::MaxCorners) {
        return m_data.corners[num];
    }

    RS_DEBUG->print(RS_Debug::D_WARNING, "Illegal corner requested from Solid");
    return RS_Vector(false);
}

/**
 * Shapes this Solid into a standard arrow (used in dimensions).
 *
 * @param point The point the arrow points to.
 * @param angle Direction of the arrow.
 * @param arrowSize Size of arrow (length).
 */
void RS_Solid::shapeArrow(const RS_Vector& point, const double angle, const double arrowSize) {
    const double arrowSide{arrowSize / cos(0.165)};
    const double cosv1{cos(angle + 0.165) * arrowSide};
    const double sinv1{sin(angle + 0.165) * arrowSide};
    const double cosv2{cos(angle - 0.165) * arrowSide};
    const double sinv2{sin(angle - 0.165) * arrowSide};

    m_data.corners[0] = point;
    m_data.corners[1] = RS_Vector(point.x - cosv1, point.y - sinv1);
    m_data.corners[2] = RS_Vector(point.x - cosv2, point.y - sinv2);
    m_data.corners[3] = RS_Vector(false);

    calculateBorders();
}

void RS_Solid::calculateBorders() {
    resetBorders();

    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (m_data.corners[i].valid) {
            m_minV = RS_Vector::minimum(m_minV, m_data.corners[i]);
            m_maxV = RS_Vector::maximum(m_maxV, m_data.corners[i]);
        }
    }
}

RS_Vector RS_Solid::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    double minDist{RS_MAXDOUBLE};
    double curDist{0.0};
    RS_Vector ret;

    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (m_data.corners[i].valid) {
            curDist = m_data.corners[i].distanceTo(coord);
            if (curDist < minDist) {
                ret = m_data.corners[i];
                minDist = curDist;
            }
        }
    }

    setDistPtr(dist, minDist);
    if (entity != nullptr) {
        *entity = const_cast<RS_Solid*>(this);
    }
    return ret;
}

bool RS_Solid::isInCrossWindow(const RS_Vector& v1, const RS_Vector& v2) const {
    RS_Vector vBL;
    RS_Vector vTR;
    RS_VectorSolutions sol;

    //sort input vectors to BottomLeft & TopRight
    if (v1.x < v2.x) {
        vBL.x = v1.x;
        vTR.x = v2.x;
    }
    else {
        vBL.x = v2.x;
        vTR.x = v1.x;
    }
    if (v1.y < v2.y) {
        vBL.y = v1.y;
        vTR.y = v2.y;
    }
    else {
        vBL.y = v2.y;
        vTR.y = v1.y;
    }

    //Check if entity is out of window
    if (getMin().x > vTR.x || getMax().x < vBL.x || getMin().y > vTR.y || getMax().y < vBL.y) {
        return false;
    }

    std::vector<RS_Line> border(4);
    border.emplace_back(m_data.corners[0], m_data.corners[1]);
    border.emplace_back(m_data.corners[1], m_data.corners[2]);
    if (isTriangle()) {
        border.emplace_back(m_data.corners[2], m_data.corners[0]);
    }
    else {
        border.emplace_back(m_data.corners[2], m_data.corners[3]);
        border.emplace_back(m_data.corners[3], m_data.corners[0]);
    }

    //Find crossing edge
    if (getMax().x > vBL.x && getMin().x < vBL.x) {
        //left
        RS_Line edge{vBL, {vBL.x, vTR.y}};
        for (const auto& line : border) {
            sol = RS_Information::getIntersection(&edge, &line, true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if (getMax().x > vTR.x && getMin().x < vTR.x) {
        //right
        RS_Line edge{{vTR.x, vBL.y}, vTR};
        for (const auto& line : border) {
            sol = RS_Information::getIntersection(&edge, &line, true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if (getMax().y > vBL.y && getMin().y < vBL.y) {
        //bottom
        RS_Line edge{vBL, {vTR.x, vBL.y}};
        for (const auto& line : border) {
            sol = RS_Information::getIntersection(&edge, &line, true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if (getMax().y > vTR.y && getMin().y < vTR.y) {
        //top
        RS_Line edge{{vBL.x, vTR.y}, vTR};
        for (const auto& line : border) {
            sol = RS_Information::getIntersection(&edge, &line, true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }

    return false;
}

/**
*
* @return true if positive o zero, false if negative.
*/
bool RS_Solid::sign(const RS_Vector& v1, const RS_Vector& v2, const RS_Vector& v3) const {
    return !std::signbit(RS_Vector::crossP(v1 - v3, v2 - v3).z);
}

RS_Vector RS_Solid::doGetNearestPointOnEntity(const RS_Vector& coord, const bool onEntity, double* dist, RS_Entity** entity) const {
    //first check if point is inside solid
    if (isInternalPoint(coord, {m_data.corners[0], m_data.corners[1], m_data.corners[2]})) {
        // inside the triangle: always inside the solid, distance is zero
        setDistPtr(dist, 0.0);
        return coord;
    }

    if (!isTriangle()) {
        if (isInternalPoint(coord, {m_data.corners[0], m_data.corners[1], m_data.corners[2], m_data.corners[3]})) {
            setDistPtr(dist, 0.0);
            return coord;
        }
    }

    // not inside solid
    // Find nearest distance from each edge
    if (nullptr != entity) {
        *entity = const_cast<RS_Solid*>(this);
    }

    RS_Vector ret(false);
    double currDist{RS_MAXDOUBLE};
    double tmpDist{0.0};
    const int totalV{isTriangle() ? RS_SolidData::Triangle : RS_SolidData::MaxCorners};

    // fixme - sand - check the logic of solid nearestPoint()!!!
    // IF THERE is <=, i might be 4 and assert in MSVC implementation of stl::vector occurs... on
    // not sure actually, how this will affect the logic, need more changes.
    // for (int i = RS_SolidData::FirstCorner, next = i + 1; i <= totalV; ++i, ++next) {
    for (int i = RS_SolidData::FirstCorner, next = i + 1; i < totalV; ++i, ++next) {
        //closing edge
        if (next == totalV) {
            next = RS_SolidData::FirstCorner;
        }

        // fixme - stl:vector assert occured for m_data.corner[i]
        RS_Vector direction{m_data.corners[next] - m_data.corners[i]};
        RS_Vector vpc{coord - m_data.corners[i]};
        const double a{direction.squared()};
        if (a < RS_TOLERANCE2) {
            //line too short
            vpc = m_data.corners[i];
        }
        else {
            //find projection on line
            vpc = m_data.corners[i] + direction * RS_Vector::dotP(vpc, direction) / a;
        }
        tmpDist = vpc.distanceTo(coord);
        if (tmpDist < currDist) {
            currDist = tmpDist;
            ret = vpc;
        }
    }

    //verify this part
    if (onEntity && !ret.isInWindowOrdered(m_minV, m_maxV)) {
        // projection point not within range, find the nearest endpoint
        ret = getNearestEndpoint(coord, nullptr, dist);
        currDist = ret.distanceTo(coord);
    }

    setDistPtr(dist, currDist);

    return ret;
}

RS_Vector RS_Solid::doGetNearestCenter([[maybe_unused]] const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    setDistPtr(dist, RS_MAXDOUBLE);
    if (entity != nullptr) {
        *entity = const_cast<RS_Solid*>(this);
    }
    return RS_Vector(false);
}

RS_Vector RS_Solid::doGetNearestMiddle([[maybe_unused]] const RS_Vector& coord, double* dist,
                                       [[maybe_unused]] const int middlePoints) const {
    setDistPtr(dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

RS_Vector RS_Solid::doGetNearestDist([[maybe_unused]] double distance, [[maybe_unused]] const RS_Vector& coord, double* dist) const {
    setDistPtr(dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

/**
 * @return Distance from one of the boundary lines of this solid to given point.
 *
 */
double RS_Solid::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, [[maybe_unused]] RS2::ResolveLevel level,
                                      [[maybe_unused]] double solidDist) const {
    if (nullptr != entity) {
        *entity = const_cast<RS_Solid*>(this);
    }

    double ret{0.0};
    getNearestPointOnEntity(coord, true, &ret, entity);
    return ret;
}

void RS_Solid::move(const RS_Vector& offset) {
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (m_data.corners[i].valid) {
            m_data.corners[i].move(offset);
        }
    }
    calculateBorders();
}

void RS_Solid::rotate(const RS_Vector& center, const double angle) {
    const RS_Vector angleVector(angle);
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (m_data.corners[i].valid) {
            m_data.corners[i].rotate(center, angleVector);
        }
    }
    calculateBorders();
}

void RS_Solid::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (m_data.corners[i].valid) {
            m_data.corners[i].rotate(center, angleVector);
        }
    }
    calculateBorders();
}

void RS_Solid::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (m_data.corners[i].valid) {
            m_data.corners[i].scale(center, factor);
        }
    }
    calculateBorders();
}

void RS_Solid::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (m_data.corners[i].valid) {
            m_data.corners[i].mirror(axisPoint1, axisPoint2);
        }
    }
    calculateBorders();
}

void RS_Solid::draw(RS_Painter* painter) {
    painter->drawSolidWCS(std::vector(m_data.corners.cbegin(), m_data.corners.cend()));
}

void RS_Solid::setDistPtr(double* dist, const double value) const {
    if (nullptr != dist) {
        *dist = value;
    }
}

/**
 * Dumps the point's m_data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Solid& p) {
    os << " Solid: " << p.getData() << "\n";
    return os;
}
