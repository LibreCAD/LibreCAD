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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include <QCoreApplication>

#include "drw_interface.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgutil.h"
#include "lc_containertraverser.h"
#include "libdwgr.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_polyline.h"
#include "rs_settings.h"

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
duint16 readLE16(const std::vector<duint8>& buf, size_t off) {
    return static_cast<duint16>(buf[off]) |
           (static_cast<duint16>(buf[off + 1]) << 8);
}

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

void ensureQtSettings() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char *argv[] = {arg0, nullptr};
    static QCoreApplication *app = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(argc, argv);
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)settingsReady;
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
    REQUIRE(numSections == 6);

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

    // AuxHeader section record (recno=5) — native R2000 writes now include
    // AcDb:AuxHeader instead of advertising only the first five locators.
    duint32 auxAddr = readLE32(bytes, 0x19 + 5 * 9 + 1);
    duint32 auxSize = readLE32(bytes, 0x19 + 5 * 9 + 5);
    REQUIRE(auxAddr > 0);
    REQUIRE(auxSize >= 111);
    REQUIRE(auxAddr + auxSize <= bytes.size());
    REQUIRE(bytes[auxAddr + 0] == 0xff);
    REQUIRE(bytes[auxAddr + 1] == 0x77);
    REQUIRE(bytes[auxAddr + 2] == 0x01);
    REQUIRE(readLE16(bytes, auxAddr + 3) == 23);

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

/// 2b.6: writes a MINSERT (col/row grid) and captures it on read.
class MInsertRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    duint32 m_blockRecH {0};
    std::vector<DRW_Insert> m_inserts;

    void writeBlocks() override {
        if (m_writer == nullptr) return;
        m_blockRecH = m_writer->defineBlock("GridSym", DRW_Coord{0.0, 0.0, 0.0});
    }
    void writeEntities() override {
        if (m_writer == nullptr || m_blockRecH == 0) return;
        DRW_Insert ins;
        ins.basePoint = DRW_Coord{10.0, 20.0, 0.0};
        ins.xscale = 1.0; ins.yscale = 1.0; ins.zscale = 1.0;
        ins.angle = 0.0;
        ins.color = 4;
        ins.colcount = 3;     // grid → MINSERT (oType 8)
        ins.rowcount = 2;
        ins.colspace = 10.0;
        ins.rowspace = 5.0;
        ins.blockRecH.ref = m_blockRecH;
        m_writer->writeInsert(&ins);
    }
    void addInsert(const DRW_Insert& i) override { m_inserts.push_back(i); }
};

} // namespace

// 2b.6 (gap minsert-attribs-dwg-write-drop): a MINSERT grid (col/row/spacing)
// now encodes as oType 8 and round-trips; a one-bit grid offset would desync
// the BLOCK_HEADER handle read, so the block reference resolving back to
// "GridSym" is the alignment proof.
TEST_CASE("dwgRW MINSERT round-trips col/row/spacing grid",
          "[dwg-write][smoke][minsert]") {
    for (DRW::Version ver : {DRW::AC1015, DRW::AC1018}) {
        const std::string path = tempPath("minsert.dwg");
        {
            dwgRW writer(path.c_str());
            MInsertRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, ver, /*bin=*/false));
            REQUIRE(iface.m_blockRecH != 0);
        }
        MInsertRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }
        REQUIRE(readIface.m_inserts.size() == 1);
        REQUIRE(readIface.m_inserts[0].name == "GridSym");  // handle aligned
        REQUIRE(readIface.m_inserts[0].colcount == 3);
        REQUIRE(readIface.m_inserts[0].rowcount == 2);
        REQUIRE(readIface.m_inserts[0].colspace == 10.0);
        REQUIRE(readIface.m_inserts[0].rowspace == 5.0);
        REQUIRE(readIface.m_inserts[0].basePoint.x == 10.0);
        std::remove(path.c_str());
    }
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
    std::vector<std::string> m_views;

    void addLayer(const DRW_Layer& l) override { m_layers.push_back(l.name); }
    void addLType(const DRW_LType& l) override { m_lTypes.push_back(l.name); }
    void addTextStyle(const DRW_Textstyle& t) override { m_textStyles.push_back(t.name); }
    void addAppId(const DRW_AppId& a) override { m_appIds.push_back(a.name); }
    void addDimStyle(const DRW_Dimstyle& d) override { m_dimStyles.push_back(d.name); }
    void addVport(const DRW_Vport& v) override { m_vports.push_back(v.name); }
    void addView(const DRW_View& v) override { m_views.push_back(v.name); }
};

bool containsName(const std::vector<std::string>& v, const std::string& name) {
    for (const auto& s : v) if (s == name) return true;
    return false;
}

} // namespace

namespace {

class ViewRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_View> m_views;

    void writeViews() override {
        if (m_writer == nullptr)
            return;
        DRW_View view;
        view.name = "NAMED-VIEW";
        view.size.x = 44.0;
        view.size.y = 22.0;
        view.center.x = 10.5;
        view.center.y = -3.25;
        view.targetPoint.x = 1.0;
        view.targetPoint.y = 2.0;
        view.targetPoint.z = 3.0;
        view.viewDirectionFromTarget.z = 1.0;
        view.twistAngle = 0.125;
        view.lensLen = 55.0;
        view.frontClippingPlaneOffset = 0.5;
        view.backClippingPlaneOffset = 250.0;
        view.viewMode = 1 | 16;
        view.renderMode = 2;
        view.cameraPlottable = true;
        view.hasUCS = true;
        view.ucsOrigin.x = 4.0;
        view.ucsOrigin.y = 5.0;
        view.ucsOrigin.z = 6.0;
        view.ucsXAxis.x = 1.0;
        view.ucsYAxis.y = 1.0;
        view.ucsElevation = 7.0;
        view.ucsOrthoType = 3;
        m_writer->addView(&view);
    }

    void addView(const DRW_View& view) override { m_views.push_back(view); }
};

void requireNamedViewRoundTrip(const DRW_View& view, DRW::Version version) {
    REQUIRE(view.name == "NAMED-VIEW");
    REQUIRE(view.size.x == 44.0);
    REQUIRE(view.size.y == 22.0);
    REQUIRE(view.center.x == 10.5);
    REQUIRE(view.center.y == -3.25);
    REQUIRE(view.targetPoint.x == 1.0);
    REQUIRE(view.targetPoint.y == 2.0);
    REQUIRE(view.targetPoint.z == 3.0);
    REQUIRE(view.viewDirectionFromTarget.z == 1.0);
    REQUIRE(view.twistAngle == 0.125);
    REQUIRE(view.lensLen == 55.0);
    REQUIRE(view.frontClippingPlaneOffset == 0.5);
    REQUIRE(view.backClippingPlaneOffset == 250.0);
    REQUIRE(view.viewMode == (1 | 16));
    REQUIRE(view.renderMode == 2);
    if (version > DRW::AC1018)
        REQUIRE(view.cameraPlottable);
    REQUIRE(view.hasUCS);
    REQUIRE(view.ucsOrigin.x == 4.0);
    REQUIRE(view.ucsOrigin.y == 5.0);
    REQUIRE(view.ucsOrigin.z == 6.0);
    REQUIRE(view.ucsXAxis.x == 1.0);
    REQUIRE(view.ucsYAxis.y == 1.0);
    REQUIRE(view.ucsElevation == 7.0);
    REQUIRE(view.ucsOrthoType == 3);
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

class ArcDimensionRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_DimArc> m_arcDimensions;

    void writeEntities() override {
        if (m_writer == nullptr) return;

        DRW_DimArc dim;
        dim.setArcDefPoint(DRW_Coord{5.0, 6.0, 0.0});
        dim.setExtLine1(DRW_Coord{1.0, 0.0, 0.0});
        dim.setExtLine2(DRW_Coord{0.0, 1.0, 0.0});
        dim.setArcCenter(DRW_Coord{0.0, 0.0, 0.0});
        dim.setLeaderPt1(DRW_Coord{2.0, 2.0, 0.0});
        dim.leaderPt2 = DRW_Coord{3.0, 3.0, 0.0};
        dim.arcStartAngle = 0.25;
        dim.arcEndAngle = 1.25;
        dim.isPartial = true;
        dim.hasLeader = true;
        dim.color = 2;
        m_writer->writeDimension(&dim);
    }

    void addDimArc(const DRW_DimArc *dim) override {
        if (dim != nullptr)
            m_arcDimensions.push_back(*dim);
    }
};

DRW_UnsupportedObject makeRawReplayObject(DRW::Version version) {
    constexpr duint16 rawClassNumber = 509;
    dwgBufferW body;
    body.putObjType(version, rawClassNumber);

    DRW_UnsupportedObject object;
    object.m_objectType = rawClassNumber;
    object.m_handle = 0x700;
    object.m_bodyBitSize = version > DRW::AC1021 ? body.bitCount() : 0;
    object.m_objectSize = static_cast<duint32>(body.data().size());
    object.m_isEntity = false;
    object.m_isCustomClass = true;
    object.m_recordName = "RAW_REPLAY_TEST";
    object.m_className = "AcDbRawReplayTest";
    object.m_rawBytes = body.data();
    return object;
}

class RawObjectReplayIface : public EmptyIface {
public:
    explicit RawObjectReplayIface(DRW::Version version)
        : m_rawObject(makeRawReplayObject(version))
    {}

    dwgRW *m_writer {nullptr};
    DRW_UnsupportedObject m_rawObject;
    std::vector<DRW_UnsupportedObject> m_unsupportedObjects;
    duint32 m_readHandseed {0};

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            m_writer->registerRawDwgObjectClass(&m_rawObject);
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeRawDwgObject(&m_rawObject));
    }

    void addHeader(const DRW_Header *header) override {
        if (header != nullptr)
            m_readHandseed = header->getHandSeed();
    }

    void addUnsupportedObject(const DRW_UnsupportedObject &object) override {
        m_unsupportedObjects.push_back(object);
    }
};

class ConflictingRawClassIface : public EmptyIface {
public:
    explicit ConflictingRawClassIface(DRW::Version version)
        : m_first(makeRawReplayObject(version))
        , m_second(makeRawReplayObject(version))
    {
        m_second.m_recordName = "RAW_REPLAY_CONFLICT";
        m_second.m_className = "AcDbRawReplayConflict";
        m_second.m_handle = 0x701;
    }

    dwgRW *m_writer {nullptr};
    DRW_UnsupportedObject m_first;
    DRW_UnsupportedObject m_second;

    void writeDwgClasses() override {
        if (m_writer == nullptr)
            return;
        REQUIRE(m_writer->registerRawDwgObjectClass(&m_first));
        REQUIRE_FALSE(m_writer->registerRawDwgObjectClass(&m_second));
    }
};

class InspectableDwgWriter15 : public dwgWriter15 {
public:
    InspectableDwgWriter15(std::ofstream *stream, DRW_Header *header)
        : dwgWriter15(stream, header)
    {}

    dint32 customClassInstanceCount(duint16 classNum) const {
        for (const DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (definition.m_classNum == classNum)
                return definition.m_instanceCount;
        }
        return -1;
    }

    // 0B.3: expose the raw item_class_id (0x1F2 entity / 0x1F3 object) so the
    // writer-side conformance can be asserted before the reader collapses it.
    dint32 customClassItemClassId(duint16 classNum) const {
        for (const DwgClassDefinition& definition : m_dwgClassDefinitions) {
            if (definition.m_classNum == classNum)
                return definition.m_entityFlagRaw;
        }
        return -1;
    }

    bool registerObjectClassForTest(const DRW_UnsupportedObject& object) {
        return registerRawObjectClass(object);
    }
};

class MLeaderRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_MLeader> m_mleaders;

    void writeEntities() override {
        if (m_writer == nullptr)
            return;

        DRW_MLeader leader;
        leader.color = 2;
        leader.classVersion = 2;
        leader.leaderType = 1;
        leader.leaderColor = 2;
        leader.leaderLineWeight = 29;
        leader.landingEnabled = true;
        leader.doglegEnabled = true;
        leader.landingDistance = 2.5;
        leader.defaultArrowHeadSize = 0.75;
        leader.styleContentType = 2;
        leader.styleHandle.ref = 0x830u;
        leader.leaderLineTypeHandle.ref = 0x14u;
        leader.arrowHeadHandle.ref = 0x15u;
        leader.styleTextStyleHandle.ref = 0x13u;
        leader.scaleFactor = 1.0;

        DRW_MLeaderRoot root;
        root.isContentValid = true;
        root.unknown291 = true;
        root.connectionPoint = DRW_Coord{20.0, 5.0, 0.0};
        root.direction = DRW_Coord{1.0, 0.0, 0.0};
        root.leaderIndex = 0;
        root.landingDistance = 2.5;

        DRW_MLeaderLeaderLine line;
        line.leaderLineIndex = 7;
        line.points.push_back(DRW_Coord{0.0, 0.0, 0.0});
        line.points.push_back(DRW_Coord{10.0, 5.0, 0.0});
        line.leaderType = 1;
        line.color = 2;
        line.lineWeight = 29;
        line.arrowSize = 0.75;
        line.lineTypeHandle.ref = 0x14u;
        line.arrowHandle.ref = 0x15u;
        root.leaderLines.push_back(line);
        leader.context.roots.push_back(root);

        leader.context.overallScale = 1.0;
        leader.context.contentBasePoint = DRW_Coord{20.0, 5.0, 0.0};
        leader.context.textHeight = 2.0;
        leader.context.arrowHeadSize = 0.75;
        leader.context.landingGap = 0.25;
        leader.context.styleLeftAttach = 1;
        leader.context.styleRightAttach = 1;
        leader.context.hasTextContents = true;
        leader.context.textLabel = "Native MLeader";
        leader.context.textStyleHandle.ref = 0x13u;
        leader.context.textNormal = DRW_Coord{0.0, 0.0, 1.0};
        leader.context.textLocation = DRW_Coord{20.0, 5.0, 0.0};
        leader.context.textDirection = DRW_Coord{1.0, 0.0, 0.0};
        leader.context.boundaryWidth = 30.0;
        leader.context.boundaryHeight = 5.0;
        leader.context.lineSpacingFactor = 1.0;
        leader.context.lineSpacingStyle = 1;
        leader.context.textColor = 2;
        leader.context.alignment = 1;
        leader.context.flowDirection = 1;
        leader.context.bgScaleFactor = 1.5;
        leader.context.basePoint = DRW_Coord{20.0, 5.0, 0.0};
        leader.context.baseDirection = DRW_Coord{1.0, 0.0, 0.0};
        leader.context.baseVertical = DRW_Coord{0.0, 1.0, 0.0};

        REQUIRE(m_writer->writeMLeader(&leader));
    }

    void addMLeader(const DRW_MLeader *leader) override {
        if (leader != nullptr)
            m_mleaders.push_back(*leader);
    }
};

class MLeaderStyleRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_MLeaderStyle m_style;
    std::vector<DRW_MLeaderStyle> m_styles;
    std::vector<DRW_UnsupportedObject> m_rawObjects;

    MLeaderStyleRoundTripIface() {
        m_style.handle = 0x830u;
        m_style.name = "CodexStyle";
        m_style.styleVersion = 2u;
        m_style.contentType = 2u;
        m_style.drawMLeaderOrder = 1u;
        m_style.drawLeaderOrder = 0u;
        m_style.maxLeaderPoints = 7;
        m_style.firstSegmentAngle = 0.25;
        m_style.secondSegmentAngle = 0.5;
        m_style.leaderType = 1u;
        m_style.leaderColor = 3;
        m_style.leaderLineTypeHandle.ref = 0x14u;
        m_style.leaderLineWeight = 29;
        m_style.landingEnabled = true;
        m_style.landingGap = 0.125;
        m_style.autoIncludeLanding = true;
        m_style.landingDistance = 2.25;
        m_style.description = "Round-trip MLeader style";
        m_style.arrowHeadBlockHandle.ref = 0x15u;
        m_style.arrowHeadSize = 0.75;
        m_style.textDefault = "Default leader text";
        m_style.textStyleHandle.ref = 0x13u;
        m_style.leftAttachment = 1u;
        m_style.rightAttachment = 2u;
        m_style.textAngleType = 1u;
        m_style.textAlignmentType = 2u;
        m_style.textColor = 5;
        m_style.textHeight = 2.5;
        m_style.textFrameEnabled = true;
        m_style.alwaysAlignTextLeft = true;
        m_style.alignSpace = 0.2;
        m_style.blockHandle.ref = 0x17u;
        m_style.blockColor = 6;
        m_style.blockScale = DRW_Coord{1.0, 2.0, 3.0};
        m_style.blockScaleEnabled = true;
        m_style.blockRotation = 0.75;
        m_style.blockRotationEnabled = true;
        m_style.blockConnectionType = 1u;
        m_style.scaleFactor = 1.5;
        m_style.propertyChanged = true;
        m_style.isAnnotative = true;
        m_style.breakSize = 0.375;
        m_style.attachmentDirection = 1u;
        m_style.topAttachment = 3u;
        m_style.bottomAttachment = 4u;
        m_style.textExtended = true;
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerMLeaderStyleObjectClass(&m_style));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeMLeaderStyle(&m_style));
    }

    void addMLeaderStyle(const DRW_MLeaderStyle *style) override {
        if (style != nullptr)
            m_styles.push_back(*style);
    }

    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        m_rawObjects.push_back(object);
    }
};

class ToleranceRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Tolerance> m_tolerances;

    void writeEntities() override {
        if (m_writer == nullptr)
            return;

        DRW_Tolerance tolerance;
        tolerance.color = 3;
        tolerance.insertionPoint = DRW_Coord{12.0, 34.0, 0.0};
        tolerance.xAxisDirectionVector = DRW_Coord{1.0, 0.25, 0.0};
        tolerance.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        tolerance.text = "{\\Fgdt;j}%%v0.05{\\Fgdt;m}A";
        REQUIRE(m_writer->writeTolerance(&tolerance));
    }

    void addTolerance(const DRW_Tolerance& tolerance) override {
        m_tolerances.push_back(tolerance);
    }
};

class HatchGradientRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Hatch> m_hatches;

    void writeEntities() override {
        if (m_writer == nullptr)
            return;

        DRW_Hatch hatch;
        hatch.color = 5;
        hatch.solid = 1;
        hatch.name = "SOLID";
        hatch.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        hatch.isGradient = 1;
        hatch.gradReserved = 0;
        hatch.gradAngle = 0.75;
        hatch.gradShift = 0.25;
        hatch.singleColor = 0;
        hatch.gradTint = 0.5;
        hatch.gradName = "LINEAR";
        DRW_Hatch::GradientStop first;
        first.value = 0.0;
        first.rgb = 0x00ff0000;
        first.aciColor = 1;
        hatch.gradColors.push_back(first);
        DRW_Hatch::GradientStop second;
        second.value = 1.0;
        second.rgb = 0x000000ff;
        second.aciColor = 5;
        hatch.gradColors.push_back(second);
        REQUIRE(m_writer->writeHatch(&hatch));
    }

    void addHatch(const DRW_Hatch *hatch) override {
        if (hatch != nullptr)
            m_hatches.push_back(*hatch);
    }
};

class MultilineAttributeWriteIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};

    static std::unique_ptr<DRW_MText> makeMText(const char *value) {
        auto mtext = std::make_unique<DRW_MText>();
        mtext->basePoint = DRW_Coord{1.0, 2.0, 0.0};
        mtext->extPoint = DRW_Coord{0.0, 0.0, 1.0};
        mtext->secPoint = DRW_Coord{1.0, 0.0, 0.0};
        mtext->widthscale = 20.0;
        mtext->height = 2.5;
        mtext->textgen = 1;
        mtext->alignH = DRW_Text::HLeft;
        mtext->interlin = 1.0;
        mtext->text = value;
        mtext->m_r2018IsNotAnnotative = true;
        mtext->m_r2018Version = 4;
        mtext->m_r2018DefaultFlag = true;
        mtext->m_r2018Attachment = DRW_MText::TopLeft;
        mtext->m_r2018XAxisDir = DRW_Coord{1.0, 0.0, 0.0};
        mtext->m_r2018InsertionPoint = mtext->basePoint;
        mtext->m_r2018RectWidth = 20.0;
        mtext->m_r2018RectHeight = 4.0;
        mtext->m_r2018ExtentsWidth = 20.0;
        mtext->m_r2018ExtentsHeight = 4.0;
        mtext->m_r2018AppIdHandle = 0x14u;
        return mtext;
    }

    void writeEntities() override {
        if (m_writer == nullptr)
            return;

        DRW_Attrib attrib;
        attrib.basePoint = DRW_Coord{4.0, 5.0, 0.0};
        attrib.secPoint = attrib.basePoint;
        attrib.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        attrib.height = 2.5;
        attrib.widthscale = 1.0;
        attrib.text = "line 1";
        attrib.tag = "ML_ATTRIB";
        attrib.attVersion = 1;
        attrib.m_attributeType = 2;
        attrib.mtext = makeMText("line 1\\Pline 2");
        REQUIRE(m_writer->writeAttrib(&attrib));

        DRW_Attdef attdef;
        attdef.basePoint = DRW_Coord{8.0, 9.0, 0.0};
        attdef.secPoint = attdef.basePoint;
        attdef.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        attdef.height = 2.5;
        attdef.widthscale = 1.0;
        attdef.text = "default";
        attdef.tag = "ML_ATTDEF";
        attdef.prompt = "Prompt";
        attdef.attVersion = 1;
        attdef.m_attributeType = 4;
        attdef.mtext = makeMText("default\\Pvalue");
        REQUIRE(m_writer->writeAttdef(&attdef));
    }
};

class LightRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Light> m_lights;

    void writeEntities() override {
        if (m_writer == nullptr)
            return;

        DRW_Light light;
        light.m_classVersion = 1;
        light.m_name = "Key light";
        light.m_type = 2;
        light.m_status = true;
        light.m_color = 3;
        light.m_plotGlyph = true;
        light.m_intensity = 4.5;
        light.m_position = DRW_Coord{1.0, 2.0, 3.0};
        light.m_target = DRW_Coord{4.0, 5.0, 6.0};
        light.m_attenuationType = 1;
        light.m_useAttenuationLimits = true;
        light.m_attenuationStartLimit = 7.0;
        light.m_attenuationEndLimit = 8.0;
        light.m_hotspotAngle = 0.25;
        light.m_falloffAngle = 0.5;
        light.m_castShadows = true;
        light.m_shadowType = 2;
        light.m_shadowMapSize = 256;
        light.m_shadowMapSoftness = 3;
        light.m_hasPhotometricData = true;
        light.m_hasWebFile = true;
        light.m_webFile = "lamp.ies";
        light.m_physicalIntensityMethod = 1;
        light.m_physicalIntensity = 9.0;
        light.m_illuminanceDistance = 10.0;
        light.m_lampColorType = 2;
        light.m_lampColorTemperature = 6500.0;
        light.m_lampColorPreset = 5;
        light.m_webRotation = DRW_Coord{0.0, 1.0, 0.0};
        light.m_extendedLightShape = 4;
        light.m_extendedLightLength = 11.0;
        light.m_extendedLightWidth = 12.0;
        light.m_extendedLightRadius = 13.0;
        REQUIRE(m_writer->writeLight(&light));
    }

    void addLight(const DRW_Light& light) override {
        m_lights.push_back(light);
    }
};

class SunRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_Sun m_sun;
    std::vector<DRW_Sun> m_suns;

    SunRoundTripIface() {
        m_sun.handle = 0x780u;
        m_sun.m_classVersion = 1u;
        m_sun.m_isOn = true;
        m_sun.m_color = 4u;
        m_sun.m_intensity = 2.75;
        m_sun.m_hasShadow = true;
        m_sun.m_julianDay = 2460001;
        m_sun.m_milliseconds = 43210000;
        m_sun.m_isDaylightSavings = true;
        m_sun.m_shadowType = 1u;
        m_sun.m_shadowMapSize = 512u;
        m_sun.m_shadowSoftness = 6u;
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerSunObjectClass(&m_sun));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeSun(&m_sun));
    }

    void addSun(const DRW_Sun& sun) override {
        m_suns.push_back(sun);
    }
};

class PlaceholderRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_AcDbPlaceholder m_placeholder;
    std::vector<DRW_AcDbPlaceholder> m_placeholders;

    PlaceholderRoundTripIface() {
        m_placeholder.handle = 0x790u;
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeAcDbPlaceholder(&m_placeholder));
    }

    void addAcDbPlaceholder(const DRW_AcDbPlaceholder& placeholder) override {
        m_placeholders.push_back(placeholder);
    }
};

// DICTIONARY round-trip — populates a tiny named-object dictionary with
// two entries, writes it via the native encoder, then reads back and
// asserts every field survived.  Exercises PR 8b's writeDictionary path
// plus the existing DRW_Dictionary::encodeDwg / parseDwg pair.
class DictionaryRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_Dictionary m_dictionary;
    std::vector<DRW_Dictionary> m_dictionaries;

    DictionaryRoundTripIface() {
        m_dictionary.handle = 0x7A0u;
        m_dictionary.parentHandle = 0xCu;
        m_dictionary.cloning = 1;        // KeepExisting
        m_dictionary.hardOwner = 0;
        DRW_Dictionary::Entry e1;
        e1.m_name = "ACAD_PLOTSETTINGS";
        e1.m_handle = 0x7A1u;
        m_dictionary.m_entries.push_back(e1);
        DRW_Dictionary::Entry e2;
        e2.m_name = "ACAD_GROUP";
        e2.m_handle = 0x7A2u;
        m_dictionary.m_entries.push_back(e2);
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeDictionary(&m_dictionary));
    }

    void addDictionary(const DRW_Dictionary& dictionary) override {
        m_dictionaries.push_back(dictionary);
    }
};

// XRECORD round-trip — populates an extended-data record with a mix of
// primitive variant types (string, int, double) plus a handle-stream
// entry, then asserts post-round-trip equivalence.  Exercises PR 8b's
// writeXRecord path and the existing DRW_XRecord byte-counted data
// section encoder.
class XRecordRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_XRecord m_xrecord;
    std::vector<DRW_XRecord> m_xrecords;

    XRecordRoundTripIface() {
        m_xrecord.handle = 0x7B0u;
        m_xrecord.parentHandle = 0xCu;
        m_xrecord.m_cloning = 1;
        m_xrecord.m_values.emplace_back(1, std::string("hello-xrecord"));
        m_xrecord.m_values.emplace_back(70, static_cast<dint32>(42));
        m_xrecord.m_values.emplace_back(40, 3.14159);
        // Handle-stream entry — code 0 means it lives in the handle stream
        // (not the data block).  Parser reads via getOffsetHandle.
        m_xrecord.m_handleValues.emplace_back(0, 0x7B1u);
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeXRecord(&m_xrecord));
    }

    void addXRecord(const DRW_XRecord& xrecord) override {
        m_xrecords.push_back(xrecord);
    }
};

// GROUP round-trip — populates a named entity-group with two entity
// handles, asserts every encoded field survives the writeGroup /
// parseDwg pair.  Exercises PR 8d.1's writeGroup path plus the existing
// DRW_Group::encodeDwg / parseDwg pair (ODA fixed type 72).
class GroupRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_Group m_group;
    std::vector<DRW_Group> m_groups;

    GroupRoundTripIface() {
        m_group.handle = 0x7D0u;
        m_group.parentHandle = 0xCu;
        m_group.m_description = "TestGroup";
        m_group.m_isUnnamed = false;
        m_group.m_selectable = true;
        m_group.m_entityHandles.push_back(0x7D1u);
        m_group.m_entityHandles.push_back(0x7D2u);
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeGroup(&m_group));
    }

    void addGroup(const DRW_Group& group) override {
        m_groups.push_back(group);
    }
};

// RASTERVARIABLES round-trip — populates a custom-class image-display
// settings object (classVersion + frame + quality + units), asserts every
// encoded field survives the writeRasterVariables / parseDwg pair.
// Exercises PR 8d.1b's class registration + dispatch path.
class RasterVariablesRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_RasterVariables m_rasterVariables;
    std::vector<DRW_RasterVariables> m_rasterVariablesObjects;

    RasterVariablesRoundTripIface() {
        m_rasterVariables.handle = 0x7E0u;
        m_rasterVariables.parentHandle = 0xCu;
        m_rasterVariables.m_classVersion = 0;
        m_rasterVariables.m_imageFrame = 1;
        m_rasterVariables.m_imageQuality = 1;
        m_rasterVariables.m_units = 2;
    }

    void writeDwgClasses() override {
        // CLASSES section is emitted before writeObjects(), so we must
        // register the custom class here (mirrors MLeaderStyleRoundTripIface).
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerRasterVariablesObjectClass(&m_rasterVariables));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeRasterVariables(&m_rasterVariables));
    }

    void addRasterVariables(const DRW_RasterVariables& rv) override {
        m_rasterVariablesObjects.push_back(rv);
    }
};

// SPATIAL_FILTER round-trip — populates a clipped-xref filter with a
// 4-point clip boundary, both clip planes, and identity transform
// matrices.  Exercises PR 8d.1d's class registration + dispatch.
class SpatialFilterRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_SpatialFilter m_filter;
    std::vector<DRW_SpatialFilter> m_filterObjects;

    SpatialFilterRoundTripIface() {
        m_filter.handle = 0x800u;
        m_filter.parentHandle = 0xCu;
        m_filter.m_boundaryPoints.emplace_back(0.0, 0.0, 0.0);
        m_filter.m_boundaryPoints.emplace_back(10.0, 0.0, 0.0);
        m_filter.m_boundaryPoints.emplace_back(10.0, 5.0, 0.0);
        m_filter.m_boundaryPoints.emplace_back(0.0, 5.0, 0.0);
        m_filter.m_normal = DRW_Coord(0.0, 0.0, 1.0);
        m_filter.m_origin = DRW_Coord(0.0, 0.0, 0.0);
        m_filter.m_displayBoundary = true;
        m_filter.m_clipFrontPlane = true;
        m_filter.m_frontDistance = 2.5;
        m_filter.m_clipBackPlane = true;
        m_filter.m_backDistance = -2.5;
        // Identity-ish 4x3 transforms (12 doubles each).
        m_filter.m_inverseInsertTransform = {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0
        };
        m_filter.m_insertTransform = {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0
        };
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerSpatialFilterObjectClass(&m_filter));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeSpatialFilter(&m_filter));
    }

    void addSpatialFilter(const DRW_SpatialFilter& filter) override {
        m_filterObjects.push_back(filter);
    }
};

// GEODATA round-trip — populates a v3 geolocation object with reference/
// design points, scale estimation, sea-level correction, observation tags,
// and a small mesh.  Exercises PR 8d.1c's class registration + dispatch.
class GeoDataRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_GeoData m_geoData;
    std::vector<DRW_GeoData> m_geoDataObjects;

    GeoDataRoundTripIface() {
        m_geoData.handle = 0x7F0u;
        m_geoData.parentHandle = 0xCu;
        m_geoData.m_hostBlockHandle = 0x1Fu;
        m_geoData.m_version = 3;
        m_geoData.m_coordinatesType = 2;
        m_geoData.m_designPoint = DRW_Coord(100.0, 200.0, 0.0);
        m_geoData.m_referencePoint = DRW_Coord(-122.5, 37.7, 10.0);
        m_geoData.m_horizontalUnitScale = 1.0;
        m_geoData.m_horizontalUnits = 6;     // meters
        m_geoData.m_verticalUnitScale = 1.0;
        m_geoData.m_verticalUnits = 6;
        m_geoData.m_upDirection = DRW_Coord(0.0, 0.0, 1.0);
        m_geoData.m_northDirection = DRW_Coord(0.0, 1.0, 0.0);
        m_geoData.m_scaleEstimationMethod = 1;
        m_geoData.m_userSpecifiedScaleFactor = 1.25;
        m_geoData.m_enableSeaLevelCorrection = true;
        m_geoData.m_seaLevelElevation = 12.5;
        m_geoData.m_coordinateProjectionRadius = 6378137.0;
        m_geoData.m_coordinateSystemDefinition = "EPSG:4326";
        m_geoData.m_geoRssTag = "test-geo-rss";
        m_geoData.m_observationFromTag = "from-tag";
        m_geoData.m_observationToTag = "to-tag";
        m_geoData.m_observationCoverageTag = "coverage-tag";
        DRW_GeoMeshPoint mp;
        mp.m_source = DRW_Coord(1.0, 2.0, 0.0);
        mp.m_destination = DRW_Coord(10.0, 20.0, 0.0);
        m_geoData.m_points.push_back(mp);
        mp.m_source = DRW_Coord(3.0, 4.0, 0.0);
        mp.m_destination = DRW_Coord(30.0, 40.0, 0.0);
        m_geoData.m_points.push_back(mp);
        DRW_GeoMeshFace mf;
        mf.m_index1 = 0;
        mf.m_index2 = 1;
        mf.m_index3 = 0;
        m_geoData.m_faces.push_back(mf);
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerGeoDataObjectClass(&m_geoData));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeGeoData(&m_geoData));
    }

    void addGeoData(const DRW_GeoData& gd) override {
        m_geoDataObjects.push_back(gd);
    }
};

// LAYOUT round-trip — populates a paper-space layout with PlotSettings
// prefix + Layout-specific fields, asserts every field survives the
// writeLayout / parseDwg pair.  Mirrors PR 8c.
class LayoutRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_Layout m_layout;
    std::vector<DRW_Layout> m_layouts;

    LayoutRoundTripIface() {
        m_layout.handle = 0x7C0u;
        m_layout.parentHandle = 0xCu;
        // PlotSettings prefix.
        m_layout.pageSetupName = "MyPageSetup";
        m_layout.printerConfig = "MyPrinter.pc3";
        m_layout.plotLayoutFlags = 0x44;
        m_layout.marginLeft = 7.5;
        m_layout.marginBottom = 20.0;
        m_layout.marginRight = 7.5;
        m_layout.marginTop = 20.0;
        m_layout.paperWidth = 297.0;
        m_layout.paperHeight = 210.0;
        m_layout.paperSize = "ISO_A4_(210.00_x_297.00_MM)";
        m_layout.plotOriginX = 0.0;
        m_layout.plotOriginY = 0.0;
        m_layout.paperUnits = 0;
        m_layout.plotRotation = 1;
        m_layout.plotType = 1;
        m_layout.windowMinX = 0.0;
        m_layout.windowMinY = 0.0;
        m_layout.windowMaxX = 12.0;
        m_layout.windowMaxY = 9.0;
        m_layout.realWorldUnits = 1.0;
        m_layout.drawingUnits = 1.0;
        m_layout.currentStyleSheet = "monochrome.ctb";
        m_layout.scaleType = 16;
        m_layout.scaleFactor = 1.0;
        m_layout.paperImageOriginX = 0.0;
        m_layout.paperImageOriginY = 0.0;
        m_layout.shadePlotMode = 0;
        m_layout.shadePlotResLevel = 2;
        m_layout.shadePlotCustomDPI = 300;
        // Layout-specific.
        m_layout.name = "Layout1";
        m_layout.layoutFlags = 0x01;
        m_layout.tabOrder = 1;
        m_layout.ucsOrigin = DRW_Coord{0.0, 0.0, 0.0};
        m_layout.limMinX = 0.0;
        m_layout.limMinY = 0.0;
        m_layout.limMaxX = 12.0;
        m_layout.limMaxY = 9.0;
        m_layout.insPoint = DRW_Coord{0.0, 0.0, 0.0};
        m_layout.ucsXAxis = DRW_Coord{1.0, 0.0, 0.0};
        m_layout.ucsYAxis = DRW_Coord{0.0, 1.0, 0.0};
        m_layout.elevation = 0.0;
        m_layout.orthoViewType = 0;
        m_layout.extMin = DRW_Coord{-100.0, -50.0, 0.0};
        m_layout.extMax = DRW_Coord{100.0, 50.0, 0.0};
        m_layout.viewportCount = 0;
        m_layout.paperSpaceBlockRecordHandle.ref = 0x7C1u;
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeLayout(&m_layout));
    }

    void addLayout(const DRW_Layout& layout) override {
        m_layouts.push_back(layout);
    }
};

// SCALE round-trip — populates an annotation-scale entry (1:50 paper-to-world)
// and asserts every field survives the writeScale / parseDwg pair.  Exercises
// PR 8d.2a's class registration + dispatch + the wrapper-emits-common-prefix
// wrinkle (SCALE encoder writes body only).
class ScaleRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_Scale m_scale;
    std::vector<DRW_Scale> m_scaleObjects;

    ScaleRoundTripIface() {
        m_scale.handle = 0x810u;
        m_scale.parentHandle = 0xCu;
        m_scale.name = "1:50";
        m_scale.flag = 0;
        m_scale.paperUnits = 1.0;
        m_scale.drawingUnits = 50.0;
        m_scale.isUnitScale = false;
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerScaleObjectClass(&m_scale));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeScale(&m_scale));
    }

    void addScale(const DRW_Scale& s) override {
        m_scaleObjects.push_back(s);
    }
};

// IDBUFFER round-trip — populates a list of object handles (used by selection
// filters and LAYER_INDEX).  Asserts every encoded field survives.
class IDBufferRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_IDBuffer m_idBuffer;
    std::vector<DRW_IDBuffer> m_idBufferObjects;

    IDBufferRoundTripIface() {
        m_idBuffer.handle = 0x820u;
        m_idBuffer.parentHandle = 0xCu;
        m_idBuffer.classVersion = 0;
        m_idBuffer.objIds = {0x1001u, 0x1002u, 0x1003u};
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerIDBufferObjectClass(&m_idBuffer));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeIDBuffer(&m_idBuffer));
    }

    void addIDBuffer(const DRW_IDBuffer& b) override {
        m_idBufferObjects.push_back(b);
    }
};

// LAYER_INDEX round-trip — populates a per-layer index with two layer entries,
// each pointing at an IDBUFFER handle.
class LayerIndexRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_LayerIndex m_layerIndex;
    std::vector<DRW_LayerIndex> m_layerIndexObjects;

    LayerIndexRoundTripIface() {
        m_layerIndex.handle = 0x830u;
        m_layerIndex.parentHandle = 0xCu;
        m_layerIndex.timestamp1 = 0x25A1F0u;
        m_layerIndex.timestamp2 = 0x4B0u;
        DRW_LayerIndexEntry e1;
        e1.indexLong = 1;
        e1.name = "0";
        e1.entryHandle = 0x1100u;
        m_layerIndex.entries.push_back(e1);
        DRW_LayerIndexEntry e2;
        e2.indexLong = 2;
        e2.name = "DETAILS";
        e2.entryHandle = 0x1101u;
        m_layerIndex.entries.push_back(e2);
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerLayerIndexObjectClass(&m_layerIndex));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeLayerIndex(&m_layerIndex));
    }

    void addLayerIndex(const DRW_LayerIndex& li) override {
        m_layerIndexObjects.push_back(li);
    }
};

// SPATIAL_INDEX round-trip — only timestamps are encoded (body beyond is
// opaque per ODA spec); the round-trip asserts the timestamps survive.
class SpatialIndexRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_SpatialIndex m_spatialIndex;
    std::vector<DRW_SpatialIndex> m_spatialIndexObjects;

    SpatialIndexRoundTripIface() {
        m_spatialIndex.handle = 0x840u;
        m_spatialIndex.parentHandle = 0xCu;
        m_spatialIndex.timestamp1 = 0x25A201u;
        m_spatialIndex.timestamp2 = 0x9C4u;
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerSpatialIndexObjectClass(&m_spatialIndex));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeSpatialIndex(&m_spatialIndex));
    }

    void addSpatialIndex(const DRW_SpatialIndex& si) override {
        m_spatialIndexObjects.push_back(si);
    }
};

// DICTIONARYVAR round-trip — populates a typed dictionary variable (schema +
// value pair stored under a named-object dictionary entry).
class DictionaryVarRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_DictionaryVar m_dictionaryVar;
    std::vector<DRW_DictionaryVar> m_dictionaryVarObjects;

    DictionaryVarRoundTripIface() {
        m_dictionaryVar.handle = 0x850u;
        m_dictionaryVar.parentHandle = 0xCu;
        m_dictionaryVar.m_schema = 0;
        m_dictionaryVar.m_value = "Standard";
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerDictionaryVarObjectClass(&m_dictionaryVar));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeDictionaryVar(&m_dictionaryVar));
    }

    void addDictionaryVar(const DRW_DictionaryVar& dv) override {
        m_dictionaryVarObjects.push_back(dv);
    }
};

// PR 8d.2b round-trip iface fixtures — four larger no-storage OBJECTS
// families.

// DICTIONARYWDFLT round-trip — a regular dictionary plus a single fallback
// handle returned when a lookup misses.  Exercises the encoder's delegation
// to DRW_Dictionary::encodeDwg + the tail default-handle write.
class DictionaryWithDefaultRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_DictionaryWithDefault m_dictionary;
    std::vector<DRW_DictionaryWithDefault> m_dictionaryObjects;

    DictionaryWithDefaultRoundTripIface() {
        m_dictionary.handle = 0x860u;
        m_dictionary.parentHandle = 0xCu;
        m_dictionary.cloning = 1;
        m_dictionary.hardOwner = 0;
        m_dictionary.name = "ACAD_DEFAULT_DICT";
        DRW_Dictionary::Entry e1;
        e1.m_name = "Entry1";
        e1.m_handle = 0x1200u;
        m_dictionary.m_entries.push_back(e1);
        DRW_Dictionary::Entry e2;
        e2.m_name = "Entry2";
        e2.m_handle = 0x1201u;
        m_dictionary.m_entries.push_back(e2);
        m_dictionary.m_defaultEntryHandle = 0x1202u;
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerDictionaryWithDefaultObjectClass(&m_dictionary));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeDictionaryWithDefault(&m_dictionary));
    }

    void addDictionaryWithDefault(const DRW_DictionaryWithDefault& d) override {
        m_dictionaryObjects.push_back(d);
    }
};

// SORTENTSTABLE round-trip — per-block draw-order override.  Exercises the
// inline-handle wrinkle: sort handles go in the body section BEFORE the
// common prefix; block-owner + entity handles follow in the handle stream.
class SortEntsTableRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_SortEntsTable m_sortEntsTable;
    std::vector<DRW_SortEntsTable> m_sortEntsTableObjects;

    SortEntsTableRoundTripIface() {
        m_sortEntsTable.handle = 0x870u;
        m_sortEntsTable.parentHandle = 0xCu;
        m_sortEntsTable.m_sortHandles = {0x2001u, 0x2002u, 0x2003u};
        m_sortEntsTable.m_blockOwnerHandle = 0x2100u;
        m_sortEntsTable.m_entityHandles = {0x2010u, 0x2011u, 0x2012u};
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerSortEntsTableObjectClass(&m_sortEntsTable));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeSortEntsTable(&m_sortEntsTable));
    }

    void addSortEntsTable(const DRW_SortEntsTable& s) override {
        m_sortEntsTableObjects.push_back(s);
    }
};

// FIELDLIST round-trip — list of FIELD handles plus the "unknown" bit
// captured by the parser.
class FieldListRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_FieldList m_fieldList;
    std::vector<DRW_FieldList> m_fieldListObjects;

    FieldListRoundTripIface() {
        m_fieldList.handle = 0x880u;
        m_fieldList.parentHandle = 0xCu;
        m_fieldList.m_unknown = 1;
        m_fieldList.m_fieldHandles = {0x2200u, 0x2201u};
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerFieldListObjectClass(&m_fieldList));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeFieldList(&m_fieldList));
    }

    void addFieldList(const DRW_FieldList& f) override {
        m_fieldListObjects.push_back(f);
    }
};

// FIELD round-trip — populates a typed CadValue (double via dataType==2),
// evaluator + code + format + messages, child handles, object handles.
// Exercises the writeCadValue helper.
class FieldRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    DRW_Field m_field;
    std::vector<DRW_Field> m_fieldObjects;

    FieldRoundTripIface() {
        m_field.handle = 0x890u;
        m_field.parentHandle = 0xCu;
        m_field.m_evaluatorId = "AcDbBlockEval";
        m_field.m_fieldCode = "%<\\AcDbBlock>%";
        m_field.m_formatString = "%lu2";
        m_field.m_evaluationOptionFlags = 1;
        m_field.m_filingOptionFlags = 0;
        m_field.m_fieldStateFlags = 2;
        m_field.m_evaluationStatusFlags = 1;
        m_field.m_evaluationErrorCode = 0;
        m_field.m_evaluationErrorMessage = "";
        // formatFlags & 3 = 0 keeps the value body present at R2007+ (the
        // parser skips the body when formatFlags & 3 != 0, treating it as
        // an "empty" value sentinel).
        m_field.m_value.m_formatFlags = 0;
        m_field.m_value.m_dataType = 2;        // double
        m_field.m_value.m_value = DRW_Variant(40, 12.5);
        m_field.m_value.m_formatString = "%lu2";
        m_field.m_value.m_valueString = "12.5";
        m_field.m_valueString = "12.5";
        m_field.m_valueStringLength = 4;
        m_field.m_childHandles = {0x2301u};
        m_field.m_objectHandles = {0x2401u};
    }

    void writeDwgClasses() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->registerFieldObjectClass(&m_field));
    }

    void writeObjects() override {
        if (m_writer != nullptr)
            REQUIRE(m_writer->writeField(&m_field));
    }

    void addField(const DRW_Field& f) override {
        m_fieldObjects.push_back(f);
    }
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

TEST_CASE("dwgRW replays raw custom OBJECT payloads with class metadata",
          "[dwg-write][raw-replay]") {
    const DRW::Version versions[] = {DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("raw_replay_object.dwg");
        RawObjectReplayIface writeIface(version);
        {
            dwgRW writer(path.c_str());
            writeIface.m_writer = &writer;
            REQUIRE(writer.write(&writeIface, version, /*bin=*/false));
        }

        RawObjectReplayIface readIface(version);
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_readHandseed > writeIface.m_rawObject.m_handle);
        const DRW_UnsupportedObject *raw = nullptr;
        for (const DRW_UnsupportedObject& object : readIface.m_unsupportedObjects) {
            if (object.m_handle == writeIface.m_rawObject.m_handle
                && object.m_objectType == writeIface.m_rawObject.m_objectType) {
                raw = &object;
                break;
            }
        }
        REQUIRE(raw != nullptr);
        REQUIRE(raw->m_bodyBitSize == writeIface.m_rawObject.m_bodyBitSize);
        REQUIRE(raw->m_isCustomClass);
        REQUIRE_FALSE(raw->m_isEntity);
        REQUIRE(raw->m_recordName == writeIface.m_rawObject.m_recordName);
        REQUIRE(raw->m_className == writeIface.m_rawObject.m_className);
        REQUIRE(raw->m_rawBytes == writeIface.m_rawObject.m_rawBytes);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW rejects conflicting custom class registrations",
          "[dwg-write][raw-replay][classes]") {
    const DRW::Version versions[] = {DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("raw_replay_class_conflict.dwg");
        ConflictingRawClassIface iface(version);
        dwgRW writer(path.c_str());
        iface.m_writer = &writer;
        REQUIRE_FALSE(writer.write(&iface, version, /*bin=*/false));
        std::remove(path.c_str());
    }
}

TEST_CASE("dwgWriter counts unique raw custom class instances",
          "[dwg-write][raw-replay][classes]") {
    DRW_Header header;
    std::ofstream stream;
    InspectableDwgWriter15 writer(&stream, &header);

    DRW_UnsupportedObject first = makeRawReplayObject(DRW::AC1024);
    DRW_UnsupportedObject second = first;
    second.m_handle = 0x701u;

    REQUIRE(writer.registerRawObjectClass(first));
    REQUIRE(writer.registerRawObjectClass(first));
    REQUIRE(writer.registerRawObjectClass(second));
    CHECK(writer.customClassInstanceCount(first.m_objectType) == 2);
}

// Phase 2b.5 — sweep verification: every Phase-2b rescued custom-class raw
// object registers a CLASSES entry (its recName) when written. This proves
// registerRawObjectClass fires for each rescued type, so the CLASSES section
// is never an orphan/missing entry for a replayed handle.
TEST_CASE("dwgWriter registers a CLASSES entry for every Phase-2b rescued type",
          "[dwg-write][raw-replay][classes][replay_rescue]") {
    const struct {
        duint16 classNum;
        const char* recName;
        const char* className;
    } kRescued[] = {
        {600, "MATERIAL", "AcDbMaterial"},
        {601, "ACDB_VISUALSTYLE_CLASS", "AcDbVisualStyle"},
        {602, "TABLESTYLE", "AcDbTableStyle"},
        {603, "CELLSTYLEMAP", "AcDbCellStyleMap"},
        {604, "TABLECONTENT", "AcDbTableContent"},
        {605, "ACDBDETAILVIEWSTYLE", "AcDbDetailViewStyle"},
        {606, "ACDBSECTIONVIEWSTYLE", "AcDbSectionViewStyle"},
        {607, "IMAGEDEF_REACTOR", "AcDbRasterImageDefReactor"},
        {608, "TABLEGEOMETRY", "AcDbTableGeometry"},
        {609, "ACAD_EVALUATION_GRAPH", "AcDbEvalGraph"},
    };

    DRW_Header header;
    std::ofstream stream;
    InspectableDwgWriter15 writer(&stream, &header);

    duint32 handle = 0x900u;
    for (const auto& r : kRescued) {
        DRW_UnsupportedObject obj = makeRawReplayObject(DRW::AC1024);
        obj.m_objectType = r.classNum;
        obj.m_handle = handle++;
        obj.m_recordName = r.recName;
        obj.m_className = r.className;
        REQUIRE(writer.registerObjectClassForTest(obj));
        // Registered with a CLASSES entry (instanceCount>=1) and tagged as an
        // OBJECT (item_class_id 0x1F3, not the entity 0x1F2).
        CHECK(writer.customClassInstanceCount(r.classNum) == 1);
        CHECK(writer.customClassItemClassId(r.classNum) == 0x1F3);
    }
}

// 0B.3 (gap classes-itemclassid): object-producing CLASSES entries must
// carry item_class_id 0x1F3; entity-producing entries 0x1F2.  Writer-only
// change; the reader maps 0x1F2->entity and everything else->object, so the
// self-round-trip classification is unchanged.
TEST_CASE("dwgWriter emits 0x1F3 item_class_id for object classes, 0x1F2 for entities",
          "[dwg-write][classes]") {
    DRW_Header header;
    std::ofstream stream;
    InspectableDwgWriter15 writer(&stream, &header);

    // Raw OBJECT custom class → 0x1F3 (was 0).
    DRW_UnsupportedObject obj = makeRawReplayObject(DRW::AC1024);
    REQUIRE(writer.registerObjectClassForTest(obj));
    CHECK(writer.customClassItemClassId(obj.m_objectType) == 0x1F3);

    // Raw ENTITY custom class → 0x1F2.
    DRW_UnsupportedObject ent = makeRawReplayObject(DRW::AC1024);
    ent.m_objectType = 510;
    ent.m_isEntity = true;
    REQUIRE(writer.registerObjectClassForTest(ent));
    CHECK(writer.customClassItemClassId(ent.m_objectType) == 0x1F2);

    // A named OBJECT helper (SUN) → 0x1F3 (was 0).
    REQUIRE(writer.registerSunObjectClass(0x900u));
    CHECK(writer.customClassItemClassId(DRW_Sun::kDwgClassNum) == 0x1F3);

    // Reader tolerance: the built-in entity registrations (ARC_DIMENSION 500,
    // MULTILEADER 501, LIGHT 502) keep 0x1F2.
    CHECK(writer.customClassItemClassId(500) == 0x1F2);
}

TEST_CASE("dwgRW writes native text MLEADER entities",
          "[dwg-write][mleader]") {
    const DRW::Version versions[] = {DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_mleader.dwg");
        {
            dwgRW writer(path.c_str());
            MLeaderRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        MLeaderRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_mleaders.size() == 1);
        const DRW_MLeader& leader = readIface.m_mleaders.front();
        REQUIRE(leader.classVersion == 2);
        REQUIRE(leader.context.hasTextContents);
        REQUIRE(leader.context.textLabel == "Native MLeader");
        REQUIRE(leader.context.roots.size() == 1);
        REQUIRE(leader.context.roots[0].leaderLines.size() == 1);
        REQUIRE(leader.context.roots[0].leaderLines[0].points.size() == 2);
        REQUIRE(leader.context.roots[0].leaderLines[0].points[1].x == 10.0);
        REQUIRE(leader.context.roots[0].leaderLines[0].lineTypeHandle.ref == 0x14u);
        REQUIRE(leader.context.roots[0].leaderLines[0].arrowHandle.ref == 0x15u);
        REQUIRE(leader.context.textLocation.x == 20.0);
        REQUIRE(leader.context.textStyleHandle.ref == 0x13u);
        REQUIRE(leader.styleHandle.ref == 0x830u);
        REQUIRE(leader.leaderLineTypeHandle.ref == 0x14u);
        REQUIRE(leader.arrowHeadHandle.ref == 0x15u);
        REQUIRE(leader.styleTextStyleHandle.ref == 0x13u);
        REQUIRE(leader.styleContentType == 2);
        REQUIRE(leader.defaultArrowHeadSize == 0.75);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW writes and reads MLEADERSTYLE metadata",
          "[dwg-write][mleaderstyle]") {
    const DRW::Version versions[] = {DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_mleaderstyle.dwg");
        MLeaderStyleRoundTripIface sourceIface;
        {
            dwgRW writer(path.c_str());
            sourceIface.m_writer = &writer;
            REQUIRE(writer.write(&sourceIface, version, /*bin=*/false));
        }

        MLeaderStyleRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_styles.size() == 1);
        const DRW_MLeaderStyle& style = readIface.m_styles.front();
        CHECK(style.handle == 0x830u);
        CHECK(style.styleVersion == 2u);
        CHECK(style.contentType == 2u);
        CHECK(style.drawMLeaderOrder == 1u);
        CHECK(style.maxLeaderPoints == 7);
        CHECK(style.leaderColor == 3);
        CHECK(style.leaderLineTypeHandle.ref == 0x14u);
        CHECK(style.leaderLineWeight == 29);
        CHECK(style.landingEnabled);
        CHECK(style.landingDistance == 2.25);
        CHECK(style.description == "Round-trip MLeader style");
        CHECK(style.arrowHeadBlockHandle.ref == 0x15u);
        CHECK(style.arrowHeadSize == 0.75);
        CHECK(style.textDefault == "Default leader text");
        CHECK(style.textStyleHandle.ref == 0x13u);
        CHECK(style.textColor == 5);
        CHECK(style.textHeight == 2.5);
        CHECK(style.textFrameEnabled);
        CHECK(style.blockHandle.ref == 0x17u);
        CHECK(style.blockColor == 6);
        CHECK(style.blockScale.x == 1.0);
        CHECK(style.blockScale.y == 2.0);
        CHECK(style.blockScale.z == 3.0);
        CHECK(style.scaleFactor == 1.5);
        CHECK(style.isAnnotative);
        CHECK(style.breakSize == 0.375);
        CHECK(style.attachmentDirection == 1u);
        CHECK(style.topAttachment == 3u);
        CHECK(style.bottomAttachment == 4u);
        CHECK(style.textExtended == (version >= DRW::AC1027));

        bool foundRawStyle = false;
        for (const DRW_UnsupportedObject& object : readIface.m_rawObjects) {
            if (object.m_recordName == "MLEADERSTYLE" && object.m_handle == 0x830u) {
                foundRawStyle = true;
                break;
            }
        }
        CHECK(foundRawStyle);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW writes and reads TOLERANCE entities",
          "[dwg-write][tolerance]") {
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1024, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_tolerance.dwg");
        {
            dwgRW writer(path.c_str());
            ToleranceRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        ToleranceRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_tolerances.size() == 1);
        const DRW_Tolerance& tolerance = readIface.m_tolerances.front();
        CHECK(tolerance.insertionPoint.x == 12.0);
        CHECK(tolerance.insertionPoint.y == 34.0);
        CHECK(tolerance.xAxisDirectionVector.x == 1.0);
        CHECK(tolerance.xAxisDirectionVector.y == 0.25);
        CHECK(tolerance.extPoint.z == 1.0);
        CHECK(tolerance.text == "{\\Fgdt;j}%%v0.05{\\Fgdt;m}A");
        CHECK(tolerance.dimStyleH.ref == 0x15);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW preserves HATCH gradient fields",
          "[dwg-write][hatch][gradient]") {
    const DRW::Version versions[] = {DRW::AC1018, DRW::AC1024, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("gradient_hatch.dwg");
        {
            dwgRW writer(path.c_str());
            HatchGradientRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        HatchGradientRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_hatches.size() == 1);
        const DRW_Hatch& hatch = readIface.m_hatches.front();
        CHECK(hatch.isGradient == 1);
        CHECK(hatch.gradAngle == 0.75);
        CHECK(hatch.gradShift == 0.25);
        CHECK(hatch.singleColor == 0);
        CHECK(hatch.gradTint == 0.5);
        CHECK(hatch.gradName == "LINEAR");
        REQUIRE(hatch.gradColors.size() == 2);
        CHECK(hatch.gradColors[0].value == 0.0);
        CHECK(hatch.gradColors[0].rgb == 0x00ff0000);
        CHECK(hatch.gradColors[1].value == 1.0);
        CHECK(hatch.gradColors[1].rgb == 0x000000ff);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW writes AC1032 multiline ATTRIB and ATTDEF payloads",
          "[dwg-write][attrib][ac1032]") {
    const std::string path = tempPath("multiline_attrib_ac1032.dwg");
    {
        dwgRW writer(path.c_str());
        MultilineAttributeWriteIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1032, /*bin=*/false));
    }
    {
        dwgRW reader(path.c_str());
        EmptyIface iface;
        REQUIRE(reader.read(&iface, /*ext=*/false));
        REQUIRE(reader.getVersion() == DRW::AC1032);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }
    std::remove(path.c_str());
}

TEST_CASE("dwgRW writes and reads LIGHT metadata",
          "[dwg-write][light]") {
    const DRW::Version versions[] = {DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_light.dwg");
        {
            dwgRW writer(path.c_str());
            LightRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        LightRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_lights.size() == 1);
        const DRW_Light& light = readIface.m_lights.front();
        CHECK(light.m_name == "Key light");
        CHECK(light.m_type == 2);
        CHECK(light.m_status);
        CHECK(light.m_color == 3);
        CHECK(light.m_intensity == 4.5);
        CHECK(light.m_position.x == 1.0);
        CHECK(light.m_target.z == 6.0);
        CHECK(light.m_hasPhotometricData);
        CHECK(light.m_hasWebFile);
        CHECK(light.m_webFile == "lamp.ies");
        CHECK(light.m_lampColorTemperature == 6500.0);
        CHECK(light.m_extendedLightRadius == 13.0);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW writes and reads SUN metadata",
          "[dwg-write][sun]") {
    const DRW::Version versions[] = {DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_sun.dwg");
        {
            dwgRW writer(path.c_str());
            SunRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        SunRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_suns.size() == 1);
        const DRW_Sun& sun = readIface.m_suns.front();
        CHECK(sun.m_classVersion == 1u);
        CHECK(sun.m_isOn);
        CHECK(sun.m_color == 4u);
        CHECK(sun.m_intensity == 2.75);
        CHECK(sun.m_hasShadow);
        CHECK(sun.m_julianDay == 2460001);
        CHECK(sun.m_milliseconds == 43210000);
        CHECK(sun.m_isDaylightSavings);
        CHECK(sun.m_shadowType == 1u);
        CHECK(sun.m_shadowMapSize == 512u);
        CHECK(sun.m_shadowSoftness == 6u);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW writes and reads ACDBPLACEHOLDER metadata",
          "[dwg-write][placeholder]") {
    // PR 13d — ACDBPLACEHOLDER (ODA fixed type 80) is universally
    // available since R2000 and the encoder is version-clean (no
    // AC1018+-only fields; only the standard string/handle split-buffer
    // routing on `version > AC1018`).  Extended the smoke array to also
    // cover AC1015/AC1018 in step with the gate broadening.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_placeholder.dwg");
        {
            dwgRW writer(path.c_str());
            PlaceholderRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        PlaceholderRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_placeholders.size() == 1);
        CHECK(readIface.m_placeholders.front().handle == 0x790u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads DICTIONARY metadata",
          "[dwg-write][dictionary]") {
    // PR 13a — DICTIONARY encoder is version-clean (string-buffer routing on
    // version > AC1018; common handle prefix unconditional per PR 2).  Pilot
    // AC1015/AC1018 smoke coverage so the filter-side gate can later drop
    // for the ≥ AC1015 path.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_dictionary.dwg");
        {
            dwgRW writer(path.c_str());
            DictionaryRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        DictionaryRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        // Find the written DICTIONARY (handle 0x7A0) — the reader also
        // surfaces the synthetic root dictionary at handle 0xC, so filter.
        const DRW_Dictionary* found = nullptr;
        for (const DRW_Dictionary& d : readIface.m_dictionaries) {
            if (d.handle == 0x7A0u) {
                found = &d;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->cloning == 1);
        CHECK(found->hardOwner == 0);
        REQUIRE(found->m_entries.size() == 2);
        CHECK(found->m_entries[0].m_name == "ACAD_PLOTSETTINGS");
        CHECK(found->m_entries[0].m_handle == 0x7A1u);
        CHECK(found->m_entries[1].m_name == "ACAD_GROUP");
        CHECK(found->m_entries[1].m_handle == 0x7A2u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads XRECORD metadata",
          "[dwg-write][xrecord]") {
    // PR 13a — XRECORD encoder uses the byte-counted-data section pattern
    // with the AC1015 xDictFlag-defaulting quirk codified in the encoder
    // (see Patterns: AC1015 xDictFlag gotcha).  Pilot AC1015/AC1018 smoke
    // coverage so the filter-side gate can later drop for the ≥ AC1015 path.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_xrecord.dwg");
        {
            dwgRW writer(path.c_str());
            XRecordRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        XRecordRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_xrecords.size() == 1);
        const DRW_XRecord& xr = readIface.m_xrecords.front();
        CHECK(xr.handle == 0x7B0u);
        CHECK(xr.m_cloning == 1);
        REQUIRE(xr.m_values.size() == 3);
        REQUIRE(xr.m_values[0].type() == DRW_Variant::STRING);
        CHECK(xr.m_values[0].code() == 1);
        CHECK(std::string(xr.m_values[0].c_str()) == "hello-xrecord");
        REQUIRE(xr.m_values[1].type() == DRW_Variant::INTEGER);
        CHECK(xr.m_values[1].code() == 70);
        CHECK(xr.m_values[1].i_val() == 42);
        REQUIRE(xr.m_values[2].type() == DRW_Variant::DOUBLE);
        CHECK(xr.m_values[2].code() == 40);
        CHECK(xr.m_values[2].d_val() == 3.14159);
        // Handle-stream entry survives — parser pushes back (0, ref).
        bool foundHandleStreamRef = false;
        for (const auto& hv : xr.m_handleValues) {
            if (hv.first == 0 && hv.second == 0x7B1u) {
                foundHandleStreamRef = true;
                break;
            }
        }
        CHECK(foundHandleStreamRef);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads LAYOUT metadata",
          "[dwg-write][layout]") {
    // PR 13c — LAYOUT encoder handles AC1015 cleanly (the AC1015-only
    // plotViewName branch, the AC1018+-only shadePlot* + viewportCount
    // raw32 + plotViewHandle branches).  Extend smoke to AC1015/AC1018 so
    // the filter-side gate can drop for fixed type 82 alongside
    // DICTIONARY/XRECORD/GROUP.  The shadePlot* CHECKs are conditional
    // because the encoder gates them on AC1018+.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_layout.dwg");
        {
            dwgRW writer(path.c_str());
            LayoutRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        LayoutRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_Layout* found = nullptr;
        for (const DRW_Layout& l : readIface.m_layouts) {
            if (l.handle == 0x7C0u) {
                found = &l;
                break;
            }
        }
        REQUIRE(found != nullptr);
        // PlotSettings prefix.
        CHECK(found->pageSetupName == "MyPageSetup");
        CHECK(found->printerConfig == "MyPrinter.pc3");
        CHECK(found->plotLayoutFlags == 0x44);
        CHECK(found->marginLeft == 7.5);
        CHECK(found->marginBottom == 20.0);
        CHECK(found->marginRight == 7.5);
        CHECK(found->marginTop == 20.0);
        CHECK(found->paperWidth == 297.0);
        CHECK(found->paperHeight == 210.0);
        CHECK(found->paperSize == "ISO_A4_(210.00_x_297.00_MM)");
        CHECK(found->paperUnits == 0);
        CHECK(found->plotRotation == 1);
        CHECK(found->plotType == 1);
        CHECK(found->windowMaxX == 12.0);
        CHECK(found->windowMaxY == 9.0);
        CHECK(found->currentStyleSheet == "monochrome.ctb");
        CHECK(found->scaleType == 16);
        CHECK(found->scaleFactor == 1.0);
        if (version >= DRW::AC1018) {
            // shadePlot* fields are R2004+; PR 13c smoke covers AC1015 too,
            // where the encoder skips these and the parser leaves defaults.
            CHECK(found->shadePlotResLevel == 2);
            CHECK(found->shadePlotCustomDPI == 300);
        }
        // Layout-specific.
        CHECK(found->name == "Layout1");
        CHECK(found->layoutFlags == 0x01);
        CHECK(found->tabOrder == 1);
        CHECK(found->limMaxX == 12.0);
        CHECK(found->limMaxY == 9.0);
        CHECK(found->extMin.x == -100.0);
        CHECK(found->extMax.x == 100.0);
        CHECK(found->viewportCount == 0);
        CHECK(found->paperSpaceBlockRecordHandle.ref == 0x7C1u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads GROUP metadata",
          "[dwg-write][group]") {
    // PR 13b — GROUP encoder has no version-gated fields; string-buffer
    // routing on version > AC1018 is the only branch.  Extend smoke
    // coverage to the AC1015/AC1018 range so the filter-side gate can
    // drop for fixed type 72 alongside DICTIONARY/XRECORD.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_group.dwg");
        {
            dwgRW writer(path.c_str());
            GroupRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        GroupRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_Group* found = nullptr;
        for (const DRW_Group& g : readIface.m_groups) {
            if (g.handle == 0x7D0u) {
                found = &g;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->m_description == "TestGroup");
        CHECK(found->m_isUnnamed == false);
        CHECK(found->m_selectable == true);
        REQUIRE(found->m_entityHandles.size() == 2);
        CHECK(found->m_entityHandles[0] == 0x7D1u);
        CHECK(found->m_entityHandles[1] == 0x7D2u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads RASTERVARIABLES metadata",
          "[dwg-write][rastervariables]") {
    // PR 13f — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_rastervariables.dwg");
        {
            dwgRW writer(path.c_str());
            RasterVariablesRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        RasterVariablesRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_RasterVariables* found = nullptr;
        for (const DRW_RasterVariables& rv : readIface.m_rasterVariablesObjects) {
            if (rv.handle == 0x7E0u) {
                found = &rv;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->m_classVersion == 0);
        CHECK(found->m_imageFrame == 1);
        CHECK(found->m_imageQuality == 1);
        CHECK(found->m_units == 2);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads SPATIAL_FILTER metadata",
          "[dwg-write][spatial-filter]") {
    // PR 13f — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_spatial_filter.dwg");
        {
            dwgRW writer(path.c_str());
            SpatialFilterRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        SpatialFilterRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_SpatialFilter* found = nullptr;
        for (const DRW_SpatialFilter& sf : readIface.m_filterObjects) {
            if (sf.handle == 0x800u) {
                found = &sf;
                break;
            }
        }
        REQUIRE(found != nullptr);
        REQUIRE(found->m_boundaryPoints.size() == 4);
        CHECK(found->m_boundaryPoints[0].x == Catch::Approx(0.0));
        CHECK(found->m_boundaryPoints[0].y == Catch::Approx(0.0));
        CHECK(found->m_boundaryPoints[1].x == Catch::Approx(10.0));
        CHECK(found->m_boundaryPoints[2].y == Catch::Approx(5.0));
        CHECK(found->m_boundaryPoints[3].x == Catch::Approx(0.0));
        CHECK(found->m_normal.z == Catch::Approx(1.0));
        CHECK(found->m_displayBoundary);
        CHECK(found->m_clipFrontPlane);
        CHECK(found->m_frontDistance == Catch::Approx(2.5));
        CHECK(found->m_clipBackPlane);
        CHECK(found->m_backDistance == Catch::Approx(-2.5));
        REQUIRE(found->m_inverseInsertTransform.size() == 12);
        REQUIRE(found->m_insertTransform.size() == 12);
        CHECK(found->m_inverseInsertTransform[0] == Catch::Approx(1.0));
        CHECK(found->m_inverseInsertTransform[5] == Catch::Approx(1.0));
        CHECK(found->m_inverseInsertTransform[10] == Catch::Approx(1.0));
        CHECK(found->m_insertTransform[0] == Catch::Approx(1.0));
        CHECK(found->m_insertTransform[5] == Catch::Approx(1.0));
        CHECK(found->m_insertTransform[10] == Catch::Approx(1.0));

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads GEODATA metadata",
          "[dwg-write][geodata]") {
    // PR 13f — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_geodata.dwg");
        {
            dwgRW writer(path.c_str());
            GeoDataRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        GeoDataRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_GeoData* found = nullptr;
        for (const DRW_GeoData& gd : readIface.m_geoDataObjects) {
            if (gd.handle == 0x7F0u) {
                found = &gd;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->m_version == 3);
        CHECK(found->m_coordinatesType == 2);
        CHECK(found->m_hostBlockHandle == 0x1Fu);
        CHECK(found->m_designPoint.x == Catch::Approx(100.0));
        CHECK(found->m_designPoint.y == Catch::Approx(200.0));
        CHECK(found->m_referencePoint.x == Catch::Approx(-122.5));
        CHECK(found->m_referencePoint.y == Catch::Approx(37.7));
        CHECK(found->m_referencePoint.z == Catch::Approx(10.0));
        CHECK(found->m_horizontalUnits == 6);
        CHECK(found->m_verticalUnits == 6);
        CHECK(found->m_horizontalUnitScale == Catch::Approx(1.0));
        CHECK(found->m_verticalUnitScale == Catch::Approx(1.0));
        CHECK(found->m_scaleEstimationMethod == 1);
        CHECK(found->m_userSpecifiedScaleFactor == Catch::Approx(1.25));
        CHECK(found->m_enableSeaLevelCorrection);
        CHECK(found->m_seaLevelElevation == Catch::Approx(12.5));
        CHECK(found->m_coordinateProjectionRadius == Catch::Approx(6378137.0));
        CHECK(found->m_coordinateSystemDefinition == "EPSG:4326");
        CHECK(found->m_geoRssTag == "test-geo-rss");
        CHECK(found->m_observationFromTag == "from-tag");
        CHECK(found->m_observationToTag == "to-tag");
        CHECK(found->m_observationCoverageTag == "coverage-tag");
        REQUIRE(found->m_points.size() == 2);
        CHECK(found->m_points[0].m_source.x == Catch::Approx(1.0));
        CHECK(found->m_points[0].m_destination.x == Catch::Approx(10.0));
        CHECK(found->m_points[1].m_source.x == Catch::Approx(3.0));
        CHECK(found->m_points[1].m_destination.x == Catch::Approx(30.0));
        REQUIRE(found->m_faces.size() == 1);
        CHECK(found->m_faces[0].m_index1 == 0);
        CHECK(found->m_faces[0].m_index2 == 1);
        CHECK(found->m_faces[0].m_index3 == 0);

        std::remove(path.c_str());
    }
}

// PR 8d.2a smoke tests — five small no-storage OBJECTS families.

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads SCALE metadata",
          "[dwg-write][scale]") {
    // PR 13g — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_scale.dwg");
        {
            dwgRW writer(path.c_str());
            ScaleRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        ScaleRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_Scale* found = nullptr;
        for (const DRW_Scale& s : readIface.m_scaleObjects) {
            if (s.handle == 0x810u) {
                found = &s;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->name == "1:50");
        CHECK(found->flag == 0);
        CHECK(found->paperUnits == Catch::Approx(1.0));
        CHECK(found->drawingUnits == Catch::Approx(50.0));
        CHECK(found->isUnitScale == false);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads IDBUFFER metadata",
          "[dwg-write][idbuffer]") {
    // PR 13g — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_idbuffer.dwg");
        {
            dwgRW writer(path.c_str());
            IDBufferRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        IDBufferRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_IDBuffer* found = nullptr;
        for (const DRW_IDBuffer& b : readIface.m_idBufferObjects) {
            if (b.handle == 0x820u) {
                found = &b;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->classVersion == 0);
        REQUIRE(found->objIds.size() == 3);
        CHECK(found->objIds[0] == 0x1001u);
        CHECK(found->objIds[1] == 0x1002u);
        CHECK(found->objIds[2] == 0x1003u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads LAYER_INDEX metadata",
          "[dwg-write][layer-index]") {
    // PR 13g — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_layer_index.dwg");
        {
            dwgRW writer(path.c_str());
            LayerIndexRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        LayerIndexRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_LayerIndex* found = nullptr;
        for (const DRW_LayerIndex& li : readIface.m_layerIndexObjects) {
            if (li.handle == 0x830u) {
                found = &li;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->timestamp1 == 0x25A1F0u);
        CHECK(found->timestamp2 == 0x4B0u);
        REQUIRE(found->entries.size() == 2);
        CHECK(found->entries[0].indexLong == 1);
        CHECK(found->entries[0].name == "0");
        CHECK(found->entries[0].entryHandle == 0x1100u);
        CHECK(found->entries[1].indexLong == 2);
        CHECK(found->entries[1].name == "DETAILS");
        CHECK(found->entries[1].entryHandle == 0x1101u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads SPATIAL_INDEX metadata",
          "[dwg-write][spatial-index]") {
    // PR 13g — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.  SPATIAL_INDEX's encoder gates the
    // common-handle prefix on `version > AC1018`, so AC1015/AC1018 emit
    // an opaque body (no handle tail) — mirrors the parser's pre-R2007
    // behaviour.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_spatial_index.dwg");
        {
            dwgRW writer(path.c_str());
            SpatialIndexRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        SpatialIndexRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_SpatialIndex* found = nullptr;
        for (const DRW_SpatialIndex& si : readIface.m_spatialIndexObjects) {
            if (si.handle == 0x840u) {
                found = &si;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->timestamp1 == 0x25A201u);
        CHECK(found->timestamp2 == 0x9C4u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads DICTIONARYVAR metadata",
          "[dwg-write][dictionary-var]") {
    // PR 13g — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_dictionary_var.dwg");
        {
            dwgRW writer(path.c_str());
            DictionaryVarRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        DictionaryVarRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_DictionaryVar* found = nullptr;
        for (const DRW_DictionaryVar& dv : readIface.m_dictionaryVarObjects) {
            if (dv.handle == 0x850u) {
                found = &dv;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->m_schema == 0);
        CHECK(found->m_value == "Standard");

        std::remove(path.c_str());
    }
}

// PR 8d.2b smoke tests — four larger no-storage OBJECTS families.

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads DICTIONARYWDFLT metadata",
          "[dwg-write][dictionary-wdflt]") {
    // PR 13h — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_dictionary_wdflt.dwg");
        {
            dwgRW writer(path.c_str());
            DictionaryWithDefaultRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        DictionaryWithDefaultRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_DictionaryWithDefault* found = nullptr;
        for (const DRW_DictionaryWithDefault& d : readIface.m_dictionaryObjects) {
            if (d.handle == 0x860u) {
                found = &d;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->cloning == 1);
        REQUIRE(found->m_entries.size() == 2);
        CHECK(found->m_entries[0].m_name == "Entry1");
        CHECK(found->m_entries[0].m_handle == 0x1200u);
        CHECK(found->m_entries[1].m_name == "Entry2");
        CHECK(found->m_entries[1].m_handle == 0x1201u);
        CHECK(found->m_defaultEntryHandle == 0x1202u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads SORTENTSTABLE metadata",
          "[dwg-write][sortentstable]") {
    // PR 13h — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_sortentstable.dwg");
        {
            dwgRW writer(path.c_str());
            SortEntsTableRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        SortEntsTableRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_SortEntsTable* found = nullptr;
        for (const DRW_SortEntsTable& s : readIface.m_sortEntsTableObjects) {
            if (s.handle == 0x870u) {
                found = &s;
                break;
            }
        }
        REQUIRE(found != nullptr);
        REQUIRE(found->m_sortHandles.size() == 3);
        CHECK(found->m_sortHandles[0] == 0x2001u);
        CHECK(found->m_sortHandles[1] == 0x2002u);
        CHECK(found->m_sortHandles[2] == 0x2003u);
        CHECK(found->m_blockOwnerHandle == 0x2100u);
        REQUIRE(found->m_entityHandles.size() == 3);
        CHECK(found->m_entityHandles[0] == 0x2010u);
        CHECK(found->m_entityHandles[1] == 0x2011u);
        CHECK(found->m_entityHandles[2] == 0x2012u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads FIELDLIST metadata",
          "[dwg-write][fieldlist]") {
    // PR 13h — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_fieldlist.dwg");
        {
            dwgRW writer(path.c_str());
            FieldListRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        FieldListRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_FieldList* found = nullptr;
        for (const DRW_FieldList& f : readIface.m_fieldListObjects) {
            if (f.handle == 0x880u) {
                found = &f;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->m_unknown == 1);
        REQUIRE(found->m_fieldHandles.size() == 2);
        CHECK(found->m_fieldHandles[0] == 0x2200u);
        CHECK(found->m_fieldHandles[1] == 0x2201u);

        std::remove(path.c_str());
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("dwgRW writes and reads FIELD metadata",
          "[dwg-write][field]") {
    // PR 13h — gate broadened to AC1015+; smoke array extended to cover
    // the new pre-AC1021 paths.  Exercises the parser-mirrored
    // `version < AC1021` m_formatString branch.
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("native_field.dwg");
        {
            dwgRW writer(path.c_str());
            FieldRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        FieldRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        const DRW_Field* found = nullptr;
        for (const DRW_Field& f : readIface.m_fieldObjects) {
            if (f.handle == 0x890u) {
                found = &f;
                break;
            }
        }
        REQUIRE(found != nullptr);
        CHECK(found->m_evaluatorId == "AcDbBlockEval");
        CHECK(found->m_fieldCode == "%<\\AcDbBlock>%");
        CHECK(found->m_evaluationOptionFlags == 1);
        CHECK(found->m_fieldStateFlags == 2);
        CHECK(found->m_evaluationStatusFlags == 1);
        CHECK(found->m_value.m_dataType == 2);
        CHECK(found->m_value.m_value.d_val() == Catch::Approx(12.5));
        CHECK(found->m_valueString == "12.5");
        CHECK(found->m_valueStringLength == 4);
        REQUIRE(found->m_childHandles.size() == 1);
        CHECK(found->m_childHandles[0] == 0x2301u);
        REQUIRE(found->m_objectHandles.size() == 1);
        CHECK(found->m_objectHandles[0] == 0x2401u);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW modern writers emit CLASSES for ARC_DIMENSION",
          "[dwg-write][classes][arc-dimension]") {
    const DRW::Version versions[] = {DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version version : versions) {
        const std::string path = tempPath("arc_dimension_classes.dwg");
        {
            dwgRW writer(path.c_str());
            ArcDimensionRoundTripIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, version, /*bin=*/false));
        }

        ArcDimensionRoundTripIface readIface;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(readIface.m_arcDimensions.size() == 1);
        REQUIRE(readIface.m_arcDimensions[0].getArcDefPoint().x == 5.0);
        REQUIRE(readIface.m_arcDimensions[0].arcStartAngle == 0.25);
        REQUIRE(readIface.m_arcDimensions[0].arcEndAngle == 1.25);

        std::remove(path.c_str());
    }
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

// 1.4 (gap classes-crc-not-validated): the CLASSES end sentinel result was
// computed and discarded.  For AC1015 (dwgReader15) it is now honored: a
// corrupted end sentinel must fail the read with BAD_READ_CLASSES.  A valid
// AC1015 file still reads clean (proving the happy path is unaffected).
TEST_CASE("dwgReader15 fails on a corrupted CLASSES end sentinel",
          "[dwg-write][smoke][classes][sentinel]") {
    const std::string path = tempPath("classes_end_sentinel.dwg");
    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
    }

    // Sanity: the pristine file reads clean.
    {
        dwgRW reader(path.c_str());
        EmptyIface readIface;
        REQUIRE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    // Locate the CLASSES section (record #1) end sentinel = last 16 bytes of
    // the section [classesAddr + classesSize - 16].
    auto bytes = slurp(path);
    duint32 classesAddr = readLE32(bytes, 0x19 + 1 * 9 + 1);
    duint32 classesSize = readLE32(bytes, 0x19 + 1 * 9 + 5);
    size_t endSentinelOff = classesAddr + classesSize - 16;
    REQUIRE(endSentinelOff + 16 <= bytes.size());
    REQUIRE(std::memcmp(bytes.data() + endSentinelOff,
                        dwgSentinels::CLASSES_END, 16) == 0);

    // Corrupt one sentinel byte and write the file back.
    bytes[endSentinelOff] ^= 0xFF;
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }

    // The read now fails with BAD_READ_CLASSES (end sentinel honored).
    {
        dwgRW reader(path.c_str());
        EmptyIface readIface;
        REQUIRE_FALSE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_READ_CLASSES);
    }

    std::remove(path.c_str());
}

// 1.5a (gap classes-crc-not-validated): the R13/R15 CLASSES CRC (crc16
// 0xC0C1) was read and discarded.  dwgReader15 now validates it over
// [si.address+16, si.address+20+classDataSize], matching writer15's emit
// range.  A pristine libdxfrw AC1015 file passes; a flipped class-data byte
// is detected as BAD_READ_CLASSES.
TEST_CASE("dwgReader15 validates the CLASSES CRC and detects a flipped byte",
          "[dwg-write][smoke][classes][crc]") {
    const std::string path = tempPath("classes_crc.dwg");
    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
    }

    // Pristine file: CRC validates, read is clean.
    {
        dwgRW reader(path.c_str());
        EmptyIface readIface;
        REQUIRE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    auto bytes = slurp(path);
    duint32 classesAddr = readLE32(bytes, 0x19 + 1 * 9 + 1);
    duint32 classesSize = readLE32(bytes, 0x19 + 1 * 9 + 5);
    // Class data lives at [classesAddr + 16(begin) + 4(sizeRL),
    // classesAddr + classesSize - 18(CRC+end sentinel)). Flip a byte inside
    // the CRC-covered region so the CRC no longer matches.
    size_t dataStart = classesAddr + 20;
    size_t dataEnd   = classesAddr + classesSize - 18;
    REQUIRE(dataEnd > dataStart);
    bytes[dataStart] ^= 0xFF;
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }

    // The CRC mismatch now fails the read.
    {
        dwgRW reader(path.c_str());
        EmptyIface readIface;
        REQUIRE_FALSE(reader.read(&readIface, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_READ_CLASSES);
    }

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

TEST_CASE("dwgRW named VIEW table records round-trip across writer versions",
          "[dwg-write][smoke][view]") {
    struct VersionCase {
        DRW::Version version;
        const char *suffix;
    };
    const VersionCase cases[] = {
        {DRW::AC1015, "r2000_named_view.dwg"},
        {DRW::AC1018, "r2004_named_view.dwg"},
        {DRW::AC1024, "r2010_named_view.dwg"},
        {DRW::AC1027, "r2013_named_view.dwg"},
        {DRW::AC1032, "r2018_named_view.dwg"}
    };

    for (const auto& item : cases) {
        const std::string path = tempPath(item.suffix);

        {
            ViewRoundTripIface iface;
            dwgRW writer(path.c_str());
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, item.version, /*bin=*/false));
        }

        ViewRoundTripIface cap;
        {
            dwgRW reader(path.c_str());
            REQUIRE(reader.read(&cap, /*ext=*/false));
            REQUIRE(reader.getVersion() == item.version);
            REQUIRE(reader.getError() == DRW::BAD_NONE);
        }

        REQUIRE(cap.m_views.size() == 1);
        requireNamedViewRoundTrip(cap.m_views[0], item.version);

        std::remove(path.c_str());
    }
}

TEST_CASE("dwgRW empty VIEW_CONTROL round-trips with no named views",
          "[dwg-write][smoke][view]") {
    const std::string path = tempPath("empty_view_control_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        EmptyIface iface;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    TableCaptureIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getVersion() == DRW::AC1024);
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    CHECK(cap.m_views.empty());

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

class PolyfaceRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Polyline> m_polylines;

    void writeEntities() override {
        if (m_writer == nullptr) return;

        DRW_Polyline pl;
        pl.flags = 64;
        pl.vertexcount = 3;
        pl.facecount = 1;

        DRW_Vertex v1(0.0, 0.0, 0.0, 0.0);
        v1.flags = 64 | 128;
        DRW_Vertex v2(10.0, 0.0, 0.0, 0.0);
        v2.flags = 64 | 128;
        DRW_Vertex v3(0.0, 10.0, 0.0, 0.0);
        v3.flags = 64 | 128;
        DRW_Vertex face;
        face.flags = 128;
        face.vindex1 = 1;
        face.vindex2 = -2;
        face.vindex3 = 3;

        pl.addVertex(v1);
        pl.addVertex(v2);
        pl.addVertex(v3);
        pl.addVertex(face);
        m_writer->writePolyline(&pl);
    }

    void addPolyline(const DRW_Polyline& pl) override {
        m_polylines.push_back(pl);
    }
};

class MeshRoundTripIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Polyline> m_polylines;

    void writeEntities() override {
        if (m_writer == nullptr) return;

        DRW_Polyline pl;
        pl.flags = 16;
        pl.vertexcount = 2;
        pl.facecount = 2;

        pl.addVertex(DRW_Vertex(0.0, 0.0, 0.0, 0.0));
        pl.addVertex(DRW_Vertex(10.0, 0.0, 1.0, 0.0));
        pl.addVertex(DRW_Vertex(0.0, 10.0, 2.0, 0.0));
        pl.addVertex(DRW_Vertex(10.0, 10.0, 3.0, 0.0));
        m_writer->writePolyline(&pl);
    }

    void addPolyline(const DRW_Polyline& pl) override {
        m_polylines.push_back(pl);
    }
};

class LWPolylineMetadataIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_LWPolyline> m_lwPolylines;

    void writeEntities() override {
        if (m_writer == nullptr) return;

        DRW_LWPolyline pl;
        pl.flags = 1;
        pl.width = 0.25;
        pl.elevation = 7.5;
        pl.thickness = 1.25;
        pl.extPoint = DRW_Coord{0.0, 1.0, 0.0};

        DRW_Vertex2D v0(0.0, 0.0, 0.0);
        v0.stawidth = 0.1;
        v0.endwidth = 0.2;
        v0.identifier = 101;
        DRW_Vertex2D v1(5.0, 0.0, 0.5);
        v1.stawidth = 0.3;
        v1.endwidth = 0.4;
        v1.identifier = 102;
        DRW_Vertex2D v2(5.0, 5.0, 0.0);
        v2.stawidth = 0.5;
        v2.endwidth = 0.6;
        v2.identifier = 103;

        pl.addVertex(v0);
        pl.addVertex(v1);
        pl.addVertex(v2);
        m_writer->writeLWPolyline(&pl);
    }

    void addLWPolyline(const DRW_LWPolyline& pl) override {
        m_lwPolylines.push_back(pl);
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
    REQUIRE(cap.m_polylines[0].dwgSeqEndHandle() != 0);
    REQUIRE(cap.m_polylines[0].vertlist.size() == 3);
    REQUIRE(cap.m_polylines[0].vertlist[0]->basePoint.x == 0.0);
    REQUIRE(cap.m_polylines[0].vertlist[1]->basePoint.x == 10.0);
    REQUIRE(cap.m_polylines[0].vertlist[2]->basePoint.y == 5.0);

    std::remove(path.c_str());
}

TEST_CASE("dwgRW R2010 polyface vertices keep DWG subtypes",
          "[dwg-write][r2010][polyline]") {
    const std::string path = tempPath("polyface_r2010.dwg");

    {
        dwgRW writer(path.c_str());
        PolyfaceRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    PolyfaceRoundTripIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(cap.m_polylines.size() == 1);
    REQUIRE(cap.m_polylines[0].flags == 64);
    REQUIRE(cap.m_polylines[0].vertlist.size() == 4);
    REQUIRE(cap.m_polylines[0].vertlist[0]->dwgSubtype()
            == DRW_Vertex::DwgSubtype::Polyface);
    REQUIRE(cap.m_polylines[0].vertlist[0]->flags == (64 | 128));
    REQUIRE(cap.m_polylines[0].vertlist[3]->dwgSubtype()
            == DRW_Vertex::DwgSubtype::PolyfaceFace);
    REQUIRE(cap.m_polylines[0].vertlist[3]->vindex1 == 1);
    REQUIRE(cap.m_polylines[0].vertlist[3]->vindex2 == -2);
    REQUIRE(cap.m_polylines[0].vertlist[3]->vindex3 == 3);

    std::remove(path.c_str());
}

TEST_CASE("RS_FilterDXFRW exports ellipse-segment polyline as old-style DWG",
          "[dwg-write][r2010][polyline]") {
    ensureQtSettings();
    const std::string path = tempPath("ellipse_polyline_export.dwg");

    RS_Graphic graphic;
    auto *polyline = new RS_Polyline(&graphic);
    polyline->addVertex(RS_Vector(0.0, 0.0, 0.0), 1.0);
    polyline->addVertex(RS_Vector(10.0, 0.0, 0.0));
    polyline->scale(RS_Vector(0.0, 0.0, 0.0), RS_Vector(1.0, 0.5, 1.0));
    graphic.addEntity(polyline);

    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(path),
                                  RS2::FormatDWG2004));
    }

    PolylineRoundTripIface cap;
    {
        dwgRW reader(path.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(cap.m_polylines.size() == 1);
    REQUIRE(!cap.m_polylines[0].vertlist.empty());

    std::remove(path.c_str());
}

TEST_CASE("RS_FilterDXFRW preserves LWPOLYLINE metadata sidecar",
          "[dwg-write][r2010][lwpolyline]") {
    ensureQtSettings();
    const std::string sourcePath = tempPath("lwpolyline_metadata_source.dwg");
    const std::string exportPath = tempPath("lwpolyline_metadata_export.dwg");

    {
        dwgRW writer(sourcePath.c_str());
        LWPolylineMetadataIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    RS_Graphic graphic;
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(graphic, QString::fromStdString(sourcePath),
                                  RS2::FormatDWG));
    }

    RS_Polyline *imported = nullptr;
    for (RS_Entity *entity :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
        if (entity != nullptr && entity->rtti() == RS2::EntityPolyline) {
            imported = static_cast<RS_Polyline*>(entity);
            break;
        }
    }
    REQUIRE(imported != nullptr);
    REQUIRE(imported->hasDrwExtData());

    bool sawMarker = false;
    int importedIds = 0;
    for (const auto& value : imported->getDrwExtData()) {
        if (!value)
            continue;
        if (value->code() == 1001
            && std::string{value->c_str()} == "LibreCAD_LWPOLYLINE") {
            sawMarker = true;
        }
        if (value->code() == 1071)
            ++importedIds;
    }
    REQUIRE(sawMarker);
    REQUIRE(importedIds == 3);

    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(exportPath),
                                  RS2::FormatDWG2004));
    }

    LWPolylineMetadataIface cap;
    {
        dwgRW reader(exportPath.c_str());
        REQUIRE(reader.read(&cap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(cap.m_lwPolylines.size() == 1);
    const DRW_LWPolyline& roundTrip = cap.m_lwPolylines[0];
    REQUIRE(roundTrip.width == 0.25);
    REQUIRE(roundTrip.elevation == 7.5);
    REQUIRE(roundTrip.thickness == 1.25);
    REQUIRE(roundTrip.extPoint.x == 0.0);
    REQUIRE(roundTrip.extPoint.y == 1.0);
    REQUIRE(roundTrip.extPoint.z == 0.0);
    REQUIRE(roundTrip.vertlist.size() == 3);
    REQUIRE(roundTrip.vertlist[0]->stawidth == 0.1);
    REQUIRE(roundTrip.vertlist[0]->endwidth == 0.2);
    REQUIRE(roundTrip.vertlist[2]->stawidth == 0.5);
    REQUIRE(roundTrip.vertlist[2]->endwidth == 0.6);

    std::remove(sourcePath.c_str());
    std::remove(exportPath.c_str());
}

TEST_CASE("RS_FilterDXFRW preserves mesh and polyface fallback sidecars",
          "[dwg-write][r2010][polyline]") {
    ensureQtSettings();
    const std::string polyfacePath = tempPath("polyface_sidecar_source.dwg");
    const std::string meshPath = tempPath("mesh_sidecar_source.dwg");
    const std::string polyfaceExportPath =
        tempPath("polyface_sidecar_export.dwg");
    const std::string meshExportPath = tempPath("mesh_sidecar_export.dwg");

    {
        dwgRW writer(polyfacePath.c_str());
        PolyfaceRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }
    {
        dwgRW writer(meshPath.c_str());
        MeshRoundTripIface iface;
        iface.m_writer = &writer;
        REQUIRE(writer.write(&iface, DRW::AC1024, /*bin=*/false));
    }

    RS_Graphic polyfaceGraphic;
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(polyfaceGraphic, QString::fromStdString(polyfacePath),
                                  RS2::FormatDWG));
    }

    bool sawPolyfaceMarker = false;
    bool sawSignedFaceIndex = false;
    int polyfaceAnchorCoords = 0;
    for (RS_Entity *entity :
         lc::LC_ContainerTraverser{polyfaceGraphic, RS2::ResolveNone}.entities()) {
        if (entity == nullptr || !entity->hasDrwExtData())
            continue;
        for (const auto& value : entity->getDrwExtData()) {
            if (!value)
                continue;
            if (value->code() == 1001
                && std::string{value->c_str()} == "LibreCAD_POLYLINE_PFACE") {
                sawPolyfaceMarker = true;
            }
            if (value->code() == 1070 && static_cast<int>(value->i_val()) == -2)
                sawSignedFaceIndex = true;
            if (value->code() == 1010)
                ++polyfaceAnchorCoords;
        }
    }
    REQUIRE(sawPolyfaceMarker);
    REQUIRE(sawSignedFaceIndex);
    REQUIRE(polyfaceAnchorCoords == 3);

    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(polyfaceGraphic,
                                  QString::fromStdString(polyfaceExportPath),
                                  RS2::FormatDWG2004));
    }
    PolyfaceRoundTripIface polyfaceCap;
    {
        dwgRW reader(polyfaceExportPath.c_str());
        REQUIRE(reader.read(&polyfaceCap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }
    REQUIRE(polyfaceCap.m_polylines.size() == 1);
    REQUIRE((polyfaceCap.m_polylines[0].flags & 64) != 0);
    REQUIRE(polyfaceCap.m_polylines[0].vertexcount == 3);
    REQUIRE(polyfaceCap.m_polylines[0].facecount == 1);
    REQUIRE(polyfaceCap.m_polylines[0].vertlist.size() == 4);
    REQUIRE(polyfaceCap.m_polylines[0].vertlist[3]->vindex2 == -2);

    RS_Graphic meshGraphic;
    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(meshGraphic, QString::fromStdString(meshPath),
                                  RS2::FormatDWG));
    }

    int meshSidecarCount = 0;
    int meshAnchorCoords = 0;
    bool sawMeshRow = false;
    bool sawMeshColumn = false;
    for (RS_Entity *entity :
         lc::LC_ContainerTraverser{meshGraphic, RS2::ResolveNone}.entities()) {
        if (entity == nullptr || !entity->hasDrwExtData())
            continue;

        bool entityHasMeshMarker = false;
        for (const auto& value : entity->getDrwExtData()) {
            if (!value)
                continue;
            if (value->code() == 1001
                && std::string{value->c_str()} == "LibreCAD_POLYLINE_MESH") {
                entityHasMeshMarker = true;
            }
            if (value->code() == 1000) {
                if (std::string{value->c_str()} == "row")
                    sawMeshRow = true;
                if (std::string{value->c_str()} == "column")
                    sawMeshColumn = true;
            }
            if (value->code() == 1010)
                ++meshAnchorCoords;
        }
        if (entityHasMeshMarker)
            ++meshSidecarCount;
    }
    REQUIRE(meshSidecarCount == 4);
    REQUIRE(sawMeshRow);
    REQUIRE(sawMeshColumn);
    REQUIRE(meshAnchorCoords == 4);

    {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(meshGraphic, QString::fromStdString(meshExportPath),
                                  RS2::FormatDWG2004));
    }
    MeshRoundTripIface meshCap;
    {
        dwgRW reader(meshExportPath.c_str());
        REQUIRE(reader.read(&meshCap, /*ext=*/false));
        REQUIRE(reader.getError() == DRW::BAD_NONE);
    }
    REQUIRE(meshCap.m_polylines.size() == 1);
    REQUIRE((meshCap.m_polylines[0].flags & 16) != 0);
    REQUIRE(meshCap.m_polylines[0].vertexcount == 2);
    REQUIRE(meshCap.m_polylines[0].facecount == 2);
    REQUIRE(meshCap.m_polylines[0].vertlist.size() == 4);
    REQUIRE(meshCap.m_polylines[0].vertlist[1]->basePoint.z == 1.0);
    REQUIRE(meshCap.m_polylines[0].vertlist[3]->basePoint.z == 3.0);

    std::remove(polyfacePath.c_str());
    std::remove(meshPath.c_str());
    std::remove(polyfaceExportPath.c_str());
    std::remove(meshExportPath.c_str());
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

// ---------------------------------------------------------------------------
// 1.7 — External write->reread validator fixtures.
//
// Hidden test [.dwg_emit_framing]: emit a trivial-but-nonempty drawing
// (2 lines + 1 named layer) as AC1015/AC1018/AC1024/AC1027/AC1032 to a known
// directory so scripts/dwg-validate.sh can re-read each with libreDWG's
// external `dwgread` (the framing gate). The in-repo self-consistency
// write->read loop runs unconditionally below to demonstrate that
// self-consistency PASS does NOT imply external PASS for AC1027/AC1032.
// ---------------------------------------------------------------------------
namespace {

class FramingIface : public EmptyIface {
public:
    dwgRW *m_writer {nullptr};
    std::vector<DRW_Line> m_lines;
    std::vector<std::string> m_layers;

    void writeLayers() override {
        if (m_writer == nullptr) return;
        DRW_Layer lay;
        lay.name = "FRAME";
        lay.color = 2;
        m_writer->addLayer(&lay);
    }
    void writeEntities() override {
        if (m_writer == nullptr) return;
        DRW_Line a;
        a.basePoint = DRW_Coord{0.0, 0.0, 0.0};
        a.secPoint  = DRW_Coord{10.0, 0.0, 0.0};
        a.color = 1;
        m_writer->writeLine(&a);
        DRW_Line b;
        b.basePoint = DRW_Coord{0.0, 0.0, 0.0};
        b.secPoint  = DRW_Coord{0.0, 10.0, 0.0};
        b.color = 3;
        m_writer->writeLine(&b);
    }
    void addLine(const DRW_Line& l) override { m_lines.push_back(l); }
    void addLayer(const DRW_Layer& l) override { m_layers.push_back(l.name); }
};

// Stable output dir under the repo's tmp/ so dwg-validate.sh can locate the
// emitted fixtures without guessing the system temp path.
std::string framingDir() {
    std::filesystem::path d = std::filesystem::path("tmp") / "dwg-validate";
    std::error_code ec;
    std::filesystem::create_directories(d, ec);
    return d.string();
}

const char* framingFileFor(DRW::Version v) {
    switch (v) {
        case DRW::AC1015: return "framing_AC1015.dwg";
        case DRW::AC1018: return "framing_AC1018.dwg";
        case DRW::AC1024: return "framing_AC1024.dwg";
        case DRW::AC1027: return "framing_AC1027.dwg";
        case DRW::AC1032: return "framing_AC1032.dwg";
        default:          return "framing_unknown.dwg";
    }
}

}  // namespace

// Hidden (leading-dot tag): not run by the default suite; invoked explicitly
// by scripts/dwg-validate.sh. Emits the 5 framing fixtures and proves the
// in-repo self-consistency loop passes for every version (the core finding:
// self-consistency PASS != external PASS for the thin AC1027/AC1032 writers).
TEST_CASE("DWG framing fixtures emit + self-consistency round-trip",
          "[.dwg_emit_framing]") {
    const std::string dir = framingDir();
    const DRW::Version versions[] = {
        DRW::AC1015, DRW::AC1018, DRW::AC1024, DRW::AC1027, DRW::AC1032};

    for (DRW::Version v : versions) {
        const std::string path =
            (std::filesystem::path(dir) / framingFileFor(v)).string();
        {
            dwgRW writer(path.c_str());
            FramingIface iface;
            iface.m_writer = &writer;
            REQUIRE(writer.write(&iface, v, /*bin=*/false));
        }
        // In-repo self-consistency: libdxfrw must re-read its own output.
        {
            dwgRW reader(path.c_str());
            FramingIface readIface;
            REQUIRE(reader.read(&readIface, /*ext=*/false));
            REQUIRE(reader.getVersion() == v);
            REQUIRE(readIface.m_lines.size() >= 2u);
        }
        // NOTE: the fixtures are intentionally LEFT on disk for
        // scripts/dwg-validate.sh to feed to the external dwgread.
    }
}
