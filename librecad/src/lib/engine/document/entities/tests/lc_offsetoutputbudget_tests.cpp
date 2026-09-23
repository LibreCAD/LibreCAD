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

#include <catch2/catch_test_macros.hpp>

#include <limits>

#include "lc_offsetoutputbudget.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_spline.h"

namespace {
RS_Spline cubicPiece() {
    RS_SplineData d(3, false);
    d.controlPoints = {{0, 0}, {1, 2}, {3, 2}, {4, 0}};
    d.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
    d.weights.assign(4, 1.0);
    return RS_Spline(nullptr, d);
}
} // namespace

TEST_CASE("The output walker counts the leaves under each root", "[curve-offset][budget]") {
    const RS_Spline spline = cubicPiece();
    const RS_Line line{nullptr, RS_LineData{{0, 0}, {1, 1}}};
    REQUIRE(spline.count() == 32);

    const LC_OffsetTreeCost cost = measureOffsetOutput({&spline, &line}, std::numeric_limits<std::size_t>::max());
    CHECK(cost.status == LC_OffsetTreeStatus::Ok);
    CHECK(cost.deepEntities == 33); // 32 display segments and one line
    CHECK(measureOffsetOutput({}, 0).status == LC_OffsetTreeStatus::Ok);

    // nested containers are followed down to their leaves
    RS_EntityContainer outer{nullptr, true};
    auto* inner = new RS_EntityContainer{&outer, true};
    inner->addEntity(new RS_Line{inner, RS_LineData{{0, 0}, {1, 0}}});
    inner->addEntity(new RS_Line{inner, RS_LineData{{1, 0}, {1, 1}}});
    outer.addEntity(inner);
    outer.addEntity(new RS_Line{&outer, RS_LineData{{1, 1}, {0, 1}}});
    CHECK(measureOffsetOutput({&outer}, 100).deepEntities == 3);
}

TEST_CASE("The output walker stops at its cap", "[curve-offset][budget]") {
    const RS_Spline spline = cubicPiece();
    const LC_OffsetTreeCost capped = measureOffsetOutput({&spline}, 31);
    CHECK(capped.status == LC_OffsetTreeStatus::LimitExceeded);
    CHECK(capped.deepEntities == 31);
    CHECK(measureOffsetOutput({&spline}, 32).status == LC_OffsetTreeStatus::Ok);
    CHECK(measureOffsetOutput({&spline}, 0).status == LC_OffsetTreeStatus::LimitExceeded);
}

TEST_CASE("The output walker rejects a malformed tree", "[curve-offset][budget]") {
    const RS_Spline spline = cubicPiece();
    CHECK(measureOffsetOutput({&spline, &spline}, 1000).status == LC_OffsetTreeStatus::InvalidTree);
    CHECK(measureOffsetOutput({nullptr}, 1000).status == LC_OffsetTreeStatus::InvalidTree);

    // a child shared by two roots (a container itself refuses to list one
    // entity twice, so this is how a tree ends up counting a leaf twice)
    RS_Line shared{nullptr, RS_LineData{{0, 0}, {1, 0}}};
    RS_EntityContainer first{nullptr, false};
    RS_EntityContainer second{nullptr, false};
    first.addEntity(&shared);
    second.addEntity(&shared);
    CHECK(measureOffsetOutput({&first, &second}, 1000).status == LC_OffsetTreeStatus::InvalidTree);

    // a container inside itself
    RS_EntityContainer cyclic{nullptr, false};
    cyclic.addEntity(&cyclic);
    CHECK(measureOffsetOutput({&cyclic}, 1000).status == LC_OffsetTreeStatus::InvalidTree);
    cyclic.clear();
    first.clear();
    second.clear();
}

TEST_CASE("The default limits bind together", "[curve-offset][budget]") {
    const LC_OffsetSourceBudget budget = makeDefaultOffsetSourceBudget();
    CHECK(isValidOffsetBudget(budget));
    CHECK_FALSE(isValidOffsetBudget(LC_OffsetSourceBudget{}));
    // every output spline carries its display segments: the deep limit admits
    // exactly as many spline pieces as the piece limit
    const RS_Spline spline = cubicPiece();
    CHECK(spline.count() <= kOffsetDeepEntitiesPerOutputSpline);
    CHECK(budget.maxDeepEntities == budget.maxOutputEntities * kOffsetDeepEntitiesPerOutputSpline);
    CHECK(budget.maxOutputEntities == budget.maxCubicPieces);
    CHECK(kDefaultOffsetDeepEntitiesPerRequest >= budget.maxDeepEntities);
}
