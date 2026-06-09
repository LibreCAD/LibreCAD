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

#include "lc_splinepoints.h"

#include <QPainterPath>

#include "lc_quadratic.h"
#include "rs_circle.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_painter.h"

namespace {
    RS_Vector getQuadPoint(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double dt) {
        return x1 * (1.0 - dt) * (1.0 - dt) + c1 * 2.0 * dt * (1.0 - dt) + x2 * dt * dt;
    }

    void strokeQuad(std::vector<RS_Vector>* plist, const RS_Vector& vx1, const RS_Vector& vc1, const RS_Vector& vx2, const int iSeg) {
        if (iSeg < 1) {
            plist->push_back(vx1);
            return;
        }

        for (int i = 0; i < iSeg; i++) {
            const double doubleI = i;
            const double dt = doubleI / iSeg;
            RS_Vector x = getQuadPoint(vx1, vc1, vx2, dt);
            plist->push_back(x);
        }
    }

    RS_Vector getQuadDir(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double dt) {
        const RS_Vector vStart = c1 - x1;
        const RS_Vector vEnd = x2 - c1;
        RS_Vector vRes(false);

        double dDist = (vEnd - vStart).squared();
        if (dDist > RS_TOLERANCE2) {
            vRes = vStart * (1.0 - dt) + vEnd * dt;
            dDist = vRes.magnitude();
            if (dDist < RS_TOLERANCE) {
                return RS_Vector(false);
            }

            return vRes / dDist;
        }

        dDist = (x2 - x1).magnitude();
        if (dDist > RS_TOLERANCE) {
            return (x2 - x1) / dDist;
        }

        return vRes;
    }

    RS_Vector getSubQuadControlPoint(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double dt1, const double dt2) {
        return x1 * (1.0 - dt1) * (1.0 - dt2) + c1 * dt1 * (1.0 - dt2) + c1 * dt2 * (1.0 - dt1) + x2 * dt1 * dt2;
    }

    double lenInt(const double x) {
        // Issue #1610 : when x is negative and much smaller than -1E5, sqrt(1+x*x) is very close to -x, so
        // brute force evaluation of x+y loses significant digits quickly when the absolute value of a negative
        // x gets larger. when x+y is evaluated to be negative, log(x+y) will become meaningless.
        // The solution, for negative x, when x+y may lose significant digits, evaluating log(x+y) as
        //   log( y + x ) = log ((y^2 - x^2)/(y - x)) = - log(y - x)
        const double y = std::sqrt(1 + x * x);
        if (std::signbit(x)) {
            return x * y - std::log(y - x);
        }
        return x * y + std::log(y + x);
    }

    double getQuadLength(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double t1, const double t2) {
        const RS_Vector xh1 = (c1 - x1) * 2.0;
        const RS_Vector xh2 = (x2 - c1) * 2.0;
        const RS_Vector xd1 = xh2 - xh1;
        const RS_Vector xd2 = xh1;

        const double dx1 = xd1.squared();
        double dx2 = xd2.squared();
        const double dx12 = xd1.x * xd2.x + xd1.y * xd2.y;
        const double dDet = dx1 * dx2 - dx12 * dx12; // always >= 0 from Schwarz inequality

        double dRes = 0.0;

        if (dDet > RS_TOLERANCE) {
            const double dA = std::sqrt(dDet);
            const double v1 = (dx1 * t1 + dx12) / dA;
            const double v2 = (dx1 * t2 + dx12) / dA;
            dRes = (lenInt(v2) - lenInt(v1)) * dDet / 2.0 / dx1 / std::sqrt(dx1);
        }
        else {
            if (dx1 < RS_TOLERANCE) {
                dRes = std::sqrt(dx2) * (t2 - t1);
            }
            else {
                dx2 = std::sqrt(dx1);
                dRes = (t2 - t1) * (dx2 * (t2 + t1) / 2.0 + dx12 / dx2);
            }
        }

        return dRes;
    }

    double getQuadLengthDeriv(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double t2) {
        const RS_Vector xh1 = (c1 - x1) * 2.0;
        const RS_Vector xh2 = (x2 - c1) * 2.0;
        const RS_Vector xd1 = xh2 - xh1;
        const RS_Vector xd2 = xh1;

        const double dx1 = xd1.squared();
        double dx2 = xd2.squared();
        const double dx12 = xd1.x * xd2.x + xd1.y * xd2.y;
        const double dDet = dx1 * dx2 - dx12 * dx12; // always >= 0 from Schwarz inequality

        double dRes = 0.0;

        if (dDet > RS_TOLERANCE) {
            const double dA = std::sqrt(dDet);
            const double v2 = (dx1 * t2 + dx12) / dA;
            const double v3 = v2 * v2;
            const double v4 = 1.0 + v3;
            const double v5 = std::sqrt(v4);
            dRes = ((v2 + v5) / (v4 + v2 * v5) + (2.0 * v3 + 1.0) / v5) * dA / 2.0 / std::sqrt(dx1);
        }
        else {
            if (dx1 < RS_TOLERANCE) {
                dRes = std::sqrt(dx2);
            }
            else {
                dx2 = std::sqrt(dx1);
                dRes = dx2 * t2 + dx12 / dx2;
            }
        }

        return dRes;
    }

    double getQuadPointAtDist(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double t1, const double dDist) {
        const RS_Vector xh1 = (c1 - x1) * 2.0;
        const RS_Vector xh2 = (x2 - c1) * 2.0;
        const RS_Vector xd1 = xh2 - xh1;
        const RS_Vector xd2 = xh1;

        const double dx1 = xd1.squared();
        double dx2 = xd2.squared();
        const double dx12 = xd1.x * xd2.x + xd1.y * xd2.y;
        const double dDet = dx1 * dx2 - dx12 * dx12; // always >= 0 from Schwarz inequality

        double dRes = RS_MAXDOUBLE;

        std::vector<double> dCoefs(0, 0.);
        std::vector<double> dSol(0, 0.);

        if (dDet > RS_TOLERANCE) {
            const double dA = std::sqrt(dDet);
            const double v1 = (dx1 * t1 + dx12) / dA;
            //double v2 = (dx1*t2 + dx12)/dA;
            //dDist = (LenInt(v2) - LenInt(v1))*dDet/2.0/dx1/std::sqrt(dx1);
            //LenInt(v2) = 2.0*dx1*std::sqrt(dx1)*dDist/dDet + LenInt(v1);
            const double dB = 2.0 * dx1 * std::sqrt(dx1) * dDist / dDet + lenInt(v1);

            dCoefs.push_back(0.0);
            dCoefs.push_back(0.0);
            dCoefs.push_back(2.0 * dB);
            dCoefs.push_back(-dB * dB);
            dSol = RS_Math::quarticSolver(dCoefs);

            dRes = t1;
            double a1 = 0;
            for (const double& d : dSol) {
                const double a0 = (d * dA - dx12) / dx1;
                const double a2 = getQuadLength(x1, c1, x2, t1, a0);
                if (std::abs(dDist - a2) < std::abs(dDist - a1)) {
                    a1 = a2;
                    dRes = a0;
                }
            }

            // we believe we are pretty close to the solution at the moment
            // so we only perform three iterations
            for (int i = 0; i < 3; i++) {
                a1 = getQuadLength(x1, c1, x2, t1, dRes) - dDist;
                const double a2 = getQuadLengthDeriv(x1, c1, x2, dRes);
                if (std::abs(a2) > RS_TOLERANCE) {
                    dRes -= a1 / a2;
                }
            }
        }
        else {
            if (dx1 < RS_TOLERANCE) {
                if (dx2 > RS_TOLERANCE) {
                    dRes = t1 + dDist / std::sqrt(dx2);
                }
            }
            else {
                dx2 = std::sqrt(dx1);
                //dRes = (t2 - t1)*(dx2*(t2 + t1)/2.0 + dx12/dx2);

                const double a0 = dx2 / 2.0;
                double a1 = dx12 / dx2;
                double a2 = -dDist - a0 * t1 * t1 - a1 * t1;

                dCoefs.push_back(a1 / a0);
                dCoefs.push_back(a2 / a0);
                dSol = RS_Math::quadraticSolver(dCoefs);

                if (dSol.size() > 0) {
                    dRes = dSol[0];
                    if (dSol.size() > 1) {
                        a1 = getQuadLength(x1, c1, x2, t1, dRes);
                        a2 = getQuadLength(x1, c1, x2, t1, dSol[1]);
                        if (std::abs(dDist - a2) < std::abs(dDist - a1)) {
                            dRes = dSol[1];
                        }
                    }
                }
            }
        }

        return dRes;
    }

    RS_Vector getThreePointsControl(const RS_Vector& x1, const RS_Vector& x2, const RS_Vector& x3) {
        const double dl1 = (x2 - x1).magnitude();
        const double dl2 = (x3 - x2).magnitude();
        const double dt = dl1 / (dl1 + dl2);

        if (dt < RS_TOLERANCE || dt > 1.0 - RS_TOLERANCE) {
            return RS_Vector{false};
        }

        const RS_Vector vRes = (x2 - x1 * (1.0 - dt) * (1.0 - dt) - x3 * dt * dt) / dt / (1 - dt) / 2.0;
        return vRes;
    }

    double getDistToLine(const RS_Vector& coord, const RS_Vector& x1, const RS_Vector& x2, double* dist) {
        const double ddet = (x2 - x1).squared();
        if (ddet < RS_TOLERANCE) {
            *dist = (coord - x1).magnitude();
            return 0.0;
        }

        const double dt = ((coord.x - x1.x) * (x2.x - x1.x) + (coord.y - x1.y) * (x2.y - x1.y)) / ddet;

        if (dt < RS_TOLERANCE) {
            *dist = (coord - x1).magnitude();
            return 0.0;
        }

        if (dt > 1.0 - RS_TOLERANCE) {
            *dist = (coord - x2).magnitude();
            return 1.0;
        }

        const RS_Vector vRes = x1 * (1.0 - dt) + x2 * dt;
        *dist = (coord - vRes).magnitude();
        return dt;
    }

    RS_Vector getQuadAtPoint(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double dt) {
        const RS_Vector vRes = x1 * (1.0 - dt) * (1.0 - dt) + c1 * 2.0 * dt * (1.0 - dt) + x2 * dt * dt;
        return vRes;
    }

    RS_Vector getQuadDirAtPoint(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, const double dt) {
        const RS_Vector vRes = (c1 - x1) * (1.0 - dt) + (x2 - c1) * dt;
        return vRes;
    }

    double getDistToQuadAtPointSquared(const RS_Vector& coord, const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2,
                                       const double dt) {
        if (dt < RS_TOLERANCE) {
            return (coord - x1).squared();
        }

        if (dt > 1.0 - RS_TOLERANCE) {
            return (coord - x2).squared();
        }

        const RS_Vector vx = getQuadAtPoint(x1, c1, x2, dt);
        return (coord - vx).squared();
    }

    // returns true if the new distance was smaller than previous one
    bool setNewDist(const bool bResSet, const double dNewDist, const double dNewT, double* pdDist, double* pdt) {
        bool bRes = false;
        if (bResSet) {
            if (dNewDist < *pdDist) {
                *pdDist = dNewDist;
                *pdt = dNewT;
                bRes = true;
            }
        }
        else {
            *pdDist = dNewDist;
            *pdt = dNewT;
            bRes = true;
        }
        return bRes;
    }

    double getDistToQuadSquared(const RS_Vector& coord, const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2, double* dist) {
        const double a1 = (x2.x - 2.0 * c1.x + x1.x) * (x2.x - 2.0 * c1.x + x1.x) + (x2.y - 2.0 * c1.y + x1.y) * (x2.y - 2.0 * c1.y + x1.y);
        const double a2 = 3.0 * ((c1.x - x1.x) * (x2.x - 2.0 * c1.x + x1.x) + (c1.y - x1.y) * (x2.y - 2.0 * c1.y + x1.y));
        const double a3 = 2.0 * (c1.x - x1.x) * (c1.x - x1.x) + (x1.x - coord.x) * (x2.x - 2.0 * c1.x + x1.x) + 2.0 * (c1.y - x1.y) * (c1.y
            - x1.y) + (x1.y - coord.y) * (x2.y - 2.0 * c1.y + x1.y);
        const double a4 = (x1.x - coord.x) * (c1.x - x1.x) + (x1.y - coord.y) * (c1.y - x1.y);

        std::vector<double> dCoefs(0, 0.);
        std::vector<double> dSol(0, 0.);

        if (std::abs(a1) > RS_TOLERANCE) // solve as cubic
        {
            dCoefs.push_back(a2 / a1);
            dCoefs.push_back(a3 / a1);
            dCoefs.push_back(a4 / a1);
            dSol = RS_Math::cubicSolver(dCoefs);
        }
        else if (std::abs(a2) > RS_TOLERANCE) // solve as quadratic
        {
            dCoefs.push_back(a3 / a2);
            dCoefs.push_back(a4 / a2);
            dSol = RS_Math::quadraticSolver(dCoefs);
        }
        else if (std::abs(a3) > RS_TOLERANCE) // solve as linear
        {
            dSol.push_back(-a4 / a3);
        }
        else {
            return -1.0;
        }

        bool bResSet = false;
        double dDist = 0., dNewDist;
        double dRes = 0.;
        for (const double& dSolValue : dSol) {
            dNewDist = getDistToQuadAtPointSquared(coord, x1, c1, x2, dSolValue);
            setNewDist(bResSet, dNewDist, dSolValue, &dDist, &dRes);
            bResSet = true;
        }

        dNewDist = (coord - x1).squared();
        setNewDist(bResSet, dNewDist, 0.0, &dDist, &dRes);

        dNewDist = (coord - x2).squared();
        setNewDist(bResSet, dNewDist, 1.0, &dDist, &dRes);

        *dist = dDist;

        return dRes;
    }

    RS_Vector getNearestMiddleLine(const RS_Vector& x1, const RS_Vector& x2, const RS_Vector& coord, double* dist, const int middlePoints) {
        double dt = 1.0 / (1.0 + middlePoints);
        RS_Vector vRes = x1 * (1.0 - dt) + x2 * dt;
        double dMinDist = (vRes - coord).magnitude();
        double dCurDist = 0.;

        for (int i = 1; i < middlePoints; i++) {
            dt = (1.0 + i) / (1.0 + middlePoints);
            RS_Vector vMiddle = x1 * (1.0 - dt) + x2 * dt;
            dCurDist = (vMiddle - coord).magnitude();

            if (dCurDist < dMinDist) {
                dMinDist = dCurDist;
                vRes = vMiddle;
            }
        }

        if (dist != nullptr) {
            *dist = dMinDist;
        }
        return vRes;
    }

    void addQuadTangentPoints(RS_VectorSolutions* pVS, const RS_Vector& point, const RS_Vector& x1, const RS_Vector& c1,
                              const RS_Vector& x2) {
        const RS_Vector vx1 = x2 - c1 * 2.0 + x1;
        const RS_Vector vx2 = c1 - x1;
        const RS_Vector vx3 = x1 - point;

        const double a1 = vx2.x * vx1.y - vx2.y * vx1.x;
        const double a2 = vx3.x * vx1.y - vx3.y * vx1.x;
        const double a3 = vx3.x * vx2.y - vx3.y * vx2.x;

        std::vector<double> dSol(0, 0.);

        if (std::abs(a1) > RS_TOLERANCE) {
            std::vector<double> dCoefs(0, 0.);

            dCoefs.push_back(a2 / a1);
            dCoefs.push_back(a3 / a1);
            dSol = RS_Math::quadraticSolver(dCoefs);
        }
        else if (std::abs(a2) > RS_TOLERANCE) {
            dSol.push_back(-a3 / a2);
        }

        for (double& d : dSol) {
            if (d > -RS_TOLERANCE && d < 1.0 + RS_TOLERANCE) {
                if (d < 0.0) {
                    d = 0.0;
                }
                if (d > 1.0) {
                    d = 1.0;
                }
                pVS->push_back(getQuadPoint(x1, c1, x2, d));
            }
        }
    }

    void addLineQuadIntersect(RS_VectorSolutions* pVS, const RS_Vector& vStart, const RS_Vector& vEnd, const RS_Vector& vx1,
                              const RS_Vector& vc1, const RS_Vector& vx2) {
        RS_Vector x1 = vx2 - vc1 * 2.0 + vx1;
        const RS_Vector x2 = vc1 - vx1;
        const RS_Vector x3 = vx1 - vStart;
        const RS_Vector x4 = vEnd - vStart;

        const double a1 = x1.x * x4.y - x1.y * x4.x;
        const double a2 = 2.0 * (x2.x * x4.y - x2.y * x4.x);
        const double a3 = x3.x * x4.y - x3.y * x4.x;

        std::vector<double> dSol(0, 0.);

        if (std::abs(a1) > RS_TOLERANCE) {
            std::vector<double> dCoefs(0, 0.);

            dCoefs.push_back(a2 / a1);
            dCoefs.push_back(a3 / a1);
            dSol = RS_Math::quadraticSolver(dCoefs);
        }
        else if (std::abs(a2) > RS_TOLERANCE) {
            dSol.push_back(-a3 / a2);
        }

        double ds = 0.;

        for (double& d : dSol) {
            if (d > -RS_TOLERANCE && d < 1.0 + RS_TOLERANCE) {
                if (d < 0.0) {
                    d = 0.0;
                }
                if (d > 1.0) {
                    d = 1.0;
                }

                ds = -1.0;
                x1 = getQuadAtPoint(vx1, vc1, vx2, d);
                if (std::abs(x4.x) > RS_TOLERANCE) {
                    ds = (x1.x - vStart.x) / x4.x;
                }
                else if (std::abs(x4.y) > RS_TOLERANCE) {
                    ds = (x1.y - vStart.y) / x4.y;
                }

                if (ds > -RS_TOLERANCE && ds < 1.0 + RS_TOLERANCE) {
                    pVS->push_back(x1);
                }
            }
        }
    }
}

LC_SplinePointsData::LC_SplinePointsData(const bool closed, const bool cut) : closed{closed}, cut{cut} {
}

std::ostream& operator <<(std::ostream& os, const LC_SplinePointsData& ld) {
    os << "( closed: " << ld.closed << ")";
    return os;
}

// RS_SplinePoints

/**
 * Constructor.
 */
LC_SplinePoints::LC_SplinePoints(RS_EntityContainer* parent, LC_SplinePointsData d) : LC_CachedLengthEntity(parent), m_data(std::move(d)) {
    if (!m_data.useControlPoints) {
        updateControlPointsUI();
    }

    LC_SplinePoints::calculateBorders();
}

RS_Entity* LC_SplinePoints::clone() const {
    auto* l = new LC_SplinePoints(*this);
    return l;
}

void LC_SplinePoints::update() {
    updateControlPointsUI();
    calculateBorders();
}

void LC_SplinePoints::updateQuadExtentUI(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2) {
    RS_Vector locMinV = RS_Vector::minimum(x1, x2);
    RS_Vector locMaxV = RS_Vector::maximum(x1, x2);

    const RS_Vector vDer = x2 - c1 * 2.0 + x1;

    if (std::abs(vDer.x) > RS_TOLERANCE) {
        const double dt = (x1.x - c1.x) / vDer.x;
        if (dt > RS_TOLERANCE && dt < 1.0 - RS_TOLERANCE) {
            const double dx = x1.x * (1.0 - dt) * (1.0 - dt) + 2.0 * c1.x * dt * (1.0 - dt) + x2.x * dt * dt;
            locMinV.x = std::min(locMinV.x, dx);
            locMaxV.x = std::max(locMaxV.x, dx);
        }
    }

    if (std::abs(vDer.y) > RS_TOLERANCE) {
        const double dt = (x1.y - c1.y) / vDer.y;
        if (dt > RS_TOLERANCE && dt < 1.0 - RS_TOLERANCE) {
            const double dy = x1.y * (1.0 - dt) * (1.0 - dt) + 2.0 * c1.y * dt * (1.0 - dt) + x2.y * dt * dt;
            locMinV.y = std::min(locMinV.y, dy);
            locMaxV.y = std::max(locMaxV.y, dy);
        }
    }

    m_minV = RS_Vector::minimum(locMinV, m_minV);
    m_maxV = RS_Vector::maximum(locMaxV, m_maxV);
}

void LC_SplinePoints::calculateBorders() {
    m_minV = RS_Vector(false);
    m_maxV = RS_Vector(false);

    const size_t n = m_data.controlPoints.size();
    if (n < 1) {
        return;
    }

    RS_Vector vStart(false);
    RS_Vector vControl(false);
    RS_Vector vEnd(false);

    if (m_data.closed) {
        if (n < 3) {
            return;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;
        updateQuadExtentUI(vStart, vControl, vEnd);

        for (size_t i = 1; i < n - 1; ++i) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;
            updateQuadExtentUI(vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        updateQuadExtentUI(vStart, vControl, vEnd);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        m_minV = vStart;
        m_maxV = vStart;

        if (n < 2) {
            return;
        }

        vEnd = m_data.controlPoints.at(1);

        if (n < 3) {
            m_minV = RS_Vector::minimum(vEnd, m_minV);
            m_maxV = RS_Vector::maximum(vEnd, m_maxV);
            return;
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);

        if (n < 4) {
            updateQuadExtentUI(vStart, vControl, vEnd);
            return;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;
        updateQuadExtentUI(vStart, vControl, vEnd);

        for (size_t i = 2; i < n - 2; ++i) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;
            updateQuadExtentUI(vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);
        updateQuadExtentUI(vStart, vControl, vEnd);
    }
    updateLength();
}

RS_VectorSolutions LC_SplinePoints::getRefPoints() const {
    if (m_data.cut) {
        return {{m_data.controlPoints.begin(), m_data.controlPoints.end()}};
    }
    return {{m_data.splinePoints.begin(), m_data.splinePoints.end()}};
}

/** @return Start point of the entity */
RS_Vector LC_SplinePoints::getStartpoint() const {
    if (m_data.closed) {
        return RS_Vector(false);
    }

    const std::vector<RS_Vector>& pts = getPoints();
    const size_t iCount = pts.size();
    if (iCount < 1) {
        return RS_Vector(false);
    }
    return pts.at(0);
}

/** @return End point of the entity */
RS_Vector LC_SplinePoints::getEndpoint() const {
    if (m_data.closed) {
        return RS_Vector(false);
    }

    const std::vector<RS_Vector>& pts = getPoints();
    const size_t iCount = pts.size();

    return (iCount < 1) ? RS_Vector{false} : pts.at(iCount - 1);
}

RS_Vector LC_SplinePoints::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    double minDist = RS_MAXDOUBLE;
    RS_Vector ret(false);
    if (!m_data.closed) // no endpoint for closed spline
    {
        const RS_Vector vp1(getStartpoint());
        const RS_Vector vp2(getEndpoint());
        const double d1 = (coord - vp1).squared();
        const double d2 = (coord - vp2).squared();
        if (d1 < d2) {
            ret = vp1;
            minDist = std::sqrt(d1);
        }
        else {
            ret = vp2;
            minDist = std::sqrt(d2);
        }
    }
    if (dist != nullptr) {
        *dist = minDist;
    }
    if (entity != nullptr) {
        *entity = const_cast<LC_SplinePoints*>(this);
    }
    return ret;
}

// returns true if pvControl is set
int LC_SplinePoints::getQuadPoints(const int iSeg, RS_Vector* pvStart, RS_Vector* pvControl, RS_Vector* pvEnd) const {
    const size_t n = m_data.controlPoints.size();

    size_t i1 = iSeg - 1;
    size_t i2 = iSeg;
    size_t i3 = iSeg + 1;

    if (m_data.closed) {
        if (n < 3) {
            return 0;
        }

        i1 = (i1 + n - 1) % n;
        i2--;
        i3 = (i3 + n - 1) % n;

        *pvStart = (m_data.controlPoints.at(i1) + m_data.controlPoints.at(i2)) / 2.0;
        *pvControl = m_data.controlPoints.at(i2);
        *pvEnd = (m_data.controlPoints.at(i2) + m_data.controlPoints.at(i3)) / 2.0;
    }
    else {
        if (iSeg < 1) {
            return 0;
        }
        if (n < 1) {
            return 0;
        }

        *pvStart = m_data.controlPoints.at(0);

        if (n < 2) {
            return 1;
        }

        *pvEnd = m_data.controlPoints.at(1);

        if (n < 3) {
            return 2;
        }

        *pvControl = *pvEnd;
        *pvEnd = m_data.controlPoints.at(2);

        if (n < 4) {
            return 3;
        }

        if (i1 < 1) {
            *pvStart = m_data.controlPoints.at(0);
        }
        else {
            *pvStart = (m_data.controlPoints.at(i1) + m_data.controlPoints.at(i2)) / 2.0;
        }
        *pvControl = m_data.controlPoints.at(i2);
        if (i3 > n - 2) {
            *pvEnd = m_data.controlPoints.at(n - 1);
        }
        else {
            *pvEnd = (m_data.controlPoints.at(i2) + m_data.controlPoints.at(i3)) / 2.0;
        }
    }

    return 3;
}

// returns the index to the nearest segment, dt holds the t parameter
// we will make an extrodrinary exception here and make the index 1-based
// return values:
//   -1: no segment found
//   0: segment is one point only
//   >0: index to then non-degenerated segment, depends on closed flag
int LC_SplinePoints::getNearestQuad(const RS_Vector& coord, double* dist, double* dt) const {
    size_t n = m_data.controlPoints.size();

    RS_Vector vStart(false), vControl(false), vEnd(false), vRes(false);

    double dDist = 0., dNewDist = 0.;
    double dRes, dNewRes;
    int iRes = -1;

    if (m_data.closed) {
        if (n < 3) {
            return -1;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        dRes = getDistToQuadSquared(coord, vStart, vControl, vEnd, &dDist);
        iRes = 1;

        for (size_t i = 1; i < n - 1; ++i) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            dNewRes = getDistToQuadSquared(coord, vStart, vControl, vEnd, &dNewDist);
            if (setNewDist(true, dNewDist, dNewRes, &dDist, &dRes)) {
                iRes = i + 1;
            }
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;

        dNewRes = getDistToQuadSquared(coord, vStart, vControl, vEnd, &dNewDist);
        if (setNewDist(true, dNewDist, dNewRes, &dDist, &dRes)) {
            iRes = n;
        }
    }
    else {
        if (n < 1) {
            return -1;
        }

        vStart = m_data.controlPoints.at(0);

        if (n < 2) {
            if (dist != nullptr) {
                *dist = (coord - vStart).magnitude();
            }
            return 0;
        }

        vEnd = m_data.controlPoints.at(1);

        if (n < 3) {
            *dt = getDistToLine(coord, vStart, vEnd, &dDist);
            if (dist != nullptr) {
                *dist = std::sqrt(dDist);
            }
            return 1;
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);

        if (n < 4) {
            *dt = getDistToQuadSquared(coord, vStart, vControl, vEnd, &dDist);
            if (dist != nullptr) {
                *dist = std::sqrt(dDist);
            }
            return 1;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;

        dRes = getDistToQuadSquared(coord, vStart, vControl, vEnd, &dDist);
        iRes = 1;

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            dNewRes = getDistToQuadSquared(coord, vStart, vControl, vEnd, &dNewDist);
            if (setNewDist(true, dNewDist, dNewRes, &dDist, &dRes)) {
                iRes = i;
            }
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        dNewRes = getDistToQuadSquared(coord, vStart, vControl, vEnd, &dNewDist);
        if (setNewDist(true, dNewDist, dNewRes, &dDist, &dRes)) {
            iRes = n - 2;
        }
    }

    *dt = dRes;
    if (dist != nullptr) {
        *dist = std::sqrt(dDist);
    }
    return iRes;
}

RS_Vector LC_SplinePoints::doGetNearestPointOnEntity(const RS_Vector& coord, [[maybe_unused]] bool onEntity, double* dist,
                                                     RS_Entity** entity) const {
    RS_Vector vStart(false), vControl(false), vEnd(false), vRes(false);

    double dt = 0.0;
    const int iQuad = getNearestQuad(coord, dist, &dt);

    if (iQuad < 0) {
        return vRes;
    }

    const int n = getQuadPoints(iQuad, &vStart, &vControl, &vEnd);

    if (n < 1) {
        return vRes;
    }

    if (n < 2) {
        vRes = vStart;
    }
    else if (n < 3) {
        vRes = vStart * (1.0 - dt) + vEnd * dt;
    }
    else {
        vRes = getQuadAtPoint(vStart, vControl, vEnd, dt);
    }

    if (entity != nullptr) {
        *entity = const_cast<LC_SplinePoints*>(this);
    }
    return vRes;
}

double LC_SplinePoints::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, [[maybe_unused]] RS2::ResolveLevel level,
                                             [[maybe_unused]] double solidDist) const {
    double dDist = RS_MAXDOUBLE;
    getNearestPointOnEntity(coord, true, &dDist, entity);
    return dDist;
}

//RS_Vector LC_SplinePoints::getNearestCenter(const RS_Vector& /*coord*/,
//	double* dist) const
//{
//	if(dist != nullptr)
//	{
//		*dist = RS_MAXDOUBLE;
//	}

//	return RS_Vector(false);
//}

RS_Vector LC_SplinePoints::getSplinePointAtDist(double dDist, const int iStartSeg, const double dStartT, int* piSeg, double* pdt) const {
    RS_Vector vRes(false);
    if (m_data.closed) {
        return vRes;
    }

    RS_Vector vStart(false), vControl(false), vEnd(false);

    const size_t n = m_data.controlPoints.size();
    size_t i = iStartSeg;

    getQuadPoints(i, &vStart, &vControl, &vEnd);
    double dQuadDist = getQuadLength(vStart, vControl, vEnd, dStartT, 1.0);
    i++;

    while (dDist > dQuadDist && i < n - 2) {
        dDist -= dQuadDist;
        vStart = vEnd;
        vControl = m_data.controlPoints.at(i);
        vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;
        dQuadDist = getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);
        i++;
    }

    if (dDist > dQuadDist) {
        dDist -= dQuadDist;
        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);
        dQuadDist = getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);
        i++;
    }

    if (dDist <= dQuadDist) {
        double t0{0.0};
        if (static_cast<size_t>(iStartSeg + 1) == i) {
            t0 = dStartT;
        }
        const double dt = getQuadPointAtDist(vStart, vControl, vEnd, t0, dDist);
        vRes = getQuadPoint(vStart, vControl, vEnd, dt);
        *piSeg = i - 1;
        *pdt = dt;
    }

    return vRes;
}

RS_Vector LC_SplinePoints::doGetNearestMiddle(const RS_Vector& coord, double* dist, const int middlePoints) const {
    if (dist != nullptr) {
        *dist = RS_MAXDOUBLE;
    }
    RS_Vector vStart(false), vControl(false), vEnd(false), vNext(false), vRes(false);

    if (middlePoints < 1) {
        return vRes;
    }
    if (m_data.closed) {
        return vRes;
    }

    const size_t n = m_data.controlPoints.size();

    if (n < 1) {
        return vRes;
    }

    vStart = m_data.controlPoints.at(0);

    if (n < 2) {
        if (dist != nullptr) {
            *dist = (vStart - coord).magnitude();
        }
        return vStart;
    }

    vEnd = m_data.controlPoints.at(1);

    if (n < 3) {
        return getNearestMiddleLine(vStart, vEnd, coord, dist, middlePoints);
    }

    double dCurDist = 0., dt = 0.;
    double dMinDist = RS_MAXDOUBLE;
    const double dDist = getLength() / (1.0 + middlePoints);

    vControl = vEnd;
    vEnd = m_data.controlPoints.at(2);

    if (n < 4) {
        dt = getQuadPointAtDist(vStart, vControl, vEnd, 0.0, dDist);
        vRes = getQuadPoint(vStart, vControl, vEnd, dt);
        dMinDist = (vRes - coord).magnitude();
        for (int j = 1; j < middlePoints; j++) {
            dt = getQuadPointAtDist(vStart, vControl, vEnd, dt, dDist);
            vNext = getQuadPoint(vStart, vControl, vEnd, dt);
            dCurDist = (vNext - coord).magnitude();

            if (dCurDist < dMinDist) {
                dMinDist = dCurDist;
                vRes = vNext;
            }
        }

        if (dist != nullptr) {
            *dist = dMinDist;
        }
        return vRes;
    }

    int iNext{0};
    vRes = getSplinePointAtDist(dDist, 1, 0.0, &iNext, &dt);
    if (vRes.valid) {
        dMinDist = (vRes - coord).magnitude();
    }
    int i = 2;
    while (vRes.valid && i <= middlePoints) {
        vNext = getSplinePointAtDist(dDist, iNext, dt, &iNext, &dt);
        dCurDist = (vNext - coord).magnitude();

        if (vNext.valid && dCurDist < dMinDist) {
            dMinDist = dCurDist;
            vRes = vNext;
        }
        i++;
    }

    if (dist != nullptr) {
        *dist = dMinDist;
    }
    return vRes;
}

RS_Vector LC_SplinePoints::doGetNearestDist([[maybe_unused]] double distance, [[maybe_unused]] const RS_Vector& coord, double* dist) const {
    printf("getNearestDist\n");
    if (dist != nullptr) {
        *dist = RS_MAXDOUBLE;
    }

    return RS_Vector(false);
}

void LC_SplinePoints::move(const RS_Vector& offset) {
    for (auto& v : m_data.splinePoints) {
        v.move(offset);
    }
    for (auto& v : m_data.controlPoints) {
        v.move(offset);
    }
    update();
}

void LC_SplinePoints::rotate(const RS_Vector& center, const double angle) {
    rotate(center, RS_Vector(angle));
}

void LC_SplinePoints::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (auto& v : m_data.splinePoints) {
        v.rotate(center, angleVector);
    }
    for (auto& v : m_data.controlPoints) {
        v.rotate(center, angleVector);
    }
    update();
}

void LC_SplinePoints::scale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& v : m_data.splinePoints) {
        v.scale(center, factor);
    }
    for (auto& v : m_data.controlPoints) {
        v.scale(center, factor);
    }
    update();
}

void LC_SplinePoints::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (auto& v : m_data.splinePoints) {
        v.mirror(axisPoint1, axisPoint2);
    }
    for (auto& v : m_data.controlPoints) {
        v.mirror(axisPoint1, axisPoint2);
    }
    update();
}

RS_Entity& LC_SplinePoints::shear(const double k) {
    for (auto& v : m_data.splinePoints) {
        v.shear(k);
    }
    for (auto& v : m_data.controlPoints) {
        v.shear(k);
    }
    update();
    return *this;
}

void LC_SplinePoints::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    for (auto& v : m_data.splinePoints) {
        // fixme - magic value - replace by constant
        if (ref.distanceTo(v) < 1.0e-4) {
            v.move(offset);
        }
    }
    for (auto& v : m_data.controlPoints) {
        // fixme - magic value - replace by constant
        if (ref.distanceTo(v) < 1.0e-4) {
            v.move(offset);
        }
    }
    update();
}

void LC_SplinePoints::revertDirection() {
    size_t j = m_data.splinePoints.size() - 1;
    for (size_t k = 0; k < m_data.splinePoints.size() / 2; ++k) {
        std::swap(m_data.splinePoints[k], m_data.splinePoints[j--]);
    }
    j = m_data.controlPoints.size() - 1;
    for (size_t k = 0; k < m_data.controlPoints.size() / 2; ++k) {
        std::swap(m_data.controlPoints[k], m_data.controlPoints[j--]);
    }
    update();
}

/**
 * @return The reference points of the spline.
 */
const std::vector<RS_Vector>& LC_SplinePoints::getPoints() const {
    if (m_data.cut) {
        return m_data.controlPoints;
    }
    return m_data.splinePoints;
}

const std::vector<RS_Vector>& LC_SplinePoints::getControlPoints() const {
    return m_data.controlPoints;
}

// fixme - sand - this method is used only for writing spline points... do we really neede a copy of the vector there?
std::vector<RS_Vector> LC_SplinePoints::getStrokePoints() const {
    const int p1 = getGraphicVariableInt("$SPLINESEGS", 8);
    std::vector<RS_Vector> result;
    fillStrokePoints(p1, result);
    return result;
}

void LC_SplinePoints::fillStrokePoints(const int splineSegments, std::vector<RS_Vector>& points) const {
    size_t iSplines = m_data.controlPoints.size();
    if (!m_data.closed) {
        iSplines -= 2;
    }

    RS_Vector vStart(false), vControl(false), vEnd(false);
    for (size_t i = 1; i <= iSplines; ++i) {
        const int iPts = getQuadPoints(i, &vStart, &vControl, &vEnd);
        if (iPts > 2) {
            strokeQuad(&points, vStart, vControl, vEnd, splineSegments);
        }
        else if (iPts > 1) {
            points.push_back(vStart);
        }
    }

    if (!m_data.closed && vEnd.valid) {
        points.push_back(vEnd);
    }
}

/**
 * push_backs the given point to the control points.
 */
bool LC_SplinePoints::addPoint(const RS_Vector& v) {
    if (m_data.cut) {
        return false;
    }

    if (m_data.splinePoints.empty() || (v - m_data.splinePoints.back()).squared() > RS_TOLERANCE2) {
        m_data.splinePoints.push_back(v);
        return true;
    }
    return false;
}

/**
 * Removes the control point that was last added.
 */
void LC_SplinePoints::removeLastPoint() {
    m_data.splinePoints.pop_back();
}

void LC_SplinePoints::addControlPoint(const RS_Vector& v) {
    m_data.controlPoints.push_back(v);
}

std::vector<double> getMatrix(const size_t iCount, const bool bClosed, const std::vector<double>& dt) {
    if (bClosed && (iCount < 3 || dt.size() != iCount)) {
        return {};
    }
    if (!bClosed && (iCount < 4 || dt.size() != iCount - 2)) {
        return {};
    }

    // closed: iDim = 5*iCount - 6; // n + 2*(n - 1) + 2*(n - 2)
    // not closed: iDim = 3*iCount - 8; // (n - 2) + 2*(n - 3)
    const int iDim = bClosed ? 5 * iCount - 6 : 3 * iCount - 8;

    std::vector<double> dRes(iDim);

    if (bClosed) {
        double* pdDiag = dRes.data();
        double* pdDiag1 = &dRes[iCount];
        double* pdDiag2 = &dRes[2 * iCount - 1];
        double* pdLastCol1 = &dRes[3 * iCount - 2];
        double* pdLastCol2 = &dRes[4 * iCount - 4];

        double x1 = (1.0 - dt[0]) * (1.0 - dt[0]) / 2.0;
        double x3 = dt[0] * dt[0] / 2.0;
        double x2 = x1 + 2.0 * dt[0] * (1.0 - dt[0]) + x3;

        pdDiag[0] = std::sqrt(x2);
        pdDiag1[0] = x3 / pdDiag[0];
        pdLastCol1[0] = x1 / pdDiag[0];

        x1 = (1.0 - dt[1]) * (1.0 - dt[1]) / 2.0;
        x3 = dt[1] * dt[1] / 2.0;
        x2 = x1 + 2.0 * dt[1] * (1.0 - dt[1]) + x3;

        pdDiag2[0] = x1 / pdDiag[0];

        pdDiag[1] = std::sqrt(x2 - pdDiag1[0] * pdDiag2[0]);
        pdDiag1[1] = x3 / pdDiag[1];
        pdLastCol1[1] = -pdDiag2[0] * pdLastCol1[0] / pdDiag[1];

        for (size_t i = 2; i < iCount - 2; i++) {
            x1 = (1.0 - dt[i]) * (1.0 - dt[i]) / 2.0;
            x3 = dt[i] * dt[i] / 2.0;
            x2 = x1 + 2.0 * dt[i] * (1.0 - dt[i]) + x3;

            pdDiag2[i - 1] = x1 / pdDiag[i - 1];

            pdDiag[i] = std::sqrt(x2 - pdDiag1[i - 1] * pdDiag2[i - 1]);
            pdDiag1[i] = x3 / pdDiag[i];
            pdLastCol1[i] = -pdDiag2[i - 1] * pdLastCol1[i - 1] / pdDiag[i];
        }
        x1 = (1.0 - dt[iCount - 2]) * (1.0 - dt[iCount - 2]) / 2.0;
        x3 = dt[iCount - 2] * dt[iCount - 2] / 2.0;
        x2 = x1 + 2.0 * dt[iCount - 2] * (1.0 - dt[iCount - 2]) + x3;

        pdDiag2[iCount - 3] = x1 / pdDiag[iCount - 3];

        pdDiag[iCount - 2] = std::sqrt(x2 - pdDiag1[iCount - 3] * pdDiag2[iCount - 3]);
        pdDiag1[iCount - 2] = (x3 - pdDiag2[iCount - 3] * pdLastCol1[iCount - 3]) / pdDiag[iCount - 2];

        x1 = (1.0 - dt[iCount - 1]) * (1.0 - dt[iCount - 1]) / 2.0;
        x3 = dt[iCount - 1] * dt[iCount - 1] / 2.0;
        x2 = x1 + 2.0 * dt[iCount - 1] * (1.0 - dt[iCount - 1]) + x3;

        pdLastCol2[0] = x3 / pdDiag[0];
        double dLastColSum = pdLastCol1[0] * pdLastCol2[0];
        for (size_t i = 1; i < iCount - 2; i++) {
            pdLastCol2[i] = -pdLastCol2[i - 1] * pdDiag1[i - 1] / pdDiag[i];
            dLastColSum += pdLastCol1[i] * pdLastCol2[i];
        }

        pdDiag2[iCount - 2] = (x1 - pdDiag1[iCount - 3] * pdLastCol2[iCount - 3]) / pdDiag[iCount - 2];

        dLastColSum += pdDiag1[iCount - 2] * pdDiag2[iCount - 2];
        pdDiag[iCount - 1] = std::sqrt(x2 - dLastColSum);
    }
    else {
        double* pdDiag = dRes.data();
        double* pdDiag1 = &dRes[iCount - 2];
        double* pdDiag2 = &dRes[2 * iCount - 5];

        double x3 = dt[0] * dt[0] / 2.0;
        double x2 = 2.0 * dt[0] * (1.0 - dt[0]) + x3;
        pdDiag[0] = std::sqrt(x2);
        pdDiag1[0] = x3 / pdDiag[0];

        for (size_t i = 1; i < iCount - 3; i++) {
            const double x1 = (1.0 - dt[i]) * (1.0 - dt[i]) / 2.0;
            x3 = dt[i] * dt[i] / 2.0;
            x2 = x1 + 2.0 * dt[i] * (1.0 - dt[i]) + x3;

            pdDiag2[i - 1] = x1 / pdDiag[i - 1];
            pdDiag[i] = std::sqrt(x2 - pdDiag1[i - 1] * pdDiag2[i - 1]);
            pdDiag1[i] = x3 / pdDiag[i];
        }

        const double x1 = (1.0 - dt[iCount - 3]) * (1.0 - dt[iCount - 3]) / 2.0;
        x2 = x1 + 2.0 * dt[iCount - 3] * (1.0 - dt[iCount - 3]);
        pdDiag2[iCount - 4] = x1 / pdDiag[iCount - 4];
        pdDiag[iCount - 3] = std::sqrt(x2 - pdDiag1[iCount - 4] * pdDiag2[iCount - 4]);
    }

    return dRes;
}

void LC_SplinePoints::updateControlPointsUI() {
    if (m_data.cut) {
        return; // no update after trim operation
    }

    if (!m_data.useControlPoints) {
        m_data.controlPoints.clear();
    }

    const size_t n = m_data.splinePoints.size();

    if (m_data.closed && n < 3) {
        if (n > 0) {
            m_data.controlPoints.push_back(m_data.splinePoints.at(0));
        }
        if (n > 1) {
            m_data.controlPoints.push_back(m_data.splinePoints.at(1));
        }
        return;
    }

    if (!m_data.closed && n < 4) {
        // use control points directly, reserved for parabola
        if (m_data.useControlPoints && m_data.controlPoints.size() == 3) {
            return;
        }
        if (n > 0) {
            m_data.controlPoints.push_back(m_data.splinePoints.at(0));
        }
        if (n > 2) {
            const RS_Vector vControl = getThreePointsControl(m_data.splinePoints.at(0), m_data.splinePoints.at(1),
                                                             m_data.splinePoints.at(2));
            if (vControl.valid) {
                m_data.controlPoints.push_back(vControl);
            }
        }
        if (n > 1) {
            m_data.controlPoints.push_back(m_data.splinePoints.at(n - 1));
        }
        return;
    }

    const int iDim = m_data.closed ? n : n - 2;

    std::vector<double> dt(iDim);

    if (m_data.closed) {
        double dl1 = (m_data.splinePoints.at(n - 1) - m_data.splinePoints.at(0)).magnitude();
        double dl2 = (m_data.splinePoints.at(1) - m_data.splinePoints.at(0)).magnitude();
        dt[0] = dl1 / (dl1 + dl2);
        for (int i = 1; i < iDim - 1; i++) {
            dl1 = dl2;
            dl2 = (m_data.splinePoints.at(i + 1) - m_data.splinePoints.at(i)).magnitude();
            dt[i] = dl1 / (dl1 + dl2);
        }
        dl1 = (m_data.splinePoints.at(n - 1) - m_data.splinePoints.at(n - 2)).magnitude();
        dl2 = (m_data.splinePoints.at(0) - m_data.splinePoints.at(n - 1)).magnitude();
        dt[iDim - 1] = dl1 / (dl1 + dl2);
    }
    else {
        double dl1 = (m_data.splinePoints.at(1) - m_data.splinePoints.at(0)).magnitude();
        double dl2 = (m_data.splinePoints.at(2) - m_data.splinePoints.at(1)).magnitude();
        dt[0] = dl1 / (dl1 + dl2 / 2.0);
        for (int i = 1; i < iDim - 1; i++) {
            dl1 = dl2;
            dl2 = (m_data.splinePoints.at(i + 2) - m_data.splinePoints.at(i + 1)).magnitude();
            dt[i] = dl1 / (dl1 + dl2);
        }
        dl1 = dl2;
        dl2 = (m_data.splinePoints.at(iDim) - m_data.splinePoints.at(iDim + 1)).magnitude();
        dt[iDim - 1] = dl1 / (dl1 + 2.0 * dl2);
    }

    const std::vector<double> pdMatrix = getMatrix(n, m_data.closed, dt);

    if (pdMatrix.empty()) {
        return;
    }

    std::vector<double> dx(iDim);
    std::vector<double> dy(iDim);
    std::vector<double> dx2(iDim);
    std::vector<double> dy2(iDim);

    if (m_data.closed) {
        const double* pdDiag = pdMatrix.data();
        const double* pdDiag1 = &pdMatrix[n];
        const double* pdDiag2 = &pdMatrix[2 * n - 1];
        const double* pdLastCol1 = &pdMatrix[3 * n - 2];
        const double* pdLastCol2 = &pdMatrix[4 * n - 4];

        dx[0] = m_data.splinePoints.at(0).x / pdDiag[0];
        dy[0] = m_data.splinePoints.at(0).y / pdDiag[0];
        for (int i = 1; i < iDim - 1; i++) {
            dx[i] = (m_data.splinePoints.at(i).x - pdDiag2[i - 1] * dx[i - 1]) / pdDiag[i];
            dy[i] = (m_data.splinePoints.at(i).y - pdDiag2[i - 1] * dy[i - 1]) / pdDiag[i];
        }

        dx[iDim - 1] = m_data.splinePoints.at(iDim - 1).x - pdDiag2[iDim - 2] * dx[iDim - 2];
        dy[iDim - 1] = m_data.splinePoints.at(iDim - 1).y - pdDiag2[iDim - 2] * dy[iDim - 2];
        for (int i = 0; i < iDim - 2; i++) {
            dx[iDim - 1] -= dx[i] * pdLastCol2[i];
            dy[iDim - 1] -= dy[i] * pdLastCol2[i];
        }
        dx[iDim - 1] /= pdDiag[iDim - 1];
        dy[iDim - 1] /= pdDiag[iDim - 1];

        dx2[iDim - 1] = dx[iDim - 1] / pdDiag[iDim - 1];
        dy2[iDim - 1] = dy[iDim - 1] / pdDiag[iDim - 1];
        dx2[iDim - 2] = (dx[iDim - 2] - pdDiag1[iDim - 2] * dx2[iDim - 1]) / pdDiag[iDim - 2];
        dy2[iDim - 2] = (dy[iDim - 2] - pdDiag1[iDim - 2] * dy2[iDim - 1]) / pdDiag[iDim - 2];

        for (int i = iDim - 3; i >= 0; i--) {
            dx2[i] = (dx[i] - pdDiag1[i] * dx2[i + 1] - pdLastCol1[i] * dx2[iDim - 1]) / pdDiag[i];
            dy2[i] = (dy[i] - pdDiag1[i] * dy2[i + 1] - pdLastCol1[i] * dy2[iDim - 1]) / pdDiag[i];
        }

        for (int i = 0; i < iDim; i++) {
            m_data.controlPoints.emplace_back(dx2[i], dy2[i]);
        }
    }
    else {
        const double* pdDiag = pdMatrix.data();
        const double* pdDiag1 = &pdMatrix[n - 2];
        const double* pdDiag2 = &pdMatrix[2 * n - 5];

        dx[0] = (m_data.splinePoints.at(1).x - m_data.splinePoints.at(0).x * (1.0 - dt[0]) * (1.0 - dt[0])) / pdDiag[0];
        dy[0] = (m_data.splinePoints.at(1).y - m_data.splinePoints.at(0).y * (1.0 - dt[0]) * (1.0 - dt[0])) / pdDiag[0];
        for (int i = 1; i < iDim - 1; i++) {
            dx[i] = (m_data.splinePoints.at(i + 1).x - pdDiag2[i - 1] * dx[i - 1]) / pdDiag[i];
            dy[i] = (m_data.splinePoints.at(i + 1).y - pdDiag2[i - 1] * dy[i - 1]) / pdDiag[i];
        }
        dx[iDim - 1] = ((m_data.splinePoints.at(iDim).x - m_data.splinePoints.at(iDim + 1).x * dt[n - 3] * dt[n - 3]) - pdDiag2[iDim - 2] *
            dx[iDim - 2]) / pdDiag[iDim - 1];
        dy[iDim - 1] = ((m_data.splinePoints.at(iDim).y - m_data.splinePoints.at(iDim + 1).y * dt[n - 3] * dt[n - 3]) - pdDiag2[iDim - 2] *
            dy[iDim - 2]) / pdDiag[iDim - 1];

        dx2[iDim - 1] = dx[iDim - 1] / pdDiag[iDim - 1];
        dy2[iDim - 1] = dy[iDim - 1] / pdDiag[iDim - 1];

        for (int i = iDim - 2; i >= 0; i--) {
            dx2[i] = (dx[i] - pdDiag1[i] * dx2[i + 1]) / pdDiag[i];
            dy2[i] = (dy[i] - pdDiag1[i] * dy2[i + 1]) / pdDiag[i];
        }

        m_data.controlPoints.push_back(m_data.splinePoints.at(0));
        for (int i = 0; i < iDim; i++) {
            m_data.controlPoints.emplace_back(dx2[i], dy2[i]);
        }
        m_data.controlPoints.push_back(m_data.splinePoints.at(n - 1));
    }
}

double getLinePointAtDist(const double dLen, const double t1, const double dDist) {
    return t1 + dDist / dLen;
}

// returns new pattern offset;
double drawPatternLine(const std::vector<double>& pdPattern, const int iPattern, double patternOffset, QPainterPath& qPath,
                       const RS_Vector& x1, const RS_Vector& x2) {
    const double dLen = (x2 - x1).magnitude();
    if (dLen < RS_TOLERANCE) {
        return patternOffset;
    }

    int i = 0;
    double dCurSegLen = 0.0;
    double dSegOffs = 0.0;
    while (patternOffset > RS_TOLERANCE) {
        if (i >= iPattern) {
            i = 0;
        }
        dCurSegLen = std::abs(pdPattern[i++]);
        if (patternOffset > dCurSegLen) {
            patternOffset -= dCurSegLen;
        }
        else {
            dSegOffs = patternOffset;
            patternOffset = 0.0;
        }
    }
    if (i > 0) {
        i--;
    }

    dCurSegLen = std::abs(pdPattern[i]) - dSegOffs;
    dSegOffs = 0.0;

    double dt1 = 0.0;
    double dt2 = 1.0;
    double dCurLen = dLen;
    if (dCurSegLen < dCurLen) {
        //        double dt2bak=dt1;
        dt2 = getLinePointAtDist(dLen, dt1, dCurSegLen);
        dCurLen -= dCurSegLen;
    }
    else {
        dSegOffs = dCurLen;
        dCurLen = 0.0;
    }

    RS_Vector p2 = x1 * (1.0 - dt2) + x2 * dt2;
    if (pdPattern[i] < 0) {
        qPath.moveTo(QPointF(p2.x, p2.y));
    }
    else {
        qPath.lineTo(QPointF(p2.x, p2.y));
    }

    i++;
    dt1 = dt2;

    while (dCurLen > RS_TOLERANCE) {
        if (i >= iPattern) {
            i = 0;
        }

        dCurSegLen = std::abs(pdPattern[i]);
        if (dCurLen > dCurSegLen) {
            dt2 = getLinePointAtDist(dLen, dt1, dCurSegLen);
            dCurLen -= dCurSegLen;
        }
        else {
            dt2 = 1.0;
            dSegOffs = dCurLen;
            dCurLen = 0.0;
        }

        p2 = x1 * (1.0 - dt2) + x2 * dt2;
        if (pdPattern[i] < 0) {
            qPath.moveTo(QPointF(p2.x, p2.y));
        }
        else {
            qPath.lineTo(QPointF(p2.x, p2.y));
        }

        i++;
        dt1 = dt2;
    }

    i--;

    while (i > 0) {
        dSegOffs += std::abs(pdPattern[--i]);
    }
    return dSegOffs;
}

// returns new pattern offset;
double drawPatternQuad(const std::vector<double>& pdPattern, const int iPattern, double patternOffset, QPainterPath& qPath,
                       const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2) {
    const double dLen = getQuadLength(x1, c1, x2, 0.0, 1.0);
    if (dLen < RS_TOLERANCE) {
        return patternOffset;
    }

    int i = 0;
    double dCurSegLen = 0.0;
    double dSegOffs = 0.0;
    while (patternOffset > RS_TOLERANCE) {
        if (i >= iPattern) {
            i = 0;
        }
        dCurSegLen = std::abs(pdPattern[i++]);
        if (patternOffset > dCurSegLen) {
            patternOffset -= dCurSegLen;
        }
        else {
            dSegOffs = patternOffset;
            patternOffset = 0.0;
        }
    }
    if (i > 0) {
        i--;
    }

    dCurSegLen = std::abs(pdPattern[i]) - dSegOffs;
    dSegOffs = 0.0;

    double dt1 = 0.0;
    double dt2 = 1.0;
    double dCurLen = dLen;
    if (dCurSegLen < dCurLen) {
        dt2 = getQuadPointAtDist(x1, c1, x2, dt1, dCurSegLen);
        dCurLen -= dCurSegLen;
    }
    else {
        dSegOffs = dCurLen;
        dCurLen = 0.0;
    }

    RS_Vector c2;

    RS_Vector p2 = getQuadPoint(x1, c1, x2, dt2);
    if (pdPattern[i] < 0) {
        qPath.moveTo(QPointF(p2.x, p2.y));
    }
    else {
        c2 = getSubQuadControlPoint(x1, c1, x2, dt1, dt2);
        qPath.quadTo(QPointF(c2.x, c2.y), QPointF(p2.x, p2.y));
    }

    i++;
    dt1 = dt2;

    while (dCurLen > RS_TOLERANCE) {
        if (i >= iPattern) {
            i = 0;
        }

        dCurSegLen = std::abs(pdPattern[i]);
        if (dCurLen > dCurSegLen) {
            dt2 = getQuadPointAtDist(x1, c1, x2, dt1, dCurSegLen);
            dCurLen -= dCurSegLen;
        }
        else {
            dt2 = 1.0;
            dSegOffs = dCurLen;
            dCurLen = 0.0;
        }

        p2 = getQuadPoint(x1, c1, x2, dt2);
        if (pdPattern[i] < 0) {
            qPath.moveTo(QPointF(p2.x, p2.y));
        }
        else {
            c2 = getSubQuadControlPoint(x1, c1, x2, dt1, dt2);
            qPath.quadTo(QPointF(c2.x, c2.y), QPointF(p2.x, p2.y));
        }

        i++;
        dt1 = dt2;
    }

    i--;

    while (i > 0) {
        dSegOffs += std::abs(pdPattern[--i]);
    }
    return dSegOffs;
}

void LC_SplinePoints::draw(RS_Painter* painter) {
    // Adjust dash offset
    painter->updateDashOffset(this);
    painter->drawSplinePointsWCS(m_data.controlPoints, m_data.closed);
}

void LC_SplinePoints::updateLength() {
    size_t n = m_data.controlPoints.size();

    if (n < 2) {
        m_cachedLength = 0;
        return;
    }

    RS_Vector vStart(false), vControl(false), vEnd(false);

    //UpdateControlPoints();

    double dRes = 0.0;

    if (m_data.closed) {
        if (n < 3) {
            m_cachedLength = 0.0;
            return;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        dRes = getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);

        for (size_t i = 1; i < n - 1; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            dRes += getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;

        dRes += getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);
        if (n < 3) {
            m_cachedLength = (vEnd - vStart).magnitude();
            return;
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            m_cachedLength = getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);
            return;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;

        dRes = getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            dRes += getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        dRes += getQuadLength(vStart, vControl, vEnd, 0.0, 1.0);
    }

    m_cachedLength = dRes;
}

double LC_SplinePoints::getDirection1() const {
    const size_t n = m_data.controlPoints.size();

    if (n < 2) {
        return 0.0;
    }

    RS_Vector vStart, vEnd;

    if (m_data.closed) {
        if (n < 3) {
            return 0.0;
        }
        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vEnd = m_data.controlPoints.at(0);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);
    }

    return vStart.angleTo(vEnd);
}

double LC_SplinePoints::getDirection2() const {
    const size_t n = m_data.controlPoints.size();

    if (n < 2) {
        return 0.0;
    }

    RS_Vector vStart, vEnd;

    if (m_data.closed) {
        if (n < 3) {
            return 0.0;
        }
        vStart = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
    }
    else {
        vStart = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);
    }

    return vEnd.angleTo(vStart);
}

RS_VectorSolutions LC_SplinePoints::getTangentPoint(const RS_Vector& point) const {
    RS_VectorSolutions ret;
    size_t n = m_data.controlPoints.size();

    if (n < 3) {
        return ret;
    }

    RS_Vector vStart(false), vControl(false), vEnd(false);

    if (m_data.closed) {
        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        addQuadTangentPoints(&ret, point, vStart, vControl, vEnd);

        for (size_t i = 1; i < n - 1; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addQuadTangentPoints(&ret, point, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;

        addQuadTangentPoints(&ret, point, vStart, vControl, vEnd);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vControl = m_data.controlPoints.at(1);
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            addQuadTangentPoints(&ret, point, vStart, vControl, vEnd);
            return ret;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;

        addQuadTangentPoints(&ret, point, vStart, vControl, vEnd);

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addQuadTangentPoints(&ret, point, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        addQuadTangentPoints(&ret, point, vStart, vControl, vEnd);
    }

    return ret;
}

RS_Vector LC_SplinePoints::getTangentDirection(const RS_Vector& point) const {
    const size_t n = m_data.controlPoints.size();

    RS_Vector vStart(false), vControl(false), vEnd(false), vRes(false);

    if (n < 2) {
        return vStart;
    }

    double dt = 0.0;
    const int iQuad = getNearestQuad(point, nullptr, &dt);
    if (iQuad < 0) {
        return vStart;
    }

    const int i = getQuadPoints(iQuad, &vStart, &vControl, &vEnd);

    if (i < 2) {
        return vStart;
    }
    if (i < 3) {
        vRes = vEnd - vStart;
    }
    else {
        vRes = getQuadDirAtPoint(vStart, vControl, vEnd, dt);
    }

    return vRes;
}

LC_SplinePointsData gddLineOffset(const RS_Vector& vx1, const RS_Vector& vx2, const double distance) {
    LC_SplinePointsData ret(false, false);

    double dDist = (vx2 - vx1).magnitude();

    if (dDist < RS_TOLERANCE) {
        return ret;
    }

    dDist = distance / dDist;

    ret.splinePoints.emplace_back(vx1.x - dDist * (vx2.y - vx1.y), vx1.y + dDist * (vx2.x - vx1.x));
    ret.splinePoints.emplace_back(vx2.x - dDist * (vx2.y - vx1.y), vx2.y + dDist * (vx2.x - vx1.x));
    return ret;
}

bool LC_SplinePoints::offsetCut(const RS_Vector& coord, const double& distance) {
    size_t n = m_data.controlPoints.size();
    if (n < 2) {
        return false;
    }

    double dt;
    int iQuad = getNearestQuad(coord, nullptr, &dt);
    if (iQuad < 0) {
        return false;
    }

    RS_Vector vStart(false), vEnd(false), vControl(false);
    RS_Vector vPoint(false), vTan(false);

    if (getQuadPoints(iQuad, &vStart, &vControl, &vEnd)) {
        vPoint = getQuadAtPoint(vStart, vControl, vEnd, dt);
        vTan = getQuadDirAtPoint(vStart, vControl, vEnd, dt);
    }
    else {
        vPoint = vEnd * (1.0 - dt) - vStart * dt;
        vTan = vEnd - vStart;
    }

    double dDist = distance;
    if ((coord.x - vPoint.x) * vTan.y - (coord.y - vPoint.y) * vTan.x > 0) {
        dDist *= -1.0;
    }

    LC_SplinePointsData spd(m_data.closed, false);

    if (m_data.closed) {
        if (n < 3) {
            return false;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }

        for (size_t i = 1; i < n - 1; i++) {
            vStart = (m_data.controlPoints.at(i - 1) + m_data.controlPoints.at(i)) / 2.0;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
            vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
            }
        }

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);

        if (n < 3) {
            spd = gddLineOffset(vStart, vEnd, dDist);
            bool bRes = spd.splinePoints.size() > 0;
            if (bRes) {
                m_data = spd;
                update();
                m_data.cut = true;
            }
            return bRes;
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vStart.x - dDist * vTan.y, vStart.y + dDist * vTan.x);
            }

            vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
            vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
            }

            vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vEnd.x - dDist * vTan.y, vEnd.y + dDist * vTan.x);
            }

            m_data = spd;
            update();
            m_data.cut = true;
            return true;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;

        vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vStart.x - dDist * vTan.y, vStart.y + dDist * vTan.x);
        }

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
            vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
            }
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }

        vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vEnd.x - dDist * vTan.y, vEnd.y + dDist * vTan.x);
        }
    }
    m_data = spd;
    update();
    m_data.cut = true;
    return true;
}

bool LC_SplinePoints::offsetSpline(const RS_Vector& coord, const double& distance) {
    size_t iPoints = m_data.splinePoints.size();
    size_t n = m_data.controlPoints.size();

    if (iPoints < 2) {
        return false;
    }
    if (n < 2) {
        return false;
    }

    double dt;
    int iQuad = getNearestQuad(coord, nullptr, &dt);
    if (iQuad < 0) {
        return false;
    }

    RS_Vector vStart(false), vEnd(false), vControl(false);
    RS_Vector vPoint(false), vTan(false);

    if (getQuadPoints(iQuad, &vStart, &vControl, &vEnd)) {
        vPoint = getQuadAtPoint(vStart, vControl, vEnd, dt);
        vTan = getQuadDirAtPoint(vStart, vControl, vEnd, dt);
    }
    else {
        vPoint = vEnd * (1.0 - dt) - vStart * dt;
        vTan = vEnd - vStart;
    }

    double dDist = distance;
    if ((coord.x - vPoint.x) * vTan.y - (coord.y - vPoint.y) * vTan.x > 0) {
        dDist *= -1.0;
    }

    LC_SplinePointsData spd(m_data.closed, m_data.cut);

    double dl1, dl2;

    if (m_data.closed) {
        if (n < 3) {
            return false;
        }

        vPoint = m_data.splinePoints.at(0);

        dl1 = (m_data.splinePoints.at(iPoints - 1) - vPoint).magnitude();
        dl2 = (m_data.splinePoints.at(1) - vPoint).magnitude();
        dt = dl1 / (dl1 + dl2);

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }

        for (size_t i = 1; i < n - 1; i++) {
            vPoint = m_data.splinePoints.at(i);

            dl1 = dl2;
            dl2 = (m_data.splinePoints.at(i + 1) - vPoint).magnitude();
            dt = dl1 / (dl1 + dl2);

            vStart = (m_data.controlPoints.at(i - 1) + m_data.controlPoints.at(i)) / 2.0;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vTan = getQuadDir(vStart, vControl, vEnd, dt);

            if (vTan.valid) {
                spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
            }
        }

        vPoint = m_data.splinePoints.at(iPoints - 1);
        dl1 = (vPoint - m_data.splinePoints.at(iPoints - 2)).magnitude();
        dl2 = (vPoint - m_data.splinePoints.at(0)).magnitude();
        dt = dl1 / (dl1 + dl2);

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);

        if (n < 3) {
            spd = gddLineOffset(vStart, vEnd, dDist);
            bool bRes = !spd.splinePoints.empty();
            if (bRes) {
                m_data = spd;
            }
            return bRes;
        }

        vPoint = m_data.splinePoints.at(1);

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            dl1 = (vPoint - vStart).magnitude();
            dl2 = (vEnd - vPoint).magnitude();
            dt = dl1 / (dl1 + dl2);

            vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vStart.x - dDist * vTan.y, vStart.y + dDist * vTan.x);
            }

            vTan = getQuadDir(vStart, vControl, vEnd, dt);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
            }

            vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vEnd.x - dDist * vTan.y, vEnd.y + dDist * vTan.x);
            }

            m_data = spd;
            return true;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;

        vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vStart.x - dDist * vTan.y, vStart.y + dDist * vTan.x);
        }

        dl1 = (vPoint - m_data.splinePoints.at(0)).magnitude();
        dl2 = (m_data.splinePoints.at(2) - vPoint).magnitude();
        dt = dl1 / (dl1 + dl2 / 2.0);

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }

        for (size_t i = 2; i < n - 2; i++) {
            vPoint = m_data.splinePoints.at(i);

            dl1 = dl2;
            dl2 = (m_data.splinePoints.at(i + 1) - vPoint).magnitude();
            dt = dl1 / (dl1 + dl2);

            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vTan = getQuadDir(vStart, vControl, vEnd, dt);
            if (vTan.valid) {
                spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
            }
        }

        vPoint = m_data.splinePoints.at(n - 2);

        dl1 = dl2;
        dl2 = (vPoint - m_data.splinePoints.at(n - 1)).magnitude();
        dt = dl1 / (dl1 + 2.0 * dl2);

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vPoint.x - dDist * vTan.y, vPoint.y + dDist * vTan.x);
        }

        vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
        if (vTan.valid) {
            spd.splinePoints.emplace_back(vEnd.x - dDist * vTan.y, vEnd.y + dDist * vTan.x);
        }
    }
    m_data = spd;
    return true;
}

bool LC_SplinePoints::offset(const RS_Vector& coord, const double distance) {
    if (m_data.cut) {
        return offsetCut(coord, distance);
    }
    return offsetSpline(coord, distance);
}

std::vector<RS_Entity*> addLineOffsets(const RS_Vector& vx1, const RS_Vector& vx2, const double& distance) {
    std::vector<RS_Entity*> ret(0, nullptr);

    double dDist = (vx2 - vx1).magnitude();

    if (dDist < RS_TOLERANCE) {
        ret.push_back(new RS_Circle(nullptr, {vx1, distance}));
        return ret;
    }

    const LC_SplinePointsData spd1(false, false);
    const LC_SplinePointsData spd2(false, false);

    auto* sp1 = new LC_SplinePoints(nullptr, spd1);
    auto* sp2 = new LC_SplinePoints(nullptr, spd2);

    dDist = distance / dDist;

    sp1->addPoint(RS_Vector(vx1.x - dDist * (vx2.y - vx1.y), vx1.y + dDist * (vx2.x - vx1.x)));
    sp2->addPoint(RS_Vector(vx1.x + dDist * (vx2.y - vx1.y), vx1.y - dDist * (vx2.x - vx1.x)));

    sp1->addPoint(RS_Vector(vx2.x - dDist * (vx2.y - vx1.y), vx2.y + dDist * (vx2.x - vx1.x)));
    sp2->addPoint(RS_Vector(vx2.x + dDist * (vx2.y - vx1.y), vx2.y - dDist * (vx2.x - vx1.x)));

    ret.push_back(sp1);
    ret.push_back(sp2);
    return ret;
}

std::vector<RS_Entity*> LC_SplinePoints::offsetTwoSidesSpline(const double& distance) const {
    std::vector<RS_Entity*> ret(0, nullptr);

    size_t iPoints = m_data.splinePoints.size();
    size_t n = m_data.controlPoints.size();

    if (iPoints < 1) {
        return ret;
    }
    if (n < 1) {
        return ret;
    }

    LC_SplinePointsData spd1(m_data.closed, false);
    LC_SplinePointsData spd2(m_data.closed, false);

    LC_SplinePoints *sp1, *sp2;

    RS_Vector vStart(false), vEnd(false), vControl(false);
    RS_Vector vPoint(false), vTan(false);

    double dt, dl1, dl2;

    if (m_data.closed) {
        if (n < 3) {
            return ret;
        }

        sp1 = new LC_SplinePoints(nullptr, spd1);
        sp2 = new LC_SplinePoints(nullptr, spd2);

        vPoint = m_data.splinePoints.at(0);

        dl1 = (m_data.splinePoints.at(iPoints - 1) - vPoint).magnitude();
        dl2 = (m_data.splinePoints.at(1) - vPoint).magnitude();
        dt = dl1 / (dl1 + dl2);

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }

        for (size_t i = 1; i < n - 1; i++) {
            vPoint = m_data.splinePoints.at(i);

            dl1 = dl2;
            dl2 = (m_data.splinePoints.at(i + 1) - vPoint).magnitude();
            dt = dl1 / (dl1 + dl2);

            vStart = (m_data.controlPoints.at(i - 1) + m_data.controlPoints.at(i)) / 2.0;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vTan = getQuadDir(vStart, vControl, vEnd, dt);

            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
            }
        }

        vPoint = m_data.splinePoints.at(iPoints - 1);
        dl1 = (vPoint - m_data.splinePoints.at(iPoints - 2)).magnitude();
        dl2 = (vPoint - m_data.splinePoints.at(0)).magnitude();
        dt = dl1 / (dl1 + dl2);

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }
    }
    else {
        vStart = m_data.controlPoints.at(0);
        if (n < 2) {
            ret.push_back(new RS_Circle(nullptr, {vStart, distance}));
            return ret;
        }

        vEnd = m_data.controlPoints.at(1);
        if (n < 3) {
            return addLineOffsets(vStart, vEnd, distance);
        }

        vPoint = m_data.splinePoints.at(1);

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);

        if (n < 4) {
            dl1 = (vPoint - vStart).magnitude();
            dl2 = (vEnd - vPoint).magnitude();
            dt = dl1 / (dl1 + dl2);

            sp1 = new LC_SplinePoints(nullptr, spd1);
            sp2 = new LC_SplinePoints(nullptr, spd2);

            vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vStart.x - distance * vTan.y, vStart.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vStart.x + distance * vTan.y, vStart.y - distance * vTan.x));
            }

            vTan = getQuadDir(vStart, vControl, vEnd, dt);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
            }

            vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vEnd.x - distance * vTan.y, vEnd.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vEnd.x + distance * vTan.y, vEnd.y - distance * vTan.x));
            }

            ret.push_back(sp1);
            ret.push_back(sp2);
            return ret;
        }

        sp1 = new LC_SplinePoints(nullptr, spd1);
        sp2 = new LC_SplinePoints(nullptr, spd2);

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;

        vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vStart.x - distance * vTan.y, vStart.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vStart.x + distance * vTan.y, vStart.y - distance * vTan.x));
        }

        dl1 = (vPoint - m_data.splinePoints.at(0)).magnitude();
        dl2 = (m_data.splinePoints.at(2) - vPoint).magnitude();
        dt = dl1 / (dl1 + dl2 / 2.0);

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }

        for (size_t i = 2; i < n - 2; i++) {
            vPoint = m_data.splinePoints.at(i);

            dl1 = dl2;
            dl2 = (m_data.splinePoints.at(i + 1) - vPoint).magnitude();
            dt = dl1 / (dl1 + dl2);

            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vTan = getQuadDir(vStart, vControl, vEnd, dt);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
            }
        }

        vPoint = m_data.splinePoints.at(n - 2);

        dl1 = dl2;
        dl2 = (vPoint - m_data.splinePoints.at(n - 1)).magnitude();
        dt = dl1 / (dl1 + 2.0 * dl2);

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        vTan = getQuadDir(vStart, vControl, vEnd, dt);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }

        vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vEnd.x - distance * vTan.y, vEnd.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vEnd.x + distance * vTan.y, vEnd.y - distance * vTan.x));
        }
    }

    ret.push_back(sp1);
    ret.push_back(sp2);
    return ret;
}

std::vector<RS_Entity*> LC_SplinePoints::offsetTwoSidesCut(const double& distance) const {
    std::vector<RS_Entity*> ret(0, nullptr);

    size_t n = m_data.controlPoints.size();

    if (n < 1) {
        return ret;
    }

    LC_SplinePointsData spd1(m_data.closed, false);
    LC_SplinePointsData spd2(m_data.closed, false);

    LC_SplinePoints *sp1, *sp2;

    RS_Vector vStart(false), vEnd(false), vControl(false);
    RS_Vector vPoint(false), vTan(false);

    if (m_data.closed) {
        if (n < 3) {
            return ret;
        }

        sp1 = new LC_SplinePoints(nullptr, spd1);
        sp2 = new LC_SplinePoints(nullptr, spd2);

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }

        for (size_t i = 1; i < n - 1; i++) {
            vStart = (m_data.controlPoints.at(i - 1) + m_data.controlPoints.at(i)) / 2.0;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
            vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
            }
        }

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }
    }
    else {
        vStart = m_data.controlPoints.at(0);
        if (n < 2) {
            ret.push_back(new RS_Circle(nullptr, RS_CircleData(vStart, distance)));
            return ret;
        }

        vEnd = m_data.controlPoints.at(1);
        if (n < 3) {
            ret = addLineOffsets(vStart, vEnd, distance);
            sp1 = static_cast<LC_SplinePoints*>(ret[0]);
            sp1->update();
            sp1->m_data.cut = true;
            sp2 = static_cast<LC_SplinePoints*>(ret[1]);
            sp2->update();
            sp2->m_data.cut = true;
            return ret;
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);

        if (n < 4) {
            sp1 = new LC_SplinePoints(nullptr, spd1);
            sp2 = new LC_SplinePoints(nullptr, spd2);

            vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vStart.x - distance * vTan.y, vStart.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vStart.x + distance * vTan.y, vStart.y - distance * vTan.x));
            }

            vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
            vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
            }

            vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vEnd.x - distance * vTan.y, vEnd.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vEnd.x + distance * vTan.y, vEnd.y - distance * vTan.x));
            }

            sp1->update();
            sp1->m_data.cut = true;
            sp2->update();
            sp2->m_data.cut = true;

            ret.push_back(sp1);
            ret.push_back(sp2);
            return ret;
        }

        sp1 = new LC_SplinePoints(nullptr, spd1);
        sp2 = new LC_SplinePoints(nullptr, spd2);

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;

        vTan = getQuadDir(vStart, vControl, vEnd, 0.0);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vStart.x - distance * vTan.y, vStart.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vStart.x + distance * vTan.y, vStart.y - distance * vTan.x));
        }

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
            vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
            if (vTan.valid) {
                sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
                sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
            }
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        vPoint = getQuadAtPoint(vStart, vControl, vEnd, 0.5);
        vTan = getQuadDir(vStart, vControl, vEnd, 0.5);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vPoint.x - distance * vTan.y, vPoint.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vPoint.x + distance * vTan.y, vPoint.y - distance * vTan.x));
        }

        vTan = getQuadDir(vStart, vControl, vEnd, 1.0);
        if (vTan.valid) {
            sp1->addPoint(RS_Vector(vEnd.x - distance * vTan.y, vEnd.y + distance * vTan.x));
            sp2->addPoint(RS_Vector(vEnd.x + distance * vTan.y, vEnd.y - distance * vTan.x));
        }
    }

    sp1->update();
    sp1->m_data.cut = true;
    sp2->update();
    sp2->m_data.cut = true;

    ret.push_back(sp1);
    ret.push_back(sp2);
    return ret;
}

std::vector<RS_Entity*> LC_SplinePoints::offsetTwoSides(const double distance) const {
    if (m_data.cut) {
        return offsetTwoSidesCut(distance);
    }
    return offsetTwoSidesSpline(distance);
}

/**
 * Dumps the spline's m_data to stdout.
 */
std::ostream& operator <<(std::ostream& os, const LC_SplinePoints& l) {
    os << " SplinePoints: " << l.getData() << "\n";
    return os;
}

RS_VectorSolutions getLineLineIntersect(const RS_Vector& vStart, const RS_Vector& vEnd, const RS_Vector& vx1, const RS_Vector& vx2) {
    RS_VectorSolutions ret;

    const RS_Vector x1 = vx2 - vx1;
    const RS_Vector x2 = vStart - vEnd;
    RS_Vector x3 = vStart - vx1;

    const double dDet = x1.x * x2.y - x1.y * x2.x;
    if (std::abs(dDet) < RS_TOLERANCE) {
        return ret;
    }

    double dt = (x2.y * x3.x - x2.x * x3.y) / dDet;
    const double ds = (-x1.y * x3.x + x1.x * x3.y) / dDet;

    if (dt < -RS_TOLERANCE) {
        return ret;
    }
    if (ds < -RS_TOLERANCE) {
        return ret;
    }
    if (dt > 1.0 + RS_TOLERANCE) {
        return ret;
    }
    if (ds > 1.0 + RS_TOLERANCE) {
        return ret;
    }

    if (dt < 0.0) {
        dt = 0.0;
    }
    if (dt > 1.0) {
        dt = 1.0;
    }

    x3 = vx1 * (1.0 - dt) + vx2 * dt;

    ret.push_back(x3);

    return ret;
}

RS_VectorSolutions LC_SplinePoints::getLineIntersect(const RS_Vector& x1, const RS_Vector& x2) const {
    RS_VectorSolutions ret;

    size_t n = m_data.controlPoints.size();
    if (n < 2) {
        return ret;
    }

    RS_Vector vStart(false), vEnd(false), vControl(false);

    if (m_data.closed) {
        if (n < 3) {
            return ret;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        addLineQuadIntersect(&ret, x1, x2, vStart, vControl, vEnd);

        for (size_t i = 1; i < n - 1; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addLineQuadIntersect(&ret, x1, x2, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;

        addLineQuadIntersect(&ret, x1, x2, vStart, vControl, vEnd);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);
        if (n < 3) {
            return getLineLineIntersect(x1, x2, vStart, vEnd);
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            addLineQuadIntersect(&ret, x1, x2, vStart, vControl, vEnd);
            return ret;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;
        addLineQuadIntersect(&ret, x1, x2, vStart, vControl, vEnd);

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addLineQuadIntersect(&ret, x1, x2, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        addLineQuadIntersect(&ret, x1, x2, vStart, vControl, vEnd);
    }

    return ret;
}

void addQuadQuadIntersect(RS_VectorSolutions* pVS, const RS_Vector& vStart, const RS_Vector& vControl, const RS_Vector& vEnd,
                          const RS_Vector& vx1, const RS_Vector& vc1, const RS_Vector& vx2) {
    //avoid intersection if there's no intersection between lines
    //TODO, avoid O(N^2) complexity
    //tangential direction along (start, control, end)
    const std::array<RS_Line, 2> lines0{{{vStart, vControl}, {vEnd, vControl}}};

    //tangential direction along (start, control, end)
    const std::array<RS_Line, 2> lines1{{{vx1, vc1}, {vx2, vc1}}};

    //if lines0, lines1 do not overlap, there's no intersection
    bool overlap = false;
    for (const auto& l0 : lines0) {
        for (const auto& l1 : lines1) {
            if (RS_Information::getIntersection(&l0, &l1, true).size()) {
                overlap = true;
                break;
            }
        }
        if (overlap) {
            break;
        }
    }
    if (!overlap) {
        //if there's no overlap, return now
        return;
    }

    const RS_Vector va0 = vStart;
    const RS_Vector va1 = (vControl - vStart) * 2.0;
    const RS_Vector va2 = vEnd - vControl * 2.0 + vStart;

    const RS_Vector vb0 = vx1;
    const RS_Vector vb1 = (vc1 - vx1) * 2.0;
    const RS_Vector vb2 = vx2 - vc1 * 2.0 + vx1;

    std::vector<double> a1(0, 0.), b1(0, 0.);
    a1.push_back(va2.x);
    b1.push_back(va2.y);
    a1.push_back(0.0);
    b1.push_back(0.0);
    a1.push_back(-vb2.x);
    b1.push_back(-vb2.y);
    a1.push_back(va1.x);
    b1.push_back(va1.y);
    a1.push_back(-vb1.x);
    b1.push_back(-vb1.y);
    a1.push_back(va0.x - vb0.x);
    b1.push_back(va0.y - vb0.y);

    std::vector<std::vector<double>> m(0);
    m.push_back(a1);
    m.push_back(b1);

    const RS_VectorSolutions& pvRes = RS_Math::simultaneousQuadraticSolverFull(m);

    for (RS_Vector vSol : pvRes) {
        if (vSol.x > -RS_TOLERANCE && vSol.x < 1.0 + RS_TOLERANCE && vSol.y > -RS_TOLERANCE && vSol.y < 1.0 + RS_TOLERANCE) {
            if (vSol.x < 0.0) {
                vSol.x = 0.0;
            }
            if (vSol.x > 1.0) {
                vSol.x = 1.0;
            }
            pVS->push_back(getQuadPoint(vStart, vControl, vEnd, vSol.x));
        }
    }
}

void LC_SplinePoints::addQuadIntersect(RS_VectorSolutions* sol, const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2) const {
    size_t n = m_data.controlPoints.size();
    if (n < 2) {
        return;
    }

    RS_Vector vStart(false), vEnd(false), vControl(false);

    if (m_data.closed) {
        if (n < 3) {
            return;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        addQuadQuadIntersect(sol, vStart, vControl, vEnd, x1, c1, x2);

        for (size_t i = 1; i < n - 1; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addQuadQuadIntersect(sol, vStart, vControl, vEnd, x1, c1, x2);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;

        addQuadQuadIntersect(sol, vStart, vControl, vEnd, x1, c1, x2);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);
        if (n < 3) {
            addLineQuadIntersect(sol, vStart, vEnd, x1, c1, x2);
            return;
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            addQuadQuadIntersect(sol, vStart, vControl, vEnd, x1, c1, x2);
            return;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;
        addQuadQuadIntersect(sol, vStart, vControl, vEnd, x1, c1, x2);

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addQuadQuadIntersect(sol, vStart, vControl, vEnd, x1, c1, x2);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        addQuadQuadIntersect(sol, vStart, vControl, vEnd, x1, c1, x2);
    }
}

RS_VectorSolutions LC_SplinePoints::getSplinePointsIntersect(LC_SplinePoints* l1) const {
    RS_VectorSolutions ret;

    size_t n = m_data.controlPoints.size();
    if (n < 2) {
        return ret;
    }

    RS_Vector vStart(false), vEnd(false), vControl(false);

    if (m_data.closed) {
        if (n < 3) {
            return ret;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        l1->addQuadIntersect(&ret, vStart, vControl, vEnd);

        for (size_t i = 1; i < n - 1; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            l1->addQuadIntersect(&ret, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;

        l1->addQuadIntersect(&ret, vStart, vControl, vEnd);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);
        if (n < 3) {
            return l1->getLineIntersect(vStart, vEnd);
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            l1->addQuadIntersect(&ret, vStart, vControl, vEnd);
            return ret;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;
        l1->addQuadIntersect(&ret, vStart, vControl, vEnd);

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            l1->addQuadIntersect(&ret, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        l1->addQuadIntersect(&ret, vStart, vControl, vEnd);
    }

    return ret;
}

RS_VectorSolutions getQuadraticLineIntersect(const std::vector<double>& dQuadCoefs, const RS_Vector& vx1, const RS_Vector& vx2) {
    RS_VectorSolutions ret;
    if (dQuadCoefs.size() < 3) {
        return ret;
    }

    const RS_Vector x1 = vx2 - vx1;

    double a0 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;

    if (dQuadCoefs.size() > 3) {
        a2 = dQuadCoefs[0] * x1.x * x1.x + dQuadCoefs[1] * x1.x * x1.y + dQuadCoefs[2] * x1.y * x1.y;
        a1 = 2.0 * (dQuadCoefs[0] * x1.x * vx1.x + dQuadCoefs[2] * x1.y * vx1.y) + dQuadCoefs[1] * (x1.x * vx1.y + x1.y * vx1.x) +
            dQuadCoefs[3] * x1.x + dQuadCoefs[4] * x1.y;
        a0 = dQuadCoefs[0] * vx1.x * vx1.x + dQuadCoefs[1] * vx1.x * vx1.y + dQuadCoefs[2] * vx1.y * vx1.y + dQuadCoefs[3] * vx1.x +
            dQuadCoefs[4] * vx1.y + dQuadCoefs[5];
    }
    else {
        a1 = dQuadCoefs[0] * x1.x + dQuadCoefs[1] * x1.y;
        a0 = dQuadCoefs[0] * vx1.x + dQuadCoefs[1] * vx1.y + dQuadCoefs[2];
    }

    std::vector<double> dSol(0, 0.);

    if (std::abs(a2) > RS_TOLERANCE) {
        std::vector<double> dCoefs(2, 0.);
        dCoefs.push_back(a1 / a2);
        dCoefs.push_back(a0 / a2);
        dSol = RS_Math::quadraticSolver(dCoefs);
    }
    else if (std::abs(a1) > RS_TOLERANCE) {
        dSol.push_back(-a0 / a1);
    }

    for (double& d : dSol) {
        if (d > -RS_TOLERANCE && d < 1.0 + RS_TOLERANCE) {
            d = qBound(0.0, d, 1.0);
            ret.push_back(vx1 * (1.0 - d) + vx2 * d);
        }
    }

    return ret;
}

void addQuadraticQuadIntersect(RS_VectorSolutions* pVS, const std::vector<double>& dQuadCoefs, const RS_Vector& vx1, const RS_Vector& vc1,
                               const RS_Vector& vx2) {
    if (dQuadCoefs.size() < 3) {
        return;
    }

    const RS_Vector x1 = vx2 - vc1 * 2.0 + vx1;
    const RS_Vector x2 = vc1 - vx1;

    double a0 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
    double a3 = 0.0;
    double a4 = 0.0;

    if (dQuadCoefs.size() > 3) {
        a4 = dQuadCoefs[0] * x1.x * x1.x + dQuadCoefs[1] * x1.x * x1.y + dQuadCoefs[2] * x1.y * x1.y;
        a3 = 4.0 * dQuadCoefs[0] * x1.x * x2.x + 2.0 * dQuadCoefs[1] * (x1.x * x2.y + x1.y * x2.x) + 4.0 * dQuadCoefs[2] * x1.y * x2.y;
        a2 = dQuadCoefs[0] * (2.0 * x1.x * vx1.x + 4.0 * x2.x * x2.x) + dQuadCoefs[1] * (x1.x * vx1.y + x1.y * vx1.x + 4.0 * x2.x * x2.y) +
            dQuadCoefs[2] * (2.0 * x1.y * vx1.y + 4.0 * x2.y * x2.y) + dQuadCoefs[3] * x1.x + dQuadCoefs[4] * x1.y;
        a1 = 4.0 * (dQuadCoefs[0] * x2.x * vx1.x + dQuadCoefs[2] * x2.y * vx1.y) + 2.0 * (dQuadCoefs[1] * (x2.x * vx1.y + x2.y * vx1.x) +
            dQuadCoefs[3] * x2.x + dQuadCoefs[4] * x2.y);
        a0 = dQuadCoefs[0] * vx1.x * vx1.x + dQuadCoefs[1] * vx1.x * vx1.y + dQuadCoefs[2] * vx1.y * vx1.y + dQuadCoefs[3] * vx1.x +
            dQuadCoefs[4] * vx1.y + dQuadCoefs[5];
    }
    else {
        a2 = dQuadCoefs[0] * x1.x + dQuadCoefs[1] * x1.y;
        a1 = 2.0 * (dQuadCoefs[0] * x2.x + dQuadCoefs[1] * x2.y);
        a0 = dQuadCoefs[0] * vx1.x + dQuadCoefs[1] * vx1.y + dQuadCoefs[2];
    }

    std::vector<double> dSol(0, 0.);
    std::vector<double> dCoefs(0, 0.);

    if (std::abs(a4) > RS_TOLERANCE) {
        dCoefs.push_back(a3 / a4);
        dCoefs.push_back(a2 / a4);
        dCoefs.push_back(a1 / a4);
        dCoefs.push_back(a0 / a4);
        dSol = RS_Math::quarticSolver(dCoefs);
    }
    else if (std::abs(a3) > RS_TOLERANCE) {
        dCoefs.push_back(a2 / a3);
        dCoefs.push_back(a1 / a3);
        dCoefs.push_back(a0 / a3);
        dSol = RS_Math::cubicSolver(dCoefs);
    }
    else if (std::abs(a2) > RS_TOLERANCE) {
        dCoefs.push_back(a1 / a2);
        dCoefs.push_back(a0 / a2);
        dSol = RS_Math::quadraticSolver(dCoefs);
    }
    else if (std::abs(a1) > RS_TOLERANCE) {
        dSol.push_back(-a0 / a1);
    }

    for (double& d : dSol) {
        if (d > -RS_TOLERANCE && d < 1.0 + RS_TOLERANCE) {
            if (d < 0.0) {
                d = 0.0;
            }
            if (d > 1.0) {
                d = 1.0;
            }
            pVS->push_back(getQuadAtPoint(vx1, vc1, vx2, d));
        }
    }
}

RS_VectorSolutions LC_SplinePoints::getQuadraticIntersect(const RS_Entity* e1) const {
    RS_VectorSolutions ret;

    size_t n = m_data.controlPoints.size();
    if (n < 2) {
        return ret;
    }

    LC_Quadratic lcQuad = e1->getQuadratic();
    std::vector<double> dQuadCoefs = lcQuad.getCoefficients();

    RS_Vector vStart(false), vEnd(false), vControl(false);

    if (m_data.closed) {
        if (n < 3) {
            return ret;
        }

        vStart = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;
        vControl = m_data.controlPoints.at(0);
        vEnd = (m_data.controlPoints.at(0) + m_data.controlPoints.at(1)) / 2.0;

        addQuadraticQuadIntersect(&ret, dQuadCoefs, vStart, vControl, vEnd);

        for (size_t i = 1; i < n - 1; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addQuadraticQuadIntersect(&ret, dQuadCoefs, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 1);
        vEnd = (m_data.controlPoints.at(n - 1) + m_data.controlPoints.at(0)) / 2.0;

        addQuadraticQuadIntersect(&ret, dQuadCoefs, vStart, vControl, vEnd);
    }
    else {
        vStart = m_data.controlPoints.at(0);
        vEnd = m_data.controlPoints.at(1);
        if (n < 3) {
            return getQuadraticLineIntersect(dQuadCoefs, vStart, vEnd);
        }

        vControl = vEnd;
        vEnd = m_data.controlPoints.at(2);
        if (n < 4) {
            addQuadraticQuadIntersect(&ret, dQuadCoefs, vStart, vControl, vEnd);
            return ret;
        }

        vEnd = (m_data.controlPoints.at(1) + m_data.controlPoints.at(2)) / 2.0;
        addQuadraticQuadIntersect(&ret, dQuadCoefs, vStart, vControl, vEnd);

        for (size_t i = 2; i < n - 2; i++) {
            vStart = vEnd;
            vControl = m_data.controlPoints.at(i);
            vEnd = (m_data.controlPoints.at(i) + m_data.controlPoints.at(i + 1)) / 2.0;

            addQuadraticQuadIntersect(&ret, dQuadCoefs, vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints.at(n - 2);
        vEnd = m_data.controlPoints.at(n - 1);

        addQuadraticQuadIntersect(&ret, dQuadCoefs, vStart, vControl, vEnd);
    }

    return ret;
}

RS_VectorSolutions LC_SplinePoints::getIntersection(const RS_Entity* e1, const RS_Entity* e2) {
    if (e2 == nullptr || (e1 != nullptr && e1->rtti() != RS2::EntitySplinePoints)) {
        std::swap(e1, e2);
    }
    if (e1 == nullptr || e1->rtti() != RS2::EntitySplinePoints) {
        return {};
    }

    const auto spline = static_cast<LC_SplinePoints*>(const_cast<RS_Entity*>(e1));

    switch (e2->rtti()) {
        case RS2::EntityLine:
            return {spline->getLineIntersect(e2->getStartpoint(), e2->getEndpoint())};
        case RS2::EntitySplinePoints:
            return {spline->getSplinePointsIntersect(static_cast<LC_SplinePoints*>(const_cast<RS_Entity*>(e2)))};
        default:
            return {spline->getQuadraticIntersect(e2)};
    }
}

RS2::EntityType LC_SplinePoints::rtti() const {
    return RS2::EntitySplinePoints;
}

/** @return false */
bool LC_SplinePoints::isEdge() const {
    return true;
}

/** @return Copy of m_data that defines the spline. */
const LC_SplinePointsData& LC_SplinePoints::getData() const {
    return m_data;
}

/** @return Copy of m_data that defines the spline. */
LC_SplinePointsData& LC_SplinePoints::getData() {
    return m_data;
}

/** @return Number of control points. */
size_t LC_SplinePoints::getNumberOfControlPoints() const {
    return m_data.controlPoints.size();
}

/**
* @retval true if the spline is closed.
* @retval false otherwise.
*/
bool LC_SplinePoints::isClosed() const {
    return m_data.closed;
}

/**
* Sets the closed flag of this spline.
*/
void LC_SplinePoints::setClosed(const bool c) {
    m_data.closed = c;
    update();
}

/*void LC_SplinePoints::trimStartpoint(const RS_Vector& pos)
{
}

void LC_SplinePoints::trimEndpoint(const RS_Vector& pos)
{
}*/

LC_SplinePoints* LC_SplinePoints::cut(const RS_Vector& pos) {
    LC_SplinePoints* ret = nullptr;

    double dt;
    const int iQuad = getNearestQuad(pos, nullptr, &dt);
    if (iQuad < 1) {
        return ret;
    }

    RS_Vector vStart(false);
    RS_Vector vControl(false);
    RS_Vector vEnd(false);

    const int iPts = getQuadPoints(iQuad, &vStart, &vControl, &vEnd);
    if (iPts < 2) {
        return ret;
    }

    RS_Vector vPoint(false);
    if (iPts < 3) {
        vPoint = vStart * (1.0 - dt) + vEnd * dt;
    }
    else {
        vPoint = getQuadPoint(vStart, vControl, vEnd, dt);
    }

    const size_t n = m_data.controlPoints.size();

    RS_Vector vNewControl(false);
    if (m_data.closed) {
        // if the spline is closed, we must delete splinePoints, add the pos
        // as start and end point and reorder control points. We must return
        // nullptr since there will still be only one spline
        for (int i = 0; i < iQuad - 1; i++) {
            vNewControl = m_data.controlPoints.front();
            m_data.controlPoints.erase(m_data.controlPoints.begin());
            m_data.controlPoints.push_back(vNewControl);
        }

        if (iPts > 2) {
            vNewControl = getSubQuadControlPoint(vStart, vControl, vEnd, 0.0, dt);
            m_data.controlPoints.push_back(vNewControl);

            vNewControl = getSubQuadControlPoint(vStart, vControl, vEnd, dt, 1.0);
            m_data.controlPoints.front() = vNewControl;
        }
        m_data.controlPoints.push_back(vPoint);
        m_data.controlPoints.insert(m_data.controlPoints.begin(), vPoint);

        m_data.closed = false;
        m_data.cut = true;
    }
    else {
        LC_SplinePointsData newData(false, true);
        for (size_t i = iQuad + 1; i < n; i++) {
            newData.controlPoints.push_back(m_data.controlPoints.at(iQuad + 1));
            m_data.controlPoints.erase(m_data.controlPoints.begin() + iQuad + 1);
        }

        if (iPts > 2) {
            vNewControl = getSubQuadControlPoint(vStart, vControl, vEnd, 0.0, dt);
            m_data.controlPoints[iQuad] = vNewControl;

            vNewControl = getSubQuadControlPoint(vStart, vControl, vEnd, dt, 1.0);
            newData.controlPoints.insert(newData.controlPoints.begin(), vNewControl);
        }
        m_data.controlPoints.push_back(vPoint);
        newData.controlPoints.insert(newData.controlPoints.begin(), vPoint);

        ret = new LC_SplinePoints(m_parent, newData);

        m_data.cut = true;
    }

    return ret;
}

QPolygonF LC_SplinePoints::getBoundingRect(const RS_Vector& x1, const RS_Vector& c1, const RS_Vector& x2) {
    QPolygonF ret;
    ret << QPointF(x1.x, x1.y);
    //find t for tangent in parallel with x2 - x1
    const RS_Vector pt = (x1 - c1 * 2. + x2) * 2.;
    const RS_Vector pl = (x1 - x2);
    const double determinant = pt.x * pl.y - pt.y * pl.x;
    if (std::abs(determinant) < RS_TOLERANCE15) {
        //bezier is a straight line
        ret << QPointF(x2.x, x2.y) << ret.front();
        return ret;
    }
    const RS_Vector pc = (x1 - c1) * 2.;
    const double t = (pc.x * pl.y - pc.y * pl.x) / determinant;
    const double tr = 1. - t;
    //offset from x1 to the extreme point
    const RS_Vector pext = x1 * (tr * tr - 1) + c1 * (2. * t * tr) + x2 * (t * t);
    //perpendicular offset from x1 to the extreme point, the component of the offset perpendicular to x1-x2
    const RS_Vector dp = pext - pl * (pext.dotP(pl) / pl.squared());
    RS_Vector v1 = x1 + dp;
    ret << QPointF(v1.x, v1.y);
    v1 = x2 + dp;
    ret << QPointF(v1.x, v1.y);
    ret << ret.front();
    return ret;
}

/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area = \oint x dy
 * @return line integral \oint x dy along the spline entity
 * @author Dongxu Li
 */
double LC_SplinePoints::areaLineIntegral() const {
    size_t n = m_data.controlPoints.size();
    if (n < 2) {
        return 0.0;
    }

    auto quadAreaIntegral = [](const RS_Vector& p0, const RS_Vector& p1, const RS_Vector& p2) -> double {
        const double x0 = p0.x;
        const double y0 = p0.y;
        const double x1 = p1.x;
        const double y1 = p1.y;
        const double x2 = p2.x;
        const double y2 = p2.y;

        const double A = 2.0 * (y1 - y0);
        const double B = 2.0 * (y2 - 2.0 * y1 + y0);
        const double C = x0;
        const double D = 2.0 * (x1 - x0);
        const double E = x0 - 2.0 * x1 + x2;

        const double int1 = C * A;
        const double int2 = (C * B + D * A) / 2.0;
        const double int3 = (D * B + E * A) / 3.0;
        const double int4 = E * B / 4.0;

        return int1 + int2 + int3 + int4;
    };

    double res = 0.0;
    RS_Vector vStart, vControl, vEnd;

    if (!m_data.closed) {
        if (n == 2) {
            RS_Vector s = getStartpoint();
            RS_Vector e = getEndpoint();
            return (s.x + e.x) / 2.0 * (e.y - s.y);
        }

        // n >= 3
        vStart = m_data.controlPoints[0];
        vControl = m_data.controlPoints[1];
        if (n == 3) {
            vEnd = m_data.controlPoints[2];
            return quadAreaIntegral(vStart, vControl, vEnd);
        }

        vEnd = (m_data.controlPoints[1] + m_data.controlPoints[2]) / 2.0;
        res += quadAreaIntegral(vStart, vControl, vEnd);

        for (size_t i = 2; i < n - 2; ++i) {
            vStart = vEnd;
            vControl = m_data.controlPoints[i];
            vEnd = (m_data.controlPoints[i] + m_data.controlPoints[i + 1]) / 2.0;
            res += quadAreaIntegral(vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints[n - 2];
        vEnd = m_data.controlPoints[n - 1];
        res += quadAreaIntegral(vStart, vControl, vEnd);
    }
    else {
        // closed, n >= 3
        if (n < 3) {
            return 0.0;
        }

        vStart = (m_data.controlPoints[n - 1] + m_data.controlPoints[0]) / 2.0;
        vControl = m_data.controlPoints[0];
        vEnd = (m_data.controlPoints[0] + m_data.controlPoints[1]) / 2.0;
        res += quadAreaIntegral(vStart, vControl, vEnd);

        for (size_t i = 1; i < n - 1; ++i) {
            vStart = vEnd;
            vControl = m_data.controlPoints[i];
            vEnd = (m_data.controlPoints[i] + m_data.controlPoints[i + 1]) / 2.0;
            res += quadAreaIntegral(vStart, vControl, vEnd);
        }

        vStart = vEnd;
        vControl = m_data.controlPoints[n - 1];
        vEnd = (m_data.controlPoints[n - 1] + m_data.controlPoints[0]) / 2.0;
        res += quadAreaIntegral(vStart, vControl, vEnd);
    }

    return res;
}

LC_SecondMoment LC_SplinePoints::secondMomentLineIntegral() const {
    // 5-point Gauss-Legendre quadrature on [0,1] — exact for polynomial degree ≤ 9.
    // Each quadratic Bézier segment has degree-7 integrands (x³y'/3 etc.),
    // so this gives the exact result per segment.
    static constexpr double t5[] = {
        0.04691007703067, 0.23076534494716, 0.5,
        0.76923465505284, 0.95308992296933
    };
    static constexpr double w5[] = {
        0.11846344252810, 0.23931433524969, 0.28444444444444,
        0.23931433524969, 0.11846344252810
    };

    // Helper: integrate second moments over one quadratic Bézier segment P0,P1,P2
    auto segMoment = [&](const RS_Vector& p0, const RS_Vector& p1, const RS_Vector& p2) -> LC_SecondMoment {
        LC_SecondMoment m;
        for (int i = 0; i < 5; ++i) {
            const double t   = t5[i];
            const double mt  = 1.0 - t;
            const double x   = mt*mt*p0.x + 2.0*mt*t*p1.x + t*t*p2.x;
            const double y   = mt*mt*p0.y + 2.0*mt*t*p1.y + t*t*p2.y;
            const double dxdt = 2.0*(mt*(p1.x-p0.x) + t*(p2.x-p1.x));
            const double dydt = 2.0*(mt*(p1.y-p0.y) + t*(p2.y-p1.y));
            m.ixx += w5[i] * x*x*x / 3.0 * dydt;
            m.iyy += w5[i] * (-y*y*y / 3.0) * dxdt;
            m.ixy += w5[i] * x*x * y / 2.0 * dydt;
        }
        return m;
    };

    // Replicate the segment decomposition from areaLineIntegral()
    size_t n = m_data.controlPoints.size();
    if (n < 2) {
        return {};
    }

    LC_SecondMoment res;
    RS_Vector vStart, vControl, vEnd;

    if (!m_data.closed) {
        if (n == 2) {
            // Degenerate: straight line
            RS_Vector s = getStartpoint();
            RS_Vector e = getEndpoint();
            RS_Line line(nullptr, RS_LineData{s, e});
            return line.secondMomentLineIntegral();
        }
        vStart   = m_data.controlPoints[0];
        vControl = m_data.controlPoints[1];
        if (n == 3) {
            vEnd = m_data.controlPoints[2];
            return segMoment(vStart, vControl, vEnd);
        }
        vEnd = (m_data.controlPoints[1] + m_data.controlPoints[2]) / 2.0;
        res += segMoment(vStart, vControl, vEnd);

        for (size_t i = 2; i < n - 2; ++i) {
            vStart   = vEnd;
            vControl = m_data.controlPoints[i];
            vEnd     = (m_data.controlPoints[i] + m_data.controlPoints[i+1]) / 2.0;
            res += segMoment(vStart, vControl, vEnd);
        }
        vStart   = vEnd;
        vControl = m_data.controlPoints[n-2];
        vEnd     = m_data.controlPoints[n-1];
        res += segMoment(vStart, vControl, vEnd);
    } else {
        if (n < 3) {
            return {};
        }
        vStart   = (m_data.controlPoints[n-1] + m_data.controlPoints[0]) / 2.0;
        vControl = m_data.controlPoints[0];
        vEnd     = (m_data.controlPoints[0]   + m_data.controlPoints[1]) / 2.0;
        res += segMoment(vStart, vControl, vEnd);

        for (size_t i = 1; i < n - 1; ++i) {
            vStart   = vEnd;
            vControl = m_data.controlPoints[i];
            vEnd     = (m_data.controlPoints[i] + m_data.controlPoints[i+1]) / 2.0;
            res += segMoment(vStart, vControl, vEnd);
        }
        vStart   = vEnd;
        vControl = m_data.controlPoints[n-1];
        vEnd     = (m_data.controlPoints[n-1] + m_data.controlPoints[0]) / 2.0;
        res += segMoment(vStart, vControl, vEnd);
    }
    return res;
}
