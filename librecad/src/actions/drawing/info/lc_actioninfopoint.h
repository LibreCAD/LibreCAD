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

#ifndef LC_ACTIONINFOPOINT_H
#define LC_ACTIONINFOPOINT_H
#include "rs_previewactioninterface.h"

class LC_ActionInfoPoint: public RS_PreviewActionInterface{
    Q_OBJECT
public:
    LC_ActionInfoPoint(LC_ActionContext* actionContext);
    ~LC_ActionInfoPoint() override = default;

protected:
    enum Status {
        SetPoint = InitialActionStatus
    };
    RS_Vector m_position{false};

    enum CoordinateType {
        POS_ABSOLUTE,
        POS_RELATIVE,
        POS_RELATIVE_ZERO
    };
    CoordinateType m_coordinatesType{POS_ABSOLUTE};

    void doTrigger() override;
    void onMouseMoveEvent(int status, LC_MouseEvent* e) override;
    void updateInfoCursor(const RS_Vector& mouse, const RS_Vector& relZero);
    void onMouseLeftButtonRelease(int status, LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent* e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& mouse) override;
    void updateMouseButtonHints() override;
    RS2::CursorType doGetMouseCursor(int status) override;
};

#endif // LC_ACTIONINFOPOINT_H
