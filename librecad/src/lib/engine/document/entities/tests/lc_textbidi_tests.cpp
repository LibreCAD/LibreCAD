/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li github.com/dxli
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

#include <catch2/catch_test_macros.hpp>

#include <QChar>
#include <QString>

#include "lc_textbidi.h"

using lc::textbidi::mirrorByLine;

// Pure ASCII reverses end-to-start.
TEST_CASE("mirrorByLine reverses pure ASCII", "[textbidi]") {
    REQUIRE(mirrorByLine(QString("abc")) == QString("cba"));
    REQUIRE(mirrorByLine(QString("1234")) == QString("4321"));
    REQUIRE(mirrorByLine(QString("a")) == QString("a"));
    REQUIRE(mirrorByLine(QString()) == QString());
}

// Empty and whitespace edge cases.
TEST_CASE("mirrorByLine handles empty and whitespace", "[textbidi]") {
    REQUIRE(mirrorByLine(QString("")) == QString(""));
    REQUIRE(mirrorByLine(QString("   ")) == QString("   "));
    REQUIRE(mirrorByLine(QString(" a ")) == QString(" a "));
}

// Multi-line input mirrors each line independently; line order is preserved.
TEST_CASE("mirrorByLine mirrors per line", "[textbidi]") {
    REQUIRE(mirrorByLine(QString("abc\n123")) == QString("cba\n321"));
    REQUIRE(mirrorByLine(QString("ab\ncd\nef"))
            == QString("ba\ndc\nfe"));
    // Empty line in the middle remains empty.
    REQUIRE(mirrorByLine(QString("abc\n\nxyz")) == QString("cba\n\nzyx"));
    // Leading/trailing newlines.
    REQUIRE(mirrorByLine(QString("\nabc\n")) == QString("\ncba\n"));
}

// Surrogate pairs (non-BMP) must be kept together: a high+low pair in the
// input becomes a high+low pair (in original surrogate order) at the new
// position in the output.
TEST_CASE("mirrorByLine keeps surrogate pairs together", "[textbidi]") {
    // U+1F600 GRINNING FACE = D83D DE00 (high low).
    QString grin;
    grin.append(QChar(0xD83D));
    grin.append(QChar(0xDE00));

    QString abcGrin = QString("abc") + grin;
    QString grinCba = grin + QString("cba");
    REQUIRE(mirrorByLine(abcGrin) == grinCba);

    QString twoEmoji = grin + grin;
    REQUIRE(mirrorByLine(twoEmoji) == twoEmoji);

    // Mixed BMP and non-BMP, multi-line.
    QString line1 = QString("a") + grin + QString("b");
    QString line2 = QString("12");
    QString in = line1 + QString("\n") + line2;
    QString out = QString("b") + grin + QString("a") + QString("\n21");
    REQUIRE(mirrorByLine(in) == out);
}

// Involution: applying mirror twice returns the original input.
TEST_CASE("mirrorByLine is involutive", "[textbidi]") {
    const QString cases[] = {
        QString(),
        QString("abc"),
        QString("a"),
        QString("1234567890"),
        QString("hello\nworld"),
        QString("\n\nabc\n\n"),
        QString::fromUtf8(u8"שלום"),
        QString::fromUtf8(u8"abc שלום 123"),
    };
    for (const QString& s : cases) {
        REQUIRE(mirrorByLine(mirrorByLine(s)) == s);
    }
}

// Hebrew strings reverse just like other code points — UAX#9 is not
// applied here; this is positional reversal. Callers that want UAX#9 use
// RS_MText::computeBidiVisualOrder instead.
TEST_CASE("mirrorByLine reverses Hebrew positionally", "[textbidi]") {
    const QString shalom = QString::fromUtf8(u8"שלום");
    const QString shalomReversed = QString::fromUtf8(u8"םולש");
    REQUIRE(mirrorByLine(shalom) == shalomReversed);
}

// Stray-low or stray-high surrogate (malformed input): treated as a single
// QChar, i.e. just placed in the new position. Not a graceful fix, but at
// least the function does not crash or truncate.
TEST_CASE("mirrorByLine tolerates lone surrogates", "[textbidi]") {
    QString lonely;
    lonely.append(QChar('a'));
    lonely.append(QChar(0xD83D));  // lone high surrogate
    lonely.append(QChar('b'));

    const QString result = mirrorByLine(lonely);
    REQUIRE(result.size() == lonely.size());
    REQUIRE(result.at(0) == QChar('b'));
    REQUIRE(result.at(2) == QChar('a'));
    // Round-trip still holds.
    REQUIRE(mirrorByLine(result) == lonely);
}
