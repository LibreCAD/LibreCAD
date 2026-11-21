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

TEST_CASE("formatEngineering and formatFractional drop the sign of lengths that round to zero", "[rs_units]") {
    auto eng = [](double length) {
        return RS_Units::formatEngineering(length, RS2::Inch, 2, false).toStdString();
    };
    auto frac = [](double length) {
        return RS_Units::formatFractional(length, RS2::Inch, 2, false).toStdString();
    };
    CHECK(eng(-0.001) == "0.00\"");
    CHECK(eng(-0.0) == "0.00\"");
    CHECK(eng(-0.4) == "-0.40\"");
    CHECK(eng(-18.0) == "-1'-6.00\"");
    CHECK(frac(-0.001) == "0");
    CHECK(frac(-0.4) == "- 3/8");
    CHECK(frac(-1.5) == "- 1 1/2");
}

TEST_CASE("formatEngineering carries inches that round up to 12 into the feet", "[rs_units]") {
    auto eng = [](double length, int prec) {
        return RS_Units::formatEngineering(length, RS2::Inch, prec, false).toStdString();
    };
    CHECK(eng(23.999, 2) == "2'-0.00\"");
    CHECK(eng(-23.999, 2) == "-2'-0.00\"");
    CHECK(eng(11.9999, 2) == "1'-0.00\"");
    CHECK(eng(23.999, 0) == "2'-0\"");
}

TEST_CASE("Negative lengths get a sign exactly when the text is not zero", "[rs_units]") {
    using Formatter = QString (*)(double, RS2::Unit, int, bool);
    const std::pair<const char*, Formatter> formats[] = {
        {"engineering", &RS_Units::formatEngineering},
        {"architectural", &RS_Units::formatArchitectural},
        {"fractional", &RS_Units::formatFractional},
    };
    auto hasNonZeroDigit = [](const std::string& s) {
        return s.find_first_of("123456789") != std::string::npos;
    };
    for (const auto& [name, format] : formats) {
        for (int prec = 0; prec <= 8; ++prec) {
            // half of the smallest shown step: the rounding boundary next to zero
            const double step = (format == &RS_Units::formatEngineering) ? std::pow(10., -prec) : 1. / (2 << prec);
            std::vector<double> lengths{-0., -std::numeric_limits<double>::denorm_min()};
            for (const double base : {0.5 * step, step, 1.5 * step, 12. - 0.5 * step, 12., 24. - 0.5 * step}) {
                for (const double eps : {0., 1e-12, -1e-12}) {
                    const double x = base + eps;
                    lengths.push_back(-x);
                    lengths.push_back(-std::nextafter(x, 0.));
                    lengths.push_back(-std::nextafter(x, 100.));
                }
            }
            for (int i = 1; i <= 2000; ++i) {
                lengths.push_back(-i * 0.0137);
            }
            for (const double length : lengths) {
                const std::string text = format(length, RS2::Inch, prec, false).toStdString();
                const std::string positive = format(-length, RS2::Inch, prec, false).toStdString();
                CAPTURE(name, prec, length, text, positive);
                CHECK((text.front() == '-') == hasNonZeroDigit(text));
                CHECK(positive.front() != '-');
                // the sign is the only difference from the positive length
                const std::string unsigned_text = text.front() != '-' ? text
                    : text.substr(text.compare(0, 2, "- ") == 0 ? 2 : 1);
                CHECK(unsigned_text == positive);
                // inches that round to 12 carry into the feet: never 12", 12.00" or 12 1/2"
                const std::size_t dash = unsigned_text.find("'-");
                const std::string inches = unsigned_text.substr(dash == std::string::npos ? 0 : dash + 2);
                if (format != &RS_Units::formatFractional) {
                    const bool wholeTwelve = inches.rfind("12", 0) == 0 && inches.size() > 2
                        && (inches[2] == '"' || inches[2] == '.' || inches[2] == ' ');
                    CHECK_FALSE(wholeTwelve);
                }
            }
        }
    }
}
