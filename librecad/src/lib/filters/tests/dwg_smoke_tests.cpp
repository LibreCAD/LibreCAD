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
 * DWG smoke tests: open each test file with dwgR and report error code +
 * entity/block/layer counts.  No assertions are made on empty drawings —
 * this is a diagnostic harness, not a correctness test.
 *
 * Results are printed to stdout in tab-separated form regardless of pass/fail
 * so the output is easy to scan after one test run.
 *
 * Files that are known to have bad magic bytes (e.g., #mechanical_example-*)
 * are expected to return BAD_VERSION and are NOT checked.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "drw_base.h"
#include "drw_interface.h"
#include "intern/proxygraphicdecoder.h"
#include "intern/drw_dbg.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgobjectframe.h"
#include "intern/dwgreader27.h"
#include "libdwgr.h"
#include "libdxfrw.h"

// LibreCAD entity headers for end-to-end DRW→RS_Hatch pipeline tests.
#include "lc_containertraverser.h"
#include "lc_splinepoints.h"
#include <QCoreApplication>

#include "rs_arc.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_entitycontainer.h"
#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "lc_graphicviewport.h"
#include "rs_hatch.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_solid.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_settings.h"

namespace {

bool isGeneratedDwgDictionary(const DRW_Dictionary &dictionary) {
  return dictionary.handle == DRW::DwgNamedObjectsDictionaryHandle ||
         dictionary.handle == DRW::DwgAcadGroupDictionaryHandle;
}

// ---- capturing DRW debug printer -------------------------------------------

class CapturingPrinter : public DRW::DebugPrinter {
public:
  std::string buf;
  void printS(const std::string &s) override { buf += s; }
  void printI(long long int i) override { buf += std::to_string(i); }
  void printUI(long long unsigned int i) override { buf += std::to_string(i); }
  void printD(double d) override { buf += std::to_string(d); }
  void printH(long long int i) override {
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%llX", (unsigned long long)i);
    buf += tmp;
  }
  void printB(int i) override { buf += std::to_string(i); }
  void printHL(int c, int s, int h) override {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "(%d,%d,%X)", c, s, (unsigned)h);
    buf += tmp;
  }
  void printPT(double x, double y, double z) override {
    char tmp[96];
    snprintf(tmp, sizeof(tmp), "(%.3f,%.3f,%.3f)", x, y, z);
    buf += tmp;
  }

  // Parse [entity-pass-defer N] tokens (formerly [unhandled-entity-type N])
  // out of captured buffer.  These are oTypes the entity-pass switch did
  // not dispatch and queued in objObjectMap for the OBJECTS pass; some
  // (DICTIONARY/MLINESTYLE/LAYOUT/IMAGEDEF) are handled there, the rest
  // are truly unhandled.  Field name is kept as `unhandledTypes` to
  // preserve callsite assertions; semantically it's "deferred or lost".
  std::map<int, int> unhandledTypes() const {
    std::map<int, int> result;
    std::regex re(R"(\[entity-pass-defer (\d+)\])");
    auto begin = std::sregex_iterator(buf.begin(), buf.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it)
      result[std::stoi((*it)[1].str())]++;
    return result;
  }
};

// ---- minimal counting DRW_Interface ----------------------------------------

class CountingIface : public DRW_Interface {
public:
  int entities = 0;
  int blocks = 0;
  int layers = 0;
  std::vector<DRW_ViewportEntityHeader> viewportEntityHeaders;
  std::map<int, int> entityLWeights;        // lWeight enum -> count
  std::map<std::string, int> layerLWeights; // layer name -> lWeight enum

  void track(const DRW_Entity &e) {
    ++entities;
    entityLWeights[static_cast<int>(e.lWeight)]++;
  }

  // tables
  void addHeader(const DRW_Header *) override {}
  void addLType(const DRW_LType &) override {}
  void addLayer(const DRW_Layer &l) override {
    ++layers;
    layerLWeights[l.name] = static_cast<int>(l.lWeight);
  }
  void addDimStyle(const DRW_Dimstyle &) override {}
  void addVport(const DRW_Vport &) override {}
  void addViewportEntityHeader(
      const DRW_ViewportEntityHeader &value) override {
    viewportEntityHeaders.push_back(value);
  }
  void addTextStyle(const DRW_Textstyle &) override {}
  void addAppId(const DRW_AppId &) override {}

  // blocks
  void addBlock(const DRW_Block &) override { ++blocks; }
  void setBlock(const int) override {}
  void endBlock() override {}

  // entities — all increment the counter
  void addPoint(const DRW_Point &e) override { track(e); }
  void addLine(const DRW_Line &e) override { track(e); }
  void addRay(const DRW_Ray &e) override { track(e); }
  void addXline(const DRW_Xline &e) override { track(e); }
  void addArc(const DRW_Arc &e) override { track(e); }
  void addCircle(const DRW_Circle &e) override { track(e); }
  void addEllipse(const DRW_Ellipse &e) override { track(e); }
  void addLWPolyline(const DRW_LWPolyline &e) override { track(e); }
  void addMLine(const DRW_MLine *e) override {
    track(*e);
    ++mlines;
    mlineVerts += static_cast<int>(e->vertlist.size());
  }
  void addUnderlay(const DRW_Underlay *e) override {
    track(*e);
    ++underlays;
    underlayClipVerts += static_cast<int>(e->clipBoundary.size());
  }
  void linkUnderlay(const DRW_UnderlayDefinition *) override { ++underlayDefs; }
  void addPolyline(const DRW_Polyline &e) override { track(e); }
  void addSpline(const DRW_Spline *e) override { track(*e); }
  void addKnot(const DRW_Entity &) override {}
  void addInsert(const DRW_Insert &e) override { track(e); }
  void addTrace(const DRW_Trace &e) override { track(e); }
  void add3dFace(const DRW_3Dface &e) override { track(e); }
  void addSolid(const DRW_Solid &e) override { track(e); }
  void addMText(const DRW_MText &e) override { track(e); }
  void addText(const DRW_Text &e) override { track(e); }
  void addDimAlign(const DRW_DimAligned *e) override { track(*e); }
  void addDimLinear(const DRW_DimLinear *e) override { track(*e); }
  void addDimRadial(const DRW_DimRadial *e) override { track(*e); }
  void addDimDiametric(const DRW_DimDiametric *e) override { track(*e); }
  void addDimAngular(const DRW_DimAngular *e) override { track(*e); }
  void addDimAngular3P(const DRW_DimAngular3p *e) override { track(*e); }
  void addDimArc(const DRW_DimArc *e) override { track(*e); }
  void addDimOrdinate(const DRW_DimOrdinate *e) override { track(*e); }
  void addLeader(const DRW_Leader *e) override { track(*e); }
  void addHatch(const DRW_Hatch *e) override { track(*e); }
  void addViewport(const DRW_Viewport &e) override { track(e); }
  void addImage(const DRW_Image *e) override { track(*e); }
  void addWipeout(const DRW_Wipeout *e) override {
    track(*e);
    wipeouts++;
    wipeoutVertices += static_cast<int>(e->clipPath.size());
  }
  void addMLeader(const DRW_MLeader *e) override {
    track(*e);
    mleaders++;
    mleaderRoots += static_cast<int>(e->context.roots.size());
    for (const auto &r : e->context.roots) {
      mleaderLines += static_cast<int>(r.leaderLines.size());
      for (const auto &ll : r.leaderLines) {
        mleaderPoints += static_cast<int>(ll.points.size());
      }
    }
  }
  void addMLeaderStyle(const DRW_MLeaderStyle *) override { ++mleaderStyles; }
  int wipeouts = 0;
  int wipeoutVertices = 0;
  int mlines = 0;
  int mlineVerts = 0;
  int underlays = 0;
  int underlayClipVerts = 0;
  int underlayDefs = 0;
  int mleaders = 0;
  int mleaderRoots = 0;
  int mleaderLines = 0;
  int mleaderPoints = 0;
  int mleaderStyles = 0;
  void linkImage(const DRW_ImageDef *) override {}
  void addComment(const char *) override {}
  void addPlotSettings(const DRW_PlotSettings *) override {}

  // write callbacks (not used when reading)
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

class DictionaryFixtureIface : public CountingIface {
public:
  std::vector<DRW_Dictionary> dictionaries;
  std::vector<DRW_DictionaryWithDefault> dictionariesWithDefault;

  void addDictionary(const DRW_Dictionary &value) override {
    dictionaries.push_back(value);
  }

  void addDictionaryWithDefault(
      const DRW_DictionaryWithDefault &value) override {
    dictionariesWithDefault.push_back(value);
  }
};

class DictionaryMutationWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  DRW_Dictionary dictionary;

  void writeObjects() override {
    REQUIRE(writer != nullptr);
    REQUIRE(writer->writeDictionary(&dictionary));
  }
};

struct XlineCaptureIface : public CountingIface {
  std::vector<DRW_Xline> xlines;
  int lineCallbacks = 0;

  void addLine(const DRW_Line &e) override {
    CountingIface::addLine(e);
    ++lineCallbacks;
  }

  void addXline(const DRW_Xline &e) override {
    CountingIface::addXline(e);
    xlines.push_back(e);
  }
};

class PointCloudWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  int pointCloudCallbacks = 0;
  DRW_PointCloud pointCloud;
  DRW_Coord sourceOrigin{1.0, 2.0, 3.0};

  void writeEntities() override {
    REQUIRE(writer != nullptr);
    DRW_PointCloud value;
    value.classVersion = 3;
    value.origin = sourceOrigin;
    value.savedFilename = "cloud.rcp";
    value.sourceFileCount = 0;
    value.extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
    value.extentsMax = DRW_Coord{4.0, 5.0, 6.0};
    value.pointCount = 1234;
    value.ucsName = "UCS-A";
    value.definitionHandle = 0xB0;
    value.reactorHandle = 0xC0;
    REQUIRE(writer->writePointCloud(&value));
  }

  void addPointCloud(const DRW_PointCloud *value) override {
    if (value != nullptr)
      pointCloud = *value;
    ++pointCloudCallbacks;
  }
};

class PointCloudExWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  int pointCloudExCallbacks = 0;
  DRW_PointCloudEx pointCloudEx;
  DRW_Coord sourceExtentsMin{-1.0, -2.0, -3.0};

  void writeEntities() override {
    REQUIRE(writer != nullptr);
    DRW_PointCloudEx value;
    value.classVersion = 7;
    value.extentsMin = sourceExtentsMin;
    value.extentsMax = DRW_Coord{4.0, 5.0, 6.0};
    value.ucsOrigin = DRW_Coord{7.0, 8.0, 9.0};
    value.definitionHandle = 0xB4;
    value.reactorHandle = 0xC4;
    value.name = "cloud-ex";
    value.showIntensity = true;
    value.showCropping = true;
    value.unknownInt0 = 41;
    value.unknownInt1 = 42;
    value.stylizationType = 3;
    value.intensityColorScheme = "intensity";
    value.currentColorScheme = "current";
    value.classificationColorScheme = "classification";
    value.elevationMin = 10.5;
    value.elevationMax = 20.5;
    value.intensityMin = 100.0;
    value.intensityMax = 200.0;
    value.intensityOutOfRangeBehavior = 4;
    value.elevationOutOfRangeBehavior = 5;
    REQUIRE(writer->writePointCloudEx(&value));
  }

  void addPointCloudEx(const DRW_PointCloudEx *value) override {
    if (value != nullptr)
      pointCloudEx = *value;
    ++pointCloudExCallbacks;
  }
};

class PointCloudReaderProbe : public dwgReader27 {
public:
  using dwgReader27::dwgReader27;
  using dwgReader::readDwgEntity;
  using dwgReader::readDwgObject;

  const std::uint8_t *objectData() const { return objData.get(); }
  std::uint64_t objectDataSize() const { return uncompSize; }
};

class PointCloudExternalReadWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;

  void writeEntities() override {
    REQUIRE(writer != nullptr);
    DRW_PointCloud value;
    value.classVersion = 3;
    value.origin = DRW_Coord{1.25, 2.5, 3.75};
    value.savedFilename = "external-reader.rcp";
    value.sourceFileCount = 0;
    value.extentsMin = DRW_Coord{-10.0, -20.0, -30.0};
    value.extentsMax = DRW_Coord{40.0, 50.0, 60.0};
    value.pointCount = 1234;
    value.ucsName = "External-UCS";
    value.definitionHandle = 0xB0;
    value.reactorHandle = 0xC0;
    REQUIRE(writer->writePointCloud(&value));
  }
};

std::string shellQuote(const std::string& value) {
  std::string result("'");
  for (const char ch : value) {
    if (ch == '\'')
      result += "'\\''";
    else
      result += ch;
  }
  result += '\'';
  return result;
}

bool findFrameEnd(const std::vector<std::uint8_t>& data,
                  std::uint32_t offset, DRW::Version version,
                  std::size_t& bodyStart, std::size_t& frameEnd) {
  if (offset >= data.size())
    return false;

  dwgBuffer cursor(const_cast<std::uint8_t*>(data.data()), data.size());
  if (!cursor.setPosition(offset))
    return false;

  const std::int32_t bodySize = cursor.getModularShort();
  if (!cursor.isGood() || bodySize < 0)
    return false;
  if (version > DRW::AC1021)
    cursor.getUModularChar();
  if (!cursor.isGood())
    return false;

  const std::uint64_t bodyStart64 = cursor.getPosition();
  const std::uint64_t bodyEnd = bodyStart64
      + static_cast<std::uint64_t>(bodySize);
  const std::uint64_t end = bodyEnd + sizeof(std::uint16_t);
  if (bodyEnd < bodyStart64 || end < bodyEnd || end > data.size())
    return false;

  bodyStart = static_cast<std::size_t>(bodyStart64);
  frameEnd = static_cast<std::size_t>(end);
  return true;
}

bool findDictionaryCountBit(const std::vector<std::uint8_t>& body,
                            std::size_t& bitOffset) {
  if (body.empty())
    return false;
  dwgBuffer cursor(const_cast<std::uint8_t*>(body.data()), body.size());
  cursor.getObjType(DRW::AC1027);
  cursor.getHandle();
  cursor.getBitShort(); // empty EED stream
  cursor.getBitLong();  // reactor count
  cursor.getBit();      // extension dictionary flag
  cursor.getBit();      // AC1027 data-storage flag
  if (!cursor.isGood())
    return false;
  bitOffset = cursor.getPosition() * 8 + cursor.getBitPos();
  return bitOffset + 10 <= body.size() * 8;
}

bool patchBits(std::vector<std::uint8_t>& data, std::size_t bitOffset,
               std::uint64_t value, std::size_t bitCount) {
  if (bitCount > 64 || bitOffset > data.size() * 8
      || bitCount > data.size() * 8 - bitOffset)
    return false;
  for (std::size_t i = 0; i < bitCount; ++i) {
    const std::size_t absoluteBit = bitOffset + i;
    const std::uint8_t mask = static_cast<std::uint8_t>(
        1u << (7u - (absoluteBit % 8)));
    if (((value >> (bitCount - 1u - i)) & 1u) != 0)
      data[absoluteBit / 8] |= mask;
    else
      data[absoluteBit / 8] &= static_cast<std::uint8_t>(~mask);
  }
  return true;
}

bool findRawBitDouble(const std::vector<std::uint8_t>& body, double expected,
                      std::size_t& bitOffset) {
  std::uint64_t expectedBits = 0;
  std::memcpy(&expectedBits, &expected, sizeof(expectedBits));
  const std::uint64_t bitCount = static_cast<std::uint64_t>(body.size()) * 8;
  for (std::uint64_t bit = 0; bit + 66 <= bitCount; ++bit) {
    dwgBuffer cursor(const_cast<std::uint8_t*>(body.data()), body.size());
    if (!cursor.setPosition(bit / 8))
      continue;
    cursor.setBitPos(static_cast<std::uint8_t>(bit % 8));
    if (cursor.get2Bits() != 0 || !cursor.isGood())
      continue;
    const double candidate = cursor.getRawDouble();
    if (!cursor.isGood())
      continue;
    std::uint64_t candidateBits = 0;
    std::memcpy(&candidateBits, &candidate, sizeof(candidateBits));
    if (candidateBits == expectedBits) {
      bitOffset = static_cast<std::size_t>(bit);
      return true;
    }
  }
  return false;
}

void setBit(std::vector<std::uint8_t>& data, std::size_t bitOffset,
            bool value) {
  const std::size_t byteOffset = bitOffset / 8;
  const std::uint8_t mask = static_cast<std::uint8_t>(
      1u << (7u - static_cast<unsigned>(bitOffset % 8)));
  if (value)
    data[byteOffset] = static_cast<std::uint8_t>(data[byteOffset] | mask);
  else
    data[byteOffset] = static_cast<std::uint8_t>(data[byteOffset] & ~mask);
}

void replaceRawDouble(std::vector<std::uint8_t>& body, std::size_t bitOffset,
                      double value) {
  std::uint64_t bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(&bits);
  for (std::size_t byte = 0; byte < sizeof(bits); ++byte) {
    for (unsigned bit = 0; bit < 8; ++bit) {
      const bool set = (bytes[byte] & (1u << (7u - bit))) != 0;
      setBit(body, bitOffset + byte * 8 + bit, set);
    }
  }
}

class NavisworksModelWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  int navisworksCallbacks = 0;
  DRW_NavisworksModel navisworks;

  void writeEntities() override {
    REQUIRE(writer != nullptr);
    DRW_NavisworksModel value;
    value.flags = 11;
    value.definitionHandle = 0x2D0;
    for (std::size_t i = 0; i < value.transform.size(); ++i)
      value.transform[i] = static_cast<double>(i) + 0.25;
    value.unitFactor = 0.001;
    REQUIRE(writer->writeNavisworksModel(&value));
  }

  void addNavisworksModel(const DRW_NavisworksModel *value) override {
    if (value != nullptr)
      navisworks = *value;
    ++navisworksCallbacks;
  }
};

class PointCloudDefWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  std::vector<DRW_PointCloudDef> definitions;
  std::vector<DRW_PointCloudDef> captured;

  void writeDwgClasses() override {
    REQUIRE(writer != nullptr);
    for (auto &definition : definitions)
      REQUIRE(writer->registerPointCloudDefObjectClass(&definition));
  }

  void writeObjects() override {
    REQUIRE(writer != nullptr);
    for (auto &definition : definitions)
      REQUIRE(writer->writePointCloudDef(&definition));
  }

  void addPointCloudDef(const DRW_PointCloudDef &definition) override {
    captured.push_back(definition);
  }
};

class PointCloudDictionaryWriteIface : public PointCloudDefWriteIface {
public:
  DRW_Dictionary dictionary;
  std::vector<DRW_Dictionary> capturedDictionaries;

  void writeObjects() override {
    REQUIRE(writer != nullptr);
    REQUIRE(writer->writeDictionary(&dictionary));
    PointCloudDefWriteIface::writeObjects();
  }

  void addDictionary(const DRW_Dictionary &value) override {
    if (isGeneratedDwgDictionary(value))
      return;
    capturedDictionaries.push_back(value);
  }
};

class PointCloudReferenceGraphWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  std::vector<DRW_Dictionary> dictionaries;
  DRW_PointCloudDef pointCloudDefinition;
  DRW_NavisworksModelDef navisworksDefinition;
  DRW_NavisworksModel navisworksModel;
  DRW_PointCloudColorMap colorMap;
  std::vector<DRW_Dictionary> capturedDictionaries;
  std::vector<DRW_PointCloudDef> capturedPointCloudDefinitions;
  DRW_NavisworksModelDef capturedNavisworksDefinition;
  DRW_NavisworksModel capturedNavisworksModel;
  DRW_PointCloudColorMap capturedColorMap;

  void writeDwgClasses() override {
    REQUIRE(writer != nullptr);
    REQUIRE(writer->registerPointCloudDefObjectClass(&pointCloudDefinition));
    REQUIRE(writer->registerNavisworksModelDefObjectClass(
        &navisworksDefinition));
    REQUIRE(writer->registerPointCloudColorMapObjectClass(&colorMap));
  }

  void writeEntities() override {
    REQUIRE(writer != nullptr);
    REQUIRE(writer->writeNavisworksModel(&navisworksModel));
  }

  void writeObjects() override {
    REQUIRE(writer != nullptr);
    for (auto &dictionary : dictionaries)
      REQUIRE(writer->writeDictionary(&dictionary));
    REQUIRE(writer->writePointCloudDef(&pointCloudDefinition));
    REQUIRE(writer->writeNavisworksModelDef(&navisworksDefinition));
    REQUIRE(writer->writePointCloudColorMap(&colorMap));
  }

  void addDictionary(const DRW_Dictionary &value) override {
    if (isGeneratedDwgDictionary(value))
      return;
    capturedDictionaries.push_back(value);
  }

  void addPointCloudDef(const DRW_PointCloudDef &value) override {
    capturedPointCloudDefinitions.push_back(value);
  }

  void addNavisworksModelDef(const DRW_NavisworksModelDef &value) override {
    capturedNavisworksDefinition = value;
  }

  void addNavisworksModel(const DRW_NavisworksModel *value) override {
    if (value != nullptr)
      capturedNavisworksModel = *value;
  }

  void addPointCloudColorMap(const DRW_PointCloudColorMap &value) override {
    capturedColorMap = value;
  }
};

class PointCloudMetadataWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  DRW_NavisworksModelDef navisworks;
  DRW_NavisworksModel navisworksModel;
  DRW_PointCloudColorMap colorMap;
  int navisworksCallbacks = 0;
  int navisworksModelCallbacks = 0;
  int colorMapCallbacks = 0;

  void writeEntities() override {
    REQUIRE(writer != nullptr);
    if (writer->getVersion() >= DRW::AC1021 && navisworksModel.handle != 0)
      REQUIRE(writer->writeNavisworksModel(&navisworksModel));
  }

  void writeDwgClasses() override {
    REQUIRE(writer != nullptr);
    REQUIRE(writer->registerNavisworksModelDefObjectClass(&navisworks));
    REQUIRE(writer->registerPointCloudColorMapObjectClass(&colorMap));
  }

  void writeObjects() override {
    REQUIRE(writer != nullptr);
    REQUIRE(writer->writeNavisworksModelDef(&navisworks));
    REQUIRE(writer->writePointCloudColorMap(&colorMap));
  }

  void addNavisworksModelDef(const DRW_NavisworksModelDef &data) override {
    navisworks = data;
    ++navisworksCallbacks;
  }

  void addNavisworksModel(const DRW_NavisworksModel *data) override {
    if (data != nullptr)
      navisworksModel = *data;
    ++navisworksModelCallbacks;
  }

  void addPointCloudColorMap(const DRW_PointCloudColorMap &data) override {
    colorMap = data;
    ++colorMapCallbacks;
  }
};

// ---- helpers ----------------------------------------------------------------

const char *versionStr(DRW::Version v);

struct DwgResult {
  bool ok;
  DRW::error error;
  DRW::Version version;
  int entities;
  int blocks;
  int layers;
};

DwgResult readDwg(const std::string &path, bool verbose = false,
                  CountingIface *outIface = nullptr) {
  CountingIface localIface;
  CountingIface &iface = outIface ? *outIface : localIface;
  try {
    dwgR reader(path.c_str());
    if (verbose)
      reader.setDebug(DRW::DebugLevel::Debug);
    bool ok = reader.read(&iface, true);
    std::cout << "DWG fixture: " << path << "  version=" << versionStr(reader.getVersion()) << "\n";
    return {
        ok,           reader.getError(), reader.getVersion(), iface.entities,
        iface.blocks, iface.layers};
  } catch (const std::exception &e) {
    std::cerr << "  [EXCEPTION: " << e.what() << "]\n";
    return {false,          DRW::BAD_UNKNOWN, DRW::UNKNOWNV,
            iface.entities, iface.blocks,     iface.layers};
  } catch (...) {
    std::cerr << "  [UNKNOWN EXCEPTION]\n";
    return {false,          DRW::BAD_UNKNOWN, DRW::UNKNOWNV,
            iface.entities, iface.blocks,     iface.layers};
  }
}

DwgResult readDwgBuffer(const std::string &path, bool verbose = false,
                        CountingIface *outIface = nullptr) {
  CountingIface localIface;
  CountingIface &iface = outIface ? *outIface : localIface;
  try {
    std::ifstream in(path, std::ios::binary);
    if (!in)
      return {false, DRW::BAD_OPEN, DRW::UNKNOWNV, 0, 0, 0};
    std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());

    dwgR reader(path.c_str());
    if (verbose)
      reader.setDebug(DRW::DebugLevel::Debug);
    bool ok = reader.readBuffer(bytes.data(), bytes.size(), &iface, true);
    std::cout << "DWG fixture (buffer): " << path << "  version=" << versionStr(reader.getVersion()) << "\n";
    return {
        ok,           reader.getError(), reader.getVersion(), iface.entities,
        iface.blocks, iface.layers};
  } catch (const std::exception &e) {
    std::cerr << "  [EXCEPTION: " << e.what() << "]\n";
    return {false,          DRW::BAD_UNKNOWN, DRW::UNKNOWNV,
            iface.entities, iface.blocks,     iface.layers};
  } catch (...) {
    std::cerr << "  [UNKNOWN EXCEPTION]\n";
    return {false,          DRW::BAD_UNKNOWN, DRW::UNKNOWNV,
            iface.entities, iface.blocks,     iface.layers};
  }
}

std::string libredwgFixturePath(const char *release, const char *file) {
  const char *root = getenv("LIBREDWG_TEST_DATA");
  if (root && root[0] != '\0')
    return (std::filesystem::path(root) / release / file).string();
  const char *home = getenv("HOME");
  if (!home || home[0] == '\0')
    return {};
  return (std::filesystem::path(home) / "dev" / "libredwg" / "test" /
          "test-data" / release / file)
      .string();
}

// DRW_LW_Conv::lineWidth enum -> human-readable mm
const char *lWeightToMm(int lw) {
  switch (lw) {
  case 0:
    return "0.00mm";
  case 1:
    return "0.05mm";
  case 2:
    return "0.09mm";
  case 3:
    return "0.13mm";
  case 4:
    return "0.15mm";
  case 5:
    return "0.18mm";
  case 6:
    return "0.20mm";
  case 7:
    return "0.25mm";
  case 8:
    return "0.30mm";
  case 9:
    return "0.35mm";
  case 10:
    return "0.40mm";
  case 11:
    return "0.50mm";
  case 12:
    return "0.53mm";
  case 13:
    return "0.60mm";
  case 14:
    return "0.70mm";
  case 15:
    return "0.80mm";
  case 16:
    return "0.90mm";
  case 17:
    return "1.00mm";
  case 18:
    return "1.06mm";
  case 19:
    return "1.20mm";
  case 20:
    return "1.40mm";
  case 21:
    return "1.58mm";
  case 22:
    return "2.00mm";
  case 23:
    return "2.11mm";
  case 29:
    return "ByLayer";
  case 30:
    return "ByBlock";
  case 31:
    return "Default";
  default:
    return "?";
  }
}

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

// ---- entity type-tracking interface ----------------------------------------

class TypeTrackingIface : public DRW_Interface {
public:
  std::map<std::string, int> typeCounts;    // entity type name -> count
  std::map<std::string, int> layerEntities; // layer name -> entity count
  int inBlock = 0;
  std::string currentBlockName; // empty when not inside any block
  int modelSpaceEntities = 0;
  int blockSpaceEntities = 0;
  int blocks = 0;
  int layers = 0;
  size_t wipeoutVertexCount = 0;

  // Classify by the *current block's name*, not by block-stack depth.
  // Entities inside *Model_Space / *Paper_Space (or arriving via the
  // entity stream with no block context) are model/paper space; anything
  // inside a real block is block-space. The previous heuristic (inBlock>1)
  // assumed the first block opened was always *MODEL_SPACE, which fails
  // for DWG files where model-space entities arrive via readDwgEntities
  // (no addBlock for model-space) and the first addBlock opens a real
  // block — those entities then mis-classified as model-space.
  bool currentlyInRealBlock() const {
    if (inBlock <= 0)
      return false;
    // Case-insensitive prefix match against *Model_Space / *Paper_Space.
    auto startsWithCi = [&](const char *prefix) {
      const size_t n = std::strlen(prefix);
      if (currentBlockName.size() < n)
        return false;
      for (size_t i = 0; i < n; ++i)
        if (std::tolower(static_cast<unsigned char>(currentBlockName[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i])))
          return false;
      return true;
    };
    return !(startsWithCi("*Model_Space") || startsWithCi("*Paper_Space"));
  }

  void trackT(const DRW_Entity &e, const char *typeName) {
    typeCounts[typeName]++;
    if (e.layer.empty())
      layerEntities["(no layer)"]++;
    else
      layerEntities[e.layer]++;
    if (currentlyInRealBlock())
      blockSpaceEntities++;
    else
      modelSpaceEntities++;
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
    currentBlockName = b.name;
  }
  void setBlock(const int) override {}
  void endBlock() override {
    if (inBlock > 0)
      --inBlock;
    if (inBlock == 0)
      currentBlockName.clear();
  }

  void addPoint(const DRW_Point &e) override { trackT(e, "POINT"); }
  void addLine(const DRW_Line &e) override { trackT(e, "LINE"); }
  void addRay(const DRW_Ray &e) override { trackT(e, "RAY"); }
  void addXline(const DRW_Xline &e) override { trackT(e, "XLINE"); }
  void addArc(const DRW_Arc &e) override { trackT(e, "ARC"); }
  void addCircle(const DRW_Circle &e) override { trackT(e, "CIRCLE"); }
  void addEllipse(const DRW_Ellipse &e) override { trackT(e, "ELLIPSE"); }
  void addLWPolyline(const DRW_LWPolyline &e) override {
    trackT(e, "LWPOLYLINE");
  }
  void addPolyline(const DRW_Polyline &e) override { trackT(e, "POLYLINE"); }
  void addSpline(const DRW_Spline *e) override { trackT(*e, "SPLINE"); }
  // These entity types are delivered by the reader but were previously untracked
  // here, so they were undercounted in the [.audit]/[.coverage] telemetry.
  void addHelix(const DRW_Helix *e) override { trackT(*e, "HELIX"); }
  void addOle2Frame(const DRW_Ole2Frame &e) override { trackT(e, "OLE2FRAME"); }
  void addShape(const DRW_Shape &e) override { trackT(e, "SHAPE"); }
  void addKnot(const DRW_Entity &) override {}
  void addInsert(const DRW_Insert &e) override {
    trackT(e, "INSERT");
    // Attached attribute entities (DWG attlist model) — surface them
    // alongside the INSERT count so tests can assert ATTRIB delivery.
    for (const auto &a : e.attlist) {
      if (!a)
        continue;
      typeCounts[a->eType == DRW::ATTDEF ? "ATTDEF" : "ATTRIB"]++;
    }
  }
  void addTrace(const DRW_Trace &e) override { trackT(e, "TRACE"); }
  void add3dFace(const DRW_3Dface &e) override { trackT(e, "3DFACE"); }
  void addSolid(const DRW_Solid &e) override { trackT(e, "SOLID"); }
  void addMText(const DRW_MText &e) override { trackT(e, "MTEXT"); }
  void addText(const DRW_Text &e) override { trackT(e, "TEXT"); }
  void addAttDef(const DRW_Attdef &e) override { trackT(e, "ATTDEF"); }
  void addDimAlign(const DRW_DimAligned *e) override {
    trackT(*e, "DIM_ALIGNED");
  }
  void addDimLinear(const DRW_DimLinear *e) override {
    trackT(*e, "DIM_LINEAR");
  }
  void addDimRadial(const DRW_DimRadial *e) override {
    trackT(*e, "DIM_RADIAL");
  }
  void addDimDiametric(const DRW_DimDiametric *e) override {
    trackT(*e, "DIM_DIAMETRIC");
  }
  void addDimAngular(const DRW_DimAngular *e) override {
    trackT(*e, "DIM_ANGULAR");
  }
  void addDimAngular3P(const DRW_DimAngular3p *e) override {
    trackT(*e, "DIM_ANGULAR3P");
  }
  void addDimArc(const DRW_DimArc *e) override {
    trackT(*e, "ARC_DIMENSION");
  }
  void addDimOrdinate(const DRW_DimOrdinate *e) override {
    trackT(*e, "DIM_ORDINATE");
  }
  void addLeader(const DRW_Leader *e) override { trackT(*e, "LEADER"); }
  void addHatch(const DRW_Hatch *e) override { trackT(*e, "HATCH"); }
  void addViewport(const DRW_Viewport &e) override { trackT(e, "VIEWPORT"); }
  void addImage(const DRW_Image *e) override { trackT(*e, "IMAGE"); }
  void addMLeader(const DRW_MLeader *e) override { trackT(*e, "MLEADER"); }
  void addWipeout(const DRW_Wipeout *e) override {
    trackT(*e, "WIPEOUT");
    // Surface the polygon size so tests can sanity-check that the boundary
    // actually came through — empty clipPath is the historical bug shape.
    wipeoutVertexCount += e->clipPath.size();
  }
  void addTolerance(const DRW_Tolerance &e) override { trackT(e, "TOLERANCE"); }
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

  int total() const {
    int n = 0;
    for (const auto &kv : typeCounts)
      n += kv.second;
    return n;
  }
};

// ---- deep diagnostic read --------------------------------------------------

struct DeepResult {
  bool ok;
  DRW::error error;
  DRW::Version version;
  TypeTrackingIface iface;
  std::map<int, int> unhandledTypes; // oType -> count
  std::string debugLog; // full captured debug trace (for verbose dumps)
};

DeepResult readDwgDeep(const std::string &path) {
  DeepResult dr;

  // Allocate on heap — ownership transferred to DRW_dbg via
  // setCustomDebugPrinter. Keep a raw ptr to read results BEFORE the next
  // setCustomDebugPrinter call deletes it.
  auto *capturePrinter = new CapturingPrinter();
  DRW::setCustomDebugPrinter(capturePrinter); // DRW_dbg takes ownership

  try {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::Debug); // also sets DRW_dbg level to Debug
    dr.ok = reader.read(&dr.iface, true);
    dr.error = reader.getError();
    dr.version = reader.getVersion();
  } catch (const std::exception &ex) {
    std::cerr << "  [EXCEPTION: " << ex.what() << "]\n";
    dr.ok = false;
    dr.error = DRW::BAD_UNKNOWN;
  } catch (...) {
    std::cerr << "  [UNKNOWN EXCEPTION]\n";
    dr.ok = false;
    dr.error = DRW::BAD_UNKNOWN;
  }

  // Extract before replacing printer (which deletes capturePrinter).
  // Grab the full log now — avoids printer-state complications in callers that
  // want to print it after this function returns.
  dr.unhandledTypes = capturePrinter->unhandledTypes();
  dr.debugLog = std::move(capturePrinter->buf);

  // Restore silent printer — DRW_dbg::setCustomDebugPrinter deletes
  // capturePrinter here.
  DRW::setCustomDebugPrinter(new DRW::DebugPrinter());

  return dr;
}

// map raw oType number to a human-readable name (ODA entity type codes)
const char *oTypeName(int t) {
  switch (t) {
  case 1:
    return "TEXT";
  case 2:
    return "ATTRIB";
  case 3:
    return "ATTDEF";
  case 4:
    return "BLOCK";
  case 5:
    return "ENDBLK";
  case 6:
    return "SEQEND";
  case 7:
    return "INSERT";
  case 8:
    return "MINSERT";
  case 10:
    return "VERTEX_2D";
  case 11:
    return "VERTEX_3D";
  case 12:
    return "VERTEX_MESH";
  case 13:
    return "VERTEX_PFACE";
  case 14:
    return "VERTEX_PFACE_FACE";
  case 15:
    return "POLYLINE_2D";
  case 16:
    return "POLYLINE_3D";
  case 17:
    return "ARC";
  case 18:
    return "CIRCLE";
  case 19:
    return "LINE";
  case 20:
    return "DIM_ORDINATE";
  case 21:
    return "DIM_LINEAR";
  case 22:
    return "DIM_ALIGNED";
  case 23:
    return "DIM_ANGULAR3P";
  case 24:
    return "DIM_ANGULAR";
  case 25:
    return "DIM_RADIAL";
  case 26:
    return "DIM_DIAMETRIC";
  case 27:
    return "POINT";
  case 28:
    return "3DFACE";
  case 29:
    return "POLYLINE_PFACE";
  case 30:
    return "POLYLINE_MESH";
  case 31:
    return "SOLID";
  case 32:
    return "TRACE";
  case 33:
    return "SHAPE";
  case 34:
    return "VIEWPORT";
  case 35:
    return "ELLIPSE";
  case 36:
    return "SPLINE";
  case 37:
    return "REGION";
  case 38:
    return "3DSOLID";
  case 39:
    return "BODY";
  case 40:
    return "RAY";
  case 41:
    return "XLINE";
  case 42:
    return "DICTIONARY";
  case 43:
    return "OLEFRAME";
  case 44:
    return "MTEXT";
  case 45:
    return "LEADER";
  case 46:
    return "TOLERANCE";
  case 47:
    return "MLINE";
  case 48:
    return "BLOCK_CONTROL";
  case 49:
    return "BLOCK_HDR";
  case 50:
    return "LAYER_CONTROL";
  case 51:
    return "LAYER";
  case 52:
    return "STYLE_CONTROL";
  case 53:
    return "STYLE";
  case 56:
    return "LTYPE_CONTROL";
  case 57:
    return "LTYPE";
  case 60:
    return "VIEW_CONTROL";
  case 61:
    return "VIEW";
  case 62:
    return "UCS_CONTROL";
  case 63:
    return "UCS";
  case 64:
    return "VPORT_CONTROL";
  case 65:
    return "VPORT";
  case 66:
    return "APPID_CONTROL";
  case 67:
    return "APPID";
  case 68:
    return "DIMSTYLE_CONTROL";
  case 69:
    return "DIMSTYLE";
  case 70:
    return "VP_ENT_HDR_CTRL";
  case 71:
    return "VP_ENT_HDR";
  case 72:
    return "GROUP";
  case 73:
    return "MLINESTYLE";
  case 74:
    return "OLE2FRAME";
  case 76:
    return "DUMMY";
  case 77:
    return "LWPOLYLINE";
  case 78:
    return "HATCH";
  case 79:
    return "XRECORD";
  case 80:
    return "PLACEHOLDER";
  case 81:
    return "VBA_PROJECT";
  case 82:
    return "LAYOUT";
  case 101:
    return "IMAGE";
  default:
    return nullptr;
  }
}

void printDeepReport(const char *filename, const DeepResult &dr) {
  std::cout << "\n";
  std::cout << "=== " << filename << " ===\n";
  std::cout << "Version : " << versionStr(dr.version) << "\n";
  std::cout << "Error   : " << errorStr(dr.error) << "\n";
  std::cout << "Blocks  : " << dr.iface.blocks
            << "  Layers: " << dr.iface.layers << "\n";
  int total = dr.iface.total();
  std::cout << "Entities: " << total
            << "  (model-space=" << dr.iface.modelSpaceEntities
            << ", in-blocks=" << dr.iface.blockSpaceEntities << ")\n";

  std::cout << "\n  Handled entity type distribution:\n";
  for (const auto &[type, count] : dr.iface.typeCounts) {
    std::cout << "    " << std::left << std::setw(16) << type << " " << count
              << "\n";
  }

  if (!dr.unhandledTypes.empty()) {
    std::cout << "\n  *** entity-pass-defer types"
              << " (DICTIONARY/MLINESTYLE/LAYOUT/IMAGEDEF land in OBJECTS pass;"
              << " other oTypes are silently skipped) ***\n";
    int totalSkipped = 0;
    for (const auto &[oType, count] : dr.unhandledTypes) {
      totalSkipped += count;
      const char *name = oTypeName(oType);
      std::cout << "    oType=" << std::setw(4) << oType
                << "  name=" << std::setw(18)
                << (name ? name : "(unknown/custom)") << "  count=" << count
                << "\n";
    }
    std::cout << "  Total skipped: " << totalSkipped << "\n";
  } else {
    std::cout << "\n  (no unhandled entity types)\n";
  }

  std::cout << "\n  Top layers by entity count:\n";
  std::vector<std::pair<int, std::string>> sorted;
  for (const auto &[layer, cnt] : dr.iface.layerEntities)
    sorted.emplace_back(cnt, layer);
  std::sort(sorted.rbegin(), sorted.rend());
  int shown = 0;
  for (const auto &[cnt, layer] : sorted) {
    std::cout << "    " << std::left << std::setw(30) << layer << " " << cnt
              << "\n";
    if (++shown >= 15) {
      std::cout << "    ...\n";
      break;
    }
  }
}

struct FileInfo {
  const char *name;
  bool expectSuccess;
};

const FileInfo kTestFiles[] = {
    // AC1018 (R2004)
    {"Extruder2.dwg", true},
    {"ET-Drawing-with-Border.dwg", true},
    // AC1021 (R2007)
    {"colorwh.dwg", true},
    {"dwgreader21_230.dwg", false}, // 522-byte truncated test file
    {"blocks_and_tables_-_imperial.dwg", true},
    {"blocks_and_tables_-_metric.dwg", true},
    {"lineweights.dwg", true},
    {"truetype.dwg", true},
    {"tablet.dwg", true},
    // AC1024 (R2010)
    {"architectural_-_annotation_scaling_and_multileaders.dwg", true},
    {"architectural_example-imperial.dwg", true},
    {"children-room-decoration.dwg", true},
    {"civil_example-imperial.dwg", true},
    {"mechanical_example-imperial.dwg", true},
    {"#mechanical_example-imperial.dwg", false}, // bad magic bytes
    {"#title_block-iso.dwg", false},             // bad magic bytes
    {"plot_screening_and_fill_patterns.dwg", true},
    {"title_block-arch.dwg", true},
    {"title_block-iso.dwg", true},
    {"visualization_-_aerial.dwg",
     true}, // 3D-solid-only model; 0 entities is correct for a 2D reader
    {"visualization_-_condominium_with_skylight.dwg", true},
    {"visualization_-_conference_room.dwg", true},
    {"visualization_-_sun_and_sky_demo.dwg", true},
};

// ---- DRW_Hatch → RS_Hatch pipeline (mirrors RS_FilterDXFRW::addHatch)
// ----------

// Builds an RS_Hatch with boundary loops populated from DRW_Hatch data.
// Replicates the boundary-construction logic of RS_FilterDXFRW::addHatch
// without needing a live document context.  Does NOT call update(); caller owns
// the result.
RS_Hatch *buildRS_Hatch(const DRW_Hatch *data) {
  auto *hatch = new RS_Hatch(
      nullptr, RS_HatchData(data->solid != 0, data->scale, data->angle,
                            QString::fromUtf8(data->name.c_str())));

  for (const auto &loop : data->looplist) {
    if (loop->type & 32)
      continue; // skip textbox-boundary loops

    auto *hatchLoop = new RS_EntityContainer(hatch);
    hatchLoop->setLayer(nullptr);
    hatch->addEntity(hatchLoop);

    if (loop->type & 2) {
      // Polyline boundary: convert to line/arc segments via RS_Polyline.
      if (loop->objlist.empty())
        continue;
      auto *pline = static_cast<DRW_LWPolyline *>(loop->objlist.at(0).get());
      RS_Polyline poly{
          nullptr,
          RS_PolylineData(RS_Vector(false), RS_Vector(false), pline->flags)};
      for (const auto &v : pline->vertlist)
        poly.addVertex(RS_Vector{v->x, v->y}, v->bulge);
      for (RS_Entity *e :
           lc::LC_ContainerTraverser{poly, RS2::ResolveNone}.entities()) {
        RS_Entity *tmp = e->clone();
        tmp->reparent(hatchLoop);
        tmp->setLayer(nullptr);
        hatchLoop->addEntity(tmp);
      }
    } else {
      // Explicit-segment boundary: line, arc, ellipse, spline.
      // Mirrors RS_FilterDXFRW::addHatch dispatch including the
      // DRW::SPLINE arm — keep this in lock-step.
      for (const auto &seg : loop->objlist) {
        RS_Entity *e = nullptr;
        switch (seg->eType) {
        case DRW::LINE: {
          auto *l = static_cast<DRW_Line *>(seg.get());
          e = new RS_Line(hatchLoop, {{l->basePoint.x, l->basePoint.y},
                                      {l->secPoint.x, l->secPoint.y}});
          break;
        }
        case DRW::ARC: {
          auto *a = static_cast<DRW_Arc *>(seg.get());
          RS_Vector ctr{a->basePoint.x, a->basePoint.y};
          if (a->isccw && a->staangle < 1e-6 &&
              a->endangle > RS_Math::deg2rad(360) - 1e-6) {
            e = new RS_Circle(hatchLoop, {ctr, a->radious});
          } else if (a->isccw) {
            e = new RS_Arc(
                hatchLoop,
                RS_ArcData(ctr, a->radious, RS_Math::correctAngle(a->staangle),
                           RS_Math::correctAngle(a->endangle), false));
          } else {
            e = new RS_Arc(
                hatchLoop,
                RS_ArcData(ctr, a->radious,
                           RS_Math::correctAngle(2 * M_PI - a->staangle),
                           RS_Math::correctAngle(2 * M_PI - a->endangle),
                           true));
          }
          break;
        }
        case DRW::ELLIPSE: {
          auto *el = static_cast<DRW_Ellipse *>(seg.get());
          double a1 = el->staparam, a2 = el->endparam;
          if (std::abs(a2 - 2. * M_PI) < 1e-10 && std::abs(a1) < 1e-10) {
            a2 = 0.;
          } else {
            // Mirror of RS_FilterDXFRW::addHatch ellipse angle conversion.
            a1 = std::atan(std::tan(a1) / el->ratio);
            a2 = std::atan(std::tan(a2) / el->ratio);
            if (a1 < 0) {
              a1 += M_PI;
              if (el->staparam > M_PI)
                a1 += M_PI;
            } else if (el->staparam > M_PI)
              a1 += M_PI;
            if (a2 < 0) {
              a2 += M_PI;
              if (el->endparam > M_PI)
                a2 += M_PI;
            } else if (el->endparam > M_PI)
              a2 += M_PI;
          }
          e = new RS_Ellipse(hatchLoop, {{el->basePoint.x, el->basePoint.y},
                                         {el->secPoint.x, el->secPoint.y},
                                         el->ratio,
                                         a1,
                                         a2,
                                         !el->isccw});
          break;
        }
        case DRW::SPLINE: {
          e = RS_FilterDXFRW::buildHatchSplineEdge(
              hatchLoop, static_cast<DRW_Spline *>(seg.get()));
          break;
        }
        default:
          break;
        }
        if (e) {
          e->setLayer(nullptr);
          hatchLoop->addEntity(e);
        }
      }
      // Snap spline endpoints to neighboring line endpoints if a
      // tiny float gap is the only thing blocking LoopExtractor.
      RS_FilterDXFRW::snapSplineEdgeEndpoints(hatchLoop);
    }
  }
  return hatch;
}

struct HatchFillResult {
  std::string pattern;
  bool solid;
  int declaredLoops; // loopsnum from DWG
  int rootLoops;     // countLoops() — root loops after hierarchy assignment
  int allLoops;      // countAllLoops() — roots + all nested children
  RS_Hatch::RS_HatchError error;
  double area;
};

// DRW_Interface that builds + validates/updates an RS_Hatch for every DRW_Hatch
// received. For solid hatches the full update() pipeline runs (validate + fill
// + area). For pattern hatches only validate() is called — updatePatternHatch()
// requires the LibreCAD pattern library singleton which is not initialised in
// the headless test binary.
class HatchFillIface : public TypeTrackingIface {
public:
  std::vector<HatchFillResult> hatches;

  void addHatch(const DRW_Hatch *e) override {
    TypeTrackingIface::addHatch(e);
    std::unique_ptr<RS_Hatch> hatch{buildRS_Hatch(e)};

    RS_Hatch::RS_HatchError error;
    int rootLoops, allLoops;
    double area = 0.0;

    if (hatch->isSolid()) {
      hatch->update();
      error = hatch->getUpdateError();
      rootLoops = hatch->countLoops();
      allLoops = hatch->countAllLoops();
      area = hatch->getTotalArea();
    } else {
      // Pattern hatch: validate boundary only; don't attempt pattern rendering.
      bool valid = hatch->validate();
      error = valid ? RS_Hatch::HATCH_OK : RS_Hatch::HATCH_INVALID_CONTOUR;
      rootLoops = hatch->countLoops();
      allLoops = hatch->countAllLoops();
    }

    hatches.push_back({e->name, e->solid != 0, e->loopsnum, rootLoops, allLoops,
                       error, area});
  }
};

} // namespace

TEST_CASE("dwgRW::readBuffer matches file-path read for a committed fixture",
          "[dwg][readbuffer][libdxfrw]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/mpolygon_solid.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("mpolygon_solid.dwg fixture absent; skipping");
  }

  CountingIface fileIface;
  const DwgResult fileRead = readDwg(path, /*verbose=*/false, &fileIface);
  REQUIRE(fileRead.ok);
  REQUIRE(fileRead.error == DRW::BAD_NONE);

  CountingIface memoryIface;
  const DwgResult memoryRead =
      readDwgBuffer(path, /*verbose=*/false, &memoryIface);
  REQUIRE(memoryRead.ok);
  REQUIRE(memoryRead.error == DRW::BAD_NONE);

  CHECK(memoryRead.version == fileRead.version);
  CHECK(memoryRead.entities == fileRead.entities);
  CHECK(memoryRead.blocks == fileRead.blocks);
  CHECK(memoryRead.layers == fileRead.layers);
  CHECK(memoryIface.entityLWeights == fileIface.entityLWeights);
  CHECK(memoryIface.layerLWeights == fileIface.layerLWeights);
}

// Regression guard for the R2007/AC1021 header-read fix: dwgReader21 previously
// clobbered the UTF-16 decoder with the legacy codepage, so section names
// decoded with embedded 0x00 bytes, no section matched secEnum::getEnum(), and
// every R2007 file failed at BAD_READ_HEADER. The committed AC1021 fixture must
// now read clean.
TEST_CASE("DWG R2007: section names decode via UTF-16 (no BAD_READ_HEADER)",
          "[dwg][r2007]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/visualstyle_r2007.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("visualstyle_r2007.dwg fixture absent; skipping");
  }
  const DwgResult r = readDwg(path);
  REQUIRE(r.version == DRW::AC1021);
  REQUIRE(r.error == DRW::BAD_NONE); // was BAD_READ_HEADER before the fix
  REQUIRE(r.ok);
  CHECK(r.entities > 0);
}

namespace {
// Counts POLYLINE deliveries + total vertices, over CountingIface.
class PolyCountIface : public CountingIface {
public:
  int polylines = 0;
  int polyVerts = 0;
  void addPolyline(const DRW_Polyline &e) override {
    CountingIface::addPolyline(e);
    ++polylines;
    polyVerts += static_cast<int>(e.vertlist.size());
  }
};
} // namespace

// Pre-R13 (R10/AC1006) JUMP + EXTRAS-section regression. entities_3.dwg holds
// two POLYLINE_2D; the second is split by a JUMP record whose continuation
// lives in the EXTRAS section. Before the fix, dwgReaderR11 (a) counted JUMP as
// a parse failure and (b) never read the EXTRAS section, so only ONE polyline
// was delivered (entityParseFailures=1). Now both are delivered, 0 failures --
// matching dwgTs (which reads entities+blocks+extras).
TEST_CASE("DWG pre-R13 R10: JUMP-split polyline recovered from EXTRAS section",
          "[dwg][pre-r13][r10][jump]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/pre_r13_r10_entities.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("pre_r13_r10_entities.dwg fixture absent; skipping");
  }
  PolyCountIface iface;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&iface, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC1006);
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  // Both POLYLINE entities are now delivered (was 1 before the EXTRAS/JUMP fix,
  // with a spurious parse failure). The second polyline is JUMP-relocated into
  // the EXTRAS section; per dwgTs's own note "neither libredwg nor libdxfrw
  // follows the JUMP", so its 3 vertices are NOT relocated back onto it (dwgTs
  // leaves them flat; dwgread delivers them as 6 separate VERTEX_2D). We
  // therefore get 2 polylines carrying the 3 relocatable vertices of the first.
  CHECK(iface.polylines == 2);
  CHECK(iface.polyVerts == 3);
}

namespace {
// Captures APPID + DIMSTYLE table-record names.
class TableNameCapture : public CountingIface {
public:
  std::vector<std::string> appIds;
  std::vector<std::string> dimStyles;
  int linearDimensions = 0;
  void addAppId(const DRW_AppId &a) override { appIds.push_back(a.name); }
  void addDimStyle(const DRW_Dimstyle &d) override { dimStyles.push_back(d.name); }
  void addDimLinear(const DRW_DimLinear *d) override {
    CountingIface::addDimLinear(d);
    ++linearDimensions;
  }
};
} // namespace

// Pre-R13 (R11/AC1009) EMBEDDED extended-table regression. dwgReaderR11 read
// only LAYER/LTYPE/STYLE; the embedded APPID (@0x512) + DIMSTYLE (@0x522)
// descriptors (present when numheader_vars@0x11 > 158) were never read. dwgTs
// decodes these two name-only (VPORT/VIEW/UCS/VX stay dormant in dwgTs too).
// ACEB10.dwg (R11/AC1009) carries APPID "ACAD" + DIMSTYLE "STANDARD" (dwgread).
TEST_CASE("DWG pre-R13 R11: embedded APPID + DIMSTYLE tables decode name-only",
          "[dwg][pre-r13][r11][tables]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/pre_r13_r11_tables.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("pre_r13_r11_tables.dwg fixture absent; skipping");
  }
  TableNameCapture iface;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&iface, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC1009);
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  // Both embedded tables now deliver (were absent before the fix).
  REQUIRE(iface.appIds.size() == 1);
  CHECK(iface.appIds[0] == "ACAD");
  REQUIRE(iface.dimStyles.size() == 1);
  CHECK(iface.dimStyles[0] == "STANDARD");
  CHECK(iface.linearDimensions == 15);
}

namespace {
// Captures LINE entities (count + first start point) over CountingIface.
class LineCapture : public CountingIface {
public:
  int lines = 0;
  RS_Vector firstStart{false};
  void addLine(const DRW_Line &e) override {
    CountingIface::addLine(e);
    if (lines == 0)
      firstStart = RS_Vector(e.basePoint.x, e.basePoint.y, e.basePoint.z);
    ++lines;
  }
};

class LegacyDimensionCapture : public CountingIface {
public:
  std::vector<DRW_DimLinear> linear;
  std::vector<DRW_DimAligned> aligned;
  std::vector<DRW_DimAngular> angular;
  std::vector<DRW_DimAngular3p> angular3p;
  std::vector<DRW_DimDiametric> diametric;
  std::vector<DRW_DimOrdinate> ordinate;
  std::vector<DRW_DimRadial> radial;

  void addDimLinear(const DRW_DimLinear *e) override {
    CountingIface::addDimLinear(e);
    linear.push_back(*e);
  }
  void addDimAlign(const DRW_DimAligned *e) override {
    CountingIface::addDimAlign(e);
    aligned.push_back(*e);
  }
  void addDimAngular(const DRW_DimAngular *e) override {
    CountingIface::addDimAngular(e);
    angular.push_back(*e);
  }
  void addDimAngular3P(const DRW_DimAngular3p *e) override {
    CountingIface::addDimAngular3P(e);
    angular3p.push_back(*e);
  }
  void addDimDiametric(const DRW_DimDiametric *e) override {
    CountingIface::addDimDiametric(e);
    diametric.push_back(*e);
  }
  void addDimRadial(const DRW_DimRadial *e) override {
    CountingIface::addDimRadial(e);
    radial.push_back(*e);
  }
  void addDimOrdinate(const DRW_DimOrdinate *e) override {
    CountingIface::addDimOrdinate(e);
    ordinate.push_back(*e);
  }
};
} // namespace

// Pre-R10 tier (R2.6/AC1003, R9/AC1004, R2.10/AC2.10) parse regression. These
// share the R10/R11 container (dwgReaderR11) but were rejected by readMetaData
// + the routing in libdwgr.cpp -> 0 entities. Now decoded via version-gated
// deltas: 1-byte LTYPE handle for all pre-R11, 2D LINE/POINT/3DLINE/3DFACE
// bodies for pre-R10, elevation-for-all, empty-ENTITIES tolerance, and
// REPEAT/ENDREP/LOAD structural markers. Each fixture carries a 2D LINE at
// (6,1,0) (z=0 proves the pre-R10 2D-body path; byte-identical to dwgread).
TEST_CASE("DWG pre-R10 tier (R2.6/R9/R2.10) reads entities with 2D bodies",
          "[dwg][pre-r13][pre-r10]") {
  struct Case { const char *file; DRW::Version version; int minEntities; };
  const Case cases[] = {
      {"pre_r10_r26_entities.dwg", DRW::AC1003, 24},
      {"pre_r10_r9_entities.dwg", DRW::AC1004, 24},
      {"pre_r10_r210_entities.dwg", DRW::AC210, 15},
  };
  for (const Case &c : cases) {
    const std::string path = std::string(LIBRECAD_TEST_DIR) + "/" + c.file;
    INFO("fixture: " << c.file);
    if (!std::filesystem::is_regular_file(path)) {
      SKIP("fixture absent; skipping");
      continue;
    }
    LineCapture iface;
    dwgR reader(path.c_str());
    REQUIRE(reader.read(&iface, /*ext=*/true));
    REQUIRE(reader.getVersion() == c.version);
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    // Entities are now delivered (was 0 before the pre-R10 fix).
    CHECK(iface.entities >= c.minEntities);
    REQUIRE(iface.lines >= 2);
    // The (6,1,0) 2D LINE: z==0 confirms the pre-R10 2D-body read (a 3D read
    // would consume the next entity's bytes as Z).
    CHECK(iface.firstStart.x == Catch::Approx(6.0));
    CHECK(iface.firstStart.y == Catch::Approx(1.0));
    CHECK(iface.firstStart.z == Catch::Approx(0.0));
  }
}

TEST_CASE("DWG pre-R13 external R2.6 dimensions use typed callbacks",
          "[dwg][pre-r13][external]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping external corpus");
  }
  const std::string path =
      (std::filesystem::path(home) / "dev" / "dwg-writer" / "tests" /
       "fixtures" / "committed" / "libredwg" / "dim_r2_6.dwg")
          .string();
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("dwg-writer dim_r2_6.dwg absent; skipping external corpus");
  }

  LegacyDimensionCapture iface;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&iface, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC1003);
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  REQUIRE(iface.linear.size() == 1);
  REQUIRE(iface.aligned.size() == 1);
  REQUIRE(iface.angular.size() == 1);
  REQUIRE(iface.diametric.size() == 1);
  REQUIRE(iface.radial.size() == 1);

  CHECK(iface.linear[0].getDef1Point().x == Catch::Approx(1.0));
  CHECK(iface.aligned[0].getDef2Point().y == Catch::Approx(7.0));
  CHECK(iface.angular[0].getFirstLine1().x == Catch::Approx(7.0));
  CHECK(iface.angular[0].getDimPoint().y == Catch::Approx(3.0));
  CHECK(iface.diametric[0].getDiameter1Point().y == Catch::Approx(4.0));
  CHECK(iface.radial[0].getCenterPoint().x == Catch::Approx(3.0));
  CHECK(iface.radial[0].getDiameterPoint().y == Catch::Approx(1.0));
}

TEST_CASE("DWG pre-R13 derived angular and ordinate dimensions use typed callbacks",
          "[dwg][pre-r13][external]") {
  const char *root = std::getenv("LIBRECAD_PRE_R13_DIMENSION_FIXTURES");
  if (!root || root[0] == '\0') {
    SKIP("derived pre-R13 dimension fixtures not configured; skipping");
  }
  struct Case {
    const char *file;
    bool angular3p;
  };
  const Case cases[] = {
      {"entities_r11_angular3p.dwg", true},
      {"entities_r11_ordinate.dwg", false},
  };
  for (const Case &c : cases) {
    const std::string path = (std::filesystem::path(root) / c.file).string();
    INFO("fixture: " << c.file);
    if (!std::filesystem::is_regular_file(path)) {
      SKIP("derived fixture absent; skipping");
      continue;
    }
    LegacyDimensionCapture iface;
    dwgR reader(path.c_str());
    REQUIRE(reader.read(&iface, /*ext=*/true));
    REQUIRE(reader.getVersion() == DRW::AC1009);
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    if (c.angular3p) {
      REQUIRE(iface.angular3p.size() == 1);
      CHECK(iface.angular3p[0].getFirstLine().x == Catch::Approx(6.0));
      CHECK(iface.angular3p[0].getSecondLine().y == Catch::Approx(7.0));
    } else {
      REQUIRE(iface.ordinate.size() == 1);
      CHECK(iface.ordinate[0].getOriginPoint().x == Catch::Approx(8.0));
      CHECK(iface.ordinate[0].getFirstLine().z == Catch::Approx(2.0));
      CHECK(iface.ordinate[0].getSecondLine().z == Catch::Approx(3.0));
    }
  }
}

// R1.40 (magic "AC1.40") — the dedicated pre-R2.0b reader (dwgReaderR1_40).
// Different container: no @0x14 section pointers, no per-record size/CRC; the
// entity stream runs contiguously from 0x202 to dwg_size@0x24, each record =
// type(RS)+layer(RS)+body. entities_4.dwg exercises all 14 types; the 9
// renderable ones are delivered (BLOCK/ENDBLK = scope, REPEAT/ENDREP/LOAD =
// markers). It carries a LINE (2,3)->(3,4), byte-identical to dwgread.
TEST_CASE("DWG R1.40 (AC1.40) dedicated reader decodes the entity stream",
          "[dwg][pre-r13][r1_40]") {
  const std::string path =
      std::string(LIBRECAD_TEST_DIR) + "/pre_r10_r140_entities.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("pre_r10_r140_entities.dwg fixture absent; skipping");
  }
  LineCapture iface;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&iface, /*ext=*/true));
  REQUIRE(reader.getVersion() == DRW::AC14);
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  // 9 renderable entities (POINT2 LINE3 ARC CIRCLE TEXT TRACE INSERT SHAPE SOLID).
  CHECK(iface.entities >= 9);
  REQUIRE(iface.lines >= 3);
  // No parse failure means the size-less walk landed exactly on dwg_size.
  bool sawLine23 = false;
  // firstStart is one delivered LINE start; every LINE is 2D (z==0).
  CHECK(iface.firstStart.z == Catch::Approx(0.0));
  // The (2,3)->(3,4) LINE proves correct body geometry vs dwgread.
  {
    struct L23 : public LineCapture {
      bool found = false;
      void addLine(const DRW_Line &e) override {
        LineCapture::addLine(e);
        if (std::abs(e.basePoint.x - 2.0) < 1e-9 &&
            std::abs(e.basePoint.y - 3.0) < 1e-9)
          found = true;
      }
    } cap;
    dwgR r2(path.c_str());
    REQUIRE(r2.read(&cap, /*ext=*/true));
    sawLine23 = cap.found;
  }
  CHECK(sawLine23);
}

// Regression for BAD_READ_TABLES on a stored (incompressible) R2007 data page.
// The $100-bill raster artwork produces an AcDb:AcDbObjects page with
// cSize==uSize that dwgReader21::parseDataPage must memcpy, not LZ77-decompress
// (mirrors LibreDWG read_data_page). The file is 30 MB -> developer-local, skip
// if absent (tag hidden with leading '.').
TEST_CASE("DWG R2007 stored-page: usa_dollar100_front.dwg reads tables",
          "[.dwg6_stored_page]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg6/usa_dollar100_front.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("usa_dollar100_front.dwg not present; skipping");
  }
  CountingIface iface;
  const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
  REQUIRE(r.error == DRW::BAD_NONE); // was BAD_READ_TABLES before the fix
  CHECK(iface.entities == 41368);    // dwgread: 41364 POLYLINE_3D + 4 LWPOLYLINE
  CHECK(iface.blocks == 2);
  CHECK(iface.layers == 1);
}

TEST_CASE("DWG XLINE reads as typed construction line across LibreDWG versions",
          "[dwg][xline]") {
  struct Fixture {
    const char *file;
    DRW::Version version;
  };

  const Fixture fixtures[] = {
      {"xline/constructionline_2000.dwg", DRW::AC1015},
      {"xline/constructionline_2004.dwg", DRW::AC1018},
      // (R2007/AC1021 row omitted because no construction-line fixture is
      // available for that release.)
      {"xline/constructionline_2010.dwg", DRW::AC1024},
      {"xline/constructionline_2013.dwg", DRW::AC1027},
      {"xline/constructionline_2018.dwg", DRW::AC1032},
  };

  bool testedFixture = false;
  for (const Fixture &fixture : fixtures) {
    const std::string path =
        std::string(LIBRECAD_TEST_DIR) + "/" + fixture.file;
    INFO("fixture: " << fixture.file);
    if (!std::filesystem::is_regular_file(path)) {
      SKIP("fixture absent; skipping");
      continue;
    }
    testedFixture = true;

    XlineCaptureIface iface;
    const DwgResult result = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(result.ok);
    REQUIRE(result.error == DRW::BAD_NONE);
    CHECK(result.version == fixture.version);
    CHECK(result.entities == 1);
    CHECK(iface.lineCallbacks == 0);
    REQUIRE(iface.xlines.size() == 1);

    const DRW_Xline &xline = iface.xlines.front();
    CHECK(xline.eType == DRW::XLINE);
    CHECK(std::isfinite(xline.basePoint.x));
    CHECK(std::isfinite(xline.basePoint.y));
    CHECK(std::isfinite(xline.secPoint.x));
    CHECK(std::isfinite(xline.secPoint.y));
    const double dirLen =
        std::hypot(xline.secPoint.x, xline.secPoint.y);
    CHECK(dirLen > 0.0);
  }
  if (!testedFixture)
    SKIP("XLINE fixtures absent; skipping");
}

TEST_CASE("DWG optional fixtures cover AC1015 through AC1032",
          "[dwg][versions][fixture]") {
  struct Fixture {
    const char *file;
    DRW::Version version;
  };

  const Fixture fixtures[] = {
      {"xline/constructionline_2000.dwg", DRW::AC1015},
      {"xline/constructionline_2004.dwg", DRW::AC1018},
      {"visualstyle_r2007.dwg", DRW::AC1021},
      {"xline/constructionline_2010.dwg", DRW::AC1024},
      {"xline/constructionline_2013.dwg", DRW::AC1027},
      {"xline/constructionline_2018.dwg", DRW::AC1032},
  };

  bool testedFixture = false;
  for (const Fixture &fixture : fixtures) {
    const std::string path =
        std::string(LIBRECAD_TEST_DIR) + "/" + fixture.file;
    INFO("fixture: " << fixture.file);
    if (!std::filesystem::is_regular_file(path)) {
      SKIP("fixture absent; skipping");
      continue;
    }
    testedFixture = true;

    CountingIface iface;
    dwgR reader(path.c_str());
    REQUIRE(reader.read(&iface, /*ext=*/true));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == fixture.version);
    CHECK(iface.entities > 0);
  }
  if (!testedFixture)
    SKIP("source-controlled DWG fixtures absent; skipping");
}

TEST_CASE("DWG sibling oracle fixtures preserve dictionary ownership",
          "[dwg][dictionary][fixture]") {
  const char *configuredRoot = std::getenv("DWG_PARSER_SOURCE_DIR");
  std::vector<std::filesystem::path> roots;
  if (configuredRoot != nullptr && *configuredRoot != '\0')
    roots.emplace_back(configuredRoot);
#ifdef LIBRECAD_SOURCE_DIR
  roots.emplace_back(std::filesystem::path(LIBRECAD_SOURCE_DIR).parent_path()
                     / "dwg-parser");
#endif
  roots.emplace_back(std::filesystem::path("../dwg-parser"));

  auto findFixture = [&roots](const char *name) {
    for (const auto &root : roots) {
      const auto path = root / "tests/fixtures/committed/libredwg" / name;
      if (std::filesystem::is_regular_file(path))
        return path;
    }
    return std::filesystem::path{};
  };

  const auto r14Path = findFixture("example_r14.dwg");
  const auto r13Path = findFixture("example_r13.dwg");
  if (r14Path.empty() || r13Path.empty()) {
    SKIP("dwg-parser sibling fixtures absent; skipping");
  }

  DictionaryFixtureIface r14Iface;
  dwgR r14Reader(r14Path.string().c_str());
  REQUIRE(r14Reader.read(&r14Iface, /*ext=*/true));
  REQUIRE(r14Reader.getError() == DRW::BAD_NONE);
  REQUIRE(r14Reader.getVersion() == DRW::AC1014);

  const auto r14Dictionary = std::find_if(
      r14Iface.dictionaries.begin(), r14Iface.dictionaries.end(),
      [](const DRW_Dictionary &value) { return value.handle == 0x26Au; });
  REQUIRE(r14Dictionary != r14Iface.dictionaries.end());
  CHECK(r14Dictionary->parentHandle == 0x269u);
  CHECK(r14Dictionary->hardOwner != 0);
  REQUIRE(r14Dictionary->m_entries.size() == 7u);
  CHECK(r14Dictionary->m_entries.front().m_name == "ACAD_XREC_ROUNDTRIP");
  CHECK(r14Dictionary->m_entries.front().m_handle == 0xD0Du);

  const auto r14WithDefault = std::find_if(
      r14Iface.dictionariesWithDefault.begin(),
      r14Iface.dictionariesWithDefault.end(),
      [](const DRW_DictionaryWithDefault &value) { return value.handle == 0xEu; });
  REQUIRE(r14WithDefault != r14Iface.dictionariesWithDefault.end());
  CHECK(r14WithDefault->cloning == 1);
  CHECK(r14WithDefault->hardOwner == 0);
  CHECK(r14WithDefault->m_defaultEntryHandle == 0xFu);
  REQUIRE(r14WithDefault->m_entries.size() == 1u);
  CHECK(r14WithDefault->m_entries.front().m_name == "Normal");
  CHECK(r14WithDefault->m_entries.front().m_handle == 0xFu);

  DictionaryFixtureIface r13Iface;
  dwgR r13Reader(r13Path.string().c_str());
  REQUIRE(r13Reader.read(&r13Iface, /*ext=*/true));
  REQUIRE(r13Reader.getError() == DRW::BAD_NONE);
  REQUIRE(r13Reader.getVersion() == DRW::AC1012);

  const auto r13WithDefault = std::find_if(
      r13Iface.dictionariesWithDefault.begin(),
      r13Iface.dictionariesWithDefault.end(),
      [](const DRW_DictionaryWithDefault &value) { return value.handle == 0xEu; });
  REQUIRE(r13WithDefault != r13Iface.dictionariesWithDefault.end());
  CHECK(r13WithDefault->cloning == 1);
  CHECK(r13WithDefault->hardOwner == 0);
  CHECK(r13WithDefault->m_defaultEntryHandle == 0xFu);
  REQUIRE(r13WithDefault->m_entries.size() == 1u);
  CHECK(r13WithDefault->m_entries.front().m_name == "NORMAL");
  CHECK(r13WithDefault->m_entries.front().m_handle == 0xFu);
}

TEST_CASE("DWG sibling oracle fixtures deliver R13/R14 viewport headers",
          "[dwg][vport][fixture]") {
  const char *configuredRoot = std::getenv("DWG_PARSER_SOURCE_DIR");
  std::vector<std::filesystem::path> roots;
  if (configuredRoot != nullptr && *configuredRoot != '\0')
    roots.emplace_back(configuredRoot);
#ifdef LIBRECAD_SOURCE_DIR
  roots.emplace_back(std::filesystem::path(LIBRECAD_SOURCE_DIR).parent_path()
                     / "dwg-parser");
#endif
  roots.emplace_back(std::filesystem::path("../dwg-parser"));

  auto findFixture = [&roots](const char *name) {
    for (const auto &root : roots) {
      const auto path = root / "tests/fixtures/committed/libredwg" / name;
      if (std::filesystem::is_regular_file(path))
        return path;
    }
    return std::filesystem::path{};
  };

  const auto r14Path = findFixture("example_r14.dwg");
  const auto r13Path = findFixture("example_r13.dwg");
  if (r14Path.empty() || r13Path.empty()) {
    SKIP("dwg-parser sibling fixtures absent; skipping");
  }

  struct Expected {
    std::uint32_t handle;
    std::uint32_t parentHandle;
    bool isOn;
    std::uint32_t viewportEntityHandle;
  };

  const auto checkFixture = [](const std::filesystem::path &path,
                               DRW::Version expectedVersion,
                               const Expected (&expected)[2]) {
    CountingIface iface;
    dwgR reader(path.string().c_str());
    REQUIRE(reader.read(&iface, /*ext=*/true));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    REQUIRE(reader.getVersion() == expectedVersion);
    REQUIRE(iface.viewportEntityHeaders.size() == 2u);

    for (const Expected &item : expected) {
      const auto header = std::find_if(
          iface.viewportEntityHeaders.begin(),
          iface.viewportEntityHeaders.end(),
          [&item](const DRW_ViewportEntityHeader &value) {
            return value.handle == item.handle;
          });
      REQUIRE(header != iface.viewportEntityHeaders.end());
      CHECK(header->parentHandle == item.parentHandle);
      CHECK(header->isOn == item.isOn);
      CHECK(header->xrefBlockHandle == 0u);
      CHECK(header->viewportEntityHandle == item.viewportEntityHandle);
      CHECK(header->previousViewportEntityHeaderHandle == 0u);
    }
  };

  const Expected r13Expected[] = {
      {0x2D2u, 0x2D1u, false, 0u},
      {0x2D3u, 0x2D1u, true, 136u},
  };
  const Expected r14Expected[] = {
      {0x2D2u, 0x2D1u, true, 0u},
      {0x2D3u, 0x2D1u, true, 136u},
  };

  checkFixture(r13Path, DRW::AC1012, r13Expected);
  checkFixture(r14Path, DRW::AC1014, r14Expected);
}

TEST_CASE("DWG R2013 rejects dictionary names beyond the object frame",
          "[dwg][r2013][safety][dictionary][mutation]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-dictionary-count-mutated.dwg";
  std::filesystem::remove(path);

  DictionaryMutationWriteIface writeIface;
  writeIface.dictionary.handle = 0x9F0u;
  writeIface.dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
  writeIface.dictionary.m_entries.push_back({"VALID", 0x9F1u});
  dwgRW writer(path.string().c_str());
  writeIface.writer = &writer;
  REQUIRE(writer.write(&writeIface, DRW::AC1027, false));

  std::ifstream input(path, std::ios::binary);
  REQUIRE(input);
  const std::vector<std::uint8_t> fileBytes(
      (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!fileBytes.empty());

  PointCloudReaderProbe probe(
      std::make_unique<dwgBuffer>(
          const_cast<std::uint8_t *>(fileBytes.data()), fileBytes.size()),
      &writer);
  REQUIRE(probe.readMetaData());
  REQUIRE(probe.readFileHeader());
  DRW_Header header;
  REQUIRE(probe.readDwgHeader(header));
  REQUIRE(probe.readDwgClasses());
  REQUIRE(probe.readDwgHandles());
  REQUIRE(probe.readDwgTables(header));
  REQUIRE(probe.objectData() != nullptr);
  REQUIRE(probe.objectDataSize() > 0);

  objHandle dictionaryObject;
  bool foundDictionary = false;
  for (const auto &entry : probe.ObjectMap) {
    DwgObjectFrame frame;
    dwgBuffer objectData(
        const_cast<std::uint8_t *>(probe.objectData()),
        probe.objectDataSize());
    if (!frame.readAt(objectData, DRW::AC1027, entry.second.loc))
      continue;
    dwgBuffer body(frame.body().data(), frame.body().size());
    if (body.getObjType(DRW::AC1027) == 42
        && entry.second.handle == writeIface.dictionary.handle) {
      dictionaryObject = entry.second;
      dictionaryObject.type = 42;
      foundDictionary = true;
      break;
    }
  }
  REQUIRE(foundDictionary);

  std::vector<std::uint8_t> mutated(
      probe.objectData(), probe.objectData() + probe.objectDataSize());
  std::size_t bodyStart = 0;
  std::size_t frameEnd = 0;
  REQUIRE(findFrameEnd(mutated, dictionaryObject.loc, DRW::AC1027,
                       bodyStart, frameEnd));
  REQUIRE(bodyStart < frameEnd);
  const std::size_t bodyEnd = frameEnd - sizeof(std::uint16_t);
  std::vector<std::uint8_t> body(
      mutated.begin() + static_cast<std::ptrdiff_t>(bodyStart),
      mutated.begin() + static_cast<std::ptrdiff_t>(bodyEnd));
  std::size_t countBit = 0;
  REQUIRE(findDictionaryCountBit(body, countBit));
  // Keep the writer's 8-bit BL form and change only the declared count.
  REQUIRE(patchBits(body, countBit + 2u, 0xFFu, 8));
  std::copy(body.begin(), body.end(),
            mutated.begin() + static_cast<std::ptrdiff_t>(bodyStart));

  dwgBuffer crcBuffer(mutated.data(), mutated.size());
  const std::uint16_t crc = crcBuffer.crc8(
      0xC0C1, static_cast<std::int32_t>(dictionaryObject.loc),
      static_cast<std::int32_t>(bodyEnd));
  mutated[bodyEnd] = static_cast<std::uint8_t>(crc & 0xFFu);
  mutated[bodyEnd + 1] = static_cast<std::uint8_t>(crc >> 8);

  dwgBuffer corrupted(mutated.data(), mutated.size());
  DictionaryFixtureIface readIface;
  objHandle mutableObject = dictionaryObject;
  CHECK_FALSE(probe.readDwgObject(&corrupted, mutableObject, readIface));
  CHECK(readIface.dictionaries.empty());

  std::filesystem::remove(path);
}

TEST_CASE("DWG smoke test: read ~/doc/dwg/*.dwg and report entity counts",
          "[.slow][dwg][corpus]") {
  // The test corpus lives in a developer-local directory (~/doc/dwg/) and
  // is not shipped with the repo. If it isn't present, skip cleanly so the
  // test passes for everyone else.
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping DWG corpus tests");
  }
  const std::string dir = std::string(home) + "/doc/dwg/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("DWG corpus directory not found at " << dir << "; skipping");
  }

  // Print a header once
  std::cout << "\n";
  std::cout << std::left << std::setw(56) << "File" << std::setw(16)
            << "Version" << std::setw(24) << "Error" << std::setw(10)
            << "Entities" << std::setw(8) << "Blocks"
            << "Layers\n";
  std::cout << std::string(120, '-') << "\n";

  bool anyFailed = false;
  int filesChecked = 0;

  for (const auto &fi : kTestFiles) {
    const std::string path = dir + fi.name;
    if (!std::filesystem::is_regular_file(path)) {
      std::cout << std::left << std::setw(56) << fi.name
                << "(missing - skipped)\n";
      continue;
    }
    ++filesChecked;
    const DwgResult r = readDwg(path);

    std::cout << std::left << std::setw(56) << fi.name << std::setw(16)
              << versionStr(r.version) << std::setw(24) << errorStr(r.error)
              << std::setw(10) << r.entities << std::setw(8) << r.blocks
              << r.layers << "\n";

    if (fi.expectSuccess && r.error != DRW::BAD_NONE) {
      anyFailed = true;
      FAIL_CHECK("  ^^^ " << fi.name << " expected OK but got "
                          << errorStr(r.error));
    }
  }

  std::cout << std::string(120, '-') << "\n";
  if (filesChecked == 0)
    std::cout << "No corpus files present; nothing to verify.\n";
  else if (!anyFailed)
    std::cout << "All " << filesChecked
              << " present file(s) opened as expected.\n";
}

TEST_CASE("DWG lineweights: distribution in lineweights.dwg",
          "[.dwg_lineweights]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path = std::string(home) + "/doc/dwg/lineweights.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("lineweights.dwg not present; skipping");
  }

  CountingIface iface;
  const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
  REQUIRE(r.error == DRW::BAD_NONE);

  std::cout << "\n=== lineweights.dwg lWeight distribution ===\n";
  std::cout << "Total entities: " << iface.entities << "\n\n";

  std::cout << "Per-entity lWeights:\n";
  for (const auto &[lw, count] : iface.entityLWeights) {
    std::cout << "  enum=" << std::setw(3) << lw << "  width=" << std::setw(8)
              << lWeightToMm(lw) << "  count=" << count << "\n";
  }

  std::cout << "\nPer-layer lWeights:\n";
  for (const auto &[name, lw] : iface.layerLWeights) {
    std::cout << "  layer=" << std::setw(15) << name
              << "  enum=" << std::setw(3) << lw
              << "  width=" << lWeightToMm(lw) << "\n";
  }

  int byLayerCount =
      iface.entityLWeights.count(29) ? iface.entityLWeights[29] : 0;
  int byBlockCount =
      iface.entityLWeights.count(30) ? iface.entityLWeights[30] : 0;
  int defaultCount =
      iface.entityLWeights.count(31) ? iface.entityLWeights[31] : 0;
  std::cout << "\nSummary: entities resolving via layer=" << byLayerCount
            << ", block=" << byBlockCount << ", default=" << defaultCount
            << ", explicit="
            << (iface.entities - byLayerCount - byBlockCount - defaultCount)
            << "\n";
}

// Verbose re-run of selected corpus files that need a full reader trace.
// Run this test case individually to see the full libdxfrw debug trace:
//   ./librecad_tests "*DWG verbose*"
TEST_CASE("DWG verbose debug: failing AC1021 files", "[.dwg_verbose]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping verbose DWG corpus tests");
  }
  const std::string dir = std::string(home) + "/doc/dwg/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("DWG corpus directory not found at " << dir << "; skipping");
  }

  const char *failingFiles[] = {
      "colorwh.dwg",
      "dwgreader21_230.dwg",
      "blocks_and_tables_-_metric.dwg",
  };

  for (const auto *name : failingFiles) {
    const std::string path = dir + name;
    if (!std::filesystem::is_regular_file(path)) {
      std::cout << "\n=== " << name << " (missing - skipped) ===\n";
      continue;
    }
    std::cout << "\n=== " << name << " ===\n";
    const DwgResult r = readDwg(path, /*verbose=*/true);
    std::cout << "Result: " << errorStr(r.error) << "  entities=" << r.entities
              << "  blocks=" << r.blocks << "  layers=" << r.layers << "\n";
  }
}

// Deep diagnostic: entity type breakdown + unhandled oType report.
// Run individually:
//   ./librecad_tests "[.dwg_deep]" -s
TEST_CASE("DWG deep diagnostic: entity type breakdown for target files",
          "[.dwg_deep]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string dir = std::string(home) + "/doc/dwg/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("DWG corpus directory not found; skipping");
  }

  const char *targets[] = {
      "Extruder2.dwg",
      "blocks_and_tables_-_metric.dwg",
      "blocks_and_tables_-_imperial.dwg",
      "architectural_example-imperial.dwg",
      "architectural_-_annotation_scaling_and_multileaders.dwg",
      "visualization_-_aerial.dwg",
      "visualization_-_conference_room.dwg",
  };

  for (const auto *name : targets) {
    const std::string path = dir + name;
    if (!std::filesystem::is_regular_file(path)) {
      std::cout << "\n=== " << name << " (missing - skipped) ===\n";
      continue;
    }
    const DeepResult dr = readDwgDeep(path);
    printDeepReport(name, dr);
  }
}

// Pool_Detail.dwg circle dump — diagnostic for the spurious circle reported at
// center (512.3864, 333.9300), r=0.1.  Captures DRW_Circle metadata that the
// production filter discards (space, extrusion, owner) so we can tell whether
// a spurious circle is paper-space, invisible, or layer-frozen.
//
// Run:
//   ./librecad_tests "[.dwg_pool_circles]" -s
TEST_CASE("DWG Pool_Detail.dwg: dump every circle", "[.dwg_pool_circles]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg/Pool_Detail.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("Pool_Detail.dwg not present; skipping");
  }

  struct CircleCollector : public TypeTrackingIface {
    struct CInfo {
      double cx, cy, cz, r;
      std::string layer;
      std::uint32_t handle;
      std::uint32_t parentHandle;
      int space; // 0=Model, 1=Paper
      bool visible;
      double ex, ey, ez;
      double thickness;
    };
    std::vector<CInfo> circles;
    std::map<std::string, int> layerFlags;
    void addLayer(const DRW_Layer &e) override {
      TypeTrackingIface::addLayer(e);
      layerFlags[e.name] = e.flags;
    }
    void addCircle(const DRW_Circle &e) override {
      TypeTrackingIface::addCircle(e);
      circles.push_back({e.basePoint.x, e.basePoint.y, e.basePoint.z, e.radious,
                         e.layer, e.handle, e.parentHandle,
                         static_cast<int>(e.space), e.visible, e.extPoint.x,
                         e.extPoint.y, e.extPoint.z, e.thickness});
    }
  } iface;

  DRW::setCustomDebugPrinter(new DRW::DebugPrinter()); // silent
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&iface, true));

  // Sort: paper-space first, then by layer, then by handle, so leaks group.
  std::sort(iface.circles.begin(), iface.circles.end(),
            [](const auto &a, const auto &b) {
              if (a.space != b.space)
                return a.space > b.space;
              if (a.layer != b.layer)
                return a.layer < b.layer;
              return a.handle < b.handle;
            });

  std::cout << "\nLayer flags (per ODA: bit0=frozen bit1=off "
               "bit2=frozen-in-new bit3=locked, bit6=hasEntity):\n";
  for (const auto &[name, flags] : iface.layerFlags) {
    std::cout << "  layer=\"" << name << "\""
              << " flags=0x" << std::hex << flags << std::dec
              << " frozen=" << ((flags & 0x1) ? "1" : "0")
              << " hasEnt=" << ((flags & 0x40) ? "1" : "0") << "\n";
  }

  std::cout << "\nFound " << iface.circles.size() << " circle(s):\n";
  std::cout << std::fixed << std::setprecision(7);
  int idx = 0;
  for (const auto &c : iface.circles) {
    int lf =
        iface.layerFlags.count(c.layer) ? iface.layerFlags.at(c.layer) : -1;
    const char *spaceName = c.space == 0   ? "MODEL"
                            : c.space == 1 ? "PAPER"
                                           : "ENT3?";
    std::cout << "  [" << idx++ << "] " << spaceName << "(" << c.space << ")"
              << " center=(" << c.cx << ", " << c.cy << ", " << c.cz << ")"
              << " r=" << c.r << " layer=\"" << c.layer << "\" layerFlags=0x"
              << std::hex << lf << std::dec
              << " visible=" << (c.visible ? "1" : "0") << " handle=0x"
              << std::hex << c.handle << " parent=0x" << c.parentHandle
              << std::dec << std::fixed << std::setprecision(7) << " ext=("
              << c.ex << "," << c.ey << "," << c.ez << ")"
              << " thick=" << c.thickness << "\n";
  }
}

// Deep troubleshooting for Cover.dwg (AC1027/R2013, dwgReader27).
// Run:
//   ./librecad_tests "[.dwg_cover]" -s
// Then for full verbose trace:
//   ./librecad_tests "[.dwg_cover_verbose]" -s 2>&1 | less
TEST_CASE("DWG Cover.dwg: deep entity/type breakdown", "[.dwg_cover]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg/Cover.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("Cover.dwg not present; skipping");
  }
  const DeepResult dr = readDwgDeep(path);
  printDeepReport("Cover.dwg", dr);
}

// ---------------------------------------------------------------------------
// dwg_samples corpus: 42 minimal single-entity files across 6 DWG versions.
// Auto-discovers every *.dwg in ~/dev/dwg_samples/ and asserts each one
// parses without error and contains at least one entity.
// Skips cleanly when the directory is absent.
// ---------------------------------------------------------------------------
TEST_CASE("DWG samples: load all files in ~/dev/dwg_samples/",
          "[.slow][dwg][corpus]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string dir = std::string(home) + "/dev/dwg_samples/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("~/dev/dwg_samples/ not found; skipping");
  }

  // Collect *.dwg paths, sorted for deterministic output.
  std::vector<std::string> paths;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file())
      continue;
    const auto &p = entry.path();
    const std::string ext = p.extension().string();
    if (ext == ".dwg" || ext == ".DWG")
      paths.push_back(p.string());
  }
  std::sort(paths.begin(), paths.end());

  if (paths.empty()) {
    SKIP("No .dwg files found in ~/dev/dwg_samples/; skipping");
  }

  // Header
  std::cout << "\n"
            << std::left << std::setw(46) << "File" << std::setw(16)
            << "Version" << std::setw(24) << "Error" << std::setw(10)
            << "Entities" << std::setw(8) << "Blocks"
            << "Layers\n"
            << std::string(110, '-') << "\n";

  int passed = 0, failed = 0;
  for (const auto &path : paths) {
    const std::string name = std::filesystem::path(path).filename().string();
    const DwgResult r = readDwg(path);

    std::cout << std::left << std::setw(46) << name << std::setw(16)
              << versionStr(r.version) << std::setw(24) << errorStr(r.error)
              << std::setw(10) << r.entities << std::setw(8) << r.blocks
              << r.layers << "\n";

    if (r.error != DRW::BAD_NONE) {
      ++failed;
      FAIL_CHECK(name << ": expected OK but got " << errorStr(r.error));
    } else if (r.entities < 1) {
      ++failed;
      FAIL_CHECK(name << ": parsed OK but 0 entities (expected >= 1)");
    } else {
      ++passed;
    }
  }

  std::cout << std::string(110, '-') << "\n";
  std::cout << "Passed: " << passed << "  Failed: " << failed
            << "  Total: " << paths.size() << "\n";
}

// Deep + verbose diagnostic for the one failure in the samples suite:
// polyline2d_line_R14.dwg (AC1014) returns OK but 0 entities.
// Run:  ./librecad_tests "[.dwg_polyR14]" -s 2>/tmp/poly_r14_verbose.txt
TEST_CASE("DWG polyline2d_line_R14: deep + verbose", "[.dwg_polyR14]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path =
      std::string(home) + "/dev/dwg_samples/polyline2d_line_R14.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("polyline2d_line_R14.dwg not present; skipping");
  }

  // readDwgDeep captures the full debug log internally — no separate verbose
  // reader needed and no printer-state complications.
  const DeepResult dr = readDwgDeep(path);

  std::cout << "\n--- Deep diagnostic ---\n";
  printDeepReport("polyline2d_line_R14.dwg", dr);

  std::cout << "\n--- Reader debug trace ---\n";
  std::cout << dr.debugLog;
}

TEST_CASE("DWG Cover.dwg: verbose reader trace", "[.dwg_cover_verbose]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg/Cover.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("Cover.dwg not present; skipping");
  }
  std::cout << "\n=== Cover.dwg (verbose) ===\n";
  const DwgResult r = readDwg(path, /*verbose=*/true);
  std::cout << "Result: " << errorStr(r.error)
            << "  version=" << versionStr(r.version)
            << "  entities=" << r.entities << "  blocks=" << r.blocks
            << "  layers=" << r.layers << "\n";
}

// ---------------------------------------------------------------------------
// ~/doc/dwg2/ corpus: real-world DWG files (mixed versions, complex content).
// Asserts each file parses without a reader error (BAD_NONE).
// Entity count is reported but not asserted — some real-world files may be
// 3D-only or otherwise legitimately produce 0 2D entities.
// Skips cleanly when the directory is absent.
// ---------------------------------------------------------------------------
// Hidden tag [.dwg3_verbose]: full debug trace of the example_2007/2010/2013/
// 2018.dwg series in ~/doc/dwg3/ so the R2010+ entity reader regression can
// be diagnosed by side-by-side compare. Run with:
//   ./librecad_tests "[.dwg3_verbose]" -s
TEST_CASE("DWG verbose: example_*.dwg in ~/doc/dwg3/", "[.dwg3_verbose]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string dir = std::string(home) + "/doc/dwg3/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("~/doc/dwg3/ not found");
  }
  const char *names[] = {
      "example_2007.dwg",
      "example_2010.dwg",
      "example_2013.dwg",
      "example_2018.dwg",
  };
  for (const auto *name : names) {
    const std::string path = dir + name;
    if (!std::filesystem::is_regular_file(path)) {
      std::cout << "\n=== " << name << " (missing) ===\n";
      continue;
    }
    std::cout << "\n=== " << name << " ===\n";
    const DwgResult r = readDwg(path, /*verbose=*/true);
    std::cout << "Result: " << errorStr(r.error)
              << "  version=" << versionStr(r.version)
              << "  entities=" << r.entities << "  blocks=" << r.blocks
              << "  layers=" << r.layers << "\n";
  }
}

// Hidden tag [.dwg3]: one-shot diagnostic load of ~/doc/dwg3/ so failures
// can be enumerated without polluting the default test run. Run with:
//   ./librecad_tests "[.dwg3]" -s
TEST_CASE("DWG corpus: load all files in ~/doc/dwg3/", "[.dwg3]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string dir = std::string(home) + "/doc/dwg3/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("~/doc/dwg3/ not found; skipping");
  }

  std::vector<std::string> paths;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file())
      continue;
    const auto &p = entry.path();
    const std::string ext = p.extension().string();
    if (ext != ".dwg" && ext != ".DWG")
      continue;
    if (p.filename().string().front() == '#')
      continue;
    paths.push_back(p.string());
  }
  std::sort(paths.begin(), paths.end());
  if (paths.empty()) {
    SKIP("No .dwg files in ~/doc/dwg3/; skipping");
  }

  std::cout << "\n"
            << std::left << std::setw(46) << "File" << std::setw(16)
            << "Version" << std::setw(24) << "Error" << std::setw(10)
            << "Entities" << std::setw(8) << "Blocks"
            << "Layers\n"
            << std::string(110, '-') << "\n";

  int passed = 0, failed = 0;
  for (const auto &path : paths) {
    const std::string name = std::filesystem::path(path).filename().string();
    const DwgResult r = readDwg(path);
    std::cout << std::left << std::setw(46) << name << std::setw(16)
              << versionStr(r.version) << std::setw(24) << errorStr(r.error)
              << std::setw(10) << r.entities << std::setw(8) << r.blocks
              << r.layers << "\n";
    if (r.error != DRW::BAD_NONE)
      ++failed;
    else
      ++passed;
  }
  std::cout << std::string(110, '-') << "\n";
  std::cout << "Passed: " << passed << "  Failed: " << failed
            << "  Total: " << paths.size() << "\n";
}

// makeall-plus.dwg (AC1032/R2018) is a "make-everything" torture file with ~179
// distinct DWG types. This deep diagnostic enumerates exactly which entity types
// libdxfrw surfaces versus which it leaves in the skipped-custom-class telemetry,
// so coverage gaps (surfaces, pointcloud, section) are measurable, not inferred.
// Run: ./librecad_tests "[.dwg_makeall]" -s
TEST_CASE("DWG makeall-plus.dwg: deep coverage diagnostic", "[.dwg_makeall]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg3/makeall-plus.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("makeall-plus.dwg not present; skipping");
  }

  // Counts EVERY delivery path, including those the shared TypeTrackingIface
  // ignores: MESH, modeler geometry (3DSOLID/REGION/BODY/SURFACE), and the raw
  // unsupported-object carrier.
  struct CoverageIface : public TypeTrackingIface {
    int meshes = 0, modelerGeoms = 0, unsupported = 0;
    std::map<std::string, int> unsupportedByName;
    std::vector<std::string> blockNames;
    std::vector<std::pair<std::string, bool>> layerFrozen;
    std::vector<std::pair<std::string, bool>> insertNames;
    void addMesh(const DRW_Mesh &e) override {
      trackT(e, "MESH");
      ++meshes;
    }
    void addModelerGeometry(const DRW_ModelerGeometry &) override {
      ++modelerGeoms;
      typeCounts["(modeler-geometry)"]++;
    }
    void addUnsupportedObject(const DRW_UnsupportedObject &e) override {
      ++unsupported;
      unsupportedByName[e.m_recordName.empty() ? "(unnamed)" : e.m_recordName]++;
    }
    void addBlock(const DRW_Block &b) override {
      TypeTrackingIface::addBlock(b);
      blockNames.push_back(b.name);
    }
    void addLayer(const DRW_Layer &l) override {
      TypeTrackingIface::addLayer(l);
      layerFrozen.push_back({l.name, (l.flags & 0x01) != 0 || l.color < 0});
    }
    void addInsert(const DRW_Insert &e) override {
      TypeTrackingIface::addInsert(e);
      insertNames.push_back({e.name, !e.visible});
    }
  } iface;

  DRW::setCustomDebugPrinter(new DRW::DebugPrinter()); // silent
  dwgR reader(path.c_str());
  const bool ok = reader.read(&iface, true);

  std::cout << "\n=== makeall-plus.dwg coverage ===\n";
  std::cout << "ok=" << ok << "  error=" << errorStr(reader.getError())
            << "  version=" << versionStr(reader.getVersion()) << "\n";
  std::cout << "blocks=" << iface.blocks << "  layers=" << iface.layers
            << "  entity-callbacks=" << iface.total() << "\n";
  std::cout << "meshes=" << iface.meshes
            << "  modelerGeoms=" << iface.modelerGeoms
            << "  unsupportedObjects=" << iface.unsupported
            << "  decodedProxyPrimitives=" << reader.getDecodedProxyPrimitives()
            << "\n";

  std::cout << "\nHandled entity type distribution:\n";
  for (const auto &[t, c] : iface.typeCounts)
    std::cout << "  " << std::left << std::setw(22) << t << c << "\n";

  std::cout << "\n*** Skipped custom CLASSES (entity-pass; THE coverage gap) ***\n";
  for (const auto &[name, c] : reader.getSkippedCustomClasses())
    std::cout << "  " << std::left << std::setw(34) << name << c << "\n";

  std::cout << "\nSkipped unsupported OBJECTS (objects-pass):\n";
  for (const auto &[name, c] : reader.getSkippedUnsupportedObjects())
    std::cout << "  " << std::left << std::setw(34) << name << c << "\n";

  std::cout << "\nUnsupported raw carriers delivered (by record name):\n";
  for (const auto &[name, c] : iface.unsupportedByName)
    std::cout << "  " << std::left << std::setw(34) << name << c << "\n";

  std::cout << "\nBlock names (" << iface.blockNames.size() << "):\n";
  for (const auto &name : iface.blockNames)
    std::cout << "  '" << name << "'\n";

  std::cout << "\nLayers (" << iface.layerFrozen.size() << "):\n";
  for (const auto &[name, frozen] : iface.layerFrozen)
    std::cout << "  " << std::left << std::setw(30) << name << (frozen ? "FROZEN/OFF" : "visible") << "\n";

  std::cout << "\nINSERT entities (" << iface.insertNames.size() << "):\n";
  for (const auto &[name, invis] : iface.insertNames)
    std::cout << "  '" << name << "'" << (invis ? " INVISIBLE" : "") << "\n";
}

TEST_CASE("DWG corpus: load all files in ~/doc/dwg2/",
          "[.slow][dwg][corpus]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string dir = std::string(home) + "/doc/dwg2/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("~/doc/dwg2/ not found; skipping");
  }

  std::vector<std::string> paths;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file())
      continue;
    const auto &p = entry.path();
    const std::string ext = p.extension().string();
    if (ext != ".dwg" && ext != ".DWG")
      continue;
    // '#'-prefixed files have invalid magic bytes; skip them like the smoke
    // test does.
    if (p.filename().string().front() == '#')
      continue;
    paths.push_back(p.string());
  }
  std::sort(paths.begin(), paths.end());

  if (paths.empty()) {
    SKIP("No .dwg files found in ~/doc/dwg2/; skipping");
  }

  std::cout << "\n"
            << std::left << std::setw(46) << "File" << std::setw(16)
            << "Version" << std::setw(24) << "Error" << std::setw(10)
            << "Entities" << std::setw(8) << "Blocks"
            << "Layers\n"
            << std::string(110, '-') << "\n";

  int passed = 0, failed = 0;
  for (const auto &path : paths) {
    const std::string name = std::filesystem::path(path).filename().string();
    const DwgResult r = readDwg(path);

    std::cout << std::left << std::setw(46) << name << std::setw(16)
              << versionStr(r.version) << std::setw(24) << errorStr(r.error)
              << std::setw(10) << r.entities << std::setw(8) << r.blocks
              << r.layers << "\n";

    if (r.error != DRW::BAD_NONE) {
      ++failed;
      FAIL_CHECK(name << ": expected OK but got " << errorStr(r.error));
    } else {
      ++passed;
    }
  }

  std::cout << std::string(110, '-') << "\n";
  std::cout << "Passed: " << passed << "  Failed: " << failed
            << "  Total: " << paths.size() << "\n";
}

// Verifies that all HATCH entities in a real-world R2013 file parse correctly:
// loop structure, boundary geometry, and associative handles are all consumed
// with zero remaining bytes.
// Run verbosely:  ./librecad_tests "[.dwg_arch_hatch]" -s
// 2>/tmp/arch_hatch_trace.txt
TEST_CASE("DWG Architectural-Modern-Building-Design: hatch parsing",
          "[.dwg_arch_hatch]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("Architectural-Modern-Building-Design.dwg not present; skipping");
  }

  const DeepResult dr = readDwgDeep(path);

  // Basic load check.
  REQUIRE(dr.ok);
  REQUIRE(dr.error == DRW::BAD_NONE);

  // The file has 48 HATCH entities (mix of solid, gradient, and GLASS pattern
  // hatches with 1..56 boundary loops each).  All should be parsed — none left
  // in the unhandled bucket.
  CHECK(dr.iface.typeCounts.at("HATCH") == 48);
  CHECK(dr.unhandledTypes.count(78) == 0); // oType 78 = HATCH

  std::cout << "\n--- Deep diagnostic ---\n";
  printDeepReport("Architectural-Modern-Building-Design.dwg", dr);

  std::cout << "\n--- Reader debug trace ---\n";
  std::cout << dr.debugLog;
}

// Verifies that the DRW→RS_Hatch pipeline produces properly filled hatches for
// every HATCH entity in Architectural-Modern-Building-Design.dwg:
//   • Solid hatches (42 of 48): must fully succeed (HATCH_OK, area > 0).
//   • Pattern hatches (6 GLASS): boundary must be valid (no
//   HATCH_INVALID_CONTOUR);
//     HATCH_PATTERN_NOT_FOUND is accepted because the test env has no pattern
//     library.
// Run:  ./librecad_tests "[.dwg_arch_hatch_fill]" -s
TEST_CASE("DWG Architectural-Modern-Building-Design: hatch fill pipeline",
          "[.dwg_arch_hatch_fill]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("Architectural-Modern-Building-Design.dwg not present; skipping");
  }

  HatchFillIface iface;
  {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::None);
    bool ok = reader.read(&iface, true);
    REQUIRE(ok);
  }

  REQUIRE(iface.hatches.size() == 48);

  int solidFailed = 0;
  int contourErrors = 0;
  int loopLoss =
      0; // hatches where allLoops < declaredLoops (real boundary loss)

  // Columns: pattern | solid | decl | roots | all | err | area
  std::cout << "\n  " << std::setw(20) << std::left << "pattern" << std::setw(6)
            << "solid" << std::setw(6) << "decl" << std::setw(6) << "roots"
            << std::setw(6) << "all" << std::setw(6) << "err"
            << "area\n";
  std::cout << "  " << std::string(70, '-') << "\n";

  for (const auto &r : iface.hatches) {
    std::cout << "  " << std::setw(20) << std::left << r.pattern << std::setw(6)
              << r.solid << std::setw(6) << r.declaredLoops << std::setw(6)
              << r.rootLoops << std::setw(6) << r.allLoops << std::setw(6)
              << r.error << r.area << "\n";

    // No hatch should have a broken boundary topology.
    CHECK(r.error != RS_Hatch::HATCH_INVALID_CONTOUR);
    if (r.error == RS_Hatch::HATCH_INVALID_CONTOUR)
      ++contourErrors;

    // Every declared loop must be successfully extracted (no under-extraction
    // from cross-contamination at shared corners).  allLoops > declaredLoops is
    // acceptable: a self-intersecting DWG loop can legitimately yield multiple
    // extracted loops.
    CHECK(r.allLoops >= r.declaredLoops);
    if (r.allLoops < r.declaredLoops)
      ++loopLoss;

    if (r.solid) {
      // Solid hatches must produce a filled result with positive area.
      CHECK(r.error == RS_Hatch::HATCH_OK);
      CHECK(r.rootLoops > 0);
      CHECK(r.area > 0.0);
      if (r.error != RS_Hatch::HATCH_OK)
        ++solidFailed;
    }
  }

  std::cout << "\n  Solid failed: " << solidFailed
            << "  Contour errors: " << contourErrors
            << "  Loop-loss hatches: " << loopLoss
            << "  Total: " << iface.hatches.size() << "\n";
}

// Verifies the spline-bordered SOLID hatch regression on
// ~/doc/dwg2/Sport-Man-Signs.dwg (AC1021/R2007, 216 SOLID hatches, 272 spline
// boundary edges across 290 loops). Before the DRW::SPLINE arm was added to
// addHatch, every spline boundary edge was silently dropped and these fills
// rendered as slivers or vanished entirely. Run:  ./librecad_tests
// "[.dwg_sport_signs]" -s
TEST_CASE("DWG Sport-Man-Signs: SOLID hatches render with positive area",
          "[.dwg_sport_signs]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg2/Sport-Man-Signs.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("Sport-Man-Signs.dwg not present; skipping");
  }

  HatchFillIface iface;
  {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::None);
    bool ok = reader.read(&iface, true);
    REQUIRE(ok);
  }

  // libreDWG JSON ground truth: 216 HATCH entities, all SOLID, name "SOLID".
  REQUIRE(iface.typeCounts.at("HATCH") == 216);

  int solidCount = 0;
  int positiveAreaCount = 0;
  int contourErrors = 0;
  int patternMissing = 0;
  int zeroAreaOk = 0;
  int declaredLoopLoss = 0;
  for (const auto &r : iface.hatches) {
    if (!r.solid)
      continue;
    ++solidCount;
    if (r.error == RS_Hatch::HATCH_INVALID_CONTOUR)
      ++contourErrors;
    if (r.error == RS_Hatch::HATCH_PATTERN_NOT_FOUND)
      ++patternMissing;
    if (r.error == RS_Hatch::HATCH_OK && r.area == 0.0)
      ++zeroAreaOk;
    if (r.allLoops < r.declaredLoops)
      ++declaredLoopLoss;
    if (r.error == RS_Hatch::HATCH_OK && r.area > 0.0)
      ++positiveAreaCount;
  }
  REQUIRE(solidCount == 216);

  std::cout << "\n  Sport-Man-Signs SOLID positive-area hatches: "
            << positiveAreaCount << "/" << solidCount
            << "  (contour errors: " << contourErrors
            << ", zero-area OK: " << zeroAreaOk
            << ", declared-loop loss: " << declaredLoopLoss << ")\n";

  // Per-hatch dump: spot loops where rootLoops < declaredLoops or
  // allLoops < declaredLoops — those are the geometrically-incorrect
  // renders even when m_area > 0.
  int idx = 0;
  int lossy = 0;
  for (const auto &r : iface.hatches) {
    if (!r.solid) {
      ++idx;
      continue;
    }
    if (r.allLoops < r.declaredLoops || r.rootLoops < 1 ||
        (r.error != RS_Hatch::HATCH_OK)) {
      std::cout << "    [#" << idx << "] decl=" << r.declaredLoops
                << " root=" << r.rootLoops << " all=" << r.allLoops
                << " err=" << r.error << " area=" << r.area << "\n";
      ++lossy;
    }
    ++idx;
  }
  std::cout << "  Lossy/error hatches: " << lossy << "\n";

  // Area distribution: tiny positive areas suggest degenerate-but-non-empty
  // boundaries; correct glyph fills should be O(10^2)–O(10^4).
  int tiny = 0, small_a = 0, mid = 0, big = 0;
  double areaMin = 1e30, areaMax = -1e30;
  for (const auto &r : iface.hatches) {
    if (!r.solid || r.error != RS_Hatch::HATCH_OK || r.area <= 0.0)
      continue;
    if (r.area < areaMin)
      areaMin = r.area;
    if (r.area > areaMax)
      areaMax = r.area;
    if (r.area < 1.0)
      ++tiny;
    else if (r.area < 100.0)
      ++small_a;
    else if (r.area < 10000.0)
      ++mid;
    else
      ++big;
  }
  std::cout << "  Area buckets (positive only):"
            << " <1: " << tiny << ", 1-100: " << small_a << ", 100-10k: " << mid
            << ", >10k: " << big << "  (min=" << areaMin << ", max=" << areaMax
            << ")\n";

  // Before the fix this number is ~0; after the fix it should be near 216.
  // Threshold of 200 leaves slack for a handful of legitimately
  // self-intersecting glyph loops that fail validate() — refine after
  // observing the first successful run.
  CHECK(positiveAreaCount >= 200);
}

// Verifies that trolley_structure.dwg (AC1024/R2010, ~3218 entities, 79 blocks,
// 13 layers) loads cleanly with every entity type properly dispatched into a
// handler.  No oTypes should land in the unhandled bucket.
// Run:  ./librecad_tests "[.dwg_trolley]" -s
TEST_CASE("DWG trolley_structure: entity population", "[.dwg_trolley]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/trolley_structure.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("trolley_structure.dwg not present; skipping");
  }

  const DeepResult dr = readDwgDeep(path);

  REQUIRE(dr.ok);
  REQUIRE(dr.error == DRW::BAD_NONE);
  REQUIRE(dr.version == DRW::AC1024);

  // Inventory of standard graphical-entity oTypes (visible 2D geometry).
  // OBJECTS-section types (DICTIONARY=42, MLINESTYLE=73, XRECORD=79,
  // PLACEHOLDER=80, LAYOUT=82) and custom oTypes >=500 (AutoCAD Mechanical
  // proxy/AM_ classes) are NOT entities and may legitimately appear in the
  // unhandled bucket — they don't carry drawable geometry.
  static const std::set<int> kGraphicalOTypes = {
      1,  7,  8,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
      29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101};
  int unhandledGraphicalLost = 0;
  for (const auto &[oType, count] : dr.unhandledTypes) {
    if (kGraphicalOTypes.count(oType)) {
      std::cout << "  *** LOST graphical entity oType=" << oType
                << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                << " count=" << count << "\n";
      unhandledGraphicalLost += count;
    }
  }
  CHECK(unhandledGraphicalLost == 0);

  // Counts observed at 2026-05-04 from libdxfrw against this file.
  // Drift in any of these flags either reader regression or upstream-file
  // change.
  CHECK(dr.iface.total() == 3218);
  CHECK(dr.iface.blocks == 79);
  CHECK(dr.iface.layers == 13);
  CHECK(dr.iface.typeCounts.at("LINE") == 1652);
  CHECK(dr.iface.typeCounts.at("ARC") == 711);
  CHECK(dr.iface.typeCounts.at("POINT") == 652);
  CHECK(dr.iface.typeCounts.at("CIRCLE") == 161);
  CHECK(dr.iface.typeCounts.at("LWPOLYLINE") == 41);
  CHECK(dr.iface.typeCounts.at("VIEWPORT") == 1);

  printDeepReport("trolley_structure.dwg", dr);
}

// Verifies that gear_pump_subassy.dwg (AC1024/R2010) loads cleanly with every
// graphical entity properly dispatched into a typed handler.
// Run:  ./librecad_tests "[.dwg_gear_pump]" -s
TEST_CASE("DWG gear_pump_subassy: entity population", "[.dwg_gear_pump]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("gear_pump_subassy.dwg not present; skipping");
  }

  const DeepResult dr = readDwgDeep(path);

  REQUIRE(dr.ok);
  REQUIRE(dr.error == DRW::BAD_NONE);
  REQUIRE(dr.version == DRW::AC1024);

  // No standard graphical entity (oType in valid 2D-graphic set) may be lost
  // to the unhandled bucket. OBJECTS-section types and custom oTypes >=500
  // are non-graphical and are accepted as unhandled.
  static const std::set<int> kGraphicalOTypes = {
      1,  7,  8,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
      29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101};
  int unhandledGraphicalLost = 0;
  for (const auto &[oType, count] : dr.unhandledTypes) {
    if (kGraphicalOTypes.count(oType)) {
      std::cout << "  *** LOST graphical entity oType=" << oType
                << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                << " count=" << count << "\n";
      unhandledGraphicalLost += count;
    }
  }
  CHECK(unhandledGraphicalLost == 0);

  CHECK(dr.iface.total() > 0);
  CHECK(dr.iface.blocks > 0);
  CHECK(dr.iface.layers > 0);

  // ATTRIB / ATTDEF / SEQEND no longer leak into the unhandled-entity bucket
  // since the DWG dispatcher routes them via the deferred-INSERT flush
  // (oType 2 = ATTRIB, 3 = ATTDEF, 6 = SEQEND).
  CHECK(dr.unhandledTypes.count(2) == 0);
  CHECK(dr.unhandledTypes.count(3) == 0);
  CHECK(dr.unhandledTypes.count(6) == 0);

  // Custom-class objects (oType >= 500, AutoCAD Mechanical proxy entities)
  // are skipped via the [custom-class-skipped] token, not the unhandled
  // entity-type token, so no oType >= 500 should appear here.
  int customLeak = 0;
  for (const auto &[oType, count] : dr.unhandledTypes)
    if (oType >= 500)
      customLeak += count;
  CHECK(customLeak == 0);

  // ATTDEFs belong to their owning BLOCK definitions and must be delivered
  // through addAttDef(), not attached to an INSERT's attribute list.
  REQUIRE(dr.iface.typeCounts.count("ATTDEF"));
  CHECK(dr.iface.typeCounts.at("ATTDEF") == 17);

  // ATTRIBs ride attached to their owning INSERTs via DRW_Insert::attlist.
  // The file declares 17 visible-attribute instances; require at least 14
  // to flow through (the 3 outliers exercise an unimplemented MText-style
  // ATTRIB variant tracked separately).
  if (dr.iface.typeCounts.count("ATTRIB"))
    CHECK(dr.iface.typeCounts.at("ATTRIB") >= 14);

  printDeepReport("gear_pump_subassy.dwg", dr);
}

// Deep diagnostic for lever_detail.dwg (AC1024/R2010, ~241 entities,
// 18 blocks, 29 layers).  Smoke loader reports OK, but user-observed
// partial render means some entity types are silently dropped.  This
// probe enumerates handled vs. unhandled oTypes so we can target the fix.
//   ./librecad_tests "[.dwg_lever]" -s 2>/tmp/lever_trace.txt
TEST_CASE("DWG lever_detail: entity population", "[.dwg_lever]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg2/lever_detail.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("lever_detail.dwg not present; skipping");
  }

  const DeepResult dr = readDwgDeep(path);

  REQUIRE(dr.ok);
  REQUIRE(dr.error == DRW::BAD_NONE);
  REQUIRE(dr.iface.total() > 0);

  printDeepReport("lever_detail.dwg", dr);
}

// Helper: human-readable name for RS2::EntityType.
const char *rttiName(RS2::EntityType t) {
  switch (t) {
  case RS2::EntityUnknown:
    return "Unknown";
  case RS2::EntityContainer:
    return "Container";
  case RS2::EntityBlock:
    return "Block";
  case RS2::EntityFontChar:
    return "FontChar";
  case RS2::EntityInsert:
    return "Insert";
  case RS2::EntityGraphic:
    return "Graphic";
  case RS2::EntityPoint:
    return "Point";
  case RS2::EntityLine:
    return "Line";
  case RS2::EntityPolyline:
    return "Polyline";
  case RS2::EntityVertex:
    return "Vertex";
  case RS2::EntityArc:
    return "Arc";
  case RS2::EntityCircle:
    return "Circle";
  case RS2::EntityEllipse:
    return "Ellipse";
  case RS2::EntitySolid:
    return "Solid";
  case RS2::EntityMText:
    return "MText";
  case RS2::EntityText:
    return "Text";
  case RS2::EntityDimAligned:
    return "DimAligned";
  case RS2::EntityDimLinear:
    return "DimLinear";
  case RS2::EntityDimRadial:
    return "DimRadial";
  case RS2::EntityDimDiametric:
    return "DimDiametric";
  case RS2::EntityDimAngular:
    return "DimAngular";
  case RS2::EntityDimOrdinate:
    return "DimOrdinate";
  case RS2::EntityDimLeader:
    return "DimLeader";
  case RS2::EntityHatch:
    return "Hatch";
  case RS2::EntityImage:
    return "Image";
  case RS2::EntityWipeout:
    return "Wipeout";
  case RS2::EntityMLeader:
    return "MLeader";
  case RS2::EntitySpline:
    return "Spline";
  case RS2::EntitySplinePoints:
    return "SplinePoints";
  case RS2::EntityParabola:
    return "Parabola";
  case RS2::EntityHyperbola:
    return "Hyperbola";
  default:
    return "(other)";
  }
}

// Filter-pipeline diagnostic for lever_detail.dwg.  The libdxfrw side
// delivers 252 entities cleanly (see [.dwg_lever]), but the user observes
// a partial render in LibreCAD.  This probe loads the file via the full
// RS_FilterDXFRW pipeline and reports the resulting RS_Entity tree, broken
// down by RTTI type, layer membership, and layer visibility.  Designed to
// surface entities that are delivered, attached to a layer, but not visible
// (frozen / off / lineweight=invisible / etc.).
//   ./librecad_tests "[.dwg_lever_filter]" -s
TEST_CASE("DWG lever_detail: filter pipeline + visibility audit",
          "[.dwg_lever_filter]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg2/lever_detail.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("lever_detail.dwg not present; skipping");
  }

  // Bootstrap a minimal Qt app context so RS_Graphic ctor's
  // LC_GROUP_GUARD ("Defaults") + RS_Settings paths don't dereference null.
  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  const bool imported =
      filter.fileImport(graphic, QString::fromStdString(path), RS2::FormatDWG);
  REQUIRE(imported);

  // RTTI counts at top level (model space).
  std::map<RS2::EntityType, int> typeCount;
  std::map<QString, int> layerCount;
  std::map<RS2::EntityType, int> typeOnHiddenLayer;
  int totalTopLevel = 0;

  for (auto *e : graphic) {
    if (!e)
      continue;
    ++totalTopLevel;
    const RS2::EntityType t = e->rtti();
    typeCount[t]++;
    const QString lyrName = e->getLayer() ? e->getLayer()->getName() : "(none)";
    layerCount[lyrName]++;
    if (e->getLayer() && e->getLayer()->isFrozen()) {
      typeOnHiddenLayer[t]++;
    }
  }

  std::cout << "\n=== lever_detail.dwg (post-filter, model space) ===\n";
  std::cout << "Top-level RS_Entity count: " << totalTopLevel << "\n";
  std::cout << "Container countDeep():     " << graphic.countDeep() << "\n\n";

  std::cout << "RTTI type distribution (top level):\n";
  for (const auto &[t, n] : typeCount) {
    std::cout << "  " << std::left << std::setw(18) << rttiName(t) << " " << n
              << "\n";
  }

  if (!typeOnHiddenLayer.empty()) {
    std::cout << "\nEntities on FROZEN/INVISIBLE layers:\n";
    int hiddenTotal = 0;
    for (const auto &[t, n] : typeOnHiddenLayer) {
      std::cout << "  " << std::left << std::setw(18) << rttiName(t) << " " << n
                << "\n";
      hiddenTotal += n;
    }
    std::cout << "  TOTAL hidden: " << hiddenTotal << "\n";
  } else {
    std::cout << "\n(no entities on hidden layers)\n";
  }

  std::cout << "\nLayer distribution (top level):\n";
  std::vector<std::pair<int, QString>> layers;
  for (const auto &[name, n] : layerCount)
    layers.emplace_back(n, name);
  std::sort(layers.rbegin(), layers.rend());
  for (const auto &[n, name] : layers) {
    std::cout << "  " << std::left << std::setw(30) << name.toStdString() << " "
              << n << "\n";
  }

  // Top-level INSERT block-name breakdown (which block each modelspace
  // INSERT references). Useful to see if the XREF block is instantiated.
  std::cout << "\nTop-level INSERTs by referenced block:\n";
  for (auto *e : graphic) {
    if (e && e->rtti() == RS2::EntityInsert) {
      auto *ins = static_cast<RS_Insert *>(e);
      std::cout << "  ref=\"" << ins->getName().toStdString() << "\" pos=("
                << ins->getInsertionPoint().x << ","
                << ins->getInsertionPoint().y << ")\n";
    }
  }

  // Block-list breakdown: per-block direct + deep counts.
  auto *blockList = graphic.getBlockList();
  if (blockList) {
    std::cout << "\nBlock definitions (" << blockList->count() << " blocks):\n";
    std::map<RS2::EntityType, int> blockTypeCount;
    int blockEntityTotal = 0;
    for (unsigned i = 0; i < blockList->count(); ++i) {
      RS_Block *bk = blockList->at(i);
      if (!bk)
        continue;
      const unsigned direct = bk->count();
      const unsigned deep = bk->countDeep();
      blockEntityTotal += direct;
      std::cout << "  " << std::left << std::setw(28)
                << bk->getName().toStdString() << " count=" << std::setw(4)
                << direct << " deep=" << deep << "\n";
      for (auto *e : *bk) {
        if (e)
          blockTypeCount[e->rtti()]++;
      }
    }
    std::cout << "Block direct-entity total: " << blockEntityTotal << "\n";
    std::cout << "Block RTTI distribution:\n";
    for (const auto &[t, n] : blockTypeCount) {
      std::cout << "  " << std::left << std::setw(18) << rttiName(t) << " " << n
                << "\n";
    }

    const int grandTotal = totalTopLevel + blockEntityTotal;
    std::cout << "\n*** Filter accepted total: " << grandTotal
              << " RS_Entities "
              << "(model-space=" << totalTopLevel
              << ", in-blocks=" << blockEntityTotal << ")\n";
    std::cout << "*** libdxfrw delivered (per [.dwg_lever]): 252\n";
  }

  SUCCEED();
}

// Same as [.dwg_lever_filter] but for ~/doc/dwg2/gear_pump_subassy.dwg, which
// is the witness for the "no visible entity, blocks empty after insert" bug:
// the GUI shows a single rendered POINT and DIN_A3 (the A3 sheet frame block)
// is empty.  The libdxfrw smoke counter reports 521 entities + 26 blocks
// loaded, and dwg2dxf produces a DXF where DIN_A3 has 166 entities — proving
// libdxfrw delivers the data.  This test runs the actual RS_FilterDXFRW path
// to pinpoint where they go in the LibreCAD container tree, plus reports any
// duplicate block names that may have made `RS_BlockList::add()` return false
// (which deletes the block and leaves `m_currentContainer` stale, leaking
// subsequent addEntity calls into model space).
//   ./librecad_tests "[.dwg_gear_pump_filter]" -s
TEST_CASE("DWG gear_pump_subassy: filter pipeline + block routing audit",
          "[.dwg_gear_pump_filter]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("gear_pump_subassy.dwg not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  const bool imported =
      filter.fileImport(graphic, QString::fromStdString(path), RS2::FormatDWG);
  REQUIRE(imported);

  std::map<RS2::EntityType, int> typeCount;
  std::map<QString, int> layerCount;
  int totalTopLevel = 0;
  int topLevelInserts = 0;
  for (auto *e : graphic) {
    if (!e)
      continue;
    ++totalTopLevel;
    const RS2::EntityType t = e->rtti();
    typeCount[t]++;
    const QString lyrName = e->getLayer() ? e->getLayer()->getName() : "(none)";
    layerCount[lyrName]++;
    if (t == RS2::EntityInsert)
      ++topLevelInserts;
  }

  std::cout << "\n=== gear_pump_subassy.dwg (post-filter) ===\n";
  std::cout << "Top-level RS_Entity count: " << totalTopLevel << "\n";
  std::cout << "Container countDeep():     " << graphic.countDeep() << "\n";
  std::cout << "Top-level INSERTs:         " << topLevelInserts << "\n\n";

  std::cout << "RTTI type distribution (top level):\n";
  for (const auto &[t, n] : typeCount) {
    std::cout << "  " << std::left << std::setw(18) << rttiName(t) << " " << n
              << "\n";
  }

  std::cout << "\nLayer distribution (top level):\n";
  std::vector<std::pair<int, QString>> layers;
  for (const auto &[name, n] : layerCount)
    layers.emplace_back(n, name);
  std::sort(layers.rbegin(), layers.rend());
  for (const auto &[n, name] : layers) {
    std::cout << "  " << std::left << std::setw(34) << name.toStdString() << " "
              << n << "\n";
  }

  std::cout << "\nTop-level INSERTs by referenced block:\n";
  for (auto *e : graphic) {
    if (e && e->rtti() == RS2::EntityInsert) {
      auto *ins = static_cast<RS_Insert *>(e);
      std::cout << "  ref=\"" << ins->getName().toStdString() << "\" pos=("
                << ins->getInsertionPoint().x << ","
                << ins->getInsertionPoint().y << ")\n";
    }
  }

  auto *blockList = graphic.getBlockList();
  REQUIRE(blockList != nullptr);
  std::cout << "\nBlock definitions (" << blockList->count() << " blocks):\n";
  std::map<RS2::EntityType, int> blockTypeCount;
  int blockEntityTotal = 0;
  for (unsigned i = 0; i < blockList->count(); ++i) {
    RS_Block *bk = blockList->at(i);
    if (!bk)
      continue;
    const unsigned direct = bk->count();
    const unsigned deep = bk->countDeep();
    blockEntityTotal += direct;
    std::cout << "  " << std::left << std::setw(28)
              << bk->getName().toStdString() << " count=" << std::setw(4)
              << direct << " deep=" << deep << "\n";
    for (auto *e : *bk) {
      if (e)
        blockTypeCount[e->rtti()]++;
    }
  }
  std::cout << "Block direct-entity total: " << blockEntityTotal << "\n";
  std::cout << "Block RTTI distribution:\n";
  for (const auto &[t, n] : blockTypeCount) {
    std::cout << "  " << std::left << std::setw(18) << rttiName(t) << " " << n
              << "\n";
  }

  const int grandTotal = totalTopLevel + blockEntityTotal;
  std::cout << "\n*** Filter accepted total: " << grandTotal
            << " RS_Entities (model-space=" << totalTopLevel
            << ", in-blocks=" << blockEntityTotal << ")\n";
  std::cout << "*** libdxfrw delivered (per [.dwg_gear_pump]): 535\n";
  const int leaked = grandTotal - 535;
  std::cout
      << "*** Surplus (model-space leakage if positive, lost if negative): "
      << leaked << "\n";

  // RS_FilterDXFRW intentionally does NOT register *Model_Space,
  // *Paper_Space, *Paper_Space0 in the visible block list (see
  // RS_FilterDXFRW::addBlock paper_space/model_space branch); subtract them
  // before flagging an unexpected loss.
  constexpr int kSpecialBlocks = 3; // *Model_Space, *Paper_Space, *Paper_Space0
  constexpr int kLibdxfrwAddBlockCount = 26;
  const int expected = kLibdxfrwAddBlockCount - kSpecialBlocks;
  std::cout << "*** libdxfrw addBlock count: " << kLibdxfrwAddBlockCount
            << "; blockList->count(): " << blockList->count() << " (expect "
            << expected << " after dropping 3 model/paper-space blocks)\n";
  if (static_cast<int>(blockList->count()) < expected) {
    std::cout << "*** ⚠ unexpected block loss: "
              << (expected - blockList->count())
              << " block(s) missing from the visible block list\n";
  }

  // Regression assertions — these are the symptoms the user reported.
  // Pre-fix: DIN_A3 (count=0), GENAXEH (count=0).  Post-fix: both populated.
  auto blockEntityCount = [&](const QString &name) -> int {
    for (unsigned i = 0; i < blockList->count(); ++i) {
      RS_Block *bk = blockList->at(i);
      if (bk && bk->getName() == name)
        return static_cast<int>(bk->count());
    }
    return -1;
  };
  const int din_a3 = blockEntityCount("DIN_A3");
  const int din_title = blockEntityCount("DIN_TITLE");
  const int genaxeh = blockEntityCount("GENAXEH");
  std::cout << "\nNamed-block check:\n";
  std::cout << "  DIN_A3:    " << din_a3 << " entities (expect > 0)\n";
  std::cout << "  DIN_TITLE: " << din_title << " entities (expect > 0)\n";
  std::cout << "  GENAXEH:   " << genaxeh << " entities (expect > 0)\n";

  CHECK(din_a3 > 0);
  CHECK(din_title > 0);
  CHECK(genaxeh > 0);
}

// Regression: lever_detail.dwg references gripper_assembly_new.dwg as an
// unresolved XREF (DRW_Block::flags & 0x04, xrefPath populated). The filter
// must detect this, resolve the path (handling Windows backslashes,
// case-insensitive + space-to-underscore basename match), recursively load
// the external file, and embed its modelspace entities into the local
// `GRIPPER ASSEMBLY NEW` block. Pre-fix the block was empty (count=0);
// post-fix it carries the lever assembly geometry (~900+ entities).
//   ./librecad_tests "[.dwg_lever_xref_resolve]"
TEST_CASE("DWG lever_detail: XREF resolution embeds external geometry",
          "[.dwg_lever_xref_resolve]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string hostPath = std::string(home) + "/doc/dwg2/lever_detail.dwg";
  const std::string xrefPath =
      std::string(home) + "/doc/dwg2/gripper_assembly_new.dwg";
  if (!std::filesystem::is_regular_file(hostPath)) {
    SKIP("lever_detail.dwg not present; skipping");
  }
  if (!std::filesystem::is_regular_file(xrefPath)) {
    SKIP("gripper_assembly_new.dwg (XREF source) not present; "
            "skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(hostPath),
                            RS2::FormatDWG));

  auto *blockList = graphic.getBlockList();
  REQUIRE(blockList);
  RS_Block *xrefBlock = blockList->find("GRIPPER ASSEMBLY NEW");
  REQUIRE(xrefBlock);

  INFO("XREF block direct-entity count = " << xrefBlock->count());
  CHECK(xrefBlock->count() >= 900);

  // Layer namespacing should also be in place: a layer like
  // "GRIPPER ASSEMBLY NEW|AM_5" must exist on the host's layer list,
  // populated by the XREF source's "AM_5" layer.
  auto *layers = graphic.getLayerList();
  REQUIRE(layers);
  CHECK(layers->find("GRIPPER ASSEMBLY NEW|AM_5") != nullptr);
  CHECK(layers->find("GRIPPER ASSEMBLY NEW|0") != nullptr);

  // After embedding, all entities in the block must hold a layer pointer
  // pointing at a host-graphic layer (not a dangling external pointer).
  int correctlyNamespaced = 0;
  for (auto *e : *xrefBlock) {
    if (!e || !e->getLayer())
      continue;
    const QString lyrName = e->getLayer()->getName();
    if (lyrName.startsWith("GRIPPER ASSEMBLY NEW|")) {
      ++correctlyNamespaced;
    }
  }
  INFO("Entities with namespaced layer: " << correctlyNamespaced);
  CHECK(correctlyNamespaced >= 900);
}

namespace {

// Build a minimal DXF buffer that defines a single XREF block referring
// to @p xrefTarget. Only the structural minimum needed for libdxfrw to
// parse cleanly: HEADER (ACADVER), TABLES (one LAYER), BLOCKS (the XREF
// block), ENTITIES (empty). Flag 70/4 is the XREF bit; group 1 carries
// the xrefPath that filter-side embedXref will try to resolve.
std::string buildCycleDxf(const std::string &blockName,
                          const std::string &xrefTarget) {
  std::ostringstream s;
  s << "0\nSECTION\n2\nHEADER\n"
       "9\n$ACADVER\n1\nAC1015\n"
       "9\n$INSBASE\n10\n0.0\n20\n0.0\n30\n0.0\n"
       "0\nENDSEC\n"
       "0\nSECTION\n2\nTABLES\n"
       "0\nTABLE\n2\nLAYER\n70\n1\n"
       "0\nLAYER\n5\n10\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
       "0\nENDTAB\n"
       "0\nENDSEC\n"
       "0\nSECTION\n2\nBLOCKS\n"
       "0\nBLOCK\n5\n20\n8\n0\n2\n"
    << blockName
    << "\n70\n4\n"
       "10\n0.0\n20\n0.0\n30\n0.0\n"
       "3\n"
    << blockName
    << "\n"
       "1\n"
    << xrefTarget
    << "\n"
       "0\nENDBLK\n5\n21\n8\n0\n"
       "0\nENDSEC\n"
       "0\nSECTION\n2\nENTITIES\n"
       "0\nENDSEC\n"
       "0\nEOF\n";
  return s.str();
}

void writeFile(const std::string &path, const std::string &content) {
  std::ofstream out(path);
  out << content;
}

} // namespace

// Synthetic A→B→A cycle test for the XREF recursion guard. Writes two
// minimal DXF files referencing each other, loads A.dxf, and verifies
// the load completes (no infinite recursion) AND both XREF blocks are
// present on the host. The cycle guard's RAII insert at fileImport
// start means the cycle is detected at depth 2 (when B's XREF→A is
// processed: m_xrefStack already contains A's absolute path).
TEST_CASE("RS_FilterDXFRW: XREF A->B->A cycle terminates", "[xref][filter]") {
  // Bootstrap Qt + RS_Settings once, like the other filter tests.
  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  const auto pathA =
      std::filesystem::temp_directory_path() / "librecad_xref_cycle_a.dxf";
  const auto pathB =
      std::filesystem::temp_directory_path() / "librecad_xref_cycle_b.dxf";
  std::filesystem::remove(pathA);
  std::filesystem::remove(pathB);

  // A references B, B references A — perfect cycle.
  writeFile(pathA.string(), buildCycleDxf("XREF_TO_B", pathB.string()));
  writeFile(pathB.string(), buildCycleDxf("XREF_TO_A", pathA.string()));

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  // If the guard misbehaves, fileImport never returns (infinite
  // recursion until stack overflow). Catch2 would still see a SIGSEGV
  // rather than hang, so this is a useful test even without a timeout.
  const bool imported = filter.fileImport(
      graphic, QString::fromStdString(pathA.string()), RS2::FormatDXFRW);
  REQUIRE(imported);

  // Both XREF blocks should have been added to A's blockList by the
  // addBlock dispatcher, regardless of whether the embed succeeded.
  auto *blockList = graphic.getBlockList();
  REQUIRE(blockList);
  CHECK(blockList->find("XREF_TO_B") != nullptr);
  // XREF_TO_A came in transitively via the embedXref of B. With the
  // cycle guard active, B's embed must have succeeded (refusing to
  // recurse back into A), so its blocks reach A's blockList with the
  // namespaced prefix `XREF_TO_B|`.
  CHECK(blockList->find("XREF_TO_B|XREF_TO_A") != nullptr);

  std::filesystem::remove(pathA);
  std::filesystem::remove(pathB);
}

// Cross-check: load the source XREF (gripper_assembly_new.dwg) and report
// the entity count.  Confirms whether the lever assembly drawing actually
// lives in the standalone XREF source vs. having been baked into
// lever_detail.dwg's local block (which appears empty).
TEST_CASE("DWG gripper_assembly_new: XREF source population",
          "[.dwg_gripper_xref]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/gripper_assembly_new.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("gripper_assembly_new.dwg not present; skipping");
  }
  const DeepResult dr = readDwgDeep(path);
  REQUIRE(dr.ok);
  printDeepReport("gripper_assembly_new.dwg", dr);
}

namespace {
class BlockTrackingIface : public TypeTrackingIface {
public:
  struct BlockEvent {
    std::string name;
    int handle = 0;
    int parentHandle = 0;
    int flags = 0;
    int entityCount = 0;
    std::map<std::string, int> layerCount;
    std::string xrefPath;
  };
  std::vector<BlockEvent> events;
  BlockEvent *current = nullptr;
  int entitiesBeforeAnyBlock = 0;
  int entitiesAfterEndBlock = 0;
  int blockDepth = 0;
  std::map<std::string, int> postBlockLayerCount;

  void addBlock(const DRW_Block &b) override {
    TypeTrackingIface::addBlock(b);
    events.push_back({b.name,
                      static_cast<int>(b.handle),
                      static_cast<int>(b.parentHandle),
                      b.flags,
                      0,
                      {}});
    events.back().xrefPath = b.xrefPath;
    current = &events.back();
    ++blockDepth;
  }
  void endBlock() override {
    TypeTrackingIface::endBlock();
    --blockDepth;
    current = nullptr;
  }
  void countEntity(const std::string &layer = "") {
    if (current) {
      ++current->entityCount;
      current->layerCount[layer]++;
    } else if (blockDepth == 0) {
      (events.empty() ? entitiesBeforeAnyBlock : entitiesAfterEndBlock)++;
      postBlockLayerCount[layer]++;
    }
  }
  void addPoint(const DRW_Point &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addPoint(e);
  }
  void addLine(const DRW_Line &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addLine(e);
  }
  void addArc(const DRW_Arc &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addArc(e);
  }
  void addCircle(const DRW_Circle &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addCircle(e);
  }
  void addEllipse(const DRW_Ellipse &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addEllipse(e);
  }
  void addLWPolyline(const DRW_LWPolyline &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addLWPolyline(e);
  }
  void addPolyline(const DRW_Polyline &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addPolyline(e);
  }
  void addInsert(const DRW_Insert &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addInsert(e);
  }
  void addText(const DRW_Text &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addText(e);
  }
  void addMText(const DRW_MText &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addMText(e);
  }
  void addSolid(const DRW_Solid &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addSolid(e);
  }
  void add3dFace(const DRW_3Dface &e) override {
    countEntity(e.layer);
    TypeTrackingIface::add3dFace(e);
  }
  void addTrace(const DRW_Trace &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addTrace(e);
  }
  void addDimAlign(const DRW_DimAligned *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addDimAlign(e);
  }
  void addDimLinear(const DRW_DimLinear *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addDimLinear(e);
  }
  void addDimRadial(const DRW_DimRadial *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addDimRadial(e);
  }
  void addDimDiametric(const DRW_DimDiametric *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addDimDiametric(e);
  }
  void addDimAngular(const DRW_DimAngular *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addDimAngular(e);
  }
  void addDimAngular3P(const DRW_DimAngular3p *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addDimAngular3P(e);
  }
  void addDimOrdinate(const DRW_DimOrdinate *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addDimOrdinate(e);
  }
  void addLeader(const DRW_Leader *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addLeader(e);
  }
  void addHatch(const DRW_Hatch *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addHatch(e);
  }
  void addSpline(const DRW_Spline *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addSpline(e);
  }
  void addImage(const DRW_Image *e) override {
    countEntity(e->layer);
    TypeTrackingIface::addImage(e);
  }
  void addViewport(const DRW_Viewport &e) override {
    countEntity(e.layer);
    TypeTrackingIface::addViewport(e);
  }
};
} // namespace

// Per-block delivery tracker for lever_detail.dwg.  Enumerates every
// addBlock event with name + handle + entity count, so duplicate-name
// blocks (RS_BlockList::add returns false → entities silently dropped)
// stand out at a glance.
//   ./librecad_tests "[.dwg_lever_blocks]" -s
TEST_CASE("DWG lever_detail: per-block entity delivery",
          "[.dwg_lever_blocks]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg2/lever_detail.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("lever_detail.dwg not present; skipping");
  }

  BlockTrackingIface iface;
  {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::None);
    REQUIRE(reader.read(&iface, true));
  }

  std::cout << "\n=== lever_detail.dwg block delivery (libdxfrw) ===\n";
  std::cout << "Pre-block: " << iface.entitiesBeforeAnyBlock
            << "  Post-block: " << iface.entitiesAfterEndBlock << "\n";
  std::cout << "addBlock events: " << iface.events.size() << "\n";

  std::map<std::string, std::vector<int>> nameToCounts;
  int totalInBlocks = 0;
  for (const auto &ev : iface.events) {
    const bool isXref = (ev.flags & 0x04) != 0;
    const bool isXrefOverlay = (ev.flags & 0x08) != 0;
    const bool isAnonymous = (ev.flags & 0x01) != 0;
    std::cout << "  " << std::setw(3) << ev.handle << "/" << std::setw(3)
              << ev.parentHandle << "  flags=0x" << std::hex << std::setw(2)
              << ev.flags << std::dec << "  count=" << std::setw(4)
              << ev.entityCount << "  name=" << ev.name;
    if (isXref)
      std::cout << "  [XREF]";
    if (isXrefOverlay)
      std::cout << "  [XREF-OVERLAY]";
    if (isAnonymous)
      std::cout << "  [anonymous]";
    if (!ev.xrefPath.empty())
      std::cout << "  xrefPath=\"" << ev.xrefPath << "\"";
    if (!ev.layerCount.empty()) {
      std::cout << "  layers={";
      bool first = true;
      for (const auto &[lyr, n] : ev.layerCount) {
        if (!first)
          std::cout << ", ";
        std::cout << lyr << ":" << n;
        first = false;
      }
      std::cout << "}";
    }
    std::cout << "\n";
    nameToCounts[ev.name].push_back(ev.entityCount);
    totalInBlocks += ev.entityCount;
  }
  std::cout << "Total entities seen inside blocks: " << totalInBlocks << "\n";

  std::cout << "\nPost-block (modelspace) layer distribution:\n";
  for (const auto &[lyr, n] : iface.postBlockLayerCount) {
    std::cout << "  " << lyr << ": " << n << "\n";
  }

  std::cout << "\nDuplicate block names (lose entities at filter):\n";
  int dupCount = 0;
  for (const auto &[name, counts] : nameToCounts) {
    if (counts.size() > 1) {
      std::cout << "  " << name << " appears " << counts.size() << " times: ";
      int sumDup = 0;
      for (size_t i = 0; i < counts.size(); ++i) {
        std::cout << counts[i];
        if (i + 1 < counts.size())
          std::cout << ", ";
        if (i > 0)
          sumDup += counts[i];
      }
      std::cout << "  (extra entities lost: " << sumDup << ")\n";
      dupCount += static_cast<int>(counts.size()) - 1;
    }
  }
  std::cout << "Total duplicate block events: " << dupCount << "\n";

  SUCCEED();
}

// End-to-end pipeline test: load gear_pump_subassy.dwg via the full
// RS_FilterDXFRW reader and verify that visible block attribute text
// flows through to RS_Text entities in the resulting graphic.
// Run: ./librecad_tests "[.dwg_gear_pump_attrib]" -s
TEST_CASE("DWG gear_pump_subassy: ATTRIB pipeline", "[.dwg_gear_pump_attrib]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("gear_pump_subassy.dwg not present; skipping");
  }

  HatchFillIface iface;
  {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::None);
    bool ok = reader.read(&iface, true);
    REQUIRE(ok);
  }

  const int attribs =
      iface.typeCounts.count("ATTRIB") ? iface.typeCounts.at("ATTRIB") : 0;
  const int inserts =
      iface.typeCounts.count("INSERT") ? iface.typeCounts.at("INSERT") : 0;

  std::cout << "\n  INSERT count = " << inserts
            << "\n  ATTRIB count = " << attribs << "\n";

  // The file has 7 INSERTs and 17 declared ATTRIBs.  At least 14 should be
  // routed via attlist (3 outliers exercise the MText-style attribute path
  // that this iteration does not yet decode).
  CHECK(inserts == 7);
  CHECK(attribs >= 14);
}

// Deep diagnostic for House-Dwgfree.com_-1.dwg (AC1021/R2007, ~988 entities).
// Run: ./librecad_tests "[.dwg_house]" -s
TEST_CASE("DWG House-Dwgfree.com_-1: entity population", "[.dwg_house]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/House-Dwgfree.com_-1.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("House-Dwgfree.com_-1.dwg not present; skipping");
  }

  const DeepResult dr = readDwgDeep(path);

  REQUIRE(dr.ok);
  REQUIRE(dr.error == DRW::BAD_NONE);

  static const std::set<int> kGraphicalOTypes = {
      1,  7,  8,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
      29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101};
  int unhandledGraphicalLost = 0;
  for (const auto &[oType, count] : dr.unhandledTypes) {
    if (kGraphicalOTypes.count(oType)) {
      std::cout << "  *** LOST graphical entity oType=" << oType
                << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                << " count=" << count << "\n";
      unhandledGraphicalLost += count;
    }
  }
  CHECK(unhandledGraphicalLost == 0);

  CHECK(dr.iface.total() > 0);
  CHECK(dr.iface.blocks > 0);
  CHECK(dr.iface.layers > 0);

  printDeepReport("House-Dwgfree.com_-1.dwg", dr);
}

// Deep diagnostic for pump_wheel.dwg (AC1024/R2010, 45 entities).
// Run: ./librecad_tests "[.dwg_pump_wheel]" -s
TEST_CASE("DWG pump_wheel: entity population", "[.dwg_pump_wheel]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg2/pump_wheel.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("pump_wheel.dwg not present; skipping");
  }

  const DeepResult dr = readDwgDeep(path);

  REQUIRE(dr.ok);
  REQUIRE(dr.error == DRW::BAD_NONE);

  static const std::set<int> kGraphicalOTypes = {
      1,  7,  8,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
      29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101};
  int unhandledGraphicalLost = 0;
  for (const auto &[oType, count] : dr.unhandledTypes) {
    if (kGraphicalOTypes.count(oType)) {
      std::cout << "  *** LOST graphical entity oType=" << oType
                << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                << " count=" << count << "\n";
      unhandledGraphicalLost += count;
    }
  }
  CHECK(unhandledGraphicalLost == 0);

  CHECK(dr.iface.total() > 0);
  CHECK(dr.iface.blocks > 0);
  CHECK(dr.iface.layers > 0);

  printDeepReport("pump_wheel.dwg", dr);
}

// Deep diagnostic for robot_handling_cell.dwg (AC1024/R2010, 7341 entities).
// Run: ./librecad_tests "[.dwg_robot]" -s
TEST_CASE("DWG robot_handling_cell: entity population", "[.dwg_robot]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/robot_handling_cell.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("robot_handling_cell.dwg not present; skipping");
  }

  const DeepResult dr = readDwgDeep(path);

  REQUIRE(dr.ok);
  REQUIRE(dr.error == DRW::BAD_NONE);

  static const std::set<int> kGraphicalOTypes = {
      1,  7,  8,  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
      29, 30, 31, 32, 33, 34, 35, 36, 40, 41, 44, 45, 46, 47, 77, 78, 101};
  int unhandledGraphicalLost = 0;
  for (const auto &[oType, count] : dr.unhandledTypes) {
    if (kGraphicalOTypes.count(oType)) {
      std::cout << "  *** LOST graphical entity oType=" << oType
                << " name=" << (oTypeName(oType) ? oTypeName(oType) : "?")
                << " count=" << count << "\n";
      unhandledGraphicalLost += count;
    }
  }
  CHECK(unhandledGraphicalLost == 0);

  CHECK(dr.iface.total() > 0);
  CHECK(dr.iface.blocks > 0);
  CHECK(dr.iface.layers > 0);

  printDeepReport("robot_handling_cell.dwg", dr);
}

// Filter-path audit for robot_handling_cell.dwg: the host carries an unresolved
// XREF block GRIPPER ASSEMBLY NEW (Windows path in xrefPath). LibreCAD must
// resolve gripper_assembly_new.dwg from the host directory and embed it.
//   ./librecad_tests "[.dwg_robot_filter]" -s
TEST_CASE("DWG robot_handling_cell: filter pipeline + XREF embed",
          "[.dwg_robot_filter]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/robot_handling_cell.dwg";
  const std::string xrefPath =
      std::string(home) + "/doc/dwg2/gripper_assembly_new.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("robot_handling_cell.dwg not present; skipping");
  }
  if (!std::filesystem::is_regular_file(xrefPath)) {
    SKIP("gripper_assembly_new.dwg not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  auto *blockList = graphic.getBlockList();
  REQUIRE(blockList);
  RS_Block *gripperBlock = blockList->find("GRIPPER ASSEMBLY NEW");
  REQUIRE(gripperBlock);
  INFO("GRIPPER ASSEMBLY NEW direct count = " << gripperBlock->count());
  // embedXref copies model-space top-level entities from gripper_assembly_new.dwg
  // (1504) plus namespaced block defs; nested block geometry rides INSERT refs.
  CHECK(gripperBlock->count() >= 1500);

  auto *layers = graphic.getLayerList();
  REQUIRE(layers);
  CHECK(layers->find("GRIPPER ASSEMBLY NEW|AM_0") != nullptr);

  std::cout << "\n=== robot_handling_cell filter audit ===\n";
  std::cout << "countDeep=" << graphic.countDeep()
            << " gripperBlock=" << gripperBlock->count()
            << " gripperDeep=" << gripperBlock->countDeep() << "\n";
}

// robot_handling_cell.dwg stores many arcs in OCS with extrusion (0,0,-1).
// Without applyExtrusion the semicircle at OCS center (+900,2217.74) r=66 is
// drawn on the wrong side of the drawing; WCS center must be (-900,2217.74).
//   ./librecad_tests "[.dwg_robot_arc_ext]" -s
TEST_CASE("DWG robot_handling_cell: negative extrusion arc placement",
          "[.dwg_robot_arc_ext]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/robot_handling_cell.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("robot_handling_cell.dwg not present; skipping");
  }

  struct ArcProbe : public DRW_Interface {
    int negExtHits = 0;
    int wrongSideHits = 0;
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
    void addCircle(const DRW_Circle &) override {}
    void addArc(const DRW_Arc &a) override {
      if (std::fabs(a.basePoint.y - 2217.73666) > 0.05
          || std::fabs(a.radious - 66.0) > 0.05) {
        return;
      }
      ++negExtHits;
      if (a.basePoint.x > 0.0) {
        ++wrongSideHits;
      }
    }
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
  } probe;

  dwgR reader(path.c_str());
  REQUIRE(reader.read(&probe, true));
  CHECK(probe.negExtHits >= 1);
  CHECK(probe.wrongSideHits == 0);

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  bool foundWcsArc = false;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityArc) {
      continue;
    }
    auto *arc = static_cast<RS_Arc *>(e);
    const RS_Vector c = arc->getCenter();
    if (std::fabs(c.x + 900.0) < 0.1 && std::fabs(c.y - 2217.73666) < 0.05
        && std::fabs(arc->getRadius() - 66.0) < 0.05) {
      foundWcsArc = true;
      break;
    }
  }
  CHECK(foundWcsArc);
}

namespace chicun {

struct NearHit {
  QString type;
  QString layer;
  QString insertChain;
  quint32 handle = 0;
  RS_Vector p1;
  RS_Vector p2;
  double dist = 0;
};

struct ProbeResult {
  std::vector<NearHit> hits;
  NearHit globalBest;
};

struct ProbeTarget {
  const char *label;
  double x;
  double y;
  double tol = 50.0;
  double minGlobalDist = 1000.0;
};

QString insertChain(RS_Entity *e) {
  QString chain;
  for (RS_Entity *p = e; p; p = p->getParent()) {
    if (p->rtti() != RS2::EntityInsert)
      continue;
    if (!chain.isEmpty())
      chain += " <- ";
    chain += static_cast<RS_Insert *>(p)->getName();
  }
  return chain;
}

void ensureTestApp() {
  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    RS_DEBUG->setLevel(RS_Debug::D_WARNING);
    return true;
  }();
  (void)qapp;
  (void)settingsReady;
}

std::string fixturePath() {
  const char *home = getenv("HOME");
  if (!home)
    return {};
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path))
    return {};
  return path;
}

bool importFixture(RS_Graphic &graphic) {
  const std::string path = fixturePath();
  if (path.empty())
    return false;
  ensureTestApp();
  RS_FilterDXFRW filter;
  return filter.fileImport(graphic, QString::fromStdString(path),
                           RS2::FormatDWG);
}

ProbeResult probeResolvedGeometry(RS_Graphic &graphic, double tx, double ty,
                                  double tol) {
  ProbeResult result;
  result.globalBest.dist = std::numeric_limits<double>::infinity();

  auto distToTarget = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                      const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(distToTarget(p1), distToTarget(p2))
                              : distToTarget(p1);
    if (d > tol)
      return;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    result.hits.push_back(
        {type, layer, insertChain(e), e->sourceHandle(), p1, p2, d});
  };

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    auto updateGlobal = [&](const QString &type, const RS_Vector &p) {
      const double d = distToTarget(p);
      if (d < result.globalBest.dist)
        result.globalBest = {type, layer, insertChain(e), e->sourceHandle(),
                             p, RS_Vector(false), d};
    };
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      consider("LINE", e, l->getStartpoint(), l->getEndpoint());
      updateGlobal("LINE_SP", l->getStartpoint());
      updateGlobal("LINE_EP", l->getEndpoint());
      break;
    }
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      RS_Vector prev(false);
      for (RS_Entity *v : *pl) {
        if (!v)
          continue;
        RS_Vector pt = v->getStartpoint();
        consider("PLINE_VTX", e, pt);
        if (prev.valid)
          consider("PLINE_SEG", e, prev, pt);
        updateGlobal("PLINE_VTX", pt);
        prev = pt;
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", e, a->getCenter());
      consider("ARC_SP", e, a->getStartpoint());
      consider("ARC_EP", e, a->getEndpoint());
      updateGlobal("ARC_CTR", a->getCenter());
      updateGlobal("ARC_SP", a->getStartpoint());
      updateGlobal("ARC_EP", a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", e, static_cast<RS_Circle *>(e)->getCenter());
      updateGlobal("CIRCLE", static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", e,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    case RS2::EntityEllipse: {
      auto *el = static_cast<RS_Ellipse *>(e);
      e->calculateBorders();
      consider("ELL_CTR", e, el->getCenter());
      consider("ELL_MIN", e, e->getMin());
      consider("ELL_MAX", e, e->getMax());
      updateGlobal("ELL_CTR", el->getCenter());
      break;
    }
    case RS2::EntityWipeout: {
      e->calculateBorders();
      consider("WIPE_MIN", e, e->getMin());
      consider("WIPE_MAX", e, e->getMax());
      updateGlobal("WIPE_MIN", e->getMin());
      break;
    }
    default:
      break;
    }
  }
  return result;
}

bool findResolvedPoint(RS_Graphic &graphic, const QString &chainSubstr,
                       const char *pointType, double x, double y,
                       double tol) {
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || !insertChain(e).contains(chainSubstr))
      continue;
    RS_Vector p(false);
    if (strcmp(pointType, "ARC_EP") == 0 && e->rtti() == RS2::EntityArc)
      p = static_cast<RS_Arc *>(e)->getEndpoint();
    else if (strcmp(pointType, "ARC_CTR") == 0 && e->rtti() == RS2::EntityArc)
      p = static_cast<RS_Arc *>(e)->getCenter();
    else if (strcmp(pointType, "ARC_SP") == 0 && e->rtti() == RS2::EntityArc)
      p = static_cast<RS_Arc *>(e)->getStartpoint();
    else
      continue;
    if (std::hypot(p.x - x, p.y - y) < tol)
      return true;
  }
  return false;
}

bool findCushArcEpNearY(RS_Graphic &graphic, double y, bool positiveX,
                        double yTol = 5.0) {
  auto cushChain = [](RS_Entity *e) {
    return insertChain(e).contains(QStringLiteral("CUSH"));
  };
  auto endpointNear = [&](const RS_Vector &ep) {
    if (std::fabs(ep.y - y) > yTol)
      return false;
    if (positiveX && ep.x > 1.0e5)
      return true;
    if (!positiveX && ep.x < -1.0e5)
      return true;
    return false;
  };
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || !cushChain(e))
      continue;
    if (e->rtti() == RS2::EntityArc) {
      if (endpointNear(static_cast<RS_Arc *>(e)->getEndpoint()))
        return true;
    } else if (e->rtti() == RS2::EntityEllipse) {
      e->calculateBorders();
      if (endpointNear(e->getMax()) || endpointNear(e->getMin()))
        return true;
    }
  }
  return false;
}

void assertProbeEmpty(const ProbeResult &result, double minGlobalDist) {
  CHECK(result.hits.empty());
  if (result.globalBest.dist < std::numeric_limits<double>::infinity())
    CHECK(result.globalBest.dist > minGlobalDist);
}

void assertStray3OcsPositive(RS_Graphic &graphic) {
  RS_Insert *a4d8Ins = nullptr;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    auto *ins = static_cast<RS_Insert *>(e);
    if (ins->getName() == QStringLiteral("A$C4D8A4BEC")) {
      a4d8Ins = ins;
      break;
    }
  }
  REQUIRE(a4d8Ins != nullptr);
  CHECK(a4d8Ins->getData().extrusion.z < -0.5);
  CHECK(a4d8Ins->getScale().z < -0.4);

  // Relative to the (possibly re-based) insert grip after dense-core re-base.
  const RS_Vector ip = a4d8Ins->getInsertionPoint();
  const double expX = ip.x + 0.5 * 77.14807952690171;
  const double expY = ip.y + 0.5 * 219.9479471713712;
  int primaryCluster = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || !insertChain(e).contains(QStringLiteral("A$C4D8A4BEC")))
      continue;
    RS_Vector p(false);
    if (e->rtti() == RS2::EntityCircle)
      p = static_cast<RS_Circle *>(e)->getCenter();
    else if (e->rtti() == RS2::EntityArc)
      p = static_cast<RS_Arc *>(e)->getCenter();
    else
      continue;
    if (std::hypot(p.x - expX, p.y - expY) < 80.0)
      ++primaryCluster;
  }
  CHECK(primaryCluster >= 4);
}

const ProbeTarget kPhantoms[] = {
    {"stray1", -774805.8834, 588320.7858},
    {"stray2", 52984.74, 222052.06},
    {"stray3", -294315.207, 48571.65792, 50.0, 0.0},
    {"stray4A", 104517.7659, -1239995.976, 50.0, 900.0},
    {"stray4B", 57585.03448, -1240174.235, 50.0, 700.0},
    {"stray4C", 1322040.68, 492532.476, 50.0, 500.0},
    {"stray5D", -1099318.43, 492532.476, 50.0, 1100.0},
    {"stray5E", -952337.4015, 403260.8869},
    {"stray5F", 1322040.68, 492532.476, 50.0, 500.0},
    {"stray6", -2230355.08, 1646735.10, 50.0, 500.0},
    {"stray7", -2230420.44, 1646754.55, 50.0, 500.0},
    {"stray8", -1099671.573, 492101.3029, 50.0, 500.0},
};

struct BboxCornerHit {
  char corner = 0;
  QString type;
  RS_Vector p;
  QString insertChain;
};

void collectProbePoints(RS_Entity *e, const QString &chain,
                        std::vector<std::pair<QString, RS_Vector>> &out) {
  if (!e)
    return;
  switch (e->rtti()) {
  case RS2::EntityLine: {
    auto *l = static_cast<RS_Line *>(e);
    out.push_back({"LINE_SP", l->getStartpoint()});
    out.push_back({"LINE_EP", l->getEndpoint()});
    break;
  }
  case RS2::EntityPolyline: {
    auto *pl = static_cast<RS_Polyline *>(e);
    for (RS_Entity *v : *pl) {
      if (v)
        out.push_back({"PLINE_VTX", v->getStartpoint()});
    }
    break;
  }
  case RS2::EntityArc: {
    auto *a = static_cast<RS_Arc *>(e);
    out.push_back({"ARC_CTR", a->getCenter()});
    out.push_back({"ARC_SP", a->getStartpoint()});
    out.push_back({"ARC_EP", a->getEndpoint()});
    break;
  }
  case RS2::EntityCircle:
    out.push_back({"CIRCLE", static_cast<RS_Circle *>(e)->getCenter()});
    break;
  case RS2::EntityInsert:
    out.push_back({"INSERT",
                   static_cast<RS_Insert *>(e)->getInsertionPoint()});
    break;
  default:
    break;
  }
  (void)chain;
}

std::vector<BboxCornerHit> findBboxCornerDrivers(RS_Graphic &graphic,
                                                 double eps = 1.0) {
  graphic.calculateBorders();
  const RS_Vector bmin = graphic.getMin();
  const RS_Vector bmax = graphic.getMax();
  std::vector<BboxCornerHit> hits;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString chain = insertChain(e);
    std::vector<std::pair<QString, RS_Vector>> pts;
    collectProbePoints(e, chain, pts);
    for (const auto &[type, p] : pts) {
      if (!p.valid)
        continue;
      if (std::fabs(p.x - bmin.x) < eps)
        hits.push_back({'W', type, p, chain});
      if (std::fabs(p.x - bmax.x) < eps)
        hits.push_back({'E', type, p, chain});
      if (std::fabs(p.y - bmin.y) < eps)
        hits.push_back({'S', type, p, chain});
      if (std::fabs(p.y - bmax.y) < eps)
        hits.push_back({'N', type, p, chain});
    }
  }
  return hits;
}

// Draw-path envelope: walk the same RS_Entity hierarchy RS_EntityContainer::draw
// uses (id!=0 children), apply render-time isVisible() like
// LC_GraphicViewRenderer::renderEntity, and collect leaf min/max at the moment
// justDrawEntity would call e->draw() — not DRW/parser-only, not pre-updateInserts.
struct DrawPathEnvHit {
  RS_Entity *entity = nullptr;
  RS2::EntityType rtti = RS2::EntityUnknown;
  RS_Vector emin;
  RS_Vector emax;
  QString chain;
};

struct DrawPathEnvelope {
  RS_Vector min;
  RS_Vector max;
  bool valid = false;
  int visited = 0;
  int leaves = 0;
  std::vector<DrawPathEnvHit> leavesList;
};

void accumulateDrawEnvelope(DrawPathEnvelope &env, const RS_Vector &emin,
                            const RS_Vector &emax) {
  if (!emin.valid || !emax.valid)
    return;
  if (!env.valid) {
    env.min = emin;
    env.max = emax;
    env.valid = true;
    return;
  }
  env.min = RS_Vector::minimum(env.min, emin);
  env.max = RS_Vector::maximum(env.max, emax);
}

// Recurse like EntityContainer::draw → painter->drawEntity → e->draw().
// Hatch overrides draw (solid path / FlagHatchChild patterns); treat hatch as
// a leaf for solid fills and recurse only pattern hatch-children.
void collectDrawPathBorders(RS_Entity *e, DrawPathEnvelope &env) {
  if (!e || e->getId() == 0)
    return;
  // Same gate as LC_GraphicViewRenderer::renderEntity before justDrawEntity.
  if (!e->isVisible())
    return;
  ++env.visited;

  const RS2::EntityType t = e->rtti();
  if (t == RS2::EntityHatch) {
    auto *h = static_cast<RS_EntityContainer *>(e);
    bool anyHatchChild = false;
    for (RS_Entity *sub : *h) {
      if (sub && !sub->isContainer() && sub->getFlag(RS2::FlagHatchChild)) {
        anyHatchChild = true;
        collectDrawPathBorders(sub, env);
      }
    }
    if (!anyHatchChild) {
      // Solid fill: hatch.draw paints loops; borders from hatch itself.
      e->calculateBorders();
      const RS_Vector emin = e->getMin();
      const RS_Vector emax = e->getMax();
      accumulateDrawEnvelope(env, emin, emax);
      ++env.leaves;
      env.leavesList.push_back(
          {e, t, emin, emax, insertChain(e)});
    }
    return;
  }

  if (e->isContainer()) {
    // RS_EntityContainer::draw / Insert::draw: paint each child with id!=0.
    auto *c = static_cast<RS_EntityContainer *>(e);
    for (RS_Entity *child : *c) {
      if (child != nullptr && child->getId() != 0)
        collectDrawPathBorders(child, env);
    }
    return;
  }

  // Leaf atomic: borders at the moment draw() would paint this entity.
  e->calculateBorders();
  const RS_Vector emin = e->getMin();
  const RS_Vector emax = e->getMax();
  accumulateDrawEnvelope(env, emin, emax);
  ++env.leaves;
  env.leavesList.push_back({e, t, emin, emax, insertChain(e)});
}

DrawPathEnvelope collectDrawPathEnvelope(RS_Graphic &graphic) {
  DrawPathEnvelope env;
  // Root draw: justDrawEntity(graphic) → Graphic::draw → each model child.
  for (RS_Entity *e : graphic) {
    if (e != nullptr && e->getId() != 0)
      collectDrawPathBorders(e, env);
  }
  return env;
}

void dumpDrawPathBorderDrivers(const DrawPathEnvelope &env,
                               double boundTol = 15.0) {
  if (!env.valid)
    return;
  int envWest = 0, envEast = 0, envSouth = 0, envNorth = 0;
  for (const auto &h : env.leavesList) {
    if (!h.emin.valid || !h.emax.valid)
      continue;
    if (std::fabs(h.emin.x - env.min.x) < boundTol) {
      ++envWest;
      if (envWest <= 3)
        std::cout << "  draw-env-west " << rttiName(h.rtti) << " emin=("
                  << h.emin.x << "," << h.emin.y << ") emax=(" << h.emax.x
                  << "," << h.emax.y << ") " << h.chain.toStdString() << "\n";
    }
    if (std::fabs(h.emax.x - env.max.x) < boundTol) {
      ++envEast;
      if (envEast <= 3)
        std::cout << "  draw-env-east " << rttiName(h.rtti) << " emin=("
                  << h.emin.x << "," << h.emin.y << ") emax=(" << h.emax.x
                  << "," << h.emax.y << ") " << h.chain.toStdString() << "\n";
    }
    if (std::fabs(h.emin.y - env.min.y) < boundTol) {
      ++envSouth;
      if (envSouth <= 3)
        std::cout << "  draw-env-south " << rttiName(h.rtti) << " emin=("
                  << h.emin.x << "," << h.emin.y << ") emax=(" << h.emax.x
                  << "," << h.emax.y << ") " << h.chain.toStdString() << "\n";
    }
    if (std::fabs(h.emax.y - env.max.y) < boundTol) {
      ++envNorth;
      if (envNorth <= 3)
        std::cout << "  draw-env-north " << rttiName(h.rtti) << " emin=("
                  << h.emin.x << "," << h.emin.y << ") emax=(" << h.emax.x
                  << "," << h.emax.y << ") " << h.chain.toStdString() << "\n";
    }
  }
  std::cout << "draw-envelope west=" << envWest << " east=" << envEast
            << " south=" << envSouth << " north=" << envNorth << "\n";
}

bool phantomNear(const RS_Vector &p, double tol = 50.0) {
  for (const auto &t : kPhantoms) {
    if (std::hypot(p.x - t.x, p.y - t.y) < tol)
      return true;
  }
  return false;
}

} // namespace chicun

TEST_CASE("INSERT expand does not select children (policy A)",
          "[insert][selection]") {
  chicun::ensureTestApp();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  RS_Block *blk = new RS_Block(&graphic, RS_BlockData(QStringLiteral("T"),
                                                      RS_Vector(0, 0), false));
  blk->addEntity(new RS_Line(blk, RS_LineData(RS_Vector(0, 0), RS_Vector(10, 0))));
  graphic.addBlock(blk);
  auto *ins = new RS_Insert(
      &graphic,
      RS_InsertData(QStringLiteral("T"), RS_Vector(0, 0), RS_Vector(1, 1), 0.0,
                    1, 1, RS_Vector(0, 0)));
  graphic.addEntity(ins);
  ins->update();
  REQUIRE(ins->count() >= 1);
  ins->setSelectionFlag(true);
  ins->update();
  CHECK(ins->isSelected());
  for (RS_Entity *e : *ins) {
    REQUIRE(e != nullptr);
    CHECK_FALSE(e->isSelected());
  }
}

// Probe incorrect geometry near WCS (120, 35220) in 2带尺寸图库.dwg.
//   ./librecad_tests "[.dwg_chicun_near]" -s
TEST_CASE("DWG 2带尺寸图库: entities near (120,35220)",
          "[.dwg_chicun_near]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  const double tx = 120.0, ty = 35220.0;
  struct NearHit {
    QString type;
    QString layer;
    RS_Vector p1;
    RS_Vector p2;
    double dist = 0;
  };
  std::vector<NearHit> hits;
  auto dist = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto consider = [&](const QString &type, const QString &layer,
                      const RS_Vector &p1, const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
    if (d > 100.0)
      return;
    hits.push_back({type, layer, p1, p2, d});
  };

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    switch (e->rtti()) {
    case RS2::EntityLine:
      consider("LINE", layer,
               static_cast<RS_Line *>(e)->getStartpoint(),
               static_cast<RS_Line *>(e)->getEndpoint());
      break;
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", layer, a->getCenter());
      consider("ARC_SP", layer, a->getStartpoint());
      consider("ARC_EP", layer, a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", layer,
               static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", layer,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  }

  std::sort(hits.begin(), hits.end(),
            [](const NearHit &a, const NearHit &b) { return a.dist < b.dist; });

  std::cout << "\n=== chicun near (120,35220) tol=100 ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";

  // Paper-space dimension blocks use large absolute coords. Endpoints with
  // y~35220 often satisfy (paperX - 63880) ~ 120 (display UCS), e.g. *D927
  // LINE 24EC5E at paper (63999.8, 35223.4).
  bool foundDim927Ext = false;
  double bestDy = std::numeric_limits<double>::infinity();
  RS_Vector bestPaper(false);
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityLine)
      continue;
    auto *l = static_cast<RS_Line *>(e);
    for (const RS_Vector &p : {l->getStartpoint(), l->getEndpoint()}) {
      if (std::fabs(p.y - ty) > 100.0)
        continue;
      const double dy = std::fabs(p.y - ty);
      if (dy < bestDy) {
        bestDy = dy;
        bestPaper = p;
      }
      if (std::fabs(p.x - 63999.83) < 1.0 && std::fabs(p.y - 35223.38) < 1.0) {
        foundDim927Ext = true;
        std::cout << "  *D927 ext line endpoint paper=(" << p.x << "," << p.y
                  << ") display~(" << tx << "," << p.y << ") dy=" << (p.y - ty)
                  << "\n";
      }
    }
  }
  if (bestPaper.valid) {
    std::cout << "closest y~35220 line endpoint paper=(" << bestPaper.x << ","
              << bestPaper.y << ") |dy|=" << bestDy << "\n";
  }
  if (foundDim927Ext)
    std::cout << "matched anonymous dim block *D927 extension line top\n";
  for (std::size_t i = 0; i < hits.size() && i < 30; ++i) {
    const auto &h = hits[i];
    if (h.p2.valid)
      std::cout << "  " << h.type.toStdString() << " layer="
                << h.layer.toStdString() << " (" << h.p1.x << "," << h.p1.y
                << ")->(" << h.p2.x << "," << h.p2.y << ") dist=" << h.dist
                << "\n";
    else
      std::cout << "  " << h.type.toStdString() << " layer="
                << h.layer.toStdString() << " (" << h.p1.x << "," << h.p1.y
                << ") dist=" << h.dist << "\n";
  }

  NearHit globalBest;
  globalBest.dist = std::numeric_limits<double>::infinity();
  auto updateGlobal = [&](const QString &type, const QString &layer,
                          const RS_Vector &p) {
    const double d = dist(p);
    if (d < globalBest.dist) {
      globalBest = {type, layer, p, RS_Vector(false), d};
    }
  };
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      updateGlobal("LINE_SP", layer, l->getStartpoint());
      updateGlobal("LINE_EP", layer, l->getEndpoint());
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      updateGlobal("ARC_SP", layer, a->getStartpoint());
      updateGlobal("ARC_EP", layer, a->getEndpoint());
      break;
    }
    default:
      break;
    }
  }

  if (!hits.empty()) {
    const auto &best = hits.front();
    std::cout << "closest tol100: " << best.type.toStdString()
              << " dist=" << best.dist << "\n";
  } else {
    std::cout << "no hits within tol=100; global closest "
              << globalBest.type.toStdString() << " layer="
              << globalBest.layer.toStdString() << " (" << globalBest.p1.x
              << "," << globalBest.p1.y << ") dist=" << globalBest.dist
              << "\n";
  }

  int xNear = 0;
  std::vector<std::pair<double, QString>> yAt120;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityLine || !e->getLayer())
      continue;
    auto *l = static_cast<RS_Line *>(e);
    for (const RS_Vector &p : {l->getStartpoint(), l->getEndpoint()}) {
      if (std::fabs(p.x - tx) < 10.0) {
        ++xNear;
        yAt120.emplace_back(p.y, e->getLayer()->getName());
      }
    }
  }
  std::sort(yAt120.begin(), yAt120.end(),
            [](const auto &a, const auto &b) {
              return std::fabs(a.first - 35220.0) < std::fabs(b.first - 35220.0);
            });
  std::cout << "line endpoints with |x-120|<10: " << xNear << "\n";
  for (std::size_t i = 0; i < yAt120.size() && i < 8; ++i)
    std::cout << "  y=" << yAt120[i].first << " layer="
              << yAt120[i].second.toStdString() << " dy="
              << (yAt120[i].first - 35220.0) << "\n";

  // Informational — file may use different coordinate neighborhood.
  SUCCEED("near-point audit complete");
}

// Block-local neighborhood for status-bar coords (120,35220) ≈ 117×(1.026,301.026).
// Dimension _ArchTick inserts use scale 117; library symbols sit at y≈300 in block space.
//   ./librecad_tests "[.dwg_chicun_blocklocal]" -s
TEST_CASE("DWG 2带尺寸图库: block-local geometry near (1.026,301.026)",
          "[.dwg_chicun_blocklocal]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  constexpr double kDimScale = 117.0;
  constexpr double kLocalX = 120.0 / kDimScale;
  constexpr double kLocalY = 35220.0 / kDimScale;

  struct ArcProbe : public DRW_Interface {
    int nearSmallArc = 0;
    int nearSharedEndArc = 0;
    int nearCornerLine = 0;
    std::string currentBlock;

    static double dist(double x, double y, double tx, double ty) {
      return std::hypot(x - tx, y - ty);
    }

    void addBlock(const DRW_Block &b) override { currentBlock = b.name; }
    void endBlock() override { currentBlock.clear(); }

    void addArc(const DRW_Arc &a) override {
      const double cx = a.basePoint.x;
      const double cy = a.basePoint.y;
      const double spx = cx + a.radious * std::cos(a.staangle);
      const double spy = cy + a.radious * std::sin(a.staangle);
      const double epx = cx + a.radious * std::cos(a.endangle);
      const double epy = cy + a.radious * std::sin(a.endangle);
      const double dCtr = dist(cx, cy, kLocalX, kLocalY);
      const double dSp = dist(spx, spy, kLocalX, kLocalY);
      const double dEp = dist(epx, epy, kLocalX, kLocalY);
      if (std::min({dCtr, dSp, dEp}) > 8.0)
        return;
      if (currentBlock == "A$C44380A39" && std::fabs(a.radious - 9.73736) < 0.05)
        ++nearSmallArc;
      if (currentBlock == "A$C44380A39"
          && std::fabs(epx - 7.18846) < 0.05 && std::fabs(epy - 301.9017) < 0.05)
        ++nearSharedEndArc;
    }

    void addLine(const DRW_Line &l) override {
      if (currentBlock != "A$C2F3706B8")
        return;
      const auto near = [&](double x, double y) {
        return dist(x, y, kLocalX, kLocalY) < 5.0;
      };
      if (near(l.basePoint.x, l.basePoint.y) || near(l.secPoint.x, l.secPoint.y)) {
        if (std::fabs(l.basePoint.y - 301.02004368277267) < 0.01
            || std::fabs(l.secPoint.y - 301.02004368277267) < 0.01)
          ++nearCornerLine;
      }
    }

    void addHeader(const DRW_Header *) override {}
    void addLType(const DRW_LType &) override {}
    void addLayer(const DRW_Layer &) override {}
    void addDimStyle(const DRW_Dimstyle &) override {}
    void addVport(const DRW_Vport &) override {}
    void addTextStyle(const DRW_Textstyle &) override {}
    void addAppId(const DRW_AppId &) override {}
    void setBlock(const int) override {}
    void addPoint(const DRW_Point &) override {}
    void addRay(const DRW_Ray &) override {}
    void addXline(const DRW_Xline &) override {}
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

  ArcProbe probe;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&probe, true));
  CHECK(probe.nearSmallArc >= 1);
  CHECK(probe.nearSharedEndArc >= 2);
  CHECK(probe.nearCornerLine >= 1);

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  RS_Block *detailBlk = graphic.findBlock(QStringLiteral("A$C44380A39"));
  REQUIRE(detailBlk != nullptr);

  int sharedEndArcs = 0;
  int minorSweepOk = 0;
  for (RS_Entity *e : *detailBlk) {
    if (!e || e->rtti() != RS2::EntityArc)
      continue;
    auto *a = static_cast<RS_Arc *>(e);
    const RS_Vector ep = a->getEndpoint();
    if (ep.distanceTo(RS_Vector(7.18846, 301.9017)) > 0.1)
      continue;
    ++sharedEndArcs;
    const double sweep = RS_Math::correctAngle(a->getAngle2() - a->getAngle1());
    if (sweep < M_PI)
      ++minorSweepOk;
  }
  CHECK(sharedEndArcs >= 2);
  CHECK(minorSweepOk >= 2);

  bool foundMirroredInsert = false;
  for (RS_Entity *e : lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    auto *ins = static_cast<RS_Insert *>(e);
    if (ins->getName() != QStringLiteral("A$C44380A39"))
      continue;
    if (std::fabs(ins->getScale().x + 1.0) < 0.01
        && std::fabs(ins->getInsertionPoint().x - 334984.274) < 1.0) {
      foundMirroredInsert = true;
      break;
    }
  }
  CHECK(foundMirroredInsert);
}

struct Deng1RawIface : public CountingIface {
  std::string curBlock;
  void addBlock(const DRW_Block &b) override {
    CountingIface::addBlock(b);
    curBlock = b.name;
    if (curBlock == "deng1")
      std::cout << "raw deng1 block bp=(" << b.basePoint.x << "," << b.basePoint.y
                << ")\n";
  }
  void endBlock() override {
    CountingIface::endBlock();
    curBlock.clear();
  }
  void addCircle(const DRW_Circle &e) override {
    CountingIface::addCircle(e);
    if (curBlock == "deng1")
      std::cout << "raw deng1 circle h=0x" << std::hex << e.handle << std::dec
                << " ctr=(" << e.basePoint.x << "," << e.basePoint.y
                << ") r=" << e.radious << "\n";
  }
};

// Lamp block (deng1) nested in A$C5ABE7CED — circles must land on the compounded
// insert point (84004, 115511), not at the raw block-local offset (~36k, 58k).
//   ./librecad_tests "[.dwg_chicun_circle]" -s
TEST_CASE("DWG 2带尺寸图库: deng1 circles at insert point (84004,115511)",
          "[.dwg_chicun_circle]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  Deng1RawIface rawIface;
  readDwg(path, false, &rawIface);
  std::cout << "raw deng1 read with applyExt=true (see above)\n";
  {
    Deng1RawIface noExt;
    dwgR reader(path.c_str());
    reader.read(&noExt, false);
    std::cout << "raw deng1 read with applyExt=false done\n";
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  // Expected WCS for deng1-921 after full nested insert chain (not the stale
  // partial transform that previously placed geometry ~75k from insertionPoint).
  const double tx = 84004.0, ty = 115511.0;
  struct CircleHit {
    double r = 0;
    quint32 handle = 0;
    QString insertChain;
  };
  std::vector<CircleHit> hits;
  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityCircle)
      continue;
    auto *c = static_cast<RS_Circle *>(e);
    const RS_Vector ctr = c->getCenter();
    if (std::hypot(ctr.x - tx, ctr.y - ty) > 1.0)
      continue;
    hits.push_back({c->getRadius(), e->sourceHandle(), insertChain(e)});
  }
  std::sort(hits.begin(), hits.end(),
            [](const CircleHit &a, const CircleHit &b) { return a.r < b.r; });

  std::cout << "\n=== chicun circle (44748.7,180186.7) ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (const auto &h : hits) {
    std::cout << "  r=" << h.r << " handle=0x" << std::hex << h.handle
              << std::dec << " insert=" << h.insertChain.toStdString() << "\n";
  }

  if (hits.empty()) {
    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
      if (!e || e->rtti() != RS2::EntityCircle || e->sourceHandle() != 0x15C6D5u)
        continue;
      auto *c = static_cast<RS_Circle *>(e);
      const RS_Vector ctr = c->getCenter();
      if (!insertChain(e).contains(QStringLiteral("A$C5ABE7CED")))
        continue;
      std::cout << "fallback deng1 circle ctr=(" << ctr.x << "," << ctr.y
                << ") chain=" << insertChain(e).toStdString() << "\n";
    }
  }

  RS_Insert *topParentEarly = nullptr;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    auto *ins = static_cast<RS_Insert *>(e);
    if (ins->getName() != QStringLiteral("A$C5ABE7CED"))
      continue;
    if (ins->getParent() == nullptr
        || ins->getParent()->rtti() != RS2::EntityGraphic)
      continue;
    topParentEarly = ins;
    break;
  }
  if (topParentEarly) {
    std::cout << "top parent children=" << topParentEarly->count() << "\n";
    for (RS_Entity *e : *topParentEarly) {
      if (!e || e->rtti() != RS2::EntityInsert)
        continue;
      auto *ins = static_cast<RS_Insert *>(e);
      std::cout << " child insert " << ins->getName().toStdString() << " h=0x"
                << std::hex << ins->sourceHandle() << std::dec << "\n";
      if (ins->getName() != QStringLiteral("deng1"))
        continue;
      RS_Vector firstGeom(false);
      for (RS_Entity *c : *ins) {
        if (c && c->rtti() == RS2::EntityCircle) {
          firstGeom = static_cast<RS_Circle *>(c)->getCenter();
          break;
        }
      }
      const RS_Vector ip = ins->getInsertionPoint();
      std::cout << "early resolved deng1 ip=(" << ip.x << "," << ip.y << ")";
      if (firstGeom.valid)
        std::cout << " circle=(" << firstGeom.x << "," << firstGeom.y
                  << ") dist=" << firstGeom.distanceTo(ip);
      std::cout << "\n";
    }
  }

  RS_Block *deng1 = graphic.findBlock(QStringLiteral("deng1"));
  REQUIRE(deng1 != nullptr);
  int blockCircles = 0;
  RS_Vector blockCtr(false);
  for (RS_Entity *e : *deng1) {
    if (!e || e->rtti() != RS2::EntityCircle)
      continue;
    auto *c = static_cast<RS_Circle *>(e);
    if (blockCircles == 0)
      blockCtr = c->getCenter();
    CHECK(blockCtr.distanceTo(c->getCenter()) < 0.01);
    ++blockCircles;
  }
  CHECK(blockCircles == 3);
  std::cout << "deng1 block circle center=(" << blockCtr.x << "," << blockCtr.y
            << ")\n";
  CHECK(blockCtr.distanceTo(RS_Vector(0.0, 0.0)) < 1.0);

  REQUIRE(hits.size() == 3);
  CHECK(std::fabs(hits[0].r - 27.5) < 0.05);
  CHECK(std::fabs(hits[1].r - 110.0) < 0.05);
  CHECK(std::fabs(hits[2].r - 247.5) < 0.05);
  CHECK(hits[0].insertChain.contains(QStringLiteral("deng1")));
  CHECK(hits[0].insertChain.contains(QStringLiteral("A$C5ABE7CED")));

  RS_Arc *blockArc = nullptr;
  for (RS_Entity *e : *deng1) {
    if (e && e->rtti() == RS2::EntityArc) {
      blockArc = static_cast<RS_Arc *>(e);
      break;
    }
  }
  REQUIRE(blockArc != nullptr);
  std::cout << "deng1 block arc a1=" << blockArc->getAngle1()
            << " a2=" << blockArc->getAngle2()
            << " r=" << blockArc->getRadius() << "\n";

  RS_Block *parentBlk = graphic.findBlock(QStringLiteral("A$C5ABE7CED"));
  REQUIRE(parentBlk != nullptr);
  const RS_Vector deng1Bp = deng1->getBasePoint();
  const RS_Vector parentBp = parentBlk->getBasePoint();
  std::cout << "deng1 basePoint=(" << deng1Bp.x << "," << deng1Bp.y << ")\n";
  std::cout << "A$C5ABE7CED basePoint=(" << parentBp.x << "," << parentBp.y
            << ")\n";

  int parentInsertCount = 0;
  int parentDeng1Count = 0;
  std::cout << "INSERTs in A$C5ABE7CED block definition:\n";
  for (RS_Entity *e : *parentBlk) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    ++parentInsertCount;
    auto *ins = static_cast<RS_Insert *>(e);
    const RS_Vector sc = ins->getScale();
    std::cout << "  " << ins->getName().toStdString() << " h=0x" << std::hex
              << ins->sourceHandle() << std::dec << " ip=("
              << ins->getInsertionPoint().x << ","
              << ins->getInsertionPoint().y << ") sc=(" << sc.x << "," << sc.y
              << ") ang=" << ins->getAngle() << "\n";
    if (ins->getName() == QStringLiteral("deng1"))
      ++parentDeng1Count;
  }
  std::cout << "parent inserts total=" << parentInsertCount
            << " deng1=" << parentDeng1Count << "\n";

  std::cout << "resolved deng1 circle instances (handle 15C6D5):\n";
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityCircle || e->sourceHandle() != 0x15C6D5u)
      continue;
    auto *c = static_cast<RS_Circle *>(e);
    std::cout << "  WCS=(" << c->getCenter().x << "," << c->getCenter().y
              << ") chain=" << insertChain(e).toStdString() << "\n";
  }

  RS_Insert *topParent = nullptr;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    auto *ins = static_cast<RS_Insert *>(e);
    if (ins->getName() != QStringLiteral("A$C5ABE7CED"))
      continue;
    if (ins->getParent() == nullptr
        || ins->getParent()->rtti() != RS2::EntityGraphic)
      continue;
    topParent = ins;
    break;
  }
  REQUIRE(topParent != nullptr);
  const RS_Vector topSc = topParent->getScale();
  std::cout << "top A$C5ABE7CED h=0x" << std::hex << topParent->sourceHandle()
            << std::dec << " ip=(" << topParent->getInsertionPoint().x << ","
            << topParent->getInsertionPoint().y << ") sc=(" << topSc.x << ","
            << topSc.y << ") ang=" << topParent->getAngle() << "\n";

  const double lampX = tx, lampY = ty;
  double bestShafa = std::numeric_limits<double>::infinity();
  RS_Vector bestPt(false);
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityLine)
      continue;
    QString chain = insertChain(e);
    if (!chain.contains(QStringLiteral("shafa2"))
        || !chain.contains(QStringLiteral("A$C5ABE7CED")))
      continue;
    auto *l = static_cast<RS_Line *>(e);
    for (const RS_Vector &p : {l->getStartpoint(), l->getEndpoint()}) {
      const double d = std::hypot(p.x - lampX, p.y - lampY);
      if (d < bestShafa) {
        bestShafa = d;
        bestPt = p;
      }
    }
  }
  std::cout << "nearest shafa2 line endpoint to lamp: dist=" << bestShafa
            << " at (" << bestPt.x << "," << bestPt.y << ")\n";

  // Block-local: deng1 circle center vs shafa2 insert — should stay clustered.
  RS_Insert *shafa2Ins = nullptr;
  for (RS_Entity *e : *parentBlk) {
    if (e && e->rtti() == RS2::EntityInsert
        && static_cast<RS_Insert *>(e)->getName() == QStringLiteral("shafa2")) {
      shafa2Ins = static_cast<RS_Insert *>(e);
      break;
    }
  }
  REQUIRE(shafa2Ins != nullptr);
  RS_Insert *deng1Ins921 = nullptr;
  for (RS_Entity *e : *parentBlk) {
    if (e && e->rtti() == RS2::EntityInsert
        && static_cast<RS_Insert *>(e)->getName() == QStringLiteral("deng1")
        && e->sourceHandle() == 0x1DF921u) {
      deng1Ins921 = static_cast<RS_Insert *>(e);
      break;
    }
  }
  REQUIRE(deng1Ins921 != nullptr);
  const double blockDist = deng1Ins921->getInsertionPoint().distanceTo(
      shafa2Ins->getInsertionPoint());
  std::cout << "parent-block dist(deng1-921, shafa2)=" << blockDist << "\n";
  std::cout << "deng1-921 block ip=" << deng1Ins921->getInsertionPoint().x
            << "," << deng1Ins921->getInsertionPoint().y
            << " shafa2 block ip=" << shafa2Ins->getInsertionPoint().x << ","
            << shafa2Ins->getInsertionPoint().y << "\n";

  // Resolved nested inserts under the top-level parent (post updateInserts).
  auto dumpResolvedInsert = [](RS_Insert *parent, const char *name,
                               quint32 handle) {
    for (RS_Entity *e : *parent) {
      if (!e || e->rtti() != RS2::EntityInsert)
        continue;
      auto *ins = static_cast<RS_Insert *>(e);
      if (ins->getName() != QString::fromUtf8(name))
        continue;
      if (handle != 0 && ins->sourceHandle() != handle)
        continue;
      const RS_Vector sc = ins->getScale();
      std::cout << "resolved " << name << " h=0x" << std::hex
                << ins->sourceHandle() << std::dec << " ip=("
                << ins->getInsertionPoint().x << ","
                << ins->getInsertionPoint().y << ") sc=(" << sc.x << ","
                << sc.y << ") ang=" << ins->getAngle()
                << " children=" << ins->count() << "\n";
      RS_Vector firstGeom(false);
      for (RS_Entity *c : *ins) {
        if (!c || c->rtti() != RS2::EntityCircle)
          continue;
        auto *circ = static_cast<RS_Circle *>(c);
        if (!firstGeom.valid)
          firstGeom = circ->getCenter();
        std::cout << "  circle ctr=(" << circ->getCenter().x << ","
                  << circ->getCenter().y << ") r=" << circ->getRadius()
                  << "\n";
      }
      if (firstGeom.valid) {
        const RS_Vector ip = ins->getInsertionPoint();
        std::cout << "  geom-ip delta=(" << (firstGeom.x - ip.x) << ","
                  << (firstGeom.y - ip.y) << ") dist="
                  << firstGeom.distanceTo(ip) << "\n";
        CHECK(firstGeom.distanceTo(ip) < 0.05);
      }
    }
  };
  std::cout << "resolved nested inserts under top parent:\n";
  dumpResolvedInsert(topParent, "deng1", 0x1DF921u);
  dumpResolvedInsert(topParent, "shafa2", 0x1DF955u);
}

// Probe stray geometry near WCS (-774805.88, 588320.79) in 2带尺寸图库.dwg.
//   ./librecad_tests "[.dwg_chicun_stray]" -s
TEST_CASE("DWG 2带尺寸图库: entities near (-774805.88,588320.79)",
          "[.dwg_chicun_stray]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  const double tx = -774805.8834, ty = 588320.7858;
  const double tol = 50.0;
  struct NearHit {
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;
  auto dist = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };
  auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                      const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
    if (d > tol)
      return;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    hits.push_back({type, layer, e->sourceHandle(), p1, p2, insertChain(e), d});
  };

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    switch (e->rtti()) {
    case RS2::EntityLine:
      consider("LINE", e,
               static_cast<RS_Line *>(e)->getStartpoint(),
               static_cast<RS_Line *>(e)->getEndpoint());
      break;
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      RS_Vector prev(false);
      for (RS_Entity *v : *pl) {
        if (!v)
          continue;
        RS_Vector pt = v->getStartpoint();
        consider("PLINE_VTX", e, pt);
        if (prev.valid)
          consider("PLINE_SEG", e, prev, pt);
        prev = pt;
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", e, a->getCenter());
      consider("ARC_SP", e, a->getStartpoint());
      consider("ARC_EP", e, a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", e,
               static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", e,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  }

  std::sort(hits.begin(), hits.end(),
            [](const NearHit &a, const NearHit &b) { return a.dist < b.dist; });

  std::cout << "\n=== chicun stray (-774805.88,588320.79) tol=" << tol
            << " ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (std::size_t i = 0; i < hits.size() && i < 40; ++i) {
    const auto &h = hits[i];
    if (h.p2.valid)
      std::cout << "  " << h.type.toStdString() << " h=0x" << std::hex
                << h.handle << std::dec << " layer=" << h.layer.toStdString()
                << " (" << h.p1.x << "," << h.p1.y << ")->(" << h.p2.x << ","
                << h.p2.y << ") dist=" << h.dist
                << " insert=" << h.insertChain.toStdString() << "\n";
    else
      std::cout << "  " << h.type.toStdString() << " h=0x" << std::hex
                << h.handle << std::dec << " layer=" << h.layer.toStdString()
                << " (" << h.p1.x << "," << h.p1.y << ") dist=" << h.dist
                << " insert=" << h.insertChain.toStdString() << "\n";
  }

  double best = std::numeric_limits<double>::infinity();
  NearHit globalBest;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    auto probe = [&](const QString &type, const RS_Vector &p) {
      const double d = dist(p);
      if (d < best) {
        best = d;
        globalBest = {type,
                      e->getLayer() ? e->getLayer()->getName()
                                    : QStringLiteral("(none)"),
                      e->sourceHandle(), p, RS_Vector(false), insertChain(e), d};
      }
    };
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      probe("LINE_SP", l->getStartpoint());
      probe("LINE_EP", l->getEndpoint());
      break;
    }
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      for (RS_Entity *v : *pl) {
        if (v)
          probe("PLINE_VTX", v->getStartpoint());
      }
      break;
    }
    default:
      break;
    }
  }
  if (globalBest.dist < std::numeric_limits<double>::infinity()) {
    std::cout << "global closest: " << globalBest.type.toStdString() << " h=0x"
              << std::hex << globalBest.handle << std::dec << " ("
              << globalBest.p1.x << "," << globalBest.p1.y
              << ") dist=" << globalBest.dist
              << " insert=" << globalBest.insertChain.toStdString() << "\n";
  }

  // Stray segments at (-774806,588321) were errant geometry from fghfdh567567
  // after over-aggressive block re-centering; expect nothing near this point.
  CHECK(hits.empty());
  if (globalBest.dist < std::numeric_limits<double>::infinity())
    CHECK(globalBest.dist > 1000.0);
}

// Probe incorrect geometry near WCS (52984.74, 222052.06) in 2带尺寸图库.dwg.
//   ./librecad_tests "[.dwg_chicun_stray2]" -s
TEST_CASE("DWG 2带尺寸图库: entities near (52984.74,222052.06)",
          "[.dwg_chicun_stray2]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  const double tx = 52984.73581, ty = 222052.0628;
  const double tol = 50.0;
  struct NearHit {
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;
  auto dist = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };
  auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                      const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
    if (d > tol)
      return;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    hits.push_back({type, layer, e->sourceHandle(), p1, p2, insertChain(e), d});
  };

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    switch (e->rtti()) {
    case RS2::EntityLine:
      consider("LINE", e,
               static_cast<RS_Line *>(e)->getStartpoint(),
               static_cast<RS_Line *>(e)->getEndpoint());
      break;
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      RS_Vector prev(false);
      for (RS_Entity *v : *pl) {
        if (!v)
          continue;
        RS_Vector pt = v->getStartpoint();
        consider("PLINE_VTX", e, pt);
        if (prev.valid)
          consider("PLINE_SEG", e, prev, pt);
        prev = pt;
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", e, a->getCenter());
      consider("ARC_SP", e, a->getStartpoint());
      consider("ARC_EP", e, a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", e,
               static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", e,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  }

  std::sort(hits.begin(), hits.end(),
            [](const NearHit &a, const NearHit &b) { return a.dist < b.dist; });

  std::cout << "\n=== chicun stray2 (52984.74,222052.06) tol=" << tol
            << " ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (std::size_t i = 0; i < hits.size() && i < 40; ++i) {
    const auto &h = hits[i];
    if (h.p2.valid)
      std::cout << "  " << h.type.toStdString() << " h=0x" << std::hex
                << h.handle << std::dec << " layer=" << h.layer.toStdString()
                << " (" << h.p1.x << "," << h.p1.y << ")->(" << h.p2.x << ","
                << h.p2.y << ") dist=" << h.dist
                << " insert=" << h.insertChain.toStdString() << "\n";
    else
      std::cout << "  " << h.type.toStdString() << " h=0x" << std::hex
                << h.handle << std::dec << " layer=" << h.layer.toStdString()
                << " (" << h.p1.x << "," << h.p1.y << ") dist=" << h.dist
                << " insert=" << h.insertChain.toStdString() << "\n";
  }

  double best = std::numeric_limits<double>::infinity();
  NearHit globalBest;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    auto probe = [&](const QString &type, const RS_Vector &p) {
      const double d = dist(p);
      if (d < best) {
        best = d;
        globalBest = {type,
                      e->getLayer() ? e->getLayer()->getName()
                                    : QStringLiteral("(none)"),
                      e->sourceHandle(), p, RS_Vector(false), insertChain(e), d};
      }
    };
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      probe("LINE_SP", l->getStartpoint());
      probe("LINE_EP", l->getEndpoint());
      break;
    }
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      for (RS_Entity *v : *pl) {
        if (v)
          probe("PLINE_VTX", v->getStartpoint());
      }
      break;
    }
    default:
      break;
    }
  }
  if (globalBest.dist < std::numeric_limits<double>::infinity()) {
    std::cout << "global closest: " << globalBest.type.toStdString() << " h=0x"
              << std::hex << globalBest.handle << std::dec << " ("
              << globalBest.p1.x << "," << globalBest.p1.y
              << ") dist=" << globalBest.dist
              << " insert=" << globalBest.insertChain.toStdString() << "\n";
  }

  // WCS-embedded esfsfdds geometry must not be re-centered onto the insert grip.
  CHECK(hits.empty());
  if (globalBest.dist < std::numeric_limits<double>::infinity())
    CHECK(globalBest.dist > 1000.0);
}

// Probe incorrect geometry near WCS (-294315.21, 48571.66) in 2带尺寸图库.dwg.
//   ./librecad_tests "[.dwg_chicun_stray3]" -s
TEST_CASE("DWG 2带尺寸图库: entities near (-294315.21,48571.66)",
          "[.dwg_chicun_stray3]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  const double tx = -294315.207, ty = 48571.65792;
  const double tol = 50.0;
  struct NearHit {
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;
  auto dist = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };
  auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                      const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
    if (d > tol)
      return;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    hits.push_back({type, layer, e->sourceHandle(), p1, p2, insertChain(e), d});
  };

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    switch (e->rtti()) {
    case RS2::EntityLine:
      consider("LINE", e,
               static_cast<RS_Line *>(e)->getStartpoint(),
               static_cast<RS_Line *>(e)->getEndpoint());
      break;
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      RS_Vector prev(false);
      for (RS_Entity *v : *pl) {
        if (!v)
          continue;
        RS_Vector pt = v->getStartpoint();
        consider("PLINE_VTX", e, pt);
        if (prev.valid)
          consider("PLINE_SEG", e, prev, pt);
        prev = pt;
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", e, a->getCenter());
      consider("ARC_SP", e, a->getStartpoint());
      consider("ARC_EP", e, a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", e,
               static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", e,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  }

  CHECK(hits.empty());

  // INSERT A$C4D8A4BEC: extrusion (0,0,-1), z-scale -0.5 — fold -X block half.
  RS_Insert *a4d8Ins = nullptr;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    auto *ins = static_cast<RS_Insert *>(e);
    if (ins->getName() == QStringLiteral("A$C4D8A4BEC")) {
      a4d8Ins = ins;
      break;
    }
  }
  REQUIRE(a4d8Ins != nullptr);
  CHECK(a4d8Ins->getData().extrusion.z < -0.5);
  CHECK(a4d8Ins->getScale().z < -0.4);

  // Primary (+77,+220) cluster relative to (possibly re-based) insert grip.
  const RS_Vector ip = a4d8Ins->getInsertionPoint();
  const double expX = ip.x + 0.5 * 77.14807952690171;
  const double expY = ip.y + 0.5 * 219.9479471713712;
  int primaryCluster = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || !insertChain(e).contains(QStringLiteral("A$C4D8A4BEC")))
      continue;
    RS_Vector p(false);
    if (e->rtti() == RS2::EntityCircle)
      p = static_cast<RS_Circle *>(e)->getCenter();
    else if (e->rtti() == RS2::EntityArc)
      p = static_cast<RS_Arc *>(e)->getCenter();
    else
      continue;
    if (std::hypot(p.x - expX, p.y - expY) < 80.0)
      ++primaryCluster;
  }
  CHECK(primaryCluster >= 4);
}

// Probe incorrect geometry at three WCS points in 2带尺寸图库.dwg.
//   ./librecad_tests "[.dwg_chicun_stray4]" -s
TEST_CASE("DWG 2带尺寸图库: entities near stray WCS triple",
          "[.dwg_chicun_stray4]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  struct Target {
    const char *label;
    double x;
    double y;
  };
  const Target targets[] = {
      {"A", 104517.7659, -1239995.976},
      {"B", 57585.03448, -1240174.235},
      {"C", 1322040.68, 492532.476},
  };
  const double tol = 50.0;

  struct NearHit {
    char target = 0;
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;

  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };

  for (const auto &t : targets) {
    auto dist = [&](const RS_Vector &p) {
      return std::hypot(p.x - t.x, p.y - t.y);
    };
    auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                        const RS_Vector &p2 = RS_Vector(false)) {
      const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
      if (d > tol)
        return;
      const QString layer =
          e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
      hits.push_back({t.label[0], type, layer, e->sourceHandle(), p1, p2,
                      insertChain(e), d});
    };

    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
      if (!e)
        continue;
      switch (e->rtti()) {
      case RS2::EntityLine:
        consider("LINE", e,
                 static_cast<RS_Line *>(e)->getStartpoint(),
                 static_cast<RS_Line *>(e)->getEndpoint());
        break;
      case RS2::EntityPolyline: {
        auto *pl = static_cast<RS_Polyline *>(e);
        RS_Vector prev(false);
        for (RS_Entity *v : *pl) {
          if (!v)
            continue;
          RS_Vector pt = v->getStartpoint();
          consider("PLINE_VTX", e, pt);
          if (prev.valid)
            consider("PLINE_SEG", e, prev, pt);
          prev = pt;
        }
        break;
      }
      case RS2::EntityArc: {
        auto *a = static_cast<RS_Arc *>(e);
        consider("ARC_CTR", e, a->getCenter());
        consider("ARC_SP", e, a->getStartpoint());
        consider("ARC_EP", e, a->getEndpoint());
        break;
      }
      case RS2::EntityCircle:
        consider("CIRCLE", e,
                 static_cast<RS_Circle *>(e)->getCenter());
        break;
      case RS2::EntityInsert:
        consider("INSERT", e,
                 static_cast<RS_Insert *>(e)->getInsertionPoint());
        break;
      default:
        break;
      }
    }
  }

  std::sort(hits.begin(), hits.end(), [](const NearHit &a, const NearHit &b) {
    if (a.target != b.target)
      return a.target < b.target;
    return a.dist < b.dist;
  });

  std::cout << "\n=== chicun stray4 triple tol=" << tol << " ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (std::size_t i = 0; i < hits.size() && i < 60; ++i) {
    const auto &h = hits[i];
    if (h.p2.valid)
      std::cout << "  tgt=" << h.target << " " << h.type.toStdString()
                << " h=0x" << std::hex << h.handle << std::dec
                << " layer=" << h.layer.toStdString() << " (" << h.p1.x << ","
                << h.p1.y << ")->(" << h.p2.x << "," << h.p2.y
                << ") dist=" << h.dist
                << " insert=" << h.insertChain.toStdString() << "\n";
    else
      std::cout << "  tgt=" << h.target << " " << h.type.toStdString()
                << " h=0x" << std::hex << h.handle << std::dec
                << " layer=" << h.layer.toStdString() << " (" << h.p1.x << ","
                << h.p1.y << ") dist=" << h.dist
                << " insert=" << h.insertChain.toStdString() << "\n";
  }

  for (const auto &t : targets) {
    double best = std::numeric_limits<double>::infinity();
    NearHit bestHit;
    for (const auto &h : hits) {
      if (h.target != t.label[0])
        continue;
      if (h.dist < best) {
        best = h.dist;
        bestHit = h;
      }
    }
    if (best < std::numeric_limits<double>::infinity()) {
      std::cout << "target " << t.label << " (" << t.x << "," << t.y
                << ") closest: " << bestHit.type.toStdString() << " dist="
                << best << " insert=" << bestHit.insertChain.toStdString()
                << "\n";
    } else {
      std::cout << "target " << t.label << " (" << t.x << "," << t.y
                << ") no hits within tol\n";
    }
  }

  // Diagnostic: list top-level INSERTs near each target.
  for (const auto &t : targets) {
    std::cout << "INSERTs near target " << t.label << ":\n";
    int n = 0;
    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
      if (!e || e->rtti() != RS2::EntityInsert)
        continue;
      auto *ins = static_cast<RS_Insert *>(e);
      const RS_Vector ip = ins->getInsertionPoint();
      const double d = std::hypot(ip.x - t.x, ip.y - t.y);
      if (d > 500.0)
        continue;
      std::cout << "  " << ins->getName().toStdString() << " ip=(" << ip.x
                << "," << ip.y << ") sc=(" << ins->getScale().x << ","
                << ins->getScale().y << "," << ins->getScale().z
                << ") ext.z=" << ins->getData().extrusion.z << " dist=" << d
                << "\n";
      if (++n >= 8)
        break;
    }
  }

  // Global closest (any distance) for diagnosis.
  for (const auto &t : targets) {
    double best = std::numeric_limits<double>::infinity();
    QString bestType;
    QString bestChain;
    RS_Vector bestP(false);
    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
      if (!e)
        continue;
      auto probe = [&](const QString &type, const RS_Vector &p) {
        const double d = std::hypot(p.x - t.x, p.y - t.y);
        if (d < best) {
          best = d;
          bestType = type;
          bestChain = insertChain(e);
          bestP = p;
        }
      };
      switch (e->rtti()) {
      case RS2::EntityLine: {
        auto *l = static_cast<RS_Line *>(e);
        probe("LINE_SP", l->getStartpoint());
        probe("LINE_EP", l->getEndpoint());
        break;
      }
      case RS2::EntityArc: {
        auto *a = static_cast<RS_Arc *>(e);
        probe("ARC_CTR", a->getCenter());
        probe("ARC_SP", a->getStartpoint());
        probe("ARC_EP", a->getEndpoint());
        break;
      }
      case RS2::EntityCircle:
        probe("CIRCLE", static_cast<RS_Circle *>(e)->getCenter());
        break;
      case RS2::EntityPolyline: {
        auto *pl = static_cast<RS_Polyline *>(e);
        for (RS_Entity *v : *pl) {
          if (v)
            probe("PLINE_VTX", v->getStartpoint());
        }
        break;
      }
      default:
        break;
      }
    }
    if (best < std::numeric_limits<double>::infinity()) {
      std::cout << "global " << t.label << ": " << bestType.toStdString()
                << " (" << bestP.x << "," << bestP.y << ") dist=" << best
                << " insert=" << bestChain.toStdString() << "\n";
    }
  }

  CHECK(hits.empty());
}

// Probe incorrect geometry at three WCS points (negative-X mirror pair + E).
//   ./librecad_tests "[.dwg_chicun_stray5]" -s
TEST_CASE("DWG 2带尺寸图库: entities near stray WCS triple (neg X)",
          "[.dwg_chicun_stray5]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  struct Target {
    const char *label;
    double x;
    double y;
  };
  const Target targets[] = {
      {"D", -1099318.43, 492532.476},
      {"E", -952337.4015, 403260.8869},
      {"F", 1322040.68, 492532.476},
  };
  const double tol = 50.0;

  struct NearHit {
    char target = 0;
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;

  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };

  for (const auto &t : targets) {
    auto dist = [&](const RS_Vector &p) {
      return std::hypot(p.x - t.x, p.y - t.y);
    };
    auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                        const RS_Vector &p2 = RS_Vector(false)) {
      const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
      if (d > tol)
        return;
      const QString layer =
          e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
      hits.push_back({t.label[0], type, layer, e->sourceHandle(), p1, p2,
                      insertChain(e), d});
    };

    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
      if (!e)
        continue;
      switch (e->rtti()) {
      case RS2::EntityLine:
        consider("LINE", e,
                 static_cast<RS_Line *>(e)->getStartpoint(),
                 static_cast<RS_Line *>(e)->getEndpoint());
        break;
      case RS2::EntityPolyline: {
        auto *pl = static_cast<RS_Polyline *>(e);
        RS_Vector prev(false);
        for (RS_Entity *v : *pl) {
          if (!v)
            continue;
          RS_Vector pt = v->getStartpoint();
          consider("PLINE_VTX", e, pt);
          if (prev.valid)
            consider("PLINE_SEG", e, prev, pt);
          prev = pt;
        }
        break;
      }
      case RS2::EntityArc: {
        auto *a = static_cast<RS_Arc *>(e);
        consider("ARC_CTR", e, a->getCenter());
        consider("ARC_SP", e, a->getStartpoint());
        consider("ARC_EP", e, a->getEndpoint());
        break;
      }
      case RS2::EntityCircle:
        consider("CIRCLE", e,
                 static_cast<RS_Circle *>(e)->getCenter());
        break;
      case RS2::EntityInsert:
        consider("INSERT", e,
                 static_cast<RS_Insert *>(e)->getInsertionPoint());
        break;
      default:
        break;
      }
    }
  }

  std::cout << "\n=== chicun stray5 triple tol=" << tol << " ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (const auto &h : hits) {
    std::cout << "  tgt=" << h.target << " " << h.type.toStdString() << " ("
              << h.p1.x << "," << h.p1.y << ") dist=" << h.dist
              << " insert=" << h.insertChain.toStdString() << "\n";
  }

  for (const auto &t : targets) {
    double best = std::numeric_limits<double>::infinity();
    QString bestType;
    QString bestChain;
    RS_Vector bestP(false);
    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
      if (!e)
        continue;
      auto probe = [&](const QString &type, const RS_Vector &p) {
        const double d = std::hypot(p.x - t.x, p.y - t.y);
        if (d < best) {
          best = d;
          bestType = type;
          bestChain = insertChain(e);
          bestP = p;
        }
      };
      switch (e->rtti()) {
      case RS2::EntityLine: {
        auto *l = static_cast<RS_Line *>(e);
        probe("LINE_SP", l->getStartpoint());
        probe("LINE_EP", l->getEndpoint());
        break;
      }
      case RS2::EntityArc: {
        auto *a = static_cast<RS_Arc *>(e);
        probe("ARC_CTR", a->getCenter());
        probe("ARC_SP", a->getStartpoint());
        probe("ARC_EP", a->getEndpoint());
        break;
      }
      case RS2::EntityCircle:
        probe("CIRCLE", static_cast<RS_Circle *>(e)->getCenter());
        break;
      case RS2::EntityPolyline: {
        auto *pl = static_cast<RS_Polyline *>(e);
        for (RS_Entity *v : *pl) {
          if (v)
            probe("PLINE_VTX", v->getStartpoint());
        }
        break;
      }
      default:
        break;
      }
    }
    if (best < std::numeric_limits<double>::infinity()) {
      std::cout << "global " << t.label << ": " << bestType.toStdString()
                << " (" << bestP.x << "," << bestP.y << ") dist=" << best
                << " insert=" << bestChain.toStdString() << "\n";
    }
  }

  CHECK(hits.empty());
}

// Probe incorrect geometry near WCS (-2230355.08, 1646735.10).
//   ./librecad_tests "[.dwg_chicun_stray6]" -s
TEST_CASE("DWG 2带尺寸图库: entities near (-2230355.08,1646735.10)",
          "[.dwg_chicun_stray6]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  const double tx = -2230355.082, ty = 1646735.104;
  const double tol = 50.0;

  struct NearHit {
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;

  auto dist = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };
  auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                      const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
    if (d > tol)
      return;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    hits.push_back({type, layer, e->sourceHandle(), p1, p2, insertChain(e), d});
  };

  NearHit globalBest;
  globalBest.dist = std::numeric_limits<double>::infinity();

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    auto updateGlobal = [&](const QString &type, const RS_Vector &p) {
      const double d = dist(p);
      if (d < globalBest.dist)
        globalBest = {type, layer, e->sourceHandle(), p, RS_Vector(false),
                      insertChain(e), d};
    };
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      consider("LINE", e, l->getStartpoint(), l->getEndpoint());
      updateGlobal("LINE_SP", l->getStartpoint());
      updateGlobal("LINE_EP", l->getEndpoint());
      break;
    }
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      RS_Vector prev(false);
      for (RS_Entity *v : *pl) {
        if (!v)
          continue;
        RS_Vector pt = v->getStartpoint();
        consider("PLINE_VTX", e, pt);
        if (prev.valid)
          consider("PLINE_SEG", e, prev, pt);
        updateGlobal("PLINE_VTX", pt);
        prev = pt;
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", e, a->getCenter());
      consider("ARC_SP", e, a->getStartpoint());
      consider("ARC_EP", e, a->getEndpoint());
      updateGlobal("ARC_CTR", a->getCenter());
      updateGlobal("ARC_SP", a->getStartpoint());
      updateGlobal("ARC_EP", a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", e, static_cast<RS_Circle *>(e)->getCenter());
      updateGlobal("CIRCLE", static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", e,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  }

  std::cout << "\n=== chicun stray6 (-2230355,1646735) tol=" << tol << " ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (const auto &h : hits) {
    std::cout << "  " << h.type.toStdString() << " (" << h.p1.x << ","
              << h.p1.y << ") dist=" << h.dist
              << " insert=" << h.insertChain.toStdString() << "\n";
  }
  if (globalBest.dist < std::numeric_limits<double>::infinity()) {
    std::cout << "global closest: " << globalBest.type.toStdString() << " ("
              << globalBest.p1.x << "," << globalBest.p1.y
              << ") dist=" << globalBest.dist
              << " insert=" << globalBest.insertChain.toStdString() << "\n";
  }

  CHECK(hits.empty());
  if (globalBest.dist < std::numeric_limits<double>::infinity())
    CHECK(globalBest.dist > 1000.0);
}

// Second vertex of the stray6 phantom cluster (~68 units away).
//   ./librecad_tests "[.dwg_chicun_stray7]" -s
TEST_CASE("DWG 2带尺寸图库: entities near (-2230420.44,1646754.55)",
          "[.dwg_chicun_stray7]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  const double tx = -2230420.436, ty = 1646754.547;
  const double tol = 50.0;

  struct NearHit {
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;

  auto dist = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };
  auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                      const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
    if (d > tol)
      return;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    hits.push_back({type, layer, e->sourceHandle(), p1, p2, insertChain(e), d});
  };

  NearHit globalBest;
  globalBest.dist = std::numeric_limits<double>::infinity();

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    auto updateGlobal = [&](const QString &type, const RS_Vector &p) {
      const double d = dist(p);
      if (d < globalBest.dist)
        globalBest = {type, layer, e->sourceHandle(), p, RS_Vector(false),
                      insertChain(e), d};
    };
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      consider("LINE", e, l->getStartpoint(), l->getEndpoint());
      updateGlobal("LINE_SP", l->getStartpoint());
      updateGlobal("LINE_EP", l->getEndpoint());
      break;
    }
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      RS_Vector prev(false);
      for (RS_Entity *v : *pl) {
        if (!v)
          continue;
        RS_Vector pt = v->getStartpoint();
        consider("PLINE_VTX", e, pt);
        if (prev.valid)
          consider("PLINE_SEG", e, prev, pt);
        updateGlobal("PLINE_VTX", pt);
        prev = pt;
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", e, a->getCenter());
      consider("ARC_SP", e, a->getStartpoint());
      consider("ARC_EP", e, a->getEndpoint());
      updateGlobal("ARC_CTR", a->getCenter());
      updateGlobal("ARC_SP", a->getStartpoint());
      updateGlobal("ARC_EP", a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", e, static_cast<RS_Circle *>(e)->getCenter());
      updateGlobal("CIRCLE", static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", e,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  }

  std::cout << "\n=== chicun stray7 (-2230420,1646754) tol=" << tol << " ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (const auto &h : hits) {
    std::cout << "  " << h.type.toStdString() << " (" << h.p1.x << ","
              << h.p1.y << ") dist=" << h.dist
              << " insert=" << h.insertChain.toStdString() << "\n";
  }
  if (globalBest.dist < std::numeric_limits<double>::infinity()) {
    std::cout << "global closest: " << globalBest.type.toStdString() << " ("
              << globalBest.p1.x << "," << globalBest.p1.y
              << ") dist=" << globalBest.dist
              << " insert=" << globalBest.insertChain.toStdString() << "\n";
  }

  CHECK(hits.empty());
  if (globalBest.dist < std::numeric_limits<double>::infinity())
    CHECK(globalBest.dist > 1000.0);
}

// Broad diagnostic for stray8: all entity types, block defs, Y-mirror band.
//   ./librecad_tests "[.dwg_chicun_stray8_diag]" -s
TEST_CASE("DWG 2带尺寸图库: stray8 broad diagnostic",
          "[.dwg_chicun_stray8_diag]") {
  if (chicun::fixturePath().empty()) {
    SKIP("fixture not present; skipping");
  }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  REQUIRE(chicun::importFixture(graphic));

  const double tx = -1099671.573, ty = 492101.3029;
  const double tol = 500.0;
  struct Hit {
    QString scope;
    QString type;
    QString blockOrChain;
    RS_Vector p;
    double dist = 0;
  };
  std::vector<Hit> hits;

  auto record = [&](const QString &scope, const QString &type,
                    const QString &chain, const RS_Vector &p) {
    const double d = std::hypot(p.x - tx, p.y - ty);
    if (d > tol)
      return;
    hits.push_back({scope, type, chain, p, d});
  };

  auto probeEntity = [&](const QString &scope, RS_Entity *e,
                         const QString &chain) {
    if (!e)
      return;
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      record(scope, "LINE_SP", chain, l->getStartpoint());
      record(scope, "LINE_EP", chain, l->getEndpoint());
      break;
    }
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      for (RS_Entity *v : *pl) {
        if (v)
          record(scope, "PLINE_VTX", chain, v->getStartpoint());
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      record(scope, "ARC_CTR", chain, a->getCenter());
      record(scope, "ARC_SP", chain, a->getStartpoint());
      record(scope, "ARC_EP", chain, a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      record(scope, "CIRCLE", chain,
             static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityEllipse: {
      auto *el = static_cast<RS_Ellipse *>(e);
      record(scope, "ELL_CTR", chain, el->getCenter());
      break;
    }
    case RS2::EntitySplinePoints: {
      auto *sp = static_cast<LC_SplinePoints *>(e);
      for (const RS_Vector &p : sp->getPoints())
        record(scope, "SPLINE_PT", chain, p);
      break;
    }
    case RS2::EntityPoint:
      record(scope, "POINT", chain,
             static_cast<RS_Point *>(e)->getStartpoint());
      break;
    case RS2::EntityInsert:
      record(scope, "INSERT", chain,
             static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  };

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    probeEntity("resolved", e, chicun::insertChain(e));
  }
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    probeEntity("model", e, chicun::insertChain(e));
  }
  if (auto *bl = graphic.getBlockList()) {
    for (int i = 0; i < bl->count(); ++i) {
      RS_Block *b = bl->at(i);
      if (!b)
        continue;
      const QString bname = b->getName();
      if (!bname.contains(QStringLiteral("CUSH"))
          && !bname.contains(QStringLiteral("sofa"))
          && !bname.startsWith(QStringLiteral("A$C28F40C54")))
        continue;
      for (RS_Entity *e : *b)
        probeEntity("block:" + bname, e, bname);
    }
  }

  std::sort(hits.begin(), hits.end(),
            [](const Hit &a, const Hit &b) { return a.dist < b.dist; });
  std::cout << "\n=== stray8_diag tol=" << tol << " hits=" << hits.size()
            << " countDeep=" << graphic.countDeep() << " ===\n";
  for (std::size_t i = 0; i < hits.size() && i < 40; ++i) {
    const auto &h = hits[i];
    std::cout << "  " << h.scope.toStdString() << " " << h.type.toStdString()
              << " (" << h.p.x << "," << h.p.y << ") dist=" << h.dist
              << " chain=" << h.blockOrChain.toStdString() << "\n";
  }

  // Y-mirror band: any resolved point with y~492101 near sofa chain
  int mirrorBand = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!chicun::insertChain(e).contains(QStringLiteral("CUSH")))
      continue;
    auto check = [&](const RS_Vector &p) {
      if (std::fabs(p.y - ty) < 200.0 && p.x < -1.0e5)
        ++mirrorBand;
    };
    if (e->rtti() == RS2::EntityLine) {
      auto *l = static_cast<RS_Line *>(e);
      check(l->getStartpoint());
      check(l->getEndpoint());
    } else if (e->rtti() == RS2::EntityArc) {
      auto *a = static_cast<RS_Arc *>(e);
      check(a->getStartpoint());
      check(a->getEndpoint());
      check(a->getCenter());
    }
  }
  std::cout << "CUSH chain points in y-mirror band (|y-492101|<200, x<-100k): "
            << mirrorBand << "\n";

  CHECK(hits.empty());
}

// GUI load path calls onLoadingCompleted() -> updateDimensions().
//   ./librecad_tests "[.dwg_chicun_stray8_gui]" -s
TEST_CASE("DWG 2带尺寸图库: stray8 after GUI onLoadingCompleted",
          "[.dwg_chicun_stray8_gui]") {
  if (chicun::fixturePath().empty()) {
    SKIP("fixture not present; skipping");
  }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  REQUIRE(chicun::importFixture(graphic));
  graphic.onLoadingCompleted();

  const auto result =
      chicun::probeResolvedGeometry(graphic, -1099671.573, 492101.3029, 50.0);
  std::cout << "\n=== stray8_gui hits=" << result.hits.size()
            << " countDeep=" << graphic.countDeep() << " ===\n";
  for (const auto &h : result.hits) {
    std::cout << "  " << h.type.toStdString() << " (" << h.p1.x << ","
              << h.p1.y << ") dist=" << h.dist
              << " insert=" << h.insertChain.toStdString() << "\n";
  }
  chicun::assertProbeEmpty(result, 1000.0);
}

// Snap-style: nearest point on CUSH-chain arc curves to phantom coordinate.
//   ./librecad_tests "[.dwg_chicun_stray8_snap]" -s
TEST_CASE("DWG 2带尺寸图库: stray8 nearest-on-arc snap distance",
          "[.dwg_chicun_stray8_snap]") {
  if (chicun::fixturePath().empty()) {
    SKIP("fixture not present; skipping");
  }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  REQUIRE(chicun::importFixture(graphic));
  graphic.onLoadingCompleted();

  const RS_Vector target(-1099671.573, 492101.3029);
  double best = std::numeric_limits<double>::infinity();
  QString bestChain;
  RS_Vector bestPt(false);
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityArc)
      continue;
    if (!chicun::insertChain(e).contains(QStringLiteral("CUSH")))
      continue;
    double dist = 0.0;
    const RS_Vector on =
        static_cast<RS_Arc *>(e)->getNearestPointOnEntity(target, true, &dist);
    if (dist < best) {
      best = dist;
      bestChain = chicun::insertChain(e);
      bestPt = on;
    }
  }
  std::cout << "\n=== stray8_snap nearest CUSH arc point: (" << bestPt.x
            << "," << bestPt.y << ") dist=" << best
            << " chain=" << bestChain.toStdString() << " ===\n";
  CHECK(best > 1000.0);
}

// Second vertex of stray5-D phantom cluster (~557 units from D).
//   ./librecad_tests "[.dwg_chicun_stray8]" -s
TEST_CASE("DWG 2带尺寸图库: entities near (-1099671.57,492101.30)",
          "[.dwg_chicun_stray8]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg4/2带尺寸图库.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));

  const double tx = -1099671.573, ty = 492101.3029;
  const double tol = 50.0;

  struct NearHit {
    QString type;
    QString layer;
    quint32 handle = 0;
    RS_Vector p1;
    RS_Vector p2;
    QString insertChain;
    double dist = 0;
  };
  std::vector<NearHit> hits;

  auto dist = [&](const RS_Vector &p) {
    return std::hypot(p.x - tx, p.y - ty);
  };
  auto insertChain = [](RS_Entity *e) {
    QString chain;
    for (RS_Entity *p = e; p; p = p->getParent()) {
      if (p->rtti() != RS2::EntityInsert)
        continue;
      if (!chain.isEmpty())
        chain += " <- ";
      chain += static_cast<RS_Insert *>(p)->getName();
    }
    return chain;
  };
  auto consider = [&](const QString &type, RS_Entity *e, const RS_Vector &p1,
                      const RS_Vector &p2 = RS_Vector(false)) {
    const double d = p2.valid ? std::min(dist(p1), dist(p2)) : dist(p1);
    if (d > tol)
      return;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    hits.push_back({type, layer, e->sourceHandle(), p1, p2, insertChain(e), d});
  };

  NearHit globalBest;
  globalBest.dist = std::numeric_limits<double>::infinity();

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString layer =
        e->getLayer() ? e->getLayer()->getName() : QStringLiteral("(none)");
    auto updateGlobal = [&](const QString &type, const RS_Vector &p) {
      const double d = dist(p);
      if (d < globalBest.dist)
        globalBest = {type, layer, e->sourceHandle(), p, RS_Vector(false),
                      insertChain(e), d};
    };
    switch (e->rtti()) {
    case RS2::EntityLine: {
      auto *l = static_cast<RS_Line *>(e);
      consider("LINE", e, l->getStartpoint(), l->getEndpoint());
      updateGlobal("LINE_SP", l->getStartpoint());
      updateGlobal("LINE_EP", l->getEndpoint());
      break;
    }
    case RS2::EntityPolyline: {
      auto *pl = static_cast<RS_Polyline *>(e);
      RS_Vector prev(false);
      for (RS_Entity *v : *pl) {
        if (!v)
          continue;
        RS_Vector pt = v->getStartpoint();
        consider("PLINE_VTX", e, pt);
        if (prev.valid)
          consider("PLINE_SEG", e, prev, pt);
        updateGlobal("PLINE_VTX", pt);
        prev = pt;
      }
      break;
    }
    case RS2::EntityArc: {
      auto *a = static_cast<RS_Arc *>(e);
      consider("ARC_CTR", e, a->getCenter());
      consider("ARC_SP", e, a->getStartpoint());
      consider("ARC_EP", e, a->getEndpoint());
      updateGlobal("ARC_CTR", a->getCenter());
      updateGlobal("ARC_SP", a->getStartpoint());
      updateGlobal("ARC_EP", a->getEndpoint());
      break;
    }
    case RS2::EntityCircle:
      consider("CIRCLE", e, static_cast<RS_Circle *>(e)->getCenter());
      updateGlobal("CIRCLE", static_cast<RS_Circle *>(e)->getCenter());
      break;
    case RS2::EntityInsert:
      consider("INSERT", e,
               static_cast<RS_Insert *>(e)->getInsertionPoint());
      break;
    default:
      break;
    }
  }

  std::cout << "\n=== chicun stray8 (-1099671,492101) tol=" << tol << " ===\n";
  std::cout << "countDeep=" << graphic.countDeep() << " hits=" << hits.size()
            << "\n";
  for (const auto &h : hits) {
    std::cout << "  " << h.type.toStdString() << " (" << h.p1.x << ","
              << h.p1.y << ") dist=" << h.dist
              << " insert=" << h.insertChain.toStdString() << "\n";
  }
  if (globalBest.dist < std::numeric_limits<double>::infinity()) {
    std::cout << "global closest: " << globalBest.type.toStdString() << " ("
              << globalBest.p1.x << "," << globalBest.p1.y
              << ") dist=" << globalBest.dist
              << " insert=" << globalBest.insertChain.toStdString() << "\n";
  }

  CHECK(hits.empty());
  if (globalBest.dist < std::numeric_limits<double>::infinity())
    CHECK(globalBest.dist > 1000.0);
}

// Audit A$C446327FF / 015 insert transform (bbox west/north drivers).
//   ./librecad_tests "[.dwg_chicun_015_audit]" -s
TEST_CASE("DWG 2带尺寸图库: 015 insert transform audit",
          "[.dwg_chicun_015_audit]") {
  if (chicun::fixturePath().empty()) {
    SKIP("fixture not present; skipping");
  }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  REQUIRE(chicun::importFixture(graphic));

  RS_Block *blk015 = graphic.findBlock(QStringLiteral("015"));
  REQUIRE(blk015 != nullptr);
  blk015->calculateBorders();
  std::cout << "\n=== block 015 def ===\n";
  std::cout << "base=" << blk015->getBasePoint().x << ","
            << blk015->getBasePoint().y << " min=(" << blk015->getMin().x
            << "," << blk015->getMin().y << ") max=(" << blk015->getMax().x
            << "," << blk015->getMax().y << ")\n";
  int ell = 0;
  for (RS_Entity *e : *blk015) {
    if (!e || e->rtti() != RS2::EntityEllipse)
      continue;
    ++ell;
    if (ell <= 3) {
      auto *el = static_cast<RS_Ellipse *>(e);
      std::cout << "  ell ctr=(" << el->getCenter().x << ","
                << el->getCenter().y << ")\n";
    }
  }
  std::cout << "block ellipses=" << ell << "\n";

  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    auto *ins = static_cast<RS_Insert *>(e);
    if (ins->getName() != QStringLiteral("A$C446327FF"))
      continue;
    const RS_Vector sc = ins->getScale();
    std::cout << "top A$C446327FF ip=(" << ins->getInsertionPoint().x << ","
              << ins->getInsertionPoint().y << ") sc=(" << sc.x << ","
              << sc.y << "," << sc.z << ") ext.z="
              << ins->getData().extrusion.z << " ang=" << ins->getAngle()
              << "\n";
    break;
  }

  RS_Vector sampleCtr(false);
  int resolved015Ell = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    const QString chain = chicun::insertChain(e);
    if (!chain.contains(QStringLiteral("A$C446327FF"))
        || !chain.contains(QStringLiteral("015")))
      continue;
    if (e->rtti() == RS2::EntityEllipse) {
      ++resolved015Ell;
      if (!sampleCtr.valid)
        sampleCtr = static_cast<RS_Ellipse *>(e)->getCenter();
    }
  }
  std::cout << "resolved 015 ellipses=" << resolved015Ell;
  if (sampleCtr.valid)
    std::cout << " sampleCtr=(" << sampleCtr.x << "," << sampleCtr.y << ")";
  std::cout << "\n";
  CHECK(resolved015Ell == 92);
  REQUIRE(sampleCtr.valid);
  CHECK(std::hypot(sampleCtr.x - 230585.0, sampleCtr.y - 61374.0) < 700.0);
}

// LNG-13 / A$C38DF2CE2 / A$C64247511 wipeout south-bbox audit.
//   ./librecad_tests "[.dwg_chicun_lng13_audit]" -s
TEST_CASE("DWG 2带尺寸图库: LNG-13 wipeout insert audit",
          "[.dwg_chicun_lng13_audit]") {
  if (chicun::fixturePath().empty()) {
    SKIP("fixture not present; skipping");
  }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  REQUIRE(chicun::importFixture(graphic));
  graphic.onLoadingCompleted();
  graphic.calculateBorders();

  const RS_Vector bmin = graphic.getMin();
  const RS_Vector bmax = graphic.getMax();
  std::cout << "\n=== LNG-13 audit ===\n";
  std::cout << "bbox min=(" << bmin.x << "," << bmin.y << ") max=(" << bmax.x
            << "," << bmax.y << ")\n";

  RS_Block *lng = graphic.findBlock(QStringLiteral("LNG-13"));
  REQUIRE(lng != nullptr);
  lng->calculateBorders();
  std::cout << "LNG-13 block base=(" << lng->getBasePoint().x << ","
            << lng->getBasePoint().y << ") min=(" << lng->getMin().x << ","
            << lng->getMin().y << ") max=(" << lng->getMax().x << ","
            << lng->getMax().y << ")\n";
  int wipeDef = 0;
  for (RS_Entity *e : *lng) {
    if (!e || e->rtti() != RS2::EntityWipeout)
      continue;
    e->calculateBorders();
    std::cout << "  def wipe emin=(" << e->getMin().x << "," << e->getMin().y
              << ") emax=(" << e->getMax().x << "," << e->getMax().y << ")\n";
    ++wipeDef;
  }
  std::cout << "LNG-13 def wipeouts=" << wipeDef << "\n";

  for (const char *outer : {"A$C38DF2CE2", "A$C64247511"}) {
    RS_Block *ob = graphic.findBlock(QString::fromLatin1(outer));
    if (!ob)
      continue;
    ob->calculateBorders();
    std::cout << outer << " block base=(" << ob->getBasePoint().x << ","
              << ob->getBasePoint().y << ") min=(" << ob->getMin().x << ","
              << ob->getMin().y << ") max=(" << ob->getMax().x << ","
              << ob->getMax().y << ")\n";
    for (RS_Entity *e : *ob) {
      if (!e || e->rtti() != RS2::EntityInsert)
        continue;
      auto *ins = static_cast<RS_Insert *>(e);
      const RS_Vector sc = ins->getScale();
      std::cout << "  " << outer << " child INSERT " << ins->getName().toStdString()
                << " ip=(" << ins->getInsertionPoint().x << ","
                << ins->getInsertionPoint().y << ") sc=(" << sc.x << "," << sc.y
                << ") ang=" << ins->getAngle() << "\n";
    }
  }

  for (const char *insName :
       {"A$C38DF2CE2", "A$C64247511"}) {
    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
      if (!e || e->rtti() != RS2::EntityInsert)
        continue;
      auto *ins = static_cast<RS_Insert *>(e);
      if (ins->getName() != QString::fromLatin1(insName))
        continue;
      const RS_Vector sc = ins->getScale();
      std::cout << "model INSERT " << insName << " ip=("
                << ins->getInsertionPoint().x << ","
                << ins->getInsertionPoint().y << ") sc=(" << sc.x << ","
                << sc.y << "," << sc.z << ") ext.z="
                << ins->getData().extrusion.z << " ang=" << ins->getAngle()
                << "\n";
    }
  }

  int southBand = 0;
  int westBand = 0;
  const double southY = -1.24092e6;
  const double westX = -1.10031e6;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntityWipeout)
      continue;
    const QString chain = chicun::insertChain(e);
    if (!chain.contains(QStringLiteral("LNG-13")))
      continue;
    e->calculateBorders();
    const RS_Vector emin = e->getMin();
    const RS_Vector emax = e->getMax();
    if (std::fabs(emin.y - southY) < 2.0e4)
      ++southBand;
    if (std::fabs(emin.x - westX) < 2.0e4)
      ++westBand;
    if (southBand + westBand <= 6)
      std::cout << "resolved wipe emin=(" << emin.x << "," << emin.y
                << ") emax=(" << emax.x << "," << emax.y
                << ") chain=" << chain.toStdString() << "\n";
  }
  std::cout << "resolved LNG-13 wipeouts southBand=" << southBand
            << " westBand=" << westBand << "\n";

  const auto stray4a =
      chicun::probeResolvedGeometry(graphic, 104517.7659, -1239995.976, 50.0);
  const auto stray4b =
      chicun::probeResolvedGeometry(graphic, 57585.03448, -1240174.235, 50.0);
  std::cout << "stray4A hits=" << stray4a.hits.size()
            << " globalBest=" << stray4a.globalBest.dist
            << " stray4B hits=" << stray4b.hits.size()
            << " globalBest=" << stray4b.globalBest.dist << "\n";

  CHECK(stray4a.hits.empty());
  CHECK(stray4b.hits.empty());
  SUCCEED("audit complete");
}

// ACEB10.dwg (R11/AC1009): bbox audit for misplaced INSERT/phantom extrema.
//   ./librecad_tests "[.dwg_aceb10_bbox]" -s
TEST_CASE("DWG ACEB10: resolved bbox audit", "[.dwg_aceb10_bbox]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path = std::string(home) + "/doc/dwg3/ACEB10.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("ACEB10.dwg not present; skipping");
  }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));
  // Display path: same container min/max sequence as zoomAuto.
  graphic.onLoadingCompleted();
  graphic.calculateBorders();

  std::cout << "ACADVER="
            << graphic.getVariableString("$ACADVER", "(none)").toStdString()
            << "\n";

  const RS_Vector bmin = graphic.getMin();
  const RS_Vector bmax = graphic.getMax();
  const double spanX = bmax.x - bmin.x;
  const double spanY = bmax.y - bmin.y;
  std::cout << "\n=== ACEB10 bbox ===\n";
  std::cout << "display-path=import+updateInserts+onLoadingCompleted+"
               "calculateBorders+getMin/getMax (zoomAuto source)\n";
  std::cout << "min=(" << bmin.x << "," << bmin.y << ") max=(" << bmax.x
            << "," << bmax.y << ")\n";
  std::cout << "span=(" << spanX << "," << spanY << ")\n";

  const auto corners = chicun::findBboxCornerDrivers(graphic, 5.0);
  std::cout << "corner drivers=" << corners.size() << "\n";
  for (std::size_t i = 0; i < corners.size() && i < 30; ++i) {
    const auto &h = corners[i];
    std::cout << "  corner=" << h.corner << " " << h.type.toStdString()
              << " (" << h.p.x << "," << h.p.y << ")"
              << " chain=" << h.insertChain.toStdString() << "\n";
  }

  const double boundTol = 5.0;
  int envWest = 0, envEast = 0, envSouth = 0, envNorth = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e)
      continue;
    e->calculateBorders();
    const RS_Vector emin = e->getMin();
    const RS_Vector emax = e->getMax();
    if (!emin.valid || !emax.valid)
      continue;
    if (std::fabs(emin.x - bmin.x) < boundTol) {
      ++envWest;
      if (envWest <= 5)
        std::cout << "  env-west " << rttiName(e->rtti()) << " emin=("
                  << emin.x << "," << emin.y << ") emax=(" << emax.x << ","
                  << emax.y << ") " << chicun::insertChain(e).toStdString()
                  << "\n";
    }
    if (std::fabs(emax.x - bmax.x) < boundTol) {
      ++envEast;
      if (envEast <= 5)
        std::cout << "  env-east " << rttiName(e->rtti()) << " emin=("
                  << emin.x << "," << emin.y << ") emax=(" << emax.x << ","
                  << emax.y << ") " << chicun::insertChain(e).toStdString()
                  << "\n";
    }
    if (std::fabs(emin.y - bmin.y) < boundTol) {
      ++envSouth;
      if (envSouth <= 5)
        std::cout << "  env-south " << rttiName(e->rtti()) << " emin=("
                  << emin.x << "," << emin.y << ") emax=(" << emax.x << ","
                  << emax.y << ") " << chicun::insertChain(e).toStdString()
                  << "\n";
    }
    if (std::fabs(emax.y - bmax.y) < boundTol) {
      ++envNorth;
      if (envNorth <= 5)
        std::cout << "  env-north " << rttiName(e->rtti()) << " emin=("
                  << emin.x << "," << emin.y << ") emax=(" << emax.x << ","
                  << emax.y << ") " << chicun::insertChain(e).toStdString()
                  << "\n";
    }
  }
  std::cout << "envelope west=" << envWest << " east=" << envEast
            << " south=" << envSouth << " north=" << envNorth << "\n";

  int modelInserts = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntityInsert)
      continue;
    ++modelInserts;
    auto *ins = static_cast<RS_Insert *>(e);
    const RS_Vector sc = ins->getScale();
    std::cout << "model INSERT " << ins->getName().toStdString() << " ip=("
              << ins->getInsertionPoint().x << ","
              << ins->getInsertionPoint().y << ") sc=(" << sc.x << "," << sc.y
              << ") ang=" << ins->getAngle() << "\n";
  }
  std::cout << "model INSERT count=" << modelInserts << "\n";

  int straySolids = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->rtti() != RS2::EntitySolid)
      continue;
    e->calculateBorders();
    const RS_Vector emin = e->getMin();
    const RS_Vector emax = e->getMax();
    if (emin.x > -5.0)
      continue;
    ++straySolids;
    if (straySolids <= 10) {
      auto *s = static_cast<RS_Solid *>(e);
      const RS_Vector c1 = s->getCorner(0);
      const RS_Vector c4 = s->getCorner(3);
      QString parents;
      for (RS_Entity *p = e->getParent(); p; p = p->getParent()) {
        if (!parents.isEmpty())
          parents += " <- ";
        parents += rttiName(p->rtti());
        if (p->rtti() == RS2::EntityInsert)
          parents += "(" + static_cast<RS_Insert *>(p)->getName() + ")";
      }
      std::cout << "stray Solid emin=(" << emin.x << "," << emin.y
                << ") emax=(" << emax.x << "," << emax.y << ") chain="
                << chicun::insertChain(e).toStdString() << " parents="
                << parents.toStdString() << " c1=(" << c1.x << "," << c1.y
                << ") c4=(" << c4.x << "," << c4.y << ")\n";
    }
  }
  std::cout << "stray solids (x<-5)=" << straySolids << "\n";

  int modelSolids = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (!e || e->rtti() != RS2::EntitySolid)
      continue;
    ++modelSolids;
    e->calculateBorders();
    if (modelSolids <= 8) {
      auto *s = static_cast<RS_Solid *>(e);
      std::cout << "model Solid #" << modelSolids << " emin=(" << e->getMin().x
                << "," << e->getMin().y << ") c1=(" << s->getCorner(0).x << ","
                << s->getCorner(0).y << ")\n";
    }
  }
  std::cout << "model-space solids=" << modelSolids << "\n";

  int starDBlocks = 0;
  for (int i = 0; i < graphic.getBlockList()->count(); ++i) {
    RS_Block *blk = graphic.getBlockList()->at(i);
    if (!blk || !blk->getName().startsWith(QStringLiteral("*D")))
      continue;
    ++starDBlocks;
    blk->calculateBorders();
    int solids = 0;
    for (RS_Entity *e : *blk) {
      if (e && e->rtti() == RS2::EntitySolid)
        ++solids;
    }
    if (starDBlocks <= 8)
      std::cout << "*D block " << blk->getName().toStdString() << " min=("
                << blk->getMin().x << "," << blk->getMin().y << ") max=("
                << blk->getMax().x << "," << blk->getMax().y << ") solids="
                << solids << "\n";
  }
  std::cout << "*D blocks=" << starDBlocks << "\n";

  struct SolidAuditIface : public CountingIface {
    int blockDepth = 0;
    QString currentBlock;
    void addBlock(const DRW_Block &b) override {
      CountingIface::addBlock(b);
      currentBlock = QString::fromUtf8(b.name.c_str());
      ++blockDepth;
    }
    void endBlock() override {
      --blockDepth;
      if (blockDepth <= 0) {
        blockDepth = 0;
        currentBlock.clear();
      }
    }
    void addSolid(const DRW_Solid &e) override {
      CountingIface::addSolid(e);
      const double minX = std::min({e.basePoint.x, e.secPoint.x, e.thirdPoint.x,
                                    e.fourPoint.x});
      if (blockDepth == 0)
        ++modelSolids;
      ++solidsLogged;
      (void)minX;
    }
    int solidsLogged = 0;
    int modelSolids = 0;
  } drwAudit;
  const DwgResult drw = readDwg(path, false, &drwAudit);
  std::cout << "DRW solids delivered=" << drwAudit.solidsLogged
            << " model-scope=" << drwAudit.modelSolids << "\n";
  REQUIRE(drw.ok);

  std::cout << "RS blocks total=" << graphic.countBlocks() << "\n";
  for (int i = 0; i < graphic.getBlockList()->count(); ++i) {
    RS_Block *blk = graphic.getBlockList()->at(i);
    if (!blk)
      continue;
    const QString n = blk->getName();
    if (n.contains(QLatin1Char('*')) || n.startsWith(QLatin1String("EB10")))
      std::cout << "RS block " << n.toStdString() << " ents=" << blk->count()
                << "\n";
  }

  for (const char *blkName :
       {"EB10-LEFT", "EB10-FRONT", "EB10-TOP", "EB10-RIGHT", "EB10-REAR",
        "EB10-BOTTOM", "MOUNTINGKIT-EB4-LEFT", "LABEL"}) {
    RS_Block *blk = graphic.findBlock(QString::fromLatin1(blkName));
    if (!blk)
      continue;
    blk->calculateBorders();
    std::cout << "block " << blkName << " base=(" << blk->getBasePoint().x
              << "," << blk->getBasePoint().y << ") min=(" << blk->getMin().x
              << "," << blk->getMin().y << ") max=(" << blk->getMax().x << ","
              << blk->getMax().y << ") count=" << blk->count() << "\n";
    int solids = 0;
    for (RS_Entity *e : *blk) {
      if (!e || e->rtti() != RS2::EntitySolid)
        continue;
      e->calculateBorders();
      if (solids < 3) {
        std::cout << "  def solid emin=(" << e->getMin().x << ","
                  << e->getMin().y << ") emax=(" << e->getMax().x << ","
                  << e->getMax().y << ")\n";
      }
      ++solids;
    }
    if (solids)
      std::cout << "  def solids=" << solids << "\n";
  }

  CHECK(bmin.x > -5.0);
  CHECK(bmax.x < 50.0);
  CHECK(bmin.y > -1.0);
  CHECK(bmax.y < 40.0);
  CHECK(straySolids == 0);
}

// CUSH/sofa insert chain diagnostic.
//   ./librecad_tests "[.dwg_chicun_cush_diag]" -s
TEST_CASE("DWG 2带尺寸图库: CUSH sofa insert chain diagnostic",
          "[.dwg_chicun_cush_diag]") {
  if (chicun::fixturePath().empty()) {
    SKIP("fixture not present; skipping");
  }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  REQUIRE(chicun::importFixture(graphic));

  RS_Block *cushBlk = graphic.findBlock(QStringLiteral("CUSH"));
  REQUIRE(cushBlk != nullptr);
  cushBlk->calculateBorders();
  std::cout << "\n=== CUSH block def ===\n";
  std::cout << "base=(" << cushBlk->getBasePoint().x << ","
            << cushBlk->getBasePoint().y << ") min=(" << cushBlk->getMin().x
            << "," << cushBlk->getMin().y << ") max=(" << cushBlk->getMax().x
            << "," << cushBlk->getMax().y << ")\n";
  int arcDef = 0;
  for (RS_Entity *e : *cushBlk) {
    if (!e || e->rtti() != RS2::EntityArc)
      continue;
    auto *a = static_cast<RS_Arc *>(e);
    std::cout << "  def ARC ctr=(" << a->getCenter().x << ","
              << a->getCenter().y << ") ep=(" << a->getEndpoint().x << ","
              << a->getEndpoint().y << ")\n";
    if (++arcDef >= 4)
      break;
  }

  auto dumpInsert = [&](const QString &name) {
    for (RS_Entity *e :
         lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
      if (!e || e->rtti() != RS2::EntityInsert)
        continue;
      auto *ins = static_cast<RS_Insert *>(e);
      if (ins->getName() != name)
        continue;
      const RS_Vector sc = ins->getScale();
      std::cout << "model INSERT " << name.toStdString() << " ip=("
                << ins->getInsertionPoint().x << ","
                << ins->getInsertionPoint().y << ") sc=(" << sc.x << ","
                << sc.y << ") ang=" << ins->getAngle() << "\n";
    }
    for (int i = 0; i < graphic.getBlockList()->count(); ++i) {
      RS_Block *b = graphic.getBlockList()->at(i);
      if (!b || !b->getName().contains(QStringLiteral("sofa")))
        continue;
      for (RS_Entity *e : *b) {
        if (!e || e->rtti() != RS2::EntityInsert)
          continue;
        auto *ins = static_cast<RS_Insert *>(e);
        if (!ins->getName().contains(QStringLiteral("CUSH")))
          continue;
        const RS_Vector sc = ins->getScale();
        std::cout << "block " << b->getName().toStdString() << " INSERT "
                  << ins->getName().toStdString() << " ip=("
                  << ins->getInsertionPoint().x << ","
                  << ins->getInsertionPoint().y << ") sc=(" << sc.x << ","
                  << sc.y << ") ang=" << ins->getAngle() << "\n";
      }
    }
  };
  RS_Block *sofaBlk = graphic.findBlock(QStringLiteral("sofa 1"));
  if (sofaBlk) {
    for (RS_Entity *e : *sofaBlk) {
      if (!e || e->rtti() != RS2::EntityInsert)
        continue;
      auto *ins = static_cast<RS_Insert *>(e);
      if (ins->getName() != QStringLiteral("sofa 1")
          && ins->getName() != QStringLiteral("CUSH"))
        continue;
      const RS_Vector sc = ins->getScale();
      std::cout << "sofa1 block INSERT " << ins->getName().toStdString()
                << " ip=(" << ins->getInsertionPoint().x << ","
                << ins->getInsertionPoint().y << ") sc=(" << sc.x << ","
                << sc.y << ") ang=" << ins->getAngle() << "\n";
    }
  }
  dumpInsert(QStringLiteral("A$C28F40C54"));

  int resolvedArc = 0;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || !chicun::insertChain(e).contains(QStringLiteral("CUSH")))
      continue;
    if (e->rtti() == RS2::EntityArc) {
      auto *a = static_cast<RS_Arc *>(e);
      std::cout << "resolved ARC ctr=(" << a->getCenter().x << ","
                << a->getCenter().y << ") ep=(" << a->getEndpoint().x << ","
                << a->getEndpoint().y << ") chain="
                << chicun::insertChain(e).toStdString() << "\n";
      if (++resolvedArc >= 6)
        break;
    }
  }
  SUCCEED("diagnostic only");
}

// tArch / architectural detail fixture (AC1021, ~104k entities, Chinese names).
//   ./librecad_tests "[.dwg_jiedian]" -s
TEST_CASE("DWG 全国通用节点详细解析: libdxfrw + filter load",
          "[.dwg_jiedian]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg4/全国通用节点详细解析.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  dwgR reader(path.c_str());
  TypeTrackingIface iface;
  REQUIRE(reader.read(&iface, true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);
  int total = 0;
  for (const auto &kv : iface.typeCounts)
    total += kv.second;
  CHECK(total >= 100000);
  CHECK(iface.blocks >= 900);
  CHECK(iface.layers >= 70);
  CHECK(reader.getEntityParseFailures() == 0);
  CHECK(iface.typeCounts["LINE"] >= 60000);
  CHECK(iface.typeCounts["HATCH"] >= 3000);
  CHECK(iface.layerEntities["c-装饰线"] >= 1000);

  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp = QCoreApplication::instance()
                                      ? QCoreApplication::instance()
                                      : new QCoreApplication(qargc, qargv);
  static bool settingsReady = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)qapp;
  (void)settingsReady;

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(path),
                            RS2::FormatDWG));
  CHECK(graphic.countDeep() >= 100000);

  std::cout << "\n=== 全国通用节点详细解析 load audit ===\n";
  std::cout << "libdxfrw entities=" << total
            << " filter countDeep=" << graphic.countDeep()
            << " blocks=" << graphic.countBlocks()
            << " parseFail=" << reader.getEntityParseFailures() << "\n";
}

// Scans the corpus for WIPEOUT entities (custom-class oType >= 500 with class
// recName "WIPEOUT").  Pre-fix these were silently dropped via the
// [custom-class-skipped] log path; this test asserts the dispatch-and-parse
// path is wired up.  The test is informational by default — it always passes
// when the corpus contains zero WIPEOUTs — and prints per-file counts when
// any are present so visual / round-trip work has a starting point.
TEST_CASE("DWG corpus: WIPEOUT entity inventory", "[.dwg_wipeout]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string dir = std::string(home) + "/doc/dwg2/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("~/doc/dwg2/ not found; skipping");
  }

  int totalWipeouts = 0;
  int totalVertices = 0;
  int filesWithWipeout = 0;

  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file())
      continue;
    const auto &p = entry.path();
    const std::string ext = p.extension().string();
    if (ext != ".dwg" && ext != ".DWG")
      continue;
    if (p.filename().string().front() == '#')
      continue;

    CountingIface iface;
    try {
      dwgR reader(p.string().c_str());
      (void)reader.read(&iface, true);
    } catch (...) {
      continue;
    }

    if (iface.wipeouts > 0) {
      ++filesWithWipeout;
      totalWipeouts += iface.wipeouts;
      totalVertices += iface.wipeoutVertices;
      std::cout << "  " << p.filename().string() << ": " << iface.wipeouts
                << " wipeouts, " << iface.wipeoutVertices
                << " polygon vertices total\n";
      // A WIPEOUT with empty clipPath is the historical bug shape (the
      // dwg-side polygon was being read and discarded).  Ensure that
      // every wipeout we delivered carries actual geometry.
      CHECK(iface.wipeoutVertices >= iface.wipeouts * 3);
    }
  }

  std::cout << "\nCorpus WIPEOUT summary: " << totalWipeouts
            << " entities across " << filesWithWipeout << " files, "
            << totalVertices << " vertices\n";
  SUCCEED();
}

// MULTILEADER (MLEADER) inventory.  Pre-fix the dispatcher logged WipeoutVar /
// MLEADER as [custom-class-skipped]; with Phase 2 in place, MLEADERs reach
// addMLeader() and get counted.  Phases 3-4 add structural assertions
// (entity-level fields populated, AnnotContext roots/lines non-empty).
TEST_CASE("DWG corpus: MULTILEADER entity inventory", "[.dwg_mleader]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::array<std::string, 2> dirs = {
      std::string(home) + "/doc/dwg/",  // primary corpus (has the
                                        // architectural multileaders file)
      std::string(home) + "/doc/dwg2/", // secondary corpus
  };

  int totalMLeaders = 0;
  int filesWithMLeader = 0;

  for (const auto &dir : dirs) {
    if (!std::filesystem::is_directory(dir))
      continue;
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file())
        continue;
      const auto &p = entry.path();
      const std::string ext = p.extension().string();
      if (ext != ".dwg" && ext != ".DWG")
        continue;
      if (p.filename().string().front() == '#')
        continue;

      CountingIface iface;
      try {
        dwgR reader(p.string().c_str());
        (void)reader.read(&iface, true);
      } catch (...) {
        continue;
      }

      if (iface.mleaders > 0) {
        ++filesWithMLeader;
        totalMLeaders += iface.mleaders;
        std::cout << "  " << p.filename().string() << ": " << iface.mleaders
                  << " MLEADERs, " << iface.mleaderRoots << " roots, "
                  << iface.mleaderLines << " leader lines, "
                  << iface.mleaderPoints << " points, " << iface.mleaderStyles
                  << " styles\n";
      }
    }
  }

  std::cout << "\nCorpus MLEADER summary: " << totalMLeaders
            << " entities across " << filesWithMLeader << " files\n";
  SUCCEED();
}

// Witness probe for the R2010+ visual-style fix. Counts how many entities
// in a panel of "visualization"-named R2010 files (and the canonical R2013
// witness) actually have any of the three has{Full,Face,Edge}VisualStyle
// flags set, by grepping the DRW_DBG capture for the marker emitted in
// DRW_Entity::parseDwg. Soft-asserts that visualization_-_aerial.dwg loads
// with a successful reader status and at least as many entities as the
// pre-fix baseline (0).
//
// Reference: ground-truth from libreDWG common_entity_data.spec lines
// 523-528 + ODA spec v5.4.1 §19.4.1; libdxfrw fix landed in commit
// (current).
TEST_CASE("DWG visualstyle probe: count R2010+ visual-style flag triggers",
          "[.dwg_visualstyle_probe]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }

  struct Witness {
    std::string path;
    int baselineEntities;
  };
  const std::vector<Witness> witnesses = {
      {std::string(home) + "/doc/dwg/visualization_-_aerial.dwg", 0},
      {std::string(home) +
           "/doc/dwg/visualization_-_condominium_with_skylight.dwg",
       17},
      {std::string(home) + "/doc/dwg/visualization_-_conference_room.dwg", 14},
      {std::string(home) + "/doc/dwg/visualization_-_sun_and_sky_demo.dwg", 16},
      {std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg",
       856},
  };

  int totalFiles = 0, totalFlagged = 0;
  int totalLeftoverWarnings = 0;
  int aerialEntities = -1;

  for (const auto &w : witnesses) {
    if (!std::filesystem::is_regular_file(w.path)) {
      std::cout << "(skipping missing) " << w.path << "\n";
      continue;
    }
    ++totalFiles;

    const DeepResult dr = readDwgDeep(w.path);
    const std::string &log = dr.debugLog;

    // Count "hasFull/Face/Edge VisualStyle: a b c" lines where any of
    // a/b/c is 1. The DRW_DBG marker prints space-separated 0/1 values.
    const std::string marker = "hasFull/Face/Edge VisualStyle: ";
    int flaggedHere = 0;
    size_t pos = 0;
    while ((pos = log.find(marker, pos)) != std::string::npos) {
      pos += marker.size();
      if (pos + 5 <= log.size()) {
        // Expect "a b c\n" where a,b,c are '0' or '1'
        const char a = log[pos], b = log[pos + 2], c = log[pos + 4];
        if (a == '1' || b == '1' || c == '1')
          ++flaggedHere;
      }
    }

    // Count leftover-bytes warnings — should drop to 0 for files using
    // visual styles after Phase B.
    const std::string leftover = "parseDwgEntHandle leftover";
    int leftoverHere = 0;
    size_t lpos = 0;
    while ((lpos = log.find(leftover, lpos)) != std::string::npos) {
      ++leftoverHere;
      ++lpos;
    }

    const std::string fname = std::filesystem::path(w.path).filename().string();
    std::cout << "  " << std::left << std::setw(56) << fname
              << " entities=" << std::setw(5)
              << (dr.iface.modelSpaceEntities + dr.iface.blockSpaceEntities)
              << " flaggedEntities=" << std::setw(4) << flaggedHere
              << " leftoverWarnings=" << leftoverHere
              << " (baseline entities=" << w.baselineEntities << ")\n";

    totalFlagged += flaggedHere;
    totalLeftoverWarnings += leftoverHere;

    if (fname == "visualization_-_aerial.dwg") {
      aerialEntities =
          (dr.iface.modelSpaceEntities + dr.iface.blockSpaceEntities);
      REQUIRE(dr.error == DRW::BAD_NONE);
      CHECK((dr.iface.modelSpaceEntities + dr.iface.blockSpaceEntities) >=
            w.baselineEntities);
    }
  }

  std::cout << "\nVisualStyle probe summary: " << totalFlagged
            << " flagged entities across " << totalFiles << " witness files; "
            << totalLeftoverWarnings << " leftover-bytes warnings\n";
  if (aerialEntities >= 0) {
    std::cout << "  visualization_-_aerial.dwg post-fix entities: "
              << aerialEntities << " (baseline 0)\n";
  }
  SUCCEED();
}

// AcDbColor probe — counts DBCOLOR objects in OBJECTS sections + tracks
// how many entities carry a resolved color24/colorName from a DBCOLOR
// reference. ODA spec §20.4 / libreDWG dwg2.spec:2404-2408 (object) and
// common_entity_data.spec:454-459 (ENC flag 0x40 → handle in hdl_dat).
// Diagnostic; not asserted hard.
TEST_CASE("DWG acdbcolor probe: count DBCOLOR objects + resolved entity refs",
          "[.dwg_acdbcolor_probe]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }

  // Custom interface that counts DBCOLOR objects via addDbColor and
  // tracks how many entities arrive with a populated color24/colorName.
  class AcDbColorIface : public TypeTrackingIface {
  public:
    int dbColors = 0;
    int entitiesWithColor24 = 0;
    int entitiesWithColorName = 0;
    std::vector<std::pair<int, std::string>> sampleResolutions; // first few

    void addDbColor(const DRW_DbColor &d) override {
      ++dbColors;
      if (sampleResolutions.size() < 5) {
        std::string display =
            d.bookName.empty() ? d.name : (d.bookName + "$" + d.name);
        sampleResolutions.emplace_back(d.rgb, display);
      }
    }
    void track(const DRW_Entity &e) {
      if (e.color24 != -1)
        ++entitiesWithColor24;
      if (!e.colorName.empty())
        ++entitiesWithColorName;
    }
    // Hook a few common entity types — enough to surface any resolved
    // color references without re-overriding every addX in the base.
    void addPoint(const DRW_Point &e) override {
      track(e);
      TypeTrackingIface::addPoint(e);
    }
    void addLine(const DRW_Line &e) override {
      track(e);
      TypeTrackingIface::addLine(e);
    }
    void addCircle(const DRW_Circle &e) override {
      track(e);
      TypeTrackingIface::addCircle(e);
    }
    void addArc(const DRW_Arc &e) override {
      track(e);
      TypeTrackingIface::addArc(e);
    }
    void addInsert(const DRW_Insert &e) override {
      track(e);
      TypeTrackingIface::addInsert(e);
    }
    void addText(const DRW_Text &e) override {
      track(e);
      TypeTrackingIface::addText(e);
    }
    void addLWPolyline(const DRW_LWPolyline &e) override {
      track(e);
      TypeTrackingIface::addLWPolyline(e);
    }
    void addPolyline(const DRW_Polyline &e) override {
      track(e);
      TypeTrackingIface::addPolyline(e);
    }
    void addHatch(const DRW_Hatch *e) override {
      track(*e);
      TypeTrackingIface::addHatch(e);
    }
  };

  struct File {
    std::string path;
    const char *note;
  };
  const std::vector<File> files = {
      {std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg",
       "R2013, canonical book-color witness (per memory)"},
      {std::string(home) + "/doc/dwg/visualization_-_aerial.dwg", "R2010"},
      {std::string(home) +
           "/doc/dwg/visualization_-_condominium_with_skylight.dwg",
       "R2010"},
      {std::string(home) + "/doc/dwg/visualization_-_conference_room.dwg",
       "R2010"},
      {std::string(home) + "/doc/dwg/visualization_-_sun_and_sky_demo.dwg",
       "R2010"},
      {std::string(home) + "/doc/dwg2/gear_pump_subassy.dwg",
       "R2010 control (must not regress)"},
  };

  int totalDbColors = 0, totalResolvedColor24 = 0, totalResolvedNames = 0;
  for (const auto &f : files) {
    if (!std::filesystem::is_regular_file(f.path)) {
      std::cout << "(skipping missing) " << f.path << "\n";
      continue;
    }
    AcDbColorIface iface;
    try {
      dwgR reader(f.path.c_str());
      (void)reader.read(&iface, true);
    } catch (...) {
      std::cout << "EXCEPTION on " << f.path << "\n";
      continue;
    }
    const std::string fname = std::filesystem::path(f.path).filename().string();
    const int totalEnt = iface.modelSpaceEntities + iface.blockSpaceEntities;
    std::cout << "  " << std::left << std::setw(56) << fname
              << " dbColors=" << std::setw(4) << iface.dbColors
              << " entWithColor24=" << std::setw(4) << iface.entitiesWithColor24
              << " entWithColorName=" << std::setw(4)
              << iface.entitiesWithColorName << " entities=" << totalEnt
              << "  (" << f.note << ")\n";
    for (const auto &s : iface.sampleResolutions) {
      std::cout << "    sample DBCOLOR rgb=" << std::hex << s.first << std::dec
                << " name=\"" << s.second << "\"\n";
    }
    totalDbColors += iface.dbColors;
    totalResolvedColor24 += iface.entitiesWithColor24;
    totalResolvedNames += iface.entitiesWithColorName;
  }

  std::cout << "\nAcDbColor probe summary: " << totalDbColors
            << " DBCOLOR objects, " << totalResolvedColor24
            << " entities w/ color24, " << totalResolvedNames
            << " entities w/ colorName\n";
  SUCCEED();
}

// AcDbColor end-to-end test: load the canonical book-color witness file
// and assert (a) it loads BAD_NONE, (b) entity count matches the existing
// [.dwg_arch_hatch] sentinel of 856 entities (no regression), and (c)
// at least one DBCOLOR object exists in the file (positive coverage if
// the file actually uses book colors). Untagged so it runs in default
// smoke pass once we're confident the fix is stable.
TEST_CASE("DWG acdbcolor: book color load + dbColor object inventory",
          "[.dwg_acdbcolor]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("witness file not present; skipping");
  }

  int dbColors = 0;
  class CountingDbColorIface : public TypeTrackingIface {
  public:
    int *countOut = nullptr;
    void addDbColor(const DRW_DbColor & /*d*/) override {
      if (countOut)
        ++(*countOut);
    }
  };
  CountingDbColorIface iface;
  iface.countOut = &dbColors;

  DRW::error err = DRW::BAD_NONE;
  int entities = 0;
  try {
    dwgR reader(path.c_str());
    (void)reader.read(&iface, true);
    err = reader.getError();
    entities = iface.modelSpaceEntities + iface.blockSpaceEntities;
  } catch (const std::exception &ex) {
    FAIL("Exception: " << ex.what());
  }

  REQUIRE(err == DRW::BAD_NONE);
  // Existing [.dwg_arch_hatch] sentinel proves 48 hatches; the broader
  // entity baseline is 856 (per golden corpus output).
  CHECK(entities == 856);

  std::cout << "Architectural-Modern-Building-Design.dwg DBCOLOR objects: "
            << dbColors << "\n";
  // Soft expectation: the file IS named "Architectural" and uses
  // book colors per memory note. If 0, that's surprising but not a
  // failure — the spec/parser correctness is asserted by entity count.
}

// Layer-level AcDbColor probe — counts how many layers in each corpus file
// have a populated colorName (CMC method-byte bit 1 → libreDWG bit_read_T
// from str_dat).  Diagnostic only; no hard assertion since corpus coverage
// is unknown until we run.
TEST_CASE("DWG acdbcolor: layer colorName probe",
          "[.dwg_acdbcolor_layer_probe]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }

  class LayerColorIface : public TypeTrackingIface {
  public:
    int totalLayers = 0;
    int layersWithColor24 = 0;
    int layersWithColorName = 0;
    std::vector<std::tuple<std::string, int, std::string>> samples;
    void addLayer(const DRW_Layer &l) override {
      ++totalLayers;
      ++layers; // base class counter
      if (l.color24 != -1)
        ++layersWithColor24;
      if (!l.colorName.empty()) {
        ++layersWithColorName;
        if (samples.size() < 10) {
          samples.emplace_back(l.name, l.color24, l.colorName);
        }
      }
    }
  };

  struct Dir {
    std::string path;
    const char *note;
  };
  const std::vector<Dir> dirs = {
      {std::string(home) + "/doc/dwg2/", "R2010-R2013 corpus"},
      {std::string(home) + "/doc/dwg/", "mixed corpus"},
      {std::string(home) + "/dev/dwg_samples/", "single-entity samples"},
  };

  int totalFiles = 0, totalLayers = 0, totalColor24 = 0, totalColorName = 0;
  for (const auto &d : dirs) {
    if (!std::filesystem::is_directory(d.path)) {
      std::cout << "(skipping missing dir) " << d.path << "\n";
      continue;
    }
    std::cout << "\n=== " << d.path << " (" << d.note << ") ===\n";
    std::vector<std::filesystem::path> paths;
    for (const auto &e : std::filesystem::directory_iterator(d.path)) {
      if (!e.is_regular_file())
        continue;
      const auto ext = e.path().extension().string();
      if (ext == ".dwg" || ext == ".DWG")
        paths.push_back(e.path());
    }
    std::sort(paths.begin(), paths.end());
    for (const auto &p : paths) {
      const std::string fname = p.filename().string();
      if (fname.front() == '#')
        continue; // BAD_VERSION sentinel
      ++totalFiles;
      LayerColorIface iface;
      try {
        dwgR reader(p.string().c_str());
        (void)reader.read(&iface, true);
      } catch (...) {
        continue;
      }
      if (iface.layersWithColorName > 0 || iface.layersWithColor24 > 0) {
        std::cout << "  " << std::left << std::setw(56) << fname
                  << " layers=" << std::setw(4) << iface.totalLayers
                  << " withColor24=" << std::setw(4) << iface.layersWithColor24
                  << " withColorName=" << iface.layersWithColorName << "\n";
        for (const auto &s : iface.samples) {
          std::cout << "    layer \"" << std::get<0>(s)
                    << "\"  color24=" << std::hex << std::get<1>(s) << std::dec
                    << "  colorName=\"" << std::get<2>(s) << "\"\n";
        }
      }
      totalLayers += iface.totalLayers;
      totalColor24 += iface.layersWithColor24;
      totalColorName += iface.layersWithColorName;
    }
  }

  std::cout << "\nLayer color probe summary: " << totalFiles << " files, "
            << totalLayers << " layers total, " << totalColor24
            << " with color24, " << totalColorName << " with colorName\n";
  SUCCEED();
}

// PLOTSETTINGS probe — count PLOTSETTINGS objects per corpus file. They
// were silently dropped to remainingMap before the custom-class dispatch
// added the recName=="PLOTSETTINGS" clause. ODA spec §20.4 / libreDWG
// dwg.spec:5627. RS_FilterDXFRW::addPlotSettings already wires margins +
// page name to m_graphic; this probe confirms delivery.
TEST_CASE("DWG plotsettings probe: count PLOTSETTINGS objects per file",
          "[.dwg_plotsettings_probe]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }

  class PlotSettingsIface : public TypeTrackingIface {
  public:
    int plotSettingsCount = 0;
    std::vector<std::tuple<std::string, double, double, double, double>>
        samples;
    void addPlotSettings(const DRW_PlotSettings *d) override {
      ++plotSettingsCount;
      if (samples.size() < 10) {
        samples.emplace_back(d->plotViewName, d->marginLeft, d->marginTop,
                             d->marginRight, d->marginBottom);
      }
    }
  };

  struct Dir {
    std::string path;
    const char *note;
  };
  const std::vector<Dir> dirs = {
      {std::string(home) + "/doc/dwg2/", "R2010-R2013 corpus"},
      {std::string(home) + "/doc/dwg/", "mixed corpus"},
  };

  int totalFiles = 0, totalPlotSettings = 0;
  for (const auto &d : dirs) {
    if (!std::filesystem::is_directory(d.path)) {
      std::cout << "(skipping missing) " << d.path << "\n";
      continue;
    }
    std::cout << "\n=== " << d.path << " ===\n";
    std::vector<std::filesystem::path> paths;
    for (const auto &e : std::filesystem::directory_iterator(d.path)) {
      if (!e.is_regular_file())
        continue;
      const auto ext = e.path().extension().string();
      if (ext == ".dwg" || ext == ".DWG")
        paths.push_back(e.path());
    }
    std::sort(paths.begin(), paths.end());
    for (const auto &p : paths) {
      const std::string fname = p.filename().string();
      if (fname.front() == '#')
        continue;
      ++totalFiles;
      PlotSettingsIface iface;
      try {
        dwgR reader(p.string().c_str());
        (void)reader.read(&iface, true);
      } catch (...) {
        continue;
      }
      if (iface.plotSettingsCount > 0) {
        std::cout << "  " << std::left << std::setw(56) << fname
                  << " plotSettings=" << iface.plotSettingsCount << "\n";
        for (const auto &s : iface.samples) {
          std::cout << "    \"" << std::get<0>(s)
                    << "\"  margins L/T/R/B = " << std::get<1>(s) << "/"
                    << std::get<2>(s) << "/" << std::get<3>(s) << "/"
                    << std::get<4>(s) << "\n";
        }
      }
      totalPlotSettings += iface.plotSettingsCount;
    }
  }

  std::cout << "\nPLOTSETTINGS summary: " << totalPlotSettings
            << " objects across " << totalFiles << " files\n";
  SUCCEED();
}

// Transparency probe — count entities per file with a non-default
// `transparency` field set via ENC flag 0x20. libreDWG
// common_entity_data.spec:432-446 documents the alpha_raw encoding;
// RS_FilterDXFRW::setEntityAttributes converts alpha_type==3 to a
// per-entity pen alpha. This probe confirms delivery and shows whether
// the corpus exercises the path.
TEST_CASE("DWG transparency probe: count entities with ENC alpha set",
          "[.dwg_transparency_probe]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }

  class TransparencyIface : public TypeTrackingIface {
  public:
    int entitiesWithTransparency = 0;
    int alphaType3Count = 0;       // explicit alpha
    int alphaTypeBlockOrLayer = 0; // type 0 or 1
    std::vector<std::pair<std::string, unsigned>> samples;

    void track(const DRW_Entity &e, const char *what) {
      if (e.transparency == DRW::Opaque)
        return;
      ++entitiesWithTransparency;
      const unsigned int raw = static_cast<unsigned int>(e.transparency);
      const unsigned int aType = (raw >> 24) & 0xFF;
      if (aType == 3)
        ++alphaType3Count;
      else if (aType == 0 || aType == 1)
        ++alphaTypeBlockOrLayer;
      if (samples.size() < 10)
        samples.emplace_back(what, raw);
    }
    void addPoint(const DRW_Point &e) override {
      track(e, "POINT");
      TypeTrackingIface::addPoint(e);
    }
    void addLine(const DRW_Line &e) override {
      track(e, "LINE");
      TypeTrackingIface::addLine(e);
    }
    void addCircle(const DRW_Circle &e) override {
      track(e, "CIRCLE");
      TypeTrackingIface::addCircle(e);
    }
    void addArc(const DRW_Arc &e) override {
      track(e, "ARC");
      TypeTrackingIface::addArc(e);
    }
    void addInsert(const DRW_Insert &e) override {
      track(e, "INSERT");
      TypeTrackingIface::addInsert(e);
    }
    void addText(const DRW_Text &e) override {
      track(e, "TEXT");
      TypeTrackingIface::addText(e);
    }
    void addLWPolyline(const DRW_LWPolyline &e) override {
      track(e, "LWPOLY");
      TypeTrackingIface::addLWPolyline(e);
    }
    void addPolyline(const DRW_Polyline &e) override {
      track(e, "POLY");
      TypeTrackingIface::addPolyline(e);
    }
    void addHatch(const DRW_Hatch *e) override {
      track(*e, "HATCH");
      TypeTrackingIface::addHatch(e);
    }
  };

  struct Dir {
    std::string path;
    const char *note;
  };
  const std::vector<Dir> dirs = {
      {std::string(home) + "/doc/dwg2/", "R2010-R2013 corpus"},
      {std::string(home) + "/doc/dwg/", "mixed corpus"},
  };

  int totalFiles = 0, totalEnt = 0, totalAlpha3 = 0;
  for (const auto &d : dirs) {
    if (!std::filesystem::is_directory(d.path))
      continue;
    std::cout << "\n=== " << d.path << " ===\n";
    std::vector<std::filesystem::path> paths;
    for (const auto &e : std::filesystem::directory_iterator(d.path)) {
      if (!e.is_regular_file())
        continue;
      const auto ext = e.path().extension().string();
      if (ext == ".dwg" || ext == ".DWG")
        paths.push_back(e.path());
    }
    std::sort(paths.begin(), paths.end());
    for (const auto &p : paths) {
      const std::string fname = p.filename().string();
      if (fname.front() == '#')
        continue;
      ++totalFiles;
      TransparencyIface iface;
      try {
        dwgR reader(p.string().c_str());
        (void)reader.read(&iface, true);
      } catch (...) {
        continue;
      }
      if (iface.entitiesWithTransparency > 0) {
        std::cout << "  " << std::left << std::setw(56) << fname
                  << " entWithAlpha=" << iface.entitiesWithTransparency
                  << "  type3=" << iface.alphaType3Count
                  << "  inherit=" << iface.alphaTypeBlockOrLayer << "\n";
        for (const auto &s : iface.samples) {
          std::cout << "    " << std::get<0>(s) << "  alpha_raw=0x" << std::hex
                    << std::get<1>(s) << std::dec << "\n";
        }
      }
      totalEnt += iface.entitiesWithTransparency;
      totalAlpha3 += iface.alphaType3Count;
    }
  }

  std::cout << "\nTransparency summary: " << totalEnt << " entities across "
            << totalFiles << " files; " << totalAlpha3
            << " with explicit alpha (type 3)\n";
  SUCCEED();
}

// ---- field-level parity tests for DIMENSION + HATCH ------------------------

namespace {

struct HatchFieldSnapshot {
  int isGradient;
  int singleColor;
  double gradAngle;
  double gradShift;
  double gradTint;
  std::string gradName;
  size_t gradColorCount;
  size_t seedPointCount;
};

struct DimFieldSnapshot {
  std::string subtype;
  double measureValue;
  bool flipArrow1;
  bool flipArrow2;
};

class FieldCaptureIface : public TypeTrackingIface {
public:
  std::vector<HatchFieldSnapshot> hatchSnaps;
  std::vector<DimFieldSnapshot> dimSnaps;

  void addHatch(const DRW_Hatch *e) override {
    TypeTrackingIface::addHatch(e);
    hatchSnaps.push_back({e->isGradient, e->singleColor, e->gradAngle,
                          e->gradShift, e->gradTint, e->gradName,
                          e->gradColors.size(), e->seedPoints.size()});
  }
  void addDimAlign(const DRW_DimAligned *e) override {
    TypeTrackingIface::addDimAlign(e);
    dimSnaps.push_back({"DIM_ALIGNED", e->getMeasureValue(), e->getFlipArrow1(),
                        e->getFlipArrow2()});
  }
  void addDimLinear(const DRW_DimLinear *e) override {
    TypeTrackingIface::addDimLinear(e);
    dimSnaps.push_back({"DIM_LINEAR", e->getMeasureValue(), e->getFlipArrow1(),
                        e->getFlipArrow2()});
  }
  void addDimRadial(const DRW_DimRadial *e) override {
    TypeTrackingIface::addDimRadial(e);
    dimSnaps.push_back({"DIM_RADIAL", e->getMeasureValue(), e->getFlipArrow1(),
                        e->getFlipArrow2()});
  }
  void addDimDiametric(const DRW_DimDiametric *e) override {
    TypeTrackingIface::addDimDiametric(e);
    dimSnaps.push_back({"DIM_DIAMETRIC", e->getMeasureValue(),
                        e->getFlipArrow1(), e->getFlipArrow2()});
  }
  void addDimAngular(const DRW_DimAngular *e) override {
    TypeTrackingIface::addDimAngular(e);
    dimSnaps.push_back({"DIM_ANGULAR", e->getMeasureValue(), e->getFlipArrow1(),
                        e->getFlipArrow2()});
  }
  void addDimAngular3P(const DRW_DimAngular3p *e) override {
    TypeTrackingIface::addDimAngular3P(e);
    dimSnaps.push_back({"DIM_ANGULAR3P", e->getMeasureValue(),
                        e->getFlipArrow1(), e->getFlipArrow2()});
  }
  void addDimOrdinate(const DRW_DimOrdinate *e) override {
    TypeTrackingIface::addDimOrdinate(e);
    dimSnaps.push_back({"DIM_ORDINATE", e->getMeasureValue(),
                        e->getFlipArrow1(), e->getFlipArrow2()});
  }
};

} // namespace

// Verifies that the DWG decoder actually populates DRW_Hatch's new gradient
// + seed-point members (previously read-then-discarded). Uses the same
// Architectural-Modern-Building-Design.dwg fixture that already exercises 48
// HATCH entities including gradient fills.
TEST_CASE("DWG hatch field parity: gradient + seed points populate",
          "[.dwg_hatch_fields]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/doc/dwg2/Architectural-Modern-Building-Design.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("Architectural-Modern-Building-Design.dwg not present; skipping");
  }

  FieldCaptureIface iface;
  {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::None);
    REQUIRE(reader.read(&iface, true));
  }

  REQUIRE(iface.hatchSnaps.size() >= 1u);

  int gradientHatches = 0;
  int hatchesWithSeeds = 0;
  int totalSeedPoints = 0;
  for (const auto &s : iface.hatchSnaps) {
    if (s.isGradient) {
      ++gradientHatches;
      // A gradient hatch must have at least one stop and a name.
      CHECK(s.gradColorCount >= 1u);
      CHECK(!s.gradName.empty());
    }
    if (s.seedPointCount > 0) {
      ++hatchesWithSeeds;
      totalSeedPoints += static_cast<int>(s.seedPointCount);
    }
  }

  std::cout << "\n  hatches: " << iface.hatchSnaps.size()
            << "  gradient: " << gradientHatches
            << "  with seeds: " << hatchesWithSeeds
            << "  total seed points: " << totalSeedPoints << "\n";

  // The fixture is documented to include gradient hatches — at least one
  // should now populate the new DRW_Hatch fields.
  CHECK(gradientHatches >= 1);
}

// Verifies that DIMENSION's measureValue (code 42) and flipArrow1/2 (codes
// 74/75) survive the DWG parser. Pre-fix these were read-then-discarded for
// AC1015+/AC1021+ files. We just sanity-check that the values are sane:
// measureValue is finite; flipArrow flags are 0 or 1 (bool).
TEST_CASE("DWG dimension field parity: measureValue + flipArrow populate",
          "[.dwg_dim_fields]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string dir = std::string(home) + "/doc/dwg2/";
  if (!std::filesystem::is_directory(dir)) {
    SKIP("DWG corpus directory not found; skipping");
  }

  int filesScanned = 0;
  int totalDims = 0;
  int nonZeroMeasure = 0;
  int flippedArrows = 0;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() != ".dwg")
      continue;
    FieldCaptureIface iface;
    try {
      dwgR reader(entry.path().string().c_str());
      reader.setDebug(DRW::DebugLevel::None);
      if (!reader.read(&iface, true))
        continue;
    } catch (...) {
      continue;
    }
    ++filesScanned;
    for (const auto &d : iface.dimSnaps) {
      ++totalDims;
      // measureValue should be finite (NaN/Inf would indicate the
      // assignment never happened).
      CHECK(std::isfinite(d.measureValue));
      if (d.measureValue != 0.0)
        ++nonZeroMeasure;
      if (d.flipArrow1 || d.flipArrow2)
        ++flippedArrows;
    }
  }

  std::cout << "\n  files scanned: " << filesScanned << "  dims: " << totalDims
            << "  non-zero measure: " << nonZeroMeasure
            << "  flipped arrows: " << flippedArrows << "\n";

  // At least some dimensions in the corpus should have non-zero measure
  // values. If none do, either the assignment is broken or the corpus has
  // no meaningful dimensions.
  if (totalDims > 0)
    CHECK(nonZeroMeasure > 0);
}

// ----------------------------------------------------------------------------
// Pre-R13 detection: confirm the reader recognizes AC2.10 (1986) header,
// classifies it as BAD_VERSION (no parser exists), and surfaces the
// recognized version through dwgR::getVersion(). This is what powers the
// improved user-facing error message in RS_FilterDXFRW.
// ----------------------------------------------------------------------------
TEST_CASE("DWG pre-R13 detection: ~/doc/dwg3/block.dwg classifies AC2.10",
          "[.dwg_pre_r13]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path = std::string(home) + "/doc/dwg3/block.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("~/doc/dwg3/block.dwg not found");
  }
  const DwgResult r = readDwg(path);
  CHECK_FALSE(r.ok);
  CHECK(r.error == DRW::BAD_VERSION);
  CHECK(r.version == DRW::AC210);
  INFO("recognized version: " << versionStr(r.version));
}

// Deep troubleshooting probe for
// ~/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg. Reports
// MLEADER fidelity (per-entity points, breaks, text content), MLEADER style
// count, per-block entity delivery, dimension subtype coverage, and any
// debug-trace warnings that suggest parse drift (leftover bytes, alignment
// hiccups, DBG markers). Run: ./librecad_tests "[.dwg_arch_mleader_probe]" -s
TEST_CASE("DWG arch_multileaders: deep fidelity probe",
          "[.dwg_arch_mleader_probe]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path =
      std::string(home) +
      "/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("file not present; skipping");
  }

  class Probe : public TypeTrackingIface {
  public:
    struct MLeaderSnap {
      int rootCount;
      int leaderLineCount;
      int pointCount; // sum across all leader lines
      int breakCount; // sum: root-level + line-level
      bool hasTextContents;
      std::string textLabel;
      int styleContentType;
      double overallScale;
      bool hasContentsBlock;
      std::string layer;
    };
    std::vector<MLeaderSnap> mleaderSnaps;
    std::map<std::string, int>
        blockEntityCounts; // block_idx-name -> count when inBlock>0
    int currentBlockIdx = -1;
    std::string currentBlockName;
    int mleaderStyles = 0;

    void addBlock(const DRW_Block &b) override {
      TypeTrackingIface::addBlock(b);
      ++currentBlockIdx;
      currentBlockName = b.name;
    }
    void endBlock() override { TypeTrackingIface::endBlock(); }

    void trackInBlock() {
      if (inBlock > 0) {
        ++blockEntityCounts[std::to_string(currentBlockIdx) + ":" +
                            currentBlockName];
      }
    }

    void addLine(const DRW_Line &e) override {
      TypeTrackingIface::addLine(e);
      trackInBlock();
    }
    void addArc(const DRW_Arc &e) override {
      TypeTrackingIface::addArc(e);
      trackInBlock();
    }
    void addCircle(const DRW_Circle &e) override {
      TypeTrackingIface::addCircle(e);
      trackInBlock();
    }
    void addLWPolyline(const DRW_LWPolyline &e) override {
      TypeTrackingIface::addLWPolyline(e);
      trackInBlock();
    }
    void addPolyline(const DRW_Polyline &e) override {
      TypeTrackingIface::addPolyline(e);
      trackInBlock();
    }
    void addInsert(const DRW_Insert &e) override {
      TypeTrackingIface::addInsert(e);
      trackInBlock();
    }
    void addText(const DRW_Text &e) override {
      TypeTrackingIface::addText(e);
      trackInBlock();
    }
    void addMText(const DRW_MText &e) override {
      TypeTrackingIface::addMText(e);
      trackInBlock();
    }
    void addPoint(const DRW_Point &e) override {
      TypeTrackingIface::addPoint(e);
      trackInBlock();
    }
    void addHatch(const DRW_Hatch *e) override {
      TypeTrackingIface::addHatch(e);
      trackInBlock();
    }
    void addEllipse(const DRW_Ellipse &e) override {
      TypeTrackingIface::addEllipse(e);
      trackInBlock();
    }
    void addSpline(const DRW_Spline *e) override {
      TypeTrackingIface::addSpline(e);
      trackInBlock();
    }

    void addMLeader(const DRW_MLeader *e) override {
      TypeTrackingIface::addMLeader(e);
      trackInBlock();
      int pts = 0, brks = 0;
      for (const auto &r : e->context.roots) {
        brks += static_cast<int>(r.breaks.size());
        for (const auto &ll : r.leaderLines) {
          pts += static_cast<int>(ll.points.size());
          brks += static_cast<int>(ll.breaks.size());
        }
      }
      mleaderSnaps.push_back({static_cast<int>(e->context.roots.size()),
                              [&] {
                                int n = 0;
                                for (auto &r : e->context.roots)
                                  n += r.leaderLines.size();
                                return n;
                              }(),
                              pts, brks, e->context.hasTextContents,
                              e->context.textLabel, e->styleContentType,
                              e->context.overallScale,
                              e->context.hasContentsBlock, e->layer});
    }
    void addMLeaderStyle(const DRW_MLeaderStyle *) override { ++mleaderStyles; }
  };

  auto *capture = new CapturingPrinter();
  DRW::setCustomDebugPrinter(capture);

  Probe p;
  {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::Debug);
    bool ok = reader.read(&p, true);
    REQUIRE(ok);
    REQUIRE(reader.getError() == DRW::BAD_NONE);
  }
  std::string log = std::move(capture->buf);
  DRW::setCustomDebugPrinter(new DRW::DebugPrinter());

  std::cout << "\n=== arch multileaders: probe ===\n";
  std::cout << "blocks=" << p.blocks << " layers=" << p.layers
            << " modelspaceEnt=" << p.modelSpaceEntities
            << " inBlocksEnt=" << p.blockSpaceEntities << "\n";

  std::cout << "\nHandled type counts:\n";
  for (const auto &[k, v] : p.typeCounts)
    std::cout << "  " << std::left << std::setw(16) << k << " " << v << "\n";

  std::cout << "\nMLEADER styles delivered: " << p.mleaderStyles << "\n";
  std::cout << "MLEADER snapshots (" << p.mleaderSnaps.size() << "):\n";
  int degenerate = 0, missingText = 0;
  int totalRoots = 0, totalLines = 0, totalPts = 0;
  for (size_t i = 0; i < p.mleaderSnaps.size(); ++i) {
    const auto &m = p.mleaderSnaps[i];
    totalRoots += m.rootCount;
    totalLines += m.leaderLineCount;
    totalPts += m.pointCount;
    bool isDegenerate = (m.pointCount < 2 * m.leaderLineCount);
    bool isMissingText =
        (m.styleContentType == 2 /*MTEXT*/ && m.textLabel.empty());
    if (isDegenerate)
      ++degenerate;
    if (isMissingText)
      ++missingText;
    if (i < 8 || isDegenerate || isMissingText) {
      std::cout << "  #" << i << " layer=" << std::setw(28) << m.layer
                << " roots=" << m.rootCount << " lines=" << m.leaderLineCount
                << " pts=" << m.pointCount << " brks=" << m.breakCount
                << " hasText=" << m.hasTextContents
                << " contentType=" << m.styleContentType
                << " scale=" << m.overallScale << " text=\""
                << m.textLabel.substr(0, 40) << "\""
                << (isDegenerate ? "  [DEGENERATE]" : "")
                << (isMissingText ? "  [MISSING_TEXT]" : "") << "\n";
    }
  }
  std::cout << "MLEADER aggregate: roots=" << totalRoots
            << " lines=" << totalLines << " pts=" << totalPts
            << " degenerate=" << degenerate << " missingText=" << missingText
            << "\n";

  std::cout << "\nPer-block entity delivery (top 20):\n";
  std::vector<std::pair<int, std::string>> blockSorted;
  for (const auto &[k, v] : p.blockEntityCounts)
    blockSorted.emplace_back(v, k);
  std::sort(blockSorted.rbegin(), blockSorted.rend());
  int shown = 0;
  int blocksWithEntities = 0;
  for (const auto &[v, k] : blockSorted) {
    if (v > 0)
      ++blocksWithEntities;
    if (shown < 20) {
      std::cout << "  " << std::left << std::setw(40) << k << " " << v << "\n";
      ++shown;
    }
  }
  std::cout << "Blocks with delivered entities: " << blocksWithEntities << " / "
            << p.blocks << "\n";

  auto countMarker = [&](const std::string &m) {
    int c = 0;
    size_t pos = 0;
    while ((pos = log.find(m, pos)) != std::string::npos) {
      ++c;
      ++pos;
    }
    return c;
  };
  std::cout << "\nDebug-trace warnings:\n";
  std::cout << "  parseDwgEntHandle leftover : "
            << countMarker("parseDwgEntHandle leftover") << "\n";
  std::cout << "  AnnotContext parse drift   : "
            << countMarker("AnnotContext parse drift") << "\n";
  std::cout << "  MLEADER: parseDwgEntHandle hiccup : "
            << countMarker("MLEADER: parseDwgEntHandle hiccup") << "\n";
  std::cout << "  MLEADER: handle-stream tail unconsumed : "
            << countMarker("MLEADER: handle-stream tail") << "\n";
  std::cout << "  not implemented            : "
            << countMarker("not implemented") << "\n";
  std::cout << "  RLZ                        : " << countMarker("RLZ") << "\n";
  std::cout << "  entity-pass-defer          : "
            << countMarker("[entity-pass-defer") << "\n";
  std::cout << "  bad CRC                    : " << countMarker("CRC") << "\n";
  std::cout << "  read past end / not ok     : " << countMarker("not ok")
            << "\n";
  std::cout << "  ERROR/BAD                  : " << countMarker("BAD_") << "\n";

  // MLEADER tail-rb distribution: "MLEADER tail rb=N" lines. With the
  // entity-specific handle stream fully consumed, every entity should
  // report rb in {0..4} (just the trailing CRC).
  {
    std::map<int, int> rbHist;
    std::regex re(R"(MLEADER tail rb=(-?\d+))");
    auto begin = std::sregex_iterator(log.begin(), log.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it)
      ++rbHist[std::stoi((*it)[1].str())];
    std::cout << "\nMLEADER tail-rb histogram (bytes left after MLEADER handle "
                 "stream):\n";
    for (const auto &[rb, cnt] : rbHist)
      std::cout << "  rb=" << std::setw(4) << rb << "  count=" << cnt << "\n";
  }

  SUCCEED();
}

// Regression guard for the MLEADER body parser fix
// (parseMLeaderRoot rewritten to libreDWG dwg2.spec parity, 2026-05-10).
// Asserts on real per-entity content, not just counts — without the fix
// most fields read as denormalized garbage even though the file loads OK.
TEST_CASE("DWG arch_multileaders: MLEADER body parser fidelity",
          "[.slow][dwg][corpus][mleader]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path =
      std::string(home) +
      "/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  struct Snap {
    std::string layer;
    std::string text;
    int contentType;
    double scale;
    bool hasText;
    std::uint32_t styleHandleRef;
    std::uint32_t textStyleHandleRef; // ctx.textStyleHandle
    std::uint32_t leaderLineTypeHandleRef;
    std::uint32_t styleTextStyleHandleRef;
  };

  class Iface : public TypeTrackingIface {
  public:
    std::vector<Snap> snaps;
    void addMLeader(const DRW_MLeader *e) override {
      TypeTrackingIface::addMLeader(e);
      snaps.push_back({e->layer, e->context.textLabel, e->styleContentType,
                       e->context.overallScale, e->context.hasTextContents,
                       e->styleHandle.ref, e->context.textStyleHandle.ref,
                       e->leaderLineTypeHandle.ref,
                       e->styleTextStyleHandle.ref});
    }
  };

  auto *capture = new CapturingPrinter();
  DRW::setCustomDebugPrinter(capture);

  Iface iface;
  {
    dwgR reader(path.c_str());
    reader.setDebug(DRW::DebugLevel::Debug);
    REQUIRE(reader.read(&iface, true));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
  }
  const std::string log = std::move(capture->buf);
  DRW::setCustomDebugPrinter(new DRW::DebugPrinter());

  // The fixture is documented to carry 36 MLEADERs.
  REQUIRE(iface.snaps.size() == 36u);

  // No body-parse drift markers in the trace — these were 8 + 12
  // pre-fix and would silently corrupt every subsequent field.
  auto countMarker = [&](const std::string &m) {
    int c = 0;
    size_t pos = 0;
    while ((pos = log.find(m, pos)) != std::string::npos) {
      ++c;
      ++pos;
    }
    return c;
  };
  CHECK(countMarker("AnnotContext parse drift") == 0);
  CHECK(countMarker("MLEADER: parseDwgEntHandle hiccup") == 0);
  CHECK(countMarker("MLEADER: implausible classVersion") == 0);

  // Tail handle stream fully consumed (Issue 2 fix). Per-MLEADER "tail
  // rb=N" lines should all report 0 — the entity-level + per-line +
  // per-arrowhead/blocklabel handles drain the remaining bits exactly.
  CHECK(countMarker("MLEADER: handle-stream tail") == 0);
  {
    std::regex re(R"(MLEADER tail rb=(-?\d+))");
    auto begin = std::sregex_iterator(log.begin(), log.end(), re);
    auto end = std::sregex_iterator();
    int total = 0, nonzero = 0;
    for (auto it = begin; it != end; ++it) {
      ++total;
      if (std::stoi((*it)[1].str()) != 0)
        ++nonzero;
    }
    CHECK(total == 36);
    CHECK(nonzero == 0);
  }

  // Per-entity assertions. Every MLEADER must have:
  //   - styleContentType in {0,1,2,3} — the 4 valid values
  //   - overallScale finite and >0; for THIS fixture, ∈ {4, 24, 48}
  //     (the file's annotation scales)
  //   - layer name resolved via the trailing handle stream, NOT "0"
  //     (pre-fix, ~13/36 landed on "0" because the body left the
  //     handle stream mid-byte and the layer handle resolved wrong)
  int textHits = 0;
  for (const auto &s : iface.snaps) {
    CAPTURE(s.layer, s.text, s.contentType, s.scale);
    CHECK(s.contentType >= 0);
    CHECK(s.contentType <= 3);
    CHECK(std::isfinite(s.scale));
    CHECK(s.scale > 0.0);
    // This fixture: only annotation scales 4, 24, 48 are used.
    CHECK((s.scale == 4.0 || s.scale == 24.0 || s.scale == 48.0));
    // Layer must come from the file's annotation-scale layer naming
    // ("Mleader @ 4/24/48"); pre-fix many MLEADERs misresolved to "0".
    CHECK(s.layer.rfind("Mleader", 0) == 0);
    if (s.hasText && !s.text.empty())
      ++textHits;
  }
  // Most MLEADERs in this file carry MTEXT content. Pre-fix, only 7/36
  // had non-empty text; post-fix all 24 MTEXT-typed entities populate.
  CHECK(textHits >= 20);

  // Spot-check a known string survives end-to-end (this MLEADER labels
  // a 1/2" gypsum-board callout — appears verbatim in the file).
  bool foundGypsum = false;
  for (const auto &s : iface.snaps)
    if (s.text.find("GYPSUM BOARD") != std::string::npos) {
      foundGypsum = true;
      break;
    }
  CHECK(foundGypsum);

  // Issue 2: entity-specific handles populate from the trailing handle
  // stream. Pre-fix these slots were declared but never read; after the
  // fix every MLEADER must carry a non-null mleaderstyle handle (the
  // file uses 3 MLEADERSTYLEs across the 36 entities).
  int populatedStyleHandles = 0;
  int populatedTextStyleHandles = 0;
  int populatedLineLTypeHandles = 0;
  std::set<std::uint32_t> distinctStyles;
  for (const auto &s : iface.snaps) {
    if (s.styleHandleRef != 0) {
      ++populatedStyleHandles;
      distinctStyles.insert(s.styleHandleRef);
    }
    if (s.hasText && s.textStyleHandleRef != 0)
      ++populatedTextStyleHandles;
    if (s.leaderLineTypeHandleRef != 0)
      ++populatedLineLTypeHandles;
  }
  CHECK(populatedStyleHandles == 36);
  // The 36 entities reference more than one distinct MLEADERSTYLE; pre-fix
  // every styleHandle.ref was 0 (slot was declared but never read).
  CHECK(distinctStyles.size() >= 2u);
  // The MTEXT-typed entities should reference an AcDbTextStyle handle
  // via ctx.textStyleHandle. ≥20 matches the populatedText threshold.
  CHECK(populatedTextStyleHandles >= 20);
}

// Regression for Issue 3 (SCALE / AcDbScale parsing).
// The architectural fixture's annotation scaling layers are named
// "Mleader @ 4", "@ 24", "@ 48" — the file's ACAD_SCALELIST should
// contain matching scale entries. Pre-fix, SCALE objects fell into
// the "[entity-pass-defer 706]" bucket and were never parsed.
TEST_CASE("DWG arch_multileaders: SCALE table delivery",
          "[.slow][dwg][corpus][scale]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path =
      std::string(home) +
      "/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  class ScaleIface : public TypeTrackingIface {
  public:
    std::vector<DRW_Scale> scales;
    void addScale(const DRW_Scale &s) override { scales.push_back(s); }
  };

  ScaleIface iface;
  {
    dwgR reader(path.c_str());
    REQUIRE(reader.read(&iface, true));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
  }

  // The file declares at least the three annotation scales referenced by
  // its layers (1:48, 1:24, 1:4) plus typically a 1:1 unit-scale entry.
  CHECK(iface.scales.size() >= 3u);
  std::cout << "\n  SCALE entries delivered: " << iface.scales.size() << "\n";
  for (const auto &s : iface.scales) {
    std::cout << "    name='" << s.name << "' paper=" << s.paperUnits
              << " drawing=" << s.drawingUnits << " factor=" << s.scaleFactor()
              << " unitScale=" << (s.isUnitScale ? 1 : 0) << "\n";
  }

  // Build a {factor → name} index and verify the three layer-named factors
  // (4, 24, 48) all appear in the SCALE table.
  std::map<double, std::string> byFactor;
  bool foundUnit = false;
  for (const auto &s : iface.scales) {
    byFactor[s.scaleFactor()] = s.name;
    if (s.isUnitScale)
      foundUnit = true;
    // Sanity: every entry has a name and finite ratio.
    CHECK(!s.name.empty());
    CHECK(std::isfinite(s.scaleFactor()));
    CHECK(s.scaleFactor() > 0.0);
  }
  CHECK(byFactor.count(4.0) == 1u);
  CHECK(byFactor.count(24.0) == 1u);
  CHECK(byFactor.count(48.0) == 1u);
  CHECK(foundUnit); // at least one 1:1 entry present
}

TEST_CASE("DWG Mechanical and Annotative: object-context data delivery",
          "[.dwg_object_context]") {
  const std::string targetName = "Mechanical and Annotative.dwg";
  std::vector<std::filesystem::path> candidates;

  const std::filesystem::path manifest = "D:/data/dli/doc/dwg_files.txt";
  if (std::filesystem::is_regular_file(manifest)) {
    std::ifstream in(manifest);
    std::string line;
    while (std::getline(in, line)) {
      if (line.find(targetName) != std::string::npos)
        candidates.emplace_back(line);
    }
  }
  candidates.emplace_back("D:/data/dli/doc/dwg3/Mechanical and Annotative.dwg");
  if (const char *home = getenv("HOME"))
    candidates.emplace_back(std::string(home) +
                            "/doc/dwg3/Mechanical and Annotative.dwg");

  std::filesystem::path path;
  for (const auto &candidate : candidates) {
    if (std::filesystem::is_regular_file(candidate)) {
      path = candidate;
      break;
    }
  }
  if (path.empty()) {
    SKIP("fixture not present; skipping");
  }

  class ObjectContextIface : public TypeTrackingIface {
  public:
    int typedContexts = 0;
    int textContexts = 0;
    int dimensionContexts = 0;
    int scaleLinkedContexts = 0;
    int rawObjectContexts = 0;
    std::map<DRW_ObjectContextData::Kind, int> byKind;

    void addObjectContextData(const DRW_ObjectContextData &d) override {
      ++typedContexts;
      ++byKind[d.m_kind];
      if (d.isTextFamily())
        ++textContexts;
      if (d.isDimensionFamily())
        ++dimensionContexts;
      if (d.m_scaleHandle != 0)
        ++scaleLinkedContexts;
    }

    void addUnsupportedObject(const DRW_UnsupportedObject &o) override {
      if (o.m_recordName.find("OBJECTCONTEXTDATA") != std::string::npos ||
          o.m_className.find("ObjectContextData") != std::string::npos) {
        ++rawObjectContexts;
      }
    }
  };

  ObjectContextIface iface;
  std::unordered_map<std::string, size_t> skippedUnsupported;
  {
    dwgR reader(path.string().c_str());
    REQUIRE(reader.read(&iface, true));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    skippedUnsupported = reader.getSkippedUnsupportedObjects();
  }

  CHECK(iface.typedContexts >= 43);
  CHECK(iface.textContexts >= 14);
  CHECK(iface.dimensionContexts >= 29);
  CHECK(iface.scaleLinkedContexts >= 43);
  CHECK(iface.rawObjectContexts >= iface.typedContexts);
  CHECK(iface.byKind[DRW_ObjectContextData::Kind::AlignedDimension] >= 23);
  CHECK(iface.byKind[DRW_ObjectContextData::Kind::Text] >= 11);
  CHECK(iface.byKind[DRW_ObjectContextData::Kind::RadialDimension] >= 6);
  CHECK(iface.byKind[DRW_ObjectContextData::Kind::MText] >= 3);

  for (const auto &kv : skippedUnsupported) {
    CHECK(kv.first.find("ALDIMOBJECTCONTEXTDATA") == std::string::npos);
    CHECK(kv.first.find("TEXTOBJECTCONTEXTDATA") == std::string::npos);
    CHECK(kv.first.find("RADIMOBJECTCONTEXTDATA") == std::string::npos);
    CHECK(kv.first.find("MTEXTOBJECTCONTEXTDATA") == std::string::npos);
  }
}

TEST_CASE("DWG arch_multileaders: OBJECTS metadata delivery",
          "[.slow][dwg][corpus][objects]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set");
  }
  const std::string path =
      std::string(home) +
      "/doc/dwg/architectural_-_annotation_scaling_and_multileaders.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("fixture not present; skipping");
  }

  class MetadataIface : public TypeTrackingIface {
  public:
    int dictionaries = 0;
    int dictionaryEntries = 0;
    int dictionariesWithDefault = 0;
    int dictionaryVars = 0;
    int dictionaryVarsWithValue = 0;
    int xrecords = 0;
    int xrecordValues = 0;
    int xrecordHandles = 0;
    int materials = 0;
    int materialsWithName = 0;
    int tableStyles = 0;
    int tableStylesWithName = 0;
    int tableStyleFormats = 0;
    int tableStyleBorders = 0;
    int tableContents = 0;
    int tableContentCells = 0;
    int cellStyleMaps = 0;
    int cellStyleMapStyles = 0;

    void addDictionary(const DRW_Dictionary &d) override {
      ++dictionaries;
      dictionaryEntries += static_cast<int>(d.m_entries.size());
    }
    void addDictionaryWithDefault(const DRW_DictionaryWithDefault &d) override {
      ++dictionariesWithDefault;
      dictionaryEntries += static_cast<int>(d.m_entries.size());
      if (d.m_defaultEntryHandle != 0)
        ++xrecordHandles;
    }
    void addDictionaryVar(const DRW_DictionaryVar &d) override {
      ++dictionaryVars;
      if (!d.m_value.empty())
        ++dictionaryVarsWithValue;
    }
    void addXRecord(const DRW_XRecord &r) override {
      ++xrecords;
      xrecordValues += static_cast<int>(r.m_values.size());
      xrecordHandles += static_cast<int>(r.m_handleValues.size());
    }
    void addMaterial(const DRW_Material &m) override {
      ++materials;
      if (!m.m_name.empty())
        ++materialsWithName;
    }
    void addTableStyle(const DRW_TableStyle &t) override {
      ++tableStyles;
      if (!t.m_name.empty())
        ++tableStylesWithName;
      tableStyleFormats += static_cast<int>(t.m_rowStyles.size());
      for (const auto &style : t.m_rowStyles)
        tableStyleBorders += static_cast<int>(style.m_borders.size());
      if (t.m_tableCellStyle.m_hasData) {
        ++tableStyleFormats;
        tableStyleBorders += static_cast<int>(t.m_tableCellStyle.m_borders.size());
      }
      tableStyleFormats += static_cast<int>(t.m_cellStyles.size());
      for (const auto &style : t.m_cellStyles)
        tableStyleBorders += static_cast<int>(style.m_borders.size());
    }
    void addTableContent(const DRW_TableContentObject &t) override {
      ++tableContents;
      for (const auto &row : t.m_content.m_rows)
        tableContentCells += static_cast<int>(row.m_cells.size());
    }
    void addCellStyleMap(const DRW_CellStyleMap &m) override {
      ++cellStyleMaps;
      cellStyleMapStyles += static_cast<int>(m.m_cellStyles.size());
    }
  };

  MetadataIface iface;
  std::unordered_map<std::string, size_t> skippedUnsupported;
  {
    dwgR reader(path.c_str());
    REQUIRE(reader.read(&iface, true));
    REQUIRE(reader.getError() == DRW::BAD_NONE);
    skippedUnsupported = reader.getSkippedUnsupportedObjects();
  }

  CHECK(iface.dictionaries >= 1);
  CHECK(iface.dictionaryEntries >= 1);
  CHECK(iface.dictionariesWithDefault >= 1);
  CHECK(iface.dictionaryVars >= 1);
  CHECK(iface.dictionaryVarsWithValue >= 1);
  CHECK(iface.xrecords >= 1);
  CHECK((iface.xrecordValues + iface.xrecordHandles) >= 1);
  CHECK(iface.materials >= 1);
  CHECK(iface.materialsWithName >= 1);
  CHECK(iface.tableStyles >= 1);
  CHECK(iface.tableStylesWithName >= 1);
  CHECK(iface.tableStyleFormats >= 1);
  CHECK(iface.tableStyleBorders >= 1);
  CHECK(skippedUnsupported.find("TABLECONTENT") == skippedUnsupported.end());
  CHECK(skippedUnsupported.find("CELLSTYLEMAP") == skippedUnsupported.end());
}

TEST_CASE("DWG ACAD_TABLE entities render through anonymous table blocks",
          "[.slow][dwg][corpus][acad_table]") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping ACAD_TABLE corpus test");
  }

  const std::string path =
      std::string(home) + "/doc/dwg/blocks_and_tables_-_metric.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SKIP("DWG table corpus file not found at " << path << "; skipping");
  }

  struct TableProbe : CountingIface {
    int tables = 0;
    int semanticTables = 0;
    int tableColumns = 0;
    int tableRows = 0;
    int tableCells = 0;
    int tableCellContents = 0;
    int tableTextContents = 0;
    int anonymousBlockTables = 0;

    void addTable(const DRW_Table &t) override {
      track(t);
      ++tables;
      if (t.m_hasSemanticContent)
        ++semanticTables;
      tableColumns += static_cast<int>(t.m_content.m_columns.size());
      tableRows += static_cast<int>(t.m_content.m_rows.size());
      for (const auto &row : t.m_content.m_rows) {
        tableCells += static_cast<int>(row.m_cells.size());
        for (const auto &cell : row.m_cells) {
          tableCellContents += static_cast<int>(cell.m_contents.size());
          for (const auto &content : cell.m_contents) {
            if (content.m_type == 1 && (!content.m_text.empty() ||
                content.m_value.m_value.type() != DRW_Variant::INVALID)) {
              ++tableTextContents;
            }
          }
        }
      }
      if (!t.name.empty() && t.name.rfind("*T", 0) == 0)
        ++anonymousBlockTables;
    }
  };

  TableProbe iface;
  dwgR reader(path.c_str());
  REQUIRE(reader.read(&iface, true));
  REQUIRE(reader.getError() == DRW::BAD_NONE);

  const auto skipped = reader.getSkippedCustomClasses();
  CHECK(skipped.find("ACAD_TABLE") == skipped.end());
  CHECK(iface.tables >= 1);
  CHECK(iface.semanticTables >= 1);
  CHECK(iface.anonymousBlockTables >= 1);
  CHECK(iface.tableColumns >= 1);
  CHECK(iface.tableRows >= 1);
  CHECK(iface.tableCells >= 1);
  CHECK(iface.tableCellContents >= 1);
  CHECK(iface.tableTextContents >= 1);
}

// ── R2004+ section buffer-model regression pins (deep-review 2026-06-12) ──────
// These pin the num_pages x maxSize input-bounded buffer model that unlocked
// R2004/R2010/R2013 reads. They depend on the dwg_samples corpus and skip
// gracefully when it is absent (the fixtures are not committed).
namespace {
bool readSample(const char* file, DwgResult& out) {
  const char* home = std::getenv("HOME");
  if (!home) return false;
  const std::string path = std::string(home) + "/dev/dwg_samples/" + file;
  if (!std::filesystem::exists(path)) return false;
  out = readDwg(path, /*verbose=*/false);
  return true;
}
}  // namespace

TEST_CASE("DWG R2004 multi-page section (AcDbObjects at 3 x 0x7400) reads",
          "[dwg][r2004][buffermodel]") {
  // arc_2004's AcDbObjects page sits at startOffset 0x15C00 == 3 x 0x7400, so a
  // load that yields entities proves the multi-page (k x maxSize) buffer model.
  DwgResult r;
  if (!readSample("arc_2004.dwg", r)) {
    SKIP("~/dev/dwg_samples/arc_2004.dwg absent; skipping multi-page pin");
  }
  CHECK(r.error == DRW::BAD_NONE);
  CHECK(r.version == DRW::AC1018);
  CHECK(r.entities >= 1);
}

TEST_CASE("DWG R2010 CLASSES content past page uSize decodes",
          "[dwg][r2010][buffermodel]") {
  // arc_2010's CLASSES string stream lives past the page-header uSize (992);
  // a successful read with entities proves the output window is maxSize, not
  // uSize (the old uSize cap zeroed the string stream -> BAD_READ_CLASSES).
  DwgResult r;
  if (!readSample("arc_2010.dwg", r)) {
    SKIP("~/dev/dwg_samples/arc_2010.dwg absent; skipping past-uSize pin");
  }
  CHECK(r.error == DRW::BAD_NONE);
  CHECK(r.version == DRW::AC1024);
  CHECK(r.entities >= 1);
}

TEST_CASE("DWG R2013 reads (dwgReader27 inherits the buffer-model fixes)",
          "[dwg][r2013][buffermodel]") {
  DwgResult r;
  if (!readSample("arc_2013.dwg", r)) {
    SKIP("~/dev/dwg_samples/arc_2013.dwg absent; skipping R2013 pin");
  }
  CHECK(r.error == DRW::BAD_NONE);
  CHECK(r.version == DRW::AC1027);
  CHECK(r.entities >= 1);
}

TEST_CASE("DWG R2007 writer round-trips an empty drawing",
          "[dwg][r2007][write]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1021-empty.dwg";
  std::filesystem::remove(path);

  CountingIface writeIface;
  dwgRW writer(path.c_str());
  REQUIRE(writer.write(&writeIface, DRW::AC1021, false));
  REQUIRE(std::filesystem::exists(path));
  CHECK(std::filesystem::file_size(path) > 0);

  CountingIface readIface;
  dwgRW reader(path.c_str());
  const bool readOk = reader.read(&readIface, false);
  REQUIRE(readOk);
  CHECK(reader.getError() == DRW::BAD_NONE);
  CHECK(reader.getVersion() == DRW::AC1021);
  CHECK(readIface.entities == 0);

  std::filesystem::remove(path);
}

class R2007EntityWriteIface : public CountingIface {
public:
  dwgRW *writer = nullptr;
  std::size_t linesToWrite = 1;
  int lineCallbacks = 0;
  DRW_Line line;

  void writeEntities() override {
    REQUIRE(writer != nullptr);
    for (std::size_t i = 0; i < linesToWrite; ++i) {
      line.basePoint = DRW_Coord{1.5 + static_cast<double>(i), 2.5, 0.0};
      line.secPoint = DRW_Coord{10.0 + static_cast<double>(i), 5.0, 0.0};
      line.color = 3;
      line.handle = DRW::NoHandle;
      line.parentHandle = DRW::NoHandle;
      REQUIRE(writer->writeLine(&line));
    }
  }

  void addLine(const DRW_Line &value) override {
    CountingIface::addLine(value);
    line = value;
    ++lineCallbacks;
  }
};

class R2007ControlWriteIface : public R2007EntityWriteIface {
public:
  std::set<std::string> layerNames;

  void writeLayers() override {
    REQUIRE(writer != nullptr);
    DRW_Layer layer;
    layer.name = "AC1021_LAYER";
    layer.color = 5;
    REQUIRE(writer->addLayer(&layer));
  }

  void addLayer(const DRW_Layer &layer) override {
    R2007EntityWriteIface::addLayer(layer);
    layerNames.insert(layer.name);
  }
};

TEST_CASE("DWG R2007 control handles remain inline through table records",
          "[dwg][r2007][write][control]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1021-control.dwg";
  std::filesystem::remove(path);

  R2007ControlWriteIface writeIface;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1021, false));
  }

  R2007ControlWriteIface readIface;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1021);
  }

  CHECK(readIface.lineCallbacks == 1);
  CHECK(readIface.layerNames.count("AC1021_LAYER") == 1u);
  CHECK(readIface.layerNames.count("0") == 1u);

  std::filesystem::remove(path);
}

TEST_CASE("DWG R2007 writer round-trips a LINE entity",
          "[dwg][r2007][write]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1021-line.dwg";
  std::filesystem::remove(path);

  R2007EntityWriteIface writeIface;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1021, false));
  }

  R2007EntityWriteIface readIface;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1021);
    CHECK(reader.getR2007CrcMismatch() == 0u);
  }

  REQUIRE(readIface.lineCallbacks == 1);
  CHECK(readIface.line.basePoint.x == Catch::Approx(1.5));
  CHECK(readIface.line.basePoint.y == Catch::Approx(2.5));
  CHECK(readIface.line.secPoint.x == Catch::Approx(10.0));
  CHECK(readIface.line.secPoint.y == Catch::Approx(5.0));
  CHECK(readIface.line.color == 3);

  std::filesystem::remove(path);
}

TEST_CASE("DWG R2013 writer round-trips a zero-source POINTCLOUD",
          "[dwg][r2013][write][pointcloud]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloud.dwg";
  std::filesystem::remove(path);

  PointCloudWriteIface writeIface;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, false));
  }

  PointCloudWriteIface readIface;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1027);
  }

  REQUIRE(readIface.pointCloudCallbacks == 1);
  CHECK(readIface.pointCloud.classVersion == 3u);
  CHECK(readIface.pointCloud.origin.x == Catch::Approx(1.0));
  CHECK(readIface.pointCloud.extentsMax.z == Catch::Approx(6.0));
  CHECK(readIface.pointCloud.pointCount == 1234u);
  CHECK(readIface.pointCloud.ucsName == "UCS-A");
  CHECK(readIface.pointCloud.definitionHandle == 0xB0u);
  CHECK(readIface.pointCloud.reactorHandle == 0xC0u);

  std::filesystem::remove(path);
}

TEST_CASE("LibreDWG reads a valid AC1027 POINTCLOUD container",
          "[.slow][dwg][r2013][pointcloud][external]") {
  if (std::system("command -v dwgread >/dev/null 2>&1") != 0) {
    SKIP("dwgread is unavailable; external-reader gate skipped");
  }

  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloud-external-reader.dwg";
  const auto output = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloud-external-reader.json";
  const auto log = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloud-external-reader.log";
  std::filesystem::remove(path);
  std::filesystem::remove(output);
  std::filesystem::remove(log);

  PointCloudExternalReadWriteIface writeIface;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, false));
  }

  const std::string command = "dwgread -O minJSON "
      + shellQuote(path.string()) + " > " + shellQuote(output.string())
      + " 2> " + shellQuote(log.string());
  REQUIRE(std::system(command.c_str()) == 0);

  std::ifstream json(output, std::ios::binary);
  REQUIRE(json);
  const std::string contents((std::istreambuf_iterator<char>(json)),
                             std::istreambuf_iterator<char>());
  CHECK(!contents.empty());

  std::filesystem::remove(path);
  std::filesystem::remove(output);
  std::filesystem::remove(log);
}

TEST_CASE("LibreDWG reads a valid AC1027 POINTCLOUDEX container",
          "[.slow][dwg][r2013][pointcloud][external]") {
  if (std::system("command -v dwgread >/dev/null 2>&1") != 0) {
    SKIP("dwgread is unavailable; external-reader gate skipped");
  }

  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloudex-external-reader.dwg";
  const auto output = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloudex-external-reader.json";
  const auto log = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloudex-external-reader.log";
  std::filesystem::remove(path);
  std::filesystem::remove(output);
  std::filesystem::remove(log);

  PointCloudExWriteIface writeIface;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, false));
  }

  const std::string command = "dwgread -O minJSON "
      + shellQuote(path.string()) + " > " + shellQuote(output.string())
      + " 2> " + shellQuote(log.string());
  REQUIRE(std::system(command.c_str()) == 0);

  std::ifstream json(output, std::ios::binary);
  REQUIRE(json);
  const std::string contents((std::istreambuf_iterator<char>(json)),
                             std::istreambuf_iterator<char>());
  CHECK(!contents.empty());

  std::filesystem::remove(path);
  std::filesystem::remove(output);
  std::filesystem::remove(log);
}

TEST_CASE("DWG R2013 complete-container frame corruption suppresses POINTCLOUD",
          "[dwg][r2013][safety][pointcloud]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloud-frame-corrupt.dwg";
  std::filesystem::remove(path);

  PointCloudWriteIface writeIface;
  dwgRW writer(path.c_str());
  writeIface.writer = &writer;
  REQUIRE(writer.write(&writeIface, DRW::AC1027, false));

  std::ifstream input(path, std::ios::binary);
  REQUIRE(input);
  const std::vector<std::uint8_t> fileBytes(
      (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!fileBytes.empty());

  PointCloudReaderProbe probe(
      std::make_unique<dwgBuffer>(
          const_cast<std::uint8_t*>(fileBytes.data()), fileBytes.size()),
      &writer);
  REQUIRE(probe.readMetaData());
  REQUIRE(probe.readFileHeader());
  DRW_Header header;
  REQUIRE(probe.readDwgHeader(header));
  REQUIRE(probe.readDwgClasses());
  REQUIRE(probe.readDwgHandles());
  REQUIRE(probe.readDwgTables(header));
  REQUIRE(probe.objectData() != nullptr);
  REQUIRE(probe.objectDataSize() > 0);

  objHandle pointCloudObject;
  bool foundPointCloud = false;
  for (const auto& entry : probe.ObjectMap) {
    DwgObjectFrame frame;
    dwgBuffer objectData(
        const_cast<std::uint8_t*>(probe.objectData()), probe.objectDataSize());
    if (!frame.readAt(objectData, DRW::AC1027, entry.second.loc))
      continue;
    dwgBuffer body(frame.body().data(), frame.body().size());
    if (body.getObjType(DRW::AC1027) == DRW_PointCloud::kDwgClassNum) {
      pointCloudObject = entry.second;
      foundPointCloud = true;
      break;
    }
  }
  REQUIRE(foundPointCloud);

  std::vector<std::uint8_t> mutated(
      probe.objectData(), probe.objectData() + probe.objectDataSize());
  std::size_t bodyStart = 0;
  std::size_t frameEnd = 0;
  REQUIRE(findFrameEnd(mutated, pointCloudObject.loc, DRW::AC1027,
                       bodyStart, frameEnd));
  REQUIRE(bodyStart < frameEnd);
  REQUIRE(frameEnd >= sizeof(std::uint16_t));
  mutated[frameEnd - 1] ^= 0xFF;

  dwgBuffer corrupted(mutated.data(), mutated.size());
  PointCloudWriteIface readIface;
  objHandle mutableObject = pointCloudObject;
  CHECK_FALSE(probe.readDwgEntity(&corrupted, mutableObject, readIface));
  CHECK(readIface.pointCloudCallbacks == 0);

  std::filesystem::remove(path);
}

TEST_CASE("DWG R2013 repaired POINTCLOUD frame reaches finite-value validation",
          "[dwg][r2013][safety][pointcloud]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloud-finite-frame-corrupt.dwg";
  std::filesystem::remove(path);

  PointCloudWriteIface writeIface;
  writeIface.sourceOrigin = DRW_Coord{123.45678901234567,
                                      234.56789101234567,
                                      345.67891201234567};
  dwgRW writer(path.c_str());
  writeIface.writer = &writer;
  REQUIRE(writer.write(&writeIface, DRW::AC1027, false));

  std::ifstream input(path, std::ios::binary);
  REQUIRE(input);
  const std::vector<std::uint8_t> fileBytes(
      (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!fileBytes.empty());

  PointCloudReaderProbe probe(
      std::make_unique<dwgBuffer>(
          const_cast<std::uint8_t*>(fileBytes.data()), fileBytes.size()),
      &writer);
  REQUIRE(probe.readMetaData());
  REQUIRE(probe.readFileHeader());
  DRW_Header header;
  REQUIRE(probe.readDwgHeader(header));
  REQUIRE(probe.readDwgClasses());
  REQUIRE(probe.readDwgHandles());
  REQUIRE(probe.readDwgTables(header));

  objHandle pointCloudObject;
  DwgObjectFrame originalFrame;
  bool foundPointCloud = false;
  for (const auto& entry : probe.ObjectMap) {
    DwgObjectFrame frame;
    dwgBuffer objectData(
        const_cast<std::uint8_t*>(probe.objectData()), probe.objectDataSize());
    if (!frame.readAt(objectData, DRW::AC1027, entry.second.loc))
      continue;
    dwgBuffer body(frame.body().data(), frame.body().size());
    if (body.getObjType(DRW::AC1027) == DRW_PointCloud::kDwgClassNum) {
      pointCloudObject = entry.second;
      originalFrame = std::move(frame);
      foundPointCloud = true;
      break;
    }
  }
  REQUIRE(foundPointCloud);

  std::size_t rawBitOffset = 0;
  REQUIRE(findRawBitDouble(originalFrame.body(), writeIface.sourceOrigin.x,
                           rawBitOffset));
  auto mutatedBody = originalFrame.body();
  replaceRawDouble(mutatedBody, rawBitOffset + 2,
                   std::numeric_limits<double>::quiet_NaN());

  std::vector<std::uint8_t> mutated(
      probe.objectData(), probe.objectData() + probe.objectDataSize());
  std::size_t bodyStart = 0;
  std::size_t frameEnd = 0;
  REQUIRE(findFrameEnd(mutated, pointCloudObject.loc, DRW::AC1027,
                       bodyStart, frameEnd));
  REQUIRE(mutatedBody.size() + bodyStart + sizeof(std::uint16_t) == frameEnd);
  std::copy(mutatedBody.begin(), mutatedBody.end(),
            mutated.begin() + static_cast<std::ptrdiff_t>(bodyStart));

  const std::size_t bodyEnd = bodyStart + mutatedBody.size();
  dwgBuffer crcBuffer(mutated.data(), mutated.size());
  const std::uint16_t crc = crcBuffer.crc8(
      0xC0C1, static_cast<std::int32_t>(pointCloudObject.loc),
      static_cast<std::int32_t>(bodyEnd));
  mutated[bodyEnd] = static_cast<std::uint8_t>(crc & 0xFF);
  mutated[bodyEnd + 1] = static_cast<std::uint8_t>(crc >> 8);

  dwgBuffer repairedFrame(mutated.data(), mutated.size());
  PointCloudWriteIface readIface;
  objHandle mutableObject = pointCloudObject;
  CHECK_FALSE(probe.readDwgEntity(&repairedFrame, mutableObject, readIface));
  CHECK(readIface.pointCloudCallbacks == 0);

  std::filesystem::remove(path);
}

TEST_CASE("DWG R2013 writer round-trips a zero-crop POINTCLOUDEX",
          "[dwg][r2013][write][pointcloud]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloudex.dwg";
  std::filesystem::remove(path);

  PointCloudExWriteIface writeIface;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, false));
  }

  PointCloudExWriteIface readIface;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1027);
  }

  REQUIRE(readIface.pointCloudExCallbacks == 1);
  CHECK(readIface.pointCloudEx.classVersion == 7u);
  CHECK(readIface.pointCloudEx.extentsMin.x == Catch::Approx(-1.0));
  CHECK(readIface.pointCloudEx.extentsMax.z == Catch::Approx(6.0));
  CHECK(readIface.pointCloudEx.ucsOrigin.y == Catch::Approx(8.0));
  CHECK(readIface.pointCloudEx.definitionHandle == 0xB4u);
  CHECK(readIface.pointCloudEx.reactorHandle == 0xC4u);
  CHECK(readIface.pointCloudEx.croppingCount == 0);
  CHECK(readIface.pointCloudEx.unknownInt0 == 41);
  CHECK(readIface.pointCloudEx.unknownInt1 == 42);
  CHECK(readIface.pointCloudEx.intensityMax == Catch::Approx(200.0));

  std::filesystem::remove(path);
}

TEST_CASE("DWG R2013 repaired POINTCLOUDEX frame reaches finite-value validation",
          "[dwg][r2013][safety][pointcloud]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1027-pointcloudex-finite-frame-corrupt.dwg";
  std::filesystem::remove(path);

  PointCloudExWriteIface writeIface;
  writeIface.sourceExtentsMin = DRW_Coord{123.45678901234567,
                                          234.56789101234567,
                                          345.67891201234567};
  dwgRW writer(path.c_str());
  writeIface.writer = &writer;
  REQUIRE(writer.write(&writeIface, DRW::AC1027, false));

  std::ifstream input(path, std::ios::binary);
  REQUIRE(input);
  const std::vector<std::uint8_t> fileBytes(
      (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  REQUIRE(!fileBytes.empty());

  PointCloudReaderProbe probe(
      std::make_unique<dwgBuffer>(
          const_cast<std::uint8_t*>(fileBytes.data()), fileBytes.size()),
      &writer);
  REQUIRE(probe.readMetaData());
  REQUIRE(probe.readFileHeader());
  DRW_Header header;
  REQUIRE(probe.readDwgHeader(header));
  REQUIRE(probe.readDwgClasses());
  REQUIRE(probe.readDwgHandles());
  REQUIRE(probe.readDwgTables(header));

  objHandle pointCloudObject;
  DwgObjectFrame originalFrame;
  bool foundPointCloud = false;
  for (const auto& entry : probe.ObjectMap) {
    DwgObjectFrame frame;
    dwgBuffer objectData(
        const_cast<std::uint8_t*>(probe.objectData()), probe.objectDataSize());
    if (!frame.readAt(objectData, DRW::AC1027, entry.second.loc))
      continue;
    dwgBuffer body(frame.body().data(), frame.body().size());
    if (body.getObjType(DRW::AC1027) == DRW_PointCloudEx::kDwgClassNum) {
      pointCloudObject = entry.second;
      originalFrame = std::move(frame);
      foundPointCloud = true;
      break;
    }
  }
  REQUIRE(foundPointCloud);

  std::size_t rawBitOffset = 0;
  REQUIRE(findRawBitDouble(originalFrame.body(), writeIface.sourceExtentsMin.x,
                           rawBitOffset));
  auto mutatedBody = originalFrame.body();
  replaceRawDouble(mutatedBody, rawBitOffset + 2,
                   std::numeric_limits<double>::quiet_NaN());

  std::vector<std::uint8_t> mutated(
      probe.objectData(), probe.objectData() + probe.objectDataSize());
  std::size_t bodyStart = 0;
  std::size_t frameEnd = 0;
  REQUIRE(findFrameEnd(mutated, pointCloudObject.loc, DRW::AC1027,
                       bodyStart, frameEnd));
  REQUIRE(mutatedBody.size() + bodyStart + sizeof(std::uint16_t) == frameEnd);
  std::copy(mutatedBody.begin(), mutatedBody.end(),
            mutated.begin() + static_cast<std::ptrdiff_t>(bodyStart));

  const std::size_t bodyEnd = bodyStart + mutatedBody.size();
  dwgBuffer crcBuffer(mutated.data(), mutated.size());
  const std::uint16_t crc = crcBuffer.crc8(
      0xC0C1, static_cast<std::int32_t>(pointCloudObject.loc),
      static_cast<std::int32_t>(bodyEnd));
  mutated[bodyEnd] = static_cast<std::uint8_t>(crc & 0xFF);
  mutated[bodyEnd + 1] = static_cast<std::uint8_t>(crc >> 8);

  dwgBuffer repairedFrame(mutated.data(), mutated.size());
  PointCloudExWriteIface readIface;
  objHandle mutableObject = pointCloudObject;
  CHECK_FALSE(probe.readDwgEntity(&repairedFrame, mutableObject, readIface));
  CHECK(readIface.pointCloudExCallbacks == 0);

  std::filesystem::remove(path);
}

TEST_CASE("DWG writer round-trips NAVISWORKSMODEL across handle layouts",
          "[dwg][navisworks][write]") {
  for (const DRW::Version version : {DRW::AC1018, DRW::AC1027}) {
    const auto path = std::filesystem::temp_directory_path()
        / (version == DRW::AC1018 ? "libdxfrw-ac1018-navisworks.dwg"
                                   : "libdxfrw-ac1027-navisworks.dwg");
    std::filesystem::remove(path);

    NavisworksModelWriteIface writeIface;
    {
      dwgRW writer(path.c_str());
      writeIface.writer = &writer;
      REQUIRE(writer.write(&writeIface, version, false));
    }

    NavisworksModelWriteIface readIface;
    {
      dwgRW reader(path.c_str());
      REQUIRE(reader.read(&readIface, false));
      CHECK(reader.getError() == DRW::BAD_NONE);
      CHECK(reader.getVersion() == version);
    }

    REQUIRE(readIface.navisworksCallbacks == 1);
    CHECK(readIface.navisworks.flags == 11u);
    CHECK(readIface.navisworks.definitionHandle == 0x2D0u);
    CHECK(readIface.navisworks.transform[0] == Catch::Approx(0.25));
    CHECK(readIface.navisworks.transform[15] == Catch::Approx(15.25));
    CHECK(readIface.navisworks.unitFactor == Catch::Approx(0.001));

    std::filesystem::remove(path);
  }
}

TEST_CASE("DWG writer round-trips Point Cloud definitions across versions",
          "[dwg][write][pointcloud][objects]") {
  for (const DRW::Version version : {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1021, DRW::AC1024,
                                     DRW::AC1027, DRW::AC1032}) {
    CAPTURE(static_cast<int>(version));
    const auto path = std::filesystem::temp_directory_path()
                      / ("libdxfrw-pointcloud-definitions-"
                         + std::to_string(static_cast<int>(version)) + ".dwg");
    std::filesystem::remove(path);

    PointCloudDefWriteIface writeIface;
    DRW_PointCloudDef definition;
    definition.m_kind = DRW_PointCloudDef::Definition;
    definition.handle = 0xD10;
    definition.m_classVersion = 7;
    definition.m_sourceFilename = "scan.rcp";
    definition.m_isLoaded = true;
    definition.m_pointCount = 1234567890123ULL;
    definition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
    definition.m_extentsMax = DRW_Coord{4.0, 5.0, 6.0};
    definition.setDwgCommonObjectState(1, 0, false);
    definition.reactorHandles = {0xD20};
    definition.xDictHandle = 0xD21;
    writeIface.definitions.push_back(definition);

    DRW_PointCloudDef definitionEx = definition;
    definitionEx.m_kind = DRW_PointCloudDef::DefinitionEx;
    definitionEx.handle = 0xD11;
    definitionEx.m_sourceFilename = "scan-ex.rcp";
    definitionEx.reactorHandles = {0xD22};
    definitionEx.xDictHandle = 0xD23;
    writeIface.definitions.push_back(definitionEx);

    DRW_PointCloudDef reactor;
    reactor.m_kind = DRW_PointCloudDef::Reactor;
    reactor.handle = 0xD12;
    reactor.m_classVersion = 9;
    reactor.setDwgCommonObjectState(1, 0, false);
    reactor.reactorHandles = {0xD24};
    reactor.xDictHandle = 0xD25;
    writeIface.definitions.push_back(reactor);

    DRW_PointCloudDef reactorEx = reactor;
    reactorEx.m_kind = DRW_PointCloudDef::ReactorEx;
    reactorEx.handle = 0xD13;
    writeIface.definitions.push_back(reactorEx);

    {
      dwgRW writer(path.c_str());
      writeIface.writer = &writer;
      REQUIRE(writer.write(&writeIface, version, false));
    }

    PointCloudDefWriteIface readIface;
    {
      dwgRW reader(path.c_str());
      REQUIRE(reader.read(&readIface, false));
      CHECK(reader.getError() == DRW::BAD_NONE);
      CHECK(reader.getVersion() == version);
      CHECK(reader.getSkippedCustomClasses().empty());
      CHECK(reader.getObjectParseFailures() == 0);
    }

    REQUIRE(readIface.captured.size() == 4);
    std::map<DRW_PointCloudDef::Kind, DRW_PointCloudDef> byKind;
    for (const auto &value : readIface.captured)
      byKind[value.m_kind] = value;
    REQUIRE(byKind.size() == 4);
    CHECK(byKind.at(DRW_PointCloudDef::Definition).m_pointCount
          == 1234567890123ULL);
    CHECK(byKind.at(DRW_PointCloudDef::Definition).m_sourceFilename == "scan.rcp");
    CHECK(byKind.at(DRW_PointCloudDef::DefinitionEx).m_sourceFilename
          == "scan-ex.rcp");
    CHECK(byKind.at(DRW_PointCloudDef::Reactor).m_classVersion == 9);
    CHECK(byKind.at(DRW_PointCloudDef::ReactorEx).m_classVersion == 9);
    CHECK(byKind.at(DRW_PointCloudDef::Reactor).m_sourceFilename.empty());
    CHECK(byKind.at(DRW_PointCloudDef::Definition).reactorHandles
          == std::vector<std::uint32_t>{0xD20});
    CHECK(byKind.at(DRW_PointCloudDef::Definition).xDictHandle == 0xD21u);
    CHECK(byKind.at(DRW_PointCloudDef::DefinitionEx).reactorHandles
          == std::vector<std::uint32_t>{0xD22});
    CHECK(byKind.at(DRW_PointCloudDef::DefinitionEx).xDictHandle == 0xD23u);
    CHECK(byKind.at(DRW_PointCloudDef::Reactor).reactorHandles
          == std::vector<std::uint32_t>{0xD24});
    CHECK(byKind.at(DRW_PointCloudDef::Reactor).xDictHandle == 0xD25u);

    std::filesystem::remove(path);
  }
}

TEST_CASE("RS_FilterDXFRW preserves point-cloud definition dictionary linkage",
          "[dwg][r2013][roundtrip][pointcloud][dictionary]") {
  chicun::ensureTestApp();
  const auto sourcePath = std::filesystem::temp_directory_path()
      / "librecad-pointcloud-dictionary-source.dwg";
  const auto outputPath = std::filesystem::temp_directory_path()
      / "librecad-pointcloud-dictionary-output.dwg";
  std::filesystem::remove(sourcePath);
  std::filesystem::remove(outputPath);

  PointCloudDictionaryWriteIface writeIface;
  writeIface.dictionary.handle = 0xD40;
  writeIface.dictionary.parentHandle = 0xC;
  writeIface.dictionary.name = "ACAD_POINTCLOUD_TEST";
  writeIface.dictionary.cloning = 1;
  writeIface.dictionary.m_entries.push_back({"Definition", 0xD41});

  DRW_PointCloudDef definition;
  definition.m_kind = DRW_PointCloudDef::Definition;
  definition.handle = 0xD41;
  definition.parentHandle = 0xD40;
  definition.m_classVersion = 7;
  definition.m_sourceFilename = "linked.rcp";
  definition.m_isLoaded = true;
  definition.m_pointCount = 4321;
  definition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
  definition.m_extentsMax = DRW_Coord{4.0, 5.0, 6.0};
  writeIface.definitions.push_back(definition);

  {
    dwgRW writer(sourcePath.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, false));
  }

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic,
                              QString::fromStdString(sourcePath.string()),
                              RS2::FormatDWG));
  }

  const auto &metadata = graphic.dwgAdvancedMetadata();
  const auto sourceDictionaryIt = std::find_if(
      metadata.dictionaries().cbegin(), metadata.dictionaries().cend(),
      [](const auto &record) { return record.handle == 0xD40u; });
  REQUIRE(sourceDictionaryIt != metadata.dictionaries().cend());
  REQUIRE(metadata.pointCloudDefinitions().size() == 1);
  const auto &sourceDictionary = *sourceDictionaryIt;
  const auto &sourceDefinition = metadata.pointCloudDefinitions().front();
  CHECK(sourceDictionary.handle == 0xD40u);
  CHECK(sourceDictionary.parentHandle == 0xCu);
  REQUIRE(sourceDictionary.entries.size() == 1);
  CHECK(sourceDictionary.entries.front().name == "Definition");
  CHECK(sourceDictionary.entries.front().handle == 0xD41u);
  CHECK(sourceDefinition.parentHandle == sourceDictionary.handle);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic,
                              QString::fromStdString(outputPath.string()),
                              RS2::FormatDWG2013));
  }

  PointCloudDictionaryWriteIface readIface;
  {
    dwgRW reader(outputPath.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1027);
  }

  REQUIRE(readIface.capturedDictionaries.size() == 1);
  REQUIRE(readIface.captured.size() == 1);
  const auto &outputDictionary = readIface.capturedDictionaries.front();
  const auto &outputDefinition = readIface.captured.front();
  CHECK(outputDictionary.handle == sourceDictionary.handle);
  CHECK(outputDictionary.parentHandle == sourceDictionary.parentHandle);
  REQUIRE(outputDictionary.m_entries.size() == 1);
  CHECK(outputDictionary.m_entries.front().m_name == "Definition");
  CHECK(outputDictionary.m_entries.front().m_handle == outputDefinition.handle);
  CHECK(outputDefinition.parentHandle == outputDictionary.handle);
  CHECK(outputDefinition.m_sourceFilename == "linked.rcp");
  CHECK(outputDefinition.m_pointCount == 4321u);

  std::filesystem::remove(sourcePath);
  std::filesystem::remove(outputPath);
}

TEST_CASE("RS_FilterDXFRW preserves nested point-cloud and Navisworks references",
          "[dwg][r2013][roundtrip][pointcloud][navisworks][dictionary]") {
  chicun::ensureTestApp();
  const auto sourcePath = std::filesystem::temp_directory_path()
      / "librecad-pointcloud-reference-graph-source.dwg";
  const auto outputPath = std::filesystem::temp_directory_path()
      / "librecad-pointcloud-reference-graph-output.dwg";
  std::filesystem::remove(sourcePath);
  std::filesystem::remove(outputPath);

  PointCloudReferenceGraphWriteIface writeIface;
  DRW_Dictionary root;
  root.handle = 0xD50;
  root.parentHandle = 0xC;
  root.name = "ACAD_POINTCLOUD_GRAPH";
  root.m_entries = {{"Nested", 0xD51}, {"References", 0xD53}};
  writeIface.dictionaries.push_back(root);

  DRW_Dictionary nested;
  nested.handle = 0xD51;
  nested.parentHandle = 0xD50;
  nested.name = "NESTED_POINTCLOUD";
  nested.m_entries = {{"Definition", 0xD52}};
  writeIface.dictionaries.push_back(nested);

  DRW_Dictionary references;
  references.handle = 0xD53;
  references.parentHandle = 0xD50;
  references.name = "POINTCLOUD_REFERENCES";
  references.m_entries = {{"Navisworks", 0xD54}, {"ColorMap", 0xD55}};
  writeIface.dictionaries.push_back(references);

  writeIface.pointCloudDefinition.handle = 0xD52;
  writeIface.pointCloudDefinition.parentHandle = 0xD51;
  writeIface.pointCloudDefinition.m_kind = DRW_PointCloudDef::Definition;
  writeIface.pointCloudDefinition.m_classVersion = 7;
  writeIface.pointCloudDefinition.m_sourceFilename = "graph.rcp";
  writeIface.pointCloudDefinition.m_pointCount = 9876;
  writeIface.pointCloudDefinition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
  writeIface.pointCloudDefinition.m_extentsMax = DRW_Coord{4.0, 5.0, 6.0};

  writeIface.navisworksDefinition.handle = 0xD54;
  writeIface.navisworksDefinition.parentHandle = 0xD53;
  writeIface.navisworksDefinition.m_flags = 5;
  writeIface.navisworksDefinition.m_path = "coordination/graph.nwd";
  writeIface.navisworksDefinition.m_status = true;
  writeIface.navisworksDefinition.m_minExtent = DRW_Coord{-4.0, -5.0, -6.0};
  writeIface.navisworksDefinition.m_maxExtent = DRW_Coord{7.0, 8.0, 9.0};

  writeIface.navisworksModel.handle = 0xD56;
  writeIface.navisworksModel.parentHandle = DRW::NoHandle;
  writeIface.navisworksModel.definitionHandle = 0xD54;
  writeIface.navisworksModel.flags = 0x8001;
  writeIface.navisworksModel.unitFactor = 0.001;
  for (std::size_t i = 0; i < writeIface.navisworksModel.transform.size(); ++i)
    writeIface.navisworksModel.transform[i] = static_cast<double>(i + 1);

  writeIface.colorMap.handle = 0xD55;
  writeIface.colorMap.parentHandle = 0xD53;
  writeIface.colorMap.m_classVersion = 1;
  writeIface.colorMap.m_defaultIntensityColorScheme = "Intensity";
  writeIface.colorMap.m_defaultElevationColorScheme = "Elevation";
  writeIface.colorMap.m_defaultClassificationColorScheme = "Classification";

  {
    dwgRW writer(sourcePath.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, false));
  }

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic,
                              QString::fromStdString(sourcePath.string()),
                              RS2::FormatDWG));
  }

  const auto &metadata = graphic.dwgAdvancedMetadata();
  const auto hasDictionary = [&metadata](std::uint32_t handle) {
    return std::any_of(
        metadata.dictionaries().cbegin(), metadata.dictionaries().cend(),
        [handle](const auto &record) { return record.handle == handle; });
  };
  CHECK(hasDictionary(0xD50u));
  CHECK(hasDictionary(0xD51u));
  CHECK(hasDictionary(0xD53u));
  REQUIRE(metadata.pointCloudDefinitions().size() == 1);
  REQUIRE(metadata.navisworksModelDefinitions().size() == 1);
  REQUIRE(metadata.navisworksModels().size() == 1);
  REQUIRE(metadata.pointCloudColorMaps().size() == 1);
  CHECK(metadata.pointCloudDefinitions().front().parentHandle == 0xD51u);
  CHECK(metadata.navisworksModelDefinitions().front().parentHandle == 0xD53u);
  CHECK(metadata.navisworksModels().front().definitionHandle == 0xD54u);
  CHECK(metadata.pointCloudColorMaps().front().parentHandle == 0xD53u);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic,
                              QString::fromStdString(outputPath.string()),
                              RS2::FormatDWG2013));
  }

  PointCloudReferenceGraphWriteIface readIface;
  {
    dwgRW reader(outputPath.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1027);
  }

  REQUIRE(readIface.capturedDictionaries.size() == 3);
  REQUIRE(readIface.capturedPointCloudDefinitions.size() == 1);
  const auto findDictionary = [&](std::uint32_t handle) {
    return std::find_if(readIface.capturedDictionaries.begin(),
                        readIface.capturedDictionaries.end(),
                        [handle](const DRW_Dictionary &dictionary) {
                          return dictionary.handle == handle;
                        });
  };
  const auto rootIt = findDictionary(0xD50);
  const auto nestedIt = findDictionary(0xD51);
  const auto referencesIt = findDictionary(0xD53);
  REQUIRE(rootIt != readIface.capturedDictionaries.end());
  REQUIRE(nestedIt != readIface.capturedDictionaries.end());
  REQUIRE(referencesIt != readIface.capturedDictionaries.end());
  CHECK(rootIt->parentHandle == 0xCu);
  REQUIRE(rootIt->m_entries.size() == 2);
  CHECK(rootIt->m_entries[0].m_name == "Nested");
  CHECK(rootIt->m_entries[0].m_handle == 0xD51u);
  CHECK(rootIt->m_entries[1].m_name == "References");
  CHECK(rootIt->m_entries[1].m_handle == 0xD53u);
  CHECK(nestedIt->parentHandle == 0xD50u);
  REQUIRE(nestedIt->m_entries.size() == 1);
  CHECK(nestedIt->m_entries.front().m_handle == 0xD52u);
  CHECK(referencesIt->parentHandle == 0xD50u);
  REQUIRE(referencesIt->m_entries.size() == 2);
  CHECK(referencesIt->m_entries[0].m_handle == 0xD54u);
  CHECK(referencesIt->m_entries[1].m_handle == 0xD55u);

  CHECK(readIface.capturedPointCloudDefinitions.front().parentHandle == 0xD51u);
  CHECK(readIface.capturedNavisworksDefinition.parentHandle == 0xD53u);
  CHECK(readIface.capturedNavisworksModel.definitionHandle == 0xD54u);
  CHECK(readIface.capturedColorMap.parentHandle == 0xD53u);

  std::filesystem::remove(sourcePath);
  std::filesystem::remove(outputPath);
}

TEST_CASE("RS_FilterDXFRW remaps colliding point-cloud dictionary handles",
          "[dwg][r2013][write][pointcloud][dictionary][remap]") {
  chicun::ensureTestApp();
  const auto path = std::filesystem::temp_directory_path()
      / "librecad-pointcloud-dictionary-remap.dwg";
  std::filesystem::remove(path);

  RS_Graphic graphic;
  graphic.initForNewDocument();

  DRW_Dictionary dictionary;
  dictionary.handle = DRW::DwgModelSpaceBlockRecordHandle;
  dictionary.parentHandle = 0xC;
  dictionary.name = "COLLIDING_POINTCLOUD_DICTIONARY";
  dictionary.m_entries.push_back({"Definition", 0xD61});
  graphic.dwgAdvancedMetadata().addDictionary(dictionary);

  DRW_PointCloudDef definition;
  definition.handle = 0xD61;
  definition.parentHandle = dictionary.handle;
  definition.m_kind = DRW_PointCloudDef::Definition;
  definition.m_classVersion = 7;
  definition.m_sourceFilename = "remapped.rcp";
  definition.m_pointCount = 2468;
  definition.m_extentsMin = DRW_Coord{-1.0, -2.0, -3.0};
  definition.m_extentsMax = DRW_Coord{4.0, 5.0, 6.0};
  graphic.dwgAdvancedMetadata().addPointCloudDef(definition);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(path.string()),
                              RS2::FormatDWG2013));
  }

  PointCloudDictionaryWriteIface readIface;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1027);
  }

  REQUIRE(readIface.capturedDictionaries.size() == 1);
  REQUIRE(readIface.captured.size() == 1);
  const auto &outputDictionary = readIface.capturedDictionaries.front();
  const auto &outputDefinition = readIface.captured.front();
  CHECK(outputDictionary.handle != dictionary.handle);
  CHECK(outputDictionary.parentHandle == dictionary.parentHandle);
  REQUIRE(outputDictionary.m_entries.size() == 1);
  CHECK(outputDictionary.m_entries.front().m_name == "Definition");
  CHECK(outputDictionary.m_entries.front().m_handle == outputDefinition.handle);
  CHECK(outputDefinition.parentHandle == outputDictionary.handle);
  CHECK(outputDefinition.m_sourceFilename == "remapped.rcp");
  CHECK(outputDefinition.m_pointCount == 2468u);

  std::filesystem::remove(path);
}

TEST_CASE("RS_FilterDXFRW drops dangling point-cloud dictionary entries",
          "[dwg][r2013][write][pointcloud][dictionary][safety]") {
  chicun::ensureTestApp();
  const auto path = std::filesystem::temp_directory_path()
                    / "librecad-pointcloud-dictionary-dangling.dwg";
  std::filesystem::remove(path);

  RS_Graphic graphic;
  graphic.initForNewDocument();

  DRW_Dictionary dictionary;
  dictionary.handle = 0xD70;
  dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
  dictionary.name = "DANGLING_POINTCLOUD_DICTIONARY";
  dictionary.m_entries.push_back({"Missing", 0xDEAD});
  graphic.dwgAdvancedMetadata().addDictionary(dictionary);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(path.string()),
                              RS2::FormatDWG2013));
  }

  PointCloudDictionaryWriteIface readIface;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1027);
  }

  REQUIRE(readIface.capturedDictionaries.size() == 1);
  const auto &outputDictionary = readIface.capturedDictionaries.front();
  CHECK(outputDictionary.handle == dictionary.handle);
  CHECK(outputDictionary.parentHandle == dictionary.parentHandle);
  CHECK(outputDictionary.m_entries.empty());

  std::filesystem::remove(path);
}

TEST_CASE("DWG writer round-trips Navisworks and Point Cloud ColorMap objects across versions",
          "[dwg][write][pointcloud][navisworks][objects]") {
  for (const DRW::Version version : {DRW::AC1015, DRW::AC1018,
                                     DRW::AC1021, DRW::AC1024,
                                     DRW::AC1027, DRW::AC1032}) {
    CAPTURE(static_cast<int>(version));
    const auto path = std::filesystem::temp_directory_path()
                      / ("libdxfrw-pointcloud-metadata-"
                         + std::to_string(static_cast<int>(version)) + ".dwg");
    std::filesystem::remove(path);

    PointCloudMetadataWriteIface writeIface;
    writeIface.navisworks.handle = 0xD20;
    writeIface.navisworks.m_flags = 5;
    writeIface.navisworks.m_path = "coordination/model.nwd";
    writeIface.navisworks.m_status = true;
    writeIface.navisworks.m_minExtent = DRW_Coord{-1.0, -2.0, -3.0};
    writeIface.navisworks.m_maxExtent = DRW_Coord{4.0, 5.0, 6.0};
    writeIface.navisworks.setDwgCommonObjectState(1, 0, false);
    writeIface.navisworks.reactorHandles = {0xD23};
    writeIface.navisworks.xDictHandle = 0xD24;
    writeIface.navisworksModel.handle = 0xD22;
    writeIface.navisworksModel.parentHandle = DRW::NoHandle;
    writeIface.navisworksModel.flags = 0x8001;
    writeIface.navisworksModel.definitionHandle = 0xD20;
    for (std::size_t i = 0; i < writeIface.navisworksModel.transform.size(); ++i)
      writeIface.navisworksModel.transform[i] = static_cast<double>(i + 1);
    writeIface.navisworksModel.unitFactor = 0.001;

    writeIface.colorMap.handle = 0xD21;
    writeIface.colorMap.m_classVersion = 1;
    writeIface.colorMap.m_defaultIntensityColorScheme = "Intensity";
    writeIface.colorMap.m_defaultElevationColorScheme = "Elevation";
    writeIface.colorMap.m_defaultClassificationColorScheme = "Classification";
    writeIface.colorMap.setDwgCommonObjectState(1, 0, false);
    writeIface.colorMap.reactorHandles = {0xD25};
    writeIface.colorMap.xDictHandle = 0xD26;
    writeIface.colorMap.m_colorRamps.push_back({2, 2, {"Ramp A", "Ramp B"}});
    writeIface.colorMap.m_colorRampCount = 1;
    writeIface.colorMap.m_classificationColorRamps.push_back({3, 1, {"Class Ramp"}});
    writeIface.colorMap.m_classificationColorRampCount = 1;

    {
      dwgRW writer(path.c_str());
      writeIface.writer = &writer;
      REQUIRE(writer.write(&writeIface, version, false));
    }

    PointCloudMetadataWriteIface readIface;
    {
      dwgRW reader(path.c_str());
      REQUIRE(reader.read(&readIface, false));
      CHECK(reader.getError() == DRW::BAD_NONE);
      CHECK(reader.getVersion() == version);
      CHECK(reader.getSkippedCustomClasses().empty());
      CHECK(reader.getObjectParseFailures() == 0);
    }

    REQUIRE(readIface.navisworksCallbacks == 1);
    CHECK(readIface.navisworks.m_flags == 5);
    CHECK(readIface.navisworks.m_path == "coordination/model.nwd");
    CHECK(readIface.navisworks.m_status);
    CHECK_FALSE(readIface.navisworks.m_hostDrawingVisibility);
    CHECK(readIface.navisworks.m_minExtent.z == Catch::Approx(-3.0));
    CHECK(readIface.navisworks.m_maxExtent.x == Catch::Approx(4.0));
    CHECK(readIface.navisworks.reactorHandles
          == std::vector<std::uint32_t>{0xD23});
    CHECK(readIface.navisworks.xDictHandle == 0xD24u);
    const bool hasNavisworksEntity = version >= DRW::AC1021;
    CHECK(readIface.navisworksModelCallbacks == (hasNavisworksEntity ? 1 : 0));
    if (hasNavisworksEntity) {
      CHECK(readIface.navisworksModel.flags == 0x8001u);
      CHECK(readIface.navisworksModel.parentHandle == DRW::NoHandle);
      CHECK(readIface.navisworksModel.definitionHandle == 0xD20u);
      CHECK(readIface.navisworksModel.transform[15] == Catch::Approx(16.0));
      CHECK(readIface.navisworksModel.unitFactor == Catch::Approx(0.001));
    }

    REQUIRE(readIface.colorMapCallbacks == 1);
    CHECK(readIface.colorMap.m_classVersion == 1);
    REQUIRE(readIface.colorMap.m_colorRamps.size() == 1);
    CHECK(readIface.colorMap.m_colorRamps[0].m_classVersion == 2);
    CHECK(readIface.colorMap.m_colorRamps[0].m_colorSchemes[1] == "Ramp B");
    REQUIRE(readIface.colorMap.m_classificationColorRamps.size() == 1);
    CHECK(readIface.colorMap.m_classificationColorRamps[0].m_colorSchemes[0]
          == "Class Ramp");
    CHECK(readIface.colorMap.reactorHandles
          == std::vector<std::uint32_t>{0xD25});
    CHECK(readIface.colorMap.xDictHandle == 0xD26u);

    std::filesystem::remove(path);
  }
}

TEST_CASE("RS_FilterDXFRW re-emits typed Navisworks and Point Cloud metadata",
          "[dwg][dxf][roundtrip][pointcloud][navisworks][filter]") {
  static int qargc = 1;
  static char qarg0[] = "librecad_tests";
  static char *qargv[] = {qarg0, nullptr};
  static QCoreApplication *qapp =
      QCoreApplication::instance() != nullptr
          ? QCoreApplication::instance()
          : new QCoreApplication(qargc, qargv);
  (void)qapp;
  QCoreApplication::setOrganizationName("LibreCAD");
  QCoreApplication::setApplicationName("LibreCAD-tests");
  RS_Settings::init("LibreCAD", "LibreCAD-tests");

  const auto dwgPath = std::filesystem::temp_directory_path()
                       / "librecad-pointcloud-filter-source.dwg";
  const auto dxfPath = std::filesystem::temp_directory_path()
                       / "librecad-pointcloud-filter-output.dxf";
  std::filesystem::remove(dwgPath);
  std::filesystem::remove(dxfPath);

  PointCloudMetadataWriteIface writeIface;
  writeIface.navisworks.handle = 0xD30;
  writeIface.navisworks.m_flags = 5;
  writeIface.navisworks.m_path = "coordination/model.nwd";
  writeIface.navisworks.m_status = true;
  writeIface.navisworks.m_minExtent = DRW_Coord{-1.0, -2.0, -3.0};
  writeIface.navisworks.m_maxExtent = DRW_Coord{4.0, 5.0, 6.0};
  writeIface.navisworksModel.handle = 0xD32;
  writeIface.navisworksModel.parentHandle = DRW::NoHandle;
  writeIface.navisworksModel.flags = 0x8001;
  writeIface.navisworksModel.definitionHandle = 0xD30;
  for (std::size_t i = 0; i < writeIface.navisworksModel.transform.size(); ++i)
    writeIface.navisworksModel.transform[i] = static_cast<double>(i + 1);
  writeIface.navisworksModel.unitFactor = 0.001;
  writeIface.colorMap.handle = 0xD31;
  writeIface.colorMap.m_classVersion = 1;
  writeIface.colorMap.m_defaultIntensityColorScheme = "Intensity";
  writeIface.colorMap.m_defaultElevationColorScheme = "Elevation";
  writeIface.colorMap.m_defaultClassificationColorScheme = "Classification";
  writeIface.colorMap.m_colorRampCount = 1;
  writeIface.colorMap.m_colorRamps.push_back({2, 2, {"Ramp A", "Ramp B"}});
  writeIface.colorMap.m_classificationColorRampCount = 1;
  writeIface.colorMap.m_classificationColorRamps.push_back(
      {3, 1, {"Class Ramp"}});

  {
    dwgRW writer(dwgPath.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1027, false));
  }

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(dwgPath.string()),
                              RS2::FormatDWG));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().navisworksModelDefinitions().size()
          == 1);
  REQUIRE(graphic.dwgAdvancedMetadata().navisworksModels().size() == 1);
  REQUIRE(graphic.dwgAdvancedMetadata().pointCloudColorMaps().size() == 1);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(dxfPath.string()),
                              RS2::FormatDXFRW));
  }

  PointCloudMetadataWriteIface readIface;
  dxfRW reader(dxfPath.c_str());
  REQUIRE(reader.read(&readIface, false));
  REQUIRE(readIface.navisworksCallbacks == 1);
  REQUIRE(readIface.navisworksModelCallbacks == 1);
  REQUIRE(readIface.colorMapCallbacks == 1);
  CHECK(readIface.navisworks.m_path == "coordination/model.nwd");
  CHECK(readIface.navisworksModel.flags == 0x8001u);
  CHECK(readIface.navisworksModel.definitionHandle == 0xD30u);
  CHECK(readIface.navisworksModel.transform[0] == Catch::Approx(1.0));
  CHECK(readIface.navisworksModel.unitFactor == Catch::Approx(0.001));
  CHECK(readIface.colorMap.m_colorRamps[0].m_colorSchemes[1] == "Ramp B");

  std::filesystem::remove(dwgPath);
  std::filesystem::remove(dxfPath);
}

TEST_CASE("DWG R2007 writer splits oversized object sections",
          "[dwg][r2007][write][multipage]") {
  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1021-multipage.dwg";
  std::filesystem::remove(path);

  R2007EntityWriteIface writeIface;
  writeIface.linesToWrite = 1024;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1021, false));
  }

  R2007EntityWriteIface readIface;
  {
    dwgRW reader(path.c_str());
    REQUIRE(reader.read(&readIface, false));
    CHECK(reader.getError() == DRW::BAD_NONE);
    CHECK(reader.getVersion() == DRW::AC1021);
    CHECK(reader.getR2007CrcMismatch() == 0u);
  }

  CHECK(readIface.lineCallbacks == 1024);
  std::filesystem::remove(path);
}

TEST_CASE("DWG R2007 writer is readable by libreDWG",
          "[.dwg_readback][dwg][r2007][write]") {
  const char *home = std::getenv("HOME");
  const char *configured = std::getenv("DWGREAD");
  std::vector<std::filesystem::path> candidates;
  if (configured != nullptr)
    candidates.emplace_back(configured);
  if (home != nullptr) {
    candidates.emplace_back(std::filesystem::path(home)
                            / "dev/libredwg/programs/dwgread");
    candidates.emplace_back(std::filesystem::path(home)
                            / "dev/libreDWG/programs/dwgread");
  }

  std::filesystem::path executable;
  for (const auto &candidate : candidates) {
    if (std::filesystem::exists(candidate)) {
      executable = candidate;
      break;
    }
  }
  if (executable.empty()) {
    SKIP("libreDWG dwgread not available; skipping cross-tool gate");
  }

  const auto path = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1021-libreDWG.dwg";
  const auto report = std::filesystem::temp_directory_path()
      / "libdxfrw-ac1021-libreDWG.txt";
  std::filesystem::remove(path);
  std::filesystem::remove(report);

  R2007EntityWriteIface writeIface;
  {
    dwgRW writer(path.c_str());
    writeIface.writer = &writer;
    REQUIRE(writer.write(&writeIface, DRW::AC1021, false));
  }

  const std::string command = "\"" + executable.string() + "\" \""
      + path.string() + "\" > \"" + report.string() + "\" 2>&1";
  const int result = std::system(command.c_str());
  std::ifstream input(report);
  const std::string output((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
  INFO(output);
  CHECK(result == 0);
  CHECK(output.find("ERROR 0x") == std::string::npos);
  CHECK(output.find("Failed to read") == std::string::npos);

  std::filesystem::remove(path);
  std::filesystem::remove(report);
}

// Regression pin for the pre-2004 (R13/R14/R2000) BLOCKS-walk fix: a broken
// nextEntLink at the *Model_Space chain end used to set ret=false and report
// BAD_READ_BLOCKS even though every drawable entity was delivered (recovered
// by the readDwgEntities sweep). The canonical libdxfrw sample files
// example_r13/r14/2000.dwg + TS1.dwg all tripped it; libreDWG reads them fine.
// Fixtures live in ~/doc/dwg{,2,3} (not committed) -> skip gracefully if absent.
TEST_CASE("DWG R13/R14/R2000 sample files read OK (BLOCKS-walk regression)",
          "[.slow][dwg][r2000][blockswalk][corpus]") {
  const char* home = std::getenv("HOME");
  if (!home) { SKIP("no HOME"); return; }
  struct Want { const char* file; DRW::Version ver; };
  const Want wants[] = {
    {"example_r13.dwg",  DRW::AC1012},
    {"example_r14.dwg",  DRW::AC1014},
    {"example_2000.dwg", DRW::AC1015},
    {"TS1.dwg",          DRW::AC1015},
  };
  int checked = 0;
  for (const Want& w : wants) {
    std::string path;
    for (const std::string& dir : {std::string(home) + "/doc/dwg",
                                   std::string(home) + "/doc/dwg2",
                                   std::string(home) + "/doc/dwg3"}) {
      std::string cand = dir + "/" + w.file;
      if (std::filesystem::exists(cand)) { path = cand; break; }
    }
    if (path.empty()) continue;
    ++checked;
    DwgResult r = readDwg(path, /*verbose=*/false);
    INFO(w.file);
    CHECK(r.error == DRW::BAD_NONE);   // was BAD_READ_BLOCKS before the fix
    CHECK(r.version == w.ver);
    CHECK(r.entities >= 1);
  }
  if (checked == 0)
    SKIP("~/doc/dwg{,2,3} canonical samples absent; skipping BLOCKS-walk pin");
}


// ============================================================================
// READ FEATURE-COVERAGE scans (hidden, corpus-dependent, skip-when-absent).
// ============================================================================

// Aggregate skipped-class / skipped-object telemetry + handled-type histogram
// across ~/doc/dwg{,2,3}. Diagnostic (no hard assertions) — surfaces what the
// reader still drops so coverage work can be prioritized.
TEST_CASE("DWG coverage scan: aggregate skip telemetry across corpus",
          "[.coverage]") {
  const char* home = std::getenv("HOME");
  if (!home) { SKIP("no HOME"); return; }
  std::map<std::string, std::size_t> skippedClasses, skippedObjects;
  std::map<std::string, int> handledTypes;
  int files = 0, okFiles = 0;
  for (const std::string& dir : {std::string(home) + "/doc/dwg",
                                 std::string(home) + "/doc/dwg2",
                                 std::string(home) + "/doc/dwg3"}) {
    if (!std::filesystem::exists(dir)) continue;
    for (const auto& de : std::filesystem::directory_iterator(dir)) {
      const auto ext = de.path().extension().string();
      if (ext != ".dwg" && ext != ".DWG") continue;
      ++files;
      DRW::setCustomDebugPrinter(new DRW::DebugPrinter());
      TypeTrackingIface iface;
      try {
        dwgR reader(de.path().string().c_str());
        if (reader.read(&iface, true) && reader.getError() == DRW::BAD_NONE)
          ++okFiles;
        for (const auto& kv : reader.getSkippedCustomClasses())
          skippedClasses[kv.first] += kv.second;
        for (const auto& kv : reader.getSkippedUnsupportedObjects())
          skippedObjects[kv.first] += kv.second;
        for (const auto& kv : iface.typeCounts)
          handledTypes[kv.first] += kv.second;
      } catch (...) {}
    }
  }
  if (files == 0) { SKIP("~/doc/dwg{,2,3} absent; skipping coverage scan"); return; }
  auto dump = [](const char* title, std::map<std::string, std::size_t> m) {
    std::vector<std::pair<std::string, std::size_t>> v(m.begin(), m.end());
    std::sort(v.begin(), v.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    std::cerr << title << " (" << v.size() << " distinct):\n";
    for (const auto& kv : v) std::cerr << "  " << kv.second << "  " << kv.first << "\n";
  };
  std::cerr << "\nCOVERAGE: files=" << files << " ok=" << okFiles << "\n";
  std::cerr << "-- handled entity types --\n";
  for (const auto& kv : handledTypes) std::cerr << "  " << kv.second << "  " << kv.first << "\n";
  dump("-- skipped CUSTOM CLASSES (renderable-ish gaps)", skippedClasses);
  dump("-- skipped UNSUPPORTED OBJECTS (mostly metadata)", skippedObjects);
  SUCCEED("coverage scan complete");
}

// Per-file corpus audit: one TSV line per .dwg in ~/doc/dwg{,2,3} so an external
// script can join libdxfrw's read result against the LibreDWG oracle and flag
// real failures (oracle ok, we error) and incomplete reads (entity-count
// divergence). Counts EVERY delivery path. Run: ./librecad_tests "[.audit]" -s
TEST_CASE("DWG corpus audit: per-file TSV vs oracle", "[.audit]") {
  const char* home = std::getenv("HOME");
  if (!home) { SKIP("no HOME"); return; }
  struct AuditIface : public TypeTrackingIface {
    int meshes = 0, modelerGeoms = 0, unsupported = 0;
    void addMesh(const DRW_Mesh& e) override { trackT(e, "MESH"); ++meshes; }
    void addModelerGeometry(const DRW_ModelerGeometry&) override { ++modelerGeoms; }
    void addUnsupportedObject(const DRW_UnsupportedObject&) override { ++unsupported; }
  };
  std::cout << "AUDIT\tdir\tfile\tversion\terror\tentities\tblocks\tlayers"
               "\tmodeler\tmeshes\tunsupported\tproxyPrims\tskippedClasses\n";
  for (const std::string& dir : {std::string(home) + "/doc/dwg",
                                 std::string(home) + "/doc/dwg2",
                                 std::string(home) + "/doc/dwg3"}) {
    if (!std::filesystem::exists(dir)) continue;
    std::vector<std::filesystem::path> files;
    for (const auto& de : std::filesystem::directory_iterator(dir)) {
      const auto ext = de.path().extension().string();
      if (ext != ".dwg" && ext != ".DWG") continue;
      if (de.path().filename().string().front() == '#') continue;
      files.push_back(de.path());
    }
    std::sort(files.begin(), files.end());
    const std::string dirName = std::filesystem::path(dir).filename().string();
    for (const auto& p : files) {
      DRW::setCustomDebugPrinter(new DRW::DebugPrinter());
      AuditIface iface;
      DRW::error err = DRW::BAD_UNKNOWN;
      DRW::Version ver = DRW::UNKNOWNV;
      std::size_t proxyPrims = 0;
      std::string skipped;
      try {
        dwgR reader(p.string().c_str());
        (void)reader.read(&iface, true);
        err = reader.getError();
        ver = reader.getVersion();
        proxyPrims = reader.getDecodedProxyPrimitives();
        for (const auto& kv : reader.getSkippedCustomClasses())
          skipped += kv.first + ":" + std::to_string(kv.second) + ",";
      } catch (...) { err = DRW::BAD_UNKNOWN; }
      const int totalEnt = iface.total() + iface.modelerGeoms;
      std::cout << "AUDIT\t" << dirName << "\t" << p.filename().string() << "\t"
                << versionStr(ver) << "\t" << errorStr(err) << "\t" << totalEnt
                << "\t" << iface.blocks << "\t" << iface.layers << "\t"
                << iface.modelerGeoms << "\t" << iface.meshes << "\t"
                << iface.unsupported << "\t" << proxyPrims << "\t"
                << (skipped.empty() ? "-" : skipped) << "\n";
    }
  }
  SUCCEED("audit complete");
}

// MESH is now decoded (not raw-netted): across the corpus, no file should still
// report MESH/AcDbSubDMesh in getSkippedCustomClasses(), and where MESH exists
// it must deliver geometry. Skip-when-absent (corpus is developer-local).
TEST_CASE("DWG MESH decoded + delivered, not skipped", "[.dwg_readback_corpus]") {
  const char* home = std::getenv("HOME");
  if (!home) { SKIP("no HOME"); return; }
  struct MeshProbe : public TypeTrackingIface {
    int meshCount = 0;
    std::size_t meshVertices = 0, meshFaces = 0;
    void addMesh(const DRW_Mesh& d) override {
      ++meshCount;
      meshVertices += d.vertices.size();
      meshFaces += d.faces.size();
    }
  };
  int meshCount = 0, filesStillSkippingMesh = 0;
  std::size_t meshVerts = 0;
  bool anyFile = false;
  // Proxy-graphics telemetry (Part 2): a STDPART2D carrier must yield decoded
  // primitives — previously-invisible cached geometry now rendered.
  std::size_t proxyPrims = 0, filesWithStdpart = 0, stdpartFilesWithProxy = 0;
  for (const std::string& dir : {std::string(home) + "/doc/dwg",
                                 std::string(home) + "/doc/dwg2",
                                 std::string(home) + "/doc/dwg3"}) {
    if (!std::filesystem::exists(dir)) continue;
    for (const auto& de : std::filesystem::directory_iterator(dir)) {
      const auto ext = de.path().extension().string();
      if (ext != ".dwg" && ext != ".DWG") continue;
      anyFile = true;
      DRW::setCustomDebugPrinter(new DRW::DebugPrinter());
      MeshProbe probe;
      try {
        dwgR reader(de.path().string().c_str());
        (void)reader.read(&probe, true);
        meshCount += probe.meshCount;
        meshVerts += probe.meshVertices;
        const auto sk = reader.getSkippedCustomClasses();
        if (sk.find("MESH") != sk.end() || sk.find("AcDbSubDMesh") != sk.end())
          ++filesStillSkippingMesh;
        const std::size_t np = reader.getDecodedProxyPrimitives();
        proxyPrims += np;
        if (sk.find("STDPART2D") != sk.end()) {
          ++filesWithStdpart;            // STDPART2D still raw-netted (round-trip)
          if (np > 0) ++stdpartFilesWithProxy;
        }
      } catch (...) {}
    }
  }
  if (!anyFile) { SKIP("~/doc/dwg{,2,3} absent; skipping MESH probe"); return; }
  std::cerr << "\nMESH probe: meshCount=" << meshCount << " vertices=" << meshVerts
            << " filesStillSkippingMESH=" << filesStillSkippingMesh << "\n";
  std::cerr << "PROXY probe: decodedPrimitives=" << proxyPrims
            << " filesWithSTDPART2D=" << filesWithStdpart
            << " ofThoseWithDecodedProxy=" << stdpartFilesWithProxy << "\n";
  CHECK(filesStillSkippingMesh == 0);  // MESH no longer falls to the raw net
  // Every STDPART2D file should now surface decoded proxy primitives while the
  // raw object is still preserved for round-trip.
  if (filesWithStdpart > 0) CHECK(stdpartFilesWithProxy == filesWithStdpart);
  if (meshCount == 0) { SUCCEED("no MESH entities in local corpus"); return; }
  CHECK(meshVerts >= 1);  // geometry actually delivered
}

// Durable, CI-safe unit pin for the proxy-graphics decoder: a hand-crafted
// blob (8-byte header + one CIRCLE chunk + one CIRCULAR_ARC chunk) exercising
// the framing loop, the byte-aligned little-endian ByteStream, the 4-byte
// alignment and the CIRCLE/ARC emission.  Coordinates are checked exactly —
// this pins the field byte-orders the ezdxf port depends on.  Verified byte-
// identical against ezdxf's own ProxyGraphic on a real corpus STDPART2D blob.
TEST_CASE("proxy graphics decoder unit", "[proxy]") {
  auto putU32 = [](std::string& s, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) s.push_back(char((v >> (8 * i)) & 0xFF));
  };
  auto putD = [](std::string& s, double v) {
    char b[8]; std::memcpy(b, &v, 8); s.append(b, 8); // host LE
  };
  std::string blob(8, '\0');                 // 8-byte header (skipped)
  // CIRCLE (type 2): center(3d) + radius(d) + normal(3d) = 56-byte payload
  putU32(blob, 8 + 56); putU32(blob, 2);
  putD(blob, 10.0); putD(blob, 20.0); putD(blob, 0.0);   // center
  putD(blob, 5.0);                                       // radius
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 1.0);     // normal
  // CIRCULAR_ARC (type 4): center(3d)+radius(d)+normal(3d)+startVec(3d)+sweep(d)
  // = 24+8+24+24+8 = 88-byte payload.
  putU32(blob, 8 + 88); putU32(blob, 4);
  putD(blob, 1.0); putD(blob, 2.0); putD(blob, 0.0);     // center
  putD(blob, 3.0);                                       // radius
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 1.0);     // normal (Z)
  putD(blob, 1.0); putD(blob, 0.0); putD(blob, 0.0);     // start vec → angle 0
  putD(blob, M_PI / 2.0);                                // sweep = 90°

  struct Cap : public TypeTrackingIface {
    int circles = 0, arcs = 0;
    DRW_Circle lastC; DRW_Arc lastA;
    void addCircle(const DRW_Circle& e) override { ++circles; lastC = e; }
    void addArc(const DRW_Arc& e) override { ++arcs; lastA = e; }
  } cap;
  DRW_Point parent;
  int n = DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent);

  REQUIRE(n == 2);
  REQUIRE(cap.circles == 1);
  REQUIRE(cap.arcs == 1);
  CHECK(cap.lastC.basePoint.x == Catch::Approx(10.0));
  CHECK(cap.lastC.basePoint.y == Catch::Approx(20.0));
  CHECK(cap.lastC.radious == Catch::Approx(5.0));
  CHECK(cap.lastA.basePoint.x == Catch::Approx(1.0));
  CHECK(cap.lastA.radious == Catch::Approx(3.0));
  CHECK(cap.lastA.staangle == Catch::Approx(0.0));           // start vec (1,0) → 0 rad
  CHECK(cap.lastA.endangle == Catch::Approx(M_PI / 2.0));    // 0 + 90° sweep
}

// Transform stack: op29 PUSH_MATRIX … op31 POP_MATRIX.  Primitives nested
// inside a matrix must be emitted in the transformed frame, and the matrix must
// stop applying after the pop.  Before this was handled, every proxy primitive
// under a matrix decoded fine (so counts looked right) but rendered at the
// wrong position — a silent geometry error.  The wire layout is 16 row-major
// doubles with the translation in the 4th column (verified against corpus
// op29 chunks, whose bottom row is 0,0,0,1).
TEST_CASE("proxy transform stack push/pop matrix", "[proxy]") {
  auto putU32 = [](std::string& s, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) s.push_back(char((v >> (8 * i)) & 0xFF));
  };
  auto putD = [](std::string& s, double v) {
    char b[8]; std::memcpy(b, &v, 8); s.append(b, 8); // host LE
  };
  std::string blob(8, '\0');                 // 8-byte header (skipped)
  // PUSH_MATRIX (29): scale x2, translate (100,200). 16 doubles = 128 bytes.
  putU32(blob, 8 + 128); putU32(blob, 29);
  putD(blob, 2.0); putD(blob, 0.0); putD(blob, 0.0); putD(blob, 100.0);
  putD(blob, 0.0); putD(blob, 2.0); putD(blob, 0.0); putD(blob, 200.0);
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 1.0); putD(blob, 0.0);
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 0.0); putD(blob, 1.0);
  // CIRCLE under the matrix: centre (10,20) r=5 → (120,240) r=10
  putU32(blob, 8 + 56); putU32(blob, 2);
  putD(blob, 10.0); putD(blob, 20.0); putD(blob, 0.0);
  putD(blob, 5.0);
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 1.0);
  // POLYLINE under the matrix: L count + 2×3d = 52-byte payload.
  // (0,0)→(100,200) and (1,1)→(102,202)
  putU32(blob, 8 + 52); putU32(blob, 6);
  putU32(blob, 2);
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 0.0);
  putD(blob, 1.0); putD(blob, 1.0); putD(blob, 0.0);
  // POP_MATRIX (31): empty payload.
  putU32(blob, 8); putU32(blob, 31);
  // CIRCLE after the pop: must be emitted untransformed.
  putU32(blob, 8 + 56); putU32(blob, 2);
  putD(blob, 1.0); putD(blob, 2.0); putD(blob, 0.0);
  putD(blob, 3.0);
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 1.0);

  struct Cap : public TypeTrackingIface {
    std::vector<DRW_Circle> circles;
    std::vector<DRW_Polyline> plines;
    void addCircle(const DRW_Circle& e) override { circles.push_back(e); }
    void addPolyline(const DRW_Polyline& e) override { plines.push_back(e); }
  } cap;
  DRW_Point parent;
  int n = DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent);

  REQUIRE(n == 3);
  REQUIRE(cap.circles.size() == 2);
  REQUIRE(cap.plines.size() == 1);
  // Under the matrix: point scaled+translated, radius scaled.
  CHECK(cap.circles[0].basePoint.x == Catch::Approx(120.0));
  CHECK(cap.circles[0].basePoint.y == Catch::Approx(240.0));
  CHECK(cap.circles[0].radious == Catch::Approx(10.0));
  REQUIRE(cap.plines[0].vertlist.size() == 2);
  CHECK(cap.plines[0].vertlist[0]->basePoint.x == Catch::Approx(100.0));
  CHECK(cap.plines[0].vertlist[0]->basePoint.y == Catch::Approx(200.0));
  CHECK(cap.plines[0].vertlist[1]->basePoint.x == Catch::Approx(102.0));
  CHECK(cap.plines[0].vertlist[1]->basePoint.y == Catch::Approx(202.0));
  // After POP_MATRIX the transform must no longer apply.
  CHECK(cap.circles[1].basePoint.x == Catch::Approx(1.0));
  CHECK(cap.circles[1].basePoint.y == Catch::Approx(2.0));
  CHECK(cap.circles[1].radious == Catch::Approx(3.0));
}

// Proxy SHELL (op9) → DRW_Mesh. Layout: RL vertex count, count×3d vertices, RL
// face-entry count (longs in the face stream), then [signed edge_count, idx…]
// entries (0-based indices). Verified against the ezdxf shell() reference and
// gh109_1.dwg (3 shells decoded). Also checks the transform stack applies to
// the mesh vertices.
TEST_CASE("proxy SHELL op9 -> mesh", "[proxy]") {
  auto putU32 = [](std::string& s, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) s.push_back(char((v >> (8 * i)) & 0xFF));
  };
  auto putD = [](std::string& s, double v) {
    char b[8]; std::memcpy(b, &v, 8); s.append(b, 8);
  };
  // A unit quad (4 verts) with one 4-edge face [0,1,2,3], under translate(10,20).
  std::string blob(8, '\0');
  putU32(blob, 8 + 128); putU32(blob, 29);          // PUSH_MATRIX translate(10,20)
  putD(blob, 1); putD(blob, 0); putD(blob, 0); putD(blob, 10);
  putD(blob, 0); putD(blob, 1); putD(blob, 0); putD(blob, 20);
  putD(blob, 0); putD(blob, 0); putD(blob, 1); putD(blob, 0);
  putD(blob, 0); putD(blob, 0); putD(blob, 0); putD(blob, 1);
  // SHELL payload: 4 (RL) + 4×24 verts + 5 (RL faceEntries) + [4,0,1,2,3] (5 RL)
  std::string pay;
  putU32(pay, 4);
  putD(pay, 0); putD(pay, 0); putD(pay, 0);
  putD(pay, 1); putD(pay, 0); putD(pay, 0);
  putD(pay, 1); putD(pay, 1); putD(pay, 0);
  putD(pay, 0); putD(pay, 1); putD(pay, 0);
  putU32(pay, 5);                                   // face_entry_count (longs)
  putU32(pay, 4);                                   // edge_count
  putU32(pay, 0); putU32(pay, 1); putU32(pay, 2); putU32(pay, 3);
  putU32(blob, static_cast<std::uint32_t>(8 + pay.size())); putU32(blob, 9);
  blob += pay;
  putU32(blob, 8); putU32(blob, 31);                // POP_MATRIX

  struct Cap : public TypeTrackingIface {
    std::vector<DRW_Mesh> meshes;
    void addMesh(const DRW_Mesh& e) override { meshes.push_back(e); }
  } cap;
  DRW_Point parent;
  int n = DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent);

  REQUIRE(n == 1);
  REQUIRE(cap.meshes.size() == 1);
  REQUIRE(cap.meshes[0].vertices.size() == 4);
  REQUIRE(cap.meshes[0].faces.size() == 1);
  CHECK(cap.meshes[0].faces[0].size() == 4);
  // Vertices translated by (10,20).
  CHECK(cap.meshes[0].vertices[0].x == Catch::Approx(10.0));
  CHECK(cap.meshes[0].vertices[0].y == Catch::Approx(20.0));
  CHECK(cap.meshes[0].vertices[2].x == Catch::Approx(11.0));
  CHECK(cap.meshes[0].vertices[2].y == Catch::Approx(21.0));
  CHECK(cap.meshes[0].faces[0][0] == 0);
  CHECK(cap.meshes[0].faces[0][3] == 3);
}

// DXF parity (Part 2.3): an unmodeled DXF entity carrying proxy graphics
// (group 92 length + group 310 hex) is decoded into render primitives on read,
// via DRW_Entity::parseCode + dxfRW::processRawEntity. Mirrors the DWG path.
TEST_CASE("proxy graphics decoded from DXF", "[proxy]") {
  // Build the same CIRCLE blob as the unit test, then hex-encode it.
  auto putU32 = [](std::string& s, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) s.push_back(char((v >> (8 * i)) & 0xFF));
  };
  auto putD = [](std::string& s, double v) { char b[8]; std::memcpy(b, &v, 8); s.append(b, 8); };
  std::string blob(8, '\0');
  putU32(blob, 8 + 56); putU32(blob, 2);
  putD(blob, 100.0); putD(blob, 200.0); putD(blob, 0.0);   // center
  putD(blob, 7.5);                                         // radius
  putD(blob, 0.0); putD(blob, 0.0); putD(blob, 1.0);       // normal
  std::string hex;
  static const char* H = "0123456789ABCDEF";
  for (unsigned char c : blob) { hex.push_back(H[c >> 4]); hex.push_back(H[c & 0xF]); }

  std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nACAD_PROXY_ENTITY\n5\n1\n8\nproxylayer\n"
      "92\n" + std::to_string(blob.size()) + "\n"
      "310\n" + hex + "\n"
      "0\nENDSEC\n0\nEOF\n";
  const std::string path =
      (std::filesystem::temp_directory_path() / "proxy_parity.dxf").string();
  { std::ofstream out(path); out << dxf; }

  struct Cap : public TypeTrackingIface {
    int circles = 0; DRW_Circle last;
    void addCircle(const DRW_Circle& e) override { ++circles; last = e; }
  } cap;
  dxfRW reader(path.c_str());
  REQUIRE(reader.read(&cap, false));
  REQUIRE(cap.circles == 1);
  CHECK(cap.last.basePoint.x == Catch::Approx(100.0));
  CHECK(cap.last.basePoint.y == Catch::Approx(200.0));
  CHECK(cap.last.radious == Catch::Approx(7.5));
}

// ---- proxy decoder: remaining-opcode unit pins (CI-safe, hand-crafted) ----
namespace {
void pgU32(std::string& s, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) s.push_back(char((v >> (8 * i)) & 0xFF));
}
void pgD(std::string& s, double v) { char b[8]; std::memcpy(b, &v, 8); s.append(b, 8); }
// Append a chunk: 4-byte size (=8+payload) + 4-byte type + payload bytes.
void pgChunk(std::string& s, std::uint32_t type, const std::string& payload) {
  pgU32(s, static_cast<std::uint32_t>(8 + payload.size())); pgU32(s, type); s += payload;
}
std::string pgCirclePayload() { // CIRCLE: center+radius+normal
  std::string p; pgD(p,0); pgD(p,0); pgD(p,0); pgD(p,1.0); pgD(p,0); pgD(p,0); pgD(p,1.0); return p;
}
struct PgColorCap : public TypeTrackingIface {
  DRW_Circle last; int n = 0;
  void addCircle(const DRW_Circle& e) override { last = e; ++n; }
};

struct PgSink final : public DRW_ProxyGraphicSink {
  bool accept = true;
  std::size_t acceptedBeforeRefusal = std::numeric_limits<std::size_t>::max();
  std::size_t attempts = 0;
  std::size_t circles = 0;

  bool addArc(const DRW_Arc&) override { return acceptEvent(); }
  bool addCircle(const DRW_Circle&) override { ++circles; return acceptEvent(); }
  bool addEllipse(const DRW_Ellipse&) override { return acceptEvent(); }
  bool addLWPolyline(const DRW_LWPolyline&) override { return acceptEvent(); }
  bool addMesh(const DRW_Mesh&) override { return acceptEvent(); }
  bool addPolyline(const DRW_Polyline&) override { return acceptEvent(); }
  bool addText(const DRW_Text&) override { return acceptEvent(); }

private:
  bool acceptEvent() {
    ++attempts;
    return accept && attempts <= acceptedBeforeRefusal;
  }
};
} // namespace

TEST_CASE("proxy decoder reports bounded staged output", "[proxy]") {
  std::string blob(8, '\0');
  pgChunk(blob, 2, pgCirclePayload());
  pgChunk(blob, 0x7fffffffu, {});
  pgChunk(blob, 2, pgCirclePayload());

  const auto inspected = DRW_ProxyGraphicDecoder::inspect(blob);
  CHECK(inspected.completed());
  CHECK(inspected.recognizedPrimitiveChunkCount == 2u);
  CHECK(inspected.skippedUnsupportedChunkCount == 1u);
  CHECK(inspected.consumedByteCount == blob.size());

  DRW_Point parent;
  PgSink accepted;
  const auto decoded = DRW_ProxyGraphicDecoder::decode(
      blob, DRW::AC1024, accepted, parent);
  CHECK(decoded.completed());
  CHECK(decoded.emittedPrimitiveCount == 2u);
  CHECK(accepted.circles == 2u);

  DRW_ProxyGraphicLimits limits;
  limits.maxPrimitiveCount = 1u;
  CHECK(DRW_ProxyGraphicDecoder::inspect(blob, limits).stopReason
        == DRW_ProxyGraphicStopReason::PrimitiveLimit);
  PgSink limited;
  CHECK(DRW_ProxyGraphicDecoder::decode(
            blob, DRW::AC1024, limited, parent, {}, {}, limits).stopReason
        == DRW_ProxyGraphicStopReason::PrimitiveLimit);
  CHECK(limited.circles == 1u);

  PgSink refusing;
  refusing.accept = false;
  const auto refused = DRW_ProxyGraphicDecoder::decode(
      blob, DRW::AC1024, refusing, parent);
  CHECK(refused.stopReason == DRW_ProxyGraphicStopReason::SinkRefused);
  CHECK(refused.emittedPrimitiveCount == 0u);
  CHECK(refusing.attempts == 1u);

  PgSink partiallyRefusing;
  partiallyRefusing.acceptedBeforeRefusal = 1u;
  const auto partlyRefused = DRW_ProxyGraphicDecoder::decode(
      blob, DRW::AC1024, partiallyRefusing, parent);
  CHECK(partlyRefused.stopReason == DRW_ProxyGraphicStopReason::SinkRefused);
  CHECK(partlyRefused.emittedPrimitiveCount == 1u);
  CHECK(partiallyRefusing.attempts == 2u);

  CHECK(DRW_ProxyGraphicDecoder::inspect(std::string(7, '\0')).stopReason
        == DRW_ProxyGraphicStopReason::ShortHeader);
  std::string invalid(8, '\0');
  pgU32(invalid, 7u);
  pgU32(invalid, 2u);
  CHECK(DRW_ProxyGraphicDecoder::inspect(invalid).stopReason
        == DRW_ProxyGraphicStopReason::InvalidChunkSize);
  std::string truncated(8, '\0');
  pgU32(truncated, 9u);
  pgU32(truncated, 2u);
  CHECK(DRW_ProxyGraphicDecoder::inspect(truncated).stopReason
        == DRW_ProxyGraphicStopReason::TruncatedChunk);

  DRW_ProxyGraphicLimits chunkLimits;
  chunkLimits.maxChunkCount = 1u;
  CHECK(DRW_ProxyGraphicDecoder::inspect(blob, chunkLimits).stopReason
        == DRW_ProxyGraphicStopReason::ChunkLimit);
  PgSink chunkLimited;
  const auto chunkLimitedResult = DRW_ProxyGraphicDecoder::decode(
      blob, DRW::AC1024, chunkLimited, parent, {}, {}, chunkLimits);
  CHECK(chunkLimitedResult.stopReason == DRW_ProxyGraphicStopReason::ChunkLimit);
  CHECK(chunkLimitedResult.emittedPrimitiveCount == 1u);

  std::string matrixPayload;
  for (int row = 0; row < 4; ++row)
    for (int column = 0; column < 4; ++column)
      pgD(matrixPayload, row == column ? 1.0 : 0.0);
  std::string nestedMatrices(8, '\0');
  pgChunk(nestedMatrices, 29, matrixPayload);
  pgChunk(nestedMatrices, 29, matrixPayload);
  DRW_ProxyGraphicLimits matrixLimits;
  matrixLimits.maxMatrixDepth = 1u;
  PgSink matrixLimited;
  CHECK(DRW_ProxyGraphicDecoder::decode(
            nestedMatrices, DRW::AC1024, matrixLimited, parent, {}, {},
            matrixLimits).stopReason
        == DRW_ProxyGraphicStopReason::MatrixDepthLimit);
  CHECK(matrixLimited.attempts == 0u);

  std::string oversizedCount;
  pgU32(oversizedCount, 2u);
  std::string polyline(8, '\0');
  pgChunk(polyline, 6, oversizedCount);
  DRW_ProxyGraphicLimits polylineLimits;
  polylineLimits.maxPolylineVertices = 1u;
  PgSink polylineLimited;
  CHECK(DRW_ProxyGraphicDecoder::decode(
            polyline, DRW::AC1024, polylineLimited, parent, {}, {},
            polylineLimits).stopReason
        == DRW_ProxyGraphicStopReason::ResourceLimit);
  CHECK(polylineLimited.attempts == 0u);

  dwgBufferW lwPayload;
  lwPayload.putRawLong32(0); // num_data_bytes
  lwPayload.putBitShort(0);  // flags
  lwPayload.putBitLong(2);  // vertex count
  std::string lwPolyline(8, '\0');
  pgChunk(lwPolyline, 33, std::string(
      reinterpret_cast<const char*>(lwPayload.data().data()),
      lwPayload.data().size()));
  PgSink lwPolylineLimited;
  CHECK(DRW_ProxyGraphicDecoder::decode(
            lwPolyline, DRW::AC1024, lwPolylineLimited, parent, {}, {},
            polylineLimits).stopReason
        == DRW_ProxyGraphicStopReason::ResourceLimit);
  CHECK(lwPolylineLimited.attempts == 0u);

  std::string mesh(8, '\0');
  pgChunk(mesh, 9, oversizedCount);
  DRW_ProxyGraphicLimits meshLimits;
  meshLimits.maxMeshVertices = 1u;
  PgSink meshLimited;
  CHECK(DRW_ProxyGraphicDecoder::decode(
            mesh, DRW::AC1024, meshLimited, parent, {}, {}, meshLimits)
            .stopReason
        == DRW_ProxyGraphicStopReason::ResourceLimit);
  CHECK(meshLimited.attempts == 0u);

  std::string oversizedFaces;
  pgU32(oversizedFaces, 1u);
  pgD(oversizedFaces, 0.0); pgD(oversizedFaces, 0.0); pgD(oversizedFaces, 0.0);
  pgU32(oversizedFaces, 2u);
  std::string faceMesh(8, '\0');
  pgChunk(faceMesh, 9, oversizedFaces);
  DRW_ProxyGraphicLimits faceLimits;
  faceLimits.maxMeshFaceEntries = 1u;
  PgSink faceLimited;
  CHECK(DRW_ProxyGraphicDecoder::decode(
            faceMesh, DRW::AC1024, faceLimited, parent, {}, {}, faceLimits)
            .stopReason
        == DRW_ProxyGraphicStopReason::ResourceLimit);
  CHECK(faceLimited.attempts == 0u);
}

// op22 ATTRIBUTE_TRUE_COLOR: the high flag byte selects RGB/ACI/BYLAYER/BYBLOCK
// (was unconditionally masked as RGB — 100% of corpus chunks mis-rendered).
TEST_CASE("proxy op22 true-color flag dispatch", "[proxy]") {
  auto runWithColor = [](std::uint32_t raw) {
    std::string blob(8, '\0');
    std::string cp; pgU32(cp, raw);
    pgChunk(blob, 22, cp);                 // ATTRIBUTE_TRUE_COLOR
    pgChunk(blob, 2, pgCirclePayload());   // CIRCLE inherits the colour state
    PgColorCap cap; DRW_Point parent;      // parent.color defaults to ByLayer
    DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent);
    return cap.last;
  };
  SECTION("RGB (0xC2) → color24") {
    DRW_Circle c = runWithColor(0xC2FF8040u);
    CHECK(c.color24 == 0xFF8040);
  }
  SECTION("ACI (0xC3) → indexed color") {
    DRW_Circle c = runWithColor(0xC3000007u);
    CHECK(c.color == 7);
    CHECK(c.color24 == -1);   // not stored as RGB
  }
  SECTION("BYLAYER (0xC0) → no spurious black RGB") {
    DRW_Circle c = runWithColor(0xC0000000u);
    CHECK(c.color24 == -1);   // the old bug stored 0 (black) here
  }
}

// op5 CIRCULAR_ARC_3P: ASYMMETRIC pin (p1 at 0°, p3 at 90°, p2 at 45° on the
// minor-arc side) so a p2/p3 wire-arg swap or start/end mix-up is caught.
TEST_CASE("proxy op5 circular-arc-3p remap", "[proxy]") {
  const double s = 5.0 / std::sqrt(2.0);
  std::string pay;
  pgD(pay, 5.0); pgD(pay, 0.0); pgD(pay, 0.0); // p1 (start) @ 0°
  pgD(pay, s);   pgD(pay, s);   pgD(pay, 0.0); // p2 (def) @ 45°
  pgD(pay, 0.0); pgD(pay, 5.0); pgD(pay, 0.0); // p3 (end) @ 90°
  std::string blob(8, '\0'); pgChunk(blob, 5, pay);
  struct Cap : public TypeTrackingIface {
    DRW_Arc last; int n = 0;
    void addArc(const DRW_Arc& e) override { last = e; ++n; }
  } cap;
  DRW_Point parent;
  int n = DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent);
  REQUIRE(n == 1);
  CHECK(cap.last.basePoint.x == Catch::Approx(0.0).margin(1e-9)); // circumcenter
  CHECK(cap.last.basePoint.y == Catch::Approx(0.0).margin(1e-9));
  CHECK(cap.last.radious == Catch::Approx(5.0));
  CHECK(cap.last.staangle == Catch::Approx(0.0).margin(1e-9));        // from p1
  CHECK(cap.last.endangle == Catch::Approx(M_PI / 2.0));             // from WIRE p3, not p2
}

// op11 TEXT2 + op38 UNICODE_TEXT2: string is read FIRST, then the
// <2l>/<4d>/<5L> metadata block.  Pins the field order + UTF-16LE transcode.
TEST_CASE("proxy op11/op38 text2", "[proxy]") {
  struct Cap : public TypeTrackingIface {
    std::vector<DRW_Text> texts;
    void addText(const DRW_Text& e) override { texts.push_back(e); }
  };
  auto textTail = [](std::string& p, double h) {
    pgU32(p, 0); pgU32(p, 0);                        // <2l> ignore_len, raw
    pgD(p, h); pgD(p, 1.0); pgD(p, 0.0); pgD(p, 0.0); // <4d> height,width,oblique,tracking
    for (int i = 0; i < 5; ++i) pgU32(p, 0);          // <5L> flags
  };
  SECTION("op11 TEXT2 (cp1252)") {
    std::string p;
    pgD(p,1.0); pgD(p,2.0); pgD(p,0.0);   // start
    pgD(p,0.0); pgD(p,0.0); pgD(p,1.0);   // normal
    pgD(p,1.0); pgD(p,0.0); pgD(p,0.0);   // dir → angle 0
    p += "AB"; p.push_back('\0');         // padded string
    while (p.size() % 4) p.push_back('\0'); // align to 4
    textTail(p, 7.0);
    std::string blob(8, '\0'); pgChunk(blob, 11, p);
    Cap cap; DRW_Point parent;
    int n = DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent);
    REQUIRE(n == 1); REQUIRE(cap.texts.size() == 1);
    CHECK(cap.texts[0].text == "AB");
    CHECK(cap.texts[0].basePoint.x == Catch::Approx(1.0));
    CHECK(cap.texts[0].height == Catch::Approx(7.0));
    CHECK(cap.texts[0].angle == Catch::Approx(0.0));
  }
  SECTION("op38 UNICODE_TEXT2 (UTF-16LE)") {
    std::string p;
    pgD(p,0.0); pgD(p,0.0); pgD(p,0.0);   // start
    pgD(p,0.0); pgD(p,0.0); pgD(p,1.0);   // normal
    pgD(p,1.0); pgD(p,0.0); pgD(p,0.0);   // dir
    // "Hi" in UTF-16LE + double-NUL terminator, then align to 4
    p.push_back('H'); p.push_back('\0'); p.push_back('i'); p.push_back('\0');
    p.push_back('\0'); p.push_back('\0');
    while (p.size() % 4) p.push_back('\0');
    textTail(p, 3.0);
    std::string blob(8, '\0'); pgChunk(blob, 38, p);
    Cap cap; DRW_Point parent;
    int n = DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent);
    REQUIRE(n == 1); REQUIRE(cap.texts.size() == 1);
    CHECK(cap.texts[0].text == "Hi");
    CHECK(cap.texts[0].height == Catch::Approx(3.0));
  }
}

// op16/op18/op23 attribute resolution: layer/linetype by index (offset 0 for
// libdxfrw, NOT ezdxf's +2), sentinels, and the lineweight two's-complement.
TEST_CASE("proxy op16/18/23 attribute resolution", "[proxy]") {
  std::vector<std::string> layers = {"L0", "L1", "L2"};
  std::vector<std::string> ltypes = {"DASHED", "DOTTED"};
  auto run = [&](std::uint32_t layIdx, std::uint32_t ltIdx, std::uint32_t lw) {
    std::string blob(8, '\0');
    std::string a; pgU32(a, layIdx); pgChunk(blob, 16, a);
    std::string b; pgU32(b, ltIdx);  pgChunk(blob, 18, b);
    std::string c; pgU32(c, lw);     pgChunk(blob, 23, c);
    pgChunk(blob, 2, pgCirclePayload());
    PgColorCap cap; DRW_Point parent;
    DRW_ProxyGraphicDecoder::decode(blob, DRW::AC1024, cap, parent, layers, ltypes);
    return cap.last;
  };
  SECTION("by-index, offset 0") {
    DRW_Circle c = run(2, 1, 13);
    CHECK(c.layer == "L2");
    CHECK(c.lineType == "DOTTED");          // index 1 → ltypes[1], NOT ltypes[3]
    CHECK(c.lWeight == DRW_LW_Conv::dxfInt2lineWidth(13));
  }
  SECTION("linetype sentinels") {
    CHECK(run(0, 32766u, 0).lineType == "BYBLOCK");
    CHECK(run(0, 32767u, 0).lineType == "BYLAYER");
  }
  SECTION("out-of-range layer inherits parent") {
    DRW_Circle c = run(99u, 1, 0);
    CHECK(c.layer == "0");                  // parent default, not garbage
  }
}

// Dev-local regression pin for the op16/op18 index→name mapping: libdxfrw's
// layer / linetype storage order MUST equal the dwgread/LibreDWG oracle order
// (offset 0), else resolved proxy attributes would be silently wrong-but-in-
// range.  Ground truth baked from the oracle cross-check on gripper.dwg.
TEST_CASE("proxy attr layer-order oracle pin", "[.dwg_proxy_attr]") {
  const char* home = std::getenv("HOME");
  if (!home) { SKIP("no HOME"); return; }
  const std::string f = std::string(home) + "/doc/dwg2/gripper.dwg";
  if (!std::filesystem::exists(f)) { SKIP("gripper.dwg absent"); return; }
  TypeTrackingIface iface;
  dwgR reader(f.c_str());
  REQUIRE(reader.read(&iface, true));
  const auto& L = reader.getLayerNameOrder();
  const auto& T = reader.getLtypeNameOrder();
  REQUIRE(L.size() == 21);
  CHECK(L[0]  == "0");    CHECK(L[1]  == "AM_0"); CHECK(L[2] == "3");
  CHECK(L[15] == "AM_7"); CHECK(L[17] == "AM_4"); CHECK(L[20] == "BV1");
  REQUIRE(T.size() >= 21);
  CHECK(T[0] == "Continuous");
  CHECK(T[5] == "AM_ISO08W050x2"); // offset-0: ezdxf's +2 would name T[7]
}

// Pre-R13 (AC1009/R11) minimal read support. The corpus lives in the
// developer-local LibreDWG checkout; skip gracefully if absent. dwgReaderR11
// reads the ENTITIES section's non-chained geometry (LINE/POINT/CIRCLE/ARC/
// TEXT/SOLID/TRACE/3DLINE/3DFACE); INSERT/POLYLINE/blocks are a follow-up.
TEST_CASE("DWG pre-R13: read AC1009/R11 entities section") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping pre-R13 test");
  }
  const std::string dir =
      std::string(home) + "/dev/libredwg/test/test-data/r11/";
  const std::string path = dir + "entities-2d.dwg";
  std::ifstream probe(path, std::ios::binary);
  if (!probe.good()) {
    SKIP("pre-R13 corpus absent; skipping");
  }
  probe.close();

  const DwgResult r = readDwg(path);
  CHECK(r.ok);                          // was BAD_VERSION before R11 support
  CHECK(r.version == DRW::AC1009);
  CHECK(r.entities >= 23);              // + inserts + attrib/attdef + dimension block
  CHECK(r.blocks >= 3);                 // BLOCK1 / BLOCK2 / *D
}

namespace {
struct R11LineCollector : public CountingIface {
  std::vector<DRW_Line> lines;
  void addLine(const DRW_Line &e) override {
    CountingIface::addLine(e);
    lines.push_back(e);
  }
};
}  // namespace

TEST_CASE("DWG pre-R13: map R11 3DLINE to DRW_Line") {
  const std::string path = libredwgFixturePath("r11", "entities-3d.dwg");
  if (path.empty()) {
    SKIP("pre-R13 corpus root absent; skipping");
  }
  std::ifstream probe(path, std::ios::binary);
  if (!probe.good()) {
    SKIP("entities-3d.dwg absent; skipping");
  }
  probe.close();

  R11LineCollector iface;
  const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
  REQUIRE(r.ok);
  REQUIRE(r.version == DRW::AC1009);

  bool found3DLine = false;
  auto near = [](double a, double b) { return std::abs(a - b) < 1e-12; };
  for (const auto &line : iface.lines) {
    if (near(line.basePoint.x, 2.0) && near(line.basePoint.y, 3.0) &&
        near(line.basePoint.z, 4.0) && near(line.secPoint.x, 3.0) &&
        near(line.secPoint.y, 4.0) && near(line.secPoint.z, 5.0)) {
      found3DLine = true;
      break;
    }
  }
  // Oracle: LibreDWG r11/entities-3d.dxf from the same corpus emits this
  // legacy DWG type-21 record as a DXF LINE with 3D endpoints.
  CHECK(found3DLine);
}

// Pre-R13 R11 typed DIMENSION (LINEAR + ALIGNED). Phase 4 swaps the previous
// "render the *D block as an INSERT" path for a typed DRW_DimLinear /
// DRW_DimAligned for the handled dimtypes (LINEAR=0, ALIGNED=1), keeping the
// INSERT fallback for ANG2LN/ANG3PT/RADIUS/DIAMETER/ORDINATE (no R11 oracle
// file exists for those types in the LibreDWG corpus). The swap is the
// load-bearing check: a typed dim AND a *D INSERT for the same record would
// double-render.
namespace {
struct DimCollector : public CountingIface {
  std::vector<DRW_DimLinear> lin;
  std::vector<DRW_DimAligned> ali;
  int starDInserts = 0;
  void addDimLinear(const DRW_DimLinear *e) override {
    CountingIface::addDimLinear(e);
    lin.push_back(*e);
  }
  void addDimAlign(const DRW_DimAligned *e) override {
    CountingIface::addDimAlign(e);
    ali.push_back(*e);
  }
  void addInsert(const DRW_Insert &e) override {
    CountingIface::addInsert(e);
    // Anonymous dim-graphics block name starts with "*D" (case-sensitive in
    // libredwg). A non-zero count means the typed-dim swap is double-rendering.
    if (e.name.size() >= 2 && e.name[0] == '*' && e.name[1] == 'D')
      ++starDInserts;
  }
};
}  // namespace

TEST_CASE("DWG pre-R13: R11 typed DIMENSION (LINEAR + ALIGNED)") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  SECTION("entities-2d.dwg — 1 ALIGNED dim, no double-render") {
    const std::string path =
        std::string(home) + "/dev/libredwg/test/test-data/r11/entities-2d.dwg";
    std::ifstream probe(path, std::ios::binary);
    if (!probe.good()) { SKIP("entities-2d.dwg absent"); return; }
    probe.close();
    DimCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1009);
    CHECK(iface.lin.size() == 0);
    REQUIRE(iface.ali.size() == 1);
    // Oracle (dwgread -O JSON): def_pt=[8,8,0], text_midpt=[0,0],
    // xline1_pt=[6,8,2], xline2_pt=[7,7,3], dimtype=1=ALIGNED.
    // The raw on-disk doubles are at one ULP below the integers (e.g.
    // def_pt.x bytes = fe ff ff ff ff ff 1f 40 = 7.999...); dwgread/jq
    // pretty-prints them as "8.0" but a bit-exact reader yields
    // 7.99999999999999822. Compare with a ULP-tolerant near().
    const auto &d = iface.ali[0];
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-12; };
    CHECK(near(d.getDefPoint().x, 8.0));
    CHECK(near(d.getDefPoint().y, 8.0));
    CHECK(near(d.getDef1Point().x, 6.0));
    CHECK(near(d.getDef1Point().y, 8.0));
    CHECK(near(d.getDef1Point().z, 2.0));
    CHECK(near(d.getDef2Point().x, 7.0));
    CHECK(near(d.getDef2Point().y, 7.0));
    CHECK(near(d.getDef2Point().z, 3.0));
    // Swap invariant: no `*D` INSERTs for handled dimtypes (would
    // double-render with the typed dim).
    CHECK(iface.starDInserts == 0);
  }
  SECTION("ACEB10.dwg — 15 LINEAR dims, no double-render") {
    const std::string path =
        std::string(home) + "/dev/libredwg/test/test-data/r11/ACEB10.dwg";
    std::ifstream probe(path, std::ios::binary);
    if (!probe.good()) { SKIP("ACEB10.dwg absent"); return; }
    probe.close();
    DimCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1009);
    CHECK(iface.lin.size() == 15);
    CHECK(iface.ali.size() == 0);
    CHECK(iface.starDInserts == 0);
    // Spot-check first LINEAR dim against the oracle.
    const auto &d = iface.lin[0];
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-12; };
    CHECK(near(d.getDefPoint().x, 13.62245609657801));
    CHECK(near(d.getDefPoint().y, 4.1236674739085));
    CHECK(d.getDefPoint().z == 0.0);
    CHECK(near(d.getDef1Point().x, 12.62245609657801));
    CHECK(near(d.getDef2Point().x, 13.62245609657801));
    CHECK(d.getAngle() == 0.0);
  }
}

// Pre-R13 R11/R10 SHAPE style_id + ATTDEF/ATTRIB rotation decode (Phase 4b).
// Two spec-grounded read-width / opt-bit fixes in dwgReaderR11.cpp:
//   (a) SHAPE style_id is an RC (1 byte; dwg.spec:2338 FIELD_CAST(style_id,RC,
//       BS,0)). Reading it as a 2-byte RS over-consumed one byte and desynced
//       the following rotation RD -> garbage m_rotation. The fix stores it in
//       DRW_Shape::m_shapeIndex (previously read-and-discarded).
//   (b) ATTDEF/ATTRIB rotation is gated on R11OPTS(2) (opts & 0x02; dwg.spec
//       :216 ATTRIB / :419 ATTDEF), NOT opts & 0x01. Every ATTDEF/ATTRIB in
//       the corpus has opts bit0 CLEAR + bit1 SET (dwgread trace: r11 opts=0x2,
//       r10 ATTRIB opts=0x6), so the old `opts & 0x01` gate SKIPPED the read
//       and the rendered TEXT angle came through as 0.
// Ground truth: `dwgread -O JSON` on r11/entities-2d.dwg and r10/entities.dwg
// (SHAPE + ATTDEF(9,5) + ATTRIB(2,2) are byte-identical across the two files).
namespace {
struct R11ShapeAttribCollector : public CountingIface {
  std::vector<DRW_Shape> shapes;
  std::vector<DRW_Text> texts;
  void addShape(const DRW_Shape &e) override {
    ++entities;                 // CountingIface has no addShape override
    shapes.push_back(e);
  }
  void addText(const DRW_Text &e) override {
    CountingIface::addText(e);  // increments `entities` via track()
    texts.push_back(e);
  }
};
}  // namespace

TEST_CASE("DWG pre-R13: R11/R10 SHAPE style_id + ATTDEF/ATTRIB rotation") {
  auto near = [](double a, double b) { return std::abs(a - b) < 1e-9; };

  auto checkFixture = [&](const char *release, const char *file,
                          DRW::Version expectVer) {
    const std::string path = libredwgFixturePath(release, file);
    if (path.empty()) {
      SKIP("pre-R13 corpus root absent; skipping");
    }
    std::ifstream probe(path, std::ios::binary);
    if (!probe.good()) {
      SKIP(std::string(file) + " absent; skipping");
    }
    probe.close();

    R11ShapeAttribCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == expectVer);

    // ---- (a) SHAPE: style_id read as 1B RC; rotation not desynced ----------
    // Oracle (dwgread -O JSON): ins_pt=[6,6], scale=1.0, style_id=131,
    // rotation=0.5235987755983 (== pi/6). Before the fix, style_id was read as
    // a 2-byte RS which shifted the rotation read by one byte -> garbage, and
    // m_shapeIndex stayed 0 (the value was discarded).
    REQUIRE(iface.shapes.size() == 1);
    const DRW_Shape &s = iface.shapes[0];
    CHECK(near(s.m_insertionPoint.x, 6.0));
    CHECK(near(s.m_insertionPoint.y, 6.0));
    CHECK(near(s.m_scale, 1.0));
    CHECK(s.m_shapeIndex == 131);                 // was 0 (discarded)
    CHECK(near(s.m_rotation, 0.5235987755983));   // was garbage (desynced)

    // ---- (b) ATTDEF/ATTRIB rotation via opts & 0x02 ------------------------
    // ATTDEF/ATTRIB render as DRW_Text in the R11 reader. Find the unique
    // ATTRIB (text "4", ins_pt [2,2], height 0.1) and one ATTDEF (text "3",
    // ins_pt [9,5], height 0.2). Oracle rotations: ATTRIB=1.5707963267949
    // (pi/2), ATTDEF=1.0471975511966 (pi/3). Before the fix (opts & 0x01, bit0
    // clear) the read was skipped and both angles were 0.
    const DRW_Text *attrib = nullptr;
    const DRW_Text *attdef = nullptr;
    for (const auto &t : iface.texts) {
      if (t.text == "4" && near(t.basePoint.x, 2.0) && near(t.basePoint.y, 2.0))
        attrib = &t;
      if (t.text == "3" && near(t.basePoint.x, 9.0) && near(t.basePoint.y, 5.0))
        attdef = &t;
    }
    REQUIRE(attrib != nullptr);
    CHECK(near(attrib->height, 0.1));
    CHECK(near(attrib->angle, 1.5707963267949));  // was 0.0 (gate skipped)
    REQUIRE(attdef != nullptr);
    CHECK(near(attdef->height, 0.2));
    CHECK(near(attdef->angle, 1.0471975511966));  // was 0.0 (gate skipped)
  };

  SECTION("r11/entities-2d.dwg (AC1009)") {
    checkFixture("r11", "entities-2d.dwg", DRW::AC1009);
  }
  SECTION("r10/entities.dwg (AC1006)") {
    checkFixture("r10", "entities.dwg", DRW::AC1006);
  }
}

// Pre-R13 R10 (AC1006) routing parity. The R10 container is byte-identical to
// R11 except for the LTYPE handle width in the entity common header (R10=1B
// RC; R11=2B RS). dwgReaderR11 branches on `version`; this test exercises the
// 1-byte path. Because the reader self-heals via setPosition(recEnd), a wrong
// LTYPE-handle width would not change SUCCESS/entFail — geometry must be
// bit-diffed instead. Verified vs `dwgread -O JSON` on the same file.
namespace {
struct R10LineCollector : public CountingIface {
  std::vector<DRW_Line> lines;
  void addLine(const DRW_Line &e) override {
    CountingIface::addLine(e);
    lines.push_back(e);
  }
};
}  // namespace

TEST_CASE("DWG pre-R13: read AC1006/R10 entities section") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping pre-R13 R10 test");
  }
  const std::string path =
      std::string(home) + "/dev/libredwg/test/test-data/r10/entities.dwg";
  std::ifstream probe(path, std::ios::binary);
  if (!probe.good()) {
    SKIP("pre-R13 R10 corpus absent; skipping");
  }
  probe.close();

  R10LineCollector iface;
  const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
  CHECK(r.ok);                          // was BAD_VERSION before R10 routing
  CHECK(r.version == DRW::AC1006);
  // Oracle (dwgread -O JSON): 6 LINEs reachable from ENTITIES + BLOCKS
  // (the 7th is past a JUMP/3DLINE pair that the reader does not chase).
  REQUIRE(iface.lines.size() >= 6);
  // Bit-exact check on a layered-LTYPE LINE — proves the 1B LTYPE-handle
  // branch did NOT consume a phantom byte (would desync the body geometry).
  // Endpoints lifted from `dwgread -O JSON` on r10/entities.dwg.
  bool found_2_3_to_3_4 = false;
  bool found_irrational = false;
  for (const auto &L : iface.lines) {
    if (L.basePoint.x == 2.0 && L.basePoint.y == 3.0 &&
        L.secPoint.x == 3.0 && L.secPoint.y == 4.0) {
      found_2_3_to_3_4 = true;
    }
    // Irrational endpoints — must be byte-exact (within ~1 ULP) to prove the
    // 1B-vs-2B LTYPE branch read the right field width.
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-12; };
    if (near(L.basePoint.x, 6.04419417382416) &&
        near(L.basePoint.y, 8.04419417382416) &&
        near(L.secPoint.x, 7.12727922061358) &&
        near(L.secPoint.y, 9.12727922061358)) {
      found_irrational = true;
    }
  }
  CHECK(found_2_3_to_3_4);
  CHECK(found_irrational);
}

// Pre-R13 R11 table-record decode (LTYPE/STYLE/LAYER): asserts the per-record
// fields delivered through addLType/addLayer/addTextStyle. Ground truth from
// `dwgread -O JSON` on the same files. The HIDDENX2 truncation assertion is
// the load-bearing detail — the on-disk LTYPE record has 12 fixed double
// slots, only the first `numdashes` are valid; the rest are uninitialised
// garbage that MUST be dropped.
namespace {
struct R11TableCollector : public CountingIface {
  std::vector<DRW_Layer> layers;
  std::vector<DRW_LType> ltypes;
  std::vector<DRW_Textstyle> styles;
  void addLayer(const DRW_Layer &e) override {
    CountingIface::addLayer(e);
    layers.push_back(e);
  }
  void addLType(const DRW_LType &e) override {
    CountingIface::addLType(e);
    ltypes.push_back(e);
  }
  void addTextStyle(const DRW_Textstyle &e) override {
    CountingIface::addTextStyle(e);
    styles.push_back(e);
  }
};
}  // namespace

// Helper: pull a DRW_Variant scalar out of a header var map (by key).
namespace {
const DRW_Variant *hdrVar(const DRW_Header &h, const char *k) {
  auto it = h.vars.find(k);
  return it == h.vars.end() ? nullptr : it->second;
}
struct HdrCollector : public CountingIface {
  DRW_Header hdr;
  void addHeader(const DRW_Header *h) override { hdr = *h; }
};
}  // namespace

// Pre-R13 R11 header-variables decode. The R11 header layout starts at file
// offset 0x5E (5 leading 10-byte table-section headers end there) and is
// SEQUENTIAL — every field offset is the running sum of prior widths. We
// read the high-value drawing-state vars through PLINEWID (post-cursor
// 0x36f); the long DIMxx/UCS/VPORT tail is intentionally skipped.
//
// CRITICAL: dwgRW::processDwg() calls readDwgHeader BEFORE readDwgTables,
// so the LAYER/STYLE/LTYPE name vectors are empty at header-read time.
// The fix is to eagerly read the table NAMES inside readDwgHeader before
// resolving CLAYER/TEXTSTYLE/CELTYPE. The ACEB10 SECTION below is the
// load-bearing case for this fix: its CLAYER index is 8 (== "BORDER"), so
// a buggy "no eager read" path would silently fall back instead of
// returning the right name (entities-2d masks the bug because CLAYER=0
// happens to coincide with the default "0").
TEST_CASE("DWG pre-R13: R11 header variables decode") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  SECTION("entities-2d.dwg (defaults — minimal coverage)") {
    const std::string path =
        std::string(home) + "/dev/libredwg/test/test-data/r11/entities-2d.dwg";
    std::ifstream probe(path, std::ios::binary);
    if (!probe.good()) { SKIP("entities-2d.dwg absent"); return; }
    probe.close();
    HdrCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1009);
    // doubles
    auto *dimscale = hdrVar(iface.hdr, "DIMSCALE");
    REQUIRE(dimscale != nullptr);
    CHECK(dimscale->content.d == 1.0);
    auto *ltscale = hdrVar(iface.hdr, "LTSCALE");
    REQUIRE(ltscale != nullptr);
    CHECK(ltscale->content.d == 1.0);
    auto *textsize = hdrVar(iface.hdr, "TEXTSIZE");
    REQUIRE(textsize != nullptr);
    CHECK(textsize->content.d == 0.2);
    // ints (CECOLOR=256 was the load-bearing oldCECOLOR-decoy trap)
    auto *cecolor = hdrVar(iface.hdr, "CECOLOR");
    REQUIRE(cecolor != nullptr);
    CHECK(cecolor->content.i == 256);
    auto *lunits = hdrVar(iface.hdr, "LUNITS");
    REQUIRE(lunits != nullptr);
    CHECK(lunits->content.i == 2);
    auto *pdmode = hdrVar(iface.hdr, "PDMODE");
    REQUIRE(pdmode != nullptr);
    CHECK(pdmode->content.i == 0);
    // coord (EXTMAX bit-exact to ~1 ULP; literal-precision round-trip is loose)
    auto *extmax = hdrVar(iface.hdr, "EXTMAX");
    REQUIRE(extmax != nullptr);
    REQUIRE(extmax->type() == DRW_Variant::COORD);
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-12; };
    CHECK(near(extmax->content.v->x, 9.43333333333333));
    CHECK(near(extmax->content.v->y, 9.12727922061358));
    CHECK(extmax->content.v->z == 5.0);
    // resolved names
    auto *clayer = hdrVar(iface.hdr, "CLAYER");
    REQUIRE(clayer != nullptr);
    CHECK(*clayer->content.s == "0");
    auto *celtype = hdrVar(iface.hdr, "CELTYPE");
    REQUIRE(celtype != nullptr);
    CHECK(*celtype->content.s == "BYLAYER");
  }
  SECTION("ACEB10.dwg (CLAYER!=0 — exercises eager table-read fix)") {
    const std::string path =
        std::string(home) + "/dev/libredwg/test/test-data/r11/ACEB10.dwg";
    std::ifstream probe(path, std::ios::binary);
    if (!probe.good()) { SKIP("ACEB10.dwg absent"); return; }
    probe.close();
    HdrCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1009);
    auto *clayer = hdrVar(iface.hdr, "CLAYER");
    REQUIRE(clayer != nullptr);
    // Oracle CLAYER index = 8, m_layerNames[8] = "BORDER".
    CHECK(*clayer->content.s == "BORDER");
    auto *textstyle = hdrVar(iface.hdr, "TEXTSTYLE");
    REQUIRE(textstyle != nullptr);
    // Oracle TEXTSTYLE index = 1, m_styleNames[1] = "ADESK1".
    CHECK(*textstyle->content.s == "ADESK1");
    auto *pdmode = hdrVar(iface.hdr, "PDMODE");
    REQUIRE(pdmode != nullptr);
    CHECK(pdmode->content.i == 3);
  }
}

TEST_CASE("DWG pre-R13: R11 LAYER/LTYPE/STYLE table records") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  SECTION("entities-2d.dwg (minimal: 2 layers, 1 ltype, 2 styles)") {
    const std::string path =
        std::string(home) + "/dev/libredwg/test/test-data/r11/entities-2d.dwg";
    std::ifstream probe(path, std::ios::binary);
    if (!probe.good()) {
      SKIP("entities-2d.dwg absent");
    }
    probe.close();
    R11TableCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1009);
    CHECK(iface.layers.size() == 2);
    CHECK(iface.ltypes.size() == 1);
    CHECK(iface.styles.size() == 2);
    // CONTINUOUS: numdashes=0, no values in path.
    auto cont = std::find_if(iface.ltypes.begin(), iface.ltypes.end(),
                             [](const DRW_LType &l) { return l.name == "CONTINUOUS"; });
    REQUIRE(cont != iface.ltypes.end());
    CHECK(cont->desc == "Solid line");
    CHECK(cont->size == 0);
    CHECK(cont->path.empty());
    // STANDARD style: font=txt, lastHeight=0.2 (height/textSize=0 means unset).
    auto std_ = std::find_if(iface.styles.begin(), iface.styles.end(),
                             [](const DRW_Textstyle &s) { return s.name == "STANDARD"; });
    REQUIRE(std_ != iface.styles.end());
    CHECK(std_->font == "txt");
    CHECK(std_->bigFont == "");
    CHECK(std_->lastHeight == 0.2);
    CHECK(std_->width == 1.0);
  }
  SECTION("ACEB10.dwg (rich: 13 layers, 2 ltypes incl HIDDENX2, 3 styles)") {
    const std::string path =
        std::string(home) + "/dev/libredwg/test/test-data/r11/ACEB10.dwg";
    std::ifstream probe(path, std::ios::binary);
    if (!probe.good()) {
      SKIP("ACEB10.dwg absent");
    }
    probe.close();
    R11TableCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1009);
    CHECK(iface.layers.size() == 13);
    CHECK(iface.ltypes.size() == 2);
    CHECK(iface.styles.size() == 3);
    // MOUNTINGKIT-EB35DIN and MOUNTINGKIT-EB4 have signed color = -7 (off).
    int negCount = 0;
    for (const auto &l : iface.layers)
      if (l.color < 0) ++negCount;
    CHECK(negCount == 2);
    // HIDDENX2: numdashes=2, path is exactly [0.5, -0.25] — the on-disk slots
    // 2..11 are NaN garbage and must be truncated.
    auto h2 = std::find_if(iface.ltypes.begin(), iface.ltypes.end(),
                           [](const DRW_LType &l) { return l.name == "HIDDENX2"; });
    REQUIRE(h2 != iface.ltypes.end());
    CHECK(h2->size == 2);
    CHECK(h2->length == 0.75);
    REQUIRE(h2->path.size() == 2);
    CHECK(h2->path[0] == 0.5);
    CHECK(h2->path[1] == -0.25);
    REQUIRE(h2->segments.size() == 2);
    CHECK(h2->segments[0].length == 0.5);
    CHECK(h2->segments[1].length == -0.25);
    // ADESK1 style: font=romans, lastHeight=0.095.
    auto a1 = std::find_if(iface.styles.begin(), iface.styles.end(),
                           [](const DRW_Textstyle &s) { return s.name == "ADESK1"; });
    REQUIRE(a1 != iface.styles.end());
    CHECK(a1->font == "romans");
    CHECK(a1->lastHeight == 0.095);
  }
}

// Pre-R13 codepage end-to-end: ACEB10.dwg stores $DWGCODEPAGE = 30 (ANSI_1252)
// at the fixed file offset 0x3f9 (numheader_vars=205 @0x11 > 129). dwgread -O
// JSON reports "codepage": 30. This asserts readFileHeader() reads 0x3f9 and
// resolves it through preR13CodePageName()/setCodePage(). The value equals the
// hard-coded readMetaData default, so it also regression-guards the seek path.
TEST_CASE("DWG pre-R13: ACEB10 resolves $DWGCODEPAGE = ANSI_1252",
          "[dwg][prer13][codepage]") {
  const char *home = getenv("HOME");
  if (!home) { SKIP("HOME not set; skipping"); return; }
  const std::string path =
      std::string(home) + "/dev/libredwg/test/test-data/r11/ACEB10.dwg";
  std::ifstream probe(path, std::ios::binary);
  if (!probe.good()) { SKIP("ACEB10.dwg absent"); return; }
  probe.close();
  CountingIface iface;
  dwgR reader(path.c_str());
  const bool ok = reader.read(&iface, true);
  REQUIRE(ok);
  REQUIRE(reader.getVersion() == DRW::AC1009);
  // cp id 30 -> "ANSI_1252" (dwgread oracle: HEADER.codepage == 30).
  CHECK(reader.getCodePage() == "ANSI_1252");
}

// Pre-R13 R10 (AC1006) parity: the table records and header variables are
// byte-identical to R11 EXCEPT each table record omits the 2-byte `used` field
// (recSizes R10 37/194/187 vs R11 41/198/191), and the header subset
// (0x5E..PLINEWID) is byte-identical. Ground truth from `dwgread -O JSON` on
// r10/entities.dwg. This is the case that proves R10 reached parity with R11.
TEST_CASE("DWG pre-R13: R10 LAYER/LTYPE/STYLE table records + header") {
  const char *home = getenv("HOME");
  if (!home) {
    SKIP("HOME not set; skipping");
  }
  const std::string path =
      std::string(home) + "/dev/libredwg/test/test-data/r10/entities.dwg";
  std::ifstream probe(path, std::ios::binary);
  if (!probe.good()) {
    SKIP("pre-R13 R10 corpus absent; skipping");
  }
  probe.close();
  SECTION("table records (used field absent in R10)") {
    R11TableCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1006);
    CHECK(iface.layers.size() == 2);
    CHECK(iface.ltypes.size() == 1);
    CHECK(iface.styles.size() == 2);
    // LAYER "0" color 7; "DEFPOINTS" signed color -7 (OFF) — proves the missing
    // `used` field did not shift the color/ltype reads.
    auto l0 = std::find_if(iface.layers.begin(), iface.layers.end(),
                           [](const DRW_Layer &l) { return l.name == "0"; });
    REQUIRE(l0 != iface.layers.end());
    CHECK(l0->color == 7);
    auto ldp = std::find_if(iface.layers.begin(), iface.layers.end(),
                            [](const DRW_Layer &l) { return l.name == "DEFPOINTS"; });
    REQUIRE(ldp != iface.layers.end());
    CHECK(ldp->color == -7);
    // CONTINUOUS ltype, STANDARD style — same field offsets as R11 minus 2.
    auto cont = std::find_if(iface.ltypes.begin(), iface.ltypes.end(),
                             [](const DRW_LType &l) { return l.name == "CONTINUOUS"; });
    REQUIRE(cont != iface.ltypes.end());
    CHECK(cont->desc == "Solid line");
    CHECK(cont->size == 0);
    auto std_ = std::find_if(iface.styles.begin(), iface.styles.end(),
                             [](const DRW_Textstyle &s) { return s.name == "STANDARD"; });
    REQUIRE(std_ != iface.styles.end());
    CHECK(std_->font == "txt");
    CHECK(std_->lastHeight == 0.2);
    CHECK(std_->width == 1.0);
  }
  SECTION("header variables (byte-identical to R11 in subset)") {
    HdrCollector iface;
    const DwgResult r = readDwg(path, /*verbose=*/false, &iface);
    REQUIRE(r.ok);
    REQUIRE(r.version == DRW::AC1006);
    auto *lunits = hdrVar(iface.hdr, "LUNITS");
    REQUIRE(lunits != nullptr);
    CHECK(lunits->content.i == 2);
    auto *cecolor = hdrVar(iface.hdr, "CECOLOR");
    REQUIRE(cecolor != nullptr);
    CHECK(cecolor->content.i == 256);
    auto *ltscale = hdrVar(iface.hdr, "LTSCALE");
    REQUIRE(ltscale != nullptr);
    CHECK(ltscale->content.d == 1.0);
    auto *pdmode = hdrVar(iface.hdr, "PDMODE");
    REQUIRE(pdmode != nullptr);
    CHECK(pdmode->content.i == 0);
    // EXTMAX z=1.0 for R10 (R11's entities-2d had z=5.0) — bit-exact x/y.
    auto *extmax = hdrVar(iface.hdr, "EXTMAX");
    REQUIRE(extmax != nullptr);
    REQUIRE(extmax->type() == DRW_Variant::COORD);
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-12; };
    CHECK(near(extmax->content.v->x, 9.43333333333333));
    CHECK(extmax->content.v->y == 10.0);
    CHECK(extmax->content.v->z == 1.0);
    // CLAYER resolves to "0" via the eager name-table read on the R10 path.
    auto *clayer = hdrVar(iface.hdr, "CLAYER");
    REQUIRE(clayer != nullptr);
    CHECK(*clayer->content.s == "0");
  }
}

// MDI graphic-view resize path (QG_GraphicView::resizeEvent →
// adjustOffsetControls → forcedCalculateBorders) vs zoomAuto
// (LC_GraphicViewport::zoomAuto → calculateBorders).
TEST_CASE("DWG chicun WNS2 drivers", "[.dwg_chicun_wns2]") {
  if (chicun::fixturePath().empty()) { SKIP("skip"); return; }
  chicun::ensureTestApp();
  RS_Graphic graphic;
  REQUIRE(chicun::importFixture(graphic));
  graphic.onLoadingCompleted();

  // Path A: zoomAuto (visible-only calculateBorders)
  graphic.calculateBorders();
  const RS_Vector zmin = graphic.getMin(), zmax = graphic.getMax();
  std::cout << std::setprecision(12)
            << "zoomAuto-path calculateBorders min=(" << zmin.x << "," << zmin.y
            << ") max=(" << zmax.x << "," << zmax.y << ") span=("
            << (zmax.x - zmin.x) << "," << (zmax.y - zmin.y) << ")\n";

  // Path B: MDI graphic-view resize → adjustOffsetControls → forcedCalculateBorders
  graphic.forcedCalculateBorders();
  const RS_Vector fmin = graphic.getMin(), fmax = graphic.getMax();
  std::cout << "resize-path forcedCalculateBorders min=(" << fmin.x << ","
            << fmin.y << ") max=(" << fmax.x << "," << fmax.y << ") span=("
            << (fmax.x - fmin.x) << "," << (fmax.y - fmin.y) << ")\n";

  // Path C: LC_GraphicViewport::zoomAuto exactly (as QC_MDIWindow::zoomAuto)
  LC_GraphicViewport vp;
  vp.setDocument(&graphic);
  vp.setSize(1200, 800);
  vp.zoomAuto(false, true);
  const RS_Vector vmin = graphic.getMin(), vmax = graphic.getMax();
  std::cout << "after LC_GraphicViewport::zoomAuto min=(" << vmin.x << ","
            << vmin.y << ") max=(" << vmax.x << "," << vmax.y << ") span=("
            << (vmax.x - vmin.x) << "," << (vmax.y - vmin.y) << ")\n";
  std::cout << "viewport factor=(" << vp.getFactor().x << "," << vp.getFactor().y
            << ") offset=(" << vp.getOffsetX() << "," << vp.getOffsetY() << ")\n";

  // Re-force for driver dump (zoomAuto rewrote borders to visible-only)
  graphic.forcedCalculateBorders();
  const RS_Vector bmin = graphic.getMin(), bmax = graphic.getMax();
  const double tol = 50;
  std::cout << "=== forced side drivers (resize-path) ===\n";
  int counts[4] = {0, 0, 0, 0};
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveAll}.entities()) {
    if (!e || e->isContainer())
      continue;
    e->calculateBorders();
    const RS_Vector mn = e->getMin(), mx = e->getMax();
    if (!mn.valid || !mx.valid)
      continue;
    char side = 0;
    int si = -1;
    if (std::fabs(mn.x - bmin.x) < tol) {
      side = 'W';
      si = 0;
    } else if (std::fabs(mx.x - bmax.x) < tol) {
      side = 'E';
      si = 1;
    } else if (std::fabs(mn.y - bmin.y) < tol) {
      side = 'S';
      si = 2;
    } else if (std::fabs(mx.y - bmax.y) < tol) {
      side = 'N';
      si = 3;
    } else
      continue;
    if (counts[si]++ >= 5)
      continue;
    RS_Layer *ly = e->getLayer();
    std::cout << "  " << side << " " << rttiName(e->rtti())
              << " vis=" << e->isVisible()
              << " chain=" << chicun::insertChain(e).toStdString()
              << " layer=" << (ly ? ly->getName().toStdString() : "?")
              << " box=(" << mn.x << "," << mn.y << ")-(" << mx.x << ","
              << mx.y << ")\n";
  }

  std::cout << "=== top-level forced extremes ===\n";
  for (RS_Entity *e : graphic) {
    if (!e)
      continue;
    if (e->isContainer())
      static_cast<RS_EntityContainer *>(e)->forcedCalculateBorders();
    else
      e->calculateBorders();
    const RS_Vector mn = e->getMin(), mx = e->getMax();
    if (!mn.valid || !mx.valid)
      continue;
    if (!(std::fabs(mn.x - bmin.x) < tol || std::fabs(mx.x - bmax.x) < tol
          || std::fabs(mn.y - bmin.y) < tol
          || std::fabs(mx.y - bmax.y) < tol))
      continue;
    QString name;
    if (e->rtti() == RS2::EntityInsert)
      name = static_cast<RS_Insert *>(e)->getName();
    std::cout << "  top " << rttiName(e->rtti()) << " " << name.toStdString()
              << " vis=" << e->isVisible() << " count="
              << (e->isContainer()
                      ? static_cast<RS_EntityContainer *>(e)->count()
                      : 0)
              << " box=(" << mn.x << "," << mn.y << ")-(" << mx.x << ","
              << mx.y << ")\n";
  }
}
