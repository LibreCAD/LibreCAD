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
**********************************************************************/

/**
 * GUI-import repro for usa_dollar100_front.dwg (AC1021, 30 MB, 41364 3D
 * polylines / 1.16M vertices). The libdxfrw READ is clean (0 parse failures),
 * but the full RS_FilterDXFRW::fileImport -> RS_Graphic build path (what the
 * GUI uses) reportedly fails/crashes. This dev-local test drives exactly that
 * path so the failure can be reproduced and located.
 */

#include <catch2/catch_test_macros.hpp>

#include <QApplication>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "drw_interface.h"
#include "libdwgr.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_block.h"
#include "rs_layer.h"
#include "rs_settings.h"

namespace {
// Minimal counting DRW_Interface -- isolates raw libdxfrw DWG-parse time from
// the LibreCAD-side RS_Entity construction/graphic-assembly time (see the
// profiling test below). DRW_Interface is fully abstract (every method is a
// pure virtual), so every entry point must be overridden even though most are
// no-ops here -- only entity/block/layer counts are tracked.
class ProfilingCountIface : public DRW_Interface {
public:
  long long entities = 0;
  int blocks = 0;
  int layers = 0;

  // tables
  void addHeader(const DRW_Header *) override {}
  void addLType(const DRW_LType &) override {}
  void addLayer(const DRW_Layer &) override { ++layers; }
  void addDimStyle(const DRW_Dimstyle &) override {}
  void addVport(const DRW_Vport &) override {}
  void addTextStyle(const DRW_Textstyle &) override {}
  void addAppId(const DRW_AppId &) override {}

  // blocks
  void addBlock(const DRW_Block &) override { ++blocks; }
  void setBlock(int) override {}
  void endBlock() override {}

  // entities
  void addPoint(const DRW_Point &) override { ++entities; }
  void addLine(const DRW_Line &) override { ++entities; }
  void addRay(const DRW_Ray &) override { ++entities; }
  void addXline(const DRW_Xline &) override { ++entities; }
  void addArc(const DRW_Arc &) override { ++entities; }
  void addCircle(const DRW_Circle &) override { ++entities; }
  void addEllipse(const DRW_Ellipse &) override { ++entities; }
  void addLWPolyline(const DRW_LWPolyline &) override { ++entities; }
  void addPolyline(const DRW_Polyline &) override { ++entities; }
  void addSpline(const DRW_Spline *) override { ++entities; }
  void addKnot(const DRW_Entity &) override {}
  void addInsert(const DRW_Insert &) override { ++entities; }
  void addTrace(const DRW_Trace &) override { ++entities; }
  void add3dFace(const DRW_3Dface &) override { ++entities; }
  void addSolid(const DRW_Solid &) override { ++entities; }
  void addMText(const DRW_MText &) override { ++entities; }
  void addText(const DRW_Text &) override { ++entities; }
  void addDimAlign(const DRW_DimAligned *) override { ++entities; }
  void addDimLinear(const DRW_DimLinear *) override { ++entities; }
  void addDimRadial(const DRW_DimRadial *) override { ++entities; }
  void addDimDiametric(const DRW_DimDiametric *) override { ++entities; }
  void addDimAngular(const DRW_DimAngular *) override { ++entities; }
  void addDimAngular3P(const DRW_DimAngular3p *) override { ++entities; }
  void addDimOrdinate(const DRW_DimOrdinate *) override { ++entities; }
  void addDimArc(const DRW_DimArc *) override { ++entities; }
  void addLeader(const DRW_Leader *) override { ++entities; }
  void addHatch(const DRW_Hatch *) override { ++entities; }
  void addViewport(const DRW_Viewport &) override { ++entities; }
  void addImage(const DRW_Image *) override { ++entities; }
  void linkImage(const DRW_ImageDef *) override {}
  void addComment(const char *) override {}
  void addPlotSettings(const DRW_PlotSettings *) override {}

  // write callbacks (unused when reading)
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
} // namespace

// Profiling harness for arbitrary DWG files: reports (a) raw libdxfrw parse
// time via dwgR::read with a counting-only interface, and (b) the full GUI
// import path (RS_FilterDXFRW::fileImport) which additionally builds every
// RS_Entity and assembles the RS_Graphic. The gap between the two isolates
// where load time is actually spent. Run:
//   ./librecad_tests "[.dwg_profile]" -s
TEST_CASE("DWG load profiling: phase timing for an arbitrary file", "[.dwg_profile]") {
  const char *envPath = std::getenv("DWG_PROFILE_FILE");
  const char *home = std::getenv("HOME");
  std::string path = envPath ? envPath
                    : (home ? std::string(home) + "/doc/dwg4/\xe6\xa4\x8d\xe7\x89\xa9.dwg"
                            : std::string());
  if (path.empty() || !std::filesystem::is_regular_file(path)) {
    SKIP("profiling target not present; skipping (path=" << path << ")");
  }

  std::cout << "\n=== DWG load profiling: " << path << " ===\n";
  const auto fileSize = std::filesystem::file_size(path);
  std::cout << "file size: " << fileSize << " bytes\n";

  // DWG_PROFILE_LOOPS repeats both phases N times in this one process so a
  // statistical sampler (macOS `sample`/`xctrace`) has a long-lived PID to
  // attach to -- a single load is too fast (sub-second) for reliable sampling.
  int loops = 1;
  if (const char *loopEnv = std::getenv("DWG_PROFILE_LOOPS")) {
    loops = std::max(1, std::atoi(loopEnv));
  }
  std::cout << "loops: " << loops << "\n";

  for (int i = 0; i < loops; ++i) {
    // Phase A: raw libdxfrw parse only (no RS_Entity construction).
    {
      DRW::setCustomDebugPrinter(new DRW::DebugPrinter()); // silent
      ProfilingCountIface iface;
      dwgR reader(path.c_str());
      const auto t0 = std::chrono::steady_clock::now();
      const bool ok = reader.read(&iface, true);
      const auto t1 = std::chrono::steady_clock::now();
      const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
      std::cout << "\n[Phase A #" << i << "] libdxfrw dwgR::read only:\n";
      std::cout << "  ok=" << ok << "  entities=" << iface.entities
                << "  blocks=" << iface.blocks << "  layers=" << iface.layers
                << "  error=" << static_cast<int>(reader.getError())
                << "  version=" << static_cast<int>(reader.getVersion()) << "\n";
      std::cout << "  elapsed: " << ms << " ms\n";
    }

    // Phase B: full GUI import path -- libdxfrw parse + RS_Entity construction +
    // RS_Graphic assembly (blocks, layers, inserts). This is what the app does
    // when a user opens the file.
    {
      static int qargc = 1;
      static char qarg0[] = "librecad_tests";
      static char *qargv[] = {qarg0, nullptr};
      static QApplication *qapp = [] {
        auto *existing = qobject_cast<QApplication *>(QCoreApplication::instance());
        return existing ? existing : new QApplication(qargc, qargv);
      }();
      static bool settingsReady = [] {
        QApplication::setOrganizationName("LibreCAD");
        QApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
      }();
      (void)qapp;
      (void)settingsReady;
      // Avoid the D_DEBUGGING per-entity vfprintf+fflush skewing the timing.
      RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

      RS_Graphic graphic;
      RS_FilterDXFRW filter;
      const auto t0 = std::chrono::steady_clock::now();
      const bool imported =
          filter.fileImport(graphic, QString::fromStdString(path), RS2::FormatDWG);
      const auto t1 = std::chrono::steady_clock::now();
      const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

      long long top = 0;
      for (auto *e : graphic) if (e) ++top;
      const unsigned nBlocks = graphic.countBlocks();
      long long blockEnts = 0;
      for (unsigned b_i = 0; b_i < nBlocks; ++b_i) {
        auto *b = graphic.blockAt(b_i);
        if (!b) continue;
        for (auto *e : *b) if (e) ++blockEnts;
      }

      std::cout << "\n[Phase B #" << i << "] full GUI import (RS_FilterDXFRW::fileImport):\n";
      std::cout << "  imported=" << imported << "  top-level=" << top
                << "  blocks=" << nBlocks << "  block-entities=" << blockEnts << "\n";
      std::cout << "  elapsed: " << ms << " ms\n";
      REQUIRE(imported);
    }
  }
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("GUI import of usa_dollar100_front.dwg completes", "[.dwg6_gui_import]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg6/usa_dollar100_front.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("usa_dollar100_front.dwg not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QApplication *qapp = [] {
    auto *existing = qobject_cast<QApplication*>(QCoreApplication::instance());
    return existing ? existing : new QApplication(qargc, qargv);
  }();
  static bool settingsReady = [] {
    QApplication::setOrganizationName("LibreCAD");
    QApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  // Measure the true GUI cost at the default (non-verbose) log level -- the
  // test singleton otherwise inits at D_DEBUGGING, which floods a per-entity
  // vfprintf+fflush over 1.16M entities and dwarfs the real work.
  RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  const bool imported =
      filter.fileImport(graphic, QString::fromStdString(path), RS2::FormatDWG);
  REQUIRE(imported);

  int top = 0;
  int polylines = 0;
  for (auto *e : graphic) {
    if (!e)
      continue;
    ++top;
    if (e->rtti() == RS2::EntityPolyline)
      ++polylines;
  }
  INFO("top-level entities: " << top << " polylines: " << polylines);
  // dwgread oracle: 41364 3D polylines + 4 LWPOLYLINE. The import must build
  // them all -- and (the point of this test) it must do so in seconds, not the
  // ~10 minutes it took before RS_Polyline::addVertex stopped calling
  // endPolyline() (a full O(N) calculateBorders) on every vertex (O(N^2)).
  CHECK(polylines >= 41368);
}

TEST_CASE("GUI import of makeall-plus.dwg counts entities", "[.dwg_makeall_gui]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg3/makeall-plus.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("makeall-plus.dwg not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QApplication *qapp = [] {
    auto *existing = qobject_cast<QApplication*>(QCoreApplication::instance());
    return existing ? existing : new QApplication(qargc, qargv);
  }();
  static bool settingsReady = [] {
    QApplication::setOrganizationName("LibreCAD");
    QApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  const bool imported =
      filter.fileImport(graphic, QString::fromStdString(path), RS2::FormatDWG);
  REQUIRE(imported);

  int nTop = 0;
  int nBlocks = 0;
  int nBlockEnts = 0;
  for (auto *e : graphic) {
    if (e) ++nTop;
  }
  nBlocks = graphic.countBlocks();
  for (unsigned i = 0; i < nBlocks; ++i) {
    auto* b = graphic.blockAt(i);
    if (b) {
      for (auto *e : *b) {
        if (e) ++nBlockEnts;
      }
    }
  }
  INFO("top-level entities: " << nTop << " blocks: " << nBlocks << " block entities: " << nBlockEnts);
  std::cout << "\n=== makeall-plus.dwg GUI import ===\n";
  std::cout << "top-level: " << nTop << "\n";
  std::cout << "blocks: " << nBlocks << "\n";
  std::cout << "entities in blocks: " << nBlockEnts << "\n";
  CHECK(nTop > 0);
}

TEST_CASE("GUI import of visualization_condominium.dwg counts entities", "[.dwg_condo]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg/visualization_-_condominium_with_skylight.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("condo dwg not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QApplication *qapp = [] {
    auto *existing = qobject_cast<QApplication*>(QCoreApplication::instance());
    return existing ? existing : new QApplication(qargc, qargv);
  }();
  static bool settingsReady = [] {
    QApplication::setOrganizationName("LibreCAD");
    QApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  const bool imported =
      filter.fileImport(graphic, QString::fromStdString(path), RS2::FormatDWG);
  std::cout << "\n=== condo dwg GUI import ===\n";
  std::cout << "imported: " << imported << "\n";
  int nTop = 0, nBlocks = 0, nBlockEnts = 0;
  for (auto *e : graphic) if (e) ++nTop;
  nBlocks = graphic.countBlocks();
  std::cout << "top-level: " << nTop << "\n";
  std::cout << "blocks: " << nBlocks << "\n";
  for (unsigned i = 0; i < nBlocks; ++i) {
    auto* b = graphic.blockAt(i);
    if (b) {
      int cnt = 0;
      for (auto *e : *b) if (e) ++cnt;
      std::cout << "  Block '" << b->getName().toStdString() << "': " << cnt << " entities\n";
      nBlockEnts += cnt;
    }
  }
  std::cout << "entities in blocks: " << nBlockEnts << "\n";

  std::cout << "\nlayers:\n";
  for (unsigned i = 0; i < graphic.countLayers(); ++i) {
    auto* l = graphic.layerAt(i);
    if (l) {
      std::cout << "  '" << l->getName().toStdString() << "' "
                << (l->isFrozen() ? "FROZEN" : "visible") << "\n";
    }
  }

  std::cout << "\ntop-level entity rtti types:\n";
  std::map<int, int> topTypes;
  for (auto *e : graphic) {
    if (!e) continue;
    topTypes[e->rtti()]++;
  }
  for (const auto& [t, c] : topTypes) std::cout << "  rtti=" << t << ": " << c << "\n";
  REQUIRE(imported);
}
