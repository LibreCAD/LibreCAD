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

#include "lc_propertiesprovider_splinepoints.h"

#include "lc_splinepoints.h"

void LC_PropertiesProviderSplinePoints::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addBoolean<LC_SplinePoints>({"closed", tr("Closed"), tr("Determines whether spline is closed or not")},
                                [](const LC_SplinePoints* e) -> bool {
                                    return e->isClosed();
                                }, [](const bool& v, LC_SplinePoints* e) -> void {
                                    e->setClosed(v);
                                }, list, contGeometry);

    addReadOnlyString<LC_SplinePoints>({"numberOfPoints", tr("Points Count"), tr("Amount of spline points")},
                                       [this](const LC_SplinePoints* e) -> QString {
                                           const int len = e->getPoints().size();
                                           QString value = formatInt(len);
                                           return value;
                                       }, list, contGeometry);

    const bool singleEntity = list.size() == 1;
    if (singleEntity) {
        // single entity
        auto spline = static_cast<LC_SplinePoints*>(list.first());
        const bool useControlPoints = spline->getData().useControlPoints;
        const auto points = spline->getPoints();
        const int pointCount = points.size();

        static int splineControlPointIndex = 1;

        if (splineControlPointIndex > pointCount) {
            splineControlPointIndex = 1;
        }

        const int initialIndex = splineControlPointIndex;

        createIndexAndPointProperties(contGeometry, spline,
                                      {
                                          "pointIdx",
                                          useControlPoints ? tr("Control Point Index") : tr("Fit Point Index"),
                                          tr("Index of current point of spline")
                                      }, {"vertexValue", tr("Control Point"), tr("Control point of spline with given index")}, pointCount,
                                      initialIndex, [spline](const int newValue) -> RS_Vector {
                                          const auto splinePoints = spline->getPoints();
                                          const RS_Vector wcsVector = splinePoints.at(newValue - 1);
                                          return wcsVector;
                                      }, [](const int newValue) -> void {
                                          splineControlPointIndex = newValue;
                                      });
    }
}

void LC_PropertiesProviderSplinePoints::doCreateCalculatedProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addReadOnlyString<LC_SplinePoints>({"length", tr("Length"), tr("Length of spline")}, [this](const LC_SplinePoints* e) -> QString {
        const double len = e->getLength();
        QString value = formatLinear(len);
        return value;
    }, list, container);

    if (list.size() == 1) {
        addVector<LC_SplinePoints>({"start", tr("Start"), tr("Start point of spline")}, [](const LC_SplinePoints* e) -> RS_Vector {
            return e->getStartpoint();
        }, nullptr, list, container);

        addVector<LC_SplinePoints>({"end", tr("End"), tr("End point of spline")}, [](const LC_SplinePoints* line) -> RS_Vector {
            return line->getEndpoint();
        }, nullptr, list, container);
    }
}

void LC_PropertiesProviderSplinePoints::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto spline = static_cast<LC_SplinePoints*>(entity);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Spline points adding"),
            {RS2::ActionDrawSplinePointAdd, tr("Insert point"), tr("Insert point of spline")},
            {RS2::ActionDrawSplinePointAppend, tr("Append point"), tr("Append point to start or end points of spline")}
        },
        {
            tr("Spline points removal"),
            {RS2::ActionDrawSplinePointRemove, tr("Remove point"), tr("Remove point of spline")},
            {RS2::ActionDrawSplinePointDelTwo, tr("Delete between 2"), tr("Delete points between two points of spline")}
        },
        {
            tr("Dividing and exploding spline"),
            {RS2::ActionModifyCut, tr("Divide"), tr("Divide spline in provided point")},
            {RS2::ActionDrawSplineExplode, tr("Explode"), tr("Explode spline to lines")},
        },
        {
            tr("Tangent and rounding box"),
            {RS2::ActionDrawLineTangent1, tr("Tangent (P,C)"), tr("Create tangental line in given position of spline")},
            {RS2::ActionDrawBoundingBox, tr("Rounding box"), tr("Create bounding box around spline")}
        }
    };
    createEntityContextCommands<LC_SplinePoints>(commands, cont, spline, "splineCommands");
}

void LC_PropertiesProviderSplinePoints::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);

    const std::list<CommandLinkInfo> commands = {
        {
            tr("Reverting direction"),
            {RS2::ActionModifyRevertDirection, tr("Revert direction"), tr("Change direction of spline by swapping start and end points")}
        }
    };
    createEntityContextCommands<LC_SplinePoints>(commands, propertyContainer, nullptr, "splineMultiCommands", false);
}
