/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
 *
 * Each test runs for both AC1015 (R2000) and AC1018 (R2004) to cover the
 * version-specific preamble path (xDictFlag bit vs haveNextLinks bit) and
 * the unconditional null XDicObjH handle emitted by encodeDwgEntHandle.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

using Catch::Approx;

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
        // named std::uint32_t to disambiguate from the 4-arg parseDwg overload
        // (which has dwgBuffer* as its 3rd parameter).
        std::uint32_t bs = 0;
        return e.parseDwg(v, buf, bs);
    }
    static dwgHandle& layerH(DRW_Entity& e) { return e.layerH; }
    static std::uint16_t oType(const DRW_Entity& e) { return e.oType; }
};

namespace {

std::vector<std::uint8_t> snapshot(const dwgBufferW& w) { return w.data(); }

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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));

        auto bytes = snapshot(w);
        REQUIRE(bytes.size() > 0);

        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

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

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Line dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.x == 1.0);
            REQUIRE(dst.basePoint.y == 2.0);
            REQUIRE(dst.basePoint.z == 0.0);
            REQUIRE(dst.secPoint.x  == 10.0);
            REQUIRE(dst.secPoint.y  == 20.0);
            REQUIRE(dst.secPoint.z  == 0.0);
        }
    }
    // Z!=0 full path: zIsZero=0, RD z + DD secZ.
    {
        DRW_Line src;
        src.handle = 0x56;
        src.color = 5;
        src.basePoint = DRW_Coord{3.0, 4.0, 5.0};
        src.secPoint  = DRW_Coord{6.0, 7.0, 8.0};
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Line dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.x == 3.0);
            REQUIRE(dst.basePoint.y == 4.0);
            REQUIRE(dst.basePoint.z == 5.0);
            REQUIRE(dst.secPoint.x  == 6.0);
            REQUIRE(dst.secPoint.y  == 7.0);
            REQUIRE(dst.secPoint.z  == 8.0);
        }
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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Circle dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x60u);
        REQUIRE(dst.basePoint.x == 50.0);
        REQUIRE(dst.basePoint.y == 50.0);
        REQUIRE(dst.radious     == 12.5);
    }
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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Arc dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x70u);
        REQUIRE(dst.basePoint.x == -10.0);
        REQUIRE(dst.basePoint.y == 25.5);
        REQUIRE(dst.radious     == 8.0);
        REQUIRE(dst.staangle    == 0.0);
        REQUIRE(dst.endangle    == 1.5707963267948966);
    }
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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Text dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

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
}

TEST_CASE("DRW_Spline::encodeDwg round-trips a control-point cubic",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF0;
    src.color = 5;
    src.flags = 8;           // planar
    src.degree = 3;
    src.tolknot    = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist  = {0, 0, 0, 0, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{3.0, 0.0, 0.0}));
    src.nknots = 8;
    src.ncontrol = 4;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Spline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.degree            == 3);
        REQUIRE(dst.nknots            == 8);
        REQUIRE(dst.ncontrol          == 4);
        REQUIRE(dst.knotslist.size()  == 8);
        REQUIRE(dst.controllist.size() == 4);
        REQUIRE(dst.knotslist[0]      == 0.0);
        REQUIRE(dst.knotslist[7]      == 1.0);
        REQUIRE(dst.controllist[0]->x == 0.0);
        REQUIRE(dst.controllist[3]->x == 3.0);
    }
}

TEST_CASE("DRW_Spline::encodeDwg preserves rational quadratic weights",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF2;
    src.color = 2;
    src.flags = 8; // planar; encoder derives rational from non-default weights
    src.degree = 2;
    src.tolknot = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist = {0, 0, 0, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 1.0, 0.0}));
    src.weightlist = {1.0, std::sqrt(0.5), 1.0};
    src.nknots = 6;
    src.ncontrol = 3;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Spline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE((dst.flags & 0x04) == 0x04);
        REQUIRE(dst.weightlist.size() == 3);
        CHECK(dst.weightlist[1] == Approx(std::sqrt(0.5)));
    }
}

TEST_CASE("DRW_Spline::encodeDwg round-trips a fit-point spline",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF1;
    src.color = 6;
    src.flags = 8;
    src.degree = 3;
    src.tolfit = 1e-9;
    src.tgStart = DRW_Coord{1.0, 0.0, 0.0};
    src.tgEnd   = DRW_Coord{0.0, 1.0, 0.0};
    src.fitlist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.fitlist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{5.0, 5.0, 0.0}));
    src.fitlist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{10.0, 0.0, 0.0}));
    src.nfit = 3;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Spline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.degree         == 3);
        REQUIRE(dst.nfit           == 3);
        REQUIRE(dst.fitlist.size() == 3);
        REQUIRE(dst.fitlist[1]->x  == 5.0);
        REQUIRE(dst.fitlist[1]->y  == 5.0);
        REQUIRE(dst.tgStart.x      == 1.0);
        REQUIRE(dst.tgEnd.y        == 1.0);
    }
}

TEST_CASE("DRW_Spline::parseDwg rejects impossible control-point layout",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF3;
    src.color = 3;
    src.flags = 8;
    src.degree = 3;
    src.tolknot = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist = {0, 0, 0, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 0.0, 0.0}));
    src.nknots = 7;
    src.ncontrol = 3; // degree 3 requires at least 4 controls
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1018, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Spline dst;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1018, &r));
}

TEST_CASE("DRW_Spline::encodeDwg round-trips a high-degree control spline",
          "[dwg-write][entity-encode]") {
    DRW_Spline src;
    src.handle = 0xF4;
    src.color = 4;
    src.flags = 8;
    src.degree = 4;
    src.tolknot = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 1.5, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{3.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{4.0, 0.0, 0.0}));
    src.nknots = 10;
    src.ncontrol = 5;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwEntityEncodeTestAccess::encode(src, DRW::AC1018, &w));
    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Spline dst;
    REQUIRE(DrwEntityEncodeTestAccess::parse(dst, DRW::AC1018, &r));

    REQUIRE(dst.degree == 4);
    REQUIRE(dst.ncontrol == 5);
    REQUIRE(dst.nknots == 10);
    REQUIRE(dst.controllist.size() == 5);
}

TEST_CASE("DRW_MText::encodeDwg round-trips multi-line text",
          "[dwg-write][entity-encode]") {
    DRW_MText src;
    src.handle = 0xE0;
    src.color = 7;
    src.basePoint = DRW_Coord{10.0, 20.0, 0.0};
    src.extPoint  = DRW_Coord{0.0, 0.0, 1.0};
    src.secPoint  = DRW_Coord{1.0, 0.0, 0.0};   // X-axis = horizontal
    src.widthscale = 100.0;
    src.height = 3.5;
    src.textgen = static_cast<int>(DRW_MText::TopLeft);
    src.alignH = static_cast<DRW_Text::HAlign>(1);  // LtoR
    src.text = "Line one\\PLine two\\PLine three";
    src.interlin = 1.5;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_MText dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0xE0u);
        REQUIRE(dst.basePoint.x == 10.0);
        REQUIRE(dst.basePoint.y == 20.0);
        REQUIRE(dst.widthscale  == 100.0);
        REQUIRE(dst.height      == 3.5);
        REQUIRE(dst.textgen     == 1);  // TopLeft
        REQUIRE(dst.text        == "Line one\\PLine two\\PLine three");
        REQUIRE(dst.interlin    == 1.5);
    }
}

TEST_CASE("DRW_Ray::encodeDwg round-trips base + direction",
          "[dwg-write][entity-encode]") {
    DRW_Ray src;
    src.handle = 0xB0;
    src.color = 7;
    src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    src.secPoint  = DRW_Coord{4.0, 5.0, 6.0};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Ray dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.basePoint.x == 1.0);
        REQUIRE(dst.basePoint.y == 2.0);
        REQUIRE(dst.basePoint.z == 3.0);
        REQUIRE(dst.secPoint.x  == 4.0);
        REQUIRE(dst.secPoint.y  == 5.0);
        REQUIRE(dst.secPoint.z  == 6.0);
    }
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

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            dwgBuffer r(w.data().data(), w.data().size());
            DRW_3Dface dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.x  == 0.0);
            REQUIRE(dst.secPoint.x   == 1.0);
            REQUIRE(dst.thirdPoint.y == 1.0);
            REQUIRE(dst.fourPoint.y  == 1.0);
            REQUIRE(dst.invisibleflag == 0);
        }
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

        for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            dwgBuffer r(w.data().data(), w.data().size());
            DRW_3Dface dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

            REQUIRE(dst.basePoint.z   == 5.0);
            REQUIRE(dst.thirdPoint.z  == 5.0);
            REQUIRE(dst.invisibleflag == 0xF);
        }
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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Trace dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.basePoint.x  == 1.0);
        REQUIRE(dst.basePoint.z  == 0.5);
        REQUIRE(dst.secPoint.x   == 11.0);
        REQUIRE(dst.thirdPoint.y == 12.0);
        REQUIRE(dst.fourPoint.x  == 11.0);
        REQUIRE(dst.fourPoint.y  == 12.0);
    }
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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Solid dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.basePoint.x  == 0.0);
        REQUIRE(dst.secPoint.x   == 10.0);
        REQUIRE(dst.thirdPoint.y == 10.0);
        REQUIRE(dst.fourPoint.x  == 10.0);
        REQUIRE(dst.fourPoint.y  == 10.0);
    }
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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_LWPolyline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_LWPolyline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.vertlist.size() == 3);
        REQUIRE(dst.vertlist[0]->bulge    == 0.5);
        REQUIRE(dst.vertlist[1]->bulge    == -0.5);
        REQUIRE(dst.vertlist[0]->stawidth == 0.1);
        REQUIRE(dst.vertlist[0]->endwidth == 0.2);
        REQUIRE(dst.vertlist[1]->stawidth == 0.2);
    }
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

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Ellipse dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle      == 0x80u);
        REQUIRE(dst.basePoint.x == 0.0);
        REQUIRE(dst.secPoint.x  == 10.0);
        REQUIRE(dst.ratio       == 0.5);
        REQUIRE(dst.staparam    == 0.0);
        REQUIRE(dst.endparam    == 6.283185307179586);
    }
}

TEST_CASE("DRW_Attrib::encodeDwg round-trips basic single-line attribute",
          "[dwg-write][entity-encode]") {
    DRW_Attrib src;
    src.handle     = 0xA1;
    src.color      = 256;         // BYLAYER
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{3.0, 7.5, 0.0};
    src.secPoint   = DRW_Coord{3.0, 7.5, 0.0};  // same as base → DD shortcut
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 2.5;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;        // degrees — stored in struct after ARAD multiply
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "HELLO";
    src.tag        = "TAGNAME";
    src.attribFlags = 0;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attrib dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle        == 0xA1u);
        REQUIRE(dst.basePoint.x   == 3.0);
        REQUIRE(dst.basePoint.y   == 7.5);
        REQUIRE(dst.basePoint.z   == 0.0);
        REQUIRE(dst.height        == 2.5);
        REQUIRE(dst.widthscale    == 1.0);
        REQUIRE(dst.text          == "HELLO");
        REQUIRE(dst.tag           == "TAGNAME");
        REQUIRE(dst.attribFlags   == 0);
        REQUIRE(dst.styleH.ref    == 0x13u);  // STANDARD textstyle (default)
    }
}

TEST_CASE("DRW_Attdef::encodeDwg round-trips tag + prompt",
          "[dwg-write][entity-encode]") {
    DRW_Attdef src;
    src.handle     = 0xA2;
    src.color      = 1;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint   = DRW_Coord{0.0, 0.0, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 1.0;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "DEFAULT";
    src.tag        = "PARTNO";
    src.prompt     = "Enter part number:";
    src.attribFlags = 4;  // verify flag set
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attdef dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle       == 0xA2u);
        REQUIRE(dst.text         == "DEFAULT");
        REQUIRE(dst.tag          == "PARTNO");
        REQUIRE(dst.prompt       == "Enter part number:");
        REQUIRE(dst.attribFlags  == 4);
        REQUIRE(dst.height       == 1.0);
    }
}

// 2a.2 (gap entity-reactors-xdict-dropped-roundtrip): reactor handles and the
// xdict handle are now persisted on read and re-emitted on write (numReactors
// BL + per-handle stream + real-or-null xdic). Empty case stays byte-stable
// (covered by the existing [entity-encode] round-trips); this proves a
// populated entity round-trips reactors + xdict without a handle-stream desync
// (the layer handle still resolves, proving alignment).
TEST_CASE("DRW entity reactors + xdict round-trip through encode (2a.2)",
          "[dwg-write][entity-encode][phase2a]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        DRW_Point src;
        src.handle = 0x71;
        src.color = 7;
        src.ltypeScale = 1.0;
        src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
        src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        src.reactorHandles = {0xA0, 0xA1};
        src.xDictHandle = 0xB0;
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.reactorHandles.size() == 2u);
        REQUIRE(dst.reactorHandles[0] == 0xA0u);
        REQUIRE(dst.reactorHandles[1] == 0xA1u);
        REQUIRE(dst.xDictHandle == 0xB0u);
        // Alignment proof: the layer handle still lands correctly after the
        // reactor + xdic handles.
        REQUIRE(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x12u);
        REQUIRE(dst.basePoint.x == 1.0);
    }
}

// Mixed: reactors empty but xdict present → reader still reads the xdic handle
// and the layer follows correctly.
TEST_CASE("DRW entity xdict-only round-trip keeps alignment (2a.2)",
          "[dwg-write][entity-encode][phase2a]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        DRW_Point src;
        src.handle = 0x72;
        src.color = 7;
        src.ltypeScale = 1.0;
        src.basePoint = DRW_Coord{4.0, 5.0, 6.0};
        src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        src.xDictHandle = 0xC0;   // reactors empty
        DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.reactorHandles.empty());
        REQUIRE(dst.xDictHandle == 0xC0u);
        REQUIRE(DrwEntityEncodeTestAccess::layerH(dst).ref == 0x12u);
    }
}

// 2a.1 (gap entity-visible-code60-dropped-dwg-read): the encoder now emits
// the invisibleFlag BS (DXF 60) from `visible` instead of a hardcoded 0,
// paired with the read assign. A real encode→parse round-trip now preserves
// visible=false; a default visible=true entity stays byte-stable.
TEST_CASE("DRW entity visibility round-trips through encode (2a.1)",
          "[dwg-write][entity-encode][visibility]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        // visible=false survives the real encoder.
        {
            DRW_Point src;
            src.handle = 0x61;
            src.color = 7;
            src.ltypeScale = 1.0;
            src.basePoint = DRW_Coord{1.0, 2.0, 3.0};
            src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
            src.visible = false;
            DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Point dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE(dst.visible == false);
            REQUIRE(dst.basePoint.x == 1.0);
        }
        // visible=true (default) round-trips visible.
        {
            DRW_Point src;
            src.handle = 0x62;
            src.color = 7;
            src.ltypeScale = 1.0;
            src.basePoint = DRW_Coord{4.0, 5.0, 6.0};
            src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
            DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_Point dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE(dst.visible == true);
        }
    }
}

// T1 (gap attrib-attdef-lockpos-version-gate-r2007): the lockPosition (DXF
// 280) READ gate was lowered AC1024->AC1021 so R2007/8/9 imports preserve
// it; the ENCODE gate stays AC1024.  This test pins the LOWER boundary: at
// AC1015/AC1018 (below AC1021) the bit is neither written nor read, so a
// source with lockPosition=true round-trips to false with the body still
// aligned (tag/prompt intact).  The AC1021+ POSITIVE path cannot be exercised
// by this single-buffer harness — the encoder emits lockPosition only at
// AC1024, and AC1024+ parse needs the separate handle/string sections + a
// back-patched objSize the harness does not provide (existing entity tests
// likewise cap at AC1018).  AC1021+ positive coverage is deferred to the
// Phase-1 external write->reread validator / a real R2007 fixture.
TEST_CASE("DRW_Attrib lockPosition below AC1021 is not read (gate lower boundary)",
          "[dwg-write][entity-encode]") {
    DRW_Attrib src;
    src.handle     = 0xA1;
    src.color      = 256;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{3.0, 7.5, 0.0};
    src.secPoint   = DRW_Coord{3.0, 7.5, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 2.5;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "HELLO";
    src.tag        = "TAGNAME";
    src.attribFlags = 0;
    src.lockPosition = true;  // set on source; must NOT survive below AC1021
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attrib dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
        REQUIRE(dst.lockPosition == false);   // below gate: not written/read
        REQUIRE(dst.tag == "TAGNAME");         // body stayed aligned
    }
}

TEST_CASE("DRW_Attdef lockPosition below AC1021 is not read (gate lower boundary)",
          "[dwg-write][entity-encode]") {
    DRW_Attdef src;
    src.handle     = 0xA2;
    src.color      = 1;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint   = DRW_Coord{0.0, 0.0, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.thickness  = 0.0;
    src.height     = 1.0;
    src.widthscale = 1.0;
    src.oblique    = 0.0;
    src.angle      = 0.0;
    src.textgen    = 0;
    src.alignH     = DRW_Text::HLeft;
    src.alignV     = DRW_Text::VBaseLine;
    src.text       = "DEFAULT";
    src.tag        = "PARTNO";
    src.prompt     = "Enter part number:";
    src.attribFlags = 4;
    src.lockPosition = true;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Attdef dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
        REQUIRE(dst.lockPosition == false);
        REQUIRE(dst.prompt == "Enter part number:");
    }
}

// T2 (gap polyline-mesh-density-dropped-read): the POLYLINE_MESH (0x1E) DWG
// path read the smooth-surface M/N density (DXF 73/74) into discarded locals
// and wrote literal 0s.  Now round-tripped via smoothM/smoothN.
TEST_CASE("DRW_Polyline POLYLINE_MESH round-trips smoothM/smoothN density",
          "[dwg-write][entity-encode]") {
    DRW_Polyline src;
    src.handle     = 0xB1;
    src.color      = 7;
    src.ltypeScale = 1.0;
    src.flags      = 16;          // bit 4 → POLYLINE_MESH (oType 0x1E)
    src.curvetype  = 0;
    src.vertexcount = 3;          // M count
    src.facecount   = 4;          // N count
    src.smoothM     = 4;          // DXF 73
    src.smoothN     = 5;          // DXF 74
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Polyline dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.smoothM == 4);     // was 0 before the fix
        REQUIRE(dst.smoothN == 5);     // was 0 before the fix
        REQUIRE(dst.vertexcount == 3); // unchanged through round-trip
        REQUIRE(dst.facecount == 4);
        REQUIRE(dst.curvetype == 0);
        // reader sets bit 4 (3D mesh); writer strips it, reader re-adds it.
        REQUIRE((dst.flags & 16) == 16);
    }
}

TEST_CASE("DRW_Attrib and DRW_Attdef block unsupported multiline DWG writes",
          "[dwg-write][entity-encode]") {
    DRW_Attrib attrib;
    attrib.handle = 0xA3;
    attrib.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    attrib.secPoint = attrib.basePoint;
    attrib.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    attrib.height = 1.0;
    attrib.widthscale = 1.0;
    attrib.text = "MULTILINE";
    attrib.tag = "NOTE";
    attrib.m_attributeType = 2;
    DrwEntityEncodeTestAccess::layerH(attrib).ref = 0x12;
    dwgBufferW attribWriter;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::encode(attrib, DRW::AC1032, &attribWriter));

    DRW_Attdef attdef;
    attdef.handle = 0xA4;
    attdef.basePoint = DRW_Coord{0.0, 0.0, 0.0};
    attdef.secPoint = attdef.basePoint;
    attdef.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    attdef.height = 1.0;
    attdef.widthscale = 1.0;
    attdef.text = "DEFAULT";
    attdef.tag = "PARTNO";
    attdef.prompt = "Enter part number:";
    attdef.m_attributeType = 4;
    DrwEntityEncodeTestAccess::layerH(attdef).ref = 0x12;
    dwgBufferW attdefWriter;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::encode(attdef, DRW::AC1032, &attdefWriter));

    DRW_Attrib legacyMTextAttrib;
    legacyMTextAttrib.handle = 0xA5;
    legacyMTextAttrib.attVersion = 1;
    legacyMTextAttrib.m_attributeType = 1;
    DrwEntityEncodeTestAccess::layerH(legacyMTextAttrib).ref = 0x12;
    dwgBufferW legacyWriter;
    REQUIRE_FALSE(DrwEntityEncodeTestAccess::encode(legacyMTextAttrib, DRW::AC1024, &legacyWriter));
}

TEST_CASE("DRW_Hatch::encodeDwg round-trips solid fill with polyline boundary",
          "[dwg-write][entity-encode]") {
    // Build a rectangular solid hatch: one polyline loop, 4 vertices.
    DRW_Hatch src;
    src.handle     = 0xB0;
    src.color      = 256;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 0.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.name       = "SOLID";
    src.solid      = 1;
    src.associative = 0;
    src.hstyle     = 1;   // outermost
    src.hpattern   = 1;   // predefined
    src.loopsnum   = 1;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    // Polyline boundary loop (type bit 2 = polyline path)
    auto loop = std::make_shared<DRW_HatchLoop>(2);
    auto pline = std::make_shared<DRW_LWPolyline>();
    pline->flags = 1;  // closed
    pline->addVertex(DRW_Vertex2D{0.0, 0.0, 0.0});
    pline->addVertex(DRW_Vertex2D{10.0, 0.0, 0.0});
    pline->addVertex(DRW_Vertex2D{10.0, 5.0, 0.0});
    pline->addVertex(DRW_Vertex2D{0.0, 5.0, 0.0});
    loop->objlist.push_back(pline);
    src.looplist.push_back(loop);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Hatch dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle     == 0xB0u);
        REQUIRE(dst.name       == "SOLID");
        REQUIRE(dst.solid      == 1);
        REQUIRE(dst.associative == 0);
        REQUIRE(dst.hstyle     == 1);
        REQUIRE(dst.hpattern   == 1);
        REQUIRE(dst.looplist.size() == 1);
        // Polyline boundary: one object in objlist
        REQUIRE(dst.looplist[0]->objlist.size() == 1);
        const DRW_LWPolyline* rp = dynamic_cast<DRW_LWPolyline*>(
            dst.looplist[0]->objlist[0].get());
        REQUIRE(rp != nullptr);
        REQUIRE(rp->vertlist.size() == 4);
        REQUIRE(rp->vertlist[0]->x  == 0.0);
        REQUIRE(rp->vertlist[1]->x  == 10.0);
        REQUIRE(rp->vertlist[2]->y  == 5.0);
    }
}

TEST_CASE("DRW_Hatch::encodeDwg round-trips non-solid fill with line-segment boundary",
          "[dwg-write][entity-encode]") {
    DRW_Hatch src;
    src.handle     = 0xB1;
    src.color      = 1;
    src.ltypeScale = 1.0;
    src.basePoint  = DRW_Coord{0.0, 0.0, 2.0};
    src.extPoint   = DRW_Coord{0.0, 0.0, 1.0};
    src.name       = "ANSI31";
    src.solid      = 0;
    src.associative = 0;
    src.hstyle     = 0;
    src.hpattern   = 1;
    src.angle      = 45.0;
    src.scale      = 1.0;
    src.doubleflag = 0;
    src.loopsnum   = 1;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    // Non-polyline loop with two line segments forming a triangle
    auto loop = std::make_shared<DRW_HatchLoop>(0);
    auto l1 = std::make_shared<DRW_Line>();
    l1->basePoint = DRW_Coord{0.0, 0.0, 0.0};
    l1->secPoint  = DRW_Coord{4.0, 0.0, 0.0};
    auto l2 = std::make_shared<DRW_Line>();
    l2->basePoint = DRW_Coord{4.0, 0.0, 0.0};
    l2->secPoint  = DRW_Coord{0.0, 3.0, 0.0};
    loop->objlist.push_back(l1);
    loop->objlist.push_back(l2);
    src.looplist.push_back(loop);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Hatch dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.handle  == 0xB1u);
        REQUIRE(dst.name    == "ANSI31");
        REQUIRE(dst.solid   == 0);
        REQUIRE(dst.angle   == 45.0);
        REQUIRE(dst.scale   == 1.0);
        REQUIRE(dst.looplist.size() == 1);
        REQUIRE(dst.looplist[0]->objlist.size() == 2);
        // Two line edges
        const DRW_Line* rl1 = dynamic_cast<DRW_Line*>(
            dst.looplist[0]->objlist[0].get());
        REQUIRE(rl1 != nullptr);
        REQUIRE(rl1->basePoint.x == 0.0);
        REQUIRE(rl1->secPoint.x  == 4.0);
        const DRW_Line* rl2 = dynamic_cast<DRW_Line*>(
            dst.looplist[0]->objlist[1].get());
        REQUIRE(rl2 != nullptr);
        REQUIRE(rl2->basePoint.x == 4.0);
        REQUIRE(rl2->secPoint.y  == 3.0);
    }
}

TEST_CASE("DRW_DimAligned::encodeDwg round-trips definition points",
          "[dwg-write][entity-encode]") {
    DRW_DimAligned src;
    src.handle = 0xC1;
    src.type   = 1;      // bit0 set — the aligned subtype flag added by parseDwg
    src.setDef1Point({1.0, 2.0, 0.0});
    src.setDef2Point({5.0, 2.0, 0.0});
    src.setDimPoint ({3.0, 4.0, 0.0});
    src.setTextPoint({3.0, 3.5, 0.0});
    src.setHDir(0.0);    // hdir not set by default ctor — must be explicit

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimAligned dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 22u);
        REQUIRE(dst.type  == 1);
        REQUIRE(dst.getDef1Point().x == 1.0);
        REQUIRE(dst.getDef1Point().y == 2.0);
        REQUIRE(dst.getDef2Point().x == 5.0);
        REQUIRE(dst.getDimPoint().x  == 3.0);
        REQUIRE(dst.getDimPoint().y  == 4.0);
    }
}

TEST_CASE("DRW_DimLinear::encodeDwg round-trips rotation angle and oblique",
          "[dwg-write][entity-encode]") {
    DRW_DimLinear src;
    src.handle = 0xC2;
    src.type   = 0;      // no subtype flags for linear
    src.setDef1Point({0.0, 0.0, 0.0});
    src.setDef2Point({6.0, 0.0, 0.0});
    src.setDimPoint ({3.0, 2.0, 0.0});
    src.setAngle(45.0);  // 45 degrees rotation
    src.setOblique(10.0);
    src.setTextPoint({3.0, 2.5, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimLinear dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 21u);
        REQUIRE(dst.type  == 0);
        REQUIRE(dst.getDef1Point().x == 0.0);
        REQUIRE(dst.getDef2Point().x == 6.0);
        REQUIRE(dst.getDimPoint().x  == 3.0);
        REQUIRE(dst.getAngle()       == Approx(45.0).margin(1e-9));
        // oblique round-trips degrees → radians on disk → degrees on read
        REQUIRE(dst.getOblique()     == Approx(10.0).margin(1e-9));
    }
}

TEST_CASE("DRW_DimRadial::encodeDwg round-trips center, radius point, leader length",
          "[dwg-write][entity-encode]") {
    DRW_DimRadial src;
    src.handle = 0xC3;
    src.type   = 4;      // bit2 — radial subtype flag
    src.setCenterPoint({2.0, 3.0, 0.0});
    src.setDiameterPoint({5.0, 3.0, 0.0});
    src.setLeaderLength(1.5);
    src.setTextPoint({3.5, 4.0, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimRadial dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 25u);
        REQUIRE(dst.type  == 4);
        REQUIRE(dst.getCenterPoint().x   == 2.0);
        REQUIRE(dst.getCenterPoint().y   == 3.0);
        REQUIRE(dst.getDiameterPoint().x == 5.0);
        REQUIRE(dst.getLeaderLength()    == 1.5);
    }
}

TEST_CASE("DRW_DimAngular::encodeDwg round-trips arc point and definition lines",
          "[dwg-write][entity-encode]") {
    DRW_DimAngular src;
    src.handle = 0xC4;
    src.type   = 2;      // bit1 — angular subtype flag
    // arcPoint (code 16) is only 2D — use setDimPoint (public wrapper for setPt6)
    src.setDimPoint({7.0, 4.0, 0.0});
    src.setFirstLine1({1.0, 0.0, 0.0});
    src.setFirstLine2({4.0, 0.0, 0.0});
    src.setSecondLine1({4.0, 3.0, 0.0});
    src.setSecondLine2({0.0, 3.0, 0.0});
    src.setTextPoint({2.0, 1.5, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimAngular dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 24u);
        REQUIRE(dst.type  == 2);
        REQUIRE(dst.getDimPoint().x    == 7.0);
        REQUIRE(dst.getDimPoint().y    == 4.0);
        REQUIRE(dst.getFirstLine1().x  == 1.0);
        REQUIRE(dst.getFirstLine2().x  == 4.0);
        REQUIRE(dst.getSecondLine1().x == 4.0);
        REQUIRE(dst.getSecondLine1().y == 3.0);
    }
}

TEST_CASE("DRW_DimAngular3p::encodeDwg round-trips vertex and line definition points",
          "[dwg-write][entity-encode]") {
    DRW_DimAngular3p src;
    src.handle = 0xC5;
    src.type   = 5;      // bits 0+2 — angular3p subtype flags
    src.setDimPoint ({5.0, 5.0, 0.0});  // vertex / defPoint (code 10)
    src.setFirstLine ({1.0, 0.0, 0.0}); // pt3: first line endpoint
    src.setSecondLine({6.0, 0.0, 0.0}); // pt4: second line endpoint
    src.SetVertexPoint({3.0, 3.0, 0.0}); // pt5: circlePoint / vertex
    src.setTextPoint({4.0, 4.0, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimAngular3p dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 23u);
        REQUIRE(dst.type  == 5);
        REQUIRE(dst.getDimPoint().x  == 5.0);
        REQUIRE(dst.getDimPoint().y  == 5.0);
        REQUIRE(dst.getFirstLine().x  == 1.0);
        REQUIRE(dst.getSecondLine().x == 6.0);
        REQUIRE(dst.getVertexPoint().x == 3.0);
        REQUIRE(dst.getVertexPoint().y == 3.0);
    }
}

TEST_CASE("DRW_DimOrdinate::encodeDwg round-trips origin and leader endpoints",
          "[dwg-write][entity-encode]") {
    DRW_DimOrdinate src;
    src.handle = 0xC6;
    src.type   = 6;      // bits 1+2 — ordinate subtype flags
    src.setOriginPoint({2.0, 1.0, 0.0}); // defPoint (code 10)
    src.setFirstLine  ({2.0, 5.0, 0.0}); // pt3: feature location
    src.setSecondLine ({4.0, 5.0, 0.0}); // pt4: leader end
    src.setTextPoint  ({3.0, 5.5, 0.0});
    src.setHDir(0.0);

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_DimOrdinate dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 20u);
        REQUIRE(dst.type  == 6);
        REQUIRE(dst.getOriginPoint().x == 2.0);
        REQUIRE(dst.getOriginPoint().y == 1.0);
        REQUIRE(dst.getFirstLine().x   == 2.0);
        REQUIRE(dst.getFirstLine().y   == 5.0);
        REQUIRE(dst.getSecondLine().x  == 4.0);
    }
}

// 0B.1 (gap dim-ordinate-xy-flag-wrong-bit): the DWG ordinate X/Y type flag
// is DXF group-70 bit 6 (0x40) — the bit the filter (`type & 64`) and the
// DXF parseCode path use.  The DWG parse/encode previously used bit 7 (0x80),
// so the byte round-tripped but the filter check never fired.  Note: a
// DWG<->DWG byte round-trip alone is INSUFFICIENT to catch this bug (the old
// 0x80 code round-trips its own byte fine); the load-bearing assertion is
// that the surviving bit is 0x40 so the filter sees the X-type.
TEST_CASE("DRW_DimOrdinate X/Y flag round-trips on bit 0x40 (filter parity)",
          "[dwg-write][entity-encode]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        // X-type ordinate: group-70 bit 6 (0x40) set.
        {
            DRW_DimOrdinate src;
            src.handle = 0xC7;
            src.type   = 6 | 0x40;   // ordinate subtype bits + X-type flag
            src.setOriginPoint({2.0, 1.0, 0.0});
            src.setFirstLine  ({2.0, 5.0, 0.0});
            src.setSecondLine ({4.0, 5.0, 0.0});
            src.setTextPoint  ({3.0, 5.5, 0.0});
            src.setHDir(0.0);

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_DimOrdinate dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE((dst.type & 0x40) != 0);   // X-type survives → filter fires
            REQUIRE((dst.type & 0x80) == 0);   // base type bit not corrupted
        }
        // Y-type ordinate: group-70 bit 6 (0x40) clear.
        {
            DRW_DimOrdinate src;
            src.handle = 0xC8;
            src.type   = 6;          // no 0x40 → Y-type
            src.setOriginPoint({2.0, 1.0, 0.0});
            src.setFirstLine  ({2.0, 5.0, 0.0});
            src.setSecondLine ({4.0, 5.0, 0.0});
            src.setTextPoint  ({3.0, 5.5, 0.0});
            src.setHDir(0.0);

            dwgBufferW w;
            REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
            auto bytes = snapshot(w);
            dwgBuffer r(bytes.data(), bytes.size());
            DRW_DimOrdinate dst;
            REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));
            REQUIRE((dst.type & 0x40) == 0);   // Y-type stays Y
        }
    }
}

TEST_CASE("DRW_Leader::encodeDwg round-trips vertices, arrow, leadertype, extrusion",
          "[dwg-write][entity-encode][leader]") {
    DRW_Leader src;
    src.handle      = 0xD0;
    src.style       = "Standard";
    src.arrow       = 1;
    src.leadertype  = 1;            // spline path — verifies P1 pathType fix
    src.flag        = 3;
    src.hookline    = 1;
    src.hookflag    = 1;
    src.textheight  = 2.5;
    src.textwidth   = 10.0;
    src.extrusionPoint = DRW_Coord{0.0, 0.0, 1.0};   // default Z
    src.horizdir       = DRW_Coord{1.0, 0.0, 0.0};   // ezdxf default X
    src.offsetblock    = DRW_Coord{0.0, 0.0, 0.0};
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(5.0, 3.0, 0.0));
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(10.0, 3.0, 0.0));
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        REQUIRE(bytes.size() > 0);

        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Leader dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(DrwEntityEncodeTestAccess::oType(dst) == 45u);
        REQUIRE(dst.arrow      == 1);
        REQUIRE(dst.leadertype == 1);                       // P1
        REQUIRE(dst.textheight == Approx(2.5));             // pre-2010 branch
        REQUIRE(dst.textwidth  == Approx(10.0));
        REQUIRE(dst.vertexlist.size() == 3u);
        REQUIRE(dst.vertexlist[0]->x == Approx(0.0));
        REQUIRE(dst.vertexlist[1]->x == Approx(5.0));
        REQUIRE(dst.vertexlist[1]->y == Approx(3.0));
        REQUIRE(dst.vertexlist[2]->x == Approx(10.0));
        REQUIRE(dst.vertexlist[2]->y == Approx(3.0));
        REQUIRE(dst.extrusionPoint.z == Approx(1.0));       // P0 wire alignment
        REQUIRE(dst.horizdir.x       == Approx(1.0));
    }
}

TEST_CASE("DRW_Leader::encodeDwg straight-leader with arrow=0 round-trips",
          "[dwg-write][entity-encode][leader]") {
    DRW_Leader src;
    src.handle      = 0xD1;
    src.style       = "Standard";
    src.arrow       = 0;
    src.leadertype  = 0;            // straight segments
    src.textheight  = 1.0;
    src.textwidth   = 1.0;
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    src.vertexlist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Leader dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        REQUIRE(dst.arrow      == 0);
        REQUIRE(dst.leadertype == 0);
        REQUIRE(dst.vertexlist.size() == 2u);
    }
}

// Phase 6.1 — SHAPE encoder (fixed oType 33). Round-trips the 8 body fields +
// the trailing SHAPEFILE style hard handle.
TEST_CASE("DRW_Shape::encodeDwg round-trips body and style handle",
          "[dwg-write][entity-encode][shape]") {
    DRW_Shape src;
    src.handle = 0x40;
    src.color = 7;
    src.ltypeScale = 1.0;
    src.m_insertionPoint = DRW_Coord{3.0, 4.0, 5.0};
    src.m_scale = 2.5;
    src.m_rotation = 0.75;
    src.m_widthFactor = 1.25;
    src.m_oblique = 0.1;
    src.m_thickness = 0.5;
    src.m_shapeIndex = 17;
    src.m_extrusion = DRW_Coord{0.0, 0.0, 1.0};
    src.m_shapeFileHandle = 0x41;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));

        auto bytes = snapshot(w);
        REQUIRE(bytes.size() > 0);
        // Real DWG objects are followed by a 2-byte CRC; the SHAPE parser
        // gates the trailing style handle on numRemainingBytes() > 2 (i.e.
        // beyond that CRC). Append 2 padding bytes so the handle is read.
        bytes.push_back(0);
        bytes.push_back(0);

        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Shape dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        CHECK(DrwEntityEncodeTestAccess::oType(dst) == 33);
        CHECK(dst.handle == 0x40u);
        CHECK(dst.m_insertionPoint.x == Approx(3.0));
        CHECK(dst.m_insertionPoint.y == Approx(4.0));
        CHECK(dst.m_insertionPoint.z == Approx(5.0));
        CHECK(dst.m_scale == Approx(2.5));
        CHECK(dst.m_rotation == Approx(0.75));
        CHECK(dst.m_widthFactor == Approx(1.25));
        CHECK(dst.m_oblique == Approx(0.1));
        CHECK(dst.m_thickness == Approx(0.5));
        CHECK(dst.m_shapeIndex == 17);
        CHECK(dst.m_extrusion.z == Approx(1.0));
        CHECK(dst.m_shapeFileHandle == 0x41u);
    }
}

// IMAGE encoder round-trip (Phase 6.3): DRW_Image::encodeDwg must shadow
// DRW_Line::encodeDwg (oType 101, not 19/LINE), round-trip the body fields,
// and preserve BOTH trailing handles (imagedef + imagedefreactor) plus the
// display-props field that the reader previously discarded.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Image::encodeDwg round-trips body + both handles",
          "[dwg-write][entity-encode][image]") {
    DRW_Image src;
    src.handle      = 0x70;
    src.color       = 7;
    src.ltypeScale  = 1.0;
    src.basePoint   = DRW_Coord{1.0, 2.0, 3.0};
    src.secPoint    = DRW_Coord{0.5, 0.0, 0.0};   // uvec
    src.vVector     = DRW_Coord{0.0, 0.5, 0.0};
    src.sizeu       = 640.0;
    src.sizev       = 480.0;
    src.m_displayProps = 5;
    src.clip        = 1;
    src.brightness  = 60;
    src.contrast    = 40;
    src.fade        = 10;
    src.ref         = 0x100;                       // imagedef handle (340)
    src.m_imageDefReactorHandle = 0x101;           // imagedefreactor (360)
    // Non-empty polygon clip boundary (forces clip_boundary_type 2).
    src.clipPath = {DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{10.0, 0.0, 0.0},
                    DRW_Coord{10.0, 8.0, 0.0}, DRW_Coord{0.0, 8.0, 0.0}};
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        // Proves the override shadows DRW_Line::encodeDwg (oType 19).
        CHECK(DrwEntityEncodeTestAccess::oType(src) == 101);

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Image dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        CHECK(DrwEntityEncodeTestAccess::oType(dst) == 101);
        CHECK(dst.handle == 0x70u);
        CHECK(dst.basePoint.x == Approx(1.0));
        CHECK(dst.basePoint.y == Approx(2.0));
        CHECK(dst.basePoint.z == Approx(3.0));
        CHECK(dst.secPoint.x  == Approx(0.5));
        CHECK(dst.vVector.y   == Approx(0.5));
        CHECK(dst.sizeu       == Approx(640.0));
        CHECK(dst.sizev       == Approx(480.0));
        CHECK(dst.m_displayProps == 5);
        CHECK(dst.clip        == 1);
        CHECK(dst.brightness  == 60);
        CHECK(dst.contrast    == 40);
        CHECK(dst.fade        == 10);
        CHECK(dst.clipPath.size() == 4u);
        CHECK(dst.clipPath[2].x == Approx(10.0));
        CHECK(dst.clipPath[2].y == Approx(8.0));
        CHECK(dst.ref == 0x100u);                       // imagedef survives
        CHECK(dst.m_imageDefReactorHandle == 0x101u);   // reactor survives
    }
}

// IMAGE empty-clip variant: clip_boundary_type 0, both handles still emitted.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Image::encodeDwg round-trips empty clip boundary",
          "[dwg-write][entity-encode][image]") {
    DRW_Image src;
    src.handle      = 0x71;
    src.color       = 7;
    src.ltypeScale  = 1.0;
    src.basePoint   = DRW_Coord{0.0, 0.0, 0.0};
    src.secPoint    = DRW_Coord{1.0, 0.0, 0.0};
    src.vVector     = DRW_Coord{0.0, 1.0, 0.0};
    src.sizeu       = 100.0;
    src.sizev       = 100.0;
    src.clip        = 0;
    src.ref         = 0x200;
    src.m_imageDefReactorHandle = 0x201;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Image dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        CHECK(DrwEntityEncodeTestAccess::oType(dst) == 101);
        CHECK(dst.clipPath.empty());
        CHECK(dst.ref == 0x200u);
        CHECK(dst.m_imageDefReactorHandle == 0x201u);
    }
}

// HELIX encoder round-trip (Phase 8a-1): a HELIX encodes as a SPLINE body
// (oType 503, custom class AcDbHelix) followed by the AcDbHelix trailer. The
// spline body must stay intact (degree/control points) and every trailer field
// (radius/turns/turnHeight/axis vectors/handedness/constraint) must survive.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Helix::encodeDwg round-trips spline body + AcDbHelix trailer",
          "[dwg-write][entity-encode][helix]") {
    DRW_Helix src;
    src.handle = 0xF8;
    src.color = 5;
    src.flags = 8;           // planar
    src.degree = 3;
    src.tolknot    = 1e-9;
    src.tolcontrol = 1e-9;
    src.knotslist  = {0, 0, 0, 0, 1, 1, 1, 1};
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{0.0, 0.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{1.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{2.0, 1.0, 0.0}));
    src.controllist.push_back(std::make_shared<DRW_Coord>(DRW_Coord{3.0, 0.0, 0.0}));
    src.nknots = 8;
    src.ncontrol = 4;
    // AcDbHelix trailer fields.
    src.m_majorVersion = 29;
    src.m_maintVersion = 63;
    src.axisBasePt = DRW_Coord{1.0, 2.0, 3.0};
    src.startPt    = DRW_Coord{4.0, 5.0, 6.0};
    src.axisVector = DRW_Coord{0.0, 0.0, 1.0};
    src.radius      = 2.5;
    src.turns       = 7.0;
    src.turnHeight  = 1.5;
    src.handedness  = true;
    src.constraintType = 2;
    DrwEntityEncodeTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwEntityEncodeTestAccess::encode(src, ver, &w));
        CHECK(DrwEntityEncodeTestAccess::oType(src) == 503);

        auto bytes = snapshot(w);
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Helix dst;
        REQUIRE(DrwEntityEncodeTestAccess::parse(dst, ver, &r));

        // Spline body intact.
        CHECK(dst.degree == 3);
        CHECK(dst.controllist.size() == 2u + 2u);  // == 4
        CHECK(dst.controllist[0]->x == Approx(0.0));
        CHECK(dst.controllist[3]->x == Approx(3.0));
        // Trailer round-trips.
        CHECK(dst.m_majorVersion == 29);
        CHECK(dst.m_maintVersion == 63);
        CHECK(dst.axisBasePt.x == Approx(1.0));
        CHECK(dst.axisBasePt.z == Approx(3.0));
        CHECK(dst.startPt.y    == Approx(5.0));
        CHECK(dst.axisVector.z == Approx(1.0));
        CHECK(dst.radius      == Approx(2.5));
        CHECK(dst.turns       == Approx(7.0));
        CHECK(dst.turnHeight  == Approx(1.5));
        CHECK(dst.handedness  == true);
        CHECK(dst.constraintType == 2);
    }
}
