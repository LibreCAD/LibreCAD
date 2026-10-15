/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2026 LibreCAD.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 ******************************************************************************/

#ifndef LC_TESTENTITYFACTORY_H
#define LC_TESTENTITYFACTORY_H

#include <array>
#include <cmath>
#include <memory>
#include <utility>

#include "lc_dimarc.h"
#include "lc_dimordinate.h"
#include "lc_hyperbola.h"
#include "lc_mleader.h"
#include "lc_parabola.h"
#include "lc_splinepoints.h"
#include "lc_tolerance.h"
#include "lc_wipeout.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_polyline.h"
#include "rs_solid.h"
#include "rs_spline.h"
#include "rs_text.h"

namespace lc::test {

/**
 * One updated entity, parented to owner but not added to it. The caller must
 * initialize settings, keep SourceFonts alive and initialize the graphic as
 * a new document. An INSERT also installs its one-line inner block.
 */
inline std::unique_ptr<RS_Entity> makeDrawingEntity(RS2::EntityType type, RS_Graphic& graphic,
                                                  RS_EntityContainer* owner) {
    std::unique_ptr<RS_Entity> entity;
    RS_DimensionData dimensionData({0, 6}, {5, 8}, RS_MTextData::VABottom, RS_MTextData::HACenter,
                                   RS_MTextData::Exact, 1.0, QStringLiteral("<>"), QStringLiteral("Standard"),
                                   0.0, 0.0, true, nullptr, false, false);
    switch (type) {
    case RS2::EntityInsert: {
        const QString innerName = QStringLiteral("LC_ENTITY_COVERAGE_INNER");
        if (graphic.findBlock(innerName) == nullptr) {
            auto inner = std::make_unique<RS_Block>(&graphic, RS_BlockData(innerName, {0, 0}, false));
            inner->addEntity(new RS_Line(inner.get(), RS_LineData({0, 0}, {5, 2})));
            if (!graphic.addBlock(inner.release()))
                return nullptr;
        }
        entity = std::make_unique<RS_Insert>(owner, RS_InsertData(innerName, {2, 1}, {1, 1}, 0.0, 1, 1, {0, 0}));
        break;
    }
    case RS2::EntityPoint:
        entity = std::make_unique<RS_Point>(owner, RS_Vector{2, 3});
        break;
    case RS2::EntityLine:
        entity = std::make_unique<RS_Line>(owner, RS_LineData({0, 0}, {10, 4}));
        break;
    case RS2::EntityPolyline: {
        auto polyline = std::make_unique<RS_Polyline>(owner);
        polyline->addVertex({0, 0});
        polyline->addVertex({10, 0});
        polyline->addVertex({10, 10});
        polyline->setClosed(true);
        polyline->endPolyline();
        entity = std::move(polyline);
        break;
    }
    case RS2::EntityArc:
        entity = std::make_unique<RS_Arc>(owner, RS_ArcData({0, 0}, 5.0, 0.0, M_PI_2, false));
        break;
    case RS2::EntityCircle:
        entity = std::make_unique<RS_Circle>(owner, RS_CircleData({0, 0}, 5.0));
        break;
    case RS2::EntityEllipse: {
        RS_EllipseData data;
        data.center = {0, 0};
        data.majorP = {10, 0};
        data.ratio = 0.5;
        entity = std::make_unique<RS_Ellipse>(owner, data);
        break;
    }
    case RS2::EntityHyperbola:
        entity = std::make_unique<LC_Hyperbola>(owner, LC_HyperbolaData({0, 0}, {5, 0}, 0.5, -0.6, 0.6));
        break;
    case RS2::EntitySolid:
        entity = std::make_unique<RS_Solid>(owner, RS_SolidData({0, 0}, {10, 0}, {0, 5}));
        break;
    case RS2::EntityMText:
        entity = std::make_unique<RS_MText>(owner, RS_MTextData({0, 0}, 2.5, 40.0, RS_MTextData::VATop,
            RS_MTextData::HALeft, RS_MTextData::LeftToRight, RS_MTextData::Exact, 1.0,
            QStringLiteral("Coverage\\Ptext"), QStringLiteral("standard"), 0.0));
        break;
    case RS2::EntityText:
        entity = std::make_unique<RS_Text>(owner, RS_TextData({0, 0}, {0, 0}, 2.5, 1.0, RS_TextData::VABaseline,
            RS_TextData::HALeft, RS_TextData::None, QStringLiteral("Coverage"), QStringLiteral("standard"),
            0.0, RS2::Update));
        break;
    case RS2::EntityDimAligned:
        entity = std::make_unique<RS_DimAligned>(owner, dimensionData, RS_DimAlignedData({0, 0}, {10, 0}));
        break;
    case RS2::EntityDimLinear:
        entity = std::make_unique<RS_DimLinear>(owner, dimensionData, RS_DimLinearData({0, 0}, {10, 0}, 0.0, 0.0));
        break;
    case RS2::EntityDimRadial:
        dimensionData.definitionPoint = {0, 0};
        entity = std::make_unique<RS_DimRadial>(owner, dimensionData, RS_DimRadialData({10, 0}, 0.0));
        break;
    case RS2::EntityDimDiametric:
        dimensionData.definitionPoint = {-10, 0};
        entity = std::make_unique<RS_DimDiametric>(owner, dimensionData, RS_DimDiametricData({10, 0}, 0.0));
        break;
    case RS2::EntityDimAngular:
        dimensionData.definitionPoint = {0, 10};
        entity = std::make_unique<RS_DimAngular>(owner, dimensionData,
            RS_DimAngularData({0, 0}, {10, 0}, {0, 0}, {6, 6}));
        break;
    case RS2::EntityDimArc:
        dimensionData.definitionPoint = {0, 10};
        entity = std::make_unique<LC_DimArc>(owner, dimensionData,
            LC_DimArcData(10.0, 5.0 * M_PI, {0, 0}, {0, 1}, {1, 0}));
        break;
    case RS2::EntityDimOrdinate:
        dimensionData.definitionPoint = {0, 0};
        entity = std::make_unique<LC_DimOrdinate>(owner, dimensionData, LC_DimOrdinateData({4, 3}, {4, 9}, true));
        break;
    case RS2::EntityTolerance:
        entity = std::make_unique<LC_Tolerance>(owner, LC_ToleranceData({0, 0}, {1, 0},
            QStringLiteral("%%v0.1%%vA"), QStringLiteral("Standard")));
        break;
    case RS2::EntityDimLeader: {
        RS_LeaderData data(true, QStringLiteral("Standard"));
        data.vertexes = {{0, 0}, {5, 3}, {10, 3}};
        data.vertnum = data.vertexes.size();
        data.horizontalDirection = {1, 0};
        entity = std::make_unique<RS_Leader>(owner, data);
        break;
    }
    case RS2::EntityHatch: {
        auto hatch = std::make_unique<RS_Hatch>(owner, RS_HatchData(true, 1.0, 0.0, "SOLID"));
        auto loop = std::make_unique<RS_EntityContainer>(hatch.get());
        loop->addEntity(new RS_Line(loop.get(), RS_LineData({0, 0}, {10, 0})));
        loop->addEntity(new RS_Line(loop.get(), RS_LineData({10, 0}, {10, 10})));
        loop->addEntity(new RS_Line(loop.get(), RS_LineData({10, 10}, {0, 10})));
        loop->addEntity(new RS_Line(loop.get(), RS_LineData({0, 10}, {0, 0})));
        hatch->addEntity(loop.release());
        entity = std::move(hatch);
        break;
    }
    case RS2::EntityImage:
        entity = std::make_unique<RS_Image>(owner, RS_ImageData(0, {1, 2}, {1, 0}, {0, 1}, {4, 3},
            QString(), 50, 50, 0));
        break;
    case RS2::EntityWipeout: {
        LC_WipeoutData data;
        data.hasNativeFrame = true;
        data.insertionPoint = {1, 2};
        data.uPixel = {1, 0};
        data.vPixel = {0, 1};
        data.sizeU = 4.0;
        data.sizeV = 3.0;
        data.clip = 1;
        data.clipBoundaryType = 2;
        data.clipPath = {{-0.5, -0.5}, {3.5, -0.5}, {3.5, 2.5}, {-0.5, 2.5}};
        entity = std::make_unique<LC_Wipeout>(owner, std::move(data));
        break;
    }
    case RS2::EntityMLeader: {
        LC_MLeaderData data;
        data.hasTextContents = true;
        data.textLabel = QStringLiteral("Coverage MLeader");
        data.textLocation = {20, 5};
        data.contentBasePoint = data.textLocation;
        data.textHeight = 2.0;
        data.textStyleName = QStringLiteral("standard");
        LC_MLeaderRoot root;
        root.connectionPoint = data.textLocation;
        root.direction = {1, 0};
        LC_MLeaderLine line;
        line.points = {{0, 0}, {10, 5}};
        root.leaderLines.push_back(std::move(line));
        data.roots.push_back(std::move(root));
        entity = std::make_unique<LC_MLeader>(owner, std::move(data));
        break;
    }
    case RS2::EntitySpline: {
        RS_SplineData data(3, false);
        data.type = RS_SplineData::SplineType::ClampedOpen;
        data.controlPoints = {{0, 0}, {4, 6}, {8, -6}, {12, 0}};
        data.knotslist = {0, 0, 0, 0, 1, 1, 1, 1};
        data.weights.assign(4, 1.0);
        entity = std::make_unique<RS_Spline>(owner, data);
        break;
    }
    case RS2::EntitySplinePoints: {
        LC_SplinePointsData data(false, false);
        data.splinePoints = {{0, 0}, {5, 7}, {10, 0}};
        entity = std::make_unique<LC_SplinePoints>(owner, std::move(data));
        break;
    }
    case RS2::EntityParabola:
        entity = std::make_unique<LC_Parabola>(owner,
            LC_ParabolaData(std::array<RS_Vector, 3>{{{0, 0}, {5, 7}, {10, 0}}}));
        break;
    default:
        return nullptr;
    }
    entity->setLayer(graphic.findLayer(QStringLiteral("0")));
    entity->update();
    entity->calculateBorders();
    return entity;
}

} // namespace lc::test

#endif
