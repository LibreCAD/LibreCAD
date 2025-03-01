/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include "lc_iconengineshared.h"
#include <QIcon>

QString LC_SVGIconEngineAPI::getColorAppKeyName(QString baseName, int mode, int state){
    QString result = baseName;
    switch (mode) {
        case QIcon::Mode::Normal: {
            result.append("N");
            break;
        }
        case QIcon::Mode::Active: {
            result.append("A");
            break;
        }
        case QIcon::Mode::Disabled: {
            result.append("D");
            break;
        }
        case QIcon::Mode::Selected: {
            result.append("S");
            break;
        }
        default:
            break;
    }

    switch (state) {
        case QIcon::State::On: {
            result.append("N");
            break;
        }
        case QIcon::State::Off: {
            result.append("F");
            break;
        }
        default:
            break;
    }
    return result;
}
