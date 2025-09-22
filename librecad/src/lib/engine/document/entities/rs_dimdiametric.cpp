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

#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_settings.h"
#include "rs_units.h"

RS_DimDiametricData::RS_DimDiametricData():
	definitionPoint(false),
	leader(0.0) {
}

RS_DimDiametricData::RS_DimDiametricData(const RS_DimDiametricData& other) :
    definitionPoint{other.definitionPoint}, leader{other.leader} {
}

/**
 * Constructor with initialisation.
 *
 * @param definitionPoint Definition point of the diametric dimension.
 * @param leader Leader length.
 */
RS_DimDiametricData::RS_DimDiametricData(const RS_Vector& _definitionPoint,
				 double _leader):
	definitionPoint(_definitionPoint)
	,leader(_leader){
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
        : RS_Dimension(parent, d), m_dimDiametricData(ed) {
}

RS_DimDiametric::RS_DimDiametric(const RS_DimDiametric& other)
    : RS_Dimension(other), m_dimDiametricData(other.m_dimDiametricData) {
}

RS_Entity* RS_DimDiametric::clone() const {
    auto* d = new RS_DimDiametric(*this);
	return d;
}

/**
 * @return Automatically created label for the default
 * measurement of this dimension.
 */
QString RS_DimDiametric::getMeasuredLabel() {
    // Definitive dimension line:
 	double distance = m_dimGenericData.definitionPoint.distanceTo(m_dimDiametricData.definitionPoint) * getGeneralFactor();
    double dist = prepareLabelLinearDistance(distance);
    QString measuredLabel =  createLinearMeasuredLabel(dist);
    return measuredLabel;
}

RS_VectorSolutions RS_DimDiametric::getRefPoints() const {
    return RS_VectorSolutions({
        m_dimDiametricData.definitionPoint,
        m_dimGenericData.definitionPoint, m_dimGenericData.middleOfText
    });
}

/**
 * Updates the sub entities of this dimension. Called when the
 * dimension or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimDiametric::doUpdateDim() {
    RS_DEBUG->print("RS_DimDiametric::update");
    // dimension line:
    // fixme - sand - rework diametric dimension
    createDimensionLine(m_dimGenericData.definitionPoint, m_dimDiametricData.definitionPoint,
                              true, true, true, true, m_dimGenericData.autoText);
}

void RS_DimDiametric::move(const RS_Vector& offset) {
    RS_Dimension::move(offset);

    m_dimDiametricData.definitionPoint.move(offset);
    update();
}

void RS_DimDiametric::rotate(const RS_Vector& center, double angle) {
    rotate(center,RS_Vector(angle));
}

void RS_DimDiametric::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_Dimension::rotate(center, angleVector);

    m_dimDiametricData.definitionPoint.rotate(center, angleVector);
    update();
}

void RS_DimDiametric::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_Dimension::scale(center, factor);

    m_dimDiametricData.definitionPoint.scale(center, factor);
        m_dimDiametricData.leader*=factor.x;
    update();
}

void RS_DimDiametric::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    m_dimDiametricData.definitionPoint.mirror(axisPoint1, axisPoint2);
    update();
}

void RS_DimDiametric::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(m_dimDiametricData.definitionPoint) < 1.0e-4) {
        RS_Vector c = (m_dimDiametricData.definitionPoint + m_dimGenericData.definitionPoint) / 2.0;
        double d = c.distanceTo(m_dimDiametricData.definitionPoint);
        double a = c.angleTo(m_dimDiametricData.definitionPoint + offset);

        RS_Vector v = RS_Vector::polar(d, a);
        m_dimDiametricData.definitionPoint = c + v;
        m_dimGenericData.definitionPoint = c - v;
        updateDim(true);
    }
    else if (ref.distanceTo(m_dimGenericData.definitionPoint) < 1.0e-4) {
        RS_Vector c = (m_dimDiametricData.definitionPoint + m_dimGenericData.definitionPoint) / 2.0;
        double d = c.distanceTo(m_dimGenericData.definitionPoint);
        double a = c.angleTo(m_dimGenericData.definitionPoint + offset);

        RS_Vector v = RS_Vector::polar(d, a);
        m_dimGenericData.definitionPoint = c + v;
        m_dimDiametricData.definitionPoint = c - v;
        updateDim(true);
    }
    else if (ref.distanceTo(m_dimGenericData.middleOfText) < 1.0e-4) {
        m_dimGenericData.middleOfText.move(offset);
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
