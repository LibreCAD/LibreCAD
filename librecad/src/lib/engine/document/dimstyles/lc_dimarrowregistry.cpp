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

#include "lc_dimarrowregistry.h"

#include "lc_arrow_box.h"
#include "lc_arrow_circle.h"
#include "lc_arrow_datum.h"
#include "lc_arrow_dot.h"
#include "lc_arrow_headclosed.h"
#include "lc_arrow_headclosed_blank.h"
#include "lc_arrow_headopen.h"
#include "lc_arrow_integral.h"
#include "lc_arrow_none.h"
#include "lc_arrow_tick.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_circle.h"
#include "rs_creation.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_solid.h"
#include "rs_vector.h"

LC_DimArrowRegistry::LC_DimArrowRegistry() {
    init();
}

std::vector<LC_DimArrowRegistry::ArrowInfo> LC_DimArrowRegistry::m_defaultArrowsInfo;

QString LC_DimArrowRegistry::ArrowInfo::ARROW_TYPE_OBLIQUE = "_OBLIQUE";
QString LC_DimArrowRegistry::ArrowInfo::ARROW_TYPE_ARCHTICK = "_ARCHTICK";

bool LC_DimArrowRegistry::isStandardBlockName(const QString& blockName) {
    init();
    for (const auto& info : m_defaultArrowsInfo) {
        if (info.blockName.compare(blockName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool LC_DimArrowRegistry::getArrowInfoByBlockName(const QString& blockName, ArrowInfo& found) {
    init();
    for (const auto& info : m_defaultArrowsInfo) {
        if (info.blockName.compare(blockName, Qt::CaseInsensitive) == 0) {
            found = info;
            return true;
        }
    }
    return false;
}

bool LC_DimArrowRegistry::getArrowInfoByType(const ArrowType type, ArrowInfo& found) {
    init();
    for (const auto& info : m_defaultArrowsInfo) {
        if (info.type == type) {
            found = info;
            return true;
        }
    }
    return false;
}

std::pair<RS_Entity*, double> LC_DimArrowRegistry::createArrowBlock(RS_EntityContainer* container,
                                                                    const QString& blockName, const RS_Vector& point,
                                                                    double directionAngle, double arrowSize) {
    auto graphic = container->getGraphic();
    ArrowInfo info;
    if (graphic != nullptr) {
        RS_BlockList* blocksList = graphic->getBlockList();
        auto customBlock = blocksList->findCaseInsensitive(blockName);
        if (customBlock != nullptr) {
            double dimLineExtension = 0.0;
            // check whether this is a block for standard arrows - and whether we need to adjust dimline points
            // to address AutoCAD-like rendering of such special built-in arrow types
            if (getArrowInfoByBlockName(blockName, info)) {
                dimLineExtension = info.dimLineCorrection;
            }
            auto blockEntity = createCustomArrowBlock(container, customBlock->getName(), point, directionAngle,
                                                      arrowSize);
            return {blockEntity, dimLineExtension};
        }
    }

    bool hasArrowType = getArrowInfoByBlockName(blockName, info);
    if (!hasArrowType) {
        info = m_defaultArrowsInfo[0];
    }
    auto blockEntity = createDefaultArrowBlock(container, info.type, point, directionAngle, arrowSize);
    return {blockEntity, 0.0};
}

RS_Entity* LC_DimArrowRegistry::createDefaultArrowBlock(RS_EntityContainer* container, ArrowType type,
                                                        const RS_Vector& point,
                                                        double directionAngle, double arrowSize) {
    switch (type) {
        case (closed_filled): {
            return new LC_ArrowHeadClosed(container, point, directionAngle, arrowSize, 0.165, true);
        }
        case (dot): {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::dot);
        }
        case (dot_small): {
            return new LC_ArrowDot(container, point, directionAngle, arrowSize, LC_ArrowDot::blank);
        }
        case (dot_blank): {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::dot_blank);
        }
        case (origin_indicator): {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::origin_indicator);
        }
        case (origin_indicator_2): {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::origin_indicator2);
        }
        case (open): {
            return new LC_ArrowHeadOpen(container, point, directionAngle, arrowSize, 0.165);
        }
        case (right_angle): {
            return new LC_ArrowHeadOpen(container, point, directionAngle, arrowSize, 0.785398);
        }
        case (open_30): {
            return new LC_ArrowHeadOpen(container, point, directionAngle, arrowSize, 0.261799);
        }
        case (closed): {
            return new LC_ArrowHeadClosed(container, point, directionAngle, arrowSize, 0.165, false);
        }
        case (dot_small_blank): {
            return new LC_ArrowDot(container, point, directionAngle, arrowSize, LC_ArrowDot::blank);
        }
        case (none): {
            return new LC_ArrowNone(container, point, directionAngle, arrowSize);
        }
        case (oblique): {
            return new LC_ArrowTick(container, point, directionAngle, arrowSize, false);
        }
        case (box_filled): {
            return new LC_ArrowBox(container, point, directionAngle, arrowSize, true);
        }
        case (box): {
            return new LC_ArrowBox(container, point, directionAngle, arrowSize, false);
        }
        case (closed_blank): {
            return new LC_ArrowHeadClosedBlank(container, point, directionAngle, arrowSize, 0.165);
        }
        case (datum_triangle_filled): {
            return new LC_ArrowDatum(container, point, directionAngle, arrowSize, true);
        }
        case (datum_triangle): {
            return new LC_ArrowDatum(container, point, directionAngle, arrowSize, false);
        }
        case (integral): {
            return new LC_ArrowIntegral(container, point, directionAngle, arrowSize);
        }
        case (architectural_tick): {
            return new LC_ArrowTick(container, point, directionAngle, arrowSize, true);
        }
        default:
            break;
    }
    return nullptr;
}

RS_Entity* LC_DimArrowRegistry::createCustomArrowBlock(RS_EntityContainer* container, QString blockName,
                                                       const RS_Vector& point, double direction_angle,
                                                       double arrowSize) {
    auto insertData = new RS_InsertData(blockName, point, RS_Vector(arrowSize, arrowSize), direction_angle,
                                        1, 1, RS_Vector(0, 0), nullptr, RS2::Update);

    auto ins = new RS_Insert(container, *insertData);
    return ins;
}

void LC_DimArrowRegistry::init() {
    if (m_defaultArrowsInfo.empty()) {
        m_defaultArrowsInfo = {
            {"", closed_filled, tr("Closed Filled"), 0.0},
            // todo - sand - dot is supported by ACAD for setting default arrow block. Think about adding such support too
            // {".", closed_filled, tr("Closed Filled")},
            {"_DOT", dot, tr("Dot"), 0.0},
            {"_DOTSMALL", dot_small, tr("Dot Small"), 1.0},
            {"_DOTBLANK", dot_blank, tr("Dot Blank"), 0.0},
            {"_ORIGIN", origin_indicator, tr("Origin Indicator"), 0.0},
            {"_ORIGIN2", origin_indicator_2, tr("Origin Indicator 2"), 0.0},
            {"_OPEN", open, tr("Open"), 0.0},
            {"_OPEN90", right_angle, tr("Right Angle"), 0.0},
            {"_OPEN30", open_30, tr("Open 30"), 0.0},
            {"_CLOSED", closed, tr("Closed"), 0.0},
            {"_SMALL", dot_small_blank, tr("Dot Small Blank"), 1},
            {"_NONE", none, tr("None"), 1.0},
            {ArrowInfo::ARROW_TYPE_OBLIQUE, oblique, tr("Oblique"), 1.0},
            {"_BOXFILLED", box_filled, tr("Box Filled"), 0.0},
            {"_BOXBLANK", box, tr("Box Blank"), 0.0},
            {"_CLOSEDBLANK", closed_blank, tr("Closed Blank"), 0.0},
            {"_DATUMFILLED", datum_triangle_filled, tr("Datum Filled"), 0.0},
            {"_DATUMBLANK", datum_triangle, tr("Datum Blank"), 0.0},
            {"_INTEGRAL", integral, tr("Integral"), 1.0},
            {ArrowInfo::ARROW_TYPE_ARCHTICK, architectural_tick, tr("Architecture Tick"), 1.0}
        };
    }
}

void LC_DimArrowRegistry::insertStandardArrowBlocks(RS_EntityContainer* container) {
    auto graphic = container->getGraphic();
    ArrowInfo info;
    if (graphic != nullptr) {
        RS_BlockList* blocksList = graphic->getBlockList();
        for (auto arrowInfo : m_defaultArrowsInfo) {
            insertStandardArrowBlock(container, blocksList, arrowInfo);
        }
    }
}

void LC_DimArrowRegistry::fillArrowBlockByEntities(RS_Block* block, ArrowType arrow) {
    switch (arrow) {
        case closed_filled: {
            block->addByBlockEntity(new RS_Solid({{0.0, 0.0}, {-1.0, 0.1667}, {-1.0, -0.1667}}));
            break;
        }
        case dot: {
            block->addByBlockLine({-1.0, 0.0}, {-0.5, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.25}));
            break;
        }
        case dot_small: {
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.0625}));
            break;
        }
        case dot_blank: {
            block->addByBlockLine({-1.0, 0.0}, {-0.5, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.5}));
            break;
        }
        case origin_indicator: {
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.5}));
            break;
        }
        case origin_indicator_2: {
            block->addByBlockLine({-1.0, 0.0}, {-0.5, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.5}));
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.25}));
            break;
        }
        case open: {
            block->addByBlockLine({-1.0, 0.1667}, {0.0, 0.0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, 0.0});
            block->addByBlockLine({-1.0, -0.1667}, {0.0, 0.0});
            break;
        }
        case right_angle: {
            block->addByBlockLine({-0.5, 0.5}, {0.0, 0.0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, 0.0});
            block->addByBlockLine({-0.5, -0.5}, {0.0, 0.0});
            break;
        }
        case open_30: {
            block->addByBlockLine({-1.0, 0.2679}, {0.0, 0.0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, 0.0});
            block->addByBlockLine({-1.0, -0.2679}, {0.0, 0.0});
            break;
        }
        case closed: {
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.0});
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.1667});
            block->addByBlockLine({-1.0, 0.1667}, {-1.0, -0.1667});
            block->addByBlockLine({-1.0, -0.1667}, {0.0, 0.0});
            break;
        }
        case dot_small_blank: {
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.25}));
            break;
        }
        case none: {
            break;
        }
        case oblique: {
            block->addByBlockLine({-0.5, -0.5}, {0.5, 0.5});
            break;
        }
        case box_filled: {
            // last 2 vertexes are switched by solid!
            block->addByBlockEntity(new RS_Solid({{-0.5, -0.5}, {-0.5, 0.5},  {0.5, -0.5}, {0.5, 0.5}}));
            block->addByBlockLine({-0.5, 0.0}, {-1.0, 0.0});
            break;
        }
        case box: {
            block->addByBlockLine({0.5, -0.5}, {0.5, 0.5});
            block->addByBlockLine({0.5, 0.5}, {-0.5, 0.5});
            block->addByBlockLine({-0.5, 0.5}, {-0.5, -0.5});
            block->addByBlockLine({-0.5, -0.5}, {0.5, -0.5});
            block->addByBlockLine({-0.5, 0.0}, {-1.0, 0.0});
            break;
        }
        case closed_blank: {
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.1667});
            block->addByBlockLine({-1.0, 0.1667}, {-1.0, -0.1667});
            block->addByBlockLine({-1.0, -0.1667}, {0.0, 0.0});
            break;
        }
        case datum_triangle_filled: {
            block->addByBlockEntity(new RS_Solid({{0.0, -0.5774}, {0.0, 0.5774}, {-1.0, 0}}));
            break;
        }
        case datum_triangle: {
            block->addByBlockLine({0.0, -0.5774}, {0.0, 0.5774});
            block->addByBlockLine({0.0, 0.5774}, {-1.0, 0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, -0.5774});
            break;
        }
        case integral: {
            RS_ArcData data1({-0.4449, 0.0913}, 0.4542, RS_Math::deg2rad(282), RS_Math::deg2rad(348), false);
            block->addByBlockEntity(new RS_Arc(data1));

            RS_ArcData data2({0.4449, -0.0913}, 0.4542, RS_Math::deg2rad(66), RS_Math::deg2rad(168), false);
            block->addByBlockEntity(new RS_Arc(data2));
            break;
        }
        case architectural_tick: {
            block->addByBlockLine({-0.5, -0.5}, {0.5, 0.5});
            break;
        }
        default:
            break;
    }
}

void LC_DimArrowRegistry::insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList,
                                                   LC_DimArrowRegistry::ArrowInfo arrowInfo) {
    QString blockName = arrowInfo.blockName;
    auto customBlock = blocksList->findCaseInsensitive(blockName);
    if (customBlock == nullptr) {
        RS_BlockData d = RS_BlockData(blockName, RS_Vector(0.0, 0.0), false);
        customBlock = new RS_Block(container, d);
        customBlock->setAutoUpdateBorders(false);
        fillArrowBlockByEntities(customBlock, arrowInfo.type);
        blocksList->add(customBlock, true);
    }
}

void LC_DimArrowRegistry::insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList,
                                                   LC_DimArrowRegistry::ArrowInfo* arrowInfo) {
    QString blockName = arrowInfo->name;
    RS_BlockData d = RS_BlockData(blockName, RS_Vector(0.0, 0.0), false);
    auto customBlock = new RS_Block(container, d);
    customBlock->setAutoUpdateBorders(false);
    fillArrowBlockByEntities(customBlock, arrowInfo->type);
    blocksList->add(customBlock, false);
}

void LC_DimArrowRegistry::insertStandardArrowBlocks(RS_Graphic* graphic, const QList<LC_DimStyle*>& styles) {
    auto blockList = graphic->getBlockList();
    auto container = graphic->getDocument();
    QSet<QString> uniqueArrowBlockNames;
    collectUsedArrowTypes(styles, uniqueArrowBlockNames);

    for (const auto& blockName : uniqueArrowBlockNames) {
        if (blockList->findCaseInsensitive(blockName) == nullptr) {
            ArrowInfo info;
            if (getArrowInfoByBlockName(blockName, info)) {
                insertStandardArrowBlock(container, blockList, info);
            }
        }
    }
}

void LC_DimArrowRegistry::collectUsedArrowTypes(const QList<LC_DimStyle*>& list, QSet<QString>& uniqueArrowBlockNames) {
    for (auto dimStyle : list) {
        auto arrowhead = dimStyle->arrowhead();
        QString firstArrowBlockName = arrowhead->arrowHeadBlockNameFirst();
        QString secondArrowBlockName = arrowhead->arrowHeadBlockNameSecond();
        QString sameArrowBlockName = arrowhead->sameBlockName();
        QString leaderName = dimStyle->leader()->arrowBlockName();

        if (!firstArrowBlockName.isEmpty()) {
            uniqueArrowBlockNames << firstArrowBlockName;
        }
        if (!secondArrowBlockName.isEmpty()) {
            uniqueArrowBlockNames << secondArrowBlockName;
        }
        if (!sameArrowBlockName.isEmpty()) {
            uniqueArrowBlockNames << sameArrowBlockName;
        }
        if (!leaderName.isEmpty()) {
            uniqueArrowBlockNames << leaderName;
        }
    }

    // no need for default arrow
    if (uniqueArrowBlockNames.contains("_CLOSEDFILLED")) {
        uniqueArrowBlockNames.remove("_CLOSEDFILLED");
    }
}

void LC_DimArrowRegistry::insertStandardArrowBlock(RS_EntityContainer* container,
                                                   RS_BlockList* blocksList,
                                                   ArrowType arrowType) {
    ArrowInfo arrowInfo;
    if (getArrowInfoByType(arrowType, arrowInfo)) {
        insertStandardArrowBlock(container, blocksList, arrowInfo);
    }
}

void LC_DimArrowRegistry::fillDefaultArrowTypes(std::vector<ArrowInfo>& arrowTypes) {
    init();
    for (auto at : m_defaultArrowsInfo) {
        QString blockName = at.blockName;
        if (blockName.isEmpty()) {
            blockName = "_CLOSEDFILLED";
        }
        ArrowInfo arrowType(blockName, at.type, at.name, at.dimLineCorrection);
        arrowTypes.push_back(arrowType);
    }
}

bool LC_DimArrowRegistry::isObliqueOrArchArrow(const QString& blockName) {
    return blockName.compare(ArrowInfo::ARROW_TYPE_OBLIQUE, Qt::CaseInsensitive) == 0 ||
        blockName.compare(ArrowInfo::ARROW_TYPE_ARCHTICK, Qt::CaseInsensitive) == 0;
}
