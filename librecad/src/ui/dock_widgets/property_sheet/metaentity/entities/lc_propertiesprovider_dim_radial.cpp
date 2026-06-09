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

#include "lc_propertiesprovider_dim_radial.h"

#include "rs_dimradial.h"

void LC_PropertiesProviderDimRadial::doCreateDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addVector<RS_DimRadial>({"dimCenterPoint", tr("Center Point"), tr("Center point of dimension")},
                            [](const RS_DimRadial* e) -> RS_Vector {
                                return e->getCenterPoint();
                            }, [](const RS_Vector& v, RS_DimRadial* e) -> void {
                                e->setCenterPoint(v);
                            }, list, container);

    addVector<RS_DimRadial>({"definitionPoint", tr("Definition Point"), tr("Definition point of dimension")},
                            [](const RS_DimRadial* e) -> RS_Vector {
                                return e->getRadialDefinitionPoint();
                            }, [](const RS_Vector& v, RS_DimRadial* e) -> void {
                                e->setDefinitionPoint(v);
                            }, list, container);

    addLinearDistance<RS_DimRadial>({"leaderLen", tr("Leader Length"), tr("Length of leader")}, [](const RS_DimRadial* e) -> double {
                                        return e->getLeader();
                                    }, [](const double& v, RS_DimRadial* l) -> void {
                                        l->setLeaderLength(v);
                                    }, list, container);
}

void LC_PropertiesProviderDimRadial::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto dim = static_cast<RS_DimRadial*>(entity);
    const std::list<CommandLinkInfo> commandsContextual = {
        {
            tr("Apply dimension style to other dimension"),
            {RS2::ActionDimStyleApply, tr("Apply style"), tr("Applies dimension style to other dimensions")}
        }
    };
    createEntityContextCommands<RS_DimRadial>(commandsContextual, cont, dim, "dimCommandsCtx");
}
