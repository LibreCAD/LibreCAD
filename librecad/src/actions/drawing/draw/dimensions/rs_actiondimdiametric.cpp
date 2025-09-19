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

#include "rs_actiondimdiametric.h"

#include "rs_dimdiametric.h"

// fixme - sand - possibility to define label inside/outside,
// todo - think whether it's practical adding multiple dimensions to selected circles?
RS_ActionDimDiametric::RS_ActionDimDiametric(LC_ActionContext *actionContext)
        :LC_ActionCircleDimBase("Draw Diametric Dimensions", actionContext, RS2::EntityDimDiametric, RS2::ActionDimDiametric)
        , m_edata{std::make_unique<RS_DimDiametricData>()}{
    reset();
}

RS_ActionDimDiametric::~RS_ActionDimDiametric() = default;

void RS_ActionDimDiametric::reset(){
    LC_ActionCircleDimBase::reset();
    *m_edata = {{}, 0.0};
    m_entity = nullptr;
    *m_position = {};
    updateOptions();
}

RS_Dimension *RS_ActionDimDiametric::createDim(RS_EntityContainer *parent) const{
    auto *newEntity= new RS_DimDiametric(parent,*m_dimensionData,*m_edata);
    return newEntity;
}

RS_Vector RS_ActionDimDiametric::preparePreview(RS_Entity *en, RS_Vector &position, bool forcePosition) {
    if (en != nullptr){
        double radius = en->getRadius();
        RS_Vector center = en->getCenter();
        double angleToUse = m_currentAngle;
        if (m_angleIsFree || forcePosition){
            angleToUse = center.angleTo(position);
        }
        m_dimensionData->definitionPoint.setPolar(radius, angleToUse + M_PI);
        m_dimensionData->definitionPoint += center;
        m_edata->definitionPoint.setPolar(radius, angleToUse);
        m_edata->definitionPoint += center;

        RS_Vector result = center + RS_Vector::polar(radius, angleToUse);
        return result;
    }
    else{
        return RS_Vector(false);
    }
}
