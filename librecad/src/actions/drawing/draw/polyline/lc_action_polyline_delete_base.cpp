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

#include "lc_action_polyline_delete_base.h"

#include "rs_polyline.h"

LC_ActionPolylineDeleteBase::LC_ActionPolylineDeleteBase(const char *name, LC_ActionContext *actionContext, const RS2::ActionType actionType)
   :LC_UndoableDocumentModificationAction(name, actionContext, actionType){
}

void LC_ActionPolylineDeleteBase::getSelectedPolylineVertex(const LC_MouseEvent *e, RS_Vector &vertex, RS_Entity *&segment){
    const bool oldSnapOnEntity = m_snapMode.snapOnEntity;  // fixme - sand - what for? review
    m_snapMode.snapOnEntity = true;
    m_snapMode.snapOnEntity = oldSnapOnEntity;
    deletePreview();
    const auto polyline = static_cast<RS_Polyline *>(catchEntityByEvent(e, RS2::EntityPolyline));
    if (polyline == m_polylineToModify){
        const RS_Vector snap = e->snapPoint;
        const RS_Vector coordinate = polyline->getNearestPointOnEntity(snap, true);
        segment = catchEntity(coordinate, RS2::ResolveAll);
        vertex  = segment->getNearestEndpoint(coordinate);
    }
    else{
        vertex = RS_Vector(false);
    }
}

void LC_ActionPolylineDeleteBase::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]] const LC_MouseEvent* e) {
}

RS2::CursorType LC_ActionPolylineDeleteBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void LC_ActionPolylineDeleteBase::finish(){
    clean();
    RS_PreviewActionInterface::finish();
}

void LC_ActionPolylineDeleteBase::clean(){
    if (m_polylineToModify != nullptr){
        unselect(m_polylineToModify);
    }
    deletePreview();
    redraw();
}

void LC_ActionPolylineDeleteBase::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deleteSnapper();
    deletePreview();
    drawPreview();
    const int newStatus = getStatus() - 1;
    if (newStatus == SetPolyline){
        clean();
    }
    setStatus(newStatus);
}
