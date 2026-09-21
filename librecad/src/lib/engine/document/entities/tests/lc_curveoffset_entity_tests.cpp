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

// Spline offsets through the entity APIs: RS_Entity::createOffset(),
// LC_SplinePoints::offset() and offsetTwoSides(), and RS_Creation's parallels.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <QList>

#include "lc_curveoffset.h"
#include "lc_parabola.h"
#include "lc_splinepoints.h"
#include "rs_creation.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_spline.h"

using Catch::Approx;

namespace {

RS_Spline sCurve() {
    RS_SplineData d(3, false);
    d.controlPoints = {{0, 0}, {4, 6}, {8, -6}, {12, 0}};
    d.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    d.weights.assign(4, 1.0);
    return RS_Spline(nullptr, d);
}

LC_SplinePoints fitSpline(const std::vector<RS_Vector>& points, const bool closed = false) {
    LC_SplinePointsData d(closed, false);
    d.splinePoints = points;
    return LC_SplinePoints(nullptr, d);
}

using Curve = std::function<RS_Vector(double)>;

/** Distance from p to a curve sampled densely on [t0, t1]. */
double distanceToCurve(const RS_Vector& p, const Curve& curve, const double t0, const double t1) {
    double best = RS_MAXDOUBLE;
    for (int i = 0; i <= 8000; ++i) {
        best = std::min(best, p.distanceTo(curve(t0 + (t1 - t0) * i / 8000.0)));
    }
    return best;
}

/** The largest error |distance to the source - d| over the pieces' points. */
double worstDistanceError(const std::vector<RS_Entity*>& pieces, const Curve& source, const double t0,
                          const double t1, const double d) {
    double worst = 0.0;
    for (const RS_Entity* piece : pieces) {
        const auto* spline = dynamic_cast<const RS_Spline*>(piece);
        REQUIRE(spline != nullptr);
        for (int k = 0; k <= 8; ++k) {
            LC_CurveJet jet;
            REQUIRE(spline->tryEvaluateJet(k / 8.0, LC_CurveEvaluationSide::Interior, jet));
            worst = std::max(worst, std::abs(distanceToCurve(jet.point, source, t0, t1) - d));
        }
    }
    return worst;
}

Curve curveOf(const RS_Spline& s) {
    return [&s](double t) { return s.getPointAt(t); };
}

Curve curveOf(const LC_SplinePoints& s) {
    return [&s](double t) {
        LC_CurveJet jet;
        s.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, jet);
        return jet.point;
    };
}

struct Owned {
    std::vector<RS_Entity*> entities;
    ~Owned() {
        for (RS_Entity* e : entities) {
            delete e;
        }
    }
};

} // namespace

TEST_CASE("RS_Spline::createOffset returns the offset as cubic pieces", "[curve-offset][entity]") {
    RS_Spline source = sCurve();
    const RS_Pen pen{RS_Color{10, 120, 200}, RS2::Width04, RS2::DotLine};
    source.setPen(pen);
    const std::vector<RS_Vector> controlsBefore = source.getData().controlPoints;

    Owned offset;
    offset.entities = source.createOffset(RS_Vector{6.0, 9.0}, 0.75);
    REQUIRE(offset.entities.size() > 1);
    CHECK(worstDistanceError(offset.entities, curveOf(source), 0.0, 1.0, 0.75) < 1e-3);
    for (const RS_Entity* piece : offset.entities) {
        CHECK(piece->getPen(false) == pen);
        CHECK(piece->getParent() == nullptr);
        CHECK_FALSE(piece->isSelected());
    }
    CHECK(source.getData().controlPoints == controlsBefore); // the source is untouched

    // a negative distance is a magnitude: the side comes from the point
    Owned negative;
    negative.entities = source.createOffset(RS_Vector{6.0, 9.0}, -0.75);
    REQUIRE(negative.entities.size() == offset.entities.size());
    CHECK(negative.entities.front()->getStartpoint() == offset.entities.front()->getStartpoint());
}

TEST_CASE("createOffset returns nothing when the offset fails", "[curve-offset][entity]") {
    const RS_Spline source = sCurve();
    // a source whose tangent vanishes has no normal there
    RS_SplineData cuspData(3, false);
    cuspData.controlPoints = {{0, 0}, {6, 3}, {0, 3}, {6, 0}};
    cuspData.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    cuspData.weights.assign(4, 1.0);
    CHECK(RS_Spline(nullptr, cuspData).createOffset(RS_Vector{3.0, 5.0}, 0.5).empty());
    // on the curve: no side
    CHECK(source.createOffset(source.getPointAt(0.3), 1.0).empty());
    CHECK(source.createOffset(RS_Vector{6.0, 9.0}, 0.0).empty());
}

TEST_CASE("LC_SplinePoints::createOffset returns the offset as cubic pieces", "[curve-offset][entity]") {
    const LC_SplinePoints source = fitSpline({{0, 0}, {3, 4}, {7, 3}, {10, 6}, {14, 2}});
    Owned offset;
    offset.entities = source.createOffset(RS_Vector{7.0, -5.0}, 0.5);
    REQUIRE_FALSE(offset.entities.empty());
    CHECK(worstDistanceError(offset.entities, curveOf(source), 0.0,
                             static_cast<double>(source.getSegmentCount()), 0.5) < 1e-3);
}

TEST_CASE("A parabola offsets through the engine, not to a coincident copy", "[curve-offset][entity]") {
    // LC_Parabola inherits LC_SplinePoints::createOffset(). Its legacy offset()
    // moved only the base spline data, which its update() then rebuilt from the
    // unchanged parabola: Draw > Parallel produced a copy on top of it.
    const LC_Parabola parabola{nullptr, LC_ParabolaData{std::array<RS_Vector, 3>{
                                            RS_Vector{-4.0, 4.0}, RS_Vector{0.0, -4.0}, RS_Vector{4.0, 4.0}}}};
    QList<RS_Entity*> created;
    RS_Creation::createParallel(RS_Vector{0.0, -5.0}, 1.0, 1, const_cast<LC_Parabola*>(&parabola), false, created);
    Owned owned;
    owned.entities.assign(created.begin(), created.end());
    REQUIRE_FALSE(owned.entities.empty());
    CHECK(worstDistanceError(owned.entities, curveOf(static_cast<const LC_SplinePoints&>(parabola)), 0.0, 1.0,
                             1.0) < 1e-3);
}

TEST_CASE("RS_Creation adds no failed copy of a spline", "[curve-offset][entity]") {
    const LC_SplinePoints source = fitSpline({{0, 0}, {3, 4}, {7, 3}, {10, 6}, {14, 2}});
    QList<RS_Entity*> created;
    RS_Creation::createParallel(RS_Vector{7.0, -5.0}, 0.25, 3, const_cast<LC_SplinePoints*>(&source), false,
                                created);
    Owned owned;
    owned.entities.assign(created.begin(), created.end());
    // three copies, each several pieces, each at its own distance
    REQUIRE(owned.entities.size() >= 3);
    for (const RS_Entity* e : owned.entities) {
        CHECK(e->rtti() == RS2::EntitySpline);
    }

    // a copy that fails ends the series: nothing unchanged is appended
    QList<RS_Entity*> failing;
    const RS_Spline s = sCurve();
    RS_Creation::createParallel(RS_Vector{6.0, 9.0}, 3.0, 4, const_cast<RS_Spline*>(&s), false, failing);
    Owned ownedFailing;
    ownedFailing.entities.assign(failing.begin(), failing.end());
    for (const RS_Entity* e : ownedFailing.entities) {
        CHECK(e != &s);
    }
}

TEST_CASE("LC_SplinePoints::offset leaves a spline it cannot offset unchanged", "[curve-offset][entity]") {
    // one fit point: no tangent anywhere
    LC_SplinePoints single = fitSpline({{2.0, 3.0}});
    const LC_SplinePointsData before = single.getData();
    CHECK_FALSE(single.offset(RS_Vector{5.0, 5.0}, 1.0));
    CHECK(single.getData().splinePoints == before.splinePoints);
    CHECK(single.getData().controlPoints == before.controlPoints);

    // an ordinary spline still offsets in place, keeping its type
    LC_SplinePoints ordinary = fitSpline({{0, 0}, {3, 4}, {7, 3}, {10, 6}, {14, 2}});
    REQUIRE(ordinary.offset(RS_Vector{7.0, -5.0}, 0.5));
    CHECK(ordinary.rtti() == RS2::EntitySplinePoints);
    CHECK(ordinary.getData().splinePoints.front().distanceTo(RS_Vector{0, 0}) == Approx(0.5).margin(1e-6));
}

TEST_CASE("LC_SplinePoints::offsetTwoSides returns both sides or nothing", "[curve-offset][entity]") {
    const LC_SplinePoints source = fitSpline({{0, 0}, {3, 4}, {7, 3}, {10, 6}, {14, 2}});
    Owned both;
    both.entities = source.offsetTwoSides(0.4);
    REQUIRE(both.entities.size() >= 2);
    CHECK(worstDistanceError(both.entities, curveOf(source), 0.0, static_cast<double>(source.getSegmentCount()),
                             0.4) < 1e-3);
    // At the apex radius of a parabola its inner side is singular: neither side
    // is returned, although the outer one exists.
    LC_SplinePointsData arch(false, false);
    arch.useControlPoints = true;
    arch.controlPoints = {{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}};
    const LC_SplinePoints parabola(nullptr, arch);
    REQUIRE_FALSE(parabola.createOffset(RS_Vector{5.0, 20.0}, 2.5).empty());
    CHECK(parabola.offsetTwoSides(2.5).empty());
}

TEST_CASE("A fillet refuses a parallel made of several pieces", "[curve-offset][entity]") {
    // RS_Modification::round() builds one parallel per entity and intersects
    // them; a multi-piece spline offset is not one curve, so it is refused
    // rather than filleted against its first piece.
    LC_SplinePoints spline = fitSpline({{0, 0}, {3, 4}, {7, 3}, {10, 6}, {14, 2}});
    RS_Line line{nullptr, RS_LineData{{14.0, 2.0}, {20.0, -4.0}}};
    RS_RoundData data;
    data.radius = 0.5;
    data.trim = false;
    LC_DocumentModificationBatch ctx;
    const LC_RoundResult result = RS_Modification::round(RS_Vector{13.0, 0.0}, RS_Vector{12.0, 4.0}, &spline,
                                                         RS_Vector{16.0, 0.0}, &line, data, ctx);
    CHECK(result.error == LC_RoundResult::NO_PARALLELS);
    qDeleteAll(ctx.entitiesToAdd);
}
