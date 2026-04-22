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

#include "lc_propertiesprovider_parabola.h"

#include "lc_parabola.h"
#include "lc_property_container.h"
#include "rs_entity.h"

void LC_PropertiesProviderParabola::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);
    // fixme - sand - complete editing of parabola
    addVector<LC_Parabola>({"focus", tr("Focus"), tr("Focus of parabola")}, [](LC_Parabola* e) -> RS_Vector {
                               return e->getFocus();
                           }, nullptr, /*[](RS_Vector& v, LC_Parabola* l) -> void {
                 l->setFocus(v);
             }, */list, contGeometry);

    addVector<LC_Parabola>({"vertex", tr("Vertex"), tr("Vertex of parabola")}, [](LC_Parabola* e) -> RS_Vector {
                               return e->getVertex();
                           }, nullptr, /*[](RS_Vector& v, LC_Parabola* l) -> void {
                  l->setEndpoint(v);
              }, */list, contGeometry);

    addWCSAngle<LC_Parabola>({"angle", tr("Angle"), tr("Angle of parabola axis")}, [](LC_Parabola* e) -> double {
                                 return e->getData().m_axis.angle();
                             }, nullptr, /*[](double& v, LC_Parabola* l) -> void {
                                 l->setAngle1(v);
                             }, */list, contGeometry);

    addReadOnlyString<LC_Parabola>({"length", tr("Length"), tr("Length of parabola")}, [this](const LC_Parabola* e) -> QString {
        const double len = e->getLength();
        QString value = formatLinear(len);
        return value;
    }, list, contGeometry);
}

void LC_PropertiesProviderParabola::fillComputedProperites([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& entitiesList) {
}

void LC_PropertiesProviderParabola::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto ellipse = static_cast<LC_Parabola*>(entity);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Dividing parabola or creation of bounding box"),
            {RS2::ActionModifyCut, tr("Divide"), tr("Divide parabola in given point")},
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Creation of bounding box for parabola")}
        },
    };

    createEntityContextCommands<LC_Parabola>(commands, cont, ellipse, "parCommands");
}
