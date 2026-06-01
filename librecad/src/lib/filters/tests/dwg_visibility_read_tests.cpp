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
 * Phase 0A — DWG entity visibility read (gap entity-visible-code60-dropped).
 *
 * DRW_Entity::parseDwg reads the common-data invisibleFlag (DXF group 60)
 * but historically dropped it into a discarded local.  It now assigns
 * `visible = (invisibleFlag == 0)`.
 *
 * The encoder still hardcodes invisibleFlag=0 (write-emit is Phase 2a), so
 * the positive (invisible) case cannot be produced by the real encoder.
 * To exercise it deterministically this test mirrors the AC1015 common-data
 * preamble + Point body + handle stream byte-for-byte and parameterizes the
 * invisibleFlag.  The mirror is SELF-VALIDATED: with invisibleFlag=0 it must
 * produce a stream that parses identically to the real DRW_Point::encodeDwg
 * (same coordinates, handles), proving the hand-built layout is correct
 * before we trust the invisibleFlag=1 variant.
 */

#include <catch2/catch_test_macros.hpp>

#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

/// Friend accessor (declared by drw_entities.h SETENTFRIENDS) for the
/// protected encodeDwg/parseDwg and the private layerH/oType members.
class DrwVisibilityTestAccess {
public:
    static bool encode(DRW_Entity& e, DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf, /*bs=*/0);
    }
    static bool parse(DRW_Entity& e, DRW::Version v, dwgBuffer* buf) {
        duint32 bs = 0;
        return e.parseDwg(v, buf, bs);
    }
    static dwgHandle& layerH(DRW_Entity& e) { return e.layerH; }
};

namespace {

/// Build a handle with the minimal byte size for `ref`.
dwgHandle makeHandle(duint8 code, duint32 ref) {
    dwgHandle h;
    h.code = code;
    h.ref = ref;
    h.size = 0;
    duint32 t = ref;
    while (t != 0) { t >>= 8; ++h.size; }
    return h;
}

/// Hand-built AC1015 POINT record mirroring DRW_Entity::encodeDwgCommon +
/// DRW_Point body + DRW_Entity::encodeDwgEntHandle, with the invisibleFlag
/// BS field parameterized.  Layout cites drw_entities.cpp:1604-1719.
std::vector<duint8> buildAc1015Point(duint32 handle, duint32 layerRef,
                                     duint16 color, double ltypeScale,
                                     const DRW_Coord& basePoint,
                                     duint16 invisibleFlag) {
    dwgBufferW w;

    // --- encodeDwgCommon (AC1015) ---
    w.putObjType(DRW::AC1015, 27);        // POINT class id (oType 27)
    w.putRawLong32(0);                    // objSize stub (back-patched=0)
    w.putHandle(makeHandle(0, handle));   // own handle, code 0
    w.putBitShort(0);                     // extDataSize = 0
    w.putBit(0);                          // graphFlag = 0
    w.put2Bits(2);                        // entmode = 2 (modelspace)
    w.putBitLong(0);                      // numReactors = 0
    w.putBit(1);                          // AC1015: haveNextLinks = 1
    w.putEnColor(DRW::AC1015, color);     // ENC color
    w.putBitDouble(ltypeScale);           // ltypeScale BD
    w.put2Bits(0);                        // ltFlags = BYLAYER
    w.put2Bits(0);                        // plotFlags = BYLAYER
    w.putBitShort(invisibleFlag);         // invisibleFlag BS  <-- under test
    w.putRawChar8(0);                     // lWeight RC = byLayer

    // --- Point body (drw_entities.cpp:1712-1717) ---
    w.putBitDouble(basePoint.x);
    w.putBitDouble(basePoint.y);
    w.putBitDouble(basePoint.z);
    w.putThickness(0.0, /*b_R2000_style=*/true);
    w.putExtrusion(DRW_Coord{0.0, 0.0, 1.0}, /*b_R2000_style=*/true);
    w.putBitDouble(0.0);                  // xAxisAngle BD

    // --- encodeDwgEntHandle (AC1015): handles inline in buf ---
    w.putHandle(makeHandle(3, 0));        // XDic null handle
    w.putHandle(makeHandle(layerRef == 0 ? 0 : 5, layerRef));  // layer hard ptr

    return w.data();
}

}  // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG entity read: hand-built preamble parses like real encoder",
          "[dwg-write][entity-encode][visibility]") {
    // Self-validation: the invisibleFlag=0 hand-built record must PARSE to
    // the same geometry/handles/visibility as the real DRW_Point::encodeDwg
    // output.  (Parse-equivalence is the load-bearing invariant; exact byte
    // equality would also depend on benign encoder-internal field encodings.)
    DRW_Point src;
    src.handle = 0x33;
    src.color = 7;
    src.ltypeScale = 1.0;
    src.basePoint = DRW_Coord{12.5, -34.75, 100.0};
    src.thickness = 0.0;
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    src.xAxisAngle = 0.0;
    DrwVisibilityTestAccess::layerH(src).ref = 0x12;

    dwgBufferW w;
    REQUIRE(DrwVisibilityTestAccess::encode(src, DRW::AC1015, &w));
    auto real = w.data();
    dwgBuffer rr(real.data(), real.size());
    DRW_Point realDst;
    REQUIRE(DrwVisibilityTestAccess::parse(realDst, DRW::AC1015, &rr));

    auto hand = buildAc1015Point(0x33, 0x12, 7, 1.0,
                                 DRW_Coord{12.5, -34.75, 100.0}, 0);
    dwgBuffer hr(hand.data(), hand.size());
    DRW_Point handDst;
    REQUIRE(DrwVisibilityTestAccess::parse(handDst, DRW::AC1015, &hr));

    REQUIRE(handDst.handle == realDst.handle);
    REQUIRE(handDst.color == realDst.color);
    REQUIRE(handDst.basePoint.x == realDst.basePoint.x);
    REQUIRE(handDst.basePoint.y == realDst.basePoint.y);
    REQUIRE(handDst.basePoint.z == realDst.basePoint.z);
    REQUIRE(handDst.visible == realDst.visible);
    REQUIRE(DrwVisibilityTestAccess::layerH(handDst).ref
            == DrwVisibilityTestAccess::layerH(realDst).ref);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG entity read: invisibleFlag==1 yields visible==false",
          "[dwg-write][entity-encode][visibility]") {
    auto bytes = buildAc1015Point(0x33, 0x12, 7, 1.0,
                                  DRW_Coord{1.0, 2.0, 3.0}, /*invisible=*/1);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Point dst;
    REQUIRE(DrwVisibilityTestAccess::parse(dst, DRW::AC1015, &r));

    REQUIRE_FALSE(dst.visible);          // gap fix: code 60 now honored
    // geometry still parsed correctly → stream stayed aligned
    REQUIRE(dst.basePoint.x == 1.0);
    REQUIRE(dst.basePoint.y == 2.0);
    REQUIRE(dst.basePoint.z == 3.0);
    REQUIRE(DrwVisibilityTestAccess::layerH(dst).ref == 0x12u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG entity read: invisibleFlag==0 yields visible==true",
          "[dwg-write][entity-encode][visibility]") {
    auto bytes = buildAc1015Point(0x44, 0x12, 7, 1.0,
                                  DRW_Coord{4.0, 5.0, 6.0}, /*invisible=*/0);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Point dst;
    REQUIRE(DrwVisibilityTestAccess::parse(dst, DRW::AC1015, &r));
    REQUIRE(dst.visible);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG entity read: default encode->parse round-trip stays visible",
          "[dwg-write][entity-encode][visibility]") {
    // Regression: the encoder emits invisibleFlag=0, so a real round-trip
    // must keep visible==true (encoder untouched by this change).
    DRW_Point src;
    src.handle = 0x55;
    src.color = 7;
    src.ltypeScale = 1.0;
    src.basePoint = DRW_Coord{7.0, 8.0, 9.0};
    src.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    DrwVisibilityTestAccess::layerH(src).ref = 0x12;

    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        dwgBufferW w;
        REQUIRE(DrwVisibilityTestAccess::encode(src, ver, &w));
        auto bytes = w.data();
        dwgBuffer r(bytes.data(), bytes.size());
        DRW_Point dst;
        REQUIRE(DrwVisibilityTestAccess::parse(dst, ver, &r));
        REQUIRE(dst.visible);
    }
}
