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

// Armenian: all 39 uppercase + 39 lowercase letters (including the rare
// TURNED AYB and YI WITH STROKE), the modifier letter left half ring, and
// the ECH YIWN ligature. Bicameral, no joining/conjuncts/reordering, so
// (like Georgian) it is fully correct with glyphs alone - no positioning
// caveat.
const char16_t kArmenian[] = {
    0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539,
    0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F, 0x0540, 0x0541, 0x0542,
    0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B,
    0x054C, 0x054D, 0x054E, 0x054F, 0x0550, 0x0551, 0x0552, 0x0553, 0x0554,
    0x0555, 0x0556, 0x0559, 0x0560, 0x0561, 0x0562, 0x0563, 0x0564, 0x0565,
    0x0566, 0x0567, 0x0568, 0x0569, 0x056A, 0x056B, 0x056C, 0x056D, 0x056E,
    0x056F, 0x0570, 0x0571, 0x0572, 0x0573, 0x0574, 0x0575, 0x0576, 0x0577,
    0x0578, 0x0579, 0x057A, 0x057B, 0x057C, 0x057D, 0x057E, 0x057F, 0x0580,
    0x0581, 0x0582, 0x0583, 0x0584, 0x0585, 0x0586, 0x0587, 0x0588, 0};

// Hebrew: the 22 base letters, the 5 final forms (kaf/mem/nun/pe/tsadi),
// and the 3 Yiddish digraph ligatures (double vav, vav-yod, double yod).
// No niqqud/cantillation marks - those are optional in ordinary Hebrew
// text (most Hebrew, outside children's books/poetry/liturgy, omits them
// entirely) and would need multi-mark stacking on a single base letter,
// a harder positioning problem than Thai/Lao's single-mark case, so left
// out of scope here. Hebrew is RTL, but that is a layout-direction
// concern already handled by RS_Text/RS_MText's bidi reordering
// (RS_MText::computeBidiVisualOrder(), tested against real Hebrew
// strings in rs_mtext_bidi_tests.cpp) - unlike Thai/Lao's combining
// marks, these glyphs have no positioning caveat of their own.
const char16_t kHebrew[] = {
    0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8,
    0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF, 0x05E0, 0x05E1,
    0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA,
    0x05F0, 0x05F1, 0x05F2, 0};

// Ethiopic (Ge'ez script - Amharic, Tigrinya and others): all 326 Lo
// codepoints in the core block. An abugida, but every consonant+vowel
// combination is its own precomposed codepoint (unlike Devanagari, there
// is no runtime conjunct formation or reordering needed), so like
// Georgian/Armenian/Hebrew/Cherokee/Tifinagh it is correct with glyphs
// alone. The block has real internal gaps (not every consonant has all
// 7-8 vowel-order forms defined) - this list follows them exactly rather
// than a naive contiguous range, since tracing an unassigned codepoint
// silently returns the reference font's .notdef box glyph instead of
// erroring (see the Cherokee commit for how that was first caught).
const char16_t kEthiopic[] = {
    0x1200, 0x1201, 0x1202, 0x1203, 0x1204, 0x1205, 0x1206, 0x1207, 0x1208,
    0x1209, 0x120A, 0x120B, 0x120C, 0x120D, 0x120E, 0x120F, 0x1210, 0x1211,
    0x1212, 0x1213, 0x1214, 0x1215, 0x1216, 0x1217, 0x1218, 0x1219, 0x121A,
    0x121B, 0x121C, 0x121D, 0x121E, 0x121F, 0x1220, 0x1221, 0x1222, 0x1223,
    0x1224, 0x1225, 0x1226, 0x1227, 0x1228, 0x1229, 0x122A, 0x122B, 0x122C,
    0x122D, 0x122E, 0x122F, 0x1230, 0x1231, 0x1232, 0x1233, 0x1234, 0x1235,
    0x1236, 0x1237, 0x1238, 0x1239, 0x123A, 0x123B, 0x123C, 0x123D, 0x123E,
    0x123F, 0x1240, 0x1241, 0x1242, 0x1243, 0x1244, 0x1245, 0x1246, 0x1247,
    0x1248, 0x124A, 0x124B, 0x124C, 0x124D, 0x1250, 0x1251, 0x1252, 0x1253,
    0x1254, 0x1255, 0x1256, 0x1258, 0x125A, 0x125B, 0x125C, 0x125D, 0x1260,
    0x1261, 0x1262, 0x1263, 0x1264, 0x1265, 0x1266, 0x1267, 0x1268, 0x1269,
    0x126A, 0x126B, 0x126C, 0x126D, 0x126E, 0x126F, 0x1270, 0x1271, 0x1272,
    0x1273, 0x1274, 0x1275, 0x1276, 0x1277, 0x1278, 0x1279, 0x127A, 0x127B,
    0x127C, 0x127D, 0x127E, 0x127F, 0x1280, 0x1281, 0x1282, 0x1283, 0x1284,
    0x1285, 0x1286, 0x1287, 0x1288, 0x128A, 0x128B, 0x128C, 0x128D, 0x1290,
    0x1291, 0x1292, 0x1293, 0x1294, 0x1295, 0x1296, 0x1297, 0x1298, 0x1299,
    0x129A, 0x129B, 0x129C, 0x129D, 0x129E, 0x129F, 0x12A0, 0x12A1, 0x12A2,
    0x12A3, 0x12A4, 0x12A5, 0x12A6, 0x12A7, 0x12A8, 0x12A9, 0x12AA, 0x12AB,
    0x12AC, 0x12AD, 0x12AE, 0x12AF, 0x12B0, 0x12B2, 0x12B3, 0x12B4, 0x12B5,
    0x12B8, 0x12B9, 0x12BA, 0x12BB, 0x12BC, 0x12BD, 0x12BE, 0x12C0, 0x12C2,
    0x12C3, 0x12C4, 0x12C5, 0x12C8, 0x12C9, 0x12CA, 0x12CB, 0x12CC, 0x12CD,
    0x12CE, 0x12CF, 0x12D0, 0x12D1, 0x12D2, 0x12D3, 0x12D4, 0x12D5, 0x12D6,
    0x12D8, 0x12D9, 0x12DA, 0x12DB, 0x12DC, 0x12DD, 0x12DE, 0x12DF, 0x12E0,
    0x12E1, 0x12E2, 0x12E3, 0x12E4, 0x12E5, 0x12E6, 0x12E7, 0x12E8, 0x12E9,
    0x12EA, 0x12EB, 0x12EC, 0x12ED, 0x12EE, 0x12EF, 0x12F0, 0x12F1, 0x12F2,
    0x12F3, 0x12F4, 0x12F5, 0x12F6, 0x12F7, 0x12F8, 0x12F9, 0x12FA, 0x12FB,
    0x12FC, 0x12FD, 0x12FE, 0x12FF, 0x1300, 0x1301, 0x1302, 0x1303, 0x1304,
    0x1305, 0x1306, 0x1307, 0x1308, 0x1309, 0x130A, 0x130B, 0x130C, 0x130D,
    0x130E, 0x130F, 0x1310, 0x1312, 0x1313, 0x1314, 0x1315, 0x1318, 0x1319,
    0x131A, 0x131B, 0x131C, 0x131D, 0x131E, 0x131F, 0x1320, 0x1321, 0x1322,
    0x1323, 0x1324, 0x1325, 0x1326, 0x1327, 0x1328, 0x1329, 0x132A, 0x132B,
    0x132C, 0x132D, 0x132E, 0x132F, 0x1330, 0x1331, 0x1332, 0x1333, 0x1334,
    0x1335, 0x1336, 0x1337, 0x1338, 0x1339, 0x133A, 0x133B, 0x133C, 0x133D,
    0x133E, 0x133F, 0x1340, 0x1341, 0x1342, 0x1343, 0x1344, 0x1345, 0x1346,
    0x1347, 0x1348, 0x1349, 0x134A, 0x134B, 0x134C, 0x134D, 0x134E, 0x134F,
    0x1350, 0x1351, 0x1352, 0x1353, 0x1354, 0x1355, 0x1356, 0x1357, 0x1358,
    0x1359, 0x135A, 0};

// Ol Chiki: all 36 letters of the Santali alphabet (~7.6 million speakers,
// an official regional language of India). A true alphabet with explicit
// vowels and no combining marks - correct with glyphs alone. Three of the
// codepoints (MU/GAAHLAA/MU-GAAHLAA TTUDDAG, gemination marks) are small
// dots whose medial-axis skeleton collapses to 1-3 pixels, same as Thai's
// phinthu and Lao's Pali virama - hand-authored as small circles at the
// position the reference font reports rather than traced.
const char16_t kOlChiki[] = {
    0x1C5A, 0x1C5B, 0x1C5C, 0x1C5D, 0x1C5E, 0x1C5F, 0x1C60, 0x1C61, 0x1C62,
    0x1C63, 0x1C64, 0x1C65, 0x1C66, 0x1C67, 0x1C68, 0x1C69, 0x1C6A, 0x1C6B,
    0x1C6C, 0x1C6D, 0x1C6E, 0x1C6F, 0x1C70, 0x1C71, 0x1C72, 0x1C73, 0x1C74,
    0x1C75, 0x1C76, 0x1C77, 0x1C78, 0x1C79, 0x1C7A, 0x1C7B, 0x1C7C, 0x1C7D, 0};

// Cherokee: all 92 assigned codepoints in the main syllabary block -
// the 85 original syllables Sequoyah devised plus the 6 modern
// (YE/YI/YO/YU/YV/MV) syllables and their 6 lowercase forms added in
// Unicode 8.0/12.0 for bicameral digital text (U+13F6/U+13F7 are
// unassigned gaps in the block and are skipped). A syllabary, not an
// alphabet, so like Georgian/Armenian/Hebrew there is no
// joining/conjunct/reordering concern - correct today, no caveats.
const char16_t kCherokee[] = {
    0x13A0, 0x13A1, 0x13A2, 0x13A3, 0x13A4, 0x13A5, 0x13A6, 0x13A7, 0x13A8,
    0x13A9, 0x13AA, 0x13AB, 0x13AC, 0x13AD, 0x13AE, 0x13AF, 0x13B0, 0x13B1,
    0x13B2, 0x13B3, 0x13B4, 0x13B5, 0x13B6, 0x13B7, 0x13B8, 0x13B9, 0x13BA,
    0x13BB, 0x13BC, 0x13BD, 0x13BE, 0x13BF, 0x13C0, 0x13C1, 0x13C2, 0x13C3,
    0x13C4, 0x13C5, 0x13C6, 0x13C7, 0x13C8, 0x13C9, 0x13CA, 0x13CB, 0x13CC,
    0x13CD, 0x13CE, 0x13CF, 0x13D0, 0x13D1, 0x13D2, 0x13D3, 0x13D4, 0x13D5,
    0x13D6, 0x13D7, 0x13D8, 0x13D9, 0x13DA, 0x13DB, 0x13DC, 0x13DD, 0x13DE,
    0x13DF, 0x13E0, 0x13E1, 0x13E2, 0x13E3, 0x13E4, 0x13E5, 0x13E6, 0x13E7,
    0x13E8, 0x13E9, 0x13EA, 0x13EB, 0x13EC, 0x13ED, 0x13EE, 0x13EF, 0x13F0,
    0x13F1, 0x13F2, 0x13F3, 0x13F4, 0x13F5, 0x13F8, 0x13F9, 0x13FA, 0x13FB,
    0x13FC, 0x13FD, 0};

// Tifinagh (Berber/Amazigh): all 56 base letters (the many regional
// variants - Berber Academy, Tuareg, Ahaggar, Ayer, Tawellemet - included,
// same completeness policy as the other scripts here) plus the
// labialization modifier letter. An alphabet with no joining, conjuncts
// or reordering - correct with glyphs alone. Several letters are pure dot
// patterns that differ only by dot count/arrangement (a real Tifinagh
// convention, not a tracing artifact - verified pairwise, see the commit).
const char16_t kTifinagh[] = {
    0x2D30, 0x2D31, 0x2D32, 0x2D33, 0x2D34, 0x2D35, 0x2D36, 0x2D37, 0x2D38,
    0x2D39, 0x2D3A, 0x2D3B, 0x2D3C, 0x2D3D, 0x2D3E, 0x2D3F, 0x2D40, 0x2D41,
    0x2D42, 0x2D43, 0x2D44, 0x2D45, 0x2D46, 0x2D47, 0x2D48, 0x2D49, 0x2D4A,
    0x2D4B, 0x2D4C, 0x2D4D, 0x2D4E, 0x2D4F, 0x2D50, 0x2D51, 0x2D52, 0x2D53,
    0x2D54, 0x2D55, 0x2D56, 0x2D57, 0x2D58, 0x2D59, 0x2D5A, 0x2D5B, 0x2D5C,
    0x2D5D, 0x2D5E, 0x2D5F, 0x2D60, 0x2D61, 0x2D62, 0x2D63, 0x2D64, 0x2D65,
    0x2D66, 0x2D67, 0x2D6F, 0};

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

// Lao: every Lo/Lm base letter (modern consonants, the Pali/Sanskrit
// transliteration consonants, the Lo/Lm vowels/marks that place with plain
// sequential advance, and the Khmu-language extension letters), plus the
// 17 Mn combining marks (tone marks, vowel signs I/II/Y/YY/U/UU, semivowel
// sign LO, cancellation mark, niggahita, yamakkan, Pali virama). Same
// horizontal-positioning caveat as kThai applies to the Mn marks here.
const char16_t kLao[] = {
    0x0E81, 0x0E82, 0x0E84, 0x0E86, 0x0E87, 0x0E88, 0x0E89, 0x0E8A, 0x0E8C,
    0x0E8D, 0x0E8E, 0x0E8F, 0x0E90, 0x0E91, 0x0E92, 0x0E93, 0x0E94, 0x0E95,
    0x0E96, 0x0E97, 0x0E98, 0x0E99, 0x0E9A, 0x0E9B, 0x0E9C, 0x0E9D, 0x0E9E,
    0x0E9F, 0x0EA0, 0x0EA1, 0x0EA2, 0x0EA3, 0x0EA5, 0x0EA7, 0x0EA8, 0x0EA9,
    0x0EAA, 0x0EAB, 0x0EAC, 0x0EAD, 0x0EAE, 0x0EAF, 0x0EB0, 0x0EB2, 0x0EB3,
    0x0EBD, 0x0EC0, 0x0EC1, 0x0EC2, 0x0EC3, 0x0EC4, 0x0EC6, 0x0EDC, 0x0EDD,
    0x0EDE, 0x0EDF, 0x0EB1, 0x0EB4, 0x0EB5, 0x0EB6, 0x0EB7, 0x0EB8, 0x0EB9,
    0x0EBA, 0x0EBB, 0x0EBC, 0x0EC8, 0x0EC9, 0x0ECA, 0x0ECB, 0x0ECC, 0x0ECD,
    0x0ECE, 0};

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

TEST_CASE("unicode.lff covers the Armenian alphabet", "[font][unicode][i18n]") {
    ensureApp();
    // A couple of Armenian letters run slightly wider than the default
    // envelope (e.g. Ա's crossbar reaches past x=9).
    checkRange(kArmenian, /*minX=*/-2.5, /*maxX=*/10.5);
}

TEST_CASE("unicode.lff covers Hebrew base letters, final forms and Yiddish "
          "ligatures",
          "[font][unicode][i18n]") {
    ensureApp();
    checkRange(kHebrew);
}

TEST_CASE("unicode.lff covers the Ethiopic (Ge'ez) syllabary",
          "[font][unicode][i18n]") {
    ensureApp();
    // A few Ethiopic letters carry two or three vertical strokes side by
    // side and run wider than the default envelope.
    checkRange(kEthiopic, /*minX=*/-2.5, /*maxX=*/16.0, /*minY=*/-4.5,
               /*maxY=*/15.0, /*minMaxY=*/1.0);
}

TEST_CASE("unicode.lff covers the Ol Chiki (Santali) alphabet",
          "[font][unicode][i18n]") {
    ensureApp();
    checkRange(kOlChiki);
}

TEST_CASE("unicode.lff covers the modern Georgian (Mkhedruli) alphabet",
          "[font][unicode][i18n]") {
    ensureApp();
    checkRange(kGeorgian);
}

TEST_CASE("unicode.lff covers the Cherokee syllabary", "[font][unicode][i18n]") {
    ensureApp();
    // Several Cherokee "W"-shaped syllables run much wider than any other
    // script covered so far (up to x=15.6).
    checkRange(kCherokee, /*minX=*/-2.5, /*maxX=*/16.0);
}

TEST_CASE("unicode.lff covers the Tifinagh (Berber/Amazigh) alphabet",
          "[font][unicode][i18n]") {
    ensureApp();
    checkRange(kTifinagh);
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

TEST_CASE("unicode.lff covers Lao base letters and combining marks",
          "[font][unicode][i18n]") {
    ensureApp();
    // Same shape as the Thai envelope: some Lao consonants carry a second
    // stroke past x=9, the Mn marks are drawn off to the left of their own
    // recentered origin, and four marks (sara u, sara uu, Pali virama,
    // semivowel sign LO) sit entirely below the baseline.
    checkRange(kLao, /*minX=*/-3.5, /*maxX=*/11.5, /*minY=*/-4.5, /*maxY=*/15.0,
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
