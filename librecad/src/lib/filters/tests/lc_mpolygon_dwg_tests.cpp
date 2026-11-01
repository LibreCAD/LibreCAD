/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
****************************************************************************/

// Regression suite for the AcDbMPolygon DWG body layout.
//
// Before this change libdxfrw parsed MPOLYGON as if it were HATCH: a leading
// style BS, an associative bit between name and loops, HATCH's edge-path
// boundary data and a trailing boundary-handle list. ODA File Converter and
// AutoCAD write a different layout: the leading BS is the version, there is
// no associative bit, every loop is an is_closed/hasBulge/vertex record, and
// the handle stream ends at the common entity handles. Every ODA-written
// MPOLYGON therefore imported with zero loops. These tests exercise the new
// parser and encoder against ezdxf-authored, ODA-converted fixtures.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "libdwgr.h"

#include "lc_filtertestsupport.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs.h"

namespace {

// Stub satisfying every DRW_Interface pure virtual, so each test file only
// needs to override the handful of entry points it exercises.
class StubInterface : public DRW_Interface {
public:
    void addHeader(const DRW_Header *) override {}
    void addLType(const DRW_LType &) override {}
    void addLayer(const DRW_Layer &) override {}
    void addDimStyle(const DRW_Dimstyle &) override {}
    void addVport(const DRW_Vport &) override {}
    void addTextStyle(const DRW_Textstyle &) override {}
    void addAppId(const DRW_AppId &) override {}
    void addBlock(const DRW_Block &) override {}
    void setBlock(const int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point &) override {}
    void addLine(const DRW_Line &) override {}
    void addRay(const DRW_Ray &) override {}
    void addXline(const DRW_Xline &) override {}
    void addArc(const DRW_Arc &) override {}
    void addCircle(const DRW_Circle &) override {}
    void addEllipse(const DRW_Ellipse &) override {}
    void addLWPolyline(const DRW_LWPolyline &) override {}
    void addPolyline(const DRW_Polyline &) override {}
    void addSpline(const DRW_Spline *) override {}
    void addKnot(const DRW_Entity &) override {}
    void addInsert(const DRW_Insert &) override {}
    void addTrace(const DRW_Trace &) override {}
    void add3dFace(const DRW_3Dface &) override {}
    void addSolid(const DRW_Solid &) override {}
    void addMText(const DRW_MText &) override {}
    void addText(const DRW_Text &) override {}
    void addDimAlign(const DRW_DimAligned *) override {}
    void addDimLinear(const DRW_DimLinear *) override {}
    void addDimRadial(const DRW_DimRadial *) override {}
    void addDimDiametric(const DRW_DimDiametric *) override {}
    void addDimAngular(const DRW_DimAngular *) override {}
    void addDimAngular3P(const DRW_DimAngular3p *) override {}
    void addDimArc(const DRW_DimArc *) override {}
    void addDimOrdinate(const DRW_DimOrdinate *) override {}
    void addLeader(const DRW_Leader *) override {}
    void addHatch(const DRW_Hatch *) override {}
    void addViewport(const DRW_Viewport &) override {}
    void addImage(const DRW_Image *) override {}
    void addWipeout(const DRW_Wipeout *) override {}
    void addMLeader(const DRW_MLeader *) override {}
    void addMLeaderStyle(const DRW_MLeaderStyle *) override {}
    void linkImage(const DRW_ImageDef *) override {}
    void addComment(const char *) override {}
    void addPlotSettings(const DRW_PlotSettings *) override {}
    void writeHeader(DRW_Header &) override {}
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

class MPolygonDwgCapture : public StubInterface {
public:
    int m_mpolygonCount = 0;
    int m_hatchCount = 0;
    DRW_MPolygon m_captured;
    void addMPolygon(const DRW_MPolygon *d) override {
        if (m_mpolygonCount == 0 && d)
            m_captured = *d;
        ++m_mpolygonCount;
    }
    void addHatch(const DRW_Hatch *) override { ++m_hatchCount; }
};

// Opens the DWG under $LIBRECAD_TEST_DIR/mpolygon/<name> through dwgR into one
// MPolygonDwgCapture. The capture lives at least as long as the returned
// reference by holding it in the caller's scope.
bool readMPolygonFixture(const std::string &name, MPolygonDwgCapture &cap) {
    const std::string path =
        std::string(LIBRECAD_TEST_DIR) + "/mpolygon/" + name;
    if (!std::filesystem::is_regular_file(path))
        return false;
    dwgR reader(path.c_str());
    if (!reader.read(&cap, /*ext=*/false))
        return false;
    return reader.getError() == DRW::BAD_NONE;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ODA MPOLYGON DWGs read their boundary loops",
          "[lc-hatchfix][mpolygon]") {
    SECTION("AC1015 pattern fixture") {
        MPolygonDwgCapture cap;
        if (!readMPolygonFixture("oda_pattern_ac1015.dwg", cap))
            SKIP("oda_pattern_ac1015.dwg fixture not found; skipping");

        REQUIRE(cap.m_mpolygonCount == 1);
        const DRW_MPolygon &mp = cap.m_captured;
        CHECK(mp.solid == 0);
        CHECK(mp.name == "ANSI31");
        CHECK(mp.hpattern == 1);
        CHECK(mp.mpVersion == 1);
        CHECK(mp.hstyle == 0);
        CHECK(mp.associative == 0);
        CHECK(mp.angle == Catch::Approx(0.0).margin(1e-12));
        CHECK(mp.xDirX == Catch::Approx(0.0).margin(1e-12));
        CHECK(mp.xDirY == Catch::Approx(0.0).margin(1e-12));
        CHECK(mp.degenerateLoops == 0);
        CHECK(mp.fillColorAci == 256);
        REQUIRE(mp.looplist.size() == 1);
        const auto &loop = mp.looplist[0];
        CHECK((loop->type & 2) == 2);
        REQUIRE_FALSE(loop->objlist.empty());
        const auto *pl =
            dynamic_cast<const DRW_LWPolyline *>(loop->objlist[0].get());
        REQUIRE(pl != nullptr);
        CHECK((pl->flags & 1) == 1);
        REQUIRE(pl->vertlist.size() == 4u);
        const double ex[] = {0.0, 100.0, 100.0, 0.0};
        const double ey[] = {0.0, 0.0, 60.0, 60.0};
        for (std::size_t i = 0; i < 4; ++i) {
            INFO("vertex " << i);
            CHECK(std::abs(pl->vertlist[i]->x - ex[i]) < 1e-9);
            CHECK(std::abs(pl->vertlist[i]->y - ey[i]) < 1e-9);
            CHECK(pl->vertlist[i]->bulge == 0.0);
        }
        REQUIRE(mp.patternLines.size() == 1);
        CHECK(mp.scale == Catch::Approx(4.0));
        CHECK(mp.patternLines[0].angle
              == Catch::Approx(0.7853981633974483).margin(1e-12));
        CHECK(mp.patternLines[0].baseX == Catch::Approx(0.0).margin(1e-12));
        CHECK(mp.patternLines[0].baseY == Catch::Approx(0.0).margin(1e-12));
        CHECK(mp.patternLines[0].offsetX
              == Catch::Approx(-8.9802561212).margin(1e-9));
        CHECK(mp.patternLines[0].offsetY
              == Catch::Approx(8.9802561212).margin(1e-9));
        CHECK(mp.patternLines[0].dashList.empty());
    }

    SECTION("AC1018 pattern fixture") {
        MPolygonDwgCapture cap;
        if (!readMPolygonFixture("oda_pattern_ac1018.dwg", cap))
            SKIP("oda_pattern_ac1018.dwg fixture not found; skipping");

        REQUIRE(cap.m_mpolygonCount == 1);
        const DRW_MPolygon &mp = cap.m_captured;
        CHECK(mp.solid == 0);
        CHECK(mp.name == "ANSI31");
        CHECK(mp.mpVersion == 1);
        CHECK(mp.associative == 0);
        REQUIRE(mp.looplist.size() == 1);
        REQUIRE(mp.patternLines.size() == 1);
        CHECK(mp.scale == Catch::Approx(4.0));
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("Solid ODA MPOLYGON DWGs read their boundary",
          "[lc-hatchfix][mpolygon]") {
    const std::vector<std::string> fixtures = {
        "oda_solid_ac1015.dwg", "oda_solid_ac1024.dwg"};
    for (const std::string &name : fixtures) {
        DYNAMIC_SECTION(name) {
            MPolygonDwgCapture cap;
            if (!readMPolygonFixture(name, cap))
                SKIP(name + " fixture not found; skipping");

            REQUIRE(cap.m_mpolygonCount == 1);
            const DRW_MPolygon &mp = cap.m_captured;
            CHECK(mp.solid == 1);
            CHECK(mp.name == "SOLID");
            CHECK(mp.mpVersion == 1);
            CHECK(mp.associative == 0);
            CHECK(mp.fillColorAci == 3);
            REQUIRE(mp.looplist.size() == 1);
            CHECK(mp.patternLines.empty());
            REQUIRE_FALSE(mp.looplist[0]->objlist.empty());
            const auto *pl = dynamic_cast<const DRW_LWPolyline *>(
                mp.looplist[0]->objlist[0].get());
            REQUIRE(pl != nullptr);
            CHECK((pl->flags & 1) == 1);
            CHECK(pl->vertlist.size() == 4u);
        }
    }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ODA MPOLYGON bulge polyline reads at AC1015",
          "[lc-hatchfix][mpolygon]") {
    MPolygonDwgCapture cap;
    if (!readMPolygonFixture("oda_bulge_two_ac1015.dwg", cap))
        SKIP("oda_bulge_two_ac1015.dwg fixture not found; skipping");

    REQUIRE(cap.m_mpolygonCount == 1);
    const DRW_MPolygon &mp = cap.m_captured;
    CHECK(mp.solid == 0);
    CHECK(mp.name == "ANSI31");
    CHECK(mp.mpVersion == 1);
    CHECK(mp.associative == 0);
    REQUIRE(mp.looplist.size() == 2);

    const auto *outer = dynamic_cast<const DRW_LWPolyline *>(
        mp.looplist[0]->objlist[0].get());
    REQUIRE(outer != nullptr);
    REQUIRE(outer->vertlist.size() == 4u);
    const double expectBulge[] = {0.0, 0.5, 0.0, 0.0};
    const double ex[] = {0.0, 100.0, 100.0, 0.0};
    const double ey[] = {0.0, 0.0, 60.0, 60.0};
    for (std::size_t i = 0; i < 4; ++i) {
        INFO("outer vertex " << i);
        CHECK(std::abs(outer->vertlist[i]->x - ex[i]) < 1e-9);
        CHECK(std::abs(outer->vertlist[i]->y - ey[i]) < 1e-9);
        CHECK(outer->vertlist[i]->bulge
              == Catch::Approx(expectBulge[i]).margin(1e-12));
    }

    const auto *inner = dynamic_cast<const DRW_LWPolyline *>(
        mp.looplist[1]->objlist[0].get());
    REQUIRE(inner != nullptr);
    REQUIRE(inner->vertlist.size() == 4u);
    const double ix[] = {30.0, 70.0, 70.0, 30.0};
    const double iy[] = {20.0, 20.0, 40.0, 40.0};
    for (std::size_t i = 0; i < 4; ++i) {
        INFO("inner vertex " << i);
        CHECK(std::abs(inner->vertlist[i]->x - ix[i]) < 1e-9);
        CHECK(std::abs(inner->vertlist[i]->y - iy[i]) < 1e-9);
        CHECK(inner->vertlist[i]->bulge == 0.0);
    }

    REQUIRE(mp.patternLines.size() == 1);
    CHECK(mp.patternLines[0].angle
          == Catch::Approx(1.3089969389957472).margin(1e-12));
    CHECK(mp.patternLines[0].offsetX
          == Catch::Approx(-12.267257994).margin(1e-9));
    CHECK(mp.patternLines[0].offsetY
          == Catch::Approx(3.2870018728).margin(1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ODA MPOLYGON three-loop brick30 fixture reads at AC1032",
          "[lc-hatchfix][mpolygon]") {
    MPolygonDwgCapture cap;
    if (!readMPolygonFixture("oda_three_loops_brick30_ac1032.dwg", cap))
        SKIP("oda_three_loops_brick30_ac1032.dwg fixture not found; skipping");

    REQUIRE(cap.m_mpolygonCount == 1);
    const DRW_MPolygon &mp = cap.m_captured;
    CHECK(mp.solid == 0);
    CHECK(mp.mpVersion == 1);
    CHECK(mp.associative == 0);
    REQUIRE(mp.looplist.size() == 3);

    // Loop 1 has four vertices and bulge pattern (0, 0.5, 0, -0.3).
    const auto *loop1 = dynamic_cast<const DRW_LWPolyline *>(
        mp.looplist[0]->objlist[0].get());
    REQUIRE(loop1 != nullptr);
    REQUIRE(loop1->vertlist.size() == 4u);
    const double expectBulge[] = {0.0, 0.5, 0.0, -0.3};
    for (std::size_t i = 0; i < 4; ++i) {
        INFO("loop 1 bulge " << i);
        CHECK(loop1->vertlist[i]->bulge
              == Catch::Approx(expectBulge[i]).margin(1e-12));
    }

    const auto *loop2 = dynamic_cast<const DRW_LWPolyline *>(
        mp.looplist[1]->objlist[0].get());
    REQUIRE(loop2 != nullptr);
    REQUIRE(loop2->vertlist.size() == 4u);
    const double l2x[] = {20.0, 80.0, 80.0, 20.0};
    const double l2y[] = {10.0, 10.0, 50.0, 50.0};
    for (std::size_t i = 0; i < 4; ++i) {
        INFO("loop 2 vertex " << i);
        CHECK(std::abs(loop2->vertlist[i]->x - l2x[i]) < 1e-9);
        CHECK(std::abs(loop2->vertlist[i]->y - l2y[i]) < 1e-9);
    }

    const auto *loop3 = dynamic_cast<const DRW_LWPolyline *>(
        mp.looplist[2]->objlist[0].get());
    REQUIRE(loop3 != nullptr);
    REQUIRE(loop3->vertlist.size() == 4u);
    const double l3x[] = {40.0, 60.0, 60.0, 40.0};
    const double l3y[] = {20.0, 20.0, 40.0, 40.0};
    for (std::size_t i = 0; i < 4; ++i) {
        INFO("loop 3 vertex " << i);
        CHECK(std::abs(loop3->vertlist[i]->x - l3x[i]) < 1e-9);
        CHECK(std::abs(loop3->vertlist[i]->y - l3y[i]) < 1e-9);
    }

    CHECK(mp.scale == Catch::Approx(2.5));
    REQUIRE(mp.patternLines.size() == 3);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ODA MPOLYGON open path fixture is reported open",
          "[lc-hatchfix][mpolygon]") {
    MPolygonDwgCapture cap;
    if (!readMPolygonFixture("oda_open_path_ac1032.dwg", cap))
        SKIP("oda_open_path_ac1032.dwg fixture not found; skipping");

    REQUIRE(cap.m_mpolygonCount == 1);
    const DRW_MPolygon &mp = cap.m_captured;
    CHECK(mp.mpVersion == 1);
    CHECK(mp.associative == 0);
    REQUIRE(mp.looplist.size() == 1);
    const auto *pl = dynamic_cast<const DRW_LWPolyline *>(
        mp.looplist[0]->objlist[0].get());
    REQUIRE(pl != nullptr);
    // is_closed = 0 in ODA's layout maps to flags & 1 == 0.
    CHECK((pl->flags & 1) == 0);
    CHECK(pl->vertlist.size() == 4u);
    REQUIRE(mp.patternLines.size() == 1);
    CHECK(mp.patternLines[0].angle
          == Catch::Approx(0.7853981633974483).margin(1e-12));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ODA MPOLYGON herringbone fixture reads its dashed definition lines",
          "[lc-hatchfix][mpolygon]") {
    MPolygonDwgCapture cap;
    if (!readMPolygonFixture("oda_ar_hbone_117_ac1032.dwg", cap))
        SKIP("oda_ar_hbone_117_ac1032.dwg fixture not found; skipping");

    REQUIRE(cap.m_mpolygonCount == 1);
    const DRW_MPolygon &mp = cap.m_captured;
    CHECK(mp.solid == 0);
    CHECK(mp.mpVersion == 1);
    CHECK(mp.associative == 0);
    CHECK(mp.scale == Catch::Approx(0.25));
    REQUIRE(mp.patternLines.size() == 2);

    CHECK(mp.patternLines[0].angle
          == Catch::Approx(2.8361600344907854).margin(1e-12));
    CHECK(mp.patternLines[0].offsetX
          == Catch::Approx(-31.8623378564).margin(1e-9));
    CHECK(mp.patternLines[0].offsetY
          == Catch::Approx(-16.5864832416).margin(1e-9));
    REQUIRE(mp.patternLines[0].dashList.size() == 2);
    CHECK(mp.patternLines[0].dashList[0]
          == Catch::Approx(76.2).margin(1e-9));
    CHECK(mp.patternLines[0].dashList[1]
          == Catch::Approx(-25.4).margin(1e-9));

    CHECK(mp.patternLines[1].angle
          == Catch::Approx(4.4069563612856820).margin(1e-12));
    CHECK(mp.patternLines[1].baseX
          == Catch::Approx(-24.2243940373).margin(1e-9));
    CHECK(mp.patternLines[1].baseY
          == Catch::Approx(7.6379221013).margin(1e-9));
    CHECK(mp.patternLines[1].offsetX
          == Catch::Approx(-31.8623378564).margin(1e-9));
    CHECK(mp.patternLines[1].offsetY
          == Catch::Approx(-16.5864832416).margin(1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("ODA MPOLYGON pattern30 fixture reads two nested loops",
          "[lc-hatchfix][mpolygon]") {
    MPolygonDwgCapture cap;
    if (!readMPolygonFixture("oda_pattern30_ac1032.dwg", cap))
        SKIP("oda_pattern30_ac1032.dwg fixture not found; skipping");

    REQUIRE(cap.m_mpolygonCount == 1);
    const DRW_MPolygon &mp = cap.m_captured;
    CHECK(mp.mpVersion == 1);
    CHECK(mp.associative == 0);
    REQUIRE(mp.looplist.size() == 2);
    REQUIRE(mp.patternLines.size() == 1);
    CHECK(mp.patternLines[0].angle
          == Catch::Approx(1.3089969389957472).margin(1e-12));
    CHECK(mp.patternLines[0].offsetX
          == Catch::Approx(-12.267257994).margin(1e-9));
    CHECK(mp.patternLines[0].offsetY
          == Catch::Approx(3.2870018728).margin(1e-9));
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("MPOLYGON polylineLoop refuses ellipse edges",
          "[lc-hatchfix][mpolygon]") {
    auto loop = std::make_shared<DRW_HatchLoop>(0); // edge-list path
    auto ellipse = std::make_shared<DRW_Ellipse>();
    ellipse->basePoint = DRW_Coord{0.0, 0.0, 0.0};
    ellipse->secPoint = DRW_Coord{10.0, 0.0, 0.0};
    ellipse->ratio = 0.5;
    ellipse->staparam = 0.0;
    ellipse->endparam = 6.283185307179586;
    loop->objlist.push_back(ellipse);
    loop->update();

    DRW_LWPolyline flattened;
    CHECK_FALSE(DRW_MPolygon::polylineLoop(*loop, flattened));
    CHECK(flattened.vertlist.empty());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("An ODA MPOLYGON DWG opens as a pattern hatch with its loops",
          "[lc-hatchfix][mpolygon]") {
    REQUIRE(lc::test::ensureSettings());

    const std::filesystem::path path =
        std::filesystem::path(LIBRECAD_TEST_DIR) / "mpolygon"
        / "oda_bulge_two_ac1015.dwg";
    if (!std::filesystem::is_regular_file(path))
        SKIP("oda_bulge_two_ac1015.dwg fixture not found; skipping");

    RS_Graphic graphic;
    {
        lc::test::QuietLog quiet;
        REQUIRE(lc::test::importDrawing(graphic, path, RS2::FormatDWG));
    }

    int hatchCount = 0;
    const RS_Hatch *hatch = nullptr;
    for (RS_Entity *e : graphic) {
        if (e != nullptr && e->rtti() == RS2::EntityHatch) {
            ++hatchCount;
            hatch = static_cast<const RS_Hatch *>(e);
        }
    }
    REQUIRE(hatchCount == 1);
    REQUIRE(hatch != nullptr);
    CHECK_FALSE(hatch->isSolid());
    CHECK(hatch->getPattern() == QString("ANSI31"));
    // Outer 100 x 60 rectangle plus the bulge-0.5 arc minus the 40 x 20 inner
    // rectangle gives 6000 + 629.0089003147672 - 800 = 5829.008900314767.
    const double expected = 5829.008900314767;
    const double actual = std::abs(hatch->getTotalArea());
    INFO("hatch area: " << actual << " (expected " << expected << ")");
    CHECK(std::abs(actual - expected) < 1e-6);
    // The outer and inner loops arrive as two independent containers.
    CHECK(hatch->countAllLoops() == 2);
}
