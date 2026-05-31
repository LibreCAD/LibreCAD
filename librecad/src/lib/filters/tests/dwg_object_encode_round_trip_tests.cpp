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
 * Per-DRW_*-OBJECT round-trip tests. Mirrors
 * dwg_entity_encode_round_trip_tests.cpp for the OBJECTS section
 * (DICTIONARY / XRECORD / LAYOUT / GROUP / SCALE / ...).
 *
 * Pattern: build a `dwgBufferW` with the same byte stream the real encoder
 * will eventually produce, parse it back via the friend-accessor, and
 * assert the resulting struct's fields match the source.  When the real
 * encoder lands (Phase B of the OBJECTS support plan), the in-test helper
 * here gets replaced by `DrwObjectEncodeTestAccess::encode`.
 *
 * Each test runs across the AC1015 (R2000) version because string fields
 * stay inline before R2007; AC1018/AC1024 require a separate string buffer
 * and are covered when the encoder lands.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

using Catch::Approx;

#include "drw_base.h"
#include "drw_objects.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

// Friend accessor for the protected parseDwg / encodeDwg + private
// post-parse fields. Declared as a friend by drw_objects.h via SETOBJFRIENDS.
class DrwObjectEncodeTestAccess {
public:
    static bool parse(DRW_TableEntry& e, DRW::Version v, dwgBuffer* buf) {
        duint32 bs = 0;
        return e.parseDwg(v, buf, bs);
    }
    static bool encodeDictionary(const DRW_Dictionary& e, DRW::Version v,
                                  dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeScale(const DRW_Scale& e, DRW::Version v,
                             dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeGroup(const DRW_Group& e, DRW::Version v,
                             dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeXRecord(const DRW_XRecord& e, DRW::Version v,
                               dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeLayout(const DRW_Layout& e, DRW::Version v,
                              dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeRasterVariables(const DRW_RasterVariables& e,
                                       DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeSortEntsTable(const DRW_SortEntsTable& e,
                                     DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeSpatialFilter(const DRW_SpatialFilter& e,
                                     DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeGeoData(const DRW_GeoData& e, DRW::Version v,
                               dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeDictionaryVar(const DRW_DictionaryVar& e, DRW::Version v,
                                     dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeDictionaryWDflt(const DRW_DictionaryWithDefault& e,
                                       DRW::Version v, dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeIDBuffer(const DRW_IDBuffer& e, DRW::Version v,
                                dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeLayerIndex(const DRW_LayerIndex& e, DRW::Version v,
                                  dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeSpatialIndex(const DRW_SpatialIndex& e, DRW::Version v,
                                    dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeField(const DRW_Field& e, DRW::Version v,
                             dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static bool encodeFieldList(const DRW_FieldList& e, DRW::Version v,
                                 dwgBufferW* buf) {
        return e.encodeDwg(v, buf);
    }
    static dint16 oType(const DRW_TableEntry& e) { return e.oType; }
    static void setNumReactors(DRW_TableEntry& e, dint32 n) { e.numReactors = n; }
    static void setXDictFlag(DRW_TableEntry& e, duint8 f) { e.xDictFlag = f; }
    // Phase 3A.0 — DIMSTYLE struct->vars sync (syncStructToVars is public, but
    // the wrapper keeps the test call site uniform with the access pattern).
    static void syncDimstyle(DRW_Dimstyle& d) { d.syncStructToVars(); }
};

namespace {

std::vector<duint8> snapshot(const dwgBufferW& w) { return w.data(); }

// Build a null handle (code 0, size 0).  Parser will read ref=0 back.
dwgHandle nullHandle() {
    dwgHandle h;
    h.code = 0;
    h.size = 0;
    h.ref  = 0;
    return h;
}

// Build a hard-pointer (code 4) handle.  Mirrors makeHardPtr in
// dwgwriter15.cpp (file-local there; inlined here).
dwgHandle hardPtr(duint32 ref) {
    dwgHandle h;
    h.code = (ref == 0) ? 0 : 4;
    h.ref  = ref;
    h.size = 0;
    if (ref != 0) {
        duint32 t = ref;
        while (t != 0) { t >>= 8; ++h.size; }
    }
    return h;
}

// Emit the common DRW_TableEntry preamble used by every OBJECTS-section
// record. Mirrors dwgwriter15.cpp::emitRecordPreamble for AC1015 (R2000),
// which is what DRW_TableEntry::parseDwg consumes in version < AC1024
// without the string-buffer split.
//
// `numReactors` and `xDictFlag` configure the corresponding preamble fields;
// the caller is responsible for emitting matching parent/reactor/xdic
// handles at the head of the post-body handle stream.
void emitObjectPreamble(dwgBufferW& body, DRW::Version version,
                        duint16 oType, duint32 handle,
                        dint32 numReactors = 0, duint8 xDictFlag = 0) {
    body.putObjType(version, oType);
    if (version > DRW::AC1014 && version < DRW::AC1024) {
        // RL 32-bit obj-size-in-bits placeholder; parser reads but does not
        // sanity-check the value.  Real encoder back-patches this; for a
        // self-consistency test the value is unused downstream.
        body.putRawLong32(0);
    }
    body.putHandle(hardPtr(handle));
    body.putBitShort(0);            // EED size = 0
    body.putBitLong(numReactors);
    if (version > DRW::AC1015) {
        body.putBit(xDictFlag);     // xDictFlag (R2004+)
    }
}

// Emit the common handle-stream prefix: parentHandle + numReactors reactor
// handles + xdic (when xDictFlag != 1).  Must precede any type-specific
// handles in the post-body handle stream so the parser's
// readCommonObjectHandles call lands its consumption in the right slot.
void emitCommonHandlePrefix(dwgBufferW& body, duint32 parentHandle,
                            const std::vector<duint32>& reactorHandles,
                            duint8 xDictFlag) {
    body.putHandle(hardPtr(parentHandle));
    for (duint32 ref : reactorHandles) {
        body.putHandle(hardPtr(ref));
    }
    if (xDictFlag != 1) {
        body.putHandle(nullHandle());      // xdic
    }
}

// Emit a LAYOUT body for the test.  Inverts DRW_Layout::parseDwg
// (drw_objects.cpp).  AC1015/AC1018 — pre-R2007 strings stay inline; LAYOUT's
// R2004+ shade-plot block and viewport handles are exercised through the
// version branch.
//
// `parentHandle` / `reactorHandles` / `xDictFlag` configure the common
// handle-stream prefix emitted immediately before the type-specific
// handles. They must match the values fed to emitObjectPreamble's
// numReactors / xDictFlag for the parser's readCommonObjectHandles call
// to consume them correctly.
void emitLayoutBody(dwgBufferW& body, DRW::Version version,
                    const DRW_Layout& src,
                    duint32 parentHandle = 0,
                    const std::vector<duint32>& reactorHandles = {},
                    duint8 xDictFlag = 0,
                    duint32 paperSpaceBlockRecordRef = 0,
                    duint32 lastActiveViewportRef = 0,
                    duint32 baseUcsRef = 0,
                    duint32 namedUcsRef = 0) {
    body.putVariableText(version, src.pageSetupName);
    body.putVariableText(version, src.printerConfig);
    body.putBitShort(src.plotLayoutFlags);
    body.putBitDouble(src.marginLeft);
    body.putBitDouble(src.marginBottom);
    body.putBitDouble(src.marginRight);
    body.putBitDouble(src.marginTop);
    body.putBitDouble(src.paperWidth);
    body.putBitDouble(src.paperHeight);
    body.putVariableText(version, src.paperSize);
    body.putBitDouble(src.plotOriginX);
    body.putBitDouble(src.plotOriginY);
    body.putBitShort(src.paperUnits);
    body.putBitShort(src.plotRotation);
    body.putBitShort(src.plotType);
    body.putBitDouble(src.windowMinX);
    body.putBitDouble(src.windowMinY);
    body.putBitDouble(src.windowMaxX);
    body.putBitDouble(src.windowMaxY);
    if (version < DRW::AC1018) {
        body.putVariableText(version, src.plotViewName);
    }
    body.putBitDouble(src.realWorldUnits);
    body.putBitDouble(src.drawingUnits);
    body.putVariableText(version, src.currentStyleSheet);
    body.putBitShort(src.scaleType);
    body.putBitDouble(src.scaleFactor);
    body.putBitDouble(src.paperImageOriginX);
    body.putBitDouble(src.paperImageOriginY);
    if (version >= DRW::AC1018) {
        body.putBitShort(src.shadePlotMode);
        body.putBitShort(src.shadePlotResLevel);
        body.putBitShort(src.shadePlotCustomDPI);
    }
    body.putVariableText(version, src.name);
    body.putBitLong(src.tabOrder);
    body.putBitShort(src.layoutFlags);
    body.put3BitDouble(src.ucsOrigin);
    body.putRawDouble(src.limMinX);
    body.putRawDouble(src.limMinY);
    body.putRawDouble(src.limMaxX);
    body.putRawDouble(src.limMaxY);
    body.put3BitDouble(src.insPoint);
    body.put3BitDouble(src.ucsXAxis);
    body.put3BitDouble(src.ucsYAxis);
    body.putBitDouble(src.elevation);
    body.putBitShort(src.orthoViewType);
    body.put3BitDouble(src.extMin);
    body.put3BitDouble(src.extMax);
    if (version >= DRW::AC1018) {
        body.putRawLong32(src.viewportCount);
    }
    // Common handle prefix (parentHandle + reactors + xdic) per ODA
    // §20.4.84 + base-object spec.  Must precede the type-specific
    // handles, matching the parser's readCommonObjectHandles call.
    emitCommonHandlePrefix(body, parentHandle, reactorHandles, xDictFlag);

    // Type-specific handle stream tail (inline for AC1015/AC1018).
    if (version >= DRW::AC1018) {
        body.putHandle(nullHandle());          // plotViewHandle
    }
    body.putHandle(hardPtr(paperSpaceBlockRecordRef));
    body.putHandle(hardPtr(lastActiveViewportRef));
    body.putHandle(hardPtr(baseUcsRef));
    body.putHandle(hardPtr(namedUcsRef));
    if (version >= DRW::AC1018) {
        for (dint32 i = 0; i < src.viewportCount; ++i) {
            body.putHandle(nullHandle());
        }
    }
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::parseDwg captures PlotSettings prefix + layout-specific fields",
          "[dwg-read][object-encode][layout]") {
    DRW_Layout src;
    src.pageSetupName    = "MyPageSetup";
    src.printerConfig    = "HP LaserJet";
    src.plotLayoutFlags  = 5;
    src.marginLeft       = 12.7;
    src.marginBottom     = 6.35;
    src.marginRight      = 12.7;
    src.marginTop        = 6.35;
    src.paperWidth       = 297.0;
    src.paperHeight      = 210.0;
    src.paperSize        = "A4";
    src.plotOriginX      = 5.0;
    src.plotOriginY      = 10.0;
    src.paperUnits       = 0;
    src.plotRotation     = 1;
    src.plotType         = 2;
    src.windowMinX       = -50.0;
    src.windowMinY       = -25.0;
    src.windowMaxX       = 250.0;
    src.windowMaxY       = 175.0;
    src.plotViewName     = "MyView";   // ignored on AC1018+
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 100.0;
    src.currentStyleSheet = "monochrome.ctb";
    src.scaleType        = 16;
    src.scaleFactor      = 0.01;
    src.paperImageOriginX = 1.5;
    src.paperImageOriginY = 2.5;
    src.name        = "Layout1";
    src.tabOrder    = 3;
    src.layoutFlags = 0;
    src.ucsOrigin   = DRW_Coord{1.0, 2.0, 3.0};
    src.limMinX     = -10.0;
    src.limMinY     = -20.0;
    src.limMaxX     = 100.0;
    src.limMaxY     = 200.0;
    src.insPoint    = DRW_Coord{0.5, 1.5, 2.5};
    src.ucsXAxis    = DRW_Coord{1.0, 0.0, 0.0};
    src.ucsYAxis    = DRW_Coord{0.0, 1.0, 0.0};
    src.elevation   = 7.5;
    src.orthoViewType = 4;
    src.extMin      = DRW_Coord{-15.0, -25.0, 0.0};
    src.extMax      = DRW_Coord{120.0, 220.0, 0.0};
    src.viewportCount = 0;

    // R2000: no R2004+ shade-plot block, no R2007+ visualStyle handle.
    DRW::Version ver = DRW::AC1015;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/82, /*handle=*/0x100);
    emitLayoutBody(w, ver, src);

    auto bytes = snapshot(w);
    REQUIRE(bytes.size() > 0);

    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.pageSetupName == "MyPageSetup");
    REQUIRE(dst.printerConfig == "HP LaserJet");
    REQUIRE(dst.plotLayoutFlags == 5);
    REQUIRE(dst.marginLeft   == Approx(12.7));
    REQUIRE(dst.marginBottom == Approx(6.35));
    REQUIRE(dst.marginRight  == Approx(12.7));
    REQUIRE(dst.marginTop    == Approx(6.35));
    REQUIRE(dst.paperWidth   == Approx(297.0));
    REQUIRE(dst.paperHeight  == Approx(210.0));
    REQUIRE(dst.paperSize    == "A4");
    REQUIRE(dst.plotOriginX  == Approx(5.0));
    REQUIRE(dst.plotOriginY  == Approx(10.0));
    REQUIRE(dst.paperUnits   == 0);
    REQUIRE(dst.plotRotation == 1);
    REQUIRE(dst.plotType     == 2);
    REQUIRE(dst.windowMinX   == Approx(-50.0));
    REQUIRE(dst.windowMaxY   == Approx(175.0));
    REQUIRE(dst.plotViewName == "MyView");
    REQUIRE(dst.realWorldUnits   == Approx(1.0));
    REQUIRE(dst.drawingUnits     == Approx(100.0));
    REQUIRE(dst.currentStyleSheet == "monochrome.ctb");
    REQUIRE(dst.scaleType   == 16);
    REQUIRE(dst.scaleFactor == Approx(0.01));
    REQUIRE(dst.paperImageOriginX == Approx(1.5));
    REQUIRE(dst.paperImageOriginY == Approx(2.5));

    REQUIRE(dst.name        == "Layout1");
    REQUIRE(dst.tabOrder    == 3);
    REQUIRE(dst.layoutFlags == 0);
    REQUIRE(dst.ucsOrigin.x == Approx(1.0));
    REQUIRE(dst.ucsOrigin.y == Approx(2.0));
    REQUIRE(dst.ucsOrigin.z == Approx(3.0));
    REQUIRE(dst.limMinX     == Approx(-10.0));
    REQUIRE(dst.limMaxX     == Approx(100.0));
    REQUIRE(dst.insPoint.x  == Approx(0.5));
    REQUIRE(dst.ucsXAxis.x  == Approx(1.0));
    REQUIRE(dst.ucsYAxis.y  == Approx(1.0));
    REQUIRE(dst.elevation   == Approx(7.5));
    REQUIRE(dst.orthoViewType == 4);
    REQUIRE(dst.extMin.x    == Approx(-15.0));
    REQUIRE(dst.extMax.y    == Approx(220.0));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::parseDwg AC1018 captures shade-plot block + viewport handles",
          "[dwg-read][object-encode][layout]") {
    DRW_Layout src;
    src.pageSetupName    = "PrintSetup";
    src.printerConfig    = "Generic PostScript";
    src.plotLayoutFlags  = 0;
    src.paperWidth       = 420.0;
    src.paperHeight      = 297.0;
    src.paperSize        = "A3";
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 1.0;
    src.currentStyleSheet = "";
    src.scaleType        = 0;
    src.scaleFactor      = 1.0;
    src.shadePlotMode      = 3;
    src.shadePlotResLevel  = 5;
    src.shadePlotCustomDPI = 300;
    src.name = "Sheet1";
    src.tabOrder = 1;
    src.layoutFlags = 1;
    src.viewportCount = 2;

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, 82, 0x101);
    emitLayoutBody(w, ver, src);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.pageSetupName == "PrintSetup");
    REQUIRE(dst.paperSize     == "A3");
    REQUIRE(dst.shadePlotMode      == 3);
    REQUIRE(dst.shadePlotResLevel  == 5);
    REQUIRE(dst.shadePlotCustomDPI == 300);
    REQUIRE(dst.name == "Sheet1");
    REQUIRE(dst.tabOrder == 1);
    REQUIRE(dst.viewportCount == 2);
    REQUIRE(dst.viewportHandles.size() == 2u);
}

// Regression test for the handle-stream alignment bug in DRW_Layout::parseDwg:
// for AC1015/AC1018 the parser previously skipped readCommonObjectHandles,
// so it consumed parentHandle + reactor + xdic bytes as the first
// type-specific handles. With non-zero parentHandle + reactor count the
// downstream type-specific assertions catch the misalignment.
//
// Pre-fix: dst.parentHandle stays 0 (never read) and dst.paperSpaceBlockRecordHandle.ref
// reads 0x42 (the misaligned parentHandle value) instead of the expected 0x70.
// Post-fix: both come out correctly.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::parseDwg consumes parentHandle + reactors before type-specific handles",
          "[dwg-read][object-encode][layout]") {
    DRW_Layout src;
    src.pageSetupName    = "Setup";
    src.printerConfig    = "Printer";
    src.paperWidth       = 297.0;
    src.paperHeight      = 210.0;
    src.paperSize        = "A4";
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 1.0;
    src.scaleType        = 0;
    src.scaleFactor      = 1.0;
    src.shadePlotMode      = 0;
    src.shadePlotResLevel  = 0;
    src.shadePlotCustomDPI = 0;
    src.name = "Layout1";
    src.tabOrder = 1;
    src.layoutFlags = 0;
    src.viewportCount = 0;

    const duint32 kParent = 0x42;
    const std::vector<duint32> kReactors = {0x50, 0x51};
    const duint32 kPaperSpaceRef = 0x70;
    const duint32 kLastActiveRef = 0x71;
    const duint32 kBaseUcsRef    = 0x72;
    const duint32 kNamedUcsRef   = 0x73;

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, 82, 0x110,
                       static_cast<dint32>(kReactors.size()),
                       /*xDictFlag=*/0);
    emitLayoutBody(w, ver, src, kParent, kReactors, /*xDictFlag=*/0,
                   kPaperSpaceRef, kLastActiveRef, kBaseUcsRef, kNamedUcsRef);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    // The fix guarantees parent is read into parentHandle, and type-specific
    // handles land in the correct slots.
    REQUIRE(static_cast<duint32>(dst.parentHandle)        == kParent);
    REQUIRE(dst.paperSpaceBlockRecordHandle.ref           == kPaperSpaceRef);
    REQUIRE(dst.lastActiveViewportHandle.ref              == kLastActiveRef);
    REQUIRE(dst.baseUcsHandle.ref                         == kBaseUcsRef);
    REQUIRE(dst.namedUcsHandle.ref                        == kNamedUcsRef);
}

// IDBUFFER parser test (ODA §20.4.79).  Common preamble + RC + BL + handle
// stream tail = parentHandle + xdic + N object-handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_IDBuffer::parseDwg captures object-id handle list",
          "[dwg-read][object-encode][idbuffer]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x200);  // custom-class
    w.putRawChar8(0);                                            // classVersion
    w.putBitLong(3);                                             // numIds

    emitCommonHandlePrefix(w, /*parentHandle=*/0x10, /*reactors=*/{}, /*xDictFlag=*/0);
    w.putHandle(hardPtr(0x100));
    w.putHandle(hardPtr(0x101));
    w.putHandle(hardPtr(0x102));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_IDBuffer dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x10u);
    REQUIRE(dst.classVersion == 0);
    REQUIRE(dst.objIds.size() == 3u);
    REQUIRE(dst.objIds[0] == 0x100u);
    REQUIRE(dst.objIds[1] == 0x101u);
    REQUIRE(dst.objIds[2] == 0x102u);
}

// LAYER_INDEX parser test (ODA §20.4.83).  timestamp1, timestamp2,
// numentries, then repeat (BL indexLong + TV name).  Handle stream:
// parentHandle + xdic + N entry handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_LayerIndex::parseDwg captures per-layer entry handles",
          "[dwg-read][object-encode][layerindex]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x300);
    w.putBitLong(0x12345);          // timestamp1
    w.putBitLong(0x6789a);          // timestamp2
    w.putBitLong(2);                // numEntries

    w.putBitLong(1);
    w.putVariableText(ver, "Layer0");
    w.putBitLong(2);
    w.putVariableText(ver, "Layer1");

    emitCommonHandlePrefix(w, 0x20, {}, 0);
    w.putHandle(hardPtr(0x200));    // entry 0 -> idbuffer handle
    w.putHandle(hardPtr(0x201));    // entry 1 -> idbuffer handle

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_LayerIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x20u);
    REQUIRE(dst.timestamp1 == 0x12345u);
    REQUIRE(dst.timestamp2 == 0x6789au);
    REQUIRE(dst.entries.size() == 2u);
    REQUIRE(dst.entries[0].indexLong == 1);
    REQUIRE(dst.entries[0].name == "Layer0");
    REQUIRE(dst.entries[0].entryHandle == 0x200u);
    REQUIRE(dst.entries[1].indexLong == 2);
    REQUIRE(dst.entries[1].name == "Layer1");
    REQUIRE(dst.entries[1].entryHandle == 0x201u);
}

// 8b-1 (gap wipeoutvariables-object-not-dispatched): WIPEOUTVARIABLES carries
// the drawing-wide display-frame flag (DXF 70). It is now parsed + dispatched
// (addWipeoutVariables) instead of only landing in the raw-replay skip set.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_WipeoutVariables::parseDwg captures display_frame flag",
          "[dwg-read][object-encode][wipeout]") {
    REQUIRE(DRW_WipeoutVariables{}.m_displayFrame == 0);  // default

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x600);
    w.putBitShort(1);   // display_frame = 1

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_WipeoutVariables dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));
    REQUIRE(dst.m_displayFrame == 1);
}

// P4-13 (gap object-imagedef-uv-size-dropped): DRW_ImageDef::parseDwg read
// the image pixel size (DXF 10/20) via get2RawDouble and discarded it; it now
// assigns u/v (distinct from the up/vp pixel-scale fields).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_ImageDef::parseDwg captures u/v pixel size",
          "[dwg-read][object-encode][imagedef]") {
    DRW::Version ver = DRW::AC1015;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x500);
    w.putBitLong(0);                 // imgVersion (class version)
    w.putRawDouble(1024.0);          // size.x → u
    w.putRawDouble(768.0);           // size.y → v
    w.putVariableText(ver, "img");   // name
    w.putBit(1);                     // loaded
    w.putRawChar8(2);                // resolution
    w.putRawDouble(0.25);            // up (pixel scale U)
    w.putRawDouble(0.5);             // vp (pixel scale V)

    // Handle stream: parentH, xdic (null), then the trailing XRefH that
    // DRW_ImageDef::parseDwg reads unconditionally.
    emitCommonHandlePrefix(w, /*parentHandle=*/0x10, /*reactors=*/{}, /*xDictFlag=*/0);
    w.putHandle(hardPtr(0));   // XRefH

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_ImageDef dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.u == 1024.0);        // was discarded before the fix
    REQUIRE(dst.v == 768.0);
    REQUIRE(dst.up == 0.25);         // pixel scale unaffected
    REQUIRE(dst.vp == 0.5);
    REQUIRE(dst.name == "img");
}

// 2a.0 (gap entity-reactors-xdict-dropped-roundtrip): DRW_TableEntry gained
// reactorHandles/xDictHandle. The hand-written copy ctor must copy them (a
// missing init-list entry would silently drop reactors on copy) and reset()
// must clear them.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_TableEntry copy ctor + reset handle reactor/xdict members",
          "[dwg-read][object-encode][phase2a]") {
    DRW_Block_Record src;   // any concrete DRW_TableEntry
    src.reactorHandles = {0xA0, 0xA1};
    src.xDictHandle = 0xB0;

    DRW_Block_Record copy(src);   // exercises the hand-written copy ctor
    REQUIRE(copy.reactorHandles.size() == 2u);
    REQUIRE(copy.reactorHandles[0] == 0xA0u);
    REQUIRE(copy.reactorHandles[1] == 0xA1u);
    REQUIRE(copy.xDictHandle == 0xB0u);

    copy.reset();
    REQUIRE(copy.reactorHandles.empty());
    REQUIRE(copy.xDictHandle == 0u);
    // The source must be untouched by the copy.
    REQUIRE(src.reactorHandles.size() == 2u);
    REQUIRE(src.xDictHandle == 0xB0u);
}

// 2a.6 (gap object-blockrecord-layout-explode-dropped): DRW_Block_Record now
// retains description (DXF 4), canExplode (280), blockScaling (281) and the
// owning LAYOUT handle (340), which parseDwg previously read-and-discarded.
// The full BLOCK_HEADER body is version-dependent and handle-heavy; the
// parse-path correctness is covered by the corpus golden check (read-only,
// member-assignment-only change). This guards the struct defaults + reset.
// NOTE: blockScaling/canExplode are populated only for R2007+ (>AC1018).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Block_Record exposes layout/explode/scaling/description members",
          "[dwg-read][object-encode][blockrecord]") {
    DRW_Block_Record br;
    // Documented defaults (used by the filter when a DWG omits them).
    REQUIRE(br.canExplode == true);
    REQUIRE(br.blockScaling == 0);
    REQUIRE(br.layoutHandle == 0u);
    REQUIRE(br.description.empty());

    // Mutate, then reset() must restore the documented defaults so a reused
    // record cannot leak a prior block's layout association.
    br.description = "stamped";
    br.canExplode = false;
    br.blockScaling = 3;
    br.layoutHandle = 0xABCD;
    br.reset();
    REQUIRE(br.canExplode == true);
    REQUIRE(br.blockScaling == 0);
    REQUIRE(br.layoutHandle == 0u);
    REQUIRE(br.description.empty());
}

// 0B.4a (gap object-mlinestyle-ltindex-inverted-version-gate-read):
// PRE-R2018 MLINESTYLE stores a signed inline lt index (BSd) per element;
// the version gate was inverted (read the BS only for R2018+), desyncing
// every element by one BS for the common R2000-R2013 case.  This test
// builds a 2-element AC1015 MLINESTYLE and asserts (a) both element
// offsets/colors stay aligned (no per-element BS drift), (b) the signed
// inline index is now CONSUMED and STORED in linetypeIndex (incl. a
// negative sentinel proving the signed read), and (c) buff stays good.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_MLineStyle::parseDwg captures per-element signed lt index (pre-R2018)",
          "[dwg-read][object-encode][mlinestyle]") {
    DRW::Version ver = DRW::AC1015;  // pre-R2018: inline BSd index present
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x400);
    w.putVariableText(ver, "MyStyle");    // name
    w.putVariableText(ver, "desc");       // description
    w.putBitShort(0);                     // flags
    w.putCmColor(ver, 256);               // fill color (CMC)
    w.putBitDouble(1.0471975512);         // startAngle
    w.putBitDouble(2.0943951024);         // endAngle
    w.putRawChar8(2);                     // numLines
    // element 0
    w.putBitDouble(0.5);                  // offset
    w.putCmColor(ver, 1);                 // color
    w.putSBitShort(0);                    // linetypeIndex (BSd) = 0
    // element 1 — uses a negative sentinel to prove the signed read.
    w.putBitDouble(-0.5);                 // offset
    w.putCmColor(ver, 2);                 // color
    w.putSBitShort(-2);                   // linetypeIndex (BSd) = -2

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_MLineStyle dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.name == "MyStyle");        // read before the element loop
    REQUIRE(dst.elements.size() == 2u);
    REQUIRE(dst.elements[0].offset == Approx(0.5));   // no BS drift
    REQUIRE(dst.elements[0].color == 1);
    REQUIRE(dst.elements[0].linetypeIndex == 0);      // inline index consumed
    REQUIRE(dst.elements[1].offset == Approx(-0.5));  // 2nd element aligned
    REQUIRE(dst.elements[1].color == 2);
    REQUIRE(dst.elements[1].linetypeIndex == -2);     // signed read preserved
    REQUIRE(r.isGood());                  // stream stayed aligned
}

// SPATIAL_INDEX parser test (ODA §20.4.95).  Body beyond timestamps is
// opaque, so we exercise only the timestamp capture.  Handle stream is
// gated to AC1024+ in the parser; under AC1018 the handles are not read.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SpatialIndex::parseDwg captures timestamps",
          "[dwg-read][object-encode][spatialindex]") {
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, /*handle=*/0x400);
    w.putBitLong(0xAAAA);
    w.putBitLong(0xBBBB);

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SpatialIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.timestamp1 == 0xAAAAu);
    REQUIRE(dst.timestamp2 == 0xBBBBu);
}

// DICTIONARY encoder round-trip (ODA §20.4.44).  Encodes into a buffer via
// DRW_Dictionary::encodeDwg, parses it back, asserts every field.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Dictionary::encodeDwg round-trips entries + cloning",
          "[dwg-write][object-encode][dictionary]") {
    DRW_Dictionary src;
    src.handle       = 0x500;
    src.parentHandle = 0xC;          // Named-objects dictionary
    src.cloning      = 1;
    src.hardOwner    = 1;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic
    src.m_entries.push_back({"ACAD_LAYOUT",       0x1A});
    src.m_entries.push_back({"ACAD_MLINESTYLE",   0x17});
    src.m_entries.push_back({"ACAD_PLOTSETTINGS", 0x19});

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/42 /* DICTIONARY */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionary(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Dictionary dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.cloning    == 1);
    REQUIRE(dst.hardOwner  == 1);
    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0xCu);
    REQUIRE(dst.m_entries.size() == 3u);
    REQUIRE(dst.m_entries[0].m_name   == "ACAD_LAYOUT");
    REQUIRE(dst.m_entries[0].m_handle == 0x1Au);
    REQUIRE(dst.m_entries[1].m_name   == "ACAD_MLINESTYLE");
    REQUIRE(dst.m_entries[1].m_handle == 0x17u);
    REQUIRE(dst.m_entries[2].m_name   == "ACAD_PLOTSETTINGS");
    REQUIRE(dst.m_entries[2].m_handle == 0x19u);
}

// SCALE encoder round-trip (ODA §20.4.92).  No handle stream in body —
// preamble + body fields only.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Scale::encodeDwg round-trips paper/drawing units + unit flag",
          "[dwg-write][object-encode][scale]") {
    DRW_Scale src;
    src.handle       = 0x600;
    src.flag         = 0;
    src.name         = "1:50";
    src.paperUnits   = 1.0;
    src.drawingUnits = 50.0;
    src.isUnitScale  = false;

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle);
    REQUIRE(DrwObjectEncodeTestAccess::encodeScale(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Scale dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.flag         == 0);
    REQUIRE(dst.name         == "1:50");
    REQUIRE(dst.paperUnits   == Approx(1.0));
    REQUIRE(dst.drawingUnits == Approx(50.0));
    REQUIRE(dst.isUnitScale  == false);
    REQUIRE(dst.scaleFactor() == Approx(50.0));
}

// GROUP encoder round-trip (ODA §20.4.72).  Description TV + unnamed flag +
// selectable flag + handle count + entity handle list.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Group::encodeDwg round-trips description + entity handles",
          "[dwg-write][object-encode][group]") {
    DRW_Group src;
    src.handle        = 0x700;
    src.parentHandle  = 0xC;
    src.m_description = "MyGroup";
    src.m_isUnnamed   = false;
    src.m_selectable  = true;
    src.m_entityHandles = {0x800, 0x801, 0x802};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);  // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/72 /* GROUP */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeGroup(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Group dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_description == "MyGroup");
    REQUIRE(dst.m_isUnnamed   == false);
    REQUIRE(dst.m_selectable  == true);
    REQUIRE(dst.m_entityHandles.size() == 3u);
    REQUIRE(dst.m_entityHandles[0] == 0x800u);
    REQUIRE(dst.m_entityHandles[1] == 0x801u);
    REQUIRE(dst.m_entityHandles[2] == 0x802u);
    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0xCu);
}

// XRECORD encoder round-trip (ODA §20.4.105 — typed-pair payload).  Covers
// every type bucket the parser recognises: string (TV), point (3RD), double
// (RD), int16 (RS), int32 (RL), int64 (RLL), bool (RC), byte (RC), binary
// (RC+bytes), and data-block handle (RS code + RLL).  The post-body handle
// stream carries common prefix (parent + reactors + xdic) followed by a
// pair of soft-pointer handle refs the parser stores with code 0.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_XRecord::encodeDwg round-trips every value type bucket",
          "[dwg-write][object-encode][xrecord]") {
    DRW_XRecord src;
    src.handle       = 0x800;
    src.parentHandle = 0xC;
    src.m_cloning    = 1;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    // Data-block values, one per type bucket.  Codes hand-picked from the
    // xRecordCodeIs* predicates to cover every branch.
    src.m_values.emplace_back(1,   UTF8STRING("hello"));               // string
    src.m_values.emplace_back(10,  DRW_Coord{1.0, 2.0, 3.0});           // 3D point
    src.m_values.emplace_back(40,  3.14159);                            // double
    src.m_values.emplace_back(70,  static_cast<dint32>(-5));            // int16
    src.m_values.emplace_back(90,  static_cast<dint32>(0x12345678));    // int32
    src.m_values.emplace_back(160, static_cast<dint64>(0x1122334455667788LL)); // int64
    src.m_values.emplace_back(290, static_cast<dint32>(1));             // bool
    src.m_values.emplace_back(280, static_cast<dint32>(7));             // byte
    src.m_values.emplace_back(310, std::vector<duint8>{0xDE, 0xAD, 0xBE, 0xEF}); // binary

    // Data-block handle — code 330 is in xRecordCodeIsHandle range; parser
    // stores the low 32 bits.
    src.m_handleValues.emplace_back(330, 0x1234u);

    // Handle-stream refs (code 0 marks handle-stream origin per parser).
    src.m_handleValues.emplace_back(0, 0xA1u);
    src.m_handleValues.emplace_back(0, 0xA2u);

    // AC1018 (R2004) — strings still inline (R2007 is the split point) and
    // the preamble's xDictFlag bit is honored, matching what the encoder
    // emits at the head of the handle stream.
    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/79 /* XRECORD */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeXRecord(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_XRecord dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_cloning == 1);
    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0xCu);
    REQUIRE(dst.m_values.size() == 9u);

    // String
    REQUIRE(dst.m_values[0].code() == 1);
    REQUIRE(dst.m_values[0].type() == DRW_Variant::STRING);
    REQUIRE(std::string(dst.m_values[0].c_str()) == "hello");

    // 3D point
    REQUIRE(dst.m_values[1].code() == 10);
    REQUIRE(dst.m_values[1].type() == DRW_Variant::COORD);
    DRW_Coord* p = dst.m_values[1].coord();
    REQUIRE(p != nullptr);
    REQUIRE(p->x == Approx(1.0));
    REQUIRE(p->y == Approx(2.0));
    REQUIRE(p->z == Approx(3.0));

    // Double
    REQUIRE(dst.m_values[2].code() == 40);
    REQUIRE(dst.m_values[2].type() == DRW_Variant::DOUBLE);
    REQUIRE(dst.m_values[2].d_val() == Approx(3.14159));

    // Int16 (stored as INTEGER)
    REQUIRE(dst.m_values[3].code() == 70);
    REQUIRE(dst.m_values[3].type() == DRW_Variant::INTEGER);
    // Parser reads RS as unsigned then casts via static_cast<int>; -5 (0xFFFB)
    // round-trips back as 0xFFFB / 65531 in unsigned representation.
    REQUIRE(static_cast<duint16>(dst.m_values[3].i_val() & 0xFFFF) == 0xFFFBu);

    // Int32
    REQUIRE(dst.m_values[4].code() == 90);
    REQUIRE(dst.m_values[4].type() == DRW_Variant::INTEGER);
    REQUIRE(static_cast<duint32>(dst.m_values[4].i_val()) == 0x12345678u);

    // Int64 (160-169)
    REQUIRE(dst.m_values[5].code() == 160);
    REQUIRE(dst.m_values[5].type() == DRW_Variant::INTEGER64);
    REQUIRE(dst.m_values[5].i64_val() == 0x1122334455667788LL);

    // Bool
    REQUIRE(dst.m_values[6].code() == 290);
    REQUIRE(dst.m_values[6].type() == DRW_Variant::INTEGER);
    REQUIRE(dst.m_values[6].i_val() == 1);

    // Byte
    REQUIRE(dst.m_values[7].code() == 280);
    REQUIRE(dst.m_values[7].type() == DRW_Variant::INTEGER);
    REQUIRE(dst.m_values[7].i_val() == 7);

    // Binary
    REQUIRE(dst.m_values[8].code() == 310);
    REQUIRE(dst.m_values[8].type() == DRW_Variant::BINARY);
    const std::vector<duint8>* raw = dst.m_values[8].binary();
    REQUIRE(raw != nullptr);
    REQUIRE(raw->size() == 4u);
    REQUIRE((*raw)[0] == 0xDE);
    REQUIRE((*raw)[3] == 0xEF);

    // Handles: 1 data-block (code 330) + 2 handle-stream (code 0) = 3.
    REQUIRE(dst.m_handleValues.size() == 3u);
    REQUIRE(dst.m_handleValues[0].first  == 330);
    REQUIRE(dst.m_handleValues[0].second == 0x1234u);
    REQUIRE(dst.m_handleValues[1].first  == 0);
    REQUIRE(dst.m_handleValues[1].second == 0xA1u);
    REQUIRE(dst.m_handleValues[2].first  == 0);
    REQUIRE(dst.m_handleValues[2].second == 0xA2u);
}

// LAYOUT encoder round-trip (ODA §20.4.84).  Exercises the PlotSettings prefix +
// layout-specific body + common handle prefix + type-specific handle tail at
// AC1018 (R2004), where the shade-plot block + viewport-count branch is active
// and strings are still inline (so we can avoid the separate strBuf path).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::encodeDwg round-trips PlotSettings + layout body + handles",
          "[dwg-write][object-encode][layout]") {
    DRW_Layout src;
    src.handle           = 0x1A;
    src.parentHandle     = 0x1A2;            // ACAD_LAYOUT dictionary
    src.pageSetupName    = "PrintSetup";
    src.printerConfig    = "Adobe PDF";
    src.plotLayoutFlags  = 9;
    src.marginLeft       = 7.5;
    src.marginBottom     = 20.0;
    src.marginRight      = 7.5;
    src.marginTop        = 20.0;
    src.paperWidth       = 297.0;
    src.paperHeight      = 420.0;
    src.paperSize        = "ISO A3 (297.00 x 420.00 MM)";
    src.plotOriginX      = 0.0;
    src.plotOriginY      = 0.0;
    src.paperUnits       = 0;
    src.plotRotation     = 1;
    src.plotType         = 5;
    src.windowMinX       = -10.0;
    src.windowMinY       = -10.0;
    src.windowMaxX       = 287.0;
    src.windowMaxY       = 410.0;
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 25.4;             // 1 inch = 25.4 mm
    src.currentStyleSheet = "acad.ctb";
    src.scaleType        = 16;
    src.scaleFactor      = 25.4;
    src.paperImageOriginX = 1.0;
    src.paperImageOriginY = 2.0;
    src.shadePlotMode      = 1;              // R2004+
    src.shadePlotResLevel  = 4;
    src.shadePlotCustomDPI = 600;
    src.name        = "Layout1";
    src.tabOrder    = 2;
    src.layoutFlags = 4;
    src.ucsOrigin   = DRW_Coord{1.0, 2.0, 3.0};
    src.limMinX     = -5.0;
    src.limMinY     = -7.5;
    src.limMaxX     = 200.0;
    src.limMaxY     = 300.0;
    src.insPoint    = DRW_Coord{10.0, 20.0, 0.0};
    src.ucsXAxis    = DRW_Coord{1.0, 0.0, 0.0};
    src.ucsYAxis    = DRW_Coord{0.0, 1.0, 0.0};
    src.elevation   = 0.25;
    src.orthoViewType = 6;
    src.extMin      = DRW_Coord{-1.0, -2.0, 0.0};
    src.extMax      = DRW_Coord{500.0, 600.0, 0.0};
    src.viewportCount = 2;
    src.viewportHandles = {0x80, 0x81};
    // Type-specific handles (encoder uses writeHandleOrHardPtr; supply refs
    // with code=0 so the encoder upgrades them to hard pointers).
    // Type-specific handles — leave code=0/size=0 and ref set; the encoder's
    // writeHandleOrHardPtr upgrades them to hard pointers.
    src.plotViewHandle.ref              = 0;
    src.paperSpaceBlockRecordHandle.ref = 0x70;
    src.lastActiveViewportHandle.ref    = 0x71;
    src.baseUcsHandle.ref               = 0x72;
    src.namedUcsHandle.ref              = 0x73;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);  // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/82 /* LAYOUT */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayout(src, ver, &w));

    auto bytes = snapshot(w);
    REQUIRE(bytes.size() > 0);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    // PlotSettings prefix
    REQUIRE(dst.pageSetupName    == "PrintSetup");
    REQUIRE(dst.printerConfig    == "Adobe PDF");
    REQUIRE(dst.plotLayoutFlags  == 9);
    REQUIRE(dst.marginLeft       == Approx(7.5));
    REQUIRE(dst.marginBottom     == Approx(20.0));
    REQUIRE(dst.marginRight      == Approx(7.5));
    REQUIRE(dst.marginTop        == Approx(20.0));
    REQUIRE(dst.paperWidth       == Approx(297.0));
    REQUIRE(dst.paperHeight      == Approx(420.0));
    REQUIRE(dst.paperSize        == "ISO A3 (297.00 x 420.00 MM)");
    REQUIRE(dst.plotOriginX      == Approx(0.0));
    REQUIRE(dst.plotOriginY      == Approx(0.0));
    REQUIRE(dst.paperUnits       == 0);
    REQUIRE(dst.plotRotation     == 1);
    REQUIRE(dst.plotType         == 5);
    REQUIRE(dst.windowMinX       == Approx(-10.0));
    REQUIRE(dst.windowMaxY       == Approx(410.0));
    REQUIRE(dst.realWorldUnits   == Approx(1.0));
    REQUIRE(dst.drawingUnits     == Approx(25.4));
    REQUIRE(dst.currentStyleSheet == "acad.ctb");
    REQUIRE(dst.scaleType        == 16);
    REQUIRE(dst.scaleFactor      == Approx(25.4));
    REQUIRE(dst.paperImageOriginX == Approx(1.0));
    REQUIRE(dst.paperImageOriginY == Approx(2.0));
    REQUIRE(dst.shadePlotMode      == 1);
    REQUIRE(dst.shadePlotResLevel  == 4);
    REQUIRE(dst.shadePlotCustomDPI == 600);

    // Layout-specific
    REQUIRE(dst.name        == "Layout1");
    REQUIRE(dst.tabOrder    == 2);
    REQUIRE(dst.layoutFlags == 4);
    REQUIRE(dst.ucsOrigin.x == Approx(1.0));
    REQUIRE(dst.ucsOrigin.y == Approx(2.0));
    REQUIRE(dst.ucsOrigin.z == Approx(3.0));
    REQUIRE(dst.limMinX     == Approx(-5.0));
    REQUIRE(dst.limMaxX     == Approx(200.0));
    REQUIRE(dst.insPoint.x  == Approx(10.0));
    REQUIRE(dst.insPoint.y  == Approx(20.0));
    REQUIRE(dst.elevation   == Approx(0.25));
    REQUIRE(dst.orthoViewType == 6);
    REQUIRE(dst.extMin.x    == Approx(-1.0));
    REQUIRE(dst.extMax.y    == Approx(600.0));
    REQUIRE(dst.viewportCount == 2);

    // Handle prefix + type-specific tail.
    REQUIRE(static_cast<duint32>(dst.parentHandle)        == 0x1A2u);
    REQUIRE(dst.paperSpaceBlockRecordHandle.ref           == 0x70u);
    REQUIRE(dst.lastActiveViewportHandle.ref              == 0x71u);
    REQUIRE(dst.baseUcsHandle.ref                         == 0x72u);
    REQUIRE(dst.namedUcsHandle.ref                        == 0x73u);
    REQUIRE(dst.viewportHandles.size() == 2u);
    REQUIRE(dst.viewportHandles[0] == 0x80u);
    REQUIRE(dst.viewportHandles[1] == 0x81u);
}

// LAYOUT encoder round-trip with non-zero reactors + xdic — verifies that the
// common handle prefix is emitted FIRST in the right order so the parser's
// readCommonObjectHandles consumes parentHandle + reactors + xdic before any
// type-specific handle.  Locks in the alignment contract introduced by PR 2.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Layout::encodeDwg emits common handle prefix before type-specific handles",
          "[dwg-write][object-encode][layout]") {
    DRW_Layout src;
    src.handle           = 0x1B;
    src.parentHandle     = 0x55;
    src.pageSetupName    = "Setup";
    src.printerConfig    = "Printer";
    src.paperWidth       = 297.0;
    src.paperHeight      = 210.0;
    src.paperSize        = "A4";
    src.realWorldUnits   = 1.0;
    src.drawingUnits     = 1.0;
    src.currentStyleSheet = "";
    src.scaleType        = 0;
    src.scaleFactor      = 1.0;
    src.shadePlotMode      = 0;
    src.shadePlotResLevel  = 0;
    src.shadePlotCustomDPI = 0;
    src.name        = "Layout1";
    src.tabOrder    = 1;
    src.layoutFlags = 0;
    src.viewportCount = 0;
    src.plotViewHandle.ref              = 0;
    src.paperSpaceBlockRecordHandle.ref = 0x90;
    src.lastActiveViewportHandle.ref    = 0x91;
    src.baseUcsHandle.ref               = 0x92;
    src.namedUcsHandle.ref              = 0x93;
    DrwObjectEncodeTestAccess::setNumReactors(src, 2);     // 2 reactors emitted as nulls
    DrwObjectEncodeTestAccess::setXDictFlag(src, 0);       // xdic present (null)

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/82, src.handle,
                       /*numReactors=*/2, /*xDictFlag=*/0);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayout(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Layout dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    // parentHandle must be 0x55, not 0 (and not consumed by paperSpaceBlockRecord).
    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x55u);
    // Type-specific handles land in the expected slots — proves the encoder
    // emitted (parent + 2 reactors + xdic) BEFORE the type-specific block.
    REQUIRE(dst.paperSpaceBlockRecordHandle.ref == 0x90u);
    REQUIRE(dst.lastActiveViewportHandle.ref    == 0x91u);
    REQUIRE(dst.baseUcsHandle.ref               == 0x92u);
    REQUIRE(dst.namedUcsHandle.ref              == 0x93u);
}

// RASTERVARIABLES encoder round-trip (ODA §20.4.91).  Tiny body: 4 ints +
// common handle prefix.  No type-specific handle tail.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_RasterVariables::encodeDwg round-trips classVersion + frame + quality + units",
          "[dwg-write][object-encode][rastervariables]") {
    DRW_RasterVariables src;
    src.handle       = 0x900;
    src.parentHandle = 0xC;          // Named-objects dictionary
    src.m_classVersion = 0;
    src.m_imageFrame   = 1;
    src.m_imageQuality = 1;
    src.m_units        = 2;          // millimeters
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0 /* custom-class */, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeRasterVariables(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_RasterVariables dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_classVersion == 0);
    REQUIRE(dst.m_imageFrame   == 1);
    REQUIRE(dst.m_imageQuality == 1);
    REQUIRE(dst.m_units        == 2);
    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0xCu);
}

// SORTENTSTABLE encoder round-trip (ODA §20.4.93).  Body: numEntries +
// inline sort handles.  Handle stream: common prefix + block owner + N
// entity handles.  N parallel arrays — sortHandles[i] corresponds to
// entityHandles[i] post-sort.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SortEntsTable::encodeDwg round-trips sort + entity handles + block owner",
          "[dwg-write][object-encode][sortentstable]") {
    DRW_SortEntsTable src;
    src.handle           = 0xA00;
    src.parentHandle     = 0x1F;            // BLOCK_RECORD owner
    src.m_blockOwnerHandle = 0x1F;
    src.m_sortHandles    = {0x101, 0x102, 0x103};
    src.m_entityHandles  = {0x201, 0x202, 0x203};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSortEntsTable(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SortEntsTable dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x1Fu);
    REQUIRE(dst.m_blockOwnerHandle == 0x1Fu);
    REQUIRE(dst.m_sortHandles.size() == 3u);
    REQUIRE(dst.m_sortHandles[0] == 0x101u);
    REQUIRE(dst.m_sortHandles[1] == 0x102u);
    REQUIRE(dst.m_sortHandles[2] == 0x103u);
    REQUIRE(dst.m_entityHandles.size() == 3u);
    REQUIRE(dst.m_entityHandles[0] == 0x201u);
    REQUIRE(dst.m_entityHandles[1] == 0x202u);
    REQUIRE(dst.m_entityHandles[2] == 0x203u);
}

// SPATIAL_FILTER encoder round-trip (ODA §20.4.94).  Boundary points + normal
// + origin + clip flags + optional clip distances + 4x3 transform matrices.
// Handle stream is just the common prefix (no type-specific handles).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SpatialFilter::encodeDwg round-trips clip boundary + transforms",
          "[dwg-write][object-encode][spatialfilter]") {
    DRW_SpatialFilter src;
    src.handle       = 0xB00;
    src.parentHandle = 0x44;                // INSERT/IMAGE owner
    src.m_boundaryPoints = {
        DRW_Coord{0.0, 0.0, 0.0},
        DRW_Coord{10.0, 0.0, 0.0},
        DRW_Coord{10.0, 10.0, 0.0},
        DRW_Coord{0.0, 10.0, 0.0}
    };
    src.m_normal = DRW_Coord{0.0, 0.0, 1.0};
    src.m_origin = DRW_Coord{0.0, 0.0, 0.0};
    src.m_displayBoundary = true;
    src.m_clipFrontPlane  = true;
    src.m_frontDistance   = 5.0;
    src.m_clipBackPlane   = false;
    src.m_backDistance    = 0.0;
    src.m_inverseInsertTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0
    };
    src.m_insertTransform = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0
    };
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSpatialFilter(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SpatialFilter dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x44u);
    REQUIRE(dst.m_boundaryPoints.size() == 4u);
    REQUIRE(dst.m_boundaryPoints[0].x == Approx(0.0));
    REQUIRE(dst.m_boundaryPoints[1].x == Approx(10.0));
    REQUIRE(dst.m_boundaryPoints[2].y == Approx(10.0));
    REQUIRE(dst.m_boundaryPoints[3].y == Approx(10.0));
    REQUIRE(dst.m_normal.z   == Approx(1.0));
    REQUIRE(dst.m_origin.x   == Approx(0.0));
    REQUIRE(dst.m_displayBoundary == true);
    REQUIRE(dst.m_clipFrontPlane  == true);
    REQUIRE(dst.m_frontDistance   == Approx(5.0));
    REQUIRE(dst.m_clipBackPlane   == false);
    REQUIRE(dst.m_inverseInsertTransform.size() == 12u);
    REQUIRE(dst.m_inverseInsertTransform[0]  == Approx(1.0));
    REQUIRE(dst.m_inverseInsertTransform[5]  == Approx(1.0));
    REQUIRE(dst.m_inverseInsertTransform[10] == Approx(1.0));
    REQUIRE(dst.m_insertTransform.size() == 12u);
    REQUIRE(dst.m_insertTransform[5] == Approx(1.0));
}

// GEODATA encoder round-trip (ODA §20.4.78).  Test the m_version=2/3 path
// (R2010+ schema) since that's what current AutoCAD releases write.
// Body: common handles + version + host block + coordinatesType + 13 fields
// + 3 observation TVs + N mesh points + N mesh faces.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_GeoData::encodeDwg round-trips version=2 metadata + mesh",
          "[dwg-write][object-encode][geodata]") {
    DRW_GeoData src;
    src.handle       = 0xC00;
    src.parentHandle = 0x1A;
    src.m_version    = 2;
    src.m_hostBlockHandle = 0x1F;
    src.m_coordinatesType = 1;
    src.m_designPoint    = DRW_Coord{0.0, 0.0, 0.0};
    src.m_referencePoint = DRW_Coord{500000.0, 4000000.0, 0.0};
    src.m_horizontalUnitScale = 0.3048;       // foot -> meter
    src.m_horizontalUnits     = 6;            // 6 = meters
    src.m_verticalUnitScale   = 1.0;
    src.m_verticalUnits       = 6;
    src.m_upDirection         = DRW_Coord{0.0, 0.0, 1.0};
    src.m_northDirection      = DRW_Coord{0.0, 1.0, 0.0};
    src.m_scaleEstimationMethod = 1;
    src.m_userSpecifiedScaleFactor = 1.0;
    src.m_enableSeaLevelCorrection = false;
    src.m_seaLevelElevation = 0.0;
    src.m_coordinateProjectionRadius = 6378137.0;
    src.m_coordinateSystemDefinition = "EPSG:3857";
    src.m_geoRssTag = "<rss/>";
    src.m_observationFromTag    = "from";
    src.m_observationToTag      = "to";
    src.m_observationCoverageTag = "coverage";
    src.m_points.push_back({DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{1.0, 1.0, 0.0}});
    src.m_points.push_back({DRW_Coord{1.0, 0.0, 0.0}, DRW_Coord{2.0, 1.0, 0.0}});
    src.m_faces.push_back({1, 2, 3});
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeGeoData(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_GeoData dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x1Au);
    REQUIRE(dst.m_version == 2);
    REQUIRE(dst.m_hostBlockHandle == 0x1Fu);
    REQUIRE(dst.m_coordinatesType == 1);
    REQUIRE(dst.m_designPoint.x    == Approx(0.0));
    REQUIRE(dst.m_referencePoint.x == Approx(500000.0));
    REQUIRE(dst.m_referencePoint.y == Approx(4000000.0));
    REQUIRE(dst.m_horizontalUnitScale == Approx(0.3048));
    REQUIRE(dst.m_horizontalUnits     == 6);
    REQUIRE(dst.m_verticalUnitScale   == Approx(1.0));
    REQUIRE(dst.m_verticalUnits       == 6);
    REQUIRE(dst.m_upDirection.z       == Approx(1.0));
    REQUIRE(dst.m_northDirection.y    == Approx(1.0));
    REQUIRE(dst.m_scaleEstimationMethod == 1);
    REQUIRE(dst.m_userSpecifiedScaleFactor == Approx(1.0));
    REQUIRE(dst.m_enableSeaLevelCorrection == false);
    REQUIRE(dst.m_coordinateProjectionRadius == Approx(6378137.0));
    REQUIRE(dst.m_coordinateSystemDefinition == "EPSG:3857");
    REQUIRE(dst.m_geoRssTag == "<rss/>");
    REQUIRE(dst.m_observationFromTag == "from");
    REQUIRE(dst.m_observationToTag   == "to");
    REQUIRE(dst.m_observationCoverageTag == "coverage");
    REQUIRE(dst.m_points.size() == 2u);
    REQUIRE(dst.m_points[0].m_source.x      == Approx(0.0));
    REQUIRE(dst.m_points[0].m_destination.y == Approx(1.0));
    REQUIRE(dst.m_points[1].m_source.x      == Approx(1.0));
    REQUIRE(dst.m_points[1].m_destination.x == Approx(2.0));
    REQUIRE(dst.m_faces.size() == 1u);
    REQUIRE(dst.m_faces[0].m_index1 == 1);
    REQUIRE(dst.m_faces[0].m_index2 == 2);
    REQUIRE(dst.m_faces[0].m_index3 == 3);
}

// DICTIONARYVAR encoder round-trip (ODA §20.4.46).  Body: schema RC + value TV.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_DictionaryVar::encodeDwg round-trips schema + value",
          "[dwg-write][object-encode][dictionaryvar]") {
    DRW_DictionaryVar src;
    src.handle       = 0xD00;
    src.parentHandle = 0x40;
    src.m_schema     = 0;
    src.m_value      = "Standard";
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionaryVar(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_DictionaryVar dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.m_schema == 0);
    REQUIRE(dst.m_value  == "Standard");
    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x40u);
}

// DICTIONARYWDFLT encoder round-trip (ODA §20.4.45).  Inherits DICTIONARY's
// body + handle stream and appends a default-entry handle.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_DictionaryWithDefault::encodeDwg round-trips dictionary + default entry",
          "[dwg-write][object-encode][dictionarywdflt]") {
    DRW_DictionaryWithDefault src;
    src.handle       = 0xE00;
    src.parentHandle = 0xC;
    src.cloning      = 1;
    src.hardOwner    = 1;
    src.m_entries.push_back({"ENTRY_A", 0x71});
    src.m_entries.push_back({"ENTRY_B", 0x72});
    src.m_defaultEntryHandle = 0x71;       // default points at ENTRY_A
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeDictionaryWDflt(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_DictionaryWithDefault dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.cloning      == 1);
    REQUIRE(dst.hardOwner    == 1);
    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0xCu);
    REQUIRE(dst.m_entries.size() == 2u);
    REQUIRE(dst.m_entries[0].m_name == "ENTRY_A");
    REQUIRE(dst.m_entries[0].m_handle == 0x71u);
    REQUIRE(dst.m_entries[1].m_name == "ENTRY_B");
    REQUIRE(dst.m_entries[1].m_handle == 0x72u);
    REQUIRE(dst.m_defaultEntryHandle == 0x71u);
}

// IDBUFFER encoder round-trip (ODA §20.4.79).  Body: class_version RC +
// numIds BL.  Handle stream: common prefix + N object handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_IDBuffer::encodeDwg round-trips object id list",
          "[dwg-write][object-encode][idbuffer]") {
    DRW_IDBuffer src;
    src.handle       = 0xF00;
    src.parentHandle = 0x10;
    src.classVersion = 0;
    src.objIds       = {0x101, 0x102, 0x103, 0x104};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeIDBuffer(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_IDBuffer dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x10u);
    REQUIRE(dst.classVersion == 0);
    REQUIRE(dst.objIds.size() == 4u);
    REQUIRE(dst.objIds[0] == 0x101u);
    REQUIRE(dst.objIds[1] == 0x102u);
    REQUIRE(dst.objIds[2] == 0x103u);
    REQUIRE(dst.objIds[3] == 0x104u);
}

// LAYER_INDEX encoder round-trip (ODA §20.4.83).  Body: timestamps + per-
// layer index + name.  Handle stream: common prefix + per-layer entry handle.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_LayerIndex::encodeDwg round-trips timestamps + per-layer entries",
          "[dwg-write][object-encode][layerindex]") {
    DRW_LayerIndex src;
    src.handle       = 0x1000;
    src.parentHandle = 0x10;
    src.timestamp1   = 0x99887766u;
    src.timestamp2   = 0x11223344u;
    src.entries.push_back({1, "0",       0x301u});
    src.entries.push_back({2, "Layer1",  0x302u});
    src.entries.push_back({3, "Layer2",  0x303u});
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeLayerIndex(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_LayerIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x10u);
    REQUIRE(dst.timestamp1 == 0x99887766u);
    REQUIRE(dst.timestamp2 == 0x11223344u);
    REQUIRE(dst.entries.size() == 3u);
    REQUIRE(dst.entries[0].indexLong == 1);
    REQUIRE(dst.entries[0].name == "0");
    REQUIRE(dst.entries[0].entryHandle == 0x301u);
    REQUIRE(dst.entries[1].indexLong == 2);
    REQUIRE(dst.entries[1].name == "Layer1");
    REQUIRE(dst.entries[1].entryHandle == 0x302u);
    REQUIRE(dst.entries[2].indexLong == 3);
    REQUIRE(dst.entries[2].name == "Layer2");
    REQUIRE(dst.entries[2].entryHandle == 0x303u);
}

// SPATIAL_INDEX encoder round-trip (ODA §20.4.95).  Body: timestamps only at
// AC1018 (parser leaves common handles unread pre-R2007 due to opaque blob).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_SpatialIndex::encodeDwg round-trips timestamps at AC1018",
          "[dwg-write][object-encode][spatialindex]") {
    DRW_SpatialIndex src;
    src.handle     = 0x1100;
    src.timestamp1 = 0xDEADBEEFu;
    src.timestamp2 = 0xCAFEBABEu;
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeSpatialIndex(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_SpatialIndex dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(dst.timestamp1 == 0xDEADBEEFu);
    REQUIRE(dst.timestamp2 == 0xCAFEBABEu);
}

// FIELD encoder round-trip (ODA §20.4.66) — exercises dataType=2 (double)
// value branch, the simplest CadValue path that round-trips cleanly at
// AC1018 (pre-R2007 omits formatFlags + unitType + format/value strings).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_Field::encodeDwg round-trips evaluator + double value + child handles",
          "[dwg-write][object-encode][field]") {
    DRW_Field src;
    src.handle       = 0x1200;
    src.parentHandle = 0x44;
    src.m_evaluatorId           = "AcVariable";
    src.m_fieldCode             = "%<\\AcVar Area>%";
    src.m_formatString          = "%lf";
    src.m_evaluationOptionFlags = 3;
    src.m_filingOptionFlags     = 1;
    src.m_fieldStateFlags       = 0;
    src.m_evaluationStatusFlags = 0;
    src.m_evaluationErrorCode   = 0;
    src.m_evaluationErrorMessage = "";
    src.m_value.m_dataType = 2;
    src.m_value.m_value.addDouble(140, 42.5);
    src.m_valueString       = "42.5";
    src.m_valueStringLength = 4;
    src.m_childHandles  = {0x401u, 0x402u};
    src.m_objectHandles = {0x501u};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeField(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_Field dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x44u);
    REQUIRE(dst.m_evaluatorId == "AcVariable");
    REQUIRE(dst.m_fieldCode   == "%<\\AcVar Area>%");
    REQUIRE(dst.m_formatString == "%lf");
    REQUIRE(dst.m_evaluationOptionFlags == 3);
    REQUIRE(dst.m_filingOptionFlags     == 1);
    REQUIRE(dst.m_value.m_dataType == 2);
    REQUIRE(dst.m_value.m_value.type() == DRW_Variant::DOUBLE);
    REQUIRE(dst.m_value.m_value.d_val() == Approx(42.5));
    REQUIRE(dst.m_valueString == "42.5");
    REQUIRE(dst.m_valueStringLength == 4);
    REQUIRE(dst.m_childHandles.size()  == 2u);
    REQUIRE(dst.m_childHandles[0]  == 0x401u);
    REQUIRE(dst.m_childHandles[1]  == 0x402u);
    REQUIRE(dst.m_objectHandles.size() == 1u);
    REQUIRE(dst.m_objectHandles[0] == 0x501u);
}

// FIELDLIST encoder round-trip (ODA §20.4.67).  Body: numFields BL + unknown
// bit B.  Handle stream: common prefix + N field handles.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DRW_FieldList::encodeDwg round-trips field handles",
          "[dwg-write][object-encode][fieldlist]") {
    DRW_FieldList src;
    src.handle       = 0x1300;
    src.parentHandle = 0x44;
    src.m_unknown    = 0;
    src.m_fieldHandles = {0x1200u, 0x1201u, 0x1202u};
    DrwObjectEncodeTestAccess::setNumReactors(src, 0);
    DrwObjectEncodeTestAccess::setXDictFlag(src, 1);   // no xdic

    DRW::Version ver = DRW::AC1018;
    dwgBufferW w;
    emitObjectPreamble(w, ver, /*oType=*/0, src.handle,
                       /*numReactors=*/0, /*xDictFlag=*/1);
    REQUIRE(DrwObjectEncodeTestAccess::encodeFieldList(src, ver, &w));

    auto bytes = snapshot(w);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_FieldList dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    REQUIRE(static_cast<duint32>(dst.parentHandle) == 0x44u);
    REQUIRE(dst.m_unknown == 0);
    REQUIRE(dst.m_fieldHandles.size() == 3u);
    REQUIRE(dst.m_fieldHandles[0] == 0x1200u);
    REQUIRE(dst.m_fieldHandles[1] == 0x1201u);
    REQUIRE(dst.m_fieldHandles[2] == 0x1202u);
}

// Phase 3A.0 — DRW_Dimstyle::syncStructToVars populates every $DIM key the
// LibreCAD createDimStyle consumer reads, from the parsed struct fields.
TEST_CASE("DRW_Dimstyle::syncStructToVars populates vars from struct fields",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle d;
    d.dimscale = 2.0;
    d.dimasz = 0.25;
    d.dimclrd = 1;
    d.dimlwd = 13;
    d.dimtxsty = "MyText";
    d.dimtad = 2;
    d.dimzin = 8;
    d.dimdec = 3;
    d.dimlunit = 4;

    DrwObjectEncodeTestAccess::syncDimstyle(d);

    REQUIRE(d.get("$DIMSCALE") != nullptr);
    CHECK(d.get("$DIMSCALE")->d_val() == Approx(2.0));
    CHECK(d.get("$DIMASZ")->d_val() == Approx(0.25));
    CHECK(d.get("$DIMCLRD")->i_val() == 1);
    CHECK(d.get("$DIMLWD")->i_val() == 13);
    REQUIRE(d.get("$DIMTXSTY") != nullptr);
    CHECK(std::string(d.get("$DIMTXSTY")->c_str()) == "MyText");
    CHECK(d.get("$DIMTAD")->i_val() == 2);
    CHECK(d.get("$DIMZIN")->i_val() == 8);
    CHECK(d.get("$DIMDEC")->i_val() == 3);
    CHECK(d.get("$DIMLUNIT")->i_val() == 4);
}

// Phase 3A.0 — sync is idempotent: the if(!get(key)) guard never clobbers a
// value already present in the vars map (DWG override / DXF-105 path).
TEST_CASE("DRW_Dimstyle::syncStructToVars is idempotent and never clobbers",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle d;
    d.dimscale = 2.0;
    // Pre-populate an override value; sync must not overwrite it.
    d.add("$DIMSCALE", 40, 9.0);

    DrwObjectEncodeTestAccess::syncDimstyle(d);
    REQUIRE(d.get("$DIMSCALE") != nullptr);
    CHECK(d.get("$DIMSCALE")->d_val() == Approx(9.0));

    // A second sync leaves the populated keys unchanged.
    const DRW_Variant* before = d.get("$DIMASZ");
    DrwObjectEncodeTestAccess::syncDimstyle(d);
    CHECK(d.get("$DIMASZ") == before);
    CHECK(d.get("$DIMSCALE")->d_val() == Approx(9.0));
}

// Phase 3A.0 — Rule-of-Five deep copy: copying a vars-populated DRW_Dimstyle
// and destroying both copies is clean (no double-free); mutating the copy
// does not affect the original.
TEST_CASE("DRW_Dimstyle deep-copy is independent and leak/double-free clean",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle original;
    original.dimscale = 3.5;
    original.dimtxt = 0.25;
    DrwObjectEncodeTestAccess::syncDimstyle(original);
    REQUIRE(original.get("$DIMSCALE")->d_val() == Approx(3.5));

    {
        DRW_Dimstyle copy(original);  // copy ctor — deep copy vars.
        REQUIRE(copy.get("$DIMSCALE") != nullptr);
        CHECK(copy.get("$DIMSCALE")->d_val() == Approx(3.5));
        // Pointers are distinct (deep copy, not shared).
        CHECK(copy.get("$DIMSCALE") != original.get("$DIMSCALE"));
        // Mutating the copy does not affect the original.
        copy.add("$DIMSCALE", 40, 7.0);
        CHECK(copy.get("$DIMSCALE")->d_val() == Approx(7.0));
        CHECK(original.get("$DIMSCALE")->d_val() == Approx(3.5));

        DRW_Dimstyle assigned;
        assigned = original;  // copy assignment.
        CHECK(assigned.get("$DIMSCALE")->d_val() == Approx(3.5));
        CHECK(assigned.get("$DIMSCALE") != original.get("$DIMSCALE"));

        DRW_Dimstyle moved(std::move(copy));  // move ctor steals vars.
        CHECK(moved.get("$DIMSCALE")->d_val() == Approx(7.0));
    }  // copy/assigned/moved destroyed here — must not double-free.

    // Original still intact after the inner scope destroyed its copies.
    CHECK(original.get("$DIMSCALE")->d_val() == Approx(3.5));
}

// Phase 3A.1 — new R2007/R2010 numeric/string members + handle refs
// default-construct/reset() to their documented defaults.
TEST_CASE("DRW_Dimstyle default-constructs R2007/R2010 members with defaults",
          "[dwg-object-encode][dimstyle][data-loss]") {
    DRW_Dimstyle d;
    CHECK(d.dimjogang == Approx(0.0));
    CHECK(d.dimtfill == 0);
    CHECK(d.dimtfillclr == 0);
    CHECK(d.dimarcsym == 0);
    CHECK(d.dimtxtdirection == 0);
    CHECK(d.dimaltmzf == Approx(1.0));
    CHECK(d.dimmzf == Approx(1.0));
    CHECK(d.dimaltmzs.empty());
    CHECK(d.dimmzs.empty());
    CHECK(d.dimtxstyH.ref == 0u);
    CHECK(d.dimldrblkH.ref == 0u);
    CHECK(d.dimblkH.ref == 0u);
    CHECK(d.dimblk1H.ref == 0u);
    CHECK(d.dimblk2H.ref == 0u);
    CHECK(d.dimltypeH.ref == 0u);
    CHECK(d.dimltex1H.ref == 0u);
    CHECK(d.dimltex2H.ref == 0u);

    // reset() restores the same defaults after mutation.
    d.dimjogang = 1.25;
    d.dimaltmzf = 5.0;
    d.dimtxstyH.ref = 0x99u;
    d.reset();
    CHECK(d.dimjogang == Approx(0.0));
    CHECK(d.dimaltmzf == Approx(1.0));
    CHECK(d.dimtxstyH.ref == 0u);
}

// Phase 4 (P4-04) — DRW_UCS::parseDwg reads origin/axes/elevation/orthoType +
// base/named handles. Synthetic AC1015 (R2000) UCS record per dwg.spec UCS
// binary field order; parsed back via the friend accessor.
TEST_CASE("DRW_UCS::parseDwg reads geometry, elevation, orthoType, handles",
          "[dwg-object-encode][ucs][data-loss]") {
    const DRW::Version ver = DRW::AC1015;
    constexpr duint16 ucsType = 0x3F;  // UCS table record.

    dwgBufferW body;
    emitObjectPreamble(body, ver, ucsType, /*handle=*/0x30u,
                       /*numReactors=*/0, /*xDictFlag=*/0);
    body.putVariableText(ver, std::string("MyUCS"));
    body.putBit(0);          // flags bit 7 (64)
    body.putBitShort(0);     // xrefindex (version < AC1021)
    body.putBit(0);          // flags bit 5 (16)
    body.put3BitDouble(DRW_Coord(10.0, 20.0, 0.0));   // ucsorg
    body.put3BitDouble(DRW_Coord(0.0, 1.0, 0.0));     // ucsxdir
    body.put3BitDouble(DRW_Coord(-1.0, 0.0, 0.0));    // ucsydir
    body.putBitDouble(5.0);  // ucs_elevation (BD, FIRST in binary order)
    body.putBitShort(1);     // UCSORTHOVIEW (BS)
    body.putBitShort(0);     // num_orthopts = 0
    // Handle stream: parent, xdic, then base_ucs, named_ucs.
    emitCommonHandlePrefix(body, /*parentHandle=*/0x3Eu, {}, /*xDictFlag=*/0);
    body.putHandle(hardPtr(0x100u));  // base_ucs
    body.putHandle(hardPtr(0x101u));  // named_ucs

    std::vector<duint8> bytes = snapshot(body);
    dwgBuffer r(bytes.data(), bytes.size());
    DRW_UCS dst;
    REQUIRE(DrwObjectEncodeTestAccess::parse(dst, ver, &r));

    CHECK(dst.name == "MyUCS");
    CHECK(dst.origin.x == Approx(10.0));
    CHECK(dst.origin.y == Approx(20.0));
    CHECK(dst.xAxisDirection.x == Approx(0.0));
    CHECK(dst.xAxisDirection.y == Approx(1.0));
    CHECK(dst.yAxisDirection.x == Approx(-1.0));
    CHECK(dst.elevation == Approx(5.0));
    CHECK(dst.orthoType == 1);
    CHECK(dst.baseUcsHandle.ref == 0x100u);
    CHECK(dst.namedUcsHandle.ref == 0x101u);
}
