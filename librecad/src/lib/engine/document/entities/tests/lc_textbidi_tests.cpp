/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

#include <algorithm>
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
  REQUIRE(mirrorByLine(QString("ab\ncd\nef")) == QString("ba\ndc\nfe"));
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
  for (const QString &s : cases) {
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
  lonely.append(QChar(0xD83D)); // lone high surrogate
  lonely.append(QChar('b'));

  const QString result = mirrorByLine(lonely);
  REQUIRE(result.size() == lonely.size());
  REQUIRE(result.at(0) == QChar('b'));
  REQUIRE(result.at(2) == QChar('a'));
  // Round-trip still holds.
  REQUIRE(mirrorByLine(result) == lonely);
}

namespace {
QString visualText(const QString &text, Qt::LayoutDirection direction) {
  QString visual;
  for (const auto &cluster : lc::textbidi::visualClusters(text, direction)) {
    for (int i = cluster.start; i < cluster.start + cluster.length; ++i) {
      const auto ch = text.at(i);
      if (ch.category() != QChar::Other_Format) {
        visual += cluster.rightToLeft ? ch.mirroredChar() : ch;
      }
    }
  }
  return visual;
}
} // namespace

TEST_CASE("Bidi ordering is font-independent and preserves numeric runs",
          "[textbidi][issue1859]") {
  const auto hebrew = QString::fromUtf8(u8"שלום");
  const auto reversed = mirrorByLine(hebrew);
  for (const auto direction : {Qt::RightToLeft, Qt::LayoutDirectionAuto}) {
    for (const QString &number :
         {QStringLiteral("123"), QStringLiteral("12.5"),
          QStringLiteral("1,234.56"), QStringLiteral("12/34"),
          QStringLiteral("(123)"), QStringLiteral("Main 123")}) {
      CHECK(visualText(hebrew + " " + number, direction) ==
            number + " " + reversed);
    }
    CHECK(visualText(hebrew + " " + QChar(0x2066) + "Main 12.5" + QChar(0x2069),
                     direction) == "Main 12.5 " + reversed);
  }
  CHECK(visualText("Main 123", Qt::RightToLeft) == "Main 123");
  CHECK(visualText(QString::fromUtf8(u8"שלום\nMain 123"),
                   Qt::LayoutDirectionAuto) == reversed + "\nMain 123");
  CHECK(visualText(QString::fromUtf8(u8"שלום\r\nMain 123"),
                   Qt::LayoutDirectionAuto) == reversed + "\r\nMain 123");
  CHECK(visualText(QString::fromUtf8(u8"שלום") + QChar(0x2029) + "Main 123",
                   Qt::LayoutDirectionAuto) ==
        reversed + QChar(0x2029) + "Main 123");
}

TEST_CASE("Bidi ordering keeps graphemes and malformed UTF-16 intact",
          "[textbidi]") {
  const QString marked = QString(QChar(0x05e9)) + QChar(0x05b0) + " 123";
  const auto clusters = lc::textbidi::visualClusters(marked, Qt::RightToLeft);
  REQUIRE(!clusters.empty());
  CHECK(clusters.back().start == 0);
  CHECK(clusters.back().length == 2);
  const QString pair = QString(QChar(0xd83d)) + QChar(0xde00);
  const auto supplementary =
      lc::textbidi::visualClusters(pair, Qt::RightToLeft);
  REQUIRE(supplementary.size() == 1);
  CHECK(supplementary.front().length == 2);
  for (const auto &text : {QString(), QString(QChar(0xd83d)),
                           QString(QChar(0xde00)), QString(pair + marked)}) {
    auto indices = lc::textbidi::visualOrder(text, Qt::RightToLeft);
    std::sort(indices.begin(), indices.end());
    REQUIRE(indices.size() == text.size());
    for (int i = 0; i < text.size(); ++i) {
      CHECK(indices[i] == i);
    }
  }
}
