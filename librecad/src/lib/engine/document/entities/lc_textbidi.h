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

#include <QString>
#include <Qt>
#include <vector>

namespace lc::textbidi {

struct Cluster {
    int start;
    int length;
    bool rightToLeft;
};

// Display-only overrides for traditional horizontal Chinese RTL. Indices and
// stored strings remain in logical UTF-16 order; generated controls map to -1.
struct DirectionalText {
    QString text;
    std::vector<int> sourcePositions;
    std::vector<int> displayPositions;
    int sourcePosition(int displayPosition) const;
};
DirectionalText directionalText(const QString &text, Qt::LayoutDirection direction);

// A source-mapped input to shaping itemization, not a visual glyph ordering.
// shapingLevels/displayLevels are indexed by original UTF-16 code unit.
// -1 denotes an X9-excluded character whose joining effect must be retained.
// contextBreaks has text.size()+1 entries indexed by gaps between source units;
// a true entry prevents shaping context from crossing an isolate or separator.
// Generated controls in the Chinese RTL projection also create source-gap
// boundaries. Font/script/language itemization is the layout service's job.
struct ShapingAnalysis {
    DirectionalText projection;
    int base = 0;
    std::vector<int> shapingLevels;
    std::vector<quint8> displayLevels;
    std::vector<bool> contextBreaks;
};
ShapingAnalysis shapingAnalysis(const QString &text, Qt::LayoutDirection direction);

// Font-independent visual order; callers retain their own font metrics.
std::vector<Cluster> visualClusters(const QString &text,
                                    Qt::LayoutDirection direction,
                                    bool legacyReversed = false);
std::vector<int> visualOrder(const QString &text, Qt::LayoutDirection direction);

// Kept for explicit legacy reversal, never for logical-order text editors.
QString mirrorByLine(const QString &input);

} // namespace lc::textbidi

#endif // LC_TEXTBIDI_H
