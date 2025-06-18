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
#include <QObject>
#include <QString>

class RS_Vector;
class RS_Entity;
class RS_EntityContainer;

enum ArrowType {
    closed_filled,
    dot,
    dot_small,
    dot_blank,
    origin_indicator,
    origin_indicator_2,
    open,
    right_angle,
    open_30,
    closed,
    dot_small_blank,
    none,
    oblique,
    box_filled,
    box,
    closed_blank,
    datum_triangle_filled,
    datum_triangle,
    integral,
    architectural_tick,
    _CUSTOM
};

class LC_DimArrowRegistry: public QObject{
    Q_OBJECT
public:
    struct ArrowInfo {
        ArrowInfo() = default;

        ArrowInfo( const QString& block, ArrowType type, const QString& name)
            : type{type},
              name{name},
              blockName{block} {
        }

        ArrowType type = _CUSTOM;
        QString blockName;
        QString name;
    };

    LC_DimArrowRegistry();
    bool getArrowInfoByBlockName(const QString& blockName, ArrowInfo &found) const;
    bool getArrowInfoByType(ArrowType type, ArrowInfo & found) const;
    RS_Entity* createArrowBlock(RS_EntityContainer* container, const QString& blockName, RS_Vector& point, double directionAngle, double arrowSize);

    static void fillDefaultArrowTypes(std::vector<ArrowInfo>& arrowTypes);
protected:
    RS_Entity* createDefaultArrowBlock(RS_EntityContainer* container, ArrowType type, RS_Vector point, double directionAngle, double
                                       arrowSize);
    RS_Entity* createCustomArrowBlock(RS_EntityContainer* container, QString blockName, RS_Vector point, double direction_angle, double arrowSize);
private:
    static std::vector<ArrowInfo> m_defaultArrowsInfo;
    static void init();
};

#endif // LC_DIMARROWREGISTRY_H
