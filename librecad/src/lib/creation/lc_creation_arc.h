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

#ifndef LC_CREATIONARC_H
#define LC_CREATIONARC_H

#include "rs_arc.h"

namespace LC_CreationArc {
    bool createFrom3P(const RS_Vector& p1, const RS_Vector& p2, const RS_Vector& p3, RS_ArcData& data);
    bool createFrom2PDirectionRadius(const RS_Vector& startPoint, const RS_Vector& endPoint, double direction1, double radius, RS_ArcData& data);
    bool createFrom2PDirectionAngle(const RS_Vector& startPoint, const RS_Vector& endPoint, double direction1, double angleLength, RS_ArcData& data);
    bool createFrom2PBulge(const RS_Vector& startPoint, const RS_Vector& endPoint, double bulge, RS_ArcData& data);
    bool createFrom2PAngle(const RS_Vector& firstPoint, const RS_Vector& secondPoint, double angle, bool reversed,bool alternate, RS_ArcData& data);
    bool createFrom2PHeight(const RS_Vector& firstPoint, const RS_Vector& secondPoint, double height, bool reversed,
                            bool alternate, RS_ArcData& data);
    bool createFrom2PArcLength(const RS_Vector& firstPoint, const RS_Vector& secondPoint, double arcLen, bool reversed,
                                       bool alternate, RS_ArcData& data);
    bool createFrom2PRadius(const RS_Vector& firstPoint, const RS_Vector& secondPoint, double radius, bool reversed,
                                       bool alternate, RS_ArcData& data);
}

#endif
