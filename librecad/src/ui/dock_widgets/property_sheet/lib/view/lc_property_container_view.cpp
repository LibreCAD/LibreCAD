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

#include "lc_property_container_view.h"

#include "lc_property_container.h"
#include "lc_property_view_part.h"

const QByteArray LC_PropertyContainerView::VIEW_NAME = QByteArrayLiteral("Default");

LC_PropertyContainerView::LC_PropertyContainerView(LC_PropertyContainer* property)
    : LC_PropertyView(property), m_owner{property} {
}

int LC_PropertyContainerView::doGetSubPropertyCount() const {
    return m_owner->childProperties().size();
}

LC_Property* LC_PropertyContainerView::doGetSubProperty(const int index) {
    return m_owner->childProperties()[index];
}

void LC_PropertyContainerView::doBuildViewParts(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    buildPartBackground(ctx, parts);
    buildPartChildrenExpansion(ctx, parts);
    buildPartName(ctx, parts);
}

void LC_PropertyContainerView::buildPartName(const LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) const {
    LC_PropertyViewPart part(ctx.rect.marginsRemoved(ctx.margins));
    part.setPropertyDescriptionAsTooltip(*m_owner);

    if (part.rect.isValid()) {
        part.funPaint = [this](const LC_PropertyPaintContext& paintContext, const LC_PropertyViewPart& item) {
            const auto painter = paintContext.painter;
            const QFont oldFont = painter->font();
            const QPen oldPen = painter->pen();

            QFont font = oldFont;
            font.setBold(true);
            painter->setFont(font);

            painter->setPen(paintContext.getTextColor(isEditableByUser()));

            const auto displayName = getProperty()->getDisplayName();
            const QString elidedName = painter->fontMetrics().elidedText(displayName, Qt::ElideRight, item.rect.width());
            painter->drawText(item.rect, Qt::AlignLeading | Qt::AlignVCenter | Qt::TextSingleLine, elidedName);

            painter->setPen(oldPen);
            painter->setFont(oldFont);
        };
        parts.append(part);
    }
}

void LC_PropertyContainerView::buildPartBackground(const LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    LC_PropertyViewPart part(ctx.rect);
    if (part.rect.isValid()) {
        part.funPaint = [](const LC_PropertyPaintContext& paintContext, const LC_PropertyViewPart& item) {
            const auto color = (paintContext.isActive)
                                   ? paintContext.getPalette().color(QPalette::Highlight)
                                   : paintContext.getAlternatedButtonColor();
            paintContext.painter->fillRect(item.rect, color);
        };
        parts.append(part);
    }
}
