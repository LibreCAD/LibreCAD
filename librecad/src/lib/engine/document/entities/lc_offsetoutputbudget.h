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

#ifndef LC_OFFSETOUTPUTBUDGET_H
#define LC_OFFSETOUTPUTBUDGET_H

#include <cstddef>

/**
 * Hard limits on what offsetting one source may produce, over all the copies
 * requested for it. They keep hostile input from stalling the program; unlike
 * the Appearance/MaxPreview preference they decide whether an offset succeeds.
 * A zero field is invalid: every engine entry rejects a budget with one.
 */
struct LC_OffsetSourceBudget {
    std::size_t maxCubicPieces{0};
    /** Entities handed to the document: one per cubic piece, or one exact line. */
    std::size_t maxOutputEntities{0};
    /** Leaf entities in the output trees, the display segments of each spline included. */
    std::size_t maxDeepEntities{0};
};

/** What an offset produced, measured before any entity is released. */
struct LC_OffsetOutputUsage {
    std::size_t cubicPieces{0};
    std::size_t outputEntities{0};
    std::size_t deepEntities{0};
};

/**
 * Every output spline carries the line segments RS_Spline::update() draws it
 * with: 32 for an open spline. Deep limits are multiples of this, so the piece,
 * entity and deep limits bind together instead of one making the others moot.
 */
inline constexpr std::size_t kOffsetDeepEntitiesPerOutputSpline = 33;

inline constexpr std::size_t kDefaultOffsetCubicPiecesPerSource = 4096U;
inline constexpr std::size_t kDefaultOffsetOutputEntitiesPerSource = kDefaultOffsetCubicPiecesPerSource;
inline constexpr std::size_t kDefaultOffsetDeepEntitiesPerSource =
    kDefaultOffsetOutputEntitiesPerSource * kOffsetDeepEntitiesPerOutputSpline;
/** Four maximal sources; a later source that no longer fits fails without being evaluated. */
inline constexpr std::size_t kDefaultOffsetDeepEntitiesPerRequest = 4U * kDefaultOffsetDeepEntitiesPerSource;

inline LC_OffsetSourceBudget makeDefaultOffsetSourceBudget() {
    return {kDefaultOffsetCubicPiecesPerSource, kDefaultOffsetOutputEntitiesPerSource,
            kDefaultOffsetDeepEntitiesPerSource};
}

inline bool isValidOffsetBudget(const LC_OffsetSourceBudget& budget) {
    return budget.maxCubicPieces > 0 && budget.maxOutputEntities > 0 && budget.maxDeepEntities > 0;
}

#endif
