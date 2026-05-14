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
