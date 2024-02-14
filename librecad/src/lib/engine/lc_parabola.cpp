/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2014 Dongxu Li (dongxuli2011@gmail.com)
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

#include <QPainterPath>
#include <QPolygonF>
#include "lc_parabola.h"

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_graphic.h"
#include "rs_painterqt.h"
#include "lc_quadratic.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_linetypepattern.h"

namespace {
LC_SplinePointsData convert2SplineData(const LC_ParabolaData& data)
{
    LC_SplinePointsData splineData{};
    RS_Line tangent0{nullptr, {data.startPoint, data.startPoint + data.startTangent}};
    RS_Line tangent1{nullptr, {data.endPoint, data.endPoint + data.endTangent}};
    RS_VectorSolutions intersection = RS_Information::getIntersectionLineLine(&tangent0, &tangent1);
    if (intersection.empty()) {
        //assert(!"tangent direction cannot be parallel");
        return {};
    }
    RS_Vector pointP1 = (data.startPoint + data.endPoint) * 0.25 + intersection.at(0) * 0.5;
    splineData.splinePoints = {{data.startPoint, pointP1, data.endPoint}};
    splineData.closed = false;
    return splineData;
}

RS_Vector getP1(const LC_ParabolaData& data)
{
    RS_Line l0{nullptr, {data.startPoint, data.startPoint + data.startTangent}};
    RS_Line l1{nullptr, {data.endPoint, data.endPoint + data.endTangent}};
    auto sol = RS_Information::getIntersection(&l0, &l1, false);
    if (sol.empty())
    {
        return RS_Vector{false};
    }
    return sol.at(0);
}

// recover the focus and directrix from data
std::pair<RS_Vector, RS_LineData> getFocusDirectrix(const LC_ParabolaData& data)
{
    // shift data.startPoint to origin
    // After shifting the parabola is: 2 t (1-t) P1 + t^2 P2
    // the tangent at start point: 2 P1
    // the tangent: 2(1-2t) P1 + 2t P2

    // control points
    RS_Vector controlP1 = getP1(data);
    if (!controlP1.valid)
        return {};
    RS_Vector c1 = controlP1 - data.startPoint;
    RS_Vector c2 = data.endPoint - data.startPoint;
    // The parabola
    auto f0 = [&c1, &c2](double t) {
        return c1*(2. * t * (1. - t)) + c2 * ( t * t);
    };
    // actually half of df0/dt
    auto f1 = [&c1, &c2](double t) {
        return c1 * (2. - 4. * t) + c2 * 2.*t;
    };

    auto axis = c2 * 0.5 - c1;
    // <c2 * 0.5 - c1 | c1 * (2. - 4. * t) + c2 * 2.*t> = 0
    // <c1|c2>-2<c1|c1> = (2<c1|c2>-4<c1|c1>-<c2|c2>+2<c1|c2>)t
    // =-|c2 - 2c1|^2t
    double t = -0.5*axis.dotP(c1)/axis.squared();
    LC_ERR<<"vertex = "<<f0(t).x<<f0(t).y<<" : "<<f1(t).dotP(axis);
    auto vertex = f0(t);
    axis.normalize();
    double dy = (c2 - vertex).dotP(axis);
    double dx = (c2 - vertex).dotP({- axis.y, axis.x});
    // dy = dx^2/(4h) , h = dx^2/(4dy)
    const double h = dx*dx/(4.*dy);
    vertex = vertex + data.startPoint;
    axis = axis *h;
    auto focus = vertex + axis;
    vertex -= axis;
    axis.rotate(M_PI/2);
    RS_LineData directrix{vertex, vertex + axis};
    return {focus, directrix};
}

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

RS_Vector getOppositePoint(const RS_Vector& a, const RS_Vector& b, const RS_Vector& c)
{
    RS_Line l0{nullptr, {a, a + (a-b).rotate(M_PI/2.)}};
    RS_Line l1{nullptr, {c, c + (c-b).rotate(M_PI/2.)}};
    RS_VectorSolutions sol = RS_Information::getIntersection(&l0, &l1, false);
    if (sol.empty()) {
        //assert(!"Parasolid points cannot be colinear");
        return {};
    }
    return sol.at(0);
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
    for(const auto& pt: pts)
        LC_ERR<<"pts: "<<pt.x<<", "<<pt.y;

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
    LC_ERR<<"po: "<<po.x<<", "<<po.y;
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
    // rotate y-axis to axis
    std::array<RS_Vector, 4> rotated;
    std::transform(points.cbegin(), points.cend(), rotated.begin(), [da=RS_Vector{M_PI/2 - axis.angle()}](const RS_Vector& p0){
        RS_Vector p1 = p0;
        return p1.rotate(da);
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
        assert(!"quadratic factor is 0 for parabola");
        return {};
    }
    const double b = (sxi2*syi - sxi*sxyi)/d;
    LC_ERR <<" axis angle: "<<axis.angle();
    for (size_t i=0; i< xis.size(); i++)
    {
        LC_ERR<<"xi = "<<xis[i]<<": "<<yis[i] - (a*xis[i] + b);
    }
    double c = 0.;
    for (size_t i=0; i< 4; i++)
    {
        c += rotated[i].y - rotated[i].x * (b + rotated[i].x * a);
        //LC_ERR<<"rxi = "<<rotated[i].x<<": "<<rotated[i].y - rotated[i].x * (b + rotated[i].x * a);
    }
    c /= 4;
    double da = {axis.angle() - M_PI/2};
    auto f0 = [&a, &b, &c, &da](const RS_Vector& pt) {
        double x = RS_Vector{pt}.rotate(-da).x;
        double y = c + x*(b + a*x);
        return RS_Vector{x, y}.rotate(da);
    };
    for (size_t i=0; i< 4; i++)
    {
        LC_ERR<<"oxi = ("<<points[i].x<<", "<< points[i].y<<"): ("<<f0(points[i]).x<<", "<<f0(points[i]).y<<"): dr="
             << (points[i] - f0(points[i])).magnitude();
    }
    // vertex: y=c + bx + ax^2=c-b^2/(4a) + a(x+b/(2a))^2, {-b/(2a), c-b^2/(4a)}
    // a=1/(4h), h = 0.25/a

    auto f1 = [&a, &b, &da](double x) {
        return RS_Vector{1., 2.*a*x +b}.rotate(da);
    };
    std::array<RS_Vector, 2> tangents = {{f1(rotated.front().x), f1(rotated.back().x)}};

    auto ret = LC_ParabolaData::FromEndPointsTangents(
        {rotated.front().rotate(da), rotated.back().rotate(da)},
        tangents
    );
    ret.axis = axis.normalized();
    ret.vertex = RS_Vector{-0.5*b/a, c-0.25*b*b/a}.rotate(da);
    ret.focus = ret.vertex + ret.axis *(0.25/a);
    ret.p1 = getP1(ret);

    // auto axisNew = ret.GetAxis();
    // double angleNew = (axisNew.endpoint - axisNew.startpoint).angle();
    // double angleOld = axis.angle();
    // double dangle = std::abs(std::remainder(angleOld - angleNew, M_PI));
    //assert( dangle < RS_TOLERANCE_ANGLE);
    return ret;
}

}

LC_ParabolaData LC_ParabolaData::FromEndPointsTangents(
           const std::array<RS_Vector, 2>& endPoints,
           const std::array<RS_Vector, 2>& endTangents)
{
    LC_ParabolaData data{};
    data.startPoint = endPoints.front();
    data.endPoint = endPoints.back();
    data.startTangent = endTangents.front();
    data.endTangent = endTangents.back();
    data.p1 = getP1(data);
    auto axis = data.p1 - (endPoints.front() + endPoints.back())*0.5;
    // 2(t-1)(p0-p1)+2t(p2-p1)= 2p1 +2t(p0+p2) - 2p0
    return data;
}

std::vector<LC_ParabolaData> LC_ParabolaData::From4Points(const std::vector<RS_Vector>& points)
{
    std::vector<RS_Vector> axes = getAxisVectors(points);

    std::vector<LC_ParabolaData> ret;
    std::transform(axes.cbegin(), axes.cend(), std::back_inserter(ret), [&points](const RS_Vector& axis) {
        return fromPointsAxis(points, axis);
    });
    return ret;
}

RS_LineData LC_ParabolaData::GetAxis() const
{
/*    {
         const auto [focus, directrix] = getFocusDirectrix(FromEndPointsTangents(
                                                               {{{0., 1.}, {2.,1}}},
                                                               {{{1., -2.}, {1., 2.}}}));
         RS_Vector p0 = RS_Line{nullptr, directrix}.getNearestPointOnEntity(focus, false);
         LC_ERR<<"Axis: angle = "<<(p0 - focus).angle()*180./M_PI;
    }*/
    return {focus, vertex*2. - focus};
}


LC_Parabola::LC_Parabola(RS_EntityContainer* parent, const LC_ParabolaData& d):
    LC_SplinePoints{parent, convert2SplineData(d)}
  , data{d}
{
}

RS2::EntityType LC_Parabola::rtti() const
{
    return RS2::EntityParabola;
}



