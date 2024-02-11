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
        assert(!"tangent direction cannot be parallel");
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
        return {};
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

    RS_Vector p1 = getP1(data) - data.startPoint;
    RS_Vector p2 = data.endPoint - data.startPoint;
    // The parabola
    auto f0 = [ct=p1, st=p2](double t) {
        return ct*(2. * t * (1. - t)) + st * ( t * t);
    };
    // actually half of df0/dt
    auto f1 = [ct=p1, st=p2](double t) {
        return ct * (1. - 2. * t) + st * t;
    };

    double t = 0.;
    double t0 = 0.;
    if (std::abs(2.*p1.squared() - p1.dotP(p2)) < RS_TOLERANCE) {
        // won't be able to find an orthogonal at the start point, try the middle point t=0.5
        // <P2|(1-2t)P1 + t P2>=0
        if (std::abs(2. * p1.dotP(p2) - p2.squared()) > RS_TOLERANCE) {
            t = p1.dotP(p2)/(2. * p1.dotP(p2) - p2.squared());
            t0 = 0.5;
        } else {
            // try the end point, t = 1
            // < - P1 + P2 | (1-2t)P1 + t P2> = - |P1|^2 + (1-3t)<P1|P2> + t |P2|^2 = 0
            t = (p1.squared() - p1.dotP(p2)) /(p2.squared() - 3. * p1.dotP(p2));
            t0 = 1.;
        }
    } else {
        // start point the value of t when the tangent is normal to P1: <P1|(1-2t)P1 + t P2> = 0
        t = p1.squared()/(2. * p1.squared() - p1.dotP(p2));
    }

    // The point chosen for the first tangent
    p1 = f0(t0);
    RS_Vector v1 = f1(t0).normalized();
    // The point for the orthogonal tangent
    p2 = f0(t);
    RS_Vector v2 = f1(t).normalized();

    // Difference of the two points
    RS_Vector dP = p2 - p1;
    double cs = dP.normalized().dotP(v1);

    // The focus
    RS_Vector pF = p1 + dP * (cs * cs);
    // Reflection of the focus around tangent lines:
    // p1
    RS_Vector directrix1 = pF.mirror(p1, p1 + v1);
    RS_Vector directrix2 = pF.mirror(p2, p2 + v2);

    return {
        pF + data.startPoint,
        {
            directrix1 + data.startPoint,
            directrix2 + data.startPoint
        }
    };
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
        if (v0.x + RS_TOLERANCE < v1.x)
            return true;
        if (v0.x - RS_TOLERANCE > v1.x)
            return false;
        if (v0.y + RS_TOLERANCE < v1.y)
            return true;
        return (v0.y - RS_TOLERANCE > v1.y);
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
For the convex case, i.e., when none of the four points A,B,C,D is inside the triangle formed by the other three, we can construct (by Euclidean methods) lines parallel to the axes of symmetry of the two parabolas as follows:
-Let e and f denote the feet of the perpendiculars to CD from A and B respectively.

-Let g denote the intersection of the line through D perpendicular to CD, and the line through A perpendicular to AC.

-Let h denote the intersection of the line Dg and the line through B perpendicular to BC.

-Let i denote the intersection of the line Ae and the line through h parallel to Ag.

-Let j denote the intersection of the line Ae and the line through B parallel to Ae.

-Let k denote the intersection of the line Ae and the line through B parallel to AB.

-Locate the point L on Ae such that the segment kL has the same length and direction as the segment Aj.

-Draw the circle Q whose diameter is the segment iL.

-Let m and n denote the intersections of the line through k perpendicular to Ae with the circle Q.

-Let o denote the intersection of the line Bf with the line through L perpendicular to Ae.

-The lines mo and no are parallel to the axes of symmetry of the two parabolas through A, B, C, and D.
     */
    // validate also reorder points to convex points
    if (!validateConvexPoints(pts)) {
        //assert(!"Parasolid points must be convex");
        return {};
    }
    //pts ABCD
    for(const auto& pt: pts)
        LC_ERR<<"pts: "<<pt.x<<", "<<pt.y;

    RS_Line ab{nullptr, {pts[0], pts[1]}};
    RS_Line cd{nullptr, {pts[2], pts[3]}};
    RS_Vector pe = cd.getNearestPointOnEntity(pts[0], false);
    if (!pe.valid)
        return {};
    LC_ERR<<"pe: "<<pe.x<<", "<<pe.y;
    //RS_Vector pf = cd.getNearestPointOnEntity(pts[1], false);
    RS_Vector pg = getOppositePoint(pts[0], pts[2], pts[3]);
    if (!pg.valid)
        return {};
    LC_ERR<<"pg: "<<pg.x<<", "<<pg.y;
    RS_Vector ph = getOppositePoint(pts[1], pts[2], pts[3]);
    if (!ph.valid)
        return {};
    RS_Vector pi = ph + pts[0] - pg;
    LC_ERR<<"pi: "<<pi.x<<", "<<pi.y;
    RS_Line ae {nullptr, {pts[0], pe}};
    RS_Vector pj = ae.getNearestPointOnEntity(pts[1], false);
    LC_ERR<<"pj: "<<pj.x<<", "<<pj.y;
    RS_Line bk {nullptr, {pts[1], pts[1] + (pts[0] - pts[1]).rotate(M_PI/2.)}};
    RS_Vector pk = RS_Information::getIntersection(&bk, &ae, false).at(0);
    LC_ERR<<"pk: "<<pk.x<<", "<<pk.y;
    RS_Vector pl = pk + pj - pts[0];
    LC_ERR<<"pl: "<<pl.x<<", "<<pl.y;
    RS_Circle circle{nullptr, {(pi+pl)*0.5, (pl - pi).magnitude()*0.5}};
    RS_Vector po = pl + pts[1] - pj;
    LC_ERR<<"po: "<<po.x<<", "<<po.y;
    RS_Line km {nullptr, {pk, pk + ae.getTangentDirection({}).rotate(M_PI/2)}};
    RS_VectorSolutions sol = RS_Information::getIntersection(&circle, &km, false);
    //assert(sol.size() == 2);
    if (sol.size() != 2)
        return {};
    return {sol.at(0) - po, sol.at(1) - po};
}

LC_ParabolaData fromPointsAxis(const std::vector<RS_Vector>& points, const RS_Vector& axis)
{
    // rotate y-axis to axis
    std::array<RS_Vector, 4> rotated;
    std::transform(points.cbegin(), points.cend(), rotated.begin(), [da=RS_Vector{axis.angle() - M_PI/2}](const RS_Vector& p0){
        RS_Vector p1 = p0;
        return p1.rotate(da);
    });
    std::sort(rotated.begin(), rotated.end(), [](const RS_Vector& p0, const RS_Vector& p1) {
        return p0.x + RS_TOLERANCE < p1.x;
    });
    // y = a*x^2 + b*x + c
    // (y(x2) - y(x1))/(x2 - x1) = a*(x2 + x1) + b
    double sxi2=0., sxi=0., sxyi=0., syi=0.;
    for (size_t i = 1; i < rotated.size(); ++i) {
        double xi = rotated[i].x + rotated.front().x;
        double yi = (rotated[i].y - rotated.front().y)/(rotated[i].x - rotated.front().x);
        sxi2 += xi * xi;
        sxi += xi;
        sxyi += xi * yi;
        syi += yi;
    }
    // least-square
    const double d = sxi2*3 - sxi*sxi;
    if (std::abs(d) < RS_TOLERANCE)
    {
        assert(!"least-square failure for Parabola");
        return {};
    }
    const double a = (sxyi*3 - syi*sxi)/d;
    if (std::abs(a) < RS_TOLERANCE2)
    {
        assert(!"quadratic factor is 0 for parabola");
        return {};
    }
    const double b = (sxi2*syi - sxi*sxyi)/d;
    RS_Vector da{M_PI/2. - axis.angle()};
    auto f1 = [&a, &b, &da](double x) {
        return RS_Vector{1., 2.*a*x +b}.rotate(da);
    };

    return LC_ParabolaData::FromEndPointsTangents(
        {rotated.front().rotate(da), rotated.back().rotate(da)},
        {f1(rotated.front().x), f1(rotated.back().x)}
    );
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
    const auto [focus, directrix] = getFocusDirectrix(*this);
    RS_Vector p0 = RS_Line{nullptr, directrix}.getNearestPointOnEntity(focus, false);
    LC_ERR<<"Axis: angle = "<<(p0 - focus).angle()*180./M_PI;
    return {p0*4.0 - focus*3., focus*4. - p0*3.};
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



