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


#include "rs_dimaligned.h"

#include "rs_graphic.h"
#include "rs_units.h"
#include "rs_constructionline.h"


/**
 * Constructor.
 *
 * @para parent Parent Entity Container.
 * @para d Common dimension geometrical data.
 * @para ed Extended geometrical data for aligned dimension.
 */
RS_DimAligned::RS_DimAligned(RS_EntityContainer* parent,
                             const RS_DimensionData& d,
                             const RS_DimAlignedData& ed)
        : RS_Dimension(parent, d), edata(ed) {

    calculateBorders();
}



/**
 * Sets a new text. The entities representing the 
 * text are updated.
 */
//void RS_DimAligned::setText(const RS_String& t) {
//    data.text = t;
//    update();
//}


RS_VectorSolutions RS_DimAligned::getRefPoints() {
	RS_VectorSolutions ret(edata.extensionPoint1, edata.extensionPoint2,
						data.definitionPoint, data.middleOfText);
	return ret;
}



/**
 * @return Automatically creted label for the default 
 * measurement of this dimension.
 */
QString RS_DimAligned::getMeasuredLabel() {
    double dist = edata.extensionPoint1.distanceTo(edata.extensionPoint2);

	RS_Graphic* graphic = getGraphic();

    RS_String ret;
	if (graphic!=NULL) {
		ret = RS_Units::formatLinear(dist, graphic->getUnit(), 
			graphic->getLinearFormat(), graphic->getLinearPrecision());
	}
	else {
    	ret = RS_String("%1").arg(dist);
	}
    return ret;
}



/**
 * Updates the sub entities of this dimension. Called when the 
 * text or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimAligned::update(bool autoText) {

    RS_DEBUG->print("RS_DimAligned::update");

    clear();

	if (isUndone()) {
		return;
	}

    // distance from entities (DIMEXO)
    double dimexo = getExtensionLineOffset();
    // definition line definition (DIMEXE)
    double dimexe = getExtensionLineExtension();
    // text height (DIMTXT)
    //double dimtxt = getTextHeight();
    // text distance to line (DIMGAP)
    //double dimgap = getDimensionLineGap();

    // Angle from extension endpoints towards dimension line
    double extAngle = edata.extensionPoint2.angleTo(data.definitionPoint);
    // extension lines length
    double extLength = edata.extensionPoint2.distanceTo(data.definitionPoint);

    RS_Vector v1, v2, e1;
    RS_LineData ld;
    RS_Line* line;

    v1.setPolar(dimexo, extAngle);
    v2.setPolar(dimexe, extAngle);
    e1.setPolar(1.0, extAngle);

    // Extension line 1:
    ld = RS_LineData(edata.extensionPoint1 + v1,
                     edata.extensionPoint1 + e1*extLength + v2);
    line = new RS_Line(this, ld);
    //line->setLayerToActive();
    //line->setPenToActive();
    line->setPen(RS_Pen(RS2::FlagInvalid));
    line->setLayer(NULL);
    addEntity(line);

    // Extension line 2:
    ld = RS_LineData(edata.extensionPoint2 + v1,
                     edata.extensionPoint2 + e1*extLength + v2);
    line = new RS_Line(this, ld);
    //line->setLayerToActive();
    //line->setPenToActive();
    line->setPen(RS_Pen(RS2::FlagInvalid));
    line->setLayer(NULL);
    addEntity(line);

    // Dimension line:
    updateCreateDimensionLine(edata.extensionPoint1 + e1*extLength,
                              edata.extensionPoint2 + e1*extLength,
							  true, true, autoText);

    calculateBorders();
}



bool RS_DimAligned::hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2) {
	return (edata.extensionPoint1.isInWindow(v1, v2) ||
	        edata.extensionPoint2.isInWindow(v1, v2));
}


void RS_DimAligned::move(RS_Vector offset) {
    RS_Dimension::move(offset);

    edata.extensionPoint1.move(offset);
    edata.extensionPoint2.move(offset);
    update();
}



void RS_DimAligned::rotate(RS_Vector center, double angle) {
    RS_Dimension::rotate(center, angle);

    edata.extensionPoint1.rotate(center, angle);
    edata.extensionPoint2.rotate(center, angle);
    update();
}



void RS_DimAligned::scale(RS_Vector center, RS_Vector factor) {
    RS_Dimension::scale(center, factor);

    edata.extensionPoint1.scale(center, factor);
    edata.extensionPoint2.scale(center, factor);
    update();
}


void RS_DimAligned::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.extensionPoint1.mirror(axisPoint1, axisPoint2);
    edata.extensionPoint2.mirror(axisPoint1, axisPoint2);
    update();
}


void RS_DimAligned::stretch(RS_Vector firstCorner,
                        RS_Vector secondCorner,
                        RS_Vector offset) {
						
    //e->calculateBorders();
    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    }
	else {	
		//RS_Vector v = data.definitionPoint - edata.extensionPoint2;
		double len = edata.extensionPoint2.distanceTo(data.definitionPoint);
		double ang1 = edata.extensionPoint1.angleTo(edata.extensionPoint2) 
		             + M_PI/2;
	
    	if (edata.extensionPoint1.isInWindow(firstCorner,
                                      secondCorner)) {
        	edata.extensionPoint1.move(offset);
    	} 
    	if (edata.extensionPoint2.isInWindow(firstCorner,
                                      secondCorner)) {
        	edata.extensionPoint2.move(offset);
    	} 
		
		double ang2 = edata.extensionPoint1.angleTo(edata.extensionPoint2) 
		             + M_PI/2;

		double diff = RS_Math::getAngleDifference(ang1, ang2);
		if (diff>M_PI) {
			diff-=2*M_PI;
		}

		if (fabs(diff)>M_PI/2) {
			ang2 = RS_Math::correctAngle(ang2+M_PI);
		}

		RS_Vector v;
		v.setPolar(len, ang2);
		data.definitionPoint = edata.extensionPoint2 + v;
	}
	update(true);
}



void RS_DimAligned::moveRef(const RS_Vector& ref, const RS_Vector& offset) {

    if (ref.distanceTo(data.definitionPoint)<1.0e-4) {
		RS_ConstructionLine l(NULL, 
			RS_ConstructionLineData(edata.extensionPoint1,
				edata.extensionPoint2));
		double d = l.getDistanceToPoint(data.definitionPoint+offset);
		double a = edata.extensionPoint2.angleTo(data.definitionPoint);
		double ad = RS_Math::getAngleDifference(a, 
			edata.extensionPoint2.angleTo(data.definitionPoint+offset));

		if (fabs(ad)>M_PI/2.0 && fabs(ad)<3.0/2.0*M_PI) {
			a = RS_Math::correctAngle(a+M_PI);
		}
		
		RS_Vector v;
		v.setPolar(d, a);
        data.definitionPoint = edata.extensionPoint2 + v;
		update(true);
    }
	else if (ref.distanceTo(data.middleOfText)<1.0e-4) {
        data.middleOfText.move(offset);
		update(false);
    }
	else if (ref.distanceTo(edata.extensionPoint1)<1.0e-4) {
		double a1 = edata.extensionPoint2.angleTo(edata.extensionPoint1);
		double a2 = edata.extensionPoint2.angleTo(edata.extensionPoint1+offset);
		double d1 = edata.extensionPoint2.distanceTo(edata.extensionPoint1);
		double d2 = edata.extensionPoint2.distanceTo(edata.extensionPoint1+offset);
		rotate(edata.extensionPoint2, a2-a1);
		if (fabs(d1)>1.0e-4) {
			scale(edata.extensionPoint2, RS_Vector(d2/d1, d2/d1));
		}
		update(true);
    }
	else if (ref.distanceTo(edata.extensionPoint2)<1.0e-4) {
		double a1 = edata.extensionPoint1.angleTo(edata.extensionPoint2);
		double a2 = edata.extensionPoint1.angleTo(edata.extensionPoint2+offset);
		double d1 = edata.extensionPoint1.distanceTo(edata.extensionPoint2);
		double d2 = edata.extensionPoint1.distanceTo(edata.extensionPoint2+offset);
		rotate(edata.extensionPoint1, a2-a1);
		if (fabs(d1)>1.0e-4) {
			scale(edata.extensionPoint1, RS_Vector(d2/d1, d2/d1));
		}
		update(true);
    }
}



/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_DimAligned& d) {
    os << " DimAligned: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}


