/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)
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

#include "rs_circle.h"

#include <QPainterPath>
#include <vector>

#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

namespace {
    // tangent condition tolerance
    // two circles are considered tangent, if the distance is within this factor of the radii
    constexpr double TANGENT_TOLERANCE_FACTOR = 1e-6; // fixme - sand - options candidate?

    /**
     * @brief isCollinearXY whether the 2x3 matrix has degenerate columns
     * @param mat - a 2x3 linear equation to solve an Appollonius
     * @return  true, if the matrix is degenerate, i.e. the 3 input circle centers have identical
     *                x or y-coordinates
     */
    bool identicalXOrY(const std::vector<std::vector<double>>& mat) {
        // matrix must be 2x3 in dimension
        assert(mat.size() >= 2 && mat.front().size() >= 3);
        const auto isDegenerateCol = [&mat](const size_t column) {
            return RS_Math::equal(std::max(std::abs(mat[0][column]), std::abs(mat[1][column])), 0., RS_TOLERANCE);
        };
        // first(x) or second(y) column
        return isDegenerateCol(0) || isDegenerateCol(1);
    }
}

RS_CircleData::RS_CircleData(const RS_Vector& center, const double radius) : center(center), radius(radius) {
}

bool RS_CircleData::isValid() const {
    return center.valid && radius > RS_TOLERANCE;
}

bool RS_CircleData::operator ==(const RS_CircleData& rhs) const {
    if (!(center.valid && rhs.center.valid)) {
        return false;
    }
    if (center.squaredTo(rhs.center) > RS_TOLERANCE2) {
        return false;
    }
    return std::abs(radius - rhs.radius) < RS_TOLERANCE;
}

std::ostream& operator <<(std::ostream& os, const RS_CircleData& ad) {
    os << "(" << ad.center << "/" << ad.radius << ")";
    return os;
}

/**
 * constructor.
 */
RS_Circle::RS_Circle(RS_EntityContainer* parent, const RS_CircleData& d)
    : LC_CachedLengthEntity(parent), m_data(d) {
    RS_Circle::calculateBorders();
}

RS_Circle::RS_Circle(const RS_CircleData& d)
    : LC_CachedLengthEntity(nullptr), m_data(d) {
    RS_Circle::calculateBorders();
}

RS_Entity* RS_Circle::clone() const {
    const auto c = new RS_Circle(*this);
    return c;
}

void RS_Circle::calculateBorders() {
    const RS_Vector r{m_data.radius, m_data.radius};
    m_minV = m_data.center - r;
    m_maxV = m_data.center + r;
    updateLength();
}

/** @return The center point (x) of this arc */
RS_Vector RS_Circle::getCenter() const {
    return m_data.center;
}

/** Sets new center. */
void RS_Circle::setCenter(const RS_Vector& c) {
    m_data.center = c;
}

/** @return The radius of this arc */
double RS_Circle::getRadius() const {
    return m_data.radius;
}

/** Sets new radius. */
void RS_Circle::setRadius(const double r) {
    m_data.radius = r;
}

/**
 * @return Angle length in rad.
 */
double RS_Circle::getAngleLength() const {
    return 2 * M_PI;
}

/**
 * @return Length of the circle which is the circumference.
 */
void RS_Circle::updateLength() {
    m_cachedLength = 2.0 * M_PI * m_data.radius;
}

bool RS_Circle::isTangent(const RS_CircleData& circleData) const {
    const double d = circleData.center.distanceTo(m_data.center);
    double r0 = std::abs(circleData.radius);
    double r1 = std::abs(m_data.radius);
    if (r0 < r1) {
        std::swap(r0, r1);
    }
    const double tol = TANGENT_TOLERANCE_FACTOR * r0;
    if (r1 < tol || d < tol) {
        return false;
    }

    const double tangentTol = std::max(200. * RS_TOLERANCE, tol);
    // Internal or external tangency
    const bool ret = std::abs(d - r0 + r1) < tangentTol || std::abs(d - r0 - r1) < tangentTol;
    return ret;
}


std::vector<RS_Entity*> RS_Circle::offsetTwoSides(const double distance) const {
    std::vector<RS_Entity*> ret(0, nullptr);
    ret.push_back(new RS_Circle(nullptr, {getCenter(), getRadius() + distance}));
    if (std::abs(getRadius() - distance) > RS_TOLERANCE) {
        ret.push_back(new RS_Circle(nullptr, {getCenter(), std::abs(getRadius() - distance)}));
    }
    return ret;
}

// fixme - sand - move Creation functions to RS_Creation!







RS_VectorSolutions RS_Circle::getRefPoints() const {
    const RS_Vector v1(m_data.radius, 0.0);
    const RS_Vector v2(0.0, m_data.radius);
    return RS_VectorSolutions({m_data.center, m_data.center + v1, m_data.center + v2, m_data.center - v1, m_data.center - v2});
}

/**
 * @brief compute nearest endpoint, intersection with X/Y axis at 0, 90, 180 and 270 degree
 *
 * Use getNearestMiddle() method to compute the nearest circle quadrant endpoints
 *
 * @param coord coordinates to compute, e.g. mouse cursor position
 * @param dist double pointer to return distance between mouse pointer and nearest entity point
 * @param entity
 * @return the nearest intersection of the circle with X/Y axis.
 */
RS_Vector RS_Circle::doGetNearestEndpoint(const RS_Vector& coord, double* dist,  [[maybe_unused]]RS_Entity** entity) const {
    return getNearestMiddle(coord, dist, 0);
}

RS_Vector RS_Circle::doGetNearestPointOnEntity(const RS_Vector& coord, [[maybe_unused]] bool onEntity, double* dist,
                                               RS_Entity** entity) const {
    if (entity != nullptr) {
        *entity = const_cast<RS_Circle*>(this);
    }
    RS_Vector vp(coord - m_data.center);
    const double d(vp.magnitude());
    if (d < RS_TOLERANCE) {
        return RS_Vector(false);
    }
    vp = m_data.center + vp * (m_data.radius / d);
    //    RS_DEBUG->print(RS_Debug::D_ERROR, "circle(%g, %g), r=%g: distance to point (%g, %g)\n",data.center.x,data.center.y,coord.x,coord.y);

    if (dist != nullptr) {
        *dist = coord.distanceTo(vp);
        //        RS_DEBUG->print(RS_Debug::D_ERROR, "circle(%g, %g), r=%g: distance to point (%g, %g)=%g\n",data.center.x,data.center.y,coord.x,coord.y,*dist);
    }
    if (entity != nullptr) {
        *entity = const_cast<RS_Circle*>(this);
    }
    return vp;
}

/**
  *find the tangential points from a given point, i.e., the tangent lines should pass
  * the given point and tangential points
  *
  *Author: Dongxu Li
  */
RS_VectorSolutions RS_Circle::getTangentPoint(const RS_Vector& point) const {
    RS_VectorSolutions ret;
    const double radius = getRadius();
    const double r2(radius * radius);
    if (r2 < RS_TOLERANCE2) {
        return ret; //circle too small
    }
    RS_Vector vp(point - getCenter());
    const double c2(vp.squared());
    if (c2 < r2 - radius * 2. * RS_TOLERANCE) {
        //inside point, no tangential point
        return ret;
    }
    if (c2 > r2 + radius * 2. * RS_TOLERANCE) {
        //external point
        RS_Vector vp1(-vp.y, vp.x);
        vp1 *= radius * sqrt(c2 - r2) / c2;
        vp *= r2 / c2;
        vp += getCenter();
        if (vp1.squared() > RS_TOLERANCE2) {
            ret.push_back(vp + vp1);
            ret.push_back(vp - vp1);
            return ret;
        }
    }
    ret.push_back(point);
    return ret;
}

RS_Vector RS_Circle::getTangentDirection(const RS_Vector& point) const {
    const RS_Vector vp(point - getCenter());
    //    double c2(vp.squared());
    //    if(c2<r2-getRadius()*2.*RS_TOLERANCE) {
    //        //inside point, no tangential point
    //        return RS_Vector(false);
    //    }
    return RS_Vector(-vp.y, vp.x);
}

RS_Vector RS_Circle::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const {
    if (dist != nullptr) {
        *dist = coord.distanceTo(m_data.center);
    }
    if (centerEntity != nullptr) {
        *centerEntity = const_cast<RS_Circle*>(this);
    }
    return m_data.center;
}

RS_Vector RS_Circle::getMiddlePoint() const {
    return RS_Vector(false);
}

/**
 * @brief compute middlePoints for each quadrant of a circle
 *
 * 0 middlePoints snaps to axis intersection at 0, 90, 180 and 270 degree (getNearestEndpoint) \n
 * 1 middlePoints snaps to 45, 135, 225 and 315 degree \n
 * 2 middlePoints snaps to 30, 60, 120, 150, 210, 240, 300 and 330 degree \n
 * and so on
 *
 * @param coord coordinates to compute, e.g. mouse cursor position
 * @param dist double pointer to return distance between mouse pointer and nearest entity point
 * @param middlePoints number of middle points to compute per quadrant (0 for endpoints)
 * @return the nearest of equidistant middle points of the circles quadrants.
 */
RS_Vector RS_Circle::doGetNearestMiddle(const RS_Vector& coord, double* dist, const int middlePoints) const {
    if (m_data.radius <= RS_TOLERANCE) {
        //circle too short
        if (nullptr != dist) {
            *dist = RS_MAXDOUBLE;
        }
        return RS_Vector(false);
    }

    RS_Vector vPoint(getNearestPointOnEntity(coord, true, dist));
    const int iCounts = middlePoints + 1;
    const double dAngleSteps = M_PI_2 / iCounts;
    const double dAngleToPoint = m_data.center.angleTo(vPoint);
    int iStepCount = static_cast<int>((dAngleToPoint + 0.5 * dAngleSteps) / dAngleSteps);
    if (0 < middlePoints) {
        // for nearest middle eliminate start/endpoints
        const int iQuadrant = static_cast<int>(dAngleToPoint / 0.5 / M_PI);
        const int iQuadrantStep = iStepCount - iQuadrant * iCounts;
        if (0 == iQuadrantStep) {
            ++iStepCount;
        }
        else if (iCounts == iQuadrantStep) {
            --iStepCount;
        }
    }

    vPoint.setPolar(m_data.radius, dAngleSteps * iStepCount);
    vPoint.move(m_data.center);

    if (dist != nullptr) {
        *dist = vPoint.distanceTo(coord);
    }
    return vPoint;
}

RS_Vector RS_Circle::doGetNearestDist([[maybe_unused]] double distance, [[maybe_unused]] const RS_Vector& coord, double* dist) const {
    if (dist != nullptr) {
        *dist = RS_MAXDOUBLE;
    }
    return RS_Vector(false);
}

RS_Vector RS_Circle::getNearestDistToEndpoint(double /*distance*/, bool /*startp*/) const {
    return RS_Vector(false);
}

RS_Vector RS_Circle::getNearestOrthTan(const RS_Vector& coord, const RS_Line& normal, bool /*onEntity = false*/) const {
    if (!coord.valid) {
        return RS_Vector(false);
    }
    const RS_Vector vp0(coord - getCenter());
    const RS_Vector vp1(normal.getAngle1());
    const double d = RS_Vector::dotP(vp0, vp1);
    if (d >= 0.) {
        return getCenter() + vp1 * getRadius();
    }
    return getCenter() - vp1 * getRadius();
}

RS_Vector RS_Circle::dualLineTangentPoint(const RS_Vector& line) const {
    const RS_Vector dr = line.normalized() * m_data.radius;
    const RS_Vector vp0 = m_data.center + dr;
    const RS_Vector vp1 = m_data.center - dr;
    auto lineEqu = [&line](const RS_Vector& vp) {
        return std::abs(line.dotP(vp) + 1.);
    };

    return lineEqu(vp0) < lineEqu(vp1) ? vp0 : vp1;
}

void RS_Circle::move(const RS_Vector& offset) {
    m_data.center.move(offset);
    moveBorders(offset);
    //    calculateBorders();
}

/**
  * this function creates offset
  *@coord, position indicates the direction of offset
  *@distance, distance of offset
  * return true, if success, otherwise, false
  *
  *Author: Dongxu Li
  */
bool RS_Circle::offset(const RS_Vector& coord, const double distance) {
    /* bool increase = coord.x > 0;
     double newRadius;
     if (increase){
         newRadius = getRadius() + std::abs(distance);
     }
     else{
         newRadius = getRadius() - std::abs(distance);
         if(newRadius < RS_TOLERANCE) {
             return false;
         }
     }*/

    const double dist(coord.distanceTo(getCenter()));
    double newRadius;
    if (dist > getRadius()) {
        //external
        newRadius = getRadius() + fabs(distance);
    }
    else {
        newRadius = getRadius() - fabs(distance);
        if (newRadius < RS_TOLERANCE) {
            return false;
        }
    }
    setRadius(newRadius);
    calculateBorders();
    return true;
}

void RS_Circle::rotate(const RS_Vector& center, const double angle) {
    m_data.center.rotate(center, angle);
    calculateBorders();
}

void RS_Circle::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    m_data.center.rotate(center, angleVector);
    calculateBorders();
}

void RS_Circle::scale(const RS_Vector& center, const RS_Vector& factor) {
    m_data.center.scale(center, factor);
    //radius always is positive
    m_data.radius *= std::abs(factor.x);
    scaleBorders(center, factor);
}

double RS_Circle::getDirection1() const {
    return M_PI_2;
}

double RS_Circle::getDirection2() const {
    return M_PI_2 * 3.0;
}

void RS_Circle::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    m_data.center.mirror(axisPoint1, axisPoint2);
    calculateBorders();
}

RS_Entity& RS_Circle::shear(const double k) {
    if (!std::isnormal(k)) {
        assert(!"shear() should not not be called for circle");
    }
    return *this;
}
/**
 * Creates a QPainterPath for this circle using the clipped/visible-segment approach.
 * This matches the style used by RS_Arc and RS_Ellipse, enabling consistent behavior
 * when building closed contours for solid fill / hatching.
 */
void RS_Circle::createPainterPath(RS_Painter* painter, QPainterPath& path) const
{
  if (painter == nullptr) {
    return;
  }

  const LC_Rect& vpRect = painter->getWcsBoundingRect();

         // Quick bounding box check
  const RS_Vector minV = getMin();
  const RS_Vector maxV = getMax();

         // Completely outside → nothing to draw
  if (!vpRect.overlaps(LC_Rect(minV, maxV))) {
    return;
  }

         // Fast path: fully inside viewport → use Qt optimized ellipse
  const bool fullyInside =
      minV.x >= vpRect.minP().x && maxV.x <= vpRect.maxP().x &&
      minV.y >= vpRect.minP().y && maxV.y <= vpRect.maxP().y;

  if (fullyInside) {
    const RS_Vector uiCenter = painter->toGui(getCenter());
    const double uiRadius    = painter->toGuiDX(getRadius());

    path.addEllipse(
        uiCenter.x - uiRadius,
        uiCenter.y - uiRadius,
        2.0 * uiRadius,
        2.0 * uiRadius
        );
    return;
  }

         // ───────────────────────────────────────────────
         // General case: partially visible → segment clipping
         // ───────────────────────────────────────────────
  constexpr double baseAngle = 0.0;
  constexpr double fullAngleLength = 2.0 * M_PI;

  std::vector<double> crossPoints;

  const std::array<RS_Vector, 4> vertices = vpRect.vertices();
  for (size_t i = 0; i < vertices.size(); ++i) {
    RS_Line edge{vertices[i], vertices[(i + 1) % vertices.size()]};

    RS_VectorSolutions sols = RS_Information::getIntersection(this, &edge, true);

    for (const RS_Vector& pt : sols) {
      const double dirCircle = getTangentDirection(pt).angle();
      const double dirEdge   = edge.getTangentDirection(pt).angle();

      if (std::abs(RS_Math::correctAngle(dirEdge - dirCircle)) > RS_TOLERANCE_ANGLE) {
        const double angle = (pt - getCenter()).angle();
        double relParam = RS_Math::correctAngle(angle - baseAngle);
        crossPoints.push_back(relParam);
      }
    }
  }

         // Always include full circle range
  crossPoints.insert(crossPoints.begin(), 0.0);
  crossPoints.push_back(fullAngleLength);

         // Sort + remove near-duplicates
  std::sort(crossPoints.begin(), crossPoints.end());
  const auto last = std::unique(crossPoints.begin(), crossPoints.end(),
                          [](double a, double b) { return std::abs(a - b) < RS_TOLERANCE_ANGLE; });
  crossPoints.erase(last, crossPoints.end());

         // Point getter: parameter (angle) → world point
  auto pointAt = [this](double angle) -> RS_Vector {
    return getCenter() + RS_Vector::polar(getRadius(), angle);
  };

         // Delegate to painter's generic parametric curve generator
  painter->pathForParametricCurve(
      path,
      crossPoints,
      pointAt,
      getRadius()   // used for sagitta / approximation control
      );
}

/**
 * Updated draw() method — now consistently uses createPainterPath()
 * (same pattern as RS_Arc and RS_Ellipse)
 */
void RS_Circle::draw(RS_Painter* painter)
{
  if (painter == nullptr) {
    return;
  }

  const double radiusUi = painter->toGuiDX(getRadius());
  if (radiusUi < RS_Painter::getMaximumArcNonErrorRadius()) {
    painter->drawCircleWCS(getCenter(), getRadius());
    return;
  }

  QPainterPath path;

         // Arbitrary starting point at angle=0, mainly to satisfy moveTo requirement
  const RS_Vector centerUi = painter->toGui(getCenter() + RS_Vector{getRadius(), 0.});
  path.moveTo(centerUi.x, centerUi.y);

         // Build the actual path (fast or clipped depending on visibility)
  createPainterPath(painter, path);

  painter->drawPath(path);
}

void RS_Circle::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(m_data.center) < 1.0e-4) {
        m_data.center += offset;
        calculateBorders();
        return;
    }
    RS_Vector v1(m_data.radius, 0.0);
    RS_VectorSolutions sol;
    sol.push_back(m_data.center + v1);
    sol.push_back(m_data.center - v1);
    v1.set(0., m_data.radius);
    sol.push_back(m_data.center + v1);
    sol.push_back(m_data.center - v1);
    double dist;
    v1 = sol.getClosest(ref, &dist);
    if (dist > 1.0e-4) {
        calculateBorders();
        return;
    }
    m_data.radius = m_data.center.distanceTo(v1 + offset);
    calculateBorders();
}

/** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic RS_Circle::getQuadratic() const {
    std::vector<double> ce(6, 0.);
    ce[0] = 1.;
    ce[2] = 1.;
    ce[5] = -m_data.radius * m_data.radius;
    LC_Quadratic ret(ce);
    ret.move(m_data.center);
    return ret;
}

/**
* @brief Returns area of full circle
* Note: Circular arcs are handled separately by RS_Arc (areaLIneIntegral)
* However, full ellipses and ellipse arcs are handled by RS_Ellipse
* @return \pi r^2
*/
double RS_Circle::areaLineIntegral() const {
    const double r = getRadius();
    return M_PI * r * r;
}

/**
 * Dumps the circle's data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const RS_Circle& a) {
    os << " Circle: " << a.m_data << "\n";
    return os;
}
