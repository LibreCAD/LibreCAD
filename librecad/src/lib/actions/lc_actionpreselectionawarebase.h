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

#ifndef LC_ACTIONPRESELECTIONAWAREBASE_H
#define LC_ACTIONPRESELECTIONAWAREBASE_H

#include "rs_actionselectbase.h"

class LC_ActionPreSelectionAwareBase:public RS_ActionSelectBase{
    Q_OBJECT
public:
    LC_ActionPreSelectionAwareBase(const char *name, LC_ActionContext *actionContext, RS2::ActionType actionType = RS2::ActionNone,
        const QList<RS2::EntityType> &entityTypeList = {}, bool countSelectionDeep = false);
    ~LC_ActionPreSelectionAwareBase() override;
    void mousePressEvent(QMouseEvent*) override;
    void init(int status) override;
    void drawSnapper() override;

protected:
    bool m_selectionComplete = false;
    bool m_countDeep = false;
    std::vector<RS_Entity*> m_selectedEntities;

    RS_Vector m_selectionCorner1 = RS_Vector(false);
    bool m_inBoxSelectionMode = false;

    void selectionFinishedByKey(QKeyEvent *e, bool escape) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    virtual void applyBoxSelectionModeIfNeeded(RS_Vector mouse);
    virtual void onSelectionCompleted(bool singleEntity, bool fromInit);
    virtual void onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *pEvent);
    virtual void onMouseRightButtonReleaseSelected(int status, LC_MouseEvent *pEvent);
    virtual void onMouseMoveEventSelected(int status, LC_MouseEvent *e);
    virtual void updateMouseButtonHintsForSelection() = 0;
    virtual void updateMouseButtonHintsForSelected(int status);
    virtual bool isAllowTriggerOnEmptySelection(){return true;};
    virtual void doTrigger(bool keepSelected) = 0;
    virtual void finishMouseMoveOnSelection(LC_MouseEvent *event);
    virtual void proceedSelectedEntity(LC_MouseEvent* e);
    RS2::CursorType doGetMouseCursor(int status) override;
    virtual RS2::CursorType doGetMouseCursorSelected(int status);
    unsigned int countSelectedEntities();
    void setSelectionComplete(bool allowEmptySelection, bool fromInit);
    void updateMouseButtonHints() override;
    void doSelectEntity(RS_Entity *entityToSelect, bool selectContour) const override;
    void doTrigger() override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;

};

#endif // LC_ACTIONPRESELECTIONAWAREBASE_H
