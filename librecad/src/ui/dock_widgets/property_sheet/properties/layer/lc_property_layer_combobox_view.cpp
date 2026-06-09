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

#include "lc_property_layer_combobox_view.h"

#include "lc_property_layer.h"
#include "lc_property_layer_combobox.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyLayerComboBoxView::VIEW_NAME = QByteArrayLiteral("LayerCombobox");

QComboBox* LC_PropertyLayerComboBoxView::doCreateEditCombobox(QWidget* parent) {
    return new LC_PropertyLayerComboBox(this, parent);
}

void LC_PropertyLayerComboBoxView::doDrawValueDetails(const QStyle* style, const ValueType& layer, QPainter& painter,
                                                      const QRect& rect) const {
    QRect textRect = rect;

    QRect colorRect = rect;
    colorRect.setWidth(colorRect.height());
    colorRect.adjust(2, 2, -2, -2);
    colorRect.adjust(3, 3, -3, -3);

    painter.fillRect(colorRect, Qt::black);
    colorRect.adjust(1, 1, -1, -1);
    painter.fillRect(colorRect, layer->getPen().getColor());

    textRect.setLeft(colorRect.right() + 5 + 3);

    if (textRect.isValid()) {
        LC_PropertyViewUtils::drawElidedText(painter, layer->getName(), textRect, style);
    }
}
