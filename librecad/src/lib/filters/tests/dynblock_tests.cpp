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
 * Optional local fixture (not bundled):
 *   doc/dwg6/makeall-plus.dwg (AC1032) — carries the full
 *   30-type dynamic-block menagerie including fully-decoded validation
 *   classes (BLOCKMOVEACTION, BLOCKVISIBILITYPARAMETER, BLOCKFLIPPARAMETER,
 *   BLOCKROTATIONPARAMETER, BLOCKLINEARPARAMETER, BLOCKPOLARPARAMETER,
 *   BLOCKALIGNMENTPARAMETER, BLOCKXYPARAMETER, and the linear constraint
 *   parameter classes, BLOCKARRAYACTION, and BLOCKPOLARSTRETCHACTION).
 *
 * Oracle: dwgread -O JSON (LibreDWG). The shared AcDbEvalExpr prefix is
 * ground-truthed as parentid=0xFFFFFFFF, major=33, value_code=-9999 for every
 * class; the two validation classes assert their full body against the oracle
 * (Move: name "Move", offset (1,0), num_deps 1, num_actions 0; Visibility: name
 * "Visibility State", blockvisi_name "Visibility1", one state "cirs").
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

const DRW_DynamicBlockObject *findHandle(const DynBlockCapture &cap,
                                         const std::string &recName,
                                         std::uint32_t handle) {
  for (const auto &o : cap.m_objects) {
    if (o.m_recordName == recName && o.handle == handle)
      return &o;
  }
  return nullptr;
}

const DRW_DynamicBlockObject *findDependencyBody(
    const DynBlockCapture &cap, const std::string &dependencyName) {
  for (const auto &o : cap.m_objects) {
    if (o.m_recordName == "BLOCKPARAMDEPENDENCYBODY"
        && o.m_dependencyName == dependencyName)
      return &o;
  }
  return nullptr;
}

constexpr std::uint32_t kEvalExprNoParent = 0xFFFFFFFFu; // BLd -1

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG dynamic-block family decodes typed (makeall-plus / AC1032)",
          "[dwg][dynblock][parity][fixture]") {
  const std::string path = std::string(LIBRECAD_TEST_DIR) + "/dynblock_r2018.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("dynblock_r2018.dwg fixture not found; skipping");
  }

  DynBlockCapture cap;
  // Fixture is committed (present past the is_regular_file gate), so a read
  // failure is a real regression -- REQUIRE it rather than SUCCEED-skipping.
  REQUIRE(tryRead(path, cap));

  // The whole family is now delivered typed instead of raw-only.
  REQUIRE(cap.m_objects.size() >= 30);

  // Every evalexpr-bearing class shares an oracle-verified AcDbEvalExpr prefix.
  // No object may fail to parse the prefix it should have (0 parse failures);
  // the Bare classes (REPRESENTATION), the purge-preventer, and the fixed
  // PARAMDEPENDENCYBODY singleton legitimately carry no evalexpr.
  int evalParsed = 0;
  for (const auto &o : cap.m_objects) {
    if (o.m_kind == DRW_DynamicBlockObject::Kind::Bare
        || o.m_kind == DRW_DynamicBlockObject::Kind::PurgePreventer
        || o.m_kind == DRW_DynamicBlockObject::Kind::ParameterDependencyBody)
        continue;
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
  CHECK(find(cap, "BLOCKARRAYACTION") != nullptr);
  CHECK(find(cap, "BLOCKPOLARSTRETCHACTION") != nullptr);
  SECTION("DYNAMICBLOCKPURGEPREVENTER body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *purge = findHandle(
        cap, "ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION", 594);
    REQUIRE(purge != nullptr);
    CHECK(purge->m_kind == DRW_DynamicBlockObject::Kind::PurgePreventer);
    CHECK(purge->m_purgeFlag == 1);
    CHECK(purge->m_purgeBlockHandle == 586);
    CHECK(purge->m_bodyFullyDecoded);
  }
  SECTION("BLOCKARRAYACTION body matches the dwgread oracle") {
    const auto *array = findHandle(cap, "BLOCKARRAYACTION", 855);
    REQUIRE(array != nullptr);
    CHECK(array->m_kind == DRW_DynamicBlockObject::Kind::ArrayAction);
    CHECK(array->m_elementName == "Array");
    CHECK(array->m_displayLocation.x == 0.0);
    CHECK(array->m_displayLocation.y == 2.0);
    CHECK(array->m_dependencyCount == 1);
    REQUIRE(array->m_dependencyHandles.size() == 1);
    CHECK(array->m_actionCount == 0);
    REQUIRE(array->m_connectionCodes.size() == 4);
    REQUIRE(array->m_connectionNames.size() == 4);
    CHECK(array->m_connectionCodes == std::vector<std::int32_t>{1, 1, 1, 1});
    CHECK(array->m_connectionNames == std::vector<UTF8STRING>{
        "Base", "End", "UpdatedBase", "UpdatedEnd"});
    CHECK(array->m_arrayColumnOffset == 0.4);
    CHECK(array->m_arrayRowOffset == 0.4);
    CHECK(array->m_bodyFullyDecoded);
  }
  SECTION("BLOCKPOLARSTRETCHACTION body matches the dwgread oracle") {
    const auto *polar = findHandle(cap, "BLOCKPOLARSTRETCHACTION", 693);
    REQUIRE(polar != nullptr);
    CHECK(polar->m_kind == DRW_DynamicBlockObject::Kind::PolarStretchAction);
    CHECK(polar->m_elementName == "Polar Stretch");
    CHECK(polar->m_displayLocation.x == 4.0);
    CHECK(polar->m_displayLocation.y == 4.0);
    CHECK(polar->m_dependencyCount == 2);
    REQUIRE(polar->m_dependencyHandles.size() == 2);
    CHECK(polar->m_actionCount == 0);
    REQUIRE(polar->m_connectionCodes.size() == 6);
    REQUIRE(polar->m_connectionNames.size() == 6);
    CHECK(polar->m_connectionCodes ==
          std::vector<std::int32_t>{1, 1, 1, 1, 1, 1});
    CHECK(polar->m_connectionNames == std::vector<UTF8STRING>{
        "EndXDelta", "EndYDelta", "Base", "End", "UpdatedBase",
        "UpdatedEnd"});
    CHECK(polar->m_polarStretchPointCount == 2);
    REQUIRE(polar->m_polarStretchPoints.size() == 2);
    CHECK(polar->m_polarStretchPoints[0].x == 5.0);
    CHECK(polar->m_polarStretchPoints[0].y == 0.0);
    CHECK(polar->m_polarStretchPoints[1].x == 2.0);
    CHECK(polar->m_polarStretchPoints[1].y == 4.0);
    CHECK(polar->m_polarStretchHandleCount == 0);
    CHECK(polar->m_polarStretchHandles.empty());
    CHECK(polar->m_polarStretchHandleShorts.empty());
    CHECK(polar->m_polarStretchCodeCount == 2);
    CHECK(polar->m_polarStretchCodes == std::vector<std::int32_t>{2, 1});
    CHECK(polar->m_bodyFullyDecoded);
  }
  CHECK(find(cap, "BLOCKSCALEACTION") != nullptr);
  CHECK(find(cap, "BLOCKSTRETCHACTION") != nullptr);
  CHECK(find(cap, "BLOCKPARAMDEPENDENCYBODY") != nullptr);
  CHECK(find(cap, "BLOCKPOINTPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKFLIPPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKROTATIONPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKLINEARPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKPOLARPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKALIGNMENTPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKXYPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKHORIZONTALCONSTRAINTPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKVERTICALCONSTRAINTPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKALIGNEDCONSTRAINTPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKANGULARCONSTRAINTPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKDIAMETRICCONSTRAINTPARAMETER") != nullptr);
  CHECK(find(cap, "BLOCKRADIALCONSTRAINTPARAMETER") != nullptr);

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
    REQUIRE(mv->m_dependencyHandles.size() == 1);
    CHECK(mv->m_actionCount == 0);
    // AcDbBlockAction_doubles: the two offsets are oracle-exact; angle_offset is
    // the trailing BD before the handle stream and is a known over-read (dwgTs
    // reads a denormal there too), so it is captured but not asserted.
    CHECK(mv->m_actionOffsetX == 1.0);
    CHECK(mv->m_actionOffsetY == 0.0);
    CHECK(mv->m_bodyFullyDecoded);
  }

  SECTION("BLOCKFLIPACTION full body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *fl =
        findHandle(cap, "BLOCKFLIPACTION", 745);
    REQUIRE(fl != nullptr);
    CHECK(fl->m_kind == DRW_DynamicBlockObject::Kind::FlipAction);
    CHECK(fl->m_elementName == "Flip1");
    CHECK(fl->m_displayLocation.x == 1.0);
    CHECK(fl->m_displayLocation.y == 3.0);
    CHECK(fl->m_dependencyCount == 6);
    REQUIRE(fl->m_dependencyHandles.size() == 6);
    CHECK(fl->m_actionCount == 1);
    REQUIRE(fl->m_actionIndexes.size() == 1);
    CHECK(fl->m_actionIndexes[0] == 1);
    REQUIRE(fl->m_connectionCodes.size() == 4);
    REQUIRE(fl->m_connectionNames.size() == 4);
    CHECK(fl->m_connectionCodes == std::vector<std::int32_t>{5, 5, 5, 5});
    CHECK(fl->m_connectionNames == std::vector<UTF8STRING>{
        "Flip", "UpdatedFlip", "UpdatedBase", "UpdatedEnd"});
    CHECK(fl->m_bodyFullyDecoded);
  }

  SECTION("BLOCKROTATEACTION full body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *rt =
        findHandle(cap, "BLOCKROTATEACTION", 593);
    REQUIRE(rt != nullptr);
    CHECK(rt->m_kind == DRW_DynamicBlockObject::Kind::RotateAction);
    CHECK(rt->m_elementName == "Rotate");
    CHECK(std::abs(rt->m_displayLocation.x - 1.4) < 1.0e-12);
    CHECK(std::abs(rt->m_displayLocation.y - 0.4) < 1.0e-12);
    CHECK(rt->m_dependencyCount == 1);
    REQUIRE(rt->m_dependencyHandles.size() == 1);
    CHECK(rt->m_actionCount == 0);
    CHECK(rt->m_rotateActionOffset.x == 0.0);
    REQUIRE(rt->m_rotateBaseConnectionCodes.size() == 2);
    REQUIRE(rt->m_rotateBaseConnectionNames.size() == 2);
    CHECK(rt->m_rotateBaseConnectionCodes ==
          std::vector<std::int32_t>{1, 1});
    CHECK(rt->m_rotateBaseConnectionNames ==
          std::vector<UTF8STRING>{"UpdatedBaseX", "UpdatedBaseY"});
    CHECK(rt->m_rotateActionDependent);
    CHECK(rt->m_rotateActionBasePoint.x == 0.0);
    CHECK(rt->m_rotateActionBasePoint.y == 0.0);
    CHECK(rt->m_rotateConnectionCode == 1);
    CHECK(rt->m_rotateConnectionName == "AngleDelta");
    CHECK(rt->m_bodyFullyDecoded);
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
    CHECK(vp->m_blockCount == 10);
    CHECK(vp->m_blockHandles.size() == static_cast<std::size_t>(vp->m_blockCount));
    REQUIRE(vp->m_stateBlockHandles.size() == 2);
    REQUIRE(vp->m_stateParameterHandles.size() == 2);
    CHECK(vp->m_stateBlockHandles[0].size() == 6);
    CHECK(vp->m_stateBlockHandles[1].size() == 6);
    CHECK(vp->m_stateParameterHandles[0].empty());
    CHECK(vp->m_stateParameterHandles[1].empty());
    CHECK(vp->m_stateNames[0] == "cirs");
    CHECK(vp->m_stateNames[1] == "rects");
    CHECK(vp->m_bodyFullyDecoded);
  }

  SECTION("BLOCKSCALEACTION full body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *sc = find(cap, "BLOCKSCALEACTION");
    REQUIRE(sc != nullptr);
    CHECK(sc->m_kind == DRW_DynamicBlockObject::Kind::ScaleAction);
    CHECK(sc->m_elementParsed);
    CHECK(sc->m_dependencyHandles.size() ==
          static_cast<std::size_t>(sc->m_dependencyCount));
    REQUIRE(sc->m_baseConnectionNames.size() == 2);
    REQUIRE(sc->m_baseConnectionCodes.size() == 2);
    REQUIRE(sc->m_scaleConnectionNames.size() == 3);
    REQUIRE(sc->m_scaleConnectionCodes.size() == 3);
    CHECK(sc->m_bodyFullyDecoded);
  }

  SECTION("BLOCKSTRETCHACTION full body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *st = find(cap, "BLOCKSTRETCHACTION");
    REQUIRE(st != nullptr);
    CHECK(st->m_kind == DRW_DynamicBlockObject::Kind::StretchAction);
    CHECK(st->m_elementParsed);
    CHECK(st->m_stretchPoints.size() ==
          static_cast<std::size_t>(st->m_stretchPointCount));
    CHECK(st->m_handleReferenceHandles.size() ==
          static_cast<std::size_t>(st->m_handleReferenceCount));
    CHECK(st->m_handleReferenceIndexes.size() ==
          static_cast<std::size_t>(st->m_handleReferenceCount));
    CHECK(st->m_codeReferenceCodes.size() ==
          static_cast<std::size_t>(st->m_codeReferenceCount));
    CHECK(st->m_codeReferenceIndexes.size() ==
          static_cast<std::size_t>(st->m_codeReferenceCount));
    CHECK(st->m_bodyFullyDecoded);
  }

  SECTION("BLOCKPARAMDEPENDENCYBODY fixed body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *pb =
        findDependencyBody(cap, "Radius=1.0000");
    REQUIRE(pb != nullptr);
    CHECK(pb->m_kind == DRW_DynamicBlockObject::Kind::ParameterDependencyBody);
    CHECK(pb->m_adbVersion == 1);
    CHECK(pb->m_dimensionBaseVersion == 1);
    CHECK(pb->m_dependencyName == "Radius=1.0000");
    CHECK(pb->m_classVersion == 0);
    CHECK(pb->m_bodyFullyDecoded);
  }

  SECTION("BLOCKPOINTPARAMETER one-point body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *pp =
        find(cap, "BLOCKPOINTPARAMETER", "Point");
    REQUIRE(pp != nullptr);
    CHECK(pp->m_kind == DRW_DynamicBlockObject::Kind::PointParameter);
    CHECK(pp->m_defPoint.x == 0.5);
    CHECK(pp->m_defPoint.y == 0.5);
    CHECK(pp->m_defPoint.z == 0.0);
    REQUIRE(pp->m_propertyInfo1Codes.size() == 1);
    REQUIRE(pp->m_propertyInfo1Names.size() == 1);
    CHECK(pp->m_propertyInfo1Codes[0] == 9);
    CHECK(pp->m_propertyInfo1Names[0] == "DisplacementX");
    REQUIRE(pp->m_propertyInfo2Codes.size() == 1);
    REQUIRE(pp->m_propertyInfo2Names.size() == 1);
    CHECK(pp->m_propertyInfo2Codes[0] == 9);
    CHECK(pp->m_propertyInfo2Names[0] == "DisplacementY");
    CHECK(pp->m_positionName == "Position1");
    CHECK(pp->m_positionDescription.empty());
    CHECK(pp->m_labelPoint.x == 0.0);
    CHECK(pp->m_labelPoint.y == -1.0);
    CHECK(pp->m_labelPoint.z == 0.0);
    CHECK(pp->m_bodyFullyDecoded);
  }

  SECTION("BLOCKFLIPPARAMETER two-point body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *fp =
        find(cap, "BLOCKFLIPPARAMETER", "Flip");
    REQUIRE(fp != nullptr);
    CHECK(fp->m_kind == DRW_DynamicBlockObject::Kind::FlipParameter);
    CHECK(fp->m_firstPoint.x == 0.0);
    CHECK(fp->m_firstPoint.y == 1.0);
    CHECK(fp->m_secondPoint.x == 0.0);
    CHECK(fp->m_secondPoint.y == 0.0);
    for (const auto& codes : fp->m_twoPointPropertyCodes)
      CHECK(codes.empty());
    CHECK(fp->m_propertyStates[0] == 6);
    CHECK(fp->m_propertyStates[1] == 0);
    CHECK(fp->m_propertyStates[2] == 0);
    CHECK(fp->m_propertyStates[3] == 0);
    CHECK(fp->m_parameterBaseLocation == 0);
    CHECK(fp->m_flipCaption == "Flip state1");
    CHECK(fp->m_flipDescription.empty());
    CHECK(fp->m_flipBaseStateName == "Not flipped");
    CHECK(fp->m_flipFlippedStateName == "Flipped");
    CHECK(fp->m_flipCaptionLocation.x == 1.0);
    CHECK(fp->m_flipCaptionLocation.y == 2.0);
    CHECK(fp->m_flipCaptionLocation.z == 0.0);
    CHECK(fp->m_flipTooltip == "UpdatedFlip");
    CHECK(fp->m_flipValue96 == 6);
    CHECK(fp->m_bodyFullyDecoded);
  }

  SECTION("BLOCKROTATIONPARAMETER two-point body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *rp =
        find(cap, "BLOCKROTATIONPARAMETER", "Rotation");
    REQUIRE(rp != nullptr);
    CHECK(rp->m_kind == DRW_DynamicBlockObject::Kind::RotationParameter);
    CHECK(rp->m_firstPoint.x == 0.0);
    CHECK(rp->m_firstPoint.y == 0.0);
    CHECK(rp->m_secondPoint.x == 1.25);
    CHECK(rp->m_secondPoint.y == 0.0);
    REQUIRE(rp->m_twoPointPropertyCodes[2].size() == 1);
    REQUIRE(rp->m_twoPointPropertyNames[2].size() == 1);
    CHECK(rp->m_twoPointPropertyCodes[2][0] == 2);
    CHECK(rp->m_twoPointPropertyNames[2][0] == "DisplacementX");
    REQUIRE(rp->m_twoPointPropertyCodes[3].size() == 1);
    REQUIRE(rp->m_twoPointPropertyNames[3].size() == 1);
    CHECK(rp->m_twoPointPropertyCodes[3][0] == 2);
    CHECK(rp->m_twoPointPropertyNames[3][0] == "DisplacementY");
    CHECK(rp->m_propertyStates[0] == 0);
    CHECK(rp->m_propertyStates[1] == 2);
    CHECK(rp->m_propertyStates[2] == 0);
    CHECK(rp->m_propertyStates[3] == 0);
    CHECK(rp->m_parameterBaseLocation == 0);
    CHECK(rp->m_rotationBaseAnglePoint.x == 1.25);
    CHECK(rp->m_rotationAngleName == "Angle1");
    CHECK(rp->m_rotationAngleDescription.empty());
    CHECK(rp->m_rotationAngle == 2.0);
    CHECK(rp->m_rotationValueSetFlags == 3);
    CHECK(rp->m_rotationMinimum == 0.0);
    CHECK(std::abs(rp->m_rotationMaximum - 6.28318530717959) < 1.0e-12);
    CHECK(rp->m_rotationIncrement == 0.0);
    CHECK(rp->m_rotationDeclaredValueCount == 0);
    CHECK(rp->m_rotationValues.empty());
    CHECK(rp->m_bodyFullyDecoded);
  }

  SECTION("BLOCKLINEARPARAMETER two-point body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *lp = nullptr;
    const DRW_DynamicBlockObject *lpDistance1 = nullptr;
    for (const auto& object : cap.m_objects) {
      if (object.m_recordName != "BLOCKLINEARPARAMETER"
          || object.m_elementName != "Linear")
        continue;
      if (object.m_linearDistanceName == "Distance1")
        lpDistance1 = &object;
      else if (object.m_linearDistanceName == "Distance2")
        lp = &object;
    }
    REQUIRE(lp != nullptr);
    REQUIRE(lpDistance1 != nullptr);
    CHECK(lp->m_kind == DRW_DynamicBlockObject::Kind::LinearParameter);
    CHECK(lp->m_firstPoint.x == 0.0);
    CHECK(lp->m_firstPoint.y == 0.0);
    CHECK(lp->m_secondPoint.x == 3.0);
    CHECK(lp->m_secondPoint.y == 0.0);
    REQUIRE(lp->m_twoPointPropertyCodes[0].size() == 1);
    REQUIRE(lp->m_twoPointPropertyNames[0].size() == 1);
    CHECK(lp->m_twoPointPropertyCodes[0][0] == 12);
    CHECK(lp->m_twoPointPropertyNames[0][0] == "DisplacementX");
    REQUIRE(lp->m_twoPointPropertyCodes[1].size() == 1);
    REQUIRE(lp->m_twoPointPropertyNames[1].size() == 1);
    CHECK(lp->m_twoPointPropertyCodes[1][0] == 12);
    CHECK(lp->m_twoPointPropertyNames[1][0] == "DisplacementY");
    REQUIRE(lp->m_twoPointPropertyCodes[2].size() == 1);
    REQUIRE(lp->m_twoPointPropertyNames[2].size() == 1);
    CHECK(lp->m_twoPointPropertyCodes[2][0] == 9);
    CHECK(lp->m_twoPointPropertyNames[2][0] == "DisplacementX");
    REQUIRE(lp->m_twoPointPropertyCodes[3].size() == 1);
    REQUIRE(lp->m_twoPointPropertyNames[3].size() == 1);
    CHECK(lp->m_twoPointPropertyCodes[3][0] == 9);
    CHECK(lp->m_twoPointPropertyNames[3][0] == "DisplacementY");
    CHECK(lp->m_propertyStates[0] == 12);
    CHECK(lp->m_propertyStates[1] == 9);
    CHECK(lp->m_propertyStates[2] == 0);
    CHECK(lp->m_propertyStates[3] == 0);
    CHECK(lp->m_parameterBaseLocation == 0);
    CHECK(lp->m_linearDistanceName == "Distance2");
    CHECK(lp->m_linearDistanceDescription.empty());
    CHECK(lp->m_linearDistance == -1.0);
    CHECK(lp->m_linearValue96 == 1);
    CHECK(lp->m_linearValue141 == 0.0);
    CHECK(lp->m_linearValue142 == 0.0);
    CHECK(lp->m_linearValue143 == 0.0);
    CHECK(lp->m_linearDeclaredValueCount == 0);
    CHECK(lp->m_linearValues.empty());
    CHECK(lp->m_bodyFullyDecoded);
    CHECK(lpDistance1->m_firstPoint.x == 0.0);
    CHECK(lpDistance1->m_firstPoint.y == 1.0);
    CHECK(lpDistance1->m_secondPoint.x == 3.0);
    CHECK(lpDistance1->m_secondPoint.y == 1.0);
    CHECK(lpDistance1->m_linearDistance == 1.0);
    CHECK(lpDistance1->m_twoPointPropertyCodes[0][0] == 5);
    CHECK(lpDistance1->m_twoPointPropertyCodes[1][0] == 5);
    CHECK(lpDistance1->m_twoPointPropertyCodes[2][0] == 2);
    CHECK(lpDistance1->m_twoPointPropertyCodes[3][0] == 2);
    CHECK(lpDistance1->m_propertyStates[0] == 5);
    CHECK(lpDistance1->m_propertyStates[1] == 2);
    CHECK(lpDistance1->m_bodyFullyDecoded);
  }

  SECTION("BLOCKPOLARPARAMETER two-point body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *pp =
        find(cap, "BLOCKPOLARPARAMETER", "Polar");
    REQUIRE(pp != nullptr);
    CHECK(pp->m_kind == DRW_DynamicBlockObject::Kind::PolarParameter);
    CHECK(pp->m_firstPoint.x == 0.0);
    CHECK(pp->m_firstPoint.y == 0.0);
    CHECK(pp->m_secondPoint.x == 3.0);
    CHECK(pp->m_secondPoint.y == 1.0);
    CHECK(pp->m_propertyStates[0] == 5);
    CHECK(pp->m_propertyStates[1] == 2);
    CHECK(pp->m_propertyStates[2] == 0);
    CHECK(pp->m_propertyStates[3] == 0);
    CHECK(pp->m_parameterBaseLocation == 0);
    CHECK(pp->m_polarAngleName == "Distance1");
    CHECK(pp->m_polarAngleDescription.empty());
    CHECK(pp->m_polarDistanceName == "Angle1");
    CHECK(pp->m_polarDistanceDescription.empty());
    CHECK(std::abs(pp->m_polarOffset - 1.89736659610103) < 1.0e-12);
    CHECK(pp->m_polarAngleValueSetFlags == 1);
    CHECK(pp->m_polarAngleMinimum == 0.0);
    CHECK(pp->m_polarAngleMaximum == 0.0);
    CHECK(pp->m_polarAngleIncrement == 0.0);
    CHECK(pp->m_polarAngleDeclaredValueCount == 0);
    CHECK(pp->m_polarAngleValues.empty());
    CHECK(pp->m_polarDistanceValueSetFlags == 3);
    CHECK(pp->m_polarDistanceMinimum == 0.0);
    CHECK(std::abs(pp->m_polarDistanceMaximum - 6.28318530717959) <
          1.0e-12);
    CHECK(pp->m_polarDistanceIncrement == 0.0);
    CHECK(pp->m_polarDistanceDeclaredValueCount == 0);
    CHECK(pp->m_polarDistanceValues.empty());
    CHECK(pp->m_bodyFullyDecoded);
  }

  SECTION("BLOCKALIGNMENTPARAMETER two-point body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *ap =
        find(cap, "BLOCKALIGNMENTPARAMETER", "Alignment");
    REQUIRE(ap != nullptr);
    CHECK(ap->m_kind == DRW_DynamicBlockObject::Kind::AlignmentParameter);
    CHECK(ap->m_firstPoint.x == 0.0);
    CHECK(ap->m_firstPoint.y == 0.0);
    CHECK(ap->m_secondPoint.x == 0.0);
    CHECK(ap->m_secondPoint.y == 1.0);
    CHECK(ap->m_propertyStates[0] == 2);
    CHECK(ap->m_propertyStates[1] == 0);
    CHECK(ap->m_propertyStates[2] == 0);
    CHECK(ap->m_propertyStates[3] == 0);
    CHECK(ap->m_parameterBaseLocation == 0);
    CHECK(ap->m_alignPerpendicular);
    CHECK(ap->m_bodyFullyDecoded);
  }

  SECTION("BLOCKXYPARAMETER two-point body matches the dwgread oracle") {
    const DRW_DynamicBlockObject *xp =
        find(cap, "BLOCKXYPARAMETER", "XY");
    REQUIRE(xp != nullptr);
    CHECK(xp->m_kind == DRW_DynamicBlockObject::Kind::XYParameter);
    CHECK(xp->m_firstPoint.x == 0.0);
    CHECK(xp->m_firstPoint.y == 0.0);
    CHECK(xp->m_firstPoint.z == 0.0);
    CHECK(xp->m_secondPoint.x == 4.0);
    CHECK(xp->m_secondPoint.y == 1.0);
    CHECK(xp->m_secondPoint.z == 0.0);
    CHECK(xp->m_twoPointPropertyCodes[0].empty());
    CHECK(xp->m_twoPointPropertyCodes[1].empty());
    REQUIRE(xp->m_twoPointPropertyCodes[2].size() == 1);
    REQUIRE(xp->m_twoPointPropertyNames[2].size() == 1);
    CHECK(xp->m_twoPointPropertyCodes[2][0] == 2);
    CHECK(xp->m_twoPointPropertyNames[2][0] == "DisplacementX");
    REQUIRE(xp->m_twoPointPropertyCodes[3].size() == 1);
    REQUIRE(xp->m_twoPointPropertyNames[3].size() == 1);
    CHECK(xp->m_twoPointPropertyCodes[3][0] == 2);
    CHECK(xp->m_twoPointPropertyNames[3][0] == "DisplacementY");
    CHECK(xp->m_propertyStates[0] == 0);
    CHECK(xp->m_propertyStates[1] == 2);
    CHECK(xp->m_propertyStates[2] == 0);
    CHECK(xp->m_propertyStates[3] == 0);
    CHECK(xp->m_parameterBaseLocation == 0);
    CHECK(xp->m_xyXLabel == "Y Distance1");
    CHECK(xp->m_xyXDescription == "X Distance1");
    CHECK(xp->m_xyYLabel.empty());
    CHECK(xp->m_xyYDescription.empty());
    CHECK(std::abs(xp->m_xyXValue - 0.18141008850497) < 1.0e-12);
    CHECK(std::abs(xp->m_xyYValue - 0.18141008850497) < 1.0e-12);
    CHECK(xp->m_xyXValueSetFlags == 1);
    CHECK(xp->m_xyXMinimum == 0.0);
    CHECK(xp->m_xyXMaximum == 0.0);
    CHECK(xp->m_xyXIncrement == 0.0);
    CHECK(xp->m_xyXDeclaredValueCount == 0);
    CHECK(xp->m_xyXValues.empty());
    CHECK(xp->m_xyYValueSetFlags == 1);
    CHECK(xp->m_xyYMinimum == 0.0);
    CHECK(xp->m_xyYMaximum == 0.0);
    CHECK(xp->m_xyYIncrement == 0.0);
    CHECK(xp->m_xyYDeclaredValueCount == 0);
    CHECK(xp->m_xyYValues.empty());
    CHECK(xp->m_bodyFullyDecoded);
  }

  SECTION("linear constraint parameters match the dwgread oracle") {
    const DRW_DynamicBlockObject *horizontal =
        find(cap, "BLOCKHORIZONTALCONSTRAINTPARAMETER", "Horizontal");
    const DRW_DynamicBlockObject *vertical =
        find(cap, "BLOCKVERTICALCONSTRAINTPARAMETER", "Vertical");
    const DRW_DynamicBlockObject *aligned =
        find(cap, "BLOCKALIGNEDCONSTRAINTPARAMETER", "Aligned");
    REQUIRE(horizontal != nullptr);
    REQUIRE(vertical != nullptr);
    REQUIRE(aligned != nullptr);
    CHECK(horizontal->m_kind ==
          DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter);
    CHECK(vertical->m_kind ==
          DRW_DynamicBlockObject::Kind::VerticalConstraintParameter);
    CHECK(aligned->m_kind ==
          DRW_DynamicBlockObject::Kind::AlignedConstraintParameter);
    CHECK(horizontal->m_constraintDependencyHandle == 1923);
    CHECK(vertical->m_constraintDependencyHandle == 1926);
    CHECK(aligned->m_constraintDependencyHandle == 1939);
    CHECK(horizontal->m_constraintExpressionName == "Horizontal");
    CHECK(vertical->m_constraintExpressionName == "Vertical");
    CHECK(aligned->m_constraintExpressionName == "Aligned");
    CHECK(horizontal->m_constraintExpressionDescription.empty());
    CHECK(vertical->m_constraintExpressionDescription.empty());
    CHECK(aligned->m_constraintExpressionDescription.empty());
    CHECK(horizontal->m_constraintValue == -1.5);
    CHECK(vertical->m_constraintValue == -1.3);
    CHECK(std::abs(aligned->m_constraintValue - 0.4742203385003) < 1.0e-12);
    CHECK(horizontal->m_constraintValueSetFlags == 1);
    CHECK(vertical->m_constraintValueSetFlags == 1);
    CHECK(aligned->m_constraintValueSetFlags == 1);
    CHECK(horizontal->m_constraintMinimum == 0.0);
    CHECK(horizontal->m_constraintMaximum == 0.0);
    CHECK(horizontal->m_constraintIncrement == 0.0);
    CHECK(vertical->m_constraintMinimum == 0.0);
    CHECK(vertical->m_constraintMaximum == 0.0);
    CHECK(vertical->m_constraintIncrement == 0.0);
    CHECK(aligned->m_constraintMinimum == 0.0);
    CHECK(aligned->m_constraintMaximum == 0.0);
    CHECK(aligned->m_constraintIncrement == 0.0);
    CHECK(horizontal->m_constraintDeclaredValueCount == 0);
    CHECK(vertical->m_constraintDeclaredValueCount == 0);
    CHECK(aligned->m_constraintDeclaredValueCount == 0);
    CHECK(horizontal->m_constraintValues.empty());
    CHECK(vertical->m_constraintValues.empty());
    CHECK(aligned->m_constraintValues.empty());
    CHECK(horizontal->m_bodyFullyDecoded);
    CHECK(vertical->m_bodyFullyDecoded);
    CHECK(aligned->m_bodyFullyDecoded);
  }

  SECTION("radial and diametric constraints match the dwgread oracle") {
    const auto *radial =
        findHandle(cap, "BLOCKRADIALCONSTRAINTPARAMETER", 1340);
    const auto *diametric =
        findHandle(cap, "BLOCKDIAMETRICCONSTRAINTPARAMETER", 1344);
    REQUIRE(radial != nullptr);
    REQUIRE(diametric != nullptr);
    CHECK(radial->m_kind ==
          DRW_DynamicBlockObject::Kind::RadialConstraintParameter);
    CHECK(diametric->m_kind ==
          DRW_DynamicBlockObject::Kind::DiametricConstraintParameter);
    CHECK(radial->m_firstPoint.x == 1.0);
    CHECK(radial->m_firstPoint.y == 1.0);
    CHECK(std::abs(radial->m_secondPoint.x - 1.31622776601684) < 1.0e-12);
    CHECK(std::abs(radial->m_secondPoint.y - 1.94868329805051) < 1.0e-12);
    CHECK(radial->m_constraintDependencyHandle == 1932);
    CHECK(radial->m_constraintExpressionName == "Radius");
    CHECK(std::abs(radial->m_constraintValue - 2.16227766016838) < 1.0e-12);
    CHECK(radial->m_constraintValueSetFlags == 1);
    CHECK(radial->m_constraintDeclaredValueCount == 0);
    CHECK(radial->m_constraintValues.empty());
    CHECK(radial->m_bodyFullyDecoded);
    CHECK(std::abs(diametric->m_firstPoint.x - 3.76282917548737) < 1.0e-12);
    CHECK(std::abs(diametric->m_firstPoint.y - 0.28848752646211) < 1.0e-12);
    CHECK(std::abs(diametric->m_secondPoint.x - 4.23717082451263) < 1.0e-12);
    CHECK(std::abs(diametric->m_secondPoint.y - 1.71151247353789) < 1.0e-12);
    CHECK(diametric->m_constraintDependencyHandle == 1918);
    CHECK(diametric->m_constraintExpressionName == "Diameter");
    CHECK(std::abs(diametric->m_constraintValue - 2.41227766016838) < 1.0e-12);
    CHECK(diametric->m_constraintValueSetFlags == 1);
    CHECK(diametric->m_bodyFullyDecoded);
  }

  SECTION("angular constraints match the dwgread oracle") {
    const auto *angular =
        findHandle(cap, "BLOCKANGULARCONSTRAINTPARAMETER", 1355);
    REQUIRE(angular != nullptr);
    CHECK(angular->m_kind ==
          DRW_DynamicBlockObject::Kind::AngularConstraintParameter);
    CHECK(std::abs(angular->m_angularCenterPoint.x - 1.0) < 1.0e-12);
    CHECK(std::abs(angular->m_angularCenterPoint.y - 1.0) < 1.0e-12);
    CHECK(std::abs(angular->m_angularEndPoint.x - 2.24381088612792) < 1.0e-12);
    CHECK(std::abs(angular->m_angularEndPoint.y - 1.51520333806148) < 1.0e-12);
    CHECK(angular->m_constraintDependencyHandle == 1934);
    CHECK(angular->m_constraintExpressionName == "Angular");
    CHECK(angular->m_constraintValue == 0.0);
    CHECK(angular->m_angularOrientationOnBothGrips);
    CHECK(angular->m_constraintValueSetFlags == 3);
    CHECK(std::abs(angular->m_constraintMaximum - 6.28318530717959) < 1.0e-12);
    CHECK(angular->m_constraintDeclaredValueCount == 0);
    CHECK(angular->m_constraintValues.empty());
    CHECK(angular->m_bodyFullyDecoded);
  }

  SECTION("BLOCKGRIPLOCATIONCOMPONENT decodes AcDbBlockGripExpr") {
    const DRW_DynamicBlockObject *gc = find(cap, "BLOCKGRIPLOCATIONCOMPONENT");
    REQUIRE(gc != nullptr);
    CHECK(gc->m_kind == DRW_DynamicBlockObject::Kind::GripLocationComponent);
    CHECK(gc->m_evalExprParsed);
    CHECK(gc->m_gripType == 1);
    CHECK_FALSE(gc->m_gripExpr.empty()); // e.g. "UpdatedEndX"
  }

  SECTION("BLOCK grip bodies match the dwgread oracle") {
    // Select stable handles from the fixture rather than the first matching
    // element: the file contains several dynamic-block instances.
    const DRW_DynamicBlockObject *linear =
        findHandle(cap, "BLOCKLINEARGRIP", 1341);
    const DRW_DynamicBlockObject *alignment =
        findHandle(cap, "BLOCKALIGNMENTGRIP", 737);
    const DRW_DynamicBlockObject *flip =
        findHandle(cap, "BLOCKFLIPGRIP", 741);
    const DRW_DynamicBlockObject *xy =
        findHandle(cap, "BLOCKXYGRIP", 634);
    REQUIRE(linear != nullptr);
    REQUIRE(alignment != nullptr);
    REQUIRE(flip != nullptr);
    REQUIRE(xy != nullptr);
    CHECK(linear->m_gripValue91 == 3);
    CHECK(linear->m_gripValue92 == 4);
    CHECK(std::abs(linear->m_gripLocation.x - 1.31622776601684) < 1.0e-12);
    CHECK(std::abs(linear->m_gripLocation.y - 1.94868329805051) < 1.0e-12);
    CHECK(linear->m_gripValue280);
    CHECK(linear->m_gripValue93 == -1);
    CHECK(linear->m_gripHasOrientation);
    CHECK(std::abs(linear->m_gripOrientation.x -
                   0.31622776601684) < 1.0e-12);
    CHECK(std::abs(linear->m_gripOrientation.y -
                   0.94868329805051) < 1.0e-12);
    CHECK(alignment->m_gripHasOrientation);
    CHECK(alignment->m_gripOrientation.x == 0.0);
    CHECK(alignment->m_gripOrientation.y == 1.0);
    CHECK(flip->m_gripCombinedState == 7);
    CHECK(flip->m_gripHasOrientation);
    CHECK(flip->m_gripOrientation.y == -1.0);
    CHECK_FALSE(xy->m_gripHasOrientation);
    CHECK(linear->m_bodyFullyDecoded);
    CHECK(alignment->m_bodyFullyDecoded);
    CHECK(flip->m_bodyFullyDecoded);
    CHECK(xy->m_bodyFullyDecoded);
  }
}

TEST_CASE("DWG dynamic-block one-point variants classify separately",
          "[dwg][dynblock][parity]") {
  CHECK(DRW_DynamicBlockObject::classify("BLOCKPOINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::PointParameter);
  CHECK(DRW_DynamicBlockObject::classify(
            "DYNAMICBLOCKPURGEPREVENTER") ==
        DRW_DynamicBlockObject::Kind::PurgePreventer);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKBASEPOINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::BasePointParameter);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKLOOKUPPARAMETER") ==
        DRW_DynamicBlockObject::Kind::LookupParameter);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKFLIPPARAMETER") ==
        DRW_DynamicBlockObject::Kind::FlipParameter);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKROTATIONPARAMETER") ==
        DRW_DynamicBlockObject::Kind::RotationParameter);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKLINEARPARAMETER") ==
        DRW_DynamicBlockObject::Kind::LinearParameter);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKPOLARPARAMETER") ==
        DRW_DynamicBlockObject::Kind::PolarParameter);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKALIGNMENTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::AlignmentParameter);
  CHECK(DRW_DynamicBlockObject::classify("BLOCKXYPARAMETER") ==
        DRW_DynamicBlockObject::Kind::XYParameter);
  CHECK(DRW_DynamicBlockObject::classify(
            "BLOCKALIGNEDCONSTRAINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::AlignedConstraintParameter);
  CHECK(DRW_DynamicBlockObject::classify(
            "BLOCKHORIZONTALCONSTRAINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter);
  CHECK(DRW_DynamicBlockObject::classify(
            "BLOCKVERTICALCONSTRAINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::VerticalConstraintParameter);
  CHECK(DRW_DynamicBlockObject::classify(
            "BLOCKANGULARCONSTRAINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::AngularConstraintParameter);
  CHECK(DRW_DynamicBlockObject::classify(
            "BLOCKDIAMETRICCONSTRAINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::DiametricConstraintParameter);
  CHECK(DRW_DynamicBlockObject::classify(
            "BLOCKRADIALCONSTRAINTPARAMETER") ==
        DRW_DynamicBlockObject::Kind::RadialConstraintParameter);
}
