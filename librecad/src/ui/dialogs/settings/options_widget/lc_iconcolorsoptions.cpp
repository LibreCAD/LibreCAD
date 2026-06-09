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

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPalette>
#include <QRegularExpression>
#include <QStyleHints>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>


#include "rs_debug.h"
#include "rs_settings.h"

namespace {
    // Light-scheme template defaults — match the SVG template colors so that
    // the icon engine's replaceColor() short-circuits and the SVG renders
    // exactly as authored.
    constexpr const char* DEFAULT_MAIN_LIGHT       = "#000";
    constexpr const char* DEFAULT_ACCENT           = "#00ff7f";
    constexpr const char* DEFAULT_BACKGROUND_LIGHT = "#fff";

    // Dark-scheme defaults — picked to avoid substring collisions in the
    // engine's naive three-pass QString::replace() at lc_svgiconengine.cpp:368.
    // Main must not contain "#fff" (would be clobbered by the BG pass);
    // Background must not contain "#000" (would be clobbered by the Main pass).
    constexpr const char* DEFAULT_MAIN_DARK       = "#e6e6e6";
    constexpr const char* DEFAULT_BACKGROUND_DARK = "#1e1e1e";

    inline const char* defaultMain() {
        return LC_IconColorsOptions::isDarkColorScheme()
            ? DEFAULT_MAIN_DARK : DEFAULT_MAIN_LIGHT;
    }
    inline const char* defaultBackground() {
        return LC_IconColorsOptions::isDarkColorScheme()
            ? DEFAULT_BACKGROUND_DARK : DEFAULT_BACKGROUND_LIGHT;
    }
}
// fixme - sand - probably it's better to move it so some generic util, as it will be used in several places
bool LC_IconColorsOptions::isDarkColorScheme() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme != Qt::ColorScheme::Unknown) {
        return scheme == Qt::ColorScheme::Dark;
    }
    // Fall through when Qt can't tell us (some Linux platform themes,
    // headless builds) — use palette luminance as a fallback.
#endif
    return QGuiApplication::palette().color(QPalette::Window).lightnessF() < 0.5;
}

LC_IconColorsOptions::LC_IconColorsOptions() = default;

LC_IconColorsOptions::LC_IconColorsOptions(const LC_IconColorsOptions& other) {
    m_colors.insert(other.m_colors);
}

void LC_IconColorsOptions::resetToDefaults() {
    m_colors.clear();
    setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main, defaultMain());
    setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent, DEFAULT_ACCENT);
    setColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background, defaultBackground());
}

void LC_IconColorsOptions::apply(const LC_IconColorsOptions& other) {
    m_colors.clear();
    m_colors.insert(other.m_colors);
}

QString LC_IconColorsOptions::getKeyBaseName(const LC_SVGIconEngineAPI::ColorType type) {
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

QString LC_IconColorsOptions::getSettingsKeyName(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                                                 const LC_SVGIconEngineAPI::ColorType type) {
    const QString keyBase = getKeyBaseName(type);
    QString keyName = getColorAppKeyName(keyBase, mode, state);
    return keyName;
}

void LC_IconColorsOptions::loadColor(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                                     const LC_SVGIconEngineAPI::ColorType type, const QString& defaultValue) {
    const QString settingsKey = getSettingsKeyName(mode, state, type);
    const QString value = LC_GET_STR(settingsKey, defaultValue);
    const int hashKey = iconHashKey(mode, state, type);
    m_colors.insert(hashKey, value);
}

void LC_IconColorsOptions::saveColor(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                                     const LC_SVGIconEngineAPI::ColorType type) {
    const QString settingsKey = getSettingsKeyName(mode, state, type);
    const int hashKey = iconHashKey(mode, state, type);
    const QString value = m_colors.value(hashKey);
    LC_SET(settingsKey, value);
}

void LC_IconColorsOptions::applyColor(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                                      const LC_SVGIconEngineAPI::ColorType type) {
    const int hashKey = iconHashKey(mode, state, type);
    const QString value = m_colors.value(hashKey);

    const QString keyBase = getKeyBaseName(type);
    setColorAppProperty(keyBase, mode, state, value);
}

void LC_IconColorsOptions::loadSettings() {
    LC_GROUP("UiIconsStyling");
    {
        loadColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main, defaultMain());
        loadColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent, DEFAULT_ACCENT);
        loadColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background, defaultBackground());

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

        m_iconOverridesDir = LC_GET_STR("IconOverridesDir", "");
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

        LC_SET("IconOverridesDir", m_iconOverridesDir);
    }
    LC_GROUP_END();
}

void LC_IconColorsOptions::applyOptions() {
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

    qApp->setProperty(LC_SVGIconEngineAPI::KEY_ICONS_OVERRIDES_DIR, m_iconOverridesDir);
}

QString LC_IconColorsOptions::getColor(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                                       const LC_SVGIconEngineAPI::ColorType type) const {
    const int key = iconHashKey(mode, state, type);
    QString result = m_colors.value(key);
    return result;
}

void LC_IconColorsOptions::setColor(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                                    const LC_SVGIconEngineAPI::ColorType type, const QString& color) {
    const int key = iconHashKey(mode, state, type);
    m_colors.insert(key, color);
}

void LC_IconColorsOptions::mark() {
    m_colorsMarkCopy.clear();
    m_colorsMarkCopy.insert(m_colors);
    m_iconOverridesDirMarkCopy = m_iconOverridesDir;
}

void LC_IconColorsOptions::restore() {
    m_colors.clear();
    m_colors.insert(m_colorsMarkCopy);
    m_colorsMarkCopy.clear();
    m_iconOverridesDir = m_iconOverridesDirMarkCopy;
}

bool LC_IconColorsOptions::isIconOverridesChanged() const {
    return m_iconOverridesDir != m_iconOverridesDirMarkCopy;
}

void LC_IconColorsOptions::getAvailableStyles(QStringList& list) const {
    const QDir dir(m_iconOverridesDir);
    QStringList files = dir.entryList(QStringList("*.lcis"));
    for (const QString& file : std::as_const(files)) {
        QString styleName = loadStyleNameFromFile(file);
        if (!styleName.isEmpty()) {
            list << styleName;
        }
    }
}

namespace {
    const QRegularExpression REGEXP_STYLE_NAME_CLEANUP("[^a-zA-Z\\d]");
    const QString STYLE_FILE_MARK("LibreCAD Icons Style");
}

bool LC_IconColorsOptions::loadFromFile(const QString& styleName) {
    const QString absFileName = getNameOfStyleFile(styleName);
    auto jsonFile = QFile(absFileName);
    if (jsonFile.open(QFile::ReadOnly)) {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            LC_ERR << "Invalid json file. Parsing failed:" << parseError.error << parseError.errorString();
        }
        else {
            if (doc.isObject()) {
                m_colors.clear();
                const QJsonObject obj = doc.object();
                // auto name = obj.value("name").toString();
                const auto type = obj.value("type").toString();
                if (STYLE_FILE_MARK == type) {
                    QJsonArray settings = obj.value("settings").toArray();
                    const qsizetype size = settings.size();
                    for (qsizetype i = 0; i < size; i++) {
                        QJsonObject settingObj = settings[i].toObject();
                        auto mode = settingObj.value("iconMode").toString();
                        auto state = settingObj.value("iconState").toString();
                        auto colorTypeName = settingObj.value("colorType").toString();

                        LC_SVGIconEngineAPI::IconMode iconMode;
                        LC_SVGIconEngineAPI::IconState iconState;
                        LC_SVGIconEngineAPI::ColorType colorType;
                        if (parseIconMode(mode, iconMode) && parseIconState(state, iconState) && parseColorType(colorTypeName, colorType)) {
                            const auto value = settingObj.value("colorSvgString").toString();
                            setColor(iconMode, iconState, colorType, value);
                        }
                    }
                }
                else {
                    LC_ERR << "Not an Icons Style";
                }
            }
            else {
                LC_ERR << "Not a JSON doc object";
            }
        }
    }
    else {
        LC_ERR << "Can't open json file for reading. Parsing failed:" << absFileName;
    }
    return true;
}

QString LC_IconColorsOptions::loadStyleNameFromFile(const QString& styleName) const {
    QString absFileName = m_iconOverridesDir;
    absFileName.append("/").append(styleName);

    auto jsonFile = QFile(absFileName);
    jsonFile.open(QFile::ReadOnly);
    const QJsonDocument doc = QJsonDocument().fromJson(jsonFile.readAll());
    const QJsonObject obj = doc.object();
    const QString type = obj.value("type").toString();
    if (STYLE_FILE_MARK == type) {
        QString name = obj.value("name").toString();
        return name;
    }
    return "";
}

QString LC_IconColorsOptions::getNameOfStyleFile(const QString& name) const {
    auto correctedName = name;
    correctedName = correctedName.remove(REGEXP_STYLE_NAME_CLEANUP);
    const auto fileName = correctedName.append(".lcis");
    QString absFileName = m_iconOverridesDir;
    absFileName.append("/").append(fileName);
    return absFileName;
}

bool LC_IconColorsOptions::removeStyle(const QString& styleName) const {
    const QString absFileName = getNameOfStyleFile(styleName);
    auto file = QFile(absFileName);
    if (file.exists()) {
        return file.remove();
    }
    return false;
}

bool LC_IconColorsOptions::saveToFile(const QString& styleName) const {
    const QString absFileName = getNameOfStyleFile(styleName);
    auto jsonFile = QFile(absFileName);

    QJsonObject style;
    style.insert("name", QJsonValue::fromVariant(styleName));
    style.insert("type", QJsonValue::fromVariant(STYLE_FILE_MARK));

    QJsonArray settings;

    exportColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Main, settings);
    exportColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Accent, settings);
    exportColor(LC_SVGIconEngineAPI::AnyMode, LC_SVGIconEngineAPI::AnyState, LC_SVGIconEngineAPI::ColorType::Background, settings);

    exportColor(LC_SVGIconEngineAPI::Active, LC_SVGIconEngineAPI::On, LC_SVGIconEngineAPI::ColorType::Main, settings);
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

    const QJsonDocument doc(style);
    jsonFile.open(QFile::WriteOnly);
    jsonFile.write(doc.toJson());

    // LC_ERR << doc.toJson();
    return false;
}

void LC_IconColorsOptions::exportColor(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                                       const LC_SVGIconEngineAPI::ColorType type, QJsonArray& array) const {
    const int hashKey = iconHashKey(mode, state, type);
    const QString value = m_colors.value(hashKey);
    if (!value.isEmpty()) {
        QJsonObject setting;
        setting.insert("iconMode", QJsonValue::fromVariant(getModeStr(mode)));
        setting.insert("iconState", QJsonValue::fromVariant(getStateStr(state)));
        setting.insert("colorType", QJsonValue::fromVariant(getTypeStr(type)));
        setting.insert("colorSvgString", QJsonValue::fromVariant(value));
        array.push_back(setting);
    }
}

QString LC_IconColorsOptions::getTypeStr(const LC_SVGIconEngineAPI::ColorType type) const {
    switch (type) {
        case LC_SVGIconEngineAPI::ColorType::Background:
            return "background";
        case LC_SVGIconEngineAPI::ColorType::Main:
            return "main";
        case LC_SVGIconEngineAPI::ColorType::Accent:
            return "accent";
    }
    return "";
}

bool LC_IconColorsOptions::parseColorType(const QString& val, LC_SVGIconEngineAPI::ColorType& type) {
    const QString trimmed = val.trimmed();
    bool result = true;
    if ("background" == trimmed) {
        type = LC_SVGIconEngineAPI::ColorType::Background;
    }
    else if ("main" == trimmed) {
        type = LC_SVGIconEngineAPI::ColorType::Main;
    }
    else if ("accent" == trimmed) {
        type = LC_SVGIconEngineAPI::ColorType::Accent;
    }
    else {
        result = false;
    }
    return result;
}

QString LC_IconColorsOptions::getStateStr(const LC_SVGIconEngineAPI::IconState state) const {
    switch (state) {
        case LC_SVGIconEngineAPI::IconState::Off: {
            return "off";
        }
        case LC_SVGIconEngineAPI::IconState::On: {
            return "on";
        }
        case LC_SVGIconEngineAPI::IconState::AnyState: {
            return "any";
        }
    }
    return "";
}

bool LC_IconColorsOptions::parseIconState(const QString& val, LC_SVGIconEngineAPI::IconState& state) {
    const QString trimmed = val.trimmed();
    bool result = true;
    if ("off" == trimmed) {
        state = LC_SVGIconEngineAPI::IconState::Off;
    }
    else if ("on" == trimmed) {
        state = LC_SVGIconEngineAPI::IconState::On;
    }
    else if ("any" == trimmed) {
        state = LC_SVGIconEngineAPI::IconState::AnyState;
    }
    else {
        result = false;
    }
    return result;
}

QString LC_IconColorsOptions::getModeStr(const LC_SVGIconEngineAPI::IconMode mode) const {
    switch (mode) {
        case LC_SVGIconEngineAPI::IconMode::Active: {
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

bool LC_IconColorsOptions::parseIconMode(const QString& val, LC_SVGIconEngineAPI::IconMode& mode) {
    const QString trimmed = val.trimmed();
    bool result = true;
    if ("active" == trimmed) {
        mode = LC_SVGIconEngineAPI::IconMode::Active;
    }
    else if ("any" == trimmed) {
        mode = LC_SVGIconEngineAPI::IconMode::AnyMode;
    }
    else if ("normal" == trimmed) {
        mode = LC_SVGIconEngineAPI::IconMode::Normal;
    }
    else if ("disabled" == trimmed) {
        mode = LC_SVGIconEngineAPI::IconMode::Disabled;
    }
    else if ("selected" == trimmed) {
        mode = LC_SVGIconEngineAPI::IconMode::Selected;
    }
    else {
        result = false;
    }
    return result;
}
