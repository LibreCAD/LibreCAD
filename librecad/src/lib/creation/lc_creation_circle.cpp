/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_creation_circle.h"

#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_information.h"
#include "rs_line.h"

namespace {
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

    bool testTan3(const std::vector<RS_AtomicEntity*>& circles, const RS_Circle& circle) {
        if (circles.size() != 3) {
            return false;
        }
        for (const auto& c : circles) {
            if (!c->isTangent(circle.getData())) {
                return false;
            }
        }
        return true;
    }

    /** solve one of the eight Appollonius Equations
    | Cx - Ci|^2=(Rx+Ri)^2
    with Cx the center of the common tangent circle, Rx the radius. Ci and Ri are the Center and radius of the i-th existing circle
    **/
    std::vector<RS_Circle> solveCircleApolloniusSingle(const std::vector<RS_Circle>& circles) {
        //          std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
        //          for(int i=0;i<circles.size();i++){
        //std::cout<<"i="<<i<<"\t center="<<circles[i].getCenter()<<"\tr="<<circles[i].getRadius()<<std::endl;
        //          }

        std::vector<RS_Vector> centers;
        std::vector<double> radii;

        for (const RS_Circle& c : circles) {
            if (!c.getCenter().valid) {
                return {};
            }
            centers.push_back(c.getCenter());
            radii.push_back(c.getRadius());
        }
        //              for(int i=0;i<circles.size();i++){
        //    std::cout<<"i="<<i<<"\t center="<<circles[i].getCenter()<<"\tr="<<radii.at(i)<<std::endl;
        //              }
        /** form the linear equation to solve center in radius **/
        std::vector<std::vector<double>> mat(2, std::vector<double>(3, 0.));
        mat[0][0] = centers[2].x - centers[0].x;
        mat[0][1] = centers[2].y - centers[0].y;
        mat[1][0] = centers[2].x - centers[1].x;
        mat[1][1] = centers[2].y - centers[1].y;

        // Issue #2160: this algebraic algorithm fails when input circle centers are identical in
        // x-coordinates or y-coordinates
        LC_LOG << __func__ << "(): identicalXOrY=" << identicalXOrY(mat);
        if (identicalXOrY(mat) || std::abs((mat[0][0] * mat[1][1]) - (mat[0][1] * mat[1][0])) < RS_TOLERANCE15) {
            return {};
        }
        // r^0 term
        mat[0][2] = 0.5 * (centers[2].squared() - centers[0].squared() + radii[0] * radii[0] - radii[2] * radii[2]);
        mat[1][2] = 0.5 * (centers[2].squared() - centers[1].squared() + radii[1] * radii[1] - radii[2] * radii[2]);
        //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
        //    for(unsigned short i=0;i<=1;i++){
        //        std::cout<<"eqs P:"<<i<<" : "<<mat[i][0]<<"*x + "<<mat[i][1]<<"*y = "<<mat[i][2]<<std::endl;
        //    }
        //    std::vector<std::vector<double> > sm(2,std::vector<double>(2,0.));
        std::vector<double> sm(2, 0.);
        if (!RS_Math::linearSolver(mat, sm)) {
            return {};
        }

        const RS_Vector vp(sm[0], sm[1]);
        //      std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
        //      std::cout<<"vp="<<vp<<std::endl;

        // r term
        mat[0][2] = radii[0] - radii[2];
        mat[1][2] = radii[1] - radii[2];
        //    for(unsigned short i=0;i<=1;i++){
        //        std::cout<<"eqs Q:"<<i<<" : "<<mat[i][0]<<"*x + "<<mat[i][1]<<"*y = "<<mat[i][2]<<std::endl;
        //    }
        if (!RS_Math::linearSolver(mat, sm)) {
            return {};
        }
        const RS_Vector vq(sm[0], sm[1]);
        //      std::cout<<"vq="<<vq<<std::endl;
        //form quadratic equation for r
        const RS_Vector dcp = vp - centers[0];
        const double a = vq.squared() - 1.;
        if (std::abs(a) < RS_TOLERANCE * 1e-4) {
            return {};
        }
        std::vector<double> ce(0, 0.);
        ce.push_back(2. * (dcp.dotP(vq) - radii[0]) / a);
        ce.push_back((dcp.squared() - radii[0] * radii[0]) / a);
        const std::vector<double> vr = RS_Math::quadraticSolver(ce);
        std::vector<RS_Circle> ret;
        for (const double dist : vr) {
            if (dist >= RS_TOLERANCE) {
                ret.emplace_back(RS_Circle(nullptr, {vp + vq * dist, std::abs(dist)}));
            }
        }
        //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
        //    std::cout<<"Found "<<ret.size()<<" solutions"<<std::endl;

        return ret;
    }

    /**
    * @brief solveApolloniusHyperbola a more generic solution based on hyperbola intersections.
    *        this algorithm is likely worse in precision compared with solveAolloniusSingle().
    *        Provided as a backup when solveAolloniusSingle() fails.
    * @param circles - the three input circles
    * @return candidates circles
    */
    std::vector<RS_Circle> solveCircleApolloniusHyperbola(const std::vector<RS_Circle>& circles) {
        assert(circles.size() == 3);
        std::vector<RS_Vector> centers;
        std::vector<double> radii;

        for (const RS_Circle& c : circles) {
            if (!c.getCenter().valid) {
                return {};
            }
            centers.push_back(c.getCenter());
            radii.push_back(c.getRadius());
        }
        const size_t i0 = (centers[0] == centers[1] || centers[0] == centers[2]) ? 1 : 0;

        std::vector<RS_Circle> ret;
        const LC_Quadratic lc0(&(circles[i0]), &(circles[(i0 + 1) % 3]));
        const LC_Quadratic lc1(&(circles[i0]), &(circles[(i0 + 2) % 3]));
        RS_VectorSolutions c0 = LC_Quadratic::getIntersection(lc0, lc1);
        for (const auto& i : c0) {
            const double dc = i.distanceTo(centers[i0]);
            ret.push_back(RS_Circle(nullptr, {i, std::abs(dc - radii[i0])}));
            if (dc > radii[i0]) {
                ret.push_back(RS_Circle(nullptr, {i, dc + radii[i0]}));
            }
        }
        return ret;
    }
}

/**
 * Creates this circle from a center point and a radius.
 *
 * @param c Center.
 * @param r Radius
 */
bool LC_CreationCircle::createFromCR(const RS_Vector& c, const double r, RS_CircleData& data) {
    if (std::abs(r) > RS_TOLERANCE && c.valid) {
        data.radius = std::abs(r);
        data.center = c;
        return true;
    }
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFromCR(): " "Cannot create a circle with radius 0.0.");
    return false;
}

/**
 * Creates this circle from two opposite points.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 */
bool LC_CreationCircle::createFrom2P(const RS_Vector& p1, const RS_Vector& p2, RS_CircleData& data) {
    const double r = 0.5 * p1.distanceTo(p2);
    if (r > RS_TOLERANCE) {
        data.radius = r;
        data.center = (p1 + p2) * 0.5;
        return true;
    }
    //        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom2P(): "
    //                        "Cannot create a circle with radius 0.0.");
    return false;
}

/**
 * Creates this circle from 3 given points which define the circle line.
 *
 * @param p1 1st point.
 * @param p2 2nd point.
 * @param p3 3rd point.
 */
bool LC_CreationCircle::createFrom3P(const RS_Vector& p1, const RS_Vector& p2, const RS_Vector& p3, RS_CircleData& data) {
    const RS_Vector vra = p2 - p1;
    const RS_Vector vrb = p3 - p1;
    const double ra2 = vra.squared() * 0.5;
    const double rb2 = vrb.squared() * 0.5;
    double crossp = (vra.x * vrb.y) - (vra.y * vrb.x);
    if (std::abs(crossp) < RS_TOLERANCE2) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom3P(): " "Cannot create a circle with radius 0.0.");
        return false;
    }
    crossp = 1. / crossp;
    data.center.set((ra2 * vrb.y - rb2 * vra.y) * crossp, (rb2 * vra.x - ra2 * vrb.x) * crossp);
    data.radius = data.center.magnitude();
    data.center += p1;
    return true;
}

//*create Circle from 3 points
//Author: Dongxu Li
bool LC_CreationCircle::createFrom3P(const RS_VectorSolutions& sol, RS_CircleData& data) {
    if (sol.getNumber() < 2) {
        return false;
    }
    if (sol.getNumber() == 2) {
        return createFrom2P(sol.get(0), sol.get(1), data);
    }
    if ((sol.get(1) - sol.get(2)).squared() < RS_TOLERANCE2) {
        return createFrom2P(sol.get(0), sol.get(1), data);
    }
    const RS_Vector vra(sol.get(1) - sol.get(0));
    const RS_Vector vrb(sol.get(2) - sol.get(0));
    const double ra2 = vra.squared() * 0.5;
    const double rb2 = vrb.squared() * 0.5;
    double crossp = (vra.x * vrb.y) - (vra.y * vrb.x);
    if (std::abs(crossp) < RS_TOLERANCE2) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Circle::createFrom3P(): " "Cannot create a circle with radius 0.0.");
        return false;
    }
    crossp = 1. / crossp;
    data.center.set((ra2 * vrb.y - rb2 * vra.y) * crossp, (rb2 * vra.x - ra2 * vrb.x) * crossp);
    data.radius = data.center.magnitude();
    data.center += sol.get(0);
    return true;
}

/**
  *create circle inscribled in a triangle
  *
  *Author: Dongxu Li
  */
bool LC_CreationCircle::createInscribe(const RS_Vector& coord, const std::vector<RS_Line*>& lines, RS_CircleData& data) {
    if (lines.size() < 3) {
        return false;
    }
    std::vector<RS_Line*> tri(lines);
    RS_VectorSolutions sol = RS_Information::getIntersectionLineLine(tri[0], tri[1]);
    if (sol.isEmpty()) {
        //move parallel to opposite
        std::swap(tri[1], tri[2]);
        sol = RS_Information::getIntersectionLineLine(tri[0], tri[1]);
    }
    if (sol.isEmpty()) {
        return false;
    }
    RS_Vector vp0(sol.get(0));
    sol = RS_Information::getIntersectionLineLine(tri[2], tri[1]);
    if (sol.isEmpty()) {
        return false;
    }
    RS_Vector vp1(sol.get(0));
    RS_Vector dvp(vp1 - vp0);
    double a(dvp.squared());
    if (a < RS_TOLERANCE2) {
        return false; //three lines share a common intersecting point
    }
    RS_Vector vp(coord - vp0);
    vp -= dvp * (RS_Vector::dotP(dvp, vp) / a); //normal component
    RS_Vector vl0(tri[0]->getEndpoint() - tri[0]->getStartpoint());
    a = dvp.angle();
    double angle0(0.5 * (vl0.angle() + a));
    if (RS_Vector::dotP(vp, vl0) < 0.) {
        angle0 += 0.5 * M_PI;
    }

    RS_Line line0(vp0, vp0 + RS_Vector(angle0)); //first bisecting line
    vl0 = tri[2]->getEndpoint() - tri[2]->getStartpoint();
    angle0 = 0.5 * (vl0.angle() + a + M_PI);
    if (RS_Vector::dotP(vp, vl0) < 0.) {
        angle0 += 0.5 * M_PI;
    }
    RS_Line line1(vp1, vp1 + RS_Vector(angle0)); //second bisection line
    sol = RS_Information::getIntersectionLineLine(&line0, &line1);
    if (sol.isEmpty()) {
        return false;
    }
    bool ret = createFromCR(sol.get(0), tri[1]->getDistanceToPoint(sol.get(0)), data);
    if (!ret) {
        return false;
    }
    for (auto p : lines) {
        if (!p->isTangent(data)) {
            return false;
        }
    }
    return true;
}

std::vector<RS_Circle> LC_CreationCircle::createTan3(const std::vector<RS_AtomicEntity*>& circles) {
    std::vector<RS_Circle> ret;
    if (circles.size() != 3) {
        return ret;
    }
    std::vector<RS_Circle> cs;
    for (const RS_AtomicEntity* c : circles) {
        cs.emplace_back(RS_Circle(nullptr, {c->getCenter(), c->getRadius()}));
    }
    unsigned short flags = 0;
    do {
        for (unsigned short j = 0U; j < 3U; ++j) {
            if (flags & (1U << j)) {
                cs[j].setRadius(-std::abs(cs[j].getRadius()));
            }
            else {
                cs[j].setRadius(std::abs(cs[j].getRadius()));
            }
        }
        //        RS_DEBUG->print(RS_Debug::D_ERROR, "flags=%d\n",flags);
        std::vector<RS_Circle> list = solveCircleApolloniusSingle(cs);
        if (list.empty()) {
            list = solveCircleApolloniusHyperbola(cs);
        }
        if (!list.empty()) {
            for (RS_Circle& c0 : list) {
                bool addNew = true;
                for (RS_Circle& c : ret) {
                    if ((c0.getCenter() - c.getCenter()).squared() < RS_TOLERANCE15 && std::abs(c0.getRadius() - c.getRadius()) <
                        RS_TOLERANCE) {
                        addNew = false;
                        break;
                    }
                }
                if (addNew) {
                    ret.push_back(c0);
                }
            }
        }
    }
    while (++flags < 8U);
    //    std::cout<<__FILE__<<" : "<<__func__<<" : line "<<__LINE__<<std::endl;
    //    std::cout<<"before testing, ret.size()="<<ret.size()<<std::endl;
    const auto it = std::remove_if(ret.begin(), ret.end(), [&circles](const RS_Circle& circle) {
        return !testTan3(circles, circle);
    });
    ret.erase(it, ret.end());
    //        DEBUG_HEADER
    //    std::cout<<"after testing, ret.size()="<<ret.size()<<std::endl;
    return ret;
}

RS_VectorSolutions LC_CreationCircle::createTan1_2P(const RS_AtomicEntity* circle, const std::vector<RS_Vector>& points) {
    if ((circle == nullptr) || points.size() < 2) {
        return {};
    }
    return LC_Quadratic::getIntersection(LC_Quadratic(circle, points[0]), LC_Quadratic(circle, points[1]));
}

/**
  * create a circle of radius r and tangential to two given entities
  */
RS_VectorSolutions LC_CreationCircle::createTan2(const std::vector<RS_AtomicEntity*>& circles, const double r) {
    if (circles.size() < 2) {
        return {};
    }
    const auto e0 = circles[0]->offsetTwoSides(r);
    const auto e1 = circles[1]->offsetTwoSides(r);
    RS_VectorSolutions centers;
    if (!e0.empty() && !e1.empty()) {
        for (const auto& it0 : e0) {
            for (const auto& it1 : e1) {
                centers.push_back(RS_Information::getIntersection(it0, it1));
            }
        }
    }
    for (const auto& it0 : e0) {
        delete it0;
    }
    for (const auto& it0 : e1) {
        delete it0;
    }
    return centers;
}

bool LC_CreationCircle::create2PRadius(const RS_Vector& point1, const RS_Vector& point2, double radius, RS_Vector& altCenter, RS_CircleData& m_circleData){
    const double chordLenght = point2.distanceTo(point1);
    if (chordLenght <= 2. * radius) {
        const RS_Vector chordMiddlePoint = (point1 + point2) * 0.5;
        const double angle = point1.angleTo(point2) + (0.5 * M_PI);
        m_circleData.radius = radius;
        const double distanceFromChordToCenter = sqrt((radius * radius) - (0.25 * point1.squaredTo(point2)));
        const RS_Vector ortoVector = RS_Vector(angle) * distanceFromChordToCenter;
        const RS_Vector& center1 = chordMiddlePoint + ortoVector;
        const RS_Vector& center2 = chordMiddlePoint - ortoVector;

        if (center1.squaredTo(center2) < RS_TOLERANCE) {
            //no need to select center, as only one solution possible
            m_circleData.center = chordMiddlePoint;
            return false;
        }
        m_circleData.center = center1;
        altCenter = center2;
        return true;
    }
    m_circleData.center.valid = false;
    return false;
}
