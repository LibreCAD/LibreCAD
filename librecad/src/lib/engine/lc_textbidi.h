/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li github.com/dxli
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

namespace lc { namespace textbidi {

/**
 * Reverse each line of @p input by code point, keeping surrogate pairs
 * together. Lines (separated by '\n') are mirrored independently; line
 * order is preserved.
 *
 * Used by the MText edit dialog to flip the visible buffer when the
 * drawingDirection radio toggle is changed, so the same logical text
 * reads in the new direction without changing what's stored on the
 * entity.
 *
 * Involutive: mirrorByLine(mirrorByLine(s)) == s.
 *
 * Single-line input is supported transparently — without a newline the
 * function reduces to a whole-string mirror.
 */
QString mirrorByLine(const QString& input);

}}  // namespace lc::textbidi

#endif  // LC_TEXTBIDI_H
