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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <QCoreApplication>

#include "drw_base.h"
#include "lc_dimstyle.h"
#include "lc_dwgadvancedmetadata.h"
#include "rs_dimaligned.h"
#include "rs_dimension.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_entity.h"
#include "rs_block.h"
#include "rs_layer.h"
#include "rs_point.h"
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

std::string trimDxfToken(std::string value) {
  if (!value.empty() && value.back() == '\r')
    value.pop_back();
  const size_t first = value.find_first_not_of(" \t");
  return first == std::string::npos ? std::string() : value.substr(first);
}

std::string dstyleDirectionVariant(const std::string &path,
                                   const std::string &directionCode,
                                   const std::string &valueCode,
                                   const std::string &value,
                                   bool closeGroup = true) {
  std::ifstream input(path);
  std::vector<std::pair<std::string, std::string>> groups;
  std::string code, groupValue;
  while (std::getline(input, code) && std::getline(input, groupValue))
    groups.emplace_back(trimDxfToken(code), trimDxfToken(groupValue));

  bool inDimension = false;
  bool inDstyle = false;
  for (size_t i = 0; i + 1 < groups.size(); ++i) {
    const auto &[groupCode, groupValueText] = groups[i];
    if (groupCode == "0") {
      inDimension = groupValueText == "DIMENSION";
      inDstyle = false;
      continue;
    }
    if (!inDimension)
      continue;
    if (groupCode == "1000" && groupValueText == "DSTYLE") {
      inDstyle = true;
      continue;
    }
    if (!inDstyle || groupCode != "1070" || groups[i + 1].first != "1070")
      continue;

    groups[i].second = directionCode;
    groups[i + 1] = {valueCode, value};
    if (!closeGroup) {
      const auto close = std::find_if(groups.begin() + static_cast<std::ptrdiff_t>(i + 2),
                                      groups.end(), [](const auto &group) {
        return group.first == "1002" && group.second == "}";
      });
      if (close != groups.end())
        groups.erase(close);
    }
    std::ostringstream output;
    for (const auto &[outCode, outValue] : groups)
      output << outCode << '\n' << outValue << '\n';
    return output.str();
  }
  return {};
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

using DxfRecordGroups = std::vector<std::pair<std::string, std::string>>;

DxfRecordGroups recordGroupsWithValue(const std::string &path,
                                      const std::string &recordName,
                                      const std::string &selectorCode,
                                      const std::string &selectorValue) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  DxfRecordGroups groups;
  bool inRecord = false;
  bool selected = false;
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string code = trimDxfToken(codeLine);
    const std::string value = trimDxfToken(valueLine);
    if (code == "0") {
      if (selected)
        return groups;
      inRecord = value == recordName;
      groups.clear();
      continue;
    }
    if (!inRecord)
      continue;
    groups.emplace_back(code, value);
    selected = selected || (code == selectorCode && value == selectorValue);
  }
  return selected ? groups : DxfRecordGroups{};
}

std::vector<std::string> namedRecordGroupValues(
    const std::string &path, const std::string &recordType,
    const std::string &recordName, const std::string &code) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::vector<std::string> values;
  std::vector<std::string> result;
  bool inRecord = false;
  bool matchingName = false;
  auto trim = [](std::string value) {
    if (!value.empty() && value.back() == '\r')
      value.pop_back();
    const size_t first = value.find_first_not_of(" \t");
    return first == std::string::npos ? std::string() : value.substr(first);
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string groupCode = trim(codeLine);
    const std::string value = trim(valueLine);
    if (groupCode == "0") {
      if (inRecord && matchingName)
        result = values;
      inRecord = value == recordType;
      matchingName = false;
      values.clear();
    }
    else if (inRecord && groupCode == "2" && value == recordName) {
      matchingName = true;
    }
    else if (inRecord && matchingName && groupCode == code) {
      values.push_back(value);
    }
  }
  if (inRecord && matchingName)
    result = values;
  return result;
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

bool containsLine(const std::string &path, const std::string &expected) {
  std::ifstream in(path);
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line == expected)
      return true;
  }
  return false;
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

TEST_CASE("DXF entity common reference groups round-trip",
          "[dxf][roundtrip][filter][common-references]") {
  ensureSettings();
  const std::string src = tmpFile("entity_common_references_src.dxf");
  const std::string out = tmpFile("entity_common_references_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\n40\n330\n1F\n"
            "102\n{ACAD_REACTORS\n330\nA0\n330\nA1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nB0\n102\n}\n"
            "100\nAcDbEntity\n8\n0\n10\n0\n20\n0\n"
            "11\n1\n21\n1\n347\nC0\n348\nD0\n"
            "390\nE0\n284\n3\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  RS_Entity *line = graphic.firstEntity();
  REQUIRE(line != nullptr);
  CHECK(line->materialHandle() == 0xC0u);
  CHECK(line->plotStyleHandle() == 0xE0u);
  CHECK(line->shadowMode() == static_cast<int>(DRW::IgnoreShadows));
  CHECK(line->fullVisualStyleHandle() == 0xD0u);
  CHECK(line->reactorHandles() == std::vector<quint32>{0xA0u, 0xA1u});
  CHECK(line->xDictHandle() == 0xB0u);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(containsLine(out, "{ACAD_REACTORS"));
  CHECK(containsLine(out, "{ACAD_XDICTIONARY"));
  CHECK(recordGroupValues(out, "LINE", "330")
        == std::vector<std::string>{"1F", "A0", "A1"});
  CHECK(recordGroupValues(out, "LINE", "360")
        == std::vector<std::string>{"B0"});
  CHECK(recordGroupValues(out, "LINE", "348")
        == std::vector<std::string>{"D0"});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF application groups reject excessive nesting before publishing",
          "[dxf][safety][application-groups]") {
  ensureSettings();
  const std::string src = tmpFile("application_group_nesting_src.dxf");
  std::filesystem::remove(src);

  std::string dxf = "0\nSECTION\n2\nENTITIES\n"
                    "0\nLINE\n8\n0\n102\n{OUTER\n";
  for (int i = 0; i < 64; ++i)
    dxf += "102\n{NESTED\n";
  dxf += "102\n{TOO_DEEP\n";
  for (int i = 0; i < 65; ++i)
    dxf += "102\n}\n";
  dxf += "10\n0\n20\n0\n11\n1\n21\n1\n"
         "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileImport(graphic, QString::fromStdString(src),
                                RS2::FormatDXFRW));
  CHECK(graphic.count() == 0);

  std::filesystem::remove(src);
}

TEST_CASE("DXF application groups reject excessive pairs before publishing",
          "[dxf][safety][application-groups]") {
  ensureSettings();
  const std::string src = tmpFile("application_group_pairs_src.dxf");
  std::filesystem::remove(src);

  std::string dxf = "0\nSECTION\n2\nENTITIES\n"
                    "0\nLINE\n8\n0\n102\n{TOO_LARGE\n";
  // The first pair is the opener.  One more than the shared 65,536-pair
  // transaction limit must be rejected without adding a partial LINE.
  for (int i = 0; i < 65536; ++i)
    dxf += "1\nopaque\n";
  dxf += "102\n}\n10\n0\n20\n0\n11\n1\n21\n1\n"
         "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileImport(graphic, QString::fromStdString(src),
                                RS2::FormatDXFRW));
  CHECK(graphic.count() == 0);

  std::filesystem::remove(src);
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

TEST_CASE("DXF filter preserves reflected planar curve conventions once",
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
  // CIRCLE and ARC retain their source OCS plane metadata so editable
  // geometry can be exported back to the original source fields. ELLIPSE
  // remains on the normalized WCS path used by its existing implementation.
  CHECK(firstGroupValueAsDouble(out, "CIRCLE", "10") == 10.0);
  CHECK(firstGroupValueAsDouble(out, "ARC", "10") == 20.0);
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "10") == -30.0);
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "11") == -5.0);
  for (const char *record : {"CIRCLE", "ARC"}) {
    CHECK(recordGroupValues(out, record, "210") ==
          std::vector<std::string>{"0"});
    CHECK(recordGroupValues(out, record, "220") ==
          std::vector<std::string>{"0"});
    CHECK(recordGroupValues(out, record, "230") ==
          std::vector<std::string>{"-1"});
  }
  CHECK(recordGroupValues(out, "ELLIPSE", "210").empty());

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }

  // Reading the first export again must not transform any curve a second
  // time, regardless of whether its export convention is OCS or WCS.
  for (const auto &[record, codes] : std::vector<
           std::pair<const char *, std::vector<const char *>>>{
           {"CIRCLE", {"10", "20", "40"}},
           {"ARC", {"10", "20", "40", "50", "51"}},
           {"ELLIPSE", {"10", "20", "11", "21", "40", "41", "42"}}}) {
    for (const char *code : codes)
      CHECK(recordGroupValues(out2, record, code)
            == recordGroupValues(out, record, code));
    for (const char *code : {"210", "220", "230"})
      CHECK(recordGroupValues(out2, record, code) ==
            recordGroupValues(out, record, code));
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
  const std::string dwg = tmpFile("wcs_point_line_extrusion.dwg");
  const std::string dwgOut = tmpFile("wcs_point_line_extrusion_from_dwg.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);

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
            "0\n3DLINE\n8\n0\n10\n10.0\n20\n11.0\n30\n12.0\n"
            "11\n13.0\n21\n14.0\n31\n15.0\n"
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
                     {"210", "0"}, {"220", "1"}, {"230", "0"}}},
           {"3DLINE", {{"30", "12"}, {"31", "15"}, {"39", "0"},
                       {"210", "0"}, {"220", "0"}, {"230", "1"}}}}) {
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
           {"LINE", {"10", "20", "30", "11", "21", "31", "39", "210", "220", "230"}},
           {"3DLINE", {"10", "20", "30", "11", "21", "31", "39", "210", "220", "230"}}}) {
    for (const char *code : codes)
    CHECK(recordGroupValues(out2, record, code)
            == recordGroupValues(out, record, code));
  }

  RS_Graphic dwgGraphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwg),
                              RS2::FormatDWG));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(dwgGraphic, QString::fromStdString(dwg),
                              RS2::FormatDWG));
    REQUIRE(filter.fileExport(dwgGraphic, QString::fromStdString(dwgOut),
                              RS2::FormatDXFRW));
  }
  REQUIRE(countRecords(dwgOut, "3DLINE") == 1);
  CHECK(firstGroupValueAsDouble(dwgOut, "3DLINE", "30") == 12.0);
  CHECK(firstGroupValueAsDouble(dwgOut, "3DLINE", "31") == 15.0);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF filter preserves ARC OCS extrusion and reflected sweep",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("arc_ocs_extrusion_src.dxf");
  const std::string out = tmpFile("arc_ocs_extrusion_out.dxf");
  const std::string out2 = tmpFile("arc_ocs_extrusion_out2.dxf");
  const std::string dwg = tmpFile("arc_ocs_extrusion.dwg");
  const std::string dwgOut = tmpFile("arc_ocs_extrusion_from_dwg.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);

  // ARC center and angles are OCS data. The importer applies the -Z OCS
  // transform for LibreCAD's planar model; export must invert it before
  // writing group 10/20/30 and 50/51 back to the source plane.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nARC\n8\n0\n10\n4.0\n20\n5.0\n30\n2.0\n"
            "40\n6.0\n39\n2.5\n210\n0.0\n220\n0.0\n230\n-1.0\n"
            "50\n11.4591559026165\n51\n63.0253574643906\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  REQUIRE(countRecords(out, "ARC") == 1);
  for (const auto &[code, expected] :
       std::initializer_list<std::pair<const char *, double>>{
           {"10", 4.0}, {"20", 5.0}, {"30", 2.0}, {"40", 6.0},
           {"39", 2.5}, {"210", 0.0}, {"220", 0.0}, {"230", -1.0},
           {"50", 11.4591559026165}, {"51", 63.0253574643906}}) {
    CHECK(firstGroupValueAsDouble(out, "ARC", code) ==
          Catch::Approx(expected));
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const char *code :
       {"10", "20", "30", "40", "39", "210", "220", "230", "50", "51"}) {
    CHECK(recordGroupValues(out2, "ARC", code) ==
          recordGroupValues(out, "ARC", code));
  }

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
  for (const char *code :
       {"10", "20", "30", "40", "39", "210", "220", "230", "50", "51"}) {
    CHECK(recordGroupValues(dwgOut, "ARC", code) ==
          recordGroupValues(out, "ARC", code));
  }
#endif

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF filter preserves CIRCLE OCS extrusion",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("circle_ocs_extrusion_src.dxf");
  const std::string out = tmpFile("circle_ocs_extrusion_out.dxf");
  const std::string out2 = tmpFile("circle_ocs_extrusion_out2.dxf");
  const std::string dwg = tmpFile("circle_ocs_extrusion.dwg");
  const std::string dwgOut = tmpFile("circle_ocs_extrusion_from_dwg.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);

  // CIRCLE center is OCS data. With -Z, the importer's axial OCS transform
  // reflects the editable center; export must restore the original OCS center.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nCIRCLE\n8\n0\n10\n4.0\n20\n5.0\n30\n2.0\n"
            "40\n6.0\n39\n2.5\n210\n0.0\n220\n0.0\n230\n-1.0\n"
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
  for (const auto &[code, expected] :
       std::initializer_list<std::pair<const char *, double>>{
           {"10", 4.0}, {"20", 5.0}, {"30", 2.0}, {"40", 6.0},
           {"39", 2.5}, {"210", 0.0}, {"220", 0.0}, {"230", -1.0}}) {
    CHECK(firstGroupValueAsDouble(out, "CIRCLE", code) ==
          Catch::Approx(expected));
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  for (const char *code : {"10", "20", "30", "40", "39", "210",
                           "220", "230"}) {
    CHECK(recordGroupValues(out2, "CIRCLE", code) ==
          recordGroupValues(out, "CIRCLE", code));
  }

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
  for (const char *code : {"10", "20", "30", "40", "39", "210",
                           "220", "230"}) {
    CHECK(recordGroupValues(dwgOut, "CIRCLE", code) ==
          recordGroupValues(out, "CIRCLE", code));
  }
#endif

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF filter preserves WCS POINT and LINE Z without side metadata",
          "[dxf][roundtrip][filter][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("wcs_point_line_z_src.dxf");
  const std::string out = tmpFile("wcs_point_line_z_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // POINT and LINE coordinates are WCS even with the default normal. Their
  // nonzero Z values still require a sidecar because LibreCAD stores 2D
  // geometry and would otherwise write an implicit z=0.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nPOINT\n8\n0\n10\n1.0\n20\n2.0\n30\n3.0\n"
            "0\nLINE\n8\n0\n10\n4.0\n20\n5.0\n30\n6.0\n"
            "11\n7.0\n21\n8.0\n31\n9.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(firstGroupValueAsDouble(out, "POINT", "30") == Catch::Approx(3.0));
  CHECK(firstGroupValueAsDouble(out, "LINE", "30") == Catch::Approx(6.0));
  CHECK(firstGroupValueAsDouble(out, "LINE", "31") == Catch::Approx(9.0));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
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
            "13\n16.0\n23\n12.0\n340\n1\n70\n3\n90\n4\n"
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
  REQUIRE(recordGroupValues(out, "IMAGE", "90") == std::vector<std::string>{"4"});
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
            "0\nBLOCK\n5\n20\n8\n0\n2\nGRID_SYMBOL\n70\n0\n"
            "10\n1.0\n20\n2.0\n30\n0.0\n3\nGRID_SYMBOL\n1\n\n"
            "0\nLINE\n8\n0\n10\n1.0\n20\n2.0\n11\n3.0\n21\n2.0\n"
            "0\nENDBLK\n5\n21\n8\n0\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nINSERT\n5\n22\n8\n0\n2\nGRID_SYMBOL\n"
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

TEST_CASE("DXF filter preserves fixture-backed BLOCK_RECORD preview chunks",
          "[dxf][roundtrip][filter][blockrecord][preview]") {
  ensureSettings();
  const std::string src = std::string(LIBRECAD_TEST_DIR)
                          + "/dxf/block_record_preview_r2007.dxf";
  const std::string out = tmpFile("block_record_preview_out.dxf");
  const std::string out2 = tmpFile("block_record_preview_out2.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  if (!std::filesystem::is_regular_file(src)) {
    SKIP("block_record_preview_r2007.dxf fixture absent; skipping");
  }
  CHECK(recordGroupValues(src, "BLOCK_RECORD", "310")
        == std::vector<std::string>{"414243", "DE"});

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  const RS_Block* block = graphic.findBlock(QStringLiteral("PREVIEW_BLOCK"));
  REQUIRE(block != nullptr);
  CHECK(block->getPreviewData()
        == std::vector<std::uint8_t>{0x41, 0x42, 0x43, 0xDE});
  CHECK(recordGroupValues(out, "BLOCK_RECORD", "310")
        == std::vector<std::string>{"414243DE"});

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  CHECK(recordGroupValues(out2, "BLOCK_RECORD", "310")
        == recordGroupValues(out, "BLOCK_RECORD", "310"));

  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves fixture-backed EED binary items",
          "[dxf][roundtrip][filter][eed][binary]") {
  ensureSettings();
  const std::string src = std::string(LIBRECAD_TEST_DIR)
                          + "/dxf/eed_binary_r2007.dxf";
  const std::string out = tmpFile("eed_binary_out.dxf");
  const std::string out2 = tmpFile("eed_binary_out2.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  if (!std::filesystem::is_regular_file(src)) {
    SKIP("eed_binary_r2007.dxf fixture absent; skipping");
  }
  CHECK(recordGroupValues(src, "POINT", "1004")
        == std::vector<std::string>{"0102", "A0B1C2"});

  const auto checkPointXData = [](RS_Graphic& drawing) {
    auto* point = dynamic_cast<RS_Point*>(drawing.firstEntity());
    REQUIRE(point != nullptr);
    const auto& extData = point->getDrwExtData();
    REQUIRE(extData.size() == 4);
    REQUIRE(extData[0] != nullptr);
    REQUIRE(extData[1] != nullptr);
    REQUIRE(extData[2] != nullptr);
    REQUIRE(extData[3] != nullptr);
    CHECK(extData[0]->code() == 1001);
    CHECK(std::string(extData[0]->c_str()) == "LIBRECAD_EED");
    CHECK(extData[1]->code() == 1000);
    CHECK(std::string(extData[1]->c_str()) == "binary items stay distinct");
    for (std::size_t i = 0; i < 2; ++i) {
      const auto& chunk = extData[i + 2];
      CHECK(chunk->code() == 1004);
      CHECK(chunk->type() == DRW_Variant::STRING);
    }
    CHECK(std::string(extData[2]->c_str()) == "0102");
    CHECK(std::string(extData[3]->c_str()) == "A0B1C2");
  };

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  checkPointXData(graphic);
  CHECK(recordGroupValues(out, "POINT", "1004")
        == std::vector<std::string>{"0102", "A0B1C2"});

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  checkPointXData(graphic2);
  CHECK(recordGroupValues(out2, "POINT", "1004")
        == std::vector<std::string>{"0102", "A0B1C2"});

  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves fixture-backed CLASS and raw entity",
          "[dxf][roundtrip][filter][classes][raw]") {
  ensureSettings();
  const std::string src = std::string(LIBRECAD_TEST_DIR)
                          + "/dxf/classes_raw_entity_r2007.dxf";
  const std::string out = tmpFile("classes_raw_entity_out.dxf");
  const std::string out2 = tmpFile("classes_raw_entity_out2.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  if (!std::filesystem::is_regular_file(src)) {
    SKIP("classes_raw_entity_r2007.dxf fixture absent; skipping");
  }
  CHECK(recordGroupValues(src, "CLASS", "91")
        == std::vector<std::string>{"7"});

  const auto checkMetadata = [](const RS_Graphic& drawing,
                                int expectedInstanceCount) {
    const auto& metadata = drawing.dwgAdvancedMetadata();
    const auto& classes = metadata.dxfClasses();
    const auto classIt = std::find_if(
        classes.cbegin(), classes.cend(), [](const auto& entry) {
          return entry.recName == "WEIRDENT";
        });
    REQUIRE(classIt != classes.cend());
    CHECK(classIt->className == "AcDbWeirdEntity");
    CHECK(classIt->appName == "CUSTOM_APP");
    CHECK(classIt->proxyFlag == 4095);
    CHECK(classIt->instanceCount == expectedInstanceCount);
    CHECK(classIt->wasaProxyFlag == 1);
    CHECK(classIt->entityFlag == 1);

    const auto& rawEntities = metadata.rawDxfEntities();
    const auto rawIt = std::find_if(
        rawEntities.cbegin(), rawEntities.cend(), [](const auto& entry) {
          return entry.name == "WEIRDENT";
        });
    REQUIRE(rawIt != rawEntities.cend());
    CHECK(rawIt->handle == 0x7Au);
  };

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  checkMetadata(graphic, 7);
  CHECK(countRecords(out, "WEIRDENT") == 1);
  CHECK(classRecordNames(out) == std::set<std::string>{"WEIRDENT"});
  CHECK(recordGroupValues(out, "CLASS", "2")
        == std::vector<std::string>{"AcDbWeirdEntity"});
  CHECK(recordGroupValues(out, "CLASS", "3")
        == std::vector<std::string>{"CUSTOM_APP"});
  CHECK(recordGroupValues(out, "CLASS", "90")
        == std::vector<std::string>{"4095"});
  CHECK(recordGroupValues(out, "CLASS", "91")
        == std::vector<std::string>{"1"});
  CHECK(recordGroupValues(out, "CLASS", "280")
        == std::vector<std::string>{"1"});
  CHECK(recordGroupValues(out, "CLASS", "281")
        == std::vector<std::string>{"1"});

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  checkMetadata(graphic2, 1);
  CHECK(countRecords(out2, "WEIRDENT") == 1);
  CHECK(classRecordNames(out2) == std::set<std::string>{"WEIRDENT"});
  CHECK(recordGroupValues(out2, "CLASS", "91")
        == std::vector<std::string>{"1"});

  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves fixture-backed raw control groups and remaps 481",
          "[dxf][roundtrip][filter][application-groups][handles]") {
  ensureSettings();
  const std::string src = std::string(LIBRECAD_TEST_DIR)
                          + "/dxf/raw_control_groups_r2007.dxf";
  const std::string out = tmpFile("raw_control_groups_out.dxf");
  const std::string out2 = tmpFile("raw_control_groups_out2.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  if (!std::filesystem::is_regular_file(src)) {
    SKIP("raw_control_groups_r2007.dxf fixture absent; skipping");
  }

  const auto values = [](const DxfRecordGroups& groups, const char* code) {
    std::vector<std::string> result;
    for (const auto& [groupCode, value] : groups) {
      if (groupCode == code)
        result.push_back(value);
    }
    return result;
  };
  const auto firstValue = [&](const DxfRecordGroups& groups, const char* code) {
    const std::vector<std::string> found = values(groups, code);
    REQUIRE(found.size() == 1);
    return found.front();
  };
  const auto controlGroups = [&](const DxfRecordGroups& groups) {
    std::vector<std::string> result;
    for (const auto& [code, value] : groups) {
      if (code == "102")
        result.push_back(value);
    }
    return result;
  };
  const auto rawValues = [](const std::vector<DRW_Variant>& groups, int code) {
    std::vector<std::string> result;
    for (const DRW_Variant& group : groups) {
      if (group.code() == code && group.c_str() != nullptr)
        result.emplace_back(group.c_str());
    }
    return result;
  };
  const auto verifySourceRecord = [&](const DxfRecordGroups& source,
                                      const DxfRecordGroups& target) {
    REQUIRE_FALSE(source.empty());
    REQUIRE_FALSE(target.empty());
    CHECK(values(source, "330") == std::vector<std::string>{"C", "80"});
    CHECK(values(source, "360") == std::vector<std::string>{"81"});
    CHECK(controlGroups(source)
          == std::vector<std::string>{"{CUSTOM_CONTROL", "{NESTED", "}", "}",
                                      "{ACAD_REACTORS", "}",
                                      "{ACAD_XDICTIONARY", "}"});
    const std::string targetHandle = firstValue(target, "5");
    CHECK(targetHandle != "14");
    CHECK(values(source, "481") == std::vector<std::string>{targetHandle});

    const auto owner = std::find(source.cbegin(), source.cend(),
                                 std::pair<std::string, std::string>{"330", "C"});
    const auto custom = std::find(source.cbegin(), source.cend(),
                                  std::pair<std::string, std::string>{"102", "{CUSTOM_CONTROL"});
    REQUIRE(owner != source.cend());
    REQUIRE(custom != source.cend());
    CHECK(owner < custom);
  };

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  const auto raw = std::find_if(
      graphic.dwgAdvancedMetadata().rawDxfObjects().cbegin(),
      graphic.dwgAdvancedMetadata().rawDxfObjects().cend(),
      [](const auto& entry) { return entry.name == "MATERIAL" && entry.handle == 0x90u; });
  REQUIRE(raw != graphic.dwgAdvancedMetadata().rawDxfObjects().cend());
  CHECK(raw->parentHandle == 0xCu);
  CHECK(rawValues(raw->groups, 481) == std::vector<std::string>{"14"});
  CHECK(rawValues(raw->groups, 102)
        == std::vector<std::string>{"{CUSTOM_CONTROL", "{NESTED", "}", "}",
                                    "{ACAD_REACTORS", "}",
                                    "{ACAD_XDICTIONARY", "}"});

  const DxfRecordGroups sourceOut =
      recordGroupsWithValue(out, "MATERIAL", "1", "SOURCE");
  const DxfRecordGroups targetOut =
      recordGroupsWithValue(out, "MATERIAL", "1", "TARGET");
  verifySourceRecord(sourceOut, targetOut);
  const std::vector<std::string> handles = collectHandles(out);
  std::set<std::string> uniqueHandles;
  for (const std::string& handle : handles) {
    INFO("duplicate handle: " << handle);
    CHECK(uniqueHandles.insert(handle).second);
  }

  RS_Graphic graphic2;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic2, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  verifySourceRecord(recordGroupsWithValue(out2, "MATERIAL", "1", "SOURCE"),
                     recordGroupsWithValue(out2, "MATERIAL", "1", "TARGET"));

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

TEST_CASE("DXF export re-emits DWG-read CAMERA entities",
          "[dxf][roundtrip][filter][camera]") {
  ensureSettings();
  const std::string src = tmpFile("camerasrc.dxf");
  const std::string out = tmpFile("camera.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  DRW_Camera camera;
  camera.handle = 0x310u;
  camera.parentHandle = 0x1Fu;
  camera.m_viewHandle = 0x90u;
  camera.extData.push_back(
      std::make_shared<DRW_Variant>(1001, std::string{"CAMERA_APP"}));
  camera.extData.push_back(
      std::make_shared<DRW_Variant>(1000, std::string{"camera-xdata"}));
  graphic.dwgAdvancedMetadata().addCamera(camera);
  REQUIRE(graphic.dwgAdvancedMetadata().cameras().size() == 1);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "CAMERA") == 1);
  CHECK(recordHasCode(out, "CAMERA", "100"));
  CHECK(recordHasCode(out, "CAMERA", "340"));
  CHECK(recordHasCode(out, "CAMERA", "1000"));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DWG export re-emits metadata CAMERA entities",
          "[dwg][roundtrip][filter][camera]") {
  ensureSettings();
  const std::string src = tmpFile("camera_dwg_src.dxf");
  const std::string out = tmpFile("camera_dwg.dwg");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  DRW_Camera camera;
  camera.handle = 0x310u;
  camera.parentHandle = 0x1Fu;
  camera.m_viewHandle = 0x90u;
  camera.extData = {
      std::make_shared<DRW_Variant>(1001, std::string{"CAMERA_EED"}),
      std::make_shared<DRW_Variant>(1000, std::string{"camera-eed"}),
  };
  auto *namedView = new LC_View("CameraView");
  graphic.getViewList()->add(namedView);
  DRW_View view;
  view.handle = camera.m_viewHandle;
  view.name = "CameraView";
  graphic.dwgAdvancedMetadata().addView(view);
  graphic.dwgAdvancedMetadata().addCamera(camera);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDWG2013));
  }

  RS_Graphic reopened;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(out),
                              RS2::FormatDWG));
  }

  REQUIRE(reopened.dwgAdvancedMetadata().cameras().size() == 1);
  const auto &stored = reopened.dwgAdvancedMetadata().cameras().front();
  CHECK(stored.viewHandle != DRW::NoHandle);
  CHECK(stored.viewHandle != camera.m_viewHandle);
  // Model-space entity ownership is carried by the BLOCK_RECORD entity list;
  // the common entity data therefore uses entmode=2 and has no owner handle.
  CHECK(stored.parentHandle == DRW::NoHandle);
  REQUIRE(stored.extData.size() == 2);
  CHECK(std::string(stored.extData[0]->c_str()) == "CAMERA_EED");
  CHECK(std::string(stored.extData[1]->c_str()) == "camera-eed");

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF round-trip preserves SECTIONOBJECT settings reference",
          "[dxf][roundtrip][filter][section-object]") {
  ensureSettings();
  const std::string src = tmpFile("section_object_dxf_src.dxf");
  const std::string out = tmpFile("section_object_dxf.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  DRW_Section settings;
  settings.handle = 0x420u;
  settings.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
  settings.m_kind = DRW_Section::Settings;
  settings.m_classVersion = 3;
  settings.m_sectionType = 4;
  settings.m_generationOptions = 5;
  settings.m_currentType = 4;
  settings.m_typeCount = 1;
  DRW_SectionTypeSettings dxfType;
  dxfType.m_type = 4;
  dxfType.m_generation = 17;
  dxfType.m_sourceHandles = {0x421u};
  dxfType.m_numSources = 1;
  dxfType.m_destinationBlockHandle = DRW::DxfModelSpaceBlockRecordHandle;
  dxfType.m_destinationFile = "section.dwg";
  dxfType.m_numGeometrySettings = 1;
  DRW_SectionGeometrySettings dxfGeometry;
  dxfGeometry.m_numGeometries = 2;
  dxfGeometry.m_hexIndex = 9;
  dxfGeometry.m_flags = 3;
  dxfGeometry.m_color = 2;
  dxfGeometry.m_layer = "CUT";
  dxfGeometry.m_lineType = "CONTINUOUS";
  dxfGeometry.m_lineTypeScale = 1.25;
  dxfGeometry.m_plotStyle = "Normal";
  dxfGeometry.m_lineWeight = 25;
  dxfGeometry.m_faceTransparency = 1;
  dxfGeometry.m_edgeTransparency = 2;
  dxfGeometry.m_hatchType = 3;
  dxfGeometry.m_hatchPattern = "ANSI31";
  dxfGeometry.m_hatchAngle = 0.5;
  dxfGeometry.m_hatchSpacing = 2.0;
  dxfGeometry.m_hatchScale = 0.75;
  dxfType.m_geometry.push_back(dxfGeometry);
  settings.m_types.push_back(dxfType);
  graphic.dwgAdvancedMetadata().addSection(settings);

  DRW_SectionObject section;
  section.handle = 0x421u;
  section.parentHandle = DRW::DxfModelSpaceBlockRecordHandle;
  section.m_state = 7u;
  section.m_flags = 9u;
  section.m_name = "Section DXF";
  section.m_vertDir = {0.0, 0.0, 1.0};
  section.m_verts = {{1.0, 2.0, 0.0}, {3.0, 4.0, 0.0}};
  section.m_sectionSettingsHandle = settings.handle;
  graphic.dwgAdvancedMetadata().addSectionObject(section);

  DRW_Section manager;
  manager.handle = 0x422u;
  manager.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
  manager.m_kind = DRW_Section::Manager;
  manager.m_isLive = true;
  manager.m_sectionHandles = {section.handle};
  graphic.dwgAdvancedMetadata().addSection(manager);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "SECTIONOBJECT") == 1);
  CHECK(countRecords(out, "SECTIONSETTINGS") == 1);
  CHECK(recordHasCode(out, "SECTIONOBJECT", "360"));

  RS_Graphic reopened;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  REQUIRE(reopened.dwgAdvancedMetadata().sectionObjects().size() == 1u);
  const auto& storedSection =
      reopened.dwgAdvancedMetadata().sectionObjects().front();
  CHECK(storedSection.parentHandle == DRW::DxfModelSpaceBlockRecordHandle);
  CHECK(storedSection.name == section.m_name);
  CHECK(storedSection.sectionSettingsHandle == settings.handle);
  REQUIRE(reopened.dwgAdvancedMetadata().sections().size() == 2u);
  const auto settingsIt = std::find_if(
      reopened.dwgAdvancedMetadata().sections().begin(),
      reopened.dwgAdvancedMetadata().sections().end(),
      [](const LC_DwgAdvancedMetadata::SectionRecord& record) {
        return record.kind == DRW_Section::Settings;
      });
  REQUIRE(settingsIt != reopened.dwgAdvancedMetadata().sections().end());
  CHECK(settingsIt->typeCount == 1);
  REQUIRE(settingsIt->types.size() == 1u);
  CHECK(settingsIt->types.front().m_type == dxfType.m_type);
  CHECK(settingsIt->types.front().m_generation == dxfType.m_generation);
  CHECK(settingsIt->types.front().m_destinationFile
        == dxfType.m_destinationFile);
  CHECK(settingsIt->types.front().m_sourceHandles.size() == 1u);
  CHECK(settingsIt->types.front().m_sourceHandles.front()
        == storedSection.handle);
  REQUIRE(settingsIt->types.front().m_geometry.size() == 1u);
  CHECK(settingsIt->types.front().m_geometry.front().m_layer
        == dxfGeometry.m_layer);
  CHECK(settingsIt->types.front().m_geometry.front().m_hatchScale
        == dxfGeometry.m_hatchScale);
  const auto managerIt = std::find_if(
      reopened.dwgAdvancedMetadata().sections().begin(),
      reopened.dwgAdvancedMetadata().sections().end(),
      [](const LC_DwgAdvancedMetadata::SectionRecord& record) {
        return record.kind == DRW_Section::Manager;
      });
  REQUIRE(managerIt != reopened.dwgAdvancedMetadata().sections().end());
  REQUIRE(managerIt->sectionHandles.size() == 1u);
  CHECK(managerIt->sectionHandles.front() == storedSection.handle);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DWG round-trip preserves SECTIONOBJECT settings reference",
          "[dwg][roundtrip][filter][section-object]") {
  ensureSettings();
  const std::string out = tmpFile("section_object_dwg.dwg");
  std::filesystem::remove(out);

  RS_Graphic source;
  source.initForNewDocument();

  DRW_Section settings;
  settings.handle = 0x430u;
  settings.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
  settings.m_kind = DRW_Section::Settings;
  settings.m_currentType = 4;
  settings.m_typeCount = 1;
  DRW_SectionTypeSettings dwgType;
  dwgType.m_type = 4;
  dwgType.m_generation = 17;
  dwgType.m_sourceHandles = {0x431u};
  dwgType.m_numSources = 1;
  dwgType.m_destinationBlockHandle = DRW::DwgModelSpaceBlockRecordHandle;
  dwgType.m_destinationFile = "section.dwg";
  dwgType.m_numGeometrySettings = 1;
  DRW_SectionGeometrySettings dwgGeometry;
  dwgGeometry.m_numGeometries = 2;
  dwgGeometry.m_hexIndex = 9;
  dwgGeometry.m_flags = 3;
  dwgGeometry.m_color = 2;
  dwgGeometry.m_layer = "CUT";
  dwgGeometry.m_lineType = "CONTINUOUS";
  dwgGeometry.m_lineTypeScale = 1.25;
  dwgGeometry.m_plotStyle = "Normal";
  dwgGeometry.m_lineWeight = 25;
  dwgGeometry.m_faceTransparency = 1;
  dwgGeometry.m_edgeTransparency = 2;
  dwgGeometry.m_hatchType = 3;
  dwgGeometry.m_hatchPattern = "ANSI31";
  dwgGeometry.m_hatchAngle = 0.5;
  dwgGeometry.m_hatchSpacing = 2.0;
  dwgGeometry.m_hatchScale = 0.75;
  dwgType.m_geometry.push_back(dwgGeometry);
  settings.m_types.push_back(dwgType);
  source.dwgAdvancedMetadata().addSection(settings);

  DRW_SectionObject section;
  section.handle = 0x431u;
  section.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
  section.m_state = 10u;
  section.m_flags = 11u;
  section.m_name = "Section DWG";
  section.m_vertDir = {0.0, 1.0, 0.0};
  section.m_topHeight = 12.0;
  section.m_bottomHeight = -1.0;
  section.m_verts = {{5.0, 6.0, 0.0}, {7.0, 8.0, 0.0}};
  section.m_sectionSettingsHandle = settings.handle;
  source.dwgAdvancedMetadata().addSectionObject(section);

  // DWG imports retain the bounded raw entity alongside the typed callback.
  // The typed route must replace that carrier rather than register/replay it
  // a second time.
  DRW_UnsupportedObject rawSection;
  rawSection.m_version = DRW::AC1027;
  rawSection.m_handle = section.handle;
  rawSection.m_isEntity = true;
  rawSection.m_isCustomClass = true;
  rawSection.m_recordName = "SECTIONOBJECT";
  rawSection.m_className = "AcDbSection";
  rawSection.m_rawBytes = {0u};
  rawSection.m_objectSize = 1u;
  rawSection.m_bodyBitSize = 8u;
  source.dwgAdvancedMetadata().addUnsupportedObject(rawSection);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(source, QString::fromStdString(out),
                              RS2::FormatDWG2013));
  }

  RS_Graphic reopened;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(out),
                              RS2::FormatDWG));
  }

  REQUIRE(reopened.dwgAdvancedMetadata().sectionObjects().size() == 1u);
  const auto& storedSection =
      reopened.dwgAdvancedMetadata().sectionObjects().front();
  CHECK(storedSection.handle == section.handle);
  CHECK(storedSection.parentHandle == DRW::NoHandle);
  CHECK(storedSection.name == section.m_name);
  CHECK(storedSection.state == section.m_state);
  CHECK(storedSection.flags == section.m_flags);
  CHECK(storedSection.sectionSettingsHandle == settings.handle);
  REQUIRE(reopened.dwgAdvancedMetadata().sections().size() == 1u);
  const auto& storedSettings =
      reopened.dwgAdvancedMetadata().sections().front();
  CHECK(storedSettings.handle == settings.handle);
  CHECK(storedSettings.currentType == settings.m_currentType);
  CHECK(storedSettings.typeCount == settings.m_typeCount);
  REQUIRE(storedSettings.types.size() == 1u);
  CHECK(storedSettings.types.front().m_type == dwgType.m_type);
  CHECK(storedSettings.types.front().m_generation == dwgType.m_generation);
  CHECK(storedSettings.types.front().m_sourceHandles
        == std::vector<std::uint32_t>{section.handle});
  CHECK(storedSettings.types.front().m_destinationFile
        == dwgType.m_destinationFile);
  REQUIRE(storedSettings.types.front().m_geometry.size() == 1u);
  CHECK(storedSettings.types.front().m_geometry.front().m_layer
        == dwgGeometry.m_layer);
  CHECK(storedSettings.types.front().m_geometry.front().m_hatchScale
        == dwgGeometry.m_hatchScale);

  std::filesystem::remove(out);
}

TEST_CASE("DWG export re-emits metadata TVDEVICEPROPERTIES objects",
          "[dwg][roundtrip][filter][tv-device-properties]") {
  ensureSettings();
  const std::string src = tmpFile("tv_device_properties_dwg_src.dxf");
  const std::string out = tmpFile("tv_device_properties_dwg.dwg");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  DRW_TvDeviceProperties properties;
  properties.handle = 0x650u;
  properties.parentHandle = 0x1Fu;
  properties.reactorHandles = {0x651u};
  properties.xDictHandle = 0x652u;
  properties.setDwgCommonObjectState(1, 0, false);
  properties.flags = 0x1234;
  properties.maxRegenThreads = 9;
  properties.useLutPalette = 1;
  properties.alternateHighlight = 0x112233445566ULL;
  properties.alternateHighlightColor = 0x223344556677ULL;
  properties.geometryShaderUsage = 0x334455667788ULL;
  properties.blendingMode = 4;
  properties.antialiasingLevel = 1.25;
  properties.valueBd2 = 2.5;
  graphic.dwgAdvancedMetadata().addTvDeviceProperties(properties);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDWG2013));
  }

  RS_Graphic reopened;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(out),
                              RS2::FormatDWG));
  }

  REQUIRE(reopened.dwgAdvancedMetadata().tvDeviceProperties().size() == 1);
  const auto& stored =
      reopened.dwgAdvancedMetadata().tvDeviceProperties().front();
  CHECK(stored.flags == 0x1234);
  CHECK(stored.maxRegenThreads == 9);
  CHECK(stored.useLutPalette == 1);
  CHECK(stored.alternateHighlight == 0x112233445566ULL);
  CHECK(stored.alternateHighlightColor == 0x223344556677ULL);
  CHECK(stored.geometryShaderUsage == 0x334455667788ULL);
  CHECK(stored.blendingMode == 4);
  CHECK(stored.antialiasingLevel == 1.25);
  CHECK(stored.valueBd2 == 2.5);
  CHECK(stored.parentHandle == DRW::DwgModelSpaceBlockRecordHandle);
  CHECK(stored.reactorHandles.empty());
  CHECK(stored.xDictHandle == DRW::NoHandle);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DWG export re-emits metadata VX objects",
          "[dwg][roundtrip][filter][vx]") {
  ensureSettings();
  const std::string src = tmpFile("vx_dwg_src.dxf");
  const std::string out = tmpFile("vx_dwg.dwg");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  DRW_VxControl control;
  control.handle = 0x660u;
  control.parentHandle = 0x1Fu;
  control.classVersion = 7u;
  control.flags = 3u;
  control.recordHandles = {0x662u, 0x663u};
  control.reactorHandles = {0x664u};
  control.xDictHandle = 0x665u;
  control.setDwgCommonObjectState(1, 0, false);
  REQUIRE(control.setDwgRawData({0xA5u, 0x3Cu}, 16, DRW::AC1027));
  graphic.dwgAdvancedMetadata().addVxControl(control);

  DRW_VxTableRecord record;
  record.handle = 0x666u;
  record.parentHandle = 0x1Fu;
  record.name = "VX-RECORD";
  record.classVersion = 9u;
  record.flags = 5u;
  record.reactorHandles = {0x668u};
  record.xDictHandle = 0x669u;
  record.setDwgCommonObjectState(1, 0, false);
  REQUIRE(record.setDwgRawData({0xF0u, 0x0Du}, 16, DRW::AC1027));
  graphic.dwgAdvancedMetadata().addVxTableRecord(record);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDWG2013));
  }

  RS_Graphic reopened;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(out),
                              RS2::FormatDWG));
  }

  REQUIRE(reopened.dwgAdvancedMetadata().vxControls().size() == 1);
  REQUIRE(reopened.dwgAdvancedMetadata().vxTableRecords().size() == 1);
  const auto& storedControl =
      reopened.dwgAdvancedMetadata().vxControls().front();
  const auto& storedRecord =
      reopened.dwgAdvancedMetadata().vxTableRecords().front();
  CHECK(storedControl.classVersion == 7u);
  CHECK(storedControl.flags == 3u);
  CHECK(storedControl.recordHandles.empty());
  CHECK(storedRecord.classVersion == 9u);
  CHECK(storedRecord.flags == 5u);
  CHECK(storedRecord.name == "VX-RECORD");
  CHECK(storedControl.parentHandle == DRW::DwgModelSpaceBlockRecordHandle);
  CHECK(storedRecord.parentHandle == DRW::DwgModelSpaceBlockRecordHandle);
  CHECK(storedControl.reactorHandles.empty());
  CHECK(storedRecord.reactorHandles.empty());
  CHECK(storedControl.xDictHandle == DRW::NoHandle);
  CHECK(storedRecord.xDictHandle == DRW::NoHandle);
  REQUIRE(storedControl.rawDataValid);
  CHECK(storedControl.rawDataBitSize >= 16u);
  REQUIRE(storedControl.rawData.size() >= 2u);
  CHECK(storedControl.rawData[0] == 0xA5u);
  CHECK(storedControl.rawData[1] == 0x3Cu);
  REQUIRE(storedRecord.rawDataValid);
  CHECK(storedRecord.rawDataBitSize >= 16u);
  REQUIRE(storedRecord.rawData.size() >= 2u);
  CHECK(storedRecord.rawData[0] == 0xF0u);
  CHECK(storedRecord.rawData[1] == 0x0Du);
  // The fixture has no replayable AcDb:AcDsPrototype_1b section.  Do not
  // advertise inline DataStorage bytes that were not emitted.
  CHECK_FALSE(storedControl.hasDsData);
  CHECK_FALSE(storedRecord.hasDsData);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export re-emits DWG-read NAVISWORKSMODEL entities",
          "[dxf][roundtrip][filter][navisworks]") {
  ensureSettings();
  const std::string src = tmpFile("navisworkssrc.dxf");
  const std::string out = tmpFile("navisworks.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n10.0\n21\n10.0\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  DRW_NavisworksModel model;
  model.handle = 0x310;
  model.parentHandle = 0x1F;
  model.flags = 0x8001;
  model.definitionHandle = 0x311;
  for (std::size_t i = 0; i < model.transform.size(); ++i)
    model.transform[i] = static_cast<double>(i + 1);
  model.unitFactor = 0.001;
  graphic.dwgAdvancedMetadata().addNavisworksModel(model);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "NAVISWORKSMODEL") == 1);
  CHECK(recordHasCode(out, "NAVISWORKSMODEL", "340"));
  CHECK(recordGroupValues(out, "NAVISWORKSMODEL", "40").size() == 17);

  RS_Graphic imported;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(imported, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  REQUIRE(imported.dwgAdvancedMetadata().navisworksModels().size() == 1);
  const auto& stored = imported.dwgAdvancedMetadata().navisworksModels()[0];
  CHECK(stored.parentHandle == DRW::DxfModelSpaceBlockRecordHandle);
  CHECK(stored.flags == model.flags);
  CHECK(stored.definitionHandle == model.definitionHandle);
  CHECK(stored.transform == model.transform);
  CHECK(stored.unitFactor == model.unitFactor);

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

TEST_CASE("DXF filter propagates typed writer rejection",
          "[dxf][roundtrip][filter][write-errors]") {
  ensureSettings();
  const std::string out = tmpFile("typed-write-failure.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  DRW_Shape invalid;
  invalid.handle = 0x301;
  invalid.m_insertionPoint.x = std::numeric_limits<double>::quiet_NaN();
  graphic.dwgAdvancedMetadata().addShape(invalid);

  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out),
                                RS2::FormatDXFRW));
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
      "0\nCLASS\n1\nweirdent\n2\nAcDbWeirdEntity\n3\nCUSTOM_APP\n"
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
    if (cls.recName == "weirdent" && cls.className == "AcDbWeirdEntity"
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
  CHECK(classRecordNames(out).count("weirdent") == 1);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF unused CLASS definitions survive filter round trip",
          "[dxf][roundtrip][filter][classes]") {
  ensureSettings();
  const std::string src = tmpFile("unused-class-src.dxf");
  const std::string out = tmpFile("unused-class-out.dxf");
  const std::string r12Out = tmpFile("unused-class-r12-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(r12Out);

  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nCLASSES\n"
            "0\nCLASS\n1\nUNUSEDCLASS\n2\nAcDbUnusedClass\n3\nCUSTOM_APP\n"
            "90\n4095\n91\n17\n280\n0\n281\n0\n"
            "0\nCLASS\n1\n\n2\nAcDbProxyOnly\n3\nCUSTOM_APP\n"
            "90\n4095\n91\n9\n280\n1\n281\n0\n"
            "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().dxfClasses().size() == 2);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(r12Out),
                                  RS2::FormatDXFRW12));
  }

  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  bool sawUnused = false;
  bool sawProxyOnly = false;
  for (const DRW_Class& cls : reloaded.dwgAdvancedMetadata().dxfClasses()) {
    if (cls.recName == "UNUSEDCLASS") {
      sawUnused = cls.className == "AcDbUnusedClass" && cls.instanceCount == 0;
    } else if (cls.recName.empty() && cls.className == "AcDbProxyOnly") {
      sawProxyOnly = cls.instanceCount == 0;
    }
  }
  CHECK(sawUnused);
  CHECK(sawProxyOnly);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(r12Out);
}

TEST_CASE("DXF export rejects conflicting case-insensitive CLASS metadata",
          "[dxf][roundtrip][filter][classes][safety]") {
  ensureSettings();
  const std::string src = tmpFile("class-conflict-src.dxf");
  const std::string out = tmpFile("class-conflict-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  const std::string dxf =
      "0\nSECTION\n2\nCLASSES\n"
      "0\nCLASS\n1\nWEIRDENT\n2\nAcDbFirst\n3\nCUSTOM_APP\n"
      "90\n4095\n91\n1\n280\n0\n281\n1\n"
      "0\nCLASS\n1\nweirdent\n2\nAcDbSecond\n3\nCUSTOM_APP\n"
      "90\n4095\n91\n1\n280\n0\n281\n1\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nWEIRDENT\n5\n7B\n8\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  writeText(src, dxf);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out),
                                RS2::FormatDXFRW));
  CHECK_FALSE(std::filesystem::exists(out));

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

TEST_CASE("DXF raw ENTITY remap updates typed GROUP members",
          "[dxf][roundtrip][filter][handles][raw]") {
  ensureSettings();
  const std::string src = tmpFile("raw-group-src.dxf");
  const std::string out = tmpFile("raw-group-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // 1F is owned by the generated Model_Space BLOCK_RECORD on export. The raw
  // entity must therefore be remapped, and the typed GROUP's source reference
  // must follow that remap instead of retaining the colliding 1F.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nWEIRDENT\n5\n1F\n8\n0\n10\n1\n20\n2\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\nACAD_GROUP\n350\nD\n"
            "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\n*RAW\n350\n90\n"
            "0\nGROUP\n5\n90\n330\nD\n100\nAcDbGroup\n300\nraw\n"
            "70\n1\n71\n1\n340\n1F\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().rawDxfEntities().size() == 1);
  REQUIRE(graphic.dwgAdvancedMetadata().groups().size() == 1);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  const std::vector<std::string> entityHandles =
      recordGroupValues(out, "WEIRDENT", "5");
  REQUIRE(entityHandles.size() == 1);
  CHECK(entityHandles.front() != "1F");
  CHECK(recordGroupValues(out, "GROUP", "340")
        == entityHandles);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF proxy raw carriers preserve wide self handles",
          "[dxf][roundtrip][filter][proxy][handles]") {
  ensureSettings();
  const std::string src = tmpFile("proxy-wide-handle-src.dxf");
  const std::string out = tmpFile("proxy-wide-handle-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // DXF code-5 handles use the same 64-bit-width lexeme space as DWG handles.
  // The typed proxy convenience field remains 32-bit, but the raw carrier must
  // accept and replay the complete source spelling.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nACAD_PROXY_ENTITY\n"
            "5\n1234567890ABCDEF\n"
            "330\n1\n"
            "100\nAcDbEntity\n8\n0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().rawDxfEntities().size() == 1);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(recordGroupValues(out, "ACAD_PROXY_ENTITY", "5")
        == std::vector<std::string>{"1234567890ABCDEF"});

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

TEST_CASE("DXF export rejects duplicate raw self-handles",
          "[dxf][roundtrip][filter][handles][safety]") {
  ensureSettings();
  const std::string out = tmpFile("duplicate_raw_handles.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto makeRaw = [](const char *value) {
    DRW_RawDxfObject raw;
    raw.name = "XRECORD";
    raw.handle = 0xAAu;
    raw.m_version = DRW::AC1021;
    raw.groups.emplace_back(5, std::string{value});
    raw.groups.emplace_back(100, std::string{"AcDbXrecord"});
    raw.groups.emplace_back(280, static_cast<std::int32_t>(1));
    return raw;
  };
  graphic.dwgAdvancedMetadata().addRawDxfObject(makeRaw("AA"));
  graphic.dwgAdvancedMetadata().addRawDxfObject(makeRaw("aa"));

  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out),
                                RS2::FormatDXFRW));
  std::filesystem::remove(out);
}

// A-4: the structural-collision handle remap must rewrite hard-pointer references
// carried in codes 390-399 and 480-481, not only 5/105/320-369/1005. A target raw
// object placed on a codec-fixed structural handle (0x14 == codec LTYPE
// CONTINUOUS) is remapped to a fresh handle; a second raw object referencing it
// via codes 390, 480, and 481 must have those refs rewritten to the new handle,
// not left dangling at the reused literal.
TEST_CASE("DXF export rewrites 390/480/481 hard-pointer refs to remapped raw objects",
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
      // Referencing raw object (no collision) points at 0x14 via 390, 480, and
      // 481. All three must follow the target's structural-collision remap.
      "0\nMATERIAL\n5\n40\n330\n12\n100\nAcDbMaterial\n1\nRefMat\n390\n14\n480\n14\n481\n14\n"
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
  // MATERIAL records' fields (handle 5, name 1, refs 390/480/481).
  std::ifstream in(out);
  std::string codeLine, valueLine;
  auto trim = [](std::string s) {
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  std::string targetHandle, ref390, ref480, ref481;
  std::string curType, curHandle, curName, cur390, cur480, cur481;
  auto flush = [&]() {
    if (curType == "MATERIAL") {
      if (curName == "TgtMat")
        targetHandle = curHandle;
      if (curName == "RefMat") {
        ref390 = cur390;
        ref480 = cur480;
        ref481 = cur481;
      }
    }
  curType = curHandle = curName = cur390 = cur480 = cur481 = "";
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trim(codeLine), v = trim(valueLine);
    if (c == "0") { flush(); curType = v; }
    else if (c == "5") curHandle = v;
    else if (c == "1") curName = v;
    else if (c == "390") cur390 = v;
    else if (c == "480") cur480 = v;
    else if (c == "481") cur481 = v;
  }
  flush();

  // The target was remapped off the colliding literal 0x14...
  REQUIRE_FALSE(targetHandle.empty());
  CHECK(targetHandle != "14");
  // ...and every reference was rewritten to the new handle (load-bearing: each
  // value was "14" before the remap and would otherwise be dangling).
  REQUIRE_FALSE(ref390.empty());
  REQUIRE_FALSE(ref480.empty());
  REQUIRE_FALSE(ref481.empty());
  CHECK(ref390 == targetHandle);
  CHECK(ref480 == targetHandle);
  CHECK(ref481 == targetHandle);
  CHECK(ref390 != "14");
  CHECK(ref480 != "14");
  CHECK(ref481 != "14");

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

TEST_CASE("DXF layer application groups survive filter round trip",
          "[dxf][roundtrip][filter][layer][application-groups]") {
  ensureSettings();
  const std::string src = tmpFile("layer_application_groups_src.dxf");
  const std::string out = tmpFile("layer_application_groups_out.dxf");
  const std::string dwgOut = tmpFile("layer_application_groups_out.dwg");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(dwgOut);

  writeText(src,
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLAYER\n70\n1\n"
            "0\nLAYER\n5\n10\n330\n2\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
            "2\nAPP_LAYER\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "102\n{CUSTOM_LAYER_APP\n310\nABCD\n"
            "102\n{NESTED\n481\n1F\n102\n}\n102\n}\n"
            "1001\nLAYER_XAPP\n1000\nlayer payload\n1040\n1.25\n1004\nA0B1\n"
            "102\n{ACAD_REACTORS\n330\nA0\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nB0\n102\n}\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.findLayer(QStringLiteral("APP_LAYER")) != nullptr);
  const auto* sourceLayer = graphic.dwgAdvancedMetadata()
                                .findLayerTableEntryByName("APP_LAYER");
  REQUIRE(sourceLayer != nullptr);
  CHECK(sourceLayer->appData.size() == 3);
  CHECK(sourceLayer->reactorHandles == std::vector<std::uint32_t>{0xA0u});
  CHECK(sourceLayer->xDictHandle == 0xB0u);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

#ifdef DWGSUPPORT
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(dwgOut),
                                  RS2::FormatDWG));
  }
#endif

  CHECK(recordGroupValues(out, "LAYER", "102")
        == std::vector<std::string>{"{CUSTOM_LAYER_APP", "{NESTED", "}", "}",
                                    "{ACAD_REACTORS", "}",
                                    "{ACAD_XDICTIONARY", "}"});
  CHECK(recordGroupValues(out, "LAYER", "310")
        == std::vector<std::string>{"ABCD"});
  CHECK(recordGroupValues(out, "LAYER", "481")
        == std::vector<std::string>{"1F"});
  CHECK(recordGroupValues(out, "LAYER", "1001")
        == std::vector<std::string>{"LAYER_XAPP"});
  CHECK(recordGroupValues(out, "LAYER", "1000")
        == std::vector<std::string>{"layer payload"});
  CHECK(recordGroupValues(out, "LAYER", "1004")
        == std::vector<std::string>{"A0B1"});
  const auto layerXdataReals = recordGroupValues(out, "LAYER", "1040");
  REQUIRE(layerXdataReals.size() == 1);
  CHECK(std::abs(std::stod(layerXdataReals.front()) - 1.25) < 1.0e-12);
  const auto layerOwnersAndReactors = recordGroupValues(out, "LAYER", "330");
  CHECK(std::count(layerOwnersAndReactors.cbegin(),
                   layerOwnersAndReactors.cend(), "A0") == 1);
  CHECK(recordGroupValues(out, "LAYER", "360")
        == std::vector<std::string>{"B0"});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF FIELD application groups block DWG export",
          "[dxf][roundtrip][filter][field][application-groups]") {
  ensureSettings();
  const std::string src = tmpFile("field_application_groups_src.dxf");
  const std::string dwgOut = tmpFile("field_application_groups_out.dwg");
  std::filesystem::remove(src);
  std::filesystem::remove(dwgOut);

  writeText(src,
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nFIELD\n5\nE0\n330\nC\n"
            "102\n{ACAD_REACTORS\n330\nE1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nE2\n102\n}\n"
            "102\n{CUSTOM\n1\npayload\n102\n}\n"
            "100\nAcDbField\n1\nAcExpr\n2\n1+1\n"
            "90\n0\n97\n0\n91\n0\n92\n0\n94\n0\n95\n0\n96\n0\n"
            "300\n\n93\n0\n"
            "7\nACFD_FIELD_VALUE\n93\n0\n90\n2\n140\n2\n"
            "94\n0\n300\n\n302\n\n304\nACVALUE_END\n"
            "301\n\n98\n0\n"
            "0\nFIELDLIST\n5\nE3\n330\nC\n"
            "102\n{ACAD_REACTORS\n330\nE4\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nE5\n102\n}\n"
            "102\n{CUSTOM\n1\npayload\n102\n}\n"
            "100\nAcDbIdSet\n90\n1\n290\n0\n330\nE0\n"
            "100\nAcDbFieldList\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().fields().size() == 1);
  REQUIRE(graphic.dwgAdvancedMetadata().fieldLists().size() == 1);
  CHECK(graphic.dwgAdvancedMetadata().fields().front().appData.size() == 3);
  CHECK(graphic.dwgAdvancedMetadata().fieldLists().front().appData.size() == 3);

#ifdef DWGSUPPORT
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(dwgOut),
                                  RS2::FormatDWG2013));
  }
#endif

  std::filesystem::remove(src);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF FIELD XDATA survives filter DXF and DWG round trips",
          "[dxf][roundtrip][filter][field][xdata]") {
  ensureSettings();
  const std::string src = tmpFile("field_xdata_src.dxf");
  const std::string out = tmpFile("field_xdata_out.dxf");
  const std::string dwgOut = tmpFile("field_xdata_out.dwg");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(dwgOut);

  writeText(src,
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nFIELD\n5\nE0\n330\nC\n"
            "100\nAcDbField\n1\nAcExpr\n2\n1+1\n"
            "90\n0\n97\n0\n91\n0\n92\n0\n94\n0\n95\n0\n96\n0\n"
            "300\n\n93\n0\n"
            "7\nACFD_FIELD_VALUE\n93\n0\n90\n2\n140\n2\n"
            "94\n0\n300\n\n302\n\n304\nACVALUE_END\n"
            "301\n\n98\n0\n"
            "1001\nFIELD_EED\n1000\nfield payload\n1070\n7\n"
            "0\nFIELDLIST\n5\nE3\n330\nC\n"
            "100\nAcDbIdSet\n90\n1\n290\n0\n330\nE0\n"
            "100\nAcDbFieldList\n"
            "1001\nFIELDLIST_EED\n1000\nfield list payload\n1070\n8\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().fields().size() == 1);
  REQUIRE(graphic.dwgAdvancedMetadata().fieldLists().size() == 1);
  CHECK(graphic.dwgAdvancedMetadata().fields().front().extData.size() == 3);
  CHECK(graphic.dwgAdvancedMetadata().fieldLists().front().extData.size() == 3);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(recordGroupValues(out, "FIELD", "1001")
        == std::vector<std::string>{"FIELD_EED"});
  CHECK(recordGroupValues(out, "FIELD", "1000")
        == std::vector<std::string>{"field payload"});
  CHECK(recordGroupValues(out, "FIELD", "1070")
        == std::vector<std::string>{"7"});
  CHECK(recordGroupValues(out, "FIELDLIST", "1001")
        == std::vector<std::string>{"FIELDLIST_EED"});
  CHECK(recordGroupValues(out, "FIELDLIST", "1000")
        == std::vector<std::string>{"field list payload"});
  CHECK(recordGroupValues(out, "FIELDLIST", "1070")
        == std::vector<std::string>{"8"});

#ifdef DWGSUPPORT
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwgOut),
                              RS2::FormatDWG2013));
  }
  RS_Graphic reopened;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(dwgOut),
                              RS2::FormatDWG));
  }
  REQUIRE(reopened.dwgAdvancedMetadata().fields().size() == 1);
  REQUIRE(reopened.dwgAdvancedMetadata().fieldLists().size() == 1);
  const auto& fieldExtData = reopened.dwgAdvancedMetadata().fields().front().extData;
  const auto& fieldListExtData =
      reopened.dwgAdvancedMetadata().fieldLists().front().extData;
  REQUIRE(fieldExtData.size() == 3);
  REQUIRE(fieldListExtData.size() == 3);
  CHECK(std::string(fieldExtData[0]->c_str()) == "FIELD_EED");
  CHECK(std::string(fieldExtData[1]->c_str()) == "field payload");
  CHECK(fieldExtData[2]->i_val() == 7);
  CHECK(std::string(fieldListExtData[0]->c_str()) == "FIELDLIST_EED");
  CHECK(std::string(fieldListExtData[1]->c_str()) == "field list payload");
  CHECK(fieldListExtData[2]->i_val() == 8);
#endif

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(dwgOut);
}

TEST_CASE("DXF construction layer retains one LibreCAD XDATA marker",
          "[dxf][roundtrip][filter][layer][xdata]") {
  ensureSettings();
  const std::string src = tmpFile("construction_layer_xdata_src.dxf");
  const std::string out = tmpFile("construction_layer_xdata_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLAYER\n70\n1\n"
            "0\nLAYER\n5\n10\n330\n2\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
            "2\nCONSTRUCTION\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "1001\nLibreCad\n1070\n1\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  const auto* layer = graphic.findLayer(QStringLiteral("CONSTRUCTION"));
  REQUIRE(layer != nullptr);
  CHECK(layer->isConstruction());

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(recordGroupValues(out, "LAYER", "1001")
        == std::vector<std::string>{"LibreCad"});
  CHECK(recordGroupValues(out, "LAYER", "1070")
        == std::vector<std::string>{"1"});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF layer marker classification does not duplicate malformed data",
          "[dxf][roundtrip][filter][layer][xdata][malformed]") {
  ensureSettings();
  const std::string src = tmpFile("malformed_layer_marker_src.dxf");
  const std::string out = tmpFile("malformed_layer_marker_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLAYER\n70\n3\n"
            "0\nLAYER\n5\n10\n330\n2\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
            "2\nBAD_DUPLICATE\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "1001\nLibreCad\n1070\n1\n"
            "1001\nACME_APP\n1000\nunrelated\n"
            "1001\nLibreCad\n1070\n1\n"
            "0\nLAYER\n5\n11\n330\n2\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
            "2\nBAD_TYPE\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "1001\nLibreCad\n1000\nwrong payload type\n"
            "0\nLAYER\n5\n12\n330\n2\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
            "2\nVALID\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "1001\nLibreCad\n1070\n1\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  const auto* duplicate = graphic.findLayer(QStringLiteral("BAD_DUPLICATE"));
  const auto* wrongType = graphic.findLayer(QStringLiteral("BAD_TYPE"));
  const auto* valid = graphic.findLayer(QStringLiteral("VALID"));
  REQUIRE(duplicate != nullptr);
  REQUIRE(wrongType != nullptr);
  REQUIRE(valid != nullptr);
  CHECK_FALSE(duplicate->isConstruction());
  CHECK_FALSE(wrongType->isConstruction());
  CHECK(valid->isConstruction());

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_DUPLICATE", "1001")
        == std::vector<std::string>{"LibreCad", "ACME_APP", "LibreCad"});
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_TYPE", "1001")
        == std::vector<std::string>{"LibreCad"});
  CHECK(namedRecordGroupValues(out, "LAYER", "VALID", "1001")
        == std::vector<std::string>{"LibreCad"});
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_DUPLICATE", "1070")
        == std::vector<std::string>{"1", "1"});
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_TYPE", "1070").empty());
  CHECK(namedRecordGroupValues(out, "LAYER", "VALID", "1070")
        == std::vector<std::string>{"1"});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF view-table application groups survive filter round trip",
          "[dxf][roundtrip][filter][view-tables][application-groups]") {
  ensureSettings();
  const std::string src = tmpFile("view_table_application_groups_src.dxf");
  const std::string out = tmpFile("view_table_application_groups_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nVPORT\n70\n1\n"
            "0\nVPORT\n5\n20\n330\n8\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbViewportTableRecord\n"
            "2\nSAVED_VPORT\n70\n0\n"
            "10\n0\n20\n0\n11\n1\n21\n1\n12\n0\n22\n0\n"
            "13\n0\n23\n0\n14\n10\n24\n10\n15\n10\n25\n10\n"
            "16\n0\n26\n0\n36\n1\n17\n0\n27\n0\n37\n0\n"
            "40\n10\n41\n1\n42\n50\n43\n0\n44\n0\n50\n0\n51\n0\n"
            "71\n0\n72\n100\n73\n1\n74\n3\n75\n0\n76\n0\n77\n0\n78\n0\n"
            "102\n{VPORT_APP\n310\nCAFE\n102\n{NESTED\n481\nA1\n102\n}\n102\n}\n"
            "1001\nVPORT_XAPP\n1000\nvport payload\n1040\n1.25\n1004\nA0B1\n"
            "102\n{ACAD_REACTORS\n330\nA2\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nA3\n102\n}\n"
            "0\nENDTAB\n"
            "0\nTABLE\n2\nUCS\n70\n1\n"
            "0\nUCS\n5\n21\n330\n7\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbUCSTableRecord\n"
            "2\nSAVED_UCS\n70\n0\n"
            "10\n0\n20\n0\n30\n0\n11\n1\n21\n0\n31\n0\n"
            "12\n0\n22\n1\n32\n0\n"
            "102\n{UCS_APP\n310\nBEEF\n102\n}\n"
            "1001\nUCS_XAPP\n1000\nucs payload\n1040\n2.5\n1004\nC2D3\n"
            "102\n{ACAD_REACTORS\n330\nB1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nB2\n102\n}\n"
            "0\nENDTAB\n"
            "0\nTABLE\n2\nVIEW\n70\n1\n"
            "0\nVIEW\n5\n22\n330\n6\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbViewTableRecord\n"
            "2\nSAVED_VIEW\n70\n0\n"
            "40\n10\n41\n10\n10\n0\n20\n0\n"
            "11\n0\n21\n0\n31\n1\n12\n0\n22\n0\n32\n0\n"
            "42\n50\n43\n0\n44\n0\n50\n0\n71\n0\n72\n0\n73\n0\n"
            "102\n{VIEW_APP\n310\nF00D\n102\n}\n"
            "1001\nVIEW_XAPP\n1000\nview payload\n1040\n3.75\n1004\nE4F5\n"
            "102\n{ACAD_REACTORS\n330\nC1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nC2\n102\n}\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().vportTableEntries().size() == 1);
  REQUIRE(graphic.dwgAdvancedMetadata().findUcsByName("SAVED_UCS") != nullptr);
  REQUIRE(graphic.dwgAdvancedMetadata().findViewByName("SAVED_VIEW") != nullptr);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(countRecords(out, "VPORT") == 2);
  CHECK(countRecords(out, "UCS") == 1);
  CHECK(countRecords(out, "VIEW") == 1);
  for (const auto& [record, chunk] :
       std::initializer_list<std::pair<const char*, const char*>>{
           {"VPORT", "CAFE"}, {"UCS", "BEEF"}, {"VIEW", "F00D"}}) {
    CHECK(recordGroupValues(out, record, "310")
          == std::vector<std::string>{chunk});
  }
  for (const auto& [record, appId, payload, binary, real] :
       std::initializer_list<std::tuple<const char*, const char*, const char*,
                                        const char*, double>>{
           {"VPORT", "VPORT_XAPP", "vport payload", "A0B1", 1.25},
           {"UCS", "UCS_XAPP", "ucs payload", "C2D3", 2.5},
           {"VIEW", "VIEW_XAPP", "view payload", "E4F5", 3.75}}) {
    CHECK(recordGroupValues(out, record, "1001")
          == std::vector<std::string>{appId});
    CHECK(recordGroupValues(out, record, "1000")
          == std::vector<std::string>{payload});
    CHECK(recordGroupValues(out, record, "1004")
          == std::vector<std::string>{binary});
    const auto reals = recordGroupValues(out, record, "1040");
    REQUIRE(reals.size() == 1);
    CHECK(std::abs(std::stod(reals.front()) - real) < 1.0e-12);
  }
  for (const auto& [record, reactor, dictionary] :
       std::initializer_list<std::tuple<const char*, const char*, const char*>>{
           {"VPORT", "A2", "A3"}, {"UCS", "B1", "B2"},
           {"VIEW", "C1", "C2"}}) {
    const auto reactors = recordGroupValues(out, record, "330");
    CHECK(std::count(reactors.cbegin(), reactors.cend(), reactor) == 1);
    CHECK(recordGroupValues(out, record, "360")
          == std::vector<std::string>{dictionary});
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF DIMSTYLE and APPID application groups survive filter round trip",
          "[dxf][roundtrip][filter][dimstyle][appid][application-groups]") {
  ensureSettings();
  const std::string src = tmpFile("dimstyle_appid_application_groups_src.dxf");
  const std::string out = tmpFile("dimstyle_appid_application_groups_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nAPPID\n70\n1\n"
            "0\nAPPID\n5\n30\n330\n9\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbRegAppTableRecord\n"
            "2\nSAVED_APPID\n70\n0\n"
            "102\n{APPID_APP\n310\nA991\n102\n}\n"
            "1001\nAPPID_XAPP\n1000\nappid payload\n1040\n1.5\n1004\nA1B2\n"
            "102\n{ACAD_REACTORS\n330\nA1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nA2\n102\n}\n"
            "0\nENDTAB\n"
            "0\nTABLE\n2\nDIMSTYLE\n70\n1\n"
            "0\nDIMSTYLE\n105\n31\n330\nA\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbDimStyleTableRecord\n"
            "2\nSAVED_DIMSTYLE\n70\n0\n"
            "102\n{DIMSTYLE_APP\n310\nD1A5\n102\n}\n"
            "1001\nDIMSTYLE_XAPP\n1000\ndimstyle payload\n1040\n2.5\n1004\nC3D4\n"
            "102\n{ACAD_REACTORS\n330\nD1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nD2\n102\n}\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata()
              .findDimStyleTableEntryByName("SAVED_DIMSTYLE") != nullptr);
  REQUIRE(graphic.dwgAdvancedMetadata()
              .findAppIdTableEntryByName("SAVED_APPID") != nullptr);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(recordGroupValues(out, "DIMSTYLE", "310")
        == std::vector<std::string>{"D1A5"});
  CHECK(recordGroupValues(out, "APPID", "310")
        == std::vector<std::string>{"A991"});
  for (const auto& [record, appId, payload, binary, real] :
       std::initializer_list<std::tuple<const char*, const char*, const char*,
                                        const char*, double>>{
           {"DIMSTYLE", "DIMSTYLE_XAPP", "dimstyle payload", "C3D4", 2.5},
           {"APPID", "APPID_XAPP", "appid payload", "A1B2", 1.5}}) {
    const auto appIds = recordGroupValues(out, record, "1001");
    CHECK(std::count(appIds.cbegin(), appIds.cend(), appId) == 1);
    const auto payloads = recordGroupValues(out, record, "1000");
    CHECK(std::count(payloads.cbegin(), payloads.cend(), payload) == 1);
    const auto binaries = recordGroupValues(out, record, "1004");
    CHECK(std::count(binaries.cbegin(), binaries.cend(), binary) == 1);
    const auto reals = recordGroupValues(out, record, "1040");
    const double expectedReal = real;
    CHECK(std::any_of(reals.cbegin(), reals.cend(),
                      [expectedReal](const auto& value) {
        return std::abs(std::stod(value) - expectedReal) < 1.0e-12;
    }));
  }
  for (const auto& [record, reactor, dictionary] :
       std::initializer_list<std::tuple<const char*, const char*, const char*>>{
           {"DIMSTYLE", "D1", "D2"}, {"APPID", "A1", "A2"}}) {
    const auto reactors = recordGroupValues(out, record, "330");
    CHECK(std::count(reactors.cbegin(), reactors.cend(), reactor) == 1);
    CHECK(recordGroupValues(out, record, "360")
          == std::vector<std::string>{dictionary});
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF R2010 DIMSTYLE text direction reaches the filter model",
          "[dxf][roundtrip][filter][dimstyle]") {
  ensureSettings();
  const std::string src = tmpFile("dimstyle_direction_r2010_src.dxf");
  const std::string out = tmpFile("dimstyle_direction_r2010_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1024\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nDIMSTYLE\n5\nA\n330\n0\n"
            "100\nAcDbSymbolTable\n70\n1\n100\nAcDbDimStyleTable\n71\n1\n"
            "0\nDIMSTYLE\n105\n27\n330\nA\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbDimStyleTableRecord\n"
            "2\nRTL\n70\n0\n295\n1\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  LC_DimStyle *style = graphic.getDimStyleByName("RTL");
  REQUIRE(style != nullptr);
  CHECK(style->text()->readingDirection() == LC_DimStyle::Text::RIGHT_TO_LEFT);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW2018));
  }
  CHECK(namedRecordGroupValues(out, "DIMSTYLE", "RTL", "295")
        == std::vector<std::string>{"1"});
  CHECK(namedRecordGroupValues(out, "DIMSTYLE", "RTL", "292").empty());
  CHECK(namedRecordGroupValues(out, "DIMSTYLE", "RTL", "294").empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF R2010 DSTYLE text direction survives a dimension round trip",
          "[dxf][roundtrip][filter][dimension][dimstyle]") {
  ensureSettings();
  const std::string out = tmpFile("dimension_dstyle_direction_r2010.dxf");
  const std::string legacyOut = tmpFile("dimension_dstyle_direction_r2007.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(legacyOut);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  RS_DimensionData data;
  data.definitionPoint = RS_Vector(5.0, 3.0);
  data.middleOfText = RS_Vector(5.0, 3.0);
  data.style = "Standard";
  auto* dimension = new RS_DimAligned(
      &graphic, data,
      RS_DimAlignedData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
  LC_DimStyle override;
  override.text()->setReadingDirection(LC_DimStyle::Text::RIGHT_TO_LEFT);
  dimension->setDimStyleOverride(&override);
  graphic.addEntity(dimension);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW2018));
  }
  CHECK(recordGroupValues(out, "DIMENSION", "1070")
        == std::vector<std::string>{"295", "1"});

  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  auto* reloadedDimension = dynamic_cast<RS_Dimension*>(reloaded.firstEntity());
  REQUIRE(reloadedDimension != nullptr);
  LC_DimStyle* reloadedOverride = reloadedDimension->getDimStyleOverride();
  REQUIRE(reloadedOverride != nullptr);
  CHECK(reloadedOverride->text()->readingDirection()
        == LC_DimStyle::Text::RIGHT_TO_LEFT);

  const auto checkDstyleInput = [&](const char *name,
                                    const std::string &directionCode,
                                    const std::string &valueCode,
                                    const std::string &value,
                                    bool closeGroup,
                                    bool expectedOverride,
                                    bool expectRtl) {
    const std::string input = tmpFile(name);
    std::filesystem::remove(input);
    const std::string variant = dstyleDirectionVariant(
        out, directionCode, valueCode, value, closeGroup);
    REQUIRE_FALSE(variant.empty());
    writeText(input, variant);

    RS_Graphic imported;
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(imported, QString::fromStdString(input),
                                RS2::FormatDXFRW));
    }
    auto* importedDimension = dynamic_cast<RS_Dimension*>(imported.firstEntity());
    REQUIRE(importedDimension != nullptr);
    LC_DimStyle* importedOverride = importedDimension->getDimStyleOverride();
    if (expectedOverride) {
      REQUIRE(importedOverride != nullptr);
      CHECK(importedOverride->text()->readingDirection()
            == (expectRtl ? LC_DimStyle::Text::RIGHT_TO_LEFT
                          : LC_DimStyle::Text::LEFT_TO_RIGHT));
    } else {
      CHECK(importedOverride == nullptr);
    }

    std::filesystem::remove(input);
  };

  for (const char *alias : {"292", "294", "295"}) {
    const std::string name = std::string("dimension_dstyle_") + alias + ".dxf";
    checkDstyleInput(name.c_str(), alias, "1070", "1", true, true, true);
  }
  checkDstyleInput("dimension_dstyle_invalid_negative.dxf", "295", "1070",
                   "-1", true, false, false);
  checkDstyleInput("dimension_dstyle_invalid_bit.dxf", "295", "1070", "2",
                   true, false, false);
  checkDstyleInput("dimension_dstyle_invalid_type.dxf", "295", "1040", "1.0",
                   true, false, false);
  checkDstyleInput("dimension_dstyle_unclosed.dxf", "295", "1070", "1",
                   false, false, false);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(legacyOut),
                              RS2::FormatDXFRW));
  }
  CHECK(recordGroupValues(legacyOut, "DIMENSION", "1070").empty());

  std::filesystem::remove(out);
  std::filesystem::remove(legacyOut);
}

TEST_CASE("DXF unused LTYPE and STYLE application groups survive filter round trip",
          "[dxf][roundtrip][filter][ltype][style][application-groups]") {
  ensureSettings();
  const std::string src = tmpFile("ltype_style_application_groups_src.dxf");
  const std::string out = tmpFile("ltype_style_application_groups_out.dxf");
  const std::string dwgOut = tmpFile("ltype_style_application_groups_out.dwg");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(dwgOut);

  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLTYPE\n70\n1\n"
            "0\nLTYPE\n5\n40\n330\n5\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
            "2\nSAVED_LTYPE\n70\n0\n3\nSaved linetype\n72\n65\n73\n0\n40\n0\n"
            "102\n{LTYPE_APP\n310\nCAFE\n102\n}\n"
            "102\n{ACAD_REACTORS\n330\nA1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nA2\n102\n}\n"
            "0\nENDTAB\n"
            "0\nTABLE\n2\nSTYLE\n70\n1\n"
            "0\nSTYLE\n5\n41\n330\n3\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbTextStyleTableRecord\n"
            "2\nSAVED_STYLE\n70\n0\n40\n0\n41\n1\n50\n0\n71\n0\n42\n1\n"
            "3\ntxt\n4\n\n"
            "102\n{STYLE_APP\n310\nBEEF\n102\n}\n"
            "102\n{ACAD_REACTORS\n330\nB1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nB2\n102\n}\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata()
              .findLineTypeTableEntryByName("SAVED_LTYPE") != nullptr);
  REQUIRE(graphic.dwgAdvancedMetadata()
              .findTextStyleTableEntryByName("SAVED_STYLE") != nullptr);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

#ifdef DWGSUPPORT
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(dwgOut),
                                  RS2::FormatDWG));
  }
#endif

  for (const auto& [record, chunk, reactor, dictionary] :
       std::initializer_list<std::tuple<const char*, const char*, const char*,
                                        const char*>>{
           {"LTYPE", "CAFE", "A1", "A2"},
           {"STYLE", "BEEF", "B1", "B2"}}) {
    CHECK(recordGroupValues(out, record, "310")
          == std::vector<std::string>{chunk});
    const auto reactors = recordGroupValues(out, record, "330");
    CHECK(std::count(reactors.cbegin(), reactors.cend(), reactor) == 1);
    CHECK(recordGroupValues(out, record, "360")
          == std::vector<std::string>{dictionary});
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(dwgOut);
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

TEST_CASE("DXF export rejects malformed typed conversion sidecars",
          "[dxf][typed-conversion][safety]") {
  ensureSettings();
  const std::string out = tmpFile("invalid_typed_conversion.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto* point = new RS_Point(&graphic, RS_Vector(1.0, 2.0));
  point->setDrwExtData({
      std::make_shared<DRW_Variant>(
          1001, std::string("LibreCAD_POINT_EXTRUSION")),
      std::make_shared<DRW_Variant>(1010, DRW_Coord(1.0, 2.0, 3.0)),
      std::make_shared<DRW_Variant>(1011, DRW_Coord(0.0, 0.0, 1.0)),
      std::make_shared<DRW_Variant>(1040, 0.0)});
  graphic.addEntity(point);

  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out),
                                RS2::FormatDXFRW));
  CHECK(countRecords(out, "POINT") == 0);

  std::filesystem::remove(out);
}
