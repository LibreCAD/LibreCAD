/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textdocument.h"
#include <catch2/catch_test_macros.hpp>
#include <QChar>
#include <QList>
#include <cmath>
#include <limits>
#include <random>

namespace {
using Document = LC_TextDocument;
using Range = Document::Range;
Document::Options mtextOptions() {
    Document::Options options;
    options.syntax = Document::Syntax::MText;
    options.initialStyle.font = QStringLiteral("lc-amiri-1.003-ot-v1");
    return options;
}

void checkProvenance(const Document &document) {
    REQUIRE(document.ok());
    int sourceCursor = document.sourceRange.start;
    for (const auto &token : document.tokens) {
        CHECK(token.source.start == sourceCursor);
        CHECK(token.source.length > 0);
        sourceCursor = token.source.end();
        REQUIRE(token.paragraph >= 0);
        REQUIRE(token.paragraph < int(document.paragraphs.size()));
        CHECK(token.text.start >= 0);
        CHECK(token.text.end() <= document.paragraphs[token.paragraph].text.size());
    }
    CHECK(sourceCursor == document.sourceRange.end());
    for (const auto &paragraph : document.paragraphs) {
        int logicalCursor = 0;
        int previousSource = paragraph.source.start;
        for (const auto &scalar : paragraph.scalars) {
            CHECK(scalar.text.start == logicalCursor);
            CHECK(scalar.text.length == (scalar.value > 0xffff ? 2 : 1));
            CHECK(scalar.source.start >= previousSource);
            CHECK(scalar.source.length > 0);
            CHECK(scalar.source.end() <= paragraph.source.end());
            const auto decoded = paragraph.text.mid(scalar.text.start, scalar.text.length).toUcs4();
            REQUIRE(decoded.size() == 1);
            CHECK(decoded.front() == scalar.value);
            CHECK(paragraph.sourceRanges(scalar.text.start, scalar.text.length) == std::vector<Range>{scalar.source});
            logicalCursor = scalar.text.end();
            previousSource = scalar.source.end();
        }
        CHECK(logicalCursor == paragraph.text.size());
        logicalCursor = 0;
        for (const auto &span : paragraph.spans) {
            CHECK(span.text.start == logicalCursor);
            CHECK(span.text.length > 0);
            CHECK(std::isfinite(span.style.heightFactor));
            CHECK(span.style.heightFactor > 0);
            CHECK(std::isfinite(span.style.widthFactor));
            CHECK(span.style.widthFactor > 0);
            for (int i = span.text.start; i < span.text.end(); ++i)
                CHECK(paragraph.styleAt(i) == &span.style);
            logicalCursor = span.text.end();
        }
        CHECK(logicalCursor == paragraph.text.size());
        CHECK(paragraph.styleAt(-1) == nullptr);
        CHECK(paragraph.styleAt(int(paragraph.text.size())) == nullptr);
        for (const auto &object : paragraph.objects) {
            CHECK(paragraph.text.at(object.textPosition).unicode() == 0xfffc);
            checkProvenance(*object.upper);
            checkProvenance(*object.lower);
        }
    }
}
} // namespace

TEST_CASE("Plain logical documents preserve Unicode and literal syntax", "[textdocument]") {
    const QString source = QStringLiteral("بَ\u200dب\u200c\u2067abc\u2069 {\\F{font}} %%c \\U+0628");
    const auto document = Document::plain(source);
    CHECK(document.source == source);
    REQUIRE(document.paragraphs.size() == 1);
    CHECK(document.paragraphs[0].text == source);
    CHECK(document.diagnostics.empty());
    checkProvenance(document);
}

TEST_CASE("Informational font modifiers do not exhaust the parser", "[textdocument][limits]") {
    const QString source = QStringLiteral("{\\fArial|b0|i0|c0|p34;word}").repeated(1025);
    auto options = mtextOptions();
    for (const int limit : {0, 1, 1024}) {
        options.maxDiagnostics = limit;
        const auto document = Document::parse(source, options);
        REQUIRE(document.ok());
        CHECK(document.source == source);
        REQUIRE(document.paragraphs.size() == 1);
        CHECK(document.paragraphs.front().text == QStringLiteral("word").repeated(1025));
        CHECK(document.diagnostics.size() == std::size_t(limit));
    }
    options.maxScopeDepth = 0;
    CHECK_FALSE(Document::parse(source, options).ok());
}

TEST_CASE("Argumentless MTEXT breaks keep the following text", "[textdocument]") {
    for (const auto *command : {"\\P", "\\N", "\\X"}) {
        const QString source = QStringLiteral("<>") + QString::fromLatin1(command) + QStringLiteral("above; below");
        const auto document = Document::parse(source, mtextOptions());
        REQUIRE(document.ok());
        CHECK(document.source == source);
        REQUIRE(document.paragraphs.size() == 2);
        CHECK(document.paragraphs[0].text == QStringLiteral("<>"));
        CHECK(document.paragraphs[1].text == QStringLiteral("above; below"));
        checkProvenance(document);
    }
}

TEST_CASE("Scalar provenance and graphemes are separate UTF16 maps", "[textdocument]") {
    const QString source = QStringLiteral("بَ") + QChar(0xd83d) + QChar(0xde00) + QChar(0x200d);
    const auto document = Document::plain(source);
    const auto &paragraph = document.paragraphs.front();
    REQUIRE(paragraph.scalars.size() == 4);
    CHECK(paragraph.scalars[2].value == 0x1f600);
    CHECK(paragraph.scalars[2].text == Range{2, 2});
    CHECK(paragraph.sourceRanges(3, 1) == std::vector<Range>{{2, 2}});
    REQUIRE(!paragraph.graphemes.empty());
    CHECK(paragraph.graphemes.front() == Range{0, 2});
    checkProvenance(document);
}

TEST_CASE("Malformed UTF16 changes layout but never backing source", "[textdocument]") {
    const QString source = QString(QChar(0xd800)) + "A" + QChar(0xdc00);
    const auto document = Document::plain(source);
    CHECK(document.source == source);
    CHECK(document.paragraphs.front().text == QStringLiteral("\ufffdA\ufffd"));
    REQUIRE(document.diagnostics.size() == 2);
    CHECK(document.diagnostics.front().source == Range{0, 1});
    checkProvenance(document);
}

TEST_CASE("Paragraph breaks preserve spelling and empty paragraphs", "[textdocument]") {
    const QString source = QStringLiteral("ب\r\n\\P\nج\u2029");
    const auto document = Document::parse(source, mtextOptions());
    REQUIRE(document.paragraphs.size() == 5);
    CHECK(document.paragraphs[0].text == QStringLiteral("ب"));
    CHECK(document.paragraphs[0].breakSource == Range{1, 2});
    CHECK(document.paragraphs[1].text.isEmpty());
    CHECK(document.paragraphs[1].breakSource == Range{3, 2});
    CHECK(document.paragraphs[2].text.isEmpty());
    CHECK(document.paragraphs[3].text == QStringLiteral("ج"));
    CHECK(document.paragraphs[4].text.isEmpty());
    checkProvenance(document);
}

TEST_CASE("MTEXT escape spelling maps to logical scalars without joining gaps", "[textdocument]") {
    const QString source = QStringLiteral("ب\\~ب\\\\\\{\\}\\;\\^\t");
    const auto document = Document::parse(source, mtextOptions());
    const auto &paragraph = document.paragraphs.front();
    CHECK(paragraph.text == QStringLiteral("ب\u00a0ب\\{};^\t"));
    CHECK(paragraph.sourceRanges(1, 1) == std::vector<Range>{{1, 2}});
    CHECK(document.source == source);
    checkProvenance(document);
}

TEST_CASE("Empty paragraphs retain their effective scoped formatting", "[textdocument][format][empty]") {
    const auto options = mtextOptions();
    const auto source = QStringLiteral("{\\f{inline}\\H2x;\\W0.5;\\Q10;\\C2;\\L\\P\\P}\\P");
    const auto document = Document::parse(source, options);
    REQUIRE(document.ok());
    REQUIRE(document.paragraphs.size() == 4);
    auto inlineStyle = options.initialStyle;
    inlineStyle.font = QStringLiteral("inline");
    inlineStyle.fontSpecification = QStringLiteral("inline");
    inlineStyle.heightFactor = 2;
    inlineStyle.widthFactor = 0.5;
    inlineStyle.obliqueDegrees = 10;
    inlineStyle.color = 2;
    inlineStyle.underline = true;
    for (std::size_t i = 0; i < document.paragraphs.size(); ++i) {
        const auto &paragraph = document.paragraphs[i];
        CHECK(paragraph.text.isEmpty());
        CHECK(paragraph.spans.empty());
        CHECK(paragraph.scalars.empty());
        REQUIRE(paragraph.emptyStyle);
        CHECK(*paragraph.emptyStyle == (i < 2 ? inlineStyle : options.initialStyle));
    }
    CHECK(document.source == source);
    checkProvenance(document);
    CHECK_FALSE(Document::Paragraph{}.emptyStyle);
    const auto empty = Document::plain({});
    REQUIRE(empty.paragraphs.front().emptyStyle);
    CHECK(*empty.paragraphs.front().emptyStyle == Document::Style{});
}

TEST_CASE("Empty stack children retain inherited inline fonts metrics and paint", "[textdocument][stack][empty]") {
    const auto source = QStringLiteral("{\\f{inline}\\H2x;\\W0.5;\\C2;\\L\\S^;}");
    const auto document = Document::parse(source, mtextOptions());
    REQUIRE(document.ok());
    REQUIRE(document.paragraphs.front().objects.size() == 1);
    const auto &object = document.paragraphs.front().objects.front();
    const auto *parentStyle = document.paragraphs.front().styleAt(0);
    REQUIRE(parentStyle);
    CHECK(parentStyle->font == QStringLiteral("inline"));
    for (const auto *child : {object.upper.get(), object.lower.get()}) {
        REQUIRE(child);
        REQUIRE(child->paragraphs.size() == 1);
        const auto &paragraph = child->paragraphs.front();
        CHECK(paragraph.text.isEmpty());
        CHECK(paragraph.spans.empty());
        REQUIRE(paragraph.emptyStyle);
        CHECK(*paragraph.emptyStyle == *parentStyle);
        CHECK(paragraph.source.start >= object.source.start);
        CHECK(paragraph.source.end() <= object.source.end());
    }
    CHECK(document.source == source);
    checkProvenance(document);
}

TEST_CASE("Scoped formatting restores fonts metrics and paint independently", "[textdocument]") {
    auto options = mtextOptions();
    options.baseHeight = 4;
    const auto source = QStringLiteral("ب{\\f{other}\\H2x;\\W0.5;\\C2;\\Lب{\\H2;\\c16711680;ج}د}ه");
    const auto document = Document::parse(source, options);
    const auto &paragraph = document.paragraphs.front();
    CHECK(paragraph.text == QStringLiteral("ببجده"));
    REQUIRE(paragraph.spans.size() == 5);
    const auto *nested = paragraph.styleAt(2);
    REQUIRE(nested);
    CHECK(nested->font == QStringLiteral("other"));
    CHECK(nested->heightFactor == 0.5);
    CHECK(nested->widthFactor == 0.5);
    CHECK(nested->color == 16711680);
    CHECK(nested->trueColor);
    CHECK(nested->underline);
    CHECK(paragraph.styleAt(1)->heightFactor == 2);
    CHECK(paragraph.styleAt(3)->color == 2);
    CHECK(paragraph.styleAt(4)->font == options.initialStyle.font);
    CHECK(paragraph.styleAt(4)->heightFactor == 1);
    CHECK_FALSE(paragraph.styleAt(4)->underline);
    // A ligature's raw source can be discontiguous across paint-only controls.
    CHECK(paragraph.sourceRanges(0, 2).size() == 2);
    checkProvenance(document);
}

TEST_CASE("Numeric MTEXT commands accept only an immediate optional semicolon", "[textdocument][numeric]") {
    struct Example { QString command; double height = 1; double width = 1; double oblique = 0; int color = -1; bool rgb = false; };
    const Example examples[] = {
        {QStringLiteral("\\H2x"), 2}, {QStringLiteral("\\H+8"), 2},
        {QStringLiteral("\\H.5x"), .5}, {QStringLiteral("\\H1e1"), 2.5},
        {QStringLiteral("\\H2E-1x"), .2}, {QStringLiteral("\\H2X"), 2},
        {QStringLiteral("\\W.5"), 1, .5}, {QStringLiteral("\\W2x"), 1, 2},
        {QStringLiteral("\\W+1e-1"), 1, .1}, {QStringLiteral("\\Q30"), 1, 1, 30},
        {QStringLiteral("\\Q-20.5"), 1, 1, -20.5}, {QStringLiteral("\\Q+2e1"), 1, 1, 20},
        {QStringLiteral("\\C1"), 1, 1, 0, 1}, {QStringLiteral("\\C256"), 1, 1, 0, 256},
        {QStringLiteral("\\c16711680"), 1, 1, 0, 16711680, true},
        {QStringLiteral("\\c16777215"), 1, 1, 0, 16777215, true}
    };
    auto options = mtextOptions(); options.baseHeight = 4;
    const auto text = QString::fromUcs4(U"ب\U0001F600");
    for (const auto &example : examples) {
        for (const bool semicolon : {false, true}) {
            const QString prefix = example.command + (semicolon ? QStringLiteral(";") : QString());
            const auto document = Document::parse(prefix + text, options);
            INFO(document.source.toStdString());
            REQUIRE(document.paragraphs.size() == 1);
            const auto &paragraph = document.paragraphs[0];
            CHECK(paragraph.text == text);
            CHECK(document.diagnostics.empty());
            REQUIRE(paragraph.styleAt(0));
            CHECK(paragraph.styleAt(0)->heightFactor == example.height);
            CHECK(paragraph.styleAt(0)->widthFactor == example.width);
            CHECK(paragraph.styleAt(0)->obliqueDegrees == example.oblique);
            CHECK(paragraph.styleAt(0)->color == example.color);
            CHECK(paragraph.styleAt(0)->trueColor == example.rgb);
            REQUIRE(document.tokens.front().kind == Document::TokenKind::Format);
            CHECK(document.tokens.front().source == Range{0, int(prefix.size())});
            CHECK(paragraph.scalars.front().source == Range{int(prefix.size()), 1});
            checkProvenance(document);
        }
    }
}

TEST_CASE("Numeric lexing preserves adjacent text commands scopes and native stacks", "[textdocument][numeric]") {
    const auto document = Document::parse(
        QStringLiteral("A{\\H2xB\\W.5C\\Q30D\\C1E\\c16711680F}G;"), mtextOptions());
    const auto &paragraph = document.paragraphs.front();
    REQUIRE(paragraph.text == QStringLiteral("ABCDEFG;"));
    CHECK(document.diagnostics.empty());
    REQUIRE(paragraph.styleAt(1));
    REQUIRE(paragraph.styleAt(5));
    REQUIRE(paragraph.styleAt(6));
    CHECK(paragraph.styleAt(1)->heightFactor == 2);
    CHECK(paragraph.styleAt(2)->widthFactor == .5);
    CHECK(paragraph.styleAt(3)->obliqueDegrees == 30);
    CHECK(paragraph.styleAt(4)->color == 1);
    CHECK(paragraph.styleAt(5)->color == 16711680);
    CHECK(paragraph.styleAt(5)->trueColor);
    CHECK(paragraph.styleAt(6)->heightFactor == 1);
    CHECK(paragraph.styleAt(6)->widthFactor == 1);
    CHECK(paragraph.styleAt(6)->obliqueDegrees == 0);
    CHECK(paragraph.styleAt(6)->color == -1);
    checkProvenance(document);
    const auto adjacent = Document::parse(QStringLiteral("\\H2xA\\C1;B\\W.5\\Q30ج"), mtextOptions());
    REQUIRE(adjacent.paragraphs.front().text == QStringLiteral("ABج"));
    CHECK(adjacent.paragraphs.front().styleAt(0)->heightFactor == 2);
    CHECK(adjacent.paragraphs.front().styleAt(1)->color == 1);
    CHECK(adjacent.paragraphs.front().styleAt(2)->widthFactor == .5);
    CHECK(adjacent.paragraphs.front().styleAt(2)->obliqueDegrees == 30);
    checkProvenance(adjacent);
    const auto lostScope = Document::parse(QStringLiteral("{\\H2xA}B;"), mtextOptions());
    REQUIRE(lostScope.paragraphs.front().text == QStringLiteral("AB;"));
    CHECK(lostScope.diagnostics.empty());
    CHECK(lostScope.paragraphs.front().styleAt(0)->heightFactor == 2);
    CHECK(lostScope.paragraphs.front().styleAt(1)->heightFactor == 1);
    checkProvenance(lostScope);
    const auto stack = Document::parse(QStringLiteral("\\H2x{\\f{other}\\S{\\W.5ب}^\\C1ج;}Z"), mtextOptions());
    REQUIRE(stack.paragraphs.front().objects.size() == 1);
    const auto &object = stack.paragraphs.front().objects.front();
    CHECK(object.upper->paragraphs.front().text == QStringLiteral("ب"));
    CHECK(object.lower->paragraphs.front().text == QStringLiteral("ج"));
    CHECK(object.upper->paragraphs.front().styleAt(0)->font == QStringLiteral("other"));
    CHECK(object.upper->paragraphs.front().styleAt(0)->heightFactor == 2);
    CHECK(object.upper->paragraphs.front().styleAt(0)->widthFactor == .5);
    CHECK(object.lower->paragraphs.front().styleAt(0)->color == 1);
    CHECK(stack.paragraphs.front().styleAt(1)->font == mtextOptions().initialStyle.font);
    checkProvenance(stack);
}

TEST_CASE("Float exponent lookahead preserves ordinary e and E text", "[textdocument][numeric]") {
    for (const auto &word : {QStringLiteral("example"), QStringLiteral("Eraser"), QStringLiteral("eب"),
                             QString::fromUcs4(U"e\U0001F600")}) {
        const auto document = Document::parse(QStringLiteral("\\H2") + word, mtextOptions());
        CHECK(document.paragraphs.front().text == word);
        CHECK(document.diagnostics.empty());
        REQUIRE(document.paragraphs.front().styleAt(0));
        CHECK(document.paragraphs.front().styleAt(0)->heightFactor == 2);
        checkProvenance(document);
    }
    const auto exponent = Document::parse(QStringLiteral("\\H1e2B\\Q2E1x"), mtextOptions());
    REQUIRE(exponent.paragraphs.front().text == QStringLiteral("Bx"));
    CHECK(exponent.paragraphs.front().styleAt(0)->heightFactor == 100);
    CHECK(exponent.paragraphs.front().styleAt(1)->obliqueDegrees == 20);
    checkProvenance(exponent);
}

TEST_CASE("Incomplete numeric spellings stay literal and cannot swallow later commands", "[textdocument][numeric]") {
    for (const auto &malformed : {QStringLiteral("\\H+"), QStringLiteral("\\H."), QStringLiteral("\\H+."),
                                 QStringLiteral("\\H1e"), QStringLiteral("\\H1e+"), QStringLiteral("\\H1e-"),
                                 QStringLiteral("\\H1..5"), QStringLiteral("\\W2.3.4"), QStringLiteral("\\Q1e2.3"),
                                 QStringLiteral("\\C+5"), QStringLiteral("\\c-5"), QStringLiteral("\\H٢")}) {
        for (const bool semicolon : {false, true}) {
            const QString spelling = malformed + (semicolon ? QStringLiteral(";") : QString());
            const auto document = Document::parse(QStringLiteral("{") + spelling + QStringLiteral("}\\C2;ب"), mtextOptions());
            INFO(document.source.toStdString());
            CHECK(document.paragraphs.front().text == spelling + QStringLiteral("ب"));
            REQUIRE(document.paragraphs.front().styleAt(int(spelling.size())));
            CHECK(document.paragraphs.front().styleAt(int(spelling.size()))->heightFactor == 1);
            CHECK(document.paragraphs.front().styleAt(int(spelling.size()))->color == 2);
            CHECK_FALSE(document.diagnostics.empty());
            checkProvenance(document);
        }
    }
    const auto pair = QString::fromUcs4(U"\U0001F600");
    const auto unicode = Document::parse(QStringLiteral("\\H1e+") + pair + QStringLiteral("\\C2;ب"), mtextOptions());
    CHECK(unicode.paragraphs.front().text == QStringLiteral("\\H1e+") + pair + QStringLiteral("ب"));
    checkProvenance(unicode);
}

TEST_CASE("Incomplete unsigned exponents recognize only active caret whitespace", "[textdocument][numeric]") {
    auto options = mtextOptions(); options.decodeCaretCodes = true;
    options.initialStyle.heightFactor = 3;
    options.initialStyle.widthFactor = 4;
    options.initialStyle.obliqueDegrees = 12;
    for (const auto &command : {QStringLiteral("\\H1e"), QStringLiteral("\\W1E"), QStringLiteral("\\Q1e")}) {
        for (const char code : {'I', 'J', 'K', 'L', 'M'}) {
            const QString caret = QStringLiteral("^") + QChar(code);
            const auto document = Document::parse(command + caret + QStringLiteral("\\C2;ب"), options);
            INFO(document.source.toStdString());
            const bool paragraphBreak = code == 'J' || code == 'M'; // Existing native caret policy.
            REQUIRE(document.paragraphs.size() == (paragraphBreak ? 2 : 1));
            const auto &first = document.paragraphs.front();
            CHECK(first.text == command + (paragraphBreak ? QString() : QString(QChar(code - 64)) + QStringLiteral("ب")));
            REQUIRE(first.styleAt(0));
            CHECK(first.styleAt(0)->heightFactor == 3);
            CHECK(first.styleAt(0)->widthFactor == 4);
            CHECK(first.styleAt(0)->obliqueDegrees == 12);
            if (paragraphBreak) {
                CHECK(first.breakSource == Range{int(command.size()), 2});
                CHECK(document.paragraphs.back().text == QStringLiteral("ب"));
            }
            const auto &last = document.paragraphs.back();
            REQUIRE(last.styleAt(int(last.text.size()) - 1));
            CHECK(last.styleAt(int(last.text.size()) - 1)->color == 2);
            REQUIRE(document.tokens.front().kind == Document::TokenKind::Malformed);
            CHECK(document.tokens.front().source == Range{0, int(command.size())});
            REQUIRE(document.diagnostics.size() == 1);
            checkProvenance(document);
        }
        const auto escaped = Document::parse(command + QStringLiteral("\\^Jب"), options);
        REQUIRE(escaped.paragraphs.size() == 1);
        CHECK(escaped.paragraphs.front().text == command + QStringLiteral("^Jب"));
        CHECK(escaped.paragraphs.front().styleAt(0)->heightFactor == 3);
        CHECK(escaped.paragraphs.front().styleAt(0)->widthFactor == 4);
        CHECK(escaped.paragraphs.front().styleAt(0)->obliqueDegrees == 12);
        checkProvenance(escaped);
    }
    options.decodeCaretCodes = false;
    const auto literal = Document::parse(QStringLiteral("\\H2e^Jب"), options);
    REQUIRE(literal.paragraphs.size() == 1);
    CHECK(literal.paragraphs.front().text == QStringLiteral("e^Jب"));
    CHECK(literal.paragraphs.front().styleAt(0)->heightFactor == 2);
    CHECK(literal.diagnostics.empty());
    checkProvenance(literal);
}

TEST_CASE("Numeric recovery has exact EOF brace command and Unicode boundaries", "[textdocument][numeric]") {
    for (const auto &command : {QStringLiteral("\\H"), QStringLiteral("\\W"), QStringLiteral("\\Q"),
                               QStringLiteral("\\C"), QStringLiteral("\\c"), QStringLiteral("\\H;"),
                               QStringLiteral("\\C;"), QStringLiteral("\\Hbad"),
                               QStringLiteral("\\H+"), QStringLiteral("\\H."), QStringLiteral("\\H1e"),
                               QStringLiteral("\\W1E+"), QStringLiteral("\\Q1e-"), QStringLiteral("\\H1..5")}) {
        const auto eof = Document::parse(command, mtextOptions());
        CHECK(eof.paragraphs.front().text == command);
        REQUIRE(eof.diagnostics.size() == 1);
        checkProvenance(eof);
        const auto next = Document::parse(command + QStringLiteral("\\C2;ب"), mtextOptions());
        CHECK(next.paragraphs.front().text == command + QStringLiteral("ب"));
        REQUIRE(next.paragraphs.front().styleAt(int(command.size())));
        CHECK(next.paragraphs.front().styleAt(int(command.size()))->color == 2);
        CHECK(next.paragraphs.front().styleAt(0)->heightFactor == 1);
        checkProvenance(next);
    }
    const auto opening = Document::parse(QStringLiteral("\\H1e{\\C2;ب}ج"), mtextOptions());
    REQUIRE(opening.paragraphs.front().text == QStringLiteral("\\H1eبج"));
    CHECK(opening.paragraphs.front().styleAt(4)->color == 2);
    CHECK(opening.paragraphs.front().styleAt(5)->color == -1);
    REQUIRE(opening.tokens.size() >= 2);
    CHECK(opening.tokens[1].kind == Document::TokenKind::ScopeOpen);
    checkProvenance(opening);
    auto options = mtextOptions(); options.decodeDxfEscapes = true;
    const auto unicodeBreak = Document::parse(QStringLiteral("\\H1e\\U+000Aب"), options);
    REQUIRE(unicodeBreak.paragraphs.size() == 2);
    CHECK(unicodeBreak.paragraphs.front().text == QStringLiteral("\\H1e"));
    CHECK(unicodeBreak.paragraphs.front().breakSource == Range{4, 7});
    CHECK(unicodeBreak.paragraphs.back().text == QStringLiteral("ب"));
    checkProvenance(unicodeBreak);
    const auto nonAscii = QString::fromUcs4(U"\\H\U0001D7D9ب");
    const auto scalar = Document::parse(nonAscii, mtextOptions());
    CHECK(scalar.paragraphs.front().text == nonAscii);
    CHECK(scalar.paragraphs.front().scalars[2].source == Range{2, 2});
    checkProvenance(scalar);
}

TEST_CASE("Numeric range failures preserve surrounding layout and existing style policy", "[textdocument][numeric]") {
    for (const auto &command : {QStringLiteral("\\H0"), QStringLiteral("\\H-2"), QStringLiteral("\\W0"),
                               QStringLiteral("\\W-1"), QStringLiteral("\\Q90"), QStringLiteral("\\Q-90"),
                               QStringLiteral("\\C257"), QStringLiteral("\\c16777216"),
                               QStringLiteral("\\C4294967296"), QStringLiteral("\\H1e9999"),
                               QStringLiteral("\\H1e-9999")}) {
        for (const bool semicolon : {false, true}) {
            const auto document = Document::parse(QStringLiteral("A") + command +
                (semicolon ? QStringLiteral(";") : QString()) + QStringLiteral("ب\\C2;ج"), mtextOptions());
            INFO(document.source.toStdString());
            const auto &paragraph = document.paragraphs.front();
            CHECK(paragraph.text == QStringLiteral("Aبج"));
            REQUIRE(paragraph.styleAt(1));
            CHECK(paragraph.styleAt(1)->heightFactor == 1);
            CHECK(paragraph.styleAt(1)->widthFactor == 1);
            CHECK(paragraph.styleAt(1)->obliqueDegrees == 0);
            CHECK(paragraph.styleAt(1)->color == -1);
            CHECK(paragraph.styleAt(2)->color == 2);
            REQUIRE(document.diagnostics.size() == 1);
            CHECK(document.diagnostics.front().code == Document::Diagnostic::Code::MalformedEscape);
            checkProvenance(document);
        }
    }
    auto options = mtextOptions(); options.initialStyle.heightFactor = 1e308;
    CHECK_FALSE(Document::parse(QStringLiteral("\\H2xب"), options).ok());
}

TEST_CASE("Numeric scans remain bounded by paragraphs and huge ASCII arguments", "[textdocument][numeric][limits]") {
    for (const auto &separator : {QStringLiteral("\n"), QStringLiteral("\r"), QStringLiteral("\u2028"),
                                  QStringLiteral("\u2029"), QStringLiteral("\\P"), QStringLiteral("^J")}) {
        auto options = mtextOptions(); options.decodeCaretCodes = true;
        for (const auto &prefix : {QStringLiteral("\\H2xA"), QStringLiteral("\\H1e+"), QStringLiteral("\\C1ب")}) {
            const auto document = Document::parse(prefix + separator + QStringLiteral("ج;د"), options);
            REQUIRE(document.paragraphs.size() == 2);
            CHECK(document.paragraphs.back().text == QStringLiteral("ج;د"));
            checkProvenance(document);
        }
    }
    const auto longArguments = Document::parse(QStringLiteral("\\C") + QString(100000, QChar('0')) +
        QStringLiteral("1ب\\H") + QString(100000, QChar('9')) + QStringLiteral("A\\C2;B"), mtextOptions());
    REQUIRE(longArguments.paragraphs.front().text == QStringLiteral("بAB"));
    CHECK(longArguments.paragraphs.front().styleAt(0)->color == 1);
    CHECK(longArguments.paragraphs.front().styleAt(1)->heightFactor == 1);
    CHECK(longArguments.paragraphs.front().styleAt(2)->color == 2);
    REQUIRE(longArguments.diagnostics.size() == 1);
    checkProvenance(longArguments);
    auto options = mtextOptions(); options.maxDiagnostics = 5000;
    const auto repeated = Document::parse(QStringLiteral("\\H1e+").repeated(4096) + QStringLiteral("\\C2;ب"), options);
    CHECK(repeated.paragraphs.front().text == QStringLiteral("\\H1e+").repeated(4096) + QStringLiteral("ب"));
    CHECK(repeated.diagnostics.size() == 4096);
    checkProvenance(repeated);
}

TEST_CASE("Standard and native font directives preserve exact metadata", "[textdocument]") {
    const auto document = Document::parse(QStringLiteral("\\fAmiri|b0|i0;ب\\F{}ج"), mtextOptions());
    const auto &paragraph = document.paragraphs.front();
    CHECK(paragraph.text == QStringLiteral("بج"));
    CHECK(paragraph.styleAt(0)->font == QStringLiteral("Amiri"));
    CHECK(paragraph.styleAt(0)->fontSpecification == QStringLiteral("Amiri|b0|i0"));
    CHECK(paragraph.styleAt(1)->font == QStringLiteral("standard"));
    REQUIRE(document.diagnostics.size() == 1);
    CHECK(document.diagnostics.front().code == Document::Diagnostic::Code::UnsupportedFormat);
    checkProvenance(document);
}

TEST_CASE("Stack children inherit effective inline font and global source ranges", "[textdocument]") {
    const QString source = QStringLiteral("A\\f{other}\\Sبَ^ ج;Z");
    const auto document = Document::parse(source, mtextOptions());
    const auto &paragraph = document.paragraphs.front();
    CHECK(paragraph.text == QStringLiteral("A\ufffcZ"));
    REQUIRE(paragraph.objects.size() == 1);
    const auto &object = paragraph.objects.front();
    CHECK(object.separator == '^');
    CHECK(object.upper->paragraphs.front().text == QStringLiteral("بَ"));
    CHECK(object.lower->paragraphs.front().text == QStringLiteral("ج"));
    CHECK(object.upper->paragraphs.front().styleAt(0)->font == QStringLiteral("other"));
    CHECK(object.lower->paragraphs.front().styleAt(0)->font == QStringLiteral("other"));
    CHECK(object.upper->source == source);
    CHECK(object.upper->sourceRange.start == source.indexOf(QStringLiteral("ب")));
    CHECK(paragraph.sourceRanges(1, 1) == std::vector<Range>{object.source});
    checkProvenance(document);
}

TEST_CASE("Unknown and invalid formatting retain source without leaking state", "[textdocument]") {
    const QString source = QStringLiteral("A{\\Zopaque;\\H-2;\\C999;B}C\\");
    const auto document = Document::parse(source, mtextOptions());
    CHECK(document.source == source);
    CHECK(document.paragraphs.front().text == QStringLiteral("ABC\\"));
    CHECK(document.paragraphs.front().styleAt(1)->heightFactor == 1);
    CHECK(document.paragraphs.front().styleAt(1)->color == -1);
    REQUIRE(document.diagnostics.size() == 4);
    CHECK(document.tokens[2].kind == Document::TokenKind::Unknown);
    checkProvenance(document);
    for (const auto &malformed : {QStringLiteral("}ب{"), QStringLiteral("\\Sbad;"), QStringLiteral("\\Hbad")})
        checkProvenance(Document::parse(malformed, mtextOptions()));
}

TEST_CASE("DXF escape decoding is explicit and preserves surrogate provenance", "[textdocument]") {
    const QString source = QStringLiteral("\\U+D83D\\U+DE00%%C^I^ %%d%%p");
    CHECK(Document::plain(source).paragraphs.front().text == source);
    auto options = mtextOptions();
    options.decodeCaretCodes = true;
    options.decodeDxfEscapes = true;
    const auto document = Document::parse(source, options);
    const auto &paragraph = document.paragraphs.front();
    CHECK(paragraph.text == QString(QChar(0xd83d)) + QChar(0xde00) + QStringLiteral("⌀\t^°±"));
    CHECK(paragraph.sourceRanges(0, 2) == std::vector<Range>{{0, 14}});
    checkProvenance(document);
    const auto malformed = Document::parse(QStringLiteral("\\U+D800"), options);
    CHECK(malformed.paragraphs.front().text == QStringLiteral("\ufffd"));
    CHECK(malformed.source == QStringLiteral("\\U+D800"));
    checkProvenance(malformed);
    for (const auto &literal : {QStringLiteral("^ب"), QString::fromUcs4(U"^\U0001F600"), QStringLiteral("^\u2028")}) {
        const auto nonAscii = Document::parse(literal, options);
        CHECK(nonAscii.source == literal);
        if (literal.endsWith(QChar(0x2028))) {
            REQUIRE(nonAscii.paragraphs.size() == 2);
            CHECK(nonAscii.paragraphs.front().text == QStringLiteral("^"));
        } else {
            CHECK(nonAscii.paragraphs.front().text == literal);
            REQUIRE(nonAscii.diagnostics.size() == 1);
            CHECK(nonAscii.diagnostics.front().source.length == literal.size());
        }
        checkProvenance(nonAscii);
    }
    for (const auto &spelling : {QStringLiteral("\\U+000A"), QStringLiteral("\\U+000D\\U+000A"), QStringLiteral("^M^J")}) {
        const auto lines = Document::parse(QStringLiteral("ب") + spelling + QStringLiteral("ج"), options);
        REQUIRE(lines.paragraphs.size() == 2);
        CHECK(lines.paragraphs[0].text == QStringLiteral("ب"));
        CHECK(lines.paragraphs[0].breakSource.length == spelling.size());
        CHECK(lines.paragraphs[1].text == QStringLiteral("ج"));
        checkProvenance(lines);
    }
}

TEST_CASE("Malformed commands cannot split valid scalars or consume another paragraph", "[textdocument]") {
    const QString pair = QString(QChar(0xd83d)) + QChar(0xde00);
    const auto scalarDocument = Document::parse(QStringLiteral("\\") + pair, mtextOptions());
    CHECK(scalarDocument.paragraphs.front().text == QStringLiteral("\\") + pair);
    checkProvenance(scalarDocument);
    for (const auto separator : {QChar(0x2028), QChar(0x2029), QChar('\n'), QChar('\r')}) {
        for (const QString &prefix : {QStringLiteral("\\Hbad"), QStringLiteral("\\f{bad"), QStringLiteral("\\"), QStringLiteral("^")}) {
            const QString source = prefix + separator + QStringLiteral("ب;ج");
            const auto document = Document::parse(source, mtextOptions());
            REQUIRE(document.paragraphs.size() == 2);
            CHECK(document.paragraphs.back().text == QStringLiteral("ب;ج"));
            checkProvenance(document);
            auto caretOptions = mtextOptions(); caretOptions.decodeCaretCodes = true;
            const auto caretDocument = Document::parse(source, caretOptions);
            REQUIRE(caretDocument.paragraphs.size() == 2);
            checkProvenance(caretDocument);
        }
    }
    auto caretOptions = mtextOptions(); caretOptions.decodeCaretCodes = true;
    for (const auto &prefix : {QStringLiteral("\\Hbad"), QStringLiteral("\\f{bad")}) {
        const auto document = Document::parse(prefix + QStringLiteral("^Jب;ج"), caretOptions);
        REQUIRE(document.paragraphs.size() == 2);
        CHECK(document.paragraphs.back().text == QStringLiteral("ب;ج"));
        checkProvenance(document);
    }
}

TEST_CASE("Formatted stacks preserve scoped children and propagate fatal errors", "[textdocument]") {
    const QString source = QStringLiteral("\\S{\\C1;بَ}^ج;");
    const auto document = Document::parse(source, mtextOptions());
    REQUIRE(document.paragraphs.front().objects.size() == 1);
    const auto &object = document.paragraphs.front().objects.front();
    CHECK(object.upper->paragraphs.front().text == QStringLiteral("بَ"));
    CHECK(object.upper->paragraphs.front().styleAt(0)->color == 1);
    CHECK(object.lower->paragraphs.front().styleAt(0)->color == -1);
    checkProvenance(document);
    auto options = mtextOptions(); options.maxScopeDepth = 0;
    CHECK_FALSE(Document::parse(source, options).ok());
}

TEST_CASE("Physical CRLF cannot coalesce a preceding independent CR boundary", "[textdocument][paragraph][crcrlf]") {
    struct Fixture { QString source; std::vector<Range> breaks; };
    const std::vector<Fixture> fixtures{
        {QStringLiteral("A\r\r\nB"), {{1, 1}, {2, 2}}},
        {QStringLiteral("A^M\r\nB"), {{1, 2}, {3, 2}}},
        {QStringLiteral("A\r^M\nB"), {{1, 1}, {2, 3}}},
        {QStringLiteral("A\r\r^JB"), {{1, 1}, {2, 3}}},
        {QStringLiteral("A^M^M^JB"), {{1, 2}, {3, 4}}},
        {QStringLiteral("A\r\n^JB"), {{1, 2}, {3, 2}}},
        {QStringLiteral("A^M\n^JB"), {{1, 3}, {4, 2}}},
        {QStringLiteral("A\r^J\r\nB"), {{1, 3}, {4, 2}}},
        {QStringLiteral("A\r\n^MB"), {{1, 2}, {3, 2}}},
        {QStringLiteral("A^M\nB"), {{1, 3}}},
        {QStringLiteral("A\r^JB"), {{1, 3}}},
        {QStringLiteral("A^M^JB"), {{1, 4}}},
        {QStringLiteral("A\r\nB"), {{1, 2}}},
        {QStringLiteral("A\\U+000D\r\nB"), {{1, 7}, {8, 2}}},
        {QStringLiteral("A\r\\U+000D\\U+000AB"), {{1, 1}, {2, 14}}},
        {QStringLiteral("A\\U+000D^JB"), {{1, 9}}},
        {QStringLiteral("A^M\\U+000AB"), {{1, 9}}}
    };
    auto options = mtextOptions();
    options.decodeCaretCodes = true;
    options.decodeDxfEscapes = true;
    for (const auto &fixture : fixtures) {
        INFO("Source UTF8 hex " << fixture.source.toUtf8().toHex().toStdString());
        const auto document = Document::parse(fixture.source, options);
        CHECK(document.source == fixture.source);
        REQUIRE(document.paragraphs.size() == fixture.breaks.size() + 1);
        CHECK(document.paragraphs.front().text == QStringLiteral("A"));
        CHECK(document.paragraphs.back().text == QStringLiteral("B"));
        std::vector<Range> tokens;
        for (const auto &token : document.tokens)
            if (token.kind == Document::TokenKind::ParagraphBreak) tokens.push_back(token.source);
        CHECK(tokens == fixture.breaks);
        int start = 0;
        for (std::size_t index = 0; index < fixture.breaks.size(); ++index) {
            CHECK(document.paragraphs[index].breakSource == fixture.breaks[index]);
            CHECK(document.paragraphs[index].source == Range{start, fixture.breaks[index].start - start});
            if (index > 0) CHECK(document.paragraphs[index].text.isEmpty());
            start = fixture.breaks[index].end();
        }
        CHECK(document.paragraphs.back().source == Range{start, 1});
        checkProvenance(document);
    }
}

TEST_CASE("Parser limits fail explicitly without truncating saved text", "[textdocument]") {
    auto options = mtextOptions();
    options.maxSourceUnits = 2;
    auto document = Document::parse(QStringLiteral("long"), options);
    CHECK_FALSE(document.ok());
    CHECK(document.source == QStringLiteral("long"));
    options = mtextOptions(); options.maxScopeDepth = 1;
    CHECK_FALSE(Document::parse(QStringLiteral("{{ب}}"), options).ok());
    options = mtextOptions(); options.maxStackDepth = 0;
    CHECK_FALSE(Document::parse(QStringLiteral("\\Sب^ج;"), options).ok());
    options = mtextOptions(); options.baseHeight = 0;
    CHECK_FALSE(Document::parse(QStringLiteral("ب"), options).ok());
    options = mtextOptions(); options.initialStyle.heightFactor = std::numeric_limits<double>::infinity();
    CHECK_FALSE(Document::parse(QStringLiteral("ب"), options).ok());
    options = mtextOptions(); options.maxDiagnostics = 4;
    document = Document::parse(QString(10000, QChar(0xd800)), options);
    CHECK_FALSE(document.ok());
    CHECK(document.source.size() == 10000);
    CHECK(document.diagnostics.size() == 5);
    options = mtextOptions(); options.maxDiagnostics = 100000;
    CHECK(Document::parse(QStringLiteral("\\X").repeated(20000), options).ok());
}

TEST_CASE("Seeded malformed and scoped inputs retain complete source provenance", "[textdocument][property]") {
    std::mt19937 random(0x06280644);
    const ushort units[] = {'A', 'B', ' ', '\t', '\n', '\\', '{', '}', ';', '^', 'S', 'H', 'f', 'C', '2',
                             0x628, 0x64e, 0x200d, 0x200c, 0x2067, 0x2069, 0xfeff, 0xd83d, 0xde00};
    for (int sample = 0; sample < 200; ++sample) {
        QString source;
        for (int i = 0; i < 80; ++i) source += QChar(units[random() % (sizeof(units) / sizeof(units[0]))]);
        auto options = mtextOptions();
        options.decodeDxfEscapes = sample % 2;
        const auto document = Document::parse(source, options);
        CHECK(document.source == source);
        checkProvenance(document);
    }
}

TEST_CASE("U+FEFF is a scalar of the text and not a byte order mark", "[textdocument]") {
    const QChar mark(0xfeff);
    const QString sources[] = {QString(mark), QString(3, mark), QStringLiteral("a") + mark + QStringLiteral("b"),
                               QString(mark) + QStringLiteral("ب") + mark};
    for (const QString &source : sources) {
        for (const auto syntax : {Document::Syntax::Plain, Document::Syntax::MText}) {
            auto options = mtextOptions();
            options.syntax = syntax;
            const auto document = Document::parse(source, options);
            REQUIRE(document.paragraphs.size() == 1);
            CHECK(document.paragraphs.front().text == source);
            CHECK(document.paragraphs.front().scalars.size() == std::size_t(source.size()));
            CHECK(document.diagnostics.empty());
            checkProvenance(document);
        }
    }
    auto options = mtextOptions();
    options.decodeDxfEscapes = true;
    // Escaped in the text, literal and escaped in the two parts of a stack.
    const QString source = QStringLiteral("a\\U+FEFFb\\S") + mark + QStringLiteral("^\\U+FEFF;");
    const auto document = Document::parse(source, options);
    REQUIRE(document.paragraphs.size() == 1);
    const auto &paragraph = document.paragraphs.front();
    CHECK(paragraph.text == QStringLiteral("a") + mark + QStringLiteral("b\ufffc"));
    REQUIRE(paragraph.scalars.size() == 4);
    CHECK(paragraph.scalars[1].value == 0xfeff);
    CHECK(paragraph.scalars[1].text == Range{1, 1});
    CHECK(paragraph.scalars[1].source == Range{1, 7});
    REQUIRE(paragraph.objects.size() == 1);
    CHECK(paragraph.objects.front().upper->paragraphs.front().text == QString(mark));
    CHECK(paragraph.objects.front().lower->paragraphs.front().text == QString(mark));
    checkProvenance(document);
    checkProvenance(*paragraph.objects.front().upper);
    checkProvenance(*paragraph.objects.front().lower);
}

TEST_CASE("Styles are equal by value, whichever buffers hold their text", "[textdocument]") {
    Document::Style first, second;
    for (QString Document::Style::*member : {&Document::Style::font, &Document::Style::fontSpecification}) {
        first.*member = QString(40, QChar('F'));
        second.*member = QString(40, QChar('F'));
        REQUIRE((first.*member).constData() != (second.*member).constData());
        CHECK(first == second);
        (second.*member)[39] = QChar('G');
        CHECK_FALSE(first == second);
        second.*member = QString(39, QChar('F'));
        CHECK_FALSE(first == second);
        second.*member = first.*member;
        CHECK(first == second);
    }
    first.language = QByteArray(8, 'a');
    second.language = QByteArray(8, 'a');
    REQUIRE(first.language.constData() != second.language.constData());
    CHECK(first == second);
    second.language[7] = 'b';
    CHECK_FALSE(first == second);
    second.language = first.language;
    first.font = QString();
    second.font = QStringLiteral("");
    CHECK(first == second);
    for (const int field : {0, 1, 2, 3, 4, 5, 6, 7}) {
        auto other = first;
        switch (field) {
        case 0: other.heightFactor = 2; break;
        case 1: other.widthFactor = 2; break;
        case 2: other.obliqueDegrees = 2; break;
        case 3: other.color = 2; break;
        case 4: other.trueColor = true; break;
        case 5: other.underline = true; break;
        case 6: other.overline = true; break;
        default: other.strikeThrough = true; break;
        }
        INFO(field);
        CHECK_FALSE(first == other);
    }
}

TEST_CASE("A font command that repeats the name continues the span", "[textdocument]") {
    const QString name(64, QChar('F'));
    const QString other = name.left(63) + QStringLiteral("G");
    // abc: the name once more after a. d: underlined in a scope. e: the scope
    // has closed. f: a name of the same length that differs in its last letter.
    const QString source = QStringLiteral("\\f") + name + QStringLiteral(";a\\f") + name + QStringLiteral(";bc{\\Ld}e\\f") +
                           other + QStringLiteral(";f");
    const auto document = Document::parse(source, mtextOptions());
    REQUIRE(document.paragraphs.size() == 1);
    const auto &paragraph = document.paragraphs.front();
    CHECK(paragraph.text == QStringLiteral("abcdef"));
    REQUIRE(paragraph.spans.size() == 4);
    CHECK(paragraph.spans[0].text == Range{0, 3});
    CHECK(paragraph.spans[1].text == Range{3, 1});
    CHECK(paragraph.spans[2].text == Range{4, 1});
    CHECK(paragraph.spans[3].text == Range{5, 1});
    for (int span = 0; span < 3; ++span) {
        CHECK(paragraph.spans[span].style.font == name);
        CHECK(paragraph.spans[span].style.fontSpecification == name);
        CHECK(paragraph.spans[span].style.underline == (span == 1));
    }
    CHECK(paragraph.spans[3].style.font == other);
    CHECK(document.diagnostics.empty());
    checkProvenance(document);
}
