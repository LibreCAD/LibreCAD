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

#include "lc_dimarrowblockpoly.h"

#include <boost/next_prior.hpp>

LC_DimArrowPoly::LC_DimArrowPoly(RS_EntityContainer* parent, const RS_Vector &pos, double angle, double size)
      : LC_DimArrow(parent,pos,angle,size) {
}

RS_Vector LC_DimArrowPoly::getNearestEndpoint(const RS_Vector& coord, double* dist /*= nullptr*/) const {
    double minDist{RS_MAXDOUBLE};
    double curDist{0.0};
    RS_Vector ret;

    for (const auto vertex : m_vertices) {
        curDist = vertex.distanceTo(coord);
        if (curDist < minDist) {
            ret = vertex;
            minDist = curDist;
        }
    }

    auto pos = getPosition();
    curDist = pos.distanceTo(coord);
    if (curDist < minDist) {
        ret = pos;
        minDist = curDist;
    }
    setDistPtr(dist, minDist);
    return ret;
}

RS_Vector LC_DimArrowPoly::getNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist,
    [[maybe_unused]]RS_Entity** entity) const {

    //
    // this is not exact and quite generic implementation.
    // it skips specifics of the geometry and relies on vertexes only.
    // however, for most cases for arrows it should be fine.
    // Actually, the major reason for bothering with nearest points -
    // is just to ensure that the entire dimension entity may be caught by
    // catchEntity(..) if the mouse hovers over the arrow entity.
    // All arrows are not full-fledge persistent entities, they are just
    // used for drawing as part of dimension ...

    RS_Vector ret(false);
    double currDist {RS_MAXDOUBLE};
    int next;
    size_t lastIndex = m_vertices.size() - 2;
    for (size_t i = 0; i <= lastIndex; i++) {
        next = i + 1;
        auto current = m_vertices[i];

        RS_Vector direction {m_vertices[next] - current};
        RS_Vector vpc {coord-current};
        double a {direction.squared()};
        if( a < RS_TOLERANCE2) {
            //line too short
            vpc = current;
        }
        else{
            //find projection on line
            vpc = current + direction * RS_Vector::dotP( vpc, direction) / a;
        }
        double tmpDist = vpc.distanceTo( coord);
        if (tmpDist < currDist) {
            currDist = tmpDist;
            ret = vpc;
        }
    }
    if (onEntity && !ret.isInWindowOrdered( minV, maxV)) {
        // projection point not within range, find the nearest endpoint
        ret = getNearestEndpoint( coord, dist);
        currDist = ret.distanceTo( coord);
    }
    setDistPtr( dist, currDist);
    return ret;
}

void LC_DimArrowPoly::positionFromZero() {
    RS_Vector angleVector(getAngle());
    positionDimLinePointFromZero(angleVector);
    auto position = getPosition();
    for (auto& vertex : m_vertices) {
        vertex.move(position);
        vertex.rotate(position, angleVector);
    }

    calculateBorders();
}

void LC_DimArrowPoly::doCalculateBorders() {
    for (const auto vertex : m_vertices) {
        minV = RS_Vector::minimum(minV, vertex);
        maxV = RS_Vector::maximum(maxV, vertex);
    }
}

void LC_DimArrowPoly::doMove(const RS_Vector& offset) {
    for (auto& vertex : m_vertices) {
        vertex.move(offset);
    }
}

void LC_DimArrowPoly::doRotate(const RS_Vector& center, const RS_Vector& angleVector) {
    for (auto& vertex : m_vertices) {
        vertex.rotate(center, angleVector);
    }
}

void LC_DimArrowPoly::doScale(const RS_Vector& center, const RS_Vector& factor) {
    for (auto& vertex : m_vertices) {
        vertex.scale(center, factor);
    }
}

void LC_DimArrowPoly::doMirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    for (auto& vertex : m_vertices) {
        vertex.mirror(axisPoint1, axisPoint2);
    }
}
