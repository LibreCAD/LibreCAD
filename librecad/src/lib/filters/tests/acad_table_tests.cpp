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
******************************************************************************/

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
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

class TableCapture : public StubInterface {
public:
  int m_tableCount = 0;
  int m_insertCount = 0;
  DRW_Table m_table;
  DRW_Insert m_insert;

  void addTable(const DRW_Table &table) override {
    if (m_tableCount == 0)
      m_table = table;
    ++m_tableCount;
  }

  void addInsert(const DRW_Insert &insert) override {
    if (m_insertCount == 0)
      m_insert = insert;
    ++m_insertCount;
  }
};

DRW_Table makeTable() {
  DRW_Table table;
  table.layer = "0";
  table.name = "*T1";
  table.basePoint = DRW_Coord(105.0, 145.0, 0.0);
  table.m_valueFlag = 22;
  table.m_tableStyleHandle = 0x6B;
  table.m_content.m_tableStyleHandle = 0x6B;
  table.m_horizontalDirection = DRW_Coord(1.0, 0.0, 0.0);
  table.m_hasSemanticContent = true;
  table.m_semanticContentComplete = true;
  table.m_content.m_columns.resize(3);
  table.m_content.m_columns[0].m_width = 10.0;
  table.m_content.m_columns[1].m_width = 20.0;
  table.m_content.m_columns[2].m_width = 30.0;
  table.m_content.m_rows.resize(2);
  table.m_content.m_rows[0].m_height = 11.0;
  table.m_content.m_rows[1].m_height = 9.0;
  for (auto &row : table.m_content.m_rows)
    row.m_cells.resize(3);

  const char *texts[2][3] = {{"Title", "", ""}, {"1", "2", "3"}};
  for (std::size_t row = 0; row < 2; ++row) {
    for (std::size_t column = 0; column < 3; ++column) {
      DRW_TableCellContent content;
      content.m_type = 1;
      content.m_text = texts[row][column];
      content.m_value.m_dataType = content.m_text.empty() ? 0 : 4;
      if (!content.m_text.empty())
        content.m_value.m_value.addString(1, content.m_text);
      table.m_content.m_rows[row].m_cells[column].m_contents.push_back(content);
    }
  }
  return table;
}

class TableDxfEmitter : public StubInterface {
public:
  DRW_Table m_table = makeTable();
  dxfRW *m_rw = nullptr;

  void writeEntities() override {
    REQUIRE(m_rw != nullptr);
    REQUIRE(m_rw->writeTable(&m_table));
  }
};

class TableDwgEmitter : public StubInterface {
public:
  DRW_Table m_table = makeTable();
  dwgRW *m_writer = nullptr;
  std::uint32_t m_blockRecord = 0;

  void writeBlocks() override {
    REQUIRE(m_writer != nullptr);
    m_blockRecord = m_writer->defineBlock("*T1", DRW_Coord(0.0, 0.0, 0.0));
    REQUIRE(m_blockRecord != 0);
  }

  void writeEntities() override {
    REQUIRE(m_writer != nullptr);
    m_table.blockRecH.ref = m_blockRecord;
    REQUIRE(m_writer->writeTable(&m_table));
  }
};

std::vector<DRW_Class> tableDxfClasses() {
  DRW_Class cls;
  REQUIRE(dxfRW::dxfClassForRecordName("ACAD_TABLE", cls));
  cls.instanceCount = 1;
  return {cls};
}

std::string tempPath(const char *suffix) {
  return (std::filesystem::temp_directory_path() /
          (std::string("acad_table_") + suffix))
      .string();
}

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

std::string writeTableDxf(const char *name) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    dxfRW w(path.string().c_str());
    w.setDxfClasses(tableDxfClasses());
    TableDxfEmitter emitter;
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

const DRW_TableCell& cellAt(const DRW_Table& table, std::size_t row, std::size_t column) {
  REQUIRE(row < table.m_content.m_rows.size());
  REQUIRE(column < table.m_content.m_rows[row].m_cells.size());
  return table.m_content.m_rows[row].m_cells[column];
}

UTF8STRING textAt(const DRW_Table& table, std::size_t row, std::size_t column) {
  const DRW_TableCell& cell = cellAt(table, row, column);
  REQUIRE(!cell.m_contents.empty());
  return cell.m_contents.front().m_text;
}

void checkTable(const TableCapture &cap) {
  REQUIRE(cap.m_tableCount == 1);
  CHECK(cap.m_insertCount == 0);

  const DRW_Table &table = cap.m_table;
  CHECK(table.eType == DRW::TABLE);
  CHECK(table.name == "*T1");
  CHECK(table.basePoint.x == Catch::Approx(105.0));
  CHECK(table.basePoint.y == Catch::Approx(145.0));
  CHECK(table.m_valueFlag == 22);
  CHECK(table.m_tableStyleHandle == 0x6Bu);
  CHECK(table.m_horizontalDirection.x == Catch::Approx(1.0));
  CHECK(table.m_hasSemanticContent);
  CHECK(table.m_semanticContentComplete);
  REQUIRE(table.m_content.m_rows.size() == 2u);
  REQUIRE(table.m_content.m_columns.size() == 3u);
  CHECK(table.m_content.m_rows[0].m_height == Catch::Approx(11.0));
  CHECK(table.m_content.m_rows[1].m_height == Catch::Approx(9.0));
  CHECK(table.m_content.m_columns[0].m_width == Catch::Approx(10.0));
  CHECK(table.m_content.m_columns[1].m_width == Catch::Approx(20.0));
  CHECK(table.m_content.m_columns[2].m_width == Catch::Approx(30.0));
  CHECK(textAt(table, 0, 0) == "Title");
  CHECK(textAt(table, 1, 0) == "1");
  CHECK(textAt(table, 1, 1) == "2");
  CHECK(textAt(table, 1, 2) == "3");
}

const char *kMinimalTableDxf =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nACAD_TABLE\n5\n7A\n100\nAcDbEntity\n8\n0\n"
    "100\nAcDbBlockReference\n2\n*T1\n10\n105.0\n20\n145.0\n30\n0.0\n"
    "100\nAcDbTable\n342\n6B\n11\n1.0\n21\n0.0\n31\n0.0\n"
    "90\n22\n91\n2\n92\n3\n93\n0\n94\n0\n95\n0\n96\n0\n"
    "141\n11.0\n141\n9.0\n142\n10.0\n142\n20.0\n142\n30.0\n"
    "171\n1\n172\n0\n173\n0\n174\n0\n175\n1\n176\n1\n91\n0\n178\n0\n145\n0.0\n92\n0\n"
    "301\nCELL_VALUE\n93\n2\n90\n4\n1\nTitle\n94\n0\n300\n\n302\nTitle\n304\nACVALUE_END\n"
    "171\n1\n172\n0\n173\n0\n174\n0\n175\n1\n176\n1\n91\n0\n178\n0\n145\n0.0\n92\n0\n"
    "301\nCELL_VALUE\n93\n3\n90\n0\n91\n0\n94\n0\n300\n\n302\n\n304\nACVALUE_END\n"
    "171\n1\n172\n0\n173\n0\n174\n0\n175\n1\n176\n1\n91\n0\n178\n0\n145\n0.0\n92\n0\n"
    "301\nCELL_VALUE\n93\n3\n90\n0\n91\n0\n94\n0\n300\n\n302\n\n304\nACVALUE_END\n"
    "171\n1\n172\n0\n173\n0\n174\n0\n175\n1\n176\n1\n91\n0\n178\n0\n145\n0.0\n92\n0\n"
    "301\nCELL_VALUE\n93\n2\n90\n4\n1\n1\n94\n0\n300\n\n302\n1\n304\nACVALUE_END\n"
    "171\n1\n172\n0\n173\n0\n174\n0\n175\n1\n176\n1\n91\n0\n178\n0\n145\n0.0\n92\n0\n"
    "301\nCELL_VALUE\n93\n2\n90\n4\n1\n2\n94\n0\n300\n\n302\n2\n304\nACVALUE_END\n"
    "171\n1\n172\n0\n173\n0\n174\n0\n175\n1\n176\n1\n91\n0\n178\n0\n145\n0.0\n92\n0\n"
    "301\nCELL_VALUE\n93\n2\n90\n4\n1\n3\n94\n0\n300\n\n302\n3\n304\nACVALUE_END\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("dxfRW reads ACAD_TABLE semantic rows and cells",
          "[acad_table][dxf]") {
  TableCapture cap;
  readDxf(kMinimalTableDxf, cap, "lc_acad_table_read.dxf");
  checkTable(cap);
}

TEST_CASE("dxfRW writes ACAD_TABLE with AcDbTable cells",
          "[acad_table][dxf][write]") {
  const std::string dxf = writeTableDxf("lc_acad_table_write.dxf");
  std::string normalized = dxf;
  normalized.erase(std::remove(normalized.begin(), normalized.end(), '\r'),
                   normalized.end());

  CHECK(normalized.find("\n  0\nACAD_TABLE\n") != std::string::npos);
  CHECK(normalized.find("\n100\nAcDbTable\n") != std::string::npos);
  CHECK(normalized.find("\n  0\nINSERT\n") == std::string::npos);
  CHECK(normalized.find("\n  1\nACAD_TABLE\n") != std::string::npos);

  TableCapture cap;
  readDxf(dxf, cap, "lc_acad_table_write_read.dxf");
  checkTable(cap);
}

TEST_CASE("dxfRW reads the local ezdxf ACAD_TABLE sample",
          "[acad_table][dxf][fixture]") {
  const std::filesystem::path fixture =
      "D:/data/dli/ezdxf/examples_dxf/acad_table_simple.dxf";
  if (!std::filesystem::is_regular_file(fixture)) {
    SUCCEED("ACAD_TABLE fixture not found at " << fixture.string());
    return;
  }

  TableCapture cap;
  dxfRW reader(fixture.string().c_str());
  REQUIRE(reader.read(&cap, /*ext=*/true));
  REQUIRE(cap.m_tableCount == 1);
  const DRW_Table &table = cap.m_table;
  REQUIRE(table.m_content.m_rows.size() == 3u);
  REQUIRE(table.m_content.m_columns.size() == 3u);
  CHECK(textAt(table, 0, 0) == "Title");
  CHECK(textAt(table, 1, 0) == "1");
  CHECK(textAt(table, 2, 2) == "6");
}

TEST_CASE("dwgRW writeTable degrades to INSERT fallback",
          "[acad_table][dwg-write]") {
  const std::string path = tempPath("insert_fallback.dwg");

  {
    dwgRW writer(path.c_str());
    TableDwgEmitter emitter;
    emitter.m_writer = &writer;
    // This scoped DWG writer path is the inherited INSERT fallback; native
    // AC1024+ TABLECONTENT encoding remains a separate backlog item.
    REQUIRE(writer.write(&emitter, DRW::AC1018, /*bin=*/false));
  }

  TableCapture cap;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&cap, /*ext=*/false));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
  }

  CHECK(cap.m_tableCount == 0);
  REQUIRE(cap.m_insertCount == 1);
  CHECK(cap.m_insert.name == "*T1");
  CHECK(cap.m_insert.basePoint.x == Catch::Approx(105.0));
  CHECK(cap.m_insert.basePoint.y == Catch::Approx(145.0));

  std::remove(path.c_str());
}
