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
    /** What a typed coordinate says, before a view places it. */
    struct Syntax {
        enum Kind {None, Origin, RelativeZero, Cartesian, Polar};
        enum Reference {Absolute, Relative, Wcs};
        Kind kind = None;
        Reference reference = Absolute;
        // x and y, or the distance and the angle in degrees
        double first = 0.;
        double second = 0.;
        // the values were read
        bool valid = false;
    };

    explicit LC_CoordinatesParser(RS_GraphicView* gview);
    RS_CoordinateEvent parseCoordinate(const QString& inputStr, bool &stringContainsCoordinate) const;
    // the angle base (in radians) and direction are needed to read a bearing as a direction
    static Syntax parseSyntax(const QString& input, double anglesBase = 0.0, bool anglesCounterClockwise = true);
    /** Whether the input is a coordinate that can be read */
    static bool isCoordinate(const QString& input);
private:
    RS_GraphicView* m_graphicView;
    RS_Vector toWCS(const RS_Vector& ucs) const;
    RS_Vector toUCS(const RS_Vector& wcs) const;
    double toAbsUCSAngle(double ucsBasisAngle) const;
    double toWCSAngle(double ucsAngle) const;
};

#endif
