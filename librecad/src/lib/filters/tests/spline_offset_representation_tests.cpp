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

// D0 representation spike for spline offsets: which entity form stores a
// validated cubic offset branch so that what is written, drawn and read back is
// still that branch. Each experiment below is evidence for the D1 decision:
//
//  - One clamped cubic RS_Spline per Bezier piece, with knots {0,0,0,0,1,1,1,1},
//    stores the piece exactly, draws it with 32 segments of its own, and
//    survives DXF and DWG unchanged. This is the D1 output form.
//  - A C1 composite of several pieces is exact too, but RS_Spline draws (and
//    hit-tests) any entity with 32 segments in all, which cannot follow many
//    pieces; the importer also rounds knots, so knots at source parameters
//    would move. Not used in D1.
//  - LC_SplinePoints is quadratic: it cannot hold a cubic piece, and its fit
//    mode has no parameter correspondence with the branch. Not an output form.
//  - A closed source's offset is a chain of open pieces whose last point is the
//    first; it has no closed or periodic flag to get wrong, and the seam
//    survives a round trip. D1 therefore accepts closed sources, but only after
//    checking that the source's ends really meet: a wrapped spline with a
//    non-periodic knot vector passes validate() with a gap at its seam.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QCoreApplication>

#include "lc_splinepoints.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_spline.h"

namespace {

void ensureSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char* argv[] = {arg0, nullptr};
    static QCoreApplication* app =
        QCoreApplication::instance() ? QCoreApplication::instance() : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

using Bezier = std::array<RS_Vector, 4>;

RS_Vector bezierAt(const Bezier& b, const double t) {
    const double s = 1.0 - t;
    return b[0] * (s * s * s) + b[1] * (3.0 * s * s * t) + b[2] * (3.0 * s * t * t) + b[3] * (t * t * t);
}

RS_Vector bezierDerivative(const Bezier& b, const double t) {
    const double s = 1.0 - t;
    return ((b[1] - b[0]) * (s * s) + (b[2] - b[1]) * (2.0 * s * t) + (b[3] - b[2]) * (t * t)) * 3.0;
}

/** One clamped cubic spline holding exactly this Bezier piece on [0, 1]. */
RS_SplineData pieceData(const Bezier& b) {
    RS_SplineData d(3, false);
    d.controlPoints.assign(b.begin(), b.end());
    d.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    d.weights.assign(4, 1.0);
    return d;
}

/** One spline holding consecutive pieces, piece i on [i, i+1]: interior knots of
 *  multiplicity 3 make the control net the pieces' nets with shared ends. */
RS_SplineData compositeData(const std::vector<Bezier>& pieces) {
    RS_SplineData d(3, false);
    d.controlPoints.push_back(pieces.front()[0]);
    d.knotslist = {0, 0, 0, 0};
    for (size_t i = 0; i < pieces.size(); ++i) {
        d.controlPoints.insert(d.controlPoints.end(), pieces[i].begin() + 1, pieces[i].end());
        const auto end = static_cast<double>(i + 1);
        d.knotslist.insert(d.knotslist.end(), (i + 1 == pieces.size()) ? 4 : 3, end);
    }
    d.weights.assign(d.controlPoints.size(), 1.0);
    return d;
}

/** Hermite cubic pieces of a circular arc: exact point and tangent at the piece ends. */
std::vector<Bezier> arcPieces(const RS_Vector& center, const double radius, const double a0, const double a1,
                              const int count) {
    std::vector<Bezier> pieces;
    const double step = (a1 - a0) / count;
    const double handle = 4.0 / 3.0 * std::tan(step / 4.0) * radius;
    for (int i = 0; i < count; ++i) {
        const double s = a0 + step * i;
        const double e = s + step;
        const RS_Vector p0 = center + RS_Vector{std::cos(s), std::sin(s)} * radius;
        const RS_Vector p3 = center + RS_Vector{std::cos(e), std::sin(e)} * radius;
        const RS_Vector t0{-std::sin(s), std::cos(s)};
        const RS_Vector t1{-std::sin(e), std::cos(e)};
        pieces.push_back({p0, p0 + t0 * handle, p3 - t1 * handle, p3});
    }
    // pin shared ends exactly, as the D1 materializer does
    for (size_t i = 1; i < pieces.size(); ++i) {
        pieces[i][0] = pieces[i - 1][3];
    }
    return pieces;
}

double distanceToSegment(const RS_Vector& p, const RS_Vector& a, const RS_Vector& b) {
    const RS_Vector ab = b - a;
    const double len2 = ab.squared();
    const double t = len2 > 0.0 ? std::clamp(RS_Vector::dotP(p - a, ab) / len2, 0.0, 1.0) : 0.0;
    return p.distanceTo(a + ab * t);
}

/** How far the lines update() draws the spline with stray from the exact curve. */
double displayDeviation(const RS_Spline& spline, const std::function<RS_Vector(double)>& exact, const double t0,
                        const double t1) {
    std::vector<RS_Line*> lines;
    for (RS_Entity* e : spline) {
        if (auto* line = dynamic_cast<RS_Line*>(e)) {
            lines.push_back(line);
        }
    }
    REQUIRE_FALSE(lines.empty());
    double worst = 0.0;
    for (int i = 0; i <= 2000; ++i) {
        const RS_Vector p = exact(t0 + (t1 - t0) * i / 2000.0);
        double nearest = RS_MAXDOUBLE;
        for (const RS_Line* line : lines) {
            nearest = std::min(nearest, distanceToSegment(p, line->getStartpoint(), line->getEndpoint()));
        }
        worst = std::max(worst, nearest);
    }
    return worst;
}

/** The splines of @p pieces written in @p format and read back, in order. */
std::vector<RS_SplineData> roundTrip(const std::vector<RS_SplineData>& pieces, const RS2::FormatType format,
                                     const char* suffix, bool& exported) {
    ensureSettings();
    const std::string path =
        (std::filesystem::temp_directory_path() / (std::string("spline_offset_repr_") + suffix)).string();
    {
        RS_Graphic graphic;
        for (const RS_SplineData& data : pieces) {
            graphic.addEntity(new RS_Spline(&graphic, data));
        }
        RS_FilterDXFRW filter;
        exported = filter.fileExport(graphic, QString::fromStdString(path), format);
    }
    std::vector<RS_SplineData> result;
    if (!exported) {
        return result;
    }
    RS_Graphic reloaded;
    RS_FilterDXFRW filter;
    const bool isDwg = format >= RS2::FormatDWG && format <= RS2::FormatDWG2018;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(path),
                              isDwg ? RS2::FormatDWG : RS2::FormatDXFRW));
    for (RS_Entity* e : reloaded) {
        if (auto* spline = dynamic_cast<RS_Spline*>(e)) {
            result.push_back(spline->getData());
        }
        else {
            FAIL("a stored piece came back as entity type " << e->rtti());
        }
    }
    std::filesystem::remove(path);
    return result;
}

LC_CurveJet jetAt(const RS_Spline& spline, const double t, const LC_CurveEvaluationSide side) {
    LC_CurveJet jet;
    REQUIRE(spline.tryEvaluateJet(t, side, jet));
    return jet;
}

// a cubic piece with an inflection, at coordinates that are not small integers
const Bezier g_piece{RS_Vector{1234.5, -87.25}, RS_Vector{1301.125, 12.5}, RS_Vector{1377.0, -140.75},
                     RS_Vector{1450.25, -30.0}};

} // namespace

TEST_CASE("A cubic Bezier piece is stored exactly as a clamped cubic RS_Spline",
          "[curve-offset][d0][representation]") {
    ensureSettings();
    RS_Graphic graphic;
    auto* spline = new RS_Spline(&graphic, pieceData(g_piece));
    graphic.addEntity(spline);
    REQUIRE(spline->validate());

    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline->getParameterDomain(t0, t1));
    CHECK(t0 == 0.0);
    CHECK(t1 == 1.0);
    for (int i = 0; i <= 100; ++i) {
        const double t = i / 100.0;
        const LC_CurveJet jet = jetAt(*spline, t, LC_CurveEvaluationSide::Interior);
        CHECK(jet.point.distanceTo(bezierAt(g_piece, t)) < 1e-10);
        CHECK(jet.first.distanceTo(bezierDerivative(g_piece, t)) < 1e-9);
    }
    CHECK(spline->getStartpoint().distanceTo(g_piece[0]) < 1e-12);
    CHECK(spline->getEndpoint().distanceTo(g_piece[3]) < 1e-12);

    // drawn with 32 segments of its own; even this sharply inflected piece,
    // about 216 units wide, is drawn within 1e-3 of its extent, and a fitted
    // offset piece turns far less
    const double extent = 216.0;
    CHECK(spline->count() == 32);
    CHECK(displayDeviation(*spline, [](double t) { return bezierAt(g_piece, t); }, 0.0, 1.0) < 1e-3 * extent);
    // its borders enclose the curve
    for (int i = 0; i <= 50; ++i) {
        const RS_Vector p = bezierAt(g_piece, i / 50.0);
        CHECK(p.x >= spline->getMin().x - 1e-9);
        CHECK(p.x <= spline->getMax().x + 1e-9);
        CHECK(p.y >= spline->getMin().y - 1e-9);
        CHECK(p.y <= spline->getMax().y + 1e-9);
    }
    // it is hit where the curve is
    double dist = 0.0;
    spline->getNearestPointOnEntity(bezierAt(g_piece, 0.37), true, &dist);
    CHECK(dist < 1e-3 * extent);
}

TEST_CASE("Two C1 pieces join exactly into one composite spline", "[curve-offset][d0][representation]") {
    const std::vector<Bezier> pieces = arcPieces(RS_Vector{10.0, -5.0}, 50.0, 0.3, 1.9, 2);
    const RS_Spline composite(nullptr, compositeData(pieces));
    REQUIRE(composite.validate());
    CHECK(composite.getBreakParameters() == std::vector<double>{0.0, 1.0, 2.0});
    for (int i = 0; i <= 50; ++i) {
        const double u = i / 50.0;
        CHECK(jetAt(composite, u, LC_CurveEvaluationSide::Interior).point.distanceTo(bezierAt(pieces[0], u)) < 1e-10);
        CHECK(jetAt(composite, 1.0 + u, LC_CurveEvaluationSide::Left).point.distanceTo(bezierAt(pieces[1], u)) <
              1e-10);
    }
    // C1 at the join: both one-sided tangents are the pieces' shared tangent
    const RS_Vector left = jetAt(composite, 1.0, LC_CurveEvaluationSide::Left).first;
    const RS_Vector right = jetAt(composite, 1.0, LC_CurveEvaluationSide::Right).first;
    CHECK(left.distanceTo(right) < 1e-9);
    CHECK(left.distanceTo(bezierDerivative(pieces[0], 1.0)) < 1e-9);
}

TEST_CASE("A composite's 32 drawn segments cannot follow many pieces; single pieces can",
          "[curve-offset][d0][representation]") {
    // A half circle of radius 100 in 40 pieces, the order of an offset fitted to a
    // tight tolerance.
    const double radius = 100.0;
    const std::vector<Bezier> pieces = arcPieces(RS_Vector{0.0, 0.0}, radius, 0.0, M_PI, 40);
    const RS_Spline composite(nullptr, compositeData(pieces));
    REQUIRE(composite.validate());
    const double compositeDeviation = displayDeviation(
        composite,
        [&pieces](double t) {
            const auto i = std::min<size_t>(static_cast<size_t>(t), pieces.size() - 1);
            return bezierAt(pieces[i], t - static_cast<double>(i));
        },
        0.0, static_cast<double>(pieces.size()));

    double pieceDeviation = 0.0;
    for (const Bezier& piece : pieces) {
        const RS_Spline single(nullptr, pieceData(piece));
        pieceDeviation = std::max(
            pieceDeviation, displayDeviation(single, [&piece](double t) { return bezierAt(piece, t); }, 0.0, 1.0));
    }
    INFO("composite deviation " << compositeDeviation << ", per-piece deviation " << pieceDeviation);
    // 32 chords over the half circle deviate by r (1 - cos(pi / 64)), about 0.12;
    // one piece per entity keeps every chord short.
    CHECK(compositeDeviation > 0.1);
    CHECK(pieceDeviation < 1e-4);
}

TEST_CASE("Single cubic pieces survive DXF and DWG unchanged", "[curve-offset][d0][representation]") {
    const std::vector<Bezier> pieces = arcPieces(RS_Vector{1234.5, -87.25}, 321.0, -0.4, 2.2, 5);
    std::vector<RS_SplineData> stored;
    for (const Bezier& piece : pieces) {
        stored.push_back(pieceData(piece));
    }
    stored.push_back(pieceData(g_piece));

    struct Format {
        RS2::FormatType type;
        const char* file;
    };
    for (const Format& format : {Format{RS2::FormatDXFRW, "pieces.dxf"}, Format{RS2::FormatDXFRW2000, "pieces_2000.dxf"},
                                 Format{RS2::FormatDWG, "pieces_r2000.dwg"},
                                 Format{RS2::FormatDWG2018, "pieces_r2018.dwg"}}) {
        DYNAMIC_SECTION("format " << format.file) {
            bool exported = false;
            const std::vector<RS_SplineData> reloaded = roundTrip(stored, format.type, format.file, exported);
            if (!exported) {
                SKIP("writing " << format.file << " is not supported in this build");
            }
            REQUIRE(reloaded.size() == stored.size());
            for (size_t i = 0; i < stored.size(); ++i) {
                const RS_SplineData& a = stored[i];
                const RS_SplineData& b = reloaded[i];
                CHECK(b.degree == 3);
                CHECK(b.type == RS_SplineData::SplineType::ClampedOpen);
                REQUIRE(b.controlPoints.size() == 4);
                CHECK(b.knotslist == a.knotslist);
                CHECK(b.weights == a.weights);
                const RS_Spline before(nullptr, a);
                const RS_Spline after(nullptr, b);
                const double scale = std::max(std::abs(a.controlPoints[0].x), std::abs(a.controlPoints[0].y));
                for (size_t k = 0; k < 4; ++k) {
                    CHECK(b.controlPoints[k].distanceTo(a.controlPoints[k]) <= 1e-12 * scale);
                    CHECK(b.controlPoints[k].z == 0.0);
                }
                for (int s = 0; s <= 20; ++s) {
                    const double t = s / 20.0;
                    const LC_CurveJet x = jetAt(before, t, LC_CurveEvaluationSide::Interior);
                    const LC_CurveJet y = jetAt(after, t, LC_CurveEvaluationSide::Interior);
                    CHECK(y.point.distanceTo(x.point) <= 1e-12 * scale);
                    CHECK(y.first.distanceTo(x.first) <= 1e-11 * scale);
                }
            }
        }
    }
}

TEST_CASE("An LC_SplinePoints cannot hold a cubic offset piece", "[curve-offset][d0][representation]") {
    // Fit mode through samples of the piece: a quadratic interpolant with no
    // parameter correspondence and an error that nothing bounds.
    LC_SplinePointsData fit(false, false);
    for (int i = 0; i <= 8; ++i) {
        fit.splinePoints.push_back(bezierAt(g_piece, i / 8.0));
    }
    const LC_SplinePoints fitted(nullptr, fit);
    const double scale = 200.0; // extent of the piece
    double fitError = 0.0;
    for (int i = 0; i <= 400; ++i) {
        double dist = 0.0;
        fitted.getNearestPointOnEntity(bezierAt(g_piece, i / 400.0), true, &dist);
        fitError = std::max(fitError, dist);
    }
    INFO("fit-mode error " << fitError);
    CHECK(fitError > 1e-6 * scale); // far above a 1e-6 relative tolerance

    // Its parameter is segment + u, unrelated to the piece's own parameter.
    LC_CurveJet jet;
    REQUIRE(fitted.tryEvaluateJet(0.5, LC_CurveEvaluationSide::Interior, jet));
    CHECK(jet.point.distanceTo(bezierAt(g_piece, 0.5)) > 1e-3);
}

TEST_CASE("A closed source's offset is a chain of open pieces whose seam survives a round trip",
          "[curve-offset][d0][representation]") {
    // A closed cubic built the way the drawing tools build one has uniform
    // knots: its curve and tangent meet across the seam, so its offset closes.
    RS_Spline source(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        source.addControlPoint(p);
    }
    source.setClosed(true);
    REQUIRE(source.isClosed());
    REQUIRE(source.validate());
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(source.getParameterDomain(t0, t1));
    const LC_CurveJet start = jetAt(source, t0, LC_CurveEvaluationSide::Right);
    const LC_CurveJet end = jetAt(source, t1, LC_CurveEvaluationSide::Left);
    CHECK(start.point.distanceTo(end.point) < 1e-9);
    CHECK(start.first.distanceTo(end.first) < 1e-9);

    // But the closed flag alone proves nothing: a wrapped spline with a
    // non-periodic knot vector validates, yet its ends do not meet. The offset
    // engine must check the seam of a closed source itself.
    RS_SplineData wrapped(3, true);
    wrapped.type = RS_SplineData::SplineType::WrappedClosed;
    wrapped.controlPoints = {{0, 0}, {15, 25}, {40, 35}, {70, 20}, {80, 0}, {0, 0}, {15, 25}, {40, 35}};
    wrapped.weights.assign(wrapped.controlPoints.size(), 1.0);
    wrapped.knotslist = {0.0, 12.0, 35.0, 60.0, 100.0, 140.0, 180.0, 220.0, 260.0, 290.0, 320.0, 350.0};
    const RS_Spline nonPeriodic(nullptr, wrapped);
    REQUIRE(nonPeriodic.validate());
    REQUIRE(nonPeriodic.getParameterDomain(t0, t1));
    CHECK(jetAt(nonPeriodic, t0, LC_CurveEvaluationSide::Right)
              .point.distanceTo(jetAt(nonPeriodic, t1, LC_CurveEvaluationSide::Left).point) > 1.0);

    // A closed chain of open pieces: the last piece ends at the first piece's start.
    std::vector<Bezier> pieces = arcPieces(RS_Vector{500.0, 250.0}, 75.0, 0.0, 2.0 * M_PI, 7);
    pieces.back()[3] = pieces.front()[0];
    std::vector<RS_SplineData> chain;
    for (const Bezier& piece : pieces) {
        chain.push_back(pieceData(piece));
    }
    bool exported = false;
    const std::vector<RS_SplineData> reloaded = roundTrip(chain, RS2::FormatDXFRW, "closed_chain.dxf", exported);
    REQUIRE(exported);
    REQUIRE(reloaded.size() == chain.size());
    for (const RS_SplineData& piece : reloaded) {
        CHECK_FALSE(RS_Spline(nullptr, piece).isClosed()); // no closed or periodic flag on a piece
    }
    const RS_Spline first(nullptr, reloaded.front());
    const RS_Spline last(nullptr, reloaded.back());
    CHECK(last.getEndpoint().distanceTo(first.getStartpoint()) < 1e-9);

    // A closed LC_SplinePoints source joins its last segment to its first with C1 continuity.
    LC_SplinePointsData ring(true, false);
    ring.useControlPoints = true;
    ring.controlPoints = {{0, 0}, {10, 0}, {12, 8}, {2, 9}};
    const LC_SplinePoints closedPoints(nullptr, ring);
    LC_CurveJet ringStart;
    LC_CurveJet ringEnd;
    const auto count = static_cast<double>(closedPoints.getSegmentCount());
    REQUIRE(closedPoints.tryEvaluateJet(0.0, LC_CurveEvaluationSide::Right, ringStart));
    REQUIRE(closedPoints.tryEvaluateJet(count, LC_CurveEvaluationSide::Left, ringEnd));
    CHECK(ringStart.point.distanceTo(ringEnd.point) < 1e-12);
    CHECK(ringStart.first.distanceTo(ringEnd.first) < 1e-12);
}
