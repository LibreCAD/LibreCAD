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
 * DXF / DWG corpus round-trip harness (F0/F1 of the DXF-completeness plan).
 *
 * Gated behind the NON-default tag [corpus] so it never runs in the default
 * suite. It imports each developer-local sample through the REAL
 * RS_FilterDXFRW and exports it to a tmp directory; scripts/ezdxf_audit.py
 * then audits the output dir externally (read failures, non-unique-handle
 * errors, per-type in->out diff).
 *
 *   DXF samples : ~/dev/dwg_samples (.dxf)  DXF->DXF round-trip   tag [corpus]
 *   DWG samples : ~/doc/dwg and ~/doc/dwg2 (.dwg)  DWG->DXF       tag [corpus][dwgdxf]
 *
 * Outputs land in <tmp>/lc_dxf_corpus_out and <tmp>/lc_dwg_corpus_out.
 * Run, then:  scripts/ezdxf_audit.py <tmp>/lc_dxf_corpus_out --input ~/dev/dwg_samples
 *
 * These tests SUCCEED-skip cleanly when the corpora are absent, so they are
 * safe for everyone; they only do work on a developer machine that has the
 * sample directories.
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <QCoreApplication>

#include "lc_containertraverser.h"
#include "rs_entity.h"
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

std::vector<std::filesystem::path> listByExt(const std::filesystem::path &dir,
                                             const std::string &ext) {
  std::vector<std::filesystem::path> out;
  if (!std::filesystem::is_directory(dir))
    return out;
  for (const auto &e : std::filesystem::directory_iterator(dir)) {
    if (!e.is_regular_file())
      continue;
    std::string s = e.path().extension().string();
    for (char &c : s)
      c = (char)std::tolower((unsigned char)c);
    if (s == ext)
      out.push_back(e.path());
  }
  std::sort(out.begin(), out.end());
  return out;
}

// Import `in` via the real filter, export to `out` as DXF. Returns true if both
// halves of the round-trip reported success.
bool roundTrip(const std::filesystem::path &in, const std::filesystem::path &out,
               RS2::FormatType inFormat) {
  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    if (!filter.fileImport(graphic, QString::fromStdString(in.string()),
                           inFormat))
      return false;
  }
  RS_FilterDXFRW filter;
  return filter.fileExport(graphic, QString::fromStdString(out.string()),
                           RS2::FormatDXFRW);
}

// Count "0\n<NAME>" record markers in a DXF file.
int countDxfRecords(const std::filesystem::path &path, const std::string &name) {
  std::ifstream in(path);
  std::string line;
  int count = 0;
  bool prevZero = false;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    size_t a = line.find_first_not_of(" \t");
    std::string trimmed = (a == std::string::npos) ? std::string() : line.substr(a);
    if (prevZero && trimmed == name)
      ++count;
    prevZero = (trimmed == "0");
  }
  return count;
}

} // namespace

// F2: RAY/XLINE/TRACE/3DFACE are typed-read but model-converted (RAY/XLINE ->
// RS_Line, TRACE -> RS_Solid, 3DFACE -> RS_Polyline), which drops the original
// type + Z. A type-fidelity XDATA sidecar attached on read lets a write
// pre-pass rebuild the native DRW type with full geometry. This test drives the
// real filter end-to-end and asserts each type survives the round-trip, and
// that a genuine SOLID is NOT mis-rebuilt as a TRACE.
TEST_CASE("DXF round-trip preserves RAY/XLINE/TRACE/3DFACE native types (F2)",
          "[dxf][roundtrip][filter][f2]") {
  ensureSettings();
  const std::filesystem::path src =
      std::filesystem::temp_directory_path() / "f2_typefidelity_src.dxf";
  const std::filesystem::path out =
      std::filesystem::temp_directory_path() / "f2_typefidelity_out.dxf";
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // RAY (base+dir w/ Z), XLINE (base+dir w/ Z), TRACE (4 corners), 3DFACE
  // (4 corners w/ distinct Z + invisible-edge flag 5), plus a genuine SOLID
  // (must stay SOLID). AcDb subclass markers make it a valid R2010 ENTITIES.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nRAY\n8\n0\n100\nAcDbEntity\n100\nAcDbRay\n"
      "10\n1.0\n20\n2.0\n30\n3.0\n11\n1.0\n21\n0.0\n31\n0.0\n"
      "0\nXLINE\n8\n0\n100\nAcDbEntity\n100\nAcDbXline\n"
      "10\n4.0\n20\n5.0\n30\n6.0\n11\n0.0\n21\n1.0\n31\n0.0\n"
      "0\nTRACE\n8\n0\n100\nAcDbEntity\n100\nAcDbTrace\n"
      "10\n0.0\n20\n0.0\n30\n0.0\n11\n1.0\n21\n0.0\n31\n0.0\n"
      "12\n1.0\n22\n1.0\n32\n0.0\n13\n0.0\n23\n1.0\n33\n0.0\n"
      "0\n3DFACE\n8\n0\n100\nAcDbEntity\n100\nAcDbFace\n"
      "10\n0.0\n20\n0.0\n30\n1.0\n11\n10.0\n21\n0.0\n31\n2.0\n"
      "12\n10.0\n22\n10.0\n32\n3.0\n13\n0.0\n23\n10.0\n33\n4.0\n70\n5\n"
      "0\nSOLID\n8\n0\n100\nAcDbEntity\n100\nAcDbTrace\n"
      "10\n20.0\n20\n20.0\n30\n0.0\n11\n21.0\n21\n20.0\n31\n0.0\n"
      "12\n21.0\n22\n21.0\n32\n0.0\n13\n20.0\n23\n21.0\n33\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  {
    std::ofstream o(src);
    o << dxf;
  }

  REQUIRE(roundTrip(src, out, RS2::FormatDXFRW));

  CHECK(countDxfRecords(out, "RAY") == 1);
  CHECK(countDxfRecords(out, "XLINE") == 1);
  CHECK(countDxfRecords(out, "TRACE") == 1);
  CHECK(countDxfRecords(out, "3DFACE") == 1);
  CHECK(countDxfRecords(out, "SOLID") == 1);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

// F3a: RS_Entity now stores its source DXF/DWG handle (group code 5), captured
// in setEntityAttributes during import. This is the enabler for an old->new
// handle map (GROUP code-340 etc.). Assert a read LINE carries its handle.
TEST_CASE("DXF import captures the source entity handle on RS_Entity (F3a)",
          "[dxf][roundtrip][filter][handles][f3a]") {
  ensureSettings();
  const std::filesystem::path src =
      std::filesystem::temp_directory_path() / "f3a_srchandle.dxf";
  std::filesystem::remove(src);

  // Two LINEs with explicit code-5 handles 1A4 and 1A5.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n1A4\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n"
      "11\n10.0\n21\n10.0\n31\n0.0\n"
      "0\nLINE\n5\n1A5\n8\n0\n10\n1.0\n20\n1.0\n30\n0.0\n"
      "11\n11.0\n21\n11.0\n31\n0.0\n"
      "0\nENDSEC\n0\nEOF\n";
  {
    std::ofstream o(src);
    o << dxf;
  }

  RS_Graphic graphic;
  {
    RS_FilterDXFRW filter;
    REQUIRE(filter.fileImport(graphic, QString::fromStdString(src.string()),
                              RS2::FormatDXFRW));
  }

  std::set<quint32> seen;
  for (RS_Entity *e :
       lc::LC_ContainerTraverser{graphic, RS2::ResolveNone}.entities()) {
    if (e->rtti() == RS2::EntityLine && e->sourceHandle() != 0)
      seen.insert(e->sourceHandle());
  }
  // 0x1A4 == 420, 0x1A5 == 421.
  CHECK(seen.count(0x1A4) == 1);
  CHECK(seen.count(0x1A5) == 1);

  std::filesystem::remove(src);
}

// F3-3: a GROUP read from DXF must survive a DXF->DXF round-trip via the real
// filter: the codec mints the group handle, augments the ACAD_GROUP D dict, and
// resolves each member's SOURCE handle (the LINE code-5) through the
// source->minted map. This is end-to-end coverage the *.dxf corpus lacks (no
// sample carries a GROUP). Asserts the GROUP record reappears with two resolved
// 340 members and that no member 340 dangles.
TEST_CASE("DXF round-trip preserves a GROUP with resolved members (F3-3)",
          "[dxf][roundtrip][filter][group]") {
  ensureSettings();
  const std::filesystem::path src =
      std::filesystem::temp_directory_path() / "f33_group_src.dxf";
  const std::filesystem::path out =
      std::filesystem::temp_directory_path() / "f33_group_out.dxf";
  std::filesystem::remove(src);
  std::filesystem::remove(out);

  // Two LINEs (handles 1A4/1A5) and a named GROUP "MyGroup" referencing both.
  const std::string dxf =
      "0\nSECTION\n2\nENTITIES\n"
      "0\nLINE\n5\n1A4\n8\n0\n10\n0.0\n20\n0.0\n30\n0.0\n"
      "11\n10.0\n21\n10.0\n31\n0.0\n"
      "0\nLINE\n5\n1A5\n8\n0\n10\n1.0\n20\n1.0\n30\n0.0\n"
      "11\n11.0\n21\n11.0\n31\n0.0\n"
      "0\nENDSEC\n"
      "0\nSECTION\n2\nOBJECTS\n"
      "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n281\n1\n"
      "3\nACAD_GROUP\n350\nD\n"
      "0\nDICTIONARY\n5\nD\n330\nC\n100\nAcDbDictionary\n281\n1\n"
      "3\nMyGroup\n350\n2F\n"
      "0\nGROUP\n5\n2F\n330\nD\n100\nAcDbGroup\n"
      "300\nmy desc\n70\n0\n71\n1\n"
      "340\n1A4\n340\n1A5\n"
      "0\nENDSEC\n0\nEOF\n";
  {
    std::ofstream o(src);
    o << dxf;
  }

  REQUIRE(roundTrip(src, out, RS2::FormatDXFRW));

  // Exactly one GROUP reappears.
  CHECK(countDxfRecords(out, "GROUP") == 1);

  // The GROUP carries exactly two 340 members (both LINEs resolved, none
  // dangling). Parse the file into (code, value) pairs and count 340s only
  // inside the GROUP record (a value may itself be "0", so a line-based record
  // split is unsafe — split on the (code,value) "0"/<TYPE> marker pair).
  std::ifstream in(out);
  std::string codeLine;
  std::string valLine;
  auto trim = [](std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' '))
      s.pop_back();
    size_t a = s.find_first_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a);
  };
  bool inGroup = false;
  int memberCount = 0;
  bool sawDesc = false;
  while (std::getline(in, codeLine) && std::getline(in, valLine)) {
    const std::string code = trim(codeLine);
    const std::string val = trim(valLine);
    if (code == "0")
      inGroup = (val == "GROUP");
    else if (inGroup && code == "340")
      ++memberCount;
    else if (inGroup && code == "300" && val == "my desc")
      sawDesc = true;
  }
  CHECK(memberCount == 2);
  CHECK(sawDesc);

  in.close();
  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST_CASE("DXF corpus: round-trip ~/dev/dwg_samples/*.dxf to a tmp dir",
          "[corpus]") {
  ensureSettings();
  const char *home = std::getenv("HOME");
  if (!home) {
    SUCCEED("HOME not set; skipping DXF corpus round-trip");
    return;
  }
  const std::filesystem::path src = std::filesystem::path(home) / "dev" / "dwg_samples";
  const auto files = listByExt(src, ".dxf");
  if (files.empty()) {
    SUCCEED("no DXF samples at " << src.string() << "; skipping");
    return;
  }

  const std::filesystem::path outDir =
      std::filesystem::temp_directory_path() / "lc_dxf_corpus_out";
  std::filesystem::remove_all(outDir);
  std::filesystem::create_directories(outDir);

  int ok = 0, fail = 0;
  std::cout << "\n== DXF corpus round-trip (" << files.size() << " files) -> "
            << outDir.string() << "\n";
  for (const auto &f : files) {
    const std::filesystem::path out = outDir / f.filename();
    bool good = false;
    try {
      good = roundTrip(f, out, RS2::FormatDXFRW);
    } catch (const std::exception &e) {
      std::cout << "  EXC  " << f.filename().string() << ": " << e.what() << "\n";
    }
    if (good) {
      ++ok;
    } else {
      ++fail;
      std::cout << "  FAIL " << f.filename().string() << "\n";
    }
    CHECK(good);
  }
  std::cout << "== DXF corpus: " << ok << " ok, " << fail << " fail. Audit with:\n"
            << "   scripts/ezdxf_audit.py " << outDir.string()
            << " --input " << src.string() << "\n";
}

TEST_CASE("DWG corpus: convert ~/doc/dwg{,2}/*.dwg to DXF in a tmp dir",
          "[corpus][dwgdxf]") {
  ensureSettings();
  const char *home = std::getenv("HOME");
  if (!home) {
    SUCCEED("HOME not set; skipping DWG corpus conversion");
    return;
  }
  std::vector<std::filesystem::path> files;
  for (const char *sub : {"doc/dwg", "doc/dwg2"}) {
    const auto found = listByExt(std::filesystem::path(home) / sub, ".dwg");
    files.insert(files.end(), found.begin(), found.end());
  }
  if (files.empty()) {
    SUCCEED("no DWG samples at ~/doc/dwg{,2}; skipping");
    return;
  }

  const std::filesystem::path outDir =
      std::filesystem::temp_directory_path() / "lc_dwg_corpus_out";
  std::filesystem::remove_all(outDir);
  std::filesystem::create_directories(outDir);

  int ok = 0, fail = 0;
  std::cout << "\n== DWG->DXF corpus (" << files.size() << " files) -> "
            << outDir.string() << "\n";
  for (const auto &f : files) {
    // Disambiguate same-named files from the two source dirs.
    std::string stem = f.parent_path().filename().string() + "__" +
                       f.stem().string() + ".dxf";
    const std::filesystem::path out = outDir / stem;
    bool good = false;
    try {
      good = roundTrip(f, out, RS2::FormatDWG);
    } catch (const std::exception &e) {
      std::cout << "  EXC  " << f.filename().string() << ": " << e.what() << "\n";
    }
    if (good)
      ++ok;
    else {
      ++fail;
      std::cout << "  FAIL " << f.filename().string() << "\n";
    }
    // DWG read is allowed to fail on some files; do not hard-assert here.
  }
  std::cout << "== DWG->DXF corpus: " << ok << " ok, " << fail << " fail. Audit with:\n"
            << "   scripts/ezdxf_audit.py " << outDir.string() << "\n";
  SUCCEED("DWG->DXF conversion complete; audit the output dir externally");
}
