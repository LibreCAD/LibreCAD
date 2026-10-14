/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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

#ifndef LC_QT_BIDI_H
#define LC_QT_BIDI_H
#include <QString>
#include <Qt>
#include <vector>

namespace lc::qtbidi {

struct Paragraph {
    std::vector<quint8> levels;
    int base = 0;
};

// Shaping uses levels after I2, before L1 and the display-only BN adjustment.
// All arrays are indexed by UTF-16 code unit. X9-excluded characters have no
// resolved shaping level (represented by -1), but remain in the logical text:
// in particular, ZWJ/ZWNJ must still be available to the shaping engine.
struct ShapingParagraph {
    Paragraph display;
    std::vector<int> shapingLevels;
    std::vector<bool> isolateBoundaries;
};

// Resolve one paragraph without fonts or a GUI application.
Paragraph resolve(const QString &text, Qt::LayoutDirection direction);
ShapingParagraph resolveForShaping(const QString &text, Qt::LayoutDirection direction);
std::vector<int> reorder(const std::vector<quint8> &levels);

} // namespace lc::qtbidi
#endif // LC_QT_BIDI_H
