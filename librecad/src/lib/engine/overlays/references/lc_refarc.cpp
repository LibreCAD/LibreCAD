/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_refarc.h"

LC_RefArc::LC_RefArc(RS_EntityContainer *parent, const RS_ArcData &d):RS_Arc(parent, d){}

RS2::EntityType LC_RefArc::rtti() const{
    return RS2::EntityRefArc;
}

RS_Entity *LC_RefArc::clone() const{
    auto* a = new LC_RefArc(*this);
    a->initId();
    return a;
}
