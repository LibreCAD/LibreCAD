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

#ifndef LC_DIMARROWREGISTRY_H
#define LC_DIMARROWREGISTRY_H

#include "rs_graphic.h"

class RS_Vector;
class RS_Entity;
class RS_EntityContainer;

enum ArrowType {
    ARROWHEAD_CLOSED_FILLED,
    ARROWHEAD_DOT,
    ARROWHEAD_DOT_SMALL,
    ARROWHEAD_DOT_BLANK,
    ARROWHEAD_ORIGIN_INDICATOR,
    ARROWHEAD_ORIGIN_INDICATOR_2,
    ARROWHEAD_OPEN,
    ARROWHEAD_RIGHT_ANGLE,
    ARROWHEAD_OPEN_30,
    ARROWHEAD_CLOSED,
    ARROWHEAD_DOT_SMALL_BLANK,
    ARROWHEAD_NONE,
    ARROWHEAD_OBLIQUE,
    ARROWHEAD_BOX_FILLED,
    ARROWHEAD_BOX,
    ARROWHEAD_CLOSED_BLANK,
    ARROWHEAD_DATUM_TRIANGLE_FILLED,
    ARROWHEAD_DATUM_TRIANGLE,
    ARROWHEAD_INTEGRAL,
    ARROWHEAD_ARCHITECTURAL_TICK,
    ARROWHEAD_CUSTOM
};

// fixme - convert to namespace?
class LC_DimArrowRegistry : public QObject {
    Q_OBJECT public:
    struct ArrowInfo {
        ArrowInfo() = default;

        ArrowInfo(const QString& block, const ArrowType type, const QString& name, const double dimCorrection)
            : type{type}, name{name}, blockName{block}, dimLineCorrection{dimCorrection} {
        }

        ArrowType type = ARROWHEAD_CUSTOM;
        QString name;
        QString blockName;
        // this is workaround for a hacky way of drawing arrow blocks in AutoCAD. Some standard blocks that are inserted
        // by AutoCAD (like Tick, Arc Tick, Integral, None, Dot Small, Dot Blank) does not include a line that
        // continues dimension line. Therefore, if they are inserted "as is" - there is a gap between dimension line
        // and block content.
        // HOWEVER! it seems that AutoCAD handles such blocks differently and fills that gap. So we use
        // adjustment value there, to extend endpoints of calculated dimension line.
        double dimLineCorrection{0};

        static constexpr auto ARROW_TYPE_ARCHTICK = "_ARCHTICK";
        static constexpr auto ARROW_TYPE_OBLIQUE = "_OBLIQUE";
    };

    LC_DimArrowRegistry();
    static bool isStandardBlockName(const QString& blockName);
    static bool getArrowInfoByBlockName(const QString& blockName, ArrowInfo& found);
    static bool getArrowInfoByType(ArrowType type, ArrowInfo& found);
    static void insertStandardArrowBlocks(RS_Graphic* graphic, const QList<LC_DimStyle*>& styles);
    static void fillDefaultArrowTypes(std::vector<ArrowInfo>& arrowTypes);
    static bool isObliqueOrArchArrow(const QString& blockName);

    std::pair<RS_Entity*, double> createArrowBlock(RS_EntityContainer* container, const QString& blockName, const RS_Vector& point,
                                                   double directionAngle, double arrowSize);

protected:
    RS_Entity* createDefaultArrowBlock(RS_EntityContainer* container, ArrowType type, const RS_Vector& point, double directionAngle,
                                       double arrowSize);
    RS_Entity* createCustomArrowBlock(RS_EntityContainer* container, const QString& blockName, const RS_Vector& point,
                                      double directionAngle, double arrowSize);

    static void insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList, const ArrowInfo& arrowInfo);
    static void insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList, const ArrowInfo* arrowInfo);
    static void collectUsedArrowTypes(const QList<LC_DimStyle*>& list, QSet<QString>& uniqueArrowBlockNames);
    static void insertStandardArrowBlock(RS_EntityContainer* container, RS_BlockList* blocksList, ArrowType arrowType);
    static void fillArrowBlockByEntities(RS_Block* block, ArrowType arrow);

private:
    static std::vector<ArrowInfo> m_defaultArrowsInfo;
    static void init();
    void insertStandardArrowBlocks(RS_EntityContainer* container);
};

#endif
