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
#include <utility>
#include <catch2/catch_test_macros.hpp>

#include <QChar>
#include <QList>
#include <QString>

#include "lc_textbidi.h"
#include "qt_bidi.h"

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
    for (char32_t scalar : text.mid(cluster.start, cluster.length).toUcs4()) {
      if (QChar::category(scalar) != QChar::Other_Format) {
        if (cluster.rightToLeft)
          scalar = QChar::mirroredChar(scalar);
        visual += QString::fromUcs4(&scalar, 1);
      }
    }
  }
  return visual;
}
} // namespace

TEST_CASE("Chinese RTL reverses Han runs but preserves numeric values",
          "[textbidi][chinese]") {
  const auto chinese = QStringLiteral("\u4e2d\u6587");
  const auto reversed = QStringLiteral("\u6587\u4e2d");
  for (const QString &number : {QStringLiteral("123"), QStringLiteral("12.5"),
                                QStringLiteral("1,234.56"), QStringLiteral("12/34"),
                                QStringLiteral("CAD123")}) {
    const auto input = chinese + number;
    CHECK(visualText(input, Qt::LeftToRight) == input);
    CHECK(visualText(input, Qt::LayoutDirectionAuto) == input);
    CHECK(visualText(input, Qt::RightToLeft) == number + reversed);
    const auto display = lc::textbidi::directionalText(input, Qt::RightToLeft);
    QString restored;
    for (int i = 0; i < int(display.sourcePositions.size()); ++i) {
      if (display.sourcePositions[i] >= 0) restored += display.text.at(i);
    }
    CHECK(restored == input);
    for (int i = 0; i <= input.size(); ++i)
      CHECK(display.sourcePosition(display.displayPositions[i]) == i);
  }
  CHECK(visualText(chinese + QStringLiteral("\u4e00\u4e8c\u4e09"), Qt::RightToLeft) ==
        QStringLiteral("\u4e09\u4e8c\u4e00") + reversed);
  const auto explicitLtr = QChar(0x2066) + chinese + "123" + QChar(0x2069);
  CHECK(visualText(explicitLtr, Qt::RightToLeft) == chinese + "123");
  const auto nested = QStringLiteral("\u2066\u202a") + chinese + QChar(0x2069);
  const auto projected = lc::textbidi::directionalText(nested + chinese, Qt::RightToLeft);
  CHECK(projected.text.startsWith(nested + QStringLiteral("\u2067\u202e")));
  const auto strayPdf = QString(QChar(0x2066)) + QChar(0x202c) + chinese + QChar(0x2069);
  CHECK(lc::textbidi::directionalText(strayPdf, Qt::RightToLeft).text == strayPdf);
  const char32_t supplementary[] = {0x20000, 0xe0100, 0x20001};
  const auto input = QString::fromUcs4(supplementary, 3) + "123";
  const auto clusters = lc::textbidi::visualClusters(input, Qt::RightToLeft);
  REQUIRE(clusters.size() == 5);
  CHECK(clusters[3].start == 4);
  CHECK(clusters[4].start == 0);
  CHECK(clusters[4].length == 4);
}

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

TEST_CASE("Bidi resolves supplementary RTL letters in LTR paragraphs",
          "[textbidi]") {
  const char32_t letters[] = {0x1e900, 0x1e901, 0x1e902};
  const char32_t reversed[] = {0x1e902, 0x1e901, 0x1e900};
  const auto adlam = QString::fromUcs4(letters, 3);
  const auto reverseAdlam = QString::fromUcs4(reversed, 3);
  for (auto direction : {Qt::LeftToRight, Qt::RightToLeft,
                         Qt::LayoutDirectionAuto}) {
    CHECK(visualText(adlam + " 123", direction) == "123 " + reverseAdlam);
    const auto levels = lc::qtbidi::resolve(adlam, direction).levels;
    REQUIRE(levels.size() == 6);
    for (size_t i = 0; i < levels.size(); i += 2)
      CHECK(levels[i] == levels[i + 1]);
  }
  for (auto direction : {Qt::LeftToRight, Qt::LayoutDirectionAuto})
    CHECK(visualText("A " + adlam + " 123", direction) ==
          "A 123 " + reverseAdlam);
  const auto clusters = lc::textbidi::visualClusters(adlam, Qt::LeftToRight);
  REQUIRE(clusters.size() == 3);
  for (int i = 0; i < 3; ++i) {
    CHECK(clusters[i].start == 4 - 2 * i);
    CHECK(clusters[i].length == 2);
  }
}

TEST_CASE("Bidi brackets use canonical equivalence, not compatibility folding",
          "[textbidi]") {
  const auto alef = QString(QChar(0x05d0)), bet = QString(QChar(0x05d1));
  for (char16_t closing : {char16_t(0xff09), char16_t(0x207e),
                           char16_t(0x208e), char16_t(0xfe5a)}) {
    const QString input = "A(" + alef + QChar(closing) + bet;
    const auto levels = lc::qtbidi::resolve(input, Qt::LeftToRight).levels;
    REQUIRE(levels.size() == 5);
    CHECK(levels[1] == 0);
    CHECK(levels[3] == 1);
    CHECK(visualText(input, Qt::LeftToRight) ==
          "A(" + bet + QChar(closing).mirroredChar() + alef);
  }
  for (const auto &pair : {std::pair<char16_t, char16_t>{0x2329, 0x3009},
                           {0x3008, 0x232a}}) {
    const QString input = "A" + QString(QChar(pair.first)) + alef +
                          QChar(pair.second) + bet;
    const auto levels = lc::qtbidi::resolve(input, Qt::LeftToRight).levels;
    REQUIRE(levels.size() == 5);
    CHECK(levels[1] == 0);
    CHECK(levels[3] == 0);
  }
}

TEST_CASE("Bidi bracket NSMs inherit resolved direction across X9 controls",
          "[textbidi]") {
  struct Case {
    std::vector<char32_t> scalars;
    std::vector<int> levels;
  };
  // Unicode 17 BidiCharacterTest: N0 cases following opening/closing brackets.
  const Case cases[] = {
      {{0x41, 0x200f, 0x5b, 0x5d0, 0x5d, 0x200d, 0x20d6},
       {0, 1, 1, 1, 1, -1, 1}},
      {{0x41, 0x200f, 0x5b, 0x200d, 0x20d6, 0x5d0, 0x5d, 0x200d, 0x20d6},
       {0, 1, 1, -1, 1, 1, 1, -1, 1}},
      {{0x41, 0x200f, 0x5b, 0x200d, 0x200b, 0x20d6, 0x5d0, 0x5d,
        0x200b, 0x200d, 0x20d6},
       {0, 1, 1, -1, -1, 1, 1, 1, -1, -1, 1}}};
  for (const auto &test : cases) {
    const auto text = QString::fromUcs4(test.scalars.data(), test.scalars.size());
    const auto resolved = lc::qtbidi::resolve(text, Qt::LeftToRight);
    CHECK(resolved.base == 0);
    REQUIRE(resolved.levels.size() == test.levels.size());
    for (size_t i = 0; i < test.levels.size(); ++i)
      if (test.levels[i] >= 0)
        CHECK(resolved.levels[i] == test.levels[i]);
  }
}

TEST_CASE("Bidi isolates and X9 controls do not leak into surrounding runs",
          "[textbidi]") {
  struct Case {
    std::vector<char32_t> scalars;
    std::vector<int> levels;
    Qt::LayoutDirection direction;
  };
  const Case cases[] = {
      {{0x41, 0x200f, 0x28, 0x2066, 0x2066, 0x41, 0x2069, 0x5d0,
        0x2069, 0x29, 0x41},
       {0, 1, 0, 0, 2, 4, 2, 3, 0, 0, 0}, Qt::LeftToRight},
      {{0x2067, 0x41, 0x2069, 0x202a, 0x3009, 0x661},
       {1, 4, 1, -1, 2, 4}, Qt::RightToLeft},
      {{0x5d0, 0x202d, 0x202c, 0x2068, 0x2069, 0x5d0},
       {1, -1, -1, 1, 1, 1}, Qt::LeftToRight},
      {{0x627, 0x202e, 0x202c, 0x31, 0x2067},
       {1, -1, -1, 2, 0}, Qt::LeftToRight},
      {{0x202d, 0x202e, 0x41, 0x202c, 0x2069, 0x202e, 0x42},
       {-1, -1, 3, -1, 2, -1, 3}, Qt::LeftToRight}};
  for (const auto &test : cases) {
    const auto text = QString::fromUcs4(test.scalars.data(), test.scalars.size());
    const auto resolved = lc::qtbidi::resolve(text, test.direction);
    REQUIRE(resolved.levels.size() == test.levels.size());
    for (size_t i = 0; i < test.levels.size(); ++i)
      if (test.levels[i] >= 0)
        CHECK(resolved.levels[i] == test.levels[i]);
  }
}

TEST_CASE("Bidi FSI matching survives explicit-level overflow",
          "[textbidi]") {
  for (int depth : {63, 125, 128, 129, 256}) {
    const QString prefix = QString(QChar(0x2066)).repeated(depth) + "A" +
                           QString(QChar(0x2069)).repeated(depth);
    const QString input = prefix + QChar(0x2068) + QChar(0x05d0) + QChar(0x2069);
    const auto levels = lc::qtbidi::resolve(input, Qt::LeftToRight).levels;
    REQUIRE(levels.size() == input.size());
    CHECK(levels[prefix.size()] == 0);
    CHECK(levels[prefix.size() + 1] == 1);
    CHECK(levels[prefix.size() + 2] == 0);
    auto order = lc::qtbidi::reorder(levels);
    std::sort(order.begin(), order.end());
    for (int i = 0; i < input.size(); ++i)
      REQUIRE(order[i] == i);
  }
}

TEST_CASE("Supplementary boundary neutrals do not prevent L1 whitespace reset",
          "[textbidi]") {
  for (const char32_t control : {char32_t(0xe0001), char32_t(0x1d173)}) {
    REQUIRE(QChar::direction(control) == QChar::DirBN);
    const auto alef = QString(QChar(0x05d0));
    const QString input = QChar(0x202b) + alef + " " +
                          QString::fromUcs4(&control, 1);
    const auto levels = lc::qtbidi::resolve(input, Qt::LeftToRight).levels;
    REQUIRE(levels.size() == 5);
    CHECK(levels[2] == 0);
    CHECK(levels[3] == levels[4]);
    CHECK(visualText(input, Qt::LeftToRight) == alef + " ");
  }
}
