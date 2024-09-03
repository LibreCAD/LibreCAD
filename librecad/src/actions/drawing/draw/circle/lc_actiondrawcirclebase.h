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

#include <QMouseEvent>
#include "rs_previewactioninterface.h"

class LC_ActionDrawCircleBase:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    LC_ActionDrawCircleBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionDrawCircleBase() override;
    void init(int status) override;
    virtual bool isReversed() const{return false;}
    virtual void setReversed ([[maybe_unused]]bool b) const{};
protected:
    virtual void reset();
    bool moveRelPointAtCenterAfterTrigger = true; // todo - move to options?
    void previewEllipseReferencePoints(const RS_Ellipse *ellipse, bool drawAxises = false,  bool allPointsNotSelectable = false, RS_Vector mouse=RS_Vector(false));
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
};

#endif // LC_ACTIONDRAWCIRCLEBASE_H
