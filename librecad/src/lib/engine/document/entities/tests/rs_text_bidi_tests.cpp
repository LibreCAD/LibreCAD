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

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <QChar>
#include <QString>
#include <Qt>

#include "lc_textbidi.h"
#include "rs_text.h"

namespace {

QString visualText(const QString &text, Qt::LayoutDirection direction) {
  QString result;
  for (int index : lc::textbidi::visualOrder(text, direction))
    result += text.at(index);
  return result;
}

/** Find visual position of @p logIdx, or -1 if absent. */
int posOf(const std::vector<int> &visual, int logIdx) {
  for (size_t k = 0; k < visual.size(); ++k) {
    if (visual[k] == logIdx)
      return static_cast<int>(k);
  }
  return -1;
}

} // namespace

TEST_CASE("RS_Text bidi: explicit LeftToRight setting wins", "[text][bidi]") {
  REQUIRE(visualText(QString::fromUtf8(u8"שלום world"), Qt::LeftToRight) ==
          QString::fromUtf8(u8"םולש world"));
}

TEST_CASE("RS_Text bidi: explicit RightToLeft setting wins", "[text][bidi]") {
  REQUIRE(visualText(QString::fromUtf8(u8"hello שלום"), Qt::RightToLeft) ==
          QString::fromUtf8(u8"םולש hello"));
}

TEST_CASE("RS_Text bidi: ByContent picks LTR for Latin", "[text][bidi]") {
  REQUIRE(visualText("hello world", Qt::LayoutDirectionAuto) == "hello world");
}

TEST_CASE("RS_Text bidi: ByContent picks RTL for first-strong Hebrew",
          "[text][bidi]") {
  // "שלום world" — leading Hebrew → first-strong is R → RTL base.
  QString s = QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  s += QStringLiteral(" world");
  REQUIRE(visualText(s, Qt::LayoutDirectionAuto) ==
          QString::fromUtf8(u8"world םולש"));
}

TEST_CASE("RS_Text bidi: ByContent picks LTR when leading neutrals + Latin",
          "[text][bidi]") {
  // "  hello שלום" — first strong is 'h' (L) → LTR base.
  QString s = QStringLiteral("  hello ");
  s += QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  REQUIRE(visualText(s, Qt::LayoutDirectionAuto) ==
          QString::fromUtf8(u8"  hello םולש"));
}

TEST_CASE("RS_Text bidi: ByContent on empty / pure-neutral falls back to LTR",
          "[text][bidi]") {
  for (const QString &s :
       {QString(), QStringLiteral("   "), QStringLiteral("123 456")})
    REQUIRE(visualText(s, Qt::LayoutDirectionAuto) == s);
}

TEST_CASE("RS_Text bidi: visual-order pass reverses Hebrew like MText does",
          "[text][bidi]") {
  const QString s = QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  auto v = lc::textbidi::visualOrder(s, Qt::LayoutDirectionAuto);
  REQUIRE(v.size() == static_cast<size_t>(s.size()));
  REQUIRE(v.front() == s.size() - 1); // logical-last is leftmost visually
  REQUIRE(v.back() == 0);             // logical-first is rightmost visually
}

TEST_CASE("RS_Text bidi: mixed Hebrew + Latin reorders correctly with auto",
          "[text][bidi]") {
  // "Hello שלום!" — first strong is 'H' (L) → LTR base. Hebrew run is
  // reversed; Latin and trailing '!' stay LTR.
  QString s = QStringLiteral("Hello ");
  const int hebrewStart = s.size();
  s += QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  const int hebrewEndExcl = s.size();
  s += QChar('!');

  auto v = lc::textbidi::visualOrder(s, Qt::LayoutDirectionAuto);

  // 'H' is leftmost.
  REQUIRE(v.front() == 0);
  // '!' (the last logical char) ends up rightmost.
  REQUIRE(v.back() == s.size() - 1);
  // Hebrew run reversed: hebrewEnd-1 visually before hebrewStart.
  int firstHebrewLogical = hebrewStart;
  int lastHebrewLogical = hebrewEndExcl - 1;
  REQUIRE(posOf(v, lastHebrewLogical) < posOf(v, firstHebrewLogical));
}
