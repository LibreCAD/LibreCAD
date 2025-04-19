/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_actionpolylineadd.h"

#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_modification.h"
#include "rs_polyline.h"

RS_ActionPolylineAdd::RS_ActionPolylineAdd(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Add node", actionContext, RS2::ActionPolylineAdd)
    , m_addCoord(std::make_unique<RS_Vector>()){
}

RS_ActionPolylineAdd::~RS_ActionPolylineAdd() = default;

void RS_ActionPolylineAdd::init(int status) {
    RS_PreviewActionInterface::init(status);
    m_polylineToModify = nullptr;
    m_addSegment = nullptr;
    *m_addCoord = {};
}

void RS_ActionPolylineAdd::doTrigger() {
    RS_DEBUG->print("RS_ActionPolylineAdd::trigger()");

    if (m_polylineToModify && m_addSegment->isAtomic() && m_addCoord->valid &&
        m_addSegment->isPointOnEntity(*m_addCoord)) {
        RS_Modification m(*m_container, m_viewport);
        RS_Polyline *createdPolyline = m.addPolylineNode(
            *m_polylineToModify,
            (RS_AtomicEntity &) *m_addSegment,
            *m_addCoord);
        if (createdPolyline != nullptr){
            m_polylineToModify = createdPolyline;
        }
        *m_addCoord = {};
    }
}

void RS_ActionPolylineAdd::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case ChooseSegment: {
            deleteSnapper();
            auto polyline = dynamic_cast<RS_Polyline *>(catchAndDescribe(e, RS2::EntityPolyline));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetAddCoord: {
            bool oldSnapOnEntity = m_snapMode.snapOnEntity;
            m_snapMode.snapOnEntity = true;
            RS_Vector snap = e->snapPoint;
            m_snapMode.snapOnEntity = oldSnapOnEntity;
            auto polyline = dynamic_cast<RS_Polyline *>(catchEntityByEvent(e, RS2::EntityPolyline));
            if (polyline == m_polylineToModify){
                RS_Vector coordinate = polyline->getNearestPointOnEntity(snap, true);
                previewRefSelectablePoint(coordinate);
                RS_Entity * segment = catchAndDescribe(coordinate, RS2::ResolveAll);
                highlightHover(segment);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionPolylineAdd::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case ChooseSegment: {
            auto en = catchEntityByEvent(e);
            if (!en){
                commandMessage(tr("No Entity found."));
            } else if (!isPolyline(en)){
                commandMessage(tr("Entity must be a polyline."));
            } else {
                m_polylineToModify = dynamic_cast<RS_Polyline *>(en);
                m_polylineToModify->setSelected(true);
                redraw();
                setStatus(SetAddCoord);
            }
            break;
        }
        case SetAddCoord: {
            bool oldSnapOnEntity = m_snapMode.snapOnEntity;
            m_snapMode.snapOnEntity = true;
            RS_Vector snap = e->snapPoint;
            m_snapMode.snapOnEntity = oldSnapOnEntity;

            const RS_Vector newCoord = m_polylineToModify->getNearestPointOnEntity(snap, true);
            *m_addCoord = newCoord;
            if (!m_polylineToModify){
                commandMessage(tr("No Entity found."));
            } else if (!m_addCoord->valid){
                commandMessage(tr("Adding point is invalid."));
            } else {
                m_addSegment = nullptr;
                m_addSegment = catchEntity(newCoord, RS2::ResolveAll);
                if (!m_addSegment){
                    commandMessage(tr("Adding point is not on entity."));
                    break;
                }
                deleteSnapper();
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionPolylineAdd::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] LC_MouseEvent *e) {
    deleteSnapper();
    finish(true);
}

void RS_ActionPolylineAdd::finish(bool updateTB){
    if (m_polylineToModify){
        m_polylineToModify->setSelected(false);
        redraw();
        m_polylineToModify = nullptr;
        m_addSegment = nullptr;
        *m_addCoord = {};
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionPolylineAdd::updateMouseButtonHints(){
    switch (getStatus()) {
        case ChooseSegment:
            updateMouseWidgetTRCancel(tr("Specify polyline to add nodes"));
            break;
        case SetAddCoord:
            updateMouseWidgetTRBack(tr("Specify adding node's point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionPolylineAdd::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
