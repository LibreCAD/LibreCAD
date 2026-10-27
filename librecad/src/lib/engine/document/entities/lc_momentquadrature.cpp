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

#include "lc_momentquadrature.h"

#include <cmath>

namespace LC_MomentQuadrature {

double signedSweep(double a0, double a1, bool reversed)
{
    if (!std::isfinite(a0) || !std::isfinite(a1)) {
        return 0.0;
    }
    // Exactly-equal parameters are the documented full-turn signal:
    // a 2.2.2-vintage arc with angle1 == angle2 covers the whole circle.
    if (a0 == a1) {
        return reversed ? -2.0 * M_PI : 2.0 * M_PI;
    }
    double d = reversed ? a0 - a1 : a1 - a0;
    constexpr double kTwoPi = 2.0 * M_PI;
    // Range-reduce into [0, 2π). Exact when d is already in [0, 2π).
    d -= kTwoPi * std::floor(d / kTwoPi);
    return reversed ? -d : d;
}

LC_AreaMoments segment(const RS_Vector& p, const RS_Vector& q,
                        const RS_Vector& origin)
{
    // Translate into the origin-local frame once, then use the closed-form
    // Green's-theorem polynomials for a straight segment.
    const double x0 = p.x - origin.x;
    const double y0 = p.y - origin.y;
    const double x1 = q.x - origin.x;
    const double y1 = q.y - origin.y;
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    LC_AreaMoments out;
    out.area       = 0.5 * dy * (x0 + x1);
    out.first.mx   =  dy * (x0 * x0 + x0 * x1 + x1 * x1) / 6.0;
    out.first.my   = -dx * (y0 * y0 + y0 * y1 + y1 * y1) / 6.0;
    out.second.ixx =  dy / 12.0 * (x0 + x1) * (x0 * x0 + x1 * x1);
    out.second.iyy = -dx / 12.0 * (y0 + y1) * (y0 * y0 + y1 * y1);
    out.second.ixy =  dy / 24.0 *
                     (x0 * x0 * (3.0 * y0 + y1)
                      + 2.0 * x0 * x1 * (y0 + y1)
                      + x1 * x1 * (y0 + 3.0 * y1));
    return out;
}

LC_AreaMoments quadraticBezier(const RS_Vector& p0, const RS_Vector& p1,
                                const RS_Vector& p2, const RS_Vector& origin)
{
    // B(t) = (1-t)^2 P0 + 2(1-t)t P1 + t^2 P2.
    // dB/dt = 2[(1-t)(P1 - P0) + t(P2 - P1)].
    // The six integrands are degree <= 7 polynomials; the 8-node Gauss-Legendre
    // rule is exact on [0, 1] with a single sub-arc (maxStep = 1).
    const double p0x = p0.x - origin.x;
    const double p0y = p0.y - origin.y;
    const double p1x = p1.x - origin.x;
    const double p1y = p1.y - origin.y;
    const double p2x = p2.x - origin.x;
    const double p2y = p2.y - origin.y;
    auto eval = [p0x, p0y, p1x, p1y, p2x, p2y](
                    double t, const RS_Vector& /*org*/, RS_Vector& pmo,
                    RS_Vector& dp) {
        const double u = 1.0 - t;
        const double b0 = u * u;
        const double b1 = 2.0 * u * t;
        const double b2 = t * t;
        pmo.x = b0 * p0x + b1 * p1x + b2 * p2x;
        pmo.y = b0 * p0y + b1 * p1y + b2 * p2y;
        pmo.valid = true;
        const double d1x = p1x - p0x;
        const double d1y = p1y - p0y;
        const double d2x = p2x - p1x;
        const double d2y = p2y - p1y;
        dp.x = 2.0 * (u * d1x + t * d2x);
        dp.y = 2.0 * (u * d1y + t * d2y);
        dp.valid = true;
    };
    return integrate(eval, 0.0, 1.0, 1.0, RS_Vector{0.0, 0.0});
}

}  // namespace LC_MomentQuadrature
