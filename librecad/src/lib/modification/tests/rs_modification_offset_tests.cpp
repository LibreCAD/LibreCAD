/*******************************************************************************
**
** This file is part of the LibreCAD project, unit tests for RS_Modification
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

// RS_Modification::offset() used to queue every selected original for deletion
// when "keep originals" was off, whether or not an offset had been produced for
// it, and to report success unconditionally. An inward offset past a circle's
// radius therefore removed the circle, created nothing, and claimed success.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "rs_circle.h"
#include "rs_document.h"
#include "rs_modification.h"
#include "rs_vector.h"

namespace {

RS_OffsetData inwardOffset(const double distance) {
    RS_OffsetData data;
    data.coord = RS_Vector{0.0, 0.0}; // the circles below are centred here
    data.distance = distance;
    data.keepOriginals = false;
    data.useCurrentLayer = true;
    data.useCurrentAttributes = true;
    return data;
}

// The batch never owns what it holds; tests that do not apply it free the additions.
struct BatchGuard {
    LC_DocumentModificationBatch ctx;
    ~BatchGuard() { qDeleteAll(ctx.entitiesToAdd); }
};

} // namespace

TEST_CASE("RS_Modification::offset keeps a source it could not offset",
          "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    BatchGuard guard;

    const bool result = RS_Modification::offset(inwardOffset(10.0), {&circle}, false, guard.ctx);

    CHECK_FALSE(result);
    CHECK_FALSE(guard.ctx.success);
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("RS_Modification::offset removes only the sources it offset",
          "[modification][offset]") {
    RS_Circle small{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    RS_Circle large{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    BatchGuard guard;

    const bool result = RS_Modification::offset(inwardOffset(10.0), {&small, &large}, false, guard.ctx);

    CHECK(result);
    CHECK(guard.ctx.success);
    REQUIRE(guard.ctx.entitiesToAdd.size() == 1);
    const auto* offsetCircle = dynamic_cast<RS_Circle*>(guard.ctx.entitiesToAdd.front());
    REQUIRE(offsetCircle != nullptr);
    CHECK(offsetCircle->getRadius() == Catch::Approx(10.0));
    REQUIRE(guard.ctx.entitiesToDelete.size() == 1);
    CHECK(guard.ctx.entitiesToDelete.front() == &large);
}

TEST_CASE("RS_Modification::offset publishes a source's copies together or not at all",
          "[modification][offset]") {
    // Radius 5, three inward copies 2 apart: radii 3 and 1 exist, the third does not.
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 5.0}};
    RS_OffsetData data = inwardOffset(2.0);
    data.multipleCopies = true;
    data.number = 3;
    BatchGuard guard;

    CHECK_FALSE(RS_Modification::offset(data, {&circle}, false, guard.ctx));
    CHECK(guard.ctx.entitiesToAdd.isEmpty());
    CHECK(guard.ctx.entitiesToDelete.isEmpty());

    data.number = 2;
    CHECK(RS_Modification::offset(data, {&circle}, false, guard.ctx));
    CHECK(guard.ctx.entitiesToAdd.size() == 2);
    CHECK(guard.ctx.entitiesToDelete.size() == 1);
}

TEST_CASE("RS_Modification::offset keeps the originals when asked to",
          "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    RS_OffsetData data = inwardOffset(10.0);
    data.keepOriginals = true;
    BatchGuard guard;

    CHECK(RS_Modification::offset(data, {&circle}, false, guard.ctx));
    CHECK(guard.ctx.entitiesToAdd.size() == 1);
    CHECK(guard.ctx.entitiesToDelete.isEmpty());
}

TEST_CASE("RS_Modification::offset honours the current layer and pen options",
          "[modification][offset]") {
    RS_Circle circle{nullptr, RS_CircleData{RS_Vector{0.0, 0.0}, 20.0}};
    RS_OffsetData data = inwardOffset(10.0);
    data.useCurrentLayer = false;
    data.useCurrentAttributes = false;
    BatchGuard guard;

    REQUIRE(RS_Modification::offset(data, {&circle}, false, guard.ctx));
    CHECK_FALSE(guard.ctx.setActiveLayer);
    CHECK_FALSE(guard.ctx.setActivePen);

    data.useCurrentLayer = true;
    BatchGuard second;
    REQUIRE(RS_Modification::offset(data, {&circle}, false, second.ctx));
    CHECK(second.ctx.setActiveLayer);
    CHECK_FALSE(second.ctx.setActivePen);
}
