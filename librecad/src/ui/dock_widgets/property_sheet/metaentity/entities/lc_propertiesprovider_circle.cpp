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

#include "lc_propertiesprovider_circle.h"

#include "rs_circle.h"

void LC_PropertiesProviderCircle::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addVector<RS_Circle>({"сenter", tr("Center"), tr("Center point of circle")}, [](const RS_Circle* e) -> RS_Vector {
                             return e->getCenter();
                         }, [](const RS_Vector& v, RS_Circle* l) -> void {
                             l->setCenter(v);
                         }, list, contGeometry);

    addLinearDistance<RS_Circle>({"radius", tr("Radius"), tr("Radius of circle")}, [](const RS_Circle* e) -> double {
                                     return e->getRadius();
                                 }, [](const double& v, RS_Circle* l) -> void {
                                     l->setRadius(v);
                                 }, list, contGeometry);

    addLinearDistance<RS_Circle>({"diameter", tr("Diameter"), tr("Diameter of circle")}, [](const RS_Circle* e) -> double {
                                     return e->getRadius() * 2.0;
                                 }, [](const double& v, RS_Circle* l) -> void {
                                     l->setRadius(v / 2.0);
                                 }, list, contGeometry);

    addLinearDistance<RS_Circle>({"circumference", tr("Circumference", "circle"), tr("Circumference of circle")},
                                 [](const RS_Circle* e) -> double {
                                     return e->getRadius() * 2.0 * M_PI;
                                 }, [](const double& v, RS_Circle* l) -> void {
                                     l->setRadius(v / 2.0 / M_PI);
                                 }, list, contGeometry);

    addLinearDistance<RS_Circle>({"area", tr("Area"), tr("Area of circle")}, [](const RS_Circle* e) -> double {
                                     const double radius = e->getRadius();
                                     return M_PI * radius * radius;
                                 }, [](const double& v, RS_Circle* l) -> void {
                                     const double radius = std::sqrt(v / M_PI);
                                     l->setRadius(radius);
                                 }, list, contGeometry);
}

void LC_PropertiesProviderCircle::fillComputedProperites([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& entitiesList) {
}

void LC_PropertiesProviderCircle::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto circle = static_cast<RS_Circle*>(entity);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Concentric circles creation"),
            {RS2::ActionModifyOffset, tr("Offset"), tr("Create parallel circle via offset")},
            {RS2::ActionDrawLineParallelThrough, tr("Parallel (Point)"), tr("Create parallel circle by point")}
        },
        {
            tr("Circle trimming or dividing"),
            {RS2::ActionModifyCut, tr("Divide"), tr("Divide circle in specified point")},
            {RS2::ActionModifyTrim, tr("Trim"), tr("Trim circle by other entity")}
        },
        {
            tr("Creation of circle tangental to circle"),
            {RS2::ActionDrawCircleTangental1Entity2Points, tr("Tangent Circle (2 P)"), tr("Create tangental circle 2 points")},
            {RS2::ActionDrawCircleTangental2Entities1Point, tr("Tangent Circle (2 E, 1 P)"), tr("Create tangental circle by 2 entitites and 1 point")}
        },
        {
            tr("Creation of circle tangental"),
            {RS2::ActionDrawCircleTan3Entities, tr("Tangent Circle (3 E)"), tr("Create tangental circle by 3 entities")},
            {RS2::ActionDrawCircleTan2EntitiesRadius, tr("Tangent Circle (2 E, R)"), tr("Create tangental circle by 2 points and radius")}
        },
        {
            tr("Creation of line tangental to circle"),
            {RS2::ActionDrawLineTangent1, tr("Tangent (P,C)"), tr("Create line tangental to circle")},
            {RS2::ActionDrawLineTangent2, tr("Tangent (C,C)"), tr("Create line tangental to two circles")}
        },
        {
            tr("Creation of line tangental to circle"),
            {RS2::ActionDrawLineOrthogonal, tr("Orthogonal"), tr("Create line orthogonal to circle")},
            {RS2::ActionDrawLineOrthTan, tr("Tangent Orthogonal"), tr("Create tangental orthogonal line to line")},
        },
        {
            tr("Dividing circle"),
            {RS2::ActionDrawSliceDivideCircle, tr("Slice/Divide"), tr("Slice or divide a circle")},
            {RS2::ActionModifyBreakDivide, tr("Break/Divide"), tr("Break or divide the circle by intesection points")}
        },
        {
            tr("Creation of line tangental to circle"),
            {RS2::ActionDrawLineAngleRel, tr("Relative"), tr("Create line with relative angle to circle")},
            {RS2::ActionModifyRound, tr("Fillet"), tr("Create fillet for circle")},
        },
        {
            tr("Creation of ellipse or bounding box"),
            {RS2::ActionDrawEllipseInscribe, tr("Ellipse inscribed"), tr("Create elipse inscribed")},
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Create bounding box for circle")}
        },
        {
            tr("Creation of dimension, diametric or radial"),
            {RS2::ActionDimDiametric, tr("Dim Diametric"), tr("Create diametric dimension for circle")},
            {RS2::ActionDimRadial, tr("Dim Radial"), tr("Create radial dimension for circle")}
        },
        {
            tr("Creation of dimension, angular or ordinate"),
            {RS2::ActionDimOrdinate, tr("Dim Ordinate"), tr("Create ordinate dimension for circle")},
            {RS2::ActionDimLeader, tr("Leader"), tr("Create leader for circle")},
        }
    };

    createEntityContextCommands<RS_Circle>(commands, cont, circle, "circleCommands");
}

void LC_PropertiesProviderCircle::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Center marks creation"),
            {RS2::ActionDrawCenterMark, tr("Center mark"), tr("Create center mark for selected circles, ellipses and arcs")}
        }
    };
    createEntityContextCommands<RS_Circle>(commands, propertyContainer, nullptr, "circleMultiCommands", false);
}
