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

#include <cmath>

#include "lc_convert.h"
#include "lc_graphicviewport.h"
#include "rs_graphicview.h"
#include "rs_math.h"

namespace {
/**
 * RS_Vector::polar() that is exact on the axes: the angle is reduced to a
 * quarter turn first, so 100<-90 is (0, -100) instead of (6.1e-15, -100).
 */
RS_Vector polarPoint(const double distance, const double angle) {
    const double quarters = std::nearbyint(angle / M_PI_2);
    const double rest = angle - quarters * M_PI_2;
    const double along = distance * std::cos(rest);
    const double across = distance * std::sin(rest);
    switch ((static_cast<int>(std::fmod(quarters, 4.)) + 4) % 4) {
        case 1:
            return {-across, along};
        case 2:
            return {-along, -across};
        case 3:
            return {across, -along};
        default:
            return {along, across};
    }
}
}

LC_CoordinatesParser::LC_CoordinatesParser(RS_GraphicView* gview):m_graphicView{gview} {}

/**
 * Reads the syntax of a typed coordinate, without a view:
 *   0 | . | ,               the UCS origin, or the relative zero
 *   [@|!] x , y             Cartesian: absolute, relative or WCS
 *   [@|!] distance < angle  polar: absolute, relative or WCS
 * Whitespace around the prefix and the separator is ignored; each value
 * goes to the expression parser as typed.
 * The first ',' separates Cartesian values; otherwise the first '<'
 * separates polar ones.
 */
LC_CoordinatesParser::Syntax LC_CoordinatesParser::parseSyntax(const QString& input) {
    Syntax syntax;
    const QString text = input.trimmed();
    if (text.isEmpty()) {
        return syntax;
    }

    // quick shortcuts for the absolute and the relative origin
    if (text.length() == 1) {
        switch (text.at(0).toLatin1()) {
            case '0':
                syntax.kind = Syntax::Origin;
                syntax.valid = true;
                break;
            case '.':
            case ',':
                syntax.kind = Syntax::RelativeZero;
                syntax.valid = true;
                break;
            default:
                break;
        }
        return syntax;
    }

    QString body = text;
    if (text.at(0) == '!') {
        syntax.reference = Syntax::Wcs;
        body = text.mid(1).trimmed();
    }
    else if (text.at(0) == '@') {
        syntax.reference = Syntax::Relative;
        body = text.mid(1).trimmed();
    }

    const qsizetype comma = body.indexOf(',');
    const qsizetype less = body.indexOf('<');
    if (comma >= 0) {
        syntax.kind = Syntax::Cartesian;
    }
    else if (less >= 0) {
        syntax.kind = Syntax::Polar;
    }
    else {
        // "!" always starts a coordinate; anything else is a command
        syntax.kind = syntax.reference == Syntax::Wcs ? Syntax::Cartesian : Syntax::None;
        return syntax;
    }

    const qsizetype separator = syntax.kind == Syntax::Cartesian ? comma : less;
    const QString first = body.left(separator).trimmed();
    const QString second = body.mid(separator + 1).trimmed();
    if (first.isEmpty() || second.isEmpty()) {
        return syntax;
    }

    bool ok1 = false;
    bool ok2 = false;
    syntax.first = RS_Math::eval(LC_Convert::updateForFraction(first), &ok1);
    if (syntax.kind == Syntax::Cartesian) {
        syntax.second = RS_Math::eval(LC_Convert::updateForFraction(second), &ok2);
    }
    else {
        syntax.second = LC_Convert::evalAngleValue(second, ok2);
    }
    syntax.valid = ok1 && ok2;
    return syntax;
}

bool LC_CoordinatesParser::isCoordinate(const QString& input) {
    return parseSyntax(input).valid;
}

RS_CoordinateEvent LC_CoordinatesParser::parseCoordinate(const QString& inputStr, bool &stringContainsCoordinate) const {
    const RS_CoordinateEvent invalid(RS_Vector(false));
    const Syntax syntax = parseSyntax(inputStr);
    stringContainsCoordinate = syntax.kind != Syntax::None;
    if (!syntax.valid) {
        return invalid;
    }

    switch (syntax.kind) {
        case Syntax::Origin:
            return RS_CoordinateEvent(toWCS(RS_Vector(0, 0, 0)), true, false);
        case Syntax::RelativeZero:
            return RS_CoordinateEvent(m_graphicView->getViewPort()->getRelativeZero(), false, true);
        case Syntax::Cartesian:
        case Syntax::Polar: {
            RS_Vector point;
            if (syntax.kind == Syntax::Cartesian) {
                point = RS_Vector(syntax.first, syntax.second);
            }
            else {
                // the drawing's angle base and direction apply, in the UCS or, for "!", the WCS
                const double angle = toAbsUCSAngle(RS_Math::deg2rad(syntax.second));
                point = polarPoint(syntax.first, angle);
            }
            switch (syntax.reference) {
                case Syntax::Wcs:
                    return RS_CoordinateEvent(point);
                case Syntax::Relative: {
                    const RS_Vector ucsRelZero = toUCS(m_graphicView->getViewPort()->getRelativeZero());
                    return RS_CoordinateEvent(toWCS(point + ucsRelZero));
                }
                default:
                    return RS_CoordinateEvent(toWCS(point));
            }
        }
        default:
            return invalid;
    }
}

RS_Vector LC_CoordinatesParser::toWCS(const RS_Vector &ucs) const {
    return m_graphicView->getViewPort()->toWorld(ucs);
}

RS_Vector LC_CoordinatesParser::toUCS(const RS_Vector &wcs) const {
    return m_graphicView->getViewPort()->toUCS(wcs);
}

double LC_CoordinatesParser::toAbsUCSAngle(const double ucsBasisAngle) const {
    return m_graphicView->getViewPort()->toAbsUCSAngle(ucsBasisAngle);
}

double LC_CoordinatesParser::toWCSAngle(const double ucsAngle) const {
    return m_graphicView->getViewPort()->toWorldAngle(ucsAngle);
}
