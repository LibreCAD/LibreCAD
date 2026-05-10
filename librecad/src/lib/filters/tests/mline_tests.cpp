/**
 * MLINE entity tests — covers DRW_MLine + DRW_MLineStyle field round-trip
 * via in-memory DXF write+read, and the LibreCAD-side decomposition that
 * fans MLINE out into N parallel polylines on import.
 */

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
#include "libdxfrw.h"
#include "libdwgr.h"

namespace {

// Stub satisfying every DRW_Interface pure virtual; tests override only
// what they need. Pattern lifted from entity_metadata_tests.cpp.
class StubInterface : public DRW_Interface {
public:
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
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
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
    void addWipeout(const DRW_Image*) override {}
    void addMLeader(const DRW_MLeader*) override {}
    void addMLeaderStyle(const DRW_MLeaderStyle*) override {}
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

class MLineCapture : public StubInterface {
public:
    int m_callCount = 0;
    DRW_MLine m_captured;
    void addMLine(const DRW_MLine* d) override {
        if (d && m_callCount == 0) m_captured = *d;
        ++m_callCount;
    }
};

class MLineEmitter : public StubInterface {
public:
    DRW_MLine m_line;
    dxfRW* m_rw = nullptr;
    void writeEntities() override {
        m_rw->writeMLine(&m_line);
    }
};

} // anon

TEST_CASE("DRW_MLine: default field values", "[mline]") {
    DRW_MLine m;
    CHECK(m.scale == 1.0);
    CHECK(m.justification == 0);
    CHECK(m.openClosed == 1);
    CHECK(m.numLines == 0);
    CHECK(m.numVerts == 0);
    CHECK(m.styleHandle == 0u);
    CHECK(m.vertlist.empty());
}

TEST_CASE("DRW_MLineStyle: default field values", "[mline]") {
    DRW_MLineStyle s;
    CHECK(s.elements.empty());
    CHECK(s.fillColor == 256);
    CHECK(s.flags == 0);
    CHECK(s.startAngle == 0.0);
    CHECK(s.endAngle == 0.0);
}

TEST_CASE("DRW_MLine: vertex container holds segParms per element", "[mline]") {
    DRW_MLineVertex v;
    v.position = DRW_Coord(1.0, 2.0, 0.0);
    v.miterDir = DRW_Coord(0.0, 1.0, 0.0);
    v.segParms.resize(2);
    v.segParms[0].push_back(0.0);
    v.segParms[1].push_back(0.0);
    v.areaFillParms.resize(2);

    CHECK(v.segParms.size() == 2);
    CHECK(v.segParms[0].size() == 1);
    CHECK(v.segParms[1].size() == 1);
    CHECK(v.areaFillParms.size() == 2);
    CHECK(v.areaFillParms[0].empty());
}

TEST_CASE("DRW_MLine: round-trip through dxfRW write+read", "[mline][dxf_roundtrip]") {
    const auto path = std::filesystem::temp_directory_path()
                      / "librecad_mline_roundtrip.dxf";
    std::filesystem::remove(path);

    MLineEmitter emitter;
    emitter.m_line.styleName = "STANDARD";
    emitter.m_line.scale = 2.5;
    emitter.m_line.justification = 1;
    emitter.m_line.openClosed = 1;
    emitter.m_line.numLines = 2;
    emitter.m_line.numVerts = 3;
    emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
    emitter.m_line.layer = "0";
    for (int i = 0; i < 3; ++i) {
        DRW_MLineVertex v;
        v.position  = DRW_Coord(static_cast<double>(i), 0.0, 0.0);
        v.vertexDir = DRW_Coord(1.0, 0.0, 0.0);
        v.miterDir  = DRW_Coord(0.0, 1.0, 0.0);
        v.segParms.resize(2);
        v.segParms[0].push_back(0.0);
        v.segParms[1].push_back(0.0);
        v.areaFillParms.resize(2);
        emitter.m_line.vertlist.push_back(v);
    }

    {
        dxfRW w(path.string().c_str());
        emitter.m_rw = &w;
        REQUIRE(w.write(&emitter, DRW::AC1021, false));
    }

    MLineCapture capture;
    {
        dxfRW r(path.string().c_str());
        REQUIRE(r.read(&capture, /*ext=*/true));
    }

    REQUIRE(capture.m_callCount == 1);
    CHECK(capture.m_captured.styleName == "STANDARD");
    CHECK(capture.m_captured.scale == 2.5);
    CHECK(capture.m_captured.justification == 1);
    CHECK(capture.m_captured.numLines == 2);
    CHECK(capture.m_captured.vertlist.size() == 3);
    if (capture.m_captured.vertlist.size() >= 3) {
        CHECK(capture.m_captured.vertlist[0].position.x == 0.0);
        CHECK(capture.m_captured.vertlist[1].position.x == 1.0);
        CHECK(capture.m_captured.vertlist[2].position.x == 2.0);
    }

    std::filesystem::remove(path);
}

TEST_CASE("DXF MLINE write emits expected codes", "[mline][dxf_roundtrip]") {
    const auto path = std::filesystem::temp_directory_path()
                      / "librecad_mline_codes.dxf";
    std::filesystem::remove(path);

    MLineEmitter emitter;
    emitter.m_line.styleName = "STANDARD";
    emitter.m_line.scale = 1.0;
    emitter.m_line.justification = 0;
    emitter.m_line.numLines = 2;
    emitter.m_line.numVerts = 2;
    emitter.m_line.layer = "0";
    for (int i = 0; i < 2; ++i) {
        DRW_MLineVertex v;
        v.position  = DRW_Coord(static_cast<double>(i), 0.0, 0.0);
        v.miterDir  = DRW_Coord(0.0, 1.0, 0.0);
        v.segParms.resize(2);
        v.areaFillParms.resize(2);
        emitter.m_line.vertlist.push_back(v);
    }

    {
        dxfRW w(path.string().c_str());
        emitter.m_rw = &w;
        REQUIRE(w.write(&emitter, DRW::AC1021, false));
    }

    std::ifstream in(path);
    std::stringstream buf;
    buf << in.rdbuf();
    const std::string content = buf.str();
    CHECK(content.find("\nMLINE\n") != std::string::npos);
    CHECK(content.find("AcDbMline") != std::string::npos);
    CHECK(content.find("STANDARD") != std::string::npos);

    std::filesystem::remove(path);
}

// Smoke probe: hidden by [.] tag, runs against ~/doc/dwg3/Multiline.dwg
// if present. Confirms the MLINE entity reaches addMLine.
TEST_CASE("DWG smoke: Multiline.dwg delivers >= 1 MLINE entity", "[.dwg_mline_probe]") {
    const char* home = getenv("HOME");
    if (!home) { SUCCEED("HOME not set"); return; }
    const std::string path = std::string(home) + "/doc/dwg3/Multiline.dwg";
    if (!std::filesystem::is_regular_file(path)) {
        SUCCEED("~/doc/dwg3/Multiline.dwg not found");
        return;
    }
    MLineCapture capture;
    dwgR reader(path.c_str());
    bool ok = reader.read(&capture, /*ext=*/true);
    CHECK(ok);
    CHECK(capture.m_callCount >= 1);
}
