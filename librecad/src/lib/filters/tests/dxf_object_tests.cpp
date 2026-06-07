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
 * DXF OBJECTS-section read tests for newly-wired object types.
 *   - slice C3: GROUP (AcDbGroup) read dispatch + DRW_Group::parseCode.
 * The DXF parser previously dispatched only 6 OBJECTS types and silently
 * skipped the rest (incl. GROUP); RS_FilterDXFRW::addGroup already stores
 * the group into LC_DwgAdvancedMetadata.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

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

class GroupCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Group m_captured;
  void addGroup(const DRW_Group &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class DictionaryCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Dictionary m_captured;
  void addDictionary(const DRW_Dictionary &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class ScaleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Scale m_captured;
  void addScale(const DRW_Scale &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class MLineStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_MLineStyle m_captured;
  void addMLineStyle(const DRW_MLineStyle &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class DictionaryVarCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_DictionaryVar m_captured;
  void addDictionaryVar(const DRW_DictionaryVar &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class DictionaryWithDefaultCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_DictionaryWithDefault m_captured;
  void addDictionaryWithDefault(const DRW_DictionaryWithDefault &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class RasterVariablesCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_RasterVariables m_captured;
  void addRasterVariables(const DRW_RasterVariables &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class SunCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Sun m_captured;
  void addSun(const DRW_Sun &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class LayoutCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Layout m_captured;
  void addLayout(const DRW_Layout &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class WipeoutVariablesCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_WipeoutVariables m_captured;
  void addWipeoutVariables(const DRW_WipeoutVariables &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class RawObjectCapture : public StubInterface {
public:
  std::vector<DRW_RawDxfObject> m_objects;
  void addRawDxfObject(const DRW_RawDxfObject &d) override {
    m_objects.push_back(d);
  }
};

class RawEntityCapture : public StubInterface {
public:
  std::vector<DRW_RawDxfObject> m_entities;
  void addRawDxfEntity(const DRW_RawDxfObject &d) override {
    m_entities.push_back(d);
  }
};

class RawObjectEmitter : public StubInterface {
public:
  DRW_RawDxfObject m_obj;
  dxfRW *m_rw = nullptr;
  void writeObjects() override { m_rw->writeRawDxfObject(&m_obj); }
};

// Emits nothing of its own; the codec emits the named-dict / group objects it
// was handed via setNamedDictObjects / setGroups. Used by the [objects] codec
// write units.
class NullObjectEmitter : public StubInterface {
public:
  void writeObjects() override {}
};

// Emits a set of points whose handle field is pre-seeded with a SOURCE handle
// (as RS_FilterDXFRW::getEntityAttributes does). Drives dxfRW::writePoint ->
// writeEntity so the codec's source->minted capture (F3) can be inspected.
class SeededPointEmitter : public StubInterface {
public:
  std::vector<std::uint32_t> m_sourceHandles;
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    for (std::uint32_t src : m_sourceHandles) {
      DRW_Point pt;
      pt.basePoint = DRW_Coord(1.0, 2.0, 0.0);
      pt.handle = src;  // seed: source-handle key consumed by writeEntity
      m_rw->writePoint(&pt);
    }
  }
};

// Emits a POLYLINE (two vertices, seeded source 0xAA) followed by a POINT (seeded
// source 0xBB). The POLYLINE drives the VERTEX/SEQEND parent re-entries into
// dxfRW::writeEntity that must NOT pollute the source->minted map (A-3).
class SeededPolylineEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    DRW_Polyline pl;
    pl.handle = 0xAAu;  // seeded source-handle key
    auto v1 = std::make_shared<DRW_Vertex>();
    v1->basePoint = DRW_Coord(0.0, 0.0, 0.0);
    auto v2 = std::make_shared<DRW_Vertex>();
    v2->basePoint = DRW_Coord(1.0, 1.0, 0.0);
    pl.vertlist.push_back(v1);
    pl.vertlist.push_back(v2);
    m_rw->writePolyline(&pl);

    DRW_Point pt;
    pt.basePoint = DRW_Coord(2.0, 2.0, 0.0);
    pt.handle = 0xBBu;  // seeded source-handle key
    m_rw->writePoint(&pt);
  }
};

// Emits a POINT whose xAxisAngle is set in TRUE RADIANS (the DWG-path / field
// convention). The DXF writer must convert it to DEGREES for code 50 (C-2).
class XAxisPointEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    DRW_Point pt;
    pt.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    pt.xAxisAngle = 1.5707963267948966;  // pi/2 radians == 90 degrees
    m_rw->writePoint(&pt);
  }
};

// Read a written DXF file back into a string for structural assertions.
std::string slurp(const std::filesystem::path &path) {
  std::ifstream in(path);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

// Format a handle the way the codec emits code-5/340/350: uppercase hex, no
// leading zeros (matches dxfRW::toHexStr's "%X").
std::string toHexUpper(std::uint32_t h) {
  char buf[9] = {'\0'};
  std::snprintf(buf, sizeof(buf), "%X", h);
  return std::string(buf);
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

} // namespace

TEST_CASE("DXF GROUP object is read into a DRW_Group (slice C3)", "[dxf][group]") {
  GroupCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGROUP\n5\n2F\n330\nC\n100\nAcDbGroup\n"
      "300\nFasteners\n70\n0\n71\n1\n"
      "340\n30\n340\n31\n340\n32\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_group_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_description == "Fasteners");
  CHECK(cap.m_captured.m_isUnnamed == false);
  CHECK(cap.m_captured.m_selectable == true);
  REQUIRE(cap.m_captured.m_entityHandles.size() == 3);
  CHECK(cap.m_captured.m_entityHandles[0] == 0x30u);
  CHECK(cap.m_captured.m_entityHandles[2] == 0x32u);
}

TEST_CASE("DXF unnamed GROUP sets the unnamed flag (slice C3)", "[dxf][group]") {
  GroupCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGROUP\n5\n3A\n330\nC\n100\nAcDbGroup\n"
      "300\n\n70\n1\n71\n0\n340\n40\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_group_unnamed.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_isUnnamed == true);
  CHECK(cap.m_captured.m_selectable == false);
  REQUIRE(cap.m_captured.m_entityHandles.size() == 1);
}

TEST_CASE("DXF DICTIONARY entries are read (name->handle) (slice C1)", "[dxf][dictionary]") {
  DictionaryCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n"
      "3\nACAD_LAYOUT\n350\n1A\n"
      "3\nACAD_MLINESTYLE\n350\n17\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictionary_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.cloning == 1);
  REQUIRE(cap.m_captured.m_entries.size() == 3);
  CHECK(cap.m_captured.m_entries[0].m_name == "ACAD_GROUP");
  CHECK(cap.m_captured.m_entries[0].m_handle == 0xDu);
  CHECK(cap.m_captured.m_entries[1].m_name == "ACAD_LAYOUT");
  CHECK(cap.m_captured.m_entries[1].m_handle == 0x1Au);
  CHECK(cap.m_captured.m_entries[2].m_name == "ACAD_MLINESTYLE");
  CHECK(cap.m_captured.m_entries[2].m_handle == 0x17u);
}

TEST_CASE("DXF SCALE object is read (label + numerator/denominator) (slice C6)", "[dxf][scale]") {
  ScaleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSCALE\n5\nB1\n330\nB0\n100\nAcDbScale\n"
      "70\n0\n300\n1:2\n140\n1.0\n141\n2.0\n290\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_scale_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "1:2");
  CHECK(cap.m_captured.paperUnits == 1.0);
  CHECK(cap.m_captured.drawingUnits == 2.0);
  CHECK(cap.m_captured.isUnitScale == false);
  CHECK(cap.m_captured.scaleFactor() == 2.0);
}

TEST_CASE("DXF MLINESTYLE object is read with elements (slice C5)", "[dxf][mlinestyle]") {
  MLineStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMLINESTYLE\n5\n18\n330\n17\n100\nAcDbMlineStyle\n"
      "2\nSTANDARD\n70\n0\n3\nstd desc\n62\n256\n51\n90.0\n52\n90.0\n"
      "71\n2\n"
      "49\n0.5\n62\n1\n6\nBYLAYER\n"
      "49\n-0.5\n62\n2\n6\nCONTINUOUS\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_mlinestyle_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "STANDARD");
  CHECK(cap.m_captured.description == "std desc");
  CHECK(cap.m_captured.fillColor == 256);  // the pre-element 62
  CHECK(cap.m_captured.startAngle == 90.0);
  REQUIRE(cap.m_captured.elements.size() == 2);
  CHECK(cap.m_captured.elements[0].offset == 0.5);
  CHECK(cap.m_captured.elements[0].color == 1);
  CHECK(cap.m_captured.elements[0].linetype == "BYLAYER");
  CHECK(cap.m_captured.elements[1].offset == -0.5);
  CHECK(cap.m_captured.elements[1].color == 2);
  CHECK(cap.m_captured.elements[1].linetype == "CONTINUOUS");
}

TEST_CASE("DXF DICTIONARYVAR object is read (schema + value)", "[dxf][dictionaryvar]") {
  DictionaryVarCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARYVAR\n5\n2A\n330\n29\n100\nDictionaryVariables\n"
      "280\n0\n1\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictvar_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_schema == 0);
  CHECK(cap.m_captured.m_value == "2");
}

TEST_CASE("DXF ACDBDICTIONARYWDFLT reads entries + default handle", "[dxf][dictionary]") {
  DictionaryWithDefaultCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBDICTIONARYWDFLT\n5\n2B\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nNormal\n350\n2C\n"
      "100\nAcDbDictionaryWithDefault\n340\n2C\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictwdflt_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_captured.m_entries.size() == 1);
  CHECK(cap.m_captured.m_entries[0].m_name == "Normal");
  CHECK(cap.m_captured.m_defaultEntryHandle == 0x2Cu);
}

TEST_CASE("DXF RASTERVARIABLES object is read (frame/quality/units)", "[dxf][rastervariables]") {
  RasterVariablesCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nRASTERVARIABLES\n5\n2D\n330\n29\n100\nAcDbRasterVariables\n"
      "90\n0\n70\n1\n71\n1\n72\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rastervars_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_imageFrame == 1);
  CHECK(cap.m_captured.m_imageQuality == 1);
  CHECK(cap.m_captured.m_units == 0);
}

TEST_CASE("DXF SUN object is read (status/intensity/shadows/date)", "[dxf][sun]") {
  SunCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSUN\n5\n2E\n330\n29\n100\nAcDbSun\n"
      "90\n1\n290\n1\n63\n7\n421\n16711680\n40\n1.0\n291\n1\n"
      "91\n2455563\n92\n43200000\n292\n0\n70\n1\n71\n256\n280\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sun_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_isOn == true);
  CHECK(cap.m_captured.m_color == 7u);
  CHECK(cap.m_captured.m_color24 == 16711680);  // code 421 true-color preserved (G-1)
  CHECK(cap.m_captured.m_intensity == 1.0);
  CHECK(cap.m_captured.m_hasShadow == true);
  CHECK(cap.m_captured.m_julianDay == 2455563);
  CHECK(cap.m_captured.m_milliseconds == 43200000);
  CHECK(cap.m_captured.m_shadowMapSize == 256);
}

TEST_CASE("DXF LAYOUT object disambiguates plot vs layout subclasses (slice C2)", "[dxf][layout]") {
  LayoutCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nLAYOUT\n5\n4F\n330\n1A\n"
      "100\nAcDbPlotSettings\n"
      "1\nMy Page Setup\n2\nDWG To PDF\n4\nANSI_A\n"
      "40\n5.8\n41\n5.8\n42\n5.8\n43\n5.8\n44\n215.9\n45\n279.4\n"
      "70\n688\n72\n0\n73\n1\n74\n5\n75\n16\n"
      "100\nAcDbLayout\n"
      "1\nLayout1\n70\n1\n71\n2\n"
      "10\n0.0\n20\n0.0\n11\n12.0\n21\n9.0\n"
      "12\n0.0\n22\n0.0\n32\n0.0\n"
      "76\n0\n146\n0.0\n330\n50\n331\n51\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_layout_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  // AcDbPlotSettings prefix
  CHECK(cap.m_captured.pageSetupName == "My Page Setup");
  CHECK(cap.m_captured.printerConfig == "DWG To PDF");
  CHECK(cap.m_captured.paperSize == "ANSI_A");
  CHECK(cap.m_captured.paperWidth == 215.9);
  CHECK(cap.m_captured.plotLayoutFlags == 688);
  // AcDbLayout body — code 1/70/76/330 must NOT be confused with the prefix
  CHECK(cap.m_captured.name == "Layout1");
  CHECK(cap.m_captured.layoutFlags == 1);
  CHECK(cap.m_captured.tabOrder == 2);
  CHECK(cap.m_captured.limMaxX == 12.0);
  CHECK(cap.m_captured.limMaxY == 9.0);
  CHECK(cap.m_captured.orthoViewType == 0);
  CHECK(cap.m_captured.paperSpaceBlockRecordHandle.ref == 0x50u);
  CHECK(cap.m_captured.lastActiveViewportHandle.ref == 0x51u);
}

TEST_CASE("DXF WIPEOUTVARIABLES object is read (display-frame flag)", "[dxf][wipeoutvars]") {
  WipeoutVariablesCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nWIPEOUTVARIABLES\n5\n30\n330\n29\n100\nAcDbWipeoutVariables\n70\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_wipeoutvars_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_displayFrame == 1);
}

TEST_CASE("DXF unmodeled OBJECT is captured verbatim, not dropped (slice A1)", "[dxf][rawobject]") {
  RawObjectCapture cap;
  // MATERIAL is a real object libdxfrw does not (yet) type for DXF; plus a
  // genuinely unknown object name. Both must be preserved, none dropped.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMATERIAL\n5\n3B\n330\n29\n100\nAcDbMaterial\n"
      "1\nMyMaterial\n94\n63\n"
      "0\nACDBWEIRDOBJECT\n5\n3C\n330\n29\n70\n5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawobject.dxf");

  REQUIRE(cap.m_objects.size() == 2);

  const DRW_RawDxfObject &mat = cap.m_objects[0];
  CHECK(mat.name == "MATERIAL");
  CHECK(mat.handle == 0x3Bu);
  CHECK(mat.parentHandle == 0x29u);
  // groups captured verbatim: 5, 330, 100, 1, 94
  REQUIRE(mat.groups.size() == 5);
  CHECK(mat.groups[0].code() == 5);
  CHECK(mat.groups[2].code() == 100);
  CHECK(mat.groups[3].code() == 1);

  const DRW_RawDxfObject &weird = cap.m_objects[1];
  CHECK(weird.name == "ACDBWEIRDOBJECT");
  CHECK(weird.handle == 0x3Cu);
  REQUIRE(weird.groups.size() == 3);  // 5, 330, 70
}

TEST_CASE("DXF unmodeled ENTITY is captured verbatim, not dropped (slice A4)", "[dxf][rawentity]") {
  RawEntityCapture cap;
  // An unknown entity and a standalone ATTDEF (no typed DXF dispatch) — both
  // must be preserved rather than silently skipped.
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n8\n0\n5\n4A\n62\n3\n10\n1.0\n20\n2.0\n"
      "0\nATTDEF\n8\n0\n5\n4B\n10\n0.0\n20\n0.0\n40\n0.5\n1\ndef\n2\nTAG\n3\nPrompt?\n70\n0\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n5.0\n21\n5.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawentity.dxf");

  // WEIRDENT + ATTDEF captured; LINE is typed (not raw).
  REQUIRE(cap.m_entities.size() == 2);
  CHECK(cap.m_entities[0].name == "WEIRDENT");
  CHECK(cap.m_entities[0].handle == 0x4Au);
  CHECK(cap.m_entities[1].name == "ATTDEF");
  CHECK(cap.m_entities[1].handle == 0x4Bu);
  // ATTDEF groups preserved verbatim (8,5,10,20,40,1,2,3,70)
  CHECK(cap.m_entities[1].groups.size() == 9);
}

TEST_CASE("DXF raw object round-trips through write+read (slice A2)",
          "[dxf][rawobject][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_rawobject_rt.dxf";
  std::filesystem::remove(path);

  RawObjectEmitter em;
  em.m_obj.name = "ACDBWEIRDOBJECT";
  em.m_obj.groups.emplace_back(5, std::string("3C"));
  em.m_obj.groups.emplace_back(330, std::string("29"));
  em.m_obj.groups.emplace_back(100, std::string("AcDbWeird"));
  em.m_obj.groups.emplace_back(1, std::string("payload text"));
  em.m_obj.groups.emplace_back(70, std::string("5"));
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  RawObjectCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }

  REQUIRE(cap.m_objects.size() == 1);
  CHECK(cap.m_objects[0].name == "ACDBWEIRDOBJECT");
  REQUIRE(cap.m_objects[0].groups.size() == 5);
  CHECK(cap.m_objects[0].groups[3].code() == 1);
  CHECK(std::string(cap.m_objects[0].groups[3].c_str()) == "payload text");

  std::filesystem::remove(path);
}

// Regression for the A1/A4 capture bug: the reader leaves strData stale for
// numeric codes (readInt16/32/64/Double/Bool parse into a local string), so the
// old all-getString() capture stored the PREVIOUS string value for every
// numeric group and mistyped it STRING. captureRawGroup must instead store a
// correctly-typed DRW_Variant per reader->type. Asserts VALUES, not just codes.
TEST_CASE("DXF raw-net captures numeric group VALUES, not stale strings "
          "(capture-bug fix)",
          "[dxf][rawobject][rawcapture]") {
  RawObjectCapture cap;
  // A string group (code 1) precedes every numeric group; under the old bug all
  // numerics would re-capture "STRINGVAL". One group of each reader type:
  // 1=string, 70=int16, 90=int32, 160=int64, 40=double, 290=bool.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBWFDIAG\n5\n3B\n330\n29\n100\nAcDbWfDiag\n"
      "1\nSTRINGVAL\n70\n7\n90\n123456\n160\n40\n40\n2.5\n290\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawcapture.dxf");

  REQUIRE(cap.m_objects.size() == 1);
  const DRW_RawDxfObject &o = cap.m_objects[0];
  CHECK(o.handle == 0x3Bu);
  CHECK(o.parentHandle == 0x29u);
  // Index map: 0=5(str) 1=330(str) 2=100(str) 3=1(str) 4=70 5=90 6=160 7=40 8=290
  REQUIRE(o.groups.size() == 9);
  CHECK(o.groups[3].type() == DRW_Variant::STRING);
  CHECK(std::string(o.groups[3].c_str()) == "STRINGVAL");

  CHECK(o.groups[4].code() == 70);
  CHECK(o.groups[4].type() == DRW_Variant::INTEGER);
  CHECK(o.groups[4].i_val() == 7);

  CHECK(o.groups[5].code() == 90);
  CHECK(o.groups[5].type() == DRW_Variant::INTEGER);
  CHECK(o.groups[5].i_val() == 123456);

  CHECK(o.groups[6].code() == 160);
  CHECK(o.groups[6].type() == DRW_Variant::INTEGER64);
  CHECK(o.groups[6].i64_val() == 40);

  CHECK(o.groups[7].code() == 40);
  CHECK(o.groups[7].type() == DRW_Variant::DOUBLE);
  CHECK(o.groups[7].d_val() == 2.5);

  CHECK(o.groups[8].code() == 290);
  CHECK(o.groups[8].type() == DRW_Variant::INTEGER);
  CHECK(o.groups[8].i_val() == 1);

  // End-to-end: re-emit the captured object and read it back; numeric values
  // must survive (writeRawDxfObject keys off variant type()).
  const auto rtPath =
      std::filesystem::temp_directory_path() / "lc_rawcapture_rt.dxf";
  std::filesystem::remove(rtPath);
  RawObjectEmitter em;
  em.m_obj = o;
  {
    dxfRW w(rtPath.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  RawObjectCapture rt;
  {
    dxfRW r(rtPath.string().c_str());
    REQUIRE(r.read(&rt, /*ext=*/true));
  }
  REQUIRE(rt.m_objects.size() == 1);
  const DRW_RawDxfObject &b = rt.m_objects[0];
  REQUIRE(b.groups.size() == 9);
  CHECK(b.groups[4].i_val() == 7);
  CHECK(b.groups[5].i_val() == 123456);
  CHECK(b.groups[6].i64_val() == 40);
  CHECK(b.groups[7].d_val() == 2.5);
  CHECK(b.groups[8].i_val() == 1);
  std::filesystem::remove(rtPath);
}

namespace {
// Parse a DXF file's lines into trimmed (code-line, value-line) text rows so a
// codec-write test can assert ordered group sequences without a full reader.
std::vector<std::pair<std::string, std::string>> readGroups(
    const std::filesystem::path &path) {
  std::vector<std::pair<std::string, std::string>> out;
  std::ifstream in(path);
  std::string codeLine;
  std::string valLine;
  auto trim = [](std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
      s.pop_back();
    std::size_t b = s.find_first_not_of(" \t");
    return b == std::string::npos ? std::string() : s.substr(b);
  };
  while (std::getline(in, codeLine) && std::getline(in, valLine))
    out.emplace_back(trim(codeLine), trim(valLine));
  return out;
}

// True if the group sequence `groups` contains, starting at any position, the
// ordered subsequence `seq` (each element a (code,value) pair), allowing other
// groups in between is NOT permitted — strictly consecutive.
bool hasConsecutive(
    const std::vector<std::pair<std::string, std::string>> &groups,
    const std::vector<std::pair<std::string, std::string>> &seq) {
  if (seq.empty() || groups.size() < seq.size())
    return false;
  for (std::size_t i = 0; i + seq.size() <= groups.size(); ++i) {
    bool ok = true;
    for (std::size_t j = 0; j < seq.size(); ++j)
      if (groups[i + j] != seq[j]) { ok = false; break; }
    if (ok)
      return true;
  }
  return false;
}
} // namespace

// F4f-1: dxfRW::setNamedDictObjects emits a named DICTIONARY object in the
// OBJECTS section, owned by C (parentHandle 0 -> 330 "C"), with its entry list,
// while setRootDictEntries re-attaches it under the root C dict. This is the
// emit that gives the previously-dangling 350 references a real, valid-owner
// target (clearing ezdxf INVALID_OWNER_HANDLE on the DWG->DXF path).
TEST_CASE("DXF setNamedDictObjects emits an owned named dictionary (F4f-1)",
          "[dxf][objects][dictionary]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_nameddict_emit.dxf";
  std::filesystem::remove(path);

  NullObjectEmitter em;
  {
    dxfRW w(path.string().c_str());
    DRW_Dictionary dict;
    dict.handle = 0x50u;
    dict.parentHandle = 0;  // -> owner "C"
    dict.cloning = 1;
    DRW_Dictionary::Entry e;
    e.m_name = "SCALE";
    e.m_handle = 0x51u;
    dict.m_entries.push_back(e);
    w.setNamedDictObjects({dict});
    w.setRootDictEntries({{"ACAD_SCALELIST", "50"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // Root C dict carries the ACAD_SCALELIST -> 50 entry.
  CHECK(hasConsecutive(groups, {{"3", "ACAD_SCALELIST"}, {"350", "50"}}));
  // A DICTIONARY object at handle 50, owned by C, with the SCALE -> 51 entry.
  CHECK(hasConsecutive(groups,
                       {{"0", "DICTIONARY"}, {"5", "50"}, {"330", "C"},
                        {"100", "AcDbDictionary"}, {"281", "1"},
                        {"3", "SCALE"}, {"350", "51"}}));
}

// C-1: the named-dictionary emit preserves the FULL duplicate-record cloning
// policy (code 281), not a 0/1 collapse. A dict with cloning=12 (keep + sort)
// must round-trip as 281=12, since parseCode reads the full int.
TEST_CASE("DXF setNamedDictObjects preserves the cloning policy (code 281)",
          "[dxf][objects][dictionary]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_nameddict_cloning.dxf";
  std::filesystem::remove(path);

  NullObjectEmitter em;
  {
    dxfRW w(path.string().c_str());
    DRW_Dictionary dict;
    dict.handle = 0x50u;
    dict.parentHandle = 0;
    dict.cloning = 12;  // keep + sort — must NOT collapse to 1
    DRW_Dictionary::Entry e;
    e.m_name = "SCALE";
    e.m_handle = 0x51u;
    dict.m_entries.push_back(e);
    w.setNamedDictObjects({dict});
    w.setRootDictEntries({{"ACAD_SCALELIST", "50"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(hasConsecutive(groups,
                       {{"0", "DICTIONARY"}, {"5", "50"}, {"330", "C"},
                        {"100", "AcDbDictionary"}, {"281", "12"}}));
}

// C-2: DRW_Point::xAxisAngle is true radians (matching the DWG path); the DXF
// writer must emit code 50 in DEGREES (radians * ARAD). A point with pi/2 rad
// must write 90, not pi/2 / ARAD (~0.0274, the pre-fix bug).
TEST_CASE("DXF writePoint emits xAxisAngle (code 50) in degrees from radians (C-2)",
          "[dxf][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_point_xaxis.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    XAxisPointEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // Find the POINT record's code-50 value and assert ~90 degrees.
  bool inPoint = false;
  bool found = false;
  double angle = -1.0;
  for (const auto &kv : groups) {
    if (kv.first == "0")
      inPoint = (kv.second == "POINT");
    else if (inPoint && kv.first == "50") {
      angle = std::stod(kv.second);
      found = true;
    }
  }
  REQUIRE(found);
  CHECK(std::fabs(angle - 90.0) < 1e-6);  // 90 deg, not ~0.0274 (the /ARAD bug)
}

// F3-1: dxfRW::writeEntity captures source-handle -> minted-handle in the
// writing context. Two entities seeded with distinct source handles map to two
// distinct minted handles (>= the first minted handle FIRSTHANDLE 0x30). The map
// is written here but consumed only by GROUP-emit (F3-2/F3-3).
TEST_CASE("DXF writeEntity captures source->minted handles (F3-1)",
          "[dxf][objects][handles]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_srcminted_capture.dxf";
  std::filesystem::remove(path);

  SeededPointEmitter em;
  em.m_sourceHandles = {0xAAu, 0xBBu};
  std::map<std::uint32_t, std::uint32_t> captured;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
    captured = w.getWritingContext()->sourceHandleToMintedMap;
  }
  std::filesystem::remove(path);

  REQUIRE(captured.count(0xAAu) == 1);
  REQUIRE(captured.count(0xBBu) == 1);
  const std::uint32_t mintedA = captured[0xAAu];
  const std::uint32_t mintedB = captured[0xBBu];
  CHECK(mintedA >= 0x30u);
  CHECK(mintedB >= 0x30u);
  CHECK(mintedA != mintedB);
}

// B3: writeEntity must emit a code-330 owner (soft-pointer to the owning
// BLOCK_RECORD) on every R2000+ entity. Pre-fix the entity carried no 330 at
// all, so ezdxf/AutoCAD treated it as an orphan. A model-space POINT must own
// to the Model_Space BLOCK_RECORD (handle 1F), emitted right after its (5)
// handle.
TEST_CASE("DXF writeEntity emits a 330 owner handle on every entity (B3)",
          "[dxf][objects][owner]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_entity_owner.dxf";
  std::filesystem::remove(path);

  SeededPointEmitter em;
  em.m_sourceHandles = {0xAAu};
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  bool foundPoint = false;
  for (std::size_t i = 0; i + 2 < groups.size(); ++i) {
    if (groups[i].first == "0" && groups[i].second == "POINT") {
      REQUIRE(groups[i + 1].first == "5");          // minted entity handle
      REQUIRE(groups[i + 2].first == "330");         // owner handle follows
      REQUIRE(groups[i + 2].second == "1F");         // Model_Space BLOCK_RECORD
      foundPoint = true;
    }
  }
  REQUIRE(foundPoint);
}

// P1 (circle/arc extrusion+thickness): writeCircle/writeArc previously dropped
// the AcDbCircle thickness (39) and extrusion (210/220/230), so non-Z-up or
// thick circles/arcs flattened on DXF export. The reader (DRW_Point::parseCode)
// already consumed them; this confirms the writer now emits them.
TEST_CASE("DXF writeCircle/writeArc emit thickness + extrusion (P1)",
          "[dxf][objects][circle]") {
  class CurveEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    void writeEntities() override {
      DRW_Circle c;
      c.basePoint = DRW_Coord(1.0, 2.0, 0.0);
      c.radious = 5.0;
      c.thickness = 2.5;
      c.extPoint = DRW_Coord(0.0, 0.0, -1.0);  // flipped normal
      m_rw->writeCircle(&c);
      DRW_Arc a;
      a.basePoint = DRW_Coord(3.0, 4.0, 0.0);
      a.radious = 1.5;
      a.thickness = 0.0;
      a.extPoint = DRW_Coord(0.0, 1.0, 0.0);  // sideways extrusion
      a.staangle = 0.0;
      a.endangle = 1.0;
      m_rw->writeArc(&a);
    }
  };

  const auto path =
      std::filesystem::temp_directory_path() / "lc_curve_extrusion.dxf";
  std::filesystem::remove(path);
  CurveEmitter em;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // Find the first (code) value within a given entity's group block (from its
  // (0,name) marker up to the next (0,*)); returns true and sets out if found.
  auto entityVal = [&](const std::string &name, const std::string &code,
                       double &out) -> bool {
    bool in = false;
    for (const auto &g : groups) {
      if (g.first == "0") { in = (g.second == name); continue; }
      if (in && g.first == code) { out = std::stod(g.second); return true; }
    }
    return false;
  };
  auto close = [](double a, double b) { double d = a - b; return (d < 0 ? -d : d) < 1e-9; };

  double v = 0.0;
  // CIRCLE: thickness 2.5, extrusion z = -1.0 (flipped).
  REQUIRE(entityVal("CIRCLE", "39", v));
  CHECK(close(v, 2.5));
  REQUIRE(entityVal("CIRCLE", "230", v));
  CHECK(close(v, -1.0));
  // ARC: no thickness (0 -> omitted), extrusion y = 1.0.
  CHECK_FALSE(entityVal("ARC", "39", v));
  REQUIRE(entityVal("ARC", "220", v));
  CHECK(close(v, 1.0));
}

// Batch A (vertex subclass): a 3D POLYLINE's VERTEX must carry the type-specific
// second subclass marker (AcDb3dPolylineVertex) after AcDbVertex. Pre-fix only
// AcDbVertex was emitted, so ezdxf/AutoCAD mis-typed 3D/mesh vertices.
TEST_CASE("DXF writePolyline emits VERTEX type subclass marker (Batch A)",
          "[dxf][objects][vertex]") {
  class Poly3dEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    void writeEntities() override {
      DRW_Polyline pl;
      pl.flags = 8;  // 3D polyline
      auto v1 = std::make_shared<DRW_Vertex>();
      v1->basePoint = DRW_Coord(0.0, 0.0, 0.0);
      auto v2 = std::make_shared<DRW_Vertex>();
      v2->basePoint = DRW_Coord(1.0, 1.0, 2.0);
      pl.vertlist.push_back(v1);
      pl.vertlist.push_back(v2);
      m_rw->writePolyline(&pl);
    }
  };

  const auto path =
      std::filesystem::temp_directory_path() / "lc_vertex_subclass.dxf";
  std::filesystem::remove(path);
  Poly3dEmitter em;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(hasConsecutive(groups,
        {{"100", "AcDbVertex"}, {"100", "AcDb3dPolylineVertex"}}));
}

// Batch A (image mandatory fields): writeImage must emit class_version (90)
// and the clip boundary (71 type, 91 count, 14/24 vertices). Pre-fix these
// were absent and ezdxf flagged the IMAGE as malformed.
TEST_CASE("DXF writeImage emits class_version + clip boundary (Batch A)",
          "[dxf][objects][image]") {
  class ImgEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    void writeEntities() override {
      DRW_Image img;
      img.basePoint = DRW_Coord(0.0, 0.0, 0.0);
      img.secPoint = DRW_Coord(1.0, 0.0, 0.0);
      img.vVector = DRW_Coord(0.0, 1.0, 0.0);
      img.sizeu = 640.0;
      img.sizev = 480.0;
      img.clipPath.push_back(DRW_Coord(0.0, 0.0, 0.0));
      img.clipPath.push_back(DRW_Coord(100.0, 0.0, 0.0));
      img.clipPath.push_back(DRW_Coord(100.0, 80.0, 0.0));
      img.clipPath.push_back(DRW_Coord(0.0, 80.0, 0.0));
      m_rw->writeImage(&img, "ref.png");
    }
  };

  const auto path =
      std::filesystem::temp_directory_path() / "lc_image_fields.dxf";
  std::filesystem::remove(path);
  ImgEmitter em;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  auto inImage = [&](const std::string &code, const std::string &val) {
    bool in = false;
    for (const auto &g : groups) {
      if (g.first == "0") { in = (g.second == "IMAGE"); continue; }
      if (in && g.first == code && g.second == val) return true;
    }
    return false;
  };
  CHECK(inImage("90", "0"));    // class_version
  CHECK(inImage("71", "2"));    // polygonal clip boundary type
  CHECK(inImage("91", "4"));    // 4 boundary vertices
}

// A-3: the VERTEX/SEQEND parent re-entries from writePolyline call
// writeEntity(ent) again on the already-minted parent. Those re-entries must NOT
// emplace into sourceHandleToMintedMap. After writing a POLYLINE (2 vertices,
// source 0xAA) and a POINT (source 0xBB), the map's keys must be EXACTLY the two
// genuine seeded source handles -- no extra minted-range keys. A polluting key
// (a stale minted handle) can numerically equal a real source handle and, via
// emplace's keep-first-seen, SHADOW the genuine mapping, mis-resolving GROUP 340.
TEST_CASE("DXF writeEntity does not pollute source->minted map on VERTEX/SEQEND re-entry (A-3)",
          "[dxf][objects][handles]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_srcminted_poly.dxf";
  std::filesystem::remove(path);

  std::map<std::uint32_t, std::uint32_t> captured;
  {
    dxfRW w(path.string().c_str());
    SeededPolylineEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
    captured = w.getWritingContext()->sourceHandleToMintedMap;
  }
  std::filesystem::remove(path);

  // Exactly the two genuine source handles, nothing else (no re-entry pollution).
  REQUIRE(captured.size() == 2u);
  CHECK(captured.count(0xAAu) == 1);
  CHECK(captured.count(0xBBu) == 1);
  CHECK(captured[0xAAu] >= 0x30u);
  CHECK(captured[0xBBu] >= 0x30u);
  CHECK(captured[0xAAu] != captured[0xBBu]);
}

// F3-2: setGroups injects each group into the ACAD_GROUP D dict (name -> minted
// group handle) and emits a GROUP object owned by D. Members are resolved
// through the source->minted map captured by writeEntity; a member whose source
// handle was never written (0xDEAD) is dropped, never emitted as a dangling 340.
TEST_CASE("DXF setGroups emits a D-owned GROUP with resolved members (F3-2)",
          "[dxf][objects][group]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_group_emit.dxf";
  std::filesystem::remove(path);

  SeededPointEmitter em;
  em.m_sourceHandles = {0xAAu, 0xBBu};
  std::map<std::uint32_t, std::uint32_t> captured;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Group g;
    g.name = "*A1";
    g.m_description = "test group";
    g.m_isUnnamed = true;
    g.m_selectable = true;
    g.m_entityHandles = {0xAAu, 0xBBu, 0xDEADu};  // 0xDEAD never written
    w.setGroups({g});
    REQUIRE(w.write(&em, DRW::AC1021, false));
    captured = w.getWritingContext()->sourceHandleToMintedMap;
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  const std::string mintedA = toHexUpper(captured[0xAAu]);
  const std::string mintedB = toHexUpper(captured[0xBBu]);

  // The GROUP object: owned by D, the unnamed/selectable flags, then exactly the
  // two resolvable members as 340 (0xDEAD dropped).
  CHECK(hasConsecutive(groups,
                       {{"0", "GROUP"}}));
  CHECK(hasConsecutive(
      groups, {{"100", "AcDbGroup"}, {"300", "test group"}, {"70", "1"},
               {"71", "1"}, {"340", mintedA}, {"340", mintedB}}));
  // The GROUP is owned by D and has exactly two 340 members (0xDEAD skipped).
  // Count 340s ONLY inside the GROUP record (from its 0/GROUP marker to the next
  // 0/...), so unrelated table 340s (e.g. STYLE font handle) are not counted.
  int groupCount = 0;
  int memberCount = 0;
  std::string groupHandle;
  for (std::size_t i = 0; i < groups.size(); ++i) {
    if (groups[i].first == "0" && groups[i].second == "GROUP") {
      ++groupCount;
      if (i + 1 < groups.size())
        groupHandle = groups[i + 1].second;  // the code-5 line
      for (std::size_t j = i + 1; j < groups.size(); ++j) {
        if (groups[j].first == "0")
          break;  // end of GROUP record
        if (groups[j].first == "340")
          ++memberCount;
      }
    }
  }
  CHECK(groupCount == 1);
  CHECK(memberCount == 2);
  // The ACAD_GROUP D dict lists the group: 3 *A1 / 350 <minted group handle>.
  CHECK(hasConsecutive(groups, {{"3", "*A1"}, {"350", groupHandle}}));
}
