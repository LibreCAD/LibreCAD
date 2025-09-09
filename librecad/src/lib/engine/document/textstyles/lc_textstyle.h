/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_TEXTSTYLE_H
#define LC_TEXTSTYLE_H
#include <QString>

#include "rs_flags.h"

class LC_TextStyle{
public:
    LC_TextStyle() = default;
    double getFixedTextHeight() const {return m_fixedTextHeight;}
    double getWidthFactor() const {return m_widthFactor;}
    double getObliqueAngle() const {return m_obliqueAngle;}
    int getGenFlag() const {return m_genFlag;}
    double getLastHeight() const {return m_lastHeight;}
    QString getFontName() const {return m_fontName;}
    QString getBigFont() const {return m_bigFontName;}
    int getFontFamilyItalicBold() const {return m_fontFamilyItalicBold;}
    void setFixedTextHeight(double fixedTextHeight);
    void setWidthFactor(double widthFactor);
    void setObliqueAngle(double obliqueAngle);
    void setGenFlag(int genFlag);
    void setLastHeight(double lastHeight);
    void setFontName(const QString& fontName);
    void setBigFont(const QString& bigFont);
    void setFontFamilyItalicBold(int fontFamilyItalicBold);
    void setName(const QString& name);
    QString getName() const {return m_name;}
    void setFlags(unsigned flags) {m_flags.setFlags(flags);};
    unsigned getFlags() const {return m_flags.getFlags();}
private:
    QString m_name;
    RS_Flags m_flags {0};
    double m_fixedTextHeight {0};          /*!< Fixed text height (0 not set), code 40 */
    double m_widthFactor {0};              /*!< Width factor, code 41 */
    double m_obliqueAngle {0.0};           /*!< Oblique angle, code 50 */
    int m_genFlag {0};                     /*!< Text generation flags, code 71 */
    double m_lastHeight {0.0};             /*!< Last height used, code 42 */
    QString  m_fontName;                   /*!< primary font file name, code 3 */
    QString  m_bigFontName;                /*!< bigfont file name or blank if none, code 4 */
    int m_fontFamilyItalicBold {0};         /*!< ttf font family, italic and bold flags, code 1071 */
};

#endif // LC_TEXTSTYLE_H
