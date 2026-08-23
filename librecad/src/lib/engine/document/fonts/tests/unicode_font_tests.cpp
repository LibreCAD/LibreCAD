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
 * Coverage tests for unicode.lff.
 *
 * unicode.lff is the font offering the widest script coverage LibreCAD
 * ships, yet it was missing every accented Latin letter needed by Turkish,
 * Romanian, Latvian, Hungarian and Vietnamese, and the Macedonian/Serbian
 * Cyrillic letters. A missing glyph is not loud: RS_Text/RS_MText silently
 * substitute U+FFFD, so the text simply renders as replacement diamonds.
 *
 * These tests assert against the real installed font that every codepoint
 * actually used by the shipped translations resolves to a non-empty glyph,
 * and that each one stays inside a sane drawing envelope - a composite whose
 * C<hex> reference is wrong, or a diacritic placed at the wrong height,
 * shows up here as an out-of-range bounding box rather than as a surprise
 * in the drawing.
 */

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QString>

#include "rs_font.h"
#include "rs_block.h"
#include "rs_settings.h"
#include "rs_vector.h"

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

RS_Font &unicodeFont() {
    static RS_Font font(QString(LIBRECAD_SOURCE_DIR) +
                        "/librecad/support/fonts/unicode.lff");
    static bool loaded = font.loadFont();
    REQUIRE(loaded);
    return font;
}

// Latin letters required by the shipped tr/ro/lv/hu/cs/sk/pl/vi translations.
const char16_t kLatin[] = {
    0x0101, 0x0103, 0x0111, 0x0112, 0x0113, 0x0117, 0x011E, 0x011F, 0x0122,
    0x0123, 0x012A, 0x012B, 0x0130, 0x0131, 0x0137, 0x013A, 0x013C, 0x013D,
    0x013E, 0x0146, 0x0150, 0x0151, 0x0155, 0x015E, 0x015F, 0x0163, 0x016A,
    0x016B, 0x0170, 0x0171, 0x01A1, 0x01B0, 0x0218, 0x0219, 0x021A, 0x021B,
    0x1EA1, 0x1EA2, 0x1EA5, 0x1EAD, 0x1EBF, 0x1EC3, 0x1ECF, 0x1ED1, 0x1ED5,
    0x1ED9, 0x1EDF, 0x1EE3, 0x1EE5, 0x1EEB, 0x1EED, 0x1EEF, 0x1EF1, 0};

// Macedonian / Serbian Cyrillic.
const char16_t kCyrillic[] = {
    0x0405, 0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x040F, 0x0452, 0x0453,
    0x0455, 0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x045F, 0};

// Georgian (Mkhedruli), the modern 33-letter unicameral alphabet.
const char16_t kGeorgian[] = {
    0x10D0, 0x10D1, 0x10D2, 0x10D3, 0x10D4, 0x10D5, 0x10D6, 0x10D7, 0x10D8,
    0x10D9, 0x10DA, 0x10DB, 0x10DC, 0x10DD, 0x10DE, 0x10DF, 0x10E0, 0x10E1,
    0x10E2, 0x10E3, 0x10E4, 0x10E5, 0x10E6, 0x10E7, 0x10E8, 0x10E9, 0x10EA,
    0x10EB, 0x10EC, 0x10ED, 0x10EE, 0x10EF, 0x10F0, 0};

// Thai: the 44 modern consonants (all of U+0E01-U+0E2E except the obsolete
// KHO KHUAT/KHO KHON), the Lo/Lm vowels and marks that place correctly with
// plain sequential glyph advance (no shaping engine involved), and the Mn
// combining marks (tone marks, sara i/ii/ue/uee/u/uu, phinthu). The Mn
// marks are drawn at their real vertical offset but will render at the
// wrong *horizontal* position - after the base letter's advance width
// rather than stacked on top of it - until RS_Text/RS_MText gain zero-
// advance combining-mark placement; see the PR description.
const char16_t kThai[] = {
    0x0E01, 0x0E02, 0x0E04, 0x0E06, 0x0E07, 0x0E08, 0x0E09, 0x0E0A, 0x0E0B,
    0x0E0C, 0x0E0D, 0x0E0E, 0x0E0F, 0x0E10, 0x0E11, 0x0E12, 0x0E13, 0x0E14,
    0x0E15, 0x0E16, 0x0E17, 0x0E18, 0x0E19, 0x0E1A, 0x0E1B, 0x0E1C, 0x0E1D,
    0x0E1E, 0x0E1F, 0x0E20, 0x0E21, 0x0E22, 0x0E23, 0x0E24, 0x0E25, 0x0E26,
    0x0E27, 0x0E28, 0x0E29, 0x0E2A, 0x0E2B, 0x0E2C, 0x0E2D, 0x0E2E, 0x0E2F,
    0x0E30, 0x0E32, 0x0E33, 0x0E40, 0x0E41, 0x0E42, 0x0E43, 0x0E44, 0x0E45,
    0x0E46, 0x0E31, 0x0E34, 0x0E35, 0x0E36, 0x0E37, 0x0E38, 0x0E39, 0x0E3A,
    0x0E47, 0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D, 0x0E4E, 0};

void checkRange(const char16_t *codepoints, double minX = -2.5, double maxX = 9.0,
                 double minY = -4.5, double maxY = 15.0, double minMaxY = 1.0) {
    RS_Font &font = unicodeFont();
    for (const char16_t *cp = codepoints; *cp != 0; ++cp) {
        INFO("U+" << QString::number(*cp, 16).toUpper().toStdString());
        RS_Block *letter = font.findLetter(QString(QChar(*cp)));
        REQUIRE(letter != nullptr);
        REQUIRE_FALSE(letter->isEmpty());

        // Envelope check. The font draws on a 0..9 body with descenders to
        // -3 and stacked diacritics reaching ~13; anything outside that means
        // a bad composite reference or a misplaced mark.
        letter->calculateBorders();
        const RS_Vector min = letter->getMin();
        const RS_Vector max = letter->getMax();
        CHECK(max.y > minMaxY);
        CHECK(max.y <= maxY);
        CHECK(min.y >= minY);
        CHECK(max.x <= maxX);
        CHECK(min.x >= minX);
    }
}

} // namespace

TEST_CASE("unicode.lff covers the accented Latin letters the translations use",
          "[font][unicode][i18n]") {
    ensureApp();
    checkRange(kLatin);
}

TEST_CASE("unicode.lff covers Macedonian and Serbian Cyrillic",
          "[font][unicode][i18n]") {
    ensureApp();
    checkRange(kCyrillic);
}

TEST_CASE("unicode.lff covers the modern Georgian (Mkhedruli) alphabet",
          "[font][unicode][i18n]") {
    ensureApp();
    checkRange(kGeorgian);
}

TEST_CASE("unicode.lff covers Thai consonants, vowels and combining marks",
          "[font][unicode][i18n]") {
    ensureApp();
    // Thai letters run wider than Latin/Cyrillic/Georgian (some consonants
    // carry a second stroke reaching past x=9), the Mn combining marks are
    // drawn off to the left of their base letter's own body (see the kThai
    // comment), and three marks (sara u, sara uu, phinthu) sit entirely
    // below the baseline with nothing above y=1, so this needs a wider
    // envelope than checkRange's default.
    checkRange(kThai, /*minX=*/-3.5, /*maxX=*/11.0, /*minY=*/-4.5, /*maxY=*/15.0,
               /*minMaxY=*/-3.5);
}

TEST_CASE("unicode.lff keeps its base letters intact", "[font][unicode]") {
    ensureApp();
    RS_Font &font = unicodeFont();
    // The new glyphs are composites (C<hex>) over these bases; if a base were
    // damaged the accented forms would silently degrade with it.
    for (char16_t cp : {u'A', u'E', u'G', u'I', u'O', u'S', u'U',
                        u'a', u'e', u'g', u'o', u's', u'u'}) {
        INFO("base " << static_cast<char>(cp));
        RS_Block *letter = font.findLetter(QString(QChar(cp)));
        REQUIRE(letter != nullptr);
        letter->calculateBorders();
        CHECK(letter->getMax().y > 4.0);
    }
}
