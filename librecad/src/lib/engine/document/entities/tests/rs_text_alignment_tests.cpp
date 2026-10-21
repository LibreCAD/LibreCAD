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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QString>

#include "rs_fontlist.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_text.h"

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

} // namespace

TEST_CASE("RS_Text middle-center aligns ISO 3098 glyph bounds", "[text][alignment][issue2902]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("iso3098") != nullptr);

    const RS_Vector alignmentPoint(100.0, 50.0);
    const RS_TextData data(alignmentPoint, alignmentPoint, 3.5, 1.0,
                           RS_TextData::VAMiddle, RS_TextData::HACenter,
                           RS_TextData::None, "A", "iso3098", 0.0,
                           RS2::Update);
    RS_Text text(nullptr, data);

    REQUIRE(text.getMin().valid);
    REQUIRE(text.getMax().valid);
    CHECK(0.5 * (text.getMin().x + text.getMax().x)
          == Catch::Approx(alignmentPoint.x).margin(1e-9));
    CHECK(0.5 * (text.getMin().y + text.getMax().y)
          == Catch::Approx(alignmentPoint.y).margin(1e-9));
}
