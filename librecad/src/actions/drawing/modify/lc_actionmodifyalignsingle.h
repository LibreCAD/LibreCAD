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

#ifndef LC_ACTIONMODIFYALIGNSINGLE_H
#define LC_ACTIONMODIFYALIGNSINGLE_H

#include "lc_actionmodifyalign.h"
#include "rs_previewactioninterface.h"

class LC_ActionModifyAlignSingle:public RS_PreviewActionInterface, public LC_ActionModifyAlignData{
    Q_OBJECT
public:
    LC_ActionModifyAlignSingle(RS_EntityContainer &container,
                                RS_GraphicView &graphicView);

    void mouseMoveEvent(QMouseEvent *event) override;
    void setAlignType(int a) override;
    void init(int status) override;
protected:
    enum State{
        SetRefPoint,
        SelectEntity
    };
    RS_Vector alignMin = RS_Vector(false);
    RS_Vector alignMax = RS_Vector(false);
    RS_Entity* baseAlignEntity = nullptr;
    RS_Entity* entityToAlign = nullptr;
    bool finishActionAfterTrigger = false;

    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void previewAlignRefPoint(const RS_Vector &min, const RS_Vector &max);
    void previewRefLines(bool drawVertical, double verticalRef, bool drawHorizontal, double horizontalRef);
    QString prepareInfoCursorMessage(double verticalRef, bool drawVertical, double horizontalRef, bool drawHorizontal);
    void doTrigger() override;
};

#endif // LC_ACTIONMODIFYALIGNSINGLE_H
