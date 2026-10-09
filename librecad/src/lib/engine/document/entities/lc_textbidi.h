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

// Font-independent visual order; callers retain their own font metrics.
std::vector<Cluster> visualClusters(const QString &text,
                                    Qt::LayoutDirection direction,
                                    bool legacyReversed = false);
std::vector<int> visualOrder(const QString &text, Qt::LayoutDirection direction);

// Kept for explicit legacy reversal, never for logical-order text editors.
QString mirrorByLine(const QString &input);

} // namespace lc::textbidi

#endif // LC_TEXTBIDI_H
