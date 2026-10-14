/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textruns.h"
#include "lc_opentypefont.h"
#include "lc_textshaper.h"
#include <catch2/catch_test_macros.hpp>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
#include <hb.h>
namespace {
QString fontPath() {
#ifdef LC_OPENTYPE_TEST_FONT_PATH
    return QString::fromUtf8(LC_OPENTYPE_TEST_FONT_PATH);
#else
    return QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir().absoluteFilePath(
        "../../../../../../support/fonts/opentype/amiri-1.003/Amiri-Regular.ttf");
#endif
}
QByteArray scriptTag(const LC_TextRuns::Run &run) {
    if (!run.representativeScalar) return "Arab"; // This test's explicit fallback policy.
    const auto script = hb_unicode_script(hb_unicode_funcs_get_default(), run.representativeScalar);
    const hb_tag_t tag = hb_script_to_iso15924_tag(script);
    char bytes[4];
    hb_tag_to_string(tag, bytes);
    return QByteArray(bytes, 4);
}
LC_TextShaper::Result shapeItem(const LC_OpenTypeFont &font, const LC_TextRuns::Paragraph &paragraph,
                              const LC_TextRuns::Run &item) {
    LC_TextShaper::Run run;
    run.contextText = paragraph.text.mid(item.context.start, item.context.length);
    run.itemStart = item.text.start - item.context.start;
    run.itemLength = item.text.length;
    run.sourceOffset = item.context.start;
    run.rightToLeft = item.rightToLeft();
    run.script = scriptTag(item);
    run.language = item.style.language;
    run.beginningOfText = item.beginsContext();
    run.endOfText = item.endsContext();
    return LC_TextShaper::shape(font, run);
}
void checkBeh(const LC_TextDocument::Paragraph &source, std::uint32_t expected) {
    const auto loaded = LC_OpenTypeFont::load(fontPath());
    REQUIRE(loaded);
    REQUIRE(loaded.font->checksum() == LC_OpenTypeFont::amiriProfile().sha256);
    LC_TextRuns::Options options;
    options.direction = Qt::LeftToRight;
    const auto paragraph = LC_TextRuns::itemize(source, options);
    REQUIRE(paragraph.ok());
    const int beh = int(paragraph.text.indexOf(QChar(0x0628)));
    REQUIRE(beh >= 0);
    std::vector<int> owned(paragraph.text.size(), 0);
    int behGlyphs = 0;
    for (const auto &item : paragraph.runs) {
        for (int unit = item.text.start; unit < item.text.end(); ++unit) ++owned[unit];
        if (item.kind != LC_TextRuns::Kind::Text) continue;
        const auto result = shapeItem(*loaded.font, paragraph, item);
        INFO(result.error.message.toStdString());
        REQUIRE(result);
        CHECK(result.unsupportedRanges.empty());
        for (const auto &glyph : result.glyphs) {
            // Borrowed pre/post context must never emit a duplicate control or
            // acquire another item's source ownership, even across font spans.
            CHECK(glyph.cluster >= item.text.start);
            CHECK(glyph.cluster < item.text.end());
            CHECK(glyph.source.start >= item.text.start);
            CHECK(glyph.source.start + glyph.source.length <= item.text.end());
            CHECK_FALSE(paragraph.sourceRanges(glyph.source.start, glyph.source.length).empty());
            if (glyph.cluster == beh && glyph.id != 1) {
                ++behGlyphs;
                CHECK(glyph.id == expected);
            }
        }
    }
    CHECK(behGlyphs == 1);
    CHECK(std::all_of(owned.begin(), owned.end(), [](int count) { return count == 1; }));
}
} // namespace

TEST_CASE("Itemizer preserves pinned joining glyphs across Latin Arabic level edges",
          "[textruns][shaping][opentype][headless]") {
    // Pinned Amiri 1.003: isolated beh=56, final=1589, initial=1611.
    // UAX #9 X9/6.1 preserves ZWJ effects on both backing-store neighbors,
    // whereas 3.5 forbids borrowing a retained other-level letter as context.
    for (const auto &text : {QStringLiteral("A\u200dب"), QStringLiteral("\u200dب"),
                            QStringLiteral("A\u200d\u202bب\u202c"),
                            QStringLiteral("A\u200d\u200c\u200dب")})
        checkBeh(LC_TextDocument::plain(text).paragraphs[0], 1589);
    for (const auto &text : {QStringLiteral("ب\u200dA"), QStringLiteral("ب\u200d")})
        checkBeh(LC_TextDocument::plain(text).paragraphs[0], 1611);
    for (const auto &text : {QStringLiteral("A\u200cب"), QStringLiteral("ب\u200cA"),
                            QStringLiteral("A\u200dCب"), QStringLiteral("A\u200d\u2067ب\u2069"),
                            QStringLiteral("A\u200d\u2067\u2066\u2069ب\u2069"),
                            QStringLiteral("A\u200d\tب"), QStringLiteral("A\u200d\ufffcب")})
        checkBeh(LC_TextDocument::plain(text).paragraphs[0], 56);
    const auto paragraphs = LC_TextDocument::plain(QStringLiteral("A\u200d\nب"));
    REQUIRE(paragraphs.paragraphs.size() == 2);
    checkBeh(paragraphs.paragraphs[1], 56);
}

TEST_CASE("Borrowed joining controls retain font scale and paint item behavior",
          "[textruns][shaping][opentype][format][headless]") {
    LC_TextDocument::Options options;
    options.syntax = LC_TextDocument::Syntax::MText;
    options.initialStyle.font = QStringLiteral("lc-amiri-1.003-ot-v1");
    // The adapter test uses the same pinned face for both font identifiers; it
    // validates item/context handoff, not resource substitution or CAD scaling.
    const auto document = LC_TextDocument::parse(
        QStringLiteral("A\u200d{\\f{Amiri-Regular.ttf}\\H2x;ب}\\C1;\u200cA"), options);
    REQUIRE(document.ok());
    checkBeh(document.paragraphs[0], 1589);
    const auto paint = LC_TextDocument::parse(QStringLiteral("A\u200d\\C1;ب"), options);
    REQUIRE(paint.ok());
    checkBeh(paint.paragraphs[0], 1589);
}

TEST_CASE("Ownership-only context reproduces the missing leading ZWJ glyph bug",
          "[textruns][shaping][opentype][regression][headless]") {
    const auto loaded = LC_OpenTypeFont::load(fontPath());
    REQUIRE(loaded);
    const auto source = LC_TextDocument::plain(QStringLiteral("A\u200dب"));
    LC_TextRuns::Options options;
    options.direction = Qt::LeftToRight;
    const auto paragraph = LC_TextRuns::itemize(source.paragraphs[0], options);
    REQUIRE(paragraph.ok());
    const auto &item = paragraph.runs[paragraph.runForUnit[2]];
    const auto corrected = shapeItem(*loaded.font, paragraph, item);
    REQUIRE(corrected);
    REQUIRE(corrected.glyphs.size() == 1);
    CHECK(corrected.glyphs[0].id == 1589);
    auto ownershipOnly = item;
    // Isolated reproducer of the previous context contract, without editing
    // the parser, resolver, shaper or pinned font to create a passing oracle.
    ownershipOnly.context = paragraph.contexts[item.contextIndex].text;
    const auto previous = shapeItem(*loaded.font, paragraph, ownershipOnly);
    REQUIRE(previous);
    REQUIRE(previous.glyphs.size() == 1);
    CHECK(previous.glyphs[0].id == 56);
    CHECK(previous.glyphs[0].id != corrected.glyphs[0].id);
}
#else
TEST_CASE("Itemizer shaping integration requires explicitly enabled OpenType",
          "[textruns][shaping][opentype][headless]") {
    CHECK_FALSE(LC_OpenTypeFont::backendAvailable());
}
#endif
