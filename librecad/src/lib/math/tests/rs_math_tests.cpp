
/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD (librecad.org)
** Copyright (C) 2025 Dongxu Li github.com/dxli
**
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <QString>

#include "rs_math.h"

TEST_CASE("RS_Math::eval tests", "[rs_math]") {
    bool ok;

    SECTION("Basic arithmetic") {
        double result = RS_Math::eval("2 + 3", &ok);
        REQUIRE(ok == true);
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(5.0, 1e-6));
    }

    SECTION("Expression with parentheses") {
        double result = RS_Math::eval("(2 + 3) * 4", &ok);
        REQUIRE(ok == true);
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(20.0, 1e-6));
    }

    SECTION("Invalid expression") {
        double result = RS_Math::eval("2 + * 3", &ok);
        REQUIRE(ok == false);
        REQUIRE(result == 0.0); // Default return on failure
    }

    SECTION("Trigonometric function") {
        double result = RS_Math::eval("sin(0)", &ok);
        REQUIRE(ok == true);
        REQUIRE_THAT(result, Catch::Matchers::WithinAbs(0.0, 1e-6));
    }
}

TEST_CASE("RS_Math::derationalize tests", "[rs_math]") {
    QString result;

    SECTION("Simple fraction") {
        result = RS_Math::derationalize("1/2");
        REQUIRE_THAT(result.toDouble(), Catch::Matchers::WithinULP(0.5, 2));
    }

    SECTION("Mixed number") {
        result = RS_Math::derationalize("2-1/4");
        REQUIRE_THAT(result.toDouble(), Catch::Matchers::WithinULP(2.25, 2));
    }

    SECTION("Invalid rational") {
        result = RS_Math::derationalize("1/0");
        REQUIRE(result == "1/0");
    }

    SECTION("Decimal input") {
        result = RS_Math::derationalize("3.14");
        REQUIRE_THAT(result.toDouble(), Catch::Matchers::WithinULP(3.14, 2));
    }
}
