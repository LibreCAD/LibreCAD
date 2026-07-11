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

// Phase 3 regression tests: caret-decode (toNativeString) generalization
// and NFC normalization on layer/block name lookups.

#include <catch2/catch_test_macros.hpp>

#include <QString>

#include "rs_filterdxfrw.h"
#include "rs_layerlist.h"
#include "rs_blocklist.h"
#include "rs_layer.h"
#include "rs_block.h"
#include "rs_vector.h"
#include "lc_dimstyleslist.h"
#include "lc_dimstyle.h"
#include "lc_viewslist.h"
#include "lc_view.h"

namespace {
// Build a decomposed (NFD) "Xö..." by inserting a base letter + COMBINING
// DIAERESIS where an NFC string would have the precomposed 'ö' (U+00F6).
QString nfdOWithDiaeresis(const QString& prefix, const QString& suffix) {
    QString s = prefix;
    s.append(QChar('o'));
    s.append(QChar(0x0308)); // COMBINING DIAERESIS
    s.append(suffix);
    return s;
}
} // namespace

TEST_CASE("toNativeString: caret-decode ^I ^J ^M as TAB/LF/CR",
          "[i18n][caret]") {
    QString in = QStringLiteral("a^Ib^Jc^Md");
    QString out = RS_FilterDXFRW::toNativeString(in);
    REQUIRE(out == QStringLiteral("a\tb\nc\rd"));
}

TEST_CASE("toNativeString: caret-decode ^space becomes literal ^",
          "[i18n][caret]") {
    QString in = QStringLiteral("x^ y");
    QString out = RS_FilterDXFRW::toNativeString(in);
    REQUIRE(out == QStringLiteral("x^y"));
}

TEST_CASE("toNativeString: caret-decode general ^A becomes SOH",
          "[i18n][caret]") {
    // 'A' = 65; (65 - 64) mod 126 = 1 = SOH.
    QString in = QStringLiteral("^A");
    QString out = RS_FilterDXFRW::toNativeString(in);
    REQUIRE(out.size() == 1);
    REQUIRE(out.at(0).unicode() == 0x01);
}

TEST_CASE("toNativeString: existing %%c %%d %%p semantics preserved",
          "[i18n][content_codes]") {
    QString in = QStringLiteral("%%c %%d %%p");
    QString out = RS_FilterDXFRW::toNativeString(in);
    REQUIRE(out == QStringLiteral("⌀ ° ±"));
}

TEST_CASE("toNativeString: \\P decode (LF) still works alongside caret",
          "[i18n][caret][content_codes]") {
    QString in = QStringLiteral("a\\Pb^Jc");
    QString out = RS_FilterDXFRW::toNativeString(in);
    REQUIRE(out == QStringLiteral("a\nb\nc"));
}

TEST_CASE("RS_LayerList::find: NFC layer matches NFD lookup",
          "[i18n][nfc][layers]") {
    RS_LayerList list;
    // Insert a layer name in composed (NFC) form: 'Röhre'.
    const QString nfc = QString::fromUtf8("R\xC3\xB6hre");
    list.add(new RS_Layer(nfc));
    // Lookup with decomposed (NFD) form: 'R' + 'o' + combining diaeresis.
    QString nfd;
    nfd.append(QChar('R'));
    nfd.append(QChar('o'));
    nfd.append(QChar(0x0308)); // COMBINING DIAERESIS
    nfd.append(QString::fromUtf8("hre"));
    REQUIRE(nfc != nfd); // byte-different
    REQUIRE(list.find(nfd) != nullptr);
}

TEST_CASE("RS_BlockList::find: NFC block matches NFD lookup",
          "[i18n][nfc][blocks]") {
    RS_BlockList list;
    const QString nfc = QString::fromUtf8("Bl\xC3\xB6" "ck");
    RS_BlockData data(nfc, RS_Vector{0, 0, 0}, false);
    list.add(new RS_Block(nullptr, data));
    QString nfd;
    nfd.append(QChar('B'));
    nfd.append(QChar('l'));
    nfd.append(QChar('o'));
    nfd.append(QChar(0x0308));
    nfd.append(QChar('c'));
    nfd.append(QChar('k'));
    REQUIRE(nfc != nfd);
    REQUIRE(list.find(nfd) != nullptr);
}

TEST_CASE("LC_DimStylesList::findByName: NFC style matches NFD lookup",
          "[i18n][nfc][dimstyles]") {
    LC_DimStylesList list;
    const QString nfc = QString::fromUtf8("St\xC3\xB6" "rung"); // "Störung" NFC
    list.addDimStyle(new LC_DimStyle(nfc));
    const QString nfd = nfdOWithDiaeresis(QStringLiteral("St"), QStringLiteral("rung"));
    REQUIRE(nfc != nfd);
    REQUIRE(list.findByName(nfd) != nullptr);
    // Case-insensitive + NFC together.
    REQUIRE(list.findByName(nfd.toUpper()) != nullptr);
}

TEST_CASE("LC_ViewList::find + getIndex: NFC view matches NFD lookup",
          "[i18n][nfc][views]") {
    LC_ViewList list;
    const QString nfc = QString::fromUtf8("Dr\xC3\xB6" "hne"); // "Dröhne" NFC
    list.add(new LC_View(nfc));
    const QString nfd = nfdOWithDiaeresis(QStringLiteral("Dr"), QStringLiteral("hne"));
    REQUIRE(nfc != nfd);
    REQUIRE(list.find(nfd) != nullptr);
    REQUIRE(list.getIndex(nfd) == 0);
}
