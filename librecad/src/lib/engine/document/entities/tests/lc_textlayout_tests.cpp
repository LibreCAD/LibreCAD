/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textlayout.h"
#include "lc_textshaper.h"
#include "qt_bidi.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace {
using Layout = LC_TextLayout;
using Document = LC_TextDocument;
using Range = Document::Range;
using Code = Layout::ErrorCode;

void atomicError(const Layout::Result &result, Code code) {
    INFO(result.error.message.toStdString());
    REQUIRE_FALSE(result);
    CHECK(result.error.code == code);
    CHECK_FALSE(result.error.message.isEmpty());
    CHECK(result.text.isEmpty());
    CHECK(result.sourceMap.empty());
    CHECK(result.graphemes.empty());
    CHECK(result.logicalRuns.empty());
    CHECK(result.clusters.empty());
    CHECK(result.glyphs.empty());
    CHECK(result.objects.empty());
    CHECK_FALSE(result.emptyFont);
    CHECK(result.fragments.empty());
    CHECK(result.visualFragments.empty());
    CHECK(result.diagnostics.empty());
    CHECK(result.metrics.logicalAdvance == 0);
    CHECK_FALSE(result.metrics.hasInk);
    CHECK(result.metrics.ascent == 0);
}
void atomicError(const Layout::BlockResult &result, Code code) {
    INFO(result.error.message.toStdString());
    REQUIRE_FALSE(result);
    CHECK(result.error.code == code);
    CHECK_FALSE(result.error.message.isEmpty());
    CHECK(result.lines.empty());
    CHECK_FALSE(result.baseFont);
    CHECK(result.metrics.logicalAdvance == 0);
    CHECK_FALSE(result.metrics.hasInk);
    CHECK(result.baselineSpan == 0);
}

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
std::shared_ptr<const LC_OpenTypeFont> font() {
    static const auto loaded = LC_OpenTypeFont::load(QString::fromUtf8(LC_OPENTYPE_TEST_FONT_PATH));
    REQUIRE_FALSE(loaded.error);
    REQUIRE(loaded.font);
    return loaded.font;
}
Layout::FontResolver resolver() {
    return [](const Layout::FontRequest &) { return Layout::FontResolution{font(), {}, {}}; };
}
Layout::Result place(const Document::Paragraph &paragraph, Qt::LayoutDirection direction = Qt::LayoutDirectionAuto) {
    Layout::Options options;
    options.direction = direction;
    return Layout::paragraph(paragraph, resolver(), options);
}
Layout::Result place(const QString &text, Qt::LayoutDirection direction = Qt::LayoutDirectionAuto) {
    return place(Document::plain(text).paragraphs.front(), direction);
}
Document mtext(const QString &source) {
    Document::Options options;
    options.syntax = Document::Syntax::MText;
    options.decodeDxfEscapes = true;
    options.initialStyle.font = QStringLiteral("lc-amiri-1.003-ot-v1");
    return Document::parse(source, options);
}
std::vector<std::uint32_t> ids(const Layout::Result &result) {
    std::vector<std::uint32_t> output;
    for (const auto &glyph : result.glyphs) output.push_back(glyph.id);
    return output;
}
std::vector<std::uint32_t> visualIds(const Layout::Result &result, bool drawableOnly = false) {
    std::vector<std::uint32_t> output;
    for (const auto fragment : result.visualFragments)
        for (const auto glyph : result.fragments[fragment].glyphs)
            if (!drawableOnly || result.glyphs[glyph].drawable) output.push_back(result.glyphs[glyph].id);
    return output;
}
void invariants(const Document::Paragraph &document, const Layout::Result &result, Qt::LayoutDirection direction) {
    INFO(result.error.message.toStdString());
    REQUIRE(result);
    LC_TextRuns::Options options;
    options.direction = direction;
    const auto logical = LC_TextRuns::itemize(document, options);
    REQUIRE(logical.ok());
    CHECK(result.text == document.text);
    CHECK(result.source == document.source);
    CHECK(result.breakSource == document.breakSource);
    CHECK(result.graphemes == document.graphemes);
    CHECK(result.sourceMap == logical.sourceMap);
    CHECK(result.baseLevel == logical.bidi.base);
    CHECK(result.unicodeVersion == QChar::currentUnicodeVersion());
    CHECK(result.shapingEngine == LC_OpenTypeFont::shapingEngineIdentity());
    REQUIRE(result.logicalRuns.size() == logical.runs.size());
    std::vector<int> unitOwners(document.text.size());
    std::vector<int> glyphOwners(result.glyphs.size());
    std::vector<std::size_t> fragmentForUnit(document.text.size());
    for (std::size_t index = 0; index < result.fragments.size(); ++index) {
        const auto &fragment = result.fragments[index];
        REQUIRE(fragment.text.start >= 0);
        REQUIRE(fragment.text.length > 0);
        REQUIRE(fragment.text.end() <= document.text.size());
        REQUIRE(fragment.logicalRun < result.logicalRuns.size());
        CHECK(fragment.sources == document.sourceRanges(fragment.text.start, fragment.text.length));
        CHECK_FALSE(document.text[fragment.text.start].isLowSurrogate());
        for (int unit = fragment.text.start; unit < fragment.text.end(); ++unit) {
            CHECK(fragment.displayLevel == logical.bidi.displayLevels[unit]);
            ++unitOwners[unit];
            fragmentForUnit[unit] = index;
        }
        std::set<std::size_t> emittedClusters;
        std::size_t previousCluster = std::numeric_limits<std::size_t>::max();
        for (auto glyphIndex : fragment.glyphs) {
            REQUIRE(glyphIndex < result.glyphs.size());
            const auto &glyph = result.glyphs[glyphIndex];
            CHECK(glyph.logicalRun == fragment.logicalRun);
            ++glyphOwners[glyphIndex];
            if (glyph.cluster != previousCluster) CHECK(emittedClusters.insert(glyph.cluster).second);
            previousCluster = glyph.cluster;
        }
    }
    for (auto owners : unitOwners) CHECK(owners == 1);
    for (auto owners : glyphOwners) CHECK(owners == 1);
    std::vector<std::size_t> expectedOrder;
    std::set<std::size_t> emitted;
    const auto display = lc::qtbidi::resolve(logical.bidi.projection.text, direction);
    for (int projected : lc::qtbidi::reorder(display.levels)) {
        const int original = logical.bidi.projection.sourcePositions[projected];
        if (original >= 0 && emitted.insert(fragmentForUnit[original]).second)
            expectedOrder.push_back(fragmentForUnit[original]);
    }
    CHECK(result.visualFragments == expectedOrder);
    double cursor = 0;
    for (auto fragmentIndex : result.visualFragments) {
        const auto &fragment = result.fragments[fragmentIndex];
        CHECK(fragment.origin.x() == Catch::Approx(cursor));
        CHECK(fragment.origin.y() == 0);
        CHECK(fragment.advance.x() >= 0);
        cursor += fragment.advance.x();
    }
    CHECK(result.metrics.logicalAdvance == Catch::Approx(cursor));
    CHECK(result.metrics.baseline == 0);
    CHECK(result.metrics.ascent > 0);
    CHECK(result.metrics.descent > 0);
    for (std::size_t runIndex = 0; runIndex < logical.runs.size(); ++runIndex) {
        const auto &run = logical.runs[runIndex];
        const auto &placedRun = result.logicalRuns[runIndex];
        CHECK(placedRun.item.text == run.text);
        CHECK(placedRun.item.context == run.context);
        CHECK(placedRun.item.style == run.style);
        REQUIRE(placedRun.font);
        std::vector<const Layout::Glyph *> actual;
        for (const auto &glyph : result.glyphs) if (glyph.logicalRun == runIndex) actual.push_back(&glyph);
        if (run.kind == LC_TextRuns::Kind::BidiBoundary || run.kind == LC_TextRuns::Kind::Tab || run.kind == LC_TextRuns::Kind::Object) {
            CHECK(actual.empty());
            continue;
        }
        LC_TextShaper::Run shaping;
        shaping.contextText = document.text.mid(run.context.start, run.context.length);
        shaping.itemStart = run.text.start - run.context.start;
        shaping.itemLength = run.text.length;
        shaping.sourceOffset = run.context.start;
        shaping.rightToLeft = run.rightToLeft();
        shaping.script = placedRun.script;
        shaping.language = run.style.language;
        const auto raw = LC_TextShaper::shape(*placedRun.font, shaping);
        REQUIRE(raw);
        REQUIRE(actual.size() == raw.glyphs.size());
        for (std::size_t index = 0; index < actual.size(); ++index) {
            const auto &glyph = *actual[index];
            const auto &expected = raw.glyphs[index];
            REQUIRE(glyph.cluster < result.clusters.size());
            CHECK(glyph.id == expected.id);
            CHECK(glyph.xAdvance == expected.xAdvance);
            CHECK(glyph.yAdvance == expected.yAdvance);
            CHECK(glyph.xOffset == expected.xOffset);
            CHECK(glyph.yOffset == expected.yOffset);
            CHECK(glyph.unsafeToBreak == expected.unsafeToBreak);
            CHECK(result.clusters[glyph.cluster].text == Range{expected.source.start, expected.source.length});
            CHECK(glyph.font == placedRun.font);
            REQUIRE(glyph.outline);
            CHECK(glyph.drawable == glyph.outline->hasInk);
        }
    }
    for (const auto &cluster : result.clusters) {
        REQUIRE(cluster.text.start >= 0);
        REQUIRE(cluster.text.length > 0);
        REQUIRE(cluster.text.end() <= document.text.size());
        CHECK(cluster.sources == document.sourceRanges(cluster.text.start, cluster.text.length));
        CHECK(cluster.paint == *document.styleAt(cluster.text.start));
        std::vector<std::size_t> overlaps;
        for (std::size_t index = 0; index < document.graphemes.size(); ++index) {
            const auto range = document.graphemes[index];
            if (range.start < cluster.text.end() && range.end() > cluster.text.start) overlaps.push_back(index);
        }
        CHECK(cluster.graphemes == overlaps);
        int retainedLevel = -1;
        for (int unit = cluster.text.start; unit < cluster.text.end(); ++unit) {
            if (logical.bidi.shapingLevels[unit] < 0) continue;
            if (retainedLevel >= 0) CHECK(retainedLevel == logical.bidi.displayLevels[unit]);
            retainedLevel = logical.bidi.displayLevels[unit];
        }
    }
}
QPointF transformed(const Layout::Affine &a, const QPointF &p) {
    return {a.xx * p.x() + a.xy * p.y() + a.dx, a.yx * p.x() + a.yy * p.y() + a.dy};
}
Layout::Affine composed(const Layout::Affine &outer, const Layout::Affine &inner) {
    return {outer.xx * inner.xx + outer.xy * inner.yx, outer.xx * inner.xy + outer.xy * inner.yy,
            outer.yx * inner.xx + outer.yy * inner.yx, outer.yx * inner.xy + outer.yy * inner.yy,
            outer.xx * inner.dx + outer.xy * inner.dy + outer.dx, outer.yx * inner.dx + outer.yy * inner.dy + outer.dy};
}
QPointF interpolate(const QPointF &a, const QPointF &b, double t) { return a * (1 - t) + b * t; }
using InkCheck = std::function<void(const QPointF &)>;
void sampleParagraph(const Layout::Result &, const Layout::Affine &, const InkCheck &);
void sampleBlock(const Layout::BlockResult &block, const Layout::Affine &outer, const InkCheck &check) {
    for (const auto &line : block.lines) {
        Layout::Affine translation;
        translation.dx = line.origin.x(); translation.dy = line.origin.y();
        sampleParagraph(line.paragraph, composed(outer, translation), check);
    }
}
void sampleParagraph(const Layout::Result &result, const Layout::Affine &outer, const InkCheck &check) {
    using Type = LC_OpenTypeFont::Command::Type;
    for (const auto &glyph : result.glyphs) {
        if (!glyph.drawable) continue;
        const auto affine = composed(outer, glyph.transform);
        QPointF current, first;
        for (const auto &command : glyph.outline->commands) {
            const auto a = transformed(affine, command.first);
            const auto b = transformed(affine, command.second);
            const auto c = transformed(affine, command.third);
            switch (command.type) {
            case Type::MoveTo: current = first = a; break;
            case Type::LineTo: check(current); check(a); current = a; break;
            case Type::QuadraticTo:
                for (int step = 0; step <= 64; ++step) {
                    const double t = step / 64.0;
                    check(interpolate(interpolate(current, a, t), interpolate(a, b, t), t));
                }
                current = b;
                break;
            case Type::CubicTo:
                for (int step = 0; step <= 64; ++step) {
                    const double t = step / 64.0;
                    check(interpolate(interpolate(interpolate(current, a, t), interpolate(a, b, t), t),
                                      interpolate(interpolate(a, b, t), interpolate(b, c, t), t), t));
                }
                current = c;
                break;
            case Type::Close: check(current); check(first); current = first; break;
            }
        }
    }
    for (const auto &object : result.objects) {
        REQUIRE(object.upper);
        REQUIRE(object.lower);
        sampleBlock(*object.upper, composed(outer, object.upperTransform), check);
        sampleBlock(*object.lower, composed(outer, object.lowerTransform), check);
        for (const auto &stroke : object.separators) { check(transformed(outer, stroke.start)); check(transformed(outer, stroke.end)); }
    }
}
template<typename Result>
InkCheck boundsCheck(const Result &result) {
    REQUIRE(result.metrics.hasInk);
    const auto bounds = result.metrics.inkBounds;
    const double epsilon = 1e-7 * std::max({1.0, std::abs(bounds.left()), std::abs(bounds.right()),
                                          std::abs(bounds.top()), std::abs(bounds.bottom())});
    return [bounds, epsilon](const QPointF &p) {
        CHECK(p.x() >= bounds.left() - epsilon);
        CHECK(p.x() <= bounds.right() + epsilon);
        CHECK(p.y() >= bounds.top() - epsilon);
        CHECK(p.y() <= bounds.bottom() + epsilon);
    };
}
void sampledInkWithinBounds(const Layout::Result &result) { sampleParagraph(result, {}, boundsCheck(result)); }
void sampledInkWithinBounds(const Layout::BlockResult &result) { sampleBlock(result, {}, boundsCheck(result)); }
#endif
} // namespace

#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
TEST_CASE("Disabled paragraph layout never resolves a replacement font", "[textlayout][headless][off]") {
    bool called = false;
    Layout::FontResolver explicitResolver = [&](const Layout::FontRequest &) {
        called = true;
        return Layout::FontResolution{};
    };
    const auto document = Document::plain(QStringLiteral("ب"));
    atomicError(Layout::paragraph(document.paragraphs.front(), explicitResolver), Code::BackendUnavailable);
    atomicError(Layout::document(document, explicitResolver), Code::BackendUnavailable);
    CHECK_FALSE(called);
}
#else
TEST_CASE("Paragraph layout consumes contextual RTL glyph order exactly once", "[textlayout][headless][shaping]") {
    const auto document = Document::plain(QStringLiteral("ببب"));
    const auto result = place(document.paragraphs.front());
    invariants(document.paragraphs.front(), result, Qt::LayoutDirectionAuto);
    REQUIRE(ids(result) == std::vector<std::uint32_t>{1589, 3999, 3958});
    CHECK(visualIds(result) == ids(result));
    REQUIRE(result.fragments.size() == 1);
    const double factor = 9.0 / 647;
    CHECK(result.glyphs[0].transform.dx == 0);
    CHECK(result.glyphs[1].transform.dx == Catch::Approx(883 * factor));
    CHECK(result.glyphs[2].transform.dx == Catch::Approx(1127 * factor));
    CHECK(result.metrics.logicalAdvance == Catch::Approx(1360 * factor));
    CHECK(document.source == QStringLiteral("ببب"));
    CHECK(document.paragraphs.front().text == document.source);
    sampledInkWithinBounds(result);
    const auto repeated = place(document.paragraphs.front());
    REQUIRE(repeated);
    REQUIRE(repeated.glyphs.size() == result.glyphs.size());
    for (std::size_t index = 0; index < result.glyphs.size(); ++index)
        CHECK(repeated.glyphs[index].outline == result.glyphs[index].outline);
}

TEST_CASE("Shared clusters keep raw source, independent graphemes and first logical paint", "[textlayout][headless][provenance]") {
    const auto document = mtext(QStringLiteral("\\C1;ب\\C2;َ\\C3;ّ"));
    REQUIRE(document.ok());
    const auto &paragraph = document.paragraphs.front();
    const auto result = place(paragraph);
    invariants(paragraph, result, Qt::LayoutDirectionAuto);
    REQUIRE(ids(result) == std::vector<std::uint32_t>{1439, 97, 56});
    REQUIRE(result.logicalRuns.size() == 1);
    CHECK(result.glyphs[0].cluster == result.glyphs[1].cluster);
    REQUIRE(result.clusters.size() == 2);
    const auto &marks = result.clusters[result.glyphs[0].cluster];
    CHECK(marks.text == Range{1, 2});
    CHECK(marks.paint.color == 2);
    REQUIRE(marks.sources.size() == 2);
    CHECK(document.source.mid(marks.sources[0].start, marks.sources[0].length) == QStringLiteral("َ"));
    CHECK(document.source.mid(marks.sources[1].start, marks.sources[1].length) == QStringLiteral("ّ"));
    CHECK(marks.graphemes == std::vector<std::size_t>{0});
    CHECK(result.clusters[result.glyphs[2].cluster].paint.color == 1);
    const double factor = 9.0 / 647;
    CHECK(result.glyphs[0].transform.dx == Catch::Approx(453 * factor));
    CHECK(result.glyphs[0].transform.dy == Catch::Approx(51 * factor));
    CHECK(result.glyphs[1].transform.dx == Catch::Approx(253 * factor));
    CHECK(result.glyphs[1].xAdvance == 0);
    sampledInkWithinBounds(result);

    const auto escaped = mtext(QStringLiteral("\\U+0628\\U+064E\\U+0651 😀"));
    REQUIRE(escaped.ok());
    const auto escapedResult = place(escaped.paragraphs.front());
    invariants(escaped.paragraphs.front(), escapedResult, Qt::LayoutDirectionAuto);
    CHECK(escapedResult.sourceMap[0] == Range{0, 7});
    CHECK(escapedResult.sourceMap.back() == escapedResult.sourceMap[escapedResult.sourceMap.size() - 2]);
    CHECK(escaped.source == QStringLiteral("\\U+0628\\U+064E\\U+0651 😀"));
}

TEST_CASE("Advance, exact ink, baseline and pinned profile metrics stay separate", "[textlayout][headless][metrics]") {
    const double factor = 9.0 / 647;
    const auto letter = place(QStringLiteral("A"));
    REQUIRE(letter);
    CHECK(letter.logicalRuns.front().script == "Latn");
    CHECK(letter.metrics.logicalAdvance == Catch::Approx(612 * factor));
    REQUIRE(letter.metrics.hasInk);
    CHECK(letter.metrics.inkBounds.left() == Catch::Approx(-14 * factor));
    // The quadratic contour's true extremum is fractional; the integer FT
    // outline box is not an exact transformed-ink measurement.
    CHECK(letter.metrics.inkBounds.right() == Catch::Approx(618.2 * factor));
    CHECK(letter.metrics.inkBounds.top() == Catch::Approx(-3 * factor));
    CHECK(letter.metrics.inkBounds.bottom() == Catch::Approx(647 * factor));
    CHECK(letter.metrics.ascent == Catch::Approx(1124 * factor));
    CHECK(letter.metrics.descent == Catch::Approx(634 * factor));
    CHECK(letter.metrics.profileLinePitch == Catch::Approx(1758 * factor));
    CHECK(letter.metrics.nominalCellHeight == 9);
    CHECK(letter.metrics.inkAboveBaseline == 9);
    CHECK(letter.metrics.inkBelowBaseline == Catch::Approx(3 * factor));
    sampledInkWithinBounds(letter);
    const auto spaces = place(QStringLiteral("   "));
    REQUIRE(spaces);
    CHECK(spaces.metrics.logicalAdvance == Catch::Approx(3 * 292 * factor));
    CHECK_FALSE(spaces.metrics.hasInk);
    CHECK(spaces.metrics.inkBounds.isNull());
    CHECK(spaces.metrics.inkAboveBaseline == 0);
    CHECK(spaces.metrics.ascent == letter.metrics.ascent);
    const auto empty = place(QString{});
    REQUIRE(empty);
    CHECK(empty.logicalRuns.empty());
    CHECK(empty.visualFragments.empty());
    CHECK(empty.metrics.logicalAdvance == 0);
    CHECK_FALSE(empty.metrics.hasInk);
    CHECK(empty.metrics.profileLinePitch == letter.metrics.profileLinePitch);
    Layout::Options options;
    options.emptyStyle.heightFactor = 2;
    auto manuallyConstructedEmpty = Document::plain(QString{}).paragraphs.front();
    manuallyConstructedEmpty.emptyStyle.reset();
    const auto doubleEmpty = Layout::paragraph(manuallyConstructedEmpty, resolver(), options);
    REQUIRE(doubleEmpty);
    CHECK(doubleEmpty.metrics.ascent == 2 * empty.metrics.ascent);
    CHECK(doubleEmpty.metrics.nominalCellHeight == 18);
}

TEST_CASE("Inline shaping changes preserve bounded joining context and transformed ink", "[textlayout][headless][formatting]") {
    const auto document = mtext(QStringLiteral("ب{\\H2x;\\W0.35;\\Q33;ب}ب"));
    REQUIRE(document.ok());
    std::vector<Layout::FontRequest> requests;
    const auto result = Layout::paragraph(document.paragraphs.front(), [&](const Layout::FontRequest &request) {
        requests.push_back(request);
        return Layout::FontResolution{font(), {}, {}};
    });
    invariants(document.paragraphs.front(), result, Qt::LayoutDirectionAuto);
    REQUIRE(requests.size() == 3);
    REQUIRE(result.glyphs.size() == 3);
    CHECK(ids(result) == std::vector<std::uint32_t>{1611, 1652, 1589});
    for (const auto &request : requests) CHECK(request.context == Range{0, 3});
    CHECK(requests[1].style.heightFactor == 2);
    CHECK(requests[1].style.widthFactor == 0.35);
    CHECK(requests[1].style.obliqueDegrees == 33);
    const double factor = 9.0 / 647;
    CHECK(result.glyphs[1].transform.xx == Catch::Approx(factor * 2 * 0.35));
    CHECK(result.glyphs[1].transform.yy == Catch::Approx(factor * 2));
    CHECK(result.glyphs[1].transform.xy == Catch::Approx(factor * 2 * 0.35 * std::tan(33 * std::acos(-1.0) / 180)));
    CHECK(visualIds(result) == std::vector<std::uint32_t>{1589, 1652, 1611});
    CHECK(result.metrics.logicalAdvance == Catch::Approx((result.glyphs[0].xAdvance +
        result.glyphs[1].xAdvance * 2 * 0.35 + result.glyphs[2].xAdvance) * factor));
    CHECK(result.metrics.nominalCellHeight == 18);
    sampledInkWithinBounds(result);
}

TEST_CASE("Highly anisotropic inline transforms measure contours rather than transformed font boxes", "[textlayout][headless][metrics]") {
    for (const auto height : {0.00001, 1.0, 100000.0}) for (const auto width : {0.001, 1000.0}) {
        Document::Options parseOptions;
        parseOptions.initialStyle.heightFactor = height;
        parseOptions.initialStyle.widthFactor = width;
        parseOptions.initialStyle.obliqueDegrees = width < 1 ? -78 : 78;
        const auto document = Document::parse(QStringLiteral("Aبَّ"), parseOptions);
        REQUIRE(document.ok());
        const auto result = place(document.paragraphs.front(), Qt::LeftToRight);
        invariants(document.paragraphs.front(), result, Qt::LeftToRight);
        CHECK(result.metrics.nominalCellHeight == Catch::Approx(9 * height));
        double advance = 0;
        for (const auto &glyph : result.glyphs) {
            advance += glyph.xAdvance * (9.0 / 647) * height * width;
            CHECK(std::isfinite(glyph.transform.dx));
            CHECK(std::isfinite(glyph.transform.dy));
        }
        CHECK(result.metrics.logicalAdvance == Catch::Approx(advance));
        sampledInkWithinBounds(result);
    }
}

TEST_CASE("L1 fragments, borrowed X9, empty isolates and Chinese projection retain logical shaping", "[textlayout][headless][bidi]") {
    const std::vector<QString> cases{
        QStringLiteral("ب\u200d"), QStringLiteral("A\u200dب"), QStringLiteral("ب\u200dA"),
        QStringLiteral("ب \u200d"), QStringLiteral("ب\u200c "), QStringLiteral("ب\u2067\u2069ب"),
        QStringLiteral("一A ب\u200d"), QStringLiteral("ب\u202a\u202cب"), QStringLiteral("A (ب) 12"),
        QStringLiteral("ب\u2067A 12\u2069ب"), QStringLiteral("\u2067\u2069"), QStringLiteral("\u200d\u200c"),
        QStringLiteral("\u200e\u200f\u061c"), QStringLiteral("ب\u2067\u2066A\u2069\u2069ب"),
        QStringLiteral("中，A ب\u200d")};
    for (const auto &text : cases) for (auto direction : {Qt::LeftToRight, Qt::RightToLeft, Qt::LayoutDirectionAuto}) {
        CAPTURE(text.toStdString(), int(direction));
        const auto document = Document::plain(text);
        invariants(document.paragraphs.front(), place(document.paragraphs.front(), direction), direction);
        CHECK(document.source == text);
    }
    const auto trailing = place(cases[0], Qt::LeftToRight);
    REQUIRE(trailing);
    REQUIRE(trailing.logicalRuns.size() == 1);
    REQUIRE(trailing.fragments.size() == 2);
    CHECK(trailing.fragments[0].displayLevel == 1);
    CHECK(trailing.fragments[1].displayLevel == 0);
    CHECK(visualIds(trailing, true) == std::vector<std::uint32_t>{1611});
    const auto isolated = place(QStringLiteral("ب\u2067\u2069ب"), Qt::LeftToRight);
    CHECK(visualIds(isolated, true) == std::vector<std::uint32_t>{56, 56});
    const auto mirrorDocument = Document::plain(QStringLiteral("(ب)"));
    const auto mirror = place(mirrorDocument.paragraphs.front(), Qt::RightToLeft);
    invariants(mirrorDocument.paragraphs.front(), mirror, Qt::RightToLeft);
    // The reference comparison above is to logical HB input, not pre-mirrored
    // characters; no second glyph mirroring/reversal is performed here.
}

TEST_CASE("Explicit missing-font and missing-coverage outcomes never infer replacements", "[textlayout][headless][coverage]") {
    const auto document = Document::plain(QStringLiteral("ب😀ب"));
    const auto result = place(document.paragraphs.front());
    invariants(document.paragraphs.front(), result, Qt::LayoutDirectionAuto);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics[0].textRanges == std::vector<Range>{{1, 2}});
    CHECK(result.diagnostics[0].sources == std::vector<Range>{{1, 2}});
    bool missingInk = false;
    for (const auto &glyph : result.glyphs) {
        CHECK(glyph.font == font());
        if (glyph.id == 0) { CHECK(glyph.drawable); missingInk = true; }
    }
    CHECK(missingInk);
    const auto unavailable = Layout::paragraph(document.paragraphs.front(), [](const Layout::FontRequest &) {
        return Layout::FontResolution{{}, QStringLiteral("Exact saved profile is absent"), {}};
    });
    atomicError(unavailable, Code::FontUnavailable);
    CHECK(unavailable.error.message == QStringLiteral("Exact saved profile is absent"));
    atomicError(Layout::paragraph(document.paragraphs.front(), [](const Layout::FontRequest &) -> Layout::FontResolution {
        throw std::runtime_error("private resolver detail");
    }), Code::FontUnavailable);
    atomicError(Layout::paragraph(document.paragraphs.front(), {}), Code::InvalidOptions);
    const auto exactError = LC_OpenTypeFont::Error{LC_OpenTypeFont::ErrorCode::ChecksumMismatch,
                                                 QStringLiteral("Pinned bytes differ")};
    const auto wrongBytes = Layout::paragraph(document.paragraphs.front(), [&](const Layout::FontRequest &) {
        return Layout::FontResolution{{}, {}, exactError};
    });
    atomicError(wrongBytes, Code::FontUnavailable);
    CHECK(wrongBytes.error.fontError.code == LC_OpenTypeFont::ErrorCode::ChecksumMismatch);

    QString malformed = QStringLiteral("ب");
    malformed += QChar(0xd800);
    malformed += QStringLiteral("ب");
    const auto malformedDocument = Document::plain(malformed);
    REQUIRE(malformedDocument.ok());
    CHECK(malformedDocument.source == malformed);
    CHECK(malformedDocument.paragraphs.front().text[1] == QChar::ReplacementCharacter);
    const auto malformedResult = place(malformedDocument.paragraphs.front());
    invariants(malformedDocument.paragraphs.front(), malformedResult, Qt::LayoutDirectionAuto);
    CHECK(malformedResult.sourceMap[1] == Range{1, 1});
}

TEST_CASE("Unsupported constructs and exhausted budgets fail atomically before partial placement", "[textlayout][headless][limits]") {
    for (const auto &text : {QStringLiteral("ب\vب"), QStringLiteral("ب\ufffcب")}) {
        CAPTURE(text.toStdString());
        bool called = false;
        const auto result = Layout::paragraph(Document::plain(text).paragraphs.front(), [&](const Layout::FontRequest &) {
            called = true; return Layout::FontResolution{font(), {}, {}};
        });
        atomicError(result, Code::UnsupportedConstruct);
        CHECK_FALSE(called);
    }
    const auto document = Document::plain(QStringLiteral("ببب"));
    Layout::Options options;
    options.maximumInputUnits = 2;
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumGlyphs = 2;
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumOutlineCommands = 1;
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumRuns = 1;
    atomicError(Layout::paragraph(Document::plain(QStringLiteral("بA")).paragraphs.front(), resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumDiagnostics = 1;
    atomicError(Layout::paragraph(mtext(QStringLiteral("😀{\\H2x;😀}")).paragraphs.front(), resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumGlyphs = 0;
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::InvalidOptions);
    options = {};
    options.emptyStyle.obliqueDegrees = 90;
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::InvalidOptions);
    auto invalid = document.paragraphs.front();
    invalid.graphemes.pop_back();
    atomicError(place(invalid), Code::InvalidParagraph);
    invalid = document.paragraphs.front();
    invalid.scalars.front().text.length = 2;
    atomicError(place(invalid), Code::InvalidParagraph);
    invalid = document.paragraphs.front();
    invalid.spans.front().style.heightFactor = std::numeric_limits<double>::infinity();
    atomicError(place(invalid), Code::InvalidParagraph);
    invalid = document.paragraphs.front();
    invalid.spans.front().style.heightFactor = 1e101;
    atomicError(place(invalid), Code::NumericRange);
}

TEST_CASE("Mixed paragraph corpus retains complete source and glyph ownership", "[textlayout][headless][corpus]") {
    const std::vector<QString> pieces{QStringLiteral("ب"), QStringLiteral("لَا"), QStringLiteral("بَّ"),
        QStringLiteral("A"), QStringLiteral("12"), QStringLiteral("(ب)"), QStringLiteral(" "),
        QStringLiteral("\u200d"), QStringLiteral("\u200c"), QStringLiteral("😀"), QStringLiteral("一"),
        QStringLiteral("\u2067A\u2069"), QStringLiteral("\u2067\u2069")};
    for (std::size_t first = 0; first < pieces.size(); ++first) for (std::size_t second = 0; second < pieces.size(); ++second) {
        const QString text = pieces[first] + pieces[second] + pieces[(first * 7 + second * 3) % pieces.size()];
        const auto direction = first % 2 ? Qt::RightToLeft : Qt::LeftToRight;
        CAPTURE(text.toStdString(), int(direction));
        const auto document = Document::plain(text);
        invariants(document.paragraphs.front(), place(document.paragraphs.front(), direction), direction);
    }
}

TEST_CASE("Tabs use shaped whole-line fields and the resolved paragraph flow edge", "[textlayout][headless][tabs]") {
    const double a = place(QStringLiteral("A")).metrics.logicalAdvance;
    const double b = place(QStringLiteral("B")).metrics.logicalAdvance;
    for (auto direction : {Qt::LeftToRight, Qt::RightToLeft}) {
        const auto document = Document::plain(QStringLiteral("AAA\tB"));
        const auto result = place(document.paragraphs.front(), direction);
        invariants(document.paragraphs.front(), result, direction);
        REQUIRE(result.fragments.size() == 3);
        REQUIRE(result.glyphs.size() == 4);
        CHECK(result.fragments[1].tab);
        CHECK(result.fragments[1].glyphs.empty());
        CHECK(result.fragments[1].sources == std::vector<Range>{{3, 1}});
        CHECK(result.metrics.logicalAdvance == Catch::Approx(36 + b));
        if (direction == Qt::LeftToRight) {
            CHECK(result.fragments[0].origin.x() == 0);
            CHECK(result.fragments[2].origin.x() == 36);
        } else {
            CHECK(result.fragments[2].origin.x() == 0);
            CHECK(result.fragments[0].origin.x() == Catch::Approx(36 + b - 3 * a));
        }
        CHECK(result.fragments[1].advance.x() == Catch::Approx(36 - 3 * a));
        sampledInkWithinBounds(result);
    }
    const std::vector<QString> cases{QStringLiteral("\t"), QStringLiteral("\t\t"), QStringLiteral("\tA\t\t"),
        QStringLiteral("ب\tA"), QStringLiteral("A\tب"), QStringLiteral("ب\u200d\tA"),
        QStringLiteral("ب\u2067A\tب\u2069B"), QStringLiteral("ب\u2067\u2066A\t12\u2069\u2069ب"),
        QStringLiteral("中，\tA ب"), QStringLiteral(" A\tBBB\tب ")};
    for (const auto &text : cases) for (auto direction : {Qt::LeftToRight, Qt::RightToLeft, Qt::LayoutDirectionAuto}) {
        CAPTURE(text.toStdString(), int(direction));
        const auto document = Document::plain(text);
        const auto result = place(document.paragraphs.front(), direction);
        invariants(document.paragraphs.front(), result, direction);
        for (const auto &fragment : result.fragments) if (fragment.tab) {
            CHECK(fragment.text.length == 1);
            CHECK(fragment.glyphs.empty());
            CHECK(fragment.object == -1);
            CHECK(fragment.advance.x() > 0);
        }
    }
    const auto onlyTabs = place(QStringLiteral("\t\t"));
    CHECK(onlyTabs.metrics.logicalAdvance == 72);
    CHECK_FALSE(onlyTabs.metrics.hasInk);
    const auto autoRtl = place(QStringLiteral("ب\tA"));
    REQUIRE(autoRtl.baseLevel == 1);
    CHECK(autoRtl.metrics.logicalAdvance == Catch::Approx(36 + a));
    CHECK(autoRtl.fragments.back().origin.x() == 0);
}

TEST_CASE("Tab stops handle exact, near and unrepresentable progress without partial results", "[textlayout][headless][tabs][limits]") {
    const auto document = Document::plain(QStringLiteral("A\tA"));
    const double width = place(QStringLiteral("A")).metrics.logicalAdvance;
    Layout::Options options;
    options.tabInterval = width;
    auto result = Layout::paragraph(document.paragraphs.front(), resolver(), options);
    REQUIRE(result);
    CHECK(result.metrics.logicalAdvance == Catch::Approx(3 * width));
    options.tabInterval = std::nextafter(width, std::numeric_limits<double>::infinity());
    result = Layout::paragraph(document.paragraphs.front(), resolver(), options);
    REQUIRE(result);
    CHECK(result.metrics.logicalAdvance == Catch::Approx(2 * width));
    options.tabInterval = std::nextafter(width, 0.0);
    result = Layout::paragraph(document.paragraphs.front(), resolver(), options);
    REQUIRE(result);
    CHECK(result.metrics.logicalAdvance == Catch::Approx(3 * width));
    options.tabInterval = std::numeric_limits<double>::denorm_min();
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::NumericRange);
    options.tabInterval = 0;
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::InvalidOptions);
    options.tabInterval = std::numeric_limits<double>::quiet_NaN();
    atomicError(Layout::paragraph(document.paragraphs.front(), resolver(), options), Code::InvalidOptions);
}

TEST_CASE("Parsed stack separators retain recursive source, paint, fonts and authoritative widths", "[textlayout][headless][stacks]") {
    for (const QChar separator : {QChar('^'), QChar('/'), QChar('#')}) {
        const auto document = mtext(QStringLiteral("\\C1;ب\\Fouter;\\S{\\Fupper;\\C2;بَّب}") + separator +
                                    QStringLiteral("{\\Flower;\\C3;A };ب"));
        REQUIRE(document.ok());
        std::set<QString> requested;
        const auto result = Layout::paragraph(document.paragraphs.front(), [&](const Layout::FontRequest &request) {
            requested.insert(request.style.font);
            return Layout::FontResolution{font(), {}, {}};
        });
        invariants(document.paragraphs.front(), result, Qt::LayoutDirectionAuto);
        REQUIRE(result.objects.size() == 1);
        REQUIRE(result.glyphs.size() == 2);
        CHECK(ids(result) == std::vector<std::uint32_t>{56, 56});
        CHECK(requested.count(QStringLiteral("upper")) == 1);
        CHECK(requested.count(QStringLiteral("lower")) == 1);
        const auto &object = result.objects.front();
        REQUIRE(object.upper);
        REQUIRE(object.lower);
        const auto &input = document.paragraphs.front().objects.front();
        CHECK(object.text == Range{1, 1});
        CHECK(object.source == input.source);
        CHECK(object.separator == separator.toLatin1());
        CHECK(object.paint.color == 1);
        CHECK(object.upper->source == input.upper->sourceRange);
        CHECK(object.lower->source == input.lower->sourceRange);
        invariants(input.upper->paragraphs.front(), object.upper->lines.front().paragraph, Qt::LayoutDirectionAuto);
        invariants(input.lower->paragraphs.front(), object.lower->lines.front().paragraph, Qt::LayoutDirectionAuto);
        CHECK(object.upperTransform.xx == Catch::Approx(4.0 / 9));
        CHECK(object.lowerTransform.yy == Catch::Approx(4.0 / 9));
        const double upper = object.upper->metrics.logicalAdvance * 4 / 9;
        const double lower = object.lower->metrics.logicalAdvance * 4 / 9;
        CHECK(object.advance == Catch::Approx(separator == '#' ? upper + 3 + lower : std::max(upper, lower)));
        CHECK(object.separators.size() == (separator == '^' ? 0 : 1));
        for (const auto &stroke : object.separators) {
            CHECK(stroke.source == object.source);
            CHECK(stroke.paint == object.paint);
            if (separator == '/') CHECK(stroke.start.y() == stroke.end.y());
            else { CHECK(stroke.end.y() > stroke.start.y()); CHECK(stroke.end.x() > stroke.start.x()); }
        }
        const double upperBottom = object.upperTransform.dy + object.upper->metrics.inkBounds.top() * 4 / 9;
        const double lowerTop = object.lowerTransform.dy + object.lower->metrics.inkBounds.bottom() * 4 / 9;
        CHECK(upperBottom == Catch::Approx(5));
        CHECK(lowerTop == Catch::Approx(4));
        CHECK(result.metrics.logicalAdvance == Catch::Approx(object.advance + 2 * result.glyphs[0].xAdvance * (9.0 / 647)));
        sampledInkWithinBounds(result);
        CHECK(document.source.contains(QStringLiteral("\\S")));
    }
}

TEST_CASE("Nested stacks compose inherited height width oblique and empty child advances once", "[textlayout][headless][stacks][metrics]") {
    const auto document = mtext(QStringLiteral("\\H2x;\\W0.3;\\Q-35;\\S{\\Sب/ب;}#A ;\tب"));
    REQUIRE(document.ok());
    const auto result = place(document.paragraphs.front(), Qt::RightToLeft);
    invariants(document.paragraphs.front(), result, Qt::RightToLeft);
    REQUIRE(result.objects.size() == 1);
    const auto &outer = result.objects.front();
    REQUIRE(outer.upper->lines.front().paragraph.objects.size() == 1);
    const auto &inner = outer.upper->lines.front().paragraph.objects.front();
    CHECK(inner.upperTransform.xx * outer.upperTransform.xx == Catch::Approx(16.0 / 81));
    const auto &leaf = inner.upper->lines.front().paragraph.glyphs.front();
    CHECK(leaf.transform.yy == Catch::Approx((9.0 / 647) * 2));
    CHECK(leaf.transform.xx == Catch::Approx((9.0 / 647) * 2 * 0.3));
    sampledInkWithinBounds(result);
    for (const auto &source : {QStringLiteral("\\S^;"), QStringLiteral("\\S/;"), QStringLiteral("\\S#;")}) {
        const auto empty = mtext(source);
        REQUIRE(empty.ok());
        const auto layout = place(empty.paragraphs.front());
        REQUIRE(layout);
        REQUIRE(layout.objects.size() == 1);
        CHECK(layout.metrics.logicalAdvance == 0);
        CHECK_FALSE(layout.metrics.hasInk);
        CHECK_FALSE(layout.objects.front().hasInk);
        CHECK(layout.objects.front().separators.empty());
    }
    const auto whitespace = mtext(QStringLiteral("\\S   ^ ;"));
    const auto whitespaceLayout = place(whitespace.paragraphs.front());
    REQUIRE(whitespaceLayout);
    CHECK_FALSE(whitespaceLayout.metrics.hasInk);
    CHECK(whitespaceLayout.metrics.logicalAdvance == Catch::Approx(3 * 292 * (9.0 / 647) * 4 / 9));
    const auto styledEmpty = mtext(QStringLiteral("\\H2x;\\S^;"));
    const auto styledEmptyLayout = place(styledEmpty.paragraphs.front());
    REQUIRE(styledEmptyLayout);
    CHECK(styledEmptyLayout.objects.front().upper->lines.front().paragraph.metrics.nominalCellHeight == 18);
    CHECK(styledEmptyLayout.objects.front().lower->lines.front().paragraph.metrics.nominalCellHeight == 18);
}

TEST_CASE("Explicit multiline blocks retain empty lines and separate Exact from AtLeast pitch", "[textlayout][headless][block]") {
    const auto document = Document::plain(QStringLiteral("\nب\n\nب\n"));
    Layout::Options options;
    options.lineSpacing = Layout::LineSpacing::Exact;
    options.linePitch = 7;
    const auto exact = Layout::document(document, resolver(), options);
    INFO(exact.error.message.toStdString());
    REQUIRE(exact);
    REQUIRE(exact.lines.size() == 5);
    CHECK(exact.baselineSpan == 28);
    for (std::size_t index = 0; index < exact.lines.size(); ++index) {
        CHECK(exact.lines[index].origin.y() == -7 * double(index));
        CHECK(exact.lines[index].paragraph.source == document.paragraphs[index].source);
        CHECK(exact.lines[index].paragraph.breakSource == document.paragraphs[index].breakSource);
        CHECK(exact.lines[index].paragraph.text == document.paragraphs[index].text);
    }
    CHECK(exact.lines.front().paragraph.emptyFont == font());
    CHECK_FALSE(exact.lines.front().paragraph.metrics.hasInk);
    CHECK_FALSE(exact.lines.back().paragraph.metrics.hasInk);
    sampledInkWithinBounds(exact);
    options.lineSpacing = Layout::LineSpacing::AtLeast;
    const auto atLeast = Layout::document(document, resolver(), options);
    REQUIRE(atLeast);
    const double pitch = 1758 * (9.0 / 647);
    CHECK(atLeast.baselineSpan == Catch::Approx(4 * pitch));
    CHECK(atLeast.metrics.logicalAdvance == exact.metrics.logicalAdvance);
    CHECK(atLeast.metrics.profileLinePitch == exact.metrics.profileLinePitch);
    const auto marked = mtext(QStringLiteral("ب\\P{\\H3x;بَّ}\\Pب"));
    options.lineSpacing = Layout::LineSpacing::Exact;
    const auto markedExact = Layout::document(marked, resolver(), options);
    REQUIRE(markedExact);
    CHECK(markedExact.lines[1].origin.y() == -7);
    CHECK(markedExact.lines[2].origin.y() == -14);
    options.lineSpacing = Layout::LineSpacing::AtLeast;
    const auto markedAtLeast = Layout::document(marked, resolver(), options);
    REQUIRE(markedAtLeast);
    for (std::size_t index = 1; index < markedAtLeast.lines.size(); ++index) {
        const auto &previous = markedAtLeast.lines[index - 1].paragraph.metrics;
        const auto &next = markedAtLeast.lines[index].paragraph.metrics;
        const double expected = std::max({7.0, previous.profileLinePitch, next.profileLinePitch,
            std::max(previous.descent, previous.inkBelowBaseline) + std::max(next.ascent, next.inkAboveBaseline)});
        CHECK(markedAtLeast.lines[index - 1].origin.y() - markedAtLeast.lines[index].origin.y() == Catch::Approx(expected));
    }
    const auto scoped = mtext(QStringLiteral("{\\H2x;\\P\\P}ب"));
    options.linePitch = 0;
    options.lineSpacing = Layout::LineSpacing::Exact;
    const auto scopedLayout = Layout::document(scoped, resolver(), options);
    REQUIRE(scopedLayout);
    CHECK(scopedLayout.lines[0].paragraph.metrics.nominalCellHeight == 18);
    CHECK(scopedLayout.lines[1].paragraph.metrics.nominalCellHeight == 18);
    CHECK(scopedLayout.lines[2].paragraph.metrics.nominalCellHeight == 9);
    CHECK(scopedLayout.lines[1].origin.y() == Catch::Approx(-pitch));
    CHECK(scopedLayout.lines[2].origin.y() == Catch::Approx(-2 * pitch));
}

TEST_CASE("Recursive budgets and invalid stack metadata fail without successful partial blocks", "[textlayout][headless][stacks][limits]") {
    const auto document = mtext(QStringLiteral("ب\\S{\\Sب^ب;}^ب;"));
    REQUIRE(document.ok());
    Layout::Options options;
    options.maximumStackDepth = 1;
    atomicError(Layout::document(document, resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumObjects = 1;
    atomicError(Layout::document(document, resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumParagraphs = 2;
    atomicError(Layout::document(document, resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumInputUnits = 3;
    atomicError(Layout::document(document, resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumGlyphs = 2;
    atomicError(Layout::document(document, resolver(), options), Code::ComplexityLimit);
    options = {};
    options.maximumOutlineCommands = 1;
    atomicError(Layout::document(document, resolver(), options), Code::ComplexityLimit);
    options = {};
    options.stack.childScale = 0;
    atomicError(Layout::document(document, resolver(), options), Code::InvalidOptions);
    auto invalid = document;
    invalid.paragraphs.front().objects.front().separator = '?';
    atomicError(Layout::document(invalid, resolver()), Code::InvalidParagraph);
    invalid = document;
    invalid.paragraphs.front().objects.front().upper.reset();
    atomicError(Layout::document(invalid, resolver()), Code::InvalidParagraph);
    invalid = document;
    invalid.paragraphs.front().source.length = 0;
    atomicError(Layout::document(invalid, resolver()), Code::InvalidParagraph);
    auto empty = Document::plain(QString{});
    empty.paragraphs.front().emptyStyle->language.clear();
    atomicError(Layout::document(empty, resolver()), Code::InvalidParagraph);
    empty = Document::plain(QString{});
    empty.paragraphs.front().emptyStyle->widthFactor = -1;
    atomicError(place(empty.paragraphs.front()), Code::InvalidParagraph);
    const auto multiline = Document::plain(QStringLiteral("ب\nب"));
    options = {};
    options.linePitch = 1e101;
    atomicError(Layout::document(multiline, resolver(), options), Code::InvalidOptions);
}

TEST_CASE("Recursive block provenance remains bound to the root original source", "[textlayout][headless][stacks][provenance-root]") {
    const auto document = mtext(QStringLiteral("A\\P\\Sب^A;\\PB"));
    REQUIRE(document.ok());
    REQUIRE(document.paragraphs[1].objects.size() == 1);
    QString altered = document.source;
    altered[0] = QLatin1Char('Z'); // Outside both child ranges; the whole original still must agree.
    REQUIRE(altered.size() == document.source.size());
    REQUIRE(altered != document.source);
    const auto replaceChildren = [](Document::InlineObject &object, const QString &source) {
        auto upper = std::make_shared<Document>(*object.upper);
        auto lower = std::make_shared<Document>(*object.lower);
        upper->source = source;
        lower->source = source;
        object.upper = upper;
        object.lower = lower;
    };
    SECTION("Equal-sized altered child originals reject the whole block after a valid earlier line") {
        auto invalid = document;
        auto &object = invalid.paragraphs[1].objects.front();
        replaceChildren(object, altered);
        const auto result = Layout::document(invalid, resolver());
        atomicError(result, Code::InvalidParagraph);
        CHECK(result.error.message.contains(QStringLiteral("canonical original source")));
        CHECK(result.error.sources == std::vector<Range>{object.upper->sourceRange});
        CHECK(invalid.source == document.source);
        // This entry point has no outer original to compare. It can establish
        // child consistency, but cannot detect alteration relative to the root.
        REQUIRE(place(invalid.paragraphs[1]));
    }
    SECTION("Equal original values with separate QString storage remain valid") {
        auto copied = document;
        QString independent = document.source;
        independent.detach();
        REQUIRE(independent == document.source);
        REQUIRE(independent.constData() != document.source.constData());
        replaceChildren(copied.paragraphs[1].objects.front(), independent);
        const auto result = Layout::document(copied, resolver());
        REQUIRE(result);
        REQUIRE(result.lines.size() == document.paragraphs.size());
        sampledInkWithinBounds(result);
        // A parsed child can also be the document entry point itself.
        REQUIRE(Layout::document(*copied.paragraphs[1].objects.front().upper, resolver()));
    }
    SECTION("An altered outer original cannot retain old children") {
        auto invalid = document;
        invalid.source = altered;
        atomicError(Layout::document(invalid, resolver()), Code::InvalidParagraph);
    }
    SECTION("One mismatched sibling and zero-ink children also reject") {
        auto invalid = document;
        auto upper = std::make_shared<Document>(*invalid.paragraphs[1].objects.front().upper);
        upper->source = altered;
        invalid.paragraphs[1].objects.front().upper = upper;
        atomicError(Layout::document(invalid, resolver()), Code::InvalidParagraph);
        auto empty = mtext(QStringLiteral("\\S^;"));
        REQUIRE(empty.ok());
        QString different = empty.source;
        different[0] = QLatin1Char('X');
        replaceChildren(empty.paragraphs.front().objects.front(), different);
        atomicError(Layout::document(empty, resolver()), Code::InvalidParagraph);
    }
    SECTION("Nested descendants retain the same original even when both siblings agree") {
        auto nested = mtext(QStringLiteral("A\\P\\S{\\Sب^A;}^B;"));
        REQUIRE(nested.ok());
        auto &outer = nested.paragraphs[1].objects.front();
        auto upper = std::make_shared<Document>(*outer.upper);
        REQUIRE(upper->paragraphs.front().objects.size() == 1);
        QString different = nested.source;
        different[0] = QLatin1Char('Z');
        replaceChildren(upper->paragraphs.front().objects.front(), different);
        outer.upper = upper;
        atomicError(Layout::document(nested, resolver()), Code::InvalidParagraph);
        // paragraph() can compare nested children with its first child's source.
        atomicError(place(nested.paragraphs[1]), Code::InvalidParagraph);
    }
    SECTION("Separate paragraph objects cannot claim different child originals") {
        auto siblings = mtext(QStringLiteral("A\\Sب^A;\\Sب^A;"));
        REQUIRE(siblings.ok());
        REQUIRE(siblings.paragraphs.front().objects.size() == 2);
        QString different = siblings.source;
        different[0] = QLatin1Char('Z');
        replaceChildren(siblings.paragraphs.front().objects[1], different);
        atomicError(Layout::document(siblings, resolver()), Code::InvalidParagraph);
        atomicError(place(siblings.paragraphs.front()), Code::InvalidParagraph);
    }
}
#endif
