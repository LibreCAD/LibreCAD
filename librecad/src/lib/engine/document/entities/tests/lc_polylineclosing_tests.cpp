/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
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
**
****************************************************************************/

// A closed polyline draws its closing edge as a segment of its own, made every
// time the polyline is ended (each vertex the importers add ends it again) and
// added to the polyline. When the last vertex is the start, the edge has no
// length and is not added, and nothing freed it: every such polyline, as
// 3DFACE and POLYLINE import make them, leaked one line.

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <optional>

#include "lc_heapprobe.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {

RS_Polyline makeClosed(const std::initializer_list<RS_Vector> vertices) {
    RS_Polyline polyline(nullptr, RS_PolylineData(RS_Vector(false), RS_Vector(false), true));
    for (const RS_Vector& v : vertices) {
        polyline.addVertex(v, 0.0);
    }
    return polyline;
}

// a square, with its first vertex given again as the last
RS_Polyline squareWithStartRepeated() {
    return makeClosed({RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0), RS_Vector(10.0, 10.0), RS_Vector(0.0, 10.0),
                       RS_Vector(0.0, 0.0)});
}

} // namespace

TEST_CASE("A closed polyline ending on its start has no closing edge of no length", "[polyline][closing]") {
    const RS_Polyline polyline = squareWithStartRepeated();
    CHECK(polyline.isClosed());
    // the four sides; the fifth vertex is where the first began
    CHECK(polyline.count() == 4);
    for (const RS_Entity* side : polyline) {
        CHECK(side->getLength() > 1.0E-4);
    }
}

TEST_CASE("A closed polyline gets one closing edge, however often it is ended", "[polyline][closing]") {
    RS_Polyline triangle = makeClosed({RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0), RS_Vector(10.0, 10.0)});
    REQUIRE(triangle.count() == 3);
    const RS_Vector start = triangle.getStartpoint();

    for (int i = 0; i < 3; ++i) {
        triangle.endPolyline();
        REQUIRE(triangle.count() == 3);
    }
    const RS_Entity* closing = triangle.last();
    REQUIRE(closing != nullptr);
    CHECK(closing->getEndpoint().distanceTo(start) < 1.0E-9);

    // a copy closes itself too
    const RS_Polyline copy(triangle);
    CHECK(copy.count() == 3);
}

// Once the polyline has been built and dropped, everything it allocated is
// freed: the heap must not grow.
TEST_CASE("Building a closed polyline that ends on its start leaves nothing behind", "[polyline][closing][leak]") {
    squareWithStartRepeated(); // warm up

    const std::optional<std::size_t> before = lc::test::heapBytesInUse();
    for (int i = 0; i < 200; ++i) {
        squareWithStartRepeated();
    }
    const std::optional<std::size_t> after = lc::test::heapBytesInUse();

    if (before.has_value() && after.has_value()) {
        CHECK(*after <= *before);
    }
}
