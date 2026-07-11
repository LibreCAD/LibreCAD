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

/**
 * Modern MESH (AcDbSubDMesh) write tests.
 *
 * This is distinct from legacy POLYLINE_MESH (fixed DWG type 30).  The mesh
 * below is level 0 and has no subdiv-vertex vector; that keeps the tests on the
 * field subset currently parsed unambiguously while still pinning base vertices,
 * face streams, edge pairs, and crease values.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

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

DRW_Mesh makeMesh() {
  DRW_Mesh mesh;
  mesh.version = 2;
  mesh.blendCrease = true;
  mesh.subdivisionLevel = 0;
  mesh.vertices = {
      DRW_Coord(0.0, 0.0, 0.0),
      DRW_Coord(4.0, 0.0, 0.0),
      DRW_Coord(4.0, 3.0, 0.0),
      DRW_Coord(0.0, 3.0, 0.0),
      DRW_Coord(2.0, 1.5, 2.0),
  };
  mesh.faces = {
      {0, 1, 2, 3},
      {0, 1, 4},
      {1, 2, 4},
      {2, 3, 4},
      {3, 0, 4},
  };
  mesh.edges = {
      {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4},
      {1, 4}, {2, 4}, {3, 4},
  };
  mesh.creases = {0.0, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75};
  return mesh;
}

class MeshCapture : public StubInterface {
public:
  int m_count = 0;
  int m_polylineCount = 0;
  DRW_Mesh m_captured;

  void addMesh(const DRW_Mesh &mesh) override {
    if (m_count == 0)
      m_captured = mesh;
    ++m_count;
  }

  void addPolyline(const DRW_Polyline &) override {
    ++m_polylineCount;
  }
};

void checkMesh(const MeshCapture &cap) {
  REQUIRE(cap.m_count == 1);
  CHECK(cap.m_polylineCount == 0);

  const DRW_Mesh &mesh = cap.m_captured;
  CHECK(mesh.eType == DRW::MESH);
  CHECK(mesh.version == 2);
  CHECK(mesh.blendCrease);
  CHECK(mesh.subdivisionLevel == 0);
  REQUIRE(mesh.vertices.size() == 5u);
  CHECK(mesh.vertices[2].x == Catch::Approx(4.0));
  CHECK(mesh.vertices[2].y == Catch::Approx(3.0));
  CHECK(mesh.vertices[4].z == Catch::Approx(2.0));
  REQUIRE(mesh.faces.size() == 5u);
  REQUIRE(mesh.faces[0].size() == 4u);
  CHECK(mesh.faces[0][0] == 0);
  CHECK(mesh.faces[0][3] == 3);
  REQUIRE(mesh.faces[2].size() == 3u);
  CHECK(mesh.faces[2][2] == 4);
  REQUIRE(mesh.edges.size() == 8u);
  CHECK(mesh.edges[4].first == 0);
  CHECK(mesh.edges[4].second == 4);
  REQUIRE(mesh.creases.size() == 8u);
  CHECK(mesh.creases[7] == Catch::Approx(1.75));
}

class MeshDxfEmitter : public StubInterface {
public:
  DRW_Mesh m_mesh = makeMesh();
  dxfRW *m_rw = nullptr;

  void writeEntities() override {
    REQUIRE(m_rw != nullptr);
    REQUIRE(m_rw->writeMesh(&m_mesh));
  }
};

class MeshDwgEmitter : public StubInterface {
public:
  DRW_Mesh m_mesh = makeMesh();
  dwgRW *m_writer = nullptr;

  void writeEntities() override {
    REQUIRE(m_writer != nullptr);
    REQUIRE(m_writer->writeMesh(&m_mesh));
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

std::string writeMeshDxf(const char *name) {
  const auto path = std::filesystem::temp_directory_path() / name;
  std::filesystem::remove(path);
  {
    dxfRW w(path.string().c_str());
    MeshDxfEmitter emitter;
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1015, false));
  }

  std::ifstream in(path);
  REQUIRE(in.good());
  std::ostringstream out;
  out << in.rdbuf();
  in.close();
  std::filesystem::remove(path);
  return out.str();
}

std::string tempPath(const char *suffix) {
  return (std::filesystem::temp_directory_path() /
          (std::string("mesh_") + suffix))
      .string();
}

} // namespace

TEST_CASE("dxfRW writes MESH as AcDbSubDMesh",
          "[dxf][mesh][write]") {
  const std::string dxf = writeMeshDxf("lc_mesh_write.dxf");
  std::string normalized = dxf;
  normalized.erase(std::remove(normalized.begin(), normalized.end(), '\r'),
                   normalized.end());

  CHECK(normalized.find("\n  0\nMESH\n") != std::string::npos);
  CHECK(normalized.find("\n100\nAcDbSubDMesh\n") != std::string::npos);
  CHECK(normalized.find("\n  0\nPOLYLINE\n") == std::string::npos);

  MeshCapture cap;
  readDxf(dxf, cap, "lc_mesh_write_read.dxf");
  checkMesh(cap);
}

TEST_CASE("dwgRW writes MESH as AcDbSubDMesh without AC1024 gating",
          "[dwg-write][mesh]") {
  const DRW::Version versions[] = {
      DRW::AC1015, DRW::AC1018, DRW::AC1024, DRW::AC1027, DRW::AC1032};

  for (DRW::Version version : versions) {
    INFO("version: " << static_cast<int>(version));
    const std::string suffix =
        std::string("write_") + std::to_string(static_cast<int>(version)) + ".dwg";
    const std::string path = tempPath(suffix.c_str());

    {
      dwgRW writer(path.c_str());
      MeshDwgEmitter emitter;
      emitter.m_writer = &writer;
      REQUIRE(writer.write(&emitter, version, /*bin=*/false));
    }

    MeshCapture cap;
    {
      dwgR reader(path.c_str());
      REQUIRE(reader.read(&cap, /*ext=*/true));
      REQUIRE(reader.getVersion() == version);
      REQUIRE(reader.getError() == DRW::BAD_NONE);
    }

    checkMesh(cap);
    std::remove(path.c_str());
  }
}
