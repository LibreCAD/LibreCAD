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
 * DWG ACSH surface-history shape-body read test (dwgTs parity, round 2).
 *
 * Exercises DRW_AcShHistoryObject::parseDwg for the shape classes whose bodies
 * decode into typed scalar fields after the shared AcDbEvalExpr +
 * AcDbShHistoryNode prefix:
 *   ACSH_BOOLEAN_CLASS   (AcDbShBoolean)
 *   ACSH_CHAMFER_CLASS   (AcDbShChamfer)
 *   ACSH_FILLET_CLASS    (AcDbShFillet)
 *   ACSH_TORUS_CLASS     (AcDbShTorus)
 *   ACSH_REVOLVE_CLASS   (AcDbShRevolve)
 *   ACSH_LOFT_CLASS      (AcDbShLoft)
 *   ACSH_EXTRUSION_CLASS (AcDbShExtrusion, shares AcDbShSweepBase)
 * matching dwgTs parseAcsh{Boolean,Chamfer,Fillet,Torus,Revolve,Loft,Extrusion}
 * Class. ACSH_BREP_CLASS is intentionally NOT deep-decoded: its body past the
 * prefix is an ACIS solid payload (dwg2.spec flags "major // also in DWG?" and
 * dwgread yields garbage major/minor), so it stays prefix-only + raw-shelved.
 *
 * Every ACSH_* object is ALSO delivered through the raw-net shelf
 * (addUnsupportedObject) — the lossless round-trip floor — which this test
 * verifies stays intact for all shapes, including BREP.
 *
 * Fixtures (already in testdata, byte-identical to ~/doc/dwg6 corpus):
 *   acsh_r2007.dwg   <- ATMOS-DC22S.dwg  (AC1021 / R2007):
 *                       BOOLEAN, BREP, CHAMFER, EXTRUSION, FILLET, TORUS
 *   dynblock_r2018.dwg <- makeall-plus.dwg (AC1032 / R2018 — R2007+ no-regress):
 *                       BOOLEAN, REVOLVE, LOFT, EXTRUSION, TORUS
 *
 * Oracle: dwgread -O JSON (LibreDWG). Values are baked in by object handle.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdint>
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

// Collects every ACSH_* object plus its raw-net shelf twin.
class AcShCapture : public StubInterface {
public:
  std::vector<DRW_AcShHistoryObject> m_objs;
  std::vector<DRW_UnsupportedObject> m_raw;

  void addAcShHistoryObject(const DRW_AcShHistoryObject &o) override {
    m_objs.push_back(o);
  }
  void addUnsupportedObject(const DRW_UnsupportedObject &o) override {
    m_raw.push_back(o);
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

  // A raw-net shelf entry exists for handle h with non-empty bytes.
  bool rawShelfIntact(std::uint32_t h) const {
    for (const auto &o : m_raw)
      if (o.m_handle == h && !o.m_rawBytes.empty()) return true;
    return false;
  }
};

bool tryReadAcSh(const std::string &path, AcShCapture &cap) {
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  if (!ok) return false;
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  return true;
}

const std::string kAtmos = std::string(LIBRECAD_TEST_DIR) + "/acsh_r2007.dwg";
const std::string kMakeall = std::string(LIBRECAD_TEST_DIR) + "/dynblock_r2018.dwg";

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG ACSH BOOLEAN/CHAMFER/FILLET/TORUS/EXTRUSION decode (ATMOS AC1021)",
          "[dwg][acsh][parity]") {
  if (!std::filesystem::is_regular_file(kAtmos)) {
    SUCCEED("acsh_r2007.dwg fixture not found; skipping");
    return;
  }
  AcShCapture cap;
  // Committed fixture -> a read failure is a real regression, not a skip.
  REQUIRE(tryReadAcSh(kAtmos, cap));
  REQUIRE(cap.m_objs.size() >= 1);

  SECTION("ACSH_BOOLEAN_CLASS handle 1297") {
    const DRW_AcShHistoryObject *o = cap.byHandle(1297);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_BOOLEAN_CLASS");
    CHECK(o->m_major == 27);
    CHECK(o->m_minor == 52);
    CHECK(o->m_operation == 2);   // subtract
    CHECK(o->m_operand1 == 1);
    CHECK(o->m_operand2 == 2);
    CHECK(cap.rawShelfIntact(1297));
  }

  SECTION("ACSH_CHAMFER_CLASS handle 1308") {
    const DRW_AcShHistoryObject *o = cap.byHandle(1308);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_CHAMFER_CLASS");
    CHECK(o->m_major == 27);
    CHECK(o->m_minor == 52);
    CHECK(o->m_bl92 == 1);
    CHECK(o->m_baseDist == Catch::Approx(10.0));
    CHECK(o->m_otherDist == Catch::Approx(10.0));
    REQUIRE(o->m_edges.size() == 1);
    CHECK(o->m_edges[0] == 151);
    CHECK(o->m_bl95 == 152);
    CHECK(cap.rawShelfIntact(1308));
  }

  SECTION("ACSH_FILLET_CLASS handle 1077") {
    const DRW_AcShHistoryObject *o = cap.byHandle(1077);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_FILLET_CLASS");
    CHECK(o->m_major == 27);
    CHECK(o->m_minor == 52);
    CHECK(o->m_bl92 == 1);
    REQUIRE(o->m_edges.size() == 1);
    CHECK(o->m_edges[0] == 113);
    REQUIRE(o->m_radiuses.size() == 1);
    CHECK(o->m_radiuses[0] == Catch::Approx(5.0));
    CHECK(o->m_endSetbacks.empty());
    REQUIRE(o->m_startSetbacks.size() == 1);
    CHECK(o->m_startSetbacks[0] == Catch::Approx(1.0));
    CHECK(cap.rawShelfIntact(1077));
  }

  SECTION("ACSH_TORUS_CLASS handle 1109") {
    const DRW_AcShHistoryObject *o = cap.byHandle(1109);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_TORUS_CLASS");
    CHECK(o->m_major == 27);
    CHECK(o->m_minor == 52);
    REQUIRE(o->m_shapeParams.size() == 2);
    CHECK(o->m_shapeParams[0] == Catch::Approx(19.0));               // major_radius
    CHECK(o->m_shapeParams[1] == Catch::Approx(2.78998345961641));   // minor_radius
    CHECK(cap.rawShelfIntact(1109));
  }

  SECTION("ACSH_EXTRUSION_CLASS handle 1113 (shares AcDbShSweepBase)") {
    const DRW_AcShHistoryObject *o = cap.byHandle(1113);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_EXTRUSION_CLASS");
    CHECK(o->m_major == 27);
    CHECK(o->m_minor == 52);
    // direction (3BD) is the reliably decodable field; the tail past it is an
    // undocumented embedded entity blob (best-effort only).
    CHECK(o->m_direction.x == Catch::Approx(-2.5071189120674));
    // y is a near-zero denormal (~-8e-16); dwgread's JSON rounds it to -0.0.
    CHECK(o->m_direction.y == Catch::Approx(0.0).margin(1e-9));
    CHECK(o->m_direction.z == Catch::Approx(-4.32600910317512));
    CHECK(cap.rawShelfIntact(1113));
  }

  SECTION("ACSH_BREP_CLASS handle 1296 stays prefix-only + raw-shelved") {
    const DRW_AcShHistoryObject *o = cap.byHandle(1296);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_BREP_CLASS");
    // BREP body is an ACIS payload — not deep-decoded — but the lossless raw
    // shelf must still preserve every byte.
    CHECK(cap.rawShelfIntact(1296));
  }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG ACSH REVOLVE/LOFT/BOOLEAN decode (makeall AC1032 / R2018)",
          "[dwg][acsh][parity]") {
  if (!std::filesystem::is_regular_file(kMakeall)) {
    SUCCEED("dynblock_r2018.dwg fixture not found; skipping");
    return;
  }
  AcShCapture cap;
  // Committed fixture -> a read failure is a real regression, not a skip.
  REQUIRE(tryReadAcSh(kMakeall, cap));
  REQUIRE(cap.m_objs.size() >= 1);

  SECTION("ACSH_REVOLVE_CLASS handle 191") {
    const DRW_AcShHistoryObject *o = cap.byHandle(191);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_REVOLVE_CLASS");
    CHECK(o->m_major == 33);
    CHECK(o->m_minor == 4);
    CHECK(o->m_axisPoint.x == Catch::Approx(11.5429));
    CHECK(o->m_axisPoint.y == Catch::Approx(0.0));
    CHECK(o->m_axisPoint.z == Catch::Approx(0.0));
    CHECK(o->m_revolveAngle == Catch::Approx(1.0));
    CHECK(o->m_startAngle == Catch::Approx(0.0));
    CHECK(o->m_draftAngle == Catch::Approx(0.0));
    CHECK(o->m_bd44 == Catch::Approx(0.0));
    CHECK(o->m_bd45 == Catch::Approx(1.0));
    CHECK(o->m_twistAngle == Catch::Approx(0.0));
    CHECK(o->m_b290 == true);
    CHECK(o->m_isCloseToAxis == true);
    // direction is a 2RD that holds a degenerate value in this makeall object;
    // decoded but not asserted (finite-but-wrong per dwgTs note).
    CHECK(cap.rawShelfIntact(191));
  }

  SECTION("ACSH_LOFT_CLASS handle 206") {
    const DRW_AcShHistoryObject *o = cap.byHandle(206);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_LOFT_CLASS");
    CHECK(o->m_major == 2);
    CHECK(o->m_minor == 35);
    CHECK(cap.rawShelfIntact(206));
  }

  SECTION("ACSH_BOOLEAN_CLASS handle 193 (R2018 path)") {
    const DRW_AcShHistoryObject *o = cap.byHandle(193);
    REQUIRE(o != nullptr);
    CHECK(o->m_recordName == "ACSH_BOOLEAN_CLASS");
    CHECK(o->m_major == 33);
    CHECK(o->m_minor == 4);
    CHECK(o->m_operation == 2);
    CHECK(o->m_operand1 == 1);
    CHECK(o->m_operand2 == 2);
    CHECK(cap.rawShelfIntact(193));
  }

  SECTION("R2007+ (AC1032) reader error stays BAD_NONE across ACSH decode") {
    // Regression guard: the deepened shape-body decode must not corrupt the
    // R2007+ handle-stream reposition. countRecord confirms shapes were seen.
    CHECK(cap.countRecord("ACSH_REVOLVE_CLASS") >= 1);
    CHECK(cap.countRecord("ACSH_LOFT_CLASS") >= 1);
  }
}
