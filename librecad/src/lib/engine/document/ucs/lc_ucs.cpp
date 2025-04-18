/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_ucs.h"

#include <QObject>

#include "lc_linemath.h"
#include "rs_math.h"

LC_UCS LC_WCS::instance = LC_WCS();

LC_UCS::LC_UCS() {}

LC_UCS::LC_UCS(QString n) {
    m_name = n;
}

void LC_UCS::setElevation(double d) {
    m_ucsElevation = d;
}

void LC_UCS::setOrigin(RS_Vector o) {
  m_ucsOrigin = o;
}

void LC_UCS::setXAxis(RS_Vector pos) {
  m_ucsXAxis = pos;
}

void LC_UCS::setYAxis(RS_Vector axis) {
  m_ucsYAxis = axis;
}

void LC_UCS::setOrthoType(int type) {
  m_ucsOrthoType = type;
}

void LC_UCS::setName(const QString &name) {
    LC_UCS::m_name = name;
}

const QString LC_UCS::getName() const {
    return m_name;
}

const RS_Vector LC_UCS::getOrthoOrigin() const {
    return m_orthoOrigin;
}

void LC_UCS::setOrthoOrigin(const RS_Vector &orthoOrigin) {
    LC_UCS::m_orthoOrigin = orthoOrigin;
}

long LC_UCS::getNamedUcsId() const {
    return namedUCS_ID;
}

long LC_UCS::getBaseUcsId() const {
    return baseUCS_ID;
}

bool LC_UCS::isValidName([[maybe_unused]]const QString &nameCandidate) {
    // fixme - implement UCS name validation rule here
    return true;
}

bool LC_UCS::isSameTo(LC_UCS *other) {
    bool result = false;
    if (other != nullptr){
//        if (isUCS() == other->isUCS()) {
            if (m_ucsOrthoType == other->getOrthoType()) {
                if (LC_LineMath::isNotMeaningfulDistance(other->getOrigin(), getOrigin())) { // same origin
                    double ownAngle = RS_Math::correctAnglePlusMinusPi(getXAxisDirection());
                    double otherAngle = RS_Math::correctAnglePlusMinusPi(other->getXAxisDirection());
                    result = RS_Math::getAngleDifference(ownAngle, otherAngle) < RS_TOLERANCE_ANGLE;
                }
            }
        }
//    }
    return result;
}


RS2::IsoGridViewType LC_UCS::getIsoGridViewType() {
    RS2::IsoGridViewType isoType = RS2::Ortho;
    switch (m_ucsOrthoType){
        case LC_UCS::FRONT:
        case LC_UCS::BACK:
            break;
        case LC_UCS::LEFT:{
            isoType = RS2::IsoLeft;
            break;
        }
        case LC_UCS::RIGHT:{
            isoType = RS2::IsoRight;
            break;
        }
        case LC_UCS::TOP:
        case LC_UCS::BOTTOM:{
            isoType = RS2::IsoTop;
            break;
        }
        default:
            isoType = RS2::Ortho;
            break;
    }
    return isoType;
}

bool LC_UCS::isIsometric() {
    return m_ucsOrthoType != LC_UCS::NON_ORTHO;
}

LC_WCS::LC_WCS():
    LC_UCS(QObject::tr("WCS"))
{}

LC_UCS::~LC_UCS() = default;

LC_UCS* LC_UCS::clone() {
    auto* clone = new LC_UCS();
    clone->m_name =  m_name;
    clone->m_ucsOrigin = m_ucsOrigin;
    clone->m_ucsXAxis = m_ucsXAxis;
    clone->m_ucsYAxis = m_ucsYAxis;
    clone->m_orthoOrigin = m_orthoOrigin;
    clone->m_ucsOrthoType = m_ucsOrthoType;
    clone->m_ucsElevation = m_ucsElevation;
    return clone;
}
