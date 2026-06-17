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
 * RS_Graphic paper-space layout API tests (PR 9 — DWG OBJECTS surface).
 *
 * Covers the LibreCAD-side accessors that expose the LayoutRecord set
 * populated by RS_FilterDXFRW::addLayout during DWG import:
 *   - RS_Graphic::layouts() thin view over LC_DwgAdvancedMetadata
 *   - RS_Graphic::findLayout(handle)
 *   - RS_Graphic::activeLayoutHandle() / setActiveLayoutHandle()
 *   - RS_Graphic::setLayoutMargins(handle, ...)
 *
 * Asserts the modified flag is only bumped when state actually changes,
 * so the UI tab bar (PR 10) can safely call setActiveLayoutHandle() on
 * every tab click without dirtying clean drawings.
 */

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>

#include "drw_objects.h"
#include "lc_dwgadvancedmetadata.h"
#include "rs_graphic.h"
#include "rs_settings.h"

namespace {

DRW_Layout makeLayout(std::uint32_t handle, const std::string& name,
                      double left = 0.0, double top = 0.0,
                      double right = 0.0, double bottom = 0.0) {
    DRW_Layout layout;
    layout.handle = handle;
    layout.name = name;
    layout.marginLeft = left;
    layout.marginTop = top;
    layout.marginRight = right;
    layout.marginBottom = bottom;
    return layout;
}

// RS_Graphic's ctor calls into LC_GROUP_GUARD("Defaults") + RS_Settings; both
// dereference null without a QCoreApplication context.  Mirrors the bootstrap
// dwg_smoke_tests.cpp uses for the same reason.
void ensureQtContext() {
    static int qargc = 1;
    static char qarg0[] = "librecad_tests";
    static char* qargv[] = {qarg0, nullptr};
    static QCoreApplication* qapp = QCoreApplication::instance()
        ? QCoreApplication::instance()
        : new QCoreApplication(qargc, qargv);
    (void)qapp;
    static bool settingsReady = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)settingsReady;
}

}  // namespace

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::layouts returns empty for a fresh document",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    REQUIRE(graphic.layouts().empty());
    REQUIRE(graphic.activeLayoutHandle() == 0u);
    REQUIRE(graphic.findLayout(0) == nullptr);
    REQUIRE(graphic.findLayout(42) == nullptr);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::layouts surfaces records added via addLayout",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x10, "Model"));
    metadata.addLayout(makeLayout(0x20, "Layout1", 10.0, 5.0, 10.0, 5.0));

    REQUIRE(graphic.layouts().size() == 2);
    REQUIRE(graphic.layouts()[0].name == "Model");
    REQUIRE(graphic.layouts()[1].name == "Layout1");
    REQUIRE(graphic.layouts()[1].marginLeft == 10.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::findLayout matches by handle",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x10, "Model"));
    metadata.addLayout(makeLayout(0x20, "Layout1"));

    const LC_Layout* match = graphic.findLayout(0x20);
    REQUIRE(match != nullptr);
    REQUIRE(match->name == "Layout1");

    REQUIRE(graphic.findLayout(0x99) == nullptr);
    REQUIRE(graphic.findLayout(0) == nullptr);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setActiveLayoutHandle bumps modified only on change",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    REQUIRE_FALSE(graphic.isModified());

    // First-time set: state changed, expect modified.
    graphic.setActiveLayoutHandle(0x20);
    REQUIRE(graphic.activeLayoutHandle() == 0x20u);
    REQUIRE(graphic.isModified());

    // Redundant set with the same handle: must not re-dirty after a save.
    graphic.setModified(false);
    REQUIRE_FALSE(graphic.isModified());
    graphic.setActiveLayoutHandle(0x20);
    REQUIRE_FALSE(graphic.isModified());

    // Change to a different handle: dirty again.
    graphic.setActiveLayoutHandle(0x30);
    REQUIRE(graphic.activeLayoutHandle() == 0x30u);
    REQUIRE(graphic.isModified());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setLayoutMargins updates record and bumps modified",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x20, "Layout1", 5.0, 5.0, 5.0, 5.0));

    REQUIRE_FALSE(graphic.isModified());
    REQUIRE(graphic.setLayoutMargins(0x20, 10.0, 20.0, 30.0, 40.0));

    const LC_Layout* match = graphic.findLayout(0x20);
    REQUIRE(match != nullptr);
    REQUIRE(match->marginLeft == 10.0);
    REQUIRE(match->marginTop == 20.0);
    REQUIRE(match->marginRight == 30.0);
    REQUIRE(match->marginBottom == 40.0);
    REQUIRE(graphic.isModified());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setLayoutMargins rejects unknown handle",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x20, "Layout1"));

    REQUIRE_FALSE(graphic.setLayoutMargins(0, 1.0, 1.0, 1.0, 1.0));
    REQUIRE_FALSE(graphic.setLayoutMargins(0x99, 1.0, 1.0, 1.0, 1.0));
    REQUIRE_FALSE(graphic.isModified());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setLayoutMargins ignores negative components",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x20, "Layout1", 5.0, 5.0, 5.0, 5.0));

    // Mirrors the legacy RS_Graphic::setMargins convention: negative
    // values mean "leave this side alone" so the plot dialog can update
    // a single margin without resupplying the other three.
    REQUIRE(graphic.setLayoutMargins(0x20, 11.0, -1.0, -1.0, -1.0));
    const LC_Layout* match = graphic.findLayout(0x20);
    REQUIRE(match != nullptr);
    REQUIRE(match->marginLeft == 11.0);
    REQUIRE(match->marginTop == 5.0);
    REQUIRE(match->marginRight == 5.0);
    REQUIRE(match->marginBottom == 5.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setLayoutMargins is a no-op when values match",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x20, "Layout1", 5.0, 5.0, 5.0, 5.0));

    REQUIRE(graphic.setLayoutMargins(0x20, 5.0, 5.0, 5.0, 5.0));
    REQUIRE_FALSE(graphic.isModified());
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::layouts() is a live view over dwgAdvancedMetadata",
          "[rs_graphic][layouts]") {
    // Surfacing the underlying storage by reference (rather than by
    // value copy) is what lets PR 10's tab bar receive add/remove
    // notifications by polling — no separate listener layer required.
    ensureQtContext();
    RS_Graphic graphic;
    REQUIRE(graphic.layouts().empty());

    graphic.dwgAdvancedMetadata().addLayout(makeLayout(0x10, "Model"));
    REQUIRE(graphic.layouts().size() == 1);

    graphic.dwgAdvancedMetadata().addLayout(makeLayout(0x20, "Layout1"));
    REQUIRE(graphic.layouts().size() == 2);
    REQUIRE(graphic.layouts().back().name == "Layout1");
}

// ---- PR 11 — active-layout margin accessors --------------------------------

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::activeLayoutMargins falls back to legacy when handle is 0",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    // No layouts loaded (DXF or fresh document) — activeLayoutHandle() is
    // 0 by construction; activeLayoutMargins() must return the document
    // singleton so the existing plot dialog read path is byte-identical.
    graphic.setMargins(7.0, 8.0, 9.0, 10.0);
    const auto margins = graphic.activeLayoutMargins();
    REQUIRE(margins[0] == 7.0);
    REQUIRE(margins[1] == 8.0);
    REQUIRE(margins[2] == 9.0);
    REQUIRE(margins[3] == 10.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::activeLayoutMargins prefers active LayoutRecord when set",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    graphic.setMargins(1.0, 2.0, 3.0, 4.0);
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x20, "Layout1", 11.0, 22.0, 33.0, 44.0));
    graphic.setActiveLayoutHandle(0x20);

    const auto margins = graphic.activeLayoutMargins();
    REQUIRE(margins[0] == 11.0);
    REQUIRE(margins[1] == 22.0);
    REQUIRE(margins[2] == 33.0);
    REQUIRE(margins[3] == 44.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::activeLayoutMargins falls back when handle has no match",
          "[rs_graphic][layouts]") {
    // Defensive — if the active handle is stale (layout removed, etc.)
    // the read path must not segfault and must surface usable legacy
    // margins so the plot dialog still has something to display.
    ensureQtContext();
    RS_Graphic graphic;
    graphic.setMargins(5.0, 6.0, 7.0, 8.0);
    graphic.setActiveLayoutHandle(0x99);  // no matching record

    const auto margins = graphic.activeLayoutMargins();
    REQUIRE(margins[0] == 5.0);
    REQUIRE(margins[1] == 6.0);
    REQUIRE(margins[2] == 7.0);
    REQUIRE(margins[3] == 8.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setActiveLayoutMargins writes through to LayoutRecord",
          "[rs_graphic][layouts]") {
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x20, "Layout1", 5.0, 5.0, 5.0, 5.0));
    graphic.setActiveLayoutHandle(0x20);
    graphic.setModified(false);

    graphic.setActiveLayoutMargins(10.0, 20.0, 30.0, 40.0);
    const auto margins = graphic.activeLayoutMargins();
    REQUIRE(margins[0] == 10.0);
    REQUIRE(margins[1] == 20.0);
    REQUIRE(margins[2] == 30.0);
    REQUIRE(margins[3] == 40.0);
    REQUIRE(graphic.isModified());

    // Legacy singletons must remain untouched — they're independent
    // storage for the DXF / no-layout fallback path.
    REQUIRE(graphic.getMarginLeft() == 0.0);
    REQUIRE(graphic.getMarginTop() == 0.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setActiveLayoutMargins falls back to setMargins for DXF",
          "[rs_graphic][layouts]") {
    // DXF or no-layout drawings have activeLayoutHandle()==0; the setter
    // must transparently write to the legacy singletons so the existing
    // plot pipeline observes the user's edit.
    ensureQtContext();
    RS_Graphic graphic;
    graphic.setActiveLayoutMargins(11.0, 22.0, 33.0, 44.0);

    REQUIRE(graphic.getMarginLeft() == 11.0);
    REQUIRE(graphic.getMarginTop() == 22.0);
    REQUIRE(graphic.getMarginRight() == 33.0);
    REQUIRE(graphic.getMarginBottom() == 44.0);
    REQUIRE(graphic.activeLayoutMargins()[0] == 11.0);
}

// NOLINTNEXTLINE(readability-identifier-naming)
TEST_CASE("RS_Graphic::setActiveLayoutMargins skips negative components",
          "[rs_graphic][layouts]") {
    // Mirrors setMargins / setLayoutMargins convention — negative means
    // "leave this side alone".
    ensureQtContext();
    RS_Graphic graphic;
    auto& metadata = graphic.dwgAdvancedMetadata();
    metadata.addLayout(makeLayout(0x20, "Layout1", 5.0, 5.0, 5.0, 5.0));
    graphic.setActiveLayoutHandle(0x20);

    graphic.setActiveLayoutMargins(11.0, -1.0, -1.0, -1.0);
    const auto margins = graphic.activeLayoutMargins();
    REQUIRE(margins[0] == 11.0);
    REQUIRE(margins[1] == 5.0);
    REQUIRE(margins[2] == 5.0);
    REQUIRE(margins[3] == 5.0);
}
