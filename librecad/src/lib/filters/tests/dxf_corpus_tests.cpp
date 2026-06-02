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
#include <iostream>
#include <string>
#include <vector>

#include <QCoreApplication>

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

} // namespace

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
