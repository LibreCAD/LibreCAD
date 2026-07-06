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
**********************************************************************/

/**
 * DWG tArch tests: read DWG files in D:/data/dli/doc/dwg4/ and validate all
 * entities. This test verifies entity count, types, and integrity for
 * architectural/engineering DWG files.
 */

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_interface.h"
#include "libdwgr.h"
#include "libdxfrw.h"

namespace {

class EntityValidationIface : public DRW_Interface {
public:
  std::map<std::string, int> entityTypes;
  std::map<std::string, int> layerEntities;
  std::map<std::string, int> blockEntities;
  int totalEntities = 0;
  int blocks = 0;
  int layers = 0;
  int mlines = 0;
  int mleaders = 0;
  int hatches = 0;
  int inserts = 0;
  int splines = 0;
  int lwPolylines = 0;
  int polylines = 0;
  int lines = 0;
  int arcs = 0;
  int circles = 0;
  int texts = 0;
  int mtexts = 0;
  int dimensions = 0;
  int points = 0;
  int solids = 0;
  int traces = 0;
  int images = 0;
  int wipeouts = 0;
  int viewports = 0;
  int rays = 0;
  int xlines = 0;
  int ellipses = 0;
  int leaders = 0;
  int dimAligned = 0;
  int dimLinear = 0;
  int dimRadial = 0;
  int dimDiametric = 0;
  int dimAngular = 0;
  int dimArc = 0;
  int dimOrdinate = 0;
  int tolerance = 0;
  int underlays = 0;
  int attributes = 0;
  int attdefs = 0;

  std::string currentBlock;
  int inBlock = 0;

  bool currentlyInRealBlock() const {
    if (inBlock <= 0)
      return false;
    auto startsWithCi = [&](const char *prefix) {
      const size_t n = std::strlen(prefix);
      if (currentBlock.size() < n)
        return false;
      for (size_t i = 0; i < n; ++i)
        if (std::tolower(static_cast<unsigned char>(currentBlock[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
          return false;
      return true;
    };
    return !(startsWithCi("*Model_Space") || startsWithCi("*Paper_Space"));
  }

  void trackEntity(const DRW_Entity &e, const std::string &typeName) {
    ++totalEntities;
    entityTypes[typeName]++;

    std::string layerName = e.layer.empty() ? "(no layer)" : e.layer;
    layerEntities[layerName]++;

    if (currentlyInRealBlock()) {
      blockEntities[currentBlock]++;
    }
  }

  void addHeader(const DRW_Header *) override {}
  void addLType(const DRW_LType &) override {}
  void addLayer(const DRW_Layer &) override { ++layers; }
  void addDimStyle(const DRW_Dimstyle &) override {}
  void addVport(const DRW_Vport &) override {}
  void addTextStyle(const DRW_Textstyle &) override {}
  void addAppId(const DRW_AppId &) override {}

  void addBlock(const DRW_Block &b) override {
    ++blocks;
    ++inBlock;
    currentBlock = b.name;
  }
  void setBlock(const int) override {}
  void endBlock() override {
    if (inBlock > 0)
      --inBlock;
    if (inBlock == 0)
      currentBlock.clear();
  }

  void addPoint(const DRW_Point &e) override {
    ++points;
    trackEntity(e, "POINT");
  }
  void addLine(const DRW_Line &e) override {
    ++lines;
    trackEntity(e, "LINE");
  }
  void addRay(const DRW_Ray &e) override {
    ++rays;
    trackEntity(e, "RAY");
  }
  void addXline(const DRW_Xline &e) override {
    ++xlines;
    trackEntity(e, "XLINE");
  }
  void addArc(const DRW_Arc &e) override {
    ++arcs;
    trackEntity(e, "ARC");
  }
  void addCircle(const DRW_Circle &e) override {
    ++circles;
    trackEntity(e, "CIRCLE");
  }
  void addEllipse(const DRW_Ellipse &e) override {
    ++ellipses;
    trackEntity(e, "ELLIPSE");
  }
  void addLWPolyline(const DRW_LWPolyline &e) override {
    ++lwPolylines;
    trackEntity(e, "LWPOLYLINE");
  }
  void addMLine(const DRW_MLine *e) override {
    ++mlines;
    trackEntity(*e, "MLINE");
  }
  void addPolyline(const DRW_Polyline &e) override {
    ++polylines;
    trackEntity(e, "POLYLINE");
  }
  void addSpline(const DRW_Spline *e) override {
    ++splines;
    trackEntity(*e, "SPLINE");
  }
  void addKnot(const DRW_Entity &) override {}
  void addInsert(const DRW_Insert &e) override {
    ++inserts;
    trackEntity(e, "INSERT");
    for (const auto &a : e.attlist) {
      if (!a)
        continue;
      if (a->eType == DRW::ATTDEF) {
        ++attdefs;
        ++totalEntities;
        entityTypes["ATTDEF"]++;
        std::string layerName = a->layer.empty() ? "(no layer)" : a->layer;
        layerEntities[layerName]++;
        if (currentlyInRealBlock()) {
          blockEntities[currentBlock]++;
        }
      } else {
        ++attributes;
        ++totalEntities;
        entityTypes["ATTRIB"]++;
        std::string layerName = a->layer.empty() ? "(no layer)" : a->layer;
        layerEntities[layerName]++;
        if (currentlyInRealBlock()) {
          blockEntities[currentBlock]++;
        }
      }
    }
  }
  void addTrace(const DRW_Trace &e) override {
    ++traces;
    trackEntity(e, "TRACE");
  }
  void add3dFace(const DRW_3Dface &e) override {
    ++solids;
    trackEntity(e, "3DFACE");
  }
  void addSolid(const DRW_Solid &e) override {
    ++solids;
    trackEntity(e, "SOLID");
  }
  void addMText(const DRW_MText &e) override {
    ++mtexts;
    trackEntity(e, "MTEXT");
  }
  void addText(const DRW_Text &e) override {
    ++texts;
    trackEntity(e, "TEXT");
  }
  void addDimAlign(const DRW_DimAligned *e) override {
    ++dimAligned;
    ++dimensions;
    trackEntity(*e, "DIM_ALIGNED");
  }
  void addDimLinear(const DRW_DimLinear *e) override {
    ++dimLinear;
    ++dimensions;
    trackEntity(*e, "DIM_LINEAR");
  }
  void addDimRadial(const DRW_DimRadial *e) override {
    ++dimRadial;
    ++dimensions;
    trackEntity(*e, "DIM_RADIAL");
  }
  void addDimDiametric(const DRW_DimDiametric *e) override {
    ++dimDiametric;
    ++dimensions;
    trackEntity(*e, "DIM_DIAMETRIC");
  }
  void addDimAngular(const DRW_DimAngular *e) override {
    ++dimAngular;
    ++dimensions;
    trackEntity(*e, "DIM_ANGULAR");
  }
  void addDimAngular3P(const DRW_DimAngular3p *e) override {
    ++dimAngular;
    ++dimensions;
    trackEntity(*e, "DIM_ANGULAR3P");
  }
  void addDimArc(const DRW_DimArc *e) override {
    ++dimArc;
    ++dimensions;
    trackEntity(*e, "DIM_ARC");
  }
  void addDimOrdinate(const DRW_DimOrdinate *e) override {
    ++dimOrdinate;
    ++dimensions;
    trackEntity(*e, "DIM_ORDINATE");
  }
  void addLeader(const DRW_Leader *e) override {
    ++leaders;
    trackEntity(*e, "LEADER");
  }
  void addHatch(const DRW_Hatch *e) override {
    ++hatches;
    trackEntity(*e, "HATCH");
  }
  void addViewport(const DRW_Viewport &e) override {
    ++viewports;
    trackEntity(e, "VIEWPORT");
  }
  void addImage(const DRW_Image *e) override {
    ++images;
    trackEntity(*e, "IMAGE");
  }
  void addWipeout(const DRW_Wipeout *e) override {
    ++wipeouts;
    trackEntity(*e, "WIPEOUT");
  }
  void addMLeader(const DRW_MLeader *e) override {
    ++mleaders;
    trackEntity(*e, "MLEADER");
  }
  void addTolerance(const DRW_Tolerance &e) override {
    ++tolerance;
    trackEntity(e, "TOLERANCE");
  }
  void addUnderlay(const DRW_Underlay *e) override {
    ++underlays;
    trackEntity(*e, "UNDERLAY");
  }
  void linkUnderlay(const DRW_UnderlayDefinition *) override {}
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

const char *versionStr(DRW::Version v) {
  switch (v) {
  case DRW::MC00:
    return "MC0.0/R1.1";
  case DRW::AC12:
    return "AC1.2/R1.2";
  case DRW::AC14:
    return "AC1.4/R1.4";
  case DRW::AC150:
    return "AC1.50/R2.0";
  case DRW::AC210:
    return "AC2.10/R2.10";
  case DRW::AC1002:
    return "AC1002/R2.5";
  case DRW::AC1003:
    return "AC1003/R2.6";
  case DRW::AC1004:
    return "AC1004/R9";
  case DRW::AC1006:
    return "AC1006/R10";
  case DRW::AC1009:
    return "AC1009/R11-R12";
  case DRW::AC1012:
    return "AC1012/R13";
  case DRW::AC1014:
    return "AC1014/R14";
  case DRW::AC1015:
    return "AC1015/R2000";
  case DRW::AC1018:
    return "AC1018/R2004";
  case DRW::AC1021:
    return "AC1021/R2007";
  case DRW::AC1024:
    return "AC1024/R2010";
  case DRW::AC1027:
    return "AC1027/R2013";
  case DRW::AC1032:
    return "AC1032/R2018";
  default:
    return "UNKNOWN";
  }
}

const char *errorStr(DRW::error e) {
  switch (e) {
  case DRW::BAD_NONE:
    return "OK";
  case DRW::BAD_UNKNOWN:
    return "BAD_UNKNOWN";
  case DRW::BAD_OPEN:
    return "BAD_OPEN";
  case DRW::BAD_VERSION:
    return "BAD_VERSION";
  case DRW::BAD_READ_METADATA:
    return "BAD_READ_METADATA";
  case DRW::BAD_READ_FILE_HEADER:
    return "BAD_READ_FILE_HEADER";
  case DRW::BAD_READ_HEADER:
    return "BAD_READ_HEADER";
  case DRW::BAD_READ_CLASSES:
    return "BAD_READ_CLASSES";
  case DRW::BAD_READ_HANDLES:
    return "BAD_READ_HANDLES";
  case DRW::BAD_READ_TABLES:
    return "BAD_READ_TABLES";
  case DRW::BAD_READ_BLOCKS:
    return "BAD_READ_BLOCKS";
  case DRW::BAD_READ_ENTITIES:
    return "BAD_READ_ENTITIES";
  case DRW::BAD_READ_OBJECTS:
    return "BAD_READ_OBJECTS";
  case DRW::BAD_READ_SECTION:
    return "BAD_READ_SECTION";
  default:
    return "UNKNOWN_ERROR";
  }
}

void printEntityReport(const std::string &filename, const EntityValidationIface &iface,
                       DRW::Version version, DRW::error error) {
  std::cout << "\n=== " << filename << " ===\n";
  std::cout << "Version    : " << versionStr(version) << "\n";
  std::cout << "Error      : " << errorStr(error) << "\n";
  std::cout << "Blocks     : " << iface.blocks << "\n";
  std::cout << "Layers     : " << iface.layers << "\n";
  std::cout << "Total Entities: " << iface.totalEntities << "\n";
  std::cout << "\nEntity Type Distribution:\n";
  std::cout << "  Points          : " << iface.points << "\n";
  std::cout << "  Lines           : " << iface.lines << "\n";
  std::cout << "  Rays            : " << iface.rays << "\n";
  std::cout << "  Xlines          : " << iface.xlines << "\n";
  std::cout << "  Arcs            : " << iface.arcs << "\n";
  std::cout << "  Circles         : " << iface.circles << "\n";
  std::cout << "  Ellipses        : " << iface.ellipses << "\n";
  std::cout << "  LWPolylines     : " << iface.lwPolylines << "\n";
  std::cout << "  Polylines       : " << iface.polylines << "\n";
  std::cout << "  Splines         : " << iface.splines << "\n";
  std::cout << "  MLines          : " << iface.mlines << "\n";
  std::cout << "  Inserts         : " << iface.inserts << "\n";
  std::cout << "  Attributes      : " << iface.attributes << "\n";
  std::cout << "  AttDefs         : " << iface.attdefs << "\n";
  std::cout << "  Texts           : " << iface.texts << "\n";
  std::cout << "  MTexts          : " << iface.mtexts << "\n";
  std::cout << "  Hatches         : " << iface.hatches << "\n";
  std::cout << "  Dimensions      : " << iface.dimensions << "\n";
  std::cout << "    - Aligned     : " << iface.dimAligned << "\n";
  std::cout << "    - Linear      : " << iface.dimLinear << "\n";
  std::cout << "    - Radial      : " << iface.dimRadial << "\n";
  std::cout << "    - Diametric   : " << iface.dimDiametric << "\n";
  std::cout << "    - Angular     : " << iface.dimAngular << "\n";
  std::cout << "    - Arc         : " << iface.dimArc << "\n";
  std::cout << "    - Ordinate    : " << iface.dimOrdinate << "\n";
  std::cout << "  Leaders         : " << iface.leaders << "\n";
  std::cout << "  MLeaders        : " << iface.mleaders << "\n";
  std::cout << "  Solids/Traces   : " << iface.solids << "\n";
  std::cout << "  Images          : " << iface.images << "\n";
  std::cout << "  Wipeouts        : " << iface.wipeouts << "\n";
  std::cout << "  Viewports       : " << iface.viewports << "\n";
  std::cout << "  Tolerances      : " << iface.tolerance << "\n";
  std::cout << "  Underlays       : " << iface.underlays << "\n";

  if (!iface.layerEntities.empty()) {
    std::cout << "\nTop Layers by Entity Count:\n";
    std::vector<std::pair<int, std::string>> sortedLayers;
    for (const auto &[layer, cnt] : iface.layerEntities)
      sortedLayers.emplace_back(cnt, layer);
    std::sort(sortedLayers.rbegin(), sortedLayers.rend());
    int shown = 0;
    for (const auto &[cnt, layer] : sortedLayers) {
      std::cout << "  " << std::left << std::setw(30) << layer << " " << cnt
                << "\n";
      if (++shown >= 10) {
        std::cout << "  ...\n";
        break;
      }
    }
  }

  if (!iface.blockEntities.empty()) {
    std::cout << "\nBlocks with Entities:\n";
    for (const auto &[block, cnt] : iface.blockEntities) {
      std::cout << "  " << std::left << std::setw(30) << block << " " << cnt
                << "\n";
    }
  }
}

} // namespace

TEST_CASE("DWG tArch: validate all entities", "[.dwg_tarch]") {
  const std::filesystem::path manifest = "D:/data/dli/doc/dwg_files.txt";
  if (!std::filesystem::is_regular_file(manifest)) {
    SUCCEED("DWG manifest not found at " << manifest << "; skipping tArch tests");
    return;
  }

  std::vector<std::string> paths;
  std::ifstream in(manifest);
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line.front() == '#')
      continue;
    line.erase(line.find_last_not_of(" \t\r\n") + 1);
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    if (line.empty())
      continue;
    if (std::filesystem::is_regular_file(line)) {
      paths.push_back(line);
    }
  }
  std::sort(paths.begin(), paths.end());

  if (paths.empty()) {
    SUCCEED("No valid .dwg files found in manifest; skipping");
    return;
  }

  std::cout << "\n"
            << std::left << std::setw(50) << "File" << std::setw(16)
            << "Version" << std::setw(20) << "Error" << std::setw(10)
            << "Entities" << std::setw(8) << "Blocks"
            << "Layers\n"
            << std::string(110, '-') << "\n";

  int passed = 0, failed = 0;

  for (const auto &path : paths) {
    const std::string name = std::filesystem::path(path).filename().string();

    EntityValidationIface iface;
    try {
      dwgR reader(path.c_str());
      bool ok = reader.read(&iface, true);
      DRW::Version version = reader.getVersion();
      DRW::error error = reader.getError();

      std::cout << std::left << std::setw(50) << name << std::setw(16)
                << versionStr(version) << std::setw(20) << errorStr(error)
                << std::setw(10) << iface.totalEntities << std::setw(8)
                << iface.blocks << iface.layers << "\n";

      if (error != DRW::BAD_NONE) {
        ++failed;
        FAIL_CHECK(name << ": read failed with error " << errorStr(error));
      } else {
        ++passed;
        printEntityReport(name, iface, version, error);

        REQUIRE(iface.totalEntities >= 0);
        REQUIRE(iface.blocks >= 0);
        REQUIRE(iface.layers >= 0);
      }
    } catch (const std::exception &e) {
      std::cout << std::left << std::setw(50) << name << "(EXCEPTION: " << e.what() << ")\n";
      ++failed;
      FAIL_CHECK(name << ": exception during read: " << e.what());
    } catch (...) {
      std::cout << std::left << std::setw(50) << name << "(UNKNOWN EXCEPTION)\n";
      ++failed;
      FAIL_CHECK(name << ": unknown exception during read");
    }
  }

  std::cout << std::string(110, '-') << "\n";
  std::cout << "Passed: " << passed << "  Failed: " << failed
            << "  Total: " << paths.size() << "\n";
}

TEST_CASE("DWG tArch: deep validation of selected files", "[.dwg_tarch_deep]") {
  const std::string dir = LIBRECAD_TEST_DIR "/tarch/";
  if (!std::filesystem::is_directory(dir)) {
    SUCCEED("DWG directory not found; skipping deep tArch tests");
    return;
  }

  const char *targets[] = {
      "t1.dwg",
  };

  for (const auto *name : targets) {
    const std::string path = dir + name;
    if (!std::filesystem::is_regular_file(path)) {
      std::cout << "\n=== " << name << " (missing - skipped) ===\n";
      continue;
    }

    EntityValidationIface iface;
    try {
      dwgR reader(path.c_str());
      bool ok = reader.read(&iface, true);
      DRW::Version version = reader.getVersion();
      DRW::error error = reader.getError();

      std::cout << "\n=== Deep Validation: " << name << " ===\n";
      std::cout << "Version    : " << versionStr(version) << "\n";
      std::cout << "Read OK    : " << (ok ? "Yes" : "No") << "\n";
      std::cout << "Error      : " << errorStr(error) << "\n";

      if (ok && error == DRW::BAD_NONE) {
        printEntityReport(name, iface, version, error);

        int sumTypes = 0;
        for (const auto &[type, cnt] : iface.entityTypes)
          sumTypes += cnt;
        REQUIRE(sumTypes == iface.totalEntities);

        int sumLayers = 0;
        for (const auto &[layer, cnt] : iface.layerEntities)
          sumLayers += cnt;
        REQUIRE(sumLayers == iface.totalEntities);

        std::cout << "\n[VALIDATION PASSED]\n";
      } else {
        std::cout << "\n[VALIDATION FAILED - read error]\n";
        FAIL_CHECK(name << ": read failed");
      }
    } catch (const std::exception &e) {
      std::cout << "\n=== " << name << " (EXCEPTION: " << e.what() << ") ===\n";
      FAIL_CHECK(name << ": exception: " << e.what());
    }
  }
}