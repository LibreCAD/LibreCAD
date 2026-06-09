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

#include "lc_propertiesprovider_dim_angular.h"

#include "rs_dimangular.h"

void LC_PropertiesProviderDimAngular::doCreateDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addVector<RS_DimAngular>({"dimDefPoint1", tr("Line 1 start point"), tr("Start point of first line")},
                             [](const RS_DimAngular* e) -> RS_Vector {
                                 return e->getDefinitionPoint1();
                             }, [](const RS_Vector& v, RS_DimAngular* e) -> void {
                                 e->setDefinitionPoint1(v);
                             }, list, container);

    addVector<RS_DimAngular>({"dimDefPoint2", tr("Line 1 end point"), tr("End point of first line")},
                             [](const RS_DimAngular* e) -> RS_Vector {
                                 return e->getDefinitionPoint2();
                             }, [](const RS_Vector& v, RS_DimAngular* e) -> void {
                                 e->setDefinitionPoint2(v);
                             }, list, container);

    addVector<RS_DimAngular>({"dimDefPoint3", tr("Line 2 start point"), tr("Start point of second line")},
                             [](const RS_DimAngular* e) -> RS_Vector {
                                 return e->getDefinitionPoint3();
                             }, [](const RS_Vector& v, RS_DimAngular* e) -> void {
                                 e->setDefinitionPoint3(v);
                             }, list, container);

    addVector<RS_DimAngular>({"dimDefPoint", tr("Line 2 end point"), tr("End point of second line")},
                             [](const RS_DimAngular* e) -> RS_Vector {
                                 return e->getDefinitionPoint();
                             }, [](const RS_Vector& v, RS_DimAngular* e) -> void {
                                 e->setDefinitionPoint(v);
                             }, list, container);

    addVector<RS_DimAngular>({"dimDefPoint4", tr("Dim arc radius point"), tr("Point that defines dimension arc")},
                             [](const RS_DimAngular* e) -> RS_Vector {
                                 return e->getDefinitionPoint4();
                             }, [](const RS_Vector& v, RS_DimAngular* e) -> void {
                                 e->setDefinitionPoint4(v);
                             }, list, container);
}

void LC_PropertiesProviderDimAngular::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto dim = static_cast<RS_DimAngular*>(entity);
    const std::list<CommandLinkInfo> commandsContextual = {
        {
            tr("Apply dimension style to other dimension"),
            {RS2::ActionDimStyleApply, tr("Apply style"), tr("Applies dimension style to other dimensions")}
        }
    };
    createEntityContextCommands<RS_DimAngular>(commandsContextual, cont, dim, "dimCommandsCtx");
}
