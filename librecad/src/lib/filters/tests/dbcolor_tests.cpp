/**
 * AcDbColor (DBCOLOR) named-color round-trip tests.
 *
 * Targets the RS_Color::colorName accessors added so that DWG/DXF
 * book-color references (formatted "BOOK$ENTRY") survive import/export
 * via rs_filterdxfrw without being silently dropped.  Integration
 * coverage for the libdxfrw DBCOLOR dispatch lives in
 * dwg_smoke_tests.cpp's [.dwg_acdbcolor*] probes.
 */

#include <catch2/catch_test_macros.hpp>

#include "rs_color.h"
#include "rs_pen.h"

TEST_CASE("RS_Color: colorName defaults to empty", "[dbcolor][rs_color]") {
    RS_Color c(10, 20, 30);
    REQUIRE_FALSE(c.hasColorName());
    REQUIRE(c.colorName().isEmpty());
}

TEST_CASE("RS_Color: setColorName / colorName round-trip", "[dbcolor][rs_color]") {
    RS_Color c(10, 20, 30);
    c.setColorName("RAL$RAL 1003");
    REQUIRE(c.hasColorName());
    REQUIRE(c.colorName() == "RAL$RAL 1003");
}

TEST_CASE("RS_Color: copy preserves colorName", "[dbcolor][rs_color]") {
    RS_Color src(10, 20, 30);
    src.setColorName("PANTONE$PMS 185");

    RS_Color copy = src;
    REQUIRE(copy.hasColorName());
    REQUIRE(copy.colorName() == "PANTONE$PMS 185");
    // RGB intact too.
    REQUIRE(copy.red() == 10);
    REQUIRE(copy.green() == 20);
    REQUIRE(copy.blue() == 30);
}

TEST_CASE("RS_Color: operator== ignores colorName", "[dbcolor][rs_color]") {
    // Design intent: the name is a passive round-trip tag, not a
    // visual property.  Two pens that render identically must compare
    // equal regardless of metadata, so existing pen-equality call
    // sites in the rendering path are unperturbed.
    RS_Color a(10, 20, 30);
    RS_Color b(10, 20, 30);
    a.setColorName("BookA$Entry1");
    b.setColorName("BookB$Entry2");
    REQUIRE(a == b);
}

TEST_CASE("RS_Pen carries colorName via its RS_Color", "[dbcolor][rs_pen]") {
    RS_Color c(200, 100, 50);
    c.setColorName("DIC$DIC 120");

    RS_Pen pen;
    pen.setColor(c);

    REQUIRE(pen.getColor().hasColorName());
    REQUIRE(pen.getColor().colorName() == "DIC$DIC 120");
    REQUIRE(pen.getColor().red() == 200);
    REQUIRE(pen.getColor().green() == 100);
    REQUIRE(pen.getColor().blue() == 50);
}

TEST_CASE("RS_Color: clearing colorName works", "[dbcolor][rs_color]") {
    RS_Color c(0, 0, 0);
    c.setColorName("SomeBook$SomeEntry");
    REQUIRE(c.hasColorName());
    c.setColorName(QString{});
    REQUIRE_FALSE(c.hasColorName());
}
