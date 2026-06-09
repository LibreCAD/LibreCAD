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

#include "lc_propertiesprovider_spline.h"

#include "rs_spline.h"

void LC_PropertiesProviderSpline::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addBoolean<RS_Spline>({"closed", tr("Closed"), tr("Determines whether spline is closed or not")},
                                [](const RS_Spline* e) -> bool {
                                    return e->isClosed();
                                }, [](const bool& v, RS_Spline* e) -> void {
                                    e->setClosed(v);
                                }, list, contGeometry);

    addIntSpinbox<RS_Spline>({"degree", tr("Degree"), tr("Degree of spline")}, [](const RS_Spline* e) -> int {
                              const int value = e->getDegree();
                              return value;
                          }, [](const int& value, RS_Spline* e) -> void {
                              int val = value;
                              if (value < 1) {
                                  val = 1;
                              }
                              else if (value > 3) {
                                  val = 3;
                              }
                              e->setDegree(val);
                          }, list, contGeometry, 1, 3);

    addReadOnlyString<RS_Spline>({"numberOfPoints", tr("Points Count"), tr("Amount of control points")},
                                       [this](const RS_Spline* e) -> QString {
                                           const int len = e->getNumberOfControlPoints();
                                           QString value = formatInt(len);
                                           return value;
                                       }, list, contGeometry);

    const bool singleEntity = list.size() == 1;
    if (singleEntity) {
        // single entity
        auto spline = static_cast<RS_Spline*>(list.first());
        const int pointCount = spline->getNumberOfControlPoints();

        static int splineControlPointIndex = 1;

        if (splineControlPointIndex > pointCount) {
            splineControlPointIndex = 1;
        }

        const int initialIndex = splineControlPointIndex;

        auto funPersistIndexValue = [](const int newValue) -> void {
            splineControlPointIndex = newValue;
        };

        auto funGetPointByIndex = [spline](const int newValue) -> RS_Vector {
            const auto splinePoints = spline->getControlPoints();
            const RS_Vector wcsVector = splinePoints.at(newValue - 1);
            return wcsVector;
        };

        LC_PropertyInt* propertyPointIndex = createIndexProperty(container, {
                                                                                "pointIdx",
                                                                                tr("Control Point Index"),
                                                                                tr("Index of current point of spline")
                                                                            }, pointCount, funGetPointByIndex, funPersistIndexValue);

        createIndexedPointProperty<RS_Spline>(container, spline,
                                              {"vertexValue", tr("Control Point"), tr("Control point of spline with given index")},
                                              funGetPointByIndex, propertyPointIndex);

        auto funGetWeightByIndex = [spline](const int idx) -> double {
            const auto result = spline->getWeight(idx);
            return result;
        };
        // fixme - sand - review implementation as RS_Spline itself will be finalized!!
        // fixme - sand - so far, it seems that RS_Spline is seriously broken, and doesn't properly reflect to changes
        auto funSetWeightByIndex = [spline](const int idx, const double& weight) -> void {
            spline->setWeight(idx, weight);
        };

        createIndexedDoubleProperty(container, spline, {"vertexWeight", tr("Weight"), tr("Weight of spline control point")},
                                     funGetWeightByIndex,
                                     funSetWeightByIndex,
                                     propertyPointIndex);

        constexpr LC_PropertyChangeReason reason(PropertyChangeReasonValueLoaded);
        propertyPointIndex->setValue(initialIndex, reason);
    }
}

void LC_PropertiesProviderSpline::doCreateCalculatedProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addReadOnlyString<RS_Spline>({"length", tr("Length"), tr("Length of spline")}, [this](const RS_Spline* e) -> QString {
          const double len = e->getLength();
          QString value = formatLinear(len);
          return value;
      }, list, container);

    if (list.size() == 1) {
        addVector<RS_Spline>({"start", tr("Start"), tr("Start point of spline")}, [](const RS_Spline* e) -> RS_Vector {
            return e->getStartpoint();
        }, nullptr, list, container);

        addVector<RS_Spline>({"end", tr("End"), tr("End point of spline")}, [](const RS_Spline* line) -> RS_Vector {
            return line->getEndpoint();
        }, nullptr, list, container);
    }
}

void LC_PropertiesProviderSpline::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto spline = static_cast<RS_Spline*>(entity);
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
            tr("Exploding spline and creating rounding box"),
            {RS2::ActionDrawSplineExplode, tr("Explode"), tr("Explode spline to lines")},
            {RS2::ActionDrawBoundingBox, tr("Rounding box"), tr("Create bounding box around spline")}
        }
    };
    createEntityContextCommands<RS_Spline>(commands, cont, spline, "splineCommands");
}

void LC_PropertiesProviderSpline::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);

    const std::list<CommandLinkInfo> commands = {
        {
            tr("Reverting direction"),
            {RS2::ActionModifyRevertDirection, tr("Revert direction"), tr("Changes direction of spline by swapping start and end points")}
        }
    };
    createEntityContextCommands<RS_Spline>(commands, propertyContainer, nullptr, "splineMultiCommands", false);
}
