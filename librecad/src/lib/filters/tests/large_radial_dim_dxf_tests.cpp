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
 * DXF LARGE_RADIAL_DIMENSION (AcDbRadialDimensionLarge) read test.
 *
 * LARGE_RADIAL_DIMENSION is a top-level entity token (not a DIMENSION subtype),
 * so dxfRW::processEntities routes it to processLargeRadialDimension, which
 * builds a DRW_DimLargeRadial and delivers it through the existing addDimRadial
 * callback (DRW_DimLargeRadial is-a DRW_DimRadial).  The parser maps the chord
 * point (code 13) onto the radial diameter point so a plain radial consumer
 * renders center->chord, and keeps the jog point (15), overridden center (14)
 * and jog angle (40) on the object.
 *
 * The fixture is the AutoCAD group-code layout for a jogged radius dimension:
 * AcDbEntity + AcDbDimension (center at code 10) + AcDbRadialDimensionLarge
 * (chord 13, override center 14, jog point 15, jog angle 40).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "libdxfrw.h"

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

// Captures the first radial dimension and records whether it arrived as a
// DRW_DimLargeRadial (proving processLargeRadialDimension routed it).
class LargeRadialCapture : public StubInterface {
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

class LargeRadialEmitter : public StubInterface {
public:
  DRW_DimLargeRadial m_dimension = makeLargeRadialDimension();
  dxfRW *m_rw = nullptr;

  void writeEntities() override {
    REQUIRE(m_rw != nullptr);
    REQUIRE(m_rw->writeDimension(&m_dimension));
  }
};

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

std::string writeLargeRadialDxf(const char *name) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    LargeRadialEmitter emitter;
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

// AutoCAD-style jogged radius dimension.  Every point uses a DISTINCT value so
// a field mix-up (e.g. a code 10/14 swap, or chord/jog confusion) is caught:
//   center       (code 10) = (5,6,0)
//   text mid     (code 11) = (2,3,0)
//   chord        (code 13) = (10,0,0)
//   override ctr (code 14) = (7,9,0)   -- deliberately != center
//   jog point    (code 15) = (8,2,0)
//   jog angle    (code 40) = pi/4
const char *kLargeRadial =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLARGE_RADIAL_DIMENSION\n5\n2A\n330\n1F\n"
    "100\nAcDbEntity\n8\n0\n"
    "100\nAcDbDimension\n"
    "10\n5.0\n20\n6.0\n30\n0.0\n"
    "11\n2.0\n21\n3.0\n31\n0.0\n"
    "70\n4\n3\nStandard\n"
    "100\nAcDbRadialDimensionLarge\n"
    "13\n10.0\n23\n0.0\n33\n0.0\n"
    "14\n7.0\n24\n9.0\n34\n0.0\n"
    "15\n8.0\n25\n2.0\n35\n0.0\n"
    "40\n0.7853981633974483\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF LARGE_RADIAL_DIMENSION routes to addDimRadial as a DRW_DimLargeRadial",
          "[dxf][dimension][large_radial]") {
  LargeRadialCapture cap;
  readDxf(kLargeRadial, cap, "lc_large_radial.dxf");

  REQUIRE(cap.m_count == 1);
  REQUIRE(cap.m_wasLarge);

  // Center = definition point (code 10).
  CHECK(cap.m_center.x == Catch::Approx(5.0));
  CHECK(cap.m_center.y == Catch::Approx(6.0));

  // Diameter point = chord point (code 13) so center->chord renders.
  CHECK(cap.m_diameter.x == Catch::Approx(10.0));
  CHECK(cap.m_diameter.y == Catch::Approx(0.0));
  CHECK(cap.m_captured.getChordPoint().x == Catch::Approx(10.0));
  CHECK(cap.m_captured.getChordPoint().y == Catch::Approx(0.0));

  // Jog vertex (code 15) and jog angle (code 40) are preserved on the object,
  // distinct from every other point.
  CHECK(cap.m_captured.jogPoint.x == Catch::Approx(8.0));
  CHECK(cap.m_captured.jogPoint.y == Catch::Approx(2.0));
  CHECK(cap.m_captured.jogAngle == Catch::Approx(0.7853981633974483));

  // Overridden center (code 14) is captured as its OWN field with its own,
  // distinct value -- this catches a code 10/14 swap or a drop of the override.
  CHECK(cap.m_captured.overrideCenterPoint.x == Catch::Approx(7.0));
  CHECK(cap.m_captured.overrideCenterPoint.y == Catch::Approx(9.0));

  // Cross-field distinctness guards: the override center must not have been
  // aliased onto the definition point (code 10) or the chord point (code 13).
  CHECK(cap.m_captured.overrideCenterPoint.x != Catch::Approx(cap.m_center.x));
  CHECK(cap.m_captured.overrideCenterPoint.x != Catch::Approx(cap.m_diameter.x));
  CHECK(cap.m_captured.jogPoint.x != Catch::Approx(cap.m_captured.overrideCenterPoint.x));
}

TEST_CASE("DXF writer keeps LARGE_RADIAL_DIMENSION as a distinct top-level token",
          "[dxf][dimension][large_radial][write]") {
  const std::string dxf = writeLargeRadialDxf("lc_large_radial_write.dxf");
  std::string normalized = dxf;
  normalized.erase(std::remove(normalized.begin(), normalized.end(), '\r'),
                   normalized.end());
  CHECK(normalized.find("\n  0\nLARGE_RADIAL_DIMENSION\n") != std::string::npos);
  CHECK(normalized.find("\n  0\nDIMENSION\n") == std::string::npos);

  LargeRadialCapture cap;
  readDxf(dxf, cap, "lc_large_radial_write_read.dxf");

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
}
