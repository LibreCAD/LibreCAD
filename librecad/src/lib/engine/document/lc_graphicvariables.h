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

#ifndef LC_GRAPHICVARIABLES_H
#define LC_GRAPHICVARIABLES_H
#include <QString>

#include "rs.h"
#include "rs_vector.h"

class RS_Graphic;

class LC_GraphicVariables{
 public:
    LC_GraphicVariables();
    RS2::AngleFormat angleUnitsDXF2LC(int aunits);

    void loadFromVars(RS_Graphic* graphic);
    void saveToVars(RS_Graphic* graphic);
    bool isGridOn() const;
    void setGridOn(bool on);
    bool isIsometricGrid() const;
    void setIsometricGrid(bool on);
    double getAnglesBase() const;
    void setAnglesBase(double baseAngle);
    bool areAnglesCounterClockWise() const;
    void setAnglesCounterClockwise(bool on);
    QString getDefaultDimStyleName();
    void setDefaultDimStyleName(QString name);
    RS2::IsoGridViewType getIsoView() const;
    void setIsoView(RS2::IsoGridViewType viewType);
    RS_Vector getPaperInsertionBase();
    void setPaperInsertionBase(const RS_Vector& p);
    int getAnglePrecision() const;
    int getLinearPrecision() const;
    RS2::AngleFormat getAngleFormat() const;
    RS2::LinearFormat getLinearFormat() const;
    RS2::Unit getUnit() const;
    RS2::LinearFormat convertLinearFormatDXF2LC(int f);
private:
    bool m_gridOn {true};
    bool m_isometricGrid{false};
    double m_anglesBase {0.0};
    bool m_anglesCounterClockWise {true};
    QString m_dimStyle = "Standard";
    RS2::IsoGridViewType m_gridViewType = RS2::IsoGridViewType::IsoTop;
    RS_Vector m_paperInsertionBase;
    int m_anglePrecision {4};
    int m_linearPrecision {4};
    RS2::AngleFormat m_angleFormat{RS2::DegreesDecimal};
    RS2::LinearFormat m_linearFormat {RS2::Decimal};
    RS2::Unit m_unit{RS2::Millimeter};
};

#endif // LC_GRAPHICVARIABLES_H
