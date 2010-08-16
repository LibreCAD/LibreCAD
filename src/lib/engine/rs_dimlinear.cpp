/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#include "rs_dimlinear.h"
#include "rs_constructionline.h"
#include "rs_text.h"
#include "rs_solid.h"
#include "rs_graphic.h"


/**
 * Constructor.
 *
 * @para parent Parent Entity Container.
 * @para d Common dimension geometrical data.
 * @para ed Extended geometrical data for linear dimension.
 */
RS_DimLinear::RS_DimLinear(RS_EntityContainer* parent,
                           const RS_DimensionData& d,
                           const RS_DimLinearData& ed)
        : RS_Dimension(parent, d), edata(ed) {

    calculateBorders();
}



RS_VectorSolutions RS_DimLinear::getRefPoints() {
	RS_VectorSolutions ret(edata.extensionPoint1, edata.extensionPoint2,
						data.definitionPoint, data.middleOfText);
	return ret;
}


/**
 * @return Automatically created label for the default 
 * measurement of this dimension.
 */
RS_String RS_DimLinear::getMeasuredLabel() {
    // direction of dimension line
    RS_Vector dirDim;
    dirDim.setPolar(100.0, edata.angle);

    // construction line for dimension line
    RS_ConstructionLine dimLine(NULL,
                                RS_ConstructionLineData(data.definitionPoint,
                                                        data.definitionPoint + dirDim));

    RS_Vector dimP1 = dimLine.getNearestPointOnEntity(edata.extensionPoint1);
    RS_Vector dimP2 = dimLine.getNearestPointOnEntity(edata.extensionPoint2);

    // Definitive dimension line:
    double dist = dimP1.distanceTo(dimP2);

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



bool RS_DimLinear::hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2) {
	return (edata.extensionPoint1.isInWindow(v1, v2) ||
	        edata.extensionPoint2.isInWindow(v1, v2));
}



/**
 * Updates the sub entities of this dimension. Called when the 
 * text or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimLinear::update(bool autoText) {

    RS_DEBUG->print("RS_DimLinear::update");

    clear();

	if (isUndone()) {
		return;
	}

    // distance from entities (DIMEXO)
    double dimexo = getExtensionLineOffset();
    // extension line extension (DIMEXE)
    double dimexe = getExtensionLineExtension();

    RS_LineData ld;
    double extAngle = edata.angle + (M_PI/2.0);

    // direction of dimension line
    RS_Vector dirDim;
    dirDim.setPolar(100.0, edata.angle);
    // direction of extension lines
    RS_Vector dirExt;
    dirExt.setPolar(100.0, extAngle);

    // construction line for dimension line
    RS_ConstructionLine dimLine(
        NULL,
        RS_ConstructionLineData(data.definitionPoint,
                                data.definitionPoint + dirDim));

    RS_Vector dimP1 = dimLine.getNearestPointOnEntity(edata.extensionPoint1);
    RS_Vector dimP2 = dimLine.getNearestPointOnEntity(edata.extensionPoint2);

    // Definitive dimension line:
    updateCreateDimensionLine(dimP1, dimP2, true, true, autoText);
    /*
    ld = RS_LineData(data.definitionPoint, dimP1);
    RS_Line* dimensionLine = new RS_Line(this, ld);
       addEntity(dimensionLine);
    */
    RS_Vector vDimexo1, vDimexe1, vDimexo2, vDimexe2;
    vDimexe1.setPolar(dimexe, edata.extensionPoint1.angleTo(dimP1));
    vDimexo1.setPolar(dimexo, edata.extensionPoint1.angleTo(dimP1));

    vDimexe2.setPolar(dimexe, edata.extensionPoint2.angleTo(dimP2));
    vDimexo2.setPolar(dimexo, edata.extensionPoint2.angleTo(dimP2));
	
    if ((edata.extensionPoint1-dimP1).magnitude()<1e-6) {
        vDimexe1.setPolar(dimexe,
                          data.definitionPoint.angleTo(dimP1)-M_PI/2.0);
        vDimexo1.setPolar(dimexo,
                          data.definitionPoint.angleTo(dimP1)-M_PI/2.0);
    }
    if ((edata.extensionPoint2-dimP2).magnitude()<1e-6) {
        vDimexe2.setPolar(dimexe,
                          data.definitionPoint.angleTo(dimP2)-M_PI/2.0);
        vDimexo2.setPolar(dimexo,
                          data.definitionPoint.angleTo(dimP2)-M_PI/2.0);
    }
	
    // extension lines:
    ld = RS_LineData(edata.extensionPoint1+vDimexo1,
                     dimP1+vDimexe1);
    RS_Line* line = new RS_Line(this, ld);
    line->setPen(RS_Pen(RS2::FlagInvalid));
    line->setLayer(NULL);
    addEntity(line);
    ld = RS_LineData(edata.extensionPoint2+vDimexo2,
                     dimP2+vDimexe2);
    //data.definitionPoint+vDimexe2);
    line = new RS_Line(this, ld);
    line->setPen(RS_Pen(RS2::FlagInvalid));
    line->setLayer(NULL);
    addEntity(line);

    calculateBorders();
}



void RS_DimLinear::move(RS_Vector offset) {
    RS_Dimension::move(offset);

    edata.extensionPoint1.move(offset);
    edata.extensionPoint2.move(offset);
    update();
}



void RS_DimLinear::rotate(RS_Vector center, double angle) {
    RS_Dimension::rotate(center, angle);

    edata.extensionPoint1.rotate(center, angle);
    edata.extensionPoint2.rotate(center, angle);
    edata.angle = RS_Math::correctAngle(edata.angle+angle);
    update();
}



void RS_DimLinear::scale(RS_Vector center, RS_Vector factor) {
    RS_Dimension::scale(center, factor);

    edata.extensionPoint1.scale(center, factor);
    edata.extensionPoint2.scale(center, factor);
    update();
}



void RS_DimLinear::mirror(RS_Vector axisPoint1, RS_Vector axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.extensionPoint1.mirror(axisPoint1, axisPoint2);
    edata.extensionPoint2.mirror(axisPoint1, axisPoint2);

    RS_Vector vec;
    vec.setPolar(1.0, edata.angle);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    edata.angle = vec.angle();

    update();
}



void RS_DimLinear::stretch(RS_Vector firstCorner,
                           RS_Vector secondCorner,
                           RS_Vector offset) {

    //e->calculateBorders();
    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    } else {
        //RS_Vector v = data.definitionPoint - edata.extensionPoint2;
        //double len = edata.extensionPoint2.distanceTo(data.definitionPoint);
        //double ang1 = edata.extensionPoint1.angleTo(edata.extensionPoint2)
        //              + M_PI/2;

        if (edata.extensionPoint1.isInWindow(firstCorner,
                                            secondCorner)) {
            edata.extensionPoint1.move(offset);
        }
        if (edata.extensionPoint2.isInWindow(firstCorner,
                                            secondCorner)) {
            edata.extensionPoint2.move(offset);
        }

		/*
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
		*/
    }
    update(true);
}



void RS_DimLinear::moveRef(const RS_Vector& ref, const RS_Vector& offset) {

    if (ref.distanceTo(data.definitionPoint)<1.0e-4) {
        data.definitionPoint += offset;
		update(true);
    }
	else if (ref.distanceTo(data.middleOfText)<1.0e-4) {
        data.middleOfText += offset;
		update(false);
    }
	else if (ref.distanceTo(edata.extensionPoint1)<1.0e-4) {
        edata.extensionPoint1 += offset;
		update(true);
    }
	else if (ref.distanceTo(edata.extensionPoint2)<1.0e-4) {
        edata.extensionPoint2 += offset;
		update(true);
    }
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_DimLinear& d) {
    os << " DimLinear: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}

