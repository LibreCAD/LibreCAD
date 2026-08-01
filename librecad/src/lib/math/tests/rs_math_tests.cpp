
/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 LibreCAD (librecad.org)
** Copyright (C) 2025 Dongxu Li (github.com/dxli)
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
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <QString>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "rs_debug.h"
#include "rs_math.h"

using Catch::Approx;

namespace {
constexpr double EPS = 1e-6;
}

TEST_CASE("RS_Math::correctAngle") {
    REQUIRE(std::abs(RS_Math::correctAngle(0.0) - 0.0) < EPS);
    REQUIRE(std::abs(RS_Math::correctAngle(2 * M_PI) - 0.0) < EPS);
    REQUIRE(std::abs(RS_Math::correctAngle(-M_PI) - M_PI) < EPS);
    REQUIRE(std::abs(RS_Math::correctAngle(3 * M_PI) - M_PI) < EPS);
    REQUIRE(std::abs(RS_Math::correctAngle(4 * M_PI + M_PI / 2) - M_PI / 2) < EPS);
}

TEST_CASE("RS_Math::rad2deg") {
    REQUIRE(std::abs(RS_Math::rad2deg(0.0) - 0.0) < EPS);
    REQUIRE(std::abs(RS_Math::rad2deg(M_PI) - 180.0) < EPS);
    REQUIRE(std::abs(RS_Math::rad2deg(M_PI / 2) - 90.0) < EPS);
    REQUIRE(std::abs(RS_Math::rad2deg(-M_PI) - -180.0) < EPS);
}

TEST_CASE("RS_Math::deg2rad") {
    REQUIRE(std::abs(RS_Math::deg2rad(0.0) - 0.0) < EPS);
    REQUIRE(std::abs(RS_Math::deg2rad(180.0) - M_PI) < EPS);
    REQUIRE(std::abs(RS_Math::deg2rad(90.0) - M_PI / 2) < EPS);
    REQUIRE(std::abs(RS_Math::deg2rad(-180.0) - -M_PI) < EPS);
}

TEST_CASE("RS_Math::rad2gra") {
    REQUIRE(std::abs(RS_Math::rad2gra(0.0) - 0.0) < EPS);
    REQUIRE(std::abs(RS_Math::rad2gra(M_PI) - 200.0) < EPS);
    REQUIRE(std::abs(RS_Math::rad2gra(M_PI / 2) - 100.0) < EPS);
}

TEST_CASE("RS_Math::gra2rad") {
    REQUIRE(std::abs(RS_Math::gra2rad(0.0) - 0.0) < EPS);
    REQUIRE(std::abs(RS_Math::gra2rad(200.0) - M_PI) < EPS);
    REQUIRE(std::abs(RS_Math::gra2rad(100.0) - M_PI / 2) < EPS);
}

TEST_CASE("RS_Math::doubleToString") {
    REQUIRE(RS_Math::doubleToString(3.14159, 2) == "3.14");
    REQUIRE(RS_Math::doubleToString(42.0, 0) == "42");
    REQUIRE(RS_Math::doubleToString(1.23456789, 8) == "1.23456789");
    REQUIRE(RS_Math::doubleToString(0.0, 3) == "0.000");
    REQUIRE(RS_Math::doubleToString(-5.678, 1) == "-5.7");
}

TEST_CASE("RS_Math::eval") {
    bool ok;
    REQUIRE(std::abs(RS_Math::eval("2+3", &ok) - 5.0) < EPS);
    REQUIRE(ok == true);
    REQUIRE(std::abs(RS_Math::eval("10-4", &ok) - 6.0) < EPS);
    REQUIRE(ok == true);
    REQUIRE(std::abs(RS_Math::eval("3*4", &ok) - 12.0) < EPS);
    REQUIRE(ok == true);
    REQUIRE(std::abs(RS_Math::eval("8/2", &ok) - 4.0) < EPS);
    REQUIRE(ok == true);
    REQUIRE(std::abs(RS_Math::eval("3.14", &ok) - 3.14) < EPS);
    REQUIRE(ok == true);
    REQUIRE(std::abs(RS_Math::eval("invalid", &ok) - 0.0) < EPS);
    REQUIRE(ok == false);
}

TEST_CASE("RS_Math::pow") {
    REQUIRE(std::abs(RS_Math::pow(2.0, 3.0) - 8.0) < EPS);
    REQUIRE(std::abs(RS_Math::pow(9.0, 0.5) - 3.0) < EPS);
    REQUIRE(std::abs(RS_Math::pow(0.0, 1.0) - 0.0) < EPS);
}

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

    SECTION("Invalid rational") {
        result = RS_Math::derationalize("1/0");
        REQUIRE(result == "1/0");
    }

    SECTION("Decimal input") {
        result = RS_Math::derationalize("3.14");
        REQUIRE_THAT(result.toDouble(), Catch::Matchers::WithinULP(3.14, 2));
    }

/* 
// TODO: fix for those two formats in RS_Math::derationalize()
    SECTION("Fractal input") {
        result = RS_Math::derationalize("2 1/4");
        REQUIRE_THAT(result.toDouble(), Catch::Matchers::WithinAbs(2.25, 1e-6));
    }

    SECTION("Fractal input") {
        result = RS_Math::derationalize("2-1/4");
        REQUIRE_THAT(result.toDouble(), Catch::Matchers::WithinAbs(2.25, 1e-6));
    }
*/
}

// Regression for issue #2722.
//
// The hyperbola nearest-point search divides every quartic coefficient by a
// denominator that is exactly zero at a vertex, so it handed the solvers a set
// of NaN coefficients. Inside the complex branch of the cubic solver that
// becomes std::pow(std::complex, 1./3), which libstdc++ evaluates as
// std::polar(std::abs(x), ...). std::abs of a NaN complex is NaN, and
// std::polar asserts its modulus is non-negative - so the process aborted
// outright where the standard library is built with _GLIBCXX_ASSERTIONS,
// as on Arch Linux. libc++ has no such assertion, which is why this was
// invisible on macOS.
//
// The solvers now reject non-finite coefficients up front. Asserting that here
// rather than in the hyperbola tests keeps the check portable: it fails on any
// standard library, not only one that aborts.
TEST_CASE("RS_Math solvers reject non-finite coefficients (#2722)",
          "[rs_math][solver][regression]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    SECTION("cubicSolver with all-NaN coefficients") {
        CHECK(RS_Math::cubicSolver({nan, nan, nan}).empty());
    }
    SECTION("cubicSolver with a single NaN coefficient") {
        CHECK(RS_Math::cubicSolver({1.0, nan, -2.0}).empty());
    }
    SECTION("cubicSolver with an infinite coefficient") {
        CHECK(RS_Math::cubicSolver({inf, 1.0, 2.0}).empty());
        CHECK(RS_Math::cubicSolver({1.0, -inf, 2.0}).empty());
    }
    SECTION("quarticSolverFull with all-NaN coefficients") {
        CHECK(RS_Math::quarticSolverFull({nan, nan, nan, nan, 1.0}).empty());
    }
    SECTION("quarticSolverFull with a single NaN coefficient") {
        CHECK(RS_Math::quarticSolverFull({1.0, 2.0, nan, 3.0, 1.0}).empty());
    }

    // Finite input must still solve: without these, returning empty
    // unconditionally would pass every case above while disabling the solvers.
    SECTION("finite cubic still solves") {
        // (x-1)(x-2)(x-3) = x^3 - 6x^2 + 11x - 6
        const std::vector<double> roots = RS_Math::cubicSolver({-6.0, 11.0, -6.0});
        REQUIRE(roots.size() == 3);
        std::vector<double> sorted = roots;
        std::sort(sorted.begin(), sorted.end());
        CHECK(sorted[0] == Approx(1.0).margin(1e-9));
        CHECK(sorted[1] == Approx(2.0).margin(1e-9));
        CHECK(sorted[2] == Approx(3.0).margin(1e-9));
    }
    SECTION("finite quartic still solves") {
        // (x-1)(x-2)(x-3)(x-4) = x^4 - 10x^3 + 35x^2 - 50x + 24
        const std::vector<double> roots =
            RS_Math::quarticSolverFull({24.0, -50.0, 35.0, -10.0, 1.0});
        CHECK_FALSE(roots.empty());
        for (double r : roots) {
            CHECK(std::isfinite(r));
        }
    }
}
