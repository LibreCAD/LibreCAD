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
    int getHAlign() const {return m_hAlign;}
    void setHAlign(const int h) { m_hAlign = h;}
    int getVAlign() const {return m_vAlign;}
    void setVAlign(const int v){m_vAlign = v;}
    bool isAsGroup() const {return m_asGroup;}
    void setAsGroup(const bool a) {m_asGroup = a;}
    int getAlignType() const {return m_alignType;}
    virtual void setAlignType(const int a) {m_alignType = a;}

protected:
    int m_hAlign = LC_Align::NONE;
    int m_vAlign = LC_Align::NONE;
    bool m_asGroup = false;
    int m_alignType = LC_Align::ENTITY;
};

class LC_ActionModifyAlign:public LC_ActionPreSelectionAwareBase, public LC_ActionModifyAlignData {
    Q_OBJECT
public:
    explicit LC_ActionModifyAlign(LC_ActionContext *actionContext);
    void setAlignType(int t) override;
    void init(int status) override;
protected:
    RS_Vector m_alignMin = RS_Vector(false);
    RS_Vector m_alignMax = RS_Vector(false);
    LC_ActionOptionsWidget *createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    bool isAllowTriggerOnEmptySelection() override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* e) override;
    RS_Vector createAlignedEntities(QList<RS_Entity *> &clonesList, const RS_Vector& min, const RS_Vector& max, bool previewOnly);
    RS_Vector getReferencePoint(const RS_Vector &min, const RS_Vector &max) const;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void previewRefLines(bool drawVertical, double verticalRef, bool drawHorizontal, double horizontalRef) const;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
