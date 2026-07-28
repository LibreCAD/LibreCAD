/****************************************************************************
**
** This file is part of the LibreCAD project, unit tests for RS_Modification
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

// RS_Modification::trim() used to warn that a non-atomic limit entity was
// unsupported and then use it anyway, casting it to RS_AtomicEntity at three
// sites. trimStartpoint(), trimEndpoint() and getTrimPoint() are declared only
// on RS_AtomicEntity, so dispatching them through a container pointer calls
// whatever occupies the same vtable slot — silent corruption rather than a
// clean failure.
//
// A container limit entity is legitimate for a one-sided trim, so the fix gates
// the casts on isAtomic() rather than refusing the entity.

#include <catch2/catch_test_macros.hpp>

#include "rs_document.h"
#include "rs_entitycontainer.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_vector.h"

TEST_CASE("RS_Modification::trim does not cast a container limit entity",
          "[rs_modification][trim][container]") {
    // A line crossing the container's segment, so an intersection exists and the
    // function proceeds past the early outs into the casting code.
    RS_Line toTrim{nullptr, RS_Vector{0.0, 5.0}, RS_Vector{10.0, 5.0}};

    RS_EntityContainer limit{nullptr};
    limit.addEntity(new RS_Line{&limit, RS_Vector{5.0, 0.0}, RS_Vector{5.0, 10.0}});
    limit.calculateBorders();

    LC_DocumentModificationBatch ctx;

    // both=true asks trim() to trim the limit entity as well. Before the fix this
    // reached static_cast<RS_AtomicEntity*>(container) and called atomic-only
    // virtuals through it.
    LC_TrimResult result;
    REQUIRE_NOTHROW(result = RS_Modification::trim(RS_Vector{9.0, 5.0}, &toTrim,
                                                   RS_Vector{5.0, 9.0}, &limit,
                                                   true, ctx));

    // The trim of the atomic entity is still allowed; only the container side is
    // declined, so nothing is produced for it.
    CHECK(result.trimmed2 == nullptr);
}

TEST_CASE("RS_Modification::trim still trims against an atomic limit entity",
          "[rs_modification][trim][container]") {
    // Guards against the fix being over-broad: an ordinary line limit must still
    // trim both entities.
    RS_Line toTrim{nullptr, RS_Vector{0.0, 5.0}, RS_Vector{10.0, 5.0}};
    RS_Line limit{nullptr, RS_Vector{5.0, 0.0}, RS_Vector{5.0, 10.0}};

    LC_DocumentModificationBatch ctx;
    const LC_TrimResult result = RS_Modification::trim(RS_Vector{9.0, 5.0}, &toTrim,
                                                       RS_Vector{5.0, 9.0}, &limit,
                                                       true, ctx);
    CHECK(result.trimmed1 != nullptr);
    CHECK(result.trimmed2 != nullptr);
}
