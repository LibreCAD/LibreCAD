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
#include "rs_dimdiametric.h"
#include "rs_mtext.h"
#include "rs_solid.h"
#include "rs_graphic.h"
#include "rs_units.h"
#include "rs_debug.h"

RS_DimDiametricData::RS_DimDiametricData():
	definitionPoint(false),
	leader(0.0)
{}

/**
 * Constructor with initialisation.
 *
 * @param definitionPoint Definition point of the diametric dimension.
 * @param leader Leader length.
 */
RS_DimDiametricData::RS_DimDiametricData(const RS_Vector& _definitionPoint,
				 double _leader):
	definitionPoint(_definitionPoint)
	,leader(_leader)
{
}

std::ostream& operator << (std::ostream& os,
								  const RS_DimDiametricData& dd) {
	os << "(" << dd.definitionPoint << "," << dd.leader << ")";
	return os;
}

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

RS_Entity* RS_DimDiametric::clone() const {
	RS_DimDiametric* d = new RS_DimDiametric(*this);
	d->setOwner(isOwner());
	d->initId();
	d->detach();
	return d;
}

/**
 * @return Automatically created label for the default
 * measurement of this dimension.
 */
QString RS_DimDiametric::getMeasuredLabel() {

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
    }
    else {
        ret = QString("%1").arg(dist);
    }

    return ret;
}



RS_VectorSolutions RS_DimDiametric::getRefPoints() const
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
void RS_DimDiametric::updateDim(bool autoText) {

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



void RS_DimDiametric::move(const RS_Vector& offset) {
    RS_Dimension::move(offset);

    edata.definitionPoint.move(offset);
    update();
}



void RS_DimDiametric::rotate(const RS_Vector& center, const double& angle) {
    rotate(center,RS_Vector(angle));
}

void RS_DimDiametric::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_Dimension::rotate(center, angleVector);

    edata.definitionPoint.rotate(center, angleVector);
    update();
}


void RS_DimDiametric::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_Dimension::scale(center, factor);

    edata.definitionPoint.scale(center, factor);
        edata.leader*=factor.x;
    update();
}



void RS_DimDiametric::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    edata.definitionPoint.mirror(axisPoint1, axisPoint2);
    update();
}



void RS_DimDiametric::moveRef(const RS_Vector& ref, const RS_Vector& offset) {

    if (ref.distanceTo(edata.definitionPoint)<1.0e-4) {
				RS_Vector c = (edata.definitionPoint + data.definitionPoint)/2.0;
                double d = c.distanceTo(edata.definitionPoint);
                double a = c.angleTo(edata.definitionPoint + offset);

				RS_Vector v = RS_Vector::polar(d, a);
        edata.definitionPoint = c + v;
				data.definitionPoint = c - v;
                updateDim(true);
    }
	else if (ref.distanceTo(data.definitionPoint)<1.0e-4) {
				RS_Vector c = (edata.definitionPoint + data.definitionPoint)/2.0;
				double d = c.distanceTo(data.definitionPoint);
				double a = c.angleTo(data.definitionPoint + offset);

				RS_Vector v = RS_Vector::polar(d, a);
		data.definitionPoint = c + v;
                edata.definitionPoint = c - v;
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
std::ostream& operator << (std::ostream& os, const RS_DimDiametric& d) {
    os << " DimDiametric: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}
