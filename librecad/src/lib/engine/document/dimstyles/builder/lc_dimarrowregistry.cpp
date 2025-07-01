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
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_creation.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_vector.h"



LC_DimArrowRegistry::LC_DimArrowRegistry() {
    init();
}

std::vector<LC_DimArrowRegistry::ArrowInfo> LC_DimArrowRegistry::m_defaultArrowsInfo;

QString LC_DimArrowRegistry::ArrowInfo::ARROW_TYPE_OBLIQUE = "_OBLIQUE";
QString LC_DimArrowRegistry::ArrowInfo::ARROW_TYPE_ARCHTICK = "_ARCHTICK";

bool LC_DimArrowRegistry::getArrowInfoByBlockName(const QString& blockName, ArrowInfo& found) const {
    QString nameToFind = blockName.toUpper();
    for (auto info: m_defaultArrowsInfo) {
        if (info.blockName == nameToFind) {
            found = info;
            return true;
        }
    }
    return false;
}

bool LC_DimArrowRegistry::getArrowInfoByType(const ArrowType type, ArrowInfo& found) const {
    for (auto info: m_defaultArrowsInfo) {
        if (info.type == type) {
            found = info;
            return true;
        }
    }
    return false;
}

RS_Entity* LC_DimArrowRegistry::createArrowBlock(RS_EntityContainer* container, const QString& blockName, const RS_Vector& point, double directionAngle, double arrowSize) {
    auto graphic = container->getGraphic();
    if (graphic != nullptr) {
        RS_BlockList* blocksList = graphic->getBlockList();
        auto customBlock = blocksList->findCaseInsensitive(blockName);
        if (customBlock != nullptr) {
            return createCustomArrowBlock(container, blockName, point, directionAngle, arrowSize);
        }
    }
    ArrowInfo info;
    bool hasArrowType = getArrowInfoByBlockName(blockName, info);
    if (!hasArrowType) {
        info = m_defaultArrowsInfo[0];
    }
    return createDefaultArrowBlock(container, info.type, point, directionAngle, arrowSize);

}

RS_Entity* LC_DimArrowRegistry::createDefaultArrowBlock(RS_EntityContainer* container, ArrowType type, const RS_Vector &point,
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
            return new LC_ArrowNone(container, point,  directionAngle, arrowSize);
        }
        case (oblique): {
            return new LC_ArrowTick(container, point,  directionAngle, arrowSize, false);
        }
        case (box_filled): {
            return new LC_ArrowBox(container, point, directionAngle, arrowSize, true);
        }
        case (box): {
            return new LC_ArrowBox(container, point,  directionAngle, arrowSize, false);
        }
        case (closed_blank): {
            return new LC_ArrowHeadClosedBlank(container, point, directionAngle, arrowSize, 0.165);
        }
        case (datum_triangle_filled): {
            return new LC_ArrowDatum(container, point, directionAngle, arrowSize, true);
        }
        case (datum_triangle): {
            return new LC_ArrowDatum(container, point,  directionAngle, arrowSize, false);
        }
        case (integral): {
            return new LC_ArrowIntegral(container, point, directionAngle, arrowSize);
        }
        case (architectural_tick): {
            return new LC_ArrowTick(container, point,  directionAngle, arrowSize, true);
        }
        default:
            break;
    }
    return nullptr;
}

RS_Entity* LC_DimArrowRegistry::createCustomArrowBlock(RS_EntityContainer* container, QString blockName,
    const RS_Vector &point, double direction_angle, double arrowSize) {

    auto insertData = new RS_InsertData(blockName, point, RS_Vector(arrowSize, arrowSize), direction_angle,
                                   1, 1, RS_Vector(0, 0), nullptr, RS2::Update);

    auto ins = new RS_Insert(container, *insertData);
    return ins;
}

void LC_DimArrowRegistry::init() {
    if (m_defaultArrowsInfo.empty()) {
        m_defaultArrowsInfo = {
            {"", closed_filled, tr("Closed Filled")},
            {"_DOT", dot, tr("Dot")},
            {"_DOTSMALL", dot_small, tr("Dot Small")},
            {"_DOTBLANK", dot_blank, tr("Dot Blank")},
            {"_ORIGIN", origin_indicator, tr("Origin Indicator")},
            {"_ORIGIN2", origin_indicator_2, tr("Origin Indicator 2")},
            {"_OPEN", open, tr("Open")},
            {"_OPEN90", right_angle, tr("Right Angle")},
            {"_OPEN30", open_30, tr("Open 30")},
            {"_CLOSED", closed, tr("Closed")},
            {"_SMALL", dot_small_blank, tr("Dot Small Blank")},
            {"_NONE", none, tr("None")},
            {ArrowInfo::ARROW_TYPE_OBLIQUE, oblique, tr("Oblique")},
            {"_BOXFILLED", box_filled, tr("Box Filled")},
            {"_BOXBLANK", box, tr("Box Blank")},
            {"_CLOSEDBLANK", closed_blank, tr("Closed Blank")},
            {"_DATUMFILLED", datum_triangle_filled, tr("Datum Filled")},
            {"_DATUMBLANK", datum_triangle, tr("Datum Blank")},
            {"_INTEGRAL", integral, tr("Integral")},
            {ArrowInfo::ARROW_TYPE_ARCHTICK, architectural_tick, tr("Architecture Tick")}
        };
    }
}

void LC_DimArrowRegistry::fillDefaultArrowTypes(std::vector<ArrowInfo>& arrowTypes) {
    init();
    for (auto at: m_defaultArrowsInfo) {
        QString blockName = at.blockName;
        if (blockName.isEmpty()) {
            blockName = "_CLOSEDFILLED";
        }
        ArrowInfo arrowType(blockName, at.type, at.name);
        arrowTypes.push_back(arrowType);
    }
}

bool LC_DimArrowRegistry::isObliqueOrArchArrow(const QString& blockName) {
    return blockName == ArrowInfo::ARROW_TYPE_OBLIQUE || blockName == ArrowInfo::ARROW_TYPE_ARCHTICK;
}
