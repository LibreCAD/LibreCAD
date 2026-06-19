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
 * End-to-end DXF round-trip through RS_FilterDXFRW (slice A2 verification).
 *
 * Unlike dxf_object_tests.cpp (codec-level, via stub DRW_Interface), this drives
 * the real filter: fileImport a DXF containing an unmodeled OBJECT and an
 * unmodeled ENTITY into an RS_Graphic (where the raw groups land in
 * LC_DwgAdvancedMetadata), then fileExport to DXF and confirm both survive —
 * proving the graphic-backed raw store bridges the separate read/write filter
 * instances.
 */

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <QCoreApplication>

#include "lc_dwgadvancedmetadata.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_entity.h"
#include "rs_layer.h"
#include "rs_settings.h"

namespace {

void ensureSettings() {
  static int argc = 1;
  static char arg0[] = "librecad_tests";
  static char *argv[] = {arg0, nullptr};
  static QCoreApplication *app = QCoreApplication::instance()
                                     ? QCoreApplication::instance()
                                     : new QCoreApplication(argc, argv);
  static bool ready = [] {
    QCoreApplication::setOrganizationName("LibreCAD");
    QCoreApplication::setApplicationName("LibreCAD-tests");
    RS_Settings::init("LibreCAD", "LibreCAD-tests");
    return true;
  }();
  (void)app;
  (void)ready;
}

std::string tmpFile(const char *suffix) {
  return (std::filesystem::temp_directory_path() /
          (std::string("dxf_roundtrip_") + suffix))
      .string();
}

void writeText(const std::string &path, const std::string &content) {
  std::ofstream out(path);
  out << content;
}

// Counts occurrences of a "0\n<NAME>\n" record marker in a DXF file.
int countRecords(const std::string &path, const std::string &name) {
  std::ifstream in(path);
  std::string line;
  int count = 0;
  bool prevZero = false;
  while (std::getline(in, line)) {
    // strip possible trailing CR
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    std::string trimmed = line;
    size_t a = trimmed.find_first_not_of(" \t");
    if (a != std::string::npos)
      trimmed = trimmed.substr(a);
    if (prevZero && trimmed == name)
      ++count;
    prevZero = (trimmed == "0");
  }
  return count;
}

// Collects every group-5 (handle) value in a DXF file, in order. DXF is a
// strict (code, value) pair stream, so read two lines at a time — scanning
// line-by-line would confuse a *value* of "5" with the group-5 code.
std::vector<std::string> collectHandles(const std::string &path) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::vector<std::string> handles;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    if (trim(codeLine) == "5")
      handles.push_back(trim(valueLine));
  }
  return handles;
}

// True if a record named `recordName` (0/<name>) contains group `code` before
// the next 0-record begins.
bool recordHasCode(const std::string &path, const std::string &recordName,
                   const std::string &code) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  bool inRecord = false;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trim(codeLine), v = trim(valueLine);
    if (c == "0")
      inRecord = (v == recordName);
    else if (inRecord && c == code)
      return true;
  }
  return false;
}

// Returns the (entry-name -> handle) map of the root NamedObjectsDictionary
// (the DICTIONARY whose code-5 handle is "C"): its 3/350 entry pairs.
std::map<std::string, std::string> rootDictEntries(const std::string &path) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::map<std::string, std::string> entries;
  enum { Other, DictPendingHandle, InRoot } state = Other;
  std::string pendingName;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string code = trim(codeLine), val = trim(valueLine);
    if (code == "0")
      state = (val == "DICTIONARY") ? DictPendingHandle : Other;
    else if (code == "5" && state == DictPendingHandle)
      state = (val == "C") ? InRoot : Other;
    else if (state == InRoot && code == "3")
      pendingName = val;
    else if (state == InRoot && code == "350" && !pendingName.empty()) {
      entries[pendingName] = val;
      pendingName.clear();
    }
  }
  return entries;
}

// Returns the $HANDSEED header value parsed as an integer (hex), or 0 if absent.
unsigned long handseedValue(const std::string &path) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  bool pending = false;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trim(codeLine), v = trim(valueLine);
    if (pending && c == "5")
      return std::strtoul(v.c_str(), nullptr, 16);
    pending = (c == "9" && v == "$HANDSEED");
  }
  return 0;
}

// Returns the largest code-5/105 handle (hex) emitted in the body sections
// (TABLES/BLOCKS/ENTITIES/OBJECTS), excluding the HEADER's $HANDSEED code-5.
unsigned long maxHandle(const std::string &path) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  unsigned long m = 0;
  std::string section;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trim(codeLine), v = trim(valueLine);
    if (c == "2" && (v == "HEADER" || v == "CLASSES" || v == "TABLES" ||
                     v == "BLOCKS" || v == "ENTITIES" || v == "OBJECTS"))
      section = v;
    else if (section != "HEADER" && (c == "5" || c == "105"))
      m = std::max(m, std::strtoul(v.c_str(), nullptr, 16));
  }
  return m;
}

// Returns the set of CLASS record names (code 1) in the CLASSES section.
std::set<std::string> classRecordNames(const std::string &path) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::set<std::string> names;
  bool expectName = false;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string code = trim(codeLine), val = trim(valueLine);
    if (expectName && code == "1") {
      names.insert(val);
      expectName = false;
    } else if (code == "0") {
      expectName = (val == "CLASS");
    }
  }
  return names;
}

} // namespace

TEST_CASE("DXF round-trip via RS_FilterDXFRW preserves unmodeled object + entity",
          "[dxf][roundtrip][filter]") {
  ensureSettings();
  const std::string src = tmpFile("src.dxf");
  const std::string out = tmpFile("out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // A minimal but valid DXF: one real LINE (so the file is non-trivial), an
  // unmodeled MATERIAL object, and an unmodeled WEIRDENT entity.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n10.0\n31\n0.0\n"
      "0\nWEIRDENT\n8\n0\n5\n4A\n62\n3\n10\n1.0\n20\n2.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n100\nAcDbDictionary\n3\nACAD_MATERIAL\n350\n3A\n"
      "0\nMATERIAL\n5\n3B\n330\nC\n100\nAcDbMaterial\n1\nMyMaterial\n94\n63\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  // Import through the real filter into a graphic.
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  // The unmodeled object + entity must have landed in the graphic's metadata.
  const auto &meta = graphic.dwgAdvancedMetadata();
  bool sawMaterial = false;
  for (const DRW_RawDxfObject &o : meta.rawDxfObjects()) {
    if (o.name == "MATERIAL")
      sawMaterial = true;
  }
  bool sawWeird = false;
  for (const DRW_RawDxfObject &e : meta.rawDxfEntities()) {
    if (e.name == "WEIRDENT")
      sawWeird = true;
  }
  CHECK(sawMaterial);
  CHECK(sawWeird);

  // Export to DXF and confirm both records were re-emitted.
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "MATERIAL") >= 1);
  CHECK(countRecords(out, "WEIRDENT") >= 1);
  // MATERIAL is a known custom OBJECT → it gets a CLASS record (AutoCAD-clean).
  // WEIRDENT is an arbitrary/unknown entity → no CLASS (lossless LC<->LC only).
  const std::set<std::string> classes = classRecordNames(out);
  CHECK(classes.count("MATERIAL") == 1);
  CHECK(classes.count("WEIRDENT") == 0);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// D4 write-path: a LIGHT read from a DWG lands only on the metadata shelf
// (LibreCAD has no RS_Light), so DWG->DXF export used to silently drop it.
// writeEntities now re-emits metadata.lights() as typed AcDbLight entities
// (R2007+). Seed a light directly and confirm it survives the DXF export.
TEST_CASE("DXF export re-emits DWG-read LIGHT entities", "[dxf][roundtrip][filter][light]") {
  ensureSettings();
  const std::string src = tmpFile("lightsrc.dxf");
  const std::string out = tmpFile("light.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // Minimal valid DXF (one LINE) to set the graphic up like a real import.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n10.0\n31\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  // Seed a LIGHT on the metadata shelf, exactly as the DWG read path would.
  {
    DRW_Light light;
    light.handle = 0x300;
    light.parentHandle = 0x1F;  // Model_Space BLOCK_RECORD
    light.m_name = "TESTLIGHT";
    light.m_type = 2;           // point light
    light.m_status = true;
    light.m_intensity = 0.75;
    light.m_position.x = 1.0; light.m_position.y = 2.0; light.m_position.z = 3.0;
    light.m_target.x = 4.0; light.m_target.y = 5.0; light.m_target.z = 6.0;
    light.m_hotspotAngle = 45.0;
    light.m_falloffAngle = 60.0;
    graphic.dwgAdvancedMetadata().addLight(light);
  }

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));  // AC1021 (R2007+)
  }

  CHECK(countRecords(out, "LIGHT") == 1);
  CHECK(recordHasCode(out, "LIGHT", "100"));  // AcDbLight subclass marker
  CHECK(recordHasCode(out, "LIGHT", "1"));    // name
  CHECK(recordHasCode(out, "LIGHT", "40"));   // intensity
  CHECK(recordHasCode(out, "LIGHT", "10"));   // position

  bool sawName = false;
  std::ifstream in(out);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line == "TESTLIGHT") { sawName = true; break; }
  }
  CHECK(sawName);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// D4 write-path: a SHAPE read from a DWG lands only on the metadata shelf; the
// export now re-emits it as a typed AcDbShape with group 2 = the resolved
// SHAPEFILE/STYLE name (the glyph index is not round-trippable without the .shx).
TEST_CASE("DXF export re-emits DWG-read SHAPE entities", "[dxf][roundtrip][filter][shape]") {
  ensureSettings();
  const std::string src = tmpFile("shapesrc.dxf");
  const std::string out = tmpFile("shape.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n10.0\n31\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  {
    DRW_Shape shape;
    shape.handle = 0x300;
    shape.parentHandle = 0x1F;
    shape.m_styleName = "TESTSHAPE";
    shape.m_shapeIndex = 5;
    shape.m_insertionPoint.x = 1.0; shape.m_insertionPoint.y = 2.0;
    shape.m_scale = 2.5;          // size -> DXF 40
    shape.m_rotation = 0.0;
    shape.m_widthFactor = 1.0;
    graphic.dwgAdvancedMetadata().addShape(shape);
  }

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "SHAPE") == 1);
  CHECK(recordHasCode(out, "SHAPE", "100"));  // AcDbShape subclass marker
  CHECK(recordHasCode(out, "SHAPE", "2"));    // shape (style) name
  CHECK(recordHasCode(out, "SHAPE", "40"));   // size

  bool sawName = false;
  std::ifstream in(out);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line == "TESTSHAPE") { sawName = true; break; }
  }
  CHECK(sawName);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF CLASSES section round-trips source custom entity metadata",
          "[dxf][roundtrip][filter][classes]") {
  ensureSettings();
  const std::string src = tmpFile("classsrc.dxf");
  const std::string out = tmpFile("classout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  const std::string dxf =
      "0\nSECTION\n2\nCLASSES\n"
      "0\nCLASS\n1\nWEIRDENT\n2\nAcDbWeirdEntity\n3\nCUSTOM_APP\n"
      "90\n4095\n91\n1\n280\n0\n281\n1\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n5\n7B\n100\nAcDbEntity\n8\n0\n"
      "100\nAcDbWeirdEntity\n10\n1.0\n20\n2.0\n30\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  const auto &metadata = graphic.dwgAdvancedMetadata();
  bool sawClass = false;
  for (const DRW_Class &cls : metadata.dxfClasses()) {
    if (cls.recName == "WEIRDENT" && cls.className == "AcDbWeirdEntity"
        && cls.appName == "CUSTOM_APP") {
      sawClass = true;
    }
  }
  CHECK(sawClass);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "WEIRDENT") == 1);
  CHECK(classRecordNames(out).count("WEIRDENT") == 1);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export reserves handle space so preserved raw handles do not "
          "collide with minted handles",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("hsrc.dxf");
  const std::string out = tmpFile("hout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // Raw entity (0x33) and raw object (0x34) carry LOW original handles that sit
  // squarely in the band LibreCAD mints (++entCount from 0x30) for the LINE
  // entities + tables on export. Without the handle-floor reserve these would
  // duplicate a freshly-minted handle; with it, minted handles start above 0x34.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nLINE\n8\n0\n10\n1.0\n20\n1.0\n11\n2.0\n21\n2.0\n"
      "0\nLINE\n8\n0\n10\n2.0\n20\n2.0\n11\n3.0\n21\n3.0\n"
      "0\nWEIRDENT\n8\n0\n5\n33\n62\n3\n10\n1.0\n20\n2.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nMATERIAL\n5\n34\n330\nC\n100\nAcDbMaterial\n1\nMyMaterial\n94\n7\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // Core invariant: every code-5 handle in the exported file is unique.
  const std::vector<std::string> handles = collectHandles(out);
  std::set<std::string> seen;
  for (const std::string &h : handles) {
    INFO("duplicate handle: " << h);
    CHECK(seen.insert(h).second);
  }
  // The preserved raw handles survive verbatim (reserve, not remap) — they sit
  // above the codec's fixed structural band, so there is no collision to remap.
  CHECK(std::count(handles.begin(), handles.end(), std::string("33")) == 1);
  CHECK(std::count(handles.begin(), handles.end(), std::string("34")) == 1);
  // $HANDSEED is strictly above every emitted body handle.
  CHECK(handseedValue(out) > maxHandle(out));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

namespace {
// Finds a raw object by record name in a graphic's metadata, or nullptr.
const DRW_RawDxfObject *findRaw(const LC_DwgAdvancedMetadata &meta,
                                const char *name) {
  for (const DRW_RawDxfObject &o : meta.rawDxfObjects())
    if (o.name == name)
      return &o;
  return nullptr;
}
// Returns the first group with the given code in a raw object, or nullptr.
const DRW_Variant *group(const DRW_RawDxfObject &o, int code) {
  for (const DRW_Variant &g : o.groups)
    if (g.code() == code)
      return &g;
  return nullptr;
}
} // namespace

TEST_CASE("DXF data-only OBJECTS round-trip their body values via the raw net "
          "(hybrid typed-export resolution)",
          "[dxf][roundtrip][filter][dataonly]") {
  ensureSettings();
  const std::string src = tmpFile("dsrc.dxf");
  const std::string out = tmpFile("dout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // OBJECTS section with one of each data-only type carrying distinctive numeric
  // and string body values. These are typed-read into metadata (DWG path) AND
  // captured into the raw net (DXF re-emit). Spine types are intentionally absent.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSUN\n5\n50\n330\nC\n100\nAcDbSun\n90\n1\n290\n1\n63\n7\n40\n0.75\n"
      "0\nSCALE\n5\n51\n330\nC\n100\nAcDbScale\n300\nHalf\n140\n1.0\n141\n2.0\n290\n1\n"
      "0\nDICTIONARYVAR\n5\n52\n330\nC\n100\nDictionaryVariables\n280\n0\n1\nLWDISPLAY\n"
      "0\nRASTERVARIABLES\n5\n53\n330\nC\n100\nAcDbRasterVariables\n90\n0\n70\n1\n71\n1\n72\n3\n"
      "0\nWIPEOUTVARIABLES\n5\n54\n330\nC\n100\nAcDbWipeoutVariables\n70\n1\n"
      // MLINESTYLE has repeated per-element groups (49/62/6) — verbatim raw
      // capture must preserve them in order.
      "0\nMLINESTYLE\n5\n55\n330\nC\n100\nAcDbMlineStyle\n2\nMYSTYLE\n70\n0\n3\n\n"
      "62\n256\n51\n90.0\n52\n90.0\n71\n2\n49\n0.5\n62\n1\n6\nBYLAYER\n"
      "49\n-0.5\n62\n1\n6\nBYLAYER\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  // On read, each data-only type lands in the raw net with correctly-typed,
  // correctly-valued body groups (validates capture fix + routing together).
  {
    const auto &meta = graphic.dwgAdvancedMetadata();
    const DRW_RawDxfObject *sun = findRaw(meta, "SUN");
    REQUIRE(sun != nullptr);
    const DRW_Variant *intensity = group(*sun, 40);
    REQUIRE(intensity != nullptr);
    CHECK(intensity->type() == DRW_Variant::DOUBLE);
    CHECK(intensity->d_val() == 0.75);

    const DRW_RawDxfObject *scale = findRaw(meta, "SCALE");
    REQUIRE(scale != nullptr);
    const DRW_Variant *num = group(*scale, 141);
    REQUIRE(num != nullptr);
    CHECK(num->d_val() == 2.0);

    const DRW_RawDxfObject *dvar = findRaw(meta, "DICTIONARYVAR");
    REQUIRE(dvar != nullptr);
    const DRW_Variant *val = group(*dvar, 1);
    REQUIRE(val != nullptr);
    CHECK(std::string(val->c_str()) == "LWDISPLAY");
  }

  // Export, then re-import: the body values must survive the full DXF->DXF trip.
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  for (const char *name : {"SUN", "SCALE", "DICTIONARYVAR", "RASTERVARIABLES",
                           "WIPEOUTVARIABLES", "MLINESTYLE"})
    CHECK(countRecords(out, name) >= 1);

  // A3: the 5 custom-class types get a CLASS record so AutoCAD/ODA accept them;
  // MLINESTYLE is a fixed built-in and must NOT get one.
  const std::set<std::string> classes = classRecordNames(out);
  for (const char *name :
       {"SUN", "SCALE", "DICTIONARYVAR", "RASTERVARIABLES", "WIPEOUTVARIABLES"}) {
    INFO("missing CLASS record: " << name);
    CHECK(classes.count(name) == 1);
  }
  CHECK(classes.count("MLINESTYLE") == 0);

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  {
    const auto &meta = graphic2.dwgAdvancedMetadata();
    const DRW_RawDxfObject *sun = findRaw(meta, "SUN");
    REQUIRE(sun != nullptr);
    const DRW_Variant *intensity = group(*sun, 40);
    REQUIRE(intensity != nullptr);
    CHECK(intensity->d_val() == 0.75);  // double value survives DXF->DXF
    const DRW_RawDxfObject *dvar = findRaw(meta, "DICTIONARYVAR");
    REQUIRE(dvar != nullptr);
    const DRW_Variant *val = group(*dvar, 1);
    REQUIRE(val != nullptr);
    CHECK(std::string(val->c_str()) == "LWDISPLAY");
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// F4 dedup regression: a DXF-READ data-only OBJECT lands in BOTH the raw net AND
// the typed metadata (processSun calls addSun AND addRawDxfObject). The raw net
// re-emits it, and the F4 typed writers ALSO emit from metadata — so on DXF->DXF
// the same object could be written TWICE. The filter must skip the typed emit
// when the record's code-5 handle is already present in the raw net. Assert each
// of the 4 F4 types appears EXACTLY ONCE (count == 1) after a DXF->DXF export.
TEST_CASE("DXF F4 typed writers do not double-emit raw-net data-only OBJECTS "
          "(DXF->DXF dedup; SUN/SCALE/DICTIONARYVAR/RASTERVARIABLES count==1)",
          "[dxf][roundtrip][filter][dataonly]") {
  ensureSettings();
  const std::string src = tmpFile("f4src.dxf");
  const std::string out = tmpFile("f4out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nSUN\n5\n50\n330\nC\n100\nAcDbSun\n90\n1\n290\n1\n63\n7\n40\n0.75\n"
      "0\nSCALE\n5\n51\n330\nC\n100\nAcDbScale\n300\nHalf\n140\n1.0\n141\n2.0\n290\n1\n"
      "0\nDICTIONARYVAR\n5\n52\n330\nC\n100\nDictionaryVariables\n280\n0\n1\nLWDISPLAY\n"
      "0\nRASTERVARIABLES\n5\n53\n330\nC\n100\nAcDbRasterVariables\n90\n0\n70\n1\n71\n1\n72\n3\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  for (const char *name :
       {"SUN", "SCALE", "DICTIONARYVAR", "RASTERVARIABLES"}) {
    INFO("expected exactly one " << name);
    CHECK(countRecords(out, name) == 1);
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF named dictionaries round-trip and stay reachable from the root "
          "(spine-dict subset; referential integrity)",
          "[dxf][roundtrip][filter][spinedict]") {
  ensureSettings();
  const std::string src = tmpFile("ksrc.dxf");
  const std::string out = tmpFile("kout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // Root NamedObjectsDictionary (C) referencing ACAD_GROUP(D) + two named child
  // dicts (ACAD_SCALELIST @ 0x50, ACAD_MATERIAL @ 0x60), each owning a data
  // object that the raw net also preserves (SCALE @ 0x51, MATERIAL @ 0x61). On
  // export the codec regenerates root C; the routed child dicts must be
  // re-attached under it (3/350) and not duplicate C/D.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n3\nACAD_SCALELIST\n350\n50\n3\nACAD_MATERIAL\n350\n60\n"
      "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "0\nDICTIONARY\n5\n50\n330\nC\n100\nAcDbDictionary\n281\n1\n3\nMyScale\n350\n51\n"
      "0\nSCALE\n5\n51\n330\n50\n100\nAcDbScale\n300\nMyScale\n140\n1.0\n141\n2.0\n290\n1\n"
      "0\nDICTIONARY\n5\n60\n330\nC\n100\nAcDbDictionary\n281\n1\n3\nMyMat\n350\n61\n"
      "0\nMATERIAL\n5\n61\n330\n60\n100\nAcDbMaterial\n1\nMyMat\n94\n7\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  // The two NON-root named dicts are routed into the raw net; the root (330=0)
  // must NOT enter the raw net (codec regenerates it at fixed C).
  {
    const auto &meta = graphic.dwgAdvancedMetadata();
    bool saw50 = false, saw60 = false, sawRootC = false;
    for (const DRW_RawDxfObject &o : meta.rawDxfObjects()) {
      if (o.name == "DICTIONARY" && o.handle == 0x50u) saw50 = true;
      if (o.name == "DICTIONARY" && o.handle == 0x60u) saw60 = true;
      if (o.name == "DICTIONARY" && o.handle == 0xCu) sawRootC = true;
    }
    CHECK(saw50);
    CHECK(saw60);
    CHECK_FALSE(sawRootC);
  }

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // (a) exactly one root dict at C and one ACAD_GROUP at D; all handles unique.
  const std::vector<std::string> handles = collectHandles(out);
  std::set<std::string> seen;
  for (const std::string &h : handles) {
    INFO("duplicate handle: " << h);
    CHECK(seen.insert(h).second);
  }
  CHECK(std::count(handles.begin(), handles.end(), std::string("C")) == 1);
  CHECK(std::count(handles.begin(), handles.end(), std::string("D")) == 1);

  // (b) the routed child dicts are re-attached under the regenerated root C.
  // Referential integrity (not literal handle values): each root 350 target must
  // resolve to an actually-emitted code-5 handle. These source handles do not
  // collide with the codec's fixed structural set, so they are preserved
  // verbatim; the test asserts the entry RESOLVES rather than hardcoding "50".
  const std::map<std::string, std::string> rootEntries = rootDictEntries(out);
  const std::set<std::string> handleSet(handles.begin(), handles.end());
  REQUIRE(rootEntries.count("ACAD_SCALELIST") == 1);
  REQUIRE(rootEntries.count("ACAD_MATERIAL") == 1);
  CHECK(handleSet.count(rootEntries.at("ACAD_SCALELIST")) == 1);
  CHECK(handleSet.count(rootEntries.at("ACAD_MATERIAL")) == 1);
  CHECK(rootEntries.count("ACAD_GROUP") == 1);  // codec's own entry -> D
  CHECK(rootEntries.at("ACAD_GROUP") == "D");   // fixed structural literal
  // The preserved (non-colliding) child-dict handles survive verbatim.
  CHECK(rootEntries.at("ACAD_SCALELIST") == "50");
  CHECK(rootEntries.at("ACAD_MATERIAL") == "60");

  // (c) referential integrity: every root 350 target + child-dict entry target
  // is actually emitted (no dangling refs an auditor would prune).
  CHECK(handleSet.count("50") == 1);  // ACAD_SCALELIST dict present
  CHECK(handleSet.count("60") == 1);  // ACAD_MATERIAL dict present
  CHECK(handleSet.count("51") == 1);  // SCALE owned by ACAD_SCALELIST
  CHECK(handleSet.count("61") == 1);  // MATERIAL owned by ACAD_MATERIAL

  // (d) the named dicts survive a full DXF->DXF re-import.
  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  {
    const auto &meta = graphic2.dwgAdvancedMetadata();
    bool saw50 = false, saw60 = false;
    for (const DRW_RawDxfObject &o : meta.rawDxfObjects()) {
      if (o.name == "DICTIONARY" && o.handle == 0x50u) saw50 = true;
      if (o.name == "DICTIONARY" && o.handle == 0x60u) saw60 = true;
    }
    CHECK(saw50);
    CHECK(saw60);
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export gives PLOTSETTINGS an owner handle (no ownerless prune)",
          "[dxf][roundtrip][filter][plotsettings]") {
  // The codec emits a synthesized PLOTSETTINGS on every DXF export; without a
  // 330 owner, conforming readers (ezdxf/AutoCAD AUDIT) delete it as ownerless,
  // dropping LibreCAD's page setup. Confirmed clean via ezdxf 1.4.4 audit
  // (0 errors) after this fix; this test guards it in-repo.
  ensureSettings();
  const std::string src = tmpFile("psrc.dxf");
  const std::string out = tmpFile("pout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n10.0\n21\n10.0\n"
            "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  REQUIRE(countRecords(out, "PLOTSETTINGS") >= 1);
  CHECK(recordHasCode(out, "PLOTSETTINGS", "330"));  // owner present

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF unmodeled custom ENTITY round-trips with a CLASS record (HELIX)",
          "[dxf][roundtrip][filter][entityclass]") {
  // A custom entity LibreCAD does not model (HELIX) reaches rawDxfEntities and is
  // re-emitted verbatim; without a CLASS, AutoCAD/ODA prune it. ezdxf 1.4.4 audit
  // confirmed clean once the entity CLASS is emitted.
  ensureSettings();
  const std::string src = tmpFile("esrc.dxf");
  const std::string out = tmpFile("eout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  // A representative HELIX (subclass markers + a few body groups). Exact spline
  // content is irrelevant here — the raw net captures and re-emits it verbatim;
  // this fixture validates round-trip + CLASS emission, not ezdxf strict parsing
  // (ezdxf knows HELIX natively and would require full AcDbSpline data — that
  // needs a real-file sample, see commit notes).
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n10.0\n21\n10.0\n"
            "0\nHELIX\n5\n7A\n100\nAcDbEntity\n8\n0\n100\nAcDbSpline\n"
            "70\n0\n71\n3\n72\n0\n73\n0\n74\n0\n"
            "100\nAcDbHelix\n90\n29\n91\n63\n10\n0.0\n20\n0.0\n30\n0.0\n"
            "40\n1.0\n41\n1.0\n42\n1.0\n290\n1\n280\n1\n"
            "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    bool sawHelix = false;
    for (const DRW_RawDxfObject &e : graphic.dwgAdvancedMetadata().rawDxfEntities())
      if (e.name == "HELIX")
        sawHelix = true;
    CHECK(sawHelix);
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(countRecords(out, "HELIX") >= 1);                  // entity re-emitted
  CHECK(classRecordNames(out).count("HELIX") == 1);        // with a CLASS record

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  bool sawHelix2 = false;
  for (const DRW_RawDxfObject &e : graphic2.dwgAdvancedMetadata().rawDxfEntities())
    if (e.name == "HELIX")
      sawHelix2 = true;
  CHECK(sawHelix2);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// Regression for the raw-vs-fixed-structural handle collision (the bug that made
// 35/42 real AC1015+ exports unreadable by ezdxf). The source reuses the codec's
// OWN fixed structural handles for unrelated OBJECTS: a DICTIONARY at 0x10 (==
// codec LAYER "0"), an XRECORD at 0x14 (== codec LTYPE), an XRECORD at 0x1F (==
// codec *Model_Space BLOCK_RECORD), and a MATERIAL at 0x21 (== codec ENDBLK).
// These cannot be preserved verbatim, so the codec remaps them to fresh handles
// and rewrites every reference. Invariants: all code-5 unique; every 330/350
// reference resolves to an emitted handle; the fixed structural handles still
// equal their canonical literals; $HANDSEED is strictly above every body handle.
TEST_CASE("DXF export remaps raw objects colliding with fixed structural handles "
          "(referential integrity + unique handles + HANDSEED)",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("colsrc.dxf");
  const std::string out = tmpFile("colout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // Root dict C points at a material dict (0x10) and a group dict (0x12); the
  // material dict owns a MATERIAL (0x21), the group dict owns XRECORDs (0x14,
  // 0x1F). Every one of 0x10/0x12/0x14/0x1F/0x21 is a codec-fixed structural
  // handle, so all must be remapped (and the C entries + 330 owners rewritten).
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n3\nACAD_MATERIAL\n350\n10\n3\nMYGROUPS\n350\n12\n"
      "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "0\nDICTIONARY\n5\n10\n330\nC\n100\nAcDbDictionary\n281\n1\n3\nMyMat\n350\n21\n"
      "0\nMATERIAL\n5\n21\n330\n10\n100\nAcDbMaterial\n1\nMyMat\n94\n7\n"
      "0\nDICTIONARY\n5\n12\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nREC_A\n350\n14\n3\nREC_B\n350\n1F\n"
      "0\nXRECORD\n5\n14\n330\n12\n100\nAcDbXrecord\n280\n1\n1\nhello\n"
      "0\nXRECORD\n5\n1F\n330\n12\n100\nAcDbXrecord\n280\n1\n1\nworld\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // (a) every code-5 handle in the file is unique.
  const std::vector<std::string> handles = collectHandles(out);
  std::set<std::string> seen;
  for (const std::string &h : handles) {
    INFO("duplicate handle: " << h);
    CHECK(seen.insert(h).second);
  }

  // (b) fixed structural handles still equal their canonical literals (the codec
  // owns them; the colliding raw objects were moved off them).
  const std::set<std::string> handleSet(handles.begin(), handles.end());
  for (const char *fixed : {"C", "D", "10", "12", "14", "1F", "21"})
    CHECK(handleSet.count(fixed) == 1);  // emitted exactly once, by the codec

  // (c) referential integrity: the root dict's ACAD_MATERIAL / MYGROUPS targets
  // were remapped to fresh handles that ARE emitted (not the old 0x10/0x12).
  const std::map<std::string, std::string> rootEntries = rootDictEntries(out);
  REQUIRE(rootEntries.count("ACAD_MATERIAL") == 1);
  REQUIRE(rootEntries.count("MYGROUPS") == 1);
  CHECK(rootEntries.at("ACAD_MATERIAL") != "10");  // remapped off the collision
  CHECK(rootEntries.at("MYGROUPS") != "12");
  CHECK(handleSet.count(rootEntries.at("ACAD_MATERIAL")) == 1);  // resolves
  CHECK(handleSet.count(rootEntries.at("MYGROUPS")) == 1);

  // (d) $HANDSEED is strictly above every body handle.
  CHECK(handseedValue(out) > maxHandle(out));

  // (e) the data survives a full DXF->DXF re-import (MATERIAL + both XRECORDs).
  CHECK(countRecords(out, "MATERIAL") >= 1);
  CHECK(countRecords(out, "XRECORD") >= 2);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// A-4: the structural-collision handle remap must rewrite hard-pointer references
// carried in codes 390-399 and 480-481, not only 5/105/320-369/1005. A target raw
// object placed on a codec-fixed structural handle (0x14 == codec LTYPE
// CONTINUOUS) is remapped to a fresh handle; a second raw object referencing it
// via code 390 AND 480 must have those refs rewritten to the new handle, not left
// dangling at the reused literal.
TEST_CASE("DXF export rewrites 390/480 hard-pointer refs to remapped raw objects",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("ref390src.dxf");
  const std::string out = tmpFile("ref390out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n3\nMYDICT\n350\n12\n"
      "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "0\nDICTIONARY\n5\n12\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nTGT\n350\n14\n3\nREF\n350\n40\n"
      // target raw object collides with codec LTYPE 0x14 -> remapped to fresh handle
      "0\nMATERIAL\n5\n14\n330\n12\n100\nAcDbMaterial\n1\nTgtMat\n94\n7\n"
      // referencing raw object (no collision) points at 0x14 via 390 AND 480
      "0\nMATERIAL\n5\n40\n330\n12\n100\nAcDbMaterial\n1\nRefMat\n390\n14\n480\n14\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // Parse the output into ordered (code,value) pairs and pull out the two
  // MATERIAL records' fields (handle 5, name 1, refs 390/480).
  std::ifstream in(out);
  std::string codeLine, valueLine;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  std::string targetHandle, ref390, ref480;
  std::string curType, curHandle, curName, cur390, cur480;
  auto flush = [&]() {
    if (curType == "MATERIAL") {
      if (curName == "TgtMat")
        targetHandle = curHandle;
      if (curName == "RefMat") {
        ref390 = cur390;
        ref480 = cur480;
      }
    }
    curType = curHandle = curName = cur390 = cur480 = "";
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trim(codeLine), v = trim(valueLine);
    if (c == "0") { flush(); curType = v; }
    else if (c == "5") curHandle = v;
    else if (c == "1") curName = v;
    else if (c == "390") cur390 = v;
    else if (c == "480") cur480 = v;
  }
  flush();

  // The target was remapped off the colliding literal 0x14...
  REQUIRE_FALSE(targetHandle.empty());
  CHECK(targetHandle != "14");
  // ...and the referencing object's 390 + 480 were rewritten to the new handle
  // (LOAD-BEARING: both are "14" before the fix -> dangling).
  REQUIRE_FALSE(ref390.empty());
  REQUIRE_FALSE(ref480.empty());
  CHECK(ref390 == targetHandle);
  CHECK(ref480 == targetHandle);
  CHECK(ref390 != "14");
  CHECK(ref480 != "14");

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// D-2(c): a raw object's captured owner (330) must be the handle OUTSIDE any
// 102 {ACAD_REACTORS} control group, not the last 330 seen. Here the owner 330=C
// is at depth 0 (first) and a reactor 330=D is at depth 1 (last); the prior
// last-wins latch took D (the reactor) as the owner.
TEST_CASE("DXF raw object owner 330 ignores reactor-group handles",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("reactor330.dxf");
  std::filesystem::remove(src);

  // MATERIAL routes to the raw net; owner C precedes a reactor group whose 330
  // (D) is the LAST 330 in the record.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n3\nMYMAT\n350\n90\n"
      "0\nMATERIAL\n5\n90\n330\nC\n102\n{ACAD_REACTORS\n330\nD\n102\n}\n"
      "100\nAcDbMaterial\n1\nRM\n94\n7\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
  }
  std::filesystem::remove(src);

  const LC_DwgAdvancedMetadata &meta = graphic.dwgAdvancedMetadata();
  const DRW_RawDxfObject *mat = nullptr;
  for (const DRW_RawDxfObject &o : meta.rawDxfObjects())
    if (o.name == "MATERIAL" && o.handle == 0x90u)
      mat = &o;
  REQUIRE(mat != nullptr);
  // Owner is C (0xC), NOT the reactor D (0xD).
  CHECK(mat->parentHandle == 0xCu);
}

TEST_CASE("DXF DETAILVIEWSTYLE/SECTIONVIEWSTYLE round-trip (typed-read OBJECT "
          "preserved via raw net + CLASS, owned xdict resolves)",
          "[dxf][roundtrip][filter][viewstyle]") {
  ensureSettings();
  const std::string src = tmpFile("vsrc.dxf");
  const std::string out = tmpFile("vout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  // Root dict + ACAD_DETAILVIEWSTYLE dict -> a DETAILVIEWSTYLE object that owns
  // an extension DICTIONARY (the dangling-owner case this fix closes). Before the
  // fix the view style was dropped on DXF write and the xdict's 330 dangled.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "3\nACAD_DETAILVIEWSTYLE\n350\n50\n"
            "0\nDICTIONARY\n5\n50\n330\nC\n100\nAcDbDictionary\n281\n1\n3\nMyDVS\n350\n51\n"
            "0\nACDBDETAILVIEWSTYLE\n5\n51\n102\n{ACAD_XDICTIONARY\n360\n52\n102\n}\n"
            "330\n50\n100\nAcDbModelDocViewStyle\n70\n0\n100\nAcDbDetailViewStyle\n"
            "70\n1\n300\nDetail\n"
            "0\nDICTIONARY\n5\n52\n330\n51\n100\nAcDbDictionary\n281\n1\n"
            "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  // The view style survives DXF->DXF (was dropped) ...
  CHECK(countRecords(out, "ACDBDETAILVIEWSTYLE") >= 1);
  // ... carries a CLASS record (custom class) ...
  CHECK(classRecordNames(out).count("ACDBDETAILVIEWSTYLE") == 1);
  // ... and its extension dictionary's 330 owner (the view-style handle) is now
  // emitted, so the owner resolves (no dangling INVALID_OWNER_HANDLE).
  // collectHandles() must be called once into a named vector: building the set
  // straight from collectHandles(out).begin()/collectHandles(out).end() takes
  // begin()/end() from two *different* temporaries, an invalid iterator range
  // (UB) that crashes on some platforms.
  const std::vector<std::string> handleList = collectHandles(out);
  const std::set<std::string> handles(handleList.begin(), handleList.end());
  CHECK(handles.count("51") == 1);  // the view style
  CHECK(handles.count("52") == 1);  // its owned xdict

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF MESH round-trips losslessly via the raw net (real geometry)",
          "[dxf][roundtrip][filter][mesh]") {
  // MESH (AcDbSubDMesh) is now MODELED: LibreCAD decodes it to a DRW_Mesh and
  // renders the base-cage faces as closed polylines (read-feature-coverage:
  // MESH is the one genuinely-absent renderable entity). It is therefore no
  // longer raw-passthrough-preserved; the 8-vertex / 6-face cube imports as 6
  // closed face polylines. (There is no MESH *writer* yet — read-only scope —
  // so it round-trips as its rendered polylines, not as a MESH entity.)
  ensureSettings();
  const std::string src = tmpFile("msrc.dxf");
  const std::string out = tmpFile("mout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  const std::string mesh =
      "0\nMESH\n5\n2F\n330\nC\n100\nAcDbEntity\n8\n0\n100\nAcDbSubDMesh\n71\n2\n72\n0\n91\n0\n"
      "92\n8\n10\n0.0\n20\n0.0\n30\n0.0\n10\n1.0\n20\n0.0\n30\n0.0\n10\n1.0\n20\n1.0\n30\n0.0\n"
      "10\n0.0\n20\n1.0\n30\n0.0\n10\n0.0\n20\n0.0\n30\n1.0\n10\n1.0\n20\n0.0\n30\n1.0\n"
      "10\n1.0\n20\n1.0\n30\n1.0\n10\n0.0\n20\n1.0\n30\n1.0\n"
      "93\n30\n90\n4\n90\n0\n90\n1\n90\n2\n90\n3\n90\n4\n90\n4\n90\n5\n90\n6\n90\n7\n"
      "90\n4\n90\n0\n90\n1\n90\n5\n90\n4\n90\n4\n90\n1\n90\n2\n90\n6\n90\n5\n"
      "90\n4\n90\n2\n90\n3\n90\n7\n90\n6\n90\n4\n90\n3\n90\n0\n90\n4\n90\n7\n"
      "94\n0\n95\n0\n90\n0\n";
  writeText(src, "0\nSECTION\n2\nENTITIES\n0\nLINE\n8\n0\n10\n0\n20\n0\n11\n1\n21\n1\n" +
                     mesh + "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
  }
  auto meshVertexCount = [](const LC_DwgAdvancedMetadata &meta) -> int {
    for (const DRW_RawDxfObject &e : meta.rawDxfEntities())
      if (e.name == "MESH")
        for (const DRW_Variant &g : e.groups)
          if (g.code() == 92)
            return g.i_val();  // captured typed (the numeric-capture fix)
    return -1;
  };
  // Now modeled, not raw-netted: the MESH is decoded to a DRW_Mesh and rendered,
  // so it no longer appears in the raw-passthrough metadata.
  CHECK(meshVertexCount(graphic.dwgAdvancedMetadata()) == -1);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  }
  // The 6 cube faces render as closed polylines; there is no MESH writer, so the
  // entity is not re-emitted as MESH.
  CHECK(countRecords(out, "MESH") == 0);
  CHECK(countRecords(out, "LWPOLYLINE") + countRecords(out, "POLYLINE") >= 6);
  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// Regression for the i18n layer-name fix: a layer name is an identifier, not
// MTEXT content, so it must NOT be run through toNativeString (which would
// caret-decode "^I" -> TAB). Before the fix, setEntityAttributes decoded the
// entity's layer name to "A<TAB>B" while addLayer stored the raw "A^IB",
// orphaning the entity from its own layer record.
TEST_CASE("DXF import preserves a caret-bearing layer name verbatim",
          "[dxf][roundtrip][filter][i18n]") {
  ensureSettings();
  const std::string src = tmpFile("caretlayer.dxf");
  std::filesystem::remove(src);

  // A LINE whose layer (group 8) is the literal identifier "A^IB". The layer
  // is auto-created by setEntityAttributes (no LAYER table entry needed).
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\nA^IB\n10\n0.0\n20\n0.0\n30\n0.0\n11\n10.0\n21\n10.0\n31\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  // The layer must exist under the verbatim name, and NOT under the
  // caret-decoded form.
  CHECK(graphic.findLayer(QStringLiteral("A^IB")) != nullptr);
  CHECK(graphic.findLayer(QStringLiteral("A\tB")) == nullptr);

  // The imported entity must resolve to that same layer (not be orphaned).
  RS_Entity *first = graphic.firstEntity();
  REQUIRE(first != nullptr);
  RS_Layer *layer = first->getLayer();
  REQUIRE(layer != nullptr);
  CHECK(layer->getName() == QStringLiteral("A^IB"));

  std::filesystem::remove(src);
}
