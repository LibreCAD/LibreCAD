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

#ifndef LC_ACTIONSPLINEAPPENDPOINT_H
#define LC_ACTIONSPLINEAPPENDPOINT_H

#include "rs_previewactioninterface.h"

class LC_ActionSplineAppendPoint:public RS_PreviewActionInterface{
Q_OBJECT
public:
    LC_ActionSplineAppendPoint(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionSplineAppendPoint() override = default;

    void mouseMoveEvent(QMouseEvent *event) override;

    void trigger() override;

    void finish(bool updateTB) override;

protected:

    enum State{
        SetEntity,
        SetFirstControlPoint,
        SetControlPoint
    };

    RS_Entity *entityToModify = nullptr;
    bool appendPointsToStart = false;
    RS_Vector vertexToAppend = RS_Vector(false);


    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;

    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;

    RS2::CursorType doGetMouseCursor(int status) override;

    void updateMouseButtonHints() override;

    bool mayAppendPointToEntity(RS_Entity *e);

    void clean();

    RS_Entity *createNewEntityWithAddedControlPoint(RS_Entity *e, RS_Vector controlPoint, bool fromStart);
};

#endif // LC_ACTIONSPLINEAPPENDPOINT_H
