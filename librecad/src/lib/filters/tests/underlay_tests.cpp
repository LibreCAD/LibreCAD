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
 * UNDERLAY entity tests — covers DRW_Underlay + DRW_UnderlayDefinition
 * field round-trip via in-memory DXF write+read, plus a smoke probe
 * against ~/doc/dwg3/Underlay.dwg confirming PDFUNDERLAY entities reach
 * the addUnderlay callback.
 */

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "drw_entities.h"
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
  void addWipeout(const DRW_Image *) override {}
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

class UnderlayCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Underlay m_captured;
  void addUnderlay(const DRW_Underlay *d) override {
    if (d && m_callCount == 0)
      m_captured = *d;
    ++m_callCount;
  }
};

class UnderlayEmitter : public StubInterface {
public:
  DRW_Underlay m_u;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeUnderlay(&m_u); }
};

} // namespace

TEST_CASE("DRW_Underlay: default field values", "[underlay]") {
  DRW_Underlay u;
  CHECK(u.kind == DRW_Underlay::PDF);
  CHECK(u.scale.x == 1.0);
  CHECK(u.scale.y == 1.0);
  CHECK(u.rotation == 0.0);
  CHECK(u.flags == 2); // visible, no clip
  CHECK(u.contrast == 100);
  CHECK(u.fade == 0);
  CHECK(u.definitionHandle == 0u);
  CHECK(u.clipBoundary.empty());
}

TEST_CASE("DRW_UnderlayDefinition: default field values", "[underlay]") {
  DRW_UnderlayDefinition d;
  CHECK(d.kind == DRW_UnderlayDefinition::PDF);
  CHECK(d.filename.empty());
  CHECK(d.sheetName.empty());
}

TEST_CASE("DRW_Underlay: kind enum routing copies", "[underlay]") {
  DRW_Underlay u;
  u.kind = DRW_Underlay::DGN;
  DRW_Underlay copy = u;
  CHECK(copy.kind == DRW_Underlay::DGN);
}

TEST_CASE("DXF round-trip: UNDERLAY entity preserves fields",
          "[underlay][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_underlay_roundtrip.dxf";
  std::filesystem::remove(path);

  UnderlayEmitter emitter;
  emitter.m_u.kind = DRW_Underlay::PDF;
  emitter.m_u.layer = "0";
  emitter.m_u.definitionHandle = 0xABCDu;
  emitter.m_u.position = DRW_Coord(10.0, 20.0, 0.0);
  emitter.m_u.scale = DRW_Coord(2.0, 3.0, 1.0);
  emitter.m_u.rotation = 45.0; // degrees on the wire
  emitter.m_u.flags = 0x0F;
  emitter.m_u.contrast = 75;
  emitter.m_u.fade = 25;
  emitter.m_u.clipBoundary.push_back(DRW_Coord(0.0, 0.0, 0.0));
  emitter.m_u.clipBoundary.push_back(DRW_Coord(100.0, 0.0, 0.0));
  emitter.m_u.clipBoundary.push_back(DRW_Coord(100.0, 50.0, 0.0));
  emitter.m_u.clipBoundary.push_back(DRW_Coord(0.0, 50.0, 0.0));

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  UnderlayCapture capture;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&capture, /*ext=*/true));
  }

  REQUIRE(capture.m_callCount == 1);
  CHECK(capture.m_captured.position.x == 10.0);
  CHECK(capture.m_captured.position.y == 20.0);
  CHECK(capture.m_captured.scale.x == 2.0);
  CHECK(capture.m_captured.scale.y == 3.0);
  CHECK(capture.m_captured.rotation == 45.0);
  CHECK(capture.m_captured.contrast == 75);
  CHECK(capture.m_captured.fade == 25);
  CHECK(capture.m_captured.flags == 0x0F);
  CHECK(capture.m_captured.clipBoundary.size() == 4);
  CHECK(capture.m_captured.definitionHandle == 0xABCDu);

  std::filesystem::remove(path);
}

TEST_CASE("DXF write: PDFUNDERLAY tag emitted in output",
          "[underlay][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "librecad_underlay_codes.dxf";
  std::filesystem::remove(path);

  UnderlayEmitter emitter;
  emitter.m_u.layer = "0";
  emitter.m_u.position = DRW_Coord(0.0, 0.0, 0.0);
  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  std::ifstream in(path);
  std::stringstream buf;
  buf << in.rdbuf();
  const std::string content = buf.str();
  CHECK(content.find("\nPDFUNDERLAY\n") != std::string::npos);
  CHECK(content.find("AcDbUnderlayReference") != std::string::npos);

  std::filesystem::remove(path);
}

namespace {
class UnderlayCounter : public StubInterface {
public:
  int m_underlays = 0;
  int m_underlayDefs = 0;
  void addUnderlay(const DRW_Underlay *) override { ++m_underlays; }
  void linkUnderlay(const DRW_UnderlayDefinition *) override {
    ++m_underlayDefs;
  }
};
} // namespace

// Smoke probe: hidden by [.] tag, runs against ~/doc/dwg3/Underlay.dwg
// when available. Confirms PDFUNDERLAY entities reach addUnderlay.
TEST_CASE("DWG smoke: Underlay.dwg delivers >= 3 PDFUNDERLAY entities",
          "[.dwg_underlay_probe]") {
  const char *home = getenv("HOME");
  if (!home) {
    SUCCEED("HOME not set");
    return;
  }
  const std::string path = std::string(home) + "/doc/dwg3/Underlay.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("~/doc/dwg3/Underlay.dwg not found");
    return;
  }
  UnderlayCounter capture;
  dwgR reader(path.c_str());
  bool ok = reader.read(&capture, /*ext=*/true);
  CHECK(ok);
  CHECK(capture.m_underlays >= 3);
  // Definition object is in OBJECTS section; expect at least one.
  CHECK(capture.m_underlayDefs >= 1);
}
