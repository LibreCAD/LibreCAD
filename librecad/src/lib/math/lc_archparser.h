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

#ifndef LC_ARCHPARSER_H
#define LC_ARCHPARSER_H

#include <QString>

/**
 * Architectural feet-inch distance parser.
 *
 * All returned values are in inches. Plain decimal inputs intentionally return
 * ok=false so callers can treat them as drawing-unit math expressions.
 */
namespace LC_ArchParser {
/**
 * Parse an architectural distance string and return the value in inches.
 *
 * @param input  The user-entered string (trimmed internally).
 * @param ok     Set to true on success, false if the string does not
 *               match any supported pattern. May be null.
 * @return       Distance in inches, or 0.0 when ok is false.
 */
double parse(const QString& input, bool* ok);

} // namespace LC_ArchParser

#endif // LC_ARCHPARSER_H
