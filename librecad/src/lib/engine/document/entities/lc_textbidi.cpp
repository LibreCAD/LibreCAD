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

#include "lc_textbidi.h"

#include <QStringList>

namespace lc::textbidi {

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
