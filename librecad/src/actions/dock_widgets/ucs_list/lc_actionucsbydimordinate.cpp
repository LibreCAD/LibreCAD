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

#include "lc_actionucsbydimordinate.h"

#include "lc_dimordinate.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_ucs.h"
#include "rs_document.h"

LC_ActionUCSByDimOrdinate::LC_ActionUCSByDimOrdinate(LC_ActionContext* actionContext)
    : LC_ActionSingleEntitySelectBase("UCSFromDimOrdinateAction", actionContext, RS2::ActionUCSSetByDimOrdinate){
}

LC_ActionUCSByDimOrdinate::~LC_ActionUCSByDimOrdinate() = default;


void LC_ActionUCSByDimOrdinate::doTrigger() {
    if (m_entity != nullptr) {
        auto dimOrdinate = dynamic_cast<LC_DimOrdinate*>(m_entity);
        if (dimOrdinate != nullptr) {
            double horizontalDirection = dimOrdinate->getHDir();
            auto definitionPoint = dimOrdinate->getDefinitionPoint();
            bool nonWCS = LC_LineMath::isMeaningfulAngle(horizontalDirection) || LC_LineMath::isMeaningfulDistance(definitionPoint, RS_Vector(0, 0, 0));
            if (nonWCS) {
                m_viewport->createUCS(definitionPoint, horizontalDirection);
            }
            else {
                m_viewport->applyUCS(&LC_WCS::instance);
            }
        }
    }
    init(-1);
}

bool LC_ActionUCSByDimOrdinate::doCheckMaySelectEntity(RS_Entity* e) {
    return e->rtti() == RS2::EntityDimOrdinate;
}

QString LC_ActionUCSByDimOrdinate::doGetMouseButtonHint() {
    return tr("Select ordinate dimension to set UCS");
}
