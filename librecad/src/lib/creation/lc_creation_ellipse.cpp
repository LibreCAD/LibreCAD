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


#include "lc_creation_ellipse.h"

#include <float.h>

#include "lc_quadratic.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"

/**
//create Ellipse with center and 3 points
*
*
*@author Dongxu Li
*/
bool LC_CreationEllipse::createEllipseFrom4P(const RS_VectorSolutions& sol, RS_EllipseData& data) {
    if (sol.getNumber() < 3) {
        return false; //need one center and 3 points on ellipse
    }
    std::vector<std::vector<double>> mt;
    size_t solSize = sol.getNumber() - 1;
    if ((sol.get(solSize) - sol.get(solSize - 1)).squared() < RS_TOLERANCE15) {
        //remove the last point
        solSize--;
    }

    mt.resize(solSize);
    std::vector<double> dn(solSize);
    switch (solSize) {
        case 2:
            for (size_t i = 0; i < solSize; i++) {
                //form the linear equation
                mt[i].resize(solSize + 1);
                const RS_Vector vp(sol.get(i + 1) - sol.get(0)); //the first vector is center
                mt[i][0] = vp.x * vp.x;
                mt[i][1] = vp.y * vp.y;
                mt[i][2] = 1.;
            }
            if (!RS_Math::linearSolver(mt, dn)) {
                return false;
            }
            if (dn[0] < RS_TOLERANCE15 || dn[1] < RS_TOLERANCE15) {
                return false;
            }
            data.majorP = RS_Vector(1. / sqrt(dn[0]), 0.);
            data.ratio = sqrt(dn[0] / dn[1]);
            data.setAngle1(0.);
            data.setAngle2(0.);
            data.center = sol.get(0);
            return true;

        case 3:
            for (size_t i = 0; i < solSize; i++) {
                //form the linear equation
                mt[i].resize(solSize + 1);
                const RS_Vector vp(sol.get(i + 1) - sol.get(0)); //the first vector is center
                mt[i][0] = vp.x * vp.x;
                mt[i][1] = vp.x * vp.y;
                mt[i][2] = vp.y * vp.y;
                mt[i][3] = 1.;
            }
            if (!RS_Math::linearSolver(mt, dn)) {
                return false;
            }
            data.center = sol.get(0);
            return createEllipseFromQuadratic(dn,data);
        default:
            return false;
    }
    return false; // only for compiler warning
}


/**
//create Ellipse with center and 3 points
*
*
*@author Dongxu Li
*/
bool LC_CreationEllipse::createEllipseFromCenter3Points(const RS_VectorSolutions& sol, RS_EllipseData& data) {
    if (sol.getNumber() < 3) {
        return false; //need one center and 3 points on ellipse
    }
    std::vector<std::vector<double>> mt;
    size_t solSize = sol.getNumber() - 1;
    if ((sol.get(solSize) - sol.get(solSize - 1)).squared() < RS_TOLERANCE15) {
        //remove the last point
        solSize--;
    }

    mt.resize(solSize);
    std::vector<double> dn(solSize);
    switch (solSize) {
        case 2:
            for (size_t i = 0; i < solSize; i++) {
                //form the linear equation
                mt[i].resize(solSize + 1);
                const RS_Vector vp(sol.get(i + 1) - sol.get(0)); //the first vector is center
                mt[i][0] = vp.x * vp.x;
                mt[i][1] = vp.y * vp.y;
                mt[i][2] = 1.;
            }
            if (!RS_Math::linearSolver(mt, dn)) {
                return false;
            }
            if (dn[0] < RS_TOLERANCE15 || dn[1] < RS_TOLERANCE15) {
                return false;
            }
            data.majorP = RS_Vector(1. / sqrt(dn[0]), 0.);
            data.ratio= sqrt(dn[0] / dn[1]);
            data.setAngle1(0.);
            data.setAngle2(0.);
            data.center = sol.get(0);
            return true;

        case 3:
            for (size_t i = 0; i < solSize; i++) {
                //form the linear equation
                mt[i].resize(solSize + 1);
                const RS_Vector vp(sol.get(i + 1) - sol.get(0)); //the first vector is center
                mt[i][0] = vp.x * vp.x;
                mt[i][1] = vp.x * vp.y;
                mt[i][2] = vp.y * vp.y;
                mt[i][3] = 1.;
            }
            if (!RS_Math::linearSolver(mt, dn)) {
                return false;
            }
            data.center = sol.get(0);
            return createEllipseFromQuadratic(dn, data);
        default:
            return false;
    }
    return false; // only for compiler warning
}


/** \brief create from quadratic form:
  * dn[0] x^2 + dn[1] xy + dn[2] y^2 =1
  * keep the ellipse center before calling this function
  *
  *@author: Dongxu Li
  */
bool LC_CreationEllipse::createEllipseFromQuadratic(const std::vector<double>& dn, RS_EllipseData &data) {
    RS_DEBUG->print("RS_Ellipse::createFromQuadratic() begin\n");
    if (dn.size() != 3) {
        return false;
    }
    //	if(std::abs(dn[0]) <RS_TOLERANCE2 || std::abs(dn[2])<RS_TOLERANCE2) return false; //invalid quadratic form

    //eigenvalues and eigenvectors of quadratic form
    // (dn[0] 0.5*dn[1])
    // (0.5*dn[1] dn[2])
    const double a = dn[0];
    const double c = dn[1];
    const double b = dn[2];

    //Eigen system
    const double d = a - b;
    const double s = hypot(d, c);
    // { a>b, d>0
    // eigenvalue: ( a+b - s)/2, eigenvector: ( -c, d + s)
    // eigenvalue: ( a+b + s)/2, eigenvector: ( d + s, c)
    // }
    // { a<b, d<0
    // eigenvalue: ( a+b - s)/2, eigenvector: ( s-d,-c)
    // eigenvalue: ( a+b + s)/2, eigenvector: ( c, s-d)
    // }

    // eigenvalues are required to be positive for ellipses
    if (s >= a + b) {
        return false;
    }
    if (a >= b) {
        data.majorP = RS_Vector(atan2(d + s, -c)) / sqrt(0.5 * (a + b - s));
    }
    else {
        data.majorP = RS_Vector(atan2(-c, s - d)) / sqrt(0.5 * (a + b - s));
    }
    data.ratio = sqrt((a + b - s) / (a + b + s));

    // start/end angle at 0. means a whole ellipse, instead of an elliptic arc
    data.setAngle1(0.);
    data.setAngle2(0.);

    RS_DEBUG->print("RS_Ellipse::createFromQuadratic(): successful\n");
    return true;
}

bool LC_CreationEllipse::createEllipseFromQuadratic(const LC_Quadratic& q, RS_EllipseData &data) {
    if (!q.isQuadratic()) {
        return false;
    }
    const auto& mQ = q.getQuad();
    const double& a = mQ(0, 0);
    const double& c = 2. * mQ(0, 1);
    const double& b = mQ(1, 1);
    const auto& mL = q.getLinear();
    const double& d = mL(0);
    const double& e = mL(1);
    const double determinant = (c * c) - (4. * a * b);
    if (determinant >= -DBL_EPSILON) {
        return false;
    }
    // find center of quadratic
    // 2 A x + C y = D
    // C x   + 2 B y = E
    // x = (2BD - EC)/( 4AB - C^2)
    // y = (2AE - DC)/(4AB - C^2)
    const RS_Vector eCenter = RS_Vector((2. * b * d) - (e * c), (2. * a * e) - (d * c)) / determinant;
    //generate centered quadratic
    LC_Quadratic qCentered = q;
    qCentered.move(-eCenter);
    if (qCentered.constTerm() >= -DBL_EPSILON) {
        return false;
    }
    const auto& mq2 = qCentered.getQuad();
    const double factor = -1. / qCentered.constTerm();
    //quadratic terms
    if (!createEllipseFromQuadratic({mq2(0, 0) * factor, 2. * mq2(0, 1) * factor, mq2(1, 1) * factor}, data)) {
        return false;
    }

    //move back to center
    data.move(eCenter);
    return true;
}

/**
//create Ellipse inscribed in a quadrilateral
*
*algorithm: http://chrisjones.id.au/Ellipses/ellipse.html
*finding the tangential points and ellipse center
*
*@author Dongxu Li
*/
bool LC_CreationEllipse::createEllipseInscribeQuadrilateral(const std::vector<RS_Line*>& lines, std::vector<RS_Vector>& tangent, RS_EllipseData &data) {
    static constexpr int LINES_COUNT = 4;

    if (lines.size() != LINES_COUNT) {
        return false; //only do 4 lines
    }

    std::array<std::unique_ptr<RS_Line>, LINES_COUNT> quad;
    {
        //form quadrilateral from intersections
        RS_EntityContainer c0(nullptr, false);
        for (RS_Line* const p : lines) {
            //copy the line pointers
            c0.addEntity(p);
        }
        const RS_VectorSolutions& s0 = RS_Information::createQuadrilateral(c0);
        if (s0.size() != LINES_COUNT) {
            return false;
        }
        for (size_t i = 0; i < LINES_COUNT; ++i) {
            quad[i].reset(new RS_Line{s0[i], s0[(i + 1) % LINES_COUNT]});
        }
    }

    //center of original square projected, intersection of diagonal
    RS_Vector centerProjection;
    {
        auto diagonal1 = RS_Line(quad[0]->getStartpoint(), quad[1]->getEndpoint());
        auto diagonal2 = RS_Line(quad[1]->getStartpoint(), quad[2]->getEndpoint());
        const RS_VectorSolutions& sol = RS_Information::getIntersectionLineLine(&diagonal1, &diagonal2);
        if (sol.getNumber() == 0) {
            //this should not happen
            //        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Ellipse::createInscribeQuadrilateral(): can not locate projection Center");
            RS_DEBUG->print("RS_Ellipse::createInscribeQuadrilateral(): can not locate projection Center");
            return false;
        }
        centerProjection = sol.get(0);
    }
    //        std::cout<<"RS_Ellipse::createInscribe(): centerProjection="<<centerProjection<<std::endl;

    //	std::vector<RS_Vector> tangent;//holds the tangential points on edges, in the order of edges: 1 3 2 0
    int parallel = 0;
    size_t parallelIndex = 0;
    for (int i = 0; i <= 1; ++i) {
        const RS_VectorSolutions& sol1 = RS_Information::getIntersectionLineLine(quad[i].get(), quad[(i + 2) % LINES_COUNT].get());
        RS_Vector direction;
        if (sol1.getNumber() == 0) {
            direction = quad[i]->getEndpoint() - quad[i]->getStartpoint();
            ++parallel;
            parallelIndex = i;
        }
        else {
            direction = sol1.get(0) - centerProjection;
        }
        //                std::cout<<"Direction: "<<direction<<std::endl;
        RS_Line l(centerProjection, centerProjection + direction);
        for (int k = 1; k <= 3; k += 2) {
            const RS_VectorSolutions sol2 = RS_Information::getIntersectionLineLine(&l, quad[(i + k) % LINES_COUNT].get());
            if (sol2.isNotEmpty()) {
                tangent.push_back(sol2.get(0));
            }
        }
    }

    if (tangent.size() < 3) {
        return false;
    }

    //find ellipse center by projection
    RS_Vector ellipseCenter;
    {
        RS_Line cl0(quad[1]->getEndpoint(), (tangent[0] + tangent[2]) * 0.5);
        RS_Line cl1(quad[2]->getEndpoint(), (tangent[1] + tangent[2]) * 0.5);
        const RS_VectorSolutions& sol = RS_Information::getIntersection(&cl0, &cl1, false);
        if (sol.getNumber() == 0) {
            //this should not happen
            //        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Ellipse::createInscribeQuadrilateral(): can not locate Ellipse Center");
            RS_DEBUG->print("RS_Ellipse::createInscribeQuadrilateral(): can not locate Ellipse Center");
            return false;
        }
        ellipseCenter = sol.get(0);
    }
    //	qDebug()<<"parallel="<<parallel;
    if (parallel == 1) {
        RS_DEBUG->print("RS_Ellipse::createInscribeQuadrilateral(): trapezoid detected\n");
        //trapezoid
        RS_Line* l0 = quad[parallelIndex].get();
        RS_Line* l1 = quad[(parallelIndex + 2) % LINES_COUNT].get();
        RS_Vector centerPoint = (l0->getMiddlePoint() + l1->getMiddlePoint()) * 0.5;
        //not symmetric, no inscribed ellipse
        if (std::abs(centerPoint.distanceTo(l0->getStartpoint()) - centerPoint.distanceTo(l0->getEndpoint())) > RS_TOLERANCE) {
            return false;
        }
        //symmetric
        RS_DEBUG->print("RS_Ellipse::createInscribeQuadrilateral(): symmetric trapezoid detected\n");
        double d = l0->getDistanceToPoint(centerPoint);
        double l = ((l0->getLength() + l1->getLength())) * 0.25;
        double k = 4. * d / std::abs(l0->getLength() - l1->getLength());
        double theta = d / (l * k);
        if (theta >= 1. || d < RS_TOLERANCE) {
            RS_DEBUG->print("RS_Ellipse::createInscribeQuadrilateral(): this should not happen\n");
            return false;
        }
        theta = asin(theta);

        //major axis
        double a = d / (k * tan(theta));
        data.center = RS_Vector(0., 0.);
        data.majorP = RS_Vector(a, 0.);
        data.ratio = d / a;
        data.rotate(l0->getAngle1());
        data.center = centerPoint; // fixme - sand - originally, there is setCenter. But shouldn't move() be used there instead?
        return true;
    }
    //    double ratio;
    //        std::cout<<"dn="<<dn[0]<<' '<<dn[1]<<' '<<dn[2]<<std::endl;
    std::vector<double> dn(3);
    RS_Vector angleVector(false);

    for (auto & i : tangent) {
        i -= ellipseCenter; //relative to ellipse center
    }
    std::vector<std::vector<double>> mt;
    mt.clear();
    constexpr double symTolerance = 20. * RS_TOLERANCE;
    for (const RS_Vector& vp : tangent) {
        //form the linear equation
        // need to remove duplicated {x^2, xy, y^2} terms due to symmetry (x => -x, y=> -y)
        // i.e. rotation of 180 degrees around ellipse center
        //		std::cout<<"point  : "<<vp<<std::endl;
        std::vector<double> mtRow;
        mtRow.push_back(vp.x * vp.x);
        mtRow.push_back(vp.x * vp.y);
        mtRow.push_back(vp.y * vp.y);
        const double l = hypot(hypot(mtRow[0], mtRow[1]), mtRow[2]);
        bool addRow(true);
        for (const auto& v : mt) {
            const RS_Vector dv{v[0] - mtRow[0], v[1] - mtRow[1], v[2] - mtRow[2]};
            if (dv.magnitude() < symTolerance * l) {
                //symmetric
                addRow = false;
                break;
            }
        }
        if (addRow) {
            mtRow.push_back(1.);
            mt.push_back(mtRow);
        }
    }
    //    std::cout<<"mt.size()="<<mt.size()<<std::endl;
    switch (mt.size()) {
        case 2: {
            // the quadrilateral is a parallelogram
            RS_DEBUG->print("RS_Ellipse::createInscribeQuadrilateral(): parallelogram detected\n");

            //fixme, need to handle degenerate case better
            //        double angle(center.angleTo(tangent[0]));
            RS_Vector majorP(tangent[0]);
            double dx(majorP.magnitude());
            if (dx < RS_TOLERANCE2) {
                return false; //refuse to return zero size ellipse
            }
            angleVector.set(majorP.x / dx, -majorP.y / dx);
            for (auto & i : tangent) {
                i.rotate(angleVector);
            }

            RS_Vector minorP(tangent[2]);
            double dy2(minorP.squared());
            if (std::abs(minorP.y) < RS_TOLERANCE || dy2 < RS_TOLERANCE2) {
                return false; //refuse to return zero size ellipse
            }
            // y'= y
            // x'= x-y/tan
            // reverse scale
            // y=y'
            // x=x'+y' tan
            //
            double ia2 = 1. / (dx * dx);
            double ib2 = 1. / (minorP.y * minorP.y);
            //ellipse scaled:drawi
            // ia2*x'^2+ib2*y'^2=1
            // ia2*(x-y*minor.x/minor.y)^2+ib2*y^2=1
            // ia2*x^2 -2*ia2*minor.x/minor.y xy + ia2*minor.x^2*ib2 y^2 + ib2*y^2 =1
            dn[0] = ia2;
            dn[1] = -2. * ia2 * minorP.x / minorP.y;
            dn[2] = (ib2 * ia2 * minorP.x * minorP.x) + ib2;
        }
        break;
        case LINES_COUNT:
            mt.pop_back(); //only 3 points needed to form the qudratic form
            if (!RS_Math::linearSolver(mt, dn)) {
                return false;
            }
            break;
        default: RS_DEBUG->print(RS_Debug::D_WARNING, "No inscribed ellipse for non isosceles trapezoid");
            return false; //invalid quadrilateral
    }

    if (!createEllipseFromQuadratic(dn, data)) {
        return false;
    }
    data.center = ellipseCenter;

    if (angleVector.valid) {
        //need to rotate back, for the parallelogram case
        angleVector.y *= -1.;
        data.rotate(ellipseCenter, angleVector);
    }
    return true;
}
