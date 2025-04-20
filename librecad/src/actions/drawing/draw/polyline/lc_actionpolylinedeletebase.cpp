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

#include "lc_actionpolylinedeletebase.h"

#include "rs_polyline.h"

LC_ActionPolylineDeleteBase::LC_ActionPolylineDeleteBase(const char *name, LC_ActionContext *actionContext, RS2::ActionType actionType)
   :RS_PreviewActionInterface(name, actionContext, actionType){
}

void LC_ActionPolylineDeleteBase::getSelectedPolylineVertex(LC_MouseEvent *e, RS_Vector &vertex, RS_Entity *&segment){
    bool oldSnapOnEntity = m_snapMode.snapOnEntity;
    m_snapMode.snapOnEntity = true;
    RS_Vector snap = e->snapPoint;
    m_snapMode.snapOnEntity = oldSnapOnEntity;
    deletePreview();
    auto polyline = dynamic_cast<RS_Polyline *>(catchEntityByEvent(e, RS2::EntityPolyline));
    if (polyline == m_polylineToModify){
        RS_Vector coordinate = polyline->getNearestPointOnEntity(snap, true);
        segment = RS_Snapper::catchEntity(coordinate, RS2::ResolveAll);
        vertex  = segment->getNearestEndpoint(coordinate);
    }
    else{
        vertex = RS_Vector(false);
    }
}

void LC_ActionPolylineDeleteBase::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
}

RS2::CursorType LC_ActionPolylineDeleteBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void LC_ActionPolylineDeleteBase::finish(bool updateTB){
    clean();
    RS_PreviewActionInterface::finish(updateTB);
}

void LC_ActionPolylineDeleteBase::clean(){
    if (m_polylineToModify){
        m_polylineToModify->setSelected(false);
    }
    deletePreview();
    redraw();
}

void LC_ActionPolylineDeleteBase::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    deleteSnapper();
    deletePreview();
    drawPreview();
    int newStatus = getStatus() - 1;
    if (newStatus == SetPolyline){
        clean();
    }
    setStatus(newStatus);
}
