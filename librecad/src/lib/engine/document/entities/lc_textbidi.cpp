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

#include <fribidi-bidi.h>
#include <fribidi-brackets.h>

#include <algorithm>
#include <limits>
#include <new>
#include <numeric>
#include <stdexcept>

namespace lc::textbidi {

namespace {

FriBidiCharType bidiType(QChar::Direction direction) {
  switch (direction) {
  case QChar::DirL:
    return FRIBIDI_TYPE_LTR;
  case QChar::DirR:
    return FRIBIDI_TYPE_RTL;
  case QChar::DirAL:
    return FRIBIDI_TYPE_AL;
  case QChar::DirEN:
    return FRIBIDI_TYPE_EN;
  case QChar::DirES:
    return FRIBIDI_TYPE_ES;
  case QChar::DirET:
    return FRIBIDI_TYPE_ET;
  case QChar::DirAN:
    return FRIBIDI_TYPE_AN;
  case QChar::DirCS:
    return FRIBIDI_TYPE_CS;
  case QChar::DirNSM:
    return FRIBIDI_TYPE_NSM;
  case QChar::DirBN:
    return FRIBIDI_TYPE_BN;
  case QChar::DirB:
    return FRIBIDI_TYPE_BS;
  case QChar::DirS:
    return FRIBIDI_TYPE_SS;
  case QChar::DirWS:
    return FRIBIDI_TYPE_WS;
  case QChar::DirLRE:
    return FRIBIDI_TYPE_LRE;
  case QChar::DirLRO:
    return FRIBIDI_TYPE_LRO;
  case QChar::DirRLE:
    return FRIBIDI_TYPE_RLE;
  case QChar::DirRLO:
    return FRIBIDI_TYPE_RLO;
  case QChar::DirPDF:
    return FRIBIDI_TYPE_PDF;
  case QChar::DirLRI:
    return FRIBIDI_TYPE_LRI;
  case QChar::DirRLI:
    return FRIBIDI_TYPE_RLI;
  case QChar::DirFSI:
    return FRIBIDI_TYPE_FSI;
  case QChar::DirPDI:
    return FRIBIDI_TYPE_PDI;
  case QChar::DirON:
    return FRIBIDI_TYPE_ON;
  }
  return FRIBIDI_TYPE_ON;
}

void reorderParagraph(const QString &text, Qt::LayoutDirection direction,
                      std::vector<Cluster>::iterator begin,
                      std::vector<Cluster>::iterator end) {
  if (begin == end) {
    return;
  }
  std::vector<FriBidiCharType> types;
  std::vector<FriBidiBracketType> brackets;
  std::vector<int> clusterForScalar;
  std::vector<int> firstScalar;
  for (auto it = begin; it != end; ++it) {
    firstScalar.push_back(static_cast<int>(types.size()));
    for (int i = it->start; i < it->start + it->length; ++i) {
      uint scalar = text.at(i).unicode();
      if (text.at(i).isHighSurrogate() && i + 1 < it->start + it->length &&
          text.at(i + 1).isLowSurrogate()) {
        scalar = QChar::surrogateToUcs4(text.at(i), text.at(i + 1));
        ++i;
      }
      const auto type = bidiType(QChar::direction(scalar));
      types.push_back(type);
      brackets.push_back(type == FRIBIDI_TYPE_ON ? fribidi_get_bracket(scalar)
                                                 : FRIBIDI_NO_BRACKET);
      clusterForScalar.push_back(static_cast<int>(it - begin));
    }
  }

  const int length = static_cast<int>(types.size());
  std::vector<FriBidiLevel> levels(length);
  FriBidiParType base = direction == Qt::RightToLeft   ? FRIBIDI_PAR_RTL
                        : direction == Qt::LeftToRight ? FRIBIDI_PAR_LTR
                                                       : FRIBIDI_PAR_ON;
  if (!fribidi_get_par_embedding_levels_ex(types.data(), brackets.data(),
                                           length, &base, levels.data())) {
    throw std::bad_alloc();
  }
  std::vector<FriBidiStrIndex> map(length);
  std::iota(map.begin(), map.end(), 0);
  if (!fribidi_reorder_line(FRIBIDI_FLAG_REORDER_NSM, types.data(), length, 0,
                            base, levels.data(), nullptr, map.data())) {
    throw std::bad_alloc();
  }

  // Apply the scalar map to whole graphemes (UBA L3). UTF-16 pairs and
  // combining marks retain their logical order within each glyph cluster.
  std::vector<bool> emitted(end - begin, false);
  std::vector<Cluster> ordered;
  ordered.reserve(end - begin);
  for (auto scalar : map) {
    const auto index = clusterForScalar[scalar];
    if (!emitted[index]) {
      auto cluster = begin[index];
      cluster.rightToLeft = (levels[firstScalar[index]] & 1) != 0;
      ordered.push_back(cluster);
      emitted[index] = true;
    }
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
