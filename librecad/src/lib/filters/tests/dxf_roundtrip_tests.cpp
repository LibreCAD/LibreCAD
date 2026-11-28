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
#include <cstdint>
#include <cstdlib>
#include <cstring>
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
#include "lc_colornumbers.h"
#include "lc_dimstyle.h"
#include "lc_dimarc.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_filtertestsupport.h"
#include "lc_hatchassociation.h"
#include "lc_linetype.h"
#include "lc_linetypenames.h"
#include "lc_mleader.h"
#include "lc_containertraverser.h"
#include "rs_arc.h"
#include "rs_dimaligned.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_dimension.h"
#include "rs_insert.h"
#include "rs_text.h"
#include "rs_fileio.h"
#include "rs_filterdxf1.h"
#include "rs_filterdxfrw.h"
#include "rs_filterjww.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_entity.h"
#include "rs_block.h"
#include "rs_layer.h"
#include "rs_leader.h"
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
  std::ofstream out(path, std::ios::binary);
  out << content;
}

void normalizeDxfLineEndings(std::string &text) {
  for (std::size_t pos = 0; (pos = text.find("\r\n", pos)) != std::string::npos; ++pos)
    text.erase(pos, 1);
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
// A record's application groups may precede its name, so buffer values until
// the next record confirms which linetype they belong to.
std::vector<std::string> ltypeRecordGroupValues(const std::string &path,
                                                const std::string &ltypeName,
                                                const std::string &code) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  std::vector<std::string> values;
  std::vector<std::string> pending;
  bool inLtype = false;
  bool nameMatched = false;
  int appDepth = 0;
  const auto appendMatched = [&] {
    if (inLtype && nameMatched)
      values.insert(values.end(), pending.begin(), pending.end());
  };
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    const std::string groupCode = trimDxfToken(codeLine);
    const std::string value = trimDxfToken(valueLine);
    if (groupCode == "0") {
      appendMatched();
      pending.clear();
      inLtype = value == "LTYPE";
      nameMatched = false;
      appDepth = 0;
    } else if (inLtype) {
      if (groupCode == "2" && appDepth == 0)
        nameMatched = value == ltypeName;
      if (groupCode == code)
        pending.push_back(value);
      if (groupCode == "102") {
        if (!value.empty() && value.front() == '{')
          ++appDepth;
        else if (value == "}" && appDepth > 0)
          --appDepth;
      }
    }
  }
  appendMatched();
  return values;
}

TEST_CASE("DXF linetype lookup includes application data before its name",
          "[system-codec][dxf][linetype]") {
  const auto path = tmpFile("ltype_lookup_order.dxf");
  writeText(path, "0\nLTYPE\n102\n{APP\n2\nOTHER\n310\nCAFE\n102\n}\n"
                  "2\nTARGET\n0\nLTYPE\n2\nOTHER\n310\nDEAD\n0\nEOF\n");
  CHECK(ltypeRecordGroupValues(path, "TARGET", "310") ==
        std::vector<std::string>{"CAFE"});
  CHECK(ltypeRecordGroupValues(path, "OTHER", "310") ==
        std::vector<std::string>{"DEAD"});
  std::filesystem::remove(path);
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

namespace {

// A block reference with one single-line ATTRIB at (1, 4) and a TEXT at
// (1, 9), both holding @p value as it is spelt in an ASCII DXF file.
std::string singleLineTextDxf(const std::string &value) {
  return "0\nSECTION\n2\nBLOCKS\n"
         "0\nBLOCK\n8\n0\n2\nTITLE\n70\n0\n10\n0\n20\n0\n3\nTITLE\n"
         "0\nENDBLK\n8\n0\n0\nENDSEC\n"
         "0\nSECTION\n2\nENTITIES\n"
         "0\nINSERT\n8\n0\n2\nTITLE\n10\n0\n20\n0\n66\n1\n"
         "0\nATTRIB\n8\n0\n100\nAcDbText\n10\n1\n20\n4\n40\n2.5\n1\n" +
         value +
         "\n100\nAcDbAttribute\n2\nPATH\n70\n0\n"
         "0\nSEQEND\n8\n0\n"
         "0\nTEXT\n8\n0\n100\nAcDbText\n10\n1\n20\n9\n40\n2.5\n1\n" +
         value + "\n0\nENDSEC\n0\nEOF\n";
}

// The value of the single-line text whose insertion point is at height @p y.
std::string singleLineTextAt(RS_Graphic &graphic, double y) {
  for (RS_Entity *entity : graphic) {
    if (entity == nullptr || entity->rtti() != RS2::EntityText)
      continue;
    const auto *text = static_cast<const RS_Text *>(entity);
    if (std::abs(text->getInsertionPoint().y - y) < 1e-6)
      return text->getText().toStdString();
  }
  return "<no single-line text>";
}

} // namespace

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
TEST_CASE("System codec respects declared UTF-8 in legacy DXF",
          "[system-codec][dxf][codepage]") {
  ensureSettings();
  struct Case { const char *version; const char *codepage; const char *bytes; };
  for (const Case &item : std::vector<Case>{
           {"AC1009", "UTF-8", "\xC3\x96"},
           {"AC1009", "ANSI_1252", "\xD6"},
           {"AC1009", "", "\xD6"},
           {"AC1021", "ANSI_1252", "\xC3\x96"}}) {
    INFO("version=" << item.version << " codepage=" << item.codepage);
    const auto source = tmpFile("system_declared_utf8_source.dxf");
    const auto output = tmpFile("system_declared_utf8_output.dxf");
    std::string content = std::string("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\n") + item.version + "\n";
    if (*item.codepage)
      content += std::string("9\n$DWGCODEPAGE\n3\n") + item.codepage + "\n";
    const std::string name = std::string(item.bytes) + "LTYPE";
    content += "0\nENDSEC\n0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLTYPE\n70\n1\n"
               "0\nLTYPE\n5\n40\n2\n" + name + "\n70\n0\n3\nCustom\n72\n65\n73\n0\n40\n0\n"
               "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
               "0\nTEXT\n5\n41\n8\n0\n10\n1\n20\n9\n40\n2.5\n1\n" + item.bytes +
               "\n0\nENDSEC\n0\nEOF\n";
    writeText(source, content);
    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    CHECK(graphic.findLineType(QStringLiteral("\u00D6LTYPE")) != nullptr);
    CHECK(singleLineTextAt(graphic, 9.0) == "\xC3\x96");
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    RS_Graphic reread;
    REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(reread.findLineType(QStringLiteral("\u00D6LTYPE")) != nullptr);
    CHECK(singleLineTextAt(reread, 9.0) == "\xC3\x96");
    std::filesystem::remove(source);
    std::filesystem::remove(output);
  }
}

TEST_CASE("System codec preserves canonical linetype metadata after retry",
          "[system-codec][dxf][linetype][transaction]") {
  ensureSettings();
  const auto output = tmpFile("system_canonical_linetype_retry.dxf");
  const std::string sentinel = "preserve destination\n";
  writeText(output, sentinel);
  RS_Graphic graphic;
  graphic.initForNewDocument();
  DRW_LType source;
  source.name = "Continuous";
  source.flags = -1;
  graphic.dwgAdvancedMetadata().addLineTypeName(source);
  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream retained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  source.flags = 64;
  graphic.dwgAdvancedMetadata().addLineTypeName(source);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(ltypeRecordGroupValues(output, "Continuous", "70") ==
        std::vector<std::string>{"64"});
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW12));
  CHECK(ltypeRecordGroupValues(output, "CONTINUOUS", "70") ==
        std::vector<std::string>{"64"});
  writeText(output, sentinel);
  source.xDictHandle = 0x20;
  graphic.dwgAdvancedMetadata().addLineTypeName(source);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW12));
  std::ifstream retainedR12(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retainedR12), {}) == sentinel);
  retainedR12.close();
  source.xDictHandle = 0;
  graphic.dwgAdvancedMetadata().addLineTypeName(source);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW12));
  CHECK(ltypeRecordGroupValues(output, "CONTINUOUS", "70") ==
        std::vector<std::string>{"64"});
  std::filesystem::remove(output);
}

TEST_CASE("System codec preserves a layer material link and refuses a missing carrier",
          "[system-codec][dxf][material][layer][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_layer_material_source.dxf");
  const auto output = tmpFile("system_layer_material_output.dxf");
  const auto retry = tmpFile("system_layer_material_retry.dxf");
  const auto materialHandle = GENERATE(std::uint32_t{0xA1}, std::uint32_t{0x14});
  const bool withReactors = GENERATE(false, true);
  const std::string materialText = materialHandle == 0xA1u ? "A1" : "14";
  std::string input =
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
            "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
            "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\nA1\n"
            "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n5\nA2\n100\nAcDbEntity\n8\nRAWTEST\n100\nAcDbLine\n"
            "10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
            "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "3\nACAD_MATERIAL\n350\nA0\n"
            "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\nPlainMat\n350\nA1\n"
            "0\nMATERIAL\n5\nA1\n330\nA0\n100\nAcDbMaterial\n1\nPlainMat\n"
            "0\nENDSEC\n0\nEOF\n";
  if (withReactors) {
    const auto insertReactors = [&](const std::string &from, const std::string &to) {
      const auto position = input.find(from);
      REQUIRE(position != std::string::npos);
      input.replace(position, from.size(), to);
    };
    insertReactors("\n5\nA0\n330\nC\n",
                   "\n5\nA0\n102\n{ACAD_REACTORS\n330\nC\n102\n}\n330\nC\n");
    insertReactors("\n5\nA1\n330\nA0\n",
                   "\n5\nA1\n102\n{ACAD_REACTORS\n330\nA0\n102\n}\n330\nA0\n");
  }
  if (materialHandle != 0xA1u) {
    const std::string from = "\nA1\n";
    for (std::size_t pos = 0; (pos = input.find(from, pos)) != std::string::npos;) {
      input.replace(pos, from.size(), "\n14\n");
      pos += 4;
    }
  }
  writeText(source, input);
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  const auto *layer = graphic.dwgAdvancedMetadata().findLayerTableEntryByName("RAWTEST");
  REQUIRE(layer != nullptr);
  CHECK(layer->handleMaterialS == materialText);
  auto &raw = const_cast<std::vector<DRW_RawDxfObject>&>(
      graphic.dwgAdvancedMetadata().rawDxfObjects());
  auto found = std::find_if(raw.begin(), raw.end(), [&](const auto &entry) {
    return entry.name == "MATERIAL" && entry.handle == materialHandle;
  });
  REQUIRE(found != raw.end());
  const DRW_RawDxfObject saved = *found;
  raw.erase(found);
  const std::string sentinel = "preserve material destination\n";
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream retained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  raw.push_back(saved);
  const auto expectRefusal = [&] {
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    std::ifstream existing(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
  };
  auto &carrier = raw.back();
  auto self = std::find_if(carrier.groups.begin(), carrier.groups.end(),
                           [](const auto &group) { return group.code() == 5; });
  REQUIRE(self != carrier.groups.end());
  const DRW_Variant oldSelf = *self;
  *self = DRW_Variant(5, std::string("A3"));
  expectRefusal();
  *self = oldSelf;
  REQUIRE(carrier.hasRawValues);
  const auto name = std::find_if(carrier.groups.begin(), carrier.groups.end(),
                                 [](const auto &group) { return group.code() == 1; });
  REQUIRE(name != carrier.groups.end());
  const auto nameIndex = static_cast<std::size_t>(std::distance(carrier.groups.begin(), name));
  const auto oldRawName = carrier.rawValues.at(nameIndex);
  carrier.rawValues[nameIndex] = "Different";
  expectRefusal();
  carrier.rawValues[nameIndex] = oldRawName;
  if (withReactors) {
    auto reactor = std::find_if(carrier.groups.begin(), carrier.groups.end(),
                                [](const auto &group) { return group.code() == 330; });
    REQUIRE(reactor != carrier.groups.end());
    const auto reactorIndex = static_cast<std::size_t>(std::distance(carrier.groups.begin(), reactor));
    const DRW_Variant oldReactor = *reactor;
    const auto oldRawReactor = carrier.rawValues.at(reactorIndex);
    *reactor = DRW_Variant(330, std::string("A3"));
    carrier.rawValues[reactorIndex] = "A3";
    expectRefusal();
    *reactor = oldReactor;
    carrier.rawValues[reactorIndex] = oldRawReactor;
  }
  carrier.m_version = DRW::AC1015;
  expectRefusal();
  carrier.m_version = saved.m_version;
  auto *mutableLayer = const_cast<DRW_Layer *>(layer);
  mutableLayer->handleMaterialS = "not-a-handle";
  expectRefusal();
  mutableLayer->handleMaterialS = materialText;
  raw.push_back(saved);
  expectRefusal();
  raw.pop_back();
  auto &dictionaries = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord>&>(
      graphic.dwgAdvancedMetadata().dictionaries());
  auto dictionary = std::find_if(dictionaries.begin(), dictionaries.end(),
      [](const auto &entry) { return entry.handle == 0xA0u; });
  REQUIRE(dictionary != dictionaries.end());
  const auto entries = dictionary->entries;
  REQUIRE_FALSE(dictionary->entries.empty());
  dictionary->entries.front().dxfHandleCode = 360;
  expectRefusal(); // typed edge role disagrees with its raw dictionary
  dictionary->entries = entries;
  dictionary->entries.front().name = "plainmat";
  expectRefusal(); // a spelling-only edit must not replay the stale raw key
  dictionary->entries = entries;
  const int cloning = dictionary->cloning;
  dictionary->cloning = 2;
  expectRefusal(); // typed cloning semantics disagree with the raw dictionary
  dictionary->cloning = cloning;
  const int hardOwner = dictionary->hardOwner;
  dictionary->hardOwner = 1;
  expectRefusal(); // typed ownership semantics disagree with the raw dictionary
  dictionary->hardOwner = hardOwner;
  dictionary->entries.clear();
  expectRefusal();
  dictionary->entries = entries;
  auto root = std::find_if(dictionaries.begin(), dictionaries.end(),
      [](const auto &entry) { return entry.handle == 0xCu; });
  REQUIRE(root != dictionaries.end());
  const auto rootEntries = root->entries;
  REQUIRE_FALSE(root->entries.empty());
  root->entries.front().dxfHandleCode = 360;
  expectRefusal(); // the generated root entry cannot retain a hard role
  root->entries = rootEntries;
  root->entries.clear();
  expectRefusal();
  root->entries = rootEntries;
  auto rawDictionary = std::find_if(raw.begin(), raw.end(), [](const auto &entry) {
    return entry.name == "DICTIONARY" && entry.handle == 0xA0u;
  });
  REQUIRE(rawDictionary != raw.end());
  auto rawCloning = std::find_if(rawDictionary->groups.begin(), rawDictionary->groups.end(),
                                 [](const auto &group) { return group.code() == 281; });
  REQUIRE(rawCloning != rawDictionary->groups.end());
  REQUIRE(rawDictionary->hasRawValues);
  const auto rawCloningIndex = static_cast<std::size_t>(
      std::distance(rawDictionary->groups.begin(), rawCloning));
  const auto savedRawCloning = rawDictionary->rawValues.at(rawCloningIndex);
  rawDictionary->rawValues[rawCloningIndex] = "2";
  expectRefusal(); // physical numeric spelling must agree with typed group data
  rawDictionary->rawValues[rawCloningIndex] = savedRawCloning;
  if (withReactors) {
    auto reactor = std::find_if(rawDictionary->groups.begin(), rawDictionary->groups.end(),
                                [](const auto &group) { return group.code() == 330; });
    REQUIRE(reactor != rawDictionary->groups.end());
    const auto reactorIndex = static_cast<std::size_t>(
        std::distance(rawDictionary->groups.begin(), reactor));
    const DRW_Variant oldReactor = *reactor;
    const auto oldRawReactor = rawDictionary->rawValues.at(reactorIndex);
    *reactor = DRW_Variant(330, std::string("A3"));
    rawDictionary->rawValues[reactorIndex] = "A3";
    expectRefusal();
    *reactor = oldReactor;
    rawDictionary->rawValues[reactorIndex] = oldRawReactor;
  }
  auto entryHandle = std::find_if(rawDictionary->groups.begin(), rawDictionary->groups.end(),
      [](const auto &group) { return group.code() == 350; });
  REQUIRE(entryHandle != rawDictionary->groups.end());
  const DRW_Variant oldEntryHandle = *entryHandle;
  *entryHandle = DRW_Variant(350, std::string("A3"));
  expectRefusal();
  *entryHandle = oldEntryHandle;
  REQUIRE(rawDictionary->hasRawValues);
  const auto dictionaryName = std::find_if(rawDictionary->groups.begin(), rawDictionary->groups.end(),
      [](const auto &group) { return group.code() == 3; });
  REQUIRE(dictionaryName != rawDictionary->groups.end());
  const auto dictionaryNameIndex = static_cast<std::size_t>(
      std::distance(rawDictionary->groups.begin(), dictionaryName));
  const auto oldRawDictionaryName = rawDictionary->rawValues.at(dictionaryNameIndex);
  rawDictionary->rawValues[dictionaryNameIndex] = "Different";
  expectRefusal();
  rawDictionary->rawValues[dictionaryNameIndex] = oldRawDictionaryName;
  // A valid selected material must not mask a missing dictionary sibling.
  rawDictionary->groups.emplace_back(3, std::string("MissingMat"));
  rawDictionary->groups.emplace_back(350, std::string("A3"));
  rawDictionary->rawValues.emplace_back("MissingMat");
  rawDictionary->rawValues.emplace_back("A3");
  expectRefusal(); // raw entry absent from the typed dictionary
  dictionary->entries.push_back({"MissingMat", 0xA3u});
  expectRefusal(); // both entries exist, but the target does not
  dictionary->entries.pop_back();
  rawDictionary->groups.pop_back();
  rawDictionary->groups.pop_back();
  rawDictionary->rawValues.pop_back();
  rawDictionary->rawValues.pop_back();
  dictionary->entries.push_back({"MissingMat", 0xA3u});
  expectRefusal(); // typed entry absent from the raw dictionary
  dictionary->entries.pop_back();
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  std::ifstream crossVersion(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(crossVersion), {}) == sentinel);
  crossVersion.close();
#if defined(LIBDXFRW_HAS_DXF_STAGED_RAW_OBJECT_INSPECTOR) && \
    defined(LIBDXFRW_HAS_DXF_STAGED_LAYER_MATERIAL_INSPECTOR)
  struct MutatingFilter : RS_FilterDXFRW {
    std::function<void()> afterHeader;
    std::function<void()> afterObjects;
    std::function<void()> beforeFinalize;
    void writeHeader(DRW_Header &header) override {
      RS_FilterDXFRW::writeHeader(header);
      if (afterHeader) afterHeader();
    }
    void writeObjects() override {
      RS_FilterDXFRW::writeObjects();
      if (afterObjects) afterObjects();
    }
    bool finalizeDxfWrite() override {
      if (beforeFinalize) beforeFinalize();
      return RS_FilterDXFRW::finalizeDxfWrite();
    }
  } mutating;
  auto &materials = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord>&>(
      graphic.dwgAdvancedMetadata().materials());
  auto selected = std::find_if(materials.begin(), materials.end(),
      [&](const auto &entry) { return entry.handle == materialHandle; });
  REQUIRE(selected != materials.end());
  const auto oldDescription = selected->description;
  mutating.afterObjects = [&] { selected->description = "late object edit"; };
  writeText(output, sentinel);
  CHECK_FALSE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream afterObjectFailure(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(afterObjectFailure), {}) == sentinel);
  afterObjectFailure.close();
  selected->description = oldDescription;
  mutating.afterObjects = {};
  mutating.beforeFinalize = [&] { selected->description = "late finalize edit"; };
  CHECK_FALSE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream afterFinalizeFailure(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(afterFinalizeFailure), {}) == sentinel);
  afterFinalizeFailure.close();
  selected->description = oldDescription;
  mutating.beforeFinalize = {};
  REQUIRE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
#ifdef LIBDXFRW_HAS_DXF_STAGED_ROOT_ENTRY_INSPECTOR
  const auto oldRootKey = root->entries.front().name;
  mutating.afterObjects = [&] { root->entries.front().name = "acad_material"; };
  writeText(output, sentinel);
  CHECK_FALSE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream afterRootFailure(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(afterRootFailure), {}) == sentinel);
  afterRootFailure.close();
  root->entries.front().name = oldRootKey;
  mutating.afterObjects = {};
  mutating.beforeFinalize = [&] { root->entries.front().name = "acad_material"; };
  CHECK_FALSE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream afterRootFinalizationFailure(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(afterRootFinalizationFailure), {}) == sentinel);
  afterRootFinalizationFailure.close();
  root->entries.front().name = oldRootKey;
  mutating.beforeFinalize = {};
  REQUIRE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  root->entries.front().name = "acad_material";
  REQUIRE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK_FALSE(recordGroupsWithValue(output, "DICTIONARY", "3", "acad_material").empty());
  root->entries.front().name = oldRootKey;
#endif
#ifdef LIBDXFRW_HAS_DXF_STAGED_HEADER_MATERIAL_INSPECTOR
  graphic.addVariable("$CMATERIAL", QString::fromStdString(materialText), 347);
  REQUIRE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  {
    RS_Graphic currentGraphic;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(currentGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto currentVariables = currentGraphic.getVariableDict();
    const auto current = currentVariables.constFind(QStringLiteral("$CMATERIAL"));
    const auto writtenMaterials = recordGroupValues(output, "MATERIAL", "5");
    REQUIRE(current != currentVariables.cend());
    REQUIRE(writtenMaterials.size() == 1u);
    CHECK(current->getString().toStdString() == writtenMaterials.front());
  }
  graphic.addVariable("$CMATERIAL", QStringLiteral("0"), 347);
  REQUIRE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  graphic.removeVariable("$CMATERIAL");
  mutating.afterHeader = [&] { graphic.addVariable("$CMATERIAL", QString::fromStdString(materialText), 347); };
  writeText(output, sentinel);
  CHECK_FALSE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream afterHeaderFailure(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(afterHeaderFailure), {}) == sentinel);
  afterHeaderFailure.close();
  graphic.removeVariable("$CMATERIAL");
  mutating.afterHeader = {};
  mutating.beforeFinalize = [&] { graphic.addVariable("$CMATERIAL", QString::fromStdString(materialText), 347); };
  CHECK_FALSE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream afterHeaderFinalizationFailure(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(afterHeaderFinalizationFailure), {}) == sentinel);
  afterHeaderFinalizationFailure.close();
  graphic.removeVariable("$CMATERIAL");
  mutating.beforeFinalize = {};
  REQUIRE(mutating.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
#endif
#endif
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto material = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(material.size() == 1);
  if (materialHandle == 0x14u)
    CHECK(material.front() != "14");
  const auto layerGroups = recordGroupsWithValue(output, "LAYER", "2", "RAWTEST");
  CHECK(std::count(layerGroups.begin(), layerGroups.end(),
                   std::make_pair(std::string("347"), material.front())) == 1);
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(reread, QString::fromStdString(retry), RS2::FormatDXFRW));
  const auto retryMaterial = recordGroupValues(retry, "MATERIAL", "5");
  REQUIRE(retryMaterial.size() == 1);
  const auto retryLayer = recordGroupsWithValue(retry, "LAYER", "2", "RAWTEST");
  CHECK(std::count(retryLayer.begin(), retryLayer.end(),
                   std::make_pair(std::string("347"), retryMaterial.front())) == 1);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_LAYER_MATERIAL_OUTPUT"))
    std::filesystem::copy_file(retry, copy, std::filesystem::copy_options::overwrite_existing);

  // Renaming the original must keep its source metadata; a new layer with
  // the old name must not acquire that metadata by name.
  auto *original = graphic.findLayer(QStringLiteral("RAWTEST"));
  REQUIRE(original != nullptr);
  original->setName(QStringLiteral("RENAMED"));
  graphic.addLayer(new RS_Layer(QStringLiteral("RAWTEST")));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto renamed = recordGroupsWithValue(output, "LAYER", "2", "RENAMED");
  const auto renamedMaterials = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(renamedMaterials.size() == 1);
  CHECK(std::count(renamed.begin(), renamed.end(),
                   std::make_pair(std::string("347"), renamedMaterials.front())) == 1);
  const auto replacement = recordGroupsWithValue(output, "LAYER", "2", "RAWTEST");
  REQUIRE_FALSE(replacement.empty());
  CHECK(std::none_of(replacement.begin(), replacement.end(),
                     [](const auto &group) { return group.first == "347"; }));
  if (const char *copy = std::getenv("LIBRECAD_KEEP_RENAMED_LAYER_MATERIAL_OUTPUT"))
    std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);

  auto *duplicate = original->clone();
  duplicate->setName(QStringLiteral("CLONE"));
  graphic.addLayer(duplicate);
  REQUIRE_FALSE(duplicate->importedRecordId());
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  duplicate->setImportedRecordId(original->importedRecordId());
  expectRefusal();
  duplicate->setImportedRecordId({});
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto clone = recordGroupsWithValue(output, "LAYER", "2", "CLONE");
  REQUIRE_FALSE(clone.empty());
  CHECK(std::none_of(clone.begin(), clone.end(),
                     [](const auto &group) { return group.first == "347"; }));

  // A layer copied from another document must not match this document's
  // unrelated imported record, even if both source names were equal.
  RS_Graphic other;
  REQUIRE(filter.fileImport(other, QString::fromStdString(source), RS2::FormatDXFRW));
  auto *foreign = original->clone();
  foreign->setImportedRecordId(original->importedRecordId());
  foreign->setName(QStringLiteral("FOREIGN"));
  other.addLayer(foreign);
  REQUIRE(filter.fileExport(other, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto foreignLayer = recordGroupsWithValue(output, "LAYER", "2", "FOREIGN");
  REQUIRE_FALSE(foreignLayer.empty());
  CHECK(std::none_of(foreignLayer.begin(), foreignLayer.end(),
                     [](const auto &group) { return group.first == "347"; }));
  std::filesystem::remove(source);
  std::filesystem::remove(output);
  std::filesystem::remove(retry);
}

TEST_CASE("System codec retains all material dictionary targets for a layer",
          "[system-codec][dxf][material][layer][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_layer_material_siblings_source.dxf");
  const auto output = tmpFile("system_layer_material_siblings_output.dxf");
  writeText(source,
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\nA1\n"
      "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n3\nSiblingMat\n350\nA2\n"
      "0\nMATERIAL\n5\nA1\n330\nA0\n100\nAcDbMaterial\n1\nPlainMat\n"
      "0\nMATERIAL\n5\nA2\n330\nA0\n100\nAcDbMaterial\n1\nSiblingMat\n"
      "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto materials = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(materials.size() == 2);
  const auto layer = recordGroupsWithValue(output, "LAYER", "2", "RAWTEST");
  CHECK(std::count(layer.begin(), layer.end(),
                   std::make_pair(std::string("347"), materials.front())) == 1);
  auto &raw = const_cast<std::vector<DRW_RawDxfObject>&>(
      graphic.dwgAdvancedMetadata().rawDxfObjects());
  const auto sibling = std::find_if(raw.begin(), raw.end(), [](const auto &record) {
    return record.name == "MATERIAL" && record.handle == 0xA2u;
  });
  REQUIRE(sibling != raw.end());
  const auto savedSibling = *sibling;
  raw.erase(sibling);
  const std::string sentinel = "preserve missing sibling destination\n";
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream retained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  raw.push_back(savedSibling);
  auto &restoredSibling = raw.back();
  const auto self = std::find_if(restoredSibling.groups.begin(), restoredSibling.groups.end(),
                                 [](const auto &group) { return group.code() == 5; });
  REQUIRE(self != restoredSibling.groups.end());
  const auto selfIndex = static_cast<std::size_t>(
      std::distance(restoredSibling.groups.begin(), self));
  REQUIRE(restoredSibling.hasRawValues);
  restoredSibling.rawValues[selfIndex] = "A3";
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream stillRetained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(stillRetained), {}) == sentinel);
  stillRetained.close();
  restoredSibling.rawValues[selfIndex] = "A2";
  auto &dictionaries = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord>&>(
      graphic.dwgAdvancedMetadata().dictionaries());
  const auto dictionary = std::find_if(dictionaries.begin(), dictionaries.end(),
      [](const auto &record) { return record.handle == 0xA0u; });
  REQUIRE(dictionary != dictionaries.end());
  REQUIRE(dictionary->entries.size() == 2);
  std::swap(dictionary->entries[0], dictionary->entries[1]);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream reordered(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(reordered), {}) == sentinel);
  reordered.close();
  std::swap(dictionary->entries[0], dictionary->entries[1]);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "MATERIAL", "5").size() == 2);
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

#if defined(LIBDXFRW_HAS_DXF_STAGED_RAW_OBJECT_INSPECTOR) && \
    defined(LIBDXFRW_HAS_DXF_STAGED_LAYER_MATERIAL_INSPECTOR)
#ifdef LIBDXFRW_HAS_DXF_STAGED_ENTITY_MATERIAL_INSPECTOR
TEST_CASE("System codec audits a material referenced only by an entity",
          "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_entity_only_material_source.dxf");
  const auto output = tmpFile("system_entity_only_material_output.dxf");
  const std::string sourceHandle = GENERATE(std::string("A1"), std::string("14"));
  const bool inBlock = GENERATE(false, true);
  std::string input =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n";
  input += inBlock
      ? "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n5\nB0\n8\n0\n2\nTESTBLOCK\n70\n0\n10\n0\n20\n0\n30\n0\n"
      : "0\nSECTION\n2\nENTITIES\n";
  input += "0\nLINE\n5\nA2\n100\nAcDbEntity\n"
      "8\n0\n347\nA1\n100\nAcDbLine\n10\n0\n20\n0\n30\n0\n"
      "11\n10\n21\n0\n31\n0\n";
  if (inBlock) input += "0\nENDBLK\n5\nB1\n8\n0\n";
  input += "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
      "100\nAcDbMaterial\n1\nPlainMat\n0\nENDSEC\n0\nEOF\n";
  if (sourceHandle != "A1") {
    const std::string replacement = "\n" + sourceHandle + "\n";
    for (std::size_t at = 0; (at = input.find("\nA1\n", at)) != std::string::npos;) {
      input.replace(at, 4u, replacement);
      at += replacement.size();
    }
  }
  writeText(source, input);
  RS_Graphic graphic;
  struct MutatingFilter : RS_FilterDXFRW {
    std::function<void()> afterObjects;
    void writeObjects() override {
      RS_FilterDXFRW::writeObjects();
      if (afterObjects) afterObjects();
    }
  } filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  auto &materials = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord>&>(
      graphic.dwgAdvancedMetadata().materials());
  REQUIRE(materials.size() == 1u);
  const auto description = materials.front().description;
  const std::string sentinel = "preserve entity material destination\n";
  writeText(output, sentinel);
  filter.afterObjects = [&] { materials.front().description = "late edit"; };
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream retained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  materials.front().description = description;
  filter.afterObjects = {};
  auto *block = inBlock ? graphic.findBlock(QStringLiteral("TESTBLOCK")) : nullptr;
  if (inBlock) REQUIRE(block != nullptr);
  auto *line = inBlock ? block->firstEntity() : graphic.firstEntity();
  REQUIRE(line != nullptr);
  const auto materialHandle = line->materialHandle();
  REQUIRE(materialHandle != 0u);
  filter.afterObjects = [&] { line->setMaterialHandle(0u); };
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream changedEntity(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(changedEntity), {}) == sentinel);
  changedEntity.close();
  line->setMaterialHandle(materialHandle);
  filter.afterObjects = {};
  auto *implicitLayer = graphic.findLayer(QStringLiteral("0"));
  REQUIRE(implicitLayer != nullptr);
  filter.afterObjects = [&] { implicitLayer->setName(QStringLiteral("LATE_LAYER")); };
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream renamedLayer(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(renamedLayer), {}) == sentinel);
  renamedLayer.close();
  implicitLayer->setName(QStringLiteral("0"));
  filter.afterObjects = {};
  line->setMaterialHandle(0xB1u);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  line->setMaterialHandle(materialHandle);
  const auto entityHandle = line->sourceHandle();
  line->setSourceHandle(0u);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  line->setSourceHandle(entityHandle);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto materialHandles = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(materialHandles.size() == 1u);
  if (sourceHandle == "14") CHECK(materialHandles.front() != sourceHandle);
  const auto lines = recordGroupValues(output, "LINE", "347");
  REQUIRE(lines.size() == 1u);
  CHECK(lines.front() == materialHandles.front());
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ENTITY_MATERIAL_OUTPUT"))
    std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}
#endif
#ifdef LIBDXFRW_HAS_DXF_STAGED_HEADER_MATERIAL_INSPECTOR
TEST_CASE("System codec audits a material referenced only by the header",
          "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_header_only_material_source.dxf");
  const auto output = tmpFile("system_header_only_material_output.dxf");
  const bool withLayer = GENERATE(false, true);
  const std::string sourceHandle = GENERATE(std::string("A1"), std::string("14"));
  std::string input =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
      "9\n$CMATERIAL\n347\nA1\n0\nENDSEC\n";
  if (withLayer) input +=
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n"
      "0\nENDTAB\n0\nENDSEC\n";
  input += "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
      "100\nAcDbMaterial\n1\nPlainMat\n0\nENDSEC\n0\nEOF\n";
  if (sourceHandle != "A1") {
    const std::string replacement = "\n" + sourceHandle + "\n";
    for (std::size_t at = 0; (at = input.find("\nA1\n", at)) != std::string::npos;) {
      input.replace(at, 4u, replacement);
      at += replacement.size();
    }
  }
  writeText(source, input);
  RS_Graphic graphic;
  struct MutatingFilter : RS_FilterDXFRW {
    std::function<void()> afterObjects;
    void writeObjects() override {
      RS_FilterDXFRW::writeObjects();
      if (afterObjects) afterObjects();
    }
  } filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  auto &materials = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord>&>(
      graphic.dwgAdvancedMetadata().materials());
  REQUIRE(materials.size() == 1u);
  const auto description = materials.front().description;
  const std::string sentinel = "preserve header-only material destination\n";
  writeText(output, sentinel);
  filter.afterObjects = [&] { materials.front().description = "late edit"; };
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream retained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  materials.front().description = description;
  filter.afterObjects = {};
  graphic.addVariable("$CMATERIAL", QStringLiteral("B1"), 347);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream missingTarget(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(missingTarget), {}) == sentinel);
  missingTarget.close();
  graphic.addVariable("$CMATERIAL", QString::fromStdString(sourceHandle), 347);
  filter.afterObjects = [&] { graphic.addVariable("$CMATERIAL", QStringLiteral("0"), 347); };
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream changedHeader(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(changedHeader), {}) == sentinel);
  changedHeader.close();
  graphic.addVariable("$CMATERIAL", QString::fromStdString(sourceHandle), 347);
  filter.afterObjects = {};
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto materialHandles = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(materialHandles.size() == 1u);
  if (sourceHandle == "14") CHECK(materialHandles.front() != sourceHandle);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_HEADER_MATERIAL_OUTPUT"))
    std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto variables = reread.getVariableDict();
  const auto current = variables.constFind(QStringLiteral("$CMATERIAL"));
  REQUIRE(current != variables.cend());
  CHECK(current->getString().toStdString() == materialHandles.front());
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}
#endif
TEST_CASE("System codec audits material graphs with more than 64 imported layers",
          "[system-codec][dxf][material][layer][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_many_layer_material_source.dxf");
  const auto output = tmpFile("system_many_layer_material_output.dxf");
  std::string input =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n65\n";
  for (unsigned i = 0; i < 65; ++i) {
    std::ostringstream handle;
    handle << std::hex << (0x100u + i);
    input += "0\nLAYER\n5\n" + handle.str() +
             "\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n2\nL" +
             std::to_string(i) + "\n70\n0\n62\n7\n6\nCONTINUOUS\n";
    if (i == 0) input += "347\nA1\n";
  }
  input += "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n"
           "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
           "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
           "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
           "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
           "100\nAcDbMaterial\n1\nPlainMat\n0\nENDSEC\n0\nEOF\n";
  writeText(source, input);
  RS_Graphic graphic;
  struct MutatingFilter : RS_FilterDXFRW {
    std::function<void()> afterObjects;
    void writeObjects() override {
      RS_FilterDXFRW::writeObjects();
      if (afterObjects) afterObjects();
    }
  } filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  auto &materials = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord>&>(
      graphic.dwgAdvancedMetadata().materials());
  REQUIRE(materials.size() == 1u);
  const auto originalDescription = materials.front().description;
  const std::string sentinel = "preserve destination after late mutation\n";
  writeText(output, sentinel);
  filter.afterObjects = [&] { materials.front().description = "late edit"; };
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream retained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  materials.front().description = originalDescription;
  filter.afterObjects = {};
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto materialHandles = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(materialHandles.size() == 1u);
  const auto linkedLayer = recordGroupsWithValue(output, "LAYER", "2", "L0");
  CHECK(std::count(linkedLayer.begin(), linkedLayer.end(),
                   std::make_pair(std::string("347"), materialHandles.front())) == 1);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_MANY_LAYER_MATERIAL_OUTPUT"))
    std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(reread.findLayer(QStringLiteral("L64")) != nullptr);
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}
#endif

TEST_CASE("System codec preserves an explicit null layer material",
          "[system-codec][dxf][material][layer]") {
  ensureSettings();
  const auto source = tmpFile("system_layer_null_material_source.dxf");
  const auto output = tmpFile("system_layer_null_material_output.dxf");
  writeText(source,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
            "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
            "2\nRAWNULL\n70\n0\n62\n7\n6\nCONTINUOUS\n347\n0\n"
            "0\nENDTAB\n0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto groups = recordGroupsWithValue(output, "LAYER", "2", "RAWNULL");
  CHECK(std::count(groups.begin(), groups.end(),
                   std::make_pair(std::string("347"), std::string("0"))) == 1);
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

TEST_CASE("System codec refuses an incomplete raw-only material after metadata storage",
          "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto output = tmpFile("system_raw_material_marker.dxf");
  const std::string sentinel = "preserve material destination\n";
  writeText(output, sentinel);
  const auto source = tmpFile("system_raw_material_marker_source.dxf");
  writeText(source, "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
                    "0\nSECTION\n2\nOBJECTS\n0\nMATERIAL\n5\nF040\n330\n72\n"
                    "100\nAcDbMaterial\n1\nRawOnly\n94\n63\n0\nENDSEC\n0\nEOF\n");
  struct OptInFilter : RS_FilterDXFRW
#ifndef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
      , DRW_DxfSparseMaterialRawPolicy
#endif
  {
    bool wantsDxfSparseMaterialRawCapture() const noexcept override { return true; }
#ifdef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
    bool wantsDxfSparseMaterialRawReplay() const noexcept override { return true; }
#endif
  };
  // Ordinary import retains incomplete materials, while ordinary export
  // remains guarded against losing their unqualified graph.
  RS_Graphic legacy;
  RS_FilterDXFRW legacyFilter;
#ifdef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
  REQUIRE(legacyFilter.fileImport(legacy, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(legacy.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(legacy.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  CHECK_FALSE(legacyFilter.fileExport(legacy, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
#else
  CHECK_FALSE(legacyFilter.fileImport(legacy, QString::fromStdString(source), RS2::FormatDXFRW));
#endif
  RS_Graphic incomplete;
  OptInFilter importer;
  REQUIRE(importer.fileImport(incomplete, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(incomplete.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(incomplete.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  REQUIRE(incomplete.dwgAdvancedMetadata().rawDxfObjects().size() == 1u);
  RS_FilterDXFRW filter;
  for (const auto format : {RS2::FormatDXFRW, RS2::FormatDXFRW2010,
                            RS2::FormatDXFRW2013, RS2::FormatDWG2010}) {
    CHECK_FALSE(filter.fileExport(incomplete, QString::fromStdString(output), format));
    std::ifstream retained(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  }
  // Simulate a missing retained carrier. The incomplete typed record must not
  // become an apparently successful but lossy fallback.
  auto &rawObjects = const_cast<std::vector<DRW_RawDxfObject> &>(
      incomplete.dwgAdvancedMetadata().rawDxfObjects());
  rawObjects.clear();
  CHECK_FALSE(filter.fileExport(incomplete, QString::fromStdString(output), RS2::FormatDXFRW));
  std::ifstream retainedAfterRemoval(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retainedAfterRemoval), {}) == sentinel);
  retainedAfterRemoval.close();

  RS_Graphic complete;
  complete.initForNewDocument();
  DRW_Material material;
  material.handle = 0xF040u;
  material.m_name = "RawOnly";
  complete.dwgAdvancedMetadata().addMaterial(material);
  CHECK_FALSE(complete.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  REQUIRE(filter.fileExport(complete, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(recordGroupValues(output, "MATERIAL", "1") == std::vector<std::string>{"RawOnly"});
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

#if defined(LIBDXFRW_HAS_DXF_VERSIONLESS_RAW_MATERIAL_CAPTURE)
TEST_CASE("System codec retains versionless sparse material without replay",
          "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_versionless_sparse_material.dxf");
  const auto output = tmpFile("system_versionless_sparse_material_out.dxf");
  writeText(source, "0\nSECTION\n2\nOBJECTS\n0\nMATERIAL\n5\nF040\n330\n72\n"
                    "100\nAcDbMaterial\n1\nRawOnly\n94\n63\n0\nENDSEC\n0\nEOF\n");
  const std::string sentinel = "preserve unversioned material destination\n";
  writeText(output, sentinel);
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(graphic.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(graphic.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  CHECK(graphic.dwgAdvancedMetadata().materials().front().rawDxfSourceConflict);
  REQUIRE(graphic.dwgAdvancedMetadata().rawDxfObjects().size() == 1u);
  CHECK(graphic.dwgAdvancedMetadata().rawDxfObjects().front().m_version == DRW::UNKNOWNV);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}
#endif

#if defined(LIBDXFRW_HAS_DXF_STAGED_ENTITY_MATERIAL_INSPECTOR) && \
    defined(LIBDXFRW_HAS_DXF_HEADER_MATERIAL_SOURCE_HANDLER) && \
    defined(LIBDXFRW_HAS_DXF_DEFAULT_PLOT_PLACEHOLDER_HANDLE) && \
    defined(LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR) && \
    defined(LIBDXFRW_HAS_DXF_PHYSICAL_ENTITY_SOURCE_HANDLERS) && \
    defined(LIBDXFRW_HAS_DXF_PHYSICAL_IDENTITY_SOURCE_HANDLER) && \
    defined(LIBDXFRW_HAS_DXF_BLOCK_RECORD_SOURCE_SELECTOR)
TEST_CASE("Opt-in system codec replays an unchanged sparse material graph",
          "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_graph_replay_source.dxf");
  const auto output = tmpFile("system_sparse_graph_replay_output.dxf");
  const std::string sourceHandle = GENERATE(std::string("A1"), std::string("14"));
  const bool withSibling = GENERATE(false, true);
  std::string input =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$CMATERIAL\n347\nA1\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\nA1\n0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\nLINE\n5\nA2\n100\nAcDbEntity\n8\nRAWTEST\n347\nA1\n"
      "100\nAcDbLine\n10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
      "100\nAcDbMaterial\n1\nPlainMat\n94\n63\n0\nENDSEC\n0\nEOF\n";
  if (withSibling) {
    const std::string entry = "3\nPlainMat\n350\nA1\n";
    const auto at = input.find(entry);
    REQUIRE(at != std::string::npos);
    input.insert(at + entry.size(), "3\nSibling\n350\nA3\n");
    const auto end = input.rfind("0\nENDSEC\n0\nEOF\n");
    REQUIRE(end != std::string::npos);
    input.insert(end, "0\nMATERIAL\n5\nA3\n330\nA0\n100\nAcDbMaterial\n1\nSibling\n94\n127\n");
  }
  if (sourceHandle != "A1") {
    const std::string replacement = "\n" + sourceHandle + "\n";
    for (std::size_t at = 0; (at = input.find("\nA1\n", at)) != std::string::npos;) {
      input.replace(at, 4u, replacement);
      at += replacement.size();
    }
  }
  writeText(source, input);
  struct OptInFilter : RS_FilterDXFRW {
    std::function<void()> afterObjects;
    bool suppressUnmaterializedLine = false;
    void addLine(const DRW_Line &line) override {
      if (!suppressUnmaterializedLine || line.handle != 0xA5u)
        RS_FilterDXFRW::addLine(line);
    }
#ifdef LIBDXFRW_HAS_DXF_PHYSICAL_IDENTITY_SOURCE_HANDLER
    std::function<void(DRW_DxfIdentitySource &)> mutateIdentity;
    void addDxfIdentitySource(const DRW_DxfIdentitySource &source) override {
      auto copy = source;
      if (mutateIdentity) mutateIdentity(copy);
      RS_FilterDXFRW::addDxfIdentitySource(copy);
    }
#endif
    bool wantsDxfSparseMaterialRawCapture() const noexcept override { return true; }
    bool wantsDxfSparseMaterialRawReplay() const noexcept override { return true; }
    void writeObjects() override {
      RS_FilterDXFRW::writeObjects();
      if (afterObjects) afterObjects();
    }
  } filter;
  RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(graphic.dwgAdvancedMetadata().materials().size() == (withSibling ? 2u : 1u));
  REQUIRE(graphic.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  {
    auto withSecondLine = input;
    const auto end = withSecondLine.find("0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n");
    REQUIRE(end != std::string::npos);
    withSecondLine.insert(end, "0\nLINE\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n"
                               "100\nAcDbLine\n10\n0\n20\n1\n11\n10\n21\n1\n");
    writeText(source, withSecondLine);
    OptInFilter deliveryFilter;
    deliveryFilter.suppressUnmaterializedLine = true;
    RS_Graphic missing;
    REQUIRE(deliveryFilter.fileImport(missing, QString::fromStdString(source), RS2::FormatDXFRW));
    const std::string unchanged = "preserve undelivered line destination\n";
    writeText(output, unchanged);
    CHECK_FALSE(deliveryFilter.fileExport(missing, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == unchanged); }
    deliveryFilter.suppressUnmaterializedLine = false;
    RS_Graphic delivered;
    REQUIRE(deliveryFilter.fileImport(delivered, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(deliveryFilter.fileExport(delivered, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(countRecords(output, "LINE") == 2);
    if (const char *copy = std::getenv("LIBRECAD_KEEP_DELIVERED_MATERIAL_LINES"))
      std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);
    writeText(source, input);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  const std::vector<std::string> expectedFlags = withSibling ? std::vector<std::string>{"63", "127"}
                                                              : std::vector<std::string>{"63"};
  CHECK(recordGroupValues(output, "MATERIAL", "94") == expectedFlags);
  const auto handles = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(handles.size() == (withSibling ? 2u : 1u));
  if (sourceHandle == "14") CHECK(handles.front() != sourceHandle);
  CHECK(recordGroupValues(output, "LINE", "347") == std::vector<std::string>{handles.front()});
  const auto layer = recordGroupsWithValue(output, "LAYER", "2", "RAWTEST");
  CHECK(std::count(layer.begin(), layer.end(), std::make_pair(std::string("347"), handles.front())) == 1);
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto variables = reread.getVariableDict();
  const auto current = variables.constFind(QStringLiteral("$CMATERIAL"));
  REQUIRE(current != variables.cend());
  CHECK(current->getString().toStdString() == handles.front());
  const auto resaved = tmpFile("system_sparse_graph_resave_output.dxf");
  const std::string sentinel = "preserve sparse material destination\n";
  const auto *reopenedLayer = reread.findLayer(QStringLiteral("RAWTEST"));
  REQUIRE(reopenedLayer);
  const auto *reopenedEntry = reread.dwgAdvancedMetadata().findLayerTableEntryByIdentity(reopenedLayer->importedRecordId());
  REQUIRE(reopenedEntry);
  auto *layerSource = const_cast<DRW_RawDxfObject*>(reread.dwgAdvancedMetadata().dxfMaterialLayerSource(reopenedEntry->handle));
  REQUIRE(layerSource);
  const auto plotStyle = std::find_if(layerSource->groups.begin(), layerSource->groups.end(),
                                     [](const auto &group) { return group.code() == 390; });
  REQUIRE(plotStyle != layerSource->groups.end());
  const auto originalPlotStyle = *plotStyle;
  plotStyle->addString(390, "FE");
  writeText(resaved, sentinel);
  CHECK_FALSE(filter.fileExport(reread, QString::fromStdString(resaved), RS2::FormatDXFRW));
  { std::ifstream retained(resaved, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  *plotStyle = originalPlotStyle;
  REQUIRE(filter.fileExport(reread, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(recordGroupValues(resaved, "MATERIAL", "94") == expectedFlags);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_SPARSE_MATERIAL_RESAVED"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  const auto expectRefusal = [&](RS2::FormatType format) {
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), format));
    std::ifstream retained(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  };
  {
    std::string unsafe = input;
    const auto at = unsafe.find("6\nCONTINUOUS\n347\n");
    REQUIRE(at != std::string::npos);
    unsafe.insert(at, "390\nFE\n");
    const auto unsafeSource = tmpFile("system_sparse_graph_unqualified_plot_style.dxf");
    writeText(unsafeSource, unsafe);
    RS_Graphic unsafeGraphic;
    OptInFilter unsafeFilter;
    REQUIRE(unsafeFilter.fileImport(unsafeGraphic, QString::fromStdString(unsafeSource), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(unsafeFilter.fileExport(unsafeGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    std::filesystem::remove(unsafeSource);
  }
#ifdef LIBDXFRW_HAS_DXF_PHYSICAL_IDENTITY_SOURCE_HANDLER
  {
    const auto noEntitiesSource = tmpFile("system_sparse_material_no_entities.dxf");
    auto noEntities = input;
    const auto begin = noEntities.find("0\nSECTION\n2\nENTITIES\n");
    REQUIRE(begin != std::string::npos);
    const auto end = noEntities.find("0\nENDSEC\n", begin);
    REQUIRE(end != std::string::npos);
    noEntities.erase(begin, end + std::string("0\nENDSEC\n").size() - begin);
    writeText(noEntitiesSource, noEntities);
    RS_Graphic emptyGraphic;
    OptInFilter emptyFilter;
    REQUIRE(emptyFilter.fileImport(emptyGraphic, QString::fromStdString(noEntitiesSource), RS2::FormatDXFRW));
    REQUIRE(emptyFilter.fileExport(emptyGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(recordGroupValues(output, "LINE", "347").empty());
    if (const char *copy = std::getenv("LIBRECAD_KEEP_SPARSE_NO_ENTITIES_OUTPUT"))
      std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);
    for (int mode = 0; mode != 5; ++mode) {
      OptInFilter alteredFilter;
      alteredFilter.mutateIdentity = [mode](DRW_DxfIdentitySource &identity) {
        const auto found = std::find_if(identity.records.begin(), identity.records.end(),
            [](const auto &record) { return record.name == "LINE"; });
        if (mode == 2) identity.status = DRW_DxfIdentitySource::Status::Quota;
        else if (found != identity.records.end()) {
          if (mode == 0) identity.records.erase(found);
          else if (mode == 1) { const auto duplicate = *found; identity.records.push_back(duplicate); }
          else ++found->endMarker.ordinal;
        }
      };
      RS_Graphic alteredGraphic;
      REQUIRE(alteredFilter.fileImport(alteredGraphic, QString::fromStdString(source), RS2::FormatDXFRW));
      writeText(output, sentinel);
      CHECK_FALSE(alteredFilter.fileExport(alteredGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      alteredFilter.mutateIdentity = {};
      RS_Graphic corrected;
      REQUIRE(alteredFilter.fileImport(corrected, QString::fromStdString(source), RS2::FormatDXFRW));
      REQUIRE(alteredFilter.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW));
    }
    std::filesystem::remove(noEntitiesSource);
  }
#endif
  {
    const auto duplicateSource = tmpFile("system_sparse_material_duplicate_group.dxf");
    for (int mode = 0; mode != 3; ++mode) {
      auto altered = input;
      const std::string reference = "8\nRAWTEST\n347\n" + sourceHandle + "\n";
      const auto at = altered.find(reference);
      REQUIRE(at != std::string::npos);
      if (mode < 2) {
        altered.insert(at + std::string("8\nRAWTEST\n").size(),
                       "347\n" + (mode == 0 ? std::string("0") : sourceHandle) + "\n");
      } else {
        const auto end = altered.find("0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n", at);
        REQUIRE(end != std::string::npos);
        altered.insert(end, "0\nLINE\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n347\n0\n"
                            "100\nAcDbLine\n10\n0\n20\n0\n11\n1\n21\n1\n");
      }
      writeText(duplicateSource, altered);
      RS_Graphic duplicateGraphic;
      OptInFilter duplicateFilter;
      REQUIRE(duplicateFilter.fileImport(duplicateGraphic, QString::fromStdString(duplicateSource), RS2::FormatDXFRW));
      writeText(output, sentinel);
      CHECK_FALSE(duplicateFilter.fileExport(duplicateGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      writeText(duplicateSource, input);
      RS_Graphic corrected;
      REQUIRE(duplicateFilter.fileImport(corrected, QString::fromStdString(duplicateSource), RS2::FormatDXFRW));
      REQUIRE(duplicateFilter.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW));
    }
    std::filesystem::remove(duplicateSource);
  }
  expectRefusal(RS2::FormatDXFRW2010);
  expectRefusal(RS2::FormatDWG2010);
  {
    RS_FilterDXFRW defaultFilter;
    RS_Graphic ordinary;
    REQUIRE(defaultFilter.fileImport(ordinary, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    REQUIRE(defaultFilter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto ordinaryMaterials = recordGroupValues(output, "MATERIAL", "5");
    REQUIRE(ordinaryMaterials.size() == (withSibling ? 2u : 1u));
    CHECK(recordGroupValues(output, "LINE", "347") == std::vector<std::string>{ordinaryMaterials.front()});
    if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_OUTPUT"))
      std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);
    RS_Graphic reopenedOrdinary;
    RS_FilterDXFRW reopenFilter;
    REQUIRE(reopenFilter.fileImport(reopenedOrdinary, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto ordinaryResave = tmpFile("system_sparse_ordinary_resave.dxf");
    REQUIRE(reopenFilter.fileExport(reopenedOrdinary, QString::fromStdString(ordinaryResave), RS2::FormatDXFRW));
    CHECK(recordGroupValues(ordinaryResave, "LINE", "347").size() == 1u);
    if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_RESAVE"))
      std::filesystem::copy_file(ordinaryResave, copy, std::filesystem::copy_options::overwrite_existing);
    {
      std::ifstream saved(output, std::ios::binary);
      const std::string original((std::istreambuf_iterator<char>(saved)), {});
      const auto unusual = tmpFile("system_sparse_ordinary_unusual_space.dxf");
      for (const std::string marker : {"AcDbBlockTableRecord\n", "AcDbBlockBegin\n", "AcDbBlockEnd\n", "AcDbPlotSettings\n"}) {
        INFO("source marker: " << marker);
        auto altered = original;
        const auto at = altered.find(marker); REQUIRE(at != std::string::npos);
        altered.insert(at + marker.size(), "  4\nCUSTOM\n");
        writeText(unusual, altered);
        RS_Graphic unusualGraphic;
        writeText(ordinaryResave, sentinel);
        const bool imported = reopenFilter.fileImport(unusualGraphic, QString::fromStdString(unusual), RS2::FormatDXFRW);
        if (marker != "AcDbBlockEnd\n") REQUIRE(imported);
        if (imported)
          CHECK_FALSE(reopenFilter.fileExport(unusualGraphic, QString::fromStdString(ordinaryResave), RS2::FormatDXFRW));
        { std::ifstream retained(ordinaryResave, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
        RS_Graphic retry;
        REQUIRE(reopenFilter.fileImport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
        REQUIRE(reopenFilter.fileExport(retry, QString::fromStdString(ordinaryResave), RS2::FormatDXFRW));
      }
      auto noPlotHandle = original;
      const auto plotAt = noPlotHandle.find("\nPLOTSETTINGS\n"); REQUIRE(plotAt != std::string::npos);
      const auto handleAt = noPlotHandle.find("  5\n", plotAt); REQUIRE(handleAt != std::string::npos);
      const auto handleEnd = noPlotHandle.find('\n', handleAt + 4u); REQUIRE(handleEnd != std::string::npos);
      noPlotHandle.erase(handleAt, handleEnd + 1u - handleAt);
      writeText(unusual, noPlotHandle);
      RS_Graphic withoutHandle;
      writeText(ordinaryResave, sentinel);
      if (reopenFilter.fileImport(withoutHandle, QString::fromStdString(unusual), RS2::FormatDXFRW))
        CHECK_FALSE(reopenFilter.fileExport(withoutHandle, QString::fromStdString(ordinaryResave), RS2::FormatDXFRW));
      { std::ifstream retained(ordinaryResave, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      std::filesystem::remove(unusual);
    }
    std::filesystem::remove(ordinaryResave);
    RS_FilterDXFRW anotherWriter;
    REQUIRE(anotherWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    auto *activePlot = ordinary.getPlotSettings();
    REQUIRE(activePlot);
    activePlot->setMarginsInMm(1.0, 0.0, 0.0, 0.0);
    writeText(output, sentinel);
    CHECK_FALSE(anotherWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    activePlot->setMarginsInMm(0.0, 0.0, 0.0, 0.0);
    REQUIRE(anotherWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    auto *newPoint = new RS_Point(&ordinary, RS_Vector(2.0, 3.0, 1.0));
    ordinary.addEntity(newPoint);
    writeText(output, sentinel);
    CHECK_FALSE(anotherWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    REQUIRE(ordinary.removeEntity(newPoint));
    REQUIRE(anotherWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    struct OrdinaryLateWriter : RS_FilterDXFRW {
      std::function<void()> afterObjects;
      void writeObjects() override {
        RS_FilterDXFRW::writeObjects();
        if (afterObjects) afterObjects();
      }
    } lateWriter;
    auto &ordinaryMetadata = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord>&>(
        ordinary.dwgAdvancedMetadata().materials());
    REQUIRE_FALSE(ordinaryMetadata.empty());
    const auto description = ordinaryMetadata.front().description;
    bool reachedObjects = false;
    lateWriter.afterObjects = [&] { reachedObjects = true; ordinaryMetadata.front().description = "late edit"; };
    writeText(output, sentinel);
    CHECK_FALSE(lateWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(reachedObjects);
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    ordinaryMetadata.front().description = description;
    lateWriter.afterObjects = {};
    REQUIRE(lateWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(anotherWriter.fileExport(ordinary, QString::fromStdString(output), RS2::FormatDXFRW2010));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    const auto unqualifiedSource = tmpFile("system_sparse_ordinary_unqualified.dxf");
    for (int mode = 0; mode != 4; ++mode) {
      auto altered = input;
      if (mode == 0) {
        const auto at = altered.find("100\nAcDbLine\n"); REQUIRE(at != std::string::npos);
        altered.insert(at + std::string("100\nAcDbLine\n").size(), "39\n1\n");
      } else if (mode == 1) {
        const auto at = altered.find("0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"); REQUIRE(at != std::string::npos);
        altered.insert(at, "0\nPOINT\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n"
                           "100\nAcDbPoint\n10\n0\n20\n0\n30\n1\n");
      } else if (mode == 2) {
        const auto at = altered.find("0\nSECTION\n2\nENTITIES\n"); REQUIRE(at != std::string::npos);
        altered.insert(at, "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n5\nB0\n8\n0\n2\nB\n70\n0\n"
                           "10\n0\n20\n0\n30\n0\n0\nENDBLK\n5\nB1\n8\n0\n0\nENDSEC\n");
      } else if (mode == 3) {
        const auto at = altered.find("30\n0\n11\n10\n"); REQUIRE(at != std::string::npos);
        altered.replace(at, std::string("30\n0\n").size(), "30\n1\n");
      } else {
        const std::string from = "0\nLINE\n5\nA2\n";
        const auto at = altered.find(from); REQUIRE(at != std::string::npos);
        altered.replace(at, from.size(), "0\nLINE\n5\n" + sourceHandle + "\n");
      }
      writeText(unqualifiedSource, altered);
      RS_Graphic unqualified;
      const bool imported = defaultFilter.fileImport(unqualified, QString::fromStdString(unqualifiedSource), RS2::FormatDXFRW);
      if (mode != 4) REQUIRE(imported);
      writeText(output, sentinel);
      if (imported) CHECK_FALSE(defaultFilter.fileExport(unqualified, QString::fromStdString(output), RS2::FormatDXFRW));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      RS_Graphic corrected;
      REQUIRE(defaultFilter.fileImport(corrected, QString::fromStdString(source), RS2::FormatDXFRW));
      REQUIRE(defaultFilter.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW));
    }
    std::filesystem::remove(unqualifiedSource);
    struct DifferentLineFilter : RS_FilterDXFRW {
      bool change = true;
      bool suppressSecond = false;
      bool changeColor = false;
      bool duplicatePhysicalIdentity = false;
      bool alterPhysicalCoordinate = false;
      void addDxfEntitiesSource(const DRW_DxfEntitiesSource &source) override {
        auto copy = source;
        if (alterPhysicalCoordinate && !copy.records.empty()) {
          auto &record = copy.records.front();
          for (std::size_t i = 0; i < record.groups.size(); ++i)
            if (record.groups[i].code() == 10) { record.rawValues[i] = "999"; break; }
        }
        RS_FilterDXFRW::addDxfEntitiesSource(copy);
      }
      void addDxfIdentitySource(const DRW_DxfIdentitySource &source) override {
        auto copy = source;
        if (duplicatePhysicalIdentity) {
          const auto candidate = std::find_if(copy.candidates.begin(), copy.candidates.end(),
              [](const auto &item) { return item.scope == DRW_DxfIdentitySource::Scope::Outer; });
          if (candidate != copy.candidates.end()) {
            const auto duplicate = *candidate;
            copy.candidates.push_back(duplicate);
          }
        }
        RS_FilterDXFRW::addDxfIdentitySource(copy);
      }
      void addLine(const DRW_Line &data) override {
        if (suppressSecond && data.handle == 0xA5u) return;
        DRW_Line changed = data;
        if (change) changed.basePoint.x += 1.0;
        if (changeColor) changed.color = 1;
        RS_FilterDXFRW::addLine(changed);
      }
    } different;
    RS_Graphic changed;
    REQUIRE(different.fileImport(changed, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(different.fileExport(changed, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    different.change = false;
    RS_Graphic corrected;
    REQUIRE(different.fileImport(corrected, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(different.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW));
    different.changeColor = true;
    RS_Graphic recolored;
    REQUIRE(different.fileImport(recolored, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(different.fileExport(recolored, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    different.changeColor = false;
    different.duplicatePhysicalIdentity = true;
    RS_Graphic collision;
    REQUIRE(different.fileImport(collision, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(different.fileExport(collision, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    different.duplicatePhysicalIdentity = false;
    RS_Graphic noCollision;
    REQUIRE(different.fileImport(noCollision, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(different.fileExport(noCollision, QString::fromStdString(output), RS2::FormatDXFRW));
    different.alterPhysicalCoordinate = true;
    RS_Graphic changedPhysical;
    REQUIRE(different.fileImport(changedPhysical, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(different.fileExport(changedPhysical, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    different.alterPhysicalCoordinate = false;
    RS_Graphic validPhysical;
    REQUIRE(different.fileImport(validPhysical, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(different.fileExport(validPhysical, QString::fromStdString(output), RS2::FormatDXFRW));
    auto twoLines = input;
    const auto end = twoLines.find("0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n");
    REQUIRE(end != std::string::npos);
    twoLines.insert(end, "0\nLINE\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n"
                         "100\nAcDbLine\n10\n0\n20\n1\n11\n10\n21\n1\n");
    writeText(unqualifiedSource, twoLines);
    different.suppressSecond = true;
    RS_Graphic missing;
    REQUIRE(different.fileImport(missing, QString::fromStdString(unqualifiedSource), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(different.fileExport(missing, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    different.suppressSecond = false;
    RS_Graphic complete;
    REQUIRE(different.fileImport(complete, QString::fromStdString(unqualifiedSource), RS2::FormatDXFRW));
    RS_Line *editable = nullptr, *linked = nullptr;
    for (RS_Entity *entity : complete)
      if (entity && entity->rtti() == RS2::EntityLine) {
        if (entity->sourceHandle() == 0xA5u) editable = static_cast<RS_Line *>(entity);
        if (entity->sourceHandle() == 0xA2u) linked = static_cast<RS_Line *>(entity);
      }
    REQUIRE(editable); REQUIRE(linked);
    editable->setEndpoint(RS_Vector(20.0, 1.0));
    linked->setEndpoint(RS_Vector(12.0, 0.0));
    REQUIRE(different.fileExport(complete, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(countRecords(output, "LINE") == 2);
    CHECK(recordGroupValues(output, "LINE", "11") == std::vector<std::string>{"12", "20"});
    RS_Graphic editedReopen;
    REQUIRE(different.fileImport(editedReopen, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(different.fileExport(editedReopen, QString::fromStdString(ordinaryResave), RS2::FormatDXFRW));
    CHECK(recordGroupValues(ordinaryResave, "LINE", "11") == std::vector<std::string>{"12", "20"});
    if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_EDITED_RESAVE"))
      std::filesystem::copy_file(ordinaryResave, copy, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(ordinaryResave);
    std::filesystem::remove(unqualifiedSource);
  }
  // A handle's complete byte sequence must be validated, not just the C
  // string prefix before an embedded NUL. Mutate both retained copies to
  // model a malformed source consistently, then require preflight refusal.
  {
    auto *dictionarySource = const_cast<DRW_RawDxfObject *>(
        graphic.dwgAdvancedMetadata().dxfMaterialDictionarySource());
    REQUIRE(dictionarySource);
    auto &objects = const_cast<std::vector<DRW_RawDxfObject> &>(
        graphic.dwgAdvancedMetadata().rawDxfObjects());
    const auto found = std::find_if(objects.begin(), objects.end(), [&](const auto &object) {
      return object.name == "DICTIONARY" && object.handle == dictionarySource->handle;
    });
    REQUIRE(found != objects.end());
    auto &dictionaryRaw = *found;
    REQUIRE(dictionarySource->hasRawValues); REQUIRE(dictionaryRaw.hasRawValues);
    const std::vector<std::pair<int, std::string>> probes = {{5, "A0"}, {330, "C"}, {350, sourceHandle}};
    for (const auto &probe : probes) {
      auto sourceGroup = std::find_if(dictionarySource->groups.begin(), dictionarySource->groups.end(),
          [&](const auto &group) { return group.code() == probe.first && group.c_str() && probe.second == group.c_str(); });
      auto rawGroup = std::find_if(dictionaryRaw.groups.begin(), dictionaryRaw.groups.end(),
          [&](const auto &group) { return group.code() == probe.first && group.c_str() && probe.second == group.c_str(); });
      REQUIRE(sourceGroup != dictionarySource->groups.end()); REQUIRE(rawGroup != dictionaryRaw.groups.end());
      const auto sourceIndex = static_cast<std::size_t>(sourceGroup - dictionarySource->groups.begin());
      const auto rawIndex = static_cast<std::size_t>(rawGroup - dictionaryRaw.groups.begin());
      const auto savedSourceGroup = *sourceGroup, savedRawGroup = *rawGroup;
      const auto savedSourceValue = dictionarySource->rawValues[sourceIndex], savedRawValue = dictionaryRaw.rawValues[rawIndex];
      const std::string malformed = probe.second + std::string("\0BAD", 4);
      for (int mode = 0; mode != 3; ++mode) {
        if (mode != 1) { sourceGroup->addString(probe.first, malformed); rawGroup->addString(probe.first, malformed); }
        if (mode != 2) { dictionarySource->rawValues[sourceIndex] = malformed; dictionaryRaw.rawValues[rawIndex] = malformed; }
        bool reachedObjects = false;
        filter.afterObjects = [&] { reachedObjects = true; };
        expectRefusal(RS2::FormatDXFRW);
        CHECK_FALSE(reachedObjects);
        *sourceGroup = savedSourceGroup; *rawGroup = savedRawGroup;
        dictionarySource->rawValues[sourceIndex] = savedSourceValue; dictionaryRaw.rawValues[rawIndex] = savedRawValue;
        filter.afterObjects = {};
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
      }
    }
  }
  auto &materials = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord>&>(
      graphic.dwgAdvancedMetadata().materials());
  const auto description = materials.front().description;
  filter.afterObjects = [&] { materials.front().description = "late edit"; };
  expectRefusal(RS2::FormatDXFRW);
  materials.front().description = description;
  filter.afterObjects = {};
  auto &raw = const_cast<std::vector<DRW_RawDxfObject>&>(graphic.dwgAdvancedMetadata().rawDxfObjects());
  const auto carrier = std::find_if(raw.begin(), raw.end(), [](const auto &record) { return record.name == "MATERIAL"; });
  REQUIRE(carrier != raw.end());
  const auto tail = std::find_if(carrier->groups.begin(), carrier->groups.end(),
                                 [](const auto &group) { return group.code() == 94; });
  REQUIRE(tail != carrier->groups.end());
  const auto savedTail = *tail;
  tail->addInt(94, 64);
  expectRefusal(RS2::FormatDXFRW);
  *tail = savedTail;
  filter.afterObjects = [&] { tail->addInt(94, 64); };
  expectRefusal(RS2::FormatDXFRW);
  *tail = savedTail;
  filter.afterObjects = {};
  auto *line = graphic.firstEntity();
  REQUIRE(line != nullptr);
  const auto originalReference = line->materialHandle();
  // Source graph identity must be checked before output, not just compared
  // with a fresh snapshot taken after a prior edit.
  line->setMaterialHandle(0u);
  expectRefusal(RS2::FormatDXFRW);
  line->setMaterialHandle(originalReference);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  if (withSibling) {
    line->setMaterialHandle(0xA3u);
    expectRefusal(RS2::FormatDXFRW);
    line->setMaterialHandle(originalReference);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  line->setFlag(RS2::FlagDeleted);
  expectRefusal(RS2::FormatDXFRW);
  line->delFlag(RS2::FlagDeleted);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  auto *duplicate = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 1.0), RS_Vector(1.0, 1.0)));
  duplicate->setSourceHandle(line->sourceHandle());
  graphic.addEntity(duplicate);
  bool reachedDuplicateObjects = false;
  filter.afterObjects = [&] { reachedDuplicateObjects = true; };
  expectRefusal(RS2::FormatDXFRW);
  CHECK_FALSE(reachedDuplicateObjects);
  duplicate->setFlag(RS2::FlagDeleted);
  filter.afterObjects = {};
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  filter.afterObjects = [&] { line->setMaterialHandle(0u); };
  expectRefusal(RS2::FormatDXFRW);
  line->setMaterialHandle(originalReference);
  filter.afterObjects = {};
  line->setReactorHandles({0xA0u});
  expectRefusal(RS2::FormatDXFRW);
  line->setReactorHandles({});
  filter.afterObjects = [&] { line->setXDictHandle(0xA0u); };
  expectRefusal(RS2::FormatDXFRW);
  line->setXDictHandle(0u);
  filter.afterObjects = {};
  const auto originalExtData = line->getDrwExtData();
  line->setDrwExtData({std::make_shared<DRW_Variant>(1005, std::string("A0"))});
  expectRefusal(RS2::FormatDXFRW);
  line->setDrwExtData(originalExtData);
  const auto savedSource = materials.front().rawDxfSource;
  materials.front().rawDxfSource.reset();
  expectRefusal(RS2::FormatDXFRW);
  materials.front().rawDxfSource = savedSource;
  if (withSibling) {
    const auto sibling = std::find_if(raw.begin(), raw.end(), [](const auto &record) {
      return record.name == "MATERIAL" && record.handle == 0xA3u;
    });
    REQUIRE(sibling != raw.end());
    const auto saved = *sibling;
    raw.erase(sibling);
    expectRefusal(RS2::FormatDXFRW);
    raw.push_back(saved);
  }
  DRW_RawDxfObject unrelated;
  unrelated.name = "XRECORD";
  unrelated.handle = 0xF200u;
  unrelated.parentHandle = 0xA0u;
  unrelated.m_version = DRW::AC1021;
  raw.push_back(unrelated);
  expectRefusal(RS2::FormatDXFRW);
  raw.pop_back();
  DRW_Material orphan;
  orphan.handle = 0xF100u;
  orphan.m_name = "Orphan";
  graphic.dwgAdvancedMetadata().addMaterial(orphan);
  expectRefusal(RS2::FormatDXFRW);
  materials.pop_back();
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  if (const char *copy = std::getenv("LIBRECAD_KEEP_SPARSE_MATERIAL_OUTPUT"))
    std::filesystem::copy_file(output, copy, std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(source);
  std::filesystem::remove(output);
  std::filesystem::remove(resaved);
}

TEST_CASE("Ordinary sparse material saves simple points", "[system-codec][dxf][material][transaction][point]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_point_source.dxf");
  const auto output = tmpFile("system_sparse_point_output.dxf");
  const auto resaved = tmpFile("system_sparse_point_resave.dxf");
  const std::string input =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$CMATERIAL\n347\nA1\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\nA1\n0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\nPOINT\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n62\n2\n347\nA1\n"
      "100\nAcDbPoint\n10\n2\n20\n3\n30\n0\n"
      "0\nPOINT\n5\nA6\n100\nAcDbEntity\n8\nRAWTEST\n100\nAcDbPoint\n10\n4\n20\n5\n"
      "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
      "100\nAcDbMaterial\n1\nPlainMat\n94\n63\n0\nENDSEC\n0\nEOF\n";
  writeText(source, input);
  RS_FilterDXFRW filter; RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  RS_Point *point = nullptr;
  for (RS_Entity *entity : graphic)
    if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityPoint)
      point = static_cast<RS_Point *>(entity);
  REQUIRE(point); CHECK(point->materialHandle() == 0xA1u);
  point->setPos(RS_Vector(6.0, 7.0));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "POINT", "10") == std::vector<std::string>{"6", "4"});
  CHECK(recordGroupValues(output, "POINT", "20") == std::vector<std::string>{"7", "5"});
  const auto material = recordGroupValues(output, "MATERIAL", "5"); REQUIRE(material.size() == 1u);
  CHECK(recordGroupValues(output, "POINT", "347") == std::vector<std::string>{material.front()});
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(recordGroupValues(resaved, "POINT", "10") == std::vector<std::string>{"6", "4"});
  auto *layer = graphic.findLayer(QStringLiteral("RAWTEST")); REQUIRE(layer);
  auto *added = new RS_Point(&graphic, RS_Vector(8.0, 9.0));
  added->setLayer(layer); added->setPen(point->getPen(false)); graphic.addEntity(added);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  RS_Graphic additions;
  REQUIRE(filter.fileImport(additions, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(additions, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(recordGroupValues(resaved, "POINT", "10") == std::vector<std::string>{"6", "4", "8"});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_POINT"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  REQUIRE(graphic.removeEntity(added));
  const std::string sentinel = "preserve sparse point destination\n";
  for (const std::string extra : {"30\n2\n", "39\n1\n", "50\n30\n", "210\n0\n220\n1\n230\n0\n"}) {
    auto unsupported = input;
    const auto at = unsupported.find("100\nAcDbPoint\n10\n2\n20\n3\n30\n0\n"); REQUIRE(at != std::string::npos);
    unsupported.replace(at, std::string("100\nAcDbPoint\n10\n2\n20\n3\n30\n0\n").size(),
                        "100\nAcDbPoint\n10\n2\n20\n3\n" + extra);
    writeText(source, unsupported); RS_Graphic rejected; writeText(output, sentinel);
    if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  struct PointDeliveryProbe : RS_FilterDXFRW {
    std::uint32_t suppress = 0;
    bool alterTyped = false, alterPhysical = false;
    void addPoint(const DRW_Point &data) override {
      if (data.handle == suppress) return;
      DRW_Point changed = data;
      if (alterTyped && data.handle == 0xA5u) changed.basePoint.x += 1.0;
      RS_FilterDXFRW::addPoint(changed);
    }
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &data) override {
      auto copy = data;
      if (alterPhysical) for (auto &record : copy.records)
        if (record.name == "POINT") for (std::size_t i = 0; i < record.groups.size(); ++i)
          if (record.groups[i].code() == 10) { record.rawValues[i] = "999"; break; }
      RS_FilterDXFRW::addDxfEntitiesSource(copy);
    }
  } probe;
  writeText(source, input);
  for (int mode = 0; mode != 4; ++mode) {
    probe.suppress = mode == 0 ? 0xA5u : mode == 1 ? 0xA6u : 0;
    probe.alterTyped = mode == 2; probe.alterPhysical = mode == 3;
    RS_Graphic rejected;
    REQUIRE(probe.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(probe.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    probe.suppress = 0; probe.alterTyped = probe.alterPhysical = false;
    RS_Graphic retry;
    REQUIRE(probe.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(probe.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  std::filesystem::remove(source); std::filesystem::remove(output); std::filesystem::remove(resaved);
}

static std::string sparseMaterialPolylineSource() {
  return
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$CMATERIAL\n347\nA1\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\nA1\n0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\nLWPOLYLINE\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n347\nA1\n"
      "100\nAcDbPolyline\n90\n3\n70\n0\n10\n1\n20\n2\n10\n4\n20\n6\n10\n8\n20\n3\n"
      "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
      "100\nAcDbMaterial\n1\nPlainMat\n94\n63\n0\nENDSEC\n0\nEOF\n";
}

TEST_CASE("Ordinary sparse material accepts explicit planar defaults", "[system-codec][dxf][material][transaction][point]") {
  ensureSettings();
  const std::string kind = GENERATE(std::string("POINT"), std::string("LINE"),
                                    std::string("CIRCLE"), std::string("ARC"),
                                    std::string("ELLIPSE"), std::string("LWPOLYLINE"));
  const bool isPoint = kind == "POINT", isLine = kind == "LINE", isArc = kind == "ARC";
  const bool isEllipse = kind == "ELLIPSE", isPolyline = kind == "LWPOLYLINE";
  const auto source = tmpFile("system_sparse_point_defaults_source.dxf");
  const auto output = tmpFile("system_sparse_point_defaults_output.dxf");
  const auto resaved = tmpFile("system_sparse_point_defaults_resave.dxf");
  const std::string prefix =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$CMATERIAL\n347\nA1\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\nA1\n0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\n" + kind + "\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n62\n2\n347\nA1\n" +
      (isPoint ? "100\nAcDbPoint\n10\n2\n20\n3\n30\n0\n" :
       isLine ? "100\nAcDbLine\n10\n2\n20\n3\n30\n0\n11\n4\n21\n5\n31\n0\n" :
       isEllipse ? "100\nAcDbEllipse\n10\n2\n20\n3\n30\n0\n11\n4\n21\n0\n31\n0\n40\n0.5\n41\n0\n42\n6.283185307179586\n" :
       isPolyline ? "100\nAcDbPolyline\n90\n3\n70\n0\n10\n2\n20\n3\n10\n4\n20\n5\n10\n6\n20\n3\n" :
                "100\nAcDbCircle\n10\n2\n20\n3\n30\n0\n40\n2\n");
  const std::string arcSubclass = isArc ? "100\nAcDbArc\n50\n30\n51\n120\n" : "";
  const std::string suffix =
      "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
      "100\nAcDbMaterial\n1\nPlainMat\n94\n63\n0\nENDSEC\n0\nEOF\n";
  RS_FilterDXFRW filter;
  std::vector<std::pair<std::string, bool>> defaultCases{{"39\n0\n", false}, {"210\n0\n220\n0\n230\n1\n", false},
      {"39\n0\n210\n0\n220\n0\n230\n1\n", false}};
  if (isEllipse) defaultCases = {{"210\n0\n220\n0\n230\n1\n", false}};
  if (isPoint) { defaultCases.push_back({"50\n0\n", false}); defaultCases.push_back({"39\n0\n50\n0\n210\n0\n220\n0\n230\n1\n", false}); }
  if (isArc) defaultCases.push_back({"210\n0\n220\n0\n230\n1\n", true});
  for (const auto &entry : defaultCases) {
    writeText(source, prefix + (entry.second ? "" : entry.first) + arcSubclass + (entry.second ? entry.first : "") + suffix);
    RS_Graphic graphic;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    RS_Entity *selected = nullptr;
    for (RS_Entity *entity : graphic)
      if (entity && entity->sourceHandle() == 0xA5u &&
          entity->rtti() == (isPoint ? RS2::EntityPoint : isLine ? RS2::EntityLine : isArc ? RS2::EntityArc :
                             isEllipse ? RS2::EntityEllipse : isPolyline ? RS2::EntityPolyline : RS2::EntityCircle)) selected = entity;
    REQUIRE(selected);
    selected->move(RS_Vector(4.0, 4.0));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto xs = recordGroupValues(output, kind, "10"), ys = recordGroupValues(output, kind, "20");
    REQUIRE(xs.size() == (isPolyline ? 3u : 1u)); REQUIRE(ys.size() == xs.size());
    CHECK(xs.front() == "6"); CHECK(ys.front() == "7");
    const auto material = recordGroupValues(output, "MATERIAL", "5"); REQUIRE(material.size() == 1u);
    CHECK(recordGroupValues(output, kind, "347") == material);
    RS_Graphic reopened;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
    const char *key = isPoint ? "LIBRECAD_KEEP_SPARSE_POINT_DEFAULTS" : isLine ? "LIBRECAD_KEEP_SPARSE_LINE_DEFAULTS" :
        isArc ? "LIBRECAD_KEEP_SPARSE_ARC_DEFAULTS" : isEllipse ? "LIBRECAD_KEEP_SPARSE_ELLIPSE_DEFAULTS" :
        isPolyline ? "LIBRECAD_KEEP_SPARSE_POLYLINE_DEFAULTS" : "LIBRECAD_KEEP_SPARSE_CIRCLE_DEFAULTS";
    if (const char *copy = std::getenv(key))
      std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  }
  const std::string sentinel = "preserve destination on unsupported point\n";
  struct DefaultPointProbe : RS_FilterDXFRW {
    bool alterPhysical = false;
    bool alterTyped = false;
    void addPoint(const DRW_Point &data) override {
      DRW_Point changed = data;
      if (alterTyped && data.handle == 0xA5u) changed.thickness = 1.0;
      RS_FilterDXFRW::addPoint(changed);
    }
    void addLine(const DRW_Line &data) override {
      DRW_Line changed = data;
      if (alterTyped && data.handle == 0xA5u) changed.thickness = 1.0;
      RS_FilterDXFRW::addLine(changed);
    }
    void addCircle(const DRW_Circle &data) override {
      DRW_Circle changed = data;
      if (alterTyped && data.handle == 0xA5u) changed.thickness = 1.0;
      RS_FilterDXFRW::addCircle(changed);
    }
    void addArc(const DRW_Arc &data) override {
      DRW_Arc changed = data;
      if (alterTyped && data.handle == 0xA5u) changed.thickness = 1.0;
      RS_FilterDXFRW::addArc(changed);
    }
    void addEllipse(const DRW_Ellipse &data) override {
      DRW_Ellipse changed = data;
      if (alterTyped && data.handle == 0xA5u) changed.extPoint.x = 1.0;
      RS_FilterDXFRW::addEllipse(changed);
    }
    void addLWPolyline(const DRW_LWPolyline &data) override {
      DRW_LWPolyline changed = data;
      if (alterTyped && data.handle == 0xA5u) changed.extPoint.x = 1.0;
      RS_FilterDXFRW::addLWPolyline(changed);
    }
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &data) override {
      auto copy = data;
      if (alterPhysical) for (auto &record : copy.records)
        if (record.name == "POINT" || record.name == "LINE" || record.name == "CIRCLE" || record.name == "ARC" || record.name == "ELLIPSE" || record.name == "LWPOLYLINE") for (std::size_t i = 0; i < record.groups.size(); ++i)
          if (record.groups[i].code() == 210) record.rawValues[i] = "1";
      RS_FilterDXFRW::addDxfEntitiesSource(copy);
    }
  } probe;
  for (int mode = 0; mode < 2; ++mode) {
    writeText(source, prefix + "210\n0\n220\n0\n230\n1\n" + arcSubclass + suffix);
    probe.alterPhysical = mode == 0; probe.alterTyped = mode == 1;
    RS_Graphic rejected; REQUIRE(probe.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(probe.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    probe.alterPhysical = false; probe.alterTyped = false;
    RS_Graphic retry; REQUIRE(probe.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(probe.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  if (isArc) {
    for (const auto &input : {prefix + arcSubclass + "39\n0\n" + suffix,
                              prefix + "210\n0\n" + arcSubclass + "220\n0\n230\n1\n" + suffix}) {
      writeText(source, input); RS_Graphic rejected; writeText(output, sentinel);
      if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
        CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
      std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
    }
  }
  for (const std::string bad : {"39\n1\n", "50\n30\n", "210\n0\n220\n1\n230\n0\n",
                                "210\n0\n", "39\n0\n39\n0\n", "50\nnan\n"}) {
    writeText(source, prefix + bad + arcSubclass + suffix); RS_Graphic rejected; writeText(output, sentinel);
    if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    std::ifstream retained(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  }
}

TEST_CASE("Ordinary sparse material accepts explicit common defaults", "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_material_model_space_source.dxf");
  const auto output = tmpFile("system_sparse_material_model_space_output.dxf");
  const auto resaved = tmpFile("system_sparse_material_model_space_resaved.dxf");
  const std::string sentinel = "preserve destination on invalid model space\n";
  const std::vector<std::pair<std::string, std::string>> entities = {
      {"POINT", "100\nAcDbPoint\n10\n1\n20\n2\n"},
      {"LINE", "100\nAcDbLine\n10\n1\n20\n2\n11\n4\n21\n6\n"},
      {"CIRCLE", "100\nAcDbCircle\n10\n1\n20\n2\n40\n3\n"},
      {"ARC", "100\nAcDbCircle\n10\n1\n20\n2\n40\n3\n100\nAcDbArc\n50\n30\n51\n120\n"},
      {"ELLIPSE", "100\nAcDbEllipse\n10\n1\n20\n2\n11\n4\n21\n3\n40\n0.5\n41\n0.25\n42\n2.5\n"},
      {"LWPOLYLINE", "100\nAcDbPolyline\n90\n3\n70\n0\n10\n1\n20\n2\n10\n4\n20\n6\n10\n8\n20\n3\n"},
  };
  struct SpaceProbe : RS_FilterDXFRW {
    int alter = 0;
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &data) override {
      auto copy = data;
      if (alter) for (auto &record : copy.records) for (std::size_t i = 0; i < record.groups.size(); ++i)
        if (record.groups[i].code() == alter) record.rawValues[i] = alter == 48 ? "2" : "1";
      RS_FilterDXFRW::addDxfEntitiesSource(copy);
    }
  } filter;
  for (const auto &entry : entities) {
    CAPTURE(entry.first);
    auto base = sparseMaterialPolylineSource();
    const auto begin = base.find("0\nLWPOLYLINE\n5\nA5\n"), end = base.find("0\nENDSEC\n", begin);
    REQUIRE(begin != std::string::npos); REQUIRE(end != std::string::npos);
    const std::string common = "0\n" + entry.first + "\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n62\n2\n347\nA1\n";
    base.replace(begin, end - begin, common + "67\n0\n48\n1\n60\n0\n" + entry.second);
    writeText(source, base); RS_Graphic graphic;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    RS_Entity *selected = nullptr;
    for (RS_Entity *entity : graphic) if (entity && entity->sourceHandle() == 0xA5u) selected = entity;
    REQUIRE(selected); selected->move(RS_Vector(4, 4));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto xs = recordGroupValues(output, entry.first, "10"), ys = recordGroupValues(output, entry.first, "20");
    REQUIRE_FALSE(xs.empty()); REQUIRE_FALSE(ys.empty()); CHECK(xs.front() == "5"); CHECK(ys.front() == "6");
    const auto material = recordGroupValues(output, "MATERIAL", "5"); REQUIRE(material.size() == 1u);
    CHECK(recordGroupValues(output, entry.first, "347") == material);
    RS_Graphic reopened; REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_SPARSE_MODEL_SPACE"))
      std::filesystem::copy_file(resaved, std::string(prefix) + entry.first + ".dxf", std::filesystem::copy_options::overwrite_existing);
    selected->setVisible(false); writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    selected->setVisible(true); REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    for (const auto &bad : std::vector<std::pair<std::string, std::string>>{
             {"67\n0\n", "67\n1\n"}, {"67\n0\n", "67\n2\n"}, {"67\n0\n", "67\n-1\n"},
             {"67\n0\n", "67\n0\n67\n0\n"}, {"67\n0\n", "67\n0junk\n"},
             {"48\n1\n", "48\n2\n"}, {"48\n1\n", "48\nnan\n"}, {"48\n1\n", "48\n1\n48\n1\n"},
             {"60\n0\n", "60\n1\n"}, {"60\n0\n", "60\n2\n"}, {"60\n0\n", "60\n0\n60\n0\n"}}) {
      auto invalid = base; const auto pos = invalid.find(bad.first, begin); REQUIRE(pos != std::string::npos);
      invalid.replace(pos, bad.first.size(), bad.second); writeText(source, invalid); writeText(output, sentinel);
      RS_Graphic rejected;
      if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
        CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
      std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    }
    for (const std::string field : {"67\n0\n", "48\n1\n", "60\n0\n"}) {
      auto misplaced = base; const auto pos = misplaced.find(field, begin); REQUIRE(pos != std::string::npos);
      misplaced.erase(pos, field.size()); const auto geometry = misplaced.find(entry.second, begin); REQUIRE(geometry != std::string::npos);
      const auto markerEnd = misplaced.find('\n', misplaced.find('\n', geometry) + 1); REQUIRE(markerEnd != std::string::npos);
      misplaced.insert(markerEnd + 1, field); writeText(source, misplaced); writeText(output, sentinel);
      RS_Graphic misplacedGraphic;
      if (filter.fileImport(misplacedGraphic, QString::fromStdString(source), RS2::FormatDXFRW))
        CHECK_FALSE(filter.fileExport(misplacedGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    }
    for (int code : {67, 48, 60}) {
      writeText(source, base); filter.alter = code; RS_Graphic rejected;
      REQUIRE(filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW)); writeText(output, sentinel);
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      filter.alter = 0; RS_Graphic retry;
      REQUIRE(filter.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
      REQUIRE(filter.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
    }
  }
}

TEST_CASE("Ordinary sparse material preserves exact true colors",
          "[system-codec][dxf][material][transaction][truecolor]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_truecolor_source.dxf");
  const auto output = tmpFile("system_sparse_truecolor_output.dxf");
  const auto resaved = tmpFile("system_sparse_truecolor_resaved.dxf");
  const std::string sentinel = "preserve true color destination\n";
  const std::vector<std::pair<std::string, std::string>> entities = {
      {"POINT", "100\nAcDbPoint\n10\n1\n20\n2\n"},
      {"LINE", "100\nAcDbLine\n10\n1\n20\n2\n11\n4\n21\n6\n"},
      {"CIRCLE", "100\nAcDbCircle\n10\n1\n20\n2\n40\n3\n"},
      {"ARC", "100\nAcDbCircle\n10\n1\n20\n2\n40\n3\n100\nAcDbArc\n50\n30\n51\n120\n"},
      {"ELLIPSE", "100\nAcDbEllipse\n10\n1\n20\n2\n11\n4\n21\n3\n40\n0.5\n41\n0.25\n42\n2.5\n"},
      {"LWPOLYLINE", "100\nAcDbPolyline\n90\n3\n70\n0\n10\n1\n20\n2\n10\n4\n20\n6\n10\n8\n20\n3\n"},
  };
  for (const auto &entry : entities) for (const RS_Color color : {RS_Color(10, 20, 30), RS_Color(18, 52, 86)}) {
    CAPTURE(entry.first, color.red(), color.green(), color.blue());
    int rgb = -1; const int index = LC_ColorNumbers::colorToNumber(color, &rgb);
    REQUIRE(rgb == ((color.red() << 16) | (color.green() << 8) | color.blue()));
    auto input = sparseMaterialPolylineSource();
    const std::string table = "0\nTABLE\n2\nLAYER\n70\n1\n0\nLAYER\n5\n30\n";
    const auto tablePos = input.find(table); REQUIRE(tablePos != std::string::npos);
    input.replace(tablePos, table.size(), "0\nTABLE\n2\nLAYER\n5\n2\n330\n0\n"
                  "100\nAcDbSymbolTable\n70\n1\n0\nLAYER\n5\n30\n330\n2\n");
    const auto begin = input.find("0\nLWPOLYLINE\n5\nA5\n");
    const auto end = input.find("0\nENDSEC\n", begin);
    REQUIRE(begin != std::string::npos); REQUIRE(end != std::string::npos);
    const auto record = "0\n" + entry.first + "\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n62\n" +
        std::to_string(index) + "\n420\n" + std::to_string(rgb) + "\n347\nA1\n" + entry.second;
    input.replace(begin, end - begin, record);
    writeText(source, input);
    RS_FilterDXFRW filter; RS_Graphic graphic;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto verify = [&](const std::string &path) {
      CHECK(recordGroupValues(path, entry.first, "62") == std::vector<std::string>{std::to_string(index)});
      CHECK(recordGroupValues(path, entry.first, "420") == std::vector<std::string>{std::to_string(rgb)});
      const auto materials = recordGroupValues(path, "MATERIAL", "5"); REQUIRE(materials.size() == 1u);
      CHECK(recordGroupValues(path, entry.first, "347") == std::vector<std::string>{materials.front()});
    };
    verify(output);
    RS_Graphic reopened;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
    verify(resaved);
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_SPARSE_TRUECOLOR"))
      std::filesystem::copy_file(resaved, std::string(prefix) + entry.first + "-" + std::to_string(rgb) + ".dxf",
                                 std::filesystem::copy_options::overwrite_existing);
    RS_Entity *entity = nullptr;
    for (RS_Entity *candidate : graphic)
      if (candidate && candidate->sourceHandle() == 0xA5u) entity = candidate;
    REQUIRE(entity);
    const auto savedPen = entity->getPen(false);
    auto editedPen = savedPen;
    const RS_Color editedColor(21, 33, 44);
    int editedRgb = -1; const int editedIndex = LC_ColorNumbers::colorToNumber(editedColor, &editedRgb);
    REQUIRE(editedRgb == ((21 << 16) | (33 << 8) | 44));
    editedPen.setColor(editedColor); entity->setPen(editedPen);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(recordGroupValues(output, entry.first, "62") == std::vector<std::string>{std::to_string(editedIndex)});
    CHECK(recordGroupValues(output, entry.first, "420") == std::vector<std::string>{std::to_string(editedRgb)});
    RS_Graphic editedReopened;
    REQUIRE(filter.fileImport(editedReopened, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(editedReopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
    CHECK(recordGroupValues(resaved, entry.first, "62") == std::vector<std::string>{std::to_string(editedIndex)});
    CHECK(recordGroupValues(resaved, entry.first, "420") == std::vector<std::string>{std::to_string(editedRgb)});
    { const auto materials = recordGroupValues(resaved, "MATERIAL", "5"); REQUIRE(materials.size() == 1u);
      CHECK(recordGroupValues(resaved, entry.first, "347") == std::vector<std::string>{materials.front()}); }
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_SPARSE_TRUECOLOR_EDITED"))
      std::filesystem::copy_file(resaved, std::string(prefix) + entry.first + "-" + std::to_string(rgb) + ".dxf",
                                 std::filesystem::copy_options::overwrite_existing);
    entity->setPen(savedPen);
    auto invalid = input;
    const auto pos = invalid.find("62\n" + std::to_string(index) + "\n420\n"); REQUIRE(pos != std::string::npos);
    invalid.replace(pos, std::string("62\n" + std::to_string(index) + "\n").size(),
                    "62\n" + std::to_string(index == 1 ? 2 : 1) + "\n");
    writeText(source, invalid); writeText(output, sentinel);
    RS_Graphic rejected;
    if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    if (entry.first == "POINT" && color.red() == 10) {
      const auto colorFields = "62\n" + std::to_string(index) + "\n420\n" + std::to_string(rgb) + "\n";
      for (const std::string &replacement : {std::string("420\n") + std::to_string(rgb) + "\n",
             colorFields + "420\n" + std::to_string(rgb) + "\n",
             std::string("62\n1\n420\n16711680\n"),
             std::string("62\n1\n420\n-1\n"), std::string("62\n1\n420\n16777216\n"),
             std::string("62\n1\n420\n123junk\n"), colorFields + "430\nNamedColor\n"}) {
        auto bad = input; const auto where = bad.find(colorFields); REQUIRE(where != std::string::npos);
        bad.replace(where, colorFields.size(), replacement); writeText(source, bad); writeText(output, sentinel);
        RS_Graphic refused;
        if (filter.fileImport(refused, QString::fromStdString(source), RS2::FormatDXFRW))
          CHECK_FALSE(filter.fileExport(refused, QString::fromStdString(output), RS2::FormatDXFRW));
        { std::ifstream retained(output, std::ios::binary);
          CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
      }
      struct ColorProbe : RS_FilterDXFRW {
        bool alterTyped = false, alterPhysical = false;
        void addPoint(const DRW_Point &value) override {
          auto changed = value; if (alterTyped) changed.color24 ^= 1;
          RS_FilterDXFRW::addPoint(changed);
        }
        void addDxfEntitiesSource(const DRW_DxfEntitiesSource &value) override {
          auto changed = value;
          if (alterPhysical) for (auto &record : changed.records)
            if (record.name == "POINT") for (std::size_t i = 0; i < record.groups.size(); ++i)
              if (record.groups[i].code() == 420) record.rawValues[i] = "12345";
          RS_FilterDXFRW::addDxfEntitiesSource(changed);
        }
      } probe;
      writeText(source, input);
      for (int mode = 0; mode != 2; ++mode) {
        probe.alterTyped = mode == 0; probe.alterPhysical = mode == 1;
        RS_Graphic refused; REQUIRE(probe.fileImport(refused, QString::fromStdString(source), RS2::FormatDXFRW));
        writeText(output, sentinel);
        CHECK_FALSE(probe.fileExport(refused, QString::fromStdString(output), RS2::FormatDXFRW));
        { std::ifstream retained(output, std::ios::binary);
          CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
        probe.alterTyped = probe.alterPhysical = false;
        RS_Graphic retry; REQUIRE(probe.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
        REQUIRE(probe.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
      }
    }
  }
  std::filesystem::remove(source); std::filesystem::remove(output); std::filesystem::remove(resaved);
}

static std::string sparseMaterialEllipseSource(bool full) {
    auto input = sparseMaterialPolylineSource();
    const std::string old = "0\nLWPOLYLINE\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n347\nA1\n"
        "100\nAcDbPolyline\n90\n3\n70\n0\n10\n1\n20\n2\n10\n4\n20\n6\n10\n8\n20\n3\n";
    const std::string replacement = "0\nELLIPSE\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n347\nA1\n"
        "100\nAcDbEllipse\n10\n1\n20\n2\n30\n0\n11\n4\n21\n3\n31\n0\n40\n0.5\n41\n" +
        std::string(full ? "0\n42\n6.283185307179586\n" : "0.25\n42\n2.5\n");
    const auto at = input.find(old); REQUIRE(at != std::string::npos);
    input.replace(at, old.size(), replacement);
    return input;
}

TEST_CASE("Ordinary sparse material saves planar ellipses",
          "[system-codec][dxf][material][transaction][ellipse]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_ellipse_source.dxf");
  const auto output = tmpFile("system_sparse_ellipse_output.dxf");
  const auto resaved = tmpFile("system_sparse_ellipse_resaved.dxf");
  for (const int shape : {0, 1, 2}) {
    const bool full = shape == 1;
    auto input = sparseMaterialEllipseSource(full);
    if (shape == 2) {
      const auto at = input.find("41\n0.25\n42\n2.5\n"); REQUIRE(at != std::string::npos);
      input.replace(at, std::string("41\n0.25\n42\n2.5\n").size(), "41\n5\n42\n1\n");
    }
    writeText(source, input);
    RS_FilterDXFRW filter; RS_Graphic graphic;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    RS_Ellipse *ellipse = nullptr;
    for (RS_Entity *entity : graphic)
      if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityEllipse)
        ellipse = static_cast<RS_Ellipse *>(entity);
    REQUIRE(ellipse); ellipse->move(RS_Vector(10, 20));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    RS_Graphic reread;
    REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(reread, QString::fromStdString(resaved), RS2::FormatDXFRW));
    CHECK(recordGroupValues(resaved, "ELLIPSE", "10") == std::vector<std::string>{"11"});
    CHECK(recordGroupValues(resaved, "ELLIPSE", "20") == std::vector<std::string>{"22"});
    CHECK(recordGroupValues(resaved, "ELLIPSE", "11") == std::vector<std::string>{"4"});
    CHECK(recordGroupValues(resaved, "ELLIPSE", "21") == std::vector<std::string>{"3"});
    CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "40").at(0)) == Catch::Approx(0.5));
    CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "41").at(0)) == Catch::Approx(full ? 0.0 : shape == 2 ? 5.0 : 0.25));
    CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "42").at(0)) == Catch::Approx(full ? 2.0 * M_PI : shape == 2 ? 1.0 : 2.5));
    const auto material = recordGroupValues(resaved, "MATERIAL", "5"); REQUIRE(material.size() == 1u);
    CHECK(recordGroupValues(resaved, "ELLIPSE", "347") == material);
    if (const char *copy = std::getenv("LIBRECAD_KEEP_SPARSE_ELLIPSE"))
      std::filesystem::copy_file(resaved, std::string(copy) + (full ? "-full.dxf" : shape == 2 ? "-wrapped.dxf" : "-partial.dxf"),
                                 std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(source); std::filesystem::remove(output); std::filesystem::remove(resaved);
}

TEST_CASE("Nearly full material ellipses preserve their interval",
          "[system-codec][dxf][material][transaction][ellipse][near-full]") {
  ensureSettings();
  auto input = sparseMaterialEllipseSource(false);
  const std::string old = "41\n0.25\n42\n2.5\n";
  const auto at = input.find(old); REQUIRE(at != std::string::npos);
  input.replace(at, old.size(), "41\n0.00000000001\n42\n6.283185307169586\n");
  const auto source = tmpFile("system_sparse_ellipse_near_full_source.dxf");
  const auto output = tmpFile("system_sparse_ellipse_near_full_output.dxf");
  const auto resaved = tmpFile("system_sparse_ellipse_near_full_resaved.dxf");
  writeText(source, input);
  RS_FilterDXFRW filter; RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  RS_Ellipse *ellipse = nullptr;
  for (RS_Entity *entity : graphic)
    if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityEllipse)
      ellipse = static_cast<RS_Ellipse *>(entity);
  REQUIRE(ellipse);
  REQUIRE(ellipse->isEllipticArc());
  CHECK(ellipse->getAngle1() == 1.0e-11);
  CHECK(ellipse->getAngle2() == 2.0 * M_PI - 1.0e-11);
  CHECK(ellipse->getAngleLength() < 2.0 * M_PI);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "41").at(0)) == 1.0e-11);
  CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "42").at(0)) == 2.0 * M_PI - 1.0e-11);
  ellipse->setAngle1(2.0e-11);
  ellipse->setAngle2(2.0 * M_PI - 2.0e-11);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  RS_Graphic edited;
  REQUIRE(filter.fileImport(edited, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(edited, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "41").at(0)) == 2.0e-11);
  CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "42").at(0)) == 2.0 * M_PI - 2.0e-11);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_NEAR_FULL_ELLIPSE"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  const std::string nearlyFull = "41\n0.00000000001\n42\n6.283185307169586\n";
  const auto tinyAt = input.find(nearlyFull); REQUIRE(tinyAt != std::string::npos);
  input.replace(tinyAt, nearlyFull.size(), "41\n0\n42\n0.00000000001\n");
  writeText(source, input);
  RS_Graphic tiny;
  REQUIRE(filter.fileImport(tiny, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(tiny, QString::fromStdString(output), RS2::FormatDXFRW));
  RS_Graphic tinyReopened;
  REQUIRE(filter.fileImport(tinyReopened, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(tinyReopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "41").at(0)) == 0.0);
  CHECK(std::stod(recordGroupValues(resaved, "ELLIPSE", "42").at(0)) == 1.0e-11);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_TINY_ELLIPSE"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(source); std::filesystem::remove(output); std::filesystem::remove(resaved);
}

TEST_CASE("Ellipse sweep distinguishes tiny gaps and tiny arcs from full turns",
          "[system-codec][dxf][ellipse][near-full]") {
  ensureSettings();
  const RS_Vector center(1, 2), major(4, 3);
  const RS_Ellipse full(nullptr, {center, major, 0.5, 0.0, 0.0, false});
  const RS_Ellipse nearFull(nullptr, {center, major, 0.5, 1.0e-11, 2.0 * M_PI - 1.0e-11, false});
  const RS_Ellipse tiny(nullptr, {center, major, 0.5, 1.0, 1.0 + 1.0e-9, false});
  const RS_Ellipse tinyAtZero(nullptr, {center, major, 0.5, 0.0, 1.0e-11, false});
  const RS_Ellipse reversedTiny(nullptr, {center, major, 0.5, 1.0e-11, 0.0, true});
  const RS_Ellipse wrapped(nullptr, {center, major, 0.5, 5.0, 1.0, false});
  CHECK(full.getAngleLength() == 2.0 * M_PI);
  CHECK(nearFull.getAngleLength() < 2.0 * M_PI);
  CHECK(nearFull.getAngleLength() > 2.0 * M_PI - 1.0e-9);
  CHECK(tiny.getAngleLength() > 0.0);
  CHECK(tiny.getAngleLength() < 2.0e-9);
  CHECK(tinyAtZero.isEllipticArc());
  CHECK(tinyAtZero.getAngleLength() > 0.0);
  CHECK(tinyAtZero.getAngleLength() < 2.0e-11);
  CHECK(tinyAtZero.getLength() > 0.0);
  CHECK(tinyAtZero.getLength() < 1.0e-9);
  CHECK(tinyAtZero.getLength() == Catch::Approx(2.5e-11).epsilon(1.0e-4).margin(1.0e-14));
  CHECK(tinyAtZero.getData().angularLength < 1.0e-6);
  CHECK(tinyAtZero.getMax().x - tinyAtZero.getMin().x < 1.0e-8);
  CHECK(tinyAtZero.getMax().y - tinyAtZero.getMin().y < 1.0e-8);
  const auto nearest = tinyAtZero.getNearestPointOnEntity(center - major, true);
  CHECK(nearest.distanceTo(tinyAtZero.getStartpoint()) < 1.0e-8);
  CHECK_FALSE(tinyAtZero.isPointOnEntity(center - major));
  CHECK(tinyAtZero.isPointOnEntity(tinyAtZero.getStartpoint()));
  CHECK(tinyAtZero.isPointOnEntity(tinyAtZero.getEndpoint()));
  CHECK(reversedTiny.getLength() < 1.0e-9);
  CHECK(reversedTiny.getMax().x - reversedTiny.getMin().x < 1.0e-8);
  CHECK(reversedTiny.getNearestPointOnEntity(center - major, true).distanceTo(reversedTiny.getStartpoint()) < 1.0e-8);
  CHECK(wrapped.getAngleLength() == Catch::Approx(2.0 * M_PI - 4.0));
}

TEST_CASE("Nearly full ellipse round trips without material metadata",
          "[system-codec][dxf][ellipse][near-full]") {
  ensureSettings();
  const auto source = tmpFile("plain_near_full_ellipse_source.dxf");
  const auto output = tmpFile("plain_near_full_ellipse_output.dxf");
  writeText(source, "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\nELLIPSE\n5\nA5\n100\nAcDbEntity\n8\n0\n"
      "100\nAcDbEllipse\n10\n1\n20\n2\n30\n0\n11\n4\n21\n3\n31\n0\n40\n0.5\n"
      "41\n0.00000000001\n42\n6.283185307169586\n0\nENDSEC\n0\nEOF\n");
  RS_FilterDXFRW filter; RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(std::stod(recordGroupValues(output, "ELLIPSE", "41").at(0)) == 1.0e-11);
  CHECK(std::stod(recordGroupValues(output, "ELLIPSE", "42").at(0)) == 2.0 * M_PI - 1.0e-11);
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto *entity = reopened.firstEntity();
  REQUIRE(entity); REQUIRE(entity->rtti() == RS2::EntityEllipse);
  const auto *ellipse = static_cast<const RS_Ellipse *>(entity);
  CHECK(ellipse->isEllipticArc());
  CHECK(ellipse->getAngleLength() < 2.0 * M_PI);
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("Sparse material ellipse rejects incomplete or altered delivery and retries",
          "[system-codec][dxf][material][transaction][ellipse]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_ellipse_reject_source.dxf");
  const auto output = tmpFile("system_sparse_ellipse_reject_output.dxf");
  const auto input = sparseMaterialEllipseSource(false);
  const std::string sentinel = "preserve sparse ellipse destination\n";
  struct EllipseProbe : RS_FilterDXFRW {
    int mode = 0;
    void addEllipse(const DRW_Ellipse &data) override {
      if (mode == 1) return;
      DRW_Ellipse changed = data;
      if (mode == 2) changed.secPoint.x += 1.0;
      RS_FilterDXFRW::addEllipse(changed);
    }
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &data) override {
      auto copy = data;
      if (mode == 3) for (auto &record : copy.records)
        if (record.name == "ELLIPSE") for (std::size_t i = 0; i < record.groups.size(); ++i)
          if (record.groups[i].code() == 11) { record.rawValues[i] = "5"; break; }
      RS_FilterDXFRW::addDxfEntitiesSource(copy);
    }
  } filter;
  writeText(source, input);
  for (int mode = 1; mode <= 3; ++mode) {
    filter.mode = mode;
    RS_Graphic bad;
    REQUIRE(filter.fileImport(bad, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(bad, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    filter.mode = 0;
    RS_Graphic retry;
    REQUIRE(filter.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  for (const auto &change : std::vector<std::pair<std::string, std::string>>{
           {"31\n0\n", "31\n1\n"}, {"40\n0.5\n", "40\n2\n"},
           {"42\n2.5\n", "42\n0.25\n"}, {"40\n0.5\n", "210\n0\n220\n1\n230\n0\n40\n0.5\n"}}) {
    auto badInput = input;
    const auto at = badInput.find(change.first); REQUIRE(at != std::string::npos);
    badInput.replace(at, change.first.size(), change.second);
    writeText(source, badInput); writeText(output, sentinel);
    RS_Graphic bad;
    if (filter.fileImport(bad, QString::fromStdString(source), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(bad, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    writeText(source, input);
    RS_Graphic retry;
    REQUIRE(filter.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  writeText(source, input);
  RS_Graphic live;
  REQUIRE(filter.fileImport(live, QString::fromStdString(source), RS2::FormatDXFRW));
  RS_Ellipse *ellipse = nullptr;
  for (RS_Entity *entity : live)
    if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityEllipse)
      ellipse = static_cast<RS_Ellipse *>(entity);
  REQUIRE(ellipse);
  for (const int mode : {0, 1}) {
    if (mode == 0) ellipse->setRatio(2.0);
    else ellipse->setReversed(true);
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(live, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    if (mode == 0) ellipse->setRatio(0.5);
    else ellipse->setReversed(false);
    REQUIRE(filter.fileExport(live, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("Ordinary sparse material saves straight lightweight polylines",
          "[system-codec][dxf][material][transaction][polyline]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_lwpolyline_source.dxf");
  const auto output = tmpFile("system_sparse_lwpolyline_output.dxf");
  const std::string input = sparseMaterialPolylineSource();
  writeText(source, input);
  RS_FilterDXFRW filter; RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(countRecords(output, "LWPOLYLINE") == 1);
  CHECK(recordGroupValues(output, "LWPOLYLINE", "10") == std::vector<std::string>{"1", "4", "8"});
  CHECK(recordGroupValues(output, "LWPOLYLINE", "20") == std::vector<std::string>{"2", "6", "3"});
  const auto material = recordGroupValues(output, "MATERIAL", "5"); REQUIRE(material.size() == 1u);
  CHECK(recordGroupValues(output, "LWPOLYLINE", "347") == std::vector<std::string>{material.front()});
  RS_Polyline *polyline = nullptr;
  for (RS_Entity *entity : graphic)
    if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityPolyline)
      polyline = static_cast<RS_Polyline *>(entity);
  REQUIRE(polyline);
  polyline->move(RS_Vector(10, 20));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "LWPOLYLINE", "10") == std::vector<std::string>{"11", "14", "18"});
  CHECK(recordGroupValues(output, "LWPOLYLINE", "20") == std::vector<std::string>{"22", "26", "23"});
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto resaved = tmpFile("system_sparse_lwpolyline_resaved.dxf");
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(recordGroupValues(resaved, "LWPOLYLINE", "10") == std::vector<std::string>{"11", "14", "18"});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_LWPOLYLINE"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);

  auto closedInput = input;
  const auto closedFlag = closedInput.find("90\n3\n70\n0\n"); REQUIRE(closedFlag != std::string::npos);
  closedInput.replace(closedFlag, std::string("90\n3\n70\n0\n").size(), "90\n3\n70\n1\n");
  writeText(source, closedInput);
  RS_Graphic closed;
  REQUIRE(filter.fileImport(closed, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(closed, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "LWPOLYLINE", "70") == std::vector<std::string>{"1"});
  CHECK(recordGroupValues(output, "LWPOLYLINE", "10") == std::vector<std::string>{"1", "4", "8"});
  RS_Polyline *closedPolyline = nullptr;
  for (RS_Entity *entity : closed)
    if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityPolyline)
      closedPolyline = static_cast<RS_Polyline *>(entity);
  REQUIRE(closedPolyline); REQUIRE(closedPolyline->isClosed()); REQUIRE(closedPolyline->count() == 3u);
  closedPolyline->move(RS_Vector(10, 20));
  REQUIRE(filter.fileExport(closed, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "LWPOLYLINE", "10") == std::vector<std::string>{"11", "14", "18"});
  CHECK(recordGroupValues(output, "LWPOLYLINE", "20") == std::vector<std::string>{"22", "26", "23"});
  RS_Graphic closedReopened;
  REQUIRE(filter.fileImport(closedReopened, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(closedReopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(recordGroupValues(resaved, "LWPOLYLINE", "70") == std::vector<std::string>{"1"});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_CLOSED_SPARSE_LWPOLYLINE"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);

  const std::string sentinel = "preserve sparse polyline destination\n";
  auto explicitZeros = input;
  const std::string firstVertex = "20\n2\n10\n4\n";
  const auto zeroVertex = explicitZeros.find(firstVertex); REQUIRE(zeroVertex != std::string::npos);
  explicitZeros.replace(zeroVertex, firstVertex.size(), "20\n2\n40\n0\n41\n0\n42\n0\n10\n4\n");
  const auto zeroHeader = explicitZeros.find("90\n3\n70\n0\n"); REQUIRE(zeroHeader != std::string::npos);
  explicitZeros.insert(zeroHeader + std::string("90\n3\n70\n0\n").size(), "38\n0\n39\n0\n");
  writeText(source, explicitZeros);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ZERO_SPARSE_LWPOLYLINE"))
    std::filesystem::copy_file(source, std::string(copy) + ".source.dxf", std::filesystem::copy_options::overwrite_existing);
  RS_Graphic zeros;
  REQUIRE(filter.fileImport(zeros, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(zeros, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "LWPOLYLINE", "10") == std::vector<std::string>{"1", "4", "8"});
  RS_Graphic zerosReopened;
  REQUIRE(filter.fileImport(zerosReopened, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(zerosReopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ZERO_SPARSE_LWPOLYLINE"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  auto closedZeros = explicitZeros;
  const auto zeroClosedFlag = closedZeros.find("90\n3\n70\n0\n"); REQUIRE(zeroClosedFlag != std::string::npos);
  closedZeros.replace(zeroClosedFlag, std::string("90\n3\n70\n0\n").size(), "90\n3\n70\n1\n");
  writeText(source, closedZeros);
  RS_Graphic zerosClosed;
  REQUIRE(filter.fileImport(zerosClosed, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(zerosClosed, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "LWPOLYLINE", "70") == std::vector<std::string>{"1"});
  RS_Graphic zerosClosedReopened;
  REQUIRE(filter.fileImport(zerosClosedReopened, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(zerosClosedReopened, QString::fromStdString(resaved), RS2::FormatDXFRW));

  for (const std::string addition : {"43\n0\n", "42\n0\n"}) {
    auto conflict = explicitZeros;
    const auto at = conflict.find(addition == "43\n0\n" ? "38\n0\n" : "42\n0\n");
    REQUIRE(at != std::string::npos);
    conflict.insert(at, addition);
    writeText(source, conflict); writeText(output, sentinel);
    RS_Graphic rejected;
    if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    REQUIRE(filter.fileExport(zeros, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  for (int mode = 0; mode != 5; ++mode) {
    if (mode == 0) polyline->setPlotStyleHandle(0xFAu);
    if (mode == 1) polyline->setShadowHandle(0xFAu);
    if (mode == 2) polyline->setVisualStyleHandles(0xFAu, 0, 0);
    if (mode == 3) polyline->setXDictHandle(0xFAu);
    if (mode == 4) polyline->setReactorHandles({0xFAu});
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    polyline->setPlotStyleHandle(0); polyline->setShadowHandle(0);
    polyline->setVisualStyleHandles(0, 0, 0); polyline->setXDictHandle(0);
    polyline->setReactorHandles({});
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  RS_Entity *closingSegment = closedPolyline->entityAt(closedPolyline->count() - 1u); REQUIRE(closingSegment);
  closingSegment->move(RS_Vector(1, 1));
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(closed, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  closingSegment->move(RS_Vector(-1, -1));
  REQUIRE(filter.fileExport(closed, QString::fromStdString(output), RS2::FormatDXFRW));
  for (const std::string last : {"10\n1\n20\n2\n", "10\n1.000001\n20\n2\n"}) {
    auto badClosed = closedInput;
    const std::string original = "10\n8\n20\n3\n";
    const auto at = badClosed.find(original); REQUIRE(at != std::string::npos);
    badClosed.replace(at, original.size(), last);
    writeText(source, badClosed); writeText(output, sentinel);
    RS_Graphic rejected;
    if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    REQUIRE(filter.fileExport(closed, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  RS_Entity *segment = polyline->entityAt(0); REQUIRE(segment);
  segment->setMaterialHandle(0xA1u);
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  segment->setMaterialHandle(0);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const std::vector<std::pair<std::string, std::string>> unsupported = {
      {"90\n3\n70\n0\n", "90\n3\n70\n128\n"},
      {"20\n2\n10\n4\n", "20\n2\n42\n0.000000000001\n10\n4\n"},
      {"90\n3\n70\n0\n", "90\n3\n70\n0\n38\n2\n"},
      {"90\n3\n70\n0\n", "90\n3\n70\n0\n43\n1\n"},
      {"20\n2\n10\n4\n", "20\n2\n40\n1\n10\n4\n"},
      {"90\n3\n70\n0\n", "90\n4\n70\n0\n"}};
  for (const auto &change : unsupported) {
    auto bad = input;
    const auto at = bad.find(change.first); REQUIRE(at != std::string::npos);
    bad.replace(at, change.first.size(), change.second);
    writeText(source, bad); writeText(output, sentinel);
    RS_Graphic rejected;
    if (filter.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  }

  const std::string originalVertices = "90\n3\n70\n0\n10\n1\n20\n2\n10\n4\n20\n6\n10\n8\n20\n3\n";
  for (const int flag : {0, 1}) for (const int count : {128, 129}) {
    std::string geometry = "90\n" + std::to_string(count) + "\n70\n" + std::to_string(flag) + "\n";
    for (int i = 0; i < count; ++i)
      geometry += "10\n" + std::to_string(i) + "\n20\n" + std::to_string(i % 7) + "\n";
    auto bounded = input;
    const auto at = bounded.find(originalVertices); REQUIRE(at != std::string::npos);
    bounded.replace(at, originalVertices.size(), geometry);
    writeText(source, bounded); writeText(output, sentinel);
    RS_Graphic candidate;
    REQUIRE(filter.fileImport(candidate, QString::fromStdString(source), RS2::FormatDXFRW));
    if (count == 128) {
      REQUIRE(filter.fileExport(candidate, QString::fromStdString(output), RS2::FormatDXFRW));
      CHECK(recordGroupValues(output, "LWPOLYLINE", "10").size() == 128u);
      if (flag == 1) {
        RS_Graphic reread;
        REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
        REQUIRE(filter.fileExport(reread, QString::fromStdString(resaved), RS2::FormatDXFRW));
        CHECK(recordGroupValues(resaved, "LWPOLYLINE", "70") == std::vector<std::string>{"1"});
        CHECK(recordGroupValues(resaved, "LWPOLYLINE", "10").size() == 128u);
        if (const char *copy = std::getenv("LIBRECAD_KEEP_CLOSED_SPARSE_LWPOLYLINE_128"))
          std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
      }
    } else {
      CHECK_FALSE(filter.fileExport(candidate, QString::fromStdString(output), RS2::FormatDXFRW));
      { std::ifstream retained(output, std::ios::binary);
        CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    }
  }

  auto two = input;
  const auto boundary = two.find("0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n");
  REQUIRE(boundary != std::string::npos);
  two.insert(boundary, "0\nLWPOLYLINE\n5\nA6\n100\nAcDbEntity\n8\nRAWTEST\n"
                       "100\nAcDbPolyline\n90\n2\n70\n0\n10\n2\n20\n4\n10\n3\n20\n5\n");
  struct PolylineDeliveryProbe : RS_FilterDXFRW {
    std::uint32_t suppress = 0;
    bool alterTyped = false, alterPhysical = false;
    void addLWPolyline(const DRW_LWPolyline &data) override {
      if (data.handle == suppress) return;
      DRW_LWPolyline changed = data;
      if (alterTyped && data.handle == 0xA5u && !changed.vertlist.empty()) {
        changed.vertlist[0] = std::make_shared<DRW_Vertex2D>(*changed.vertlist[0]);
        changed.vertlist[0]->x += 1.0;
      }
      RS_FilterDXFRW::addLWPolyline(changed);
    }
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &data) override {
      auto copy = data;
      if (alterPhysical) for (auto &record : copy.records)
        if (record.name == "LWPOLYLINE") for (std::size_t i = 0; i < record.groups.size(); ++i)
          if (record.groups[i].code() == 10) { record.rawValues[i] = "999"; break; }
      RS_FilterDXFRW::addDxfEntitiesSource(copy);
    }
  } probe;
  writeText(source, two);
  for (int mode = 0; mode != 4; ++mode) {
    probe.suppress = mode == 0 ? 0xA5u : mode == 1 ? 0xA6u : 0;
    probe.alterTyped = mode == 2; probe.alterPhysical = mode == 3;
    RS_Graphic rejected;
    REQUIRE(probe.fileImport(rejected, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(probe.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    probe.suppress = 0; probe.alterTyped = probe.alterPhysical = false;
    RS_Graphic retry;
    REQUIRE(probe.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(probe.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(countRecords(output, "LWPOLYLINE") == 2);
  }
  std::filesystem::remove(resaved);
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("Ordinary sparse material saves curved lightweight polylines",
          "[system-codec][dxf][material][transaction][polyline][bulge]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_curved_polyline_source.dxf");
  const auto output = tmpFile("system_sparse_curved_polyline_output.dxf");
  const auto resaved = tmpFile("system_sparse_curved_polyline_resaved.dxf");
  const std::string sentinel = "preserve curved polyline destination\n";
  for (const bool closed : {false, true}) for (const double bulge : {0.5, -0.5, 1.0, -1.0, 2.0, -2.0}) {
    CAPTURE(closed, bulge);
    auto input = sparseMaterialPolylineSource();
    const auto first = input.find("20\n2\n10\n4\n"); REQUIRE(first != std::string::npos);
    input.replace(first, std::string("20\n2\n10\n4\n").size(),
                  "20\n2\n42\n" + std::to_string(bulge) + "\n10\n4\n");
    const auto last = input.find("20\n3\n0\nENDSEC\n"); REQUIRE(last != std::string::npos);
    input.replace(last, std::string("20\n3\n0\nENDSEC\n").size(),
                  "20\n3\n42\n0.25\n0\nENDSEC\n");
    if (closed) {
      const auto flag = input.find("90\n3\n70\n0\n"); REQUIRE(flag != std::string::npos);
      input.replace(flag, std::string("90\n3\n70\n0\n").size(), "90\n3\n70\n1\n");
    }
    writeText(source, input); writeText(output, sentinel);
    RS_FilterDXFRW filter; RS_Graphic graphic;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto verify = [&](const std::string &path) {
      const auto values = recordGroupValues(path, "LWPOLYLINE", "42");
      REQUIRE(values.size() == 2u);
      CHECK(std::stod(values[0]) == Catch::Approx(bulge));
      CHECK(std::stod(values[1]) == Catch::Approx(0.25));
      const auto material = recordGroupValues(path, "MATERIAL", "5"); REQUIRE(material.size() == 1u);
      CHECK(recordGroupValues(path, "LWPOLYLINE", "347") == std::vector<std::string>{material.front()});
    };
    verify(output);
    RS_Polyline *polyline = nullptr;
    for (RS_Entity *entity : graphic)
      if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityPolyline)
        polyline = static_cast<RS_Polyline *>(entity);
    REQUIRE(polyline); polyline->move(RS_Vector(10, 20));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    verify(output);
    RS_Graphic reread;
    REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(reread, QString::fromStdString(resaved), RS2::FormatDXFRW));
    verify(resaved);
    if (const char *copy = std::getenv("LIBRECAD_KEEP_CURVED_SPARSE_LWPOLYLINE")) {
      const std::string suffix = std::string(closed ? "-closed-" : "-open-") + std::to_string(bulge) + ".dxf";
      std::filesystem::copy_file(resaved, std::string(copy) + suffix,
                                 std::filesystem::copy_options::overwrite_existing);
    }
  }
  std::filesystem::remove(source); std::filesystem::remove(output); std::filesystem::remove(resaved);
}

TEST_CASE("Curved sparse material rejects altered geometry and retries",
          "[system-codec][dxf][material][transaction][polyline][bulge]") {
  ensureSettings();
  auto input = sparseMaterialPolylineSource();
  const auto first = input.find("20\n2\n10\n4\n"); REQUIRE(first != std::string::npos);
  input.replace(first, std::string("20\n2\n10\n4\n").size(), "20\n2\n42\n0.5\n10\n4\n");
  const auto source = tmpFile("system_sparse_curved_polyline_mismatch.dxf");
  const auto output = tmpFile("system_sparse_curved_polyline_mismatch_out.dxf");
  const std::string sentinel = "preserve curved mismatch destination\n";
  struct CurvedProbe : RS_FilterDXFRW {
    bool alterTyped = false, alterPhysical = false;
    void addLWPolyline(const DRW_LWPolyline &data) override {
      DRW_LWPolyline changed = data;
      if (alterTyped && data.handle == 0xA5u && !changed.vertlist.empty()) {
        changed.vertlist[0] = std::make_shared<DRW_Vertex2D>(*changed.vertlist[0]);
        changed.vertlist[0]->bulge = 0.75;
      }
      RS_FilterDXFRW::addLWPolyline(changed);
    }
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &data) override {
      auto copy = data;
      if (alterPhysical) for (auto &record : copy.records)
        if (record.name == "LWPOLYLINE") for (std::size_t i = 0; i < record.groups.size(); ++i)
          if (record.groups[i].code() == 42) { record.rawValues[i] = "0.75"; break; }
      RS_FilterDXFRW::addDxfEntitiesSource(copy);
    }
  } filter;
  writeText(source, input);
  for (int mode = 0; mode != 2; ++mode) {
    filter.alterTyped = mode == 0; filter.alterPhysical = mode == 1;
    RS_Graphic changed;
    REQUIRE(filter.fileImport(changed, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(changed, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    filter.alterTyped = filter.alterPhysical = false;
    RS_Graphic retry;
    REQUIRE(filter.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  RS_Graphic live;
  REQUIRE(filter.fileImport(live, QString::fromStdString(source), RS2::FormatDXFRW));
  RS_Polyline *polyline = nullptr;
  for (RS_Entity *entity : live)
    if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityPolyline)
      polyline = static_cast<RS_Polyline *>(entity);
  REQUIRE(polyline); REQUIRE(polyline->count() == 2u);
  auto *arc = polyline->entityAt(0); REQUIRE(arc); REQUIRE(arc->rtti() == RS2::EntityArc);
  arc->move(RS_Vector(1, 1));
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(live, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  arc->move(RS_Vector(-1, -1));
  REQUIRE(filter.fileExport(live, QString::fromStdString(output), RS2::FormatDXFRW));
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("Ordinary sparse material saves simple circles arcs and lines",
          "[system-codec][dxf][material][transaction][circle][arc]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_circle_source.dxf");
  const auto output = tmpFile("system_sparse_circle_output.dxf");
  const auto resaved = tmpFile("system_sparse_circle_resave.dxf");
  const std::string input =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$CMATERIAL\n347\nA1\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\nA1\n0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\nLINE\n5\nA2\n100\nAcDbEntity\n8\nRAWTEST\n62\n1\n347\nA1\n"
      "100\nAcDbLine\n10\n0\n20\n0\n11\n10\n21\n0\n"
      "0\nCIRCLE\n5\nA5\n100\nAcDbEntity\n8\nRAWTEST\n62\n2\n347\nA1\n"
      "100\nAcDbCircle\n10\n2\n20\n3\n30\n0\n40\n4\n"
      "0\nARC\n5\nA6\n100\nAcDbEntity\n8\nRAWTEST\n62\n3\n347\nA1\n"
      "100\nAcDbCircle\n10\n4\n20\n5\n40\n2\n100\nAcDbArc\n50\n30\n51\n120\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
      "100\nAcDbDictionary\n281\n1\n3\nACAD_MATERIAL\n350\nA0\n"
      "0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n"
      "100\nAcDbMaterial\n1\nPlainMat\n94\n63\n0\nENDSEC\n0\nEOF\n";
  writeText(source, input);
  RS_FilterDXFRW filter;
  RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  RS_Circle *circle = nullptr;
  RS_Arc *arc = nullptr;
  for (RS_Entity *entity : graphic) {
    if (entity && entity->sourceHandle() == 0xA5u && entity->rtti() == RS2::EntityCircle)
      circle = static_cast<RS_Circle *>(entity);
    if (entity && entity->sourceHandle() == 0xA6u && entity->rtti() == RS2::EntityArc)
      arc = static_cast<RS_Arc *>(entity);
  }
  REQUIRE(circle); REQUIRE(arc);
  CHECK(circle->materialHandle() == 0xA1u);
  CHECK(arc->materialHandle() == 0xA1u);
  circle->setCenter(RS_Vector(5.0, 6.0));
  circle->setRadius(7.0);
  arc->setCenter(RS_Vector(8.0, 9.0));
  arc->setRadius(3.5);
  arc->setAngle1(M_PI / 4.0);
  arc->setAngle2(M_PI);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "CIRCLE", "10") == std::vector<std::string>{"5"});
  CHECK(recordGroupValues(output, "CIRCLE", "20") == std::vector<std::string>{"6"});
  CHECK(recordGroupValues(output, "CIRCLE", "40") == std::vector<std::string>{"7"});
  CHECK(recordGroupValues(output, "ARC", "10") == std::vector<std::string>{"8"});
  CHECK(recordGroupValues(output, "ARC", "20") == std::vector<std::string>{"9"});
  CHECK(recordGroupValues(output, "ARC", "40") == std::vector<std::string>{"3.5"});
  CHECK(recordGroupValues(output, "LINE", "62") == std::vector<std::string>{"1"});
  CHECK(recordGroupValues(output, "CIRCLE", "62") == std::vector<std::string>{"2"});
  CHECK(recordGroupValues(output, "ARC", "62") == std::vector<std::string>{"3"});
  const auto start = recordGroupValues(output, "ARC", "50");
  const auto end = recordGroupValues(output, "ARC", "51");
  REQUIRE(start.size() == 1u); REQUIRE(end.size() == 1u);
  CHECK(std::abs(std::stod(start.front()) - 45.0) < 1e-10);
  CHECK(std::abs(std::stod(end.front()) - 180.0) < 1e-10);
  const auto material = recordGroupValues(output, "MATERIAL", "5");
  REQUIRE(material.size() == 1u);
  CHECK(recordGroupValues(output, "CIRCLE", "347") == std::vector<std::string>{material.front()});
  CHECK(recordGroupValues(output, "ARC", "347") == std::vector<std::string>{material.front()});
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(recordGroupValues(resaved, "CIRCLE", "40") == std::vector<std::string>{"7"});
  CHECK(recordGroupValues(resaved, "ARC", "40") == std::vector<std::string>{"3.5"});
  CHECK(recordGroupValues(resaved, "LINE", "62") == std::vector<std::string>{"1"});
  CHECK(recordGroupValues(resaved, "CIRCLE", "62") == std::vector<std::string>{"2"});
  CHECK(recordGroupValues(resaved, "ARC", "62") == std::vector<std::string>{"3"});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_CIRCLE"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  const auto colorSource = tmpFile("system_sparse_circle_colors.dxf");
  for (int color : {1, 2, 3, 4, 5, 6, 8, 9, 11, 20, 40, 89, 120, 149, 199, 249, 250, 254, 255}) {
    auto colored = input;
    const auto at = colored.find("62\n2\n347\nA1\n100\nAcDbCircle\n");
    REQUIRE(at != std::string::npos);
    colored.replace(at, std::string("62\n2\n").size(), "62\n" + std::to_string(color) + "\n");
    writeText(colorSource, colored);
    RS_Graphic colorGraphic;
    REQUIRE(filter.fileImport(colorGraphic, QString::fromStdString(colorSource), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(colorGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(recordGroupValues(output, "CIRCLE", "62") == std::vector<std::string>{std::to_string(color)});
    RS_Graphic colorReopened;
    REQUIRE(filter.fileImport(colorReopened, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(colorReopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
    CHECK(recordGroupValues(resaved, "CIRCLE", "62") == std::vector<std::string>{std::to_string(color)});
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_PALETTE"))
      std::filesystem::copy_file(resaved, std::string(prefix) + std::to_string(color) + ".dxf",
                                 std::filesystem::copy_options::overwrite_existing);
  }
  const std::string colorSentinel = "preserve unsupported color destination\n";
  for (const std::string extra : {"62\n7\n", "62\n-1\n", "62\n10\n", "62\n50\n", "62\n90\n",
                                  "62\n130\n", "62\n170\n", "62\n210\n", "62\n2\n62\n2\n",
                                  "62\n2\n420\n16711680\n"}) {
    auto unsupported = input;
    const auto at = unsupported.find("62\n2\n347\nA1\n100\nAcDbCircle\n");
    REQUIRE(at != std::string::npos);
    unsupported.replace(at, std::string("62\n2\n").size(), extra);
    writeText(colorSource, unsupported);
    RS_Graphic rejected;
    writeText(output, colorSentinel);
    if (filter.fileImport(rejected, QString::fromStdString(colorSource), RS2::FormatDXFRW))
      CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == colorSentinel); }
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  {
    // Exercise the complete canonical palette together without hundreds of
    // individual file conversions. The six aliases and contextual ACI 7 are
    // tested as refusals above.
    const std::set<int> excluded{7, 10, 50, 90, 130, 170, 210};
    std::vector<std::string> expected;
    std::string circles;
    for (int color = 1; color <= 255; ++color) {
      if (excluded.count(color)) continue;
      expected.push_back(std::to_string(color));
      circles += "0\nCIRCLE\n5\n" + QString::number(0x100 + color, 16).toUpper().toStdString() +
          "\n100\nAcDbEntity\n8\nRAWTEST\n62\n" + std::to_string(color) +
          "\n347\nA1\n100\nAcDbCircle\n10\n" + std::to_string(color) + "\n20\n3\n40\n4\n";
    }
    REQUIRE(expected.size() == 248u);
    auto palette = input;
    const auto first = palette.find("0\nCIRCLE\n5\nA5\n");
    const auto last = palette.find("0\nARC\n5\nA6\n", first);
    REQUIRE(first != std::string::npos); REQUIRE(last != std::string::npos);
    palette.replace(first, last - first, circles);
    writeText(colorSource, palette);
    RS_Graphic paletteGraphic;
    REQUIRE(filter.fileImport(paletteGraphic, QString::fromStdString(colorSource), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(paletteGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(recordGroupValues(output, "CIRCLE", "62") == expected);
    RS_Graphic paletteReopened;
    REQUIRE(filter.fileImport(paletteReopened, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(paletteReopened, QString::fromStdString(resaved), RS2::FormatDXFRW));
    CHECK(recordGroupValues(resaved, "CIRCLE", "62") == expected);
    if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_FULL_PALETTE"))
      std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  }
  auto savedPen = circle->getPen(false);
  auto changedPen = savedPen;
  changedPen.setColor(RS_Color(0, 255, 255));
  circle->setPen(changedPen);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(recordGroupValues(output, "CIRCLE", "62") == std::vector<std::string>{"4"});
  RS_Graphic changedColor;
  REQUIRE(filter.fileImport(changedColor, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(changedColor, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(recordGroupValues(resaved, "CIRCLE", "62") == std::vector<std::string>{"4"});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_EDITED_COLOR"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  RS_Color namedColor(10, 20, 30);
  namedColor.setColorName(QStringLiteral("unqualified named color"));
  for (const RS_Color &color : {RS_Color(0, 0, 0), namedColor}) {
    changedPen.setColor(color);
    circle->setPen(changedPen);
    writeText(output, colorSentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == colorSentinel); }
  }
  circle->setPen(savedPen);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  std::filesystem::remove(colorSource);
  const auto curveOnlySource = tmpFile("system_sparse_curves_only_source.dxf");
  for (int mode = 0; mode != 3; ++mode) {
    auto curveOnly = input;
    const auto first = curveOnly.find("0\nLINE\n"); REQUIRE(first != std::string::npos);
    const auto last = curveOnly.find(mode == 1 ? "0\nARC\n" : "0\nCIRCLE\n", first);
    REQUIRE(last != std::string::npos);
    curveOnly.erase(first, last - first);
    if (mode == 2) {
      const auto arcStart = curveOnly.find("0\nARC\n"); REQUIRE(arcStart != std::string::npos);
      const auto arcEnd = curveOnly.find("0\nENDSEC\n", arcStart); REQUIRE(arcEnd != std::string::npos);
      curveOnly.erase(arcStart, arcEnd - arcStart);
    }
    writeText(curveOnlySource, curveOnly);
    RS_Graphic onlyCurves;
    REQUIRE(filter.fileImport(onlyCurves, QString::fromStdString(curveOnlySource), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(onlyCurves, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(countRecords(output, "LINE") == 0);
    CHECK(countRecords(output, "CIRCLE") == (mode == 1 ? 0 : 1));
    CHECK(countRecords(output, "ARC") == (mode == 2 ? 0 : 1));
    RS_Graphic rereadCurves;
    REQUIRE(filter.fileImport(rereadCurves, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(rereadCurves, QString::fromStdString(resaved), RS2::FormatDXFRW));
  }
  std::filesystem::remove(curveOnlySource);
  auto *addedCircle = new RS_Circle(&graphic, RS_CircleData(RS_Vector(11.0, 12.0), 2.0));
  auto *addedArc = new RS_Arc(&graphic, RS_ArcData(RS_Vector(13.0, 14.0), 3.0, 0.0, M_PI / 2.0, false));
  auto *targetLayer = graphic.findLayer(QStringLiteral("RAWTEST")); REQUIRE(targetLayer);
  addedCircle->setLayer(targetLayer); addedArc->setLayer(targetLayer);
  addedCircle->setPen(circle->getPen(false)); addedArc->setPen(arc->getPen(false));
  graphic.addEntity(addedCircle); graphic.addEntity(addedArc);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(countRecords(output, "CIRCLE") == 2); CHECK(countRecords(output, "ARC") == 2);
  RS_Graphic additions;
  REQUIRE(filter.fileImport(additions, QString::fromStdString(output), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(additions, QString::fromStdString(resaved), RS2::FormatDXFRW));
  CHECK(countRecords(resaved, "CIRCLE") == 2); CHECK(countRecords(resaved, "ARC") == 2);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_ADDED"))
    std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
  REQUIRE(graphic.removeEntity(addedCircle)); REQUIRE(graphic.removeEntity(addedArc));
  const std::string sentinel = "preserve circle material destination\n";
  circle->setRadius(-1.0);
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  circle->setRadius(7.0);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  arc->setReversed(true);
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  arc->setReversed(false);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto alteredSource = tmpFile("system_sparse_circle_unqualified.dxf");
  for (int mode = 0; mode != 4; ++mode) {
    auto altered = input;
    if (mode == 0) {
      const auto at = altered.find("100\nAcDbCircle\n10\n2\n"); REQUIRE(at != std::string::npos);
      altered.insert(at + std::string("100\nAcDbCircle\n").size(), "39\n1\n");
    } else if (mode == 1) {
      const auto at = altered.find("30\n0\n40\n4\n"); REQUIRE(at != std::string::npos);
      altered.replace(at, std::string("30\n0\n").size(), "30\n1\n");
    } else if (mode == 2) {
      const auto at = altered.find("50\n30\n51\n120\n"); REQUIRE(at != std::string::npos);
      altered.replace(at, std::string("50\n30\n").size(), "50\n360\n");
    } else {
      const auto at = altered.find("0\nCIRCLE\n5\nA5\n"); REQUIRE(at != std::string::npos);
      altered.replace(at, std::string("0\nCIRCLE\n5\nA5\n").size(), "0\nCIRCLE\n5\nA2\n");
    }
    writeText(alteredSource, altered);
    RS_Graphic unqualified;
    writeText(output, sentinel);
    const bool imported = filter.fileImport(unqualified, QString::fromStdString(alteredSource), RS2::FormatDXFRW);
    if (mode < 3) REQUIRE(imported);
    if (imported) CHECK_FALSE(filter.fileExport(unqualified, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    RS_Graphic retry;
    REQUIRE(filter.fileImport(retry, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  struct DeliveryProbe : RS_FilterDXFRW {
    std::uint32_t suppress = 0;
    bool alterRadius = false, alterArc = false, alterPhysicalRadius = false;
    void addCircle(const DRW_Circle &data) override {
      if (data.handle == suppress) return;
      DRW_Circle changed = data;
      if (alterRadius) changed.radious += 1.0;
      RS_FilterDXFRW::addCircle(changed);
    }
    void addArc(const DRW_Arc &data) override {
      if (data.handle == suppress) return;
      DRW_Arc changed = data;
      if (alterArc) changed.staangle += 0.1;
      RS_FilterDXFRW::addArc(changed);
    }
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &source) override {
      auto copy = source;
      if (alterPhysicalRadius) for (auto &record : copy.records)
        if (record.name == "CIRCLE") for (std::size_t i = 0; i < record.groups.size(); ++i)
          if (record.groups[i].code() == 40) { record.rawValues[i] = "999"; break; }
      RS_FilterDXFRW::addDxfEntitiesSource(copy);
    }
  } probe;
  for (int mode = 0; mode != 5; ++mode) {
    auto altered = input;
    if (mode < 2) {
      const auto at = altered.find("0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"); REQUIRE(at != std::string::npos);
      altered.insert(at, mode == 0
          ? "0\nCIRCLE\n5\nA7\n100\nAcDbEntity\n8\nRAWTEST\n100\nAcDbCircle\n10\n1\n20\n1\n40\n1\n"
          : "0\nARC\n5\nA8\n100\nAcDbEntity\n8\nRAWTEST\n100\nAcDbCircle\n10\n1\n20\n1\n40\n1\n100\nAcDbArc\n50\n300\n51\n20\n");
      probe.suppress = mode == 0 ? 0xA7u : 0xA8u;
    } else if (mode == 2) probe.alterRadius = true;
    else if (mode == 3) probe.alterArc = true;
    else probe.alterPhysicalRadius = true;
    writeText(alteredSource, altered);
    RS_Graphic incomplete;
    REQUIRE(probe.fileImport(incomplete, QString::fromStdString(alteredSource), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(probe.fileExport(incomplete, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    probe.suppress = 0; probe.alterRadius = probe.alterArc = probe.alterPhysicalRadius = false;
    RS_Graphic retry;
    REQUIRE(probe.fileImport(retry, QString::fromStdString(mode < 2 ? alteredSource : source), RS2::FormatDXFRW));
    REQUIRE(probe.fileExport(retry, QString::fromStdString(output), RS2::FormatDXFRW));
    if (mode == 0) CHECK(countRecords(output, "CIRCLE") == 2);
    if (mode == 1) {
      CHECK(countRecords(output, "ARC") == 2);
      const auto starts = recordGroupValues(output, "ARC", "50");
      const auto ends = recordGroupValues(output, "ARC", "51");
      REQUIRE(starts.size() == 2u); REQUIRE(ends.size() == 2u);
      CHECK(std::abs(std::stod(starts.back()) - 300.0) < 1e-10);
      CHECK(std::abs(std::stod(ends.back()) - 20.0) < 1e-10);
      RS_Graphic wrapped;
      REQUIRE(probe.fileImport(wrapped, QString::fromStdString(output), RS2::FormatDXFRW));
      REQUIRE(probe.fileExport(wrapped, QString::fromStdString(resaved), RS2::FormatDXFRW));
      if (const char *copy = std::getenv("LIBRECAD_KEEP_ORDINARY_SPARSE_WRAP"))
        std::filesystem::copy_file(resaved, copy, std::filesystem::copy_options::overwrite_existing);
    }
  }
  std::filesystem::remove(alteredSource);
  std::filesystem::remove(source); std::filesystem::remove(output); std::filesystem::remove(resaved);
}

TEST_CASE("Opt-in sparse material binds block entity source ownership",
          "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_sparse_material_block_source.dxf");
  const auto output = tmpFile("system_sparse_material_block_output.dxf");
  const std::string sentinel = "preserve block material destination\n";
  writeText(source,
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n1\n"
      "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
      "2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n0\nENDTAB\n0\nENDSEC\n"
      "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n5\nB0\n8\n0\n2\nTESTBLOCK\n70\n0\n10\n0\n20\n0\n30\n0\n"
      "0\nLINE\n5\nA2\n100\nAcDbEntity\n8\n0\n347\nA1\n"
      "100\nAcDbLine\n10\n0\n20\n0\n30\n0\n11\n10\n21\n0\n31\n0\n"
      "0\nENDBLK\n5\nB1\n8\n0\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_MATERIAL\n350\nA0\n0\nDICTIONARY\n5\nA0\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nPlainMat\n350\nA1\n0\nMATERIAL\n5\nA1\n330\nA0\n100\nAcDbMaterial\n1\nPlainMat\n94\n63\n"
      "0\nENDSEC\n0\nEOF\n");
  struct OptInFilter : RS_FilterDXFRW {
    bool suppressUnmaterializedLine = false;
    void addLine(const DRW_Line &line) override {
      if (!suppressUnmaterializedLine || line.handle != 0xA5u)
        RS_FilterDXFRW::addLine(line);
    }
#ifdef LIBDXFRW_HAS_DXF_PHYSICAL_IDENTITY_SOURCE_HANDLER
    std::function<void(DRW_DxfIdentitySource &)> mutateIdentity;
    void addDxfIdentitySource(const DRW_DxfIdentitySource &source) override {
      auto copy = source;
      if (mutateIdentity) mutateIdentity(copy);
      RS_FilterDXFRW::addDxfIdentitySource(copy);
    }
#endif
    bool wantsDxfSparseMaterialRawCapture() const noexcept override { return true; }
    bool wantsDxfSparseMaterialRawReplay() const noexcept override { return true; }
  } filter;
  RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  auto *block = graphic.findBlock(QStringLiteral("TESTBLOCK")); REQUIRE(block);
  auto *line = block->firstEntity(); REQUIRE(line); REQUIRE(line->materialHandle() == 0xA1u);
  const auto *captured = graphic.dwgAdvancedMetadata().dxfMaterialEntitySources();
  REQUIRE(captured); REQUIRE(captured->size() == 1u);
  CHECK(captured->front().source == line->sourceHandle());
  CHECK(captured->front().lineage == line->lineage());
  CHECK(captured->front().block); CHECK(captured->front().blockName == "TESTBLOCK");
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
#ifdef LIBDXFRW_HAS_DXF_PHYSICAL_IDENTITY_SOURCE_HANDLER
  for (const std::string name : {"BLOCK", "LINE", "ENDBLK"}) {
    OptInFilter alteredFilter;
    alteredFilter.mutateIdentity = [name](DRW_DxfIdentitySource &identity) {
      const auto found = std::find_if(identity.records.begin(), identity.records.end(),
          [&](const auto &record) { return record.name == name; });
      if (found != identity.records.end()) identity.records.erase(found);
    };
    RS_Graphic alteredGraphic;
    REQUIRE(alteredFilter.fileImport(alteredGraphic, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(alteredFilter.fileExport(alteredGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    alteredFilter.mutateIdentity = {};
    RS_Graphic corrected;
    REQUIRE(alteredFilter.fileImport(corrected, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(alteredFilter.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW));
  }
#endif
  {
    std::ifstream file(source, std::ios::binary);
    const std::string original((std::istreambuf_iterator<char>(file)), {});
    auto withSecondLine = original;
    const auto end = withSecondLine.find("0\nENDBLK\n");
    REQUIRE(end != std::string::npos);
    withSecondLine.insert(end, "0\nLINE\n5\nA5\n100\nAcDbEntity\n8\n0\n"
                               "100\nAcDbLine\n10\n0\n20\n1\n11\n10\n21\n1\n");
    writeText(source, withSecondLine);
    OptInFilter deliveryFilter;
    deliveryFilter.suppressUnmaterializedLine = true;
    RS_Graphic missing;
    REQUIRE(deliveryFilter.fileImport(missing, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(deliveryFilter.fileExport(missing, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    deliveryFilter.suppressUnmaterializedLine = false;
    RS_Graphic delivered;
    REQUIRE(deliveryFilter.fileImport(delivered, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(deliveryFilter.fileExport(delivered, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(countRecords(output, "LINE") == 2);
    writeText(source, original);
  }
  block->setName(QStringLiteral("CHANGED")); writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  block->setName(QStringLiteral("TESTBLOCK"));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  const auto originalLineage = line->lineage();
  line->setLineage(RS_Entity::newLineage()); writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  line->setLineage(originalLineage);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  line->setMaterialHandle(0u); writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  line->setMaterialHandle(0xA1u);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  {
    std::ifstream file(source, std::ios::binary);
    const std::string original((std::istreambuf_iterator<char>(file)), {});
    auto altered = original;
    const auto at = altered.find("8\n0\n347\nA1\n");
    REQUIRE(at != std::string::npos);
    altered.insert(at + std::string("8\n0\n").size(), "347\n0\n");
    writeText(source, altered);
    RS_Graphic duplicate;
    REQUIRE(filter.fileImport(duplicate, QString::fromStdString(source), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(duplicate, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    writeText(source, original);
    RS_Graphic corrected;
    REQUIRE(filter.fileImport(corrected, QString::fromStdString(source), RS2::FormatDXFRW));
    REQUIRE(filter.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW));
  }
  std::filesystem::remove(source); std::filesystem::remove(output);
}
#endif

TEST_CASE("System codec gates ambiguous AC1024 default material",
          "[system-codec][dxf][material][transaction]") {
  ensureSettings();
  const auto source = tmpFile("system_ezdxf_material_source.dxf");
  const auto output = tmpFile("system_ezdxf_material_output.dxf");
  // Created locally from the observed default-material group shape. The
  // second 73 is intentionally left ambiguous and must remain raw-only.
  writeText(source,
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1024\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n0\nMATERIAL\n5\n1F\n"
      "102\n{ACAD_REACTORS\n330\nE\n102\n}\n330\nE\n100\nAcDbMaterial\n"
      "1\nByBlock\n2\n\n70\n0\n40\n1\n71\n1\n41\n1\n91\n-1023410177\n"
      "42\n1\n72\n1\n3\n\n73\n1\n74\n1\n75\n1\n44\n0.5\n73\n0\n"
      "45\n1\n46\n1\n77\n1\n4\n\n78\n1\n79\n1\n170\n1\n48\n1\n"
      "171\n1\n6\n\n172\n1\n173\n1\n174\n1\n140\n1\n141\n1\n"
      "175\n1\n7\n\n176\n1\n177\n1\n178\n1\n143\n1\n179\n1\n8\n\n"
      "270\n1\n271\n1\n272\n1\n145\n1\n146\n1\n273\n1\n9\n\n"
      "274\n1\n275\n1\n276\n1\n42\n1\n72\n1\n3\n\n73\n1\n74\n1\n75\n1\n"
      "94\n63\n0\nENDSEC\n0\nEOF\n");
  struct OptInFilter : RS_FilterDXFRW
#ifndef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
      , DRW_DxfSparseMaterialRawPolicy
#endif
  {
    bool wantsDxfSparseMaterialRawCapture() const noexcept override { return true; }
#ifdef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
    bool wantsDxfSparseMaterialRawReplay() const noexcept override { return true; }
#endif
  };
  RS_Graphic legacy;
  RS_FilterDXFRW legacyFilter;
#ifdef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
  REQUIRE(legacyFilter.fileImport(legacy, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(legacy.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(legacy.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
#else
  CHECK_FALSE(legacyFilter.fileImport(legacy, QString::fromStdString(source), RS2::FormatDXFRW));
#endif
  RS_Graphic graphic;
  OptInFilter importer;
  REQUIRE(importer.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(graphic.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(graphic.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  const auto sourceSnapshot = graphic.dwgAdvancedMetadata().materials().front().rawDxfSource;
  REQUIRE(sourceSnapshot);
  CHECK_FALSE(graphic.dwgAdvancedMetadata().materials().front().rawDxfSourceConflict);
  REQUIRE(graphic.dwgAdvancedMetadata().rawDxfObjects().size() == 1u);
  CHECK(graphic.dwgAdvancedMetadata().rawDxfObjects().front().groups.size() == 61u);
  REQUIRE(sourceSnapshot->groups.size() == 61u);
  REQUIRE(sourceSnapshot->hasRawValues);
  REQUIRE(sourceSnapshot->rawValues.size() == sourceSnapshot->groups.size());
  auto &mutableRaw = const_cast<std::vector<DRW_RawDxfObject> &>(
      graphic.dwgAdvancedMetadata().rawDxfObjects());
  REQUIRE(mutableRaw.front().groups.back().type() == DRW_Variant::INTEGER);
  const int originalTail = sourceSnapshot->groups.back().i_val();
  const std::string originalSpelling = sourceSnapshot->rawValues.back();
  mutableRaw.front().groups.back().addInt(94, originalTail + 1);
  mutableRaw.front().rawValues.back() = "changed";
  CHECK(sourceSnapshot->groups.back().i_val() == originalTail);
  CHECK(sourceSnapshot->rawValues.back() == originalSpelling);
  const std::string sentinel = "retain AC1024 destination\n";
  writeText(output, sentinel);
  RS_FilterDXFRW exporter;
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  std::ifstream retained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  mutableRaw.front().groups.back().addInt(94, originalTail);
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  mutableRaw.front().rawValues.back() = originalSpelling;
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  mutableRaw.front().groups.back().addInt(94, originalTail + 1);
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  mutableRaw.front().groups.back().addInt(94, originalTail);
  auto &material = const_cast<LC_DwgAdvancedMetadata::MaterialRecord &>(
      graphic.dwgAdvancedMetadata().materials().front());
  material.name = "changed";
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  material.name = "ByBlock";
  const auto originalOwner = material.parentHandle;
  material.parentHandle = originalOwner + 1;
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  material.parentHandle = originalOwner;
  const auto originalReactors = material.reactorHandles;
  material.reactorHandles.push_back(0x99u);
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  material.reactorHandles = originalReactors;
  const auto nameGroup = std::find_if(mutableRaw.front().groups.begin(),
      mutableRaw.front().groups.end(), [](const auto &group) { return group.code() == 1; });
  REQUIRE(nameGroup != mutableRaw.front().groups.end());
  const auto originalNameGroup = *nameGroup;
  nameGroup->addString(1, std::string("ByBlock\0extra", 13));
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  *nameGroup = originalNameGroup;
  material.requiresRawDxfReplay = false;
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  material.requiresRawDxfReplay = true;
  mutableRaw.push_back(mutableRaw.front());
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  mutableRaw.pop_back();
  // Add a locally authored root and material dictionary to the same source;
  // the opt-in path should retain both source frames without admitting export.
  const auto graphSource = tmpFile("system_sparse_material_dictionary_source.dxf");
  {
    std::ifstream input(source, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const std::string marker = "0\nSECTION\n2\nOBJECTS\n";
    const auto at = graph.find(marker);
    REQUIRE(at != std::string::npos);
    graph.insert(at + marker.size(),
        "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
        "3\nACAD_MATERIAL\n350\nE\n"
        "0\nDICTIONARY\n5\nE\n330\nC\n100\nAcDbDictionary\n281\n1\n"
        "3\nByBlock\n350\n1F\n");
    const std::string versionMarker = "1\nAC1024\n";
    const auto headerAt = graph.find(versionMarker);
    REQUIRE(headerAt != std::string::npos);
    graph.insert(headerAt + versionMarker.size(), "9\n$CMATERIAL\n347\n1F\n");
    graph.insert(graph.find(marker), "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n");
    writeText(graphSource, graph);
  }
  RS_Graphic graphGraphic;
  OptInFilter graphImporter;
  REQUIRE(graphImporter.fileImport(graphGraphic, QString::fromStdString(graphSource), RS2::FormatDXFRW));
  const auto &graphMetadata = graphGraphic.dwgAdvancedMetadata();
  REQUIRE(graphMetadata.dxfRootDictionarySource());
  REQUIRE(graphMetadata.dxfMaterialDictionarySource());
  CHECK(graphMetadata.dxfRootDictionarySource()->handle == 0xCu);
  CHECK(graphMetadata.dxfMaterialDictionarySource()->handle == 0xEu);
  REQUIRE(graphMetadata.materials().size() == 1u);
  CHECK(graphMetadata.materials().front().rawDxfSource);
#if defined(LIBDXFRW_HAS_DXF_DEFAULT_PLOT_PLACEHOLDER_HANDLE) && defined(LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR)
  const auto rawReplay = tmpFile("system_ezdxf_material_graph_replay.dxf");
  REQUIRE(graphImporter.fileExport(graphGraphic, QString::fromStdString(rawReplay), RS2::FormatDXFRW2010));
  CHECK(recordGroupValues(rawReplay, "MATERIAL", "94") == std::vector<std::string>{"63"});
  RS_Graphic replayed;
  OptInFilter replayFilter;
  REQUIRE(replayFilter.fileImport(replayed, QString::fromStdString(rawReplay), RS2::FormatDXFRW));
  REQUIRE(replayFilter.fileExport(replayed, QString::fromStdString(rawReplay), RS2::FormatDXFRW2010));
  CHECK(recordGroupValues(rawReplay, "MATERIAL", "94") == std::vector<std::string>{"63"});
  auto &replayedRaw = const_cast<std::vector<DRW_RawDxfObject>&>(replayed.dwgAdvancedMetadata().rawDxfObjects());
  const auto plot = std::find_if(replayedRaw.begin(), replayedRaw.end(), [](const auto &record) {
    return record.name == "ACDBDICTIONARYWDFLT";
  });
  REQUIRE(plot != replayedRaw.end());
  const auto cloning = std::find_if(plot->groups.begin(), plot->groups.end(),
                                    [](const auto &group) { return group.code() == 281; });
  REQUIRE(cloning != plot->groups.end());
  const auto originalPlotCloning = *cloning;
  cloning->addInt(281, 2);
  writeText(output, sentinel);
  CHECK_FALSE(replayFilter.fileExport(replayed, QString::fromStdString(output), RS2::FormatDXFRW2010));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  *cloning = originalPlotCloning;
  REQUIRE(replayFilter.fileExport(replayed, QString::fromStdString(rawReplay), RS2::FormatDXFRW2010));
  if (const char *copy = std::getenv("LIBRECAD_KEEP_SPARSE_2010_OUTPUT"))
    std::filesystem::copy_file(rawReplay, copy, std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(rawReplay);
  writeText(output, sentinel);
#endif
#ifdef LIBDXFRW_HAS_DXF_HEADER_MATERIAL_SOURCE_HANDLER
  const auto *physicalHeader = graphMetadata.dxfMaterialHeaderPhysicalSource();
  REQUIRE(physicalHeader);
  CHECK(physicalHeader->version == DRW::AC1024);
  CHECK_FALSE(physicalHeader->binary);
  CHECK(physicalHeader->present);
  CHECK(physicalHeader->dollarName);
  CHECK(std::string(physicalHeader->handleLexeme.data(), physicalHeader->handleLength) == "1F");
  {
    std::ifstream file(graphSource, std::ios::binary);
    const std::string original((std::istreambuf_iterator<char>(file)), {});
    REQUIRE(physicalHeader->variableName.endOffset <= original.size());
    REQUIRE(physicalHeader->value.endOffset <= original.size());
    CHECK(original.substr(physicalHeader->variableName.beginOffset,
                          physicalHeader->variableName.endOffset - physicalHeader->variableName.beginOffset) ==
          "9\n$CMATERIAL\n");
    CHECK(original.substr(physicalHeader->value.beginOffset,
                          physicalHeader->value.endOffset - physicalHeader->value.beginOffset) ==
          "347\n1F\n");
  }
  const auto bareHeaderSource = tmpFile("system_sparse_material_bare_header.dxf");
  {
    std::ifstream input(graphSource, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const auto at = graph.find("9\n$CMATERIAL\n347\n1F\n");
    REQUIRE(at != std::string::npos);
    graph.erase(at + 2u, 1u);
    writeText(bareHeaderSource, graph);
  }
  RS_Graphic bareHeaderGraphic;
  OptInFilter bareHeaderImporter;
  REQUIRE(bareHeaderImporter.fileImport(bareHeaderGraphic, QString::fromStdString(bareHeaderSource), RS2::FormatDXFRW));
  REQUIRE(bareHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource());
  CHECK_FALSE(bareHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource()->dollarName);
  CHECK_FALSE(exporter.fileExport(bareHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  writeText(output, sentinel);
  CHECK_FALSE(bareHeaderImporter.fileExport(bareHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  std::filesystem::remove(bareHeaderSource);
#endif
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  graphGraphic.removeVariable("$CMATERIAL");
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  graphGraphic.addVariable("$CMATERIAL", QStringLiteral("1F"), 347);
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  graphGraphic.addVariable("$CMATERIAL", QStringLiteral("2F"), 347);
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  graphGraphic.addVariable("$CMATERIAL", QStringLiteral("1F"), 5);
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  graphGraphic.addVariable("$CMATERIAL", QStringLiteral("not-a-handle"), 347);
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  graphGraphic.addVariable("$CMATERIAL", QStringLiteral("0"), 347);
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  graphGraphic.addVariable("$CMATERIAL", QStringLiteral("1F"), 347);
  auto &graphRaw = const_cast<std::vector<DRW_RawDxfObject> &>(graphMetadata.rawDxfObjects());
  const auto dictionary = std::find_if(graphRaw.begin(), graphRaw.end(),
      [](const auto &record) { return record.name == "DICTIONARY" && record.handle == 0xEu; });
  REQUIRE(dictionary != graphRaw.end());
  const auto entry = std::find_if(dictionary->groups.begin(), dictionary->groups.end(),
      [](const auto &group) { return group.code() == 3; });
  REQUIRE(entry != dictionary->groups.end());
  entry->addString(3, "changed");
  const auto &sourceGroups = graphMetadata.dxfMaterialDictionarySource()->groups;
  const auto originalEntry = std::find_if(sourceGroups.begin(), sourceGroups.end(),
      [](const auto &group) { return group.code() == 3; });
  REQUIRE(originalEntry != sourceGroups.end());
  CHECK(originalEntry->stringEquals("ByBlock"));
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  entry->addString(3, "ByBlock");
  auto &typedDictionaries = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord> &>(
      graphMetadata.dictionaries());
  const auto typedMaterialDictionary = std::find_if(typedDictionaries.begin(), typedDictionaries.end(),
      [](const auto &record) { return record.handle == 0xEu; });
  REQUIRE(typedMaterialDictionary != typedDictionaries.end());
  const auto originalCloning = typedMaterialDictionary->cloning;
  typedMaterialDictionary->cloning = originalCloning + 1;
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  typedMaterialDictionary->cloning = originalCloning;
  const auto typedRoot = std::find_if(typedDictionaries.begin(), typedDictionaries.end(),
      [](const auto &record) { return record.handle == 0xCu; });
  REQUIRE(typedRoot != typedDictionaries.end());
  REQUIRE(typedRoot->entries.size() == 1u);
  const auto originalRootRole = typedRoot->entries.front().dxfHandleCode;
  typedRoot->entries.front().dxfHandleCode = 360;
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  typedRoot->entries.front().dxfHandleCode = originalRootRole;
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  graphRaw.push_back(*dictionary);
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  graphRaw.pop_back();
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  std::ifstream graphRetained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(graphRetained), {}) == sentinel);
  const auto duplicateSource = *graphMetadata.dxfMaterialDictionarySource();
  graphGraphic.dwgAdvancedMetadata().addDxfMaterialDictionarySource(duplicateSource);
  CHECK_FALSE(graphMetadata.dxfMaterialDictionarySource());
  CHECK_FALSE(exporter.fileExport(graphGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  const auto danglingSource = tmpFile("system_sparse_material_dangling_target.dxf");
  {
    std::ifstream input(graphSource, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const std::string entryMarker = "3\nByBlock\n350\n1F\n";
    const auto at = graph.find(entryMarker);
    REQUIRE(at != std::string::npos);
    graph.insert(at + entryMarker.size(), "3\nMissing\n350\n2F\n");
    writeText(danglingSource, graph);
  }
  RS_Graphic danglingGraphic;
  OptInFilter danglingImporter;
  REQUIRE(danglingImporter.fileImport(danglingGraphic, QString::fromStdString(danglingSource), RS2::FormatDXFRW));
  REQUIRE(danglingGraphic.dwgAdvancedMetadata().dxfMaterialDictionarySource());
  CHECK_FALSE(exporter.fileExport(danglingGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  std::filesystem::remove(danglingSource);
  const auto absentHeaderSource = tmpFile("system_sparse_material_absent_header.dxf");
  {
    std::ifstream input(graphSource, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const std::string field = "9\n$CMATERIAL\n347\n1F\n";
    const auto at = graph.find(field);
    REQUIRE(at != std::string::npos);
    graph.erase(at, field.size());
    writeText(absentHeaderSource, graph);
  }
  RS_Graphic absentHeaderGraphic;
  OptInFilter absentHeaderImporter;
  REQUIRE(absentHeaderImporter.fileImport(absentHeaderGraphic, QString::fromStdString(absentHeaderSource), RS2::FormatDXFRW));
  bool headerPresent = true;
  std::uint32_t headerHandle = 42u;
  REQUIRE(absentHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderSource(DRW::AC1024, headerPresent, headerHandle));
  CHECK_FALSE(headerPresent);
#ifdef LIBDXFRW_HAS_DXF_HEADER_MATERIAL_SOURCE_HANDLER
  REQUIRE(absentHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource());
  CHECK_FALSE(absentHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource()->present);
#endif
  CHECK_FALSE(absentHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderSource(DRW::AC1021, headerPresent, headerHandle));
  CHECK_FALSE(exporter.fileExport(absentHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  absentHeaderGraphic.addVariable("$CMATERIAL", QStringLiteral("1F"), 347);
  CHECK_FALSE(exporter.fileExport(absentHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  absentHeaderGraphic.removeVariable("$CMATERIAL");
  CHECK_FALSE(exporter.fileExport(absentHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  std::filesystem::remove(absentHeaderSource);
  const auto nullHeaderSource = tmpFile("system_sparse_material_null_header.dxf");
  {
    std::ifstream input(graphSource, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const std::string field = "9\n$CMATERIAL\n347\n1F\n";
    const auto at = graph.find(field);
    REQUIRE(at != std::string::npos);
    graph.replace(at, field.size(), "9\n$CMATERIAL\n347\n0\n");
    writeText(nullHeaderSource, graph);
  }
  RS_Graphic nullHeaderGraphic;
  OptInFilter nullHeaderImporter;
  REQUIRE(nullHeaderImporter.fileImport(nullHeaderGraphic, QString::fromStdString(nullHeaderSource), RS2::FormatDXFRW));
  REQUIRE(nullHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderSource(DRW::AC1024, headerPresent, headerHandle));
  CHECK(headerPresent);
  CHECK(headerHandle == 0u);
#ifdef LIBDXFRW_HAS_DXF_HEADER_MATERIAL_SOURCE_HANDLER
  REQUIRE(nullHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource());
  CHECK(nullHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource()->handleLexeme[0] == '0');
#endif
  CHECK_FALSE(exporter.fileExport(nullHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
  nullHeaderGraphic.removeVariable("$CMATERIAL");
  CHECK_FALSE(exporter.fileExport(nullHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  std::filesystem::remove(nullHeaderSource);
  const auto duplicateHeaderSource = tmpFile("system_sparse_material_duplicate_header.dxf");
  {
    std::ifstream input(graphSource, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const std::string field = "9\n$CMATERIAL\n347\n1F\n";
    const auto at = graph.find(field);
    REQUIRE(at != std::string::npos);
    graph.insert(at + field.size(), field);
    writeText(duplicateHeaderSource, graph);
  }
  RS_Graphic duplicateHeaderGraphic;
  OptInFilter duplicateHeaderImporter;
  REQUIRE(duplicateHeaderImporter.fileImport(duplicateHeaderGraphic, QString::fromStdString(duplicateHeaderSource), RS2::FormatDXFRW));
  CHECK_FALSE(duplicateHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderSource(DRW::AC1024, headerPresent, headerHandle));
#ifdef LIBDXFRW_HAS_DXF_HEADER_MATERIAL_SOURCE_HANDLER
  CHECK_FALSE(duplicateHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource());
#endif
  CHECK_FALSE(exporter.fileExport(duplicateHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  std::ifstream headerRetained(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(headerRetained), {}) == sentinel);
  std::filesystem::remove(duplicateHeaderSource);
#ifdef LIBDXFRW_HAS_DXF_HEADER_MATERIAL_SOURCE_HANDLER
  const auto repeatedHeaderSource = tmpFile("system_sparse_material_repeated_header.dxf");
  {
    std::ifstream input(graphSource, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const std::string marker = "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n";
    const auto at = graph.find(marker);
    REQUIRE(at != std::string::npos);
    graph.insert(at + std::string("0\nENDSEC\n").size(),
                 "0\nSECTION\n2\nHEADER\n9\n$CMATERIAL\n347\n1F\n0\nENDSEC\n");
    writeText(repeatedHeaderSource, graph);
  }
  RS_Graphic repeatedHeaderGraphic;
  OptInFilter repeatedHeaderImporter;
  REQUIRE(repeatedHeaderImporter.fileImport(repeatedHeaderGraphic, QString::fromStdString(repeatedHeaderSource), RS2::FormatDXFRW));
  CHECK_FALSE(repeatedHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource());
  CHECK_FALSE(repeatedHeaderGraphic.dwgAdvancedMetadata().dxfMaterialHeaderSource(DRW::AC1024, headerPresent, headerHandle));
  CHECK_FALSE(exporter.fileExport(repeatedHeaderGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  std::filesystem::remove(repeatedHeaderSource);
#endif
  const auto layerSource = tmpFile("system_sparse_material_layer_reference.dxf");
  for (const auto &target : {std::string("2F"), std::string("1F")}) {
    std::ifstream input(graphSource, std::ios::binary);
    std::string graph((std::istreambuf_iterator<char>(input)), {});
    const auto at = graph.find("0\nSECTION\n2\nOBJECTS\n");
    REQUIRE(at != std::string::npos);
    graph.insert(at, "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n5\n60\n70\n1\n"
                     "0\nLAYER\n5\n30\n100\nAcDbSymbolTableRecord\n100\nAcDbLayerTableRecord\n"
                     "2\nRAWTEST\n70\n0\n62\n7\n6\nCONTINUOUS\n347\n" + target +
                     "\n0\nENDTAB\n0\nENDSEC\n");
    writeText(layerSource, graph);
    RS_Graphic layerGraphic;
    OptInFilter layerImporter;
    REQUIRE(layerImporter.fileImport(layerGraphic, QString::fromStdString(layerSource), RS2::FormatDXFRW));
    const auto *layerFrame = layerGraphic.dwgAdvancedMetadata().dxfMaterialLayerSource(0x30u);
    REQUIRE(layerFrame);
    CHECK_FALSE(exporter.fileExport(layerGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
    CHECK(exporter.exportReport().contains(target == "1F"
        ? QStringLiteral("raw-only graph requires qualification")
        : QStringLiteral("raw source missing, changed or unverified")));
    if (target == "1F") {
      auto *mutableFrame = const_cast<DRW_RawDxfObject *>(layerFrame);
      const auto group = std::find_if(mutableFrame->groups.begin(), mutableFrame->groups.end(),
          [](const auto &value) { return value.code() == 347; });
      REQUIRE(group != mutableFrame->groups.end());
      group->addString(347, "2F");
      CHECK_FALSE(exporter.fileExport(layerGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
      CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
      group->addString(347, "1F");
      CHECK_FALSE(exporter.fileExport(layerGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
      CHECK(exporter.exportReport().contains(QStringLiteral("raw-only graph requires qualification")));
      const auto sourceVersion = mutableFrame->m_version;
      mutableFrame->m_version = DRW::AC1021;
      CHECK_FALSE(exporter.fileExport(layerGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
      CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
      mutableFrame->m_version = sourceVersion;
      const auto duplicate = *layerFrame;
      layerGraphic.dwgAdvancedMetadata().addDxfMaterialLayerSource(duplicate);
      CHECK_FALSE(layerGraphic.dwgAdvancedMetadata().dxfMaterialLayerSource(0x30u));
      CHECK_FALSE(exporter.fileExport(layerGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
      CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
    }
  }
  // Generate the same locally authored control in Binary DXF without relying
  // on a private codec writer. Only the group kinds in this fixture are used.
  const auto binarySource = tmpFile("system_ezdxf_material_source_binary.dxf");
  {
    std::ifstream ascii(layerSource, std::ios::binary);
    std::ofstream binary(binarySource, std::ios::binary | std::ios::trunc);
    const char magic[] = "AutoCAD Binary DXF\r\n\x1a";
    binary.write(magic, sizeof(magic));
    const auto littleEndian = [&](std::uint64_t value, unsigned bytes) {
      for (unsigned i = 0; i != bytes; ++i)
        binary.put(static_cast<char>((value >> (i * 8)) & 0xffu));
    };
    std::string codeText, value;
    while (std::getline(ascii, codeText)) {
      REQUIRE(static_cast<bool>(std::getline(ascii, value)));
      const int code = std::stoi(codeText);
      littleEndian(static_cast<std::uint16_t>(code), 2);
      if (code <= 9 || code == 100 || code == 102 || code == 330 || code == 347 ||
          (code >= 350 && code <= 369)) {
        binary.write(value.data(), static_cast<std::streamsize>(value.size()));
        binary.put('\0');
      } else if ((code >= 40 && code <= 59) || (code >= 140 && code <= 149)) {
        // Exercise binary64 values that need full precision in ASCII DXF.
        const double number = code == 44 ? std::nextafter(1.0, 2.0) : std::stod(value);
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(number), "DXF doubles need 64 bits");
        std::memcpy(&bits, &number, sizeof(bits));
        littleEndian(bits, 8);
      } else if (code >= 90 && code <= 99) {
        littleEndian(static_cast<std::uint32_t>(std::stoll(value)), 4);
      } else {
        REQUIRE(((code >= 60 && code <= 79) || (code >= 170 && code <= 179) ||
                 (code >= 270 && code <= 289)));
        littleEndian(static_cast<std::uint16_t>(std::stoi(value)), 2);
      }
    }
    REQUIRE(binary.good());
  }
  {
    std::ifstream file(binarySource, std::ios::binary);
    std::string altered((std::istreambuf_iterator<char>(file)), {});
    const auto entities = altered.find(std::string("ENTITIES\0", 9));
    REQUIRE(entities != std::string::npos);
    const auto end = altered.find(std::string("\0\0ENDSEC\0", 9), entities + 9u);
    REQUIRE(end != std::string::npos);
    std::string entity;
    const auto textGroup = [&](unsigned code, const std::string &value) {
      entity.push_back(static_cast<char>(code & 255u));
      entity.push_back(static_cast<char>((code >> 8) & 255u));
      entity += value; entity.push_back('\0');
    };
    textGroup(0, "LINE"); textGroup(5, "A5"); textGroup(100, "AcDbEntity");
    textGroup(8, "0"); textGroup(347, "0"); textGroup(100, "AcDbLine");
    const auto doubleGroup = [&](unsigned code, double value) {
      entity.push_back(static_cast<char>(code & 255u));
      entity.push_back(static_cast<char>((code >> 8) & 255u));
      std::uint64_t bits = 0; std::memcpy(&bits, &value, sizeof(bits));
      for (unsigned i = 0; i != 8; ++i) entity.push_back(static_cast<char>((bits >> (8u * i)) & 255u));
    };
    doubleGroup(10, 0.0); doubleGroup(20, 0.0); doubleGroup(30, 0.0);
    doubleGroup(11, 1.0); doubleGroup(21, 1.0); doubleGroup(31, 0.0);
    altered.insert(end, entity);
    const auto invalidBinary = tmpFile("system_sparse_material_binary_zero_entity.dxf");
    writeText(invalidBinary, altered);
    RS_Graphic invalidGraphic;
    OptInFilter invalidImporter;
    REQUIRE(invalidImporter.fileImport(invalidGraphic, QString::fromStdString(invalidBinary), RS2::FormatDXFRW));
    writeText(output, sentinel);
    CHECK_FALSE(invalidImporter.fileExport(invalidGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    RS_Graphic corrected;
    REQUIRE(invalidImporter.fileImport(corrected, QString::fromStdString(binarySource), RS2::FormatDXFRW));
    REQUIRE(invalidImporter.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW2010));
    std::filesystem::remove(invalidBinary);
  }
  if (const char *copy = std::getenv("LIBRECAD_KEEP_SPARSE_BINARY_SOURCE"))
    std::filesystem::copy_file(binarySource, copy, std::filesystem::copy_options::overwrite_existing);
#ifdef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
  RS_Graphic ordinaryBinary;
  RS_FilterDXFRW ordinaryBinaryFilter;
  REQUIRE(ordinaryBinaryFilter.fileImport(ordinaryBinary, QString::fromStdString(binarySource), RS2::FormatDXFRW));
  REQUIRE(ordinaryBinary.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(ordinaryBinary.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  writeText(output, sentinel);
  CHECK_FALSE(ordinaryBinaryFilter.fileExport(ordinaryBinary, QString::fromStdString(output), RS2::FormatDXFRW2010));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
#endif
  RS_Graphic binaryGraphic;
  OptInFilter binaryImporter;
  REQUIRE(binaryImporter.fileImport(binaryGraphic, QString::fromStdString(binarySource), RS2::FormatDXFRW));
  REQUIRE(binaryGraphic.dwgAdvancedMetadata().materials().size() == 1u);
  const auto binarySnapshot = binaryGraphic.dwgAdvancedMetadata().materials().front().rawDxfSource;
  REQUIRE(binarySnapshot);
  CHECK_FALSE(binarySnapshot->hasRawValues);
  CHECK(binarySnapshot->groups.size() == 61u);
#ifdef LIBDXFRW_HAS_DXF_HEADER_MATERIAL_SOURCE_HANDLER
  const auto *binaryHeader = binaryGraphic.dwgAdvancedMetadata().dxfMaterialHeaderPhysicalSource();
  REQUIRE(binaryHeader);
  CHECK(binaryHeader->binary);
  CHECK(binaryHeader->present);
  CHECK(std::string(binaryHeader->handleLexeme.data(), binaryHeader->handleLength) == "1F");
  {
    std::ifstream file(binarySource, std::ios::binary);
    const std::string original((std::istreambuf_iterator<char>(file)), {});
    REQUIRE(binaryHeader->variableName.endOffset <= original.size());
    REQUIRE(binaryHeader->value.endOffset <= original.size());
    CHECK(original.substr(binaryHeader->variableName.beginOffset,
                          binaryHeader->variableName.endOffset - binaryHeader->variableName.beginOffset) ==
          std::string("\x09\x00$CMATERIAL\x00", 13));
    CHECK(original.substr(binaryHeader->value.beginOffset,
                          binaryHeader->value.endOffset - binaryHeader->value.beginOffset) ==
          std::string("\x5b\x01" "1F\x00", 5));
  }
#endif
  REQUIRE(binaryGraphic.dwgAdvancedMetadata().dxfRootDictionarySource());
  REQUIRE(binaryGraphic.dwgAdvancedMetadata().dxfMaterialDictionarySource());
  CHECK_FALSE(binaryGraphic.dwgAdvancedMetadata().dxfMaterialDictionarySource()->hasRawValues);
  REQUIRE(binaryGraphic.dwgAdvancedMetadata().dxfMaterialLayerSource(0x30u));
  CHECK_FALSE(binaryGraphic.dwgAdvancedMetadata().dxfMaterialLayerSource(0x30u)->hasRawValues);
  const auto binaryReplay = tmpFile("system_sparse_material_binary_to_ascii.dxf");
  const bool binaryWritten = binaryImporter.fileExport(binaryGraphic, QString::fromStdString(binaryReplay), RS2::FormatDXFRW2010);
  INFO(binaryImporter.exportReport().toStdString());
  REQUIRE(binaryWritten);
  RS_Graphic binaryReopened;
  OptInFilter binaryReopener;
  REQUIRE(binaryReopener.fileImport(binaryReopened, QString::fromStdString(binaryReplay), RS2::FormatDXFRW));
  REQUIRE(binaryReopened.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(binaryReopened.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  const auto replaySnapshot = binaryReopened.dwgAdvancedMetadata().materials().front().rawDxfSource;
  REQUIRE(replaySnapshot);
  REQUIRE(replaySnapshot->groups.size() == binarySnapshot->groups.size());
  for (std::size_t i = 0; i < binarySnapshot->groups.size(); ++i) {
    const auto &before = binarySnapshot->groups[i];
    const auto &after = replaySnapshot->groups[i];
    REQUIRE(before.code() == after.code());
    REQUIRE(before.type() == after.type());
    // Identity and ownership are checked through the staged reference graph.
    if (before.code() == 5 || before.code() == 330) continue;
    if (before.type() == DRW_Variant::STRING) {
      REQUIRE(before.content.s);
      REQUIRE(after.content.s);
      CHECK(*before.content.s == *after.content.s);
    } else if (before.type() == DRW_Variant::INTEGER) {
      CHECK(before.i_val() == after.i_val());
    } else if (before.type() == DRW_Variant::INTEGER64) {
      CHECK(before.i64_val() == after.i64_val());
    } else if (before.type() == DRW_Variant::DOUBLE) {
      const double a = before.d_val(), b = after.d_val();
      CHECK(std::memcmp(&a, &b, sizeof(a)) == 0);
    } else {
      FAIL("unexpected material group type");
    }
  }
  if (const char *copy = std::getenv("LIBRECAD_KEEP_SPARSE_BINARY_REPLAY"))
    std::filesystem::copy_file(binaryReplay, copy, std::filesystem::copy_options::overwrite_existing);
  writeText(output, sentinel);
  auto &binaryRaw = const_cast<std::vector<DRW_RawDxfObject> &>(
      binaryGraphic.dwgAdvancedMetadata().rawDxfObjects());
  const auto binaryMaterial = std::find_if(binaryRaw.begin(), binaryRaw.end(),
      [](const auto &record) { return record.name == "MATERIAL" && record.handle == 0x1Fu; });
  REQUIRE(binaryMaterial != binaryRaw.end());
  binaryMaterial->groups.back().addInt(94, originalTail + 1);
  CHECK_FALSE(binaryImporter.fileExport(binaryGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(binaryImporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  binaryMaterial->groups.back().addInt(94, originalTail);
  const auto binaryDictionary = std::find_if(binaryRaw.begin(), binaryRaw.end(),
      [](const auto &record) { return record.name == "DICTIONARY" && record.handle == 0xEu; });
  REQUIRE(binaryDictionary != binaryRaw.end());
  const auto binaryEntry = std::find_if(binaryDictionary->groups.begin(), binaryDictionary->groups.end(),
      [](const auto &group) { return group.code() == 3; });
  REQUIRE(binaryEntry != binaryDictionary->groups.end());
  binaryEntry->addString(3, "changed");
  CHECK_FALSE(binaryImporter.fileExport(binaryGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(binaryImporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  binaryEntry->addString(3, "ByBlock");
  // Binary input has no raw lexeme array. A mutated typed string must still
  // be checked in full before the staged writer sees it.
  auto *binarySourceDictionary = const_cast<DRW_RawDxfObject *>(
      binaryGraphic.dwgAdvancedMetadata().dxfMaterialDictionarySource());
  REQUIRE(binarySourceDictionary); CHECK_FALSE(binarySourceDictionary->hasRawValues);
  const std::vector<std::pair<int, std::string>> binaryProbes = {
      {100, "AcDbDictionary"}, {3, "ByBlock"}, {350, "1F"}};
  for (const auto &probe : binaryProbes) {
    auto sourceGroup = std::find_if(binarySourceDictionary->groups.begin(), binarySourceDictionary->groups.end(),
        [&](const auto &group) { return group.code() == probe.first && group.c_str() && probe.second == group.c_str(); });
    auto liveGroup = std::find_if(binaryDictionary->groups.begin(), binaryDictionary->groups.end(),
        [&](const auto &group) { return group.code() == probe.first && group.c_str() && probe.second == group.c_str(); });
    REQUIRE(sourceGroup != binarySourceDictionary->groups.end()); REQUIRE(liveGroup != binaryDictionary->groups.end());
    const auto savedSource = *sourceGroup, savedLive = *liveGroup;
    const std::string malformed = probe.second + std::string("\0BAD", 4);
    sourceGroup->addString(probe.first, malformed); liveGroup->addString(probe.first, malformed);
    CHECK_FALSE(binaryImporter.fileExport(binaryGraphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
    CHECK(binaryImporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    *sourceGroup = savedSource; *liveGroup = savedLive;
    REQUIRE(binaryImporter.fileExport(binaryGraphic, QString::fromStdString(binaryReplay), RS2::FormatDXFRW2010));
  }
  CHECK_FALSE(binaryImporter.fileExport(binaryGraphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  REQUIRE(binaryImporter.fileExport(binaryGraphic, QString::fromStdString(binaryReplay), RS2::FormatDXFRW2010));
  std::filesystem::remove(binaryReplay);
  std::filesystem::remove(binarySource);
  std::filesystem::remove(layerSource);
  std::filesystem::remove(graphSource);
  graphic.dwgAdvancedMetadata().addDxfRawMaterialSource(*sourceSnapshot);
  CHECK_FALSE(graphic.dwgAdvancedMetadata().materials().front().rawDxfSource);
  CHECK(graphic.dwgAdvancedMetadata().materials().front().rawDxfSourceConflict);
  CHECK_FALSE(exporter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
  // The optional source-copy profile is bounded independently of normal
  // import. Trailing whitespace leaves the drawing intact but crosses the
  // existing 256 KiB file-capture limit.
  {
    std::ofstream append(source, std::ios::binary | std::ios::app);
    append << std::string(256u * 1024u, ' ');
    REQUIRE(append.good());
  }
#ifdef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
  RS_Graphic ordinaryOversized;
  RS_FilterDXFRW ordinaryOversizedImporter;
  CHECK_FALSE(ordinaryOversizedImporter.fileImport(ordinaryOversized, QString::fromStdString(source), RS2::FormatDXFRW));
#endif
  RS_Graphic oversized;
  OptInFilter oversizedImporter;
  REQUIRE(oversizedImporter.fileImport(oversized, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(oversized.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK(oversized.dwgAdvancedMetadata().materials().front().requiresRawDxfReplay);
  CHECK_FALSE(oversized.dwgAdvancedMetadata().materials().front().rawDxfSource);
  CHECK(oversized.dwgAdvancedMetadata().materials().front().rawDxfSourceConflict);
  CHECK_FALSE(exporter.fileExport(oversized, QString::fromStdString(output), RS2::FormatDXFRW2010));
  auto &oversizedMaterial = const_cast<LC_DwgAdvancedMetadata::MaterialRecord &>(
      oversized.dwgAdvancedMetadata().materials().front());
  oversizedMaterial.requiresRawDxfReplay = false;
  CHECK_FALSE(exporter.fileExport(oversized, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK(exporter.exportReport().contains(QStringLiteral("raw source missing, changed or unverified")));
  std::ifstream afterRefusals(output, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(afterRefusals), {}) == sentinel);
  afterRefusals.close();
  RS_Graphic clean;
  clean.initForNewDocument();
  REQUIRE(exporter.fileExport(clean, QString::fromStdString(output), RS2::FormatDXFRW2010));
  CHECK_FALSE(exporter.exportReport().contains(QStringLiteral("MATERIAL raw")));
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

TEST_CASE("Sparse material source snapshots have a bounded record count",
          "[system-codec][dxf][material][transaction]") {
  LC_DwgAdvancedMetadata metadata;
  for (std::uint32_t handle = 1; handle <= 4; ++handle) {
    DRW_Material material;
    material.handle = handle;
    metadata.addMaterial(material);
    auto &records = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord> &>(
        metadata.materials());
    records.back().requiresRawDxfReplay = true;
    DRW_RawDxfObject source;
    source.name = "MATERIAL";
    source.handle = handle;
    source.m_version = DRW::AC1024;
    metadata.addDxfRawMaterialSource(source);
    CHECK(static_cast<bool>(records.back().rawDxfSource) == (handle <= 3));
    CHECK(records.back().rawDxfSourceConflict == (handle > 3));
  }
  metadata.clear();
  for (unsigned index = 0; index != 2; ++index) {
    DRW_Material material;
    material.handle = 42;
    metadata.addMaterial(material);
    auto &records = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord> &>(
        metadata.materials());
    records.back().requiresRawDxfReplay = true;
    DRW_RawDxfObject source;
    source.name = "MATERIAL";
    source.handle = 42;
    source.m_version = DRW::AC1024;
    metadata.addDxfRawMaterialSource(source);
  }
  REQUIRE(metadata.materials().size() == 2u);
  for (const auto &record : metadata.materials()) {
    CHECK_FALSE(record.rawDxfSource);
    CHECK(record.rawDxfSourceConflict);
  }
  for (unsigned scenario = 0; scenario != 5; ++scenario) {
    metadata.clear();
    DRW_Material material;
    material.handle = scenario == 3 ? 0 : scenario == 4 ? DRW::NoHandle : 9;
    metadata.addMaterial(material);
    auto &record = const_cast<LC_DwgAdvancedMetadata::MaterialRecord &>(metadata.materials().back());
    record.requiresRawDxfReplay = true;
    DRW_RawDxfObject source;
    source.name = "MATERIAL";
    source.handle = material.handle;
    source.m_version = scenario == 0 ? DRW::UNKNOWNV : DRW::AC1024;
    if (scenario == 1)
      for (unsigned i = 0; i != 65; ++i)
        source.groups.emplace_back(70, static_cast<int>(i));
    if (scenario == 2) {
      source.hasRawValues = true;
      source.groups.emplace_back(70, 0);
    }
    metadata.addDxfRawMaterialSource(source);
    CHECK_FALSE(record.rawDxfSource);
    CHECK(record.rawDxfSourceConflict);
  }
}

TEST_CASE("Raster source frames are bounded and selected by handle",
          "[system-codec][dxf][image][transaction]") {
  LC_DwgAdvancedMetadata metadata;
  DRW_RawDxfObject image;
  image.name = "IMAGE";
  image.m_version = DRW::AC1021;
  image.handle = 0x31;
  metadata.addDxfImageSource(image);
  REQUIRE(metadata.dxfImageSource());
  CHECK(metadata.dxfImageSource(0x31)->handle == 0x31);
  image.handle = 0x32;
  metadata.addDxfImageSource(image);
  CHECK_FALSE(metadata.dxfImageSource()); // no ambiguous singleton fallback
  REQUIRE(metadata.dxfImageSource(0x32));
  CHECK(metadata.dxfImageSource(0x31)->handle == 0x31);
  CHECK_FALSE(metadata.dxfImageSource(0x33));
  DRW_RawDxfObject definition;
  definition.name = "IMAGEDEF";
  definition.m_version = DRW::AC1021;
  definition.handle = 0x41;
  metadata.addDxfImageDefSource(definition);
  definition.handle = 0x42;
  metadata.addDxfImageDefSource(definition);
  REQUIRE(metadata.dxfImageDefSource(0x41));
  REQUIRE(metadata.dxfImageDefSource(0x42));
  CHECK_FALSE(metadata.dxfImageDefSource());
  DRW_RawDxfObject root;
  root.name = "DICTIONARY";
  root.handle = 0xC;
  metadata.addDxfRootDictionarySource(root);
  metadata.addDxfImageDefSource(definition); // duplicate identity poisons capture
  CHECK_FALSE(metadata.dxfImageDefSource(0x41));
  CHECK_FALSE(metadata.dxfImageSource(0x31));
  CHECK(metadata.dxfRootDictionarySource()); // unrelated dictionary provenance survives
  metadata.clear();
  image.handle = 1;
  for (std::uint32_t handle = 1; handle <= 256; ++handle) {
    image.handle = handle;
    metadata.addDxfImageSource(image);
    REQUIRE(metadata.dxfImageSource(handle));
  }
  image.handle = 257;
  metadata.addDxfImageSource(image);
  CHECK_FALSE(metadata.dxfImageSource(1));
  metadata.clear();
  image.handle = 0;
  metadata.addDxfImageSource(image);
  image.handle = 1;
  metadata.addDxfImageSource(image);
  CHECK_FALSE(metadata.dxfImageSource(1));
  for (unsigned scenario = 0; scenario != 4; ++scenario) {
    metadata.clear();
    image = DRW_RawDxfObject{};
    image.name = scenario == 0 ? "IMAGEDEF" : "IMAGE";
    image.handle = 1;
    image.m_version = scenario == 1 ? DRW::UNKNOWNV : DRW::AC1021;
    if (scenario == 2) {
      image.hasRawValues = true;
      image.groups.emplace_back(70, 0);
    }
    if (scenario == 3)
      image.groups.emplace_back(1, std::string(64u * 1024u, 'x'));
    metadata.addDxfImageSource(image);
    CHECK_FALSE(metadata.dxfImageSource(1));
  }
  metadata.clear();
  image = DRW_RawDxfObject{};
  image.name = "IMAGE";
  image.handle = 1;
  image.m_version = DRW::AC1021;
  metadata.addDxfImageSource(image);
  REQUIRE(metadata.dxfImageSource(1)); // corrected retry after a capture failure

  // Exercise the parser callback for more than one physical IMAGE frame.
  ensureSettings();
  const auto input = tmpFile("multi_image_source_capture.dxf");
  const auto imageFrame = [](const char *handle) {
    return std::string("0\nIMAGE\n5\n") + handle +
        "\n8\n0\n100\nAcDbRasterImage\n90\n0\n"
        "10\n1\n20\n2\n30\n3\n11\n1\n21\n0\n31\n0\n"
        "12\n0\n22\n1\n32\n0\n13\n16\n23\n12\n340\n1\n"
        "70\n3\n280\n1\n281\n50\n282\n50\n283\n0\n"
        "71\n1\n91\n2\n14\n-0.5\n24\n-0.5\n14\n15.5\n24\n11.5\n";
  };
  writeText(input, "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
                   "0\nSECTION\n2\nENTITIES\n" + imageFrame("31") + imageFrame("32") +
                   "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(input), RS2::FormatDXFRW));
  const auto &captured = graphic.dwgAdvancedMetadata();
  REQUIRE(captured.dxfImageSource(0x31));
  REQUIRE(captured.dxfImageSource(0x32));
  CHECK_FALSE(captured.dxfImageSource());
  std::filesystem::remove(input);

  const auto definitionsInput = tmpFile("multi_image_definition_capture.dxf");
  const auto definitionFrame = [](const char *handle, const char *name) {
    return std::string("0\nIMAGEDEF\n5\n") + handle +
        "\n330\nE\n100\nAcDbRasterImageDef\n90\n0\n1\n" + name +
        "\n10\n16\n20\n12\n11\n1\n21\n1\n280\n1\n281\n0\n";
  };
  writeText(definitionsInput,
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_IMAGE_DICT\n350\nE\n"
      "0\nDICTIONARY\n5\nE\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nfirst\n350\n41\n3\nsecond\n350\n42\n" +
      definitionFrame("41", "first.png") + definitionFrame("42", "second.png") +
      "0\nENDSEC\n0\nEOF\n");
  RS_Graphic definitionsGraphic;
  RS_FilterDXFRW definitionsFilter;
  REQUIRE(definitionsFilter.fileImport(definitionsGraphic, QString::fromStdString(definitionsInput),
                                       RS2::FormatDXFRW));
  const auto &definitionSources = definitionsGraphic.dwgAdvancedMetadata();
  REQUIRE(definitionSources.dxfImageDefSource(0x41));
  REQUIRE(definitionSources.dxfImageDefSource(0x42));
  CHECK_FALSE(definitionSources.dxfImageDefSource());
  std::filesystem::remove(definitionsInput);
}

TEST_CASE("System codec preserves bounded multi-image definition graphs",
          "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto format = GENERATE(RS2::FormatDXFRW2000, RS2::FormatDXFRW2004,
                               RS2::FormatDXFRW, RS2::FormatDXFRW2010,
                               RS2::FormatDXFRW2013, RS2::FormatDXFRW2018);
  CAPTURE(format);
  const int shape = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
  const bool withMaterial = GENERATE(false, true);
  // A small mixed graph exercises the composed IMAGE/MATERIAL inspectors
  // without running the full size/version matrix during focused development.
  if (std::getenv("LIBRECAD_FAST_MIXED_IMAGE_MATERIAL") &&
      (format != RS2::FormatDXFRW2010 || shape != 1 || !withMaterial))
    return;
  const bool legacyMaterial = withMaterial &&
      (format == RS2::FormatDXFRW2000 || format == RS2::FormatDXFRW2004);
  const unsigned imageCount = shape < 2 ? 2u : shape < 5 ? 3u : shape == 5 ? 4u : shape == 6 ? 32u : shape == 7 ? 64u : shape == 8 ? 256u : 384u;
  const unsigned definitionCount = shape == 0 || shape == 4 ? 1u :
                                   shape == 3 || shape >= 5 ? 3u : 2u;
  const bool sharedDefinition = definitionCount == 1u;
  const auto source = tmpFile("two_images_one_definition_source.dxf");
  const auto output = tmpFile("two_images_one_definition_output.dxf");
  RS_Graphic original;
  original.initForNewDocument();
  for (unsigned i = 0; i < imageCount; ++i) {
    const double x = 1.0 + 29.0 * i;
    const auto path = i == 0 || shape == 4 || (i == 1 && shape == 0) || (i == 2 && shape == 2)
        ? QStringLiteral("two-image.png")
        : i == 1 ? QStringLiteral("second-image.png") : QStringLiteral("third-image.png");
    auto *image = new RS_Image(&original, RS_ImageData(0, RS_Vector(x, 2), RS_Vector(1, 0),
        RS_Vector(0, 1), RS_Vector(16, 12), path,
        50, 50, 0));
    original.addEntity(image);
  }
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  REQUIRE(countRecords(source, "IMAGE") == static_cast<int>(imageCount));
  REQUIRE(countRecords(source, "IMAGEDEF") == static_cast<int>(definitionCount));
  REQUIRE(countRecords(source, "IMAGEDEF_REACTOR") == static_cast<int>(imageCount));
  // Give the locally generated source nonzero WCS Z coordinates in all
  // IMAGE frames so the admitted profile also exercises the 3D sidecar.
  {
    std::ifstream input(source, std::ios::binary);
    std::string code, value, rewritten;
    bool image = false, sectionStart = false, root = false;
    std::string section;
    unsigned changed = 0;
    while (std::getline(input, code) && std::getline(input, value)) {
      const int group = std::stoi(code);
      if (withMaterial && group == 0 && root) {
        rewritten += "3\nACAD_MATERIAL\n350\nF700\n";
        root = false;
      }
      if (group == 0 && value == "SECTION") sectionStart = true;
      else if (sectionStart && group == 2) { section = value; sectionStart = false; }
      if (withMaterial && group == 0 && value == "ENDSEC" && section == "ENTITIES")
        rewritten += "0\nLINE\n5\nF701\n100\nAcDbEntity\n8\n0\n347\nF702\n"
                     "100\nAcDbLine\n10\n0\n20\n0\n30\n0\n11\n1\n21\n0\n31\n0\n";
      if (withMaterial && group == 0 && value == "ENDSEC" && section == "OBJECTS")
        rewritten += "0\nDICTIONARY\n5\nF700\n330\nC\n100\nAcDbDictionary\n281\n1\n"
                     "3\nPlainMat\n350\nF702\n0\nMATERIAL\n5\nF702\n330\nF700\n"
                     "100\nAcDbMaterial\n1\nPlainMat\n";
      if (withMaterial && section == "OBJECTS" && group == 5 && value == "C") root = true;
      if (group == 0) image = value == "IMAGE";
      if (image && (group == 30 || group == 31 || group == 32)) {
        value = group == 30 ? "3" : group == 31 ? "4" : "5";
        ++changed;
      }
      rewritten += code + "\n" + value + "\n";
    }
    REQUIRE(changed == 3u * imageCount);
    writeText(source, rewritten);
  }
  if (const char *copy = std::getenv("LIBRECAD_KEEP_TWO_IMAGE_SOURCE")) {
    const std::string path = std::string(copy) + ".shape" + std::to_string(shape) +
                             (withMaterial ? ".material" : "") +
                             (format == RS2::FormatDXFRW ? "" : ".v" + std::to_string(static_cast<int>(format))) + ".dxf";
    std::filesystem::copy_file(source, path, std::filesystem::copy_options::overwrite_existing);
  }
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(source), format));
  if (legacyMaterial) {
    const std::string sentinel = "preserve legacy material destination";
    writeText(output, sentinel);
    CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    REQUIRE(filter.fileExport(original, QString::fromStdString(output), format));
    CHECK(countRecords(output, "IMAGE") == static_cast<int>(imageCount));
    std::filesystem::remove(source);
    std::filesystem::remove(output);
    return;
  }
  const auto &metadata = reopened.dwgAdvancedMetadata();
  REQUIRE(metadata.rasterImages().size() == imageCount);
  REQUIRE(metadata.imageDefinitions().size() == definitionCount);
  REQUIRE(metadata.imageDefinitionReactors().size() == imageCount);
  for (const auto &image : metadata.rasterImages()) REQUIRE(metadata.dxfImageSource(image.handle));
  for (const auto &definition : metadata.imageDefinitions())
    REQUIRE(metadata.dxfImageDefSource(definition.handle));
  const std::string sentinel = "preserve two-image destination";
  auto &reactors = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionReactorRecord>&>(
      metadata.imageDefinitionReactors());
  const auto parent = reactors.front().parentHandle;
  ++reactors.front().parentHandle;
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  reactors.front().parentHandle = parent;
  auto &rawObjects = const_cast<std::vector<DRW_RawDxfObject>&>(metadata.rawDxfObjects());
  auto plot = std::find_if(rawObjects.begin(), rawObjects.end(),
      [](const auto &record) { return record.name == "ACDBDICTIONARYWDFLT"; });
  REQUIRE(plot != rawObjects.end());
  plot->groups.emplace_back(70, 7);
  if (plot->hasRawValues) plot->rawValues.push_back("7");
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  plot->groups.pop_back();
  if (plot->hasRawValues) plot->rawValues.pop_back();
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(output), format));
  if (withMaterial) {
    const auto material = recordGroupValues(output, "MATERIAL", "5");
    const auto lineMaterial = recordGroupValues(output, "LINE", "347");
    REQUIRE(material.size() == 1u);
    CHECK(lineMaterial == material);
  }
  CHECK(countRecords(output, "IMAGE") == static_cast<int>(imageCount));
  CHECK(countRecords(output, "IMAGEDEF") == static_cast<int>(definitionCount));
  CHECK(countRecords(output, "IMAGEDEF_REACTOR") == static_cast<int>(imageCount));
  for (const char *code : {"90", "10", "20", "30", "11", "21", "31", "12", "22", "32", "13", "23",
                           "70", "280", "281", "282", "283", "71", "91", "14", "24"})
    CHECK(recordGroupValues(output, "IMAGE", code) == recordGroupValues(source, "IMAGE", code));
  for (const char *code : {"90", "1", "10", "20", "11", "21", "280", "281"})
    CHECK(recordGroupValues(output, "IMAGEDEF", code) == recordGroupValues(source, "IMAGEDEF", code));
  RS_Graphic readback;
  REQUIRE(filter.fileImport(readback, QString::fromStdString(output), format));
  const auto &result = readback.dwgAdvancedMetadata();
  REQUIRE(result.rasterImages().size() == imageCount);
  REQUIRE(result.imageDefinitions().size() == definitionCount);
  REQUIRE(result.imageDefinitionReactors().size() == imageCount);
  std::set<std::uint32_t> resultDefinitions, resultReactors;
  for (const auto &image : result.rasterImages()) {
    resultDefinitions.insert(image.definitionHandle);
    resultReactors.insert(image.reactorHandle);
  }
  CHECK(resultDefinitions.size() == definitionCount);
  CHECK(resultReactors.size() == imageCount);
  {
    struct MutatingFilter : RS_FilterDXFRW {
      std::function<void()> beforeFinalize;
      bool finalizeDxfWrite() override {
        if (beforeFinalize) beforeFinalize();
        return RS_FilterDXFRW::finalizeDxfWrite();
      }
    } late;
    RS_Image *image = nullptr, *other = nullptr;
    for (RS_Entity *entity : lc::LC_ContainerTraverser{reopened, RS2::ResolveNone}.entities())
      if (entity->rtti() == RS2::EntityImage) {
        if (!image) image = static_cast<RS_Image *>(entity);
        else if (!other) other = static_cast<RS_Image *>(entity);
      }
    REQUIRE(image);
    REQUIRE(other);
    if (withMaterial) {
      auto &materials = const_cast<std::vector<LC_DwgAdvancedMetadata::MaterialRecord>&>(metadata.materials());
      REQUIRE(materials.size() == 1u);
      const auto description = materials.front().description;
      late.beforeFinalize = [&] { materials.front().description = "late mixed-graph edit"; };
      writeText(output, sentinel);
      CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
      { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
      materials.front().description = description;
      late.beforeFinalize = {};
      REQUIRE(late.fileExport(reopened, QString::fromStdString(output), format));
    }
    const auto firstPath = image->getFile(), secondPath = other->getFile();
    image->setFile(QStringLiteral("unsupported^path.png"));
    other->setFile(QStringLiteral("unsupported^path.png"));
    writeText(output, sentinel);
    CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    image->setFile(firstPath);
    other->setFile(secondPath);
    const auto position = image->getInsertionPoint();
    late.beforeFinalize = [&] { image->setInsertionPoint(position + RS_Vector(1, 0)); };
    writeText(output, sentinel);
    CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    image->setInsertionPoint(position);
    REQUIRE(image->hasDrwExtData());
    const auto originalSidecar = image->getDrwExtData();
    REQUIRE(originalSidecar.size() == 4u);
    REQUIRE(originalSidecar[1]);
    REQUIRE(originalSidecar[1]->coord());
    const auto savedPoint = *originalSidecar[1]->coord();
    late.beforeFinalize = [&] {
      auto changed = originalSidecar;
      changed[1] = std::make_shared<DRW_Variant>(1010, DRW_Coord(savedPoint.x, savedPoint.y, savedPoint.z + 1));
      image->setDrwExtData(std::move(changed));
    };
    writeText(output, sentinel);
    CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    image->setDrwExtData(originalSidecar);
    const auto originalPen = image->getPen(false);
    auto changedPen = originalPen;
    changedPen.setWidth(RS2::Width20);
    late.beforeFinalize = [&] { image->setPen(changedPen); };
    writeText(output, sentinel);
    CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    image->setPen(originalPen);
    // Mutate both typed and captured source brightness together after staged
    // output, so recomputing only typed/raw agreement would miss the edit.
    auto &rasters = const_cast<std::vector<LC_DwgAdvancedMetadata::RasterImageRecord>&>(metadata.rasterImages());
    auto raster = std::find_if(rasters.begin(), rasters.end(), [&](const auto &record) {
      return record.handle == image->sourceHandle();
    });
    REQUIRE(raster != rasters.end());
    auto *sourceImage = const_cast<DRW_RawDxfObject *>(metadata.dxfImageSource(raster->handle));
    REQUIRE(sourceImage);
    auto brightnessGroup = std::find_if(sourceImage->groups.begin(), sourceImage->groups.end(),
        [](const auto &group) { return group.code() == 281; });
    REQUIRE(brightnessGroup != sourceImage->groups.end());
    const auto brightnessIndex = static_cast<std::size_t>(brightnessGroup - sourceImage->groups.begin());
    REQUIRE((!sourceImage->hasRawValues || brightnessIndex < sourceImage->rawValues.size()));
    const int originalBrightness = raster->brightness;
    const std::string originalRaw = sourceImage->hasRawValues ? sourceImage->rawValues[brightnessIndex] : std::string{};
    late.beforeFinalize = [&] {
      raster->brightness = originalBrightness + 1;
      brightnessGroup->addInt(281, raster->brightness);
      if (sourceImage->hasRawValues) sourceImage->rawValues[brightnessIndex] = std::to_string(raster->brightness);
    };
    writeText(output, sentinel);
    CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    raster->brightness = originalBrightness;
    brightnessGroup->addInt(281, originalBrightness);
    if (sourceImage->hasRawValues) sourceImage->rawValues[brightnessIndex] = originalRaw;
    if (!sharedDefinition) {
      auto &definitions = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionRecord>&>(metadata.imageDefinitions());
      auto &definition = definitions.back();
      auto *sourceDefinition = const_cast<DRW_RawDxfObject *>(metadata.dxfImageDefSource(definition.handle));
      REQUIRE(sourceDefinition);
      auto pathGroup = std::find_if(sourceDefinition->groups.begin(), sourceDefinition->groups.end(),
          [](const auto &group) { return group.code() == 1; });
      REQUIRE(pathGroup != sourceDefinition->groups.end());
      const auto index = static_cast<std::size_t>(pathGroup - sourceDefinition->groups.begin());
      REQUIRE((!sourceDefinition->hasRawValues || index < sourceDefinition->rawValues.size()));
      const auto originalPath = definition.path;
      const auto rawPath = sourceDefinition->hasRawValues ? sourceDefinition->rawValues[index] : std::string{};
      late.beforeFinalize = [&] {
        definition.path = "changed-source.png";
        pathGroup->addString(1, definition.path);
        if (sourceDefinition->hasRawValues) sourceDefinition->rawValues[index] = definition.path;
      };
      writeText(output, sentinel);
      CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
      { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
      definition.path = originalPath;
      pathGroup->addString(1, originalPath);
      if (sourceDefinition->hasRawValues) sourceDefinition->rawValues[index] = rawPath;
    }
    late.beforeFinalize = {};
    REQUIRE(late.fileExport(reopened, QString::fromStdString(output), format));
  }
  if (const char *copy = std::getenv("LIBRECAD_KEEP_TWO_IMAGE_OUTPUT")) {
    const std::string path = std::string(copy) + ".shape" + std::to_string(shape) +
                             (withMaterial ? ".material" : "") +
                             (format == RS2::FormatDXFRW ? "" : ".v" + std::to_string(static_cast<int>(format))) + ".dxf";
    std::filesystem::copy_file(output, path, std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

TEST_CASE("System codec preserves modern multi-image clip modes",
          "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto format = GENERATE(RS2::FormatDXFRW2010, RS2::FormatDXFRW2013,
                               RS2::FormatDXFRW2018);
  const bool polygon = GENERATE(false, true);
  const unsigned imageCount = GENERATE(1u, 3u);
  if (imageCount == 1u && !polygon) return;
  CAPTURE(format, polygon, imageCount);
  const auto source = tmpFile("modern_images_source.dxf");
  const auto output = tmpFile("modern_images_output.dxf");
  RS_Graphic original;
  original.initForNewDocument();
  for (unsigned i = 0; i < imageCount; ++i)
    original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12),
        i < 2 ? QStringLiteral("shared.png") : QStringLiteral("other.png"), 50, 50, 0)));
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  {
    std::ifstream input(source, std::ios::binary);
    std::string code, value, rewritten;
    bool image = false;
    unsigned changed = 0, vertices = 0;
    while (std::getline(input, code) && std::getline(input, value)) {
      const int group = std::stoi(code);
      if (group == 0) image = value == "IMAGE";
      if (image && group == 70 && changed == 0) value = "7";
      if (image && group == 280 && changed == 0) value = "1";
      if (image && group == 71 && changed == 0 && polygon) value = "2";
      if (image && group == 91 && changed == 0 && polygon) value = "3";
      if (image && (group == 14 || group == 24) && changed == 0) {
        const char *coordinates[] = {"2", "3", "10", "9"};
        REQUIRE(vertices < 4u);
        value = coordinates[vertices++];
      }
      if (image && group == 290 && changed == 0) {
        if (polygon) rewritten += "14\n8\n24\n10\n";
        value = "1"; ++changed;
      }
      rewritten += code + "\n" + value + "\n";
    }
    REQUIRE(changed == 1u);
    REQUIRE(vertices == 4u);
    writeText(source, rewritten);
  }
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(source), format));
  const auto &metadata = reopened.dwgAdvancedMetadata();
  REQUIRE(metadata.rasterImages().size() == imageCount);
  REQUIRE(metadata.imageDefinitions().size() == (imageCount == 1u ? 1u : 2u));
  REQUIRE(metadata.imageDefinitionReactors().size() == imageCount);
  CHECK(metadata.rasterImages().front().displayProps == 7);
  CHECK(metadata.rasterImages().front().clip == 1);
  CHECK(metadata.rasterImages().front().clipMode);
  CHECK(metadata.rasterImages().front().clipBoundaryType == (polygon ? 2 : 1));
  REQUIRE(metadata.rasterImages().front().clipPath.size() == (polygon ? 3u : 2u));
  const std::string sentinel = "preserve modern image destination";
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  auto &rasters = const_cast<std::vector<LC_DwgAdvancedMetadata::RasterImageRecord>&>(metadata.rasterImages());
  auto *sourceImage = const_cast<DRW_RawDxfObject *>(metadata.dxfImageSource(rasters.front().handle));
  REQUIRE(sourceImage);
  auto mode = std::find_if(sourceImage->groups.begin(), sourceImage->groups.end(),
      [](const auto &group) { return group.code() == 290; });
  REQUIRE(mode != sourceImage->groups.end());
  const auto index = static_cast<std::size_t>(mode - sourceImage->groups.begin());
  REQUIRE((!sourceImage->hasRawValues || index < sourceImage->rawValues.size()));
  struct MutatingFilter : RS_FilterDXFRW {
    std::function<void()> beforeFinalize;
    bool finalizeDxfWrite() override {
      if (beforeFinalize) beforeFinalize();
      return RS_FilterDXFRW::finalizeDxfWrite();
    }
  } late;
  const auto originalRaw = sourceImage->hasRawValues ? sourceImage->rawValues[index] : std::string{};
  late.beforeFinalize = [&] {
    rasters.front().clipMode = false;
    mode->addInt(290, 0);
    if (sourceImage->hasRawValues) sourceImage->rawValues[index] = "0";
  };
  writeText(output, sentinel);
  CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  rasters.front().clipMode = true;
  mode->addInt(290, 1);
  if (sourceImage->hasRawValues) sourceImage->rawValues[index] = originalRaw;
  if (polygon) {
    auto point = std::find_if(sourceImage->groups.rbegin(), sourceImage->groups.rend(),
        [](const auto &group) { return group.code() == 14; });
    REQUIRE(point != sourceImage->groups.rend());
    const auto pointIndex = static_cast<std::size_t>(std::prev(point.base()) - sourceImage->groups.begin());
    REQUIRE((!sourceImage->hasRawValues || pointIndex < sourceImage->rawValues.size()));
    const double originalX = rasters.front().clipPath.back().x;
    const auto rawPoint = sourceImage->hasRawValues ? sourceImage->rawValues[pointIndex] : std::string{};
    late.beforeFinalize = [&] {
      rasters.front().clipPath.back().x = originalX + 1;
      point->addDouble(14, originalX + 1);
      if (sourceImage->hasRawValues) sourceImage->rawValues[pointIndex] = std::to_string(originalX + 1);
    };
    writeText(output, sentinel);
    CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    rasters.front().clipPath.back().x = originalX;
    point->addDouble(14, originalX);
    if (sourceImage->hasRawValues) sourceImage->rawValues[pointIndex] = rawPoint;
  }
  late.beforeFinalize = {};
  REQUIRE(late.fileExport(reopened, QString::fromStdString(output), format));
  for (const char *code : {"70", "280", "290", "71", "91", "14", "24"})
    CHECK(recordGroupValues(output, "IMAGE", code) == recordGroupValues(source, "IMAGE", code));
  RS_Graphic readback;
  REQUIRE(filter.fileImport(readback, QString::fromStdString(output), format));
  REQUIRE(readback.dwgAdvancedMetadata().rasterImages().size() == imageCount);
  CHECK(readback.dwgAdvancedMetadata().rasterImages().front().clipMode);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_MODERN_IMAGE")) {
    const auto prefix = std::string(copy) + "." + std::to_string(static_cast<int>(format)) +
                        (polygon ? ".polygon" : ".rectangle") + "." + std::to_string(imageCount);
    std::filesystem::copy_file(source, prefix + ".source.dxf", std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(output, prefix + ".output.dxf", std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

TEST_CASE("System codec preserves larger polygon image boundaries", "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto source = tmpFile("large_polygon_image_source.dxf"), output = tmpFile("large_polygon_image_output.dxf");
  const unsigned imageCount = GENERATE(1u, 2u, 384u, 512u);
  CAPTURE(imageCount);
  RS_Graphic original; original.initForNewDocument();
  for (unsigned i = 0; i < imageCount; ++i)
    original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12), QStringLiteral("polygon.png"), 50, 50, 0)));
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), RS2::FormatDXFRW2010));
  std::ifstream originalSource(source, std::ios::binary);
  const std::string baseline(std::istreambuf_iterator<char>(originalSource), {});
  const auto makeSource = [&](unsigned count) {
    std::istringstream input(baseline);
    std::string code, value, rewritten;
    bool image = false, inserted = false;
    unsigned imageOrdinal = 0;
    while (std::getline(input, code) && std::getline(input, value)) {
      const int group = std::stoi(code);
      if (group == 0) { image = value == "IMAGE" && imageOrdinal++ == 0u; }
      if (image && group == 70) value = "7";
      if (image && group == 280) value = "1";
      if (image && group == 71) value = "2";
      if (image && group == 91) value = std::to_string(count);
      if (image && (group == 14 || group == 24)) {
        if (!inserted) {
          for (unsigned i = 0; i < count; ++i) {
            const double angle = 2.0 * M_PI * i / count;
            rewritten += "14\n" + std::to_string(8.0 + 4.0 * std::cos(angle)) +
                         "\n24\n" + std::to_string(6.0 + 4.0 * std::sin(angle)) + "\n";
          }
          inserted = true;
        }
        continue;
      }
      rewritten += code + "\n" + value + "\n";
    }
    REQUIRE(inserted); return rewritten;
  };
  const std::string sentinel = "preserve large polygon destination\n";
  for (const unsigned count : {18u, 256u, 257u}) {
    CAPTURE(count);
    writeText(source, makeSource(count)); writeText(output, sentinel);
    RS_Graphic graphic;
    const bool imported = filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW2010);
    if (count > 256u) {
      if (imported) CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      writeText(source, makeSource(256)); RS_Graphic corrected;
      REQUIRE(filter.fileImport(corrected, QString::fromStdString(source), RS2::FormatDXFRW2010));
      REQUIRE(filter.fileExport(corrected, QString::fromStdString(output), RS2::FormatDXFRW2010));
      continue;
    }
    REQUIRE(imported);
    auto &metadata = graphic.dwgAdvancedMetadata();
    auto &rasters = const_cast<std::vector<LC_DwgAdvancedMetadata::RasterImageRecord>&>(metadata.rasterImages());
    REQUIRE(rasters.size() == imageCount); REQUIRE(rasters.front().clipPath.size() == count);
    auto *liveImage = dynamic_cast<RS_Image *>(graphic.firstEntity());
    REQUIRE(liveImage); REQUIRE(liveImage->clipData().present);
    CHECK(liveImage->clipData().clipping == 1);
    CHECK(liveImage->clipData().boundaryType == 2);
    REQUIRE(liveImage->clipData().vertices.size() == count);
    for (unsigned i = 0; i < count; ++i) {
      CHECK(liveImage->clipData().vertices[i].x == rasters.front().clipPath[i].x);
      CHECK(liveImage->clipData().vertices[i].y == rasters.front().clipPath[i].y);
    }
    auto *raw = const_cast<DRW_RawDxfObject *>(metadata.dxfImageSource(rasters.front().handle)); REQUIRE(raw);
    if (imageCount == 1u) {
      auto changedClip = liveImage->clipData();
      changedClip.vertices.back().x += 1.0;
      liveImage->setClipData(changedClip);
      CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      changedClip.vertices.back().x -= 1.0;
      liveImage->setClipData(changedClip);
      auto &reactors = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionReactorRecord>&>(metadata.imageDefinitionReactors());
      auto reactor = std::find_if(reactors.begin(), reactors.end(), [&](const auto &value) {
        return value.handle == rasters.front().reactorHandle;
      });
      REQUIRE(reactor != reactors.end());
      const auto originalOwner = reactor->parentHandle;
      reactor->parentHandle = 0;
      CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
      reactor->parentHandle = originalOwner;
      CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2018));
      { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    }
    auto point = std::find_if(raw->groups.rbegin(), raw->groups.rend(), [](const auto &group) { return group.code() == 14; });
    REQUIRE(point != raw->groups.rend());
    const auto index = static_cast<std::size_t>(std::prev(point.base()) - raw->groups.begin());
    const auto oldX = rasters.front().clipPath.back().x;
    const auto oldRaw = raw->hasRawValues ? raw->rawValues.at(index) : std::string{};
    struct LatePolygonFilter : RS_FilterDXFRW {
      std::function<void()> beforeFinalize;
      bool finalizeDxfWrite() override { if (beforeFinalize) beforeFinalize(); return RS_FilterDXFRW::finalizeDxfWrite(); }
    } late;
    late.beforeFinalize = [&] {
      rasters.front().clipPath.back().x = oldX + 1.0; point->addDouble(14, oldX + 1.0);
      if (raw->hasRawValues) raw->rawValues[index] = std::to_string(oldX + 1.0);
    };
    CHECK_FALSE(late.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
    { std::ifstream retained(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    rasters.front().clipPath.back().x = oldX; point->addDouble(14, oldX);
    if (raw->hasRawValues) raw->rawValues[index] = oldRaw;
    late.beforeFinalize = {};
    REQUIRE(late.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW2010));
    RS_Graphic reread; REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW2010));
    const auto &replayed = reread.dwgAdvancedMetadata().rasterImages();
    REQUIRE(replayed.size() == imageCount); REQUIRE(replayed.front().clipPath.size() == count);
    for (unsigned i = 0; i < count; ++i) {
      CHECK(replayed.front().clipPath[i].x == rasters.front().clipPath[i].x);
      CHECK(replayed.front().clipPath[i].y == rasters.front().clipPath[i].y);
    }
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_LARGE_IMAGE_POLYGON")) {
      std::filesystem::copy_file(source, std::string(prefix) + std::to_string(imageCount) + "-" + std::to_string(count) + "-source.dxf", std::filesystem::copy_options::overwrite_existing);
      std::filesystem::copy_file(output, std::string(prefix) + std::to_string(imageCount) + "-" + std::to_string(count) + "-output.dxf", std::filesystem::copy_options::overwrite_existing);
    }
  }
}

TEST_CASE("System codec preserves legacy polygonal image boundaries",
          "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto format = GENERATE(RS2::FormatDXFRW2000, RS2::FormatDXFRW2004,
                               RS2::FormatDXFRW);
  const unsigned imageCount = GENERATE(1u, 2u);
  CAPTURE(format, imageCount);
  const auto source = tmpFile("legacy_polygon_images_source.dxf");
  const auto output = tmpFile("legacy_polygon_images_output.dxf");
  RS_Graphic original;
  original.initForNewDocument();
  for (unsigned i = 0; i < imageCount; ++i)
    original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12),
        QStringLiteral("legacy-polygon.png"), 50, 50, 0)));
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  {
    std::ifstream input(source, std::ios::binary);
    std::string code, value, rewritten;
    bool image = false, changed = false;
    unsigned vertices = 0;
    while (std::getline(input, code) && std::getline(input, value)) {
      const int group = std::stoi(code);
      if (group == 0) image = value == "IMAGE";
      if (image && !changed) {
        if (group == 70) value = "7";
        if (group == 280) value = "1";
        if (group == 71) value = "2";
        if (group == 91) value = "3";
        if (group == 14 || group == 24) {
          const char *coordinates[] = {"2", "3", "10", "9"};
          REQUIRE(vertices < 4u);
          value = coordinates[vertices++];
        }
      }
      rewritten += code + "\n" + value + "\n";
      if (image && !changed && vertices == 4u) {
        rewritten += "14\n8\n24\n10\n";
        changed = true;
      }
    }
    REQUIRE(changed);
    writeText(source, rewritten);
  }
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(source), format));
  const auto &metadata = reopened.dwgAdvancedMetadata();
  REQUIRE(metadata.rasterImages().size() == imageCount);
  REQUIRE(metadata.rasterImages().front().clipPath.size() == 3u);
  CHECK(metadata.rasterImages().front().clipBoundaryType == 2);
  CHECK(metadata.rasterImages().front().clip == 1);
  auto &rasters = const_cast<std::vector<LC_DwgAdvancedMetadata::RasterImageRecord>&>(metadata.rasterImages());
  auto *sourceImage = const_cast<DRW_RawDxfObject *>(metadata.dxfImageSource(rasters.front().handle));
  REQUIRE(sourceImage);
  auto point = std::find_if(sourceImage->groups.rbegin(), sourceImage->groups.rend(),
      [](const auto &group) { return group.code() == 14; });
  REQUIRE(point != sourceImage->groups.rend());
  const auto index = static_cast<std::size_t>(std::prev(point.base()) - sourceImage->groups.begin());
  REQUIRE((!sourceImage->hasRawValues || index < sourceImage->rawValues.size()));
  const double originalX = rasters.front().clipPath.back().x;
  const auto rawPoint = sourceImage->hasRawValues ? sourceImage->rawValues[index] : std::string{};
  struct MutatingFilter : RS_FilterDXFRW {
    std::function<void()> beforeFinalize;
    bool finalizeDxfWrite() override {
      if (beforeFinalize) beforeFinalize();
      return RS_FilterDXFRW::finalizeDxfWrite();
    }
  } late;
  const std::string sentinel = "preserve legacy polygon destination";
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), RS2::FormatDXFRW2018));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  late.beforeFinalize = [&] {
    rasters.front().clipPath.back().x = originalX + 1;
    point->addDouble(14, originalX + 1);
    if (sourceImage->hasRawValues) sourceImage->rawValues[index] = std::to_string(originalX + 1);
  };
  writeText(output, sentinel);
  CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  rasters.front().clipPath.back().x = originalX;
  point->addDouble(14, originalX);
  if (sourceImage->hasRawValues) sourceImage->rawValues[index] = rawPoint;
  late.beforeFinalize = {};
  REQUIRE(late.fileExport(reopened, QString::fromStdString(output), format));
  for (const char *group : {"70", "280", "71", "91", "14", "24"})
    CHECK(recordGroupValues(output, "IMAGE", group) == recordGroupValues(source, "IMAGE", group));
  RS_Graphic readback;
  REQUIRE(filter.fileImport(readback, QString::fromStdString(output), format));
  REQUIRE(readback.dwgAdvancedMetadata().rasterImages().size() == imageCount);
  CHECK(readback.dwgAdvancedMetadata().rasterImages().front().clipPath.size() == 3u);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_LEGACY_POLYGON")) {
    const auto prefix = std::string(copy) + "." + std::to_string(static_cast<int>(format)) + "." + std::to_string(imageCount);
    std::filesystem::copy_file(source, prefix + ".source.dxf", std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(output, prefix + ".output.dxf", std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

TEST_CASE("System codec round trips 33 image graphs", "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto source = tmpFile("thirty_three_image_source.dxf");
  const auto output = tmpFile("thirty_three_image_output.dxf");
  RS_Graphic original; original.initForNewDocument();
  for (unsigned i = 0; i < 33u; ++i) {
    const auto path = QStringLiteral("bounded-image-%1.png").arg(i % 4u);
    original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12), path, 50, 50, 0)));
  }
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), RS2::FormatDXFRW));
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(source), RS2::FormatDXFRW));
  REQUIRE(reopened.dwgAdvancedMetadata().rasterImages().size() == 33u);
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(countRecords(output, "IMAGE") == 33);
  CHECK(countRecords(output, "IMAGEDEF") == 4);
  CHECK(countRecords(output, "IMAGEDEF_REACTOR") == 33);
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(reread.dwgAdvancedMetadata().rasterImages().size() == 33u);
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("Raster source capture applies a shared logical budget", "[system-codec][dxf][image][budget]") {
  LC_DwgAdvancedMetadata metadata;
  DRW_RawDxfObject source;
  source.m_version = DRW::AC1027; source.hasRawValues = true;
  source.groups.emplace_back(1, std::string(30000u, 'a'));
  source.rawValues.emplace_back(30000u, 'b');
  unsigned completePairs = 0; bool rejected = false;
  for (unsigned i = 0; i < 40u; ++i) {
    source.name = "IMAGE"; source.handle = 100u + i;
    metadata.addDxfImageSource(source);
    if (!metadata.dxfImageSource(source.handle)) { rejected = true; break; }
    source.name = "IMAGEDEF"; source.handle = 1000u + i;
    metadata.addDxfImageDefSource(source);
    if (!metadata.dxfImageDefSource(source.handle)) { rejected = true; break; }
    ++completePairs;
  }
  CHECK(rejected); CHECK(completePairs > 32u); CHECK(completePairs < 40u);
  CHECK(metadata.dxfImageSource(100u) == nullptr);
  CHECK(metadata.dxfImageDefSource(1000u) == nullptr);
  metadata.clear();
  source.groups.clear(); source.rawValues.clear();
  source.groups.emplace_back(1, std::string("small")); source.rawValues.emplace_back("small");
  for (unsigned i = 0; i < LC_DwgAdvancedMetadata::MaxDxfRasterSourcesPerKind; ++i) {
    source.name = "IMAGE"; source.handle = 100u + i; metadata.addDxfImageSource(source);
    source.name = "IMAGEDEF"; source.handle = 1000u + i; metadata.addDxfImageDefSource(source);
    REQUIRE(metadata.dxfImageSource(100u + i)); REQUIRE(metadata.dxfImageDefSource(1000u + i));
  }
  source.name = "IMAGE"; source.handle = 100u + LC_DwgAdvancedMetadata::MaxDxfRasterSourcesPerKind; metadata.addDxfImageSource(source);
  CHECK(metadata.dxfImageSource(100u) == nullptr);
  CHECK(metadata.dxfImageDefSource(1000u) == nullptr);
  metadata.clear();
  source.handle = 100u; metadata.addDxfImageSource(source);
  CHECK(metadata.dxfImageSource(100u) != nullptr);
  metadata.clear();
  source.name = "IMAGEDEF"; source.handle = 1000u;
  source.groups.assign(LC_DwgAdvancedMetadata::MaxDxfImageDefSourceGroups, DRW_Variant(90, 0));
  source.rawValues.assign(LC_DwgAdvancedMetadata::MaxDxfImageDefSourceGroups, "0");
  metadata.addDxfImageDefSource(source); REQUIRE(metadata.dxfImageDefSource(1000u));
  source.handle = 1001u; source.groups.emplace_back(90, 0); source.rawValues.emplace_back("0");
  metadata.addDxfImageDefSource(source); CHECK_FALSE(metadata.dxfImageDefSource(1000u));
  metadata.clear(); source.name = "IMAGE"; source.handle = 100u;
  source.groups.assign(LC_DwgAdvancedMetadata::MaxDxfImageSourceGroups, DRW_Variant(90, 0));
  source.rawValues.assign(LC_DwgAdvancedMetadata::MaxDxfImageSourceGroups, "0");
  metadata.addDxfImageSource(source); REQUIRE(metadata.dxfImageSource(100u));
  source.handle = 101u; source.groups.emplace_back(90, 0); source.rawValues.emplace_back("0");
  metadata.addDxfImageSource(source); CHECK_FALSE(metadata.dxfImageSource(100u));
}

TEST_CASE("System codec audits 512-image source graphs", "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto format = GENERATE(RS2::FormatDXFRW, RS2::FormatDXFRW2010);
  const unsigned imageCount = GENERATE(385u, 512u, 513u);
  const unsigned definitionCount = GENERATE(1u, 4u);
  CAPTURE(format, imageCount, definitionCount);
  const auto source = tmpFile("image_512_source.dxf");
  const auto output = tmpFile("image_512_output.dxf");
  RS_Graphic original; original.initForNewDocument();
  for (unsigned i = 0; i < imageCount; ++i)
    original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12),
        QStringLiteral("bounded-%1.png").arg(i % definitionCount), 50, 50, 0)));
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  if (const char *prefix = std::getenv("LIBRECAD_KEEP_512_IMAGE_PREFIX"))
    std::filesystem::copy_file(source, std::string(prefix) + "." + std::to_string(imageCount) + ".d" +
        std::to_string(definitionCount) + ".v" + std::to_string(static_cast<int>(format)) + ".source.dxf",
        std::filesystem::copy_options::overwrite_existing);
  RS_Graphic graphic;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), format));
  const auto &metadata = graphic.dwgAdvancedMetadata();
  REQUIRE(metadata.rasterImages().size() == imageCount);
  REQUIRE(metadata.imageDefinitions().size() == definitionCount);
  REQUIRE(metadata.imageDefinitionReactors().size() == imageCount);
  const std::string sentinel = "preserve 512-image destination";
  writeText(output, sentinel);
  if (imageCount > LC_DwgAdvancedMetadata::MaxDxfRasterSourcesPerKind) {
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    RS_Entity *last = nullptr;
    for (RS_Entity *entity : original)
      if (entity && entity->rtti() == RS2::EntityImage) last = entity;
    REQUIRE(last); REQUIRE(original.removeEntity(last));
    REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
    RS_Graphic corrected; REQUIRE(filter.fileImport(corrected, QString::fromStdString(source), format));
    REQUIRE(filter.fileExport(corrected, QString::fromStdString(output), format));
    CHECK(countRecords(output, "IMAGE") == static_cast<int>(imageCount - 1u));
  } else {
    auto &reactors = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionReactorRecord>&>(
        metadata.imageDefinitionReactors());
    const auto owner = reactors.back().parentHandle;
    ++reactors.back().parentHandle;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    reactors.back().parentHandle = owner;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), format));
    CHECK(countRecords(output, "IMAGE") == static_cast<int>(imageCount));
    CHECK(countRecords(output, "IMAGEDEF") == static_cast<int>(definitionCount));
    CHECK(countRecords(output, "IMAGEDEF_REACTOR") == static_cast<int>(imageCount));
    RS_Graphic reread;
    REQUIRE(filter.fileImport(reread, QString::fromStdString(output), format));
    CHECK(reread.dwgAdvancedMetadata().rasterImages().size() == imageCount);
    CHECK(reread.dwgAdvancedMetadata().imageDefinitions().size() == definitionCount);
    CHECK(reread.dwgAdvancedMetadata().imageDefinitionReactors().size() == imageCount);
    if (imageCount == 512u) {
      struct MutatingFilter : RS_FilterDXFRW {
        std::function<void()> beforeFinalize;
        bool finalizeDxfWrite() override {
          if (beforeFinalize) beforeFinalize();
          return RS_FilterDXFRW::finalizeDxfWrite();
        }
      } late;
      RS_Image *lastImage = nullptr;
      for (RS_Entity *entity : lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities())
        if (entity->rtti() == RS2::EntityImage) lastImage = static_cast<RS_Image *>(entity);
      REQUIRE(lastImage);
      const auto position = lastImage->getInsertionPoint();
      late.beforeFinalize = [&] { lastImage->setInsertionPoint(position + RS_Vector(1, 0)); };
      writeText(output, sentinel);
      CHECK_FALSE(late.fileExport(graphic, QString::fromStdString(output), format));
      { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
      lastImage->setInsertionPoint(position);
      late.beforeFinalize = {};
      REQUIRE(late.fileExport(graphic, QString::fromStdString(output), format));
    }
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_512_IMAGE_PREFIX"))
      std::filesystem::copy_file(output, std::string(prefix) + "." + std::to_string(imageCount) + ".d" +
          std::to_string(definitionCount) + ".v" + std::to_string(static_cast<int>(format)) + ".output.dxf",
          std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("System codec retains the image source file-size guard", "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto source = tmpFile("image_file_guard_source.dxf");
  const auto output = tmpFile("image_file_guard_output.dxf");
  const auto format = RS2::FormatDXFRW2010;
  const std::string sentinel = "preserve source-size destination";
  RS_FilterDXFRW filter;

  // The retained-record budget remains independent of the input-size limit.
  RS_Graphic distinct; distinct.initForNewDocument();
  for (unsigned i = 0; i < 512u; ++i)
    distinct.addEntity(new RS_Image(&distinct, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12),
        QStringLiteral("distinct-image-%1.png").arg(i), 50, 50, 0)));
  REQUIRE(filter.fileExport(distinct, QString::fromStdString(source), format));
  REQUIRE(std::filesystem::file_size(source) > 256u * 1024u);
  RS_Graphic tooManySources;
  REQUIRE(filter.fileImport(tooManySources, QString::fromStdString(source), format));
  REQUIRE(tooManySources.dwgAdvancedMetadata().rasterImages().size() == 512u);
  CHECK(tooManySources.dwgAdvancedMetadata().dxfImageDefSource() == nullptr);
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(tooManySources, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }

  RS_Graphic original; original.initForNewDocument();
  for (unsigned i = 0; i < 512u; ++i)
    original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12),
        QStringLiteral("shared-image-%1.png").arg(i % 256u), 50, 50, 0)));
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  auto bytes = std::filesystem::file_size(source);
  REQUIRE(bytes < 512u * 1024u);
  // Whitespace after EOF changes only the size gate, not the image graph.
  if (bytes <= 256u * 1024u) {
    std::ofstream append(source, std::ios::binary | std::ios::app);
    append << std::string(256u * 1024u + 1u - bytes, ' ');
    REQUIRE(append.good());
  }
  bytes = std::filesystem::file_size(source);
  REQUIRE(bytes > 256u * 1024u);
  std::ifstream input(source, std::ios::binary);
  const std::string originalBytes(std::istreambuf_iterator<char>(input), {});
  input.close();
  REQUIRE(originalBytes.size() == bytes);
  if (const char *prefix = std::getenv("LIBRECAD_KEEP_LARGE_IMAGE_PREFIX"))
    std::filesystem::copy_file(source, std::string(prefix) + ".source.dxf",
                               std::filesystem::copy_options::overwrite_existing);
  RS_Graphic large;
  REQUIRE(filter.fileImport(large, QString::fromStdString(source), format));
  REQUIRE(large.dwgAdvancedMetadata().rasterImages().size() == 512u);
  REQUIRE(filter.fileExport(large, QString::fromStdString(output), format));
  CHECK(countRecords(output, "IMAGE") == 512);
  CHECK(countRecords(output, "IMAGEDEF") == 256);
  CHECK(countRecords(output, "IMAGEDEF_REACTOR") == 512);
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), format));
  CHECK(reread.dwgAdvancedMetadata().rasterImages().size() == 512u);
  CHECK(reread.dwgAdvancedMetadata().imageDefinitions().size() == 256u);
  CHECK(reread.dwgAdvancedMetadata().imageDefinitionReactors().size() == 512u);
  if (const char *prefix = std::getenv("LIBRECAD_KEEP_LARGE_IMAGE_PREFIX"))
    std::filesystem::copy_file(output, std::string(prefix) + ".output.dxf",
                               std::filesystem::copy_options::overwrite_existing);

  writeText(source, originalBytes + std::string(512u * 1024u - bytes, ' '));
  REQUIRE(std::filesystem::file_size(source) == 512u * 1024u);
  RS_Graphic boundary;
  REQUIRE(filter.fileImport(boundary, QString::fromStdString(source), format));
  REQUIRE(filter.fileExport(boundary, QString::fromStdString(output), format));
  { std::ofstream append(source, std::ios::binary | std::ios::app); append.put(' '); }
  REQUIRE(std::filesystem::file_size(source) == 512u * 1024u + 1u);
  RS_Graphic oversized;
  REQUIRE(filter.fileImport(oversized, QString::fromStdString(source), format));
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(oversized, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  writeText(source, originalBytes);
  RS_Graphic corrected;
  REQUIRE(filter.fileImport(corrected, QString::fromStdString(source), format));
  REQUIRE(filter.fileExport(corrected, QString::fromStdString(output), format));
  CHECK(countRecords(output, "IMAGE") == 512);
  CHECK(countRecords(output, "IMAGEDEF") == 256);
  CHECK(countRecords(output, "IMAGEDEF_REACTOR") == 512);

  // A larger raster file must not opt an accompanying MATERIAL into capture.
  std::istringstream materialInput(originalBytes);
  std::string code, value, materialBytes, section;
  bool sectionStart = false, root = false;
  while (std::getline(materialInput, code) && std::getline(materialInput, value)) {
    const int group = std::stoi(code);
    if (group == 0 && root) {
      materialBytes += "3\nACAD_MATERIAL\n350\nF700\n";
      root = false;
    }
    if (group == 0 && value == "SECTION") sectionStart = true;
    else if (sectionStart && group == 2) { section = value; sectionStart = false; }
    if (group == 0 && value == "ENDSEC" && section == "ENTITIES")
      materialBytes += "0\nLINE\n5\nF701\n100\nAcDbEntity\n8\n0\n347\nF702\n"
                       "100\nAcDbLine\n10\n0\n20\n0\n30\n0\n11\n1\n21\n0\n31\n0\n";
    if (group == 0 && value == "ENDSEC" && section == "OBJECTS")
      materialBytes += "0\nDICTIONARY\n5\nF700\n330\nC\n100\nAcDbDictionary\n281\n1\n"
                       "3\nPlainMat\n350\nF702\n0\nMATERIAL\n5\nF702\n330\nF700\n"
                       "100\nAcDbMaterial\n1\nPlainMat\n";
    if (section == "OBJECTS" && group == 5 && value == "C") root = true;
    materialBytes += code + "\n" + value + "\n";
  }
  REQUIRE(materialBytes.size() > 256u * 1024u);
  REQUIRE(materialBytes.size() <= 512u * 1024u);
  writeText(source, materialBytes);
  writeText(output, sentinel);
  RS_Graphic mixed;
  REQUIRE(filter.fileImport(mixed, QString::fromStdString(source), format));
  REQUIRE(mixed.dwgAdvancedMetadata().materials().size() == 1u);
  CHECK_FALSE(filter.fileExport(mixed, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  writeText(source, originalBytes);
  RS_Graphic retry;
  REQUIRE(filter.fileImport(retry, QString::fromStdString(source), format));
  REQUIRE(filter.fileExport(retry, QString::fromStdString(output), format));
  CHECK(countRecords(output, "IMAGE") == 512);
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("System codec audits larger bounded image graphs",
          "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto format = GENERATE(RS2::FormatDXFRW2000, RS2::FormatDXFRW2004,
                               RS2::FormatDXFRW, RS2::FormatDXFRW2010,
                               RS2::FormatDXFRW2013, RS2::FormatDXFRW2018);
  CAPTURE(format);
  const unsigned shape = GENERATE(0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u);
  const unsigned imageCount = shape == 0u ? 4u : shape == 1u ? 8u : shape == 5u ? 33u :
                              shape == 6u || shape == 7u || shape == 9u ? 64u : shape == 8u ? 257u :
                              shape == 10u ? 65u : shape == 13u ? 129u : shape == 16u ? 513u :
                              shape >= 17u ? 384u :
                              shape >= 14u ? 256u : shape >= 11u ? 128u : 32u;
  const unsigned definitionCount = shape == 0u || shape == 2u || shape == 9u || shape == 11u || shape == 14u ? 1u :
                                   shape == 4u ? 32u : shape == 7u ? 64u : shape == 12u ? 128u :
                                   shape == 15u ? 256u : shape == 18u ? 1u : shape == 19u ? 384u : 4u;
  const auto source = tmpFile("many_image_source.dxf");
  const auto output = tmpFile("many_image_output.dxf");
  RS_Graphic original;
  original.initForNewDocument();
  for (unsigned i = 0; i < imageCount; ++i) {
    const auto path = QStringLiteral("many-image-%1.png").arg(i % definitionCount);
    original.addEntity(new RS_Image(&original, RS_ImageData(0, RS_Vector(29.0 * i, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12), path, 50, 50, 0)));
  }
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  if (const char *copy = std::getenv("LIBRECAD_KEEP_MANY_IMAGE_SOURCE"))
    std::filesystem::copy_file(source, std::string(copy) + "." + std::to_string(imageCount) + ".d" +
                                 std::to_string(definitionCount) +
                                 (format == RS2::FormatDXFRW ? "" : ".v" + std::to_string(static_cast<int>(format))) + ".dxf",
                               std::filesystem::copy_options::overwrite_existing);
  REQUIRE(countRecords(source, "IMAGE") == static_cast<int>(imageCount));
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(source), format));
  const auto &metadata = reopened.dwgAdvancedMetadata();
  REQUIRE(metadata.rasterImages().size() == imageCount);
  REQUIRE(metadata.imageDefinitions().size() == definitionCount);
  REQUIRE(metadata.imageDefinitionReactors().size() == imageCount);
  const std::string sentinel = "preserve many-image destination";
  writeText(output, sentinel);
  if (imageCount > LC_DwgAdvancedMetadata::MaxDxfRasterSourcesPerKind) {
    CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    RS_Entity *last = nullptr;
    for (RS_Entity *entity : original)
      if (entity && entity->rtti() == RS2::EntityImage) last = entity;
    REQUIRE(last); REQUIRE(original.removeEntity(last));
    REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
    RS_Graphic corrected; REQUIRE(filter.fileImport(corrected, QString::fromStdString(source), format));
    REQUIRE(filter.fileExport(corrected, QString::fromStdString(output), format));
    CHECK(countRecords(output, "IMAGE") == static_cast<int>(LC_DwgAdvancedMetadata::MaxDxfRasterSourcesPerKind));
  } else {
    auto &reactors = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionReactorRecord>&>(
        metadata.imageDefinitionReactors());
    const auto owner = reactors.back().parentHandle;
    ++reactors.back().parentHandle;
    CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    reactors.back().parentHandle = owner;
    REQUIRE(filter.fileExport(reopened, QString::fromStdString(output), format));
    CHECK(countRecords(output, "IMAGE") == static_cast<int>(imageCount));
    CHECK(countRecords(output, "IMAGEDEF") == static_cast<int>(definitionCount));
    CHECK(countRecords(output, "IMAGEDEF_REACTOR") == static_cast<int>(imageCount));
    RS_Graphic reread;
    REQUIRE(filter.fileImport(reread, QString::fromStdString(output), format));
    CHECK(reread.dwgAdvancedMetadata().rasterImages().size() == imageCount);
    CHECK(reread.dwgAdvancedMetadata().imageDefinitions().size() == definitionCount);
    struct MutatingFilter : RS_FilterDXFRW {
      std::function<void()> beforeFinalize;
      bool finalizeDxfWrite() override {
        if (beforeFinalize) beforeFinalize();
        return RS_FilterDXFRW::finalizeDxfWrite();
      }
    } late;
    RS_Image *lastImage = nullptr;
    for (RS_Entity *entity : lc::LC_ContainerTraverser{reopened, RS2::ResolveNone}.entities())
      if (entity->rtti() == RS2::EntityImage) lastImage = static_cast<RS_Image *>(entity);
    REQUIRE(lastImage);
    const auto originalPosition = lastImage->getInsertionPoint();
    late.beforeFinalize = [&] { lastImage->setInsertionPoint(originalPosition + RS_Vector(1, 0)); };
    writeText(output, sentinel);
    CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
    { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
    lastImage->setInsertionPoint(originalPosition);
    late.beforeFinalize = {};
    REQUIRE(late.fileExport(reopened, QString::fromStdString(output), format));
    if (const char *copy = std::getenv("LIBRECAD_KEEP_MANY_IMAGE_OUTPUT"))
      std::filesystem::copy_file(output, std::string(copy) + "." + std::to_string(imageCount) + ".d" +
                                     std::to_string(definitionCount) +
                                 (format == RS2::FormatDXFRW ? "" : ".v" + std::to_string(static_cast<int>(format))) + ".dxf",
                                 std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

TEST_CASE("System codec preserves larger image graphs across model and block spaces", "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto format = GENERATE(RS2::FormatDXFRW, RS2::FormatDXFRW2018);
  const unsigned imageCount = GENERATE(64u, 256u, 384u, 512u);
  CAPTURE(format, imageCount);
  const auto source = tmpFile("sixty_four_mixed_images_source.dxf");
  const auto output = tmpFile("sixty_four_mixed_images_output.dxf");
  RS_Graphic original; original.initForNewDocument();
  auto *block = new RS_Block(&original, RS_BlockData(QStringLiteral("MANY_RASTERS"), RS_Vector(0, 0), false));
  for (unsigned i = 0; i < imageCount; ++i) {
    RS_EntityContainer *owner = i < imageCount / 2u ? static_cast<RS_EntityContainer *>(&original) : block;
    owner->addEntity(new RS_Image(owner, RS_ImageData(0, RS_Vector(29.0 * i, 2), RS_Vector(1, 0),
        RS_Vector(0, 1), RS_Vector(16, 12), QStringLiteral("mixed-%1.png").arg(i % 2u), 50, 50, 0)));
  }
  REQUIRE(original.addBlock(block));
  original.addEntity(new RS_Insert(&original, RS_InsertData(QStringLiteral("MANY_RASTERS"),
      RS_Vector(20, 0), RS_Vector(1, 1), 0.0, 1, 1, RS_Vector(0, 0))));
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(source), format));
  REQUIRE(reopened.dwgAdvancedMetadata().rasterImages().size() == imageCount);
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(output), format));
  CHECK(countRecords(output, "IMAGE") == static_cast<int>(imageCount)); CHECK(countRecords(output, "IMAGEDEF") == 2);
  CHECK(countRecords(output, "IMAGEDEF_REACTOR") == static_cast<int>(imageCount));
  RS_Block *savedBlock = nullptr;
  for (unsigned i = 0; i < reopened.countBlocks(); ++i)
    if (reopened.blockAt(i) && reopened.blockAt(i)->getName() == "MANY_RASTERS") savedBlock = reopened.blockAt(i);
  REQUIRE(savedBlock);
  RS_Image *blockImage = nullptr;
  for (RS_Entity *entity : lc::LC_ContainerTraverser{*savedBlock, RS2::ResolveNone}.entities())
    if (entity->rtti() == RS2::EntityImage) { blockImage = static_cast<RS_Image *>(entity); break; }
  REQUIRE(blockImage);
  struct LateImageFilter : RS_FilterDXFRW {
    std::function<void()> beforeFinalize;
    bool finalizeDxfWrite() override { if (beforeFinalize) beforeFinalize(); return RS_FilterDXFRW::finalizeDxfWrite(); }
  } late;
  const auto oldPosition = blockImage->getInsertionPoint();
  late.beforeFinalize = [&] { blockImage->setInsertionPoint(oldPosition + RS_Vector(1, 0)); };
  const std::string sentinel = "preserve mixed-image block destination";
  writeText(output, sentinel); CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  blockImage->setInsertionPoint(oldPosition); late.beforeFinalize = {};
  REQUIRE(late.fileExport(reopened, QString::fromStdString(output), format));
  RS_Graphic reread; REQUIRE(filter.fileImport(reread, QString::fromStdString(output), format));
  CHECK(reread.dwgAdvancedMetadata().rasterImages().size() == imageCount);
  if (const char *prefix = std::getenv("LIBRECAD_KEEP_LARGE_BLOCK_IMAGES")) {
    const auto suffix = format == RS2::FormatDXFRW ? std::string{} : ".v" + std::to_string(static_cast<int>(format));
    std::filesystem::copy_file(source, std::string(prefix) + "." + std::to_string(imageCount) + ".source" + suffix + ".dxf", std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(output, std::string(prefix) + "." + std::to_string(imageCount) + ".output" + suffix + ".dxf", std::filesystem::copy_options::overwrite_existing);
  }
  std::filesystem::remove(source); std::filesystem::remove(output);
}

TEST_CASE("System codec verifies shared IMAGE graphs across model and block spaces",
          "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const auto format = GENERATE(RS2::FormatDXFRW2000, RS2::FormatDXFRW2004,
                               RS2::FormatDXFRW, RS2::FormatDXFRW2010,
                               RS2::FormatDXFRW2013, RS2::FormatDXFRW2018);
  CAPTURE(format);
  const auto source = tmpFile("mixed_block_image_source.dxf");
  const auto output = tmpFile("mixed_block_image_output.dxf");
  RS_Graphic original;
  original.initForNewDocument();
  auto addImage = [](RS_EntityContainer &owner, double x, const QString &path) {
    owner.addEntity(new RS_Image(&owner, RS_ImageData(0, RS_Vector(x, 2),
        RS_Vector(1, 0), RS_Vector(0, 1), RS_Vector(16, 12), path, 50, 50, 0)));
  };
  addImage(original, 1, QStringLiteral("shared-image.png"));
  auto *block = new RS_Block(&original, RS_BlockData(QStringLiteral("RASTER_BLOCK"), RS_Vector(0, 0), false));
  addImage(*block, 2, QStringLiteral("shared-image.png"));
  addImage(*block, 3, QStringLiteral("block-image.png"));
  REQUIRE(original.addBlock(block));
  auto *otherBlock = new RS_Block(&original, RS_BlockData(QStringLiteral("OTHER_RASTER"), RS_Vector(0, 0), false));
  addImage(*otherBlock, 4, QStringLiteral("shared-image.png"));
  REQUIRE(original.addBlock(otherBlock));
  original.addEntity(new RS_Insert(&original, RS_InsertData(QStringLiteral("RASTER_BLOCK"),
      RS_Vector(10, 0), RS_Vector(1, 1), 0.0, 1, 1, RS_Vector(0, 0))));
  original.addEntity(new RS_Insert(&original, RS_InsertData(QStringLiteral("OTHER_RASTER"),
      RS_Vector(20, 0), RS_Vector(1, 1), 0.0, 1, 1, RS_Vector(0, 0))));
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(original, QString::fromStdString(source), format));
  REQUIRE(countRecords(source, "IMAGE") == 4);
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(source), format));
  REQUIRE(reopened.dwgAdvancedMetadata().rasterImages().size() == 4u);
  REQUIRE(reopened.dwgAdvancedMetadata().imageDefinitions().size() == 2u);
  RS_Block *sourceBlock = nullptr, *secondBlock = nullptr;
  for (unsigned i = 0; i < reopened.countBlocks(); ++i) {
    auto *candidate = reopened.blockAt(i);
    if (!candidate) continue;
    if (candidate->getName() == QStringLiteral("RASTER_BLOCK")) sourceBlock = candidate;
    if (candidate->getName() == QStringLiteral("OTHER_RASTER")) secondBlock = candidate;
  }
  REQUIRE(sourceBlock);
  REQUIRE(secondBlock);
  REQUIRE(sourceBlock->sourceHandle() != 0u);
  RS_Image *blockImage = nullptr;
  for (RS_Entity *entity : lc::LC_ContainerTraverser{*sourceBlock, RS2::ResolveNone}.entities())
    if (entity->rtti() == RS2::EntityImage) blockImage = static_cast<RS_Image *>(entity);
  REQUIRE(blockImage);
  const std::string sentinel = "preserve mixed block-image destination";
  writeText(output, sentinel);
  const auto blockHandle = sourceBlock->sourceHandle();
  sourceBlock->setSourceHandle(blockHandle + 1u);
  CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  sourceBlock->setSourceHandle(blockHandle);
  const auto secondHandle = secondBlock->sourceHandle();
  REQUIRE(secondHandle != blockHandle);
  secondBlock->setSourceHandle(blockHandle);
  CHECK_FALSE(filter.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  secondBlock->setSourceHandle(secondHandle);
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(output), format));
  CHECK(countRecords(output, "IMAGE") == 4);
  CHECK(countRecords(output, "IMAGEDEF") == 2);
  CHECK(countRecords(output, "IMAGEDEF_REACTOR") == 4);
  struct MutatingFilter : RS_FilterDXFRW {
    std::function<void()> beforeFinalize;
    bool finalizeDxfWrite() override {
      if (beforeFinalize) beforeFinalize();
      return RS_FilterDXFRW::finalizeDxfWrite();
    }
  } late;
  const auto position = blockImage->getInsertionPoint();
  late.beforeFinalize = [&] { blockImage->setInsertionPoint(position + RS_Vector(1, 0)); };
  writeText(output, sentinel);
  CHECK_FALSE(late.fileExport(reopened, QString::fromStdString(output), format));
  { std::ifstream in(output, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(in), {}) == sentinel); }
  blockImage->setInsertionPoint(position);
  late.beforeFinalize = {};
  REQUIRE(late.fileExport(reopened, QString::fromStdString(output), format));
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(output), format));
  CHECK(reread.dwgAdvancedMetadata().rasterImages().size() == 4u);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_BLOCK_IMAGE_SOURCE"))
    std::filesystem::copy_file(source, std::string(copy) + (format == RS2::FormatDXFRW ? "" : ".v" + std::to_string(static_cast<int>(format))),
                               std::filesystem::copy_options::overwrite_existing);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_BLOCK_IMAGE_OUTPUT"))
    std::filesystem::copy_file(output, std::string(copy) + (format == RS2::FormatDXFRW ? "" : ".v" + std::to_string(static_cast<int>(format))),
                               std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(source);
  std::filesystem::remove(output);
}

TEST_CASE("System codec verifies an IMAGE inside a named block",
          "[system-codec][dxf][image][transaction]") {
  ensureSettings();
  const bool polygon = GENERATE(false, true);
  CAPTURE(polygon);
  const auto input = tmpFile("block_image_source.dxf");
  const auto generated = tmpFile("block_image_generated.dxf");
  const auto output = tmpFile("block_image_verified.dxf");
  writeText(input,
      "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n2\nRASTER_BLOCK\n70\n0\n"
      "10\n0\n20\n0\n30\n0\n3\nRASTER_BLOCK\n1\n\n"
      "0\nIMAGE\n8\n0\n100\nAcDbRasterImage\n90\n0\n"
      "10\n1\n20\n2\n30\n3\n11\n1\n21\n0\n31\n0\n"
      "12\n0\n22\n1\n32\n0\n13\n16\n23\n12\n340\n1\n"
      "70\n3\n280\n1\n281\n50\n282\n50\n283\n0\n"
      "71\n1\n91\n2\n14\n-0.5\n24\n-0.5\n14\n15.5\n24\n11.5\n"
      "0\nENDBLK\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
      "0\nINSERT\n8\n0\n2\nRASTER_BLOCK\n10\n0\n20\n0\n30\n0\n"
      "0\nENDSEC\n0\nEOF\n");
  RS_Graphic initial;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(initial, QString::fromStdString(input), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(initial, QString::fromStdString(generated), RS2::FormatDXFRW));
  if (polygon) {
    std::ifstream stream(generated, std::ios::binary);
    std::string code, value, rewritten;
    bool image = false, inserted = false;
    while (std::getline(stream, code) && std::getline(stream, value)) {
      const int group = std::stoi(code);
      if (group == 0) image = value == "IMAGE";
      if (image && group == 70) value = "7";
      if (image && group == 71) value = "2";
      if (image && group == 91) value = "3";
      if (image && (group == 14 || group == 24)) {
        if (!inserted) { rewritten += "14\n1\n24\n1\n14\n10\n24\n1\n14\n4\n24\n9\n"; inserted = true; }
        continue;
      }
      rewritten += code + "\n" + value + "\n";
    }
    REQUIRE(inserted); writeText(generated, rewritten);
  }
  RS_Graphic reopened;
  REQUIRE(filter.fileImport(reopened, QString::fromStdString(generated), RS2::FormatDXFRW));
  REQUIRE(filter.fileExport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(countRecords(output, "IMAGE") == 1);
  const auto block = recordGroupsWithValue(output, "BLOCK_RECORD", "2", "RASTER_BLOCK");
  const auto image = recordGroupsWithValue(output, "IMAGE", "8", "0");
  const auto value = [](const auto &groups, const std::string &code) {
    const auto found = std::find_if(groups.begin(), groups.end(),
        [&](const auto &group) { return group.first == code; });
    return found == groups.end() ? std::string{} : found->second;
  };
  REQUIRE_FALSE(value(block, "5").empty());
  CHECK(value(image, "330") == value(block, "5"));
  if (polygon) {
    for (const char *group : {"71", "91", "14", "24"})
      CHECK(recordGroupValues(output, "IMAGE", group) == recordGroupValues(generated, "IMAGE", group));
    RS_Graphic reread; REQUIRE(filter.fileImport(reread, QString::fromStdString(output), RS2::FormatDXFRW));
    REQUIRE(reread.dwgAdvancedMetadata().rasterImages().size() == 1u);
    CHECK(reread.dwgAdvancedMetadata().rasterImages().front().clipPath.size() == 3u);
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_SINGLE_BLOCK_POLYGON")) {
      std::filesystem::copy_file(generated, std::string(prefix) + "-source.dxf", std::filesystem::copy_options::overwrite_existing);
      std::filesystem::copy_file(output, std::string(prefix) + "-output.dxf", std::filesystem::copy_options::overwrite_existing);
    }
  }
  std::filesystem::remove(input);
  std::filesystem::remove(generated);
  std::filesystem::remove(output);
}

TEST_CASE("System codec preserves single-line controls and literal carets",
          "[system-codec][dxf][text]") {
  ensureSettings();
  for (const auto &header : {
           std::string{},
           std::string{"0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"},
           std::string{"0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC9999\n0\nENDSEC\n"},
           std::string{"0\nSECTION\n2\nHEADER\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"}}) {
    for (const auto &[wire, native] : std::vector<std::pair<std::string, std::string>>{
             {"A^IB", "A\tB"}, {"A^JB", "A\nB"}, {"A^MB", "A\rB"},
             {"A^ B", "A^B"}, {"A^ IB", "A^IB"}}) {
      INFO("header=" << header << " wire=" << wire);
      const std::string src = tmpFile("system_codec_text_source.dxf");
      const std::string out = tmpFile("system_codec_text_output.dxf");
      auto content = singleLineTextDxf(wire);
      if (!header.empty()) {
        const auto begin = content.find("0\nBLOCK\n8\n");
        const auto end = content.find("0\nENDBLK\n8\n");
        REQUIRE(begin != std::string::npos);
        REQUIRE(end != std::string::npos);
        content.replace(end, 11, "0\nENDBLK\n5\n21\n8\n");
        content.replace(begin, 10, "0\nBLOCK\n5\n20\n8\n");
      }
      writeText(src, header + content);
      RS_Graphic graphic;
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
      CHECK(singleLineTextAt(graphic, 4.0) == native);
      CHECK(singleLineTextAt(graphic, 9.0) == native);
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
      CHECK(recordGroupValues(out, "ATTRIB", "1") == std::vector<std::string>{wire});
      CHECK(recordGroupValues(out, "TEXT", "1") == std::vector<std::string>{wire});
      RS_Graphic reread;
      REQUIRE(filter.fileImport(reread, QString::fromStdString(out), RS2::FormatDXFRW));
      CHECK(singleLineTextAt(reread, 4.0) == native);
      CHECK(singleLineTextAt(reread, 9.0) == native);
      std::filesystem::remove(src);
      std::filesystem::remove(out);
    }
  }
}

TEST_CASE("System codec treats binary DXF carets as literal bytes",
          "[system-codec][dxf][text][binary]") {
  ensureSettings();
  const std::string src = tmpFile("system_codec_binary_text.dxf");
  {
    std::ofstream out(src, std::ios::binary);
    const char magic[] = "AutoCAD Binary DXF\r\n\x1a";
    out.write(magic, sizeof(magic));
    const auto code = [&](unsigned n) {
      out.put(static_cast<char>(n & 0xff));
      out.put(static_cast<char>((n >> 8) & 0xff));
    };
    const auto text = [&](unsigned n, const std::string &value) {
      code(n);
      out.write(value.data(), static_cast<std::streamsize>(value.size()));
      out.put('\0');
    };
    const auto real = [&](unsigned n, double value) {
      std::uint64_t bits = 0;
      static_assert(sizeof(bits) == sizeof(value), "binary DXF requires binary64 doubles");
      std::memcpy(&bits, &value, sizeof(bits));
      code(n);
      for (unsigned i = 0; i < 8; ++i)
        out.put(static_cast<char>((bits >> (8 * i)) & 0xff));
    };
    text(0, "SECTION"); text(2, "HEADER"); text(9, "$ACADVER");
    text(1, "AC1021"); text(0, "ENDSEC"); text(0, "SECTION");
    text(2, "ENTITIES");
    for (const auto &[handle, y, value] :
         std::vector<std::tuple<std::string, double, std::string>>{
             {"20", 9.0, "A^IB"}, {"21", 4.0, "A\tB"}}) {
      text(0, "TEXT"); text(5, handle); text(8, "0");
      real(10, 1.0); real(20, y); real(40, 2.5); text(1, value);
    }
    text(0, "ENDSEC"); text(0, "EOF");
    REQUIRE(out.good());
  }
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
  CHECK(singleLineTextAt(graphic, 9.0) == "A^IB");
  CHECK(singleLineTextAt(graphic, 4.0) == "A\tB");
  std::filesystem::remove(src);
}


#endif

// A single-line value (TEXT, ATTRIB) has no MTEXT markup: its backslashes are
// ordinary characters, as in the path of a title block. Reading "\P" as a
// line break left a line feed in the attribute, and the DXF writer refuses a
// record that holds one.
TEST_CASE("Single-line text and attribute values keep their backslashes",
          "[system-codec][dxf][dwg][roundtrip][filter][attrib][text]") {
  ensureSettings();

  struct Case {
    const char *name;
    std::string dxf;    // the value in the source file
    QString native;     // the value of the entity
    std::string saved;  // the value an ASCII DXF export writes
    bool dwg = true;    // whether a DWG reads the value back
  };
  const std::vector<Case> cases{
      {"plain", "plain", QStringLiteral("plain"), "plain"},
      {"paragraph", R"(D:\PROJECTS\PLAN.dwg)",
       QStringLiteral(R"(D:\PROJECTS\PLAN.dwg)"), R"(D:\PROJECTS\PLAN.dwg)"},
      {"program_files", R"(C:\Program Files\x)",
       QStringLiteral(R"(C:\Program Files\x)"), R"(C:\Program Files\x)"},
      {"hard_space", R"(\\SERVER\~TMP\A.dwg)",
       QStringLiteral(R"(\\SERVER\~TMP\A.dwg)"), R"(\\SERVER\~TMP\A.dwg)"},
      {"font_tag", R"({\fArial|b0;Title})",
       QStringLiteral(R"({\fArial|b0;Title})"), R"({\fArial|b0;Title})"},
      {"tab", "A^IB", QStringLiteral("A\tB"), "A^IB"},
      {"line_feed", "A^JB", QStringLiteral("A\nB"), "A^JB"},
      // A DWG string holds the caret itself, and the import takes it for the
      // start of a caret code: that is not a matter of single-line text.
      {"caret", "A^ B", QStringLiteral("A^B"), "A^ B", false},
      {"degree", "45%%d", QStringLiteral("45\u00B0"), "45%%D"},
  };

  struct Format {
    const char *name;
    RS2::FormatType type;
    bool dxf;
  };
  const std::vector<Format> formats{
      {"dxf2018", RS2::FormatDXFRW2018, true},
      {"dxf2007", RS2::FormatDXFRW, true},
      {"dxf2000", RS2::FormatDXFRW2000, true},
      {"dxf12", RS2::FormatDXFRW12, true},
      {"dwg2018", RS2::FormatDWG2018, false},
      {"dwg2000", RS2::FormatDWG, false},
  };

  for (const Case &item : cases) {
    const std::string src =
        tmpFile((std::string("single_line_") + item.name + "_src.dxf").c_str());
    writeText(src, singleLineTextDxf(item.dxf));
    const std::string native = item.native.toStdString();

    RS_Graphic graphic;
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                                RS2::FormatDXFRW));
    }
    {
      INFO("imported value of case " << item.name);
      CHECK(singleLineTextAt(graphic, 4.0) == native);
      CHECK(singleLineTextAt(graphic, 9.0) == native);
    }

    for (const Format &format : formats) {
      if (!format.dxf && !item.dwg)
        continue;
      INFO("case " << item.name << " saved as " << format.name);
      const std::string out = tmpFile(
          (std::string("single_line_") + item.name + "_" + format.name +
           (format.dxf ? ".dxf" : ".dwg"))
              .c_str());
      std::filesystem::remove(out);
      bool exported = false;
      {
        RS_FilterDXFRW filter;
        exported = filter.fileExport(graphic, QString::fromStdString(out),
                                     format.type);
      }
      CHECK(exported);
      if (!exported)
        continue;
      if (format.dxf) {
        CHECK(recordGroupValues(out, "ATTRIB", "1") ==
              std::vector<std::string>{item.saved});
        CHECK(recordGroupValues(out, "TEXT", "1") ==
              std::vector<std::string>{item.saved});
      }

      RS_Graphic reloaded;
      {
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileImport(reloaded, QString::fromStdString(out),
                                  format.type));
      }
      CHECK(singleLineTextAt(reloaded, 4.0) == native);
      CHECK(singleLineTextAt(reloaded, 9.0) == native);
      std::filesystem::remove(out);
    }
    std::filesystem::remove(src);
  }
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
          "[dxf][roundtrip][filter][extrusion][system-codec-extrusion]") {
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
  CHECK(std::stod(firstX[0]) == Catch::Approx(-1.0));
  CHECK(std::stod(firstElevation[0]) == -7.0);
  CHECK(std::stod(firstBulges[0]) < -0.499);
  CHECK(std::stod(firstBulges[0]) > -0.501);
  CHECK(std::stod(firstBulges[1]) == Catch::Approx(0.5));
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
  CHECK(std::stod(secondX[0]) == Catch::Approx(-1.0));
  CHECK(std::stod(secondElevation[0]) == -7.0);
  CHECK(recordGroupValues(out2, "LWPOLYLINE", "210").empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter normalizes reflected legacy POLYLINE extrusion once",
          "[dxf][roundtrip][filter][extrusion][system-codec-extrusion]") {
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
  CHECK(std::stod(firstX[0]) == Catch::Approx(-1.0));
  CHECK(std::stod(firstElevation[0]) == -7.0);
  CHECK(std::stod(firstThickness[0]) == -0.25);
  CHECK(std::abs(std::stod(firstBulges[0]) + 0.5) < 1.0e-12);
  CHECK(std::stod(firstBulges[1]) == Catch::Approx(0.5));
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
  for (const char *code : {"10", "20", "38", "39", "40", "41", "42"}) {
    const auto actual = recordGroupValues(out2, "LWPOLYLINE", code);
    const auto expected = recordGroupValues(out, "LWPOLYLINE", code);
    REQUIRE(actual.size() == expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i)
      CHECK(std::stod(actual[i]) == Catch::Approx(std::stod(expected[i])));
  }
  CHECK(recordGroupValues(out2, "LWPOLYLINE", "210").empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves reflected planar curve conventions once",
          "[dxf][roundtrip][filter][extrusion][system-codec-extrusion]") {
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
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  // ELLIPSE center and major axis are WCS. A -Z plane reverses the parameter
  // interval when represented with the default +Z normal.
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "10") == 30.0);
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "11") == 5.0);
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "41") == Catch::Approx(2. * M_PI - 1.75));
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "42") == Catch::Approx(2. * M_PI - 0.25));
#else
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "10") == -30.0);
  CHECK(firstGroupValueAsDouble(out, "ELLIPSE", "11") == -5.0);
#endif
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
          "[dxf][roundtrip][filter][extrusion][system-codec]") {
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
      CHECK(std::stod(actual.front()) ==
            Catch::Approx(std::stod(expected)).epsilon(1e-12).margin(1e-12));
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
  {
    // The leading HATCH 10 is the hatch base point ("0"); the remaining
    // four 10 values come from the extracted boundary edges. The exporter
    // now writes one boundary path per extracted loop, so the order of
    // those edges depends on the loop extractor and is checked
    // order-free. The 20 values ride with the 10 values and are compared
    // in matching order so each (x, y) pair stays together.
    const auto tenValues = recordGroupValues(out, "HATCH", "10");
    const auto twentyValues = recordGroupValues(out, "HATCH", "20");
    REQUIRE(tenValues.size() == 5u);
    REQUIRE(twentyValues.size() == 5u);
    CHECK(tenValues.front() == "0");
    CHECK(twentyValues.front() == "0");
    std::vector<std::pair<std::string, std::string>> edgePairs;
    for (std::size_t i = 1; i < tenValues.size(); ++i) {
      edgePairs.emplace_back(tenValues[i], twentyValues[i]);
    }
    std::sort(edgePairs.begin(), edgePairs.end());
    const std::vector<std::pair<std::string, std::string>> expectedEdges{
        {"1", "2"}, {"1", "4"}, {"3", "2"}, {"3", "4"}};
    CHECK(edgePairs == expectedEdges);
  }

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

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
TEST_CASE("DXF dictionary metadata retains each source handle role",
          "[dxf][filter][dictionary][system-codec-dictionary-roles]") {
  ensureSettings();
  const std::string src = tmpFile("dictionary_edge_roles.dxf");
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n"
            "280\n0\n281\n1\n3\nSOFT\n350\nA0\n"
            "3\nHARD\n360\nA1\n"
            "0\nXRECORD\n5\nA0\n330\nC\n100\nAcDbXrecord\n280\n1\n1\nsoft\n"
            "0\nXRECORD\n5\nA1\n330\nC\n100\nAcDbXrecord\n280\n1\n1\nhard\n"
            "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
  const auto &dictionaries = graphic.dwgAdvancedMetadata().dictionaries();
  const auto root = std::find_if(dictionaries.begin(), dictionaries.end(),
                                 [](const auto &d) { return d.handle == 0xCu; });
  REQUIRE(root != dictionaries.end());
  REQUIRE(root->entries.size() == 2);
  CHECK(root->entries[0].name == "SOFT");
  CHECK(root->entries[0].handle == 0xA0u);
  CHECK(root->entries[0].dxfHandleCode == 350u);
  CHECK(root->entries[1].name == "HARD");
  CHECK(root->entries[1].handle == 0xA1u);
  CHECK(root->entries[1].dxfHandleCode == 360u);
  std::filesystem::remove(src);
}

TEST_CASE("DXF typed dictionary retains handle roles and retries invalid roles",
          "[dxf][filter][dictionary][system-codec-dictionary-roles]") {
  ensureSettings();
  const std::string out = tmpFile("dictionary_typed_edge_roles.dxf");
  RS_Graphic graphic;
  auto &metadata = graphic.dwgAdvancedMetadata();
  DRW_Dictionary root;
  root.handle = 0xCu;
  root.m_entries.push_back({"CUSTOM", 0xA5u});
  metadata.addDictionary(root);
  DRW_Dictionary dictionary;
  dictionary.handle = 0xA5u;
  dictionary.parentHandle = 0xCu;
  dictionary.m_entries.push_back({"SOFT", 0xA0u,
      DRW_Dictionary::Entry::DxfHandleCode::SoftOwner});
  dictionary.m_entries.push_back({"HARD", 0xA1u,
      DRW_Dictionary::Entry::DxfHandleCode::HardOwner});
  metadata.addDictionary(dictionary);
  for (const auto handle : {0xA0u, 0xA1u}) {
    DRW_Scale scale;
    scale.handle = handle;
    scale.parentHandle = 0xA5u;
    scale.name = handle == 0xA0u ? "soft-scale" : "hard-scale";
    metadata.addScale(scale);
  }
  auto &records = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord>&>(
      metadata.dictionaries());
  REQUIRE(records.size() == 2);
  records[1].entries[0].dxfHandleCode = 351;
  const std::string sentinel = "existing destination";
  writeText(out, sentinel);
  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  std::ifstream existing(out, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
  existing.close();
  records[1].entries[0].dxfHandleCode = 350;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  const auto groups = recordGroupsWithValue(out, "DICTIONARY", "5", "A5");
  const auto soft = std::find(groups.begin(), groups.end(), std::make_pair(std::string("3"), std::string("SOFT")));
  const auto hard = std::find(groups.begin(), groups.end(), std::make_pair(std::string("3"), std::string("HARD")));
  REQUIRE(soft != groups.end());
  REQUIRE(hard != groups.end());
  REQUIRE(std::next(soft) != groups.end());
  REQUIRE(std::next(hard) != groups.end());
  CHECK(*std::next(soft) == std::make_pair(std::string("350"), std::string("A0")));
  CHECK(*std::next(hard) == std::make_pair(std::string("360"), std::string("A1")));
  std::filesystem::remove(out);
}
#endif

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
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  {
    const std::string unknown = tmpFile("image_wcs_unknown_definition.dxf");
    std::ifstream input(out, std::ios::binary);
    std::string bytes(std::istreambuf_iterator<char>(input), {});
    const std::string marker = "\n100\nAcDbRasterImageDef\n";
    const auto position = bytes.find(marker);
    REQUIRE(position != std::string::npos);
    bytes.insert(position + marker.size(), "70\n7\n");
    writeText(unknown, bytes);
    RS_Graphic rejected;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(rejected, QString::fromStdString(unknown), RS2::FormatDXFRW));
    const std::string sentinel = "preserve destination for unknown IMAGEDEF";
    writeText(out2, sentinel);
    CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(out2), RS2::FormatDXFRW));
    std::ifstream existing(out2, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
    existing.close();
    std::filesystem::remove(unknown);
  }
  {
    const std::string unknown = tmpFile("image_wcs_unknown_entity_field.dxf");
    std::ifstream input(out, std::ios::binary);
    std::string bytes(std::istreambuf_iterator<char>(input), {});
    const std::string marker = "\n100\nAcDbRasterImage\n";
    const auto position = bytes.find(marker);
    REQUIRE(position != std::string::npos);
    bytes.insert(position + marker.size(), "39\n7\n");
    writeText(unknown, bytes);
    RS_Graphic rejected;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(rejected, QString::fromStdString(unknown), RS2::FormatDXFRW));
    const std::string sentinel = "preserve destination for unknown IMAGE field";
    writeText(out2, sentinel);
    CHECK_FALSE(filter.fileExport(rejected, QString::fromStdString(out2), RS2::FormatDXFRW));
    std::ifstream existing(out2, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
    existing.close();
    std::filesystem::remove(unknown);
  }
  {
    const std::string large = tmpFile("image_wcs_large_source.dxf");
    std::ifstream input(out, std::ios::binary);
    std::string bytes(std::istreambuf_iterator<char>(input), {});
    bytes.append(256u * 1024u, ' '); // trailing input after EOF is ignored
    writeText(large, bytes);
    RS_Graphic imported;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(imported, QString::fromStdString(large), RS2::FormatDXFRW));
    CHECK(imported.dwgAdvancedMetadata().dxfRootDictionarySource() == nullptr);
    const std::string sentinel = "preserve destination for uncaptured IMAGEDEF";
    writeText(out2, sentinel);
    CHECK_FALSE(filter.fileExport(imported, QString::fromStdString(out2), RS2::FormatDXFRW));
    std::ifstream existing(out2, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
    existing.close();
    std::filesystem::remove(large);
  }
#endif
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
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
    auto &metadata = graphic2.dwgAdvancedMetadata();
    auto &definitions = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionRecord>&>(
        metadata.imageDefinitions());
    auto &reactors = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionReactorRecord>&>(
        metadata.imageDefinitionReactors());
    auto &dictionaries = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord>&>(
        metadata.dictionaries());
    REQUIRE(definitions.size() == 1);
    REQUIRE(reactors.size() == 1);
    const auto dictionary = std::find_if(dictionaries.begin(), dictionaries.end(),
        [&](const auto &record) { return record.handle == definitions.front().parentHandle; });
    REQUIRE(dictionary != dictionaries.end());
    REQUIRE(dictionary->entries.size() == 1);
    const std::string sentinel = "preserve existing image destination";
    const auto expectRefusal = [&] {
      writeText(out2, sentinel);
      CHECK_FALSE(filter.fileExport(graphic2, QString::fromStdString(out2), RS2::FormatDXFRW));
      std::ifstream existing(out2, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
    };
    auto *rootSource = const_cast<DRW_RawDxfObject *>(metadata.dxfRootDictionarySource());
    REQUIRE(rootSource != nullptr);
    REQUIRE(metadata.dxfFixedGroupDictionarySource() != nullptr);
    rootSource->groups.emplace_back(70, std::int32_t{7});
    if (rootSource->hasRawValues)
      rootSource->rawValues.push_back("7");
    expectRefusal(); // unmodeled root data must not disappear on regeneration
    rootSource->groups.pop_back();
    if (rootSource->hasRawValues)
      rootSource->rawValues.pop_back();
    auto *definitionSource = const_cast<DRW_RawDxfObject *>(metadata.dxfImageDefSource());
    REQUIRE(definitionSource != nullptr);
    definitionSource->groups.emplace_back(70, std::int32_t{7});
    if (definitionSource->hasRawValues)
      definitionSource->rawValues.push_back("7");
    expectRefusal(); // unknown source IMAGEDEF fields must not be discarded
    definitionSource->groups.pop_back();
    if (definitionSource->hasRawValues)
      definitionSource->rawValues.pop_back();
    const auto originalParent = reactors.front().parentHandle;
    reactors.front().parentHandle = originalParent + 1;
    expectRefusal();
    reactors.front().parentHandle = originalParent;
    auto &rawObjects = const_cast<std::vector<DRW_RawDxfObject>&>(metadata.rawDxfObjects());
    auto rawReactor = std::find_if(rawObjects.begin(), rawObjects.end(),
        [&](const auto &record) { return record.handle == reactors.front().handle; });
    REQUIRE(rawReactor != rawObjects.end());
    auto classVersion = std::find_if(rawReactor->groups.begin(), rawReactor->groups.end(),
                                    [](const auto &group) { return group.code() == 90; });
    REQUIRE(classVersion != rawReactor->groups.end());
    REQUIRE(rawReactor->hasRawValues);
    const auto versionIndex = static_cast<std::size_t>(std::distance(rawReactor->groups.begin(), classVersion));
    const auto originalVersionGroup = *classVersion;
    const auto originalVersionRaw = rawReactor->rawValues.at(versionIndex);
    const auto originalVersion = reactors.front().classVersion;
    reactors.front().classVersion = 3;
    *classVersion = DRW_Variant(90, std::int32_t{3});
    rawReactor->rawValues[versionIndex] = "3";
    expectRefusal(); // the shared writer cannot preserve this source version
    reactors.front().classVersion = originalVersion;
    *classVersion = originalVersionGroup;
    rawReactor->rawValues[versionIndex] = originalVersionRaw;
    auto owner = std::find_if(rawReactor->groups.begin(), rawReactor->groups.end(),
                              [](const auto &group) { return group.code() == 330; });
    REQUIRE(owner != rawReactor->groups.end());
    REQUIRE(rawReactor->hasRawValues);
    const auto ownerIndex = static_cast<std::size_t>(std::distance(rawReactor->groups.begin(), owner));
    const auto originalOwner = *owner;
    const auto originalRawOwner = rawReactor->rawValues.at(ownerIndex);
    *owner = DRW_Variant(330, std::string("FFFF"));
    rawReactor->rawValues[ownerIndex] = "FFFF";
    expectRefusal();
    *owner = originalOwner;
    rawReactor->rawValues[ownerIndex] = originalRawOwner;
    const auto originalRole = dictionary->entries.front().dxfHandleCode;
    dictionary->entries.front().dxfHandleCode = originalRole == 350 ? 360 : 350;
    expectRefusal();
    dictionary->entries.front().dxfHandleCode = originalRole;
    definitions.push_back(definitions.front());
    expectRefusal();
    definitions.pop_back();
#endif
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2),
                              RS2::FormatDXFRW));
  }
#if defined(LIBRECAD_SYSTEM_CODEC_TESTS) && defined(LIBDXFRW_HAS_DXF_STAGED_IMAGE_REFERENCE_INSPECTOR)
  {
    struct MutatingImageFilter : RS_FilterDXFRW {
      std::function<void()> afterObjects, beforeFinalize;
      void writeObjects() override {
        RS_FilterDXFRW::writeObjects();
        if (afterObjects) afterObjects();
      }
      bool finalizeDxfWrite() override {
        if (beforeFinalize) beforeFinalize();
        return RS_FilterDXFRW::finalizeDxfWrite();
      }
    } filter;
    auto &reactors = const_cast<std::vector<LC_DwgAdvancedMetadata::ImageDefinitionReactorRecord>&>(
        graphic2.dwgAdvancedMetadata().imageDefinitionReactors());
    REQUIRE(reactors.size() == 1u);
    const auto originalParent = reactors.front().parentHandle;
    const std::string sentinel = "preserve late IMAGE graph failure";
    const auto rejectMutation = [&] {
      writeText(out2, sentinel);
      CHECK_FALSE(filter.fileExport(graphic2, QString::fromStdString(out2), RS2::FormatDXFRW));
      std::ifstream input(out2, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(input), {}) == sentinel);
      reactors.front().parentHandle = originalParent;
    };
    filter.afterObjects = [&] { ++reactors.front().parentHandle; };
    rejectMutation();
    filter.afterObjects = {};
    filter.beforeFinalize = [&] { ++reactors.front().parentHandle; };
    rejectMutation();
    RS_Image *liveImage = nullptr;
    for (RS_Entity *entity : lc::LC_ContainerTraverser{graphic2, RS2::ResolveNone}.entities())
      if (entity->rtti() == RS2::EntityImage) liveImage = static_cast<RS_Image *>(entity);
    REQUIRE(liveImage);
    const auto originalPath = liveImage->getFile();
    filter.beforeFinalize = [&] { liveImage->setFile(QStringLiteral("late-image-change.png")); };
    rejectMutation();
    liveImage->setFile(originalPath);
    filter.beforeFinalize = [&] { liveImage->setFile(originalPath + QChar(0) + QStringLiteral("hidden")); };
    rejectMutation();
    liveImage->setFile(originalPath);
    filter.beforeFinalize = {};
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2), RS2::FormatDXFRW));
  }
#endif
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  if (const char *copy = std::getenv("LIBRECAD_KEEP_IMAGE_ROUNDTRIP"))
    std::filesystem::copy_file(out2, copy, std::filesystem::copy_options::overwrite_existing);
#endif
  for (const char *code : {"10", "20", "30", "11", "21", "31", "12", "22", "32"})
    CHECK(recordGroupValues(out2, "IMAGE", code)
          == recordGroupValues(out, "IMAGE", code));

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  RS_Image *image = nullptr;
  for (RS_Entity *entity : lc::LC_ContainerTraverser{graphic2, RS2::ResolveNone}.entities()) {
    if (entity->rtti() == RS2::EntityImage) {
      REQUIRE(image == nullptr);
      image = static_cast<RS_Image *>(entity);
    }
  }
  REQUIRE(image != nullptr);
  image->setFile(QStringLiteral("renamed-image.png"));
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic2, QString::fromStdString(out2), RS2::FormatDXFRW));
  }
  CHECK(recordGroupValues(out2, "IMAGEDEF", "1") ==
        std::vector<std::string>{"renamed-image.png"});
#endif

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
          "[dxf][roundtrip][filter][extrusion][system-codec-extrusion]") {
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

TEST_CASE("DXF imports expand INSERTs after complete and partial reads",
          "[dxf][filter][partial][insert]") {
  ensureSettings();
  lc::test::QuietLog quiet;
  const auto readError = GENERATE(DRW::BAD_NONE, DRW::BAD_READ_ENTITIES, DRW::BAD_READ_BLOCKS);
  const bool partial = readError != DRW::BAD_NONE;
  const bool direct = GENERATE(false, true);
  CAPTURE(readError, direct);
  const std::string src = tmpFile("partial_inserts.dxf");
  const std::string blocks =
            "0\nSECTION\n2\nBLOCKS\n"
            "0\nBLOCK\n8\n0\n2\nOUTER\n70\n0\n10\n0.0\n20\n0.0\n30\n0.0\n"
            "0\nINSERT\n8\n0\n2\nINNER\n10\n10.0\n20\n0.0\n"
            "0\nENDBLK\n8\n0\n"
            "0\nBLOCK\n8\n0\n2\nINNER\n70\n0\n10\n0.0\n20\n0.0\n30\n0.0\n"
            "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n2.0\n21\n0.0\n"
            "0\nENDBLK\n8\n0\n";
  const std::string entities =
            "0\nSECTION\n2\nENTITIES\n"
            "0\nINSERT\n8\n0\n2\nOUTER\n10\n100.0\n20\n50.0\n"
            "0\nINSERT\n8\n0\n2\nABSENT\n10\n0.0\n20\n0.0\n";
  // Put ENTITIES first to retain model-space references when BLOCKS is truncated.
  const std::string body = readError == DRW::BAD_READ_BLOCKS
      ? entities + "0\nENDSEC\n" + blocks + "0\nBLOCK\n8\n0\n2\nCUT\n70\n0\n10\n0.0\n"
      : blocks + "0\nENDSEC\n" + entities
            + (partial ? "0\nLINE\n8\n0\n10\n0.0\n" : "0\nENDSEC\n0\nEOF\n");
  writeText(src, "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n" + body);
  const QString expectedError = readError == DRW::BAD_READ_BLOCKS
      ? QObject::tr("error reading DXF/DWG blocks", "RS_FilterDXFRW")
      : QObject::tr("error reading DXF/DWG entities", "RS_FilterDXFRW");

  RS_Graphic graphic;
  graphic.initForNewDocument();
  const QString path = QString::fromStdString(src);
  if (direct) {
    RS_FilterDXFRW filter;
    CHECK(filter.fileImport(graphic, path, RS2::FormatDXFRW) == !partial);
    if (partial)
      CHECK(filter.lastError() == expectedError);
  } else {
    int callbacks = 0;
    REQUIRE(RS_FileIO::instance()->fileImport(
        graphic, path, RS2::FormatUnknown, [&](bool hasData, const QString &error) {
          ++callbacks;
          CHECK(hasData);
          CHECK(error == expectedError);
          return hasData;
        }));
    CHECK(callbacks == (partial ? 1 : 0));
    graphic.onLoadingCompleted();
  }

  REQUIRE(graphic.count() == 2);
  auto *outer = dynamic_cast<RS_Insert *>(graphic.entityAt(0));
  REQUIRE(outer != nullptr);
  REQUIRE(outer->count() == 1);
  CHECK(outer->getMin().x == Catch::Approx(110.0));
  CHECK(outer->getMin().y == Catch::Approx(50.0));
  CHECK(outer->getMax().x == Catch::Approx(112.0));
  CHECK(outer->getMax().y == Catch::Approx(50.0));
  RS_Block *block = graphic.findBlock("OUTER");
  REQUIRE(block != nullptr);
  auto *inner = dynamic_cast<RS_Insert *>(block->entityAt(0));
  REQUIRE(inner != nullptr);
  CHECK(inner->count() == 1);
  auto *absent = dynamic_cast<RS_Insert *>(graphic.entityAt(1));
  REQUIRE(absent != nullptr);
  CHECK(absent->getBlockForInsert() == nullptr);
  CHECK(absent->count() == 0);
  std::filesystem::remove(src);
}

TEST_CASE("DXF filter preserves reflected MINSERT source fields with a block definition",
          "[dxf][roundtrip][filter][extrusion][insert][system-codec-extrusion]") {
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
      CHECK(std::stod(actual.front()) == Catch::Approx(std::stod(expected)));
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

TEST_CASE("DXF filter registers CLASS records for reconstructed underlays",
          "[system-codec][dxf][roundtrip][filter][classes][underlay]") {
  ensureSettings();
  lc::test::QuietLog quiet;
  lc::test::OutputDirectory directory("underlay_classes");
  const auto [kind, className] = GENERATE(table<std::string, std::string>({
      {"PDF", "AcDbPdfReference"}, {"DGN", "AcDbDgnReference"},
      {"DWF", "AcDbDwfReference"}}));
  const auto [format, acadVersion] = GENERATE(table<RS2::FormatType, std::string>({
      {RS2::FormatDXFRW14, "AC1014"}, {RS2::FormatDXFRW2000, "AC1015"},
      {RS2::FormatDXFRW2004, "AC1018"}, {RS2::FormatDXFRW, "AC1021"},
      {RS2::FormatDXFRW2010, "AC1024"}, {RS2::FormatDXFRW2013, "AC1027"},
      {RS2::FormatDXFRW2018, "AC1032"},
      {RS2::FormatDXFRW12, "AC1021"}}));
  const bool sourceClass = GENERATE(false, true);
  CAPTURE(kind, acadVersion, format, sourceClass);
  const std::string recordName = kind + "UNDERLAY";
  std::string source = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\n" +
                       acadVersion + "\n0\nENDSEC\n";
  if (sourceClass) {
    source += "0\nSECTION\n2\nCLASSES\n0\nCLASS\n1\n" + recordName +
              "\n2\n" + className + "\n3\nUNDERLAY_TEST\n90\n4095\n";
    if (acadVersion >= "AC1018")
      source += "91\n7\n";
    source += "280\n1\n281\n1\n0\nENDSEC\n";
  }
  source += "0\nSECTION\n2\nENTITIES\n";
  for (const char* handle : {"101", "102", "103"}) {
    source += "0\n" + recordName + "\n5\n" + handle +
              "\n8\n0\n100\nAcDbUnderlayReference\n340\n200\n"
              "10\n0\n20\n0\n30\n0\n280\n2\n281\n100\n282\n0\n";
  }
  source += "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n0\n" + kind +
            "DEFINITION\n5\n200\n330\n0\n100\nAcDbUnderlayDefinition\n"
            "1\nsheet\n2\n1\n0\nENDSEC\n0\nEOF\n";
  const auto input = directory.file("source.dxf");
  writeText(input.string(), source);
  RS_Graphic graphic;
  REQUIRE(lc::test::importDrawing(graphic, input, RS2::FormatDXFRW));
  REQUIRE(graphic.getEntityList().size() == 3);
  graphic.getEntityList().back()->setFlag(RS2::FlagDeleted);

  const auto output = directory.file("output.dxf");
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  // The installed writer refuses versions that cannot represent the profile.
  // Preserve the original destination, then retry the same filter with a
  // version that supports all three underlay kinds.
  const bool supported = format == RS2::FormatDXFRW2010 ||
                         format == RS2::FormatDXFRW2013 ||
                         format == RS2::FormatDXFRW2018 ||
                         (format == RS2::FormatDXFRW && kind == "DWF");
  if (!supported) {
    const std::string sentinel = "preserve existing destination\n";
    writeText(output.string(), sentinel);
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output.string()), format));
    std::ifstream saved(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(saved), {}) == sentinel);
    saved.close();
    // The original source itself can claim an older DXF version while
    // carrying a newer underlay. Reimport a corrected profile before retry.
    std::string corrected = source;
    const std::string oldVersion = "9\n$ACADVER\n1\n" + acadVersion;
    const auto versionAt = corrected.find(oldVersion + "\n");
    REQUIRE(versionAt != std::string::npos);
    corrected.replace(versionAt, oldVersion.size(), "9\n$ACADVER\n1\nAC1024");
    if (sourceClass && acadVersion < "AC1018") {
      const auto classAt = corrected.find("280\n1\n281\n1\n");
      REQUIRE(classAt != std::string::npos);
      corrected.insert(classAt, "91\n7\n");
    }
    const auto correctedInput = directory.file("corrected.dxf");
    writeText(correctedInput.string(), corrected);
    RS_Graphic correctedGraphic;
    REQUIRE(filter.fileImport(correctedGraphic,
                              QString::fromStdString(correctedInput.string()),
                              RS2::FormatDXFRW));
    REQUIRE(correctedGraphic.getEntityList().size() == 3);
    correctedGraphic.getEntityList().back()->setFlag(RS2::FlagDeleted);
    REQUIRE(filter.fileExport(correctedGraphic, QString::fromStdString(output.string()),
                              RS2::FormatDXFRW2010));
    CHECK(countRecords(output.string(), recordName) == 2);
    return;
  }
#endif
  REQUIRE(lc::test::exportDrawing(graphic, output, format));
  if (format == RS2::FormatDXFRW12) {
    CHECK_FALSE(containsLine(output.string(), "CLASSES"));
    CHECK(countRecords(output.string(), recordName) == 0);
    return;
  }
  CHECK(countRecords(output.string(), recordName) == 2);
  int classCount = 0;
  for (const auto& cls : lc::test::dxfRecords(output, "CLASS")) {
    if (cls.own(1) != std::vector<std::string>{recordName})
      continue;
    ++classCount;
    CHECK(cls.own(2) == std::vector<std::string>{className});
    CHECK(cls.own(3) == std::vector<std::string>{sourceClass ? "UNDERLAY_TEST" : "ObjectDBX Classes"});
    CHECK(cls.own(90) == std::vector<std::string>{"4095"});
    CHECK(cls.own(280) == std::vector<std::string>{sourceClass ? "1" : "0"});
    CHECK(cls.own(281) == std::vector<std::string>{"1"});
    if (acadVersion >= "AC1018")
      CHECK(cls.own(91) == std::vector<std::string>{"2"});
  }
  CHECK(classCount == 1);

  for (RS_Entity* entity : graphic)
    entity->setFlag(RS2::FlagDeleted);
  const auto emptyOutput = directory.file("deleted.dxf");
  REQUIRE(lc::test::exportDrawing(graphic, emptyOutput, format));
  CHECK(countRecords(emptyOutput.string(), recordName) == 0);
  CHECK(classRecordNames(emptyOutput.string()).count(recordName) == (sourceClass ? 1 : 0));
}

TEST_CASE("DXF filter preserves fixture-backed CLASS and raw entity",
          "[system-codec][dxf][roundtrip][filter][classes][raw]") {
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
  const auto checkClass = [](const std::string &path) {
    const auto classes = lc::test::dxfRecords(path, "CLASS");
    int matches = 0;
    for (const auto &entry : classes) {
      if (entry.own(1) != std::vector<std::string>{"WEIRDENT"})
        continue;
      ++matches;
      CHECK(entry.own(2) == std::vector<std::string>{"AcDbWeirdEntity"});
      CHECK(entry.own(3) == std::vector<std::string>{"CUSTOM_APP"});
      CHECK(entry.own(90) == std::vector<std::string>{"4095"});
      CHECK(entry.own(91) == std::vector<std::string>{"1"});
      CHECK(entry.own(280) == std::vector<std::string>{"1"});
      CHECK(entry.own(281) == std::vector<std::string>{"1"});
    }
    CHECK(matches == 1);
  };
  checkClass(out);

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
  checkClass(out2);

  std::filesystem::remove(out);
  std::filesystem::remove(out2);
}

TEST_CASE("DXF filter preserves fixture-derived raw control groups and remaps 481",
          "[dxf][roundtrip][filter][application-groups][handles]") {
  ensureSettings();
  const std::string fixture = std::string(LIBRECAD_TEST_DIR)
                              + "/dxf/raw_control_groups_r2007.dxf";
  const std::string src = tmpFile("raw_control_groups_neutral.dxf");
  const std::string out = tmpFile("raw_control_groups_out.dxf");
  const std::string out2 = tmpFile("raw_control_groups_out2.dxf");
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
  if (!std::filesystem::is_regular_file(fixture)) {
    SKIP("raw_control_groups_r2007.dxf fixture absent; skipping");
  }

#ifdef LIBDXFRW_HAS_DXF_STAGED_LAYER_PLOT_STYLE_INSPECTOR
  // The original fixture contains incomplete MATERIAL objects. Preserve the
  // guard for that graph independently of the generic handle-remapping test.
  {
    RS_Graphic materialGraphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(materialGraphic, QString::fromStdString(fixture),
                              RS2::FormatDXFRW));
    writeText(out, "preserve destination\n");
    CHECK_FALSE(filter.fileExport(materialGraphic, QString::fromStdString(out),
                                  RS2::FormatDXFRW));
    std::ifstream retained(out, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {})
          == "preserve destination\n");
  }
#endif
  // Reuse the fixture's exact control groups with neutral raw object types;
  // this test exercises handle transport, not MATERIAL graph semantics.
  std::ifstream input(fixture, std::ios::binary);
  std::string contents((std::istreambuf_iterator<char>(input)), {});
  input.close();
  // Git may check out the fixture with CRLF on Windows. Preserve any isolated
  // carriage returns while making the record boundaries platform-independent.
  std::string lineEndingControl = "0\r\nMATERIAL\r\ntext\rvalue\n";
  normalizeDxfLineEndings(lineEndingControl);
  CHECK(lineEndingControl == "0\nMATERIAL\ntext\rvalue\n");
  normalizeDxfLineEndings(contents);
  const auto replaceAll = [&](const std::string& from, const std::string& to) {
    std::size_t count = 0;
    for (std::size_t pos = 0; (pos = contents.find(from, pos)) != std::string::npos;
         pos += to.size()) {
      contents.replace(pos, from.size(), to);
      ++count;
    }
    return count;
  };
  REQUIRE(replaceAll("\nMATERIAL\n", "\nTEST_RAW_OBJECT\n") == 2);
  REQUIRE(replaceAll("\nAcDbMaterial\n", "\nTestRawObject\n") == 2);
  writeText(src, contents);

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
      [](const auto& entry) { return entry.name == "TEST_RAW_OBJECT" && entry.handle == 0x90u; });
  REQUIRE(raw != graphic.dwgAdvancedMetadata().rawDxfObjects().cend());
  CHECK(raw->parentHandle == 0xCu);
  CHECK(rawValues(raw->groups, 481) == std::vector<std::string>{"14"});
  CHECK(rawValues(raw->groups, 102)
        == std::vector<std::string>{"{CUSTOM_CONTROL", "{NESTED", "}", "}",
                                    "{ACAD_REACTORS", "}",
                                    "{ACAD_XDICTIONARY", "}"});

  const DxfRecordGroups sourceOut =
      recordGroupsWithValue(out, "TEST_RAW_OBJECT", "1", "SOURCE");
  const DxfRecordGroups targetOut =
      recordGroupsWithValue(out, "TEST_RAW_OBJECT", "1", "TARGET");
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
  verifySourceRecord(recordGroupsWithValue(out2, "TEST_RAW_OBJECT", "1", "SOURCE"),
                     recordGroupsWithValue(out2, "TEST_RAW_OBJECT", "1", "TARGET"));

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
  CHECK_FALSE(std::filesystem::exists(out));
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
      "0\nTEST_RAW_OBJECT\n5\n34\n330\nC\n100\nTestRawObject\n1\nRawObject\n94\n7\n"
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
          "[dxf][roundtrip][filter][handles][groups][system-codec]") {
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

  const auto group = recordGroupValues(out, "GROUP", "5");
  REQUIRE(group.size() == 1);
  const auto dictionary = recordGroupsWithValue(out, "DICTIONARY", "3", "PAIR");
  const auto name = std::find(dictionary.begin(), dictionary.end(),
                              std::make_pair(std::string("3"), std::string("PAIR")));
  REQUIRE(name != dictionary.end());
  REQUIRE(std::next(name) != dictionary.end());
  CHECK(*std::next(name) == std::make_pair(std::string("350"), group.front()));
  CHECK(recordGroupValues(out, "GROUP", "340")
        == recordGroupValues(out, "LINE", "5"));
  CHECK(recordReactors(out, "LINE")
        == std::vector<std::vector<std::string>>{{group.front()}, {group.front()}});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

#if defined(LIBRECAD_SYSTEM_CODEC_TESTS) && defined(LIBDXFRW_HAS_DXF_PHYSICAL_ENTITY_SOURCE_HANDLERS)
TEST_CASE("ODA compact-material hatch drawing imports without claiming replay",
          "[.external][hatch-oda-peer]") {
  ensureSettings();
  const char *path = std::getenv("LIBRECAD_HATCH_ODA_PEER");
  REQUIRE(path);
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromLocal8Bit(path), RS2::FormatDXFRW));
  const RS_Hatch *hatch = nullptr;
  const RS_Polyline *boundary = nullptr;
  for (const RS_Entity *entity : graphic) {
    if (entity && entity->rtti() == RS2::EntityHatch)
      hatch = static_cast<const RS_Hatch *>(entity);
    if (entity && entity->rtti() == RS2::EntityPolyline)
      boundary = static_cast<const RS_Polyline *>(entity);
  }
  REQUIRE(hatch);
  REQUIRE(boundary);
  CHECK(hatch->sourceAssociative());
  CHECK(LC_HatchAssociation::matchesClosedStraightBoundary(*hatch, *boundary));
  CHECK(graphic.dwgAdvancedMetadata().materials().size() == 3u);
  CHECK(hatch->physicalSourceRoles().hatch == 0u); // source exceeds the bounded capture
}

TEST_CASE("DXF physical hatch source roles reject conflicting records",
          "[dxf][roundtrip][filter][system-codec][hatch-source-roles]") {
  ensureSettings();
  const auto src = tmpFile("physical-hatch-source-roles.dxf");
  const std::string input =
      "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1024\n0\nENDSEC\n"
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLWPOLYLINE\n5\nB0\n102\n{ACAD_REACTORS\n330\nB1\n102\n}\n330\nA0\n"
      "100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n90\n4\n70\n1\n"
      "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
      "0\nHATCH\n5\nB1\n330\nA0\n100\nAcDbEntity\n8\n0\n100\nAcDbHatch\n"
      "10\n0\n20\n0\n30\n0\n210\n0\n220\n0\n230\n1\n2\nSOLID\n70\n1\n71\n1\n"
      "91\n1\n92\n3\n72\n0\n73\n1\n93\n4\n"
      "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
      "97\n1\n330\nB0\n75\n0\n76\n1\n98\n0\n"
      "0\nENDSEC\n0\nEOF\n";
  class SourceProbe : public RS_FilterDXFRW {
  public:
    int fault = 0;
    std::string mutationPath;
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource &source) override {
      auto altered = source;
      if (fault == 1 && !altered.records.empty()) {
        for (std::size_t i = 0; i < altered.records.front().groups.size(); ++i)
          if (altered.records.front().groups[i].code() == 330) {
            altered.records.front().rawValues[i] = "FFFF";
            break;
          }
      }
      if (fault == 2 && !altered.records.empty() && !altered.records.front().spans.empty())
        altered.records.front().spans.front().beginOffset = altered.records.front().marker.beginOffset;
      if (fault == 4) altered.scopeId = 0;
      if (fault == 5 && !altered.records.empty() && !altered.records.front().spans.empty())
        ++altered.records.front().spans.front().ordinal;
      if (fault == 6) {
        std::ofstream changed(mutationPath, std::ios::binary | std::ios::app);
        changed.put('\n');
      }
      if (fault == 7 && !altered.records.empty()) {
        for (std::size_t i = 0; i < altered.records.front().groups.size(); ++i)
          if (altered.records.front().groups[i].code() == 8) {
            altered.records.front().groups[i] = DRW_Variant(8, std::string("X"));
            altered.records.front().rawValues[i] = "X";
            break;
          }
      }
      if (fault == 8 && !altered.records.empty()) {
        for (auto &group : altered.records.front().groups)
          if (group.code() == 10 && group.type() == DRW_Variant::DOUBLE) {
            group = DRW_Variant(10, group.d_val() + 1.0);
            break;
          }
      }
      if (fault == 9) {
        for (auto &record : altered.records)
          if (record.name == "HATCH")
            for (auto &group : record.groups)
              if (group.code() == 73) { group = DRW_Variant(73, 0); break; }
      }
      RS_FilterDXFRW::addDxfEntitiesSource(altered);
      if (fault == 3) RS_FilterDXFRW::addDxfEntitiesSource(altered);
    }
  };
  const auto readRoles = [&](const std::string &text, int fault) {
    writeText(src, text);
    RS_Graphic graphic;
    SourceProbe filter;
    filter.fault = fault;
    filter.mutationPath = src;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
    for (const RS_Entity *entity : graphic)
      if (entity && entity->rtti() == RS2::EntityHatch)
        return static_cast<const RS_Hatch *>(entity)->physicalSourceRoles();
    FAIL("HATCH was not imported");
    return RS_Hatch::PhysicalSourceRoles{};
  };
  const auto roles = readRoles(input, 0);
  CHECK(roles.hatch == 0xB1u);
  CHECK(roles.boundary == 0xB0u);
  CHECK(roles.owner == 0xA0u);
  CHECK_FALSE(roles.modelSpace);
  CHECK_FALSE(roles.sourceHandleIsolation);
  CHECK(roles.sourceGeometry.size() == 4u);
  const std::vector<LC_HatchAssociation::Edge> expectedGeometry{
      {{0, 0, 0, 10}}, {{0, 0, 10, 0}}, {{0, 10, 10, 10}}, {{10, 0, 10, 10}}};
  CHECK(roles.sourceGeometry == expectedGeometry);
  const auto changedTypedGeometry = readRoles(input, 8);
  CHECK(changedTypedGeometry.hatch == 0xB1u);
  CHECK(changedTypedGeometry.sourceGeometry.empty());
  CHECK(readRoles(input, 1).hatch == 0u);
  CHECK(readRoles(input, 2).hatch == 0u);
  CHECK(readRoles(input, 3).hatch == 0u);
  CHECK(readRoles(input, 4).hatch == 0u);
  CHECK(readRoles(input, 5).hatch == 0u);
  CHECK(readRoles(input, 6).hatch == 0u);
  CHECK(readRoles(input, 7).hatch == 0u);
  auto duplicateOwner = input;
  const auto position = duplicateOwner.find("330\nA0\n100\nAcDbEntity");
  REQUIRE(position != std::string::npos);
  duplicateOwner.insert(position, "330\nA0\n");
  CHECK(readRoles(duplicateOwner, 0).hatch == 0u);
  auto extraReactor = input;
  const auto reactorPosition = extraReactor.find("330\nB1\n102\n}\n");
  REQUIRE(reactorPosition != std::string::npos);
  extraReactor.insert(reactorPosition, "330\n0\n");
  CHECK(readRoles(extraReactor, 0).hatch == 0u);
  auto nondefaultColor = input;
  const auto subclass = nondefaultColor.find("100\nAcDbHatch\n");
  REQUIRE(subclass != std::string::npos);
  nondefaultColor.insert(subclass, "62\n7\n");
  CHECK(readRoles(nondefaultColor, 0).hatch == 0u);
  auto changedBoundaryGeometry = input;
  const auto vertex = changedBoundaryGeometry.find("10\n10\n20\n0\n");
  REQUIRE(vertex != std::string::npos);
  changedBoundaryGeometry.replace(vertex, std::string("10\n10\n20\n0\n").size(), "10\n11\n20\n0\n");
  const auto unmatched = readRoles(changedBoundaryGeometry, 0);
  CHECK(unmatched.hatch == 0xB1u);
  CHECK(unmatched.sourceGeometry.empty());
  CHECK(readRoles(input, 9).sourceGeometry.empty());
  const std::string spaceSections =
      "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nBLOCK_RECORD\n5\nD0\n70\n1\n"
      "0\nBLOCK_RECORD\n5\nA0\n330\nD0\n100\nAcDbSymbolTableRecord\n"
      "100\nAcDbBlockTableRecord\n2\n*Model_Space\n70\n0\n280\n1\n281\n0\n"
      "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nBLOCKS\n"
      "0\nBLOCK\n5\nD1\n330\nA0\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockBegin\n"
      "2\n*Model_Space\n70\n0\n10\n0\n20\n0\n30\n0\n3\n*Model_Space\n1\n\n"
      "0\nENDBLK\n5\nD2\n330\nA0\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockEnd\n"
      "0\nENDSEC\n";
  auto structured = input;
  const auto entitiesPosition = structured.find("0\nSECTION\n2\nENTITIES\n");
  REQUIRE(entitiesPosition != std::string::npos);
  structured.insert(entitiesPosition, spaceSections);
  const auto structuredRoles = readRoles(structured, 0);
  CHECK(structuredRoles.modelSpace);
#ifdef LIBDXFRW_HAS_DXF_CANONICAL_HEADER_VARIABLE_QUERY
  CHECK(structuredRoles.sourceHandleIsolation);
#else
  CHECK_FALSE(structuredRoles.sourceHandleIsolation);
#endif
  for (const int code : {5, 330, 340, 481, 1005}) {
    auto externalReference = structured;
    const auto headerEnd = externalReference.find("0\nENDSEC\n");
    REQUIRE(headerEnd != std::string::npos);
    externalReference.insert(headerEnd, "9\n$DIMBLK\n" + std::to_string(code) + "\nB0\n");
    const auto externallyReferenced = readRoles(externalReference, 0);
    CHECK(externallyReferenced.modelSpace);
    CHECK_FALSE(externallyReferenced.sourceHandleIsolation);
  }
  for (const int code : {310, 1000}) {
    auto opaqueSource = structured;
    const auto headerEnd = opaqueSource.find("0\nENDSEC\n");
    REQUIRE(headerEnd != std::string::npos);
    opaqueSource.insert(headerEnd, "9\n$DIMBLK\n" + std::to_string(code) + "\nABCD\n");
    const auto opaqueRoles = readRoles(opaqueSource, 0);
    CHECK(opaqueRoles.modelSpace);
    CHECK_FALSE(opaqueRoles.sourceHandleIsolation);
  }
  auto unknownHeader = structured;
  const auto unknownHeaderEnd = unknownHeader.find("0\nENDSEC\n");
  REQUIRE(unknownHeaderEnd != std::string::npos);
  unknownHeader.insert(unknownHeaderEnd, "9\n$VENDOR_DATA\n1\nopaque\n");
  const auto unknownHeaderRoles = readRoles(unknownHeader, 0);
  CHECK(unknownHeaderRoles.modelSpace);
  CHECK_FALSE(unknownHeaderRoles.sourceHandleIsolation);
  auto customProperty = structured;
  const auto propertyEnd = customProperty.find("0\nENDSEC\n");
  REQUIRE(propertyEnd != std::string::npos);
  customProperty.insert(propertyEnd, "9\n$CUSTOMPROPERTYTAG\n1\nKEY\n9\n$CUSTOMPROPERTY\n1\nVALUE\n");
  const auto propertyRoles = readRoles(customProperty, 0);
  CHECK(propertyRoles.modelSpace);
  CHECK_FALSE(propertyRoles.sourceHandleIsolation);
  auto aliasProperty = structured;
  const auto aliasPropertyEnd = aliasProperty.find("0\nENDSEC\n");
  REQUIRE(aliasPropertyEnd != std::string::npos);
  aliasProperty.insert(aliasPropertyEnd, "9\nCUSTOMPROPERTY\n1\nVALUE\n");
  const auto aliasPropertyRoles = readRoles(aliasProperty, 0);
  CHECK(aliasPropertyRoles.modelSpace);
  CHECK_FALSE(aliasPropertyRoles.sourceHandleIsolation);
  auto unknownHeaderField = structured;
  const auto unknownFieldEnd = unknownHeaderField.find("0\nENDSEC\n");
  REQUIRE(unknownFieldEnd != std::string::npos);
  unknownHeaderField.insert(unknownFieldEnd, "9\n$DIMBLK\n300\nopaque\n");
  const auto unknownFieldRoles = readRoles(unknownHeaderField, 0);
  CHECK(unknownFieldRoles.modelSpace);
  CHECK_FALSE(unknownFieldRoles.sourceHandleIsolation);
  auto unknownClass = structured;
  const auto headerEndForClass = unknownClass.find("0\nENDSEC\n");
  REQUIRE(headerEndForClass != std::string::npos);
  const auto afterHeader = headerEndForClass + std::string("0\nENDSEC\n").size();
  unknownClass.insert(afterHeader,
      "0\nSECTION\n2\nCLASSES\n0\nCLASS\n1\nACME_UNKNOWN\n2\nAcmeUnknown\n"
      "3\nAcme\n90\n0\n91\n0\n280\n0\n281\n0\n0\nENDSEC\n");
  const auto unknownRoles = readRoles(unknownClass, 0);
  CHECK(unknownRoles.modelSpace);
  CHECK_FALSE(unknownRoles.sourceHandleIsolation);
  auto wrongSpace = structured;
  const auto modelName = wrongSpace.find("2\n*Model_Space\n70\n0\n280\n1");
  REQUIRE(modelName != std::string::npos);
  wrongSpace.replace(modelName, std::string("2\n*Model_Space").size(), "2\n*Model_SpacX");
  CHECK_FALSE(readRoles(wrongSpace, 0).modelSpace);
  auto wrongBlockOwner = structured;
  const auto blockOwner = wrongBlockOwner.find("0\nBLOCK\n5\nD1\n330\nA0\n");
  REQUIRE(blockOwner != std::string::npos);
  wrongBlockOwner.replace(blockOwner, std::string("0\nBLOCK\n5\nD1\n330\nA0\n").size(),
                          "0\nBLOCK\n5\nD1\n330\nA2\n");
  CHECK_FALSE(readRoles(wrongBlockOwner, 0).modelSpace);
  auto duplicateVersion = structured;
  const auto version = duplicateVersion.find("9\n$ACADVER\n1\nAC1024\n");
  REQUIRE(version != std::string::npos);
  duplicateVersion.insert(version, "9\n$ACADVER\n1\nAC1024\n");
  CHECK_FALSE(readRoles(duplicateVersion, 0).modelSpace);
  auto nonemptyBlock = structured;
  const auto endBlock = nonemptyBlock.find("0\nENDBLK\n5\nD2\n");
  REQUIRE(endBlock != std::string::npos);
  nonemptyBlock.insert(endBlock, "0\nPOINT\n5\nD3\n330\nA0\n100\nAcDbEntity\n8\n0\n"
                               "100\nAcDbPoint\n10\n0\n20\n0\n30\n0\n");
  CHECK_FALSE(readRoles(nonemptyBlock, 0).modelSpace);
  auto wrongVersion = structured;
  wrongVersion.replace(wrongVersion.find("AC1024"), 6u, "AC1015");
  CHECK_FALSE(readRoles(wrongVersion, 0).modelSpace);
  writeText(src, input);
  SourceProbe retry;
  retry.fault = 1;
  RS_Graphic rejected;
  REQUIRE(retry.fileImport(rejected, QString::fromStdString(src), RS2::FormatDXFRW));
  for (const RS_Entity *entity : rejected)
    if (entity && entity->rtti() == RS2::EntityHatch)
      CHECK(static_cast<const RS_Hatch *>(entity)->physicalSourceRoles().hatch == 0u);
  retry.fault = 0;
  RS_Graphic corrected;
  REQUIRE(retry.fileImport(corrected, QString::fromStdString(src), RS2::FormatDXFRW));
  for (const RS_Entity *entity : corrected)
    if (entity && entity->rtti() == RS2::EntityHatch)
      CHECK(static_cast<const RS_Hatch *>(entity)->physicalSourceRoles().hatch == 0xB1u);
  const auto endSection = input.rfind("0\nENDSEC\n0\nEOF\n");
  REQUIRE(endSection != std::string::npos);
  writeText(src, input.substr(0, endSection));
  RS_Graphic partial;
  SourceProbe partialFilter;
  CHECK_FALSE(partialFilter.fileImport(partial, QString::fromStdString(src), RS2::FormatDXFRW));
  for (const RS_Entity *entity : partial)
    if (entity && entity->rtti() == RS2::EntityHatch)
      CHECK(static_cast<const RS_Hatch *>(entity)->physicalSourceRoles().hatch == 0u);
  writeText(src, input);
  RS_Graphic afterFailure;
  REQUIRE(partialFilter.fileImport(afterFailure, QString::fromStdString(src), RS2::FormatDXFRW));
  for (const RS_Entity *entity : afterFailure)
    if (entity && entity->rtti() == RS2::EntityHatch)
      CHECK(static_cast<const RS_Hatch *>(entity)->physicalSourceRoles().hatch == 0xB1u);
  if (const char *local = std::getenv("LIBRECAD_HATCH_MODEL_SOURCE")) {
    RS_Graphic full;
    SourceProbe fullFilter;
    REQUIRE(fullFilter.fileImport(full, QString::fromLocal8Bit(local), RS2::FormatDXFRW));
    RS_Hatch *selectedHatch = nullptr;
    RS_Polyline *selectedBoundary = nullptr;
    for (RS_Entity *entity : full)
      if (entity && entity->rtti() == RS2::EntityHatch) {
        selectedHatch = static_cast<RS_Hatch *>(entity);
        const auto roles = selectedHatch->physicalSourceRoles();
        CHECK(roles.hatch == 0xA1u);
        CHECK(roles.boundary == 0xB1u);
        CHECK(roles.owner == 0x1Fu);
        CHECK(roles.modelSpace);
#ifdef LIBDXFRW_HAS_DXF_CANONICAL_HEADER_VARIABLE_QUERY
        CHECK(roles.sourceHandleIsolation);
#else
        CHECK_FALSE(roles.sourceHandleIsolation);
#endif
        CHECK(roles.sourceGeometry.size() == 4u);
      } else if (entity && entity->rtti() == RS2::EntityPolyline)
        selectedBoundary = static_cast<RS_Polyline *>(entity);
    REQUIRE(selectedHatch);
    REQUIRE(selectedBoundary);
    const auto original = selectedHatch->physicalSourceRoles().sourceGeometry;
    std::vector<LC_HatchAssociation::Edge> live;
    REQUIRE(LC_HatchAssociation::matchesClosedStraightBoundary(*selectedHatch, *selectedBoundary, &live));
    CHECK(live == original);
    selectedHatch->move(RS_Vector(1, 0));
    selectedBoundary->move(RS_Vector(1, 0));
    REQUIRE(LC_HatchAssociation::matchesClosedStraightBoundary(*selectedHatch, *selectedBoundary, &live));
    CHECK(live != original);
    CHECK(selectedHatch->physicalSourceRoles().sourceGeometry == original);
  }
  std::filesystem::remove(src);
}
#endif

TEST_CASE("DXF import retains hatch source boundary identities per loop",
          "[dxf][roundtrip][filter][handles][hatch-assoc]") {
  ensureSettings();
  const auto src = tmpFile("hatch-source-loop-identities.dxf");
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLWPOLYLINE\n5\nB0\n102\n{ACAD_REACTORS\n330\nB1\n102\n}\n330\nA0\n"
            "100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n90\n4\n70\n1\n"
            "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
            "0\nHATCH\n5\nB1\n330\nA0\n100\nAcDbEntity\n8\n0\n100\nAcDbHatch\n"
            "10\n0\n20\n0\n30\n0\n210\n0\n220\n0\n230\n1\n2\nSOLID\n70\n1\n71\n1\n"
            "91\n2\n92\n3\n72\n0\n73\n1\n93\n4\n"
            "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
            "97\n1\n330\nB0\n"
            "92\n2\n72\n0\n73\n1\n93\n4\n"
            "10\n2\n20\n2\n10\n4\n20\n2\n10\n4\n20\n4\n10\n2\n20\n4\n"
            "97\n2\n330\nB2\n330\nB3\n75\n0\n76\n1\n98\n0\n"
            "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
  RS_Hatch *hatch = nullptr;
  for (auto *entity : graphic)
    if (entity && entity->rtti() == RS2::EntityHatch)
      hatch = static_cast<RS_Hatch *>(entity);
  REQUIRE(hatch);
  const std::vector<std::vector<quint32>> expected{{0xB0u}, {0xB2u, 0xB3u}};
  CHECK(hatch->sourceLoopBoundaries() == expected);
  std::uint32_t sourceOwner = 0u;
  REQUIRE(graphic.dwgAdvancedMetadata().dxfEntitySourceOwner(0xB1u, sourceOwner));
  CHECK(sourceOwner == 0xA0u);
  REQUIRE(graphic.dwgAdvancedMetadata().dxfEntitySourceOwner(0xB0u, sourceOwner));
  CHECK(sourceOwner == 0xA0u);
  std::unique_ptr<RS_Entity> copy(hatch->clone());
  REQUIRE(copy);
  auto *copiedHatch = static_cast<RS_Hatch *>(copy.get());
  CHECK(copiedHatch->sourceLoopBoundaries() == expected);
  CHECK(copiedHatch->sourceHandle() == hatch->sourceHandle());
  copiedHatch->setSourceLoopBoundaries({{0xB4u}});
  CHECK(hatch->sourceLoopBoundaries() == expected);
  graphic.dwgAdvancedMetadata().addDxfEntitySourceOwner(0xB1u, 0xA0u);
  sourceOwner = 0x55u;
  CHECK_FALSE(graphic.dwgAdvancedMetadata().dxfEntitySourceOwner(0xB1u, sourceOwner));
  CHECK(sourceOwner == 0x55u);
  CHECK_FALSE(graphic.dwgAdvancedMetadata().dxfEntitySourceOwner(0xB2u, sourceOwner));
  LC_DwgAdvancedMetadata bounded;
  for (std::uint32_t id = 1; id <= 65536u; ++id)
    bounded.addDxfEntitySourceOwner(id, 0xA0u);
  REQUIRE(bounded.dxfEntitySourceOwner(1u, sourceOwner));
  bounded.addDxfEntitySourceOwner(65537u, 0xA0u);
  CHECK_FALSE(bounded.dxfEntitySourceOwner(1u, sourceOwner));
  bounded.clear();
  bounded.addDxfEntitySourceOwner(1u, 0xA0u);
  CHECK(bounded.dxfEntitySourceOwner(1u, sourceOwner));
  std::filesystem::remove(src);
}

TEST_CASE("DXF export reports hatch deassociation and retains unrelated reactors",
          "[dxf][roundtrip][filter][handles][system-codec][hatch-assoc]") {
  ensureSettings();
  const std::string src = tmpFile("entity-reactors-src.dxf");
  const std::string out = tmpFile("entity-reactors-out.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // The hatch names its boundary, which has both the hatch reactor and an
  // unrelated reactor. Reconstructed loops cannot preserve the association.
  writeText(src,
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLWPOLYLINE\n5\nB0\n102\n{ACAD_REACTORS\n330\nB1\n330\nB3\n330\nB2\n102\n}\n"
            "100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n90\n4\n70\n1\n"
            "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
            "0\nLINE\n5\nB2\n100\nAcDbEntity\n8\n0\n100\nAcDbLine\n"
            "10\n0\n20\n20\n30\n0\n11\n10\n21\n20\n31\n0\n"
            "0\nHATCH\n5\nB1\n100\nAcDbEntity\n8\n0\n100\nAcDbHatch\n"
            "10\n0\n20\n0\n30\n0\n210\n0\n220\n0\n230\n1\n2\nSOLID\n70\n1\n71\n1\n"
            "91\n1\n92\n3\n72\n0\n73\n1\n93\n4\n"
            "10\n0\n20\n0\n10\n10\n20\n0\n10\n10\n20\n10\n10\n0\n20\n10\n"
            "97\n1\n330\nB0\n75\n0\n76\n1\n98\n0\n"
            "0\nHATCH\n5\nB3\n100\nAcDbEntity\n8\n0\n100\nAcDbHatch\n"
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
  CHECK(graphic.dwgAdvancedMetadata().hasSourceHatchAssociation(0xB1));
  CHECK(graphic.dwgAdvancedMetadata().isSourceHatchBoundary(0xB1, 0xB0));
  CHECK(graphic.dwgAdvancedMetadata().isSourceHatchBoundary(0xB3, 0xB0));
  RS_FilterDXFRW filter;
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  const std::string sentinel = "preserve destination\n";
  writeText(out, sentinel);
  DRW_LType invalid;
  invalid.name = "Continuous";
  invalid.flags = -1;
  graphic.dwgAdvancedMetadata().addLineTypeName(invalid);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  std::ifstream retained(out, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  CHECK_FALSE(filter.exportReport().contains(QStringLiteral("associativity")));
  invalid.flags = 64;
  graphic.dwgAdvancedMetadata().addLineTypeName(invalid);
#endif
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(filter.exportReport().contains(QStringLiteral("associativity of 2 hatch")));
  CHECK(recordGroupValues(out, "HATCH", "71") == std::vector<std::string>{"0", "0"});
  CHECK(recordGroupValues(out, "HATCH", "97") == std::vector<std::string>{"0", "0"});
  const auto line = recordGroupValues(out, "LINE", "5");
  REQUIRE(line.size() == 1);
  CHECK(recordReactors(out, "LWPOLYLINE")
        == std::vector<std::vector<std::string>>{{line.front()}});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_HATCH_ASSOC_OUTPUT"))
    std::filesystem::copy_file(out, copy, std::filesystem::copy_options::overwrite_existing);

  const auto r12 = tmpFile("entity-reactors-r12.dxf");
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(r12), RS2::FormatDXFRW12));
  CHECK(filter.exportReport().contains(QStringLiteral("2 × HATCH")));
  CHECK(countRecords(r12, "HATCH") == 0);
  CHECK(danglingReferences(r12).empty());
  std::filesystem::remove(r12);

#ifdef DWGSUPPORT
  const std::string dwg = tmpFile("entity-reactors-out.dwg");
  const std::string dwgOut = tmpFile("entity-reactors-dwg-out.dxf");
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwg), RS2::FormatDWG2004));
  CHECK(filter.exportReport().contains(QStringLiteral("associativity of 2 hatch")));
  RS_Graphic fromDwg;
  RS_FilterDXFRW dwgFilter;
  REQUIRE(dwgFilter.fileImport(fromDwg, QString::fromStdString(dwg), RS2::FormatDWG));
  REQUIRE(dwgFilter.fileExport(fromDwg, QString::fromStdString(dwgOut), RS2::FormatDXFRW));
  CHECK(recordGroupValues(dwgOut, "HATCH", "71") == std::vector<std::string>{"0", "0"});
  CHECK(recordGroupValues(dwgOut, "HATCH", "97") == std::vector<std::string>{"0", "0"});
  const auto dwgLine = recordGroupValues(dwgOut, "LINE", "5");
  REQUIRE(dwgLine.size() == 1);
  CHECK(recordReactors(dwgOut, "LWPOLYLINE")
        == std::vector<std::vector<std::string>>{{dwgLine.front()}});
  std::filesystem::remove(dwg);
  std::filesystem::remove(dwgOut);
#endif

  RS_Entity *boundary = nullptr;
  for (RS_Entity *entity : graphic)
    if (entity && entity->sourceHandle() == 0xB0)
      boundary = entity;
  REQUIRE(boundary != nullptr);
  RS_Entity *sourceHatch = nullptr;
  for (RS_Entity *entity : graphic)
    if (entity && entity->sourceHandle() == 0xB1)
      sourceHatch = entity;
  REQUIRE(sourceHatch != nullptr);
  RS_Entity *boundaryCopy = boundary->clone();
  RS_Entity *hatchCopy = sourceHatch->clone();
  REQUIRE(boundaryCopy != nullptr); REQUIRE(hatchCopy != nullptr);
  REQUIRE(boundaryCopy->sourceHandle() == boundary->sourceHandle());
  REQUIRE(hatchCopy->sourceHandle() == sourceHatch->sourceHandle());
  boundaryCopy->move(RS_Vector(20.0, 0.0));
  hatchCopy->move(RS_Vector(20.0, 0.0));
  graphic.addEntity(boundaryCopy);
  graphic.addEntity(hatchCopy);
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  writeText(out, sentinel);
  invalid.flags = -1;
  graphic.dwgAdvancedMetadata().addLineTypeName(invalid);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream retained(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  invalid.flags = 64;
  graphic.dwgAdvancedMetadata().addLineTypeName(invalid);
#endif
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(filter.exportReport().contains(QStringLiteral("associativity of 3 hatch")));
  CHECK(countRecords(out, "HATCH") == 3);
  CHECK(countRecords(out, "LWPOLYLINE") == 2);
  CHECK(recordGroupValues(out, "HATCH", "71") == std::vector<std::string>{"0", "0", "0"});
  CHECK(recordGroupValues(out, "HATCH", "97") == std::vector<std::string>{"0", "0", "0"});
  const auto copyLine = recordGroupValues(out, "LINE", "5");
  REQUIRE(copyLine.size() == 1u);
  CHECK(recordReactors(out, "LWPOLYLINE") ==
        std::vector<std::vector<std::string>>{{copyLine.front()}, {copyLine.front()}});
  CHECK(danglingReferences(out).empty());
  if (const char *copy = std::getenv("LIBRECAD_KEEP_HATCH_COPIES_OUTPUT"))
    std::filesystem::copy_file(out, copy, std::filesystem::copy_options::overwrite_existing);
#ifdef DWGSUPPORT
  const auto copiedDwg = tmpFile("entity-reactors-copies.dwg");
  const auto copiedDxf = tmpFile("entity-reactors-copies-readback.dxf");
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(copiedDwg), RS2::FormatDWG2004));
  CHECK(filter.exportReport().contains(QStringLiteral("associativity of 3 hatch")));
  RS_Graphic copiedReadback;
  RS_FilterDXFRW copiedReader;
  REQUIRE(copiedReader.fileImport(copiedReadback, QString::fromStdString(copiedDwg), RS2::FormatDWG));
  REQUIRE(copiedReader.fileExport(copiedReadback, QString::fromStdString(copiedDxf), RS2::FormatDXFRW));
  CHECK(countRecords(copiedDxf, "HATCH") == 3);
  CHECK(countRecords(copiedDxf, "LWPOLYLINE") == 2);
  CHECK(recordGroupValues(copiedDxf, "HATCH", "71") == std::vector<std::string>{"0", "0", "0"});
  const auto readbackLines = recordGroupValues(copiedDxf, "LINE", "5");
  REQUIRE(readbackLines.size() == 1u);
  CHECK(recordReactors(copiedDxf, "LWPOLYLINE") ==
        std::vector<std::vector<std::string>>{{readbackLines.front()}, {readbackLines.front()}});
  CHECK(danglingReferences(copiedDxf).empty());
  if (const char *copy = std::getenv("LIBRECAD_KEEP_HATCH_COPIES_DWG_OUTPUT"))
    std::filesystem::copy_file(copiedDxf, copy, std::filesystem::copy_options::overwrite_existing);
  std::filesystem::remove(copiedDwg);
  std::filesystem::remove(copiedDxf);
#endif
  REQUIRE(graphic.removeEntity(boundaryCopy));
  REQUIRE(graphic.removeEntity(hatchCopy));
  boundary->move(RS_Vector(5.0, 0.0));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(filter.exportReport().contains(QStringLiteral("associativity of 2 hatch")));
  CHECK(recordGroupValues(out, "HATCH", "71") == std::vector<std::string>{"0", "0"});
  const auto movedLine = recordGroupValues(out, "LINE", "5");
  REQUIRE(movedLine.size() == 1);
  CHECK(recordReactors(out, "LWPOLYLINE")
        == std::vector<std::vector<std::string>>{{movedLine.front()}});
  REQUIRE(graphic.removeEntity(boundary));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(filter.exportReport().contains(QStringLiteral("associativity of 2 hatch")));
  CHECK(recordGroupValues(out, "HATCH", "71") == std::vector<std::string>{"0", "0"});
  CHECK(recordGroupValues(out, "HATCH", "97") == std::vector<std::string>{"0", "0"});
  CHECK(countRecords(out, "LWPOLYLINE") == 0);

  // An associative hatch can be imported even if its source handle is absent.
  // Its loss report must follow the hatch itself, including copies.
  std::ifstream sourceStream(src, std::ios::binary);
  std::string anonymousSource(std::istreambuf_iterator<char>(sourceStream), {});
  sourceStream.close();
  const std::string namedHatch = "0\nHATCH\n5\nB1\n";
  const auto position = anonymousSource.find(namedHatch);
  REQUIRE(position != std::string::npos);
  anonymousSource.replace(position, namedHatch.size(), "0\nHATCH\n");
  const auto noHandleSource = tmpFile("hatch-no-handle-source.dxf");
  writeText(noHandleSource, anonymousSource);
  RS_Graphic noHandleGraphic;
  RS_FilterDXFRW noHandleFilter;
  REQUIRE(noHandleFilter.fileImport(noHandleGraphic, QString::fromStdString(noHandleSource), RS2::FormatDXFRW));
  RS_Entity *noHandleHatch = nullptr;
  for (RS_Entity *entity : noHandleGraphic)
    if (entity && entity->rtti() == RS2::EntityHatch && entity->sourceHandle() == 0)
      noHandleHatch = entity;
  REQUIRE(noHandleHatch);
  RS_Entity *noHandleBoundary = nullptr;
  for (RS_Entity *entity : noHandleGraphic)
    if (entity && entity->sourceHandle() == 0xB0u) noHandleBoundary = entity;
  REQUIRE(noHandleBoundary);
  const std::string missingTargetSentinel = "preserve unidentified hatch destination";
  writeText(out, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  CHECK_FALSE(noHandleFilter.exportReport().contains(QStringLiteral("associativity")));
  const auto noHandleR12 = tmpFile("hatch-no-handle-r12.dxf");
  writeText(noHandleR12, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(noHandleR12), RS2::FormatDXFRW12));
  { std::ifstream kept(noHandleR12, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
#ifdef DWGSUPPORT
  const auto noHandleDwg = tmpFile("hatch-no-handle.dwg");
  writeText(noHandleDwg, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(noHandleDwg), RS2::FormatDWG2004));
  { std::ifstream kept(noHandleDwg, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
#endif
  // The remaining named hatch reactor is removed by the established path;
  // B2 names a unique, retained LINE. B1 has no identifiable target.
  noHandleBoundary->setReactorHandles({0xB3u, 0xB2u});
  RS_Entity *unrelatedLine = nullptr;
  for (RS_Entity *entity : noHandleGraphic)
    if (entity && entity->sourceHandle() == 0xB2u) unrelatedLine = entity;
  REQUIRE(unrelatedLine);
  RS_Entity *ambiguousLine = unrelatedLine->clone();
  REQUIRE(ambiguousLine); noHandleGraphic.addEntity(ambiguousLine);
  writeText(out, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  REQUIRE(noHandleGraphic.removeEntity(ambiguousLine));
  DRW_Dictionary collidingDictionary;
  collidingDictionary.handle = 0xB2u;
  noHandleGraphic.dwgAdvancedMetadata().addDictionary(collidingDictionary);
  writeText(out, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  auto &dictionaries = const_cast<std::vector<LC_DwgAdvancedMetadata::DictionaryRecord> &>(
      noHandleGraphic.dwgAdvancedMetadata().dictionaries());
  REQUIRE_FALSE(dictionaries.empty());
  dictionaries.pop_back();
  DRW_Group unrelatedGroup;
  unrelatedGroup.handle = 0xC0u;
  noHandleGraphic.dwgAdvancedMetadata().addGroup(unrelatedGroup);
  writeText(out, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  auto &groups = const_cast<std::vector<LC_DwgAdvancedMetadata::GroupRecord> &>(
      noHandleGraphic.dwgAdvancedMetadata().groups());
  REQUIRE_FALSE(groups.empty());
  groups.pop_back();
  // A source boundary is not an admitted target kind for an unidentified
  // hatch edge; do not guess at self-referential or unsupported graphs.
  noHandleBoundary->setReactorHandles({0xB3u, 0xB0u});
  writeText(out, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  auto *targetPoint = new RS_Point(&noHandleGraphic, RS_Vector(2, 30));
  auto *targetCircle = new RS_Circle(&noHandleGraphic, RS_CircleData(RS_Vector(4, 30), 1));
  auto *targetArc = new RS_Arc(&noHandleGraphic, RS_ArcData(RS_Vector(7, 30), 1, 0, M_PI / 2, false));
  const std::vector<std::pair<RS_Entity *, std::string>> extraTargets = {
      {targetPoint, "POINT"}, {targetCircle, "CIRCLE"}, {targetArc, "ARC"}};
  quint32 targetHandle = 0xB4u;
  for (const auto &target : extraTargets) {
    target.first->setSourceHandle(targetHandle++);
    noHandleGraphic.addEntity(target.first);
  }
  for (const auto &target : extraTargets) {
    noHandleBoundary->setReactorHandles({0xB3u, target.first->sourceHandle()});
    REQUIRE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
    const auto targetIds = recordGroupValues(out, target.second, "5");
    REQUIRE(targetIds.size() == 1u);
    CHECK(recordReactors(out, "LWPOLYLINE") == std::vector<std::vector<std::string>>{{targetIds.front()}});
    CHECK(danglingReferences(out).empty());
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_HANDLELESS_PRIMITIVE_PREFIX"))
      std::filesystem::copy_file(out, std::string(prefix) + "." + target.second + ".dxf",
                                 std::filesystem::copy_options::overwrite_existing);
#ifdef DWGSUPPORT
    const auto primitiveDwg = tmpFile("hatch-no-handle-primitive.dwg");
    const auto primitiveDxf = tmpFile("hatch-no-handle-primitive-readback.dxf");
    REQUIRE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(primitiveDwg), RS2::FormatDWG2004));
    RS_Graphic primitiveGraphic;
    RS_FilterDXFRW primitiveReader;
    REQUIRE(primitiveReader.fileImport(primitiveGraphic, QString::fromStdString(primitiveDwg), RS2::FormatDWG));
    REQUIRE(primitiveReader.fileExport(primitiveGraphic, QString::fromStdString(primitiveDxf), RS2::FormatDXFRW));
    const auto readbackIds = recordGroupValues(primitiveDxf, target.second, "5");
    REQUIRE(readbackIds.size() == 1u);
    CHECK(recordReactors(primitiveDxf, "LWPOLYLINE") == std::vector<std::vector<std::string>>{{readbackIds.front()}});
    CHECK(danglingReferences(primitiveDxf).empty());
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_HANDLELESS_PRIMITIVE_PREFIX"))
      std::filesystem::copy_file(primitiveDxf, std::string(prefix) + "." + target.second + ".dwg-readback.dxf",
                                 std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(primitiveDwg); std::filesystem::remove(primitiveDxf);
#endif
  }
  for (const auto &target : extraTargets) REQUIRE(noHandleGraphic.removeEntity(target.first));
  noHandleBoundary->setReactorHandles({0xB3u, 0xB2u});
  REQUIRE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(noHandleFilter.exportReport().contains(QStringLiteral("associativity of 2 hatch")));
  REQUIRE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(noHandleR12), RS2::FormatDXFRW12));
  CHECK(danglingReferences(noHandleR12).empty());
  std::filesystem::remove(noHandleR12);
#ifdef DWGSUPPORT
  REQUIRE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(noHandleDwg), RS2::FormatDWG2004));
  CHECK(noHandleFilter.exportReport().contains(QStringLiteral("associativity of 2 hatch")));
  RS_Graphic noHandleDwgGraphic;
  RS_FilterDXFRW noHandleDwgFilter;
  REQUIRE(noHandleDwgFilter.fileImport(noHandleDwgGraphic, QString::fromStdString(noHandleDwg), RS2::FormatDWG));
  const auto noHandleDwgDxf = tmpFile("hatch-no-handle-dwg-readback.dxf");
  REQUIRE(noHandleDwgFilter.fileExport(noHandleDwgGraphic, QString::fromStdString(noHandleDwgDxf), RS2::FormatDXFRW));
  CHECK(danglingReferences(noHandleDwgDxf).empty());
  std::filesystem::remove(noHandleDwgDxf);
  std::filesystem::remove(noHandleDwg);
#endif
  struct LateHatchMutationFilter : RS_FilterDXFRW {
    std::function<void()> beforeFinal;
    bool finalizeDxfWrite() override {
      if (beforeFinal) beforeFinal();
      return RS_FilterDXFRW::finalizeDxfWrite();
    }
    bool finalizeDwgWrite() override {
      if (beforeFinal) beforeFinal();
      return RS_FilterDXFRW::finalizeDwgWrite();
    }
  } lateHatch;
  writeText(out, missingTargetSentinel);
  lateHatch.beforeFinal = [&] { noHandleBoundary->setReactorHandles({0xB1u, 0xB3u, 0xB2u}); };
  CHECK_FALSE(lateHatch.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  noHandleBoundary->setReactorHandles({0xB3u, 0xB2u});
  lateHatch.beforeFinal = {};
  REQUIRE(lateHatch.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
#ifdef DWGSUPPORT
  const auto lateDwg = tmpFile("hatch-no-handle-late.dwg");
  writeText(lateDwg, missingTargetSentinel);
  lateHatch.beforeFinal = [&] { noHandleBoundary->setReactorHandles({0xB1u, 0xB3u, 0xB2u}); };
  CHECK_FALSE(lateHatch.fileExport(noHandleGraphic, QString::fromStdString(lateDwg), RS2::FormatDWG2004));
  { std::ifstream kept(lateDwg, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  noHandleBoundary->setReactorHandles({0xB3u, 0xB2u});
  lateHatch.beforeFinal = {};
  REQUIRE(lateHatch.fileExport(noHandleGraphic, QString::fromStdString(lateDwg), RS2::FormatDWG2004));
  std::filesystem::remove(lateDwg);
#endif
  RS_Entity *noHandleCopy = noHandleHatch->clone();
  REQUIRE(noHandleCopy); noHandleCopy->move(RS_Vector(20.0, 0.0));
  noHandleGraphic.addEntity(noHandleCopy);
  REQUIRE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(noHandleFilter.exportReport().contains(QStringLiteral("associativity of 3 hatch")));
  CHECK(recordGroupValues(out, "HATCH", "71") == std::vector<std::string>{"0", "0", "0"});
  const auto noHandleLine = recordGroupValues(out, "LINE", "5");
  REQUIRE(noHandleLine.size() == 1u);
  CHECK(recordReactors(out, "LWPOLYLINE") == std::vector<std::vector<std::string>>{{noHandleLine.front()}});
  CHECK(danglingReferences(out).empty());
  if (const char *copy = std::getenv("LIBRECAD_KEEP_HANDLELESS_HATCH_OUTPUT"))
    std::filesystem::copy_file(out, copy, std::filesystem::copy_options::overwrite_existing);

  // A pasted hatch carries its own boundary evidence even if the receiving
  // document has no source metadata for the original drawing.
  RS_Graphic receiving; receiving.initForNewDocument();
  RS_Entity *pastedHatch = noHandleHatch->clone();
  RS_Entity *pastedBoundary = noHandleBoundary->clone();
  RS_Entity *pastedLine = unrelatedLine->clone();
  REQUIRE(pastedHatch); REQUIRE(pastedBoundary); REQUIRE(pastedLine);
  for (RS_Entity *entity : {pastedHatch, pastedBoundary, pastedLine}) {
    entity->reparent(&receiving); receiving.addEntity(entity);
  }
  RS_FilterDXFRW receivingFilter;
  writeText(out, missingTargetSentinel);
  CHECK_FALSE(receivingFilter.fileExport(receiving, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  pastedBoundary->setReactorHandles({0xB2u});
  REQUIRE(receivingFilter.fileExport(receiving, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(receivingFilter.exportReport().contains(QStringLiteral("associativity of 1 hatch")));
  CHECK(danglingReferences(out).empty());
  const auto pastedLineIds = recordGroupValues(out, "LINE", "5");
  REQUIRE(pastedLineIds.size() == 1u);
  CHECK(recordReactors(out, "LWPOLYLINE") == std::vector<std::vector<std::string>>{{pastedLineIds.front()}});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_PASTED_HATCH_OUTPUT"))
    std::filesystem::copy_file(out, copy, std::filesystem::copy_options::overwrite_existing);

  // Deleting both anonymous hatches must not erase the boundary's source
  // history while an unidentified reverse reference still remains.
  noHandleBoundary->setReactorHandles({0xB1u, 0xB3u, 0xB2u});
  REQUIRE(noHandleGraphic.removeEntity(noHandleCopy));
  REQUIRE(noHandleGraphic.removeEntity(noHandleHatch));
  writeText(out, missingTargetSentinel);
  CHECK_FALSE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  { std::ifstream kept(out, std::ios::binary); CHECK(std::string(std::istreambuf_iterator<char>(kept), {}) == missingTargetSentinel); }
  noHandleBoundary->setReactorHandles({0xB3u, 0xB2u});
  REQUIRE(noHandleFilter.fileExport(noHandleGraphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(noHandleFilter.exportReport().contains(QStringLiteral("associativity of 1 hatch")));

  const auto assocFlag = anonymousSource.find("71\n1\n91\n");
  REQUIRE(assocFlag != std::string::npos);
  anonymousSource.replace(assocFlag, std::string("71\n1\n91\n").size(), "71\n0\n91\n");
  const auto boundaryLink = anonymousSource.find("97\n1\n330\nB0\n");
  REQUIRE(boundaryLink != std::string::npos);
  anonymousSource.replace(boundaryLink, std::string("97\n1\n330\nB0\n").size(), "97\n0\n");
  writeText(noHandleSource, anonymousSource);
  RS_Graphic nonAssociative;
  REQUIRE(noHandleFilter.fileImport(nonAssociative, QString::fromStdString(noHandleSource), RS2::FormatDXFRW));
  REQUIRE(noHandleFilter.fileExport(nonAssociative, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(noHandleFilter.exportReport().contains(QStringLiteral("associativity of 1 hatch")));
  std::filesystem::remove(noHandleSource);
  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF reports an unexportable hatch and removes its boundary reactor",
          "[dxf][roundtrip][filter][handles][hatch-assoc]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *hatch = new RS_Hatch(&graphic, RS_HatchData(true, 1.0, 0.0, "SOLID"));
  hatch->setSourceHandle(0xB1u);
  graphic.addEntity(hatch);
  auto *boundary = new RS_Line(&graphic, RS_LineData(RS_Vector(0, 0), RS_Vector(1, 0)));
  boundary->setSourceHandle(0xB0u);
  boundary->setReactorHandles({0xB1u, 0xB2u});
  graphic.addEntity(boundary);
  auto *unrelated = new RS_Line(&graphic, RS_LineData(RS_Vector(0, 1), RS_Vector(1, 1)));
  unrelated->setSourceHandle(0xB2u);
  graphic.addEntity(unrelated);
  graphic.dwgAdvancedMetadata().addSourceHatchBoundaries(0xB1u, {0xB0u});
  const auto out = tmpFile("unexportable_hatch_report.dxf");
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(countRecords(out, "HATCH") == 0);
  CHECK(filter.exportReport().contains(QStringLiteral("1 × HATCH")));
  const auto lines = recordGroupValues(out, "LINE", "5");
  REQUIRE(lines.size() == 2u);
  const auto reactors = recordReactors(out, "LINE");
  REQUIRE(reactors.size() == 2u);
  CHECK(reactors[0] == std::vector<std::string>{lines[1]});
  CHECK(danglingReferences(out).empty());
  if (const char *copy = std::getenv("LIBRECAD_KEEP_EMPTY_HATCH_OUTPUT"))
    std::filesystem::copy_file(out, copy, std::filesystem::copy_options::overwrite_existing);
  auto *emptyPattern = new RS_Hatch(&graphic, RS_HatchData(false, 1.0, 0.0, "ANSI31"));
  graphic.addEntity(emptyPattern);
  const auto r12 = tmpFile("unexportable_hatch_report_r12.dxf");
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(r12), RS2::FormatDXFRW12));
  CHECK(filter.exportReport().contains(QStringLiteral("2 × HATCH")));
  CHECK(countRecords(r12, "INSERT") == 0);
  CHECK(danglingReferences(r12).empty());
  REQUIRE(graphic.removeEntity(emptyPattern));
  std::filesystem::remove(r12);
#ifdef DWGSUPPORT
  const auto dwg = tmpFile("unexportable_hatch_report.dwg");
  const std::string sentinel = "preserve DWG destination";
  writeText(dwg, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(dwg), RS2::FormatDWG2004));
  std::ifstream retained(dwg, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
#endif
  REQUIRE(graphic.removeEntity(hatch));
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK_FALSE(filter.exportReport().contains(QStringLiteral("HATCH")));
#ifdef DWGSUPPORT
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwg), RS2::FormatDWG2004));
  CHECK_FALSE(filter.exportReport().contains(QStringLiteral("HATCH")));
  RS_Graphic reopened;
  RS_FilterDXFRW reader;
  REQUIRE(reader.fileImport(reopened, QString::fromStdString(dwg), RS2::FormatDWG));
  const auto readback = tmpFile("unexportable_hatch_readback.dxf");
  REQUIRE(reader.fileExport(reopened, QString::fromStdString(readback), RS2::FormatDXFRW));
  CHECK(countRecords(readback, "HATCH") == 0);
  const auto dwgLines = recordGroupValues(readback, "LINE", "5");
  const auto dwgReactors = recordReactors(readback, "LINE");
  REQUIRE(dwgLines.size() == 2u);
  REQUIRE(dwgReactors.size() == 2u);
  CHECK(dwgReactors[0] == std::vector<std::string>{dwgLines[1]});
  CHECK(danglingReferences(readback).empty());
  std::filesystem::remove(readback);
  std::filesystem::remove(dwg);
#endif
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

#ifdef DWGSUPPORT
namespace {
// The names of everything in a directory, hidden files included.
std::vector<std::string> entryNames(const std::filesystem::path &directory) {
  std::vector<std::string> names;
  for (const auto &entry : std::filesystem::directory_iterator(directory))
    names.push_back(entry.path().filename().string());
  std::sort(names.begin(), names.end());
  return names;
}
} // namespace

TEST_CASE("DWG export replaces an existing file and leaves no other file",
          "[dwg][roundtrip][filter][export]") {
  ensureSettings();
  // A directory of its own, so that whatever the export leaves beside its
  // output is seen.
  const std::filesystem::path directory = tmpFile("dwg_export_twice");
  std::filesystem::remove_all(directory);
  REQUIRE(std::filesystem::create_directory(directory));
  const QString out =
      QString::fromStdString((directory / "drawing.dwg").string());
  const std::vector<std::string> outputOnly{"drawing.dwg"};

  RS_Graphic graphic;
  graphic.initForNewDocument();
  graphic.addEntity(new RS_Line(
      &graphic, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0))));
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, out, RS2::FormatDWG));
  }
  CHECK(entryNames(directory) == outputOnly);

  // The second export meets the file of the first, as every Save of a
  // drawing after its first one does. Windows refused it: the export could
  // not remove the placeholder of its backup name while it held that open.
  graphic.addEntity(new RS_Line(
      &graphic, RS_LineData(RS_Vector(0.0, 10.0), RS_Vector(10.0, 10.0))));
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, out, RS2::FormatDWG));
  }
  CHECK(entryNames(directory) == outputOnly);

  RS_Graphic reloaded;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(reloaded, out, RS2::FormatDWG));
  }
  // Both lines: the file is the second export, not the first.
  int lines = 0;
  for (RS_Entity *e : reloaded) {
    if (e->rtti() == RS2::EntityLine)
      ++lines;
  }
  CHECK(lines == 2);

  std::filesystem::remove_all(directory);
}

TEST_CASE("DWG export does not move a directory out of its way",
          "[dwg][roundtrip][filter][export]") {
  ensureSettings();
  const std::filesystem::path directory = tmpFile("dwg_export_onto_directory");
  std::filesystem::remove_all(directory);
  // A directory has the name the export is to write.
  const std::filesystem::path taken = directory / "drawing.dwg";
  REQUIRE(std::filesystem::create_directories(taken));
  writeText((taken / "kept.txt").string(), "kept");

  RS_Graphic graphic;
  graphic.initForNewDocument();
  graphic.addEntity(new RS_Line(
      &graphic, RS_LineData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0))));
  {
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(
        graphic, QString::fromStdString(taken.string()), RS2::FormatDWG));
  }
  // Neither under a backup name with a drawing in its place, nor emptied.
  CHECK(entryNames(directory) == std::vector<std::string>{"drawing.dwg"});
  CHECK(std::filesystem::is_directory(taken));
  CHECK(entryNames(taken) == std::vector<std::string>{"kept.txt"});

  std::filesystem::remove_all(directory);
}
#endif

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
          "[system-codec][dxf][roundtrip][filter][handles]") {
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
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  CHECK(plotStyleOf("New") == std::vector<std::string>{"4A"});
#else
  CHECK(plotStyleOf("New").empty());
#endif
  CHECK(danglingReferences(out).empty());

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  // A contradictory default edge must not publish a partial destination;
  // correcting the drawing must also work through the same filter.
  std::ifstream sourceFile(src, std::ios::binary);
  std::string invalid((std::istreambuf_iterator<char>(sourceFile)), {});
  sourceFile.close();
  const auto defaultAt = invalid.find("340\n4A\n");
  REQUIRE(defaultAt != std::string::npos);
  invalid.replace(defaultAt, std::string("340\n4A\n").size(), "340\n4B\n");
  const auto badSrc = tmpFile("plot-style-invalid-src.dxf");
  writeText(badSrc, invalid);
  RS_Graphic bad;
  RS_FilterDXFRW retry;
  REQUIRE(retry.fileImport(bad, QString::fromStdString(badSrc), RS2::FormatDXFRW));
  const std::string sentinel = "preserve destination\n";
  writeText(out, sentinel);
  CHECK_FALSE(retry.fileExport(bad, QString::fromStdString(out), RS2::FormatDXFRW2000));
  std::ifstream retained(out, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  REQUIRE(retry.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW2000));
  CHECK(plotStyleOf("New") == std::vector<std::string>{"4A"});
  CHECK(danglingReferences(out).empty());
  std::filesystem::remove(badSrc);
#endif
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
          "[dxf][roundtrip][filter][handles][xdata][system-codec]") {
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
  const std::string field2004Out = tmpFile("typed_gates_r2004.dxf");
  const std::string newOut = tmpFile("typed_gates_r2007.dxf");
  std::filesystem::remove(oldOut);
  std::filesystem::remove(fieldOut);
  std::filesystem::remove(field2004Out);
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
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  CHECK(recordGroupValues(fieldOut, "FIELD", "5").empty());
  CHECK(recordGroupValues(fieldOut, "FIELDLIST", "5").empty());
  CHECK(rootDictEntries(fieldOut).count("ACAD_TYPED") == 0);
  CHECK_FALSE(containsLine(fieldOut, "{ACAD_REACTORS"));
#else
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
#endif

  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(field2004Out),
                              RS2::FormatDXFRW2004));
  }
  CHECK(recordGroupValues(field2004Out, "FIELD", "5") ==
        std::vector<std::string>{"E1"});
  CHECK(recordGroupValues(field2004Out, "FIELDLIST", "5") ==
        std::vector<std::string>{"E2"});
  CHECK(rootDictEntries(field2004Out).count("ACAD_TYPED") == 1);
  CHECK(danglingReferences(field2004Out).empty());

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
  const auto parentGroups = recordGroupsWithValue(newOut, "DICTIONARY", "5", "70");
  const auto graphKey = std::find(parentGroups.begin(), parentGroups.end(),
                                  std::make_pair(std::string("3"), std::string("GRAPH")));
  REQUIRE(graphKey != parentGroups.end());
  REQUIRE(std::next(graphKey) != parentGroups.end());
  CHECK(*std::next(graphKey) == std::make_pair(std::string("350"), std::string("E3")));
  CHECK(danglingReferences(newOut).empty());

  std::filesystem::remove(oldOut);
  std::filesystem::remove(fieldOut);
  std::filesystem::remove(field2004Out);
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

TEST_CASE("DXF reports typed records omitted by the target version",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto &metadata = graphic.dwgAdvancedMetadata();
  DRW_MLeaderStyle style; style.handle = 0xE0u; metadata.addMLeaderStyle(style);
  DRW_Field field; field.handle = 0xE1u; metadata.addField(field);
  DRW_FieldList list; list.handle = 0xE2u; metadata.addFieldList(list);
  const auto out = tmpFile("typed_gates_report_r14.dxf");
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW14));
  const auto report = filter.exportReport();
  CHECK(report.contains(QStringLiteral("1 × MLEADERSTYLE")));
  CHECK(report.contains(QStringLiteral("1 × FIELD")));
  CHECK(report.contains(QStringLiteral("1 × FIELDLIST")));
  CHECK(recordGroupValues(out, "MLEADERSTYLE", "5").empty());
  CHECK(recordGroupValues(out, "FIELD", "5").empty());
  CHECK(recordGroupValues(out, "FIELDLIST", "5").empty());
  CHECK(danglingReferences(out).empty());
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK_FALSE(filter.exportReport().contains(QStringLiteral("MLEADERSTYLE")));
  CHECK_FALSE(filter.exportReport().contains(QStringLiteral("FIELD")));
  CHECK(recordGroupValues(out, "MLEADERSTYLE", "5") == std::vector<std::string>{"E0"});
  CHECK(recordGroupValues(out, "FIELD", "5") == std::vector<std::string>{"E1"});
  CHECK(recordGroupValues(out, "FIELDLIST", "5") == std::vector<std::string>{"E2"});
  std::filesystem::remove(out);
}

TEST_CASE("DXF evaluation graph keeps its parent dictionary when emitted alone",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto &metadata = graphic.dwgAdvancedMetadata();
  DRW_EvaluationGraph graph; graph.handle = 0xE3u; graph.parentHandle = 0x70u;
  metadata.addEvaluationGraph(graph);
  DRW_Dictionary parent; parent.handle = 0x70u; parent.parentHandle = 0xCu;
  parent.name = "ACAD_GRAPH"; parent.m_entries.push_back({"GRAPH", 0xE3u});
  metadata.addDictionary(parent);
  const auto out = tmpFile("typed_graph_only.dxf");
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(rootDictEntries(out).count("ACAD_GRAPH") == 1u);
  const auto groups = recordGroupsWithValue(out, "DICTIONARY", "5", "70");
  const auto entry = std::find(groups.begin(), groups.end(),
                               std::make_pair(std::string("3"), std::string("GRAPH")));
  REQUIRE(entry != groups.end());
  REQUIRE(std::next(entry) != groups.end());
  CHECK(*std::next(entry) == std::make_pair(std::string("350"), std::string("E3")));
  CHECK(recordGroupValues(out, "EVALUATION_GRAPH", "5") == std::vector<std::string>{"E3"});
  CHECK(danglingReferences(out).empty());
  std::filesystem::remove(out);
}

TEST_CASE("DXF omission report distinguishes a different raw object at the same handle",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  for (const std::string rawName : {"MLEADERSTYLE", "mleaderstyle", "FIELD"}) {
    const bool sameType = rawName != "FIELD";
    INFO(rawName);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto &metadata = graphic.dwgAdvancedMetadata();
    DRW_MLeaderStyle style; style.handle = 0xE0u; metadata.addMLeaderStyle(style);
    DRW_RawDxfObject raw;
    raw.handle = style.handle;
    raw.name = rawName;
    raw.m_version = DRW::AC1021;
    metadata.addRawDxfObject(raw);
    const auto out = tmpFile("typed_raw_collision_report_r14.dxf");
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW14));
    const auto report = filter.exportReport();
    CHECK(report.contains(QStringLiteral("1 raw record(s)")));
    CHECK(report.contains(QStringLiteral("1 × MLEADERSTYLE")) == !sameType);
    CHECK(danglingReferences(out).empty());
    std::filesystem::remove(out);
  }
}

TEST_CASE("DXF typed stand-in does not hide a different omitted raw object",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  for (const std::string type : {"SCALE", "MLINESTYLE"}) {
    for (const std::string rawName : {"MATCH", "LOWER", "FIELD", "DUPLICATE", "RAWENTITY"}) {
      INFO(type << ' ' << rawName);
      RS_Graphic graphic;
      graphic.initForNewDocument();
      auto &metadata = graphic.dwgAdvancedMetadata();
      if (type == "SCALE") {
        DRW_Scale scale; scale.handle = 0xE0u; scale.name = "test scale";
        metadata.addScale(scale);
      } else {
        DRW_MLineStyle style; style.handle = 0xE0u; style.name = "test style";
        metadata.addMLineStyle(style);
      }
      DRW_RawDxfObject raw;
      raw.handle = 0xE0u;
      raw.name = rawName == "FIELD" ? "FIELD" :
          rawName == "LOWER" ? QString::fromStdString(type).toLower().toStdString() : type;
      raw.m_version = DRW::AC1015;
      metadata.addRawDxfObject(raw);
      if (rawName == "DUPLICATE") {
        raw.name = "FIELD";
        metadata.addRawDxfObject(raw);
      } else if (rawName == "RAWENTITY") {
        raw.name = "LINE";
        metadata.addRawDxfEntity(raw);
      }
      auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0, 0), RS_Vector(1, 0)));
      line->setReactorHandles({0xE0u});
      graphic.addEntity(line);
      const auto out = tmpFile("typed_standin_collision_report_r2007.dxf");
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
      CHECK(recordGroupValues(out, type, "5") == std::vector<std::string>{"E0"});
      const auto report = filter.exportReport();
      CHECK(report.contains(QStringLiteral("1 raw record(s)")) == (rawName == "FIELD"));
      CHECK(report.contains(QStringLiteral("2 raw record(s)")) ==
            (rawName == "DUPLICATE" || rawName == "RAWENTITY"));
      const auto references = recordGroupValues(out, "LINE", "330");
      CHECK((std::find(references.begin(), references.end(), "E0") != references.end()) ==
            (rawName != "FIELD" && rawName != "DUPLICATE" && rawName != "RAWENTITY"));
      CHECK(danglingReferences(out).empty());
      std::filesystem::remove(out);
    }
  }
}

TEST_CASE("DXF typed entity stand-ins bind only unambiguous raw identities",
          "[dxf][roundtrip][filter][version][handles]") {
  ensureSettings();
  for (const std::string scenario : {"MATCH", "OTHER", "DUPLICATE", "NONE", "LIVE"}) {
    INFO(scenario);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto &metadata = graphic.dwgAdvancedMetadata();
    DRW_NavisworksModel model;
    model.handle = 0xE0u; model.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
    metadata.addNavisworksModel(model);
    DRW_RawDxfObject raw;
    raw.handle = 0xE0u;
    raw.name = scenario == "OTHER" ? "LINE" : "NAVISWORKSMODEL";
    raw.m_version = DRW::AC1015;
    if (scenario != "NONE" && scenario != "LIVE")
      metadata.addRawDxfEntity(raw);
    if (scenario == "DUPLICATE") {
      raw.name = "FIELD";
      metadata.addRawDxfObject(raw);
    }
    auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0, 0), RS_Vector(1, 0)));
    if (scenario == "LIVE") line->setSourceHandle(0xE0u);
    line->setReactorHandles({0xE0u});
    graphic.addEntity(line);
    const auto out = tmpFile("typed_entity_standin_collision_r2007.dxf");
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
    const auto emitted = recordGroupValues(out, "NAVISWORKSMODEL", "5");
    REQUIRE(emitted.size() == 1u);
    const auto report = filter.exportReport();
    CHECK(report.contains(QStringLiteral("1 raw record(s)")) == (scenario == "OTHER"));
    CHECK(report.contains(QStringLiteral("2 raw record(s)")) == (scenario == "DUPLICATE"));
    const auto references = recordGroupValues(out, "LINE", "330");
    CHECK((std::find(references.begin(), references.end(), emitted.front()) != references.end()) ==
          (scenario == "MATCH" || scenario == "NONE"));
    CHECK(danglingReferences(out).empty());
    if (scenario == "MATCH")
      if (const char *copy = std::getenv("LIBRECAD_KEEP_NAVIS_STANDIN_OUTPUT"))
        std::filesystem::copy_file(out, copy, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(out);
  }
}

TEST_CASE("DXF typed entity handle planning retries after an invalid write",
          "[dxf][roundtrip][filter][version][handles]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto &metadata = graphic.dwgAdvancedMetadata();
  DRW_NavisworksModel model;
  model.handle = 0xE0u; model.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
  model.unitFactor = std::numeric_limits<double>::quiet_NaN();
  metadata.addNavisworksModel(model);
  DRW_RawDxfObject raw;
  raw.handle = model.handle; raw.name = "NAVISWORKSMODEL"; raw.m_version = DRW::AC1015;
  metadata.addRawDxfEntity(raw);
  auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0, 0), RS_Vector(1, 0)));
  line->setReactorHandles({model.handle});
  graphic.addEntity(line);
  const auto out = tmpFile("typed_entity_standin_retry_r2007.dxf");
  const std::string sentinel = "preserve destination";
  writeText(out, sentinel);
  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  std::ifstream existing(out, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
  existing.close();
  auto &records = const_cast<std::vector<LC_DwgAdvancedMetadata::NavisworksModelRecord> &>(
      metadata.navisworksModels());
  REQUIRE(records.size() == 1u);
  records.front().unitFactor = 1.0;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  const auto emitted = recordGroupValues(out, "NAVISWORKSMODEL", "5");
  REQUIRE(emitted.size() == 1u);
  const auto refs = recordGroupValues(out, "LINE", "330");
  CHECK(std::find(refs.begin(), refs.end(), emitted.front()) != refs.end());
  CHECK(danglingReferences(out).empty());
  std::filesystem::remove(out);
}

TEST_CASE("DXF fixed entity stand-ins retain only unambiguous identities",
          "[dxf][roundtrip][filter][version][handles]") {
  ensureSettings();
  for (const std::string type : {"GEOPOSITIONMARKER", "SECTIONOBJECT"}) {
    for (const std::string scenario : {"MATCH", "ALIAS", "OTHER", "DUPLICATE", "NONE", "LIVE"}) {
      INFO(type << ' ' << scenario);
      RS_Graphic graphic;
      graphic.initForNewDocument();
      auto &metadata = graphic.dwgAdvancedMetadata();
      if (type == "GEOPOSITIONMARKER") {
        DRW_GeoPositionMarker marker; marker.handle = 0xE0u;
        marker.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        marker.m_radius = 2.0;
        metadata.addGeoPositionMarker(marker);
      } else {
        DRW_SectionObject section; section.handle = 0xE0u;
        section.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        section.m_name = "test section"; section.m_vertDir = {0.0, 0.0, 1.0};
        section.m_verts = {{1.0, 2.0, 0.0}, {3.0, 4.0, 0.0}};
        metadata.addSectionObject(section);
      }
      DRW_RawDxfObject raw;
      raw.handle = 0xE0u; raw.m_version = DRW::AC1015;
      raw.name = scenario == "OTHER" ? "LINE" :
          scenario == "ALIAS" ? (type == "GEOPOSITIONMARKER" ? "POSITIONMARKER" : "SECTION_OBJECT") : type;
      if (scenario != "NONE" && scenario != "LIVE") metadata.addRawDxfEntity(raw);
      if (scenario == "DUPLICATE") { raw.name = "FIELD"; metadata.addRawDxfObject(raw); }
      auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0, 0), RS_Vector(1, 0)));
      if (scenario == "LIVE") line->setSourceHandle(0xE0u);
      line->setReactorHandles({0xE0u});
      graphic.addEntity(line);
      const auto out = tmpFile("fixed_entity_standin_r2013.dxf");
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW2013));
      const auto emitted = recordGroupValues(out, type, "5");
      REQUIRE(emitted.size() == 1u);
      const auto report = filter.exportReport();
      CHECK(report.contains(QStringLiteral("1 raw record(s)")) == (scenario == "OTHER"));
      CHECK(report.contains(QStringLiteral("2 raw record(s)")) == (scenario == "DUPLICATE"));
      const auto references = recordGroupValues(out, "LINE", "330");
      CHECK((std::find(references.begin(), references.end(), emitted.front()) != references.end()) ==
            (scenario == "MATCH" || scenario == "ALIAS" || scenario == "NONE"));
      CHECK(danglingReferences(out).empty());
      if (scenario == "ALIAS")
        if (const char *prefix = std::getenv("LIBRECAD_KEEP_FIXED_STANDIN_PREFIX"))
          std::filesystem::copy_file(out, std::string(prefix) + type + ".dxf",
                                     std::filesystem::copy_options::overwrite_existing);
      std::filesystem::remove(out);
    }
  }
}

TEST_CASE("DXF same-version fixed entity replay binds or drops its references",
          "[dxf][roundtrip][filter][version][handles]") {
  ensureSettings();
  for (const std::string type : {"GEOPOSITIONMARKER", "SECTIONOBJECT"}) {
    for (const std::string scenario : {"NORMAL", "ALIAS", "COLLISION"}) {
      INFO(type << ' ' << scenario);
      const bool collision = scenario == "COLLISION";
      const auto src = tmpFile("fixed_entity_same_version_source.dxf");
      const auto out = tmpFile("fixed_entity_same_version_output.dxf");
      RS_Graphic seed;
      seed.initForNewDocument();
      if (type == "GEOPOSITIONMARKER") {
        DRW_GeoPositionMarker marker; marker.handle = 0xE0u;
        marker.parentHandle = DRW::DwgModelSpaceBlockRecordHandle; marker.m_radius = 2.0;
        seed.dwgAdvancedMetadata().addGeoPositionMarker(marker);
      } else {
        DRW_SectionObject section; section.handle = 0xE0u;
        section.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
        section.m_name = "test section"; section.m_vertDir = {0.0, 0.0, 1.0};
        section.m_verts = {{1.0, 2.0, 0.0}, {3.0, 4.0, 0.0}};
        seed.dwgAdvancedMetadata().addSectionObject(section);
      }
      RS_FilterDXFRW writer;
      REQUIRE(writer.fileExport(seed, QString::fromStdString(src), RS2::FormatDXFRW2013));
      RS_Graphic loaded;
      RS_FilterDXFRW reader;
      REQUIRE(reader.fileImport(loaded, QString::fromStdString(src), RS2::FormatDXFRW));
      const auto &metadata = loaded.dwgAdvancedMetadata();
      REQUIRE((type == "GEOPOSITIONMARKER" ? metadata.geoPositionMarkers().size()
                                          : metadata.sectionObjects().size()) == 1u);
      const auto handle = type == "GEOPOSITIONMARKER" ?
          metadata.geoPositionMarkers().at(0).handle : metadata.sectionObjects().at(0).handle;
      REQUIRE(handle != 0u);
      const std::string outputKind = scenario == "ALIAS" ?
          (type == "GEOPOSITIONMARKER" ? "POSITIONMARKER" : "SECTION_OBJECT") : type;
      if (scenario == "ALIAS") {
        auto &raw = const_cast<std::vector<DRW_RawDxfObject> &>(metadata.rawDxfEntities());
        auto carrier = std::find_if(raw.begin(), raw.end(),
                                    [handle](const auto &entry) { return entry.handle == handle; });
        REQUIRE(carrier != raw.end());
        carrier->name = outputKind;
      }
      if (collision) {
        DRW_RawDxfObject other; other.handle = handle; other.name = "FIELD";
        other.m_version = DRW::AC1015;
        loaded.dwgAdvancedMetadata().addRawDxfObject(other);
      }
      auto *line = new RS_Line(&loaded, RS_LineData(RS_Vector(0, 0), RS_Vector(1, 0)));
      line->setReactorHandles({handle}); loaded.addEntity(line);
      RS_FilterDXFRW exporter;
      REQUIRE(exporter.fileExport(loaded, QString::fromStdString(out), RS2::FormatDXFRW2013));
      const auto emitted = recordGroupValues(out, outputKind, "5");
      REQUIRE(emitted.size() == 1u);
      if (scenario == "ALIAS") CHECK(recordGroupValues(out, type, "5").empty());
      const auto refs = recordGroupValues(out, "LINE", "330");
      CHECK((std::find(refs.begin(), refs.end(), emitted.front()) != refs.end()) == !collision);
      CHECK(exporter.exportReport().contains(QStringLiteral("1 raw record(s)")) == collision);
      CHECK(danglingReferences(out).empty());
      std::filesystem::remove(src); std::filesystem::remove(out);
    }
  }
}

TEST_CASE("DXF fixed entity version gates retain loss reporting and retry",
          "[dxf][roundtrip][filter][version][handles]") {
  ensureSettings();
  for (const std::string type : {"GEOPOSITIONMARKER", "SECTIONOBJECT"}) {
    INFO(type);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto &metadata = graphic.dwgAdvancedMetadata();
    if (type == "GEOPOSITIONMARKER") {
      DRW_GeoPositionMarker marker; marker.handle = 0xE0u;
      marker.parentHandle = DRW::DwgModelSpaceBlockRecordHandle; marker.m_radius = 2.0;
      metadata.addGeoPositionMarker(marker);
    } else {
      DRW_SectionObject section; section.handle = 0xE0u;
      section.parentHandle = DRW::DwgModelSpaceBlockRecordHandle;
      section.m_name = "test section"; section.m_vertDir = {0.0, 0.0, 1.0};
      section.m_verts = {{1.0, 2.0, 0.0}, {3.0, 4.0, 0.0}};
      metadata.addSectionObject(section);
    }
    DRW_RawDxfObject raw; raw.name = type; raw.handle = 0xE0u; raw.m_version = DRW::AC1032;
    metadata.addRawDxfEntity(raw);
    auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0, 0), RS_Vector(1, 0)));
    line->setReactorHandles({0xE0u}); graphic.addEntity(line);
    const auto out = tmpFile("fixed_entity_version_retry.dxf");
    const std::string sentinel = "preserve destination";
    writeText(out, sentinel);
    RS_FilterDXFRW filter;
    const auto oldFormat = type == "GEOPOSITIONMARKER" ? RS2::FormatDXFRW2010 : RS2::FormatDXFRW2004;
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), oldFormat));
    std::ifstream existing(out, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
    existing.close();
#else
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), oldFormat));
#endif
    CHECK(filter.exportReport().contains(QStringLiteral("1 raw record(s)")));
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW2013));
    CHECK(filter.exportReport().isEmpty());
    const auto emitted = recordGroupValues(out, type, "5");
    REQUIRE(emitted.size() == 1u);
    const auto refs = recordGroupValues(out, "LINE", "330");
    CHECK(std::find(refs.begin(), refs.end(), emitted.front()) != refs.end());
    CHECK(danglingReferences(out).empty());
    std::filesystem::remove(out);
  }
}

TEST_CASE("DXF refuses an unknown point-cloud definition kind and retries",
          "[dxf][roundtrip][filter][version][handles]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto &metadata = graphic.dwgAdvancedMetadata();
  DRW_PointCloudDef definition; definition.handle = 0xE0u;
  definition.m_sourceFilename = "test.rcp";
  metadata.addPointCloudDef(definition);
  auto &records = const_cast<std::vector<LC_DwgAdvancedMetadata::PointCloudDefinitionRecord> &>(
      metadata.pointCloudDefinitions());
  REQUIRE(records.size() == 1u);
  records.front().kind = 99;
  const auto out = tmpFile("unknown_pointcloud_kind_retry.dxf");
  const std::string sentinel = "preserve destination";
  writeText(out, sentinel);
  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(filter.exportReport().contains(QStringLiteral("invalid typed DXF record kind")));
  std::ifstream existing(out, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(existing), {}) == sentinel);
  existing.close();
  records.front().kind = static_cast<int>(DRW_PointCloudDef::Definition);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK_FALSE(filter.exportReport().contains(QStringLiteral("invalid typed DXF record kind")));
  CHECK(recordGroupValues(out, "POINTCLOUDDEFINITION", "5") == std::vector<std::string>{"E0"});
  CHECK(danglingReferences(out).empty());
  std::filesystem::remove(out);
}

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
TEST_CASE("DXF version-gated typed write preserves destination and retries",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  DRW_MLeaderStyle style; style.handle = 0xE0u; style.styleVersion = 1u;
  auto &metadata = graphic.dwgAdvancedMetadata();
  metadata.addMLeaderStyle(style);
  auto &records = const_cast<std::vector<LC_DwgAdvancedMetadata::MLeaderStyleRecord>&>(metadata.mleaderStyles());
  REQUIRE(records.size() == 1u);
  const auto out = tmpFile("typed_gates_retry.dxf");
  const std::string sentinel = "preserve destination\n";
  writeText(out, sentinel);
  RS_FilterDXFRW filter;
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  std::ifstream retained(out);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  records.front().styleVersion = 2u;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(recordGroupValues(out, "MLEADERSTYLE", "5") == std::vector<std::string>{"E0"});
  std::filesystem::remove(out);
}
#endif

TEST_CASE("DXF omits ambiguous live references despite an omitted raw carrier",
          "[dxf][roundtrip][filter][handles]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  for (double y : {0.0, 1.0}) {
    auto *line = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, y),
                                                    RS_Vector(1.0, y)));
    line->setSourceHandle(0xA0u);
    graphic.addEntity(line);
  }
  auto *referer = new RS_Line(&graphic, RS_LineData(RS_Vector(0.0, 2.0),
                                                   RS_Vector(1.0, 2.0)));
  referer->setSourceHandle(0xA1u);
  referer->setReactorHandles({0xA0u});
  graphic.addEntity(referer);
  DRW_RawDxfObject raw;
  raw.name = "LINE";
  raw.handle = 0xA0u;
  raw.m_version = DRW::AC1015;
  graphic.dwgAdvancedMetadata().addRawDxfEntity(raw);
  const auto out = tmpFile("ambiguous_live_handle.dxf");
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  const auto handles = recordGroupValues(out, "LINE", "5");
  REQUIRE(handles.size() == 3);
  CHECK(std::set<std::string>(handles.begin(), handles.end()).size() == 3);
  const auto reactors = recordReactors(out, "LINE");
  REQUIRE(reactors.size() == 3);
  CHECK(reactors.back().empty());
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
  CHECK(io->lastExportReport() == QStringLiteral(
      "2 raw record(s) from another DXF version were not replayed; some may have been regenerated"));

  std::filesystem::remove(src);
  std::filesystem::remove(same);
  std::filesystem::remove(other);
}

TEST_CASE("DXF cross-version report covers skipped raw default records",
          "[dxf][roundtrip][filter][version]") {
  ensureSettings();
  const auto src = tmpFile("report-regenerated-src.dxf");
  const auto out = tmpFile("report-regenerated-out.dxf");
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0\n20\n0\n30\n0\n11\n1\n21\n0\n31\n0\n"
            "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
            "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
            "3\nACAD_GROUP\n350\nD\n3\nACAD_PLOTSTYLENAME\n350\nE\n"
            "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "0\nACDBDICTIONARYWDFLT\n5\nE\n330\nC\n100\nAcDbDictionary\n281\n1\n"
            "3\nNormal\n350\nF\n100\nAcDbDictionaryWithDefault\n340\nF\n"
            "0\nACDBPLACEHOLDER\n5\nF\n330\nE\n"
            "0\nENDSEC\n0\nEOF\n");
  RS_Graphic graphic;
  RS_FilterDXFRW importer;
  REQUIRE(importer.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));
  RS_FilterDXFRW exporter;
  REQUIRE(exporter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(exporter.exportReport() == QStringLiteral(
      "2 raw record(s) from another DXF version were not replayed; some may have been regenerated"));
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  CHECK(recordGroupValues(out, "ACDBDICTIONARYWDFLT", "5") == std::vector<std::string>{"E"});
  CHECK(recordGroupValues(out, "ACDBPLACEHOLDER", "5") == std::vector<std::string>{"F"});
  CHECK(rootDictEntries(out).count("ACAD_PLOTSTYLENAME") == 1u);
#else
  CHECK(recordGroupValues(out, "ACDBDICTIONARYWDFLT", "5").empty());
  CHECK(recordGroupValues(out, "ACDBPLACEHOLDER", "5").empty());
  CHECK(rootDictEntries(out).count("ACAD_PLOTSTYLENAME") == 0u);
#endif
  CHECK(danglingReferences(out).empty());
  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF text keeps control characters and carets as caret codes",
          "[system-codec][dxf][roundtrip][filter][text]") {
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
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  const std::string expectedLayer = "A^ B";
#else
  const std::string expectedLayer = "A^B";
#endif
  CHECK(textLayers.front() == expectedLayer);
  const auto layerNames = recordGroupValues(out, "LAYER", "2");
  CHECK(std::find(layerNames.cbegin(), layerNames.cend(), expectedLayer) != layerNames.cend());

  RS_Graphic reread;
  RS_FilterDXFRW rereader;
  REQUIRE(rereader.fileImport(reread, QString::fromStdString(out), RS2::FormatDXFRW));
  CHECK(reread.findLayer(QStringLiteral("A^B")) != nullptr);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF proxy raw carriers preserve wide self handles",
          "[dxf][roundtrip][filter][proxy][handles]") {
  ensureSettings();
  const std::string src = tmpFile("proxy-wide-handle-src.dxf");
  const std::string out = tmpFile("proxy-wide-handle-out.dxf");
  const std::string out2 = tmpFile("proxy-wide-handle-retry.dxf");
  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);

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
  RS_Graphic reread;
  RS_FilterDXFRW retryFilter;
  REQUIRE(retryFilter.fileImport(reread, QString::fromStdString(out), RS2::FormatDXFRW));
  REQUIRE(retryFilter.fileExport(reread, QString::fromStdString(out2), RS2::FormatDXFRW));
  CHECK(recordGroupValues(out2, "ACAD_PROXY_ENTITY", "5")
        == std::vector<std::string>{"1234567890ABCDEF"});
  if (const char *copy = std::getenv("LIBRECAD_KEEP_WIDE_PROXY_OUTPUT"))
    std::filesystem::copy_file(out, copy, std::filesystem::copy_options::overwrite_existing);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
  std::filesystem::remove(out2);
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

  // Root dict C points at a raw-object dict (0x10) and a group dict (0x12); the
  // raw-object dict owns a raw object (0x21), the group dict owns XRECORDs (0x14,
  // 0x1F). Every one of 0x10/0x12/0x14/0x1F/0x21 is a codec-fixed structural
  // handle, so all must be remapped (and the C entries + 330 owners rewritten).
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n8\n0\n10\n0.0\n20\n0.0\n11\n1.0\n21\n1.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n3\nRAW_OBJECTS\n350\n10\n3\nMYGROUPS\n350\n12\n"
      "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "0\nDICTIONARY\n5\n10\n330\nC\n100\nAcDbDictionary\n281\n1\n3\nMyMat\n350\n21\n"
      "0\nTEST_RAW_OBJECT\n5\n21\n330\n10\n100\nTestRawObject\n1\nMyMat\n94\n7\n"
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

  // (c) referential integrity: the root dict's RAW_OBJECTS / MYGROUPS targets
  // were remapped to fresh handles that ARE emitted (not the old 0x10/0x12).
  const std::map<std::string, std::string> rootEntries = rootDictEntries(out);
  REQUIRE(rootEntries.count("RAW_OBJECTS") == 1);
  REQUIRE(rootEntries.count("MYGROUPS") == 1);
  CHECK(rootEntries.at("RAW_OBJECTS") != "10");  // remapped off the collision
  CHECK(rootEntries.at("MYGROUPS") != "12");
  CHECK(handleSet.count(rootEntries.at("RAW_OBJECTS")) == 1);  // resolves
  CHECK(handleSet.count(rootEntries.at("MYGROUPS")) == 1);

  // (d) $HANDSEED is strictly above every body handle.
  CHECK(handseedValue(out) > maxHandle(out));

  // (e) the raw object and both XRECORDs remain in the exported DXF.
  CHECK(countRecords(out, "TEST_RAW_OBJECT") >= 1);
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
  CHECK_FALSE(std::filesystem::exists(out));
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
      "0\nTEST_RAW_OBJECT\n5\n14\n330\n12\n100\nTestRawObject\n1\nTgtMat\n94\n7\n"
      // Referencing raw object (no collision) points at 0x14 via 390, 480, and
      // 481. All three must follow the target's structural-collision remap.
      "0\nTEST_RAW_OBJECT\n5\n40\n330\n12\n100\nTestRawObject\n1\nRefMat\n390\n14\n480\n14\n481\n14\n"
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
  // raw object records' fields (handle 5, name 1, refs 390/480/481).
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
    if (curType == "TEST_RAW_OBJECT") {
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
            "1001\nFIELD_EED\n1000\nfield payload\n1070\n32768\n"
            "0\nFIELDLIST\n5\nE3\n330\nC\n"
            "100\nAcDbIdSet\n90\n1\n290\n0\n330\nE0\n"
            "100\nAcDbFieldList\n"
            "1001\nFIELDLIST_EED\n1000\nfield list payload\n1070\n65535\n"
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
        == std::vector<std::string>{"32768"});
  CHECK(recordGroupValues(out, "FIELDLIST", "1001")
        == std::vector<std::string>{"FIELDLIST_EED"});
  CHECK(recordGroupValues(out, "FIELDLIST", "1000")
        == std::vector<std::string>{"field list payload"});
  CHECK(recordGroupValues(out, "FIELDLIST", "1070")
        == std::vector<std::string>{"65535"});

#ifdef DWGSUPPORT
  {
    RS_FilterDXFRW filter;
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
    // DWG EED group 1070 is signed 16-bit. Preserve the invalid-source
    // refusal and explicitly correct the two values before retrying.
    const std::string sentinel = "preserve FIELD EED destination\n";
    writeText(dwgOut, sentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(dwgOut),
                                  RS2::FormatDWG2013));
    std::ifstream retained(dwgOut, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
    retained.close();
    auto &fields = const_cast<std::vector<LC_DwgAdvancedMetadata::FieldRecord>&>(
        graphic.dwgAdvancedMetadata().fields());
    auto &lists = const_cast<std::vector<LC_DwgAdvancedMetadata::FieldListRecord>&>(
        graphic.dwgAdvancedMetadata().fieldLists());
    REQUIRE(fields.size() == 1);
    REQUIRE(lists.size() == 1);
    REQUIRE(fields.front().extData.size() == 3);
    REQUIRE(lists.front().extData.size() == 3);
    *fields.front().extData[2] = DRW_Variant(1070, -32768);
    *lists.front().extData[2] = DRW_Variant(1070, -1);
#endif
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
  CHECK(fieldExtData[2]->i_val() == -32768);
  CHECK(std::string(fieldListExtData[0]->c_str()) == "FIELDLIST_EED");
  CHECK(std::string(fieldListExtData[1]->c_str()) == "field list payload");
  CHECK(fieldListExtData[2]->i_val() == -1);
  if (const char *copy = std::getenv("LIBRECAD_KEEP_FIELD_EED_DWG"))
    std::filesystem::copy_file(dwgOut, copy, std::filesystem::copy_options::overwrite_existing);
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
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
    // The system codec refuses repeated APPID groups. It must not replace an
    // existing destination, and the same filter must work after correction.
    const std::string sentinel = "preserve malformed layer destination\n";
    writeText(out, sentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out),
                                  RS2::FormatDXFRW));
    std::ifstream retained(out, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
    retained.close();
    auto *record = const_cast<DRW_Layer *>(
        graphic.dwgAdvancedMetadata().findLayerTableEntryByName("BAD_DUPLICATE"));
    REQUIRE(record != nullptr);
    REQUIRE(record->extData.size() == 6);
    delete record->extData.back();
    record->extData.pop_back();
    delete record->extData.back();
    record->extData.pop_back();
#endif
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_DUPLICATE", "1001")
        == std::vector<std::string>{"LibreCad", "ACME_APP"});
#else
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_DUPLICATE", "1001")
        == std::vector<std::string>{"LibreCad", "ACME_APP", "LibreCad"});
#endif
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_TYPE", "1001")
        == std::vector<std::string>{"LibreCad"});
  CHECK(namedRecordGroupValues(out, "LAYER", "VALID", "1001")
        == std::vector<std::string>{"LibreCad"});
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_DUPLICATE", "1070")
        == std::vector<std::string>{"1"});
#else
  CHECK(namedRecordGroupValues(out, "LAYER", "BAD_DUPLICATE", "1070")
        == std::vector<std::string>{"1", "1"});
#endif
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

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
TEST_CASE("System codec preserves unit settings across legacy DXF refusal and retry",
          "[system-codec][dxf][units]") {
  ensureSettings();
  const std::string src = tmpFile("system_codec_units_source.dxf");
  writeText(src, dimStyleLineTypeFixture({"DASHED", "HIDDEN", "CENTER"}, kVendorDim));
  RS_Graphic graphic;
  RS_FilterDXFRW importer;
  REQUIRE(importer.fileImport(graphic, QString::fromStdString(src), RS2::FormatDXFRW));

  for (const auto &[suffix, format] :
       {std::make_pair("r12", RS2::FormatDXFRW12),
        std::make_pair("r14", RS2::FormatDXFRW14)}) {
    INFO(suffix);
    const std::string out = tmpFile((std::string("system_codec_units_") + suffix + ".dxf").c_str());
    writeText(out, "keep destination");
    graphic.setUnit(RS2::Millimeter);
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), format));
    std::ifstream before(out, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(before), {}) == "keep destination");
    before.close();
    graphic.setUnit(RS2::None);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    std::ifstream after(out, std::ios::binary);
    const std::string contents(std::istreambuf_iterator<char>(after), {});
    CHECK(contents.find("$INSUNITS") == std::string::npos);
    after.close();
    RS_Graphic reopened;
    RS_FilterDXFRW reader;
    REQUIRE(reader.fileImport(reopened, QString::fromStdString(out), RS2::FormatDXFRW));
    CHECK(reopened.getUnit() == RS2::None);
    std::filesystem::remove(out);
  }

  const std::string modern = tmpFile("system_codec_units_r2000.dxf");
  graphic.setUnit(RS2::Millimeter);
  RS_FilterDXFRW modernWriter;
  REQUIRE(modernWriter.fileExport(graphic, QString::fromStdString(modern), RS2::FormatDXFRW2000));
  RS_Graphic reopened;
  RS_FilterDXFRW modernReader;
  REQUIRE(modernReader.fileImport(reopened, QString::fromStdString(modern), RS2::FormatDXFRW));
  CHECK(reopened.getUnit() == RS2::Millimeter);
  std::filesystem::remove(modern);
  std::filesystem::remove(src);
}

TEST_CASE("System codec filter reuse keeps DWG circles in the DWG callback frame",
          "[system-codec][dxf][dwg][extrusion]") {
  ensureSettings();
  const std::string src = tmpFile("system_codec_reuse_circle.dxf");
  const std::string dwg = tmpFile("system_codec_reuse_circle.dwg");
  writeText(src, "0\nSECTION\n2\nENTITIES\n0\nCIRCLE\n8\n0\n"
                 "10\n10\n20\n20\n40\n3\n210\n0\n220\n0\n230\n-1\n"
                 "0\nENDSEC\n0\nEOF\n");
  RS_FilterDXFRW reused;
  RS_Graphic source;
  REQUIRE(reused.fileImport(source, QString::fromStdString(src), RS2::FormatDXFRW));
  REQUIRE(reused.fileExport(source, QString::fromStdString(dwg), RS2::FormatDWG2004));

  RS_Graphic reusedResult;
  REQUIRE(reused.fileImport(reusedResult, QString::fromStdString(dwg), RS2::FormatDWG));
  RS_Graphic freshResult;
  RS_FilterDXFRW fresh;
  REQUIRE(fresh.fileImport(freshResult, QString::fromStdString(dwg), RS2::FormatDWG));
  REQUIRE(reusedResult.count() == 1);
  REQUIRE(freshResult.count() == 1);
  const auto *reusedCircle = dynamic_cast<const RS_Circle *>(reusedResult.entityAt(0));
  const auto *freshCircle = dynamic_cast<const RS_Circle *>(freshResult.entityAt(0));
  REQUIRE(reusedCircle != nullptr);
  REQUIRE(freshCircle != nullptr);
  CHECK(reusedCircle->getCenter().x == Catch::Approx(freshCircle->getCenter().x));
  CHECK(reusedCircle->getCenter().y == Catch::Approx(freshCircle->getCenter().y));
  CHECK(freshCircle->getCenter().x == Catch::Approx(-10.0));
  CHECK(freshCircle->getCenter().y == Catch::Approx(20.0));
  const std::string bad = tmpFile("system_codec_reuse_bad.dxf");
  writeText(bad, "0\nSECTION\n2\nENTITIES\n0\nCIRCLE\n10\n1\n");
  RS_Graphic rejected;
  CHECK_FALSE(reused.fileImport(rejected, QString::fromStdString(bad), RS2::FormatDXFRW));
  RS_Graphic afterFailure;
  REQUIRE(reused.fileImport(afterFailure, QString::fromStdString(dwg), RS2::FormatDWG));
  REQUIRE(afterFailure.count() == 1);
  const auto *afterFailureCircle = dynamic_cast<const RS_Circle *>(afterFailure.entityAt(0));
  REQUIRE(afterFailureCircle != nullptr);
  CHECK(afterFailureCircle->getCenter().x == Catch::Approx(freshCircle->getCenter().x));
  CHECK(afterFailureCircle->getCenter().y == Catch::Approx(freshCircle->getCenter().y));
  std::filesystem::remove(src);
  std::filesystem::remove(dwg);
  std::filesystem::remove(bad);
}
#endif

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

TEST_CASE("A named DIMSTYLE that leaves a field unset follows the header, not the library default",
          "[dxf][roundtrip][filter][dimstyle]") {
  ensureSettings();
  const std::string src = tmpFile("dimstyle_dimtad_header_fallback_src.dxf");
  std::filesystem::remove(src);
  // $DIMTAD is 0 ("center text between the extension lines") in the header,
  // but the SPARSE_STYLE table entry never writes group 77 at all -- some
  // DXF writers only store a DIMSTYLE's fields that differ from the
  // drawing's own $DIM* header values, unlike AutoCAD's own writer, which
  // always writes every field.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n"
            "9\n$DIMTAD\n70\n0\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nDIMSTYLE\n5\nA\n330\n0\n"
            "100\nAcDbSymbolTable\n70\n1\n100\nAcDbDimStyleTable\n71\n0\n" +
            dimStyleRecord("31", "SPARSE_STYLE", "") +
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                            RS2::FormatDXFRW));
  graphic.onLoadingCompleted();

  LC_DimStyle *style =
      graphic.getDimStyleByName(QStringLiteral("SPARSE_STYLE"));
  REQUIRE(style != nullptr);
  // LC_DimStyle::Text's own hardcoded default is
  // ABOVE_DIM_LINE_EXCEPT_NOT_HORIZONTAL (1); the drawing's header says 0.
  CHECK(style->text()->verticalPositioning() ==
        LC_DimStyle::Text::CENTER_BETWEEN_EXT_LINES);

  std::filesystem::remove(src);
}

TEST_CASE("RS_EntityContainer::updateDimensions(true) forces a dimension "
          "back to its auto text position",
          "[dimension]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();

  RS_DimensionData dimensionData;
  dimensionData.definitionPoint = RS_Vector{50.0, 20.0};
  dimensionData.autoText = true;
  auto *dimension = new RS_DimAligned(
      &graphic, dimensionData,
      RS_DimAlignedData{RS_Vector{0.0, 0.0}, RS_Vector{100.0, 0.0}});
  graphic.addEntity(dimension);
  dimension->update();
  const RS_Vector autoPosition = dimension->getMiddleOfText();

  // Simulate a dimension whose text was explicitly placed away from the
  // auto position (as a loaded DIMENSION entity's own text_midpoint would),
  // then locked in as "not auto" by a plain update().
  const RS_Vector explicitPosition = autoPosition + RS_Vector{0.0, 25.0};
  dimension->setMiddleOfText(explicitPosition);
  dimension->updateDim(false);
  REQUIRE(dimension->getMiddleOfText().distanceTo(explicitPosition) < 1e-9);

  // updateDimensions(true) means "reposition every label automatically" --
  // it must actually reach the dimension's own autoText flag, not just call
  // its plain update(), which would keep replaying the explicit position.
  graphic.updateDimensions(true);
  CHECK(dimension->getMiddleOfText().distanceTo(autoPosition) < 1e-9);
}

namespace {

const RS_Vector kLeaderVertices[] = {{0.0, 0.0}, {10.0, 10.0}, {20.0, 10.0}};

RS_Leader *addTestLeader(RS_EntityContainer &owner, const bool arrow) {
  auto *leader = new RS_Leader(&owner, RS_LeaderData(arrow, QString()));
  for (const auto &vertex : kLeaderVertices)
    leader->addVertex(vertex);
  leader->update();
  owner.addEntity(leader);
  return leader;
}

void checkLeaderGeometry(const RS_Leader &leader, const RS_Vector &offset) {
  const int firstLine = leader.hasArrowHead() ? 1 : 0;
  REQUIRE(leader.count() == 2 + firstLine);
  if (firstLine)
    CHECK(leader.entityAt(0)->rtti() == RS2::EntitySolid);
  for (int segment = 0; segment < 2; ++segment) {
    const auto *line = leader.entityAt(segment + firstLine);
    REQUIRE(line->rtti() == RS2::EntityLine);
    CHECK(line->getStartpoint().distanceTo(kLeaderVertices[segment] + offset) < 1e-9);
    CHECK(line->getEndpoint().distanceTo(kLeaderVertices[segment + 1] + offset) < 1e-9);
  }
}

} // namespace

TEST_CASE("Dimension updates preserve leaders and ordinary dimension labels",
          "[dimension][leader][regression]") {
  ensureSettings();
  const lc::test::QuietLog quietLog;
  const bool arrow = GENERATE(false, true);
  const bool autoText = GENERATE(false, true);
  CAPTURE(arrow, autoText);
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *leader = addTestLeader(graphic, arrow);
  auto *block = new RS_Block(&graphic, RS_BlockData("CALLOUT", {0.0, 0.0}, false));
  addTestLeader(*block, arrow);
  graphic.addBlock(block);
  const RS_Vector offset{100.0, 0.0};
  auto *insert = new RS_Insert(&graphic, RS_InsertData("CALLOUT", offset,
      {1.0, 1.0}, 0.0, 1, 1, {0.0, 0.0}));
  graphic.addEntity(insert);
  RS_DimensionData data;
  data.definitionPoint = {50.0, 20.0};
  auto *dimension = new RS_DimAligned(&graphic, data, {{0.0, 0.0}, {100.0, 0.0}});
  graphic.addEntity(dimension);

  graphic.onLoadingCompleted();
  checkLeaderGeometry(*leader, {0.0, 0.0});
  REQUIRE(insert->count() == 1);
  REQUIRE(insert->entityAt(0)->rtti() == RS2::EntityDimLeader);
  const auto *insertedLeader = static_cast<const RS_Leader *>(insert->entityAt(0));
  checkLeaderGeometry(*insertedLeader, offset);

  const RS_Vector automatic = dimension->getMiddleOfText();
  const RS_Vector manual = automatic + RS_Vector(0.0, 25.0);
  dimension->setMiddleOfText(manual);
  dimension->updateDim(false);
  REQUIRE(dimension->getMiddleOfText().distanceTo(manual) < 1e-9);
  CHECK(graphic.updateDimensions(autoText) == 3);
  CHECK(dimension->getGenericData().autoText == autoText);
  CHECK(dimension->getMiddleOfText().distanceTo(autoText ? automatic : manual) < 1e-9);
  checkLeaderGeometry(*leader, {0.0, 0.0});
  checkLeaderGeometry(*insertedLeader, offset);

  leader->setVisible(false);
  CHECK(graphic.updateDimensions(autoText) == 3);
  CHECK(graphic.updateVisibleDimensions(autoText) == 2);
  checkLeaderGeometry(*leader, {0.0, 0.0});
  leader->markDeleted();
  CHECK(graphic.updateDimensions(autoText) == 2);
  checkLeaderGeometry(*leader, {0.0, 0.0});
}

TEST_CASE("Opening and saving DXF or DWG keeps leader vertices and arrow flags",
          "[system-codec][dxf][dwg][roundtrip][filter][dimension][leader][regression]") {
  ensureSettings();
  const lc::test::QuietLog quietLog;
  const auto format = GENERATE(RS2::FormatDXFRW, RS2::FormatDWG,
      RS2::FormatDWG2004, RS2::FormatDWG2007, RS2::FormatDWG2010,
      RS2::FormatDWG2013, RS2::FormatDWG2018);
  const bool arrow = GENERATE(false, true);
  CAPTURE(format, arrow);
  const lc::test::OutputDirectory directory("leader_reopen");
  const auto drawn = directory.file(format == RS2::FormatDXFRW ? "drawn.dxf" : "drawn.dwg");
  const auto saved = directory.file("saved.dxf");
  {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    addTestLeader(graphic, arrow);
    REQUIRE(lc::test::exportDrawing(graphic, drawn, format));
  }

  RS_Graphic graphic;
  REQUIRE(lc::test::importDrawing(graphic, drawn, format));
  graphic.onLoadingCompleted();
  REQUIRE(graphic.count() == 1);
  REQUIRE(graphic.entityAt(0)->rtti() == RS2::EntityDimLeader);
  checkLeaderGeometry(*static_cast<const RS_Leader *>(graphic.entityAt(0)), {0.0, 0.0});
  REQUIRE(lc::test::exportDrawing(graphic, saved, RS2::FormatDXFRW));
  const auto leaders = lc::test::dxfRecords(saved, "LEADER");
  REQUIRE(leaders.size() == 1);
  CHECK(lc::test::sameValues({"0", "10", "20"}, leaders.front().own(10)));
  CHECK(lc::test::sameValues({"0", "10", "10"}, leaders.front().own(20)));
  CHECK(lc::test::sameValues({arrow ? "1" : "0"}, leaders.front().own(71)));
  CHECK(lc::test::sameValues({"3"}, leaders.front().own(76)));
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  // The library emits these rows before the adapter's LTYPE callback. Their
  // source flags must survive the DWG-to-DXF save, including R2007+ flags.
  for (const char *name : {"ByBlock", "ByLayer", "Continuous"}) {
    const auto *source =
        graphic.dwgAdvancedMetadata().findLineTypeTableEntryByName(name);
    INFO("canonical linetype=" << name);
    REQUIRE(source != nullptr);
    if (format == RS2::FormatDWG2007 || format == RS2::FormatDWG2010 ||
        format == RS2::FormatDWG2013 || format == RS2::FormatDWG2018)
      CHECK(source->flags == 64);
    CHECK(ltypeRecordGroupValues(saved.string(), name, "70") ==
          std::vector<std::string>{std::to_string(source->flags)});
  }
#endif
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

TEST_CASE("DXF DIMSTYLE arrow blocks named only by handle resolve, even when "
          "DIMSTYLE precedes BLOCK_RECORD in the file",
          "[dxf][roundtrip][filter][dimstyle][arrow]") {
  ensureSettings();
  const std::string src = tmpFile("dimstyle_arrow_handle_only_src.dxf");
  std::filesystem::remove(src);

  // As real AutoCAD-family files do (matches
  // libredwg/test/test-data/2013/gh109_1.dxf byte for byte): the DIMSTYLE
  // table entry names its arrow block ONLY by handle (342), no legacy name
  // (5); DIMSTYLE itself comes before BLOCK_RECORD, per the DXF spec's own
  // table order, so the handle cannot be resolved until the whole file --
  // including BLOCK_RECORD -- has been read.
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nDIMSTYLE\n5\nA\n330\n0\n"
            "100\nAcDbSymbolTable\n70\n1\n100\nAcDbDimStyleTable\n71\n0\n" +
                dimStyleRecord("31", "HANDLE_ONLY", "342\n2A\n") +
                "0\nENDTAB\n"
                "0\nTABLE\n2\nBLOCK_RECORD\n5\n1\n330\n0\n100\nAcDbSymbolTable\n70\n1\n" +
                dxfBlockRecord("2A", "_ArchTick") +
                "0\nENDTAB\n0\nENDSEC\n"
                "0\nSECTION\n2\nBLOCKS\n" +
                dxfEmptyBlock("2B", "2C", "2A", "_ArchTick") +
                "0\nENDSEC\n"
                "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                            RS2::FormatDXFRW));

  LC_DimStyle *style = graphic.getDimStyleByName(QStringLiteral("HANDLE_ONLY"));
  REQUIRE(style != nullptr);
  // Before the fix this read back as the raw handle string "2A", because
  // DIMSTYLE's own parseCode() clobbered the (here nonexistent) legacy name
  // with the handle's text rather than leaving it for handle resolution.
  CHECK(style->arrowhead()->sameBlockName() == QStringLiteral("_ArchTick"));

  std::filesystem::remove(src);
}

TEST_CASE("A dimension's override arrow becomes an INSERT as soon as its "
          "block exists, without needing a reopen first",
          "[dimension][arrow]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();

  RS_DimensionData data;
  data.definitionPoint = RS_Vector(5.0, 3.0);
  data.middleOfText = RS_Vector(5.0, 3.0);
  data.style = "Standard";
  auto *dimension = new RS_DimAligned(
      &graphic, data,
      RS_DimAlignedData(RS_Vector(0.0, 0.0), RS_Vector(10.0, 0.0)));
  LC_DimStyle override;
  override.arrowhead()->setSameBlockName(QStringLiteral("_DOT"));
  dimension->setDimStyleOverride(&override);
  graphic.addEntity(dimension);
  dimension->update();

  // _DOT is not yet a block in this graphic, so the dimension can only have
  // built its own LC_DimArrow fallback for it.
  bool hasBareArrow = false;
  for (RS_Entity *child :
       lc::LC_ContainerTraverser{*dimension, RS2::ResolveNone}.entities()) {
    if (child != nullptr && child->rtti() == RS2::EntityDimArrowBlock)
      hasBareArrow = true;
    REQUIRE(child->rtti() != RS2::EntityInsert);
  }
  REQUIRE(hasBareArrow);

  // prepareForSave() inserts the override's standard arrow blocks; it must
  // also recompute the dimension so this same save draws the real block
  // instead of repeating whatever the writer can approximate a bare
  // LC_DimArrow with (or nothing, for a kind with no export primitives).
  graphic.prepareForSave();

  int insertCount = 0;
  for (RS_Entity *child :
       lc::LC_ContainerTraverser{*dimension, RS2::ResolveNone}.entities()) {
    if (child == nullptr || child->rtti() != RS2::EntityInsert)
      continue;
    ++insertCount;
    CHECK(static_cast<RS_Insert *>(child)->getName() ==
          QStringLiteral("_DOT"));
  }
  CHECK(insertCount == 2); // one per end
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
  CHECK_FALSE(std::filesystem::exists(out));
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
// match the number of 49s, or the reader rejects the file. R12 text is
// Windows-1252, so the file holds U+00D6 as the single byte D6.
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
    "0\nLTYPE\n2\n" "\xD6" "lfarbe\n70\n0\n3\nVendor non-ASCII\n72\n65\n"
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
    "0\nLINE\n8\nL_OEL\n6\n" "\xD6" "LFARBE\n"
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

TEST_CASE("Legacy DXF writes block inserts without reverse block references",
          "[dxf][roundtrip][filter][linetype][block][legacy][transaction]") {
  ensureSettings();
  const auto version = GENERATE(RS2::FormatDXFRW12, RS2::FormatDXFRW14);
  const auto source = writeFixture("legacy_insert_refs_source.dxf", kNamedR12Fixture);
  const auto first = tmpFile("legacy_insert_refs_first.dxf");
  const auto second = tmpFile("legacy_insert_refs_second.dxf");
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  RS_Entity *insert = nullptr;
  for (RS_Entity *entity : graphic) {
    if (entity != nullptr && entity->rtti() == RS2::EntityInsert) {
      insert = entity;
      break;
    }
  }
  REQUIRE(insert != nullptr);
  insert->setSourceHandle(0xABCu);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(first), version));
  RS_Graphic reread;
  REQUIRE(filter.fileImport(reread, QString::fromStdString(first), RS2::FormatDXFRW));
  RS_Graphic &retryGraphic = reread;
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  auto *layer = retryGraphic.findLayer(QStringLiteral("L_VENDOR"));
  REQUIRE(layer != nullptr);
  layer->setPrint(false);
  const std::string sentinel = "preserve legacy insert destination\n";
  writeText(second, sentinel);
  CHECK_FALSE(filter.fileExport(retryGraphic, QString::fromStdString(second), version));
  std::ifstream retained(second, std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
  retained.close();
  layer->setPrint(true);
#endif
  REQUIRE(filter.fileExport(retryGraphic, QString::fromStdString(second), version));
  CHECK(recordGroupValues(second, "INSERT", "2") == std::vector<std::string>{"B"});
  CHECK(recordGroupValues(second, "BLOCK_RECORD", "331").empty());
  const auto y = recordGroupValues(second, "INSERT", "20");
  REQUIRE(y.size() == 1);
  CHECK(std::stod(y.front()) == Catch::Approx(160.0));
  if (const char *copy = std::getenv("LIBRECAD_KEEP_LEGACY_INSERT_OUTPUT"))
    std::filesystem::copy_file(second, std::string(copy) +
        (version == RS2::FormatDXFRW12 ? "-r12.dxf" : "-r14.dxf"),
        std::filesystem::copy_options::overwrite_existing);
  RS_Graphic rereadAgain;
  REQUIRE(filter.fileImport(rereadAgain, QString::fromStdString(second), RS2::FormatDXFRW));
  CHECK(entityOnLayer(rereadAgain, QStringLiteral("L_VENDOR")) != nullptr);
  std::filesystem::remove(source);
  std::filesystem::remove(first);
  std::filesystem::remove(second);
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
          "[dxf][roundtrip][filter][polyline][linetype][named][system-codec]") {
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
  const auto parentHandles = recordGroupValues(out12, "POLYLINE", "5");
  const auto vertexHandles = recordGroupValues(out12, "VERTEX", "5");
  const auto endHandles = recordGroupValues(out12, "SEQEND", "5");
  REQUIRE(parentHandles.size() == 1);
  REQUIRE(vertexHandles.size() == 2);
  REQUIRE(endHandles.size() == 1);
  CHECK(std::set<std::string>{parentHandles.front(), vertexHandles[0],
                              vertexHandles[1], endHandles.front()}.size() == 4);

  std::filesystem::remove(out12);
}

static std::vector<double> polylineBulges(const std::string &path, bool legacy) {
  std::ifstream stream(path);
  std::string codeLine, valueLine;
  std::vector<double> bulges;
  bool inRecord = false;
  while (std::getline(stream, codeLine) && std::getline(stream, valueLine)) {
    const auto code = trimDxfToken(codeLine), value = trimDxfToken(valueLine);
    if (code == "0") {
      inRecord = value == (legacy ? "VERTEX" : "LWPOLYLINE");
      if (inRecord && legacy) bulges.push_back(0.0);
    } else if (inRecord && code == "10" && !legacy) {
      bulges.push_back(0.0);
    } else if (inRecord && code == "42") {
      if (bulges.empty()) return {};
      bulges.back() = std::stod(value);
    }
  }
  return bulges;
}

TEST_CASE("An open polyline preserves its terminal bulge through DXF saves",
          "[dxf][roundtrip][filter][polyline][bulge][system-codec]") {
  ensureSettings();
  for (const double terminalBulge : {0.0, -0.25}) {
    for (const RS2::FormatType format : {RS2::FormatDXFRW12, RS2::FormatDXFRW}) {
      RS_Graphic graphic;
      graphic.initForNewDocument();
      auto *polyline = new RS_Polyline(&graphic);
      polyline->addVertex(RS_Vector(0, 0), 0.0);
      polyline->addVertex(RS_Vector(5, 0), 0.5);
      polyline->addVertex(RS_Vector(10, 5), terminalBulge);
      graphic.addEntity(polyline);
      const auto out = tmpFile("polyline_terminal_bulge.dxf");
      const auto resaved = tmpFile("polyline_terminal_bulge_resaved.dxf");
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
      const auto verify = [&](const std::string &path) {
        const auto bulges = polylineBulges(path, format == RS2::FormatDXFRW12);
        REQUIRE(bulges.size() == 3u);
        CHECK(bulges[0] == Catch::Approx(0.0));
        CHECK(bulges[1] == Catch::Approx(0.5));
        CHECK(bulges[2] == Catch::Approx(terminalBulge));
      };
      verify(out);
      RS_Graphic reopened;
      REQUIRE(filter.fileImport(reopened, QString::fromStdString(out), RS2::FormatDXFRW));
      REQUIRE(filter.fileExport(reopened, QString::fromStdString(resaved), format));
      verify(resaved);
      if (terminalBulge != 0.0) {
        if (const char *copy = std::getenv("LIBRECAD_KEEP_TERMINAL_BULGE")) {
          const std::string prefix = std::string(copy) + (format == RS2::FormatDXFRW12 ? "-r12" : "-modern");
          std::filesystem::copy_file(out, prefix + ".dxf", std::filesystem::copy_options::overwrite_existing);
          std::filesystem::copy_file(resaved, prefix + "-resaved.dxf", std::filesystem::copy_options::overwrite_existing);
        }
      }
      RS_Polyline *reopenedPolyline = nullptr;
      for (RS_Entity *entity : reopened)
        if (entity && entity->rtti() == RS2::EntityPolyline)
          reopenedPolyline = static_cast<RS_Polyline *>(entity);
      REQUIRE(reopenedPolyline);
      reopenedPolyline->setClosed(true, 0.0);
      const auto *closing = reopenedPolyline->entityAt(reopenedPolyline->count() - 1u);
      REQUIRE(closing);
      CHECK(closing->rtti() == (terminalBulge == 0.0 ? RS2::EntityLine : RS2::EntityArc));
      if (terminalBulge != 0.0)
        CHECK(static_cast<const RS_Arc *>(closing)->getBulge() == Catch::Approx(terminalBulge));
      const auto closedCount = reopenedPolyline->count();
      reopenedPolyline->setClosed(false, 0.0);
      CHECK(reopenedPolyline->count() + 1u == closedCount);
      reopenedPolyline->setClosed(true, 0.0);
      REQUIRE(reopenedPolyline->count() == closedCount);
      closing = reopenedPolyline->entityAt(reopenedPolyline->count() - 1u);
      REQUIRE(closing);
      CHECK(closing->rtti() == (terminalBulge == 0.0 ? RS2::EntityLine : RS2::EntityArc));
      if (terminalBulge != 0.0)
        CHECK(static_cast<const RS_Arc *>(closing)->getBulge() == Catch::Approx(terminalBulge));
      std::filesystem::remove(out);
      std::filesystem::remove(resaved);
    }
  }
}

TEST_CASE("Open polyline edits update the terminal bulge before DXF export",
          "[dxf][roundtrip][filter][polyline][bulge][system-codec]") {
  ensureSettings();
  for (int edit = 0; edit != 8; ++edit) {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto *polyline = new RS_Polyline(&graphic);
    polyline->addVertex(RS_Vector(0, 0), 0.0);
    polyline->addVertex(RS_Vector(5, 0), 0.5);
    polyline->addVertex(RS_Vector(10, 5), -0.25);
    graphic.addEntity(polyline);
    if (edit == 0) polyline->mirror(RS_Vector(0, 0), RS_Vector(0, 1));
    if (edit == 1) polyline->revertDirection();
    if (edit == 2) polyline->scale(RS_Vector(0, 0), RS_Vector(-1, 1));
    if (edit == 3) polyline->removeLastVertex();
    if (edit == 4) polyline->move(RS_Vector(2, 3));
    if (edit == 5) polyline->mirror(RS_Vector(0, 0), RS_Vector(0, 0));
    if (edit == 6) { polyline->mirror(RS_Vector(0, 0), RS_Vector(0, 1));
                     polyline->mirror(RS_Vector(0, 0), RS_Vector(0, 1)); }
    if (edit == 7) { polyline->revertDirection(); polyline->revertDirection(); }
    const double expected = edit == 3 ? 0.5 : edit >= 4 ? -0.25 : 0.25;
    const auto output = tmpFile("polyline_edited_terminal_bulge.dxf");
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto bulges = polylineBulges(output, countRecords(output, "POLYLINE") != 0);
    REQUIRE(bulges.size() == (edit == 3 ? 2u : 3u));
    CHECK(bulges.back() == Catch::Approx(expected));
    if (const char *copy = std::getenv("LIBRECAD_KEEP_TERMINAL_BULGE_EDITS"))
      std::filesystem::copy_file(output, std::string(copy) + "-" + std::to_string(edit) + ".dxf",
                                 std::filesystem::copy_options::overwrite_existing);
    RS_Graphic reopened;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(output), RS2::FormatDXFRW));
    RS_Polyline *restored = nullptr;
    for (RS_Entity *entity : reopened)
      if (entity && entity->rtti() == RS2::EntityPolyline)
        restored = static_cast<RS_Polyline *>(entity);
    REQUIRE(restored);
    restored->setClosed(true, 0.0);
    const auto *closing = restored->entityAt(restored->count() - 1u);
    REQUIRE(closing); REQUIRE(closing->rtti() == RS2::EntityArc);
    CHECK(static_cast<const RS_Arc *>(closing)->getBulge() == Catch::Approx(expected));
    std::filesystem::remove(output);
  }
}

TEST_CASE("Closed polylines retain their closing segment across edits and reopening",
          "[dxf][roundtrip][filter][polyline][bulge][system-codec]") {
  ensureSettings();
  for (int edit = 0; edit != 2; ++edit) {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    auto *polyline = new RS_Polyline(&graphic);
    polyline->addVertex(RS_Vector(0, 0), 0.5);
    polyline->addVertex(RS_Vector(5, 0), 0.0);
    polyline->addVertex(RS_Vector(10, 5), -0.25);
    polyline->setClosed(true, 0.0);
    graphic.addEntity(polyline);
    REQUIRE(polyline->count() == 3u);
    if (edit == 0) polyline->revertDirection();
    else polyline->scale(RS_Vector(0, 0), RS_Vector(-1, 1));
    const auto *originalClosing = polyline->entityAt(polyline->count() - 1u);
    REQUIRE(originalClosing);
    const auto start = originalClosing->getStartpoint(), end = originalClosing->getEndpoint();
    polyline->setClosed(false, 0.0);
    REQUIRE(polyline->count() == 2u);
    polyline->setClosed(true, 0.0);
    REQUIRE(polyline->count() == 3u);
    const auto *closing = polyline->entityAt(polyline->count() - 1u);
    REQUIRE(closing); REQUIRE(closing->rtti() == RS2::EntityArc);
    CHECK(static_cast<const RS_Arc *>(closing)->getBulge() == Catch::Approx(edit == 0 ? -0.5 : 0.25));
    CHECK(closing->getStartpoint().distanceTo(start) < 1.0e-8);
    CHECK(closing->getEndpoint().distanceTo(end) < 1.0e-8);
    const auto output = tmpFile("polyline_closed_edited.dxf");
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    const auto bulges = polylineBulges(output, countRecords(output, "POLYLINE") != 0);
    REQUIRE(bulges.size() == 3u);
    CHECK(bulges.back() == Catch::Approx(edit == 0 ? -0.5 : 0.25));
    if (const char *copy = std::getenv("LIBRECAD_KEEP_CLOSED_BULGE_EDITS"))
      std::filesystem::copy_file(output, std::string(copy) + "-" + std::to_string(edit) + ".dxf",
                                 std::filesystem::copy_options::overwrite_existing);
    RS_Graphic restored;
    REQUIRE(filter.fileImport(restored, QString::fromStdString(output), RS2::FormatDXFRW));
    RS_Polyline *reopened = nullptr;
    for (RS_Entity *entity : restored)
      if (entity && entity->rtti() == RS2::EntityPolyline)
        reopened = static_cast<RS_Polyline *>(entity);
    REQUIRE(reopened); CHECK(reopened->isClosed()); CHECK(reopened->count() == 3u);
    if (edit == 1) {
      // Older LibreCAD sidecars stored only group 1040. A circular case can
      // still be reconstructed without the new explicit X radius.
      std::ifstream stream(output);
      std::string codeLine, valueLine, legacy;
      unsigned removed = 0;
      while (std::getline(stream, codeLine) && std::getline(stream, valueLine)) {
        if (trimDxfToken(codeLine) == "1041") { ++removed; continue; }
        legacy += codeLine + "\n" + valueLine + "\n";
      }
      REQUIRE(removed == 1u);
      const auto oldOutput = tmpFile("polyline_closed_legacy_sidecar.dxf");
      writeText(oldOutput, legacy);
      RS_Graphic oldGraphic;
      REQUIRE(filter.fileImport(oldGraphic, QString::fromStdString(oldOutput), RS2::FormatDXFRW));
      RS_Polyline *oldPolyline = nullptr;
      for (RS_Entity *entity : oldGraphic)
        if (entity && entity->rtti() == RS2::EntityPolyline)
          oldPolyline = static_cast<RS_Polyline *>(entity);
      REQUIRE(oldPolyline); CHECK(oldPolyline->isClosed()); CHECK(oldPolyline->count() == 3u);
      std::filesystem::remove(oldOutput);
    }
    std::filesystem::remove(output);
  }
}

TEST_CASE("A scaled polyline retains its elliptic segment on DXF reopen",
          "[dxf][roundtrip][filter][polyline][bulge][system-codec]") {
  ensureSettings();
  for (const auto scale : {RS_Vector(2, 1), RS_Vector(1, 2)}) {
    for (const double sourceBulge : {0.5, -0.5}) {
      for (const RS2::FormatType format : {RS2::FormatDXFRW12, RS2::FormatDXFRW}) {
        CAPTURE(scale.x, scale.y, sourceBulge, format);
        RS_Graphic graphic;
        graphic.initForNewDocument();
        auto *polyline = new RS_Polyline(&graphic);
        polyline->addVertex(RS_Vector(0, 0), sourceBulge);
        polyline->addVertex(RS_Vector(6, 0), 0.0);
        polyline->addVertex(RS_Vector(9, 3), 0.0);
        graphic.addEntity(polyline);
        polyline->scale(RS_Vector(0, 0), scale);
        REQUIRE(polyline->count() == 2u);
        REQUIRE(polyline->entityAt(0)->rtti() == RS2::EntityEllipse);
        const auto *original = static_cast<const RS_Ellipse *>(polyline->entityAt(0));
        const auto output = tmpFile("polyline_scaled_ellipse.dxf");
        RS_FilterDXFRW filter;
        REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), format));
        if (const char *copy = std::getenv("LIBRECAD_KEEP_SCALED_POLYLINE")) {
          const std::string suffix = std::string(scale.x > scale.y ? "-x" : "-y") +
              (sourceBulge > 0 ? "-ccw" : "-cw") +
              (format == RS2::FormatDXFRW12 ? "-r12.dxf" : "-modern.dxf");
          std::filesystem::copy_file(output, std::string(copy) + suffix,
                                     std::filesystem::copy_options::overwrite_existing);
        }
        RS_Graphic restored;
        REQUIRE(filter.fileImport(restored, QString::fromStdString(output), RS2::FormatDXFRW));
        RS_Polyline *reopened = nullptr;
        for (RS_Entity *entity : restored)
          if (entity && entity->rtti() == RS2::EntityPolyline)
            reopened = static_cast<RS_Polyline *>(entity);
        REQUIRE(reopened); REQUIRE(reopened->count() == 2u);
        REQUIRE(reopened->entityAt(0)->rtti() == RS2::EntityEllipse);
        const auto *roundtrip = static_cast<const RS_Ellipse *>(reopened->entityAt(0));
        CHECK(roundtrip->getCenter().distanceTo(original->getCenter()) < 1.0e-8);
        CHECK(roundtrip->getStartpoint().distanceTo(original->getStartpoint()) < 1.0e-8);
        CHECK(roundtrip->getEndpoint().distanceTo(original->getEndpoint()) < 1.0e-8);
        CHECK(roundtrip->getRatio() == Catch::Approx(original->getRatio()));
        CHECK(roundtrip->isReversed() == original->isReversed());
        const auto resaved = tmpFile("polyline_scaled_ellipse_resaved.dxf");
        REQUIRE(filter.fileExport(restored, QString::fromStdString(resaved), format));
        RS_Graphic second;
        REQUIRE(filter.fileImport(second, QString::fromStdString(resaved), RS2::FormatDXFRW));
        RS_Polyline *secondPolyline = nullptr;
        for (RS_Entity *entity : second)
          if (entity && entity->rtti() == RS2::EntityPolyline)
            secondPolyline = static_cast<RS_Polyline *>(entity);
        REQUIRE(secondPolyline); REQUIRE(secondPolyline->count() == 2u);
        REQUIRE(secondPolyline->entityAt(0)->rtti() == RS2::EntityEllipse);
        const auto *secondEllipse = static_cast<const RS_Ellipse *>(secondPolyline->entityAt(0));
        CHECK(secondEllipse->getCenter().distanceTo(original->getCenter()) < 1.0e-8);
        CHECK(secondEllipse->getStartpoint().distanceTo(original->getStartpoint()) < 1.0e-8);
        CHECK(secondEllipse->getEndpoint().distanceTo(original->getEndpoint()) < 1.0e-8);
        CHECK(secondEllipse->getRatio() == Catch::Approx(original->getRatio()));
        CHECK(secondEllipse->isReversed() == original->isReversed());
        std::filesystem::remove(resaved);
        std::filesystem::remove(output);
      }
    }
  }
}

TEST_CASE("A rotated elliptic polyline refuses DXF output without replacing it",
          "[dxf][roundtrip][filter][polyline][bulge][transaction][system-codec]") {
  ensureSettings();
  for (const bool inBlock : {false, true}) {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_EntityContainer *owner = &graphic;
    if (inBlock) {
      auto *block = new RS_Block(&graphic, RS_BlockData("ELLIPSE_BLOCK", RS_Vector(0, 0), false));
      REQUIRE(graphic.addBlock(block));
      owner = block;
    }
    auto *polyline = new RS_Polyline(owner);
    polyline->addVertex(RS_Vector(0, 0), 0.5);
    polyline->addVertex(RS_Vector(6, 0), 0.0);
    owner->addEntity(polyline);
    polyline->scale(RS_Vector(0, 0), RS_Vector(2, 1));
    polyline->rotate(RS_Vector(0, 0), 0.3);
    const auto output = tmpFile("polyline_rotated_ellipse_refusal.dxf");
    const std::string sentinel = "preserve rotated ellipse destination\n";
    writeText(output, sentinel);
    RS_FilterDXFRW filter;
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    { std::ifstream retained(output, std::ios::binary);
      CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
    polyline->rotate(RS_Vector(0, 0), -0.3);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
    CHECK(countRecords(output, "POLYLINE") == 1);
    std::filesystem::remove(output);
  }
}

TEST_CASE("A late rotated elliptic polyline refuses and retries DXF output",
          "[dxf][roundtrip][filter][polyline][bulge][transaction][system-codec]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  auto *polyline = new RS_Polyline(&graphic);
  polyline->addVertex(RS_Vector(0, 0), 0.5);
  polyline->addVertex(RS_Vector(6, 0), 0.0);
  graphic.addEntity(polyline);
  polyline->scale(RS_Vector(0, 0), RS_Vector(2, 1));
  struct LateRotationFilter : RS_FilterDXFRW {
    RS_Polyline *target = nullptr;
    bool rotateOnWrite = true;
    void writeEntities() override {
      if (rotateOnWrite) target->rotate(RS_Vector(0, 0), 0.3);
      RS_FilterDXFRW::writeEntities();
    }
  } filter;
  filter.target = polyline;
  const auto output = tmpFile("polyline_late_rotated_ellipse_refusal.dxf");
  const std::string sentinel = "preserve late rotated ellipse destination\n";
  writeText(output, sentinel);
  CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  { std::ifstream retained(output, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel); }
  polyline->rotate(RS_Vector(0, 0), -0.3);
  filter.rotateOnWrite = false;
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(output), RS2::FormatDXFRW));
  CHECK(countRecords(output, "POLYLINE") == 1);
  std::filesystem::remove(output);
}

TEST_CASE("An open polyline keeps every vertex on DXF re-save; a closed "
          "one is unchanged",
          "[dxf][roundtrip][filter][polyline][system-codec]") {
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

TEST_CASE("An arc dimension's arc-length symbol is written as a real ARC, "
          "not Unicode text",
          "[dxf][roundtrip][filter][dimension][arc]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();
  RS_DimensionData dimensionData;
  dimensionData.definitionPoint = RS_Vector(10.0, 0.0);
  dimensionData.middleOfText = RS_Vector(0.0, 10.0);
  dimensionData.text = "<>";
  dimensionData.style = "Standard";
  dimensionData.autoText = true;
  LC_DimArcData arcData(10.0, 5.0 * M_PI, RS_Vector(0.0, 0.0),
                        RS_Vector(0.0, 1.0), RS_Vector(1.0, 0.0));
  arcData.arcSymbol = 1; // above the text, as its own entity
  auto *dimension = new LC_DimArc(&graphic, dimensionData, arcData);
  graphic.addEntity(dimension);
  dimension->update();

  // The symbol is drawn as a one-character MText before export -- confirm
  // the fixture actually exercises the arcSymbol==1 path.
  bool foundMTextSymbol = false;
  for (RS_Entity *e : *dimension) {
    if (e != nullptr && e->rtti() == RS2::EntityMText &&
        static_cast<RS_MText *>(e)->getText() == QStringLiteral("∩")) {
      foundMTextSymbol = true;
    }
  }
  REQUIRE(foundMTextSymbol);

  const std::string out = tmpFile("dimarc_symbol.dxf");
  std::filesystem::remove(out);
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                              RS2::FormatDXFRW));
  }

  // The symbol glyph itself no longer appears anywhere in the file...
  {
    std::ifstream check(out);
    const std::string content((std::istreambuf_iterator<char>(check)),
                              std::istreambuf_iterator<char>());
    CHECK(content.find("\xE2\x88\xA9") == std::string::npos);
  }
  // ...and exactly one more ARC did, much smaller than the two dimension arcs.
  std::vector<double> radii;
  {
    std::ifstream in(out);
    std::string codeLine, valueLine;
    bool inArc = false;
    while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
      const std::string code = trimDxfToken(codeLine);
      const std::string value = trimDxfToken(valueLine);
      if (code == "0")
        inArc = value == "ARC";
      else if (inArc && code == "40")
        radii.push_back(std::stod(value));
    }
  }
  REQUIRE(radii.size() == 3);
  std::sort(radii.begin(), radii.end());
  // The two dimension arcs share the dimension's own 10.0 radius; the
  // symbol's is half the style's default text height (2.5), clearly smaller
  // but not by an exact, code-independent ratio.
  CHECK(radii[0] < radii[1] * 0.2);

  std::filesystem::remove(out);
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
          "[system-codec][dxf][roundtrip][filter][linetype][named]") {
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
  RS_Layer *layer = graphic.findLayer(QStringLiteral("L_VENDOR"));
  REQUIRE(layer != nullptr);
  CHECK(layer->getPen().getLineTypeName() == QStringLiteral("VENDOR_LAY"));
  CHECK(layer->getPen().getWidth() == RS2::WidthDefault);

  {
    RS_FilterDXFRW filter;
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
    const RS_Pen saved = layer->getPen();
    RS_Pen invalid = saved;
    invalid.setWidth(RS2::WidthByLayer);
    layer->setPen(invalid);
    const std::string sentinel = "preserve QCad layer destination\n";
    writeText(out, sentinel);
    CHECK_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW));
    std::ifstream retained(out, std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(retained), {}) == sentinel);
    retained.close();
    layer->setPen(saved);
#endif
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
  CHECK(namedRecordGroupValues(out, "LAYER", "L_VENDOR", "370") ==
        std::vector<std::string>{"-3"});
  RS_Graphic reread;
  RS_FilterDXFRW reader;
  REQUIRE(reader.fileImport(reread, QString::fromStdString(out), RS2::FormatDXFRW));
  const auto *reopenedLayer = reread.findLayer(QStringLiteral("L_VENDOR"));
  REQUIRE(reopenedLayer != nullptr);
  CHECK(reopenedLayer->getPen().getWidth() == RS2::WidthDefault);
  CHECK(reopenedLayer->getPen().getLineTypeName() == QStringLiteral("VENDOR_LAY"));

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
    CHECK_FALSE(filter.exportReport().contains(QStringLiteral("HATCH")));
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

namespace {

// A file's lines with any '\r' dropped: the ASCII DXF writer opens its
// stream in text mode, so a Windows runner writes CRLF, and a checkout may
// do the same to a golden.
std::vector<std::string> linesWithoutCR(const std::string &path) {
  std::ifstream in(path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    lines.push_back(line);
  }
  return lines;
}

std::string joinLines(const std::vector<std::string> &lines, std::size_t first,
                      std::size_t last) {
  std::string text;
  for (std::size_t i = first; i < last; ++i) {
    text += lines[i];
    text += '\n';
  }
  return text;
}

std::string textWithoutCR(const std::string &path) {
  const std::vector<std::string> lines = linesWithoutCR(path);
  return joinLines(lines, 0, lines.size());
}

// The LTYPE table of a DXF file, "0/TABLE/2/LTYPE" through "0/ENDTAB",
// verbatim but for '\r'. Empty when the file has none.
std::string ltypeTableSection(const std::string &path) {
  const std::vector<std::string> lines = linesWithoutCR(path);
  const auto is = [&lines](std::size_t i, const char *value) {
    return i < lines.size() && trimDxfToken(lines[i]) == value;
  };
  for (std::size_t i = 0; i + 3 < lines.size(); ++i) {
    if (!(is(i, "0") && is(i + 1, "TABLE") && is(i + 2, "2") &&
          is(i + 3, "LTYPE")))
      continue;
    // Group code and value alternate, so stepping by two stays on codes.
    for (std::size_t j = i + 4; j + 1 < lines.size(); j += 2) {
      if (is(j, "0") && is(j + 1, "ENDTAB"))
        return joinLines(lines, i, j + 2);
    }
    return joinLines(lines, i, lines.size());
  }
  return std::string();
}

// Where two texts first differ, for the failure message.
std::string firstDifference(const std::string &expected,
                            const std::string &actual) {
  std::istringstream a(expected), b(actual);
  std::string x, y;
  for (int line = 1;; ++line) {
    const bool gotX = static_cast<bool>(std::getline(a, x));
    const bool gotY = static_cast<bool>(std::getline(b, y));
    if (!gotX && !gotY)
      return "no difference";
    if (!gotX || !gotY || x != y)
      return "line " + std::to_string(line) + ": golden '" +
             (gotX ? x : std::string("<end>")) + "', written '" +
             (gotY ? y : std::string("<end>")) + "'";
  }
}

// Group 73 and the group 40 literal of the 35 writeLType() calls at the head
// of writeLTypes() as of 86d2c9325 (rs_filterdxfrw.cpp:19100-19169), in call
// order. 17 of the lengths are not the |dash| sum of their pattern as a
// double, so a table that re-summed would change these records' DWG bytes.
struct BuiltinLTypeLength {
  const char *name;
  int size;
  double length;
};

const BuiltinLTypeLength kBuiltinLTypeLengths[] = {
    {"CONTINUOUS", 0, 0.0},
    {"ByLayer", 0, 0.0},
    {"ByBlock", 0, 0.0},
    {"DOT", 2, 6.35},
    {"DOTTINY", 2, 0.9525},
    {"DOT2", 2, 3.175},
    {"DOTX2", 2, 12.7},
    {"DASHED", 2, 19.05},
    {"DASHEDTINY", 2, 2.8575},
    {"DASHED2", 2, 9.525},
    {"DASHEDX2", 2, 38.1},
    {"HIDDEN", 2, 9.525},
    {"HIDDENTINY", 2, 1.42875},
    {"HIDDEN2", 2, 4.7625},
    {"HIDDENX2", 2, 19.05},
    {"DASHDOT", 4, 25.4},
    {"DASHDOTTINY", 4, 3.81},
    {"DASHDOT2", 4, 12.7},
    {"DASHDOTX2", 4, 50.8},
    {"DIVIDE", 6, 31.75},
    {"DIVIDETINY", 6, 4.7625},
    {"DIVIDE2", 6, 15.875},
    {"DIVIDEX2", 6, 63.5},
    {"BORDER", 6, 44.45},
    {"BORDERTINY", 6, 6.6675},
    {"BORDER2", 6, 22.225},
    {"BORDERX2", 6, 88.9},
    {"CENTER", 4, 50.8},
    {"CENTERTINY", 4, 7.62},
    {"CENTER2", 4, 28.575},
    {"CENTERX2", 4, 101.6},
    {"PHANTOM", 6, 63.5},
    {"PHANTOMTINY", 6, 9.525},
    {"PHANTOM2", 6, 31.75},
    {"PHANTOMX2", 6, 127.0},
};

} // namespace

#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
TEST_CASE("R12 DXF reports an omitted ellipse setting and supports retry",
          "[dxf][roundtrip][filter][system-codec][r12][linetype]") {
  ensureSettings();
  const std::string source = writeFixture("legacy_pellipse_source.dxf", kNamedR12Fixture);
  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
  const std::string out = tmpFile("legacy_pellipse_out.dxf");
  graphic.addVariable("$PELLIPSE", 1, 70);
  writeText(out, "existing destination\n");
  REQUIRE_FALSE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW12));
  CHECK(textWithoutCR(out) == "existing destination\n");
  graphic.addVariable("$PELLIPSE", 0, 70);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW12));
  CHECK(filter.exportReport().contains(QStringLiteral("PELLIPSE")));
  CHECK(graphic.getVariableDict().value("$PELLIPSE").getInt() == 0);
  CHECK(textWithoutCR(out).find("$PELLIPSE") == std::string::npos);
  graphic.addVariable("$PELLIPSE", 1, 70);
  REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), RS2::FormatDXFRW14));
  CHECK_FALSE(filter.exportReport().contains(QStringLiteral("PELLIPSE")));
  CHECK(textWithoutCR(out).find("$PELLIPSE") != std::string::npos);
  std::filesystem::remove(out);
  std::filesystem::remove(source);
}

TEST_CASE("Legacy DXF reports omitted default angular precision and supports retry",
          "[dxf][roundtrip][filter][system-codec][r12][linetype]") {
  ensureSettings();
  const std::string source = writeFixture("legacy_angular_precision_source.dxf", kNamedR12Fixture);
  for (const RS2::FormatType format : {RS2::FormatDXFRW12, RS2::FormatDXFRW14}) {
    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    const std::string out = tmpFile("legacy_angular_precision_out.dxf");
    graphic.addVariable("$DIMADEC", 3, 70);
    writeText(out, "existing destination\n");
    REQUIRE_FALSE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(textWithoutCR(out) == "existing destination\n");
    CHECK_FALSE(filter.exportReport().contains(QStringLiteral("angular dimension precision")));
    graphic.addVariable("$DIMADEC", 0, 70);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(filter.exportReport().contains(QStringLiteral("angular dimension precision")));
    CHECK(graphic.getVariableDict().value("$DIMADEC").getInt() == 0);
    CHECK(textWithoutCR(out).find("$DIMADEC") == std::string::npos);
    std::filesystem::remove(out);
  }
  std::filesystem::remove(source);
}

TEST_CASE("Legacy DXF reports omitted default paper scale and supports retry",
          "[dxf][roundtrip][filter][system-codec][r12][linetype]") {
  ensureSettings();
  const std::string source = writeFixture("legacy_paper_scale_source.dxf", kNamedR12Fixture);
  for (const RS2::FormatType format : {RS2::FormatDXFRW12, RS2::FormatDXFRW14}) {
    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    const std::string out = tmpFile("legacy_paper_scale_out.dxf");
    graphic.addVariable("$PSVPSCALE", 2.0, 40);
    writeText(out, "existing destination\n");
    REQUIRE_FALSE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(textWithoutCR(out) == "existing destination\n");
    CHECK_FALSE(filter.exportReport().contains(QStringLiteral("paper-space scale")));
    graphic.addVariable("$PSVPSCALE", 1.0, 40);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(filter.exportReport().contains(QStringLiteral("paper-space scale")));
    CHECK(graphic.getVariableDict().value("$PSVPSCALE").getDouble() == 1.0);
    CHECK(textWithoutCR(out).find("$PSVPSCALE") == std::string::npos);
    std::filesystem::remove(out);
  }
  std::filesystem::remove(source);
}

TEST_CASE("Legacy DXF reports omitted generated cap and join settings",
          "[dxf][roundtrip][filter][system-codec][r12][linetype]") {
  ensureSettings();
  const std::string source = writeFixture("legacy_header_styles_source.dxf", kNamedR12Fixture);
  for (const RS2::FormatType format : {RS2::FormatDXFRW12, RS2::FormatDXFRW14}) {
    RS_Graphic graphic;
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(source), RS2::FormatDXFRW));
    const std::string out = tmpFile("legacy_header_styles_out.dxf");
    graphic.addVariable("$ENDCAPS", 2, 280);
    writeText(out, "existing destination\n");
    REQUIRE_FALSE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(textWithoutCR(out) == "existing destination\n");
    CHECK_FALSE(filter.exportReport().contains(QStringLiteral("line cap or join")));
    graphic.addVariable("$ENDCAPS", 1, 280);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(filter.exportReport().contains(QStringLiteral("2 line cap or join")));
    CHECK(graphic.getVariableDict().value("$ENDCAPS").getInt() == 1);
    CHECK(graphic.getVariableDict().value("$JOINSTYLE").getInt() == 1);
    CHECK(textWithoutCR(out).find("$ENDCAPS") == std::string::npos);
    CHECK(textWithoutCR(out).find("$JOINSTYLE") == std::string::npos);
    std::filesystem::remove(out);
  }
  std::filesystem::remove(source);
}

TEST_CASE("Legacy DXF reports omitted layer lineweight and supports retry",
          "[dxf][roundtrip][filter][system-codec][r12]") {
  ensureSettings();
  for (const RS2::FormatType format : {RS2::FormatDXFRW12, RS2::FormatDXFRW14}) {
    RS_Graphic graphic;
    graphic.initForNewDocument();
    RS_Layer *layer = graphic.findLayer(QStringLiteral("0"));
    REQUIRE(layer != nullptr);
    REQUIRE(layer->getPen().getWidth() == RS2::Width07);
    auto *otherLayer = new RS_Layer(QStringLiteral("CUSTOM"));
    RS_Pen otherPen = otherLayer->getPen();
    otherPen.setWidth(RS2::Width20);
    otherLayer->setPen(otherPen);
    graphic.addLayer(otherLayer);
    const std::string out = tmpFile("legacy_layer_width_retry.dxf");
    writeText(out, "existing destination\n");
    RS_FilterDXFRW filter;

    // Plot-off is still an unsupported legacy property. Its refusal must not
    // publish a partial file or poison a corrected retry on the same filter.
    layer->setPrint(false);
    REQUIRE_FALSE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(textWithoutCR(out) == "existing destination\n");
    CHECK_FALSE(filter.exportReport().contains(QStringLiteral("lineweight")));
    layer->setPrint(true);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK(filter.exportReport().contains(QStringLiteral("lineweight of 2 layer")));
    CHECK(recordGroupValues(out, "LAYER", "370").empty());
    CHECK(layer->getPen().getWidth() == RS2::Width07);
    CHECK(otherLayer->getPen().getWidth() == RS2::Width20);

    RS_Graphic reopened;
    REQUIRE(filter.fileImport(reopened, QString::fromStdString(out), format));
    const RS_Layer *reopenedLayer = reopened.findLayer(QStringLiteral("0"));
    REQUIRE(reopenedLayer != nullptr);
    CHECK(reopenedLayer->getPen().getWidth() == RS2::WidthDefault);
    const RS_Layer *reopenedOther = reopened.findLayer(QStringLiteral("CUSTOM"));
    REQUIRE(reopenedOther != nullptr);
    CHECK(reopenedOther->getPen().getWidth() == RS2::WidthDefault);

    RS_Pen pen = layer->getPen();
    pen.setWidth(RS2::WidthDefault);
    layer->setPen(pen);
    otherPen.setWidth(RS2::WidthDefault);
    otherLayer->setPen(otherPen);
    REQUIRE(filter.fileExport(graphic, QString::fromStdString(out), format));
    CHECK_FALSE(filter.exportReport().contains(QStringLiteral("lineweight")));
    std::filesystem::remove(out);
  }
}
#endif

// The goldens are the LTYPE table this export wrote on 86d2c9325, before the
// head of writeLTypes() was rewritten: the three records libdxfrw writes
// itself and the 32 the filter adds, handles in allocation order. R2000 and
// R2007 write the same table and share a golden. They pin names, order,
// groups 3, 72, 73 and 49. The bundled path retains the original byte
// snapshot; the system path also checks the declared group-40 totals exactly.
TEST_CASE("A new drawing writes the built-in LTYPE table as before",
          "[dxf][roundtrip][filter][linetype][ltype][named]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();

  struct Target {
    RS2::FormatType format;
    const char *golden;
  };
  for (const Target &target :
       {Target{RS2::FormatDXFRW, "ltype_table_ac1015_ac1021.dxf"},
        Target{RS2::FormatDXFRW2000, "ltype_table_ac1015_ac1021.dxf"},
        Target{RS2::FormatDXFRW12, "ltype_table_ac1009.dxf"}}) {
    INFO("format " << static_cast<int>(target.format) << ", golden "
                   << target.golden);
    const std::string out = tmpFile("builtin_ltype_table.dxf");
    std::filesystem::remove(out);
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(out),
                                target.format));
    }
    REQUIRE(countRecords(out, "LTYPE") == 35);

    const std::string golden =
        std::string(LIBRECAD_TEST_DIR) + "/dxf/" + target.golden;
    REQUIRE(std::filesystem::exists(golden));
    const std::string expected = textWithoutCR(golden);
    const std::string written = ltypeTableSection(out);
    REQUIRE_FALSE(written.empty());
    INFO(firstDifference(expected, written));
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
    // The installed writer reports the real table count, allocates its own
    // handles, and emits enough decimal digits to round-trip each double.
    // Compare every other field with the legacy snapshot semantically.
    const auto groups = [](const std::string &text) {
      std::istringstream stream(text);
      std::vector<std::pair<int, std::string>> result;
      std::string code, value;
      while (std::getline(stream, code) && std::getline(stream, value))
        result.emplace_back(std::stoi(code), value);
      return result;
    };
    const auto actualGroups = groups(written);
    const auto expectedGroups = groups(expected);
    REQUIRE(actualGroups.size() == expectedGroups.size());
    bool inTableHeader = true;
    std::set<unsigned long long> handles;
    for (std::size_t i = 0; i < actualGroups.size(); ++i) {
      const auto &[code, value] = actualGroups[i];
      const auto &[expectedCode, expectedValue] = expectedGroups[i];
      INFO("group index " << i);
      REQUIRE(code == expectedCode);
      if (code == 0 && value == "LTYPE")
        inTableHeader = false;
      if (code == 70 && inTableHeader) {
        CHECK(std::stoi(value) == 35);
        CHECK(std::stoi(expectedValue) == 4); // historical stale count
      } else if (code == 5) {
        const auto handle = std::stoull(value, nullptr, 16);
        CHECK(handle != 0);
        CHECK(handles.insert(handle).second);
      } else if (code == 40) {
        const double actual = std::stod(value);
        const double expectedTotal = std::stod(expectedValue);
        // The old snapshot sums the dashes; exact declared totals are
        // checked separately below, so allow only rounding noise here.
        CHECK(actual == Catch::Approx(expectedTotal).epsilon(1e-12).margin(0.0));
      } else if (code == 49) {
        CHECK(std::stod(value) == std::stod(expectedValue));
      } else {
        CHECK(value == expectedValue);
      }
    }
    // The old snapshot re-summed these totals; the current writer must keep
    // each built-in's declared total exactly, including its last bit.
    for (const BuiltinLTypeLength &row : kBuiltinLTypeLengths) {
      if (row.size == 0) continue;
      INFO("LTYPE " << row.name);
      const auto total = ltypeRecordGroupValues(out, row.name, "40");
      REQUIRE(total.size() == 1);
      CHECK(std::stod(total.front()) == row.length);
    }
#else
    CHECK(written == expected);
#endif
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_BUILTIN_LTYPE_DXF"))
      std::filesystem::copy_file(out, std::string(prefix) + "-" +
          std::to_string(static_cast<int>(target.format)) + ".dxf",
          std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(out);
  }
}

#ifdef DWGSUPPORT
// DWG retains the declared pattern-length double through encoder
// normalization and the subsequent import.
TEST_CASE("A new drawing's built-in linetype lengths survive a DWG round trip",
          "[dwg][roundtrip][filter][linetype][ltype][named]") {
  ensureSettings();
  RS_Graphic graphic;
  graphic.initForNewDocument();

  for (const RS2::FormatType format : {RS2::FormatDWG, RS2::FormatDWG2004}) {
    INFO("format " << static_cast<int>(format));
    const std::string dwg = tmpFile("builtin_ltype_lengths.dwg");
    std::filesystem::remove(dwg);
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileExport(graphic, QString::fromStdString(dwg), format));
    }
    RS_Graphic fromDwg;
    {
      RS_FilterDXFRW filter;
      REQUIRE(filter.fileImport(fromDwg, QString::fromStdString(dwg),
                                RS2::FormatDWG));
    }

    // The three records without dashes are libdxfrw's own; the 32 the
    // filter adds are the ones a re-summed table would change.
    const auto &meta = fromDwg.dwgAdvancedMetadata();
    for (const BuiltinLTypeLength &row : kBuiltinLTypeLengths) {
      if (row.size == 0)
        continue;
      INFO("LTYPE " << row.name);
      const DRW_LType *record = meta.findLineTypeTableEntryByName(row.name);
      REQUIRE(record != nullptr);
      CHECK(record->size == row.size);
      CHECK(record->path.size() == static_cast<std::size_t>(row.size));
      // Exact, not Approx: the same double or a changed file.
      CHECK(record->length == row.length);
    }
    if (const char *prefix = std::getenv("LIBRECAD_KEEP_BUILTIN_LTYPE_DWG"))
      std::filesystem::copy_file(dwg, std::string(prefix) + "-" +
          std::to_string(static_cast<int>(format)) + ".dwg",
          std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(dwg);
  }
}
#endif // DWGSUPPORT

namespace {

// An R12 drawing with only an LTYPE table and a padded walked name. It
// declares UTF-8, which the names below are; an R12 file that declares no
// codepage is Windows-1252.
std::string ltypeTableDrawing(const std::string &records) {
  return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n"
         "9\n$DWGCODEPAGE\n3\nUTF-8\n"
         "9\n$CELTYPE\n6\nVENDOR \n0\nENDSEC\n"
         "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLTYPE\n70\n8\n" +
         records +
         "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
}

std::string ltypeRecord(const std::string &name,
                        const std::vector<double> &dashes) {
  double length = 0.0;
  for (const double dash : dashes)
    length += std::fabs(dash);
  std::ostringstream out;
  out << "0\nLTYPE\n2\n" << name << "\n70\n0\n3\n" << name << "\n72\n65\n73\n"
      << dashes.size() << "\n40\n" << length << "\n";
  for (const double dash : dashes)
    out << "49\n" << dash << "\n";
  return out.str();
}

bool importFile(RS_Graphic &graphic, const std::string &path,
                RS2::FormatType format = RS2::FormatDXFRW) {
  RS_FilterDXFRW filter;
  return filter.fileImport(graphic, QString::fromStdString(path), format);
}

bool exportAs(RS_Graphic &graphic, const std::string &path,
              RS2::FormatType format = RS2::FormatDXFRW) {
  std::filesystem::remove(path);
  RS_FilterDXFRW filter;
  return filter.fileExport(graphic, QString::fromStdString(path), format);
}

void importRecords(RS_Graphic &graphic, const std::string &records) {
  const std::string src = tmpFile("table_records_src.dxf");
  writeText(src, ltypeTableDrawing(records));
  REQUIRE(importFile(graphic, src));
  std::filesystem::remove(src);
}

std::vector<double> dashesOf(const std::string &path,
                             const std::string &name) {
  std::vector<double> dashes;
  for (const std::string &value : ltypeRecordGroupValues(path, name, "49"))
    dashes.push_back(std::stod(value));
  return dashes;
}

// Each LTYPE record spelt `name` to the byte, blanks included, as its group
// 49 dashes. ltypeRecordGroupValues() trims the name, so it can tell neither
// a padded record from the plain one nor a dashless record from none.
using Records = std::vector<std::vector<double>>;
Records ltypeRecordsSpelt(const std::string &path, const std::string &name) {
  std::ifstream in(path);
  std::string codeLine, valueLine;
  Records records;
  bool inLtype = false;
  bool nameMatched = false;
  while (std::getline(in, codeLine) && std::getline(in, valueLine)) {
    if (!valueLine.empty() && valueLine.back() == '\r')
      valueLine.pop_back();
    const std::string groupCode = trimDxfToken(codeLine);
    if (groupCode == "0") {
      inLtype = trimDxfToken(valueLine) == "LTYPE";
      nameMatched = false;
    } else if (inLtype && groupCode == "2") {
      nameMatched = valueLine == name;
      if (nameMatched)
        records.emplace_back();
    } else if (nameMatched && groupCode == "49") {
      records.back().push_back(std::stod(valueLine));
    }
  }
  return records;
}

// The entry a record made: Imported and backed by the archive.
const LC_LineType *importedEntry(const RS_Graphic &graphic, const char *name) {
  INFO("entry " << name);
  const LC_LineType *entry = graphic.findLineType(QString::fromUtf8(name));
  if (entry != nullptr) {
    CHECK(entry->origin == LC_LineType::Origin::Imported);
    CHECK(entry->hasImportedRecord);
  }
  return entry;
}

// A QCad-1 file with an LTYPE table, which that reader does not parse.
const char *const kDxf1WithLTypeFixture =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLTYPE\n70\n1\n"
    "0\nLTYPE\n2\nVENDOR_TAB\n70\n0\n3\nVendor tabulator\n72\n65\n73\n2\n"
    "40\n40.0\n49\n20.0\n49\n-20.0\n"
    "0\nENDTAB\n"
    "0\nTABLE\n2\nLAYER\n70\n1\n"
    "0\nLAYER\n2\nL_VENDOR\n70\n0\n62\n7\n6\nVENDOR_TAB\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n8\n0\n6\nVENDOR_TAB\n10\n0\n20\n0\n11\n10\n21\n0\n"
    "0\nENDSEC\n0\nEOF\n";

} // namespace

TEST_CASE("DXF import fills the line type list from its LTYPE records only",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = writeFixture("table_fill_src.dxf", kNamedR12Fixture);
  const std::string out = tmpFile("table_fill_out.dxf");

  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));

  // Eleven records: eight custom keys (the case twins share one) and two
  // built-ins.
  CHECK(graphic.countLineTypes() == 43);
  const LC_LineType *tab = importedEntry(graphic, "VENDOR_TAB");
  REQUIRE(tab != nullptr);
  CHECK(tab->name == "VENDOR_TAB");
  CHECK(tab->description == "Vendor tabulator");
  CHECK(tab->pattern == std::vector<double>{20.0, -20.0});
  // Gaps only, a dot and a zero-length pattern are patterns too.
  struct Edge {
    const char *name;
    std::vector<double> dashes;
  };
  for (const Edge &edge : {Edge{"VENDOR_UTL", {20.0, -20.0, 2.0, -20.0}},
                           Edge{"VENDOR_NEG", {-20.0, -20.0}},
                           Edge{"VENDOR_ODD", {10.0, -5.0, 0.0}},
                           Edge{"VENDOR_ZERO", {0.0, 0.0}}}) {
    INFO("LTYPE " << edge.name);
    const LC_LineType *entry = importedEntry(graphic, edge.name);
    REQUIRE(entry != nullptr);
    CHECK(entry->pattern == edge.dashes);
  }

  // Case twins share one entry: the byte-order-first spelling, its dashes.
  const LC_LineType *mixed = importedEntry(graphic, "vendor_mixedcase");
  REQUIRE(mixed != nullptr);
  CHECK(mixed->name == "VENDOR_MIXEDCASE");
  CHECK(mixed->pattern == std::vector<double>{30.0, -30.0});
  CHECK(graphic.findLineType(QStringLiteral("Vendor_mixedCase")) == mixed);
  // A non-ASCII name, verbatim, found in either Unicode form.
  const LC_LineType *oel = importedEntry(graphic, kOelfarbe);
  REQUIRE(oel != nullptr);
  CHECK(oel->name == QString::fromUtf8(kOelfarbe));
  CHECK(oel->pattern == std::vector<double>{20.0, -20.0});
  CHECK(graphic.findLineType(QString::fromUtf8("O\xCC\x88" "lfarbe")) == oel);

  // A record redefining a built-in brings its dashes; one that only names
  // it keeps the seed's, as writeLType() writes them; a built-in no record
  // names stays unbacked.
  const LC_LineType *hidden = graphic.findLineType(QStringLiteral("HIDDEN"));
  REQUIRE(hidden != nullptr);
  CHECK(hidden == graphic.lineTypeAt(11));
  CHECK(hidden->pattern == std::vector<double>{64.0, -32.0});
  CHECK(hidden->description == "Vendor hidden");
  CHECK(hidden->origin == LC_LineType::Origin::BuiltIn);
  CHECK(hidden->hasImportedRecord);
  const LC_LineType *dashed = graphic.findLineType(QStringLiteral("DASHED"));
  REQUIRE(dashed != nullptr);
  CHECK(dashed == graphic.lineTypeAt(7));
  CHECK(dashed->pattern == std::vector<double>{12.7, -6.35});
  CHECK(dashed->origin == LC_LineType::Origin::BuiltIn);
  CHECK(dashed->hasImportedRecord);
  const LC_LineType *dashed2 = graphic.findLineType(QStringLiteral("DASHED2"));
  REQUIRE(dashed2 != nullptr);
  CHECK_FALSE(dashed2->hasImportedRecord);

  // A name only a pen, a layer, a block member or an ISO alias gives makes
  // no entry, before or after the export that writes its marker record.
  const char *const bare[] = {"VENDOR_NOREC", "VENDOR_LAYER_NOREC",
                              "VENDOR_BLK_NOREC", "ACAD_ISO02W100",
                              "ACAD_ISO09W100"};
  for (const char *name : bare) {
    INFO("name " << name);
    CHECK(graphic.findLineType(QString::fromUtf8(name)) == nullptr);
  }
  REQUIRE(exportAs(graphic, out));
  for (const char *name : bare) {
    INFO("name " << name);
    CHECK(graphic.findLineType(QString::fromUtf8(name)) == nullptr);
  }
  CHECK(graphic.countLineTypes() == 43);
  // What the file gets for these records is what the list shows.
  CHECK(dashesOf(out, "HIDDEN") == hidden->pattern);
  CHECK(dashesOf(out, "VENDOR_TAB") == tab->pattern);
  // The pens are what they were: the list is a second reader of the records.
  RS_Entity *line = entityOnLayer(graphic, QStringLiteral("L_ENT_NOREC"));
  REQUIRE(line != nullptr);
  CHECK(line->getPen(false).getLineTypeName() ==
        QStringLiteral("VENDOR_NOREC"));

  // A load ends with markSaved(): a filled list is no edit.
  graphic.markSaved(QDateTime::currentDateTime());
  CHECK_FALSE(graphic.isModified());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF LTYPE twins fill one list entry whichever comes first",
          "[system-codec][dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string out = tmpFile("table_twins_out.dxf");
  const std::string upper = ltypeRecord("VENDOR_MIXEDCASE", {30.0, -30.0});
  const std::string mixed = ltypeRecord("Vendor_mixedCase", {20.0, -20.0});
  const std::string padded = ltypeRecord(" VENDOR", {3.0, -3.0});
  const std::string plain = ltypeRecord("VENDOR", {2.0, -2.0});
  const std::string nfc = ltypeRecord(kOelfarbe, {20.0, -20.0});
  const std::string nfd = ltypeRecord("O\xCC\x88" "lfarbe", {7.0, -7.0});
  const std::string lower = ltypeRecord("dashed", {64.0, -32.0});
  const std::string paddedBuiltin = ltypeRecord(" HIDDEN", {9.0, -9.0});

  for (const bool swapped : {false, true}) {
    INFO("swapped " << swapped);
    RS_Graphic graphic;
    importRecords(graphic, swapped ? mixed + upper + plain + padded + nfd +
                                         nfc + paddedBuiltin + lower
                                   : upper + mixed + padded + plain + nfc +
                                         nfd + lower + paddedBuiltin);
    CHECK(graphic.countLineTypes() == 38);
    // The archive keeps every spelling; the list keeps one per key.
    CHECK(graphic.dwgAdvancedMetadata().lineTypeTableEntries().size() == 8);

    // Case twins: the byte-order-first spelling, the one record (c') writes.
    const LC_LineType *twin =
        graphic.findLineType(QStringLiteral("vendor_mixedcase"));
    REQUIRE(twin != nullptr);
    CHECK(twin->name == "VENDOR_MIXEDCASE");
    CHECK(twin->pattern == std::vector<double>{30.0, -30.0});
    // A padded record loses to an unpadded one though its bytes sort first;
    // the padded $CELTYPE is no record at all.
    const LC_LineType *vendor = graphic.findLineType(QStringLiteral("VENDOR"));
    REQUIRE(vendor != nullptr);
    CHECK(vendor->name == "VENDOR");
    CHECK(vendor->pattern == std::vector<double>{2.0, -2.0});
    // NFC and NFD are one key: the entry shows the byte-order-first (NFD)
    // record.
    const LC_LineType *oel = graphic.findLineType(QString::fromUtf8(kOelfarbe));
    REQUIRE(oel != nullptr);
    CHECK(oel->name == QString::fromUtf8("O\xCC\x88" "lfarbe"));
    CHECK(oel->pattern == std::vector<double>{7.0, -7.0});
    // A built-in spelt in lower case takes that spelling and its dashes, as
    // writeLType() substitutes the record; a padded spelling is the writer's
    // own record and leaves the seed unbacked.
    const LC_LineType *dashed = graphic.findLineType(QStringLiteral("DASHED"));
    REQUIRE(dashed != nullptr);
    CHECK(dashed->name == "dashed");
    CHECK(dashed->pattern == std::vector<double>{64.0, -32.0});
    CHECK(dashed->origin == LC_LineType::Origin::BuiltIn);
    CHECK(dashed->hasImportedRecord);
    const LC_LineType *hidden = graphic.findLineType(QStringLiteral(" HIDDEN"));
    REQUIRE(hidden != nullptr);
    CHECK(hidden->name == "HIDDEN");
    CHECK(hidden->pattern == LC_LineTypeNames::builtinMetrics()[11].pattern);
    CHECK_FALSE(hidden->hasImportedRecord);

    // 2007 on purpose: the spelling checks below are pinned to this writer.
    REQUIRE(exportAs(graphic, out, RS2::FormatDXFRW));
    CHECK(dashesOf(out, "VENDOR_MIXEDCASE") == twin->pattern);
    CHECK(ltypeRecordsSpelt(out, "Vendor_mixedCase").empty());
    CHECK(dashesOf(out, "dashed") == dashed->pattern);
    CHECK(ltypeRecordsSpelt(out, "DASHED").empty());
    // The other twins keep both records: the writer walks the archive, which
    // holds every spelling, where the list holds one entry per key.
    CHECK(dashesOf(out, kOelfarbe) == std::vector<double>{20.0, -20.0});
    CHECK(dashesOf(out, "O\xCC\x88" "lfarbe") ==
          std::vector<double>{7.0, -7.0});
    CHECK(ltypeRecordsSpelt(out, " VENDOR") == Records{{3.0, -3.0}});
    CHECK(ltypeRecordsSpelt(out, " HIDDEN") == Records{{9.0, -9.0}});
  }

  // The same spelling twice: the later record, as the archive keeps it.
  RS_Graphic graphic;
  importRecords(graphic, plain + ltypeRecord("VENDOR", {5.0, -5.0}));
  CHECK(graphic.countLineTypes() == 36);
  const LC_LineType *vendor = graphic.findLineType(QStringLiteral("VENDOR"));
  REQUIRE(vendor != nullptr);
  CHECK(vendor->pattern == std::vector<double>{5.0, -5.0});
  REQUIRE(exportAs(graphic, out, RS2::FormatDXFRW));
  CHECK(dashesOf(out, "VENDOR") == vendor->pattern);

  std::filesystem::remove(out);
}

TEST_CASE("DXF import keeps a complex record's shape data in the archive",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src =
      writeFixture("table_r2000_src.dxf", kNamedR2000Fixture);
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  CHECK(graphic.countLineTypes() == 37);
  const LC_LineType *tab = importedEntry(graphic, "VENDOR_TAB");
  REQUIRE(tab != nullptr);
  CHECK(tab->pattern == std::vector<double>{20.0, -20.0});

  // The entry holds the dash lengths; the text segment, the style handle
  // and the application group stay with the archived record.
  const LC_LineType *cplx = importedEntry(graphic, "VENDOR_CPLX");
  REQUIRE(cplx != nullptr);
  CHECK(cplx->description == "Vendor complex");
  CHECK(cplx->pattern == std::vector<double>{20.0, -10.0});
  const DRW_LType *record =
      graphic.dwgAdvancedMetadata().findLineTypeTableEntryByName("VENDOR_CPLX");
  REQUIRE(record != nullptr);
  CHECK(record->handle == 0x41u);
  CHECK(record->path == cplx->pattern);

  // Names a DIMSTYLE handle or an MLINESTYLE element carry make no entry.
  CHECK(graphic.findLineType(QStringLiteral("VENDOR_MLS")) == nullptr);
  CHECK(graphic.findLineType(QStringLiteral("VENDOR_DIM")) == nullptr);
  std::filesystem::remove(src);
}

TEST_CASE("The QCad-1 reader fills no line type entry",
          "[dxf][roundtrip][filter][linetype][named][table][dxf1]") {
  ensureSettings();
  const std::string src =
      writeFixture("table_dxf1_src.dxf", kDxf1WithLTypeFixture);

  // The file does carry the record: the DXF reader makes an entry of it.
  {
    RS_Graphic viaDxfrw;
    REQUIRE(importFile(viaDxfrw, src));
    const LC_LineType *tab = importedEntry(viaDxfrw, "VENDOR_TAB");
    REQUIRE(tab != nullptr);
    CHECK(tab->pattern == std::vector<double>{20.0, -20.0});
  }

  // The QCad-1 reader parses layers and entities only: the pens get the
  // name, the list stays the seed.
  RS_Graphic graphic;
  {
    RS_FilterDXF1 filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src),
                              RS2::FormatDXF1));
  }
  REQUIRE(graphic.firstEntity() != nullptr);
  CHECK(graphic.firstEntity()->getPen(false).getLineTypeName() ==
        QStringLiteral("VENDOR_TAB"));
  CHECK(graphic.countLineTypes() == 35);
  CHECK(graphic.findLineType(QStringLiteral("VENDOR_TAB")) == nullptr);
  for (unsigned i = 0; i < graphic.countLineTypes(); ++i) {
    INFO("entry " << i << " " << graphic.lineTypeAt(i)->name.toStdString());
    CHECK_FALSE(graphic.lineTypeAt(i)->hasImportedRecord);
  }
  std::filesystem::remove(src);
}

#ifdef DWGSUPPORT
TEST_CASE("DWG import fills the line type list from its records",
          "[dwg][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = writeFixture("table_dwg_src.dxf", kNamedR12Fixture);
  const std::string dwg = tmpFile("table_dwg_out.dwg");
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  REQUIRE(exportAs(graphic, dwg, RS2::FormatDWG2004));
  RS_Graphic fromDwg;
  REQUIRE(importFile(fromDwg, dwg, RS2::FormatDWG));

  // The records arrive from a DWG in no set order; the list is the same:
  // the seeds, the eight vendor keys and the five marker records the
  // writer gave the names no record had.
  CHECK(fromDwg.countLineTypes() == 48);
  const LC_LineType *tab = importedEntry(fromDwg, "VENDOR_TAB");
  REQUIRE(tab != nullptr);
  CHECK(tab->pattern == std::vector<double>{20.0, -20.0});
  const LC_LineType *utl = importedEntry(fromDwg, "VENDOR_UTL");
  REQUIRE(utl != nullptr);
  CHECK(utl->pattern == std::vector<double>{20.0, -20.0, 2.0, -20.0});
  const LC_LineType *hidden = fromDwg.findLineType(QStringLiteral("HIDDEN"));
  REQUIRE(hidden != nullptr);
  CHECK(hidden->pattern == std::vector<double>{64.0, -32.0});
  CHECK(hidden->origin == LC_LineType::Origin::BuiltIn);
  CHECK(hidden->hasImportedRecord);
  // The DWG writer gave VENDOR_NOREC a record of its own: read back, an entry
  // without dashes or description.
  const LC_LineType *bare = importedEntry(fromDwg, "VENDOR_NOREC");
  REQUIRE(bare != nullptr);
  CHECK(bare->pattern.empty());
  CHECK(bare->description.isEmpty());

  std::filesystem::remove(src);
  std::filesystem::remove(dwg);
}
#endif // DWGSUPPORT

namespace {

// kOelfarbe with U+0308 COMBINING DIAERESIS after the O: the same name in
// NFD, another byte spelling of one key.
const char *const kOelfarbeNfd = "O\xCC\x88" "lfarbe";

// An R12 drawing whose references are spelt unlike their records: NFD for
// an NFC record, no blanks for a padded record, and an ISO alias for a
// padded record that only names it. Each LINE sits on a layer of its own.
// It declares UTF-8, which the two spellings are.
std::string foldedReferenceDrawing() {
  return "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n"
         "9\n$DWGCODEPAGE\n3\nUTF-8\n0\nENDSEC\n"
         "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLTYPE\n70\n3\n" +
         ltypeRecord(kOelfarbe, {20.0, -20.0}) +
         ltypeRecord(" VENDOR", {3.0, -3.0}) +
         "0\nLTYPE\n2\n ACAD_ISO02W100\n70\n0\n"
         "0\nENDTAB\n0\nTABLE\n2\nLAYER\n70\n4\n"
         "0\nLAYER\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
         "0\nLAYER\n2\nL_NFD\n70\n0\n62\n7\n6\nCONTINUOUS\n"
         "0\nLAYER\n2\nL_PLAIN\n70\n0\n62\n7\n6\nCONTINUOUS\n"
         "0\nLAYER\n2\nL_ISO\n70\n0\n62\n7\n6\nCONTINUOUS\n"
         "0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
         "0\nLINE\n8\nL_NFD\n6\n" +
         std::string(kOelfarbeNfd) +
         "\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
         "0\nLINE\n8\nL_PLAIN\n6\nVENDOR\n"
         "10\n0.0\n20\n10.0\n11\n10.0\n21\n10.0\n"
         "0\nLINE\n8\nL_ISO\n6\nACAD_ISO02W100\n"
         "10\n0.0\n20\n20.0\n11\n10.0\n21\n20.0\n"
         "0\nENDSEC\n0\nEOF\n";
}

} // namespace

// A reference the writer's byte key matches to no record gets a record of
// its own under its spelling; it carries the dashes of the list entry the
// name folds to, where it carried none. An entry without dashes does not
// hide the family's.
TEST_CASE("DXF a reference spelt unlike its record takes the entry's dashes",
          "[system-codec][dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = tmpFile("table_folded_ref_src.dxf");
  const std::string out = tmpFile("table_folded_ref_out.dxf");
  writeText(src, foldedReferenceDrawing());
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  CHECK(graphic.countLineTypes() == 38);

  // 2007 on purpose: it writes the names byte for byte.
  REQUIRE(exportAs(graphic, out, RS2::FormatDXFRW));
  // The records keep their own spelling and dashes...
  CHECK(ltypeRecordsSpelt(out, kOelfarbe) == Records{{20.0, -20.0}});
  CHECK(ltypeRecordsSpelt(out, " VENDOR") == Records{{3.0, -3.0}});
  CHECK(ltypeRecordsSpelt(out, " ACAD_ISO02W100") ==
        Records{std::vector<double>{}});
  // ...and each reference its record, spelt as the entity spells it.
  CHECK(ltypeRecordsSpelt(out, kOelfarbeNfd) == Records{{20.0, -20.0}});
  CHECK(ltypeRecordsSpelt(out, "VENDOR") == Records{{3.0, -3.0}});
  CHECK(ltypeRecordsSpelt(out, "ACAD_ISO02W100") == Records{{12.7, -6.35}});
  CHECK(recordGroupValues(out, "LINE", "6") ==
        std::vector<std::string>{kOelfarbeNfd, "VENDOR", "ACAD_ISO02W100"});
  // Nothing is added and nothing moves: the built-ins as the golden has
  // them, the records in byte order, the markers in the order they were met.
  std::vector<std::string> names = recordGroupValues(
      std::string(LIBRECAD_TEST_DIR) + "/dxf/ltype_table_ac1015_ac1021.dxf",
      "LTYPE", "2");
  REQUIRE(names.size() == 35);
  for (const char *name : {"ACAD_ISO02W100", "VENDOR", kOelfarbe,
                           kOelfarbeNfd, "VENDOR", "ACAD_ISO02W100"})
    names.push_back(name);
  CHECK(recordGroupValues(out, "LTYPE", "2") == names);
  CHECK(graphic.countLineTypes() == 38);

  // Read back, each fold's one entry holds the dashes whichever spelling
  // wins it.
  RS_Graphic again;
  REQUIRE(importFile(again, out));
  CHECK(again.countLineTypes() == 38);
  const LC_LineType *oel = again.findLineType(QString::fromUtf8(kOelfarbe));
  REQUIRE(oel != nullptr);
  CHECK(oel->name == QString::fromUtf8(kOelfarbeNfd));
  CHECK(oel->pattern == std::vector<double>{20.0, -20.0});
  const LC_LineType *vendor = again.findLineType(QStringLiteral("VENDOR"));
  REQUIRE(vendor != nullptr);
  CHECK(vendor->name == "VENDOR");
  CHECK(vendor->pattern == std::vector<double>{3.0, -3.0});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

#ifdef DWGSUPPORT
// The same record reaches a DWG, which keeps group 40 as the double it is
// given: the entry's dashes and their sum.
TEST_CASE("DWG a reference spelt unlike its record takes the entry's dashes",
          "[dwg][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = tmpFile("table_folded_ref_dwg_src.dxf");
  const std::string dwg = tmpFile("table_folded_ref_out.dwg");
  writeText(src, foldedReferenceDrawing());
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  REQUIRE(exportAs(graphic, dwg, RS2::FormatDWG2004));
  RS_Graphic fromDwg;
  REQUIRE(importFile(fromDwg, dwg, RS2::FormatDWG));

  struct Reference {
    const char *layer;
    const char *name;
    std::vector<double> dashes;
    double length;
  };
  const auto &meta = fromDwg.dwgAdvancedMetadata();
  for (const Reference &ref :
       {Reference{"L_NFD", kOelfarbeNfd, {20.0, -20.0}, 40.0},
        Reference{"L_PLAIN", "VENDOR", {3.0, -3.0}, 6.0}}) {
    INFO("reference " << ref.name);
    RS_Entity *line = entityOnLayer(fromDwg, QLatin1String(ref.layer));
    REQUIRE(line != nullptr);
    CHECK(line->getPen(false).getLineTypeName() == QString::fromUtf8(ref.name));
    const DRW_LType *record = meta.findLineTypeTableEntryByName(ref.name);
    REQUIRE(record != nullptr);
    CHECK(record->path == ref.dashes);
    CHECK(record->length == ref.length);
  }
  const DRW_LType *nfc = meta.findLineTypeTableEntryByName(kOelfarbe);
  REQUIRE(nfc != nullptr);
  CHECK(nfc->path == std::vector<double>{20.0, -20.0});
  // Read back, each fold's one entry holds those dashes.
  CHECK(fromDwg.countLineTypes() == 38);
  const LC_LineType *oel = fromDwg.findLineType(QString::fromUtf8(kOelfarbe));
  REQUIRE(oel != nullptr);
  CHECK(oel->pattern == std::vector<double>{20.0, -20.0});
  const LC_LineType *vendor = fromDwg.findLineType(QStringLiteral("VENDOR"));
  REQUIRE(vendor != nullptr);
  CHECK(vendor->pattern == std::vector<double>{3.0, -3.0});

  std::filesystem::remove(src);
  std::filesystem::remove(dwg);
}
#endif // DWGSUPPORT

namespace {

// A record that only names a line type: no 72, 73, 40 or 49.
std::string ltypeNameOnly(const std::string &name) {
  return "0\nLTYPE\n2\n" + name + "\n70\n0\n";
}

} // namespace

TEST_CASE("DXF does not synthesize LTYPE records for unwritable names",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string out = tmpFile("unwritable_ltype_out.dxf");
  RS_Graphic graphic;
  REQUIRE(exportAs(graphic, out));
  const int originalRecords = countRecords(out, "LTYPE");

  RS_Graphic source;
  auto *lineType = new LC_LineType(QStringLiteral("VENDOR\nTYPE"));
  lineType->pattern = {2.0, -2.0};
  REQUIRE(source.addLineType(lineType) == lineType);
  graphic.mergeLineTypes(source);

  REQUIRE(exportAs(graphic, out));
  CHECK(countRecords(out, "LTYPE") == originalRecords);
  std::filesystem::remove(out);
}

// A list entry no record backs, as a library insert leaves it, gets a record
// of its own: spelt and described as the entry is, after the drawing's own
// records and before those written for names that have none.
TEST_CASE("DXF a line type no record backs is written from the list",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = tmpFile("table_only_src.dxf");
  const std::string out = tmpFile("table_only_out.dxf");
  writeText(src, kNamedR12Fixture);
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  REQUIRE(graphic.countLineTypes() == 43);
  REQUIRE(exportAs(graphic, out));
  const std::vector<std::string> before = recordGroupValues(out, "LTYPE", "2");

  auto *lib = new LC_LineType(QStringLiteral("Vendor_Lib"));
  lib->description = QStringLiteral("Vendor library");
  lib->pattern = {2.0, -2.0};
  REQUIRE(graphic.addLineType(lib) == lib);
  auto *bare = new LC_LineType(QStringLiteral("VENDOR_LIB_BARE"));
  REQUIRE(graphic.addLineType(bare) == bare);
  REQUIRE_FALSE(lib->hasImportedRecord);

  REQUIRE(exportAs(graphic, out));
  CHECK(ltypeRecordsSpelt(out, "Vendor_Lib") == Records{{2.0, -2.0}});
  CHECK(ltypeRecordGroupValues(out, "Vendor_Lib", "3") ==
        std::vector<std::string>{"Vendor library"});
  CHECK(ltypeRecordGroupValues(out, "Vendor_Lib", "73") ==
        std::vector<std::string>{"2"});
  // An entry without dashes is still a name of the drawing's table.
  CHECK(ltypeRecordsSpelt(out, "VENDOR_LIB_BARE") == Records{{}});
  // The two records, in list order, between the archive's and the markers;
  // every other record where it was.
  const std::vector<std::string> after = recordGroupValues(out, "LTYPE", "2");
  REQUIRE(after.size() == before.size() + 2);
  const auto at = std::find(after.cbegin(), after.cend(), "Vendor_Lib");
  REQUIRE(at != after.cend());
  REQUIRE(at + 1 != after.cend());
  CHECK(*(at + 1) == "VENDOR_LIB_BARE");
  std::vector<std::string> others(after.cbegin(), at);
  others.insert(others.end(), at + 2, after.cend());
  CHECK(others == before);
  const auto &archive = graphic.dwgAdvancedMetadata();
  for (auto name = after.cbegin() + 35; name != at; ++name) {
    INFO("record " << *name);
    CHECK(archive.findLineTypeTableEntryByName(*name) != nullptr);
  }
  CHECK(at + 2 != after.cend());
  for (auto name = at + 2; name != after.cend(); ++name) {
    INFO("marker " << *name);
    CHECK(archive.findLineTypeTableEntryByName(*name) == nullptr);
  }
  // A built-in no record backs is the head's, once.
  CHECK(ltypeRecordsSpelt(out, "DASHED2").size() == 1);
  CHECK(graphic.countLineTypes() == 45);
  CHECK(archive.findLineTypeTableEntryByName("Vendor_Lib") == nullptr);

  // Read back, each is a record's entry.
  RS_Graphic back;
  REQUIRE(importFile(back, out));
  const LC_LineType *entry = importedEntry(back, "Vendor_Lib");
  REQUIRE(entry != nullptr);
  CHECK(entry->name == "Vendor_Lib");
  CHECK(entry->pattern == std::vector<double>{2.0, -2.0});
  CHECK(entry->description == "Vendor library");
  const LC_LineType *bareBack = importedEntry(back, "VENDOR_LIB_BARE");
  REQUIRE(bareBack != nullptr);
  CHECK(bareBack->pattern.empty());
  CHECK(back.countLineTypes() == after.size()); // the markers are records now

  // R12 writes every record's name in upper case, these too.
  REQUIRE(exportAs(graphic, out, RS2::FormatDXFRW12));
  CHECK(ltypeRecordsSpelt(out, "VENDOR_LIB") == Records{{2.0, -2.0}});
  CHECK(ltypeRecordsSpelt(out, "Vendor_Lib").empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

namespace {
// One R2007 LTYPE record with an XDATA application, so the table-entry part
// the writer carries over is visible.
const char *const kChangedDefinitionFixture =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n"
    "0\nTABLE\n2\nLTYPE\n70\n1\n"
    "0\nLTYPE\n5\n40\n330\n5\n"
    "100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
    "2\nVENDOR_TAB\n70\n0\n3\nTab marks\n72\n65\n73\n2\n40\n15.0\n"
    "49\n10.0\n49\n-5.0\n"
    "1001\nVENDOR_APP\n1000\ntab-data\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\n100\n330\n1F\n100\nAcDbEntity\n8\n0\n6\nVENDOR_TAB\n"
    "100\nAcDbLine\n10\n0.0\n20\n0.0\n11\n10.0\n21\n0.0\n"
    "0\nENDSEC\n0\nEOF\n";
} // namespace

// The table's definition replaced the record's; the record's identity and
// its table-entry part stay.
TEST_CASE("DXF writes a changed line type definition over its record",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = tmpFile("changed_definition_src.dxf");
  const std::string out = tmpFile("changed_definition_out.dxf");
  writeText(src, kChangedDefinitionFixture);
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  LC_LineType *entry = graphic.findLineType(QStringLiteral("VENDOR_TAB"));
  REQUIRE(entry != nullptr);
  REQUIRE(entry->pattern == std::vector<double>{10.0, -5.0});
  REQUIRE(entry->description == QStringLiteral("Tab marks"));
  entry->pattern = {20.0, -10.0};
  entry->description = QStringLiteral("Wider tab marks");

  REQUIRE(exportAs(graphic, out));
  CHECK(ltypeRecordsSpelt(out, "VENDOR_TAB") == Records{{20.0, -10.0}});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_TAB", "3") ==
        std::vector<std::string>{"Wider tab marks"});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_TAB", "1001") ==
        std::vector<std::string>{"VENDOR_APP"});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_TAB", "1000") ==
        std::vector<std::string>{"tab-data"});

  RS_Graphic back;
  REQUIRE(importFile(back, out));
  const LC_LineType *backEntry = importedEntry(back, "VENDOR_TAB");
  REQUIRE(backEntry != nullptr);
  CHECK(backEntry->pattern == std::vector<double>{20.0, -10.0});
  CHECK(backEntry->description == QStringLiteral("Wider tab marks"));

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

#ifdef DWGSUPPORT
TEST_CASE("DWG writes a changed line type definition over its record",
          "[dwg][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = tmpFile("changed_definition_dwg_src.dxf");
  const std::string dwg = tmpFile("changed_definition_out.dwg");
  writeText(src, kChangedDefinitionFixture);
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  LC_LineType *entry = graphic.findLineType(QStringLiteral("VENDOR_TAB"));
  REQUIRE(entry != nullptr);
  entry->pattern = {20.0, -10.0};
  entry->description = QStringLiteral("Wider tab marks");

  REQUIRE(exportAs(graphic, dwg, RS2::FormatDWG2004));
  RS_Graphic fromDwg;
  REQUIRE(importFile(fromDwg, dwg, RS2::FormatDWG));
  const LC_LineType *backEntry = importedEntry(fromDwg, "VENDOR_TAB");
  REQUIRE(backEntry != nullptr);
  CHECK(backEntry->pattern == std::vector<double>{20.0, -10.0});
  CHECK(backEntry->description == QStringLiteral("Wider tab marks"));

  std::filesystem::remove(src);
  std::filesystem::remove(dwg);
}
#endif

TEST_CASE("DXF writes a redefined built-in from the table",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string out = tmpFile("changed_builtin_out.dxf");
  RS_Graphic graphic;
  importBuiltinLTypeFixture(graphic, "changed_builtin_src.dxf", "70\n0\n");
  LC_LineType *entry = graphic.findLineType(QStringLiteral("HIDDEN"));
  REQUIRE(entry != nullptr);
  REQUIRE(entry->pattern == std::vector<double>{64.0, -32.0});
  entry->pattern = {30.0, -10.0};

  REQUIRE(exportAs(graphic, out));
  CHECK(ltypeRecordsSpelt(out, "HIDDEN") == Records{{30.0, -10.0}});

  std::filesystem::remove(out);
}

TEST_CASE("DXF writes a changed complex line type without its segments",
          "[system-codec][dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = writeFixture("named_t8_src.dxf", kNamedR2000Fixture);
  const std::string out = tmpFile("changed_complex_out.dxf");
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  LC_LineType *entry = graphic.findLineType(QStringLiteral("VENDOR_CPLX"));
  REQUIRE(entry != nullptr);
  REQUIRE(entry->pattern == std::vector<double>{20.0, -10.0});
  entry->pattern = {12.0, -6.0};

  REQUIRE(exportAs(graphic, out));
  CHECK(ltypeRecordsSpelt(out, "VENDOR_CPLX") == Records{{12.0, -6.0}});
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "73") ==
        std::vector<std::string>{"2"});
  // The table has no complex elements; the archived application group stays.
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "9").empty());
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "75").empty());
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "340").empty());
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "46").empty());
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "50").empty());
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "44").empty());
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "45").empty());
  CHECK(ltypeRecordGroupValues(out, "VENDOR_CPLX", "310") ==
        std::vector<std::string>{"CAFE"});

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// A twin is the record of another spelling, not the one the table kept.
TEST_CASE("DXF writes a twin archived spelling as it came beside a changed record",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string out = tmpFile("changed_twin_out.dxf");
  RS_Graphic graphic;
  importRecords(graphic, ltypeRecord("VENDOR_TAB", {10.0, -5.0}) +
                            ltypeRecord(" VENDOR_TAB", {7.0, -7.0}));
  LC_LineType *entry = graphic.findLineType(QStringLiteral("VENDOR_TAB"));
  REQUIRE(entry != nullptr);
  REQUIRE(entry->name == QStringLiteral("VENDOR_TAB"));
  entry->pattern = {20.0, -10.0};

  REQUIRE(exportAs(graphic, out));
  CHECK(ltypeRecordsSpelt(out, "VENDOR_TAB") == Records{{20.0, -10.0}});
  CHECK(ltypeRecordsSpelt(out, " VENDOR_TAB") == Records{{7.0, -7.0}});

  std::filesystem::remove(out);
}

// A record that only names a line type is written with the dashes the list
// has for it, which a library insert may have brought. The record keeps
// everything else it came with.
TEST_CASE("DXF a record that only names a line type takes the list's dashes",
          "[dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string src = tmpFile("name_only_takes_src.dxf");
  const std::string out = tmpFile("name_only_takes_out.dxf");
  writeText(src,
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
            "0\nSECTION\n2\nTABLES\n"
            "0\nTABLE\n2\nLTYPE\n70\n1\n"
            "0\nLTYPE\n5\n40\n330\n5\n"
            "100\nAcDbSymbolTableRecord\n100\nAcDbLinetypeTableRecord\n"
            "2\nSAVED_LTYPE\n70\n0\n3\nSaved linetype\n72\n66\n73\n0\n40\n0\n"
            "102\n{LTYPE_APP\n310\nCAFE\n102\n}\n"
            "102\n{ACAD_REACTORS\n330\nA1\n102\n}\n"
            "102\n{ACAD_XDICTIONARY\n360\nA2\n102\n}\n"
            "0\nENDTAB\n0\nENDSEC\n"
            "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  // DXF LTYPE alignment is 65. Keep the invalid-value refusal covered, then
  // exercise pattern reconstruction with a valid alignment value.
  {
    RS_Graphic invalidGraphic;
    RS_FilterDXFRW invalidFilter;
    CHECK_FALSE(invalidFilter.fileImport(invalidGraphic, QString::fromStdString(src),
                                         RS2::FormatDXFRW));
  }
  {
    std::ifstream input(src, std::ios::binary);
    std::string content(std::istreambuf_iterator<char>(input), {});
    const auto alignment = content.find("\n72\n66\n");
    REQUIRE(alignment != std::string::npos);
    content.replace(alignment, std::string("\n72\n66\n").size(), "\n72\n65\n");
    input.close();
    writeText(src, content);
  }
#endif
  RS_Graphic graphic;
  REQUIRE(importFile(graphic, src));
  LC_LineType *entry = graphic.findLineType(QStringLiteral("SAVED_LTYPE"));
  REQUIRE(entry != nullptr);
  REQUIRE(entry->pattern.empty());
  REQUIRE(entry->hasImportedRecord);
  // Without dashes in the list the record is written as it came.
  REQUIRE(exportAs(graphic, out));
  CHECK(ltypeRecordsSpelt(out, "SAVED_LTYPE") == Records{{}});

  entry->pattern = {2.0, -2.0}; // what a library insert leaves
  // The record came with 73 0: the writer refuses it with two dashes put in,
  // so it is written rebuilt.
  REQUIRE(exportAs(graphic, out));
  CHECK(ltypeRecordsSpelt(out, "SAVED_LTYPE") == Records{{2.0, -2.0}});
  CHECK(ltypeRecordGroupValues(out, "SAVED_LTYPE", "73") ==
        std::vector<std::string>{"2"});
  const auto length = ltypeRecordGroupValues(out, "SAVED_LTYPE", "40");
  REQUIRE(length.size() == 1);
  CHECK(std::stod(length.front()) == 4.0);
  CHECK(ltypeRecordGroupValues(out, "SAVED_LTYPE", "3") ==
        std::vector<std::string>{"Saved linetype"});
#ifdef LIBRECAD_SYSTEM_CODEC_TESTS
  CHECK(ltypeRecordGroupValues(out, "SAVED_LTYPE", "72") ==
        std::vector<std::string>{"65"});
#else
  CHECK(ltypeRecordGroupValues(out, "SAVED_LTYPE", "72") ==
        std::vector<std::string>{"66"});
#endif
  CHECK(recordGroupValues(out, "LTYPE", "310") ==
        std::vector<std::string>{"CAFE"});
  const auto reactors = recordGroupValues(out, "LTYPE", "330");
  CHECK(std::count(reactors.cbegin(), reactors.cend(), "A1") == 1);
  CHECK(recordGroupValues(out, "LTYPE", "360") ==
        std::vector<std::string>{"A2"});
  CHECK(countRecords(out, "LTYPE") == 36);
  // The archive is as it was: the dashes are the list's.
  const DRW_LType *record =
      graphic.dwgAdvancedMetadata().findLineTypeTableEntryByName("SAVED_LTYPE");
  REQUIRE(record != nullptr);
  CHECK(record->path.empty());

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// Only an insert or a paste gives such a record dashes. One beside a twin
// that has them, or spelt as a built-in with blanks, is written as it came.
TEST_CASE("DXF a name-only LTYPE record beside a twin with dashes is written as it came",
          "[system-codec][dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string out = tmpFile("name_only_twins_out.dxf");
  const std::string nfdName = "O\xCC\x88" "lfarbe";
  const std::string plain = ltypeRecord("VENDOR", {2.0, -2.0});
  const std::string nfd = ltypeRecord(nfdName, {7.0, -7.0});
  for (const bool swapped : {false, true}) {
    INFO("swapped " << swapped);
    RS_Graphic graphic;
    importRecords(graphic,
                  swapped ? nfd + plain + ltypeNameOnly(kOelfarbe) +
                                ltypeNameOnly(" VENDOR") +
                                ltypeNameOnly(" HIDDEN") +
                                ltypeNameOnly("VENDOR_ALONE")
                          : ltypeNameOnly("VENDOR_ALONE") +
                                ltypeNameOnly(" HIDDEN") +
                                ltypeNameOnly(" VENDOR") +
                                ltypeNameOnly(kOelfarbe) + plain + nfd);
    REQUIRE(graphic.countLineTypes() == 38);
    // The entries the twins fold to have the dashes.
    for (const char *name : {" VENDOR", " HIDDEN", kOelfarbe}) {
      INFO("entry " << name);
      const LC_LineType *entry = graphic.findLineType(QString::fromUtf8(name));
      REQUIRE(entry != nullptr);
      REQUIRE(entry->pattern.size() == 2);
    }
    REQUIRE(exportAs(graphic, out));
    CHECK(ltypeRecordsSpelt(out, "VENDOR") == Records{{2.0, -2.0}});
    CHECK(ltypeRecordsSpelt(out, " VENDOR") == Records{{}});
    CHECK(ltypeRecordsSpelt(out, nfdName) == Records{{7.0, -7.0}});
    CHECK(ltypeRecordsSpelt(out, kOelfarbe) == Records{{}});
    const Records hidden = ltypeRecordsSpelt(out, "HIDDEN");
    REQUIRE(hidden.size() == 1);
    CHECK(hidden.front().size() == 2);
    CHECK(ltypeRecordsSpelt(out, " HIDDEN") == Records{{}});
    CHECK(ltypeRecordsSpelt(out, "VENDOR_ALONE") == Records{{}});
    CHECK(countRecords(out, "LTYPE") == 41);
  }
  std::filesystem::remove(out);
}

// The list reads a name as UTF-8 and the archive keeps its bytes. A record
// whose name is not UTF-8 is still its entry's record: written once, beside
// a record spelt as the list reads it, which stays as it came.
TEST_CASE("DXF an LTYPE record whose name is not UTF-8 is written once",
          "[system-codec][dxf][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string out = tmpFile("not_utf8_out.dxf");
  const std::string asRead = "VENDOR_\xEF\xBF\xBD";
  // One sorts before the spelling it is read as, one after.
  for (const std::string bad : {"VENDOR_\xE9", "VENDOR_\xFF"}) {
    INFO("byte " << static_cast<int>(static_cast<unsigned char>(bad.back())));
    for (const bool twin : {false, true}) {
      INFO("twin " << twin);
      RS_Graphic graphic;
      importRecords(graphic, (twin ? ltypeNameOnly(asRead) : std::string()) +
                                 ltypeRecord(bad, {2.0, -2.0}));
      const auto &archive =
          graphic.dwgAdvancedMetadata().lineTypeTableEntries();
      REQUIRE(archive.count(bad) == 1);
      REQUIRE(graphic.countLineTypes() == 36);
      const LC_LineType *entry = graphic.lineTypeAt(35);
      REQUIRE(entry->name.toStdString() == asRead);
      REQUIRE(entry->pattern == std::vector<double>{2.0, -2.0});
      REQUIRE(exportAs(graphic, out));
      CHECK(ltypeRecordsSpelt(out, bad) == Records{{2.0, -2.0}});
      CHECK(ltypeRecordsSpelt(out, asRead) == (twin ? Records{{}} : Records{}));
      // The built-ins, the archive's and the marker of $CELTYPE.
      CHECK(countRecords(out, "LTYPE") == (twin ? 38 : 37));
    }
  }
  std::filesystem::remove(out);
}

#ifdef DWGSUPPORT
// DWG keeps group 40 as the double it is given: both kinds of record carry
// the dashes and their sum.
TEST_CASE("DWG line types of the list reach the file with their dashes",
          "[dwg][roundtrip][filter][linetype][named][table]") {
  ensureSettings();
  const std::string dwg = tmpFile("table_only_out.dwg");
  RS_Graphic graphic;
  importRecords(graphic, ltypeNameOnly("VENDOR_X"));
  LC_LineType *named = graphic.findLineType(QStringLiteral("VENDOR_X"));
  REQUIRE(named != nullptr);
  REQUIRE(named->pattern.empty());
  named->pattern = {2.0, -2.0};
  // DWG alone has it: the block of the external reference a record depends on.
  DRW_LType dependent =
      *graphic.dwgAdvancedMetadata().findLineTypeTableEntryByName("VENDOR_X");
  dependent.xrefBlockHandle.ref = 0x1F;
  graphic.dwgAdvancedMetadata().addLineTypeName(dependent);
  auto *lib = new LC_LineType(QStringLiteral("VENDOR_LIB"));
  lib->description = QStringLiteral("Vendor library");
  lib->pattern = {4.0, -1.0};
  REQUIRE(graphic.addLineType(lib) == lib);

  REQUIRE(exportAs(graphic, dwg, RS2::FormatDWG2004));
  RS_Graphic fromDwg;
  REQUIRE(importFile(fromDwg, dwg, RS2::FormatDWG));
  const auto &archive = fromDwg.dwgAdvancedMetadata();
  const DRW_LType *record = archive.findLineTypeTableEntryByName("VENDOR_LIB");
  REQUIRE(record != nullptr);
  CHECK(record->path == std::vector<double>{4.0, -1.0});
  CHECK(record->length == 5.0);
  CHECK(record->desc == "Vendor library");
  record = archive.findLineTypeTableEntryByName("VENDOR_X");
  REQUIRE(record != nullptr);
  CHECK(record->path == std::vector<double>{2.0, -2.0});
  CHECK(record->length == 4.0);
  CHECK(record->xrefBlockHandle.ref == 0x1F);
  const LC_LineType *entry = importedEntry(fromDwg, "VENDOR_LIB");
  REQUIRE(entry != nullptr);
  CHECK(entry->pattern == std::vector<double>{4.0, -1.0});
  std::filesystem::remove(dwg);
}
#endif // DWGSUPPORT
