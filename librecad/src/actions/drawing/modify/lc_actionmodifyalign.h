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

#ifndef LC_ACTIONMODIFYALIGN_H
#define LC_ACTIONMODIFYALIGN_H

#include "lc_actionpreselectionawarebase.h"
#include "lc_align.h"

class LC_ActionModifyAlignData {
public:
    virtual ~LC_ActionModifyAlignData() = default;
    int getHAlign() const {return hAlign;}
    void setHAlign(int h) { hAlign = h;}
    int getVAlign() const {return vAlign;}
    void setVAlign(int v){vAlign = v;}
    bool isAsGroup() const {return asGroup;}
    void setAsGroup(bool a) {asGroup = a;}
    int getAlignType() const {return alignType;}
    virtual void setAlignType(int a) {alignType = a;}

protected:
    int hAlign = LC_Align::NONE;
    int vAlign = LC_Align::NONE;
    bool asGroup = false;
    int alignType = LC_Align::ENTITY;
};

class LC_ActionModifyAlign:public LC_ActionPreSelectionAwareBase, public LC_ActionModifyAlignData {
    Q_OBJECT
public:
    LC_ActionModifyAlign(LC_ActionContext *actionContext);
    void setAlignType(int a) override;
    void init(int status) override;
protected:
    RS_Vector m_alignMin = RS_Vector(false);
    RS_Vector m_alignMax = RS_Vector(false);
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    void onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    void onMouseRightButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    bool isAllowTriggerOnEmptySelection() override;
    void onMouseMoveEventSelected(int status, LC_MouseEvent *e) override;
    RS_Vector createAlignedEntities(QList<RS_Entity *> &list, RS_Vector min, RS_Vector max, bool previewOnly);
    RS_Vector getReferencePoint(const RS_Vector &min, const RS_Vector &max);
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void doTrigger(bool selected) override;
    void previewRefLines(bool drawVertical, double verticalRef, bool drawHorizontal, double horizontalRef);
};

#endif // LC_ACTIONMODIFYALIGN_H
