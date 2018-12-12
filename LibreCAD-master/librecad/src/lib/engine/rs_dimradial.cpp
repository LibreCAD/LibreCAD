/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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

#include <iostream>
#include <cmath>
#include "rs_dimradial.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_solid.h"
#include "rs_graphic.h"
#include "rs_debug.h"

RS_DimRadialData::RS_DimRadialData():
	definitionPoint(false),
	leader(0.0)
{}

/**
 * Constructor with initialisation.
 *
 * @param definitionPoint Definition point of the radial dimension.
 * @param leader Leader length.
 */
RS_DimRadialData::RS_DimRadialData(const RS_Vector& _definitionPoint,
				 double _leader):
	definitionPoint(_definitionPoint)
	,leader(_leader)
{
}

std::ostream& operator << (std::ostream& os,
								  const RS_DimRadialData& dd) {
	os << "(" << dd.definitionPoint << "/" << dd.leader << ")";
	return os;
}

/**
 * Constructor.
 *
 * @para parent Parent Entity Container.
 * @para d Common dimension geometrical data.
 * @para ed Extended geometrical data for radial dimension.
 */
RS_DimRadial::RS_DimRadial(RS_EntityContainer* parent,
                           const RS_DimensionData& d,
                           const RS_DimRadialData& ed)
        : RS_Dimension(parent, d), edata(ed) {}

RS_Entity* RS_DimRadial::clone() const {
	RS_DimRadial* d = new RS_DimRadial(*this);
	d->setOwner(isOwner());
	d->initId();
	d->detach();
	return d;
}


/**
 * @return Automatically created label for the default
 * measurement of this dimension.
 */
QString RS_DimRadial::getMeasuredLabel() {

    // Definitive dimension line:
	double dist = data.definitionPoint.distanceTo(edata.definitionPoint) * getGeneralFactor();

    RS_Graphic* graphic = getGraphic();

    QString ret;
    if (graphic) {
        int dimlunit = getGraphicVariableInt("$DIMLUNIT", 2);
        int dimdec = getGraphicVariableInt("$DIMDEC", 4);
        int dimzin = getGraphicVariableInt("$DIMZIN", 1);
        RS2::LinearFormat format = graphic->getLinearFormat(dimlunit);
        ret = RS_Units::formatLinear(dist, RS2::None, format, dimdec);
        if (format == RS2::Decimal)
            ret = stripZerosLinear(ret, dimzin);
        //verify if units are decimal and comma separator
        if (format == RS2::Decimal || format == RS2::ArchitecturalMetric){
            if (getGraphicVariableInt("$DIMDSEP", 0) == 44)
                ret.replace(QChar('.'), QChar(','));
        }
    } else {
        ret = QString("%1").arg(dist);
    }

    return ret;
}


RS_VectorSolutions RS_DimRadial::getRefPoints() const
{
		return RS_VectorSolutions({edata.definitionPoint,
												data.definitionPoint, data.middleOfText});
}


/**
 * Updates the sub entities of this dimension. Called when the
 * dimension or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimRadial::updateDim(bool autoText) {

    RS_DEBUG->print("RS_DimRadial::update");

    clear();

    if (isUndone()) {
        return;
    }

    // general scale (DIMSCALE)
    double dimscale = getGeneralScale();

	RS_Vector p1 = data.definitionPoint;
    RS_Vector p2 = edata.definitionPoint;
    double angle = p1.angleTo(p2);

    // text height (DIMTXT)
    double dimtxt = getTextHeight()*dimscale;

    RS_Pen pen(getDimensionLineColor(),
           getDimensionLineWidth(),
           RS2::LineByBlock);

    RS_MTextData textData;

    textData = RS_MTextData(RS_Vector(0.0,0.0),
                           dimtxt, 30.0,
                           RS_MTextData::VAMiddle,
                           RS_MTextData::HACenter,
                           RS_MTextData::LeftToRight,
                           RS_MTextData::Exact,
                           1.0,
                           getLabel(),
                           getTextStyle(),
                           0.0);

    RS_MText* text = new RS_MText(this, textData);
    double textWidth = text->getSize().x;

    double tick_size = getTickSize()*dimscale;
    double arrow_size = getArrowSize()*dimscale;
    double length = p1.distanceTo(p2); // line length

    bool outsideArrow = false;

    if (tick_size == 0 && arrow_size != 0)
    {
        // do we have to put the arrow / text outside of the arc?
        outsideArrow = (length < arrow_size*2+textWidth);
        double arrowAngle;

        if (outsideArrow) {
            length += arrow_size*2 + textWidth;
            arrowAngle = angle+M_PI;
        } else {
            arrowAngle = angle;
        }

        // create arrow:
        RS_SolidData sd;
        RS_Solid* arrow;

        arrow = new RS_Solid(this, sd);
        arrow->shapeArrow(p2, arrowAngle, arrow_size);
        arrow->setPen(pen);
        arrow->setLayer(nullptr);
        addEntity(arrow);
    }

	RS_Vector p3 = RS_Vector::polar(length, angle);
    p3 += p1;

    // Create dimension line:
	RS_Line* dimensionLine = new RS_Line{this, p1, p3};
    dimensionLine->setPen(pen);
	dimensionLine->setLayer(nullptr);
    addEntity(dimensionLine);

    RS_Vector distV;
    double textAngle;

    // text distance to line (DIMGAP)
    double dimgap = getDimensionLineGap()*dimscale;

    // rotate text so it's readable from the bottom or right (ISO)
    // quadrant 1 & 4
    if (angle > M_PI_2*3.0+0.001 || angle < M_PI_2+0.001)
    {
		distV.setPolar(dimgap + dimtxt/2.0, angle+M_PI_2);
        textAngle = angle;
    }
    // quadrant 2 & 3
    else
    {
		distV.setPolar(dimgap + dimtxt/2.0, angle-M_PI_2);
        textAngle = angle+M_PI;
    }

    // move text label:
    RS_Vector textPos;

    if (data.middleOfText.valid && !autoText) {
        textPos = data.middleOfText;
    } else {
        if (outsideArrow) {
            textPos.setPolar(length-textWidth/2.0-arrow_size, angle);
        } else {
            textPos.setPolar(length/2.0, angle);
        }
        textPos += p1;
        // move text away from dimension line:
        textPos += distV;
        data.middleOfText = textPos;
    }

	text->rotate({0., 0.}, textAngle);
    text->move(textPos);

    text->setPen(RS_Pen(getTextColor(), RS2::WidthByBlock, RS2::SolidLine));
	text->setLayer(nullptr);
    addEntity(text);

    calculateBorders();
}



void RS_DimRadial::move(const RS_Vector& offset) {
    RS_Dimension::move(offset);

    edata.definitionPoint.move(offset);
    update();
}



void RS_DimRadial::rotate(const RS_Vector& center, const double& angle) {
    rotate(center,RS_Vector(angle));
}


void RS_DimRadial::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_Dimension::rotate(center, angleVector);

    edata.definitionPoint.rotate(center, angleVector);
    update();
}



void RS_DimRadial::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_Dimension::scale(center, factor);

    edata.definitionPoint.scale(center, factor);
    edata.leader*=factor.x;
    update();
}



void RS_DimRadial::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.definitionPoint.mirror(axisPoint1, axisPoint2);
    update();
}


void RS_DimRadial::moveRef(const RS_Vector& ref, const RS_Vector& offset) {

    if (ref.distanceTo(edata.definitionPoint)<1.0e-4) {
				double d = data.definitionPoint.distanceTo(edata.definitionPoint);
				double a = data.definitionPoint.angleTo(edata.definitionPoint + offset);

				RS_Vector v = RS_Vector::polar(d, a);
		edata.definitionPoint = data.definitionPoint + v;
                updateDim(true);
    }
        else if (ref.distanceTo(data.middleOfText)<1.0e-4) {
        data.middleOfText.move(offset);
                updateDim(false);
    }
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_DimRadial& d) {
    os << " DimRadial: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}
