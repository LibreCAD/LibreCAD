/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

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

#ifndef LC_LINETYPE_H
#define LC_LINETYPE_H

#include <QString>
#include <vector>

/**
 * One line type of a drawing: a built-in or an imported LTYPE record.
 */
struct LC_LineType {
    explicit LC_LineType(const QString& name);
    virtual ~LC_LineType() = default;

    QString name;                   // the spelling that won, verbatim for an import
    QString description;            // DXF group 3
    std::vector<double> pattern;    // DXF group 49, drawing units, before $LTSCALE

    enum class Origin : unsigned char { BuiltIn, Imported };
    Origin origin = Origin::Imported;
    bool hasImportedRecord = false; // an archived DRW_LType backs this entry

    /// what two spellings of one line type share: trimmed, NFC, ASCII upper case
    QString key() const;
};

#endif
