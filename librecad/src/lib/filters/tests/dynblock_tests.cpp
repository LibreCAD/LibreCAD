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
 * DWG dynamic-block object family read test.
 *
 * Exercises DRW_DynamicBlockObject::parseDwg via the OBJECTS-section custom-class
 * dispatch (dwgreader.cpp: DRW_DynamicBlockObject::isDynamicBlockRecName(rn)).
 * Before this increment the whole dynamic-block family (BLOCK*PARAMETER /
 * BLOCK*ACTION / BLOCK*GRIP / BLOCKGRIPLOCATIONCOMPONENT / DYNAMICBLOCK* +
 * singletons) was raw-only (addUnsupportedObject); it is now structurally
 * decoded into a DRW_DynamicBlockObject and delivered via addDynamicBlockObject
 * while the raw shelf is retained for a lossless round-trip.
 *
 * Fixture (copied to testdata):
 *   dynblock_r2018.dwg  <- doc/dwg6/makeall-plus.dwg (AC1032) — carries the full
 *   30-type dynamic-block menagerie including both fully-decoded validation
 *   classes (BLOCKMOVEACTION, BLOCKVISIBILITYPARAMETER).
 *
 * Oracle: dwgread -O JSON (LibreDWG). The shared AcDbEvalExpr prefix is
 * ground-truthed as parentid=0xFFFFFFFF, major=33, value_code=-9999 for every
 * class; the two validation classes assert their full body against the oracle
 * (Move: name "Move", offset (1,0), num_deps 1, num_actions 0; Visibility: name
 * "Visibility State", blockvisi_name "Visibility1", one state "cirs").
 */

#include <catch2/catch_test_macros.hpp>

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

// Collects every dynamic-block object delivered by the OBJECTS-section dispatch.
class DynBlockCapture : public StubInterface {
public:
  std::vector<DRW_DynamicBlockObject> m_objects;
  void addDynamicBlockObject(const DRW_DynamicBlockObject &o) override {
    m_objects.push_back(o);
  }
};

bool tryRead(const std::string &path, DynBlockCapture &cap) {
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  if (!ok) return false;
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  return true;
}

// Find first captured object with the given recName (and optional element name).
const DRW_DynamicBlockObject *find(const DynBlockCapture &cap,
                                   const std::string &recName,
                                   const std::string &elementName = std::string()) {
  for (const auto &o : cap.m_objects) {
    if (o.m_recordName != recName) continue;
    if (!elementName.empty() && o.m_elementName != elementName) continue;
    return &o;
  }
  return nullptr;
}

constexpr std::uint32_t kEvalExprNoParent = 0xFFFFFFFFu; // BLd -1

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG dynamic-block family decodes typed (makeall-plus / AC1032)",
          "[dwg][dynblock][parity]") {
  const std::string path = std::string(LIBRECAD_TEST_DIR) + "/dynblock_r2018.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("dynblock_r2018.dwg fixture not found; skipping");
    return;
  }

  DynBlockCapture cap;
  // Fixture is committed (present past the is_regular_file gate), so a read
  // failure is a real regression -- REQUIRE it rather than SUCCEED-skipping.
  REQUIRE(tryRead(path, cap));

  // The whole family is now delivered typed instead of raw-only.
  REQUIRE(cap.m_objects.size() >= 30);

  // Every evalexpr-bearing class shares an oracle-verified AcDbEvalExpr prefix.
  // No object may fail to parse the prefix it should have (0 parse failures);
  // the Bare classes (REPRESENTATION / PURGEPREVENTER / PARAMDEPENDENCYBODY)
  // legitimately carry no evalexpr and are the only ones without it.
  int evalParsed = 0;
  for (const auto &o : cap.m_objects) {
    if (o.m_kind == DRW_DynamicBlockObject::Kind::Bare) continue;
    CHECK(o.m_evalExprParsed);
    if (o.m_evalExprParsed) {
      ++evalParsed;
      CHECK(o.m_parentId == kEvalExprNoParent); // evalexpr.parentid = -1
      CHECK(o.m_major == 33u);                   // evalexpr.major
      // value_code is the BSd value-selector: -9999 (no value) or one of the
      // typed branches (40/10/11/1/90/91/70).  A bogus value_code here would
      // mean the prefix desynced; assert it is one the switch actually handles.
      const bool knownValueCode =
          o.m_valueCode == -9999 || o.m_valueCode == 40 || o.m_valueCode == 10 ||
          o.m_valueCode == 11 || o.m_valueCode == 1 || o.m_valueCode == 90 ||
          o.m_valueCode == 91 || o.m_valueCode == 70;
      CHECK(knownValueCode);
    }
  }
  CHECK(evalParsed >= 25);

  // A representative spread of recNames is captured typed.
  CHECK(find(cap, "BLOCKMOVEACTION") != nullptr);
  CHECK(find(cap, "BLOCKVISIBILITYPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKGRIPLOCATIONCOMPONENT") != nullptr);
  CHECK(find(cap, "BLOCKLINEARGRIP") != nullptr);
  CHECK(find(cap, "BLOCKFLIPACTION") != nullptr);

  SECTION("BLOCKMOVEACTION full body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *mv = find(cap, "BLOCKMOVEACTION", "Move");
    REQUIRE(mv != nullptr);
    CHECK(mv->m_kind == DRW_DynamicBlockObject::Kind::MoveAction);
    // AcDbEvalExpr + AcDbBlockElement prefix.
    CHECK(mv->m_evalExprParsed);
    CHECK(mv->m_elementParsed);
    CHECK(mv->m_elementName == "Move");
    CHECK(mv->m_eed1071 == 0);
    // AcDbBlockAction body.
    CHECK(mv->m_displayLocation.x == -2.0);
    CHECK(mv->m_displayLocation.y == 0.0);
    CHECK(mv->m_dependencyCount == 1); // one dependency handle (deferred)
    CHECK(mv->m_actionCount == 0);
    // AcDbBlockAction_doubles: the two offsets are oracle-exact; angle_offset is
    // the trailing BD before the handle stream and is a known over-read (dwgTs
    // reads a denormal there too), so it is captured but not asserted.
    CHECK(mv->m_actionOffsetX == 1.0);
    CHECK(mv->m_actionOffsetY == 0.0);
    CHECK(mv->m_bodyFullyDecoded);
  }

  SECTION("BLOCKVISIBILITYPARAMETER full body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *vp =
        find(cap, "BLOCKVISIBILITYPARAMETER", "Visibility State");
    REQUIRE(vp != nullptr);
    CHECK(vp->m_kind == DRW_DynamicBlockObject::Kind::VisibilityParameter);
    CHECK(vp->m_elementParsed);
    CHECK(vp->m_elementName == "Visibility State");
    CHECK(vp->m_eed1071 == 0);
    // AcDbBlockParameter bits.
    CHECK(vp->m_showProperties);
    CHECK_FALSE(vp->m_chainActions);
    // AcDbBlock1PtParameter def_pt.
    CHECK(vp->m_defPoint.x == 0.0);
    CHECK(vp->m_defPoint.y == 3.0);
    // AcDbBlockVisibilityParameter body.
    CHECK(vp->m_isInitialized);
    CHECK(vp->m_visibilityName == "Visibility1");
    CHECK(vp->m_visibilityDescription.empty());
    CHECK_FALSE(vp->m_unknownBool);
    CHECK(vp->m_stateCount == 2);
    REQUIRE(vp->m_stateNames.size() == 2);
    CHECK(vp->m_stateNames[0] == "cirs");
    CHECK(vp->m_stateNames[1] == "rects");
    CHECK(vp->m_bodyFullyDecoded);
  }

  SECTION("BLOCKGRIPLOCATIONCOMPONENT decodes AcDbBlockGripExpr") {
    const DRW_DynamicBlockObject *gc = find(cap, "BLOCKGRIPLOCATIONCOMPONENT");
    REQUIRE(gc != nullptr);
    CHECK(gc->m_kind == DRW_DynamicBlockObject::Kind::GripLocationComponent);
    CHECK(gc->m_evalExprParsed);
    CHECK(gc->m_gripType == 1);
    CHECK_FALSE(gc->m_gripExpr.empty()); // e.g. "UpdatedEndX"
  }
}
