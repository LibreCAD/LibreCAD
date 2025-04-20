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

#ifndef LC_ACTIONPOLYLINEDELETEBASE_H
#define LC_ACTIONPOLYLINEDELETEBASE_H

#include "rs_previewactioninterface.h"
class RS_Polyline;

class LC_ActionPolylineDeleteBase:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    LC_ActionPolylineDeleteBase(const char *name, LC_ActionContext *actionContext, RS2::ActionType actionType = RS2::ActionNone);
    ~LC_ActionPolylineDeleteBase() override = default;
protected:
    /**
   * Action States.
   */
    enum Status {
        SetPolyline,		/**< Choosing segment of existing polyline to delete between two nodes. */
        SetVertex1,    /**< Setting the node's point1. */
        SetVertex2     /**< Setting the node's point2. */
    };

    RS_Polyline *m_polylineToModify = nullptr;
    RS_Vector m_vertexToDelete = RS_Vector(false);

    void getSelectedPolylineVertex(LC_MouseEvent *e, RS_Vector &vertex, RS_Entity *&segment);
    void finish(bool updateTB) override;
    void clean();
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
};
#endif // LC_ACTIONPOLYLINEDELETEBASE_H
