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

#include "lc_formatter.h"

#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_units.h"

/**
 * Formatting double value
 * @param x
 * @return
 */
QString LC_Formatter::formatDouble(const double x) const{
    QString result =  RS_Units::formatDecimal(x, RS2::Unit::None, m_linearPrecision, false);
    return result;
}

/**
 * formatting int value
 * @param x
 * @return
 */
QString LC_Formatter::formatInt(const int x) const{
    QString result;
    result.setNum(x);
    return result;
}

/**
 * Performs formatting of given vector according to units specified by drawing preferences
 * @param wcsPos vector
 * @return  formatted string
 */
QString LC_Formatter::formatWCSVector(const RS_Vector &wcsPos) const{
    if (m_viewport == nullptr) {
        return "";
    }
    double ucsX, ucsY;
    m_viewport->toUCS(wcsPos, ucsX, ucsY);

    const QString x = RS_Units::formatLinear(ucsX, m_unit, m_linearFormat, m_linearPrecision);
    const QString y = RS_Units::formatLinear(ucsY, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x % ", " % y;
    return result;
}

QString LC_Formatter::formatUCSVector(const RS_Vector &ucsPos) const{
    const QString x = RS_Units::formatLinear(ucsPos.x, m_unit, m_linearFormat, m_linearPrecision);
    const QString y = RS_Units::formatLinear(ucsPos.y, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x % ", " % y;
    return result;
}

QString LC_Formatter::formatWCSDeltaVector(const RS_Vector &wcsDelta) const{
    if (m_viewport == nullptr) {
        return "";
    }
    double ucsX, ucsY;
    m_viewport->toUCSDelta(wcsDelta, ucsX, ucsY);

    const QString x = RS_Units::formatLinear(ucsX, m_unit, m_linearFormat, m_linearPrecision);
    const QString y = RS_Units::formatLinear(ucsY, m_unit, m_linearFormat, m_linearPrecision);

    QString result = x % ", " % y;
    return result;
}

/**
 * performs formatting of angle value according to settings of drawing preferences
 * @param wcsAngle
 * @return
 */
QString LC_Formatter::formatWCSAngle(const double wcsAngle) const {
    if (m_viewport == nullptr) {
        return "";
    }
    double ucsAngle = wcsAngle;
    if (m_viewport->hasUCS()){
        ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    }
    const double ucsRelAngle = m_viewport->toUCSBasisAngle(ucsAngle, m_anglesBase, m_anglesCounterClockWise);
    return formatRawAngle(ucsRelAngle);
}

QString LC_Formatter::formatWCSAngleDegrees(double wcsAngle) const {
    if (m_viewport->hasUCS()){
        wcsAngle = m_viewport->toUCSAngle(wcsAngle);
    }
    const double ucsRelAngle = m_viewport->toUCSBasisAngle(wcsAngle, m_anglesBase, m_anglesCounterClockWise);
    QString result = RS_Units::formatAngle(ucsRelAngle, RS2::AngleFormat::DegreesDecimal, m_anglePrecision);
    return result;
}

QString LC_Formatter::formatUCSAngle(const double wcsAngle) const {
    const double ucsRelAngle = m_viewport->toUCSBasisAngle(wcsAngle, m_anglesBase, m_anglesCounterClockWise);
    return formatRawAngle(ucsRelAngle);
}

QString LC_Formatter::formatRawAngle(const double angle) const {
    QString result =  RS_Units::formatAngle(angle, m_angleFormat, m_anglePrecision);
    return result;
}

QString LC_Formatter::formatRawAngle(const double angle, const RS2::AngleFormat format) const {
    QString result =  RS_Units::formatAngle(angle, format, m_anglePrecision);
    return result;
}

/**
 * formats linear value according to settings of drawing preferences
 * @param length
 * @return
 */
QString LC_Formatter::formatLinear(const double length) const {
    QString result = RS_Units::formatLinear(length,  m_unit,  m_linearFormat,m_linearPrecision);
    return result;
}

void LC_Formatter::updateByGraphic(const RS_Graphic* graphic){
    if (graphic != nullptr) {
        m_unit = graphic->getUnit();
        m_linearFormat = graphic->getLinearFormat();
        m_linearPrecision = graphic->getLinearPrecision();
        m_angleFormat = graphic->getAngleFormat();
        m_anglePrecision = graphic->getAnglePrecision();

        m_anglesBase = graphic->getAnglesBase();
        m_anglesCounterClockWise = graphic->areAnglesCounterClockWise();
    }
}

double LC_Formatter::toUCSBasisAngleDegrees(const double wcsAngle) const{
    const double ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    const double ucsBasisAngle = m_viewport->toUCSBasisAngle(ucsAngle, m_anglesBase, m_anglesCounterClockWise);
    const double result = RS_Math::rad2deg(ucsBasisAngle);
    return result;
}

double LC_Formatter::toWorldAngleFromUCSBasisDegrees(const double ucsBasisAngleDegrees) const{
    const double ucsBasisAngle = RS_Math::deg2rad(ucsBasisAngleDegrees);
    const double ucsAngle = m_viewport->toUCSAbsAngle(ucsBasisAngle, m_anglesBase, m_anglesCounterClockWise);
    const double wcsAngle = m_viewport->toWorldAngle(ucsAngle);
    return wcsAngle;
}

double LC_Formatter::toWorldAngleFromUCSBasis(const double ucsBasisAngle) const{
    const double ucsAngle = m_viewport->toUCSAbsAngle(ucsBasisAngle, m_anglesBase, m_anglesCounterClockWise);
    const double wcsAngle = m_viewport->toWorldAngle(ucsAngle);
    return wcsAngle;
}

double LC_Formatter::toUCSBasisAngle(const double wcsAngle) const{
    const double ucsAngle = m_viewport->toUCSAngle(wcsAngle);
    const double ucsBasisAngle = m_viewport->toUCSBasisAngle(ucsAngle, m_anglesBase, m_anglesCounterClockWise);
    return ucsBasisAngle;
}

double LC_Formatter::toUCSBasisAngleFromUCS(const double ucsAbsAngle) const {
    const double ucsBasisAngle = m_viewport->toUCSBasisAngle(ucsAbsAngle, m_anglesBase, m_anglesCounterClockWise);
    return ucsBasisAngle;
}

double LC_Formatter::toUCSAbsAngleFromUCSBasis(const double ucsBasisAngle) const {
    const double ucsAbsAngle = m_viewport->toUCSAbsAngle(ucsBasisAngle, m_anglesBase, m_anglesCounterClockWise);
    return ucsAbsAngle;
}

double LC_Formatter::adjustRelativeAngleSignByBasis(const double relativeAngle) const{
    double result;
    if (m_anglesCounterClockWise){
        result = relativeAngle;
    }
    else{
        result = -relativeAngle;
    }
    return result;
}

bool LC_Formatter::hasNonDefaultAnglesBasis() const {
    return  LC_LineMath::isMeaningfulAngle(m_anglesBase) || !m_anglesCounterClockWise;
}

QString LC_Formatter::linearUnitAsString() const {
    return RS_Units::unitToString(m_unit);
}

double LC_Formatter::getAnglesBase() const {
    return m_anglesBase;
}

bool LC_Formatter::isAnglesCounterClockWise() const {
    return m_anglesCounterClockWise;
}
