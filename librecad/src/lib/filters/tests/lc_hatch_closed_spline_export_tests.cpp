/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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

// Saving a hatch whose boundary is a closed RS_Spline.
//
// A closed RS_Spline repeats its first degree control points and weights at
// the end of its data, with a knot vector to match. The hatch writer took the
// control points without the repeats (getControlPoints()) but the whole knot
// vector, so the edge had degree knots too many. libdxfrw refused it, and the
// DXF save failed. The DWG writer does not check the count: it wrote the edge,
// and on reopening a cubic one no longer made a spline, so the hatch was gone.
//
// The edge is now the stored data, repeats included. That data draws the
// closed curve over [knots[degree], knots[count]] for any NURBS reader, which
// the ODA File Converter confirms; without the repeats only a reader that
// wraps periodic splines itself, as LibreCAD does, would close it.

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "lc_containertraverser.h"
#include "lc_filtertestsupport.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_spline.h"

namespace {

struct SplineCase {
    int degree = 3;
    std::vector<double> weights; ///< empty: not rational
};

/// A closed spline whose control points are the corners of a 60 x 40 box.
RS_Spline* closedSpline(RS_EntityContainer* parent, const SplineCase& c) {
    auto* spline = new RS_Spline(parent, RS_SplineData(c.degree, false));
    const std::vector<RS_Vector> corners{{0.0, 0.0}, {60.0, 0.0}, {60.0, 40.0}, {0.0, 40.0}};
    for (std::size_t i = 0; i < corners.size(); ++i) {
        spline->addControlPoint(corners[i], c.weights.empty() ? 1.0 : c.weights[i]);
    }
    spline->setClosed(true);
    spline->update();
    return spline;
}

/// A drawing with one SOLID hatch bounded by that closed spline.
std::unique_ptr<RS_Graphic> drawingWithClosedSplineHatch(const SplineCase& c) {
    auto graphic = std::make_unique<RS_Graphic>();
    graphic->initForNewDocument();
    auto* hatch = new RS_Hatch(graphic.get(), RS_HatchData(true, 1.0, 0.0, QStringLiteral("SOLID")));
    auto* loop = new RS_EntityContainer(hatch);
    loop->addEntity(closedSpline(loop, c));
    hatch->addEntity(loop);
    graphic->addEntity(hatch);
    hatch->update();
    return graphic;
}

double polygonLength(const std::vector<RS_Vector>& points) {
    double length = 0.0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        length += points[i].distanceTo(points[(i + 1) % points.size()]);
    }
    return length;
}

double polygonArea(const std::vector<RS_Vector>& points) {
    double twice = 0.0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const RS_Vector& a = points[i];
        const RS_Vector& b = points[(i + 1) % points.size()];
        twice += a.x * b.y - b.x * a.y;
    }
    return std::abs(twice) / 2.0;
}

/// The area inside the closed spline, from a fine sampling of the curve. The
/// RS_Spline hatch measures the lines it draws the curve with, which lie a
/// little inside it, so the reopened hatch is compared with this.
double curveArea(const SplineCase& c) {
    const std::unique_ptr<RS_Spline> spline{closedSpline(nullptr, c)};
    std::vector<RS_Vector> points;
    spline->fillStrokePoints(16384, points);
    return polygonArea(points);
}

/// The length of the closed spline, from the same sampling.
double curveLength(const SplineCase& c) {
    const std::unique_ptr<RS_Spline> spline{closedSpline(nullptr, c)};
    std::vector<RS_Vector> points;
    spline->fillStrokePoints(16384, points);
    return polygonLength(points);
}

/// The length of a hatch's boundary: a spur that encloses nothing changes it
/// while the area stays.
double boundaryLength(const RS_Hatch& hatch) {
    double length = 0.0;
    for (RS_Entity* loop : hatch) {
        if (loop == nullptr || !loop->isContainer() || loop->getFlag(RS2::FlagHatchChild)) {
            continue;
        }
        for (RS_Entity* edge : *static_cast<RS_EntityContainer*>(loop)) {
            if (edge != nullptr) {
                length += edge->getLength();
            }
        }
    }
    return length;
}

std::vector<RS_Hatch*> hatchesOf(RS_Graphic& graphic) {
    std::vector<RS_Hatch*> hatches;
    for (RS_Entity* e : lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
        if (e != nullptr && e->rtti() == RS2::EntityHatch) {
            hatches.push_back(static_cast<RS_Hatch*>(e));
        }
    }
    return hatches;
}

std::vector<double> numbers(const std::vector<std::string>& values) {
    std::vector<double> result;
    for (const std::string& v : values) {
        result.push_back(std::stod(v));
    }
    return result;
}

const std::vector<SplineCase> kCases{
    {2, {}},
    {3, {}},
    {3, {1.0, 2.5, 0.5, 1.7}},
};

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("The closed spline areas the hatch checks compare with",
          "[hatch][spline]") {
    // The uniform periodic quadratic and cubic of the box enclose 2000 and
    // 4880/3: the sampling that curveArea() uses is exact enough for 1e-4.
    CHECK(std::abs(curveArea({2, {}}) - 2000.0) <= 1e-6 * 2000.0);
    CHECK(std::abs(curveArea({3, {}}) - 4880.0 / 3.0) <= 1e-6 * 4880.0 / 3.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A hatch bounded by a closed spline is saved to DXF and opens again",
          "[hatch][spline][dxf]") {
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());

    const std::size_t which = GENERATE(range(std::size_t{0}, kCases.size()));
    const SplineCase& splineCase = kCases[which];
    const RS2::FormatType format = GENERATE(RS2::FormatDXFRW2000, RS2::FormatDXFRW, RS2::FormatDXFRW2018);
    const int degree = splineCase.degree;
    const bool rational = !splineCase.weights.empty();
    INFO("degree " << degree << (rational ? ", rational" : "") << ", format " << static_cast<int>(format));

    auto graphic = drawingWithClosedSplineHatch(splineCase);
    const std::vector<RS_Hatch*> drawn = hatchesOf(*graphic);
    REQUIRE(drawn.size() == 1u);
    REQUIRE(drawn.front()->getUpdateError() == RS_Hatch::HATCH_OK);
    const double area = curveArea(splineCase);
    REQUIRE(std::abs(drawn.front()->getTotalArea() - area) <= 0.01 * area);

    const lc::test::OutputDirectory out("lc_hatch_closed_spline_dxf");
    const auto file = out.file("closed_spline_hatch.dxf");
    REQUIRE(lc::test::exportDrawing(*graphic, file, format));

    // One HATCH with one boundary path of one SPLINE edge (72 = 4): the four
    // corners and the degree repeats, and control points + degree + 1 knots.
    const auto records = lc::test::dxfRecords(file, "HATCH");
    REQUIRE(records.size() == 1u);
    const lc::test::DxfRecord& record = records.front();
    CHECK(record.own(91) == std::vector<std::string>{"1"});
    CHECK(record.own(72) == std::vector<std::string>{"4"});
    REQUIRE(record.own(95).size() == 1u);
    REQUIRE(record.own(96).size() == 1u);
    const std::size_t controls = std::stoul(record.own(96).front());
    REQUIRE(controls == 4u + degree);
    const std::vector<double> knots = numbers(record.own(40));
    REQUIRE(std::stoul(record.own(95).front()) == controls + degree + 1);
    REQUIRE(knots.size() == controls + degree + 1);
    const std::vector<double> xs = numbers(record.own(10));
    const std::vector<double> ys = numbers(record.own(20));
    // the HATCH's own elevation point comes first
    REQUIRE(xs.size() == controls + 1);
    REQUIRE(ys.size() == controls + 1);
    const std::vector<double> weights = numbers(record.own(42));
    CHECK(weights.size() == (rational ? controls : 0u));

    // Read as plain NURBS data, with no closed or periodic flag, the edge is
    // already the closed curve.
    RS_SplineData literal(degree, false);
    literal.type = RS_SplineData::SplineType::Standard;
    for (std::size_t i = 0; i < controls; ++i) {
        literal.controlPoints.emplace_back(xs[i + 1], ys[i + 1]);
        literal.weights.push_back(rational && i < weights.size() ? weights[i] : 1.0);
    }
    literal.knotslist = knots;
    RS_Spline asWritten(nullptr, literal);
    std::vector<RS_Vector> written;
    asWritten.fillStrokePoints(16384, written);
    REQUIRE(written.size() > 2u);
    CHECK(written.front().distanceTo(written.back()) <= 1e-9 * 60.0);
    CHECK(std::abs(polygonArea(written) - area) <= 1e-6 * area);

    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, file, RS2::FormatDXFRW));
    const std::vector<RS_Hatch*> read = hatchesOf(reopened);
    REQUIRE(read.size() == 1u);
    CHECK(read.front()->getUpdateError() == RS_Hatch::HATCH_OK);
    // The importer samples a cubic edge, so allow a little for the sampling.
    CHECK(std::abs(read.front()->getTotalArea() - area) <= 1e-4 * area);
    const double length = curveLength(splineCase);
    CHECK(std::abs(boundaryLength(*read.front()) - length) <= 1e-3 * length);
}

#ifdef DWGSUPPORT
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("A hatch bounded by a closed spline is saved to DWG and opens again",
          "[hatch][spline][dwg]") {
    // Before the fix the DWG file kept the edge with its knots too many: a
    // quadratic one reopened from its control points alone, a cubic one lost
    // its hatch.
    lc::test::QuietLog quiet;
    REQUIRE(lc::test::ensureSettings());

    const std::size_t which = GENERATE(range(std::size_t{0}, kCases.size()));
    const SplineCase& splineCase = kCases[which];
    const RS2::FormatType format = GENERATE(RS2::FormatDWG, RS2::FormatDWG2018);
    INFO("degree " << splineCase.degree << (splineCase.weights.empty() ? "" : ", rational") << ", format "
                   << static_cast<int>(format));

    auto graphic = drawingWithClosedSplineHatch(splineCase);
    const std::vector<RS_Hatch*> drawn = hatchesOf(*graphic);
    REQUIRE(drawn.size() == 1u);
    REQUIRE(drawn.front()->getUpdateError() == RS_Hatch::HATCH_OK);
    const double area = curveArea(splineCase);

    const lc::test::OutputDirectory out("lc_hatch_closed_spline_dwg");
    const auto file = out.file("closed_spline_hatch.dwg");
    REQUIRE(lc::test::exportDrawing(*graphic, file, format));

    RS_Graphic reopened;
    REQUIRE(lc::test::importDrawing(reopened, file, RS2::FormatDWG));
    const std::vector<RS_Hatch*> read = hatchesOf(reopened);
    REQUIRE(read.size() == 1u);
    CHECK(read.front()->getUpdateError() == RS_Hatch::HATCH_OK);
    CHECK(std::abs(read.front()->getTotalArea() - area) <= 1e-4 * area);
    const double length = curveLength(splineCase);
    CHECK(std::abs(boundaryLength(*read.front()) - length) <= 1e-3 * length);
}
#endif
