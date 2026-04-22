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

#include "lc_propertiesprovider_line.h"

#include "lc_convert.h"
#include "lc_property_action.h"
#include "lc_propertyprovider_utils.h"
#include "rs_line.h"


void LC_PropertiesProviderLine::doCreateEntitySpecificProperties(LC_PropertyContainer* cont, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(cont);

    addVector<RS_Line>({"start", tr("Start"), tr("Start point of line")}, [](const RS_Line* e) -> RS_Vector {
                           return e->getStartpoint();
                       }, [](const RS_Vector& v, RS_Line* l) -> void {
                           l->setStartpoint(v);
                       }, list, contGeometry);

    addVector<RS_Line>({"end", tr("End"), tr("End point of line")}, [](const RS_Line* line) -> RS_Vector {
                           return line->getEndpoint();
                       }, [](const RS_Vector& v, RS_Line* l) -> void {
                           l->setEndpoint(v);
                       }, list, contGeometry);
}

void LC_PropertiesProviderLine::doCreateCalculatedProperties(LC_PropertyContainer* cont, const QList<RS_Entity*>& list) {
    addReadOnlyString<RS_Line>({"length", tr("Length"), tr("Length of line")}, [this](const RS_Line* e) -> QString {
        const double len = e->getLength();
        QString value = formatLinear(len);
        return value;
    }, list, cont);

    addReadOnlyString<RS_Line>({"angle1", tr("Angle 1"), tr("Angle from 0.0 to first point of line")}, [this](const RS_Line* e) -> QString {
        const double wcsAngleRad = e->getAngle1();
        QString value = formatWCSAngleDegrees(wcsAngleRad);
        return value;
    }, list, cont);

    addReadOnlyString<RS_Line>({"angle2", tr("Angle 2"), tr("Angle from 0.0 to second point of line")},
                               [this](const RS_Line* e) -> QString {
                                   const double wcsAngleRad = e->getAngle2();
                                   QString value = formatWCSAngleDegrees(wcsAngleRad);
                                   return value;
                               }, list, cont);

    // addReadOnlyString<RS_Line>({"inclination", tr("Inclination"), tr("Angle of the line inclination to x-axis")},
    //                            [this](const RS_Line* e) -> QString {
    //                                const double wcsAngleRad = RS_Math::correctAngle0ToPi(e->getAngle1());
    //                                QString value = formatWCSAngleDegrees(wcsAngleRad);
    //                                return value;
    //                            }, list, container);

    const auto graphicViewport = m_actionContext->getViewport();
    if (graphicViewport != nullptr) {
        addVector<RS_Line>({"delta", tr("Delta"), tr("Distance between start and end point")}, [this](const RS_Line* line) -> RS_Vector {
            const auto deltaVectorWCS = line->getEndpoint() - line->getStartpoint();
            const auto viewport = m_actionContext->getViewport();
            if (viewport == nullptr) {
                return deltaVectorWCS;
            }
            const auto ucsDelta = viewport->toUCSDelta(deltaVectorWCS);
            return ucsDelta;
        }, nullptr, list, cont);
    }

    addVector<RS_Line>({"middle", tr("Middle Point"), tr("Middle point of line")}, [](const RS_Line* e) -> RS_Vector {
        return e->getMiddlePoint();
    }, nullptr, list, cont);
}

void LC_PropertiesProviderLine::doCreateSingleEntityCommands(LC_PropertyContainer* container, RS_Entity* ent) {
    const auto line = static_cast<RS_Line*>(ent);

    const std::list<CommandLinkInfo> commands1 = {
        {
            tr("Length and lines join"),
            {RS2::ActionModifyTrimAmount, tr("Lengthen"), tr("Create parallel line through point")},
            {RS2::ActionModifyLineJoin, tr("Line Join"), tr("Join two lines")}
        },
        {
            tr("Trimming two lines"),
            {RS2::ActionModifyTrim, tr("Trim"), tr("Trim line by limiting entity")},
            {RS2::ActionModifyTrim2, tr("Trim Two"), tr("Trim two lines")}
        },
        {
            tr("Cutting and gap"),
            {RS2::ActionModifyCut, tr("Divide"), tr("Divide line")},
            {RS2::ActionModifyLineGap, tr("Line Gap"), tr("Break the line by gap")}
        }
    };

    createEntityContextCommands<RS_Line>(commands1, container, line, "lineCommands1");

    auto clickHandler = [this]([[maybe_unused]] RS_Line* entity, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                m_actionContext->saveContextMenuActionContext(entity, entity->getStartpoint(), false);
                m_actionContext->setCurrentAction(RS2::ActionDrawArcTangential, nullptr);
                break;
            }
            case 1: {
                m_actionContext->saveContextMenuActionContext(entity, entity->getEndpoint(), false);
                m_actionContext->setCurrentAction(RS2::ActionDrawArcTangential, nullptr);
                break;
            }
            default:
                break;
        }
    };
    LC_PropertyProviderUtils::createSingleEntityCommand<RS_Line>(container, "lineActTanStart", tr("Tangental Arc (start)"),
                                                                   tr("Create tangental arc in start point"),
                                                                   tr("Tangental Arc (end)"), tr("Create tangental arc in end point"),line,
                                                                   clickHandler, tr("Creation of tangental arc"));

    const std::list<CommandLinkInfo> commands2 = {
        {
            tr("Creation of parallels or bisector lines"),
            {RS2::ActionDrawLineParallelThrough, tr("Parallel"), tr("Create parallel line through point")},
            {RS2::ActionDrawLineBisector, tr("Bisector"), tr("Create bisector between lines")}
        },
        {
            tr("Creating orthogonal or relative angle lines"),
            {RS2::ActionDrawLineOrthogonal, tr("Orthogonal"), tr("Create line orhtogonal to this line")},
            {RS2::ActionDrawLineRelAngle, tr("Relative Angle"), tr("Create line with related angle to this line")}
        },
        {
            tr("Creation line from point or tangental orthogonal"),
            {RS2::ActionDrawLineFromPointToLine, tr("Line from Point"), tr("Create line from point to this line")},
            {RS2::ActionDrawLineOrthTan, tr("Tangent Ort"), tr("Create orthogonal line that is tangental to other entity")}
        },
        {
            tr("Creation of circle tangental to line"),
            {RS2::ActionDrawCircleTangental1Entity2Points, tr("Tangential Circle (2 P)"), tr("Create circle tangental 2 points")},
            {RS2::ActionDrawCircleTangental2Entities1Point, tr("Tangential Circle (2 E, 1 P)"), tr("Create circle tangental by 2 entitites and 1 point")}
        },
        {
            tr("Creation of circle tangental to line"),
            {RS2::ActionDrawCircleTan3Entities, tr("Tangential Circle (3 E)"), tr("Create circle tangental to 3 entities")},
            {RS2::ActionDrawCircleTan2EntitiesRadius, tr("Tangential Cicle (2 E, R)"), tr("Create circle tangental by 2 entities and radius")}
        },
        {
            tr("Creation of ellipse or bounding box"),
            {RS2::ActionDrawEllipseInscribe, tr("Ellipse inscribed"), tr("Create elipse inscribed")},
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Create of bounding box for line")}
        },
        {
            tr("Modification of line with bevel or fillet"),
            {RS2::ActionModifyBevel, tr("Bevel"), tr("Create a bevel")},
            {RS2::ActionModifyRound, tr("Round"), tr("Create rounding between line and other entity")}
        },
        {
            tr("Dividing line"),
            {RS2::ActionDrawSliceDivideLine, tr("Slice/Divide"), tr("Slice or divide a line")},
            {RS2::ActionModifyBreakDivide, tr("Break/Divide"), tr("Break or divide the line by intesection points")}
        },
        {tr("Center line"),
            {RS2::ActionDrawCenterLine, tr("Centerline"), tr("Create center line between two lines")},
            {RS2::ActionDrawLineRadiant, tr("Radiant"), tr("Create radiant line from center point")},
        },
        {
            tr("Creation of dimension, aligned or linear"),
            {RS2::ActionDimAligned, tr("Dim Aligned"), tr("Create aligned dimension for line")},
            {RS2::ActionDimLinear, tr("Dim Linear"), tr("Create linear dimension for line")}
        },
        {
            tr("Creation of dimension, angular or ordinate"),
            {RS2::ActionDimAngular, tr("Dim Angular"), tr("Create angular dimension for line")},
            {RS2::ActionDimOrdinate, tr("Dim Ordinate"), tr("Create ordinate dimension for line")}
        }
    };

    createEntityContextCommands<RS_Line>(commands2, container, line, "lineCommands2");
}

void LC_PropertiesProviderLine::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);

    const std::list<CommandLinkInfo> commands = {
        {
            tr("Reverting direction"),
            {RS2::ActionModifyRevertDirection, tr("Revert direction"), tr("Change direction of line by swapping start and end points")}
        }
    };
    createEntityContextCommands<RS_Line>(commands, propertyContainer, nullptr, "arcMultiCommands", false);
}
