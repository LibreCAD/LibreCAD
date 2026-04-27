/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015-2024 LibreCAD.org
** Copyright (C) 2015-2024 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2026 Grok team (modern C++ cleanup, tolerance unification,
**                              const correctness, RAII helpers)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>

#include "lc_quadratic.h"
#include "lc_quadraticutils.h"
#include "lc_hyperbola.h"
#include "lc_parabola.h"

#include "rs_atomicentity.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pen.h"
#include "rs_point.h"
#include "rs_polyline.h"

#ifdef EMU_C99
#include "emu_c99.h" /* C99 math */
#endif

namespace LC_QuadraticUtils {

/* ────────────────────────────────────────────────────────────────────────────
   Basic properties / classification
   ──────────────────────────────────────────────────────────────────────────── */

bool isValid(const LC_Quadratic& q)
{
    return !(RS_Math::equal(q.getA(), 0.0) &&
             RS_Math::equal(q.getB(), 0.0) &&
             RS_Math::equal(q.getC(), 0.0) &&
             RS_Math::equal(q.getD(), 0.0) &&
             RS_Math::equal(q.getE(), 0.0) &&
             RS_Math::equal(q.getF(), 0.0));
}

bool isQuadratic(const LC_Quadratic& q)
{
    return std::hypot(q.getA(), q.getB(), q.getC()) > RS_TOLERANCE * 100.0;
}

bool isDegenerate(const LC_Quadratic& q)
{
    double det3 = computeHomogeneousDeterminant(q);
    double s = computeScale(q);
    return std::abs(det3) < RS_TOLERANCE * s * s;
}

bool isParabolaCondition(double discriminant, const LC_Quadratic& q)
{
    double mag = std::max({std::abs(q.getA()),
                           std::abs(q.getB()),
                           std::abs(q.getC()),
                           1e-10});

    double relTol = RS_TOLERANCE * mag * mag;
    bool hasQuadraticTerm = mag > RS_TOLERANCE * 10.0;

    return hasQuadraticTerm && std::abs(discriminant) <= relTol;
}

/* ────────────────────────────────────────────────────────────────────────────
   Geometric properties
   ──────────────────────────────────────────────────────────────────────────── */

RS_Vector computeCenter(const LC_Quadratic& q)
{
    double detQ = 4.0 * q.getA() * q.getC() - q.getB() * q.getB();
    double qn = computeQuadNorm(q);
    if (std::abs(detQ) <= RS_TOLERANCE * qn * qn) {
        return RS_Vector(false);
    }
    return RS_Vector(
        (q.getB() * q.getE() - 2.0 * q.getC() * q.getD()) / detQ,
        (q.getB() * q.getD() - 2.0 * q.getA() * q.getE()) / detQ
    );
}

double evaluateAt(const LC_Quadratic& q, const RS_Vector& p)
{
    if (!p.valid) return 0.0;
    double x = p.x, y = p.y;
    return q.getA() * x * x +
           q.getB() * x * y +
           q.getC() * y * y +
           q.getD() * x +
           q.getE() * y +
           q.getF();
}

double computeDiscriminant(const LC_Quadratic& q)
{
    return q.getB() * q.getB() - 4.0 * q.getA() * q.getC();
}

double computeQuadNorm(const LC_Quadratic& q)
{
    return std::hypot(q.getA(), q.getB(), q.getC());
}

double computeLinearNorm(const LC_Quadratic& q)
{
    return std::hypot(q.getD(), q.getE());
}

double computeScale(const LC_Quadratic& q)
{
    return std::max({computeQuadNorm(q),
                     computeLinearNorm(q),
                     std::abs(q.getF()),
                     1e-6});
}

double computeHomogeneousDeterminant(const LC_Quadratic& q)
{
    double A = q.getA(), h = q.getB() / 2.0, C = q.getC();
    double D = q.getD(), E = q.getE(), F = q.getF();

    return A * (C * F - E * E / 4.0) - h * (h * F - D * E / 4.0)
           + (D / 2.0) * (h * E / 2.0 - C * D / 2.0);
}

double computeRotationAngle(const LC_Quadratic& q)
{
    double B = q.getB();
    double diff = q.getA() - q.getC();
    if (std::abs(B) > RS_TOLERANCE) {
        return 0.5 * std::atan2(B, diff);
    }
    return (q.getA() < q.getC() - RS_TOLERANCE) ? M_PI / 2.0 : 0.0;
}

std::pair<double, double> computePrincipalCoefficients(const LC_Quadratic& q, double theta)
{
    double ct = std::cos(theta), st = std::sin(theta);
    double Ap = q.getA() * ct * ct + q.getB() * ct * st + q.getC() * st * st;
    double Cp = q.getA() * st * st - q.getB() * ct * st + q.getC() * ct * ct;
    return {Ap, Cp};
}

/* ────────────────────────────────────────────────────────────────────────────
   Parabola / Hyperbola / Ellipse utilities
   ──────────────────────────────────────────────────────────────────────────── */

std::array<RS_Vector, 3> computeParabolaControlPoints(
    const RS_Vector& center, double p, const RS_Vector& axis, const RS_Vector& perp)
{
    return {
        center + p * 4.0 * axis - 4.0 * p * perp,
        center,
        center + p * 4.0 * axis + 4.0 * p * perp
    };
}

std::pair<RS_Vector, double> computeHyperbolaMajorPAndRatio(
    double Ap, double Cp, double v, double theta)
{
    double a2 = -v / Ap;
    double b2 = -v / Cp;
    if (a2 <= 0.0 || b2 <= 0.0 || std::min(a2, b2) < 1e-8) {
        return {{}, 0.0};
    }

    bool xTrans = Ap * v < 0.0;
    double semiTrans = std::sqrt(xTrans ? a2 : b2);
    double semiConj  = std::sqrt(xTrans ? b2 : a2);

    RS_Vector majorP = xTrans
        ? RS_Vector(std::cos(theta) * semiTrans, std::sin(theta) * semiTrans)
        : RS_Vector(-std::sin(theta) * semiTrans, std::cos(theta) * semiTrans);

    return {majorP, semiConj / semiTrans};
}

std::tuple<double, double, RS_Vector> computeEllipseSemiAxesAndMajorP(
    double Ap, double Cp, double v, double theta)
{
    double α = -v / Ap;
    double β = -v / Cp;

    if (α <= 0.0 || β <= 0.0 || α < 1e-8 || β < 1e-8) {
        return {0.0, 0.0, RS_Vector()};
    }

    double sm = std::sqrt(std::max(α, β));
    double sn = std::sqrt(std::min(α, β));
    double r = sn / sm;

    RS_Vector mp = (α >= β)
        ? RS_Vector(std::cos(theta) * sm, std::sin(theta) * sm)
        : RS_Vector(-std::sin(theta) * sm, std::cos(theta) * sm);

    return {sm, r, mp};
}

/* ────────────────────────────────────────────────────────────────────────────
   Entity creation helpers
   ──────────────────────────────────────────────────────────────────────────── */

RS_Entity* createLineFromLinearCoefficients(const LC_Quadratic& q)
{
    double D = q.getD();
    double E = q.getE();
    double F = q.getF();

    double linNorm = std::hypot(D, E);
    double scale   = std::max({linNorm, std::abs(F), 1e-6});
    const double eps = RS_TOLERANCE * 10.0;

    if (linNorm < eps * scale) {
        return nullptr;
    }

    RS_Vector base;
    if (std::abs(E) > eps) {
        base = RS_Vector(0.0, -F / E);
    } else if (std::abs(D) > eps) {
        base = RS_Vector(-F / D, 0.0);
    } else {
        return nullptr;
    }

    RS_Vector dir(-E, D);
    if (dir.squared() < 1e-12) {
        dir = RS_Vector(1.0, 0.0);
    }
    dir.normalize();

    const double halfLength = 400.0;
    return new RS_Line(nullptr, RS_LineData{
        base - dir * halfLength,
        base + dir * halfLength
    });
}

RS_Entity* createDegeneratePointOrIntersecting(
    const LC_Quadratic& q,
    const RS_Vector& center,
    double valueAtCenter)
{
    if (!center.valid) return nullptr;

    double scale = computeScale(q);
    if (std::abs(valueAtCenter) > RS_TOLERANCE * scale) {
        return nullptr;
    }

    double yy = center.y;
    double aa = q.getA();
    double bb = q.getB() * yy + q.getD();
    double cc = q.getC() * yy * yy + q.getE() * yy + q.getF();

    double dd = bb * bb - 4.0 * aa * cc;

    if (std::abs(dd) >= RS_TOLERANCE * scale * scale || std::abs(aa) < RS_TOLERANCE) {
        return new RS_Point(nullptr, RS_PointData(center));
    }

    double sd = std::sqrt(std::max(0.0, dd));
    double x1 = (-bb + sd) / (2.0 * aa);
    double x2 = (-bb - sd) / (2.0 * aa);

    RS_Vector dir1(x1 - center.x, yy - center.y);
    RS_Vector dir2(x2 - center.x, yy - center.y);

    double cosAngle = std::abs(dir1.dotP(dir2)) /
                      (dir1.magnitude() * dir2.magnitude() + 1e-10);

    if (cosAngle > 0.999 || dir1.squared() < 1e-8 || dir2.squared() < 1e-8) {
        RS_Vector commonDir = dir1.magnitude() > dir2.magnitude() ? dir1 : dir2;
        return new RS_Line(nullptr, {center - commonDir * 500.0, center + commonDir * 500.0});
    }

    auto poly = new RS_Polyline(nullptr, RS_PolylineData());
    RS_Vector u1 = dir1.normalized();
    RS_Vector u2 = dir2.normalized();

    poly->addVertex(center - u1 * 500.0);
    poly->addVertex(center + u1 * 500.0);
    poly->addVertex(center - u2 * 500.0);
    poly->addVertex(center + u2 * 500.0);

    return poly;
}

// ---------------------------------------------------------------------------
// 1. Ellipse / Circle
// ---------------------------------------------------------------------------
RS_Entity* createEllipseOrCircle(const LC_Quadratic& q,
                                 const RS_Vector& center,
                                 double valueAtCenter,
                                 double Ap,
                                 double Cp,
                                 double theta)
{
    double alpha = -valueAtCenter / Ap;
    double beta  = -valueAtCenter / Cp;

    if (alpha <= 0.0 || beta <= 0.0 || alpha < 1e-8 || beta < 1e-8) {
        return new RS_Point(nullptr, RS_PointData(center));
    }

    double semiMajor = std::sqrt(std::max(alpha, beta));
    double semiMinor = std::sqrt(std::min(alpha, beta));
    double ratio     = semiMinor / semiMajor;

    RS_Vector majorP = (alpha >= beta)
        ? RS_Vector(std::cos(theta) * semiMajor, std::sin(theta) * semiMajor)
        : RS_Vector(-std::sin(theta) * semiMajor, std::cos(theta) * semiMajor);

    if (ratio > 0.999 && std::abs(theta) < 1e-5) {
        return new RS_Circle(nullptr, RS_CircleData(center, semiMajor));
    }

    RS_EllipseData data;
    data.center     = center;
    data.majorP     = majorP;
    data.ratio      = ratio;
    data.angle1     = 0.0;
    data.angle2     = 0.0;
    data.reversed   = false;

    return new RS_Ellipse(nullptr, data);
}

// ---------------------------------------------------------------------------
// 2. Parabola
// ---------------------------------------------------------------------------
RS_Entity* createParabola(const LC_Quadratic& q,
                          const RS_Vector& center,
                          double valueAtCenter,
                          double Ap,
                          double Cp)
{
    RS_Vector axis;
    if (std::abs(q.getB()) < RS_TOLERANCE) {
        axis = std::abs(q.getA()) <= std::abs(q.getC())
            ? RS_Vector(1.0, 0.0)
            : RS_Vector(0.0, 1.0);
    } else {
        double angle = 0.5 * std::atan2(q.getB(), q.getA() - q.getC());
        axis = RS_Vector(std::cos(angle), std::sin(angle));
    }
    axis.normalize();

    RS_Vector perp(-axis.y, axis.x);

    double denom = std::abs(Ap) + std::abs(Cp) + 0.5 * std::abs(q.getB());
    double p = std::abs(valueAtCenter) / (denom + RS_TOLERANCE);

    if (p < 1e-6) {
        double linear = q.getB() * axis.x * axis.y + q.getD() * axis.x + q.getE() * axis.y;
        if (std::abs(linear) > RS_TOLERANCE) {
            p = std::abs(linear) / (2.0 * denom);
        } else {
            p = 1.0;
        }
    }

    if (p < 1e-6) {
        return new RS_Point(nullptr, RS_PointData(center));
    }

    if (valueAtCenter < 0.0) {
        perp = -perp;
    }

    std::array<RS_Vector, 3> cps = computeParabolaControlPoints(center, p, axis, perp);

    LC_ParabolaData data;
    data.m_controlPoints = cps;

    return new LC_Parabola(nullptr, data);
}

// ---------------------------------------------------------------------------
// 3. Hyperbola (both branches)
// ---------------------------------------------------------------------------
RS_Entity* createHyperbola(const LC_Quadratic& q,
                           const RS_Vector& center,
                           double valueAtCenter,
                           double Ap,
                           double Cp,
                           double theta)
{
    double aa2 = -valueAtCenter / Ap;
    double bb2 = -valueAtCenter / Cp;

    if (aa2 * bb2 >= 0.0) return nullptr;

    if (aa2 < 0.0) {
        std::swap(aa2, bb2);
        theta += M_PI / 2.0;
    }
    if (aa2 <= 0.0 || bb2 >= 0.0) return nullptr;

    double semiTrans = std::sqrt(aa2);
    double semiConj  = std::sqrt(-bb2);

    RS_Vector majorP = RS_Vector(std::cos(theta) * semiTrans, std::sin(theta) * semiTrans);

    auto container = new RS_EntityContainer(nullptr, true);

    // First branch
    LC_HyperbolaData data1;
    data1.center   = center;
    data1.majorP   = majorP;
    data1.ratio    = semiConj / semiTrans;
    data1.angle1   = -4.0;
    data1.angle2   = 4.0;
    data1.reversed = false;
    container->addEntity(new LC_Hyperbola(container, data1));

    // Second branch
    LC_HyperbolaData data2;
    data2.center   = center;
    data2.majorP   = -majorP;
    data2.ratio    = semiConj / semiTrans;
    data2.angle1   = -4.0;
    data2.angle2   = 4.0;
    data2.reversed = false;
    container->addEntity(new LC_Hyperbola(container, data2));

    return container;
}

RS_Entity* createDualAroundCenter(RS_Entity* entity, const RS_Vector& center)
{
    if (!entity || !center.valid) return nullptr;

    switch (entity->rtti()) {
    case RS2::EntityLine:
    {
        auto* line = static_cast<RS_Line*>(entity);
        RS_Vector normal = line->getNormalVector();
        double c = -normal.dotP(line->getStartpoint().move(-center));
        if (RS_Math::equal(c, 0.0)) return nullptr;
        return new RS_Point{nullptr, {RS_Vector{normal.x / c, normal.y / c} + center}};
    }
    case RS2::EntityPoint:
    {
        auto* point = static_cast<RS_Point*>(entity);
        RS_Vector pos = point->getPos().move(-center);
        if (RS_Math::equal(std::hypot(pos.x, pos.y), 0.0)) return nullptr;
        LC_Quadratic dualQ{{pos.x, pos.y, 1.0}};
        return dualQ.move(center).toEntity();
    }
    default:
        break;
    }

    LC_Quadratic q = entity->getQuadratic();
    if (!q.isValid()) return nullptr;

    LC_Quadratic dualQ = q.move(-center).getDualCurve().move(center);

    std::unique_ptr<RS_Entity> dualEntity(dualQ.toEntity());
    if (!dualEntity) return nullptr;

    dualEntity->setPen(entity->getPen());
    dualEntity->setLayer(entity->getLayer());
    dualEntity->setPenToActive();

    return dualEntity.release();
}

} // namespace LC_QuadraticUtils
//EOF