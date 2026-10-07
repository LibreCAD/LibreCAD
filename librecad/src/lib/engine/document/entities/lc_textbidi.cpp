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

#include "lc_textbidi.h"

#include <QStringList>
#include <QTextBoundaryFinder>

#include "qt_bidi.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace lc::textbidi {

namespace {

void reorderParagraph(const QString &text, Qt::LayoutDirection direction,
                      std::vector<Cluster>::iterator begin,
                      std::vector<Cluster>::iterator end) {
    if (begin == end) return;
    const int start = begin->start;
    const auto &last = *(end - 1);
    const auto paragraph = text.mid(start, last.start + last.length - start);
    const auto resolved = lc::qtbidi::resolve(paragraph, direction);
    const auto order = lc::qtbidi::reorder(resolved.levels);
    std::vector<int> clusterForUnit(paragraph.size());
    for (auto it = begin; it != end; ++it) {
        const int first = it->start - start;
        std::fill(clusterForUnit.begin() + first,
                  clusterForUnit.begin() + first + it->length, int(it - begin));
    }
    std::vector<bool> emitted(end - begin, false);
    std::vector<Cluster> ordered;
    ordered.reserve(end - begin);
    for (int unit : order) {
        const int index = clusterForUnit[unit];
        if (emitted[index]) continue;
        auto cluster = begin[index];
        cluster.rightToLeft = (resolved.levels[cluster.start - start] & 1) != 0;
        ordered.push_back(cluster);
        emitted[index] = true;
    }
    std::copy(ordered.begin(), ordered.end(), begin);
}

} // namespace

std::vector<Cluster> visualClusters(const QString &text,
                                    Qt::LayoutDirection direction,
                                    bool legacyReversed) {
  if (text.size() > std::numeric_limits<int>::max()) {
    throw std::length_error("Bidi text exceeds index range");
  }
  std::vector<Cluster> clusters;
  QTextBoundaryFinder boundaries(QTextBoundaryFinder::Grapheme, text);
  for (int start = 0, end = boundaries.toNextBoundary(); end >= 0;
       start = end, end = boundaries.toNextBoundary()) {
    clusters.push_back({start, end - start, false});
  }
  if (clusters.empty()) {
    return clusters;
  }
  if (legacyReversed) {
    std::reverse(clusters.begin(), clusters.end());
    return clusters;
  }
  if (direction != Qt::RightToLeft &&
      std::all_of(text.begin(), text.end(), [](QChar ch) {
        return ch.unicode() >= 0x20 && ch.unicode() <= 0x7e;
      })) {
    return clusters;
  }

  auto paragraph = clusters.begin();
  for (auto it = clusters.begin(); it != clusters.end(); ++it) {
    if (text.at(it->start).direction() == QChar::DirB) {
      reorderParagraph(text, direction, paragraph, it);
      paragraph = it + 1;
    }
  }
  reorderParagraph(text, direction, paragraph, clusters.end());
  return clusters;
}

std::vector<int> visualOrder(const QString &text,
                             Qt::LayoutDirection direction) {
  std::vector<int> order;
  order.reserve(text.size());
  for (const auto &cluster : visualClusters(text, direction)) {
    for (int i = cluster.start; i < cluster.start + cluster.length; ++i) {
      order.push_back(i);
    }
  }
  return order;
}

QString mirrorByLine(const QString &input) {
  QStringList lines = input.split(QLatin1Char('\n'));
  for (QString &line : lines) {
    QString out;
    out.reserve(line.size());
    int i = line.size();
    while (i > 0) {
      --i;
      if (i > 0 && line.at(i).isLowSurrogate() &&
          line.at(i - 1).isHighSurrogate()) {
        out.append(line.at(i - 1));
        out.append(line.at(i));
        --i;
      } else {
        out.append(line.at(i));
      }
    }
    line = out;
  }
  return lines.join(QLatin1Char('\n'));
}

} // namespace lc::textbidi
