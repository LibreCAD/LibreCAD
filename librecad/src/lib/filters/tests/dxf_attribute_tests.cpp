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
 * DXF entity read/write tests for newly-wired types.
 *   - slices B1/B2: ATTRIB collection on INSERT (DRW_Insert::attlist) +
 *     write-back (mirrors the POLYLINE/VERTEX/SEQEND pattern).
 *   - slice E1: TOLERANCE (AcDbFcf) read dispatch + codec write.
 * Before these slices the DXF parser silently dropped these entities.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
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

// Captures the first HATCH entity.
class HatchCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Hatch m_captured;
  void addHatch(const DRW_Hatch *d) override {
    if (m_callCount == 0)
      m_captured = *d;
    ++m_callCount;
  }
};

// Emits a single HATCH on write.
class HatchEmitter : public StubInterface {
public:
  DRW_Hatch m_hatch;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeHatch(&m_hatch); }
};

// Captures addImage / addWipeout (both call through DRW_Image*).
class ImageCapture : public StubInterface {
public:
  int m_imageCount = 0;
  int m_wipeoutCount = 0;
  DRW_Image m_lastImage;
  DRW_Image m_lastWipeout;
  void addImage(const DRW_Image *d) override {
    m_lastImage = *d;
    ++m_imageCount;
  }
  void addWipeout(const DRW_Wipeout *d) override {
    m_lastWipeout = *d;
    ++m_wipeoutCount;
  }
};

// Emits a single WIPEOUT with a triangular clip path.
class WipeoutEmitter : public StubInterface {
public:
  DRW_Wipeout m_wipeout;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeWipeout(&m_wipeout); }
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

class LineCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Line m_captured;
  void addLine(const DRW_Line &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class SurfaceCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Surface m_captured;
  void addSurface(const DRW_Surface *d) override {
    if (m_callCount == 0)
      m_captured = *d;
    ++m_callCount;
  }
};

class ModelerGeometryCapture : public StubInterface {
public:
  std::vector<DRW_ModelerGeometry> m_items;
  void addModelerGeometry(const DRW_ModelerGeometry &d) override {
    m_items.push_back(d);
  }
};

class ModelerGeometryEmitter : public StubInterface {
public:
  std::vector<DRW_ModelerGeometry> m_items;
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    for (DRW_ModelerGeometry &item : m_items)
      m_rw->writeModelerGeometry(&item);
  }
};

// Emits a single INSERT (with whatever attlist it carries) on write.
class AttribEmitter : public StubInterface {
public:
  DRW_Insert m_insert;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeInsert(&m_insert); }
};

class AttdefEmitter : public StubInterface {
public:
  DRW_Attdef m_attdef;
  bool m_viaAttrib = false;
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    if (m_viaAttrib)
      m_rw->writeAttrib(&m_attdef);
    else
      m_rw->writeAttdef(&m_attdef);
  }
};

class RawEntityCapture : public StubInterface {
public:
  std::vector<DRW_RawDxfObject> m_entities;
  void addRawDxfEntity(const DRW_RawDxfObject &d) override {
    m_entities.push_back(d);
  }
};

// Emits a single LINE on write (for the thickness/extrusion field-drop test).
class LineEmitter : public StubInterface {
public:
  DRW_Line m_line;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeLine(&m_line); }
};

// Captures the first TOLERANCE entity.
class ToleranceCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Tolerance m_captured;
  void addTolerance(const DRW_Tolerance &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

// Emits a single TOLERANCE on write.
class ToleranceEmitter : public StubInterface {
public:
  DRW_Tolerance m_tol;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeTolerance(&m_tol); }
};

// Writes `dxf` to a temp file, reads it back through dxfRW into `cap`.
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

bool hasStringGroup(const DRW_RawDxfObject &obj, int code, const char *value) {
  for (const DRW_Variant &group : obj.groups) {
    if (group.code() == code && group.type() == DRW_Variant::STRING &&
        std::string(group.c_str()) == value) {
      return true;
    }
  }
  return false;
}

bool hasIntGroup(const DRW_RawDxfObject &obj, int code, int value) {
  for (const DRW_Variant &group : obj.groups) {
    if (group.code() == code && group.type() == DRW_Variant::INTEGER &&
        group.i_val() == value) {
      return true;
    }
  }
  return false;
}

std::vector<std::uint8_t> bytesOf(const std::string &text) {
  return std::vector<std::uint8_t>(text.begin(), text.end());
}

std::vector<std::uint8_t> modelerPayload(const std::string &prefix,
                                         std::size_t size) {
  std::vector<std::uint8_t> bytes = bytesOf(prefix);
  for (std::size_t i = bytes.size(); i < size; ++i)
    bytes.push_back(static_cast<std::uint8_t>((i * 17u) & 0xffu));
  return bytes;
}

void addXData(DRW_Entity &entity, const char *payload) {
  entity.extData.push_back(
      std::make_shared<DRW_Variant>(1001, std::string{"MODELAPP"}));
  entity.extData.push_back(
      std::make_shared<DRW_Variant>(1000, std::string{payload}));
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

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF BLOCK first entity keeps first data group", "[dxf][block]") {
  LineCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n20\n8\nBlockLayer\n2\nB1\n70\n0\n10\n0\n20\n0\n30\n0\n"
      "0\nLINE\n5\n21\n8\nInnerLayer\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDBLK\n5\n22\n8\nBlockLayer\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_block_first_entity.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x21u);
  CHECK(cap.m_captured.layer == "InnerLayer");
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF entity 102 reactor handles do not overwrite owner", "[dxf][reactor]") {
  LineCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n21\n330\nC\n102\n{ACAD_REACTORS\n330\nD\n102\n}\n"
      "8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_entity_102_owner.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.parentHandle == 0xCu);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF malformed ASCII group code is rejected", "[dxf][malformed]") {
  const auto path = std::filesystem::temp_directory_path() / "lc_bad_group_code.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\nBAD\nLINE\n0\nENDSEC\n0\nEOF\n";
  }

  StubInterface cap;
  dxfRW r(path.string().c_str());
  CHECK_FALSE(r.read(&cap, /*ext=*/true));
  std::filesystem::remove(path);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF SURFACE malformed ACIS hex is rejected without throwing",
          "[dxf][surface][malformed]") {
  for (const char *hex : {"GG", "A"}) {
    const std::string dxf =
        std::string("0\nSECTION\n2\nENTITIES\n")
        + "0\nPLANESURFACE\n8\n0\n100\nAcDbEntity\n100\nAcDbSurface\n"
        + "70\n1\n71\n0\n72\n0\n310\n" + hex + "\n"
        + "0\nENDSEC\n0\nEOF\n";

    const auto path =
        std::filesystem::temp_directory_path() / "lc_surface_bad_acis_hex.dxf";
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << dxf;
    }

    SurfaceCapture cap;
    dxfRW r(path.string().c_str());
    bool ok = true;
    REQUIRE_NOTHROW(ok = r.read(&cap, /*ext=*/true));
    CHECK_FALSE(ok);
    CHECK(cap.m_callCount == 0);
    std::filesystem::remove(path);
  }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF modeler geometry entities read raw SAT/SAB payloads",
          "[dxf][modeler]") {
  ModelerGeometryCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\n3DSOLID\n5\n4A\n330\n1F\n100\nAcDbEntity\n"
      "100\nAcDbModelerGeometry\n100\nAcDb3dSolid\n70\n1\n"
      "310\n41434953\n310\n2042696E61727946696C65\n"
      "1001\nMODELAPP\n1000\nsolid-xdata\n"
      "0\nREGION\n5\n4B\n330\n1F\n100\nAcDbEntity\n"
      "100\nAcDbModelerGeometry\n100\nAcDbRegion\n"
      "1\nBegin-of-ACIS-History\n3\n-SAT-\n"
      "0\nBODY\n5\n4C\n330\n1F\n100\nAcDbEntity\n"
      "100\nAcDbModelerGeometry\n100\nAcDbBody\n310\n00FF10\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_modeler_geometry_read.dxf");

  REQUIRE(cap.m_items.size() == 3u);
  CHECK(cap.m_items[0].eType == DRW::E3DSOLID);
  CHECK(cap.m_items[0].handle == 0x4Au);
  CHECK(cap.m_items[0].parentHandle == 0x1Fu);
  CHECK(cap.m_items[0].m_modelerVersion == 1u);
  CHECK(cap.m_items[0].m_rawBytes == bytesOf("ACIS BinaryFile"));
  REQUIRE(cap.m_items[0].extData.size() == 2u);
  CHECK(cap.m_items[0].extData[1]->code() == 1000);
  CHECK(std::string(cap.m_items[0].extData[1]->c_str()) == "solid-xdata");

  CHECK(cap.m_items[1].eType == DRW::REGION);
  CHECK(cap.m_items[1].m_rawBytes == bytesOf("Begin-of-ACIS-History-SAT-"));

  CHECK(cap.m_items[2].eType == DRW::BODY);
  REQUIRE(cap.m_items[2].m_rawBytes.size() == 3u);
  CHECK(cap.m_items[2].m_rawBytes[0] == 0x00u);
  CHECK(cap.m_items[2].m_rawBytes[1] == 0xffu);
  CHECK(cap.m_items[2].m_rawBytes[2] == 0x10u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF modeler geometry raw payload round-trips through typed writer",
          "[dxf][modeler][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_modeler_geometry_rt.dxf";
  std::filesystem::remove(path);

  ModelerGeometryEmitter emitter;
  DRW_ModelerGeometry solid(DRW::E3DSOLID);
  solid.layer = "0";
  solid.m_modelerVersion = 1;
  solid.m_rawBytes = modelerPayload("ACIS BinaryFile", 180);
  addXData(solid, "roundtrip-solid");
  emitter.m_items.push_back(solid);

  DRW_ModelerGeometry region(DRW::REGION);
  region.layer = "0";
  region.m_rawBytes = bytesOf("Begin-of-ACIS-History-SAT-region");
  emitter.m_items.push_back(region);

  DRW_ModelerGeometry body(DRW::BODY);
  body.layer = "0";
  body.m_rawBytes = {0x00u, 0x7fu, 0x80u, 0xffu};
  emitter.m_items.push_back(body);

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  ModelerGeometryCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_items.size() == 3u);
  CHECK(cap.m_items[0].eType == DRW::E3DSOLID);
  CHECK(cap.m_items[0].m_modelerVersion == 1u);
  CHECK(cap.m_items[0].m_rawBytes == solid.m_rawBytes);
  REQUIRE(cap.m_items[0].extData.size() == 2u);
  CHECK(std::string(cap.m_items[0].extData[1]->c_str()) == "roundtrip-solid");

  CHECK(cap.m_items[1].eType == DRW::REGION);
  CHECK(cap.m_items[1].m_rawBytes == region.m_rawBytes);
  CHECK(cap.m_items[2].eType == DRW::BODY);
  CHECK(cap.m_items[2].m_rawBytes == body.m_rawBytes);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF modeler geometry malformed ACIS hex is rejected without throwing",
          "[dxf][modeler][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_modeler_bad_acis_hex.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\n3DSOLID\n8\n0\n100\nAcDbEntity\n"
           "100\nAcDbModelerGeometry\n100\nAcDb3dSolid\n310\nGG\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  ModelerGeometryCapture cap;
  dxfRW r(path.string().c_str());
  bool ok = true;
  REQUIRE_NOTHROW(ok = r.read(&cap, /*ext=*/true));
  CHECK_FALSE(ok);
  CHECK(cap.m_items.empty());
  std::filesystem::remove(path);
}

// NOLINTNEXTLINE(readability-identifier-naming)
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

// NOLINTNEXTLINE(readability-identifier-naming)
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

// NOLINTNEXTLINE(readability-identifier-naming)
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

// NOLINTNEXTLINE(readability-identifier-naming)
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

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF LINE thickness(39)+extrusion(210/220/230) round-trip "
          "(write-review B1)", "[dxf][line][dxf_roundtrip]") {
  // Regression for the DXF field-drop fixes: writeLine used to emit neither
  // thickness nor extrusion, flattening thick / out-of-plane lines on DXF save
  // even though the reader and DWG encoder both preserve them.
  const auto path =
      std::filesystem::temp_directory_path() / "lc_line_thick_extr.dxf";
  std::filesystem::remove(path);

  LineEmitter emitter;
  emitter.m_line.layer = "0";
  emitter.m_line.basePoint = DRW_Coord(1.0, 2.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(4.0, 6.0, 0.0);
  emitter.m_line.thickness = 2.5;
  emitter.m_line.extPoint = DRW_Coord(0.0, 1.0, 0.0);  // non-default normal

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  LineCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.thickness == 2.5);          // 39 survived (was dropped)
  CHECK(cap.m_captured.extPoint.x == 0.0);         // 210/220/230 survived
  CHECK(cap.m_captured.extPoint.y == 1.0);
  CHECK(cap.m_captured.extPoint.z == 0.0);

  std::filesystem::remove(path);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF entity visibility(60) round-trip (write-review pass-2 #11)",
          "[dxf][line][dxf_roundtrip]") {
  // writeEntity now emits code 60 for an invisible entity; the reader parses it
  // back into DRW_Entity::visible. (Default-visible entities omit 60.)
  const auto path =
      std::filesystem::temp_directory_path() / "lc_line_invisible.dxf";
  std::filesystem::remove(path);

  LineEmitter emitter;
  emitter.m_line.layer = "0";
  emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
  emitter.m_line.visible = false;  // -> code 60 = 1

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }
  LineCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.visible == false);  // 60 survived (was dropped)

  std::filesystem::remove(path);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF TOLERANCE is read into a DRW_Tolerance (slice E1)", "[dxf][tolerance]") {
  ToleranceCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nTOLERANCE\n8\n0\n100\nAcDbFcf\n3\nSTANDARD\n"
      "10\n4.0\n20\n5.0\n30\n0.0\n"
      "1\n{\\Fgdt;j}%%v\n"
      "11\n1.0\n21\n0.0\n31\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_tolerance_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.dimStyleName == "STANDARD");
  CHECK(cap.m_captured.insertionPoint.x == 4.0);
  CHECK(cap.m_captured.insertionPoint.y == 5.0);
  CHECK(cap.m_captured.text == "{\\Fgdt;j}%%v");
  CHECK(cap.m_captured.xAxisDirectionVector.x == 1.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF TOLERANCE round-trips through write+read (slice E1)",
          "[dxf][tolerance][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_tolerance_roundtrip.dxf";
  std::filesystem::remove(path);

  ToleranceEmitter emitter;
  emitter.m_tol.layer = "0";
  emitter.m_tol.dimStyleName = "STANDARD";
  emitter.m_tol.text = "{\\Fgdt;n0.5}%%v";
  emitter.m_tol.insertionPoint = DRW_Coord(2.0, 3.0, 0.0);
  emitter.m_tol.xAxisDirectionVector = DRW_Coord(1.0, 0.0, 0.0);

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  ToleranceCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.dimStyleName == "STANDARD");
  CHECK(cap.m_captured.text == "{\\Fgdt;n0.5}%%v");
  CHECK(cap.m_captured.insertionPoint.x == 2.0);
  CHECK(cap.m_captured.insertionPoint.y == 3.0);
  CHECK(cap.m_captured.xAxisDirectionVector.x == 1.0);

  std::filesystem::remove(path);
}

// attrib-73: DXF AcDbAttribute subclass uses code 73 for field length (not
// the vertical alignment from AcDbText), and code 74 for vertical alignment.
// Before this fix, code 73 in AcDbAttribute wrongly set DRW_Text::alignV (via
// the default DRW_Text::parseCode fallback) and code 74 was silently dropped.
// Also verifies that writeAttrib emits both 73 (fieldLength) and 74 (alignV).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF ATTRIB AcDbAttribute codes 73/74 round-trip (attrib-73)",
          "[dxf][attrib][attrib-73]") {
  // R2000+ DXF with explicit AcDbAttribute subclass, 73=fieldLength, 74=valign.
  // alignV=VTop (3) — the default is VBaseLine (0) so any difference is visible.
  const char *kAttrib73 =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nINSERT\n8\n0\n2\nTBLK\n10\n0\n20\n0\n30\n0\n66\n1\n"
      "0\nATTRIB\n5\n30\n330\n1F\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbText\n10\n1\n20\n2\n30\n0\n40\n0.5\n1\nVALUE\n"
      "100\nAcDbAttribute\n2\nTAG\n70\n0\n73\n5\n74\n3\n"
      "0\nSEQEND\n"
      "0\nENDSEC\n0\nEOF\n";
  InsertCapture cap;
  readDxf(kAttrib73, cap, "lc_attrib_73_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_captured.attlist.size() == 1);
  const DRW_Attrib &att = *cap.m_captured.attlist[0];
  CHECK(att.m_fieldLength == 5);
  CHECK(att.alignV == DRW_Text::VTop);    // code 74 must set alignV, not be dropped

  // Round-trip via write+read: fieldLength and alignV must survive.
  const auto path =
      std::filesystem::temp_directory_path() / "lc_attrib_73_rt.dxf";
  std::filesystem::remove(path);

  AttribEmitter em;
  em.m_insert.name = "TBLK";
  auto a = std::make_shared<DRW_Attrib>();
  a->basePoint = DRW_Coord(1.0, 2.0, 0.0);
  a->height = 0.5;
  a->text = "VALUE";
  a->tag = "TAG";
  a->attribFlags = 0;
  a->m_fieldLength = 7;
  a->alignV = DRW_Text::VMiddle;
  em.m_insert.attlist.push_back(a);
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  InsertCapture cap2;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap2, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap2.m_captured.attlist.size() == 1);
  const DRW_Attrib &att2 = *cap2.m_captured.attlist[0];
  CHECK(att2.m_fieldLength == 7);
  CHECK(att2.alignV == DRW_Text::VMiddle);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF ATTDEF writer emits attribute definition fields",
          "[dxf][attdef][dxf_roundtrip]") {
  for (const bool viaAttrib : {false, true}) {
    const auto path = std::filesystem::temp_directory_path() /
        (viaAttrib ? "lc_attdef_via_attrib.dxf" : "lc_attdef_direct.dxf");
    std::filesystem::remove(path);

    AttdefEmitter em;
    em.m_viaAttrib = viaAttrib;
    em.m_attdef.layer = "0";
    em.m_attdef.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    em.m_attdef.secPoint = DRW_Coord(3.0, 4.0, 0.0);
    em.m_attdef.height = 0.5;
    em.m_attdef.text = "Default";
    em.m_attdef.tag = "PARTNO";
    em.m_attdef.prompt = "Part number?";
    em.m_attdef.attribFlags = 3;
    em.m_attdef.m_fieldLength = 12;
    em.m_attdef.alignV = DRW_Text::VMiddle;
    em.m_attdef.lockPosition = true;
    {
      dxfRW w(path.string().c_str());
      em.m_rw = &w;
      REQUIRE(w.write(&em, DRW::AC1021, false));
    }

    RawEntityCapture cap;
    {
      dxfRW r(path.string().c_str());
      REQUIRE(r.read(&cap, /*ext=*/true));
    }
    std::filesystem::remove(path);

    REQUIRE(cap.m_entities.size() == 1);
    const DRW_RawDxfObject &attdef = cap.m_entities[0];
    CHECK(attdef.name == "ATTDEF");
    CHECK(hasStringGroup(attdef, 100, "AcDbAttributeDefinition"));
    CHECK(hasStringGroup(attdef, 1, "Default"));
    CHECK(hasStringGroup(attdef, 2, "PARTNO"));
    CHECK(hasStringGroup(attdef, 3, "Part number?"));
    CHECK(hasIntGroup(attdef, 70, 3));
    CHECK(hasIntGroup(attdef, 73, 12));
    CHECK(hasIntGroup(attdef, 74, DRW_Text::VMiddle));
    CHECK(hasIntGroup(attdef, 280, 1));
  }
}

// image-wipeout-71: DXF code 71 (clip boundary type) was not stored by
// DRW_Image::parseCode.  After the fix, code 71 in IMAGE and WIPEOUT entities
// is captured in DRW_Image::m_clipBoundaryType.  Also verifies that
// writeWipeout now emits code 71 (it was previously omitted).
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF IMAGE/WIPEOUT code 71 stored in m_clipBoundaryType (image-wipeout-71)",
          "[dxf][image][wipeout][image-wipeout-71]") {
  // --- read side: IMAGE with explicit 71=1 (rectangular) ---
  const char *kImage71 =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nIMAGE\n5\nA1\n330\n0\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbRasterImage\n"
      "10\n0\n20\n0\n30\n0\n"
      "11\n1\n21\n0\n31\n0\n"
      "12\n0\n22\n1\n32\n0\n"
      "13\n100\n23\n100\n"
      "340\n0\n"
      "70\n1\n280\n1\n281\n50\n282\n50\n283\n0\n"
      "360\n0\n"
      "71\n1\n"
      "91\n2\n14\n-0.5\n24\n-0.5\n14\n99.5\n24\n99.5\n"
      "0\nENDSEC\n0\nEOF\n";
  ImageCapture cap1;
  readDxf(kImage71, cap1, "lc_image_71_read.dxf");
  REQUIRE(cap1.m_imageCount == 1);
  CHECK(cap1.m_lastImage.m_clipBoundaryType == 1);
  CHECK(cap1.m_lastImage.clipPath.size() == 2);

  // --- read side: WIPEOUT with explicit 71=2 (polygonal) ---
  const char *kWipeout71 =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWIPEOUT\n5\nB1\n330\n0\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbRasterImage\n"
      "10\n1\n20\n2\n30\n0\n"
      "11\n0.01\n21\n0\n31\n0\n"
      "12\n0\n22\n0.01\n32\n0\n"
      "13\n100\n23\n100\n"
      "70\n1\n280\n1\n281\n50\n282\n50\n283\n0\n"
      "100\nAcDbWipeout\n"
      "90\n0\n"
      "71\n2\n"
      "91\n3\n14\n10\n24\n0\n14\n20\n24\n10\n14\n10\n24\n20\n"
      "290\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  ImageCapture cap2;
  readDxf(kWipeout71, cap2, "lc_wipeout_71_read.dxf");
  REQUIRE(cap2.m_wipeoutCount == 1);
  CHECK(cap2.m_lastWipeout.m_clipBoundaryType == 2);
  CHECK(cap2.m_lastWipeout.clipPath.size() == 3);

  // --- write+read: writeWipeout must emit code 71 ---
  // Write a WIPEOUT with a 3-vertex clip path; default m_clipBoundaryType=0
  // triggers the wipeout-default of type 2.  Reading back should store type 2.
  const auto path =
      std::filesystem::temp_directory_path() / "lc_wipeout_71_rt.dxf";
  std::filesystem::remove(path);

  WipeoutEmitter em;
  em.m_wipeout.basePoint = DRW_Coord(5.0, 5.0, 0.0);
  em.m_wipeout.secPoint  = DRW_Coord(0.01, 0.0, 0.0);
  em.m_wipeout.vVector   = DRW_Coord(0.0, 0.01, 0.0);
  em.m_wipeout.sizeu = 100;
  em.m_wipeout.sizev = 100;
  em.m_wipeout.clipPath.push_back({0.0, 0.0, 0.0});
  em.m_wipeout.clipPath.push_back({50.0, 0.0, 0.0});
  em.m_wipeout.clipPath.push_back({25.0, 50.0, 0.0});
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  ImageCapture cap3;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap3, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap3.m_wipeoutCount == 1);
  CHECK(cap3.m_lastWipeout.m_clipBoundaryType == 2);
  CHECK(cap3.m_lastWipeout.clipPath.size() == 3);
}

// hatch-97: DXF code 97 appears in two distinct contexts inside a HATCH
// entity:
//   1. As a spline-edge fit-point count (nfit), preceding code-11/21 pairs.
//   2. As a per-loop source boundary object count (associative hatch),
//      preceding code-330 handle pairs.
// Before this fix, a spline edge with nfit=0 left the 'spline' pointer
// active, so the *next* code-97 (the boundary count) was wrongly consumed
// as a second nfit rather than stored as the boundary handle count.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF HATCH code 97/330 boundary handles stored per-loop (hatch-97)",
          "[dxf][hatch][hatch-97]") {
  // Minimal associative-hatch DXF: one spline edge with nfit=0,
  // followed by one boundary source handle (0xBEEF).
  const char *kHatch97 =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nHATCH\n5\nA0\n330\n0\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbHatch\n"
      "10\n0.0\n20\n0.0\n30\n0.0\n"
      "210\n0.0\n220\n0.0\n230\n1.0\n"
      "2\nSOLID\n70\n1\n71\n1\n"   // solid, associative
      "91\n1\n"                     // 1 loop
      "92\n0\n"                     // edge path
      "93\n1\n"                     // 1 edge
      "72\n4\n"                     // spline edge
      "94\n2\n"                     // degree
      "73\n0\n74\n0\n"              // not rational, not periodic
      "95\n4\n"                     // 4 knots
      "96\n2\n"                     // 2 control points
      "40\n0.0\n40\n0.0\n40\n1.0\n40\n1.0\n"  // knots
      "10\n0.0\n20\n0.0\n"         // cp1
      "10\n4.0\n20\n0.0\n"         // cp2
      "97\n0\n"                     // nfit = 0
      "97\n1\n"                     // boundary handle count = 1
      "330\nBEEF\n"                 // handle 0xBEEF = 48879
      "75\n0\n76\n1\n77\n0\n78\n0\n47\n1.0\n"
      "98\n0\n"
      "0\nENDSEC\n0\nEOF\n";

  HatchCapture cap1;
  readDxf(kHatch97, cap1, "lc_hatch97_read.dxf");
  REQUIRE(cap1.m_callCount == 1);
  REQUIRE(cap1.m_captured.looplist.size() == 1);
  const DRW_HatchLoop &loop = *cap1.m_captured.looplist[0];
  REQUIRE(loop.m_boundaryHandles.size() == 1);
  CHECK(loop.m_boundaryHandles[0] == 0xBEEFu);

  // --- write+read round-trip: boundary handles must survive ---
  // Build a HATCH with a single LINE boundary loop carrying one handle.
  const auto path =
      std::filesystem::temp_directory_path() / "lc_hatch97_rt.dxf";
  std::filesystem::remove(path);

  HatchEmitter em;
  em.m_hatch.name = "SOLID";
  em.m_hatch.solid = 1;
  em.m_hatch.associative = 1;
  em.m_hatch.hstyle = 0;
  em.m_hatch.hpattern = 1;
  em.m_hatch.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  auto loopPtr = std::make_shared<DRW_HatchLoop>(0);
  auto lineEnt = std::make_shared<DRW_Line>();
  lineEnt->basePoint = DRW_Coord(0.0, 0.0, 0.0);
  lineEnt->secPoint  = DRW_Coord(1.0, 0.0, 0.0);
  loopPtr->objlist.push_back(lineEnt);
  loopPtr->m_boundaryHandles.push_back(0xCAFEu);
  em.m_hatch.appendLoop(loopPtr);
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  HatchCapture cap2;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap2, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap2.m_callCount == 1);
  REQUIRE(cap2.m_captured.looplist.size() == 1);
  const DRW_HatchLoop &rtLoop = *cap2.m_captured.looplist[0];
  REQUIRE(rtLoop.m_boundaryHandles.size() == 1);
  CHECK(rtLoop.m_boundaryHandles[0] == 0xCAFEu);
}
