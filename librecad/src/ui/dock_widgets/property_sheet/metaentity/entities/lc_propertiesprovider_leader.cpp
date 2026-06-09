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

#include "lc_propertiesprovider_leader.h"

#include "rs_leader.h"

void LC_PropertiesProviderLeader::doCreateEntitySpecificProperties([[maybe_unused]]LC_PropertyContainer* container,[[maybe_unused]] const QList<RS_Entity*>& list) {
     const auto contGeometry = createGeometrySection(container);

     addBoolean<RS_Leader>({"hasArrowhead", tr("Has arrowhead"), tr("Determines whether leader has arrowhead or not")},
                                  [](const RS_Leader* e) -> bool {
                                      return e->hasArrowHead();
                                  }, [](const bool& v, RS_Leader* e) -> void {
                                      e->setHasArrwoHead(v);
                                  }, list, contGeometry);

    addReadOnlyString<RS_Leader>({"numberOfPoints", tr("Vertexes count"), tr("Amount of vertexes")},
                                     [this](const RS_Leader* e) -> QString {
                                         const int len = e->getVertexesCount();
                                         QString value = formatInt(len);
                                         return value;
                                     }, list, contGeometry);

    const bool singleEntity = list.size() == 1;
    if (singleEntity) {
        auto leader = static_cast<RS_Leader*>(list.first());
        const auto vertexes = leader->getData().vertexes;
        const int vertexesCount = vertexes.size();

        static int vertexIndex = 1;

        if (vertexIndex > vertexesCount) {
            vertexIndex = 1;
        }

        const int initialIndex = vertexIndex;

        createIndexAndPointProperties(contGeometry, leader,
                                      {
                                          "pointIdx",
                                          tr("Vertex index") ,
                                          tr("Index of current vertex of leader")
                                      }, {"vertexValue", tr("Vertex"), tr("Vertext of leader with given index")}, vertexesCount,
                                      initialIndex, [leader](const int newValue) -> RS_Vector {
                                          const RS_Vector wcsVector = leader->getVertexAt(newValue - 1);
                                          return wcsVector;
                                      }, [](const int newValue) -> void {
                                          vertexIndex = newValue;
                                      });
    }

    // fixme - sand - add more properties as leader entity will be finalized
}

void LC_PropertiesProviderLeader::fillComputedProperites([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& entitiesList) {
}

void LC_PropertiesProviderLeader::fillSingleEntityCommands([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& entitiesList) {
    // todo - add copy style? Regenerate dims commands?
}
