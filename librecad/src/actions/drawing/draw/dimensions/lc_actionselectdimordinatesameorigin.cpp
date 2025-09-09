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

#include "lc_actionselectdimordinatesameorigin.h"

#include "lc_dimordinate.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"


LC_ActionSelectDimOrdinateSameOrigin::LC_ActionSelectDimOrdinateSameOrigin(LC_ActionContext* actionContext)
    : LC_ActionSingleEntitySelectBase("SelectDimOrdinateSameOriginAction", actionContext, RS2::ActionDimOrdByOriginSelect){
}

LC_ActionSelectDimOrdinateSameOrigin::~LC_ActionSelectDimOrdinateSameOrigin() = default;



void LC_ActionSelectDimOrdinateSameOrigin::selectOrdinatesWithTheSameBase() {
    auto dimOrdinate = dynamic_cast<LC_DimOrdinate*>(m_entity);
    if (dimOrdinate != nullptr) {
        double horizontalDirection = dimOrdinate->getHDir();
        auto definitionPoint = dimOrdinate->getDefinitionPoint();
        for (auto en: *m_document) {
            if (en->isUndone()) {
                continue;
            }
            if (en->rtti() == RS2::EntityDimOrdinate) {
                if (en->isVisible()){
                    auto otherDimOrdinate = dynamic_cast<LC_DimOrdinate*>(en);
                    bool sameBasePoint = LC_LineMath::isSameAngle(otherDimOrdinate->getHDir(), horizontalDirection)
                        &&
                        LC_LineMath::isNotMeaningfulDistance(otherDimOrdinate->getDefinitionPoint(),
                                                             definitionPoint);
                    if (sameBasePoint) {
                        en->setSelected(true);
                    }
                }
            }
        }
    }
}

void LC_ActionSelectDimOrdinateSameOrigin::doTrigger() {
    if (m_entity != nullptr) {
        selectOrdinatesWithTheSameBase();
    }
    m_viewport->notifyChanged();
    init(-1);
}

bool LC_ActionSelectDimOrdinateSameOrigin::doCheckMaySelectEntity(RS_Entity* e) {
    return e->rtti() == RS2::EntityDimOrdinate;
}

QString LC_ActionSelectDimOrdinateSameOrigin::doGetMouseButtonHint() {
    return tr("Select reference ordinate dimension");
}
