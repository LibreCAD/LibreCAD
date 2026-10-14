/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textruns.h"
#include <catch2/catch_test_macros.hpp>
#include <QList>
#include <limits>
#include <random>

namespace {
using Runs = LC_TextRuns;
using Document = LC_TextDocument;
using Range = Document::Range;
Document::Options mtextOptions() {
    Document::Options options;
    options.syntax = Document::Syntax::MText;
    options.initialStyle.font = QStringLiteral("lc-amiri-1.003-ot-v1");
    return options;
}
Runs::Paragraph plain(const QString &text, Qt::LayoutDirection direction = Qt::LayoutDirectionAuto) {
    Runs::Options options;
    options.direction = direction;
    return Runs::itemize(Document::plain(text).paragraphs.front(), options);
}
const Runs::Run &at(const Runs::Paragraph &paragraph, int position) {
    REQUIRE(position >= 0);
    REQUIRE(position < int(paragraph.runForUnit.size()));
    const int index = paragraph.runForUnit[position];
    REQUIRE(index >= 0);
    REQUIRE(index < int(paragraph.runs.size()));
    return paragraph.runs[index];
}
void checkInvariants(const Document::Paragraph &source, const Runs::Paragraph &paragraph) {
    INFO(paragraph.error.toStdString());
    REQUIRE(paragraph.ok());
    CHECK(paragraph.text == source.text);
    CHECK(paragraph.source == source.source);
    CHECK(paragraph.breakSource == source.breakSource);
    CHECK(paragraph.unicodeVersion == QChar::currentUnicodeVersion());
    CHECK(paragraph.sourceMap.size() == std::size_t(source.text.size()));
    CHECK(paragraph.runForUnit.size() == std::size_t(source.text.size()));
    CHECK(paragraph.bidi.shapingLevels.size() == std::size_t(source.text.size()));
    CHECK(paragraph.bidi.displayLevels.size() == std::size_t(source.text.size()));
    int cursor = 0;
    for (const auto &context : paragraph.contexts) {
        CHECK(context.text.start == cursor);
        REQUIRE(context.text.length > 0);
        CHECK(context.text.end() <= source.text.size());
        CHECK(context.shapingText.start <= context.text.start);
        CHECK(context.shapingText.end() >= context.text.end());
        for (int i = context.shapingText.start; i < context.shapingText.end(); ++i) {
            if (paragraph.bidi.shapingLevels[i] >= 0)
                CHECK(context.level == paragraph.bidi.shapingLevels[i]);
            if (i > context.shapingText.start) CHECK_FALSE(paragraph.bidi.contextBreaks[i]);
        }
        for (int i = context.text.start; i < context.text.end(); ++i) {
            if (paragraph.bidi.shapingLevels[i] >= 0)
                CHECK(context.level == paragraph.bidi.shapingLevels[i]);
            if (i > context.text.start) CHECK_FALSE(paragraph.bidi.contextBreaks[i]);
        }
        cursor = context.text.end();
    }
    CHECK(cursor == source.text.size());
    cursor = 0;
    for (int index = 0; index < int(paragraph.runs.size()); ++index) {
        const auto &run = paragraph.runs[index];
        CHECK(run.text.start == cursor);
        REQUIRE(run.text.length > 0);
        CHECK(run.text.start >= run.context.start);
        CHECK(run.text.end() <= run.context.end());
        REQUIRE(run.contextIndex >= 0);
        REQUIRE(run.contextIndex < int(paragraph.contexts.size()));
        CHECK(run.context == paragraph.contexts[run.contextIndex].shapingText);
        CHECK(run.level == paragraph.contexts[run.contextIndex].level);
        CHECK(run.sources == source.sourceRanges(run.text.start, run.text.length));
        CHECK(paragraph.sourceRanges(run.text.start, run.text.length) == run.sources);
        if (run.representativeScalar)
            CHECK(QChar::script(run.representativeScalar) == run.script);
        for (int i = run.text.start; i < run.text.end(); ++i) CHECK(paragraph.runForUnit[i] == index);
        if (run.text.start > 0)
            CHECK_FALSE(source.text.at(run.text.start).isLowSurrogate());
        cursor = run.text.end();
    }
    CHECK(cursor == source.text.size());
    for (const auto &scalar : source.scalars) {
        for (int i = scalar.text.start; i < scalar.text.end(); ++i)
            CHECK(paragraph.sourceMap[i] == scalar.source);
        CHECK(at(paragraph, scalar.text.start).text.end() >= scalar.text.end());
    }
    CHECK(paragraph.projectedSourceRanges(0, int(paragraph.bidi.projection.text.size())) ==
          source.sourceRanges(0, int(source.text.size())));
}
} // namespace

TEST_CASE("Trailing joining controls retain Arabic pre-L1 context", "[textruns][bidi]") {
    for (const auto control : {0x200d, 0x200c}) {
        const auto source = Document::plain(QStringLiteral("ب") + QChar(control));
        Runs::Options options;
        options.direction = Qt::LeftToRight;
        const auto paragraph = Runs::itemize(source.paragraphs.front(), options);
        checkInvariants(source.paragraphs.front(), paragraph);
        REQUIRE(paragraph.runs.size() == 1);
        CHECK(paragraph.runs[0].text == Range{0, 2});
        CHECK(paragraph.runs[0].level == 1);
        CHECK(paragraph.runs[0].script == QChar::Script_Arabic);
        CHECK(paragraph.bidi.shapingLevels[1] == -1);
        CHECK(paragraph.bidi.displayLevels[1] == 0);
        CHECK(paragraph.runs[0].beginsContext());
        CHECK(paragraph.runs[0].endsContext());
    }
    const auto leading = plain(QStringLiteral("\u200d\u200cب"), Qt::LeftToRight);
    REQUIRE(leading.runs.size() == 1);
    CHECK(leading.runs.front().level == 1);
    CHECK(leading.runs.front().text == Range{0, 3});
}

TEST_CASE("Joining controls affect both neighbors without borrowing other-level letters", "[textruns][bidi]") {
    for (const auto control : {0x200d, 0x200c}) {
        for (const auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
            for (const bool arabicFirst : {false, true}) {
                const QString text = (arabicFirst ? QStringLiteral("ب") : QStringLiteral("A")) +
                    QChar(control) + (arabicFirst ? QStringLiteral("A") : QStringLiteral("ب"));
                const auto document = Document::plain(text);
                Runs::Options options;
                options.direction = direction;
                const auto paragraph = Runs::itemize(document.paragraphs[0], options);
                checkInvariants(document.paragraphs[0], paragraph);
                REQUIRE(paragraph.contexts.size() == 2);
                CHECK(at(paragraph, 0).context == Range{0, 2});
                CHECK(at(paragraph, 2).context == Range{1, 2});
                CHECK(at(paragraph, 0).level != at(paragraph, 2).level);
                CHECK(at(paragraph, 1).text.start == 0); // Owned once, by preceding run.
                CHECK(at(paragraph, 2).text == Range{2, 1});
                CHECK_FALSE(at(paragraph, 2).beginsContext());
                CHECK(at(paragraph, 2).endsContext());
                CHECK(paragraph.contexts[1].text == Range{2, 1});
                CHECK(paragraph.contexts[1].shapingText == Range{1, 2});
            }
        }
    }
    const auto sequences = plain(QStringLiteral("A\u200d\u200c\u200dب\u200cA"), Qt::LeftToRight);
    CHECK(at(sequences, 4).context == Range{1, 5});
    CHECK(at(sequences, 6).context == Range{5, 2});
    CHECK(at(sequences, 4).text == Range{4, 2});
}

TEST_CASE("Borrowed joining context stops at hard boundaries and retains formatting", "[textruns][bidi][format]") {
    for (const auto &text : {QStringLiteral("A\u200d\u2067ب\u2069"),
                            QStringLiteral("A\u200d\u2067\u2066\u2069ب\u2069"),
                            QStringLiteral("A\u200d\tب"), QStringLiteral("A\u200d\ufffcب")}) {
        const auto document = Document::plain(text);
        const auto paragraph = Runs::itemize(document.paragraphs[0]);
        checkInvariants(document.paragraphs[0], paragraph);
        const int beh = int(text.indexOf(QChar(0x0628)));
        CHECK(at(paragraph, beh).context.start == beh);
        CHECK(at(paragraph, beh).beginsContext());
    }
    auto options = mtextOptions();
    const auto document = Document::parse(QStringLiteral("A\u200d{\\f{other}\\H2x;ب}\\C1;\u200cA"), options);
    const auto paragraph = Runs::itemize(document.paragraphs[0]);
    checkInvariants(document.paragraphs[0], paragraph);
    CHECK(at(paragraph, 2).style.font == QStringLiteral("other"));
    CHECK(at(paragraph, 2).style.heightFactor == 2);
    CHECK(at(paragraph, 2).context == Range{1, 3});
    CHECK(at(paragraph, 4).context == Range{3, 2});
    const auto paragraphs = Document::plain(QStringLiteral("A\u200d\nب"));
    REQUIRE(paragraphs.paragraphs.size() == 2);
    const auto afterBreak = Runs::itemize(paragraphs.paragraphs[1]);
    CHECK(at(afterBreak, 0).context == Range{0, 1});
}

TEST_CASE("Empty and nested isolates terminate otherwise equal joining levels", "[textruns][bidi]") {
    for (const auto &text : {QStringLiteral("ب\u2066\u2069ب"),
                            QStringLiteral("ب\u2067ب\u2066\u2069ب\u2069ب"),
                            QStringLiteral("ب\u2068ب\u2069ب"),
                            QStringLiteral("ب\u2069ب"), QStringLiteral("ب\u2067ب")}) {
        const auto source = Document::plain(text);
        const auto paragraph = Runs::itemize(source.paragraphs.front());
        checkInvariants(source.paragraphs.front(), paragraph);
        for (int i = 0; i < text.size(); ++i) {
            const auto direction = text.at(i).direction();
            if (direction != QChar::DirLRI && direction != QChar::DirRLI && direction != QChar::DirPDI) continue;
            const auto &run = at(paragraph, i);
            CHECK(run.kind == Runs::Kind::BidiBoundary);
            CHECK(run.context == Range{i, 1});
            CHECK_FALSE(paragraph.contexts[run.contextIndex].shapeable);
            if (i > 0) CHECK(at(paragraph, i - 1).contextIndex != run.contextIndex);
            if (i + 1 < text.size()) CHECK(at(paragraph, i + 1).contextIndex != run.contextIndex);
        }
    }
    const auto empty = plain(QStringLiteral("ب\u2066\u2069ب"));
    REQUIRE(empty.contexts.size() == 4);
    CHECK(at(empty, 0).level == at(empty, 3).level);
    CHECK(at(empty, 0).contextIndex != at(empty, 3).contextIndex);
}

TEST_CASE("Explicit embeddings keep excluded controls without crossing level changes", "[textruns][bidi]") {
    const auto document = Document::plain(QStringLiteral("ب\u202b\u202bب\u202c\u202cب\u200dَ"));
    Runs::Options options;
    options.direction = Qt::LeftToRight;
    const auto paragraph = Runs::itemize(document.paragraphs[0], options);
    checkInvariants(document.paragraphs[0], paragraph);
    REQUIRE(paragraph.contexts.size() == 3);
    CHECK(at(paragraph, 0).context == Range{0, 3});
    CHECK(at(paragraph, 3).context == Range{1, 5}); // Borrow embedding controls, not beh at 0.
    CHECK(at(paragraph, 6).context == Range{4, 5}); // Borrow PDFs, not beh at 3.
    CHECK(paragraph.contexts[1].text == Range{3, 3});
    CHECK(paragraph.contexts[2].text == Range{6, 3});
    CHECK(at(paragraph, 0).level == 1);
    CHECK(at(paragraph, 3).level == 3);
    CHECK(at(paragraph, 6).level == 1);
    CHECK(paragraph.bidi.shapingLevels[1] == -1);
    CHECK(paragraph.bidi.shapingLevels[4] == -1);
    CHECK(at(paragraph, 7).script == QChar::Script_Arabic);
    CHECK(at(paragraph, 8).script == QChar::Script_Arabic);
    // An embedding whose final I2 levels match is not an isolate boundary.
    const auto equal = plain(QStringLiteral("ب\u202bب\u202cب"), Qt::LeftToRight);
    REQUIRE(equal.contexts.size() == 1);
    CHECK(equal.contexts[0].text == Range{0, 5});
}

TEST_CASE("Formatting changes create items but retain bounded joining context", "[textruns][format]") {
    const auto document = Document::parse(QStringLiteral("ب{\\f{other}\\H2x;\\W0.5;ب}ب"), mtextOptions());
    const auto paragraph = Runs::itemize(document.paragraphs.front());
    checkInvariants(document.paragraphs.front(), paragraph);
    REQUIRE(paragraph.runs.size() == 3);
    REQUIRE(paragraph.contexts.size() == 1);
    for (const auto &run : paragraph.runs) {
        CHECK(run.context == Range{0, 3});
        CHECK(run.script == QChar::Script_Arabic);
    }
    CHECK(paragraph.runs[0].style.font == QStringLiteral("lc-amiri-1.003-ot-v1"));
    CHECK(paragraph.runs[1].style.font == QStringLiteral("other"));
    CHECK(paragraph.runs[1].style.heightFactor == 2);
    CHECK(paragraph.runs[1].style.widthFactor == 0.5);
    CHECK_FALSE(paragraph.runs[1].beginsContext());
    CHECK_FALSE(paragraph.runs[1].endsContext());
    CHECK(paragraph.runs[2].endsContext());
    CHECK(paragraph.sourceRanges(0, 3).size() == 3);
}

TEST_CASE("Language and oblique changes are items not joining boundaries", "[textruns][format]") {
    auto document = Document::plain(QStringLiteral("ببب"));
    auto style = document.paragraphs[0].spans[0].style;
    auto other = style;
    other.language = "fa";
    auto oblique = other;
    oblique.obliqueDegrees = 12;
    document.paragraphs[0].spans = {{{0, 1}, style}, {{1, 1}, other}, {{2, 1}, oblique}};
    const auto paragraph = Runs::itemize(document.paragraphs.front());
    checkInvariants(document.paragraphs.front(), paragraph);
    REQUIRE(paragraph.runs.size() == 3);
    CHECK(paragraph.runs[1].style.language == "fa");
    CHECK(paragraph.runs[2].style.obliqueDegrees == 12);
    CHECK(paragraph.contexts.size() == 1);
}

TEST_CASE("Paint-only changes do not split ligatures or marks from bases", "[textruns][format]") {
    for (const auto &text : {QStringLiteral("ل\\C1;\\Lا"), QStringLiteral("ب\\c16711680;\\Oَ"),
                            QStringLiteral("A\\K\\C2;\u05b0")}) {
        const auto document = Document::parse(text, mtextOptions());
        const auto paragraph = Runs::itemize(document.paragraphs.front());
        checkInvariants(document.paragraphs.front(), paragraph);
        REQUIRE(paragraph.runs.size() == 1);
        CHECK(paragraph.runs[0].text == Range{0, 2});
        REQUIRE(paragraph.paintSpans.size() == 2);
        CHECK(paragraph.paintSpans[0].style.color == -1);
        CHECK(paragraph.paintSpans[1].style.color != -1);
    }
    const auto document = Document::parse(QStringLiteral("A\\f{other}َ"), mtextOptions());
    const auto paragraph = Runs::itemize(document.paragraphs.front());
    REQUIRE(paragraph.runs.size() == 2);
    CHECK(paragraph.runs[1].script == QChar::Script_Latin);
    CHECK(paragraph.runs[1].representativeScalar == U'A');
    CHECK(paragraph.runs[1].style.font == QStringLiteral("other"));
    CHECK(paragraph.runs[1].context == Range{0, 2});
}

TEST_CASE("Common scripts inherit deliberately and paired punctuation matches", "[textruns][script]") {
    const auto latin = plain(QStringLiteral("...ABC, 123!"), Qt::LeftToRight);
    REQUIRE(latin.runs.size() == 1);
    CHECK(latin.runs[0].script == QChar::Script_Latin);
    CHECK(latin.runs[0].text == Range{0, int(latin.text.size())});
    const auto brackets = plain(QStringLiteral("abc(γ)def"), Qt::LeftToRight);
    REQUIRE(brackets.runs.size() == 3);
    CHECK(brackets.text.mid(brackets.runs[0].text.start, brackets.runs[0].text.length) == QStringLiteral("abc("));
    CHECK(brackets.runs[1].script == QChar::Script_Greek);
    CHECK(brackets.runs[2].script == QChar::Script_Latin);
    CHECK(brackets.text.mid(brackets.runs[2].text.start, brackets.runs[2].text.length) == QStringLiteral(")def"));
    CHECK(brackets.contexts.size() == 1);
    auto document = Document::plain(QStringLiteral("123,.\u200d"));
    Runs::Options options;
    options.fallbackScript = QChar::Script_Greek;
    const auto fallback = Runs::itemize(document.paragraphs[0], options);
    REQUIRE(fallback.runs.size() == 1);
    CHECK(fallback.runs[0].script == QChar::Script_Greek);
    CHECK(fallback.runs[0].representativeScalar == 0);
    const auto mixed = plain(QStringLiteral("ب ١٢٣ ABC"), Qt::LeftToRight);
    CHECK(at(mixed, 2).script == QChar::Script_Arabic);
    CHECK(at(mixed, 6).script == QChar::Script_Latin);
    CHECK(at(mixed, 0).contextIndex != at(mixed, 2).contextIndex);
}

TEST_CASE("Tabs inline objects and paragraphs are hard layout boundaries", "[textruns][objects]") {
    const auto document = Document::parse(QStringLiteral("ب\tب\\Sب^ج;ب\\Pب"), mtextOptions());
    REQUIRE(document.paragraphs.size() == 2);
    const auto paragraph = Runs::itemize(document.paragraphs[0]);
    checkInvariants(document.paragraphs[0], paragraph);
    REQUIRE(paragraph.runs.size() == 5);
    CHECK(at(paragraph, 1).kind == Runs::Kind::Tab);
    CHECK(at(paragraph, 3).kind == Runs::Kind::Object);
    CHECK(at(paragraph, 3).objectIndex == 0);
    CHECK(at(paragraph, 3).sources == std::vector<Range>{document.paragraphs[0].objects[0].source});
    for (const auto &run : paragraph.runs) CHECK(run.context == run.text);
    const auto child = Runs::itemize(paragraph.objects[0].upper->paragraphs[0]);
    REQUIRE(child.runs.size() == 1);
    CHECK(child.sourceMap[0].start > paragraph.sourceMap[0].start);
    const auto next = Runs::itemize(document.paragraphs[1]);
    REQUIRE(next.runs.size() == 1);
    CHECK(next.runs[0].context == Range{0, 1});
    CHECK(next.source.start >= paragraph.breakSource.end());
    const auto literal = plain(QStringLiteral("ب\ufffcب"));
    CHECK(at(literal, 1).kind == Runs::Kind::Object);
    CHECK(at(literal, 1).objectIndex == -1);
}

TEST_CASE("Supplementary escapes and invalid UTF16 retain original source mappings", "[textruns][source]") {
    auto options = mtextOptions();
    options.decodeDxfEscapes = true;
    const QString source = QStringLiteral("ب\\U+D83D\\U+DE00") + QChar(0xd800) + QStringLiteral("A");
    const auto document = Document::parse(source, options);
    const auto paragraph = Runs::itemize(document.paragraphs[0]);
    checkInvariants(document.paragraphs[0], paragraph);
    CHECK(document.source == source);
    CHECK(paragraph.text == QStringLiteral("ب😀\ufffdA"));
    CHECK(paragraph.sourceMap[1] == Range{1, 14});
    CHECK(paragraph.sourceMap[2] == Range{1, 14});
    CHECK(paragraph.sourceRanges(2, 1) == std::vector<Range>{{1, 14}});
    CHECK(paragraph.sourceMap[3] == Range{15, 1});
    CHECK(at(paragraph, 1).text.end() >= 3);
}

TEST_CASE("Chinese RTL projection remains display-only and source-mapped", "[textruns][projection]") {
    auto options = mtextOptions();
    options.decodeDxfEscapes = true;
    const auto document = Document::parse(QStringLiteral("ب\\U+4E00二ABC"), options);
    Runs::Options runOptions;
    runOptions.direction = Qt::RightToLeft;
    const auto paragraph = Runs::itemize(document.paragraphs[0], runOptions);
    checkInvariants(document.paragraphs[0], paragraph);
    CHECK(paragraph.text == QStringLiteral("ب一二ABC"));
    CHECK(paragraph.bidi.projection.text.size() > paragraph.text.size());
    CHECK(at(paragraph, 1).script == QChar::Script_Han);
    CHECK(at(paragraph, 1).rightToLeft());
    CHECK(at(paragraph, 1).context == Range{1, 2});
    CHECK(at(paragraph, 0).contextIndex != at(paragraph, 1).contextIndex);
    CHECK(at(paragraph, 3).contextIndex != at(paragraph, 1).contextIndex);
    for (int i = 0; i < paragraph.bidi.projection.text.size(); ++i) {
        const int original = paragraph.bidi.projection.sourcePositions[i];
        if (original < 0) CHECK(paragraph.projectedSourceRanges(i, 1).empty());
        else CHECK(paragraph.projectedSourceRanges(i, 1) == std::vector<Range>{paragraph.sourceMap[original]});
    }
}

TEST_CASE("Empty paragraphs and control-only contexts have deterministic policy", "[textruns]") {
    const auto empty = plain({});
    REQUIRE(empty.ok());
    CHECK(empty.runs.empty());
    CHECK(empty.contexts.empty());
    CHECK(empty.sourceRanges(0, 0).empty());
    const auto controls = plain(QStringLiteral("\u200d\u200c\u202a\u202c"));
    REQUIRE(controls.ok());
    REQUIRE(controls.runs.size() == 1);
    CHECK(controls.runs[0].level == controls.bidi.base);
    CHECK(controls.runs[0].script == QChar::Script_Arabic);
    CHECK(controls.runs[0].text.length == 4);
}

TEST_CASE("Invalid public paragraph structures are rejected without partial success", "[textruns][negative]") {
    const auto valid = Document::plain(QStringLiteral("😀ب")).paragraphs.front();
    auto invalid = valid;
    invalid.scalars.front().source.start = std::numeric_limits<int>::max();
    CHECK_FALSE(Runs::itemize(invalid).ok());
    invalid = valid;
    invalid.scalars.front().value = U'A';
    CHECK_FALSE(Runs::itemize(invalid).ok());
    invalid = valid;
    invalid.scalars.clear();
    CHECK_FALSE(Runs::itemize(invalid).ok());
    invalid = valid;
    invalid.spans.front().text.length = 1;
    invalid.spans.push_back({{1, 2}, invalid.spans.front().style});
    CHECK_FALSE(Runs::itemize(invalid).ok());
    invalid = valid;
    invalid.spans.front().style.heightFactor = std::numeric_limits<double>::quiet_NaN();
    CHECK_FALSE(Runs::itemize(invalid).ok());
    invalid = valid;
    invalid.spans.front().style.obliqueDegrees = 90;
    CHECK_FALSE(Runs::itemize(invalid).ok());
    Runs::Options options;
    options.fallbackScript = QChar::Script_Inherited;
    CHECK_FALSE(Runs::itemize(valid, options).ok());
    options = {};
    // Three is within this enum's representable range, but not a direction.
    options.direction = static_cast<Qt::LayoutDirection>(3);
    CHECK_FALSE(Runs::itemize(valid, options).ok());
    options = {};
    options.fallbackScript = QChar::ScriptCount;
    CHECK_FALSE(Runs::itemize(valid, options).ok());
    invalid = valid;
    invalid.breakSource = {1, 1};
    CHECK_FALSE(Runs::itemize(invalid).ok());
    auto object = Document::parse(QStringLiteral("\\Sب^ج;"), mtextOptions()).paragraphs[0];
    object.objects[0].source.length = 1;
    CHECK_FALSE(Runs::itemize(object).ok());
    const auto paragraph = Runs::itemize(valid);
    CHECK(paragraph.sourceRanges(-1, 2).empty());
    CHECK(paragraph.sourceRanges(0, std::numeric_limits<int>::max()).empty());
    CHECK(paragraph.projectedSourceRanges(1, -1).empty());
}

TEST_CASE("Random logical itemization preserves every scalar exactly once", "[textruns][source]") {
    std::mt19937 random(0x1859);
    const QList<QString> pieces{QStringLiteral("ب"), QStringLiteral("َ"), QStringLiteral("A"),
        QStringLiteral("γ"), QStringLiteral("一"), QStringLiteral("😀"), QStringLiteral("\u200d"),
        QStringLiteral("\u200c"), QStringLiteral("\u2066"), QStringLiteral("\u2067"),
        QStringLiteral("\u2069"), QStringLiteral("\u202a"), QStringLiteral("\u202c"),
        QStringLiteral("("), QStringLiteral(")"), QStringLiteral("12"), QStringLiteral(" "),
        QStringLiteral("\t"), QStringLiteral("\ufffc"), QString(QChar(0xd800))};
    for (int iteration = 0; iteration < 256; ++iteration) {
        QString text;
        for (int i = 0; i < 40; ++i) text += pieces[random() % pieces.size()];
        const auto document = Document::plain(text);
        Runs::Options options;
        options.direction = iteration % 2 ? Qt::LeftToRight : Qt::RightToLeft;
        checkInvariants(document.paragraphs[0], Runs::itemize(document.paragraphs[0], options));
    }
}

TEST_CASE("Many font and paint spans retain shared contexts without text copies", "[textruns][limits]") {
    auto document = Document::plain(QString(8192, QChar(0x0628)));
    auto style = document.paragraphs[0].spans[0].style;
    document.paragraphs[0].spans.clear();
    for (int i = 0; i < 8192; ++i) {
        style.font = i % 2 ? QStringLiteral("one") : QStringLiteral("two");
        document.paragraphs[0].spans.push_back({{i, 1}, style});
    }
    const auto fonts = Runs::itemize(document.paragraphs[0]);
    REQUIRE(fonts.ok());
    CHECK(fonts.runs.size() == 8192);
    REQUIRE(fonts.contexts.size() == 1);
    CHECK(fonts.runs.back().context == Range{0, 8192});
    for (auto &span : document.paragraphs[0].spans) {
        span.style.font = QStringLiteral("one");
        span.style.color = span.text.start % 255;
    }
    const auto paint = Runs::itemize(document.paragraphs[0]);
    REQUIRE(paint.ok());
    CHECK(paint.runs.size() == 1);
    CHECK(paint.paintSpans.size() == 8192);
    const auto nested = plain(QString(8192, QChar('(')) + QStringLiteral("Aγ") +
                              QString(8192, QChar(')')), Qt::LeftToRight);
    REQUIRE(nested.ok());
    CHECK(nested.contexts.size() == 1);
    CHECK(at(nested, 0).script == QChar::Script_Latin);
    CHECK(at(nested, int(nested.text.size()) - 1).script == QChar::Script_Latin);
}

TEST_CASE("Equal font names held in separate buffers stay one item", "[textruns][format]") {
    const QString name(48, QChar('F'));
    const QString other = name.left(47) + QStringLiteral("G");
    // The second command repeats the name of the first, and the underline keeps
    // the two spans apart: one item of three letters, in two spans. The last
    // name has the same length and differs in its last letter.
    const QString source = QStringLiteral("\\f") + name + QStringLiteral(";ب\\f") + name + QStringLiteral(";\\Lبب\\f") +
                           other + QStringLiteral(";ب");
    const auto document = Document::parse(source, mtextOptions());
    REQUIRE(document.ok());
    const auto &logical = document.paragraphs.front();
    REQUIRE(logical.spans.size() == 3);
    REQUIRE(logical.spans[0].style.font.constData() != logical.spans[1].style.font.constData());
    const auto paragraph = Runs::itemize(logical);
    checkInvariants(logical, paragraph);
    REQUIRE(paragraph.runs.size() == 2);
    CHECK(paragraph.runs[0].text == Range{0, 3});
    CHECK(paragraph.runs[0].style.font == name);
    CHECK(paragraph.runs[1].text == Range{3, 1});
    CHECK(paragraph.runs[1].style.font == other);
    CHECK(paragraph.contexts.size() == 1);
}
