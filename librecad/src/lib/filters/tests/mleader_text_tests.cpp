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

/**
 * LC_MLeader text-content rendering tests.
 *
 * A multileader's annotation text was stored on LC_MLeaderData but never drawn
 * (LC_MLeader::draw rendered only leader lines + arrowheads). It now renders a
 * transient RS_MText built by LC_MLeader::textContentData(). The painting is
 * GUI, but the error-prone part is the LC_MLeaderData -> RS_MTextData mapping,
 * which this exercises directly: insertion point, height (with the 0-height
 * fallback), rotation and the text body.
 */

#include <catch2/catch_test_macros.hpp>

#include "lc_mleader.h"
#include "rs_mtext.h"

TEST_CASE("LC_MLeader maps text content to RS_MTextData", "[mleader][text]") {
  LC_MLeaderData d;
  d.hasTextContents = true;
  d.textLabel = "N1";
  d.textLocation = RS_Vector(12.0, 34.0);
  d.textHeight = 2.5;
  d.textRotation = 0.5;
  LC_MLeader m(nullptr, d);

  RS_MTextData td;
  REQUIRE(m.textContentData(td));
  CHECK(td.text == QString("N1"));
  CHECK(td.insertionPoint.x == 12.0);
  CHECK(td.insertionPoint.y == 34.0);
  CHECK(td.height == 2.5);
  CHECK(td.angle == 0.5);
}

TEST_CASE("LC_MLeader text height falls back when context height is 0",
          "[mleader][text]") {
  LC_MLeaderData d;
  d.hasTextContents = true;
  d.textLabel = "x";
  d.textLocation = RS_Vector(0.0, 0.0);
  d.textHeight = 0.0;     // no height in the context
  d.scaleFactor = 4.0;    // -> fallback height = scaleFactor * 2.5 = 10
  LC_MLeader m(nullptr, d);

  RS_MTextData td;
  REQUIRE(m.textContentData(td));
  CHECK(td.height == 10.0);
}

TEST_CASE("LC_MLeader with no text content yields no RS_MTextData",
          "[mleader][text]") {
  LC_MLeaderData d;            // hasTextContents defaults to false
  LC_MLeader m(nullptr, d);
  RS_MTextData td;
  CHECK_FALSE(m.textContentData(td));

  LC_MLeaderData blk;
  blk.hasBlockContents = true; // block content, not text
  blk.textLocation = RS_Vector(1.0, 1.0);
  LC_MLeader mb(nullptr, blk);
  CHECK_FALSE(mb.textContentData(td));

  LC_MLeaderData empty;
  empty.hasTextContents = true; // flagged but empty label -> nothing to draw
  empty.textLocation = RS_Vector(1.0, 1.0);
  LC_MLeader me(nullptr, empty);
  CHECK_FALSE(me.textContentData(td));
}
