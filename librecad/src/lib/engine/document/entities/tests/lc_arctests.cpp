/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "rs_arc.h"

TEST_CASE("RS_Arc::revertDirection keeps the shape and swaps the ends", "[rs_arc]") {
    // "Is Reversed" in the arc dialog and in the property sheet reverts the direction. setReversed() alone
    // turns the arc into the rest of the circle.
    using Catch::Matchers::WithinAbs;
    for (const bool reversed : {false, true}) {
        INFO("reversed " << reversed);
        RS_Arc arc(nullptr, RS_ArcData(RS_Vector(3., -4.), 100., 0.2, 1.7, reversed));
        const RS_Vector start = arc.getStartpoint();
        const RS_Vector end = arc.getEndpoint();
        const RS_Vector middle = arc.getMiddlePoint();
        const RS_Vector min = arc.getMin();
        const RS_Vector max = arc.getMax();
        const double length = arc.getLength();

        arc.revertDirection();

        CHECK(arc.isReversed() != reversed);
        CHECK(arc.getStartpoint().distanceTo(end) < 1e-9);
        CHECK(arc.getEndpoint().distanceTo(start) < 1e-9);
        CHECK(arc.getMiddlePoint().distanceTo(middle) < 1e-9);
        CHECK(arc.getMin().distanceTo(min) < 1e-9);
        CHECK(arc.getMax().distanceTo(max) < 1e-9);
        CHECK_THAT(arc.getLength(), WithinAbs(length, 1e-9));

        // the cached points agree with an arc made from the same data
        const RS_Arc fresh(nullptr, arc.getData());
        CHECK(fresh.getStartpoint().distanceTo(arc.getStartpoint()) < 1e-9);
        CHECK(fresh.getEndpoint().distanceTo(arc.getEndpoint()) < 1e-9);
        CHECK(fresh.getMiddlePoint().distanceTo(arc.getMiddlePoint()) < 1e-9);
    }
}
