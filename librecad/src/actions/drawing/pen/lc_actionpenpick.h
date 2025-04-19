
/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 sand1024

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
#ifndef LC_ACTIONPENPICK_H
#define LC_ACTIONPENPICK_H

#include "rs_previewactioninterface.h"

/**
 * Action that picks the pen (resolved or not) from entity selected by the user
 * and applies it to pen toolbar
 */
class LC_ActionPenPick:public RS_PreviewActionInterface {
       Q_OBJECT
public:
    enum {
          SelectEntity
     };
    LC_ActionPenPick(LC_ActionContext *actionContext,bool resolve);
    void init(int status) override;
    void finish(bool updateTB) override;
private:
    /**
     * flag that indicates whether pen from entity should be resolved
     */
    bool m_resolveMode;
    void applyPenToPenToolBar(RS_Entity* entity);
protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void updateMouseButtonHints() override;
};
#endif // LC_ACTIONPENPICK_H
