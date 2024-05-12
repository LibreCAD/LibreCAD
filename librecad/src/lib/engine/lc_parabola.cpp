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

#include "lc_quadratic.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

namespace {

// convert the parabola data to SplinePoint data
LC_SplinePointsData convert2SplineData(const LC_ParabolaData& data)
{
    LC_SplinePointsData splineData{};
    splineData.controlPoints = {data.controlPoints.cbegin(), data.controlPoints.cend()};
    // spline points are probably not used
    RS_Vector sp1 = (data.controlPoints.front() + data.controlPoints.back())*0.25 + data.controlPoints.at(1)*0.5;
    splineData.splinePoints = {data.controlPoints.front(), sp1, data.controlPoints.back()};
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
    // rotate y-axis to axis around points.front()
    const auto& rCenter = points.front();
    std::array<RS_Vector, 4> rotated;
    std::transform(points.cbegin(), points.cend(), rotated.begin(),
                   [&rCenter, da=RS_Vector{M_PI/2 - axis.angle()}](const RS_Vector& vp){
        RS_Vector ret = vp;
        return ret.rotate(rCenter, da);
    });
    std::sort(rotated.begin(), rotated.end(), [](const RS_Vector& p0, const RS_Vector& p1) {
        return p0.x < p1.x;
    });
    // y = a*x^2 + b*x + c
    // (y(x2) - y(x1))/(x2 - x1) = a*(x2 + x1) + b
    double sxi2=0., sxi=0., sxyi=0., syi=0.;
    std::vector<double> xis,yis;
    for (size_t i = 1; i < rotated.size(); ++i) {
        double xi = rotated[i].x + rotated.front().x;
        //assert(std::abs(rotated[i].x - rotated.front().x) > RS_TOLERANCE);
        if (std::abs(rotated[i].x - rotated.front().x) <= RS_TOLERANCE)
            continue;
        double yi = (rotated[i].y - rotated.front().y)/(rotated[i].x - rotated.front().x);
        sxi2 += xi * xi;
        sxi += xi;
        sxyi += xi * yi;
        syi += yi;
        xis.push_back(xi);
        yis.push_back(yi);
    }
    // least-square
    const double d = sxi2*xis.size() - sxi*sxi;
    if (std::abs(d) < RS_TOLERANCE)
    {
        //assert(!"least-square failure for Parabola");
        return {};
    }
    const double a = (sxyi*xis.size() - syi*sxi)/d;
    if (std::abs(a) < RS_TOLERANCE2)
    {
        return {};
    }
    const double b = (sxi2*syi - sxi*sxyi)/d;
    //LC_ERR <<" axis angle: "<<axis.angle();
    for (size_t i=0; i< xis.size(); i++)
    {
        LC_LOG<<__func__<<"(): fitting: xi = "<<xis[i]<<": "<<yis[i] - (a*xis[i] + b);
    }
    double c = 0.;
    for (const auto& point: rotated)
    {
        c += point.y - point.x * (b + point.x * a);
    }
    c /= 4;
    double da = {axis.angle() - M_PI/2};
    auto f0 = [&a, &b, &c, &rCenter, &da](const RS_Vector& pt) {
        double x = RS_Vector{pt}.rotate(rCenter, -da).x;
        double y = c + x*(b + a*x);
        return RS_Vector{x, y}.rotate(rCenter, da);
    };

    // fitting errors
    double ds2=0.;
    for (const auto& point: points)
    {
        double ds= (point - f0(point)).squared();
        ds2 += ds;
        LC_LOG<<"oxi = ("<<point.x<<", "<< point.y<<"): ("<<f0(point).x<<", "<<f0(point).y<<"): dr="
             << ds<<": "<<ds2;
    }
    if (ds2 >= RS_TOLERANCE2)
        return {};
    // vertex: y=c + bx + ax^2=c-b^2/(4a) + a(x+b/(2a))^2, {-b/(2a), c-b^2/(4a)}
    // a=1/(4h), h = 0.25/a

    auto f1 = [&a, &b, &da](double x) {
        return RS_Vector{1., 2.*a*x +b}.rotate(da);
    };
    std::array<RS_Vector, 2> tangents = {{f1(rotated.front().x), f1(rotated.back().x)}};

    auto ret = LC_ParabolaData::FromEndPointsTangents(
        {rotated.front().rotate(rCenter, da), rotated.back().rotate(rCenter, da)},
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
    std::vector<RS_Vector> axes = getAxisVectors(points);

    std::vector<LC_ParabolaData> ret;
    std::transform(axes.cbegin(), axes.cend(), std::back_inserter(ret), [&points](const RS_Vector& axis) {
        return fromPointsAxis(points, axis);
    });
    ret.erase(std::remove_if(ret.begin(), ret.end(), [](const LC_ParabolaData& d){return !d.valid;}), ret.end());
    return ret;
}

LC_ParabolaData::LC_ParabolaData(std::array<RS_Vector, 3> controlPoints):
    controlPoints{std::move(controlPoints)}
  , valid{true}
{
    CalculatePrimitives();
}

void LC_ParabolaData::CalculatePrimitives()
{
    // shift the first control point to origin
    // After shifting the parabola is: 2 t (1-t) c1 + t^2 c2
    // the tangent at start point: 2 c1
    // the tangent: 2(1-2t) c1 + 2t c2

    // control points
    RS_Vector c1 = controlPoints.at(1) - controlPoints.at(0);
    RS_Vector c2 = controlPoints.at(2) - controlPoints.at(0);
    // The parabola
    auto f0 = [&c1, &c2](double t) {
        return c1*(2. * t * (1. - t)) + c2 * ( t * t);
    };

    axis = c2 * 0.5 - c1;
    if(axis.squared() < RS_TOLERANCE2) {
        valid = false;
        return;
    }
    // <c2 * 0.5 - c1 | c1 * (2. - 4. * t) + c2 * 2.*t> = 0
    // <c1|c2>-2<c1|c1> = (2<c1|c2>-4<c1|c1>-<c2|c2>+2<c1|c2>)t
    // =-|c2 - 2c1|^2t
    double t = -0.5*axis.dotP(c1)/axis.squared();
    auto localVertex = f0(t);
    axis.normalize();
    double dy = (c2 - localVertex).dotP(axis);
    double dx = (c2 - localVertex).dotP({- axis.y, axis.x});
    // dy = dx^2/(4h) , h = dx^2/(4dy)
    const double h = dx*dx/(4.*dy);
    vertex = localVertex + controlPoints.front();
    axis *= h;
    focus = vertex + axis;
}

RS_LineData LC_ParabolaData::GetAxis() const
{
    const auto vp = (controlPoints.front() + controlPoints.back())*0.5 - vertex;
    return {vertex, vertex + axis*(0.5*vp.dotP(axis)/axis.squared())};
}

double LC_ParabolaData::FindX(const RS_Vector& point) const
{
    // in regular coordinates (4hy=x^2)
    const auto vp = RS_Vector{point}.rotate(vertex, M_PI/2 - axis.angle()) - vertex;
    return vp.x;
}

RS_Vector LC_ParabolaData::FromX(double x) const
{
    // in regular coordinates (4hy=x^2)
    auto vp = RS_Vector{x, x*x/(4.*axis.magnitude())}.rotate(axis.angle() - M_PI/2) + vertex;
    return vp;
}

/** \brief return the equation of the entity
a quadratic contains coefficients for quadratic:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
LC_Quadratic LC_ParabolaData::getQuadratic() const
{
    if (!valid)
        return LC_Quadratic{};
    std::vector<double> ce(6, 0.);
    ce[0] = 1.;
    ce[4] = -4. * axis.magnitude();
    LC_Quadratic lq{ce};
    lq.rotate(axis.angle() - M_PI/2);
    lq.move(vertex);
    return lq;
}

LC_Parabola::LC_Parabola(RS_EntityContainer* parent, const LC_ParabolaData& d):
    LC_SplinePoints{parent, convert2SplineData(d)}
  , data{d}
{
}

RS_Entity* LC_Parabola::clone() const
{
    auto* e = new LC_Parabola(*this);
    e->initId();
    return e;
}


RS2::EntityType LC_Parabola::rtti() const
{
    return RS2::EntityParabola;
}

LC_Quadratic LC_Parabola::getQuadratic() const
{
    return data.getQuadratic();
}

RS_Vector LC_Parabola::getTangentDirection(const RS_Vector& point)const
{
    RS_VectorSolutions sol = getTangentPoint(point);
    if(sol.empty())
        return {};
    RS_Vector tangentPoint = getTangentPoint(point).at(0);
    RS_Vector p0 = rotateToQuadratic(tangentPoint) - data.vertex;
    return RS_Vector{2.*data.axis.magnitude(), p0.x}.rotate(data.axis.angle() - M_PI/2).normalized();
}

RS_VectorSolutions LC_Parabola::getTangentPoint(const RS_Vector& point) const
{
    RS_Vector p0 = rotateToQuadratic(point) - data.vertex;
    // y=x^2/(4h)
    // (x^2/(4h) - py) = x/(2h)(x - px)
    // x^2/(4h) - px/(2h) x + py = 0
    // (x - px)^2 = px^2 - 4h py
    // x = px \pm \sqrt(px^2 - 4h py) ; py <= px^2/(4h)
    const double h = data.axis.magnitude();
    if (4.0*h*p0.y >= p0.x * p0.x + RS_TOLERANCE)
        return {};
    auto pf = [this, &h](double x){
        return RS_Vector{x, x*x/(4.*h)}.rotate(data.axis.angle() - M_PI/2.) + data.vertex;
    };
    double dx = std::sqrt(std::abs(p0.x*p0.x - 4. * h * p0.y));
    if (dx <= RS_TOLERANCE)
        return {point};
    return {pf(p0.x + dx), pf(p0.x - dx)};
}

RS_Vector LC_Parabola::dualLineTangentPoint(const RS_Vector& line) const
{
    // rotate to regular form
    auto uv = RS_Vector{line}.rotate(M_PI/2. - data.axis.angle());
    // slope = {2h, x} <(2h,x)|uv> = 0
    // x=-2h uv.x/(uv.y)
    if (std::abs(uv.y) < RS_TOLERANCE)
        return RS_Vector{false};
    return data.FromX(-2.*data.axis.magnitude()*uv.x/uv.y);
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
    return vp.rotate(data.vertex, M_PI/2 - data.axis.angle());
}

void LC_Parabola::LC_Parabola::moveStartpoint(const RS_Vector& pos)
{
    RS_Vector p0=getNearestPointOnEntity(pos);
    RS_Vector t0=getTangentDirection(p0);
    auto t1 = RS_Vector{getDirection2()};
    data = LC_ParabolaData::FromEndPointsTangents(
                { p0, data.controlPoints.back() },
                { t0, t1}
                );
    LC_SplinePoints::getData() = convert2SplineData(data);
    calculateBorders();
}

void LC_Parabola::LC_Parabola::moveEndpoint(const RS_Vector& pos)
{
    auto t0 = RS_Vector{getDirection1()};
    RS_Vector p2=getNearestPointOnEntity(pos);
    RS_Vector t2=RS_Vector{getTangentDirection(p2)};
    data = LC_ParabolaData::FromEndPointsTangents(
                { data.controlPoints.front(), p2 },
                { t0, t2}
                );
    LC_SplinePoints::getData() = convert2SplineData(data);
    calculateBorders();
}

double LC_Parabola::getDirection1() const
{
    return (data.controlPoints.at(1) - data.controlPoints.front()).angle();
}

double LC_Parabola::getDirection2() const
{
    return (data.controlPoints.back() - data.controlPoints.at(1)).angle();
}
RS_VectorSolutions LC_Parabola::getRefPoints() const
{
    return {data.controlPoints.front(), data.controlPoints.at(1), data.controlPoints.back()};
}


void LC_Parabola::move(const RS_Vector& offset)
{
    for(auto& point: data.controlPoints)
        point.move(offset);
    update();
}
void LC_Parabola::rotate(const RS_Vector& center, const double& angle)
{
    for(auto& point: data.controlPoints)
        point.rotate(center, angle);
    update();
}
void LC_Parabola::rotate(const RS_Vector& center, const RS_Vector& angleVector)
{
    for(auto& point: data.controlPoints)
        point.rotate(center, angleVector);
    update();
}
void LC_Parabola::scale(const RS_Vector& center, const RS_Vector& factor)
{
    for(auto& point: data.controlPoints)
        point.scale(center, factor);
    update();
}
void LC_Parabola::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2)
{
    for(auto& point: data.controlPoints)
        point.mirror(axisPoint1, axisPoint2);
    update();
}

RS_Entity& LC_Parabola::shear(double k)
{
    for(auto& point: data.controlPoints)
        point.shear(k);
    update();
    return *this;
}

void LC_Parabola::moveRef(const RS_Vector& ref, const RS_Vector& offset)
{
    for(auto& point: data.controlPoints)
        if (point.squaredTo(ref) < RS_TOLERANCE2) {
            point.move(offset);
            break;
        }

    update();
}

void LC_Parabola::revertDirection()
{
    std::swap(data.controlPoints.front(), data.controlPoints.back());
}

void LC_Parabola::LC_Parabola::update()
{
    data.CalculatePrimitives();
    LC_SplinePoints::getData() = convert2SplineData(data);
    calculateBorders();
}

RS_Vector LC_Parabola::getNearestOrthTan([[maybe_unused]] const RS_Vector& coord,
                                        const RS_Line& normal,
                                        bool onEntity ) const
{
    // transform to regular form: 4hy=x^2
    auto line = RS_Vector{ normal.getDirection1() }.rotate(M_PI/2 - data.axis.angle());
    // parabola tangent: <{2.*h, x}|line>=0, x=line.y/(2h line.x)>
    if (onEntity) {
        const double x0 = data.FindX(getStartpoint());
        const double x1 = data.FindX(getEndpoint());
        if (std::signbit(line.x - x0) == std::signbit(line.x - x1))
            return RS_Vector{false};
    }
    if (std::abs(line.y) < RS_TOLERANCE)
        return RS_Vector{false};
    const double x = -2.*data.axis.magnitude()*line.x/line.y;
    return RS_Vector{x, x*x/(4.*data.axis.magnitude())}.rotate(data.axis.angle() - M_PI/2) + data.vertex;
}

// void LC_Parabola::LC_Parabola::draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset)
// {
//     for (size_t i=0; i<2; ++i){
//         RS_Line l0{nullptr, {data.controlPoints.at(i), data.controlPoints.at(i+1)}};
//         l0.draw(painter, view, patternOffset);
//     }
//     LC_SplinePoints::draw(painter, view, patternOffset);
// }

