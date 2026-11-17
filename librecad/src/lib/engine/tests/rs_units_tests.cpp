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

// Tests for the degrees/minutes/seconds format of RS_Units::formatAngle().

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

#include <QString>

#include "rs_math.h"
#include "rs_units.h"

namespace Catch {
template <>
struct StringMaker<QString> {
    static std::string convert(const QString &value) {
        return '"' + value.toStdString() + '"';
    }
};
}

namespace {
QString dms(double degrees, int prec) {
    return RS_Units::formatAngle(RS_Math::deg2rad(degrees), RS2::DegreesMinutesSeconds, prec);
}
}

TEST_CASE("formatAngle DMS rounds the whole angle", "[rs_units]") {
    CHECK(dms(10.4999, 0) == QString::fromUtf8("10°"));
    CHECK(dms(10.5, 0) == QString::fromUtf8("11°"));
    CHECK(dms(10.0083, 1) == QString::fromUtf8("10° 0'"));
    CHECK(dms(10.99999, 1) == QString::fromUtf8("11° 0'"));
    CHECK(dms(10.9917, 2) == QString::fromUtf8("10° 59' 30\""));
}

TEST_CASE("formatAngle DMS carries a rounded up value", "[rs_units]") {
    CHECK(dms(10.9917, 1) == QString::fromUtf8("11° 0'"));
    CHECK(dms(10.99999, 2) == QString::fromUtf8("11° 0' 0\""));
    CHECK(dms(10.99999, 3) == QString::fromUtf8("11° 0' 0.0\""));
    CHECK(dms(10.4999999, 4) == QString::fromUtf8("10° 30' 0.00\""));
    CHECK(dms(10.99999, 5) == QString::fromUtf8("10° 59' 59.964\""));
}
