// File: lc_wipeout.h

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef LC_WIPEOUT_H
#define LC_WIPEOUT_H

#include <vector>

#include "rs_atomicentity.h"

struct LC_WipeoutData {
    LC_WipeoutData() = default;
    explicit LC_WipeoutData(std::vector<RS_Vector> verts)
        : vertices(std::move(verts)) {}

    std::vector<RS_Vector> vertices;
};

class LC_Wipeout : public RS_AtomicEntity {
public:
    LC_Wipeout(RS_EntityContainer* parent, LC_WipeoutData d);

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityWipeout;
    }

    const LC_WipeoutData& getData() const { return data; }
    const std::vector<RS_Vector>& getVertices() const { return data.vertices; }

    void calculateBorders() override;
    void draw(RS_Painter* painter) override;

    RS_Vector getNearestEndpoint(const RS_Vector& coord,
                                 double* dist = nullptr) const override;
    RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
                                      bool onEntity = true,
                                      double* dist = nullptr,
                                      RS_Entity** entity = nullptr) const override;
    RS_Vector getNearestCenter(const RS_Vector& coord,
                               double* dist = nullptr) const override;
    RS_Vector getNearestMiddle(const RS_Vector& coord,
                               double* dist = nullptr,
                               int middlePoints = 1) const override;
    RS_Vector getNearestDist(double distance,
                             const RS_Vector& coord,
                             double* dist = nullptr) const override;
    double getDistanceToPoint(const RS_Vector& coord,
                              RS_Entity** entity = nullptr,
                              RS2::ResolveLevel level = RS2::ResolveNone,
                              double solidDist = RS_MAXDOUBLE) const override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear([[maybe_unused]] double k) override { return *this; }

protected:
    LC_WipeoutData data;
};

#endif // LC_WIPEOUT_H