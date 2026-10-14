/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textruns.h"

#include <QTextBoundaryFinder>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <utility>

bool LC_TextRuns::ShapingStyle::operator==(const ShapingStyle &s) const {
    return heightFactor == s.heightFactor && widthFactor == s.widthFactor &&
           obliqueDegrees == s.obliqueDegrees && LC_TextDocument::sameText(font, s.font) &&
           LC_TextDocument::sameText(fontSpecification, s.fontSpecification) &&
           LC_TextDocument::sameText(language, s.language);
}

namespace {
using Runs = LC_TextRuns;
using Range = Runs::Range;
using Script = QChar::Script;
bool validRange(Range range, int limit) {
    return range.start >= 0 && range.length >= 0 && range.start <= limit &&
           range.length <= limit - range.start;
}
void appendSource(std::vector<Range> &ranges, Range source) {
    if (source.length <= 0 || (!ranges.empty() && ranges.back() == source)) return;
    if (!ranges.empty() && ranges.back().end() == source.start)
        ranges.back().length += source.length;
    else if (!ranges.empty() && source.start >= ranges.back().start && source.end() <= ranges.back().end())
        return; // The second UTF-16 unit of an already merged scalar interval.
    else
        ranges.push_back(source);
}
bool explicitScript(Script script) {
    return script >= QChar::Script_Unknown && script < QChar::ScriptCount &&
           script != QChar::Script_Common && script != QChar::Script_Inherited &&
           script != QChar::Script_Unknown;
}
Runs::ShapingStyle shapingStyle(const LC_TextDocument::Style &style) {
    return {style.font, style.fontSpecification, style.language,
            style.heightFactor, style.widthFactor, style.obliqueDegrees};
}
Runs::Kind kind(char32_t value) {
    if (value == '\t') return Runs::Kind::Tab;
    if (value == 0xfffc) return Runs::Kind::Object;
    const auto direction = QChar::direction(value);
    if (direction == QChar::DirLRI || direction == QChar::DirRLI ||
        direction == QChar::DirFSI || direction == QChar::DirPDI)
        return Runs::Kind::BidiBoundary;
    if (direction == QChar::DirB || direction == QChar::DirS || value == 0x2028)
        return Runs::Kind::Separator;
    return Runs::Kind::Text; // Keep BN, ZWJ/ZWNJ and explicit formatting controls.
}
struct Unit {
    Range text;
    char32_t scalar = 0;
    int level = -1;
    Runs::Kind kind = Runs::Kind::Text;
    Runs::ShapingStyle style;
    Script script = QChar::Script_Common;
    char32_t representative = 0;
    int object = -1;
};
struct Group {
    int first = 0;
    int end = 0;
    Script script = QChar::Script_Common;
    char32_t representative = 0;
};

// UAX #24 sections 5.1/5.2: resolve Common from surrounding text; keep
// combining sequences together regardless of a mark's nominal Script value.
// Qt's public Script property supplies classification, not Script_Extensions.
// This deterministic rendering policy is not a script-identification claim.
void resolveScripts(std::vector<Unit> &units, int first, int end,
                    const std::vector<int> &graphemeForUnit, Script fallback) {
    std::vector<Group> groups;
    for (int i = first; i < end; ++i) {
        if (groups.empty() || graphemeForUnit[units[i].text.start] !=
                              graphemeForUnit[units[groups.back().first].text.start])
            groups.push_back({i, i + 1, QChar::Script_Common, 0});
        else
            groups.back().end = i + 1;
        auto &group = groups.back();
        const auto script = QChar::script(units[i].scalar);
        if (!explicitScript(group.script) && explicitScript(script)) {
            group.script = script;
            group.representative = units[i].scalar;
        }
    }
    std::vector<int> nextExplicit(groups.size(), -1);
    int next = -1;
    for (int i = int(groups.size()) - 1; i >= 0; --i) {
        if (explicitScript(groups[i].script)) next = i;
        nextExplicit[i] = next;
    }
    // Matching bracket stacks are amortized linear: each opening is popped
    // once, including unmatched openings discarded by a matching outer close.
    struct Opening { int group; char32_t close; };
    std::vector<Opening> openings;
    std::map<char32_t, std::vector<int>> openingDepths;
    Script previous = fallback;
    char32_t previousRepresentative = 0;
    bool havePrevious = false;
    for (int i = 0; i < int(groups.size()); ++i) {
        auto &group = groups[i];
        const bool implicit = !explicitScript(group.script);
        if (implicit) {
            if (havePrevious) {
                group.script = previous;
                group.representative = previousRepresentative;
            } else if (nextExplicit[i] >= 0) {
                group.script = groups[nextExplicit[i]].script;
                group.representative = groups[nextExplicit[i]].representative;
            } else {
                group.script = fallback;
            }
        }
        const char32_t scalar = units[group.first].scalar;
        const auto category = QChar::category(scalar);
        if (category == QChar::Punctuation_Open) {
            const char32_t close = QChar::mirroredChar(scalar);
            if (close != scalar) {
                openingDepths[close].push_back(int(openings.size()));
                openings.push_back({i, close});
            }
        } else if (category == QChar::Punctuation_Close) {
            const auto match = openingDepths.find(scalar);
            if (match != openingDepths.end() && !match->second.empty()) {
                const int depth = match->second.back();
                const auto &open = groups[openings[depth].group];
                if (implicit) {
                    group.script = open.script;
                    group.representative = open.representative;
                }
                while (int(openings.size()) > depth) {
                    openingDepths[openings.back().close].pop_back();
                    openings.pop_back();
                }
            }
        }
        previous = group.script;
        previousRepresentative = group.representative;
        havePrevious = true;
        for (int j = group.first; j < group.end; ++j) {
            units[j].script = group.script;
            units[j].representative = group.representative;
        }
    }
}
} // namespace

std::vector<LC_TextRuns::Range> LC_TextRuns::Paragraph::sourceRanges(int start, int length) const {
    std::vector<Range> result;
    if (!validRange({start, length}, int(sourceMap.size()))) return result;
    for (int i = start; i < start + length; ++i) appendSource(result, sourceMap[i]);
    return result;
}

std::vector<LC_TextRuns::Range> LC_TextRuns::Paragraph::projectedSourceRanges(int start, int length) const {
    std::vector<Range> result;
    if (!validRange({start, length}, int(bidi.projection.sourcePositions.size()))) return result;
    for (int i = start; i < start + length; ++i) {
        const int logical = bidi.projection.sourcePositions[i];
        if (logical >= 0 && logical < int(sourceMap.size())) appendSource(result, sourceMap[logical]);
    }
    return result;
}

LC_TextRuns::Paragraph LC_TextRuns::itemize(const LC_TextDocument::Paragraph &paragraph,
                                         const Options &options) {
    Paragraph result;
    result.text = paragraph.text;
    result.source = paragraph.source;
    result.breakSource = paragraph.breakSource;
    result.paintSpans = paragraph.spans;
    result.objects = paragraph.objects;
    const auto fail = [&](const char *message) {
        result.error = QString::fromLatin1(message);
        return result;
    };
    if (paragraph.text.size() > std::numeric_limits<int>::max() / 5 ||
        !validRange(paragraph.source, std::numeric_limits<int>::max()) ||
        !validRange(paragraph.breakSource, std::numeric_limits<int>::max()))
        return fail("Paragraph exceeds UTF-16 index range");
    if (options.direction != Qt::LeftToRight && options.direction != Qt::RightToLeft &&
        options.direction != Qt::LayoutDirectionAuto)
        return fail("Invalid paragraph direction");
    if (!explicitScript(options.fallbackScript)) return fail("Fallback script must be explicit");
    if (paragraph.breakSource.length > 0 && paragraph.breakSource.start < paragraph.source.end())
        return fail("Paragraph break overlaps its logical source");
    const int size = int(paragraph.text.size());
    int cursor = 0;
    for (const auto &span : paragraph.spans) {
        if (!validRange(span.text, size) || span.text.start != cursor || span.text.length == 0 ||
            !std::isfinite(span.style.heightFactor) || span.style.heightFactor <= 0 ||
            !std::isfinite(span.style.widthFactor) || span.style.widthFactor <= 0 ||
            !std::isfinite(span.style.obliqueDegrees) || std::abs(span.style.obliqueDegrees) >= 90.0)
            return fail("Invalid or incomplete formatting spans");
        cursor = span.text.end();
        if (cursor < size && paragraph.text.at(cursor).isLowSurrogate() &&
            paragraph.text.at(cursor - 1).isHighSurrogate())
            return fail("Formatting span splits a Unicode scalar");
    }
    if (cursor != size) return fail("Formatting spans do not cover logical text");
    result.sourceMap.resize(size);
    std::vector<Unit> units;
    units.reserve(paragraph.scalars.size());
    cursor = 0;
    int previousSource = paragraph.source.start;
    std::size_t spanIndex = 0;
    for (const auto &scalar : paragraph.scalars) {
        if (!validRange(scalar.text, size) || scalar.text.start != cursor ||
            scalar.value > 0x10ffff || (scalar.value >= 0xd800 && scalar.value <= 0xdfff) ||
            scalar.text.length != (scalar.value > 0xffff ? 2 : 1) ||
            !validRange(scalar.source, paragraph.source.end()) || scalar.source.length == 0 ||
            scalar.source.start < previousSource)
            return fail("Invalid scalar or original-source mapping");
        const auto firstUnit = paragraph.text.at(scalar.text.start);
        char32_t decoded = firstUnit.unicode();
        if (scalar.text.length == 2 && firstUnit.isHighSurrogate() &&
            paragraph.text.at(scalar.text.start + 1).isLowSurrogate())
            decoded = QChar::surrogateToUcs4(firstUnit, paragraph.text.at(scalar.text.start + 1));
        if (decoded != scalar.value)
            return fail("Scalar map does not describe logical text");
        while (spanIndex + 1 < paragraph.spans.size() &&
               paragraph.spans[spanIndex].text.end() <= scalar.text.start) ++spanIndex;
        Unit unit;
        unit.text = scalar.text;
        unit.scalar = scalar.value;
        unit.kind = kind(scalar.value);
        unit.style = shapingStyle(paragraph.spans[spanIndex].style);
        units.push_back(std::move(unit));
        for (int i = scalar.text.start; i < scalar.text.end(); ++i) result.sourceMap[i] = scalar.source;
        cursor = scalar.text.end();
        previousSource = scalar.source.end();
    }
    if (cursor != size) return fail("Scalar map does not cover logical text");
    std::vector<int> unitForPosition(size, -1);
    for (int i = 0; i < int(units.size()); ++i)
        for (int p = units[i].text.start; p < units[i].text.end(); ++p) unitForPosition[p] = i;
    for (int i = 0; i < int(paragraph.objects.size()); ++i) {
        const auto &object = paragraph.objects[i];
        if (object.textPosition < 0 || object.textPosition >= size ||
            units[unitForPosition[object.textPosition]].scalar != 0xfffc ||
            units[unitForPosition[object.textPosition]].object >= 0 ||
            !validRange(object.source, paragraph.source.end()) ||
            object.source.start < paragraph.source.start || object.source.length == 0 ||
            !(object.source == result.sourceMap[object.textPosition]))
            return fail("Invalid or duplicate inline-object mapping");
        units[unitForPosition[object.textPosition]].object = i;
    }
    result.bidi = lc::textbidi::shapingAnalysis(paragraph.text, options.direction);
    result.runForUnit.assign(size, -1);
    std::vector<int> graphemeForUnit(size, 0);
    QTextBoundaryFinder boundaries(QTextBoundaryFinder::Grapheme, paragraph.text);
    int grapheme = 0;
    for (int start = 0, end = boundaries.toNextBoundary(); end >= 0;
         start = end, end = boundaries.toNextBoundary(), ++grapheme)
        std::fill(graphemeForUnit.begin() + start, graphemeForUnit.begin() + end, grapheme);

    // First split only at genuine context boundaries. X9 units acquire the
    // preceding retained level, or the following level for a leading sequence.
    for (int hardFirst = 0; hardFirst < int(units.size());) {
        int hardEnd = hardFirst + 1;
        if (units[hardFirst].kind == Kind::Text)
            while (hardEnd < int(units.size()) && units[hardEnd].kind == Kind::Text &&
                   !result.bidi.contextBreaks[units[hardEnd].text.start]) ++hardEnd;
        int following = result.bidi.base;
        for (int i = hardFirst; i < hardEnd; ++i) {
            const int level = result.bidi.shapingLevels[units[i].text.start];
            if (level >= 0) { following = level; break; }
        }
        int previous = following;
        for (int i = hardFirst; i < hardEnd; ++i) {
            const int level = result.bidi.shapingLevels[units[i].text.start];
            units[i].level = level >= 0 ? level : previous;
            previous = units[i].level;
        }
        for (int first = hardFirst; first < hardEnd;) {
            int end = first + 1;
            while (end < hardEnd && units[end].level == units[first].level) ++end;
            int shapingFirst = first;
            int shapingEnd = end;
            if (units[first].kind == Kind::Text) {
                // Ownership alone cannot express ZWJ/ZWNJ effects on BOTH
                // backing-store neighbors. Borrow only contiguous X9 units
                // within this hard partition; the first retained other-level
                // scalar is an absolute context boundary. Controls stay owned
                // once and are not added to the borrowed item's output range.
                while (shapingFirst > hardFirst &&
                       result.bidi.shapingLevels[units[shapingFirst - 1].text.start] < 0)
                    --shapingFirst;
                while (shapingEnd < hardEnd &&
                       result.bidi.shapingLevels[units[shapingEnd].text.start] < 0)
                    ++shapingEnd;
            }
            const Context context{{units[first].text.start,
                units[end - 1].text.end() - units[first].text.start},
                {units[shapingFirst].text.start,
                 units[shapingEnd - 1].text.end() - units[shapingFirst].text.start},
                units[first].level, units[first].kind == Kind::Text};
            const int contextIndex = int(result.contexts.size());
            result.contexts.push_back(context);
            resolveScripts(units, first, end, graphemeForUnit, options.fallbackScript);
            for (int item = first; item < end;) {
                int itemEnd = item + 1;
                // The style is compared with the unit before: the units of one
                // span share the buffers of its font names, the first unit of
                // the run may hold equal names in buffers of another span.
                while (itemEnd < end && units[itemEnd].kind == units[item].kind &&
                       units[itemEnd].script == units[item].script &&
                       units[itemEnd].style == units[itemEnd - 1].style) ++itemEnd;
                Run run;
                run.kind = units[item].kind;
                run.text = {units[item].text.start, units[itemEnd - 1].text.end() - units[item].text.start};
                run.context = context.shapingText;
                run.contextIndex = contextIndex;
                run.level = context.level;
                run.script = units[item].script;
                run.representativeScalar = units[item].representative;
                run.style = units[item].style;
                run.objectIndex = units[item].object;
                run.sources = result.sourceRanges(run.text.start, run.text.length);
                std::fill(result.runForUnit.begin() + run.text.start,
                          result.runForUnit.begin() + run.text.end(), int(result.runs.size()));
                result.runs.push_back(std::move(run));
                item = itemEnd;
            }
            first = end;
        }
        hardFirst = hardEnd;
    }
    return result;
}
