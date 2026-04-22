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

#ifndef LC_FORMATTER_H
#define LC_FORMATTER_H

#include <QString>

#include "rs.h"

class RS_Graphic;
class LC_GraphicViewport;
class RS_Vector;

class LC_Formatter{
public:
    explicit LC_Formatter(LC_GraphicViewport* viewport): m_viewport{viewport} {}
    virtual ~LC_Formatter() = default;
    RS2::Unit getUnit() const {return m_unit;}
    QString formatWCSVector(const RS_Vector &wcsPos) const;
    QString formatUCSVector(const RS_Vector &ucsPos) const;
    QString formatWCSDeltaVector(const RS_Vector &wcsDelta) const;
    QString formatWCSAngle(double wcsAngle) const;
    QString formatWCSAngleDegrees(double wcsAngle) const;
    QString formatUCSAngle(double wcsAngle) const;
    QString formatRawAngle(double angle) const;
    QString formatRawAngle(double angle, RS2::AngleFormat format) const;
    QString formatLinear(double length) const;
    QString formatDouble(double x) const;
    QString formatInt(int x) const;
    void updateByGraphic(const RS_Graphic* graphic);
    double toUCSBasisAngleDegrees(double wcsAngle) const;
    double toWorldAngleFromUCSBasisDegrees(double ucsBasisAngleDegrees) const;
    double toWorldAngleFromUCSBasis(double ucsBasisAngle) const;
    double toUCSBasisAngle(double wcsAngle) const;
    double toUCSBasisAngleFromUCS(double ucsAbsAngle) const;
    double toUCSAbsAngleFromUCSBasis(double ucsBasisAngle) const;
    double adjustRelativeAngleSignByBasis(double relativeAngle) const;
    bool hasNonDefaultAnglesBasis() const;
    QString linearUnitAsString() const;
    double getAnglesBase() const;
    bool isAnglesCounterClockWise() const;

    static std::string toStdStr(const QString& str) {
        const QByteArray utf8ByteArray = str.toUtf8();
        const auto result = std::string(utf8ByteArray.constData(), utf8ByteArray.size());
        return result;
    }

protected:
    RS2::Unit m_unit {RS2::Millimeter};
    RS2::LinearFormat m_linearFormat {RS2::Decimal};
    int m_linearPrecision {2};

    RS2::AngleFormat m_angleFormat {RS2::DegreesDecimal};
    int m_anglePrecision {2};

    double m_anglesBase = 0;
    bool m_anglesCounterClockWise = true;

    LC_GraphicViewport* m_viewport {nullptr};
};

#endif
