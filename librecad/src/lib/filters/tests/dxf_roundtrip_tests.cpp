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
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <QCoreApplication>

#include "drw_base.h"
#include "lc_dimstyle.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_linetypenames.h"
#include "lc_mleader.h"
#include "rs_dimaligned.h"
#include "rs_dimension.h"
#include "rs_fileio.h"
#include "rs_filterdxf1.h"
#include "rs_filterdxfrw.h"
#include "rs_filterjww.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_entity.h"
#include "rs_block.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_linetypepattern.h"
#include "rs_pen.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_settings.h"
#include "rs_spline.h"

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

// Returns the group-`code` values of the LTYPE table record named `ltypeName`.
// libdxfrw writes the name (2) before the pattern groups (3/73/40/49), so the
// name is matched first and the values are collected until the next record.
std::vector<std::string> ltypeRecordGroupValues(const std::string &path,
                                                const std::string &ltypeName,
                                                const std::string &code) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::vector<std::string> values;
  bool inLtype = false;
  bool nameMatched = false;
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string groupCode = trimDxfToken(codeLine);
    const std::string value = trimDxfToken(valueLine);
    if (groupCode == "0") {
      inLtype = value == "LTYPE";
      nameMatched = false;
    } else if (inLtype && groupCode == "2") {
      nameMatched = value == ltypeName;
    } else if (inLtype && nameMatched && groupCode == code) {
      values.push_back(value);
    }
  }
  return values;
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

// The 330 values inside the {ACAD_REACTORS groups of every `recordName`
// record, one list per record, in file order.
std::vector<std::vector<std::string>> recordReactors(const std::string &path,
                                                     const std::string &recordName) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::vector<std::vector<std::string>> reactors;
  bool inRecord = false;
  bool inReactors = false;
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trimDxfToken(codeLine), v = trimDxfToken(valueLine);
    if (c == "0") {
      inRecord = v == recordName;
      inReactors = false;
      if (inRecord)
        reactors.emplace_back();
    } else if (inRecord && c == "102") {
      inReactors = v == "{ACAD_REACTORS";
    } else if (inRecord && inReactors && c == "330") {
      reactors.back().push_back(v);
    }
  }
  return reactors;
}

// Owner and reactor (330), soft/hard pointer (340/350/360) and plot style
// (390) references that name no handle (5/105) of the file.
std::vector<std::string> danglingReferences(const std::string &path) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::set<std::string> defined;
  std::vector<std::pair<std::string, std::string>> references;
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string c = trimDxfToken(codeLine), v = trimDxfToken(valueLine);
    if (c == "5" || c == "105")
      defined.insert(v);
    else if ((c == "330" || c == "340" || c == "350" || c == "360" ||
              c == "390") && v != "0")
      references.emplace_back(c, v);
  }
  std::vector<std::string> dangling;
  for (const auto &[code, handle] : references) {
    if (defined.count(handle) == 0)
      dangling.push_back(code + " " + handle);
  }
  return dangling;
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

TEST_CASE("DXF round-trip keeps frozen-layer entities visible after thaw",
          "[dxf][roundtrip][filter][layer][issue2913]") {
  ensureSettings();
  const std::string src = tmpFile("frozen_layer_src.dxf");
  const std::string out = tmpFile("frozen_layer_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(
      src,
      "0\nSECTION\n2\nHEADER\n"
      "9\n$CLAYER\n2\nLAYER2\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLAYER\n70\n3\n"
      "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
      "0\nLAYER\n2\nLAYER1\n70\n1\n62\n1\n6\nCONTINUOUS\n"
      "0\nLAYER\n2\nLAYER2\n70\n0\n62\n3\n6\nCONTINUOUS\n"
      "0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\nLAYER1\n"
      "10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
      "0\nLINE\n8\nLAYER2\n60\n1\n"
      "10\n20.0\n20\n0.0\n11\n30.0\n21\n0.0\n"
      "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  RS_Layer *layer = reloaded.findLayer(QStringLiteral("LAYER1"));
  REQUIRE(layer != nullptr);
  CHECK(layer->isFrozen());

  auto *line = dynamic_cast<RS_Line *>(reloaded.firstEntity());
  REQUIRE(line != nullptr);
  CHECK(line->getFlag(RS2::FlagVisible));
  CHECK_FALSE(line->isVisible());

  auto *hiddenLine = dynamic_cast<RS_Line *>(reloaded.nextEntity());
  REQUIRE(hiddenLine != nullptr);
  CHECK_FALSE(hiddenLine->getFlag(RS2::FlagVisible));
  CHECK_FALSE(hiddenLine->isVisible());

  layer->freeze(false);
  CHECK(line->isVisible());
  CHECK_FALSE(hiddenLine->isVisible());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF INSERT attribute visibility combines common and attribute flags",
          "[dxf][roundtrip][filter][attrib][visibility]") {
  ensureSettings();
  const std::string src = tmpFile("attrib_visibility_src.dxf");
  const std::string out = tmpFile("attrib_visibility_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  writeText(
      src,
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n8\n0\n2\nTITLEBLK\n70\n0\n10\n0\n20\n0\n3\nTITLEBLK\n"
      "0\nENDBLK\n8\n0\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nINSERT\n8\n0\n2\nTITLEBLK\n10\n0\n20\n0\n66\n1\n"
      "0\nATTRIB\n8\n0\n10\n1\n20\n1\n40\n1\n1\nCommonHidden\n"
      "2\nCOMMON_HIDDEN\n60\n1\n70\n0\n"
      "0\nATTRIB\n8\n0\n10\n1\n20\n2\n40\n1\n1\nFlagHidden\n"
      "2\nFLAG_HIDDEN\n70\n1\n"
      "0\nATTRIB\n8\n0\n10\n1\n20\n3\n40\n1\n1\nVisible\n"
      "2\nVISIBLE\n70\n0\n"
      "0\nATTRIB\n8\n0\n100\nAcDbText\n10\n1\n20\n4\n40\n1\n"
      "100\nAcDbAttribute\n2\nMTEXT_HIDDEN\n70\n0\n"
      "100\nEmbedded Object\n10\n1\n20\n4\n40\n1\n71\n1\n72\n1\n"
      "1\nEmbedded\n60\n1\n"
      "0\nSEQEND\n8\n0\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  const auto checkVisibility = [](RS_Graphic &drawing) {
    int visibleAttributes = 0;
    int hiddenAttributes = 0;
    for (int index = 0; index < drawing.count(); ++index) {
      RS_Entity *entity = drawing.entityAt(index);
      if (entity == nullptr ||
          (entity->rtti() != RS2::EntityText &&
           entity->rtti() != RS2::EntityMText))
        continue;
      if (entity->getFlag(RS2::FlagVisible))
        ++visibleAttributes;
      else
        ++hiddenAttributes;
    }
    CHECK(visibleAttributes == 1);
    CHECK(hiddenAttributes == 3);
  };
  checkVisibility(graphic);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  checkVisibility(reloaded);

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

TEST_CASE("A 3DLINE saved as R12 falls back to a plain LINE instead of being dropped",
          "[dxf][roundtrip][filter][r12]") {
  ensureSettings();
  const std::string src = tmpFile("threedline_r12_src.dxf");
  const std::string out = tmpFile("threedline_r12_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // 3DLINE has no R12 record (dxfRW::write3DLine() leaves it out below
  // AC1015). The source line must still reach the file, as a plain LINE.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\n3DLINE\n8\n0\n10\n10.0\n20\n11.0\n30\n12.0\n"
            "11\n13.0\n21\n14.0\n31\n15.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW12));
  }

  // The fallback LINE keeps the endpoints' X/Y; RS_Line has no field of its
  // own for the Z each point had as a 3DLINE (that survives only in the
  // type-fidelity XDATA a *native* 3DLINE write reads back), so losing it
  // here, on a downgrade to a version that cannot hold a 3DLINE, is the
  // best this fallback can do. The point is that the line itself is not
  // silently dropped.
  CHECK(countRecords(out, "3DLINE") == 0);
  REQUIRE(countRecords(out, "LINE") == 1);
  CHECK(firstGroupValueAsDouble(out, "LINE", "10") == 10.0);
  CHECK(firstGroupValueAsDouble(out, "LINE", "20") == 11.0);
  CHECK(firstGroupValueAsDouble(out, "LINE", "11") == 13.0);
  CHECK(firstGroupValueAsDouble(out, "LINE", "21") == 14.0);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
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
    // R12 has no CLASSES section: the classes are left out, the save is not
    RS_FilterDXFRW filter;
    CHECK(filter.fileExport(graphic, QString::fromStdString(r12Out),
                            RS2::FormatDXFRW12));
    CHECK_FALSE(containsLine(r12Out, "CLASSES"));
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

TEST_CASE("DXF export keeps a GROUP's handle for its members' reactors",
          "[dxf][roundtrip][filter][handles][groups]") {
  ensureSettings();
  const std::string src = tmpFile("group-reactors-src.dxf");
  const std::string out = tmpFile("group-reactors-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // The members' reactors are written with the entities, before OBJECTS; the
  // GROUP used to get a new handle there, so they named nothing.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA1\n330\n1F\n102\n{ACAD_REACTORS\n330\n90\n102\n}\n"
            "100\nAcDbEntity\n8\n0\n100\nAcDbLine\n10\n0\n20\n0\n30\n0\n"
            "11\n10\n21\n0\n31\n0\n"
            "0\nLINE\n5\nA2\n330\n1F\n102\n{ACAD_REACTORS\n330\n90\n102\n}\n"
            "100\nAcDbEntity\n8\n0\n100\nAcDbLine\n10\n0\n20\n5\n30\n0\n"
            "11\n10\n21\n5\n31\n0\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "3\nACAD_GROUP\n350\nD\n"
            "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\nPAIR\n350\n90\n"
            "0\nGROUP\n5\n90\n330\nD\n100\nAcDbGroup\n300\npair\n70\n0\n71\n1\n"
            "340\nA1\n340\nA2\n"
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

  CHECK(recordGroupValues(out, "GROUP", "5") == std::vector<std::string>{"90"});
  CHECK(recordGroupValues(out, "GROUP", "340")
        == recordGroupValues(out, "LINE", "5"));
  CHECK(recordReactors(out, "LINE")
        == std::vector<std::vector<std::string>>{{"90"}, {"90"}});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export points an entity's reactors at the entities they name",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("entity-reactors-src.dxf");
  const std::string out = tmpFile("entity-reactors-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // An associative HATCH is a reactor of its boundary; both entities are
  // written under fresh handles, the HATCH after the polyline.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLWPOLYLINE\n5\nB0\n102\n{ACAD_REACTORS\n330\nB1\n102\n}\n"
            "100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n90\n4\n70\n1\n"
            "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
            "0\nHATCH\n5\nB1\n100\nAcDbEntity\n8\n0\n100\nAcDbHatch\n"
            "10\n0\n20\n0\n30\n0\n210\n0\n220\n0\n230\n1\n2\nSOLID\n70\n1\n71\n1\n"
            "91\n1\n92\n3\n72\n0\n73\n1\n93\n4\n"
            "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
            "97\n1\n330\nB0\n75\n0\n76\n1\n98\n0\n"
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

  const auto hatch = recordGroupValues(out, "HATCH", "5");
  REQUIRE(hatch.size() == 1);
  CHECK(hatch.front() != "B1");
  CHECK(recordReactors(out, "LWPOLYLINE")
        == std::vector<std::vector<std::string>>{{hatch.front()}});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export resolves reactors a table record keeps as application data",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("dimstyle-reactors-src.dxf");
  const std::string out = tmpFile("dimstyle-reactors-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // A DIMSTYLE lists the dimensions using it as reactors, kept verbatim as
  // an application-data group; any entity serves as the target here.
  writeText(src,
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nDIMSTYLE\n5\nA\n70\n1\n"
            "0\nDIMSTYLE\n105\n30\n102\n{ACAD_REACTORS\n330\nA1\n102\n}\n330\nA\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbDimStyleTableRecord\n2\nCUSTOM\n70\n0\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA1\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
            "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
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

  const auto line = recordGroupValues(out, "LINE", "5");
  REQUIRE(line.size() == 1);
  const auto dimStyles = recordReactors(out, "DIMSTYLE");
  const auto custom = std::find_if(dimStyles.cbegin(), dimStyles.cend(),
                                   [](const auto &reactors) { return !reactors.empty(); });
  REQUIRE(custom != dimStyles.cend());
  CHECK(*custom == std::vector<std::string>{line.front()});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export drops reactors to a GROUP whose handle it cannot keep",
          "[dxf][roundtrip][filter][handles][groups]") {
  ensureSettings();
  const std::string src = tmpFile("group-structural-src.dxf");
  const std::string out = tmpFile("group-structural-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // 1F is the Model_Space BLOCK_RECORD the codec writes: the GROUP must move,
  // and a reactor naming 1F would name that record instead.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA1\n102\n{ACAD_REACTORS\n330\n1F\n102\n}\n"
            "100\nAcDbEntity\n8\n0\n100\nAcDbLine\n10\n0\n20\n0\n30\n0\n"
            "11\n10\n21\n0\n31\n0\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "3\nACAD_GROUP\n350\nD\n"
            "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\nONE\n350\n1F\n"
            "0\nGROUP\n5\n1F\n330\nD\n100\nAcDbGroup\n300\none\n70\n0\n71\n1\n"
            "340\nA1\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.dwgAdvancedMetadata().groups().size() == 1);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  const auto groupHandles = recordGroupValues(out, "GROUP", "5");
  REQUIRE(groupHandles.size() == 1);
  CHECK(groupHandles.front() != "1F");
  CHECK(recordGroupValues(out, "GROUP", "340")
        == recordGroupValues(out, "LINE", "5"));
  CHECK(recordReactors(out, "LINE")
        == std::vector<std::vector<std::string>>{{}});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export follows an extension dictionary moved off a structural handle",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("xdict-structural-src.dxf");
  const std::string out = tmpFile("xdict-structural-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // 1C is a BLOCK_RECORD handle the codec writes, so the dictionary is
  // re-emitted under a fresh handle; the LINE's 360 must follow it.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA1\n102\n{ACAD_XDICTIONARY\n360\n1C\n102\n}\n"
            "100\nAcDbEntity\n8\n0\n100\nAcDbLine\n10\n0\n20\n0\n30\n0\n"
            "11\n10\n21\n0\n31\n0\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "0\nDICTIONARY\n5\n1C\n330\nA1\n100\nAcDbDictionary\n281\n1\n"
            "3\nMYDATA\n350\nA6\n"
            "0\nXRECORD\n5\nA6\n330\n1C\n100\nAcDbXrecord\n280\n1\n1\nhello\n"
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

  const auto xdict = recordGroupValues(out, "LINE", "360");
  REQUIRE(xdict.size() == 1);
  CHECK(xdict.front() != "1C");
  const DxfRecordGroups dictionary =
      recordGroupsWithValue(out, "DICTIONARY", "5", xdict.front());
  REQUIRE_FALSE(dictionary.empty());
  const auto owner = std::find_if(dictionary.cbegin(), dictionary.cend(),
                                  [](const auto &group) { return group.first == "330"; });
  REQUIRE(owner != dictionary.cend());
  CHECK(owner->second == recordGroupValues(out, "LINE", "5").front());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

namespace {
// An R2000 drawing whose dictionary and ACME_THING LibreCAD keeps only as
// raw DXF; the MATERIAL is kept both raw and typed.
const char *const kR2000WithRawMaterial =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
    "0\nSECTION\n2\nCLASSES\n"
    "0\nCLASS\n1\nACME_THING\n2\nAcmeThing\n3\nACME\n90\n0\n280\n0\n281\n0\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\nA1\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
    "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nOBJECTS\n"
    "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
    "3\nACAD_MATERIAL\n350\n80\n"
    "0\nDICTIONARY\n5\n80\n330\nC\n100\nAcDbDictionary\n281\n1\n"
    "3\nMINE\n350\n90\n3\nTHING\n350\n91\n"
    "0\nMATERIAL\n5\n90\n330\n80\n100\nAcDbMaterial\n1\nMINE\n"
    "0\nACME_THING\n5\n91\n330\n80\n100\nAcmeThing\n1\nkept raw\n"
    "0\nENDSEC\n0\nEOF\n";
} // namespace

TEST_CASE("DXF import records its version and saves back in it",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  // The version the file declares, not the one it is decoded as (R2000 for
  // R13 to R2004, R2007 for R2007 and later).
  const auto [acadVer, format] = GENERATE(
      std::pair<std::string, RS2::FormatType>{"AC1014", RS2::FormatDXFRW14},
      std::pair<std::string, RS2::FormatType>{"AC1015", RS2::FormatDXFRW2000},
      std::pair<std::string, RS2::FormatType>{"AC1018", RS2::FormatDXFRW2004},
      std::pair<std::string, RS2::FormatType>{"AC1021", RS2::FormatDXFRW},
      std::pair<std::string, RS2::FormatType>{"AC1024", RS2::FormatDXFRW2010},
      std::pair<std::string, RS2::FormatType>{"AC1027", RS2::FormatDXFRW2013},
      std::pair<std::string, RS2::FormatType>{"AC1032", RS2::FormatDXFRW2018});
  CAPTURE(acadVer);
  const std::string src = tmpFile(("version-" + acadVer + "-src.dxf").c_str());
  const std::string out = tmpFile(("version-" + acadVer + "-out.dxf").c_str());
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::string text = kR2000WithRawMaterial;
  const std::string r2000 = "AC1015";
  text.replace(text.find(r2000), r2000.size(), acadVer);
  if (acadVer >= "AC1018") {
    // From R2004 on, a CLASS also counts its instances (91).
    const std::string proxyFlags = "90\n0\n280";
    text.replace(text.find(proxyFlags), proxyFlags.size(), "90\n0\n91\n1\n280");
  }
  writeText(src, text);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.getFormatType() == format);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              graphic.getFormatType()));
  }
  CHECK(recordGroupValues(out, "MATERIAL", "5") == std::vector<std::string>{"90"});
  CHECK(recordGroupValues(out, "ACME_THING", "5") == std::vector<std::string>{"91"});
  CHECK(danglingReferences(out).empty());
  {
    // closed before the file is removed below (Windows keeps open files)
    std::ifstream written(out);
    const std::string saved((std::istreambuf_iterator<char>(written)),
                            std::istreambuf_iterator<char>());
    CHECK(saved.find("$ACADVER\n  1\n" + acadVer) != std::string::npos);
  }

  RS_Graphic reread;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reread, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(reread.getFormatType() == format);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

namespace {
std::string dxfLayoutRecord(const char *handle, const char *name, int tab,
                            const char *blockRecord) {
  return std::string("0\nLAYOUT\n5\n") + handle + "\n330\n1A\n" +
         "100\nAcDbPlotSettings\n1\n\n2\nnone_device\n4\n\n6\n\n"
         "40\n0\n41\n0\n42\n0\n43\n0\n44\n0\n45\n0\n"
         "70\n688\n72\n0\n73\n0\n74\n5\n7\n\n75\n16\n"
         "100\nAcDbLayout\n1\n" + name + "\n70\n1\n71\n" + std::to_string(tab) +
         "\n10\n0\n20\n0\n11\n12\n21\n9\n12\n0\n22\n0\n32\n0\n76\n0\n146\n0\n"
         "330\n" + blockRecord + "\n";
}

std::string dxfBlockRecord(const char *handle, const char *name) {
  return std::string("0\nBLOCK_RECORD\n5\n") + handle +
         "\n330\n1\n100\nAcDbSymbolTableRecord\n100\nAcDbBlockTableRecord\n2\n" +
         name + "\n";
}

std::string dxfEmptyBlock(const char *handle, const char *endHandle,
                          const char *owner, const char *name) {
  return std::string("0\nBLOCK\n5\n") + handle + "\n330\n" + owner +
         "\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockBegin\n2\n" + name +
         "\n70\n0\n10\n0\n20\n0\n30\n0\n3\n" + name + "\n1\n\n" +
         "0\nENDBLK\n5\n" + endHandle + "\n330\n" + owner +
         "\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockEnd\n";
}

// The name of the block record the AcDbLayout part of layout @p name names.
std::string layoutBlockRecordName(const std::string &path,
                                  const std::string &name) {
  const DxfRecordGroups layout = recordGroupsWithValue(path, "LAYOUT", "1", name);
  std::string blockRecord;
  bool inLayout = false;
  for (const auto &[code, value] : layout) {
    if (code == "100")
      inLayout = value == "AcDbLayout";
    else if (inLayout && code == "330")
      blockRecord = value;
  }
  if (blockRecord.empty())
    return {};
  for (const auto &[code, value] :
       recordGroupsWithValue(path, "BLOCK_RECORD", "5", blockRecord))
    if (code == "2")
      return value;
  return "?" + blockRecord;
}
} // namespace

TEST_CASE("DXF layouts name the block records of their spaces",
          "[dxf][roundtrip][filter][handles][layout]") {
  ensureSettings();
  const std::string src = tmpFile("layout-spaces-src.dxf");
  const std::string out = tmpFile("layout-spaces-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // The source's space block records have handles the save gives other
  // records: its *Model_Space is 1E, where the save writes *Paper_Space, and
  // the reverse. Layout2's *Paper_Space0 has no LibreCAD block.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nBLOCK_RECORD\n5\n1\n330\n0\n100\nAcDbSymbolTable\n70\n3\n" +
                dxfBlockRecord("1E", "*Model_Space") +
                dxfBlockRecord("1F", "*Paper_Space") +
                dxfBlockRecord("2C", "*Paper_Space0") +
                "0\nENDTAB\n0\nENDSEC\n"
                "0\nSECTION\n2\nBLOCKS\n" +
                dxfEmptyBlock("2D", "2E", "1E", "*Model_Space") +
                dxfEmptyBlock("2F", "30", "1F", "*Paper_Space") +
                dxfEmptyBlock("31", "32", "2C", "*Paper_Space0") +
                "0\nENDSEC\n"
                "0\nSECTION\n2\nENTITIES\n"
                "0\nLINE\n5\nA1\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
                "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
                "0\nENDSEC\n"
                "0\nSECTION\n2\nOBJECTS\n"
                "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
                "3\nACAD_LAYOUT\n350\n1A\n"
                "0\nDICTIONARY\n5\n1A\n330\nC\n100\nAcDbDictionary\n281\n1\n"
                "3\nLayout1\n350\n3B\n3\nLayout2\n350\n3C\n3\nModel\n350\n3A\n" +
                dxfLayoutRecord("3A", "Model", 0, "1E") +
                dxfLayoutRecord("3B", "Layout1", 1, "1F") +
                dxfLayoutRecord("3C", "Layout2", 2, "2C") +
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
                              graphic.getFormatType()));
  }

  CHECK(layoutBlockRecordName(out, "Model") == "*Model_Space");
  CHECK(layoutBlockRecordName(out, "Layout1") == "*Paper_Space");
  CHECK(layoutBlockRecordName(out, "Layout2") == "*Paper_Space0");
  const auto blocks = recordGroupValues(out, "BLOCK", "2");
  CHECK(std::count(blocks.begin(), blocks.end(), "*Paper_Space0") == 1);
  CHECK(danglingReferences(out).empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF moves a typed object off a handle the save gives a structural record",
          "[dxf][roundtrip][filter][handles][layout]") {
  ensureSettings();
  const std::string src = tmpFile("layout-structural-src.dxf");
  const std::string out = tmpFile("layout-structural-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // Layout1 has handle 1E, where the save writes the *Paper_Space block
  // record, as in DWGs from some other applications.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nBLOCK_RECORD\n5\n1\n330\n0\n100\nAcDbSymbolTable\n70\n2\n" +
                dxfBlockRecord("1F", "*Model_Space") +
                dxfBlockRecord("58", "*Paper_Space") +
                "0\nENDTAB\n0\nENDSEC\n"
                "0\nSECTION\n2\nBLOCKS\n" +
                dxfEmptyBlock("20", "21", "1F", "*Model_Space") +
                dxfEmptyBlock("59", "5A", "58", "*Paper_Space") +
                "0\nENDSEC\n"
                "0\nSECTION\n2\nENTITIES\n"
                "0\nLINE\n5\nA1\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
                "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
                "0\nENDSEC\n"
                "0\nSECTION\n2\nOBJECTS\n"
                "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
                "3\nACAD_LAYOUT\n350\n1A\n"
                "0\nDICTIONARY\n5\n1A\n330\nC\n100\nAcDbDictionary\n281\n1\n"
                "3\nLayout1\n350\n1E\n3\nModel\n350\n22\n" +
                dxfLayoutRecord("22", "Model", 0, "1F") +
                dxfLayoutRecord("1E", "Layout1", 1, "58") +
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
                              graphic.getFormatType()));
  }

  const auto layout1 = recordGroupsWithValue(out, "LAYOUT", "1", "Layout1");
  REQUIRE_FALSE(layout1.empty());
  std::string layoutHandle;
  for (const auto &[code, value] : layout1)
    if (code == "5")
      layoutHandle = value;
  CHECK(layoutHandle != "1E");
  CHECK(layoutBlockRecordName(out, "Layout1") == "*Paper_Space");
  CHECK(layoutBlockRecordName(out, "Model") == "*Model_Space");
  // The layout dictionary follows the layout to its new handle.
  const auto layoutDict = recordGroupsWithValue(out, "DICTIONARY", "3", "Layout1");
  bool named = false;
  for (std::size_t i = 0; i + 1 < layoutDict.size(); ++i)
    if (layoutDict[i] == std::make_pair(std::string("3"), std::string("Layout1")))
      named = layoutDict[i + 1].second == layoutHandle;
  CHECK(named);
  {
    std::ifstream in(out);
    std::string code, value;
    std::map<std::string, int> defined;
    while (std::getline(in, code) && std::getline(in, value))
      if (trimDxfToken(code) == "5" || trimDxfToken(code) == "105")
        ++defined[trimDxfToken(value)];
    for (const auto &[handle, count] : defined) {
      CAPTURE(handle);
      CHECK(count == 1);
    }
  }
  CHECK(danglingReferences(out).empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF references to table records follow them to their written handles",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("table-refs-src.dxf");
  const std::string out = tmpFile("table-refs-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // A raw object names a text style, a layer and a linetype by handle; the
  // save writes each table record under a handle of its own.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
            "0\nSECTION\n2\nCLASSES\n"
            "0\nCLASS\n1\nACME_THING\n2\nAcmeThing\n3\nACME\n90\n0\n280\n0\n281\n0\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLTYPE\n5\n5\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
            "0\nLTYPE\n5\n3D\n330\n5\n100\nAcDbSymbolTableRecord\n"
            "100\nAcDbLinetypeTableRecord\n2\nDASHED\n70\n0\n3\n__ __\n72\n65\n"
            "73\n2\n40\n0.75\n49\n0.5\n74\n0\n49\n-0.25\n74\n0\n"
            "0\nENDTAB\n"
            "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
            "0\nLAYER\n5\n40\n330\n2\n100\nAcDbSymbolTableRecord\n"
            "100\nAcDbLayerTableRecord\n2\nWalls\n70\n0\n62\n1\n6\nDASHED\n"
            "0\nENDTAB\n"
            "0\nTABLE\n2\nSTYLE\n5\n3\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
            "0\nSTYLE\n5\n3F\n330\n3\n100\nAcDbSymbolTableRecord\n"
            "100\nAcDbTextStyleTableRecord\n2\nMyStyle\n70\n0\n40\n0\n41\n1\n"
            "50\n0\n71\n0\n42\n2.5\n3\ntxt\n4\n\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA1\n100\nAcDbEntity\n8\nWalls\n100\nAcDbLine\n"
            "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "3\nACME_THINGS\n350\n80\n"
            "0\nDICTIONARY\n5\n80\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\nTHING\n350\n91\n"
            "0\nACME_THING\n5\n91\n330\n80\n100\nAcmeThing\n"
            "340\n3F\n340\n40\n340\n3D\n"
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
                              graphic.getFormatType()));
  }

  const auto refs = recordGroupValues(out, "ACME_THING", "340");
  REQUIRE(refs.size() == 3);
  const auto nameOf = [&out](const char *record, const std::string &handle) {
    for (const auto &[code, value] : recordGroupsWithValue(out, record, "5", handle))
      if (code == "2")
        return value;
    return std::string("?") + handle;
  };
  CHECK(nameOf("STYLE", refs[0]) == "MyStyle");
  CHECK(nameOf("LAYER", refs[1]) == "Walls");
  CHECK(nameOf("LTYPE", refs[2]) == "DASHED");
  CHECK(danglingReferences(out).empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF layers name their plot style only where it is written",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("plot-style-src.dxf");
  const std::string out = tmpFile("plot-style-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // The source's "Normal" plot style placeholder has handle 4A, not the F
  // the save used to name on every layer.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
            "0\nSECTION\n2\nCLASSES\n"
            "0\nCLASS\n1\nACDBDICTIONARYWDFLT\n2\nAcDbDictionaryWithDefault\n"
            "3\nObjectDBX Classes\n90\n0\n280\n0\n281\n0\n"
            "0\nCLASS\n1\nACDBPLACEHOLDER\n2\nAcDbPlaceHolder\n"
            "3\nObjectDBX Classes\n90\n0\n280\n0\n281\n0\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n2\n"
            "0\nLAYER\n5\n10\n330\n2\n100\nAcDbSymbolTableRecord\n"
            "100\nAcDbLayerTableRecord\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "370\n-3\n390\n4A\n"
            "0\nLAYER\n5\n40\n330\n2\n100\nAcDbSymbolTableRecord\n"
            "100\nAcDbLayerTableRecord\n2\nWalls\n70\n0\n62\n1\n6\nCONTINUOUS\n"
            "370\n-3\n390\n4A\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA1\n100\nAcDbEntity\n8\nWalls\n100\nAcDbLine\n"
            "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
            "0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "3\nACAD_PLOTSTYLENAME\n350\n49\n"
            "0\nACDBDICTIONARYWDFLT\n5\n49\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\nNormal\n350\n4A\n100\nAcDbDictionaryWithDefault\n340\n4A\n"
            "0\nACDBPLACEHOLDER\n5\n4A\n330\n49\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  // A layer the drawing did not have names no plot style.
  graphic.addLayer(new RS_Layer(QStringLiteral("New")));
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              graphic.getFormatType()));
  }

  const auto plotStyleOf = [&out](const char *layer) {
    std::vector<std::string> values;
    for (const auto &[code, value] : recordGroupsWithValue(out, "LAYER", "2", layer))
      if (code == "390")
        values.push_back(value);
    return values;
  };
  REQUIRE(recordGroupValues(out, "ACDBPLACEHOLDER", "5") ==
          std::vector<std::string>{"4A"});
  CHECK(plotStyleOf("Walls") == std::vector<std::string>{"4A"});
  CHECK(plotStyleOf("0") == std::vector<std::string>{"4A"});
  CHECK(plotStyleOf("New").empty());
  CHECK(danglingReferences(out).empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF references to the source's root dictionary name the one written",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  const std::string src = tmpFile("root-dict-src.dxf");
  const std::string out = tmpFile("root-dict-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // The source's root dictionary is B; the save writes its own at C. The
  // material dictionary names the root as owner and as reactor.
  std::string text = kR2000WithRawMaterial;
  const auto replaceOnce = [&text](const std::string &from, const std::string &to) {
    const auto at = text.find(from);
    REQUIRE(at != std::string::npos);
    text.replace(at, from.size(), to);
  };
  replaceOnce("0\nDICTIONARY\n5\nC\n330\n0\n", "0\nDICTIONARY\n5\nB\n330\n0\n");
  replaceOnce("0\nDICTIONARY\n5\n80\n330\nC\n",
              "0\nDICTIONARY\n5\n80\n102\n{ACAD_REACTORS\n330\nB\n102\n}\n330\nB\n");
  writeText(src, text);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              graphic.getFormatType()));
  }

  CHECK(rootDictEntries(out).count("ACAD_MATERIAL") == 1);
  const auto materials = recordGroupsWithValue(out, "DICTIONARY", "5", "80");
  REQUIRE_FALSE(materials.empty());
  std::vector<std::string> owners;
  for (const auto &[code, value] : materials)
    if (code == "330")
      owners.push_back(value);
  CHECK(owners == std::vector<std::string>{"C", "C"});
  CHECK(danglingReferences(out).empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF XDATA handles follow the entities they name",
          "[dxf][roundtrip][filter][handles][xdata]") {
  ensureSettings();
  const std::string src = tmpFile("xdata-handle-src.dxf");
  const std::string out = tmpFile("xdata-handle-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // An MTEXT's columns are linked this way; here one line names the other.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA1\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
            "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
            "1001\nACME_APP\n1005\nA2\n"
            "0\nLINE\n5\nA2\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
            "10\n0\n20\n5\n30\n0\n11\n10\n21\n5\n31\n0\n"
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
                              graphic.getFormatType()));
  }

  const auto lines = recordGroupValues(out, "LINE", "5");
  REQUIRE(lines.size() == 2);
  const auto named = recordGroupValues(out, "LINE", "1005");
  REQUIRE(named.size() == 1);
  CHECK(named.front() != "A2");
  CHECK(std::count(lines.begin(), lines.end(), named.front()) == 1);
  CHECK(recordGroupsWithValue(out, "LINE", "1005", named.front()) !=
        recordGroupsWithValue(out, "LINE", "5", named.front()));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF saved in another version leaves out the raw objects it cannot hold",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  const std::string src = tmpFile("other-version-src.dxf");
  const std::string out = tmpFile("other-version-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  writeText(src, kR2000WithRawMaterial);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    // Raw objects are replayed only into the version they were read from.
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  CHECK(recordGroupValues(out, "LINE", "5").size() == 1);
  // The raw ACME_THING and its dictionary are left out; the MATERIAL, which
  // LibreCAD also keeps typed, is written typed instead.
  CHECK(recordGroupValues(out, "ACME_THING", "5").empty());
  CHECK(rootDictEntries(out).count("ACAD_MATERIAL") == 0);
  CHECK(recordGroupValues(out, "MATERIAL", "5") == std::vector<std::string>{"90"});
  CHECK(danglingReferences(out).empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF write planning excludes typed objects below their version gate",
          "[dxf][roundtrip][filter][version][objects]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 0.0),
                                                RS_Vector(1.0, 1.0)));
  line->setReactorHandles({0x70u, 0xE0u, 0xE1u, 0xE2u, 0xE3u});
  graphic.addEntity(line);

  auto &metadata = graphic.dwgAdvancedMetadata();
  DRW_MLeaderStyle style;
  style.handle = 0xE0u;
  style.parentHandle = 0x70u;
  metadata.addMLeaderStyle(style);
  DRW_Field field;
  field.handle = 0xE1u;
  field.parentHandle = 0x70u;
  metadata.addField(field);
  DRW_FieldList fieldList;
  fieldList.handle = 0xE2u;
  fieldList.parentHandle = 0x70u;
  metadata.addFieldList(fieldList);
  DRW_EvaluationGraph graph;
  graph.handle = 0xE3u;
  metadata.addEvaluationGraph(graph);

  DRW_Dictionary parent;
  parent.handle = 0x70u;
  parent.parentHandle = 0xCu;
  parent.name = "ACAD_TYPED";
  for (const auto &[name, handle] :
       {std::pair<const char *, std::uint32_t>{"STYLE", 0xE0u},
        {"FIELD", 0xE1u}, {"FIELDLIST", 0xE2u}, {"GRAPH", 0xE3u}})
    parent.m_entries.push_back({name, handle});
  metadata.addDictionary(parent);

  for (const auto &[name, handle] :
       {std::pair<const char *, std::uint32_t>{"MLEADERSTYLE", 0xE0u},
        {"FIELD", 0xE1u}, {"FIELDLIST", 0xE2u},
        {"EVALUATION_GRAPH", 0xE3u}}) {
    DRW_RawDxfObject raw;
    raw.name = name;
    raw.handle = handle;
    raw.m_version = DRW::AC1032;
    metadata.addRawDxfObject(raw);
  }
  DRW_RawDxfObject oldParent;
  oldParent.name = "DICTIONARY";
  oldParent.handle = 0x70u;
  oldParent.m_version = DRW::AC1032;
  metadata.addRawDxfObject(oldParent);

  const std::string oldOut = tmpFile("typed_gates_r14.dxf");
  const std::string fieldOut = tmpFile("typed_gates_r2000.dxf");
  const std::string newOut = tmpFile("typed_gates_r2007.dxf");
  std::filesystem::remove(oldOut);
  std::filesystem::remove(fieldOut);
  std::filesystem::remove(newOut);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(oldOut),
                              RS2::FormatDXFRW14));
  }
  const auto oldClasses = recordGroupValues(oldOut, "CLASS", "1");
  for (const char *name : {"MLEADERSTYLE", "FIELD", "FIELDLIST",
                           "EVALUATION_GRAPH"}) {
    CHECK(recordGroupValues(oldOut, name, "5").empty());
    CHECK(std::find(oldClasses.begin(), oldClasses.end(), name) ==
          oldClasses.end());
  }
  CHECK(rootDictEntries(oldOut).count("ACAD_TYPED") == 0);
  const auto oldDictionaries = recordGroupValues(oldOut, "DICTIONARY", "5");
  CHECK(std::find(oldDictionaries.begin(), oldDictionaries.end(), "70") ==
        oldDictionaries.end());
  CHECK_FALSE(containsLine(oldOut, "{ACAD_REACTORS"));

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(fieldOut),
                              RS2::FormatDXFRW2000));
  }
  CHECK(recordGroupValues(fieldOut, "MLEADERSTYLE", "5").empty());
  CHECK(recordGroupValues(fieldOut, "EVALUATION_GRAPH", "5").empty());
  CHECK(recordGroupValues(fieldOut, "FIELD", "5") ==
        std::vector<std::string>{"E1"});
  CHECK(recordGroupValues(fieldOut, "FIELDLIST", "5") ==
        std::vector<std::string>{"E2"});
  CHECK(rootDictEntries(fieldOut).count("ACAD_TYPED") == 1);
  const auto references = recordGroupValues(fieldOut, "LINE", "330");
  CHECK(std::find(references.begin(), references.end(), "70") !=
        references.end());
  CHECK(std::find(references.begin(), references.end(), "E0") ==
        references.end());
  CHECK(std::find(references.begin(), references.end(), "E1") !=
        references.end());
  CHECK(std::find(references.begin(), references.end(), "E2") !=
        references.end());
  CHECK(std::find(references.begin(), references.end(), "E3") ==
        references.end());

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(newOut),
                              RS2::FormatDXFRW));
  }
  for (const auto &[name, handle] :
       {std::pair<const char *, const char *>{"MLEADERSTYLE", "E0"},
        {"FIELD", "E1"}, {"FIELDLIST", "E2"},
        {"EVALUATION_GRAPH", "E3"}})
    CHECK(recordGroupValues(newOut, name, "5") ==
          std::vector<std::string>{handle});
  CHECK(rootDictEntries(newOut).count("ACAD_TYPED") == 1);

  std::filesystem::remove(oldOut);
  std::filesystem::remove(fieldOut);
  std::filesystem::remove(newOut);
}

TEST_CASE("DXF live identities win over left-out raw carriers with the same handle",
          "[dxf][roundtrip][filter][handles][version]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *target = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 0.0),
                                                  RS_Vector(1.0, 0.0)));
  target->setSourceHandle(0xA0u);
  graphic.addEntity(target);
  auto *referer = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 1.0),
                                                   RS_Vector(1.0, 1.0)));
  referer->setSourceHandle(0xA1u);
  referer->setReactorHandles({0xA0u, 0xB0u});
  graphic.addEntity(referer);

  DRW_Group group;
  group.handle = 0xB0u;
  group.m_entityHandles = {0xA0u};
  auto &metadata = graphic.dwgAdvancedMetadata();
  metadata.addGroup(group);
  for (const std::uint32_t handle : {0xA0u, 0xB0u}) {
    DRW_RawDxfObject raw;
    raw.name = handle == 0xA0u ? "LINE" : "GROUP";
    raw.handle = handle;
    raw.m_version = DRW::AC1015;
    if (handle == 0xA0u)
      metadata.addRawDxfEntity(raw);
    else
      metadata.addRawDxfObject(raw);
  }

  const std::string out = tmpFile("live_raw_handle_collision.dxf");
  std::filesystem::remove(out);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  const auto lines = recordGroupValues(out, "LINE", "5");
  REQUIRE(lines.size() == 2);
  CHECK(recordGroupValues(out, "GROUP", "5") ==
        std::vector<std::string>{"B0"});
  const auto references = recordGroupValues(out, "LINE", "330");
  CHECK(std::count(references.begin(), references.end(), lines.front()) == 1);
  CHECK(std::count(references.begin(), references.end(), "B0") == 1);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export reports what it left out",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  const std::string src = tmpFile("report-src.dxf");
  const std::string same = tmpFile("report-same.dxf");
  const std::string other = tmpFile("report-other.dxf");
  writeText(src, kR2000WithRawMaterial);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  RS_FileIO *io = RS_FileIO::instance();
  REQUIRE(io->fileExport(graphic, QString::fromStdString(same), graphic.getFormatType()));
  CHECK(io->lastExportReport().isEmpty());
  // R2007 cannot hold the raw ACME_THING and its dictionary read from R2000
  REQUIRE(io->fileExport(graphic, QString::fromStdString(other), RS2::FormatDXFRW));
  CHECK(io->lastExportReport().startsWith(QStringLiteral("2 object(s)")));

  std::filesystem::remove(src);
  std::filesystem::remove(same);
  std::filesystem::remove(other);
}

TEST_CASE("DXF text keeps control characters and carets as caret codes",
          "[dxf][roundtrip][filter][text]") {
  ensureSettings();
  const std::string src = tmpFile("caret-src.dxf");
  const std::string out = tmpFile("caret-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // ^I is a TAB, ^M a carriage return, "^ " a literal caret; ASCII DXF can
  // hold neither control character as such.
  writeText(src,
            "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n"
            "0\nLAYER\n2\nA^B\n70\n0\n62\n7\n6\nCONTINUOUS\n0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nTEXT\n8\nA^B\n10\n0\n20\n0\n30\n0\n40\n1\n1\nx^Iy\n"
            "0\nTEXT\n8\n0\n10\n0\n20\n5\n30\n0\n40\n1\n1\nc^Md\n"
            "0\nTEXT\n8\n0\n10\n0\n20\n9\n30\n0\n40\n1\n1\np^ q\n"
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

  CHECK(recordGroupValues(out, "TEXT", "1")
        == std::vector<std::string>{"x^Iy", "c^Md", "p^ q"});
  const auto textLayers = recordGroupValues(out, "TEXT", "8");
  REQUIRE_FALSE(textLayers.empty());
  CHECK(textLayers.front() == "A^B");
  const auto layerNames = recordGroupValues(out, "LAYER", "2");
  CHECK(std::find(layerNames.cbegin(), layerNames.cend(), "A^B") != layerNames.cend());

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

TEST_CASE("DXF DSTYLE line type reference survives a non-ASCII linetype name",
          "[dxf][roundtrip][filter][dimension][dimstyle][ltype]") {
  ensureSettings();

  // The writer keys its LTYPE handle table by folding the raw UTF-8 bytes, so
  // the dimension style query has to fold the same way. An ASCII name folds
  // identically under either rule and is the control.
  const auto checkDimLineTypeRef = [](const char *fileStem,
                                      const char *lineTypeName) {
    const std::string out = tmpFile(fileStem);
    std::filesystem::remove(out);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    DRW_LType imported;
    imported.updateValues(lineTypeName, "Imported dashed", 2, 12.7,
                          {6.35, -6.35});
    graphic.dwgAdvancedMetadata().addLineTypeName(imported);

    RS_DimensionData data;
    data.definitionPoint = RS_Vector(5.0, 3.0);
    data.middleOfText = RS_Vector(5.0, 3.0);
    data.style = "Standard";
    auto *dimension = new RS_DimAligned(
        &graphic, data,
        RS_DimAlignedData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
    LC_DimStyle override;
    override.dimensionLine()->setLineType(QString::fromUtf8(lineTypeName));
    dimension->setDimStyleOverride(&override);
    graphic.addEntity(dimension);

    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                                RS2::FormatDXFRW2018));
    }
    CHECK(recordGroupValues(out, "DIMENSION", "1070")
          == std::vector<std::string>{"345"});
    CHECK(recordGroupValues(out, "DIMENSION", "1005").size() == 1);

    RS_Graphic reloaded;
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                                RS2::FormatDXFRW));
    }
    auto *reloadedDimension =
        dynamic_cast<RS_Dimension *>(reloaded.firstEntity());
    REQUIRE(reloadedDimension != nullptr);
    LC_DimStyle *reloadedOverride = reloadedDimension->getDimStyleOverride();
    CHECK(reloadedOverride != nullptr);
    if (reloadedOverride != nullptr)
      CHECK(reloadedOverride->dimensionLine()->lineTypeName()
            == QString::fromUtf8(lineTypeName));

    std::filesystem::remove(out);
  };

  checkDimLineTypeRef("dimension_dstyle_ltype_ascii.dxf", "vendor_dash");
  checkDimLineTypeRef("dimension_dstyle_ltype_utf8.dxf", "\xC3\xB6lfarbe");
}

namespace {

// A DIMSTYLE record with the given groups after its name and flags.
std::string dimStyleRecord(const std::string &handle, const std::string &name,
                           const std::string &groups) {
  return "0\nDIMSTYLE\n105\n" + handle + "\n330\nA\n"
         "100\nAcDbSymbolTableRecord\n100\nAcDbDimStyleTableRecord\n"
         "2\n" + name + "\n70\n0\n" + groups;
}

// ByBlock, ByLayer and Continuous at handles 14-16, `lineTypes` at 40, 41
// and so on, and the DIMSTYLE records `dimStyles`.
std::string dimStyleLineTypeFixture(const std::vector<std::string> &lineTypes,
                                    const std::string &dimStyles) {
  const auto record = [](const std::string &handle, const std::string &name,
                         const std::string &pattern) {
    return "0\nLTYPE\n5\n" + handle + "\n330\n5\n"
           "100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
           "2\n" + name + "\n70\n0\n" + pattern;
  };
  const std::string solid = "72\n65\n73\n0\n40\n0.0\n";
  std::string records = record("14", "ByBlock", "3\n\n" + solid) +
                        record("15", "ByLayer", "3\n\n" + solid) +
                        record("16", "Continuous", "3\nSolid line\n" + solid);
  for (std::size_t i = 0; i < lineTypes.size(); ++i)
    records += record(std::to_string(40 + i), lineTypes[i],
                      "3\nDashed\n72\n65\n73\n2\n40\n19.05\n"
                      "49\n12.7\n74\n0\n49\n-6.35\n74\n0\n");
  return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
         "0\nSECTION\n2\nTABLES\n"
         "0\nTABLE\n2\nLTYPE\n5\n5\n330\n0\n100\nAcDbSymbolTable\n70\n" +
         std::to_string(3 + lineTypes.size()) + "\n" + records +
         "0\nENDTAB\n"
         "0\nTABLE\n2\nDIMSTYLE\n5\nA\n330\n0\n"
         "100\nAcDbSymbolTable\n70\n1\n100\nAcDbDimStyleTable\n71\n0\n" +
         dimStyles +
         "0\nENDTAB\n0\nENDSEC\n"
         "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
}

// VENDOR_DIM with DIMLTYPE, DIMLTEX1 and DIMLTEX2 at LTYPE 40, 41 and 42.
const std::string kVendorDim =
    dimStyleRecord("31", "VENDOR_DIM", "345\n40\n346\n41\n347\n42\n");

// DIMLTYPE, DIMLTEX1 and DIMLTEX2 of a dimension style.
std::vector<std::string> dimStyleLineTypes(const RS_Graphic &graphic,
                                           const char *name) {
  LC_DimStyle *style = graphic.getDimStyleByName(QString::fromUtf8(name));
  REQUIRE(style != nullptr);
  return {style->dimensionLine()->lineTypeName().toStdString(),
          style->extensionLine()->lineTypeFirstRaw().toStdString(),
          style->extensionLine()->lineTypeSecondRaw().toStdString()};
}

std::string lineTypeHandle(const std::string &path, const std::string &name) {
  for (const auto &[code, value] :
       recordGroupsWithValue(path, "LTYPE", "2", name))
    if (code == "5")
      return value;
  return {};
}

// Groups 345-347 of a written DIMSTYLE record point at the LTYPE records
// named `lineTypes`. ByBlock is written as no group.
void checkWrittenDimStyleLineTypes(const std::string &path, const char *name,
                                   const std::vector<std::string> &lineTypes) {
  const char *codes[] = {"345", "346", "347"};
  for (std::size_t i = 0; i < 3; ++i) {
    INFO(name << " group " << codes[i]);
    const auto written = namedRecordGroupValues(path, "DIMSTYLE", name,
                                                codes[i]);
    if (lineTypes[i] == "ByBlock") {
      CHECK(written.empty());
    } else {
      const std::string handle = lineTypeHandle(path, lineTypes[i]);
      CHECK_FALSE(handle.empty());
      CHECK(written == std::vector<std::string>{handle});
    }
  }
}

// Opens `fixture`, saves it as R2007 DXF and reopens the saved file.
void checkDimStyleLineTypeRoundTrip(const std::string &stem,
                                    const std::string &fixture,
                                    const std::vector<std::string> &lineTypes) {
  const std::string src = tmpFile((stem + "_src.dxf").c_str());
  const std::string out = tmpFile((stem + "_out.dxf").c_str());
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  writeText(src, fixture);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  CHECK(dimStyleLineTypes(graphic, "VENDOR_DIM") == lineTypes);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  checkWrittenDimStyleLineTypes(out, "VENDOR_DIM", lineTypes);

  RS_Graphic reimported;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reimported, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(dimStyleLineTypes(reimported, "VENDOR_DIM") == lineTypes);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// The LINE records of a dimension block, in every DXF version, carry the
// linetypes `expected` (sorted).
void checkDimensionBlockLineTypes(const std::string &stem,
                                  const std::vector<std::string> &lineTypes,
                                  const std::vector<std::string> &expected) {
  const std::string src = tmpFile((stem + "_src.dxf").c_str());
  std::filesystem::remove(src);
  writeText(src, dimStyleLineTypeFixture(lineTypes, kVendorDim));

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  RS_DimensionData data;
  data.definitionPoint = RS_Vector(5.0, 3.0);
  data.middleOfText = RS_Vector(5.0, 3.0);
  data.style = "VENDOR_DIM";
  auto *dimension = new RS_DimAligned(
      &graphic, data,
      RS_DimAlignedData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
  graphic.addEntity(dimension);
  dimension->update();

  for (const auto &[suffix, format] :
       {std::make_pair("r12", RS2::FormatDXFRW12),
        std::make_pair("r2000", RS2::FormatDXFRW2000),
        std::make_pair("r2007", RS2::FormatDXFRW)}) {
    INFO(suffix);
    const std::string out = tmpFile((stem + "_" + suffix + ".dxf").c_str());
    std::filesystem::remove(out);
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    }
    std::vector<std::string> written = recordGroupValues(out, "LINE", "6");
    std::sort(written.begin(), written.end());
    CHECK(written == expected);
    std::filesystem::remove(out);
  }
  std::filesystem::remove(src);
}

// The DXF text of `path` with LTYPE record `name` moved to handle `to`,
// and its DSTYLE references with it.
std::string withLineTypeHandle(const std::string &path, const std::string &name,
                               const std::string &to) {
  const std::string from = lineTypeHandle(path, name);
  REQUIRE_FALSE(from.empty());
  std::ifstream input(path);
  std::ostringstream output;
  std::string code, value;
  while (std::getline(input, code) && std::getline(input, value)) {
    const std::string groupCode = trimDxfToken(code);
    if ((groupCode == "5" || groupCode == "1005") &&
        trimDxfToken(value) == from)
      value = to;
    output << code << '\n' << value << '\n';
  }
  return output.str();
}

} // namespace

TEST_CASE("DXF DIMSTYLE linetype reference resolves back to the named linetype",
          "[dxf][roundtrip][filter][dimstyle][ltype]") {
  ensureSettings();
  const std::vector<std::string> lineTypes = {"VENDOR_TAB", "VENDOR_DOT",
                                              "VENDOR_DASH"};
  checkDimStyleLineTypeRoundTrip(
      "dimstyle_ltype_named", dimStyleLineTypeFixture(lineTypes, kVendorDim),
      lineTypes);
}

TEST_CASE("A built-in DIMLTYPE survives a DXF round trip",
          "[dxf][roundtrip][filter][dimstyle][ltype]") {
  ensureSettings();
  const std::vector<std::string> lineTypes = {"DASHED", "HIDDEN", "CENTER"};
  checkDimStyleLineTypeRoundTrip(
      "dimstyle_ltype_builtin", dimStyleLineTypeFixture(lineTypes, kVendorDim),
      lineTypes);
}

TEST_CASE("ByLayer and Continuous DIMSTYLE linetypes survive a DXF round trip",
          "[dxf][roundtrip][filter][dimstyle][ltype]") {
  ensureSettings();
  checkDimStyleLineTypeRoundTrip(
      "dimstyle_ltype_fixed",
      dimStyleLineTypeFixture(
          {}, dimStyleRecord("31", "VENDOR_DIM", "345\n15\n346\n16\n347\n14\n")),
      {"ByLayer", "Continuous", "ByBlock"});
}

TEST_CASE("DXF DIMSTYLE linetypes in legacy LibreCAD groups 347 and 348 read",
          "[dxf][roundtrip][filter][dimstyle][ltype]") {
  ensureSettings();
  const std::vector<std::string> lineTypes = {"DASHED", "HIDDEN", "CENTER"};
  checkDimStyleLineTypeRoundTrip(
      "dimstyle_ltype_legacy",
      dimStyleLineTypeFixture(
          lineTypes,
          dimStyleRecord("31", "VENDOR_DIM", "345\n40\n347\n41\n348\n42\n")),
      lineTypes);
}

TEST_CASE("A DIMSTYLE for one dimension type does not inherit linetypes",
          "[dxf][roundtrip][filter][dimstyle][ltype]") {
  ensureSettings();
  const std::string src = tmpFile("dimstyle_ltype_child_src.dxf");
  const std::string out = tmpFile("dimstyle_ltype_child_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  const std::vector<std::string> lineTypes = {"DASHED", "HIDDEN", "CENTER"};
  const std::vector<std::string> byBlock(3, "ByBlock");
  writeText(src, dimStyleLineTypeFixture(
                     lineTypes,
                     kVendorDim + dimStyleRecord("32", "VENDOR_DIM$0", "")));

  // A record without linetype groups means ByBlock, as it does for a base
  // style; loading merges only what a type style leaves unset.
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  graphic.onLoadingCompleted();
  CHECK(dimStyleLineTypes(graphic, "VENDOR_DIM") == lineTypes);
  CHECK(dimStyleLineTypes(graphic, "VENDOR_DIM$0") == byBlock);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  checkWrittenDimStyleLineTypes(out, "VENDOR_DIM", lineTypes);
  checkWrittenDimStyleLineTypes(out, "VENDOR_DIM$0", byBlock);

  RS_Graphic reimported;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reimported, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  reimported.onLoadingCompleted();
  CHECK(dimStyleLineTypes(reimported, "VENDOR_DIM$0") == byBlock);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF dimension blocks draw with the DIMSTYLE linetypes",
          "[dxf][roundtrip][filter][dimension][dimstyle][ltype]") {
  ensureSettings();
  checkDimensionBlockLineTypes("dimstyle_ltype_block_builtin",
                               {"DASHED", "HIDDEN", "CENTER"},
                               {"CENTER", "DASHED", "HIDDEN"});
  // A name without a built-in pattern draws continuous (nothing in LibreCAD
  // renders an arbitrary dash pattern), but the name itself still reaches
  // the written file, the same as it does on any other entity's pen.
  checkDimensionBlockLineTypes("dimstyle_ltype_block_named",
                               {"VENDOR_TAB", "VENDOR_DOT", "VENDOR_DASH"},
                               {"VENDOR_DASH", "VENDOR_DOT", "VENDOR_TAB"});
}

TEST_CASE("DXF DSTYLE linetype references resolve high handles",
          "[dxf][roundtrip][filter][dimension][dimstyle][ltype]") {
  ensureSettings();
  const std::string out = tmpFile("dimension_dstyle_ltype_high.dxf");
  const std::string moved = tmpFile("dimension_dstyle_ltype_high_moved.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(moved);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  for (const char *name : {"vendor_dash", "vendor_dot", "vendor_center"}) {
    DRW_LType imported;
    imported.updateValues(name, "Imported dashed", 2, 12.7, {6.35, -6.35});
    graphic.dwgAdvancedMetadata().addLineTypeName(imported);
  }
  RS_DimensionData data;
  data.definitionPoint = RS_Vector(5.0, 3.0);
  data.middleOfText = RS_Vector(5.0, 3.0);
  data.style = "Standard";
  auto *dimension = new RS_DimAligned(
      &graphic, data,
      RS_DimAlignedData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
  LC_DimStyle override;
  override.dimensionLine()->setLineType(QStringLiteral("vendor_dash"));
  override.extensionLine()->setLineTypeFirst(QStringLiteral("vendor_dot"));
  override.extensionLine()->setLineTypeSecond(QStringLiteral("vendor_center"));
  dimension->setDimStyleOverride(&override);
  graphic.addEntity(dimension);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW2018));
  }
  // A handle above 0x7FFFFFFF does not fit a signed 32-bit parse.
  writeText(moved, withLineTypeHandle(out, "vendor_dash", "80000040"));

  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(moved),
                              RS2::FormatDXFRW));
  }
  auto *reloadedDimension = dynamic_cast<RS_Dimension *>(reloaded.firstEntity());
  REQUIRE(reloadedDimension != nullptr);
  LC_DimStyle *reloadedOverride = reloadedDimension->getDimStyleOverride();
  REQUIRE(reloadedOverride != nullptr);
  CHECK(reloadedOverride->dimensionLine()->lineTypeName().toStdString() ==
        "vendor_dash");
  CHECK(reloadedOverride->extensionLine()->lineTypeFirstRaw().toStdString() ==
        "vendor_dot");
  CHECK(reloadedOverride->extensionLine()->lineTypeSecondRaw().toStdString() ==
        "vendor_center");

  std::filesystem::remove(out);
  std::filesystem::remove(moved);
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

TEST_CASE("DXF import maps HIDDEN2 to the half-scale hidden linetype",
          "[dxf][filter][linetype][regression]") {
  ensureSettings();
  const std::string src = tmpFile("hidden2_src.dxf");
  std::filesystem::remove(src);

  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLTYPE\n70\n1\n"
            "0\nLTYPE\n2\nHIDDEN2\n70\n0\n3\nHidden (.5X)\n72\n65\n73\n2\n40\n0.1875\n"
            "49\n0.125\n74\n0\n49\n-0.0625\n74\n0\n"
            "0\nENDTAB\n0\nTABLE\n2\nLAYER\n70\n2\n"
            "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "0\nLAYER\n2\nHIDDEN_LAYER\n70\n0\n62\n7\n6\nHIDDEN2\n"
            "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\nHIDDEN_LAYER\n6\nHIDDEN2\n"
            "10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  const auto *layer = graphic.findLayer(QStringLiteral("HIDDEN_LAYER"));
  REQUIRE(layer != nullptr);
  CHECK(layer->getPen().getLineType() == RS2::HiddenLine2);

  RS_Entity *line = graphic.firstEntity();
  REQUIRE(line != nullptr);
  CHECK(line->getPen(false).getLineType() == RS2::HiddenLine2);

  std::filesystem::remove(src);
}

TEST_CASE("DXF round-trip preserves the HIDDEN linetype on an entity",
          "[dxf][roundtrip][filter][linetype]") {
  ensureSettings();
  const std::string out = tmpFile("hidden_out.dxf");
  const std::string dwg = tmpFile("hidden.dwg");
  std::filesystem::remove(out);
  std::filesystem::remove(dwg);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 0.0),
                                                 RS_Vector(10.0, 10.0)));
  line->setPen(RS_Pen(RS_Color(255, 0, 0), RS2::Width00, RS2::HiddenLine));
  graphic.addEntity(line);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // The entity references the linetype by its acad.lin name...
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"HIDDEN"});
  // ...and the LTYPE table carries acad.lin's HIDDEN (A,.25,-.125) in mm.
  const auto dashes = ltypeRecordGroupValues(out, "HIDDEN", "49");
  REQUIRE(dashes.size() == 2);
  CHECK(std::stod(dashes[0]) == Catch::Approx(6.35));
  CHECK(std::stod(dashes[1]) == Catch::Approx(-3.175));
  CHECK(ltypeRecordGroupValues(out, "HIDDEN", "73") ==
        std::vector<std::string>{"2"});

  RS_Graphic reimported;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reimported, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  RS_Entity *imported = reimported.firstEntity();
  REQUIRE(imported != nullptr);
  CHECK(imported->getPen(false).getLineType() == RS2::HiddenLine);

  // The reimported drawing now carries HIDDEN in its raw LTYPE table copy as
  // well; saving it again must write the record once, not once per source.
  const std::string out2 = tmpFile("hidden_out2.dxf");
  std::filesystem::remove(out2);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(reimported, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  CHECK(ltypeRecordGroupValues(out2, "HIDDEN", "73") ==
        std::vector<std::string>{"2"});
  CHECK(recordGroupValues(out2, "LINE", "6") ==
        std::vector<std::string>{"HIDDEN"});
  std::filesystem::remove(out2);

#ifdef DWGSUPPORT
  // The DWG writer resolves entity linetypes by handle against the LTYPE
  // table emitted by writeLTypes(), so a missing record would silently
  // degrade the pen here.
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
  }
  RS_Entity *dwgLine = fromDwg.firstEntity();
  REQUIRE(dwgLine != nullptr);
  CHECK(dwgLine->getPen(false).getLineType() == RS2::HiddenLine);
#endif

  std::filesystem::remove(out);
  std::filesystem::remove(dwg);
}

TEST_CASE("Every DXF linetype name LibreCAD writes maps back to the same RS2::LineType",
          "[dxf][filter][linetype]") {
  for (const RS2::LineType type : {
           RS2::SolidLine,
           RS2::DotLine, RS2::DotLineTiny, RS2::DotLine2, RS2::DotLineX2,
           RS2::DashLine, RS2::DashLineTiny, RS2::DashLine2, RS2::DashLineX2,
           RS2::HiddenLine, RS2::HiddenLineTiny, RS2::HiddenLine2,
           RS2::HiddenLineX2,
           RS2::PhantomLine, RS2::PhantomLineTiny, RS2::PhantomLine2,
           RS2::PhantomLineX2,
           RS2::DashDotLine, RS2::DashDotLineTiny, RS2::DashDotLine2,
           RS2::DashDotLineX2,
           RS2::DivideLine, RS2::DivideLineTiny, RS2::DivideLine2,
           RS2::DivideLineX2,
           RS2::CenterLine, RS2::CenterLineTiny, RS2::CenterLine2,
           RS2::CenterLineX2,
           RS2::BorderLine, RS2::BorderLineTiny, RS2::BorderLine2,
           RS2::BorderLineX2,
           RS2::LineByLayer, RS2::LineByBlock}) {
    const QString name = LC_LineTypeNames::lineTypeToName(type);
    INFO("linetype " << static_cast<int>(type) << " -> " << name.toStdString());
    CHECK(LC_LineTypeNames::nameToLineType(name) == type);
  }

  // The hidden family keeps its acad.lin names, distinct from DASHED*.
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::HiddenLine) == "HIDDEN");
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::HiddenLineTiny) == "HIDDENTINY");
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::HiddenLine2) == "HIDDEN2");
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::HiddenLineX2) == "HIDDENX2");

  // The phantom family keeps its acad.lin names (PHANTOM* used to fall
  // through to CONTINUOUS).
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::PhantomLine) == "PHANTOM");
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::PhantomLineTiny) ==
        "PHANTOMTINY");
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::PhantomLine2) == "PHANTOM2");
  CHECK(LC_LineTypeNames::lineTypeToName(RS2::PhantomLineX2) == "PHANTOMX2");
  // ISO 128-20 type 09 (long-dashed double-short-dashed) imports as PHANTOM,
  // the way ACAD_ISO04W100 / ACAD_ISO05W100 already alias DASHDOTX2 / DIVIDEX2.
  CHECK(LC_LineTypeNames::nameToLineType(QStringLiteral("ACAD_ISO09W100")) ==
        RS2::PhantomLine);
  CHECK(LC_LineTypeNames::nameToLineType(QStringLiteral("acad_iso09w100")) ==
        RS2::PhantomLine);

  // The JWW filter carries its own name tables (without tiny variants); they
  // must agree with the engine ones.
  for (const RS2::LineType type : {RS2::HiddenLine, RS2::HiddenLine2,
                                   RS2::HiddenLineX2, RS2::PhantomLine,
                                   RS2::PhantomLine2, RS2::PhantomLineX2}) {
    INFO("linetype " << static_cast<int>(type));
    const QString name = RS_FilterJWW::lineTypeToName(type);
    CHECK(name == LC_LineTypeNames::lineTypeToName(type));
    CHECK(RS_FilterJWW::nameToLineType(name) == type);
  }

  // The values are persisted (QSettings, .lcp palettes, $DIMLTYPE) and the JWW
  // export loop treats HiddenLine..PhantomLineX2 as one contiguous range.
  CHECK(static_cast<int>(RS2::HiddenLine) == 28);
  CHECK(static_cast<int>(RS2::HiddenLineX2) == 31);
  CHECK(static_cast<int>(RS2::PhantomLine) == 32);
  CHECK(static_cast<int>(RS2::PhantomLineX2) == 35);

  // Every drawable type must have a screen pattern: RS_Painter dereferences
  // getPattern() without a null check.
  for (const RS2::LineType type : {RS2::HiddenLine, RS2::HiddenLineTiny,
                                   RS2::HiddenLine2, RS2::HiddenLineX2,
                                   RS2::PhantomLine, RS2::PhantomLineTiny,
                                   RS2::PhantomLine2, RS2::PhantomLineX2}) {
    INFO("linetype " << static_cast<int>(type));
    CHECK(RS_LineTypePattern::getPattern(type) != nullptr);
  }
}

TEST_CASE("DXF import keeps PHANTOM on a layer and on an entity",
          "[dxf][filter][linetype]") {
  ensureSettings();
  const std::string src = tmpFile("phantom_src.dxf");
  std::filesystem::remove(src);

  // acad.lin: PHANTOM A,1.25,-.25,.25,-.25,.25,-.25 and PHANTOM2 at half
  // that (inch values, as an imperial AutoCAD drawing carries them; acadiso.lin
  // is the same x 25.4). Without the phantom family nameToLineType() fell
  // through to SolidLine for both names.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLTYPE\n70\n2\n"
            "0\nLTYPE\n2\nPHANTOM\n70\n0\n3\nPhantom ______  __  __  ______\n"
            "72\n65\n73\n6\n40\n2.5\n"
            "49\n1.25\n74\n0\n49\n-0.25\n74\n0\n49\n0.25\n74\n0\n"
            "49\n-0.25\n74\n0\n49\n0.25\n74\n0\n49\n-0.25\n74\n0\n"
            "0\nLTYPE\n2\nPHANTOM2\n70\n0\n3\nPhantom (.5x) ___ _ _ ___ _ _\n"
            "72\n65\n73\n6\n40\n1.25\n"
            "49\n0.625\n74\n0\n49\n-0.125\n74\n0\n49\n0.125\n74\n0\n"
            "49\n-0.125\n74\n0\n49\n0.125\n74\n0\n49\n-0.125\n74\n0\n"
            "0\nENDTAB\n0\nTABLE\n2\nLAYER\n70\n2\n"
            "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "0\nLAYER\n2\nPHANTOM_LAYER\n70\n0\n62\n7\n6\nPHANTOM\n"
            "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\nPHANTOM_LAYER\n6\nPHANTOM\n"
            "10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
            "0\nLINE\n8\n0\n6\nPHANTOM2\n"
            "10\n0.0\n20\n5.0\n11\n10.0\n21\n5.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  const auto *layer = graphic.findLayer(QStringLiteral("PHANTOM_LAYER"));
  REQUIRE(layer != nullptr);
  CHECK(layer->getPen().getLineType() == RS2::PhantomLine);

  RS_Entity *line = graphic.firstEntity();
  REQUIRE(line != nullptr);
  CHECK(line->getPen(false).getLineType() == RS2::PhantomLine);

  RS_Entity *half = graphic.nextEntity();
  REQUIRE(half != nullptr);
  CHECK(half->getPen(false).getLineType() == RS2::PhantomLine2);

  std::filesystem::remove(src);
}

TEST_CASE("DXF round-trip preserves the PHANTOM linetype on an entity",
          "[dxf][roundtrip][filter][linetype]") {
  ensureSettings();
  const std::string out = tmpFile("phantom_out.dxf");
  const std::string dwg = tmpFile("phantom.dwg");
  std::filesystem::remove(out);
  std::filesystem::remove(dwg);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 0.0),
                                                 RS_Vector(10.0, 10.0)));
  line->setPen(RS_Pen(RS_Color(255, 0, 0), RS2::Width00, RS2::PhantomLine));
  graphic.addEntity(line);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // The entity references the linetype by its acad.lin name...
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"PHANTOM"});
  // ...and the LTYPE table carries acad.lin's PHANTOM
  // (A,1.25,-.25,.25,-.25,.25,-.25) in mm.
  const auto dashes = ltypeRecordGroupValues(out, "PHANTOM", "49");
  REQUIRE(dashes.size() == 6);
  CHECK(std::stod(dashes[0]) == Catch::Approx(31.75));
  CHECK(std::stod(dashes[1]) == Catch::Approx(-6.35));
  CHECK(std::stod(dashes[2]) == Catch::Approx(6.35));
  CHECK(std::stod(dashes[3]) == Catch::Approx(-6.35));
  CHECK(std::stod(dashes[4]) == Catch::Approx(6.35));
  CHECK(std::stod(dashes[5]) == Catch::Approx(-6.35));
  CHECK(ltypeRecordGroupValues(out, "PHANTOM", "73") ==
        std::vector<std::string>{"6"});
  const auto length = ltypeRecordGroupValues(out, "PHANTOM", "40");
  REQUIRE(length.size() == 1);
  CHECK(std::stod(length[0]) == Catch::Approx(63.5));

  // The three scaled records are emitted alongside, at acad.lin's ratios.
  struct ScaledRecord {
    const char *name;
    double length;
    std::vector<double> dashes;
  };
  for (const ScaledRecord &record : {
           ScaledRecord{"PHANTOMTINY", 9.525,
                        {4.7625, -0.9525, 0.9525, -0.9525, 0.9525, -0.9525}},
           ScaledRecord{"PHANTOM2", 31.75,
                        {15.875, -3.175, 3.175, -3.175, 3.175, -3.175}},
           ScaledRecord{"PHANTOMX2", 127.0,
                        {63.5, -12.7, 12.7, -12.7, 12.7, -12.7}}}) {
    INFO("LTYPE " << record.name);
    CHECK(ltypeRecordGroupValues(out, record.name, "73") ==
          std::vector<std::string>{"6"});
    const auto scaledLength = ltypeRecordGroupValues(out, record.name, "40");
    REQUIRE(scaledLength.size() == 1);
    CHECK(std::stod(scaledLength[0]) == Catch::Approx(record.length));
    const auto scaledDashes = ltypeRecordGroupValues(out, record.name, "49");
    REQUIRE(scaledDashes.size() == record.dashes.size());
    for (std::size_t i = 0; i < record.dashes.size(); ++i) {
      CHECK(std::stod(scaledDashes[i]) == Catch::Approx(record.dashes[i]));
    }
  }

  RS_Graphic reimported;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reimported, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  RS_Entity *imported = reimported.firstEntity();
  REQUIRE(imported != nullptr);
  CHECK(imported->getPen(false).getLineType() == RS2::PhantomLine);

  // The reimported drawing now carries PHANTOM in its raw LTYPE table copy
  // as well; saving it again must write the record once, not once per
  // source.
  const std::string out2 = tmpFile("phantom_out2.dxf");
  std::filesystem::remove(out2);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(reimported, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
  CHECK(ltypeRecordGroupValues(out2, "PHANTOM", "73") ==
        std::vector<std::string>{"6"});
  CHECK(recordGroupValues(out2, "LINE", "6") ==
        std::vector<std::string>{"PHANTOM"});
  std::filesystem::remove(out2);

#ifdef DWGSUPPORT
  // The DWG writer resolves entity linetypes by handle against the LTYPE
  // table emitted by writeLTypes(), so a missing record would silently
  // degrade the pen here.
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
  }
  RS_Entity *dwgLine = fromDwg.firstEntity();
  REQUIRE(dwgLine != nullptr);
  CHECK(dwgLine->getPen(false).getLineType() == RS2::PhantomLine);
#endif

  std::filesystem::remove(out);
  std::filesystem::remove(dwg);
}

namespace {
// An R12 file whose HIDDEN record has its own dashes and whose DASHED record
// has the groups in `dashed`, with a LINE drawn in each.
std::string builtinLTypeFixture(const char *dashed) {
  return std::string(
             "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
             "0\nSECTION\n2\nTABLES\n"
             "0\nTABLE\n2\nLTYPE\n70\n2\n"
             "0\nLTYPE\n2\nHIDDEN\n70\n0\n3\nFile hidden\n72\n65\n73\n2\n"
             "40\n96.0\n49\n64.0\n49\n-32.0\n"
             "0\nLTYPE\n2\nDASHED\n") +
         dashed +
         "0\nENDTAB\n0\nENDSEC\n"
         "0\nSECTION\n2\nENTITIES\n"
         "0\nLINE\n8\n0\n6\nHIDDEN\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
         "0\nLINE\n8\n0\n6\nDASHED\n10\n0.0\n20\n5.0\n11\n10.0\n21\n5.0\n"
         "0\nENDSEC\n0\nEOF\n";
}

void importBuiltinLTypeFixture(RS_Graphic &graphic, const char *suffix,
                               const char *dashed) {
  const std::string src = tmpFile(suffix);
  writeText(src, builtinLTypeFixture(dashed));
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                            RS2::FormatDXFRW));
  std::filesystem::remove(src);
}

void checkBuiltinDashed(const std::string &out) {
  CHECK(ltypeRecordGroupValues(out, "DASHED", "73") ==
        std::vector<std::string>{"2"});
  const auto dashes = ltypeRecordGroupValues(out, "DASHED", "49");
  REQUIRE(dashes.size() == 2);
  CHECK(std::stod(dashes[0]) == Catch::Approx(12.7));
  CHECK(std::stod(dashes[1]) == Catch::Approx(-6.35));
}
} // namespace

TEST_CASE("DXF export keeps a built-in's dashes when the file's record has none",
          "[dxf][roundtrip][filter][linetype][ltype]") {
  ensureSettings();
  const std::string out = tmpFile("name_only_ltype_out.dxf");

  // Early dxflib wrote records like this one, with neither a description nor
  // dashes, as in the shipped empty.dxf template.
  RS_Graphic graphic;
  importBuiltinLTypeFixture(graphic, "name_only_ltype_src.dxf", "70\n64\n");
  for (const RS2::FormatType format :
       {RS2::FormatDXFRW12, RS2::FormatDXFRW2000, RS2::FormatDXFRW}) {
    INFO("format " << static_cast<int>(format));
    std::filesystem::remove(out);
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    }

    // A record with a pattern still replaces the built-in, once.
    CHECK(ltypeRecordGroupValues(out, "HIDDEN", "73") ==
          std::vector<std::string>{"2"});
    const auto hidden = ltypeRecordGroupValues(out, "HIDDEN", "49");
    REQUIRE(hidden.size() == 2);
    CHECK(std::stod(hidden[0]) == Catch::Approx(64.0));
    CHECK(std::stod(hidden[1]) == Catch::Approx(-32.0));

    // A record with none keeps its flags, and takes the built-in's
    // description and dashes.
    CHECK(ltypeRecordGroupValues(out, "DASHED", "70") ==
          std::vector<std::string>{"64"});
    CHECK(ltypeRecordGroupValues(out, "DASHED", "3") ==
          std::vector<std::string>{"Dashed _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _"});
    checkBuiltinDashed(out);
  }

  std::filesystem::remove(out);
}

TEST_CASE("DXF export keeps a built-in's dashes when the file's record says 73 0",
          "[dxf][roundtrip][filter][linetype][ltype]") {
  ensureSettings();
  const std::string out = tmpFile("zero_dash_ltype_out.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  importBuiltinLTypeFixture(graphic, "zero_dash_ltype_src.dxf",
                            "70\n0\n3\nFile dashes\n72\n65\n73\n0\n40\n0.0\n");
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // The file's own description is kept.
  CHECK(ltypeRecordGroupValues(out, "DASHED", "3") ==
        std::vector<std::string>{"File dashes"});
  checkBuiltinDashed(out);

  std::filesystem::remove(out);
}

#ifdef DWGSUPPORT
TEST_CASE("DWG export keeps a built-in's dashes when the file's record has none",
          "[dwg][roundtrip][filter][linetype][ltype]") {
  ensureSettings();
  const std::string dwg = tmpFile("name_only_ltype.dwg");
  std::filesystem::remove(dwg);

  RS_Graphic graphic;
  importBuiltinLTypeFixture(graphic, "name_only_ltype_dwg_src.dxf", "70\n0\n");
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
  }
  const DRW_LType *dashed =
      fromDwg.dwgAdvancedMetadata().findLineTypeTableEntryByName("DASHED");
  REQUIRE(dashed != nullptr);
  CHECK(dashed->path.size() == 2);

  std::filesystem::remove(dwg);
}

// A native MLEADER resolves its linetype through the DWG source handle of the
// archived record. A name without dashes of its own takes that record whole,
// description included; DASHED takes only its table entry part.
TEST_CASE("DWG export keeps the linetype record a native MLEADER refers to",
          "[dwg][filter][linetype][ltype][mleader]") {
  ensureSettings();
  const std::string dwg = tmpFile("ltype_mleader.dwg");

  struct Expected {
    const char *name;
    std::size_t dashes;
    const char *desc;
  };
  for (const Expected &expected :
       {Expected{"ByBlock", 0, ""}, Expected{"CONTINUOUS", 0, ""},
        Expected{"DASHED", 2, "Dashed _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _"}}) {
    INFO("LTYPE " << expected.name);
    std::filesystem::remove(dwg);

    RS_Graphic graphic;
    graphic.initForNewDocument();
    DRW_LType record;
    record.handle = 0xBA0u;
    record.name = expected.name;
    graphic.dwgAdvancedMetadata().addLineTypeName(record);

    LC_MLeaderData data;
    data.hasTextContents = true;
    data.textLabel = QStringLiteral("Leader");
    data.textLocation = RS_Vector(20.0, 5.0, 0.0);
    data.contentBasePoint = data.textLocation;
    data.textHeight = 2.0;
    data.dwgLeaderLineTypeHandle = record.handle;
    LC_MLeaderRoot root;
    root.connectionPoint = data.textLocation;
    root.direction = RS_Vector(1.0, 0.0, 0.0);
    LC_MLeaderLine line;
    line.points = {RS_Vector(0.0, 0.0, 0.0), RS_Vector(10.0, 5.0, 0.0)};
    root.leaderLines.push_back(std::move(line));
    data.roots.push_back(std::move(root));
    auto *leader = new LC_MLeader(&graphic, std::move(data));
    leader->setSourceHandle(0xBA1u);
    graphic.addEntity(leader);
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwg),
                                RS2::FormatDWG2013));
    }

    RS_Graphic fromDwg;
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(fromDwg, QString::fromStdString(dwg),
                                RS2::FormatDWG));
    }
    const auto &metadata = fromDwg.dwgAdvancedMetadata();
    const auto *imported = dynamic_cast<LC_MLeader *>(fromDwg.firstEntity());
    REQUIRE(imported != nullptr);
    const std::string leaderLType = metadata.lineTypeNameForHandle(
        imported->getData().dwgLeaderLineTypeHandle);
    CHECK(QString::fromStdString(leaderLType)
              .compare(QLatin1String(expected.name), Qt::CaseInsensitive) == 0);
    const DRW_LType *written =
        metadata.findLineTypeTableEntryByName(expected.name);
    REQUIRE(written != nullptr);
    CHECK(written->path.size() == expected.dashes);
    CHECK(written->desc == expected.desc);
  }

  std::filesystem::remove(dwg);
}
#endif

TEST_CASE("DXF import maps ACAD_ISO09W100 to PHANTOM and keeps its alias name on write",
          "[dxf][roundtrip][filter][linetype]") {
  ensureSettings();
  const std::string src = tmpFile("iso09_src.dxf");
  const std::string out = tmpFile("iso09_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // acadiso.lin: ACAD_ISO09W100 "ISO long-dash double-short-dash"
  // A,24,-3,6,-3,6,-3. Like the other ACAD_ISO aliases it is drawn as the
  // family it maps to, while the entity and the layer keep the alias name.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLTYPE\n70\n1\n"
            "0\nLTYPE\n2\nACAD_ISO09W100\n70\n0\n"
            "3\nISO long-dash double-short-dash\n"
            "72\n65\n73\n6\n40\n45.0\n"
            "49\n24.0\n74\n0\n49\n-3.0\n74\n0\n49\n6.0\n74\n0\n"
            "49\n-3.0\n74\n0\n49\n6.0\n74\n0\n49\n-3.0\n74\n0\n"
            "0\nENDTAB\n0\nTABLE\n2\nLAYER\n70\n2\n"
            "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
            "0\nLAYER\n2\nISO09_LAYER\n70\n0\n62\n7\n6\nACAD_ISO09W100\n"
            "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\nISO09_LAYER\n6\nACAD_ISO09W100\n"
            "10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  const auto *layer = graphic.findLayer(QStringLiteral("ISO09_LAYER"));
  REQUIRE(layer != nullptr);
  CHECK(layer->getPen().getLineType() == RS2::PhantomLine);
  RS_Entity *line = graphic.firstEntity();
  REQUIRE(line != nullptr);
  CHECK(line->getPen(false).getLineType() == RS2::PhantomLine);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"ACAD_ISO09W100"});
  CHECK(namedRecordGroupValues(out, "LAYER", "ISO09_LAYER", "6") ==
        std::vector<std::string>{"ACAD_ISO09W100"});
  CHECK(ltypeRecordGroupValues(out, "PHANTOM", "73") ==
        std::vector<std::string>{"6"});
  // The imported ISO09 record is re-emitted as it came in, once.
  CHECK(ltypeRecordGroupValues(out, "ACAD_ISO09W100", "73") ==
        std::vector<std::string>{"6"});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("An open spline written to R12 ends its polyline at the spline's end",
          "[dxf][filter][spline][r12]") {
  ensureSettings();

  // away from the origin, where the polyline used to end
  RS_Graphic graphic;
  RS_SplineData data(3, false);
  data.controlPoints = {{100., 50.}, {110., 70.}, {130., 40.}, {140., 60.}};
  data.knotslist = {0., 0., 0., 0., 1., 1., 1., 1.};
  data.weights.assign(4, 1.);
  auto *spline = new RS_Spline(&graphic, data);
  graphic.addEntity(spline);
  spline->update();
  REQUIRE(spline->count() > 0);

  const std::string path = tmpFile("open_spline_r12.dxf");
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(path), RS2::FormatDXFRW12));
  }
  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, QString::fromStdString(path), RS2::FormatDXFRW));
  }
  std::filesystem::remove(path);

  std::vector<RS_Vector> vertices;
  for (RS_Entity *e : reloaded) {
    REQUIRE(e->rtti() == RS2::EntityPolyline);
    CHECK_FALSE(static_cast<RS_Polyline *>(e)->isClosed());
    for (RS_Entity *segment : *static_cast<RS_EntityContainer *>(e)) {
      if (vertices.empty())
        vertices.push_back(segment->getStartpoint());
      vertices.push_back(segment->getEndpoint());
    }
  }
  REQUIRE(vertices.size() > 2);
  CHECK(vertices.front().distanceTo(RS_Vector(100., 50.)) < 1e-9);
  CHECK(vertices.back().distanceTo(RS_Vector(140., 60.)) < 1e-9);
  for (const RS_Vector &v : vertices) {
    INFO("vertex " << v);
    CHECK(v.distanceTo(RS_Vector(0., 0.)) > 1.);
  }
}

// Linetype names -------------------------------------------------------------

namespace {

// "Oelfarbe" and "OELFARBE" with U+00D6 as UTF-8, split so that \x cannot
// swallow the next letter.
const char *const kOelfarbe = "\xC3\x96" "lfarbe";
const char *const kOelfarbeUpper = "\xC3\x96" "LFARBE";

// An R12 drawing with linetypes of its own: vendor records with plain and edge
// patterns, a pair of case twins, a non-ASCII name, HIDDEN redefined, a
// name-only DASHED record, names with no record on an entity, a layer and a
// block member, an empty and a blank group 6, and two ISO aliases with no
// record. Each entity of interest sits on a layer of its own. Group 73 must
// match the number of 49s, or the reader rejects the file.
const char *const kNamedR12Fixture =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLTYPE\n70\n11\n"
    "0\nLTYPE\n2\nVENDOR_TAB\n70\n0\n3\nVendor tabulator\n72\n65\n73\n2\n"
    "40\n40.0\n49\n20.0\n49\n-20.0\n"
    "0\nLTYPE\n2\nVENDOR_UTL\n70\n0\n3\nVendor utility\n72\n65\n73\n4\n"
    "40\n62.0\n49\n20.0\n49\n-20.0\n49\n2.0\n49\n-20.0\n"
    "0\nLTYPE\n2\nVENDOR_STOP\n70\n0\n3\nVendor stop\n72\n65\n73\n2\n"
    "40\n12.0\n49\n2.0\n49\n-10.0\n"
    "0\nLTYPE\n2\nVendor_mixedCase\n70\n0\n3\nVendor mixed case\n72\n65\n"
    "73\n2\n40\n40.0\n49\n20.0\n49\n-20.0\n"
    "0\nLTYPE\n2\nVENDOR_MIXEDCASE\n70\n0\n3\nVendor mixed case twin\n"
    "72\n65\n73\n2\n40\n60.0\n49\n30.0\n49\n-30.0\n"
    "0\nLTYPE\n2\nVENDOR_NEG\n70\n0\n3\nVendor all gaps\n72\n65\n73\n2\n"
    "40\n40.0\n49\n-20.0\n49\n-20.0\n"
    "0\nLTYPE\n2\nVENDOR_ODD\n70\n0\n3\nVendor odd\n72\n65\n73\n3\n"
    "40\n15.0\n49\n10.0\n49\n-5.0\n49\n0.0\n"
    "0\nLTYPE\n2\nVENDOR_ZERO\n70\n0\n3\nVendor zero\n72\n65\n73\n2\n"
    "40\n0.0\n49\n0.0\n49\n0.0\n"
    "0\nLTYPE\n2\n" "\xC3\x96" "lfarbe\n70\n0\n3\nVendor non-ASCII\n72\n65\n"
    "73\n2\n40\n40.0\n49\n20.0\n49\n-20.0\n"
    "0\nLTYPE\n2\nHIDDEN\n70\n0\n3\nVendor hidden\n72\n65\n73\n2\n"
    "40\n96.0\n49\n64.0\n49\n-32.0\n"
    "0\nLTYPE\n2\nDASHED\n70\n0\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nLAYER\n70\n17\n"
    "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_VENDOR\n70\n0\n62\n7\n6\nVENDOR_TAB\n"
    "0\nLAYER\n2\nL_MIXED\n70\n0\n62\n7\n6\nVendor_mixedCase\n"
    "0\nLAYER\n2\nL_NOREC\n70\n0\n62\n7\n6\nVENDOR_LAYER_NOREC\n"
    "0\nLAYER\n2\nL_UTL\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_STOP\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_NEG\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_ODD\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_ZERO\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_OEL\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_HIDDEN\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_DASHED\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_ENT_NOREC\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_EMPTY6\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_BLANK6\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_ISO09\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n2\nL_ISO02\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nBLOCKS\n"
    "0\nBLOCK\n8\n0\n2\nB\n70\n0\n10\n0.0\n20\n0.0\n30\n0.0\n3\nB\n1\n\n"
    "0\nLINE\n8\n0\n6\nVENDOR_BLK_NOREC\n"
    "10\n0.0\n20\n0.0\n11\n5.0\n21\n0.0\n"
    "0\nENDBLK\n8\n0\n"
    "0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n8\nL_VENDOR\n6\nVENDOR_TAB\n"
    "10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
    "0\nLINE\n8\nL_UTL\n6\nVENDOR_UTL\n"
    "10\n0.0\n20\n10.0\n11\n10.0\n21\n10.0\n"
    "0\nLINE\n8\nL_STOP\n6\nVENDOR_STOP\n"
    "10\n0.0\n20\n20.0\n11\n10.0\n21\n20.0\n"
    "0\nLINE\n8\nL_MIXED\n6\nVENDOR_MIXEDCASE\n"
    "10\n0.0\n20\n30.0\n11\n10.0\n21\n30.0\n"
    "0\nLINE\n8\nL_NEG\n6\nVENDOR_NEG\n"
    "10\n0.0\n20\n40.0\n11\n10.0\n21\n40.0\n"
    "0\nLINE\n8\nL_ODD\n6\nVENDOR_ODD\n"
    "10\n0.0\n20\n50.0\n11\n10.0\n21\n50.0\n"
    "0\nLINE\n8\nL_ZERO\n6\nVENDOR_ZERO\n"
    "10\n0.0\n20\n60.0\n11\n10.0\n21\n60.0\n"
    "0\nLINE\n8\nL_OEL\n6\n" "\xC3\x96" "LFARBE\n"
    "10\n0.0\n20\n70.0\n11\n10.0\n21\n70.0\n"
    "0\nLINE\n8\nL_HIDDEN\n6\nHIDDEN\n"
    "10\n0.0\n20\n80.0\n11\n10.0\n21\n80.0\n"
    "0\nLINE\n8\nL_DASHED\n6\nDASHED\n"
    "10\n0.0\n20\n90.0\n11\n10.0\n21\n90.0\n"
    "0\nLINE\n8\nL_ENT_NOREC\n6\nVENDOR_NOREC\n"
    "10\n0.0\n20\n100.0\n11\n10.0\n21\n100.0\n"
    "0\nLINE\n8\nL_EMPTY6\n6\n\n"
    "10\n0.0\n20\n110.0\n11\n10.0\n21\n110.0\n"
    "0\nLINE\n8\nL_BLANK6\n6\n   \n"
    "10\n0.0\n20\n120.0\n11\n10.0\n21\n120.0\n"
    "0\nLINE\n8\nL_ISO09\n6\nACAD_ISO09W100\n"
    "10\n0.0\n20\n130.0\n11\n10.0\n21\n130.0\n"
    "0\nLINE\n8\nL_ISO02\n6\nACAD_ISO02W100\n"
    "10\n0.0\n20\n140.0\n11\n10.0\n21\n140.0\n"
    "0\nLINE\n8\nL_NOREC\n"
    "10\n0.0\n20\n150.0\n11\n10.0\n21\n150.0\n"
    "0\nINSERT\n8\n0\n2\nB\n10\n0.0\n20\n160.0\n30\n0.0\n"
    "0\nENDSEC\n0\nEOF\n";

// An R2000 drawing: a complex LTYPE record with a text segment and an
// application group, a DIMSTYLE whose 345 points at an LTYPE, and an
// MLINESTYLE whose first element names a linetype nothing else names.
const char *const kNamedR2000Fixture =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLTYPE\n5\n5\n330\n0\n"
    "100\nAcDbSymbolTable\n70\n2\n"
    "0\nLTYPE\n5\n40\n330\n5\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
    "2\nVENDOR_TAB\n70\n0\n3\nVendor tabulator\n72\n65\n73\n2\n40\n40.0\n"
    "49\n20.0\n74\n0\n49\n-20.0\n74\n0\n"
    "0\nLTYPE\n5\n41\n330\n5\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
    "2\nVENDOR_CPLX\n70\n0\n3\nVendor complex\n72\n65\n73\n2\n40\n30.0\n"
    "49\n20.0\n74\n0\n"
    "49\n-10.0\n74\n2\n75\n0\n340\n4A\n46\n1.0\n50\n0.0\n44\n-5.0\n45\n0.0\n"
    "9\nGAS\n"
    "102\n{LTYPE_APP\n310\nCAFE\n102\n}\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nSTYLE\n5\n3\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
    "0\nSTYLE\n5\n4A\n330\n3\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbTextStyleTableRecord\n"
    "2\nGASSTYLE\n70\n0\n40\n0.0\n41\n1.0\n50\n0.0\n71\n0\n42\n2.5\n"
    "3\ntxt\n4\n\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
    "0\nLAYER\n5\n50\n330\n2\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
    "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nDIMSTYLE\n5\nA\n330\n0\n"
    "100\nAcDbSymbolTable\n70\n1\n100\nAcDbDimStyleTable\n71\n0\n"
    "0\nDIMSTYLE\n105\n31\n330\nA\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbDimStyleTableRecord\n"
    "2\nVENDOR_DIM\n70\n0\n345\n40\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\n100\n330\n1F\n100\nAcDbEntity\n8\n0\n6\nVENDOR_CPLX\n"
    "100\nAcDbLine\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
    "0\nENDSEC\n"
    // Handle 60 and owner C are free in this fixture.
    "0\nSECTION\n2\nOBJECTS\n"
    "0\nMLINESTYLE\n5\n60\n330\nC\n100\nAcDbMlineStyle\n"
    "2\nVENDOR_MLS_STYLE\n70\n0\n3\n\n62\n256\n51\n90.0\n52\n90.0\n71\n2\n"
    "49\n0.5\n62\n1\n6\nVENDOR_MLS\n"
    "49\n-0.5\n62\n1\n6\nBYLAYER\n"
    "0\nENDSEC\n0\nEOF\n";

std::string writeFixture(const char *suffix, const char *text) {
  const std::string path = tmpFile(suffix);
  std::filesystem::remove(path);
  writeText(path, text);
  return path;
}

// The first model-space entity on `layerName`.
RS_Entity *entityOnLayer(RS_Graphic &g, const QString &layerName) {
  for (RS_Entity *e : g) {
    if (e == nullptr)
      continue;
    const RS_Layer *layer = e->getLayer();
    if (layer != nullptr && layer->getName() == layerName)
      return e;
  }
  return nullptr;
}

std::size_t countValues(const std::vector<std::string> &values,
                        const std::string &value) {
  return static_cast<std::size_t>(
      std::count(values.cbegin(), values.cend(), value));
}

std::vector<std::string> sortedValues(std::vector<std::string> values) {
  std::sort(values.begin(), values.end());
  return values;
}

// Exposes the protected undo API, the way rs_undo_tests.cpp does.
class UndoGraphic : public RS_Graphic {
public:
  using RS_Document::endUndoCycle;
  using RS_Document::startUndoCycle;
  using RS_Undo::addUndoable;
};

} // namespace

TEST_CASE("DXF import keeps a custom linetype name on entity and layer pens",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t1_src.dxf", kNamedR12Fixture);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  RS_Entity *line = entityOnLayer(graphic, QStringLiteral("L_VENDOR"));
  REQUIRE(line != nullptr);
  // The enum is only the nearest built-in, and a vendor pattern has none.
  CHECK(line->getPen(false).getLineTypeName() == QStringLiteral("VENDOR_TAB"));
  CHECK(line->getPen(false).getLineType() == RS2::SolidLine);
  CHECK(line->getPen(false).getLineTypeId() != 0);

  const RS_Layer *layer = graphic.findLayer(QStringLiteral("L_VENDOR"));
  REQUIRE(layer != nullptr);
  CHECK(layer->getPen().getLineTypeName() == QStringLiteral("VENDOR_TAB"));
  CHECK(layer->getPen().getLineTypeId() != 0);

  std::filesystem::remove(src);
}

TEST_CASE("DXF round-trip of a custom linetype is stable in both DXF versions",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t2_src.dxf", kNamedR12Fixture);

  for (const auto &[version, label] :
       std::initializer_list<std::pair<RS2::FormatType, const char *>>{
           {RS2::FormatDXFRW, "AC1021"}, {RS2::FormatDXFRW12, "AC1009"}}) {
    INFO("export version " << label);
    const bool r12 = version == RS2::FormatDXFRW12;
    const std::string out =
        tmpFile(r12 ? "named_t2_out12.dxf" : "named_t2_out.dxf");
    const std::string out2 =
        tmpFile(r12 ? "named_t2_out12_again.dxf" : "named_t2_out_again.dxf");
    std::filesystem::remove(out);
    std::filesystem::remove(out2);

    RS_Graphic graphic;
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                                RS2::FormatDXFRW));
    }
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), version));
    }

    // One record, referenced by the entity and by the layer.
    CHECK(countValues(recordGroupValues(out, "LINE", "6"), "VENDOR_TAB") == 1);
    CHECK(namedRecordGroupValues(out, "LAYER", "L_VENDOR", "6") ==
          std::vector<std::string>{"VENDOR_TAB"});
    CHECK(ltypeRecordGroupValues(out, "VENDOR_TAB", "73") ==
          std::vector<std::string>{"2"});
    const auto length = ltypeRecordGroupValues(out, "VENDOR_TAB", "40");
    REQUIRE(length.size() == 1);
    CHECK(std::stod(length[0]) == Catch::Approx(40.0));
    const auto dashes = ltypeRecordGroupValues(out, "VENDOR_TAB", "49");
    REQUIRE(dashes.size() == 2);
    CHECK(std::stod(dashes[0]) == Catch::Approx(20.0));
    CHECK(std::stod(dashes[1]) == Catch::Approx(-20.0));

    // The three reserved records are libdxfrw's own, once each and with no
    // dashes. R12 has no table handles and no segment groups, and upper-cases
    // every name.
    if (r12) {
      CHECK(recordGroupValues(out, "LTYPE", "5").empty());
      CHECK(ltypeRecordGroupValues(out, "VENDOR_TAB", "74").empty());
      CHECK(ltypeRecordGroupValues(out, "BYLAYER", "73") ==
            std::vector<std::string>{"0"});
      CHECK(ltypeRecordGroupValues(out, "BYBLOCK", "73") ==
            std::vector<std::string>{"0"});
      CHECK(ltypeRecordGroupValues(out, "CONTINUOUS", "73") ==
            std::vector<std::string>{"0"});
    } else {
      CHECK(ltypeRecordGroupValues(out, "ByLayer", "73") ==
            std::vector<std::string>{"0"});
      CHECK(ltypeRecordGroupValues(out, "ByBlock", "73") ==
            std::vector<std::string>{"0"});
      CHECK(ltypeRecordGroupValues(out, "Continuous", "73") ==
            std::vector<std::string>{"0"});
    }

    RS_Graphic reimported;
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(reimported, QString::fromStdString(out),
                                RS2::FormatDXFRW));
    }
    RS_Entity *line = entityOnLayer(reimported, QStringLiteral("L_VENDOR"));
    REQUIRE(line != nullptr);
    CHECK(line->getPen(false).getLineTypeName() ==
          QStringLiteral("VENDOR_TAB"));

    // A second export writes the same record and references, once each.
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(reimported, QString::fromStdString(out2),
                                version));
    }
    CHECK(ltypeRecordGroupValues(out2, "VENDOR_TAB", "73") ==
          std::vector<std::string>{"2"});
    CHECK(ltypeRecordGroupValues(out2, "VENDOR_TAB", "49") ==
          ltypeRecordGroupValues(out, "VENDOR_TAB", "49"));
    CHECK(namedRecordGroupValues(out2, "LAYER", "L_VENDOR", "6") ==
          std::vector<std::string>{"VENDOR_TAB"});
    CHECK(countValues(recordGroupValues(out2, "LINE", "6"), "VENDOR_TAB") == 1);

    std::filesystem::remove(out);
    std::filesystem::remove(out2);
  }

  std::filesystem::remove(src);
}

TEST_CASE("DXF unregistered linetype names survive as empty marker records",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t3_src.dxf", kNamedR12Fixture);
  const std::string out = tmpFile("named_t3_out.dxf");
  std::filesystem::remove(out);

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

  // A name with no record of its own gets one with no dashes, whether an
  // entity, a layer or a block member names it.
  for (const char *name : {"VENDOR_NOREC", "VENDOR_LAYER_NOREC",
                           "VENDOR_BLK_NOREC"}) {
    INFO("marker record " << name);
    CHECK(ltypeRecordGroupValues(out, name, "73") ==
          std::vector<std::string>{"0"});
  }
  const auto entityNames = recordGroupValues(out, "LINE", "6");
  CHECK(countValues(entityNames, "VENDOR_NOREC") == 1);
  CHECK(namedRecordGroupValues(out, "LAYER", "L_NOREC", "6") ==
        std::vector<std::string>{"VENDOR_LAYER_NOREC"});
  // The block member is written with the blocks, so it is a LINE too.
  CHECK(countValues(entityNames, "VENDOR_BLK_NOREC") == 1);

  RS_Block *block = graphic.findBlock(QStringLiteral("B"));
  REQUIRE(block != nullptr);
  RS_Entity *member = block->firstEntity();
  REQUIRE(member != nullptr);
  CHECK(member->getPen(false).getLineTypeName() ==
        QStringLiteral("VENDOR_BLK_NOREC"));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF empty group 6 is ByLayer and ISO aliases keep drawable metrics",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t3ab_src.dxf", kNamedR12Fixture);
  const std::string out = tmpFile("named_t3ab_out.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }

  // An empty group 6 and a blank one both land as ByLayer, so no record with
  // an empty name can appear.
  RS_Entity *empty = entityOnLayer(graphic, QStringLiteral("L_EMPTY6"));
  REQUIRE(empty != nullptr);
  CHECK(empty->getPen(false).isLineTypeByLayer());
  RS_Entity *blank = entityOnLayer(graphic, QStringLiteral("L_BLANK6"));
  REQUIRE(blank != nullptr);
  CHECK(blank->getPen(false).isLineTypeByLayer());

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(ltypeRecordGroupValues(out, "", "73").empty());
  // The three reserved names never get a second record.
  CHECK(ltypeRecordGroupValues(out, "ByLayer", "73").size() == 1);
  CHECK(ltypeRecordGroupValues(out, "ByBlock", "73").size() == 1);
  CHECK(ltypeRecordGroupValues(out, "Continuous", "73").size() == 1);

  // An ISO alias with no record is drawn as its built-in family, so the record
  // written for it carries that family's dashes (DASHED for ISO02), not an
  // empty pattern that other programs would draw solid.
  CHECK(ltypeRecordGroupValues(out, "ACAD_ISO02W100", "73") ==
        std::vector<std::string>{"2"});
  const auto isoDashes = ltypeRecordGroupValues(out, "ACAD_ISO02W100", "49");
  REQUIRE(isoDashes.size() == 2);
  CHECK(std::stod(isoDashes[0]) == Catch::Approx(12.7));
  CHECK(std::stod(isoDashes[1]) == Catch::Approx(-6.35));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF export synthesises no record for a name only undo memory holds",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string out = tmpFile("named_t3c_out.dxf");

  UndoGraphic graphic;
  graphic.initForNewDocument();
  auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 0.0),
                                                 RS_Vector(10.0, 0.0)));
  RS_Pen pen;
  pen.setLineTypeName(QStringLiteral("VENDOR_MEMORY"));
  line->setPen(pen);
  graphic.addEntity(line);

  auto exportAndRead = [&](const char *why,
                           const char *name = "VENDOR_MEMORY") {
    INFO(why);
    std::filesystem::remove(out);
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
    return ltypeRecordGroupValues(out, name, "73");
  };

  // Only a pen names it, so a record is written while the entity is alive...
  CHECK(exportAndRead("live entity") == std::vector<std::string>{"0"});

  // ...and none once it is deleted, as the entity itself is not written.
  graphic.startUndoCycle();
  line->markDeleted();
  graphic.addUndoable(line);
  graphic.endUndoCycle();
  CHECK(exportAndRead("deleted entity").empty());

  // Undo brings the reference back, and the record with it.
  REQUIRE(graphic.undo());
  REQUIRE(line->isAlive());
  CHECK(exportAndRead("undone delete") == std::vector<std::string>{"0"});

  // A deleted block is not written either, so its members name nothing.
  auto *block = new RS_Block(
      &graphic, RS_BlockData(QStringLiteral("BDEL"), RS_Vector(0.0, 0.0),
                             false));
  auto *member = new RS_Line(block, RS_LineData(RS_Vector(0.0, 0.0),
                                                RS_Vector(5.0, 0.0)));
  RS_Pen memberPen;
  memberPen.setLineTypeName(QStringLiteral("VENDOR_BLOCK_MEMORY"));
  member->setPen(memberPen);
  block->addEntity(member);
  graphic.addBlock(block);
  CHECK(exportAndRead("live block", "VENDOR_BLOCK_MEMORY") ==
        std::vector<std::string>{"0"});
  block->markDeleted();
  CHECK(exportAndRead("deleted block", "VENDOR_BLOCK_MEMORY").empty());

  std::filesystem::remove(out);
}

#ifdef DWGSUPPORT
TEST_CASE("DWG round-trip keeps unregistered linetype names",
          "[dwg][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t3d_src.dxf", kNamedR12Fixture);
  const std::string dwg = tmpFile("named_t3d_out.dwg");
  std::filesystem::remove(dwg);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
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
  }

  RS_Entity *line = entityOnLayer(fromDwg, QStringLiteral("L_ENT_NOREC"));
  REQUIRE(line != nullptr);
  CHECK(line->getPen(false).getLineTypeName() ==
        QStringLiteral("VENDOR_NOREC"));
  CHECK(line->getPen(false).getLineType() == RS2::SolidLine);

  // The DWG writer falls back to CONTINUOUS for a layer whose linetype it
  // cannot find, silently, so the layer name is what catches a missing record.
  const RS_Layer *layer = fromDwg.findLayer(QStringLiteral("L_NOREC"));
  REQUIRE(layer != nullptr);
  CHECK(layer->getPen().getLineTypeName() ==
        QStringLiteral("VENDOR_LAYER_NOREC"));

  RS_Block *block = fromDwg.findBlock(QStringLiteral("B"));
  REQUIRE(block != nullptr);
  RS_Entity *member = block->firstEntity();
  REQUIRE(member != nullptr);
  CHECK(member->getPen(false).getLineTypeName() ==
        QStringLiteral("VENDOR_BLK_NOREC"));

  std::filesystem::remove(src);
  std::filesystem::remove(dwg);
}
#endif // DWGSUPPORT

// R2000+ writes a plain 2D polyline (no ellipse segment) as LWPOLYLINE, whose
// own writer (RS_FilterDXFRW::writeLWPolyline) already added the last
// segment's endpoint correctly before this fix. The old-style POLYLINE/VERTEX
// writer this fix touches (RS_FilterDXFRW::writePolyline) is reached only for
// an R12 target, or a polyline holding an ellipse segment at any version
// (writeLWPolyline's has_ellipse check) -- an R12 target is the simpler of
// the two to construct.
TEST_CASE("An open polyline's VERTEX and SEQEND carry its own layer and "
          "linetype in R12",
          "[dxf][roundtrip][filter][polyline][linetype][named]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  graphic.addLayer(new RS_Layer(QStringLiteral("PLINE_LAYER")));

  auto *polyline = new RS_Polyline(&graphic);
  polyline->setLayer(QStringLiteral("PLINE_LAYER"));
  polyline->setPen(
      RS_Pen(RS_Color(Qt::black), RS2::WidthByLayer, RS2::LineByBlock));
  polyline->addVertex(RS_Vector(0.0, 0.0, 0.0));
  polyline->addVertex(RS_Vector(10.0, 0.0, 0.0));
  graphic.addEntity(polyline);

  const std::string out12 = tmpFile("polyline_vertex_r12.dxf");
  std::filesystem::remove(out12);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out12),
                              RS2::FormatDXFRW12));
  }

  // R12 upper-cases every name it writes (dxfWriter::writeUtf8Caps).
  CHECK(sortedValues(recordGroupValues(out12, "VERTEX", "8")) ==
        std::vector<std::string>{"PLINE_LAYER", "PLINE_LAYER"});
  CHECK(sortedValues(recordGroupValues(out12, "VERTEX", "6")) ==
        std::vector<std::string>{"BYBLOCK", "BYBLOCK"});
  CHECK(recordGroupValues(out12, "SEQEND", "8") ==
        std::vector<std::string>{"PLINE_LAYER"});
  CHECK(recordGroupValues(out12, "SEQEND", "6") ==
        std::vector<std::string>{"BYBLOCK"});

  std::filesystem::remove(out12);
}

TEST_CASE("An open polyline keeps every vertex on DXF re-save; a closed "
          "one is unchanged",
          "[dxf][roundtrip][filter][polyline]") {
  ensureSettings();

  RS_Graphic openGraphic;
  openGraphic.initForNewDocument();
  auto *openPolyline = new RS_Polyline(&openGraphic);
  openPolyline->addVertex(RS_Vector(0.0, 0.0, 0.0));
  openPolyline->addVertex(RS_Vector(10.0, 0.0, 0.0));
  openPolyline->addVertex(RS_Vector(10.0, 10.0, 0.0));
  openGraphic.addEntity(openPolyline);

  RS_Graphic closedGraphic;
  closedGraphic.initForNewDocument();
  auto *closedPolyline = new RS_Polyline(&closedGraphic);
  closedPolyline->addVertex(RS_Vector(0.0, 0.0, 0.0));
  closedPolyline->addVertex(RS_Vector(10.0, 0.0, 0.0));
  closedPolyline->addVertex(RS_Vector(10.0, 10.0, 0.0));
  // The two-argument overload both sets the flag and adds the explicit
  // closing segment (RS_Polyline::endPolyline()); the one-argument overload
  // only sets the flag, leaving a triangle with just two sides.
  closedPolyline->setClosed(true, 0.0);
  closedGraphic.addEntity(closedPolyline);

  const std::string openOut = tmpFile("polyline_open_r12.dxf");
  const std::string closedOut = tmpFile("polyline_closed_r12.dxf");
  std::filesystem::remove(openOut);
  std::filesystem::remove(closedOut);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(openGraphic, QString::fromStdString(openOut),
                              RS2::FormatDXFRW12));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(closedGraphic, QString::fromStdString(closedOut),
                              RS2::FormatDXFRW12));
  }
  // Three points: an open polyline (two segments) needs a vertex for each
  // segment's start plus one more for the last segment's end; a closed one
  // (three segments, the third closing back to the first point) needs one
  // per segment's start and no more, since the last segment's end coincides
  // with the first vertex already in the list.
  CHECK(countRecords(openOut, "VERTEX") == 3);
  CHECK(countRecords(closedOut, "VERTEX") == 3);
  std::filesystem::remove(openOut);
  std::filesystem::remove(closedOut);
}

#ifdef DWGSUPPORT
TEST_CASE("An open polyline keeps every vertex through a DWG round-trip",
          "[dwg][roundtrip][filter][polyline]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *polyline = new RS_Polyline(&graphic);
  polyline->addVertex(RS_Vector(0.0, 0.0, 0.0));
  polyline->addVertex(RS_Vector(10.0, 0.0, 0.0));
  graphic.addEntity(polyline);

  const std::string dwg = tmpFile("polyline_open_roundtrip.dwg");
  std::filesystem::remove(dwg);
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
  }
  RS_Entity *reloaded = fromDwg.firstEntity(RS2::ResolveNone);
  REQUIRE(reloaded != nullptr);
  REQUIRE(reloaded->rtti() == RS2::EntityPolyline);
  auto *reloadedPolyline = static_cast<RS_Polyline *>(reloaded);
  CHECK(reloadedPolyline->getStartpoint().distanceTo(RS_Vector(0.0, 0.0)) <
        RS_TOLERANCE);
  CHECK(reloadedPolyline->getEndpoint().distanceTo(RS_Vector(10.0, 0.0)) <
        RS_TOLERANCE);

  std::filesystem::remove(dwg);
}
#endif // DWGSUPPORT

TEST_CASE("DXF ISO alias keeps its literal name instead of the family name",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t5_src.dxf", kNamedR12Fixture);
  const std::string out = tmpFile("named_t5_out.dxf");
  const std::string out12 = tmpFile("named_t5_out12.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(out12);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  RS_Entity *line = entityOnLayer(graphic, QStringLiteral("L_ISO09"));
  REQUIRE(line != nullptr);
  // The alias is drawn as the PHANTOM family...
  CHECK(line->getPen(false).getLineType() == RS2::PhantomLine);
  // ...but the pen keeps the name the file used, and that is what is written.
  CHECK(line->getPen(false).getLineTypeName() ==
        QStringLiteral("ACAD_ISO09W100"));
  CHECK(line->getPen(false).getLineTypeId() != 0);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(countValues(recordGroupValues(out, "LINE", "6"), "ACAD_ISO09W100") == 1);
  CHECK(countValues(recordGroupValues(out, "LINE", "6"), "PHANTOM") == 0);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out12),
                              RS2::FormatDXFRW12));
  }
  // Already upper-case, so R12 writes the very same name.
  CHECK(countValues(recordGroupValues(out12, "LINE", "6"), "ACAD_ISO09W100") ==
        1);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out12);
}

TEST_CASE("DXF complex linetype record travels through export opaque",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t8_src.dxf", kNamedR2000Fixture);
  const std::string out = tmpFile("named_t8_out.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW2000));
  }

  // Handles are minted afresh and 73/40 recomputed; the pattern and every
  // segment decoration must survive.
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"VENDOR_CPLX"});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "73") ==
        std::vector<std::string>{"2"});
  const auto dashes = ltypeRecordGroupValues(out, "VENDOR_CPLX", "49");
  REQUIRE(dashes.size() == 2);
  CHECK(std::stod(dashes[0]) == Catch::Approx(20.0));
  CHECK(std::stod(dashes[1]) == Catch::Approx(-10.0));
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "9") ==
        std::vector<std::string>{"GAS"});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "74") ==
        std::vector<std::string>{"0", "2"});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "75") ==
        std::vector<std::string>{"0"});
  const auto scale = ltypeRecordGroupValues(out, "VENDOR_CPLX", "46");
  REQUIRE(scale.size() == 1);
  CHECK(std::stod(scale[0]) == Catch::Approx(1.0));
  const auto rotation = ltypeRecordGroupValues(out, "VENDOR_CPLX", "50");
  REQUIRE(rotation.size() == 1);
  CHECK(std::stod(rotation[0]) == Catch::Approx(0.0));
  const auto xOffset = ltypeRecordGroupValues(out, "VENDOR_CPLX", "44");
  REQUIRE(xOffset.size() == 1);
  CHECK(std::stod(xOffset[0]) == Catch::Approx(-5.0));
  const auto yOffset = ltypeRecordGroupValues(out, "VENDOR_CPLX", "45");
  REQUIRE(yOffset.size() == 1);
  CHECK(std::stod(yOffset[0]) == Catch::Approx(0.0));
  // The application group survives with its binary chunk.
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "310") ==
        std::vector<std::string>{"CAFE"});
  // Group 340 is not asserted here: it is the style handle of a complex
  // segment, which export now resolves through the reference resolver to the
  // STYLE record it writes.

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF edge linetype patterns and names are written verbatim",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t13_src.dxf", kNamedR12Fixture);
  const std::string out = tmpFile("named_t13_out.dxf");
  std::filesystem::remove(out);

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

  // An all-gap pattern, an odd count ending in a dot, and an all-zero pattern
  // are written as the file gave them; nothing drawing-side may reshape them.
  struct EdgeRecord {
    const char *name;
    const char *size;
    std::vector<double> dashes;
  };
  for (const EdgeRecord &record : {
           EdgeRecord{"VENDOR_NEG", "2", {-20.0, -20.0}},
           EdgeRecord{"VENDOR_ODD", "3", {10.0, -5.0, 0.0}},
           EdgeRecord{"VENDOR_ZERO", "2", {0.0, 0.0}}}) {
    INFO("LTYPE " << record.name);
    CHECK(ltypeRecordGroupValues(out, record.name, "73") ==
          std::vector<std::string>{record.size});
    const auto values = ltypeRecordGroupValues(out, record.name, "49");
    REQUIRE(values.size() == record.dashes.size());
    for (std::size_t i = 0; i < record.dashes.size(); ++i) {
      INFO("value " << i);
      CHECK(std::stod(values[i]) == Catch::Approx(record.dashes[i]));
    }
  }

  // The entity spells the non-ASCII name differently from its record, in ASCII
  // letters only, so both share one record and the entity keeps its spelling.
  CHECK(ltypeRecordGroupValues(out, kOelfarbe, "73").size() == 1);
  CHECK(ltypeRecordGroupValues(out, kOelfarbeUpper, "73").empty());
  RS_Entity *oelLine = entityOnLayer(graphic, QStringLiteral("L_OEL"));
  REQUIRE(oelLine != nullptr);
  CHECK(oelLine->getPen(false).getLineTypeName() ==
        QString::fromUtf8(kOelfarbeUpper));
  CHECK(oelLine->getPen(false).getLineTypeId() != 0);
  CHECK(countValues(recordGroupValues(out, "LINE", "6"), kOelfarbeUpper) == 1);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("QCad-1 reader keeps group 6 and synthesises a marker for it",
          "[dxf][roundtrip][filter][linetype][named][dxf1]") {
  ensureSettings();
  const std::string src = tmpFile("named_t14_src.dxf");
  const std::string out = tmpFile("named_t14_out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // The QCad-1 reader has no LTYPE table, so a name read through it never has
  // a pattern.
  writeText(src,
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLAYER\n70\n1\n"
            "0\nLAYER\n2\nL_VENDOR\n70\n0\n62\n7\n6\nVENDOR_LAY\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n6\nVENDOR_TAB\n"
            "10\n0\n20\n0\n11\n10\n21\n0\n"
            "0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  {
    RS_FilterDXF1 filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXF1));
  }
  REQUIRE(graphic.firstEntity() != nullptr);
  CHECK(graphic.firstEntity()->getPen(false).getLineTypeName() ==
        QStringLiteral("VENDOR_TAB"));
  const RS_Layer *layer = graphic.findLayer(QStringLiteral("L_VENDOR"));
  REQUIRE(layer != nullptr);
  CHECK(layer->getPen().getLineTypeName() == QStringLiteral("VENDOR_LAY"));

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(ltypeRecordGroupValues(out, "VENDOR_TAB", "73") ==
        std::vector<std::string>{"0"});
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"VENDOR_TAB"});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_LAY", "73") ==
        std::vector<std::string>{"0"});
  CHECK(namedRecordGroupValues(out, "LAYER", "L_VENDOR", "6") ==
        std::vector<std::string>{"VENDOR_LAY"});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("An active pen carries its linetype identity into another document",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string out = tmpFile("named_t16_out.dxf");
  std::filesystem::remove(out);

  // Switching windows hands one document's active pen to the next, and every
  // entity added afterwards takes that pen.
  RS_Graphic a;
  RS_Pen active;
  active.setLineTypeName(QStringLiteral("VENDOR_TAB"));
  a.setActivePen(active);

  RS_Graphic b;
  b.initForNewDocument();
  b.setActivePen(a.getActivePen());
  CHECK(b.getActivePen().getLineTypeName() == QStringLiteral("VENDOR_TAB"));
  CHECK(b.getActivePen().getLineTypeId() != 0);
  CHECK(b.getActivePen().getLineTypeId() == active.getLineTypeId());

  auto *line = new RS_Line(&b, RS_LineData(RS_Vector(0.0, 0.0),
                                           RS_Vector(10.0, 0.0)));
  line->setPen(b.getActivePen());
  b.addEntity(line);
  CHECK(line->getPen(false).getLineTypeName() == QStringLiteral("VENDOR_TAB"));
  CHECK(line->getPen(false).getLineTypeId() == active.getLineTypeId());

  // Document b never read the name from a file, so saving it writes a record
  // for the name, and the entity keeps it.
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(b, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(ltypeRecordGroupValues(out, "VENDOR_TAB", "73") ==
        std::vector<std::string>{"0"});
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"VENDOR_TAB"});

  std::filesystem::remove(out);
}

// AC1015/AC1018 (R2000/R2004) keep a DIMSTYLE's own DIMLTYPE/DIMLTEX1/
// DIMLTEX2 in ACAD_DSTYLE_DIM[_EXT1|_EXT2]_LINETYPE XDATA (1001 app name,
// 1070 380/381/382, 1005 LTYPE handle), not in the native 345-347 groups
// R2007+ uses (#2928).
const char *const kNamedR2000DimStyleXDataFixture =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLTYPE\n5\n5\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
    "0\nLTYPE\n5\n40\n330\n5\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
    "2\nVENDOR_XDIM\n70\n0\n3\nVendor XDATA dimstyle linetype\n72\n65\n"
    "73\n2\n40\n40.0\n49\n20.0\n74\n0\n49\n-20.0\n74\n0\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
    "0\nLAYER\n5\n50\n330\n2\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
    "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nDIMSTYLE\n5\nA\n330\n0\n"
    "100\nAcDbSymbolTable\n70\n1\n100\nAcDbDimStyleTable\n71\n0\n"
    "0\nDIMSTYLE\n105\n31\n330\nA\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbDimStyleTableRecord\n"
    "2\nVENDOR_XDIM_STYLE\n70\n0\n"
    "1001\nACAD_DSTYLE_DIM_LINETYPE\n1070\n380\n1005\n40\n"
    "1001\nACAD_DSTYLE_DIM_EXT1_LINETYPE\n1070\n381\n1005\n40\n"
    "1001\nACAD_DSTYLE_DIM_EXT2_LINETYPE\n1070\n382\n1005\n40\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";

TEST_CASE("DXF DIMSTYLE linetypes in AC1015/AC1018 XDATA are read and "
          "written back",
          "[dxf][roundtrip][filter][linetype][named][dimstyle]") {
  ensureSettings();
  const std::string src =
      writeFixture("named_xdim_src.dxf", kNamedR2000DimStyleXDataFixture);
  const std::string out = tmpFile("named_xdim_out.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  const LC_DimStyle *style =
      graphic.getDimStyleList()->findByName(QStringLiteral("VENDOR_XDIM_STYLE"));
  REQUIRE(style != nullptr);
  CHECK(style->dimensionLine()->lineTypeName() ==
        QStringLiteral("VENDOR_XDIM"));
  CHECK(style->extensionLine()->lineTypeFirstRaw() ==
        QStringLiteral("VENDOR_XDIM"));
  CHECK(style->extensionLine()->lineTypeSecondRaw() ==
        QStringLiteral("VENDOR_XDIM"));

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW2000));
  }
  // A single unnamed-type style like this one is its own base style, so
  // export also emits its own (unrelated) ACAD_DSTYLE_DIMTALN default;
  // check the three linetype names are each written once rather than
  // requiring an exact list.
  const auto appNames = recordGroupValues(out, "DIMSTYLE", "1001");
  for (const char *name : {"ACAD_DSTYLE_DIM_LINETYPE",
                           "ACAD_DSTYLE_DIM_EXT1_LINETYPE",
                           "ACAD_DSTYLE_DIM_EXT2_LINETYPE"}) {
    INFO(name);
    CHECK(countValues(appNames, name) == 1);
  }
  // The fixture's own LTYPE record (two dash segments) is unmodified and
  // still there -- not replaced by a name-only marker.
  CHECK(ltypeRecordGroupValues(out, "VENDOR_XDIM", "73") ==
        std::vector<std::string>{"2"});

  std::filesystem::remove(out);
}

TEST_CASE("DXF a linetype named only by an MLINESTYLE element reaches the walk",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture("named_t17_src.dxf", kNamedR2000Fixture);
  const std::string out = tmpFile("named_t17_out.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  // The export targets R2000; a higher target is untested here.
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW2000));
  }
  // VENDOR_MLS is named by the MLINESTYLE element and by nothing else: no
  // entity, layer, dim style or LTYPE record. It gets a record...
  CHECK(ltypeRecordGroupValues(out, "VENDOR_MLS", "73") ==
        std::vector<std::string>{"0"});
  // ...and the element still names it, once; the second one stays BYLAYER.
  const auto elementNames =
      namedRecordGroupValues(out, "MLINESTYLE", "VENDOR_MLS_STYLE", "6");
  CHECK(countValues(elementNames, "VENDOR_MLS") == 1);
  CHECK(countValues(elementNames, "BYLAYER") == 1);

  // An MLEADERSTYLE or MLEADER names its linetype by handle only, and a handle
  // resolves to a name only through an LTYPE record the file already carries.

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("Linetype names on polylines, texts and inserts get a record",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  // The names sit on the pens of the containers themselves; the block member
  // has no group 6.
  const std::string src = writeFixture(
      "named_containers_src.dxf",
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nBLOCK_RECORD\n5\n1\n330\n0\n100\nAcDbSymbolTable\n70\n1\n"
      "0\nBLOCK_RECORD\n5\n30\n330\n1\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbBlockTableRecord\n2\nBLK\n70\n0\n"
      "0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\n31\n330\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockBegin\n"
      "2\nBLK\n70\n0\n10\n0.0\n20\n0.0\n30\n0.0\n3\nBLK\n1\n\n"
      "0\nLINE\n5\n32\n330\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
      "10\n0.0\n20\n0.0\n11\n5.0\n21\n0.0\n"
      "0\nENDBLK\n5\n33\n330\n30\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockEnd\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLWPOLYLINE\n5\n40\n100\nAcDbEntity\n8\n0\n6\nVENDOR_PL\n"
      "100\nAcDbPolyline\n90\n2\n70\n0\n10\n0.0\n20\n0.0\n10\n10.0\n20\n0.0\n"
      "0\nTEXT\n5\n41\n100\nAcDbEntity\n8\n0\n6\nVENDOR_TX\n100\nAcDbText\n"
      "10\n0.0\n20\n10.0\n30\n0.0\n40\n2.0\n1\nT\n100\nAcDbText\n"
      "0\nINSERT\n5\n42\n100\nAcDbEntity\n8\n0\n6\nVENDOR_INS\n"
      "100\nAcDbBlockReference\n2\nBLK\n10\n0.0\n20\n20.0\n30\n0.0\n"
      "0\nENDSEC\n0\nEOF\n");
  const std::string out = tmpFile("named_containers_out.dxf");
  std::filesystem::remove(out);

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
  for (const auto &[record, name] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"LWPOLYLINE", "VENDOR_PL"},
           {"TEXT", "VENDOR_TX"},
           {"INSERT", "VENDOR_INS"}}) {
    INFO(record);
    CHECK(ltypeRecordGroupValues(out, name, "73") ==
          std::vector<std::string>{"0"});
    CHECK(recordGroupValues(out, record, "6") ==
          std::vector<std::string>{name});
  }

#ifdef DWGSUPPORT
  const std::string dwg = tmpFile("named_containers_out.dwg");
  std::filesystem::remove(dwg);
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
  }
  std::map<RS2::EntityType, QString> names;
  for (RS_Entity *e : fromDwg) {
    if (e != nullptr)
      names[e->rtti()] = e->getPen(false).getLineTypeName();
  }
  CHECK(names[RS2::EntityPolyline] == QStringLiteral("VENDOR_PL"));
  CHECK(names[RS2::EntityText] == QStringLiteral("VENDOR_TX"));
  CHECK(names[RS2::EntityInsert] == QStringLiteral("VENDOR_INS"));
  std::filesystem::remove(dwg);
#endif

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("A drawing without custom linetypes keeps its group 6 as before",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  // Built-in names in other spellings, one with a leading blank, an entity
  // with no group 6, and a layer whose group 6 is blanks only.
  const std::string src = writeFixture(
      "named_builtin_src.dxf",
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n"
      "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n100\nAcDbSymbolTable\n70\n3\n"
      "0\nLAYER\n5\n50\n330\n2\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbLayerTableRecord\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
      "0\nLAYER\n5\n51\n330\n2\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbLayerTableRecord\n2\nL_AC\n70\n0\n62\n7\n6\nContinuous\n"
      "0\nLAYER\n5\n52\n330\n2\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbLayerTableRecord\n2\nL_BLANK\n70\n0\n62\n7\n6\n   \n"
      "0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\nL_AC\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
      "0\nLINE\n8\n0\n6\nBYLAYER\n10\n0.0\n20\n10.0\n11\n10.0\n21\n10.0\n"
      "0\nLINE\n8\n0\n6\nhidden\n10\n0.0\n20\n20.0\n11\n10.0\n21\n20.0\n"
      "0\nLINE\n8\n0\n6\nByBlock\n10\n0.0\n20\n30.0\n11\n10.0\n21\n30.0\n"
      "0\nLINE\n8\n0\n6\n DASHED\n10\n0.0\n20\n40.0\n11\n10.0\n21\n40.0\n"
      "0\nENDSEC\n0\nEOF\n");
  const std::string out = tmpFile("named_builtin_out.dxf");
  const std::string out12 = tmpFile("named_builtin_out12.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(out12);

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
  // " DASHED" is the one declared change: it used to be written CONTINUOUS.
  // The test helpers strip the blank, so that entry pins the name only.
  CHECK(sortedValues(recordGroupValues(out, "LINE", "6")) ==
        sortedValues({"ByLayer", "ByLayer", "HIDDEN", "ByBlock", "DASHED"}));
  CHECK(namedRecordGroupValues(out, "LAYER", "L_AC", "6") ==
        std::vector<std::string>{"CONTINUOUS"});
  // A layer cannot be ByLayer, so blanks stay CONTINUOUS there.
  CHECK(namedRecordGroupValues(out, "LAYER", "L_BLANK", "6") ==
        std::vector<std::string>{"CONTINUOUS"});
  CHECK(ltypeRecordGroupValues(out, "hidden", "73").empty());
  CHECK(ltypeRecordGroupValues(out, "BYLAYER", "73").empty());

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out12),
                              RS2::FormatDXFRW12));
  }
  CHECK(sortedValues(recordGroupValues(out12, "LINE", "6")) ==
        sortedValues({"BYLAYER", "BYLAYER", "HIDDEN", "BYBLOCK", "DASHED"}));
  CHECK(namedRecordGroupValues(out12, "LAYER", "L_AC", "6") ==
        std::vector<std::string>{"CONTINUOUS"});
  CHECK(namedRecordGroupValues(out12, "LAYER", "L_BLANK", "6") ==
        std::vector<std::string>{"CONTINUOUS"});
  CHECK(ltypeRecordGroupValues(out12, "BYLAYER", "73").size() == 1);
  CHECK(ltypeRecordGroupValues(out12, "HIDDEN", "73").size() == 1);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out12);
}

TEST_CASE("Header linetype names get a record",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string src = writeFixture(
      "named_header_src.dxf",
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
      "9\n$CELTYPE\n6\nVENDOR_CEL\n9\n$DIMLTYPE\n6\nVENDOR_DLT\n"
      "9\n$DIMLTEX1\n6\nVENDOR_DX1\n9\n$DIMLTEX2\n6\nVENDOR_DX2\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
      "0\nENDSEC\n0\nEOF\n");
  const std::string out = tmpFile("named_header_out.dxf");
  std::filesystem::remove(out);

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

  // The header value that follows `variable`.
  const auto headerValue = [&out](const std::string &variable) {
    std::ifstream in(out);
    std::string code, value;
    bool found = false;
    while (std::getline(in, code) && std::getline(in, value)) {
      if (found)
        return trimDxfToken(value);
      found = trimDxfToken(code) == "9" && trimDxfToken(value) == variable;
    }
    return std::string();
  };
  for (const auto &[variable, name] :
       std::initializer_list<std::pair<const char *, const char *>>{
           {"$CELTYPE", "VENDOR_CEL"},
           {"$DIMLTYPE", "VENDOR_DLT"},
           {"$DIMLTEX1", "VENDOR_DX1"},
           {"$DIMLTEX2", "VENDOR_DX2"}}) {
    INFO(variable);
    CHECK(headerValue(variable) == name);
    CHECK(ltypeRecordGroupValues(out, name, "73") ==
          std::vector<std::string>{"0"});
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("Dimension linetype names without a record get one and keep their "
          "reference",
          "[dxf][roundtrip][filter][linetype][named][dimension][dimstyle]") {
  ensureSettings();
  const std::string out = tmpFile("named_dimension_out.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *style = new LC_DimStyle(QStringLiteral("Standard"));
  style->dimensionLine()->setLineType(QStringLiteral("VENDOR_DS"));
  style->extensionLine()->setLineTypeFirst(QStringLiteral("VENDOR_EX1"));
  style->extensionLine()->setLineTypeSecond(QStringLiteral("VENDOR_EX2"));
  graphic.getDimStyleList()->addDimStyle(style);

  RS_DimensionData data;
  data.definitionPoint = RS_Vector(5.0, 3.0);
  data.middleOfText = RS_Vector(5.0, 3.0);
  data.style = "Standard";
  auto *dimension = new RS_DimAligned(
      &graphic, data,
      RS_DimAlignedData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
  LC_DimStyle override;
  override.dimensionLine()->setLineType(QStringLiteral("VENDOR_OVR"));
  dimension->setDimStyleOverride(&override);
  graphic.addEntity(dimension);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW2018));
  }
  // The handle (5) comes before the name (2) in an LTYPE record.
  const auto ltypeHandle = [&out](const char *name) {
    for (const auto &[code, value] :
         recordGroupsWithValue(out, "LTYPE", "2", name)) {
      if (code == "5")
        return value;
    }
    return std::string();
  };
  for (const char *name : {"VENDOR_OVR", "VENDOR_DS", "VENDOR_EX1",
                           "VENDOR_EX2"}) {
    INFO(name);
    CHECK(ltypeRecordGroupValues(out, name, "73") ==
          std::vector<std::string>{"0"});
  }
  const std::string overrideHandle = ltypeHandle("VENDOR_OVR");
  const std::string styleHandle = ltypeHandle("VENDOR_DS");
  REQUIRE_FALSE(overrideHandle.empty());
  REQUIRE_FALSE(styleHandle.empty());
  CHECK(recordGroupValues(out, "DIMENSION", "1070") ==
        std::vector<std::string>{"345"});
  CHECK(recordGroupValues(out, "DIMENSION", "1005") ==
        std::vector<std::string>{overrideHandle});
  CHECK(namedRecordGroupValues(out, "DIMSTYLE", "Standard", "345") ==
        std::vector<std::string>{styleHandle});
  CHECK(namedRecordGroupValues(out, "DIMSTYLE", "Standard", "346") ==
        std::vector<std::string>{ltypeHandle("VENDOR_EX1")});
  CHECK(namedRecordGroupValues(out, "DIMSTYLE", "Standard", "347") ==
        std::vector<std::string>{ltypeHandle("VENDOR_EX2")});

  std::filesystem::remove(out);
}

TEST_CASE("A linetype name DXF cannot hold saves as before",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  // The reader drops a trailing carriage return only, so this one stays.
  const std::string src = writeFixture(
      "named_unwritable_src.dxf",
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n6\nVENDOR\rX\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
      "0\nENDSEC\n0\nEOF\n");
  const std::string out = tmpFile("named_unwritable_out.dxf");
  std::filesystem::remove(out);

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXFRW));
  }
  REQUIRE(graphic.firstEntity() != nullptr);
  REQUIRE(graphic.firstEntity()->getPen(false).getLineTypeName().contains(
      QLatin1Char('\r')));

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"CONTINUOUS"});
  // The helpers split at \n, so only this \r can show up inside a name.
  for (const std::string &name : recordGroupValues(out, "LTYPE", "2")) {
    INFO(name);
    CHECK(name.find('\r') == std::string::npos);
  }

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("Linetype names on hatch pattern lines get a record in R12",
          "[dxf][roundtrip][filter][linetype][named]") {
  ensureSettings();
  const std::string out12 = tmpFile("named_hatch_out12.dxf");
  std::filesystem::remove(out12);

  // A pattern line keeps the pen its hatch had at its last update, which the
  // hatch and its layer need not name any more. R12 writes it in a block.
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *hatch =
      new RS_Hatch(&graphic, RS_HatchData(false, 1.0, 0.0, "ANSI31"));
  auto *patternLine = new RS_Line(hatch, RS_LineData(RS_Vector(0.0, 0.0),
                                                     RS_Vector(10.0, 0.0)));
  RS_Pen pen;
  pen.setLineTypeName(QStringLiteral("VENDOR_HCH"));
  patternLine->setPen(pen);
  patternLine->setFlag(RS2::FlagHatchChild);
  hatch->addEntity(patternLine);
  graphic.addEntity(hatch);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out12),
                              RS2::FormatDXFRW12));
  }
  CHECK(countValues(recordGroupValues(out12, "LINE", "6"), "VENDOR_HCH") ==
        1);
  CHECK(ltypeRecordGroupValues(out12, "VENDOR_HCH", "73") ==
        std::vector<std::string>{"0"});

  std::filesystem::remove(out12);
}

TEST_CASE("Blanks around a linetype name string add no second record",
          "[dxf][roundtrip][filter][linetype][named][dimension]") {
  ensureSettings();
  const std::string out = tmpFile("named_padded_out.dxf");
  std::filesystem::remove(out);

  // A pen drops the blanks around its name, while a header variable or a
  // dim style keeps them. The padded override comes first on purpose.
  RS_Graphic graphic;
  graphic.initForNewDocument();
  graphic.getDimStyleList()->addDimStyle(
      new LC_DimStyle(QStringLiteral("Standard")));
  graphic.addVariable(QStringLiteral("$CELTYPE"), QStringLiteral("BYLAYER "),
                      6);

  RS_DimensionData data;
  data.definitionPoint = RS_Vector(5.0, 3.0);
  data.middleOfText = RS_Vector(5.0, 3.0);
  data.style = "Standard";
  auto *dimension = new RS_DimAligned(
      &graphic, data,
      RS_DimAlignedData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
  LC_DimStyle override;
  override.dimensionLine()->setLineType(QStringLiteral("VENDOR_PAD "));
  dimension->setDimStyleOverride(&override);
  graphic.addEntity(dimension);

  auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 10.0),
                                                 RS_Vector(10.0, 10.0)));
  RS_Pen pen;
  pen.setLineTypeName(QStringLiteral("VENDOR_PAD"));
  line->setPen(pen);
  graphic.addEntity(line);

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }
  // The helpers keep trailing blanks, so each name below is exact.
  const auto names = recordGroupValues(out, "LTYPE", "2");
  CHECK(countValues(names, "VENDOR_PAD") == 1);
  CHECK(countValues(names, "VENDOR_PAD ") == 0);
  CHECK(countValues(names, "BYLAYER ") == 0);
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{"VENDOR_PAD"});

  std::filesystem::remove(out);
}
