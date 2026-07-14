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
 * DWG LEADER / BLKREF OBJECTCONTEXTDATA structured-decode read test.
 *
 * Exercises DRW_ObjectContextData::parseDwg's Kind::Leader and
 * Kind::BlockReference branches, reached via the widened OBJECTS-section
 * dispatch (dwgreader.cpp objectContextKindFromClassNames):
 *   LEADEROBJECTCONTEXTDATA          -> Kind::Leader
 *   BLKREFOBJECTCONTEXTDATA          -> Kind::BlockReference
 * mirroring dwgTs parseLeaderObjectContextData / parseBlockReferenceObjectContextData.
 * The typed record is delivered via addObjectContextData while the raw DWG
 * bytes are still emitted (addUnsupportedObject) for lossless replay.
 *
 * Fixtures (in testdata):
 *   ocd_leader_r2000.dwg   <- ~/dev/libredwg/test/test-data/2000/Leader.dwg (AC1015)
 *       one LEADEROBJECTCONTEXTDATA: classVersion 3, default flag set, 3 points,
 *       xDirection (1,0,0), endpointProjection (0,-0.09,0).
 *   dynblock_r2018.dwg     <- ~/doc/dwg6/makeall-plus.dwg
 *       (shared AC1032 kitchen-sink fixture; also used by the dynamic-block and
 *        EVALUATION_GRAPH suites)
 *       two BLKREFOBJECTCONTEXTDATA: classVersion 4, rotation 0, insertion
 *       (26,24,0), scales (2,2,2) and (4,4,4).
 *
 * Oracle: LibreDWG dwgread -O JSON. Values asserted below are the exact
 * decoded oracle values for these objects.
 */

#include <catch2/catch_test_macros.hpp>

#include <cmath>
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

// Collects every OBJECTCONTEXTDATA object delivered by the dispatch.
class ContextCapture : public StubInterface {
public:
  std::vector<DRW_ObjectContextData> m_ctx;
  void addObjectContextData(const DRW_ObjectContextData &d) override {
    m_ctx.push_back(d);
  }

  std::vector<const DRW_ObjectContextData *>
  ofKind(DRW_ObjectContextData::Kind k) const {
    std::vector<const DRW_ObjectContextData *> v;
    for (const auto &c : m_ctx)
      if (c.m_kind == k)
        v.push_back(&c);
    return v;
  }
};

bool approx(double a, double b, double eps = 1e-6) {
  return std::fabs(a - b) <= eps;
}

// Reads a fixture; SUCCEED-skips if missing or the read fails. On success,
// asserts a clean read with no stream desync.
bool tryRead(const std::string &path, ContextCapture &cap) {
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("fixture not found; skipping: " << path);
    return false;
  }
  dwgR reader(path.c_str());
  // Fixture is present (passed the is_regular_file gate above), so a read
  // failure is a real regression -- REQUIRE it, do not SUCCEED-skip.
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  CHECK(reader.getEntityParseFailures() == 0u);
  CHECK(reader.getObjectParseFailures() == 0u);
  return true;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG LEADEROBJECTCONTEXTDATA structured decode (Leader / AC1015)",
          "[dwg][objectcontext][leader][parity]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/ocd_leader_r2000.dwg";
  ContextCapture cap;
  if (!tryRead(path, cap))
    return;

  const auto leaders = cap.ofKind(DRW_ObjectContextData::Kind::Leader);
  REQUIRE(leaders.size() >= 1);
  const DRW_ObjectContextData *l = leaders.front();

  // Common context preamble (oracle: class_version 3, is_default 1).
  CHECK(l->m_classVersion == 3);
  CHECK(l->m_defaultFlag == true);

  // Body: 3 leader points (oracle).
  REQUIRE(l->m_leaderPoints.size() == 3u);
  CHECK(approx(l->m_leaderPoints[0].x, 9.57446797349922));
  CHECK(approx(l->m_leaderPoints[0].y, 10.0703252287432));
  CHECK(approx(l->m_leaderPoints[1].x, 12.43818642667787));
  CHECK(approx(l->m_leaderPoints[1].y, 12.75399576363243));
  CHECK(approx(l->m_leaderPoints[2].x, 15.39139605215153));
  CHECK(approx(l->m_leaderPoints[2].y, 12.75399576363243));

  CHECK(l->m_leaderUnknown290 == false);         // b290 = 0
  CHECK(approx(l->m_leaderXDir.x, 1.0));          // x_direction (1,0,0)
  CHECK(approx(l->m_leaderXDir.y, 0.0));
  CHECK(approx(l->m_leaderInsertionOffset.x, 0.0));  // inspt_offset (0,0,0)
  CHECK(approx(l->m_leaderInsertionOffset.y, 0.0));
  CHECK(approx(l->m_leaderEndpointProjection.x, 0.0));   // endptproj (0,-0.09,0)
  CHECK(approx(l->m_leaderEndpointProjection.y, -0.09));

  // Handle stream still anchors (scale link resolved, oracle handle 183).
  CHECK(l->m_scaleHandle == 183u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG BLKREFOBJECTCONTEXTDATA structured decode (makeall-plus)",
          "[dwg][objectcontext][blkref][parity]") {
  // Shares the AC1032 makeall-plus kitchen-sink fixture with the dynamic-block
  // and EVALUATION_GRAPH suites (all three exercise the same file).
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/dynblock_r2018.dwg";
  ContextCapture cap;
  if (!tryRead(path, cap))
    return;

  const auto blkrefs = cap.ofKind(DRW_ObjectContextData::Kind::BlockReference);
  REQUIRE(blkrefs.size() >= 2);

  // Both oracle instances: classVersion 4, rotation 0, insertion (26,24,0).
  bool sawScale2 = false;
  bool sawScale4 = false;
  for (const DRW_ObjectContextData *b : blkrefs) {
    CHECK(b->m_classVersion == 4);
    CHECK(approx(b->m_blkRefRotation, 0.0));
    CHECK(approx(b->m_blkRefInsertionPoint.x, 26.0));
    CHECK(approx(b->m_blkRefInsertionPoint.y, 24.0));
    CHECK(approx(b->m_blkRefInsertionPoint.z, 0.0));
    // Uniform scale factors in the fixture.
    CHECK(approx(b->m_blkRefScale.x, b->m_blkRefScale.y));
    CHECK(approx(b->m_blkRefScale.y, b->m_blkRefScale.z));
    if (approx(b->m_blkRefScale.x, 2.0))
      sawScale2 = true;
    if (approx(b->m_blkRefScale.x, 4.0))
      sawScale4 = true;
  }
  CHECK(sawScale2);  // instance 1: scale_factor (2,2,2)
  CHECK(sawScale4);  // instance 2: scale_factor (4,4,4)
}
