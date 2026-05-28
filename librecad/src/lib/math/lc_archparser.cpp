/****************************************************************************
**
* Parser for architectural feet-inch distance notation.
*
* Returns distances in inches. Does not call RS_Math::eval.
* Designed to be called from distance-entry actions when the global
* "Architectural feet-inch input" setting is enabled.
*
****************************************************************************/

#include "lc_archparser.h"

#include <QRegularExpression>

namespace {

// Only allow characters that can appear in architectural notation.
// Rejects expressions like pi*10, 10+3, sqrt(2), etc. so they fall
// through to RS_Math::eval.
bool hasInvalidChars(const QString& s) {
    for (const QChar c : s) {
        if (!c.isDigit()
                && c != ' '
                && c != '.'
                && c != '\''
                && c != '"'
                && c != '-'
                && c != '/')
            return true;
    }
    return false;
}

inline double feetToInches(double feet) {
    return feet * 12.0;
}

} // namespace

/**
 * Parse an architectural distance string and return the value in inches.
 *
 * Disambiguation rules (applied in order, most-specific first):
 *
 *   With explicit foot mark ('):
 *     A' B-C/D ["?]  →  feet=A  inches=B  fraction=C/D
 *     A' [B] ["?]    →  feet=A  inches=B (or 0)
 *
 *   Without foot mark (construction shorthand):
 *     A-B-C/D        →  feet=A  inches=B  fraction=C/D
 *     A-B C/D        →  feet=A  inches=B  fraction=C/D
 *     A-B/C          →  inches=A  fraction=B/C
 *     A-B            →  feet=A  inches=B
 *     A B/C          →  inches=A  fraction=B/C
 *     A/B            →  fraction=A/B inches
 *     A[.dec]        →  inches=A  (plain decimal)
 *
 * Rejects:
 *   - empty input
 *   - characters outside [0-9 .'"-/]  (catches math ops and alpha)
 *   - denominator == 0
 *   - result <= 0
 */
double LC_ArchParser::parse(const QString& input, bool* ok) {
    bool okTmp = false;
    if (!ok) ok = &okTmp;
    *ok = false;

    const QString s = input.trimmed();
    if (s.isEmpty())
        return 0.0;

    // Reject any character that cannot appear in architectural notation.
    if (hasInvalidChars(s))
        return 0.0;

    double result = 0.0;

    if (s.contains('\'')) {
        // ----------------------------------------------------------------
        // Patterns with explicit foot mark
        // ----------------------------------------------------------------

        // Priority 1: A' B-C/D ["?]
        // Examples: 2'4-7/8   2' 4-7/8"
        {
            static const QRegularExpression re(
                R"(^(\d+\.?\d*)\s*'\s*(\d+\.?\d*)\s*-\s*(\d+)\s*/\s*(\d+)\s*"?$)"
            );
            const auto m = re.match(s);
            if (m.hasMatch()) {
                const double denom = m.captured(4).toDouble();
                if (denom == 0.0) return 0.0;
                result = feetToInches(m.captured(1).toDouble())
                       + m.captured(2).toDouble()
                       + m.captured(3).toDouble() / denom;
                if (result <= 0.0) return 0.0;
                *ok = true;
                return result;
            }
        }

        // Priority 2: A' [B ["?]]
        // Examples: 2'   2'4"   24'7.75   2'4.5"
        // The inches group is optional (handles bare A' case).
        {
            static const QRegularExpression re(
                R"(^(\d+\.?\d*)\s*'(?:\s*(\d+\.?\d*)\s*"?)?$)"
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
    // Example: 10-3-7/8  →  feet=10 in=3 frac=7/8  =  123.875
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+)\s*-\s*(\d+)\s*/\s*(\d+)$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double denom = m.captured(4).toDouble();
            if (denom == 0.0) return 0.0;
            result = feetToInches(m.captured(1).toDouble())
                   + m.captured(2).toDouble()
                   + m.captured(3).toDouble() / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 4: A-B C/D
    // Example: 10-3 7/8  →  feet=10 in=3 frac=7/8  =  123.875
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+)\s+(\d+)\s*/\s*(\d+)$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double denom = m.captured(4).toDouble();
            if (denom == 0.0) return 0.0;
            result = feetToInches(m.captured(1).toDouble())
                   + m.captured(2).toDouble()
                   + m.captured(3).toDouble() / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 5: A-B/C  (inches + fractional inches, hyphen separator)
    // Example: 237-7/8  →  in=237 frac=7/8  =  237.875
    // Disambiguated from A-B (feet-in) because the second part is a fraction.
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+)\s*/\s*(\d+)$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double denom = m.captured(3).toDouble();
            if (denom == 0.0) return 0.0;
            result = m.captured(1).toDouble()
                   + m.captured(2).toDouble() / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 6: A-B  (feet-inches, no fraction)
    // Example: 10-3  →  feet=10 in=3  =  123.0
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)\s*-\s*(\d+\.?\d*)$)"
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
    // Example: 4 7/8  →  in=4 frac=7/8  =  4.875
    {
        static const QRegularExpression re(
            R"(^(\d+)\s+(\d+)\s*/\s*(\d+)$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double denom = m.captured(3).toDouble();
            if (denom == 0.0) return 0.0;
            result = m.captured(1).toDouble()
                   + m.captured(2).toDouble() / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 8: A/B  (simple fraction, inches)
    // Example: 7/8  →  0.875
    {
        static const QRegularExpression re(
            R"(^(\d+)\s*/\s*(\d+)$)"
        );
        const auto m = re.match(s);
        if (m.hasMatch()) {
            const double denom = m.captured(2).toDouble();
            if (denom == 0.0) return 0.0;
            result = m.captured(1).toDouble() / denom;
            if (result <= 0.0) return 0.0;
            *ok = true;
            return result;
        }
    }

    // Priority 9: plain decimal or integer (inches)
    // Examples: 235.75   120
    {
        static const QRegularExpression re(
            R"(^(\d+\.?\d*)$)"
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
