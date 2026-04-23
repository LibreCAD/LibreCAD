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

#include "lc_property_qstring_invalid_view_base.h"

const QByteArray LC_PropertyQStringInvalidViewBase::ATTR_INVALID_COLOR = QByteArrayLiteral("invalidColor");

LC_PropertyQStringInvalidViewBase::LC_PropertyQStringInvalidViewBase(LC_PropertyQString* property)
    : LC_PropertyQStringLineEditView(property), m_invalidColor(Qt::red) {
    m_multiline = false;
}

void LC_PropertyQStringInvalidViewBase::doApplyAttributes(const LC_PropertyViewDescriptor& atts) {
    atts.load(ATTR_INVALID_COLOR, m_invalidColor);
}

void LC_PropertyQStringInvalidViewBase::doDrawValue(LC_PropertyPaintContext& ctx, QStylePainter& painter, const QRect& rect) {
    const QPen oldPen = painter.pen();
    if ((m_invalidColor.alpha() != 0) && isNormalPainterState(painter) && !isPlaceholderColor() && !doCheckPropertyValid() &&
        isEditableByUser()) {
        painter.setPen(m_invalidColor.rgb());
    }
    LC_PropertyQStringLineEditView::doDrawValue(ctx, painter, rect);
    painter.setPen(oldPen);
}
