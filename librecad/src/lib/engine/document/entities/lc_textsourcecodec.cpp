/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#include "lc_textsourcecodec.h"

#include <QStringConverter>
#include <algorithm>
#include <limits>

namespace {
using Codec = LC_TextSourceCodec;
using Document = LC_TextDocument;
using Kind = Document::TokenKind;
using Range = Document::Range;

QString owned(const QString &input) {
    return input.isNull() ? QString{} : QString(input.constData(), input.size());
}
QByteArray owned(const QByteArray &input) {
    return input.isNull() ? QByteArray{} : QByteArray(input.constData(), input.size());
}
Codec::Error error(Codec::ErrorCode code, Range source, const char *message) {
    return {code, source, QString::fromLatin1(message)};
}
bool valid(const Codec::Options &o) {
    return (o.syntax == Document::Syntax::Plain || o.syntax == Document::Syntax::MText) &&
        o.maxSourceUnits >= 0 && o.maxOutputUnits >= 0 && o.maxSourceBytes >= 0 && o.maxOutputBytes >= 0 &&
        o.maxScopeDepth >= 0 && o.maxStackDepth >= 0 && o.maxStackDepth <= 64 && o.maxDiagnostics >= 0 && o.maxTraversalNodes >= 0;
}
bool affected(ushort unit, const Codec::Options &o) {
    return unit == '^' || (o.targetCaretCodes && unit < 0x20);
}
bool affected(const QString &source, const Codec::Options &o) {
    return std::any_of(source.begin(), source.end(), [&](QChar ch) { return affected(ch.unicode(), o); });
}
bool affected(const QByteArray &source, const Codec::Options &o) {
    return std::any_of(source.begin(), source.end(), [&](char ch) { return affected(static_cast<unsigned char>(ch), o); });
}
bool numeric(const QString &source, const Document::Token &token) {
    if ((token.kind != Kind::Format && token.kind != Kind::Malformed) || token.source.length < 2 ||
        source.at(token.source.start) != '\\' || source.at(token.source.end() - 1) == ';') return false;
    const auto command = source.at(token.source.start + 1);
    return command == 'H' || command == 'W' || command == 'Q' || command == 'C' || command == 'c';
}
bool exponentNumeric(const QString &source, const Document::Token &token) {
    if (token.kind != Kind::Format || !numeric(source, token)) return false;
    const auto command = source.at(token.source.start + 1);
    // A relative H/W suffix follows (and finishes) the exponent grammar; a
    // subsequent literal e/E cannot fuse into this already completed number.
    const auto last = source.at(token.source.end() - 1);
    return command == 'Q' || ((command == 'H' || command == 'W') && last != 'x' && last != 'X');
}
QString unicodeEscape(ushort unit) {
    return QStringLiteral("\\U+") + QString::number(unit, 16).rightJustified(4, QChar('0')).toUpper();
}

struct Edit { Range source; QString replacement; };
class Conversion {
public:
    Conversion(const QString &source, const Codec::Options &options) : source(source), options(options) {
        sensitivePrefix.reserve(std::size_t(source.size()) + 1);
        sensitivePrefix.push_back(0);
        for (const auto unit : source)
            sensitivePrefix.push_back(sensitivePrefix.back() + (unit == '^' || unit.unicode() < 0x20 ? 1 : 0));
    }

    Codec::Error collect(const Document &document, int depth = 0) {
        if (depth > options.maxStackDepth) return error(Codec::ErrorCode::ResourceLimit, document.sourceRange, "Source traversal stack depth exceeded");
        for (std::size_t index = 0; index < document.tokens.size(); ++index) {
            if (++visited > options.maxTraversalNodes)
                return error(Codec::ErrorCode::ResourceLimit, document.tokens[index].source, "Source traversal node budget exceeded");
            const auto &token = document.tokens[index];
            if (token.kind == Kind::Stack) {
                const auto &objects = document.paragraphs.at(std::size_t(token.paragraph)).objects;
                const auto found = std::lower_bound(objects.begin(), objects.end(), token.text.start,
                    [](const Document::InlineObject &object, int at) { return object.textPosition < at; });
                if (found == objects.end() || !(found->source == token.source) || !found->upper || !found->lower)
                    return error(Codec::ErrorCode::SemanticMismatch, token.source, "Parsed stack ownership is inconsistent");
                if (auto failure = collect(*found->upper, depth + 1)) return failure;
                if (auto failure = collect(*found->lower, depth + 1)) return failure;
                continue; // Structural separator and optional '^ ' padding stay exact.
            }
            if (token.kind == Kind::Unknown || token.kind == Kind::Malformed) {
                if (sensitive(token.source))
                    return error(Codec::ErrorCode::AmbiguousSyntax, token.source, "Sensitive opaque or malformed MTEXT syntax cannot be transcoded losslessly");
                // An unterminated unknown command has only its prefix tokenized;
                // its apparent trailing text still has ambiguous argument ownership.
                if (token.kind == Kind::Malformed && token.source.length == 2 && source.at(token.source.start) == '\\' &&
                    source.at(token.source.start + 1).isLetter() && !numeric(source, token)) {
                    const auto &paragraph = document.paragraphs.at(std::size_t(token.paragraph));
                    const Range suffix{token.source.end(), paragraph.source.end() - token.source.end()};
                    if (sensitive(suffix) || sensitive(paragraph.breakSource))
                        return error(Codec::ErrorCode::AmbiguousSyntax, token.source, "Unterminated MTEXT argument has ambiguous sensitive ownership");
                }
                continue;
            }
            if (token.kind == Kind::Format || token.kind == Kind::ScopeOpen || token.kind == Kind::ScopeClose) {
                if (options.targetCaretCodes && token.kind == Kind::Format) {
                    for (int at = token.source.start; at < token.source.end(); ++at)
                        if (source.at(at).unicode() < 0x20)
                            return error(Codec::ErrorCode::AmbiguousSyntax, token.source, "A control in an opaque format argument cannot be safely caret-encoded");
                }
                continue;
            }
            const bool numericBoundary = index > 0 && document.tokens[index - 1].source.end() == token.source.start &&
                numeric(source, document.tokens[index - 1]);
            if (token.kind == Kind::Escape) {
                if (options.sourceCaretCodes && token.source.length == 2 && source.at(token.source.start) == '^') {
                    const int next = source.at(token.source.start + 1).unicode();
                    const ushort decoded = ushort(next == ' ' ? '^' : ((next - 64) % 126 + 126) % 126);
                    auto replacement = literal(decoded, numericBoundary);
                    // The literal e/E in \H2e^x is not part of its original
                    // numeric token. Decoding the caret to 8 would retroactively
                    // turn it into an exponent; an escape/whitespace would make
                    // it malformed instead. Close only that affected numeric
                    // token with its optional delimiter before the literal e.
                    if (index >= 2 && document.tokens[index - 1].kind == Kind::Text &&
                        document.tokens[index - 1].source.length == 1 &&
                        document.tokens[index - 1].source.end() == token.source.start &&
                        (source.at(token.source.start - 1) == 'e' || source.at(token.source.start - 1) == 'E') &&
                        exponentNumeric(source, document.tokens[index - 2]) &&
                        document.tokens[index - 2].source.end() == document.tokens[index - 1].source.start) {
                        const auto first = replacement.front();
                        if (first.isDigit() || first == '+' || first == '-' || first == ';' || first == '\\' ||
                            first == '{' || first == '}' || first.isSpace())
                            edits.push_back({{document.tokens[index - 2].source.end(), 0}, QStringLiteral(";")});
                    }
                    edits.push_back({token.source, std::move(replacement)});
                }
                continue; // Existing backslash/Unicode/%% escapes stay verbatim.
            }
            if (token.kind == Kind::Text || token.kind == Kind::ParagraphBreak) {
                for (int at = token.source.start; at < token.source.end(); ++at) {
                    const auto unit = source.at(at).unicode();
                    if (options.sourceCaretCodes && unit == '^' && at + 1 < token.source.end()) {
                        const int next = source.at(at + 1).unicode();
                        if (next <= 0x7f && next != '\r' && next != '\n') {
                            const ushort decoded = ushort(next == ' ' ? '^' : ((next - 64) % 126 + 126) % 126);
                            edits.push_back({{at, 2}, literal(decoded, numericBoundary)});
                            ++at;
                        }
                    } else if (options.targetCaretCodes && (unit == '^' || unit < 0x20)) {
                        const QString replacement = unit == '^' ? QStringLiteral("^ ") : QString(QChar('^')) + QChar(unit + 64);
                        edits.push_back({{at, 1}, replacement});
                    }
                }
            }
        }
        return {};
    }

    Codec::Result finish() {
        std::sort(edits.begin(), edits.end(), [](const Edit &a, const Edit &b) { return a.source.start < b.source.start; });
        qint64 length = source.size();
        int end = 0;
        for (const auto &edit : edits) {
            if (edit.source.start < end || edit.source.length < 0 || edit.source.end() > source.size())
                return {{}, error(Codec::ErrorCode::SemanticMismatch, edit.source, "Source edits overlap or exceed their original ownership"), false};
            length += edit.replacement.size() - edit.source.length;
            end = edit.source.end();
        }
        if (length < 0 || length > options.maxOutputUnits)
            return {{}, error(Codec::ErrorCode::ResourceLimit, {}, "Complete transcoded output exceeds the UTF-16 budget"), false};
        if (edits.empty()) return {owned(source), {}, false};
        QString result;
        result.reserve(qsizetype(length));
        end = 0;
        for (const auto &edit : edits) {
            result.append(source.constData() + end, edit.source.start - end);
            result.append(edit.replacement);
            end = edit.source.end();
        }
        result.append(source.constData() + end, source.size() - end);
        return {std::move(result), {}, true};
    }

private:
    bool sensitive(Range range) const {
        return sensitivePrefix[std::size_t(range.end())] != sensitivePrefix[std::size_t(range.start)];
    }
    QString literal(ushort unit, bool numericBoundary) const {
        if (options.syntax == Document::Syntax::MText) {
            if (unit == '\\' || unit == '{' || unit == '}' || unit == ';' || unit == '^')
                return QString(QChar('\\')) + QChar(unit);
            // A formerly separate caret token must not extend an unterminated
            // numeric argument (e.g. \H2^x -> \H28 would change height).
            if (numericBoundary && ((unit >= '0' && unit <= '9') || unit == '.' || unit == '+' || unit == '-' ||
                                    unit == 'e' || unit == 'E' || unit == 'x' || unit == 'X')) {
                if (options.decodeDxfEscapes) return unicodeEscape(unit);
                return QString(QChar('{')) + QChar(unit) + QChar('}');
            }
        }
        // Do not accidentally activate a new %% or plain \U+ escape by
        // concatenating separately decoded literal tokens with their neighbors.
        if (options.decodeDxfEscapes && (unit == '%' || (unit == '\\' && options.syntax == Document::Syntax::Plain)))
            return unicodeEscape(unit);
        return QString(QChar(unit));
    }
    const QString &source;
    const Codec::Options &options;
    qint64 visited = 0;
    std::vector<int> sensitivePrefix;
    std::vector<Edit> edits;
};

bool equivalent(const Document &a, const Document &b, qint64 &budget) {
    if (a.paragraphs.size() != b.paragraphs.size()) return false;
    for (std::size_t index = 0; index < a.paragraphs.size(); ++index) {
        if (--budget < 0) return false;
        const auto &left = a.paragraphs[index];
        const auto &right = b.paragraphs[index];
        if (left.text != right.text || left.emptyStyle.has_value() != right.emptyStyle.has_value() ||
            (left.emptyStyle && !(*left.emptyStyle == *right.emptyStyle)) || left.spans.size() != right.spans.size() ||
            left.objects.size() != right.objects.size()) return false;
        for (std::size_t span = 0; span < left.spans.size(); ++span) {
            if (--budget < 0 || !(left.spans[span].text == right.spans[span].text) || !(left.spans[span].style == right.spans[span].style)) return false;
        }
        for (std::size_t object = 0; object < left.objects.size(); ++object) {
            if (--budget < 0) return false;
            const auto &l = left.objects[object];
            const auto &r = right.objects[object];
            if (l.textPosition != r.textPosition || l.separator != r.separator || !l.upper || !l.lower || !r.upper || !r.lower ||
                !equivalent(*l.upper, *r.upper, budget) || !equivalent(*l.lower, *r.lower, budget)) return false;
        }
    }
    return true;
}
Document::Options parserOptions(const Codec::Options &options, bool caret) {
    Document::Options result;
    result.syntax = options.syntax;
    result.decodeCaretCodes = caret;
    result.decodeDxfEscapes = options.decodeDxfEscapes;
    result.maxSourceUnits = std::max(options.maxSourceUnits, options.maxOutputUnits);
    result.maxScopeDepth = options.maxScopeDepth;
    result.maxStackDepth = options.maxStackDepth;
    result.maxDiagnostics = options.maxDiagnostics;
    return result;
}
qint64 utf8Length(const QString &source) {
    qint64 length = 0;
    for (qsizetype at = 0; at < source.size(); ++at) {
        const auto unit = source.at(at).unicode();
        if (QChar::isHighSurrogate(unit) && at + 1 < source.size() && source.at(at + 1).isLowSurrogate()) { length += 4; ++at; }
        else length += unit < 0x80 ? 1 : unit < 0x800 ? 2 : 3;
    }
    return length;
}
} // namespace

LC_TextSourceCodec::Result LC_TextSourceCodec::transcode(const QString &source, const Options &options) {
    if (!valid(options)) return {{}, error(ErrorCode::InvalidOptions, {}, "Invalid source codec options"), false};
    if (source.size() > options.maxSourceUnits || source.size() > std::numeric_limits<int>::max())
        return {{}, error(ErrorCode::ResourceLimit, {}, "Source exceeds the UTF-16 budget"), false};
    if (options.sourceCaretCodes == options.targetCaretCodes || !affected(source, options)) {
        if (source.size() > options.maxOutputUnits) return {{}, error(ErrorCode::ResourceLimit, {}, "Output exceeds the UTF-16 budget"), false};
        return {owned(source), {}, false};
    }
    const auto original = Document::parse(source, parserOptions(options, options.sourceCaretCodes));
    if (!original.ok()) return {{}, error(ErrorCode::ResourceLimit, {}, "Source parser exceeded its safety limits"), false};
    Conversion conversion(source, options);
    if (auto failure = conversion.collect(original)) return {{}, std::move(failure), false};
    auto result = conversion.finish();
    if (!result) return result;
    const auto target = Document::parse(*result.source, parserOptions(options, options.targetCaretCodes));
    if (!target.ok()) return {{}, error(ErrorCode::ResourceLimit, {}, "Target parser exceeded its safety limits"), false};
    qint64 budget = options.maxTraversalNodes;
    if (!equivalent(original, target, budget))
        return {{}, error(budget < 0 ? ErrorCode::ResourceLimit : ErrorCode::SemanticMismatch, {},
                          "Transcoded source does not preserve complete parsed logical/style/stack semantics"), false};
    return result;
}

LC_TextSourceCodec::ByteResult LC_TextSourceCodec::transcode(const QByteArray &source, const Options &options) {
    if (!valid(options)) return {{}, error(ErrorCode::InvalidOptions, {}, "Invalid source codec options"), false};
    if (source.size() > options.maxSourceBytes)
        return {{}, error(ErrorCode::ResourceLimit, {}, "Source exceeds the byte budget"), false};
    if (options.sourceCaretCodes == options.targetCaretCodes || !affected(source, options)) {
        if (source.size() > options.maxOutputBytes) return {{}, error(ErrorCode::ResourceLimit, {}, "Output exceeds the byte budget"), false};
        return {owned(source), {}, false};
    }
    QStringDecoder decoder(QStringDecoder::Utf8, QStringConverter::Flag::ConvertInitialBom);
    const QString text = decoder(source);
    if (decoder.hasError() || text.toUtf8() != source)
        return {{}, error(ErrorCode::InvalidUtf8, {}, "Affected cross-convention source is not lossless UTF-8"), false};
    auto result = transcode(text, options);
    if (!result) return {{}, std::move(result.error), false};
    if (utf8Length(*result.source) > options.maxOutputBytes)
        return {{}, error(ErrorCode::ResourceLimit, {}, "Complete transcoded output exceeds the byte budget"), false};
    return {result.source->toUtf8(), {}, result.changed};
}
