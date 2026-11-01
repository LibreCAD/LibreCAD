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
**********************************************************************/

#include "lc_hatchdensity.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

#include "lc_looputils.h"
#include "rs.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pattern.h"
#include "rs_vector.h"

namespace {

LC_HatchDensityPolicy& currentPolicy() noexcept {
    static LC_HatchDensityPolicy s_policy{};
    return s_policy;
}

std::uint64_t& currentRevision() noexcept {
    static std::uint64_t s_rev = 0;
    return s_rev;
}

// Thread-local pointer to the budget shared by every hatch clone of one
// insert expansion. nullptr outside a ScopedSharedBudget scope.
thread_local LC_HatchPieceBudget* t_sharedBudget = nullptr;

// -------- helpers for the P⊥ computation ----------

/** Total variation of n . p along an atomic entity (line / arc / ellipse arc).
 *  n is a unit normal perpendicular to the family direction. For other atomic
 *  types we sample the entity at 64 equal-length steps.
 */
double totalVariationOfProjection(const RS_Entity* e, const RS_Vector& n) {
    if (e == nullptr) return 0.0;
    switch (e->rtti()) {
    case RS2::EntityLine: {
        const auto* ln = static_cast<const RS_Line*>(e);
        const RS_Vector d = ln->getEndpoint() - ln->getStartpoint();
        return std::abs(RS_Vector::dotP(n, d));
    }
    case RS2::EntityArc:
    case RS2::EntityCircle: {
        // Full circle or arc: p(phi) = c + r (cos phi, sin phi).
        // n . p(phi) = n . c + r cos(phi) n.x + r sin(phi) n.y
        //           = n . c + r sin(phi - beta) where beta = atan2(-n.x, n.y)
        //           (so that sin(phi - beta) = n.x cos phi / |n| + ...).
        // We simply take extrema analytically: on each sweep, split at the
        // monotone peaks phi = beta + pi/2 + k pi.
        double c_x = 0.0, c_y = 0.0, r = 0.0, a0 = 0.0, a1 = 0.0;
        bool reversed = false;
        if (e->rtti() == RS2::EntityArc) {
            const auto* a = static_cast<const RS_Arc*>(e);
            c_x = a->getCenter().x;
            c_y = a->getCenter().y;
            r = a->getRadius();
            a0 = a->getAngle1();
            a1 = a->getAngle2();
            reversed = a->isReversed();
        } else {
            const auto* c = static_cast<const RS_Circle*>(e);
            c_x = c->getCenter().x;
            c_y = c->getCenter().y;
            r = c->getRadius();
            a0 = 0.0;
            a1 = 2.0 * M_PI;
            reversed = false;
        }
        if (!std::isfinite(r) || r <= 0.0) return 0.0;
        // Reduce to CCW sweep [a0, a0 + sweep).
        double sweep = reversed ? (a0 - a1) : (a1 - a0);
        while (sweep <= 0.0) sweep += 2.0 * M_PI;
        if (reversed) {
            // Walk from a1 up to a1+sweep.
            a0 = a1;
        }
        if (sweep > 2.0 * M_PI) sweep = 2.0 * M_PI;
        // Reduce n to a unit vector; a zero normal returns 0.
        const double nlen = std::hypot(n.x, n.y);
        if (!std::isfinite(nlen) || nlen <= 0.0) return 0.0;
        const RS_Vector un{n.x / nlen, n.y / nlen};
        // atan2(un.x, un.y) ties the peak phi where sin(phi - beta) = +/-1 to
        // the direction of un; see derivation above.
        const double beta = std::atan2(un.x, un.y);
        auto proj = [&](double phi) {
            return un.x * (c_x + r * std::cos(phi))
                 + un.y * (c_y + r * std::sin(phi));
        };
        // Collect split points at beta + pi/2 + k pi within (a0, a0+sweep).
        std::vector<double> xs;
        xs.push_back(a0);
        for (int k = -4; k <= 4; ++k) {
            const double phi = beta + M_PI_2 + static_cast<double>(k) * M_PI;
            if (phi > a0 && phi < a0 + sweep) {
                xs.push_back(phi);
            }
        }
        xs.push_back(a0 + sweep);
        std::sort(xs.begin(), xs.end());
        double tv = 0.0;
        for (std::size_t i = 1; i < xs.size(); ++i) {
            tv += std::abs(proj(xs[i]) - proj(xs[i - 1]));
        }
        return tv * nlen;
    }
    case RS2::EntityEllipse: {
        const auto* el = static_cast<const RS_Ellipse*>(e);
        const RS_Vector c = el->getCenter();
        const RS_Vector majorP = el->getMajorP();
        const double ratio = el->getRatio();
        const double a0 = el->getAngle1();
        const double a1 = el->getAngle2();
        const bool reversed = el->isReversed();
        const bool isFull = !el->isArc()
            || std::abs(el->getAngleLength() - 2.0 * M_PI) < RS_TOLERANCE_ANGLE;
        // The ellipse param uses rotation = atan2(majorP.y, majorP.x), semi-axes
        //   M = |majorP|, V = ratio * M along the perpendicular.
        // In world: p(t) = c + M * (cos t) u + V * (sin t) v,
        //   where u = majorP / |majorP|, v = u rotated by +90°.
        const double M = std::hypot(majorP.x, majorP.y);
        if (!std::isfinite(M) || M <= 0.0) return 0.0;
        const RS_Vector u{majorP.x / M, majorP.y / M};
        const RS_Vector v{-u.y, u.x};
        const double V = ratio * M;
        // Reduce n to unit normal.
        const double nlen = std::hypot(n.x, n.y);
        if (!std::isfinite(nlen) || nlen <= 0.0) return 0.0;
        const RS_Vector un{n.x / nlen, n.y / nlen};
        const double nu = RS_Vector::dotP(un, u);
        const double nv = RS_Vector::dotP(un, v);
        // n . p(t) = n . c + M nu cos t + V nv sin t
        //        = n . c + R cos(t - phi0),  R = hypot(M nu, V nv), phi0 = atan2(V nv, M nu).
        const double R = std::hypot(M * nu, V * nv);
        if (!std::isfinite(R)) return 0.0;
        const double phi0 = std::atan2(V * nv, M * nu);
        // n . p(t) = n . c + R cos(t - phi0), where R and phi0 collapse the
        // (A cos t + B sin t) terms into a single phase.
        auto projClosed = [&](double t) {
            return RS_Vector::dotP(un, c) + R * std::cos(t - phi0);
        };
        double start = a0;
        double sweep = 0.0;
        if (isFull) {
            start = 0.0;
            sweep = 2.0 * M_PI;
        } else {
            sweep = reversed ? (a0 - a1) : (a1 - a0);
            while (sweep <= 0.0) sweep += 2.0 * M_PI;
            if (reversed) start = a1;
            if (sweep > 2.0 * M_PI) sweep = 2.0 * M_PI;
        }
        std::vector<double> xs;
        xs.push_back(start);
        for (int k = -4; k <= 4; ++k) {
            const double t = phi0 + static_cast<double>(k) * M_PI;
            if (t > start && t < start + sweep) xs.push_back(t);
        }
        xs.push_back(start + sweep);
        std::sort(xs.begin(), xs.end());
        double tv = 0.0;
        for (std::size_t i = 1; i < xs.size(); ++i) {
            tv += std::abs(projClosed(xs[i]) - projClosed(xs[i - 1]));
        }
        return tv * nlen;
    }
    default: {
        // Generic fallback for LC_SplinePoints, LC_Parabola, LC_Hyperbola,
        // RS_Spline, ...: 64 equal-length samples along the entity.
        const double len = e->getLength();
        if (!std::isfinite(len) || len <= 0.0) return 0.0;
        const RS_Vector start = e->getStartpoint();
        constexpr int kSamples = 64;
        double prev = RS_Vector::dotP(n, start);
        double tv = 0.0;
        for (int i = 1; i <= kSamples; ++i) {
            const double d = len * static_cast<double>(i) / kSamples;
            RS_Vector p = e->getNearestDist(d, start);
            if (!p.valid) continue;
            const double cur = RS_Vector::dotP(n, p);
            if (std::isfinite(cur) && std::isfinite(prev)) {
                tv += std::abs(cur - prev);
            }
            prev = cur;
        }
        return tv;
    }
    }
}

/** Collect every atomic edge of a loop forest, outer and children, recursively. */
void collectAtomicEdges(const LC_LoopUtils::LC_Loops& loops,
                        std::vector<const RS_Entity*>& out) {
    const auto outer = loops.getOuterLoop();
    if (outer != nullptr) {
        for (const RS_Entity* e : *outer) {
            if (e != nullptr && e->isAtomic()) out.push_back(e);
        }
    }
    for (const auto& child : loops.getChildren()) {
        collectAtomicEdges(child, out);
    }
}

/** Recursive even-odd area accumulator. Depth-0 adds; children subtract. */
double evenOddAreaImpl(const LC_LoopUtils::LC_Loops& l, int depth) {
    const auto outer = l.getOuterLoop();
    double a = 0.0;
    if (outer != nullptr) {
        // RS_EntityContainer has areaLineIntegral(); we take |value| and
        // apply the depth sign ourselves.
        const double raw = outer->areaLineIntegral();
        a = (depth % 2 == 0 ? 1.0 : -1.0) * std::abs(raw);
    }
    for (const auto& c : l.getChildren()) {
        a += evenOddAreaImpl(c, depth + 1);
    }
    return a;
}

} // namespace

namespace LC_HatchDensity {

const LC_HatchDensityPolicy& policy() noexcept {
    return currentPolicy();
}

std::uint64_t policyRevision() noexcept {
    return currentRevision();
}

ScopedPolicy::ScopedPolicy(const LC_HatchDensityPolicy& p)
    : m_previous(currentPolicy()) {
    currentPolicy() = p;
    ++currentRevision();
}

ScopedPolicy::~ScopedPolicy() {
    currentPolicy() = m_previous;
    ++currentRevision();
}

double evenOddArea(const std::vector<LC_LoopUtils::LC_Loops>& roots) {
    double a = 0.0;
    for (const auto& root : roots) {
        a += evenOddAreaImpl(root, 0);
    }
    return a;
}

LC_HatchDensityReport estimate(const RS_Pattern& scaledPattern,
                               const std::vector<LC_LoopUtils::LC_Loops>& roots,
                               double patternTurn) {
    LC_HatchDensityReport r{};
    const RS_Vector sz = scaledPattern.getSize();
    const double Wt = sz.x;
    const double Ht = sz.y;
    const double At = Wt * Ht;
    if (!std::isfinite(Wt) || !std::isfinite(Ht) || !(At > 0.0)) {
        r.finite = false;
        return r;
    }

    // E, group lines by direction, sum curved perimeter.
    std::size_t E = 0;
    std::map<long long, double> groupLength;  // key: direction angle rounded to 1e-9 rad
    double curvedPerimeter = 0.0;
    for (const RS_Entity* e : scaledPattern) {
        if (e == nullptr || !e->isAtomic()) continue;
        ++E;
        if (e->rtti() == RS2::EntityLine) {
            const auto* ln = static_cast<const RS_Line*>(e);
            const RS_Vector d = ln->getEndpoint() - ln->getStartpoint();
            double angle = std::atan2(d.y, d.x);
            // Reduce to [0, pi) so antiparallel lines group together.
            while (angle < 0.0) angle += M_PI;
            while (angle >= M_PI) angle -= M_PI;
            const long long key =
                static_cast<long long>(std::llround(angle / 1e-9));
            groupLength[key] += std::hypot(d.x, d.y);
        } else {
            const double l = e->getLength();
            if (std::isfinite(l) && l > 0.0) curvedPerimeter += l;
        }
    }

    r.pieceDensity = static_cast<double>(E) / At;

    // Boundary edges and total boundary length.
    std::vector<const RS_Entity*> boundaries;
    for (const auto& root : roots) {
        collectAtomicEdges(root, boundaries);
    }
    double Lb = 0.0;
    for (const RS_Entity* b : boundaries) {
        if (b == nullptr) continue;
        const double l = b->getLength();
        if (std::isfinite(l) && l > 0.0) Lb += l;
    }
    r.lineDensity = Lb / At;

    // Even-odd area.
    r.area = evenOddArea(roots);
    if (!std::isfinite(r.area) || r.area < 0.0) {
        r.finite = false;
        return r;
    }
    r.areaTerm = static_cast<double>(E) * r.area / At;

    // boundaryTerm: for each group (angle alpha_g, length Σ ℓ_g), sum
    //   Σℓ_g * TV(n_perp) / (2 A_t), where n_perp = (-sin beta, cos beta)
    //   with beta = alpha_g + turn.
    double boundary = 0.0;
    for (const auto& kv : groupLength) {
        const double alpha =
            static_cast<double>(kv.first) * 1e-9 + patternTurn;
        const RS_Vector nvec{-std::sin(alpha), std::cos(alpha)};
        double tv = 0.0;
        for (const RS_Entity* b : boundaries) {
            tv += totalVariationOfProjection(b, nvec);
        }
        boundary += kv.second * tv / (2.0 * At);
    }
    // Curved pattern entities: approximate P_perp by (2 Lb / pi), the average
    // over all directions for a loop (the analytic estimate for arcs).
    if (curvedPerimeter > 0.0) {
        boundary += curvedPerimeter * (2.0 * Lb / M_PI) / (2.0 * At);
    }
    r.boundaryTerm = boundary;

    if (!std::isfinite(r.areaTerm) || !std::isfinite(r.boundaryTerm)
        || r.areaTerm < 0.0 || r.boundaryTerm < 0.0) {
        r.finite = false;
        return r;
    }
    r.estimatedPieces = r.areaTerm + r.boundaryTerm;
    r.finite = std::isfinite(r.estimatedPieces);
    return r;
}

double minimumScale(const LC_HatchDensityReport& r,
                    double currentScale,
                    double maxPieces) {
    if (!r.finite || !std::isfinite(currentScale) || currentScale <= 0.0
        || !std::isfinite(maxPieces) || maxPieces <= 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    // N(s*) = c1 (s/s*)^2 + c2 (s/s*). Let x = s/s*; solve c1 x^2 + c2 x = M.
    const double c1 = r.areaTerm;
    const double c2 = r.boundaryTerm;
    const double M = maxPieces;
    double x;
    if (c1 > 0.0) {
        const double disc = c2 * c2 + 4.0 * c1 * M;
        if (!std::isfinite(disc) || disc < 0.0) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        // Numerically stable: x = 2 M / (c2 + sqrt(disc)); avoids cancellation
        // when c1 is small compared with c2.
        x = 2.0 * M / (c2 + std::sqrt(disc));
    } else if (c2 > 0.0) {
        x = M / c2;
    } else {
        // Degenerate: empty density has no finite limiting scale.
        return std::numeric_limits<double>::infinity();
    }
    if (!std::isfinite(x) || x <= 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return currentScale / x;
}

double tintAlpha(const LC_HatchDensityReport& r,
                 double kPhys,
                 double wPhys) noexcept {
    // Non-finite report: fall back to opaque fill rather than leave the hatch
    // invisible on screen. The caller then draws the whole region.
    if (!r.finite) return 1.0;
    if (!std::isfinite(kPhys) || kPhys <= 0.0) return 1.0;
    if (!std::isfinite(wPhys) || wPhys <= 0.0) wPhys = 1.0;
    // Coverage predictor from the density note: f = w/k * line density
    // + (w/k)^2 * piece density. clamp(1 - exp(-f), 0, 1).
    const double rk = wPhys / kPhys;
    const double f = rk * r.lineDensity + rk * rk * r.pieceDensity;
    if (!std::isfinite(f) || f <= 0.0) return 0.0;
    const double a = 1.0 - std::exp(-f);
    if (!std::isfinite(a)) return 1.0;
    if (a <= 0.0) return 0.0;
    if (a >= 1.0) return 1.0;
    return a;
}

LC_HatchPieceBudget* sharedBudget() noexcept {
    return t_sharedBudget;
}

ScopedSharedBudget::ScopedSharedBudget(LC_HatchPieceBudget& b) noexcept
    : m_previous(t_sharedBudget) {
    t_sharedBudget = &b;
}

ScopedSharedBudget::~ScopedSharedBudget() {
    t_sharedBudget = m_previous;
}

} // namespace LC_HatchDensity
