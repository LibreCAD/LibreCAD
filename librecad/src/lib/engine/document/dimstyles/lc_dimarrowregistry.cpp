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

#include <QSet>

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

std::pair<RS_Entity*, double> LC_DimArrowRegistry::createArrowBlock(RS_EntityContainer* container, const QString& blockName,
                                                                    const RS_Vector& point, const double directionAngle,
                                                                    const double arrowSize) {
    const auto graphic = container->getGraphic();
    ArrowInfo info;
    if (graphic != nullptr) {
        const RS_BlockList* blocksList = graphic->getBlockList();
        const auto customBlock = blocksList->findCaseInsensitive(blockName);
        if (customBlock != nullptr) {
            double dimLineExtension = 0.0;
            // check whether this is a block for standard arrows - and whether we need to adjust dimline points
            // to address AutoCAD-like rendering of such special built-in arrow types
            if (getArrowInfoByBlockName(blockName, info)) {
                dimLineExtension = info.dimLineCorrection;
            }
            auto blockEntity = createCustomArrowBlock(container, customBlock->getName(), point, directionAngle, arrowSize);
            return {blockEntity, dimLineExtension};
        }
    }

    const bool hasArrowType = getArrowInfoByBlockName(blockName, info);
    if (!hasArrowType) {
        info = m_defaultArrowsInfo[0];
    }
    auto blockEntity = createDefaultArrowBlock(container, info.type, point, directionAngle, arrowSize);
    return {blockEntity, 0.0};
}

RS_Entity* LC_DimArrowRegistry::createDefaultArrowBlock(RS_EntityContainer* container, const ArrowType type, const RS_Vector& point,
                                                        const double directionAngle, const double arrowSize) {
    switch (type) {
        case ARROWHEAD_CLOSED_FILLED: {
            return new LC_ArrowHeadClosed(container, point, directionAngle, arrowSize, 0.165, true);
        }
        case ARROWHEAD_DOT: {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::DOT);
        }
        case ARROWHEAD_DOT_SMALL: {
            return new LC_ArrowDot(container, point, directionAngle, arrowSize, LC_ArrowDot::BLANK);
        }
        case ARROWHEAD_DOT_BLANK: {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::DOT_BLANK);
        }
        case ARROWHEAD_ORIGIN_INDICATOR: {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::ORIGIN_INDICATOR);
        }
        case ARROWHEAD_ORIGIN_INDICATOR_2: {
            return new LC_ArrowCircle(container, point, directionAngle, arrowSize, LC_ArrowCircle::ORIGIN_INDICATOR2);
        }
        case ARROWHEAD_OPEN: {
            return new LC_ArrowHeadOpen(container, point, directionAngle, arrowSize, 0.165);
        }
        case ARROWHEAD_RIGHT_ANGLE: {
            return new LC_ArrowHeadOpen(container, point, directionAngle, arrowSize, 0.785398);
        }
        case ARROWHEAD_OPEN_30: {
            return new LC_ArrowHeadOpen(container, point, directionAngle, arrowSize, 0.261799);
        }
        case ARROWHEAD_CLOSED: {
            return new LC_ArrowHeadClosed(container, point, directionAngle, arrowSize, 0.165, false);
        }
        case ARROWHEAD_DOT_SMALL_BLANK: {
            return new LC_ArrowDot(container, point, directionAngle, arrowSize, LC_ArrowDot::BLANK);
        }
        case ARROWHEAD_NONE: {
            return new LC_ArrowNone(container, point, directionAngle, arrowSize);
        }
        case ARROWHEAD_OBLIQUE: {
            return new LC_ArrowTick(container, point, directionAngle, arrowSize, false);
        }
        case ARROWHEAD_BOX_FILLED: {
            return new LC_ArrowBox(container, point, directionAngle, arrowSize, true);
        }
        case ARROWHEAD_BOX: {
            return new LC_ArrowBox(container, point, directionAngle, arrowSize, false);
        }
        case ARROWHEAD_CLOSED_BLANK: {
            return new LC_ArrowHeadClosedBlank(container, point, directionAngle, arrowSize, 0.165);
        }
        case ARROWHEAD_DATUM_TRIANGLE_FILLED: {
            return new LC_ArrowDatum(container, point, directionAngle, arrowSize, true);
        }
        case ARROWHEAD_DATUM_TRIANGLE: {
            return new LC_ArrowDatum(container, point, directionAngle, arrowSize, false);
        }
        case ARROWHEAD_INTEGRAL: {
            return new LC_ArrowIntegral(container, point, directionAngle, arrowSize);
        }
        case ARROWHEAD_ARCHITECTURAL_TICK: {
            return new LC_ArrowTick(container, point, directionAngle, arrowSize, true);
        }
        default:
            break;
    }
    return nullptr;
}

RS_Entity* LC_DimArrowRegistry::createCustomArrowBlock(RS_EntityContainer* container, const QString& blockName, const RS_Vector& point,
                                                       const double directionAngle, const double arrowSize) {
    const auto insertData = new RS_InsertData(blockName, point, RS_Vector(arrowSize, arrowSize),
        directionAngle, 1, 1, RS_Vector(0, 0), nullptr, RS2::Update);

    const auto ins = new RS_Insert(container, *insertData);
    return ins;
}

void LC_DimArrowRegistry::init() {
    if (m_defaultArrowsInfo.empty()) {
        m_defaultArrowsInfo = {
            {"", ARROWHEAD_CLOSED_FILLED, tr("Closed Filled"), 0.0},
            // todo - sand - dot is supported by ACAD for setting default arrow block. Think about adding such support too
            // {".", closed_filled, tr("Closed Filled")},
            {"_DOT", ARROWHEAD_DOT, tr("Dot"), 0.0},
            {"_DOTSMALL", ARROWHEAD_DOT_SMALL, tr("Dot Small"), 1.0},
            {"_DOTBLANK", ARROWHEAD_DOT_BLANK, tr("Dot Blank"), 0.0},
            {"_ORIGIN", ARROWHEAD_ORIGIN_INDICATOR, tr("Origin Indicator"), 0.0},
            {"_ORIGIN2", ARROWHEAD_ORIGIN_INDICATOR_2, tr("Origin Indicator 2"), 0.0},
            {"_OPEN", ARROWHEAD_OPEN, tr("Open"), 0.0},
            {"_OPEN90", ARROWHEAD_RIGHT_ANGLE, tr("Right Angle"), 0.0},
            {"_OPEN30", ARROWHEAD_OPEN_30, tr("Open 30"), 0.0},
            {"_CLOSED", ARROWHEAD_CLOSED, tr("Closed"), 0.0},
            {"_SMALL", ARROWHEAD_DOT_SMALL_BLANK, tr("Dot Small Blank"), 1},
            {"_NONE", ARROWHEAD_NONE, tr("None"), 1.0},
            {ArrowInfo::ARROW_TYPE_OBLIQUE, ARROWHEAD_OBLIQUE, tr("Oblique"), 1.0},
            {"_BOXFILLED", ARROWHEAD_BOX_FILLED, tr("Box Filled"), 0.0},
            {"_BOXBLANK", ARROWHEAD_BOX, tr("Box Blank"), 0.0},
            {"_CLOSEDBLANK", ARROWHEAD_CLOSED_BLANK, tr("Closed Blank"), 0.0},
            {"_DATUMFILLED", ARROWHEAD_DATUM_TRIANGLE_FILLED, tr("Datum Filled"), 0.0},
            {"_DATUMBLANK", ARROWHEAD_DATUM_TRIANGLE, tr("Datum Blank"), 0.0},
            {"_INTEGRAL", ARROWHEAD_INTEGRAL, tr("Integral"), 1.0},
            {ArrowInfo::ARROW_TYPE_ARCHTICK, ARROWHEAD_ARCHITECTURAL_TICK, tr("Architecture Tick"), 1.0}
        };
    }
}

void LC_DimArrowRegistry::insertStandardArrowBlocks(RS_EntityContainer* container) {
    const auto graphic = container->getGraphic();
    if (graphic != nullptr) {
        RS_BlockList* blocksList = graphic->getBlockList();
        for (const auto& arrowInfo : m_defaultArrowsInfo) {
            insertStandardArrowBlock(container, blocksList, arrowInfo);
        }
    }
}

void LC_DimArrowRegistry::fillArrowBlockByEntities(RS_Block* block, const ArrowType arrow) {
    switch (arrow) {
        case ARROWHEAD_CLOSED_FILLED: {
            block->addByBlockEntity(new RS_Solid({{0.0, 0.0}, {-1.0, 0.1667}, {-1.0, -0.1667}}));
            break;
        }
        case ARROWHEAD_DOT: {
            block->addByBlockLine({-1.0, 0.0}, {-0.5, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.25}));
            break;
        }
        case ARROWHEAD_DOT_SMALL: {
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.0625}));
            break;
        }
        case ARROWHEAD_DOT_BLANK: {
            block->addByBlockLine({-1.0, 0.0}, {-0.5, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.5}));
            break;
        }
        case ARROWHEAD_ORIGIN_INDICATOR: {
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.5}));
            break;
        }
        case ARROWHEAD_ORIGIN_INDICATOR_2: {
            block->addByBlockLine({-1.0, 0.0}, {-0.5, 0.0});
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.5}));
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.25}));
            break;
        }
        case ARROWHEAD_OPEN: {
            block->addByBlockLine({-1.0, 0.1667}, {0.0, 0.0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, 0.0});
            block->addByBlockLine({-1.0, -0.1667}, {0.0, 0.0});
            break;
        }
        case ARROWHEAD_RIGHT_ANGLE: {
            block->addByBlockLine({-0.5, 0.5}, {0.0, 0.0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, 0.0});
            block->addByBlockLine({-0.5, -0.5}, {0.0, 0.0});
            break;
        }
        case ARROWHEAD_OPEN_30: {
            block->addByBlockLine({-1.0, 0.2679}, {0.0, 0.0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, 0.0});
            block->addByBlockLine({-1.0, -0.2679}, {0.0, 0.0});
            break;
        }
        case ARROWHEAD_CLOSED: {
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.0});
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.1667});
            block->addByBlockLine({-1.0, 0.1667}, {-1.0, -0.1667});
            block->addByBlockLine({-1.0, -0.1667}, {0.0, 0.0});
            break;
        }
        case ARROWHEAD_DOT_SMALL_BLANK: {
            block->addByBlockEntity(new RS_Circle({{0.0, 0.0}, 0.25}));
            break;
        }
        case ARROWHEAD_NONE: {
            break;
        }
        case ARROWHEAD_OBLIQUE: {
            block->addByBlockLine({-0.5, -0.5}, {0.5, 0.5});
            break;
        }
        case ARROWHEAD_BOX_FILLED: {
            // last 2 vertexes are switched by solid!
            block->addByBlockEntity(new RS_Solid({{-0.5, -0.5}, {-0.5, 0.5}, {0.5, -0.5}, {0.5, 0.5}}));
            block->addByBlockLine({-0.5, 0.0}, {-1.0, 0.0});
            break;
        }
        case ARROWHEAD_BOX: {
            block->addByBlockLine({0.5, -0.5}, {0.5, 0.5});
            block->addByBlockLine({0.5, 0.5}, {-0.5, 0.5});
            block->addByBlockLine({-0.5, 0.5}, {-0.5, -0.5});
            block->addByBlockLine({-0.5, -0.5}, {0.5, -0.5});
            block->addByBlockLine({-0.5, 0.0}, {-1.0, 0.0});
            break;
        }
        case ARROWHEAD_CLOSED_BLANK: {
            block->addByBlockLine({0.0, 0.0}, {-1.0, 0.1667});
            block->addByBlockLine({-1.0, 0.1667}, {-1.0, -0.1667});
            block->addByBlockLine({-1.0, -0.1667}, {0.0, 0.0});
            break;
        }
        case ARROWHEAD_DATUM_TRIANGLE_FILLED: {
            block->addByBlockEntity(new RS_Solid({{0.0, -0.5774}, {0.0, 0.5774}, {-1.0, 0}}));
            break;
        }
        case ARROWHEAD_DATUM_TRIANGLE: {
            block->addByBlockLine({0.0, -0.5774}, {0.0, 0.5774});
            block->addByBlockLine({0.0, 0.5774}, {-1.0, 0});
            block->addByBlockLine({-1.0, 0.0}, {0.0, -0.5774});
            break;
        }
        case ARROWHEAD_INTEGRAL: {
            const RS_ArcData data1({-0.4449, 0.0913}, 0.4542, RS_Math::deg2rad(282), RS_Math::deg2rad(348), false);
            block->addByBlockEntity(new RS_Arc(data1));

            const RS_ArcData data2({0.4449, -0.0913}, 0.4542, RS_Math::deg2rad(66), RS_Math::deg2rad(168), false);
            block->addByBlockEntity(new RS_Arc(data2));
            break;
        }
        case ARROWHEAD_ARCHITECTURAL_TICK: {
            block->addByBlockLine({-0.5, -0.5}, {0.5, 0.5});
            break;
        }
        default:
            break;
    }
}

void LC_DimArrowRegistry::insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList, const ArrowInfo& arrowInfo) {
    const QString blockName = arrowInfo.blockName;
    auto customBlock = blocksList->findCaseInsensitive(blockName);
    if (customBlock == nullptr) {
        const auto d = RS_BlockData(blockName, RS_Vector(0.0, 0.0), false);
        customBlock = new RS_Block(container, d);
        customBlock->setAutoUpdateBorders(false);
        fillArrowBlockByEntities(customBlock, arrowInfo.type);
        blocksList->add(customBlock, true);
    }
}

void LC_DimArrowRegistry::insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList, const ArrowInfo* arrowInfo) {
    const QString blockName = arrowInfo->name;
    const auto d = RS_BlockData(blockName, RS_Vector(0.0, 0.0), false);
    const auto customBlock = new RS_Block(container, d);
    customBlock->setAutoUpdateBorders(false);
    fillArrowBlockByEntities(customBlock, arrowInfo->type);
    blocksList->add(customBlock, false);
}

void LC_DimArrowRegistry::insertStandardArrowBlocks(RS_Graphic* graphic, const QList<LC_DimStyle*>& styles) {
    const auto blockList = graphic->getBlockList();
    const auto container = graphic->getDocument();
    QSet<QString> uniqueArrowBlockNames;
    collectUsedArrowTypes(styles, uniqueArrowBlockNames);

    for (const auto& blockName : std::as_const(uniqueArrowBlockNames)) {
        if (blockList->findCaseInsensitive(blockName) == nullptr) {
            ArrowInfo info;
            if (getArrowInfoByBlockName(blockName, info)) {
                insertStandardArrowBlock(container, blockList, info);
            }
        }
    }
}

void LC_DimArrowRegistry::collectUsedArrowTypes(const QList<LC_DimStyle*>& list, QSet<QString>& uniqueArrowBlockNames) {
    for (const auto dimStyle : list) {
        const auto arrowhead = dimStyle->arrowhead();
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

void LC_DimArrowRegistry::insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList, const ArrowType arrowType) {
    ArrowInfo arrowInfo;
    if (getArrowInfoByType(arrowType, arrowInfo)) {
        insertStandardArrowBlock(container, blocksList, arrowInfo);
    }
}

void LC_DimArrowRegistry::fillDefaultArrowTypes(std::vector<ArrowInfo>& arrowTypes) {
    init();
    for (const auto& at : m_defaultArrowsInfo) {
        QString blockName = at.blockName;
        if (blockName.isEmpty()) {
            blockName = "_CLOSEDFILLED";
        }
        ArrowInfo arrowType(blockName, at.type, at.name, at.dimLineCorrection);
        arrowTypes.push_back(arrowType);
    }
}

bool LC_DimArrowRegistry::isObliqueOrArchArrow(const QString& blockName) {
    return blockName.compare(ArrowInfo::ARROW_TYPE_OBLIQUE, Qt::CaseInsensitive) == 0 || blockName.compare(
        ArrowInfo::ARROW_TYPE_ARCHTICK, Qt::CaseInsensitive) == 0;
}
