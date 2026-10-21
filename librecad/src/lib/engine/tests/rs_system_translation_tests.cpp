/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

#include <catch2/catch_test_macros.hpp>

#include "rs_system.h"

TEST_CASE("Spanish regional locales fall back to shared Spanish", "[translation]") {
    CHECK(RS_System::translationLocaleFallbacks("es_AR") == (QStringList{"es", "es_AR"}));
    CHECK(RS_System::translationLocaleFallbacks("es-ES") == (QStringList{"es", "es_ES"}));
    CHECK(RS_System::translationLocaleFallbacks("ES_mx") == (QStringList{"es", "es_MX"}));
    CHECK(RS_System::translationLocaleFallbacks("es") == QStringList{"es"});
}

TEST_CASE("Non-Spanish regional locales do not inherit Spanish or base catalogs", "[translation]") {
    CHECK(RS_System::translationLocaleFallbacks("fr_CA") == QStringList{"fr_CA"});
    CHECK(RS_System::translationLocaleFallbacks("en") == QStringList{"en"});
    CHECK(RS_System::translationLocaleFallbacks("").isEmpty());
}
