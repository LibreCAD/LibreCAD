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
    static dint16 oType(const DRW_TableEntry& e) { return e.oType; }
    static void setNumReactors(DRW_TableEntry& e, dint32 n) { e.numReactors = n; }
    static void setXDictFlag(DRW_TableEntry& e, duint8 f) { e.xDictFlag = f; }
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
