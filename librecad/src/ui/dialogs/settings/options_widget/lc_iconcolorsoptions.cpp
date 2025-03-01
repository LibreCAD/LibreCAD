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

#include "lc_iconcolorsoptions.h"

#include "rs_settings.h"

LC_IconColorsOptions::LC_IconColorsOptions() {
}

LC_IconColorsOptions::LC_IconColorsOptions(LC_IconColorsOptions &other){
    colors.insert(other.colors);
}

void LC_IconColorsOptions::resetToDefaults() {
    colors.clear();
    setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main, "#000");
    setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent, "#00ff7f");
    setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background, "#fff");
}

void LC_IconColorsOptions::apply(LC_IconColorsOptions &other) {
  colors.clear();
  colors.insert(other.colors);
}

QString LC_IconColorsOptions::getKeyBaseName(LC_SVGIconEngineAPI::ColorType type){
    switch (type) {
        case LC_SVGIconEngineAPI::ColorType::Main:
            return LC_SVGIconEngineAPI::KEY_COLOR_MAIN;
        case LC_SVGIconEngineAPI::ColorType::Accent:
            return LC_SVGIconEngineAPI::KEY_COLOR_ACCENT;
        case LC_SVGIconEngineAPI::ColorType::Background:
            return LC_SVGIconEngineAPI::KEY_COLOR_BG;
        default:
            return "";
    }
}

QString LC_IconColorsOptions::getSettingsKeyName(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type){
    QString keyBase = getKeyBaseName(type);
    QString keyName = getColorAppKeyName(keyBase, mode, state);
    return keyName;
}

void LC_IconColorsOptions::loadColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, QString defaultValue){
    QString settingsKey = getSettingsKeyName(mode, state, type);
    QString value = LC_GET_STR(settingsKey, defaultValue);
    int hashKey = iconHashKey(mode, state, type);
    colors.insert(hashKey, value);
}

void LC_IconColorsOptions::saveColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type){
    QString settingsKey = getSettingsKeyName(mode, state, type);
    int hashKey = iconHashKey(mode, state, type);
    QString value = colors.value(hashKey);
    LC_SET(settingsKey, value);
}

void LC_IconColorsOptions::applyColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type){
    QString keyBase = getKeyBaseName(type);

    int hashKey = iconHashKey(mode, state, type);
    QString value = colors.value(hashKey);

    setColorAppProperty(keyBase, mode, state, value);
}

void LC_IconColorsOptions::loadSettings() {
    LC_GROUP("UiIconsStyling");
    {
        loadColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main, "#000");
        loadColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent, "#00ff7f");
        loadColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background, "#fff");

        loadColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, "");
        loadColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, "");

        loadColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, "");
        loadColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, "");

        loadColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, "");
        loadColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, "");

        loadColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, "");
        loadColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, "");
        loadColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, "");
        loadColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, "");

        iconOverridesDir = LC_GET_STR("IconOverridesDir", "");
    }
    LC_GROUP_END();
}

void LC_IconColorsOptions::save() {
    LC_GROUP("UiIconsStyling");
    {
        saveColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background);

        saveColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
        saveColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

        saveColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
        saveColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

        saveColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
        saveColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

        saveColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
        saveColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
        saveColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
        saveColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

        LC_SET("IconOverridesDir", iconOverridesDir);
    }
    LC_GROUP_END();
}

void LC_IconColorsOptions::applyOptions(){
    applyColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background);

    applyColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
    applyColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

    applyColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
    applyColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

    applyColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
    applyColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

    applyColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background);
    applyColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main);
    applyColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent);
    applyColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background);

    qApp->setProperty(LC_SVGIconEngineAPI::KEY_ICONS_OVERRIDES_DIR,  iconOverridesDir);
}

QString LC_IconColorsOptions::getColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type){
    int key = iconHashKey(mode, state, type);
    QString result = colors.value(key);
    return result;
}

void LC_IconColorsOptions::setColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, QString color){
    int key = iconHashKey(mode, state, type);
    colors.insert(key, color);
}

void LC_IconColorsOptions::mark(){
    colorsMarkCopy.clear();
    colorsMarkCopy.insert(colors);
    iconOverridesDirMarkCopy = iconOverridesDir;
}

void LC_IconColorsOptions::restore(){
    colors.clear();
    colors.insert(colorsMarkCopy);
    colorsMarkCopy.clear();
    iconOverridesDir = iconOverridesDirMarkCopy;
}

bool LC_IconColorsOptions::isIconOverridesChanged(){
    return iconOverridesDir != iconOverridesDirMarkCopy;
}
