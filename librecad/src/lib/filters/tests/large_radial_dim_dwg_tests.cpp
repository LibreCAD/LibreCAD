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
 * DWG LARGE_RADIAL_DIMENSION (AcDbRadialDimensionLarge) read test.
 *
 * Exercises DRW_DimLargeRadial::parseDwg via the DWG custom-class dispatch in
 * dwgReader.  Prior to the accompanying fix this parseDwg field order was
 * unexercised (the corpus scan found no LARGE_RADIAL DWG anywhere).
 *
 * Fixture provenance: a LARGE_RADIAL_DIMENSION whose AcDbRadialDimensionLarge
 * group codes are all DISTINCT -- definition point (10)=(5,6,0),
 * chord (13)=(10,0,0), override center (14)=(7,9,0), jog (15)=(8,2,0),
 * jog angle (40)=pi/4 -- was injected into a complete minimal DXF (with the
 * AcDbRadialDimensionLarge CLASS and an anonymous dimension block) and converted
 * DXF->DWG (AC1032) with the ODA File Converter 27.1.0 into
 * testdata/large_radial.dwg.
 *
 * Oracle / field-order verification.  The three subclass points are decoded in
 * the DWG body in the order (definition, JOG, jogAngle, CHORD, OVERRIDE).  This
 * was established by an ODA DXF->DWG->DXF round-trip, which preserves group
 * codes 13/14/15 exactly, cross-checked against the dwg-parser's DXF read of the
 * same drawing:
 *     definitionPoint (5,6)  chordPoint (10,0)  overrideCenter (7,9)  jog (8,2)
 * matching ezdxf's AcDbRadialDimensionLarge group-code mapping and libdxfrw's
 * own DXF parseCode.  (Note the dwg-parser's *DWG* reader labels these three
 * points as a cyclic rotation -- a bug in that read-only reference parser that
 * libdxfrw previously mirrored; hence this test asserts the DXF-consistent
 * values, not the parser's rotated DWG output.)
 *
 * The jog angle (code 40) is 0 here because the ODA converter does not carry the
 * DXF code-40 value into the DWG body (the DXF read still shows pi/4); the point
 * fields -- which are what the field-order fix is about -- are all preserved.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdio>
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

// Captures the first radial dimension delivered and records whether it arrived
// as a DRW_DimLargeRadial (proving the DWG custom-class dispatch routed it).
class LargeRadialDwgCapture : public StubInterface {
public:
  int m_count = 0;
  bool m_wasLarge = false;
  DRW_DimLargeRadial m_captured;
  DRW_Coord m_center;
  DRW_Coord m_diameter;
  void addDimRadial(const DRW_DimRadial *d) override {
    ++m_count;
    if (d) {
      m_center = d->getCenterPoint();
      m_diameter = d->getDiameterPoint();
      if (const auto *lr = dynamic_cast<const DRW_DimLargeRadial *>(d)) {
        m_wasLarge = true;
        m_captured = *lr;
      }
    }
  }
};

DRW_DimLargeRadial makeLargeRadialDimension() {
  DRW_DimLargeRadial dim;
  dim.setCenterPoint(DRW_Coord(5.0, 6.0, 0.0));
  dim.setTextPoint(DRW_Coord(2.0, 3.0, 0.0));
  dim.setChordPoint(DRW_Coord(10.0, 0.0, 0.0));
  dim.overrideCenterPoint = DRW_Coord(7.0, 9.0, 0.0);
  dim.jogPoint = DRW_Coord(8.0, 2.0, 0.0);
  dim.jogAngle = 0.7853981633974483;
  dim.type = 4;
  dim.setStyle("STANDARD");
  dim.setExtrusion(DRW_Coord(0.0, 0.0, 1.0));
  return dim;
}

class LargeRadialDwgEmitter : public StubInterface {
public:
  DRW_DimLargeRadial m_dimension = makeLargeRadialDimension();
  dwgRW *m_writer = nullptr;

  void writeEntities() override {
    REQUIRE(m_writer != nullptr);
    REQUIRE(m_writer->writeDimension(&m_dimension));
  }
};

std::string tempPath(const char *suffix) {
  return (std::filesystem::temp_directory_path() /
          (std::string("large_radial_") + suffix))
      .string();
}

} // namespace

TEST_CASE("DWG LARGE_RADIAL_DIMENSION decodes via parseDwg with DXF-consistent fields",
          "[dwg][dimension][large_radial]") {
  const std::string path = std::string(LIBRECAD_TEST_DIR) + "/large_radial.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("large_radial.dwg fixture not found; skipping");
    return;
  }

  LargeRadialDwgCapture cap;
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  REQUIRE(ok);
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // The dimension must be delivered typed as a DRW_DimLargeRadial.
  REQUIRE(cap.m_count == 1);
  REQUIRE(cap.m_wasLarge);

  // Cross-checked against the dwg-parser's DXF read of the same drawing
  // (definitionPoint 5,6 / chordPoint 10,0 / overrideCenter 7,9 / jog 8,2):

  // Definition point (code 10).
  CHECK(cap.m_center.x == Catch::Approx(5.0));
  CHECK(cap.m_center.y == Catch::Approx(6.0));

  // Chord point (code 13) == radial diameter point, so center->chord renders.
  CHECK(cap.m_diameter.x == Catch::Approx(10.0));
  CHECK(cap.m_diameter.y == Catch::Approx(0.0));
  CHECK(cap.m_captured.getChordPoint().x == Catch::Approx(10.0));
  CHECK(cap.m_captured.getChordPoint().y == Catch::Approx(0.0));

  // Overridden center (code 14).
  CHECK(cap.m_captured.overrideCenterPoint.x == Catch::Approx(7.0));
  CHECK(cap.m_captured.overrideCenterPoint.y == Catch::Approx(9.0));

  // Jog vertex (code 15).
  CHECK(cap.m_captured.jogPoint.x == Catch::Approx(8.0));
  CHECK(cap.m_captured.jogPoint.y == Catch::Approx(2.0));

  // Cross-field distinctness: each subclass point landed in its own slot (a
  // field-order rotation would collapse these onto each other).
  CHECK(cap.m_captured.getChordPoint().x != Catch::Approx(cap.m_captured.overrideCenterPoint.x));
  CHECK(cap.m_captured.getChordPoint().x != Catch::Approx(cap.m_captured.jogPoint.x));
  CHECK(cap.m_captured.overrideCenterPoint.x != Catch::Approx(cap.m_captured.jogPoint.x));

  // Jog angle (code 40): the ODA converter does not carry it into the DWG body,
  // so it decodes as 0 here (the DXF read of the same drawing shows pi/4).
  CHECK(cap.m_captured.jogAngle == Catch::Approx(0.0));
}

TEST_CASE("dwgRW writes LARGE_RADIAL_DIMENSION as AcDbRadialDimensionLarge",
          "[dwg-write][dimension][large_radial]") {
  const DRW::Version versions[] = {DRW::AC1015, DRW::AC1024, DRW::AC1032};

  for (DRW::Version version : versions) {
    INFO("version: " << static_cast<int>(version));
    const std::string suffix =
        std::string("write_") + std::to_string(static_cast<int>(version)) + ".dwg";
    const std::string path = tempPath(suffix.c_str());

    {
      dwgRW writer(path.c_str());
      LargeRadialDwgEmitter emitter;
      emitter.m_writer = &writer;
      REQUIRE(writer.write(&emitter, version, /*bin=*/false));
    }

    LargeRadialDwgCapture cap;
    {
      dwgR reader(path.c_str());
      REQUIRE(reader.read(&cap, /*ext=*/true));
      REQUIRE(reader.getVersion() == version);
      REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    REQUIRE(cap.m_count == 1);
    REQUIRE(cap.m_wasLarge);
    CHECK(cap.m_center.x == Catch::Approx(5.0));
    CHECK(cap.m_center.y == Catch::Approx(6.0));
    CHECK(cap.m_captured.getChordPoint().x == Catch::Approx(10.0));
    CHECK(cap.m_captured.overrideCenterPoint.x == Catch::Approx(7.0));
    CHECK(cap.m_captured.overrideCenterPoint.y == Catch::Approx(9.0));
    CHECK(cap.m_captured.jogPoint.x == Catch::Approx(8.0));
    CHECK(cap.m_captured.jogPoint.y == Catch::Approx(2.0));
    CHECK(cap.m_captured.jogAngle == Catch::Approx(0.7853981633974483));

    std::remove(path.c_str());
  }
}
