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

#include "lc_property_rscolor_combobox_view.h"

#include <QPaintEvent>

#include "lc_peninforegistry.h"
#include "lc_property_rscolor_combobox.h"
#include "lc_property_view_utils.h"
#include "qg_colorbox.h"

namespace {
    void  drawColor(const QStyle* style, const RS_Color& color, QPainter& painter, const QRect& rect) {
        QRect textRect = rect;
        const auto pensRegistry = LC_PenInfoRegistry::instance();

        QRect colorRect = rect;
        colorRect.setWidth(colorRect.height());
        colorRect.adjust(2, 2, -2, -2);

        const auto oldBrush = painter.brush();
        painter.setBrush(color);
        int adjustdelta = 0;

        if (color.isByLayer() || color.isByBlock()) {
            const auto lineTypeIcon = pensRegistry->getColorIcon(color, colorRect.width(), colorRect.height());
            lineTypeIcon.paint(&painter, colorRect);
        }
        else {
            colorRect.adjust(3, 3, -3, -3);
            painter.fillRect(colorRect, Qt::black);
            colorRect.adjust(1, 1, -1, -1);
            painter.fillRect(colorRect, color);
            adjustdelta = 4;
        }

        painter.setBrush(oldBrush);
        textRect.setLeft(colorRect.right() + 3 + adjustdelta);

        if (textRect.isValid()) {
            const QString colorName = pensRegistry->getColorName(color, LC_PenInfoRegistry::NATURAL);
            LC_PropertyViewUtils::drawElidedText(painter, colorName, textRect, style);
        }
    }
}


const QByteArray LC_PropertyRSColorComboBoxView::VIEW_NAME = QByteArrayLiteral("ColorCombobox");
const QByteArray LC_PropertyRSColorComboBoxView::ATTR_SHOW_BY_LAYER = QByteArrayLiteral("showByLayer");

void LC_PropertyRSColorComboBoxView::doApplyAttributes(const LC_PropertyViewDescriptor& info) {
    info.load(ATTR_SHOW_BY_LAYER, m_showByLayerItem);
}

void LC_PropertyRSColorComboBoxView::doDrawValue([[maybe_unused]] LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) {
    if (isMultiValue()) {
        LC_PropertyViewUtils::drawElidedText(painter, LC_Property::getMultiValuePlaceholder(), rect, painter.style());
        return;
    }
    const auto color = typedProperty().value();
    drawColor(painter.style(), color, painter, rect);
}

QWidget* LC_PropertyRSColorComboBoxView::doCreateValueEditor(QWidget* parent, const QRect& rect, const EditActivationContext* ctx) {
    const auto combo = new LC_PropertyRSColorComboBox(this, parent, m_showByLayerItem, false);
    combo->setGeometry(rect);
    if (isInplaceEditAllowed(ctx)) {
        combo->showPopup();
    }
    return combo;
}
