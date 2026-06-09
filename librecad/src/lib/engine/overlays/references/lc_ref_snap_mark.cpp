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

#include "lc_ref_snap_mark.h"

#include "dxf_format.h"
#include "rs_painter.h"

LC_RefSnapMark::LC_RefSnapMark(RS_EntityContainer* parent, const RS_Vector& d, int sizePx, MarkType markType)
    : RS_Point(parent, RS_PointData(d)), m_markType(markType), m_sizePx{sizePx} {
    LC_RefSnapMark::calculateBorders();
}

RS2::EntityType LC_RefSnapMark::rtti() const {
    return RS2::EntitySnapMark;
}

RS_Entity* LC_RefSnapMark::clone() const {
    auto* p = new LC_RefSnapMark(*this);
    return p;
}

void LC_RefSnapMark::draw(RS_Painter* painter) {
    double uiX, uiY;
    painter->toGui(m_data.pos, uiX, uiY);
    int mode;
    switch (m_markType) {
        case NORMAL: {
            mode = DXF_FORMAT_PDMode_CentrePlus;
            break;
        }
        case HIGHLIGHTED: {
            mode = NON_DXF_FORMAT_PDMode_EncloseCircleOUTER(DXF_FORMAT_PDMode_CentreCross);
            break;
        }
        case HIGHLIGHTED_REMOVED: {
            mode = NON_DXF_FORMAT_PDMode_EncloseSquareOUTER(DXF_FORMAT_PDMode_CentreCross);
            break;
        }
        case PROJECTED: {
            mode = DXF_FORMAT_PDMode_CentreCross;
            break;
        }
        default:
            mode = DXF_FORMAT_PDMode_CentrePlus;
            break;
    }
    painter->drawPointEntityUI({uiX, uiY}, mode, m_sizePx);
}
