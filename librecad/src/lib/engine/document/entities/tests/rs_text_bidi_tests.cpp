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
#include "rs_text.h"

namespace {

// QGuiApplication bootstrap (offscreen) — matches the fixture in
// rs_mtext_bidi_tests.cpp so the bidi pass can run regardless of which test
// file Catch2 happens to schedule first. Process-singleton, intentionally
// never destroyed.
struct QtAppBootstrap {
  QtAppBootstrap() {
    if (QGuiApplication::instance() != nullptr)
      return;
    qputenv("QT_QPA_PLATFORM", "offscreen");
    static int argc = 1;
    static char arg0[] = "librecad_tests";
    static char *argv[] = {arg0, nullptr};
    new QGuiApplication(argc, argv);
  }
};
static QtAppBootstrap s_qtAppBootstrap;

/**
 * Mirror of the helper in rs_text.cpp's anonymous namespace. We re-implement
 * here so the test can assert direction resolution behavior without needing
 * to instantiate an RS_Text (which would require a font).
 */
Qt::LayoutDirection
resolveBaseDirection(const QString &text,
                     RS_TextData::DrawingDirection setting) {
  if (setting == RS_TextData::LeftToRight)
    return Qt::LeftToRight;
  if (setting == RS_TextData::RightToLeft)
    return Qt::RightToLeft;
  for (int i = 0; i < text.size(); ++i) {
    const QChar::Direction d = text.at(i).direction();
    if (d == QChar::DirL)
      return Qt::LeftToRight;
    if (d == QChar::DirR || d == QChar::DirAL)
      return Qt::RightToLeft;
  }
  return Qt::LeftToRight;
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
  REQUIRE(resolveBaseDirection(QString::fromUtf8("\xD7\xA9\xD7\x9C"),
                               RS_TextData::LeftToRight) == Qt::LeftToRight);
}

TEST_CASE("RS_Text bidi: explicit RightToLeft setting wins", "[text][bidi]") {
  REQUIRE(resolveBaseDirection(QStringLiteral("hello"),
                               RS_TextData::RightToLeft) == Qt::RightToLeft);
}

TEST_CASE("RS_Text bidi: ByContent picks LTR for Latin", "[text][bidi]") {
  REQUIRE(resolveBaseDirection(QStringLiteral("hello world"),
                               RS_TextData::ByContent) == Qt::LeftToRight);
}

TEST_CASE("RS_Text bidi: ByContent picks RTL for first-strong Hebrew",
          "[text][bidi]") {
  // "שלום world" — leading Hebrew → first-strong is R → RTL base.
  QString s = QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  s += QStringLiteral(" world");
  REQUIRE(resolveBaseDirection(s, RS_TextData::ByContent) == Qt::RightToLeft);
}

TEST_CASE("RS_Text bidi: ByContent picks LTR when leading neutrals + Latin",
          "[text][bidi]") {
  // "  hello שלום" — first strong is 'h' (L) → LTR base.
  QString s = QStringLiteral("  hello ");
  s += QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  REQUIRE(resolveBaseDirection(s, RS_TextData::ByContent) == Qt::LeftToRight);
}

TEST_CASE("RS_Text bidi: ByContent on empty / pure-neutral falls back to LTR",
          "[text][bidi]") {
  REQUIRE(resolveBaseDirection(QString(), RS_TextData::ByContent) ==
          Qt::LeftToRight);
  REQUIRE(resolveBaseDirection(QStringLiteral("   "), RS_TextData::ByContent) ==
          Qt::LeftToRight);
  REQUIRE(resolveBaseDirection(QStringLiteral("123 456"),
                               RS_TextData::ByContent) == Qt::LeftToRight);
}

TEST_CASE("RS_Text bidi: visual-order pass reverses Hebrew like MText does",
          "[text][bidi]") {
  // End-to-end check that RS_MText::computeBidiVisualOrder, fed with the
  // direction RS_Text resolves, reorders Hebrew to visual order. RS_Text
  // calls computeBidiVisualOrder(text, baseDir) directly, so this also
  // validates the integration.
  const QString s = QString::fromUtf8("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
  Qt::LayoutDirection base = resolveBaseDirection(s, RS_TextData::ByContent);
  REQUIRE(base == Qt::RightToLeft);

  auto v = RS_MText::computeBidiVisualOrder(s, base);
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

  Qt::LayoutDirection base = resolveBaseDirection(s, RS_TextData::ByContent);
  REQUIRE(base == Qt::LeftToRight);
  auto v = RS_MText::computeBidiVisualOrder(s, base);

  // 'H' is leftmost.
  REQUIRE(v.front() == 0);
  // '!' (the last logical char) ends up rightmost.
  REQUIRE(v.back() == s.size() - 1);
  // Hebrew run reversed: hebrewEnd-1 visually before hebrewStart.
  int firstHebrewLogical = hebrewStart;
  int lastHebrewLogical = hebrewEndExcl - 1;
  REQUIRE(posOf(v, lastHebrewLogical) < posOf(v, firstHebrewLogical));
}
