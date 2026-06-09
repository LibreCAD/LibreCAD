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

#include "lc_propertiesprovider_arc.h"

#include "lc_convert.h"
#include "lc_entity_type_propertiesprovider.h"
#include "lc_propertysheetwidget.h"
#include "rs_arc.h"

void LC_PropertiesProviderArc::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addVector<RS_Arc>({"center", tr("Center"), tr("Center point of arc")}, [](const RS_Arc* e) -> RS_Vector {
                          return e->getCenter();
                      }, [](const RS_Vector& v, RS_Arc* e) -> void {
                          e->setCenter(v);
                      }, list, contGeometry);

    addLinearDistance<RS_Arc>({"radius", tr("Radius"), tr("Radius of arc")}, [](const RS_Arc* e) -> double {
                                  return e->getRadius();
                              }, [](const double& v, RS_Arc* l) -> void {
                                  l->setRadius(v);
                              }, list, contGeometry);

    addLinearDistance<RS_Arc>({"diameter", tr("Diameter"), tr("Diameter of arc")}, [](const RS_Arc* e) -> double {
                                  return e->getRadius() * 2.0;
                              }, [](const double& v, RS_Arc* l) -> void {
                                  l->setRadius(v / 2.0);
                              }, list, contGeometry);

    addBoolean<RS_Arc>({"reversed", tr("Is Reversed"), tr("Clockwise direction if reversed, counterclockwise otherwise")},
                       [](const RS_Arc* e) -> bool {
                           return e->isReversed();
                       }, [](const bool& v, RS_Arc* e) -> void {
                           e->setReversed(v);
                       }, list, contGeometry);

    addWCSAngle<RS_Arc>({"angle1", tr("Start Angle"), tr("Start angle of arc")}, [](const RS_Arc* e) -> double {
                            return e->getAngle1();
                        }, [](const double& v, RS_Arc* l) -> void {
                            l->setAngle1(v);
                        }, list, contGeometry);

    addWCSAngle<RS_Arc>({"angle2", tr("End Angle"), tr("End angle of arc")}, [](const RS_Arc* e) -> double {
                            return e->getAngle2();
                        }, [](const double& v, RS_Arc* l) -> void {
                            l->setAngle2(v);
                        }, list, contGeometry);
}

void LC_PropertiesProviderArc::doCreateCalculatedProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addVector<RS_Arc>({"start", tr("Start Point"), tr("Start point of arc")}, [](const RS_Arc* e) -> RS_Vector {
        return e->getStartpoint();
    }, nullptr, list, container);

    addVector<RS_Arc>({"end", tr("End Point"), tr("End point of arc")}, [](const RS_Arc* e) -> RS_Vector {
        return e->getEndpoint();
    }, nullptr, list, container);

    addReadOnlyString<RS_Arc>({"arcLength", tr("Arc Length"), tr("Total angle of arc")}, [this](const RS_Arc* e)-> QString {
        const double angleRad = e->getAngleLength();
        QString value = formatRawAngle(angleRad);
        return value;
    }, list, container);

    addReadOnlyString<RS_Arc>({"circumference", tr("Circumference", "arc"), tr("Total linear length of arc")},
                              [this](const RS_Arc* e)-> QString {
                                  const double len = e->getLength();
                                  QString value = formatLinear(len);
                                  return value;
                              }, list, container);

    addReadOnlyString<RS_Arc>({"chord", tr("Chord Length"), tr("Distance between end points of arc")}, [this](const RS_Arc* e)-> QString {
        const double len = e->getChord();
        QString value = formatLinear(len);
        return value;
    }, list, container);

    addReadOnlyString<RS_Arc>({"sagitta", tr("Sagitta"), tr("Height of arc")}, [this](const RS_Arc* e)-> QString {
        const double len = e->getSagitta();
        QString value = formatLinear(len);
        return value;
    }, list, container);

    addReadOnlyString<RS_Arc>({"bulge", tr("Bulge"), tr("Bulge of arc")}, [this](const RS_Arc* e)-> QString {
        const double len = e->getBulge();
        QString value = formatDouble(len);
        return value;
    }, list, container);
}

void LC_PropertiesProviderArc::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto arc = static_cast<RS_Arc*>(entity);
    auto clickHandler = [this]([[maybe_unused]] RS_Arc* ent, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                m_actionContext->saveContextMenuActionContext(ent, ent->getStartpoint(), false);
                m_actionContext->setCurrentAction(RS2::ActionDrawArcTangential, nullptr);
                break;
            }
            case 1: {
                m_actionContext->saveContextMenuActionContext(ent, ent->getEndpoint(), false);
                m_actionContext->setCurrentAction(RS2::ActionDrawArcTangential, nullptr);
                break;
            }
            default:
                break;
        }
    };
    LC_PropertyProviderUtils::createSingleEntityCommand<RS_Arc>(cont, "arcArcTanental", tr("Tangental Arc (start)"),
                                                                tr("Create tangental arc in start point"), tr("Tangental Arc (end)"),
                                                                tr("Create tangental arc in end point"), arc, clickHandler,
                                                                tr("Create tangental arc"));
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Concentric arcs creation"),
            {RS2::ActionModifyOffset, tr("Offset"), tr("Create parallel arc via offset")},
            {RS2::ActionDrawLineParallelThrough, tr("Parallel (Point)"), tr("Create parallel arc by point")}
        },
        {
            tr("Arc trimming or dividing"),
            {RS2::ActionModifyCut, tr("Divide"), tr("Divide arc in specified point")},
            {RS2::ActionModifyTrim, tr("Trim"), tr("Trim the arc by other entity")}
        },
        {
            tr("Creation of circle tangental to arc"),
            {RS2::ActionDrawCircleTangental1Entity2Points, tr("Tangent Circle (2 P)"), tr("Create tangental circle by 2 points")},
            {RS2::ActionDrawCircleTangental2Entities1Point, tr("Tangent Circle (2 E, 1 P)"), tr("Create tangental circle by 2 entitites and 1 point")}
        },
        {
            tr("Creation of circle tangental"),
            {RS2::ActionDrawCircleTan3Entities, tr("Tangent Circle (3 E)"), tr("Create tangental circle by 3 entities")},
            {RS2::ActionDrawCircleTan2EntitiesRadius, tr("Tangent Circle (2 E, R)"), tr("Create tangental circle by 2 points and radius")}
        },
        {
            tr("Creation of line tangental to circle"),
            {RS2::ActionDrawLineTangent1, tr("Tangent (P,C)"), tr("Create line tangental to arc")},
            {RS2::ActionDrawLineTangent2, tr("Tangent (C,C)"), tr("Create line tangental for two circles/arcs")}
        },
        {
            tr("Creation of line tangental to circle"),
            {RS2::ActionDrawLineOrthogonal, tr("Orthogonal"), tr("Create line orthogonal to arc")},
            {RS2::ActionDrawLineOrthTan, tr("Tangent Orthogonal"), tr("Create arc tangental line orthogonal to line")},
        },
        {
            tr("Creation of line tangental to circle"),
            {RS2::ActionDrawLineRelAngle, tr("Relative angle"), tr("Create line with relative angle to arc")},
            {RS2::ActionModifyRound, tr("Fillet"), tr("Create fillet for arc")},
        },
        {
            tr("Dividing arc"),
            {RS2::ActionDrawSliceDivideCircle, tr("Slice/Divide"), tr("Slice or divide an arc")},
            {RS2::ActionModifyBreakDivide, tr("Break/Divide"), tr("Break or divide the arc by intesection points")}
        },
        {
            tr("Creation of ellipse or bounding box"),
            {RS2::ActionDrawEllipseInscribe, tr("Ellipse inscribed"), tr("Create elipse inscribed")},
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Create bounding box for arc")}
        },
        {
            tr("Creation of dimension, diametric or radial"),
            {RS2::ActionDimDiametric, tr("Dim Diametric"), tr("Create of diametric dimension for arc")},
            {RS2::ActionDimRadial, tr("Dim Radial"), tr("Create of radial dimension for arc")}
        },
        {
            tr("Creation of dimension, angular or ordinate"),
            {RS2::ActionDimOrdinate, tr("Dim Ordinate"), tr("Create of ordinate dimension for arc")},
            {RS2::ActionDimArc, tr("Dim Arc"), tr("Create of arc dimension")},
        }
    };
    createEntityContextCommands<RS_Arc>(commands, cont, arc, "arcCommands2");
}

void LC_PropertiesProviderArc::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Center marks and circle creation"),
            {RS2::ActionDrawCenterMark, tr("Center mark"), tr("Create center mark for selected circles and arcs")},
            {RS2::ActionDrawCircleByArc, tr("Circle by arc"), tr("Create circles by selected arcs")}
        },
        {
            tr("Reverting direction"),
            {RS2::ActionModifyRevertDirection, tr("Revert direction"), tr("Change direction of arcs by swapping start and end points")}
        }
    };
    createEntityContextCommands<RS_Arc>(commands, propertyContainer, nullptr, "arcMultiCommands", false);
}
