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

#ifndef LC_ICONCOLORSOPTIONS_H
#define LC_ICONCOLORSOPTIONS_H

#include "lc_iconengineshared.h"

// fixme - sand - no copy assignment operator!
class LC_IconColorsOptions {
public:
    LC_IconColorsOptions();
    LC_IconColorsOptions(const LC_IconColorsOptions& other);
    QString getKeyBaseName(LC_SVGIconEngineAPI::ColorType type);
    void loadSettings();
    void save();
    QString getColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type) const;
    void setColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, const QString& color);
    void applyOptions();
    void mark();
    void restore();
    bool isIconOverridesChanged() const;
    void getAvailableStyles(QStringList &list) const;
    void resetToDefaults();
    void apply(const LC_IconColorsOptions &other);
    // Returns true if the active Qt color scheme (or, on Qt < 6.5, the
    // current QPalette) indicates dark mode. Used to seed icon defaults
    // so #000 strokes don't disappear against a dark toolbar.
    static bool isDarkColorScheme();
    void setIconsOverridesDir(const QString& path) {m_iconOverridesDir = path;}
    QString getIconsOverridesDir() {return m_iconOverridesDir;}
    bool loadFromFile(const QString& styleName);
    QString loadStyleNameFromFile(const QString& styleName) const;
    QString getNameOfStyleFile(const QString &name) const;
    bool saveToFile(const QString &styleName) const;
    bool removeStyle(const QString &styleName) const;
protected:
    QHash<int, QString> m_colors;
    QHash<int, QString> m_colorsMarkCopy;
    QString m_iconOverridesDir;
    QString m_iconOverridesDirMarkCopy;

    static int iconHashKey(const LC_SVGIconEngineAPI::IconMode mode, const LC_SVGIconEngineAPI::IconState state,
                           const LC_SVGIconEngineAPI::ColorType type) {
        return (mode << 6) + (state << 4) + type;
    }
    QString getSettingsKeyName(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type);
    void loadColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, const QString& defaultValue);
    void saveColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type);
    void applyColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type);
    void exportColor(LC_SVGIconEngineAPI::IconMode mode, LC_SVGIconEngineAPI::IconState state, LC_SVGIconEngineAPI::ColorType type, QJsonArray &array) const;
    QString getTypeStr(LC_SVGIconEngineAPI::ColorType type) const;
    QString getStateStr(LC_SVGIconEngineAPI::IconState state) const;
    QString getModeStr(LC_SVGIconEngineAPI::IconMode mode) const;

    bool parseIconMode(const QString &val, LC_SVGIconEngineAPI::IconMode &mode);
    bool parseIconState(const QString &val, LC_SVGIconEngineAPI::IconState &state);
    bool parseColorType(const QString &val, LC_SVGIconEngineAPI::ColorType &type);
};

#endif
