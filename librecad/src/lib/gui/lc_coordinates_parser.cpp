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

#include "lc_coordinates_parser.h"

#include <QString>

#include "lc_convert.h"
#include "lc_graphicviewport.h"
#include "rs_graphicview.h"
#include "rs_math.h"

LC_CoordinatesParser::LC_CoordinatesParser(RS_GraphicView* gview):m_graphicView{gview} {}

RS_CoordinateEvent LC_CoordinatesParser::parseCoordinate(const QString& inputStr, bool &stringContainsCoordinate) {
    RS_CoordinateEvent invalid(RS_Vector(false));

    stringContainsCoordinate = true;

    // handle quick shortcuts for absolute/current origins:
    if (inputStr.length() == 1) {
        switch (inputStr[0].toLatin1()) {
            case '0': {
                RS_Vector ucs0 = RS_Vector(0, 0, 0);
                RS_Vector wcs0 = toWCS(ucs0);
                return RS_CoordinateEvent(wcs0, true, false);
            }
            case '.':
            case ',': {
                RS_Vector wcs0 = m_graphicView->getViewPort()->getRelativeZero();
                return RS_CoordinateEvent(wcs0, false, true);
            }
            default: /* NO OP */
                stringContainsCoordinate = false;
                return invalid;
        }
    }

    bool wcsCoordinates = inputStr.at(0) == '!';
    if (wcsCoordinates) {
        // proceed absolute wcs coordinates
        QString candidate = inputStr.mid(1);
        bool isCartesian = inputStr.contains(',');
        if (isCartesian) {
            qsizetype separatorPos = candidate.indexOf(',');
            bool ok1{false}, ok2{false};
            double x = RS_Math::eval(LC_Convert::updateForFraction(candidate.left(separatorPos)), &ok1);
            double y = RS_Math::eval(LC_Convert::updateForFraction(candidate.mid(separatorPos + 1)), &ok2);
            if (ok1 && ok2) {
                const RS_Vector& wcsCoordinate = RS_Vector(x, y);
                return RS_CoordinateEvent(wcsCoordinate);
            }
            return invalid;
        }
        bool isPolar = candidate.contains('<');
        if (isPolar) {
            // proceed absolute polar coordinates
            qsizetype separatorPos = candidate.indexOf('<');
            bool ok1{false}, ok2{false};
            double distance = RS_Math::eval(LC_Convert::updateForFraction(candidate.left(separatorPos)), &ok1);
            const QString& angleStr = candidate.mid(separatorPos + 1);
            double angleDegrees = LC_Convert::evalAngleValue(angleStr, ok2);
            if (ok1 && ok2) {
                double wcsAngle = RS_Math::deg2rad(angleDegrees); // fixme - sand - check whether angle base should be considered!!
                RS_Vector wcsCoordinate = RS_Vector(distance, wcsAngle);
                return RS_CoordinateEvent(wcsCoordinate);
            }
            return invalid;
        }
        return invalid;
    }
    bool absoluteCoordinates = inputStr.at(0) != '@';
    bool isCartesian = inputStr.contains(',');
    if (isCartesian) {
        qsizetype separatorPos = inputStr.indexOf(',');
        if (absoluteCoordinates) {
            // absolute cartesian coordinates
            bool ok1{false}, ok2{false};
            double x = RS_Math::eval(LC_Convert::updateForFraction(inputStr.left(separatorPos)), &ok1);
            double y = RS_Math::eval(LC_Convert::updateForFraction(inputStr.mid(separatorPos + 1)), &ok2);
            if (ok1 && ok2) {
                const RS_Vector& ucsCoordinate = RS_Vector(x, y);
                RS_Vector wcsCoordinate = toWCS(ucsCoordinate);
                return RS_CoordinateEvent(wcsCoordinate);
            }
            return invalid;
        }
        // relative cartesian coordinates
        bool ok1{false}, ok2{false};
        double x = RS_Math::eval(LC_Convert::updateForFraction(inputStr.mid(1, separatorPos - 1)), &ok1);
        double y = RS_Math::eval(LC_Convert::updateForFraction(inputStr.mid(separatorPos + 1)), &ok2);

        if (ok1 && ok2) {
            const RS_Vector& ucsOffset = RS_Vector(x, y);
            const RS_Vector ucsRelZero = toUCS(m_graphicView->getViewPort()->getRelativeZero());
            const RS_Vector ucsCoordinate = ucsOffset + ucsRelZero;
            const RS_Vector& wcsCoordinate = toWCS(ucsCoordinate);
            return RS_CoordinateEvent(wcsCoordinate);
        }
        return invalid;
    }
    // try to handle polar coordinate input:
    bool isPolar = inputStr.contains('<');
    if (isPolar) {
        qsizetype separatorPos = inputStr.indexOf('<');
        if (absoluteCoordinates) {
            // handle absolute polar coordinate input:
            bool ok1{false}, ok2{false};
            double ucsDistance = RS_Math::eval(LC_Convert::updateForFraction(inputStr.left(separatorPos)), &ok1);
            const QString& angleStr = inputStr.mid(separatorPos + 1);
            double ucsBasisAngleDegrees = LC_Convert::evalAngleValue(angleStr, ok2);

            if (ok1 && ok2) {
                double ucsBasisAngleRad = RS_Math::deg2rad(ucsBasisAngleDegrees);
                double ucsAngle = toAbsUCSAngle(ucsBasisAngleRad);
                RS_Vector ucsCoordinate{RS_Vector::polar(ucsDistance, ucsAngle)};
                RS_Vector wcsCoordinate = toWCS(ucsCoordinate);
                return RS_CoordinateEvent(wcsCoordinate);
            }
            return invalid;
        }
        // handle relative polar coordinate input:
        qsizetype commaPos = inputStr.indexOf('<');
        bool ok1{false}, ok2{false};
        double ucsDistance = RS_Math::eval(LC_Convert::updateForFraction(inputStr.mid(1, commaPos - 1)), &ok1);
        const QString& angleStr = inputStr.mid(commaPos + 1);
        double ucsBasisAngleDegrees = LC_Convert::evalAngleValue(angleStr, ok2);

        if (ok1 && ok2) {
            double ucsBasisAngleRad = RS_Math::deg2rad(ucsBasisAngleDegrees);
            double ucsAngle = toAbsUCSAngle(ucsBasisAngleRad);
            RS_Vector ucsOffset = RS_Vector::polar(ucsDistance, ucsAngle);
            const RS_Vector ucsRelZero = toUCS(m_graphicView->getViewPort()->getRelativeZero());
            const RS_Vector ucsCoordinate = ucsOffset + ucsRelZero;
            const RS_Vector& wcsCoordinate = toWCS(ucsCoordinate);
            return RS_CoordinateEvent(wcsCoordinate);
        }
        return invalid;
    }
    stringContainsCoordinate = false;
    return invalid;
}

RS_Vector LC_CoordinatesParser::toWCS(const RS_Vector &ucs) {
    return m_graphicView->getViewPort()->toWorld(ucs);
}

RS_Vector LC_CoordinatesParser::toUCS(const RS_Vector &wcs) {
    return m_graphicView->getViewPort()->toUCS(wcs);
}

double LC_CoordinatesParser::toAbsUCSAngle(double ucsBasisAngle) {
    return m_graphicView->getViewPort()->toAbsUCSAngle(ucsBasisAngle);
}

double LC_CoordinatesParser::toWCSAngle(double ucsAngle) {
    return m_graphicView->getViewPort()->toWorldAngle(ucsAngle);
}
