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
 * <=AC1018 object handle-stream regression.
 *
 * seekObjectHandleStream() is a no-op for <=AC1018 (handles are inline right
 * after the object body). A parseDwg that reads the common owner/reactor/xdict
 * handles from a body-start buffer copy BEFORE reading the body therefore reads
 * garbage handles on R2000/R2004. This was fixed for EvaluationGraph (7edf47978)
 * and here for DRW_ImageDefinitionReactor (AcDbRasterImageDefReactor): the owner
 * (parentHandle) of every raster-image reactor was corrupted in pre-R2007 files.
 *
 * Fixture is developer-local (4.7 MB, hidden '.' tag): ~/doc/dwg6/Big-Blocks-CAD.dwg
 * (AC1018, 9 IMAGEDEF_REACTOR). Oracle dwgread -O JSON: reactor handle [0,3,5789]
 * has ownerhandle [8,0,0,5788] and class_version 2 (all nine reactors: owner ==
 * own handle - 1, class_version 2). Before the fix parentHandle decoded to a
 * value read out of the class_version bytes (not 5788).
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
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

// Collects every IMAGEDEF_REACTOR delivered by the OBJECTS-section dispatch.
class ReactorCapture : public StubInterface {
public:
  std::vector<DRW_ImageDefinitionReactor> m_reactors;
  void addImageDefinitionReactor(const DRW_ImageDefinitionReactor &r) override {
    m_reactors.push_back(r);
  }
};

// Collects every dynamic-block object delivered by the OBJECTS-section dispatch.
class DynBlockCapture : public StubInterface {
public:
  std::vector<DRW_DynamicBlockObject> m_objs;
  void addDynamicBlockObject(const DRW_DynamicBlockObject &o) override {
    m_objs.push_back(o);
  }
};

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG <=AC1018 IMAGEDEF_REACTOR owner handle decodes correctly",
          "[.dwg6_imagedef_reactor]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SUCCEED("HOME not set; skipping");
    return;
  }
  const std::string path =
      std::string(home) + "/doc/dwg6/Big-Blocks-CAD.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("Big-Blocks-CAD.dwg not present; skipping");
    return;
  }

  ReactorCapture cap;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // All nine reactors are delivered typed.
  REQUIRE(cap.m_reactors.size() == 9);

  // Oracle invariant (dwgread -O JSON): every AcDbRasterImageDefReactor has
  // class_version 2 and its owner handle is (own handle - 1) -- the reactor
  // sits immediately after the AcDbRasterImageDef it belongs to. This holds
  // for all nine reactors. Before the <=AC1018 handle-stream fix, parentHandle
  // was resolved from the class_version bytes (the no-op seek left the cursor at
  // body start), so this owner-relative-to-own invariant did NOT hold.
  for (const auto &r : cap.m_reactors) {
    CHECK(r.m_classVersion == 2);
    CHECK(static_cast<std::int64_t>(r.handle) - r.parentHandle == 1);
  }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG <=AC1018 dynamic-block object owner handle decodes correctly",
          "[.dwg6_dynblock_ac1015]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SUCCEED("HOME not set; skipping");
    return;
  }
  const std::string path =
      std::string(home) + "/doc/dwg6/sample_AC1015.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("sample_AC1015.dwg not present; skipping");
    return;
  }

  DynBlockCapture cap;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // dwgread -O JSON: the twelve dynamic-block definition objects
  // (BLOCK*PARAMETER/GRIP/COMPONENT/ACTION, own handles 2689..2700) are all
  // owned by handle 2688 -- the *U anonymous dynamic block record. libdxfrw
  // resolves owner handles in the same numbering here, so assert the owner
  // equals 2688 for exactly those objects. Before the <=AC1018 handle-stream
  // fix, seekObjectHandleStream was a no-op so each owner was resolved from that
  // object's own differing body bytes -> garbage, not 2688.
  REQUIRE(cap.m_objs.size() >= 12);
  int ownedBy2688 = 0;
  for (const auto &o : cap.m_objs) {
    if (o.handle >= 2689 && o.handle <= 2700) {
      ++ownedBy2688;
      CHECK(o.parentHandle == 2688);
    }
  }
  CHECK(ownedBy2688 == 12);
}
