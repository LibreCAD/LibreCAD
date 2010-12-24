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


#include "rs_dimdiametric.h"
#include "rs_text.h"
#include "rs_solid.h"
#include "rs_graphic.h"
#include "rs_units.h"


/**
 * Constructor.
 *
 * @para parent Parent Entity Container.
 * @para d Common dimension geometrical data.
 * @para ed Extended geometrical data for diametric dimension.
 */
RS_DimDiametric::RS_DimDiametric(RS_EntityContainer* parent,
                           const RS_DimensionData& d,
                           const RS_DimDiametricData& ed)
        : RS_Dimension(parent, d), edata(ed) {

    calculateBorders();
}



/**
 * @return Automatically created label for the default 
 * measurement of this dimension.
 */
RS_String RS_DimDiametric::getMeasuredLabel() {

    // Definitive dimension line:
    double dist = data.definitionPoint.distanceTo(edata.definitionPoint);

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



RS_VectorSolutions RS_DimDiametric::getRefPoints() {
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
void RS_DimDiametric::update(bool autoText) {

    RS_DEBUG->print("RS_DimDiametric::update");

    clear();

	if (isUndone()) {
		return;
	}

    // dimension line:
    updateCreateDimensionLine(data.definitionPoint, edata.definitionPoint,
	true, true, autoText);

    calculateBorders();
}



void RS_DimDiametric::move(RS_Vector offset) {
    RS_Dimension::move(offset);

    edata.definitionPoint.move(offset);
    update();
}



void RS_DimDiametric::rotate(RS_Vector center, double angle) {
    RS_Dimension::rotate(center, angle);

    edata.definitionPoint.rotate(center, angle);
    update();
}



void RS_DimDiametric::scale(RS_Vector center, RS_Vector factor) {
    RS_Dimension::scale(center, factor);

    edata.definitionPoint.scale(center, factor);
	edata.leader*=factor.x;
    update();
}



void RS_DimDiametric::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.definitionPoint.mirror(axisPoint1, axisPoint2);
    update();
}



void RS_DimDiametric::moveRef(const RS_Vector& ref, const RS_Vector& offset) {

    if (ref.distanceTo(edata.definitionPoint)<1.0e-4) {
		RS_Vector c = (edata.definitionPoint + data.definitionPoint)/2.0;
		double d = c.distanceTo(edata.definitionPoint);
		double a = c.angleTo(edata.definitionPoint + offset);
		
		RS_Vector v;
		v.setPolar(d, a);
        edata.definitionPoint = c + v;
		data.definitionPoint = c - v;
		update(true);
    }
    else if (ref.distanceTo(data.definitionPoint)<1.0e-4) {
		RS_Vector c = (edata.definitionPoint + data.definitionPoint)/2.0;
		double d = c.distanceTo(data.definitionPoint);
		double a = c.angleTo(data.definitionPoint + offset);
		
		RS_Vector v;
		v.setPolar(d, a);
        data.definitionPoint = c + v;
		edata.definitionPoint = c - v;
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
std::ostream& operator << (std::ostream& os, const RS_DimDiametric& d) {
    os << " DimDiametric: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}

