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

#ifndef LC_ACTIONMODIFYALIGNREF_H
#define LC_ACTIONMODIFYALIGNREF_H


#include "lc_actionmodifybase.h"
#include "rs_modification.h"

class LC_ActionModifyAlignRef: public LC_ActionModifyBase{
    Q_OBJECT
public:
    LC_ActionModifyAlignRef(LC_ActionContext *actionContext);
    void setScale(bool val);
    bool isScale();
protected:
    enum State{
        SetRefPoint1,
        SetTargetPoint1,
        SetRefPoint2,
        SetTargetPoint2
    };

    struct ActionData {
        LC_AlignRefData data;
        RS_Vector referencePoint1;
        RS_Vector targetPoint1;
        RS_Vector referencePoint2;
        RS_Vector targetPoint2;
    };

    ActionData m_actionData;

    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void updateMouseButtonHintsForSelection() override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    void onMouseRightButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    void onMouseMoveEventSelected(int status, LC_MouseEvent *e) override;
    void updateMouseButtonHintsForSelected(int status) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    bool isAllowTriggerOnEmptySelection() override;
    void prepareAlignRefData(const RS_Vector &snap);
    void doTrigger(bool keepSelected) override;
};

#endif // LC_ACTIONMODIFYALIGNREF_H
