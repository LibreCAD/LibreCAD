/****************************************************************************
**
** This file is part of the LibreCAD project, unit tests for RS_Creation
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

// Regression tests for issue #2708.
//
// The parallel tools hand RS_Creation whatever entity the last mouse move caught,
// which is a null pointer whenever the cursor is not over a supported entity. These
// entry points used to declare that precondition with Q_ASSERT, which is compiled
// out in every build the project ships, so a null argument reached a virtual rtti()
// call and crashed. They must instead return without producing anything.
//
// Note this file is deliberately free of any action or document dependency:
// RS_Creation is a namespace of free functions, so the contract is testable directly.
//
// The failure guarded against is a segmentation fault, not an exception, so the calls
// are made plainly rather than wrapped in a no-throw assertion, which would imply the
// wrong failure mode. Catch2 installs a fatal-signal handler and reports SIGSEGV as a
// failed assertion, so a regression fails the test rather than passing silently.
// Verified: with the guards removed these abort with
// "SIGSEGV - Segmentation violation signal" and exit 139.

#include <catch2/catch_test_macros.hpp>

#include <QList>

#include "rs_creation.h"
#include "rs_entity.h"
#include "rs_line.h"
#include "rs_vector.h"

namespace {
const RS_Vector g_coord{1.0, 2.0};
}

TEST_CASE("RS_Creation::createParallel tolerates a null entity", "[rs_creation][null]") {
    QList<RS_Entity*> created;
    RS_Creation::createParallel(g_coord, 0.5, 1, nullptr, false, created);
    CHECK(created.isEmpty());
}

TEST_CASE("RS_Creation::createParallelThrough tolerates a null entity", "[rs_creation][null]") {
    QList<RS_Entity*> created;
    RS_Creation::createParallelThrough(g_coord, 1, nullptr, false, false, created);
    CHECK(created.isEmpty());
}

// ---------------------------------------------------------------------------
// Positive cases. Without these the null-input tests above would still pass if
// a guard were mutated into an unconditional early return, which would disable
// the tools rather than fix them.
// ---------------------------------------------------------------------------

TEST_CASE("RS_Creation::createParallel still produces a parallel for a real line",
          "[rs_creation][positive]") {
    RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
    QList<RS_Entity*> created;
    RS_Creation::createParallel(RS_Vector{5.0, 3.0}, 2.0, 1, &line, false, created);
    REQUIRE(created.size() == 1);
    CHECK(created.front()->rtti() == RS2::EntityLine);
    qDeleteAll(created);
}

TEST_CASE("RS_Creation::createParallel honours the requested count",
          "[rs_creation][positive]") {
    RS_Line line{nullptr, RS_Vector{0.0, 0.0}, RS_Vector{10.0, 0.0}};
    QList<RS_Entity*> created;
    RS_Creation::createParallel(RS_Vector{5.0, 3.0}, 1.0, 4, &line, false, created);
    CHECK(created.size() == 4);
    qDeleteAll(created);
}
