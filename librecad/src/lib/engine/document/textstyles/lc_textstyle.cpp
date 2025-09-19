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

#include "lc_textstyle.h"

void LC_TextStyle::setFixedTextHeight(const double fixedTextHeight) {
    m_fixedTextHeight = fixedTextHeight;
}

void LC_TextStyle::setWidthFactor(const double widthFactor) {
    m_widthFactor = widthFactor;
}

void LC_TextStyle::setObliqueAngle(const double obliqueAngle) {
    m_obliqueAngle = obliqueAngle;
}

void LC_TextStyle::setGenFlag(const int genFlag) {
    m_genFlag = genFlag;
}

void LC_TextStyle::setLastHeight(const double lastHeight) {
    m_lastHeight = lastHeight;
}

void LC_TextStyle::setFontName(const QString& fontName) {
    m_fontName = fontName;
}

void LC_TextStyle::setBigFont(const QString& bigFont) {
    m_bigFontName = bigFont;
}

void LC_TextStyle::setFontFamilyItalicBold(const int fontFamilyItalicBold) {
    m_fontFamilyItalicBold = fontFamilyItalicBold;
}

void LC_TextStyle::setName(const QString& name) {
    m_name = name;
}
