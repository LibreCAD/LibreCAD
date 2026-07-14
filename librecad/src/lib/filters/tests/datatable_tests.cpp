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
 * DWG DATATABLE (AcDbDataTable) object-body read test.
 *
 * Exercises DRW_DataTable::parseDwg via the OBJECTS-section custom-class
 * dispatch (dwgreader.cpp: rn=="DATATABLE" / className=="AcDbDataTable").
 * Before this increment DATATABLE was raw-only (addUnsupportedObject); it is
 * now structurally decoded into a DRW_DataTable and delivered via addDataTable
 * while the raw shelf is retained for a lossless round-trip.
 *
 * Fixture (copied to testdata):
 *   datatable_r2010.dwg  <- libredwg/test/test-data/2010/gh209_1.dwg (AC1024)
 *
 * Oracle: dwgread -O JSON (LibreDWG). The single DATATABLE (dwg type 520,
 * handle [0,1,145]) reports flags=2, an empty table_name and a 4-column x
 * 15-row body; column 0 is named "OriginalClassName". Those reliable-prefix
 * values are asserted below. NOTE: DATATABLE is a DEBUG_CLASS in LibreDWG
 * ("(varies) TODO") — the deeper cell layout drifts even in the oracle, so the
 * body walk is best-effort (graceful-degrade) and only the prefix is asserted.
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

// Collects every DATATABLE delivered by the OBJECTS-section dispatch.
class DataTableCapture : public StubInterface {
public:
  std::vector<DRW_DataTable> m_tables;
  void addDataTable(const DRW_DataTable &dt) override { m_tables.push_back(dt); }
};

// Returns true on a clean read; false when libdxfrw can't consume the file.
bool tryRead(const std::string &path, DataTableCapture &cap) {
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  if (!ok) return false;
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  return true;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG DATATABLE prefix decodes (gh209_1.dwg / AC1024)",
          "[dwg][datatable][parity]") {
  const std::string path = std::string(LIBRECAD_TEST_DIR) + "/datatable_r2010.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("datatable_r2010.dwg fixture not found; skipping");
    return;
  }

  DataTableCapture cap;
  // Fixture is committed (present past the is_regular_file gate), so a read
  // failure is a real regression -- REQUIRE it rather than SUCCEED-skipping.
  REQUIRE(tryRead(path, cap));

  // Exactly one DATATABLE in the fixture; the prefix is the reliable,
  // oracle-verified part of the record.
  REQUIRE(cap.m_tables.size() == 1);
  const DRW_DataTable &dt = cap.m_tables.front();

  CHECK(dt.flags == 2);          // DXF 70
  CHECK(dt.columnCount == 4);    // DXF 90 (num_cols)
  CHECK(dt.rowCount == 15);      // DXF 91 (num_rows)
  CHECK(dt.tableName.empty());   // DXF 1 (empty in the fixture)

  // The first column name decodes cleanly (same string stream the oracle
  // reads); deeper cells are DEBUG-class "(varies)" so are not asserted.
  REQUIRE(!dt.columns.empty());
  CHECK(dt.columns.front().text == "OriginalClassName");
}
