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
 * DXF MULTILEADER CONTEXT_DATA{} parse tests.
 *
 * The DXF reader previously captured only entity-level MLEADER scalars; the
 * embedded CONTEXT_DATA{} block (leader roots/lines + text/block content) was
 * skipped, so DXF-sourced multileaders rendered nothing. DRW_MLeader now parses
 * the nested-block stream into `context`, reusing the same structure the DWG
 * path fills and the LC_MLeader renderer consumes.
 *
 * The fixture is a faithful reduction of a real ezdxf/AutoCAD-emitted
 * MULTILEADER (libredwg example_2010.dxf): the same group-code order, the
 * 300/302/304 open + 301/303/305 close markers, and the overloaded numeric
 * codes (40 = scale in CONTEXT, landingDistance in LEADER; 10/20/30 = content
 * base point, connection point, line vertex).
 */

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
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

class MLeaderCapture : public StubInterface {
public:
  int m_callCount = 0;
  DRW_MLeader m_captured;
  void addMLeader(const DRW_MLeader *d) override {
    if (m_callCount == 0 && d)
      m_captured = *d;
    ++m_callCount;
  }
};

class MLeaderEmitter : public StubInterface {
public:
  DRW_MLeader m_mleader;
  dxfRW *m_rw = nullptr;
  void writeEntities() override { m_rw->writeMultiLeader(&m_mleader); }
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

void seedMLeaderContext(DRW_MLeader &mleader) {
  mleader.overrideFlags = 279552;
  mleader.leaderType = 1;
  mleader.styleContentType = 2;
  mleader.landingEnabled = true;
  mleader.doglegEnabled = true;
  mleader.attachmentDirection = 0;
  mleader.styleTopAttach = 9;
  mleader.styleBottomAttach = 9;

  DRW_MLeaderAnnotContext &ctx = mleader.context;
  ctx.overallScale = 1.0;
  ctx.contentBasePoint = DRW_Coord(-2239.13, 607.22, 0.0);
  ctx.textHeight = 4.0;
  ctx.arrowHeadSize = 4.0;
  ctx.landingGap = 2.0;
  ctx.hasTextContents = true;
  ctx.textLabel = "N1";
  ctx.textNormal = DRW_Coord(0.0, 0.0, 1.0);
  ctx.textLocation = DRW_Coord(-2237.13, 609.22, 0.0);
  ctx.textDirection = DRW_Coord(1.0, 0.0, 0.0);
  ctx.boundaryWidth = 9.3;
  ctx.lineSpacingFactor = 1.0;
  ctx.lineSpacingStyle = 1;
  ctx.hasContentsBlock = false;
  ctx.basePoint = DRW_Coord(-2577.7, 142.43, 0.0);
  ctx.baseDirection = DRW_Coord(1.0, 0.0, 0.0);
  ctx.baseVertical = DRW_Coord(0.0, 1.0, 0.0);
  ctx.styleTopAttach = 9;
  ctx.styleBottomAttach = 9;

  DRW_MLeaderRoot root;
  root.isContentValid = true;
  root.unknown291 = true;
  root.connectionPoint = DRW_Coord(-2239.49, 607.22, 0.0);
  root.direction = DRW_Coord(1.0, 0.0, 0.0);
  root.landingDistance = 0.36;
  DRW_MLeaderLeaderLine line;
  line.points.push_back(DRW_Coord(-2577.7, 142.43, 0.0));
  line.leaderLineIndex = 0;
  root.leaderLines.push_back(line);
  ctx.roots.push_back(root);
}

const char *kMLeaderText =
    "0\nSECTION\n2\nENTITIES\n"
    "0\nMULTILEADER\n5\n640\n100\nAcDbEntity\n8\n0\n100\nAcDbMLeader\n"
    "270\n2\n"
    "300\nCONTEXT_DATA{\n"
    "40\n1.0\n10\n-2239.13\n20\n607.22\n30\n0.0\n"
    "41\n4.0\n140\n4.0\n145\n2.0\n"
    "290\n1\n304\nN1\n"
    "11\n0.0\n21\n0.0\n31\n1.0\n"
    "12\n-2237.13\n22\n609.22\n32\n0.0\n"
    "13\n1.0\n23\n0.0\n33\n0.0\n"
    "42\n0.0\n43\n9.3\n44\n0.0\n45\n1.0\n170\n1\n"
    "296\n0\n"
    "110\n-2577.7\n120\n142.43\n130\n0.0\n"
    "111\n1.0\n121\n0.0\n131\n0.0\n"
    "112\n0.0\n122\n1.0\n132\n0.0\n297\n0\n"
    "302\nLEADER{\n"
    "290\n1\n291\n1\n"
    "10\n-2239.49\n20\n607.22\n30\n0.0\n"
    "11\n1.0\n21\n0.0\n31\n0.0\n"
    "90\n0\n40\n0.36\n"
    "304\nLEADER_LINE{\n"
    "10\n-2577.7\n20\n142.43\n30\n0.0\n"
    "91\n0\n"
    "305\n}\n"
    "271\n0\n"
    "303\n}\n"
    "272\n9\n273\n9\n"
    "301\n}\n"
    "340\n2AF\n90\n279552\n170\n1\n91\n-1056964608\n171\n-2\n"
    "290\n1\n291\n1\n172\n2\n173\n1\n"
    "0\nENDSEC\n0\nEOF\n";

bool approx(double a, double b) { return std::abs(a - b) < 1e-6; }

} // namespace

TEST_CASE("DXF MULTILEADER CONTEXT_DATA parses text content + leader geometry",
          "[mleader][dxf][context]") {
  MLeaderCapture cap;
  readDxf(kMLeaderText, cap, "lc_mleader_ctx.dxf");
  REQUIRE(cap.m_callCount == 1);
  const DRW_MLeaderAnnotContext &ctx = cap.m_captured.context;

  SECTION("context-level common + text fields") {
    CHECK(approx(ctx.overallScale, 1.0));
    CHECK(approx(ctx.contentBasePoint.x, -2239.13));
    CHECK(approx(ctx.contentBasePoint.y, 607.22));
    CHECK(approx(ctx.textHeight, 4.0));
    CHECK(ctx.hasTextContents == true);
    CHECK(ctx.textLabel == "N1");
    CHECK(approx(ctx.textLocation.x, -2237.13));
    CHECK(approx(ctx.textLocation.y, 609.22));
    CHECK(approx(ctx.boundaryWidth, 9.3));
    CHECK(ctx.hasContentsBlock == false);
    CHECK(approx(ctx.basePoint.x, -2577.7));
  }

  SECTION("leader root + leader line geometry") {
    REQUIRE(ctx.roots.size() == 1);
    const DRW_MLeaderRoot &root = ctx.roots.at(0);
    CHECK(root.isContentValid == true);
    CHECK(approx(root.connectionPoint.x, -2239.49));
    CHECK(approx(root.connectionPoint.y, 607.22));
    CHECK(approx(root.direction.x, 1.0));
    CHECK(approx(root.landingDistance, 0.36));
    REQUIRE(root.leaderLines.size() == 1);
    const DRW_MLeaderLeaderLine &line = root.leaderLines.at(0);
    REQUIRE(line.points.size() == 1);
    CHECK(approx(line.points.at(0).x, -2577.7));
    CHECK(approx(line.points.at(0).y, 142.43));
    CHECK(line.leaderLineIndex == 0);
  }

  SECTION("entity-level fields after the context block still parse") {
    // 290/291/90/172 are overloaded with context codes; after 301 (state reset)
    // they must reach the entity-level switch, not the context.
    CHECK(cap.m_captured.overrideFlags == 279552);
    CHECK(cap.m_captured.leaderType == 1);
    CHECK(cap.m_captured.styleContentType == 2);
    CHECK(cap.m_captured.landingEnabled == true);
  }
}

TEST_CASE("DXF MULTILEADER CONTEXT_DATA writes text content + leader geometry",
          "[mleader][dxf][context][dxf_roundtrip]") {
  const auto path =
      std::filesystem::temp_directory_path() / "lc_mleader_ctx_write.dxf";
  std::filesystem::remove(path);

  MLeaderEmitter emitter;
  seedMLeaderContext(emitter.m_mleader);
  {
    dxfRW w(path.string().c_str());
    emitter.m_rw = &w;
    REQUIRE(w.write(&emitter, DRW::AC1021, false));
  }

  MLeaderCapture cap;
  {
    dxfRW r(path.string().c_str());
    REQUIRE(r.read(&cap, /*ext=*/true));
  }
  std::filesystem::remove(path);

  REQUIRE(cap.m_callCount == 1);
  CHECK(cap.m_captured.overrideFlags == 279552);
  CHECK(cap.m_captured.styleContentType == 2);
  const DRW_MLeaderAnnotContext &ctx = cap.m_captured.context;
  CHECK(approx(ctx.overallScale, 1.0));
  CHECK(approx(ctx.contentBasePoint.x, -2239.13));
  CHECK(approx(ctx.textHeight, 4.0));
  CHECK(ctx.hasTextContents == true);
  CHECK(ctx.textLabel == "N1");
  CHECK(approx(ctx.textLocation.x, -2237.13));
  CHECK(approx(ctx.boundaryWidth, 9.3));
  CHECK(ctx.hasContentsBlock == false);
  REQUIRE(ctx.roots.size() == 1);
  const DRW_MLeaderRoot &root = ctx.roots.at(0);
  CHECK(root.isContentValid == true);
  CHECK(approx(root.connectionPoint.x, -2239.49));
  CHECK(approx(root.landingDistance, 0.36));
  REQUIRE(root.leaderLines.size() == 1);
  const DRW_MLeaderLeaderLine &line = root.leaderLines.at(0);
  REQUIRE(line.points.size() == 1);
  CHECK(approx(line.points.at(0).x, -2577.7));
  CHECK(line.leaderLineIndex == 0);
}
