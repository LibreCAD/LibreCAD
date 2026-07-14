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
 * SECTIONOBJECT (AcDbSection) typed-decode parity test.
 *
 * libdxfrw historically raw-shelved SECTIONOBJECT (it fell through to the
 * ProxyHostEntity / makeRawEntity raw net) while dwgTs decodes it fully. The
 * new DRW_SectionObject typed decode restores the section geometry + metadata + the
 * section_settings reference; the raw shelf is still emitted for round-trip.
 *
 * Fixture: testdata/section_object_r2018.dwg (AC1032/R2018, 40 KB, copied from
 * ~/doc/dwg6/LiveSection1.dwg). Oracle values are from `dwgread -O JSON`:
 *   state 1, flags 5, name "Section Plane (1)", vert_dir (0,0,1),
 *   top_height 5.0, bottom_height 15.0, indicator_alpha 70,
 *   num_verts 2 -> [(14.02991512351477, 6.95425112892047, 0),
 *                   (23.42940697423184, 22.94757273443976, 0)],
 *   num_blverts 0, section_settings handle 0x22A (554).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

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

// Collects every SECTIONOBJECT delivered by the ENTITY-section dispatch.
class SectionCapture : public StubInterface {
public:
  std::vector<DRW_SectionObject> m_sections;
  void addSectionObject(const DRW_SectionObject &s) override { m_sections.push_back(s); }
};

} // namespace

using Catch::Matchers::WithinAbs;

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG SECTIONOBJECT (AcDbSection) decodes to typed DRW_Section",
          "[section_object]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/section_object_r2018.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("section_object_r2018.dwg fixture absent; skipping");
    return;
  }

  SectionCapture cap;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  // LiveSection1.dwg contains exactly one SECTIONOBJECT.
  REQUIRE(cap.m_sections.size() == 1);
  const DRW_SectionObject &s = cap.m_sections.front();

  // Scalar body, matched against dwgread -O JSON.
  CHECK(s.m_state == 1u);
  CHECK(s.m_flags == 5u);
  CHECK(s.m_name == "Section Plane (1)");
  CHECK_THAT(s.m_vertDir.x, WithinAbs(0.0, 1e-9));
  CHECK_THAT(s.m_vertDir.y, WithinAbs(0.0, 1e-9));
  CHECK_THAT(s.m_vertDir.z, WithinAbs(1.0, 1e-9));
  CHECK_THAT(s.m_topHeight, WithinAbs(5.0, 1e-9));
  CHECK_THAT(s.m_bottomHeight, WithinAbs(15.0, 1e-9));
  CHECK(s.m_indicatorAlpha == 70);

  // num_verts 2, num_blverts 0.
  REQUIRE(s.m_verts.size() == 2);
  CHECK(s.m_blVerts.empty());
  CHECK_THAT(s.m_verts[0].x, WithinAbs(14.02991512351477, 1e-9));
  CHECK_THAT(s.m_verts[0].y, WithinAbs(6.95425112892047, 1e-9));
  CHECK_THAT(s.m_verts[0].z, WithinAbs(0.0, 1e-9));
  CHECK_THAT(s.m_verts[1].x, WithinAbs(23.42940697423184, 1e-9));
  CHECK_THAT(s.m_verts[1].y, WithinAbs(22.94757273443976, 1e-9));
  CHECK_THAT(s.m_verts[1].z, WithinAbs(0.0, 1e-9));

  // section_settings hard reference resolves to handle 0x22A (554).
  CHECK(s.m_sectionSettingsHandle == 0x22Au);
}
