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

// H01 regression tests for the hatch pattern trimmer.
//
// #2287 (dash extension and per-source intercept de-dup) stretched each
// bundled LINE pattern to 1.5·max(W, H) of the loop bbox and de-duplicated
// tile copies by perpendicular intercept. BRICK then lost its stagger; HEX,
// HONEYCOMB and PAISLEY turned into long lines; and ANSI31 drew about four
// times the right ink. These tests pin tile-local behaviour of the restored
// trimmer. The algorithm is "clip every tile copy against the boundary and
// keep the sub-pieces whose midpoint is inside"; see
// LC_Loops::trimPatternEntities in lc_looputils.cpp.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "lc_filtertestsupport.h"
#include "lc_hatchtestsupport.h"
#include "lc_looputils.h"
#include "lc_patternlinededup.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"

namespace {

using Piece = lc::test::LinePiece;
using lc::test::Ring;
using lc::test::rectRing;
using lc::test::shiftedRing;

// NOLINTBEGIN(readability-identifier-naming)

double inkOf(const std::vector<Piece>& pieces) {
    double total = 0.0;
    for (const Piece& p : pieces) total += p.a.distanceTo(p.b);
    return total;
}

// Mirrors RS_Hatch::updatePatternHatch's pattern setup for angle 0: request,
// clone with ownership, scale about the pattern's bbox centre, recompute borders.
std::unique_ptr<RS_Pattern> scaledPattern(const QString& name, double s) {
    std::unique_ptr<RS_Pattern> p = RS_PATTERNLIST->requestPattern(name);
    REQUIRE(p);
    p.reset(dynamic_cast<RS_Pattern*>(p->clone()));
    REQUIRE(p);
    p->setOwner(true);
    p->scale((p->getMin() + p->getMax()) * 0.5, RS_Vector{s, s});
    p->calculateBorders();
    return p;
}

// Builds a boundary container with RS_Line edges for every ring, runs the
// LoopOptimizer to extract the loop hierarchy, calls trimPatternEntities on
// each root, and returns the LINE pieces. Deliberately bypasses RS_Hatch so
// later de-dup / cache stages of the stack do not affect these oracles.
std::vector<Piece> tilePathPieces(const QString& name, double s,
                                  const std::vector<Ring>& rings) {
    std::unique_ptr<RS_Pattern> pattern = scaledPattern(name, s);
    RS_EntityContainer contour;
    for (const Ring& ring : rings) {
        for (size_t i = 0; i < ring.size(); ++i) {
            const RS_Vector& a = ring[i];
            const RS_Vector& b = ring[(i + 1) % ring.size()];
            contour.addEntity(new RS_Line(&contour, a, b));
        }
    }
    contour.calculateBorders();
    LC_LoopUtils::LoopOptimizer opt(contour);
    auto roots = opt.getResults();
    REQUIRE(roots);
    std::vector<Piece> out;
    for (const LC_LoopUtils::LC_Loops& root : *roots) {
        auto tiles = root.createTiles(*pattern, RS_Vector{0.0, 0.0});
        LC_HatchPieceBudget budget{std::numeric_limits<std::size_t>::max()};
        auto trimmed = root.trimPatternEntities(*pattern, tiles, budget);
        REQUIRE(trimmed);
        for (RS_Entity* en : *trimmed) {
            if (en && en->rtti() == RS2::EntityLine) {
                auto* line = static_cast<RS_Line*>(en);
                out.push_back({line->getStartpoint(), line->getEndpoint()});
            }
        }
    }
    return out;
}

// Clip the segment [a,b] against the closed axis-aligned rect using
// Liang-Barsky. Returns false if nothing is left.
bool clipSegment(RS_Vector& a, RS_Vector& b, double xmin, double ymin,
                 double xmax, double ymax) {
    double tE = 0.0, tL = 1.0;
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double ps[4] = {-dx, dx, -dy, dy};
    const double qs[4] = {a.x - xmin, xmax - a.x, a.y - ymin, ymax - a.y};
    for (int i = 0; i < 4; ++i) {
        if (std::abs(ps[i]) < 1e-15) {
            if (qs[i] < 0.0) return false;
            continue;
        }
        const double t = qs[i] / ps[i];
        if (ps[i] < 0.0) {
            if (t > tL) return false;
            if (t > tE) tE = t;
        } else {
            if (t < tE) return false;
            if (t < tL) tL = t;
        }
    }
    const RS_Vector a0 = a, b0 = b;
    a = {a0.x + tE * dx, a0.y + tE * dy};
    b = {a0.x + tL * dx, a0.y + tL * dy};
    return true;
}

// Rolls out tile copies of pattern's LINE edges within the closed rectangle
// using anchor as (0,0) of the lattice. One copy per tile is emitted; each
// copy is clipped to the rect and only its clipped remnant is returned.
std::vector<Piece> carpetInRect(const RS_Pattern& scaled, const RS_Vector& anchor,
                                double xmin, double ymin, double xmax, double ymax) {
    const RS_Vector pmin = scaled.getMin();
    const RS_Vector pmax = scaled.getMax();
    const double W = pmax.x - pmin.x;
    const double H = pmax.y - pmin.y;
    REQUIRE(W > 0.0);
    REQUIRE(H > 0.0);
    const long i0 = static_cast<long>(std::floor((xmin - anchor.x) / W)) - 1;
    const long i1 = static_cast<long>(std::ceil((xmax - anchor.x) / W)) + 1;
    const long k0 = static_cast<long>(std::floor((ymin - anchor.y) / H)) - 1;
    const long k1 = static_cast<long>(std::ceil((ymax - anchor.y) / H)) + 1;
    std::vector<Piece> out;
    for (RS_Entity* en : scaled) {
        if (!en || en->rtti() != RS2::EntityLine) continue;
        const auto* line = static_cast<const RS_Line*>(en);
        const RS_Vector sa = line->getStartpoint() - pmin + anchor;
        const RS_Vector sb = line->getEndpoint() - pmin + anchor;
        for (long i = i0; i < i1; ++i) {
            for (long k = k0; k < k1; ++k) {
                const RS_Vector d(i * W, k * H);
                RS_Vector a = sa + d;
                RS_Vector b = sb + d;
                if (!clipSegment(a, b, xmin, ymin, xmax, ymax)) continue;
                if (a.distanceTo(b) < 1e-8) continue;
                out.push_back({a, b});
            }
        }
    }
    return out;
}

// Half-open crossing parity using a +x ray from m: counts how many ring edges
// the ray crosses.
bool oddParity(const RS_Vector& m, const std::vector<Ring>& rings) {
    int crossings = 0;
    for (const Ring& ring : rings) {
        for (size_t i = 0; i < ring.size(); ++i) {
            const RS_Vector& a = ring[i];
            const RS_Vector& b = ring[(i + 1) % ring.size()];
            const bool aAbove = (a.y > m.y);
            const bool bAbove = (b.y > m.y);
            if (aAbove == bAbove) continue;
            const double t = (m.y - a.y) / (b.y - a.y);
            if (a.x + t * (b.x - a.x) > m.x) ++crossings;
        }
    }
    return (crossings & 1) != 0;
}

// Carpet laid against a hierarchy of CCW rings: emits one tile copy of every
// pattern LINE edge, splits each at every ring-edge crossing, and keeps the
// sub-pieces whose midpoint has odd global parity. Parallel segment-pair
// intersections are skipped on purpose.
std::vector<Piece> carpetInRings(const RS_Pattern& scaled, const RS_Vector& anchor,
                                 const std::vector<Ring>& rings) {
    double xmin = +RS_MAXDOUBLE, ymin = +RS_MAXDOUBLE;
    double xmax = -RS_MAXDOUBLE, ymax = -RS_MAXDOUBLE;
    for (const Ring& ring : rings) {
        for (const RS_Vector& v : ring) {
            xmin = std::min(xmin, v.x);
            ymin = std::min(ymin, v.y);
            xmax = std::max(xmax, v.x);
            ymax = std::max(ymax, v.y);
        }
    }
    const std::vector<Piece> raw = carpetInRect(scaled, anchor, xmin, ymin, xmax, ymax);
    std::vector<Piece> out;
    for (const Piece& p : raw) {
        std::vector<double> ts{0.0, 1.0};
        for (const Ring& ring : rings) {
            for (size_t i = 0; i < ring.size(); ++i) {
                const RS_Vector& a = ring[i];
                const RS_Vector& b = ring[(i + 1) % ring.size()];
                const double rx = b.x - a.x, ry = b.y - a.y;
                const double sx = p.b.x - p.a.x, sy = p.b.y - p.a.y;
                const double det = rx * sy - ry * sx;
                if (std::abs(det) < 1e-15) continue;
                const double u = ((p.a.x - a.x) * sy - (p.a.y - a.y) * sx) / det;
                const double t = ((p.a.x - a.x) * ry - (p.a.y - a.y) * rx) / det;
                if (u < -1e-9 || u > 1 + 1e-9) continue;
                if (t < 1e-9 || t > 1 - 1e-9) continue;
                ts.push_back(t);
            }
        }
        std::sort(ts.begin(), ts.end());
        for (size_t i = 1; i < ts.size(); ++i) {
            const double t0 = ts[i - 1], t1 = ts[i];
            if (t1 - t0 < 1e-9) continue;
            const double tm = 0.5 * (t0 + t1);
            const RS_Vector mid(p.a.x + tm * (p.b.x - p.a.x),
                                p.a.y + tm * (p.b.y - p.a.y));
            if (!oddParity(mid, rings)) continue;
            out.push_back({{p.a.x + t0 * (p.b.x - p.a.x), p.a.y + t0 * (p.b.y - p.a.y)},
                           {p.a.x + t1 * (p.b.x - p.a.x), p.a.y + t1 * (p.b.y - p.a.y)}});
        }
    }
    return out;
}

// A piece is close to the boundary when all three of start, middle and end
// are within tol of some ring edge.
double distanceToSegment(const RS_Vector& p, const RS_Vector& a, const RS_Vector& b) {
    const double dx = b.x - a.x, dy = b.y - a.y;
    const double len2 = dx * dx + dy * dy;
    if (len2 < 1e-18) return p.distanceTo(a);
    double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

bool nearSomeEdge(const RS_Vector& p, const std::vector<Ring>& rings, double tol) {
    for (const Ring& ring : rings) {
        for (size_t i = 0; i < ring.size(); ++i) {
            if (distanceToSegment(p, ring[i], ring[(i + 1) % ring.size()]) <= tol) {
                return true;
            }
        }
    }
    return false;
}

bool alongBoundary(const Piece& p, const std::vector<Ring>& rings, double tol) {
    const RS_Vector mid((p.a.x + p.b.x) * 0.5, (p.a.y + p.b.y) * 0.5);
    return nearSomeEdge(p.a, rings, tol) && nearSomeEdge(p.b, rings, tol) &&
           nearSomeEdge(mid, rings, tol);
}

std::vector<Piece> interiorOnly(const std::vector<Piece>& pieces,
                                const std::vector<Ring>& rings) {
    std::vector<Piece> out;
    out.reserve(pieces.size());
    for (const Piece& p : pieces) {
        if (!alongBoundary(p, rings, 1e-3)) out.push_back(p);
    }
    return out;
}

// Rounds a vector to a 1e-6 lattice for bucketing.
long long round1e6(double v) { return static_cast<long long>(std::llround(v * 1e6)); }

// Fraction of a whose end-point pairs lie within tol of some piece of b,
// ignoring orientation. O((|a|+|b|)·log|b|) with a 1e-4-rounded midpoint bucket.
double matchedFraction(const std::vector<Piece>& a, const std::vector<Piece>& b,
                       double tol) {
    if (a.empty()) return 1.0;
    struct Key { long long x, y; };
    auto hash = [](const Key& k) {
        return std::hash<long long>()(k.x) ^ (std::hash<long long>()(k.y) << 1);
    };
    auto eq = [](const Key& l, const Key& r) { return l.x == r.x && l.y == r.y; };
    std::unordered_map<Key, std::vector<size_t>, decltype(hash), decltype(eq)> bucket(0, hash, eq);
    const long long bucketStep = static_cast<long long>(std::ceil(1.0 / 1e-4));
    (void)bucketStep;
    for (size_t j = 0; j < b.size(); ++j) {
        const RS_Vector mid((b[j].a.x + b[j].b.x) * 0.5, (b[j].a.y + b[j].b.y) * 0.5);
        const Key k{static_cast<long long>(std::llround(mid.x / 1e-3)),
                    static_cast<long long>(std::llround(mid.y / 1e-3))};
        bucket[k].push_back(j);
    }
    size_t hits = 0;
    for (const Piece& ap : a) {
        const RS_Vector mid((ap.a.x + ap.b.x) * 0.5, (ap.a.y + ap.b.y) * 0.5);
        const Key kc{static_cast<long long>(std::llround(mid.x / 1e-3)),
                     static_cast<long long>(std::llround(mid.y / 1e-3))};
        bool found = false;
        for (long long dx = -1; dx <= 1 && !found; ++dx) {
            for (long long dy = -1; dy <= 1 && !found; ++dy) {
                const Key k{kc.x + dx, kc.y + dy};
                auto it = bucket.find(k);
                if (it == bucket.end()) continue;
                for (size_t j : it->second) {
                    const bool d1 = ap.a.distanceTo(b[j].a) <= tol &&
                                    ap.b.distanceTo(b[j].b) <= tol;
                    const bool d2 = ap.a.distanceTo(b[j].b) <= tol &&
                                    ap.b.distanceTo(b[j].a) <= tol;
                    if (d1 || d2) {
                        found = true;
                        break;
                    }
                }
            }
        }
        if (found) ++hits;
    }
    return static_cast<double>(hits) / static_cast<double>(a.size());
}

// NOLINTEND(readability-identifier-naming)

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lines: BRICK verticals stay half a course long and keep the stagger",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const double s = GENERATE(1.0, 0.5);
    CAPTURE(s);
    auto hatch = lc::test::makePatternHatch(
        "BRICK", s, 0.0, {rectRing(0, 0, 500, 300)});
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const auto pieces = lc::test::patternLines(*hatch);
    REQUIRE_FALSE(pieces.empty());

    // Pattern period: 100 x 100.00001 at scale 1. Vertical span: 50.000006
    // and 50.000004 (two independent chord lengths from the .dxf).
    const double pH = 100.00001 * s;
    const double limit = 50.000006 * s + 1e-9;

    bool sawEven = false;
    bool sawOdd = false;
    for (const auto& p : pieces) {
        const double dx = p.b.x - p.a.x;
        const double dy = p.b.y - p.a.y;
        const double len = std::hypot(dx, dy);
        if (len < 1e-9) continue;
        const double ux = dx / len, uy = dy / len;
        if (std::abs(uy) < 1.0 - 1e-12) continue;  // only vertical pieces
        (void)ux;
        INFO("piece len=" << len << " start=(" << p.a.x << "," << p.a.y << ")");
        REQUIRE(len <= limit);

        const double midX = 0.5 * (p.a.x + p.b.x);
        const double midY = 0.5 * (p.a.y + p.b.y);
        const double row = std::floor(midY / (pH * 0.5));
        const long rowParity = static_cast<long>(std::fmod(row, 2.0));
        const double period = 100.0 * s;
        const double xmod = midX - period * std::floor(midX / period);
        const double expected = (rowParity == 0) ? 50.000002 * s : 0.0;
        const double delta = std::min(std::abs(xmod - expected),
                                      std::abs(xmod - expected - period));
        INFO("row " << row << " parity=" << rowParity << " xmod=" << xmod
                   << " expected=" << expected);
        REQUIRE(delta <= 1e-6 * s + 1e-9);
        if (rowParity == 0) sawEven = true; else sawOdd = true;
    }
    CHECK(sawEven);
    CHECK(sawOdd);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lines: HEX dashes are never longer than the tile's longest line",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const auto pieces = tilePathPieces("HEX", 1.0, {rectRing(0, 0, 500, 300)});
    REQUIRE_FALSE(pieces.empty());
    double longest = 0.0;
    for (const auto& p : pieces) longest = std::max(longest, p.a.distanceTo(p.b));
    // Longest LINE in the scaled HEX tile is 4.79928; +1e-9 for ULP.
    REQUIRE(longest <= 4.80 + 1e-9);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lines: ANSI31 ink equals area over spacing",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    auto hatch = lc::test::makePatternHatch(
        "ANSI31", 1.0, 0.0, {rectRing(0, 0, 500, 300)});
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const auto pieces = lc::test::patternLines(*hatch);
    REQUIRE_FALSE(pieces.empty());

    // ANSI31 is 45-deg lines at spacing 1/sqrt(2) within a 500x300 rect.
    // Projecting the rect normal to the line direction gives width 150000·sqrt(2)/2
    // over sqrt(2)/2 ≈ 150000 ÷ 1 ink units ≈ 106066. The ±P∥ bound: 500+300=800
    // divided by sqrt(2) ≈ 565 — the plan gives ±1131.
    const double ink = inkOf(pieces);
    INFO("ink=" << ink);
    REQUIRE(std::abs(ink - 150000.0 / std::sqrt(2.0)) <= 1600.0 / std::sqrt(2.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lines: no bundled tile stretches a line beyond its own length",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const QDir dir(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));
    QStringList names = dir.entryList({QStringLiteral("*.dxf")}, QDir::Files, QDir::Name);
    REQUIRE(!names.isEmpty());
    for (const QString& fileName : names) {
        const QString name = QFileInfo(fileName).baseName();
        CAPTURE(name.toStdString());
        auto raw = scaledPattern(name, 1.0);
        double longest = 0.0;
        bool hasLine = false;
        for (RS_Entity* en : *raw) {
            if (en && en->rtti() == RS2::EntityLine) {
                hasLine = true;
                const auto* line = static_cast<const RS_Line*>(en);
                longest = std::max(longest, line->getStartpoint().distanceTo(
                                                line->getEndpoint()));
            }
        }
        if (!hasLine) continue;  // arcs-only patterns: skip (gost_wood, misc01, …)
        const auto pieces = tilePathPieces(name, 1.0, {rectRing(0, 0, 250, 150)});
        REQUIRE_FALSE(pieces.empty());
        double maxPiece = 0.0;
        for (const auto& p : pieces) maxPiece = std::max(maxPiece, p.a.distanceTo(p.b));
        INFO("longest LINE in tile = " << longest << ", maxPiece = " << maxPiece);
        REQUIRE(maxPiece <= longest + 1e-9);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lines: PAISLEY piece count matches the clipped carpet",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const auto rect = rectRing(0, 0, 500, 300);
    const auto fixed = tilePathPieces("PAISLEY", 1.0, {rect});
    REQUIRE_FALSE(fixed.empty());

    auto scaled = scaledPattern("PAISLEY", 1.0);
    const auto carpet = carpetInRect(*scaled, RS_Vector(0, 0),
                                     0.0, 0.0, 500.0, 300.0);
    REQUIRE_FALSE(carpet.empty());

    // Exact interior equality: counts match and every piece has a mate within
    // 1e-7 (orientation ignored).
    const std::vector<Ring> rings{rect};
    const auto fixedIn = interiorOnly(fixed, rings);
    const auto carpetIn = interiorOnly(carpet, rings);
    REQUIRE(fixedIn.size() == carpetIn.size());
    REQUIRE(matchedFraction(fixedIn, carpetIn, 1e-7) == Catch::Approx(1.0));
    REQUIRE(matchedFraction(carpetIn, fixedIn, 1e-7) == Catch::Approx(1.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lines: BRICK around a square island matches the clipped carpet",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const std::vector<Ring> rings = {rectRing(0, 0, 400, 400),
                                     rectRing(100, 100, 300, 300)};
    const auto fixed = interiorOnly(tilePathPieces("BRICK", 1.0, rings), rings);
    REQUIRE_FALSE(fixed.empty());
    auto scaled = scaledPattern("BRICK", 1.0);
    const auto carpet = interiorOnly(
        carpetInRings(*scaled, RS_Vector(0, 0), rings), rings);
    REQUIRE_FALSE(carpet.empty());

    // Exact interior equality: counts match and every piece has a mate within
    // 1e-7 (orientation ignored).
    REQUIRE(fixed.size() == carpet.size());
    REQUIRE(matchedFraction(fixed, carpet, 1e-7) == Catch::Approx(1.0));
    REQUIRE(matchedFraction(carpet, fixed, 1e-7) == Catch::Approx(1.0));
}

// --- H02 lattice-anchor tests. --------------------------------------------
// Master anchors each root's lattice at that root's rotated bbox corner, so
// two disjoint roots of one hatch get different phases and every edit moves
// the pattern. The fix anchors every root to the WCS point h = (0, 0), passes
// it through LC_Loops::trimPatternEntities and uses [floor, ceil) tile
// indices in createTiles. See plan step H02.

namespace {

// NOLINTBEGIN(readability-identifier-naming)

// Pattern end-point fractions ((e.x - pmin.x)/W mod 1, (e.y - pmin.y)/H mod 1)
// for every LINE child of scaled.
struct Fraction { double u, w; };
std::vector<Fraction> tileFractions(const RS_Pattern& scaled) {
    const RS_Vector pmin = scaled.getMin();
    const RS_Vector pmax = scaled.getMax();
    const double W = pmax.x - pmin.x;
    const double H = pmax.y - pmin.y;
    auto wrap = [](double v) { return v - std::floor(v); };
    std::vector<Fraction> out;
    for (const RS_Entity* en : scaled) {
        if (!en || en->rtti() != RS2::EntityLine) continue;
        const auto* line = static_cast<const RS_Line*>(en);
        for (const RS_Vector& e : {line->getStartpoint(), line->getEndpoint()}) {
            out.push_back({wrap((e.x - pmin.x) / W), wrap((e.y - pmin.y) / H)});
        }
    }
    return out;
}

// min over fractions f of max(circ(u - f.u), circ(w - f.w)) with the lattice
// h, rotated into the loop frame by -a. circ(d) = |d - round(d)|. Returns
// RS_MAXDOUBLE if there are no fractions (an arcs-only pattern).
double latticeResidue(const RS_Vector& p, const std::vector<Fraction>& fractions,
                      double W, double H, const RS_Vector& h, double a) {
    if (fractions.empty()) return RS_MAXDOUBLE;
    RS_Vector v = p - h;
    v.rotate(RS_Vector{-a});
    const double u = v.x / W;
    const double w = v.y / H;
    auto circ = [](double d) { return std::abs(d - std::round(d)); };
    double best = RS_MAXDOUBLE;
    for (const Fraction& f : fractions) {
        const double du = circ(u - f.u);
        const double dw = circ(w - f.w);
        best = std::min(best, std::max(du, dw));
    }
    return best;
}

// Shortest distance from a point to any edge of any ring.
double distanceToRings(const RS_Vector& p, const std::vector<Ring>& rings) {
    double best = RS_MAXDOUBLE;
    for (const Ring& ring : rings) {
        for (size_t i = 0; i < ring.size(); ++i) {
            best = std::min(best, distanceToSegment(p, ring[i], ring[(i + 1) % ring.size()]));
        }
    }
    return best;
}

// Liang-Barsky clip of every piece against the axis-aligned closed rect; drop
// pieces with length <= 1e-9 after clipping.
std::vector<Piece> clipToRect(const std::vector<Piece>& pieces,
                              double xmin, double ymin, double xmax, double ymax) {
    std::vector<Piece> out;
    out.reserve(pieces.size());
    for (const Piece& p : pieces) {
        RS_Vector a = p.a;
        RS_Vector b = p.b;
        if (!clipSegment(a, b, xmin, ymin, xmax, ymax)) continue;
        if (a.distanceTo(b) <= 1e-9) continue;
        out.push_back({a, b});
    }
    return out;
}

struct FarPatternCase { const char* name; double angle; };
const FarPatternCase kFarPatternCases[] = {
    {"BRICK",   M_PI / 6.0},
    {"HEX",     0.0},
    {"ANSI31",  M_PI / 4.0},
};

struct TileCountCase { const char* name; size_t tiles; };
const TileCountCase kTileCountCases[] = { {"BRICK", 15}, {"HEX", 20} };

// NOLINTEND(readability-identifier-naming)

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lattice: two disjoint roots share the WCS phase",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const double angle = M_PI / 6.0;
    auto hatch = lc::test::makePatternHatch(
        "BRICK", 1.0, angle,
        {rectRing(0, 0, 500, 300), rectRing(700, 50, 900, 260)});
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    auto scaled = scaledPattern("BRICK", 1.0);
    const RS_Vector pmin = scaled->getMin();
    const RS_Vector pmax = scaled->getMax();
    const double W = pmax.x - pmin.x;
    const double H = pmax.y - pmin.y;
    const std::vector<Fraction> fractions = tileFractions(*scaled);
    REQUIRE_FALSE(fractions.empty());

    const std::vector<Ring> rings = {rectRing(0, 0, 500, 300),
                                     rectRing(700, 50, 900, 260)};
    size_t rootA = 0;
    size_t rootB = 0;
    const auto pieces = lc::test::patternLines(*hatch);
    REQUIRE_FALSE(pieces.empty());
    for (const Piece& p : pieces) {
        for (const RS_Vector& e : {p.a, p.b}) {
            if (distanceToRings(e, rings) <= 1e-6) continue;  // boundary piece
            const double r = latticeResidue(e, fractions, W, H, RS_Vector{0.0, 0.0}, angle);
            INFO("end=(" << e.x << "," << e.y << ") residue=" << r);
            REQUIRE(r <= 1e-9);
            if (e.x < 600) ++rootA; else ++rootB;
        }
    }
    CHECK(rootA > 0);
    CHECK(rootB > 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lattice: a hatch far from the origin keeps the WCS phase",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const auto c = GENERATE(from_range(std::begin(kFarPatternCases),
                                       std::end(kFarPatternCases)));
    CAPTURE(c.name);
    const RS_Vector shift(1.0e6, -2.0e6);
    const auto ring = shiftedRing(rectRing(0, 0, 500, 300), shift);
    auto hatch = lc::test::makePatternHatch(
        c.name, 1.0, c.angle, {ring});
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    auto scaled = scaledPattern(c.name, 1.0);
    const RS_Vector pmin = scaled->getMin();
    const RS_Vector pmax = scaled->getMax();
    const double W = pmax.x - pmin.x;
    const double H = pmax.y - pmin.y;
    const std::vector<Fraction> fractions = tileFractions(*scaled);
    REQUIRE_FALSE(fractions.empty());
    const auto pieces = lc::test::patternLines(*hatch);
    REQUIRE_FALSE(pieces.empty());
    size_t interiorEnds = 0;
    for (const Piece& p : pieces) {
        for (const RS_Vector& e : {p.a, p.b}) {
            if (distanceToRings(e, {ring}) <= 1e-6) continue;
            const double r = latticeResidue(e, fractions, W, H, RS_Vector{0.0, 0.0}, c.angle);
            INFO("end=(" << e.x << "," << e.y << ") residue=" << r);
            REQUIRE(r <= 1e-9);
            ++interiorEnds;
        }
    }
    CHECK(interiorEnds > 0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lattice: the pattern stays put when the boundary moves",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const double angle = M_PI / 6.0;
    const RS_Vector shift(37.3, 11.9);
    auto hatchA = lc::test::makePatternHatch(
        "BRICK", 1.0, angle, {rectRing(0, 0, 500, 300)});
    auto hatchB = lc::test::makePatternHatch(
        "BRICK", 1.0, angle, {shiftedRing(rectRing(0, 0, 500, 300), shift)});
    REQUIRE(hatchA->getUpdateError() == RS_Hatch::HATCH_OK);
    REQUIRE(hatchB->getUpdateError() == RS_Hatch::HATCH_OK);
    // Shrunk interior Omega: (37.3,11.9)-(500,300) by 1e-3.
    const double xmin = shift.x + 1e-3;
    const double ymin = shift.y + 1e-3;
    const double xmax = 500.0 - 1e-3;
    const double ymax = 300.0 - 1e-3;
    const auto A = clipToRect(lc::test::patternLines(*hatchA), xmin, ymin, xmax, ymax);
    const auto B = clipToRect(lc::test::patternLines(*hatchB), xmin, ymin, xmax, ymax);
    REQUIRE_FALSE(A.empty());
    REQUIRE_FALSE(B.empty());
    // The two hatches share the WCS lattice, so inside the common clip rect
    // the clipped line sets must agree exactly.
    const double ab = matchedFraction(A, B, 1e-7);
    const double ba = matchedFraction(B, A, 1e-7);
    INFO("|A|=" << A.size() << " |B|=" << B.size()
                << " ab=" << ab << " ba=" << ba);
    REQUIRE(ab == Catch::Approx(1.0));
    REQUIRE(ba == Catch::Approx(1.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lattice: tile indices are half-open as in 2.2.1",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    // Build a 500x300 rect boundary and extract the loop.
    RS_EntityContainer contour;
    const Ring ring = rectRing(0, 0, 500, 300);
    for (size_t i = 0; i < ring.size(); ++i) {
        const RS_Vector& a = ring[i];
        const RS_Vector& b = ring[(i + 1) % ring.size()];
        contour.addEntity(new RS_Line(&contour, a, b));
    }
    contour.calculateBorders();
    LC_LoopUtils::LoopOptimizer opt(contour);
    auto roots = opt.getResults();
    REQUIRE(roots);
    REQUIRE_FALSE(roots->empty());
    const LC_LoopUtils::LC_Loops& root = (*roots)[0];

    const auto c = GENERATE(from_range(std::begin(kTileCountCases),
                                       std::end(kTileCountCases)));
    CAPTURE(c.name);
    auto scaled = scaledPattern(c.name, 1.0);
    const auto tiles = root.createTiles(*scaled, RS_Vector{0.0, 0.0});
    INFO("tiles=" << tiles.size());
    REQUIRE(tiles.size() == c.tiles);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lattice: an island off the lattice is hatched from the WCS lattice",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const RS_Vector shift(37.3, 11.9);
    const Ring outer = shiftedRing(rectRing(0, 0, 300, 300), shift);
    // Narrow U-shape I2 inside the outer rect.
    const Ring I2raw = {RS_Vector(31, 43.5),  RS_Vector(269, 43.5),
                        RS_Vector(269, 270),  RS_Vector(231, 270),
                        RS_Vector(231, 83.5), RS_Vector(69, 83.5),
                        RS_Vector(69, 270),   RS_Vector(31, 270)};
    const Ring I2 = shiftedRing(I2raw, shift);
    const std::vector<Ring> rings = {outer, I2};

    const auto fixed = interiorOnly(tilePathPieces("ANSI31", 1.0, rings), rings);
    REQUIRE_FALSE(fixed.empty());
    auto scaled = scaledPattern("ANSI31", 1.0);
    const auto carpet = interiorOnly(
        carpetInRings(*scaled, RS_Vector(0, 0), rings), rings);
    REQUIRE_FALSE(carpet.empty());

    // Deterministic classifier: the island draws exactly the clipped carpet.
    REQUIRE(fixed.size() == carpet.size());
    REQUIRE(matchedFraction(fixed, carpet, 1e-7) == Catch::Approx(1.0));
    REQUIRE(matchedFraction(carpet, fixed, 1e-7) == Catch::Approx(1.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lattice: a scale-only edit keeps the phase",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const double angle = M_PI / 6.0;
    const Ring ring = rectRing(0, 0, 500, 300);
    auto hatch = std::make_unique<RS_Hatch>(
        nullptr, RS_HatchData(false, 1.0, angle, QStringLiteral("BRICK")));
    auto* loop = new RS_EntityContainer(hatch.get());
    hatch->addEntity(loop);
    for (size_t i = 0; i < ring.size(); ++i) {
        const RS_Vector& a = ring[i];
        const RS_Vector& b = ring[(i + 1) % ring.size()];
        loop->addEntity(new RS_Line(loop, a, b));
    }
    hatch->update();
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    // setScale deliberately does not set m_needOptimization: validate() is
    // skipped on the second update, so the stored m_pivot/m_loopAngle are
    // what the trimmer uses.
    hatch->setScale(0.5);
    hatch->update();
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);

    auto scaled = scaledPattern("BRICK", 0.5);
    const RS_Vector pmin = scaled->getMin();
    const RS_Vector pmax = scaled->getMax();
    const double W = pmax.x - pmin.x;
    const double H = pmax.y - pmin.y;
    const std::vector<Fraction> fractions = tileFractions(*scaled);
    REQUIRE_FALSE(fractions.empty());
    const auto pieces = lc::test::patternLines(*hatch);
    REQUIRE_FALSE(pieces.empty());
    size_t interiorEnds = 0;
    for (const Piece& p : pieces) {
        for (const RS_Vector& e : {p.a, p.b}) {
            if (distanceToRings(e, {ring}) <= 1e-6) continue;
            const double r = latticeResidue(e, fractions, W, H, RS_Vector{0.0, 0.0}, angle);
            INFO("end=(" << e.x << "," << e.y << ") residue=" << r);
            REQUIRE(r <= 1e-9);
            ++interiorEnds;
        }
    }
    CHECK(interiorEnds > 0);
}

namespace {

// A canonical bit-equal signature of a hatch's LINE children, sorted so that
// the comparison is independent of insertion order inside the hatch.
struct PieceSignature {
    double ax, ay, bx, by;
    bool operator<(const PieceSignature& o) const {
        if (ax != o.ax) return ax < o.ax;
        if (ay != o.ay) return ay < o.ay;
        if (bx != o.bx) return bx < o.bx;
        return by < o.by;
    }
    bool operator==(const PieceSignature& o) const {
        return ax == o.ax && ay == o.ay && bx == o.bx && by == o.by;
    }
};

std::vector<PieceSignature> hatchSignature(RS_Hatch& h) {
    std::vector<PieceSignature> out;
    const auto pieces = lc::test::patternLines(h);
    out.reserve(pieces.size());
    for (const Piece& p : pieces) {
        PieceSignature s{p.a.x, p.a.y, p.b.x, p.b.y};
        // Canonicalise orientation: the smaller end comes first.
        if (std::tie(s.bx, s.by) < std::tie(s.ax, s.ay)) {
            std::swap(s.ax, s.bx);
            std::swap(s.ay, s.by);
        }
        out.push_back(s);
    }
    std::sort(out.begin(), out.end());
    return out;
}

struct DetCase {
    const char* name;
    const char* pattern;
    double scale;
    std::vector<lc::test::Ring> rings;
};

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Pattern children are identical over regenerations",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const auto L_outer = lc::test::Ring{
        RS_Vector(0, 0),   RS_Vector(400, 0),  RS_Vector(400, 40),
        RS_Vector(40, 40), RS_Vector(40, 400), RS_Vector(0, 400)};
    const auto A_outer = lc::test::rectRing(0, 0, 300, 300);
    const auto A_inner = lc::test::rectRing(100, 100, 200, 200);

    const std::vector<DetCase> cases = {
        {"ANSI31-rect", "ANSI31", 1.0, {rectRing(0, 0, 500, 300)}},
        {"BRICK-rect",  "BRICK",  1.0, {rectRing(0, 0, 500, 300)}},
        {"HEX-rect",    "HEX",    1.0, {rectRing(0, 0, 500, 300)}},
        {"BRICK-L",     "BRICK",  1.0, {L_outer}},
        {"HEX-annulus", "HEX",    1.0, {A_outer, A_inner}},
    };
    for (const DetCase& c : cases) {
        CAPTURE(c.name);
        auto hatch = std::make_unique<RS_Hatch>(
            nullptr, RS_HatchData(false, c.scale, 0.0, QString::fromUtf8(c.pattern)));
        for (const auto& ring : c.rings) {
            auto* loop = new RS_EntityContainer(hatch.get());
            hatch->addEntity(loop);
            for (size_t i = 0; i < ring.size(); ++i) {
                const RS_Vector& a = ring[i];
                const RS_Vector& b = ring[(i + 1) % ring.size()];
                loop->addEntity(new RS_Line(loop, a, b));
            }
        }
        hatch->update();
        REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
        const auto baseline = hatchSignature(*hatch);
        REQUIRE_FALSE(baseline.empty());
        for (int iter = 0; iter < 20; ++iter) {
            hatch->update();
            REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
            const auto again = hatchSignature(*hatch);
            INFO("iteration " << iter << " pattern=" << c.name);
            REQUIRE(again == baseline);
        }
    }
}

namespace {

// Signature of all pattern hatches in a graphic (set of per-hatch sorted
// LINE-piece tuples). Deterministic across hatch order inside the graphic.
std::vector<std::vector<PieceSignature>> graphicSignature(RS_Graphic& g) {
    std::vector<std::vector<PieceSignature>> out;
    for (RS_Entity* en : g) {
        if (!en || en->rtti() != RS2::EntityHatch) continue;
        auto& h = static_cast<RS_Hatch&>(*en);
        if (h.isSolid()) continue;
        out.push_back(hatchSignature(h));
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Density fixtures regenerate identically",
          "[.slow][lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    lc::test::QuietLog quiet;
    const std::filesystem::path dir =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "hatch" / "density";
    if (!std::filesystem::exists(dir)) {
        SKIP("density fixtures not found: " << dir.string());
    }
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dxf") {
            files.push_back(entry.path());
        }
    }
    REQUIRE_FALSE(files.empty());
    std::sort(files.begin(), files.end());

    // Ten regenerations per fixture: enough to show that the extraction order
    // is deterministic across repeated updates. (The research note's 100 is
    // excessive here: some scale-1 fixtures produce hundreds of thousands of
    // pieces per hatch.)
    const int iterations = 10;
    for (const auto& file : files) {
        CAPTURE(file.string());
        RS_Graphic g;
        REQUIRE(lc::test::importDrawing(g, file, RS2::FormatDXFRW));
        const auto baseline = graphicSignature(g);
        if (baseline.empty()) continue;  // no pattern hatches in this file
        for (int iter = 0; iter < iterations; ++iter) {
            // Re-update every hatch and check the signature stays identical.
            for (RS_Entity* en : g) {
                if (!en || en->rtti() != RS2::EntityHatch) continue;
                en->update();
            }
            const auto again = graphicSignature(g);
            INFO("iteration " << iter);
            REQUIRE(again == baseline);
        }
    }
}

namespace {

// U-shape outer boundary (300 x 300 with a 100-wide inner notch from the top).
Ring hatchfixOuterU() {
    return {RS_Vector(0, 0),     RS_Vector(300, 0),   RS_Vector(300, 300),
            RS_Vector(200, 300), RS_Vector(200, 100), RS_Vector(100, 100),
            RS_Vector(100, 300), RS_Vector(0, 300)};
}

// U-shape hole nested in the above (bbox centre lies in the notch).
Ring hatchfixInnerU() {
    return {RS_Vector(30, 40),   RS_Vector(270, 40),  RS_Vector(270, 270),
            RS_Vector(230, 270), RS_Vector(230, 80),  RS_Vector(70, 80),
            RS_Vector(70, 270),  RS_Vector(30, 270)};
}

// Shifted U hole whose x- and y-walls are off the outer U by 1 and 3.5 so the
// correct hierarchy answer still matches the clipped carpet exactly.
Ring hatchfixShiftedInnerU() {
    return {RS_Vector(31, 43.5),  RS_Vector(269, 43.5), RS_Vector(269, 270),
            RS_Vector(231, 270),  RS_Vector(231, 83.5), RS_Vector(69, 83.5),
            RS_Vector(69, 270),   RS_Vector(31, 270)};
}

// L-shape outer boundary.
Ring hatchfixLOuter() {
    return {RS_Vector(0, 0),     RS_Vector(300, 0),   RS_Vector(300, 100),
            RS_Vector(100, 100), RS_Vector(100, 300), RS_Vector(0, 300)};
}

// L-shape hole whose bbox centre is outside the outer L (above its vertical leg).
Ring hatchfixLInner() {
    return {RS_Vector(20, 20),   RS_Vector(280, 20),  RS_Vector(280, 80),
            RS_Vector(80, 80),   RS_Vector(80, 280),  RS_Vector(20, 280)};
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern lines: islands in a U and an L wall match the clipped carpet",
          "[lc-hatchfix][pattern]") {
    // Three oracle rings. The parent is a U or an L, so each island's bbox
    // centre lies outside its parent; the master build therefore leaves the
    // island a root and hatches its interior as well. The clipped carpet
    // uses global half-open parity and does not hatch the island; the
    // reference and the trimmer must agree only once the sorter picks a
    // strictly interior point of the island to decide its parent.
    lc::test::SourcePatterns patterns;
    SECTION("BRICK U-in-U") {
        const std::vector<Ring> rings{hatchfixOuterU(), hatchfixInnerU()};
        const auto fixed = interiorOnly(tilePathPieces("BRICK", 1.0, rings), rings);
        REQUIRE_FALSE(fixed.empty());
        auto scaled = scaledPattern("BRICK", 1.0);
        const auto carpet = interiorOnly(
            carpetInRings(*scaled, RS_Vector(0, 0), rings), rings);
        REQUIRE_FALSE(carpet.empty());
        REQUIRE(fixed.size() == carpet.size());
        REQUIRE(matchedFraction(fixed, carpet, 1e-7) == Catch::Approx(1.0));
        REQUIRE(matchedFraction(carpet, fixed, 1e-7) == Catch::Approx(1.0));
    }
    SECTION("BRICK L-wall") {
        const std::vector<Ring> rings{hatchfixLOuter(), hatchfixLInner()};
        const auto fixed = interiorOnly(tilePathPieces("BRICK", 1.0, rings), rings);
        REQUIRE_FALSE(fixed.empty());
        auto scaled = scaledPattern("BRICK", 1.0);
        const auto carpet = interiorOnly(
            carpetInRings(*scaled, RS_Vector(0, 0), rings), rings);
        REQUIRE_FALSE(carpet.empty());
        REQUIRE(fixed.size() == carpet.size());
        REQUIRE(matchedFraction(fixed, carpet, 1e-7) == Catch::Approx(1.0));
        REQUIRE(matchedFraction(carpet, fixed, 1e-7) == Catch::Approx(1.0));
    }
    SECTION("ANSI31 K_ansi_U_in_U_shifted") {
        const RS_Vector shift(37.3, 11.9);
        const std::vector<Ring> rings{shiftedRing(hatchfixOuterU(), shift),
                                      shiftedRing(hatchfixShiftedInnerU(), shift)};
        const auto fixed = interiorOnly(tilePathPieces("ANSI31", 1.0, rings), rings);
        REQUIRE_FALSE(fixed.empty());
        auto scaled = scaledPattern("ANSI31", 1.0);
        const auto carpet = interiorOnly(
            carpetInRings(*scaled, RS_Vector(0, 0), rings), rings);
        REQUIRE_FALSE(carpet.empty());
        REQUIRE(fixed.size() == carpet.size());
        REQUIRE(matchedFraction(fixed, carpet, 1e-7) == Catch::Approx(1.0));
        REQUIRE(matchedFraction(carpet, fixed, 1e-7) == Catch::Approx(1.0));
    }
}

// --- Interior-tile fast path. ---------------------------------------------
// LC_Loops::createTiles now returns LC_TileSlot values whose interior flag is
// true when the tile's expanded rect (one slack per side) does not overlap any
// boundary edge. The trimmer's fast path emits every clone of such a slot
// whole, without clipping; the no-intersection branch of the slow path would
// have emitted the identical clone through keepPiece.

namespace {

// NOLINTBEGIN(readability-identifier-naming)

// A root for the first ring of rings, built through LoopOptimizer so m_eps is
// set from the per-hatch tolerance of the loop container.
std::shared_ptr<LC_LoopUtils::LoopOptimizer>
buildOptimizerFromRings(RS_EntityContainer& contour, const std::vector<Ring>& rings) {
    for (const Ring& ring : rings) {
        for (size_t i = 0; i < ring.size(); ++i) {
            const RS_Vector& a = ring[i];
            const RS_Vector& b = ring[(i + 1) % ring.size()];
            contour.addEntity(new RS_Line(&contour, a, b));
        }
    }
    contour.calculateBorders();
    return std::make_shared<LC_LoopUtils::LoopOptimizer>(contour);
}

// A stable key for a trimmer-produced piece. Doubles use exact == comparison
// so the fast and slow paths' bit-identical clones match element-by-element.
struct PieceKey {
    int rtti = 0;
    std::array<double, 8> data{};
    bool operator<(const PieceKey& o) const {
        if (rtti != o.rtti) return rtti < o.rtti;
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] != o.data[i]) return data[i] < o.data[i];
        }
        return false;
    }
    bool operator==(const PieceKey& o) const {
        return rtti == o.rtti && data == o.data;
    }
};

PieceKey pieceKey(const RS_Entity* e) {
    PieceKey k;
    k.rtti = static_cast<int>(e->rtti());
    switch (e->rtti()) {
    case RS2::EntityLine: {
        const auto* ln = static_cast<const RS_Line*>(e);
        k.data[0] = ln->getStartpoint().x;
        k.data[1] = ln->getStartpoint().y;
        k.data[2] = ln->getEndpoint().x;
        k.data[3] = ln->getEndpoint().y;
        break;
    }
    case RS2::EntityArc: {
        const auto* ar = static_cast<const RS_Arc*>(e);
        k.data[0] = ar->getCenter().x;
        k.data[1] = ar->getCenter().y;
        k.data[2] = ar->getRadius();
        k.data[3] = ar->getAngle1();
        k.data[4] = ar->getAngle2();
        k.data[5] = ar->isReversed() ? 1.0 : 0.0;
        break;
    }
    case RS2::EntityCircle: {
        const auto* ci = static_cast<const RS_Circle*>(e);
        k.data[0] = ci->getCenter().x;
        k.data[1] = ci->getCenter().y;
        k.data[2] = ci->getRadius();
        break;
    }
    case RS2::EntityEllipse: {
        const auto* el = static_cast<const RS_Ellipse*>(e);
        k.data[0] = el->getCenter().x;
        k.data[1] = el->getCenter().y;
        k.data[2] = el->getMajorP().x;
        k.data[3] = el->getMajorP().y;
        k.data[4] = el->getRatio();
        k.data[5] = el->getAngle1();
        k.data[6] = el->getAngle2();
        k.data[7] = el->isReversed() ? 1.0 : 0.0;
        break;
    }
    default:
        break;
    }
    return k;
}

std::vector<PieceKey> pieceKeys(const RS_EntityContainer& trimmed) {
    std::vector<PieceKey> keys;
    for (const RS_Entity* e : trimmed) {
        if (e == nullptr) continue;
        keys.push_back(pieceKey(e));
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

// NOLINTEND(readability-identifier-naming)

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern fast path: only tiles away from the boundary are interior",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    RS_EntityContainer contour;
    auto opt = buildOptimizerFromRings(contour, {rectRing(0, 0, 500, 300)});
    auto roots = opt->getResults();
    REQUIRE(roots);
    REQUIRE_FALSE(roots->empty());
    const LC_LoopUtils::LC_Loops& root = (*roots)[0];

    SECTION("HEX") {
        // HEX tile 100 x 60: 5 x 5 grid for 500 x 300 but the two outer rows
        // of y-centres fall outside the rect, leaving 20 kept slots. Interior
        // slots are the 6 tiles whose expanded rect does not overlap the
        // boundary (columns 1-3 x rows 1-2).
        auto scaled = scaledPattern("HEX", 1.0);
        auto tileSlots = root.createTiles(*scaled, RS_Vector{0.0, 0.0});
        REQUIRE(tileSlots.size() == 20u);
        int interior = 0;
        for (const auto& s : tileSlots) if (s.interior) ++interior;
        CHECK(interior == 6);
    }
    SECTION("BRICK") {
        // BRICK tile 100 x 100: 5 x 3 kept. Interior: columns 1-3, row 1.
        auto scaled = scaledPattern("BRICK", 1.0);
        auto tileSlots = root.createTiles(*scaled, RS_Vector{0.0, 0.0});
        REQUIRE(tileSlots.size() == 15u);
        int interior = 0;
        for (const auto& s : tileSlots) if (s.interior) ++interior;
        CHECK(interior == 3);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern fast path: interior tiles draw exactly what clipping draws",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;

    auto compareFastAndSlow = [](const LC_LoopUtils::LC_Loops& root,
                                 const RS_Pattern& scaled) {
        auto tileSlots = root.createTiles(scaled, RS_Vector{0.0, 0.0});
        LC_HatchPieceBudget fast{std::numeric_limits<std::size_t>::max()};
        LC_HatchPieceBudget slow{std::numeric_limits<std::size_t>::max()};
        auto fastOut = root.trimPatternEntities(scaled, tileSlots, fast, true);
        auto slowOut = root.trimPatternEntities(scaled, tileSlots, slow, false);
        REQUIRE(fastOut);
        REQUIRE(slowOut);
        REQUIRE(fast.taken == slow.taken);
        const auto fastKeys = pieceKeys(*fastOut);
        const auto slowKeys = pieceKeys(*slowOut);
        REQUIRE(fastKeys.size() == slowKeys.size());
        for (size_t i = 0; i < fastKeys.size(); ++i) {
            CHECK(fastKeys[i] == slowKeys[i]);
        }
    };

    SECTION("500 x 300 rect, BRICK") {
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour, {rectRing(0, 0, 500, 300)});
        auto roots = opt->getResults();
        REQUIRE(roots);
        auto scaled = scaledPattern("BRICK", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("500 x 300 rect, HEX") {
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour, {rectRing(0, 0, 500, 300)});
        auto roots = opt->getResults();
        REQUIRE(roots);
        auto scaled = scaledPattern("HEX", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("500 x 300 rect, ANSI31") {
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour, {rectRing(0, 0, 500, 300)});
        auto roots = opt->getResults();
        REQUIRE(roots);
        auto scaled = scaledPattern("ANSI31", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("500 x 300 rect, PAISLEY") {
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour, {rectRing(0, 0, 500, 300)});
        auto roots = opt->getResults();
        REQUIRE(roots);
        auto scaled = scaledPattern("PAISLEY", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("Rect with a square island, BRICK") {
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour,
            {rectRing(0, 0, 400, 400), rectRing(100, 100, 300, 300)});
        auto roots = opt->getResults();
        REQUIRE(roots);
        REQUIRE_FALSE(roots->empty());
        auto scaled = scaledPattern("BRICK", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("Full circle, HEX") {
        auto contour = std::make_unique<RS_EntityContainer>(nullptr, true);
        contour->addEntity(new RS_Circle(contour.get(),
            RS_CircleData(RS_Vector(250.0, 150.0), 150.0)));
        contour->calculateBorders();
        LC_LoopUtils::LoopOptimizer opt(*contour);
        auto roots = opt.getResults();
        REQUIRE(roots);
        REQUIRE_FALSE(roots->empty());
        auto scaled = scaledPattern("HEX", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("Full ellipse rotated 20 deg, ANSI31") {
        auto contour = std::make_unique<RS_EntityContainer>(nullptr, true);
        RS_Vector major(240.0, 0.0);
        major.rotate(20.0 * M_PI / 180.0);
        contour->addEntity(new RS_Ellipse(contour.get(),
            RS_EllipseData{RS_Vector(250.0, 150.0), major, 140.0 / 240.0,
                           0.0, 0.0, false}));
        contour->calculateBorders();
        LC_LoopUtils::LoopOptimizer opt(*contour);
        auto roots = opt.getResults();
        REQUIRE(roots);
        REQUIRE_FALSE(roots->empty());
        auto scaled = scaledPattern("ANSI31", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("BRICK, slack-sensitive top-edge rect") {
        // Top edge sits a fraction of epsOn above the BRICK line y = 300.00003
        // (BRICK tile height is 100.00001 in the loaded pattern). The slack
        // keeps every top-row tile non-interior so keepPiece can decide
        // whether the top-line piece draws.
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour,
            {rectRing(0.0, 0.0, 500.0, 300.00003 + 1e-7)});
        auto roots = opt->getResults();
        REQUIRE(roots);
        auto scaled = scaledPattern("BRICK", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("U-in-U, BRICK") {
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour,
            {hatchfixOuterU(), hatchfixInnerU()});
        auto roots = opt->getResults();
        REQUIRE(roots);
        auto scaled = scaledPattern("BRICK", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
    SECTION("L wall, BRICK") {
        RS_EntityContainer contour;
        auto opt = buildOptimizerFromRings(contour,
            {hatchfixLOuter(), hatchfixLInner()});
        auto roots = opt->getResults();
        REQUIRE(roots);
        auto scaled = scaledPattern("BRICK", 1.0);
        compareFastAndSlow((*roots)[0], *scaled);
    }
}

// --- Tile-relative de-dup of LINE pieces. --------------------------------
// LC_LoopUtils::deduplicatePatternLines merges tile-border overlaps so that
// no ink is drawn twice. Collinear pieces whose offsets differ by at most
// tolOffset glue; adjacent dashes whose t-gap is <= tolOffset glue; and
// the output never depends on input order. tolOffset = 1e-6 x the scaled
// tile size, so scale changes the result accordingly.

namespace {

// NOLINTBEGIN(readability-identifier-naming)

struct AxisLineKey {
    bool vertical = false;
    double offset = 0.0;    // x for vertical, y for horizontal
    double start = 0.0;     // smaller t
    double end = 0.0;       // larger t
};

// Return a sorted sequence of maximal runs of the axis-aligned pieces of
// `before`, taking the union along each axis-aligned line. Only axis-aligned
// pieces are considered: SQUARE has horizontal and vertical ink only.
std::vector<AxisLineKey> unionRunsOfAxisAligned(const std::vector<Piece>& before,
                                                double tol) {
    std::unordered_map<long long, std::vector<std::pair<double, double>>> horizontals;
    std::unordered_map<long long, std::vector<std::pair<double, double>>> verticals;
    auto bucketKey = [tol](double v) -> long long {
        return static_cast<long long>(std::llround(v / tol));
    };
    for (const Piece& p : before) {
        const double dx = p.b.x - p.a.x;
        const double dy = p.b.y - p.a.y;
        if (std::abs(dy) < tol && std::abs(dx) > tol) {
            const long long k = bucketKey(0.5 * (p.a.y + p.b.y));
            horizontals[k].push_back({std::min(p.a.x, p.b.x), std::max(p.a.x, p.b.x)});
        } else if (std::abs(dx) < tol && std::abs(dy) > tol) {
            const long long k = bucketKey(0.5 * (p.a.x + p.b.x));
            verticals[k].push_back({std::min(p.a.y, p.b.y), std::max(p.a.y, p.b.y)});
        }
    }
    std::vector<AxisLineKey> runs;
    auto flush = [tol, &runs](const auto& bucketMap, bool vertical) {
        for (const auto& kv : bucketMap) {
            auto ranges = kv.second;
            std::sort(ranges.begin(), ranges.end());
            double curA = ranges.front().first;
            double curB = ranges.front().second;
            const double offset = static_cast<double>(kv.first) * tol;
            for (std::size_t i = 1; i < ranges.size(); ++i) {
                if (ranges[i].first <= curB + tol) {
                    curB = std::max(curB, ranges[i].second);
                } else {
                    runs.push_back({vertical, offset, curA, curB});
                    curA = ranges[i].first;
                    curB = ranges[i].second;
                }
            }
            runs.push_back({vertical, offset, curA, curB});
        }
    };
    flush(horizontals, false);
    flush(verticals, true);
    std::sort(runs.begin(), runs.end(),
              [](const AxisLineKey& a, const AxisLineKey& b) {
                  if (a.vertical != b.vertical) return !a.vertical && b.vertical;
                  if (a.offset != b.offset) return a.offset < b.offset;
                  return a.start < b.start;
              });
    return runs;
}

// Simple segment representation for sort/eq comparisons.
struct SortedLine {
    double x1;
    double y1;
    double x2;
    double y2;
    SortedLine(const RS_LineData& d) {
        RS_Vector a = d.startpoint;
        RS_Vector b = d.endpoint;
        if (b.x < a.x || (b.x == a.x && b.y < a.y)) std::swap(a, b);
        x1 = a.x; y1 = a.y;
        x2 = b.x; y2 = b.y;
    }
    bool operator<(const SortedLine& o) const {
        if (x1 != o.x1) return x1 < o.x1;
        if (y1 != o.y1) return y1 < o.y1;
        if (x2 != o.x2) return x2 < o.x2;
        return y2 < o.y2;
    }
    bool operator==(const SortedLine& o) const {
        return x1 == o.x1 && y1 == o.y1 && x2 == o.x2 && y2 == o.y2;
    }
};

std::vector<SortedLine> sortedLines(const std::vector<RS_LineData>& v) {
    std::vector<SortedLine> out;
    out.reserve(v.size());
    for (const RS_LineData& d : v) out.emplace_back(d);
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<RS_LineData> toLineData(const std::vector<Piece>& pieces) {
    std::vector<RS_LineData> out;
    out.reserve(pieces.size());
    for (const Piece& p : pieces) out.push_back(RS_LineData(p.a, p.b));
    return out;
}

std::vector<RS_LineData> hatchLineData(const RS_Hatch& h) {
    std::vector<RS_LineData> out;
    for (const auto& lp : lc::test::patternLines(h)) {
        out.push_back(RS_LineData(lp.a, lp.b));
    }
    return out;
}

// Equality of two line-data sets within an endpoint tolerance (used by case 21:
// the hatch's own output can move by one ulp through the rotate-by-zero path).
// Sorts by a quantised key (rounded to tolQuant), which keeps near-equal pieces
// in the same relative order across two independent computations, then checks
// each pair's endpoint distance.
bool approxLineSetsEqual(std::vector<RS_LineData> a, std::vector<RS_LineData> b, double tol) {
    if (a.size() != b.size()) return false;
    auto canon = [](RS_LineData& d) {
        if (d.endpoint.x < d.startpoint.x ||
            (d.endpoint.x == d.startpoint.x && d.endpoint.y < d.startpoint.y)) {
            std::swap(d.startpoint, d.endpoint);
        }
    };
    for (auto& d : a) canon(d);
    for (auto& d : b) canon(d);
    const double tolQuant = std::max(tol * 100.0, 1e-7);
    auto quant = [tolQuant](double v) -> long long {
        return static_cast<long long>(std::llround(v / tolQuant));
    };
    auto key = [&](const RS_LineData& x) {
        return std::array<long long, 4>{
            quant(x.startpoint.x), quant(x.startpoint.y),
            quant(x.endpoint.x), quant(x.endpoint.y)};
    };
    auto cmp = [&](const RS_LineData& x, const RS_LineData& y) {
        return key(x) < key(y);
    };
    std::sort(a.begin(), a.end(), cmp);
    std::sort(b.begin(), b.end(), cmp);
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].startpoint.distanceTo(b[i].startpoint) > tol) return false;
        if (a[i].endpoint.distanceTo(b[i].endpoint) > tol) return false;
    }
    return true;
}

// NOLINTEND(readability-identifier-naming)

}  // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern de-dup: SQUARE draws every line once",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    auto hatch = lc::test::makePatternHatch(
        "SQUARE", 1.0, 0.0, {rectRing(0, 0, 500, 300)});
    REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
    const auto hatchPieces = lc::test::patternLines(*hatch);
    REQUIRE_FALSE(hatchPieces.empty());

    const auto before = tilePathPieces("SQUARE", 1.0, {rectRing(0, 0, 500, 300)});
    REQUIRE_FALSE(before.empty());

    const double tol = 5e-5;

    // Σ child length equals the union length of before (axis-aligned union
    // computed in the test) to 1e-9 relative.
    const auto runs = unionRunsOfAxisAligned(before, tol);
    double unionLen = 0.0;
    for (const auto& r : runs) unionLen += (r.end - r.start);
    const double drawnInk = inkOf(hatchPieces);
    INFO("drawnInk=" << drawnInk << " unionLen=" << unionLen);
    REQUIRE(std::abs(drawnInk - unionLen) <= 1e-9 * unionLen);

    // No two collinear children overlap by more than tol.
    for (std::size_t i = 0; i < hatchPieces.size(); ++i) {
        const Piece& a = hatchPieces[i];
        const double adx = a.b.x - a.a.x;
        const double ady = a.b.y - a.a.y;
        for (std::size_t j = i + 1; j < hatchPieces.size(); ++j) {
            const Piece& b = hatchPieces[j];
            const double bdx = b.b.x - b.a.x;
            const double bdy = b.b.y - b.a.y;
            const bool bothH = std::abs(ady) < tol && std::abs(bdy) < tol;
            const bool bothV = std::abs(adx) < tol && std::abs(bdx) < tol;
            if (!bothH && !bothV) continue;
            if (bothH && std::abs(a.a.y - b.a.y) > tol) continue;
            if (bothV && std::abs(a.a.x - b.a.x) > tol) continue;
            double aLo, aHi, bLo, bHi;
            if (bothH) {
                aLo = std::min(a.a.x, a.b.x);
                aHi = std::max(a.a.x, a.b.x);
                bLo = std::min(b.a.x, b.b.x);
                bHi = std::max(b.a.x, b.b.x);
            } else {
                aLo = std::min(a.a.y, a.b.y);
                aHi = std::max(a.a.y, a.b.y);
                bLo = std::min(b.a.y, b.b.y);
                bHi = std::max(b.a.y, b.b.y);
            }
            const double overlap = std::min(aHi, bHi) - std::max(aLo, bLo);
            INFO("overlap=" << overlap);
            REQUIRE(overlap <= tol);
        }
    }

    // The child count equals the number of maximal runs in that union.
    CHECK(hatchPieces.size() == runs.size());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern de-dup: ANSI31 keeps its ink and glues tile-border pieces",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const auto before = tilePathPieces("ANSI31", 1.0, {rectRing(0, 0, 500, 300)});
    REQUIRE_FALSE(before.empty());

    auto scaled = scaledPattern("ANSI31", 1.0);
    const RS_Vector pSize = scaled->getSize();
    const double tolOffset = 1e-6 * std::max(pSize.x, pSize.y);

    const auto after = LC_LoopUtils::deduplicatePatternLines(
        toLineData(before), tolOffset);
    REQUIRE_FALSE(after.empty());

    double inkBefore = 0.0;
    for (const Piece& p : before) inkBefore += p.a.distanceTo(p.b);
    double inkAfter = 0.0;
    for (const RS_LineData& d : after) inkAfter += d.startpoint.distanceTo(d.endpoint);
    CAPTURE(inkBefore);
    CAPTURE(inkAfter);
    CAPTURE(before.size());
    CAPTURE(after.size());
    REQUIRE(std::abs(inkAfter - inkBefore) <= 1e-6 * inkBefore);
    REQUIRE(after.size() < before.size());

    // No collinear overlap greater than tol in the output: group by (direction,
    // perpendicular offset) and check that intervals are disjoint.
    struct Bucket { double offset; std::vector<std::pair<double, double>> intervals; };
    std::unordered_map<long long, std::vector<Bucket>> byDir;
    const double tolDir = 1e-6;
    for (const RS_LineData& d : after) {
        const double dx = d.endpoint.x - d.startpoint.x;
        const double dy = d.endpoint.y - d.startpoint.y;
        const double L = std::hypot(dx, dy);
        if (L < tolOffset) continue;
        double theta = std::atan2(dy, dx);
        if (theta < 0.0) theta += M_PI;
        if (theta >= M_PI) theta -= M_PI;
        const long long dirKey = static_cast<long long>(std::llround(theta / tolDir));
        const double ux = std::cos(theta);
        const double uy = std::sin(theta);
        const double nx = -uy, ny = ux;
        const double c = 0.5 * ((d.startpoint.x + d.endpoint.x) * nx +
                                (d.startpoint.y + d.endpoint.y) * ny);
        double t0 = d.startpoint.x * ux + d.startpoint.y * uy;
        double t1 = d.endpoint.x * ux + d.endpoint.y * uy;
        if (t1 < t0) std::swap(t0, t1);
        auto& buckets = byDir[dirKey];
        Bucket* found = nullptr;
        for (Bucket& bk : buckets) {
            if (std::abs(bk.offset - c) <= tolOffset) { found = &bk; break; }
        }
        if (!found) {
            buckets.push_back({c, {}});
            found = &buckets.back();
        }
        found->intervals.push_back({t0, t1});
    }
    for (auto& kv : byDir) {
        for (Bucket& bk : kv.second) {
            std::sort(bk.intervals.begin(), bk.intervals.end());
            for (std::size_t i = 1; i < bk.intervals.size(); ++i) {
                const double overlap = bk.intervals[i - 1].second - bk.intervals[i].first;
                INFO("overlap=" << overlap);
                REQUIRE(overlap <= tolOffset);
            }
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern de-dup: the result does not depend on input order",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;

    auto checkInvariant = [](const QString& name) {
        const auto before = tilePathPieces(name, 1.0, {rectRing(0, 0, 500, 300)});
        REQUIRE_FALSE(before.empty());

        auto scaled = scaledPattern(name, 1.0);
        const RS_Vector pSize = scaled->getSize();
        const double tolOffset = 1e-6 * std::max(pSize.x, pSize.y);

        const auto baseline = LC_LoopUtils::deduplicatePatternLines(
            toLineData(before), tolOffset);
        REQUIRE_FALSE(baseline.empty());

        auto shuffled = toLineData(before);
        std::mt19937 rng(20261025u);
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        const auto out = LC_LoopUtils::deduplicatePatternLines(
            std::move(shuffled), tolOffset);

        const auto a = sortedLines(baseline);
        const auto b = sortedLines(out);
        REQUIRE(a.size() == b.size());
        for (std::size_t i = 0; i < a.size(); ++i) {
            REQUIRE(a[i] == b[i]);
        }
    };

    SECTION("SQUARE") { checkInvariant("SQUARE"); }
    SECTION("ANSI31") { checkInvariant("ANSI31"); }
    SECTION("HEX") { checkInvariant("HEX"); }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern de-dup: the tolerance follows the tile, not the drawing",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;

    auto scaleBy = [](const std::vector<Piece>& pieces, double s) {
        std::vector<RS_LineData> out;
        out.reserve(pieces.size());
        for (const Piece& p : pieces) {
            out.push_back(RS_LineData(RS_Vector(p.a.x * s, p.a.y * s),
                                      RS_Vector(p.b.x * s, p.b.y * s)));
        }
        return out;
    };

    SECTION("SAND at scale 5e-4 keeps every piece") {
        const auto before = tilePathPieces("SAND", 1.0, {rectRing(0, 0, 500, 300)});
        REQUIRE_FALSE(before.empty());
        auto scaled = scaledPattern("SAND", 5e-4);
        const RS_Vector pSize = scaled->getSize();
        const double tolOffset = 1e-6 * std::max(pSize.x, pSize.y);
        auto scaledInput = scaleBy(before, 5e-4);
        double inkIn = 0.0;
        for (const RS_LineData& d : scaledInput) inkIn += d.startpoint.distanceTo(d.endpoint);

        const auto after = LC_LoopUtils::deduplicatePatternLines(
            scaledInput, tolOffset);
        double inkOut = 0.0;
        for (const RS_LineData& d : after) inkOut += d.startpoint.distanceTo(d.endpoint);
        CAPTURE(before.size());
        CAPTURE(after.size());
        CAPTURE(inkIn);
        CAPTURE(inkOut);
        REQUIRE(after.size() == before.size());
        REQUIRE(std::abs(inkOut / inkIn - 1.0) <= 5e-5);

        // Sensitivity: at the drawing-scale absolute tolerance 1e-4 (the
        // refuted alternative), every piece is shorter than the tolerance. The
        // plan's own-adaptation (keep the original short piece, never emit a
        // zero-length RS_Line) prevents the prototype's dot-collapse: ink and
        // piece count stay within a few percent of the input. One tolerance
        // coarser than the whole scaled tile (~0.05) collapses user-visible
        // geometry; 1e-4 here does not.
        const auto afterBad = LC_LoopUtils::deduplicatePatternLines(
            scaledInput, 1e-4);
        double inkBad = 0.0;
        for (const RS_LineData& d : afterBad) inkBad += d.startpoint.distanceTo(d.endpoint);
        CAPTURE(inkBad);
        REQUIRE(afterBad.size() >= 0.95 * before.size());
        REQUIRE(inkBad >= 0.95 * inkIn);
    }
    SECTION("GOST_GLASS1 at scale 1e-4 keeps every piece") {
        const auto before = tilePathPieces("GOST_GLASS1", 1.0, {rectRing(0, 0, 500, 300)});
        REQUIRE_FALSE(before.empty());
        auto scaled = scaledPattern("GOST_GLASS1", 1e-4);
        const RS_Vector pSize = scaled->getSize();
        const double tolOffset = 1e-6 * std::max(pSize.x, pSize.y);
        auto scaledInput = scaleBy(before, 1e-4);
        double inkIn = 0.0;
        for (const RS_LineData& d : scaledInput) inkIn += d.startpoint.distanceTo(d.endpoint);

        const auto after = LC_LoopUtils::deduplicatePatternLines(
            scaledInput, tolOffset);
        double inkOut = 0.0;
        for (const RS_LineData& d : after) inkOut += d.startpoint.distanceTo(d.endpoint);
        CAPTURE(before.size());
        CAPTURE(after.size());
        REQUIRE(after.size() == before.size());
        REQUIRE(std::abs(inkOut / inkIn - 1.0) <= 5e-5);
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern de-dup: HEX seams merge into double dashes only",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;
    const auto before = tilePathPieces("HEX", 1.0, {rectRing(0, 0, 500, 300)});
    REQUIRE_FALSE(before.empty());
    auto scaled = scaledPattern("HEX", 1.0);
    const RS_Vector pSize = scaled->getSize();
    const double tolOffset = 1e-6 * std::max(pSize.x, pSize.y);

    const auto after = LC_LoopUtils::deduplicatePatternLines(
        toLineData(before), tolOffset);
    REQUIRE_FALSE(after.empty());

    double maxLen = 0.0;
    for (const RS_LineData& d : after) {
        maxLen = std::max(maxLen, d.startpoint.distanceTo(d.endpoint));
    }
    CAPTURE(maxLen);
    CAPTURE(before.size());
    CAPTURE(after.size());
    REQUIRE(maxLen <= 9.60);
    REQUIRE(after.size() < before.size());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Hatch pattern de-dup: the hatch draws the de-duplicated tile path",
          "[lc-hatchfix][pattern]") {
    lc::test::SourcePatterns patterns;

    auto checkHatch = [](const QString& name) {
        auto hatch = lc::test::makePatternHatch(
            name, 1.0, 0.0, {rectRing(0, 0, 500, 300)});
        REQUIRE(hatch->getUpdateError() == RS_Hatch::HATCH_OK);
        const auto hatchData = hatchLineData(*hatch);
        REQUIRE_FALSE(hatchData.empty());

        const auto before = tilePathPieces(name, 1.0, {rectRing(0, 0, 500, 300)});
        auto scaled = scaledPattern(name, 1.0);
        const RS_Vector pSize = scaled->getSize();
        const double tolOffset = 1e-6 * std::max(pSize.x, pSize.y);
        const auto expected = LC_LoopUtils::deduplicatePatternLines(
            toLineData(before), tolOffset);
        REQUIRE_FALSE(expected.empty());

        CAPTURE(hatchData.size());
        CAPTURE(expected.size());
        CAPTURE(before.size());
        REQUIRE(approxLineSetsEqual(hatchData, expected, 1e-9));
    };

    SECTION("SQUARE") { checkHatch("SQUARE"); }
    SECTION("HEX") { checkHatch("HEX"); }
}
