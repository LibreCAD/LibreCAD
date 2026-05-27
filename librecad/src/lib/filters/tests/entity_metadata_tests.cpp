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
 * Entity-metadata sidecar tests — covers transparency / material /
 * plotStyle / shadow / visualstyle round-trip preservation through
 * RS_Entity. The libdxfrw read+write paths (parseCode + writeEntity)
 * are tested via in-memory DXF round-trip below.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "lc_dwgadvancedmetadata.h"
#include "rs_line.h"
#include "rs_vector.h"

#include "drw_entities.h"
#include "drw_objects.h"
#include "libdxfrw.h"

TEST_CASE("RS_Entity: sidecar defaults are zero", "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  REQUIRE(line.materialHandle() == 0u);
  REQUIRE(line.plotStyleHandle() == 0u);
  REQUIRE(line.shadowMode() == 0);
  REQUIRE(line.fullVisualStyleHandle() == 0u);
  REQUIRE(line.faceVisualStyleHandle() == 0u);
  REQUIRE(line.edgeVisualStyleHandle() == 0u);
}

TEST_CASE("RS_Entity: material/plotStyle/shadow round-trip",
          "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});

  line.setMaterialHandle(0xABCDu);
  line.setPlotStyleHandle(0x1234u);
  line.setShadowMode(2);

  REQUIRE(line.materialHandle() == 0xABCDu);
  REQUIRE(line.plotStyleHandle() == 0x1234u);
  REQUIRE(line.shadowMode() == 2);
}

TEST_CASE("RS_Entity: visual-style handles round-trip", "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  line.setVisualStyleHandles(0xAAAAu, 0xBBBBu, 0xCCCCu);
  REQUIRE(line.fullVisualStyleHandle() == 0xAAAAu);
  REQUIRE(line.faceVisualStyleHandle() == 0xBBBBu);
  REQUIRE(line.edgeVisualStyleHandle() == 0xCCCCu);
}

TEST_CASE("RS_Entity: copy ctor preserves sidecars", "[entity_metadata]") {
  RS_Line src(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  src.setMaterialHandle(0x77u);
  src.setPlotStyleHandle(0x88u);
  src.setShadowMode(3);
  src.setVisualStyleHandles(0x11u, 0x22u, 0x33u);

  RS_Line copy(src);
  REQUIRE(copy.materialHandle() == 0x77u);
  REQUIRE(copy.plotStyleHandle() == 0x88u);
  REQUIRE(copy.shadowMode() == 3);
  REQUIRE(copy.fullVisualStyleHandle() == 0x11u);
  REQUIRE(copy.faceVisualStyleHandle() == 0x22u);
  REQUIRE(copy.edgeVisualStyleHandle() == 0x33u);

  // Mutating the source must not affect the copy.
  src.setMaterialHandle(0u);
  REQUIRE(src.materialHandle() == 0u);
  REQUIRE(copy.materialHandle() == 0x77u);
}

TEST_CASE("RS_Entity: setting sidecars to zero clears them",
          "[entity_metadata]") {
  RS_Line line(RS_Vector{0., 0., 0.}, RS_Vector{1., 1., 0.});
  line.setMaterialHandle(0xFFu);
  line.setMaterialHandle(0u);
  REQUIRE(line.materialHandle() == 0u);
}

TEST_CASE("DWG advanced metadata caches raw and semantic sidecars",
          "[entity_metadata][dwg_metadata]") {
  LC_DwgAdvancedMetadata metadata;

  DRW_UnsupportedObject raw;
  raw.m_objectType = 501;
  raw.m_handle = 0x77u;
  raw.m_bodyBitSize = 128u;
  raw.m_objectOffset = 4096u;
  raw.m_objectSize = 32u;
  raw.m_isCustomClass = true;
  raw.m_recordName = "ACDBASSOCNETWORK";
  raw.m_className = "AcDbAssocNetwork";
  raw.m_rawBytes = {0x01u, 0x02u, 0x03u};
  metadata.addUnsupportedObject(raw);

  DRW_View view;
  view.name = "Camera A";
  view.handle = 0x90u;
  view.namedUCS_ID = 0x91u;
  view.baseUCS_ID = 0x92u;
  view.m_backgroundHandle = 0x93u;
  view.m_visualStyleHandle = 0x94u;
  view.m_sunHandle = 0x95u;
  view.m_liveSectionHandle = 0x96u;
  view.m_useDefaultLights = false;
  view.m_defaultLightingType = 2u;
  view.m_brightness = 0.25;
  view.m_contrast = 0.75;
  view.m_ambientColor = 7u;
  metadata.addView(view);

  DRW_MLeaderStyle style;
  style.handle = 0xA0u;
  style.parentHandle = 0xA1u;
  style.name = "CalloutStyle";
  style.styleVersion = 2u;
  style.contentType = 2u;
  style.leaderType = 1u;
  style.leaderLineTypeHandle.ref = 0xA2u;
  style.arrowHeadBlockHandle.ref = 0xA3u;
  style.textStyleHandle.ref = 0xA4u;
  style.blockHandle.ref = 0xA5u;
  style.arrowHeadSize = 0.75;
  style.textHeight = 2.5;
  style.scaleFactor = 3.0;
  style.isAnnotative = true;
  metadata.addMLeaderStyle(style);

  REQUIRE(metadata.rawObjects().size() == 1);
  CHECK(metadata.rawObjects().front().handle == 0x77u);
  CHECK(metadata.rawObjects().front().bodyBitSize == 128u);
  CHECK(metadata.rawObjects().front().rawBytes.size() == 3);

  const auto* foundView = metadata.findViewByName("Camera A");
  REQUIRE(foundView != nullptr);
  CHECK(foundView->namedUcsHandle == 0x91u);
  CHECK(foundView->visualStyleHandle == 0x94u);
  CHECK(foundView->sunHandle == 0x95u);
  CHECK(foundView->useDefaultLights == false);
  CHECK(foundView->defaultLightingType == 2u);
  CHECK(foundView->ambientColor == 7u);

  REQUIRE(metadata.mleaderStyles().size() == 1);
  const auto& capturedStyle = metadata.mleaderStyles().front();
  CHECK(capturedStyle.handle == 0xA0u);
  CHECK(capturedStyle.parentHandle == 0xA1u);
  CHECK(capturedStyle.name == "CalloutStyle");
  CHECK(capturedStyle.contentType == 2u);
  CHECK(capturedStyle.textStyleHandle == 0xA4u);
  CHECK(capturedStyle.blockHandle == 0xA5u);
  CHECK(capturedStyle.arrowHeadSize == 0.75);
  CHECK(capturedStyle.textHeight == 2.5);
  CHECK(capturedStyle.scaleFactor == 3.0);
  CHECK(capturedStyle.isAnnotative);

  REQUIRE(metadata.hasReplayableAdvancedObjects());
  metadata.invalidateByOwner(0xA1u);
  CHECK(capturedStyle.replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  metadata.invalidateByHandle(0x77u);
  CHECK(metadata.rawObjects().front().replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayInvalidated);
  CHECK(foundView->replayState ==
        LC_DwgAdvancedMetadata::ReplayState::ReplayAllowed);
}

TEST_CASE("DWG fixture manifest is valid JSON and optional by default",
          "[entity_metadata][dwg_fixtures]") {
  QFile manifest("libraries/libdxfrw/testdata/dwg-fixtures.json");
  REQUIRE(manifest.open(QIODevice::ReadOnly));
  QJsonParseError error;
  const QJsonDocument document =
      QJsonDocument::fromJson(manifest.readAll(), &error);
  REQUIRE(error.error == QJsonParseError::NoError);
  REQUIRE(document.isObject());

  const QJsonObject root = document.object();
  CHECK(root.value("version").toInt() == 1);
  const QJsonArray fixtures = root.value("fixtures").toArray();
  REQUIRE(!fixtures.isEmpty());
  const QJsonObject first = fixtures.first().toObject();
  CHECK(first.value("optional").toBool());
  CHECK(first.value("path").toString().contains("${HOME}"));
  CHECK(first.value("expect").toObject().value("preserveRawUnsupported").toBool());
}

namespace {

// Stub base that satisfies every DRW_Interface pure virtual with an
// empty body. Test-specific interfaces below override only what they
// need.
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

class SingleLineCapture : public StubInterface {
public:
  bool m_gotLine = false;
  DRW_Line m_captured;
  void addLine(const DRW_Line &d) override {
    if (!m_gotLine) {
      m_captured = d;
      m_gotLine = true;
    }
  }
};

class SingleLineEmitter : public StubInterface {
public:
  DRW_Line m_line;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeLine(&m_line); }
};

class SingleSplineEmitter : public StubInterface {
public:
  DRW_Spline m_spline;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeSpline(&m_spline); }
};

class SingleSplineCapture : public StubInterface {
public:
  bool m_gotSpline = false;
  DRW_Spline m_captured;
  void addSpline(const DRW_Spline *spline) override {
    if (!m_gotSpline && spline != nullptr) {
      m_captured = *spline;
      m_gotSpline = true;
    }
  }
};

class SinglePolylineEmitter : public StubInterface {
public:
  DRW_Polyline m_polyline;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writePolyline(&m_polyline); }
};

class SinglePolylineCapture : public StubInterface {
public:
  bool m_gotPolyline = false;
  DRW_Polyline m_captured;
  void addPolyline(const DRW_Polyline &polyline) override {
    if (!m_gotPolyline) {
      m_captured = polyline;
      m_gotPolyline = true;
    }
  }
};

} // namespace

TEST_CASE("DXF round-trip: 284/347/390/430/440 survive write+read",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_entity_metadata_roundtrip.dxf";
  std::filesystem::remove(path);

  SingleLineEmitter emitter;
  emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
  emitter.m_line.layer = "0";
  emitter.m_line.color = 256;                        // ByLayer
  emitter.m_line.transparency = (0x03 << 24) | 0x80; // 50% alpha
  emitter.m_line.material = 0xABCDu;
  emitter.m_line.plotStyle = 0x1234;
  emitter.m_line.shadow = static_cast<DRW::ShadowMode>(2); // ReceiveOnly
  emitter.m_line.colorName = "RAL$RAL 1003";

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false)); // R2007 ASCII
  }

  SingleLineCapture capture;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&capture, /*ext=*/true));
  }

  REQUIRE(capture.m_gotLine);
  CHECK(capture.m_captured.transparency == ((0x03 << 24) | 0x80));
  CHECK(capture.m_captured.material == 0xABCDu);
  CHECK(capture.m_captured.plotStyle == 0x1234);
  CHECK(static_cast<int>(capture.m_captured.shadow) == 2);
  CHECK(capture.m_captured.colorName == "RAL$RAL 1003");

  std::filesystem::remove(path);
}

TEST_CASE("DXF round-trip: default-valued entity emits no metadata codes",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_entity_metadata_default.dxf";
  std::filesystem::remove(path);

  SingleLineEmitter emitter;
  emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
  emitter.m_line.layer = "0";
  emitter.m_line.color = 256;
  // All metadata fields left at default sentinels.

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  // Read the file as text and assert the metadata group codes are
  // absent — confirms the skip-on-default write guards work.
  std::ifstream in(path);
  std::stringstream buf;
  buf << in.rdbuf();
  const std::string content = buf.str();

  // Each appears as "\n<code>\n..." in DXF ASCII. Only check codes
  // that are entity-unique — 284 collides with DIMSTYLE::dimtzin
  // and 390 is also emitted by LTYPE writes, so they appear in the
  // file regardless of entity content. 347/430/440 are entity-only
  // and prove the skip-on-default guards work.
  CHECK(content.find("\n347\n") == std::string::npos);
  CHECK(content.find("\n430\n") == std::string::npos);
  CHECK(content.find("\n440\n") == std::string::npos);

  std::filesystem::remove(path);
}

TEST_CASE("DXF write: app-data doubles preserve fractional values",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_entity_appdata_double.dxf";
  std::filesystem::remove(path);

  SingleLineEmitter emitter;
  emitter.m_line.basePoint = DRW_Coord(0.0, 0.0, 0.0);
  emitter.m_line.secPoint = DRW_Coord(1.0, 1.0, 0.0);
  emitter.m_line.layer = "0";
  emitter.m_line.color = 256;

  std::list<DRW_Variant> appData;
  appData.emplace_back(102, std::string{"APPDATA"});
  appData.emplace_back(40, 12.75);
  emitter.m_line.appData.push_back(appData);

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  std::ifstream in(path);
  std::stringstream buf;
  buf << in.rdbuf();
  const std::string content = buf.str();

  CHECK(content.find("\n102\n{APPDATA\n") != std::string::npos);
  CHECK(content.find("\n 40\n12.75\n") != std::string::npos);
  CHECK(content.find("\n102\n}\n") != std::string::npos);

  std::filesystem::remove(path);
}

TEST_CASE("DXF write: spline weights force rational flag",
          "[entity_metadata][dxf_roundtrip]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_spline_rational_flag.dxf";
  std::filesystem::remove(path);

  SingleSplineEmitter emitter;
  emitter.m_spline.layer = "0";
  emitter.m_spline.color = 256;
  emitter.m_spline.flags = 8; // planar only; writer should add rational bit
  emitter.m_spline.degree = 2;
  emitter.m_spline.knotslist = {0, 0, 0, 1, 1, 1};
  emitter.m_spline.controllist.push_back(std::make_shared<DRW_Coord>(0.0, 0.0, 0.0));
  emitter.m_spline.controllist.push_back(std::make_shared<DRW_Coord>(1.0, 1.0, 0.0));
  emitter.m_spline.controllist.push_back(std::make_shared<DRW_Coord>(2.0, 0.0, 0.0));
  emitter.m_spline.weightlist = {1.0, 0.5, 1.0};

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  SingleSplineCapture capture;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&capture, /*ext=*/true));
  }

  REQUIRE(capture.m_gotSpline);
  CHECK((capture.m_captured.flags & 0x04) == 0x04);
  REQUIRE(capture.m_captured.weightlist.size() == 3);
  CHECK(capture.m_captured.weightlist[1] == 0.5);

  std::filesystem::remove(path);
}

TEST_CASE("DXF round-trip: polyface vertex flags survive write+read",
          "[entity_metadata][dxf_roundtrip][polyline]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "librecad_polyface_vertex_flags.dxf";
  std::filesystem::remove(path);

  SinglePolylineEmitter emitter;
  emitter.m_polyline.layer = "0";
  emitter.m_polyline.color = 256;
  emitter.m_polyline.flags = 64;
  emitter.m_polyline.vertexcount = 3;
  emitter.m_polyline.facecount = 1;

  DRW_Vertex v1(0.0, 0.0, 0.0, 0.0);
  v1.flags = 64 | 128;
  DRW_Vertex v2(10.0, 0.0, 0.0, 0.0);
  v2.flags = 64 | 128;
  DRW_Vertex v3(0.0, 10.0, 0.0, 0.0);
  v3.flags = 64 | 128;
  DRW_Vertex face;
  face.flags = 128;
  face.vindex1 = 1;
  face.vindex2 = -2;
  face.vindex3 = 3;

  emitter.m_polyline.addVertex(v1);
  emitter.m_polyline.addVertex(v2);
  emitter.m_polyline.addVertex(v3);
  emitter.m_polyline.addVertex(face);

  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  SinglePolylineCapture capture;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&capture, /*ext=*/true));
  }

  REQUIRE(capture.m_gotPolyline);
  CHECK(capture.m_captured.flags == 64);
  REQUIRE(capture.m_captured.vertlist.size() == 4);
  CHECK(capture.m_captured.vertlist[0]->flags == (64 | 128));
  CHECK(capture.m_captured.vertlist[1]->flags == (64 | 128));
  CHECK(capture.m_captured.vertlist[2]->flags == (64 | 128));
  CHECK(capture.m_captured.vertlist[3]->flags == 128);
  CHECK(capture.m_captured.vertlist[3]->vindex1 == 1);
  CHECK(capture.m_captured.vertlist[3]->vindex2 == -2);
  CHECK(capture.m_captured.vertlist[3]->vindex3 == 3);

  std::filesystem::remove(path);
}
