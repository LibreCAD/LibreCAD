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

#include <QFile>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "lc_iconcolorsoptions.h"

#include <QDir>

#include "rs_settings.h"
#include "rs_debug.h"

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
    int hashKey = iconHashKey(mode, state, type);
    QString value = colors.value(hashKey);

    QString keyBase = getKeyBaseName(type);
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

void LC_IconColorsOptions::getAvailableStyles(QStringList& list){
    QDir dir(iconOverridesDir);
    QStringList files = dir.entryList( QStringList( "*.lcis"));
    for(QString& file: files){
        QString styleName = loadStyleNameFromFile((file));
        if (!styleName.isEmpty()) {
            list << styleName;
        }
    }
}
namespace {
    QRegularExpression styleNameCleanupExpression("[^a-zA-Z\\d]");
    QString styleFileMark("LibreCAD Icons Style");
}

bool LC_IconColorsOptions::loadFromFile(QString styleName){
    QString absFileName = getNameOfStyleFile(styleName);
    QFile jsonFile = QFile(absFileName);
    if (jsonFile.open(QFile::ReadOnly)) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument().fromJson(jsonFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            LC_ERR << "Invalid json file. Parsing failed:" << parseError.error << parseError.errorString();
        } else {
            if (doc.isObject()) {
                colors.clear();
                QJsonObject obj = doc.object();
                auto name = obj.value("name").toString();
                auto type = obj.value("type").toString();
                if (styleFileMark == type) {
                    QJsonArray settings = obj.value("settings").toArray();
                    int size = settings.size();
                    for(int i = 0; i < size; i++){
                        QJsonObject settingObj = settings[i].toObject();
                        auto mode = settingObj.value("iconMode").toString();
                        auto state = settingObj.value("iconState").toString();
                        auto type = settingObj.value("colorType").toString();
                        auto value = settingObj.value("colorSvgString").toString();


                        LC_SVGIconEngineAPI::IconMode iconMode;
                        LC_SVGIconEngineAPI::IconState iconState;
                        LC_SVGIconEngineAPI::ColorType colorType;
                        if (parseIconMode(mode, iconMode) && parseIconState(state, iconState) && parseColorType(type, colorType)){
                            setColor(iconMode, iconState, colorType, value);
                        }
                    }
                }
                else{
                    LC_ERR << "Not an Icons Style";
                }
            }
            else{
                LC_ERR << "Not a JSON doc object";
            }
        }
    }
    else{
        LC_ERR << "Can't open json file for reading. Parsing failed:" << absFileName;
    }
    return true;
}

QString LC_IconColorsOptions::loadStyleNameFromFile(QString fileName){
    QString absFileName = iconOverridesDir;
    absFileName.append("/").append(fileName);

    QFile jsonFile = QFile(absFileName);
    jsonFile.open(QFile::ReadOnly);
    QJsonDocument doc = QJsonDocument().fromJson(jsonFile.readAll());
    QJsonObject obj  = doc.object();
    QString name = obj.value("name").toString();
    QString type = obj.value("type").toString();
    if ( styleFileMark == type) {
        return name;
    }
    return "";
}


QString LC_IconColorsOptions::getNameOfStyleFile(const QString &styleName) const{
    auto correctedName = styleName;
    correctedName = correctedName.remove(styleNameCleanupExpression);
    auto fileName = correctedName.append(".lcis");
    QString absFileName = iconOverridesDir;
    absFileName.append("/").append(fileName);
    return absFileName;
}

bool LC_IconColorsOptions::removeStyle(const QString &styleName) const{
    QString absFileName = getNameOfStyleFile(styleName);
    QFile file = QFile(absFileName);
    if (file.exists()) {
       return file.remove();
    }
    return false;
}

bool LC_IconColorsOptions::saveToFile(const QString &styleName) const {

    QString absFileName = getNameOfStyleFile(styleName);
    QFile jsonFile = QFile(absFileName);

    QJsonObject style;
    style.insert("name", QJsonValue::fromVariant(styleName));
    style.insert("type", QJsonValue::fromVariant(styleFileMark));

    QJsonArray  settings;

    exportColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background, settings);

    exportColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main,settings);
    exportColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, settings);
    exportColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, settings);

    exportColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, settings);
    exportColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Normal, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, settings);

    exportColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, settings);
    exportColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Selected, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, settings);

    exportColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Background, settings);
    exportColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::Disabled, LC_SVGIconEngineAPI::Off, LC_SVGIconEngineAPI::ColorType::Background, settings);

    style.insert("settings", settings);

    QJsonDocument doc(style);
    jsonFile.open(QFile::WriteOnly);
    jsonFile.write(doc.toJson());

    // LC_ERR << doc.toJson();
    return false;
}

void LC_IconColorsOptions::exportColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, QJsonArray &array) const{
    int hashKey = iconHashKey(mode, state, type);
    QString value = colors.value(hashKey);
    if (!value.isEmpty()){
        QJsonObject setting;
        setting.insert("iconMode", QJsonValue::fromVariant(getModeStr(mode)));
        setting.insert("iconState", QJsonValue::fromVariant(getStateStr(state)));
        setting.insert("colorType", QJsonValue::fromVariant(getTypeStr(type)));
        setting.insert("colorSvgString", QJsonValue::fromVariant(value));
        array.push_back(setting);
    }
}

const QString LC_IconColorsOptions::getTypeStr(LC_SVGIconEngineAPI::ColorType type) const{
    switch (type) {
        case (LC_SVGIconEngineAPI::ColorType::Background):
            return "background";
        case (LC_SVGIconEngineAPI::ColorType::Main):
            return "main";
        case (LC_SVGIconEngineAPI::ColorType::Accent):
            return "accent";
    }
    return "";
}

bool LC_IconColorsOptions::parseColorType(const QString &val, LC_SVGIconEngineAPI::ColorType &type) {
    QString trimmed = val.trimmed();
    bool result = true;
    if ("background" == trimmed){
        type = LC_SVGIconEngineAPI::ColorType::Background;
    }
    else if ("main" == trimmed){
        type = LC_SVGIconEngineAPI::ColorType::Main;
    }
    else if ("accent" == trimmed){
        type = LC_SVGIconEngineAPI::ColorType::Accent;
    }
    else{
        result = false;
    }
    return result;
}

const QString LC_IconColorsOptions::getStateStr(LC_SVGIconEngineAPI::IconState state) const{
    switch (state) {
        case (LC_SVGIconEngineAPI::IconState::Off): {
            return "off";
        }
        case (LC_SVGIconEngineAPI::IconState::On): {
            return "on";
        }
        case (LC_SVGIconEngineAPI::IconState::AnyState): {
            return "any";
        }
    }
    return "";
}

bool LC_IconColorsOptions::parseIconState(const QString &val, LC_SVGIconEngineAPI::IconState &state) {
    QString trimmed = val.trimmed();
    bool result = true;
    if ("off" == trimmed){
        state = LC_SVGIconEngineAPI::IconState::Off;
    }
    else if ("on" == trimmed){
        state = LC_SVGIconEngineAPI::IconState::On;
    }
    else if ("any" == trimmed){
        state = LC_SVGIconEngineAPI::IconState::AnyState;
    }
    else{
        result = false;
    }
    return result;
}

const QString LC_IconColorsOptions::getModeStr(LC_SVGIconEngineAPI::IconMode mode) const {
    switch (mode){
        case LC_SVGIconEngineAPI::IconMode::Active:{
            return "active";
        }
        case LC_SVGIconEngineAPI::IconMode::AnyMode: {
            return "any";
        }
        case LC_SVGIconEngineAPI::IconMode::Normal: {
            return "normal";
        }
        case LC_SVGIconEngineAPI::IconMode::Disabled: {
            return "disabled";
        }
        case LC_SVGIconEngineAPI::IconMode::Selected: {
            return "selected";
        }
    }
    return "";
}

bool LC_IconColorsOptions::parseIconMode(const QString &val, LC_SVGIconEngineAPI::IconMode &mode) {
    QString trimmed = val.trimmed();
    bool result = true;
    if ("active" == trimmed){
        mode = LC_SVGIconEngineAPI::IconMode::Active;
    }
    else if ("any" == trimmed){
        mode = LC_SVGIconEngineAPI::IconMode::AnyMode;
    }
    else if ("normal" == trimmed){
        mode = LC_SVGIconEngineAPI::IconMode::Normal;
    }
    else if ("disabled" == trimmed){
        mode = LC_SVGIconEngineAPI::IconMode::Disabled;
    }
    else if ("selected" == trimmed){
        mode = LC_SVGIconEngineAPI::IconMode::Selected;
    }
    else{
        result = false;
    }
    return result;
}
