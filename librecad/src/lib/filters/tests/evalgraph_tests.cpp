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
 * DWG ACAD_EVALUATION_GRAPH (AcDbEvalGraph) object-body read test.
 *
 * Exercises DRW_EvaluationGraph::parseDwg via the OBJECTS-section custom-class
 * dispatch (dwgreader.cpp: rn=="ACAD_EVALUATION_GRAPH" || className=="AcDbEvalGraph").
 * The body layout (value96/97, nodes, edges) is version-agnostic and matches
 * dwgTs parseEvaluationGraph + LibreDWG DWG_OBJECT(EVALUATION_GRAPH); before this
 * increment parseDwg DECLINED for <=AC1018 (R2007+ handle-stream layout only), so
 * R2000/R2004 eval graphs were raw-only. The parser now reads the handle stream
 * inline for <=AC1018 (the same way DRW_Dictionary does) and from the separate
 * handle stream for R2007+, so the typed body decodes at every version.
 *
 * Fixtures:
 *   evalgraph_r2004.dwg    <- ~/doc/dwg6/example_2004.dwg (AC1018, R2004) — the
 *                             <=AC1018 fixture proving inline-handle decode.
 *   dynblock_r2018.dwg        (AC1032, R2018, already committed) — the R2007+
 *                             regression fixture (27 EVALUATION_GRAPH objects);
 *                             shared AC1032 kitchen-sink (~/doc/dwg6/makeall-plus.dwg).
 *
 * Oracle: dwgread -O JSON (LibreDWG). The single EVALUATION_GRAPH in
 * example_2004.dwg (handle [0,2,739]) decodes to:
 *   first_nodeid 1, first_nodeid_copy 1, num_nodes 1, num_edges 0
 *   node[0]: id 0, edge_flags 32, nextid 1, evalexpr handle 738, node [-1,-1,-1,-1]
 * (byte-identical to the R2000 example_2000.dwg oracle). LibreDWG documents
 * edge_flags as "always 32", asserted as a structural invariant on every node.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "drw_entities.h"
#include "drw_header.h"
#include "drw_objects.h"
#include "libdwgr.h"

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

// Collects every ACAD_EVALUATION_GRAPH delivered by the OBJECTS-section dispatch.
class EvalGraphCapture : public StubInterface {
public:
  std::vector<DRW_EvaluationGraph> m_graphs;
  void addEvaluationGraph(const DRW_EvaluationGraph &g) override {
    m_graphs.push_back(g);
  }
};

bool tryReadGraphs(const std::string &path, EvalGraphCapture &cap) {
  dwgR reader(path.c_str());
  const bool ok = reader.read(&cap, /*ext=*/true);
  if (!ok) return false;
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  return true;
}

} // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG EVALUATION_GRAPH decodes on R2004 / AC1018 (<=AC1018 inline handles)",
          "[dwg][evalgraph][parity]") {
  const std::string path = std::string(LIBRECAD_TEST_DIR) + "/evalgraph_r2004.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("evalgraph_r2004.dwg fixture not found; skipping");
    return;
  }

  EvalGraphCapture cap;
  // Fixture is committed (present past the is_regular_file gate), so a read
  // failure is a real regression -- REQUIRE it rather than SUCCEED-skipping.
  REQUIRE(tryReadGraphs(path, cap));

  // Exactly one EVALUATION_GRAPH, now delivered typed (was raw-only <=AC1018).
  REQUIRE(cap.m_graphs.size() == 1);
  const DRW_EvaluationGraph &g = cap.m_graphs.front();

  // Oracle (dwgread): first_nodeid == first_nodeid_copy == 1.
  CHECK(g.m_value96 == 1);
  CHECK(g.m_value97 == 1);

  // One node, zero edges. An exact body match here IS the no-desync gate: a
  // <=AC1018 handle-stream mis-seek would corrupt the expression handle and the
  // node counts.
  REQUIRE(g.m_nodes.size() == 1);
  CHECK(g.m_edges.empty());

  const DRW_EvaluationGraphNode &n = g.m_nodes.front();
  CHECK(n.m_index == 0);
  CHECK(n.m_flags == 32);        // LibreDWG: edge_flags "always 32"
  CHECK(n.m_nextNodeIndex == 1);
  CHECK(n.m_data1 == -1);
  CHECK(n.m_data2 == -1);
  CHECK(n.m_data3 == -1);
  CHECK(n.m_data4 == -1);
  // evalexpr hard-pointer resolves to absolute handle 0x2E2 (738) — read from
  // the inline handle stream on <=AC1018.
  CHECK(n.m_expressionHandle == 738u);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("DWG EVALUATION_GRAPH still decodes on R2018 / AC1032 (no >AC1018 regression)",
          "[dwg][evalgraph][parity]") {
  // Reuses the AC1032 makeall-plus kitchen-sink fixture (shared with the
  // dynamic-block suite) as an R2018 EVALUATION_GRAPH carrier.
  const std::string path = std::string(LIBRECAD_TEST_DIR) + "/dynblock_r2018.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("dynblock_r2018.dwg fixture not found; skipping");
    return;
  }

  EvalGraphCapture cap;
  // Fixture is committed (present past the is_regular_file gate), so a read
  // failure is a real regression -- REQUIRE it rather than SUCCEED-skipping.
  REQUIRE(tryReadGraphs(path, cap));

  // dwgread oracle: 27 EVALUATION_GRAPH objects in makeall-plus. The restructure
  // keeps the R2007+ separate-handle-stream path byte-identical, so every one
  // must still decode typed.
  REQUIRE(cap.m_graphs.size() == 27);

  // Every decoded node carries the LibreDWG "always 32" edge_flags invariant;
  // a desynced R2007+ handle-stream seek would scramble it.
  std::size_t totalNodes = 0;
  for (const auto &g : cap.m_graphs) {
    totalNodes += g.m_nodes.size();
    for (const auto &n : g.m_nodes)
      CHECK(n.m_flags == 32);
  }
  CHECK(totalNodes == 163);  // dwgread oracle: 163 nodes across the 27 graphs
}
