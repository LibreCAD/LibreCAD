/****************************************************************************
**
* Abstract base class for actions that operates with selected entities

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

#ifndef LC_ACTIONPRESELECTIONAWAREBASE_H
#define LC_ACTIONPRESELECTIONAWAREBASE_H

#include "rs_actionselectbase.h"

class LC_ActionPreSelectionAwareBase:public RS_ActionSelectBase
{
public:
    LC_ActionPreSelectionAwareBase(
        const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
        const QList<RS2::EntityType> &entityTypeList = {}, bool countSelectionDeep = false);

    ~LC_ActionPreSelectionAwareBase() override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void init(int status) override;
protected:
    bool selectionComplete = false;
    bool countDeep = false;
    std::vector<RS_Entity*> selectedEntities;
    void selectionFinishedByKey(QKeyEvent *e, bool escape) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    virtual void selectionCompleted(bool singleEntity, bool fromInit);
    virtual void mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *pEvent);
    virtual void mouseRightButtonReleaseEventSelected(int status, QMouseEvent *pEvent);
    virtual void mouseMoveEventSelected(QMouseEvent *e);
    virtual void updateMouseButtonHintsForSelection() = 0;
    virtual void updateMouseButtonHintsForSelected(int status);

    RS2::CursorType doGetMouseCursor(int status) override;
    virtual RS2::CursorType doGetMouseCursorSelected(int status);

    unsigned int countSelectedEntities();
    void setSelectionComplete(bool allowEmptySelection, bool fromInit);
    virtual bool isAllowTriggerOnEmptySelection(){return true;};
    void updateMouseButtonHints() override;
};

#endif // LC_ACTIONPRESELECTIONAWAREBASE_H
