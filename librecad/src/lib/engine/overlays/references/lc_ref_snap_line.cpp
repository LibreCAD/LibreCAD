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

#include "lc_ref_snap_line.h"

LC_RefSnapLine::LC_RefSnapLine(const RS_Vector& pStart, const RS_Vector& pEnd) :
    RS_Line(nullptr, RS_LineData(pStart, pEnd)) {
}

RS2::EntityType LC_RefSnapLine::rtti() const {
    return RS2::EntitySnapLine;
}

RS_Entity* LC_RefSnapLine::clone() const {
    auto* l = new LC_RefSnapLine(*this);
    l->updateSnapInfo(m_snapInfo);
    return l;
}

void LC_RefSnapLine::draw(RS_Painter* painter) {
    RS_Line::draw(painter);
    if (!m_labelString.isEmpty()) {
        drawMarker(painter, m_font, getStartpoint(), m_baseLabelOffset, m_snapInfo.labelOffset, m_labelString);
    }
}
