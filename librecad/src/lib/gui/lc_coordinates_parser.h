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

#ifndef LC_COORDINATESPARSER_H
#define LC_COORDINATESPARSER_H

#include "rs_coordinateevent.h"
#include "rs_vector.h"

class QString;
class RS_GraphicView;

class LC_CoordinatesParser{
public:
    LC_CoordinatesParser(RS_GraphicView* gview);
    RS_CoordinateEvent parseCoordinate(const QString& inputStr, bool &stringContainsCoordinate);
private:
    RS_GraphicView* m_graphicView;
    RS_Vector toWCS(const RS_Vector& ucs);
    RS_Vector toUCS(const RS_Vector& wcs);
    double toAbsUCSAngle(double ucsBasisAngle);
    double toWCSAngle(double ucsAngle);
};

#endif // LC_COORDINATESPARSER_H
