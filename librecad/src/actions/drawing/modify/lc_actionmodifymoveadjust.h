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

#ifndef LC_ACTIONMODIFYMOVEADJUST_H
#define LC_ACTIONMODIFYMOVEADJUST_H
#include "lc_actionmodifybase.h"
#include "rs_modification.h"

class LC_ActionModifyMoveAdjust: public LC_ActionModifyBase{
public:
    struct MovementInfo {
        enum Step {
            GRID,
            SUB_GRID,
            META_GRID
        };
        MovementInfo(RS2::Direction direction, Step step)
            : m_direction{direction}, m_step(step){

        }
        RS2::Direction getDirection() const {return m_direction;}
        Step getStep() const {return m_step;}

    protected:
        RS2::Direction m_direction {RS2::Left};
        Step m_step {GRID};
    };

    LC_ActionModifyMoveAdjust(LC_ActionContext *actionContext, MovementInfo info);
    void onSelectionCompleted(bool singleEntity, bool fromInit);
    ~LC_ActionModifyMoveAdjust() override = default;
    bool isSupportsPredecessorAction() override {return true;};
protected:
    enum Status {
        Move = InitialActionStatus
    };

    MovementInfo m_movementInfo;
    RS_MoveData m_moveData;

    RS2::CursorType doGetMouseCursorSelected(int status) override;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    RS_Vector calculateOffset() const;
    void doTrigger(bool keepSelected) override;
    void updateMouseButtonHintsForSelection() override {};
};

#endif // LC_ACTIONMODIFYMOVEADJUST_H
