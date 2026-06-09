/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_property_qstring_font_combobox_view.h"

#include "lc_property_qstring_font_combobox.h"

const QByteArray LC_PropertyQStringFontComboboxView::VIEW_NAME = QByteArrayLiteral("FontName");

void LC_PropertyQStringFontComboboxView::doDrawValueDetails(const QStyle* style, const ValueType& value, QPainter& painter,
                                                            const QRect& rect) const {
    QRect textRect = rect;
    textRect.adjust(2, 2, -2, -2);

    if (textRect.isValid()) {
        const QString name = value;
        LC_PropertyViewUtils::drawElidedText(painter, name, textRect, style);
    }
}

QComboBox* LC_PropertyQStringFontComboboxView::doCreateEditCombobox(QWidget* parent) {
    return new LC_PropertyQStringFontCombobox(this, parent);
}
