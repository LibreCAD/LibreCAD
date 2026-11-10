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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <vector>

#include "lc_curveoffset.h"
#include "lc_hyperbola.h"
#include "lc_offsetresultcheck.h"
#include "lc_offsetsource.h"
#include "lc_parabola.h"
#include "lc_splinepoints.h"
#include "drw_base.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_spline.h"

using Catch::Approx;

TEST_CASE("Offset source snapshots own every supported source type") {
    RS_EllipseData ellipseData;
    ellipseData.center = {1.0, 2.0};
    ellipseData.majorP = {4.0, 0.0};
    ellipseData.ratio = .5;
    RS_Ellipse ellipse{nullptr, ellipseData};

    RS_SplineData splineData{2, false};
    splineData.controlPoints = {{0, 0}, {1, 2}, {3, 0}};
    splineData.knotslist = {0, 0, 0, 1, 1, 1};
    splineData.weights = {1, .7, 1};
    RS_Spline spline{nullptr, splineData};

    LC_SplinePointsData pointsData{false, false};
    pointsData.useControlPoints = true;
    pointsData.controlPoints = {{0, 0}, {1, 1}, {2, 0}};
    LC_SplinePoints points{nullptr, pointsData};

    LC_Parabola parabola{nullptr, LC_ParabolaData{{RS_Vector{0, 1}, RS_Vector{1, 0}, RS_Vector{2, 1}}}};
    LC_Hyperbola hyperbola{nullptr, LC_HyperbolaData{{0, 0}, {2, 0}, .5, -.5, .5}};
    LC_Hyperbola unbounded{nullptr, LC_HyperbolaData{{0, 0}, {2, 0}, .5}};
    RS_Line line{nullptr, RS_LineData{{0, 0}, {1, 0}}};
    RS_Arc arc{nullptr, RS_ArcData{{2, 3}, 4.0, .25, 1.5, true}};
    RS_Circle circle{nullptr, RS_CircleData{{-2, 3}, 5.0}};
    RS_Polyline polyline;
    polyline.addVertex({0, 0}, 1.0); polyline.addVertex({2, 0}); polyline.addVertex({2, 2});

    const std::array<const RS_Entity*, 9> entities{&ellipse, &spline, &points, &parabola, &hyperbola,
                                                    &line, &arc, &circle, &polyline};
    const std::array<LC_OffsetSourceGeometry::Kind, 9> kinds{
        LC_OffsetSourceGeometry::Kind::Ellipse,
        LC_OffsetSourceGeometry::Kind::Spline,
        LC_OffsetSourceGeometry::Kind::SplinePoints,
        LC_OffsetSourceGeometry::Kind::Parabola,
        LC_OffsetSourceGeometry::Kind::BoundedHyperbola,
        LC_OffsetSourceGeometry::Kind::Line,
        LC_OffsetSourceGeometry::Kind::Arc,
        LC_OffsetSourceGeometry::Kind::Circle,
        LC_OffsetSourceGeometry::Kind::Polyline};
    for (std::size_t i = 0; i < entities.size(); ++i) {
        auto snapshot = LC_OffsetSourceGeometry::capture(*entities[i]);
        REQUIRE(snapshot);
        CHECK(snapshot->kind() == kinds[i]);
        CHECK(snapshot->isWellFormed());
        auto restored = snapshot->makeEntity();
        REQUIRE(restored);
        CHECK(restored->rtti() == entities[i]->rtti());
        auto recaptured = LC_OffsetSourceGeometry::capture(*restored);
        REQUIRE(recaptured);
        CHECK(recaptured->kind() == kinds[i]);
        switch (kinds[i]) {
        case LC_OffsetSourceGeometry::Kind::Ellipse: {
            const auto* value = dynamic_cast<const RS_Ellipse*>(restored.get());
            REQUIRE(value);
            CHECK(value->getData().center == ellipseData.center);
            CHECK(value->getData().majorP == ellipseData.majorP);
            CHECK(value->getData().ratio == ellipseData.ratio);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::Spline: {
            const auto* value = dynamic_cast<const RS_Spline*>(restored.get());
            REQUIRE(value);
            CHECK(value->getData().controlPoints == splineData.controlPoints);
            CHECK(value->getData().knotslist == splineData.knotslist);
            CHECK(value->getData().weights == splineData.weights);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::SplinePoints: {
            const auto* value = dynamic_cast<const LC_SplinePoints*>(restored.get());
            REQUIRE(value);
            CHECK(value->getData().controlPoints == pointsData.controlPoints);
            CHECK(value->getData().useControlPoints == pointsData.useControlPoints);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::Parabola: {
            const auto* value = dynamic_cast<const LC_Parabola*>(restored.get());
            REQUIRE(value);
            CHECK(value->getData().m_controlPoints[0] == parabola.getData().m_controlPoints[0]);
            CHECK(value->getData().m_controlPoints[1] == parabola.getData().m_controlPoints[1]);
            CHECK(value->getData().m_controlPoints[2] == parabola.getData().m_controlPoints[2]);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::BoundedHyperbola: {
            const auto* value = dynamic_cast<const LC_Hyperbola*>(restored.get());
            REQUIRE(value);
            CHECK(value->getData().center == hyperbola.getData().center);
            CHECK(value->getData().majorP == hyperbola.getData().majorP);
            CHECK(value->getData().angle1 == hyperbola.getData().angle1);
            CHECK(value->getData().angle2 == hyperbola.getData().angle2);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::Line: {
            const auto* value = dynamic_cast<const RS_Line*>(restored.get());
            REQUIRE(value);
            CHECK(value->getStartpoint() == line.getStartpoint());
            CHECK(value->getEndpoint() == line.getEndpoint());
            break;
        }
        case LC_OffsetSourceGeometry::Kind::Arc: {
            const auto* value = dynamic_cast<const RS_Arc*>(restored.get());
            REQUIRE(value);
            CHECK(value->getData().center == arc.getData().center);
            CHECK(value->getData().radius == arc.getData().radius);
            CHECK(value->getData().angle1 == arc.getData().angle1);
            CHECK(value->getData().angle2 == arc.getData().angle2);
            CHECK(value->getData().reversed == arc.getData().reversed);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::Circle: {
            const auto* value = dynamic_cast<const RS_Circle*>(restored.get());
            REQUIRE(value);
            CHECK(value->getData().center == circle.getData().center);
            CHECK(value->getData().radius == circle.getData().radius);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::Polyline: {
            const auto* value = dynamic_cast<const RS_Polyline*>(restored.get());
            REQUIRE(value);
            CHECK(value->isClosed() == polyline.isClosed());
            CHECK(value->getStartpoint() == polyline.getStartpoint());
            CHECK(value->getEndpoint() == polyline.getEndpoint());
            CHECK(value->size() == polyline.size());
            REQUIRE(value->size() == 2);
            CHECK(value->entityAt(0)->rtti() == RS2::EntityArc);
            CHECK(value->entityAt(1)->rtti() == RS2::EntityLine);
            break;
        }
        case LC_OffsetSourceGeometry::Kind::Invalid:
            FAIL("valid source snapshot has invalid kind");
        }
    }

    auto capturedParabola = LC_OffsetSourceGeometry::capture(parabola);
    REQUIRE(capturedParabola);
    CHECK(std::holds_alternative<LC_ParabolaData>(capturedParabola->value));
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(unbounded));
    CHECK(LC_OffsetSourceGeometry::capture(line));
    RS_Line degenerateLine{nullptr, RS_LineData{{1, 1}, {1, 1}}};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(degenerateLine));
    auto nonPlanarEllipseData = ellipseData;
    nonPlanarEllipseData.center.z = 1.0;
    RS_Ellipse nonPlanarEllipse{nullptr, nonPlanarEllipseData};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(nonPlanarEllipse));
    auto degenerateEllipseData = ellipseData;
    degenerateEllipseData.majorP = {0, 0};
    RS_Ellipse degenerateEllipse{nullptr, degenerateEllipseData};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(degenerateEllipse));

    LC_OffsetPolylineData disconnected{{0, 0}, {2, 0}, false,
        {RS_LineData{{0, 0}, {1, 0}}, RS_LineData{{1.1, 0}, {2, 0}}}};
    CHECK_FALSE(LC_OffsetSourceGeometry{disconnected}.isWellFormed());
    LC_OffsetPolylineData inconsistentHeader{{0, 0}, {3, 0}, false,
        {RS_LineData{{0, 0}, {2, 0}}}};
    CHECK_FALSE(LC_OffsetSourceGeometry{inconsistentHeader}.isWellFormed());
    LC_OffsetPolylineData closedChain{{0, 0}, {1, 1}, true,
        {RS_LineData{{0, 0}, {2, 0}}, RS_LineData{{2, 0}, {1, 1}},
         RS_LineData{{1, 1}, {0, 0}}}};
    CHECK(LC_OffsetSourceGeometry{closedChain}.isWellFormed());

    auto ownedSpline = LC_OffsetSourceGeometry::capture(spline);
    REQUIRE(ownedSpline);
    spline.getData().controlPoints[0] = {99, 99};
    CHECK(std::get<RS_SplineData>(ownedSpline->value).controlPoints[0] == RS_Vector{0, 0});

    RS_SplineData malformedSplineData = splineData;
    malformedSplineData.knotslist.pop_back();
    RS_Spline malformedSpline{nullptr, malformedSplineData};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(malformedSpline));

    LC_SplinePointsData degeneratePointsData{false, false};
    degeneratePointsData.useControlPoints = true;
    degeneratePointsData.controlPoints = {{0, 0}};
    LC_SplinePoints degeneratePoints{nullptr, degeneratePointsData};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(degeneratePoints));

    LC_OffsetSourceDefinition definition{*ownedSpline, 2.0, 1e-4, false};
    CHECK(definition.isWellFormed());
    definition.signedDistance = std::numeric_limits<double>::infinity();
    CHECK_FALSE(definition.isWellFormed());
}

TEST_CASE("A zero-length spline-points source refuses offsets without mutation", "[curve-offset][direct][spline-points][degenerate]") {
    const std::vector<RS_Vector> controls{{0.0, 0.0}, {0.0, 0.0}};
    LC_SplinePointsData data{false, false};
    data.useControlPoints = true;
    data.controlPoints = controls;
    LC_SplinePoints source{nullptr, data};
    const auto snapshot = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(snapshot);
    CHECK(snapshot->kind() == LC_OffsetSourceGeometry::Kind::SplinePoints);
    CHECK(snapshot->isWellFormed());

    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
    const auto result = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25),
                                                            options, LC_CurveOffset::makeDirectSourceBudget());
    CHECK(result.status == LC_CurveOffsetStatus::UndefinedTangent);
    CHECK(result.branches.empty());
    CHECK(source.getData().controlPoints == controls);
    const auto after = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(after);
    CHECK(after->kind() == snapshot->kind());
    CHECK(std::get<LC_SplinePointsData>(after->value).controlPoints == controls);
}

TEST_CASE("Stationary spline-points spans are removed before offsetting moving spans",
          "[curve-offset][direct][spline-points][degenerate]") {
    const std::vector<RS_Vector> controls{{0.0, 0.0}, {1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {2.0, 0.0}};
    LC_SplinePointsData data{false, false};
    data.useControlPoints = true;
    data.controlPoints = controls;
    LC_SplinePoints source{nullptr, data};
    REQUIRE(source.getSegmentCount() == 3);
    LC_CurveJet stationaryJet;
    REQUIRE(source.tryEvaluateJet(1.5, LC_CurveEvaluationSide::Interior, stationaryJet));
    CHECK(stationaryJet.first == RS_Vector(0.0, 0.0, 0.0));

    const auto snapshot = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(snapshot);
    CHECK(snapshot->kind() == LC_OffsetSourceGeometry::Kind::SplinePoints);
    CHECK(snapshot->isWellFormed());

    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
    const auto result = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25),
                                                            options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(result.branches.empty());
    const auto cubicAt = [](const std::array<RS_Vector, 4>& bezier, const double t) {
        const double u = 1.0 - t;
        return bezier[0] * (u * u * u) + bezier[1] * (3.0 * u * u * t) + bezier[2] * (3.0 * u * t * t) + bezier[3] * (t * t * t);
    };
    size_t checkedSamples = 0;
    for (const LC_OffsetBranch& branch : result.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            if (piece.provenance.arcCentre.valid) {
                continue;
            }
            for (const int sample : {3, 9, 15, 21, 27, 33}) {
                const double fraction = sample / 37.0;
                const double parameter = piece.provenance.sourceT0 + fraction * (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                CHECK(((parameter < 1.0) || (parameter > 2.0)));
                LC_CurveJet jet;
                REQUIRE(source.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, jet));
                const double speed = jet.first.magnitude();
                REQUIRE(speed > 0.0);
                const RS_Vector exact = jet.point + RS_Vector{-jet.first.y, jet.first.x} * (0.25 / speed);
                CHECK(cubicAt(piece.bezier, fraction).distanceTo(exact) <= options.tolerance.requestedGeometry);
                ++checkedSamples;
            }
        }
    }
    CHECK(checkedSamples > 0);
    CHECK(source.getData().controlPoints == controls);
    const auto after = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(after);
    CHECK(after->kind() == snapshot->kind());
    CHECK(std::get<LC_SplinePointsData>(after->value).controlPoints == controls);
}

TEST_CASE("Multiple stationary spline-points spans are omitted from offset provenance",
          "[curve-offset][direct][spline-points][degenerate]") {
    const std::vector<RS_Vector> controls{{0.0, 0.0}, {1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {2.0, 0.0},
                                         {3.0, 1.0}, {3.0, 1.0}, {3.0, 1.0}, {4.0, 0.0}};
    LC_SplinePointsData data{false, false};
    data.useControlPoints = true;
    data.controlPoints = controls;
    LC_SplinePoints source{nullptr, data};
    REQUIRE(source.getSegmentCount() == 7);
    for (const double parameter : {1.5, 5.5}) {
        LC_CurveJet stationaryJet;
        REQUIRE(source.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, stationaryJet));
        CHECK(stationaryJet.first == RS_Vector(0.0, 0.0, 0.0));
    }

    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
    const auto result = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(result.branches.empty());

    const auto cubicAt = [](const std::array<RS_Vector, 4>& bezier, const double t) {
        const double u = 1.0 - t;
        return bezier[0] * (u * u * u) + bezier[1] * (3.0 * u * u * t) + bezier[2] * (3.0 * u * t * t) +
               bezier[3] * (t * t * t);
    };
    size_t checkedSamples = 0;
    for (const LC_OffsetBranch& branch : result.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            if (piece.provenance.arcCentre.valid) {
                continue;
            }
            for (const int sample : {3, 9, 15, 21, 27, 33}) {
                const double fraction = sample / 37.0;
                const double parameter = piece.provenance.sourceT0 + fraction * (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                CHECK(((parameter < 1.0) || (parameter > 2.0 && parameter < 5.0) || (parameter > 6.0)));
                LC_CurveJet jet;
                REQUIRE(source.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, jet));
                const double speed = jet.first.magnitude();
                REQUIRE(speed > 0.0);
                const RS_Vector exact = jet.point + RS_Vector{-jet.first.y, jet.first.x} * (0.25 / speed);
                CHECK(cubicAt(piece.bezier, fraction).distanceTo(exact) <= options.tolerance.requestedGeometry);
                ++checkedSamples;
            }
        }
    }
    CHECK(checkedSamples > 0);
    CHECK(source.getData().controlPoints == controls);
}

TEST_CASE("Stationary spline-points plateaus and separated spans are omitted",
          "[curve-offset][direct][spline-points][degenerate]") {
    const std::vector<RS_Vector> controls{{0.0, 0.0}, {1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0},
                                         {1.0, 1.0}, {2.0, 0.0}, {3.0, 1.0}, {3.0, 1.0}, {3.0, 1.0},
                                         {4.0, 0.0}};
    LC_SplinePointsData data{false, false};
    data.useControlPoints = true;
    data.controlPoints = controls;
    LC_SplinePoints source{nullptr, data};
    REQUIRE(source.getSegmentCount() == 9);
    for (const double parameter : {1.5, 2.5, 3.5, 7.5}) {
        LC_CurveJet stationaryJet;
        REQUIRE(source.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, stationaryJet));
        CHECK(stationaryJet.first == RS_Vector(0.0, 0.0, 0.0));
    }

    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
    const auto result = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(result.branches.empty());
    const auto cubicAt = [](const std::array<RS_Vector, 4>& bezier, const double t) {
        const double u = 1.0 - t;
        return bezier[0] * (u * u * u) + bezier[1] * (3.0 * u * u * t) + bezier[2] * (3.0 * u * t * t) +
               bezier[3] * (t * t * t);
    };
    size_t checkedSamples = 0;
    for (const LC_OffsetBranch& branch : result.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            if (piece.provenance.arcCentre.valid) {
                continue;
            }
            for (const int sample : {3, 9, 15, 21, 27, 33}) {
                const double fraction = sample / 37.0;
                const double parameter = piece.provenance.sourceT0 + fraction * (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                CHECK(((parameter < 1.0) || (parameter > 4.0 && parameter < 7.0) || (parameter > 8.0)));
                LC_CurveJet jet;
                REQUIRE(source.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, jet));
                const double speed = jet.first.magnitude();
                REQUIRE(speed > 0.0);
                const RS_Vector exact = jet.point + RS_Vector{-jet.first.y, jet.first.x} * (0.25 / speed);
                CHECK(cubicAt(piece.bezier, fraction).distanceTo(exact) <= options.tolerance.requestedGeometry);
                ++checkedSamples;
            }
        }
    }
    CHECK(checkedSamples > 0);
    CHECK(source.getData().controlPoints == controls);
}

TEST_CASE("Endpoint stationary spline-points spans are omitted before offsetting",
          "[curve-offset][direct][spline-points][degenerate]") {
    const std::array<std::vector<RS_Vector>, 2> profiles{{
        {{0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 1.0}, {2.0, 0.0}, {3.0, 1.0}},
        {{0.0, 0.0}, {1.0, 1.0}, {2.0, 0.0}, {3.0, 1.0}, {3.0, 1.0}, {3.0, 1.0}},
    }};
    const std::array<std::pair<double, double>, 2> stationaryIntervals{{{0.0, 1.0}, {3.0, 4.0}}};
    const auto cubicAt = [](const std::array<RS_Vector, 4>& bezier, const double t) {
        const double u = 1.0 - t;
        return bezier[0] * (u * u * u) + bezier[1] * (3.0 * u * u * t) + bezier[2] * (3.0 * u * t * t) +
               bezier[3] * (t * t * t);
    };

    for (std::size_t i = 0; i < profiles.size(); ++i) {
        LC_SplinePointsData data{false, false};
        data.useControlPoints = true;
        data.controlPoints = profiles[i];
        LC_SplinePoints source{nullptr, data};
        REQUIRE(source.getSegmentCount() == 4);
        LC_CurveJet stationaryJet;
        const auto [stationaryStart, stationaryEnd] = stationaryIntervals[i];
        REQUIRE(source.tryEvaluateJet((stationaryStart + stationaryEnd) / 2.0, LC_CurveEvaluationSide::Interior, stationaryJet));
        CHECK(stationaryJet.first == RS_Vector(0.0, 0.0, 0.0));

        const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
        const auto result = LC_CurveOffset::buildDirectBranches(
            source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25), options,
            LC_CurveOffset::makeDirectSourceBudget());
        INFO("profile " << i);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE_FALSE(result.branches.empty());
        size_t checkedSamples = 0;
        for (const LC_OffsetBranch& branch : result.branches) {
            for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                if (piece.provenance.arcCentre.valid) {
                    continue;
                }
                for (const int sample : {3, 9, 15, 21, 27, 33}) {
                    const double fraction = sample / 37.0;
                    const double parameter = piece.provenance.sourceT0 + fraction * (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                    CHECK(((parameter < stationaryStart) || (parameter > stationaryEnd)));
                    LC_CurveJet jet;
                    REQUIRE(source.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, jet));
                    const double speed = jet.first.magnitude();
                    REQUIRE(speed > 0.0);
                    const RS_Vector exact = jet.point + RS_Vector{-jet.first.y, jet.first.x} * (0.25 / speed);
                    CHECK(cubicAt(piece.bezier, fraction).distanceTo(exact) <= options.tolerance.requestedGeometry);
                    ++checkedSamples;
                }
            }
        }
        CHECK(checkedSamples > 0);
        CHECK(source.getData().controlPoints == profiles[i]);
    }
}

TEST_CASE("Stationary spline-points profiles preserve exact normals on both offset sides",
          "[curve-offset][direct][spline-points][oracle]") {
    struct Profile {
        std::vector<RS_Vector> controls;
        std::vector<std::pair<double, double>> stationary;
    };
    const std::array<Profile, 5> profiles{{
        {{{0, 0}, {1, 1}, {1, 1}, {1, 1}, {2, 0}}, {{1.0, 2.0}}},
        {{{0, 0}, {1, 1}, {1, 1}, {1, 1}, {2, 0}, {3, 1}, {3, 1}, {3, 1}, {4, 0}}, {{1.0, 2.0}, {5.0, 6.0}}},
        {{{0, 0}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {2, 0}, {3, 1}, {3, 1}, {3, 1}, {4, 0}},
         {{1.0, 4.0}, {7.0, 8.0}}},
        {{{0, 0}, {0, 0}, {0, 0}, {1, 1}, {2, 0}, {3, 1}}, {{0.0, 1.0}}},
        {{{0, 0}, {1, 1}, {2, 0}, {3, 1}, {3, 1}, {3, 1}}, {{3.0, 4.0}}},
    }};
    const auto cubicAt = [](const std::array<RS_Vector, 4>& bezier, const double t) {
        const double u = 1.0 - t;
        return bezier[0] * (u * u * u) + bezier[1] * (3.0 * u * u * t) + bezier[2] * (3.0 * u * t * t) +
               bezier[3] * (t * t * t);
    };

    for (std::size_t i = 0; i < profiles.size(); ++i) {
        LC_SplinePointsData data{false, false};
        data.useControlPoints = true;
        data.controlPoints = profiles[i].controls;
        LC_SplinePoints source{nullptr, data};
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            CAPTURE(i, side);
            const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
            const auto result = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(side, 0.25), options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            REQUIRE_FALSE(result.branches.empty());
            const double signedDistance = side == LC_CurveOffsetSide::Left ? 0.25 : -0.25;
            size_t checkedSamples = 0;
            for (const LC_OffsetBranch& branch : result.branches) {
                for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
                    if (piece.provenance.arcCentre.valid) {
                        continue;
                    }
                    for (const int sample : {3, 9, 15, 21, 27, 33}) {
                        const double fraction = sample / 37.0;
                        const double parameter = piece.provenance.sourceT0 +
                                                 fraction * (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                        for (const auto& interval : profiles[i].stationary) {
                            CHECK(((parameter < interval.first) || (parameter > interval.second)));
                        }
                        LC_CurveJet jet;
                        REQUIRE(source.tryEvaluateJet(parameter, LC_CurveEvaluationSide::Interior, jet));
                        const double speed = jet.first.magnitude();
                        REQUIRE(speed > 0.0);
                        const RS_Vector exact = jet.point + RS_Vector{-jet.first.y, jet.first.x} * (signedDistance / speed);
                        CHECK(cubicAt(piece.bezier, fraction).distanceTo(exact) <= options.tolerance.requestedGeometry);
                        ++checkedSamples;
                    }
                }
            }
            CHECK(checkedSamples > 0);
        }
        CHECK(source.getData().controlPoints == profiles[i].controls);
    }
}

TEST_CASE("Large translated parabola remains an eligible typed source", "[lc-offsetcurve][source][parabola][precision]") {
    const double base = 1e308;
    const double step = std::nextafter(base, std::numeric_limits<double>::infinity()) - base;
    const LC_Parabola parabola{
        nullptr, LC_ParabolaData{{RS_Vector{base, base}, RS_Vector{base + 2.0 * step, base + step}, RS_Vector{base + 4.0 * step, base}}}};

    REQUIRE(parabola.getData().isValid());
    CHECK(std::isfinite(parabola.getData().m_vertex.x));
    CHECK(std::isfinite(parabola.getData().m_focus.x));
    const auto& splinePoints = static_cast<const LC_SplinePoints&>(parabola).getData().splinePoints;
    REQUIRE(splinePoints.size() == 3);
    CHECK(LC_OffsetSourceDetail::finite(splinePoints[1]));
    const auto source = LC_OffsetSourceGeometry::capture(parabola);
    REQUIRE(source);
    CHECK(source->kind() == LC_OffsetSourceGeometry::Kind::Parabola);
    auto restoredEntity = source->makeEntity();
    REQUIRE(restoredEntity);
    const auto* restored = dynamic_cast<const LC_Parabola*>(restoredEntity.get());
    REQUIRE(restored);
    const auto& restoredSplinePoints =
        static_cast<const LC_SplinePoints&>(*restored).getData().splinePoints;
    REQUIRE(restoredSplinePoints.size() == 3);
    CHECK(LC_OffsetSourceDetail::finite(restoredSplinePoints[1]));
}

TEST_CASE("Offset source capture rejects lossy polyline preview projections",
          "[lc-offsetcurve][source][polyline][provenance]") {
    const auto makePolyline = [] {
        RS_Polyline polyline;
        polyline.addVertex({0.0, 0.0}, 0.0);
        polyline.addVertex({2.0, 0.0}, 0.0);
        return polyline;
    };

    auto ordinary = makePolyline();
    CHECK(LC_OffsetSourceGeometry::capture(ordinary));

    auto nativeLwpolyline = makePolyline();
    nativeLwpolyline.setDrwExtData({
        std::make_shared<DRW_Variant>(1001, "LibreCAD_LWPOLYLINE"),
        std::make_shared<DRW_Variant>(1040, 0.0)});
    CHECK(LC_OffsetSourceGeometry::capture(nativeLwpolyline));

    for (const char* marker : {"LibreCAD_POLYLINE_MESH",
                               "LibreCAD_POLYLINE_PFACE",
                               "LibreCAD_MLINE",
                               "LibreCAD_UNDERLAY",
                               "LibreCAD_POLYLINE_OCS",
                               "librecad_polyline_mesh"}) {
        CAPTURE(marker);
        auto projection = makePolyline();
        projection.setDrwExtData({
            std::make_shared<DRW_Variant>(1001, marker),
            std::make_shared<DRW_Variant>(1000, "unrelated payload")});
        CHECK_FALSE(LC_OffsetSourceGeometry::capture(projection));
    }

    auto unrelatedXData = makePolyline();
    unrelatedXData.setDrwExtData({
        std::make_shared<DRW_Variant>(1001, "CUSTOM_APP"),
        std::make_shared<DRW_Variant>(1000, "LibreCAD_POLYLINE_MESH")});
    CHECK(LC_OffsetSourceGeometry::capture(unrelatedXData));

    auto malformedAppId = makePolyline();
    malformedAppId.setDrwExtData({std::make_shared<DRW_Variant>(1001, 42)});
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(malformedAppId));

    auto nullXData = makePolyline();
    nullXData.setDrwExtData({nullptr});
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(nullXData));
}

TEST_CASE("Offset source capture rejects projected arbitrary-OCS conics",
          "[lc-offsetcurve][source][provenance][ocs]") {
    RS_Ellipse projected{nullptr,
        {RS_Vector{4.0, -2.0}, RS_Vector{3.0, 1.0}, 0.4, 0.0, 0.0, false}};
    CHECK(LC_OffsetSourceGeometry::capture(projected));

    projected.setDrwExtData({
        std::make_shared<DRW_Variant>(1001, "LibreCAD_CONIC_OCS"),
        std::make_shared<DRW_Variant>(1070, 0),
        std::make_shared<DRW_Variant>(1010, DRW_Coord{0.3, 0.4, 0.8660254037844386})});
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(projected));

    projected.setDrwExtData({
        std::make_shared<DRW_Variant>(1001, "CUSTOM_APP"),
        std::make_shared<DRW_Variant>(1000, "LibreCAD_CONIC_OCS")});
    CHECK(LC_OffsetSourceGeometry::capture(projected));
}

TEST_CASE("Polyline source topology validation accepts its maximum bounded segment count",
          "[lc-offsetcurve][source][polyline][bounded]") {
    constexpr std::size_t segmentCount = 65536;
    LC_OffsetPolylineData data;
    data.startpoint = {0, 0};
    data.endpoint = {static_cast<double>(segmentCount), 0};
    data.segments.reserve(segmentCount);
    for (std::size_t i = 0; i < segmentCount; ++i) {
        const double start = static_cast<double>(i);
        data.segments.emplace_back(RS_LineData{{start, 0}, {start + 1.0, 0}});
    }

    CHECK(LC_OffsetSourceGeometry{std::move(data)}.isWellFormed());
}

TEST_CASE("Polyline source topology validation rejects segment self-intersections",
          "[lc-offsetcurve][source][polyline][topology]") {
    const LC_OffsetPolylineData bowTie{{0, 0}, {2, 0}, false,
        {RS_LineData{{0, 0}, {2, 2}}, RS_LineData{{2, 2}, {0, 2}},
         RS_LineData{{0, 2}, {2, 0}}}};
    CHECK_FALSE(LC_OffsetSourceGeometry{bowTie}.isWellFormed());

    const LC_OffsetPolylineData arcCrossing{{1, 0}, {0, 0.5}, false,
        {RS_ArcData{{0, 0}, 1.0, 0.0, M_PI, false},
         RS_LineData{{-1, 0}, {-2, 0}}, RS_LineData{{-2, 0}, {0, 0.5}}}};
    CHECK_FALSE(LC_OffsetSourceGeometry{arcCrossing}.isWellFormed());

    const double intersectionY = std::sqrt(3.0) / 2.0;
    const LC_OffsetPolylineData arcArcCrossing{{1, 0}, {2, 0}, false,
        {RS_ArcData{{0, 0}, 1.0, 0.0, M_PI / 3.0, true},
         RS_ArcData{{1, 0}, 1.0, 2.0 * M_PI / 3.0, 0.0, false}}};
    CHECK(LC_OffsetSourceDetail::sameEndpoint(
        {0.5, intersectionY}, {1.0 + std::cos(2.0 * M_PI / 3.0),
                               std::sin(2.0 * M_PI / 3.0)}));
    CHECK_FALSE(LC_OffsetSourceGeometry{arcArcCrossing}.isWellFormed());

    const LC_OffsetPolylineData overlappingArcs{{1, 0}, {0.5, intersectionY}, false,
        {RS_ArcData{{0, 0}, 1.0, 0.0, 2.0 * M_PI / 3.0, false},
         RS_ArcData{{0, 0}, 1.0, 2.0 * M_PI / 3.0, M_PI / 3.0, false}}};
    CHECK_FALSE(LC_OffsetSourceGeometry{overlappingArcs}.isWellFormed());

    const LC_OffsetPolylineData touchingArcs{{1, 0}, {-1, 0}, false,
        {RS_ArcData{{0, 0}, 1.0, 0.0, 2.0 * M_PI / 3.0, false},
         RS_ArcData{{0, 0}, 1.0, 2.0 * M_PI / 3.0, M_PI, false}}};
    CHECK(LC_OffsetSourceGeometry{touchingArcs}.isWellFormed());

    const double nearTangentOffset = 1.0e-10;
    const double nearTangentY = 1.0 - nearTangentOffset;
    const double nearTangentX = std::sqrt(1.0 - nearTangentY * nearTangentY);
    const double nearTangentJoin = std::atan2(nearTangentY, nearTangentX);
    const double nearTangentStart = nearTangentJoin - 1.0e-6;
    const double nearTangentEnd = nearTangentJoin + 1.0e-3;
    constexpr double lineHalfLength = 1.0e-6;
    const LC_OffsetPolylineData nearTangentCrossing{
        {nearTangentX - lineHalfLength, nearTangentY},
        {std::cos(nearTangentEnd), std::sin(nearTangentEnd)}, false,
        {RS_LineData{{nearTangentX - lineHalfLength, nearTangentY},
                     {nearTangentX + lineHalfLength, nearTangentY}},
         RS_LineData{{nearTangentX + lineHalfLength, nearTangentY},
                     {std::cos(nearTangentStart), std::sin(nearTangentStart)}},
         RS_ArcData{{0, 0}, 1.0, nearTangentStart, nearTangentEnd, false}}};
    const RS_Line nearTangentLine{nullptr,
        std::get<RS_LineData>(nearTangentCrossing.segments[0])};
    const RS_Arc nearTangentArc{nullptr,
        std::get<RS_ArcData>(nearTangentCrossing.segments[2])};
    const RS_Vector nearTangentIntersection{nearTangentX, nearTangentY};
    CHECK(nearTangentLine.isPointOnEntity(nearTangentIntersection, 1.0e-12));
    CHECK(nearTangentArc.isPointOnEntity(nearTangentIntersection, 1.0e-12));
    CHECK_FALSE(LC_OffsetSourceGeometry{nearTangentCrossing}.isWellFormed());

    constexpr double shallowCrossing = 1.0e-10;
    const LC_OffsetPolylineData nearParallelCrossing{{0, 0}, {1, -shallowCrossing}, false,
        {RS_LineData{{0, 0}, {3, 0}},
         RS_LineData{{3, 0}, {2, shallowCrossing}},
         RS_LineData{{2, shallowCrossing}, {1, -shallowCrossing}}}};
    CHECK_FALSE(LC_OffsetSourceGeometry{nearParallelCrossing}.isWellFormed());

    const double nearConcentricShift = 1.0e-8;
    const double nearConcentricRadius = std::hypot(nearConcentricShift, 1.0);
    double nearConcentricJoinAngle = std::atan2(-1.0, -nearConcentricShift);
    if (nearConcentricJoinAngle < 0.0) nearConcentricJoinAngle += 2.0 * M_PI;
    const LC_OffsetPolylineData nearConcentricCrossing{{1, 0},
        {nearConcentricShift + nearConcentricRadius, 0}, false,
        {RS_ArcData{{0, 0}, 1.0, 0.0, 3.0 * M_PI / 2.0, false},
         RS_ArcData{{nearConcentricShift, 0}, nearConcentricRadius,
                    nearConcentricJoinAngle, 0.0, true}}};
    const RS_Arc nearConcentricFirstArc{nullptr,
        std::get<RS_ArcData>(nearConcentricCrossing.segments[0])};
    const RS_Arc nearConcentricSecondArc{nullptr,
        std::get<RS_ArcData>(nearConcentricCrossing.segments[1])};
    CHECK(nearConcentricFirstArc.isPointOnEntity({0, 1}, 1.0e-12));
    CHECK(nearConcentricSecondArc.isPointOnEntity({0, 1}, 1.0e-12));
    CHECK_FALSE(LC_OffsetSourceGeometry{nearConcentricCrossing}.isWellFormed());

    const LC_OffsetPolylineData simpleChain{{0, 0}, {3, 1}, false,
        {RS_LineData{{0, 0}, {1, 0}}, RS_LineData{{1, 0}, {2, 1}},
         RS_LineData{{2, 1}, {3, 1}}}};
    CHECK(LC_OffsetSourceGeometry{simpleChain}.isWellFormed());

    const LC_OffsetPolylineData overlappingLines{{0, 0}, {5, 0}, false,
        {RS_LineData{{0, 0}, {4, 0}}, RS_LineData{{4, 0}, {2, 0}},
         RS_LineData{{2, 0}, {5, 0}}}};
    CHECK_FALSE(LC_OffsetSourceGeometry{overlappingLines}.isWellFormed());
}

TEST_CASE("Offset polyline source snapshots preserve positive and negative bulge arcs",
          "[lc-offsetcurve][source][polyline]") {
    const double bulge = std::tan(M_PI / 16.0);
    RS_Polyline source;
    source.addVertex({0, 0}, bulge);
    source.addVertex({2, 0}, -bulge);
    source.addVertex({4, 0});
    source.addVertex({6, 0});

    REQUIRE(source.size() == 3);
    REQUIRE(source.entityAt(0)->rtti() == RS2::EntityArc);
    REQUIRE(source.entityAt(1)->rtti() == RS2::EntityArc);
    REQUIRE(source.entityAt(2)->rtti() == RS2::EntityLine);
    const auto* first = dynamic_cast<const RS_Arc*>(source.entityAt(0));
    const auto* second = dynamic_cast<const RS_Arc*>(source.entityAt(1));
    REQUIRE(first);
    REQUIRE(second);
    CHECK_FALSE(first->getData().reversed);
    CHECK(second->getData().reversed);

    const auto snapshot = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(snapshot);
    REQUIRE(snapshot->kind() == LC_OffsetSourceGeometry::Kind::Polyline);
    const auto& data = std::get<LC_OffsetPolylineData>(snapshot->value);
    REQUIRE(data.segments.size() == 3);
    REQUIRE(std::holds_alternative<RS_ArcData>(data.segments[0]));
    REQUIRE(std::holds_alternative<RS_ArcData>(data.segments[1]));
    const auto checkArcData = [](const RS_ArcData& actual, const RS_ArcData& expected) {
        CHECK(actual.center == expected.center);
        CHECK(actual.radius == expected.radius);
        CHECK(actual.angle1 == expected.angle1);
        CHECK(actual.angle2 == expected.angle2);
        CHECK(actual.reversed == expected.reversed);
    };
    checkArcData(std::get<RS_ArcData>(data.segments[0]), first->getData());
    checkArcData(std::get<RS_ArcData>(data.segments[1]), second->getData());

    auto restoredEntity = snapshot->makeEntity();
    const auto* restored = dynamic_cast<const RS_Polyline*>(restoredEntity.get());
    REQUIRE(restored);
    REQUIRE(restored->size() == source.size());
    for (std::size_t i = 0; i < source.size(); ++i) {
        const auto* expected = dynamic_cast<const RS_Arc*>(source.entityAt(i));
        const auto* actual = dynamic_cast<const RS_Arc*>(restored->entityAt(i));
        if (expected) {
            REQUIRE(actual);
            checkArcData(actual->getData(), expected->getData());
        } else {
            CHECK(restored->entityAt(i)->rtti() == RS2::EntityLine);
        }
    }
}

namespace {

RS_Spline makeSpline(const size_t degree, const std::vector<RS_Vector>& controls,
                     const std::vector<double>& knots, std::vector<double> weights = {}) {
    RS_SplineData data(static_cast<int>(degree), false);
    data.controlPoints = controls;
    data.knotslist = knots;
    data.weights = weights.empty() ? std::vector<double>(controls.size(), 1.0) : std::move(weights);
    return RS_Spline(nullptr, data);
}

LC_SplinePoints fromControlPoints(const std::vector<RS_Vector>& controls, const bool closed = false) {
    LC_SplinePointsData data(closed, false);
    data.useControlPoints = true;
    data.controlPoints = controls;
    return LC_SplinePoints(nullptr, data);
}

RS_Spline quarterCircle(const RS_Vector& center = RS_Vector{0.0, 0.0}, const double r = 1.0) {
    return makeSpline(2, {center + RS_Vector{r, 0.0}, center + RS_Vector{r, r}, center + RS_Vector{0.0, r}},
                      {0, 0, 0, 1, 1, 1}, {1.0, std::sqrt(0.5), 1.0});
}

TEST_CASE("Consistent tangent-constrained linear spline can be offset",
          "[curve-offset][direct][fit-points][tangents][linear]") {
    RS_Spline source = makeSpline(1, {{0, 0}, {4, 0}}, {0, 0, 1, 1});
    source.getData().fitPoints = {{0, 0}, {4, 0}};
    source.getData().fitTangentsConstrained = true;
    source.getData().fitTolerance = 1e-8;
    source.getData().fitStartTangent = {1, 0};
    source.getData().fitEndTangent = {2, 0};
    REQUIRE(source.fitTangentsMatchCurve());
    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.5);
    const auto left = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.5);
    const auto right = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 0.5);

    CHECK(LC_CurveOffset::buildDirectBranches(source, left, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::Ok);
    CHECK(LC_CurveOffset::buildDirectBranches(source, right, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::Ok);

    source.getData().fitEndTangent = {0, 0};
    REQUIRE(source.fitTangentsMatchCurve());
    CHECK(LC_CurveOffset::buildDirectBranches(source, left, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::Ok);
    CHECK(LC_CurveOffset::buildDirectBranches(source, right, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::Ok);

    source.getData().fitEndTangent = {-1, 0};
    CHECK(LC_CurveOffset::buildDirectBranches(source, left, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::InvalidSource);
}

TEST_CASE("Exact tangent-constrained cubic spline can be offset",
          "[curve-offset][direct][fit-points][tangents]") {
    RS_Spline source{nullptr, RS_SplineData{3, false}};
    REQUIRE(source.setFitPointsWithTangents(
        {{0, 0}, {2, 3}, {6, 4}, {9, 0}}, {2, 1}, {1, -3}));
    REQUIRE(source.fitTangentsMatchCurve());
    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
    const auto result = LC_CurveOffset::buildDirectBranches(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25), options,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(result.status == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("Consistent rational tangent-constrained cubic spline can be offset",
          "[curve-offset][direct][fit-points][tangents][rational]") {
    RS_Spline source = makeSpline(3,
        {{0, 0}, {2, 3}, {6, 4}, {9, 0}}, {0, 0, 0, 0, 1, 1, 1, 1},
        {1.0, 0.75, 0.8, 1.0});
    source.getData().fitTangentsConstrained = true;
    source.getData().fitTolerance = 1e-2;
    source.getData().fitStartTangent = {2, 3};
    source.getData().fitEndTangent = {3, -4};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(source.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        source.getData().fitPoints.push_back(jet.point);
    }
    REQUIRE(source.fitTangentsMatchCurve());

    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
    const auto result = LC_CurveOffset::buildDirectBranches(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25), options,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(result.status == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("Consistent rational tangent-constrained quadratic spline can be offset",
          "[curve-offset][direct][fit-points][tangents][rational][quadratic]") {
    RS_Spline source = makeSpline(2, {{0, 0}, {2, 3}, {5, 0}},
        {0, 0, 0, 1, 1, 1}, {1.0, 0.7, 1.0});
    source.getData().fitTangentsConstrained = true;
    source.getData().fitTolerance = 1e-2;
    source.getData().fitStartTangent = {2, 3};
    source.getData().fitEndTangent = {3, -3};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(source.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        source.getData().fitPoints.push_back(jet.point);
    }
    REQUIRE(source.fitTangentsMatchCurve());

    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.1);
    const auto result = LC_CurveOffset::buildDirectBranches(source,
        LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.1), options,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(result.status == LC_CurveOffsetStatus::Ok);
}

// A cubic with an inflection.
RS_Spline sCurve(const RS_Vector& shift = RS_Vector{0.0, 0.0}) {
    return makeSpline(3, {shift + RS_Vector{0, 0}, shift + RS_Vector{4, 6}, shift + RS_Vector{8, -6},
                          shift + RS_Vector{12, 0}},
                      {0, 0, 0, 0, 1, 1, 1, 1});
}

LC_CurveOffsetGeometryResult offsetToSide(const RS_Entity& source, const LC_CurveOffsetSide side,
                                          const double distance) {
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, distance);
    return LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(side, distance), options,
                                               LC_CurveOffset::makeDirectSourceBudget());
}

LC_CurveOffsetGeometryResult offsetTowards(const RS_Entity& source, const RS_Vector& point, const double distance) {
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, distance);
    return LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeDirectionRequest(point, distance),
                                               options, LC_CurveOffset::makeDirectSourceBudget());
}

using EvalFn = std::function<bool(double, LC_CurveEvaluationSide, LC_CurveJet&)>;

EvalFn evaluator(const RS_Spline& s) {
    return [&s](double t, LC_CurveEvaluationSide side, LC_CurveJet& jet) { return s.tryEvaluateJet(t, side, jet); };
}

EvalFn evaluator(const LC_SplinePoints& s) {
    return [&s](double t, LC_CurveEvaluationSide side, LC_CurveJet& jet) { return s.tryEvaluateJet(t, side, jet); };
}

RS_Vector bezierAt(const std::array<RS_Vector, 4>& b, const double s) {
    const double r = 1.0 - s;
    return b[0] * (r * r * r) + b[1] * (3.0 * r * r * s) + b[2] * (3.0 * r * s * s) + b[3] * (s * s * s);
}

/** The exact offset C + d N at t. */
RS_Vector exactOffset(const EvalFn& eval, const double t, const LC_CurveEvaluationSide side, const double d) {
    LC_CurveJet c;
    REQUIRE(eval(t, side, c));
    return c.point + RS_Vector{-c.first.y, c.first.x} / c.first.magnitude() * d;
}

/**
 * The largest distance between the pieces and the exact offset C + d N at the
 * same source parameter, at samples the fitter never used, and between a round
 * corner's arc and its circle of radius |d| about the corner; also checks the
 * pieces are contiguous and follow the source parameter, and that each branch
 * starts where the previous one ends (at a cusp), unless that one ends at a
 * corner that turns towards the offset. A stall, or a stretch of the source
 * that stands still, may leave a gap in the parameter, over which the exact
 * offset, from either side of the gap, moves no more than @p stall; a round
 * corner may span a gap of any size.
 */
double maxDeviation(const LC_CurveOffsetGeometryResult& result, const EvalFn& eval, const double stall = 0.0) {
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE_FALSE(result.branches.empty());
    double worst = 0.0;
    double previousT1 = result.branches.front().cubicPieces.front().provenance.sourceT0;
    const LC_OffsetCubicPiece* previous = nullptr;
    const LC_OffsetBranch* before = nullptr;
    bool corner = false; // since the last piece that follows the source
    for (const LC_OffsetBranch& branch : result.branches) {
    REQUIRE_FALSE(branch.cubicPieces.empty());
    const bool overlap = before != nullptr && before->endEnd == LC_OffsetBranchEnd::Kink;
    corner = corner || overlap;
    for (size_t i = 0; i < branch.cubicPieces.size(); ++i) {
        const LC_OffsetCubicPiece& piece = branch.cubicPieces[i];
        const auto& p = piece.provenance;
        CHECK(p.signedDistance == result.signedDistance);
        if (previous != nullptr && !(overlap && i == 0)) {
            CHECK(piece.bezier[0] == previous->bezier[3]);
        }
        previous = &piece;
        if (p.arcCentre.valid) {
            for (int k = 0; k <= 37; ++k) {
                const double r = bezierAt(piece.bezier, k / 37.0).distanceTo(p.arcCentre);
                worst = std::max(worst, std::abs(r - std::abs(p.signedDistance)));
            }
            corner = true;
            continue;
        }
        CHECK(p.sourceT0 < p.sourceT1);
        if (p.sourceT0 != previousT1) {
            CHECK(p.sourceT0 > previousT1);
            if (!corner) {
                CHECK(exactOffset(eval, previousT1, LC_CurveEvaluationSide::Left, p.signedDistance)
                          .distanceTo(exactOffset(eval, p.sourceT0, LC_CurveEvaluationSide::Right,
                                                  p.signedDistance)) <= stall);
            }
        }
        corner = false;
        previousT1 = p.sourceT1;
        for (int k = 0; k <= 37; ++k) {
            const double s = k / 37.0;
            const double t = p.sourceT0 + s * (p.sourceT1 - p.sourceT0);
            LC_CurveJet c;
            const auto side = (k == 0) ? LC_CurveEvaluationSide::Right
                              : (k == 37) ? LC_CurveEvaluationSide::Left
                                          : LC_CurveEvaluationSide::Interior;
            REQUIRE(eval(t, side, c));
            const RS_Vector n = RS_Vector{-c.first.y, c.first.x} / c.first.magnitude();
            const RS_Vector exact = c.point + n * p.signedDistance;
            worst = std::max(worst, bezierAt(piece.bezier, s).distanceTo(exact));
        }
    }
    before = &branch;
    }
    return worst;
}

} // namespace

TEST_CASE("Default and partial requests are rejected before any evaluation",
          "[curve-offset][direct][validation]") {
    const RS_Spline source = sCurve();
    const LC_CurveOffsetOptions good = LC_CurveOffset::makeDirectOptions(source, 1.0);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0);
    const LC_OffsetSourceBudget budget = LC_CurveOffset::makeDirectSourceBudget();
    REQUIRE(LC_CurveOffset::buildDirectBranches(source, request, good, budget).status == LC_CurveOffsetStatus::Ok);

    // every default is a sentinel that fails, never a silent zero tolerance or open cap
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, LC_CurveOffsetOptions{}, budget).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, good, LC_OffsetSourceBudget{}).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    CHECK(LC_CurveOffset::buildDirectBranches(source, LC_CurveOffsetRequest{}, good, budget).status ==
          LC_CurveOffsetStatus::InvalidDistance);

    LC_CurveOffsetOptions partial = good;
    partial.tolerance.fit = std::nan("");
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, partial, budget).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    partial = good;
    partial.maxSamples = 0;
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, partial, budget).status ==
          LC_CurveOffsetStatus::InvalidRequest);
    partial = good;
    partial.mode = LC_CurveOffsetMode::RegionBoundary; // an open curve encloses no region
    CHECK(LC_CurveOffset::buildDirectBranches(source, request, partial, budget).status ==
          LC_CurveOffsetStatus::InvalidSource);

    for (const double bad : {0.0, -1.0, std::nan(""), std::numeric_limits<double>::infinity()}) {
        CHECK(LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, bad),
                                                  good, budget)
                  .status == LC_CurveOffsetStatus::InvalidDistance);
    }
    // a direction request needs a direction point
    CHECK(LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeDirectionRequest(RS_Vector{false}, 1.0),
                                              good, budget)
              .status == LC_CurveOffsetStatus::InvalidRequest);
    // the factory refuses what has no finite scale
    CHECK(std::isnan(LC_CurveOffset::makeDirectOptions(source, 0.0).tolerance.fit));
    CHECK(std::isnan(LC_CurveOffset::makeDirectOptions(source, std::nan("")).tolerance.fit));

    // factory output is deterministic
    const LC_CurveOffsetOptions again = LC_CurveOffset::makeDirectOptions(source, 1.0);
    CHECK(again.tolerance.fit == good.tolerance.fit);
    CHECK(again.tolerance.requestedGeometry == good.tolerance.requestedGeometry);
    const LC_CurveOffsetTolerances& t = good.tolerance;
    CHECK(t.evaluation + t.fit + t.nodeMerge == Approx(t.requestedGeometry));
}

TEST_CASE("Consistent tangent-constrained spline-points can be offset",
          "[curve-offset][direct][fit-points][tangents][spline-points]") {
    LC_SplinePointsData data{false, false};
    data.splinePoints = {{0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    data.fitTangentsConstrained = true;
    data.fitTolerance = 1e-10;
    LC_SplinePoints source{nullptr, data};
    const double endParameter = static_cast<double>(source.getSegmentCount());
    LC_CurveJet startJet;
    LC_CurveJet endJet;
    REQUIRE(source.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Right, startJet));
    REQUIRE(source.tryEvaluateJet(endParameter, LC_CurveEvaluationSide::Left, endJet));
    source.getData().fitStartTangent = startJet.first;
    source.getData().fitEndTangent = endJet.first;
    REQUIRE(source.fitTangentsMatchCurve());
    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.5);
    const auto left = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.5);
    const auto right = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, 0.5);

    CHECK(LC_CurveOffset::buildDirectBranches(source, left, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::Ok);
    CHECK(LC_CurveOffset::buildDirectBranches(source, right, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::Ok);

    source.getData().fitEndTangent *= -1.0;
    CHECK_FALSE(source.fitTangentsMatchCurve());
    CHECK(LC_CurveOffset::buildDirectBranches(source, left, options,
        LC_CurveOffset::makeDirectSourceBudget()).status == LC_CurveOffsetStatus::InvalidSource);
}

TEST_CASE("Closed fit-point spline sources follow exact offset normals across the seam",
          "[curve-offset][direct][spline-points][closed][oracle]") {
    LC_SplinePointsData data{true, false};
    data.splinePoints = {{0, 0}, {10, 0}, {13, 8}, {4, 13}, {-3, 8}};
    LC_SplinePoints source{nullptr, data};
    REQUIRE(source.isClosed());
    REQUIRE(source.getSegmentCount() > 0);

    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        const double distance = 0.25;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        const auto result = LC_CurveOffset::buildDirectBranches(
            source, LC_CurveOffset::makeSideRequest(side, distance), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, evaluator(source)) <= options.tolerance.requestedGeometry);
    }
}

TEST_CASE("Closed control-point spline-points follow exact offset normals across the seam",
          "[curve-offset][direct][spline-points][closed][control-points][oracle]") {
    const LC_SplinePoints source = fromControlPoints({{0, 0}, {10, 0}, {13, 8}, {4, 13}, {-3, 8}}, true);
    REQUIRE(source.isClosed());
    REQUIRE(source.getData().useControlPoints);
    REQUIRE(source.getSegmentCount() == source.getControlPoints().size());

    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        constexpr double distance = 0.25;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        const auto result = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(side, distance), options,
                                                                LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, evaluator(source)) <= options.tolerance.requestedGeometry);
    }
}

TEST_CASE("A straight source offsets exactly to either side", "[curve-offset][direct]") {
    const LC_SplinePoints line = fromControlPoints({{0.0, 0.0}, {10.0, 0.0}});
    const RS_Spline linear = makeSpline(1, {{0.0, 0.0}, {10.0, 0.0}}, {0, 0, 1, 1});
    for (const RS_Entity* source : std::vector<const RS_Entity*>{&line, &linear}) {
        const LC_CurveOffsetGeometryResult left = offsetTowards(*source, RS_Vector{5.0, 7.0}, 2.0);
        REQUIRE(left.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(left.branches.size() == 1);
        const LC_OffsetBranch& branch = left.branches.front();
        CHECK(branch.straight);
        REQUIRE(branch.cubicPieces.size() == 1);
        CHECK(branch.cubicPieces[0].bezier[0] == RS_Vector(0.0, 2.0));
        CHECK(branch.cubicPieces[0].bezier[3] == RS_Vector(10.0, 2.0));
        CHECK(left.signedDistance == 2.0);

        const LC_CurveOffsetGeometryResult right = offsetTowards(*source, RS_Vector{5.0, -0.1}, 2.0);
        REQUIRE(right.status == LC_CurveOffsetStatus::Ok);
        CHECK(right.signedDistance == -2.0);
        CHECK(right.branches.front().cubicPieces[0].bezier[3] == RS_Vector(10.0, -2.0));
    }
}

TEST_CASE("Native circles preserve exact normals near inner collapse",
          "[curve-offset][direct][circle][oracle]") {
    const RS_Circle source{nullptr, RS_CircleData{{5.0, -2.0}, 2.0}};
    const EvalFn exactCircle = [data = source.getData()](const double parameter, LC_CurveEvaluationSide,
                                                         LC_CurveJet& jet) {
        const double angle = M_PI_4 + parameter;
        jet.point = data.center + RS_Vector{std::cos(angle), std::sin(angle)} * data.radius;
        jet.first = RS_Vector{-std::sin(angle), std::cos(angle)} * data.radius;
        jet.second = RS_Vector{-std::cos(angle), -std::sin(angle)} * data.radius;
        return true;
    };

    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        const double distance = 1.9;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        const auto result = LC_CurveOffset::buildDirectBranches(
            source, LC_CurveOffset::makeSideRequest(side, distance), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        CHECK(maxDeviation(result, exactCircle) <= options.tolerance.requestedGeometry);
    }

    const auto collapseOptions = LC_CurveOffset::makeDirectOptions(source, 2.0);
    const auto collapsed = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 2.0), collapseOptions,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(collapsed.branches.empty());

    const double beyondCollapseDistance = 2.1;
    const auto beyondOptions = LC_CurveOffset::makeDirectOptions(source, beyondCollapseDistance);
    const auto beyondCollapse = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, beyondCollapseDistance), beyondOptions,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(beyondCollapse.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(beyondCollapse.branches.size() == 1);
    CHECK(maxDeviation(beyondCollapse, exactCircle) <= beyondOptions.tolerance.requestedGeometry);
}

TEST_CASE("Zero-radius circles are refused without offset geometry",
          "[curve-offset][direct][circle][refusal]") {
    const RS_Circle source{nullptr, RS_CircleData{{2.0, -3.0}, 0.0}};
    CHECK_FALSE(source.getData().isValid());
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(source));

    const auto options = LC_CurveOffset::makeDirectOptions(source, 0.25);
    const auto result = LC_CurveOffset::buildDirectBranches(
        source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.25), options,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(result.status != LC_CurveOffsetStatus::Ok);
    CHECK(result.branches.empty());
}

TEST_CASE("Native circle offsets preserve exact normals across scale and translation",
          "[curve-offset][direct][circle][oracle][scale]") {
    for (const double scale : {1e-6, 1e-4, 1.0, 1e4, 1e8}) {
        for (const double translationRatio : {0.0, 1e6}) {
            const RS_Vector center{translationRatio * scale, -translationRatio * scale};
            const double radius = 2.0 * scale;
            const double distance = 0.2 * scale;
            const RS_Circle source{nullptr, RS_CircleData{center, radius}};
            CAPTURE(scale, translationRatio);
            const EvalFn exactCircle = [center, radius](const double parameter,
                LC_CurveEvaluationSide, LC_CurveJet& jet) {
                const double angle = M_PI_4 + parameter;
                jet.point = center + RS_Vector{std::cos(angle), std::sin(angle)} * radius;
                jet.first = RS_Vector{-std::sin(angle), std::cos(angle)} * radius;
                jet.second = RS_Vector{-std::cos(angle), -std::sin(angle)} * radius;
                return true;
            };
            const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
            REQUIRE(options.tolerance.requestedGeometry > 0.0);
            for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
                CAPTURE(side);
                const auto result = LC_CurveOffset::buildDirectBranches(
                    source, LC_CurveOffset::makeSideRequest(side, distance), options,
                    LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                CHECK(maxDeviation(result, exactCircle) <= options.tolerance.requestedGeometry);
            }
        }
    }
}

TEST_CASE("Reversed circular arcs follow exact offset normals",
          "[curve-offset][direct][arc][oracle]") {
    const RS_ArcData data{{3.0, -4.0}, 4.0, 0.25, 1.5, true};
    const RS_Arc source{nullptr, data};
    REQUIRE(data.isValid());
    const EvalFn exactArc = [data](const double parameter, LC_CurveEvaluationSide, LC_CurveJet& jet) {
        const double direction = data.reversed ? -1.0 : 1.0;
        const double angle = data.angle1 + direction * parameter;
        jet.point = data.center + RS_Vector{std::cos(angle), std::sin(angle)} * data.radius;
        jet.first = RS_Vector{-std::sin(angle), std::cos(angle)} * (direction * data.radius);
        jet.second = RS_Vector{-std::cos(angle), -std::sin(angle)} * data.radius;
        return true;
    };

    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        const double distance = 0.5;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        const auto result = LC_CurveOffset::buildDirectBranches(
            source, LC_CurveOffset::makeSideRequest(side, distance), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, exactArc) <= options.tolerance.requestedGeometry);
    }
}

TEST_CASE("Circular arc offsets preserve seam and near-full sweep geometry",
          "[curve-offset][direct][arc][oracle][sweep]") {
    for (const double seam : {1e-4, 1e-6, 1e-7, 1e-8}) {
        const std::array<RS_ArcData, 4> profiles{{
            {{3.0, -4.0}, 4.0, 2.0 * M_PI - seam, seam, false},
            {{3.0, -4.0}, 4.0, seam, 2.0 * M_PI - seam, false},
            {{3.0, -4.0}, 4.0, seam, 2.0 * M_PI - seam, true},
            {{3.0, -4.0}, 4.0, 2.0 * M_PI - seam, seam, true},
        }};

        for (std::size_t profileIndex = 0; profileIndex < profiles.size(); ++profileIndex) {
            const RS_ArcData data = profiles[profileIndex];
            const RS_Arc source{nullptr, data};
            CAPTURE(seam, profileIndex, data.angle1, data.angle2, data.reversed);
            REQUIRE(data.isValid());
            const EvalFn exactArc = [data](const double parameter, LC_CurveEvaluationSide,
                                           LC_CurveJet& jet) {
                const double direction = data.reversed ? -1.0 : 1.0;
                const double angle = data.angle1 + direction * parameter;
                jet.point = data.center + RS_Vector{std::cos(angle), std::sin(angle)} * data.radius;
                jet.first = RS_Vector{-std::sin(angle), std::cos(angle)} * (direction * data.radius);
                jet.second = RS_Vector{-std::cos(angle), -std::sin(angle)} * data.radius;
                return true;
            };

            for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
                CAPTURE(side);
                constexpr double distance = 0.5;
                const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
                const auto result = LC_CurveOffset::buildDirectBranches(
                    source, LC_CurveOffset::makeSideRequest(side, distance), options,
                    LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                CHECK(maxDeviation(result, exactArc) <= options.tolerance.requestedGeometry);
            }
        }
    }

    const RS_ArcData belowValidity{{0, 0}, 4.0, 2.0 * M_PI - 4e-9, 4e-9, false};
    CHECK_FALSE(belowValidity.isValid());
    const RS_Arc refused{nullptr, belowValidity};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(refused));
}

TEST_CASE("Translated long lines keep exact offset normals at small relative distance",
          "[curve-offset][direct][line][oracle]") {
    const RS_LineData data{{1.0e9, -2.0e9}, {1.0e9 + 1000.0, -2.0e9 + 250.0}};
    const RS_Line source{nullptr, data};
    const RS_Vector direction = (data.endpoint - data.startpoint) / data.startpoint.distanceTo(data.endpoint);
    const EvalFn exactLine = [data, direction](const double parameter, LC_CurveEvaluationSide,
                                               LC_CurveJet& jet) {
        jet.point = data.startpoint + direction * parameter;
        jet.first = direction;
        jet.second = {0.0, 0.0};
        return true;
    };

    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        const double distance = 0.01;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        const auto result = LC_CurveOffset::buildDirectBranches(
            source, LC_CurveOffset::makeSideRequest(side, distance), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, exactLine) <= options.tolerance.requestedGeometry);
    }
}

TEST_CASE("Line offsets retain exact normals across orientation and scale boundaries",
          "[curve-offset][direct][line][oracle][precision]") {
    struct Profile {
        RS_Vector start;
        RS_Vector end;
    };
    const std::array<Profile, 3> profiles{{
        {{0.0, 0.0}, {10.0, 0.0}},
        {{0.0, 0.0}, {6.0, 8.0}},
        {{6.0, 8.0}, {0.0, 0.0}},
    }};

    for (const double scale : {1e-8, 1.0, 1e8}) {
        for (const double translationRatio : {0.0, 1e6}) {
            const RS_Vector translation{translationRatio * scale, -translationRatio * scale};
            for (std::size_t profileIndex = 0; profileIndex < profiles.size(); ++profileIndex) {
                const RS_LineData data{
                    profiles[profileIndex].start * scale + translation,
                    profiles[profileIndex].end * scale + translation};
                const RS_Line source{nullptr, data};
                const RS_Vector delta = data.endpoint - data.startpoint;
                const double length = std::hypot(delta.x, delta.y);
                REQUIRE(std::isfinite(length));
                REQUIRE(length > 0.0);
                const RS_Vector direction = delta / length;
                const EvalFn exactLine = [data, direction](const double parameter,
                    LC_CurveEvaluationSide, LC_CurveJet& jet) {
                    jet.point = data.startpoint + direction * parameter;
                    jet.first = direction;
                    jet.second = {0.0, 0.0};
                    return true;
                };

                CAPTURE(scale, translationRatio, profileIndex);
                const double distance = 0.02 * scale;
                for (const LC_CurveOffsetSide side :
                     {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
                    CAPTURE(side);
                    const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
                    const auto result = LC_CurveOffset::buildDirectBranches(
                        source, LC_CurveOffset::makeSideRequest(side, distance), options,
                        LC_CurveOffset::makeDirectSourceBudget());
                    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                    REQUIRE(result.branches.size() == 1);
                    CHECK(result.signedDistance == (side == LC_CurveOffsetSide::Left
                        ? distance : -distance));
                    CHECK(maxDeviation(result, exactLine) <= options.tolerance.requestedGeometry);
                }
            }
        }
    }

    const double largeOrigin = 1e16;
    const double nextRepresentable = std::nextafter(largeOrigin, std::numeric_limits<double>::infinity());
    const RS_Line oneUlpLine{nullptr, RS_LineData{{largeOrigin, 0.0}, {nextRepresentable, 0.0}}};
    const auto oneUlpSource = LC_OffsetSourceGeometry::capture(oneUlpLine);
    REQUIRE(oneUlpSource);
    CHECK(oneUlpSource->isWellFormed());
    const double oneUlpLength = nextRepresentable - largeOrigin;
    REQUIRE(oneUlpLength > 0.0);
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        const double distance = .25;
        const auto options = LC_CurveOffset::makeDirectOptions(oneUlpLine, distance);
        const auto result = LC_CurveOffset::buildDirectBranches(
            oneUlpLine, LC_CurveOffset::makeSideRequest(side, distance), options,
            LC_CurveOffset::makeDirectSourceBudget());
        CHECK(result.status == LC_CurveOffsetStatus::InvalidRequest);
        CHECK(result.branches.empty());
        CHECK_FALSE(std::isfinite(result.signedDistance));
    }

    const RS_Line collapsed{nullptr, RS_LineData{{largeOrigin, -1e16}, {largeOrigin + .5, -1e16}}};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(collapsed));
    const RS_Line nonfinite{nullptr, RS_LineData{{0.0, 0.0},
        {std::numeric_limits<double>::infinity(), 0.0}}};
    CHECK_FALSE(LC_OffsetSourceGeometry::capture(nonfinite));
}

TEST_CASE("A parabola's offset follows its exact normals", "[curve-offset][direct]") {
    // B(u) = (10u, 20u(1-u)) turns clockwise, tightest at its apex: radius 2.5.
    const LC_SplinePoints parabola = fromControlPoints({{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}});
    for (const double distance : {0.5, 2.0}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, side, distance);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, distance);
            INFO("distance " << distance << " side " << static_cast<int>(side));
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK(maxDeviation(result, evaluator(parabola)) <= options.tolerance.requestedGeometry);
            CHECK(result.maxObservedError <= options.tolerance.fit);
        }
    }
    // inside its apex, at the radius of curvature, the offset stops dead at the
    // centre of curvature (5, 2.5) and runs on
    const LC_CurveOffsetGeometryResult atRadius = offsetToSide(parabola, LC_CurveOffsetSide::Right, 2.5);
    REQUIRE(atRadius.status == LC_CurveOffsetStatus::Ok);
    CHECK(atRadius.branches.size() == 1);
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 2.5);
    CHECK(maxDeviation(atRadius, evaluator(parabola), options.tolerance.nodeMerge) <=
          options.tolerance.requestedGeometry);
    CHECK(offsetToSide(parabola, LC_CurveOffsetSide::Left, 3.0).status == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("Nearly parallel endpoint tangents retain parabola offset support",
          "[curve-offset][direct][parabola][tangent][oracle]") {
    const std::array<RS_Vector, 2> endpoints{{{0.0, 0.0}, {10.0, 0.0}}};
    for (const double slope : {1e-4, 1e-6, 1e-8}) {
        CAPTURE(slope);
        const LC_ParabolaData data = LC_ParabolaData::FromEndPointsTangents(
            endpoints, {{{1.0, slope}, {1.0, -slope}}});
        REQUIRE(data.isValid());
        CHECK(data.m_controlPoints[1].distanceTo({5.0, 5.0 * slope}) < 1e-7);

        const LC_Parabola source{nullptr, data};
        const auto snapshot = LC_OffsetSourceGeometry::capture(source);
        REQUIRE(snapshot);
        CHECK(snapshot->kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        const EvalFn exactCurve = evaluator(source);
        const double distance = 0.01;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            CAPTURE(side);
            const auto result = offsetToSide(source, side, distance);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK(maxDeviation(result, exactCurve) <= options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("Four-point parabola reconstructions retain typed offset support",
          "[curve-offset][direct][parabola][reconstruction][oracle]") {
    const std::array<RS_Vector, 3> controls{{{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}}};
    const auto quadraticAt = [&controls](const double t) {
        const double u = 1.0 - t;
        return controls[0] * (u * u) + controls[1] * (2.0 * u * t) + controls[2] * (t * t);
    };
    const std::vector<RS_Vector> samples{
        quadraticAt(0.0), quadraticAt(0.3), quadraticAt(0.7), quadraticAt(1.0)};
    const std::vector<LC_ParabolaData> candidates = LC_ParabolaData::From4Points(samples);
    REQUIRE_FALSE(candidates.empty());

    std::size_t validCandidates = 0;
    for (const LC_ParabolaData& candidate : candidates) {
        CAPTURE(validCandidates);
        if (!candidate.isValid()) continue;
        ++validCandidates;

        const LC_Parabola source{nullptr, candidate};
        const auto snapshot = LC_OffsetSourceGeometry::capture(source);
        REQUIRE(snapshot);
        CHECK(snapshot->kind() == LC_OffsetSourceGeometry::Kind::Parabola);
        CHECK(snapshot->isWellFormed());

        const EvalFn exactCurve = evaluator(source);
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            CAPTURE(side);
            const double distance = 0.1;
            const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
            const auto result = offsetToSide(source, side, distance);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            REQUIRE_FALSE(result.branches.empty());
            CHECK(maxDeviation(result, exactCurve) <= options.tolerance.requestedGeometry);
        }
    }
    CHECK(validCandidates > 0);
}

TEST_CASE("A second four-point parabola reconstruction follows exact offset normals",
          "[curve-offset][direct][parabola][reconstruction][oracle]") {
    const std::array<RS_Vector, 3> controls{{{0.0, 0.0}, {5.0, 10.0}, {11.0, 2.0}}};
    const auto quadraticAt = [&controls](const double t) {
        const double u = 1.0 - t;
        return controls[0] * (u * u) + controls[1] * (2.0 * u * t) + controls[2] * (t * t);
    };
    const std::vector<RS_Vector> samples{quadraticAt(0.0), quadraticAt(0.3), quadraticAt(0.7), quadraticAt(1.0)};
    const auto candidates = LC_ParabolaData::From4Points(samples);
    const auto candidate = std::find_if(candidates.begin(), candidates.end(), [](const LC_ParabolaData& data) { return data.isValid(); });
    REQUIRE(candidate != candidates.end());

    const LC_Parabola source{nullptr, *candidate};
    const auto snapshot = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(snapshot);
    CHECK(snapshot->kind() == LC_OffsetSourceGeometry::Kind::Parabola);
    const EvalFn exactCurve = evaluator(source);
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        const double distance = 0.1;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        const auto result = offsetToSide(source, side, distance);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, exactCurve) <= options.tolerance.requestedGeometry);
    }
}

TEST_CASE("Typed parabola offsets remain exact after an affine shape transform",
          "[curve-offset][direct][parabola][affine]") {
    const std::array<LC_ParabolaData, 4> transformedSources{{
        LC_ParabolaData{{RS_Vector{4.0, -3.0}, RS_Vector{15.0, 3.0}, RS_Vector{19.0, -5.0}}},
        LC_ParabolaData::FromEndPointsTangents(
            {RS_Vector{7.0, -2.0}, RS_Vector{-5.0, 0.5}},
            {RS_Vector{-2.0, 9.25}, RS_Vector{-10.0, -6.75}}),
        LC_ParabolaData{{RS_Vector{0.0, 0.0}, RS_Vector{5.0, 50.0}, RS_Vector{10.0, 0.0}}},
        LC_ParabolaData{{RS_Vector{0.0, 0.0}, RS_Vector{5.0, 1e-14}, RS_Vector{10.0, 0.0}}}}};
    CHECK(transformedSources[1].m_controlPoints[1].distanceTo({5.0, 7.25}) < 1e-12);
    for (std::size_t sourceIndex = 0; sourceIndex < transformedSources.size(); ++sourceIndex) {
        for (const double scale : {1e-8, 1.0, 1e8}) {
            CAPTURE(sourceIndex, scale);
            auto controlPoints = transformedSources[sourceIndex].m_controlPoints;
            for (RS_Vector& point : controlPoints) {
                point.x *= scale;
                point.y *= scale;
            }
            const LC_ParabolaData data{controlPoints};
            const LC_Parabola parabola{nullptr, data};
            REQUIRE(data.isValid());
            const auto source = LC_OffsetSourceGeometry::capture(parabola);
            REQUIRE(source);
            CHECK(source->kind() == LC_OffsetSourceGeometry::Kind::Parabola);

            const EvalFn exactCurve = evaluator(parabola);
            for (const double normalizedDistance : {0.05, 0.25}) {
                const double distance = normalizedDistance * scale;
                for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
                    CAPTURE(normalizedDistance, distance, side);
                    const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, side, distance);
                    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, distance);
                    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                    CHECK_FALSE(result.branches.empty());
                    CHECK(maxDeviation(result, exactCurve) <= options.tolerance.requestedGeometry);
                }
            }
        }
    }
}

TEST_CASE("Large-origin scaled parabola offsets retain exact normals", "[curve-offset][direct][parabola][oracle][precision]") {
    constexpr double scale = 1e4;
    const RS_Vector origin{1e6, -2e6};
    const LC_ParabolaData data{{origin, origin + RS_Vector{5.0 * scale, 10.0 * scale}, origin + RS_Vector{10.0 * scale, 0.0}}};
    REQUIRE(data.isValid());
    const LC_Parabola source{nullptr, data};
    const auto typedSource = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(typedSource);
    CHECK(typedSource->kind() == LC_OffsetSourceGeometry::Kind::Parabola);

    const EvalFn exactCurve = evaluator(source);
    for (const double normalizedDistance : {0.05, 0.25}) {
        const double distance = normalizedDistance * scale;
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            CAPTURE(normalizedDistance, distance, side);
            const auto result = offsetToSide(source, side, distance);
            const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            REQUIRE_FALSE(result.branches.empty());
            CHECK(maxDeviation(result, exactCurve) <= options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("Translated small-scale parabola offsets retain exact normals", "[curve-offset][direct][parabola][oracle][precision]") {
    constexpr double scale = 1e-3;
    const RS_Vector origin{10.0, -20.0};
    const LC_ParabolaData data{{origin, origin + RS_Vector{5.0 * scale, 10.0 * scale}, origin + RS_Vector{10.0 * scale, 0.0}}};
    REQUIRE(data.isValid());
    const LC_Parabola source{nullptr, data};
    const auto typedSource = LC_OffsetSourceGeometry::capture(source);
    REQUIRE(typedSource);
    CHECK(typedSource->kind() == LC_OffsetSourceGeometry::Kind::Parabola);

    const EvalFn exactCurve = evaluator(source);
    for (const double normalizedDistance : {0.05, 0.25}) {
        const double distance = normalizedDistance * scale;
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            CAPTURE(normalizedDistance, distance, side);
            const auto result = offsetToSide(source, side, distance);
            const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            REQUIRE_FALSE(result.branches.empty());
            CHECK(maxDeviation(result, exactCurve) <= options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("A circular arc offsets to a concentric arc", "[curve-offset][direct]") {
    const RS_Vector center{3.0, -2.0};
    const RS_Spline arc = quarterCircle(center, 10.0);
    struct Case {
        RS_Vector point; // the side to offset towards
        double radius;
    };
    // outside the counter-clockwise arc is its right side, inside its left
    for (const Case& c : {Case{center + RS_Vector{20.0, 20.0}, 13.0}, Case{center + RS_Vector{1.0, 1.0}, 7.0}}) {
        const LC_CurveOffsetGeometryResult result = offsetTowards(arc, c.point, 3.0);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(arc, 3.0);
        for (const LC_OffsetCubicPiece& piece : result.branches.front().cubicPieces) {
            for (int k = 0; k <= 20; ++k) {
                CHECK(bezierAt(piece.bezier, k / 20.0).distanceTo(center) ==
                      Approx(c.radius).margin(options.tolerance.requestedGeometry));
            }
        }
        CHECK(maxDeviation(result, evaluator(arc)) <= options.tolerance.requestedGeometry);
    }
}

TEST_CASE("Mixed-bulge polyline offsets stay at exact distance from its line and arc segments",
          "[curve-offset][direct][polyline][bulge][oracle]") {
    RS_Polyline source;
    source.addVertex({0, 0}, 0.2);
    source.addVertex({8, 0}, -0.2);
    source.addVertex({8, 6});
    source.addVertex({0, 6});
    source.setClosed(true);
    source.endPolyline();
    const auto sourceSegments = orientedSegments(source);
    REQUIRE(sourceSegments.size() == 4);
    CHECK(sourceSegments[0].arc);
    CHECK(sourceSegments[1].arc);
    std::vector<double> sourceBreaks{0.0};
    for (const auto& segment : sourceSegments) {
        sourceBreaks.push_back(sourceBreaks.back() +
            (segment.arc ? std::abs(segment.sweep) : segment.length()));
    }

    for (const double distance : {0.25, 0.5}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left,
                                               LC_CurveOffsetSide::Right}) {
            CAPTURE(distance, side);
            const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
            const auto result = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(side, distance), options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            REQUIRE_FALSE(result.branches.empty());
            CHECK(result.signedDistance ==
                  (side == LC_CurveOffsetSide::Left ? distance : -distance));

            for (const auto& branch : result.branches) {
                REQUIRE_FALSE(branch.cubicPieces.empty());
                for (const auto& piece : branch.cubicPieces) {
                    if (piece.provenance.arcCentre.valid) {
                        for (int sample = 0; sample <= 37; ++sample) {
                            const RS_Vector point = bezierAt(piece.bezier, sample / 37.0);
                            CHECK(std::abs(point.distanceTo(piece.provenance.arcCentre) - distance) <=
                                  2.0 * options.tolerance.requestedGeometry);
                        }
                        continue;
                    }
                    for (int sample = 0; sample <= 37; ++sample) {
                        if (sample == 0 || sample == 37) continue;
                        const double fraction = sample / 37.0;
                        const double parameter = piece.provenance.sourceT0 + fraction *
                            (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                        std::size_t segmentIndex = 0;
                        while (segmentIndex + 1 < sourceSegments.size() &&
                               parameter >= sourceBreaks[segmentIndex + 1]) {
                            ++segmentIndex;
                        }
                        REQUIRE(parameter >= sourceBreaks[segmentIndex]);
                        REQUIRE(parameter <= sourceBreaks[segmentIndex + 1]);
                        const auto& segment = sourceSegments[segmentIndex];
                        const double local = (parameter - sourceBreaks[segmentIndex]) /
                            (sourceBreaks[segmentIndex + 1] - sourceBreaks[segmentIndex]);
                        RS_Vector tangent;
                        if (segment.arc) {
                            const double angle = segment.startAngle + segment.sweep * local;
                            const double direction = std::copysign(1.0, segment.sweep);
                            tangent = {-std::sin(angle) * direction, std::cos(angle) * direction};
                        } else {
                            tangent = (segment.b - segment.a) / segment.length();
                        }
                        const RS_Vector exact = segment.pointAt(local) +
                            RS_Vector{-tangent.y, tangent.x} * piece.provenance.signedDistance;
                        CHECK(bezierAt(piece.bezier, fraction).distanceTo(exact) <=
                              options.tolerance.requestedGeometry);
                    }
                }
            }
        }
    }
}

TEST_CASE("Open arc-line polylines follow exact normals across tangent and corner joins",
          "[curve-offset][direct][polyline][open][oracle]") {
    const auto verifyProfile = [](const double bulge, const double scale = 1.0, const RS_Vector& translation = RS_Vector{0.0, 0.0}) {
        RS_Polyline source;
        source.addVertex(translation, bulge);
        source.addVertex(translation + RS_Vector{2.0 * scale, 0.0});
        source.addVertex(translation + RS_Vector{2.0 * scale, 2.0 * scale});
        source.endPolyline();
        const auto sourceSegments = orientedSegments(source);
        REQUIRE(sourceSegments.size() == 2);
        REQUIRE(sourceSegments[0].arc);
        CHECK_FALSE(sourceSegments[1].arc);
        CHECK_FALSE(source.isClosed());

        std::vector<double> sourceBreaks{0.0};
        for (const auto& segment : sourceSegments) {
            sourceBreaks.push_back(sourceBreaks.back() + (segment.arc ? std::abs(segment.sweep) : segment.length()));
        }

        for (const double unitDistance : {0.1, 0.25}) {
            const double distance = unitDistance * scale;
            for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
                CAPTURE(bulge, scale, translation, distance, side);
                const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
                const auto result = LC_CurveOffset::buildDirectBranches(source, LC_CurveOffset::makeSideRequest(side, distance), options,
                                                                        LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                REQUIRE_FALSE(result.branches.empty());
                if (bulge == 0.2 && distance == 0.1 && side == LC_CurveOffsetSide::Left) {
                    CHECK(result.branches.size() == 2);
                }
                const double expectedDistance = side == LC_CurveOffsetSide::Left ? distance : -distance;
                const auto exactOffset = [expectedDistance](const LC_OffsetSegment& segment, const double local) {
                    RS_Vector tangent;
                    if (segment.arc) {
                        const double angle = segment.startAngle + segment.sweep * local;
                        const double direction = std::copysign(1.0, segment.sweep);
                        tangent = {-std::sin(angle) * direction, std::cos(angle) * direction};
                    }
                    else {
                        tangent = (segment.b - segment.a) / segment.length();
                    }
                    return segment.pointAt(local) + RS_Vector{-tangent.y, tangent.x} * expectedDistance;
                };
                bool hasStart = false;
                bool hasEnd = false;

                for (const auto& branch : result.branches) {
                    REQUIRE_FALSE(branch.cubicPieces.empty());
                    for (const RS_Vector endpoint : {branch.cubicPieces.front().bezier[0], branch.cubicPieces.back().bezier[3]}) {
                        hasStart = hasStart ||
                            endpoint.distanceTo(exactOffset(sourceSegments.front(), 0.0)) <= options.tolerance.requestedGeometry;
                        hasEnd =
                            hasEnd || endpoint.distanceTo(exactOffset(sourceSegments.back(), 1.0)) <= options.tolerance.requestedGeometry;
                    }

                    for (const auto& piece : branch.cubicPieces) {
                        if (piece.provenance.arcCentre.valid) {
                            for (int sample = 0; sample <= 37; ++sample) {
                                const RS_Vector point = bezierAt(piece.bezier, sample / 37.0);
                                CHECK(std::abs(point.distanceTo(piece.provenance.arcCentre) - distance) <=
                                      2.0 * options.tolerance.requestedGeometry);
                            }
                            continue;
                        }

                        for (int sample = 1; sample < 37; ++sample) {
                            const double fraction = sample / 37.0;
                            const double parameter =
                                piece.provenance.sourceT0 + fraction * (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                            const std::size_t segmentIndex = parameter < sourceBreaks[1] ? 0 : 1;
                            const auto& segment = sourceSegments[segmentIndex];
                            const double local =
                                (parameter - sourceBreaks[segmentIndex]) / (sourceBreaks[segmentIndex + 1] - sourceBreaks[segmentIndex]);
                            CHECK(bezierAt(piece.bezier, fraction).distanceTo(exactOffset(segment, local)) <=
                                  options.tolerance.requestedGeometry);
                        }
                    }
                }
                CHECK(hasStart);
                CHECK(hasEnd);
            }
        }
    };
    verifyProfile(1.0);
    verifyProfile(0.2);
    verifyProfile(2.0);
    verifyProfile(0.2, 1e-3, {10.0, -20.0});
    verifyProfile(0.2, 1e-2, {10.0, -20.0});
    verifyProfile(0.2, 1e4, {1e6, -2e6});
    verifyProfile(2.0, 1e4, {1e6, -2e6});
}

TEST_CASE("Major and minor bulge offsets follow exact segment normals", "[curve-offset][direct][polyline][bulge][major][oracle]") {
    RS_Polyline source;
    source.addVertex({0, 0}, 2.0);
    source.addVertex({10, 0}, -0.2);
    source.addVertex({10, 10});
    source.addVertex({0, 10});
    source.setClosed(true);
    source.endPolyline();
    const auto sourceSegments = orientedSegments(source);
    REQUIRE(sourceSegments.size() == 4);
    REQUIRE(sourceSegments[0].arc);
    REQUIRE(sourceSegments[1].arc);
    CHECK(std::abs(sourceSegments[0].sweep) > M_PI);
    CHECK(std::abs(sourceSegments[1].sweep) < M_PI);
    CHECK(sourceSegments[0].sweep > 0.0);
    CHECK(sourceSegments[1].sweep < 0.0);

    std::vector<double> sourceBreaks{0.0};
    for (const auto& segment : sourceSegments) {
        sourceBreaks.push_back(sourceBreaks.back() +
            (segment.arc ? std::abs(segment.sweep) : segment.length()));
    }

    for (const double distance : {0.25, 0.5}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left,
                                               LC_CurveOffsetSide::Right}) {
            CAPTURE(distance, side);
            const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
            const auto result = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(side, distance), options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            REQUIRE_FALSE(result.branches.empty());

            for (const auto& branch : result.branches) {
                REQUIRE_FALSE(branch.cubicPieces.empty());
                for (const auto& piece : branch.cubicPieces) {
                    if (piece.provenance.arcCentre.valid) {
                        for (int sample = 0; sample <= 37; ++sample) {
                            const RS_Vector point = bezierAt(piece.bezier, sample / 37.0);
                            CHECK(std::abs(point.distanceTo(piece.provenance.arcCentre) - distance) <=
                                  2.0 * options.tolerance.requestedGeometry);
                        }
                        continue;
                    }
                    for (int sample = 1; sample < 37; ++sample) {
                        const double fraction = sample / 37.0;
                        const double parameter = piece.provenance.sourceT0 + fraction *
                            (piece.provenance.sourceT1 - piece.provenance.sourceT0);
                        std::size_t segmentIndex = 0;
                        while (segmentIndex + 1 < sourceSegments.size() &&
                               parameter >= sourceBreaks[segmentIndex + 1]) {
                            ++segmentIndex;
                        }
                        REQUIRE(parameter >= sourceBreaks[segmentIndex]);
                        REQUIRE(parameter <= sourceBreaks[segmentIndex + 1]);
                        const auto& segment = sourceSegments[segmentIndex];
                        const double local = (parameter - sourceBreaks[segmentIndex]) /
                            (sourceBreaks[segmentIndex + 1] - sourceBreaks[segmentIndex]);
                        RS_Vector tangent;
                        if (segment.arc) {
                            const double angle = segment.startAngle + segment.sweep * local;
                            const double direction = std::copysign(1.0, segment.sweep);
                            tangent = {-std::sin(angle) * direction, std::cos(angle) * direction};
                        } else {
                            tangent = (segment.b - segment.a) / segment.length();
                        }
                        const RS_Vector exact = segment.pointAt(local) +
                            RS_Vector{-tangent.y, tangent.x} * piece.provenance.signedDistance;
                        CHECK(bezierAt(piece.bezier, fraction).distanceTo(exact) <=
                              options.tolerance.requestedGeometry);
                    }
                }
            }
        }
    }
}

TEST_CASE("Tangent-constrained rational and polynomial splines follow their exact offset normals",
          "[curve-offset][direct][spline][tangents][oracle]") {
    RS_Spline polynomial{nullptr, RS_SplineData{3, false}};
    REQUIRE(polynomial.setFitPointsWithTangents(
        {{0, 0}, {2, 3}, {6, 4}, {9, 0}}, {2, 1}, {1, -3}));
    REQUIRE(polynomial.fitTangentsMatchCurve());

    RS_SplineData rationalData{3, false};
    rationalData.controlPoints = {{0, 0}, {2, 3}, {6, 4}, {9, 0}};
    rationalData.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    rationalData.weights = {1.0, 0.75, 0.8, 1.0};
    RS_Spline rational{nullptr, rationalData};
    rational.getData().fitTangentsConstrained = true;
    rational.getData().fitTolerance = 1e-2;
    rational.getData().fitStartTangent = {2, 3};
    rational.getData().fitEndTangent = {3, -4};
    for (const double parameter : {0.0, 0.2, 0.5, 0.8, 1.0}) {
        LC_CurveJet jet;
        REQUIRE(rational.tryEvaluateJet(parameter,
            parameter == 1.0 ? LC_CurveEvaluationSide::Left
                             : LC_CurveEvaluationSide::Right, jet));
        rational.getData().fitPoints.push_back(jet.point);
    }
    REQUIRE(rational.fitTangentsMatchCurve());

    const std::array<const RS_Spline*, 2> sources{&polynomial, &rational};
    for (std::size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
        const RS_Spline& source = *sources[sourceIndex];
        for (const double distance : {0.125, 0.5}) {
            for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left,
                                                   LC_CurveOffsetSide::Right}) {
                CAPTURE(sourceIndex, distance, side);
                const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
                const auto result = LC_CurveOffset::buildDirectBranches(
                    source, LC_CurveOffset::makeSideRequest(side, distance), options,
                    LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                CHECK(maxDeviation(result, evaluator(source)) <=
                      options.tolerance.requestedGeometry);
            }
        }
    }
}

TEST_CASE("Tangent-constrained spline-points follow their exact offset normals",
          "[curve-offset][direct][spline-points][tangents][oracle]") {
    LC_SplinePointsData data{false, false};
    data.splinePoints = {{0, 0}, {1, 2}, {3, 1.5}, {4, 0}};
    data.fitTangentsConstrained = true;
    data.fitTolerance = 1e-10;
    LC_SplinePoints source{nullptr, data};
    const double endParameter = static_cast<double>(source.getSegmentCount());
    LC_CurveJet start;
    LC_CurveJet end;
    REQUIRE(source.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Right, start));
    REQUIRE(source.tryEvaluateJet(endParameter, LC_CurveEvaluationSide::Left, end));
    source.getData().fitStartTangent = start.first;
    source.getData().fitEndTangent = end.first;
    REQUIRE(source.fitTangentsMatchCurve());

    for (const double distance : {0.125, 0.5}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left,
                                               LC_CurveOffsetSide::Right}) {
            CAPTURE(distance, side);
            const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
            const auto result = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(side, distance), options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            CHECK(maxDeviation(result, evaluator(source)) <=
                  options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("Clamped rational degree-four through degree-six splines follow exact offset normals",
          "[curve-offset][direct][spline][high-degree][rational][oracle]") {
    for (const std::size_t degree : {4U, 5U, 6U}) {
        std::vector<RS_Vector> controls;
        std::vector<double> weights;
        for (std::size_t i = 0; i <= degree; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(degree);
            controls.emplace_back(4.0 * t, 2.0 * t * (1.0 - t));
            weights.push_back(0.75 + 0.5 * t);
        }
        std::vector<double> knots(degree + 1, 0.0);
        knots.insert(knots.end(), degree + 1, 1.0);
        const RS_Spline source = makeSpline(degree, controls, knots, weights);
        CAPTURE(degree);
        REQUIRE(source.validate());

        for (const double distance : {0.05, 0.2}) {
            for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left,
                                                   LC_CurveOffsetSide::Right}) {
                CAPTURE(distance, side);
                const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
                const auto result = LC_CurveOffset::buildDirectBranches(
                    source, LC_CurveOffset::makeSideRequest(side, distance), options,
                    LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                CHECK(maxDeviation(result, evaluator(source)) <=
                      options.tolerance.requestedGeometry);
            }
        }
    }
}

TEST_CASE("Closed periodic cubic splines follow exact offset normals at their seam",
          "[curve-offset][direct][spline][periodic][oracle]") {
    RS_Spline source{nullptr, RS_SplineData{3, false}};
    for (const RS_Vector& point : {RS_Vector{0, 0}, RS_Vector{40, -10},
                                   RS_Vector{60, 30}, RS_Vector{20, 50},
                                   RS_Vector{-15, 25}}) {
        source.addControlPoint(point);
    }
    source.setClosed(true);
    REQUIRE(source.validate());
    double first = 0.0;
    double last = 0.0;
    REQUIRE(source.getParameterDomain(first, last));
    REQUIRE(last > first);

    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left,
                                           LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        const double distance = 0.2;
        const auto options = LC_CurveOffset::makeDirectOptions(source, distance);
        const auto result = LC_CurveOffset::buildDirectBranches(
            source, LC_CurveOffset::makeSideRequest(side, distance), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, evaluator(source)) <=
              options.tolerance.requestedGeometry);
    }
}

TEST_CASE("An S-curve offsets on both sides through its inflection", "[curve-offset][direct]") {
    const RS_Spline s = sCurve();
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        const LC_CurveOffsetGeometryResult result = offsetToSide(s, side, 0.75);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, evaluator(s)) <=
              LC_CurveOffset::makeDirectOptions(s, 0.75).tolerance.requestedGeometry);
    }
}

TEST_CASE("The offset is regular or it is refused", "[curve-offset][direct][regularity]") {
    // Inside a unit arc the offset shrinks to a point at d = 1: a cusp for any
    // open offset curve. Either side of it the offset is regular.
    const RS_Spline arc = quarterCircle();
    const LC_CurveOffsetGeometryResult singular = offsetToSide(arc, LC_CurveOffsetSide::Left, 1.0);
    CHECK((singular.status == LC_CurveOffsetStatus::SingularOffset ||
           singular.status == LC_CurveOffsetStatus::AmbiguousRegularity));
    CHECK(singular.branches.empty());
    CHECK(offsetToSide(arc, LC_CurveOffsetSide::Left, 0.99).status == LC_CurveOffsetStatus::Ok);
    CHECK(offsetToSide(arc, LC_CurveOffsetSide::Left, 1.01).status == LC_CurveOffsetStatus::Ok);

    // A source that never moves has no direction anywhere (see also the
    // tests of repeated control points below).
    const RS_Spline point = makeSpline(3, {{1, 2}, {1, 2}, {1, 2}, {1, 2}}, {0, 0, 0, 0, 1, 1, 1, 1});
    const LC_CurveOffsetGeometryResult noTangent = offsetToSide(point, LC_CurveOffsetSide::Left, 0.1);
    CHECK(noTangent.status == LC_CurveOffsetStatus::UndefinedTangent);
    CHECK(noTangent.branches.empty());
}

TEST_CASE("A kink is rounded where it turns away from the offset, its sides overlap where it turns towards it",
          "[curve-offset][direct][kink]") {
    // At (3, 0) the cubic turns left, from (1, -1) to (1, 2): the right side
    // leaves a gap, rounded by an arc of radius 0.2 about the kink; on the left
    // the two sides' offsets overlap, and each branch ends there.
    const RS_Spline kinked = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 0}, {4, 2}, {5, 2}, {6, 0}},
                                        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    const LC_CurveOffsetGeometryResult right = offsetToSide(kinked, LC_CurveOffsetSide::Right, 0.2);
    REQUIRE(right.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(right.branches.size() == 1);
    int arcPieces = 0;
    for (const LC_OffsetCubicPiece& piece : right.branches.front().cubicPieces) {
        if (piece.provenance.arcCentre.valid) {
            ++arcPieces;
            CHECK(piece.provenance.arcCentre.distanceTo(RS_Vector{3.0, 0.0}) < 1e-12);
            CHECK(piece.bezier[0].distanceTo(RS_Vector{3.0, 0.0}) == Approx(0.2).epsilon(1e-12));
            CHECK(piece.bezier[3].distanceTo(RS_Vector{3.0, 0.0}) == Approx(0.2).epsilon(1e-12));
        }
    }
    CHECK(arcPieces > 0);

    const LC_CurveOffsetGeometryResult left = offsetToSide(kinked, LC_CurveOffsetSide::Left, 0.2);
    REQUIRE(left.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(left.branches.size() == 2);
    CHECK(left.branches[0].endEnd == LC_OffsetBranchEnd::Kink);
    CHECK(left.branches[1].startEnd == LC_OffsetBranchEnd::Kink);
    CHECK(left.branches[0].cubicPieces.back().bezier[3].distanceTo(left.branches[1].cubicPieces.front().bezier[0]) >
          0.1);

    // the same knot multiplicity with collinear handles is G1
    const RS_Spline smooth = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 2}, {6, 0}},
                                        {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    const LC_CurveOffsetGeometryResult result = offsetToSide(smooth, LC_CurveOffsetSide::Left, 0.2);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(maxDeviation(result, evaluator(smooth)) <=
          LC_CurveOffset::makeDirectOptions(smooth, 0.2).tolerance.requestedGeometry);
}

TEST_CASE("A cusp hidden at a join where the curvature jumps splits the offset there",
          "[curve-offset][direct][cusp]") {
    // Two quadratic segments meet with a common tangent, turning right. The
    // first bends with radii between 1.4 and 11.3; the second, past the join,
    // with radii of 28 and more. At a distance between those, 1 - d kappa is
    // negative throughout the first segment and positive throughout the second:
    // no box inside either span contains a zero, yet the offset turns back at
    // the join.
    const LC_SplinePoints joined = fromControlPoints({{0, 0}, {2, 2}, {4, 0}, {24.9, -21.1}});
    auto radiusAt = [&joined](const double t, const LC_CurveEvaluationSide side) {
        LC_CurveJet j;
        REQUIRE(joined.tryEvaluateJet(t, side, j));
        const double s = j.first.magnitude();
        return s * s * s / std::abs(j.first.x * j.second.y - j.first.y * j.second.x);
    };
    const double firstMax = radiusAt(0.0, LC_CurveEvaluationSide::Right);  // largest on the first segment
    const double secondMin = radiusAt(1.0, LC_CurveEvaluationSide::Right); // smallest on the second
    const double between = std::sqrt(firstMax * secondMin);
    REQUIRE(firstMax < between);
    REQUIRE(between < secondMin);
    // the inside of the turn is the right side: one branch per segment, meeting at the join
    const LC_CurveOffsetGeometryResult split = offsetToSide(joined, LC_CurveOffsetSide::Right, between);
    REQUIRE(split.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(split.branches.size() == 2);
    CHECK(split.branches.front().cubicPieces.back().provenance.sourceT1 == 1.0);
    CHECK(maxDeviation(split, evaluator(joined)) <=
          LC_CurveOffset::makeDirectOptions(joined, between).tolerance.requestedGeometry);
    CHECK(offsetToSide(joined, LC_CurveOffsetSide::Right, 0.5).branches.size() == 1);
    CHECK(offsetToSide(joined, LC_CurveOffsetSide::Left, between).branches.size() == 1);
}

TEST_CASE("The nearest point just before a closed curve's seam decides the side", "[curve-offset][direct][side]") {
    // A thin closed strip, counter-clockwise, its seam on the top edge at
    // (10, 0.05). The point is 0.01 above the top edge, just before the seam,
    // and 0.06 above the bottom edge: the top edge, seen from outside, is nearer.
    const LC_SplinePoints strip = fromControlPoints({{0, 0.05}, {0, 0}, {20, 0}, {20, 0.05}}, true);
    const RS_Vector point{10.2, 0.06};
    const LC_OffsetSideResolution side =
        LC_CurveOffset::resolveSide(strip, point, LC_CurveOffset::makeDirectOptions(strip, 1.0));
    REQUIRE(side.status == LC_CurveOffsetStatus::Ok);
    CHECK(side.side == LC_CurveOffsetSide::Right);
}

TEST_CASE("A click outside a sharp turn at a closed curve's seam still has a side", "[curve-offset][direct][side]") {
    // The seam sits in a turn of 135 degrees within 1e-4 of parameter. The
    // nearest point lies just after the seam; refined from before it, the seam
    // itself, no minimum, is as near to the tolerance but on the incoming tangent.
    const LC_SplinePoints corner = fromControlPoints({{0, 0}, {10, 0}, {10, 10}, {0.001, 0.001}}, true);
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(corner, 1.0);
    for (const RS_Vector& point : {RS_Vector{-0.5, -1.0}, RS_Vector{-1.0, -1.0}}) {
        INFO("point " << point.x << ", " << point.y);
        const LC_OffsetSideResolution side = LC_CurveOffset::resolveSide(corner, point, options);
        REQUIRE(side.status == LC_CurveOffsetStatus::Ok);
        CHECK(side.side == LC_CurveOffsetSide::Right); // outside of a counter-clockwise curve
    }
}

TEST_CASE("The sample limit counts every branch of a cusped offset", "[curve-offset][direct][cusp]") {
    const RS_Spline parabola = makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1});
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    const LC_CurveOffsetGeometryResult geometry = LC_CurveOffset::buildDirectBranches(
        parabola, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() == 3);
    // the least sample limit that materializes a geometry
    auto least = [&](const LC_CurveOffsetGeometryResult& g) {
        std::size_t lo = g.exactSamples;
        std::size_t hi = options.maxSamples;
        while (lo < hi) {
            LC_CurveOffsetOptions limited = options;
            limited.maxSamples = lo + (hi - lo) / 2;
            const bool ok = LC_CurveOffset::materializeBranches(parabola, g, limited,
                                                                LC_CurveOffset::makeDirectSourceBudget())
                                .status == LC_CurveOffsetStatus::Ok;
            (ok ? hi : lo) = ok ? limited.maxSamples : limited.maxSamples + 1;
        }
        return lo;
    };
    std::size_t perBranch = 0;
    for (const LC_OffsetBranch& branch : geometry.branches) {
        LC_CurveOffsetGeometryResult one = geometry;
        one.branches = {branch};
        perBranch += least(one) - geometry.exactSamples;
    }
    CHECK(least(geometry) == geometry.exactSamples + perBranch);
}

TEST_CASE("A direction point on the curve has no side", "[curve-offset][direct][side]") {
    const RS_Spline s = sCurve();
    LC_CurveJet onCurve;
    REQUIRE(s.tryEvaluateJet(0.3, LC_CurveEvaluationSide::Interior, onCurve));
    CHECK(offsetTowards(s, onCurve.point, 1.0).status == LC_CurveOffsetStatus::AmbiguousSide);
    // on the tangent line beyond an open end
    const LC_SplinePoints line = fromControlPoints({{0.0, 0.0}, {10.0, 0.0}});
    CHECK(offsetTowards(line, RS_Vector{15.0, 0.0}, 1.0).status == LC_CurveOffsetStatus::AmbiguousSide);
    // just off it, the side is defined
    CHECK(offsetTowards(line, RS_Vector{15.0, 0.5}, 1.0).status == LC_CurveOffsetStatus::Ok);
}

TEST_CASE("Reversing the source swaps the side a point selects, not the offset", "[curve-offset][direct][side]") {
    RS_Spline forward = sCurve();
    RS_Spline reversed = sCurve();
    reversed.revertDirection();
    const RS_Vector point{6.0, 4.0};
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(forward, 1.0);
    const LC_OffsetSideResolution a = LC_CurveOffset::resolveSide(forward, point, options);
    const LC_OffsetSideResolution b = LC_CurveOffset::resolveSide(reversed, point, options);
    REQUIRE(a.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(b.status == LC_CurveOffsetStatus::Ok);
    CHECK(a.side != b.side);

    const LC_CurveOffsetGeometryResult fa = offsetTowards(forward, point, 1.0);
    const LC_CurveOffsetGeometryResult fb = offsetTowards(reversed, point, 1.0);
    REQUIRE(fa.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(fb.status == LC_CurveOffsetStatus::Ok);
    const auto& pa = fa.branches.front().cubicPieces;
    const auto& pb = fb.branches.front().cubicPieces;
    CHECK(pa.front().bezier[0].distanceTo(pb.back().bezier[3]) < 1e-9);
    CHECK(pa.back().bezier[3].distanceTo(pb.front().bezier[0]) < 1e-9);

    // the side of a multi-copy offset is resolved once, for every distance
    const LC_OffsetSideResolution far = LC_CurveOffset::resolveSide(
        forward, point, LC_CurveOffset::makeDirectOptions(forward, 3.0));
    CHECK(far.side == a.side);
}

TEST_CASE("Closed sources offset to a closed chain; a flag without a seam is refused",
          "[curve-offset][direct][closed]") {
    const LC_SplinePoints ring = fromControlPoints({{0, 0}, {10, 0}, {12, 8}, {2, 9}}, true);
    const LC_CurveOffsetGeometryResult outward = offsetTowards(ring, RS_Vector{30.0, 30.0}, 1.0);
    REQUIRE(outward.status == LC_CurveOffsetStatus::Ok);
    const LC_OffsetBranch& branch = outward.branches.front();
    CHECK(branch.closed);
    CHECK(branch.cubicPieces.back().bezier[3] == branch.cubicPieces.front().bezier[0]);
    CHECK(maxDeviation(outward, evaluator(ring)) <=
          LC_CurveOffset::makeDirectOptions(ring, 1.0).tolerance.requestedGeometry);

    RS_Spline periodic(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        periodic.addControlPoint(p);
    }
    periodic.setClosed(true);
    const LC_CurveOffsetGeometryResult inward = offsetTowards(periodic, RS_Vector{22.0, 18.0}, 2.0);
    REQUIRE(inward.status == LC_CurveOffsetStatus::Ok);
    CHECK(inward.branches.front().closed);

    RS_SplineData wrapped(3, true);
    wrapped.type = RS_SplineData::SplineType::WrappedClosed;
    wrapped.controlPoints = {{0, 0}, {15, 25}, {40, 35}, {70, 20}, {80, 0}, {0, 0}, {15, 25}, {40, 35}};
    wrapped.weights.assign(wrapped.controlPoints.size(), 1.0);
    wrapped.knotslist = {0.0, 12.0, 35.0, 60.0, 100.0, 140.0, 180.0, 220.0, 260.0, 290.0, 320.0, 350.0};
    const RS_Spline gap(nullptr, wrapped);
    CHECK(offsetToSide(gap, LC_CurveOffsetSide::Left, 1.0).status == LC_CurveOffsetStatus::InvalidSource);
}

TEST_CASE("Offsets stay accurate far from the origin", "[curve-offset][direct]") {
    const RS_Spline near = sCurve();
    const RS_Spline far = sCurve(RS_Vector{1.0e6, -2.0e6});
    const LC_CurveOffsetGeometryResult a = offsetToSide(near, LC_CurveOffsetSide::Right, 0.5);
    const LC_CurveOffsetGeometryResult b = offsetToSide(far, LC_CurveOffsetSide::Right, 0.5);
    REQUIRE(a.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(b.status == LC_CurveOffsetStatus::Ok);
    CHECK(maxDeviation(b, evaluator(far)) <= LC_CurveOffset::makeDirectOptions(far, 0.5).tolerance.requestedGeometry);
}

TEST_CASE("Unsupported and non-planar sources are refused", "[curve-offset][direct]") {
    RS_Spline raised = sCurve();
    raised.getData().controlPoints[1].z = 1.0;
    CHECK(offsetToSide(raised, LC_CurveOffsetSide::Left, 1.0).status == LC_CurveOffsetStatus::UnsupportedNonPlanar);
    CHECK_FALSE(LC_CurveOffset::isSupportedSource(RS_Line(nullptr, RS_LineData{{0, 0}, {1, 0}})));
    CHECK(LC_CurveOffset::isSupportedSource(sCurve()));
    CHECK(LC_CurveOffset::isSupportedSource(fromControlPoints({{0, 0}, {1, 1}, {2, 0}})));
}

TEST_CASE("Caps end the offset with no branch", "[curve-offset][direct][limits]") {
    const RS_Spline s = sCurve();
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(s, 0.5);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.5);
    const LC_CurveOffsetGeometryResult full =
        LC_CurveOffset::buildDirectBranches(s, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(full.status == LC_CurveOffsetStatus::Ok);
    const size_t pieces = full.branches.front().cubicPieces.size();
    REQUIRE(pieces > 1);

    LC_OffsetSourceBudget tight = LC_CurveOffset::makeDirectSourceBudget();
    tight.maxCubicPieces = pieces - 1;
    const LC_CurveOffsetGeometryResult capped = LC_CurveOffset::buildDirectBranches(s, request, options, tight);
    CHECK(capped.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(capped.branches.empty());
    tight.maxCubicPieces = pieces;
    CHECK(LC_CurveOffset::buildDirectBranches(s, request, options, tight).status == LC_CurveOffsetStatus::Ok);

    LC_CurveOffsetOptions fewSamples = options;
    fewSamples.maxSamples = 10;
    CHECK(LC_CurveOffset::buildDirectBranches(s, request, fewSamples, LC_CurveOffset::makeDirectSourceBudget())
              .status == LC_CurveOffsetStatus::LimitExceeded);

    LC_CurveOffsetOptions shallow = options;
    shallow.maxSubdivisionDepth = 1;
    const LC_CurveOffsetGeometryResult unmet =
        LC_CurveOffset::buildDirectBranches(s, request, shallow, LC_CurveOffset::makeDirectSourceBudget());
    CHECK(unmet.status != LC_CurveOffsetStatus::Ok);
    CHECK(unmet.branches.empty());
}

TEST_CASE("Piece counts at the default tolerance", "[curve-offset][direct][calibration]") {
    // Recorded for the D1 tolerance choice: one output entity per piece.
    const RS_Spline s = sCurve();
    const RS_Spline arc = quarterCircle(RS_Vector{0, 0}, 50.0);
    const LC_SplinePoints parabola = fromControlPoints({{0.0, 0.0}, {5.0, 10.0}, {10.0, 0.0}});
    for (const double relative : {1e-4, 1e-5, 1e-6}) {
        auto count = [relative](const RS_Entity& source, const double d) {
            const LC_CurveOffsetOptions probe = LC_CurveOffset::makeDirectOptions(source, d);
            const double feature = probe.tolerance.requestedGeometry / LC_CurveOffset::kDefaultRelativeOffsetTolerance;
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, d, relative * feature);
            const LC_CurveOffsetGeometryResult r = LC_CurveOffset::buildDirectBranches(
                source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Right, d), options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(r.status == LC_CurveOffsetStatus::Ok);
            return r.branches.front().cubicPieces.size();
        };
        const size_t sPieces = count(s, 0.75);
        const size_t arcPieces = count(arc, 5.0);
        const size_t parabolaPieces = count(parabola, 1.0);
        WARN("relative tolerance " << relative << ": S-curve " << sPieces << ", quarter circle " << arcPieces
                                   << ", parabola " << parabolaPieces << " pieces");
        CHECK(sPieces < 200);
    }
}

// ---------------------------------------------------------------------------
// Materialization (D1b)
// ---------------------------------------------------------------------------
namespace {
LC_CurveOffsetMaterializationResult materialize(const RS_Entity& source, const LC_CurveOffsetSide side,
                                                const double distance,
                                                const LC_OffsetSourceBudget budget = LC_CurveOffset::makeDirectSourceBudget()) {
    return LC_CurveOffset::createEntities(source, LC_CurveOffset::makeSideRequest(side, distance),
                                          LC_CurveOffset::makeDirectOptions(source, distance), budget);
}
} // namespace

TEST_CASE("A branch materializes as one cubic spline whose spans are its validated pieces",
          "[curve-offset][direct][materialize]") {
    const RS_Spline s = sCurve();
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(s, 0.75);
    const LC_CurveOffsetRequest request = LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 0.75);
    const LC_CurveOffsetGeometryResult geometry =
        LC_CurveOffset::buildDirectBranches(s, request, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(geometry.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(geometry.branches.size() == 1);
    const std::vector<LC_OffsetCubicPiece> before = geometry.branches.front().cubicPieces;
    REQUIRE(before.size() > 1);

    const LC_CurveOffsetMaterializationResult result =
        LC_CurveOffset::materializeBranches(s, geometry, options, LC_CurveOffset::makeDirectSourceBudget());
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    CHECK(result.validationLevel == LC_OffsetValidationLevel::SampledBidirectional);
    CHECK(result.maxObservedError <= options.tolerance.requestedGeometry);
    REQUIRE(result.entities.size() == 1);
    CHECK(result.usage.outputEntities == 1);
    CHECK(result.usage.cubicPieces >= before.size());

    const auto* spline = dynamic_cast<const RS_Spline*>(result.entities.front().get());
    REQUIRE(spline != nullptr);
    CHECK(spline->validate());
    CHECK(spline->getDegree() == 3);
    CHECK_FALSE(spline->isClosed());
    CHECK(spline->getParent() == nullptr);
    CHECK_FALSE(spline->isSelected());
    CHECK(result.usage.deepEntities == spline->count());
    // integer knots, of multiplicity 3 inside: each span is one piece's Bezier net
    const RS_SplineData& data = spline->getData();
    const size_t spans = result.usage.cubicPieces;
    REQUIRE(data.controlPoints.size() == 3 * spans + 1);
    REQUIRE(data.knotslist.size() == data.controlPoints.size() + 4);
    for (size_t k = 0; k < data.knotslist.size(); ++k) {
        const double expected = (k < 4) ? 0.0 : std::min(static_cast<double>((k - 1) / 3), static_cast<double>(spans));
        CHECK(data.knotslist[k] == expected);
    }
    // it follows the exact offset like the pieces did
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline->getParameterDomain(t0, t1));
    CHECK(t0 == 0.0);
    CHECK(t1 == static_cast<double>(spans));
    CHECK(spline->getStartpoint() == before.front().bezier[0]);
    CHECK(spline->getEndpoint() == before.back().bezier[3]);

    // materializing reads the branch; it never changes it
    const auto& after = geometry.branches.front().cubicPieces;
    REQUIRE(after.size() == before.size());
    for (size_t i = 0; i < before.size(); ++i) {
        CHECK(after[i].bezier == before[i].bezier);
    }
}

TEST_CASE("A straight source materializes as one exact line", "[curve-offset][direct][materialize]") {
    const LC_SplinePoints line = fromControlPoints({{0.0, 0.0}, {10.0, 0.0}});
    const LC_CurveOffsetMaterializationResult result = materialize(line, LC_CurveOffsetSide::Right, 2.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.entities.size() == 1);
    const auto* offset = dynamic_cast<const RS_Line*>(result.entities.front().get());
    REQUIRE(offset != nullptr);
    CHECK(offset->getStartpoint() == RS_Vector(0.0, -2.0));
    CHECK(offset->getEndpoint() == RS_Vector(10.0, -2.0));
    CHECK(result.maxObservedError <= 1e-12);
}

TEST_CASE("Output copies only the source's layer and pen", "[curve-offset][direct][materialize]") {
    RS_Spline s = sCurve();
    const RS_Pen pen{RS_Color{200, 10, 30}, RS2::Width04, RS2::DashLine};
    s.setPen(pen);
    s.setSelectionFlag(true);
    s.setHighlighted(true);
    const LC_CurveOffsetMaterializationResult result = materialize(s, LC_CurveOffsetSide::Right, 0.5);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    for (const std::unique_ptr<RS_Entity>& entity : result.entities) {
        CHECK(entity->getPen(false) == pen);
        CHECK_FALSE(entity->isSelected());
        CHECK_FALSE(entity->isHighlighted());
        CHECK(entity->getParent() == nullptr);
    }
}

TEST_CASE("Limits and failures return no entity", "[curve-offset][direct][materialize][limits]") {
    const RS_Spline s = sCurve();
    const LC_CurveOffsetMaterializationResult full = materialize(s, LC_CurveOffsetSide::Left, 0.75);
    REQUIRE(full.status == LC_CurveOffsetStatus::Ok);
    const std::size_t pieces = full.usage.cubicPieces;
    REQUIRE(pieces > 1);

    LC_OffsetSourceBudget budget = LC_CurveOffset::makeDirectSourceBudget();
    budget.maxCubicPieces = pieces - 1;
    const LC_CurveOffsetMaterializationResult tooMany = materialize(s, LC_CurveOffsetSide::Left, 0.75, budget);
    CHECK(tooMany.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(tooMany.entities.empty());

    budget = LC_CurveOffset::makeDirectSourceBudget();
    budget.maxDeepEntities = full.usage.deepEntities - 1;
    const LC_CurveOffsetMaterializationResult tooDeep = materialize(s, LC_CurveOffsetSide::Left, 0.75, budget);
    CHECK(tooDeep.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(tooDeep.entities.empty());

    budget.maxDeepEntities = full.usage.deepEntities;
    CHECK(materialize(s, LC_CurveOffsetSide::Left, 0.75, budget).status == LC_CurveOffsetStatus::Ok);

    // a failed geometry never reaches an entity
    const RS_Spline arc = quarterCircle();
    const LC_CurveOffsetMaterializationResult singular = materialize(arc, LC_CurveOffsetSide::Left, 1.0);
    CHECK(singular.status != LC_CurveOffsetStatus::Ok);
    CHECK(singular.entities.empty());

    // a failed geometry result handed to the materializer is refused as it is
    LC_CurveOffsetGeometryResult failed;
    failed.status = LC_CurveOffsetStatus::SingularOffset;
    CHECK(LC_CurveOffset::materializeBranches(s, failed, LC_CurveOffset::makeDirectOptions(s, 1.0),
                                              LC_CurveOffset::makeDirectSourceBudget())
              .status == LC_CurveOffsetStatus::SingularOffset);
}

TEST_CASE("A closed source materializes as one open spline whose ends meet in a smooth stretch",
          "[curve-offset][direct][materialize][closed]") {
    // LibreCAD's closed spline is a wrapped one, which a chain of cubic pieces is
    // not: the chain is an open spline whose ends coincide, starting in the
    // middle of a piece so that the seam is no corner.
    const LC_SplinePoints ring = fromControlPoints({{0, 0}, {10, 0}, {12, 8}, {2, 9}}, true);
    const LC_CurveOffsetMaterializationResult result = materialize(ring, LC_CurveOffsetSide::Right, 1.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.entities.size() == 1);
    const auto& spline = dynamic_cast<const RS_Spline&>(*result.entities.front());
    CHECK_FALSE(spline.isClosed());
    CHECK(spline.getEndpoint() == spline.getStartpoint());
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline.getParameterDomain(t0, t1));
    LC_CurveJet start;
    LC_CurveJet end;
    REQUIRE(spline.tryEvaluateJet(t0, LC_CurveEvaluationSide::Right, start));
    REQUIRE(spline.tryEvaluateJet(t1, LC_CurveEvaluationSide::Left, end));
    CHECK(std::abs(RS_Vector::crossP(start.first, end.first).z) <=
          1e-9 * start.first.magnitude() * end.first.magnitude());
}

TEST_CASE("Offsets of 20-control-point cubics, timed", "[.benchmark]") {
    // The interactive target: below 50 ms for one regular, open cubic with 20
    // control points at the default tolerance, Direct and Trimmed (what the
    // tools use, on every preview mouse move), measured without sanitizers.
    struct Shape {
        const char* name;
        std::vector<RS_Vector> points;
        double distance;
    };
    std::vector<Shape> shapes{{"wave", {}, 1.0}, {"spiral", {}, 2.0}, {"cubic", {}, 5.0}};
    for (int i = 0; i < 20; ++i) {
        shapes[0].points.emplace_back(10.0 * i, 15.0 * std::sin(0.7 * i));
        const double angle = 0.35 * i;
        shapes[1].points.emplace_back((40.0 + 4.0 * i) * std::cos(angle), (40.0 + 4.0 * i) * std::sin(angle));
        shapes[2].points.emplace_back(10.0 * i, 0.02 * (i - 10.0) * (i - 10.0) * (i - 10.0));
    }
    for (const Shape& shape : shapes) {
        RS_SplineData d(3, false);
        d.controlPoints = shape.points;
        d.knotslist = {0, 0, 0, 0};
        for (int k = 1; k <= 16; ++k) {
            d.knotslist.push_back(k);
        }
        d.knotslist.insert(d.knotslist.end(), 4, 17.0);
        d.weights.assign(20, 1.0);
        const RS_Spline source(nullptr, d);
        for (const LC_CurveOffsetMode mode : {LC_CurveOffsetMode::Direct, LC_CurveOffsetMode::Trimmed}) {
            std::vector<double> ms;
            size_t pieces = 0;
            for (int run = 0; run < 41; ++run) {
                LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(source, shape.distance);
                options.mode = mode;
                const auto start = std::chrono::steady_clock::now();
                const LC_CurveOffsetMaterializationResult r = LC_CurveOffset::createEntities(
                    source, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, shape.distance), options,
                    LC_CurveOffset::makeDirectSourceBudget());
                ms.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count());
                REQUIRE(r.status == LC_CurveOffsetStatus::Ok);
                pieces = r.usage.cubicPieces;
            }
            std::sort(ms.begin(), ms.end());
            const double median = ms[ms.size() / 2];
            const double p95 = ms[ms.size() * 95 / 100];
            WARN(shape.name << (mode == LC_CurveOffsetMode::Direct ? " direct: " : " trimmed: ") << pieces
                            << " pieces, median " << median << " ms, p95 " << p95 << " ms");
            CHECK(median < 50.0);
        }
    }
}

// ---------------------------------------------------------------------------
// Cusp branches (D2)
// ---------------------------------------------------------------------------
namespace {
/** y = x^2 on [-2, 2], at x = 4t - 2: radius of curvature (1 + 4x^2)^(3/2) / 2, 0.5 at the vertex. */
RS_Spline unitParabola() {
    return makeSpline(2, {{-2, 4}, {0, -4}, {2, 4}}, {0, 0, 0, 1, 1, 1});
}

/** Where the radius of y = x^2 equals d, as parameters of unitParabola(): its cusps inside. */
std::array<double, 2> parabolaCusps(const double d) {
    const double x = std::sqrt((std::cbrt(4.0 * d * d) - 1.0) / 4.0);
    return {(2.0 - x) / 4.0, (2.0 + x) / 4.0};
}
} // namespace

TEST_CASE("Inside its radius of curvature the offset is one branch; past it, it splits at the cusps",
          "[curve-offset][cusp]") {
    const RS_Spline parabola = unitParabola();
    // inside the vertex radius: regular
    CHECK(offsetToSide(parabola, LC_CurveOffsetSide::Left, 0.4).branches.size() == 1);

    for (const double d : {0.501, 0.51, 0.75, 1.0, 1.5}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        // before the first cusp, the swallowtail between them, after the second
        REQUIRE(result.branches.size() == 3);
        CHECK_FALSE(result.branches[0].reversed);
        CHECK(result.branches[1].reversed);
        CHECK_FALSE(result.branches[2].reversed);
        CHECK(result.branches[0].startEnd == LC_OffsetBranchEnd::Free);
        CHECK(result.branches[0].endEnd == LC_OffsetBranchEnd::Cusp);
        CHECK(result.branches[2].endEnd == LC_OffsetBranchEnd::Free);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
        CHECK(maxDeviation(result, evaluator(parabola)) <= options.tolerance.requestedGeometry);
        const std::array<double, 2> cusps = parabolaCusps(d);
        for (size_t k = 0; k < 2; ++k) {
            const LC_OffsetCubicPiece& end = result.branches[k].cubicPieces.back();
            CHECK(end.provenance.sourceT1 == Approx(cusps[k]).margin(1e-9));
            // the cusp: the exact offset there, where its derivative vanishes
            LC_CurveJet c;
            REQUIRE(parabola.tryEvaluateJet(cusps[k], LC_CurveEvaluationSide::Interior, c));
            const RS_Vector exact = c.point + RS_Vector{-c.first.y, c.first.x} / c.first.magnitude() * d;
            CHECK(end.bezier[3].distanceTo(exact) <= options.tolerance.requestedGeometry);
            CHECK(result.branches[k + 1].cubicPieces.front().bezier[0] == end.bezier[3]);
        }
        for (const LC_OffsetBranch& branch : result.branches) {
            CHECK_FALSE(branch.closed);
        }
    }
}

TEST_CASE("A swallowtail within twice the merge tolerance is collapsed before anything else", "[curve-offset][cusp]") {
    // A hair past the vertex radius the cusps are resolved but the swallowtail
    // between them spans less than twice the merge tolerance: the branches
    // either side meet at its middle as one branch.
    const RS_Spline parabola = unitParabola();
    for (const double d : {0.5 + 1e-6, 0.5 + 1e-7}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        CHECK_FALSE(result.branches.front().reversed);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
        CHECK(maxDeviation(result, evaluator(parabola), 2.0 * options.tolerance.nodeMerge) <=
              options.tolerance.requestedGeometry);
    }
}

TEST_CASE("At a radius of curvature the offset stalls and runs on, one branch", "[curve-offset][cusp][stall]") {
    const RS_Spline parabola = unitParabola();
    // At the vertex radius 1 - d kappa touches zero without changing sign: the
    // offset slows to a stop at the centre of curvature (0, 0.5) and runs on in
    // the same direction. A hair past it the two cusps lie closer than the
    // merge tolerance, which leaves the same shape.
    for (const double d : {0.5, 0.5 + 1e-13}) {
        INFO("distance " << d);
        const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        REQUIRE(result.branches.size() == 1);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
        CHECK(maxDeviation(result, evaluator(parabola), options.tolerance.nodeMerge) <=
              options.tolerance.requestedGeometry);
        // the stall is a shared end of two pieces, at the centre of curvature
        const auto& pieces = result.branches.front().cubicPieces;
        const bool atCentre = std::any_of(pieces.begin(), pieces.end(), [&](const LC_OffsetCubicPiece& piece) {
            return piece.bezier[3].distanceTo(RS_Vector{0.0, 0.5}) <= options.tolerance.requestedGeometry;
        });
        CHECK(atCentre);
        // and the same way every time
        CHECK(offsetToSide(parabola, LC_CurveOffsetSide::Left, d).branches.front().cubicPieces.size() ==
              pieces.size());
    }
    // a whole span of zeros: the arc's offset shrinks to its centre
    const LC_CurveOffsetGeometryResult point = offsetToSide(quarterCircle(), LC_CurveOffsetSide::Left, 1.0);
    CHECK(point.status != LC_CurveOffsetStatus::Ok);
    CHECK(point.branches.empty());
}

TEST_CASE("A stall is found whatever the curve's orientation", "[curve-offset][cusp][stall]") {
    // Rotated, the interval bounds of 1 - d kappa no longer cancel, and the
    // unproved run around the vertex is hundreds of boxes long.
    for (const double angle : {0.0, M_PI / 6.0, M_PI / 4.0, 1.0}) {
        RS_Spline parabola = unitParabola();
        parabola.rotate(RS_Vector{0.3, -0.2}, angle);
        for (const double d : {0.5, 0.5 * (1.0 + 1e-9), 0.5 * (1.0 - 1e-9), 0.5 * (1.0 + 1e-6)}) {
            INFO("angle " << angle << " distance " << std::setprecision(17) << d);
            const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
            CHECK(maxDeviation(result, evaluator(parabola), options.tolerance.nodeMerge) <=
                  options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("Near the radius of curvature the offset fits far from the origin too", "[curve-offset][cusp][stall]") {
    // Where the offset all but stops its exact tangent is mostly cancellation;
    // the fit weighs a tangent error by how far the offset moves.
    for (const RS_Vector& shift : {RS_Vector{1.0e4, -2.0e4}, RS_Vector{1.0e6, -2.0e6}}) {
        RS_Spline parabola = unitParabola();
        parabola.move(shift);
        for (const double d : {0.49999, 0.499999, 0.5, 0.5 * (1.0 + 1e-6)}) {
            INFO("shift " << shift.x << " distance " << std::setprecision(17) << d);
            const LC_CurveOffsetGeometryResult result = offsetToSide(parabola, LC_CurveOffsetSide::Left, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, d);
            CHECK(maxDeviation(result, evaluator(parabola), 2.0 * options.tolerance.nodeMerge) <=
                  options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("A stall across a closed curve's seam closes the branch", "[curve-offset][cusp][stall][closed]") {
    // A uniform closed cubic, symmetric about the x axis, whose seam lies at its
    // sharpest point on the positive x axis: the curvature is largest there.
    RS_Spline lemon(nullptr, RS_SplineData(3, false));
    // (the seam lies at the second control point's influence centre)
    for (const RS_Vector& p : {RS_Vector{2, -1}, RS_Vector{4, 0}, RS_Vector{2, 1}, RS_Vector{-2, 1}, RS_Vector{-4, 0},
                               RS_Vector{-2, -1}}) {
        lemon.addControlPoint(p);
    }
    lemon.setClosed(true);
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(lemon.getParameterDomain(t0, t1));
    LC_CurveJet seam;
    REQUIRE(lemon.tryEvaluateJet(t0, LC_CurveEvaluationSide::Right, seam));
    REQUIRE(std::abs(seam.point.y) < 1e-12);
    const double kappa = RS_Vector::crossP(seam.first, seam.second).z / std::pow(seam.first.magnitude(), 3.0);
    REQUIRE(std::abs(kappa) > 0.0);
    const double rho = 1.0 / std::abs(kappa);
    // the centre of curvature at the seam lies to the side the curve turns
    const LC_CurveOffsetSide inward = kappa > 0.0 ? LC_CurveOffsetSide::Left : LC_CurveOffsetSide::Right;
    const LC_CurveOffsetGeometryResult result = offsetToSide(lemon, inward, rho);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.branches.size() == 1);
    CHECK(result.branches.front().closed);
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(lemon, rho);
    CHECK(maxDeviation(result, evaluator(lemon), options.tolerance.nodeMerge) <= options.tolerance.requestedGeometry);
}

TEST_CASE("Cusp branches materialize as entities meeting at the cusps", "[curve-offset][cusp][materialize]") {
    const RS_Spline parabola = unitParabola();
    const LC_CurveOffsetMaterializationResult result = materialize(parabola, LC_CurveOffsetSide::Left, 1.0);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.entities.size() >= 3);
    for (size_t i = 1; i < result.entities.size(); ++i) {
        CHECK(result.entities[i]->getStartpoint() == result.entities[i - 1]->getEndpoint());
    }
    // the swallowtail between the cusps runs against the source: its entities
    // go right to left while the source goes left to right
    size_t reversed = 0;
    for (const std::unique_ptr<RS_Entity>& entity : result.entities) {
        reversed += entity->getEndpoint().x < entity->getStartpoint().x ? 1 : 0;
    }
    CHECK(reversed > 0);
    CHECK(reversed < result.entities.size());

    // more branches than allowed
    LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(parabola, 1.0);
    options.maxOutputBranches = 2;
    const LC_CurveOffsetGeometryResult capped = LC_CurveOffset::buildDirectBranches(
        parabola, LC_CurveOffset::makeSideRequest(LC_CurveOffsetSide::Left, 1.0), options,
        LC_CurveOffset::makeDirectSourceBudget());
    CHECK(capped.status == LC_CurveOffsetStatus::LimitExceeded);
    CHECK(capped.branches.empty());
}

TEST_CASE("A closed source's cusped offset is a cycle of open branches", "[curve-offset][cusp]") {
    RS_Spline ring(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        ring.addControlPoint(p);
    }
    ring.setClosed(true);
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(ring.getParameterDomain(t0, t1));
    // its tightest bend, from samples
    double tightest = RS_MAXDOUBLE;
    for (int i = 0; i < 2000; ++i) {
        LC_CurveJet j;
        REQUIRE(ring.tryEvaluateJet(t0 + (t1 - t0) * i / 2000.0, LC_CurveEvaluationSide::Interior, j));
        const double s = j.first.magnitude();
        tightest = std::min(tightest, s * s * s / std::abs(j.first.x * j.second.y - j.first.y * j.second.x));
    }
    // inwards (the ring turns left), well past the tightest bend
    const double d = 1.5 * tightest;
    const LC_CurveOffsetGeometryResult result = offsetToSide(ring, LC_CurveOffsetSide::Left, d);
    REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(result.branches.size() >= 2);
    CHECK(result.branches.size() % 2 == 0); // 1 - d kappa changes sign an even number of times around
    for (size_t i = 0; i < result.branches.size(); ++i) {
        const LC_OffsetBranch& branch = result.branches[i];
        const LC_OffsetBranch& next = result.branches[(i + 1) % result.branches.size()];
        CHECK_FALSE(branch.closed);
        CHECK(next.cubicPieces.front().bezier[0] == branch.cubicPieces.back().bezier[3]);
    }
    // every piece against the exact offset
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(ring, d);
    for (const LC_OffsetBranch& branch : result.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            const auto& p = piece.provenance;
            for (int k = 1; k < 16; ++k) {
                const double s = k / 16.0;
                LC_CurveJet c;
                REQUIRE(ring.tryEvaluateJet(p.sourceT0 + s * (p.sourceT1 - p.sourceT0),
                                            LC_CurveEvaluationSide::Interior, c));
                const RS_Vector exact = c.point + RS_Vector{-c.first.y, c.first.x} / c.first.magnitude() * d;
                CHECK(bezierAt(piece.bezier, s).distanceTo(exact) <= options.tolerance.requestedGeometry);
            }
        }
    }
}

namespace {
/** A branch of one straight cubic piece from @p from to @p to. */
LC_OffsetBranch straightBranch(const RS_Vector& from, const RS_Vector& to, const bool reversed,
                               const LC_OffsetBranchEnd startEnd, const LC_OffsetBranchEnd endEnd) {
    LC_OffsetCubicPiece piece;
    piece.bezier = {from, from + (to - from) / 3.0, from + (to - from) * (2.0 / 3.0), to};
    LC_OffsetBranch branch;
    branch.cubicPieces.push_back(piece);
    branch.reversed = reversed;
    branch.startEnd = startEnd;
    branch.endEnd = endEnd;
    return branch;
}
} // namespace

TEST_CASE("A tiny reversed branch at an end of a closed chain collapses across the seam",
          "[curve-offset][cusp][closed]") {
    // The branches of a closed source's offset are a ring: the one after the
    // last is the first. Collapsing a tiny reversed branch took its neighbour
    // at k + 1 or k - 1 without wrapping, which at either end of the ring read
    // (and wrote) outside the vector.
    const double merge = 1.0;

    SECTION("the last branch takes the first as its neighbour") {
        std::vector<LC_OffsetBranch> branches{
            straightBranch({0, 0}, {100, 0}, false, LC_OffsetBranchEnd::Cusp, LC_OffsetBranchEnd::Kink),
            straightBranch({100, 10}, {0, 10}, false, LC_OffsetBranchEnd::Kink, LC_OffsetBranchEnd::Cusp),
            // reversed, shorter than the merge tolerance, kink before it and cusp after
            straightBranch({0, 10}, {0.1, 10}, true, LC_OffsetBranchEnd::Kink, LC_OffsetBranchEnd::Cusp)};

        LC_CurveOffset::collapseTinyReversals(branches, merge);

        REQUIRE(branches.size() == 2);
        // the first branch now starts where the collapsed one did
        CHECK(branches.front().cubicPieces.front().bezier[0] == RS_Vector{0, 10});
        CHECK(branches.front().startEnd == LC_OffsetBranchEnd::Free);
    }

    SECTION("the first branch takes the last as its neighbour") {
        std::vector<LC_OffsetBranch> branches{
            // reversed, tiny, cusp before it and kink after
            straightBranch({0.1, 10}, {0, 10}, true, LC_OffsetBranchEnd::Cusp, LC_OffsetBranchEnd::Kink),
            straightBranch({0, 0}, {100, 0}, false, LC_OffsetBranchEnd::Kink, LC_OffsetBranchEnd::Cusp),
            straightBranch({100, 10}, {0.1, 10}, false, LC_OffsetBranchEnd::Cusp, LC_OffsetBranchEnd::Cusp)};

        LC_CurveOffset::collapseTinyReversals(branches, merge);

        REQUIRE(branches.size() == 2);
        // the last branch now ends where the collapsed one did
        CHECK(branches.back().cubicPieces.back().bezier[3] == RS_Vector{0, 10});
        CHECK(branches.back().endEnd == LC_OffsetBranchEnd::Free);
    }
}

TEST_CASE("A closed source's branches are joined and collapsed around their ring",
          "[curve-offset][cusp][closed]") {
    // The branches of a closed source's offset form a ring: the last one runs
    // into the first. Two steps used to step off the ends of that ring -
    // collapsing a tiny reversed branch took the neighbour at k + 1 or k - 1
    // without wrapping, and joining the corners read the first branch again
    // after a push had moved it. Both need a closed source with a corner and a
    // distance near a bend as tight as it, so the sweep below walks distances
    // through and past the tightest bend of three closed sources. It checks
    // that nothing is read outside the ring (under a sanitizer) and that what
    // comes back is a ring.
    RS_Spline wrapped(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        wrapped.addControlPoint(p);
    }
    wrapped.setClosed(true);

    // a corner at the seam and another opposite it: a repeated point stands still
    const LC_SplinePoints cornered = fromControlPoints(
        {{0, 0}, {50, 0}, {50, 0}, {70, 35}, {30, 60}, {30, 60}, {-10, 30}}, true);
    const LC_SplinePoints squarish = fromControlPoints(
        {{0, 0}, {40, 0}, {40, 40}, {0, 40}}, true);

    const std::vector<const RS_Entity*> sources{&wrapped, &cornered, &squarish};
    for (const RS_Entity* source : sources) {
        double tightest = RS_MAXDOUBLE;
        double t0 = 0.0;
        double t1 = 0.0;
        const RS_Vector extent = source->getMax() - source->getMin();
        const double size = std::max(extent.x, extent.y);
        if (const auto* spline = dynamic_cast<const RS_Spline*>(source); spline != nullptr) {
            REQUIRE(spline->getParameterDomain(t0, t1));
            for (int i = 0; i < 400; ++i) {
                LC_CurveJet j;
                if (!spline->tryEvaluateJet(t0 + (t1 - t0) * i / 400.0, LC_CurveEvaluationSide::Interior, j)) {
                    continue;
                }
                const double speed = j.first.magnitude();
                const double turn = std::abs(j.first.x * j.second.y - j.first.y * j.second.x);
                if (turn > 0.0) {
                    tightest = std::min(tightest, speed * speed * speed / turn);
                }
            }
        }
        const double largest = std::isfinite(tightest) ? std::min(2.0 * tightest, size) : 0.4 * size;
        for (int step = 1; step <= 60; ++step) {
            const double distance = largest * step / 60.0;
            for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
                INFO("distance " << distance << " side " << static_cast<int>(side));
                const LC_CurveOffsetGeometryResult result = offsetToSide(*source, side, distance);
                if (result.status != LC_CurveOffsetStatus::Ok) {
                    continue; // a distance the engine refuses is not this test's business
                }
                REQUIRE_FALSE(result.branches.empty());
                for (size_t i = 0; i < result.branches.size(); ++i) {
                    const LC_OffsetBranch& branch = result.branches[i];
                    REQUIRE_FALSE(branch.cubicPieces.empty());
                    if (branch.closed) {
                        CHECK(result.branches.size() == 1);
                        continue;
                    }
                    // every branch of a ring ends where the next one begins
                    const LC_OffsetBranch& next = result.branches[(i + 1) % result.branches.size()];
                    if (branch.endEnd == LC_OffsetBranchEnd::Cusp) {
                        CHECK(next.cubicPieces.front().bezier[0] == branch.cubicPieces.back().bezier[3]);
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Hyperbolas
// ---------------------------------------------------------------------------
namespace {
/**
 * A hyperbola's exact jet, written here from its definition rather than taken
 * from the engine's adapter: centre + R (s a cosh t, b sinh t), R the rotation
 * onto the major axis, s = -1 on the left branch.
 */
EvalFn hyperbolaEvaluator(const LC_HyperbolaData& d) {
    return [d](const double t, LC_CurveEvaluationSide, LC_CurveJet& jet) {
        const double a = d.majorP.magnitude();
        const double b = a * d.ratio;
        const double s = d.reversed ? -1.0 : 1.0;
        const double angle = d.majorP.angle();
        const auto world = [&](const double x, const double y) { return RS_Vector{x, y}.rotated(angle); };
        jet.point = d.center + world(s * a * std::cosh(t), b * std::sinh(t));
        jet.first = world(s * a * std::sinh(t), b * std::cosh(t));
        jet.second = world(s * a * std::cosh(t), b * std::sinh(t));
        return true;
    };
}
} // namespace

TEST_CASE("A hyperbola is offset along its exact curve on either branch and either side",
          "[curve-offset][hyperbola]") {
    // a = |(4, 2)| = 4.47, b = 0.6 a = 2.68: the radius of curvature is smallest
    // at the vertex, b^2 / a = 1.61, so both distances stay short of it
    for (const bool reversed : {false, true}) {
        const LC_HyperbolaData data{RS_Vector{5.0, -3.0}, RS_Vector{4.0, 2.0}, 0.6, -1.2, 1.5, reversed};
        const LC_Hyperbola hyperbola(nullptr, data);
        REQUIRE(hyperbola.isValid());
        REQUIRE(LC_CurveOffset::isSupportedSource(hyperbola));
        // the evaluator above is the entity's own curve
        LC_CurveJet jet;
        REQUIRE(hyperbolaEvaluator(data)(0.4, LC_CurveEvaluationSide::Interior, jet));
        CHECK(jet.point.distanceTo(hyperbola.getPoint(0.4, reversed)) < 1e-12);

        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            for (const double distance : {0.3, 1.0}) {
                INFO("reversed " << reversed << " side " << static_cast<int>(side) << " distance " << distance);
                const LC_CurveOffsetGeometryResult result = offsetToSide(hyperbola, side, distance);
                REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(hyperbola, distance);
                CHECK(maxDeviation(result, hyperbolaEvaluator(data)) <= options.tolerance.requestedGeometry);
                // one branch from end to end: a smooth, convex source has no corners or cusps
                CHECK(result.branches.size() == 1);
            }
        }
    }
}

TEST_CASE("A rotated hyperbola with a wide bounded interval follows exact offset normals",
          "[curve-offset][direct][hyperbola][oracle]") {
    const LC_HyperbolaData data{RS_Vector{12.0, -7.0}, RS_Vector{3.0, 1.0}, 1.2, -2.0, 1.8, false};
    const LC_Hyperbola hyperbola{nullptr, data};
    REQUIRE(hyperbola.isValid());
    REQUIRE(LC_CurveOffset::isSupportedSource(hyperbola));

    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        CAPTURE(side);
        for (const double distance : {0.25, 1.0}) {
            CAPTURE(distance);
            const auto options = LC_CurveOffset::makeDirectOptions(hyperbola, distance);
            const auto result = LC_CurveOffset::buildDirectBranches(
                hyperbola, LC_CurveOffset::makeSideRequest(side, distance), options,
                LC_CurveOffset::makeDirectSourceBudget());
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(result.branches.size() == 1);
            CHECK(maxDeviation(result, hyperbolaEvaluator(data)) <= options.tolerance.requestedGeometry);
        }
    }
}

TEST_CASE("Hyperbola offset normals hold across eccentricity, branch, and reversed bounds",
          "[curve-offset][direct][hyperbola][oracle][variants]") {
    const std::array<LC_HyperbolaData, 2> profiles{{
        LC_HyperbolaData{{350.0, -275.0}, {17.32050807568877, 10.0}, 0.2, -1.4, 1.8, true},
        LC_HyperbolaData{{-5.0, 9.0}, {6.128355544951824, -5.142300877492315}, 1.5, 1.7, -1.9, false},
    }};

    for (const LC_HyperbolaData& data : profiles) {
        const LC_Hyperbola hyperbola{nullptr, data};
        REQUIRE(hyperbola.isValid());
        REQUIRE(LC_CurveOffset::isSupportedSource(hyperbola));
        const double vertexRadius = data.majorP.magnitude() * data.ratio * data.ratio;

        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            CAPTURE(data.ratio, data.reversed, data.angle1, data.angle2, side);
            for (const double fraction : {0.05, 0.2}) {
                const double distance = fraction * vertexRadius;
                CAPTURE(distance);
                const auto options = LC_CurveOffset::makeDirectOptions(hyperbola, distance);
                const auto result = LC_CurveOffset::buildDirectBranches(hyperbola, LC_CurveOffset::makeSideRequest(side, distance), options,
                                                                        LC_CurveOffset::makeDirectSourceBudget());
                REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
                REQUIRE(result.branches.size() == 1);
                CHECK(maxDeviation(result, hyperbolaEvaluator(data)) <= options.tolerance.requestedGeometry);
            }
        }
    }
}

TEST_CASE("A hyperbola whose ends run backwards is offset all the same", "[curve-offset][hyperbola]") {
    // grip edits, DXF imports and reversing a hyperbola all leave angle1 > angle2
    const LC_HyperbolaData forwards{RS_Vector{5.0, -3.0}, RS_Vector{4.0, 2.0}, 0.6, -1.2, 1.5, false};
    LC_HyperbolaData backwards = forwards;
    std::swap(backwards.angle1, backwards.angle2);
    const LC_Hyperbola hyperbola(nullptr, backwards);
    REQUIRE(hyperbola.isValid());
    REQUIRE(LC_CurveOffset::isSupportedSource(hyperbola));
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        INFO("side " << static_cast<int>(side));
        const LC_CurveOffsetGeometryResult result = offsetToSide(hyperbola, side, 1.0);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(hyperbola, 1.0);
        CHECK(maxDeviation(result, hyperbolaEvaluator(forwards)) <= options.tolerance.requestedGeometry);
        REQUIRE(result.branches.size() == 1);
        // it runs from end to end: the offsets of both ends are on it
        const LC_OffsetBranch& branch = result.branches.front();
        const RS_Vector first = branch.cubicPieces.front().bezier[0];
        const RS_Vector last = branch.cubicPieces.back().bezier[3];
        for (const double t : {-1.2, 1.5}) {
            const RS_Vector end = hyperbola.getPoint(t, false);
            CHECK(std::min(first.distanceTo(end), last.distanceTo(end)) == Catch::Approx(1.0).margin(1e-6));
        }
    }
}

TEST_CASE("A hyperbola past its vertex's radius of curvature is trimmed", "[curve-offset][hyperbola]") {
    const LC_HyperbolaData data{RS_Vector{0.0, 0.0}, RS_Vector{3.0, 0.0}, 1.0, -1.5, 1.5, false};
    const LC_Hyperbola hyperbola(nullptr, data);
    // a = b = 3: radius of curvature 3 at the vertex (3, 0); towards the focus at
    // (4.24, 0) an offset by 4 folds back there, and trimming cuts the fold away
    std::vector<RS_Entity*> towardsFocus = hyperbola.createOffset(RS_Vector{10.0, 0.0}, 4.0);
    std::vector<RS_Entity*> away = hyperbola.createOffset(RS_Vector{0.0, 0.0}, 4.0);
    CHECK_FALSE(towardsFocus.empty());
    CHECK_FALSE(away.empty());
    // nothing of the trimmed offset lies nearer the hyperbola than the distance
    for (RS_Entity* e : towardsFocus) {
        for (int k = 0; k <= 50; ++k) {
            RS_Vector p;
            if (auto* spline = dynamic_cast<RS_Spline*>(e)) {
                double t0 = 0.0;
                double t1 = 0.0;
                REQUIRE(spline->getParameterDomain(t0, t1));
                LC_CurveJet j;
                REQUIRE(spline->tryEvaluateJet(t0 + (t1 - t0) * k / 50.0, LC_CurveEvaluationSide::Interior, j));
                p = j.point;
            }
            else {
                p = e->getStartpoint() + (e->getEndpoint() - e->getStartpoint()) * (k / 50.0);
            }
            // the nearest point of the hyperbola, by a fine search over its parameter
            double nearest = RS_MAXDOUBLE;
            for (int i = 0; i <= 3000; ++i) {
                nearest = std::min(nearest, p.distanceTo(hyperbola.getPoint(-1.5 + 3.0 * i / 3000.0, false)));
            }
            CHECK(nearest > 4.0 - 1e-3);
        }
    }
    for (RS_Entity* e : towardsFocus) {
        delete e;
    }
    for (RS_Entity* e : away) {
        delete e;
    }
}

TEST_CASE("An unbounded hyperbola is not offset", "[curve-offset][hyperbola]") {
    // both angles zero: the whole of both branches, with no ends to offset between
    const LC_Hyperbola hyperbola(nullptr, LC_HyperbolaData{RS_Vector{0.0, 0.0}, RS_Vector{3.0, 0.0}, 1.0});
    REQUIRE(hyperbola.isInfinite());
    CHECK(hyperbola.createOffset(RS_Vector{10.0, 0.0}, 1.0).empty());
    const LC_CurveOffsetGeometryResult result = offsetToSide(hyperbola, LC_CurveOffsetSide::Left, 1.0);
    CHECK(result.status != LC_CurveOffsetStatus::Ok);
}

// ---------------------------------------------------------------------------
// Repeated control points: stretches that stand still, tangents that vanish
// ---------------------------------------------------------------------------
namespace {

/** Whether a piece of the result ends within @p tolerance of @p p, and whether any piece is a round corner. */
bool endsNear(const LC_CurveOffsetGeometryResult& result, const RS_Vector& p, const double tolerance, bool& arcs) {
    bool near = false;
    arcs = false;
    for (const LC_OffsetBranch& branch : result.branches) {
        near = near || branch.cubicPieces.front().bezier[0].distanceTo(p) <= tolerance;
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            near = near || piece.bezier[3].distanceTo(p) <= tolerance;
            arcs = arcs || piece.provenance.arcCentre.valid;
        }
    }
    return near;
}

/** No piece follows the source inside (@p t0, @p t1), where it stands still. */
bool skips(const LC_CurveOffsetGeometryResult& result, const double t0, const double t1) {
    for (const LC_OffsetBranch& branch : result.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            const LC_OffsetBranchProvenance& p = piece.provenance;
            if (!p.arcCentre.valid && std::max(p.sourceT0, p.sourceT1) > t0 && std::min(p.sourceT0, p.sourceT1) < t1) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

TEST_CASE("A cubic with a doubled control point offsets through it along the tangent's limit",
          "[curve-offset][direct][singular]") {
    // Two Bezier pieces meet at (3, 1), and the second one's first handle is
    // doubled onto the join: the tangent vanishes there, but tends to (1, 0)
    // from both sides, so the curve is smooth. Past the join it bends right
    // ever more sharply, and on the right the offset turns back at the join.
    const RS_Spline doubled = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {3, 1}, {4, 1}, {5, 0}},
                                         {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    for (const double d : {0.25, 1.0}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            INFO("distance " << d << " side " << static_cast<int>(side));
            const LC_CurveOffsetGeometryResult result = offsetToSide(doubled, side, d);
            REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(doubled, d);
            CHECK(maxDeviation(result, evaluator(doubled), options.tolerance.nodeMerge) <=
                  options.tolerance.requestedGeometry);
            // through the join's offset along the limit's normal (0, 1), with no corner
            const double offset = (side == LC_CurveOffsetSide::Left) ? d : -d;
            bool arcs = false;
            CHECK(endsNear(result, RS_Vector{3.0, 1.0 + offset}, options.tolerance.requestedGeometry, arcs));
            CHECK_FALSE(arcs);
            CHECK(result.branches.size() == (side == LC_CurveOffsetSide::Left ? 1u : 3u));
        }
    }
    CHECK(materialize(doubled, LC_CurveOffsetSide::Right, 0.25).status == LC_CurveOffsetStatus::Ok);

    // With simple knots a doubled control point only slows the curve down: its
    // tangent never vanishes.
    const RS_Spline slowed = makeSpline(3, {{0, 0}, {2, 2}, {4, 0}, {4, 0}, {6, 2}, {8, 0}},
                                        {0, 0, 0, 0, 1, 2, 3, 3, 3, 3});
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        const LC_CurveOffsetGeometryResult result = offsetToSide(slowed, side, 0.5);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        CHECK(maxDeviation(result, evaluator(slowed)) <=
              LC_CurveOffset::makeDirectOptions(slowed, 0.5).tolerance.requestedGeometry);
    }
}

TEST_CASE("Where a vanishing tangent tends to different directions, the source turns a corner there",
          "[curve-offset][direct][singular][kink]") {
    // The doubled handle leaves (3, 1) towards (4, 2): the curve arrives along
    // (1, 0) and leaves along (1, 1), turning left by 45 degrees. As at any
    // corner, an arc of radius d about it rounds the right side, and the two
    // sides' offsets overlap on the left.
    const RS_Spline cornered = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 1}, {3, 1}, {4, 2}, {5, 1}},
                                          {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    const double d = 0.25;
    const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(cornered, d);
    const LC_CurveOffsetGeometryResult right = offsetToSide(cornered, LC_CurveOffsetSide::Right, d);
    REQUIRE(right.status == LC_CurveOffsetStatus::Ok);
    CHECK(maxDeviation(right, evaluator(cornered), options.tolerance.nodeMerge) <=
          options.tolerance.requestedGeometry);
    int arcPieces = 0;
    for (const LC_OffsetBranch& branch : right.branches) {
        for (const LC_OffsetCubicPiece& piece : branch.cubicPieces) {
            if (piece.provenance.arcCentre.valid) {
                ++arcPieces;
                CHECK(piece.provenance.arcCentre.distanceTo(RS_Vector{3.0, 1.0}) <= options.tolerance.nodeMerge);
            }
        }
    }
    CHECK(arcPieces > 0);
    bool arcs = false;
    CHECK(endsNear(right, RS_Vector{3.0, 1.0 - d}, options.tolerance.requestedGeometry, arcs));
    CHECK(endsNear(right, RS_Vector{3.0, 1.0} + RS_Vector{1.0, -1.0} * (d / std::sqrt(2.0)),
                   options.tolerance.requestedGeometry, arcs));

    const LC_CurveOffsetGeometryResult left = offsetToSide(cornered, LC_CurveOffsetSide::Left, d);
    REQUIRE(left.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(left.branches.size() == 2);
    CHECK(left.branches[0].endEnd == LC_OffsetBranchEnd::Kink);
    CHECK(left.branches[1].startEnd == LC_OffsetBranchEnd::Kink);
    CHECK(maxDeviation(left, evaluator(cornered), options.tolerance.nodeMerge) <= options.tolerance.requestedGeometry);
    CHECK(materialize(cornered, LC_CurveOffsetSide::Right, d).status == LC_CurveOffsetStatus::Ok);

    // A cusp, where the tangent vanishes inside a span and turns back: a
    // half turn, which turns away from either side, as a polyline's does.
    const RS_Spline cusp = makeSpline(3, {{0, 0}, {2, 1}, {0, 1}, {2, 0}}, {0, 0, 0, 0, 1, 1, 1, 1});
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        INFO("side " << static_cast<int>(side));
        const LC_CurveOffsetGeometryResult result = offsetToSide(cusp, side, 0.1);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        const LC_CurveOffsetOptions cuspOptions = LC_CurveOffset::makeDirectOptions(cusp, 0.1);
        CHECK(maxDeviation(result, evaluator(cusp), cuspOptions.tolerance.nodeMerge) <=
              cuspOptions.tolerance.requestedGeometry);
        // over the top of the cusp at (1, 0.75)
        CHECK(endsNear(result, RS_Vector{1.0, 0.85}, 1e-3, arcs) == false);
        CHECK(arcs);
    }
}

TEST_CASE("A repeated vertex of a polyline spline changes nothing", "[curve-offset][direct][singular]") {
    // Between the two copies of (5, 0) the spline stands still. Without that
    // span it is the polyline through the three points, which turns left.
    const RS_Spline repeated = makeSpline(1, {{0, 0}, {5, 0}, {5, 0}, {5, 5}}, {0, 0, 1, 2, 3, 3});
    const RS_Spline plain = makeSpline(1, {{0, 0}, {5, 0}, {5, 5}}, {0, 0, 1, 2, 2});
    for (const double d : {0.25, 1.0}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            INFO("distance " << d << " side " << static_cast<int>(side));
            const LC_CurveOffsetGeometryResult a = offsetToSide(repeated, side, d);
            const LC_CurveOffsetGeometryResult b = offsetToSide(plain, side, d);
            REQUIRE(a.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(b.status == LC_CurveOffsetStatus::Ok);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(repeated, d);
            CHECK(maxDeviation(a, evaluator(repeated)) <= options.tolerance.requestedGeometry);
            CHECK(skips(a, 1.0, 2.0));
            // the same branches, corners and ends
            REQUIRE(a.branches.size() == b.branches.size());
            for (size_t i = 0; i < a.branches.size(); ++i) {
                const std::vector<LC_OffsetCubicPiece>& pa = a.branches[i].cubicPieces;
                const std::vector<LC_OffsetCubicPiece>& pb = b.branches[i].cubicPieces;
                CHECK(a.branches[i].startEnd == b.branches[i].startEnd);
                CHECK(a.branches[i].endEnd == b.branches[i].endEnd);
                REQUIRE(pa.size() == pb.size());
                for (size_t k = 0; k < pa.size(); ++k) {
                    for (size_t j = 0; j < 4; ++j) {
                        CHECK(pa[k].bezier[j].distanceTo(pb[k].bezier[j]) <= 1e-12);
                    }
                }
            }
        }
    }
    // a point just outside the corner, nearest to it, is on its right
    const LC_OffsetSideResolution side =
        LC_CurveOffset::resolveSide(repeated, RS_Vector{6.0, -1.0}, LC_CurveOffset::makeDirectOptions(repeated, 1.0));
    REQUIRE(side.status == LC_CurveOffsetStatus::Ok);
    CHECK(side.side == LC_CurveOffsetSide::Right);
    CHECK(materialize(repeated, LC_CurveOffsetSide::Right, 1.0).status == LC_CurveOffsetStatus::Ok);

    // along a straight line, the repeated vertex leaves one straight offset
    const RS_Spline straight = makeSpline(1, {{0, 0}, {5, 0}, {5, 0}, {10, 0}}, {0, 0, 1, 2, 3, 3});
    const LC_CurveOffsetGeometryResult line = offsetToSide(straight, LC_CurveOffsetSide::Left, 1.0);
    REQUIRE(line.status == LC_CurveOffsetStatus::Ok);
    REQUIRE(line.branches.size() == 1);
    CHECK(line.branches.front().cubicPieces.front().bezier[0] == RS_Vector(0.0, 1.0));
    CHECK(line.branches.front().cubicPieces.back().bezier[3] == RS_Vector(10.0, 1.0));
    CHECK(maxDeviation(line, evaluator(straight)) <= 1e-12);
}

TEST_CASE("A knot interval over which a spline stands still is skipped", "[curve-offset][direct][singular]") {
    // Three Bezier pieces, the middle one the point (3, 0): its four control
    // points coincide. The curve leaves it along (1, -1), the direction it
    // arrives in, just as it passes (3, 0) without the middle piece.
    const RS_Spline still =
        makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 0}, {3, 0}, {3, 0}, {3, 0}, {4, -1}, {5, -1}, {6, 0}},
                   {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3});
    const RS_Spline without = makeSpline(3, {{0, 0}, {1, 1}, {2, 1}, {3, 0}, {4, -1}, {5, -1}, {6, 0}},
                                         {0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2});
    for (const double d : {0.25, 1.0}) {
        for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
            INFO("distance " << d << " side " << static_cast<int>(side));
            const LC_CurveOffsetGeometryResult a = offsetToSide(still, side, d);
            const LC_CurveOffsetGeometryResult b = offsetToSide(without, side, d);
            REQUIRE(a.status == LC_CurveOffsetStatus::Ok);
            REQUIRE(b.status == LC_CurveOffsetStatus::Ok);
            const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(still, d);
            CHECK(maxDeviation(a, evaluator(still), options.tolerance.nodeMerge) <=
                  options.tolerance.requestedGeometry);
            CHECK(skips(a, 1.0, 2.0));
            REQUIRE(a.branches.size() == b.branches.size());
            CHECK(a.branches.front().cubicPieces.front().bezier[0] == b.branches.front().cubicPieces.front().bezier[0]);
            CHECK(a.branches.back().cubicPieces.back().bezier[3] == b.branches.back().cubicPieces.back().bezier[3]);
            bool arcs = false;
            CHECK(endsNear(a, RS_Vector{3.0, 0.0} + RS_Vector{1.0, 1.0} * ((side == LC_CurveOffsetSide::Left ? d : -d) /
                                                                          std::sqrt(2.0)),
                           options.tolerance.requestedGeometry, arcs));
            CHECK_FALSE(arcs);
        }
    }

    // A uniform cubic whose middle four control points coincide stands still
    // over the knot interval [2, 3], and runs straight into it along (1, -1)
    // and out of it along (1, 1), its tangent vanishing either side: a corner.
    const RS_Spline uniform = makeSpline(3, {{0, 0}, {2, 2}, {4, 0}, {4, 0}, {4, 0}, {4, 0}, {6, 2}, {8, 0}},
                                         {0, 0, 0, 0, 1, 2, 3, 4, 5, 5, 5, 5});
    for (const LC_CurveOffsetSide side : {LC_CurveOffsetSide::Left, LC_CurveOffsetSide::Right}) {
        INFO("side " << static_cast<int>(side));
        const LC_CurveOffsetGeometryResult result = offsetToSide(uniform, side, 0.5);
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeDirectOptions(uniform, 0.5);
        CHECK(maxDeviation(result, evaluator(uniform), options.tolerance.nodeMerge) <=
              options.tolerance.requestedGeometry);
        CHECK(skips(result, 2.0, 3.0));
        bool arcs = false;
        endsNear(result, RS_Vector{}, 0.0, arcs);
        CHECK(arcs == (side == LC_CurveOffsetSide::Right)); // rounded outside the left turn
        CHECK(result.branches.size() == (side == LC_CurveOffsetSide::Right ? 1u : 2u));
    }
    CHECK(materialize(uniform, LC_CurveOffsetSide::Right, 0.5).status == LC_CurveOffsetStatus::Ok);
}

namespace {
/** The least radius of curvature of @p s over the source stretch [lo, hi]. */
double minRadiusOfCurvature(const LC_SplinePoints& s, const double lo, const double hi, const int samples) {
    double best = RS_MAXDOUBLE;
    for (int i = 0; i <= samples; ++i) {
        const double t = lo + (hi - lo) * i / samples;
        LC_CurveJet j;
        if (!s.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, j)) {
            continue;
        }
        const double speed = j.first.magnitude();
        const double turn = std::abs(j.first.x * j.second.y - j.first.y * j.second.x);
        if (turn > 0.0) {
            best = std::min(best, speed * speed * speed / turn);
        }
    }
    return best;
}
} // namespace

TEST_CASE("Three or more corners trimmed past their cusp distance in the same offset stay symmetric",
          "[curve-offset][cusp][closed]") {
    // removeLocalLoops resolves each trimmed corner's reversed loop in turn; a
    // closed source needing only one or two corners trimmed (an ellipse's two
    // vertices, say) is well covered elsewhere, but a source needing three or
    // more forces the loop to fall back, more than once in a row, on the
    // narrower "ring" resolution (the previous corner's own branches, merged
    // into one, aliased as both neighbours of the next). A regular n-gon spun
    // out of a closed LC_SplinePoints, offset past every vertex's own radius of
    // curvature at once, is n-fold symmetric by construction; nothing in how
    // removeLocalLoops walks and merges its branch list (in array order, not
    // geometric order) is symmetric, so a wrongly-bounded search that only
    // happens to work for one or two corners would show up here as a corner
    // that lands somewhere other than its expected rotation of the others.
    for (const int n : {3, 4, 5, 6, 7}) {
        INFO("n " << n);
        const double R = 20.0;
        std::vector<RS_Vector> controls;
        for (int i = 0; i < n; ++i) {
            const double angle = 2.0 * M_PI * i / n;
            controls.push_back(RS_Vector{R * std::cos(angle), R * std::sin(angle)});
        }
        const LC_SplinePoints polygon = fromControlPoints(controls, true);
        double tightest = RS_MAXDOUBLE;
        for (int i = 0; i < n; ++i) {
            tightest = std::min(tightest, minRadiusOfCurvature(polygon, i - 0.45, i + 0.45, 2000));
        }
        // comfortably past every vertex's own cusp distance, well short of
        // where the whole shape shrinks away
        const double d = tightest * 1.05;
        INFO("d " << d << " tightest " << tightest);
        const LC_CurveOffsetOptions options = LC_CurveOffset::makeOffsetOptions(polygon, d);
        const LC_CurveOffsetGeometryResult result = LC_CurveOffset::buildDirectBranches(
            polygon, LC_CurveOffset::makeDirectionRequest(RS_Vector{0.0, 0.0}, d), options,
            LC_CurveOffset::makeDirectSourceBudget());
        REQUIRE(result.status == LC_CurveOffsetStatus::Ok);
        // every one of the n loops removed: (n - 1) ordinary merges and, forced
        // by the last one, a single ring resolution, three pieces each
        CHECK(result.removedIntervals == static_cast<std::size_t>(3 * n));
        REQUIRE(result.branches.size() == 1);
        const LC_OffsetBranch& branch = result.branches.front();
        CHECK(branch.closed);
        // the corners: consecutive pieces whose source parameters do not run
        // on, save for the closed source's own seam (t = n wrapping to t = 0,
        // the same physical point, not a corner)
        std::vector<RS_Vector> corners;
        const size_t m = branch.cubicPieces.size();
        for (size_t k = 0; k < m; ++k) {
            const LC_OffsetCubicPiece& piece = branch.cubicPieces[k];
            const LC_OffsetCubicPiece& next = branch.cubicPieces[(k + 1) % m];
            const bool seam =
                piece.provenance.sourceT1 == static_cast<double>(n) && next.provenance.sourceT0 == 0.0;
            if (piece.provenance.sourceT1 != next.provenance.sourceT0 && !seam) {
                corners.push_back(piece.bezier[3]);
            }
        }
        REQUIRE(corners.size() == static_cast<size_t>(n));
        // every corner, rotated by one n-th of a turn, lands on another
        const double step = 2.0 * M_PI / n;
        for (const RS_Vector& c : corners) {
            const RS_Vector rotated = c.rotated(step);
            double nearest = RS_MAXDOUBLE;
            for (const RS_Vector& other : corners) {
                nearest = std::min(nearest, rotated.distanceTo(other));
            }
            CHECK(nearest < 1e-6 * R);
        }
    }
}
