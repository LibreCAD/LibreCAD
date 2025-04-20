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

#include "rs_actiondimradial.h"

#include "rs_dimradial.h"

// fixme - sand - options for selection definition point angle, ability to specify whether label is inside or outside
// todo - think about multiple adding dimensions to already selected circles

RS_ActionDimRadial::RS_ActionDimRadial(LC_ActionContext *actionContext)
    :LC_ActionCircleDimBase("Draw Radial Dimensions",actionContext, RS2::ActionDimRadial)
    , m_edata{ std::make_unique<RS_DimRadialData>()}{
    reset();
}

RS_ActionDimRadial::~RS_ActionDimRadial() = default;

void RS_ActionDimRadial::reset(){
    RS_ActionDimension::reset();

    *m_edata = {};
    m_entity = nullptr;
    *m_position = {};
    m_lastStatus = SetEntity;
}

RS_Dimension *RS_ActionDimRadial::createDim(RS_EntityContainer *parent) const{
    auto *newEntity = new RS_DimRadial(parent, *m_dimensionData, *m_edata);
    return newEntity;
}

RS_Vector RS_ActionDimRadial::preparePreview(RS_Entity *en, RS_Vector &position, bool forcePosition) {
    if (en != nullptr){
        double radius = en->getRadius();
        RS_Vector center = en->getCenter();
        m_dimensionData->definitionPoint = center;
        double angleToUse = m_currentAngle;
        if (m_angleIsFree || forcePosition){
            angleToUse = m_dimensionData->definitionPoint.angleTo(position);
        }
        m_edata->definitionPoint.setPolar(radius, angleToUse);
        m_edata->definitionPoint += m_dimensionData->definitionPoint;
        RS_Vector result = center + RS_Vector::polar(radius, angleToUse);
        return result;
    }
    else{
        return RS_Vector(false);
    }
}
