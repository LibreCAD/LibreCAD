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

#ifndef LC_DIMARROWBLOCKPOLY_H
#define LC_DIMARROWBLOCKPOLY_H
#include "lc_dimarrowblock.h"

class LC_DimArrowPoly: public LC_DimArrow{
public:
    LC_DimArrowPoly(RS_EntityContainer* parent, const RS_Vector &pos, double angle, double size);
    RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist) const override;
    RS_Vector getNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist,
                                      RS_Entity** entity) const override;
protected:
    void positionFromZero();
    void doCalculateBorders() override;
    void doMove(const RS_Vector& offset) override;
    auto doRotate(const RS_Vector& center, const RS_Vector& angleVector) -> void;
    void doScale(const RS_Vector& center, const RS_Vector& factor) override;
    void doMirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void initVertexes(int size){m_vertices.resize(size);}
    void addVertex(RS_Vector vertex) {m_vertices.push_back(vertex);}
    void addVertex(double x, double y) {m_vertices.push_back({x, y});}
    void setVertex(int index, RS_Vector vertex) {m_vertices[index] = vertex;}
    void setVertex(int index, double x, double y) {m_vertices[index] = {x,y};}
    RS_Vector vertexAt(int index){return m_vertices[index];}

    const std::vector<RS_Vector> &getVertexes() const {
        return m_vertices;
    }
private:
    std::vector<RS_Vector> m_vertices;
};

#endif // LC_DIMARROWBLOCKPOLY_H
