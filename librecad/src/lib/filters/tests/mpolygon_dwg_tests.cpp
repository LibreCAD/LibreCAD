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
 * DWG MPOLYGON (AcDbMPolygon) read test.
 *
 * Exercises DRW_MPolygon::parseDwg via the DWG custom-class dispatch in
 * dwgReader (recName=="MPOLYGON" / className=="AcDbMPolygon" -> entryParse ->
 * addMPolygon).  Prior to this the parseDwg field order was unexercised (no
 * fixture existed in the corpus).
 *
 * Fixture provenance: the ezdxf-verified inline MPOLYGON group-code stream from
 * mpolygon_tests.cpp (solid fill, one 10x10 closed-polyline boundary, ACI 3) was
 * injected into a complete minimal DXF (Minimal_DXF_AC1021, itself emitted by
 * libdxfrw), a matching AcDbMPolygon CLASS entry was added, and the file was
 * converted DXF->DWG (AC1032) with the ODA File Converter 27.1.0.  The resulting
 * testdata/mpolygon_solid.dwg was confirmed with the read-only TypeScript
 * dwg-parser (cad-to-json), which reports it as class 500 / dxfName "MPOLYGON" /
 * className "AcDbMPolygon" / isEntity=true, patternName "SOLID", isSolid=true.
 *
 * This test asserts libdxfrw delivers that entity *typed* through addMPolygon
 * (not the addHatch default forward, and not the addUnsupportedObject raw route)
 * and that the decoded scalar fields agree with the fixture.
 */

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

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

// Captures MPOLYGONs delivered on the DWG read path and separately counts the
// addHatch default-forward and the addUnsupportedObject raw route so the test
// can prove the entity arrived *typed* through addMPolygon.
class MPolygonDwgCapture : public StubInterface {
public:
  int m_mpolygonCount = 0;
  int m_hatchCount = 0;
  int m_unsupportedCount = 0;
  DRW_MPolygon m_captured;
  void addMPolygon(const DRW_MPolygon *d) override {
    if (m_mpolygonCount == 0 && d)
      m_captured = *d;
    ++m_mpolygonCount;
  }
  void addHatch(const DRW_Hatch *) override { ++m_hatchCount; }
  void addUnsupportedObject(const DRW_UnsupportedObject &) override {
    ++m_unsupportedCount;
  }
};

} // namespace

TEST_CASE("DWG MPOLYGON is read into a DRW_MPolygon via parseDwg",
          "[dwg][mpolygon]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/mpolygon_solid.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("mpolygon_solid.dwg fixture not found; skipping");
    return;
  }

  MPolygonDwgCapture cap;
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  REQUIRE(ok);
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // The MPOLYGON must be delivered typed through addMPolygon -- not the addHatch
  // default forward.  That it routed to addMPolygon (count 1) is itself proof it
  // did NOT fall through to the addUnsupportedObject raw route.  (m_unsupportedCount
  // is non-zero only because the ODA-written file carries unrelated boilerplate
  // OBJECTS -- AcDbScale/VisualStyle/Material/dictionaries -- that libdxfrw keeps
  // as raw; none of them is the MPOLYGON.)
  REQUIRE(cap.m_mpolygonCount == 1);
  CHECK(cap.m_hatchCount == 0);

  // Decoded scalar fields must match the source fixture (solid SOLID fill).
  const DRW_MPolygon &mp = cap.m_captured;
  CHECK(mp.eType == DRW::MPOLYGON);
  CHECK(mp.name == "SOLID");
  CHECK(mp.solid == 1);
}
