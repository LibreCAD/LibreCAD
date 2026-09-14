/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 pcfixindude (github.com/pcfixindude)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#include <QRegularExpression>

#include "lc_archparser.h"
#include "rs_units.h"

namespace {

// Only allow characters that can appear in architectural notation.
// Rejects expressions like pi*10, 10+3, sqrt(2), etc. so they fall
// through to RS_Math::eval.
bool isValidNotation(const QString& s) {
    for (const QChar c : s) {
        if (!c.isDigit()
                && c != ' '
                && c != '.'
                && c != '\''
                && c != '"'
                && c != '-'
                && c != '/')
            return false;
    }
    return true;
}

inline double feetToInches(double feet) {
    return RS_Units::convert(feet, RS2::Foot, RS2::Inch);
}

bool isProperFraction(double numerator, double denominator) {
    return denominator > 0.0 && numerator >= 0.0 && numerator < denominator;
}

} // namespace

/**
 * Parse an architectural distance string and return the value in inches.
 *
 * Disambiguation rules (applied in order, most-specific first):
 *
 *   With explicit foot mark ('):
 *     A'[-]B-C/D ["?]    ->  feet=A  inches=B  fraction=C/D
 *     A'[-][B ]C/D ["?]  ->  feet=A  inches=B  fraction=C/D
 *     A'[-][B ["?]]      ->  feet=A  inches=B (or 0)
 *
 *   Without foot mark (construction shorthand):
 *     A-B-C/D        ->  feet=A  inches=B  fraction=C/D
 *     A-B C/D        ->  feet=A  inches=B  fraction=C/D
 *     A-B/C          ->  inches=A  fraction=B/C
 *     A-B            ->  feet=A  inches=B
 *     A B/C          ->  inches=A  fraction=B/C
 *     A/B            ->  fraction=A/B inches
 *     A"             ->  inches=A
 *
 * Rejects:
 *   - empty input
 *   - characters outside [0-9 .'"-/]  (catches math ops and alpha)
 *   - improper fractions such as 8/2, so normal math can evaluate them
 *   - result <= 0
 */
double LC_ArchParser::parse(const QString& input, bool* ok) {
    bool okTmp = false;
    if (nullptr == ok) {
        ok = &okTmp;
    }
    *ok = false;

    const QString s = input.trimmed();
    if (s.isEmpty())
        return 0.0;

    // Reject any character that cannot appear in architectural notation.
    if (!isValidNotation(s))
        return 0.0;

    double result = 0.0;

    if (s.contains('\'')) {
        // ----------------------------------------------------------------
        // Patterns with explicit foot mark
        // ----------------------------------------------------------------

        // Priority 1: A'[-]B-C/D ["?]
        // Examples: 2'4-7/8   2'-4-7/8"
        {
            static const QRegularExpression re(
                R"(^(\d+\.?\d*)\s*'\s*-?\s*(\d+\.?\d*)\s*-\s*(\d+)\s*/\s*(\d+)\s*"?$)"
            );
            const auto m = re.match(s);
            if (m.hasMatch()) {
                const double numerator = m.captured(3).toDouble();
                const double denom = m.captured(4).toDouble();
                if (!isProperFraction(numerator, denom)) return 0.0;
                result = feetToInches(m.captured(1).toDouble())
                       + m.captured(2).toDouble()
                       + numerator / denom;
                if (result <= 0.0) return 0.0;
                *ok = true;
                return result;
            }
        }

        // Priority 2: A'[-][B ]C/D ["?]
        // Examples: 2'-4 7/8"   2' 4 7/8"   2'-7/8"
        {
            static const QRegularExpression re(
                R"(^(\d+\.?\d*)\s*'\s*-?\s*(?:(\d+\.?\d*)\s+)?(\d+)\s*/\s*(\d+)\s*"?$)"
            );
            const auto m = re.match(s);
            if (m.hasMatch()) {
                const double inches = m.captured(2).isEmpty()
                                      ? 0.0
                                      : m.captured(2).toDouble();
                const double numerator = m.captured(3).toDouble();
                const double denom = m.captured(4).toDouble();
                if (!isProperFraction(numerator, denom)) return 0.0;
                result = feetToInches(m.captured(1).toDouble())
                       + inches
                       + numerator / denom;
                if (result <= 0.0) return 0.0;
                *ok = true;
                return result;
            }
        }

        // Priority 3: A'[-][B ["?]]
        // Examples: 2'   2'4"   2'-4"   24'7.75   2'4.5"
        // The inches group is optional (handles bare A' case).
        {
            static const QRegularExpression re(
                R"(^(\d+\.?\d*)\s*'(?:\s*-?\s*(\d+\.?\d*)\s*"?)?$)"
            );
            const auto m = re.match(s);
            if (m.hasMatch()) {
                const double inches = m.captured(2).isEmpty()
                                      ? 0.0
                                      : m.captured(2).toDouble();
                result = feetToInches(m.captured(1).toDouble()) + inches;
                if (result <= 0.0) return 0.0;
                *ok = true;
                return result;
            }
        }

        // No pattern matched for foot-mark input.
        return 0.0;
    }

    // ----------------------------------------------------------------
    // Patterns without explicit foot mark (construction shorthand)
    // ----------------------------------------------------------------

    // Priority 3: A-B-C/D
    // Example: 10-3-7/8 -> feet=10 in=3 frac=7/8 = 123.875
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+)\s*-\s*(\d+)\s*/\s*(\d+)\s*"?$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double numerator = m.captured(3).toDouble();
            const double denom = m.captured(4).toDouble();
            if (!isProperFraction(numerator, denom)) return 0.0;
            result = feetToInches(m.captured(1).toDouble())
                   + m.captured(2).toDouble()
                   + numerator / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 4: A-B C/D
    // Example: 10-3 7/8 -> feet=10 in=3 frac=7/8 = 123.875
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+)\s+(\d+)\s*/\s*(\d+)\s*"?$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double numerator = m.captured(3).toDouble();
            const double denom = m.captured(4).toDouble();
            if (!isProperFraction(numerator, denom)) return 0.0;
            result = feetToInches(m.captured(1).toDouble())
                   + m.captured(2).toDouble()
                   + numerator / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 5: A-B/C  (inches + fractional inches, hyphen separator)
    // Example: 237-7/8 -> in=237 frac=7/8 = 237.875
    // Disambiguated from A-B (feet-in) because the second part is a fraction.
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+)\s*/\s*(\d+)\s*"?$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double numerator = m.captured(2).toDouble();
            const double denom = m.captured(3).toDouble();
            if (!isProperFraction(numerator, denom)) return 0.0;
            result = m.captured(1).toDouble()
                   + numerator / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 6: A-B  (feet-inches, no fraction)
    // Example: 10-3 -> feet=10 in=3 = 123.0
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+\.?\d*)\s*"?$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            result = feetToInches(m.captured(1).toDouble())
                   + m.captured(2).toDouble();
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 7: A B/C  (integer inches + fraction, space-separated)
    // Example: 4 7/8 -> in=4 frac=7/8 = 4.875
    {
        static const QRegularExpression re(
            R"(^(\d+)\s+(\d+)\s*/\s*(\d+)\s*"?$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double numerator = m.captured(2).toDouble();
            const double denom = m.captured(3).toDouble();
            if (!isProperFraction(numerator, denom)) return 0.0;
            result = m.captured(1).toDouble()
                   + numerator / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 8: A/B  (simple fraction, inches)
    // Example: 7/8 -> 0.875
    {
        static const QRegularExpression re(
            R"(^(\d+)\s*/\s*(\d+)\s*"?$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double numerator = m.captured(1).toDouble();
            const double denom = m.captured(2).toDouble();
            if (!isProperFraction(numerator, denom)) return 0.0;
            result = numerator / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 9: A"  (explicit inches, but plain A without the quote
    // remains a drawing-unit math expression for the caller).
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*"$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            result = m.captured(1).toDouble();
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // No pattern matched.
    return 0.0;
}
