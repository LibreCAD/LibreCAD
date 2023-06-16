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
#endif // _MSC_VER > 1000


#include "rs_dimension.h"


struct LC_DimArcData
{
    LC_DimArcData() = default;

    LC_DimArcData(const LC_DimArcData& input_dimArcData);

    LC_DimArcData( double input_radius,
                   double input_arcLength,
                   const RS_Vector& input_centre, 
                   const RS_Vector& input_endAngle, 
                   const RS_Vector& input_startAngle);

    double radius = 0.;
    double arcLength = 0.;

    RS_Vector centre;
    RS_Vector endAngle;
    RS_Vector startAngle;
};


std::ostream& operator << (std::ostream& os, const LC_DimArcData& input_dimArcData);


class LC_DimArc : public RS_Dimension
{
    friend std::ostream& operator << (std::ostream& os, const LC_DimArc& input_dimArc);


    public:

        LC_DimArc( RS_EntityContainer* parent, 
                   const RS_DimensionData& input_commonDimData, 
                   const LC_DimArcData& input_dimArcData);

        RS_Entity* clone() const override;

        RS2::EntityType rtti() const override
        {
            return RS2::EntityDimArc;
        }

        LC_DimArcData getData() const
        {
            return dimArcData;
        }

        double getRadius() const override
        {
            return dimArcData.radius;
        }

        double getArcLength() const
        {
            return dimArcData.arcLength;
        }

        double getStartAngle() const
        {
            return dimArcData.startAngle.angle();
        }

        double getEndAngle() const
        {
            return dimArcData.endAngle.angle();
        }

        RS_Vector getCenter() const override
        {
            return dimArcData.centre;
        }

        QString getMeasuredLabel() override;

        void updateDim(bool autoText = false) override;

        void update() override;

        void move   (const RS_Vector& offset)                                   override;
        void rotate (const RS_Vector& center,     const double& angle)          override;
        void rotate (const RS_Vector& center,     const RS_Vector& angleVector) override;
        void scale  (const RS_Vector& center,     const RS_Vector& factor)      override;
        void mirror (const RS_Vector& axisPoint1, const RS_Vector& axisPoint2)  override;


    protected:

        LC_DimArcData dimArcData;


    private:

        RS_Vector arrowStartPoint;
        RS_Vector arrowEndPoint;

        RS_Vector dimStartPoint;
        RS_Vector dimEndPoint;

        RS_Line* extLine1 = nullptr;
        RS_Line* extLine2 = nullptr;
        RS_Arc*  dimArc1 = nullptr;
        RS_Arc*  dimArc2 = nullptr;

        void calcDimension();

        RS_Vector truncateVector(const RS_Vector input_vector);

        void arrow( const RS_Vector& point, 
                    const double angle, 
                    const double direction, 
                    const RS_Pen& pen);
};

#endif // LC_DIMARC_H

