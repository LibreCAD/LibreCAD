/****************************************************************************
**
* Parser for architectural feet-inch distance notation.
* Returns distances in inches.
*
* Supports construction-style shorthand input like:
*   10-3       = 10 feet 3 inches = 123.0"
*   10-3 7/8   = 123.875"
*   237-7/8    = 237.875"
*   4 7/8      = 4.875"
*   2'4-7/8    = 28.875"
*
* Called when the global "Architectural feet-inch input" setting is enabled.
* Does not modify RS_Math::eval or global command parsing.
*
****************************************************************************/

#ifndef LC_ARCHPARSER_H
#define LC_ARCHPARSER_H

#include <QString>

/**
 * Architectural feet-inch distance parser.
 *
 * All returned values are in inches. Returns ok=false for any input that
 * does not unambiguously match a supported architectural pattern so callers
 * can fall back to RS_Math::eval.
 */
class LC_ArchParser {
public:
    /**
     * Parse an architectural distance string and return the value in inches.
     *
     * @param input  The user-entered string (trimmed internally).
     * @param ok     Set to true on success, false if the string does not
     *               match any supported pattern. Must not be null.
     * @return       Distance in inches, or 0.0 when ok is false.
     */
    static double parse(const QString& input, bool* ok);
};

#endif // LC_ARCHPARSER_H
