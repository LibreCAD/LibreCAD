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

#ifndef LC_ACTIONINTERACTIVEPICKPOSITION_H
#define LC_ACTIONINTERACTIVEPICKPOSITION_H

#include "lc_actioncontext.h"
#include "lc_actioninteractivepickbase.h"
#include "rs_previewactioninterface.h"

class LC_ActionInteractivePickPosition :public LC_ActionInteractivePickBase {
    Q_OBJECT
public:
    explicit  LC_ActionInteractivePickPosition(LC_ActionContext *actionContext);
    ~LC_ActionInteractivePickPosition() override;
    void init(int status) override;
protected:
    /**
    * Action States.
    */
    enum Status {
        SetPoint = InitialActionStatus
    };

    RS_Vector m_wcsPosition{false};

    enum CoordinateType {
        POS_ABSOLUTE,
        POS_RELATIVE,
        POS_RELATIVE_ZERO
    };
    CoordinateType m_coordinatesType{POS_ABSOLUTE};
    bool isInteractiveDataValid() override;
    void doSetInteractiveInputValue(LC_ActionContext::InteractiveInputInfo* interactiveInputInfo) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    void updateInfoCursor(const RS_Vector &mouse, const RS_Vector &relZero);
};

#endif // LC_ACTIONINTERACTIVEPICKPOSITION_H
