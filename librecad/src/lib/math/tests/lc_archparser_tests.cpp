/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 pcfixindude (github.com/pcfixindude)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "lc_archparser.h"

using Catch::Approx;

TEST_CASE("LC_ArchParser parses architectural distance syntax", "[lc_archparser]") {
    bool ok = false;

    CHECK(LC_ArchParser::parse("10-3", &ok) == Approx(123.0));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("10-3 7/8", &ok) == Approx(123.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("10-3-7/8", &ok) == Approx(123.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("237-7/8", &ok) == Approx(237.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("4 7/8", &ok) == Approx(4.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("4 7/8\"", &ok) == Approx(4.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("7/8\"", &ok) == Approx(0.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("4\"", &ok) == Approx(4.0));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("2'4-7/8", &ok) == Approx(28.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("2'-4 7/8\"", &ok) == Approx(28.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("2' 4 7/8\"", &ok) == Approx(28.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("2'-7/8\"", &ok) == Approx(24.875));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("2'-4\"", &ok) == Approx(28.0));
    CHECK(ok);

    CHECK(LC_ArchParser::parse("24'7.75", &ok) == Approx(295.75));
    CHECK(ok);
}

TEST_CASE("LC_ArchParser rejects ambiguous math-style input", "[lc_archparser]") {
    bool ok = true;

    CHECK(LC_ArchParser::parse("120", &ok) == Approx(0.0));
    CHECK_FALSE(ok);

    ok = true;
    CHECK(LC_ArchParser::parse("235.75", &ok) == Approx(0.0));
    CHECK_FALSE(ok);

    ok = true;
    CHECK(LC_ArchParser::parse("8/2", &ok) == Approx(0.0));
    CHECK_FALSE(ok);

    ok = true;
    CHECK(LC_ArchParser::parse("sqrt(2)*30", &ok) == Approx(0.0));
    CHECK_FALSE(ok);
}
