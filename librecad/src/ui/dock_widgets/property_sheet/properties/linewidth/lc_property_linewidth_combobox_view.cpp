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

#include "lc_property_linewidth_combobox_view.h"

#include "lc_peninforegistry.h"
#include "lc_property_linewidth_combobox.h"
#include "lc_property_view_utils.h"

const QByteArray LC_PropertyLineWidthComboboxView::VIEW_NAME = QByteArrayLiteral("LinewidthCombobox");
const QByteArray LC_PropertyLineWidthComboboxView::ATTR_SHOW_BY_LAYER = QByteArrayLiteral("showByLayer");

QComboBox* LC_PropertyLineWidthComboboxView::doCreateEditCombobox(QWidget* parent) {
    return new LC_PropertyLineWidthCombobox(this, parent, m_showByLayer, false);
}

void LC_PropertyLineWidthComboboxView::doApplyAttributes(const LC_PropertyViewDescriptor& info) {
    info.load(ATTR_SHOW_BY_LAYER, m_showByLayer);
}

void LC_PropertyLineWidthComboboxView::doDrawValueDetails(const QStyle* style, const RS2::LineWidth& value, QPainter& painter,
                                                          const QRect& rect) const {
    QRect textRect = rect;

    const auto pensRegistry = LC_PenInfoRegistry::instance();
    const auto lineTypeIcon = pensRegistry->getLineWidthIcon(value);

    QRect iconRect = rect;
    iconRect.setWidth(iconRect.height());
    iconRect.adjust(2, 2, -2, -2);

    lineTypeIcon.paint(&painter, iconRect);

    textRect.setLeft(iconRect.right() + 3);

    if (textRect.isValid()) {
        const QString linetypeName = pensRegistry->getLineWidthText(value);
        LC_PropertyViewUtils::drawElidedText(painter, linetypeName, textRect, style);
    }
}
