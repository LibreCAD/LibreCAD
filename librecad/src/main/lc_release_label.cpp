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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 * ********************************************************************************
 */

#include "main.h"

#include <map>

#include <QObject>

QString LCReleaseLabel() {
    const QString version{XSTR(LC_VERSION)};
    const std::map<QString, QString> labelMap = {
        {"rc", QObject::tr("Release Candidate")},
        {"beta", QObject::tr("BETA")},
        {"alpha", QObject::tr("ALPHA")}
    };
    for (const auto& [key, value]: labelMap) {
        if (version.contains(key, Qt::CaseInsensitive)) {
            return value;
        }
    }

    // Issue #2371: default version to alpha
    return QObject::tr("ALPHA");
}
