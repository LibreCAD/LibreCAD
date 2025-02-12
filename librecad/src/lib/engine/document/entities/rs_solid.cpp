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

#include<cmath>
#include<iostream>

#include <QVariant>

#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_solid.h"

namespace {
/**
 * @brief isCounterclockwise v3 - v1, v2 - v1, if ordered as counterclockwise
 * @param v1
 * @param v2
 * @param v3
 * @return
 */
bool isCounterClockwise (const RS_Vector& v1, const RS_Vector& v2, const RS_Vector& v3)
{
    double res = RS_Vector::crossP(v2 - v1, v3 - v1).z;

    return !std::signbit(res);
}

/**
 * @brief getLinePointDistance - find the shortest distance from a point to points
 *                               on a line segment
 * @param line - a line
 * @param point - a point
 * @return the shortest distance in between
 */
double getLinePointDistance(const RS_Line& line, const RS_Vector& point)
{
    double distance = RS_MAXDOUBLE;
    line.getNearestPointOnEntity(point, true, &distance);
    return distance;
}

// Compare by coordinates, assuming no degenerate points
bool compareCoordinates(const RS_Vector& p0, const RS_Vector& p1)
{
    return p0.distanceTo(p1) >= RS_TOLERANCE && (
               p0.x < p1.x || (p0.x <= p1.x && p0.y < p1.y));
}

/**
 * @brief convexHull - find the convex hull by Graham's scan
 * @param points - input points
 * @return - the convex hull found
 */
RS_VectorSolutions convexHull(const RS_VectorSolutions& points)
{
    RS_VectorSolutions sol = points;
    // ignore invalid points
    auto it = std::remove_if(sol.begin(), sol.end(), [](const RS_Vector& p) {
        return ! p.valid;});
    sol = {{sol.begin(), it}};

    if (sol.size() <= 1)
        return sol;

    // find the left-most and lowest corner
    std::sort(sol.begin(), sol.end(), compareCoordinates);

    // avoid duplicates
    RS_VectorSolutions hull{{sol.at(0)}};
    for(size_t i = 1; i < sol.size(); ++i) {
        if (hull.back().distanceTo(sol.at(i)) > RS_TOLERANCE)
            hull.push_back(sol.at(i));
    }

    if (hull.size() <= 2)
        return hull;

    // soft by the angle to the corner
    std::sort(hull.begin() + 1, hull.end(),
              [lowerLeft=hull.at(0)](const RS_Vector& lhs, const RS_Vector& rhs) {
        return lowerLeft.angleTo(lhs) < lowerLeft.angleTo(rhs);
    });

    // keep the farthest for the same angle
    sol = {hull.at(0), hull.at(1)};
    for (size_t i = 2;
         i < hull.size(); ++i) {
        RS_Vector& back = sol.back();
        const double angle0 = sol.at(0).angleTo(back);
        const double angle1 = sol.at(0).angleTo(hull.at(i));
        if (std::abs(angle0 - angle1) <= RS_TOLERANCE) {
            if (sol.at(0).distanceTo(hull.at(i)) < sol.at(0).distanceTo(back))
                back = hull.at(i);
        } else {
            sol.push_back(hull.at(i));
        }
    }

    // only keep left turns
    hull = {sol.at(0), sol.at(1)};
    for (size_t i = 2; i < sol.size(); ++i) {
        const size_t j = hull.size() - 1;
        if (isCounterClockwise(hull.at(j-1), hull.at(j), sol.at(i)))
            hull.push_back(sol.at(i));
        else
            hull.at(j) = sol.at(i);
    }
    return hull;
}

/**
 * @brief whether a point is within a contour
 */
bool isInternalPoint(const RS_Vector& point, const RS_VectorSolutions& contour)
{
    if (!point.valid)
        return false;

    RS_VectorSolutions convex = convexHull(contour);
    switch(convex.size()) {
    case 0:
        return false;
    case 1:
        return convex.at(0).distanceTo(point) < RS_TOLERANCE;
    case 2:
        return getLinePointDistance(RS_Line{nullptr, {convex.at(0), convex.at(1)}}, point) < RS_TOLERANCE;
    default:
        break;
    }

    QVariant leftTurn{};
    for(size_t i = 0; i < convex.size(); ++i) {
        const bool currentTurn = isCounterClockwise(convex.at(i), convex.at((i+1)%convex.size()), point);
        if (leftTurn.isNull()) {
            leftTurn = currentTurn;
        } else {
            if (leftTurn.toBool() != currentTurn)
                return false;
        }
    }

    return true;
}
}

RS_SolidData::RS_SolidData():
    corner{{RS_Vector(false), RS_Vector(false), RS_Vector(false), RS_Vector(false)}}
{
}

/**
 * Constructor for a solid with 3 corners.
 */
RS_SolidData::RS_SolidData(const RS_Vector& corner1,
                           const RS_Vector& corner2,
                           const RS_Vector& corner3):
    corner{{corner1, corner2, corner3, RS_Vector(false)}}
{
}

/**
 * Constructor for a solid with 4 corners.
 */
RS_SolidData::RS_SolidData(const RS_Vector& corner1,
                           const RS_Vector& corner2,
                           const RS_Vector& corner3,
                           const RS_Vector& corner4):
    corner{{corner1, corner2, corner3, corner4}}
{
}

std::ostream& operator << (std::ostream& os,
                           const RS_SolidData& pd)
{
    os << "(";
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; i++) {
        os << pd.corner[i];
    }
    os << ")";

    return os;
}

/**
 * Default constructor.
 */
RS_Solid::RS_Solid(RS_EntityContainer* parent,
                   const RS_SolidData& d) :
    RS_AtomicEntity(parent),
    data(d)
{
    calculateBorders();
}

RS_Entity* RS_Solid::clone() const
{
    RS_Solid* s = new RS_Solid(*this);
    s->initId();

    return s;
}

/**
 * @return Corner number 'num'.
 */
RS_Vector RS_Solid::unsafeGetCorner(int num) const{
    return data.corner[num];
}

RS_Vector RS_Solid::getCorner(int num) const
{
    if (num >= RS_SolidData::FirstCorner && num < RS_SolidData::MaxCorners) {
        return data.corner[num];
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
void RS_Solid::shapeArrow(const RS_Vector& point,
                          double angle,
                          double arrowSize)
{
    double arrowSide {arrowSize / cos(0.165)};
    double cosv1 {cos( angle + 0.165) * arrowSide};
    double sinv1 {sin( angle + 0.165) * arrowSide};
    double cosv2 {cos( angle - 0.165) * arrowSide};
    double sinv2 {sin( angle - 0.165) * arrowSide};

    data.corner[0] = point;
    data.corner[1] = RS_Vector(point.x - cosv1, point.y - sinv1);
    data.corner[2] = RS_Vector(point.x - cosv2, point.y - sinv2);
    data.corner[3] = RS_Vector(false);

    calculateBorders();
}

void RS_Solid::calculateBorders()
{
    resetBorders();

    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (data.corner[i].valid) {
            minV = RS_Vector::minimum( minV, data.corner[i]);
            maxV = RS_Vector::maximum( maxV, data.corner[i]);
        }
    }
}

RS_Vector RS_Solid::getNearestEndpoint(const RS_Vector& coord, double* dist /*= nullptr*/)const
{
    double minDist {RS_MAXDOUBLE};
    double curDist {0.0};
    RS_Vector ret;

    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (data.corner[i].valid) {
            curDist = data.corner[i].distanceTo(coord);
            if (curDist < minDist) {
                ret = data.corner[i];
                minDist = curDist;
            }
        }
    }

    setDistPtr( dist, minDist);

    return ret;
}

bool RS_Solid::isInCrossWindow(const RS_Vector& v1, const RS_Vector& v2) const
{
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
    if (getMin().x > vTR.x
        || getMax().x < vBL.x
        || getMin().y > vTR.y
        || getMax().y < vBL.y) {
        return false;
    }

    std::vector<RS_Line> border;
    border.emplace_back( data.corner[0], data.corner[1]);
    border.emplace_back( data.corner[1], data.corner[2]);
    if (isTriangle()) {
        border.emplace_back( data.corner[2], data.corner[0]);
    }
    else {
        border.emplace_back( data.corner[2], data.corner[3]);
        border.emplace_back( data.corner[3], data.corner[0]);
    }

    //Find crossing edge
    if (getMax().x > vBL.x
        && getMin().x < vBL.x) {    //left
        RS_Line edge {vBL, {vBL.x, vTR.y}};
        for (auto const& line: border) {
            sol = RS_Information::getIntersection(&edge, &line, true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if (getMax().x > vTR.x
        && getMin().x < vTR.x) {    //right
        RS_Line edge {{vTR.x, vBL.y}, vTR};
        for (auto const& line: border) {
            sol = RS_Information::getIntersection(&edge, &line, true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if (getMax().y > vBL.y
        && getMin().y < vBL.y) {    //bottom
        RS_Line edge {vBL, {vTR.x, vBL.y}};
        for(auto const& line: border) {
            sol = RS_Information::getIntersection(&edge, &line, true);
            if (sol.hasValid()) {
                return true;
            }
        }
    }
    if(getMax().y > vTR.y
       && getMin().y < vTR.y) { //top
        RS_Line edge {{vBL.x, vTR.y}, vTR};
        for(auto const& line: border) {
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
bool RS_Solid::sign (const RS_Vector& v1, const RS_Vector& v2, const RS_Vector& v3) const
{
    double res = (v1.x - v3.x) * (v2.y - v3.y) - (v2.x - v3.x) * (v1.y - v3.y);

    return (res >= 0.0);
}

/**
 * @todo Implement this.
 */
RS_Vector RS_Solid::getNearestPointOnEntity(const RS_Vector& coord,
                                            bool onEntity /*= true*/,
                                            double* dist /*= nullptr*/,
                                            RS_Entity** entity /*= nullptr*/) const
{
    //first check if point is inside solid
    if (isInternalPoint(coord, {data.corner[0], data.corner[1], data.corner[2]})) {
        // inside of the triangle: always inside of the solid, distance is zero
        setDistPtr( dist, 0.0);
        return coord;
    }

    if (!isTriangle()) {
        if (isInternalPoint(coord, {data.corner[0], data.corner[1], data.corner[2], data.corner[3]})) {
            setDistPtr( dist, 0.0);
            return coord;
        }
    }

    // not inside of solid
    // Find nearest distance from each edge
    if (nullptr != entity) {
        *entity = const_cast<RS_Solid*>(this);
    }

    RS_Vector ret(false);
    double currDist {RS_MAXDOUBLE};
    double tmpDist {0.0};
    int totalV {isTriangle() ? RS_SolidData::Triangle : RS_SolidData::MaxCorners};
    for (int i = RS_SolidData::FirstCorner, next = i + 1; i <= totalV; ++i, ++next) {
        //closing edge
        if (next == totalV) {
            next = RS_SolidData::FirstCorner;
        }

        RS_Vector direction {data.corner[next] - data.corner[i]};
        RS_Vector vpc {coord-data.corner[i]};
        double a {direction.squared()};
        if( a < RS_TOLERANCE2) {
            //line too short
            vpc = data.corner[i];
        }
        else{
            //find projection on line
            vpc = data.corner[i] + direction * RS_Vector::dotP( vpc, direction) / a;
        }
        tmpDist = vpc.distanceTo( coord);
        if (tmpDist < currDist) {
            currDist = tmpDist;
            ret = vpc;
        }
    }

    //verify this part
    if (onEntity && !ret.isInWindowOrdered( minV, maxV)) {
        // projection point not within range, find the nearest endpoint
        ret = getNearestEndpoint( coord, dist);
        currDist = ret.distanceTo( coord);
    }

    setDistPtr( dist, currDist);

    return ret;
}

RS_Vector RS_Solid::getNearestCenter([[maybe_unused]] const RS_Vector& coord,
                                     double* dist /*= nullptr*/) const{
    setDistPtr( dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

RS_Vector RS_Solid::getNearestMiddle([[maybe_unused]] const RS_Vector& coord,
                                     double* dist /*= nullptr*/,
                                     [[maybe_unused]] const int middlePoints /*= 1*/) const{
    setDistPtr( dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

RS_Vector RS_Solid::getNearestDist([[maybe_unused]] double distance,
                                   [[maybe_unused]] const RS_Vector& coord,
                                   double* dist /*= nullptr*/) const{
    setDistPtr( dist, RS_MAXDOUBLE);
    return RS_Vector(false);
}

/**
 * @return Distance from one of the boundary lines of this solid to given point.
 *
 */
double RS_Solid::getDistanceToPoint(const RS_Vector& coord,
                                    RS_Entity** entity /*= nullptr*/,
                                    [[maybe_unused]] RS2::ResolveLevel level /*= RS2::ResolveNone*/,
                                    [[maybe_unused]] double solidDist /*= RS_MAXDOUBLE*/) const{
    if (nullptr != entity) {
        *entity = const_cast<RS_Solid*>(this);
    }

    double ret {0.0};
    getNearestPointOnEntity( coord, true, &ret, entity);
    return ret;
}

void RS_Solid::move(const RS_Vector& offset){
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (data.corner[i].valid) {
            data.corner[i].move(offset);
        }
    }
    calculateBorders();
}

void RS_Solid::rotate(const RS_Vector& center, const double& angle){
    RS_Vector angleVector(angle);
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (data.corner[i].valid) {
            data.corner[i].rotate( center, angleVector);
        }
    }
    calculateBorders();
}

void RS_Solid::rotate(const RS_Vector& center, const RS_Vector& angleVector){
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (data.corner[i].valid) {
            data.corner[i].rotate( center, angleVector);
        }
    }
    calculateBorders();
}

void RS_Solid::scale(const RS_Vector& center, const RS_Vector& factor){
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (data.corner[i].valid) {
            data.corner[i].scale( center, factor);
        }
    }
    calculateBorders();
}

void RS_Solid::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2){
    for (int i = RS_SolidData::FirstCorner; i < RS_SolidData::MaxCorners; ++i) {
        if (data.corner[i].valid) {
            data.corner[i].mirror( axisPoint1, axisPoint2);
        }
    }
    calculateBorders();
}

void RS_Solid::draw(RS_Painter* painter){
    painter->drawSolidWCS(data.corner[0], data.corner[1], data.corner[2], data.corner[3]);
}

void RS_Solid::setDistPtr(double *dist, const double value) const{
    if (nullptr != dist) {
        *dist = value;
    }
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Solid& p){
    os << " Solid: " << p.getData() << "\n";
    return os;
}
