/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
**********************************************************************/

// Tests for the degrees/minutes/seconds format of RS_Units::formatAngle() and
// the architectural length format.

#include <catch2/catch_test_macros.hpp>

#include <string>

#include <QString>

#include "rs_math.h"
#include "rs_units.h"

namespace {
// The text as UTF-8, which a failing check prints. A Catch::StringMaker<QString> in this
// file would not help: other test files instantiate Catch's own printer for QString under
// the same name, and the linker keeps one of those, which prints { {?}, {?}, ... }.
std::string dms(double degrees, int prec) {
    return RS_Units::formatAngle(RS_Math::deg2rad(degrees), RS2::DegreesMinutesSeconds, prec).toStdString();
}
}

TEST_CASE("formatArchitecturalMetric writes one minus sign", "[rs_units]") {
    auto metric = [](double length, int prec) {
        return RS_Units::formatArchitecturalMetric(length, RS2::Meter, prec, false).toStdString();
    };
    CHECK(metric(1.239, 2) == "1.24");
    CHECK(metric(-1.239, 2) == "-1.24");
    CHECK(metric(-1.235, 2) == "-1.23\u2075");
    CHECK(metric(0.42, 2) == "42");
    CHECK(metric(-0.42, 2) == "-42");
}

TEST_CASE("formatAngle DMS rounds the whole angle", "[rs_units]") {
    CHECK(dms(10.4999, 0) == "10°");
    CHECK(dms(10.5, 0) == "11°");
    CHECK(dms(10.0083, 1) == "10° 0'");
    CHECK(dms(10.99999, 1) == "11° 0'");
    CHECK(dms(10.9917, 2) == "10° 59' 30\"");
}

TEST_CASE("formatAngle DMS carries a rounded up value", "[rs_units]") {
    CHECK(dms(10.9917, 1) == "11° 0'");
    CHECK(dms(10.99999, 2) == "11° 0' 0\"");
    CHECK(dms(10.99999, 3) == "11° 0' 0.0\"");
    CHECK(dms(10.4999999, 4) == "10° 30' 0.00\"");
    CHECK(dms(10.99999, 5) == "10° 59' 59.964\"");
}

// A drawing can carry a negative precision: AutoCAD writes $DIMADEC = -1, and Drawing
// Preferences saves $AUPREC = -1 after a switch from decimal degrees with 7 or 8 places.
// Such a string has the fields of precision 2, so the angle is rounded to whole seconds.
TEST_CASE("formatAngle DMS with a negative precision rounds to whole seconds", "[rs_units]") {
    CHECK(dms(10.5, -1) == "10° 30' 0\"");
    CHECK(dms(10.4999, -1) == "10° 30' 0\"");
    CHECK(dms(280.25, -1) == "280° 15' 0\"");
    CHECK(dms(30.52555, -1) == "30° 31' 32\"");
}

TEST_CASE("formatArchitectural keeps the sign of lengths below one foot", "[rs_units]") {
    auto arch = [](double length, RS2::Unit unit, int prec) {
        return RS_Units::formatArchitectural(length, unit, prec, false).toStdString();
    };
    CHECK(arch(-6.0, RS2::Inch, 1) == "-6\"");
    CHECK(arch(-0.5, RS2::Foot, 1) == "-6\"");
    CHECK(arch(-0.25, RS2::Inch, 1) == "-1/4\"");
    CHECK(arch(6.0, RS2::Inch, 1) == "6\"");
    CHECK(arch(-18.0, RS2::Inch, 1) == "-1'-6\"");
    // rounds to zero, so no sign
    CHECK(arch(-0.01, RS2::Inch, 1) == "0\"");
}
