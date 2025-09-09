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

#ifndef LC_ACTIONINTERACTIVEPICKANGLE_H
#define LC_ACTIONINTERACTIVEPICKANGLE_H

#include "lc_actioninteractivepickbase.h"
#include "rs_previewactioninterface.h"

class LC_ActionInteractivePickAngle:public LC_ActionInteractivePickBase {
    Q_OBJECT
public:
    LC_ActionInteractivePickAngle(LC_ActionContext *actionContext);
    ~LC_ActionInteractivePickAngle() override;
    void init(int status) override;
protected:

    enum Status{
        SetPoint1 = InitialActionStatus,
        SetPoint2,
        SetPoint3,
        SetSecondLine
    };

    RS_Vector m_point1;
    RS_Vector m_point2;
    RS_Vector m_point3;
    RS_Vector m_intersection;
    RS_Entity* m_entity1 = nullptr;

    double m_angle {0.0};
    bool m_mayTrigger {false};
    bool m_pickAlternative {false};

    bool isInteractiveDataValid() override;
    void doSetInteractiveInputValue(LC_ActionContext::InteractiveInputInfo* interactiveInputInfo) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void updateInfoCursor2(const RS_Vector& point2, const RS_Vector& intersection);
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    void updateInfoCursor(const RS_Vector& mouse, const RS_Vector& point2, const RS_Vector& startPoint);
    void updateInfoCursor(const RS_Vector &mouse, const RS_Vector &startPoint);
    void doTrigger() override;
};

#endif // LC_ACTIONINTERACTIVEPICKANGLE_H
