/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
**********************************************************************/

/**
 * Phase 4a self-consistency tests for per-entity DRW_*::encodeDwg.
 *
 * Pattern mirrors dwg_header_encode_round_trip_tests.cpp: encode an
 * entity via the protected encodeDwg, wrap the bytes as a dwgBuffer,
 * and parse them back into a fresh entity instance.  Compare the
 * relevant fields field-by-field.  Any bit-stream desync in the
 * common preamble or per-entity body shifts every following bit and
 * trips a downstream assertion.
 */

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

/// Friend accessor for the protected encodeDwg / parseDwg + private
/// post-parse fields.  Declared as a friend by drw_entities.h via
/// SETENTFRIENDS.
class DrwEntityEncodeTestAccess {
public:
    static bool encode(DRW_Entity& e, DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf, /*bs=*/0);
    }
    static bool parse(DRW_Entity& e, DRW::Version v, dwgBuffer* buf) {
        // Per-entity parseDwg overrides the abstract; both Point/Line/etc.
        // call DRW_Entity::parseDwg(buf, NULL, bs) themselves.  Use a
        // named duint32 to disambiguate from the 4-arg parseDwg overload
        // (which has dwgBuffer* as its 3rd parameter).
        duint32 bs = 0;
        return e.parseDwg(v, buf, bs);
    }
    static dwgHandle& layerH(DRW_Entity& e) { return e.layerH; }
};

namespace {

std::vector<duint8> snapshot(const dwgBufferW& w) { return w.data(); }

} // namespace

TEST_CASE("DRW_Point::encodeDwg round-trips coordinates and thickness",
          "[dwg-write][entity-encode]") {
    DRW_Point src;
    src.handle = 0x33;          // arbitrary user-handle past the reserved set
    src.color = 7;              // ACI white
    src.ltypeScale = 1.0;
    src.basePoint = DRW_Coord{12.5, -34.75, 100.0};
    src.thickness = 0.0;        // BT shortcut path
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};  // BE shortcut path
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;  // layer "0"

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));

    auto bytes = snapshot(w);
    REQUIRE(bytes.size() > 0);

    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Point dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.handle        == 0x33u);
    REQUIRE(dst.color         == 7);
    REQUIRE(dst.ltypeScale    == 1.0);
    REQUIRE(dst.basePoint.x   == 12.5);
    REQUIRE(dst.basePoint.y   == -34.75);
    REQUIRE(dst.basePoint.z   == 100.0);
    REQUIRE(dst.thickness     == 0.0);
    REQUIRE(dst.extPoint.x    == 0.0);
    REQUIRE(dst.extPoint.y    == 0.0);
    REQUIRE(dst.extPoint.z    == 1.0);
    REQUIRE(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x12u);
}

TEST_CASE("DRW_Point::encodeDwg round-trips non-default thickness + extrusion",
          "[dwg-write][entity-encode]") {
    DRW_Point src;
    src.handle = 0x4A;
    src.color = 256;            // BYLAYER
    src.ltypeScale = 2.5;
    src.basePoint = DRW_Coord{0.5, 0.25, -1.0};
    src.thickness = 0.125;
    src.extPoint = DRW_Coord{0.1, 0.2, 0.97};  // forces full BE emit
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Point dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.handle      == 0x4Au);
    REQUIRE(dst.color       == 256);
    REQUIRE(dst.ltypeScale  == 2.5);
    REQUIRE(dst.basePoint.x == 0.5);
    REQUIRE(dst.basePoint.y == 0.25);
    REQUIRE(dst.basePoint.z == -1.0);
    REQUIRE(dst.thickness   == 0.125);
    REQUIRE(dst.extPoint.x  == 0.1);
    REQUIRE(dst.extPoint.y  == 0.2);
    REQUIRE(dst.extPoint.z  == 0.97);
}

TEST_CASE("DRW_Line::encodeDwg round-trips both Z paths",
          "[dwg-write][entity-encode]") {
    // Z==0 shortcut: zIsZero=1 path.
    {
        DRW_Line src;
        src.handle = 0x55;
        src.color = 1;
        src.ltypeScale = 1.0;
        src.basePoint = DRW_Coord{1.0, 2.0, 0.0};
        src.secPoint  = DRW_Coord{10.0, 20.0, 0.0};
        src.thickness = 0.0;
        src.extPoint  = DRW_Coord{0.0, 0.0, 1.0};
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Line dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

        REQUIRE(dst.basePoint.x == 1.0);
        REQUIRE(dst.basePoint.y == 2.0);
        REQUIRE(dst.basePoint.z == 0.0);
        REQUIRE(dst.secPoint.x  == 10.0);
        REQUIRE(dst.secPoint.y  == 20.0);
        REQUIRE(dst.secPoint.z  == 0.0);
    }
    // Z!=0 full path: zIsZero=0, RD z + DD secZ.
    {
        DRW_Line src;
        src.handle = 0x56;
        src.color = 5;
        src.basePoint = DRW_Coord{3.0, 4.0, 5.0};
        src.secPoint  = DRW_Coord{6.0, 7.0, 8.0};
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Line dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

        REQUIRE(dst.basePoint.x == 3.0);
        REQUIRE(dst.basePoint.y == 4.0);
        REQUIRE(dst.basePoint.z == 5.0);
        REQUIRE(dst.secPoint.x  == 6.0);
        REQUIRE(dst.secPoint.y  == 7.0);
        REQUIRE(dst.secPoint.z  == 8.0);
    }
}

TEST_CASE("DRW_Circle::encodeDwg round-trips center + radius",
          "[dwg-write][entity-encode]") {
    DRW_Circle src;
    src.handle = 0x60;
    src.color = 3;
    src.basePoint = DRW_Coord{50.0, 50.0, 0.0};
    src.radious = 12.5;
    src.thickness = 0.0;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Circle dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.handle      == 0x60u);
    REQUIRE(dst.basePoint.x == 50.0);
    REQUIRE(dst.basePoint.y == 50.0);
    REQUIRE(dst.radious     == 12.5);
}

TEST_CASE("DRW_Arc::encodeDwg round-trips center + radius + angles",
          "[dwg-write][entity-encode]") {
    DRW_Arc src;
    src.handle = 0x70;
    src.color = 6;
    src.basePoint = DRW_Coord{-10.0, 25.5, 0.0};
    src.radious = 8.0;
    src.staangle = 0.0;
    src.endangle = 1.5707963267948966;  // π/2
    src.thickness = 0.0;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Arc dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.handle      == 0x70u);
    REQUIRE(dst.basePoint.x == -10.0);
    REQUIRE(dst.basePoint.y == 25.5);
    REQUIRE(dst.radious     == 8.0);
    REQUIRE(dst.staangle    == 0.0);
    REQUIRE(dst.endangle    == 1.5707963267948966);
}

TEST_CASE("DRW_Text::encodeDwg round-trips string + position + style",
          "[dwg-write][entity-encode]") {
    DRW_Text src;
    src.handle = 0x90;
    src.color = 7;
    src.basePoint = DRW_Coord{100.0, 50.0, 0.0};
    src.secPoint = src.basePoint;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness = 0.0;
    src.height = 2.5;
    src.angle = 45.0;            // degrees
    src.widthscale = 1.0;
    src.oblique = 0.0;
    src.text = "HELLO";
    src.textgen = 0;
    src.alignH = DRW_Text::HLeft;
    src.alignV = DRW_Text::VBaseLine;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Text dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.handle      == 0x90u);
    REQUIRE(dst.basePoint.x == 100.0);
    REQUIRE(dst.basePoint.y == 50.0);
    REQUIRE(dst.height      == 2.5);
    REQUIRE(dst.text        == "HELLO");
    REQUIRE(dst.widthscale  == 1.0);
    REQUIRE(dst.oblique     == 0.0);
    // angle round-trips degrees → radians on disk → degrees on read.
    // Floating-point exact match holds for the canonical π/4 conversion
    // path, but allow a tiny epsilon for safety.
    REQUIRE(std::abs(dst.angle - 45.0) < 1e-9);
}

TEST_CASE("DRW_Ray::encodeDwg round-trips base + direction",
          "[dwg-write][entity-encode]") {
    DRW_Ray src;
    src.handle = 0xB0;
    src.color = 7;
    src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    src.secPoint  = DRW_Coord{4.0, 5.0, 6.0};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Ray dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.basePoint.x == 1.0);
    REQUIRE(dst.basePoint.y == 2.0);
    REQUIRE(dst.basePoint.z == 3.0);
    REQUIRE(dst.secPoint.x  == 4.0);
    REQUIRE(dst.secPoint.y  == 5.0);
    REQUIRE(dst.secPoint.z  == 6.0);
}

TEST_CASE("DRW_3Dface::encodeDwg round-trips four corners + invisibility flags",
          "[dwg-write][entity-encode]") {
    // Case 1: no invisible edges, z=0 (both bit shortcuts on).
    {
        DRW_3Dface src;
        src.handle = 0xD0;
        src.color = 7;
        src.basePoint = DRW_Coord{0.0, 0.0, 0.0};
        src.secPoint  = DRW_Coord{1.0, 0.0, 0.0};
        src.thirdPoint = DRW_Coord{1.0, 1.0, 0.0};
        src.fourPoint  = DRW_Coord{0.0, 1.0, 0.0};
        src.invisibleflag = 0;  // NoEdge
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
        dwgBuffer r(w.data().data(), w.data().size());
        DRW_3Dface dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

        REQUIRE(dst.basePoint.x  == 0.0);
        REQUIRE(dst.secPoint.x   == 1.0);
        REQUIRE(dst.thirdPoint.y == 1.0);
        REQUIRE(dst.fourPoint.y  == 1.0);
        REQUIRE(dst.invisibleflag == 0);
    }
    // Case 2: invisible flag set + non-zero z (both bit shortcuts off).
    {
        DRW_3Dface src;
        src.handle = 0xD1;
        src.basePoint = DRW_Coord{0.0, 0.0, 5.0};
        src.secPoint  = DRW_Coord{2.0, 0.0, 5.0};
        src.thirdPoint = DRW_Coord{2.0, 2.0, 5.0};
        src.fourPoint  = DRW_Coord{0.0, 2.0, 5.0};
        src.invisibleflag = 0xF;  // AllEdges
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
        dwgBuffer r(w.data().data(), w.data().size());
        DRW_3Dface dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

        REQUIRE(dst.basePoint.z   == 5.0);
        REQUIRE(dst.thirdPoint.z  == 5.0);
        REQUIRE(dst.invisibleflag == 0xF);
    }
}

TEST_CASE("DRW_Trace::encodeDwg round-trips four corners",
          "[dwg-write][entity-encode]") {
    DRW_Trace src;
    src.handle = 0xBA;
    src.color = 4;
    src.basePoint = DRW_Coord{1.0, 2.0, 0.5};  // base.z is the elevation
    src.secPoint  = DRW_Coord{11.0, 2.0, 0.5};
    src.thirdPoint = DRW_Coord{1.0, 12.0, 0.5};
    src.fourPoint = DRW_Coord{11.0, 12.0, 0.5};
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness = 0.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Trace dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.basePoint.x  == 1.0);
    REQUIRE(dst.basePoint.z  == 0.5);
    REQUIRE(dst.secPoint.x   == 11.0);
    REQUIRE(dst.thirdPoint.y == 12.0);
    REQUIRE(dst.fourPoint.x  == 11.0);
    REQUIRE(dst.fourPoint.y  == 12.0);
}

TEST_CASE("DRW_Solid::encodeDwg round-trips four corners",
          "[dwg-write][entity-encode]") {
    DRW_Solid src;
    src.handle = 0xC0;
    src.color = 3;
    src.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint  = DRW_Coord{10.0, 0.0, 0.0};
    src.thirdPoint = DRW_Coord{0.0, 10.0, 0.0};
    src.fourPoint = DRW_Coord{10.0, 10.0, 0.0};
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness = 0.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Solid dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.basePoint.x  == 0.0);
    REQUIRE(dst.secPoint.x   == 10.0);
    REQUIRE(dst.thirdPoint.y == 10.0);
    REQUIRE(dst.fourPoint.x  == 10.0);
    REQUIRE(dst.fourPoint.y  == 10.0);
}

TEST_CASE("DRW_LWPolyline::encodeDwg round-trips closed quad with no bulges",
          "[dwg-write][entity-encode]") {
    DRW_LWPolyline src;
    src.handle = 0xA0;
    src.color = 1;
    src.flags = 1;  // closed (DXF bit 0)
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};

    auto v0 = src.addVertex(); v0->x = 0.0;  v0->y = 0.0;
    auto v1 = src.addVertex(); v1->x = 10.0; v1->y = 0.0;
    auto v2 = src.addVertex(); v2->x = 10.0; v2->y = 10.0;
    auto v3 = src.addVertex(); v3->x = 0.0;  v3->y = 10.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_LWPolyline dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.handle == 0xA0u);
    REQUIRE(dst.flags  == 1);  // closed bit survives the DWG-flag round-trip
    REQUIRE(dst.vertlist.size() == 4);
    REQUIRE(dst.vertlist[0]->x == 0.0);
    REQUIRE(dst.vertlist[0]->y == 0.0);
    REQUIRE(dst.vertlist[1]->x == 10.0);
    REQUIRE(dst.vertlist[2]->x == 10.0);
    REQUIRE(dst.vertlist[2]->y == 10.0);
    REQUIRE(dst.vertlist[3]->x == 0.0);
    REQUIRE(dst.vertlist[3]->y == 10.0);
}

TEST_CASE("DRW_LWPolyline::encodeDwg round-trips bulges + widths",
          "[dwg-write][entity-encode]") {
    DRW_LWPolyline src;
    src.handle = 0xA1;
    src.color = 2;
    src.flags = 0;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};

    auto v0 = src.addVertex(); v0->x = 0.0; v0->y = 0.0;
        v0->bulge = 0.5;  v0->stawidth = 0.1; v0->endwidth = 0.2;
    auto v1 = src.addVertex(); v1->x = 5.0; v1->y = 0.0;
        v1->bulge = -0.5; v1->stawidth = 0.2; v1->endwidth = 0.1;
    auto v2 = src.addVertex(); v2->x = 5.0; v2->y = 5.0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_LWPolyline dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.vertlist.size() == 3);
    REQUIRE(dst.vertlist[0]->bulge    == 0.5);
    REQUIRE(dst.vertlist[1]->bulge    == -0.5);
    REQUIRE(dst.vertlist[0]->stawidth == 0.1);
    REQUIRE(dst.vertlist[0]->endwidth == 0.2);
    REQUIRE(dst.vertlist[1]->stawidth == 0.2);
}

TEST_CASE("DRW_Ellipse::encodeDwg round-trips center + axis + ratio + params",
          "[dwg-write][entity-encode]") {
    DRW_Ellipse src;
    src.handle = 0x80;
    src.color = 4;
    src.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint  = DRW_Coord{10.0, 0.0, 0.0};  // major axis
    src.extPoint  = DRW_Coord{0.0, 0.0, 1.0};
    src.ratio = 0.5;
    src.staparam = 0.0;
    src.endparam = 6.283185307179586;  // 2π — full ellipse
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1015, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Ellipse dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE(dst.handle      == 0x80u);
    REQUIRE(dst.basePoint.x == 0.0);
    REQUIRE(dst.secPoint.x  == 10.0);
    REQUIRE(dst.ratio       == 0.5);
    REQUIRE(dst.staparam    == 0.0);
    REQUIRE(dst.endparam    == 6.283185307179586);
}
