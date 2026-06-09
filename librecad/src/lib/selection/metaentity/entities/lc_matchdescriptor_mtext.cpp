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


#include "lc_matchdescriptor_mtext.h"

#include "rs_mtext.h"

void LC_MatchDescriptorMText::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
        const auto entity = new LC_TypedEntityMatchDescriptor<RS_MText>(tr("MText"), RS2::EntityMText);
    initCommonEntityAttributesProperties<RS_MText>(entity);

    entity->addVectorX("insertX", [](const RS_MText* e) {
        return e->getInsertionPoint();
    }, tr("Insert X"), tr("X coordinate for text's insertion point"));

    entity->addVectorY("insertY", [](const RS_MText* e) {
        return e->getInsertionPoint();
    }, tr("Insert Y"), tr("Y coordinate for text's insertion point"));

    entity->addAngle("angle", [](const RS_MText* e) {
        return e->getAngle();
    }, tr("Angle"), tr("Text rotation angle"));

    entity->addFontStringList("style", [](const RS_MText* e) -> QString {
        return e->getStyle();
    }, tr("Style"), tr("Name of the text style)"));

    entity->addLength("height", [](const RS_MText* e) {
        return e->getHeight();
    }, tr("Height"), tr("Height of the text"));

    entity->addLength("width", [](const RS_MText* e) {
        return e->getWidth();
    }, tr("Width"), tr("Width of the text"));

    entity->addInt("linesCount", [](const RS_MText* e) {
        return e->getNumberOfLines();
    }, tr("Lines Count"), tr("Number of lines in multiline text"));

    entity->addIntChoice("halign", [](const RS_MText* e) {
        return e->getHAlign();
    }, tr("Horizontal Align"), tr("Horizontal align for text"), {
        {tr("Left"), RS_MTextData::HAlign::HALeft},
        {tr("Center"), RS_MTextData::HAlign::HACenter},
        {tr("Right"),RS_MTextData::HAlign::HARight}
    });

    entity->addIntChoice("valign", [](const RS_MText* e) {
        return e->getVAlign();
    }, tr("Vertical Align"), tr("Vertical align for text"), {
        {tr("Bottom"), RS_MTextData::VAlign::VABottom},
        {tr("Middle"), RS_MTextData::VAlign::VAMiddle},
        {tr("Top"), RS_MTextData::VAlign::VATop}
    });

    entity->addIntChoice("drawDirection", [](const RS_MText* e) {
        return e->getVAlign();
    }, tr("Direction"), tr("Drawing direction for the text"),  {
        {tr("Left to right"), RS_MTextData::MTextDrawingDirection::LeftToRight},
        {tr("Right to left"), RS_MTextData::MTextDrawingDirection::RightToLeft},
        {tr("Top to bottom"), RS_MTextData::MTextDrawingDirection::TopToBottom},
        {tr("By Style"), RS_MTextData::MTextDrawingDirection::ByStyle},
    });

    entity->addIntChoice("lspacingStyle", [](const RS_MText* e) {
        return e->getLineSpacingStyle();
    }, tr("Line spacing style"), tr("Style of linespacing"), {
        {tr("At Least"), RS_MTextData::MTextLineSpacingStyle::AtLeast},
        {tr("Exact"), RS_MTextData::MTextLineSpacingStyle::Exact}
    });

    entity->addDouble("linespacingFactor", [](const RS_MText* e) {
        return e->getLineSpacingFactor();
    }, tr("Linespacing"), tr("Linespacing factor for the text"));

    map.insert(RS2::EntityMText, entity);

}
