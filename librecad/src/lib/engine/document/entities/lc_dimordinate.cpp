/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */
#include<iostream>
#include "lc_dimordinate.h"

#include "lc_linemath.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_text.h"

LC_DimOrdinateData::~LC_DimOrdinateData() = default;

std::ostream& operator << (std::ostream& os,
                           const LC_DimOrdinateData& dd) {
    os << "(" << dd.featurePoint << "," << dd.leaderEndPoint<<")";
    return os;
}

LC_DimOrdinate::LC_DimOrdinate(RS_EntityContainer* parent, const RS_DimensionData& d, const LC_DimOrdinateData& ed):
   RS_Dimension{parent, d}, m_dimOrdinateData{ed}{
   RS_EntityContainer::calculateBorders();
}

LC_DimOrdinate::LC_DimOrdinate(const LC_DimOrdinate& other)
    :RS_Dimension(other), m_dimOrdinateData{other.m_dimOrdinateData} {
}

RS_Entity* LC_DimOrdinate::clone() const {
    auto* d = new LC_DimOrdinate(*this);
    return d;
}

RS_VectorSolutions LC_DimOrdinate::getRefPoints() const {
    return RS_VectorSolutions({m_dimGenericData.definitionPoint, m_dimOrdinateData.featurePoint, m_dimOrdinateData.leaderEndPoint, m_dimGenericData.middleOfText});
}

RS_Vector LC_DimOrdinate::getFeaturePoint() const {
    return m_dimOrdinateData.featurePoint;
}

RS_Vector LC_DimOrdinate::getLeaderEndPoint() const{
    return m_dimOrdinateData.leaderEndPoint;
}

void LC_DimOrdinate::move(const RS_Vector& offset) {
    RS_Dimension::move(offset);
    m_dimOrdinateData.featurePoint.move(offset);
    m_dimOrdinateData.leaderEndPoint.move(offset);
    update();
}

void LC_DimOrdinate::rotate(const RS_Vector& center, double angle) {
    RS_Vector angleVector(angle);
    RS_Dimension::rotate(center, angleVector);

    m_dimOrdinateData.featurePoint.rotate(center, angleVector);
    m_dimOrdinateData.leaderEndPoint.rotate(center, angleVector);
    update();
}

void LC_DimOrdinate::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    RS_Dimension::rotate(center, angleVector);

    m_dimOrdinateData.featurePoint.rotate(center, angleVector);
    m_dimOrdinateData.leaderEndPoint.rotate(center, angleVector);
    update();
}

void LC_DimOrdinate::scale(const RS_Vector& center, const RS_Vector& factor) {
    RS_Dimension::scale(center, factor);

    m_dimOrdinateData.featurePoint.scale(center, factor);
    m_dimOrdinateData.leaderEndPoint.scale(center, factor);
    update();
}

void LC_DimOrdinate::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    RS_Dimension::mirror(axisPoint1, axisPoint2);

    m_dimOrdinateData.featurePoint.mirror(axisPoint1, axisPoint2);
    m_dimOrdinateData.leaderEndPoint.mirror(axisPoint1, axisPoint2);

    update();
}

void LC_DimOrdinate::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    if (ref.distanceTo(m_dimGenericData.definitionPoint)<1.0e-4) {
        m_dimGenericData.definitionPoint += offset;
        updateDim(true);
    }
    else if (ref.distanceTo(m_dimGenericData.middleOfText)<1.0e-4) {
        m_dimGenericData.middleOfText += offset;
        updateDim(false);
    }
    else if (ref.distanceTo(m_dimOrdinateData.featurePoint)<1.0e-4) {
        m_dimOrdinateData.featurePoint += offset;
        updateDim(true);
    }
    else if (ref.distanceTo(m_dimOrdinateData.leaderEndPoint)<1.0e-4) {
        m_dimOrdinateData.leaderEndPoint += offset;
        updateDim(true);
    }
}

std::ostream& operator<<(std::ostream& os, const LC_DimOrdinate& d) {
    os << " DimOrdinate: " << d.getData() << "\n" << d.getEData() << "\n";
    return os;
}

void LC_DimOrdinate::determineKneesPositions(const RS_Vector& featurePoint, const RS_Vector& leaderEndPoint,
                                             RS_Vector& kneeOne, RS_Vector& kneeTwo, RS_Vector& textOffsetV) {
    bool xAxisRotatedInUCS = LC_LineMath::isMeaningfulAngle(m_dimGenericData.horizontalAxisDirection);
    bool inXDirection = m_dimOrdinateData.ordinateForX;
    double legSize = getArrowSize()*2; // fixme - sand - play with factors, settings?
    double doubleLeg = legSize*2; // fixme - sand - play with factors, settings?
    double featurePointY = featurePoint.y;
    double featurePointX = featurePoint.x;
    double leaderPointX = leaderEndPoint.x;
    double leaderPointY = leaderEndPoint.y;
    if (inXDirection) {
        kneeOne.x = featurePointX;
        kneeTwo.x = leaderPointX;

        if (featurePointY < leaderPointY) {
            kneeOne.y = leaderPointY - doubleLeg;
            double featureYPlusLeg = featurePointY + legSize;
            if (kneeOne.y < featureYPlusLeg) {
                kneeOne.y = featureYPlusLeg;
            }

            kneeTwo.y = leaderPointY - legSize;
            textOffsetV = RS_Vector(0,1);
        }
        else {
            kneeOne.y = leaderPointY + doubleLeg;
            double featureYMinusLeg = featurePointY - legSize;
            if (kneeOne.y > featureYMinusLeg) {
                kneeOne.y = featureYMinusLeg;
            }

            kneeTwo.y = leaderPointY + legSize;
            textOffsetV = RS_Vector(0,-1);
        }
    }
    else {// horizontal, measuring Y
        kneeOne.y = featurePointY;
        kneeTwo.y = leaderPointY;

        if (featurePointX < leaderPointX) {
            kneeOne.x = leaderPointX - doubleLeg;
            double featureXPlusLeg = featurePointX + legSize;
            if (kneeOne.x < featureXPlusLeg) {
                kneeOne.x = featureXPlusLeg;
            }

            kneeTwo.x = leaderPointX - legSize;
            textOffsetV = RS_Vector(1,0);
        }
        else {
            kneeOne.x = leaderPointX + doubleLeg;
            double featurePointXMinusLeg = featurePointX - legSize;
            if (kneeOne.x > featurePointXMinusLeg) {
                kneeOne.x = featurePointXMinusLeg;
            }

            kneeTwo.x = leaderPointX + legSize;
            textOffsetV = RS_Vector(-1,0);
        }
    }

    if (xAxisRotatedInUCS) {
        kneeOne = kneeOne.rotate(featurePoint, m_dimGenericData.horizontalAxisDirection);
        kneeTwo = kneeTwo.rotate(featurePoint, m_dimGenericData.horizontalAxisDirection);
    }
}

void LC_DimOrdinate::doUpdateDim() {
    RS_DEBUG->print("RS_DimLinear::update");
    clear();
    if (isUndone()) {
        return;
    }

    // general scale (DIMSCALE)
    double dimscale = getGeneralScale();
    // distance from entities (DIMEXO)
    double dimexo = getExtensionLineOffset()*dimscale;

    RS_Vector featurePoint = m_dimOrdinateData.featurePoint;
    RS_Vector leaderEndPoint = m_dimOrdinateData.leaderEndPoint;

    bool xAxisRotatedInUCS = LC_LineMath::isMeaningfulAngle(m_dimGenericData.horizontalAxisDirection);

    if (xAxisRotatedInUCS) {
        leaderEndPoint.rotate(featurePoint, -m_dimGenericData.horizontalAxisDirection);
    }

    RS_Vector kneeOne;
    RS_Vector kneeTwo;
    RS_Vector textOffsetV;   // normal vector in direction of text offset
    determineKneesPositions(featurePoint, leaderEndPoint, kneeOne, kneeTwo, textOffsetV);

    auto linePen = getPenExtensionLine(true);

    if (featurePoint.distanceTo(kneeOne) > dimexo) {
        auto startPoint = featurePoint + textOffsetV*dimexo;
        if (xAxisRotatedInUCS) {
            startPoint = startPoint.rotate(featurePoint, m_dimGenericData.horizontalAxisDirection);
        }
        // RS_Line* dummy;
        // adjustExtensionLineFixLength(line, dummy, false);
        addDimComponentLine(startPoint, kneeOne, linePen);
    }

    addDimComponentLine(kneeOne, kneeTwo, linePen);
    addDimComponentLine(kneeTwo,m_dimOrdinateData.leaderEndPoint, linePen);

    double textHeight = getTextHeight() * dimscale;
    double dimgap = getDimensionLineGap();

    bool corrected = false;
    double textWidth = 0;
    double textAngle = RS_Math::makeAngleReadable(0, true, &corrected);

    bool inXDirection = m_dimOrdinateData.ordinateForX;
    if (inXDirection) {
        textAngle = M_PI_2;
    }
    else {
        textAngle = 0;
    }

    textAngle += m_dimGenericData.horizontalAxisDirection;

    auto* mtext = createDimText({0,0},textHeight,textAngle);

    textWidth = mtext->getUsedTextWidth();
    textHeight = mtext->getUsedTextHeight();

    RS_Vector middlePos;
    RS_Vector textPos;

    if (m_dimGenericData.autoText) {
        if (inXDirection) {
            textPos = m_dimOrdinateData.leaderEndPoint; // fixme - positioning of text by x
            textPos.y = textPos.y + (textOffsetV * (textWidth/2.0+dimgap)).y;
        }
        else {
            textPos = m_dimOrdinateData.leaderEndPoint;// fixme - positioning of text by Y;
            textPos.x = textPos.x + (textOffsetV * (textWidth/2.0 + dimgap )).x;
        }

        if (xAxisRotatedInUCS) {
            textPos.rotate(m_dimOrdinateData.leaderEndPoint, m_dimGenericData.horizontalAxisDirection);
        }

        m_dimGenericData.middleOfText = textPos;
    } else {
        middlePos = m_dimGenericData.middleOfText;
        textPos = middlePos;
    }
    mtext->move(textPos);
    calculateBorders();
}

void LC_DimOrdinate::adjustExtensionLineIfFixLength([[maybe_unused]]RS_Line* extLine1, [[maybe_unused]]RS_Line* extLine2, [[maybe_unused]]bool addDimExe) const {
    // fixme - sand - dims - decide how the fixed len could be supported in general?
    // extension line extension (DIMEXE)
    // double dimexe = getExtensionLineExtension()*dimscale;
    /*// extension lines with fixed length:
    if (dimensionData->isExtLineFix()) {
        double extLineLen = dimensionData->getExtLineFixLength();
        if (RS_Math::fuzzyCompare(extLineLen, 0.0)) {
            // value of 0 for extension line fixed length means fixed length is off:
            return;
        }

        if (addDimExe) {
            double dimexe = dimensionData->getDimexe();
            extLineLen += dimexe;
        }
        if (extLine1.isValid()) {
            extLine1.setLength(qMin(extLine1.getLength(), extLineLen), false);
        }
        if (extLine2.isValid()) {
            extLine2.setLength(qMin(extLine2.getLength(), extLineLen), false);
        }
    }*/
}


QString LC_DimOrdinate::getMeasuredLabel() {

    bool xAxisRotatedInUCS = LC_LineMath::isMeaningfulAngle(m_dimGenericData.horizontalAxisDirection);

    RS_Vector featurePoint = m_dimOrdinateData.featurePoint;
    if (xAxisRotatedInUCS) {
        featurePoint.rotate(m_dimGenericData.definitionPoint, -m_dimGenericData.horizontalAxisDirection);
    }

    RS_Vector delta = ( featurePoint - m_dimGenericData.definitionPoint);

    double distance;
    if (m_dimOrdinateData.ordinateForX) {
        distance = delta.x;
    }
    else {
        distance = delta.y;
    }

    double dist = prepareLabelLinearDistance(distance);
    QString measuredLabel =  createLinearMeasuredLabel(dist);
    return measuredLabel;
}
