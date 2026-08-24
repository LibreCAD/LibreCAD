/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
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
**********************************************************************/

/**
 * Regression tests for issue #2758: Turkish-specific characters
 * (ğ, Ğ, ş, Ş, ı, İ) had no glyph entries in the default LFF font, so
 * RS_Font::findLetter() returned nullptr for them. RS_Text::update() and
 * RS_MText::update() both react to a missing glyph by silently substituting
 * QChar(0xfffd) - which standard.lff *does* define - so the visible symptom
 * was every Turkish-specific character rendering as the replacement-character
 * diamond instead of itself.
 *
 * These tests load the real, installed standard.lff and assert the six
 * codepoints resolve to a non-empty glyph, so a future edit that breaks the
 * LFF syntax (a stray blank line, a malformed coordinate, a wrong hex key)
 * fails loudly here instead of silently falling back to the diamond again.
 */

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QString>

#include "rs_font.h"
#include "rs_block.h"
#include "rs_settings.h"

namespace {

void ensureApp() {
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char *argv[] = {arg0, nullptr};
    static QCoreApplication *app = QCoreApplication::instance()
                                       ? QCoreApplication::instance()
                                       : new QCoreApplication(argc, argv);
    static bool ready = [] {
        QCoreApplication::setOrganizationName("LibreCAD");
        QCoreApplication::setApplicationName("LibreCAD-tests");
        RS_Settings::init("LibreCAD", "LibreCAD-tests");
        return true;
    }();
    (void)app;
    (void)ready;
}

QString standardFontPath() {
    return QString(LIBRECAD_SOURCE_DIR) + "/librecad/support/fonts/standard.lff";
}

} // namespace

TEST_CASE("standard.lff resolves every Turkish-specific character", "[font][i18n][issue2758]") {
    ensureApp();

    RS_Font font(standardFontPath());
    REQUIRE(font.loadFont());

    // codepoint, name - kept explicit rather than looping over a QChar
    // literal array so a failure names the actual missing character.
    struct Case { char16_t codepoint; const char *name; };
    const Case cases[] = {
        {0x011E, "G with breve (Ğ)"},
        {0x011F, "g with breve (ğ)"},
        {0x0130, "I with dot above (İ)"},
        {0x0131, "dotless i (ı)"},
        {0x015E, "S with cedilla (Ş)"},
        {0x015F, "s with cedilla (ş)"},
    };

    for (const auto &c : cases) {
        INFO(c.name);
        RS_Block *letter = font.findLetter(QString(QChar(c.codepoint)));
        REQUIRE(letter != nullptr);
        CHECK_FALSE(letter->isEmpty());
    }
}

TEST_CASE("standard.lff still falls back to the replacement character for a "
          "genuinely undefined codepoint",
          "[font][i18n][issue2758]") {
    ensureApp();

    RS_Font font(standardFontPath());
    REQUIRE(font.loadFont());

    // U+E000 is in the Private Use Area: guaranteed absent from every real
    // font. This is the behaviour the six characters above used to trigger.
    RS_Block *letter = font.findLetter(QString(QChar(char16_t{0xE000})));
    CHECK(letter == nullptr);
}
