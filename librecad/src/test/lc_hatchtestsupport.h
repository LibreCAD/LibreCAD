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

#include <memory>
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

/**
 * A pattern hatch covering `rings`. Each ring becomes an RS_EntityContainer
 * loop of RS_Line edges joining consecutive vertices (and the last back to
 * the first). `parent` is kept null unless the caller needs the hatch inside
 * a drawing or block; the hatch is updated before it is returned.
 */
inline std::unique_ptr<RS_Hatch> makePatternHatch(
    const QString& pattern, double scale, double angle,
    const std::vector<Ring>& rings, RS_EntityContainer* parent = nullptr) {
    auto hatch = std::make_unique<RS_Hatch>(
        parent, RS_HatchData(false, scale, angle, pattern));
    for (const Ring& ring : rings) {
        if (ring.size() < 3) continue;
        auto* loop = new RS_EntityContainer(hatch.get());
        hatch->addEntity(loop);
        for (size_t i = 0; i < ring.size(); ++i) {
            const RS_Vector& a = ring[i];
            const RS_Vector& b = ring[(i + 1) % ring.size()];
            loop->addEntity(new RS_Line(loop, a, b));
        }
    }
    hatch->update();
    return hatch;
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
