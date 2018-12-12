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

#include<iostream>
#include<cmath>
#include "rs_dimlinear.h"
#include "rs_line.h"
#include "rs_constructionline.h"
#include "rs_mtext.h"
#include "rs_solid.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_debug.h"


RS_DimLinearData::RS_DimLinearData():
	extensionPoint1(false),
	extensionPoint2(false),
	angle(0.0),
	oblique(0.0)
{}

RS_DimLinearData::RS_DimLinearData(const RS_Vector& _extensionPoint1,
				 const RS_Vector& _extensionPoint2,
				 double _angle, double _oblique):
	extensionPoint1(_extensionPoint1)
	,extensionPoint2(_extensionPoint2)
	,angle(_angle)
	,oblique(_oblique)
{
}

std::ostream& operator << (std::ostream& os,
								  const RS_DimLinearData& dd) {
	os << "(" << dd.extensionPoint1 << ","
	   << dd.extensionPoint1 <<','
	   << dd.angle <<','
	   << dd.oblique <<','
		  <<")";
	return os;
}

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

RS_Entity* RS_DimLinear::clone() const {
	RS_DimLinear* d = new RS_DimLinear(*this);
	d->setOwner(isOwner());
	d->initId();
	d->detach();
	return d;
}

RS_VectorSolutions RS_DimLinear::getRefPoints() const
{
		return RS_VectorSolutions({edata.extensionPoint1, edata.extensionPoint2,
												data.definitionPoint, data.middleOfText});
}
void RS_DimLinear::setAngle(double a) {
	edata.angle = RS_Math::correctAngle(a);
}


/**
 * @return Automatically created label for the default
 * measurement of this dimension.
 */
QString RS_DimLinear::getMeasuredLabel() {
    // direction of dimension line
	RS_Vector dirDim = RS_Vector::polar(100.0, edata.angle);

    // construction line for dimension line
	RS_ConstructionLine dimLine(nullptr,
								RS_ConstructionLineData(data.definitionPoint,
														data.definitionPoint + dirDim));

    RS_Vector dimP1 = dimLine.getNearestPointOnEntity(edata.extensionPoint1);
    RS_Vector dimP2 = dimLine.getNearestPointOnEntity(edata.extensionPoint2);

    // Definitive dimension line:
    double dist = dimP1.distanceTo(dimP2) * getGeneralFactor();

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
        }
        else {
        ret = QString("%1").arg(dist);
        }

    return ret;
}



bool RS_DimLinear::hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) {
        return (edata.extensionPoint1.isInWindow(v1, v2) ||
                edata.extensionPoint2.isInWindow(v1, v2));
}



/**
 * Updates the sub entities of this dimension. Called when the
 * text or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimLinear::updateDim(bool autoText) {

    RS_DEBUG->print("RS_DimLinear::update");

    clear();

    if (isUndone()) {
        return;
    }

    // general scale (DIMSCALE)
    double dimscale = getGeneralScale();
    // distance from entities (DIMEXO)
    double dimexo = getExtensionLineOffset()*dimscale;
    // extension line extension (DIMEXE)
    double dimexe = getExtensionLineExtension()*dimscale;

    // direction of dimension line
	RS_Vector dirDim = RS_Vector::polar(100.0, edata.angle);

    // construction line for dimension line
    RS_ConstructionLine dimLine(
		nullptr,
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

    double extAngle1, extAngle2;

    if ((edata.extensionPoint1-dimP1).magnitude()<1e-6) {
        if ((edata.extensionPoint2-dimP2).magnitude()<1e-6) {
            //boot extension points are in dimension line only rotate 90
			extAngle2 = edata.angle + (M_PI_2);
        } else {
            //first extension point are in dimension line use second
            extAngle2 = edata.extensionPoint2.angleTo(dimP2);
        }
            extAngle1 = extAngle2;
    } else {
        //first extension point not are in dimension line use it
        extAngle1 = edata.extensionPoint1.angleTo(dimP1);
        if ((edata.extensionPoint2-dimP2).magnitude()<1e-6)
            extAngle2 = extAngle1;
        else
            extAngle2 = edata.extensionPoint2.angleTo(dimP2);
    }

    RS_Vector vDimexe1 = RS_Vector::polar(dimexe, extAngle1);
    RS_Vector vDimexe2 = RS_Vector::polar(dimexe, extAngle2);

    RS_Vector vDimexo1, vDimexo2;
    if (getFixedLengthOn()){
        double dimfxl = getFixedLength()*dimscale;
        double extLength = (edata.extensionPoint1-dimP1).magnitude();
        if (extLength-dimexo > dimfxl)
            vDimexo1.setPolar(extLength - dimfxl, extAngle1);
        extLength = (edata.extensionPoint2-dimP2).magnitude();
        if (extLength-dimexo > dimfxl)
            vDimexo2.setPolar(extLength - dimfxl, extAngle2);
    } else {
        vDimexo1.setPolar(dimexo, extAngle1);
        vDimexo2.setPolar(dimexo, extAngle2);
    }

    RS_Pen pen(getExtensionLineColor(),
           getExtensionLineWidth(),
           RS2::LineByBlock);

    // extension lines:
	RS_Line* line = new RS_Line{this,
			edata.extensionPoint1+vDimexo1, dimP1+vDimexe1};
    line->setPen(pen);
//    line->setPen(RS_Pen(RS2::FlagInvalid));
	line->setLayer(nullptr);
    addEntity(line);
    //data.definitionPoint+vDimexe2);
	line = new RS_Line{this,
			edata.extensionPoint2+vDimexo2, dimP2+vDimexe2};
    line->setPen(pen);
//    line->setPen(RS_Pen(RS2::FlagInvalid));
	line->setLayer(nullptr);
    addEntity(line);

    calculateBorders();
}

void RS_DimLinear::move(const RS_Vector& offset) {
    RS_Dimension::move(offset);

    edata.extensionPoint1.move(offset);
    edata.extensionPoint2.move(offset);
    update();
}



void RS_DimLinear::rotate(const RS_Vector& center, const double& angle) {
    RS_Vector angleVector(angle);
    RS_Dimension::rotate(center, angleVector);

    edata.extensionPoint1.rotate(center, angleVector);
    edata.extensionPoint2.rotate(center, angleVector);
    edata.angle = RS_Math::correctAngle(edata.angle+angle);
    update();
}


void RS_DimLinear::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_Dimension::rotate(center, angleVector);

    edata.extensionPoint1.rotate(center, angleVector);
    edata.extensionPoint2.rotate(center, angleVector);
    edata.angle = RS_Math::correctAngle(edata.angle+angleVector.angle());
    update();
}



void RS_DimLinear::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_Dimension::scale(center, factor);

    edata.extensionPoint1.scale(center, factor);
    edata.extensionPoint2.scale(center, factor);
    update();
}



void RS_DimLinear::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.extensionPoint1.mirror(axisPoint1, axisPoint2);
    edata.extensionPoint2.mirror(axisPoint1, axisPoint2);

    RS_Vector vec;
    vec.setPolar(1.0, edata.angle);
    vec.mirror(RS_Vector(0.0,0.0), axisPoint2-axisPoint1);
    edata.angle = vec.angle();

    update();
}



void RS_DimLinear::stretch(const RS_Vector& firstCorner,
                           const RS_Vector& secondCorner,
                           const RS_Vector& offset) {

    //e->calculateBorders();
    if (getMin().isInWindow(firstCorner, secondCorner) &&
            getMax().isInWindow(firstCorner, secondCorner)) {

        move(offset);
    } else {
        //RS_Vector v = data.definitionPoint - edata.extensionPoint2;
        //double len = edata.extensionPoint2.distanceTo(data.definitionPoint);
        //double ang1 = edata.extensionPoint1.angleTo(edata.extensionPoint2)
		//              + M_PI_2;

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
					  + M_PI_2;

        double diff = RS_Math::getAngleDifference(ang1, ang2);
        if (diff>M_PI) {
            diff-=2*M_PI;
        }

		if (fabs(diff)>M_PI_2) {
            ang2 = RS_Math::correctAngle(ang2+M_PI);
        }

        RS_Vector v;
        v.setPolar(len, ang2);
        data.definitionPoint = edata.extensionPoint2 + v;
                */
    }
    updateDim(true);
}



void RS_DimLinear::moveRef(const RS_Vector& ref, const RS_Vector& offset) {

	if (ref.distanceTo(data.definitionPoint)<1.0e-4) {
		data.definitionPoint += offset;
                updateDim(true);
    }
        else if (ref.distanceTo(data.middleOfText)<1.0e-4) {
        data.middleOfText += offset;
                updateDim(false);
    }
        else if (ref.distanceTo(edata.extensionPoint1)<1.0e-4) {
        edata.extensionPoint1 += offset;
                updateDim(true);
    }
        else if (ref.distanceTo(edata.extensionPoint2)<1.0e-4) {
        edata.extensionPoint2 += offset;
                updateDim(true);
    }
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_DimLinear& d) {
    os << " DimLinear: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}
