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
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "lc_hatchtestsupport.h"
#include "lc_looputils.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_pattern.h"
#include "rs_patternlist.h"

namespace {

using Piece = lc::test::LinePiece;
using lc::test::Ring;
using lc::test::rectRing;

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
        auto trimmed = root.trimPatternEntities(*pattern);
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

    // Range oracle until H05 provides deterministic equality.
    const double countRatio = static_cast<double>(fixed.size()) /
                              static_cast<double>(carpet.size());
    const double inkRatio = inkOf(fixed) / inkOf(carpet);
    INFO("fixed=" << fixed.size() << " carpet=" << carpet.size()
                  << " countRatio=" << countRatio << " inkRatio=" << inkRatio);
    REQUIRE(countRatio > 0.80);
    REQUIRE(countRatio < 1.20);
    REQUIRE(inkRatio > 0.80);
    REQUIRE(inkRatio < 1.20);
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

    const double countRatio = static_cast<double>(fixed.size()) /
                              static_cast<double>(carpet.size());
    const double inkRatio = inkOf(fixed) / inkOf(carpet);
    INFO("fixed=" << fixed.size() << " carpet=" << carpet.size()
                  << " countRatio=" << countRatio << " inkRatio=" << inkRatio);
    REQUIRE(countRatio > 0.60);
    REQUIRE(countRatio < 1.40);
    REQUIRE(inkRatio > 0.60);
    REQUIRE(inkRatio < 1.40);
    // matchedFraction is informational: pieces may rotate within a tile but
    // keep near-equal lengths and midpoints.
    const double matched = matchedFraction(fixed, carpet, 2.5);
    INFO("matched=" << matched);
    CHECK(matched > 0.30);
}
