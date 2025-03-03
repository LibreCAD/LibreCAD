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

#include <cmath>
#include <iostream>

#include "rs_debug.h"
#include "rs_dimradial.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_mtext.h"
#include "rs_solid.h"
#include "rs_units.h"
#include "rs_settings.h"

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

    // fixme - try to read settings once during action lifecycle
    if (!LC_GET_ONE_BOOL("Appearance", "UnitlessGrid", true) ) {
        dist = RS_Units::convert(dist);
    }

    RS_Graphic* graphic = getGraphic();

    QString ret;
    if (graphic) {
        int dimlunit = getGraphicVariableInt("$DIMLUNIT", 2);
        int dimdec = getGraphicVariableInt("$DIMDEC", 4);
        int dimzin = getGraphicVariableInt("$DIMZIN", 1);
        RS2::LinearFormat format = graphic->getLinearFormat(dimlunit);
        ret = RS_Units::formatLinear(dist, getGraphicUnit(), format, dimdec);
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
void RS_DimRadial::updateDim(bool autoText)
{

    RS_DEBUG->print("RS_DimRadial::update");

    clear();

    if (isUndone()) return;

    const RS_Vector p1 = data.definitionPoint;
    const RS_Vector p2 = edata.definitionPoint;

    const RS_Pen pen(getDimensionLineColor(), getDimensionLineWidth(), RS2::LineByBlock);

    const RS_MTextData textData = RS_MTextData( RS_Vector(0.0,0.0), 
                                                getTextHeight() * getGeneralScale(), 
                                                30.0, 
                                                RS_MTextData::VAMiddle, 
                                                RS_MTextData::HACenter, 
                                                RS_MTextData::LeftToRight, 
                                                RS_MTextData::Exact, 
                                                1.0, 
                                                getLabel(), 
                                                getTextStyle(), 
                                                0.0);

    RS_MText* text = new RS_MText(this, textData);

    const double textWidth   = text->getSize().x;
    const double arrow_size  = getArrowSize() * getGeneralScale();
    const double line_length = p1.distanceTo(p2);
    const double line_angle  = p1.angleTo(p2);
    if (line_length < ((arrow_size * 2.0) + textWidth))
    {
        const RS_Vector p1b = p1 + RS_Vector::polar(line_length, line_angle);


        // Create dimension line 1:
        RS_Line* dimensionLine_1 = new RS_Line{this, p1, p1b};
        dimensionLine_1->setPen(pen);
        dimensionLine_1->setLayer(nullptr);
        addEntity(dimensionLine_1);

        const double extended_line_length = line_length + (arrow_size * 2) + textWidth;

        const RS_Vector p3 = p1 + RS_Vector::polar(extended_line_length, line_angle);

        updateCreateDimensionLine(p1b, p3, true, false, autoText);
    }
    else
    {
        const RS_Vector p3 = p1 + RS_Vector::polar(line_length, line_angle);

        updateCreateDimensionLine(p1, p3, false, true, autoText);
    }

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
