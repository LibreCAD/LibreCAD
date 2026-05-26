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
 * Phase 2 smoke test: write a minimum-empty R2000 DWG and verify the
 * structural invariants the next phases will depend on.
 *
 * What this test catches:
 *   (a) writer doesn't crash on an empty input
 *   (b) file is the expected minimum size (~ few hundred bytes)
 *   (c) "AC1015" version string lands at byte 0
 *   (d) FILE_HEADER_END sentinel lands at the predicted offset
 *      (the libdxfrw reader's checkSentinel is a no-op, so explicit
 *      byte-compare here closes that gap)
 *   (e) HEADER and CLASSES begin sentinels appear at the addresses
 *      recorded in the file-header section locator
 *   (f) dwgRW::read() accepts the file (returns true)
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "drw_interface.h"
#include "intern/dwgutil.h"
#include "libdwgr.h"

namespace {

/// Minimal DRW_Interface that does nothing — Phase 2 writes an empty
/// document so no callbacks should fire (yet).
class EmptyIface : public DRW_Interface {
public:
    EmptyIface() = default;

    void addHeader(const DRW_Header*) override {}
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addArc(const DRW_Arc&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimArc(const DRW_DimArc*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}
};

/// Read a file fully into memory for byte-compare checks.
std::vector<duint8> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    in.seekg(0, std::ios::end);
    auto sz = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<duint8> buf(static_cast<size_t>(sz));
    if (sz > 0)
        in.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

/// Decode little-endian u32 at an offset.
duint32 readLE32(const std::vector<duint8>& buf, size_t off) {
    return static_cast<duint32>(buf[off]) |
           (static_cast<duint32>(buf[off + 1]) << 8) |
           (static_cast<duint32>(buf[off + 2]) << 16) |
           (static_cast<duint32>(buf[off + 3]) << 24);
}

std::string tempPath(const char* suffix) {
    auto dir = std::filesystem::temp_directory_path();
    auto path = dir / (std::string("dwg_write_smoke_") + suffix);
    return path.string();
}

} // namespace

TEST_CASE("dwgRW::write produces a syntactically valid empty R2000 file",
          "[dwg-write][smoke]") {
    const std::string path = tempPath("empty.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        bool ok = writer.write(&iface, DRW::AC1015, /*bin=*/false);
        REQUIRE(ok);
    }

    auto bytes = slurp(path);
    REQUIRE(bytes.size() > 100);  // sanity: at least a few sections worth

    // (c) Version string at offset 0.
    REQUIRE(std::memcmp(bytes.data(), "AC1015", 6) == 0);

    // Byte 0x0B: maintenance release version = 0x0F per LibreDWG.
    REQUIRE(bytes[0x0B] == 0x0F);
    // Byte 0x0C: zero_one_or_three marker.
    REQUIRE(bytes[0x0C] == 0x01);
    // Bytes 0x11-0x12: dwg_version + maint_version (= 0x17, 0x00).
    REQUIRE(bytes[0x11] == 0x17);
    REQUIRE(bytes[0x12] == 0x00);
    // Bytes 0x13-0x14: RS codepage LE = 30 (ANSI_1252).
    REQUIRE(bytes[0x13] == 30);
    REQUIRE(bytes[0x14] == 0);

    // Bytes 0x15-0x18: RL num_sections LE.
    duint32 numSections = readLE32(bytes, 0x15);
    REQUIRE(numSections == 5);

    // (d) FILE_HEADER_END sentinel at offset (0x19 + 9N + 2).
    size_t sentinelOffset = 0x19 + 9 * numSections + 2;
    REQUIRE(bytes.size() >= sentinelOffset + 16);
    REQUIRE(std::memcmp(bytes.data() + sentinelOffset,
                        dwgSentinels::FILE_HEADER_END, 16) == 0);

    // (e) HEADER section record (recno=0) — back-patched address must point
    // at the HEADER_BEGIN sentinel.
    duint32 headerAddr = readLE32(bytes, 0x19 + 0 * 9 + 1);
    duint32 headerSize = readLE32(bytes, 0x19 + 0 * 9 + 5);
    REQUIRE(headerAddr > 0);
    REQUIRE(headerSize >= 38);  // 16 + 4 + 0 + 16 + 2 = 38 minimum
    REQUIRE(headerAddr + 16 <= bytes.size());
    REQUIRE(std::memcmp(bytes.data() + headerAddr,
                        dwgSentinels::HEADER_BEGIN, 16) == 0);

    // CLASSES section record (recno=1) — sentinel must match.
    duint32 classesAddr = readLE32(bytes, 0x19 + 1 * 9 + 1);
    duint32 classesSize = readLE32(bytes, 0x19 + 1 * 9 + 5);
    REQUIRE(classesAddr > headerAddr);
    REQUIRE(classesSize >= 38);
    REQUIRE(std::memcmp(bytes.data() + classesAddr,
                        dwgSentinels::CLASSES_BEGIN, 16) == 0);

    // HANDLES section record (recno=2) — has real entries after Phase 3d
    // (10 control objects + terminator page).  Floor of 4 = empty terminator.
    duint32 handlesSize = readLE32(bytes, 0x19 + 2 * 9 + 5);
    REQUIRE(handlesSize >= 4);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW round-trip: write empty, reader returns true",
          "[dwg-write][smoke]") {
    const std::string path = tempPath("roundtrip.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
    }

    // Phase 3d milestone (landed 2026-05-15, ahead of the plan's Phase 3e
    // schedule): the 10 control objects in the object stream let
    // `readDwgTables` resolve every required control-handle lookup.
    // The +2 phantom child handles in BLOCK_CONTROL/LTYPE_CONTROL fail
    // their ObjectMap lookup but that's a silent per-handle warning in
    // the reader — not a section failure.  Phase 3e adds the actual
    // table records (BYLAYER/CONTINUOUS linetypes, layer "0", etc.).
    {
        dwgRW reader(path.c_str());
        EmptyIface iface;
        bool ok = reader.read(&iface, /*ext=*/false);
        REQUIRE(reader.getVersion() == DRW::AC1015);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
        REQUIRE(ok);
    }

    std::remove(path.c_str());
}

namespace {

/// Iface that populates LTSCALE + INSUNITS on the write side, then
/// captures the parsed header on the read side.  Used to prove that
/// caller-supplied header data round-trips through `dwgRW::write`.
class HeaderPopulatingIface : public EmptyIface {
public:
    double m_writeLtscale {0.0};
    int    m_writeInsunits {0};
    bool   m_writeCalled   {false};
    bool   m_readCalled    {false};
    double m_readLtscale   {0.0};
    int    m_readInsunits  {0};

    void writeHeader(DRW_Header& data) override {
        m_writeCalled = true;
        data.addDouble("LTSCALE", m_writeLtscale, 40);
        data.addInt("INSUNITS", m_writeInsunits, 70);
    }
    void addHeader(const DRW_Header* h) override {
        if (h == nullptr) return;
        m_readCalled = true;
        auto itL = h->vars.find("LTSCALE");
        if (itL != h->vars.end() && itL->second)
            m_readLtscale = itL->second->d_val();
        auto itU = h->vars.find("INSUNITS");
        if (itU != h->vars.end() && itU->second)
            m_readInsunits = static_cast<int>(itU->second->i_val());
    }
};

} // namespace

TEST_CASE("dwgRW::write invokes writeHeader and the values reach the file",
          "[dwg-write][smoke]") {
    const std::string path = tempPath("header_writeback.dwg");

    HeaderPopulatingIface writeIface;
    writeIface.m_writeLtscale  = 4.25;
    writeIface.m_writeInsunits = 4;  // mm

    {
        dwgRW writer(path.c_str());
        REQUIRE(writer.write(&writeIface, DRW::AC1015, /*bin=*/false));
    }
    REQUIRE(writeIface.m_writeCalled);  // proves writeHeader was invoked

    HeaderPopulatingIface readIface;
    {
        dwgRW reader(path.c_str());
        bool ok = reader.read(&readIface, /*ext=*/false);
        REQUIRE(ok);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
        REQUIRE(reader.getVersion() == DRW::AC1015);
    }
    REQUIRE(readIface.m_readCalled);
    REQUIRE(readIface.m_readLtscale  == 4.25);
    REQUIRE(readIface.m_readInsunits == 4);

    std::remove(path.c_str());
}

namespace {

/// Iface for the INSERT smoke test.  Defines a user block in
/// writeBlocks(), captures the returned block_record handle, then
/// writes a single INSERT entity referencing it.  On read, captures
/// the addBlock + addInsert callbacks and verifies the name resolution
/// works end-to-end.
class InsertRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    duint32 m_blockRecH {0};
    std::vector<std::string> m_blocks;
    std::vector<DRW_Insert>  m_inserts;

    void writeBlocks() override {
        if (m_writer == nullptr) return;
        m_blockRecH = m_writer->defineBlock("MySymbol", DRW_Coord{0.0, 0.0, 0.0});
    }
    void writeEntities() override {
        if (m_writer == nullptr || m_blockRecH == 0) return;
        DRW_Insert ins;
        ins.basePoint = DRW_Coord{50.0, 75.0, 0.0};
        ins.xscale = 2.0;
        ins.yscale = 2.0;
        ins.zscale = 2.0;
        ins.angle = 0.0;
        ins.color = 3;
        ins.blockRecH.ref = m_blockRecH;
        m_writer->writeInsert(&ins);
    }
    void addBlock(const DRW_Block& b)  override { m_blocks.push_back(b.name); }
    void addInsert(const DRW_Insert& i) override { m_inserts.push_back(i); }
};

} // namespace

TEST_CASE("dwgRW INSERT round-trips via defineBlock + writeInsert",
          "[dwg-write][smoke]") {
    const std::string path = tempPath("insert.dwg");

    {
        dwgRW writer(path.c_str());
        InsertRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
        REQUIRE(iface.m_blockRecH != 0);
    }

    InsertRoundTripIface readIface;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    // addBlock fires for the user block plus the canonical
    // *Model_Space / *Paper_Space.
    bool sawUserBlock = false;
    for (const auto& n : readIface.m_blocks) {
        if (n == "MySymbol") sawUserBlock = true;
    }
    REQUIRE(sawUserBlock);

    // The INSERT entity makes it through with the right block name +
    // position + scale.  findTableName resolves blockRecH.ref to the
    // user block_record's name only because BLOCK_CONTROL now lists it.
    REQUIRE(readIface.m_inserts.size() == 1);
    REQUIRE(readIface.m_inserts[0].name        == "MySymbol");
    REQUIRE(readIface.m_inserts[0].basePoint.x == 50.0);
    REQUIRE(readIface.m_inserts[0].basePoint.y == 75.0);
    REQUIRE(readIface.m_inserts[0].xscale      == 2.0);
    REQUIRE(readIface.m_inserts[0].yscale      == 2.0);
    REQUIRE(readIface.m_inserts[0].zscale      == 2.0);
    REQUIRE(readIface.m_inserts[0].color       == 3);

    std::remove(path.c_str());
}

namespace {

/// Iface that captures names from every table-record callback.  Used
/// to prove the Phase 3e milestone — the standard layer "0", linetype
/// "CONTINUOUS", textstyle "STANDARD", appid "ACAD", dimstyle
/// "STANDARD", vport "*ACTIVE" all reach the caller via their addXxx
/// callback after a round-trip through `dwgRW::write` + `dwgRW::read`.
class TableCaptureIface : public EmptyIface {
public:
    std::vector<std::string> m_layers;
    std::vector<std::string> m_lTypes;
    std::vector<std::string> m_textStyles;
    std::vector<std::string> m_appIds;
    std::vector<std::string> m_dimStyles;
    std::vector<std::string> m_vports;

    void addLayer(const DRW_Layer& l) override { m_layers.push_back(l.name); }
    void addLType(const DRW_LType& l) override { m_lTypes.push_back(l.name); }
    void addTextStyle(const DRW_Textstyle& t) override { m_textStyles.push_back(t.name); }
    void addAppId(const DRW_AppId& a) override { m_appIds.push_back(a.name); }
    void addDimStyle(const DRW_Dimstyle& d) override { m_dimStyles.push_back(d.name); }
    void addVport(const DRW_Vport& v) override { m_vports.push_back(v.name); }
};

bool containsName(const std::vector<std::string>& v, const std::string& name) {
    for (const auto& s : v) if (s == name) return true;
    return false;
}

} // namespace

namespace {

/// Iface that writes a fixed set of POINT / LINE / CIRCLE / ARC
/// entities through the new Phase 4b public API, and captures them
/// back on the read side.  Proves the round-trip:
///   user iface → dwgRW::writePoint → object stream → file →
///   readDwgEntities → addPoint callback.
class EntityRoundTripIface : public EmptyIface {
public:
    // Write-side: dwgRW to call back into.
    dwgRW *m_writer {nullptr};

    // Captured on read.
    std::vector<DRW_Point>   m_points;
    std::vector<DRW_Line>    m_lines;
    std::vector<DRW_Circle>  m_circles;
    std::vector<DRW_Arc>     m_arcs;
    std::vector<DRW_Ellipse> m_ellipses;
    std::vector<std::string> m_blocks;

    void writeEntities() override {
        if (m_writer == nullptr) return;
        // Emit one of each.
        DRW_Point pt;
        pt.basePoint = DRW_Coord{1.5, 2.5, 0.0};
        pt.color = 2;
        m_writer->writePoint(&pt);

        DRW_Line ln;
        ln.basePoint = DRW_Coord{0.0, 0.0, 0.0};
        ln.secPoint  = DRW_Coord{10.0, 5.0, 0.0};
        ln.color = 3;
        m_writer->writeLine(&ln);

        DRW_Circle ci;
        ci.basePoint = DRW_Coord{100.0, 100.0, 0.0};
        ci.radious = 25.0;
        ci.color = 5;
        m_writer->writeCircle(&ci);

        DRW_Arc ar;
        ar.basePoint = DRW_Coord{50.0, 50.0, 0.0};
        ar.radious = 10.0;
        ar.staangle = 0.0;
        ar.endangle = 3.141592653589793;
        ar.color = 6;
        m_writer->writeArc(&ar);

        DRW_Ellipse el;
        el.basePoint = DRW_Coord{200.0, 200.0, 0.0};
        el.secPoint  = DRW_Coord{30.0, 0.0, 0.0};
        el.extPoint  = DRW_Coord{0.0, 0.0, 1.0};
        el.ratio = 0.5;
        el.staparam = 0.0;
        el.endparam = 6.283185307179586;
        el.color = 4;
        m_writer->writeEllipse(&el);
    }

    void addPoint(const DRW_Point& p)    override { m_points.push_back(p); }
    void addLine(const DRW_Line& l)      override { m_lines.push_back(l); }
    void addCircle(const DRW_Circle& c)  override { m_circles.push_back(c); }
    void addArc(const DRW_Arc& a)        override { m_arcs.push_back(a); }
    void addEllipse(const DRW_Ellipse& e) override { m_ellipses.push_back(e); }
    void addBlock(const DRW_Block& b)    override { m_blocks.push_back(b.name); }
};

} // namespace

TEST_CASE("dwgRW writes POINT/LINE/CIRCLE/ARC and reader recovers them",
          "[dwg-write][smoke]") {
    const std::string path = tempPath("entities.dwg");

    {
        dwgRW writer(path.c_str());
        EntityRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
    }

    EntityRoundTripIface readIface;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(readIface.m_points.size()   == 1);
    REQUIRE(readIface.m_lines.size()    == 1);
    REQUIRE(readIface.m_circles.size()  == 1);
    REQUIRE(readIface.m_arcs.size()     == 1);
    REQUIRE(readIface.m_ellipses.size() == 1);

    REQUIRE(readIface.m_points[0].basePoint.x == 1.5);
    REQUIRE(readIface.m_points[0].basePoint.y == 2.5);
    REQUIRE(readIface.m_points[0].color       == 2);

    REQUIRE(readIface.m_lines[0].basePoint.x == 0.0);
    REQUIRE(readIface.m_lines[0].secPoint.x  == 10.0);
    REQUIRE(readIface.m_lines[0].secPoint.y  == 5.0);
    REQUIRE(readIface.m_lines[0].color       == 3);

    REQUIRE(readIface.m_circles[0].basePoint.x == 100.0);
    REQUIRE(readIface.m_circles[0].radious     == 25.0);
    REQUIRE(readIface.m_circles[0].color       == 5);

    REQUIRE(readIface.m_arcs[0].basePoint.x == 50.0);
    REQUIRE(readIface.m_arcs[0].radious     == 10.0);
    REQUIRE(readIface.m_arcs[0].staangle    == 0.0);
    REQUIRE(readIface.m_arcs[0].endangle    == 3.141592653589793);
    REQUIRE(readIface.m_arcs[0].color       == 6);

    REQUIRE(readIface.m_ellipses[0].basePoint.x == 200.0);
    REQUIRE(readIface.m_ellipses[0].secPoint.x  == 30.0);
    REQUIRE(readIface.m_ellipses[0].ratio       == 0.5);
    REQUIRE(readIface.m_ellipses[0].color       == 4);

    // Phase 4d milestone: addBlock fires for *Model_Space and
    // *Paper_Space — the BLOCK_CONTROL +2 phantom handles resolve
    // through the Block_Record + DRW_Block emission rather than the
    // pre-4d silent warnings.
    bool sawModel = false;
    bool sawPaper = false;
    for (const auto& n : readIface.m_blocks) {
        if (n == "*Model_Space") sawModel = true;
        if (n == "*Paper_Space") sawPaper = true;
    }
    REQUIRE(sawModel);
    REQUIRE(sawPaper);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW round-trip delivers the standard R2000 table records",
          "[dwg-write][smoke]") {
    const std::string path = tempPath("standard_tables.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
    }

    TableCaptureIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    // Phase 3e milestone: the standard fixed-handle table records make
    // it through readDwgTables and fire their respective iface callbacks.
    // libdxfrw's reader name resolution may produce empty names for the
    // minimum-stub records — check for "non-empty name" presence rather
    // than exact match, since the stub parses past the name field into
    // garbage and the final stored name may vary slightly by record type.
    REQUIRE(containsName(cap.m_lTypes,    "BYBLOCK"));
    REQUIRE(containsName(cap.m_lTypes,    "BYLAYER"));
    REQUIRE(containsName(cap.m_lTypes,    "CONTINUOUS"));
    REQUIRE(containsName(cap.m_layers,    "0"));
    REQUIRE(containsName(cap.m_textStyles,"STANDARD"));
    REQUIRE(containsName(cap.m_appIds,    "ACAD"));
    REQUIRE(containsName(cap.m_dimStyles, "STANDARD"));
    REQUIRE(containsName(cap.m_vports,    "*ACTIVE"));

    std::remove(path.c_str());
}

// ---- R2004 (AC1018) write smoke tests ---------------------------------------

TEST_CASE("dwgRW::write produces a syntactically valid empty R2004 file",
          "[dwg-write][smoke][r2004]") {
    const std::string path = tempPath("empty_r2004.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1018, /*bin=*/false));
    }

    auto bytes = slurp(path);
    // R2004 fixed file header is 0x100 bytes; data pages follow immediately.
    REQUIRE(bytes.size() > 0x100);

    // Version string at offset 0.
    REQUIRE(std::memcmp(bytes.data(), "AC1018", 6) == 0);

    // Bytes 6-10: five NUL padding bytes.
    for (int i = 6; i <= 10; ++i)
        REQUIRE(bytes[i] == 0x00);

    // Bytes 19-20: codepage LE = 30 (ANSI_1252).
    REQUIRE(bytes[19] == 30);
    REQUIRE(bytes[20] == 0);

    // Bytes 40-43: constant 0x00000080 LE.
    REQUIRE(bytes[40] == 0x80);
    REQUIRE(bytes[41] == 0x00);
    REQUIRE(bytes[42] == 0x00);
    REQUIRE(bytes[43] == 0x00);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2004 round-trip: write empty, reader returns true",
          "[dwg-write][smoke][r2004]") {
    const std::string path = tempPath("roundtrip_r2004.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1018, /*bin=*/false));
    }

    {
        dwgRW reader(path.c_str());
        EmptyIface iface;
        bool ok = reader.read(&iface, /*ext=*/false);
        REQUIRE(reader.getVersion() == DRW::AC1018);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
        REQUIRE(ok);
    }

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2004 writes POINT/LINE/CIRCLE/ARC and reader recovers them",
          "[dwg-write][smoke][r2004]") {
    const std::string path = tempPath("entities_r2004.dwg");

    {
        dwgRW writer(path.c_str());
        EntityRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1018, /*bin=*/false));
    }

    EntityRoundTripIface readIface;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getVersion() == DRW::AC1018);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(readIface.m_points.size()   == 1);
    REQUIRE(readIface.m_lines.size()    == 1);
    REQUIRE(readIface.m_circles.size()  == 1);
    REQUIRE(readIface.m_arcs.size()     == 1);
    REQUIRE(readIface.m_ellipses.size() == 1);

    REQUIRE(readIface.m_points[0].basePoint.x == 1.5);
    REQUIRE(readIface.m_points[0].basePoint.y == 2.5);
    REQUIRE(readIface.m_points[0].color       == 2);

    REQUIRE(readIface.m_lines[0].basePoint.x == 0.0);
    REQUIRE(readIface.m_lines[0].secPoint.x  == 10.0);
    REQUIRE(readIface.m_lines[0].secPoint.y  == 5.0);
    REQUIRE(readIface.m_lines[0].color       == 3);

    REQUIRE(readIface.m_circles[0].basePoint.x == 100.0);
    REQUIRE(readIface.m_circles[0].radious     == 25.0);
    REQUIRE(readIface.m_circles[0].color       == 5);

    REQUIRE(readIface.m_arcs[0].basePoint.x == 50.0);
    REQUIRE(readIface.m_arcs[0].radious     == 10.0);
    REQUIRE(readIface.m_arcs[0].staangle    == 0.0);
    REQUIRE(readIface.m_arcs[0].endangle    == 3.141592653589793);
    REQUIRE(readIface.m_arcs[0].color       == 6);

    REQUIRE(readIface.m_ellipses[0].basePoint.x == 200.0);
    REQUIRE(readIface.m_ellipses[0].secPoint.x  == 30.0);
    REQUIRE(readIface.m_ellipses[0].ratio       == 0.5);
    REQUIRE(readIface.m_ellipses[0].color       == 4);

    bool sawModel = false;
    bool sawPaper = false;
    for (const auto& n : readIface.m_blocks) {
        if (n == "*Model_Space") sawModel = true;
        if (n == "*Paper_Space") sawPaper = true;
    }
    REQUIRE(sawModel);
    REQUIRE(sawPaper);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2004 round-trip delivers the standard table records",
          "[dwg-write][smoke][r2004]") {
    const std::string path = tempPath("standard_tables_r2004.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1018, /*bin=*/false));
    }

    TableCaptureIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_lTypes,    "BYBLOCK"));
    REQUIRE(containsName(cap.m_lTypes,    "BYLAYER"));
    REQUIRE(containsName(cap.m_lTypes,    "CONTINUOUS"));
    REQUIRE(containsName(cap.m_layers,    "0"));
    REQUIRE(containsName(cap.m_textStyles,"STANDARD"));
    REQUIRE(containsName(cap.m_appIds,    "ACAD"));
    REQUIRE(containsName(cap.m_dimStyles, "STANDARD"));
    REQUIRE(containsName(cap.m_vports,    "*ACTIVE"));

    std::remove(path.c_str());
}

// ---- R2010 (AC1024) write smoke tests ---------------------------------------

TEST_CASE("dwgRW::write produces a syntactically valid empty R2010 file",
          "[dwg-write][smoke][r2010]") {
    const std::string path = tempPath("empty_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    auto bytes = slurp(path);
    REQUIRE(bytes.size() > 0x100);
    REQUIRE(std::memcmp(bytes.data(), "AC1024", 6) == 0);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2010 round-trip: write empty, reader returns true",
          "[dwg-write][smoke][r2010]") {
    const std::string path = tempPath("roundtrip_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    {
        dwgRW reader(path.c_str());
        EmptyIface iface;
        bool ok = reader.read(&iface, /*ext=*/false);
        REQUIRE(reader.getVersion() == DRW::AC1024);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
        REQUIRE(ok);
    }

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2010 writes POINT/LINE/CIRCLE/ARC and reader recovers them",
          "[dwg-write][smoke][r2010]") {
    const std::string path = tempPath("entities_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        EntityRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    EntityRoundTripIface readIface;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getVersion() == DRW::AC1024);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(readIface.m_points.size()   == 1);
    REQUIRE(readIface.m_lines.size()    == 1);
    REQUIRE(readIface.m_circles.size()  == 1);
    REQUIRE(readIface.m_arcs.size()     == 1);
    REQUIRE(readIface.m_ellipses.size() == 1);

    REQUIRE(readIface.m_points[0].basePoint.x == 1.5);
    REQUIRE(readIface.m_points[0].basePoint.y == 2.5);
    REQUIRE(readIface.m_points[0].color       == 2);

    REQUIRE(readIface.m_lines[0].basePoint.x == 0.0);
    REQUIRE(readIface.m_lines[0].secPoint.x  == 10.0);
    REQUIRE(readIface.m_lines[0].secPoint.y  == 5.0);
    REQUIRE(readIface.m_lines[0].color       == 3);

    REQUIRE(readIface.m_circles[0].basePoint.x == 100.0);
    REQUIRE(readIface.m_circles[0].radious     == 25.0);
    REQUIRE(readIface.m_circles[0].color       == 5);

    REQUIRE(readIface.m_arcs[0].basePoint.x == 50.0);
    REQUIRE(readIface.m_arcs[0].radious     == 10.0);
    REQUIRE(readIface.m_arcs[0].staangle    == 0.0);
    REQUIRE(readIface.m_arcs[0].endangle    == 3.141592653589793);
    REQUIRE(readIface.m_arcs[0].color       == 6);

    REQUIRE(readIface.m_ellipses[0].basePoint.x == 200.0);
    REQUIRE(readIface.m_ellipses[0].secPoint.x  == 30.0);
    REQUIRE(readIface.m_ellipses[0].ratio       == 0.5);
    REQUIRE(readIface.m_ellipses[0].color       == 4);

    bool sawModel = false;
    bool sawPaper = false;
    for (const auto& n : readIface.m_blocks) {
        if (n == "*Model_Space") sawModel = true;
        if (n == "*Paper_Space") sawPaper = true;
    }
    REQUIRE(sawModel);
    REQUIRE(sawPaper);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2010 round-trip delivers the standard table records",
          "[dwg-write][smoke][r2010]") {
    const std::string path = tempPath("standard_tables_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    TableCaptureIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_lTypes,    "BYBLOCK"));
    REQUIRE(containsName(cap.m_lTypes,    "BYLAYER"));
    REQUIRE(containsName(cap.m_lTypes,    "CONTINUOUS"));
    REQUIRE(containsName(cap.m_layers,    "0"));
    REQUIRE(containsName(cap.m_textStyles,"STANDARD"));
    REQUIRE(containsName(cap.m_appIds,    "ACAD"));
    REQUIRE(containsName(cap.m_dimStyles, "STANDARD"));
    REQUIRE(containsName(cap.m_vports,    "*ACTIVE"));

    std::remove(path.c_str());
}

// ---- R2010 user-defined table round-trip tests --------------------------------

namespace {

/// Iface that registers a custom layer "WALLS" (color=1/red) on write and
/// captures layer names on read.  Exercises the add*() infrastructure end-to-end.
class CustomLayerIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<std::string> m_layerNames;
    int m_wallsColor {-1};

    void writeLayers() override {
        if (m_writer == nullptr) return;
        DRW_Layer lay;
        lay.name  = "WALLS";
        lay.color = 1;
        m_writer->addLayer(&lay);
    }
    void addLayer(const DRW_Layer& l) override {
        m_layerNames.push_back(l.name);
        if (l.name == "WALLS") m_wallsColor = l.color;
    }
};

/// Iface that registers a custom linetype "DASHED" with a 2-element dash
/// pattern on write, and captures linetype names on read.
class CustomLtypeIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<std::string> m_ltNames;

    void writeLTypes() override {
        if (m_writer == nullptr) return;
        DRW_LType lt;
        lt.name = "DASHED";
        lt.desc = "Dashed __  __  __";
        lt.length = 0.375;
        lt.path.push_back(0.25);
        lt.path.push_back(-0.125);
        lt.size = static_cast<int>(lt.path.size());
        m_writer->addLType(&lt);
    }
    void addLType(const DRW_LType& l) override { m_ltNames.push_back(l.name); }
};

/// Iface that writes a POINT on layer "WALLS" and reads it back.
/// Validates layer-name→handle resolution in encodeEntity().
class EntityOnLayerIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<std::string> m_pointLayers;
    std::vector<std::string> m_layerNames;

    void writeLayers() override {
        if (m_writer == nullptr) return;
        DRW_Layer lay;
        lay.name  = "WALLS";
        lay.color = 2;
        m_writer->addLayer(&lay);
    }
    void writeEntities() override {
        if (m_writer == nullptr) return;
        DRW_Point pt;
        pt.basePoint = DRW_Coord{3.0, 4.0, 0.0};
        pt.layer = "WALLS";
        m_writer->writePoint(&pt);
    }
    void addPoint(const DRW_Point& p) override { m_pointLayers.push_back(p.layer); }
    void addLayer(const DRW_Layer& l) override { m_layerNames.push_back(l.name); }
};

} // namespace

TEST_CASE("dwgRW R2010 round-trip with custom layer",
          "[dwg-write][smoke][r2010]") {
    const std::string path = tempPath("custom_layer_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        CustomLayerIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    CustomLayerIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_layerNames, "0"));
    REQUIRE(containsName(cap.m_layerNames, "WALLS"));
    REQUIRE(cap.m_wallsColor == 1);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2010 entity on custom layer",
          "[dwg-write][smoke][r2010]") {
    const std::string path = tempPath("entity_on_layer_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        EntityOnLayerIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    EntityOnLayerIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_layerNames, "WALLS"));
    REQUIRE(cap.m_pointLayers.size() == 1);
    REQUIRE(cap.m_pointLayers[0] == "WALLS");

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2010 round-trip with custom linetype",
          "[dwg-write][smoke][r2010]") {
    const std::string path = tempPath("custom_ltype_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        CustomLtypeIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    CustomLtypeIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_ltNames, "BYBLOCK"));
    REQUIRE(containsName(cap.m_ltNames, "BYLAYER"));
    REQUIRE(containsName(cap.m_ltNames, "CONTINUOUS"));
    REQUIRE(containsName(cap.m_ltNames, "DASHED"));

    std::remove(path.c_str());
}

namespace {

/// Reusable iface for R2000/R2004 custom-layer round-trip tests.
class CustomLayerR2000Iface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<std::string> m_layerNames;
    int m_wallsColor {-1};

    void writeLayers() override {
        if (m_writer == nullptr) return;
        DRW_Layer lay;
        lay.name  = "WALLS";
        lay.color = 1;
        m_writer->addLayer(&lay);
    }
    void addLayer(const DRW_Layer& l) override {
        m_layerNames.push_back(l.name);
        if (l.name == "WALLS") m_wallsColor = l.color;
    }
};

} // namespace

TEST_CASE("dwgRW R2000 round-trip with custom layer",
          "[dwg-write][smoke]") {
    const std::string path = tempPath("custom_layer_r2000.dwg");

    {
        dwgRW writer(path.c_str());
        CustomLayerR2000Iface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
    }

    CustomLayerR2000Iface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_layerNames, "0"));
    REQUIRE(containsName(cap.m_layerNames, "WALLS"));
    REQUIRE(cap.m_wallsColor == 1);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2004 round-trip with custom layer",
          "[dwg-write][smoke][r2004]") {
    const std::string path = tempPath("custom_layer_r2004.dwg");

    {
        dwgRW writer(path.c_str());
        CustomLayerR2000Iface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1018, /*bin=*/false));
    }

    CustomLayerR2000Iface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_layerNames, "0"));
    REQUIRE(containsName(cap.m_layerNames, "WALLS"));
    REQUIRE(cap.m_wallsColor == 1);

    std::remove(path.c_str());
}

// ---- Phase 5 entity encoder smoke tests --------------------------------

namespace {

class ViewportRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Viewport> m_viewports;

    void writeEntities() override {
        if (m_writer == nullptr) return;
        DRW_Viewport vp;
        vp.pswidth  = 297.0;
        vp.psheight = 210.0;
        m_writer->writeViewport(&vp);
    }
    void addViewport(const DRW_Viewport& vp) override {
        m_viewports.push_back(vp);
    }
};

class PolylineRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Polyline> m_polylines;

    void writeEntities() override {
        if (m_writer == nullptr) return;
        DRW_Polyline pl;
        pl.flags = 1;  // closed 2D polyline (bit 0)
        pl.addVertex(DRW_Vertex(0.0,  0.0, 0.0, 0.0));
        pl.addVertex(DRW_Vertex(10.0, 0.0, 0.0, 0.0));
        pl.addVertex(DRW_Vertex(10.0, 5.0, 0.0, 0.0));
        m_writer->writePolyline(&pl);
    }
    void addPolyline(const DRW_Polyline& pl) override {
        m_polylines.push_back(pl);
    }
};

} // namespace (phase-5 ifaces)

TEST_CASE("dwgRW R2010 viewport entity round-trip", "[dwg-write][r2010][smoke]") {
    const std::string path = tempPath("viewport_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        ViewportRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    ViewportRoundTripIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(cap.m_viewports.size() == 1);
    REQUIRE(cap.m_viewports[0].pswidth  == 297.0);
    REQUIRE(cap.m_viewports[0].psheight == 210.0);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2010 polyline (2D) round-trip", "[dwg-write][r2010][smoke]") {
    const std::string path = tempPath("polyline_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        PolylineRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    PolylineRoundTripIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(cap.m_polylines.size() == 1);
    REQUIRE(cap.m_polylines[0].vertlist.size() == 3);
    REQUIRE(cap.m_polylines[0].vertlist[0]->basePoint.x == 0.0);
    REQUIRE(cap.m_polylines[0].vertlist[1]->basePoint.x == 10.0);
    REQUIRE(cap.m_polylines[0].vertlist[2]->basePoint.y == 5.0);

    std::remove(path.c_str());
}

// ---- R2013 (AC1027) write smoke tests ---------------------------------------

TEST_CASE("dwgRW::write produces a syntactically valid empty R2013 file",
          "[dwg-write][smoke][r2013]") {
    const std::string path = tempPath("empty_r2013.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1027, /*bin=*/false));
    }

    auto bytes = slurp(path);
    REQUIRE(bytes.size() > 0x100);
    REQUIRE(std::memcmp(bytes.data(), "AC1027", 6) == 0);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2013 round-trip: write empty, reader returns true",
          "[dwg-write][smoke][r2013]") {
    const std::string path = tempPath("roundtrip_r2013.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1027, /*bin=*/false));
    }

    {
        dwgRW reader(path.c_str());
        EmptyIface iface;
        bool ok = reader.read(&iface, /*ext=*/false);
        REQUIRE(reader.getVersion() == DRW::AC1027);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
        REQUIRE(ok);
    }

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2013 writes POINT/LINE/CIRCLE/ARC and reader recovers them",
          "[dwg-write][smoke][r2013]") {
    const std::string path = tempPath("entities_r2013.dwg");

    {
        dwgRW writer(path.c_str());
        EntityRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1027, /*bin=*/false));
    }

    EntityRoundTripIface readIface;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getVersion() == DRW::AC1027);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(readIface.m_points.size()   == 1);
    REQUIRE(readIface.m_lines.size()    == 1);
    REQUIRE(readIface.m_circles.size()  == 1);
    REQUIRE(readIface.m_arcs.size()     == 1);
    REQUIRE(readIface.m_ellipses.size() == 1);

    REQUIRE(readIface.m_points[0].basePoint.x == 1.5);
    REQUIRE(readIface.m_points[0].basePoint.y == 2.5);
    REQUIRE(readIface.m_points[0].color       == 2);

    REQUIRE(readIface.m_lines[0].basePoint.x == 0.0);
    REQUIRE(readIface.m_lines[0].secPoint.x  == 10.0);
    REQUIRE(readIface.m_lines[0].secPoint.y  == 5.0);
    REQUIRE(readIface.m_lines[0].color       == 3);

    REQUIRE(readIface.m_circles[0].basePoint.x == 100.0);
    REQUIRE(readIface.m_circles[0].radious     == 25.0);
    REQUIRE(readIface.m_circles[0].color       == 5);

    REQUIRE(readIface.m_arcs[0].basePoint.x == 50.0);
    REQUIRE(readIface.m_arcs[0].radious     == 10.0);
    REQUIRE(readIface.m_arcs[0].staangle    == 0.0);
    REQUIRE(readIface.m_arcs[0].endangle    == 3.141592653589793);
    REQUIRE(readIface.m_arcs[0].color       == 6);

    REQUIRE(readIface.m_ellipses[0].basePoint.x == 200.0);
    REQUIRE(readIface.m_ellipses[0].secPoint.x  == 30.0);
    REQUIRE(readIface.m_ellipses[0].ratio       == 0.5);
    REQUIRE(readIface.m_ellipses[0].color       == 4);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2013 round-trip delivers the standard table records",
          "[dwg-write][smoke][r2013]") {
    const std::string path = tempPath("standard_tables_r2013.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1027, /*bin=*/false));
    }

    TableCaptureIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(containsName(cap.m_lTypes,    "BYBLOCK"));
    REQUIRE(containsName(cap.m_lTypes,    "BYLAYER"));
    REQUIRE(containsName(cap.m_lTypes,    "CONTINUOUS"));
    REQUIRE(containsName(cap.m_layers,    "0"));
    REQUIRE(containsName(cap.m_textStyles,"STANDARD"));
    REQUIRE(containsName(cap.m_appIds,    "ACAD"));
    REQUIRE(containsName(cap.m_dimStyles, "STANDARD"));
    REQUIRE(containsName(cap.m_vports,    "*ACTIVE"));

    std::remove(path.c_str());
}

// ---- R2018 (AC1032) write smoke tests ---------------------------------------

TEST_CASE("dwgRW::write produces a syntactically valid empty R2018 file",
          "[dwg-write][smoke][r2018]") {
    const std::string path = tempPath("empty_r2018.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1032, /*bin=*/false));
    }

    auto bytes = slurp(path);
    REQUIRE(bytes.size() > 0x100);
    REQUIRE(std::memcmp(bytes.data(), "AC1032", 6) == 0);

    std::remove(path.c_str());
}

namespace {

class R2018MTextRoundTripIface : public EntityRoundTripIface {
public:
    std::vector<DRW_MText> m_mtexts;

    void writeEntities() override {
        EntityRoundTripIface::writeEntities();
        if (m_writer == nullptr) return;

        DRW_MText mt;
        mt.basePoint = DRW_Coord{12.0, 34.0, 0.0};
        mt.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        mt.secPoint = DRW_Coord{1.0, 0.0, 0.0};
        mt.widthscale = 80.0;
        mt.height = 2.5;
        mt.textgen = DRW_MText::TopLeft;
        mt.alignH = DRW_Text::HLeft;
        mt.interlin = 1.25;
        mt.text = "R2018 MTEXT";
        mt.color = 2;
        mt.m_backgroundFlags = 0x10; // R2018 text frame bit
        mt.m_backgroundScale = 150;
        mt.m_backgroundColor = 3;
        mt.m_backgroundTransparency = 0;
        mt.m_r2018IsNotAnnotative = true;
        mt.m_r2018Version = 1;
        mt.m_r2018DefaultFlag = false;
        mt.m_r2018Attachment = DRW_MText::TopLeft;
        mt.m_r2018XAxisDir = DRW_Coord{1.0, 0.0, 0.0};
        mt.m_r2018InsertionPoint = mt.basePoint;
        mt.m_r2018RectWidth = mt.widthscale;
        mt.m_r2018RectHeight = 7.5;
        mt.m_r2018ExtentsHeight = 2.5;
        mt.m_r2018ExtentsWidth = 30.0;
        mt.m_r2018ColumnType = 2;
        mt.m_r2018ColumnCount = 2;
        mt.m_r2018ColumnWidth = 25.0;
        mt.m_r2018ColumnGutter = 1.5;
        mt.m_r2018ColumnAutoHeight = false;
        mt.m_r2018ColumnFlowReversed = true;
        mt.m_r2018ColumnHeights = {7.5, 8.0};
        REQUIRE(m_writer->writeMText(&mt));
    }

    void addMText(const DRW_MText& m) override { m_mtexts.push_back(m); }
};

} // namespace

TEST_CASE("dwgRW R2018 writes geometry and MTEXT then reader recovers them",
          "[dwg-write][smoke][r2018]") {
    const std::string path = tempPath("entities_r2018.dwg");

    {
        dwgRW writer(path.c_str());
        R2018MTextRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1032, /*bin=*/false));
    }

    R2018MTextRoundTripIface readIface;
    {
        dwgRW reader(path.c_str());
        bool ok = reader.read(&readIface, /*ext=*/false);
        INFO("reader error = " << reader.getError());
        REQUIRE(ok);
        REQUIRE(reader.getVersion() == DRW::AC1032);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(readIface.m_points.size() == 1);
    REQUIRE(readIface.m_lines.size() == 1);
    REQUIRE(readIface.m_mtexts.size() == 1);
    REQUIRE(readIface.m_mtexts[0].basePoint.x == 12.0);
    REQUIRE(readIface.m_mtexts[0].basePoint.y == 34.0);
    REQUIRE(readIface.m_mtexts[0].height == 2.5);
    REQUIRE(readIface.m_mtexts[0].text == "R2018 MTEXT");
    REQUIRE(readIface.m_mtexts[0].interlin == 1.25);
    REQUIRE(readIface.m_mtexts[0].m_backgroundFlags == 0x10);
    REQUIRE(readIface.m_mtexts[0].m_backgroundScale == 150);
    REQUIRE(readIface.m_mtexts[0].m_backgroundColor == 3);
    REQUIRE(readIface.m_mtexts[0].m_r2018IsNotAnnotative);
    REQUIRE(readIface.m_mtexts[0].m_r2018Version == 1);
    REQUIRE(readIface.m_mtexts[0].m_r2018Attachment == DRW_MText::TopLeft);
    REQUIRE(readIface.m_mtexts[0].m_r2018AppIdHandle == 0x14u);
    REQUIRE(readIface.m_mtexts[0].m_r2018RectWidth == 80.0);
    REQUIRE(readIface.m_mtexts[0].m_r2018RectHeight == 7.5);
    REQUIRE(readIface.m_mtexts[0].m_r2018ColumnType == 2);
    REQUIRE(readIface.m_mtexts[0].m_r2018ColumnCount == 2);
    REQUIRE(readIface.m_mtexts[0].m_r2018ColumnFlowReversed);
    REQUIRE(readIface.m_mtexts[0].m_r2018ColumnHeights.size() == 2);
    REQUIRE(readIface.m_mtexts[0].m_r2018ColumnHeights[1] == 8.0);

    std::remove(path.c_str());
}
