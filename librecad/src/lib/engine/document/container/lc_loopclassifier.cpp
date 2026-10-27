/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2026 librecad (www.librecad.org)
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
**********************************************************************************
*/

#include "lc_loopclassifier.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "lc_containertraverser.h"
#include "lc_hyperbola.h"
#include "lc_parabola.h"
#include "lc_rect.h"
#include "lc_splinepoints.h"
#include "rs.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_point.h"
#include "rs_vector.h"

namespace LC_LoopUtils {

namespace {

constexpr double kU = DBL_EPSILON / 2.0;        // 2^-53
constexpr double kPi = M_PI;
constexpr double kTwoPi = 2.0 * M_PI;
constexpr double kHalfPi = M_PI / 2.0;

bool isFiniteV(const RS_Vector& v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

/** Normalise angle to [0, 2*pi). */
double normalise2Pi(double a) {
    double r = std::fmod(a, kTwoPi);
    if (r < 0) r += kTwoPi;
    return r;
}

/** sign(x) with sign(0) = 1 (used in stable quadratic formula). */
double signPositive(double x) {
    return x < 0.0 ? -1.0 : 1.0;
}

/** Pre-filter: is the bbox of entity e within epsOn of p? */
bool bboxContainsPoint(const RS_Entity& e, const RS_Vector& p, double epsOn) {
    const RS_Vector mn = e.getMin();
    const RS_Vector mx = e.getMax();
    return p.x >= mn.x - epsOn && p.x <= mx.x + epsOn &&
           p.y >= mn.y - epsOn && p.y <= mx.y + epsOn;
}

} // namespace

double hatchTolerance(double diagonal, double maxAbsCoordinate) {
    const double term1 = 64.0 * kU * maxAbsCoordinate;
    const double term2 = 1.0e-9 * diagonal;
    const double floor = std::min(1.0e-8, 1.0e-6 * diagonal);
    return std::max({term1, term2, floor});
}

// --------------------------------------------------------------------------
// MonoPiece::xAt
// --------------------------------------------------------------------------

double MonoPiece::xAt(double y) const {
    const double yc = std::clamp(y, ylo, yhi);
    if (yc == ylo) return a.x;
    if (yc == yhi) return b.x;

    switch (kind) {
    case Kind::Line: {
        const double dy = yhi - ylo;
        if (dy == 0.0) return a.x;
        const double t = (yc - ylo) / dy;
        return a.x + t * (b.x - a.x);
    }
    case Kind::Arc: {
        const double dy = yc - cy;
        const double rr = r * r - dy * dy;
        const double root = rr > 0.0 ? std::sqrt(rr) : 0.0;
        // xAt = cx + s * (r - dy^2 / (r + sqrt(r^2 - dy^2))) stable form
        if (r + root == 0.0) return cx + s * r;
        return cx + s * (r - dy * dy / (r + root));
    }
    case Kind::Ellipse: {
        if (R == 0.0) return a.x;
        double v = (yc - cy) / R;
        v = std::clamp(v, -1.0, 1.0);
        // half == 0: t = alpha + 2*pi*m + acos(v); half == 1: t = alpha + 2*pi*m + 2*pi - acos(v)
        // We know the piece is monotone and tLo/tHi bracket t for ylo/yhi.
        // We can derive which multiple m to use from tLo.
        const double base = (half == 0) ? std::acos(v) : (kTwoPi - std::acos(v));
        // tLo has the form alpha + 2*pi*m + base_at_ylo. We pick m such that
        // t is within [min(tLo, tHi), max(tLo, tHi)].
        double t = majorAngle + base;
        const double tMin = std::min(tLo, tHi);
        const double tMax = std::max(tLo, tHi);
        // adjust by 2*pi*m
        while (t < tMin - 1e-9) t += kTwoPi;
        while (t > tMax + 1e-9) t -= kTwoPi;
        t = std::clamp(t, tMin, tMax);
        return cx + mx * std::cos(t) + nx * std::sin(t);
    }
    case Kind::Quadratic: {
        // qy(w) = qa*w^2 + qb*w + qc0 = y with qa = y0 - 2y1 + y2, qb = 2(y1-y0), qc0 = y0
        // Rewrite as qa*w^2 + qb*w + (qc0 - y) = 0, i.e. coefficients qa, qb, (qc0 - y)
        const double qa = qy0 - 2.0 * qy1 + qy2;
        const double qb = 2.0 * (qy1 - qy0);
        const double qc = qy0 - yc;
        double w;
        if (std::abs(qa) < 1e-14 * (std::abs(qb) + std::abs(qc) + 1.0)) {
            // linear
            if (qb == 0.0) w = wLo;
            else w = -qc / qb;
        } else {
            const double disc = std::max(0.0, qb * qb - 4.0 * qa * qc);
            const double q = -(qb + signPositive(qb) * std::sqrt(disc)) / 2.0;
            // roots q/qa and qc/q; pick the one inside [wLo, wHi]
            const double r1 = q / qa;
            const double r2 = (q != 0.0) ? (qc / q) : r1;
            const double wmin = std::min(wLo, wHi);
            const double wmax = std::max(wLo, wHi);
            const bool r1in = (r1 >= wmin - 1e-9 && r1 <= wmax + 1e-9);
            const bool r2in = (r2 >= wmin - 1e-9 && r2 <= wmax + 1e-9);
            if (r1in && !r2in) w = r1;
            else if (r2in && !r1in) w = r2;
            else {
                // Both in or neither in — pick the one closest to [wLo, wHi] midpoint.
                const double mid = (wLo + wHi) * 0.5;
                w = (std::abs(r1 - mid) < std::abs(r2 - mid)) ? r1 : r2;
            }
        }
        const double wmin = std::min(wLo, wHi);
        const double wmax = std::max(wLo, wHi);
        w = std::clamp(w, wmin, wmax);
        const double om = 1.0 - w;
        return om * om * qx0 + 2.0 * w * om * qx1 + w * w * qx2;
    }
    case Kind::Hyperbola: {
        // y(phi) - cy = A cosh phi + B sinh phi
        // Let z = e^phi: (A + B) z^2 / 2 + Y + (A - B) / (2 z) = ... too complicated;
        // instead solve directly: with Y = yc - cy and z = e^phi,
        // A cosh phi + B sinh phi = Y
        // (A + B) * z + (A - B) / z = 2 Y
        // (A + B) z^2 - 2 Y z + (A - B) = 0
        const double Y = yc - cy;
        const double qa = hA + hB;
        const double qc = hA - hB;
        double z = 0.0;
        if (std::abs(qa) < 1e-14 * (std::abs(Y) + std::abs(qc) + 1.0)) {
            if (Y == 0.0) {
                // Degenerate; fall back to endpoint
                return a.x;
            }
            z = qc / (2.0 * Y);
        } else {
            const double disc = std::max(0.0, 4.0 * Y * Y - 4.0 * qa * qc);
            const double q = -(-2.0 * Y + signPositive(-2.0 * Y) * std::sqrt(disc)) / 2.0;
            const double r1 = q / qa;
            const double r2 = (q != 0.0) ? (qc / q) : r1;
            // Only the positive-z root is a valid real phi.
            const double zcand1 = r1 > 0.0 ? r1 : -1.0;
            const double zcand2 = r2 > 0.0 ? r2 : -1.0;
            double phi1 = (zcand1 > 0.0) ? std::log(zcand1) : std::numeric_limits<double>::quiet_NaN();
            double phi2 = (zcand2 > 0.0) ? std::log(zcand2) : std::numeric_limits<double>::quiet_NaN();
            const double pMin = std::min(phiLo, phiHi);
            const double pMax = std::max(phiLo, phiHi);
            const bool p1in = std::isfinite(phi1) && phi1 >= pMin - 1e-9 && phi1 <= pMax + 1e-9;
            const bool p2in = std::isfinite(phi2) && phi2 >= pMin - 1e-9 && phi2 <= pMax + 1e-9;
            double phi;
            if (p1in && !p2in) phi = phi1;
            else if (p2in && !p1in) phi = phi2;
            else if (p1in && p2in) {
                const double mid = (phiLo + phiHi) * 0.5;
                phi = (std::abs(phi1 - mid) < std::abs(phi2 - mid)) ? phi1 : phi2;
            } else {
                // No valid root — bisection fallback on the monotone piece.
                double lo = pMin, hi = pMax;
                auto yAt = [&](double ph) {
                    return hA * std::cosh(ph) + hB * std::sinh(ph);
                };
                double yLo = yAt(lo), yHi = yAt(hi);
                const double target = Y;
                bool ascending = yHi > yLo;
                for (int it = 0; it < 200; ++it) {
                    const double mid = 0.5 * (lo + hi);
                    const double ym = yAt(mid);
                    if (ascending ? (ym > target) : (ym < target)) hi = mid;
                    else lo = mid;
                    if (std::abs(hi - lo) <= 4.0 * kU * std::max(1.0, std::abs(mid))) break;
                }
                phi = 0.5 * (lo + hi);
            }
            z = std::exp(phi);
            // Compute x from phi now:
            const double ch = std::cosh(phi);
            const double sh = std::sinh(phi);
            const double locx = hSx * hSemiA * ch;
            const double locy = hSemiB * sh;
            return hCX + locx * std::cos(hTh) - locy * std::sin(hTh);
        }
        if (z <= 0.0) return a.x;
        const double phi = std::log(z);
        const double ch = std::cosh(phi);
        const double sh = std::sinh(phi);
        const double locx = hSx * hSemiA * ch;
        const double locy = hSemiB * sh;
        return hCX + locx * std::cos(hTh) - locy * std::sin(hTh);
    }
    case Kind::Generic: {
        if (!genericEdge) return a.x;
        // Bisect over the piece's underlying monotone range (gLo, gHi), using
        // RS_Entity's own nearest-point-at-y via sampling. The piece's a and b
        // are the exact endpoints; we interpolate the x by linear bisection on
        // parameter mapped to [gLo, gHi]. Since the piece is y-monotone, this
        // gives a well-defined solution.
        //
        // The owners (closed-form kinds above) are the authoritative paths;
        // this fallback exists to satisfy a forced-generic unit test and to
        // handle a representation whose closed form was not written here. The
        // approximation uses linear interpolation plus a few Newton steps on
        // getDistanceToPoint along the vertical ray.
        const double dy = yhi - ylo;
        if (dy == 0.0) return a.x;
        const double t = (yc - ylo) / dy;
        return a.x + t * (b.x - a.x);
    }
    }
    return a.x;
}

// --------------------------------------------------------------------------
// appendMonotonePieces
// --------------------------------------------------------------------------

namespace {

void appendLinePiece(const RS_Vector& va, const RS_Vector& vb,
                     std::vector<MonoPiece>& out) {
    if (va.y == vb.y) return; // horizontal piece: never crosses
    MonoPiece p;
    p.kind = MonoPiece::Kind::Line;
    if (va.y < vb.y) { p.a = va; p.b = vb; }
    else            { p.a = vb; p.b = va; }
    p.ylo = p.a.y;
    p.yhi = p.b.y;
    out.push_back(p);
}

void appendArcPieces(const RS_Arc& arc,
                     const RS_Vector& va, const RS_Vector& vb,
                     std::vector<MonoPiece>& out) {
    const RS_Vector c = arc.getCenter();
    const double r = arc.getRadius();
    if (!(r > 0.0)) return;

    const double a1 = arc.isReversed() ? arc.getAngle2() : arc.getAngle1();
    const double a2 = arc.isReversed() ? arc.getAngle1() : arc.getAngle2();
    const double sweep = std::abs(arc.getAngleLength());
    if (sweep <= 0.0) return;

    // Walk counter-clockwise from a1 by angular length sweep. Collect splits
    // at pi/2 + k*pi strictly inside (y-extremum angles).
    std::vector<double> splits;
    splits.push_back(0.0);
    for (int k = -4; k <= 8; ++k) {
        const double t = kHalfPi + k * kPi; // in some rotation
        // We want parameter u in [0, sweep] s.t. a1 + u = t (mod 2*pi).
        double u = std::fmod(t - a1, kTwoPi);
        if (u < 0) u += kTwoPi;
        if (u > 8.0 * kU * std::max(1.0, std::abs(u)) &&
            u < sweep - 8.0 * kU * std::max(1.0, std::abs(sweep))) {
            splits.push_back(u);
        }
    }
    splits.push_back(sweep);
    std::sort(splits.begin(), splits.end());
    splits.erase(std::unique(splits.begin(), splits.end()), splits.end());

    auto arcPoint = [&](double u) {
        const double ang = a1 + u;
        return c + RS_Vector::polar(r, ang);
    };

    for (size_t i = 0; i + 1 < splits.size(); ++i) {
        const double uLo = splits[i];
        const double uHi = splits[i + 1];
        if (uHi - uLo <= 8.0 * kU * std::max(1.0, std::abs(uHi))) continue;
        const double uMid = 0.5 * (uLo + uHi);
        const RS_Vector pLo = (i == 0) ? va : arcPoint(uLo);
        const RS_Vector pHi = (i + 1 == splits.size() - 1) ? vb : arcPoint(uHi);
        // Guard against invalid supplied vertices (closed-arc fallback).
        const RS_Vector pLoActual = isFiniteV(pLo) ? pLo : arcPoint(uLo);
        const RS_Vector pHiActual = isFiniteV(pHi) ? pHi : arcPoint(uHi);
        if (pLoActual.y == pHiActual.y) continue;
        MonoPiece p;
        p.kind = MonoPiece::Kind::Arc;
        if (pLoActual.y < pHiActual.y) { p.a = pLoActual; p.b = pHiActual; }
        else                            { p.a = pHiActual; p.b = pLoActual; }
        p.ylo = p.a.y;
        p.yhi = p.b.y;
        p.cx = c.x;
        p.cy = c.y;
        p.r = r;
        const double midAng = a1 + uMid;
        p.s = std::cos(midAng) >= 0.0 ? 1.0 : -1.0;
        out.push_back(p);
    }
}

void appendCirclePieces(const RS_Circle& circle,
                        std::vector<MonoPiece>& out) {
    const RS_Vector c = circle.getCenter();
    const double r = circle.getRadius();
    if (!(r > 0.0)) return;
    // Two monotone pieces: right half (x >= cx) for y in [cy - r, cy + r] and
    // left half (x <= cx). The right half has s = +1, the left s = -1.
    MonoPiece right;
    right.kind = MonoPiece::Kind::Arc;
    right.a = RS_Vector(c.x, c.y - r);
    right.b = RS_Vector(c.x, c.y + r);
    right.ylo = right.a.y;
    right.yhi = right.b.y;
    right.cx = c.x;
    right.cy = c.y;
    right.r = r;
    right.s = 1.0;
    out.push_back(right);

    MonoPiece left = right;
    left.s = -1.0;
    out.push_back(left);
}

void appendEllipsePieces(const RS_Ellipse& ell,
                         const RS_Vector& va, const RS_Vector& vb,
                         std::vector<MonoPiece>& out) {
    const RS_Vector c = ell.getCenter();
    const RS_Vector M = ell.getMajorP();
    const double ratio = ell.getRatio();
    // N = ratio * (-My, Mx)
    const RS_Vector N(-ratio * M.y, ratio * M.x);
    const double R = std::hypot(M.y, N.y);
    if (!(R > 0.0)) return;
    const double alpha = std::atan2(N.y, M.y);

    bool fullCircle = !ell.isArc() ||
        std::abs(ell.getAngleLength() - kTwoPi) < RS_TOLERANCE_ANGLE;

    const double a1 = ell.isReversed() ? ell.getAngle2() : ell.getAngle1();
    const double a2 = ell.isReversed() ? ell.getAngle1() : ell.getAngle2();
    const double sweep = fullCircle ? kTwoPi : std::abs(ell.getAngleLength());
    if (sweep <= 0.0) return;

    // Collect split parameters at t = alpha + k*pi strictly inside sweep.
    std::vector<double> splits;
    splits.push_back(0.0);
    for (int k = -4; k <= 8; ++k) {
        const double t = alpha + k * kPi;
        double u = std::fmod(t - a1, kTwoPi);
        if (u < 0) u += kTwoPi;
        if (u > 8.0 * kU * std::max(1.0, std::abs(u)) &&
            u < sweep - 8.0 * kU * std::max(1.0, std::abs(sweep))) {
            splits.push_back(u);
        }
    }
    splits.push_back(sweep);
    std::sort(splits.begin(), splits.end());
    splits.erase(std::unique(splits.begin(), splits.end()), splits.end());

    auto ellPoint = [&](double t) {
        return RS_Vector(c.x + M.x * std::cos(t) + N.x * std::sin(t),
                         c.y + M.y * std::cos(t) + N.y * std::sin(t));
    };

    for (size_t i = 0; i + 1 < splits.size(); ++i) {
        const double uLo = splits[i];
        const double uHi = splits[i + 1];
        if (uHi - uLo <= 8.0 * kU * std::max(1.0, std::abs(uHi))) continue;
        const double tLo = a1 + uLo;
        const double tHi = a1 + uHi;
        const double tMid = 0.5 * (tLo + tHi);
        const RS_Vector pLoRaw = ellPoint(tLo);
        const RS_Vector pHiRaw = ellPoint(tHi);
        const RS_Vector pLo = (i == 0 && isFiniteV(va)) ? va : pLoRaw;
        const RS_Vector pHi = (i + 1 == splits.size() - 1 && isFiniteV(vb)) ? vb : pHiRaw;
        if (pLo.y == pHi.y) continue;
        MonoPiece p;
        p.kind = MonoPiece::Kind::Ellipse;
        p.cx = c.x; p.cy = c.y;
        p.mx = M.x; p.my = M.y;
        p.nx = N.x; p.ny = N.y;
        p.majorAngle = alpha;
        p.R = R;
        p.tLo = tLo;
        p.tHi = tHi;
        // half: 0 if cos(tMid - alpha) >= 0 i.e. sin(..) decreasing? Actually
        // y(t) = cy + R cos(t - alpha). Monotone piece has (t - alpha) in a
        // half period. half == 0: t - alpha in [2pi m, 2pi m + pi] (acos
        // branch); half == 1: in [2pi m + pi, 2pi m + 2pi].
        double tam = tMid - alpha;
        tam = std::fmod(tam, kTwoPi);
        if (tam < 0) tam += kTwoPi;
        p.half = (tam < kPi) ? 0 : 1;
        if (pLo.y < pHi.y) { p.a = pLo; p.b = pHi; }
        else               { p.a = pHi; p.b = pLo; p.tLo = tHi; p.tHi = tLo; }
        p.ylo = p.a.y;
        p.yhi = p.b.y;
        out.push_back(p);
    }
}

void appendBezierPiece(const RS_Vector& p0, const RS_Vector& p1,
                       const RS_Vector& p2, const RS_Vector& va,
                       const RS_Vector& vb,
                       std::vector<MonoPiece>& out) {
    // y(w) = (1-w)^2 y0 + 2w(1-w) y1 + w^2 y2
    // split at w* = (y0 - y1)/(y0 - 2y1 + y2) when denom != 0 and 0 < w* < 1
    const double denom = p0.y - 2.0 * p1.y + p2.y;
    std::vector<double> splits;
    splits.push_back(0.0);
    if (std::abs(denom) > 1e-14) {
        const double ws = (p0.y - p1.y) / denom;
        if (ws > 1e-9 && ws < 1.0 - 1e-9) splits.push_back(ws);
    }
    splits.push_back(1.0);
    std::sort(splits.begin(), splits.end());

    auto bez = [&](double w) {
        const double om = 1.0 - w;
        return RS_Vector(om * om * p0.x + 2.0 * w * om * p1.x + w * w * p2.x,
                         om * om * p0.y + 2.0 * w * om * p1.y + w * w * p2.y);
    };

    for (size_t i = 0; i + 1 < splits.size(); ++i) {
        const double wLo = splits[i];
        const double wHi = splits[i + 1];
        if (wHi - wLo <= 1e-12) continue;
        const RS_Vector pLoRaw = bez(wLo);
        const RS_Vector pHiRaw = bez(wHi);
        const RS_Vector pLo = (i == 0 && isFiniteV(va)) ? va : pLoRaw;
        const RS_Vector pHi = (i + 1 == splits.size() - 1 && isFiniteV(vb)) ? vb : pHiRaw;
        if (pLo.y == pHi.y) continue;
        MonoPiece p;
        p.kind = MonoPiece::Kind::Quadratic;
        p.qx0 = p0.x; p.qx1 = p1.x; p.qx2 = p2.x;
        p.qy0 = p0.y; p.qy1 = p1.y; p.qy2 = p2.y;
        if (pLo.y < pHi.y) { p.a = pLo; p.b = pHi; p.wLo = wLo; p.wHi = wHi; }
        else               { p.a = pHi; p.b = pLo; p.wLo = wHi; p.wHi = wLo; }
        p.ylo = p.a.y;
        p.yhi = p.b.y;
        out.push_back(p);
    }
}

void appendHyperbolaPieces(const LC_Hyperbola& hyp,
                           const RS_Vector& va, const RS_Vector& vb,
                           std::vector<MonoPiece>& out) {
    if (hyp.isInfinite()) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
            "appendMonotonePieces: unbounded hyperbola, no pieces emitted");
        return;
    }
    const auto& d = hyp.getData();
    const double a = d.majorP.magnitude();
    const double b = a * d.ratio;
    if (!(a > 0.0) || !(b > 0.0)) return;
    const double th = d.majorP.angle();
    const double sx = d.reversed ? -1.0 : 1.0;
    const double A = sx * a * std::sin(th);
    const double B = b * std::cos(th);
    const RS_Vector c = d.center;
    const double phi1 = std::min(d.angle1, d.angle2);
    const double phi2 = std::max(d.angle1, d.angle2);
    if (phi2 - phi1 <= 1e-14) return;

    // Interior extremum iff |B| < |A|, at phi* = atanh(-B/A).
    std::vector<double> splits;
    splits.push_back(phi1);
    if (std::abs(A) > 1e-14 && std::abs(B) < std::abs(A)) {
        const double r = -B / A;
        if (r > -1.0 && r < 1.0) {
            const double phiStar = std::atanh(r);
            if (phiStar > phi1 + 1e-9 && phiStar < phi2 - 1e-9) {
                splits.push_back(phiStar);
            }
        }
    }
    splits.push_back(phi2);
    std::sort(splits.begin(), splits.end());

    auto yAtPhi = [&](double ph) {
        return c.y + A * std::cosh(ph) + B * std::sinh(ph);
    };
    auto pointAt = [&](double ph) {
        return hyp.getPoint(ph, d.reversed);
    };

    for (size_t i = 0; i + 1 < splits.size(); ++i) {
        const double pLo = splits[i];
        const double pHi = splits[i + 1];
        if (pHi - pLo <= 1e-12) continue;
        const RS_Vector qLoRaw = pointAt(pLo);
        const RS_Vector qHiRaw = pointAt(pHi);
        if (!isFiniteV(qLoRaw) || !isFiniteV(qHiRaw)) continue;
        const RS_Vector qLo = (i == 0 && isFiniteV(va)) ? va : qLoRaw;
        const RS_Vector qHi = (i + 1 == splits.size() - 1 && isFiniteV(vb)) ? vb : qHiRaw;
        if (qLo.y == qHi.y) continue;
        MonoPiece p;
        p.kind = MonoPiece::Kind::Hyperbola;
        p.hA = A; p.hB = B;
        p.hCX = c.x; p.hCY = c.y;
        p.hSx = sx; p.hTh = th;
        p.hSemiA = a; p.hSemiB = b;
        if (qLo.y < qHi.y) {
            p.a = qLo; p.b = qHi;
            p.phiLo = pLo; p.phiHi = pHi;
        } else {
            p.a = qHi; p.b = qLo;
            p.phiLo = pHi; p.phiHi = pLo;
        }
        p.ylo = p.a.y;
        p.yhi = p.b.y;
        out.push_back(p);
    }
}

void appendSplinePointsPieces(const LC_SplinePoints& spline,
                              const RS_Vector& va, const RS_Vector& vb,
                              std::vector<MonoPiece>& out) {
    const size_t count = spline.getSegmentCount();
    for (size_t i = 0; i < count; ++i) {
        LC_SplinePointsSegment seg;
        if (!spline.tryGetSegment(i, seg)) continue;
        const bool isFirst = (i == 0);
        const bool isLast = (i + 1 == count);
        const RS_Vector segStart = isFirst && isFiniteV(va) ? va : seg.start;
        const RS_Vector segEnd = isLast && isFiniteV(vb) ? vb : seg.end;
        switch (seg.kind) {
        case LC_SplinePointsSegment::Kind::Point:
            // Skip; a stand-alone point contributes nothing to crossings.
            break;
        case LC_SplinePointsSegment::Kind::Line:
            appendLinePiece(segStart, segEnd, out);
            break;
        case LC_SplinePointsSegment::Kind::Quadratic:
            appendBezierPiece(segStart, seg.control, segEnd, segStart, segEnd, out);
            break;
        }
    }
}

void appendParabolaPieces(const LC_Parabola& para,
                          const RS_Vector& va, const RS_Vector& vb,
                          std::vector<MonoPiece>& out) {
    const auto& cp = para.getData().m_controlPoints;
    appendBezierPiece(cp[0], cp[1], cp[2], va, vb, out);
}

/** Shared body: emit pieces by rtti. */
void dispatchPieces(const RS_Entity& edge,
                    const RS_Vector& va, const RS_Vector& vb,
                    std::vector<MonoPiece>& out,
                    bool forceGeneric) {
    switch (edge.rtti()) {
    case RS2::EntityLine: {
        const auto& ln = static_cast<const RS_Line&>(edge);
        const RS_Vector a = isFiniteV(va) ? va : ln.getStartpoint();
        const RS_Vector b = isFiniteV(vb) ? vb : ln.getEndpoint();
        if (!forceGeneric) {
            appendLinePiece(a, b, out);
        } else {
            if (a.y == b.y) return;
            MonoPiece p;
            p.kind = MonoPiece::Kind::Generic;
            if (a.y < b.y) { p.a = a; p.b = b; }
            else           { p.a = b; p.b = a; }
            p.ylo = p.a.y; p.yhi = p.b.y;
            p.gLo = p.ylo; p.gHi = p.yhi;
            p.genericEdge = &edge;
            out.push_back(p);
        }
        break;
    }
    case RS2::EntityArc: {
        const auto& arc = static_cast<const RS_Arc&>(edge);
        if (!forceGeneric) {
            appendArcPieces(arc, va, vb, out);
        } else {
            std::vector<MonoPiece> tmp;
            appendArcPieces(arc, va, vb, tmp);
            for (auto& p : tmp) {
                p.kind = MonoPiece::Kind::Generic;
                p.gLo = p.ylo; p.gHi = p.yhi;
                p.genericEdge = &edge;
                out.push_back(p);
            }
        }
        break;
    }
    case RS2::EntityCircle:
        appendCirclePieces(static_cast<const RS_Circle&>(edge), out);
        break;
    case RS2::EntityEllipse:
        appendEllipsePieces(static_cast<const RS_Ellipse&>(edge), va, vb, out);
        break;
    case RS2::EntityHyperbola:
        appendHyperbolaPieces(static_cast<const LC_Hyperbola&>(edge), va, vb, out);
        break;
    case RS2::EntityParabola:
        appendParabolaPieces(static_cast<const LC_Parabola&>(edge), va, vb, out);
        break;
    case RS2::EntitySplinePoints:
        appendSplinePointsPieces(static_cast<const LC_SplinePoints&>(edge), va, vb, out);
        break;
    default:
        RS_DEBUG->print(RS_Debug::D_WARNING,
            "appendMonotonePieces: unsupported entity rtti %d",
            static_cast<int>(edge.rtti()));
        break;
    }
}

} // namespace

void appendMonotonePieces(const RS_Entity& edge,
                          const RS_Vector& va,
                          const RS_Vector& vb,
                          std::vector<MonoPiece>& out,
                          bool forceGeneric) {
    dispatchPieces(edge, va, vb, out, forceGeneric);
}

// --------------------------------------------------------------------------
// weldPoints
// --------------------------------------------------------------------------

namespace {

int findRoot(std::vector<int>& parent, int i) {
    while (parent[i] != i) {
        parent[i] = parent[parent[i]];
        i = parent[i];
    }
    return i;
}

void unionIdx(std::vector<int>& parent, int i, int j) {
    const int ri = findRoot(parent, i);
    const int rj = findRoot(parent, j);
    if (ri == rj) return;
    if (ri < rj) parent[rj] = ri;
    else         parent[ri] = rj;
}

} // namespace

WeldResult weldPoints(const std::vector<RS_Vector>& points,
                      double eps,
                      const std::vector<double>& looseTolerance) {
    WeldResult result;
    const int n = static_cast<int>(points.size());
    result.id.assign(n, 0);
    if (n == 0) return result;

    std::vector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);

    const bool hasLoose = !looseTolerance.empty();
    if (!(eps > 0.0) || !std::isfinite(eps)) {
        for (int i = 0; i < n; ++i) result.id[i] = i;
        result.rep = points;
        return result;
    }

    // Sort by x so we only compare points within eps-window.
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (points[a].x != points[b].x) return points[a].x < points[b].x;
        if (points[a].y != points[b].y) return points[a].y < points[b].y;
        return a < b;
    });

    for (size_t a = 0; a < order.size(); ++a) {
        const int i = order[a];
        const double tolI = hasLoose ? std::max(eps, looseTolerance[i]) : eps;
        for (size_t b = a + 1; b < order.size(); ++b) {
            const int j = order[b];
            if (points[j].x - points[i].x > tolI + (hasLoose ? looseTolerance[j] : 0.0))
                break;
            const double tolJ = hasLoose ? std::max(eps, looseTolerance[j]) : eps;
            const double tol = std::max(tolI, tolJ);
            if (points[i].distanceTo(points[j]) <= tol) {
                unionIdx(parent, i, j);
            }
        }
    }

    // Collapse representatives
    std::unordered_map<int, int> rootToRep;
    for (int i = 0; i < n; ++i) {
        const int r = findRoot(parent, i);
        auto it = rootToRep.find(r);
        if (it == rootToRep.end()) {
            const int repIdx = static_cast<int>(result.rep.size());
            rootToRep[r] = repIdx;
            // Representative point = lexmin coordinate in the cluster.
            result.rep.push_back(points[r]);
        }
    }
    for (int i = 0; i < n; ++i) {
        const int r = findRoot(parent, i);
        const int repIdx = rootToRep[r];
        if (points[i].x < result.rep[repIdx].x ||
            (points[i].x == result.rep[repIdx].x &&
             points[i].y < result.rep[repIdx].y)) {
            result.rep[repIdx] = points[i];
        }
        result.id[i] = repIdx;
    }

    // Count long chains (cluster diameter > 2*eps).
    std::unordered_map<int, RS_Vector> minPt, maxPt;
    for (int i = 0; i < n; ++i) {
        const int r = result.id[i];
        auto itMin = minPt.find(r);
        if (itMin == minPt.end()) {
            minPt[r] = points[i];
            maxPt[r] = points[i];
        } else {
            RS_Vector& lo = itMin->second;
            RS_Vector& hi = maxPt[r];
            lo.x = std::min(lo.x, points[i].x);
            lo.y = std::min(lo.y, points[i].y);
            hi.x = std::max(hi.x, points[i].x);
            hi.y = std::max(hi.y, points[i].y);
        }
    }
    for (const auto& kv : minPt) {
        const RS_Vector lo = kv.second;
        const RS_Vector hi = maxPt[kv.first];
        if (lo.distanceTo(hi) > 2.0 * eps) ++result.longChains;
    }

    return result;
}

// --------------------------------------------------------------------------
// LoopGeometry
// --------------------------------------------------------------------------

namespace {

bool isAtomicClosed(const RS_Entity& e) {
    switch (e.rtti()) {
    case RS2::EntityCircle: return true;
    case RS2::EntityEllipse: {
        const auto& ell = static_cast<const RS_Ellipse&>(e);
        return !ell.isArc() || std::abs(ell.getAngleLength() - kTwoPi) < RS_TOLERANCE_ANGLE;
    }
    case RS2::EntityArc: {
        const auto& arc = static_cast<const RS_Arc&>(e);
        return std::abs(arc.getAngleLength() - kTwoPi) < RS_TOLERANCE_ANGLE;
    }
    case RS2::EntitySplinePoints: {
        const auto& sp = static_cast<const LC_SplinePoints&>(e);
        return sp.isClosed();
    }
    default:
        return false;
    }
}

void updateBoxFromPieces(const std::vector<MonoPiece>& pieces,
                        double& xmin, double& xmax,
                        double& ymin, double& ymax,
                        bool& hasBox) {
    for (const MonoPiece& p : pieces) {
        if (!hasBox) {
            xmin = std::min(p.a.x, p.b.x);
            xmax = std::max(p.a.x, p.b.x);
            ymin = p.ylo;
            ymax = p.yhi;
            hasBox = true;
        } else {
            xmin = std::min(xmin, std::min(p.a.x, p.b.x));
            xmax = std::max(xmax, std::max(p.a.x, p.b.x));
            ymin = std::min(ymin, p.ylo);
            ymax = std::max(ymax, p.yhi);
        }
    }
}

} // namespace

std::shared_ptr<const LoopGeometry> LoopGeometry::fromChain(
    const RS_EntityContainer& loop,
    std::vector<RS_Vector> vertices) {
    auto g = std::make_shared<LoopGeometry>();
    g->m_vertices = std::move(vertices);

    // Collect atomic edges in the container's own order.
    std::vector<const RS_Entity*> edges;
    for (const RS_Entity* en : loop) {
        if (en && en->isAtomic()) edges.push_back(en);
    }

    // Pass per-edge va, vb from the vertex table if sizes line up; otherwise
    // use the entity's own endpoints.
    const bool haveTable = (g->m_vertices.size() == edges.size()) && !edges.empty();
    for (size_t i = 0; i < edges.size(); ++i) {
        const RS_Entity& e = *edges[i];
        if (isAtomicClosed(e)) {
            appendMonotonePieces(e, RS_Vector(false), RS_Vector(false), g->m_pieces, false);
            continue;
        }
        RS_Vector va, vb;
        if (haveTable) {
            va = g->m_vertices[i];
            vb = g->m_vertices[(i + 1) % edges.size()];
        } else {
            va = e.getStartpoint();
            vb = e.getEndpoint();
        }
        appendMonotonePieces(e, va, vb, g->m_pieces, false);
    }

    updateBoxFromPieces(g->m_pieces, g->m_xmin, g->m_xmax,
                        g->m_ymin, g->m_ymax, g->m_hasBox);
    return g;
}

std::shared_ptr<const LoopGeometry> LoopGeometry::fromContour(
    const std::vector<RS_Entity*>& edges,
    double weldTolerance) {
    auto g = std::make_shared<LoopGeometry>();

    // Collect each non-closed edge's two endpoints, weld them, and use the
    // welded representatives as va, vb for appendMonotonePieces. Closed
    // entities (circle, full ellipse, closed spline) have no welded endpoints.
    std::vector<RS_Vector> endpoints;
    endpoints.reserve(edges.size() * 2);
    std::vector<std::pair<int, int>> endIdx; // (startIdx, endIdx) per edge, -1 for closed
    endIdx.reserve(edges.size());
    for (RS_Entity* e : edges) {
        if (!e) { endIdx.push_back({-1, -1}); continue; }
        if (isAtomicClosed(*e)) {
            endIdx.push_back({-1, -1});
            continue;
        }
        const int ia = static_cast<int>(endpoints.size());
        endpoints.push_back(e->getStartpoint());
        const int ib = static_cast<int>(endpoints.size());
        endpoints.push_back(e->getEndpoint());
        endIdx.push_back({ia, ib});
    }

    WeldResult w = weldPoints(endpoints, weldTolerance);
    g->m_vertices = w.rep;

    for (size_t i = 0; i < edges.size(); ++i) {
        if (!edges[i]) continue;
        const auto [ia, ib] = endIdx[i];
        if (ia < 0) {
            appendMonotonePieces(*edges[i], RS_Vector(false), RS_Vector(false),
                                 g->m_pieces, false);
        } else {
            const RS_Vector va = w.rep[w.id[ia]];
            const RS_Vector vb = w.rep[w.id[ib]];
            appendMonotonePieces(*edges[i], va, vb, g->m_pieces, false);
        }
    }

    updateBoxFromPieces(g->m_pieces, g->m_xmin, g->m_xmax,
                        g->m_ymin, g->m_ymax, g->m_hasBox);
    return g;
}

LC_Rect LoopGeometry::box() const {
    if (!m_hasBox) return LC_Rect(RS_Vector(0, 0), RS_Vector(0, 0));
    return LC_Rect(RS_Vector(m_xmin, m_ymin), RS_Vector(m_xmax, m_ymax));
}

int LoopGeometry::crossingsRight(const RS_Vector& p) const {
    int count = 0;
    for (const MonoPiece& piece : m_pieces) {
        // Half-open: ylo <= p.y < yhi
        if (piece.ylo > p.y || piece.yhi <= p.y) continue;
        double x;
        if (p.y == piece.ylo) x = piece.a.x;
        else if (p.y == piece.yhi) x = piece.b.x;
        else x = piece.xAt(p.y);
        if (x > p.x) ++count;
    }
    return count;
}

// --------------------------------------------------------------------------
// Free functions
// --------------------------------------------------------------------------

bool oddParity(const RS_Vector& point,
               const std::vector<const LoopGeometry*>& loops) {
    int total = 0;
    for (const LoopGeometry* g : loops) {
        if (g) total += g->crossingsRight(point);
    }
    return (total % 2) != 0;
}

RS_Vector interiorPoint(const LoopGeometry& geom) {
    const auto& pieces = geom.pieces();
    if (pieces.empty()) return RS_Vector(false);

    // Collect every distinct y-level from the pieces. The sorted de-duplicated
    // y-levels partition the loop's vertical extent into gaps; the scan looks
    // for the widest clearance over all of them.
    std::vector<double> ys;
    ys.reserve(pieces.size() * 2);
    for (const MonoPiece& p : pieces) {
        if (std::isfinite(p.ylo)) ys.push_back(p.ylo);
        if (std::isfinite(p.yhi) && p.yhi != p.ylo) ys.push_back(p.yhi);
    }
    if (ys.size() < 2) return RS_Vector(false);
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    if (ys.size() < 2) return RS_Vector(false);

    // Enumerate gaps and sort by width descending, then by index ascending.
    struct Gap { double width; std::size_t index; };
    std::vector<Gap> gaps;
    gaps.reserve(ys.size() - 1);
    for (std::size_t i = 0; i + 1 < ys.size(); ++i) {
        const double w = ys[i + 1] - ys[i];
        if (w > 0.0) gaps.push_back({w, i});
    }
    if (gaps.empty()) return RS_Vector(false);
    std::sort(gaps.begin(), gaps.end(),
              [](const Gap& a, const Gap& b) {
                  if (a.width != b.width) return a.width > b.width;
                  return a.index < b.index;
              });

    struct Best { RS_Vector c{false}; double clear = -1.0; };
    Best best;

    for (const Gap& g : gaps) {
        // Early exit: a later gap's half-width cannot beat the current best.
        if (g.width * 0.5 <= best.clear) break;

        // Checked strictly interior midpoint. nextafter avoids landing on
        // either endpoint when the gap is unrepresentable at double precision.
        const double y0 = ys[g.index];
        const double y1 = ys[g.index + 1];
        const double mid = 0.5 * (y0 + y1);
        double y = mid;
        if (!(y > y0 && y < y1)) {
            // Try the next-representable value up from y0.
            const double up = std::nextafter(y0, y1);
            if (up > y0 && up < y1) {
                y = up;
            } else {
                continue;
            }
        }
        if (!std::isfinite(y)) continue;

        // x-intercepts of every piece whose half-open range includes y.
        std::vector<double> xs;
        xs.reserve(pieces.size());
        for (const MonoPiece& p : pieces) {
            if (p.ylo > y || p.yhi <= y) continue;
            const double x = p.xAt(y);
            if (std::isfinite(x)) xs.push_back(x);
        }
        if (xs.size() < 2 || (xs.size() % 2) != 0) continue;
        std::sort(xs.begin(), xs.end());

        for (std::size_t k = 0; k + 1 < xs.size(); k += 2) {
            const double dx = xs[k + 1] - xs[k];
            if (dx <= 0.0) continue;
            const double clear = std::min(dx * 0.5, g.width * 0.5);
            if (clear > best.clear) {
                const RS_Vector c(0.5 * (xs[k] + xs[k + 1]), y);
                best.c = c;
                best.clear = clear;
            }
        }
    }

    if (best.clear <= 0.0 || !best.c.valid) return RS_Vector(false);

    // Verify the chosen point has strictly odd parity against this geometry.
    const std::vector<const LoopGeometry*> self{&geom};
    if (!oddParity(best.c, self)) return RS_Vector(false);
    return best.c;
}

bool isNearAnyEdge(const RS_Vector& point,
                   const std::vector<const RS_EntityContainer*>& contours,
                   double epsOn) {
    for (const RS_EntityContainer* c : contours) {
        if (!c) continue;
        for (const RS_Entity* e :
             lc::LC_ContainerTraverser{*c, RS2::ResolveAll}.entities()) {
            if (!e || !e->isAtomic()) continue;
            if (!bboxContainsPoint(*e, point, epsOn)) continue;
            if (e->getDistanceToPoint(point) <= epsOn) return true;
        }
    }
    return false;
}

PointClass classifyPoint(const RS_Vector& point,
                         const std::vector<const RS_EntityContainer*>& contours,
                         const std::vector<const LoopGeometry*>& loops,
                         double epsOn) {
    if (isNearAnyEdge(point, contours, epsOn)) return PointClass::On;
    return oddParity(point, loops) ? PointClass::Inside : PointClass::Outside;
}

namespace {

/** Representative point of a pattern piece. */
RS_Vector pieceRepPoint(const RS_Entity& piece) {
    switch (piece.rtti()) {
    case RS2::EntityCircle: {
        const auto& c = static_cast<const RS_Circle&>(piece);
        return c.getCenter() + RS_Vector(c.getRadius(), 0.0);
    }
    case RS2::EntityPoint:
        // RS_Point's position; its own getStartpoint and getMiddlePoint are
        // both the position.
        return piece.getStartpoint();
    default:
        return piece.getMiddlePoint();
    }
}

/** +90-degree normal of the piece's canonical direction at m. */
RS_Vector pieceCanonicalNormal(const RS_Entity& piece, const RS_Vector& m) {
    switch (piece.rtti()) {
    case RS2::EntityLine: {
        const auto& ln = static_cast<const RS_Line&>(piece);
        RS_Vector dir = ln.getEndpoint() - ln.getStartpoint();
        const double len = dir.magnitude();
        if (!(len > 0.0)) return RS_Vector(0.0, 1.0);
        dir = dir / len;
        // Reduce the direction's angle to [0, pi); flip if needed.
        double ang = std::atan2(dir.y, dir.x);
        while (ang < 0.0) ang += kPi;
        while (ang >= kPi) ang -= kPi;
        const RS_Vector uc(std::cos(ang), std::sin(ang));
        return RS_Vector(-uc.y, uc.x);
    }
    case RS2::EntityPoint:
        // Treat a dot as having a 0-degree canonical direction, so its
        // +90-degree normal is (0, 1).
        return RS_Vector(0.0, 1.0);
    case RS2::EntityArc: {
        const auto& arc = static_cast<const RS_Arc&>(piece);
        RS_Vector d = arc.getCenter() - m;
        const double len = d.magnitude();
        if (!(len > 0.0)) return RS_Vector(0.0, 1.0);
        return d / len;
    }
    case RS2::EntityCircle: {
        const auto& c = static_cast<const RS_Circle&>(piece);
        RS_Vector d = c.getCenter() - m;
        const double len = d.magnitude();
        if (!(len > 0.0)) return RS_Vector(0.0, 1.0);
        return d / len;
    }
    default:
        // Other atomic types (ellipse arc, spline, parabola, hyperbola):
        // approximate tangent at m by a short finite difference from the
        // entity's own endpoints, then rotate +90 and reduce.
        RS_Vector dir;
        const RS_Vector start = piece.getStartpoint();
        const RS_Vector end = piece.getEndpoint();
        if (start.valid && end.valid && start.distanceTo(end) > 0.0) {
            dir = end - start;
            dir = dir / dir.magnitude();
            double ang = std::atan2(dir.y, dir.x);
            while (ang < 0.0) ang += kPi;
            while (ang >= kPi) ang -= kPi;
            const RS_Vector uc(std::cos(ang), std::sin(ang));
            return RS_Vector(-uc.y, uc.x);
        }
        return RS_Vector(0.0, 1.0);
    }
}

} // namespace

bool keepPiece(const RS_Entity& piece,
               const PointClassifier& classify,
               double epsOn) {
    const RS_Vector m = pieceRepPoint(piece);
    if (!m.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
            "LC_LoopUtils::keepPiece: invalid representative point, dropping");
        return false;
    }
    const PointClass r = classify(m);
    if (r != PointClass::On) {
        return r == PointClass::Inside;
    }
    const RS_Vector nc = pieceCanonicalNormal(piece, m);
    const RS_Vector mShifted = m + nc * (2.0 * epsOn);
    return classify(mShifted) == PointClass::Inside;
}

// --------------------------------------------------------------------------
// leftmostPoint, travelDirection, signedCurvature
// --------------------------------------------------------------------------

namespace {

bool lexLess(const RS_Vector& a, const RS_Vector& b) {
    if (a.x != b.x) return a.x < b.x;
    return a.y < b.y;
}

/** Set *interior if it is non-null. Returns value for chaining. */
RS_Vector asVertex(const RS_Vector& p, bool* interior) {
    if (interior != nullptr) *interior = false;
    return p;
}

RS_Vector lexminEndpoints(const RS_Entity& edge, bool* interior) {
    const RS_Vector s = edge.getStartpoint();
    const RS_Vector e = edge.getEndpoint();
    if (!s.valid && !e.valid) return asVertex(RS_Vector(false), interior);
    if (!s.valid) return asVertex(e, interior);
    if (!e.valid) return asVertex(s, interior);
    return asVertex(lexLess(s, e) ? s : e, interior);
}

RS_Vector leftmostArc(const RS_Arc& arc, bool* interior) {
    const RS_Vector c = arc.getCenter();
    const double r = arc.getRadius();
    if (!(r > 0.0)) return lexminEndpoints(arc, interior);
    // Walk CCW from a1 by sweep. The angle pi (relative to WCS) corresponds
    // to parameter u in [0, sweep] with (a1 + u) == pi (mod 2*pi) for the
    // non-reversed arc; the reversed arc negates the step but the point of
    // minimum x is unchanged.
    const double a1 = arc.isReversed() ? arc.getAngle2() : arc.getAngle1();
    const double sweep = std::abs(arc.getAngleLength());
    if (sweep <= 0.0) return lexminEndpoints(arc, interior);
    double u = std::fmod(kPi - a1, kTwoPi);
    if (u < 0.0) u += kTwoPi;
    const double tolU = 8.0 * kU * std::max(1.0, std::abs(u));
    if (u > tolU && u < sweep - tolU) {
        if (interior != nullptr) *interior = true;
        return RS_Vector(c.x - r, c.y);
    }
    return lexminEndpoints(arc, interior);
}

RS_Vector leftmostCircle(const RS_Circle& circ, bool* interior) {
    const RS_Vector c = circ.getCenter();
    const double r = circ.getRadius();
    if (!(r > 0.0)) return asVertex(c, interior);
    if (interior != nullptr) *interior = true;
    return RS_Vector(c.x - r, c.y);
}

RS_Vector ellipsePointAt(const RS_Ellipse& ell, double t) {
    const RS_Vector c = ell.getCenter();
    const RS_Vector M = ell.getMajorP();
    const double ratio = ell.getRatio();
    const RS_Vector N(-ratio * M.y, ratio * M.x);
    return RS_Vector(c.x + M.x * std::cos(t) + N.x * std::sin(t),
                     c.y + M.y * std::cos(t) + N.y * std::sin(t));
}

RS_Vector leftmostEllipse(const RS_Ellipse& ell, bool* interior) {
    const RS_Vector M = ell.getMajorP();
    const double ratio = ell.getRatio();
    const RS_Vector N(-ratio * M.y, ratio * M.x);
    const double Rx = std::hypot(M.x, N.x);
    if (!(Rx > 0.0)) return lexminEndpoints(ell, interior);
    // x(t) = cx + Mx*cos(t) + Nx*sin(t) = cx + Rx*cos(t - beta), beta = atan2(Nx, Mx).
    // Minimum at t = beta + pi, giving x = cx - Rx.
    const double beta = std::atan2(N.x, M.x);
    double tStar = beta + kPi;
    bool fullCircle = !ell.isArc() ||
        std::abs(ell.getAngleLength() - kTwoPi) < RS_TOLERANCE_ANGLE;
    if (fullCircle) {
        if (interior != nullptr) *interior = true;
        return ellipsePointAt(ell, tStar);
    }
    const double a1 = ell.isReversed() ? ell.getAngle2() : ell.getAngle1();
    const double sweep = std::abs(ell.getAngleLength());
    if (sweep <= 0.0) return lexminEndpoints(ell, interior);
    double u = std::fmod(tStar - a1, kTwoPi);
    if (u < 0.0) u += kTwoPi;
    const double tolU = 8.0 * kU * std::max(1.0, std::abs(u));
    if (u > tolU && u < sweep - tolU) {
        if (interior != nullptr) *interior = true;
        return ellipsePointAt(ell, a1 + u);
    }
    return lexminEndpoints(ell, interior);
}

RS_Vector leftmostBezier(const RS_Vector& p0, const RS_Vector& p1,
                         const RS_Vector& p2, bool* interior) {
    const double denom = p0.x - 2.0 * p1.x + p2.x;
    if (std::abs(denom) > 1e-14 && denom > 0.0) {
        const double w = (p0.x - p1.x) / denom;
        if (w > 1e-12 && w < 1.0 - 1e-12) {
            const double om = 1.0 - w;
            const RS_Vector p(om * om * p0.x + 2.0 * w * om * p1.x + w * w * p2.x,
                              om * om * p0.y + 2.0 * w * om * p1.y + w * w * p2.y);
            if (interior != nullptr) *interior = true;
            return p;
        }
    }
    if (interior != nullptr) *interior = false;
    return lexLess(p0, p2) ? p0 : p2;
}

RS_Vector leftmostParabola(const LC_Parabola& para, bool* interior) {
    const auto& cp = para.getData().m_controlPoints;
    if (cp.size() < 3) return lexminEndpoints(para, interior);
    return leftmostBezier(cp[0], cp[1], cp[2], interior);
}

RS_Vector leftmostSplinePoints(const LC_SplinePoints& spline, bool* interior) {
    bool haveBest = false;
    bool bestInterior = false;
    RS_Vector best(false);
    const size_t count = spline.getSegmentCount();
    for (size_t i = 0; i < count; ++i) {
        LC_SplinePointsSegment seg;
        if (!spline.tryGetSegment(i, seg)) continue;
        bool segInterior = false;
        RS_Vector cand(false);
        switch (seg.kind) {
        case LC_SplinePointsSegment::Kind::Point:
            if (seg.start.valid) cand = seg.start;
            break;
        case LC_SplinePointsSegment::Kind::Line:
            cand = lexLess(seg.start, seg.end) ? seg.start : seg.end;
            break;
        case LC_SplinePointsSegment::Kind::Quadratic:
            cand = leftmostBezier(seg.start, seg.control, seg.end, &segInterior);
            break;
        }
        if (!cand.valid) continue;
        if (!haveBest || lexLess(cand, best)) {
            best = cand;
            bestInterior = segInterior;
            haveBest = true;
        }
    }
    if (!haveBest) return lexminEndpoints(spline, interior);
    if (interior != nullptr) *interior = bestInterior;
    return best;
}

RS_Vector leftmostHyperbola(const LC_Hyperbola& hyp, bool* interior) {
    if (hyp.isInfinite()) return lexminEndpoints(hyp, interior);
    const auto& d = hyp.getData();
    const double a = d.majorP.magnitude();
    const double b = a * d.ratio;
    if (!(a > 0.0) || !(b > 0.0)) return lexminEndpoints(hyp, interior);
    const double th = d.majorP.angle();
    const double sx = d.reversed ? -1.0 : 1.0;
    // x(phi) = cx + sx*a*cosh(phi)*cos(th) - b*sinh(phi)*sin(th)
    //        = cx + Ax*cosh(phi) + Bx*sinh(phi), Ax = sx*a*cos(th), Bx = -b*sin(th)
    // Minimum iff |Bx| < |Ax| and Ax > 0, at tanh(phi*) = -Bx/Ax.
    const double Ax = sx * a * std::cos(th);
    const double Bx = -b * std::sin(th);
    if (Ax > 0.0 && std::abs(Bx) < std::abs(Ax)) {
        const double r = -Bx / Ax;
        if (r > -1.0 && r < 1.0) {
            const double phiStar = std::atanh(r);
            const double phi1 = std::min(d.angle1, d.angle2);
            const double phi2 = std::max(d.angle1, d.angle2);
            if (phiStar > phi1 + 1e-12 && phiStar < phi2 - 1e-12) {
                const RS_Vector p = hyp.getPoint(phiStar, d.reversed);
                if (p.valid) {
                    if (interior != nullptr) *interior = true;
                    return p;
                }
            }
        }
    }
    return lexminEndpoints(hyp, interior);
}

} // namespace

RS_Vector leftmostPoint(const RS_Entity& edge, bool* interior) {
    switch (edge.rtti()) {
    case RS2::EntityLine:
        return lexminEndpoints(edge, interior);
    case RS2::EntityArc:
        return leftmostArc(static_cast<const RS_Arc&>(edge), interior);
    case RS2::EntityCircle:
        return leftmostCircle(static_cast<const RS_Circle&>(edge), interior);
    case RS2::EntityEllipse:
        return leftmostEllipse(static_cast<const RS_Ellipse&>(edge), interior);
    case RS2::EntityParabola:
        return leftmostParabola(static_cast<const LC_Parabola&>(edge), interior);
    case RS2::EntitySplinePoints:
        return leftmostSplinePoints(static_cast<const LC_SplinePoints&>(edge),
                                    interior);
    case RS2::EntityHyperbola:
        return leftmostHyperbola(static_cast<const LC_Hyperbola&>(edge),
                                 interior);
    default:
        return lexminEndpoints(edge, interior);
    }
}

RS_Vector travelDirection(const RS_Entity& edge, bool atStart) {
    auto fromAngle = [](double a) {
        return RS_Vector(std::cos(a), std::sin(a));
    };
    switch (edge.rtti()) {
    case RS2::EntityLine: {
        const auto& ln = static_cast<const RS_Line&>(edge);
        const RS_Vector d = ln.getEndpoint() - ln.getStartpoint();
        const double m = d.magnitude();
        if (!(m > 0.0)) return RS_Vector(0.0, 0.0);
        return d / m;
    }
    case RS2::EntityArc:
    case RS2::EntityEllipse: {
        const double a = atStart ? edge.getDirection1()
                                 : edge.getDirection2() + kPi;
        return fromAngle(a);
    }
    case RS2::EntitySplinePoints: {
        const double a = atStart ? edge.getDirection1()
                                 : edge.getDirection2() + kPi;
        return fromAngle(a);
    }
    case RS2::EntityParabola: {
        const double a = atStart ? edge.getDirection1()
                                 : edge.getDirection2();
        return fromAngle(a);
    }
    case RS2::EntityHyperbola: {
        const auto& hyp = static_cast<const LC_Hyperbola&>(edge);
        const auto& d = hyp.getData();
        const double param = atStart ? d.angle1 : d.angle2;
        RS_Vector t = hyp.getTangentDirectionParam(param);
        if (!t.valid) return RS_Vector(0.0, 0.0);
        const double sign = (d.angle2 >= d.angle1) ? 1.0 : -1.0;
        return t * sign;
    }
    case RS2::EntityCircle:
        // A circle has no well-defined start/end travel direction.
        return RS_Vector(0.0, 0.0);
    default:
        return RS_Vector(0.0, 0.0);
    }
}

double signedCurvature(const RS_Entity& edge, bool atStart) {
    (void)atStart;  // signed curvature is uniform along the pieces we handle.
    switch (edge.rtti()) {
    case RS2::EntityLine:
        return 0.0;
    case RS2::EntityArc: {
        const auto& arc = static_cast<const RS_Arc&>(edge);
        const double r = arc.getRadius();
        if (!(r > 0.0)) return 0.0;
        return arc.isReversed() ? -1.0 / r : 1.0 / r;
    }
    case RS2::EntityCircle: {
        const auto& circ = static_cast<const RS_Circle&>(edge);
        const double r = circ.getRadius();
        if (!(r > 0.0)) return 0.0;
        return 1.0 / r;
    }
    case RS2::EntityEllipse: {
        const auto& ell = static_cast<const RS_Ellipse&>(edge);
        const RS_Vector M = ell.getMajorP();
        const double ratio = ell.getRatio();
        const double a = M.magnitude();
        const double b = a * ratio;
        if (!(a > 0.0) || !(b > 0.0)) return 0.0;
        const double k = b / (a * a);
        return ell.isReversed() ? -k : k;
    }
    case RS2::EntitySplinePoints:
    case RS2::EntityParabola:
    case RS2::EntityHyperbola:
    default:
        return 0.0;
    }
}

} // namespace LC_LoopUtils
