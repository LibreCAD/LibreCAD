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
 * DXF MPOLYGON (AcDbMPolygon) read tests.
 *
 * MPOLYGON was registered in the DXF CLASS table but never dispatched, so its
 * geometry was dropped (raw-preserved only) on import.  dxfRW::processMPolygon
 * now parses it into a DRW_MPolygon (hatch-derived) and delivers it via
 * addMPolygon, which defaults to addHatch so it renders as a filled hatch.
 *
 * The fixture below is the exact group-code stream AutoCAD/ezdxf emit for a
 * solid-filled MPOLYGON with a 4-vertex closed polyline boundary (verified with
 * ezdxf 1.4.4 add_mpolygon + set_solid_fill).  Validating against a real-world
 * encoder — not a self-authored layout — keeps this from being a green==correct
 * tautology.  Note ezdxf emits the polyline-path flags as 73 (is_closed) BEFORE
 * 72 (has_bulge), which exercises the loop-flag handling in DRW_Hatch::parseCode.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
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

// Captures the first MPOLYGON delivered.  Overriding addMPolygon (rather than
// letting the default forward to addHatch) also proves the dispatch reaches the
// new callback.
class MPolygonCapture : public StubInterface {
public:
  int m_callCount = 0;
  int m_hatchFallbackCount = 0;
  DRW_MPolygon m_captured;
  void addMPolygon(const DRW_MPolygon *d) override {
    if (m_callCount == 0 && d)
      m_captured = *d;
    ++m_callCount;
  }
  void addHatch(const DRW_Hatch *) override { ++m_hatchFallbackCount; }
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

// Exact ezdxf-emitted MPOLYGON: SOLID fill (pattern name "SOLID"), one external
// polyline boundary path (flag 92==3), closed (73==1), 4 vertices forming a
// 10x10 square, fill color ACI 3.
const char *kSolidMPolygon =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nMPOLYGON\n5\n2F\n330\n17\n100\nAcDbEntity\n8\n0\n62\n2\n"
    "100\nAcDbMPolygon\n"
    "70\n1\n10\n0.0\n20\n0.0\n30\n0.0\n210\n0.0\n220\n0.0\n230\n1.0\n"
    "2\nSOLID\n71\n1\n"
    "91\n1\n"
    "92\n3\n73\n1\n72\n0\n93\n4\n"
    "10\n0.0\n20\n0.0\n"
    "10\n10.0\n20\n0.0\n"
    "10\n10.0\n20\n10.0\n"
    "10\n0.0\n20\n10.0\n"
    "76\n1\n63\n3\n11\n0.0\n21\n0.0\n99\n0\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF MPOLYGON is read into a DRW_MPolygon (solid fill, polyline boundary)",
          "[dxf][mpolygon]") {
  MPolygonCapture cap;
  readDxf(kSolidMPolygon, cap, "lc_mpolygon_solid.dxf");

  REQUIRE(cap.m_callCount == 1);
  // We override addMPolygon, so the default addHatch forward must NOT fire.
  CHECK(cap.m_hatchFallbackCount == 0);

  const DRW_MPolygon &mp = cap.m_captured;
  CHECK(mp.eType == DRW::MPOLYGON);
  CHECK(mp.solid == 1);
  CHECK(mp.name == "SOLID");
  CHECK(mp.fillColorAci == 3);
  CHECK(mp.degenerateLoops == 0);

  REQUIRE(mp.looplist.size() == 1);
  const auto &loop = mp.looplist.at(0);
  // External (1) + polyline (2) => flag 3.
  CHECK((loop->type & 2) == 2);
  REQUIRE_FALSE(loop->objlist.empty());

  auto *pl = dynamic_cast<DRW_LWPolyline *>(loop->objlist.at(0).get());
  REQUIRE(pl != nullptr);
  CHECK(pl->vertlist.size() == 4u);
  // The boundary must be closed for the hatch fill to validate.  ezdxf emits
  // 73 (closed) before 72 (bulge); this asserts the loop-flag handling keeps the
  // closed bit so RS_Hatch::validate() does not reject the area.
  CHECK((pl->flags & 1) == 1);
}
