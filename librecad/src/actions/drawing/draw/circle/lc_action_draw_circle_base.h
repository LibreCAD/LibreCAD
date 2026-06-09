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

#ifndef LC_ACTIONDRAWCIRCLEBASE_H
#define LC_ACTIONDRAWCIRCLEBASE_H

#include "lc_undoabledocumentmodificationaction.h"
#include "rs_previewactioninterface.h"

class LC_ActionDrawCircleBase : public LC_SingleEntityCreationAction {
    Q_OBJECT
public:
    void init(int status) override;

    virtual bool isReversed() const {
        return false;
    }

    virtual void setReversed([[maybe_unused]] bool b) const {
    }

protected:
    virtual void reset();
    bool m_moveRelPointAtCenterAfterTrigger = true; // todo - move to options?

    LC_ActionDrawCircleBase(const QString& name, LC_ActionContext* actionContext, RS2::ActionType type = RS2::ActionNone);
    ~LC_ActionDrawCircleBase() override;

    void previewEllipseReferencePoints(const RS_Ellipse* ellipse, bool drawAxises = false, bool allPointsNotSelectable = false,
                                       const RS_Vector& mouse = RS_Vector(false)) const;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
};

#endif
