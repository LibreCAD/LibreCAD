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
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <tuple>
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

std::vector<std::string> recordGroupValues(const std::string &path,
                                           const std::string &recordName,
                                           const std::string &code) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::vector<std::string> values;
  bool inRecord = false;
  auto trim = [](std::string value) {
    if (!value.empty() && value.back() == '\r')
      value.pop_back();
    const size_t first = value.find_first_not_of(" \t");
    return first == std::string::npos ? std::string() : value.substr(first);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string groupCode = trim(codeLine);
    const std::string value = trim(valueLine);
    if (groupCode == "0")
      inRecord = value == recordName;
    else if (inRecord && groupCode == code)
      values.push_back(value);
  }
  return values;
}

// Reads the first value of a group as a double, requiring that the group is
// actually present. recordGroupValues() legitimately returns an empty vector
// when the record carries no such group, and calling .front() on that is
// undefined behaviour — on Linux a segmentation fault that aborts the whole
// suite instead of failing one assertion. Checking that a record exists does
// not imply any particular group inside it exists.
double firstGroupValueAsDouble(const std::string &path,
                               const std::string &recordName,
                               const std::string &code) {
  const std::vector<std::string> values = recordGroupValues(path, recordName, code);
  REQUIRE_FALSE(values.empty());
  return std::stod(values.front());
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

// Returns the code-290 (plot flag) value of the LAYER record named `layerName`,
// or "" if absent. In our writer code 2 (name) precedes code 290 within a
// LAYER record, so match the name first then capture the next 290.
std::string layerPlotFlag(const std::string &path, const std::string &layerName) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  bool inLayer = false, nameMatched = false;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trim(codeLine), v = trim(valueLine);
    if (c == "0")
      { inLayer = (v == "LAYER"); nameMatched = false; }
    else if (inLayer && c == "2")
      nameMatched = (v == layerName);
    else if (inLayer && nameMatched && c == "290")
      return v;
  }
  return std::string();
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

TEST_CASE("DXF filter normalizes reflected LWPOLYLINE extrusion once",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("lwpolyline_reflected_src.dxf");
  const std::string out = tmpFile("lwpolyline_reflected_out.dxf");
  const std::string out2 = tmpFile("lwpolyline_reflected_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLWPOLYLINE\n8\n0\n90\n2\n70\n0\n38\n7.0\n"
            "10\n1.0\n20\n2.0\n42\n0.5\n"
            "10\n3.0\n20\n4.0\n42\n-0.5\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  const auto firstX = recordGroupValues(out, "LWPOLYLINE", "10");
  const auto firstElevation = recordGroupValues(out, "LWPOLYLINE", "38");
  const auto firstBulges = recordGroupValues(out, "LWPOLYLINE", "42");
  REQUIRE(firstX.size() == 2);
  REQUIRE(firstElevation.size() == 1);
  REQUIRE(firstBulges.size() == 2);
  CHECK(std::stod(firstX[0]) == -1.0);
  CHECK(std::stod(firstElevation[0]) == -7.0);
  CHECK(std::stod(firstBulges[0]) < -0.499);
  CHECK(std::stod(firstBulges[0]) > -0.501);
  CHECK(recordGroupValues(out, "LWPOLYLINE", "210").empty());

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  const auto secondX = recordGroupValues(out2, "LWPOLYLINE", "10");
  const auto secondElevation = recordGroupValues(out2, "LWPOLYLINE", "38");
  REQUIRE(secondX.size() == 2);
  REQUIRE(secondElevation.size() == 1);
  CHECK(std::stod(secondX[0]) == -1.0);
  CHECK(std::stod(secondElevation[0]) == -7.0);
  CHECK(recordGroupValues(out2, "LWPOLYLINE", "210").empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter normalizes reflected legacy POLYLINE extrusion once",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("polyline_reflected_src.dxf");
  const std::string out = tmpFile("polyline_reflected_out.dxf");
  const std::string out2 = tmpFile("polyline_reflected_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nPOLYLINE\n8\n0\n66\n1\n10\n0.0\n20\n0.0\n30\n7.0\n"
            "39\n0.25\n40\n0.1\n41\n0.2\n70\n0\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nVERTEX\n8\n0\n10\n1.0\n20\n2.0\n42\n0.5\n"
            "0\nVERTEX\n8\n0\n10\n3.0\n20\n4.0\n42\n-0.5\n"
            "0\nSEQEND\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  REQUIRE(countRecords(out, "POLYLINE") == 0);
  REQUIRE(countRecords(out, "LWPOLYLINE") == 1);
  const auto firstX = recordGroupValues(out, "LWPOLYLINE", "10");
  const auto firstElevation = recordGroupValues(out, "LWPOLYLINE", "38");
  const auto firstThickness = recordGroupValues(out, "LWPOLYLINE", "39");
  const auto firstBulges = recordGroupValues(out, "LWPOLYLINE", "42");
  const auto firstStartWidths = recordGroupValues(out, "LWPOLYLINE", "40");
  const auto firstEndWidths = recordGroupValues(out, "LWPOLYLINE", "41");
  REQUIRE(firstX.size() == 2);
  REQUIRE(firstElevation.size() == 1);
  REQUIRE(firstThickness.size() == 1);
  REQUIRE(firstBulges.size() == 2);
  REQUIRE(firstStartWidths.size() == 2);
  REQUIRE(firstEndWidths.size() == 2);
  CHECK(std::stod(firstX[0]) == -1.0);
  CHECK(std::stod(firstElevation[0]) == -7.0);
  CHECK(std::stod(firstThickness[0]) == -0.25);
  CHECK(std::abs(std::stod(firstBulges[0]) + 0.5) < 1.0e-12);
  CHECK(std::stod(firstStartWidths[0]) == 0.1);
  CHECK(std::stod(firstEndWidths[0]) == 0.2);
  CHECK(recordGroupValues(out, "LWPOLYLINE", "210").empty());

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "38", "39", "40", "41", "42"})
    CHECK(recordGroupValues(out2, "LWPOLYLINE", code)
          == recordGroupValues(out, "LWPOLYLINE", code));
  CHECK(recordGroupValues(out2, "LWPOLYLINE", "210").empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter normalizes reflected planar curves once",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("planar_curves_reflected_src.dxf");
  const std::string out = tmpFile("planar_curves_reflected_out.dxf");
  const std::string out2 = tmpFile("planar_curves_reflected_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nCIRCLE\n8\n0\n10\n10.0\n20\n20.0\n40\n3.0\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nARC\n8\n0\n10\n20.0\n20\n30.0\n40\n4.0\n50\n15.0\n51\n120.0\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nELLIPSE\n8\n0\n10\n30.0\n20\n40.0\n"
            "11\n5.0\n21\n1.0\n40\n0.3\n41\n0.25\n42\n1.75\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  REQUIRE(countRecords(out, "CIRCLE") == 1);
  REQUIRE(countRecords(out, "ARC") == 1);
  REQUIRE(countRecords(out, "ELLIPSE") == 1);
  CHECK(firstGroupValueAsDouble(out, "CIRCLE", "10") == -10.0);
  CHECK(firstGroupValueAsDouble(out, "ARC", "10") == -20.0);
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "10") == -30.0);
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "11") == -5.0);
  for (const char *record : {"CIRCLE", "ARC", "ELLIPSE"})
    CHECK(recordGroupValues(out, record, "210").empty());

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }

  // The first export is normalized WCS, so reading it again must not reflect
  // centers, axes, or curve parameters a second time.
  for (const auto &[record, codes] : std::vector<
           std::pair<const char *, std::vector<const char *>>>{
           {"CIRCLE", {"10", "20", "40"}},
           {"ARC", {"10", "20", "40", "50", "51"}},
           {"ELLIPSE", {"10", "20", "11", "21", "40", "41", "42"}}}) {
    for (const char *code : codes)
      CHECK(recordGroupValues(out2, record, code)
            == recordGroupValues(out, record, code));
    CHECK(recordGroupValues(out2, record, "210").empty());
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves WCS POINT and LINE extrusion fields",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("wcs_point_line_extrusion_src.dxf");
  const std::string out = tmpFile("wcs_point_line_extrusion_out.dxf");
  const std::string out2 = tmpFile("wcs_point_line_extrusion_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  // POINT and LINE coordinates are WCS. Their extrusion vectors govern
  // thickness, so the filter must retain them rather than OCS-transforming
  // coordinates or silently exporting the default normal.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nPOINT\n8\n0\n10\n1.0\n20\n2.0\n30\n3.0\n39\n0.5\n50\n30.0\n"
            "210\n0.2\n220\n0.3\n230\n0.9327379053088815\n"
            "0\nLINE\n8\n0\n10\n4.0\n20\n5.0\n30\n6.0\n"
            "11\n7.0\n21\n8.0\n31\n9.0\n39\n2.5\n"
            "210\n0.0\n220\n1.0\n230\n0.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  for (const auto &[record, values] : std::vector<
           std::pair<const char *, std::vector<std::pair<const char *, const char *>>>>{
           {"POINT", {{"30", "3"}, {"39", "0.5"}, {"50", "30"}, {"210", "0.2"},
                      {"220", "0.3"}, {"230", "0.9327379053088815"}}},
           {"LINE", {{"30", "6"}, {"31", "9"}, {"39", "2.5"},
                     {"210", "0"}, {"220", "1"}, {"230", "0"}}}}) {
    for (const auto &[code, expected] : values) {
      const auto actual = recordGroupValues(out, record, code);
      REQUIRE(actual.size() == 1);
      CHECK(std::stod(actual.front()) == std::stod(expected));
    }
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const auto &[record, codes] : std::vector<
           std::pair<const char *, std::vector<const char *>>>{
           {"POINT", {"10", "20", "30", "39", "50", "210", "220", "230"}},
           {"LINE", {"10", "20", "30", "11", "21", "31", "39", "210", "220", "230"}}}) {
    for (const char *code : codes)
      CHECK(recordGroupValues(out2, record, code)
            == recordGroupValues(out, record, code));
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves raw HATCH OCS elevation and extrusion",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("hatch_ocs_extrusion_src.dxf");
  const std::string out = tmpFile("hatch_ocs_extrusion_out.dxf");
  const std::string out2 = tmpFile("hatch_ocs_extrusion_out2.dxf");
  const std::string dwg = tmpFile("hatch_ocs_extrusion.dwg");
  const std::string dwgOut = tmpFile("hatch_ocs_extrusion_from_dwg.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);

  // libdxfrw keeps HATCH boundaries in OCS. The filter must therefore retain
  // the elevation and normal together with those unchanged boundary values.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nHATCH\n8\n0\n100\nAcDbHatch\n"
            "10\n0.0\n20\n0.0\n30\n7.0\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "2\nSOLID\n70\n1\n71\n0\n91\n1\n"
            "92\n2\n72\n0\n73\n1\n93\n4\n"
            "10\n1.0\n20\n2.0\n10\n3.0\n20\n2.0\n"
            "10\n3.0\n20\n4.0\n10\n1.0\n20\n4.0\n97\n0\n"
            "75\n0\n76\n1\n98\n0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  REQUIRE(countRecords(out, "HATCH") == 1);
  for (const auto &[code, expected] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"30", "7"}, {"210", "0"}, {"220", "0"}, {"230", "-1"}}) {
    const auto values = recordGroupValues(out, "HATCH", code);
    REQUIRE(values.size() == 1);
    CHECK(std::stod(values.front()) == std::stod(expected));
  }
  CHECK(recordGroupValues(out, "HATCH", "10")
        == std::vector<std::string>{"0", "1", "3", "3", "1"});

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "30", "210", "220", "230"})
    CHECK(recordGroupValues(out2, "HATCH", code)
          == recordGroupValues(out, "HATCH", code));

#ifdef DWGSUPPORT
  // Drive the same raw-sidecar boundary through an R2004 DWG write/read. The
  // source graphic originated as DXF, avoiding unrelated raw-DWG object replay
  // while still exercising the native DWG HATCH encoder and decoder.
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwg),
                              RS2::FormatDWG2004));
  }
  RS_Graphic fromDwg;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(fromDwg, QString::fromStdString(dwg),
                              RS2::FormatDWG));
    REQUIRE(filter.fileExport(fromDwg, QString::fromStdString(dwgOut),
                              RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "30", "210", "220", "230"})
    CHECK(recordGroupValues(dwgOut, "HATCH", code)
          == recordGroupValues(out, "HATCH", code));
#endif

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF filter preserves IMAGE WCS frame Z coordinates",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("image_wcs_frame_src.dxf");
  const std::string out = tmpFile("image_wcs_frame_out.dxf");
  const std::string out2 = tmpFile("image_wcs_frame_out2.dxf");
  const std::string dwg = tmpFile("image_wcs_frame.dwg");
  const std::string dwgOut = tmpFile("image_wcs_frame_from_dwg.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);

  // IMAGE's insertion point and per-pixel U/V vectors are WCS fields, not
  // OCS coordinates. LibreCAD edits their XY frame only, so retain Z values
  // independently rather than relabeling the full source frame as planar.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nIMAGE\n8\n0\n100\nAcDbRasterImage\n90\n0\n"
            "10\n1.0\n20\n2.0\n30\n3.0\n"
            "11\n0.5\n21\n0.0\n31\n4.0\n"
            "12\n0.0\n22\n0.25\n32\n5.0\n"
            "13\n16.0\n23\n12.0\n340\n1\n70\n3\n"
            "280\n1\n281\n50\n282\n50\n283\n0\n"
            "71\n1\n91\n2\n14\n-0.5\n24\n-0.5\n"
            "14\n15.5\n24\n11.5\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  REQUIRE(countRecords(out, "IMAGE") == 1);
  const auto xdataApps = recordGroupValues(out, "IMAGE", "1001");
  CHECK(std::find(xdataApps.begin(), xdataApps.end(), "LibreCAD_IMAGE_FRAME")
        == xdataApps.end());
  for (const auto &[code, expected] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"10", "1"}, {"20", "2"}, {"30", "3"},
           {"11", "0.5"}, {"21", "0"}, {"31", "4"},
           {"12", "0"}, {"22", "0.25"}, {"32", "5"}}) {
    const auto values = recordGroupValues(out, "IMAGE", code);
    REQUIRE(values.size() == 1);
    CHECK(std::stod(values.front()) == std::stod(expected));
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "30", "11", "21", "31", "12", "22", "32"})
    CHECK(recordGroupValues(out2, "IMAGE", code)
          == recordGroupValues(out, "IMAGE", code));

#ifdef DWGSUPPORT
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwg),
                              RS2::FormatDWG2004));
  }
  RS_Graphic fromDwg;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(fromDwg, QString::fromStdString(dwg),
                              RS2::FormatDWG));
    REQUIRE(filter.fileExport(fromDwg, QString::fromStdString(dwgOut),
                              RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "30", "11", "21", "31", "12", "22", "32"})
    CHECK(recordGroupValues(dwgOut, "IMAGE", code)
          == recordGroupValues(out, "IMAGE", code));
#endif

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF filter preserves raw TEXT OCS fields",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("text_ocs_src.dxf");
  const std::string out = tmpFile("text_ocs_out.dxf");
  const std::string out2 = tmpFile("text_ocs_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nTEXT\n8\n0\n10\n1.0\n20\n2.0\n30\n7.0\n"
            "11\n4.0\n21\n5.0\n31\n7.0\n40\n2.0\n1\nOCS\n"
            "50\n15.0\n7\nSTANDARD\n39\n0.5\n72\n5\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  }
  for (const auto &[code, expected] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"30", "7"}, {"31", "7"}, {"39", "0.5"},
           {"210", "0"}, {"220", "0"}, {"230", "-1"}}) {
    const auto values = recordGroupValues(out, "TEXT", code);
    REQUIRE(values.size() == 1);
    CHECK(std::stod(values.front()) == std::stod(expected));
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2), RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "30", "11", "21", "31", "39", "210", "220", "230"})
    CHECK(recordGroupValues(out2, "TEXT", code)
          == recordGroupValues(out, "TEXT", code));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves MTEXT OCS frame and x-axis vector",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("mtext_ocs_src.dxf");
  const std::string out = tmpFile("mtext_ocs_out.dxf");
  const std::string out2 = tmpFile("mtext_ocs_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nMTEXT\n8\n0\n10\n1.0\n20\n2.0\n30\n7.0\n"
            "40\n2.0\n41\n10.0\n71\n1\n72\n1\n1\nOCS MTEXT\n"
            "7\nSTANDARD\n210\n0.0\n220\n0.0\n230\n-1.0\n"
            "11\n1.0\n21\n0.0\n31\n3.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  }
  for (const auto &[code, expected] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"30", "7"}, {"11", "1"}, {"21", "0"}, {"31", "3"},
           {"210", "0"}, {"220", "0"}, {"230", "-1"}}) {
    const auto values = recordGroupValues(out, "MTEXT", code);
    REQUIRE(values.size() == 1);
    CHECK(std::stod(values.front()) == std::stod(expected));
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2), RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "30", "11", "21", "31", "210", "220", "230"})
    CHECK(recordGroupValues(out2, "MTEXT", code)
          == recordGroupValues(out, "MTEXT", code));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves reflected MINSERT source fields with a block definition",
          "[dxf][roundtrip][filter][extrusion][insert]") {
  ensureSettings();
  const std::string src = tmpFile("minsert_source_fields_src.dxf");
  const std::string out = tmpFile("minsert_source_fields_out.dxf");
  const std::string out2 = tmpFile("minsert_source_fields_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  // A real definition makes the filter expand derived children, while export
  // must still serialize the original MINSERT source record rather than those
  // children. The nonzero block base point also exercises the source equation.
  writeText(src,
            "0\nSECTION\n2\nBLOCKS\n"
            "0\nBLOCK\n8\n0\n2\nGRID_SYMBOL\n70\n0\n"
            "10\n1.0\n20\n2.0\n30\n0.0\n3\nGRID_SYMBOL\n1\n\n"
            "0\nLINE\n8\n0\n10\n1.0\n20\n2.0\n11\n3.0\n21\n2.0\n"
            "0\nENDBLK\n8\n0\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nINSERT\n8\n0\n2\nGRID_SYMBOL\n"
            "10\n10.0\n20\n20.0\n30\n30.0\n"
            "41\n2.0\n42\n-3.0\n43\n4.0\n50\n30.0\n"
            "70\n3\n71\n2\n44\n5.0\n45\n6.0\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  REQUIRE(countRecords(out, "INSERT") == 1);
  for (const auto &[code, expected] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"2", "GRID_SYMBOL"}, {"10", "10"}, {"20", "20"},
           {"30", "30"}, {"41", "2"}, {"42", "-3"}, {"43", "4"},
           {"50", "30"}, {"70", "3"}, {"71", "2"}, {"44", "5"},
           {"45", "6"}, {"210", "0"}, {"220", "0"}, {"230", "-1"}}) {
    const auto actual = recordGroupValues(out, "INSERT", code);
    REQUIRE(actual.size() == 1);
    if (std::string(code) == "2")
      CHECK(actual.front() == expected);
    else
      CHECK(std::stod(actual.front()) == std::stod(expected));
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const char *code : {"2", "10", "20", "30", "41", "42", "43",
                           "50", "70", "71", "44", "45", "210", "220", "230"}) {
    CHECK(recordGroupValues(out2, "INSERT", code)
          == recordGroupValues(out, "INSERT", code));
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter normalizes TRACE and SOLID extrusion once",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("trace_solid_reflected_src.dxf");
  const std::string out = tmpFile("trace_solid_reflected_out.dxf");
  const std::string out2 = tmpFile("trace_solid_reflected_out2.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nTRACE\n8\n0\n10\n1.0\n20\n2.0\n30\n3.0\n"
            "11\n4.0\n21\n5.0\n31\n6.0\n12\n7.0\n22\n8.0\n32\n9.0\n"
            "13\n10.0\n23\n11.0\n33\n12.0\n39\n0.5\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nSOLID\n8\n0\n10\n13.0\n20\n14.0\n30\n15.0\n"
            "11\n16.0\n21\n17.0\n31\n18.0\n12\n19.0\n22\n20.0\n32\n21.0\n"
            "13\n22.0\n23\n23.0\n33\n24.0\n39\n1.5\n"
            "210\n0.0\n220\n0.0\n230\n-1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  for (const auto &[record, expectedX, expectedThickness] :
       {std::tuple{"TRACE", -1.0, -0.5}, std::tuple{"SOLID", -13.0, -1.5}}) {
    REQUIRE(countRecords(out, record) == 1);
    CHECK(firstGroupValueAsDouble(out, record, "10") == expectedX);
    CHECK(firstGroupValueAsDouble(out, record, "39") == expectedThickness);
    CHECK(recordGroupValues(out, record, "210").empty());
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const auto &[record, codes] : std::vector<
           std::pair<const char *, std::vector<const char *>>>{
           {"TRACE", {"10", "20", "30", "11", "21", "31", "12", "22", "32", "13", "23", "33", "39"}},
           {"SOLID", {"10", "20", "30", "11", "21", "31", "12", "22", "32", "13", "23", "33", "39"}}}) {
    for (const char *code : codes)
      CHECK(recordGroupValues(out2, record, code)
            == recordGroupValues(out, record, code));
    CHECK(recordGroupValues(out2, record, "210").empty());
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter rejects tilted planar entities before 2D creation",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("lwpolyline_tilted_src.dxf");
  const std::string out = tmpFile("lwpolyline_tilted_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
            "0\nLWPOLYLINE\n8\n0\n90\n2\n"
            "10\n1.0\n20\n2.0\n10\n3.0\n20\n4.0\n"
            "210\n0.0\n220\n1.0\n230\n0.0\n"
            "0\nCIRCLE\n8\n0\n10\n5.0\n20\n5.0\n40\n2.0\n"
            "210\n0.0\n220\n1.0\n230\n0.0\n"
            "0\nARC\n8\n0\n10\n10.0\n20\n5.0\n40\n2.0\n50\n0.0\n51\n90.0\n"
            "210\n0.0\n220\n1.0\n230\n0.0\n"
            "0\nELLIPSE\n8\n0\n10\n15.0\n20\n5.0\n"
            "11\n3.0\n21\n0.0\n40\n0.5\n41\n0.0\n42\n6.283185307179586\n"
            "210\n0.0\n220\n1.0\n230\n0.0\n"
            "0\nPOLYLINE\n8\n0\n66\n1\n70\n0\n"
            "210\n0.0\n220\n1.0\n230\n0.0\n"
            "0\nVERTEX\n8\n0\n10\n20.0\n20\n5.0\n"
            "0\nVERTEX\n8\n0\n10\n22.0\n20\n5.0\n"
            "0\nSEQEND\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "LINE") == 1);
  CHECK(countRecords(out, "LWPOLYLINE") == 0);
  CHECK(countRecords(out, "POLYLINE") == 0);
  CHECK(countRecords(out, "CIRCLE") == 0);
  CHECK(countRecords(out, "ARC") == 0);
  CHECK(countRecords(out, "ELLIPSE") == 0);

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

  in.close();
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

  in.close();
  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// D4 write-path: an OLE2FRAME read from a DWG lands only on the metadata shelf;
// the export now re-emits it as a typed AcDbOle2Frame with its frame rectangle
// (10/11) and the opaque OLE payload replayed verbatim (group 310).
TEST_CASE("DXF export re-emits DWG-read OLE2FRAME entities", "[dxf][roundtrip][filter][ole]") {
  ensureSettings();
  const std::string src = tmpFile("olesrc.dxf");
  const std::string out = tmpFile("ole.dxf");
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

  const std::size_t payloadLen = 300;  // > 254 so the 310 stream is multi-chunk
  {
    DRW_Ole2Frame ole;
    ole.handle = 0x300;
    ole.parentHandle = 0x1F;
    ole.m_flags = 2;   // embedded
    ole.m_mode = 0;
    ole.m_pt1.x = 1.0; ole.m_pt1.y = 6.0;  // upper-left
    ole.m_pt2.x = 5.0; ole.m_pt2.y = 2.0;  // lower-right
    ole.m_payloadBytes.resize(payloadLen);
    for (std::size_t i = 0; i < payloadLen; ++i)
      ole.m_payloadBytes[i] = static_cast<std::uint8_t>(i & 0xFF);
    graphic.dwgAdvancedMetadata().addOle2Frame(ole);
  }

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "OLE2FRAME") == 1);
  CHECK(recordHasCode(out, "OLE2FRAME", "100")); // AcDbOle2Frame subclass marker
  CHECK(recordHasCode(out, "OLE2FRAME", "10"));  // upper-left
  CHECK(recordHasCode(out, "OLE2FRAME", "11"));  // lower-right
  CHECK(recordHasCode(out, "OLE2FRAME", "90"));  // payload length
  CHECK(recordHasCode(out, "OLE2FRAME", "310")); // binary payload

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
    const DRW_RawDxfObject *sunObject = findRaw(meta, "SUN");
    REQUIRE(sunObject != nullptr);
    const DRW_Variant *intensity = group(*sunObject, 40);
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
    const DRW_RawDxfObject *sunObject = findRaw(meta, "SUN");
    REQUIRE(sunObject != nullptr);
    const DRW_Variant *intensity = group(*sunObject, 40);
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

TEST_CASE("DXF unmodeled source custom ENTITY round-trips with a CLASS record",
          "[dxf][roundtrip][filter][entityclass]") {
  // A source-defined custom entity LibreCAD does not model reaches
  // rawDxfEntities and is re-emitted verbatim with its CLASS metadata.
  ensureSettings();
  const std::string src = tmpFile("esrc.dxf");
  const std::string out = tmpFile("eout.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  // The raw net captures and re-emits the unknown body verbatim; this fixture
  // validates round-trip plus source CLASS preservation.
  writeText(src,
            "0\nSECTION\n2\nCLASSES\n"
            "0\nCLASS\n1\nWEIRDENT\n2\nAcDbWeirdEntity\n3\nCUSTOM_APP\n"
            "90\n4095\n91\n1\n280\n0\n281\n1\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n10.0\n21\n10.0\n"
            "0\nWEIRDENT\n5\n7A\n100\nAcDbEntity\n8\n0\n"
            "100\nAcDbWeirdEntity\n10\n0.0\n20\n0.0\n30\n0.0\n"
            "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    bool sawWeirdEnt = false;
    for (const DRW_RawDxfObject &e : graphic.dwgAdvancedMetadata().rawDxfEntities())
      if (e.name == "WEIRDENT")
        sawWeirdEnt = true;
    CHECK(sawWeirdEnt);
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(countRecords(out, "WEIRDENT") >= 1);             // entity re-emitted
  CHECK(classRecordNames(out).count("WEIRDENT") == 1);   // with a CLASS record

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  bool sawWeirdEnt2 = false;
  for (const DRW_RawDxfObject &e : graphic2.dwgAdvancedMetadata().rawDxfEntities())
    if (e.name == "WEIRDENT")
      sawWeirdEnt2 = true;
  CHECK(sawWeirdEnt2);

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

  in.close();
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

// Audit follow-up to the DWG layer-0 plot-flag fix: the DXF LAYER plot flag
// (code 290) used to be emitted ONLY when plotF was false, relying on
// "absent => true". That was inconsistent with the always-emitted lineweight
// (370)/plotstyle (390), non-conformant with AutoCAD/ezdxf (which always write
// every R2000+ LAYER field), and dropped an explicit "290 1" written by a
// strict external tool on re-save. It is now emitted unconditionally. Confirm a
// plot-on layer emits 290=1 (the discriminating case: the old code emitted NO
// 290 for it), a plot-off layer emits 290=0, and both round-trip.
TEST_CASE("DXF export always emits the layer plot flag (290)",
          "[dxf][roundtrip][filter][layer-plotflag]") {
  ensureSettings();
  const std::string src = tmpFile("plotflagsrc.dxf");
  const std::string out = tmpFile("plotflag.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // Minimal valid DXF (one LINE on "0") to set the graphic up like a real
  // import, which creates the standard "0" layer (plot-on by default).
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

  // Add a non-default plot-off layer alongside the plot-on "0".
  {
    auto *noplot = new RS_Layer(QStringLiteral("NOPLOT"));
    noplot->setPrint(false);
    graphic.addLayer(noplot);
  }
  REQUIRE(graphic.findLayer(QStringLiteral("0")) != nullptr);
  REQUIRE(graphic.findLayer(QStringLiteral("0"))->isPrint());
  REQUIRE(graphic.findLayer(QStringLiteral("NOPLOT")) != nullptr);
  REQUIRE(!graphic.findLayer(QStringLiteral("NOPLOT"))->isPrint());

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));  // AC1021 (R2007+, > AC1014)
  }

  // Regression: with a plot-on layer the old writer emitted NO 290 at all.
  CHECK(recordHasCode(out, "LAYER", "290"));
  CHECK(layerPlotFlag(out, "0") == "1");        // plottable
  CHECK(layerPlotFlag(out, "NOPLOT") == "0");   // not plottable

  // Both values round-trip back through the reader.
  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  REQUIRE(reloaded.findLayer(QStringLiteral("0")) != nullptr);
  REQUIRE(reloaded.findLayer(QStringLiteral("NOPLOT")) != nullptr);
  CHECK(reloaded.findLayer(QStringLiteral("0"))->isPrint());
  CHECK(!reloaded.findLayer(QStringLiteral("NOPLOT"))->isPrint());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// Regression (Save-As failure on AC1021 sources, e.g. 植物.dwg): the DWG
// writer emits its control objects at FIXED low handles (LTYPE_CONTROL=0x05,
// UCS_CONTROL=0x07, ...) while real source files reuse those very numbers for
// ordinary preserved OBJECTS (ACDBPLACEHOLDER@0x5, DICTIONARY@0x7). Without
// the structural-collision remap both sides land in the object map,
// writeDwgHandles() aborts the whole save (BAD_OPEN) and Save-As leaves a
// zero-byte file. fileExport now remaps colliding typed objects above the
// preserved high-water mark and rewrites typed references to them.
TEST_CASE("DWG export remaps preserved objects colliding with fixed writer handles",
          "[dwg][roundtrip][fixed-handle-remap]") {
  ensureSettings();

  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto& md = graphic.dwgAdvancedMetadata();
  DRW_AcDbPlaceholder ph;
  ph.handle = 0x5;       // collides with the writer's LTYPE_CONTROL handle
  ph.parentHandle = 0x7;
  md.addAcDbPlaceholder(ph);
  DRW_Dictionary dict;
  dict.handle = 0x7;     // collides with the writer's UCS_CONTROL handle
  dict.parentHandle = 0xC;
  DRW_Dictionary::Entry entry;
  entry.m_name = "LC_REMAP_TEST";
  entry.m_handle = 0x5;  // reference must follow the placeholder's remap
  dict.m_entries.push_back(entry);
  md.addDictionary(dict);

  const std::string out = tmpFile("fixed_handle_remap.dwg");
  std::filesystem::remove(out);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDWG));
  }
  REQUIRE(std::filesystem::exists(out));
  CHECK(std::filesystem::file_size(out) > 0);

  // Re-import: the file must parse, and the preserved pair must come back on
  // fresh handles with the dictionary entry still pointing at the placeholder.
  RS_Graphic reloaded;
  reloaded.initForNewDocument();
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                              RS2::FormatDWG));
  }
  const auto& rmd = reloaded.dwgAdvancedMetadata();
  REQUIRE(rmd.placeholders().size() == 1);
  const std::uint32_t phHandle = rmd.placeholders().front().handle;
  CHECK(phHandle != 0x5u);
  CHECK(phHandle > 0x2Fu);
  bool entryFollowsRemap = false;
  for (const auto& d : rmd.dictionaries())
    for (const auto& e : d.entries)
      if (e.name == "LC_REMAP_TEST" && e.handle == phHandle)
        entryFollowsRemap = true;
  CHECK(entryFollowsRemap);

  std::filesystem::remove(out);
}
