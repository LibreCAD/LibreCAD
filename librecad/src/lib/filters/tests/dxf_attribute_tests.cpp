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
 * DXF block-attribute tests (slice B1) — covers the DXF read path that
 * collects ATTRIB entities trailing an INSERT into DRW_Insert::attlist
 * (mirrors the POLYLINE/VERTEX/SEQEND collection pattern). Before B1 the
 * DXF parser silently dropped every ATTRIB.
 */

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "drw_entities.h"
#include "libdxfrw.h"

namespace {

// Stub satisfying every DRW_Interface pure virtual; tests override only
// what they need. Pattern lifted from mline_tests.cpp / entity_metadata_tests.cpp.
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

// Captures the first INSERT (deep-enough: attlist shared_ptrs are copied).
class InsertCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Insert m_captured;
  void addInsert(const DRW_Insert &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

// Emits a single INSERT (with whatever attlist it carries) on write.
class AttribEmitter : public StubInterface {
public:
  DRW_Insert m_insert;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeInsert(&m_insert); }
};

// Writes `dxf` to a temp file, reads it back through dxfRW into `cap`.
void readDxf(const std::string &dxf, InsertCapture &cap, const char *name) {
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

// Minimal ENTITIES-only DXF: INSERT (66=1) + one ATTRIB + SEQEND.
const char *kInsertOneAttrib =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nINSERT\n8\n0\n2\nTITLEBLK\n10\n0.0\n20\n0.0\n30\n0.0\n66\n1\n"
    "0\nATTRIB\n8\n0\n10\n1.0\n20\n2.0\n30\n0.0\n40\n0.5\n"
    "1\nACME Corp\n2\nCOMPANY\n70\n0\n"
    "0\nSEQEND\n8\n0\n"
    "0\nENDSEC\n0\nEOF\n";

// Two attributes, second flagged invisible (70=1).
const char *kInsertTwoAttribs =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nINSERT\n8\n0\n2\nTITLEBLK\n10\n5.0\n20\n5.0\n30\n0.0\n66\n1\n"
    "0\nATTRIB\n8\n0\n10\n1.0\n20\n2.0\n40\n0.5\n1\nVisVal\n2\nT1\n70\n0\n"
    "0\nATTRIB\n8\n0\n10\n1.0\n20\n3.0\n40\n0.5\n1\nHidden\n2\nT2\n70\n1\n"
    "0\nSEQEND\n8\n0\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF INSERT collects one trailing ATTRIB into attlist", "[dxf][attrib]") {
  InsertCapture cap;
  readDxf(kInsertOneAttrib, cap, "lc_attrib_one.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "TITLEBLK");
  REQUIRE(cap.m_captured.attlist.size() == 1);
  const auto &att = cap.m_captured.attlist[0];
  REQUIRE(att != nullptr);
  CHECK(att->tag == "COMPANY");
  CHECK(att->text == "ACME Corp");
  CHECK(att->basePoint.x == 1.0);
  CHECK(att->basePoint.y == 2.0);
  CHECK(att->height == 0.5);
}

TEST_CASE("DXF INSERT collects multiple ATTRIBs incl. invisible flag", "[dxf][attrib]") {
  InsertCapture cap;
  readDxf(kInsertTwoAttribs, cap, "lc_attrib_two.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_captured.attlist.size() == 2);
  CHECK(cap.m_captured.attlist[0]->tag == "T1");
  CHECK(cap.m_captured.attlist[0]->text == "VisVal");
  CHECK((cap.m_captured.attlist[0]->attribFlags & 0x1) == 0);
  CHECK(cap.m_captured.attlist[1]->tag == "T2");
  CHECK(cap.m_captured.attlist[1]->text == "Hidden");
  CHECK((cap.m_captured.attlist[1]->attribFlags & 0x1) == 1);
}

TEST_CASE("DXF INSERT without attributes yields empty attlist", "[dxf][attrib]") {
  InsertCapture cap;
  const char *noAttr =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nINSERT\n8\n0\n2\nPLAIN\n10\n0.0\n20\n0.0\n30\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(noAttr, cap, "lc_attrib_none.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "PLAIN");
  CHECK(cap.m_captured.attlist.empty());
}

TEST_CASE("DXF INSERT attlist round-trips through write+read (slice B2)",
          "[dxf][attrib][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_attrib_roundtrip.dxf";
  std::filesystem::remove(path);

  AttribEmitter emitter;
  emitter.m_insert.name = "TITLEBLK";
  emitter.m_insert.layer = "0";
  emitter.m_insert.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  {
    auto a = std::make_shared<DRW_Attrib>();
    a->layer = "0";
    a->tag = "COMPANY";
    a->text = "ACME Corp";
    a->basePoint = DRW_Coord(1.0, 2.0, 0.0);
    a->height = 0.5;
    a->attribFlags = 0;
    emitter.m_insert.attlist.push_back(a);
  }
  {
    auto a = std::make_shared<DRW_Attrib>();
    a->layer = "0";
    a->tag = "REV";
    a->text = "B";
    a->basePoint = DRW_Coord(1.0, 3.0, 0.0);
    a->height = 0.5;
    a->attribFlags = 1; // invisible
    emitter.m_insert.attlist.push_back(a);
  }

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  InsertCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_captured.attlist.size() == 2);
  CHECK(cap.m_captured.attlist[0]->tag == "COMPANY");
  CHECK(cap.m_captured.attlist[0]->text == "ACME Corp");
  CHECK(cap.m_captured.attlist[0]->basePoint.x == 1.0);
  CHECK(cap.m_captured.attlist[0]->basePoint.y == 2.0);
  CHECK((cap.m_captured.attlist[0]->attribFlags & 0x1) == 0);
  CHECK(cap.m_captured.attlist[1]->tag == "REV");
  CHECK(cap.m_captured.attlist[1]->text == "B");
  CHECK((cap.m_captured.attlist[1]->attribFlags & 0x1) == 1);

  std::filesystem::remove(path);
}
