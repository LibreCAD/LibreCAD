/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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


#include "rs_dimradial.h"
//#include "rs_constructionline.h"
#include "rs_text.h"
#include "rs_solid.h"
#include "rs_graphic.h"


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



/**
 * @return Automatically created label for the default 
 * measurement of this dimension.
 */
RS_String RS_DimRadial::getMeasuredLabel() {

    // Definitive dimension line:
    double dist = data.definitionPoint.distanceTo(edata.definitionPoint);

    RS_Graphic* graphic = getGraphic();

    RS_String ret;
    if (graphic!=NULL) {
        ret = RS_Units::formatLinear(dist, graphic->getUnit(),
                                     graphic->getLinearFormat(), graphic->getLinearPrecision());
    } else {
        ret = RS_String("%1").arg(dist);
    }

    return ret;
}


RS_VectorSolutions RS_DimRadial::getRefPoints() {
	RS_VectorSolutions ret(edata.definitionPoint,
						data.definitionPoint, data.middleOfText);
	return ret;
}


/**
 * Updates the sub entities of this dimension. Called when the 
 * dimension or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimRadial::update(bool autoText) {

    RS_DEBUG->print("RS_DimRadial::update");

    clear();

    if (isUndone()) {
        return;
    }

    // dimension line:
    //updateCreateDimensionLine(data.definitionPoint, edata.definitionPoint,
    //false, true);

    RS_Vector p1 = data.definitionPoint;
    RS_Vector p2 = edata.definitionPoint;
    double angle = p1.angleTo(p2);

    // text height (DIMTXT)
    double dimtxt = getTextHeight();
    // text distance to line (DIMGAP)
    double dimgap = getDimensionLineGap();

    // length of dimension line:
    double length = p1.distanceTo(p2);

    RS_TextData textData;

    textData = RS_TextData(RS_Vector(0.0,0.0),
                           dimtxt, 30.0,
                           RS2::VAlignMiddle,
                           RS2::HAlignCenter,
                           RS2::LeftToRight,
                           RS2::Exact,
                           1.0,
                           getLabel(),
                           "standard",
                           0.0);

    RS_Text* text = new RS_Text(this, textData);
    double textWidth = text->getSize().x;

    // do we have to put the arrow / text outside of the arc?
    bool outsideArrow = (length<getArrowSize()*2+textWidth);
    double arrowAngle;

    if (outsideArrow) {
        length += getArrowSize()*2 + textWidth;
        arrowAngle = angle+M_PI;
    } else {
        arrowAngle = angle;
    }

    // create arrow:
    RS_SolidData sd;
    RS_Solid* arrow;

    arrow = new RS_Solid(this, sd);
    arrow->shapeArrow(p2,
                      arrowAngle,
                      getArrowSize());
    arrow->setPen(RS_Pen(RS2::FlagInvalid));
    arrow->setLayer(NULL);
    addEntity(arrow);

    RS_Vector p3;
    p3.setPolar(length, angle);
    p3 += p1;

    // Create dimension line:
    RS_Line* dimensionLine = new RS_Line(this, RS_LineData(p1, p3));
    dimensionLine->setPen(RS_Pen(RS2::FlagInvalid));
    dimensionLine->setLayer(NULL);
    addEntity(dimensionLine);

    RS_Vector distV;
    double textAngle;

    // rotate text so it's readable from the bottom or right (ISO)
    // quadrant 1 & 4
    if (angle>M_PI/2.0*3.0+0.001 ||
            angle<M_PI/2.0+0.001) {

        distV.setPolar(dimgap + dimtxt/2.0, angle+M_PI/2.0);
        textAngle = angle;
    }
    // quadrant 2 & 3
    else {
        distV.setPolar(dimgap + dimtxt/2.0, angle-M_PI/2.0);
        textAngle = angle+M_PI;
    }

    // move text label:
    RS_Vector textPos;

    if (data.middleOfText.valid && !autoText) {
        textPos = data.middleOfText;
    } else {
        if (outsideArrow) {
            textPos.setPolar(length-textWidth/2.0-getArrowSize(), angle);
        } else {
            textPos.setPolar(length/2.0, angle);
        }
        textPos+=p1;
        // move text away from dimension line:
        textPos += distV;
    	data.middleOfText = textPos;
    }

    text->rotate(RS_Vector(0.0,0.0), textAngle);
    text->move(textPos);

    text->setPen(RS_Pen(RS2::FlagInvalid));
    text->setLayer(NULL);
    addEntity(text);

    calculateBorders();
}



void RS_DimRadial::move(RS_Vector offset) {
    RS_Dimension::move(offset);

    edata.definitionPoint.move(offset);
    update();
}



void RS_DimRadial::rotate(RS_Vector center, double angle) {
    RS_Dimension::rotate(center, angle);

    edata.definitionPoint.rotate(center, angle);
    update();
}



void RS_DimRadial::scale(RS_Vector center, RS_Vector factor) {
    RS_Dimension::scale(center, factor);

    edata.definitionPoint.scale(center, factor);
    edata.leader*=factor.x;
    update();
}



void RS_DimRadial::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.definitionPoint.mirror(axisPoint1, axisPoint2);
    update();
}


void RS_DimRadial::moveRef(const RS_Vector& ref, const RS_Vector& offset) {

    if (ref.distanceTo(edata.definitionPoint)<1.0e-4) {
		double d = data.definitionPoint.distanceTo(edata.definitionPoint);
		double a = data.definitionPoint.angleTo(edata.definitionPoint + offset);
		
		RS_Vector v;
		v.setPolar(d, a);
        edata.definitionPoint = data.definitionPoint + v;
		update(true);
    }
	else if (ref.distanceTo(data.middleOfText)<1.0e-4) {
        data.middleOfText.move(offset);
		update(false);
    }
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_DimRadial& d) {
    os << " DimRadial: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}


