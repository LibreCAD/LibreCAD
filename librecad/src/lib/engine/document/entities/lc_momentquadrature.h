/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
****************************************************************************/

// Eight-node Gauss-Legendre quadrature of Green's-theorem line integrals of
// (P - origin) along a parametric curve. The accumulator is LC_AreaMoments,
// which bundles ∬1, (∬x, ∬y) and (∬x², ∬y², ∬xy). The caller supplies an
// `evalRelative(t, origin, p_minus_o, dp)` functor that evaluates P(t) - origin
// and the raw tangent dP/dt for a scalar parameter t; `integrate` walks the
// interval [t0, t1] in |span|/maxStep sub-arcs and sums the six integrands
// per sub-arc. Reversed runs (t1 < t0) return the negated forward moments.
//
// Free helpers for the common closed forms (line segment, quadratic Bezier)
// skip the integrator because their integrands are low-degree polynomials
// that Green's theorem writes down directly. `signedSweep` is the arc-angle
// range reducer the hatch moment code uses: it preserves exact zeros, keeps
// nonzero short sweeps short, and only promotes an identically-equal pair of
// parameters to a full turn.

#ifndef LC_MOMENTQUADRATURE_H
#define LC_MOMENTQUADRATURE_H

#include <array>
#include <cmath>

#include "lc_secondmoment.h"
#include "rs_vector.h"

namespace LC_MomentQuadrature {

/// Positive abscissae of the 8-node Gauss-Legendre rule on [-1, 1]; combined
/// with the symmetric negatives they cover the eight nodes needed to make
/// the integrator exact for polynomials up to degree 15.
inline constexpr std::array<double, 4> kNodes{
    0.1834346424956498049394761,
    0.5255324099163289858177390,
    0.7966664774136267395915539,
    0.9602898564975362316835609};

/// The corresponding weights. They sum to one on [0, 1] (so to two with the
/// negatives on [-1, 1]).
inline constexpr std::array<double, 4> kWeights{
    0.3626837833783619829651504,
    0.3137066458778872873379622,
    0.2223810344533744705443560,
    0.1012285362903762591525314};

/**
 * @brief Range-reduce `a1 - a0` (or `a0 - a1` when reversed) into (-2π, 2π].
 *
 * Preserves `exactly-equal` endpoints as 2π (the documented full-turn
 * convention used by the loop extractor and the ellipse arc data), returns 0
 * when `a0 == a1 && !reversed`, i.e. passes "exactly equal parameters"
 * through only when the caller flags a full-turn arc. A nonzero short sweep
 * is never promoted to 2π based on endpoint tolerance alone.
 */
double signedSweep(double a0, double a1, bool reversed);

/**
 * @brief Area/moment contribution of a straight segment from `p` to `q` about
 *        `origin`, computed from the closed-form polynomial anti-derivatives.
 */
LC_AreaMoments segment(const RS_Vector& p, const RS_Vector& q,
                        const RS_Vector& origin);

/**
 * @brief Area/moment contribution of a quadratic Bezier segment with control
 *        points `p0`, `p1`, `p2` about `origin`. The integrands are degree-7
 *        polynomials; the 8-node rule is exact.
 */
LC_AreaMoments quadraticBezier(const RS_Vector& p0, const RS_Vector& p1,
                                const RS_Vector& p2, const RS_Vector& origin);

/**
 * @brief Walk [t0, t1] in |span|/maxStep sub-arcs and sum the six Green's
 *        theorem line integrals of (P - origin). The caller supplies
 *        `eval(t, origin, p_minus_o, dp)` which writes P(t) - origin to
 *        `p_minus_o` and the raw tangent dP/dt to `dp`. `maxStep` must be
 *        positive; a reversed call with t1 < t0 produces the negated run.
 */
template <class Eval>
LC_AreaMoments integrate(const Eval& eval, double t0, double t1,
                          double maxStep, const RS_Vector& origin)
{
    LC_AreaMoments out;
    if (!std::isfinite(t0) || !std::isfinite(t1) || !(maxStep > 0.0)) {
        return out;
    }
    const double span = t1 - t0;
    if (span == 0.0) {
        return out;
    }
    const double absSpan = std::fabs(span);
    // Checked ceil: never cast an unbounded or non-finite ratio.
    const double ratio = absSpan / maxStep;
    constexpr double kMaxSteps = 1.0e9;
    double stepsReal = std::ceil(ratio);
    if (!(stepsReal >= 1.0)) {
        stepsReal = 1.0;
    }
    if (stepsReal > kMaxSteps) {
        stepsReal = kMaxSteps;
    }
    const long steps = static_cast<long>(stepsReal);
    const double stepSpan = span / static_cast<double>(steps);
    const double half = stepSpan * 0.5;
    for (long k = 0; k < steps; ++k) {
        const double mid = t0 + (static_cast<double>(k) + 0.5) * stepSpan;
        for (std::size_t j = 0; j < kNodes.size(); ++j) {
            for (int s = -1; s <= 1; s += 2) {
                const double t = mid + static_cast<double>(s) * half * kNodes[j];
                RS_Vector pmo;
                RS_Vector dp;
                eval(t, origin, pmo, dp);
                if (!std::isfinite(pmo.x) || !std::isfinite(pmo.y) ||
                    !std::isfinite(dp.x) || !std::isfinite(dp.y)) {
                    continue;
                }
                const double x = pmo.x;
                const double y = pmo.y;
                const double w = kWeights[j] * half;
                out.area       += w * x * dp.y;
                out.first.mx   += w * x * x * dp.y * 0.5;
                out.first.my   -= w * y * y * dp.x * 0.5;
                out.second.ixx += w * x * x * x * dp.y / 3.0;
                out.second.iyy -= w * y * y * y * dp.x / 3.0;
                out.second.ixy += w * x * x * y * dp.y * 0.5;
            }
        }
    }
    return out;
}

}  // namespace LC_MomentQuadrature

#endif  // LC_MOMENTQUADRATURE_H
