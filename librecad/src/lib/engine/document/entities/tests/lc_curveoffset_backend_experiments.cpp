/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

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
 ******************************************************************************/

// Experimental offset backends of the plan's series P, kept out of the
// production code: P1 recognizes spans whose offset is exact (straight,
// circular, polynomial Pythagorean-hodograph) and checks those offsets against
// direct evaluation; P2 approximates spans by G1 biarcs, offsets the arcs
// exactly, and measures them against the production Direct fitting. They run
// only on request ("[.experimental]") and report what the decision rests on.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <complex>
#include <functional>
#include <vector>

#include "lc_curveoffset.h"
#include "rs_spline.h"

using Catch::Approx;

namespace {

using Complex = std::complex<double>;

RS_Spline makeSpline(const size_t degree, const std::vector<RS_Vector>& controls, const std::vector<double>& knots,
                     std::vector<double> weights = {}) {
    RS_SplineData data(static_cast<int>(degree), false);
    data.controlPoints = controls;
    data.knotslist = knots;
    data.weights = weights.empty() ? std::vector<double>(controls.size(), 1.0) : std::move(weights);
    return RS_Spline(nullptr, data);
}

/** The exact Direct offset C + d N at t, the reference every backend is held to. */
RS_Vector directOffset(const RS_Spline& s, const double t, const double d) {
    LC_CurveJet j;
    REQUIRE(s.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, j));
    return j.point + RS_Vector{-j.first.y, j.first.x} / j.first.magnitude() * d;
}

// ---------------------------------------------------------------------------
// P1: exact recognition
// ---------------------------------------------------------------------------

/** A straight span: every control point on one line. Its offset is the span moved along the line's normal. */
bool recognizeStraight(const RS_SplineData& d, RS_Vector& normal) {
    const RS_Vector a = d.controlPoints.front();
    const RS_Vector b = d.controlPoints.back();
    const RS_Vector u = b - a;
    const double length = u.magnitude();
    if (!(length > 0.0)) {
        return false;
    }
    for (const RS_Vector& p : d.controlPoints) {
        if (std::abs(u.x * (p.y - a.y) - u.y * (p.x - a.x)) > 1e-12 * length * length) {
            return false;
        }
    }
    normal = RS_Vector{-u.y, u.x} / length;
    return true;
}

/**
 * A circular arc: a rational quadratic with equal legs whose middle weight is
 * the cosine of half the arc. Its offset is the arc scaled about the centre,
 * with the same parameterization.
 */
bool recognizeArc(const RS_SplineData& d, RS_Vector& centre, double& radius, bool& ccw) {
    if (d.degree != 2 || d.controlPoints.size() != 3 || d.weights.size() != 3) {
        return false;
    }
    const RS_Vector& p0 = d.controlPoints[0];
    const RS_Vector& p1 = d.controlPoints[1];
    const RS_Vector& p2 = d.controlPoints[2];
    const double leg0 = p0.distanceTo(p1);
    const double leg2 = p2.distanceTo(p1);
    if (std::abs(leg0 - leg2) > 1e-12 * (leg0 + leg2) || d.weights[0] != d.weights[2]) {
        return false;
    }
    const RS_Vector a = (p0 - p1) / leg0;
    const RS_Vector b = (p2 - p1) / leg2;
    const double halfAngle = 0.5 * std::acos(std::clamp(RS_Vector::dotP(a, b), -1.0, 1.0)); // at p1
    const double w = d.weights[1] / d.weights[0];
    if (std::abs(w - std::sin(halfAngle)) > 1e-12) { // cos of half the arc = sin of half the angle at p1
        return false;
    }
    const RS_Vector bisector = (a + b) / (a + b).magnitude();
    const double toCentre = leg0 / std::cos(halfAngle);
    centre = p1 + bisector * toCentre;
    radius = centre.distanceTo(p0);
    ccw = (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x) > 0.0;
    return true;
}

/** A polynomial PH cubic from the complex linear polynomial u + i v (Farouki's construction). */
RS_Spline phCubic(const RS_Vector& start, const Complex w0, const Complex w1) {
    const Complex p0{start.x, start.y};
    const Complex p1 = p0 + w0 * w0 / 3.0;
    const Complex p2 = p1 + w0 * w1 / 3.0;
    const Complex p3 = p2 + w1 * w1 / 3.0;
    return makeSpline(3,
                      {{p0.real(), p0.imag()}, {p1.real(), p1.imag()}, {p2.real(), p2.imag()},
                       {p3.real(), p3.imag()}},
                      {0, 0, 0, 0, 1, 1, 1, 1});
}

/**
 * Whether a cubic Bezier is PH: its hodograph's Bernstein coefficients
 * h0, h1, h2 are w0^2, w0 w1, w1^2 for some complex w0, w1. Returns them.
 */
bool recognizePh(const RS_SplineData& d, Complex& w0, Complex& w1) {
    if (d.degree != 3 || d.controlPoints.size() != 4) {
        return false;
    }
    auto c = [&d](const size_t i) { return Complex{d.controlPoints[i].x, d.controlPoints[i].y}; };
    const Complex h0 = 3.0 * (c(1) - c(0));
    const Complex h1 = 3.0 * (c(2) - c(1));
    const Complex h2 = 3.0 * (c(3) - c(2));
    w0 = std::sqrt(h0 / 3.0);
    w1 = std::sqrt(h2 / 3.0);
    const Complex expected = w0 * w1 * 3.0;
    const double scale = std::abs(h0) + std::abs(h1) + std::abs(h2);
    if (std::abs(expected - h1) <= 1e-12 * scale) {
        return true;
    }
    w1 = -w1; // the other root
    return std::abs(w0 * w1 * 3.0 - h1) <= 1e-12 * scale;
}

/** The PH cubic's exact offset: N = (-2uv, u^2 - v^2) / (u^2 + v^2), rational in t. */
RS_Vector phOffset(const RS_Spline& s, const Complex w0, const Complex w1, const double t, const double d) {
    const Complex w = w0 * (1.0 - t) + w1 * t;
    const double u = w.real();
    const double v = w.imag();
    const RS_Vector n = RS_Vector{-2.0 * u * v, u * u - v * v} / (u * u + v * v);
    return s.getPointAt(t) + n * d;
}

// ---------------------------------------------------------------------------
// P2: biarcs
// ---------------------------------------------------------------------------

struct Arc {
    RS_Vector start;
    RS_Vector end;
    RS_Vector centre{false}; // invalid for a straight segment
    double radius{0.0};
    bool ccw{true};
};

/** The arc from a with unit tangent ta to b; a segment when b lies on the tangent line. */
Arc arcThrough(const RS_Vector& a, const RS_Vector& ta, const RS_Vector& b) {
    Arc arc{a, b};
    const RS_Vector n{-ta.y, ta.x};
    const RS_Vector chord = b - a;
    const double lateral = RS_Vector::dotP(chord, n);
    if (std::abs(lateral) <= 1e-14 * chord.squared()) {
        return arc;
    }
    const double r = chord.squared() / (2.0 * lateral); // signed: positive turns left
    arc.centre = a + n * r;
    arc.radius = std::abs(r);
    arc.ccw = r > 0.0;
    return arc;
}

/** Unit tangent of the arc at its end point. */
RS_Vector endTangent(const Arc& arc) {
    if (!arc.centre.valid) {
        return (arc.end - arc.start) / arc.end.distanceTo(arc.start);
    }
    const RS_Vector r = (arc.end - arc.centre) / arc.radius;
    return arc.ccw ? RS_Vector{-r.y, r.x} : RS_Vector{r.y, -r.x};
}

/** The equal-tangent-length biarc from p0 (unit tangent t0) to p1 (unit tangent t1). */
std::pair<Arc, Arc> biarc(const RS_Vector& p0, const RS_Vector& t0, const RS_Vector& p1, const RS_Vector& t1) {
    const RS_Vector v = p1 - p0;
    const RS_Vector t = t0 + t1;
    const double c = 1.0 - RS_Vector::dotP(t0, t1);
    double alpha = 0.0;
    if (c < 1e-12) {
        alpha = v.squared() / (4.0 * RS_Vector::dotP(v, t1)); // parallel tangents
    }
    else {
        const double vt = RS_Vector::dotP(v, t);
        alpha = (-vt + std::sqrt(vt * vt + 2.0 * c * v.squared())) / (2.0 * c);
    }
    const RS_Vector joint = (p0 + t0 * alpha + p1 - t1 * alpha) * 0.5;
    const Arc first = arcThrough(p0, t0, joint);
    return {first, arcThrough(joint, endTangent(first), p1)};
}

double distanceToArc(const RS_Vector& p, const Arc& arc) {
    if (!arc.centre.valid) {
        const RS_Vector ab = arc.end - arc.start;
        const double s = std::clamp(RS_Vector::dotP(p - arc.start, ab) / ab.squared(), 0.0, 1.0);
        return p.distanceTo(arc.start + ab * s);
    }
    // on the arc's sweep: radial distance; otherwise the nearer end
    auto angle = [&arc](const RS_Vector& q) { return std::atan2(q.y - arc.centre.y, q.x - arc.centre.x); };
    double a0 = angle(arc.start);
    double a1 = angle(arc.end);
    double a = angle(p);
    if (!arc.ccw) {
        std::swap(a0, a1);
    }
    const double sweep = std::remainder(a1 - a0, 2.0 * M_PI) < 0 ? std::remainder(a1 - a0, 2.0 * M_PI) + 2.0 * M_PI
                                                                  : std::remainder(a1 - a0, 2.0 * M_PI);
    double along = std::remainder(a - a0, 2.0 * M_PI);
    if (along < 0.0) {
        along += 2.0 * M_PI;
    }
    if (along <= sweep) {
        return std::abs(p.distanceTo(arc.centre) - arc.radius);
    }
    return std::min(p.distanceTo(arc.start), p.distanceTo(arc.end));
}

/** The arc moved to its left by d: concentric, or the segment shifted. False where it collapses. */
bool offsetArc(const Arc& arc, const double d, Arc& out) {
    out = arc;
    if (!arc.centre.valid) {
        const RS_Vector u = (arc.end - arc.start) / arc.end.distanceTo(arc.start);
        const RS_Vector shift = RS_Vector{-u.y, u.x} * d;
        out.start = arc.start + shift;
        out.end = arc.end + shift;
        return true;
    }
    out.radius = arc.ccw ? arc.radius - d : arc.radius + d; // left of a left turn is the centre
    if (!(out.radius > 0.0)) {
        return false;
    }
    out.start = arc.centre + (arc.start - arc.centre) * (out.radius / arc.radius);
    out.end = arc.centre + (arc.end - arc.centre) * (out.radius / arc.radius);
    return true;
}

struct BiarcResult {
    std::vector<Arc> arcs;
    bool ok{true};
};

/**
 * Biarcs within @p position of the source, with tangents within @p angle so
 * that the offset of the approximation stays within |d| angle of the true
 * offset: the joint budget |C - P| + |d| |N_C - N_P|.
 */
BiarcResult fitBiarcs(const RS_Spline& s, const double position, const double angle) {
    BiarcResult result;
    const std::vector<double> breaks = s.getBreakParameters();
    struct Box {
        double a;
        double b;
        int depth;
    };
    for (size_t k = 0; k + 1 < breaks.size(); ++k) {
        std::vector<Box> stack{{breaks[k], breaks[k + 1], 0}};
        while (!stack.empty()) {
            const Box box = stack.back();
            stack.pop_back();
            LC_CurveJet ja;
            LC_CurveJet jb;
            REQUIRE(s.tryEvaluateJet(box.a, LC_CurveEvaluationSide::Right, ja));
            REQUIRE(s.tryEvaluateJet(box.b, LC_CurveEvaluationSide::Left, jb));
            const auto [first, second] =
                biarc(ja.point, ja.first / ja.first.magnitude(), jb.point, jb.first / jb.first.magnitude());
            bool fits = true;
            for (int i = 1; i < 16 && fits; ++i) {
                LC_CurveJet j;
                REQUIRE(s.tryEvaluateJet(box.a + (box.b - box.a) * i / 16.0, LC_CurveEvaluationSide::Interior, j));
                const double error = std::min(distanceToArc(j.point, first), distanceToArc(j.point, second));
                // the tangent of the nearer arc there, against the source's
                const Arc& near = distanceToArc(j.point, first) <= distanceToArc(j.point, second) ? first : second;
                RS_Vector tangent = (near.end - near.start) / near.end.distanceTo(near.start);
                if (near.centre.valid) {
                    const RS_Vector r = (j.point - near.centre) / j.point.distanceTo(near.centre);
                    tangent = near.ccw ? RS_Vector{-r.y, r.x} : RS_Vector{r.y, -r.x};
                }
                const RS_Vector unit = j.first / j.first.magnitude();
                const double turn = std::atan2(std::abs(unit.x * tangent.y - unit.y * tangent.x),
                                               RS_Vector::dotP(unit, tangent));
                fits = error <= position && turn <= angle;
            }
            if (fits) {
                result.arcs.push_back(first);
                result.arcs.push_back(second);
                continue;
            }
            if (box.depth >= 24) {
                result.ok = false;
                return result;
            }
            const double mid = 0.5 * (box.a + box.b);
            stack.push_back({mid, box.b, box.depth + 1});
            stack.push_back({box.a, mid, box.depth + 1});
        }
    }
    return result;
}

} // namespace

TEST_CASE("P1: straight and circular spans offset exactly, matching direct evaluation", "[.experimental]") {
    const RS_Spline straight = makeSpline(3, {{1, 1}, {3, 2}, {5, 3}, {9, 5}}, {0, 0, 0, 0, 1, 1, 1, 1});
    RS_Vector normal;
    REQUIRE(recognizeStraight(straight.getData(), normal));
    const RS_Spline arc = makeSpline(2, {{10, 0}, {10, 10}, {0, 10}}, {0, 0, 0, 1, 1, 1}, {1, std::sqrt(0.5), 1});
    RS_Vector centre;
    double radius = 0.0;
    bool ccw = false;
    REQUIRE(recognizeArc(arc.getData(), centre, radius, ccw));
    CHECK(centre.distanceTo(RS_Vector{0, 0}) < 1e-12);
    CHECK(radius == Approx(10.0).epsilon(1e-14));
    CHECK(ccw);
    for (const double d : {0.5, -2.0}) {
        for (int k = 0; k <= 20; ++k) {
            const double t = k / 20.0;
            // the straight span moved along its normal, point for point
            CHECK(directOffset(straight, t, d).distanceTo(straight.getPointAt(t) + normal * d) < 1e-12);
            // the arc scaled about its centre: the left of a counter-clockwise arc is inside
            const double scaled = (radius - d) / radius;
            CHECK(directOffset(arc, t, d).distanceTo(centre + (arc.getPointAt(t) - centre) * scaled) < 1e-12);
        }
    }
    // a near-arc is not an arc
    const RS_Spline almost = makeSpline(2, {{10, 0}, {10, 10}, {0, 10}}, {0, 0, 0, 1, 1, 1}, {1, 0.7071, 1});
    CHECK_FALSE(recognizeArc(almost.getData(), centre, radius, ccw));
}

TEST_CASE("P1: a PH cubic is recognized and its rational offset is the direct one", "[.experimental]") {
    const RS_Spline ph = phCubic({2.0, -1.0}, Complex{2.0, 1.0}, Complex{1.5, -1.2});
    Complex w0;
    Complex w1;
    REQUIRE(recognizePh(ph.getData(), w0, w1));
    for (const double d : {0.3, -0.8}) {
        double worst = 0.0;
        for (int k = 0; k <= 50; ++k) {
            const double t = k / 50.0;
            worst = std::max(worst, phOffset(ph, w0, w1, t, d).distanceTo(directOffset(ph, t, d)));
        }
        CHECK(worst < 1e-12);
    }
    // The offset is rational of degree 5 (a cubic over the quadratic u^2 + v^2),
    // beyond RS_Spline's degrees 1 to 3: an exact PH path needs that model first.
    // A generic cubic is not PH:
    const RS_Spline generic = makeSpline(3, {{0, 0}, {4, 6}, {8, -6}, {12, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    CHECK_FALSE(recognizePh(generic.getData(), w0, w1));
}

TEST_CASE("P2: biarc offsets against the Direct fitting", "[.experimental]") {
    struct Shape {
        const char* name;
        RS_SplineData data; // a spline is built per case: RS_Spline is not copied
        double distance;
    };
    std::vector<RS_Vector> wave;
    std::vector<RS_Vector> spiral;
    for (int i = 0; i < 20; ++i) {
        wave.emplace_back(10.0 * i, 15.0 * std::sin(0.7 * i));
        const double a = 0.35 * i;
        spiral.emplace_back((40.0 + 4.0 * i) * std::cos(a), (40.0 + 4.0 * i) * std::sin(a));
    }
    std::vector<double> knots{0, 0, 0, 0};
    for (int k = 1; k <= 16; ++k) {
        knots.push_back(k);
    }
    knots.insert(knots.end(), 4, 17.0);
    std::vector<Shape> shapes;
    shapes.push_back(
        {"s-curve", makeSpline(3, {{0, 0}, {4, 6}, {8, -6}, {12, 0}}, {0, 0, 0, 0, 1, 1, 1, 1}).getData(), 0.75});
    shapes.push_back({"quarter circle",
                      makeSpline(2, {{50, 0}, {50, 50}, {0, 50}}, {0, 0, 0, 1, 1, 1}, {1, std::sqrt(0.5), 1}).getData(),
                      5.0});
    shapes.push_back({"wave", makeSpline(3, wave, knots).getData(), 1.0});
    shapes.push_back({"spiral", makeSpline(3, spiral, knots).getData(), 2.0});
    for (const Shape& entry : shapes) {
        struct {
            const char* name;
            RS_Spline spline;
            double distance;
        } shape{entry.name, RS_Spline(nullptr, entry.data), entry.distance};
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(shape.spline, shape.distance);
        const double tolerance = options.tolerance.requestedGeometry;

        const auto directStart = std::chrono::steady_clock::now();
        const LC_CurveOffsetMaterializationResult direct = LC_CurveOffset::createEntities(
            shape.spline, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, shape.distance), options,
            LC_CurveOffset::makeDirectSourceBudget());
        const double directMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - directStart).count();
        REQUIRE(direct.status == LC_CurveOffsetStatus::Ok);

        // half the budget on position, half on the normal's effect at the distance
        const auto biarcStart = std::chrono::steady_clock::now();
        const BiarcResult fitted = fitBiarcs(shape.spline, 0.5 * tolerance, 0.5 * tolerance / shape.distance);
        std::vector<Arc> offsets;
        bool collapsed = false;
        for (const Arc& arc : fitted.arcs) {
            Arc moved;
            collapsed = collapsed || !offsetArc(arc, shape.distance, moved);
            offsets.push_back(moved);
        }
        const double biarcMs =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - biarcStart).count();
        REQUIRE(fitted.ok);

        // the biarc offset against the exact offset, both ways over samples
        double worst = 0.0;
        double t0 = 0.0;
        double t1 = 0.0;
        REQUIRE(shape.spline.getParameterDomain(t0, t1));
        for (int k = 0; k <= 400 && !collapsed; ++k) {
            const RS_Vector q = directOffset(shape.spline, t0 + (t1 - t0) * k / 400.0, shape.distance);
            double nearest = RS_MAXDOUBLE;
            for (const Arc& arc : offsets) {
                nearest = std::min(nearest, distanceToArc(q, arc));
            }
            worst = std::max(worst, nearest);
        }
        WARN(shape.name << ": direct " << direct.entities.size() << " cubic pieces in " << directMs << " ms; biarc "
                        << offsets.size() << " arcs in " << biarcMs << " ms, worst " << worst << " of tolerance "
                        << tolerance << (collapsed ? ", an arc collapsed" : ""));
        CHECK_FALSE(collapsed);
        CHECK(worst <= tolerance);
    }
}
