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

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QChar>
#include <QGuiApplication>
#include <QString>
#include <Qt>

#include "rs_mtext.h"

namespace {

// QTextLayout (used inside computeBidiVisualOrder) reaches into the Qt font
// database, which requires a live QGuiApplication. Catch2's default main()
// does not construct one, so we spin up a minimal offscreen instance via a
// static fixture before any test runs. Process-singleton, intentionally never
// destroyed.
struct QtAppBootstrap {
    QtAppBootstrap() {
        if (QGuiApplication::instance() != nullptr) return;
        qputenv("QT_QPA_PLATFORM", "offscreen");
        static int argc = 1;
        static char arg0[] = "librecad_tests";
        static char *argv[] = {arg0, nullptr};
        new QGuiApplication(argc, argv);
    }
};
static QtAppBootstrap s_qtAppBootstrap;

/** Helper: invoke the bidi pass and return result. */
std::vector<int> visualOrder(const QString &s,
                              Qt::LayoutDirection base = Qt::LeftToRight) {
    return RS_MText::computeBidiVisualOrder(s, base);
}

/**
 * Returns true if @p result is a permutation of [0, n).
 */
bool isPermutationOf(const std::vector<int> &result, int n) {
    if (static_cast<int>(result.size()) != n) return false;
    std::vector<bool> seen(n, false);
    for (int i : result) {
        if (i < 0 || i >= n || seen[i]) return false;
        seen[i] = true;
    }
    return true;
}

/**
 * Find the visual position (index into @p visual) where logical index @p logIdx
 * appears. Returns -1 if missing.
 */
int posOf(const std::vector<int> &visual, int logIdx) {
    for (size_t k = 0; k < visual.size(); ++k) {
        if (visual[k] == logIdx) return static_cast<int>(k);
    }
    return -1;
}

}  // namespace

TEST_CASE("computeBidiVisualOrder: empty input", "[mtext][bidi]") {
    auto v = visualOrder(QString());
    REQUIRE(v.empty());
}

TEST_CASE("computeBidiVisualOrder: pure ASCII is identity", "[mtext][bidi]") {
    QString s = QStringLiteral("Hello world");
    auto v = visualOrder(s);
    REQUIRE(isPermutationOf(v, s.size()));
    // Latin LTR text: visual order matches logical order one-to-one.
    for (int i = 0; i < s.size(); ++i) {
        REQUIRE(v[i] == i);
    }
}

TEST_CASE("computeBidiVisualOrder: pure Hebrew reverses", "[mtext][bidi]") {
    // "שלום" = Shin (logical 0), Lamed (1), Vav (2), Final-Mem (3).
    // Visually rendered (reading right-to-left), the rightmost glyph is the
    // first logical character. In a left-to-right coordinate system, the
    // visual order array should therefore have logical index 3 first
    // (leftmost), then 2, 1, 0 (rightmost).
    const QString s = QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
    REQUIRE(s.size() == 4);
    auto v = visualOrder(s);
    REQUIRE(isPermutationOf(v, s.size()));
    // The logical character that ends up rightmost (visual position N-1) must
    // be logical index 0 (the first Hebrew letter, ש).
    REQUIRE(v.back() == 0);
    // And the leftmost visual must be the last logical character (final mem).
    REQUIRE(v.front() == 3);
}

TEST_CASE("computeBidiVisualOrder: mixed LTR with embedded Hebrew",
          "[mtext][bidi]") {
    // "A שלום B" — logical layout:
    //   0='A', 1=' ', 2='ש', 3='ל', 4='ו', 5='ם', 6=' ', 7='B'
    // Expected visual order with LTR base: A, ' ', (Hebrew reversed: ם,ו,ל,ש),
    // ' ', B.
    QString s = QStringLiteral("A ");
    s += QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
    s += QStringLiteral(" B");
    REQUIRE(s.size() == 8);

    auto v = visualOrder(s, Qt::LeftToRight);
    REQUIRE(isPermutationOf(v, s.size()));

    // 'A' is the first character visually.
    REQUIRE(v.front() == 0);
    // 'B' is the last character visually.
    REQUIRE(v.back() == 7);

    // Within the Hebrew run, the logical-3 char (ל, the second Hebrew letter)
    // appears before logical-2 char (ש, the first) in visual order — i.e. ש
    // ends up rightmost of the Hebrew run. We assert relative ordering: the
    // higher logical index comes earlier in visual order.
    REQUIRE(posOf(v, 5) < posOf(v, 4));  // ם before ו visually
    REQUIRE(posOf(v, 4) < posOf(v, 3));  // ו before ל visually
    REQUIRE(posOf(v, 3) < posOf(v, 2));  // ל before ש visually
}

TEST_CASE("computeBidiVisualOrder: digits inside Hebrew stay LTR (UAX#9 W2)",
          "[mtext][bidi]") {
    // "שלום 123 שלום" — the 123 is a numeric run inside RTL context. Per
    // UAX#9 rules W2/W4 (and L2 reordering), the digits should themselves
    // remain in left-to-right order ("1" left of "2" left of "3"), even
    // though the surrounding Hebrew is RTL.
    QString s = QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
    s += QStringLiteral(" 123 ");
    s += QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");

    auto v = visualOrder(s, Qt::RightToLeft);
    REQUIRE(isPermutationOf(v, s.size()));

    // Find the visual positions of '1', '2', '3' — they are at logical
    // indices 5, 6, 7 respectively.
    int p1 = posOf(v, 5);
    int p2 = posOf(v, 6);
    int p3 = posOf(v, 7);
    REQUIRE(p1 >= 0);
    REQUIRE(p2 >= 0);
    REQUIRE(p3 >= 0);
    // Digits visually L→R: '1' to the left of '2' to the left of '3'.
    REQUIRE(p1 < p2);
    REQUIRE(p2 < p3);
}

TEST_CASE("computeBidiVisualOrder: object replacement char reorders neutrally",
          "[mtext][bidi]") {
    // U+FFFC is the placeholder we use for super-/subscript stacks. It is
    // bidi class ON (Other Neutral) and must reorder consistently with
    // surrounding context.
    QString s;
    s += QChar('A');
    s += QChar(0xFFFC);  // stack placeholder
    s += QChar('B');
    auto v = visualOrder(s, Qt::LeftToRight);
    REQUIRE(isPermutationOf(v, s.size()));
    // In an all-LTR context, order is preserved.
    REQUIRE(v[0] == 0);
    REQUIRE(v[1] == 1);
    REQUIRE(v[2] == 2);
}

TEST_CASE("computeBidiVisualOrder: result is always a permutation",
          "[mtext][bidi]") {
    // Stress: a few mixed strings — the result must always be a permutation
    // of [0, length) regardless of base direction.
    const QString cases[] = {
        QStringLiteral(""),
        QStringLiteral("a"),
        QStringLiteral("abc"),
        QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D"),
        QString::fromUtf8("hello \xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D 42 world"),
        QStringLiteral("12345"),
    };
    for (const QString &s : cases) {
        for (Qt::LayoutDirection dir :
             {Qt::LeftToRight, Qt::RightToLeft}) {
            auto v = visualOrder(s, dir);
            REQUIRE(isPermutationOf(v, s.size()));
        }
    }
}
