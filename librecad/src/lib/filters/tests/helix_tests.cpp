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
******************************************************************************/

/**
 * HELIX (AcDbHelix) typed read/write tests.
 *
 * A HELIX is a custom entity whose DXF body is an AcDbSpline approximation
 * followed by an AcDbHelix trailer.  LibreCAD maps it to a spline for display,
 * but libdxfrw preserves the typed HELIX data through addHelix and the direct
 * DXF/DWG writer APIs.
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "libdwgr.h"
#include "libdxfrw.h"

namespace {

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

DRW_Helix makeHelix() {
  DRW_Helix helix;
  helix.layer = "0";
  helix.flags = 0;
  helix.degree = 3;
  helix.m_scenario = 1;
  helix.m_knotParam = 15;
  helix.tolknot = 1.0e-9;
  helix.tolcontrol = 1.0e-10;
  helix.tolfit = 1.0e-9;
  helix.knotslist = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
  helix.controllist = {
      std::make_shared<DRW_Coord>(1.0, 0.0, 0.0),
      std::make_shared<DRW_Coord>(0.0, 1.0, 0.35),
      std::make_shared<DRW_Coord>(-1.0, 0.0, 0.7),
      std::make_shared<DRW_Coord>(0.0, -1.0, 1.0),
  };
  helix.m_majorVersion = 33;
  helix.m_maintVersion = 29;
  helix.axisBasePt = DRW_Coord(2.25, 13.375, 0.0);
  helix.startPt = DRW_Coord(3.60, 13.15, 0.0);
  helix.axisVector = DRW_Coord(0.0, 0.0, 1.0);
  helix.radius = 6.827831659934211;
  helix.turns = 3.0;
  helix.turnHeight = 1.0 / 3.0;
  helix.handedness = true;
  helix.constraintType = 1;
  return helix;
}

class HelixCapture : public StubInterface {
public:
  int m_helixCount = 0;
  int m_splineCount = 0;
  DRW_Helix m_captured;

  void addHelix(const DRW_Helix *helix) override {
    if (m_helixCount == 0 && helix != nullptr)
      m_captured = *helix;
    ++m_helixCount;
  }

  void addSpline(const DRW_Spline *) override {
    ++m_splineCount;
  }
};

void checkHelix(const HelixCapture &cap) {
  REQUIRE(cap.m_helixCount == 1);
  CHECK(cap.m_splineCount == 0);

  const DRW_Helix &helix = cap.m_captured;
  CHECK(helix.eType == DRW::HELIX);
  CHECK(helix.degree == 3);
  CHECK(helix.knotslist.size() == 8u);
  CHECK(helix.controllist.size() == 4u);
  CHECK(helix.controllist[1]->x == Catch::Approx(0.0));
  CHECK(helix.controllist[1]->y == Catch::Approx(1.0));
  CHECK(helix.controllist[3]->z == Catch::Approx(1.0));
  CHECK(helix.m_majorVersion == 33);
  CHECK(helix.m_maintVersion == 29);
  CHECK(helix.axisBasePt.x == Catch::Approx(2.25));
  CHECK(helix.axisBasePt.y == Catch::Approx(13.375));
  CHECK(helix.startPt.x == Catch::Approx(3.60));
  CHECK(helix.startPt.y == Catch::Approx(13.15));
  CHECK(helix.axisVector.z == Catch::Approx(1.0));
  CHECK(helix.radius == Catch::Approx(6.827831659934211));
  CHECK(helix.turns == Catch::Approx(3.0));
  CHECK(helix.turnHeight == Catch::Approx(1.0 / 3.0));
  CHECK(helix.handedness);
  CHECK(helix.constraintType == 1);
}

class HelixDxfEmitter : public StubInterface {
public:
  DRW_Helix m_helix = makeHelix();
  dxfRW *m_rw = nullptr;

  void writeEntities() override {
    REQUIRE(m_rw != nullptr);
    REQUIRE(m_rw->writeHelix(&m_helix));
  }
};

class HelixDwgEmitter : public StubInterface {
public:
  DRW_Helix m_helix = makeHelix();
  dwgRW *m_writer = nullptr;

  void writeEntities() override {
    REQUIRE(m_writer != nullptr);
    REQUIRE(m_writer->writeHelix(&m_helix));
  }
};

std::vector<DRW_Class> helixDxfClasses() {
  DRW_Class cls;
  REQUIRE(dxfRW::dxfClassForRecordName("HELIX", cls));
  cls.instanceCount = 1;
  return {cls};
}

void readDxf(const std::string &dxf, DRW_Interface &cap, const char *name) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW r(path.string().c_str());
  REQUIRE(r.read(&cap, /*ext=*/true));
  std::filesystem::remove(path);
}

std::string writeHelixDxf(const char *name) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    dxfRW w(path.string().c_str());
    w.setDxfClasses(helixDxfClasses());
    HelixDxfEmitter emitter;
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  std::ifstream in(path);
  REQUIRE(in.good());
  std::ostringstream out;
  out << in.rdbuf();
  in.close();
  std::filesystem::remove(path);
  return out.str();
}

std::string tempPath(const char *suffix) {
  return (std::filesystem::temp_directory_path() /
          (std::string("helix_") + suffix))
      .string();
}

const char *kMinimalHelixDxf =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nHELIX\n5\n7A\n100\nAcDbEntity\n8\n0\n100\nAcDbSpline\n"
    "70\n0\n71\n3\n72\n8\n73\n4\n74\n0\n42\n0.000000001\n43\n0.0000000001\n"
    "40\n0.0\n40\n0.0\n40\n0.0\n40\n0.0\n40\n1.0\n40\n1.0\n40\n1.0\n40\n1.0\n"
    "10\n1.0\n20\n0.0\n30\n0.0\n10\n0.0\n20\n1.0\n30\n0.35\n"
    "10\n-1.0\n20\n0.0\n30\n0.7\n10\n0.0\n20\n-1.0\n30\n1.0\n"
    "100\nAcDbHelix\n90\n33\n91\n29\n10\n2.25\n20\n13.375\n30\n0.0\n"
    "11\n3.60\n21\n13.15\n31\n0.0\n12\n0.0\n22\n0.0\n32\n1.0\n"
    "40\n6.827831659934211\n41\n3.0\n42\n0.3333333333333333\n290\n1\n280\n1\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF HELIX is read as typed AcDbHelix", "[dxf][helix]") {
  HelixCapture cap;
  readDxf(kMinimalHelixDxf, cap, "lc_helix_minimal_read.dxf");
  checkHelix(cap);
}

TEST_CASE("dxfRW writes HELIX with AcDbSpline and AcDbHelix subclasses",
          "[dxf][helix][write]") {
  const std::string dxf = writeHelixDxf("lc_helix_write.dxf");
  std::string normalized = dxf;
  normalized.erase(std::remove(normalized.begin(), normalized.end(), '\r'),
                   normalized.end());

  CHECK(normalized.find("\n  0\nHELIX\n") != std::string::npos);
  CHECK(normalized.find("\n  0\nCLASS\n") != std::string::npos);
  CHECK(normalized.find("\n  1\nHELIX\n") != std::string::npos);
  CHECK(normalized.find("\n  2\nAcDbHelix\n") != std::string::npos);
  CHECK(normalized.find("\n100\nAcDbSpline\n") != std::string::npos);
  CHECK(normalized.find("\n100\nAcDbHelix\n") != std::string::npos);
  CHECK(normalized.find("\n  0\nSPLINE\n") == std::string::npos);

  HelixCapture cap;
  readDxf(dxf, cap, "lc_helix_write_read.dxf");
  checkHelix(cap);
}

TEST_CASE("dwgRW writes HELIX as AcDbHelix custom class",
          "[dwg-write][helix]") {
  const DRW::Version versions[] = {
      DRW::AC1015, DRW::AC1018, DRW::AC1024, DRW::AC1027, DRW::AC1032};

  for (DRW::Version version : versions) {
    INFO("version: " << static_cast<int>(version));
    const std::string suffix =
        std::string("write_") + std::to_string(static_cast<int>(version)) + ".dwg";
    const std::string path = tempPath(suffix.c_str());
    std::filesystem::remove(path);

    {
      dwgRW writer(path.c_str());
      HelixDwgEmitter emitter;
      emitter.m_writer = &writer;
      REQUIRE(writer.write(&emitter, version, /*bin=*/false));
    }

    HelixCapture cap;
    {
      dwgR reader(path.c_str());
      REQUIRE(reader.read(&cap, /*ext=*/true));
      REQUIRE(reader.getVersion() == version);
      REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    checkHelix(cap);
    std::remove(path.c_str());
  }
}

TEST_CASE("LibreDWG 2007 HELIX DXF fixture reads through addHelix",
          "[dxf][helix][fixture]") {
  const std::string path =
      "D:/data/dli/libredwg/test/test-data/2007/Helix.dxf";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("LibreDWG 2007 Helix.dxf fixture not found; skipping");
    return;
  }

  HelixCapture cap;
  dxfRW reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  REQUIRE(cap.m_helixCount == 1);
  CHECK(cap.m_captured.degree == 3);
  CHECK(cap.m_captured.m_majorVersion == 33);
  CHECK(cap.m_captured.m_maintVersion == 29);
  CHECK(cap.m_captured.radius == Catch::Approx(6.827831659934211));
  CHECK(cap.m_captured.turns == Catch::Approx(3.0));
  CHECK(cap.m_captured.turnHeight == Catch::Approx(1.0 / 3.0));
  CHECK(cap.m_captured.handedness);
  CHECK(cap.m_captured.constraintType == 1);
}
