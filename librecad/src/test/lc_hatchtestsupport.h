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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QColor>
#include <QImage>
#include <QString>
#include <QStringLiteral>

#include "lc_actiontestsupport.h"
#include "lc_graphicviewport.h"
#include "lc_rect.h"
#include "lc_settingguard.h"
#include "rs.h"
#include "rs_color.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_painter.h"
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

/**
 * Minimal raster rig for solid-fill tests. Defaults are the historical 256
 * px canvas, 1:1 WCS mapping, y-axis flipped, origin at (0, 0): the hatch
 * tests built against those defaults keep rendering identical ink.
 *
 * `fitting(box, size)` centres a bounding box in a square canvas with a
 * 20-pixel margin: f = (size - 40) / max(w, h) and the offsets place the
 * box's own min at image coordinates (20, size - 20).
 *
 * `filledAt(wcsX, wcsY)` reads the pixel at `toGuiX(wcsX)`/`toGuiY(wcsY)`;
 * with the defaults that is `round(wcsX)` and `round(size - wcsY)`.
 */
class TestPainter {
public:
    TestPainter() : TestPainter(256, 1.0, RS_Vector(0.0, 0.0)) {}

    TestPainter(int size, double factor, const RS_Vector& offset)
        : m_size(size),
          m_factor(factor),
          m_offset(offset),
          image(size, size, QImage::Format_RGB32),
          painter(&image) {
        image.fill(Qt::white);
        viewport.setSize(size, size);
        viewport.justSetOffsetAndFactor(offset.x, offset.y, factor);
        painter.setViewPort(&viewport);
        // Wide bounding rect so arc segments are never clipped as out-of-view.
        const double w = static_cast<double>(size) / factor;
        LC_Rect bounds{RS_Vector(-w, -w) + offset,
                       RS_Vector(2.0 * w, 2.0 * w) + offset};
        painter.setWorldBoundingRect(bounds);
        painter.setPen(RS_Color(0, 0, 0));
    }

    /** Builds a TestPainter whose canvas just covers box with a margin. */
    static TestPainter fitting(const LC_Rect& box, int size = 600) {
        const RS_Vector mn = box.minP();
        const RS_Vector mx = box.maxP();
        const double w = std::max(1e-12, mx.x - mn.x);
        const double h = std::max(1e-12, mx.y - mn.y);
        const double f = static_cast<double>(size - 40) / std::max(w, h);
        // Place box's min at image coordinates (20, size - 20):
        //   toGuiX(mn.x) = (mn.x - offX) * f = 20
        //   toGuiY(mn.y) = size - (mn.y - offY) * f = size - 20
        const double offX = mn.x - 20.0 / f;
        const double offY = mn.y - 20.0 / f;
        return TestPainter(size, f, RS_Vector(offX, offY));
    }

    bool filledAt(double wcsX, double wcsY) const {
        const int px = static_cast<int>(
            std::round((wcsX - m_offset.x) * m_factor));
        const int py = static_cast<int>(
            std::round(m_size - (wcsY - m_offset.y) * m_factor));
        if (px < 0 || px >= m_size || py < 0 || py >= m_size) {
            return false;
        }
        return image.pixel(px, py) != qRgb(255, 255, 255);
    }

    // Legacy canvas constants used by existing tests.
    static constexpr int W = 256;
    static constexpr int H = 256;

    QImage image;
    LC_GraphicViewport viewport;
    RS_Painter painter;

private:
    int m_size = 256;
    double m_factor = 1.0;
    RS_Vector m_offset{0.0, 0.0};
};

} // namespace lc::test

#endif // LC_HATCHTESTSUPPORT_H
