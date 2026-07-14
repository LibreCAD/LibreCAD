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
 * DWG ACSH_*_CLASS solid-history shape-body read test.
 *
 * Exercises DRW_AcShHistoryObject::parseDwg via the OBJECTS-section custom-class
 * dispatch (dwgreader.cpp: rn.rfind("ACSH_", 0) == 0). The BOX / WEDGE /
 * SPHERE / CYLINDER / CONE shape classes decode their primitive dimensions into
 * DRW_AcShHistoryObject::m_shapeParams after the shared AcDbEvalExpr +
 * AcDbShHistoryNode prefix (matching dwgTs parseAcshBoxOrWedgeBody /
 * parseAcshSphereClass / parseAcshCylinderOrConeBody). Every other ACSH_* class
 * still routes to the shell parser and is preserved by the raw shelf.
 *
 * Fixture (in testdata):
 *   acsh_r2007.dwg  <- ~/dev/libredwg/test/test-data/2007/ATMOS-DC22S.dwg
 *                      (AC1021 / R2007; contains BOX, WEDGE, CYLINDER, ... ACSH)
 *
 * Oracle: dwgread -O JSON (LibreDWG). Baked-in dims by handle:
 *   BOX      [0,2,1101]: length 38.0, width 38.0, height 9.39007478740232
 *   WEDGE    [0,2,1091]: length 33.0, width 24.0, height 18.44832515310796
 *   CYLINDER [0,2,1076]: height 19.65275352860217, radii 18/18/18
 * major/minor for all three = 27/52.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "libdwgr.h"

namespace {

// Stub satisfying every DRW_Interface pure virtual.
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

// Collects every ACSH_* object delivered by the OBJECTS-section dispatch.
class AcShCapture : public StubInterface {
public:
  std::vector<DRW_AcShHistoryObject> m_objs;
  void addAcShHistoryObject(const DRW_AcShHistoryObject &o) override {
    m_objs.push_back(o);
  }

  const DRW_AcShHistoryObject *byHandle(std::uint32_t h) const {
    for (const auto &o : m_objs)
      if (o.handle == h) return &o;
    return nullptr;
  }

  size_t countRecord(const std::string &rn) const {
    size_t n = 0;
    for (const auto &o : m_objs)
      if (o.m_recordName == rn) ++n;
    return n;
  }
};

bool tryReadAcSh(const std::string &path, AcShCapture &cap) {
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  if (!ok) return false;
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  return true;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG ACSH_* shape classes decode primitive dims (ATMOS / AC1021)",
          "[dwg][acsh][parity]") {
  const std::string path = std::string(LIBRECAD_TEST_DIR) + "/acsh_r2007.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("acsh_r2007.dwg fixture not found; skipping");
    return;
  }

  AcShCapture cap;
  // The fixture is committed, so a read failure is a real regression -- REQUIRE
  // it (a SUCCEED-skip here would silently hide a broken AC1021 read).
  REQUIRE(tryReadAcSh(path, cap));

  // Every ACSH_* class routes to the shell parser now, so the collection is
  // populated across many class kinds.
  REQUIRE(cap.m_objs.size() >= 1);
  CHECK(cap.countRecord("ACSH_BOX_CLASS") >= 1);
  CHECK(cap.countRecord("ACSH_WEDGE_CLASS") >= 1);
  CHECK(cap.countRecord("ACSH_CYLINDER_CLASS") >= 1);

  // BOX handle 1101 -> dwgread length/width/height oracle.
  const DRW_AcShHistoryObject *box = cap.byHandle(1101);
  REQUIRE(box != nullptr);
  CHECK(box->m_recordName == "ACSH_BOX_CLASS");
  CHECK(box->m_major == 27);
  CHECK(box->m_minor == 52);
  REQUIRE(box->m_shapeParams.size() == 3);
  CHECK(box->m_shapeParams[0] == Catch::Approx(38.0));            // length
  CHECK(box->m_shapeParams[1] == Catch::Approx(38.0));            // width
  CHECK(box->m_shapeParams[2] == Catch::Approx(9.39007478740232)); // height

  // WEDGE handle 1091 -> shares the box body layout.
  const DRW_AcShHistoryObject *wedge = cap.byHandle(1091);
  REQUIRE(wedge != nullptr);
  CHECK(wedge->m_recordName == "ACSH_WEDGE_CLASS");
  REQUIRE(wedge->m_shapeParams.size() == 3);
  CHECK(wedge->m_shapeParams[0] == Catch::Approx(33.0));
  CHECK(wedge->m_shapeParams[1] == Catch::Approx(24.0));
  CHECK(wedge->m_shapeParams[2] == Catch::Approx(18.44832515310796));

  // CYLINDER handle 1076 -> height + three radii.
  const DRW_AcShHistoryObject *cyl = cap.byHandle(1076);
  REQUIRE(cyl != nullptr);
  CHECK(cyl->m_recordName == "ACSH_CYLINDER_CLASS");
  REQUIRE(cyl->m_shapeParams.size() == 4);
  CHECK(cyl->m_shapeParams[0] == Catch::Approx(19.65275352860217)); // height
  CHECK(cyl->m_shapeParams[1] == Catch::Approx(18.0));              // majorRadius
  CHECK(cyl->m_shapeParams[2] == Catch::Approx(18.0));              // minorRadius
  CHECK(cyl->m_shapeParams[3] == Catch::Approx(18.0));              // xRadius
}
