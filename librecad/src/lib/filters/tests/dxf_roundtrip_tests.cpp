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
  // The preserved raw handles survive verbatim (reserve, not remap).
  CHECK(std::count(handles.begin(), handles.end(), std::string("33")) == 1);
  CHECK(std::count(handles.begin(), handles.end(), std::string("34")) == 1);

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
  const std::map<std::string, std::string> rootEntries = rootDictEntries(out);
  REQUIRE(rootEntries.count("ACAD_SCALELIST") == 1);
  REQUIRE(rootEntries.count("ACAD_MATERIAL") == 1);
  CHECK(rootEntries.at("ACAD_SCALELIST") == "50");
  CHECK(rootEntries.at("ACAD_MATERIAL") == "60");
  CHECK(rootEntries.count("ACAD_GROUP") == 1);  // codec's own entry -> D
  CHECK(rootEntries.at("ACAD_GROUP") == "D");

  // (c) referential integrity: every root 350 target + child-dict entry target
  // is actually emitted (no dangling refs an auditor would prune).
  const std::set<std::string> handleSet(handles.begin(), handles.end());
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
