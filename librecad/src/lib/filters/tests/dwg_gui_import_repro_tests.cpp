/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
**********************************************************************/

/**
 * GUI-import repro for usa_dollar100_front.dwg (AC1021, 30 MB, 41364 3D
 * polylines / 1.16M vertices). The libdxfrw READ is clean (0 parse failures),
 * but the full RS_FilterDXFRW::fileImport -> RS_Graphic build path (what the
 * GUI uses) reportedly fails/crashes. This dev-local test drives exactly that
 * path so the failure can be reproduced and located.
 */

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>

#include <cstdlib>
#include <filesystem>
#include <string>

#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_filterdxfrw.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_settings.h"

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("GUI import of usa_dollar100_front.dwg completes", "[.dwg6_gui_import]") {
  const char *home = std::getenv("HOME");
  if (!home) {
    SUCCEED("HOME not set; skipping");
    return;
  }
  const std::string path =
      std::string(home) + "/doc/dwg6/usa_dollar100_front.dwg";
  if (!std::filesystem::is_regular_file(path)) {
    SUCCEED("usa_dollar100_front.dwg not present; skipping");
    return;
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

  // Measure the true GUI cost at the default (non-verbose) log level -- the
  // test singleton otherwise inits at D_DEBUGGING, which floods a per-entity
  // vfprintf+fflush over 1.16M entities and dwarfs the real work.
  RS_DEBUG->setLevel(RS_Debug::D_NOTHING);

  RS_Graphic graphic;
  RS_FilterDXFRW filter;
  const bool imported =
      filter.fileImport(graphic, QString::fromStdString(path), RS2::FormatDWG);
  REQUIRE(imported);

  int top = 0;
  int polylines = 0;
  for (auto *e : graphic) {
    if (!e)
      continue;
    ++top;
    if (e->rtti() == RS2::EntityPolyline)
      ++polylines;
  }
  INFO("top-level entities: " << top << " polylines: " << polylines);
  // dwgread oracle: 41364 3D polylines + 4 LWPOLYLINE. The import must build
  // them all -- and (the point of this test) it must do so in seconds, not the
  // ~10 minutes it took before RS_Polyline::addVertex stopped calling
  // endPolyline() (a full O(N) calculateBorders) on every vertex (O(N^2)).
  CHECK(polylines >= 41368);
}
