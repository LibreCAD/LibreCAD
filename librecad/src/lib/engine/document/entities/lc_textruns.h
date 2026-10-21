/****************************************************************************
** This file is part of LibreCAD, a 2D CAD program.
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
** SPDX-License-Identifier: GPL-2.0-or-later
****************************************************************************/
#ifndef LC_TEXTRUNS_H
#define LC_TEXTRUNS_H

#include "lc_textbidi.h"
#include "lc_textdocument.h"
#include <QChar>
#include <vector>

/** QtCore-only logical itemization. This service does not shape or reorder. */
class LC_TextRuns final {
public:
    using Range = LC_TextDocument::Range;
    struct Options {
        Qt::LayoutDirection direction = Qt::LayoutDirectionAuto;
        // Explicit policy for a context containing no script-specific scalar.
        QChar::Script fallbackScript = QChar::Script_Arabic;
    };
    struct ShapingStyle {
        QString font;
        QString fontSpecification;
        QByteArray language;
        double heightFactor = 1.0;
        double widthFactor = 1.0;
        double obliqueDegrees = 0.0;
        bool operator==(const ShapingStyle &other) const;
    };
    enum class Kind { Text, Tab, Object, BidiBoundary, Separator };
    struct Context {
        Range text; // Disjoint ownership, paragraph-local logical UTF-16.
        // X9 has no I2 level. Its original joining effects may apply on both
        // sides of a level boundary (UAX #9 X9 / 6.1), so neighboring X9-only
        // units may be borrowed here, never retained other-level characters.
        Range shapingText;
        int level = 0;
        bool shapeable = true;
    };
    struct Run {
        Kind kind = Kind::Text;
        Range text;
        Range context; // Context.shapingText; borrowed units are context, not output.
        int contextIndex = -1;
        int level = 0; // Effective pre-L1 level; excluded controls inherit locally.
        QChar::Script script = QChar::Script_Common;
        // A scalar known to have the resolved script, possibly in surrounding
        // context (e.g. a mark whose font differs from its base). Zero means the
        // explicit fallback policy was used; never cast Qt enums to HB tags.
        char32_t representativeScalar = 0;
        ShapingStyle style;
        std::vector<Range> sources; // Original-source intervals, possibly disjoint.
        int objectIndex = -1;
        bool rightToLeft() const { return (level & 1) != 0; }
        // HB BOT/EOT edges include borrowed controls, not just ownership. The
        // owned context edge remains available through Context.text.
        bool beginsContext() const { return text.start == context.start; }
        bool endsContext() const { return text.end() == context.end(); }
    };
    struct Paragraph {
        QString text;
        Range source;
        Range breakSource;
        std::vector<LC_TextDocument::Span> paintSpans;
        std::vector<LC_TextDocument::InlineObject> objects;
        // Script/grapheme data comes from public QtCore; retain its identity
        // alongside the separately versioned bidi resolver/profile policy.
        QChar::UnicodeVersion unicodeVersion = QChar::currentUnicodeVersion();
        // Each logical UTF-16 unit maps to its complete original scalar spelling.
        // Both units of a surrogate pair map to the same source interval.
        std::vector<Range> sourceMap;
        lc::textbidi::ShapingAnalysis bidi; // Raw -1 X9 levels and final display data.
        std::vector<Context> contexts;
        std::vector<Run> runs; // Logical order, including non-shaping boundaries.
        std::vector<int> runForUnit;
        QString error; // Invalid public paragraph data is rejected, never guessed.
        bool ok() const { return error.isEmpty(); }
        std::vector<Range> sourceRanges(int start, int length) const;
        // Generated Chinese projection controls have no original-source range.
        std::vector<Range> projectedSourceRanges(int start, int length) const;
    };

    static Paragraph itemize(const LC_TextDocument::Paragraph &paragraph,
                             const Options &options);
    static Paragraph itemize(const LC_TextDocument::Paragraph &paragraph) {
        return itemize(paragraph, Options{});
    }
};

#endif
