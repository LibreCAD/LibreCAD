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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "drw_header.h"
#include "drw_entities.h"
#include "drw_objects.h"
#include "intern/dxfwriter.h"
#include "libdxfrw.h"

namespace {

std::string slurp(const std::filesystem::path &path);

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

class LegacyUnsupportedEmitter : public StubInterface {
public:
  enum class Kind {
    LwPolyline,
    Spline,
    Helix,
    Hatch,
    MPolygon,
    Leader,
    Dimension,
    RText,
    ArcAlignedText,
    MLine,
    Underlay,
    MText,
    Light,
    Mesh
  };

  dxfRW *m_rw = nullptr;
  Kind m_kind = Kind::LwPolyline;
  bool m_result = true;

  void writeEntities() override {
    switch (m_kind) {
    case Kind::LwPolyline: {
      DRW_LWPolyline entity;
      m_result = m_rw->writeLWPolyline(&entity);
      break;
    }
    case Kind::Spline: {
      DRW_Spline entity;
      m_result = m_rw->writeSpline(&entity);
      break;
    }
    case Kind::Helix: {
      DRW_Helix entity;
      m_result = m_rw->writeHelix(&entity);
      break;
    }
    case Kind::Hatch: {
      DRW_Hatch entity;
      m_result = m_rw->writeHatch(&entity);
      break;
    }
    case Kind::MPolygon: {
      DRW_MPolygon entity;
      m_result = m_rw->writeMPolygon(&entity);
      break;
    }
    case Kind::Leader: {
      DRW_Leader entity;
      m_result = m_rw->writeLeader(&entity);
      break;
    }
    case Kind::Dimension: {
      DRW_DimLinear entity;
      m_result = m_rw->writeDimension(&entity);
      break;
    }
    case Kind::RText: {
      DRW_RText entity;
      m_result = m_rw->writeRText(&entity);
      break;
    }
    case Kind::ArcAlignedText: {
      DRW_ArcAlignedText entity;
      m_result = m_rw->writeArcAlignedText(&entity);
      break;
    }
    case Kind::MLine: {
      DRW_MLine entity;
      m_result = m_rw->writeMLine(&entity);
      break;
    }
    case Kind::Underlay: {
      DRW_Underlay entity;
      m_result = m_rw->writeUnderlay(&entity);
      break;
    }
    case Kind::MText: {
      DRW_MText entity;
      m_result = m_rw->writeMText(&entity);
      break;
    }
    case Kind::Light: {
      DRW_Light entity;
      m_result = m_rw->writeLight(&entity);
      break;
    }
    case Kind::Mesh: {
      DRW_Mesh entity;
      m_result = m_rw->writeMesh(&entity);
      break;
    }
    }
  }
};

class TableSourceStateEmitter final : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_LType m_ltype;
  DRW_Vport m_vport;
  bool m_ltypeResult = false;
  bool m_vportResult = false;

  void writeLTypes() override {
    m_ltypeResult = m_writer->writeLineType(&m_ltype);
  }

  void writeVports() override {
    m_vportResult = m_writer->writeVport(&m_vport);
  }
};

TEST_CASE("DXF table writers do not normalize caller-owned state",
          "[dxf][writer][source-state]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_table_writer_source_state.dxf";
  std::error_code ignored;
  std::filesystem::remove(path, ignored);

  TableSourceStateEmitter emitter;
  emitter.m_ltype.name = "SOURCE_LTYPE";
  emitter.m_ltype.path = {1.0, -0.5};
  emitter.m_ltype.size = 77;
  emitter.m_ltype.length = 123.0;
  emitter.m_vport.name = "SOURCE_VPORT";
  dxfRW writer(path.string().c_str());
  emitter.m_writer = &writer;

  REQUIRE(writer.write(&emitter, DRW::AC1015, false));
  CHECK(emitter.m_ltypeResult);
  CHECK(emitter.m_vportResult);
  CHECK(emitter.m_ltype.segments.empty());
  CHECK(emitter.m_ltype.size == 77);
  CHECK(emitter.m_ltype.length == 123.0);
  CHECK(emitter.m_vport.name == "SOURCE_VPORT");

  std::filesystem::remove(path, ignored);
}

TEST_CASE("DXF legacy writer rejects unsupported entity emitters",
          "[dxf][writer][unsupported][safety]") {
  const std::array<LegacyUnsupportedEmitter::Kind, 14> kinds = {
      LegacyUnsupportedEmitter::Kind::LwPolyline,
      LegacyUnsupportedEmitter::Kind::Spline,
      LegacyUnsupportedEmitter::Kind::Helix,
      LegacyUnsupportedEmitter::Kind::Hatch,
      LegacyUnsupportedEmitter::Kind::MPolygon,
      LegacyUnsupportedEmitter::Kind::Leader,
      LegacyUnsupportedEmitter::Kind::Dimension,
      LegacyUnsupportedEmitter::Kind::RText,
      LegacyUnsupportedEmitter::Kind::ArcAlignedText,
      LegacyUnsupportedEmitter::Kind::MLine,
      LegacyUnsupportedEmitter::Kind::Underlay,
      LegacyUnsupportedEmitter::Kind::MText,
      LegacyUnsupportedEmitter::Kind::Light,
      LegacyUnsupportedEmitter::Kind::Mesh};

  int index = 0;
  for (const auto kind : kinds) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_dxf_legacy_unsupported_" +
                       std::to_string(index++) + ".dxf");
    {
      std::ofstream previous(path);
      previous << "previous output\n";
    }

    LegacyUnsupportedEmitter emitter;
    emitter.m_kind = kind;
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1009, false));
    CHECK_FALSE(emitter.m_result);
    CHECK(slurp(path) == "previous output\n");
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
  }
}

class BlockCapture : public StubInterface {
public:
  std::vector<DRW_Block> m_blocks;
  std::vector<DRW_Insert> m_inserts;
  std::vector<std::string> m_events;
  int m_lineCount = 0;
  int m_polylineCount = 0;
  int m_endBlockCount = 0;

  void addBlock(const DRW_Block &block) override {
    m_blocks.push_back(block);
    m_events.push_back("block");
  }

  void addInsert(const DRW_Insert &insert) override {
    m_inserts.push_back(insert);
  }

  void addLine(const DRW_Line &) override {
    ++m_lineCount;
    m_events.push_back("line");
  }

  void addPolyline(const DRW_Polyline &) override {
    ++m_polylineCount;
    m_events.push_back("polyline");
  }

  void endBlock() override {
    ++m_endBlockCount;
    m_events.push_back("end");
  }
};

class LayerCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Layer m_captured;
  std::vector<DRW_Layer> m_layers;

  void addLayer(const DRW_Layer &layer) override {
    if (m_callCount == 0)
      m_captured = layer;
    m_layers.push_back(layer);
    ++m_callCount;
  }
};

class LayerEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_Layer m_layer;
  bool m_writeResult = true;

  void writeLayers() override {
    m_writeResult = m_writer->writeLayer(&m_layer);
  }
};

class LTypeCapture : public StubInterface {
public:
  std::vector<DRW_LType> m_ltypes;

  void addLType(const DRW_LType &ltype) override {
    m_ltypes.push_back(ltype);
  }
};

class LTypeEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_LType m_ltype;
  bool m_writeResult = true;

  void writeLTypes() override {
    m_writeResult = m_writer->writeLineType(&m_ltype);
  }
};

class RejectingStreambuf final : public std::streambuf {
protected:
  std::streamsize xsputn(const char *, std::streamsize) override { return 0; }
  int_type overflow(int_type) override { return traits_type::eof(); }
};

class PartialStreambuf final : public std::streambuf {
public:
  explicit PartialStreambuf(std::size_t limit) : m_remaining(limit) {}

  const std::string &data() const { return m_data; }

protected:
  std::streamsize xsputn(const char *data, std::streamsize size) override {
    const std::streamsize accepted = static_cast<std::streamsize>(
        std::min<std::size_t>(m_remaining, static_cast<std::size_t>(size)));
    m_data.append(data, static_cast<std::size_t>(accepted));
    m_remaining -= static_cast<std::size_t>(accepted);
    return accepted;
  }

  int_type overflow(int_type value) override {
    if (m_remaining == 0 || traits_type::eq_int_type(value, traits_type::eof()))
      return traits_type::eof();
    m_data.push_back(traits_type::to_char_type(value));
    --m_remaining;
    return value;
  }

private:
  std::size_t m_remaining;
  std::string m_data;
};

TEST_CASE("dxfRW converts handle exhaustion into a failed write",
          "[dxf-write][handles][safety]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_reserve_handle_overflow.dxf";
  dxfRW writer(path.string().c_str());
  StubInterface iface;
  CHECK_FALSE(writer.reserveHandle(std::numeric_limits<std::uint32_t>::max()));

  CHECK_FALSE(writer.write(&iface, DRW::AC1015, /*bin=*/false));
  CHECK(writer.getError() == DRW::BAD_OPEN);
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
}

TEST_CASE("DXF writer record scope commits and aborts bytes",
          "[dxf][writer][scope]") {
  std::ostringstream asciiOutput;
  dxfWriterAscii asciiWriter(&asciiOutput);
  {
    DxfWriterRecordScope record(asciiWriter);
    REQUIRE(asciiWriter.writeString(0, "POINT"));
    record.abort();
  }
  CHECK(asciiOutput.str().empty());

  {
    DxfWriterRecordScope record(asciiWriter);
    REQUIRE(asciiWriter.writeString(0, "POINT"));
    REQUIRE(record.commit());
  }
  CHECK(asciiOutput.str() == "  0\nPOINT\n");

  std::ostringstream binaryOutput;
  dxfWriterBinary binaryWriter(&binaryOutput);
  {
    DxfWriterRecordScope outer(binaryWriter);
    REQUIRE(binaryWriter.writeString(0, "POINT"));
    {
      DxfWriterRecordScope inner(binaryWriter);
      REQUIRE(binaryWriter.writeString(8, "0"));
      REQUIRE(inner.commit());
    }
    outer.abort();
  }
  CHECK(binaryOutput.str().empty());

  RejectingStreambuf rejectingBuffer;
  std::ostream rejectingOutput(&rejectingBuffer);
  dxfWriterAscii rejectingWriter(&rejectingOutput);
  {
    DxfWriterRecordScope record(rejectingWriter);
    REQUIRE(rejectingWriter.writeString(0, "POINT"));
    CHECK_FALSE(record.commit());
  }
  CHECK(rejectingOutput.fail());
  CHECK(rejectingWriter.hasWriteError());

  PartialStreambuf partialBuffer(3);
  std::ostream partialOutput(&partialBuffer);
  dxfWriterAscii partialWriter(&partialOutput);
  {
    DxfWriterRecordScope record(partialWriter);
    REQUIRE(partialWriter.writeString(0, "POINT"));
    CHECK_FALSE(record.commit());
  }
  CHECK(partialOutput.fail());
  CHECK(partialWriter.hasWriteError());
  // A generic parent stream cannot retract the bytes accepted before the
  // short write; only the outer staged file/phase can discard them.
  CHECK(partialBuffer.data().size() == 3);
}

class ExhaustingEntityEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  bool m_writeResult = true;

  void writeEntities() override {
    m_writeResult = m_writer->reserveHandle(
        std::numeric_limits<std::uint32_t>::max() - 1u);
    DRW_Point point;
    point.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    m_writeResult = m_writer->writePoint(&point) && m_writeResult;
  }
};

class ExhaustingObjectsEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  bool m_writeResult = true;

  void writeEntities() override {
    m_writeResult = m_writer->reserveHandle(
        std::numeric_limits<std::uint32_t>::max() - 1u);
  }
};

TEST_CASE("DXF record emitters fail cleanly on allocator exhaustion",
          "[dxf][writer][handles][safety]") {
  const auto entityPath = std::filesystem::temp_directory_path()
                          / "lc_dxf_entity_handle_exhaustion.dxf";
  const auto objectPath = std::filesystem::temp_directory_path()
                          / "lc_dxf_object_handle_exhaustion.dxf";
  std::error_code ignored;
  std::filesystem::remove(entityPath, ignored);
  std::filesystem::remove(objectPath, ignored);

  dxfRW entityWriter(entityPath.string().c_str());
  ExhaustingEntityEmitter entityEmitter;
  entityEmitter.m_writer = &entityWriter;
  REQUIRE_FALSE(entityWriter.write(&entityEmitter, DRW::AC1015, false));
  CHECK_FALSE(entityEmitter.m_writeResult);
  CHECK_FALSE(std::filesystem::exists(entityPath));

  dxfRW objectWriter(objectPath.string().c_str());
  ExhaustingObjectsEmitter objectEmitter;
  objectEmitter.m_writer = &objectWriter;
  DRW_Group group;
  group.name = "exhaustion";
  objectWriter.setGroups({group});
  REQUIRE_FALSE(objectWriter.write(&objectEmitter, DRW::AC1015, false));
  CHECK(objectEmitter.m_writeResult);
  CHECK_FALSE(std::filesystem::exists(objectPath));

  std::filesystem::remove(entityPath, ignored);
  std::filesystem::remove(objectPath, ignored);
}

class ReusablePointEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_invalid = true;
  bool m_writeResult = true;

  void writeEntities() override {
    DRW_Point point;
    point.handle = 0x100;
    point.basePoint = m_invalid
        ? DRW_Coord(std::numeric_limits<double>::quiet_NaN(), 2.0, 0.0)
        : DRW_Coord(1.0, 2.0, 0.0);
    m_writeResult = m_rw->writePoint(&point);
  }
};

TEST_CASE("dxfRW can retry a failed write with a reset handle stream",
          "[dxf-write][handles][reset][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_dxf_retry_after_failed_write.dxf";
  std::error_code ignored;
  std::filesystem::remove(path, ignored);

  dxfRW writer(path.string().c_str());
  ReusablePointEmitter emitter;
  emitter.m_rw = &writer;
  REQUIRE(writer.reserveHandle(0x100));
  {
    std::ofstream previous(path);
    previous << "previous output\n";
  }
  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
  CHECK_FALSE(emitter.m_writeResult);
  CHECK(slurp(path) == "previous output\n");

  emitter.m_invalid = false;
  REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  CHECK(emitter.m_writeResult);
  CHECK(slurp(path).find("\nPOINT\n") != std::string::npos);
  std::filesystem::remove(path, ignored);
}

class ReusableLayerEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_invalid = true;
  bool m_writeResult = true;

  void writeLayers() override {
    DRW_Layer layer;
    layer.name = "RETRY_LAYER";
    layer.color = m_invalid ? 257 : 7;
    m_writeResult = m_rw->writeLayer(&layer);
  }
};

TEST_CASE("dxfRW can retry after a failed TABLES callback",
          "[dxf][writer][tables][retry]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_retry_after_failed_tables_write.dxf";
  std::error_code ignored;
  std::filesystem::remove(path, ignored);

  dxfRW writer(path.string().c_str());
  ReusableLayerEmitter emitter;
  emitter.m_rw = &writer;
  {
    std::ofstream previous(path);
    previous << "previous output\n";
  }

  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
  CHECK_FALSE(emitter.m_writeResult);
  CHECK(slurp(path) == "previous output\n");

  emitter.m_invalid = false;
  CHECK(writer.write(&emitter, DRW::AC1021, false));
  CHECK(emitter.m_writeResult);

  const std::string output = slurp(path);
  CHECK(output.find("RETRY_LAYER") != std::string::npos);
  CHECK(output.find("RETRY_LAYER") == output.rfind("RETRY_LAYER"));
  std::filesystem::remove(path, ignored);
}

TEST_CASE("dxfRW rejects unsupported output versions before opening",
          "[dxf-write][version][safety]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_unsupported_output_version.dxf";
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
  StubInterface iface;

  const std::array<DRW::Version, 3> unsupported = {
      DRW::UNKNOWNV,
      DRW::AC1006,
      DRW::AC1012,
  };
  for (const DRW::Version version : unsupported) {
    dxfRW writer(path.string().c_str());
    CHECK_FALSE(writer.write(&iface, version, /*bin=*/false));
    CHECK(writer.getError() == DRW::BAD_VERSION);
    CHECK_FALSE(std::filesystem::exists(path));
  }
}

class GeometryMutationEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  DRW_Ellipse m_ellipse;
  DRW_LWPolyline m_lwPolyline;
  DRW_Hatch m_hatch;
  DRW_MPolygon m_mpolygon;
  DRW_DimLinear m_dimension;
  bool m_ellipseResult = false;
  bool m_lwPolylineResult = false;
  bool m_hatchResult = false;
  bool m_mpolygonResult = false;
  bool m_dimensionResult = false;

  void writeEntities() override {
    m_ellipseResult = m_rw->writeEllipse(&m_ellipse);
    m_lwPolylineResult = m_rw->writeLWPolyline(&m_lwPolyline);
    m_hatchResult = m_rw->writeHatch(&m_hatch);
    m_mpolygonResult = m_rw->writeMPolygon(&m_mpolygon);
    m_dimensionResult = m_rw->writeDimension(&m_dimension);
  }
};

TEST_CASE("DXF geometry writers do not mutate source normalization fields",
          "[dxf][writer][source-state]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_writer_source_state.dxf";
  std::error_code ignored;
  std::filesystem::remove(path, ignored);

  GeometryMutationEmitter emitter;
  emitter.m_ellipse.basePoint = DRW_Coord(1.0, 2.0, 0.0);
  emitter.m_ellipse.secPoint = DRW_Coord(2.0, 0.0, 0.0);
  emitter.m_ellipse.ratio = 2.0;
  emitter.m_ellipse.staparam = 0.25;
  emitter.m_ellipse.endparam = 1.0;
  emitter.m_lwPolyline.vertexnum = 37;
  auto lwVertex = emitter.m_lwPolyline.addVertex();
  lwVertex->x = 3.0;
  lwVertex->y = 4.0;
  emitter.m_lwPolyline.flags = 1;
  auto hatchLoop = std::make_shared<DRW_HatchLoop>(0);
  auto hatchEllipse = std::make_shared<DRW_Ellipse>();
  hatchEllipse->basePoint = DRW_Coord(5.0, 6.0, 0.0);
  hatchEllipse->secPoint = DRW_Coord(0.0, 2.0, 0.0);
  hatchEllipse->ratio = 2.0;
  hatchEllipse->staparam = 0.5;
  hatchEllipse->endparam = 1.5;
  hatchLoop->numedges = 41;
  hatchLoop->objlist.push_back(hatchEllipse);
  emitter.m_hatch.name = "SOLID";
  emitter.m_hatch.loopsnum = 43;
  emitter.m_hatch.looplist.push_back(hatchLoop);
  emitter.m_mpolygon.name = "SOLID";
  emitter.m_mpolygon.loopsnum = 47;
  const DRW_Coord ellipseAxis = emitter.m_ellipse.secPoint;
  const double ellipseRatio = emitter.m_ellipse.ratio;
  const double ellipseStart = emitter.m_ellipse.staparam;
  const double ellipseEnd = emitter.m_ellipse.endparam;
  const int lwVertexCount = emitter.m_lwPolyline.vertexnum;
  const double lwVertexX = lwVertex->x;
  const double lwVertexY = lwVertex->y;
  const int hatchLoopCount = emitter.m_hatch.loopsnum;
  const int hatchEdgeCount = hatchLoop->numedges;
  const DRW_Coord hatchEllipseAxis = hatchEllipse->secPoint;
  const double hatchEllipseRatio = hatchEllipse->ratio;
  const int mpolygonLoopCount = emitter.m_mpolygon.loopsnum;
  const int dimensionType = emitter.m_dimension.type;

  dxfRW writer(path.string().c_str());
  emitter.m_rw = &writer;
  REQUIRE(writer.write(&emitter, DRW::AC1015, false));
  CHECK(emitter.m_ellipseResult);
  CHECK(emitter.m_lwPolylineResult);
  CHECK(emitter.m_hatchResult);
  CHECK(emitter.m_mpolygonResult);
  CHECK(emitter.m_dimensionResult);
  CHECK(emitter.m_ellipse.secPoint.x == ellipseAxis.x);
  CHECK(emitter.m_ellipse.secPoint.y == ellipseAxis.y);
  CHECK(emitter.m_ellipse.secPoint.z == ellipseAxis.z);
  CHECK(emitter.m_ellipse.ratio == ellipseRatio);
  CHECK(emitter.m_ellipse.staparam == ellipseStart);
  CHECK(emitter.m_ellipse.endparam == ellipseEnd);
  CHECK(emitter.m_lwPolyline.vertexnum == lwVertexCount);
  REQUIRE(emitter.m_lwPolyline.vertlist.size() == 1);
  CHECK(emitter.m_lwPolyline.vertlist.front()->x == lwVertexX);
  CHECK(emitter.m_lwPolyline.vertlist.front()->y == lwVertexY);
  CHECK(emitter.m_hatch.loopsnum == hatchLoopCount);
  REQUIRE(emitter.m_hatch.looplist.size() == 1);
  CHECK(emitter.m_hatch.looplist.front()->numedges == hatchEdgeCount);
  REQUIRE(emitter.m_hatch.looplist.front()->objlist.size() == 1);
  const auto outputHatchEllipse = std::dynamic_pointer_cast<DRW_Ellipse>(
      emitter.m_hatch.looplist.front()->objlist.front());
  REQUIRE(outputHatchEllipse != nullptr);
  CHECK(outputHatchEllipse->secPoint.x == hatchEllipseAxis.x);
  CHECK(outputHatchEllipse->secPoint.y == hatchEllipseAxis.y);
  CHECK(outputHatchEllipse->secPoint.z == hatchEllipseAxis.z);
  CHECK(outputHatchEllipse->ratio == hatchEllipseRatio);
  CHECK(emitter.m_mpolygon.loopsnum == mpolygonLoopCount);
  CHECK(emitter.m_dimension.type == dimensionType);
  std::filesystem::remove(path, ignored);
}

class LineCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Line m_captured;

  void addLine(const DRW_Line &line) override {
    if (m_callCount == 0)
      m_captured = line;
    ++m_callCount;
  }
};

class LeaderCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Leader m_captured;

  void addLeader(const DRW_Leader *leader) override {
    if (m_callCount == 0 && leader != nullptr)
      m_captured = *leader;
    ++m_callCount;
  }
};

class LeaderEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  DRW_Leader m_leader;
  bool m_writeResult = true;

  void writeEntities() override {
    m_writeResult = m_rw->writeLeader(&m_leader);
  }
};

class ViewportCapture : public StubInterface {
public:
  std::vector<DRW_Viewport> m_viewports;

  void addViewport(const DRW_Viewport &viewport) override {
    m_viewports.push_back(viewport);
  }
};

class ViewportEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_Viewport m_viewport;
  bool m_writeResult = true;

  void writeEntities() override {
    m_writeResult = m_writer->writeViewport(&m_viewport);
  }
};

class ViewCapture : public StubInterface {
public:
  std::vector<DRW_View> m_views;

  void addView(const DRW_View &view) override { m_views.push_back(view); }
};

class ViewEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_View m_view;
  bool m_writeResult = true;

  void writeViews() override {
    m_writeResult = m_writer->writeView(&m_view);
  }
};

class UcsCapture : public StubInterface {
public:
  std::vector<DRW_UCS> m_ucss;

  void addUCS(const DRW_UCS &ucs) override { m_ucss.push_back(ucs); }
};

class UcsEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_UCS m_ucs;
  bool m_writeResult = true;

  void writeUCSs() override {
    m_writeResult = m_writer->writeUCS(&m_ucs);
  }
};

class AppIdCapture : public StubInterface {
public:
  std::vector<DRW_AppId> m_appIds;

  void addAppId(const DRW_AppId &appId) override {
    m_appIds.push_back(appId);
  }
};

class TextStyleCapture : public StubInterface {
public:
  std::vector<DRW_Textstyle> m_styles;

  void addTextStyle(const DRW_Textstyle &style) override {
    m_styles.push_back(style);
  }
};

class TextStyleEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_Textstyle m_style;
  bool m_writeResult = true;

  void writeTextstyles() override {
    m_writeResult = m_writer->writeTextstyle(&m_style);
  }
};

class AppIdEmitter : public StubInterface {
public:
  dxfRW *m_writer = nullptr;
  DRW_AppId m_appId;
  bool m_writeResult = true;

  void writeAppId() override {
    m_writeResult = m_writer->writeAppId(&m_appId);
  }
};

class InfiniteLineCapture : public StubInterface {
public:
  int m_rayCallCount = 0;
  int m_xlineCallCount = 0;

  void addRay(const DRW_Ray &) override { ++m_rayCallCount; }
  void addXline(const DRW_Xline &) override { ++m_xlineCallCount; }
};

class QuadEntityCapture : public StubInterface {
public:
  int m_traceCallCount = 0;
  int m_solidCallCount = 0;
  int m_faceCallCount = 0;
  DRW_Trace m_trace;

  void addTrace(const DRW_Trace &entity) override {
    m_trace = entity;
    ++m_traceCallCount;
  }

  void addSolid(const DRW_Solid &) override { ++m_solidCallCount; }
  void add3dFace(const DRW_3Dface &) override { ++m_faceCallCount; }
};

class TextEntityCapture : public StubInterface {
public:
  int m_textCallCount = 0;
  int m_mtextCallCount = 0;

  void addText(const DRW_Text &) override { ++m_textCallCount; }
  void addMText(const DRW_MText &) override { ++m_mtextCallCount; }
};

class VportCapture : public StubInterface {
public:
  int m_callCount = 0;
  std::vector<DRW_Vport> m_captured;

  void addVport(const DRW_Vport &vport) override {
    m_captured.push_back(vport);
    ++m_callCount;
  }
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

class PlotSettingsCapture : public StubInterface {
public:
  int m_callCount = 0;

  void addPlotSettings(const DRW_PlotSettings *) override { ++m_callCount; }
};

class InsertCapture : public StubInterface {
public:
  std::vector<DRW_Insert> m_captured;

  void addInsert(const DRW_Insert &d) override {
    m_captured.push_back(d);
  }
};

class TableCapture : public StubInterface {
public:
  int m_callCount = 0;

  void addTable(const DRW_Table &table) override {
    (void)table;
    ++m_callCount;
  }
};

class EllipseCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Ellipse m_captured;

  void addEllipse(const DRW_Ellipse &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class CircleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Circle m_captured;

  void addCircle(const DRW_Circle &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class ArcCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Arc m_captured;

  void addArc(const DRW_Arc &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class ShapeCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Shape m_captured;
  int m_lineCallCount = 0;
  DRW_Line m_line;

  void addShape(const DRW_Shape &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }

  void addLine(const DRW_Line &line) override {
    if (m_lineCallCount == 0)
      m_line = line;
    ++m_lineCallCount;
  }
};

class CameraCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Camera m_captured;

  void addCamera(const DRW_Camera &camera) override {
    if (m_callCount == 0)
      m_captured = camera;
    ++m_callCount;
  }
};

class GeoPositionMarkerCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_GeoPositionMarker m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addGeoPositionMarker(const DRW_GeoPositionMarker &marker) override {
    if (m_callCount == 0)
      m_captured = marker;
    ++m_callCount;
  }

  void addRawDxfEntity(const DRW_RawDxfObject &entity) override {
    m_raw.push_back(entity);
  }
};

class SectionObjectCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_SectionObject m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addSectionObject(const DRW_SectionObject &section) override {
    if (m_callCount == 0)
      m_captured = section;
    ++m_callCount;
  }

  void addRawDxfEntity(const DRW_RawDxfObject &entity) override {
    m_raw.push_back(entity);
  }
};

class LightListCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_LightList m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addLightList(const DRW_LightList &lightList) override {
    if (m_callCount == 0)
      m_captured = lightList;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class LayerFilterCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_LayerFilter m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addLayerFilter(const DRW_LayerFilter &layerFilter) override {
    if (m_callCount == 0)
      m_captured = layerFilter;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class DataLinkCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_DataLink m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addDataLink(const DRW_DataLink &dataLink) override {
    if (m_callCount == 0)
      m_captured = dataLink;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class GeoMapImageCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_GeoMapImage m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addGeoMapImage(const DRW_GeoMapImage &image) override {
    if (m_callCount == 0)
      m_captured = image;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class IndexCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Index m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addIndex(const DRW_Index &index) override {
    if (m_callCount == 0)
      m_captured = index;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class IDBufferCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_IDBuffer m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addIDBuffer(const DRW_IDBuffer &buffer) override {
    if (m_callCount == 0)
      m_captured = buffer;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class LayerIndexCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_LayerIndex m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addLayerIndex(const DRW_LayerIndex &index) override {
    if (m_callCount == 0)
      m_captured = index;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class SpatialIndexCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_SpatialIndex m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addSpatialIndex(const DRW_SpatialIndex &index) override {
    if (m_callCount == 0)
      m_captured = index;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class Ole2FrameCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Ole2Frame m_captured;

  void addOle2Frame(const DRW_Ole2Frame &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class PointCloudCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_PointCloud m_captured;

  void addPointCloud(const DRW_PointCloud *data) override {
    if (data != nullptr && m_callCount == 0)
      m_captured = *data;
    ++m_callCount;
  }
};

class PointCloudExCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_PointCloudEx m_captured;

  void addPointCloudEx(const DRW_PointCloudEx *data) override {
    if (data != nullptr && m_callCount == 0)
      m_captured = *data;
    ++m_callCount;
  }
};

class NavisworksModelCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_NavisworksModel m_captured;

  void addNavisworksModel(const DRW_NavisworksModel *data) override {
    if (data != nullptr && m_callCount == 0)
      m_captured = *data;
    ++m_callCount;
  }
};

class OleFrameCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_OleFrame m_captured;

  void addOleFrame(const DRW_OleFrame &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class LWPolylineCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_LWPolyline m_captured;

  void addLWPolyline(const DRW_LWPolyline &d) override {
    if (m_callCount == 0)
      m_captured = d;
    ++m_callCount;
  }
};

class SplineCapture : public StubInterface {
public:
  int m_callCount = 0;

  void addSpline(const DRW_Spline *) override { ++m_callCount; }
};

class PolylineCapture : public StubInterface {
public:
  int m_callCount = 0;

  void addPolyline(const DRW_Polyline &) override { ++m_callCount; }
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

class XRecordCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_XRecord m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addXRecord(const DRW_XRecord &data) override {
    if (m_callCount == 0)
      m_captured = data;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &data) override {
    m_raw.push_back(data);
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

class FieldCapture : public StubInterface {
public:
  std::vector<DRW_Field> m_fields;
  std::vector<DRW_FieldList> m_lists;
  std::vector<DRW_RawDxfObject> m_raw;

  void addField(const DRW_Field &field) override { m_fields.push_back(field); }
  void addFieldList(const DRW_FieldList &list) override {
    m_lists.push_back(list);
  }
  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    m_raw.push_back(object);
  }
};

class RawSectionCapture : public StubInterface {
public:
  std::vector<DRW_RawDxfSection> m_sections;

  void addRawDxfSection(const DRW_RawDxfSection &section) override {
    m_sections.push_back(section);
  }
};

class DynamicBlockCapture : public StubInterface {
public:
  int m_callCount = 0;
  std::vector<DRW_RawDxfObject> m_raw;
  DRW_DynamicBlockObject m_captured;

  void addDynamicBlockObject(const DRW_DynamicBlockObject &data) override {
    if (m_callCount == 0)
      m_captured = data;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &data) override {
    m_raw.push_back(data);
  }
};

class AcShHistoryCapture : public StubInterface {
public:
  int m_callCount = 0;
  std::vector<DRW_RawDxfObject> m_raw;
  DRW_AcShHistoryObject m_captured;

  void addAcShHistoryObject(const DRW_AcShHistoryObject &data) override {
    if (m_callCount == 0)
      m_captured = data;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &data) override {
    m_raw.push_back(data);
  }
};

class TvDevicePropertiesCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_TvDeviceProperties m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addTvDeviceProperties(const DRW_TvDeviceProperties &data) override {
    if (m_callCount == 0)
      m_captured = data;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &data) override {
    m_raw.push_back(data);
  }
};

class CsacDocumentOptionsCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_CsacDocumentOptions m_captured;
  std::vector<DRW_RawDxfObject> m_raw;

  void addCsacDocumentOptions(
      const DRW_CsacDocumentOptions &data) override {
    if (m_callCount == 0)
      m_captured = data;
    ++m_callCount;
  }

  void addRawDxfObject(const DRW_RawDxfObject &data) override {
    m_raw.push_back(data);
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

class BlockRawEntityCapture : public BlockCapture {
public:
  std::vector<DRW_RawDxfObject> m_entities;

  void addRawDxfEntity(const DRW_RawDxfObject &entity) override {
    m_entities.push_back(entity);
  }
};

class ProxyEntityCapture : public StubInterface {
public:
  std::vector<DRW_ProxyEntity> m_entities;
  std::vector<DRW_RawDxfObject> m_raw;

  void addProxyEntity(const DRW_ProxyEntity &data) override {
    m_entities.push_back(data);
  }

  void addRawDxfEntity(const DRW_RawDxfObject &data) override {
    m_raw.push_back(data);
  }
};

class ProxyObjectCapture : public StubInterface {
public:
  std::vector<DRW_ProxyObject> m_objects;
  std::vector<DRW_RawDxfObject> m_raw;

  void addProxyObject(const DRW_ProxyObject &data) override {
    m_objects.push_back(data);
  }

  void addRawDxfObject(const DRW_RawDxfObject &data) override {
    m_raw.push_back(data);
  }
};

class DxfClassCapture : public StubInterface {
public:
  std::vector<DRW_Class> m_classes;

  void addDxfClass(const DRW_Class &data) override {
    m_classes.push_back(data);
  }
};

class FailingDxfWriter final : public dxfWriter {
public:
  FailingDxfWriter() : dxfWriter(nullptr) {}

  bool writeString(int, std::string) override {
    ++calls;
    return false;
  }
  bool writeInt16(int, int) override {
    ++calls;
    return false;
  }
  bool writeInt32(int, int) override {
    ++calls;
    return false;
  }
  bool writeInt64(int, std::int64_t) override {
    ++calls;
    return false;
  }
  bool writeDouble(int, double) override {
    ++calls;
    return false;
  }
  bool writeBool(int, bool) override {
    ++calls;
    return false;
  }

  int calls = 0;
};

class CommentPolicyCapture : public RawObjectCapture {
public:
  int m_headerCount = 0;
  std::string m_headerComments;

  void addHeader(const DRW_Header *header) override {
    ++m_headerCount;
    m_headerComments = header ? header->getComments() : std::string{};
  }
};

class RawObjectEmitter : public StubInterface {
public:
  DRW_RawDxfObject m_obj;
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;
  void writeObjects() override {
    m_writeResult = m_rw->writeRawDxfObject(&m_obj);
  }
};

class FieldEmitter : public StubInterface {
public:
  DRW_Field m_field;
  DRW_FieldList m_list;
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeObjects() override {
    m_writeResult = m_rw->writeField(&m_field)
                 && m_rw->writeFieldList(&m_list);
  }
};

class RawEntityEmitter : public StubInterface {
public:
  DRW_RawDxfObject m_entity;
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;
  void writeEntities() override {
    m_writeResult = m_rw->writeRawDxfObject(&m_entity);
  }
};

class DimensionAssociationEmitter : public StubInterface {
public:
  DRW_DimensionAssociation m_association;
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeObjects() override {
    m_writeResult = m_rw->writeDimensionAssociation(&m_association);
  }
};

class EvaluationGraphEmitter : public StubInterface {
public:
  DRW_EvaluationGraph m_graph;
  const char *m_recordName = "EVALUATION_GRAPH";
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeObjects() override {
    m_writeResult = m_rw->writeEvaluationGraph(&m_graph, m_recordName);
  }
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
  std::uint32_t m_parentHandle = 0;
  dxfRW *m_rw = nullptr;
  void writeEntities() override {
    for (std::uint32_t src : m_sourceHandles) {
      DRW_Point pt;
      pt.basePoint = DRW_Coord(1.0, 2.0, 0.0);
      pt.handle = src;  // seed: source-handle key consumed by writeEntity
      pt.parentHandle = m_parentHandle;
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

class TableXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;

  static void addXData(DRW_TableEntry &entry, const char *payload, int intCode,
                       int intValue) {
    entry.extData.push_back(new DRW_Variant(1001, std::string{"TABLEAPP"}));
    entry.extData.push_back(new DRW_Variant(1000, std::string{payload}));
    entry.extData.push_back(
        new DRW_Variant(intCode, static_cast<std::int32_t>(intValue)));
  }

  void writeLTypes() override {
    DRW_LType lt;
    lt.name = "XDASH";
    lt.desc = "xdata ltype";
    addXData(lt, "ltype-xdata", 1071, 42);
    m_rw->writeLineType(&lt);
  }

  void writeTextstyles() override {
    DRW_Textstyle style;
    style.name = "XDATASTYLE";
    addXData(style, "style-xdata", 1070, 7);
    m_rw->writeTextstyle(&style);
  }

  void writeVports() override {
    DRW_Vport vp;
    vp.name = "XVPORT";
    addXData(vp, "vport-xdata", 1070, 8);
    m_rw->writeVport(&vp);
  }

  void writeViews() override {
    DRW_View view;
    view.name = "XVIEW";
    addXData(view, "view-xdata", 1070, 9);
    m_rw->writeView(&view);
  }

  void writeUCSs() override {
    DRW_UCS ucs;
    ucs.name = "XUCS";
    addXData(ucs, "ucs-xdata", 1070, 10);
    m_rw->writeUCS(&ucs);
  }

  void writeAppId() override {
    DRW_AppId tableApp;
    tableApp.name = "TABLEAPP";
    m_rw->writeAppId(&tableApp);

    DRW_AppId xapp;
    xapp.name = "XAPPID";
    addXData(xapp, "appid-xdata", 1070, 11);
    m_rw->writeAppId(&xapp);
  }

  void writeDimstyles() override {
    DRW_Dimstyle dim;
    dim.name = "XDIM";
    addXData(dim, "dimstyle-xdata", 1070, 12);
    m_rw->writeDimstyle(&dim);
  }
};

class InvalidXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;

  void writeLTypes() override {
    DRW_LType lt;
    lt.name = "INVALID_XDATA";
    auto value = std::make_unique<DRW_Variant>(1001, std::string{"BROKEN"});
    value->content.s = nullptr;
    if (lt.addExtData(std::move(value)))
      m_rw->writeLineType(&lt);
  }
};

class InvalidEntityXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeEntities() override {
    DRW_Point point;
    point.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    point.extData.push_back(
        std::make_shared<DRW_Variant>(1001, std::string{"ENTITYAPP"}));
    point.extData.push_back(
        std::make_shared<DRW_Variant>(1004, std::string{"ABC"}));
    m_writeResult = m_rw->writePoint(&point);
  }
};

class InvalidAppDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeLayers() override {
    DRW_Layer layer;
    layer.name = "INVALID_APPDATA";
    auto& group = layer.appData.emplace_back();
    group.emplace_back(102, std::string{"INVALID_APP"});
    group.emplace_back(1040, std::numeric_limits<double>::quiet_NaN());
    group.emplace_back(102, std::string{"}"});
    m_writeResult = m_rw->writeLayer(&layer);
  }
};

class InvalidDimstyleEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeDimstyles() override {
    DRW_Dimstyle style;
    style.name = "INVALID_DIMSTYLE";
    style.add("$DIMTXT", 140, std::string{"not-a-real"});
    m_writeResult = m_rw->writeDimstyle(&style);
  }
};

class DimstyleDirectionEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeDimstyles() override {
    DRW_Dimstyle style;
    style.name = "DIRECTION";
    style.dimtxtdirection = 1;
    style.add("$DIMTXTDIRECTION", 295, style.dimtxtdirection);
    m_writeResult = m_rw->writeDimstyle(&style);
  }
};

class InvalidObjectXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;
  bool m_writeResult = true;

  void writeObjects() override {
    DRW_Scale scale;
    scale.name = "INVALID_OBJECT_XDATA";
    auto* value = new DRW_Variant(1001, std::string{"OBJECTAPP"});
    value->content.s = nullptr;
    scale.extData.push_back(value);
    m_writeResult = m_rw->writeScale(&scale);
  }
};

class EntityXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;

  static void addXData(DRW_Entity &entity, const char *payload, int intCode,
                       int intValue) {
    entity.extData.push_back(
        std::make_shared<DRW_Variant>(1001, std::string{"ENTITYAPP"}));
    entity.extData.push_back(
        std::make_shared<DRW_Variant>(1000, std::string{payload}));
    entity.extData.push_back(
        std::make_shared<DRW_Variant>(intCode, static_cast<std::int32_t>(intValue)));
  }

  void writeEntities() override {
    DRW_Line line;
    line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
    line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
    addXData(line, "line-xdata", 1070, 17);
    m_rw->writeLine(&line);

    DRW_LWPolyline lw;
    auto v1 = lw.addVertex();
    v1->x = 0.0;
    v1->y = 0.0;
    auto v2 = lw.addVertex();
    v2->x = 2.0;
    v2->y = 0.0;
    addXData(lw, "lwpolyline-xdata", 1071, 18);
    m_rw->writeLWPolyline(&lw);

    DRW_Text text;
    text.basePoint = DRW_Coord(3.0, 4.0, 0.0);
    text.height = 0.5;
    text.text = "XDATA text";
    addXData(text, "text-xdata", 1070, 19);
    m_rw->writeText(&text);

    DRW_Point point;
    point.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    addXData(point, "point-xdata", 1070, 20);
    m_rw->writePoint(&point);

    DRW_Ray ray;
    ray.basePoint = DRW_Coord(0.0, 1.0, 0.0);
    ray.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    addXData(ray, "ray-xdata", 1070, 21);
    m_rw->writeRay(&ray);

    DRW_Xline xline;
    xline.basePoint = DRW_Coord(0.0, 2.0, 0.0);
    xline.secPoint = DRW_Coord(0.0, 1.0, 0.0);
    addXData(xline, "xline-xdata", 1070, 22);
    m_rw->writeXline(&xline);

    DRW_Circle circle;
    circle.basePoint = DRW_Coord(4.0, 4.0, 0.0);
    circle.radious = 2.0;
    addXData(circle, "circle-xdata", 1070, 23);
    m_rw->writeCircle(&circle);

    DRW_Arc arc;
    arc.basePoint = DRW_Coord(5.0, 5.0, 0.0);
    arc.radious = 1.5;
    arc.endangle = 1.0;
    addXData(arc, "arc-xdata", 1070, 24);
    m_rw->writeArc(&arc);

    DRW_Ellipse ellipse;
    ellipse.basePoint = DRW_Coord(6.0, 6.0, 0.0);
    ellipse.secPoint = DRW_Coord(2.0, 0.0, 0.0);
    ellipse.ratio = 0.5;
    ellipse.endparam = 6.283185307179586;
    addXData(ellipse, "ellipse-xdata", 1070, 25);
    m_rw->writeEllipse(&ellipse);

    DRW_Trace trace;
    trace.basePoint = DRW_Coord(0.0, 0.0, 0.0);
    trace.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    trace.thirdPoint = DRW_Coord(1.0, 1.0, 0.0);
    trace.fourPoint = DRW_Coord(0.0, 1.0, 0.0);
    addXData(trace, "trace-xdata", 1070, 26);
    m_rw->writeTrace(&trace);

    DRW_Solid solid;
    solid.basePoint = DRW_Coord(2.0, 0.0, 0.0);
    solid.secPoint = DRW_Coord(3.0, 0.0, 0.0);
    solid.thirdPoint = DRW_Coord(3.0, 1.0, 0.0);
    solid.fourPoint = DRW_Coord(2.0, 1.0, 0.0);
    addXData(solid, "solid-xdata", 1070, 27);
    m_rw->writeSolid(&solid);

    DRW_3Dface face;
    face.basePoint = DRW_Coord(4.0, 0.0, 0.0);
    face.secPoint = DRW_Coord(5.0, 0.0, 0.0);
    face.thirdPoint = DRW_Coord(5.0, 1.0, 0.0);
    face.fourPoint = DRW_Coord(4.0, 1.0, 0.0);
    addXData(face, "3dface-xdata", 1070, 28);
    m_rw->write3dface(&face);

    DRW_Polyline polyline;
    DRW_Vertex pv1;
    pv1.basePoint = DRW_Coord(0.0, 0.0, 0.0);
    polyline.addVertex(pv1);
    DRW_Vertex pv2;
    pv2.basePoint = DRW_Coord(1.0, 0.0, 0.0);
    polyline.addVertex(pv2);
    addXData(polyline, "polyline-xdata", 1070, 29);
    m_rw->writePolyline(&polyline);

    DRW_Spline spline;
    spline.normalVec = DRW_Coord(0.0, 0.0, 1.0);
    spline.degree = 1;
    spline.knotslist = {0.0, 0.0, 1.0, 1.0};
    spline.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    spline.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
    addXData(spline, "spline-xdata", 1070, 30);
    m_rw->writeSpline(&spline);

    DRW_Helix helix;
    helix.normalVec = DRW_Coord(0.0, 0.0, 1.0);
    helix.degree = 1;
    helix.knotslist = {0.0, 0.0, 1.0, 1.0};
    helix.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    helix.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 0.5, 0.0));
    helix.radius = 0.5;
    helix.turns = 1.0;
    helix.turnHeight = 1.0;
    addXData(helix, "helix-xdata", 1070, 31);
    m_rw->writeHelix(&helix);

    DRW_Hatch hatch;
    hatch.name = "SOLID";
    addXData(hatch, "hatch-xdata", 1070, 32);
    m_rw->writeHatch(&hatch);

    DRW_MPolygon mpolygon;
    mpolygon.name = "SOLID";
    addXData(mpolygon, "mpolygon-xdata", 1070, 33);
    m_rw->writeMPolygon(&mpolygon);

    DRW_Leader leader;
    leader.vertexlist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
    leader.vertexlist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
    addXData(leader, "leader-xdata", 1070, 34);
    m_rw->writeLeader(&leader);

    DRW_Insert insert;
    insert.name = "XDATA_BLOCK";
    insert.basePoint = DRW_Coord(1.0, 1.0, 0.0);
    addXData(insert, "insert-xdata", 1070, 35);
    m_rw->writeInsert(&insert);

    DRW_Table table;
    table.name = "XDATA_TABLE";
    table.basePoint = DRW_Coord(1.0, 2.0, 0.0);
    addXData(table, "table-xdata", 1070, 36);
    m_rw->writeTable(&table);

    DRW_Attrib attrib;
    attrib.basePoint = DRW_Coord(2.0, 2.0, 0.0);
    attrib.height = 0.25;
    attrib.text = "value";
    attrib.tag = "TAG";
    addXData(attrib, "attrib-xdata", 1070, 37);
    m_rw->writeAttrib(&attrib);

    DRW_Attdef attdef;
    attdef.basePoint = DRW_Coord(2.0, 3.0, 0.0);
    attdef.height = 0.25;
    attdef.text = "default";
    attdef.tag = "TAGDEF";
    attdef.prompt = "Prompt";
    addXData(attdef, "attdef-xdata", 1070, 38);
    m_rw->writeAttdef(&attdef);

    DRW_RText rtext;
    rtext.basePoint = DRW_Coord(3.0, 3.0, 0.0);
    rtext.height = 0.5;
    rtext.text = "RText";
    addXData(rtext, "rtext-xdata", 1070, 39);
    m_rw->writeRText(&rtext);

    DRW_ArcAlignedText arcText;
    arcText.text = "Arc text";
    arcText.m_center = DRW_Coord(4.0, 4.0, 0.0);
    arcText.m_radius = 2.0;
    arcText.m_endAngle = 1.0;
    arcText.height = 0.25;
    addXData(arcText, "arctext-xdata", 1070, 40);
    m_rw->writeArcAlignedText(&arcText);

    DRW_Tolerance tolerance;
    tolerance.insertionPoint = DRW_Coord(5.0, 5.0, 0.0);
    tolerance.text = "tol";
    tolerance.xAxisDirectionVector = DRW_Coord(1.0, 0.0, 0.0);
    addXData(tolerance, "tolerance-xdata", 1070, 41);
    m_rw->writeTolerance(&tolerance);

    DRW_DimLinear dim;
    dim.setDimPoint(DRW_Coord(0.0, 0.0, 0.0));
    dim.setTextPoint(DRW_Coord(0.5, 0.5, 0.0));
    dim.setDef1Point(DRW_Coord(0.0, 0.0, 0.0));
    dim.setDef2Point(DRW_Coord(1.0, 0.0, 0.0));
    addXData(dim, "dimension-xdata", 1070, 42);
    m_rw->writeDimension(&dim);

    DRW_DimArc arcDim;
    arcDim.setArcDefPoint(DRW_Coord(0.0, 0.0, 0.0));
    arcDim.setTextPoint(DRW_Coord(0.5, 0.5, 0.0));
    arcDim.setExtLine1(DRW_Coord(1.0, 0.0, 0.0));
    arcDim.setExtLine2(DRW_Coord(0.0, 1.0, 0.0));
    arcDim.setArcCenter(DRW_Coord(0.0, 0.0, 0.0));
    addXData(arcDim, "arcdim-xdata", 1070, 43);
    m_rw->writeDimension(&arcDim);

    DRW_DimLargeRadial largeRadial;
    largeRadial.setCenterPoint(DRW_Coord(0.0, 0.0, 0.0));
    largeRadial.setTextPoint(DRW_Coord(1.0, 1.0, 0.0));
    largeRadial.setChordPoint(DRW_Coord(2.0, 0.0, 0.0));
    largeRadial.overrideCenterPoint = DRW_Coord(0.1, 0.2, 0.0);
    largeRadial.jogPoint = DRW_Coord(1.0, 0.5, 0.0);
    largeRadial.jogAngle = 0.25;
    addXData(largeRadial, "largeradial-xdata", 1070, 44);
    m_rw->writeDimension(&largeRadial);

    DRW_MLeader mleader;
    addXData(mleader, "mleader-xdata", 1070, 45);
    m_rw->writeMultiLeader(&mleader);

    DRW_Light light;
    light.m_name = "XDATA_LIGHT";
    addXData(light, "light-xdata", 1070, 46);
    m_rw->writeLight(&light);

    DRW_Mesh mesh;
    addXData(mesh, "mesh-xdata", 1070, 47);
    m_rw->writeMesh(&mesh);

    DRW_Shape shape;
    shape.m_insertionPoint = DRW_Coord(6.0, 6.0, 0.0);
    shape.m_styleName = "STANDARD";
    addXData(shape, "shape-xdata", 1070, 48);
    m_rw->writeShape(&shape);

    DRW_Ole2Frame ole;
    addXData(ole, "ole2frame-xdata", 1070, 49);
    m_rw->writeOle2Frame(&ole);

    DRW_Viewport viewport;
    viewport.basePoint = DRW_Coord(7.0, 7.0, 0.0);
    viewport.pswidth = 2.0;
    viewport.psheight = 1.0;
    addXData(viewport, "viewport-xdata", 1070, 50);
    m_rw->writeViewport(&viewport);

    DRW_Image image;
    image.basePoint = DRW_Coord(8.0, 8.0, 0.0);
    image.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    image.vVector = DRW_Coord(0.0, 1.0, 0.0);
    image.sizeu = 10.0;
    image.sizev = 10.0;
    addXData(image, "image-xdata", 1070, 51);
    m_rw->writeImage(&image, "xdata.png");

    DRW_Wipeout wipeout;
    wipeout.basePoint = DRW_Coord(9.0, 9.0, 0.0);
    wipeout.secPoint = DRW_Coord(1.0, 0.0, 0.0);
    wipeout.vVector = DRW_Coord(0.0, 1.0, 0.0);
    wipeout.sizeu = 2.0;
    wipeout.sizev = 2.0;
    wipeout.m_clipBoundaryType = 2;
    wipeout.clipPath = {DRW_Coord{-0.5, -0.5, 0.0},
                         DRW_Coord{1.5, -0.5, 0.0},
                         DRW_Coord{0.5, 1.5, 0.0}};
    addXData(wipeout, "wipeout-xdata", 1070, 52);
    m_rw->writeWipeout(&wipeout);

    DRW_PointCloud pointCloud;
    pointCloud.savedFilename = "cloud.rcp";
    addXData(pointCloud, "pointcloud-xdata", 1070, 53);
    m_rw->writePointCloud(&pointCloud);

    DRW_PlaneSurface surface;
    surface.modelerFormatVersion = 1;
    addXData(surface, "surface-xdata", 1070, 54);
    m_rw->writeSurface(&surface);
  }
};

class ObjectXDataEmitter : public StubInterface {
public:
  dxfRW *m_rw = nullptr;

  static void addXData(DRW_TableEntry &entry, const char *payload, int value) {
    entry.extData.push_back(new DRW_Variant(1001, std::string{"OBJECTAPP"}));
    entry.extData.push_back(new DRW_Variant(1000, std::string{payload}));
    entry.extData.push_back(new DRW_Variant(1070, static_cast<std::int32_t>(value)));
  }

  void writeObjects() override {
    DRW_Scale scale;
    scale.handle = 0x510u;
    scale.parentHandle = 0xCu;
    scale.name = "XOBJ_SCALE";
    scale.paperUnits = 1.0;
    scale.drawingUnits = 48.0;
    addXData(scale, "scale-xdata", 21);
    m_rw->writeScale(&scale);

    DRW_MLineStyle mlineStyle;
    mlineStyle.handle = 0x511u;
    mlineStyle.parentHandle = 0xCu;
    mlineStyle.name = "XOBJ_MLINESTYLE";
    mlineStyle.description = "object xdata";
    addXData(mlineStyle, "mlinestyle-xdata", 22);
    m_rw->writeMLineStyle(&mlineStyle);

    DRW_Field field;
    field.handle = 0x512u;
    field.parentHandle = 0xCu;
    field.m_evaluatorId = "AcExpr";
    field.m_fieldCode = "1+1";
    addXData(field, "field-xdata", 23);
    m_rw->writeField(&field);

    DRW_WipeoutVariables vars;
    vars.handle = 0x513u;
    vars.parentHandle = 0xCu;
    vars.m_displayFrame = 1;
    addXData(vars, "wipeoutvars-xdata", 24);
    m_rw->writeWipeoutVariables(&vars);
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

void readDxf(const std::string &dxf, DRW_Interface &cap, const char *name,
             bool applyExtrusion = true) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW r(path.string().c_str());
  REQUIRE(r.read(&cap, applyExtrusion));
  std::filesystem::remove(path);
}

bool tryReadDxf(const std::string &dxf, DRW_Interface &cap, const char *name,
                bool applyExtrusion = true) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  const bool result = reader.read(&cap, applyExtrusion);
  std::filesystem::remove(path);
  return result;
}

} // namespace

namespace {
std::vector<std::pair<std::string, std::string>> readGroups(
    const std::filesystem::path &path);
bool recordHasConsecutive(
    const std::vector<std::pair<std::string, std::string>> &groups,
    const std::string &recordType, const std::string &recordName,
    const std::vector<std::pair<std::string, std::string>> &sequence);
}

TEST_CASE("DXF CLASS writer propagates failures", "[dxf][classes][safety]") {
    DRW_Class cls;
    cls.recName = "ACAD_PROXY_OBJECT";
    cls.className = "AcDbProxyObject";
  cls.appName = "ObjectDBX Classes";

  FailingDxfWriter writer;
  CHECK_FALSE(cls.write(&writer, DRW::AC1018));
  CHECK(writer.calls == 1);
    CHECK(cls.write(&writer, DRW::AC1009));
    CHECK(writer.calls == 1);
    CHECK_FALSE(cls.write(nullptr, DRW::AC1018));

    SECTION("invalid metadata is rejected before writing") {
        const auto checkRejected = [](DRW_Class invalid) {
            FailingDxfWriter writer;
            CHECK_FALSE(invalid.write(&writer, DRW::AC1018));
            CHECK(writer.calls == 0);
        };

        DRW_Class missingName = cls;
        missingName.className.clear();
        checkRejected(missingName);

        DRW_Class badProxy = cls;
        badProxy.proxyFlag = std::numeric_limits<std::uint16_t>::max() + 1;
        checkRejected(badProxy);

        DRW_Class badFlags = cls;
        badFlags.wasaProxyFlag = 2;
        checkRejected(badFlags);

        DRW_Class badString = cls;
        badString.appName = "ObjectDBX\nClasses";
        checkRejected(badString);
    }
}

TEST_CASE("DXF writer validates and freezes CLASS metadata",
          "[dxf][classes][writer][safety]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_class_writer_preflight.dxf";

  const auto validClass = [](const char *recordName,
                             const char *className) {
    DRW_Class cls;
    cls.recName = recordName;
    cls.className = className;
    cls.appName = "ObjectDBX Classes";
    cls.proxyFlag = 1153;
    cls.instanceCount = 1;
    return cls;
  };

  SECTION("duplicate record names are rejected") {
    std::filesystem::remove(path);
    DRW_Class first = validClass("CUSTOM", "AcDbCustom");
    DRW_Class duplicate = validClass("custom", "AcDbOther");
    dxfRW writer(path.string().c_str());
    writer.setDxfClasses({first, duplicate});
    StubInterface emitter;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(std::filesystem::exists(path));
  }

  SECTION("duplicate class identities are rejected") {
    std::filesystem::remove(path);
    DRW_Class first = validClass("CUSTOM_A", "AcDbCustom");
    DRW_Class duplicate = validClass("CUSTOM_B", "acdbcustom");
    dxfRW writer(path.string().c_str());
    writer.setDxfClasses({first, duplicate});
    StubInterface emitter;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(std::filesystem::exists(path));
  }

  SECTION("late registration is rejected after preflight") {
    class LateClassEmitter final : public StubInterface {
    public:
      dxfRW *writer = nullptr;

      void writeEntities() override {
        DRW_Class late;
        late.recName = "LATE";
        late.className = "AcDbLate";
        late.appName = "ObjectDBX Classes";
        late.proxyFlag = 1153;
        late.instanceCount = 1;
        writer->setDxfClasses({late});
      }
    } emitter;

    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    emitter.writer = &writer;
    writer.setDxfClasses({validClass("INITIAL", "AcDbInitial")});
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(std::filesystem::exists(path));
  }
  std::filesystem::remove(path);
}

TEST_CASE("DXF unknown sections are skipped as one bounded record stream",
          "[dxf][sections][safety]") {
  std::string dxf =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC1015\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nCUSTOM_SECTION\n"
      // This must remain data in CUSTOM_SECTION, not dispatch as ENTITIES.
      "2\nENTITIES\n0\nLINE\n8\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  StubInterface capture;
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_unknown_section_skip.dxf";
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  const bool result = reader.read(&capture, true);
  CHECK(result);
  std::filesystem::remove(path);

  const std::string truncated =
      "0\nSECTION\n2\nCUSTOM_SECTION\n2\nENTITIES\n0\nEOF\n";
  CHECK_FALSE(tryReadDxf(truncated, capture,
                         "lc_unknown_section_truncated.dxf"));
}

TEST_CASE("DXF VIEWPORT preserves spec field and handle codes",
          "[dxf][viewport]") {
  ViewportCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nVIEWPORT\n5\n10\n330\n2\n100\nAcDbEntity\n"
      "8\n0\n100\nAcDbViewport\n"
      "10\n1.0\n20\n2.0\n30\n3.0\n40\n40.0\n41\n30.0\n"
      "12\n4.0\n22\n5.0\n13\n6.0\n23\n7.0\n"
      "14\n8.0\n24\n9.0\n15\n10.0\n25\n11.0\n"
      "16\n0.0\n26\n1.0\n36\n0.0\n17\n12.0\n27\n13.0\n37\n14.0\n"
      "42\n75.0\n43\n0.5\n44\n100.0\n45\n20.0\n50\n0.25\n51\n0.75\n"
      "61\n5\n63\n7\n68\n2\n69\n3\n71\n1\n72\n80\n74\n1\n79\n4\n"
      "90\n35\n110\n15.0\n120\n16.0\n130\n17.0\n"
      "111\n0.0\n121\n1.0\n131\n0.0\n112\n-1.0\n122\n0.0\n132\n0.0\n"
      "141\n0.4\n142\n0.8\n146\n18.0\n170\n4\n281\n2\n282\n2\n292\n0\n"
      "331\n21\n331\n22\n332\n27\n333\n29\n340\n24\n345\n25\n346\n26\n348\n28\n361\n2A\n"
      "431\nAmbientName\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_viewport_entity_fields.dxf", false);

  REQUIRE(capture.m_viewports.size() == 1u);
  const DRW_Viewport &viewport = capture.m_viewports.front();
  CHECK(viewport.basePoint.z == 3.0);
  CHECK(viewport.snapSpPY == 9.0);
  CHECK(viewport.viewDir.y == 1.0);
  CHECK(viewport.viewTarget.z == 14.0);
  CHECK(viewport.viewLength == 75.0);
  CHECK(viewport.circleZoom == 80.0);
  CHECK(viewport.majorGridLines == 5);
  CHECK(viewport.ucsPerViewport);
  CHECK(viewport.ucsAtOrigin);
  CHECK(viewport.ucsOrthographicType == 4);
  CHECK(viewport.shadePlotMode == 4);
  CHECK_FALSE(viewport.useDefaultLighting);
  CHECK(viewport.ambientColor == 7u);
  CHECK(viewport.ambientColorName == "AmbientName");
  CHECK(viewport.frozenLayerHandles == std::vector<std::uint32_t>{0x21u, 0x22u});
  CHECK(viewport.backgroundHandle == 0x27u);
  CHECK(viewport.shadePlotHandle == 0x29u);
  CHECK(viewport.clipBoundaryHandle == 0x24u);
  CHECK(viewport.visualStyleHandle == 0x28u);
  CHECK(viewport.m_sunHandle == 0x2Au);
}

TEST_CASE("DXF VIEWPORT writer emits spec field and handle codes",
          "[dxf][viewport][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeEntities() override {
      DRW_Viewport viewport;
      viewport.basePoint = DRW_Coord{1.0, 2.0, 3.0};
      viewport.pswidth = 40.0;
      viewport.psheight = 30.0;
      viewport.viewDir = DRW_Coord{0.0, 1.0, 0.0};
      viewport.viewTarget = DRW_Coord{4.0, 5.0, 6.0};
      viewport.viewHeight = 20.0;
      viewport.viewLength = 75.0;
      viewport.circleZoom = 80.0;
      viewport.majorGridLines = 5;
      viewport.ucsAtOrigin = true;
      viewport.ucsPerViewport = true;
      viewport.ucsOrthographicType = 4;
      viewport.shadePlotMode = 4;
      viewport.useDefaultLighting = false;
      viewport.defaultLightingType = 2;
      viewport.ambientColor = 7;
      viewport.ambientColorName = "AmbientName";
      viewport.frozenLayerHandles = {0x21, 0x22};
      viewport.backgroundHandle = 0x27;
      viewport.shadePlotHandle = 0x29;
      viewport.clipBoundaryHandle = 0x24;
      viewport.namedUcsHandle = 0x25;
      viewport.baseUcsHandle = 0x26;
      viewport.visualStyleHandle = 0x28;
      viewport.m_sunHandle = 0x2A;
      REQUIRE(m_writer->writeViewport(&viewport));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_viewport_entity_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  ViewportCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  std::filesystem::remove(path);
  REQUIRE(capture.m_viewports.size() == 1u);
  const DRW_Viewport &viewport = capture.m_viewports.front();
  CHECK(viewport.circleZoom == 80.0);
  CHECK(viewport.majorGridLines == 5);
  CHECK(viewport.ucsPerViewport);
  CHECK(viewport.ucsAtOrigin);
  CHECK(viewport.shadePlotMode == 4);
  CHECK_FALSE(viewport.useDefaultLighting);
  CHECK(viewport.ambientColor == 7u);
  CHECK(viewport.ambientColorName == "AmbientName");
  CHECK(viewport.frozenLayerHandles
        == std::vector<std::uint32_t>{0x21u, 0x22u});
  CHECK(viewport.backgroundHandle == 0x27u);
  CHECK(viewport.shadePlotHandle == 0x29u);
  CHECK(viewport.m_sunHandle == 0x2Au);
}

TEST_CASE("DXF VIEWPORT rejects invalid scalar domains transactionally",
          "[dxf][viewport][safety]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nVIEWPORT\n5\n10\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbViewport\n"
      "10\n1\n20\n2\n40\n40\n41\n30\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  ViewportCapture capture;
  CHECK_FALSE(tryReadDxf(prefix + "281\n256\n" + suffix, capture,
                         "lc_viewport_invalid_render_mode.dxf"));
  CHECK(capture.m_viewports.empty());

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_viewport_invalid_write.dxf";
  std::filesystem::remove(path);
  ViewportEmitter emitter;
  emitter.m_viewport.renderMode = 256;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  CHECK(slurp(path).find("VIEWPORT") == std::string::npos);
  std::filesystem::remove(path);
}

TEST_CASE("DXF VIEWPORT stops after a failed common entity prefix",
          "[dxf][viewport][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_viewport_invalid_appdata.dxf";
  std::filesystem::remove(path);

  ViewportEmitter emitter;
  auto &group = emitter.m_viewport.appData.emplace_back();
  group.emplace_back(102, std::string{"{INVALID_VIEWPORT_APPDATA"});
  group.emplace_back(1040, std::numeric_limits<double>::quiet_NaN());
  group.emplace_back(102, std::string{"}"});
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }

  CHECK_FALSE(std::filesystem::exists(path));
}

TEST_CASE("DXF VIEW preserves modern lighting CMC and handles",
          "[dxf][tables][view]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_view_modern_fields.dxf";
  std::filesystem::remove(path);
  ViewEmitter emitter;
  emitter.m_view.name = "MODERN_VIEW";
  emitter.m_view.flags = 1;
  emitter.m_view.viewMode = 16;
  emitter.m_view.hasUCS = true;
  emitter.m_view.ucsOrthoType = 2;
  emitter.m_view.m_useDefaultLights = false;
  emitter.m_view.m_defaultLightingType = 2;
  emitter.m_view.m_brightness = 0.25;
  emitter.m_view.m_contrast = 0.75;
  emitter.m_view.m_ambientColor = 7;
  emitter.m_view.m_ambientColorRgb = 0x123456;
  emitter.m_view.m_ambientColorName = "Book$AmbientName";
  emitter.m_view.m_backgroundHandle = 0x21;
  emitter.m_view.m_visualStyleHandle = 0x22;
  emitter.m_view.m_sunHandle = 0x23;
  emitter.m_view.m_liveSectionHandle = 0x24;
  emitter.m_view.baseUCS_ID = 0x25;
  emitter.m_view.namedUCS_ID = 0x26;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    CHECK(emitter.m_writeResult);
  }

  ViewCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  std::filesystem::remove(path);
  REQUIRE(capture.m_views.size() == 1u);
  const DRW_View &view = capture.m_views.front();
  CHECK(view.name == "MODERN_VIEW");
  CHECK(view.flags == 1);
  CHECK(view.viewMode == 16);
  CHECK(view.hasUCS);
  CHECK(view.ucsOrthoType == 2);
  CHECK_FALSE(view.m_useDefaultLights);
  CHECK(view.m_defaultLightingType == 2);
  CHECK(view.m_brightness == Catch::Approx(0.25));
  CHECK(view.m_contrast == Catch::Approx(0.75));
  CHECK(view.m_ambientColor == 7u);
  CHECK(view.m_ambientColorRgb == 0x123456);
  CHECK(view.m_ambientColorName == "Book$AmbientName");
  CHECK(view.m_backgroundHandle == 0x21u);
  CHECK(view.m_visualStyleHandle == 0x22u);
  CHECK(view.m_sunHandle == 0x23u);
  CHECK(view.m_liveSectionHandle == 0x24u);
  CHECK(view.baseUCS_ID == 0x25u);
  CHECK(view.namedUCS_ID == 0x26u);
}

TEST_CASE("DXF VIEW rejects invalid payloads transactionally",
          "[dxf][tables][view][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_view_invalid_write.dxf";
  std::filesystem::remove(path);
  ViewEmitter emitter;
  emitter.m_view.renderMode = 256;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  std::filesystem::remove(path);
}

TEST_CASE("DXF VIEW camera-plottable field follows its version gate",
          "[dxf][view][write]") {
  const auto hasCameraPlottable = [](const auto &groups) {
    bool inView = false;
    for (const auto &group : groups) {
      if (group.first == "0") {
        if (inView)
          return false;
        inView = group.second == "VIEW";
      } else if (inView && group.first == "73") {
        return true;
      }
    }
    return false;
  };

  const auto writeViews = [](DRW::Version version, const char *fileName) {
    const auto path = std::filesystem::temp_directory_path() / fileName;
    std::filesystem::remove(path);
    {
      dxfRW writer(path.string().c_str());
      TableXDataEmitter emitter;
      emitter.m_rw = &writer;
      REQUIRE(writer.write(&emitter, version, false));
    }
    const auto groups = readGroups(path);
    std::filesystem::remove(path);
    return groups;
  };

  CHECK_FALSE(hasCameraPlottable(
      writeViews(DRW::AC1018, "lc_view_r2004_version_gate.dxf")));
  CHECK(hasCameraPlottable(
      writeViews(DRW::AC1021, "lc_view_r2007_version_gate.dxf")));
}

TEST_CASE("DXF UCS writer preserves orthographic fields",
          "[dxf][ucs][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeUCSs() override {
      DRW_UCS ucs;
      ucs.name = "ORTHO-UCS";
      ucs.orthoOrigin = DRW_Coord{1.0, 2.0, 3.0};
      ucs.orthoType = 4;
      ucs.elevation = 6.0;
      ucs.baseUcsHandle.ref = 0x42;
      ucs.namedUcsHandle.ref = 0x43;
      REQUIRE(m_writer->writeUCS(&ucs));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_ucs_orthographic_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  std::vector<std::pair<std::string, std::string>> record;
  bool inRecord = false;
  for (const auto &group : groups) {
    if (group.first == "0") {
      if (inRecord)
        break;
      inRecord = group.second == "UCS";
    }
    if (inRecord)
      record.push_back(group);
  }
  REQUIRE(!record.empty());
  const std::vector<std::pair<std::string, double>> expected = {
      {"13", 1.0}, {"23", 2.0}, {"33", 3.0},
      {"79", 4.0}, {"146", 6.0}};
  std::size_t next = 0;
  for (const auto &expectedGroup : expected) {
    const std::string &code = expectedGroup.first;
    const double value = expectedGroup.second;
    auto it = std::find_if(record.begin() + static_cast<std::ptrdiff_t>(next),
                           record.end(),
                           [&](const auto &group) { return group.first == code; });
    REQUIRE(it != record.end());
    CHECK(std::stod(it->second) == Catch::Approx(value));
    next = static_cast<std::size_t>(std::distance(record.begin(), it)) + 1;
  }
  CHECK(std::none_of(record.begin(), record.end(), [](const auto &group) {
    return group.first == "71";
  }));
  const auto findGroup = [&record](const char *code) {
    return std::find_if(record.begin(), record.end(),
                        [code](const auto &group) {
                          return group.first == code;
                        });
  };
  const auto base = findGroup("346");
  const auto named = findGroup("345");
  REQUIRE(base != record.end());
  REQUIRE(named != record.end());
  CHECK(base->second == "42");
  CHECK(named->second == "43");
}

TEST_CASE("DXF UCS reads orthographic view and handle fields",
          "[dxf][ucs][read]") {
  UcsCapture capture;
  const std::string dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nUCS\n70\n1\n"
      "0\nUCS\n5\n10\n330\n7\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbUCSTableRecord\n2\nREAD-UCS\n70\n0\n"
      "10\n1\n20\n2\n30\n3\n11\n1\n21\n0\n31\n0\n"
      "12\n0\n22\n1\n32\n0\n13\n4\n23\n5\n33\n6\n"
      "79\n4\n146\n7\n346\n42\n345\n43\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_ucs_orthographic_read.dxf", false);

  REQUIRE(capture.m_ucss.size() == 1u);
  const DRW_UCS &ucs = capture.m_ucss.front();
  CHECK(ucs.name == "READ-UCS");
  CHECK(ucs.orthoType == 4);
  CHECK(ucs.elevation == 7.0);
  CHECK(ucs.orthoOrigin.z == 6.0);
  CHECK(ucs.baseUcsHandle.ref == 0x42u);
  CHECK(ucs.namedUcsHandle.ref == 0x43u);
}

TEST_CASE("DXF UCS rejects non-finite payloads before writing",
          "[dxf][ucs][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_ucs_invalid_write.dxf";
  std::filesystem::remove(path);
  UcsEmitter emitter;
  emitter.m_ucs.origin.x = std::numeric_limits<double>::quiet_NaN();
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  std::filesystem::remove(path);
}

TEST_CASE("DXF APPID preserves its unknown group-71 byte",
          "[dxf][appid][read]") {
  AppIdCapture capture;
  const std::string dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nAPPID\n70\n1\n"
      "0\nAPPID\n5\n10\n330\n9\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbRegAppTableRecord\n2\nREAD-APP\n70\n0\n71\n7\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_appid_unknown71_read.dxf", false);

  REQUIRE(capture.m_appIds.size() == 1u);
  CHECK(capture.m_appIds.front().name == "READ-APP");
  CHECK(capture.m_appIds.front().unknown71 == 7u);
}

TEST_CASE("DXF APPID validates and writes its unknown group-71 byte",
          "[dxf][appid][write][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_appid_unknown71_write.dxf";
  std::filesystem::remove(path);
  AppIdEmitter emitter;
  emitter.m_appId.name = "WRITE-APP";
  emitter.m_appId.unknown71 = 9;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);
  CHECK(recordHasConsecutive(groups, "APPID", "WRITE-APP",
                             {{"71", "9"}}));

  std::filesystem::remove(path);
  emitter.m_appId.flags = 1;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  std::filesystem::remove(path);
}

TEST_CASE("DXF TEXTSTYLE preserves typed fields and rejects invalid flags",
          "[dxf][textstyle]") {
  TextStyleCapture capture;
  const std::string dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nSTYLE\n70\n1\n"
      "0\nSTYLE\n5\n10\n330\n3\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbTextStyleTableRecord\n2\nREAD-STYLE\n70\n5\n"
      "40\n2.5\n41\n0.8\n50\n0.25\n71\n6\n42\n3.5\n"
      "3\nfont.ttf\n4\nbigfont.shx\n1071\n33554433\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_textstyle_typed_read.dxf", false);

  REQUIRE(capture.m_styles.size() == 1u);
  const DRW_Textstyle &style = capture.m_styles.front();
  CHECK(style.name == "READ-STYLE");
  CHECK(style.flags == 5);
  CHECK(style.height == Catch::Approx(2.5));
  CHECK(style.width == Catch::Approx(0.8));
  CHECK(style.oblique == Catch::Approx(0.25));
  CHECK(style.genFlag == 6);
  CHECK(style.lastHeight == Catch::Approx(3.5));
  CHECK(style.font == "font.ttf");
  CHECK(style.bigFont == "bigfont.shx");
  CHECK(style.fontFamily == 33554433);

  const std::string invalid =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nSTYLE\n70\n1\n"
      "0\nSTYLE\n5\n11\n330\n3\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbTextStyleTableRecord\n2\nBAD-STYLE\n70\n0\n"
      "40\n1\n41\n1\n50\n0\n71\n1\n42\n1\n3\ntxt\n4\n\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  TextStyleCapture rejected;
  CHECK_FALSE(tryReadDxf(invalid, rejected, "lc_textstyle_invalid_read.dxf",
                         false));
  CHECK(rejected.m_styles.empty());
}

TEST_CASE("DXF TEXTSTYLE writer validates fields and propagates failures",
          "[dxf][textstyle][write][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_textstyle_typed_write.dxf";
  std::filesystem::remove(path);
  TextStyleEmitter emitter;
  emitter.m_style.name = "WRITE-STYLE";
  emitter.m_style.flags = 5;
  emitter.m_style.height = 2.5;
  emitter.m_style.width = 0.8;
  emitter.m_style.oblique = 0.25;
  emitter.m_style.genFlag = 6;
  emitter.m_style.lastHeight = 3.5;
  emitter.m_style.font = "font.ttf";
  emitter.m_style.bigFont = "bigfont.shx";
  emitter.m_style.fontFamily = 33554433;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);
  CHECK(recordHasConsecutive(
      groups, "STYLE", "WRITE-STYLE",
      {{"70", "5"}, {"40", "2.5"}, {"41", "0.8"}, {"50", "0.25"},
       {"71", "6"}, {"42", "3.5"}, {"3", "font.ttf"},
       {"4", "bigfont.shx"}, {"1071", "33554433"}}));

  emitter.m_style.genFlag = 1;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  std::filesystem::remove(path);

  emitter.m_style.genFlag = 0;
  emitter.m_style.height = std::numeric_limits<double>::quiet_NaN();
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  std::filesystem::remove(path);
}

TEST_CASE("DXF structural section keywords are case insensitive",
          "[dxf][sections][compatibility]") {
  const std::string dxf =
      "0\nsection\n2\nheader\n"
      "9\n$ACADVER\n1\nAC1015\n"
      "0\nendsec\n"
      "0\nsection\n2\nentities\n"
      "0\nLINE\n5\n1\n8\n0\n10\n1\n20\n2\n11\n3\n21\n4\n"
      "0\nendsec\n0\neof\n";
  LineCapture capture;
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_case_insensitive_sections.dxf";
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  const bool result = reader.read(&capture, true);
  INFO("DXF error: " << static_cast<int>(reader.getError()));
  std::filesystem::remove(path);
  CHECK(result);
  CHECK(capture.m_callCount == 1);
  CHECK(capture.m_captured.basePoint.x == Catch::Approx(1.0));
  CHECK(capture.m_captured.basePoint.y == Catch::Approx(2.0));
  CHECK(capture.m_captured.secPoint.x == Catch::Approx(3.0));
  CHECK(capture.m_captured.secPoint.y == Catch::Approx(4.0));
}

TEST_CASE("DXF unknown sections round-trip with typed groups",
          "[dxf][sections][rawcapture]") {
  const std::string source =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC1015\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nCUSTOM_SECTION\n"
      "2\nENTITIES\n0\nCUSTOM_RECORD\n5\n4A\n70\n0007\n40\n-0.000E+0\n"
      "310\nA1B2\n"
      "0\nENDSEC\n0\nEOF\n";

  RawSectionCapture captured;
  readDxf(source, captured, "lc_unknown_section_capture.dxf");
  REQUIRE(captured.m_sections.size() == 1);
  const DRW_RawDxfSection &section = captured.m_sections.front();
  CHECK(section.m_name == "CUSTOM_SECTION");
  CHECK(section.m_version == DRW::AC1015);
  CHECK(section.m_hasRawValues);
  REQUIRE(section.m_groups.size() == 6);
  CHECK(section.m_groups[0].code() == 2);
  CHECK(std::string(section.m_groups[0].c_str()) == "ENTITIES");
  CHECK(section.m_groups[2].code() == 5);
  CHECK(std::string(section.m_groups[2].c_str()) == "4A");
  CHECK(section.m_groups[3].i_val() == 7);
  CHECK(section.m_groups[4].d_val() == Catch::Approx(0.0));
  CHECK(section.m_groups[5].code() == 310);
  CHECK(std::string(section.m_groups[5].c_str()) == "A1B2");
  CHECK(section.m_rawValues[3] == "0007");
  CHECK(section.m_rawValues[4] == "-0.000E+0");
  CHECK(section.m_rawValues[5] == "A1B2");

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_unknown_section_capture_rt.dxf";
  std::filesystem::remove(path);
  NullObjectEmitter emitter;
  {
    dxfRW writer(path.string().c_str());
    writer.setRawDxfSections({section});
    REQUIRE(writer.write(&emitter, DRW::AC1015, false));
  }

  RawSectionCapture replayed;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&replayed, true));
  }
  REQUIRE(replayed.m_sections.size() == 1);
  CHECK(replayed.m_sections.front().m_name == section.m_name);
  CHECK(replayed.m_sections.front().m_groups.size() == section.m_groups.size());
  CHECK(replayed.m_sections.front().m_rawValues == section.m_rawValues);
  std::filesystem::remove(path);

  const auto binaryPath = std::filesystem::temp_directory_path() /
                          "lc_unknown_section_capture_rt.bin";
  const auto binaryAsciiPath = std::filesystem::temp_directory_path() /
                               "lc_unknown_section_capture_rt_ascii.dxf";
  std::filesystem::remove(binaryPath);
  std::filesystem::remove(binaryAsciiPath);
  {
    dxfRW writer(binaryPath.string().c_str());
    writer.setRawDxfSections({section});
    REQUIRE(writer.write(&emitter, DRW::AC1015, true));
  }

  RawSectionCapture binaryCaptured;
  {
    dxfRW reader(binaryPath.string().c_str());
    REQUIRE(reader.read(&binaryCaptured, true));
  }
  REQUIRE(binaryCaptured.m_sections.size() == 1);
  const DRW_RawDxfSection &binarySection = binaryCaptured.m_sections.front();
  CHECK_FALSE(binarySection.m_hasRawValues);
  REQUIRE(binarySection.m_rawValues.size() == binarySection.m_groups.size());
  CHECK(binarySection.m_rawValues[3].empty());

  {
    dxfRW writer(binaryAsciiPath.string().c_str());
    writer.setRawDxfSections({binarySection});
    REQUIRE(writer.write(&emitter, DRW::AC1015, false));
  }
  RawSectionCapture binaryReplayedAsAscii;
  {
    dxfRW reader(binaryAsciiPath.string().c_str());
    REQUIRE(reader.read(&binaryReplayedAsAscii, true));
  }
  REQUIRE(binaryReplayedAsAscii.m_sections.size() == 1);
  const DRW_RawDxfSection &asciiSection =
      binaryReplayedAsAscii.m_sections.front();
  CHECK(asciiSection.m_hasRawValues);
  REQUIRE(asciiSection.m_groups.size() == section.m_groups.size());
  CHECK(asciiSection.m_groups[3].i_val() == 7);
  CHECK(asciiSection.m_groups[4].d_val() == Catch::Approx(0.0));
  CHECK(std::string(asciiSection.m_groups[5].c_str()) == "A1B2");
  CHECK_FALSE(asciiSection.m_rawValues[3].empty());
  CHECK_FALSE(asciiSection.m_rawValues[4].empty());
  std::filesystem::remove(binaryPath);
  std::filesystem::remove(binaryAsciiPath);

  const auto crossVersionPath = std::filesystem::temp_directory_path() /
                                "lc_unknown_section_cross_version.dxf";
  std::filesystem::remove(crossVersionPath);
  {
    dxfRW writer(crossVersionPath.string().c_str());
    writer.setRawDxfSections({section});
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
  }
  std::filesystem::remove(crossVersionPath);

  const auto reservedSectionPath = std::filesystem::temp_directory_path() /
                                   "lc_unknown_section_reserved_name.dxf";
  std::filesystem::remove(reservedSectionPath);
  DRW_RawDxfSection reservedSection = section;
  reservedSection.m_name = "objects";
  {
    dxfRW writer(reservedSectionPath.string().c_str());
    writer.setRawDxfSections({reservedSection});
    CHECK_FALSE(writer.write(&emitter, DRW::AC1015, false));
  }
  std::filesystem::remove(reservedSectionPath);

  const std::string unbalanced =
      "0\nSECTION\n2\nCUSTOM_SECTION\n"
      "102\n{APPDATA\n1\nvalue\n0\nENDSEC\n0\nEOF\n";
  RawSectionCapture rejected;
  CHECK_FALSE(tryReadDxf(unbalanced, rejected,
                         "lc_unknown_section_unbalanced.dxf"));
  CHECK(rejected.m_sections.empty());
}

TEST_CASE("DXF binary writer latches malformed chunk failures",
          "[dxf][writer][safety]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_writer_invalid_chunk.bin";
  std::filesystem::remove(path);
  {
    std::ofstream output(path, std::ios::binary);
    dxfWriterBinary writer(&output);
    CHECK_FALSE(writer.writeString(310, "GG"));
    CHECK(writer.hasWriteError());
  }
  std::filesystem::remove(path);
}

TEST_CASE("DXF ASCII binary chunks honor the 127-byte limit",
          "[dxf][reader][binary][malformed]") {
  StubInterface capture;
  const std::string oversized(256, 'A');
  const std::string dxf =
      "0\nSECTION\n2\nCUSTOM_SECTION\n"
      "310\n" + oversized + "\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK_FALSE(tryReadDxf(dxf, capture, "lc_ascii_oversized_binary_chunk.dxf"));
}

TEST_CASE("DXF binary writer uses explicit little-endian primitive encoding",
          "[dxf][writer][binary]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_writer_little_endian.bin";
  std::filesystem::remove(path);
  {
    std::ofstream output(path, std::ios::binary);
    dxfWriterBinary writer(&output);
    REQUIRE(writer.writeInt16(10, 0x8001));
    REQUIRE(writer.writeInt32(90, -2));
    REQUIRE(writer.writeInt64(160, 0x0102030405060708LL));
    REQUIRE(writer.writeDouble(40, 1.0));
    CHECK_FALSE(writer.writeInt16(70, 0x10000));
    CHECK_FALSE(writer.writeInt16(70, -32769));
    CHECK_FALSE(writer.writeInt16(1072, 0));
    CHECK_FALSE(writer.writeString(999, "comment"));
    CHECK_FALSE(writer.writeString(1, std::string{"A\0B", 3}));
    CHECK_FALSE(writer.writeInt16(-1, 0));
    CHECK_FALSE(writer.writeInt32(0x10000, 0));
    CHECK(writer.hasWriteError());
  }

  std::ifstream input(path, std::ios::binary);
  const std::vector<std::uint8_t> bytes{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  const std::vector<std::uint8_t> expected{
      0x0A, 0x00, 0x01, 0x80,
      0x5A, 0x00, 0xFE, 0xFF, 0xFF, 0xFF,
      0xA0, 0x00, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
      0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xF0, 0x3F};
  CHECK(bytes == expected);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF ASCII writer preserves 32-bit values and rejects truncated fields",
          "[dxf][writer][ascii]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_writer_int16_range.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream output(path);
    dxfWriterAscii writer(&output);
    REQUIRE(writer.writeInt32(90, 70000));
    CHECK_FALSE(writer.writeInt16(70, 0x10000));
    CHECK_FALSE(writer.writeInt16(70, -32769));
    CHECK_FALSE(writer.writeString(1072, "invalid-code"));
    CHECK_FALSE(writer.writeString(1, "LINE\n0\nEOF"));
    CHECK(writer.hasWriteError());
  }

  std::ifstream input(path);
  const std::string text{std::istreambuf_iterator<char>(input),
                         std::istreambuf_iterator<char>()};
  CHECK(text.find("70000") != std::string::npos);
  CHECK(text.find("65536") == std::string::npos);
  CHECK(text.find("-32769") == std::string::npos);
  CHECK(text.find("invalid-code") == std::string::npos);
  CHECK(text.find("LINE\n0\nEOF") == std::string::npos);
  input.close();
  std::filesystem::remove(path);
}

TEST_CASE("DXF LEADER validates its declared vertex count",
          "[dxf][leader][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLEADER\n5\n30\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbLeader\n"
      "71\n1\n72\n0\n73\n3\n74\n1\n75\n1\n"
      "40\n1\n41\n1\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  LeaderCapture malformed;
  const std::string mismatch =
      prefix + "76\n2\n10\n0\n20\n0\n30\n0\n" + suffix;
  CHECK_FALSE(tryReadDxf(mismatch, malformed, "lc_leader_count_mismatch.dxf"));
  CHECK(malformed.m_callCount == 0);

  LeaderCapture valid;
  const std::string matching =
      prefix + "76\n2\n"
      "10\n0\n20\n0\n30\n0\n"
      "10\n1\n20\n1\n30\n0\n" + suffix;
  CHECK(tryReadDxf(matching, valid, "lc_leader_count_valid.dxf"));
  REQUIRE(valid.m_callCount == 1);
  CHECK(valid.m_captured.vertexlist.size() == 2);
  CHECK(valid.m_captured.flag == 3);
}

TEST_CASE("DXF LEADER writer rejects invalid payloads before output",
          "[dxf][leader][writer][safety]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_leader_invalid_write.dxf";

  LeaderEmitter nullVertex;
  nullVertex.m_leader.vertexlist.push_back(nullptr);
  {
    dxfRW writer(path.string().c_str());
    nullVertex.m_rw = &writer;
    CHECK_FALSE(writer.write(&nullVertex, DRW::AC1021, false));
    CHECK_FALSE(nullVertex.m_writeResult);
  }
  CHECK(slurp(path).find("LEADER") == std::string::npos);
  std::filesystem::remove(path);

  LeaderEmitter nonFinite;
  nonFinite.m_leader.textheight = std::numeric_limits<double>::quiet_NaN();
  {
    dxfRW writer(path.string().c_str());
    nonFinite.m_rw = &writer;
    CHECK_FALSE(writer.write(&nonFinite, DRW::AC1021, false));
    CHECK_FALSE(nonFinite.m_writeResult);
  }
  CHECK(slurp(path).find("LEADER") == std::string::npos);
  std::filesystem::remove(path);
}

TEST_CASE("DXF BLOCK requires a non-default name before callback",
          "[dxf][block][malformed]") {
  BlockCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n20\n8\nBlockLayer\n70\n0\n"
      "0\nLINE\n5\n21\n8\nInnerLayer\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDBLK\n5\n22\n8\nBlockLayer\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_block_missing_name.dxf"));
  CHECK(cap.m_blocks.empty());
}

TEST_CASE("DXF BLOCK record boundaries are case-insensitive",
          "[dxf][block]") {
  BlockCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nblock\n5\n20\n8\nBlockLayer\n2\nB1\n70\n0\n"
      "0\nendblk\n5\n21\n8\nBlockLayer\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK(tryReadDxf(dxf, cap, "lc_block_case_insensitive.dxf"));
  REQUIRE(cap.m_blocks.size() == 1);
  CHECK(cap.m_blocks.front().name == "B1");
  CHECK(cap.m_endBlockCount == 1);
}

TEST_CASE("DXF BLOCK retains its first raw child entity",
          "[dxf][block][raw]") {
  BlockRawEntityCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n20\n8\nBlockLayer\n2\nB1\n70\n0\n"
      "0\nCUSTOM\n5\n21\n8\nInnerLayer\n1\nfirst\n"
      "0\nENDBLK\n5\n22\n8\nBlockLayer\n"
      "0\nENDSEC\n0\nEOF\n";

  REQUIRE(tryReadDxf(dxf, cap, "lc_block_first_raw_child.dxf"));
  REQUIRE(cap.m_blocks.size() == 1);
  REQUIRE(cap.m_entities.size() == 1);
  CHECK(cap.m_entities.front().name == "CUSTOM");
  REQUIRE(cap.m_entities.front().groups.size() >= 1);
  CHECK(cap.m_entities.front().groups.front().code() == 5);
  CHECK(cap.m_entities.front().groups.front().c_str() == std::string{"21"});
  CHECK(cap.m_endBlockCount == 1);
}

TEST_CASE("DXF binary BLOCK stages children until a valid footer",
          "[dxf][block][binary]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "lc_binary_block_callbacks.dxf";
  std::filesystem::remove(path);

  class Emitter final : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    bool m_blockRecordResult = false;
    bool m_blockResult = false;
    bool m_lineResult = false;

    void writeBlockRecords() override {
      m_blockRecordResult = m_rw->writeBlockRecord("BINARY_BLOCK");
    }

    void writeBlocks() override {
      DRW_Block block;
      block.name = "BINARY_BLOCK";
      m_blockResult = m_rw->writeBlock(&block);

      DRW_Line line;
      line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
      line.secPoint = DRW_Coord(4.0, 3.0, 0.0);
      m_lineResult = m_rw->writeLine(&line);
    }
  } emitter;

  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, true));
  }

  class Capture final : public StubInterface {
  public:
    std::vector<std::string> events;
    bool inTargetBlock = false;

    void addBlock(const DRW_Block &block) override {
      if (block.name == "BINARY_BLOCK") {
        inTargetBlock = true;
        events.push_back("block");
      }
    }

    void addLine(const DRW_Line &) override {
      if (inTargetBlock)
        events.push_back("line");
    }

    void endBlock() override {
      if (inTargetBlock) {
        events.push_back("end");
        inTargetBlock = false;
      }
    }
  } capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, false));
  }
  std::filesystem::remove(path);

  CHECK(emitter.m_blockRecordResult);
  CHECK(emitter.m_blockResult);
  CHECK(emitter.m_lineResult);
  CHECK(capture.events ==
        std::vector<std::string>{"block", "line", "end"});
}

TEST_CASE("DXF malformed BLOCK child closes the published block scope",
          "[dxf][block][malformed]") {
  BlockCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n20\n8\nBlockLayer\n2\nB1\n70\n0\n"
      "0\nLINE\n5\n21\n8\nInnerLayer\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_block_unclosed_scope.dxf"));
  REQUIRE(cap.m_blocks.size() == 1);
  CHECK(cap.m_endBlockCount == 1);
}

TEST_CASE("DXF rejects duplicate BLOCK self handles",
          "[dxf][block][handles][malformed]") {
  BlockCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n20\n8\nBlockLayer\n2\nB1\n70\n0\n"
      "0\nENDBLK\n5\n21\n8\nBlockLayer\n"
      "0\nBLOCK\n5\n20\n8\nBlockLayer\n2\nB2\n70\n0\n"
      "0\nENDBLK\n5\n22\n8\nBlockLayer\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_duplicate_block_handle.dxf"));
  CHECK(cap.m_blocks.size() == 1);
  CHECK(cap.m_blocks.front().name == "B1");
  CHECK(cap.m_endBlockCount == 1);
}

TEST_CASE("DXF rejects duplicate typed entity self handles",
          "[dxf][entities][handles][malformed]") {
  LineCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n30\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nLINE\n5\n30\n8\n0\n10\n2\n20\n2\n11\n3\n21\n3\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_duplicate_entity_handle.dxf"));
  CHECK(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x30u);
}

TEST_CASE("DXF basic entities require mandatory geometry fields",
          "[dxf][entities][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> entities = {
      "0\nPOINT\n10\n1\n",
      "0\nLINE\n10\n0\n20\n0\n11\n1\n",
      "0\nCIRCLE\n10\n0\n20\n0\n",
      "0\nARC\n10\n0\n20\n0\n40\n1\n50\n0\n",
      "0\nELLIPSE\n10\n0\n20\n0\n11\n1\n21\n0\n"};

  for (std::size_t i = 0; i < entities.size(); ++i) {
    LineCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + entities[i] + suffix, cap,
                           ("lc_basic_entity_missing_field_"
                            + std::to_string(i) + ".dxf").c_str()));
    CHECK(cap.m_callCount == 0);
  }
}

TEST_CASE("DXF infinite line entities require mandatory geometry fields",
          "[dxf][entities][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> entities = {
      "0\nRAY\n10\n1\n20\n2\n11\n3\n",
      "0\nXLINE\n10\n1\n20\n2\n21\n4\n"};

  for (std::size_t i = 0; i < entities.size(); ++i) {
    InfiniteLineCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + entities[i] + suffix, cap,
                           ("lc_infinite_line_missing_field_"
                            + std::to_string(i) + ".dxf").c_str()));
    CHECK(cap.m_rayCallCount == 0);
    CHECK(cap.m_xlineCallCount == 0);
  }
}

TEST_CASE("DXF quad entities require three corners and default the fourth",
          "[dxf][entities][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> malformed = {
      "0\nTRACE\n10\n0\n20\n0\n11\n1\n21\n0\n12\n2\n",
      "0\nSOLID\n10\n0\n20\n0\n11\n1\n21\n0\n12\n2\n22\n0\n13\n3\n",
      "0\n3DFACE\n10\n0\n20\n0\n11\n1\n21\n0\n12\n2\n22\n0\n23\n3\n"};

  for (std::size_t i = 0; i < malformed.size(); ++i) {
    QuadEntityCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + malformed[i] + suffix, cap,
                           ("lc_quad_entity_missing_field_"
                            + std::to_string(i) + ".dxf").c_str()));
    CHECK(cap.m_traceCallCount == 0);
    CHECK(cap.m_solidCallCount == 0);
    CHECK(cap.m_faceCallCount == 0);
  }

  QuadEntityCapture triangle;
  const std::string trace =
      "0\nTRACE\n10\n0\n20\n0\n11\n1\n21\n0\n12\n2\n22\n0\n";
  REQUIRE(tryReadDxf(prefix + trace + suffix, triangle,
                     "lc_trace_triangle.dxf"));
  REQUIRE(triangle.m_traceCallCount == 1);
  CHECK(triangle.m_trace.fourPoint.x == Catch::Approx(2.0));
  CHECK(triangle.m_trace.fourPoint.y == Catch::Approx(0.0));
}

TEST_CASE("DXF SHAPE requires its identifying geometry fields",
          "[dxf][entities][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> malformed = {
      "0\nSHAPE\n10\n0\n20\n0\n40\n1\n",
      "0\nSHAPE\n2\nTEST\n20\n0\n40\n1\n",
      "0\nSHAPE\n2\nTEST\n10\n0\n20\n0\n"};

  for (std::size_t i = 0; i < malformed.size(); ++i) {
    ShapeCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + malformed[i] + suffix, cap,
                           ("lc_shape_missing_field_"
                            + std::to_string(i) + ".dxf").c_str()));
    CHECK(cap.m_callCount == 0);
  }
}

TEST_CASE("DXF text entities require text, insertion, and height fields",
          "[dxf][entities][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> malformed = {
      "0\nTEXT\n10\n0\n20\n0\n40\n1\n",
      "0\nMTEXT\n10\n0\n20\n0\n1\nhello\n",
      "0\nRTEXT\n10\n0\n20\n0\n40\n1\n",
      "0\nTEXT\n1\nhello\n10\n0\n40\n1\n"};

  for (std::size_t i = 0; i < malformed.size(); ++i) {
    TextEntityCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + malformed[i] + suffix, cap,
                           ("lc_text_entity_missing_field_"
                            + std::to_string(i) + ".dxf").c_str()));
    CHECK(cap.m_textCallCount == 0);
    CHECK(cap.m_mtextCallCount == 0);
  }
}

TEST_CASE("DXF ATTDEF and ATTRIB publish only complete text children",
          "[dxf][entities][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> malformedAttdefs = {
      "0\nATTDEF\n1\ndef\n10\n0\n20\n0\n40\n0.5\n",
      "0\nATTDEF\n2\nTAG\n10\n0\n20\n0\n40\n0.5\n",
      "0\nATTDEF\n1\ndef\n2\nTAG\n10\n0\n40\n0.5\n"};
  for (std::size_t i = 0; i < malformedAttdefs.size(); ++i) {
    RawEntityCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + malformedAttdefs[i] + suffix, cap,
                           ("lc_attdef_missing_field_"
                            + std::to_string(i) + ".dxf").c_str()));
    CHECK(cap.m_attdefs.empty());
  }

  const std::string insertPrefix =
      prefix + "0\nINSERT\n2\nBLOCK\n10\n0\n20\n0\n66\n1\n";
  const std::string malformedAttrib =
      "0\nATTRIB\n1\nvalue\n10\n1\n20\n1\n40\n1\n"
      "0\nSEQEND\n";
  InsertCapture malformedInsert;
  CHECK_FALSE(tryReadDxf(insertPrefix + malformedAttrib + suffix,
                         malformedInsert, "lc_attrib_missing_tag.dxf"));
  CHECK(malformedInsert.m_captured.empty());

  const std::string validAttrib =
      "0\nATTRIB\n1\nvalue\n2\nTAG\n10\n1\n20\n1\n40\n1\n"
      "0\nSEQEND\n";
  InsertCapture validInsert;
  REQUIRE(tryReadDxf(insertPrefix + validAttrib + suffix, validInsert,
                     "lc_attrib_complete.dxf"));
  REQUIRE(validInsert.m_captured.size() == 1);
  CHECK(validInsert.m_captured.front().attlist.size() == 1);
}

TEST_CASE("DXF INSERT requires a block name and insertion point",
          "[dxf][entities][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";
  const std::vector<std::string> malformed = {
      "0\nINSERT\n10\n0\n20\n0\n",
      "0\nMINSERT\n2\nBLOCK\n10\n0\n"};

  for (std::size_t i = 0; i < malformed.size(); ++i) {
    InsertCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + malformed[i] + suffix, cap,
                           ("lc_insert_missing_field_"
                            + std::to_string(i) + ".dxf").c_str()));
    CHECK(cap.m_captured.empty());
  }
}

TEST_CASE("DXF MINSERT rejects out-of-range array counts",
          "[dxf][insert][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nMINSERT\n8\n0\n2\nGRID\n10\n0\n20\n0\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  for (const auto &field : {std::string{"70\n-1\n"},
                            std::string{"71\n32768\n"}}) {
    InsertCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + field + suffix, cap,
                           "lc_minsert_invalid_count.dxf"));
    CHECK(cap.m_captured.empty());
  }
}

TEST_CASE("DXF ACAD_TABLE rejects out-of-range grid counts",
          "[dxf][table][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nACAD_TABLE\n100\nAcDbBlockReference\n"
      "2\nGRID\n10\n0\n20\n0\n30\n0\n"
      "100\nAcDbTable\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  for (const auto &field : {std::string{"91\n-1\n"},
                            std::string{"92\n1001\n"}}) {
    TableCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + field + suffix, cap,
                           "lc_acad_table_invalid_grid.dxf"));
    CHECK(cap.m_callCount == 0);
  }
}

TEST_CASE("DXF MESH rejects non-finite vertex coordinates",
          "[dxf][mesh][malformed]") {
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nMESH\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbSubDMesh\n"
      "71\n2\n72\n0\n91\n0\n92\n1\n"
      "10\nnan\n20\n0\n30\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  StubInterface capture;
  CHECK_FALSE(tryReadDxf(dxf, capture, "lc_mesh_nonfinite_vertex.dxf"));
}

TEST_CASE("DXF MESH writer rejects invalid geometry before writing",
          "[dxf][mesh][writer][safety]") {
  class Emitter : public StubInterface {
  public:
    DRW_Mesh mesh;
    dxfRW *writer = nullptr;
    bool result = true;

    void writeEntities() override { result = writer->writeMesh(&mesh); }
  };

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_mesh_invalid_geometry.dxf";
  const auto write = [&](DRW_Mesh mesh) {
    std::filesystem::remove(path);
    Emitter emitter;
    emitter.mesh = std::move(mesh);
    dxfRW writer(path.string().c_str());
    emitter.writer = &writer;
    const bool result = writer.write(&emitter, DRW::AC1021, false);
    std::ifstream output(path);
    const std::string content{std::istreambuf_iterator<char>(output),
                              std::istreambuf_iterator<char>()};
    std::filesystem::remove(path);
    CHECK_FALSE(result);
    CHECK_FALSE(emitter.result);
    CHECK(content.find("\nMESH\n") == std::string::npos);
  };

  DRW_Mesh nonFinite;
  nonFinite.vertices = {DRW_Coord(std::numeric_limits<double>::quiet_NaN(),
                                  0.0, 0.0)};
  write(std::move(nonFinite));

  DRW_Mesh badFace;
  badFace.vertices = {DRW_Coord(0.0, 0.0, 0.0),
                      DRW_Coord(1.0, 0.0, 0.0),
                      DRW_Coord(0.0, 1.0, 0.0)};
  badFace.faces = {{0, 1, 3}};
  write(std::move(badFace));

  DRW_Mesh badEdge;
  badEdge.vertices = {DRW_Coord(0.0, 0.0, 0.0)};
  badEdge.edges = {{0, 1}};
  write(std::move(badEdge));
}

TEST_CASE("DXF preserves the full 32-bit entity handle width",
          "[dxf][handles]") {
  LineCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\nFFFFFFFF\n330\n80000000\n"
      "8\nHANDLE_LAYER\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDSEC\n0\nEOF\n";

  readDxf(dxf, cap, "lc_high_bit_entity_handles.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0xFFFFFFFFu);
  CHECK(cap.m_captured.parentHandle == 0x80000000u);
}

TEST_CASE("DXF rejects unchecked LWPOLYLINE and SPLINE counts",
          "[dxf][polyline][spline][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  SECTION("negative LWPOLYLINE vertex count") {
    LWPolylineCapture capture;
    const std::string dxf = prefix +
        "0\nLWPOLYLINE\n8\n0\n90\n-1\n" + suffix;
    CHECK_FALSE(tryReadDxf(dxf, capture, "lc_lwpolyline_negative_count.dxf"));
    CHECK(capture.m_callCount == 0);
  }

  SECTION("extra LWPOLYLINE vertex") {
    LWPolylineCapture capture;
    const std::string dxf = prefix +
        "0\nLWPOLYLINE\n8\n0\n90\n1\n"
        "10\n0\n20\n0\n10\n1\n20\n1\n" + suffix;
    CHECK_FALSE(tryReadDxf(dxf, capture, "lc_lwpolyline_extra_vertex.dxf"));
    CHECK(capture.m_callCount == 0);
  }

  SECTION("oversized SPLINE knot count") {
    SplineCapture capture;
    const std::string dxf = prefix +
        "0\nSPLINE\n8\n0\n71\n1\n72\n1000001\n73\n0\n74\n0\n" + suffix;
    CHECK_FALSE(tryReadDxf(dxf, capture, "lc_spline_oversized_knot_count.dxf"));
    CHECK(capture.m_callCount == 0);
  }

  SECTION("extra SPLINE control point") {
    SplineCapture capture;
    const std::string dxf = prefix +
        "0\nSPLINE\n8\n0\n71\n1\n72\n0\n73\n1\n74\n0\n"
        "10\n0\n20\n0\n30\n0\n10\n1\n20\n1\n30\n0\n" + suffix;
    CHECK_FALSE(tryReadDxf(dxf, capture, "lc_spline_extra_control_point.dxf"));
    CHECK(capture.m_callCount == 0);
  }
}

TEST_CASE("DXF rejects invalid LWPOLYLINE payload fields",
          "[dxf][polyline][malformed]") {
  const std::string prefix =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLWPOLYLINE\n8\n0\n90\n1\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  SECTION("non-finite vertex coordinate") {
    LWPolylineCapture capture;
    CHECK_FALSE(tryReadDxf(prefix + "10\nnan\n20\n0\n" + suffix,
                           capture, "lc_lwpolyline_nonfinite_vertex.dxf"));
    CHECK(capture.m_callCount == 0);
  }

  SECTION("unsupported group-70 flag") {
    LWPolylineCapture capture;
    CHECK_FALSE(tryReadDxf(
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLWPOLYLINE\n8\n0\n90\n0\n70\n256\n"
        "0\nENDSEC\n0\nEOF\n",
        capture, "lc_lwpolyline_invalid_flags.dxf"));
    CHECK(capture.m_callCount == 0);
  }
}

TEST_CASE("DXF BLOCK handle requirement follows source version",
          "[dxf][block][version]") {
  SECTION("post-R12 requires a self handle") {
    BlockCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
        "0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n8\nBlockLayer\n2\nB1\n70\n0\n"
        "0\nENDBLK\n8\nBlockLayer\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_modern_block_missing_handle.dxf"));
    CHECK(cap.m_blocks.empty());
  }

  SECTION("R12 permits a handle-less block") {
    BlockCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n"
        "0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n8\nBlockLayer\n2\nB1\n70\n0\n"
        "0\nENDBLK\n8\nBlockLayer\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK(tryReadDxf(dxf, cap, "lc_r12_block_without_handle.dxf"));
    REQUIRE(cap.m_blocks.size() == 1);
    CHECK(cap.m_blocks.front().name == "B1");
  }
}

TEST_CASE("DXF ENDBLK validates modern footer identity",
          "[dxf][block][malformed]") {
  const auto makeDxf = [](const std::string& footer) {
    return std::string{
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
        "0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n5\nA0\n330\nA1\n2\nB1\n70\n0\n"}
        + footer + "0\nENDSEC\n0\nEOF\n";
  };

  SECTION("rejects a malformed footer handle") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(makeDxf("0\nENDBLK\n5\nNOT_HEX\n330\nA1\n"),
                           capture, "lc_endblk_bad_handle.dxf"));
    CHECK(capture.m_blocks.size() == 1);
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("requires an owner when BLOCK declares one") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(makeDxf("0\nENDBLK\n5\nA2\n"), capture,
                           "lc_endblk_missing_owner.dxf"));
    CHECK(capture.m_blocks.size() == 1);
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("rejects an owner that differs from BLOCK") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("0\nENDBLK\n5\nA2\n330\nA9\n"), capture,
        "lc_endblk_owner_mismatch.dxf"));
    CHECK(capture.m_blocks.size() == 1);
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("rejects an undocumented footer code") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("0\nENDBLK\n5\nA2\n330\nA1\n70\n0\n"), capture,
        "lc_endblk_unknown_code.dxf"));
    CHECK(capture.m_blocks.size() == 1);
    CHECK(capture.m_endBlockCount == 1);
    CHECK(capture.m_lineCount == 0);
  }

  SECTION("accepts a balanced application group") {
    BlockCapture capture;
    CHECK(tryReadDxf(
        makeDxf("0\nENDBLK\n5\nA2\n330\nA1\n100\nAcDbEntity\n"
                "8\nBlockLayer\n102\n{CUSTOM\n1\npayload\n102\n}\n"
                "100\nAcDbBlockEnd\n"),
        capture, "lc_endblk_application_group.dxf"));
    CHECK(capture.m_blocks.size() == 1);
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("rejects an unbalanced application group") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("0\nENDBLK\n5\nA2\n330\nA1\n102\n{CUSTOM\n1\n"
                "payload\n"),
        capture, "lc_endblk_unbalanced_application_group.dxf"));
    CHECK(capture.m_blocks.size() == 1);
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("preserves callback order for a valid child") {
    BlockCapture capture;
    const std::string modernFooter =
        "0\nENDBLK\n5\nA2\n330\nA1\n100\nAcDbEntity\n"
        "8\nBlockLayer\n100\nAcDbBlockEnd\n";
    std::string dxf = makeDxf(modernFooter);
    const std::string child =
        "0\nLINE\n5\nA3\n8\nBlockLayer\n10\n0\n20\n0\n"
        "11\n1\n21\n1\n";
    const std::size_t footerOffset = dxf.find(modernFooter);
    REQUIRE(footerOffset != std::string::npos);
    dxf.insert(footerOffset, child);
    CHECK(tryReadDxf(
        dxf,
        capture, "lc_endblk_callback_order.dxf"));
    CHECK(capture.m_events ==
          std::vector<std::string>{"block", "line", "end"});
  }
}

TEST_CASE("DXF malformed compound BLOCK children publish no child callbacks",
          "[dxf][block][children][malformed]") {
  const auto makeDxf = [](const std::string &children) {
    return std::string{
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
        "0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
        "0\nBLOCK\n5\n20\n8\nBlockLayer\n2\nB1\n70\n0\n"}
        + children
        + "0\nENDBLK\n5\n22\n8\nBlockLayer\n"
          "0\nENDSEC\n0\nEOF\n";
  };

  SECTION("incomplete LINE") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("0\nLINE\n5\n21\n8\nInnerLayer\n10\n0\n20\n0\n"),
        capture, "lc_block_child_line_missing_endpoint.dxf"));
    CHECK(capture.m_lineCount == 0);
    CHECK(capture.m_inserts.empty());
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("incomplete ATTRIB sequence") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("0\nINSERT\n2\nB1\n10\n0\n20\n0\n66\n1\n"
                "0\nATTRIB\n1\nvalue\n10\n1\n20\n1\n40\n1\n"
                "0\nSEQEND\n"),
        capture, "lc_block_child_attrib_missing_tag.dxf"));
    CHECK(capture.m_inserts.empty());
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("incomplete VERTEX sequence") {
    BlockCapture capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("0\nPOLYLINE\n8\n0\n70\n0\n"
                "0\nVERTEX\n8\n0\n20\n2\n0\nSEQEND\n"),
        capture, "lc_block_child_vertex_missing_x.dxf"));
    CHECK(capture.m_polylineCount == 0);
    CHECK(capture.m_endBlockCount == 1);
  }

  SECTION("valid child is discarded when the footer is malformed") {
    BlockCapture capture;
    const std::string dxf = makeDxf(
        "0\nLINE\n5\n21\n8\nInnerLayer\n10\n0\n20\n0\n11\n1\n21\n1\n"
        "0\nENDBLK\n5\n22\n8\nBlockLayer\n70\n0\n");
    CHECK_FALSE(tryReadDxf(dxf, capture,
                           "lc_block_child_bad_footer_no_publish.dxf"));
    CHECK(capture.m_lineCount == 0);
    CHECK(capture.m_events == std::vector<std::string>{"block", "end"});
    CHECK(capture.m_endBlockCount == 1);
  }
}

TEST_CASE("DXF BLOCK_RECORD preview chunks reach the BLOCK callback",
          "[dxf][blockrecord][preview]") {
  BlockCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nBLOCK_RECORD\n70\n1\n"
      "0\nBLOCK_RECORD\n5\nA1\n330\n1\n"
      "100\nAcDbSymbolTableRecord\n100\nAcDbBlockTableRecord\n"
      "2\nPREVIEW_BLOCK\n70\n7\n280\n1\n281\n0\n"
      "310\n414243\n310\nDE\n"
      "0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\nA2\n330\nA1\n100\nAcDbEntity\n8\n0\n"
      "100\nAcDbBlockBegin\n2\nPREVIEW_BLOCK\n70\n0\n"
      "10\n0.0\n20\n0.0\n3\nPREVIEW_BLOCK\n1\n\n"
      "0\nENDBLK\n5\nA3\n330\nA1\n100\nAcDbEntity\n8\n0\n"
      "100\nAcDbBlockEnd\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_block_record_preview.dxf", false);

  const auto it = std::find_if(
      capture.m_blocks.begin(), capture.m_blocks.end(),
      [](const DRW_Block &block) { return block.name == "PREVIEW_BLOCK"; });
  REQUIRE(it != capture.m_blocks.end());
  CHECK(it->insUnits == 7);
  CHECK(it->previewData == std::vector<std::uint8_t>{0x41, 0x42, 0x43, 0xDE});
}

TEST_CASE("DXF BLOCK_RECORD preserves BLKREFS INSERT handles",
          "[dxf][blockrecord][blkrefs]") {
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nBLOCK_RECORD\n70\n1\n"
      "0\nBLOCK_RECORD\n5\nA1\n2\nBLKREFS_BLOCK\n"
      "340\nC2\n280\n0\n281\n1\n"
      "102\n{ACAD_XDICTIONARY\n360\nC1\n102\n}\n"
      "102\n{BLKREFS\n331\nB1\n331\nB2\n102\n}\n"
      "0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\nA2\n330\nA1\n100\nAcDbEntity\n8\n0\n"
      "100\nAcDbBlockBegin\n2\nBLKREFS_BLOCK\n70\n0\n"
      "0\nENDBLK\n5\nA3\n330\nA1\n100\nAcDbEntity\n8\n0\n"
      "100\nAcDbBlockEnd\n"
      "0\nENDSEC\n0\nEOF\n";

  BlockCapture capture;
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_block_record_blkrefs.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  const auto *context = reader.getReadingContext();
  REQUIRE(context != nullptr);
  const auto it = context->blockRecordMap.find(0xA1u);
  REQUIRE(it != context->blockRecordMap.end());
  CHECK(it->second.insertHandles ==
        std::vector<std::uint32_t>{0xB1u, 0xB2u});
  CHECK(context->resolveBlockRecordInsertHandles(0xA1u) ==
        std::vector<std::uint32_t>{0xB1u, 0xB2u});
  CHECK(it->second.layoutHandle == 0xC2u);
  CHECK_FALSE(it->second.canExplode);
  CHECK(it->second.blockScaling == 1u);
  CHECK(context->resolveBlockRecordLayoutHandle(0xA1u) == 0xC2u);
  std::filesystem::remove(path);
}

TEST_CASE("DXF BLOCK_RECORD rejects malformed BLKREFS",
          "[dxf][blockrecord][blkrefs][malformed]") {
  const auto makeDxf = [](const std::string &body) {
    return std::string(
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
        "0\nENDSEC\n0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nBLOCK_RECORD\n70\n1\n"
        "0\nBLOCK_RECORD\n5\nA1\n2\nBROKEN\n")
        + body +
        "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  };

  SECTION("missing close") {
    StubInterface capture;
    CHECK_FALSE(tryReadDxf(makeDxf("102\n{BLKREFS\n331\nB1\n"),
                           capture, "lc_block_record_blkrefs_unclosed.dxf"));
  }
  SECTION("invalid handle") {
    StubInterface capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("102\n{BLKREFS\n331\nnot-a-handle\n102\n}\n"),
        capture, "lc_block_record_blkrefs_bad_handle.dxf"));
  }
  SECTION("duplicate handle") {
    StubInterface capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("102\n{BLKREFS\n331\nB1\n331\nB1\n102\n}\n"),
        capture, "lc_block_record_blkrefs_duplicate.dxf"));
  }
  SECTION("null handle") {
    StubInterface capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("102\n{BLKREFS\n331\n0\n102\n}\n"),
        capture, "lc_block_record_blkrefs_null.dxf"));
  }
  SECTION("unexpected control value") {
    StubInterface capture;
    CHECK_FALSE(tryReadDxf(
        makeDxf("102\n{BLKREFS\n102\n{NESTED\n102\n}\n"),
        capture, "lc_block_record_blkrefs_nested.dxf"));
  }
  SECTION("legacy version") {
    StubInterface capture;
    const std::string dxf =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1014\n"
        "0\nENDSEC\n0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nBLOCK_RECORD\n70\n1\n"
        "0\nBLOCK_RECORD\n5\nA1\n2\nLEGACY\n"
        "102\n{BLKREFS\n331\nB1\n102\n}\n"
        "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, capture,
                           "lc_block_record_blkrefs_legacy.dxf"));
  }
  SECTION("invalid block flags") {
    StubInterface capture;
    CHECK_FALSE(tryReadDxf(makeDxf("280\n2\n"), capture,
                           "lc_block_record_bad_explode.dxf"));
    CHECK_FALSE(tryReadDxf(makeDxf("281\n2\n"), capture,
                           "lc_block_record_bad_scaling.dxf"));
  }
}

TEST_CASE("DXF BLOCK_RECORD rejects malformed preview chunks",
          "[dxf][blockrecord][preview][malformed]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_block_record_preview_bad.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nTABLES\n"
           "0\nTABLE\n2\nBLOCK_RECORD\n70\n1\n"
           "0\nBLOCK_RECORD\n5\nA1\n2\nBAD_PREVIEW\n310\nABC\n"
           "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  }

  BlockCapture capture;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, false));
  CHECK(capture.m_blocks.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF BLOCK_RECORD rejects malformed handles",
          "[dxf][blockrecord][handles][malformed]") {
  const char *dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nBLOCK_RECORD\n70\n1\n"
      "0\nBLOCK_RECORD\n5\nnot-a-handle\n2\nBROKEN\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  BlockCapture capture;
  CHECK_FALSE(tryReadDxf(dxf, capture, "lc_block_record_bad_handle.dxf"));
  CHECK(capture.m_blocks.empty());
}

TEST_CASE("DXF BLOCK_RECORD preview chunks are writable and readable",
          "[dxf][blockrecord][preview][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeBlockRecords() override {
      REQUIRE(m_writer->writeBlockRecord(
          "PREVIEW_BLOCK", 7,
          std::vector<std::uint8_t>{0x41, 0x42, 0x43, 0xDE}));
    }

    void writeBlocks() override {
      DRW_Block block;
      block.name = "PREVIEW_BLOCK";
      block.basePoint = DRW_Coord(0.0, 0.0, 0.0);
      REQUIRE(m_writer->writeBlock(&block));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_block_record_preview_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  BlockCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  const auto it = std::find_if(
      capture.m_blocks.begin(), capture.m_blocks.end(),
      [](const DRW_Block &block) { return block.name == "PREVIEW_BLOCK"; });
  REQUIRE(it != capture.m_blocks.end());
  CHECK(it->insUnits == 7);
  CHECK(it->previewData == std::vector<std::uint8_t>{0x41, 0x42, 0x43, 0xDE});
  std::filesystem::remove(path);
}

TEST_CASE("DXF BLOCK_RECORD writes planned INSERT BLKREFS handles",
          "[dxf][blockrecord][blkrefs][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;
    std::uint32_t m_insertHandle = 0;
    std::uint32_t m_blockRecordHandle = 0;

    void writeBlockRecords() override {
      m_insertHandle = m_writer->preallocateEntityHandle(0x100u);
      REQUIRE(m_insertHandle != 0);
      REQUIRE(m_writer->writeBlockRecord(
          "REF_BLOCK", 0, {}, {m_insertHandle}));
      m_blockRecordHandle = static_cast<std::uint32_t>(
          m_writer->getBlockRecordHandleToWrite("REF_BLOCK"));
      REQUIRE(m_blockRecordHandle != 0);
    }

    void writeBlocks() override {
      DRW_Block block;
      block.name = "REF_BLOCK";
      REQUIRE(m_writer->writeBlock(&block));
    }

    void writeEntities() override {
      DRW_Insert insert;
      insert.handle = 0x100u;
      insert.name = "REF_BLOCK";
      REQUIRE(m_writer->writeInsert(&insert));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_block_record_blkrefs_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  BlockCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  const auto *context = reader.getReadingContext();
  REQUIRE(context != nullptr);
  const auto record = context->blockRecordMap.find(emitter.m_blockRecordHandle);
  REQUIRE(record != context->blockRecordMap.end());
  CHECK(record->second.insertHandles
        == std::vector<std::uint32_t>{emitter.m_insertHandle});
  REQUIRE(capture.m_inserts.size() == 1u);
  CHECK(capture.m_inserts.front().handle == emitter.m_insertHandle);
  std::filesystem::remove(path);
}

TEST_CASE("DXF VPORT preserves modern viewport fields",
          "[dxf][tables][vport]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeVports() override {
      DRW_Vport active;
      REQUIRE(m_writer->writeVport(&active));

      DRW_Vport vport;
      vport.name = "MODERN_VPORT";
      vport.handle = 0x101;
      vport.renderMode = 3;
      vport.ucsPerVP = true;
      vport.ucsOrigin = DRW_Coord(1.0, 2.0, 3.0);
      vport.ucsXAxis = DRW_Coord(0.0, 1.0, 0.0);
      vport.ucsYAxis = DRW_Coord(-1.0, 0.0, 0.0);
      vport.ucsElevation = 4.0;
      vport.ucsOrthoType = 2;
      vport.gridBehavior = 5;
      vport.gridMajorLines = 9;
      vport.useDefaultLighting = false;
      vport.defaultLightingType = 2;
      vport.brightness = 0.25;
      vport.contrast = 0.75;
      vport.ambientColor = 7;
      vport.ambientColorRgb = 0x123456;
      vport.ambientColorName = "AmbientName";
      vport.backgroundHandle = 0x102;
      vport.visualStyleHandle = 0x103;
      vport.m_sunHandle = 0x104;
      vport.namedUcsHandle = 0x105;
      vport.baseUcsHandle = 0x106;
      REQUIRE(m_writer->writeVport(&vport));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_modern_vport_roundtrip.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  VportCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount >= 2);
  const auto it = std::find_if(
      capture.m_captured.begin(), capture.m_captured.end(),
      [](const DRW_Vport &vport) { return vport.name == "MODERN_VPORT"; });
  REQUIRE(it != capture.m_captured.end());
  const DRW_Vport &vport = *it;
  CHECK(vport.renderMode == 3u);
  CHECK(vport.ucsPerVP);
  CHECK(vport.ucsOrigin.x == 1.0);
  CHECK(vport.ucsOrigin.y == 2.0);
  CHECK(vport.ucsOrigin.z == 3.0);
  CHECK(vport.ucsXAxis.y == 1.0);
  CHECK(vport.ucsYAxis.x == -1.0);
  CHECK(vport.ucsElevation == 4.0);
  CHECK(vport.ucsOrthoType == 2);
  CHECK(vport.gridBehavior == 5);
  CHECK(vport.gridMajorLines == 9);
  CHECK_FALSE(vport.useDefaultLighting);
  CHECK(vport.defaultLightingType == 2u);
  CHECK(vport.brightness == 0.25);
  CHECK(vport.contrast == 0.75);
  CHECK(vport.ambientColor == 7u);
  CHECK(vport.ambientColorRgb == 0x123456);
  CHECK(vport.ambientColorName == "AmbientName");
  CHECK(vport.backgroundHandle == 0x102u);
  CHECK(vport.visualStyleHandle == 0x103u);
  CHECK(vport.m_sunHandle == 0x104u);
  CHECK(vport.namedUcsHandle == 0x105u);
  CHECK(vport.baseUcsHandle == 0x106u);
}

TEST_CASE("DXF VPORT rejects out-of-range byte fields",
          "[dxf][tables][vport][numeric]") {
  VportCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nVPORT\n70\n1\n"
      "0\nVPORT\n2\nBAD_VPORT\n5\n20\n282\n256\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_vport_out_of_range_byte.dxf"));
  CHECK(cap.m_captured.empty());
}

TEST_CASE("DXF VPORT rejects non-finite payloads transactionally",
          "[dxf][tables][vport][safety]") {
  VportCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nVPORT\n70\n1\n"
      "0\nVPORT\n2\nBAD_VPORT\n5\n20\n10\nnan\n20\n0\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_vport_non_finite.dxf"));
  CHECK(cap.m_captured.empty());

  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;
    bool m_writeResult = true;

    void writeVports() override {
      DRW_Vport invalid;
      invalid.name = "INVALID_VPORT";
      invalid.renderMode = 256;
      m_writeResult = m_writer->writeVport(&invalid);
      CHECK(invalid.name == "INVALID_VPORT");
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_vport_invalid_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  std::filesystem::remove(path);
}

TEST_CASE("DXF VPORT and VIEW reject negative render modes",
          "[dxf][tables][numeric][malformed]") {
  const std::vector<std::string> malformed = {
      "VPORT\n70\n1\n0\nVPORT\n2\nBAD_VPORT\n5\n20\n281\n-1\n",
      "VIEW\n70\n1\n0\nVIEW\n2\nBAD_VIEW\n5\n20\n281\n-1\n"};
  for (std::size_t i = 0; i < malformed.size(); ++i) {
    const std::string dxf =
        "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\n" + malformed[i]
        + "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
    StubInterface capture;
    CHECK_FALSE(tryReadDxf(
        dxf, capture,
        ("lc_negative_render_mode_" + std::to_string(i) + ".dxf").c_str()));
  }
}

TEST_CASE("DXF table-entry application groups preserve common references",
          "[dxf][tables][application-groups]") {
  LayerCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLAYER\n5\n2\n100\nAcDbSymbolTable\n70\n1\n"
      "0\nLAYER\n5\nA\n330\n0\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbLayerTableRecord\n2\nGROUP_LAYER\n70\n0\n62\n7\n6\nCONTINUOUS\n"
      "102\n{ACAD_REACTORS\n330\nB\n330\nC\n102\n}\n"
      "102\n{ACAD_XDICTIONARY\n360\nD\n102\n}\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  readDxf(dxf, cap, "lc_table_application_groups.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "GROUP_LAYER");
  CHECK(cap.m_captured.reactorHandles ==
        std::vector<std::uint32_t>{0xB, 0xC});
  CHECK(cap.m_captured.xDictHandle == 0xDu);
}

TEST_CASE("DXF table-entry application groups round trip typed records",
          "[dxf][tables][application-groups][roundtrip]") {
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLAYER\n5\n2\n100\nAcDbSymbolTable\n70\n1\n"
      "0\nLAYER\n5\nA\n330\n2\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbLayerTableRecord\n2\nGROUP_LAYER\n70\n0\n62\n7\n"
      "6\nCONTINUOUS\n"
      "102\n{CUSTOM_TABLE_APP\n1\npayload\n90\n42\n310\nA1B2\n"
      "102\n{NESTED\n481\nDE\n102\n}\n102\n}\n"
      "102\n{ACAD_REACTORS\n330\nB\n330\nC\n102\n}\n"
      "102\n{ACAD_XDICTIONARY\n360\nD\n102\n}\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";

  LayerCapture source;
  readDxf(dxf, source, "lc_table_app_data_source.dxf");
  REQUIRE(source.m_layers.size() == 1);
  const DRW_Layer &parsed = source.m_layers.front();
  REQUIRE(parsed.appData.size() == 3);
  CHECK(parsed.reactorHandles == std::vector<std::uint32_t>{0xB, 0xC});
  CHECK(parsed.xDictHandle == 0xDu);
  REQUIRE(parsed.appData.front().front().code() == 102);
  CHECK(std::string(parsed.appData.front().front().c_str()) ==
        "CUSTOM_TABLE_APP");

  bool hasNestedHandle = false;
  for (const DRW_Variant &value : parsed.appData.front()) {
    if (value.code() == 481 && value.c_str() != nullptr
        && std::string(value.c_str()) == "DE") {
      hasNestedHandle = true;
    }
  }
  CHECK(hasNestedHandle);

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_table_app_data_roundtrip.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    LayerEmitter emitter;
    emitter.m_writer = &writer;
    emitter.m_layer = parsed;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    REQUIRE(emitter.m_writeResult);
  }

  LayerCapture roundTrip;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&roundTrip, /*ext=*/true));
  }
  const auto found = std::find_if(
      roundTrip.m_layers.cbegin(), roundTrip.m_layers.cend(),
      [](const DRW_Layer &layer) { return layer.name == "GROUP_LAYER"; });
  REQUIRE(found != roundTrip.m_layers.cend());
  CHECK(found->appData.size() == 3);
  CHECK(found->reactorHandles == std::vector<std::uint32_t>{0xB, 0xC});
  CHECK(found->xDictHandle == 0xDu);
  std::filesystem::remove(path);

  const auto typedPath = std::filesystem::temp_directory_path() /
                         "lc_table_typed_refs.dxf";
  std::filesystem::remove(typedPath);
  {
    dxfRW writer(typedPath.string().c_str());
    LayerEmitter emitter;
    emitter.m_writer = &writer;
    emitter.m_layer.name = "TYPED_REFS";
    emitter.m_layer.reactorHandles = {0xE};
    emitter.m_layer.xDictHandle = 0xFu;
    auto &nested = emitter.m_layer.appData.emplace_back();
    nested.emplace_back(102, std::string{"CUSTOM_TABLE_APP"});
    nested.emplace_back(102, std::string{"{ACAD_REACTORS"});
    nested.emplace_back(330, std::string{"99"});
    nested.emplace_back(102, std::string{"}"});
    nested.emplace_back(102, std::string{"}"});
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    REQUIRE(emitter.m_writeResult);
  }
  LayerCapture typedOutput;
  {
    dxfRW reader(typedPath.string().c_str());
    REQUIRE(reader.read(&typedOutput, /*ext=*/true));
  }
  const auto typed = std::find_if(
      typedOutput.m_layers.cbegin(), typedOutput.m_layers.cend(),
      [](const DRW_Layer &layer) { return layer.name == "TYPED_REFS"; });
  REQUIRE(typed != typedOutput.m_layers.cend());
  CHECK(typed->appData.size() == 3);
  CHECK(typed->reactorHandles == std::vector<std::uint32_t>{0xE});
  CHECK(typed->xDictHandle == 0xFu);
  std::filesystem::remove(typedPath);

  const auto legacyPath = std::filesystem::temp_directory_path() /
                          "lc_table_app_data_r12.dxf";
  std::filesystem::remove(legacyPath);
  {
    dxfRW writer(legacyPath.string().c_str());
    LayerEmitter emitter;
    emitter.m_writer = &writer;
    emitter.m_layer.name = "LEGACY_APP_DATA";
    auto &group = emitter.m_layer.appData.emplace_back();
    group.emplace_back(102, std::string{"CUSTOM_TABLE_APP"});
    group.emplace_back(102, std::string{"}"});
    CHECK_FALSE(writer.write(&emitter, DRW::AC1009, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  std::filesystem::remove(legacyPath);
}

TEST_CASE("DXF common table writers emit application groups",
          "[dxf][tables][application-groups][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;
    bool m_writeResult = true;

    static void addAppData(DRW_TableEntry &entry) {
      auto &group = entry.appData.emplace_back();
      group.emplace_back(102, std::string{"WRITER_APP"});
      group.emplace_back(90, std::int32_t{17});
      group.emplace_back(102, std::string{"}"});
    }

    void writeLTypes() override {
      DRW_LType entry;
      entry.name = "APP_LTYPE";
      entry.desc = "application-data linetype";
      addAppData(entry);
      m_writeResult = m_writer->writeLineType(&entry) && m_writeResult;
    }

    void writeLayers() override {
      DRW_Layer entry;
      entry.name = "APP_LAYER";
      addAppData(entry);
      m_writeResult = m_writer->writeLayer(&entry) && m_writeResult;
    }

    void writeTextstyles() override {
      DRW_Textstyle entry;
      entry.name = "APP_STYLE";
      addAppData(entry);
      m_writeResult = m_writer->writeTextstyle(&entry) && m_writeResult;
    }

    void writeVports() override {
      DRW_Vport entry;
      entry.name = "APP_VPORT";
      addAppData(entry);
      m_writeResult = m_writer->writeVport(&entry) && m_writeResult;
    }

    void writeDimstyles() override {
      DRW_Dimstyle entry;
      entry.name = "APP_DIMSTYLE";
      addAppData(entry);
      m_writeResult = m_writer->writeDimstyle(&entry) && m_writeResult;
    }

    void writeViews() override {
      DRW_View entry;
      entry.name = "APP_VIEW";
      addAppData(entry);
      m_writeResult = m_writer->writeView(&entry) && m_writeResult;
    }

    void writeUCSs() override {
      DRW_UCS entry;
      entry.name = "APP_UCS";
      addAppData(entry);
      m_writeResult = m_writer->writeUCS(&entry) && m_writeResult;
    }

    void writeAppId() override {
      DRW_AppId entry;
      entry.name = "APP_APPID";
      addAppData(entry);
      m_writeResult = m_writer->writeAppId(&entry) && m_writeResult;
    }
  };

  class Capture : public StubInterface {
  public:
    int m_appDataRecords = 0;

    void record(const DRW_TableEntry &entry) {
      if (entry.appData.size() == 1)
        ++m_appDataRecords;
    }

    void addLType(const DRW_LType &entry) override { record(entry); }
    void addLayer(const DRW_Layer &entry) override { record(entry); }
    void addTextStyle(const DRW_Textstyle &entry) override { record(entry); }
    void addVport(const DRW_Vport &entry) override { record(entry); }
    void addDimStyle(const DRW_Dimstyle &entry) override { record(entry); }
    void addView(const DRW_View &entry) override { record(entry); }
    void addUCS(const DRW_UCS &entry) override { record(entry); }
    void addAppId(const DRW_AppId &entry) override { record(entry); }
  };

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_table_app_data_writers.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    Emitter emitter;
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    REQUIRE(emitter.m_writeResult);
  }
  Capture capture;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, /*ext=*/true));
  }
  CHECK(capture.m_appDataRecords == 8);
  std::filesystem::remove(path);
}

TEST_CASE("DXF CLASSES rejects incomplete records before callbacks",
          "[dxf][classes][malformed]") {
  class ClassCapture : public StubInterface {
  public:
    std::vector<DRW_Class> m_classes;

    void addDxfClass(const DRW_Class &entry) override {
      m_classes.push_back(entry);
    }
  };

  const auto document = [](const std::string &classBody,
                           const char *version = "AC1021") {
    return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\n"
           + std::string(version)
           + "\n0\nENDSEC\n0\nSECTION\n2\nCLASSES\n0\nCLASS\n"
           + classBody + "0\nENDSEC\n0\nEOF\n";
  };
  const std::string requiredR2007 =
      "1\nCUSTOM\n2\nAcDbCustom\n3\nCUSTOM_APP\n"
      "90\n4095\n91\n1\n280\n0\n281\n1\n";

  const std::array<std::string, 7> fields = {
      "1\nCUSTOM\n", "2\nAcDbCustom\n", "3\nCUSTOM_APP\n",
      "90\n4095\n", "91\n1\n", "280\n0\n", "281\n1\n"};
  for (std::size_t omitted = 0; omitted < fields.size(); ++omitted) {
    std::string incomplete;
    for (std::size_t index = 0; index < fields.size(); ++index) {
      if (index != omitted)
        incomplete += fields[index];
    }
    ClassCapture capture;
    CHECK_FALSE(tryReadDxf(document(incomplete), capture,
                           "lc_incomplete_dxf_class.dxf"));
    CHECK(capture.m_classes.empty());
  }

  ClassCapture r2000Capture;
  CHECK(tryReadDxf(document(
                        "1\nCUSTOM\n2\nAcDbCustom\n3\nCUSTOM_APP\n"
                        "90\n4095\n280\n0\n281\n1\n",
                        "AC1015"),
                    r2000Capture, "lc_r2000_dxf_class.dxf"));
  REQUIRE(r2000Capture.m_classes.size() == 1);
  CHECK(r2000Capture.m_classes.front().recName == "CUSTOM");

  ClassCapture r2007Capture;
  CHECK(tryReadDxf(document(requiredR2007), r2007Capture,
                    "lc_r2007_dxf_class.dxf"));
  REQUIRE(r2007Capture.m_classes.size() == 1);
  CHECK(r2007Capture.m_classes.front().instanceCount == 1);
}

TEST_CASE("DXF TABLES propagates malformed table-entry errors",
          "[dxf][tables][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_malformed_layer_table.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nTABLES\n"
           "0\nTABLE\n2\nLAYER\n"
           "0\nLAYER\n5\nA\n2\nBROKEN\n"
           "102\n{ACAD_REACTORS\n0\nENDTAB\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  StubInterface cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  std::filesystem::remove(path);
}

TEST_CASE("DXF rejects an invalid ACADVER before header publication",
          "[dxf][header][malformed]") {
  class HeaderCapture : public StubInterface {
  public:
    int headerCalls = 0;
    void addHeader(const DRW_Header *) override { ++headerCalls; }
  } capture;

  const char *dxf =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC9999\n"
      "0\nENDSEC\n0\nEOF\n";

  const auto path =
      std::filesystem::temp_directory_path() / "lc_invalid_acadver.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }

  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, /*ext=*/true));
  CHECK(reader.getError() == DRW::BAD_VERSION);
  CHECK(capture.headerCalls == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF TABLES require ENDTAB for unknown tables",
          "[dxf][tables][malformed]") {
  const auto malformedPath =
      std::filesystem::temp_directory_path() / "lc_unknown_table_no_endtab.dxf";
  std::filesystem::remove(malformedPath);
  {
    std::ofstream out(malformedPath);
    out << "0\nSECTION\n2\nTABLES\n"
           "0\nTABLE\n2\nCUSTOM_TABLE\n"
           "0\nCUSTOM_RECORD\n5\nA\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  StubInterface cap;
  dxfRW malformed(malformedPath.string().c_str());
  CHECK_FALSE(malformed.read(&cap, /*ext=*/true));
  std::filesystem::remove(malformedPath);

  const auto validPath =
      std::filesystem::temp_directory_path() / "lc_unknown_table_endtab.dxf";
  std::filesystem::remove(validPath);
  {
    std::ofstream out(validPath);
    out << "0\nSECTION\n2\nTABLES\n"
           "0\nTABLE\n2\nCUSTOM_TABLE\n"
           "0\nCUSTOM_RECORD\n5\nA\n"
           "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  }

  dxfRW valid(validPath.string().c_str());
  CHECK(valid.read(&cap, /*ext=*/true));
  std::filesystem::remove(validPath);
}

TEST_CASE("DXF OBJECTS rejects an ENDBLK boundary",
          "[dxf][objects][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_objects_endblk.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nOBJECTS\n"
           "0\nENDBLK\n5\n20\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  StubInterface cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  std::filesystem::remove(path);
}

TEST_CASE("DXF OBJECTS does not publish a raw object before ENDBLK",
          "[dxf][objects][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_object_endblk.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nOBJECTS\n"
           "0\nACDBWFDIAG\n5\n3B\n1\npartial\n"
           "0\nENDBLK\n5\n20\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  RawObjectCapture cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  CHECK(cap.m_objects.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF OBJECTS does not publish a raw object before an empty boundary",
          "[dxf][objects][malformed]") {
  RawObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBWFDIAG\n5\n3B\n1\npartial\n"
      "0\n\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_object_empty_boundary.dxf"));
  CHECK(cap.m_objects.empty());
}

TEST_CASE("DXF OBJECTS does not publish a typed object before an empty boundary",
          "[dxf][objects][malformed]") {
  GroupCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGROUP\n5\n20\n"
      "0\n\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_typed_object_empty_boundary.dxf"));
  CHECK(cap.m_callCount == 0);
}

TEST_CASE("DXF PLOTSETTINGS rejects an invalid shade plot handle",
          "[dxf][objects][plotsettings][malformed]") {
  PlotSettingsCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nPLOTSETTINGS\n5\n20\n100\nAcDbPlotSettings\n"
      "333\nNOT_HEX\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, capture,
                         "lc_plotsettings_invalid_shade_handle.dxf"));
  CHECK(capture.m_callCount == 0);
}

TEST_CASE("DXF ENTITIES rejects a top-level ENDBLK boundary",
          "[dxf][entities][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_entities_endblk.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nLINE\n5\n20\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
           "0\nENDBLK\n5\n21\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  StubInterface cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  std::filesystem::remove(path);
}

TEST_CASE("DXF ENTITIES rejects an empty entity boundary",
          "[dxf][entities][malformed]") {
  StubInterface cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n20\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\n\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_entities_empty_boundary.dxf"));
}

TEST_CASE("DXF ENTITIES rejects a non-boundary record at dispatch",
          "[dxf][entities][boundary][malformed]") {
  LineCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "8\n0\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_entities_nonboundary_dispatch.dxf"));
  CHECK(cap.m_callCount == 0);
}

TEST_CASE("DXF POLYLINE requires its SEQEND terminator",
          "[dxf][entities][polyline][malformed]") {
  const std::vector<std::string> boundaries = {"ENDSEC", "ENDBLK"};
  for (std::size_t i = 0; i < boundaries.size(); ++i) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_polyline_missing_seqend_" + std::to_string(i) +
                       ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
             "0\nPOLYLINE\n5\n20\n8\n0\n"
             "0\n"
          << boundaries[i]
          << "\n5\n21\n"
             "0\nENDSEC\n0\nEOF\n";
    }

    PolylineCapture cap;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&cap, /*ext=*/true));
    CHECK(cap.m_callCount == 0);
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF rejects non-finite numeric values",
          "[dxf][safety][numeric]") {
  const std::vector<std::string> values = {"nan", "inf", "-inf"};
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_nonfinite_dxf_" + std::to_string(i) + ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
             "0\nLINE\n8\n0\n10\n"
          << values[i]
          << "\n20\n0\n11\n1\n21\n1\n"
             "0\nENDSEC\n0\nEOF\n";
    }

    StubInterface cap;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&cap, /*ext=*/true));
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF rejects out-of-range integer values",
          "[dxf][safety][numeric]") {
  const std::vector<std::pair<std::string, std::string>> values = {
      {"70", "65536"}, {"90", "2147483648"}};
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto path = std::filesystem::temp_directory_path() /
                      ("lc_out_of_range_dxf_" + std::to_string(i) + ".dxf");
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nOBJECTS\n"
             "0\nACDBWFDIAG\n5\n3B\n"
          << values[i].first << "\n" << values[i].second
          << "\n0\nENDSEC\n0\nEOF\n";
    }

    StubInterface cap;
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&cap, /*ext=*/true));
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF rejects invalid entity space indicators",
          "[dxf][safety][numeric]") {
  LineCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n20\n8\n0\n67\n2\n"
      "10\n0\n20\n0\n11\n1\n21\n1\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_invalid_entity_space.dxf"));
  CHECK(cap.m_callCount == 0);
}

TEST_CASE("DXF rejects out-of-range boolean values",
          "[dxf][safety][numeric]") {
  RawObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBBOOLRANGE\n5\n3B\n290\n2147483648\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_bool_out_of_range.dxf"));
  CHECK(cap.m_objects.empty());
}

TEST_CASE("DXF SHAPE preserves typed fields", "[dxf][shape]") {
  ShapeCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nSHAPE\n5\n101\n330\n1F\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbShape\n"
      "2\nTESTSHAPE\n10\n1.0\n20\n2.0\n30\n3.0\n"
      "39\n0.5\n40\n2.5\n41\n1.25\n50\n90.0\n51\n30.0\n"
      "210\n0.0\n220\n1.0\n230\n0.0\n"
      "0\nLINE\n5\n102\n8\n0\n10\n4.0\n20\n5.0\n11\n6.0\n21\n7.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_shape_typed_read.dxf", false);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x101u);
  CHECK(cap.m_captured.parentHandle == 0x1Fu);
  CHECK(cap.m_captured.m_styleName == "TESTSHAPE");
  CHECK(cap.m_captured.m_insertionPoint.x == 1.0);
  CHECK(cap.m_captured.m_insertionPoint.y == 2.0);
  CHECK(cap.m_captured.m_insertionPoint.z == 3.0);
  CHECK(cap.m_captured.m_scale == 2.5);
  CHECK(cap.m_captured.m_rotation == Catch::Approx(std::acos(-1.0) / 2.0));
  CHECK(cap.m_captured.m_widthFactor == 1.25);
  CHECK(cap.m_captured.m_oblique == Catch::Approx(std::acos(-1.0) / 6.0));
  CHECK(cap.m_captured.m_thickness == 0.5);
  CHECK(cap.m_captured.m_extrusion.x == 0.0);
  CHECK(cap.m_captured.m_extrusion.y == 1.0);
  CHECK(cap.m_captured.m_extrusion.z == 0.0);
  REQUIRE(cap.m_lineCallCount == 1);
  CHECK(cap.m_line.handle == 0x102u);
  CHECK(cap.m_line.basePoint.x == 4.0);
  CHECK(cap.m_line.basePoint.y == 5.0);
  CHECK(cap.m_line.secPoint.x == 6.0);
  CHECK(cap.m_line.secPoint.y == 7.0);
}

TEST_CASE("DXF CAMERA preserves its view reference and writes back",
          "[dxf][camera]") {
  CameraCapture readCapture;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nCAMERA\n5\n25\n330\n1F\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbCamera\n340\n75\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, readCapture, "lc_camera_typed_read.dxf", false);

  REQUIRE(readCapture.m_callCount == 1);
  CHECK(readCapture.m_captured.handle == 0x25u);
  CHECK(readCapture.m_captured.parentHandle == 0x1Fu);
  CHECK(readCapture.m_captured.m_viewHandle == 0x75u);

  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeEntities() override {
      DRW_Camera camera;
      camera.handle = 0x25u;
      camera.parentHandle = 0x1Fu;
      camera.m_viewHandle = 0x75u;
      REQUIRE(m_writer->writeCamera(&camera));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_camera_typed_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  CameraCapture writtenCapture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&writtenCapture, false));
  REQUIRE(writtenCapture.m_callCount == 1);
  CHECK(writtenCapture.m_captured.m_viewHandle == 0x75u);
  std::filesystem::remove(path);
}

TEST_CASE("DXF GEOPOSITIONMARKER preserves typed fields and raw aliases",
          "[dxf][geopositionmarker]") {
  GeoPositionMarkerCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nGEOPOSITIONMARKER\n5\n25\n330\n1F\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbGeoPositionMarker\n"
      "90\n3\n10\n1.5\n20\n2.5\n30\n3.5\n"
      "40\n4.5\n1\nsurvey note\n40\n0.25\n"
      "290\n1\n280\n2\n290\n0\n"
      "0\nPOSITIONMARKER\n5\n26\n330\n1F\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbGeoPositionMarker\n"
      "90\n4\n10\n10.0\n20\n20.0\n30\n0.0\n"
      "40\n1.0\n1\nalias\n40\n0.5\n290\n0\n280\n1\n290\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_geopositionmarker_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 2);
  REQUIRE(capture.m_raw.size() == 2);
  CHECK(capture.m_captured.handle == 0x25u);
  CHECK(capture.m_captured.parentHandle == 0x1Fu);
  CHECK(capture.m_captured.m_classVersion == 3u);
  CHECK(capture.m_captured.m_position.x == 1.5);
  CHECK(capture.m_captured.m_position.y == 2.5);
  CHECK(capture.m_captured.m_position.z == 3.5);
  CHECK(capture.m_captured.m_radius == 4.5);
  CHECK(capture.m_captured.m_notes == "survey note");
  CHECK(capture.m_captured.m_landingGap == 0.25);
  CHECK(capture.m_captured.m_mtextVisible);
  CHECK(capture.m_captured.m_textAlignment == 2);
  CHECK_FALSE(capture.m_captured.m_enableFrameText);
  CHECK(capture.m_raw[0].name == "GEOPOSITIONMARKER");
  CHECK(capture.m_raw[1].name == "POSITIONMARKER");
  CHECK(capture.m_raw[0].groups.size() > 8);
}

TEST_CASE("DXF GEOPOSITIONMARKER rejects out-of-range byte fields",
          "[dxf][geopositionmarker][numeric][malformed]") {
  GeoPositionMarkerCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nGEOPOSITIONMARKER\n5\n25\n330\n1F\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbGeoPositionMarker\n"
      "90\n3\n10\n1.5\n20\n2.5\n30\n3.5\n"
      "280\n256\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, capture,
                         "lc_geopositionmarker_bad_byte.dxf", false));
  CHECK(capture.m_callCount == 0);
  CHECK(capture.m_raw.empty());
}

TEST_CASE("DXF GEOPOSITIONMARKER rejects invalid version and boolean fields",
          "[dxf][geopositionmarker][numeric][malformed]") {
  const std::vector<std::string> malformed = {
      "90\n-1\n",
      "90\n3\n290\n2\n"};
  for (std::size_t i = 0; i < malformed.size(); ++i) {
    GeoPositionMarkerCapture capture;
    const std::string dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nGEOPOSITIONMARKER\n5\n25\n330\n1F\n8\n0\n"
        "100\nAcDbEntity\n100\nAcDbGeoPositionMarker\n"
        + malformed[i] + "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(
        dxf, capture,
        ("lc_geopositionmarker_bad_scalar_" + std::to_string(i) + ".dxf").c_str(),
        false));
    CHECK(capture.m_callCount == 0);
    CHECK(capture.m_raw.empty());
  }
}

TEST_CASE("DXF raw-backed entity shells reject unclosed application groups",
          "[dxf][malformed][entity][application-group]") {
  SECTION("GEOPOSITIONMARKER") {
    GeoPositionMarkerCapture capture;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nGEOPOSITIONMARKER\n5\n25\n102\n{CUSTOM\n1\npayload\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, capture,
                           "lc_geopositionmarker_unclosed_app_group.dxf", false));
    CHECK(capture.m_callCount == 0);
    CHECK(capture.m_raw.empty());
  }

  SECTION("SECTIONOBJECT") {
    SectionObjectCapture capture;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nSECTIONOBJECT\n5\n40\n102\n{CUSTOM\n1\npayload\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, capture,
                           "lc_sectionobject_unclosed_app_group.dxf", false));
    CHECK(capture.m_callCount == 0);
    CHECK(capture.m_raw.empty());
  }
}

TEST_CASE("DXF SECTIONOBJECT preserves complete raw application groups",
          "[dxf][rawentity][sectionobject][application-group]") {
  const std::string source =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nSECTIONOBJECT\n5\n40\n330\n1F\n8\n0\n100\nAcDbEntity\n"
      "102\n{ACAD_REACTORS\n330\nF0\n102\n}\n"
      "102\n{CUSTOM\n102\n{NESTED\n1\npayload\n102\n}\n102\n}\n"
      "100\nAcDbSection\n90\n1\n91\n2\n1\nSection\n"
      "10\n0\n20\n0\n30\n1\n40\n2\n41\n3\n70\n4\n62\n5\n"
      "92\n0\n93\n0\n360\n42\n"
      "0\nENDSEC\n0\nEOF\n";
  SectionObjectCapture captured;
  readDxf(source, captured, "lc_sectionobject_application_groups.dxf", false);

  REQUIRE(captured.m_callCount == 1);
  CHECK(captured.m_captured.reactorHandles
        == std::vector<std::uint32_t>{0xF0u});
  CHECK(captured.m_captured.appData.size() == 2);
  CHECK(captured.m_captured.m_sectionSettingsHandle == 0x42u);
  REQUIRE(captured.m_raw.size() == 1);
  const DRW_RawDxfObject &entity = captured.m_raw.front();
  const std::vector<int> expectedCodes = {
      5, 330, 8, 100, 102, 330, 102, 102, 102, 1, 102, 102,
      100, 90, 91, 1, 10, 20, 30, 40, 41, 70, 62, 92, 93, 360};
  std::vector<int> codes;
  codes.reserve(entity.groups.size());
  for (const DRW_Variant &group : entity.groups)
    codes.push_back(group.code());
  CHECK(codes == expectedCodes);

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_sectionobject_application_groups_rt.dxf";
  std::filesystem::remove(path);
  RawEntityEmitter emitter;
  emitter.m_entity = entity;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    CHECK(emitter.m_writeResult);
  }

  SectionObjectCapture replayed;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&replayed, false));
  }
  REQUIRE(replayed.m_raw.size() == 1);
  CHECK(replayed.m_raw.front().groups.size() == entity.groups.size());
  std::filesystem::remove(path);
}

TEST_CASE("DXF OLE2FRAME preserves typed fields and binary payload",
          "[dxf][ole2frame]") {
  Ole2FrameCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nOLE2FRAME\n5\n102\n330\n1F\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbOle2Frame\n"
      "70\n2\n3\nOLE\n"
      "10\n1.0\n20\n6.0\n30\n0.0\n"
      "11\n5.0\n21\n2.0\n31\n0.0\n"
      "71\n2\n72\n1\n73\n1\n90\n4\n310\n01020304\n1\nOLE\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_ole2frame_typed_read.dxf", false);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x102u);
  CHECK(cap.m_captured.parentHandle == 0x1Fu);
  CHECK(cap.m_captured.m_oleVersion == 2);
  CHECK(cap.m_captured.m_oleClient == "OLE");
  CHECK(cap.m_captured.m_flags == 2);
  CHECK(cap.m_captured.m_mode == 1);
  CHECK(cap.m_captured.m_lockAspect == 1);
  CHECK(cap.m_captured.m_pt1.x == 1.0);
  CHECK(cap.m_captured.m_pt1.y == 6.0);
  CHECK(cap.m_captured.m_pt2.x == 5.0);
  CHECK(cap.m_captured.m_pt2.y == 2.0);
  CHECK(cap.m_captured.m_declaredPayloadLength == 4);
  CHECK(cap.m_captured.m_payloadBytes ==
        std::vector<std::uint8_t>{0x01, 0x02, 0x03, 0x04});
}

TEST_CASE("DXF OLE2FRAME rejects a truncated binary payload",
          "[dxf][ole2frame][safety]") {
  Ole2FrameCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nOLE2FRAME\n5\n103\n330\n1F\n8\n0\n"
      "70\n2\n90\n5\n310\n01020304\n"
      "0\nENDSEC\n0\nEOF\n";
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_ole2frame_truncated_read.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, false));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF OLE2FRAME writer preserves lock-aspect and payload",
          "[dxf][ole2frame][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeEntities() override {
      DRW_Ole2Frame frame;
      frame.m_flags = 3;
      frame.m_mode = 1;
      frame.m_lockAspect = 7;
      frame.m_pt1 = DRW_Coord{1.0, 4.0, 0.0};
      frame.m_pt2 = DRW_Coord{8.0, 2.0, 0.0};
      frame.m_payloadBytes = {0xA0, 0xB1, 0xC2};
      REQUIRE(m_writer->writeOle2Frame(&frame));
    }
  } emitter;

  for (const bool binary : {false, true}) {
    const auto path = std::filesystem::temp_directory_path()
                      / (binary ? "lc_ole2frame_typed_write_bin.dxf"
                                 : "lc_ole2frame_typed_write_ascii.dxf");
    std::filesystem::remove(path);
    {
      dxfRW writer(path.string().c_str());
      emitter.m_writer = &writer;
      REQUIRE(writer.write(&emitter, DRW::AC1021, binary));
    }

    Ole2FrameCapture capture;
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, false));
    std::filesystem::remove(path);

    REQUIRE(capture.m_callCount == 1);
    CHECK(capture.m_captured.m_flags == 3);
    CHECK(capture.m_captured.m_mode == 1);
    CHECK(capture.m_captured.m_lockAspect == 7);
    CHECK(capture.m_captured.m_pt1.x == 1.0);
    CHECK(capture.m_captured.m_pt2.y == 2.0);
    CHECK(capture.m_captured.m_payloadBytes ==
          std::vector<std::uint8_t>{0xA0, 0xB1, 0xC2});
  }
}

TEST_CASE("DXF POINTCLOUD preserves source files and references",
          "[dxf][pointcloud]") {
  PointCloudCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nPOINTCLOUD\n5\nA0\n330\n10\n8\ncloud-layer\n"
      "100\nAcDbEntity\n100\nAcDbPointCloud\n70\n3\n"
      "10\n1.5\n20\n2.5\n30\n3.5\n1\ncloud.rcp\n"
      "90\n2\n2\none.rcs\n2\ntwo.rcs\n"
      "11\n-1\n21\n-2\n31\n-3\n12\n4\n22\n5\n32\n6\n"
      "92\n123456789\n3\nUCS-A\n13\n7\n23\n8\n33\n9\n"
      "210\n1\n220\n0\n230\n0\n211\n0\n221\n1\n231\n0\n"
      "212\n0\n222\n0\n232\n1\n330\nB0\n360\nC0\n"
      "71\n4\n40\n0.1\n41\n0.9\n42\n0.2\n43\n0.8\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointcloud_typed_read.dxf", false);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0xA0u);
  CHECK(cap.m_captured.parentHandle == 0x10u);
  CHECK(cap.m_captured.layer == "cloud-layer");
  CHECK(cap.m_captured.classVersion == 3u);
  CHECK(cap.m_captured.origin.x == 1.5);
  CHECK(cap.m_captured.savedFilename == "cloud.rcp");
  CHECK(cap.m_captured.sourceFileCount == 2);
  REQUIRE(cap.m_captured.sourceFiles.size() == 2);
  CHECK(cap.m_captured.sourceFiles[0] == "one.rcs");
  CHECK(cap.m_captured.sourceFiles[1] == "two.rcs");
  CHECK(cap.m_captured.pointCount == 123456789u);
  CHECK(cap.m_captured.ucsName == "UCS-A");
  CHECK(cap.m_captured.definitionHandle == 0xB0u);
  CHECK(cap.m_captured.reactorHandle == 0xC0u);
  CHECK(cap.m_captured.intensityScheme == 4);
  CHECK(cap.m_captured.intensityStyle.highThreshold == 0.8);
}

TEST_CASE("DXF POINTCLOUD preserves the zero-source extended branch",
          "[dxf][pointcloud]") {
  PointCloudCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nPOINTCLOUD\n5\nA1\n100\nAcDbEntity\n"
      "100\nAcDbPointCloud\n70\n4\n10\n0\n20\n0\n30\n0\n1\nempty.rcp\n"
      "90\n0\n11\n-10\n21\n-20\n31\n-30\n12\n10\n22\n20\n32\n30\n"
      "92\n2147483647\n3\nUCS\n13\n1\n23\n2\n33\n3\n"
      "210\n1\n220\n0\n230\n0\n211\n0\n221\n1\n231\n0\n"
      "212\n0\n222\n0\n232\n1\n330\nB1\n360\nC1\n"
      "71\n2\n40\n0.1\n41\n0.9\n42\n0.2\n43\n0.8\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointcloud_zero_source_read.dxf", false);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.sourceFileCount == 0);
  CHECK(cap.m_captured.pointCount == 2147483647ULL);
  CHECK(cap.m_captured.extentsMin.z == -30.0);
  CHECK(cap.m_captured.extentsMax.x == 10.0);
  CHECK(cap.m_captured.definitionHandle == 0xB1u);
  CHECK(cap.m_captured.reactorHandle == 0xC1u);
}

TEST_CASE("DXF POINTCLOUD rejects an invalid source-file count",
          "[dxf][pointcloud][safety]") {
  PointCloudCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nPOINTCLOUD\n5\nA2\n100\nAcDbEntity\n100\nAcDbPointCloud\n"
      "70\n3\n1\ncloud.rcp\n90\n-1\n"
      "0\nENDSEC\n0\nEOF\n";
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointcloud_invalid_count.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, false));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF POINTCLOUD writer emits the complete source-file form",
          "[dxf][pointcloud][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeEntities() override {
      DRW_PointCloud pointCloud;
      pointCloud.classVersion = 3;
      pointCloud.origin = DRW_Coord{1.0, 2.0, 3.0};
      pointCloud.savedFilename = "cloud.rcp";
      pointCloud.sourceFileCount = 2;
      pointCloud.sourceFiles = {"one.rcs", "two.rcs"};
      pointCloud.extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
      pointCloud.extentsMax = DRW_Coord{4.0, 5.0, 6.0};
      pointCloud.pointCount = 123456;
      pointCloud.ucsName = "UCS-A";
      pointCloud.definitionHandle = 0xB0;
      pointCloud.reactorHandle = 0xC0;
      REQUIRE(m_writer->writePointCloud(&pointCloud));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointcloud_source_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1027, false));
  }

  PointCloudCapture cap;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&cap, false));
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.sourceFiles ==
        std::vector<UTF8STRING>{"one.rcs", "two.rcs"});
  CHECK(cap.m_captured.extentsMax.z == 6.0);
  CHECK(cap.m_captured.pointCount == 123456u);
  CHECK(cap.m_captured.ucsName == "UCS-A");
  CHECK(cap.m_captured.definitionHandle == 0xB0u);
  CHECK(cap.m_captured.reactorHandle == 0xC0u);
}

TEST_CASE("DXF POINTCLOUDEX preserves a crop record",
          "[dxf][pointcloud][pointcloud-ex]") {
  PointCloudExCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nPOINTCLOUDEX\n5\nA3\n330\n10\n100\nAcDbEntity\n"
      "100\nAcDbPointCloud\n70\n7\n11\n-1\n21\n-2\n31\n-3\n"
      "12\n7\n22\n8\n32\n9\n210\n1\n220\n0\n230\n0\n"
      "211\n0\n221\n1\n231\n0\n212\n0\n222\n0\n232\n1\n"
      "290\n1\n330\nB3\n360\nC3\n1\ncloud-ex\n291\n1\n295\n1\n"
      "92\n1\n280\n2\n290\n1\n290\n0\n13\n1\n23\n2\n33\n3\n"
      "213\n4\n223\n5\n233\n6\n213\n7\n223\n8\n233\n9\n"
      "93\n2\n13\n10\n23\n11\n33\n12\n13\n13\n23\n14\n33\n15\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointcloudex_typed_read.dxf", false);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0xA3u);
  CHECK(cap.m_captured.parentHandle == 0x10u);
  CHECK(cap.m_captured.classVersion == 7u);
  CHECK(cap.m_captured.definitionHandle == 0xB3u);
  CHECK(cap.m_captured.reactorHandle == 0xC3u);
  CHECK(cap.m_captured.croppingCount == 1);
  REQUIRE(cap.m_captured.croppings.size() == 1);
  const DRW_PointCloudExCropping &crop = cap.m_captured.croppings.front();
  CHECK(crop.type == 2);
  CHECK(crop.isInside);
  CHECK_FALSE(crop.isInverted);
  CHECK(crop.cropPlane.z == 3.0);
  CHECK(crop.cropYDirection.x == 7.0);
  REQUIRE(crop.points.size() == 2);
  CHECK(crop.points[1].z == 15.0);
}

TEST_CASE("DXF POINTCLOUDEX writer round-trips the zero-crop form",
          "[dxf][pointcloud][pointcloud-ex][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeEntities() override {
      DRW_PointCloudEx pointCloud;
      pointCloud.classVersion = 7;
      pointCloud.extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
      pointCloud.extentsMax = DRW_Coord{4.0, 5.0, 6.0};
      pointCloud.ucsOrigin = DRW_Coord{7.0, 8.0, 9.0};
      pointCloud.definitionHandle = 0xB4;
      pointCloud.reactorHandle = 0xC4;
      pointCloud.name = "cloud-ex";
      pointCloud.showIntensity = true;
      pointCloud.showCropping = true;
      pointCloud.unknownInt0 = 41;
      pointCloud.unknownInt1 = 42;
      pointCloud.stylizationType = 3;
      pointCloud.intensityColorScheme = "intensity";
      pointCloud.currentColorScheme = "current";
      pointCloud.classificationColorScheme = "classification";
      pointCloud.elevationMin = 10.5;
      pointCloud.elevationMax = 20.5;
      pointCloud.intensityMin = 100.0;
      pointCloud.intensityMax = 200.0;
      pointCloud.intensityOutOfRangeBehavior = 4;
      pointCloud.elevationOutOfRangeBehavior = 5;
      REQUIRE(m_writer->writePointCloudEx(&pointCloud));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointcloudex_zero_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1027, false));
  }

  PointCloudExCapture cap;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&cap, false));
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.extentsMin.x == -1.0);
  CHECK(cap.m_captured.extentsMax.z == 6.0);
  CHECK(cap.m_captured.ucsOrigin.y == 8.0);
  CHECK(cap.m_captured.definitionHandle == 0xB4u);
  CHECK(cap.m_captured.reactorHandle == 0xC4u);
  CHECK(cap.m_captured.croppingCount == 0);
  CHECK(cap.m_captured.unknownInt0 == 41);
  CHECK(cap.m_captured.unknownInt1 == 42);
  CHECK(cap.m_captured.intensityMax == 200.0);
}

TEST_CASE("DXF POINTCLOUDEX rejects a truncated crop record",
          "[dxf][pointcloud][pointcloud-ex][safety]") {
  PointCloudExCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nPOINTCLOUDEX\n5\nA4\n100\nAcDbEntity\n100\nAcDbPointCloud\n"
      "70\n7\n290\n1\n1\ncloud-ex\n92\n1\n280\n2\n290\n1\n290\n0\n"
      "13\n1\n23\n2\n33\n3\n213\n4\n223\n5\n233\n6\n"
      "213\n7\n223\n8\n233\n9\n93\n2\n13\n10\n23\n11\n33\n12\n"
      "0\nENDSEC\n0\nEOF\n";
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointcloudex_truncated_crop.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, false));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF NAVISWORKSMODEL preserves transform and definition handle",
          "[dxf][navisworks]") {
  NavisworksModelCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nNAVISWORKSMODEL\n5\nA5\n330\n10\n8\nmodel-layer\n"
      "100\nAcDbEntity\n100\nAcDbNavisworksModel\n70\n7\n340\n2D0\n"
      "40\n1\n40\n2\n40\n3\n40\n4\n40\n5\n40\n6\n40\n7\n40\n8\n"
      "40\n9\n40\n10\n40\n11\n40\n12\n40\n13\n40\n14\n"
      "40\n15\n40\n16\n40\n0.001\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_navisworks_model_typed_read.dxf", false);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0xA5u);
  CHECK(cap.m_captured.parentHandle == 0x10u);
  CHECK(cap.m_captured.layer == "model-layer");
  CHECK(cap.m_captured.flags == 7u);
  CHECK(cap.m_captured.definitionHandle == 0x2D0u);
  CHECK(cap.m_captured.transform[0] == 1.0);
  CHECK(cap.m_captured.transform[15] == 16.0);
  CHECK(cap.m_captured.unitFactor == Catch::Approx(0.001));
}

TEST_CASE("DXF NAVISWORKSMODEL writer round-trips its complete matrix",
          "[dxf][navisworks][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeEntities() override {
      DRW_NavisworksModel model;
      model.flags = 0x8001;
      model.definitionHandle = 0x2E0;
      for (std::size_t i = 0; i < model.transform.size(); ++i)
        model.transform[i] = static_cast<double>(i) + 0.5;
      model.unitFactor = 0.0254;
      REQUIRE(m_writer->writeNavisworksModel(&model));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_navisworks_model_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1027, false));
  }

  NavisworksModelCapture cap;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&cap, false));
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.flags == 0x8001u);
  CHECK(cap.m_captured.definitionHandle == 0x2E0u);
  CHECK(cap.m_captured.transform[3] == Catch::Approx(3.5));
  CHECK(cap.m_captured.transform[12] == Catch::Approx(12.5));
  CHECK(cap.m_captured.unitFactor == Catch::Approx(0.0254));
}

TEST_CASE("DXF NAVISWORKSMODEL rejects an incomplete or overlong matrix",
          "[dxf][navisworks][safety]") {
  for (const auto &values : {std::string("40\n1\n")
                                  + "40\n2\n40\n3\n40\n4\n40\n5\n"
                                    "40\n6\n40\n7\n40\n8\n40\n9\n40\n10\n"
                                    "40\n11\n40\n12\n40\n13\n40\n14\n"
                                    "40\n15\n40\n16\n",
                              std::string("40\n1\n40\n2\n40\n3\n40\n4\n40\n5\n"
                                    "40\n6\n40\n7\n40\n8\n40\n9\n40\n10\n"
                                    "40\n11\n40\n12\n40\n13\n40\n14\n"
                                    "40\n15\n40\n16\n40\n0.001\n40\n0.002\n")}) {
    NavisworksModelCapture cap;
    const auto path = std::filesystem::temp_directory_path()
                      / "lc_navisworks_model_invalid_matrix.dxf";
    std::filesystem::remove(path);
    {
      std::ofstream out(path);
      out << "0\nSECTION\n2\nENTITIES\n"
          << "0\nNAVISWORKSMODEL\n5\nA6\n100\nAcDbEntity\n"
          << "100\nAcDbNavisworksModel\n70\n1\n" << values
          << "0\nENDSEC\n0\nEOF\n";
    }
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&cap, false));
    CHECK(cap.m_callCount == 0);
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF OLEFRAME preserves typed fields and binary payload",
          "[dxf][oleframe]") {
  OleFrameCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nOLEFRAME\n5\n104\n330\n1F\n8\nOLE_LAYER\n"
      "100\nAcDbEntity\n100\nAcDbOleFrame\n"
      "70\n2\n90\n4\n310\nDEADBEEF\n1\nOLE\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_oleframe_typed_read.dxf", false);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x104u);
  CHECK(cap.m_captured.parentHandle == 0x1Fu);
  CHECK(cap.m_captured.layer == "OLE_LAYER");
  CHECK(cap.m_captured.m_flags == 2u);
  CHECK(cap.m_captured.m_declaredPayloadLength == 4u);
  CHECK(cap.m_captured.m_payloadByteCount == 4u);
  CHECK(cap.m_captured.m_payloadPresent);
  CHECK_FALSE(cap.m_captured.m_payloadTruncated);
  CHECK(cap.m_captured.m_payloadBytes ==
        std::vector<std::uint8_t>{0xDE, 0xAD, 0xBE, 0xEF});
}

TEST_CASE("DXF OLEFRAME rejects a truncated binary payload",
          "[dxf][oleframe][safety]") {
  OleFrameCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nOLEFRAME\n5\n105\n330\n1F\n8\n0\n"
      "70\n2\n90\n7\n310\nDEADBEEF\n310\nCAFE\n"
      "0\nENDSEC\n0\nEOF\n";
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_oleframe_truncated_read.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, false));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF OLEFRAME writer preserves flags and payload",
          "[dxf][oleframe][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *m_writer = nullptr;

    void writeEntities() override {
      DRW_OleFrame frame;
      frame.handle = 0x106u;
      frame.parentHandle = 0x1Fu;
      frame.m_flags = 3u;
      frame.m_mode = 1u;
      frame.m_payloadBytes = {0xA0, 0xB1, 0xC2};
      frame.m_payloadPresent = true;
      frame.m_declaredPayloadLength = 3u;
      REQUIRE(m_writer->writeOleFrame(&frame));
    }
  } emitter;

  for (const bool binary : {false, true}) {
    const auto path = std::filesystem::temp_directory_path()
                      / (binary ? "lc_oleframe_typed_write_bin.dxf"
                                 : "lc_oleframe_typed_write_ascii.dxf");
    std::filesystem::remove(path);
    {
      dxfRW writer(path.string().c_str());
      emitter.m_writer = &writer;
      REQUIRE(writer.write(&emitter, DRW::AC1021, binary));
    }

    OleFrameCapture capture;
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&capture, false));
    std::filesystem::remove(path);

    REQUIRE(capture.m_callCount == 1);
    CHECK(capture.m_captured.m_flags == 3u);
    CHECK(capture.m_captured.m_declaredPayloadLength == 3u);
    CHECK(capture.m_captured.m_payloadBytes ==
          std::vector<std::uint8_t>{0xA0, 0xB1, 0xC2});
  }
}

TEST_CASE("DXF CLASSES are published only after a valid section", "[dxf][classes][safety]") {
  DxfClassCapture cap;
  const auto path = std::filesystem::temp_directory_path() / "lc_classes_transaction.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nCLASSES\n"
           "0\nCLASS\n1\nGOOD\n2\nAcDbGood\n"
           "0\nCLASS\n1\nBROKEN\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, false));
  CHECK(cap.m_classes.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF CLASSES accept case-insensitive CLASS records",
          "[dxf][classes]") {
  DxfClassCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nCLASSES\n"
      "0\nclass\n1\nLOWERCASE\n2\nAcDbLowercase\n"
      "3\nObjectDBX Classes\n90\n0\n91\n1\n280\n0\n281\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK(tryReadDxf(dxf, cap, "lc_classes_case_insensitive.dxf"));
  REQUIRE(cap.m_classes.size() == 1);
  CHECK(cap.m_classes.front().recName == "LOWERCASE");
  CHECK(cap.m_classes.front().className == "AcDbLowercase");
}

TEST_CASE("DXF CLASSES reject data outside CLASS records",
          "[dxf][classes][malformed]") {
  DxfClassCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nCLASSES\n"
      "1\nstray-before-class\n"
      "0\nCLASS\n1\nGOOD\n2\nAcDbGood\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_classes_stray_data.dxf"));
  CHECK(cap.m_classes.empty());
}

TEST_CASE("DXF CLASSES reject duplicate fields and negative instances",
          "[dxf][classes][malformed]") {
  SECTION("duplicate class name") {
    DxfClassCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nCLASSES\n"
        "0\nCLASS\n1\nDUP\n2\nAcDbFirst\n2\nAcDbSecond\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_classes_duplicate_field.dxf"));
    CHECK(cap.m_classes.empty());
  }

  SECTION("negative instance count") {
    DxfClassCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nCLASSES\n"
        "0\nCLASS\n1\nNEG\n2\nAcDbNegative\n91\n-1\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_classes_negative_instances.dxf"));
    CHECK(cap.m_classes.empty());
  }
}

TEST_CASE("DXF CLASSES reject invalid proxy and entity flags",
          "[dxf][classes][malformed]") {
  for (const auto &field : {std::string("280\n2\n"),
                            std::string("281\n-1\n"),
                            std::string("90\n-1\n")}) {
    DxfClassCapture cap;
    const std::string dxf =
        std::string("0\nSECTION\n2\nCLASSES\n")
        + "0\nCLASS\n1\nBADFLAGS\n2\nAcDbBadFlags\n" + field
        + "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_classes_invalid_flags.dxf"));
    CHECK(cap.m_classes.empty());
  }
}

TEST_CASE("DXF comment policy keeps pre-section comments only",
          "[dxf][comments]") {
  CommentPolicyCapture cap;
  const char *dxf =
      "999\nfile comment\n"
      "0\nSECTION\n2\nHEADER\n"
      "999\nheader comment\n"
      "9\n$ACADVER\n1\nAC1021\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "999\nobject comment\n"
      "0\nACDBCOMMENTTEST\n5\n3B\n330\n29\n100\nAcDbCommentTest\n"
      "1\npayload\n0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dxf_comment_policy.dxf");

  CHECK(cap.m_headerCount == 1);
  CHECK(cap.m_headerComments == "file comment");
  REQUIRE(cap.m_objects.size() == 1);
  CHECK(std::none_of(cap.m_objects.front().groups.begin(),
                     cap.m_objects.front().groups.end(),
                     [](const DRW_Variant &group) { return group.code() == 999; }));
}

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

TEST_CASE("DXF OBJECTS dispatch is case-insensitive", "[dxf][objects]") {
  DictionaryCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\ndictionary\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictionary_case_insensitive.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_captured.m_entries.size() == 1);
  CHECK(cap.m_captured.m_entries.front().m_handle == 0xDu);
}

TEST_CASE("DXF TABLES dispatch is case-insensitive", "[dxf][tables]") {
  LayerCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\ntable\n2\nlayer\n"
      "0\nlayer\n5\n10\n2\nMixedLayer\n70\n0\n62\n7\n6\nCONTINUOUS\n"
      "0\nendtab\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_layer_case_insensitive.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.name == "MixedLayer");
}

TEST_CASE("DXF DICTIONARY rejects malformed entry pairs atomically",
          "[dxf][dictionary][safety]") {
  const std::string prefix =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  SECTION("a name without its handle") {
    DictionaryCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + "3\nLONELY\n" + suffix, cap,
                           "lc_dictionary_missing_entry_handle.dxf"));
    CHECK(cap.m_callCount == 0);
  }

  SECTION("an entry handle without a name") {
    DictionaryCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + "350\n2A\n" + suffix, cap,
                           "lc_dictionary_missing_entry_name.dxf"));
    CHECK(cap.m_callCount == 0);
  }

  SECTION("an empty name or null target") {
    DictionaryCapture emptyName;
    CHECK_FALSE(tryReadDxf(prefix + "3\n\n350\n2A\n" + suffix, emptyName,
                           "lc_dictionary_empty_entry_name.dxf"));
    CHECK(emptyName.m_callCount == 0);

    DictionaryCapture nullTarget;
    CHECK_FALSE(tryReadDxf(prefix + "3\nNULL\n350\n0\n" + suffix,
                           nullTarget,
                           "lc_dictionary_null_entry_target.dxf"));
    CHECK(nullTarget.m_callCount == 0);
  }

  SECTION("the entry reference code follows hard-owner") {
    DictionaryCapture softWithHardCode;
    CHECK_FALSE(tryReadDxf(prefix + "3\nSOFT\n360\n2A\n" + suffix,
                           softWithHardCode,
                           "lc_dictionary_soft_entry_hard_code.dxf"));
    CHECK(softWithHardCode.m_callCount == 0);

    DictionaryCapture hardWithSoftCode;
    CHECK_FALSE(tryReadDxf(prefix + "280\n1\n3\nHARD\n350\n2A\n" + suffix,
                           hardWithSoftCode,
                           "lc_dictionary_hard_entry_soft_code.dxf"));
    CHECK(hardWithSoftCode.m_callCount == 0);
  }
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

TEST_CASE("DXF XRECORD decodes typed values and common handles", "[dxf][xrecord]") {
  XRecordCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nXRECORD\n5\n30\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\n31\n102\n}\n"
      "102\n{ACAD_XDICTIONARY\n360\n32\n102\n}\n"
      "102\n{CUSTOM\n102\n{NESTED\n1\npayload\n102\n}\n102\n}\n"
      "100\nAcDbXrecord\n280\n1\n1\nhello\n"
      "10\n1.5\n20\n2.5\n30\n3.5\n70\n7\n310\nA1B2\n"
      "330\n41\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_xrecord_typed_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_raw.size() == 1);
  CHECK(cap.m_captured.handle == 0x30u);
  CHECK(cap.m_captured.parentHandle == 0xCu);
  REQUIRE(cap.m_captured.reactorHandles.size() == 1);
  CHECK(cap.m_captured.reactorHandles[0] == 0x31u);
  CHECK(cap.m_captured.xDictHandle == 0x32u);
  CHECK(cap.m_captured.appData.size() == 3);
  CHECK(cap.m_captured.m_cloning == 1);
  REQUIRE(cap.m_captured.m_values.size() == 4);
  CHECK(cap.m_captured.m_values[0].code() == 1);
  CHECK(std::string(cap.m_captured.m_values[0].c_str()) == "hello");
  REQUIRE(cap.m_captured.m_values[1].type() == DRW_Variant::COORD);
  CHECK(cap.m_captured.m_values[1].coord()->x == 1.5);
  CHECK(cap.m_captured.m_values[1].coord()->y == 2.5);
  CHECK(cap.m_captured.m_values[1].coord()->z == 3.5);
  CHECK(cap.m_captured.m_values[2].i_val() == 7);
  REQUIRE(cap.m_captured.m_values[3].type() == DRW_Variant::BINARY);
  REQUIRE(cap.m_captured.m_values[3].binary()->size() == 2);
  CHECK((*cap.m_captured.m_values[3].binary())[0] == 0xA1u);
  CHECK((*cap.m_captured.m_values[3].binary())[1] == 0xB2u);
  REQUIRE(cap.m_captured.m_handleValues.size() == 1);
  CHECK(cap.m_captured.m_handleValues[0].first == 330);
  CHECK(cap.m_captured.m_handleValues[0].second == 0x41u);
  CHECK(cap.m_raw[0].groups.size() == 22);

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_xrecord_application_groups_rt.dxf";
  std::filesystem::remove(path);
  RawObjectEmitter emitter;
  emitter.m_obj = cap.m_raw.front();
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.reserveHandle(emitter.m_obj.handle));
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    CHECK(emitter.m_writeResult);
  }

  RawObjectCapture replayed;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&replayed, false));
  }
  REQUIRE(replayed.m_objects.size() == 1);
  CHECK(replayed.m_objects.front().groups.size() == cap.m_raw.front().groups.size());
  std::filesystem::remove(path);
}

TEST_CASE("DXF FIELD and FIELDLIST preserve application groups",
          "[dxf][field][application-group]") {
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nFIELD\n5\nE0\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\nE1\n102\n}\n"
      "102\n{ACAD_XDICTIONARY\n360\nE2\n102\n}\n"
      "102\n{CUSTOM\n102\n{NESTED\n1\npayload\n102\n}\n102\n}\n"
      "100\nAcDbField\n1\nAcExpr\n2\n1+1\n"
      "90\n0\n97\n0\n91\n0\n92\n0\n94\n0\n95\n0\n96\n0\n"
      "300\n\n93\n0\n301\n\n98\n0\n"
      "0\nFIELDLIST\n5\nE3\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\nE4\n102\n}\n"
      "102\n{ACAD_XDICTIONARY\n360\nE5\n102\n}\n"
      "102\n{CUSTOM\n1\npayload\n102\n}\n"
      "100\nAcDbIdSet\n90\n1\n290\n0\n330\nE0\n"
      "100\nAcDbFieldList\n"
      "0\nENDSEC\n0\nEOF\n";
  FieldCapture captured;
  readDxf(dxf, captured, "lc_field_application_groups.dxf");

  REQUIRE(captured.m_fields.size() == 1);
  REQUIRE(captured.m_lists.size() == 1);
  CHECK(captured.m_fields.front().appData.size() == 3);
  CHECK(captured.m_fields.front().reactorHandles
        == std::vector<std::uint32_t>{0xE1u});
  CHECK(captured.m_fields.front().xDictHandle == 0xE2u);
  CHECK(captured.m_fields.front().reactorCount() == 1);
  CHECK(captured.m_fields.front().extensionDictionaryFlag() == 1);
  CHECK(captured.m_lists.front().appData.size() == 3);
  CHECK(captured.m_lists.front().reactorHandles
        == std::vector<std::uint32_t>{0xE4u});
  CHECK(captured.m_lists.front().xDictHandle == 0xE5u);
  CHECK(captured.m_lists.front().reactorCount() == 1);
  CHECK(captured.m_lists.front().extensionDictionaryFlag() == 1);
  REQUIRE(captured.m_raw.size() == 2);

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_field_application_groups_rt.dxf";
  std::filesystem::remove(path);
  FieldEmitter emitter;
  emitter.m_field = captured.m_fields.front();
  emitter.m_list = captured.m_lists.front();
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.reserveHandle(emitter.m_field.handle));
    REQUIRE(writer.reserveHandle(emitter.m_list.handle));
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    CHECK(emitter.m_writeResult);
  }

  FieldCapture replayed;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&replayed, false));
  }
  REQUIRE(replayed.m_fields.size() == 1);
  REQUIRE(replayed.m_lists.size() == 1);
  CHECK(replayed.m_fields.front().appData.size() == 3);
  CHECK(replayed.m_lists.front().appData.size() == 3);
  std::filesystem::remove(path);
}

TEST_CASE("DXF FIELD readers reject unclosed application groups",
          "[dxf][field][application-group][malformed]") {
  const auto fieldDxf = [](const char *record) {
    return std::string("0\nSECTION\n2\nOBJECTS\n0\n") + record
           + "\n5\nE0\n330\nC\n102\n{CUSTOM\n1\npayload\n"
             "100\nAcDbIdSet\n90\n0\n290\n0\n"
             "0\nENDSEC\n0\nEOF\n";
  };

  SECTION("FIELD") {
    FieldCapture captured;
    CHECK_FALSE(tryReadDxf(fieldDxf("FIELD"), captured,
                           "lc_field_unclosed_application_group.dxf"));
    CHECK(captured.m_fields.empty());
    CHECK(captured.m_raw.empty());
  }

  SECTION("FIELDLIST") {
    FieldCapture captured;
    CHECK_FALSE(tryReadDxf(fieldDxf("FIELDLIST"), captured,
                           "lc_fieldlist_unclosed_application_group.dxf"));
    CHECK(captured.m_lists.empty());
    CHECK(captured.m_raw.empty());
  }
}

TEST_CASE("DXF dynamic-block shells preserve common handles and raw groups",
          "[dxf][dynamic-block][shell]") {
  DynamicBlockCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nBLOCKPOINTPARAMETER\n5\n40\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\n41\n102\n}\n"
      "100\nAcDbObject\n100\nAcDbEvalExpr\n90\n7\n300\nopaque\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dynamic_block_shell_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_raw.size() == 1);
  CHECK(cap.m_captured.m_recordName == "BLOCKPOINTPARAMETER");
  CHECK(cap.m_captured.handle == 0x40u);
  CHECK(cap.m_captured.parentHandle == 0xCu);
  REQUIRE(cap.m_raw[0].groups.size() == 9);
  CHECK(cap.m_raw[0].handle == 0x40u);
  CHECK(cap.m_raw[0].parentHandle == 0xCu);
}

TEST_CASE("DXF ACSH shells preserve common handles and raw groups",
          "[dxf][acsh][shell]") {
  AcShHistoryCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACSH_BOX_CLASS\n5\n50\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbShBox\n90\n3\n140\n2.5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_acsh_shell_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_raw.size() == 1);
  CHECK(cap.m_captured.m_recordName == "ACSH_BOX_CLASS");
  CHECK(cap.m_captured.handle == 0x50u);
  CHECK(cap.m_captured.parentHandle == 0xCu);
  REQUIRE(cap.m_raw[0].groups.size() == 6);
  CHECK(cap.m_raw[0].handle == 0x50u);
  CHECK(cap.m_raw[0].parentHandle == 0xCu);
}

TEST_CASE("DXF TVDEVICEPROPERTIES decodes fields and preserves raw groups",
          "[dxf][tvdeviceproperties]") {
  TvDevicePropertiesCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nTVDEVICEPROPERTIES\n5\n46\n330\nC\n"
      "102\n{ACAD_XDICTIONARY\n360\nF\n102\n}\n"
      "90\n1\n70\n2\n90\n3\n"
      "160\n4\n161\n5\n162\n6\n163\n99\n"
      "90\n7\n90\n88\n40\n1.5\n40\n2.5\n40\n9.5\n"
      "0\nACDBTVDEVICEPROPERTIES\n5\n47\n330\nD\n90\n8\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_tv_device_properties_read.dxf");

  REQUIRE(cap.m_callCount == 2);
  REQUIRE(cap.m_raw.size() == 2);
  CHECK(cap.m_captured.handle == 0x46u);
  CHECK(cap.m_captured.parentHandle == 0xCu);
  CHECK(cap.m_captured.xDictHandle == 0xFu);
  CHECK(cap.m_captured.appData.size() == 1);
  CHECK(cap.m_captured.flags == 1);
  CHECK(cap.m_captured.maxRegenThreads == 2);
  CHECK(cap.m_captured.useLutPalette == 3);
  CHECK(cap.m_captured.alternateHighlight == 4u);
  CHECK(cap.m_captured.alternateHighlightColor == 5u);
  CHECK(cap.m_captured.geometryShaderUsage == 6u);
  CHECK(cap.m_captured.blendingMode == 7);
  CHECK(cap.m_captured.antialiasingLevel == 1.5);
  CHECK(cap.m_captured.valueBd2 == 2.5);
  CHECK(cap.m_raw[0].groups.size() == 17);
  CHECK(cap.m_raw[0].handle == 0x46u);
  CHECK(cap.m_raw[0].parentHandle == 0xCu);
  CHECK(cap.m_raw[1].name == "ACDBTVDEVICEPROPERTIES");
  CHECK(cap.m_raw[1].handle == 0x47u);
  CHECK(cap.m_raw[1].parentHandle == 0xDu);
}

TEST_CASE("DXF TVDEVICEPROPERTIES rejects unterminated application groups",
          "[dxf][tvdeviceproperties][safety]") {
  TvDevicePropertiesCapture cap;
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_tv_device_properties_invalid.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nOBJECTS\n"
           "0\nTVDEVICEPROPERTIES\n5\n48\n330\nC\n"
           "102\n{ACAD_XDICTIONARY\n360\nF\n"
           "90\n1\n0\nENDSEC\n0\nEOF\n";
  }

  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, true));
  CHECK(cap.m_callCount == 0);
  CHECK(cap.m_raw.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF TVDEVICEPROPERTIES rejects malformed reactor handles",
          "[dxf][tvdeviceproperties][safety][handle]") {
  TvDevicePropertiesCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nTVDEVICEPROPERTIES\n5\n48\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\nnot-hex\n102\n}\n"
      "90\n1\n0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_tv_device_properties_bad_reactor.dxf"));
  CHECK(cap.m_callCount == 0);
  CHECK(cap.m_raw.empty());
}

TEST_CASE("DXF CSACDOCUMENTOPTIONS decodes fields and preserves raw groups",
          "[dxf][csacdocumentoptions]") {
  CsacDocumentOptionsCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nCSACDOCUMENTOPTIONS\n5\n65\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\nD\n102\n}\n"
      "102\n{ACAD_XDICTIONARY\n360\nE\n102\n}\n"
      "90\n17\n90\n34\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_csac_document_options_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_raw.size() == 1);
  CHECK(cap.m_captured.handle == 0x65u);
  CHECK(cap.m_captured.parentHandle == 0xCu);
  CHECK(cap.m_captured.classVersion == 17u);
  CHECK(cap.m_captured.flags == 34u);
  CHECK(cap.m_captured.reactorHandles == std::vector<std::uint32_t>{0xDu});
  CHECK(cap.m_captured.xDictHandle == 0xEu);
  CHECK(cap.m_captured.appData.size() == 2);
  CHECK(cap.m_raw.front().name == "CSACDOCUMENTOPTIONS");
  CHECK(cap.m_raw.front().groups.size() == 10);
  CHECK(cap.m_raw.front().handle == 0x65u);
  CHECK(cap.m_raw.front().parentHandle == 0xCu);
}

TEST_CASE("DXF CSACDOCUMENTOPTIONS rejects unterminated application groups",
          "[dxf][csacdocumentoptions][safety]") {
  CsacDocumentOptionsCapture cap;
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_csac_document_options_invalid.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nOBJECTS\n"
           "0\nCSACDOCUMENTOPTIONS\n5\n66\n330\nC\n"
           "102\n{ACAD_XDICTIONARY\n360\nE\n"
           "90\n17\n0\nENDSEC\n0\nEOF\n";
  }

  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, true));
  CHECK(cap.m_callCount == 0);
  CHECK(cap.m_raw.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF manual object parsers reject malformed self handles",
          "[dxf][malformed][handle][manual-object]") {
  SECTION("TVDEVICEPROPERTIES") {
    TvDevicePropertiesCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nTVDEVICEPROPERTIES\n5\nGZ\n90\n1\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_tv_bad_self_handle.dxf"));
    CHECK(cap.m_callCount == 0);
    CHECK(cap.m_raw.empty());
  }

  SECTION("CSACDOCUMENTOPTIONS") {
    CsacDocumentOptionsCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nCSACDOCUMENTOPTIONS\n5\nGZ\n90\n1\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_csac_bad_self_handle.dxf"));
    CHECK(cap.m_callCount == 0);
    CHECK(cap.m_raw.empty());
  }
}

TEST_CASE("DXF DICTIONARYWDFLT alias reads entries + default handle", "[dxf][dictionary]") {
  DictionaryWithDefaultCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARYWDFLT\n5\n2B\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nNormal\n350\n2C\n"
      "100\nAcDbDictionaryWithDefault\n340\n2C\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dictwdflt_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  REQUIRE(cap.m_captured.m_entries.size() == 1);
  CHECK(cap.m_captured.m_entries[0].m_name == "Normal");
  CHECK(cap.m_captured.m_defaultEntryHandle == 0x2Cu);
}

TEST_CASE("DXF DICTIONARYWDFLT requires one non-null default handle",
          "[dxf][dictionary][safety]") {
  const std::string prefix =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARYWDFLT\n5\n2B\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nNormal\n350\n2C\n100\nAcDbDictionaryWithDefault\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  SECTION("missing default") {
    DictionaryWithDefaultCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + suffix, cap,
                           "lc_dictwdflt_missing_default.dxf"));
    CHECK(cap.m_callCount == 0);
  }

  SECTION("null default") {
    DictionaryWithDefaultCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + "340\n0\n" + suffix, cap,
                           "lc_dictwdflt_null_default.dxf"));
    CHECK(cap.m_callCount == 0);
  }

  SECTION("duplicate default") {
    DictionaryWithDefaultCapture cap;
    CHECK_FALSE(tryReadDxf(prefix + "340\n2C\n340\n2D\n" + suffix, cap,
                           "lc_dictwdflt_duplicate_default.dxf"));
    CHECK(cap.m_callCount == 0);
  }
}

TEST_CASE("DXF RASTERVARIABLES object is read (frame/quality/units)", "[dxf][rastervariables]") {
  RasterVariablesCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBRASTERVARIABLES\n5\n2D\n330\n29\n100\nAcDbRasterVariables\n"
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
      "0\nACDBSUN\n5\n2E\n330\n29\n100\nAcDbSun\n"
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

TEST_CASE("DXF LIGHTLIST preserves named light references",
          "[dxf][lightlist]") {
  LightListCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nLIGHTLIST\n5\n2A\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbLightList\n"
      "90\n7\n90\n2\n5\n101\n1\nKey light\n"
      "5\n102\n1\nFill light\n"
      "0\nACDBLIGHTLIST\n5\n2B\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbLightList\n"
      "90\n8\n90\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_lightlist_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 2);
  REQUIRE(capture.m_raw.size() == 2);
  CHECK(capture.m_captured.handle == 0x2Au);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.m_classVersion == 7u);
  CHECK(capture.m_captured.m_lightCount == 2u);
  REQUIRE(capture.m_captured.m_lights.size() == 2);
  CHECK(capture.m_captured.m_lights[0].m_handle == 0x101u);
  CHECK(capture.m_captured.m_lights[0].m_name == "Key light");
  CHECK(capture.m_captured.m_lights[1].m_handle == 0x102u);
  CHECK(capture.m_captured.m_lights[1].m_name == "Fill light");
  CHECK(capture.m_raw[0].name == "LIGHTLIST");
  CHECK(capture.m_raw[1].name == "ACDBLIGHTLIST");
}

TEST_CASE("DXF LAYERFILTER preserves layer names", "[dxf][layerfilter]") {
  LayerFilterCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nLAYER_FILTER\n5\n2C\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbLayerFilter\n"
      "8\nWalls\n8\nDimensions\n8\nAnnotations\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_layerfilter_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_raw.size() == 1);
  CHECK(capture.m_captured.handle == 0x2Cu);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.m_nameCount == 3u);
  REQUIRE(capture.m_captured.m_names.size() == 3);
  CHECK(capture.m_captured.m_names[0] == "Walls");
  CHECK(capture.m_captured.m_names[1] == "Dimensions");
  CHECK(capture.m_captured.m_names[2] == "Annotations");
  CHECK(capture.m_raw[0].name == "LAYER_FILTER");
}

TEST_CASE("DXF DATALINK preserves connection and custom data",
          "[dxf][datalink]") {
  DataLinkCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDATALINK\n5\n2D\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbDataLink\n"
      "1\nOLEDB\n300\nDescription\n301\nTooltip\n"
      "302\nConnection\n90\n2\n91\n3\n92\n4\n"
      "170\n2024\n171\n1\n172\n2\n173\n3\n174\n4\n"
      "175\n5\n176\n6\n177\n7\n93\n8\n304\nUpdated\n"
      "94\n2\n1\nDATAMAP_BEGIN\n"
      "330\n101\n304\nFirst\n330\n102\n304\nSecond\n"
      "309\nDATAMAP_END\n360\n103\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_datalink_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_raw.size() == 1);
  CHECK(capture.m_captured.handle == 0x2Du);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.m_dataAdapter == "OLEDB");
  CHECK(capture.m_captured.m_description == "Description");
  CHECK(capture.m_captured.m_connectionString == "Connection");
  CHECK(capture.m_captured.m_option == 2);
  CHECK(capture.m_captured.m_year == 2024);
  CHECK(capture.m_captured.m_pathOption == 7);
  CHECK(capture.m_captured.m_updateStatus == "Updated");
  CHECK(capture.m_captured.m_customDataCount == 2);
  REQUIRE(capture.m_captured.m_customData.size() == 2);
  CHECK(capture.m_captured.m_customData[0].m_targetHandle == 0x101u);
  CHECK(capture.m_captured.m_customData[0].m_text == "First");
  CHECK(capture.m_captured.m_customData[1].m_targetHandle == 0x102u);
  CHECK(capture.m_captured.m_customData[1].m_text == "Second");
  CHECK(capture.m_captured.m_hardOwnerHandle == 0x103u);
  CHECK(capture.m_raw[0].name == "DATALINK");
}

TEST_CASE("DXF DATALINK rejects out-of-range int16 fields",
          "[dxf][datalink][numeric]") {
  DataLinkCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDATALINK\n5\n2D\n330\nC\n100\nAcDbObject\n"
      "100\nAcDbDataLink\n170\n32768\n0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_datalink_out_of_range_int16.dxf"));
  CHECK(cap.m_callCount == 0);
}

TEST_CASE("DXF GEOMAPIMAGE preserves spec-backed display fields",
          "[dxf][geomapimage]") {
  GeoMapImageCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGEOMAPIMAGE\n5\n2E\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbGeomapImage\n"
      "90\n3\n10\n1.0\n20\n2.0\n30\n3.0\n"
      "13\n640.0\n23\n480.0\n70\n5\n280\n1\n"
      "281\n200\n282\n150\n283\n25\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_geomapimage_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_raw.size() == 1);
  CHECK(capture.m_captured.handle == 0x2Eu);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.m_classVersion == 3);
  CHECK(capture.m_captured.m_insertionPoint.x == 1.0);
  CHECK(capture.m_captured.m_insertionPoint.y == 2.0);
  CHECK(capture.m_captured.m_insertionPoint.z == 3.0);
  CHECK(capture.m_captured.m_imageSize.x == 640.0);
  CHECK(capture.m_captured.m_imageSize.y == 480.0);
  CHECK(capture.m_captured.m_displayProps == 5u);
  CHECK(capture.m_captured.m_clipping == true);
  CHECK(capture.m_captured.m_brightness == 200u);
  CHECK(capture.m_captured.m_contrast == 150u);
  CHECK(capture.m_captured.m_fade == 25u);
  CHECK(capture.m_raw[0].name == "GEOMAPIMAGE");
}

TEST_CASE("DXF INDEX preserves AcDbIndex last-updated time",
          "[dxf][index]") {
  IndexCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nINDEX\n5\n2F\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbIndex\n"
      "40\n2451545.12345678\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_index_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_raw.size() == 1);
  CHECK(capture.m_captured.handle == 0x2Fu);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.timestamp1 == 2451545u);
  CHECK(capture.m_captured.timestamp2 == 10666666u);
  CHECK(capture.m_raw[0].name == "INDEX");
}

TEST_CASE("DXF IDBUFFER preserves post-subclass object handles",
          "[dxf][idbuffer]") {
  IDBufferCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nIDBUFFER\n5\n30\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbIdBuffer\n"
      "70\n0\n330\n100\n330\n101\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_idbuffer_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_raw.size() == 1);
  CHECK(capture.m_captured.handle == 0x30u);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.classVersion == 0);
  REQUIRE(capture.m_captured.objIds.size() == 2u);
  CHECK(capture.m_captured.objIds[0] == 0x100u);
  CHECK(capture.m_captured.objIds[1] == 0x101u);
  CHECK(capture.m_raw[0].name == "IDBUFFER");
}

TEST_CASE("DXF IDBUFFER rejects null post-subclass handles",
          "[dxf][idbuffer][safety]") {
  IDBufferCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nIDBUFFER\n5\n31\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbIdBuffer\n"
      "70\n0\n330\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_idbuffer_invalid_handle.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, true));
  CHECK(capture.m_callCount == 0);
  CHECK(capture.m_raw.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF LAYER_INDEX preserves entries and timestamp",
          "[dxf][layerindex]") {
  LayerIndexCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nLAYER_INDEX\n5\n30\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbIndex\n"
      "40\n2451545.50000000\n100\nAcDbLayerIndex\n"
      "90\n2\n8\nLayerA\n360\n100\n"
      "90\n3\n8\nLayerB\n360\n101\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_layerindex_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_raw.size() == 1);
  REQUIRE(capture.m_captured.entries.size() == 2);
  CHECK(capture.m_captured.handle == 0x30u);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.timestamp1 == 2451545u);
  CHECK(capture.m_captured.timestamp2 == 43200000u);
  CHECK(capture.m_captured.entries[0].indexLong == 2);
  CHECK(capture.m_captured.entries[0].name == "LayerA");
  CHECK(capture.m_captured.entries[0].entryHandle == 0x100u);
  CHECK(capture.m_captured.entries[1].indexLong == 3);
  CHECK(capture.m_captured.entries[1].name == "LayerB");
  CHECK(capture.m_captured.entries[1].entryHandle == 0x101u);
  CHECK(capture.m_raw[0].name == "LAYER_INDEX");
}

TEST_CASE("DXF SPATIAL_INDEX preserves timestamp and raw body",
          "[dxf][spatialindex]") {
  SpatialIndexCapture capture;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSPATIAL_INDEX\n5\n31\n330\nC\n"
      "100\nAcDbObject\n100\nAcDbIndex\n100\nAcDbSpatialIndex\n"
      "40\n2451545.50000000\n40\n1.0\n310\nA1B2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, capture, "lc_spatialindex_typed_read.dxf", false);

  REQUIRE(capture.m_callCount == 1);
  REQUIRE(capture.m_raw.size() == 1);
  CHECK(capture.m_captured.handle == 0x31u);
  CHECK(capture.m_captured.parentHandle == 0xCu);
  CHECK(capture.m_captured.timestamp1 == 2451545u);
  CHECK(capture.m_captured.timestamp2 == 43200000u);
  CHECK(capture.m_raw[0].name == "SPATIAL_INDEX");
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
      "76\n0\n146\n0.0\n330\n50\n331\n51\n333\n52\n"
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
  CHECK(cap.m_captured.shadePlotHandle.ref == 0x52u);
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

TEST_CASE("DXF raw OBJECT handles are unique at full lexeme width",
          "[dxf][rawobject][malformed]") {
  RawObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1024\n"
      "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBWIDEOBJECTA\n5\n123456789ABCDEF0\n1\nfirst\n"
      "0\nACDBWIDEOBJECTB\n5\n123456789ABCDEF0\n1\nsecond\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_duplicate_wide_handle.dxf"));
  CHECK(cap.m_objects.size() == 1);
}

TEST_CASE("DXF raw handle admission resets between read sessions",
          "[dxf][rawobject][state]") {
  std::string dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBREUSABLEOBJECT\n5\n4A\n1\npayload\n"
      "0\nENDSEC\n0\nEOF\n";

  RawObjectCapture first;
  RawObjectCapture second;
  dxfRW reader("read-ascii-state-test.dxf");
  REQUIRE(reader.readAscii(&first, /*ext=*/true, dxf));
  REQUIRE(reader.readAscii(&second, /*ext=*/true, dxf));
  CHECK(first.m_objects.size() == 1);
  CHECK(second.m_objects.size() == 1);
}

TEST_CASE("DXF BREAKDATA/BREAKPOINTREF are raw-captured, not dropped (write-review 7.2)",
          "[dxf][rawobject][breakdata]") {
  // processBreakData/processBreakPointRef previously called only their typed
  // add* callbacks (no DXF writer for either), so both were silently dropped on
  // DXF->DXF. They now also raw-capture (mirroring processScale), and the
  // raw-net replay loop re-emits them.
  RawObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nBREAKDATA\n5\nA1\n330\nA0\n100\nAcDbBreakData\n90\n2\n331\nB5\n"
      "0\nBREAKPOINTREF\n5\nA2\n330\nA1\n100\nAcDbBreakPointRef\n90\n0\n10\n1.0\n20\n2.0\n30\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_breakdata.dxf");

  REQUIRE(cap.m_objects.size() == 2);
  const DRW_RawDxfObject &bd = cap.m_objects[0];
  CHECK(bd.name == "BREAKDATA");
  CHECK(bd.handle == 0xA1u);
  REQUIRE(bd.groups.size() == 5);  // 5, 330, 100, 90, 331
  CHECK(bd.groups[0].code() == 5);
  CHECK(bd.groups[2].code() == 100);
  const DRW_RawDxfObject &bp = cap.m_objects[1];
  CHECK(bp.name == "BREAKPOINTREF");
  CHECK(bp.handle == 0xA2u);
  CHECK(bp.groups.size() >= 5);
}

TEST_CASE("DXF unmodeled ENTITY is captured verbatim, not dropped (slice A4)", "[dxf][rawentity]") {
  RawEntityCapture cap;
  // The unknown record remains raw, while ATTDEF has a typed route so BLOCK
  // definitions can preserve their visible text and attribute fields.
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n8\n0\n5\n4A\n62\n3\n10\n1.0\n20\n2.0\n"
      "0\nATTDEF\n8\n0\n5\n4B\n10\n0.0\n20\n0.0\n40\n0.5\n1\ndef\n2\nTAG\n3\nPrompt?\n70\n0\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n5.0\n21\n5.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawentity.dxf");

  // WEIRDENT is raw; ATTDEF and LINE are typed.
  REQUIRE(cap.m_entities.size() == 1);
  CHECK(cap.m_entities[0].name == "WEIRDENT");
  CHECK(cap.m_entities[0].handle == 0x4Au);
  REQUIRE(cap.m_attdefs.size() == 1);
  CHECK(cap.m_attdefs[0].handle == 0x4Bu);
  CHECK(cap.m_attdefs[0].text == "def");
  CHECK(cap.m_attdefs[0].tag == "TAG");
  CHECK(cap.m_attdefs[0].prompt == "Prompt?");
  CHECK(cap.m_attdefs[0].attribFlags == 0);
}

TEST_CASE("DXF raw ENTITY preserves nested 102 groups", "[dxf][rawentity][102]") {
  RawEntityCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n5\n4A\n102\n{ACAD_REACTORS\n330\nD\n"
      "102\n{NESTED\n310\nA1B2\n102\n}\n102\n}\n"
      "8\n0\n70\n7\n"
      "0\nLINE\n5\n4B\n8\n0\n10\n0.0\n20\n0.0\n"
      "11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawentity_nested_102.dxf");

  REQUIRE(cap.m_entities.size() == 1);
  const DRW_RawDxfObject &entity = cap.m_entities.front();
  REQUIRE(entity.groups.size() == 9);
  CHECK(entity.groups[1].code() == 102);
  CHECK(entity.groups[1].c_str() == std::string("{ACAD_REACTORS"));
  CHECK(entity.groups[2].code() == 330);
  CHECK(entity.groups[3].code() == 102);
  CHECK(entity.groups[3].c_str() == std::string("{NESTED"));
  CHECK(entity.groups[4].code() == 310);
  CHECK(entity.groups[4].c_str() == std::string("A1B2"));
  CHECK(entity.groups[5].code() == 102);
  CHECK(entity.groups[5].c_str() == std::string("}"));
  CHECK(entity.groups[6].code() == 102);
  CHECK(entity.groups[6].c_str() == std::string("}"));
  CHECK(entity.groups[7].code() == 8);
  CHECK(entity.groups[8].code() == 70);
}

TEST_CASE("DXF raw carriers reject unbalanced application groups",
          "[dxf][raw][102][malformed]") {
  SECTION("OBJECTS") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nWEIRDOBJECT\n5\n4A\n102\n{ACAD_XDICTIONARY\n"
        "360\nD\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_object_unbalanced_102.dxf"));
    CHECK(cap.m_objects.empty());
  }

  SECTION("ENTITIES") {
    RawEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nWEIRDENTITY\n5\n4A\n102\n}\n"
        "8\n0\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_entity_unbalanced_102.dxf"));
    CHECK(cap.m_entities.empty());
  }

  SECTION("bare opening marker") {
    RawObjectCapture objects;
    const char *objectDxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nWEIRDOBJECT\n5\n4A\n102\n{\n"
        "102\n}\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(objectDxf, objects,
                           "lc_raw_object_bare_102_open.dxf"));
    CHECK(objects.m_objects.empty());

    RawEntityCapture entities;
    const char *entityDxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nWEIRDENTITY\n5\n4A\n102\n{\n"
        "102\n}\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(entityDxf, entities,
                           "lc_raw_entity_bare_102_open.dxf"));
    CHECK(entities.m_entities.empty());
  }
}

TEST_CASE("DXF ACAD_PROXY_ENTITY exposes typed payloads and references",
          "[dxf][proxy-entity]") {
  ProxyEntityCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nACAD_PROXY_ENTITY\n5\n25\n330\n1F\n"
      "8\nproxy-layer\n6\nCONTINUOUS\n62\n3\n370\n20\n"
      "48\n2.5\n60\n0\n67\n1\n347\nAA\n390\nAB\n"
      "420\n11259375\n430\nByLayer\n440\n128\n"
      "100\nAcDbEntity\n100\nAcDbProxyEntity\n"
      "90\n501\n91\n600\n92\n1\n310\nAB\n"
      "93\n16\n310\nCDEF\n96\n2\n310\n12\n311\n34\n"
      "95\n305397846\n70\n1\n330\n40\n340\n41\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_proxy_entity.dxf");

  REQUIRE(cap.m_entities.size() == 1);
  REQUIRE(cap.m_raw.size() == 1);
  const DRW_ProxyEntity &entity = cap.m_entities.front();
  CHECK(entity.handle == 0x25u);
  CHECK(entity.parentHandle == 0x1Fu);
  CHECK(entity.layer == "proxy-layer");
  CHECK(entity.lineType == "CONTINUOUS");
  CHECK(entity.color == 3);
  CHECK(entity.visible);
  CHECK(entity.space == DRW::PaperSpace);
  CHECK(entity.m_hasProxyCarrierId);
  CHECK(entity.m_proxyCarrierId == 501);
  CHECK(entity.m_hasProxyClassId);
  CHECK(entity.m_proxyClassId == 600);
  CHECK(entity.m_hasProxyDrawingFormat);
  CHECK(entity.m_proxyDrawingFormat == 0x12340056u);
  CHECK(entity.m_proxyDwgVersion == 0x0056u);
  CHECK(entity.m_proxyMaintenanceVersion == 0x1234u);
  CHECK(entity.m_hasFromDxf);
  CHECK(entity.m_fromDxf);
  CHECK(entity.m_hasProxyGraphicsByteSize);
  CHECK(entity.m_proxyGraphicsByteSize == 1u);
  CHECK(entity.proxyGraphics == std::string("\xAB", 1));
  CHECK(entity.m_hasEntityDataBitSize);
  CHECK(entity.m_entityDataBitSize == 16u);
  CHECK(entity.m_entityData == std::vector<std::uint8_t>{0xCD, 0xEF});
  CHECK(entity.m_hasUnknownDataByteSize);
  CHECK(entity.m_unknownDataByteSize == 2u);
  CHECK(entity.m_unknownData == std::vector<std::uint8_t>{0x12, 0x34});

  REQUIRE(entity.m_objectIdRefs.size() == 2);
  CHECK(entity.m_objectIdRefs[0].m_dxfCode == 330);
  CHECK(entity.m_objectIdRefs[0].m_handleCode == 2);
  CHECK(entity.m_objectIdRefs[0].m_handle == 0x40u);
  CHECK(entity.m_objectIdRefs[1].m_dxfCode == 340);
  CHECK(entity.m_objectIdRefs[1].m_handleCode == 3);
  CHECK(entity.m_objectIdRefs[1].m_handle == 0x41u);
  CHECK(cap.m_raw.front().parentHandle == 0x1Fu);
}

TEST_CASE("DXF ACAD_PROXY_OBJECT separates binary and object payloads",
          "[dxf][proxy-object]") {
  ProxyObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACAD_PROXY_OBJECT\n5\n51\n330\n50\n"
      "90\n701\n91\n702\n92\n2\n310\nA1B2\n"
      "93\n40\n310\nC0FFEE\n96\n2\n310\n1122\n"
      "311\n3344\n70\n1\n360\n88\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_proxy_object.dxf");

  REQUIRE(cap.m_objects.size() == 1);
  REQUIRE(cap.m_raw.size() == 1);
  const DRW_ProxyObject &object = cap.m_objects.front();
  CHECK(object.handle == 0x51u);
  CHECK(object.parentHandle == 0x50u);
  CHECK(object.m_hasProxyCarrierId);
  CHECK(object.m_proxyCarrierId == 701);
  CHECK(object.m_hasProxyClassId);
  CHECK(object.m_proxyClassId == 702);
  CHECK(object.m_binaryData == std::vector<std::uint8_t>{0xA1, 0xB2});
  CHECK(object.m_hasObjectDataBitSize);
  CHECK(object.m_objectDataBitSize == 40u);
  CHECK(object.m_objectData ==
        std::vector<std::uint8_t>{0xC0, 0xFF, 0xEE, 0x33, 0x44});
  CHECK(object.m_unknownData == std::vector<std::uint8_t>{0x11, 0x22});
  REQUIRE(object.m_objectIdRefs.size() == 1);
  CHECK(object.m_objectIdRefs.front().m_dxfCode == 360);
  CHECK(object.m_objectIdRefs.front().m_handleCode == 5);
  CHECK(object.m_objectIdRefs.front().m_handle == 0x88u);
  CHECK(cap.m_raw.front().parentHandle == 0x50u);
}

TEST_CASE("DXF proxy carriers preserve common control-group references",
          "[dxf][proxy][102][references]") {
  SECTION("entity") {
    ProxyEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nACAD_PROXY_ENTITY\n5\n25\n330\n1F\n"
        "102\n{ACAD_REACTORS\n330\n40\n"
        "102\n{NESTED\n330\n41\n102\n}\n102\n}\n"
        "102\n{ACAD_XDICTIONARY\n360\n42\n102\n}\n"
        "90\n1\n0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_proxy_entity_common_refs.dxf");

    REQUIRE(cap.m_entities.size() == 1);
    const DRW_ProxyEntity &entity = cap.m_entities.front();
    CHECK(entity.reactorHandles == std::vector<std::uint32_t>{0x40u});
    CHECK(entity.xDictHandle == 0x42u);
    CHECK(entity.m_objectIdRefs.empty());
  }

  SECTION("object") {
    ProxyObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACAD_PROXY_OBJECT\n5\n51\n330\n50\n"
        "102\n{ACAD_REACTORS\n330\n60\n"
        "102\n{NESTED\n330\n61\n102\n}\n102\n}\n"
        "102\n{ACAD_XDICTIONARY\n360\n62\n102\n}\n"
        "90\n701\n0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_proxy_object_common_refs.dxf");

    REQUIRE(cap.m_objects.size() == 1);
    const DRW_ProxyObject &object = cap.m_objects.front();
    CHECK(object.reactorHandles == std::vector<std::uint32_t>{0x60u});
    CHECK(object.xDictHandle == 0x62u);
    CHECK(object.m_objectIdRefs.empty());
  }
}

TEST_CASE("DXF proxy binary payload errors publish no partial carrier",
          "[dxf][proxy][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_proxy_entity_bad.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nACAD_PROXY_ENTITY\n5\n25\n90\n1\n92\n1\n"
           "310\nABC\n0\nENDSEC\n0\nEOF\n";
  }

  ProxyEntityCapture cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  CHECK(cap.m_entities.empty());
  CHECK(cap.m_raw.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF proxy declared payload sizes are transactional",
          "[dxf][proxy][malformed]") {
  SECTION("graphics byte count cannot exceed captured chunks") {
    ProxyEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nACAD_PROXY_ENTITY\n5\n25\n90\n1\n92\n2\n310\nAB\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_proxy_short_graphics.dxf"));
    CHECK(cap.m_entities.empty());
    CHECK(cap.m_raw.empty());
  }

  SECTION("entity-data bit count must match the encoded byte stream") {
    ProxyObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACAD_PROXY_OBJECT\n5\n51\n90\n1\n93\n17\n"
        "310\nA1B2\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_proxy_short_object_data.dxf"));
    CHECK(cap.m_objects.empty());
    CHECK(cap.m_raw.empty());
  }

  SECTION("version-specific duplicate length fields are rejected") {
    ProxyEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nACAD_PROXY_ENTITY\n5\n25\n90\n1\n92\n1\n"
        "160\n1\n310\nAB\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_proxy_duplicate_graphics_size.dxf"));
    CHECK(cap.m_entities.empty());
    CHECK(cap.m_raw.empty());
  }
}

TEST_CASE("DXF proxy handle errors publish no partial carrier",
          "[dxf][proxy][malformed][handle]") {
  SECTION("proxy entity hard-pointer") {
    ProxyEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nACAD_PROXY_ENTITY\n5\n25\n390\nZZ\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_proxy_bad_entity_handle.dxf"));
    CHECK(cap.m_entities.empty());
    CHECK(cap.m_raw.empty());
  }

  SECTION("proxy object material pointer") {
    ProxyObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACAD_PROXY_OBJECT\n5\n51\n347\nQ1\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_proxy_bad_object_handle.dxf"));
    CHECK(cap.m_objects.empty());
    CHECK(cap.m_raw.empty());
  }
}

TEST_CASE("DXF raw ENTITY does not publish before top-level ENDBLK",
          "[dxf][rawentity][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_entity_endblk.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nENTITIES\n"
           "0\nWEIRDENT\n5\n4A\n8\n0\n10\n1\n20\n2\n"
           "0\nENDBLK\n5\n4B\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  RawEntityCapture cap;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, /*ext=*/true));
  CHECK(cap.m_entities.empty());
  std::filesystem::remove(path);
}

TEST_CASE("DXF raw ENTITY does not publish before an empty boundary",
          "[dxf][rawentity][malformed]") {
  RawEntityCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n5\n4A\n8\n0\n10\n1\n20\n2\n"
      "0\n\n"
      "0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_entity_empty_boundary.dxf"));
  CHECK(cap.m_entities.empty());
}

TEST_CASE("DXF rejects malformed handle-valued raw groups transactionally",
          "[dxf][raw][malformed][handle]") {
  SECTION("self handle code 5") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBBADHANDLE\n5\nGZ\n1\npayload\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_bad_handle_5.dxf"));
    CHECK(cap.m_objects.empty());
  }

  SECTION("owner handle code 330") {
    RawEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nWEIRDENT\n5\n1A\n330\nnot-hex\n8\n0\n"
        "10\n1.0\n20\n2.0\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_bad_handle_330.dxf"));
    CHECK(cap.m_entities.empty());
  }

  SECTION("hard-pointer code 481") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBBADPOINTER\n5\n1A\n481\nZZ\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_bad_handle_481.dxf"));
    CHECK(cap.m_objects.empty());
  }

  SECTION("odd-length binary chunk") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBBADBINARY\n5\n1B\n310\nA1B\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_bad_binary_odd.dxf"));
    CHECK(cap.m_objects.empty());
  }

  SECTION("non-hex extended-data binary chunk") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBBADXDATA\n5\n1C\n1004\nG1\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_bad_binary_hex.dxf"));
    CHECK(cap.m_objects.empty());
  }
}

TEST_CASE("DXF raw carriers require self handles in modern files",
          "[dxf][raw][malformed][handle][version]") {
  SECTION("modern raw OBJECT") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
        "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBNOHANDLE\n1\npayload\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_object_missing_self_handle.dxf"));
    CHECK(cap.m_objects.empty());
  }

  SECTION("modern raw ENTITY") {
    RawEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nWEIRDENT\n8\n0\n10\n1\n20\n2\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_entity_missing_self_handle.dxf"));
    CHECK(cap.m_entities.empty());
  }

  SECTION("legacy raw ENTITY remains tolerant") {
    RawEntityCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nWEIRDENT\n8\n0\n10\n1\n20\n2\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK(tryReadDxf(dxf, cap, "lc_raw_entity_legacy_no_handle.dxf"));
    REQUIRE(cap.m_entities.size() == 1);
    CHECK(cap.m_entities.front().handle == 0);
  }
}

TEST_CASE("DXF rejects malformed typed pointer groups transactionally",
          "[dxf][malformed][handle]") {
  SECTION("typed object owner") {
    class Capture final : public StubInterface {
    public:
      int calls = 0;
      void addObjectPtr(const DRW_ObjectPtr &) override { ++calls; }
    } cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nOBJECTPTR\n5\n2F3\n330\nnot-hex\n"
        "100\nAcDbObjectPtr\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_typed_bad_owner.dxf"));
    CHECK(cap.calls == 0);
  }

  SECTION("typed XRECORD xdata handle") {
    XRecordCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nXRECORD\n5\n30\n100\nAcDbXrecord\n1005\nZZ\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_typed_bad_xdata_handle.dxf"));
    CHECK(cap.m_callCount == 0);
  }
}

TEST_CASE("DXF validates context-sensitive typed code-5 handles",
          "[dxf][malformed][handle]") {
  SECTION("entity self handle") {
    class Capture final : public StubInterface {
    public:
      int calls = 0;
      void addLine(const DRW_Line &) override { ++calls; }
    } cap;
    const char *dxf =
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\nGZ\n8\n0\n10\n1\n20\n2\n11\n3\n21\n4\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_typed_bad_entity_handle.dxf"));
    CHECK(cap.calls == 0);
  }

  SECTION("table-record self handle") {
    LayerCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nTABLES\n"
        "0\nTABLE\n2\nLAYER\n70\n1\n"
        "0\nLAYER\n5\nGZ\n330\n2\n100\nAcDbSymbolTableRecord\n"
        "100\nAcDbLayerTableRecord\n2\nBROKEN\n70\n0\n62\n7\n6\nCONTINUOUS\n"
        "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_typed_bad_table_handle.dxf"));
    CHECK(cap.m_callCount == 0);
  }
}

TEST_CASE("DXF opaque object shells reject malformed self handles",
          "[dxf][malformed][handle][shell]") {
  SECTION("dynamic-block shell") {
    DynamicBlockCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nBLOCKPOINTPARAMETER\n5\nGZ\n90\n1\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_dynamic_bad_self_handle.dxf"));
    CHECK(cap.m_callCount == 0);
    CHECK(cap.m_raw.empty());
  }

  SECTION("ACSH shell") {
    AcShHistoryCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACSH_BOX_CLASS\n5\nGZ\n90\n1\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_acsh_bad_self_handle.dxf"));
    CHECK(cap.m_callCount == 0);
    CHECK(cap.m_raw.empty());
  }
}

TEST_CASE("DXF raw-backed shells reject unclosed application groups",
          "[dxf][malformed][shell][application-group]") {
  SECTION("dynamic-block shell") {
    DynamicBlockCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nBLOCKPOINTPARAMETER\n5\n40\n102\n{ACAD_REACTORS\n330\n41\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_dynamic_unclosed_app_group.dxf"));
    CHECK(cap.m_callCount == 0);
    CHECK(cap.m_raw.empty());
  }

  SECTION("ACSH shell") {
    AcShHistoryCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACSH_BOX_CLASS\n5\n50\n102\n{ACAD_REACTORS\n330\n51\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_acsh_unclosed_app_group.dxf"));
    CHECK(cap.m_callCount == 0);
    CHECK(cap.m_raw.empty());
  }
}

TEST_CASE("DXF raw ENTITY accepts ENDBLK inside a BLOCK",
          "[dxf][rawentity][block]") {
  RawEntityCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n20\n8\n0\n2\nB1\n70\n0\n"
      "0\nWEIRDENT\n5\n21\n8\n0\n10\n1\n20\n2\n"
      "0\nENDBLK\n5\n22\n8\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_raw_entity_block.dxf");

  REQUIRE(cap.m_entities.size() == 1);
  CHECK(cap.m_entities.front().name == "WEIRDENT");
  CHECK(cap.m_entities.front().handle == 0x21u);
}

TEST_CASE("DXF raw ENTITY replay preserves source version and lexemes",
          "[dxf][rawentity][rawcapture][version]") {
  RawEntityCapture captured;
  const char *dxf =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC1024\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n5\n4A\n8\n0\n70\n0007\n40\n-0.000E+0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, captured, "lc_rawentity_version.dxf");

  REQUIRE(captured.m_entities.size() == 1);
  const DRW_RawDxfObject &source = captured.m_entities.front();
  CHECK(source.m_version == DRW::AC1024);
  REQUIRE(source.rawValues.size() == source.groups.size());
  CHECK(source.rawValues[2] == "0007");
  CHECK(source.rawValues[3] == "-0.000E+0");

  const auto sameVersionPath =
      std::filesystem::temp_directory_path() / "lc_rawentity_version_rt.dxf";
  std::filesystem::remove(sameVersionPath);
  RawEntityEmitter emitter;
  emitter.m_entity = source;
  {
    dxfRW writer(sameVersionPath.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1024, false));
  }

  RawEntityCapture replayed;
  {
    dxfRW reader(sameVersionPath.string().c_str());
    REQUIRE(reader.read(&replayed, /*ext=*/true));
  }
  REQUIRE(replayed.m_entities.size() == 1);
  CHECK(replayed.m_entities.front().m_version == DRW::AC1024);
  CHECK(replayed.m_entities.front().rawValues == source.rawValues);

  const auto crossVersionPath =
      std::filesystem::temp_directory_path() / "lc_rawentity_version_bad.dxf";
  std::filesystem::remove(crossVersionPath);
  RawEntityEmitter crossVersion;
  crossVersion.m_entity = source;
  {
    dxfRW writer(crossVersionPath.string().c_str());
    crossVersion.m_rw = &writer;
    CHECK_FALSE(writer.write(&crossVersion, DRW::AC1021, false));
    CHECK_FALSE(crossVersion.m_writeResult);
  }

  std::filesystem::remove(sameVersionPath);
  std::filesystem::remove(crossVersionPath);
}

TEST_CASE("DXF raw ENTITY provenance covers legacy and binary sources",
          "[dxf][rawentity][rawcapture][version][binary]") {
  RawEntityCapture legacy;
  const char *legacyDxf =
      "0\nSECTION\n2\nHEADER\n"
      "9\n$ACADVER\n1\nAC1009\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLEGACYENT\n5\n2A\n70\n0007\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(legacyDxf, legacy, "lc_rawentity_legacy.dxf");

  REQUIRE(legacy.m_entities.size() == 1);
  const DRW_RawDxfObject &legacyEntity = legacy.m_entities.front();
  CHECK(legacyEntity.m_version == DRW::AC1009);
  REQUIRE(legacyEntity.rawValues.size() == legacyEntity.groups.size());
  CHECK(legacyEntity.rawValues[1] == "0007");

  RawEntityCapture binary;
  const auto binaryPath =
      std::filesystem::temp_directory_path() / "lc_rawentity_binary.dxf";
  const auto binaryRoundTripPath =
      std::filesystem::temp_directory_path() / "lc_rawentity_binary_rt.dxf";
  const auto binaryCrossVersionPath =
      std::filesystem::temp_directory_path() / "lc_rawentity_binary_bad.dxf";
  std::filesystem::remove(binaryPath);
  std::filesystem::remove(binaryRoundTripPath);
  std::filesystem::remove(binaryCrossVersionPath);

  RawEntityEmitter emitter;
  emitter.m_entity.name = "BINARYENT";
  emitter.m_entity.groups.emplace_back(5, std::string{"4A"});
  emitter.m_entity.groups.emplace_back(8, std::string{"0"});
  emitter.m_entity.groups.emplace_back(70, static_cast<std::int32_t>(7));
  emitter.m_entity.groups.emplace_back(40, 2.5);
  {
    dxfRW writer(binaryPath.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, true));
  }

  {
    dxfRW reader(binaryPath.string().c_str());
    REQUIRE(reader.read(&binary, /*ext=*/true));
  }
  REQUIRE(binary.m_entities.size() == 1);
  const DRW_RawDxfObject &binaryEntity = binary.m_entities.front();
  CHECK(binaryEntity.m_version == DRW::AC1021);
  REQUIRE(binaryEntity.rawValues.size() == binaryEntity.groups.size());
  CHECK(std::all_of(binaryEntity.rawValues.begin(), binaryEntity.rawValues.end(),
                    [](const UTF8STRING &value) { return value.empty(); }));
  CHECK(binaryEntity.groups[2].i_val() == 7);
  CHECK(binaryEntity.groups[3].d_val() == 2.5);

  RawEntityEmitter replay;
  replay.m_entity = binaryEntity;
  {
    dxfRW writer(binaryRoundTripPath.string().c_str());
    replay.m_rw = &writer;
    REQUIRE(writer.write(&replay, DRW::AC1021, true));
  }
  RawEntityCapture replayed;
  {
    dxfRW reader(binaryRoundTripPath.string().c_str());
    REQUIRE(reader.read(&replayed, /*ext=*/true));
  }
  REQUIRE(replayed.m_entities.size() == 1);
  CHECK(replayed.m_entities.front().m_version == DRW::AC1021);
  CHECK(replayed.m_entities.front().groups[2].i_val() == 7);
  CHECK(replayed.m_entities.front().groups[3].d_val() == 2.5);

  RawEntityEmitter crossVersion;
  crossVersion.m_entity = binaryEntity;
  {
    dxfRW writer(binaryCrossVersionPath.string().c_str());
    crossVersion.m_rw = &writer;
    CHECK_FALSE(writer.write(&crossVersion, DRW::AC1009, true));
    CHECK_FALSE(crossVersion.m_writeResult);
  }

  std::filesystem::remove(binaryPath);
  std::filesystem::remove(binaryRoundTripPath);
  std::filesystem::remove(binaryCrossVersionPath);
}

TEST_CASE("DXF raw ENTITY version matrix preserves source provenance",
          "[dxf][rawentity][rawcapture][version][cross-version]") {
  struct VersionFixture {
    DRW::Version version;
    const char *acadver;
  };
  constexpr VersionFixture fixtures[] = {
      {DRW::AC1009, "AC1009"}, {DRW::AC1014, "AC1014"},
      {DRW::AC1015, "AC1015"}, {DRW::AC1018, "AC1018"},
      {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
      {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"},
  };

  for (const VersionFixture &fixture : fixtures) {
    RawEntityCapture cap;
    const std::string dxf =
        std::string("0\nSECTION\n2\nHEADER\n")
        + "9\n$ACADVER\n1\n" + fixture.acadver
        + "\n0\nENDSEC\n"
          "0\nSECTION\n2\nENTITIES\n"
          "0\nWEIRDENT\n5\n4A\n8\n0\n70\n0007\n"
          "0\nENDSEC\n0\nEOF\n";
    const std::string name =
        std::string("lc_rawentity_version_matrix_") + fixture.acadver
        + ".dxf";
    readDxf(dxf, cap, name.c_str());

    REQUIRE(cap.m_entities.size() == 1);
    const DRW_RawDxfObject &entity = cap.m_entities.front();
    CHECK(entity.m_version == fixture.version);
    REQUIRE(entity.groups.size() == 3);
    CHECK(entity.handle == 0x4Au);
    CHECK(entity.groups[2].i_val() == 7);
  }
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

TEST_CASE("DXF raw carriers preserve bounded wide handle lexemes",
          "[dxf][rawcapture][wide-handle]") {
  const std::string header =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n";
  const std::string suffix = "0\nENDSEC\n0\nEOF\n";

  SECTION("unknown OBJECTS records round-trip") {
    const std::string source = header +
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBWIDEHANDLE\n5\n100000001\n330\n100000002\n"
        "481\n100000003\n100\nAcDbWideHandle\n1\npayload\n" + suffix;
    RawObjectCapture captured;
    readDxf(source, captured, "lc_raw_object_wide_handle.dxf");

    REQUIRE(captured.m_objects.size() == 1);
    const DRW_RawDxfObject &object = captured.m_objects.front();
    // The legacy fields intentionally remain 32-bit; the raw groups carry
    // the authoritative text for a wider handle.
    CHECK(object.handle == DRW::NoHandle);
    CHECK(object.parentHandle == DRW::NoHandle);
    REQUIRE(object.groups.size() == 5);
    CHECK(std::string(object.groups[0].c_str()) == "100000001");
    CHECK(std::string(object.groups[1].c_str()) == "100000002");
    CHECK(std::string(object.groups[2].c_str()) == "100000003");

    const auto path = std::filesystem::temp_directory_path() /
                      "lc_raw_object_wide_handle_rt.dxf";
    std::filesystem::remove(path);
    RawObjectEmitter emitter;
    emitter.m_obj = object;
    {
      dxfRW writer(path.string().c_str());
      emitter.m_rw = &writer;
      REQUIRE(writer.write(&emitter, DRW::AC1021, false));
      CHECK(emitter.m_writeResult);
    }

    RawObjectCapture replayed;
    {
      dxfRW reader(path.string().c_str());
      REQUIRE(reader.read(&replayed, true));
    }
    REQUIRE(replayed.m_objects.size() == 1);
    const auto &roundTrip = replayed.m_objects.front();
    REQUIRE(roundTrip.groups.size() == object.groups.size());
    CHECK(std::string(roundTrip.groups[0].c_str()) == "100000001");
    CHECK(std::string(roundTrip.groups[1].c_str()) == "100000002");
    CHECK(std::string(roundTrip.groups[2].c_str()) == "100000003");
    std::filesystem::remove(path);

    const auto binaryPath = std::filesystem::temp_directory_path() /
                            "lc_raw_object_wide_handle_rt.bin";
    std::filesystem::remove(binaryPath);
    {
      dxfRW writer(binaryPath.string().c_str());
      emitter.m_rw = &writer;
      REQUIRE(writer.write(&emitter, DRW::AC1021, true));
      CHECK(emitter.m_writeResult);
    }
    RawObjectCapture binaryReplayed;
    {
      dxfRW reader(binaryPath.string().c_str());
      REQUIRE(reader.read(&binaryReplayed, true));
    }
    REQUIRE(binaryReplayed.m_objects.size() == 1);
    const auto &binaryRoundTrip = binaryReplayed.m_objects.front();
    REQUIRE(binaryRoundTrip.groups.size() == object.groups.size());
    CHECK(std::string(binaryRoundTrip.groups[0].c_str()) == "100000001");
    CHECK(std::string(binaryRoundTrip.groups[1].c_str()) == "100000002");
    CHECK(std::string(binaryRoundTrip.groups[2].c_str()) == "100000003");
    std::filesystem::remove(binaryPath);
  }

  SECTION("unknown ENTITIES records retain wide self handles") {
    const std::string source = header +
        "0\nSECTION\n2\nENTITIES\n"
        "0\nWIDEENTITY\n5\n100000001\n330\n100000002\n8\n0\n" + suffix;
    RawEntityCapture captured;
    readDxf(source, captured, "lc_raw_entity_wide_handle.dxf");
    REQUIRE(captured.m_entities.size() == 1);
    const auto &entity = captured.m_entities.front();
    CHECK(entity.handle == DRW::NoHandle);
    REQUIRE(entity.groups.size() == 3);
    CHECK(std::string(entity.groups[0].c_str()) == "100000001");
    CHECK(std::string(entity.groups[1].c_str()) == "100000002");
  }

  SECTION("unknown sections retain wide reference handles") {
    const std::string source = header +
        "0\nSECTION\n2\nCUSTOM_SECTION\n"
        "105\n100000001\n330\n100000002\n" + suffix;
    RawSectionCapture captured;
    readDxf(source, captured, "lc_raw_section_wide_handle.dxf");
    REQUIRE(captured.m_sections.size() == 1);
    const auto &section = captured.m_sections.front();
    REQUIRE(section.m_groups.size() == 2);
    CHECK(std::string(section.m_groups[0].c_str()) == "100000001");
    CHECK(std::string(section.m_groups[1].c_str()) == "100000002");
  }

  SECTION("typed records and oversize raw lexemes are rejected") {
    LineCapture typed;
    CHECK_FALSE(tryReadDxf(header +
        "0\nSECTION\n2\nENTITIES\n"
        "0\nLINE\n5\n100000001\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n" + suffix,
        typed, "lc_typed_wide_handle.dxf"));
    CHECK(typed.m_callCount == 0);

    RawObjectCapture oversize;
    CHECK_FALSE(tryReadDxf(header +
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBWIDEHANDLE\n5\n10000000000000000\n" + suffix,
        oversize, "lc_raw_object_oversize_handle.dxf"));
    CHECK(oversize.m_objects.empty());
  }
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

TEST_CASE("DXF 64-bit integer groups preserve signed values and bounds",
          "[dxf][rawobject][rawcapture][numeric][malformed]") {
  SECTION("ASCII signed limits") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBSIGNED64\n5\n3B\n"
        "160\n-9223372036854775808\n"
        "161\n9223372036854775807\n"
        "0\nENDSEC\n0\nEOF\n";
    REQUIRE(tryReadDxf(dxf, cap, "lc_raw_signed64_limits.dxf"));
    REQUIRE(cap.m_objects.size() == 1);
    const DRW_RawDxfObject &object = cap.m_objects.front();
    REQUIRE(object.groups.size() == 3);
    CHECK(object.groups[1].type() == DRW_Variant::INTEGER64);
    CHECK(object.groups[1].i64_val() == std::numeric_limits<std::int64_t>::min());
    CHECK(object.groups[2].type() == DRW_Variant::INTEGER64);
    CHECK(object.groups[2].i64_val() == std::numeric_limits<std::int64_t>::max());
  }

  SECTION("ASCII overflow is transactional") {
    RawObjectCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nACDBSIGNED64\n5\n3B\n"
        "160\n9223372036854775808\n"
        "0\nENDSEC\n0\nEOF\n";
    CHECK_FALSE(tryReadDxf(dxf, cap, "lc_raw_signed64_overflow.dxf"));
    CHECK(cap.m_objects.empty());
  }

  SECTION("binary signed limits") {
    const auto path =
        std::filesystem::temp_directory_path() / "lc_raw_signed64_binary.dxf";
    std::filesystem::remove(path);

    RawObjectEmitter emitter;
    emitter.m_obj.name = "ACDBSIGNED64";
    emitter.m_obj.groups.emplace_back(5, std::string{"3B"});
    emitter.m_obj.groups.emplace_back(
        160, std::numeric_limits<std::int64_t>::min());
    emitter.m_obj.groups.emplace_back(
        161, std::numeric_limits<std::int64_t>::max());
    {
      dxfRW writer(path.string().c_str());
      emitter.m_rw = &writer;
      REQUIRE(writer.write(&emitter, DRW::AC1021, true));
    }

    RawObjectCapture captured;
    {
      dxfRW reader(path.string().c_str());
      REQUIRE(reader.read(&captured, /*ext=*/true));
    }
    REQUIRE(captured.m_objects.size() == 1);
    const DRW_RawDxfObject &object = captured.m_objects.front();
    REQUIRE(object.groups.size() == 3);
    CHECK(object.groups[1].i64_val() == std::numeric_limits<std::int64_t>::min());
    CHECK(object.groups[2].i64_val() == std::numeric_limits<std::int64_t>::max());
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF raw capture matches reader types in the reserved 240 range",
          "[dxf][rawobject][rawcapture][types]") {
  RawObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBWFDIAG\n5\n3B\n240\n2.5\n260\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawcapture_reserved_240.dxf");

  REQUIRE(cap.m_objects.size() == 1);
  const DRW_RawDxfObject &object = cap.m_objects.front();
  REQUIRE(object.groups.size() == 3);
  CHECK(object.groups[1].code() == 240);
  CHECK(object.groups[1].type() == DRW_Variant::DOUBLE);
  CHECK(object.groups[1].d_val() == 2.5);
  CHECK(object.groups[2].code() == 260);
  CHECK(object.groups[2].type() == DRW_Variant::INTEGER);
  CHECK(object.groups[2].i_val() == 1);
}

TEST_CASE("DXF raw capture preserves reserved string ranges",
          "[dxf][rawobject][rawcapture][types]") {
  RawObjectCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBWFDIAG\n5\n3B\n"
      "80\nreserved-80\n103\nreserved-103\n"
      "150\nreserved-150\n180\nreserved-180\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_rawcapture_reserved_strings.dxf");

  REQUIRE(cap.m_objects.size() == 1);
  const DRW_RawDxfObject &object = cap.m_objects.front();
  REQUIRE(object.groups.size() == 5);
  CHECK(object.groups[1].type() == DRW_Variant::STRING);
  CHECK(object.groups[1].c_str() == std::string("reserved-80"));
  CHECK(object.groups[2].type() == DRW_Variant::STRING);
  CHECK(object.groups[2].c_str() == std::string("reserved-103"));
  CHECK(object.groups[3].type() == DRW_Variant::STRING);
  CHECK(object.groups[3].c_str() == std::string("reserved-150"));
  CHECK(object.groups[4].type() == DRW_Variant::STRING);
  CHECK(object.groups[4].c_str() == std::string("reserved-180"));
}

TEST_CASE("DXF raw OBJECTs preserve ASCII numeric lexemes",
          "[dxf][rawobject][rawcapture][lexeme]") {
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBLEXEME\n5\n3B\n330\n29\n100\nAcDbLexeme\n"
      "70\n0007\n90\n+000123\n160\n0000000000000042\n"
      "40\n-0.000E+0\n290\n01\n"
      "0\nENDSEC\n0\nEOF\n";
  RawObjectCapture captured;
  readDxf(dxf, captured, "lc_raw_ascii_lexeme.dxf");

  REQUIRE(captured.m_objects.size() == 1);
  const DRW_RawDxfObject &object = captured.m_objects.front();
  REQUIRE(object.groups.size() == 8);
  REQUIRE(object.rawValues.size() == object.groups.size());
  CHECK(object.rawValues[3] == "0007");
  CHECK(object.rawValues[4] == "+000123");
  CHECK(object.rawValues[5] == "0000000000000042");
  CHECK(object.rawValues[6] == "-0.000E+0");
  CHECK(object.rawValues[7] == "01");

  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_ascii_lexeme_rt.dxf";
  std::filesystem::remove(path);
  RawObjectEmitter emitter;
  emitter.m_obj = object;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }
  const std::string output = slurp(path);
  CHECK(output.find("0007") != std::string::npos);
  CHECK(output.find("+000123") != std::string::npos);
  CHECK(output.find("-0.000E+0") != std::string::npos);

  RawObjectCapture replayed;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&replayed, /*ext=*/true));
  }
  REQUIRE(replayed.m_objects.size() == 1);
  CHECK(replayed.m_objects.front().rawValues == object.rawValues);
  std::filesystem::remove(path);
}

TEST_CASE("DXF binary raw OBJECT chunks round-trip as hex pairs",
          "[dxf][rawobject][binary]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_binary_object.dxf";
  const auto roundTripPath =
      std::filesystem::temp_directory_path() / "lc_raw_binary_object_rt.dxf";
  std::filesystem::remove(path);
  std::filesystem::remove(roundTripPath);

  RawObjectEmitter emitter;
  emitter.m_obj.name = "ACDBBINARY";
  emitter.m_obj.groups.emplace_back(5, std::string{"3B"});
  emitter.m_obj.groups.emplace_back(330, std::string{"29"});
  emitter.m_obj.groups.emplace_back(100, std::string{"AcDbBinary"});
  emitter.m_obj.groups.emplace_back(310, std::string{"00FF10"});
  emitter.m_obj.groups.emplace_back(310, std::string{"A5"});
  emitter.m_obj.groups.emplace_back(1004, std::string{"1234"});

  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, true));
  }

  RawObjectCapture captured;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&captured, /*ext=*/true));
  }
  REQUIRE(captured.m_objects.size() == 1);
  const DRW_RawDxfObject &object = captured.m_objects.front();
  REQUIRE(object.groups.size() == 6);
  CHECK(object.groups[3].type() == DRW_Variant::STRING);
  CHECK(std::string(object.groups[3].c_str()) == "00FF10");
  CHECK(std::string(object.groups[4].c_str()) == "A5");
  CHECK(std::string(object.groups[5].c_str()) == "1234");

  RawObjectEmitter replay;
  replay.m_obj = object;
  {
    dxfRW writer(roundTripPath.string().c_str());
    replay.m_rw = &writer;
    REQUIRE(writer.write(&replay, DRW::AC1021, true));
  }

  RawObjectCapture replayed;
  {
    dxfRW reader(roundTripPath.string().c_str());
    REQUIRE(reader.read(&replayed, /*ext=*/true));
  }
  REQUIRE(replayed.m_objects.size() == 1);
  REQUIRE(replayed.m_objects.front().groups.size() == 6);
  CHECK(std::string(replayed.m_objects.front().groups[3].c_str()) == "00FF10");
  CHECK(std::string(replayed.m_objects.front().groups[4].c_str()) == "A5");
  CHECK(std::string(replayed.m_objects.front().groups[5].c_str()) == "1234");

  std::filesystem::remove(path);
  std::filesystem::remove(roundTripPath);
}

TEST_CASE("DXF binary raw OBJECT typed values preserve code widths",
          "[dxf][rawobject][binary][typed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_binary_typed.dxf";
  const auto roundTripPath =
      std::filesystem::temp_directory_path() / "lc_raw_binary_typed_rt.dxf";
  const auto asciiPath =
      std::filesystem::temp_directory_path() / "lc_raw_binary_typed_ascii.dxf";
  std::filesystem::remove(path);
  std::filesystem::remove(roundTripPath);
  std::filesystem::remove(asciiPath);

  RawObjectEmitter emitter;
  emitter.m_obj.name = "ACDBTYPEDBINARY";
  emitter.m_obj.groups.emplace_back(5, std::string{"3B"});
  emitter.m_obj.groups.emplace_back(330, std::string{"29"});
  emitter.m_obj.groups.emplace_back(100, std::string{"AcDbTypedBinary"});
  emitter.m_obj.groups.emplace_back(70, static_cast<std::int32_t>(7));
  emitter.m_obj.groups.emplace_back(90, static_cast<std::int32_t>(123456));
  emitter.m_obj.groups.emplace_back(
      160, static_cast<std::int64_t>(0x0102030405060708LL));
  emitter.m_obj.groups.emplace_back(40, 2.5);
  emitter.m_obj.groups.emplace_back(290, static_cast<std::int32_t>(1));
  emitter.m_obj.groups.emplace_back(1071, static_cast<std::int32_t>(0x10203040));
  emitter.m_obj.groups.emplace_back(481, std::string{"3B"});
  emitter.m_obj.groups.emplace_back(1005, std::string{"29"});

  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, true));
  }

  RawObjectCapture captured;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&captured, /*ext=*/true));
  }
  REQUIRE(captured.m_objects.size() == 1);
  const DRW_RawDxfObject &object = captured.m_objects.front();
  CHECK_FALSE(object.hasRawValues);
  REQUIRE(object.groups.size() == 11);
  CHECK(object.groups[3].i_val() == 7);
  CHECK(object.groups[4].i_val() == 123456);
  CHECK(object.groups[5].i64_val() == 0x0102030405060708LL);
  CHECK(object.groups[6].d_val() == 2.5);
  CHECK(object.groups[7].i_val() == 1);
  CHECK(object.groups[8].i_val() == 0x10203040);
  CHECK(std::string(object.groups[9].c_str()) == "3B");
  CHECK(std::string(object.groups[10].c_str()) == "29");

  RawObjectEmitter replay;
  replay.m_obj = object;
  {
    dxfRW writer(roundTripPath.string().c_str());
    replay.m_rw = &writer;
    REQUIRE(writer.write(&replay, DRW::AC1021, true));
  }
  RawObjectCapture replayed;
  {
    dxfRW reader(roundTripPath.string().c_str());
    REQUIRE(reader.read(&replayed, /*ext=*/true));
  }
  REQUIRE(replayed.m_objects.size() == 1);
  const DRW_RawDxfObject &roundTrip = replayed.m_objects.front();
  REQUIRE(roundTrip.groups.size() == 11);
  CHECK(roundTrip.groups[3].i_val() == 7);
  CHECK(roundTrip.groups[4].i_val() == 123456);
  CHECK(roundTrip.groups[5].i64_val() == 0x0102030405060708LL);
  CHECK(roundTrip.groups[6].d_val() == 2.5);
  CHECK(roundTrip.groups[7].i_val() == 1);
  CHECK(roundTrip.groups[8].i_val() == 0x10203040);

  RawObjectEmitter asciiReplay;
  asciiReplay.m_obj = object;
  {
    dxfRW writer(asciiPath.string().c_str());
    asciiReplay.m_rw = &writer;
    REQUIRE(writer.write(&asciiReplay, DRW::AC1021, false));
  }
  RawObjectCapture asciiRoundTrip;
  {
    dxfRW reader(asciiPath.string().c_str());
    REQUIRE(reader.read(&asciiRoundTrip, /*ext=*/true));
  }
  REQUIRE(asciiRoundTrip.m_objects.size() == 1);
  const DRW_RawDxfObject &asciiObject = asciiRoundTrip.m_objects.front();
  CHECK(asciiObject.hasRawValues);
  REQUIRE(asciiObject.groups.size() == 11);
  CHECK(asciiObject.groups[3].i_val() == 7);
  CHECK(asciiObject.groups[4].i_val() == 123456);
  CHECK(asciiObject.groups[5].i64_val() == 0x0102030405060708LL);
  CHECK(asciiObject.groups[6].d_val() == 2.5);
  CHECK(asciiObject.groups[7].i_val() == 1);
  CHECK(asciiObject.groups[8].i_val() == 0x10203040);
  CHECK(std::string(asciiObject.groups[9].c_str()) == "3B");
  CHECK(std::string(asciiObject.groups[10].c_str()) == "29");

  std::filesystem::remove(path);
  std::filesystem::remove(roundTripPath);
  std::filesystem::remove(asciiPath);
}

TEST_CASE("DXF binary raw OBJECT rejects malformed chunks",
          "[dxf][rawobject][binary][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_binary_object_bad.dxf";
  std::filesystem::remove(path);

  RawObjectEmitter emitter;
  emitter.m_obj.name = "ACDBBINARY";
  emitter.m_obj.groups.emplace_back(5, std::string{"3B"});
  emitter.m_obj.groups.emplace_back(310, std::string{"A5"});
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, true));
  }

  std::ifstream input(path, std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
  const std::array<unsigned char, 4> chunk = {0x36, 0x01, 0x01, 0xA5};
  auto it = std::search(bytes.begin(), bytes.end(), chunk.begin(), chunk.end(),
                        [](char lhs, unsigned char rhs) {
                          return static_cast<unsigned char>(lhs) == rhs;
                        });
  REQUIRE(it != bytes.end());
  const std::size_t codeOffset =
      static_cast<std::size_t>(std::distance(bytes.begin(), it));
  REQUIRE(codeOffset + 2 < bytes.size());
  bytes.resize(codeOffset + 3);
  bytes[codeOffset + 2] = static_cast<char>(3);
  {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    REQUIRE(output.good());
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
  }

  RawObjectCapture captured;
  {
    dxfRW reader(path.string().c_str());
    CHECK_FALSE(reader.read(&captured, /*ext=*/true));
    CHECK(captured.m_objects.empty());
  }

  std::filesystem::remove(path);
}

TEST_CASE("DXF binary raw OBJECT rejects invalid chunk hex on write",
          "[dxf][rawobject][binary][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_binary_object_hex.dxf";
  std::filesystem::remove(path);

  RawObjectEmitter emitter;
  emitter.m_obj.name = "ACDBBINARY";
  emitter.m_obj.groups.emplace_back(310, std::string{"GG"});
  dxfRW writer(path.string().c_str());
  emitter.m_rw = &writer;
  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, true));
  CHECK_FALSE(emitter.m_writeResult);

  std::filesystem::remove(path);
}

TEST_CASE("DXF binary raw OBJECT rejects oversized chunks on write",
          "[dxf][rawobject][binary][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_binary_object_oversize.dxf";
  std::filesystem::remove(path);

  RawObjectEmitter emitter;
  emitter.m_obj.name = "ACDBBINARY";
  emitter.m_obj.groups.emplace_back(310, std::string(256, 'A'));
  dxfRW writer(path.string().c_str());
  emitter.m_rw = &writer;
  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, true));
  CHECK_FALSE(emitter.m_writeResult);

  std::filesystem::remove(path);
}

TEST_CASE("DXF raw write preflights malformed carriers",
          "[dxf][rawobject][writer][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_preflight.dxf";

  const auto verifyRejected = [&](const DRW_RawDxfObject &object,
                                  bool binary = false) {
    std::filesystem::remove(path);
    RawObjectEmitter emitter;
    emitter.m_obj = object;
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, binary));
    CHECK_FALSE(emitter.m_writeResult);
    CHECK(slurp(path).find(object.name) == std::string::npos);
  };

  SECTION("invalid handle") {
    DRW_RawDxfObject object;
    object.name = "ACDBINVALIDHANDLE";
    object.groups.emplace_back(5, std::string{"not-hex"});
    verifyRejected(object);
  }

  SECTION("modern records require a nonzero self handle") {
    DRW_RawDxfObject missing;
    missing.name = "ACDBMISSINGHANDLE";
    missing.groups.emplace_back(1, std::string{"payload"});
    verifyRejected(missing);

    DRW_RawDxfObject zero;
    zero.name = "ACDBZEROHANDLE";
    zero.groups.emplace_back(5, std::string{"0"});
    verifyRejected(zero);
  }

  SECTION("unbalanced application group") {
    DRW_RawDxfObject object;
    object.name = "ACDBINVALIDAPPGROUP";
    object.groups.emplace_back(102, std::string{"{ACAD_REACTORS"});
    verifyRejected(object);
  }

  SECTION("mismatched value type") {
    DRW_RawDxfObject object;
    object.name = "ACDBINVALIDTYPE";
    object.groups.emplace_back(70, 7.0);
    verifyRejected(object);
  }

  SECTION("text numeric value cannot be written as binary") {
    DRW_RawDxfObject object;
    object.name = "ACDBBINARYNUMERICTEXT";
    object.groups.emplace_back(70, std::string{"7"});
    verifyRejected(object, true);
  }

  SECTION("invalid numeric source lexeme") {
    DRW_RawDxfObject object;
    object.name = "ACDBINVALIDNUMERICLEXEME";
    object.hasRawValues = true;
    object.groups.emplace_back(70, 7);
    object.rawValues.emplace_back("not-a-number");
    verifyRejected(object);
  }

  SECTION("invalid handle source lexeme") {
    DRW_RawDxfObject object;
    object.name = "ACDBINVALIDHANDLELEXEME";
    object.hasRawValues = true;
    object.groups.emplace_back(5, std::string{"4A"});
    object.rawValues.emplace_back("not-hex");
    verifyRejected(object);
  }

  SECTION("unbalanced source application group") {
    DRW_RawDxfObject object;
    object.name = "ACDBINVALIDAPPLEXEME";
    object.hasRawValues = true;
    object.groups.emplace_back(102, std::string{"{ACAD_REACTORS"});
    object.groups.emplace_back(102, std::string{"}"});
    object.rawValues.emplace_back("{ACAD_REACTORS");
    object.rawValues.emplace_back("{ACAD_XDICTIONARY");
    verifyRejected(object);
  }

  SECTION("source lexeme cannot introduce a record boundary") {
    DRW_RawDxfObject object;
    object.name = "ACDBINVALIDLINEBREAK";
    object.hasRawValues = true;
    object.groups.emplace_back(1, std::string{"stable"});
    object.rawValues.emplace_back("first\nsecond");
    verifyRejected(object);
  }

  std::filesystem::remove(path);
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

bool recordHasConsecutive(
    const std::vector<std::pair<std::string, std::string>> &groups,
    const std::string &recordType, const std::string &recordName,
    const std::vector<std::pair<std::string, std::string>> &seq) {
  std::vector<std::pair<std::string, std::string>> record;
  auto matches = [&]() {
    if (record.empty())
      return false;
    bool hasName = false;
    for (const auto &kv : record) {
      if (kv.first == "2" && kv.second == recordName) {
        hasName = true;
        break;
      }
    }
    return hasName && hasConsecutive(record, seq);
  };

  bool inRecord = false;
  for (const auto &kv : groups) {
    if (kv.first == "0") {
      if (inRecord && matches())
        return true;
      inRecord = (kv.second == recordType);
      record.clear();
    }
    if (inRecord)
      record.push_back(kv);
  }
  return inRecord && matches();
}

bool recordTypeHasConsecutive(
    const std::vector<std::pair<std::string, std::string>> &groups,
    const std::string &recordType,
    const std::vector<std::pair<std::string, std::string>> &seq) {
  std::vector<std::pair<std::string, std::string>> record;
  bool inRecord = false;
  for (const auto &kv : groups) {
    if (kv.first == "0") {
      if (inRecord && hasConsecutive(record, seq))
        return true;
      inRecord = (kv.second == recordType);
      record.clear();
    }
    if (inRecord)
      record.push_back(kv);
  }
  return inRecord && hasConsecutive(record, seq);
}
} // namespace

TEST_CASE("DXF LAYOUT object writes plot prefix and layout body",
          "[dxf][layout][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_layout_write.dxf";
  std::filesystem::remove(path);

  class LayoutEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_Layout m_layout;

    void writeObjects() override { m_rw->writeLayout(&m_layout); }
  };

  LayoutEmitter em;
  em.m_layout.handle = 0x4Fu;
  em.m_layout.parentHandle = 0x1Au;
  em.m_layout.pageSetupName = "My Page Setup";
  em.m_layout.printerConfig = "DWG To PDF.pc3";
  em.m_layout.paperSize = "ANSI_A";
  em.m_layout.plotLayoutFlags = 688;
  em.m_layout.marginLeft = 5.8;
  em.m_layout.marginBottom = 6.1;
  em.m_layout.marginRight = 5.9;
  em.m_layout.marginTop = 6.2;
  em.m_layout.paperWidth = 215.9;
  em.m_layout.paperHeight = 279.4;
  em.m_layout.plotOriginX = 1.0;
  em.m_layout.plotOriginY = 2.0;
  em.m_layout.paperUnits = 0;
  em.m_layout.plotRotation = 1;
  em.m_layout.plotType = 5;
  em.m_layout.windowMinX = 0.5;
  em.m_layout.windowMinY = 0.25;
  em.m_layout.windowMaxX = 12.0;
  em.m_layout.windowMaxY = 9.0;
  em.m_layout.currentStyleSheet = "monochrome.ctb";
  em.m_layout.scaleType = 16;
  em.m_layout.scaleFactor = 1.0;
  em.m_layout.paperImageOriginX = 0.0;
  em.m_layout.paperImageOriginY = 0.0;
  em.m_layout.shadePlotMode = 1;
  em.m_layout.shadePlotResLevel = 2;
  em.m_layout.shadePlotCustomDPI = 300;
  em.m_layout.name = "Layout1";
  em.m_layout.layoutFlags = 1;
  em.m_layout.tabOrder = 2;
  em.m_layout.limMinX = 0.0;
  em.m_layout.limMinY = 0.0;
  em.m_layout.limMaxX = 12.0;
  em.m_layout.limMaxY = 9.0;
  em.m_layout.insPoint = DRW_Coord(0.0, 0.0, 0.0);
  em.m_layout.extMin = DRW_Coord(-1.0, -2.0, 0.0);
  em.m_layout.extMax = DRW_Coord(13.0, 10.0, 0.0);
  em.m_layout.ucsOrigin = DRW_Coord(1.0, 2.0, 3.0);
  em.m_layout.ucsXAxis = DRW_Coord(1.0, 0.0, 0.0);
  em.m_layout.ucsYAxis = DRW_Coord(0.0, 1.0, 0.0);
  em.m_layout.elevation = 0.0;
  em.m_layout.orthoViewType = 0;
  em.m_layout.paperSpaceBlockRecordHandle.ref = 0x50u;
  em.m_layout.lastActiveViewportHandle.ref = 0x51u;
  em.m_layout.shadePlotHandle.ref = 0x54u;
  em.m_layout.namedUcsHandle.ref = 0x52u;
  em.m_layout.baseUcsHandle.ref = 0x53u;

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Dictionary layoutDict;
    layoutDict.handle = 0x1Au;
    layoutDict.parentHandle = 0;
    layoutDict.cloning = 1;
    layoutDict.m_entries.push_back({"Layout1", 0x4Fu});
    w.setNamedDictObjects({layoutDict});
    w.setRootDictEntries({{"ACAD_LAYOUT", "1A"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups, {{"3", "ACAD_LAYOUT"}, {"350", "1A"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "DICTIONARY"}, {"5", "1A"}, {"330", "C"},
                        {"100", "AcDbDictionary"}, {"281", "1"},
                        {"3", "Layout1"}, {"350", "4F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "LAYOUT"}, {"5", "4F"}, {"330", "1A"},
                        {"100", "AcDbPlotSettings"}}));
  CHECK(hasConsecutive(groups,
                       {{"100", "AcDbLayout"}, {"1", "Layout1"},
                        {"70", "1"}, {"71", "2"}}));
  CHECK(hasConsecutive(groups, {{"76", "0"}, {"333", "54"}}));

  LayoutCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x4Fu);
  CHECK(cap.m_captured.parentHandle == 0x1A);
  CHECK(cap.m_captured.pageSetupName == "My Page Setup");
  CHECK(cap.m_captured.plotLayoutFlags == 688);
  CHECK(cap.m_captured.shadePlotMode == 1);
  CHECK(cap.m_captured.name == "Layout1");
  CHECK(cap.m_captured.layoutFlags == 1);
  CHECK(cap.m_captured.tabOrder == 2);
  CHECK(cap.m_captured.limMaxX == 12.0);
  CHECK(cap.m_captured.extMin.x == -1.0);
  CHECK(cap.m_captured.ucsOrigin.z == 3.0);
  CHECK(cap.m_captured.paperSpaceBlockRecordHandle.ref == 0x50u);
  CHECK(cap.m_captured.lastActiveViewportHandle.ref == 0x51u);
  CHECK(cap.m_captured.shadePlotHandle.ref == 0x54u);
  CHECK(cap.m_captured.namedUcsHandle.ref == 0x52u);
  CHECK(cap.m_captured.baseUcsHandle.ref == 0x53u);
}

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

TEST_CASE("DXF table record writers preserve XDATA",
          "[dxf][table][xdata]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_table_xdata.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    TableXDataEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(recordHasConsecutive(groups, "LTYPE", "XDASH",
                             {{"1001", "TABLEAPP"}, {"1000", "ltype-xdata"},
                              {"1071", "42"}}));
  CHECK(recordHasConsecutive(groups, "STYLE", "XDATASTYLE",
                             {{"1001", "TABLEAPP"}, {"1000", "style-xdata"},
                              {"1070", "7"}}));
  CHECK(recordHasConsecutive(groups, "VPORT", "*ACTIVE",
                             {{"1001", "TABLEAPP"}, {"1000", "vport-xdata"},
                              {"1070", "8"}}));
  CHECK(recordHasConsecutive(groups, "VIEW", "XVIEW",
                             {{"1001", "TABLEAPP"}, {"1000", "view-xdata"},
                              {"1070", "9"}}));
  CHECK(recordHasConsecutive(groups, "UCS", "XUCS",
                             {{"1001", "TABLEAPP"}, {"1000", "ucs-xdata"},
                              {"1070", "10"}}));
  CHECK(recordHasConsecutive(groups, "APPID", "XAPPID",
                             {{"1001", "TABLEAPP"}, {"1000", "appid-xdata"},
                              {"1070", "11"}}));
  CHECK(recordHasConsecutive(groups, "DIMSTYLE", "XDIM",
                             {{"1001", "TABLEAPP"}, {"1000", "dimstyle-xdata"},
                              {"1070", "12"}}));
}

TEST_CASE("DXF LTYPE preserves complex segment payloads",
          "[dxf][table][ltype]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_complex_ltype.dxf";
  std::filesystem::remove(path);

  LTypeEmitter emitter;
  emitter.m_ltype.name = "COMPLEX";
  emitter.m_ltype.desc = "complex line type";
  emitter.m_ltype.segments = {
      DRW_LTypeSegment{1.0, 0, {}, 0.0, 0.0, 1.0, 0.0, 0, {}},
      DRW_LTypeSegment{-.5, 23, {}, 1.25, -2.5, 2.0, M_PI / 4.0, 4, {}},
      DRW_LTypeSegment{.25, 0, {}, -.25, .5, .75, -M_PI / 6.0, 2, "TXT"}};
  emitter.m_ltype.segments[1].styleHandle.ref = 0x42u;
  emitter.m_ltype.segments[1].styleHandle.code = 5;
  emitter.m_ltype.segments[1].styleHandle.ref64 = 0x42u;
  emitter.m_ltype.segments[2].styleHandle.ref = 0x43u;
  emitter.m_ltype.segments[2].styleHandle.code = 5;
  emitter.m_ltype.segments[2].styleHandle.ref64 = 0x43u;

  {
    dxfRW writer(path.string().c_str());
    emitter.m_writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    CHECK(emitter.m_writeResult);
  }

  LTypeCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  std::filesystem::remove(path);

  const auto it = std::find_if(
      capture.m_ltypes.cbegin(), capture.m_ltypes.cend(),
      [](const DRW_LType &ltype) { return ltype.name == "COMPLEX"; });
  REQUIRE(it != capture.m_ltypes.cend());
  const DRW_LType &parsed = *it;
  REQUIRE(parsed.segments.size() == 3u);
  CHECK(parsed.alignment == 'A');
  CHECK(parsed.segments[0].length == Catch::Approx(1.0));
  CHECK(parsed.segments[1].complexShapeCode == 23);
  CHECK(parsed.segments[1].shapeFlags == 4);
  CHECK(parsed.segments[1].styleHandle.ref == 0x42u);
  CHECK(parsed.segments[1].xOffset == Catch::Approx(1.25));
  CHECK(parsed.segments[1].yOffset == Catch::Approx(-2.5));
  CHECK(parsed.segments[1].scale == Catch::Approx(2.0));
  CHECK(parsed.segments[1].rotation == Catch::Approx(M_PI / 4.0));
  CHECK(parsed.segments[2].shapeFlags == 2);
  CHECK(parsed.segments[2].styleHandle.ref == 0x43u);
  CHECK(parsed.segments[2].text == "TXT");
  CHECK(parsed.segments[2].rotation == Catch::Approx(-M_PI / 6.0));
}

TEST_CASE("DXF writer rejects malformed XDATA payload pointers",
          "[dxf][xdata][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_invalid_xdata_pointer.dxf";
  std::filesystem::remove(path);

  dxfRW writer(path.string().c_str());
  InvalidXDataEmitter emitter;
  emitter.m_rw = &writer;
  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));

  std::filesystem::remove(path);
}

TEST_CASE("DXF entity writer rejects malformed binary XDATA",
          "[dxf][xdata][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_invalid_entity_xdata.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream output(path);
    output << "existing destination\n";
  }

  dxfRW writer(path.string().c_str());
  InvalidEntityXDataEmitter emitter;
  emitter.m_rw = &writer;
  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
  CHECK_FALSE(emitter.m_writeResult);
  CHECK(slurp(path) == "existing destination\n");

  std::filesystem::remove(path);
}

TEST_CASE("DXF APPDATA and DIMSTYLE reject malformed values before emission",
          "[dxf][xdata][malformed]") {
  const auto appDataPath =
      std::filesystem::temp_directory_path() / "lc_invalid_appdata.dxf";
  std::filesystem::remove(appDataPath);
  {
    dxfRW writer(appDataPath.string().c_str());
    InvalidAppDataEmitter emitter;
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  CHECK(slurp(appDataPath).find("102\nINVALID_APP\n") == std::string::npos);
  std::filesystem::remove(appDataPath);

  const auto dimstylePath =
      std::filesystem::temp_directory_path() / "lc_invalid_dimstyle.dxf";
  std::filesystem::remove(dimstylePath);
  {
    dxfRW writer(dimstylePath.string().c_str());
    InvalidDimstyleEmitter emitter;
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  CHECK(slurp(dimstylePath).find("INVALID_DIMSTYLE") == std::string::npos);
  std::filesystem::remove(dimstylePath);
}

TEST_CASE("DXF object writers propagate malformed XDATA",
          "[dxf][xdata][malformed]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_invalid_object_xdata.dxf";
  std::filesystem::remove(path);

  {
    dxfRW writer(path.string().c_str());
    InvalidObjectXDataEmitter emitter;
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_writeResult);
  }
  CHECK(slurp(path).find("OBJECTAPP") == std::string::npos);
  std::filesystem::remove(path);
}

TEST_CASE("DXF selected entity writers preserve XDATA",
          "[dxf][entity][xdata]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_entity_xdata.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    EntityXDataEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(recordTypeHasConsecutive(
      groups, "LINE", {{"1001", "ENTITYAPP"}, {"1000", "line-xdata"},
                       {"1070", "17"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "LWPOLYLINE",
      {{"1001", "ENTITYAPP"}, {"1000", "lwpolyline-xdata"}, {"1071", "18"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "TEXT", {{"1001", "ENTITYAPP"}, {"1000", "text-xdata"},
                       {"1070", "19"}}));

  struct EntityXDataExpectation {
    const char *recordType;
    const char *payload;
    int value;
  };
  const EntityXDataExpectation expectations[] = {
      {"POINT", "point-xdata", 20},
      {"RAY", "ray-xdata", 21},
      {"XLINE", "xline-xdata", 22},
      {"CIRCLE", "circle-xdata", 23},
      {"ARC", "arc-xdata", 24},
      {"ELLIPSE", "ellipse-xdata", 25},
      {"TRACE", "trace-xdata", 26},
      {"SOLID", "solid-xdata", 27},
      {"3DFACE", "3dface-xdata", 28},
      {"POLYLINE", "polyline-xdata", 29},
      {"SPLINE", "spline-xdata", 30},
      {"HELIX", "helix-xdata", 31},
      {"HATCH", "hatch-xdata", 32},
      {"MPOLYGON", "mpolygon-xdata", 33},
      {"LEADER", "leader-xdata", 34},
      {"INSERT", "insert-xdata", 35},
      {"ACAD_TABLE", "table-xdata", 36},
      {"ATTRIB", "attrib-xdata", 37},
      {"ATTDEF", "attdef-xdata", 38},
      {"RTEXT", "rtext-xdata", 39},
      {"ARCALIGNEDTEXT", "arctext-xdata", 40},
      {"TOLERANCE", "tolerance-xdata", 41},
      {"DIMENSION", "dimension-xdata", 42},
      {"ARC_DIMENSION", "arcdim-xdata", 43},
      {"LARGE_RADIAL_DIMENSION", "largeradial-xdata", 44},
      {"MULTILEADER", "mleader-xdata", 45},
      {"LIGHT", "light-xdata", 46},
      {"MESH", "mesh-xdata", 47},
      {"SHAPE", "shape-xdata", 48},
      {"OLE2FRAME", "ole2frame-xdata", 49},
      {"VIEWPORT", "viewport-xdata", 50},
      {"IMAGE", "image-xdata", 51},
      {"WIPEOUT", "wipeout-xdata", 52},
      {"POINTCLOUD", "pointcloud-xdata", 53},
      {"PLANESURFACE", "surface-xdata", 54},
  };
  for (const auto &expected : expectations) {
    CAPTURE(expected.recordType);
    CHECK(recordTypeHasConsecutive(
        groups, expected.recordType,
        {{"1001", "ENTITYAPP"}, {"1000", expected.payload},
         {"1070", std::to_string(expected.value)}}));
  }
}

TEST_CASE("DXF selected object writers preserve XDATA",
          "[dxf][object][xdata]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_object_xdata.dxf";
  std::filesystem::remove(path);

  {
    dxfRW w(path.string().c_str());
    ObjectXDataEmitter em;
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(recordTypeHasConsecutive(
      groups, "SCALE", {{"1001", "OBJECTAPP"}, {"1000", "scale-xdata"},
                        {"1070", "21"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "MLINESTYLE",
      {{"1001", "OBJECTAPP"}, {"1000", "mlinestyle-xdata"}, {"1070", "22"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "FIELD", {{"1001", "OBJECTAPP"}, {"1000", "field-xdata"},
                        {"1070", "23"}}));
  CHECK(recordTypeHasConsecutive(
      groups, "WIPEOUTVARIABLES",
      {{"1001", "OBJECTAPP"}, {"1000", "wipeoutvars-xdata"}, {"1070", "24"}}));
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

TEST_CASE("DXF duplicate source handles do not resolve deferred references",
          "[dxf][objects][handles][safety]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_srcminted_duplicate.dxf";
  std::filesystem::remove(path);

  std::map<std::uint32_t, std::uint32_t> captured;
  {
    dxfRW w(path.string().c_str());
    class Emitter final : public StubInterface {
    public:
      dxfRW *m_rw = nullptr;

      void writeBlockRecords() override {
        m_rw->markSourceHandleAmbiguous(0xAAu);
      }

      void writeEntities() override {
        for (int i = 0; i < 2; ++i) {
          DRW_Point point;
          point.handle = 0xAAu;
          point.basePoint = DRW_Coord(1.0, 2.0, 0.0);
          m_rw->writePoint(&point);
        }
      }
    } em;
    em.m_rw = &w;
    DRW_Group group;
    group.name = "*DUPLICATE";
    group.m_entityHandles = {0xAAu};
    w.setGroups({group});
    REQUIRE(w.write(&em, DRW::AC1021, false));
    captured = w.getWritingContext()->sourceHandleToMintedMap;
    CHECK(w.getWritingContext()->ambiguousSourceHandles.count(0xAAu) == 1);
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  CHECK(captured.count(0xAAu) == 0);
  for (std::size_t i = 0; i < groups.size(); ++i) {
    if (groups[i].first != "0" || groups[i].second != "GROUP")
      continue;
    for (std::size_t j = i + 1; j < groups.size() && groups[j].first != "0";
         ++j) {
      CHECK(groups[j].first != "340");
    }
  }
}

TEST_CASE("DXF raw records bind source handles before deferred references",
          "[dxf][objects][handles][raw]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_raw_source_binding.dxf";
  std::filesystem::remove(path);

  class Emitter final : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    bool m_firstBinding = false;
    bool m_conflictingBinding = true;
    bool m_zeroBinding = true;
    bool m_writeResult = false;

    void writeEntities() override {
      DRW_RawDxfObject entity;
      entity.name = "WEIRDENT";
      entity.handle = 0xAAu;
      entity.groups.emplace_back(5, std::string{"AA"});
      entity.groups.emplace_back(8, std::string{"0"});
      m_firstBinding = m_rw->bindSourceEntityHandle(0xAAu, 0x101u);
      m_conflictingBinding =
          m_rw->bindSourceEntityHandle(0xAAu, 0x102u);
      m_zeroBinding = m_rw->bindSourceEntityHandle(0, 0x103u);
      m_writeResult = m_rw->writeRawDxfObject(&entity);
    }
  } emitter;

  DRW_Group group;
  group.name = "*RAW";
  group.m_entityHandles = {0xAAu};
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    writer.setGroups({group});
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  CHECK(emitter.m_firstBinding);
  CHECK_FALSE(emitter.m_conflictingBinding);
  CHECK_FALSE(emitter.m_zeroBinding);
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // The conflicting bind removes the source key. Neither the raw record nor
  // the GROUP may then claim one of the two incompatible output handles.
  CHECK(hasConsecutive(groups, {{"0", "WEIRDENT"}, {"5", "AA"}}));
  for (std::size_t i = 0; i < groups.size(); ++i) {
    if (groups[i].first != "0" || groups[i].second != "GROUP")
      continue;
    for (std::size_t j = i + 1; j < groups.size() && groups[j].first != "0";
         ++j) {
      CHECK(groups[j] != std::pair<std::string, std::string>{"340", "101"});
      CHECK(groups[j] != std::pair<std::string, std::string>{"340", "102"});
    }
  }

  // A separate successful binding proves that raw self-handles and deferred
  // references use the same output identity.
  const auto boundPath =
      std::filesystem::temp_directory_path() / "lc_raw_source_binding_ok.dxf";
  std::filesystem::remove(boundPath);
  class BoundEmitter final : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    bool m_writeResult = false;

    void writeEntities() override {
      DRW_RawDxfObject entity;
      entity.name = "WEIRDENT";
      entity.handle = 0xAAu;
      entity.groups.emplace_back(5, std::string{"AA"});
      entity.groups.emplace_back(8, std::string{"0"});
      m_rw->bindSourceEntityHandle(0xAAu, 0x101u);
      m_writeResult = m_rw->writeRawDxfObject(&entity);
    }
  } boundEmitter;
  {
    dxfRW writer(boundPath.string().c_str());
    boundEmitter.m_rw = &writer;
    writer.setGroups({group});
    DRW_RawDxfSection section;
    section.m_name = "CUSTOM_DATA";
    section.m_groups.emplace_back(5, std::string{"AA"});
    writer.setRawDxfSections({section});
    REQUIRE(writer.write(&boundEmitter, DRW::AC1021, false));
  }
  const auto boundGroups = readGroups(boundPath);
  std::filesystem::remove(boundPath);
  CHECK(hasConsecutive(boundGroups, {{"0", "WEIRDENT"}, {"5", "101"}}));
  CHECK(hasConsecutive(boundGroups, {{"340", "101"}}));
  CHECK(hasConsecutive(boundGroups,
                       {{"0", "SECTION"}, {"2", "CUSTOM_DATA"},
                        {"5", "AA"}}));
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
  em.m_parentHandle = 0x80000000u;
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
      REQUIRE(groups[i + 2].second == "80000000");  // explicit high-bit owner
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

TEST_CASE("DXF writeInsert distinguishes INSERT and MINSERT subtypes", "[dxf][insert]") {
  class InsertEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    void writeEntities() override {
      DRW_Insert insert;
      insert.name = "PLAIN";
      insert.basePoint = DRW_Coord(1.0, 2.0, 3.0);
      insert.extPoint = DRW_Coord(0.0, 0.0, -1.0);
      m_rw->writeInsert(&insert);

      DRW_Insert minsert;
      minsert.name = "GRID";
      minsert.basePoint = DRW_Coord(4.0, 5.0, 6.0);
      minsert.colcount = 2;
      minsert.rowcount = 3;
      minsert.colspace = 7.0;
      minsert.rowspace = 8.0;
      minsert.extPoint = DRW_Coord(0.0, 0.0, -1.0);
      m_rw->writeInsert(&minsert);
    }
  };

  const auto path = std::filesystem::temp_directory_path() / "lc_insert_subtypes.dxf";
  std::filesystem::remove(path);
  InsertEmitter em;
  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }
  const auto groups = readGroups(path);

  auto recordValue = [&](const std::string &recordType,
                         const std::string &recordName,
                         const std::string &code, std::string &out) {
    std::vector<std::pair<std::string, std::string>> record;
    auto matches = [&]() {
      const bool hasName = std::any_of(
          record.begin(), record.end(), [&](const auto &group) {
            return group.first == "2" && group.second == recordName;
          });
      if (!hasName)
        return false;
      const auto it = std::find_if(record.begin(), record.end(),
                                   [&](const auto &group) {
                                     return group.first == code;
                                   });
      if (it == record.end())
        return false;
      out = it->second;
      return true;
    };

    bool inRecord = false;
    for (const auto &group : groups) {
      if (group.first == "0") {
        if (inRecord && matches())
          return true;
        inRecord = group.second == recordType;
        record.clear();
      }
      if (inRecord)
        record.push_back(group);
    }
    return inRecord && matches();
  };
  auto numericValue = [&](const std::string &recordType,
                          const std::string &recordName,
                          const std::string &code, double expected) {
    std::string value;
    REQUIRE(recordValue(recordType, recordName, code, value));
    CHECK(std::abs(std::stod(value) - expected) < 1e-9);
  };
  auto recordHasValue = [&](const std::string &recordType,
                            const std::string &recordName,
                            const std::string &code,
                            const std::string &expected) {
    bool inRecord = false;
    bool hasName = false;
    bool hasValue = false;
    auto matches = [&]() { return hasName && hasValue; };
    for (const auto &group : groups) {
      if (group.first == "0") {
        if (inRecord && matches())
          return true;
        inRecord = group.second == recordType;
        hasName = false;
        hasValue = false;
      }
      if (inRecord) {
        hasName = hasName || (group.first == "2" && group.second == recordName);
        hasValue = hasValue || (group.first == code && group.second == expected);
      }
    }
    return inRecord && matches();
  };

  CHECK(recordHasValue("INSERT", "PLAIN", "100", "AcDbBlockReference"));
  numericValue("INSERT", "PLAIN", "10", 1.0);
  numericValue("INSERT", "PLAIN", "20", 2.0);
  numericValue("INSERT", "PLAIN", "30", 3.0);
  numericValue("INSERT", "PLAIN", "230", -1.0);

  CHECK(recordHasValue("INSERT", "GRID", "100", "AcDbMInsertBlock"));
  numericValue("INSERT", "GRID", "70", 2.0);
  numericValue("INSERT", "GRID", "71", 3.0);
  numericValue("INSERT", "GRID", "44", 7.0);
  numericValue("INSERT", "GRID", "45", 8.0);
  numericValue("INSERT", "GRID", "230", -1.0);

  InsertCapture captured;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&captured, false));
  std::filesystem::remove(path);

  REQUIRE(captured.m_captured.size() == 2);
  CHECK(captured.m_captured[0].name == "PLAIN");
  CHECK(captured.m_captured[0].basePoint.z == 3.0);
  CHECK(captured.m_captured[0].extPoint.z == -1.0);
  CHECK_FALSE(captured.m_captured[0].isMInsert());
  CHECK(captured.m_captured[1].name == "GRID");
  CHECK(captured.m_captured[1].isMInsert());
  CHECK(captured.m_captured[1].colcount == 2);
  CHECK(captured.m_captured[1].rowcount == 3);
  CHECK(captured.m_captured[1].colspace == 7.0);
  CHECK(captured.m_captured[1].rowspace == 8.0);
  CHECK(captured.m_captured[1].extPoint.z == -1.0);
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

TEST_CASE("DXF unnamed images receive distinct definition names",
          "[dxf][objects][image][writer][safety]") {
  class UnnamedImageEmitter final : public StubInterface {
  public:
    dxfRW *writer = nullptr;
    bool result = true;

    void writeEntities() override {
      for (int i = 0; i < 2; ++i) {
        DRW_Image image;
        image.secPoint = DRW_Coord(1.0, 0.0, 0.0);
        image.vVector = DRW_Coord(0.0, 1.0, 0.0);
        image.sizeu = 10.0;
        image.sizev = 10.0;
        result = writer->writeImage(&image, "") != nullptr && result;
      }
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_unnamed_image_definitions.dxf";
  std::filesystem::remove(path);
  dxfRW writer(path.string().c_str());
  emitter.writer = &writer;
  REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  CHECK(emitter.result);

  const auto groups = readGroups(path);
  std::vector<std::string> names;
  bool inImageDefinition = false;
  for (const auto &group : groups) {
    if (group.first == "0") {
      inImageDefinition = group.second == "IMAGEDEF";
    } else if (inImageDefinition && group.first == "1") {
      names.push_back(group.second);
    }
  }
  REQUIRE(names.size() == 2);
  CHECK_FALSE(names.front().empty());
  CHECK_FALSE(names.back().empty());
  CHECK(names.front() != names.back());
  std::filesystem::remove(path);
}

TEST_CASE("DXF image write retry rebuilds generated definitions",
          "[dxf][objects][image][writer][reuse]") {
  class RetryEmitter final : public StubInterface {
  public:
    dxfRW *writer = nullptr;
    bool failAfterImage = true;
    bool result = true;

    void writeEntities() override {
      DRW_Image image;
      image.secPoint = DRW_Coord(1.0, 0.0, 0.0);
      image.vVector = DRW_Coord(0.0, 1.0, 0.0);
      image.sizeu = 10.0;
      image.sizev = 10.0;
      result = writer->writeImage(&image, "retry.png") != nullptr;
      if (failAfterImage) {
        DRW_Wipeout invalid;
        invalid.basePoint.x = std::numeric_limits<double>::quiet_NaN();
        result = writer->writeWipeout(&invalid) && result;
      }
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_image_retry.dxf";
  std::filesystem::remove(path);
  dxfRW writer(path.string().c_str());
  emitter.writer = &writer;
  CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
  CHECK_FALSE(emitter.result);

  emitter.failAfterImage = false;
  REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  CHECK(emitter.result);
  const auto groups = readGroups(path);
  const auto countRecords = [&](const std::string &name) {
    return std::count_if(groups.begin(), groups.end(),
                         [&](const auto &group) {
                           return group.first == "0" && group.second == name;
                         });
  };
  CHECK(countRecords("IMAGEDEF") == 1);
  CHECK(countRecords("IMAGEDEF_REACTOR") == 1);
  std::filesystem::remove(path);
}

TEST_CASE("DXF IMAGE and WIPEOUT reject non-finite payload fields",
          "[dxf][objects][image][wipeout][writer][safety]") {
  class ImageEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    bool m_imageResult = true;
    bool m_wipeoutResult = true;
    bool m_writeWipeout = false;

    void writeEntities() override {
      if (m_writeWipeout) {
        DRW_Wipeout wipeout;
        wipeout.clipPath = {DRW_Coord{0.0, 0.0, 0.0},
                            DRW_Coord{1.0, 0.0, 0.0},
                            DRW_Coord{0.0, 1.0, 0.0}};
        wipeout.m_clipBoundaryType = 2;
        wipeout.basePoint.x = std::numeric_limits<double>::quiet_NaN();
        m_wipeoutResult = m_rw->writeWipeout(&wipeout);
      } else {
        DRW_Image image;
        image.basePoint.x = std::numeric_limits<double>::infinity();
        m_imageResult = m_rw->writeImage(&image, "invalid.png") != nullptr;
      }
    }
  };

  for (const bool wipeout : {false, true}) {
    const auto path = std::filesystem::temp_directory_path() /
                      (wipeout ? "lc_dxf_wipeout_nonfinite.dxf"
                               : "lc_dxf_image_nonfinite.dxf");
    std::filesystem::remove(path);
    ImageEmitter emitter;
    emitter.m_writeWipeout = wipeout;
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    const bool writeResult = wipeout ? emitter.m_wipeoutResult
                                     : emitter.m_imageResult;
    CHECK_FALSE(writeResult);
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF UNDERLAY rejects invalid writer payloads",
          "[dxf][underlay][writer][safety]") {
  class UnderlayEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_Underlay m_underlay;
    bool m_result = true;

    void writeEntities() override { m_result = m_rw->writeUnderlay(&m_underlay); }
  };

  SECTION("non-finite transform") {
    UnderlayEmitter emitter;
    emitter.m_underlay.position.x = std::numeric_limits<double>::quiet_NaN();
    const auto path = std::filesystem::temp_directory_path() /
                      "lc_dxf_underlay_nonfinite.dxf";
    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    std::filesystem::remove(path);
  }

  SECTION("inverse clipping before R2010") {
    UnderlayEmitter emitter;
    emitter.m_underlay.inverseClipBoundary = {
        DRW_Coord{0.0, 0.0, 0.0}, DRW_Coord{1.0, 1.0, 0.0}};
    const auto path = std::filesystem::temp_directory_path() /
                      "lc_dxf_underlay_old_inverse.dxf";
    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF MLINE writer rejects inconsistent payloads",
          "[dxf][mline][writer][safety]") {
  class MLineEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_MLine m_mline;
    bool m_result = true;

    void writeEntities() override { m_result = m_rw->writeMLine(&m_mline); }
  };

  SECTION("vertex count mismatch") {
    MLineEmitter emitter;
    emitter.m_mline.numVerts = 1;
    const auto path = std::filesystem::temp_directory_path() /
                      "lc_dxf_mline_vertex_mismatch.dxf";
    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    std::filesystem::remove(path);
  }

  SECTION("non-finite parameter") {
    MLineEmitter emitter;
    emitter.m_mline.numLines = 1;
    emitter.m_mline.numVerts = 1;
    DRW_MLineVertex vertex;
    vertex.segParms.resize(1);
    vertex.areaFillParms.resize(1);
    vertex.segParms.front().push_back(std::numeric_limits<double>::quiet_NaN());
    emitter.m_mline.vertlist.push_back(std::move(vertex));
    const auto path = std::filesystem::temp_directory_path() /
                      "lc_dxf_mline_nonfinite_parameter.dxf";
    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    std::filesystem::remove(path);
  }
}

TEST_CASE("DXF POLYLINE writer rejects invalid child payloads",
          "[dxf][polyline][writer][safety]") {
  class PolylineEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_Polyline m_polyline;
    bool m_result = true;

    void writeEntities() override {
      m_result = m_rw->writePolyline(&m_polyline);
    }
  };

  const auto write = [](DRW_Polyline polyline,
                        const std::string &filename) {
    const auto path = std::filesystem::temp_directory_path() / filename;
    std::filesystem::remove(path);
    PolylineEmitter emitter;
    emitter.m_polyline = std::move(polyline);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    std::filesystem::remove(path);
  };

  DRW_Polyline nullChild;
  nullChild.vertlist.push_back(nullptr);
  write(std::move(nullChild), "lc_dxf_polyline_null_child.dxf");

  DRW_Polyline nonFiniteChild;
  auto vertex = std::make_shared<DRW_Vertex>();
  vertex->basePoint.x = std::numeric_limits<double>::quiet_NaN();
  nonFiniteChild.vertlist.push_back(std::move(vertex));
  write(std::move(nonFiniteChild), "lc_dxf_polyline_nonfinite_child.dxf");
}

TEST_CASE("DXF INSERT writer rejects invalid attribute children",
          "[dxf][insert][writer][safety]") {
  class InsertEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_Insert m_insert;
    bool m_result = true;

    void writeEntities() override {
      m_result = m_rw->writeInsert(&m_insert);
    }
  };

  const auto write = [](DRW_Insert insert, const std::string& filename) {
    const auto path = std::filesystem::temp_directory_path() / filename;
    std::filesystem::remove(path);
    InsertEmitter emitter;
    emitter.m_insert = std::move(insert);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    CHECK_FALSE(std::filesystem::exists(path));
  };

  DRW_Insert nullAttribute;
  nullAttribute.name = "BLOCK";
  nullAttribute.attlist.push_back(nullptr);
  write(std::move(nullAttribute), "lc_dxf_insert_null_attribute.dxf");

  DRW_Insert unsafeAttribute;
  unsafeAttribute.name = "BLOCK";
  auto attribute = std::make_shared<DRW_Attrib>();
  attribute->tag = "TAG\nBROKEN";
  unsafeAttribute.attlist.push_back(std::move(attribute));
  write(std::move(unsafeAttribute), "lc_dxf_insert_unsafe_attribute.dxf");
}

TEST_CASE("DXF LWPOLYLINE writer rejects invalid payloads",
          "[dxf][polyline][writer][safety]") {
  class LwPolylineEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_LWPolyline m_polyline;
    bool m_result = true;

    void writeEntities() override {
      m_result = m_rw->writeLWPolyline(&m_polyline);
    }
  };

  const auto write = [](DRW_LWPolyline& polyline,
                        const std::string& filename) {
    const auto path = std::filesystem::temp_directory_path() / filename;
    std::filesystem::remove(path);
    LwPolylineEmitter emitter;
    emitter.m_polyline = polyline;
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    std::ifstream output(path);
    const std::string content{std::istreambuf_iterator<char>(output),
                              std::istreambuf_iterator<char>()};
    CHECK(content.find("\nLWPOLYLINE\n") == std::string::npos);
    std::filesystem::remove(path);
  };

  DRW_LWPolyline nullChild;
  nullChild.vertlist.push_back(nullptr);
  write(nullChild, "lc_dxf_lwpolyline_null_child.dxf");

  DRW_LWPolyline nonFinite;
  auto vertex = nonFinite.addVertex();
  vertex->x = std::numeric_limits<double>::infinity();
  write(nonFinite, "lc_dxf_lwpolyline_nonfinite_vertex.dxf");

  DRW_LWPolyline invalidFlags;
  invalidFlags.flags = 0x100;
  write(invalidFlags, "lc_dxf_lwpolyline_invalid_flags.dxf");
}

TEST_CASE("DXF SPLINE and HELIX writers reject invalid payloads",
          "[dxf][spline][helix][writer][safety]") {
  class SplineEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_Spline m_spline;
    bool m_result = true;

    void writeEntities() override { m_result = m_rw->writeSpline(&m_spline); }
  };

  class HelixEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_Helix m_helix;
    bool m_result = true;

    void writeEntities() override { m_result = m_rw->writeHelix(&m_helix); }
  };

  const auto validSpline = [] {
    DRW_Spline spline;
    spline.degree = 1;
    spline.knotslist = {0.0, 0.0, 1.0, 1.0};
    spline.controllist = {
        std::make_shared<DRW_Coord>(0.0, 0.0, 0.0),
        std::make_shared<DRW_Coord>(1.0, 1.0, 0.0)};
    return spline;
  };

  const auto writeSpline = [](DRW_Spline spline, const std::string &filename) {
    const auto path = std::filesystem::temp_directory_path() / filename;
    std::filesystem::remove(path);
    SplineEmitter emitter;
    emitter.m_spline = std::move(spline);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1021, false));
    CHECK_FALSE(emitter.m_result);
    std::ifstream output(path);
    const std::string content{std::istreambuf_iterator<char>(output),
                              std::istreambuf_iterator<char>()};
    CHECK(content.find("\nSPLINE\n") == std::string::npos);
    std::filesystem::remove(path);
  };

  DRW_Spline nullControl = validSpline();
  nullControl.controllist.front().reset();
  writeSpline(std::move(nullControl), "lc_dxf_spline_null_control.dxf");

  DRW_Spline invalidDegree = validSpline();
  invalidDegree.degree = 0;
  writeSpline(std::move(invalidDegree), "lc_dxf_spline_invalid_degree.dxf");

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_dxf_helix_nonfinite_trailer.dxf";
  std::filesystem::remove(path);
  HelixEmitter helixEmitter;
  helixEmitter.m_helix = DRW_Helix{};
  helixEmitter.m_helix.degree = 1;
  helixEmitter.m_helix.knotslist = {0.0, 0.0, 1.0, 1.0};
  helixEmitter.m_helix.controllist = {
      std::make_shared<DRW_Coord>(0.0, 0.0, 0.0),
      std::make_shared<DRW_Coord>(1.0, 1.0, 0.0)};
  helixEmitter.m_helix.radius = std::numeric_limits<double>::infinity();
  dxfRW helixWriter(path.string().c_str());
  helixEmitter.m_rw = &helixWriter;
  CHECK_FALSE(helixWriter.write(&helixEmitter, DRW::AC1021, false));
  CHECK_FALSE(helixEmitter.m_result);
  std::ifstream helixOutput(path);
  const std::string helixContent{std::istreambuf_iterator<char>(helixOutput),
                                 std::istreambuf_iterator<char>()};
  CHECK(helixContent.find("\nHELIX\n") == std::string::npos);
  std::filesystem::remove(path);
}

TEST_CASE("DXF rejects invalid HELIX trailer fields",
          "[dxf][helix][malformed]") {
  class HelixCapture : public StubInterface {
  public:
    int m_callCount = 0;

    void addHelix(const DRW_Helix *) override { ++m_callCount; }
  };

  const std::string valid =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nHELIX\n5\n7A\n330\n1F\n8\n0\n"
      "100\nAcDbEntity\n100\nAcDbSpline\n"
      "70\n0\n71\n1\n72\n4\n73\n2\n74\n0\n"
      "42\n0.0000001\n43\n0.0000001\n44\n0.0000001\n"
      "40\n0\n40\n0\n40\n1\n40\n1\n"
      "10\n0\n20\n0\n30\n0\n10\n1\n20\n1\n30\n0\n"
      "100\nAcDbHelix\n90\n0\n91\n0\n"
      "10\n0\n20\n0\n30\n0\n11\n0\n21\n0\n31\n0\n"
      "12\n0\n22\n0\n32\n1\n40\n1\n41\n1\n42\n1\n"
      "290\n1\n280\n0\n0\nENDSEC\n0\nEOF\n";

  SECTION("non-finite radius") {
    HelixCapture capture;
    std::string malformed = valid;
    const std::size_t radius = malformed.find("40\n1\n41");
    REQUIRE(radius != std::string::npos);
    malformed.replace(radius, std::string("40\n1\n41").size(),
                      "40\nnan\n41");
    CHECK_FALSE(tryReadDxf(malformed, capture,
                           "lc_helix_nonfinite_radius.dxf", false));
    CHECK(capture.m_callCount == 0);
  }

  SECTION("non-boolean handedness") {
    HelixCapture capture;
    std::string malformed = valid;
    const std::size_t handedness = malformed.find("290\n1\n");
    REQUIRE(handedness != std::string::npos);
    malformed.replace(handedness, std::string("290\n1\n").size(),
                      "290\n2\n");
    CHECK_FALSE(tryReadDxf(malformed, capture,
                           "lc_helix_invalid_handedness.dxf", false));
    CHECK(capture.m_callCount == 0);
  }
}

// A-3: child records must have their own handles and point back to the
// compound parent. The source->minted map must still contain only genuine
// source handles; synthetic SEQEND records have no source identity.
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

TEST_CASE("DXF compound children use independent handles and parent owners",
          "[dxf][polyline][insert][handles]") {
  class CompoundEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;

    void writeEntities() override {
      DRW_Polyline polyline;
      polyline.handle = 0xA0u;
      auto vertex = std::make_shared<DRW_Vertex>();
      vertex->basePoint = DRW_Coord(1.0, 2.0, 0.0);
      polyline.vertlist.push_back(vertex);
      REQUIRE(m_rw->writePolyline(&polyline));

      DRW_Insert insert;
      insert.handle = 0xB0u;
      insert.name = "BLOCK";
      auto attribute = std::make_shared<DRW_Attrib>();
      attribute->tag = "TAG";
      attribute->text = "value";
      attribute->basePoint = DRW_Coord(3.0, 4.0, 0.0);
      attribute->height = 1.0;
      insert.attlist.push_back(attribute);
      REQUIRE(m_rw->writeInsert(&insert));
    }
  };

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_dxf_compound_child_handles.dxf";
  std::filesystem::remove(path);
  CompoundEmitter emitter;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);
  std::vector<std::vector<std::pair<std::string, std::string>>> records;
  for (const auto &group : groups) {
    if (group.first == "0")
      records.emplace_back();
    if (!records.empty())
      records.back().push_back(group);
  }

  const auto record = [&records](const std::string &type, std::size_t ordinal) {
    std::size_t seen = 0;
    for (const auto &candidate : records) {
      if (!candidate.empty() && candidate.front() == std::make_pair(std::string{"0"}, type)) {
        if (seen++ == ordinal)
          return &candidate;
      }
    }
    return static_cast<const std::vector<std::pair<std::string, std::string>> *>(nullptr);
  };
  const auto value = [](const auto &candidate, const std::string &code) {
    const auto it = std::find_if(candidate->cbegin(), candidate->cend(),
                                 [&code](const auto &group) {
                                     return group.first == code;
                                 });
    return it == candidate->cend() ? std::string{} : it->second;
  };

  const auto *polyline = record("POLYLINE", 0);
  const auto *vertex = record("VERTEX", 0);
  const auto *polylineEnd = record("SEQEND", 0);
  const auto *insert = record("INSERT", 0);
  const auto *attribute = record("ATTRIB", 0);
  const auto *insertEnd = record("SEQEND", 1);
  REQUIRE(polyline != nullptr);
  REQUIRE(vertex != nullptr);
  REQUIRE(polylineEnd != nullptr);
  REQUIRE(insert != nullptr);
  REQUIRE(attribute != nullptr);
  REQUIRE(insertEnd != nullptr);

  const std::string polylineHandle = value(polyline, "5");
  const std::string vertexHandle = value(vertex, "5");
  const std::string polylineEndHandle = value(polylineEnd, "5");
  const std::string insertHandle = value(insert, "5");
  CHECK_FALSE(polylineHandle.empty());
  CHECK_FALSE(vertexHandle.empty());
  CHECK_FALSE(polylineEndHandle.empty());
  CHECK_FALSE(insertHandle.empty());
  CHECK(vertexHandle != polylineHandle);
  CHECK(polylineEndHandle != polylineHandle);
  CHECK(value(vertex, "330") == polylineHandle);
  CHECK(value(polylineEnd, "330") == polylineHandle);
  CHECK(value(attribute, "330") == insertHandle);
  CHECK(value(insertEnd, "330") == insertHandle);
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

// dxf-struct-003 (getstr-prefix): DRW_Header::getStr/getInt/getDouble/getCoord
// now try the alternate $-convention when the exact key is not found.  This
// bridges the DWG->DXF path where DWG parseDwg stores bare keys ("LTSCALE")
// but encodeDxf (DRW_Header::write) queries with "$" prefix ("$LTSCALE").
// Without the fix, every header var would fall through to the codec-internal
// default, silently discarding what was read from the DWG file.
//
// Also verifies that $FINGERPRINTGUID/$VERSIONGUID are now emitted (R2000+)
// when stored under their DWG bare-key names.
// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DXF header write tolerates bare vs $-prefixed key convention (dxf-struct-003)",
          "[dxf][header][getstr-prefix]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_header_getstr.dxf";
  std::filesystem::remove(path);

  class BareKeyHeaderEmitter : public StubInterface {
  public:
    void writeHeader(DRW_Header &h) override {
      // DWG-parse convention: bare keys, no '$'.
      h.addDouble("LTSCALE", 4.25, 40);  // default would be 1.0 -> emits "1"
      h.addInt("LUNITS", 3, 70);          // default would be 2
      h.addDouble("LATITUDE", 11.0, 40);
      h.addDouble("LONGITUDE", 22.0, 40);
      h.addDouble("NORTHDIRECTION", 33.0, 40);
      h.addInt("PELLIPSE", 1, 70);
      h.addInt("PROXIGRAPHICS", 0, 70);
      h.addInt("ISOLINES", 9, 70);
      h.addDouble("FACETRES", 0.75, 40);
      h.addInt("TEXTQLTY", 42, 70);
      h.addDouble("TDCREATE", 2450000.25, 40);
      h.addDouble("TDUPDATE", 2450001.5, 40);
      h.addDouble("TDINDWG", 0.125, 40);
      h.addDouble("TDUSRTIMER", 0.75, 40);
      h.addInt("TSTACKALIGN", 2, 70);
      h.addInt("TSTACKSIZE", 80, 70);
      h.addInt("OBSCUREDCOLOR", 123, 70);
      h.addInt("OBSCUREDLTYPE", 4, 70);
      h.addDouble("DIMALTMZF", 1.25, 40);
      h.addStr("DIMALTMZS", "alt-suffix", 1);
      h.addDouble("DIMMZF", 2.5, 40);
      h.addStr("DIMMZS", "main-suffix", 1);
      h.addStr("FINGERPRINTGUID",
               "{AABBCCDD-0000-0000-0000-001122334455}", 2);
      h.addStr("VERSIONGUID",
               "{FFEEDDCC-0000-0000-0000-AABBCCDDEEFF}", 2);
    }
  } em;

  {
    dxfRW w(path.string().c_str());
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }
  const auto groups = readGroups(path);
  std::filesystem::remove(path);

  // LTSCALE: bare key must propagate 4.25, not the default 1.
  CHECK(hasConsecutive(groups, {{"9", "$LTSCALE"}, {"40", "4.25"}}));
  // LUNITS: bare key must propagate 3, not the default 2.
  CHECK(hasConsecutive(groups, {{"9", "$LUNITS"}, {"70", "3"}}));
  CHECK(hasConsecutive(groups, {{"9", "$LATITUDE"}, {"40", "11"}}));
  CHECK(hasConsecutive(groups, {{"9", "$LONGITUDE"}, {"40", "22"}}));
  // NORTHDIRECTION must use its own value, not the longitude value.
  CHECK(hasConsecutive(groups, {{"9", "$NORTHDIRECTION"}, {"40", "33"}}));
  CHECK(hasConsecutive(groups, {{"9", "$PELLIPSE"}, {"70", "1"}}));
  CHECK(hasConsecutive(groups, {{"9", "$PROXYGRAPHICS"}, {"70", "0"}}));
  CHECK(hasConsecutive(groups, {{"9", "$ISOLINES"}, {"70", "9"}}));
  CHECK(hasConsecutive(groups, {{"9", "$FACETRES"}, {"40", "0.75"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TEXTQLTY"}, {"70", "42"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDCREATE"}, {"40", "2450000.25"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDUPDATE"}, {"40", "2450001.5"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDINDWG"}, {"40", "0.125"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TDUSRTIMER"}, {"40", "0.75"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TSTACKALIGN"}, {"70", "2"}}));
  CHECK(hasConsecutive(groups, {{"9", "$TSTACKSIZE"}, {"70", "80"}}));
  CHECK(hasConsecutive(groups, {{"9", "$OBSCOLOR"}, {"70", "123"}}));
  CHECK(hasConsecutive(groups, {{"9", "$OBSLTYPE"}, {"280", "4"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMALTMZF"}, {"40", "1.25"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMALTMZS"}, {"1", "alt-suffix"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMMZF"}, {"40", "2.5"}}));
  CHECK(hasConsecutive(groups, {{"9", "$DIMMZS"}, {"1", "main-suffix"}}));
  // GUIDs must appear (not silently skipped) when stored as bare keys.
  CHECK(hasConsecutive(groups,
      {{"9", "$FINGERPRINTGUID"},
       {"2", "{AABBCCDD-0000-0000-0000-001122334455}"}}));
  CHECK(hasConsecutive(groups,
      {{"9", "$VERSIONGUID"},
       {"2", "{FFEEDDCC-0000-0000-0000-AABBCCDDEEFF}"}}));
}

// ── Preservation parity: structured DXF read of objects previously raw-only ──
namespace {
class MaterialCapture : public StubInterface {
public:
  int m_callCount = 0;
  int m_rawCount = 0;
  DRW_Material m_captured;
  void addMaterial(const DRW_Material &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
  void addRawDxfObject(const DRW_RawDxfObject &object) override {
    ++m_rawCount;
    m_raw.push_back(object);
  }

  std::vector<DRW_RawDxfObject> m_raw;
};

class GeoDataCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_GeoData m_captured;
  void addGeoData(const DRW_GeoData &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF MATERIAL is read into a DRW_Material (name + description)",
          "[dxf][material][preservation]") {
  MaterialCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMATERIAL\n5\nEF\n330\nC\n100\nAcDbMaterial\n"
      "1\nBrass\n2\nPolished brass material\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_material_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_name == "Brass");
  CHECK(cap.m_captured.m_description == "Polished brass material");
  // Dual-mode: also preserved raw for lossless DXF re-emit.
  CHECK(cap.m_rawCount == 1);
}

TEST_CASE("DXF raw-backed objects preserve complete application groups",
          "[dxf][rawobject][application-group]") {
  const std::string source =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMATERIAL\n5\nEF\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\nF0\n102\n}\n"
      "102\n{CUSTOM\n102\n{NESTED\n1\npayload\n102\n}\n102\n}\n"
      "100\nAcDbMaterial\n1\nBrass\n2\nPolished brass material\n"
      "0\nENDSEC\n0\nEOF\n";
  MaterialCapture captured;
  readDxf(source, captured, "lc_material_application_groups.dxf");

  REQUIRE(captured.m_callCount == 1);
  CHECK(captured.m_captured.reactorHandles
        == std::vector<std::uint32_t>{0xF0u});
  CHECK(captured.m_captured.appData.size() == 2);
  REQUIRE(captured.m_raw.size() == 1);
  const DRW_RawDxfObject &object = captured.m_raw.front();
  const std::vector<int> expectedCodes = {5, 330, 102, 330, 102,
                                          102, 102, 1, 102, 102,
                                          100, 1, 2};
  std::vector<int> codes;
  codes.reserve(object.groups.size());
  for (const DRW_Variant &group : object.groups)
    codes.push_back(group.code());
  CHECK(codes == expectedCodes);

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_material_application_groups_rt.dxf";
  std::filesystem::remove(path);
  RawObjectEmitter emitter;
  emitter.m_obj = object;
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
    CHECK(emitter.m_writeResult);
  }

  RawObjectCapture replayed;
  {
    dxfRW reader(path.string().c_str());
    REQUIRE(reader.read(&replayed, true));
  }
  REQUIRE(replayed.m_objects.size() == 1);
  CHECK(replayed.m_objects.front().groups.size() == object.groups.size());
  std::filesystem::remove(path);
}

TEST_CASE("DXF GEODATA is read into a DRW_GeoData (scalar geolocation fields)",
          "[dxf][geodata][preservation]") {
  GeoDataCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBGEODATA\n5\nF0\n330\n1F\n100\nAcDbGeoData\n"
      "90\n3\n70\n2\n"
      "10\n100.0\n20\n200.0\n30\n0.0\n"
      "11\n12.5\n21\n55.25\n31\n0.0\n"
      "40\n1.0\n41\n1.0\n"
      "95\n1\n141\n2.5\n294\n1\n142\n123.0\n"
      "302\ngeo-rss-tag\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_geodata_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  const DRW_GeoData &g = cap.m_captured;
  CHECK(g.m_version == 3);
  CHECK(g.m_coordinatesType == 2);
  CHECK(g.m_designPoint.x == 100.0);
  CHECK(g.m_designPoint.y == 200.0);
  CHECK(g.m_referencePoint.x == 12.5);
  CHECK(g.m_referencePoint.y == 55.25);
  CHECK(g.m_scaleEstimationMethod == 1);
  CHECK(g.m_userSpecifiedScaleFactor == 2.5);
  CHECK(g.m_enableSeaLevelCorrection == true);
  CHECK(g.m_seaLevelElevation == 123.0);
  CHECK(g.m_geoRssTag == "geo-rss-tag");
}

TEST_CASE("DXF GEODATA object writes class and geolocation fields",
          "[dxf][geodata][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_geodata_write.dxf";
  std::filesystem::remove(path);

  class GeoDataEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_GeoData m_geo;

    void writeObjects() override { m_rw->writeGeoData(&m_geo); }
  };

  GeoDataEmitter em;
  em.m_geo.handle = 0xF0u;
  em.m_geo.parentHandle = 0x1Fu;
  em.m_geo.m_hostBlockHandle = 0x70u;
  em.m_geo.m_version = 3;
  em.m_geo.m_coordinatesType = 2;
  em.m_geo.m_designPoint = DRW_Coord(100.0, 200.0, 0.0);
  em.m_geo.m_referencePoint = DRW_Coord(12.5, 55.25, 3.0);
  em.m_geo.m_horizontalUnitScale = 1.0;
  em.m_geo.m_verticalUnitScale = 1.0;
  em.m_geo.m_horizontalUnits = 6;
  em.m_geo.m_verticalUnits = 6;
  em.m_geo.m_upDirection = DRW_Coord(0.0, 0.0, 1.0);
  em.m_geo.m_northDirection = DRW_Coord(0.0, 1.0, 0.0);
  em.m_geo.m_scaleEstimationMethod = 1;
  em.m_geo.m_userSpecifiedScaleFactor = 2.5;
  em.m_geo.m_enableSeaLevelCorrection = true;
  em.m_geo.m_seaLevelElevation = 123.0;
  em.m_geo.m_coordinateProjectionRadius = 6378137.0;
  em.m_geo.m_coordinateSystemDefinition = "EPSG:4326";
  em.m_geo.m_geoRssTag = "geo-rss";
  em.m_geo.m_observationFromTag = "from-tag";
  em.m_geo.m_observationToTag = "to-tag";
  em.m_geo.m_observationCoverageTag = "coverage-tag";
  DRW_GeoMeshPoint point;
  point.m_source = DRW_Coord(0.0, 0.0, 0.0);
  point.m_destination = DRW_Coord(100.0, 200.0, 0.0);
  em.m_geo.m_points.push_back(point);
  point.m_source = DRW_Coord(1.0, 2.0, 0.0);
  point.m_destination = DRW_Coord(101.0, 202.0, 0.0);
  em.m_geo.m_points.push_back(point);
  DRW_GeoMeshFace face;
  face.m_index1 = 0;
  face.m_index2 = 1;
  face.m_index3 = 0;
  em.m_geo.m_faces.push_back(face);

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("GEODATA", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary geoDict;
    geoDict.handle = 0x1Fu;
    geoDict.parentHandle = 0;
    geoDict.cloning = 1;
    geoDict.m_entries.push_back({"GeoData", 0xF0u});
    w.setNamedDictObjects({geoDict});
    w.setRootDictEntries({{"ACAD_GEOGRAPHICDATA", "1F"}});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "GEODATA"},
                        {"2", "AcDbGeoData"}}));
  CHECK(hasConsecutive(groups,
                       {{"3", "ACAD_GEOGRAPHICDATA"}, {"350", "1F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "GEODATA"}, {"5", "F0"}, {"330", "1F"},
                        {"100", "AcDbGeoData"}, {"90", "3"}, {"330", "70"},
                        {"70", "2"}}));
  CHECK(hasConsecutive(groups, {{"301", "EPSG:4326"}, {"302", "geo-rss"}}));
  CHECK(hasConsecutive(groups, {{"93", "2"}}));
  CHECK(hasConsecutive(groups, {{"96", "1"}, {"97", "0"}, {"98", "1"},
                                {"99", "0"}}));

  GeoDataCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_GeoData &g = cap.m_captured;
  CHECK(g.handle == 0xF0u);
  CHECK(g.parentHandle == 0x1F);
  CHECK(g.m_hostBlockHandle == 0x70u);
  CHECK(g.m_version == 3);
  CHECK(g.m_coordinatesType == 2);
  CHECK(g.m_designPoint.x == 100.0);
  CHECK(g.m_referencePoint.z == 3.0);
  CHECK(g.m_horizontalUnits == 6);
  CHECK(g.m_verticalUnits == 6);
  CHECK(g.m_upDirection.z == 1.0);
  CHECK(g.m_northDirection.y == 1.0);
  CHECK(g.m_scaleEstimationMethod == 1);
  CHECK(g.m_userSpecifiedScaleFactor == 2.5);
  CHECK(g.m_enableSeaLevelCorrection == true);
  CHECK(g.m_seaLevelElevation == 123.0);
  CHECK(g.m_coordinateProjectionRadius == 6378137.0);
  CHECK(g.m_coordinateSystemDefinition == "EPSG:4326");
  CHECK(g.m_geoRssTag == "geo-rss");
  CHECK(g.m_observationFromTag == "from-tag");
  CHECK(g.m_observationToTag == "to-tag");
  CHECK(g.m_observationCoverageTag == "coverage-tag");
  REQUIRE(g.m_points.size() == 2);
  CHECK(g.m_points[0].m_source.x == 0.0);
  CHECK(g.m_points[0].m_destination.y == 200.0);
  CHECK(g.m_points[1].m_source.y == 2.0);
  CHECK(g.m_points[1].m_destination.x == 101.0);
  REQUIRE(g.m_faces.size() == 1);
  CHECK(g.m_faces[0].m_index1 == 0);
  CHECK(g.m_faces[0].m_index2 == 1);
  CHECK(g.m_faces[0].m_index3 == 0);
}

namespace {
class VisualStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_VisualStyle m_captured;
  void addVisualStyle(const DRW_VisualStyle &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF VISUALSTYLE is read into a DRW_VisualStyle (desc + type)",
          "[dxf][visualstyle][preservation]") {
  VisualStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBVISUALSTYLE\n5\nF1\n330\nC\n100\nAcDbVisualStyle\n"
      "2\nConceptual\n70\n5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_visualstyle_read.dxf");

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.desc == "Conceptual");
  CHECK(cap.m_captured.type == 5);
}

TEST_CASE("DXF OBJECT aliases resolve their custom CLASS definitions",
          "[dxf][classes][aliases]") {
  const std::vector<std::pair<const char *, const char *>> aliases = {
      {"ACDBGEODATA", "AcDbGeoData"},
      {"ACDBVISUALSTYLE", "AcDbVisualStyle"},
      {"ACDBRASTERVARIABLES", "AcDbRasterVariables"},
      {"ACDBWIPEOUTVARIABLES", "AcDbWipeoutVariables"},
      {"DICTIONARYWDFLT", "AcDbDictionaryWithDefault"},
      {"SPATIALFILTER", "AcDbSpatialFilter"},
      {"ACDBPOINTCLOUDDEF", "AcDbPointCloudDef"},
      {"ACDBPOINTCLOUDDEFEX", "AcDbPointCloudDefEx"},
      {"ACDBPOINTCLOUDDEFREACTOR", "AcDbPointCloudDefReactor"},
      {"ACDBPOINTCLOUDDEFREACTOREX", "AcDbPointCloudDefReactorEx"},
      {"ACSH_BOX_CLASS", "AcDbShBox"},
      {"BLOCKPOINTPARAMETER", "AcDbBlockPointParameter"},
      {"ACDB_DYNAMICBLOCKPROXYNODE", "AcDbDynamicBlockProxyNode"},
      {"ACDBASSOCDEPENDENCY", "AcDbAssocDependency"},
      {"ACDBASSOCNETWORK", "AcDbAssocNetwork"},
      {"ACDBASSOCACTIONPARAM", "AcDbAssocActionParam"},
      {"CONTEXTDATAMANAGER", "AcDbContextDataManager"},
      {"VBA_PROJECT", "AcDbVbaProject"},
      {"ACAD_PROXY_ENTITY_WRAPPER", "AcDbProxyEntityWrapper"},
      {"ACAD_PROXY_OBJECT_WRAPPER", "AcDbProxyObjectWrapper"},
      {"ACDB_ALDIMOBJECTCONTEXTDATA_CLASS", "AcDbAlignedDimensionObjectContextData"},
      {"ACDB_ANGDIMOBJECTCONTEXTDATA_CLASS", "AcDbAngularDimensionObjectContextData"},
      {"ACDB_ANNOTSCALEOBJECTCONTEXTDATA_CLASS", "AcDbAnnotScaleObjectContextData"},
      {"ACDB_BLKREFOBJECTCONTEXTDATA_CLASS", "AcDbBlkrefObjectContextData"},
      {"ACDB_DMDIMOBJECTCONTEXTDATA_CLASS", "AcDbDiametricDimensionObjectContextData"},
      {"ACDB_FCFOBJECTCONTEXTDATA_CLASS", "AcDbFcfObjectContextData"},
      {"ACDB_LEADEROBJECTCONTEXTDATA_CLASS", "AcDbLeaderObjectContextData"},
      {"ACDB_MLEADEROBJECTCONTEXTDATA_CLASS", "AcDbMLeaderObjectContextData"},
      {"ACDB_ORDDIMOBJECTCONTEXTDATA_CLASS", "AcDbOrdinateDimensionObjectContextData"},
      {"ACDB_RADIMLGOBJECTCONTEXTDATA_CLASS", "AcDbRadialDimensionLargeObjectContextData"},
      {"ACDB_RADIMOBJECTCONTEXTDATA_CLASS", "AcDbRadialDimensionObjectContextData"},
      {"ACDB_TEXTOBJECTCONTEXTDATA_CLASS", "AcDbTextObjectContextData"},
  };
  for (const auto &[recordName, className] : aliases) {
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName(recordName, cls));
    CHECK(cls.recName == recordName);
    CHECK(cls.className == className);
    CHECK(cls.entityFlag == 0);
  }
}

namespace {
class TableStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_TableStyle m_captured;
  void addTableStyle(const DRW_TableStyle &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
class MLeaderStyleCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_MLeaderStyle m_captured;
  void addMLeaderStyle(const DRW_MLeaderStyle *d) override {
    if (m_callCount == 0 && d) m_captured = *d;
    ++m_callCount;
  }
};
class SpatialFilterCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_SpatialFilter m_captured;
  void addSpatialFilter(const DRW_SpatialFilter &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
class ImageDefReactorCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_ImageDefinitionReactor m_captured;
  void addImageDefinitionReactor(const DRW_ImageDefinitionReactor &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF TABLESTYLE is read into a DRW_TableStyle (top-level fields)",
          "[dxf][tablestyle][preservation]") {
  TableStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nTABLESTYLE\n5\n1C0\n330\nC\n100\nAcDbTableStyle\n"
      "3\nMyStyle\n70\n1\n71\n2\n40\n0.06\n41\n0.07\n280\n1\n281\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_tablestyle_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_name == "MyStyle");
  CHECK(cap.m_captured.m_flowDirection == 1);
  CHECK(cap.m_captured.m_flags == 2);
  CHECK(cap.m_captured.m_horizontalCellMargin == 0.06);
  CHECK(cap.m_captured.m_verticalCellMargin == 0.07);
  CHECK(cap.m_captured.m_titleSuppressed == true);
  CHECK(cap.m_captured.m_headerSuppressed == false);
}

TEST_CASE("DXF MLEADERSTYLE is read into a DRW_MLeaderStyle",
          "[dxf][mleaderstyle][preservation]") {
  MLeaderStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMLEADERSTYLE\n5\n1D0\n330\nC\n100\nAcDbMLeaderStyle\n"
      "3\nStandard\n300\nDefault Text\n170\n2\n90\n2\n"
      "40\n0.5\n41\n0.75\n173\n1\n44\n2.5\n45\n3.0\n"
      "290\n1\n291\n0\n142\n1.5\n296\n1\n"
      "340\n1E\n342\n20\n343\n21\n271\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_mleaderstyle_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_MLeaderStyle &s = cap.m_captured;
  CHECK(s.description == "Standard");
  CHECK(s.textDefault == "Default Text");
  CHECK(s.contentType == 2);
  CHECK(s.maxLeaderPoints == 2);
  CHECK(s.firstSegmentAngle == 0.5);
  CHECK(s.secondSegmentAngle == 0.75);
  CHECK(s.leaderType == 1);
  CHECK(s.arrowHeadSize == 2.5);
  CHECK(s.textHeight == 3.0);
  CHECK(s.landingEnabled == true);
  CHECK(s.autoIncludeLanding == false);
  CHECK(s.scaleFactor == 1.5);
  CHECK(s.isAnnotative == true);
  CHECK(s.attachmentDirection == 2);
  CHECK(s.leaderLineTypeHandle.ref == 0x1Eu);
  CHECK(s.textStyleHandle.ref == 0x20u);
  CHECK(s.blockHandle.ref == 0x21u);
}

TEST_CASE("DXF MLEADERSTYLE object writes class and style fields",
          "[dxf][mleaderstyle][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_mleaderstyle_write.dxf";
  std::filesystem::remove(path);

  class MLeaderStyleEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_MLeaderStyle m_style;

    void writeObjects() override { m_rw->writeMLeaderStyle(&m_style); }
  };

  MLeaderStyleEmitter em;
  em.m_style.handle = 0x1D0u;
  em.m_style.parentHandle = 0x1CFu;
  em.m_style.name = "Standard";
  em.m_style.flags = 4;
  em.m_style.styleVersion = 2;
  em.m_style.contentType = 2;
  em.m_style.drawMLeaderOrder = 1;
  em.m_style.drawLeaderOrder = 0;
  em.m_style.maxLeaderPoints = 7;
  em.m_style.firstSegmentAngle = 0.5;
  em.m_style.secondSegmentAngle = 0.75;
  em.m_style.leaderType = 1;
  em.m_style.leaderColor = 3;
  em.m_style.leaderLineTypeHandle.ref = 0x1E0u;
  em.m_style.leaderLineWeight = 29;
  em.m_style.landingEnabled = true;
  em.m_style.landingGap = 1.25;
  em.m_style.autoIncludeLanding = false;
  em.m_style.landingDistance = 2.25;
  em.m_style.description = "Round-trip MLeader style";
  em.m_style.arrowHeadBlockHandle.ref = 0x1E1u;
  em.m_style.arrowHeadSize = 0.75;
  em.m_style.textDefault = "Default leader text";
  em.m_style.textStyleHandle.ref = 0x1E2u;
  em.m_style.leftAttachment = 1;
  em.m_style.rightAttachment = 2;
  em.m_style.textAngleType = 1;
  em.m_style.textAlignmentType = 2;
  em.m_style.textColor = 5;
  em.m_style.textHeight = 2.5;
  em.m_style.textFrameEnabled = true;
  em.m_style.alwaysAlignTextLeft = true;
  em.m_style.alignSpace = 0.625;
  em.m_style.blockHandle.ref = 0x1E3u;
  em.m_style.blockColor = 6;
  em.m_style.blockScale = DRW_Coord(1.0, 2.0, 3.0);
  em.m_style.blockScaleEnabled = true;
  em.m_style.blockRotation = 0.25;
  em.m_style.blockRotationEnabled = true;
  em.m_style.blockConnectionType = 1;
  em.m_style.scaleFactor = 1.5;
  em.m_style.propertyChanged = true;
  em.m_style.isAnnotative = true;
  em.m_style.breakSize = 0.125;
  em.m_style.attachmentDirection = 2;
  em.m_style.topAttachment = 9;
  em.m_style.bottomAttachment = 10;
  em.m_style.textExtended = true;

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("MLEADERSTYLE", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary styleDict;
    styleDict.handle = 0x1CFu;
    styleDict.parentHandle = 0;
    styleDict.cloning = 1;
    styleDict.m_entries.push_back({"Standard", 0x1D0u});
    w.setNamedDictObjects({styleDict});
    w.setRootDictEntries({{"ACAD_MLEADERSTYLE", "1CF"}});
    REQUIRE(w.write(&em, DRW::AC1021, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "MLEADERSTYLE"},
                        {"2", "AcDbMLeaderStyle"},
                        {"3", "ACDB_MLEADERSTYLE_CLASS"}}));
  CHECK(hasConsecutive(groups,
                       {{"3", "ACAD_MLEADERSTYLE"}, {"350", "1CF"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "MLEADERSTYLE"}, {"5", "1D0"}, {"330", "1CF"},
                        {"100", "AcDbMLeaderStyle"}, {"2", "Standard"}}));
  CHECK(hasConsecutive(groups,
                       {{"340", "1E0"}, {"92", "29"}, {"290", "1"}}));
  CHECK(hasConsecutive(groups,
                       {{"300", "Default leader text"}, {"342", "1E2"}}));
  CHECK(hasConsecutive(groups,
                       {{"343", "1E3"}, {"94", "6"}}));

  MLeaderStyleCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_MLeaderStyle &s = cap.m_captured;
  CHECK(s.handle == 0x1D0u);
  CHECK(s.parentHandle == 0x1CF);
  CHECK(s.name == "Standard");
  CHECK(s.flags == 4);
  CHECK(s.styleVersion == 2);
  CHECK(s.drawMLeaderOrder == 1);
  CHECK(s.maxLeaderPoints == 7);
  CHECK(s.leaderColor == 3);
  CHECK(s.leaderLineTypeHandle.ref == 0x1E0u);
  CHECK(s.landingGap == 1.25);
  CHECK(s.autoIncludeLanding == false);
  CHECK(s.description == "Round-trip MLeader style");
  CHECK(s.arrowHeadBlockHandle.ref == 0x1E1u);
  CHECK(s.textDefault == "Default leader text");
  CHECK(s.textStyleHandle.ref == 0x1E2u);
  CHECK(s.textFrameEnabled == true);
  CHECK(s.blockHandle.ref == 0x1E3u);
  CHECK(s.blockScale.z == 3.0);
  CHECK(s.isAnnotative == true);
  CHECK(s.bottomAttachment == 10);
  CHECK(s.textExtended == true);
}

TEST_CASE("DXF SPATIAL_FILTER is read into a DRW_SpatialFilter",
          "[dxf][spatialfilter][preservation]") {
  SpatialFilterCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSPATIAL_FILTER\n5\n1B0\n330\nC\n100\nAcDbFilter\n"
      "100\nAcDbSpatialFilter\n"
      "70\n3\n10\n0.0\n20\n0.0\n10\n10.0\n20\n0.0\n10\n10.0\n20\n10.0\n"
      "210\n0.0\n220\n0.0\n230\n1.0\n11\n1.0\n21\n2.0\n31\n0.0\n"
      "71\n1\n72\n0\n73\n1\n41\n5.5\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_spatialfilter_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_SpatialFilter &f = cap.m_captured;
  REQUIRE(f.m_boundaryPoints.size() == 3);
  CHECK(f.m_boundaryPoints[1].x == 10.0);
  CHECK(f.m_boundaryPoints[2].y == 10.0);
  CHECK(f.m_normal.z == 1.0);
  CHECK(f.m_origin.x == 1.0);
  CHECK(f.m_origin.y == 2.0);
  CHECK(f.m_displayBoundary == true);
  CHECK(f.m_clipFrontPlane == false);
  CHECK(f.m_clipBackPlane == true);
  CHECK(f.m_backDistance == 5.5);
}

TEST_CASE("DXF SPATIAL_FILTER object writes class and clip fields",
          "[dxf][spatialfilter][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_spatialfilter_write.dxf";
  std::filesystem::remove(path);

  class SpatialFilterEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_SpatialFilter m_filter;

    void writeObjects() override { m_rw->writeSpatialFilter(&m_filter); }
  };

  SpatialFilterEmitter em;
  em.m_filter.handle = 0x1B0u;
  em.m_filter.parentHandle = 0x1AFu;
  em.m_filter.m_boundaryPoints = {
      DRW_Coord(0.0, 0.0, 0.0),
      DRW_Coord(10.0, 0.0, 0.0),
      DRW_Coord(10.0, 10.0, 0.0)};
  em.m_filter.m_normal = DRW_Coord(0.0, 0.0, 1.0);
  em.m_filter.m_origin = DRW_Coord(1.0, 2.0, 0.0);
  em.m_filter.m_displayBoundary = true;
  em.m_filter.m_clipFrontPlane = true;
  em.m_filter.m_frontDistance = 2.25;
  em.m_filter.m_clipBackPlane = true;
  em.m_filter.m_backDistance = 5.5;
  em.m_filter.m_inverseInsertTransform = {
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 0.0, 1.0,
      3.0, 4.0, 5.0};
  em.m_filter.m_insertTransform = {
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 0.0, 1.0,
      6.0, 7.0, 8.0};

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("SPATIAL_FILTER", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary filterDict;
    filterDict.handle = 0x1AFu;
    filterDict.parentHandle = 0;
    filterDict.cloning = 1;
    filterDict.m_entries.push_back({"SPATIAL", 0x1B0u});
    w.setNamedDictObjects({filterDict});
    w.setRootDictEntries({{"ACAD_SPATIALFILTERS", "1AF"}});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "SPATIAL_FILTER"},
                        {"2", "AcDbSpatialFilter"},
                        {"3", "ObjectDBX Classes"}}));
  CHECK(hasConsecutive(groups,
                       {{"3", "ACAD_SPATIALFILTERS"}, {"350", "1AF"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "SPATIAL_FILTER"}, {"5", "1B0"},
                        {"330", "1AF"}, {"100", "AcDbFilter"},
                        {"100", "AcDbSpatialFilter"}, {"70", "3"}}));
  CHECK(hasConsecutive(groups,
                       {{"71", "1"}, {"72", "1"}, {"40", "2.25"},
                        {"73", "1"}, {"41", "5.5"}}));
  CHECK(hasConsecutive(groups, {{"40", "6"}, {"40", "7"}, {"40", "8"}}));

  SpatialFilterCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_SpatialFilter &f = cap.m_captured;
  CHECK(f.handle == 0x1B0u);
  CHECK(f.parentHandle == 0x1AF);
  REQUIRE(f.m_boundaryPoints.size() == 3);
  CHECK(f.m_boundaryPoints[1].x == 10.0);
  CHECK(f.m_boundaryPoints[2].y == 10.0);
  CHECK(f.m_normal.z == 1.0);
  CHECK(f.m_origin.x == 1.0);
  CHECK(f.m_origin.y == 2.0);
  CHECK(f.m_displayBoundary == true);
  CHECK(f.m_clipFrontPlane == true);
  CHECK(f.m_frontDistance == 2.25);
  CHECK(f.m_clipBackPlane == true);
  CHECK(f.m_backDistance == 5.5);
  REQUIRE(f.m_inverseInsertTransform.size() == 12);
  REQUIRE(f.m_insertTransform.size() == 12);
  CHECK(f.m_inverseInsertTransform[9] == 3.0);
  CHECK(f.m_inverseInsertTransform[10] == 4.0);
  CHECK(f.m_inverseInsertTransform[11] == 5.0);
  CHECK(f.m_insertTransform[9] == 6.0);
  CHECK(f.m_insertTransform[10] == 7.0);
  CHECK(f.m_insertTransform[11] == 8.0);
}

TEST_CASE("DXF IMAGEDEF_REACTOR is read into a DRW_ImageDefinitionReactor",
          "[dxf][imagedefreactor][preservation]") {
  ImageDefReactorCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nIMAGEDEF_REACTOR\n5\n1A0\n330\n1A\n100\nAcDbRasterImageDefReactor\n"
      "90\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_imagedefreactor_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_classVersion == 2);
}

namespace {
class SortEntsTableCapture : public StubInterface {
public:
  int m_callCount = 0;
  std::vector<DRW_RawDxfObject> m_rawObjects;
  DRW_SortEntsTable m_captured;
  void addSortEntsTable(const DRW_SortEntsTable &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
  void addRawDxfObject(const DRW_RawDxfObject &d) override {
    m_rawObjects.push_back(d);
  }
};
class DimAssocCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_DimensionAssociation m_captured;
  void addDimensionAssociation(const DRW_DimensionAssociation &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF SORTENTSTABLE is read into a DRW_SortEntsTable (draw order)",
          "[dxf][sortents][preservation]") {
  SortEntsTableCapture cap;
  // 5/330 before the 100 marker are the object's own handle/owner; after it,
  // 330=block owner and 331/5 are the entity/sort pairs.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSORTENTSTABLE\n5\n355\n330\n1A\n100\nAcDbSortentsTable\n"
      "330\n1F\n331\n35A\n5\n0\n331\n35B\n5\n356\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sortents_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_SortEntsTable &s = cap.m_captured;
  CHECK(s.m_blockOwnerHandle == 0x1Fu);
  REQUIRE(s.m_entityHandles.size() == 2);
  REQUIRE(s.m_sortHandles.size() == 2);
  CHECK(s.m_entityHandles[0] == 0x35Au);
  CHECK(s.m_entityHandles[1] == 0x35Bu);
  CHECK(s.m_sortHandles[0] == DRW::NoHandle);
  CHECK(s.m_sortHandles[1] == 0x356u);
  const auto rawSort = std::find_if(
      cap.m_rawObjects.begin(), cap.m_rawObjects.end(),
      [](const DRW_RawDxfObject &o) { return o.name == "SORTENTSTABLE"; });
  REQUIRE(rawSort != cap.m_rawObjects.end());
  CHECK(rawSort->handle == 0x355u);
  CHECK(rawSort->parentHandle == 0x1Au);
}

TEST_CASE("DXF SORTENTSTABLE rejects an incomplete pair",
          "[dxf][sortents][safety]") {
  SortEntsTableCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSORTENTSTABLE\n5\n355\n330\n1A\n100\nAcDbSortentsTable\n"
      "330\n1F\n331\n35A\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_sortents_incomplete_pair.dxf"));
  CHECK(cap.m_callCount == 0);
  CHECK(cap.m_rawObjects.empty());
}

TEST_CASE("DXF SORTENTSTABLE rejects a sort key without an entity",
          "[dxf][sortents][safety]") {
  SortEntsTableCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSORTENTSTABLE\n5\n355\n330\n1A\n100\nAcDbSortentsTable\n"
      "330\n1F\n5\n354\n"
      "0\nENDSEC\n0\nEOF\n";
  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_sortents_orphan_key.dxf"));
  CHECK(cap.m_callCount == 0);
  CHECK(cap.m_rawObjects.empty());
}

TEST_CASE("DXF SORTENTSTABLE object writes class and draw order",
          "[dxf][sortents][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_sortents_write.dxf";
  std::filesystem::remove(path);

  class SortEntsTableEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_SortEntsTable m_table;

    void writeObjects() override { m_rw->writeSortEntsTable(&m_table); }
  };

  SortEntsTableEmitter em;
  em.m_table.handle = 0x355u;
  em.m_table.parentHandle = 0x1Au;
  em.m_table.m_blockOwnerHandle = 0x1Fu;
  em.m_table.m_entityHandles = {0x35Au, 0x35Bu};
  em.m_table.m_sortHandles = {DRW::NoHandle, 0x356u};

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("SORTENTSTABLE", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary sortDict;
    sortDict.handle = 0x1Au;
    sortDict.parentHandle = 0;
    sortDict.cloning = 1;
    sortDict.m_entries.push_back({"DrawOrder", 0x355u});
    w.setNamedDictObjects({sortDict});
    w.setRootDictEntries({{"ACAD_SORTENTS", "1A"}});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"0", "CLASS"}, {"1", "SORTENTSTABLE"},
                        {"2", "AcDbSortentsTable"},
                        {"3", "ObjectDBX Classes"}}));
  CHECK(hasConsecutive(groups, {{"3", "ACAD_SORTENTS"}, {"350", "1A"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "SORTENTSTABLE"}, {"5", "355"},
                        {"330", "1A"}, {"100", "AcDbSortentsTable"},
                        {"330", "1F"}, {"331", "35A"}, {"5", "0"},
                        {"331", "35B"}, {"5", "356"}}));

  SortEntsTableCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  const DRW_SortEntsTable &s = cap.m_captured;
  CHECK(s.handle == 0x355u);
  CHECK(s.parentHandle == 0x1A);
  CHECK(s.m_blockOwnerHandle == 0x1Fu);
  REQUIRE(s.m_entityHandles.size() == 2);
  REQUIRE(s.m_sortHandles.size() == 2);
  CHECK(s.m_entityHandles[0] == 0x35Au);
  CHECK(s.m_entityHandles[1] == 0x35Bu);
  CHECK(s.m_sortHandles[0] == DRW::NoHandle);
  CHECK(s.m_sortHandles[1] == 0x356u);
  const auto rawSort = std::find_if(
      cap.m_rawObjects.begin(), cap.m_rawObjects.end(),
      [](const DRW_RawDxfObject &o) { return o.name == "SORTENTSTABLE"; });
  REQUIRE(rawSort != cap.m_rawObjects.end());
  CHECK(rawSort->handle == 0x355u);
}

TEST_CASE("DXF SORTENTSTABLE remaps source entity handles on write",
          "[dxf][sortents][objects]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_sortents_remap.dxf";
  std::filesystem::remove(path);

  class SortEntsRemapEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    DRW_SortEntsTable m_table;

    void writeEntities() override {
      DRW_Point first;
      first.basePoint = DRW_Coord(1.0, 2.0, 0.0);
      first.handle = 0x35Au;
      m_rw->writePoint(&first);
      DRW_Point second;
      second.basePoint = DRW_Coord(3.0, 4.0, 0.0);
      second.handle = 0x35Bu;
      m_rw->writePoint(&second);
    }
    void writeObjects() override { m_rw->writeSortEntsTable(&m_table); }
  };

  SortEntsRemapEmitter em;
  em.m_table.handle = 0x355u;
  em.m_table.parentHandle = 0;
  em.m_table.m_blockOwnerHandle = 0x1Fu;
  em.m_table.m_entityHandles = {0x35Au, 0x35Bu};
  em.m_table.m_sortHandles = {0x1Au, DRW::NoHandle};

  {
    dxfRW w(path.string().c_str());
    em.m_rw = &w;
    DRW_Class cls;
    REQUIRE(dxfRW::dxfClassForRecordName("SORTENTSTABLE", cls));
    cls.instanceCount = 1;
    w.setDxfClasses({cls});
    DRW_Dictionary sortKey;
    sortKey.handle = 0x1Au;
    sortKey.parentHandle = 0;
    sortKey.cloning = 1;
    w.setNamedDictObjects({sortKey});
    w.setRootDictEntries({{"SORT_KEY", "1A"}});
    REQUIRE(w.write(&em, DRW::AC1024, false));
  }

  const auto groups = readGroups(path);
  std::filesystem::remove(path);
  auto handlesForType = [](const auto &groupList, const std::string &typeName) {
    std::vector<std::string> handles;
    for (std::size_t i = 0; i < groupList.size(); ++i) {
      if (groupList[i].first != "0" || groupList[i].second != typeName)
        continue;
      for (std::size_t j = i + 1; j < groupList.size()
           && groupList[j].first != "0"; ++j) {
        if (groupList[j].first == "5") {
          handles.push_back(groupList[j].second);
          break;
        }
      }
    }
    return handles;
  };
  const auto pointHandles = handlesForType(groups, "POINT");
  REQUIRE(pointHandles.size() >= 2);
  CHECK(hasConsecutive(groups,
                       {{"0", "POINT"}, {"5", pointHandles[0]},
                        {"330", "1F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "POINT"}, {"5", pointHandles[1]},
                        {"330", "1F"}}));
  CHECK(hasConsecutive(groups,
                       {{"0", "SORTENTSTABLE"}, {"5", "355"},
                        {"330", "C"}, {"100", "AcDbSortentsTable"},
                        {"330", "1F"}, {"331", pointHandles[0]},
                        {"5", "1A"}, {"331", pointHandles[1]},
                        {"5", "0"}}));
}

TEST_CASE("DXF OBJECTS helper failures latch the enclosing write",
          "[dxf][objects][writer][safety]") {
  class HelperEmitter : public StubInterface {
  public:
    enum class Kind { Background, RenderSettings, Section, DbColor };

    dxfRW *m_rw = nullptr;
    Kind m_kind = Kind::Background;
    const char *m_recordName = nullptr;
    bool m_result = true;

    void writeObjects() override {
      switch (m_kind) {
      case Kind::Background: {
        DRW_Background value;
        m_result = m_rw->writeBackground(&value, m_recordName);
        break;
      }
      case Kind::RenderSettings: {
        DRW_RenderSettings value;
        m_result = m_rw->writeRenderSettings(&value, m_recordName);
        break;
      }
      case Kind::Section: {
        DRW_Section value;
        m_result = m_rw->writeSection(&value, m_recordName);
        break;
      }
      case Kind::DbColor: {
        DRW_DbColor value;
        m_result = m_rw->writeDbColor(&value, m_recordName);
        break;
      }
      }
    }
  };

  const std::array<HelperEmitter::Kind, 4> kinds = {
      HelperEmitter::Kind::Background,
      HelperEmitter::Kind::RenderSettings,
      HelperEmitter::Kind::Section,
      HelperEmitter::Kind::DbColor};
  const std::array<const char *, 2> invalidNames = {nullptr, ""};
  int index = 0;
  for (const HelperEmitter::Kind kind : kinds) {
    for (const char *recordName : invalidNames) {
      const auto path = std::filesystem::temp_directory_path() /
                        ("lc_dxf_object_helper_failure_" +
                         std::to_string(index++) + ".dxf");
      {
        std::ofstream previous(path);
        previous << "previous output\n";
      }

      HelperEmitter emitter;
      emitter.m_kind = kind;
      emitter.m_recordName = recordName;
      dxfRW writer(path.string().c_str());
      emitter.m_rw = &writer;
      CHECK_FALSE(writer.write(&emitter, DRW::AC1027, false));
      CHECK_FALSE(emitter.m_result);
      CHECK(slurp(path) == "previous output\n");
      std::error_code ignored;
      std::filesystem::remove(path, ignored);
    }
  }
}

TEST_CASE("DXF SORTENTSTABLE rejects unresolved remapped entity handles",
          "[dxf][sortents][writer][safety]") {
  class SortEntsFailureEmitter : public StubInterface {
  public:
    dxfRW *m_rw = nullptr;
    bool m_entityResult = true;
    bool m_sortResult = true;

    void writeEntities() override {
      DRW_Point point;
      point.handle = 0x123u;
      m_entityResult = m_rw->writePoint(&point);
    }

    void writeObjects() override {
      DRW_SortEntsTable table;
      table.handle = 0x355u;
      table.m_blockOwnerHandle = DRW::DxfModelSpaceBlockRecordHandle;
      table.m_entityHandles = {0x999u};
      table.m_sortHandles = {0x999u};
      m_sortResult = m_rw->writeSortEntsTable(&table);
    }
  };

  const auto path = std::filesystem::temp_directory_path() /
                    "lc_dxf_sortents_unresolved.dxf";
  {
    std::ofstream previous(path);
    previous << "previous output\n";
  }

  SortEntsFailureEmitter emitter;
  dxfRW writer(path.string().c_str());
  emitter.m_rw = &writer;
  CHECK_FALSE(writer.write(&emitter, DRW::AC1027, false));
  CHECK(emitter.m_entityResult);
  CHECK_FALSE(emitter.m_sortResult);
  CHECK(slurp(path) == "previous output\n");
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
}

TEST_CASE("DXF DIMASSOC is read into a DRW_DimensionAssociation",
          "[dxf][dimassoc][preservation]") {
  DimAssocCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDIMASSOC\n5\n521\n330\n1A\n100\nAcDbDimAssoc\n"
      "330\n500\n90\n3\n70\n0\n71\n0\n"
      "1\nAcDbOsnapPointRef\n72\n1\n331\n510\n"
      "1\nAcDbOsnapPointRef\n72\n7\n331\n511\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dimassoc_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_DimensionAssociation &a = cap.m_captured;
  CHECK(a.m_dimensionHandle == 0x500u);
  CHECK(a.m_associativityFlags == 3u);
  CHECK(a.m_isTransSpace == false);
  REQUIRE(a.m_osnapRefs.size() == 2);
  CHECK(a.m_osnapRefs[0].m_className == "AcDbOsnapPointRef");
  CHECK(a.m_osnapRefs[0].m_objectOsnapType == 1);
  CHECK(a.m_osnapRefs[0].m_objectHandle == 0x510u);
  CHECK(a.m_osnapRefs[1].m_objectOsnapType == 7);
  CHECK(a.m_osnapRefs[1].m_objectHandle == 0x511u);
  CHECK(a.m_hasUnrepresentableDetail);
}

TEST_CASE("DXF DIMASSOC rejects out-of-range byte fields",
          "[dxf][dimassoc][numeric]") {
  const auto makeDxf = [](const char *body) {
    return std::string(
               "0\nSECTION\n2\nOBJECTS\n"
               "0\nDIMASSOC\n5\n521\n330\n1A\n100\nAcDbDimAssoc\n")
           + body + "\n0\nENDSEC\n0\nEOF\n";
  };

  DimAssocCapture rotatedTypeCapture;
  CHECK_FALSE(tryReadDxf(makeDxf("70\n256"), rotatedTypeCapture,
                         "lc_dimassoc_rotated_type_overflow.dxf"));
  CHECK(rotatedTypeCapture.m_callCount == 0);

  DimAssocCapture osnapTypeCapture;
  CHECK_FALSE(tryReadDxf(
      makeDxf("1\nAcDbOsnapPointRef\n72\n256\n331\n510"),
      osnapTypeCapture, "lc_dimassoc_osnap_type_overflow.dxf"));
  CHECK(osnapTypeCapture.m_callCount == 0);
}

TEST_CASE("DXF DIMASSOC writer refuses lossy associations",
          "[dxf][dimassoc][writer][safety]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_dimassoc_lossy_write.dxf";
  const auto makeAssociation = [] {
    DRW_DimensionAssociation association;
    association.handle = 0x521;
    association.parentHandle = 0x1A;
    association.m_dimensionHandle = 0x500;
    association.m_associativityFlags = 0x01;
    association.m_osnapRefs.push_back({"AcDbOsnapPointRef", 0, 0x510});
    return association;
  };
  const auto verifyRejected = [&](DRW_DimensionAssociation association) {
    std::filesystem::remove(path);
    DimensionAssociationEmitter emitter;
    emitter.m_association = std::move(association);
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, DRW::AC1027, false));
    CHECK_FALSE(emitter.m_writeResult);
  };

  SECTION("unrepresentable DWG detail") {
    DRW_DimensionAssociation association = makeAssociation();
    association.m_hasUnrepresentableDetail = true;
    verifyRejected(std::move(association));
  }

  SECTION("osnap subtype requires a raw carrier") {
    DRW_DimensionAssociation association = makeAssociation();
    association.m_osnapRefs[0].m_objectOsnapType = 1;
    verifyRejected(std::move(association));
  }

  SECTION("active slots must match retained references") {
    DRW_DimensionAssociation association = makeAssociation();
    association.m_associativityFlags = 0x03;
    verifyRejected(std::move(association));
  }

  std::filesystem::remove(path);
}

TEST_CASE("DXF EVALUATION_GRAPH writer enforces native limits",
          "[dxf][evalgraph][writer][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_evaluation_graph_lossy_write.dxf";
  const auto makeGraph = [] {
    DRW_EvaluationGraph graph;
    graph.handle = 0x707;
    graph.parentHandle = 0xC;
    return graph;
  };
  const auto verifyRejected = [&](DRW::Version version,
                                  DRW_EvaluationGraph graph,
                                  const char *recordName = "EVALUATION_GRAPH") {
    std::filesystem::remove(path);
    EvaluationGraphEmitter emitter;
    emitter.m_graph = std::move(graph);
    emitter.m_recordName = recordName;
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    CHECK_FALSE(writer.write(&emitter, version, false));
    CHECK_FALSE(emitter.m_writeResult);
  };

  SECTION("unsupported DXF version") {
    verifyRejected(DRW::AC1018, makeGraph());
  }

  SECTION("node count exceeds the DWG-native limit") {
    DRW_EvaluationGraph graph = makeGraph();
    graph.m_nodes.resize(100001);
    verifyRejected(DRW::AC1027, std::move(graph));
  }

  SECTION("record name is required") {
    verifyRejected(DRW::AC1027, makeGraph(), "");
  }

  std::filesystem::remove(path);
}

namespace {
class BackgroundCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Background m_captured;
  void addBackground(const DRW_Background &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF GRADIENTBACKGROUND is read into a DRW_Background",
          "[dxf][background][preservation]") {
  BackgroundCapture cap;
  // 90 appears twice: class version, then color_top.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nGRADIENTBACKGROUND\n5\n2A0\n330\nC\n100\nAcDbGradientBackground\n"
      "90\n1\n90\n100\n91\n200\n92\n300\n140\n0.5\n141\n0.25\n142\n1.57\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_gradientbg_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Background &b = cap.m_captured;
  CHECK(b.m_kind == DRW_Background::Gradient);
  CHECK(b.m_classVersion == 1);
  CHECK(b.m_colorTop == 100);
  CHECK(b.m_colorMiddle == 200);
  CHECK(b.m_colorBottom == 300);
  CHECK(b.m_horizon == 0.5);
  CHECK(b.m_height == 0.25);
  CHECK(b.m_rotation == 1.57);
}

TEST_CASE("DXF IMAGEBACKGROUND is read into a DRW_Background",
          "[dxf][background][preservation]") {
  BackgroundCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nIMAGEBACKGROUND\n5\n2A1\n330\nC\n100\nAcDbImageBackground\n"
      "90\n1\n300\nsky.jpg\n290\n1\n291\n0\n292\n1\n"
      "140\n2.0\n141\n-3.0\n142\n1.5\n143\n0.75\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_imagebg_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Background &b = cap.m_captured;
  CHECK(b.m_kind == DRW_Background::Image);
  CHECK(b.m_fileName == "sky.jpg");
  CHECK(b.m_fitToScreen == true);
  CHECK(b.m_maintainAspect == false);
  CHECK(b.m_useTiling == true);
  CHECK(b.m_offset.x == 2.0);
  CHECK(b.m_offset.y == -3.0);
  CHECK(b.m_scale.x == 1.5);
  CHECK(b.m_scale.y == 0.75);
}

TEST_CASE("DXF SKYLIGHTBACKGROUND + SOLIDBACKGROUND read into DRW_Background",
          "[dxf][background][preservation]") {
  {
    BackgroundCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nSKYLIGHTBACKGROUND\n5\n2A2\n330\nC\n100\nAcDbSkyBackground\n"
        "90\n2\n340\n2BC\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_skybg_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_Background::Skylight);
    CHECK(cap.m_captured.m_classVersion == 2);
    CHECK(cap.m_captured.m_sunHandle == 0x2BCu);
  }
  {
    BackgroundCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nSOLIDBACKGROUND\n5\n2A3\n330\nC\n100\nAcDbSolidBackground\n"
        "90\n1\n90\n255\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_solidbg_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_Background::Solid);
    CHECK(cap.m_captured.m_classVersion == 1);
    CHECK(cap.m_captured.m_solidColor == 255);
  }
}

namespace {
class PointCloudDefCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_PointCloudDef m_captured;
  void addPointCloudDef(const DRW_PointCloudDef &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF POINTCLOUDDEFINITION is read into a DRW_PointCloudDef",
          "[dxf][pointcloud][preservation]") {
  PointCloudDefCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBPOINTCLOUDDEF\n5\n2D0\n330\nC\n100\nAcDbPointCloudDef\n"
      "90\n1\n1\nscan.rcp\n280\n1\n"
      "160\n1234567890123\n"
      "10\n-5.0\n20\n-6.0\n30\n0.0\n11\n5.0\n21\n6.0\n31\n2.0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointclouddef_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_PointCloudDef &p = cap.m_captured;
  CHECK(p.m_kind == DRW_PointCloudDef::Definition);
  CHECK(p.m_classVersion == 1);
  CHECK(p.m_sourceFilename == "scan.rcp");
  CHECK(p.m_isLoaded == true);
  CHECK(p.m_pointCount == 1234567890123ULL);
  CHECK(p.m_extentsMin.x == -5.0);
  CHECK(p.m_extentsMin.y == -6.0);
  CHECK(p.m_extentsMax.x == 5.0);
  CHECK(p.m_extentsMax.z == 2.0);
}

TEST_CASE("DXF POINTCLOUDDEFREACTOR is read (class version only)",
          "[dxf][pointcloud][preservation]") {
  PointCloudDefCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nPOINTCLOUDDEFREACTOR\n5\n2D1\n330\nC\n100\nAcDbPointCloudDefReactor\n"
      "90\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointcloudreactor_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.m_kind == DRW_PointCloudDef::Reactor);
  CHECK(cap.m_captured.m_classVersion == 2);
}

TEST_CASE("DXF Point Cloud definition variants preserve their class kind",
          "[dxf][pointcloud][preservation]") {
  PointCloudDefCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBPOINTCLOUDDEFEX\n5\n2D2\n330\nC\n100\nAcDbPointCloudDefEx\n"
      "90\n3\n1\nscan.rcp\n280\n0\n160\n42\n"
      "10\n-1.0\n20\n-2.0\n30\n-3.0\n11\n4.0\n21\n5.0\n31\n6.0\n"
      "0\nACDBPOINTCLOUDDEFREACTOREX\n5\n2D3\n330\nC\n100\nAcDbPointCloudDefReactorEx\n"
      "90\n4\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointclouddef_variants_read.dxf");
  REQUIRE(cap.m_callCount == 2);
  CHECK(cap.m_captured.m_kind == DRW_PointCloudDef::DefinitionEx);
  CHECK(cap.m_captured.m_pointCount == 42);
}

TEST_CASE("DXF Point Cloud definitions reject invalid class versions",
          "[dxf][pointcloud][malformed]") {
  PointCloudDefCapture cap;
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointclouddef_invalid.dxf";
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nOBJECTS\n"
        << "0\nPOINTCLOUDDEFINITION\n5\n2D4\n330\nC\n"
        << "100\nAcDbPointCloudDef\n90\n1000001\n"
        << "0\nENDSEC\n0\nEOF\n";
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, false));
  CHECK(cap.m_callCount == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF Point Cloud definitions are writable and readable",
          "[dxf][pointcloud][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *writer = nullptr;

    void writeObjects() override {
      REQUIRE(writer != nullptr);
      DRW_PointCloudDef definition;
      definition.handle = 0x2D5;
      definition.parentHandle = 0xC;
      definition.m_classVersion = 5;
      definition.m_sourceFilename = "write.rcp";
      definition.m_isLoaded = true;
      definition.m_pointCount = 9876543210ULL;
      definition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
      definition.m_extentsMax = DRW_Coord{4.0, 5.0, 6.0};
      REQUIRE(writer->writePointCloudDef(&definition));

      definition.m_kind = DRW_PointCloudDef::ReactorEx;
      definition.handle = 0x2D6;
      definition.m_classVersion = 6;
      definition.m_sourceFilename.clear();
      definition.m_pointCount = 0;
      REQUIRE(writer->writePointCloudDef(&definition));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointclouddef_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }

  PointCloudDefCapture cap;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&cap, false));
  REQUIRE(cap.m_callCount == 2);
  CHECK(cap.m_captured.m_kind == DRW_PointCloudDef::Definition);
  CHECK(cap.m_captured.m_pointCount == 9876543210ULL);
  CHECK(cap.m_captured.m_sourceFilename == "write.rcp");
  std::filesystem::remove(path);
}

namespace {
class PointCloudMetadataCapture : public StubInterface {
public:
  int m_navisworksCount = 0;
  int m_colorMapCount = 0;
  DRW_NavisworksModelDef m_navisworks;
  DRW_PointCloudColorMap m_colorMap;

  void addNavisworksModelDef(const DRW_NavisworksModelDef &data) override {
    m_navisworks = data;
    ++m_navisworksCount;
  }
  void addPointCloudColorMap(const DRW_PointCloudColorMap &data) override {
    m_colorMap = data;
    ++m_colorMapCount;
  }
};
} // namespace

TEST_CASE("DXF Navisworks and Point Cloud ColorMap metadata follows oracle layout",
          "[dxf][pointcloud][navisworks][preservation]") {
  PointCloudMetadataCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nNAVISWORKSMODELDEF\n5\n2E0\n330\nC\n"
      "100\nAcDbNavisworksModelDef\n70\n5\n1\ncoordination/model.nwd\n"
      "290\n1\n10\n-1.0\n20\n-2.0\n30\n-3.0\n"
      "11\n4.0\n21\n5.0\n31\n6.0\n291\n0\n"
      "0\nPOINTCLOUDCOLORMAP\n5\n2E1\n330\nC\n"
      "100\nAcDbPointCloudColorMap\n90\n1\n"
      "1\nintensity\n1\nelevation\n1\nclassify\n"
      "91\n2\n90\n3\n1\nred\n1\nblue\n"
      "70\n1\n91\n4\n90\n5\n1\ngreen\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_pointcloud_metadata_read.dxf");

  REQUIRE(cap.m_navisworksCount == 1);
  CHECK(cap.m_navisworks.m_flags == 5);
  CHECK(cap.m_navisworks.m_path == "coordination/model.nwd");
  CHECK(cap.m_navisworks.m_status);
  CHECK_FALSE(cap.m_navisworks.m_hostDrawingVisibility);
  CHECK(cap.m_navisworks.m_minExtent.z == -3.0);
  CHECK(cap.m_navisworks.m_maxExtent.x == 4.0);

  REQUIRE(cap.m_colorMapCount == 1);
  CHECK(cap.m_colorMap.m_classVersion == 1);
  CHECK(cap.m_colorMap.m_defaultIntensityColorScheme == "intensity");
  REQUIRE(cap.m_colorMap.m_colorRamps.size() == 1);
  CHECK(cap.m_colorMap.m_colorRamps[0].m_classVersion == 3);
  CHECK(cap.m_colorMap.m_colorRamps[0].m_rampCount == 2);
  CHECK(cap.m_colorMap.m_colorRamps[0].m_colorSchemes
        == std::vector<UTF8STRING>{"red", "blue"});
  REQUIRE(cap.m_colorMap.m_classificationColorRamps.size() == 1);
  CHECK(cap.m_colorMap.m_classificationColorRamps[0].m_classVersion == 5);
  CHECK(cap.m_colorMap.m_classificationColorRamps[0].m_rampCount == 4);
  CHECK(cap.m_colorMap.m_classificationColorRamps[0].m_colorSchemes
        == std::vector<UTF8STRING>{"green"});
}

TEST_CASE("DXF Navisworks and Point Cloud ColorMap metadata is writable",
          "[dxf][pointcloud][navisworks][write]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *writer = nullptr;

    void writeObjects() override {
      REQUIRE(writer != nullptr);
      DRW_NavisworksModelDef navisworks;
      navisworks.handle = 0x2E2;
      navisworks.parentHandle = 0xC;
      navisworks.m_flags = 5;
      navisworks.m_path = "model.nwd";
      navisworks.m_status = true;
      navisworks.m_minExtent = DRW_Coord{-1.0, -2.0, -3.0};
      navisworks.m_maxExtent = DRW_Coord{4.0, 5.0, 6.0};
      REQUIRE(writer->writeNavisworksModelDef(&navisworks));

      DRW_PointCloudColorMap colorMap;
      colorMap.handle = 0x2E3;
      colorMap.parentHandle = 0xC;
      colorMap.m_classVersion = 1;
      colorMap.m_defaultIntensityColorScheme = "intensity";
      colorMap.m_defaultElevationColorScheme = "elevation";
      colorMap.m_defaultClassificationColorScheme = "classify";
      colorMap.m_colorRamps.push_back({3, 2, {"red", "blue"}});
      colorMap.m_colorRampCount = 1;
      colorMap.m_classificationColorRamps.push_back({5, 1, {"green"}});
      colorMap.m_classificationColorRampCount = 1;
      REQUIRE(writer->writePointCloudColorMap(&colorMap));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointcloud_metadata_write.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1021, false));
  }
  PointCloudMetadataCapture cap;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&cap, false));
  CHECK(cap.m_navisworksCount == 1);
  CHECK(cap.m_colorMapCount == 1);
  CHECK(cap.m_navisworks.m_path == "model.nwd");
  CHECK(cap.m_colorMap.m_colorRamps[0].m_colorSchemes[1] == "blue");
  std::filesystem::remove(path);
}

TEST_CASE("DXF Point Cloud ColorMap rejects unbounded ramp counts",
          "[dxf][pointcloud][malformed]") {
  PointCloudMetadataCapture cap;
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_pointcloud_colormap_invalid.dxf";
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nOBJECTS\n"
        << "0\nPOINTCLOUDCOLORMAP\n5\n2E4\n330\nC\n"
        << "100\nAcDbPointCloudColorMap\n90\n1\n"
        << "1\ni\n1\ne\n1\nc\n91\n70000\n"
        << "0\nENDSEC\n0\nEOF\n";
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, false));
  CHECK(cap.m_colorMapCount == 0);
  std::filesystem::remove(path);
}

namespace {
class SunStudyCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_SunStudy m_captured;
  void addSunStudy(const DRW_SunStudy &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF SUNSTUDY is read into a DRW_SunStudy (scalar config)",
          "[dxf][sunstudy][preservation]") {
  SunStudyCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSUNSTUDY\n5\n2E0\n330\nC\n100\nAcDbSunStudy\n"
      "90\n1\n1\nStudy1\n2\nMy study\n3\nSet\n4\nSubset\n70\n0\n"
      "290\n1\n291\n0\n292\n1\n293\n1\n294\n0\n"
      "93\n100\n94\n200\n95\n10\n74\n5\n75\n4\n76\n2\n77\n2\n40\n0.5\n"
      "341\n2E1\n342\n2E2\n343\n2E3\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sunstudy_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_SunStudy &s = cap.m_captured;
  CHECK(s.m_classVersion == 1);
  CHECK(s.m_setupName == "Study1");
  CHECK(s.m_description == "My study");
  CHECK(s.m_outputType == 0);
  CHECK(s.m_sheetSetName == "Set");
  CHECK(s.m_sheetSubsetName == "Subset");
  CHECK(s.m_useSubset == true);
  CHECK(s.m_selectDatesFromCalendar == false);
  CHECK(s.m_selectRangeOfDates == true);
  CHECK(s.m_lockViewports == true);
  CHECK(s.m_labelViewports == false);
  CHECK(s.m_startTime == 100);
  CHECK(s.m_endTime == 200);
  CHECK(s.m_interval == 10);
  CHECK(s.m_viewportCount == 4);
  CHECK(s.m_rowCount == 2);
  CHECK(s.m_columnCount == 2);
  CHECK(s.m_spacing == 0.5);
  CHECK(s.m_viewHandle == 0x2E1u);
  CHECK(s.m_visualStyleHandle == 0x2E2u);
  CHECK(s.m_textStyleHandle == 0x2E3u);
}

TEST_CASE("DXF SUNSTUDY rejects out-of-range int16 fields",
          "[dxf][sunstudy][numeric]") {
  SunStudyCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSUNSTUDY\n5\n2E0\n330\nC\n100\nAcDbSunStudy\n"
      "90\n1\n70\n32768\n0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_sunstudy_out_of_range_int16.dxf"));
  CHECK(cap.m_callCount == 0);
}

namespace {
class MotionPathCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_MotionPath m_captured;

  void addMotionPath(const DRW_MotionPath &data) override {
    if (m_callCount == 0)
      m_captured = data;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF MOTIONPATH preserves positional fields and references",
          "[dxf][motionpath][preservation]") {
  MotionPathCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMOTIONPATH\n5\n2F0\n330\nC\n"
      "100\nAcDbMotionPath\n90\n2\n340\n61\n340\n62\n340\n63\n"
      "90\n120\n90\n24\n290\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_motionpath_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.handle == 0x2F0u);
  CHECK(cap.m_captured.parentHandle == 0xCu);
  CHECK(cap.m_captured.m_classVersion == 2);
  CHECK(cap.m_captured.m_cameraPathHandle == 0x61u);
  CHECK(cap.m_captured.m_targetPathHandle == 0x62u);
  CHECK(cap.m_captured.m_viewTableHandle == 0x63u);
  CHECK(cap.m_captured.m_frames == 120);
  CHECK(cap.m_captured.m_frameRate == 24);
  CHECK(cap.m_captured.m_cornerDeceleration);
}

namespace {
class PathCapture : public StubInterface {
public:
  int curveCalls = 0;
  int pointCalls = 0;
  DRW_CurvePath curve;
  DRW_PointPath point;

  void addCurvePath(const DRW_CurvePath &data) override {
    curve = data;
    ++curveCalls;
  }
  void addPointPath(const DRW_PointPath &data) override {
    point = data;
    ++pointCalls;
  }
};
} // namespace

TEST_CASE("DXF CURVEPATH and POINTPATH preserve fixed fields",
          "[dxf][path][preservation]") {
  PathCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBCURVEPATH\n5\n2F1\n330\nC\n"
      "100\nAcDbCurvePath\n90\n3\n340\n71\n"
      "0\nACDBPOINTPATH\n5\n2F2\n330\nC\n"
      "100\nAcDbPointPath\n90\n4\n10\n1.25\n20\n-2.5\n30\n3.75\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_path_read.dxf");
  REQUIRE(cap.curveCalls == 1);
  REQUIRE(cap.pointCalls == 1);
  CHECK(cap.curve.handle == 0x2F1u);
  CHECK(cap.curve.parentHandle == 0xCu);
  CHECK(cap.curve.m_classVersion == 3);
  CHECK(cap.curve.m_entityHandle == 0x71u);
  CHECK(cap.point.handle == 0x2F2u);
  CHECK(cap.point.parentHandle == 0xCu);
  CHECK(cap.point.m_classVersion == 4);
  CHECK(cap.point.m_point.x == 1.25);
  CHECK(cap.point.m_point.y == -2.5);
  CHECK(cap.point.m_point.z == 3.75);
}

namespace {
class ObjectPtrCapture : public StubInterface {
public:
  int calls = 0;
  DRW_ObjectPtr object;
  void addObjectPtr(const DRW_ObjectPtr &data) override {
    object = data;
    ++calls;
  }
};
} // namespace

TEST_CASE("DXF OBJECTPTR preserves common object handles",
          "[dxf][object-ptr][preservation]") {
  ObjectPtrCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nOBJECTPTR\n5\n2F3\n330\nC\n"
      "100\nAcDbObjectPtr\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_object_ptr_read.dxf");
  REQUIRE(cap.calls == 1);
  CHECK(cap.object.handle == 0x2F3u);
  CHECK(cap.object.parentHandle == 0xCu);
}

namespace {
class PersistentSubentityCapture : public StubInterface {
public:
  int calls = 0;
  int rawCalls = 0;
  DRW_AssociativeObject object;

  void addAssociativeObject(const DRW_AssociativeObject &data) override {
    object = data;
    ++calls;
  }
  void addRawDxfObject(const DRW_RawDxfObject &) override { ++rawCalls; }
};
} // namespace

namespace {
class AssociativeShellCapture : public StubInterface {
public:
  std::vector<DRW_AssociativeObject> objects;
  std::vector<DRW_RawDxfObject> raw;

  void addAssociativeObject(const DRW_AssociativeObject &data) override {
    objects.push_back(data);
  }

  void addRawDxfObject(const DRW_RawDxfObject &data) override {
    raw.push_back(data);
  }
};
} // namespace

TEST_CASE("DXF PERSUBENTMGR preserves subentity handle references",
          "[dxf][persistent-subentity-manager][preservation]") {
  PersistentSubentityCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nPERSUBENTMGR\n5\n2F6\n330\nC\n"
      "100\nAcDbPersSubentManager\n"
      "90\n2\n90\n0\n90\n2\n"
      "330\nD1\n330\nD2\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_persubentmgr_read.dxf");
  REQUIRE(cap.calls == 1);
  CHECK(cap.rawCalls == 1);
  CHECK(cap.object.handle == 0x2F6u);
  CHECK(cap.object.parentHandle == 0xCu);
  CHECK(cap.object.m_classVersion == 2u);
  CHECK(cap.object.m_persistentSubentityCount == 2u);
  REQUIRE(cap.object.m_persistentSubentityHandles.size() == 2u);
  CHECK(cap.object.m_persistentSubentityHandles[0] == 0xD1u);
  CHECK(cap.object.m_persistentSubentityHandles[1] == 0xD2u);
}

TEST_CASE("DXF ASSOC aliases preserve application groups and owner handles",
          "[dxf][associativity][shell]") {
  AssociativeShellCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nASSOCDEPENDENCY\n5\n60\n330\nC\n"
      "102\n{ACAD_REACTORS\n330\n61\n102\n}\n"
      "102\n{ACAD_XDICTIONARY\n360\n62\n102\n}\n"
      "100\nAcDbAssocDependency\n90\n7\n"
      "0\nACDBASSOCNETWORK\n5\n63\n330\nC\n100\nAcDbAssocNetwork\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_associative_shell_read.dxf");

  REQUIRE(cap.objects.size() == 2);
  REQUIRE(cap.raw.size() == 2);
  CHECK(cap.objects[0].m_recordName == "ASSOCDEPENDENCY");
  CHECK(cap.objects[0].handle == 0x60u);
  CHECK(cap.objects[0].parentHandle == 0xCu);
  REQUIRE(cap.objects[0].reactorHandles.size() == 1);
  CHECK(cap.objects[0].reactorHandles[0] == 0x61u);
  CHECK(cap.objects[0].xDictHandle == 0x62u);
  CHECK(cap.objects[0].appData.size() == 2);
  CHECK(cap.raw[0].groups.size() == 10);
  CHECK(cap.raw[0].parentHandle == 0xCu);
  CHECK(cap.objects[1].m_recordName == "ACDBASSOCNETWORK");
  CHECK(cap.objects[1].handle == 0x63u);
  CHECK(cap.objects[1].parentHandle == 0xCu);
}

namespace {
class PartialViewingIndexCapture : public StubInterface {
public:
  int calls = 0;
  DRW_PartialViewingIndex index;

  void addPartialViewingIndex(const DRW_PartialViewingIndex &data) override {
    index = data;
    ++calls;
  }
};
} // namespace

TEST_CASE("DXF PARTIAL_VIEWING_INDEX preserves positional entries",
          "[dxf][partial-viewing-index][preservation]") {
  PartialViewingIndexCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nPARTIAL_VIEWING_INDEX\n5\n2F4\n330\nC\n"
      "100\nAcDbPartialViewingIndex\n"
      "10\n0\n20\n1\n30\n2\n11\n3\n21\n4\n31\n5\n340\n91\n"
      "10\n-1\n20\n-2\n30\n-3\n11\n6\n21\n7\n31\n8\n340\n92\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_partial_viewing_index_read.dxf");
  REQUIRE(cap.calls == 1);
  CHECK(cap.index.handle == 0x2F4u);
  CHECK(cap.index.parentHandle == 0xCu);
  CHECK(cap.index.m_entryCount == 2u);
  CHECK(cap.index.m_hasEntries);
  REQUIRE(cap.index.m_entries.size() == 2u);
  CHECK(cap.index.m_entries[0].extentsMin.y == 1.0);
  CHECK(cap.index.m_entries[0].extentsMax.z == 5.0);
  CHECK(cap.index.m_entries[0].objectHandle == 0x91u);
  CHECK(cap.index.m_entries[1].extentsMin.x == -1.0);
  CHECK(cap.index.m_entries[1].objectHandle == 0x92u);
}

TEST_CASE("DXF PARTIAL_VIEWING_INDEX rejects incomplete entries",
          "[dxf][partial-viewing-index][safety]") {
  PartialViewingIndexCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nPARTIALVIEWINGINDEX\n5\n2F5\n330\nC\n"
      "100\nAcDbPartialViewingIndex\n10\n0\n20\n1\n30\n2\n"
      "0\nENDSEC\n0\nEOF\n";
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_partial_viewing_index_incomplete.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << dxf;
  }
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&cap, true));
  CHECK(cap.calls == 0);
  std::filesystem::remove(path);
}

namespace {
class RenderSettingsCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_RenderSettings m_captured;
  void addRenderSettings(const DRW_RenderSettings &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

TEST_CASE("DXF RENDERENVIRONMENT decodes named fog fields (positional)",
          "[dxf][rendersettings][preservation]") {
  RenderSettingsCapture cap;
  // 90 classVersion, 290×3 (fogEnabled, fogBgEnabled, envImgEnabled),
  // 280×3 RC fogColor, 40×4 fog densities/distances, 1 filename.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nRENDERENVIRONMENT\n5\n2F0\n330\nC\n100\nAcDbRenderEnvironment\n"
      "90\n1\n290\n1\n290\n0\n290\n1\n280\n10\n280\n20\n280\n30\n"
      "40\n0.1\n40\n0.9\n40\n5.0\n40\n50.0\n1\nbg.hdr\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_renderenv_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_RenderSettings &r = cap.m_captured;
  CHECK(r.m_kind == DRW_RenderSettings::Environment);
  CHECK(r.m_classVersion == 1);
  CHECK(r.m_fogEnabled == true);
  CHECK(r.m_fogBackgroundEnabled == false);
  CHECK(r.m_environmentImageEnabled == true);
  CHECK(r.m_fogColorR == 10);
  CHECK(r.m_fogColorG == 20);
  CHECK(r.m_fogColorB == 30);
  CHECK(r.m_fogDensityNear == 0.1);
  CHECK(r.m_fogDensityFar == 0.9);
  CHECK(r.m_fogDistanceNear == 5.0);
  CHECK(r.m_fogDistanceFar == 50.0);
  CHECK(r.m_name == "bg.hdr");
}

TEST_CASE("DXF RENDERGLOBAL + RENDERSETTINGS capture class version + vectors",
          "[dxf][rendersettings][preservation]") {
  {
    RenderSettingsCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nRENDERGLOBAL\n5\n2F1\n330\nC\n100\nAcDbRenderGlobal\n"
        "90\n1\n90\n2\n90\n3\n290\n1\n290\n0\n1\nout.png\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_renderglobal_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_RenderSettings::Global);
    CHECK(cap.m_captured.m_classVersion == 1);
    CHECK(cap.m_captured.m_procedure == 2);
    CHECK(cap.m_captured.m_destination == 3);
    CHECK(cap.m_captured.m_name == "out.png");
  }
  {
    RenderSettingsCapture cap;
    const char *dxf =
        "0\nSECTION\n2\nOBJECTS\n"
        "0\nRENDERSETTINGS\n5\n2F2\n330\nC\n100\nAcDbRenderSettings\n"
        "90\n1\n1\npreset\n70\n2\n40\n12.5\n"
        "0\nENDSEC\n0\nEOF\n";
    readDxf(dxf, cap, "lc_rendersettings_read.dxf");
    REQUIRE(cap.m_callCount == 1);
    CHECK(cap.m_captured.m_kind == DRW_RenderSettings::Settings);
    CHECK(cap.m_captured.m_classVersion == 1);
    CHECK(cap.m_captured.m_name == "preset");
    REQUIRE(cap.m_captured.m_doubles.size() == 1);
    CHECK(cap.m_captured.m_doubles[0] == 12.5);
  }
}

namespace {
class RenderSettingsAliasCapture : public StubInterface {
public:
  std::vector<DRW_RenderSettings::Kind> kinds;
  void addRenderSettings(const DRW_RenderSettings &data) override {
    kinds.push_back(data.m_kind);
  }
};
} // namespace

TEST_CASE("DXF render settings aliases dispatch all supported kinds",
          "[dxf][rendersettings][aliases]") {
  RenderSettingsAliasCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBRENDERGLOBAL\n5\n2F3\n330\nC\n"
      "100\nAcDbRenderGlobal\n90\n1\n"
      "0\nACDBRAPIDRTRENDERSETTINGS\n5\n2F4\n330\nC\n"
      "100\nAcDbRapidRTRenderSettings\n90\n2\n"
      "0\nACDBMENTALRAYRENDERSETTINGS\n5\n2F5\n330\nC\n"
      "100\nAcDbMentalRayRenderSettings\n90\n3\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_render_settings_aliases.dxf");
  REQUIRE(cap.kinds.size() == 3u);
  CHECK(cap.kinds[0] == DRW_RenderSettings::Global);
  CHECK(cap.kinds[1] == DRW_RenderSettings::RapidRT);
  CHECK(cap.kinds[2] == DRW_RenderSettings::MentalRay);
}

namespace {
class SectionCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_Section m_captured;
  void addSection(const DRW_Section &d) override {
    if (m_callCount == 0) m_captured = d;
    ++m_callCount;
  }
};
} // namespace

namespace {
class TypedObjectsCapture : public StubInterface {
public:
  int backgrounds = 0;
  int materials = 0;
  int renderSettings = 0;
  int sunStudies = 0;
  int motionPaths = 0;
  int curvePaths = 0;
  int pointPaths = 0;
  int objectPtrs = 0;
  int partialViewingIndexes = 0;
  int dbColors = 0;
  int dimensionAssociations = 0;
  int evaluationGraphs = 0;
  int sections = 0;
  int detailViewStyles = 0;
  int sectionViewStyles = 0;
  DRW_Background background;
  DRW_Material material;
  DRW_RenderSettings renderSettingsValue;
  DRW_SunStudy sunStudy;
  DRW_MotionPath motionPath;
  DRW_CurvePath curvePath;
  DRW_PointPath pointPath;
  DRW_ObjectPtr objectPtr;
  DRW_PartialViewingIndex partialViewingIndex;
  DRW_DbColor dbColor;
  DRW_DimensionAssociation dimensionAssociation;
  DRW_EvaluationGraph evaluationGraph;
  DRW_Section section;
  DRW_DetailViewStyle detailViewStyle;
  DRW_SectionViewStyle sectionViewStyle;

  void addBackground(const DRW_Background &data) override {
    background = data;
    ++backgrounds;
  }
  void addMaterial(const DRW_Material &data) override {
    material = data;
    ++materials;
  }
  void addRenderSettings(const DRW_RenderSettings &data) override {
    renderSettingsValue = data;
    ++renderSettings;
  }
  void addSunStudy(const DRW_SunStudy &data) override {
    sunStudy = data;
    ++sunStudies;
  }
  void addMotionPath(const DRW_MotionPath &data) override {
    motionPath = data;
    ++motionPaths;
  }
  void addCurvePath(const DRW_CurvePath &data) override {
    curvePath = data;
    ++curvePaths;
  }
  void addPointPath(const DRW_PointPath &data) override {
    pointPath = data;
    ++pointPaths;
  }
  void addObjectPtr(const DRW_ObjectPtr &data) override {
    objectPtr = data;
    ++objectPtrs;
  }
  void addPartialViewingIndex(
      const DRW_PartialViewingIndex &data) override {
    partialViewingIndex = data;
    ++partialViewingIndexes;
  }
  void addDbColor(const DRW_DbColor &data) override {
    dbColor = data;
    ++dbColors;
  }
  void addDimensionAssociation(
      const DRW_DimensionAssociation &data) override {
    dimensionAssociation = data;
    ++dimensionAssociations;
  }
  void addEvaluationGraph(const DRW_EvaluationGraph &data) override {
    evaluationGraph = data;
    ++evaluationGraphs;
  }
  void addSection(const DRW_Section &data) override {
    section = data;
    ++sections;
  }
  void addDetailViewStyle(const DRW_DetailViewStyle &data) override {
    detailViewStyle = data;
    ++detailViewStyles;
  }
  void addSectionViewStyle(const DRW_SectionViewStyle &data) override {
    sectionViewStyle = data;
    ++sectionViewStyles;
  }
};
} // namespace

TEST_CASE("DXF typed OBJECTS families write and read through production codec",
          "[dxf][objects][typed][roundtrip]") {
  class Emitter : public StubInterface {
  public:
    dxfRW *writer = nullptr;

    void writeObjects() override {
      DRW_Background background;
      background.handle = 0x701;
      background.parentHandle = 0xC;
      background.m_kind = DRW_Background::Image;
      background.m_classVersion = 7;
      background.m_fileName = "sky.png";
      background.m_fitToScreen = true;
      background.m_maintainAspect = true;
      background.m_useTiling = false;
      background.m_offset = {2.0, 3.0, 0.0};
      background.m_scale = {4.0, 5.0, 0.0};
      REQUIRE(writer->writeBackground(&background, "IMAGEBACKGROUND"));

      DRW_Material material;
      material.handle = 0x702;
      material.parentHandle = 0xC;
      material.m_name = "Steel";
      material.m_description = "Brushed steel";
      REQUIRE(writer->writeMaterial(&material));

      DRW_RenderSettings render;
      render.handle = 0x703;
      render.parentHandle = 0xC;
      render.m_kind = DRW_RenderSettings::Environment;
      render.m_longs = {8};
      render.m_strings = {"Studio", "sky.hdr", "description"};
      render.m_bools = {true, false, true};
      render.m_bytes = {1, 2, 3};
      render.m_doubles = {0.5, 1.5, 2.5, 3.5};
      REQUIRE(writer->writeRenderSettings(&render, "RENDERENVIRONMENT"));

      DRW_SunStudy sunStudy;
      sunStudy.handle = 0x704;
      sunStudy.parentHandle = 0xC;
      sunStudy.m_classVersion = 9;
      sunStudy.m_setupName = "Summer";
      sunStudy.m_description = "South facade";
      sunStudy.m_outputType = 2;
      sunStudy.m_selectDatesFromCalendar = true;
      sunStudy.m_selectRangeOfDates = true;
      sunStudy.m_startTime = 100;
      sunStudy.m_endTime = 200;
      sunStudy.m_interval = 15;
      sunStudy.m_spacing = 0.75;
      sunStudy.m_viewHandle = 0x711;
      sunStudy.m_visualStyleHandle = 0x712;
      sunStudy.m_textStyleHandle = 0x713;
      REQUIRE(writer->writeSunStudy(&sunStudy));

      DRW_MotionPath motionPath;
      motionPath.handle = 0x7041;
      motionPath.parentHandle = 0xC;
      motionPath.m_classVersion = 2;
      motionPath.m_cameraPathHandle = 0x761;
      motionPath.m_targetPathHandle = 0x762;
      motionPath.m_viewTableHandle = 0x763;
      motionPath.m_frames = 120;
      motionPath.m_frameRate = 24;
      motionPath.m_cornerDeceleration = true;
      REQUIRE(writer->writeMotionPath(&motionPath));

      DRW_CurvePath curvePath;
      curvePath.handle = 0x7042;
      curvePath.parentHandle = 0xC;
      curvePath.m_classVersion = 3;
      curvePath.m_entityHandle = 0x771;
      REQUIRE(writer->writeCurvePath(&curvePath));

      DRW_PointPath pointPath;
      pointPath.handle = 0x7043;
      pointPath.parentHandle = 0xC;
      pointPath.m_classVersion = 4;
      pointPath.m_point = {1.25, -2.5, 3.75};
      REQUIRE(writer->writePointPath(&pointPath));

      DRW_ObjectPtr objectPtr;
      objectPtr.handle = 0x7044;
      objectPtr.parentHandle = 0xC;
      REQUIRE(writer->writeObjectPtr(&objectPtr));

      DRW_PartialViewingIndex partialViewingIndex;
      partialViewingIndex.handle = 0x7045;
      partialViewingIndex.parentHandle = 0xC;
      partialViewingIndex.m_hasEntries = true;
      partialViewingIndex.m_entries.push_back(
          {{0.0, 1.0, 2.0}, {3.0, 4.0, 5.0}, 0x791});
      REQUIRE(writer->writePartialViewingIndex(&partialViewingIndex));

      DRW_DbColor color;
      color.handle = 0x705;
      color.parentHandle = 0xC;
      color.name = "SteelColor";
      color.bookName = "Company Colors";
      color.rgb = 0x102030;
      REQUIRE(writer->writeDbColor(&color));

      DRW_DimensionAssociation association;
      association.handle = 0x706;
      association.parentHandle = 0xC;
      association.m_dimensionHandle = 0x721;
      association.m_associativityFlags = 0x01;
      association.m_isTransSpace = true;
      association.m_rotatedDimensionType = 3;
      association.m_osnapRefs.push_back({"AcDbLine", 0, 0x722});
      REQUIRE(writer->writeDimensionAssociation(&association));

      DRW_EvaluationGraph graph;
      graph.handle = 0x707;
      graph.parentHandle = 0xC;
      graph.m_value96 = 96;
      graph.m_value97 = 97;
      graph.m_nodes.push_back({1, 2, 3, 0x731, 4, 5, 6, 7});
      graph.m_edges.push_back({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
      REQUIRE(writer->writeEvaluationGraph(&graph));

      DRW_Section section;
      section.handle = 0x708;
      section.parentHandle = 0xC;
      section.m_kind = DRW_Section::Settings;
      section.m_classVersion = 12;
      section.m_sectionType = 13;
      section.m_generationOptions = 14;
      section.m_destinationBlockHandle = 0x741;
      REQUIRE(writer->writeSection(&section, "SECTIONSETTINGS"));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_typed_objects_roundtrip.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1027, false));
  }

  TypedObjectsCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  std::filesystem::remove(path);

  REQUIRE(capture.backgrounds == 1);
  CHECK(capture.background.m_kind == DRW_Background::Image);
  CHECK(capture.background.m_fileName == "sky.png");
  CHECK(capture.background.m_offset.y == 3.0);
  CHECK(capture.background.m_scale.y == 5.0);
  REQUIRE(capture.materials == 1);
  CHECK(capture.material.m_name == "Steel");
  REQUIRE(capture.renderSettings == 1);
  CHECK(capture.renderSettingsValue.m_kind == DRW_RenderSettings::Environment);
  CHECK(capture.renderSettingsValue.m_bools.size() == 3);
  CHECK(capture.renderSettingsValue.m_bytes.size() == 3);
  REQUIRE(capture.sunStudies == 1);
  CHECK(capture.sunStudy.m_setupName == "Summer");
  CHECK(capture.sunStudy.m_viewHandle == 0x711u);
  REQUIRE(capture.motionPaths == 1);
  CHECK(capture.motionPath.m_cameraPathHandle == 0x761u);
  CHECK(capture.motionPath.m_targetPathHandle == 0x762u);
  CHECK(capture.motionPath.m_viewTableHandle == 0x763u);
  CHECK(capture.motionPath.m_frames == 120);
  CHECK(capture.motionPath.m_frameRate == 24);
  CHECK(capture.motionPath.m_cornerDeceleration);
  REQUIRE(capture.curvePaths == 1);
  CHECK(capture.curvePath.m_classVersion == 3);
  CHECK(capture.curvePath.m_entityHandle == 0x771u);
  REQUIRE(capture.pointPaths == 1);
  CHECK(capture.pointPath.m_classVersion == 4);
  CHECK(capture.pointPath.m_point.x == 1.25);
  CHECK(capture.pointPath.m_point.y == -2.5);
  CHECK(capture.pointPath.m_point.z == 3.75);
  REQUIRE(capture.objectPtrs == 1);
  CHECK(capture.objectPtr.handle == 0x7044u);
  CHECK(capture.objectPtr.parentHandle == 0xCu);
  REQUIRE(capture.partialViewingIndexes == 1);
  REQUIRE(capture.partialViewingIndex.m_entries.size() == 1u);
  CHECK(capture.partialViewingIndex.m_entries[0].objectHandle == 0x791u);
  CHECK(capture.partialViewingIndex.m_entries[0].extentsMax.z == 5.0);
  REQUIRE(capture.dbColors == 1);
  CHECK(capture.dbColor.rgb == 0x102030);
  CHECK(capture.dbColor.bookName == "Company Colors");
  CHECK(capture.dbColor.name == "SteelColor");
  REQUIRE(capture.dimensionAssociations == 1);
  CHECK(capture.dimensionAssociation.m_dimensionHandle == 0x721u);
  REQUIRE(capture.dimensionAssociation.m_osnapRefs.size() == 1);
  CHECK(capture.dimensionAssociation.m_osnapRefs[0].m_objectHandle == 0x722u);
  REQUIRE(capture.evaluationGraphs == 1);
  REQUIRE(capture.evaluationGraph.m_nodes.size() == 1);
  REQUIRE(capture.evaluationGraph.m_edges.size() == 1);
  CHECK(capture.evaluationGraph.m_nodes[0].m_expressionHandle == 0x731u);
  CHECK(capture.evaluationGraph.m_edges[0].m_value92e == 10);
  REQUIRE(capture.sections == 1);
  CHECK(capture.section.m_kind == DRW_Section::Settings);
  CHECK(capture.section.m_generationOptions == 14);
  CHECK(capture.section.m_destinationBlockHandle == 0x741u);
}

TEST_CASE("DXF EVALUATION_GRAPH rejects an incomplete node",
          "[dxf][objects][typed][safety]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_evaluation_graph_incomplete.dxf";
  std::filesystem::remove(path);
  {
    std::ofstream out(path);
    out << "0\nSECTION\n2\nOBJECTS\n"
           "0\nEVALUATION_GRAPH\n5\n709\n330\nC\n"
           "100\nAcDbEvalGraph\n96\n1\n97\n2\n"
           "91\n3\n93\n4\n95\n5\n360\n731\n"
           "92\n6\n92\n7\n92\n8\n"
           "0\nENDSEC\n0\nEOF\n";
  }

  TypedObjectsCapture capture;
  dxfRW reader(path.string().c_str());
  CHECK_FALSE(reader.read(&capture, false));
  CHECK(capture.evaluationGraphs == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF EVALUATION_GRAPH rejects malformed fixed-field order",
          "[dxf][evalgraph][parse][safety]") {
  const auto makeDxf = [](const std::string &body) {
    return std::string(
               "0\nSECTION\n2\nOBJECTS\n"
               "0\nEVALUATION_GRAPH\n5\n709\n330\nC\n"
               "100\nAcDbEvalGraph\n")
           + body + "0\nENDSEC\n0\nEOF\n";
  };
  const auto expectRejected = [&](const std::string &body,
                                  const char *fileName) {
    TypedObjectsCapture capture;
    CHECK_FALSE(tryReadDxf(makeDxf(body), capture, fileName));
    CHECK(capture.evaluationGraphs == 0);
  };
  const std::string completeNode =
      "96\n1\n97\n2\n"
      "91\n3\n93\n4\n95\n5\n360\n731\n"
      "92\n6\n92\n7\n92\n8\n92\n9\n";
  const std::string completeEdge =
      "92\n10\n93\n11\n94\n12\n91\n13\n91\n14\n"
      "92\n15\n92\n16\n92\n17\n92\n18\n92\n19\n";

  SECTION("node requires the expression handle") {
    expectRejected("96\n1\n97\n2\n"
                   "91\n3\n93\n4\n95\n5\n"
                   "92\n6\n92\n7\n92\n8\n92\n9\n",
                   "lc_evaluation_graph_missing_expression.dxf");
  }

  SECTION("node flags cannot be repeated") {
    expectRejected("96\n1\n97\n2\n"
                   "91\n3\n93\n4\n93\n5\n95\n5\n360\n731\n"
                   "92\n6\n92\n7\n92\n8\n92\n9\n",
                   "lc_evaluation_graph_duplicate_node_flags.dxf");
  }

  SECTION("edge requires its first edge reference") {
    expectRejected(completeNode + "92\n10\n93\n11\n"
                   "91\n13\n91\n14\n"
                   "92\n15\n92\n16\n92\n17\n92\n18\n92\n19\n",
                   "lc_evaluation_graph_missing_edge_reference.dxf");
  }

  SECTION("graph requires both leading values") {
    expectRejected(completeNode.substr(5) + completeEdge,
                   "lc_evaluation_graph_missing_leading_value.dxf");
  }
}

TEST_CASE("DXF EVALUATION_GRAPH writes null expression handles explicitly",
          "[dxf][evalgraph][writer][roundtrip]") {
  const auto path = std::filesystem::temp_directory_path()
                    / "lc_evaluation_graph_null_expression.dxf";
  std::filesystem::remove(path);

  EvaluationGraphEmitter emitter;
  emitter.m_graph.handle = 0x707;
  emitter.m_graph.parentHandle = 0xC;
  emitter.m_graph.m_nodes.push_back({1, 2, 3, 0, 4, 5, 6, 7});
  {
    dxfRW writer(path.string().c_str());
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1027, false));
    REQUIRE(emitter.m_writeResult);
  }

  const auto groups = readGroups(path);
  CHECK(hasConsecutive(groups,
                       {{"91", "1"}, {"93", "2"}, {"95", "3"},
                        {"360", "0"}, {"92", "4"}}));
  TypedObjectsCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  REQUIRE(capture.evaluationGraphs == 1);
  REQUIRE(capture.evaluationGraph.m_nodes.size() == 1);
  CHECK(capture.evaluationGraph.m_nodes[0].m_expressionHandle == 0);
  std::filesystem::remove(path);
}

TEST_CASE("DXF DETAILVIEWSTYLE rejects out-of-range byte fields",
          "[dxf][detailviewstyle][numeric]") {
  const auto makeDxf = [](int group, const char *value) {
    return std::string(
               "0\nSECTION\n2\nOBJECTS\n"
               "0\nACDBDETAILVIEWSTYLE\n5\n51\n330\n50\n"
               "100\nAcDbModelDocViewStyle\n70\n0\n"
               "100\nAcDbDetailViewStyle\n71\n")
           + std::to_string(group) + "\n280\n" + value
           + "\n0\nENDSEC\n0\nEOF\n";
  };

  TypedObjectsCapture identifierCapture;
  CHECK_FALSE(tryReadDxf(makeDxf(1, "256"), identifierCapture,
                         "lc_detailviewstyle_identifier_overflow.dxf"));
  CHECK(identifierCapture.detailViewStyles == 0);

  TypedObjectsCapture edgeCapture;
  CHECK_FALSE(tryReadDxf(makeDxf(4, "256"), edgeCapture,
                         "lc_detailviewstyle_edge_overflow.dxf"));
  CHECK(edgeCapture.detailViewStyles == 0);
}

TEST_CASE("DXF view styles preserve typed application groups",
          "[dxf][viewstyle][application-group]") {
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nACDBDETAILVIEWSTYLE\n5\n51\n330\n50\n"
      "102\n{ACAD_XDICTIONARY\n360\n52\n102\n}\n"
      "102\n{CUSTOM\n102\n{NESTED\n1\npayload\n102\n}\n102\n}\n"
      "100\nAcDbModelDocViewStyle\n70\n0\n"
      "100\nAcDbDetailViewStyle\n70\n1\n"
      "0\nACDBSECTIONVIEWSTYLE\n5\n61\n330\n60\n"
      "102\n{ACAD_REACTORS\n330\n62\n102\n}\n"
      "102\n{CUSTOM\n102\n{NESTED\n1\npayload\n102\n}\n102\n}\n"
      "100\nAcDbModelDocViewStyle\n70\n0\n"
      "100\nAcDbSectionViewStyle\n70\n1\n"
      "0\nENDSEC\n0\nEOF\n";
  TypedObjectsCapture capture;
  readDxf(dxf, capture, "lc_viewstyle_application_groups.dxf", false);

  REQUIRE(capture.detailViewStyles == 1);
  CHECK(capture.detailViewStyle.xDictHandle == 0x52u);
  CHECK(capture.detailViewStyle.appData.size() == 2);
  REQUIRE(capture.sectionViewStyles == 1);
  CHECK(capture.sectionViewStyle.reactorHandles
        == std::vector<std::uint32_t>{0x62u});
  CHECK(capture.sectionViewStyle.appData.size() == 2);
}

TEST_CASE("DXF SECTIONMANAGER is read into a DRW_Section (live + handles)",
          "[dxf][section][preservation]") {
  SectionCapture cap;
  // 330 before the 100 marker is the owner; 330s after are section handles.
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSECTIONMANAGER\n5\n300\n330\nC\n100\nAcDbSectionManager\n"
      "70\n1\n90\n2\n330\n3A0\n330\n3A1\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sectionmanager_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Section &s = cap.m_captured;
  CHECK(s.m_kind == DRW_Section::Manager);
  CHECK(s.m_isLive == true);
  CHECK(s.m_sectionCount == 2);
  REQUIRE(s.m_sectionHandles.size() == 2);
  CHECK(s.m_sectionHandles[0] == 0x3A0u);
  CHECK(s.m_sectionHandles[1] == 0x3A1u);
}

TEST_CASE("DXF SECTIONSETTINGS is read into a DRW_Section (type triple)",
          "[dxf][section][preservation]") {
  SectionCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSECTIONSETTINGS\n5\n301\n330\nC\n100\nAcDbSectionSettings\n"
      "90\n1\n91\n2\n90\n4\n331\n3B0\n"
      "0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_sectionsettings_read.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_Section &s = cap.m_captured;
  CHECK(s.m_kind == DRW_Section::Settings);
  CHECK(s.m_classVersion == 1);
  CHECK(s.m_sectionType == 2);
  CHECK(s.m_generationOptions == 4);
  CHECK(s.m_destinationBlockHandle == 0x3B0u);
}

TEST_CASE("DXF SECTIONSETTINGS preserves nested type and geometry settings",
          "[dxf][section][preservation]") {
  class Emitter : public StubInterface {
  public:
    dxfRW* writer = nullptr;

    void writeObjects() override {
      DRW_Section section;
      section.handle = 0x302;
      section.parentHandle = 0xC;
      section.m_kind = DRW_Section::Settings;
      section.m_currentType = 4;
      section.m_typeCount = 1;

      DRW_SectionTypeSettings type;
      type.m_type = 4;
      type.m_generation = 17;
      type.m_numSources = 1;
      type.m_sourceHandles.push_back(0x750);
      type.m_destinationBlockHandle = 0x741;
      type.m_destinationFile = "section-output.dwg";
      type.m_numGeometrySettings = 1;

      DRW_SectionGeometrySettings geometry;
      geometry.m_numGeometries = 4;
      geometry.m_hexIndex = 1;
      geometry.m_flags = 1;
      geometry.m_color = 1;
      geometry.m_rgbColor = 0;
      geometry.m_hasRgbColor = true;
      geometry.m_layer = "Defpoints";
      geometry.m_lineType = "Continuous";
      geometry.m_lineTypeScale = 1.1;
      geometry.m_plotStyle = "ByColor";
      geometry.m_lineWeight = 40;
      geometry.m_faceTransparency = 65535;
      geometry.m_edgeTransparency = 65534;
      geometry.m_hatchType = 65533;
      geometry.m_hatchPattern = "SOLID";
      geometry.m_hatchAngle = 0.25;
      geometry.m_hatchSpacing = 2.0;
      geometry.m_hatchScale = 1.5;
      type.m_geometry.push_back(geometry);
      section.m_types.push_back(type);

      REQUIRE(writer->writeSection(&section, "SECTIONSETTINGS"));
    }
  } emitter;

  const auto path = std::filesystem::temp_directory_path()
                    / "lc_sectionsettings_nested_roundtrip.dxf";
  std::filesystem::remove(path);
  {
    dxfRW writer(path.string().c_str());
    emitter.writer = &writer;
    REQUIRE(writer.write(&emitter, DRW::AC1027, false));
  }

  SectionCapture capture;
  dxfRW reader(path.string().c_str());
  REQUIRE(reader.read(&capture, false));
  std::filesystem::remove(path);

  REQUIRE(capture.m_callCount == 1);
  const DRW_Section& section = capture.m_captured;
  CHECK(section.m_currentType == 4);
  CHECK(section.m_typeCount == 1);
  REQUIRE(section.m_types.size() == 1);
  const DRW_SectionTypeSettings& type = section.m_types.front();
  CHECK(type.m_type == 4);
  CHECK(type.m_generation == 17);
  REQUIRE(type.m_sourceHandles.size() == 1);
  CHECK(type.m_sourceHandles.front() == 0x750u);
  CHECK(type.m_destinationBlockHandle == 0x741u);
  CHECK(type.m_destinationFile == "section-output.dwg");
  REQUIRE(type.m_geometry.size() == 1);
  const DRW_SectionGeometrySettings& geometry = type.m_geometry.front();
  CHECK(geometry.m_numGeometries == 4);
  CHECK(geometry.m_hexIndex == 1);
  CHECK(geometry.m_flags == 1);
  CHECK(geometry.m_hasRgbColor);
  CHECK(geometry.m_rgbColor == 0);
  CHECK(geometry.m_layer == "Defpoints");
  CHECK(geometry.m_lineType == "Continuous");
  CHECK(geometry.m_lineTypeScale == Catch::Approx(1.1));
  CHECK(geometry.m_plotStyle == "ByColor");
  CHECK(geometry.m_lineWeight == 40);
  CHECK(geometry.m_faceTransparency == 65535u);
  CHECK(geometry.m_edgeTransparency == 65534u);
  CHECK(geometry.m_hatchType == 65533u);
  CHECK(geometry.m_hatchAngle == Catch::Approx(0.25));
  CHECK(geometry.m_hatchSpacing == Catch::Approx(2.0));
  CHECK(geometry.m_hatchScale == Catch::Approx(1.5));
}

TEST_CASE("DXF SECTIONSETTINGS rejects out-of-range uint16 fields",
          "[dxf][section][numeric]") {
  const auto makeDxf = [](const char *value) {
    return std::string(
               "0\nSECTION\n2\nOBJECTS\n"
               "0\nSECTIONSETTINGS\n5\n301\n330\nC\n"
               "100\nAcDbSectionSettings\n90\n4\n91\n1\n"
               "1\nSectionTypeSettings\n90\n4\n91\n17\n92\n0\n"
               "1\n\n93\n1\n2\nSectionGeometrySettings\n"
               "90\n4\n91\n1\n92\n1\n62\n1\n8\nDefpoints\n"
               "6\nContinuous\n40\n1.1\n1\nByColor\n370\n40\n70\n")
           + value
           + "\n0\nENDSEC\n0\nEOF\n";
  };

  SectionCapture overflowCapture;
  CHECK_FALSE(tryReadDxf(makeDxf("65536"), overflowCapture,
                         "lc_sectionsettings_uint16_overflow.dxf"));
  CHECK(overflowCapture.m_callCount == 0);

  SectionCapture negativeCapture;
  CHECK_FALSE(tryReadDxf(makeDxf("-1"), negativeCapture,
                         "lc_sectionsettings_uint16_negative.dxf"));
  CHECK(negativeCapture.m_callCount == 0);
}

TEST_CASE("DXF SECTION records reject incomplete declared lists",
          "[dxf][section][safety]") {
  const auto readSection = [](const std::string& body, const char* name) {
    SectionCapture capture;
    CHECK_FALSE(tryReadDxf(
        "0\nSECTION\n2\nOBJECTS\n" + body
            + "0\nENDSEC\n0\nEOF\n",
        capture, name));
    CHECK(capture.m_callCount == 0);
  };

  readSection(
      "0\nSECTIONSETTINGS\n5\n301\n330\nC\n"
      "100\nAcDbSectionSettings\n90\n4\n91\n1\n"
      "1\nSectionTypeSettings\n90\n4\n91\n17\n92\n1\n"
      "1\nsection.dwg\n93\n0\n",
      "lc_sectionsettings_missing_source.dxf");
  readSection(
      "0\nSECTIONSETTINGS\n5\n302\n330\nC\n"
      "100\nAcDbSectionSettings\n90\n4\n91\n1\n"
      "1\nSectionTypeSettings\n90\n4\n91\n17\n92\n0\n"
      "1\nsection.dwg\n93\n1\n",
      "lc_sectionsettings_missing_geometry.dxf");
  readSection(
      "0\nSECTIONMANAGER\n5\n303\n330\nC\n"
      "100\nAcDbSectionManager\n70\n1\n90\n2\n330\n3A0\n",
      "lc_sectionmanager_missing_section.dxf");
  readSection(
      "0\nSECTIONSETTINGS\n5\n304\n330\nC\n"
      "100\nAcDbSectionSettings\n90\n4\n91\n-1\n"
      "90\n1\n",
      "lc_sectionsettings_negative_type_count.dxf");
}

TEST_CASE("DXF DBCOLOR rejects out-of-range color indices",
          "[dxf][objects][dbcolor][numeric]") {
  TypedObjectsCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDBCOLOR\n5\n704\n330\nC\n100\nAcDbColor\n"
      "62\n65536\n0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_dbcolor_out_of_range_index.dxf"));
  CHECK(cap.dbColors == 0);
}

TEST_CASE("DXF typed/raw DBCOLOR rejects malformed owner handles",
          "[dxf][objects][dbcolor][raw][malformed][handle]") {
  TypedObjectsCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDBCOLOR\n5\n704\n330\nnot-hex\n100\nAcDbColor\n"
      "62\n7\n0\nENDSEC\n0\nEOF\n";

  CHECK_FALSE(tryReadDxf(dxf, cap, "lc_dbcolor_bad_owner_handle.dxf"));
  CHECK(cap.dbColors == 0);
}

TEST_CASE("Ellipse extrusion transforms the OCS center and axis vector",
          "[dxf][entities][extrusion]") {
  DRW_Ellipse ellipse;
  ellipse.basePoint = DRW_Coord(10.0, 20.0, 0.0);
  ellipse.secPoint = DRW_Coord(3.0, 0.0, 0.0);
  ellipse.extPoint = DRW_Coord(0.0, 0.0, -1.0);
  ellipse.haveExtrusion = true;
  ellipse.staparam = 0.2;
  ellipse.endparam = 1.3;

  ellipse.applyExtrusion();

  // An OCS normal of (0, 0, -1) reverses OCS X while preserving Y.
  CHECK(ellipse.basePoint.x == -10.0);
  CHECK(ellipse.basePoint.y == 20.0);
  CHECK(ellipse.basePoint.z == 0.0);
  CHECK(ellipse.secPoint.x == -3.0);
  CHECK(ellipse.secPoint.y == 0.0);
  CHECK(ellipse.secPoint.z == 0.0);
  CHECK(ellipse.staparam == Catch::Approx(2.0 * 3.14159265358979323846 - 1.3));
  CHECK(ellipse.endparam == Catch::Approx(2.0 * 3.14159265358979323846 - 0.2));
}

TEST_CASE("DXF ellipse callbacks distinguish raw OCS from converted geometry",
          "[dxf][entities][extrusion]") {
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nELLIPSE\n5\n10\n100\nAcDbEntity\n8\n0\n100\nAcDbEllipse\n"
      "10\n10.0\n20\n20.0\n30\n0.0\n"
      "11\n3.0\n21\n0.0\n31\n0.0\n40\n0.5\n41\n0.2\n42\n1.3\n"
      "210\n0.0\n220\n0.0\n230\n-1.0\n"
      "0\nENDSEC\n0\nEOF\n";

  EllipseCapture raw;
  readDxf(dxf, raw, "lc_ellipse_raw_ocs.dxf", /*applyExtrusion=*/false);
  REQUIRE(raw.m_callCount == 1);
  CHECK(raw.m_captured.basePoint.x == 10.0);
  CHECK(raw.m_captured.secPoint.x == 3.0);
  CHECK(raw.m_captured.extPoint.z == -1.0);

  EllipseCapture converted;
  readDxf(dxf, converted, "lc_ellipse_converted_wcs.dxf",
          /*applyExtrusion=*/true);
  REQUIRE(converted.m_callCount == 1);
  CHECK(converted.m_captured.basePoint.x == -10.0);
  CHECK(converted.m_captured.secPoint.x == -3.0);
  CHECK(converted.m_captured.extPoint.z == -1.0);
  CHECK(converted.m_captured.staparam ==
        Catch::Approx(2.0 * 3.14159265358979323846 - 1.3));
  CHECK(converted.m_captured.endparam ==
        Catch::Approx(2.0 * 3.14159265358979323846 - 0.2));
}

TEST_CASE("DXF circle callbacks distinguish raw OCS from converted geometry",
          "[dxf][entities][extrusion]") {
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nCIRCLE\n5\n11\n100\nAcDbEntity\n8\n0\n100\nAcDbCircle\n"
      "10\n10.0\n20\n20.0\n30\n0.0\n40\n3.0\n"
      "210\n0.0\n220\n0.0\n230\n-1.0\n"
      "0\nENDSEC\n0\nEOF\n";

  CircleCapture raw;
  readDxf(dxf, raw, "lc_circle_raw_ocs.dxf", /*applyExtrusion=*/false);
  REQUIRE(raw.m_callCount == 1);
  CHECK(raw.m_captured.basePoint.x == 10.0);
  CHECK(raw.m_captured.radious == 3.0);
  CHECK(raw.m_captured.extPoint.z == -1.0);

  CircleCapture converted;
  readDxf(dxf, converted, "lc_circle_converted_wcs.dxf",
          /*applyExtrusion=*/true);
  REQUIRE(converted.m_callCount == 1);
  CHECK(converted.m_captured.basePoint.x == -10.0);
  CHECK(converted.m_captured.radious == 3.0);
  CHECK(converted.m_captured.extPoint.z == -1.0);
}

TEST_CASE("DXF arc conversion reverses an axial-negative partial sweep",
          "[dxf][entities][extrusion]") {
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nARC\n5\n12\n100\nAcDbEntity\n8\n0\n100\nAcDbCircle\n"
      "10\n10.0\n20\n20.0\n30\n0.0\n40\n3.0\n"
      "210\n0.0\n220\n0.0\n230\n-1.0\n"
      "100\nAcDbArc\n50\n30.0\n51\n120.0\n"
      "0\nENDSEC\n0\nEOF\n";

  ArcCapture raw;
  readDxf(dxf, raw, "lc_arc_raw_ocs.dxf", /*applyExtrusion=*/false);
  REQUIRE(raw.m_callCount == 1);
  CHECK(raw.m_captured.basePoint.x == 10.0);
  CHECK(raw.m_captured.staangle == Catch::Approx(3.14159265358979323846 / 6.0));
  CHECK(raw.m_captured.endangle == Catch::Approx(2.0 * 3.14159265358979323846 / 3.0));

  ArcCapture converted;
  readDxf(dxf, converted, "lc_arc_converted_wcs.dxf",
          /*applyExtrusion=*/true);
  REQUIRE(converted.m_callCount == 1);
  CHECK(converted.m_captured.basePoint.x == -10.0);
  // The reflected OCS frame reverses traversal. Arc stores an undirected
  // CCW sweep, so the converted endpoints are mirrored and swapped.
  CHECK(converted.m_captured.staangle ==
        Catch::Approx(3.14159265358979323846 / 3.0));
  CHECK(converted.m_captured.endangle ==
        Catch::Approx(5.0 * 3.14159265358979323846 / 6.0));
  CHECK(converted.m_captured.extPoint.z == -1.0);
}

TEST_CASE("DXF LWPolyline conversion retains raw elevation metadata",
          "[dxf][entities][extrusion]") {
  const char *dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLWPOLYLINE\n5\n13\n100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n"
      "90\n2\n70\n0\n38\n7.0\n"
      "10\n1.0\n20\n2.0\n42\n0.5\n"
      "10\n3.0\n20\n4.0\n42\n-0.5\n"
      "210\n0.0\n220\n0.0\n230\n-1.0\n"
      "0\nENDSEC\n0\nEOF\n";

  LWPolylineCapture raw;
  readDxf(dxf, raw, "lc_lwpolyline_raw_ocs.dxf", /*applyExtrusion=*/false);
  REQUIRE(raw.m_callCount == 1);
  REQUIRE(raw.m_captured.vertlist.size() == 2);
  CHECK(raw.m_captured.vertlist[0]->x == 1.0);
  CHECK(raw.m_captured.elevation == 7.0);
  CHECK(raw.m_captured.extPoint.z == -1.0);

  LWPolylineCapture converted;
  readDxf(dxf, converted, "lc_lwpolyline_converted_wcs.dxf",
          /*applyExtrusion=*/true);
  REQUIRE(converted.m_callCount == 1);
  REQUIRE(converted.m_captured.vertlist.size() == 2);
  CHECK(converted.m_captured.vertlist[0]->x == -1.0);
  CHECK(converted.m_captured.vertlist[0]->y == 2.0);
  CHECK(converted.m_captured.elevation == 7.0);
  CHECK(converted.m_captured.extPoint.z == -1.0);
  CHECK(converted.m_captured.vertlist[0]->bulge == 0.5);
  CHECK(converted.m_captured.vertlist[1]->bulge == -0.5);
}

namespace {
struct CapturedDimStyle {
  std::string m_name;
  double m_dimasz = -1.0;
  double m_dimtxt = -1.0;
  std::string m_dimblk;
  int m_dimtxtdirection = -1;
  int m_dimtxtdirection_code = 0;
};

class DimStyleCapture : public StubInterface {
public:
  std::vector<CapturedDimStyle> m_captured;
  void addDimStyle(const DRW_Dimstyle &d) override {
    CapturedDimStyle style;
    style.m_name = d.name;
    // Read through get() exactly like RS_FilterDXFRW::createDimStyle does.
    if (const DRW_Variant *var = d.get("$DIMASZ")) style.m_dimasz = var->d_val();
    if (const DRW_Variant *var = d.get("$DIMTXT")) style.m_dimtxt = var->d_val();
    if (const DRW_Variant *var = d.get("$DIMBLK")) style.m_dimblk = var->c_str();
    if (const DRW_Variant *var = d.get("$DIMTXTDIRECTION")) {
      style.m_dimtxtdirection = var->i_val();
      style.m_dimtxtdirection_code = var->code();
    }
    m_captured.push_back(style);
  }
};
} // namespace

// Issue #2723: processDimStyle reuses one DRW_Dimstyle for every record, so each
// record has to start from a pristine one. reset() leaves the $DIM override map
// and the optional string codes populated, and syncStructToVars keeps whatever
// the map already holds, so every style was imported with the first one's values.
TEST_CASE("DXF DIMSTYLE records keep their own values (issue #2723)",
          "[dxf][dimstyle]") {
  DimStyleCapture cap;
  const char *dxf =
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nDIMSTYLE\n5\n5F\n100\nAcDbSymbolTable\n70\n3\n"
      "0\nDIMSTYLE\n105\n27\n100\nAcDbDimStyleTableRecord\n2\nStandard\n70\n0\n"
      "5\nMYARROW\n41\n2.5\n140\n2.5\n"
      "0\nDIMSTYLE\n105\n28\n100\nAcDbDimStyleTableRecord\n2\nARCH_MM\n70\n0\n"
      "41\n100.0\n140\n80.0\n"
      "0\nDIMSTYLE\n105\n29\n100\nAcDbDimStyleTableRecord\n2\nEZ_M_100\n70\n0\n"
      "41\n0.25\n140\n0.25\n"
      "0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  readDxf(dxf, cap, "lc_dimstyle_multi_read.dxf");

  REQUIRE(cap.m_captured.size() == 3);
  CHECK(cap.m_captured[0].m_name == "Standard");
  CHECK(cap.m_captured[0].m_dimasz == 2.5);
  CHECK(cap.m_captured[0].m_dimtxt == 2.5);
  CHECK(cap.m_captured[0].m_dimblk == "MYARROW");

  // These came back as Standard's 2.5/2.5/MYARROW before the fix.
  CHECK(cap.m_captured[1].m_name == "ARCH_MM");
  CHECK(cap.m_captured[1].m_dimasz == 100.0);
  CHECK(cap.m_captured[1].m_dimtxt == 80.0);
  CHECK(cap.m_captured[1].m_dimblk.empty());

  CHECK(cap.m_captured[2].m_name == "EZ_M_100");
  CHECK(cap.m_captured[2].m_dimasz == 0.25);
  CHECK(cap.m_captured[2].m_dimtxt == 0.25);
  CHECK(cap.m_captured[2].m_dimblk.empty());
}

TEST_CASE("DXF DIMSTYLE text direction uses canonical R2010 group 295",
          "[dxf][dimstyle]") {
  const auto dimstyleDxf = [](int code, int value) {
    return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1024\n0\nENDSEC\n"
           "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nDIMSTYLE\n70\n1\n"
           "0\nDIMSTYLE\n105\n27\n2\nDIRECTION\n70\n0\n"
           + std::to_string(code) + "\n" + std::to_string(value)
           + "\n0\nENDTAB\n0\nENDSEC\n0\nEOF\n";
  };

  for (const int inputCode : {295, 294, 292}) {
    DimStyleCapture capture;
    const std::string dxf = dimstyleDxf(inputCode, 1);
    const std::string name = "lc_dimstyle_direction_" +
                             std::to_string(inputCode) + ".dxf";
    readDxf(dxf, capture, name.c_str());
    REQUIRE(capture.m_captured.size() == 1);
    CHECK(capture.m_captured.front().m_dimtxtdirection == 1);
    CHECK(capture.m_captured.front().m_dimtxtdirection_code == 295);
  }

  DimStyleCapture invalid;
  CHECK_FALSE(tryReadDxf(dimstyleDxf(295, 2), invalid,
                         "lc_dimstyle_direction_invalid.dxf"));
  CHECK(invalid.m_captured.empty());

  const auto writeAndRead = [](DRW::Version version, const char *name) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove(path);
    dxfRW writer(path.string().c_str());
    DimstyleDirectionEmitter emitter;
    emitter.m_rw = &writer;
    REQUIRE(writer.write(&emitter, version, false));
    CHECK(emitter.m_writeResult);
    const auto groups = readGroups(path);
    std::filesystem::remove(path);
    return groups;
  };

  const auto r2010 = writeAndRead(DRW::AC1024,
                                  "lc_dimstyle_direction_r2010.dxf");
  CHECK(recordHasConsecutive(r2010, "DIMSTYLE", "DIRECTION", {{"295", "1"}}));
  CHECK_FALSE(std::any_of(r2010.cbegin(), r2010.cend(),
                          [](const auto &group) { return group.first == "294"; }));

  const auto r2007 = writeAndRead(DRW::AC1021,
                                  "lc_dimstyle_direction_r2007.dxf");
  CHECK_FALSE(std::any_of(r2007.cbegin(), r2007.cend(),
                          [](const auto &group) { return group.first == "295"; }));
}
