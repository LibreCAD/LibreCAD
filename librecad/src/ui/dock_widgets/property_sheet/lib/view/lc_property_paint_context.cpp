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

#include "lc_property_paint_context.h"

#include <QPalette>
#include <QStyleOption>

#include "lc_properties_sheet.h"

QStyle* LC_PropertyPaintContext::style() const {
    return sheet->style();
}

void LC_PropertyPaintContext::initStyleOption(QStyleOption& option) const {
    option.initFrom(sheet);
}

const QPalette& LC_PropertyPaintContext::getPalette() const {
    return sheet->palette();
}

QPalette::ColorGroup LC_PropertyPaintContext::getCurrentColorGroup() const {
    return getPalette().currentColorGroup();
}

QColor LC_PropertyPaintContext::getTextColor(const bool normalText) const {
    QPalette::ColorRole role;
    QPalette::ColorGroup group = normalText ? getPalette().currentColorGroup() : QPalette::Disabled;
    if (isActive) {
        role = QPalette::HighlightedText;
    }
    else if (normalText) {
        role = QPalette::Text;
    }
    else {
        group = QPalette::Active;
        role = QPalette::PlaceholderText;
    }
    return getPalette().color(group, role);
}

QColor LC_PropertyPaintContext::getAlternatedButtonColor() const {
    auto color = getPalette().color(QPalette::Button);
    if (color == getPalette().color(QPalette::Window)) {
        color = color.darker(103);
    }
    return color;
}
