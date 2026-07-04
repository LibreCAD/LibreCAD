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
 * DXF FIELD / FIELDLIST (AcDbField / AcDbFieldList) read tests.
 *
 * The DWG read path already typed these (dwgReader OBJECTS dispatch ->
 * DRW_Field / DRW_FieldList -> addField / addFieldList); the DXF read path was
 * missing.  dxfRW::processField / processFieldList now parse the DXF group codes
 * and deliver via the same callbacks.
 *
 * The fixture below mirrors the exact group-code layout an ODA-converted
 * FIELD-rich DWG emits (blocks_and_tables_-_imperial.dwg -> DXF, ODA File
 * Converter 27.1.0): a field carries evaluatorId (1), field code (2), the
 * evaluation flag longs (91/92/94/95/96), child-field handles (360), a run of
 * per-child value sub-records (each opened by a 6/7 key, closed by 304
 * "ACVALUE_END"), then the cached value string (301) + its length (98).  The
 * fixture adds a non-empty value string and a typed child value (absent in the
 * real file's fields, which were all empty) so the value path is asserted, not
 * just present.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
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

// Captures the first FIELD and FIELDLIST delivered.
class FieldCapture : public StubInterface {
public:
  int m_fieldCount = 0;
  int m_fieldListCount = 0;
  DRW_Field m_field;
  DRW_FieldList m_list;
  void addField(const DRW_Field &d) override {
    if (m_fieldCount == 0)
      m_field = d;
    ++m_fieldCount;
  }
  void addFieldList(const DRW_FieldList &d) override {
    if (m_fieldListCount == 0)
      m_list = d;
    ++m_fieldListCount;
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

// A FIELD (evaluator "_text", field code "%<\AcVar Filename>%") that resolves to
// the cached string "drawing.dwg", plus a FIELDLIST that references it.  The
// FIELD carries one child-field handle (0x2C) and two per-child value records:
// an empty "attdef" long value and the resolved "ACFD_FIELD_VALUE" string.
const char *kFieldObjects =
    "0\nSECTION\n2\nOBJECTS\n"
    "0\nFIELD\n5\n2A\n330\n28\n100\nAcDbField\n"
    "1\n_text\n"
    "2\n%<\\AcVar Filename>%\n"
    "90\n1\n360\n2C\n"
    "97\n0\n"
    "91\n63\n92\n0\n94\n9\n95\n2\n96\n0\n300\n\n"
    "93\n2\n"
    "6\nACFD_FIELDTEXT_ATTDEF\n93\n0\n90\n1\n91\n7\n94\n0\n300\n\n302\n\n304\nACVALUE_END\n"
    "6\nACFD_FIELD_VALUE\n93\n1\n90\n4\n300\ndrawing.dwg\n94\n0\n302\n\n304\nACVALUE_END\n"
    "301\ndrawing.dwg\n98\n11\n"
    "0\nFIELDLIST\n5\n2B\n330\nC\n100\nAcDbIdSet\n90\n1\n290\n0\n330\n2A\n100\nAcDbFieldList\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF FIELD is read into a DRW_Field via processField",
          "[dxf][field]") {
  FieldCapture cap;
  readDxf(kFieldObjects, cap, "lc_field_objects.dxf");

  REQUIRE(cap.m_fieldCount == 1);
  const DRW_Field &f = cap.m_field;

  CHECK(f.handle == 0x2Au);
  CHECK(f.m_evaluatorId == "_text");
  CHECK(f.m_fieldCode == "%<\\AcVar Filename>%");
  CHECK(f.m_evaluationOptionFlags == 63);
  CHECK(f.m_filingOptionFlags == 0);
  CHECK(f.m_fieldStateFlags == 9);
  CHECK(f.m_evaluationStatusFlags == 2);
  CHECK(f.m_evaluationErrorCode == 0);

  // Cached value string + declared length (code 301 / 98).
  CHECK(f.m_valueString == "drawing.dwg");
  CHECK(f.m_valueStringLength == 11);

  // One child-field hard-owner handle (code 360).
  REQUIRE(f.m_childHandles.size() == 1u);
  CHECK(f.m_childHandles.at(0) == 0x2Cu);

  // Two per-child value records; the field-level flags (90/91/94/300) must not
  // have been clobbered by the recurring codes inside these sub-records.
  REQUIRE(f.m_childValues.size() == 2u);
  CHECK(f.m_childValues.at(0).m_key == "ACFD_FIELDTEXT_ATTDEF");
  CHECK(f.m_childValues.at(1).m_key == "ACFD_FIELD_VALUE");
  CHECK(f.m_childValues.at(1).m_value.m_dataType == 4);
  CHECK(f.m_childValues.at(1).m_value.m_valueString == "drawing.dwg");
}

TEST_CASE("DXF FIELDLIST is read into a DRW_FieldList via processFieldList",
          "[dxf][field]") {
  FieldCapture cap;
  readDxf(kFieldObjects, cap, "lc_field_objects.dxf");

  REQUIRE(cap.m_fieldListCount == 1);
  const DRW_FieldList &l = cap.m_list;

  CHECK(l.handle == 0x2Bu);
  CHECK(l.m_unknown == 0);
  // The first 330 (0xC) is the owner; the 330 after the AcDbIdSet marker (0x2A)
  // is the field handle referencing the FIELD above.
  REQUIRE(l.m_fieldHandles.size() == 1u);
  CHECK(l.m_fieldHandles.at(0) == 0x2Au);
}
