/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTLAYOUT_H
#define LC_TEXTLAYOUT_H

#include "lc_textruns.h"
#include "lc_opentypefont.h"
#include <QPointF>
#include <QRectF>
#include <functional>
#include <memory>
#include <vector>

/** QtCore-only placement of explicit horizontal paragraphs/lines.
 * Does not wrap, create native entities, cache layouts or mutate source.
 * Parsed stacks are recursive layout objects. Tabs use whole-line bidi fields;
 * custom tab/paragraph properties and literal unparsed separators are not guessed.
 */
class LC_TextLayout final {
public:
    using Range = LC_TextDocument::Range;
    struct FontRequest {
        LC_TextRuns::ShapingStyle style;
        Range text; // Owned paragraph-local UTF-16, never borrowed context.
        Range context;
        std::vector<Range> sources;
    };
    struct FontResolution {
        std::shared_ptr<const LC_OpenTypeFont> font;
        QString error;
        LC_OpenTypeFont::Error fontError;
        explicit operator bool() const { return font && error.isEmpty() && !fontError; }
    };
    // The caller enforces the saved profile/engine policy (e.g. via the font
    // registry). No system-font, LFF-name or locale substitution occurs here.
    using FontResolver = std::function<FontResolution(const FontRequest &)>;
    enum class LineSpacing { Exact, AtLeast };
    struct StackPolicy {
        // Explicit initial opt-in stack-v1 policy, not legacy/CAD equivalence.
        double childScale = 4.0 / 9.0;
        double axis = 4.5; // Parent nominal cell coordinates, scaled by inline H.
        double gap = 1.0;
        double diagonalWidth = 3.0; // Also scaled by the parent's inline W.
    };
    struct Options {
        Qt::LayoutDirection direction = Qt::LayoutDirectionAuto;
        // Spanless empty paragraphs still need an explicitly resolved font.
        LC_TextDocument::Style emptyStyle;
        double tabInterval = 36.0; // Four initial 9-cell heights; never pixels.
        LineSpacing lineSpacing = LineSpacing::AtLeast;
        // Explicit local pitch. Zero derives the base font/profile pitch, not
        // the tallest mark. Entity line-spacing factors are the caller's input.
        double linePitch = 0;
        StackPolicy stack;
        int maximumInputUnits = 1024 * 1024;
        std::size_t maximumGlyphs = 262144;
        std::size_t maximumRuns = 65536;
        std::size_t maximumOutlineCommands = 4 * 1024 * 1024;
        std::size_t maximumDiagnostics = 1024;
        std::size_t maximumObjects = 4096;
        std::size_t maximumParagraphs = 65536;
        int maximumStackDepth = 16;
    };
    enum class ErrorCode {
        None, BackendUnavailable, InvalidOptions, InvalidParagraph,
        UnsupportedConstruct, FontUnavailable, ShapingFailure,
        UnsupportedCluster, UnsupportedMetrics, OutlineFailure,
        NumericRange, ComplexityLimit
    };
    struct Error {
        ErrorCode code = ErrorCode::None;
        QString message;
        Range text;
        std::vector<Range> sources;
        LC_OpenTypeFont::Error fontError;
        explicit operator bool() const { return code != ErrorCode::None; }
    };
    struct Diagnostic {
        enum class Code { UnsupportedCoverage };
        Code code = Code::UnsupportedCoverage;
        std::vector<Range> textRanges;
        std::vector<Range> sources;
        QString message;
    };
    struct Affine {
        // Font coordinates -> local nominal 9-cell coordinates. Includes inline
        // height/width/oblique, glyph pen and HB offsets, not entity transforms.
        double xx = 1, xy = 0, yx = 0, yy = 1, dx = 0, dy = 0;
    };
    struct LogicalRun {
        LC_TextRuns::Run item;
        QByteArray script; // Explicit ISO 15924 tag, never a cast Qt enum.
        std::shared_ptr<const LC_OpenTypeFont> font;
    };
    struct Cluster {
        Range text; // Whole HB logical cluster interval, not an editor caret map.
        std::vector<Range> sources;
        std::vector<std::size_t> graphemes; // Indices in Result.graphemes.
        LC_TextDocument::Style paint; // First logical component's effective pen.
    };
    struct Glyph {
        std::shared_ptr<const LC_OpenTypeFont> font;
        std::shared_ptr<const LC_OpenTypeFont::Outline> outline;
        std::size_t logicalRun = 0;
        std::size_t cluster = 0;
        std::uint32_t id = 0;
        int xAdvance = 0, yAdvance = 0, xOffset = 0, yOffset = 0;
        bool unsafeToBreak = false;
        bool drawable = false;
        Affine transform;
    };
    struct BlockResult;
    struct Stroke {
        QPointF start, end;
        Range source; // Whole parsed stack spelling, never fabricated Unicode.
        LC_TextDocument::Style paint;
    };
    struct PlacedObject {
        Range text, source;
        LC_TextDocument::Style paint;
        char separator = '^';
        std::shared_ptr<const BlockResult> upper, lower;
        Affine upperTransform, lowerTransform; // Child block -> this paragraph.
        std::vector<Stroke> separators;
        QPointF origin;
        double advance = 0;
        bool hasInk = false;
        QRectF inkBounds; // Paragraph-local after final placement.
        double ascent = 0, descent = 0; // Profile extents, separate from ink.
    };
    struct Fragment {
        Range text; // Disjoint ownership, including zero-ink controls.
        std::vector<Range> sources;
        std::size_t logicalRun = 0;
        int displayLevel = 0;
        QPointF origin;
        QPointF advance;
        // Every equal-cluster group is atomic; indices preserve HB order.
        std::vector<std::size_t> glyphs;
        int object = -1; // Index in Result.objects; never a glyph/cluster index.
        bool tab = false;
    };
    struct Metrics {
        double logicalAdvance = 0; // Horizontal interval [0, logicalAdvance].
        bool hasInk = false;
        QRectF inkBounds; // Exact local contour extrema; no-ink uses hasInk=false.
        double baseline = 0;
        double ascent = 0, descent = 0;
        double nominalCellHeight = 0;
        double profileLinePitch = 0;
        double inkAboveBaseline = 0, inkBelowBaseline = 0;
    };
    struct Result {
        QString text;
        Range source, breakSource;
        int baseLevel = 0;
        QChar::UnicodeVersion unicodeVersion = QChar::currentUnicodeVersion();
        QString shapingEngine;
        std::shared_ptr<const LC_OpenTypeFont> emptyFont; // Explicit spanless-line resource.
        std::vector<Range> sourceMap;
        std::vector<Range> graphemes; // Independent from shaping clusters.
        std::vector<LogicalRun> logicalRuns;
        std::vector<Cluster> clusters;
        std::vector<Glyph> glyphs; // Logical items, HB order within each item.
        std::vector<PlacedObject> objects;
        std::vector<Fragment> fragments; // Logical ownership order.
        std::vector<std::size_t> visualFragments; // Final left-to-right order.
        Metrics metrics;
        std::vector<Diagnostic> diagnostics;
        Error error;
        explicit operator bool() const { return !error; }
    };
    struct Line {
        Result paragraph;
        QPointF origin; // First baseline is zero; subsequent baselines descend.
    };
    struct BlockResult {
        Range source;
        std::shared_ptr<const LC_OpenTypeFont> baseFont; // Default pitch identity.
        std::vector<Line> lines; // Original explicit empty paragraphs retained.
        Metrics metrics;
        double baselineSpan = 0;
        Error error;
        explicit operator bool() const { return !error; }
    };

    // Error results contain no partial runs, fragments, glyphs or metrics.
    // Unsupported coverage uses the same exact font's visible .notdef and a
    // source diagnostic; approved complete-segment fallback is a later slice.
    // A paragraph has no original-source string: callers retain that contract.
    // Its parsed stack descendants must still share one canonical original.
    static Result paragraph(const LC_TextDocument::Paragraph &paragraph,
                            const FontResolver &resolver, const Options &options);
    static Result paragraph(const LC_TextDocument::Paragraph &paragraph,
                            const FontResolver &resolver) {
        return LC_TextLayout::paragraph(paragraph, resolver, Options{});
    }
    // Every recursive parsed block must retain the root's whole original source,
    // not merely an equal-sized string or matching text within its sourceRange.
    static BlockResult document(const LC_TextDocument &document,
                                const FontResolver &resolver, const Options &options);
    static BlockResult document(const LC_TextDocument &document, const FontResolver &resolver) {
        return LC_TextLayout::document(document, resolver, Options{});
    }
private:
    struct Context;
    static Result paragraphImpl(const LC_TextDocument::Paragraph &, Context &, const Options &, int depth);
    static BlockResult blockImpl(const LC_TextDocument &, Context &, const Options &, int depth);
};

#endif
