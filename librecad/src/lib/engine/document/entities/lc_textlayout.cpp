/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textlayout.h"
#include "lc_textshaper.h"
#include "qt_bidi.h"

#include <QTextBoundaryFinder>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
#include <hb.h>
#endif

namespace {
using Layout = LC_TextLayout;
using Range = Layout::Range;
using Code = Layout::ErrorCode;

Layout::Result failure(Code code, const QString &message, Range text = {},
                       std::vector<Range> sources = {}, LC_OpenTypeFont::Error fontError = {}) {
    Layout::Result result;
    result.error = {code, message, text, std::move(sources), std::move(fontError)};
    return result;
}
Layout::BlockResult blockFailure(Layout::Error error) {
    Layout::BlockResult result;
    result.error = std::move(error);
    return result;
}

#if defined(LC_WITH_OPENTYPE) && LC_WITH_OPENTYPE
constexpr int hardInputLimit = 1024 * 1024;
constexpr std::size_t hardGlyphLimit = 2 * 1024 * 1024;
constexpr std::size_t hardRunLimit = 65536;
constexpr std::size_t hardCommandLimit = 16 * 1024 * 1024;
constexpr std::size_t maximumFonts = 256;
constexpr double coordinateLimit = 1e100; // Local arithmetic, not a native/document range claim.

bool number(double value) {
    return std::isfinite(value) && std::abs(value) <= coordinateLimit;
}
bool point(const QPointF &value) { return number(value.x()) && number(value.y()); }
bool validStyle(const LC_TextDocument::Style &style) {
    return std::isfinite(style.heightFactor) && style.heightFactor > 0 &&
           std::isfinite(style.widthFactor) && style.widthFactor > 0 &&
           std::isfinite(style.obliqueDegrees) && std::abs(style.obliqueDegrees) < 90 &&
           !style.language.isEmpty() && style.language.size() <= 63 && !style.language.contains('\0');
}
bool validOptions(const Layout::Options &options) {
    return options.maximumInputUnits > 0 && options.maximumInputUnits <= hardInputLimit &&
        options.maximumGlyphs && options.maximumGlyphs <= hardGlyphLimit &&
        options.maximumRuns && options.maximumRuns <= hardRunLimit &&
        options.maximumOutlineCommands && options.maximumOutlineCommands <= hardCommandLimit &&
        options.maximumDiagnostics && options.maximumDiagnostics <= hardRunLimit &&
        options.maximumObjects && options.maximumObjects <= hardRunLimit &&
        options.maximumParagraphs && options.maximumParagraphs <= hardRunLimit &&
        options.maximumStackDepth >= 0 && options.maximumStackDepth <= 64 &&
        validStyle(options.emptyStyle) && number(options.tabInterval) && options.tabInterval > 0 &&
        number(options.linePitch) && options.linePitch >= 0 &&
        (options.lineSpacing == Layout::LineSpacing::Exact || options.lineSpacing == Layout::LineSpacing::AtLeast) &&
        number(options.stack.childScale) && options.stack.childScale > 0 && options.stack.childScale <= 1 &&
        number(options.stack.axis) && options.stack.axis >= 0 && number(options.stack.gap) && options.stack.gap >= 0 &&
        number(options.stack.diagonalWidth) && options.stack.diagonalWidth > 0;
}
LC_TextRuns::ShapingStyle shapingStyle(const LC_TextDocument::Style &style) {
    return {style.font, style.fontSpecification, style.language,
            style.heightFactor, style.widthFactor, style.obliqueDegrees};
}
void appendSource(std::vector<Range> &output, Range range) {
    if (range.length <= 0) return;
    if (!output.empty() && range.start >= output.back().start && range.end() <= output.back().end()) return;
    if (!output.empty() && output.back().end() == range.start) output.back().length += range.length;
    else output.push_back(range);
}
std::vector<Range> graphemes(const QString &text) {
    std::vector<Range> result;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    for (int start = 0, end = int(finder.toNextBoundary()); end >= 0;
         start = end, end = int(finder.toNextBoundary())) result.push_back({start, end - start});
    return result;
}
QByteArray scriptTag(const LC_TextRuns::Run &run) {
    // All-Common contexts use the explicit Arabic policy of this initial
    // profile. Otherwise derive ISO 15924 from a real resolved-script scalar.
    const auto script = run.representativeScalar
        ? hb_unicode_script(hb_unicode_funcs_get_default(), run.representativeScalar) : HB_SCRIPT_ARABIC;
    if (script == HB_SCRIPT_INVALID || script == HB_SCRIPT_UNKNOWN) return {};
    const hb_tag_t tag = hb_script_to_iso15924_tag(script);
    const char bytes[]{char(tag >> 24), char(tag >> 16), char(tag >> 8), char(tag)};
    return QByteArray(bytes, 4);
}
bool linear(const LC_OpenTypeFont &font, const LC_TextRuns::ShapingStyle &style, Layout::Affine &result) {
    const double height = font.normalization() * style.heightFactor;
    const double width = height * style.widthFactor;
    const double shear = width * std::tan(style.obliqueDegrees * (std::acos(-1.0) / 180.0));
    if (!number(height) || !number(width) || !number(shear) || height <= 0 || width <= 0) return false;
    result = {width, shear, 0, height, 0, 0};
    return true;
}
bool map(const Layout::Affine &a, const QPointF &p, QPointF &destination) {
    destination = {std::fma(a.xx, p.x(), std::fma(a.xy, p.y(), a.dx)),
                   std::fma(a.yx, p.x(), std::fma(a.yy, p.y(), a.dy))};
    return point(destination);
}

struct Bounds {
    bool hasInk = false;
    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    bool add(const QPointF &p) {
        if (!point(p)) return false;
        if (!hasInk) { minX = maxX = p.x(); minY = maxY = p.y(); hasInk = true; }
        else { minX = std::min(minX, p.x()); maxX = std::max(maxX, p.x());
               minY = std::min(minY, p.y()); maxY = std::max(maxY, p.y()); }
        return true;
    }
    bool translated(const Bounds &other, double x, double y) {
        return !other.hasInk || (add({other.minX + x, other.minY + y}) && add({other.maxX + x, other.maxY + y}));
    }
    QRectF rectangle() const { return hasInk ? QRectF(minX, minY, maxX - minX, maxY - minY) : QRectF{}; }
    bool scaled(const Layout::Metrics &metrics, double scale, double x, double y) {
        return !metrics.hasInk || (add({metrics.inkBounds.left() * scale + x, metrics.inkBounds.top() * scale + y}) &&
                                  add({metrics.inkBounds.right() * scale + x, metrics.inkBounds.bottom() * scale + y}));
    }
};
QPointF interpolate(const QPointF &a, const QPointF &b, double t) {
    return {std::fma(b.x(), t, a.x() * (1 - t)), std::fma(b.y(), t, a.y() * (1 - t))};
}
QPointF quadratic(const QPointF &a, const QPointF &b, const QPointF &c, double t) {
    return interpolate(interpolate(a, b, t), interpolate(b, c, t), t);
}
QPointF cubic(const QPointF &a, const QPointF &b, const QPointF &c, const QPointF &d, double t) {
    return interpolate(quadratic(a, b, c, t), quadratic(b, c, d, t), t);
}
template<typename Evaluate>
bool cubicExtrema(double p0, double p1, double p2, double p3, Evaluate evaluate) {
    long double a = -static_cast<long double>(p0) + 3 * static_cast<long double>(p1) -
                    3 * static_cast<long double>(p2) + p3;
    long double b = 2 * (static_cast<long double>(p0) - 2 * static_cast<long double>(p1) + p2);
    long double c = static_cast<long double>(p1) - p0;
    const long double scale = std::max({std::abs(a), std::abs(b), std::abs(c)});
    if (scale == 0) return true;
    a /= scale; b /= scale; c /= scale;
    const auto root = [&](long double t) { return !(t > 0 && t < 1) || evaluate(static_cast<double>(t)); };
    if (a == 0) return b == 0 || root(-c / b);
    long double discriminant = std::fma(b, b, -4 * a * c);
    if (discriminant < 0) {
        if (discriminant < -64 * std::numeric_limits<long double>::epsilon()) return true;
        discriminant = 0; // Rounded double root; adding it is conservative.
    }
    const long double q = -0.5L * (b + std::copysign(std::sqrt(discriminant), b));
    return q == 0 ? root(-b / (2 * a)) : root(q / a) && root(c / q);
}
bool contourBounds(const LC_OpenTypeFont::Outline &outline, const Layout::Affine &affine, Bounds &bounds) {
    if (!outline.hasInk) return true;
    using Type = LC_OpenTypeFont::Command::Type;
    QPointF current, first, a, b, c;
    bool active = false;
    for (const auto &command : outline.commands) {
        switch (command.type) {
        case Type::MoveTo:
            if (active || !map(affine, command.first, current)) return false;
            first = current; active = true;
            break;
        case Type::LineTo:
            if (!active || !map(affine, command.first, a) || !bounds.add(current) || !bounds.add(a)) return false;
            current = a;
            break;
        case Type::QuadraticTo:
            if (!active || !map(affine, command.first, a) || !map(affine, command.second, b) ||
                !bounds.add(current) || !bounds.add(b)) return false;
            for (int dimension = 0; dimension < 2; ++dimension) {
                const double p0 = dimension ? current.y() : current.x();
                const double p1 = dimension ? a.y() : a.x();
                const double p2 = dimension ? b.y() : b.x();
                const long double denominator = static_cast<long double>(p0) - 2 * static_cast<long double>(p1) + p2;
                if (denominator != 0) {
                    const long double t = (static_cast<long double>(p0) - p1) / denominator;
                    if (t > 0 && t < 1 && !bounds.add(quadratic(current, a, b, static_cast<double>(t)))) return false;
                }
            }
            current = b;
            break;
        case Type::CubicTo:
            if (!active || !map(affine, command.first, a) || !map(affine, command.second, b) ||
                !map(affine, command.third, c) || !bounds.add(current) || !bounds.add(c)) return false;
            if (!cubicExtrema(current.x(), a.x(), b.x(), c.x(), [&](double t) { return bounds.add(cubic(current, a, b, c, t)); }) ||
                !cubicExtrema(current.y(), a.y(), b.y(), c.y(), [&](double t) { return bounds.add(cubic(current, a, b, c, t)); })) return false;
            current = c;
            break;
        case Type::Close:
            if (!active || !bounds.add(current) || !bounds.add(first)) return false;
            active = false;
            break;
        }
    }
    return !active && bounds.hasInk;
}
bool addAdvance(std::int64_t &value, int amount) {
    if ((amount > 0 && value > std::numeric_limits<std::int64_t>::max() - amount) ||
        (amount < 0 && value < std::numeric_limits<std::int64_t>::min() - amount)) return false;
    value += amount;
    return true;
}
#endif
} // namespace

struct LC_TextLayout::Context {
    const FontResolver &resolver;
    const Options &limits;
    std::size_t input = 0, runs = 0, glyphs = 0, commands = 0, diagnostics = 0, objects = 0, paragraphs = 0;
    std::set<const LC_TextDocument *> active;
    std::set<const LC_OpenTypeFont *> fonts;
    std::optional<QString> canonicalSource;
    bool consume(std::size_t &value, std::size_t amount, std::size_t maximum) {
        if (amount > maximum - value) return false;
        value += amount;
        return true;
    }
};

LC_TextLayout::Result LC_TextLayout::paragraph(const LC_TextDocument::Paragraph &document,
                                              const FontResolver &resolver, const Options &options) {
    Context context{resolver, options, 0, 0, 0, 0, 0, 0, 0, {}, {}, {}};
    return paragraphImpl(document, context, options, 0);
}

LC_TextLayout::Result LC_TextLayout::paragraphImpl(const LC_TextDocument::Paragraph &document,
                                                  Context &context, const Options &options, int depth) {
#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
    Q_UNUSED(document)
    Q_UNUSED(context)
    Q_UNUSED(options)
    Q_UNUSED(depth)
    return failure(Code::BackendUnavailable, QStringLiteral("OpenType paragraph layout is disabled in this build"));
#else
    if (!context.resolver || !validOptions(options))
        return failure(Code::InvalidOptions, QStringLiteral("Invalid paragraph layout resolver, style or resource limits"));
    if (!context.consume(context.paragraphs, 1, options.maximumParagraphs) ||
        !context.consume(context.input, document.text.size(), options.maximumInputUnits) || document.spans.size() > options.maximumRuns)
        return failure(Code::ComplexityLimit, QStringLiteral("The paragraph input or span budget was exceeded"));
    LC_TextRuns::Options itemOptions;
    itemOptions.direction = options.direction;
    const auto logical = LC_TextRuns::itemize(document, itemOptions);
    if (!logical.ok()) return failure(Code::InvalidParagraph, logical.error);
    const auto paragraphGraphemes = graphemes(document.text);
    if (document.graphemes != paragraphGraphemes)
        return failure(Code::InvalidParagraph, QStringLiteral("The grapheme map does not describe the complete logical paragraph"));
    if (!context.consume(context.runs, logical.runs.size(), options.maximumRuns))
        return failure(Code::ComplexityLimit, QStringLiteral("The logical run budget was exceeded"));
    for (const auto &run : logical.runs) {
        if (run.kind == LC_TextRuns::Kind::Separator || (run.kind == LC_TextRuns::Kind::Object && run.objectIndex < 0))
            return failure(Code::UnsupportedConstruct, QStringLiteral("Unparsed inline objects and line separators require an explicit source/layout policy"),
                           run.text, run.sources);
    }
    Result result;
    result.text = logical.text; result.source = logical.source; result.breakSource = logical.breakSource;
    result.baseLevel = logical.bidi.base; result.unicodeVersion = logical.unicodeVersion;
    result.shapingEngine = LC_OpenTypeFont::shapingEngineIdentity();
    result.sourceMap = logical.sourceMap; result.graphemes = paragraphGraphemes;
    std::vector<QString> contexts;
    contexts.reserve(logical.contexts.size());
    for (const auto &context : logical.contexts)
        contexts.push_back(logical.text.mid(context.shapingText.start, context.shapingText.length));
    std::vector<std::size_t> fragmentForUnit(logical.text.size());
    std::map<const LC_OpenTypeFont *, std::size_t> resources;
    using OutlineKey = std::pair<std::size_t, std::uint32_t>;
    using BoundsKey = std::tuple<std::size_t, std::uint32_t, double, double, double>;
    std::map<OutlineKey, std::shared_ptr<const LC_OpenTypeFont::Outline>> outlines;
    std::map<BoundsKey, Bounds> localBounds;
    std::vector<Bounds> glyphBounds;

    const auto resolve = [&](const FontRequest &request) {
        FontResolution resolved;
        try { resolved = context.resolver(request); }
        catch (...) { resolved.error = QStringLiteral("The explicit font resolver failed"); }
        return resolved;
    };
    const auto metrics = [&](const LC_OpenTypeFont &font, const LC_TextRuns::ShapingStyle &style) {
        const double factor = font.normalization() * style.heightFactor;
        const auto &profile = font.profile();
        const double ascent = profile.ascender * factor, descent = -static_cast<double>(profile.descender) * factor;
        const double pitch = profile.lineHeight * factor, cell = 9 * style.heightFactor;
        if (!number(ascent) || !number(descent) || !number(pitch) || !number(cell)) return false;
        result.metrics.ascent = std::max(result.metrics.ascent, ascent);
        result.metrics.descent = std::max(result.metrics.descent, descent);
        result.metrics.profileLinePitch = std::max(result.metrics.profileLinePitch, pitch);
        result.metrics.nominalCellHeight = std::max(result.metrics.nominalCellHeight, cell);
        return true;
    };
    if (logical.runs.empty()) {
        const auto &effective = document.emptyStyle ? *document.emptyStyle : options.emptyStyle;
        if (!validStyle(effective)) return failure(Code::InvalidParagraph, QStringLiteral("Invalid effective empty-paragraph style"));
        const auto style = shapingStyle(effective);
        const auto font = resolve({style, {}, {}, {}});
        if (!font) return failure(Code::FontUnavailable, font.error.isEmpty() ? QStringLiteral("The exact empty-paragraph font is unavailable") : font.error,
                                  {}, {}, font.fontError);
        if (!metrics(*font.font, style)) return failure(Code::NumericRange, QStringLiteral("The empty-paragraph profile metrics exceed the local numeric range"));
        context.fonts.insert(font.font.get());
        if (context.fonts.size() > maximumFonts) return failure(Code::ComplexityLimit, QStringLiteral("The font resource budget was exceeded"));
        result.emptyFont = font.font;
        return result;
    }

    const auto stackObject = [&](const LC_TextRuns::Run &run, PlacedObject &placed) -> Error {
        const auto &object = document.objects[run.objectIndex];
        const auto invalid = [&](Code code, const QString &message) {
            return Error{code, message, run.text, run.sources, {}};
        };
        if (depth >= options.maximumStackDepth || !context.consume(context.objects, 1, options.maximumObjects))
            return invalid(Code::ComplexityLimit, QStringLiteral("The recursive stack depth or object budget was exceeded"));
        if (!object.upper || !object.lower || (object.separator != '^' && object.separator != '/' && object.separator != '#'))
            return invalid(Code::InvalidParagraph, QStringLiteral("A parsed stack has invalid children or separator"));
        const auto childRange = [&](Range range) {
            return range.start >= object.source.start && range.length >= 0 &&
                range.length <= object.source.end() - range.start;
        };
        if (!childRange(object.upper->sourceRange) || !childRange(object.lower->sourceRange) ||
            object.upper->sourceRange.end() > object.lower->sourceRange.start ||
            !LC_TextDocument::sameText(object.upper->source, object.lower->source))
            return invalid(Code::InvalidParagraph, QStringLiteral("Stack children do not retain ordered original-source ownership"));
        placed.text = run.text; placed.source = object.source; placed.separator = object.separator;
        placed.paint = *document.styleAt(run.text.start);
        Options childOptions = options;
        childOptions.emptyStyle = placed.paint;
        auto upper = blockImpl(*object.upper, context, childOptions, depth + 1);
        if (!upper) return upper.error;
        auto lower = blockImpl(*object.lower, context, childOptions, depth + 1);
        if (!lower) return lower.error;
        const double scale = options.stack.childScale;
        const double upperWidth = upper.metrics.logicalAdvance * scale, lowerWidth = lower.metrics.logicalAdvance * scale;
        const double axis = options.stack.axis * placed.paint.heightFactor;
        const double gap = options.stack.gap * placed.paint.heightFactor;
        const double shear = placed.paint.widthFactor * std::tan(placed.paint.obliqueDegrees * (std::acos(-1.0) / 180.0));
        const double upperBelow = (upper.metrics.hasInk ? -upper.metrics.inkBounds.top() : upper.metrics.descent) * scale;
        const double lowerAbove = (lower.metrics.hasInk ? lower.metrics.inkBounds.bottom() : lower.metrics.ascent) * scale;
        const double upperY = axis + gap / 2 + upperBelow, lowerY = axis - gap / 2 - lowerAbove;
        double width = std::max(upperWidth, lowerWidth);
        double upperX = (width - upperWidth) / 2, lowerX = (width - lowerWidth) / 2;
        const bool content = upperWidth > 0 || lowerWidth > 0 || upper.metrics.hasInk || lower.metrics.hasInk;
        if (object.separator == '#' && content) {
            const double diagonalWidth = options.stack.diagonalWidth * placed.paint.heightFactor * placed.paint.widthFactor;
            width = upperWidth + diagonalWidth + lowerWidth;
            upperX = 0; lowerX = upperWidth + diagonalWidth;
            const double bottom = axis - 4 * placed.paint.heightFactor, top = axis + 4 * placed.paint.heightFactor;
            placed.separators.push_back({{upperWidth + diagonalWidth / 4 + shear * bottom, bottom},
                                         {upperWidth + 3 * diagonalWidth / 4 + shear * top, top}, object.source, placed.paint});
        } else if (object.separator == '/' && width > 0) {
            placed.separators.push_back({{shear * axis, axis}, {width + shear * axis, axis}, object.source, placed.paint});
        }
        placed.upperTransform = {scale, 0, 0, scale, upperX + shear * upperY, upperY};
        placed.lowerTransform = {scale, 0, 0, scale, lowerX + shear * lowerY, lowerY};
        placed.advance = width;
        Bounds bounds;
        if (!number(width) || width < 0 || !number(placed.upperTransform.dx) || !number(placed.upperTransform.dy) ||
            !number(placed.lowerTransform.dx) || !number(placed.lowerTransform.dy) ||
            !bounds.scaled(upper.metrics, scale, placed.upperTransform.dx, upperY) ||
            !bounds.scaled(lower.metrics, scale, placed.lowerTransform.dx, lowerY))
            return invalid(Code::NumericRange, QStringLiteral("Recursive stack placement exceeds the local numeric range"));
        for (const auto &stroke : placed.separators)
            if (!bounds.add(stroke.start) || !bounds.add(stroke.end))
                return invalid(Code::NumericRange, QStringLiteral("A stack separator exceeds the local numeric range"));
        placed.hasInk = bounds.hasInk; placed.inkBounds = bounds.rectangle();
        if (content) {
            placed.ascent = std::max({0.0, upperY + scale * upper.metrics.ascent, lowerY + scale * lower.metrics.ascent});
            placed.descent = std::max({0.0, scale * upper.metrics.descent - upperY, scale * lower.metrics.descent - lowerY});
        }
        if (!number(placed.ascent) || !number(placed.descent))
            return invalid(Code::NumericRange, QStringLiteral("Recursive stack profile extents exceed the local numeric range"));
        placed.upper = std::make_shared<const BlockResult>(std::move(upper));
        placed.lower = std::make_shared<const BlockResult>(std::move(lower));
        return {};
    };

    for (std::size_t runIndex = 0; runIndex < logical.runs.size(); ++runIndex) {
        const auto &run = logical.runs[runIndex];
        const auto resolved = resolve({run.style, run.text, run.context, run.sources});
        if (!resolved) return failure(Code::FontUnavailable,
            resolved.error.isEmpty() ? QStringLiteral("The exact requested font is unavailable; no substitution was made") : resolved.error,
            run.text, run.sources, resolved.fontError);
        auto inserted = resources.emplace(resolved.font.get(), resources.size());
        context.fonts.insert(resolved.font.get());
        if (context.fonts.size() > maximumFonts)
            return failure(Code::ComplexityLimit, QStringLiteral("The paragraph font resource budget was exceeded"), run.text, run.sources);
        const std::size_t resource = inserted.first->second;
        Affine affine;
        if (!linear(*resolved.font, run.style, affine) || !metrics(*resolved.font, run.style))
            return failure(Code::NumericRange, QStringLiteral("The inline font transform or profile metrics exceed the local numeric range"), run.text, run.sources);
        LogicalRun placedRun{run, {}, resolved.font};
        if (run.kind == LC_TextRuns::Kind::Text) {
            placedRun.script = scriptTag(run);
            if (placedRun.script.isEmpty())
                return failure(Code::ShapingFailure, QStringLiteral("The resolved script has no available explicit shaping tag"), run.text, run.sources);
        }
        result.logicalRuns.push_back(placedRun);
        for (int start = run.text.start; start < run.text.end();) {
            int end = start + 1;
            while (end < run.text.end() && logical.bidi.displayLevels[end] == logical.bidi.displayLevels[start]) ++end;
            Fragment fragment;
            fragment.text = {start, end - start}; fragment.logicalRun = runIndex;
            fragment.displayLevel = logical.bidi.displayLevels[start];
            fragment.sources = logical.sourceRanges(start, end - start);
            fragment.tab = run.kind == LC_TextRuns::Kind::Tab;
            std::fill(fragmentForUnit.begin() + start, fragmentForUnit.begin() + end, result.fragments.size());
            result.fragments.push_back(std::move(fragment));
            start = end;
        }
        if (run.kind == LC_TextRuns::Kind::Object) {
            if (run.text.length != 1) return failure(Code::InvalidParagraph, QStringLiteral("An inline object is not a single ownership scalar"), run.text, run.sources);
            PlacedObject object;
            const auto error = stackObject(run, object);
            if (error) return failure(error.code, error.message, error.text, error.sources, error.fontError);
            result.metrics.ascent = std::max(result.metrics.ascent, object.ascent);
            result.metrics.descent = std::max(result.metrics.descent, object.descent);
            result.fragments[fragmentForUnit[run.text.start]].object = int(result.objects.size());
            result.objects.push_back(std::move(object));
            continue;
        }
        if (run.kind == LC_TextRuns::Kind::BidiBoundary || run.kind == LC_TextRuns::Kind::Tab) continue;
        LC_TextShaper::Run shaping;
        shaping.contextText = contexts[run.contextIndex];
        shaping.itemStart = run.text.start - run.context.start;
        shaping.itemLength = run.text.length; shaping.sourceOffset = run.context.start;
        shaping.rightToLeft = run.rightToLeft(); shaping.script = placedRun.script; shaping.language = run.style.language;
        const auto shaped = LC_TextShaper::shape(*resolved.font, shaping);
        if (!shaped) return failure(Code::ShapingFailure, shaped.error.message, run.text, run.sources, shaped.error);
        if (!context.consume(context.glyphs, shaped.glyphs.size(), options.maximumGlyphs))
            return failure(Code::ComplexityLimit, QStringLiteral("The paragraph glyph budget was exceeded"), run.text, run.sources);
        if (!shaped.unsupportedRanges.empty()) {
            if (!context.consume(context.diagnostics, 1, options.maximumDiagnostics))
                return failure(Code::ComplexityLimit, QStringLiteral("The paragraph diagnostic budget was exceeded"), run.text, run.sources);
            Diagnostic diagnostic;
            diagnostic.message = QStringLiteral("The exact font lacks coverage; its shaped missing-glyph indication is retained without partial-font substitution");
            for (const auto &range : shaped.unsupportedRanges) {
                diagnostic.textRanges.push_back({range.start, range.length});
                for (const auto &source : logical.sourceRanges(range.start, range.length)) appendSource(diagnostic.sources, source);
            }
            result.diagnostics.push_back(std::move(diagnostic));
        }
        std::map<int, bool> seenClusters;
        for (std::size_t first = 0; first < shaped.glyphs.size();) {
            const auto &initial = shaped.glyphs[first];
            std::size_t end = first + 1;
            while (end < shaped.glyphs.size() && shaped.glyphs[end].cluster == initial.cluster) ++end;
            if (!seenClusters.emplace(initial.cluster, true).second)
                return failure(Code::UnsupportedCluster, QStringLiteral("A shaping cluster is not contiguous in returned glyph order"), run.text, run.sources);
            const Range clusterRange{initial.source.start, initial.source.length};
            std::size_t fragment = fragmentForUnit[clusterRange.start];
            int retainedLevel = -1;
            for (int unit = clusterRange.start; unit < clusterRange.end(); ++unit) {
                if (logical.bidi.shapingLevels[unit] < 0) continue;
                const int display = logical.bidi.displayLevels[unit];
                if (retainedLevel >= 0 && retainedLevel != display)
                    return failure(Code::UnsupportedCluster, QStringLiteral("A final line-level boundary crosses an indivisible retained shaping cluster"),
                                   clusterRange, logical.sourceRanges(clusterRange.start, clusterRange.length));
                if (retainedLevel < 0) fragment = fragmentForUnit[unit];
                retainedLevel = display;
            }
            Cluster cluster;
            cluster.text = clusterRange;
            cluster.sources = logical.sourceRanges(clusterRange.start, clusterRange.length);
            cluster.paint = *document.styleAt(clusterRange.start);
            auto grapheme = std::lower_bound(result.graphemes.begin(), result.graphemes.end(), clusterRange.start,
                [](const Range &range, int position) { return range.end() <= position; });
            for (; grapheme != result.graphemes.end() && grapheme->start < clusterRange.end(); ++grapheme)
                cluster.graphemes.push_back(std::size_t(grapheme - result.graphemes.begin()));
            const std::size_t clusterIndex = result.clusters.size();
            result.clusters.push_back(std::move(cluster));
            for (std::size_t index = first; index < end; ++index) {
                const auto &raw = shaped.glyphs[index];
                if (raw.source.start != clusterRange.start || raw.source.length != clusterRange.length)
                    return failure(Code::UnsupportedCluster, QStringLiteral("Equal shaping clusters have inconsistent source intervals"), run.text, run.sources);
                const OutlineKey outlineKey{resource, raw.id};
                auto cachedOutline = outlines.find(outlineKey);
                if (cachedOutline == outlines.end()) {
                    const auto outline = resolved.font->outline(raw.id);
                    if (!outline) return failure(Code::OutlineFailure, outline.error.message, clusterRange,
                                                logical.sourceRanges(clusterRange.start, clusterRange.length), outline.error);
                    cachedOutline = outlines.emplace(outlineKey, outline.outline).first;
                }
                const auto &outline = cachedOutline->second;
                if (!raw.id && !outline->hasInk)
                    return failure(Code::OutlineFailure, QStringLiteral("The exact font has no visible deterministic missing-glyph indication"),
                                   clusterRange, logical.sourceRanges(clusterRange.start, clusterRange.length));
                if (retainedLevel < 0 && (outline->hasInk || raw.xAdvance || raw.yAdvance))
                    return failure(Code::UnsupportedMetrics, QStringLiteral("An excluded bidi-control cluster unexpectedly produces ink or advance"),
                                   clusterRange, logical.sourceRanges(clusterRange.start, clusterRange.length));
                const BoundsKey boundsKey{resource, raw.id, affine.xx, affine.xy, affine.yy};
                auto bounds = localBounds.find(boundsKey);
                if (bounds == localBounds.end()) {
                    if (!context.consume(context.commands, outline->commands.size(), options.maximumOutlineCommands))
                        return failure(Code::ComplexityLimit, QStringLiteral("The transformed contour command budget was exceeded"), run.text, run.sources);
                    Bounds measured;
                    if (!contourBounds(*outline, affine, measured))
                        return failure(Code::NumericRange, QStringLiteral("The transformed font contours cannot be measured reliably"), run.text, run.sources);
                    bounds = localBounds.emplace(boundsKey, measured).first;
                }
                Glyph glyph;
                glyph.font = resolved.font; glyph.outline = outline; glyph.logicalRun = runIndex; glyph.cluster = clusterIndex;
                glyph.id = raw.id; glyph.xAdvance = raw.xAdvance; glyph.yAdvance = raw.yAdvance;
                glyph.xOffset = raw.xOffset; glyph.yOffset = raw.yOffset; glyph.unsafeToBreak = raw.unsafeToBreak;
                glyph.drawable = outline->hasInk; glyph.transform = affine;
                result.fragments[fragment].glyphs.push_back(result.glyphs.size());
                result.glyphs.push_back(std::move(glyph)); glyphBounds.push_back(bounds->second);
            }
            first = end;
        }
    }

    // Retain the full projection for L1/L2 ordering. Generated Chinese controls
    // affect order only; they never receive a fragment, glyph or raw source.
    const auto display = lc::qtbidi::resolve(logical.bidi.projection.text, options.direction);
    const auto order = lc::qtbidi::reorder(display.levels);
    std::vector<bool> emitted(result.fragments.size(), false);
    for (int projected : order) {
        const int original = logical.bidi.projection.sourcePositions[projected];
        if (original < 0) continue;
        const auto fragment = fragmentForUnit[original];
        if (!emitted[fragment]) { result.visualFragments.push_back(fragment); emitted[fragment] = true; }
    }
    if (result.visualFragments.size() != result.fragments.size())
        return failure(Code::ShapingFailure, QStringLiteral("Final display ordering lost logical source ownership"));
    // Measure the already-shaped fragments exactly once. Tabs are field
    // boundaries in the final whole-line display order, not separate bidi runs.
    for (auto &fragment : result.fragments) {
        if (fragment.object >= 0) {
            fragment.advance = {result.objects[fragment.object].advance, 0};
            continue;
        }
        if (fragment.glyphs.empty()) continue;
        std::int64_t x = 0, y = 0;
        for (auto index : fragment.glyphs)
            if (!addAdvance(x, result.glyphs[index].xAdvance) || !addAdvance(y, result.glyphs[index].yAdvance))
                return failure(Code::NumericRange, QStringLiteral("Accumulated fragment advances exceed the numeric range"), fragment.text, fragment.sources);
        auto affine = result.glyphs[fragment.glyphs.front()].transform;
        affine.dx = affine.dy = 0;
        if (!map(affine, {static_cast<double>(x), static_cast<double>(y)}, fragment.advance) || fragment.advance.x() < 0)
            return failure(Code::UnsupportedMetrics, QStringLiteral("The horizontal fragment advance is not a nonnegative interval"), fragment.text, fragment.sources);
    }
    struct Field { std::vector<std::size_t> fragments; double width = 0, distance = 0, x = 0; };
    std::vector<Field> fields(1);
    std::vector<std::size_t> tabs;
    for (auto index : result.visualFragments) {
        const auto &fragment = result.fragments[index];
        if (fragment.tab) { tabs.push_back(index); fields.emplace_back(); }
        else {
            fields.back().fragments.push_back(index);
            fields.back().width += fragment.advance.x();
            if (!number(fields.back().width)) return failure(Code::NumericRange, QStringLiteral("A tab field exceeds the local numeric range"));
        }
    }
    double distance = 0;
    const bool rtl = (result.baseLevel & 1) != 0;
    for (std::size_t step = 0; step < fields.size(); ++step) {
        auto &field = fields[rtl ? fields.size() - 1 - step : step];
        field.distance = distance;
        distance += field.width;
        if (!number(distance)) return failure(Code::NumericRange, QStringLiteral("The paragraph advance exceeds the local numeric range"));
        if (step + 1 < fields.size()) {
            const double next = (std::floor(distance / options.tabInterval) + 1) * options.tabInterval;
            if (!number(next) || next <= distance)
                return failure(Code::NumericRange, QStringLiteral("The next tab stop is not finite, bounded and strictly advancing"));
            distance = next;
        }
    }
    for (auto &field : fields) {
        // Match the accumulated endpoint expression so the leftmost RTL
        // field's anchor is exactly zero, not a cancellation-sized residue.
        field.x = rtl ? distance - (field.distance + field.width) : field.distance;
        double x = field.x;
        for (auto index : field.fragments) {
            result.fragments[index].origin = {x, 0};
            x += result.fragments[index].advance.x();
        }
    }
    for (std::size_t index = 0; index < tabs.size(); ++index) {
        auto &tab = result.fragments[tabs[index]];
        const double left = fields[index].x + fields[index].width, right = fields[index + 1].x;
        if (!number(left) || !number(right) || right < left)
            return failure(Code::NumericRange, QStringLiteral("A tab field gap is not a finite nonnegative interval"), tab.text, tab.sources);
        tab.origin = {left, 0}; tab.advance = {right - left, 0};
    }
    Bounds ink;
    for (std::size_t fragmentIndex : result.visualFragments) {
        auto &fragment = result.fragments[fragmentIndex];
        const double cursor = fragment.origin.x();
        if (fragment.object >= 0) {
            auto &object = result.objects[fragment.object];
            object.origin = fragment.origin;
            object.upperTransform.dx += cursor; object.lowerTransform.dx += cursor;
            if (!number(object.upperTransform.dx) || !number(object.lowerTransform.dx))
                return failure(Code::NumericRange, QStringLiteral("A positioned stack exceeds the local numeric range"), fragment.text, fragment.sources);
            for (auto &stroke : object.separators) { stroke.start.rx() += cursor; stroke.end.rx() += cursor; }
            Bounds measured;
            if (object.hasInk && (!measured.add({object.inkBounds.left() + cursor, object.inkBounds.top()}) ||
                                  !measured.add({object.inkBounds.right() + cursor, object.inkBounds.bottom()})))
                return failure(Code::NumericRange, QStringLiteral("Positioned stack ink exceeds the local numeric range"), fragment.text, fragment.sources);
            if (!ink.translated(measured, 0, 0))
                return failure(Code::NumericRange, QStringLiteral("Positioned stack ink cannot be accumulated"), fragment.text, fragment.sources);
            object.inkBounds = measured.rectangle();
            continue;
        }
        std::int64_t penX = 0, penY = 0;
        Affine affine;
        for (std::size_t glyphIndex : fragment.glyphs) {
            auto &glyph = result.glyphs[glyphIndex];
            affine = glyph.transform;
            QPointF origin;
            affine.dx = cursor;
            if (!map(affine, {static_cast<double>(penX) + glyph.xOffset, static_cast<double>(penY) + glyph.yOffset}, origin))
                return failure(Code::NumericRange, QStringLiteral("A positioned glyph exceeds the local numeric range"), fragment.text, fragment.sources);
            glyph.transform.dx = origin.x(); glyph.transform.dy = origin.y();
            if (!ink.translated(glyphBounds[glyphIndex], origin.x(), origin.y()) ||
                !addAdvance(penX, glyph.xAdvance) || !addAdvance(penY, glyph.yAdvance))
                return failure(Code::NumericRange, QStringLiteral("Glyph ink or accumulated advances exceed the local numeric range"), fragment.text, fragment.sources);
        }
    }
    result.metrics.logicalAdvance = distance;
    result.metrics.hasInk = ink.hasInk; result.metrics.inkBounds = ink.rectangle();
    if (ink.hasInk) { result.metrics.inkAboveBaseline = std::max(0.0, ink.maxY); result.metrics.inkBelowBaseline = std::max(0.0, -ink.minY); }
    return result;
#endif
}

LC_TextLayout::BlockResult LC_TextLayout::document(const LC_TextDocument &document,
                                                   const FontResolver &resolver, const Options &options) {
    Context context{resolver, options, 0, 0, 0, 0, 0, 0, 0, {}, {}, document.source};
    return blockImpl(document, context, options, 0);
}

LC_TextLayout::BlockResult LC_TextLayout::blockImpl(const LC_TextDocument &document, Context &context,
                                                   const Options &options, int depth) {
#if !defined(LC_WITH_OPENTYPE) || !LC_WITH_OPENTYPE
    Q_UNUSED(document)
    Q_UNUSED(context)
    Q_UNUSED(options)
    Q_UNUSED(depth)
    return blockFailure({Code::BackendUnavailable, QStringLiteral("OpenType block layout is disabled in this build"), {}, {}, {}});
#else
    const auto fail = [&](Code code, const QString &message, LC_OpenTypeFont::Error fontError = {}) {
        return blockFailure({code, message, {}, {document.sourceRange}, std::move(fontError)});
    };
    if (!context.resolver || !validOptions(options)) return fail(Code::InvalidOptions, QStringLiteral("Invalid block layout options or resolver"));
    if (!document.ok() || document.paragraphs.empty() || document.sourceRange.start < 0 || document.sourceRange.length < 0 ||
        document.sourceRange.start > document.source.size() || document.sourceRange.length > document.source.size() - document.sourceRange.start)
        return fail(Code::InvalidParagraph, QStringLiteral("Invalid parsed block or original-source range"));
    if (document.source.size() > hardInputLimit || document.paragraphs.size() > options.maximumParagraphs - context.paragraphs)
        return fail(Code::ComplexityLimit, QStringLiteral("The recursive block source or paragraph budget was exceeded"));
    if (context.canonicalSource && !LC_TextDocument::sameText(document.source, *context.canonicalSource))
        return fail(Code::InvalidParagraph, QStringLiteral("Recursive parsed blocks do not share the canonical original source"));
    // paragraph() has no outer original string. Its first parsed child supplies
    // the canonical source for all children/descendants, not for the paragraph.
    if (!context.canonicalSource) context.canonicalSource = document.source;
    if (!context.active.insert(&document).second)
        return fail(Code::InvalidParagraph, QStringLiteral("Recursive stack documents contain a cycle"));
    struct ActiveGuard {
        std::set<const LC_TextDocument *> &active;
        const LC_TextDocument *document;
        ~ActiveGuard() { active.erase(document); }
    } guard{context.active, &document};
    int sourceCursor = document.sourceRange.start;
    for (const auto &paragraph : document.paragraphs) {
        if (paragraph.source.start != sourceCursor || paragraph.source.length < 0 ||
            paragraph.source.length > document.sourceRange.end() - paragraph.source.start || paragraph.breakSource.length < 0 ||
            (paragraph.breakSource.length > 0 && (paragraph.breakSource.start != paragraph.source.end() ||
                paragraph.breakSource.length > document.sourceRange.end() - paragraph.breakSource.start)))
            return fail(Code::InvalidParagraph, QStringLiteral("Explicit paragraph/break ownership does not cover the parsed block"));
        sourceCursor = paragraph.breakSource.length > 0 ? paragraph.breakSource.end() : paragraph.source.end();
    }
    if (sourceCursor != document.sourceRange.end())
        return fail(Code::InvalidParagraph, QStringLiteral("Parsed paragraph ownership omits original source"));
    FontResolution base;
    try { base = context.resolver({shapingStyle(options.emptyStyle), {}, {}, {document.sourceRange}}); }
    catch (...) { base.error = QStringLiteral("The explicit base-font resolver failed"); }
    if (!base) return fail(Code::FontUnavailable, base.error.isEmpty() ? QStringLiteral("The exact base font for line pitch is unavailable") : base.error, base.fontError);
    context.fonts.insert(base.font.get());
    if (context.fonts.size() > maximumFonts) return fail(Code::ComplexityLimit, QStringLiteral("The recursive font resource budget was exceeded"));
    const double basePitch = base.font->profile().lineHeight * base.font->normalization() * options.emptyStyle.heightFactor;
    const double requestedPitch = options.linePitch > 0 ? options.linePitch : basePitch;
    if (!number(basePitch) || !number(requestedPitch) || requestedPitch <= 0)
        return fail(Code::NumericRange, QStringLiteral("The explicit/default baseline pitch exceeds the local numeric range"));
    BlockResult result;
    result.source = document.sourceRange; result.baseFont = base.font;
    Bounds ink;
    double baseline = 0;
    for (const auto &paragraph : document.paragraphs) {
        auto line = paragraphImpl(paragraph, context, options, depth);
        if (!line) return blockFailure(line.error);
        if (!result.lines.empty()) {
            double pitch = requestedPitch;
            if (options.lineSpacing == LineSpacing::AtLeast) {
                const auto &previous = result.lines.back().paragraph.metrics;
                const auto &next = line.metrics;
                pitch = std::max({pitch, previous.profileLinePitch, next.profileLinePitch,
                    std::max(previous.descent, previous.inkBelowBaseline) + std::max(next.ascent, next.inkAboveBaseline)});
            }
            const double nextBaseline = baseline - pitch;
            if (!number(pitch) || pitch <= 0 || !number(nextBaseline) || nextBaseline >= baseline)
                return fail(Code::NumericRange, QStringLiteral("Explicit baseline placement is not finite and strictly descending"));
            baseline = nextBaseline;
        }
        const auto &metrics = line.metrics;
        if (!ink.scaled(metrics, 1, 0, baseline)) return fail(Code::NumericRange, QStringLiteral("Multiline ink bounds exceed the local numeric range"));
        result.metrics.logicalAdvance = std::max(result.metrics.logicalAdvance, metrics.logicalAdvance);
        result.metrics.ascent = std::max(result.metrics.ascent, baseline + metrics.ascent);
        result.metrics.descent = std::max(result.metrics.descent, metrics.descent - baseline);
        result.metrics.nominalCellHeight = std::max(result.metrics.nominalCellHeight, metrics.nominalCellHeight);
        result.metrics.profileLinePitch = std::max(result.metrics.profileLinePitch, metrics.profileLinePitch);
        result.lines.push_back({std::move(line), {0, baseline}});
    }
    if (!number(result.metrics.ascent) || !number(result.metrics.descent))
        return fail(Code::NumericRange, QStringLiteral("Multiline profile extents exceed the local numeric range"));
    result.baselineSpan = -baseline;
    result.metrics.hasInk = ink.hasInk; result.metrics.inkBounds = ink.rectangle();
    if (ink.hasInk) { result.metrics.inkAboveBaseline = std::max(0.0, ink.maxY); result.metrics.inkBelowBaseline = std::max(0.0, -ink.minY); }
    return result;
#endif
}
