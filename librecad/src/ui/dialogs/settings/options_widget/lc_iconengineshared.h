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

    [[maybe_unused]] static constexpr auto KEY_ICONS_OVERRIDES_DIR = "LCI_BaseDir";
    [[maybe_unused]] static constexpr auto KEY_COLOR_MAIN = "LCI_ColorMain";
    [[maybe_unused]] static constexpr auto KEY_COLOR_ACCENT = "LCI_ColorAccent";
    [[maybe_unused]] static constexpr auto KEY_COLOR_BG = "LCI_ColorBack";

    QString getColorAppKeyName(const QString& baseName, int mode, int state);

    inline void setColorAppProperty(const QString& baseKey, const IconMode mode, const IconState state, const QString& value){
        const QString key = getColorAppKeyName(baseKey, mode, state);
        // const auto basicString = key.toStdString();
        const std::string utf8_text = key.toUtf8().constData();
        const auto name = utf8_text.c_str();
        qApp->setProperty(name,   value);
    }

    inline QString getColorAppProperty(const QString& baseKey, const IconMode mode, const IconState state){
        const QString key = getColorAppKeyName(baseKey, mode, state);
        const std::string utf8_text = key.toUtf8().constData();
        const auto name = utf8_text.c_str();
        // const QVariant vProperty = qApp->property(key.toStdString().c_str());
         const QVariant vProperty = qApp->property(name);
        if (vProperty.isValid()) {
            return vProperty.value<QString>();
        }
        return QString();
    }
}

#endif
