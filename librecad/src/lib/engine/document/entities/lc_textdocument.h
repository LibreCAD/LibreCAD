/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTDOCUMENT_H
#define LC_TEXTDOCUMENT_H

#include <QByteArray>
#include <QString>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/** Logical text and raw-source provenance. Parsing never rewrites source. */
class LC_TextDocument final {
public:
    enum class Syntax { Plain, MText };
    struct Range {
        int start = 0;
        int length = 0;
        int end() const { return start + length; }
        bool operator==(const Range &other) const { return start == other.start && length == other.length; }
    };
    // Equal text. A copy of a string shares its buffer, and that is looked at
    // before the characters are: a font name or a source can be as long as the
    // whole text, and styles and sources are compared once per scalar or stack.
    template <typename Text> static bool sameText(const Text &a, const Text &b) {
        return a.size() == b.size() && (a.constData() == b.constData() || a == b);
    }
    struct Style {
        QString font;
        QString fontSpecification;
        QByteArray language = "ar";
        double heightFactor = 1.0;
        double widthFactor = 1.0;
        double obliqueDegrees = 0.0;
        int color = -1; // -1 inherits the entity pen; otherwise ACI or RGB.
        bool trueColor = false;
        bool underline = false;
        bool overline = false;
        bool strikeThrough = false;
        bool operator==(const Style &other) const;
    };
    struct Options {
        Syntax syntax = Syntax::Plain;
        Style initialStyle;
        double baseHeight = 1.0; // Entity units, for absolute MTEXT \H values.
        bool decodeCaretCodes = false; // Enable only for original DXF payloads.
        bool decodeDxfEscapes = false; // %%c/d/p and \U+hhhh, not plain editor input.
        int maxSourceUnits = 1024 * 1024;
        int maxScopeDepth = 64;
        int maxStackDepth = 16;
        int maxDiagnostics = 1024;
    };
    struct Scalar {
        char32_t value = 0;
        Range text; // UTF-16 in this paragraph, never scalar ordinals.
        Range source; // UTF-16 in original source, including escape spelling.
    };
    struct Span {
        Range text;
        Style style;
    };
    struct InlineObject {
        int textPosition = 0; // U+FFFC in the parent logical paragraph.
        Range source;
        char separator = '^';
        std::shared_ptr<const LC_TextDocument> upper;
        std::shared_ptr<const LC_TextDocument> lower;
    };
    struct Paragraph {
        QString text;
        Range source;
        Range breakSource; // Original newline/\P; empty for the final paragraph.
        std::vector<Scalar> scalars;
        std::vector<Span> spans;
        std::vector<Range> graphemes; // Separate from shaping clusters.
        std::vector<InlineObject> objects;
        // Effective style at the paragraph's end. Empty paragraphs have no
        // spans, but their explicit font/metrics/paint must not be discarded.
        // Absent only for manually constructed public paragraph records.
        std::optional<Style> emptyStyle;
        std::vector<Range> sourceRanges(int start, int length) const;
        const Style *styleAt(int textPosition) const;
    };
    enum class TokenKind { Text, Escape, Format, ScopeOpen, ScopeClose, Stack, ParagraphBreak, Unknown, Malformed };
    struct Token {
        TokenKind kind = TokenKind::Text;
        Range source;
        int paragraph = 0;
        Range text; // A format token can have zero logical length.
    };
    struct Diagnostic {
        enum class Code { InvalidUtf16, MalformedEscape, UnbalancedScope, UnsupportedFormat, ResourceLimit, InvalidOptions };
        Code code = Code::MalformedEscape;
        Range source;
        QString message;
        bool fatal = false;
    };

    static LC_TextDocument parse(const QString &source, const Options &options);
    static LC_TextDocument plain(const QString &source);
    bool ok() const;

    QString source; // Immutable-by-contract input; never normalized or reordered.
    Range sourceRange; // Stack children share source but parse only this range.
    std::vector<Paragraph> paragraphs;
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;
};

#endif
