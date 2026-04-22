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

#include "lc_action_polyline_add.h"

#include "lc_actioncontext.h"
#include "rs_atomicentity.h"
#include "rs_document.h"
#include "rs_entity.h"
#include "rs_modification.h"
#include "rs_polyline.h"

LC_ActionPolylineAdd::LC_ActionPolylineAdd(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionPolylineAdd", actionContext, RS2::ActionPolylineAdd)
    , m_addCoord(std::make_unique<RS_Vector>()){
}

LC_ActionPolylineAdd::~LC_ActionPolylineAdd() = default;


void LC_ActionPolylineAdd::init(const int status) {
    m_polylineToModify = nullptr;
    m_addSegment = nullptr;
    *m_addCoord = {};
    RS_PreviewActionInterface::init(status);
}

void LC_ActionPolylineAdd::doInitWithContextEntity(RS_Entity* contextEntity,[[maybe_unused]] const RS_Vector& pos) {
    setPolylineToModify(contextEntity);
}

bool LC_ActionPolylineAdd::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (m_polylineToModify != nullptr && m_addSegment->isAtomic() && m_addCoord->valid &&
        m_addSegment->isPointOnEntity(*m_addCoord)) {
        const auto segment                 = static_cast<RS_AtomicEntity*>(m_addSegment);
        RS_Polyline* createdPolyline = RS_Modification::addPolylineNode(m_polylineToModify, *segment, *m_addCoord, ctx);
        if (createdPolyline != nullptr){
            createdPolyline->setLayer(m_polylineToModify->getLayer(false));
            createdPolyline->setPen(m_polylineToModify->getPen(false));
            m_polylineToModify = createdPolyline;
        }
        ctx.dontSetActiveLayerAndPen();
        *m_addCoord = {};
        return true;
    }
    return false;
}

void LC_ActionPolylineAdd::doTriggerCompletion([[maybe_unused]]bool success) {
    select(m_polylineToModify);
}

bool LC_ActionPolylineAdd::isInVisualSnapStatus(int status) {
    return LC_UndoableDocumentModificationAction::isInVisualSnapStatus(status) || (status == SetAddCoord);
}

void LC_ActionPolylineAdd::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case ChooseSegment: {
            deleteSnapper();
            const auto polyline = dynamic_cast<RS_Polyline *>(catchAndDescribe(e, RS2::EntityPolyline));
            if (polyline != nullptr){
                highlightHover(polyline);
            }
            break;
        }
        case SetAddCoord: {
            const bool oldSnapOnEntity = m_snapMode.snapOnEntity; // fixme - sand - what for? review
            m_snapMode.snapOnEntity = true;
            m_snapMode.snapOnEntity = oldSnapOnEntity;
            const auto polyline = static_cast<RS_Polyline *>(catchEntityByEvent(e, RS2::EntityPolyline));
            if (polyline == m_polylineToModify){
                const RS_Vector snap = e->snapPoint;
                const RS_Vector coordinate = polyline->getNearestPointOnEntity(snap, true);
                previewRefSelectablePoint(coordinate);
                const RS_Entity * segment = catchAndDescribe(coordinate, RS2::ResolveAll);
                highlightHover(segment);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionPolylineAdd::setPolylineToModify(RS_Entity* en) {
    if (en == nullptr){
        commandMessage(tr("No Entity found."));
    } else if (!isPolyline(en)){
        commandMessage(tr("Entity must be a polyline."));
    } else {
        m_polylineToModify = dynamic_cast<RS_Polyline *>(en);
        select(m_polylineToModify);
        redraw();
        setStatus(SetAddCoord);
    }
}

void LC_ActionPolylineAdd::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case ChooseSegment: {
            const auto en = catchEntityByEvent(e);
            setPolylineToModify(en);
            break;
        }
        case SetAddCoord: {
            const bool oldSnapOnEntity = m_snapMode.snapOnEntity; // fixme - sand - what for? review
            m_snapMode.snapOnEntity = true;
            m_snapMode.snapOnEntity = oldSnapOnEntity;

            if (m_polylineToModify == nullptr){
                commandMessage(tr("No Entity found."));
            }
            else {
                const RS_Vector snap = e->snapPoint;
                const RS_Vector newCoord = m_polylineToModify->getNearestPointOnEntity(snap, true);
                *m_addCoord = newCoord;
                if (!m_addCoord->valid){
                    commandMessage(tr("Adding point is invalid."));
                } else {
                    m_addSegment = nullptr;
                    m_addSegment = catchEntity(newCoord, RS2::ResolveAll);
                    if (m_addSegment == nullptr){
                        commandMessage(tr("Adding point is not on entity."));
                        break;
                    }
                    deleteSnapper();
                    trigger();
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionPolylineAdd::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deleteSnapper();
    finish();
}

void LC_ActionPolylineAdd::finish(){
    if (m_polylineToModify != nullptr){
        unselect(m_polylineToModify);
        redraw();
        m_polylineToModify = nullptr;
        m_addSegment = nullptr;
        *m_addCoord = {};
    }
    RS_PreviewActionInterface::finish();
}

void LC_ActionPolylineAdd::updateActionPrompt(){
    switch (getStatus()) {
        case ChooseSegment:
            updatePromptTRCancel(tr("Specify polyline to add nodes"));
            break;
        case SetAddCoord:
            updatePromptTRBack(tr("Specify adding node's point"));
            break;
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType LC_ActionPolylineAdd::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
