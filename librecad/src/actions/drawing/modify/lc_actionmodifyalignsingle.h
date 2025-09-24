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
    explicit LC_ActionModifyAlignSingle(LC_ActionContext *actionContext);
    void setAlignType(int a) override;
    void init(int status) override;
protected:
    enum State{
        SetRefPoint = InitialActionStatus,
        SelectEntity
    };
    RS_Vector m_alignMin = RS_Vector(false);
    RS_Vector m_alignMax = RS_Vector(false);
    RS_Entity* m_baseAlignEntity = nullptr;
    RS_Entity* m_entityToAlign = nullptr;
    bool m_finishActionAfterTrigger = false;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void previewAlignRefPoint(const RS_Vector &min, const RS_Vector &max);
    void previewRefLines(bool drawVertical, double verticalRef, bool drawHorizontal, double horizontalRef);
    QString prepareInfoCursorMessage(double verticalRef, bool drawVertical, double horizontalRef, bool drawHorizontal);
    void doTrigger() override;
    void previewAlign(RS_Entity* entity, double verticalRef, bool drawVertical, double horizontalRef,
                      bool drawHorizontal, const RS_Vector& alignMin, const RS_Vector& m_alignMax);
};

#endif // LC_ACTIONMODIFYALIGNSINGLE_H
