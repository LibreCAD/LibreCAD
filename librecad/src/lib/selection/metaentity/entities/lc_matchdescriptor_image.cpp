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

#include "lc_matchdescriptor_image.h"

#include "rs_image.h"
#include "rs_units.h"

void LC_MatchDescriptorImage::init(QMap<RS2::EntityType, LC_EntityMatchDescriptor*>& map) {
        const auto entity = new LC_TypedEntityMatchDescriptor<RS_Image>(tr("Image"), RS2::EntityImage);
    initCommonEntityAttributesProperties<RS_Image>(entity);

    entity->addString("file", [](const RS_Image* e) {
        return e->getFile();
    }, tr("File"), tr("Name of the image file"));

    entity->addVectorX("insertX", [](const RS_Image* e) {
        return e->getInsertionPoint();
    }, tr("Insert X"), tr("X coordinate for image insertion point"));

    entity->addVectorY("insertY", [](const RS_Image* e) {
        return e->getInsertionPoint();
    }, tr("Insert Y"), tr("Y coordinate for image insertion point"));

    entity->addDouble("scale", [](const RS_Image* e) {
        return e->getUVector().magnitude();
    }, tr("Scale"), tr("Scale factor for image"));

    entity->addAngle("angle", [](const RS_Image* e) {
        return e->getUVector().angle();
    }, tr("Angle"), tr("Image rotation angle"));

    entity->addDouble("sizeX", [](const RS_Image* e) {
        return e->getData().size.getX();
    }, tr("Width pixels"), tr("Width of image in pixels"));

    entity->addDouble("sizeY", [](const RS_Image* e) {
        return e->getData().size.getY();
    }, tr("Height pixels"), tr("Height of image in pixels"));

    entity->addLength("width", [](const RS_Image* e) {
        return e->getImageWidth();
    }, tr("Width"), tr("Width of image"));

    entity->addLength("height", [](const RS_Image* e) {
        return e->getImageHeight();
    }, tr("Height"), tr("Height of image"));

    entity->addDouble("dpi", [](const RS_Image* e) {
        const double scale = e->getUVector().magnitude();
        return RS_Units::scaleToDpi(scale, e->getGraphicUnit());
    }, tr("DPI"), tr("Dots per inch for the image"));

    map.insert(RS2::EntityImage, entity);

}
