/****************************************************************************
**
* Angle for 3 points action

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
**********************************************************************/

#ifndef LC_ACTIONINFO3POINTSANGLE_H
#define LC_ACTIONINFO3POINTSANGLE_H

#include "rs_previewactioninterface.h"

class LC_ActionInfo3PointsAngle :public RS_PreviewActionInterface{
Q_OBJECT
public:
    LC_ActionInfo3PointsAngle(RS_EntityContainer &container,
                                  RS_GraphicView &graphicView);

    ~LC_ActionInfo3PointsAngle() override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void trigger() override;
protected:
    enum Status{
        SetPoint1,
        SetPoint2,
        SetPoint3
    };

    RS_Vector point1{false};
    RS_Vector point2{false};
    RS_Vector point3{false};

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
};
#endif // LC_ACTIONINFO3POINTSANGLE_H
