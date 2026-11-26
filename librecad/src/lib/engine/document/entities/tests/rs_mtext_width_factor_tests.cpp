/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
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

#include <QCoreApplication>
#include <QString>

#include "rs_fontlist.h"
#include "rs_mtext.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {

class SourceFonts {
public:
    SourceFonts() {
        static int argc = 1;
        static char arg0[] = "librecad_tests";
        static char* argv[] = {arg0, nullptr};
        static QCoreApplication* app = QCoreApplication::instance()
            ? QCoreApplication::instance()
            : new QCoreApplication(argc, argv);
        static bool settingsReady = [] {
            QCoreApplication::setOrganizationName("LibreCAD");
            QCoreApplication::setApplicationName("LibreCAD-tests");
            RS_Settings::init("LibreCAD", "LibreCAD-tests");
            RS_SYSTEM->init("LibreCAD", "tests", "librecad", "librecad_tests");
            return true;
        }();
        (void)app;
        (void)settingsReady;

        LC_GROUP_GUARD("Paths");
        m_previous = LC_GET_STR("Fonts", "");
        LC_SET("Fonts", QStringLiteral(LIBRECAD_SOURCE_DIR "/librecad/support/fonts"));
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }

    ~SourceFonts() {
        LC_GROUP_GUARD("Paths");
        LC_SET("Fonts", m_previous);
        RS_FONTLIST->clearFonts();
        RS_FONTLIST->init();
    }

private:
    QString m_previous;
};

double textWidth(const QString& text) {
    const RS_MTextData data(RS_Vector(0.0, 0.0), 9.0, 0.0, RS_MTextData::VATop,
                            RS_MTextData::HALeft, RS_MTextData::LeftToRight,
                            RS_MTextData::Exact, 1.0, text, "standard", 0.0,
                            RS2::Update);
    const RS_MText mtext(nullptr, data);
    return mtext.getUsedTextWidth();
}

} // namespace

TEST_CASE("RS_MText applies the \\W width factor instead of drawing it", "[mtext][width]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("standard") != nullptr);

    const double plain = textWidth("AB CD");
    REQUIRE(plain > 0.0);

    CHECK(textWidth("\\W1;AB CD") == Catch::Approx(plain));
    CHECK(textWidth("\\W2;AB CD") == Catch::Approx(2.0 * plain));
    CHECK(textWidth("\\W0.5;AB CD") == Catch::Approx(0.5 * plain));
}

TEST_CASE("RS_MText multiplies a relative \\W width factor", "[mtext][width]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("standard") != nullptr);

    CHECK(textWidth("\\W2;\\W0.5x;ABCD") == Catch::Approx(textWidth("ABCD")));
}

TEST_CASE("RS_MText keeps an incomplete \\W as text", "[mtext][width]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("standard") != nullptr);

    CHECK(textWidth("\\Work") > textWidth("ork"));
}
