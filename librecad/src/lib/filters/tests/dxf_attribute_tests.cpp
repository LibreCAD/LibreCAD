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

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <list>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
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

class LTypeCapture : public StubInterface {
public:
  int m_callCount = 0;
  void addLType(const DRW_LType &) override { ++m_callCount; }
};

class LayerCapture : public StubInterface {
public:
  int m_callCount = 0;
  void addLayer(const DRW_Layer &) override { ++m_callCount; }
};

class PolylineCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Polyline m_captured;
  void addPolyline(const DRW_Polyline &d) override {
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

class RevolvedSurfaceCapture : public StubInterface {
public:
  int m_callCount = 0;
  bool m_isRevolved = false;
  DRW_RevolvedSurface m_captured;
  void addSurface(const DRW_Surface *d) override {
    if (m_callCount == 0) {
      const auto *revolved = dynamic_cast<const DRW_RevolvedSurface *>(d);
      m_isRevolved = revolved != nullptr;
      if (revolved != nullptr)
        m_captured = *revolved;
    }
    ++m_callCount;
  }
};

class ExtrudedSurfaceCapture : public StubInterface {
public:
  int m_callCount = 0;
  bool m_isExtruded = false;
  DRW_ExtrudedSurface m_captured;
  void addSurface(const DRW_Surface *d) override {
    if (m_callCount == 0) {
      const auto *extruded = dynamic_cast<const DRW_ExtrudedSurface *>(d);
      m_isExtruded = extruded != nullptr;
      if (extruded != nullptr)
        m_captured = *extruded;
    }
    ++m_callCount;
  }
};

class SweptSurfaceCapture : public StubInterface {
public:
  int m_callCount = 0;
  bool m_isSwept = false;
  DRW_SweptSurface m_captured;
  void addSurface(const DRW_Surface *d) override {
    if (m_callCount == 0) {
      const auto *swept = dynamic_cast<const DRW_SweptSurface *>(d);
      m_isSwept = swept != nullptr;
      if (swept != nullptr)
        m_captured = *swept;
    }
    ++m_callCount;
  }
};

class LoftedSurfaceCapture : public StubInterface {
public:
  int m_callCount = 0;
  bool m_isLofted = false;
  DRW_LoftedSurface m_captured;
  void addSurface(const DRW_Surface *d) override {
    if (m_callCount == 0) {
      const auto *lofted = dynamic_cast<const DRW_LoftedSurface *>(d);
      m_isLofted = lofted != nullptr;
      if (lofted != nullptr)
        m_captured = *lofted;
    }
    ++m_callCount;
  }
};

class NurbsSurfaceCapture : public StubInterface {
public:
  int m_callCount = 0;
  bool m_isNurbs = false;
  DRW_NurbsSurface m_captured;
  void addSurface(const DRW_Surface *d) override {
    if (m_callCount == 0) {
      const auto *nurbs = dynamic_cast<const DRW_NurbsSurface *>(d);
      m_isNurbs = nurbs != nullptr;
      if (nurbs != nullptr)
        m_captured = *nurbs;
    }
    ++m_callCount;
  }
};

class SurfaceListCapture : public StubInterface {
public:
  std::vector<DRW::ETYPE> m_types;
  std::vector<DRW_Surface> m_surfaces;
  void addSurface(const DRW_Surface *d) override {
    m_types.push_back(d->eType);
    m_surfaces.push_back(*d);
  }
};

class SurfaceEmitter : public StubInterface {
public:
  DRW_Surface *m_surface = nullptr;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeSurface(m_surface); }
};

class ModelerGeometryCapture : public StubInterface {
public:
  std::vector<DRW_ModelerGeometry> m_items;
  void addModelerGeometry(const DRW_ModelerGeometry &d) override {
    m_items.push_back(d);
  }
};

class MTextCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_MText m_captured;
  void addMText(const DRW_MText &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
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

class MissingBlockRecordEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_writeBlockResult = true;

  void writeBlocks() override {
    DRW_Block block;
    block.name = "UNREGISTERED";
    m_writeBlockResult = m_rw->writeBlock(&block);
  }
};

class CaseInsensitiveBlockEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  std::uint32_t m_lookup = DRW::NoHandle;
  bool m_writeBlockResult = false;

  void writeBlockRecords() override {
    REQUIRE(m_rw->writeBlockRecord("MixedCase"));
  }

  void writeBlocks() override {
    m_lookup = m_rw->getBlockRecordHandleToWrite("mixedcase");
    DRW_Block block;
    block.name = "MIXEDCASE";
    m_writeBlockResult = m_rw->writeBlock(&block);
  }
};

class DuplicateBlockRecordEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_firstResult = false;
  bool m_duplicateResult = true;

  void writeBlockRecords() override {
    m_firstResult = m_rw->writeBlockRecord("MixedCase");
    m_duplicateResult = m_rw->writeBlockRecord("mixedcase");
  }
};

class DimstyleBlockReferenceEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  int m_blockRecordHandle = -1;
  int m_headerBlockRecordHandle = -1;

  void writeHeader(DRW_Header &) override {
    m_headerBlockRecordHandle =
        m_rw->getBlockRecordHandleToWrite("LEADERBLOCK");
  }

  void writeBlockRecords() override {
    REQUIRE(m_rw->writeBlockRecord("LeaderBlock"));
  }

  void writeDimstyles() override {
    m_blockRecordHandle = m_rw->getBlockRecordHandleToWrite("leaderblock");
    DRW_Dimstyle dimstyle;
    dimstyle.name = "LeaderReference";
    dimstyle.dimldrblk = "LEADERBLOCK";
    dimstyle.dimblk = "LEADERBLOCK";
    dimstyle.dimblk1 = "LEADERBLOCK";
    dimstyle.dimblk2 = "LEADERBLOCK";
    dimstyle.add("_$DIMLDRBLK", 341,
                 m_rw->toHexStrHandle(m_blockRecordHandle));
    REQUIRE(m_rw->writeDimstyle(&dimstyle));
  }

  void writeBlocks() override {
    DRW_Block block;
    block.name = "leaderblock";
    REQUIRE(m_rw->writeBlock(&block));
  }
};

class R12BlockRecordOrderEmitter : public StubInterface {
public:
  std::vector<std::string> calls;

  void writeHeader(DRW_Header &) override { calls.push_back("header"); }
  void writeBlockRecords() override { calls.push_back("block-records"); }
};

class DimstyleLinetypeEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  std::uint32_t m_linetypeHandle = DRW::NoHandle;
  std::string m_linetypeName = "DIMLINE";

  void writeLTypes() override {
    DRW_LType linetype;
    linetype.name = m_linetypeName;
    REQUIRE(m_rw->writeLineType(&linetype));
    m_linetypeHandle = m_rw->getWritingContext()->lineTypesMap.back().second;
  }

  void writeDimstyles() override {
    REQUIRE(m_linetypeHandle > 0);
    const std::string handle = m_rw->toHexStrHandle(m_linetypeHandle);
    DRW_Dimstyle dimstyle;
    dimstyle.name = "LinetypeHandles";
    dimstyle.add("$DIMLTYPE", 345, handle);
    dimstyle.add("$DIMLTEX1", 346, handle);
    dimstyle.add("$DIMLTEX2", 347, handle);
    REQUIRE(m_rw->writeDimstyle(&dimstyle));
  }
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
  std::vector<DRW_Attdef> m_attdefs;
  void addRawDxfEntity(const DRW_RawDxfObject &d) override {
    m_entities.push_back(d);
  }
  void addAttDef(const DRW_Attdef &d) override {
    m_attdefs.push_back(d);
  }
};

// Emits a single LINE on write (for the thickness/extrusion field-drop test).
class LineEmitter : public StubInterface {
public:
  DRW_Line m_line;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeLine(&m_line); }
};

class ThreeDLineCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_3DLine m_captured;
  void add3DLine(const DRW_3DLine &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class ThreeDLineEmitter : public StubInterface {
public:
  DRW_3DLine m_line;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->write3DLine(&m_line); }
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

bool tryReadDxf(const std::string &dxf, DRW_Interface &cap, const char *name) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  const bool result = reader.read(&cap, /*ext=*/true);
  std::filesystem::remove(path);
  return result;
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

TEST_CASE("DXF ASCII binary groups retain their binary reader type",
          "[dxf][reader][binary]") {
  std::istringstream source("310\nA5\n");
  dxfReaderAscii reader(&source);
  int code = -1;

  REQUIRE(reader.readRec(&code));
  CHECK(code == 310);
  CHECK(reader.type == dxfReader::BINARY);
  CHECK(reader.getString() == "A5");
}

TEST_CASE("DXF reader clears state after a failed record",
          "[dxf][reader][malformed]") {
  std::istringstream source("10\n1.25\n10\nnot-a-number\n");
  dxfReaderAscii reader(&source);
  int code = -1;

  REQUIRE(reader.readRec(&code));
  CHECK(reader.type == dxfReader::DOUBLE);
  CHECK(reader.getDouble() == 1.25);

  CHECK_FALSE(reader.readRec(&code));
  CHECK(reader.type == dxfReader::INVALID);
  CHECK(reader.getRawValue().empty());
  CHECK(reader.getString().empty());
  CHECK(reader.getDouble() == 0.0);
  CHECK(reader.getInt32() == 0);
  CHECK(reader.getInt64() == 0);
  CHECK_FALSE(reader.getBool());
}

TEST_CASE("DXF reader clears state after a truncated comment",
          "[dxf][reader][malformed]") {
  std::istringstream source("999\n");
  dxfReaderAscii reader(&source);
  int code = -1;

  CHECK_FALSE(reader.readRec(&code));
  CHECK(reader.type == dxfReader::INVALID);
  CHECK(reader.getRawValue().empty());
  CHECK(reader.getString().empty());
  CHECK(reader.getDouble() == 0.0);
  CHECK(reader.getInt32() == 0);
  CHECK(reader.getInt64() == 0);
  CHECK_FALSE(reader.getBool());
}

TEST_CASE("DXF reader rejects null record arguments",
          "[dxf][reader][malformed]") {
  std::istringstream source;
  dxfReaderAscii reader(&source);
  CHECK_FALSE(reader.readRec(nullptr));
}

TEST_CASE("R12 binary DXF reader decodes native and escaped group codes",
          "[dxf][reader][binary][r12]") {
  std::string source;
  source.push_back('\0');
  source.append("LINE");
  source.push_back('\0');
  source.push_back(static_cast<char>(0xFF));
  source.push_back(static_cast<char>(0xE8));
  source.push_back(static_cast<char>(0x03));
  source.append("XDATA");
  source.push_back('\0');

  std::istringstream stream(source, std::ios::in | std::ios::binary);
  dxfReaderBinaryR12 reader(&stream);
  int code = -1;

  REQUIRE(reader.readRec(&code));
  CHECK(code == 0);
  CHECK(reader.type == dxfReader::STRING);
  CHECK(reader.getString() == "LINE");

  REQUIRE(reader.readRec(&code));
  CHECK(code == 1000);
  CHECK(reader.type == dxfReader::STRING);
  CHECK(reader.getString() == "XDATA");

  CHECK_FALSE(reader.readRec(&code));
}

TEST_CASE("R12 binary DXF writer emits native and escaped group codes",
          "[dxf][writer][binary][r12]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_r12_binary_codes.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream output(path, std::ios::binary);
    dxfWriterBinaryR12 writer(&output);
    REQUIRE(writer.writeString(0, "LINE"));
    REQUIRE(writer.writeInt16(70, 0x8001));
    REQUIRE(writer.writeString(1000, "XDATA"));
    CHECK_FALSE(writer.writeString(310, "AA"));
    CHECK_FALSE(writer.writeString(999, "comment"));
    CHECK(writer.hasWriteError());
  }

  std::ifstream input(path, std::ios::binary);
  const std::vector<std::uint8_t> bytes{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  const std::vector<std::uint8_t> expected{
      0x00, 'L', 'I', 'N', 'E', 0x00,
      0x46, 0x01, 0x80,
      0xFF, 0xE8, 0x03, 'X', 'D', 'A', 'T', 'A', 0x00};
  CHECK(bytes == expected);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF AC1009 binary export selects the R12 group-code form",
          "[dxf][writer][binary][r12]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_r12_binary_export.dxf";
  std::filesystem::remove(path);
  StubInterface source;
  dxfRW writer(path.string().c_str());
  REQUIRE(writer.write(&source, DRW::AC1009, true));

  std::ifstream input(path, std::ios::binary);
  const std::vector<std::uint8_t> bytes{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  constexpr std::size_t signatureSize = 22;
  REQUIRE(bytes.size() > signatureSize + 1);
  CHECK(bytes[signatureSize] == 0x00);
  CHECK(bytes[signatureSize + 1] == static_cast<std::uint8_t>('S'));

  StubInterface reread;
  dxfRW reader(path.string().c_str());
  CHECK(reader.read(&reread, /*ext=*/true));
  CHECK(reader.getVersion() == DRW::AC1009);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF writers reject non-finite doubles before output",
          "[dxf][writer][malformed]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_nonfinite_writer.dxf";
  const double value = std::numeric_limits<double>::quiet_NaN();

  {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    dxfWriterBinary writer(&output);
    CHECK_FALSE(writer.writeDouble(10, value));
    CHECK(writer.hasWriteError());
  }
  CHECK(std::filesystem::file_size(path) == 0);

  {
    std::ofstream output(path, std::ios::trunc);
    dxfWriterAscii writer(&output);
    CHECK_FALSE(writer.writeDouble(10, value));
    CHECK(writer.hasWriteError());
  }
  CHECK(std::filesystem::file_size(path) == 0);
  std::filesystem::remove(path);
}

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

TEST_CASE("DXF entity dispatch is case-insensitive", "[dxf][entity]") {
  LineCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nline\n5\n21\n8\nEntityLayer\n"
      "10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_entity_case_insensitive.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x21u);
  CHECK(cap.m_captured.layer == "EntityLayer");
}

TEST_CASE("DXF BLOCK requires an ENDBLK boundary", "[dxf][block][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_block_missing_endblk.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nBLOCKS\n"
           "0\nBLOCK\n5\n20\n8\nBlockLayer\n2\nB1\n70\n0\n"
           "0\nLINE\n5\n21\n8\nInnerLayer\n10\n0\n20\n0\n11\n1\n21\n1\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  StubInterface cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  std::filesystem::remove(path);
}

TEST_CASE("DXF writer rejects a BLOCK without its BLOCK_RECORD",
          "[dxf][block][writer][malformed]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_block_missing_record.dxf";
  std::filesystem::remove(path);

  MissingBlockRecordEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;

  CHECK_FALSE(writer.write(&source, DRW::AC1015, /*bin=*/false));
  CHECK_FALSE(source.m_writeBlockResult);
  std::filesystem::remove(path);
}

TEST_CASE("DXF BLOCK_RECORD names resolve case-insensitively",
          "[dxf][block][writer]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_block_case_insensitive.dxf";
  std::filesystem::remove(path);

  CaseInsensitiveBlockEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;

  REQUIRE(writer.write(&source, DRW::AC1015, /*bin=*/false));
  CHECK(source.m_lookup > 0);
  CHECK(source.m_writeBlockResult);
  std::filesystem::remove(path);
}

TEST_CASE("DXF writer rejects duplicate BLOCK_RECORD names",
          "[dxf][block][writer][malformed]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_duplicate_block_record.dxf";
  std::filesystem::remove(path);

  DuplicateBlockRecordEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;

  CHECK_FALSE(writer.write(&source, DRW::AC1015, /*bin=*/false));
  CHECK(source.m_firstResult);
  CHECK_FALSE(source.m_duplicateResult);
  std::filesystem::remove(path);
}

TEST_CASE("DXF reader rejects duplicate BLOCK_RECORD handles",
          "[dxf][block][malformed]") {
  const std::string dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nBLOCK_RECORD\n"
      "0\nBLOCK_RECORD\n5\n20\n2\n*Model_Space\n70\n0\n"
      "0\nBLOCK_RECORD\n5\n20\n2\nDUPLICATE\n70\n0\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_duplicate_block_record_handle.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  StubInterface capture;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, /*ext=*/true));
  CHECK(reader.getReadingContext()->blockRecordMap.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF reader rejects duplicate LTYPE handles",
          "[dxf][table][malformed]") {
  const std::string dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLTYPE\n"
      "0\nLTYPE\n5\n14\n2\nCONTINUOUS\n70\n0\n3\nSolid line\n72\n65\n73\n0\n40\n0.0\n"
      "0\nLTYPE\n5\n14\n2\nDUPLICATE\n70\n0\n3\nDuplicate\n72\n65\n73\n0\n40\n0.0\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  LTypeCapture capture;
  CHECK_FALSE(tryReadDxf(dxf, capture, "lc_duplicate_ltype_handle.dxf"));
  CHECK(capture.m_callCount == 0);
}

TEST_CASE("DXF reader rejects an unexpected LTYPE table boundary",
          "[dxf][table][malformed]") {
  const std::string dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLTYPE\n"
      "0\nLTYPE\n5\n14\n2\nCONTINUOUS\n70\n0\n3\nSolid line\n72\n65\n73\n0\n40\n0.0\n"
      "0\nNOT_A_LTYPE\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  LTypeCapture capture;
  CHECK_FALSE(tryReadDxf(dxf, capture,
                         "lc_unexpected_ltype_boundary.dxf"));
}

TEST_CASE("DXF reader rejects duplicate LAYER handles",
          "[dxf][table][malformed]") {
  const std::string dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLAYER\n"
      "0\nLAYER\n5\n10\n2\nLayerA\n70\n0\n62\n7\n6\nCONTINUOUS\n"
      "0\nLAYER\n5\n10\n2\nLayerB\n70\n0\n62\n7\n6\nCONTINUOUS\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  LayerCapture capture;
  CHECK_FALSE(tryReadDxf(dxf, capture, "lc_duplicate_layer_handle.dxf"));
  CHECK(capture.m_callCount == 0);
}

TEST_CASE("DXF reader rejects an incomplete modern table record",
          "[dxf][table][malformed]") {
  const std::string dxf =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLTYPE\n"
      "0\nLTYPE\n5\n14\n70\n0\n3\nSolid line\n72\n65\n73\n0\n40\n0.0\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  LTypeCapture capture;
  CHECK_FALSE(tryReadDxf(dxf, capture, "lc_incomplete_modern_table_record.dxf"));
  CHECK(capture.m_callCount == 0);
}

TEST_CASE("DXF LTYPE records publish and resolve at ENDTAB",
          "[dxf][table]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_ltype_table_commit.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nTABLES\n"
           "0\nTABLE\n2\nLTYPE\n"
           "0\nLTYPE\n5\n14\n2\nCONTINUOUS\n70\n0\n3\nSolid line\n"
           "72\n65\n73\n0\n40\n0.0\n"
           "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  }

  LTypeCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, /*ext=*/true));
  CHECK(capture.m_callCount == 1);
  CHECK(reader.getReadingContext()->resolveLineTypeName(0x14)
        == "CONTINUOUS");
  std::filesystem::remove(path);
}

TEST_CASE("DXF DIMSTYLE resolves forward BLOCK_RECORD references",
          "[dxf][block][dimstyle][writer]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dimstyle_block_record_reference.dxf";
  std::filesystem::remove(path);

  DimstyleBlockReferenceEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;

  REQUIRE(writer.write(&source, DRW::AC1015, /*bin=*/false));
  REQUIRE(source.m_blockRecordHandle > 0);
  CHECK(source.m_headerBlockRecordHandle == source.m_blockRecordHandle);

  std::ifstream input(path);
  const std::string output{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const std::string handle =
      source.m_rw->toHexStrHandle(source.m_blockRecordHandle);
  CHECK(output.find("341\n" + handle + "\n") != std::string::npos);
  CHECK(output.find("341\n" + handle + "\n") ==
        output.rfind("341\n" + handle + "\n"));
  CHECK(output.find("342\n" + handle + "\n") != std::string::npos);
  CHECK(output.find("343\n" + handle + "\n") != std::string::npos);
  CHECK(output.find("344\n" + handle + "\n") != std::string::npos);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF R12 keeps the BLOCK_RECORD callback after HEADER",
          "[dxf][block][r12][writer]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_r12_block_record_callback_order.dxf";
  std::filesystem::remove(path);

  R12BlockRecordOrderEmitter source;
  dxfRW writer(path.string().c_str());
  REQUIRE(writer.write(&source, DRW::AC1009, /*bin=*/false));
  REQUIRE(source.calls.size() == 2);
  CHECK(source.calls[0] == "header");
  CHECK(source.calls[1] == "block-records");
  std::filesystem::remove(path);
}

TEST_CASE("DXF R2007 DIMSTYLE emits linetype hard pointers",
          "[dxf][dimstyle][r2007][writer]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_r2007_dimstyle_linetype_handles.dxf";
  std::filesystem::remove(path);

  DimstyleLinetypeEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;
  REQUIRE(writer.write(&source, DRW::AC1021, /*bin=*/false));

  std::ifstream input(path);
  const std::string output{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const std::string handle = writer.toHexStrHandle(source.m_linetypeHandle);
  CHECK(output.find("345\n" + handle + "\n") != std::string::npos);
  CHECK(output.find("346\n" + handle + "\n") != std::string::npos);
  CHECK(output.find("347\n" + handle + "\n") != std::string::npos);
  CHECK(output.find("348\n" + handle + "\n") == std::string::npos);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF R2004 omits DIMSTYLE linetype hard pointers",
          "[dxf][dimstyle][r2004][writer]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_r2004_dimstyle_linetype_handles.dxf";
  std::filesystem::remove(path);

  DimstyleLinetypeEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;
  REQUIRE(writer.write(&source, DRW::AC1018, /*bin=*/false));

  std::ifstream input(path);
  const std::string output{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const std::string handle = writer.toHexStrHandle(source.m_linetypeHandle);
  CHECK(output.find("345\n" + handle + "\n") == std::string::npos);
  CHECK(output.find("346\n" + handle + "\n") == std::string::npos);
  CHECK(output.find("347\n" + handle + "\n") == std::string::npos);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF writing context preserves high-bit handles",
          "[dxf][handles][writer]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_high_bit_context_handles.dxf";
  std::filesystem::remove(path);

  DimstyleLinetypeEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;
  REQUIRE(writer.reserveHandle(0x80000000u));
  REQUIRE(writer.write(&source, DRW::AC1021, /*bin=*/false));
  REQUIRE(source.m_linetypeHandle > 0x80000000u);

  std::ifstream input(path);
  const std::string output{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const std::string handle = writer.toHexStrHandle(source.m_linetypeHandle);
  CHECK(output.find("  5\n" + handle + "\n") != std::string::npos);
  CHECK(output.find("345\n" + handle + "\n") != std::string::npos);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF writing context resets derived linetype state on reuse",
          "[dxf][handles][writer][reuse]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_linetype_context_reuse.dxf";
  std::filesystem::remove(path);

  DimstyleLinetypeEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;
  REQUIRE(writer.write(&source, DRW::AC1021, /*bin=*/false));
  REQUIRE(writer.getWritingContext()->lineTypesMap.size() == 1);

  source.m_linetypeName = "DIMLINE_REUSED";
  REQUIRE(writer.write(&source, DRW::AC1021, /*bin=*/false));
  REQUIRE(writer.getWritingContext()->lineTypesMap.size() == 1);
  CHECK(writer.getWritingContext()->lineTypesMap.front().first
        == "DIMLINE_REUSED");
  std::ifstream input(path);
  const std::string output{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  CHECK(output.find("2\nDIMLINE_REUSED\n") != std::string::npos);
  CHECK(output.find("2\nDIMLINE\n") == std::string::npos);

  std::filesystem::remove(path);
}

TEST_CASE("DXF BLOCK handle arithmetic remains unsigned",
          "[dxf][handles][block][writer]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_high_bit_block_handles.dxf";
  std::filesystem::remove(path);

  CaseInsensitiveBlockEmitter source;
  dxfRW writer(path.string().c_str());
  source.m_rw = &writer;
  REQUIRE(writer.reserveHandle(0x80000000u));
  REQUIRE(writer.write(&source, DRW::AC1021, /*bin=*/false));
  REQUIRE(source.m_lookup > 0x80000000u);

  std::ifstream input(path);
  const std::string output{std::istreambuf_iterator<char>(input),
                           std::istreambuf_iterator<char>()};
  const std::string blockHandle = writer.toHexStrHandle(source.m_lookup + 1);
  const std::string endBlockHandle = writer.toHexStrHandle(source.m_lookup + 2);
  CHECK(output.find("5\n" + blockHandle + "\n") != std::string::npos);
  CHECK(output.find("5\n" + endBlockHandle + "\n") != std::string::npos);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF entity callbacks reject section boundaries in the wrong scope",
          "[dxf][entities][malformed]") {
  const std::vector<std::string> inputs = {
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n21\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDBLK\n5\n22\n0\nENDSEC\n0\nEOF\n",
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n20\n8\n0\n2\nB1\n70\n0\n"
      "0\nLINE\n5\n21\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDSEC\n0\nEOF\n",
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n21\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\n\n0\nENDSEC\n0\nEOF\n"};

  for (std::size_t i = 0; i < inputs.size(); ++i) {
    LineCapture cap;
    const std::string name =
        "lc_entity_boundary_scope_" + std::to_string(i) + ".dxf";
    CHECK_FALSE(tryReadDxf(inputs[i], cap, name.c_str()));
    CHECK(cap.m_callCount == 0);
  }
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

TEST_CASE("DXF nested 102 groups preserve their marker stack", "[dxf][102]") {
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n21\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\nD\n"
      "102\n{NESTED\n330\nE\n102\n}\n102\n}\n"
      "8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDSEC\n0\nEOF\n";

  LineCapture source;
  readDxf(dxf, source, "lc_nested_102_source.dxf");
  REQUIRE(source.m_callCount == 1);
  REQUIRE(source.m_captured.appData.size() == 1);

  const auto path =
      std::filesystem::temp_directory_path() / "lc_nested_102_roundtrip.dxf";
  std::filesystem::remove(path);
  LineEmitter emitter;
  emitter.m_line = source.m_captured;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  LineCapture roundTrip;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&roundTrip, /*ext=*/true));
  }
  REQUIRE(roundTrip.m_callCount == 1);
  REQUIRE(roundTrip.m_captured.appData.size() == 1);

  std::vector<std::string> markers;
  for (const DRW_Variant &value : roundTrip.m_captured.appData.front()) {
    if (value.code() == 102)
      markers.emplace_back(value.c_str());
  }
  REQUIRE(markers.size() == 4);
  CHECK(markers[0] == "ACAD_REACTORS");
  CHECK(markers[1] == "{NESTED");
  CHECK(markers[2] == "}");
  CHECK(markers[3] == "}");
  std::filesystem::remove(path);
}

TEST_CASE("DXF writer rejects unbalanced 102 application groups", "[dxf][102][writer]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_unbalanced_102_write.dxf";
  std::filesystem::remove(path);

  LineEmitter emitter;
  auto &group = emitter.m_line.appData.emplace_back();
  group.emplace_back(102, std::string{"ACAD_REACTORS"});
  group.emplace_back(330, std::string{"D"});
  group.emplace_back(102, std::string{"{NESTED"});
  group.emplace_back(102, std::string{"}"});

  dxfRW writer(path.string().c_str());
  emitter.m_rw = &writer;
  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
  std::filesystem::remove(path);
}

TEST_CASE("DXF writer rejects invalid 102 application markers", "[dxf][102][writer]") {
  const std::array<std::list<DRW_Variant>, 3> invalidGroups = {{
      {DRW_Variant(102, 1)},
      {DRW_Variant(102, std::string{"ACAD_REACTORS"}),
       DRW_Variant(102, std::string{})},
      {DRW_Variant(330, std::string{"D"})},
  }};

  for (std::size_t i = 0; i < invalidGroups.size(); ++i) {
    const auto path = std::filesystem::temp_directory_path()
        / ("lc_invalid_102_write_" + std::to_string(i) + ".dxf");
    std::filesystem::remove(path);

    LineEmitter emitter;
    emitter.m_line.appData.push_back(invalidGroups[i]);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF malformed 102 groups do not publish partial entities",
          "[dxf][102][malformed]") {
  const std::vector<std::string> inputs = {
      "0\nSECTION\n2\nENTITIES\n0\nLINE\n"
      "102\n{ACAD_REACTORS\n8\n0\n0\nENDSEC\n0\nEOF\n",
      "0\nSECTION\n2\nENTITIES\n0\nLINE\n"
      "102\n}\n8\n0\n0\nENDSEC\n0\nEOF\n",
      "0\nSECTION\n2\nENTITIES\n0\nLINE\n"
      "102\n{\n102\n}\n8\n0\n0\nENDSEC\n0\nEOF\n"};

  for (std::size_t i = 0; i < inputs.size(); ++i) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_malformed_102_" + std::to_string(i) + ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << inputs[i];
    }

    LineCapture cap;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&cap, /*ext=*/true));
    CHECK(cap.m_callCount == 0);
    std::filesystem::remove(path);
  }
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
TEST_CASE("DXF surface subtypes preserve text ACIS payloads",
          "[dxf][surface][subtypes][acis]") {
  const std::vector<std::pair<const char *, DRW::ETYPE>> types = {
      {"EXTRUDEDSURFACE", DRW::EXTRUDEDSURFACE},
      {"LOFTEDSURFACE", DRW::LOFTEDSURFACE},
      {"REVOLVEDSURFACE", DRW::REVOLVEDSURFACE},
      {"SWEPTSURFACE", DRW::SWEPTSURFACE},
      {"PLANESURFACE", DRW::PLANESURFACE},
      {"NURBSSURFACE", DRW::NURBSURFACE},
  };

  std::string dxf = "0\nSECTION\n2\nENTITIES\n";
  std::size_t handle = 0x700;
  for (const auto &[name, type] : types) {
    (void)type;
    dxf += std::string("0\n") + name + "\n5\n" +
           std::to_string(handle++) +
           "\n330\n1F\n100\nAcDbEntity\n8\n0\n"
           "100\nAcDbModelerGeometry\n1\nACIS BinaryFile\n"
           "70\n7\n1\npayload-a\n3\npayload-b\n"
           "100\nAcDbSurface\n71\n4\n72\n5\n";
  }
  dxf += "0\nENDSEC\n0\nEOF\n";

  SurfaceListCapture cap;
  readDxf(dxf, cap, "lc_surface_subtypes_text_acis.dxf");
  REQUIRE(cap.m_types == std::vector<DRW::ETYPE>{
      DRW::EXTRUDEDSURFACE, DRW::LOFTEDSURFACE, DRW::REVOLVEDSURFACE,
      DRW::SWEPTSURFACE, DRW::PLANESURFACE, DRW::NURBSURFACE});
  for (const DRW_Surface &surface : cap.m_surfaces) {
    CHECK(surface.modelerFormatVersion == 7);
    CHECK(surface.uIsolines == 4);
    CHECK(surface.vIsolines == 5);
    CHECK(surface.rawAcisData == std::vector<std::uint8_t>{
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e',
        'p', 'a', 'y', 'l', 'o', 'a', 'd', '-', 'a', 'p', 'a', 'y', 'l', 'o', 'a',
        'd', '-', 'b'});
  }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF surface writer emits modeler and subtype subclasses",
          "[dxf][surface][subtypes][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_surface_subtype_writer.dxf";
  std::filesystem::remove(path);

  DRW_ExtrudedSurface source;
  source.layer = "0";
  source.modelerFormatVersion = 2;
  source.uIsolines = 3;
  source.vIsolines = 4;
  source.rawAcisData = {'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y',
                        'F', 'i', 'l', 'e', ' ', 's', 'u', 'r', 'f', 'a', 'c', 'e'};

  SurfaceEmitter emitter;
  emitter.m_surface = &source;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  std::ifstream serializedFile(path);
  const std::string serialized((std::istreambuf_iterator<char>(serializedFile)),
                               std::istreambuf_iterator<char>());
  serializedFile.close();
  CHECK(serialized.find("AcDbModelerGeometry") != std::string::npos);
  CHECK(serialized.find("AcDbExtrudedSurface") != std::string::npos);

  SurfaceCapture capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, true));
  }
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount == 1);
  CHECK(capture.m_captured.eType == DRW::EXTRUDEDSURFACE);
  CHECK(capture.m_captured.modelerFormatVersion == 2);
  CHECK(capture.m_captured.uIsolines == 3);
  CHECK(capture.m_captured.vIsolines == 4);
  CHECK(capture.m_captured.rawAcisData == source.rawAcisData);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF revolved surface retains subtype fields",
          "[dxf][surface][revolved][subtypes]") {
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nREVOLVEDSURFACE\n5\n701\n330\n1F\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbModelerGeometry\n70\n1\n"
      "100\nAcDbSurface\n71\n2\n72\n3\n"
      "100\nAcDbRevolvedSurface\n90\n36\n"
      "10\n1.0\n20\n2.0\n30\n3.0\n"
      "11\n0.0\n21\n0.0\n31\n1.0\n"
      "40\n1.25\n41\n-0.5\n"
      "42\n1\n42\n2\n42\n3\n42\n4\n"
      "42\n5\n42\n6\n42\n7\n42\n8\n"
      "42\n9\n42\n10\n42\n11\n42\n12\n"
      "42\n13\n42\n14\n42\n15\n42\n16\n"
      "43\n0.125\n44\n2.5\n45\n3.5\n46\n0.75\n"
      "290\n1\n291\n1\n"
      "0\nENDSEC\n0\nEOF\n";

  RevolvedSurfaceCapture capture;
  readDxf(dxf, capture, "lc_revolved_surface_fields.dxf");
  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isRevolved);
  CHECK(capture.m_captured.classId == 36u);
  CHECK(capture.m_captured.axisPoint.x == 1.0);
  CHECK(capture.m_captured.axisPoint.y == 2.0);
  CHECK(capture.m_captured.axisPoint.z == 3.0);
  CHECK(capture.m_captured.axisVector.x == 0.0);
  CHECK(capture.m_captured.axisVector.y == 0.0);
  CHECK(capture.m_captured.axisVector.z == 1.0);
  CHECK(capture.m_captured.revolveAngle == 1.25);
  CHECK(capture.m_captured.startAngle == -0.5);
  CHECK(capture.m_captured.transform[0] == 1.0);
  CHECK(capture.m_captured.transform[15] == 16.0);
  CHECK(capture.m_captured.draftAngle == 0.125);
  CHECK(capture.m_captured.draftStartDistance == 2.5);
  CHECK(capture.m_captured.draftEndDistance == 3.5);
  CHECK(capture.m_captured.twistAngle == 0.75);
  CHECK(capture.m_captured.solid);
  CHECK(capture.m_captured.closeToAxis);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF revolved surface writer round-trips subtype fields",
          "[dxf][surface][revolved][subtypes][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_revolved_surface_writer.dxf";
  std::filesystem::remove(path);

  DRW_RevolvedSurface source;
  source.classId = 36;
  source.axisPoint = DRW_Coord(1.0, 2.0, 3.0);
  source.axisVector = DRW_Coord(0.0, 0.0, 1.0);
  source.revolveAngle = 1.25;
  source.startAngle = -0.5;
  for (std::size_t i = 0; i < source.transform.size(); ++i)
    source.transform[i] = static_cast<double>(i + 1);
  source.draftAngle = 0.125;
  source.draftStartDistance = 2.5;
  source.draftEndDistance = 3.5;
  source.twistAngle = 0.75;
  source.solid = true;
  source.closeToAxis = true;

  SurfaceEmitter emitter;
  emitter.m_surface = &source;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  RevolvedSurfaceCapture capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, true));
  }
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isRevolved);
  CHECK(capture.m_captured.classId == source.classId);
  CHECK(capture.m_captured.axisPoint.x == source.axisPoint.x);
  CHECK(capture.m_captured.axisPoint.y == source.axisPoint.y);
  CHECK(capture.m_captured.axisPoint.z == source.axisPoint.z);
  CHECK(capture.m_captured.axisVector.x == source.axisVector.x);
  CHECK(capture.m_captured.axisVector.y == source.axisVector.y);
  CHECK(capture.m_captured.axisVector.z == source.axisVector.z);
  CHECK(capture.m_captured.revolveAngle == source.revolveAngle);
  CHECK(capture.m_captured.startAngle == source.startAngle);
  CHECK(capture.m_captured.transform == source.transform);
  CHECK(capture.m_captured.draftAngle == source.draftAngle);
  CHECK(capture.m_captured.draftStartDistance == source.draftStartDistance);
  CHECK(capture.m_captured.draftEndDistance == source.draftEndDistance);
  CHECK(capture.m_captured.twistAngle == source.twistAngle);
  CHECK(capture.m_captured.solid);
  CHECK(capture.m_captured.closeToAxis);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF extruded surface retains subtype fields",
          "[dxf][surface][extruded][subtypes]") {
  std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nEXTRUDEDSURFACE\n5\n702\n330\n1F\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbModelerGeometry\n70\n2\n"
      "100\nAcDbSurface\n71\n3\n72\n4\n"
      "100\nAcDbExtrudedSurface\n90\n37\n"
      "10\n1.0\n20\n2.0\n30\n3.0\n";
  for (int i = 1; i <= 16; ++i)
    dxf += "40\n" + std::to_string(i) + "\n";
  dxf += "42\n0.125\n43\n2.5\n44\n3.5\n45\n0.75\n"
         "48\n1.25\n49\n0.5\n";
  for (int i = 17; i <= 32; ++i)
    dxf += "46\n" + std::to_string(i) + "\n";
  for (int i = 33; i <= 48; ++i)
    dxf += "47\n" + std::to_string(i) + "\n";
  dxf += "290\n1\n70\n5\n71\n6\n"
         "292\n1\n293\n1\n294\n1\n295\n1\n296\n1\n"
         "11\n4.0\n21\n5.0\n31\n6.0\n"
         "0\nENDSEC\n0\nEOF\n";

  ExtrudedSurfaceCapture capture;
  readDxf(dxf, capture, "lc_extruded_surface_fields.dxf");
  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isExtruded);
  CHECK(capture.m_captured.classId == 37u);
  CHECK(capture.m_captured.modelerFormatVersion == 2);
  CHECK(capture.m_captured.uIsolines == 3);
  CHECK(capture.m_captured.vIsolines == 4);
  CHECK(capture.m_captured.sweepVector.x == 1.0);
  CHECK(capture.m_captured.sweepVector.y == 2.0);
  CHECK(capture.m_captured.sweepVector.z == 3.0);
  CHECK(capture.m_captured.extrudedTransform[0] == 1.0);
  CHECK(capture.m_captured.extrudedTransform[15] == 16.0);
  CHECK(capture.m_captured.draftAngle == 0.125);
  CHECK(capture.m_captured.draftStartDistance == 2.5);
  CHECK(capture.m_captured.draftEndDistance == 3.5);
  CHECK(capture.m_captured.twistAngle == 0.75);
  CHECK(capture.m_captured.scaleFactor == 1.25);
  CHECK(capture.m_captured.alignAngle == 0.5);
  CHECK(capture.m_captured.sweepEntityTransform[0] == 17.0);
  CHECK(capture.m_captured.sweepEntityTransform[15] == 32.0);
  CHECK(capture.m_captured.pathEntityTransform[0] == 33.0);
  CHECK(capture.m_captured.pathEntityTransform[15] == 48.0);
  CHECK(capture.m_captured.solid);
  CHECK(capture.m_captured.sweepAlignmentFlags == 5);
  CHECK(capture.m_captured.pathFlags == 6);
  CHECK(capture.m_captured.alignStart);
  CHECK(capture.m_captured.bank);
  CHECK(capture.m_captured.basePointSet);
  CHECK(capture.m_captured.sweepEntityTransformComputed);
  CHECK(capture.m_captured.pathEntityTransformComputed);
  CHECK(capture.m_captured.referenceVector.x == 4.0);
  CHECK(capture.m_captured.referenceVector.y == 5.0);
  CHECK(capture.m_captured.referenceVector.z == 6.0);
}

TEST_CASE("DXF extruded surface rejects a non-16-value transform",
          "[dxf][surface][extruded][malformed]") {
  for (const int transformCount : {15, 17}) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_extruded_surface_transform_"
                       + std::to_string(transformCount) + ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
             "0\nEXTRUDEDSURFACE\n8\n0\n"
             "100\nAcDbEntity\n100\nAcDbModelerGeometry\n"
             "70\n2\n100\nAcDbSurface\n71\n3\n72\n4\n"
             "100\nAcDbExtrudedSurface\n90\n37\n"
             "10\n1\n20\n2\n30\n3\n";
      for (int i = 0; i < transformCount; ++i)
        out << "40\n" << i << "\n";
      out << "0\nENDSEC\n0\nEOF\n";
    }

    ExtrudedSurfaceCapture capture;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&capture, /*ext=*/true));
    CHECK(capture.m_callCount == 0);
    std::filesystem::remove(path);
  }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF extruded surface writer round-trips subtype fields",
          "[dxf][surface][extruded][subtypes][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_extruded_surface_writer.dxf";
  std::filesystem::remove(path);

  DRW_ExtrudedSurface source;
  source.classId = 37;
  source.modelerFormatVersion = 2;
  source.uIsolines = 3;
  source.vIsolines = 4;
  source.sweepVector = DRW_Coord(1.0, 2.0, 3.0);
  for (std::size_t i = 0; i < source.extrudedTransform.size(); ++i) {
    source.extrudedTransform[i] = static_cast<double>(i + 1);
    source.sweepEntityTransform[i] = static_cast<double>(i + 17);
    source.pathEntityTransform[i] = static_cast<double>(i + 33);
  }
  source.draftAngle = 0.125;
  source.draftStartDistance = 2.5;
  source.draftEndDistance = 3.5;
  source.twistAngle = 0.75;
  source.scaleFactor = 1.25;
  source.alignAngle = 0.5;
  source.solid = true;
  source.sweepAlignmentFlags = 5;
  source.pathFlags = 6;
  source.alignStart = true;
  source.bank = true;
  source.basePointSet = true;
  source.sweepEntityTransformComputed = true;
  source.pathEntityTransformComputed = true;
  source.referenceVector = DRW_Coord(4.0, 5.0, 6.0);

  SurfaceEmitter emitter;
  emitter.m_surface = &source;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  ExtrudedSurfaceCapture capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, true));
  }
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isExtruded);
  CHECK(capture.m_captured.classId == source.classId);
  CHECK(capture.m_captured.modelerFormatVersion == source.modelerFormatVersion);
  CHECK(capture.m_captured.uIsolines == source.uIsolines);
  CHECK(capture.m_captured.vIsolines == source.vIsolines);
  CHECK(capture.m_captured.sweepVector.x == source.sweepVector.x);
  CHECK(capture.m_captured.sweepVector.y == source.sweepVector.y);
  CHECK(capture.m_captured.sweepVector.z == source.sweepVector.z);
  CHECK(capture.m_captured.extrudedTransform == source.extrudedTransform);
  CHECK(capture.m_captured.sweepEntityTransform == source.sweepEntityTransform);
  CHECK(capture.m_captured.pathEntityTransform == source.pathEntityTransform);
  CHECK(capture.m_captured.draftAngle == source.draftAngle);
  CHECK(capture.m_captured.draftStartDistance == source.draftStartDistance);
  CHECK(capture.m_captured.draftEndDistance == source.draftEndDistance);
  CHECK(capture.m_captured.twistAngle == source.twistAngle);
  CHECK(capture.m_captured.scaleFactor == source.scaleFactor);
  CHECK(capture.m_captured.alignAngle == source.alignAngle);
  CHECK(capture.m_captured.solid);
  CHECK(capture.m_captured.sweepAlignmentFlags == source.sweepAlignmentFlags);
  CHECK(capture.m_captured.pathFlags == source.pathFlags);
  CHECK(capture.m_captured.alignStart);
  CHECK(capture.m_captured.bank);
  CHECK(capture.m_captured.basePointSet);
  CHECK(capture.m_captured.sweepEntityTransformComputed);
  CHECK(capture.m_captured.pathEntityTransformComputed);
  CHECK(capture.m_captured.referenceVector.x == source.referenceVector.x);
  CHECK(capture.m_captured.referenceVector.y == source.referenceVector.y);
  CHECK(capture.m_captured.referenceVector.z == source.referenceVector.z);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF swept surface retains subtype fields",
          "[dxf][surface][swept][subtypes]") {
  std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nSWEPTSURFACE\n5\n703\n330\n1F\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbModelerGeometry\n70\n2\n"
      "310\n41434953\n"
      "100\nAcDbSurface\n71\n3\n72\n4\n"
      "100\nAcDbSweptSurface\n90\n101\n310\nAABB\n"
      "91\n202\n310\nCCDD\n";
  for (int i = 1; i <= 16; ++i)
    dxf += "40\n" + std::to_string(i) + "\n";
  for (int i = 17; i <= 32; ++i)
    dxf += "41\n" + std::to_string(i) + "\n";
  dxf += "42\n0.125\n43\n2.5\n44\n3.5\n45\n0.75\n"
         "48\n1.25\n49\n0.5\n";
  for (int i = 33; i <= 48; ++i)
    dxf += "46\n" + std::to_string(i) + "\n";
  for (int i = 49; i <= 64; ++i)
    dxf += "47\n" + std::to_string(i) + "\n";
  dxf += "290\n1\n70\n5\n71\n6\n"
         "292\n1\n293\n1\n294\n1\n295\n1\n296\n1\n"
         "11\n4.0\n21\n5.0\n31\n6.0\n"
         "0\nENDSEC\n0\nEOF\n";

  SweptSurfaceCapture capture;
  readDxf(dxf, capture, "lc_swept_surface_fields.dxf");
  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isSwept);
  CHECK(capture.m_captured.modelerFormatVersion == 2);
  CHECK(capture.m_captured.uIsolines == 3);
  CHECK(capture.m_captured.vIsolines == 4);
  CHECK(capture.m_captured.sweepEntityId == 101u);
  CHECK(capture.m_captured.sweepData == std::vector<std::uint8_t>{0xAA, 0xBB});
  CHECK(capture.m_captured.pathEntityId == 202u);
  CHECK(capture.m_captured.pathData == std::vector<std::uint8_t>{0xCC, 0xDD});
  CHECK(capture.m_captured.rawAcisData == std::vector<std::uint8_t>{
      0x41, 0x43, 0x49, 0x53});
  CHECK(capture.m_captured.sweepEntityTransformed[0] == 1.0);
  CHECK(capture.m_captured.sweepEntityTransformed[15] == 16.0);
  CHECK(capture.m_captured.pathEntityTransformed[0] == 17.0);
  CHECK(capture.m_captured.pathEntityTransformed[15] == 32.0);
  CHECK(capture.m_captured.draftAngle == 0.125);
  CHECK(capture.m_captured.draftStartDistance == 2.5);
  CHECK(capture.m_captured.draftEndDistance == 3.5);
  CHECK(capture.m_captured.twistAngle == 0.75);
  CHECK(capture.m_captured.scaleFactor == 1.25);
  CHECK(capture.m_captured.alignAngle == 0.5);
  CHECK(capture.m_captured.sweepEntityTransform[0] == 33.0);
  CHECK(capture.m_captured.sweepEntityTransform[15] == 48.0);
  CHECK(capture.m_captured.pathEntityTransform[0] == 49.0);
  CHECK(capture.m_captured.pathEntityTransform[15] == 64.0);
  CHECK(capture.m_captured.solid);
  CHECK(capture.m_captured.sweepAlignmentFlags == 5);
  CHECK(capture.m_captured.pathFlags == 6);
  CHECK(capture.m_captured.alignStart);
  CHECK(capture.m_captured.bank);
  CHECK(capture.m_captured.basePointSet);
  CHECK(capture.m_captured.sweepEntityTransformComputed);
  CHECK(capture.m_captured.pathEntityTransformComputed);
  CHECK(capture.m_captured.referenceVector.x == 4.0);
  CHECK(capture.m_captured.referenceVector.y == 5.0);
  CHECK(capture.m_captured.referenceVector.z == 6.0);
}

TEST_CASE("DXF swept surface rejects a non-16-value transform",
          "[dxf][surface][swept][malformed]") {
  for (const int transformCount : {15, 17}) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_swept_surface_transform_"
                       + std::to_string(transformCount) + ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
             "0\nSWEPTSURFACE\n8\n0\n"
             "100\nAcDbEntity\n100\nAcDbModelerGeometry\n"
             "70\n2\n100\nAcDbSurface\n71\n3\n72\n4\n"
             "100\nAcDbSweptSurface\n90\n101\n91\n202\n";
      for (int i = 0; i < transformCount; ++i)
        out << "40\n" << i << "\n";
      out << "0\nENDSEC\n0\nEOF\n";
    }

    SweptSurfaceCapture capture;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&capture, /*ext=*/true));
    CHECK(capture.m_callCount == 0);
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF swept surface rejects oversized subtype binary data",
          "[dxf][surface][swept][malformed]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_swept_surface_oversized_data.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nSWEPTSURFACE\n8\n0\n"
           "100\nAcDbEntity\n100\nAcDbModelerGeometry\n"
           "70\n2\n100\nAcDbSurface\n71\n3\n72\n4\n"
           "100\nAcDbSweptSurface\n90\n101\n310\n"
        << std::string(2 * (DRW_SweptSurface::kMaxSweepDataSize + 1), 'A')
        << "\n0\nENDSEC\n0\nEOF\n";
  }

  SweptSurfaceCapture capture;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, /*ext=*/true));
  CHECK(capture.m_callCount == 0);
  std::filesystem::remove(path);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF swept surface writer round-trips subtype fields",
          "[dxf][surface][swept][subtypes][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_swept_surface_writer.dxf";
  std::filesystem::remove(path);

  DRW_SweptSurface source;
  source.modelerFormatVersion = 2;
  source.uIsolines = 3;
  source.vIsolines = 4;
  source.rawAcisData = {0x41, 0x43, 0x49, 0x53};
  source.sweepEntityId = 101;
  source.sweepData = {0xAA, 0xBB};
  source.pathEntityId = 202;
  source.pathData = {0xCC, 0xDD};
  for (std::size_t i = 0; i < source.sweepEntityTransformed.size(); ++i) {
    source.sweepEntityTransformed[i] = static_cast<double>(i + 1);
    source.pathEntityTransformed[i] = static_cast<double>(i + 17);
    source.sweepEntityTransform[i] = static_cast<double>(i + 33);
    source.pathEntityTransform[i] = static_cast<double>(i + 49);
  }
  source.draftAngle = 0.125;
  source.draftStartDistance = 2.5;
  source.draftEndDistance = 3.5;
  source.twistAngle = 0.75;
  source.scaleFactor = 1.25;
  source.alignAngle = 0.5;
  source.solid = true;
  source.sweepAlignmentFlags = 5;
  source.pathFlags = 6;
  source.alignStart = true;
  source.bank = true;
  source.basePointSet = true;
  source.sweepEntityTransformComputed = true;
  source.pathEntityTransformComputed = true;
  source.referenceVector = DRW_Coord(4.0, 5.0, 6.0);

  SurfaceEmitter emitter;
  emitter.m_surface = &source;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  SweptSurfaceCapture capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, true));
  }
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isSwept);
  CHECK(capture.m_captured.sweepEntityId == source.sweepEntityId);
  CHECK(capture.m_captured.sweepData == source.sweepData);
  CHECK(capture.m_captured.pathEntityId == source.pathEntityId);
  CHECK(capture.m_captured.pathData == source.pathData);
  CHECK(capture.m_captured.sweepEntityTransformed == source.sweepEntityTransformed);
  CHECK(capture.m_captured.pathEntityTransformed == source.pathEntityTransformed);
  CHECK(capture.m_captured.sweepEntityTransform == source.sweepEntityTransform);
  CHECK(capture.m_captured.pathEntityTransform == source.pathEntityTransform);
  CHECK(capture.m_captured.draftAngle == source.draftAngle);
  CHECK(capture.m_captured.draftStartDistance == source.draftStartDistance);
  CHECK(capture.m_captured.draftEndDistance == source.draftEndDistance);
  CHECK(capture.m_captured.twistAngle == source.twistAngle);
  CHECK(capture.m_captured.scaleFactor == source.scaleFactor);
  CHECK(capture.m_captured.alignAngle == source.alignAngle);
  CHECK(capture.m_captured.solid);
  CHECK(capture.m_captured.sweepAlignmentFlags == source.sweepAlignmentFlags);
  CHECK(capture.m_captured.pathFlags == source.pathFlags);
  CHECK(capture.m_captured.alignStart);
  CHECK(capture.m_captured.bank);
  CHECK(capture.m_captured.basePointSet);
  CHECK(capture.m_captured.sweepEntityTransformComputed);
  CHECK(capture.m_captured.pathEntityTransformComputed);
  CHECK(capture.m_captured.referenceVector.x == source.referenceVector.x);
  CHECK(capture.m_captured.referenceVector.y == source.referenceVector.y);
  CHECK(capture.m_captured.referenceVector.z == source.referenceVector.z);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF lofted surface retains subtype fields and references",
          "[dxf][surface][lofted][subtypes]") {
  std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLOFTEDSURFACE\n5\n705\n330\n1F\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbModelerGeometry\n70\n2\n"
      "310\n41434953\n"
      "100\nAcDbSurface\n71\n3\n72\n4\n"
      "100\nAcDbLoftedSurface\n";
  for (int i = 1; i <= 16; ++i)
    dxf += "40\n" + std::to_string(i) + "\n";
  dxf += "90\n77\n90\n544\n310\nAABB\n"
         "90\n77\n90\n608\n310\nCCDD\n"
         "70\n2\n41\n0.125\n42\n0.25\n43\n1.5\n44\n2.5\n"
         "290\n1\n291\n0\n292\n1\n293\n0\n294\n1\n295\n0\n"
         "296\n1\n297\n0\n5\nABC\n"
         "0\nENDSEC\n0\nEOF\n";

  LoftedSurfaceCapture capture;
  readDxf(dxf, capture, "lc_lofted_surface_fields.dxf");
  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isLofted);
  CHECK(capture.m_captured.modelerFormatVersion == 2);
  CHECK(capture.m_captured.uIsolines == 3);
  CHECK(capture.m_captured.vIsolines == 4);
  CHECK(capture.m_captured.rawAcisData == std::vector<std::uint8_t>{
      0x41, 0x43, 0x49, 0x53});
  CHECK(capture.m_captured.loftEntityTransform[0] == 1.0);
  CHECK(capture.m_captured.loftEntityTransform[15] == 16.0);
  CHECK(capture.m_captured.planeNormalLoftingType == 2);
  CHECK(capture.m_captured.startDraftAngle == 0.125);
  CHECK(capture.m_captured.endDraftAngle == 0.25);
  CHECK(capture.m_captured.startDraftMagnitude == 1.5);
  CHECK(capture.m_captured.endDraftMagnitude == 2.5);
  CHECK(capture.m_captured.arcLengthParameterization);
  CHECK_FALSE(capture.m_captured.noTwist);
  CHECK(capture.m_captured.alignDirection);
  CHECK_FALSE(capture.m_captured.simpleSurfaces);
  CHECK(capture.m_captured.closedSurfaces);
  CHECK_FALSE(capture.m_captured.solid);
  CHECK(capture.m_captured.ruledSurface);
  CHECK_FALSE(capture.m_captured.virtualGuide);
  CHECK(capture.m_captured.pathCurveHandle == 0xABCu);
  REQUIRE(capture.m_captured.dxfReferenceData.size() == 6);
  CHECK(capture.m_captured.dxfReferenceData[0].code() == 90);
  CHECK(capture.m_captured.dxfReferenceData[0].i_val() == 77);
  CHECK(capture.m_captured.dxfReferenceData[1].i_val() == 544);
  CHECK(capture.m_captured.dxfReferenceData[2].code() == 310);
  CHECK(*capture.m_captured.dxfReferenceData[2].binary()
        == std::vector<std::uint8_t>{0xAA, 0xBB});
  CHECK(capture.m_captured.dxfReferenceData[3].i_val() == 77);
  CHECK(capture.m_captured.dxfReferenceData[4].i_val() == 608);
  CHECK(*capture.m_captured.dxfReferenceData[5].binary()
        == std::vector<std::uint8_t>{0xCC, 0xDD});
}

TEST_CASE("DXF lofted surface rejects malformed subtype data",
          "[dxf][surface][lofted][malformed]") {
  for (const int transformCount : {15, 17}) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_lofted_surface_transform_"
                       + std::to_string(transformCount) + ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
             "0\nLOFTEDSURFACE\n8\n0\n"
             "100\nAcDbEntity\n100\nAcDbModelerGeometry\n"
             "70\n2\n100\nAcDbSurface\n71\n3\n72\n4\n"
             "100\nAcDbLoftedSurface\n";
      for (int i = 0; i < transformCount; ++i)
        out << "40\n" << i << "\n";
      out << "0\nENDSEC\n0\nEOF\n";
    }

    LoftedSurfaceCapture capture;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&capture, /*ext=*/true));
    CHECK(capture.m_callCount == 0);
    std::filesystem::remove(path);
  }

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_lofted_surface_oversized_data.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nLOFTEDSURFACE\n8\n0\n"
           "100\nAcDbEntity\n100\nAcDbModelerGeometry\n"
           "70\n2\n100\nAcDbSurface\n71\n3\n72\n4\n"
           "100\nAcDbLoftedSurface\n";
    for (int i = 0; i < 16; ++i)
      out << "40\n" << i << "\n";
    out << "310\n"
        << std::string(2 * (DRW_LoftedSurface::kMaxReferenceDataSize + 1), 'A')
        << "\n0\nENDSEC\n0\nEOF\n";
  }

  LoftedSurfaceCapture capture;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, /*ext=*/true));
  CHECK(capture.m_callCount == 0);
  std::filesystem::remove(path);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF lofted surface writer round-trips subtype fields",
          "[dxf][surface][lofted][subtypes][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_lofted_surface_writer.dxf";
  std::filesystem::remove(path);

  DRW_LoftedSurface source;
  source.modelerFormatVersion = 2;
  source.uIsolines = 3;
  source.vIsolines = 4;
  source.rawAcisData = {0x41, 0x43, 0x49, 0x53};
  for (std::size_t i = 0; i < source.loftEntityTransform.size(); ++i)
    source.loftEntityTransform[i] = static_cast<double>(i + 1);
  source.planeNormalLoftingType = 2;
  source.startDraftAngle = 0.125;
  source.endDraftAngle = 0.25;
  source.startDraftMagnitude = 1.5;
  source.endDraftMagnitude = 2.5;
  source.arcLengthParameterization = true;
  source.noTwist = false;
  source.alignDirection = true;
  source.simpleSurfaces = false;
  source.closedSurfaces = true;
  source.solid = false;
  source.ruledSurface = true;
  source.virtualGuide = false;
  source.dxfReferenceData.emplace_back(90, 77);
  source.dxfReferenceData.emplace_back(90, 544);
  source.dxfReferenceData.emplace_back(310,
                                        std::vector<std::uint8_t>{0xAA, 0xBB});
  source.dxfReferenceData.emplace_back(90, 77);
  source.dxfReferenceData.emplace_back(90, 608);
  source.dxfReferenceData.emplace_back(310,
                                        std::vector<std::uint8_t>{0xCC, 0xDD});
  source.pathCurveHandle = 0xABCu;

  SurfaceEmitter emitter;
  emitter.m_surface = &source;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  LoftedSurfaceCapture capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, true));
  }
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isLofted);
  CHECK(capture.m_captured.loftEntityTransform
        == source.loftEntityTransform);
  CHECK(capture.m_captured.planeNormalLoftingType
        == source.planeNormalLoftingType);
  CHECK(capture.m_captured.startDraftAngle == source.startDraftAngle);
  CHECK(capture.m_captured.endDraftAngle == source.endDraftAngle);
  CHECK(capture.m_captured.startDraftMagnitude == source.startDraftMagnitude);
  CHECK(capture.m_captured.endDraftMagnitude == source.endDraftMagnitude);
  CHECK(capture.m_captured.arcLengthParameterization
        == source.arcLengthParameterization);
  CHECK(capture.m_captured.noTwist == source.noTwist);
  CHECK(capture.m_captured.alignDirection == source.alignDirection);
  CHECK(capture.m_captured.simpleSurfaces == source.simpleSurfaces);
  CHECK(capture.m_captured.closedSurfaces == source.closedSurfaces);
  CHECK(capture.m_captured.solid == source.solid);
  CHECK(capture.m_captured.ruledSurface == source.ruledSurface);
  CHECK(capture.m_captured.virtualGuide == source.virtualGuide);
  CHECK(capture.m_captured.dxfReferenceData.size()
        == source.dxfReferenceData.size());
  CHECK((capture.m_captured.dxfReferenceData[2].binary()
         && *capture.m_captured.dxfReferenceData[2].binary()
                == *source.dxfReferenceData[2].binary()));
  CHECK((capture.m_captured.dxfReferenceData[5].binary()
         && *capture.m_captured.dxfReferenceData[5].binary()
                == *source.dxfReferenceData[5].binary()));
  CHECK(capture.m_captured.pathCurveHandle == source.pathCurveHandle);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF NURBS surface retains AC1027 subtype fields",
          "[dxf][surface][nurbs][subtypes]") {
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nNURBSSURFACE\n5\n706\n330\n1F\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbModelerGeometry\n70\n2\n"
      "100\nAcDbSurface\n71\n3\n72\n4\n"
      "100\nAcDbNurbSurface\n170\n7\n290\n1\n"
      "10\n1.0\n20\n2.0\n30\n3.0\n"
      "11\n4.0\n21\n5.0\n31\n6.0\n"
      "12\n7.0\n22\n8.0\n32\n9.0\n"
      "13\n10.0\n23\n11.0\n33\n12.0\n"
      "0\nENDSEC\n0\nEOF\n";

  NurbsSurfaceCapture capture;
  readDxf(dxf, capture, "lc_nurbs_surface_fields.dxf");
  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isNurbs);
  CHECK(capture.m_captured.short170 == 7);
  CHECK(capture.m_captured.cvHullDisplay);
  CHECK(capture.m_captured.uvec1.x == 1.0);
  CHECK(capture.m_captured.uvec1.y == 2.0);
  CHECK(capture.m_captured.uvec1.z == 3.0);
  CHECK(capture.m_captured.vvec1.x == 4.0);
  CHECK(capture.m_captured.vvec1.y == 5.0);
  CHECK(capture.m_captured.vvec1.z == 6.0);
  CHECK(capture.m_captured.uvec2.x == 7.0);
  CHECK(capture.m_captured.uvec2.y == 8.0);
  CHECK(capture.m_captured.uvec2.z == 9.0);
  CHECK(capture.m_captured.vvec2.x == 10.0);
  CHECK(capture.m_captured.vvec2.y == 11.0);
  CHECK(capture.m_captured.vvec2.z == 12.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF NURBS surface writer round-trips AC1027 fields",
          "[dxf][surface][nurbs][subtypes][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_nurbs_surface_writer.dxf";
  std::filesystem::remove(path);

  DRW_NurbsSurface source;
  source.modelerFormatVersion = 2;
  source.uIsolines = 3;
  source.vIsolines = 4;
  source.short170 = 7;
  source.cvHullDisplay = true;
  source.uvec1 = DRW_Coord(1.0, 2.0, 3.0);
  source.vvec1 = DRW_Coord(4.0, 5.0, 6.0);
  source.uvec2 = DRW_Coord(7.0, 8.0, 9.0);
  source.vvec2 = DRW_Coord(10.0, 11.0, 12.0);

  SurfaceEmitter emitter;
  emitter.m_surface = &source;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1027, false));
  }

  NurbsSurfaceCapture capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, true));
  }
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_isNurbs);
  CHECK(capture.m_captured.short170 == source.short170);
  CHECK(capture.m_captured.cvHullDisplay == source.cvHullDisplay);
  CHECK(capture.m_captured.uvec1.x == source.uvec1.x);
  CHECK(capture.m_captured.uvec1.y == source.uvec1.y);
  CHECK(capture.m_captured.uvec1.z == source.uvec1.z);
  CHECK(capture.m_captured.vvec1.x == source.vvec1.x);
  CHECK(capture.m_captured.vvec1.y == source.vvec1.y);
  CHECK(capture.m_captured.vvec1.z == source.vvec1.z);
  CHECK(capture.m_captured.uvec2.x == source.uvec2.x);
  CHECK(capture.m_captured.uvec2.y == source.uvec2.y);
  CHECK(capture.m_captured.uvec2.z == source.uvec2.z);
  CHECK(capture.m_captured.vvec2.x == source.vvec2.x);
  CHECK(capture.m_captured.vvec2.y == source.vvec2.y);
  CHECK(capture.m_captured.vvec2.z == source.vvec2.z);
}

TEST_CASE("DXF NURBS surface rejects partial AC1027 coordinates",
          "[dxf][surface][nurbs][malformed]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_nurbs_surface_partial.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nNURBSSURFACE\n8\n0\n"
           "100\nAcDbEntity\n100\nAcDbModelerGeometry\n"
           "70\n2\n100\nAcDbSurface\n71\n3\n72\n4\n"
           "100\nAcDbNurbSurface\n170\n7\n"
           "10\n1.0\n20\n2.0\n30\n3.0\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  NurbsSurfaceCapture capture;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, /*ext=*/true));
  CHECK(capture.m_callCount == 0);
  std::filesystem::remove(path);
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

TEST_CASE("DXF modeler geometry rejects out-of-range version fields",
          "[dxf][modeler][numeric]") {
  const auto readMalformed = [](const char *value, const char *name) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
             "0\n3DSOLID\n8\n0\n100\nAcDbEntity\n"
             "100\nAcDbModelerGeometry\n100\nAcDb3dSolid\n70\n"
          << value
          << "\n0\nENDSEC\n0\nEOF\n";
    }

    ModelerGeometryCapture cap;
    dxfRW reader(path.string().c_str());
    bool ok = true;
    REQUIRE_NOTHROW(ok = reader.read(&cap, /*ext=*/true));
    CHECK_FALSE(ok);
    CHECK(cap.m_items.empty());
    std::filesystem::remove(path);
  };

  readMalformed("65536", "lc_modeler_version_overflow.dxf");
  readMalformed("-1", "lc_modeler_version_negative.dxf");
}

TEST_CASE("DXF MTEXT rejects out-of-range line spacing fields",
          "[dxf][mtext][numeric]") {
  const auto readMalformed = [](const char *value, const char *name) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
             "0\nMTEXT\n8\n0\n100\nAcDbEntity\n"
             "100\nAcDbMText\n10\n0\n20\n0\n40\n1\n1\ntext\n73\n"
          << value
          << "\n0\nENDSEC\n0\nEOF\n";
    }

    MTextCapture cap;
    dxfRW reader(path.string().c_str());
    bool ok = true;
    REQUIRE_NOTHROW(ok = reader.read(&cap, /*ext=*/true));
    CHECK_FALSE(ok);
    CHECK(cap.m_callCount == 0);
    std::filesystem::remove(path);
  };

  readMalformed("65536", "lc_mtext_linespacing_overflow.dxf");
  readMalformed("-1", "lc_mtext_linespacing_negative.dxf");
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

TEST_CASE("DXF INSERT rejects an ATTRIB sequence without SEQEND",
          "[dxf][attrib][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_attrib_missing_seqend.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nINSERT\n8\n0\n2\nTITLEBLK\n10\n0\n20\n0\n66\n1\n"
           "0\nATTRIB\n8\n0\n10\n1\n20\n2\n40\n0.5\n1\nVALUE\n2\nTAG\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  InsertCapture cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF POLYLINE rejects a VERTEX sequence without SEQEND",
          "[dxf][polyline][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_polyline_missing_seqend.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nPOLYLINE\n8\n0\n70\n0\n"
           "0\nVERTEX\n8\n0\n10\n1\n20\n2\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  PolylineCapture cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF POLYLINE rejects a VERTEX without insertion coordinates",
          "[dxf][polyline][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_polyline_invalid_vertex.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nPOLYLINE\n8\n0\n70\n0\n"
           "0\nVERTEX\n8\n0\n20\n2\n"
           "0\nSEQEND\n8\n0\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  PolylineCapture cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
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
TEST_CASE("DXF 3DLINE preserves its distinct type and raw-double fields",
          "[dxf][3dline][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_3dline_roundtrip.dxf";
  std::filesystem::remove(path);

  ThreeDLineEmitter emitter;
  emitter.m_line.layer = "0";
  emitter.m_line.basePoint = DRW_Coord(1.0, 2.0, 3.0);
  emitter.m_line.secPoint = DRW_Coord(4.0, 5.0, 6.0);
  emitter.m_line.extPoint = DRW_Coord(0.0, -1.0, 0.0);
  emitter.m_line.thickness = 0.75;
  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1015, false));
  }

  ThreeDLineCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.eType == DRW::THREEDLINE);
  CHECK(cap.m_captured.basePoint.x == 1.0);
  CHECK(cap.m_captured.basePoint.y == 2.0);
  CHECK(cap.m_captured.basePoint.z == 3.0);
  CHECK(cap.m_captured.secPoint.x == 4.0);
  CHECK(cap.m_captured.secPoint.y == 5.0);
  CHECK(cap.m_captured.secPoint.z == 6.0);
  CHECK(cap.m_captured.extPoint.x == 0.0);
  CHECK(cap.m_captured.extPoint.y == -1.0);
  CHECK(cap.m_captured.extPoint.z == 0.0);
  CHECK(cap.m_captured.thickness == 0.75);

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

    REQUIRE(cap.m_entities.empty());
    REQUIRE(cap.m_attdefs.size() == 1);
    const DRW_Attdef &attdef = cap.m_attdefs.front();
    CHECK(attdef.text == "Default");
    CHECK(attdef.tag == "PARTNO");
    CHECK(attdef.prompt == "Part number?");
    CHECK(attdef.attribFlags == 3);
    CHECK(attdef.m_fieldLength == 12);
    CHECK(attdef.alignV == DRW_Text::VMiddle);
    CHECK(attdef.lockPosition == true);
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
  // Write a valid polygonal WIPEOUT. Reading back should retain type 2.
  const auto path =
      std::filesystem::temp_directory_path() / "lc_wipeout_71_rt.dxf";
  std::filesystem::remove(path);

  WipeoutEmitter em;
  em.m_wipeout.basePoint = DRW_Coord(5.0, 5.0, 0.0);
  em.m_wipeout.secPoint  = DRW_Coord(0.01, 0.0, 0.0);
  em.m_wipeout.vVector   = DRW_Coord(0.0, 0.01, 0.0);
  em.m_wipeout.sizeu = 100;
  em.m_wipeout.sizev = 100;
  em.m_wipeout.m_clipBoundaryType = 2;
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

  // Rectangle records retain their compact two-corner representation. The
  // display flags and image/reactor handles must survive the same round trip.
  const auto rectPath =
      std::filesystem::temp_directory_path() / "lc_wipeout_rectangle_rt.dxf";
  std::filesystem::remove(rectPath);
  WipeoutEmitter rectEmitter;
  rectEmitter.m_wipeout.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  rectEmitter.m_wipeout.secPoint = DRW_Coord(1.0, 0.0, 0.0);
  rectEmitter.m_wipeout.vVector = DRW_Coord(0.0, 1.0, 0.0);
  rectEmitter.m_wipeout.sizeu = 3.0;
  rectEmitter.m_wipeout.sizev = 4.0;
  rectEmitter.m_wipeout.m_displayProps = 7;
  rectEmitter.m_wipeout.ref = 0x123u;
  rectEmitter.m_wipeout.m_imageDefReactorHandle = 0x456u;
  rectEmitter.m_wipeout.m_clipBoundaryType = 1;
  rectEmitter.m_wipeout.clipPath = {{-0.5, -0.5, 0.0}, {2.5, 3.5, 0.0}};
  {
    dxfRW w(rectPath.string().c_str());
    rectEmitter.m_rw = &w;
    REQUIRE(w.write(&rectEmitter, DRW::AC1024, false));
  }
  ImageCapture rectCapture;
  {
    dxfRW r(rectPath.string().c_str());
    REQUIRE(r.read(&rectCapture, /*ext=*/true));
  }
  std::filesystem::remove(rectPath);
  REQUIRE(rectCapture.m_wipeoutCount == 1);
  CHECK(rectCapture.m_lastWipeout.m_clipBoundaryType == 1);
  CHECK(rectCapture.m_lastWipeout.clipPath.size() == 2);
  CHECK(rectCapture.m_lastWipeout.m_displayProps == 7);
  CHECK(rectCapture.m_lastWipeout.ref == 0x123u);
  CHECK(rectCapture.m_lastWipeout.m_imageDefReactorHandle == 0x456u);
}

TEST_CASE("DXF WIPEOUT rejects a malformed clipping boundary", "[dxf][wipeout]") {
  const std::string malformed =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWIPEOUT\n5\nB1\n330\n0\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbRasterImage\n"
      "10\n0\n20\n0\n30\n0\n"
      "11\n1\n21\n0\n31\n0\n"
      "12\n0\n22\n1\n32\n0\n"
      "13\n2\n23\n2\n"
      "70\n1\n280\n1\n281\n50\n282\n50\n283\n0\n"
      "100\nAcDbWipeout\n90\n0\n71\n2\n"
      // Two vertices are declared but only one complete 14/24 pair follows.
      "91\n2\n14\n0\n24\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  const auto path = std::filesystem::temp_directory_path() / "lc_wipeout_malformed.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << malformed;
  }

  ImageCapture capture;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, /*ext=*/true));
  CHECK(capture.m_wipeoutCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF IMAGE rejects out-of-range scalar fields",
          "[dxf][image][numeric][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nIMAGE\n5\nA1\n330\n0\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbRasterImage\n"
      "90\n0\n10\n0\n20\n0\n30\n0\n"
      "11\n1\n21\n0\n31\n0\n12\n0\n22\n1\n32\n0\n"
      "13\n10\n23\n10\n340\n0\n";
  const std::string suffix = "360\n0\n0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> fields = {
      "70\n65536\n280\n1\n281\n50\n282\n50\n283\n0\n71\n0\n",
      "70\n1\n280\n2\n281\n50\n282\n50\n283\n0\n71\n0\n",
      "70\n1\n280\n1\n281\n101\n282\n50\n283\n0\n71\n0\n",
      "70\n1\n280\n1\n281\n50\n282\n-1\n283\n0\n71\n0\n",
      "70\n1\n280\n1\n281\n50\n282\n50\n283\n101\n71\n0\n",
      "70\n1\n280\n1\n281\n50\n282\n50\n283\n0\n71\n3\n"};

  for (std::size_t i = 0; i < fields.size(); ++i) {
    ImageCapture capture;
    CHECK_FALSE(tryReadDxf(prefix + fields[i] + suffix, capture,
                           ("lc_image_bad_scalar_" + std::to_string(i)
                            + ".dxf").c_str()));
    CHECK(capture.m_imageCount == 0);
  }
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
  CHECK(cap1.m_captured.seedPoints.empty());
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

TEST_CASE("DXF HATCH rejects incomplete declared streams",
          "[dxf][hatch][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nHATCH\n5\nA1\n330\n0\n"
      "100\nAcDbEntity\n8\n0\n"
      "100\nAcDbHatch\n"
      "10\n0.0\n20\n0.0\n30\n0.0\n"
      "210\n0.0\n220\n0.0\n230\n1.0\n"
      "2\nSOLID\n70\n1\n71\n0\n";
  const std::string suffix =
      "75\n0\n76\n1\n77\n0\n78\n0\n98\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> malformed = {
      // Declares one loop but never opens a boundary path.
      "91\n1\n" + suffix,
      // Declares one polyline vertex but supplies two.
      "91\n1\n92\n2\n73\n1\n93\n1\n"
      "10\n0.0\n20\n0.0\n10\n1.0\n20\n0.0\n" + suffix,
      // Declares two spline control points but supplies only one.
      "91\n1\n92\n0\n93\n1\n72\n4\n94\n2\n73\n0\n74\n0\n"
      "95\n1\n96\n2\n40\n0.0\n10\n0.0\n20\n0.0\n97\n0\n" + suffix,
      // Declares one dash length but supplies two.
      "91\n0\n75\n0\n76\n1\n78\n1\n53\n0.0\n"
      "43\n0.0\n44\n0.0\n45\n1.0\n46\n0.0\n79\n1\n"
      "49\n1.0\n49\n2.0\n" + suffix,
      // Declares two gradient stops but supplies only one.
      "91\n0\n75\n0\n76\n1\n78\n0\n98\n0\n"
      "450\n1\n453\n2\n463\n0.0\n421\n1\n"
      "0\nENDSEC\n0\nEOF\n",
      // Declares one boundary source handle but supplies none.
      "91\n1\n92\n0\n93\n1\n72\n1\n"
      "10\n0.0\n20\n0.0\n11\n1.0\n21\n0.0\n97\n1\n" + suffix,
  };

  for (std::size_t i = 0; i < malformed.size(); ++i) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_hatch_malformed_counts_" + std::to_string(i) + ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << prefix + malformed[i];
    }

    HatchCapture cap;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&cap, /*ext=*/true));
    CHECK(cap.m_callCount == 0);
    std::filesystem::remove(path);
  }
}
