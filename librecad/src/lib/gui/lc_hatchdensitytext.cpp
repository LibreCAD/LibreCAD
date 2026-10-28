/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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

#include "lc_hatchdensitytext.h"

#include <QCoreApplication>

#include <cmath>

#include "lc_hatchdensity.h"

namespace {

QString tr_(const char* key) {
    return QCoreApplication::translate("LC_HatchDensity", key);
}

QString formatCount(std::size_t v) {
    return QString::number(static_cast<qulonglong>(v));
}

QString formatCount(double v) {
    if (!std::isfinite(v) || v < 0.0) return QStringLiteral("--");
    return QString::number(static_cast<qulonglong>(std::llround(v)));
}

QString formatScale(double v) {
    if (!std::isfinite(v) || v <= 0.0) return QStringLiteral("--");
    return QString::number(v, 'g', 4);
}

} // namespace

namespace LC_HatchDensityText {

QString tooDenseMessage(const LC_HatchDensityReport& r, double currentScale) {
    const QString tmpl = tr_(
        "Hatch pattern too dense: about %1 pieces (limit %2); shown as a fill. "
        "Use scale >= %3.");
    QString pieces;
    if (r.reason == LC_HatchDensityReport::Reason::Budget) {
        pieces = formatCount(r.attempted);
    } else {
        pieces = formatCount(r.estimatedPieces);
    }
    const QString limit = formatCount(r.limit);
    const double sMin = LC_HatchDensity::minimumScale(r, currentScale, r.limit);
    return tmpl.arg(pieces, limit, formatScale(sMin));
}

QString patternLinesStatus(const LC_HatchDensityReport& r) {
    const bool dense = r.reason != LC_HatchDensityReport::Reason::None;
    if (!dense) {
        return tr_("generated");
    }
    const QString tmpl = tr_("not generated (~ %1 > limit %2)");
    const QString pieces = r.reason == LC_HatchDensityReport::Reason::Budget
        ? formatCount(r.attempted)
        : formatCount(r.estimatedPieces);
    return tmpl.arg(pieces, formatCount(r.limit));
}

QString importSummary(int n) {
    return QCoreApplication::translate(
               "LC_HatchDensity",
               "%n hatch(es) exceed the hatch pattern limit "
               "and are shown as fills.",
               nullptr, n);
}

QString r12Warning(int n) {
    return QCoreApplication::translate(
               "LC_HatchDensity",
               "R12 export: %n hatch(es) are too dense; "
               "their pattern is not written and they will look empty in the "
               "file.",
               nullptr, n);
}

} // namespace LC_HatchDensityText
