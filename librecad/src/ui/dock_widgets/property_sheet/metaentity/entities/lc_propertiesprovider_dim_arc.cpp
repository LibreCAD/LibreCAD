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

#include "lc_propertiesprovider_dim_arc.h"

#include "lc_dimarc.h"

void LC_PropertiesProviderDimArc::doCreateDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addVector<LC_DimArc>({"dimCenter", tr("Center point"), tr("Center point of arc dimension")}, [](const LC_DimArc* e) -> RS_Vector {
                             return e->getCenter();
                         }, [](const RS_Vector& v, LC_DimArc* e) -> void {
                             e->setCenter(v);
                         }, list, container);

    // fixme - add more properties as DimArc will be finalized !!!
}

void LC_PropertiesProviderDimArc::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto dim = static_cast<LC_DimArc*>(entity);
    const std::list<CommandLinkInfo> commandsContextual = {
        {
            tr("Apply dimension style to other dimension"),
            {RS2::ActionDimStyleApply, tr("Apply style"), tr("Applies dimension style to other dimensions")}
        }
    };
    createEntityContextCommands<LC_DimArc>(commandsContextual, cont, dim, "dimCommandsCtx");
}
