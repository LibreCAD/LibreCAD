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

#ifndef LC_ACTIONDRAWGDTFEATURECONTROLFRAME_H
#define LC_ACTIONDRAWGDTFEATURECONTROLFRAME_H

#include <lc_tolerance.h>

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class LC_ActionDrawGDTFeatureControlFrame: public LC_SingleEntityCreationAction{
    Q_OBJECT
public:
    explicit LC_ActionDrawGDTFeatureControlFrame(LC_ActionContext* actionContext);
    void init(int status) override;
    QStringList getAvailableCommands() override;
protected:
    enum State {
        ShowDialog = InitialActionStatus,
        SetInsertionPoint
    };

    struct ActionData {
        ~ActionData() {
            clear();
        }

        void clear() {
            insertionPoint.valid = false;
            // delete m_entity;
        }

        RS_Vector insertionPoint;
        LC_Tolerance* entity {nullptr};
    };

    std::unique_ptr<ActionData> m_actionData;
    void updateActionPrompt() override;
    bool doProcessCommand(int status, const QString& command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* event) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    QStringList doGetAvailableCommands(int status) override;
};

#endif
