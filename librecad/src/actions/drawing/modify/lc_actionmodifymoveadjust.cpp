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

#include "lc_actionmodifymoveadjust.h"

#include "lc_graphicviewport.h"
#include "rs_grid.h"

LC_ActionModifyMoveAdjust::LC_ActionModifyMoveAdjust(LC_ActionContext* actionContext, MovementInfo movement)
    : LC_ActionModifyBase("MoveAdjust Entities", actionContext, RS2::ActionModifyMoveAdjust),
      m_movementInfo{movement} {
}

void LC_ActionModifyMoveAdjust::onSelectionCompleted(bool singleEntity, bool fromInit) {
    LC_ActionPreSelectionAwareBase::onSelectionCompleted(singleEntity, fromInit);
}

RS2::CursorType LC_ActionModifyMoveAdjust::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

LC_ModifyOperationFlags* LC_ActionModifyMoveAdjust::getModifyOperationFlags() {
    return &m_moveData;
}

void LC_ActionModifyMoveAdjust::doTrigger([[maybe_unused]]bool keepSelected) {
    m_moveData.offset = calculateOffset();
    m_moveData.keepOriginals = false;
    m_moveData.keepOriginals = false;
    RS_Modification m(*m_container, m_viewport);
    m.move(m_moveData, m_selectedEntities, false, true);
    finish(false);
}

RS_Vector LC_ActionModifyMoveAdjust::calculateOffset() const {
    auto grid = m_viewport->getGrid();
    auto cellVector = grid->getCellVector();

    int metagridFactor = grid->getMetaGridEvery();

    double offsetValueX = 0.0;
    double offsetValueY = 0.0;
    bool isometricGrid = grid->isIsometric();

    switch (m_movementInfo.getDirection()) {
        case RS2::Down:{
             offsetValueY = -cellVector.y;
             break;
         }
        case RS2::Up:{
            offsetValueY = cellVector.y;
            break;
        }
        case RS2::Left:{
            offsetValueX = cellVector.x;
            break;
        }
        case RS2::Right:{
            offsetValueX = -cellVector.x;
            break;
        }
        default:
            break;
    }

    if (isometricGrid) {
        offsetValueX = offsetValueX / 4.0;
        offsetValueY = offsetValueY / 4.0;
    }

    // fixme - sand - should we use different factors based on current drawing units (metric/imperial)?
    // for imperial, we may be rely on the scale instead of metagrid
    double factor = 1.0;
    switch (m_movementInfo.getStep()) {
        case MovementInfo::SUB_GRID:
            factor /= metagridFactor;
            break;
        case MovementInfo::META_GRID:
            factor *= metagridFactor;
            break;
        default:
            factor = 1.0;
            break;
    }

    RS_Vector ucsDelta(offsetValueX*factor, offsetValueY*factor);
    RS_Vector result = toWorldDelta(ucsDelta);
    return result;
}
