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

#ifndef LC_MATCHDESCRIPTORBASE_H
#define LC_MATCHDESCRIPTORBASE_H

#include "lc_entitymatchdescriptor.h"
#include "lc_propertymatchertypes.h"
#include "rs_entity.h"
#include "rs_pen.h"

class LC_MatchDescriptorBase: public QObject {
    Q_OBJECT
protected:
template <typename EntityType>
static void initCommonEntityAttributesProperties(LC_TypedEntityMatchDescriptor<EntityType>* entity) {
    entity->template add<RS_Layer*>("layer", [](const RS_Entity* e) {
        return e->getLayer();
    }, tr("Layer"), tr("Layer of the entity"), LC_PropertyMatcherTypes::LAYER);

    entity->template add<RS_Color>("color", [](const RS_Entity* e) {
        const RS_Pen pen = e->getPen(false);
        RS_Color color = pen.getColor();
        return color;
    }, tr("Color"), tr("Color attribute as it is stored in the entity"), LC_PropertyMatcherTypes::COLOR);

    entity->template add<RS_Color>("colorR", [](const RS_Entity* e) {
        const RS_Pen pen = e->getPen(true);
        RS_Color color = pen.getColor();
        return color;
    }, tr("Color (Resolved)"), tr("Resolved color that is used when entity is drawn"), LC_PropertyMatcherTypes::COLOR_RESOLVED);

    entity->template add<RS2::LineWidth>("lineWidth", [](const RS_Entity* e) {
        const RS_Pen pen = e->getPen(false);
        RS2::LineWidth linewidth = pen.getWidth();
        return linewidth;
    }, tr("Line Width"), tr("Width of line attribute that is stored in the entity"), LC_PropertyMatcherTypes::LINE_WIDTH);

    entity->template add<RS2::LineWidth>("lineWidthR", [](const RS_Entity* e) {
        const RS_Pen pen = e->getPen(true);
        RS2::LineWidth linewidth = pen.getWidth();
        return linewidth;
    }, tr("Line Width (Resolved)"), tr("Resolved width of line that is used when entity is drawn"), LC_PropertyMatcherTypes::LINE_WIDTH_RESOLVED);

    entity->template add<RS2::LineType>("lineType", [](const RS_Entity* e) {
        const RS_Pen pen = e->getPen(false);
        RS2::LineType lineType = pen.getLineType();
        return lineType;
    }, tr("Line Type"), tr("Type of line attribute stored in the entity"), LC_PropertyMatcherTypes::LINE_TYPE);

    entity->template add<RS2::LineType>("lineTypeR", [](const RS_Entity* e) {
        const RS_Pen pen = e->getPen(true);
        RS2::LineType lineType = pen.getLineType();
        return lineType;
    }, tr("Line Type (Resolved)"), tr("Resolved type of line that is used when entity is drawn"), LC_PropertyMatcherTypes::LINE_TYPE_RESOLVED);
}
};

#endif
