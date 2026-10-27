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

#ifndef LC_HATCHTESTSUPPORT_H
#define LC_HATCHTESTSUPPORT_H

// Shared hatch-test helpers for librecad_tests only. The header walks the
// bundled hatch patterns under librecad/support/patterns and must never be
// included from a headless fast-lane target.

#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QString>
#include <QStringLiteral>

#include "lc_actiontestsupport.h"
#include "lc_settingguard.h"
#include "rs.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_settings.h"
#include "rs_vector.h"

namespace lc::test {

// Points the pattern search at the source tree's patterns while this guard
// lives. Builds a QApplication through lc::test::application() so a hatch's
// update() path (which eventually asks RS_Settings for the pattern directory)
// has a running Qt event loop.
class SourcePatterns {
public:
    SourcePatterns() {
        REQUIRE(m_qtReady);
        m_patterns.set(QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/patterns"));
    }

private:
    const bool m_qtReady{lc::test::application() != nullptr};
    lc::test::SettingGuard m_patterns{RS_SETTINGS, "Paths", "Patterns"};
};

using Ring = std::vector<RS_Vector>;

/** Four-corner CCW rectangle (x1,y1)..(x2,y2). */
inline Ring rectRing(double x1, double y1, double x2, double y2) {
    return {RS_Vector(x1, y1), RS_Vector(x2, y1), RS_Vector(x2, y2), RS_Vector(x1, y2)};
}

/** The same ring translated by shift. */
inline Ring shiftedRing(const Ring& ring, const RS_Vector& shift) {
    Ring out;
    out.reserve(ring.size());
    for (const RS_Vector& v : ring) {
        out.push_back(v + shift);
    }
    return out;
}

/** Fills `container` with the ring's edges (closing the ring). */
inline void makePolygonLoop(RS_EntityContainer& container, const Ring& pts) {
    const size_t n = pts.size();
    for (size_t i = 0; i < n; ++i) {
        const RS_Vector& a = pts[i];
        const RS_Vector& b = pts[(i + 1) % n];
        container.addEntity(new RS_Line(&container, a, b));
    }
}

/**
 * Builds a hatch from ready-made loop containers. Each loop container is
 * reparented into the hatch; the hatch's update() runs after every loop is
 * attached. Any atomic edge type is accepted (RS_Arc, RS_Circle, RS_Ellipse,
 * LC_Parabola, LC_SplinePoints, LC_Hyperbola, ...).
 */
inline std::unique_ptr<RS_Hatch> makeHatch(
    std::vector<std::unique_ptr<RS_EntityContainer>> loops,
    const RS_HatchData& data,
    RS_EntityContainer* parent = nullptr) {
    auto hatch = std::make_unique<RS_Hatch>(parent, data);
    for (auto& loop : loops) {
        if (!loop) continue;
        loop->setParent(hatch.get());
        hatch->addEntity(loop.release());
    }
    hatch->update();
    return hatch;
}

/**
 * Builds a hatch from rings of vertices. Each ring becomes a container of
 * RS_Line edges joining consecutive vertices (and the last back to the first);
 * forwards to the container overload.
 */
inline std::unique_ptr<RS_Hatch> makeHatch(
    const std::vector<Ring>& rings,
    const RS_HatchData& data,
    RS_EntityContainer* parent = nullptr) {
    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    loops.reserve(rings.size());
    for (const Ring& ring : rings) {
        if (ring.size() < 3) continue;
        auto loop = std::make_unique<RS_EntityContainer>(nullptr);
        makePolygonLoop(*loop, ring);
        loops.push_back(std::move(loop));
    }
    return makeHatch(std::move(loops), data, parent);
}

/**
 * A pattern hatch covering `rings`. Forwards to makeHatch; keeps the same
 * signature as the earlier helper so existing callers do not need to change.
 */
inline std::unique_ptr<RS_Hatch> makePatternHatch(
    const QString& pattern, double scale, double angle,
    const std::vector<Ring>& rings, RS_EntityContainer* parent = nullptr) {
    return makeHatch(rings, RS_HatchData(false, scale, angle, pattern), parent);
}

/**
 * Exact integer even-odd test. poly is a closed polygon; the point (x,y) is
 * tested against it. When onBoundary != nullptr, it is set to true when the
 * point lies exactly on an edge (then the return value is unspecified but set
 * to true as a convention). All arithmetic is integer: there is no roundoff.
 */
inline bool exactInsideInteger(
    const std::vector<std::array<long long, 2>>& poly,
    long long x, long long y, bool* onBoundary = nullptr) {
    if (onBoundary != nullptr) *onBoundary = false;
    const size_t n = poly.size();
    if (n < 3) return false;
    bool inside = false;
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        const long long xi = poly[i][0], yi = poly[i][1];
        const long long xj = poly[j][0], yj = poly[j][1];
        // On-boundary check: collinear and within the edge's bbox.
        const long long dxE = xj - xi;
        const long long dyE = yj - yi;
        const long long dxP = x - xi;
        const long long dyP = y - yi;
        const long long cross = dxE * dyP - dyE * dxP;
        if (cross == 0) {
            const long long dot = dxE * dxP + dyE * dyP;
            const long long len2 = dxE * dxE + dyE * dyE;
            if (dot >= 0 && dot <= len2) {
                if (onBoundary != nullptr) *onBoundary = true;
                return true;
            }
        }
        // Half-open crossing: yi <= y < yj or yj <= y < yi
        const bool yiLeY = (yi <= y);
        const bool yjLeY = (yj <= y);
        const bool crosses = (yiLeY != yjLeY);
        if (crosses) {
            // x-intercept: xi + (y - yi) * (xj - xi) / (yj - yi)
            // Avoid division: compare (x - xi) * (yj - yi) with (y - yi) * (xj - xi)
            // using the sign of (yj - yi).
            const long long rhs = (y - yi) * dxE;
            const long long lhs = (x - xi) * dyE;
            const bool side = dyE > 0 ? (lhs < rhs) : (lhs > rhs);
            if (side) inside = !inside;
        }
    }
    return inside;
}

/** A pattern child's position in world space, start and end. */
struct LinePiece {
    RS_Vector a;
    RS_Vector b;
};

/**
 * Clones of the direct children of hatch that carry FlagHatchChild. The caller
 * owns the clones. H18-3 reimplements this on materializePattern() so tests
 * that read pieces keep working after pattern children are gone.
 */
inline std::vector<std::unique_ptr<RS_Entity>> patternPieces(const RS_Hatch& hatch) {
    std::vector<std::unique_ptr<RS_Entity>> out;
    for (const RS_Entity* en : hatch) {
        if (en && en->getFlag(RS2::FlagHatchChild)) {
            out.emplace_back(en->clone());
        }
    }
    return out;
}

/** The LINE pieces of the hatch, in world coordinates. */
inline std::vector<LinePiece> patternLines(const RS_Hatch& hatch) {
    std::vector<LinePiece> out;
    for (const RS_Entity* en : hatch) {
        if (en && en->getFlag(RS2::FlagHatchChild) && en->rtti() == RS2::EntityLine) {
            const auto* line = static_cast<const RS_Line*>(en);
            out.push_back({line->getStartpoint(), line->getEndpoint()});
        }
    }
    return out;
}

/** Count of pattern pieces (FlagHatchChild direct children). */
inline std::size_t patternPieceCount(const RS_Hatch& hatch) {
    std::size_t count = 0;
    for (const RS_Entity* en : hatch) {
        if (en && en->getFlag(RS2::FlagHatchChild)) ++count;
    }
    return count;
}

} // namespace lc::test

#endif // LC_HATCHTESTSUPPORT_H
