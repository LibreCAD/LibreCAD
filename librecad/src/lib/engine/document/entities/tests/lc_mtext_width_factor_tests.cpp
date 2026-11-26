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
#include <QList>
#include <QString>

#include <string>
#include <vector>

#include "rs_document.h"
#include "rs_fontlist.h"
#include "rs_insert.h"
#include "rs_modification.h"
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

RS_MTextData textData(const QString& text) {
    return RS_MTextData(RS_Vector(0.0, 0.0), 9.0, 0.0, RS_MTextData::VATop,
                        RS_MTextData::HALeft, RS_MTextData::LeftToRight,
                        RS_MTextData::Exact, 1.0, text, "standard", 0.0,
                        RS2::Update);
}

double textWidth(const QString& text) {
    const RS_MText mtext(nullptr, textData(text));
    return mtext.getUsedTextWidth();
}

// The width factor of each letter drawn, from the left, stacks included: the
// horizontal scale of its insert over the vertical one.
std::vector<double> letterWidthFactors(const RS_EntityContainer& container) {
    std::vector<double> factors;
    for (const RS_Entity* entity : container) {
        if (entity->rtti() == RS2::EntityInsert) {
            const RS_Vector scale = static_cast<const RS_Insert*>(entity)->getScale();
            factors.push_back(scale.x / scale.y);
        }
        else if (entity->isContainer()) {
            const auto nested = letterWidthFactors(*static_cast<const RS_EntityContainer*>(entity));
            factors.insert(factors.end(), nested.begin(), nested.end());
        }
    }
    return factors;
}

void checkLetterWidths(const QString& text, const std::vector<double>& expected) {
    INFO(text.toStdString());
    const RS_MText mtext(nullptr, textData(text));
    const auto factors = letterWidthFactors(mtext);
    REQUIRE(factors.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        INFO("letter " << i);
        CHECK(factors[i] == Catch::Approx(expected[i]));
    }
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

    // drawn as it is when its backslash is escaped
    CHECK(textWidth("\\Work") == Catch::Approx(textWidth("\\\\Work")));
    CHECK(textWidth("\\W;ork") == Catch::Approx(textWidth("\\\\W;ork")));
    CHECK(textWidth("\\W0.5ork") == Catch::Approx(textWidth("\\\\W0.5ork")));
}

TEST_CASE("RS_MText does not draw a \\W that sets no width", "[mtext][width]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("standard") != nullptr);

    // RS_MText::toPlainLine() leaves these out as well; the width stays
    const double plain = textWidth("AB CD");
    CHECK(textWidth("\\W0;AB CD") == Catch::Approx(plain));
    CHECK(textWidth("\\W-2;AB CD") == Catch::Approx(plain));
    CHECK(textWidth("\\W2;\\W0x;AB CD") == Catch::Approx(2.0 * plain));
}

TEST_CASE("RS_MText ends a \\W with the group it is in", "[mtext][width]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("standard") != nullptr);

    // the braces are drawn, as before
    checkLetterWidths("{\\W0.5;AB}CD", {1.0, 0.5, 0.5, 1.0, 1.0, 1.0});
    checkLetterWidths("\\W0.5;{AB}CD", {0.5, 0.5, 0.5, 0.5, 0.5, 0.5});
    checkLetterWidths("{{\\W0.5;A}\\W2;B}C", {1.0, 1.0, 0.5, 1.0, 2.0, 1.0, 1.0});
    // "\}" is a brace of the text: it closes no group
    checkLetterWidths("{\\W0.5;A\\}B}C", {1.0, 0.5, 0.5, 0.5, 1.0, 1.0});
    // a "}" without a group ends no width, nor does a new line
    checkLetterWidths("\\W0.5;A}B", {0.5, 0.5, 0.5});
    checkLetterWidths("\\W0.5;A\\PB", {0.5, 0.5});
}

TEST_CASE("RS_MText draws a stack at the width factor", "[mtext][width]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("standard") != nullptr);

    const double stack = textWidth("\\S1^2;");
    REQUIRE(stack > 0.0);

    checkLetterWidths("\\W0.5;\\S1^2;", {0.5, 0.5});
    CHECK(textWidth("\\W2;\\S1^2;") == Catch::Approx(2.0 * stack));
    // the parts get the factor itself, not one rounded to six decimals
    CHECK(textWidth("\\W0.0000001;\\S1^2;") == Catch::Approx(0.0000001 * stack));
}

TEST_CASE("Exploding \\W text into letters keeps their width", "[mtext][width]") {
    SourceFonts fonts;
    REQUIRE(RS_FONTLIST->requestFont("standard") != nullptr);

    RS_MText mtext(nullptr, textData("\\W0.5;AB"));
    LC_DocumentModificationBatch ctx;
    REQUIRE(RS_Modification::explodeTextIntoLetters({&mtext}, ctx));
    REQUIRE(ctx.entitiesToAdd.size() == 2);
    for (const RS_Entity* entity : std::as_const(ctx.entitiesToAdd)) {
        const auto* letter = dynamic_cast<const RS_MText*>(entity);
        REQUIRE(letter != nullptr);
        const auto factors = letterWidthFactors(*letter);
        REQUIRE(factors.size() == 1);
        CHECK(factors.front() == Catch::Approx(0.5));
    }
    qDeleteAll(ctx.entitiesToAdd);
}
