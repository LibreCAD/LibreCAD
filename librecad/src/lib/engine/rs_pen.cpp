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

#include "rs_pen.h"

#include <iostream>

RS_Pen::RS_Pen() :
    RS_Pen(RS_Color{Qt::black}, RS2::WidthByLayer, RS2::LineByLayer)
{}

std::ostream& operator << (std::ostream& os, const RS_Pen& p) {
    //os << "style: " << p.style << std::endl;
    os << " pen color: " << p.getColor()
    << " pen width: " << p.getWidth()
    << " pen screen width: " << p.getScreenWidth()
    << " pen line type: " << p.getLineType()
    << " flags: " << (p.getFlag(RS2::FlagInvalid) ? "INVALID" : "")
    << std::endl;
    return os;
}
