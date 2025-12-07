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
#include "rs_dimradial.h"

#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_units.h"

RS_DimRadialData::RS_DimRadialData():
	definitionPoint(false),
	leader(0.0) {
}

/**
 * Constructor with initialisation.
 *
 * @param definitionPoint Definition point of the radial dimension.
 * @param leader Leader length.
 */
RS_DimRadialData::RS_DimRadialData(const RS_Vector& _definitionPoint, double _leader):
    definitionPoint(_definitionPoint)
    , leader(_leader) {
}

std::ostream& operator <<(std::ostream& os,const RS_DimRadialData& dd) {
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
RS_DimRadial::RS_DimRadial(RS_EntityContainer* parent, const RS_DimensionData& d, const RS_DimRadialData& ed)
    : RS_Dimension(parent, d), m_dimRadialData(ed) {
}

RS_DimRadial::RS_DimRadial(const RS_DimRadial& other)
    :RS_Dimension(other), m_dimRadialData{other.m_dimRadialData} {
}

RS_Entity* RS_DimRadial::clone() const {
    auto* d = new RS_DimRadial(*this);
	return d;
}

/**
 * @return Automatically created label for the default
 * measurement of this dimension.
 */
QString RS_DimRadial::getMeasuredLabel() {
    // Definitive dimension line:
    double distance = m_dimGenericData.definitionPoint.distanceTo(m_dimRadialData.definitionPoint);

    double dist = prepareLabelLinearDistance(distance);
    QString measuredLabel =  createLinearMeasuredLabel(dist);
    return measuredLabel;
}

RS_VectorSolutions RS_DimRadial::getRefPoints() const {
    return RS_VectorSolutions({
        m_dimRadialData.definitionPoint,
        m_dimGenericData.definitionPoint, m_dimGenericData.middleOfText
    });
}

/**
 * Updates the sub entities of this dimension. Called when the
 * dimension or the position, alignment, .. changes.
 *
 * @param autoText Automatically reposition the text label
 */
void RS_DimRadial::doUpdateDim() {
    RS_DEBUG->print("RS_DimRadial::update");

    const RS_Vector p1 = m_dimGenericData.definitionPoint;
    const RS_Vector p2 = m_dimRadialData.definitionPoint;

    // temporary text used to measure the width of the text
    auto textData = createDimTextData({0,0},getTextHeight() * getGeneralScale(), 0.0);
    RS_MText text(this, textData);

    const double textWidth = text.getUsedTextWidth();

    const double arrow_size = getArrowSize() * getGeneralScale();
    const double line_length = p1.distanceTo(p2);
    const double line_angle = p1.angleTo(p2);
    double doubleArrowSize = arrow_size * 2.0;
    if (line_length < (doubleArrowSize + textWidth)) {
        const RS_Vector p1b = p1 + RS_Vector::polar(line_length, line_angle);

        // Create outer dimension line 1:
        addDimDimensionLine(p1, p1b);

        const double extended_line_length = line_length + doubleArrowSize + textWidth;
        const RS_Vector p3 = p1 + RS_Vector::polar(extended_line_length, line_angle);
        createDimensionLine(p1b, p3, true, false, true, false, m_dimGenericData.autoText);
    }
    else {
        const RS_Vector p3 = p1 + RS_Vector::polar(line_length, line_angle);
        createDimensionLine(p1, p3, false, true, false, true, m_dimGenericData.autoText);
    }
}

void RS_DimRadial::move(const RS_Vector& offset) {
    RS_Dimension::move(offset);

    m_dimRadialData.definitionPoint.move(offset);
    update();
}

void RS_DimRadial::rotate(const RS_Vector& center, double angle) {
    rotate(center,RS_Vector(angle));
}

void RS_DimRadial::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_Dimension::rotate(center, angleVector);
    m_dimRadialData.definitionPoint.rotate(center, angleVector);
    update();
}

void RS_DimRadial::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_Dimension::scale(center, factor);

    m_dimRadialData.definitionPoint.scale(center, factor);
    m_dimRadialData.leader*=factor.x;
    update();
}

void RS_DimRadial::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    m_dimRadialData.definitionPoint.mirror(axisPoint1, axisPoint2);
    update();
}

void RS_DimRadial::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(m_dimRadialData.definitionPoint) < 1.0e-4) {
        double d = m_dimGenericData.definitionPoint.distanceTo(m_dimRadialData.definitionPoint);
        double a = m_dimGenericData.definitionPoint.angleTo(m_dimRadialData.definitionPoint + offset);

        RS_Vector v = RS_Vector::polar(d, a);
        m_dimRadialData.definitionPoint = m_dimGenericData.definitionPoint + v;
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
std::ostream& operator << (std::ostream& os, const RS_DimRadial& d) {
    os << " DimRadial: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}
