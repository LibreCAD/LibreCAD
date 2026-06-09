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

#include "lc_propertiesprovider_polyline.h"

#include "rs_polyline.h"

void LC_PropertiesProviderPolyline::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addBoolean<RS_Polyline>({"closed", tr("Is Closed"), tr("Determines whether polyline is closed")}, [](const RS_Polyline* e) -> bool {
                                return e->isClosed();
                            }, [](const bool& v, RS_Polyline* e) -> void {
                                e->setClosed(v);
                            }, list, contGeometry);

    addReadOnlyString<RS_Polyline>({"segmentsCount", tr("Segments Count"), tr("Amount of polyline segments")},
                                   [this](const RS_Polyline* e) -> QString {
                                       const int len = e->count();
                                       QString value = formatInt(len);
                                       return value;
                                   }, list, contGeometry);

    const bool singleEntity = list.size() == 1;
    if (singleEntity) {
        // single entity
        auto* polyline = static_cast<RS_Polyline*>(list.first());
        const auto vertexes = polyline->getVertexes();
        const qsizetype vertexesCount = vertexes.count();

        static int polylineVertexIndex = 1;

        if (polylineVertexIndex > vertexesCount) {
            polylineVertexIndex = 1;
        }

        const int initialIndex = polylineVertexIndex;

        createIndexAndPointProperties(contGeometry, polyline, {"vertexIdx", tr("Vertex Index"), tr("Index of current vertex of polyline")},
                                      {"vertexValue", tr("Vertex"), tr("Index of current vertex of polyline")}, vertexesCount, initialIndex,
                                      [polyline](const int newValue) -> RS_Vector {
                                          const RS_Vector wcsVector = polyline->getVertex(newValue - 1);
                                          return wcsVector;
                                      }, [](const int newValue) -> void {
                                          polylineVertexIndex = newValue;
                                      });
    }
}

void LC_PropertiesProviderPolyline::doCreateCalculatedProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addReadOnlyString<RS_Polyline>({"length", tr("Length"), tr("Length of polyline")}, [this](const RS_Polyline* e) -> QString {
        const double len = e->getLength();
        QString value = formatLinear(len);
        return value;
    }, list, container);

    addReadOnlyString<RS_Polyline>({"hasArcs", tr("Has Arcs"), tr("Determines whether polyline includes arc segments")},
                                   [](const RS_Polyline* e) -> QString {
                                       const bool hasArcs = e->containsArc();
                                       QString value = hasArcs ? tr("Yes") : tr("No");
                                       return value;
                                   }, list, container);

    if (list.size() == 1) {
        addVector<RS_Polyline>({"start", tr("Start"), tr("Start point of polyline")}, [](const RS_Polyline* e) -> RS_Vector {
            return e->getStartpoint();
        }, nullptr, list, container);

        addVector<RS_Polyline>({"end", tr("End"), tr("End point of polyline")}, [](const RS_Polyline* line) -> RS_Vector {
            return line->getEndpoint();
        }, nullptr, list, container);
    }
}

void LC_PropertiesProviderPolyline::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto polyline = static_cast<RS_Polyline*>(entity);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Polyline structure expanding"),
            {RS2::ActionPolylineAdd, tr("Add node"), tr("Inserts node of polyline")},
            {RS2::ActionPolylineAppend, tr("Append node"), tr("Append nodes to start or end points of polyline")}
        },
        {
            tr("Polyline nodes removal"),
            {RS2::ActionPolylineDel, tr("Delete node"), tr("Delete node")},
            {RS2::ActionPolylineDelBetween, tr("Delete between 2 nodes"), tr("Delete segments between two nodes")}
        },
        {
            tr("Trimming and exploding polyline"),
            {RS2::ActionPolylineTrim, tr("Trim segments"), tr("Trim segments of polyline")},
            {RS2::ActionPolylineArcsToLines, tr("Arc segments to lines"), tr("Change arc segments to lines")},
        },
        {
            tr("Modification of segments of polyline"),
            {RS2::ActionBlocksExplode, tr("Explode"), tr("Explodes polyline to individual segments")},
            {RS2::ActionPolylineChangeSegmentType, tr("Change segment type"), tr("Changes type of selected segment")}
        },
        {
            tr("Parallel polyline and changing direction"),
            {RS2::ActionPolylineEquidistant, tr("Equidistant"), tr("Creates equidistnat polyline")},
            {RS2::ActionModifyRevertDirection, tr("Revert direction"), tr("Reverst direction of polyline, swapping start and end points")}
        },
        {tr("Creation of bounding box or spline"),
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Creation of bounding box for polyline")},
            // fixme - will be be more conveient if spline from polyline will be called without context?
           {RS2::ActionDrawSplineFromPolyline, tr("Spline"), tr("Creates spline from polyline")}
        }
    };
    createEntityContextCommands<RS_Polyline>(commands, cont, polyline, "polylineCommands");
}
