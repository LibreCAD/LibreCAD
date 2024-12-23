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

#ifndef LC_ACTIONSPLINEMODIFYBASE_H
#define LC_ACTIONSPLINEMODIFYBASE_H


#include "rs_previewactioninterface.h"

class LC_ActionSplineModifyBase:public RS_PreviewActionInterface{
    Q_OBJECT
public:
    LC_ActionSplineModifyBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionSplineModifyBase() override = default;
    void drawSnapper() override;
    void finish(bool updateTB) override;
    void mouseMoveEvent(QMouseEvent *event) override;
protected:
    enum State{
        SetEntity,
        SetBeforeControlPoint,
        SetControlPoint
    };

    RS_Entity *entityToModify = nullptr;
    RS_Vector vertexPoint = RS_Vector(false);
    RS_Vector selectedVertexPoint = RS_Vector(false);
    bool directionFromStart = false;

    void clean();
    virtual bool mayModifySplineEntity([[maybe_unused]]RS_Entity *pEntity) {return true;};
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    virtual void doCompleteTrigger();
    virtual void doAfterTrigger();
    virtual RS_Entity *createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool startDirection)=0;
    virtual void onMouseMove(RS_Vector mouse, int status, QMouseEvent *e) = 0;
    virtual void doOnEntityNotCreated();
    void doTrigger() override;
};

#endif // LC_ACTIONSPLINEMODIFYBASE_H
