/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org
 Copyright (C) 2026 Dongxu Li (github.com/dxli)

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_LINFILE_H
#define LC_LINFILE_H

#include <QByteArray>
#include <QString>
#include <vector>

#include "lc_linetype.h"

/**
 * Reads and writes line type files: a "*NAME,description" line followed by an
 * "A,d1,d2,..." line, the text format CAD programs and machine vendors ship
 * line types in.
 */
namespace LC_LinFile {

/// One record the reader or the writer left out, and why.
struct Issue {
    enum class Kind {
        UnknownLine,          // neither a comment, a "*" line nor an "A," line
        MissingPattern,       // a "*" line without its "A," line
        OrphanPattern,        // an "A," line without a "*" line
        BadNumber,            // an element that is not a finite number
        FirstElementNotDash,  // the list begins with a gap
        CannotDraw,           // LC_LineTypeNames::canonicalDashes() finds nothing to draw
        TooManyElements,      // more elements than kMaxElements
        ComplexElement,       // a "[...]" element
        InvalidName,          // LC_LineType::checkName() refuses the name
        BuiltInName,          // the canonical name of a line type LibreCAD has built in, or By Layer / By Block
        DuplicateName,        // an earlier record the reader took has the same key(); for the writer, an earlier entry
    };
    Kind kind = Kind::UnknownLine;
    int line = 0;             // 1-based: the record's "*" line for a name, a
                              // missing pattern, a list that draws nothing or
                              // too many elements; the offending line
                              // otherwise; the entry's index + 1 for the writer
    QString text;             // the name, the element or the line the issue is about
};

struct ReadResult {
    std::vector<LC_LineType> entries;   // in file order
    std::vector<Issue> issues;          // in file order
};

/// The most elements one line type may have; a guard against a runaway line.
constexpr int kMaxElements = 256;

/// Reads the line types of a line type file. Every length is multiplied by
/// scale: 25.4 for a file in inches read into a drawing in millimeters.
ReadResult read(const QByteArray& data, double scale = 1.0);

/// The text of a line type file holding these entries, two lines each. An
/// entry the format cannot hold, or whose key an earlier entry has, is left
/// out and reported. Names and descriptions are written without the blanks
/// around them.
QByteArray write(const std::vector<LC_LineType>& entries, std::vector<Issue>* issues = nullptr);

}

#endif
