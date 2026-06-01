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

#include <filesystem>
#include <fstream>
#include <string>

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

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}
