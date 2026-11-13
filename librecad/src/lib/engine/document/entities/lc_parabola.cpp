/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2014 Pevel Krejcir (pavel@pamsoft.cz)

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
**********************************************************************/

#include "lc_parabola.h"

#include "lc_momentquadrature.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

// convert the parabola control points to SplinePoint control points
LC_SplinePointsData convert2SplineData(const LC_ParabolaData &parabolaData) {
  LC_SplinePointsData splineData{};
  splineData.controlPoints = {parabolaData.m_controlPoints.cbegin(),
                              parabolaData.m_controlPoints.cend()};
  // spline points are probably not used
  const RS_Vector& first = parabolaData.m_controlPoints.front();
  const RS_Vector& control = parabolaData.m_controlPoints.at(1);
  const RS_Vector& last = parabolaData.m_controlPoints.back();
  const RS_Vector chordMidpoint = first + (last - first) * 0.5;
  const RS_Vector sp1 = chordMidpoint + (control - chordMidpoint) * 0.5;
  splineData.splinePoints = {parabolaData.m_controlPoints.front(), sp1,
                             parabolaData.m_controlPoints.back()};
  splineData.useControlPoints = true;
  splineData.closed = false;
  return splineData;
}

// valid the 4 points forms a convex quadralateral. non-parallelogram convex 4 points are required to form a parabola
bool validateConvexPoints(std::vector<RS_Vector>& points)
{
    if (points.size() != 4)
        return false;
    bool valid = std::all_of(points.cbegin(), points.cend(), [](const RS_Vector& point) {
        return point.valid;
    });
    if (!valid)
        return false;
    // sort points by coordinates
    std::sort(points.begin(), points.end(), [](const RS_Vector& v0, const RS_Vector& v1) {
        if (v0.y + RS_TOLERANCE < v1.y)
            return true;
        if (v0.y - RS_TOLERANCE > v1.y)
            return false;
        return v0.x + RS_TOLERANCE < v1.x;
    });
    // find any coincidence
    for(size_t i=1; i<points.size(); ++i)
        if (points[i].squaredTo(points[i-1]) <= RS_TOLERANCE2)
            return false;
    // sort by angle
    std::sort(points.begin() + 1, points.end(), [origin=points.front()] (const RS_Vector& pt0,
              const RS_Vector& pt1) {
        return (pt0 - origin).angle() < (pt1 - origin).angle();
    });
    RS_Line l0{nullptr, {points[0], points[2]}};
    RS_Line l1{nullptr, {points[1], points[3]}};
    RS_VectorSolutions sol = RS_Information::getIntersection(&l0, &l1, true);
    return ! sol.empty();
}

std::vector<RS_Vector> getAxisVectors(std::vector<RS_Vector> pts)
{
    /*
     *
- Let O denote the intersection of the lines AB and CD.

- Locate the point E on the line AB such that the segment OE is equal in length to OB and has the same sense as AO.  (If O is between A and B then E is at B.)

- Locate the point F on the line CD such that the segment OF is equal in length to OC and has the same sense as DO.  (If O is between C and D then F is at C.)

- Let G denote the circle on the diameter AE.

- Let H denote the circle on the diameter DF.

- Let I and J denote the intersections of the line through O perpendicular to AB and the circle G.

- Let K and L denote the intersections of the line through O perpendicular to CD and the circle H.

- Let M and N denote the points on AB that are a distance OI from O.

- Let P and Q denote the points on CD that are a distance OK from O.

- The quadrangle MPNQ is a parallelagram whose sides are parallel to the axes of the two parabolas that pass through the points A, B, C, and D.
     */
    // validate also reorder points to convex points
    if (!validateConvexPoints(pts)) {
        //assert(!"Parasolid points must be convex");
        return {};
    }
    //pts ABCD forms a convex hull in order

    const auto& a = pts[0];
    const auto& b = pts[2];
    const auto& c = pts[1];
    const auto& d = pts[3];
    RS_Line ab{nullptr, {a, b}};
    RS_Line cd{nullptr, {c, d}};
    auto sol0 = RS_Information::getIntersection(&ab, &cd, true);
    if (sol0.empty())
        return {};
    auto po = sol0.at(0);
    //LC_ERR<<"po: "<<po.x<<", "<<po.y;
    auto findCircleIntersection=[&po, &pts](size_t index)->std::pair<RS_Vector, RS_Vector> {
        double ds = std::sqrt((pts[index] - po).magnitude()*(pts[index+2] - po).magnitude());
        RS_Vector dv = (pts[index+2] - pts[index]).normalized();
        return {po - dv * ds, po + dv *ds};
    };
    const auto [pm, pn] = findCircleIntersection(0);
    const auto [pq, pp] = findCircleIntersection(1);
    return { (pm - pq).normalized(), (pn - pq).normalized()};
}

LC_ParabolaData fromPointsAxis(const std::vector<RS_Vector>& points, const RS_Vector& axis)
{
    // points.size() must be exactly 4 — the std::array below is fixed-size
    // and std::transform would write past its end (or leave slots
    // uninitialized) if the input size were different.
    if (points.size() != 4)
        return {};
    if (!std::all_of(points.begin(), points.end(), [](const RS_Vector& point) {
            return point.valid && std::isfinite(point.x) && std::isfinite(point.y) &&
                   std::isfinite(point.z) && point.z == 0.0;
        }))
        return {};
    // Fit in a centered, uniformly normalized frame. The absolute world
    // coordinates otherwise make the least-squares determinant depend on
    // drawing units and lose precision for translated samples.
    const auto& rCenter = points.front();
    const double toAxisAngle = M_PI/2 - axis.angle();
    std::array<RS_Vector, 4> rotated;
    std::transform(points.cbegin(), points.cend(), rotated.begin(),
                   [&rCenter, toAxisAngle](const RS_Vector& point) {
        return (point - rCenter).rotate(RS_Vector{}, toAxisAngle);
    });
    double coordinateScale = 0.0;
    for (const RS_Vector& point : rotated)
        coordinateScale = std::max(coordinateScale, std::hypot(point.x, point.y));
    if (!std::isfinite(coordinateScale) || coordinateScale == 0.0)
        return {};
    for (RS_Vector& point : rotated)
        point /= coordinateScale;
    std::sort(rotated.begin(), rotated.end(), [](const RS_Vector& p0, const RS_Vector& p1) {
        return p0.x < p1.x;
    });
    // y = a*x^2 + b*x + c
    // (y(x2) - y(x1))/(x2 - x1) = a*(x2 + x1) + b
    double sxi2=0., sxi=0., sxyi=0., syi=0.;
    std::vector<double> xis;
    for (size_t i = 1; i < rotated.size(); ++i) {
        double xi = rotated[i].x + rotated.front().x;
        const double deltaX = rotated[i].x - rotated.front().x;
        if (std::abs(deltaX) <= 64.0 * std::numeric_limits<double>::epsilon())
            continue;
        double yi = (rotated[i].y - rotated.front().y)/deltaX;
        sxi2 += xi * xi;
        sxi += xi;
        sxyi += xi * yi;
        syi += yi;
        xis.push_back(xi);
    }
    // least-square
    if (xis.size() < 2)
        return {};
    const double d = sxi2*xis.size() - sxi*sxi;
    const double determinantScale = std::max({1.0, std::abs(sxi2*xis.size()), std::abs(sxi*sxi)});
    if (!std::isfinite(d) || d <= 64.0 * std::numeric_limits<double>::epsilon() * determinantScale)
    {
        //assert(!"least-square failure for Parabola");
        return {};
    }
    const double a = (sxyi*xis.size() - syi*sxi)/d;
    if (!std::isfinite(a) || std::abs(a) <= 64.0 * std::numeric_limits<double>::epsilon())
    {
        return {};
    }
    const double b = (sxi2*syi - sxi*sxyi)/d;
    double c = 0.;
    for (const auto& point: rotated)
    {
        c += point.y - point.x * (b + point.x * a);
    }
    c /= 4;
    double da = {axis.angle() - M_PI/2};
    double ds2 = 0.0;
    for (const auto& point : rotated) {
        const double residual = point.y - (c + point.x * (b + a * point.x));
        const double ds = residual * residual;
        ds2 += ds;
    }
    double pointSpan2 = 0.;
    for (size_t i = 0; i < rotated.size(); ++i)
      for (size_t j = i + 1; j < rotated.size(); ++j)
        pointSpan2 = std::max(pointSpan2, rotated[i].squaredTo(rotated[j]));
    const double fitTolerance2 =
        std::max(RS_TOLERANCE2, RS_TOLERANCE * RS_TOLERANCE * pointSpan2);
    if (ds2 >= fitTolerance2)
      return {};
    // vertex: y=c + bx + ax^2=c-b^2/(4a) + a(x+b/(2a))^2, {-b/(2a), c-b^2/(4a)}
    // a=1/(4h), h = 0.25/a

    auto f1 = [&a, &b, &da](double x) {
        return RS_Vector{1., 2.*a*x +b}.rotate(da);
    };
    std::array<RS_Vector, 2> tangents = {{f1(rotated.front().x), f1(rotated.back().x)}};
    const auto toWorld = [&rCenter, coordinateScale, da](const RS_Vector& point) {
        RS_Vector world = point;
        world.rotate(da);
        return world * coordinateScale + rCenter;
    };

    auto ret = LC_ParabolaData::FromEndPointsTangents(
        {toWorld(rotated.front()), toWorld(rotated.back())},
        tangents
    );

    return ret;
}

}

LC_ParabolaData LC_ParabolaData::FromEndPointsTangents(
           const std::array<RS_Vector, 2>& endPoints,
           const std::array<RS_Vector, 2>& endTangents)
{
    RS_Line l0{nullptr, {endPoints.at(0), endPoints.at(0) + endTangents.at(0)}};
    RS_Line l1{nullptr, {endPoints.at(1), endPoints.at(1) + endTangents.at(1)}};
    auto sol = RS_Information::getIntersection(&l0, &l1);
    if (sol.empty())
        return {};
    return {{endPoints.at(0), sol.at(0), endPoints.at(1)}};
}

std::vector<LC_ParabolaData> LC_ParabolaData::From4Points(const std::vector<RS_Vector>& points)
{
    if (points.size() != 4)
        return {};
    std::vector<RS_Vector> axes = getAxisVectors(points);

    std::vector<LC_ParabolaData> ret;
    std::transform(axes.cbegin(), axes.cend(), std::back_inserter(ret), [&points](const RS_Vector& axis) {
        return fromPointsAxis(points, axis);
    });
    ret.erase(std::remove_if(ret.begin(), ret.end(), [](const LC_ParabolaData& d){return !d.m_valid;}), ret.end());
    return ret;
}

LC_ParabolaData::LC_ParabolaData(std::array<RS_Vector, 3> controlPoints):
    m_controlPoints{std::move(controlPoints)}
  , m_valid{true}
{
    calculatePrimitives();
}

void LC_ParabolaData::calculatePrimitives()
{
    // shift the first control point to origin
    // After shifting the parabola is: 2 t (1-t) c1 + t^2 c2
    // the tangent at start point: 2 c1
    // the tangent: 2(1-2t) c1 + 2t c2

    const auto finitePoint = [](const RS_Vector& point) {
        return point.valid && std::isfinite(point.x) && std::isfinite(point.y) &&
               std::isfinite(point.z) && point.z == 0.0;
    };
    if (!std::all_of(m_controlPoints.begin(), m_controlPoints.end(), finitePoint)) {
        m_valid = false;
        return;
    }

    const RS_Vector c1 = m_controlPoints.at(1) - m_controlPoints.at(0);
    const RS_Vector c2 = m_controlPoints.at(2) - m_controlPoints.at(0);
    const double localScale = std::max(std::hypot(c1.x, c1.y), std::hypot(c2.x, c2.y));
    if (!std::isfinite(localScale) || localScale == 0.0) {
        m_valid = false;
        return;
    }

    // Keep dot products and squared lengths in a unit-scale frame. Valid
    // finite control nets can otherwise overflow or underflow in the vertex
    // and focal-distance calculations.
    const RS_Vector normalizedC1{c1.x / localScale, c1.y / localScale};
    const RS_Vector normalizedC2{c2.x / localScale, c2.y / localScale};
    const RS_Vector normalizedAxis = normalizedC2 * 0.5 - normalizedC1;
    const double curvatureMagnitude = std::hypot(normalizedAxis.x, normalizedAxis.y);
    const double degeneracyTolerance = 4.0 * std::numeric_limits<double>::epsilon();
    if (!std::isfinite(curvatureMagnitude) || curvatureMagnitude <= degeneracyTolerance) {
        m_valid = false;
        return;
    }
    const RS_Vector axisDirection{normalizedAxis.x / curvatureMagnitude,
                                  normalizedAxis.y / curvatureMagnitude};
    // <c2 * 0.5 - c1 | c1 * (2. - 4. * t) + c2 * 2.*t> = 0
    // <c1|c2>-2<c1|c1> = (2<c1|c2>-4<c1|c1>-<c2|c2>+2<c1|c2>)t
    // =-|c2 - 2c1|^2t
    double t = -0.5 * normalizedAxis.dotP(normalizedC1) /
               (curvatureMagnitude * curvatureMagnitude);
    const RS_Vector normalizedVertex =
        normalizedC1 * (2. * t * (1. - t)) + normalizedC2 * (t * t);
    const double dy = (normalizedC2 - normalizedVertex).dotP(axisDirection);
    const double dx = (normalizedC2 - normalizedVertex).dotP({-axisDirection.y, axisDirection.x});
    // dy = dx^2/(4h) , h = dx^2/(4dy)
    const double h = localScale * (dx * dx / (4. * dy));
    if (!std::isfinite(t) || !std::isfinite(dx) || !std::isfinite(dy) ||
        dy == 0.0 || !std::isfinite(h) || h == 0.0) {
        m_valid = false;
        return;
    }
    m_vertex = normalizedVertex * localScale + m_controlPoints.front();
    m_axis = axisDirection * h;
    m_focus = m_vertex + m_axis;
    if (!finitePoint(m_vertex) || !finitePoint(m_axis) || !finitePoint(m_focus)) {
        m_valid = false;
    }
}

RS_LineData LC_ParabolaData::getAxis() const
{
    const RS_Vector vp = (m_controlPoints.front() + m_controlPoints.back())*0.5 - m_vertex;
    return {m_vertex, m_vertex + m_axis*(0.5*vp.dotP(m_axis)/m_axis.squared())};
}

RS_Vector LC_ParabolaData::getFocus() const { return m_focus; }

RS_LineData LC_ParabolaData::getDirectrix() const {
  if (!m_valid)
    return {};
  // The directrix is perpendicular to the axis, at the focal distance h
  // from the vertex on the side opposite the focus. Span it across the
  // parabola's chord width so it renders meaningfully alongside the curve.
  const RS_Vector perp = RS_Vector{-m_axis.y, m_axis.x}.normalized();
  const RS_Vector midpoint = m_vertex - m_axis;
  const double halfWidth = std::max(
      m_axis.magnitude(),
      0.5 * m_controlPoints.front().distanceTo(m_controlPoints.back()));
  return {midpoint - perp * halfWidth, midpoint + perp * halfWidth};
}

double LC_ParabolaData::findX(const RS_Vector& point) const
{
    // in regular coordinates (4hy=x^2)
    const RS_Vector vp = RS_Vector{point}.rotate(m_vertex, M_PI/2 - m_axis.angle()) - m_vertex;
    return vp.x;
}

RS_Vector LC_ParabolaData::fromX(double x) const
{
    // in regular coordinates (4hy=x^2)
    const RS_Vector vp = RS_Vector{x, x*x/(4.*m_axis.magnitude())}.rotate(m_axis.angle() - M_PI/2) + m_vertex;
    return vp;
}

std::array<RS_Vector, 2> LC_ParabolaData::fromXWithTangent(double x) const
{
    const double h = m_vertex.distanceTo(m_focus) * 0.5;
    return {RS_Vector{x, x*x/(4.*h)}.rotate(m_axis.angle() - M_PI/2) + m_vertex,
            RS_Vector{2.*h, x}.rotate(m_axis.angle() - M_PI/2)};
}

/** \brief return the equation of the entity
a quadratic contains coefficients for quadratic:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic LC_ParabolaData::getQuadratic() const
{
    if (!m_valid)
        return LC_Quadratic{};
    std::vector<double> ce(6, 0.);
    ce[0] = 1.;
    ce[4] = -4. * m_axis.magnitude();
    LC_Quadratic lq{ce};
    lq.rotate(m_axis.angle() - M_PI/2);
    lq.move(m_vertex);
    return lq;
}

LC_Parabola::LC_Parabola(RS_EntityContainer *parent, const LC_ParabolaData &d)
    : LC_SplinePoints{parent, convert2SplineData(d)}, m_data{d} {}

RS_Entity* LC_Parabola::clone() const
{
    auto* e = new LC_Parabola(*this);
    return e;
}


RS2::EntityType LC_Parabola::rtti() const
{
    return RS2::EntityParabola;
}

LC_Quadratic LC_Parabola::getQuadratic() const { return m_data.getQuadratic(); }

RS_Vector LC_Parabola::getTangentDirection(const RS_Vector& point)const
{
    RS_VectorSolutions sol = getTangentPoint(point);
    if(sol.empty())
        return {};
    RS_Vector tangentPoint = getTangentPoint(point).at(0);
    RS_Vector p0 = rotateToQuadratic(tangentPoint) - m_data.m_vertex;
    return RS_Vector{2. * m_data.m_axis.magnitude(), p0.x}
        .rotate(m_data.m_axis.angle() - M_PI / 2)
        .normalized();
}

RS_VectorSolutions LC_Parabola::getTangentPoint(const RS_Vector& point) const
{
  RS_Vector p0 = rotateToQuadratic(point) - m_data.m_vertex;
  // y=x^2/(4h)
  // (x^2/(4h) - py) = x/(2h)(x - px)
  // x^2/(4h) - px/(2h) x + py = 0
  // (x - px)^2 = px^2 - 4h py
  // x = px \pm \sqrt(px^2 - 4h py) ; py <= px^2/(4h)
  const double h = m_data.m_axis.magnitude();
  if (4.0 * h * p0.y >= p0.x * p0.x + RS_TOLERANCE)
    return {};
  auto pf = [this, &h](double x) {
    return RS_Vector{x, x * x / (4. * h)}.rotate(m_data.m_axis.angle() -
                                                 M_PI / 2.) +
           m_data.m_vertex;
  };
  double dx = std::sqrt(std::abs(p0.x * p0.x - 4. * h * p0.y));
  if (dx <= RS_TOLERANCE)
    return {point};
  return {pf(p0.x + dx), pf(p0.x - dx)};
}

RS_Vector LC_Parabola::dualLineTangentPoint(const RS_Vector& line) const
{
    // rotate to regular form
    auto uv = RS_Vector{line}.rotate(M_PI / 2. - m_data.m_axis.angle());
    // slope = {2h, x} <(2h,x)|uv> = 0
    // x=-2h uv.x/(uv.y)
    if (std::abs(uv.y) < RS_TOLERANCE)
        return RS_Vector{false};
    return m_data.fromX(-2. * m_data.m_axis.magnitude() * uv.x / uv.y);
}


RS2::Ending LC_Parabola::getTrimPoint(const RS_Vector& trimCoord,
                         const RS_Vector& trimPoint)
{
    double xi[] = {rotateToQuadratic(getStartpoint()).x,
    rotateToQuadratic(trimCoord).x,
    rotateToQuadratic(trimPoint).x};
    return (std::signbit(xi[0] - xi[1]) != std::signbit(xi[2] - xi[1])) ?
                RS2::EndingEnd : RS2::EndingStart;
}

RS_Vector LC_Parabola::prepareTrim(const RS_Vector& trimCoord,
                                   const RS_VectorSolutions& trimSol)
{
    //prepare trimming for multiple intersections
    if ( ! trimSol.hasValid()) return(RS_Vector(false));
    if ( trimSol.getNumber() == 1 ) return(trimSol.get(0));
    auto vp0=trimSol.getClosest(trimCoord, nullptr, 0);

    double dr2=trimCoord.squaredTo(vp0);
    //the trim point found is closer to mouse location (trimCoord) than both end points, return this trim point
    if(dr2 < trimCoord.squaredTo(getStartpoint()) && dr2 < trimCoord.squaredTo(getEndpoint()))
        return vp0;
    //the closer endpoint to trimCoord
    RS_Vector vp1=(trimCoord.squaredTo(getStartpoint()) <= trimCoord.squaredTo(getEndpoint()))?getStartpoint():getEndpoint();

    //searching for intersection in the direction of the closer end point
    auto dvp1=vp1 - trimCoord;
    RS_VectorSolutions sol1;
    for(size_t i=0; i<trimSol.size(); i++){
        auto dvp2=trimSol.at(i) - trimCoord;
        if( RS_Vector::dotP(dvp1, dvp2) > RS_TOLERANCE)
            sol1.push_back(trimSol.at(i));
    }
    //if found intersection in direction, return the closest to trimCoord from it
    if(sol1.size())
        return sol1.getClosest(trimCoord, nullptr, 0);

    //no intersection by direction, return previously found closest intersection
    return vp0;
}

RS_Vector LC_Parabola::rotateToQuadratic(RS_Vector vp) const
{
  return vp.rotate(m_data.m_vertex, M_PI / 2 - m_data.m_axis.angle());
}

void LC_Parabola::moveStartpoint(const RS_Vector &pos) {
  // Trim the parabola so the start coincides with `pos`. The trim machinery
  // computes `pos` to lie on the existing curve; projecting onto the curve
  // is therefore a no-op for valid inputs and tolerant for slight roundoff.
  // Rebuild from the projection point with the same tangent the curve had
  // there and the unchanged end-tangent — so the new control points define
  // the original parabola's [pos, end] sub-segment.
  const RS_Vector p0 = getNearestPointOnEntity(pos);
  const RS_Vector t0 = getTangentDirection(p0);
  const RS_Vector t1 = RS_Vector{getDirection2()};
  m_data = LC_ParabolaData::FromEndPointsTangents(
      {p0, m_data.m_controlPoints.back()}, {t0, t1});
  LC_SplinePoints::getData() = convert2SplineData(m_data);
  calculateBorders();
}

void LC_Parabola::moveEndpoint(const RS_Vector &pos) {
  const RS_Vector t0 = RS_Vector{getDirection1()};
  const RS_Vector p2 = getNearestPointOnEntity(pos);
  const RS_Vector t2 = getTangentDirection(p2);
  m_data = LC_ParabolaData::FromEndPointsTangents(
      {m_data.m_controlPoints.front(), p2}, {t0, t2});
  LC_SplinePoints::getData() = convert2SplineData(m_data);
  calculateBorders();
}

double LC_Parabola::getDirection1() const
{
  return (m_data.m_controlPoints.at(1) - m_data.m_controlPoints.front())
      .angle();
}

double LC_Parabola::getDirection2() const
{
  return (m_data.m_controlPoints.back() - m_data.m_controlPoints.at(1)).angle();
}
RS_VectorSolutions LC_Parabola::getRefPoints() const
{
  return {m_data.m_controlPoints.front(), m_data.m_controlPoints.at(1),
          m_data.m_controlPoints.back()};
}


void LC_Parabola::move(const RS_Vector& offset)
{
  for (auto &point : m_data.m_controlPoints)
    point.move(offset);
  update();
}
void LC_Parabola::rotate(const RS_Vector& center, double angle)
{
  for (auto &point : m_data.m_controlPoints)
    point.rotate(center, angle);
  update();
}
void LC_Parabola::rotate(const RS_Vector& center, const RS_Vector& angleVector)
{
  for (auto &point : m_data.m_controlPoints)
    point.rotate(center, angleVector);
  update();
}
void LC_Parabola::scale(const RS_Vector& center, const RS_Vector& factor)
{
  for (auto &point : m_data.m_controlPoints)
    point.scale(center, factor);
  update();
}
void LC_Parabola::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2)
{
  for (auto &point : m_data.m_controlPoints)
    point.mirror(axisPoint1, axisPoint2);
  update();
}

RS_Entity& LC_Parabola::shear(double k)
{
  for (auto &point : m_data.m_controlPoints)
    point.shear(k);
  update();
  return *this;
}

void LC_Parabola::moveRef(const RS_Vector& ref, const RS_Vector& offset)
{
  for (auto &point : m_data.m_controlPoints)
    if (point.squaredTo(ref) < RS_TOLERANCE2) {
      point.move(offset);
      break;
    }

  update();
}

void LC_Parabola::revertDirection()
{
  std::swap(m_data.m_controlPoints.front(), m_data.m_controlPoints.back());
  // the spline data the curve is read through follows, as for every other edit
  update();
}

bool LC_Parabola::offset([[maybe_unused]] const RS_Vector& coord, [[maybe_unused]] const double distance)
{
  // an offset of a parabola is no parabola: only createOffset() makes one,
  // and a same-type offset of the spline data alone would leave the
  // parabola's own data behind
  return false;
}

void LC_Parabola::update() {
  m_data.calculatePrimitives();
  LC_SplinePoints::getData() = convert2SplineData(m_data);
  calculateBorders();
}

RS_Vector LC_Parabola::getNearestOrthTan([[maybe_unused]] const RS_Vector& coord,
                                        const RS_Line& normal,
                                        bool onEntity ) const
{
    // transform to regular form: 4hy=x^2
    auto line = RS_Vector{normal.getDirection1()}.rotate(M_PI / 2 -
                                                         m_data.m_axis.angle());
    // parabola tangent direction at local x is {2h, x}; orthogonal to `line`
    // means <{2h, x}|line> = 0, so x_cand = -2h * line.x / line.y
    if (std::abs(line.y) < RS_TOLERANCE)
      return RS_Vector{false};
    const double h = m_data.m_axis.magnitude();
    const double x = -2. * h * line.x / line.y;
    if (onEntity) {
      const double x0 = m_data.findX(getStartpoint());
      const double x1 = m_data.findX(getEndpoint());
      if (std::signbit(x - x0) == std::signbit(x - x1))
        return RS_Vector{false};
    }
    return RS_Vector{x, x * x / (4. * h)}.rotate(m_data.m_axis.angle() -
                                                 M_PI / 2) +
           m_data.m_vertex;
}

std::unique_ptr<LC_Parabola> LC_Parabola::approximateOffset(double dist) const
{
  if (!m_data.m_valid)
    return nullptr;
  // Approximate the offset by a parabola that:
  //   - passes through the offset endpoints (perpendicular distance `dist`)
  //   - shares the original tangent direction at each endpoint
  // The exact offset of a parabola is an algebraic curve of degree 6, but
  // the tangent-preserving construction below is the natural one-segment
  // parabola approximation — exact at the endpoints and a close fit in
  // between for moderate `dist`. Positive `dist` offsets to the convex side
  // (away from the focus); negative `dist` offsets toward the focus.
  const RS_Vector &p0 = m_data.m_controlPoints[0];
  const RS_Vector &p1 = m_data.m_controlPoints[1];
  const RS_Vector &p2 = m_data.m_controlPoints[2];

  auto tangentAt = [&](double t) {
    return (p1 - p0) * (2. * (1. - t)) + (p2 - p1) * (2. * t);
  };
  // Unit normal pointing away from the focus (convex side). The parabola
  // axis points from vertex to focus, so flip the 90°-rotated tangent if
  // it ends up pointing toward the focus.
  auto outerNormal = [&](double t) {
    const RS_Vector tan = tangentAt(t).normalized();
    RS_Vector n{-tan.y, tan.x};
    if (n.dotP(m_data.m_axis) > 0.)
      n = -n;
    return n;
  };

  const RS_Vector newStart = p0 + outerNormal(0.) * dist;
  const RS_Vector newEnd = p2 + outerNormal(1.) * dist;
  LC_ParabolaData offsetData = LC_ParabolaData::FromEndPointsTangents(
      {newStart, newEnd}, {tangentAt(0.), tangentAt(1.)});
  if (!offsetData.m_valid)
    return nullptr;
  return std::make_unique<LC_Parabola>(nullptr, offsetData);
}

double LC_Parabola::areaLineIntegral() const
{
  if (!m_data.m_valid) {
    return 0.0;
  }
  const RS_Vector &p0 = m_data.m_controlPoints[0];
  const RS_Vector &p1 = m_data.m_controlPoints[1];
  const RS_Vector &p2 = m_data.m_controlPoints[2];
  double x0 = p0.x, y0 = p0.y;
  double x1 = p1.x, y1 = p1.y;
  double x2 = p2.x, y2 = p2.y;

  double a = -x0 / 2.0 - x1 / 3.0 - x2 / 6.0;
  double b = x0 / 3.0 - x2 / 3.0;
  double c = x0 / 6.0 + x1 / 3.0 + x2 / 2.0;

  return a * y0 + b * y1 + c * y2;
}
LC_AreaMoments LC_Parabola::momentLineIntegrals(const RS_Vector& origin) const {
  // The parabola stores its three control points in world coordinates. The
  // six Green's theorem integrands are polynomials of degree ≤ 7 in the
  // Bezier parameter, so the 8-node Gauss-Legendre rule inside
  // quadraticBezier is exact.
  if (!m_data.m_valid) {
    return {};
  }
  const auto& cp = m_data.m_controlPoints;
  return LC_MomentQuadrature::quadraticBezier(cp[0], cp[1], cp[2], origin);
}

double LC_Parabola::getLength() const
{
  if (!m_data.m_valid)
    return 0.0;
  double h = m_data.m_axis.magnitude();
  if (h < RS_TOLERANCE)
    return 0.0;

  double x1 = m_data.findX(getStartpoint());
  double x2 = m_data.findX(getEndpoint());

  double xmin = std::min(x1, x2);
  double xmax = std::max(x1, x2);

  auto F = [h](double x) -> double {
    double sqrt_term = std::sqrt(x * x + 4 * h * h);
    return (x / (4. * h)) * sqrt_term + h * std::log(x + sqrt_term);
  };

  return F(xmax) - F(xmin);
}
