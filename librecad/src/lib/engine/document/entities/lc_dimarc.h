/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo <carlo.melwyn@outlook.com>
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#ifndef LC_DIMARC_H
#define LC_DIMARC_H

#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif

#include "rs_dimension.h"

class RS_Arc;
// fixme - sand - no assignment operator!
struct LC_DimArcData {
    LC_DimArcData() = default;
    LC_DimArcData(const LC_DimArcData& other);
    LC_DimArcData(double radius,
                  double arcLength,
                  const RS_Vector& centre,
                  const RS_Vector& endAngle,
                  const RS_Vector& startAngle);
    RS_Vector centre;
    RS_Vector endAngle;
    RS_Vector startAngle;
    double radius = 0.;
    double arcLength = 0.;
};

std::ostream& operator <<(std::ostream& os, const LC_DimArcData& dimArc);

// fixme - sand - no copy assignment operator!
class LC_DimArc : public RS_Dimension {
    friend std::ostream& operator <<(std::ostream& os, const LC_DimArc& dimArc);
public:
    LC_DimArc(const LC_DimArc& other);
    LC_DimArc(RS_EntityContainer* parent,
              const RS_DimensionData& commonDimData,
              const LC_DimArcData& dimArcData);

    RS_Entity* clone() const override;

    RS2::EntityType rtti() const override {
        return RS2::EntityDimArc;
    }

    LC_DimArcData getData() const {
        return m_dimArcData;
    }

    double getRadius() const override {
        return m_dimArcData.radius;
    }

    double getArcLength() const {
        return m_dimArcData.arcLength;
    }

    double getStartAngle() const {
        return m_dimArcData.startAngle.angle();
    }

    double getEndAngle() const {
        return m_dimArcData.endAngle.angle();
    }

    RS_Vector getCenter() const override {
        return m_dimArcData.centre;
    }

    void setCenter(const RS_Vector& v) {
        m_dimArcData.centre = v;
    }

    QString getMeasuredLabel() override;

    void update() override;

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
protected:
    LC_DimArcData m_dimArcData;
    void doUpdateDim() override;
private:
    RS_Vector m_arrowStartPoint;
    RS_Vector m_arrowEndPoint;
    RS_Vector m_dimStartPoint;
    RS_Vector m_dimEndPoint;
    RS_Line* m_extLine1 = nullptr;
    RS_Line* m_extLine2 = nullptr;
    RS_Arc* m_dimArc1 = nullptr;
    RS_Arc* m_dimArc2 = nullptr;

    void calcDimension();
    RS_Vector truncateVector(const RS_Vector& v);
    void arrow(const RS_Vector& point, double angle, double direction,
               const RS_Pen& pen);
};

#endif
