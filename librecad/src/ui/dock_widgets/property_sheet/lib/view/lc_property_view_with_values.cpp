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

#include "lc_property_view_with_values.h"

#include "lc_properties_sheet.h"
#include "lc_property.h"
#include "lc_property_event_context.h"
#include "lc_property_view_part.h"
#include "lc_property_view_utils.h"

void LC_PropertyViewWithValues::doBuildViewParts(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    buildPartBackground(ctx, parts);
    buildPartSelection(ctx, parts);
    buildPartChildrenExpansion(ctx, parts);
    buildPartName(ctx, parts);
    buildPartValues(ctx, parts);
}

bool LC_PropertyViewWithValues::isSplittable() const {
    return true;
}

LC_PropertyViewWithValues::LC_PropertyViewWithValues(LC_Property* property)
    : LC_PropertyView(property) {
}

void LC_PropertyViewWithValues::buildPartBackground(const LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    LC_PropertyViewPart part(ctx.rect);
    if (!part.rect.isValid()) {
        return;
    }
    part.funPaint = [](const LC_PropertyPaintContext& pc, const LC_PropertyViewPart& p) //
    {
        auto& painter = *pc.painter;
        const auto& rect = p.rect;
        const auto splitPos = pc.splitPos;

        const QPen oldPen = painter.pen();
        const QPen linesPen(pc.getPalette().color(QPalette::Button));
        painter.setPen(linesPen);

        // draw part borders
        painter.drawLine(rect.bottomLeft(), rect.bottomRight());
        painter.drawLine(splitPos, rect.top(), splitPos, rect.bottom());

        painter.setPen(oldPen);
    };

    parts.append(part);
}

void LC_PropertyViewWithValues::buildPartSelection(const LC_PropertyPaintContext& context, QList<LC_PropertyViewPart>& parts) {
    LC_PropertyViewPart part(context.rect);
    if (!part.rect.isValid()) {
        return;
    }

    part.funPaint = [](const LC_PropertyPaintContext& pc, const LC_PropertyViewPart& vp) {
        if (pc.isActive) {
            pc.painter->fillRect(vp.rect, pc.getPalette().color(QPalette::Highlight));
        }
    };

    parts.append(part);
}

void LC_PropertyViewWithValues::buildPartName(const LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    LC_PropertyViewPart part(ctx.rect.marginsRemoved(ctx.margins));
    part.rect.setRight(ctx.splitPos);
    part.setPropertyDescriptionAsTooltip(*getProperty());

    if (!part.rect.isValid()) {
        return;
    }

    part.funPaint = [this](const LC_PropertyPaintContext& pc, const LC_PropertyViewPart& vp) {
        pc.painter->save();
        pc.painter->setPen(pc.getTextColor(isEditableByUser()));
        if (getStateProperty()->isValueModified()) {
            auto font = pc.painter->font();
            font.setBold(true);
            pc.painter->setFont(font);
        }

        pc.painter->drawText(vp.rect, static_cast<int>(Qt::AlignLeading | Qt::AlignVCenter) | Qt::TextSingleLine,
                              LC_PropertyViewUtils::getElidedText(*pc.painter, getProperty()->getDisplayName(), vp.rect));

        pc.painter->restore();
    };

    parts.append(part);
}

void LC_PropertyViewWithValues::buildPartValues(LC_PropertyPaintContext& ctx, QList<LC_PropertyViewPart>& parts) {
    auto rect = ctx.rect.marginsRemoved(ctx.margins);
    rect.setLeft(ctx.splitPos);
    if (rect.isValid()) {
        doBuildPartValues(ctx, rect, parts);
    }
}
