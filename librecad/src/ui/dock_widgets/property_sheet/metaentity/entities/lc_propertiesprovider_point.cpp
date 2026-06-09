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

#include "lc_propertiesprovider_point.h"

#include "rs_point.h"

void LC_PropertiesProviderPoint::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addVector<RS_Point>({"pos", tr("Position"), tr("Position of point")}, [](const RS_Point* e) -> RS_Vector {
                            return e->getPos();
                        }, [](const RS_Vector& v, RS_Point* e) -> void {
                            e->setPos(v);
                        }, list, contGeometry);
}

void LC_PropertiesProviderPoint::fillComputedProperites([[maybe_unused]] LC_PropertyContainer* container,
                                                        [[maybe_unused]] const QList<RS_Entity*>& entitiesList) {
}

void LC_PropertiesProviderPoint::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto text = static_cast<RS_Point*>(entity);

    const std::list<CommandLinkInfo> commands = {
        {
            tr("Selection operations"),
            {RS2::ActionSelectPoints, tr("Select points"), tr("Selecting points within selected area")},
            {RS2::ActionDrawPointsMiddle, tr("Middle points"), tr("Draw points in the middle of line between two points")}
        },
        {
            tr("Other point operations"),
            {RS2::ActionDrawPointsLine, tr("Line of points"), tr("Creation of several points along specified direction")},
            {RS2::ActionDrawPointsLattice, tr("Lattice of points"), tr("Creation of lattice of points")}
        }
    };

    createEntityContextCommands<RS_Point>(commands, cont, text, "pointCommands", false);
}

void LC_PropertiesProviderPoint::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);

    const std::list<CommandLinkInfo> commands = {
        {
            tr("Point operations"),
            {
                RS2::ActionPasteToPoints,
                tr("Paste to points"),
                tr("Perform paste from clipboard, inserting copied content into positions of selected points")
            }
        }
    };

    createEntityContextCommands<RS_Point>(commands, propertyContainer, nullptr, "pointCommands", false);
}
