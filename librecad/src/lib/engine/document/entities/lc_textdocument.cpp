/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textdocument.h"

#include <QChar>
#include <QTextBoundaryFinder>
#include <algorithm>
#include <cmath>
#include <limits>

bool LC_TextDocument::Style::operator==(const Style &s) const {
    return heightFactor == s.heightFactor && widthFactor == s.widthFactor && obliqueDegrees == s.obliqueDegrees &&
           color == s.color && trueColor == s.trueColor && underline == s.underline &&
           overline == s.overline && strikeThrough == s.strikeThrough &&
           sameText(font, s.font) && sameText(fontSpecification, s.fontSpecification) && sameText(language, s.language);
}

std::vector<LC_TextDocument::Range> LC_TextDocument::Paragraph::sourceRanges(int start, int length) const {
    std::vector<Range> result;
    if (start < 0 || length <= 0 || start > text.size() || length > text.size() - start) return result;
    const auto first = std::lower_bound(scalars.begin(), scalars.end(), start,
        [](const Scalar &scalar, int position) { return scalar.text.end() <= position; });
    for (auto it = first; it != scalars.end(); ++it) {
        const auto &scalar = *it;
        if (scalar.text.start >= start + length) break;
        if (!result.empty() && result.back().end() == scalar.source.start)
            result.back().length += scalar.source.length;
        else if (result.empty() || !(result.back() == scalar.source))
            result.push_back(scalar.source);
    }
    return result;
}

const LC_TextDocument::Style *LC_TextDocument::Paragraph::styleAt(int position) const {
    if (position < 0 || position >= text.size()) return nullptr;
    const auto it = std::upper_bound(spans.begin(), spans.end(), position,
        [](int index, const Span &span) { return index < span.text.start; });
    return it == spans.begin() ? nullptr : &std::prev(it)->style;
}

bool LC_TextDocument::ok() const {
    return std::none_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic &d) { return d.fatal; });
}

namespace {
using Document = LC_TextDocument;
using Range = Document::Range;
using Style = Document::Style;
using Code = Document::Diagnostic::Code;
using Kind = Document::TokenKind;

// Constant-time lookahead prevents repeated malformed commands from rescanning
// the entire suffix. Stack children share these immutable source-indexed tables.
struct Lookahead {
    std::vector<int> semicolon;
    std::vector<int> matchedCloseBrace;
    std::vector<int> stackSemicolon;
    Lookahead(const QString &source, bool decodeCaretCodes)
        : semicolon(source.size() + 2, -1), matchedCloseBrace(source.size() + 2, -1), stackSemicolon(source.size() + 2, -1) {
        const auto literalSeparator = [](QChar ch) {
            return ch == '\n' || ch == '\r' || ch.unicode() == 0x2028 || ch.unicode() == 0x2029;
        };
        const auto separator = [&](int position) {
            if (literalSeparator(source.at(position))) return true;
            if (!decodeCaretCodes || source.at(position) != '^' || position + 1 >= source.size()) return false;
            const auto next = source.at(position + 1).unicode();
            if (next > 0x7f) return false;
            const int code = ((int(next) - 64) % 126 + 126) % 126;
            return code == '\n' || code == '\r';
        };
        std::vector<int> open;
        for (int i = 0; i < source.size(); ++i) {
            const auto ch = source.at(i);
            if (ch == '\\' && i + 1 < source.size() && !literalSeparator(source.at(i + 1))) { ++i; continue; }
            if (separator(i)) open.clear();
            else if (ch == '{') open.push_back(i);
            else if (ch == '}' && !open.empty()) {
                matchedCloseBrace[open.back()] = i; open.pop_back();
            }
        }
        for (int i = int(source.size()) - 1; i >= 0; --i) {
            const auto ch = source.at(i);
            const int next = ch == '\\' && i + 1 < source.size() && !literalSeparator(source.at(i + 1)) ? i + 2 : i + 1;
            semicolon[i] = ch == ';' ? i : separator(i) ? -1 : semicolon[next];
            stackSemicolon[i] = ch == ';' ? i : separator(i) || ch == '}' ? -1 :
                ch == '{' ? (matchedCloseBrace[i] < 0 ? -1 : stackSemicolon[matchedCloseBrace[i] + 1]) : stackSemicolon[next];
        }
    }
};

class Parser {
public:
    Parser(const QString &source, Range range, const Document::Options &options, int stackDepth = 0,
           std::shared_ptr<const Lookahead> lookahead = {})
        : options(options), style(options.initialStyle), stackDepth(stackDepth), position(range.start), end(range.end()),
          lookahead(lookahead ? std::move(lookahead) : std::make_shared<Lookahead>(source, options.decodeCaretCodes)) {
        result.source = source;
        result.sourceRange = range;
        result.paragraphs.emplace_back();
        paragraph().source.start = range.start;
    }

    Document run() {
        while (position < end && !failed) {
            const int start = position;
            const int logicalStart = int(paragraph().text.size());
            const QChar ch = result.source.at(position);
            if (ch == '\r' || ch == '\n' || ch.unicode() == 0x2029 || ch.unicode() == 0x2028) {
                ++position;
                // Coalesce decoded CR/LF in breakParagraph, including physical
                // CRLF. Pre-coalescing here would let its LF consume a separate
                // preceding CR in a CRCRLF sequence.
                breakParagraph({start, 1}, ch.unicode());
                continue;
            }
            if (options.decodeCaretCodes && ch == '^' && position + 1 < end &&
                result.source.at(position + 1).unicode() <= 0x7f &&
                result.source.at(position + 1) != '\n' && result.source.at(position + 1) != '\r' &&
                result.source.at(position + 1).unicode() != 0x2028 && result.source.at(position + 1).unicode() != 0x2029) {
                const auto next = result.source.at(position + 1).unicode();
                const int code = next == ' ' ? '^' : ((int(next) - 64) % 126 + 126) % 126;
                position += 2;
                if (code == '\n' || code == '\r') breakParagraph({start, 2}, char32_t(code));
                else { appendScalar(char32_t(code), {start, 2}); token(Kind::Escape, start, logicalStart); }
                continue;
            }
            // Caret notation is an ASCII control encoding. Do not apply its
            // modulo operation to a Unicode unit, especially half a surrogate
            // pair; retain an unrecognized Unicode spelling literally.
            if (options.decodeCaretCodes && ch == '^' && position + 1 < end &&
                result.source.at(position + 1).unicode() > 0x7f) {
                diagnostic(Code::MalformedEscape, {start, 1 + scalarLength(position + 1)},
                           "Non-ASCII caret spelling retained literally");
            }
            if (options.decodeDxfEscapes && ch == '%' && position + 2 < end && result.source.at(position + 1) == '%') {
                const auto code = result.source.at(position + 2).toLower().unicode();
                const char32_t scalar = code == 'c' ? 0x2300 : code == 'd' ? 0xb0 : code == 'p' ? 0xb1 : 0;
                if (scalar) {
                    position += 3; appendScalar(scalar, {start, 3}); token(Kind::Escape, start, logicalStart); continue;
                }
            }
            if (options.decodeDxfEscapes && ch == '\\' && unicodeEscape()) continue;
            if (options.syntax == Document::Syntax::MText) {
                if (ch == '{') {
                    ++position;
                    if (int(scopes.size()) >= options.maxScopeDepth) {
                        diagnostic(Code::ResourceLimit, {start, 1}, "MTEXT scope depth exceeded", true); break;
                    }
                    scopes.push_back({style, start}); token(Kind::ScopeOpen, start, logicalStart); continue;
                }
                if (ch == '}') {
                    ++position;
                    if (scopes.empty()) {
                        diagnostic(Code::UnbalancedScope, {start, 1}, "Unmatched MTEXT closing brace");
                        appendScalar('}', {start, 1}); token(Kind::Malformed, start, logicalStart);
                    } else {
                        style = scopes.back().first; scopes.pop_back(); token(Kind::ScopeClose, start, logicalStart);
                    }
                    continue;
                }
                if (ch == '\\' && mtextEscape()) continue;
            }
            appendLiteral(position, position + scalarLength(position));
            token(Kind::Text, start, logicalStart);
        }
        finishParagraph(end);
        for (const auto &scope : scopes)
            diagnostic(Code::UnbalancedScope, {scope.second, 1}, "Unclosed MTEXT formatting scope");
        return std::move(result);
    }

private:
    Document::Paragraph &paragraph() { return result.paragraphs.back(); }
    void diagnostic(Code code, Range source, const char *message, bool fatal = false) {
        if (failed) return;
        if (int(result.diagnostics.size()) >= options.maxDiagnostics) {
            if (code == Code::UnsupportedFormat && !fatal) return;
            result.diagnostics.push_back({Code::ResourceLimit, source, QStringLiteral("Text diagnostic limit exceeded"), true});
            failed = true;
            return;
        }
        result.diagnostics.push_back({code, source, QString::fromLatin1(message), fatal});
        failed = fatal;
    }
    void token(Kind kind, int start, int logicalStart) {
        result.tokens.push_back({kind, {start, position - start}, int(result.paragraphs.size()) - 1,
                                 {logicalStart, int(paragraph().text.size()) - logicalStart}});
    }
    int scalarLength(int at) const {
        return result.source.at(at).isHighSurrogate() && at + 1 < end && result.source.at(at + 1).isLowSurrogate() ? 2 : 1;
    }
    void appendScalar(char32_t scalar, Range source) {
        const int start = int(paragraph().text.size());
        // Not QString::fromUcs4(): it takes U+FEFF for a byte order mark and
        // drops it, and the scalar would then own no unit of the text.
        const int length = QChar::requiresSurrogates(scalar) ? 2 : 1;
        if (length == 2) {
            paragraph().text += QChar(QChar::highSurrogate(scalar));
            paragraph().text += QChar(QChar::lowSurrogate(scalar));
        } else {
            paragraph().text += QChar(char16_t(scalar));
        }
        paragraph().scalars.push_back({scalar, {start, length}, source});
        auto &spans = paragraph().spans;
        if (!spans.empty() && spans.back().style == style) {
            // A font command that repeats the name of the span's font holds the
            // name in a buffer of its own. The span takes the buffers of the
            // style here, so that the two names are compared once and not again
            // for every scalar that follows.
            auto &held = spans.back().style;
            if (held.font.constData() != style.font.constData() ||
                held.fontSpecification.constData() != style.fontSpecification.constData() ||
                held.language.constData() != style.language.constData())
                held = style;
            spans.back().text.length += length;
        } else {
            spans.push_back({{start, length}, style});
        }
    }
    void appendLiteral(int begin, int limit) {
        position = begin;
        while (position < limit) {
            const auto ch = result.source.at(position);
            const int length = std::min(scalarLength(position), limit - position);
            char32_t scalar = ch.unicode();
            if (length == 2 && position + 1 < limit)
                scalar = QChar::surrogateToUcs4(ch, result.source.at(position + 1));
            else if (ch.isSurrogate()) {
                scalar = 0xfffd;
                diagnostic(Code::InvalidUtf16, {position, 1}, "Ill-formed UTF-16 replaced only in layout");
            }
            appendScalar(scalar, {position, length});
            position += length;
        }
    }
    void finishParagraph(int sourceEnd) {
        paragraph().source.length = sourceEnd - paragraph().source.start;
        paragraph().emptyStyle = style;
        QTextBoundaryFinder boundaries(QTextBoundaryFinder::Grapheme, paragraph().text);
        for (int begin = 0, boundary = int(boundaries.toNextBoundary()); boundary >= 0;
             begin = boundary, boundary = int(boundaries.toNextBoundary()))
            paragraph().graphemes.push_back({begin, boundary - begin});
    }
    void breakParagraph(Range separator, char32_t scalar = '\n') {
        // A decoded CR followed immediately by a decoded LF is still one
        // paragraph boundary, regardless of its raw escape spelling.
        if (scalar == '\n' && previousBreakWasCr && paragraph().source.start == separator.start &&
            paragraph().text.isEmpty() && !result.tokens.empty() &&
            result.tokens.back().kind == Kind::ParagraphBreak) {
            result.paragraphs[result.paragraphs.size() - 2].breakSource.length += separator.length;
            result.tokens.back().source.length += separator.length;
            paragraph().source.start = separator.end();
            previousBreakWasCr = false;
            return;
        }
        const int logicalStart = int(paragraph().text.size());
        token(Kind::ParagraphBreak, separator.start, logicalStart);
        paragraph().breakSource = separator;
        finishParagraph(separator.start);
        result.paragraphs.emplace_back();
        paragraph().source.start = separator.end();
        previousBreakWasCr = scalar == '\r';
    }
    bool readUnicode(int at, ushort &unit) const {
        if (at + 7 > end || result.source.mid(at, 3) != QStringLiteral("\\U+")) return false;
        unsigned value = 0;
        for (int i = at + 3; i < at + 7; ++i) {
            const ushort c = result.source.at(i).unicode();
            const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 :
                              c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
            if (digit < 0) return false;
            value = value * 16 + unsigned(digit);
        }
        unit = ushort(value); return true;
    }
    bool unicodeEscape() {
        ushort first;
        if (!readUnicode(position, first)) return false;
        const int start = position, logicalStart = int(paragraph().text.size());
        position += 7;
        char32_t scalar = first;
        ushort second;
        if (QChar::isHighSurrogate(first) && readUnicode(position, second) && QChar::isLowSurrogate(second)) {
            scalar = QChar::surrogateToUcs4(first, second); position += 7;
        } else if (QChar::isSurrogate(first)) {
            scalar = 0xfffd;
            diagnostic(Code::InvalidUtf16, {start, 7}, "Ill-formed escaped UTF-16 replaced only in layout");
        }
        if (scalar == '\n' || scalar == '\r' || scalar == 0x2028 || scalar == 0x2029)
            breakParagraph({start, position - start}, scalar);
        else { appendScalar(scalar, {start, position - start}); token(Kind::Escape, start, logicalStart); }
        return true;
    }
    int terminator(int start) const {
        const int found = lookahead->semicolon[start];
        return found >= start && found < end ? found : -1;
    }
    bool stack(int start, int terminatorPosition, int logicalStart) {
        int split = -1;
        for (int i = start + 2; i < terminatorPosition; ++i) {
            if (result.source.at(i) == '\\' && i + 1 < terminatorPosition) { ++i; continue; }
            if (result.source.at(i) == '{' && lookahead->matchedCloseBrace[i] >= 0) {
                i = lookahead->matchedCloseBrace[i]; continue;
            }
            if (result.source.at(i) == '^' || result.source.at(i) == '/' || result.source.at(i) == '#') { split = i; break; }
        }
        position = terminatorPosition + 1;
        if (split < 0) {
            diagnostic(Code::MalformedEscape, {start, position - start}, "Stack has no separator");
            appendLiteral(start, position); token(Kind::Malformed, start, logicalStart); return true;
        }
        if (stackDepth >= options.maxStackDepth) {
            diagnostic(Code::ResourceLimit, {start, position - start}, "MTEXT stack depth exceeded", true); return true;
        }
        Document::Options childOptions = options;
        childOptions.initialStyle = style;
        int lowerStart = split + 1;
        if (result.source.at(split) == '^' && lowerStart < terminatorPosition && result.source.at(lowerStart) == ' ') ++lowerStart;
        auto upper = Parser(result.source, {start + 2, split - start - 2}, childOptions, stackDepth + 1, lookahead).run();
        auto lower = Parser(result.source, {lowerStart, terminatorPosition - lowerStart}, childOptions, stackDepth + 1, lookahead).run();
        const Range source{start, position - start};
        for (const auto *child : {&upper, &lower})
            for (const auto &entry : child->diagnostics)
                diagnostic(entry.code, entry.source, entry.message.toLatin1().constData(), entry.fatal);
        appendScalar(0xfffc, source);
        paragraph().objects.push_back({logicalStart, source, char(result.source.at(split).unicode()),
                                      std::make_shared<Document>(std::move(upper)), std::make_shared<Document>(std::move(lower))});
        token(Kind::Stack, start, logicalStart); return true;
    }
    bool numericEscape(ushort command, int start, int logicalStart) {
        // Numeric MTEXT arguments have an optional, immediate semicolon, unlike
        // font/stack arguments. Never search a later command's text for one.
        // Scan only ASCII syntax, so recovery cannot split a Unicode scalar.
        const auto digit = [&](int at) {
            return at < end && result.source.at(at) >= QChar('0') && result.source.at(at) <= QChar('9');
        };
        const auto caretSpace = [&](int at) {
            // Match existing native decoding of ASCII whitespace ^I..^M;
            // this lookahead does not consume or reinterpret the caret token.
            return options.decodeCaretCodes && at + 1 < end && result.source.at(at) == '^' &&
                   result.source.at(at + 1) >= QChar('I') && result.source.at(at + 1) <= QChar('M');
        };
        const int argumentStart = start + 2;
        int at = argumentStart;
        bool syntax = true;
        bool relative = false;
        const bool integer = command == 'C' || command == 'c';
        if (!integer && at < end && (result.source.at(at) == '+' || result.source.at(at) == '-')) ++at;
        int digits = 0;
        while (digit(at)) { ++at; ++digits; }
        if (!integer && at < end && result.source.at(at) == '.') {
            ++at;
            while (digit(at)) { ++at; ++digits; }
        }
        syntax = digits > 0;
        if (!integer && syntax) {
            if (at < end && result.source.at(at) == '.') {
                // Do not reinterpret malformed 1..5 as valid height 1 plus
                // literal .5. Retain this numeric spelling without state change.
                syntax = false;
                while (at < end && (digit(at) || result.source.at(at) == '.')) ++at;
            }
            if (at < end && (result.source.at(at) == 'e' || result.source.at(at) == 'E')) {
                int exponent = at + 1;
                const bool signedExponent = exponent < end &&
                    (result.source.at(exponent) == '+' || result.source.at(exponent) == '-');
                if (signedExponent) ++exponent;
                if (digit(exponent)) {
                    at = exponent;
                    while (digit(at)) ++at;
                } else if (signedExponent || exponent == end || result.source.at(exponent) == ';' ||
                           result.source.at(exponent) == '\\' || result.source.at(exponent) == '{' ||
                           result.source.at(exponent) == '}' || result.source.at(exponent).isSpace() ||
                           caretSpace(exponent)) {
                    // An incomplete exponent remains literal. In contrast, an
                    // ordinary word such as "example" after H2 starts with e
                    // but is not consumed as part of the numeric argument.
                    at = exponent;
                    syntax = false;
                }
            }
            if (at < end && result.source.at(at) == '.') {
                syntax = false;
                while (at < end && (digit(at) || result.source.at(at) == '.')) ++at;
            }
        }
        const int numberEnd = at;
        if (!integer && (command == 'H' || command == 'W') && at < end &&
            (result.source.at(at) == 'x' || result.source.at(at) == 'X')) {
            relative = true;
            ++at;
        }
        if (at < end && result.source.at(at) == ';') ++at;
        position = at;
        if (!syntax) {
            appendLiteral(start, position); token(Kind::Malformed, start, logicalStart);
            diagnostic(Code::MalformedEscape, {start, position - start}, "Invalid MTEXT numeric spelling");
            return true;
        }
        const QString argument = result.source.mid(argumentStart, numberEnd - argumentStart);
        bool valid = false;
        if (integer) {
            bool parsed = false;
            const auto number = argument.toUInt(&parsed);
            valid = parsed && number <= (command == 'C' ? 256u : 0xffffffu);
            if (valid) { style.color = int(number); style.trueColor = command == 'c'; }
        } else {
            bool parsed = false;
            const double number = argument.toDouble(&parsed);
            valid = parsed && std::isfinite(number) && (command == 'Q' ? std::abs(number) < 90.0 : number > 0.0);
            if (valid) {
                if (command == 'H') style.heightFactor = relative ? style.heightFactor * number : number / options.baseHeight;
                else if (command == 'W') style.widthFactor = relative ? style.widthFactor * number : number;
                else style.obliqueDegrees = number;
                if (!std::isfinite(style.heightFactor) || style.heightFactor <= 0 ||
                    !std::isfinite(style.widthFactor) || style.widthFactor <= 0) {
                    diagnostic(Code::ResourceLimit, {start, position - start}, "MTEXT scale overflow", true);
                    return true;
                }
            }
        }
        if (!valid) diagnostic(Code::MalformedEscape, {start, position - start}, "Invalid MTEXT format value");
        token(valid ? Kind::Format : Kind::Malformed, start, logicalStart);
        return true;
    }
    bool mtextEscape() {
        const int start = position, logicalStart = int(paragraph().text.size());
        if (position + 1 >= end) {
            ++position; appendScalar('\\', {start, 1}); token(Kind::Malformed, start, logicalStart);
            diagnostic(Code::MalformedEscape, {start, 1}, "Trailing MTEXT backslash"); return true;
        }
        const ushort command = result.source.at(position + 1).unicode();
        if (command == '\n' || command == '\r' || command == 0x2028 || command == 0x2029) {
            ++position; appendScalar('\\', {start, 1}); token(Kind::Malformed, start, logicalStart);
            diagnostic(Code::MalformedEscape, {start, 1}, "MTEXT backslash before a paragraph boundary"); return true;
        }
        if (command == '\\' || command == '{' || command == '}' || command == ';' || command == '^') {
            position += 2; appendScalar(command, {start, 2}); token(Kind::Escape, start, logicalStart); return true;
        }
        if (command == '~') {
            position += 2; appendScalar(0xa0, {start, 2}); token(Kind::Escape, start, logicalStart); return true;
        }
        if (command == 'P' || command == 'N' || command == 'X') {
            position += 2; breakParagraph({start, 2}); return true;
        }
        if (command == 'L' || command == 'l' || command == 'O' || command == 'o' || command == 'K' || command == 'k') {
            if (command == 'L' || command == 'l') style.underline = command == 'L';
            if (command == 'O' || command == 'o') style.overline = command == 'O';
            if (command == 'K' || command == 'k') style.strikeThrough = command == 'K';
            position += 2; token(Kind::Format, start, logicalStart); return true;
        }
        if (!((command >= 'A' && command <= 'Z') || (command >= 'a' && command <= 'z'))) {
            position += 1 + scalarLength(position + 1);
            appendLiteral(start, position); token(Kind::Malformed, start, logicalStart);
            diagnostic(Code::MalformedEscape, {start, position - start}, "Nonalphabetic MTEXT command retained literally"); return true;
        }
        if ((command == 'f' || command == 'F') && position + 2 < end && result.source.at(position + 2) == '{') {
            const int close = lookahead->matchedCloseBrace[position + 2];
            if (close >= 0 && close < end) {
                style.fontSpecification = result.source.mid(position + 3, close - position - 3);
                style.font = style.fontSpecification.isEmpty() ? QStringLiteral("standard") : style.fontSpecification;
                position = close + 1; token(Kind::Format, start, logicalStart); return true;
            }
            position += 2; appendLiteral(start, position); token(Kind::Malformed, start, logicalStart);
            diagnostic(Code::MalformedEscape, {start, position - start}, "Unterminated native MTEXT font directive"); return true;
        }
        if (command == 'H' || command == 'W' || command == 'Q' || command == 'C' || command == 'c')
            return numericEscape(command, start, logicalStart);
        const int close = command == 'S' ? lookahead->stackSemicolon[position + 2] : terminator(position + 2);
        if (close < 0 || close >= end) {
            position += 2; appendLiteral(start, position); token(Kind::Malformed, start, logicalStart);
            diagnostic(Code::MalformedEscape, {start, position - start}, "Unterminated MTEXT command"); return true;
        }
        if (command == 'S') return stack(start, close, logicalStart);
        const QString argument = result.source.mid(position + 2, close - position - 2);
        position = close + 1;
        if (command == 'f' || command == 'F') {
            style.fontSpecification = argument;
            style.font = argument.section('|', 0, 0);
            if (style.font.isEmpty()) style.font = QStringLiteral("standard");
            if (argument.contains('|'))
                diagnostic(Code::UnsupportedFormat, {start, position - start}, "Font modifiers retained but not resolved by parser");
        } else {
            diagnostic(Code::UnsupportedFormat, {start, position - start}, "Unsupported MTEXT command retained in source");
            token(Kind::Unknown, start, logicalStart); return true;
        }
        token(Kind::Format, start, logicalStart); return true;
    }

    const Document::Options &options;
    Style style;
    int stackDepth;
    int position;
    int end;
    bool failed = false;
    bool previousBreakWasCr = false;
    std::shared_ptr<const Lookahead> lookahead;
    Document result;
    std::vector<std::pair<Style, int>> scopes;
};
} // namespace

LC_TextDocument LC_TextDocument::parse(const QString &source, const Options &options) {
    const auto &style = options.initialStyle;
    if (source.size() > std::numeric_limits<int>::max() || source.size() > options.maxSourceUnits ||
        options.maxSourceUnits < 0 || options.maxScopeDepth < 0 || options.maxStackDepth < 0 || options.maxDiagnostics < 0 ||
        !std::isfinite(options.baseHeight) || options.baseHeight <= 0 ||
        !std::isfinite(style.heightFactor) || style.heightFactor <= 0 ||
        !std::isfinite(style.widthFactor) || style.widthFactor <= 0 ||
        !std::isfinite(style.obliqueDegrees) || std::abs(style.obliqueDegrees) >= 90) {
        Document result;
        result.source = source;
        result.diagnostics.push_back({source.size() > options.maxSourceUnits ? Code::ResourceLimit : Code::InvalidOptions,
                                      {}, QStringLiteral("Invalid text parser options or input limit exceeded"), true});
        return result;
    }
    return Parser(source, {0, int(source.size())}, options).run();
}

LC_TextDocument LC_TextDocument::plain(const QString &source) {
    return parse(source, Options{});
}
