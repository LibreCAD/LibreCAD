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

#ifndef LC_ACTIONSINGLEENTITYSELECTBASE_H
#define LC_ACTIONSINGLEENTITYSELECTBASE_H
#include "rs_previewactioninterface.h"

class LC_ActionSingleEntitySelectBase: public RS_PreviewActionInterface{
public:
    explicit LC_ActionSingleEntitySelectBase(const char *name,LC_ActionContext *actionContext,RS2::ActionType actionType);
    ~LC_ActionSingleEntitySelectBase() override;

protected:
    enum Status {
        SetEntity = InitialActionStatus
    };

    RS_Entity* m_entity;
    void doInitialInit() override;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    virtual QString doGetMouseButtonHint() = 0;
    virtual bool doCheckMaySelectEntity(RS_Entity* e) = 0;
    void updateMouseButtonHints() override;
    void onMouseMoveEvent(int status, LC_MouseEvent* event) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent* e) override;
};

#endif // LC_ACTIONSINGLEENTITYSELECTBASE_H
