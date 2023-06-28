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


#include <cmath>
#include <iostream>

#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_solid.h"
#include "rs_units.h"

#include "lc_dimarc.h"


LC_DimArcData::LC_DimArcData(const LC_DimArcData &input_dimArcData)
    :
      radius     (input_dimArcData.radius),
      arcLength  (input_dimArcData.arcLength),
      centre     (input_dimArcData.centre),
      endAngle   (input_dimArcData.endAngle),
      startAngle (input_dimArcData.startAngle)
{
}


LC_DimArcData::LC_DimArcData( double input_radius,
                              double input_arcLength,
                              const RS_Vector& input_centre,
                              const RS_Vector& input_endAngle,
                              const RS_Vector& input_startAngle)
    :
      radius     (input_radius),
      arcLength  (input_arcLength),
      centre     (input_centre),
      endAngle   (input_endAngle),
      startAngle (input_startAngle)
{
}


LC_DimArc::LC_DimArc( RS_EntityContainer* parent, 
                      const RS_DimensionData& input_commonDimData,
                      const LC_DimArcData& input_dimArcData)
    :
      RS_Dimension (parent, input_commonDimData),
      dimArcData (input_dimArcData)
{
    update();
}


RS_Entity* LC_DimArc::clone() const
{
    LC_DimArc *cloned_dimArc_entity { new LC_DimArc(*this) };

    cloned_dimArc_entity->setOwner(isOwner());
    cloned_dimArc_entity->initId();
    cloned_dimArc_entity->detach();
    cloned_dimArc_entity->update();

    return cloned_dimArc_entity;
}


QString LC_DimArc::getMeasuredLabel()
{
    RS_Graphic* currentGraphic = getGraphic();

    QString measuredLabel;

    if (currentGraphic)
    {
        const int dimlunit { getGraphicVariableInt (QStringLiteral("$DIMLUNIT"), 2) };
        const int dimdec   { getGraphicVariableInt (QStringLiteral("$DIMDEC"),   4) };
        const int dimzin   { getGraphicVariableInt (QStringLiteral("$DIMZIN"),   1) };

        RS2::LinearFormat format = currentGraphic->getLinearFormat(dimlunit);

        measuredLabel = RS_Units::formatLinear(dimArcData.arcLength, getGraphicUnit(), format, dimdec);

        if (format == RS2::Decimal) measuredLabel = stripZerosLinear(measuredLabel, dimzin);

        if ((format == RS2::Decimal) || (format == RS2::ArchitecturalMetric))
        {
            if (getGraphicVariableInt("$DIMDSEP", 0) == 44) measuredLabel.replace(QChar('.'), QChar(','));
        }
    }
    else
    {
        measuredLabel = QString("%1").arg(dimArcData.arcLength);
    }

    return measuredLabel;
}


void LC_DimArc::arrow( const RS_Vector& point, 
                       const double angle, 
                       const double direction, 
                       const RS_Pen& pen)
{
    if ((getTickSize() * getGeneralScale()) < 0.01)
    {
        double endAngle { 0.0 };

        if (dimArcData.radius > RS_TOLERANCE_ANGLE) endAngle = getArrowSize() / dimArcData.radius;

        const RS_Vector arrowEnd = RS_Vector::polar(dimArcData.radius, angle + std::copysign(endAngle, direction)) 
                                 + dimArcData.centre;

        const double arrowAngle { arrowEnd.angleTo(point) };

        RS_SolidData dummyVar;

        RS_Solid* arrow = new RS_Solid(this, dummyVar);
        arrow->shapeArrow(point, arrowAngle, getArrowSize());
        arrow->setPen( pen);
        arrow->setLayer(nullptr);
        addEntity(arrow);
    }
    else
    {
        const double deg45 = M_PI_2 / 2.0;

        const double midAngle = (dimArcData.startAngle.angle() + dimArcData.endAngle.angle()) / 2.0;

        const RS_Vector tickVector = RS_Vector::polar(getTickSize() * getGeneralScale(), midAngle - deg45);

        RS_Line* tick = new RS_Line(this, point - tickVector, point + tickVector);
        tick->setPen(pen);
        tick->setLayer(nullptr);
        addEntity(tick);
    }
}


void LC_DimArc::updateDim(bool autoText /* = false */)
{
    Q_UNUSED (autoText)

    RS_DEBUG->print("LC_DimArc::update");

    clear();

    if (isUndone()) return;

    if ( ! dimArcData.centre.valid) return;

    calcDimension();

    RS_Pen pen (getExtensionLineColor(), getExtensionLineWidth(), RS2::LineByBlock);

    pen.setWidth (getDimensionLineWidth());
    pen.setColor (getDimensionLineColor());

    extLine1->setPen (pen);
    extLine2->setPen (pen);

    extLine1->setLayer (nullptr);
    extLine2->setLayer (nullptr);

    addEntity (extLine1);
    addEntity (extLine2);

    RS_Arc* refArc
    {
        new RS_Arc( this, 
                    RS_ArcData( dimArcData.centre, 
                                dimArcData.radius, 
                                dimArcData.startAngle.angle(), 
                                dimArcData.endAngle.angle(), 
                                false) 
                  ) 
    };

    arrow (arrowStartPoint, dimArcData.startAngle.angle(), +1.0, pen);
    arrow (arrowEndPoint,   dimArcData.endAngle.angle(),   -1.0, pen);

    double textAngle  { 0.0 };

    RS_Vector textPos { refArc->getMiddlePoint() };

    const double textAngle_preliminary { std::trunc((textPos.angleTo(dimArcData.centre) - M_PI) * 1.0E+10) * 1.0E-10 };

    if ( ! this->getInsideHorizontalText())
    {
        RS_Vector textPosOffset;

        const double deg360 { M_PI * 2.0 };

        const double degTolerance { 1.0E-3 };

        /* With regards to Quadrants #1 and #2 */
        if (((textAngle_preliminary >= -degTolerance) && (textAngle_preliminary <= (M_PI + degTolerance))) 
        ||  ((textAngle_preliminary <= -(M_PI - degTolerance)) && (textAngle_preliminary >= -(deg360 + degTolerance))))
        {
            textPosOffset.setPolar (getDimensionLineGap(), textAngle_preliminary);
            textAngle = textAngle_preliminary + M_PI + M_PI_2;
        }
        /* With regards to Quadrants #3 and #4 */
        else
        {
            textPosOffset.setPolar (getDimensionLineGap(), textAngle_preliminary + M_PI);
            textAngle = textAngle_preliminary + M_PI_2;
        }
    }

    QString dimLabel { getLabel() };

    bool ok;

    const double dummyVar = dimLabel.toDouble(&ok);

    if (dummyVar) { /* This is a dummy code, to suppress the unused variable compiler warning. */ }

    if (ok) dimLabel.prepend("∩ ");

    RS_MTextData textData
    {
        RS_MTextData( textPos, 
                      getTextHeight(), 
                      30.0, 
                      RS_MTextData::VABottom, 
                      RS_MTextData::HACenter, 
                      RS_MTextData::LeftToRight, 
                      RS_MTextData::Exact, 
                      1.0, 
                      dimLabel, 
                      QString("unicode"), 
                      textAngle) 
    };

    RS_MText* text { new RS_MText (this, textData) };

    text->setPen (RS_Pen (getTextColor(), RS2::WidthByBlock, RS2::SolidLine));
    text->setLayer (nullptr);
    addEntity (text);

    double halfWidth_plusGap  = (text->getUsedTextWidth() / 2.0) + getDimensionLineGap();
    double halfHeight_plusGap = (getTextHeight()          / 2.0) + getDimensionLineGap();

    text->move(-RS_Vector::polar(getTextHeight() / 2.0, textAngle + M_PI_2));

    /* Text rectangle's corners : top left, top right, bottom right, bottom left. */
    RS_Vector textRectCorners [4] = 
    {
        RS_Vector(false), 
        RS_Vector(textPos + RS_Vector(+halfWidth_plusGap, +halfHeight_plusGap)), 
        RS_Vector(false), 
        RS_Vector(textPos + RS_Vector(-halfWidth_plusGap, -halfHeight_plusGap))
    };

    RS_Vector cornerTopRight   { textRectCorners [1] };
    RS_Vector cornerBottomLeft { textRectCorners [3] };

    textRectCorners [0] = RS_Vector(cornerBottomLeft.x, cornerTopRight.y);
    textRectCorners [2] = RS_Vector(cornerTopRight.x,   cornerBottomLeft.y);

    for (int i = 0; i < 4; i++)
    {
        textRectCorners[i].rotate(textPos, text->getAngle());
        textRectCorners[i].x = std::trunc(textRectCorners[i].x * 1.0E+4) * 1.0E-4;
        textRectCorners[i].y = std::trunc(textRectCorners[i].y * 1.0E+4) * 1.0E-4;
    }

    if (RS_DEBUG->getLevel() == RS_Debug::D_INFORMATIONAL)
    {
        std::cout << std::endl 
                  << " LC_DimArc::updateDim: Text position / angle : " << textPos << " / " << text->getAngle() 
                  << std::endl;

        std::cout << std::endl 
                  << " LC_DimArc::updateDim: Reference arc middle point : " << refArc->getMiddlePoint() 
                  << std::endl;

        std::cout << std::endl 
                  << " LC_DimArc::updateDim: DimArc-1 start point : " << dimArc1->getStartpoint() 
                  << std::endl;

        std::cout << std::endl 
                  << " LC_DimArc::updateDim: DimArc-2 start point : " << dimArc2->getStartpoint() 
                  << std::endl;

        std::cout << std::endl 
                  << " LC_DimArc::updateDim: Text rectangle corners : " << textRectCorners [0] << ", " 
                                                                        << textRectCorners [1] << ", " 
                                                                        << textRectCorners [2] << ", " 
                                                                        << textRectCorners [3] 
                  << std::endl;
    }

    const double cornerLeftX
    {
        std::min({textRectCorners[0].x, textRectCorners[1].x, textRectCorners[2].x, textRectCorners[3].x})
    };

    const double cornerRightX
    {
        std::max({textRectCorners[0].x, textRectCorners[1].x, textRectCorners[2].x, textRectCorners[3].x})
    };

    const double cornerBottomY
    {
        std::min({textRectCorners[0].y, textRectCorners[1].y, textRectCorners[2].y, textRectCorners[3].y})
    };

    const double cornerTopY
    {
        std::max({textRectCorners[0].y, textRectCorners[1].y, textRectCorners[2].y, textRectCorners[3].y})
    };

    constexpr double deltaOffset { 1.0E-2 };

    while (((dimArc1->getEndpoint().x < cornerLeftX)   || (dimArc1->getEndpoint().x > cornerRightX) 
    ||      (dimArc1->getEndpoint().y < cornerBottomY) || (dimArc1->getEndpoint().y > cornerTopY))

    &&     (dimArc1->getAngle2() < RS_MAXDOUBLE) 
    &&     (dimArc1->getAngle2() > RS_MINDOUBLE))
    {
        dimArc1->setAngle2(dimArc1->getAngle2() + deltaOffset);
    }

    while (((dimArc2->getStartpoint().x < cornerLeftX)   || (dimArc2->getStartpoint().x > cornerRightX) 
    ||      (dimArc2->getStartpoint().y < cornerBottomY) || (dimArc2->getStartpoint().y > cornerTopY)) 

    &&     (dimArc2->getAngle1() < RS_MAXDOUBLE) 
    &&     (dimArc2->getAngle1() > RS_MINDOUBLE))
    {
        dimArc2->setAngle1(dimArc2->getAngle1() - deltaOffset);
    }

    dimArc1->setPen (pen);
    dimArc2->setPen (pen);

    dimArc1->setLayer (nullptr);
    dimArc2->setLayer (nullptr);

    addEntity (dimArc1);
    addEntity (dimArc2);

    calculateBorders();
}


void LC_DimArc::update()
{
    updateDim();
    RS_Dimension::update();
}


void LC_DimArc::move(const RS_Vector& offset)
{
    RS_Dimension::move (offset);

    dimArcData.centre.move (offset);

    update();
}


void LC_DimArc::rotate(const RS_Vector& center, const double& angle)
{
    rotate (center, RS_Vector (angle));

    update();
}


void LC_DimArc::rotate(const RS_Vector& center, const RS_Vector& angleVector)
{
    RS_Dimension::rotate (center, angleVector);

    dimArcData.centre.rotate (center, angleVector);

    const double arcDeltaAngle { dimArcData.startAngle.angleTo(dimArcData.endAngle) };

    dimArcData.startAngle = RS_Vector(data.definitionPoint.angleTo(dimArcData.centre) - M_PI);

    dimArcData.endAngle = dimArcData.startAngle;

	dimArcData.endAngle.rotate (arcDeltaAngle);

    update();
}


void LC_DimArc::scale(const RS_Vector& center, const RS_Vector& factor)
{
    const double adjustedFactor = factor.x < factor.y 
                                ? factor.x 
                                : factor.y;

    const RS_Vector adjustedFactorVector(adjustedFactor, adjustedFactor);

    RS_Dimension::scale (center, adjustedFactorVector);

    dimArcData.centre.scale (center, adjustedFactorVector);

    dimArcData.radius *= adjustedFactor;

    update();
}


void LC_DimArc::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2)
{
    const RS_Vector previousDefinitionPoint = data.definitionPoint;

    RS_Dimension::mirror (axisPoint1, axisPoint2);

    dimArcData.centre.mirror     (axisPoint1, axisPoint2);

    /*
        // Just another way of accomplishing the operation below this comment.

        dimStartPoint.mirror (axisPoint1, axisPoint2);
        dimEndPoint.mirror   (axisPoint1, axisPoint2);

        dimArcData.startAngle = RS_Vector((dimStartPoint - dimArcData.centre).angle() - M_PI);
        dimArcData.endAngle   = RS_Vector((dimEndPoint   - dimArcData.centre).angle() - M_PI);
    */

    const RS_Vector originPoint(0.0, 0.0);

    const RS_Vector deltaAxisPoints = axisPoint2 - axisPoint1;

    /* The minus one (-1) value denotes that mirroring changes direction (and hence, sign). */
    dimArcData.startAngle.setPolar (-1.0, dimArcData.startAngle.angle());
    dimArcData.endAngle.setPolar   (-1.0, dimArcData.endAngle.angle());

    dimArcData.startAngle.mirror (originPoint, deltaAxisPoints);
    dimArcData.endAngle.mirror   (originPoint, deltaAxisPoints);

    update();
}


RS_Vector LC_DimArc::truncateVector(const RS_Vector input_vector)
{
    return RS_Vector( std::trunc(input_vector.x * 1.0E+10) * 1.0E-10, 
                      std::trunc(input_vector.y * 1.0E+10) * 1.0E-10, 
                      0.0);
}


void LC_DimArc::calcDimension()
{
    const double endAngle   = dimArcData.endAngle.angle();
    const double startAngle = dimArcData.startAngle.angle();

    dimArc1 = new RS_Arc (this, RS_ArcData(dimArcData.centre, dimArcData.radius, startAngle, startAngle, false));
    dimArc2 = new RS_Arc (this, RS_ArcData(dimArcData.centre, dimArcData.radius, endAngle,   endAngle, false));

    RS_Vector entityStartPoint = truncateVector(data.definitionPoint);

    const double entityRadius  = dimArcData.centre.distanceTo(entityStartPoint);

    RS_Vector entityEndPoint   = truncateVector(dimArcData.centre 
                               + RS_Vector(dimArcData.endAngle).scale(entityRadius));

    dimStartPoint = dimArcData.centre 
                  + RS_Vector(dimArcData.startAngle).scale(dimArcData.radius);

    dimEndPoint   = dimArcData.centre 
                  + RS_Vector(dimArcData.endAngle).scale(dimArcData.radius);

    arrowStartPoint = dimStartPoint;
    arrowEndPoint   = dimEndPoint;

    entityStartPoint += RS_Vector::polar (getExtensionLineOffset(),    entityStartPoint.angleTo(dimStartPoint));
    entityEndPoint   += RS_Vector::polar (getExtensionLineOffset(),    entityEndPoint.angleTo(dimEndPoint));
    dimStartPoint    += RS_Vector::polar (getExtensionLineExtension(), entityStartPoint.angleTo(dimStartPoint));
    dimEndPoint      += RS_Vector::polar (getExtensionLineExtension(), entityEndPoint.angleTo(dimEndPoint));

    extLine1 = new RS_Line (this, entityStartPoint, dimStartPoint);
    extLine2 = new RS_Line (this, entityEndPoint,   dimEndPoint);

    /* RS_DEBUG->setLevel(RS_Debug::D_INFORMATIONAL); */

    RS_DEBUG->print( RS_Debug::D_INFORMATIONAL, 
                     "\n LC_DimArc::calcDimension: Start / end angles : %lf / %lf\n", 
                     startAngle, endAngle);

    RS_DEBUG->print( RS_Debug::D_INFORMATIONAL, 
                     "\n LC_DimArc::calcDimension: Dimension / entity radii : %lf / %lf\n", 
                     dimArcData.radius, entityRadius);

    if (RS_DEBUG->getLevel() == RS_Debug::D_INFORMATIONAL)
    {
        std::cout << std::endl 
                  << " LC_DimArc::calcDimension: Start Points : " << entityStartPoint << " to " << dimStartPoint 
                  << std::endl;

        std::cout << std::endl 
                  << " LC_DimArc::calcDimension: End Points : " << entityEndPoint << " to " << dimEndPoint 
                  << std::endl;
    }
}


std::ostream& operator << (std::ostream& os, const LC_DimArc& input_dimArc)
{
    os << " DimArc Information : \n" 
       << input_dimArc.getData() << std::endl << std::endl;

    return os;
}


std::ostream& operator << (std::ostream& os, const LC_DimArcData& input_dimArcData)
{
    os << " {\n\tCentre      : " << input_dimArcData.centre 
       <<   "\n\tRadius      : " << input_dimArcData.radius 
       <<   "\n\tStart Angle : " << input_dimArcData.startAngle 
       <<   "\n\tEnd   Angle : " << input_dimArcData.endAngle 
       <<   "\n}"                << std::endl << std::endl;

    return os;
}
