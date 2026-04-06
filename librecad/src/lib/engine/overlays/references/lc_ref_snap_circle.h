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

#ifndef LC_REFSNAPCIRCLE_H
#define LC_REFSNAPCIRCLE_H

#include "lc_ref_snap_entity.h"
#include "rs_circle.h"

class LC_RefSnapCircle :public RS_Circle, public LC_RefSnapEntity{
public:
    LC_RefSnapCircle(RS_EntityContainer *parent, const RS_Vector &center, double radius);
    LC_RefSnapCircle(const RS_Vector &center, double radius);
    RS2::EntityType rtti() const override;
    RS_Entity *clone() const override;
    bool isArc() const override {return true;}
    bool isArcCircleLine() const override {return true;}
    void draw(RS_Painter* painter) override;
};
#endif
