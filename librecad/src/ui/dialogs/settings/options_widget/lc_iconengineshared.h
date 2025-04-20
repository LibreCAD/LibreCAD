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

#ifndef LC_ICONENGINESHARED_H
#define LC_ICONENGINESHARED_H
#include <QGuiApplication>
#include <QString>

namespace LC_SVGIconEngineAPI {

    static constexpr int ANY_STATE = 2;
    static constexpr int ANY_MODE = 4;

    enum IconMode {
        Normal, Disabled, Active, Selected, AnyMode
    };

    enum IconState {
        On, Off, AnyState
    };

    enum ColorType{
        Main,
        Accent,
        Background
    };

    [[maybe_unused]] static const char* KEY_ICONS_OVERRIDES_DIR = "LCI_BaseDir";
    [[maybe_unused]] static const char* KEY_COLOR_MAIN = "LCI_ColorMain";
    [[maybe_unused]] static const char* KEY_COLOR_ACCENT = "LCI_ColorAccent";
    [[maybe_unused]] static const char* KEY_COLOR_BG = "LCI_ColorBack";

    QString getColorAppKeyName(QString baseName, int mode, int state);

    inline void setColorAppProperty(QString baseKey, IconMode mode, IconState state, QString value){
        QString key = getColorAppKeyName(baseKey, mode, state);
        qApp->setProperty(key.toStdString().c_str(),   value);
    }

    inline QString getColorAppProperty(QString baseKey, IconMode mode, IconState state){
        QString key = getColorAppKeyName(baseKey, mode, state);
        QVariant vProperty = qApp->property(key.toStdString().c_str());
        if (vProperty.isValid()) {
            return vProperty.value<QString>();
        }
        return QString();
    }
}

#endif // LC_ICONENGINESHARED_H
