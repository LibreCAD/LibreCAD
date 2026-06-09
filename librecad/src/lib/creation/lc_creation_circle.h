/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#ifndef LC_CREATIONCIRCLE_H
#define LC_CREATIONCIRCLE_H

#include "rs_circle.h"

namespace LC_CreationCircle {
    bool createFromCR(const RS_Vector& c, double r, RS_CircleData& data);
    bool createFrom2P(const RS_Vector& p1, const RS_Vector& p2, RS_CircleData& data);
    bool createFrom3P(const RS_Vector& p1, const RS_Vector& p2, const RS_Vector& p3, RS_CircleData& data);
    bool createFrom3P(const RS_VectorSolutions& sol, RS_CircleData& data);
    bool createInscribe(const RS_Vector& coord, const std::vector<RS_Line*>& lines, RS_CircleData& data);
    std::vector<RS_Circle> createTan3(const std::vector<RS_AtomicEntity*>& circles);
    RS_VectorSolutions createTan1_2P(const RS_AtomicEntity* circle, const std::vector<RS_Vector>& points);
    RS_VectorSolutions createTan2(const std::vector<RS_AtomicEntity*>& circles, double r);
    bool create2PRadius(const RS_Vector& point1, const RS_Vector& point2, double radius, RS_Vector& altCenter, RS_CircleData& m_circleData);
}

#endif
