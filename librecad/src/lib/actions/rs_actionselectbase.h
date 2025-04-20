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

#ifndef RS_ACTIONSELECTBASE_H
#define RS_ACTIONSELECTBASE_H

#include "lc_overlayboxaction.h"

/**
 * This class is the base class to all select actions.
 *
 * @author Andrew Mustun
 */
class RS_ActionSelectBase:public LC_OverlayBoxAction {
    Q_OBJECT
public:
    RS_ActionSelectBase(const char *name,LC_ActionContext *actionContext, RS2::ActionType actionType = RS2::ActionNone, QList<RS2::EntityType> entityTypeList = {});
    void keyReleaseEvent(QKeyEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
protected:
    const QList<RS2::EntityType> m_catchForSelectionEntityTypes;

    RS2::CursorType doGetMouseCursor(int status) override;
    virtual bool isEntityAllowedToSelect([[maybe_unused]]RS_Entity *ent) const { return true; };
    bool selectEntity(RS_Entity* entityToSelect, bool selectContour);
    RS_Entity *selectionMouseMove(LC_MouseEvent *event);
    virtual void selectionFinishedByKey(QKeyEvent *e, bool escape) = 0;
    virtual bool isShowRefPointsOnHighlight();
    void deselectAll();
    virtual void doSelectEntity(RS_Entity* entityToSelect, bool selectContour) const;
};

#endif
