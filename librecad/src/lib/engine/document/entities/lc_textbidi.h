/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

#ifndef LC_TEXTBIDI_H
#define LC_TEXTBIDI_H

class QString;

namespace lc::textbidi {

/**
 * Reverse each line of @p input by code point, keeping surrogate pairs
 * together. Lines (separated by '\n') are mirrored independently; line
 * order is preserved.
 *
 * Used to translate between an RS_MText / RS_Text logical-order text field
 * and the editor's visually-mirrored display when drawingDirection ==
 * RightToLeft. AutoCAD's drawingDirection is positional, not Unicode
 * bidi — UAX#9 leaves EN digits direction-immune, so only an explicit
 * mirror makes pure-digit strings like "1234" visibly flip on toggle.
 *
 * Involutive: mirrorByLine(mirrorByLine(s)) == s.
 *
 * Single-line input is supported transparently — without a newline the
 * function reduces to a whole-string mirror.
 */
QString mirrorByLine(const QString &input);

} // namespace lc::textbidi

#endif // LC_TEXTBIDI_H
