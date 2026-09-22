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

// Representation of spline offsets: which entity form stores a validated cubic
// offset branch so that what is written, drawn and read back is still that
// branch. The experiments below are the evidence:
//
//  - One clamped cubic RS_Spline per Bezier piece, with knots {0,0,0,0,1,1,1,1},
//    stores the piece exactly and survives DXF and DWG unchanged.
//  - Consecutive pieces join exactly into one clamped cubic RS_Spline whose
//    interior knots are the integers, each of multiplicity 3: piece i is the
//    span [i, i+1], and the control net is the pieces' nets with shared ends.
//    Integer knots survive the importer, which rounds knots, and RS_Spline
//    draws (and hit-tests) such a spline span by span within a display
//    tolerance, so many pieces draw as well as one. This is the output form:
//    one spline per offset chain.
//  - LC_SplinePoints is quadratic: it cannot hold a cubic piece, and its fit
//    mode has no parameter correspondence with the branch. Not an output form.
//  - A closed source's offset is one open spline whose last point is its first;
//    it has no closed or periodic flag to get wrong, and the seam survives a
//    round trip. Closed sources are therefore accepted, but only after checking
//    that their ends really meet: a wrapped spline with a non-periodic knot
//    vector passes validate() with a gap at its seam.
//
// The persistence gate follows: what the offset engine produces comes back
// from DXF and DWG as the curve it validated, and R12, which has no SPLINE,
// gets a polyline within its own export tolerance.

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include "lc_curveoffset.h"
#include "lc_makercamsvg.h"
#include "lc_splinepoints.h"
#include "lc_xmlwriterqxmlstreamwriter.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_polyline.h"
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
        std::filesystem::remove(path);
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

    // drawn with at least 32 segments of its own, more where it bends: even this
    // sharply inflected piece, about 216 units wide, is drawn within 1e-3 of its
    // extent, and a fitted offset piece turns far less
    const double extent = 216.0;
    CHECK(spline->count() >= 32);
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

TEST_CASE("A composite of many pieces is drawn span by span, within the display tolerance",
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
    // at least a chord per piece, where 32 chords over the whole half circle
    // would deviate by r (1 - cos(pi / 64)), about 0.12
    CHECK(composite.count() >= static_cast<unsigned>(pieces.size()));
    CHECK(compositeDeviation < 0.1);
    // within the display tolerance, 1e-3 of the control points' extent, which
    // is more than the diameter here
    CHECK(compositeDeviation <= 1e-3 * 2.0 * radius);
    CHECK(pieceDeviation < 1e-4);
}

TEST_CASE("Cubic pieces, and composites of them, survive DXF and DWG unchanged", "[curve-offset][d0][representation]") {
    const std::vector<Bezier> pieces = arcPieces(RS_Vector{1234.5, -87.25}, 321.0, -0.4, 2.2, 5);
    std::vector<RS_SplineData> stored;
    for (const Bezier& piece : pieces) {
        stored.push_back(pieceData(piece));
    }
    stored.push_back(pieceData(g_piece));
    stored.push_back(compositeData(pieces));

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
            REQUIRE(exported); // every format here is written by this build
            REQUIRE(reloaded.size() == stored.size());
            for (size_t i = 0; i < stored.size(); ++i) {
                const RS_SplineData& a = stored[i];
                const RS_SplineData& b = reloaded[i];
                CHECK(b.degree == 3);
                CHECK(b.type == RS_SplineData::SplineType::ClampedOpen);
                REQUIRE(b.controlPoints.size() == a.controlPoints.size());
                CHECK(b.knotslist == a.knotslist);
                CHECK(b.weights == a.weights);
                const RS_Spline before(nullptr, a);
                const RS_Spline after(nullptr, b);
                const double scale = std::max(std::abs(a.controlPoints[0].x), std::abs(a.controlPoints[0].y));
                for (size_t k = 0; k < a.controlPoints.size(); ++k) {
                    CHECK(b.controlPoints[k].distanceTo(a.controlPoints[k]) <= 1e-12 * scale);
                    CHECK(b.controlPoints[k].z == 0.0);
                }
                const double domainEnd = a.knotslist.back();
                const int samples = 20 * static_cast<int>(domainEnd);
                for (int s = 0; s <= samples; ++s) {
                    const double t = domainEnd * s / samples;
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

TEST_CASE("A closed source's offset is one open spline whose seam survives a round trip",
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

    // A closed chain in one open spline: its last piece ends at its first piece's start.
    std::vector<Bezier> pieces = arcPieces(RS_Vector{500.0, 250.0}, 75.0, 0.0, 2.0 * M_PI, 7);
    pieces.back()[3] = pieces.front()[0];
    bool exported = false;
    const std::vector<RS_SplineData> reloaded =
        roundTrip({compositeData(pieces)}, RS2::FormatDXFRW, "closed_chain.dxf", exported);
    REQUIRE(exported);
    REQUIRE(reloaded.size() == 1);
    const RS_Spline chain(nullptr, reloaded.front());
    CHECK_FALSE(chain.isClosed()); // no closed or periodic flag
    CHECK(chain.getEndpoint().distanceTo(chain.getStartpoint()) < 1e-9);

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

// ---------------------------------------------------------------------------
// D1 persistence gate

namespace {

/** The R12 writer's tolerance, relative to the diagonal of the control points' box. */
constexpr double kR12RelativeTolerance = 1e-4;

double controlBoxDiagonal(const RS_Spline& s) {
    RS_Vector low{false};
    RS_Vector high{false};
    for (const RS_Vector& v : s.getUnwrappedControlPoints()) {
        low = RS_Vector::minimum(low, v);
        high = RS_Vector::maximum(high, v);
    }
    return low.distanceTo(high);
}

/** Distance from @p p to the curve: the nearest of 200 samples, refined by Newton steps. */
double distanceToCurve(const RS_Spline& s, const RS_Vector& p) {
    double t0 = 0.0;
    double t1 = 0.0;
    if (!s.getParameterDomain(t0, t1)) {
        return RS_MAXDOUBLE;
    }
    const auto pointAt = [&s](const double t) {
        LC_CurveJet j;
        return s.tryEvaluateJet(t, LC_CurveEvaluationSide::Interior, j) ? j.point : RS_Vector{RS_MAXDOUBLE, RS_MAXDOUBLE};
    };
    double best = t0;
    double bestDistance = RS_MAXDOUBLE;
    for (int i = 0; i <= 200; ++i) {
        const double t = t0 + (t1 - t0) * i / 200.0;
        const double distance = pointAt(t).distanceTo(p);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = t;
        }
    }
    for (int k = 0; k < 12; ++k) {
        LC_CurveJet j;
        if (!s.tryEvaluateJet(best, LC_CurveEvaluationSide::Interior, j)) {
            break;
        }
        const RS_Vector r = j.point - p;
        const double slope = RS_Vector::dotP(r, j.first);
        const double curvature = j.first.squared() + RS_Vector::dotP(r, j.second);
        if (!(curvature > 0.0)) {
            break;
        }
        best = std::clamp(best - slope / curvature, t0, t1);
    }
    return std::min(bestDistance, pointAt(best).distanceTo(p));
}

std::string tempPath(const char* name) {
    return (std::filesystem::temp_directory_path() / (std::string("spline_offset_persist_") + name)).string();
}

/** The vertices of a polyline read back, in order. */
std::vector<RS_Vector> polylineVertices(const RS_Polyline& polyline) {
    std::vector<RS_Vector> vertices;
    for (const RS_Entity* e : polyline) {
        if (vertices.empty()) {
            vertices.push_back(e->getStartpoint());
        }
        vertices.push_back(e->getEndpoint());
    }
    return vertices;
}

/** The largest distance between @p spline and the polyline through @p vertices, both ways. */
double polylineDeviation(const RS_Spline& spline, const std::vector<RS_Vector>& vertices, const bool closed) {
    std::vector<std::pair<RS_Vector, RS_Vector>> segments;
    for (size_t i = 1; i < vertices.size(); ++i) {
        segments.emplace_back(vertices[i - 1], vertices[i]);
    }
    if (closed && vertices.size() > 2) {
        segments.emplace_back(vertices.back(), vertices.front());
    }
    REQUIRE_FALSE(segments.empty());
    double worst = 0.0;
    double t0 = 0.0;
    double t1 = 0.0;
    REQUIRE(spline.getParameterDomain(t0, t1));
    for (int i = 0; i <= 2000; ++i) {
        LC_CurveJet j;
        if (!spline.tryEvaluateJet(t0 + (t1 - t0) * i / 2000.0, LC_CurveEvaluationSide::Interior, j)) {
            return RS_MAXDOUBLE;
        }
        double nearest = RS_MAXDOUBLE;
        for (const auto& [a, b] : segments) {
            nearest = std::min(nearest, distanceToSegment(j.point, a, b));
        }
        worst = std::max(worst, nearest);
    }
    // chord midpoints, where a chord strays farthest
    for (const auto& [a, b] : segments) {
        worst = std::max(worst, distanceToCurve(spline, (a + b) * 0.5));
    }
    return worst;
}

/** Writes @p splines to R12 and reads back what became of each, in order. */
std::vector<RS_Polyline*> r12RoundTrip(RS_Graphic& reloaded, const std::vector<RS_SplineData>& splines,
                                       const char* name, bool& exported, const bool dropDisplay = false) {
    ensureSettings();
    const std::string path = tempPath(name);
    {
        RS_Graphic graphic;
        for (const RS_SplineData& data : splines) {
            auto* spline = new RS_Spline(&graphic, data);
            graphic.addEntity(spline);
            if (dropDisplay) {
                spline->clear(); // the segments it is drawn with
                REQUIRE(spline->count() == 0);
            }
        }
        RS_FilterDXFRW filter;
        exported = filter.fileExport(graphic, QString::fromStdString(path), RS2::FormatDXFRW12);
    }
    std::vector<RS_Polyline*> result;
    if (exported) {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(reloaded, QString::fromStdString(path), RS2::FormatDXFRW));
        for (RS_Entity* e : reloaded) {
            auto* polyline = dynamic_cast<RS_Polyline*>(e);
            REQUIRE(polyline != nullptr);
            result.push_back(polyline);
        }
    }
    std::filesystem::remove(path);
    return result;
}

RS_SplineData sCurveData() {
    RS_SplineData d(3, false);
    d.controlPoints = {{1234.5, -87.25}, {1238.5, -81.25}, {1242.5, -93.25}, {1246.5, -87.25}};
    d.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    d.weights.assign(4, 1.0);
    return d;
}

/** A cubic that swings through several bends: more than 32 segments need to follow it. */
RS_SplineData wavyData() {
    RS_SplineData d(3, false);
    for (int i = 0; i < 14; ++i) {
        d.controlPoints.emplace_back(500.0 + 30.0 * i, 200.0 + ((i % 2 == 0) ? -40.0 : 40.0));
    }
    d.knotslist = {0, 0, 0, 0};
    for (int k = 1; k <= 10; ++k) {
        d.knotslist.push_back(k);
    }
    d.knotslist.insert(d.knotslist.end(), 4, 11.0);
    d.weights.assign(d.controlPoints.size(), 1.0);
    return d;
}

} // namespace

TEST_CASE("The offset engine's spline comes back from DXF and DWG as the curve it validated",
          "[curve-offset][d1][persistence]") {
    // Binary DXF is not covered: the filter only exports ASCII DXF.
    ensureSettings();
    const RS_Pen pen{RS_Color{255, 0, 0}, RS2::Width07, RS2::SolidLine};
    struct Format {
        RS2::FormatType type;
        const char* file;
    };
    for (const Format& format : {Format{RS2::FormatDXFRW, "d1.dxf"}, Format{RS2::FormatDXFRW2000, "d1_2000.dxf"},
                                 Format{RS2::FormatDWG, "d1_r2000.dwg"},
                                 Format{RS2::FormatDWG2018, "d1_r2018.dwg"}}) {
        DYNAMIC_SECTION("format " << format.file) {
            const std::string path = tempPath(format.file);
            std::vector<RS_SplineData> written;
            double tolerance = 0.0;
            constexpr double distance = 0.75;
            bool exported = false;
            {
                RS_Graphic graphic;
                auto* layer = new RS_Layer("Offsets");
                graphic.addLayer(layer);
                RS_SplineData sourceData = sCurveData();
                sourceData.fitPoints = {{1234.5, -87.25}, {1246.5, -87.25}}; // not the offset's
                auto* source = new RS_Spline(&graphic, sourceData);
                graphic.addEntity(source);
                source->setLayer(layer);
                source->setPen(pen);
                tolerance = LC_CurveOffset::makeDirectOptions(*source, distance).tolerance.requestedGeometry;
                const std::vector<RS_Entity*> pieces = source->createOffset(RS_Vector{1240.5, -78.25}, distance);
                REQUIRE(pieces.size() == 1); // one spline for the whole offset
                for (RS_Entity* piece : pieces) {
                    graphic.addEntity(piece);
                    piece->reparent(&graphic);
                    const auto* spline = dynamic_cast<const RS_Spline*>(piece);
                    REQUIRE(spline != nullptr);
                    CHECK(spline->getData().fitPoints.empty());
                    written.push_back(spline->getData());
                }
                RS_FilterDXFRW filter;
                exported = filter.fileExport(graphic, QString::fromStdString(path), format.type);
            }
            if (!exported) {
                std::filesystem::remove(path);
            }
            REQUIRE(exported); // every format here is written by this build
            RS_Graphic reloaded;
            {
                RS_FilterDXFRW filter;
                const bool isDwg = format.type >= RS2::FormatDWG && format.type <= RS2::FormatDWG2018;
                REQUIRE(filter.fileImport(reloaded, QString::fromStdString(path),
                                          isDwg ? RS2::FormatDWG : RS2::FormatDXFRW));
            }
            std::filesystem::remove(path);
            std::vector<RS_Spline*> splines;
            for (RS_Entity* e : reloaded) {
                auto* spline = dynamic_cast<RS_Spline*>(e);
                REQUIRE(spline != nullptr);
                splines.push_back(spline);
            }
            REQUIRE(splines.size() == written.size() + 1);
            const RS_Spline& source = *splines.front();
            const double scale = 1246.5;
            for (size_t i = 0; i < written.size(); ++i) {
                const RS_Spline& piece = *splines[i + 1];
                const RS_SplineData& b = piece.getData();
                CHECK(b.degree == 3);
                CHECK(b.type == RS_SplineData::SplineType::ClampedOpen);
                CHECK_FALSE(piece.isClosed());
                CHECK(b.knotslist == written[i].knotslist);
                const size_t n = written[i].controlPoints.size();
                CHECK(n > 4); // several spans
                CHECK(b.weights == std::vector<double>(n, 1.0));
                CHECK(b.fitPoints.empty());
                REQUIRE(b.controlPoints.size() == n);
                for (size_t k = 0; k < n; ++k) {
                    CHECK(b.controlPoints[k].distanceTo(written[i].controlPoints[k]) <= 1e-12 * scale);
                }
                REQUIRE(piece.getLayer() != nullptr);
                CHECK(piece.getLayer()->getName() == "Offsets");
                CHECK(piece.getPen(false) == pen);
                // still the offset, within the budget it was validated against
                double t0 = 0.0;
                double t1 = 0.0;
                REQUIRE(piece.getParameterDomain(t0, t1));
                const int samples = 16 * static_cast<int>(std::lround(t1 - t0));
                for (int k = 0; k <= samples; ++k) {
                    const RS_Vector p =
                        jetAt(piece, t0 + (t1 - t0) * k / samples, LC_CurveEvaluationSide::Interior).point;
                    CHECK(std::abs(distanceToCurve(source, p) - distance) <= tolerance);
                }
            }
        }
    }
}

TEST_CASE("A closed source's offset comes back as an open spline that closes", "[curve-offset][d1][persistence]") {
    RS_Spline source(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        source.addControlPoint(p);
    }
    source.setClosed(true);
    const std::vector<RS_Entity*> pieces = source.createOffset(RS_Vector{25.0, 20.0}, 2.0); // inside
    REQUIRE(pieces.size() == 1);
    std::vector<RS_SplineData> chain;
    for (RS_Entity* piece : pieces) {
        chain.push_back(static_cast<RS_Spline*>(piece)->getData());
        delete piece;
    }
    bool exported = false;
    const std::vector<RS_SplineData> reloaded = roundTrip(chain, RS2::FormatDXFRW, "d1_closed.dxf", exported);
    REQUIRE(exported);
    REQUIRE(reloaded.size() == 1);
    const RS_Spline offset(nullptr, reloaded.front());
    CHECK_FALSE(offset.isClosed());
    CHECK(offset.getEndpoint().distanceTo(offset.getStartpoint()) < 1e-9);
}

TEST_CASE("R12 gets an offset as a polyline within the export tolerance", "[curve-offset][d1][persistence][r12]") {
    RS_Spline source(nullptr, sCurveData());
    const std::vector<RS_Entity*> pieces = source.createOffset(RS_Vector{1240.5, -78.25}, 0.75);
    REQUIRE(pieces.size() == 1);
    std::vector<RS_SplineData> stored;
    for (RS_Entity* piece : pieces) {
        stored.push_back(static_cast<RS_Spline*>(piece)->getData());
        delete piece;
    }
    ensureSettings();
    RS_Graphic reloaded;
    bool exported = false;
    const std::vector<RS_Polyline*> polylines = r12RoundTrip(reloaded, stored, "d1_r12.dxf", exported);
    REQUIRE(exported);
    REQUIRE(polylines.size() == stored.size());
    for (size_t i = 0; i < stored.size(); ++i) {
        const RS_Spline piece(nullptr, stored[i]);
        const std::vector<RS_Vector> vertices = polylineVertices(*polylines[i]);
        REQUIRE(vertices.size() >= 2);
        CHECK_FALSE(polylines[i]->isClosed());
        CHECK(vertices.front().distanceTo(piece.getStartpoint()) <= 1e-9);
        CHECK(vertices.back().distanceTo(piece.getEndpoint()) <= 1e-9);
        CHECK(polylineDeviation(piece, vertices, false) <= kR12RelativeTolerance * controlBoxDiagonal(piece));
    }
}

TEST_CASE("R12 follows a spline more closely than the segments it is drawn with", "[curve-offset][d1][persistence][r12]") {
    const RS_SplineData wavy = wavyData();
    RS_Spline drawn(nullptr, wavy);
    drawn.update();
    const double tolerance = kR12RelativeTolerance * controlBoxDiagonal(drawn);
    const auto exact = [&drawn](const double t) { return jetAt(drawn, t, LC_CurveEvaluationSide::Interior).point; };
    // drawn within 1e-3 of its extent, ten times the R12 tolerance
    REQUIRE(displayDeviation(drawn, exact, 0.0, 11.0) > 2.0 * tolerance);

    ensureSettings();
    RS_Graphic reloaded;
    bool exported = false;
    const std::vector<RS_Polyline*> polylines = r12RoundTrip(reloaded, {wavy}, "wavy_r12.dxf", exported);
    REQUIRE(exported);
    REQUIRE(polylines.size() == 1);
    const std::vector<RS_Vector> vertices = polylineVertices(*polylines.front());
    CHECK(vertices.size() > drawn.count() + 1);
    CHECK(polylineDeviation(drawn, vertices, false) <= tolerance);
}

TEST_CASE("R12 writes a spline that was never drawn, with no stray vertex", "[curve-offset][d1][persistence][r12]") {
    // Without its drawn segments, which the old writer enumerated, adding only
    // the end point after them.
    ensureSettings();
    RS_Graphic reloaded;
    bool exported = false;
    const std::vector<RS_Polyline*> polylines =
        r12RoundTrip(reloaded, {sCurveData()}, "undrawn_r12.dxf", exported, true);
    REQUIRE(exported);
    REQUIRE(polylines.size() == 1);
    const RS_Spline spline(nullptr, sCurveData());
    const std::vector<RS_Vector> vertices = polylineVertices(*polylines.front());
    REQUIRE(vertices.size() > 2);
    for (const RS_Vector& v : vertices) {
        CHECK(v.distanceTo(RS_Vector{0.0, 0.0}) > 1000.0);
        CHECK(distanceToCurve(spline, v) <= 1e-9);
    }
}

TEST_CASE("R12 writes a closed or rational spline within the export tolerance", "[curve-offset][d1][persistence][r12]") {
    // a circle as four rational quadratic arcs
    RS_SplineData circle(2, false);
    const RS_Vector c{-300.0, 75.0};
    const double r = 40.0;
    circle.controlPoints = {c + RS_Vector{r, 0},  c + RS_Vector{r, r},   c + RS_Vector{0, r},
                            c + RS_Vector{-r, r}, c + RS_Vector{-r, 0},  c + RS_Vector{-r, -r},
                            c + RS_Vector{0, -r}, c + RS_Vector{r, -r},  c + RS_Vector{r, 0}};
    const double w = std::sqrt(0.5);
    circle.weights = {1, w, 1, w, 1, w, 1, w, 1};
    circle.knotslist = {0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4};

    RS_Spline closed(nullptr, RS_SplineData(3, false));
    for (const RS_Vector& p : {RS_Vector{0, 0}, RS_Vector{40, -10}, RS_Vector{60, 30}, RS_Vector{20, 50},
                               RS_Vector{-15, 25}}) {
        closed.addControlPoint(p);
    }
    closed.setClosed(true);

    ensureSettings();
    RS_Graphic reloaded;
    bool exported = false;
    const std::vector<RS_Polyline*> polylines =
        r12RoundTrip(reloaded, {circle, closed.getData()}, "closed_rational_r12.dxf", exported);
    REQUIRE(exported);
    REQUIRE(polylines.size() == 2);

    const std::vector<RS_Vector> ring = polylineVertices(*polylines[0]);
    const double circleTolerance = kR12RelativeTolerance * controlBoxDiagonal(RS_Spline(nullptr, circle));
    for (size_t i = 0; i < ring.size(); ++i) {
        CHECK(std::abs(ring[i].distanceTo(c) - r) <= 1e-9);
        if (i > 0) {
            // the sagitta of each chord
            const double half = ring[i].distanceTo(ring[i - 1]) / 2.0;
            CHECK(r - std::sqrt(r * r - half * half) <= circleTolerance);
        }
    }

    CHECK(polylines[1]->isClosed());
    const std::vector<RS_Vector> loop = polylineVertices(*polylines[1]);
    CHECK(polylineDeviation(closed, loop, true) <= kR12RelativeTolerance * controlBoxDiagonal(closed));
    // the closing segment is implied, not a repeated vertex
    const std::vector<RS_Vector> distinct(loop.begin(), loop.end() - 1);
    for (size_t i = 1; i < distinct.size(); ++i) {
        CHECK(distinct[i].distanceTo(distinct.front()) > 1e-6);
    }
}

TEST_CASE("A spline R12 cannot hold within its vertex limit fails the export, writing no part of it",
          "[curve-offset][d1][persistence][r12]") {
    RS_SplineData zigzag(3, false);
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        zigzag.controlPoints.emplace_back(i / static_cast<double>(count), (i % 2 == 0) ? 0.0 : 1.0);
    }
    zigzag.knotslist = {0, 0, 0, 0};
    for (int k = 1; k <= count - 4; ++k) {
        zigzag.knotslist.push_back(k);
    }
    zigzag.knotslist.insert(zigzag.knotslist.end(), 4, count - 3.0);
    zigzag.weights.assign(count, 1.0);
    REQUIRE(RS_Spline(nullptr, zigzag).validate());

    ensureSettings();
    const std::string path = tempPath("over_limit_r12.dxf");
    {
        std::ofstream previous(path);
        previous << "an earlier file";
    }
    {
        RS_Graphic graphic;
        graphic.addEntity(new RS_Spline(&graphic, zigzag));
        RS_FilterDXFRW filter;
        CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(path), RS2::FormatDXFRW12));
    }
    // the failed export leaves the file it would have replaced as it was
    std::ifstream in(path);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(text == "an earlier file");
    in.close();
    std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// MakerCAM SVG

namespace {

/** The 'd' attribute of every path in @p svg, in order. */
std::vector<QString> svgPaths(const std::string& svg) {
    std::vector<QString> paths;
    const QRegularExpression pathData{QStringLiteral(" d=\"([^\"]*)\"")};
    auto it = pathData.globalMatch(QString::fromStdString(svg));
    while (it.hasNext()) {
        paths.push_back(it.next().captured(1));
    }
    return paths;
}

/** The commands of path data @p d: each a letter and the points after it, back in drawing coordinates. */
std::vector<std::pair<QChar, std::vector<RS_Vector>>> svgCommands(const QString& d, const RS_Vector& min,
                                                                  const RS_Vector& max) {
    std::vector<std::pair<QChar, std::vector<RS_Vector>>> commands;
    for (const QString& token : d.split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        QString coordinates = token;
        if (token.front().isLetter()) {
            commands.emplace_back(token.front(), std::vector<RS_Vector>{});
            coordinates = token.mid(1);
        }
        if (coordinates.isEmpty()) {
            continue;
        }
        const QStringList xy = coordinates.split(QLatin1Char(','));
        REQUIRE(xy.size() == 2);
        REQUIRE_FALSE(commands.empty());
        // the writer's x - min.x, max.y - y
        commands.back().second.emplace_back(xy[0].toDouble() + min.x, max.y - xy[1].toDouble());
    }
    return commands;
}

} // namespace

TEST_CASE("MakerCAM SVG writes an offset spline's pieces where they are", "[curve-offset][persistence][svg]") {
    ensureSettings();
    RS_Graphic graphic;
    graphic.setUnit(RS2::Millimeter);
    RS_Spline source(nullptr, sCurveData());
    const std::vector<RS_Entity*> offset = source.createOffset(RS_Vector{1240.5, -78.25}, 0.75);
    REQUIRE(offset.size() == 1);
    auto* spline = static_cast<RS_Spline*>(offset.front());
    graphic.addEntity(spline);
    spline->reparent(&graphic);
    const size_t spans = (spline->getData().controlPoints.size() - 1) / 3;
    REQUIRE(spans > 1);
    // and a degree-1 spline, drawn as the polyline through its control points
    RS_SplineData corners(1, false);
    corners.controlPoints = {{1230.0, -95.0}, {1236.0, -90.0}, {1240.0, -96.0}, {1250.0, -94.0}};
    corners.knotslist = {0.0, 0.0, 1.0, 2.0, 3.0, 3.0};
    corners.weights.assign(4, 1.0);
    auto* polyline = new RS_Spline(&graphic, corners);
    graphic.addEntity(polyline);
    REQUIRE(polyline->validate());
    // the writer goes layer by layer
    auto* layer = new RS_Layer("Offsets");
    graphic.addLayer(layer);
    spline->setLayer(layer);
    polyline->setLayer(layer);

    LC_MakerCamSVG svg(std::make_unique<LC_XMLWriterQXmlStreamWriter>());
    REQUIRE(svg.generate(&graphic));
    const RS_Vector min = graphic.getMin();
    const RS_Vector max = graphic.getMax();
    const std::vector<QString> paths = svgPaths(svg.resultAsString());
    REQUIRE(paths.size() == 2);

    // one cubic per span, each the span: its ends and its middle are the spline's
    const auto curve = svgCommands(paths[0], min, max);
    REQUIRE(curve.size() == 1 + spans);
    REQUIRE(curve[0].first == QLatin1Char('M'));
    REQUIRE(curve[0].second.size() == 1);
    const double scale = 1250.0;
    const double written = 1e-7 * scale; // eight decimals
    CHECK(curve[0].second[0].distanceTo(spline->getStartpoint()) <= written);
    RS_Vector start = curve[0].second[0];
    for (size_t k = 0; k < spans; ++k) {
        const auto& [letter, points] = curve[k + 1];
        REQUIRE(letter == QLatin1Char('C'));
        REQUIRE(points.size() == 3);
        const Bezier piece{start, points[0], points[1], points[2]};
        for (const double u : {0.25, 0.5, 0.75, 1.0}) {
            const RS_Vector expected =
                jetAt(*spline, static_cast<double>(k) + u,
                      u < 1.0 ? LC_CurveEvaluationSide::Interior : LC_CurveEvaluationSide::Left)
                    .point;
            CHECK(bezierAt(piece, u).distanceTo(expected) <= written);
        }
        start = points[2];
    }

    const auto lines = svgCommands(paths[1], min, max);
    REQUIRE(lines.size() == corners.controlPoints.size());
    CHECK(lines[0].first == QLatin1Char('M'));
    for (size_t i = 0; i < lines.size(); ++i) {
        CHECK(lines[i].first == (i == 0 ? QLatin1Char('M') : QLatin1Char('L')));
        REQUIRE(lines[i].second.size() == 1);
        CHECK(lines[i].second[0].distanceTo(corners.controlPoints[i]) <= written);
    }
}
