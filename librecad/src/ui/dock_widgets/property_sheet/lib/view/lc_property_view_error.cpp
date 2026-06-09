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

#include "lc_property_view_error.h"

#include "lc_property_view_part.h"
#include "lc_property_view_utils.h"

LC_PropertyViewError::LC_PropertyViewError(LC_Property* property, const QString& error)
    : LC_PropertyViewWithValue(property), m_error(error) {
}

bool LC_PropertyViewError::doBuildPartValue([[maybe_unused]] LC_PropertyPaintContext& ctx, LC_PropertyViewPart& part) {
    part.funPaint = [this](const LC_PropertyPaintContext& paintContext, const LC_PropertyViewPart& p) {
        const QPen oldPen = paintContext.painter->pen();
        paintContext.painter->setPen(Qt::red);
        LC_PropertyViewUtils::drawElidedText(*paintContext.painter, m_error, p.rect, paintContext.style());
        paintContext.painter->setPen(oldPen);
    };
    return true;
}

LC_PropertyView* LC_PropertyViewError::createErrorView(LC_Property* property, const QString& error) {
    return new LC_PropertyViewError(property, error);
}
