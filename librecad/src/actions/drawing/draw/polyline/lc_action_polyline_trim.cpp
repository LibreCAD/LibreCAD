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

#include "lc_action_polyline_trim.h"

#include "lc_actioncontext.h"
#include "rs_atomicentity.h"
#include "rs_document.h"
#include "rs_modification.h"
#include "rs_polyline.h"

LC_ActionPolylineTrim::LC_ActionPolylineTrim(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionPolylineTrim",actionContext, RS2::ActionPolylineTrim) {
}

void LC_ActionPolylineTrim::init(const int status) {
    m_polylineToModify = nullptr;
    m_segment1 = nullptr;
    m_segment2 = nullptr;
    RS_PreviewActionInterface::init(status);
}

void LC_ActionPolylineTrim::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    setPolylineToModify(contextEntity);
}

bool LC_ActionPolylineTrim::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const auto newPolyline = RS_Modification::polylineTrim(m_polylineToModify, *m_segment1, *m_segment2, ctx);
    if (newPolyline != nullptr){
        newPolyline->setLayer(m_polylineToModify->getLayer(false));
        newPolyline->setPen(m_polylineToModify->getPen(false));
        m_polylineToModify = newPolyline;
        ctx.dontSetActiveLayerAndPen();
        select(m_polylineToModify);
    }
    return false;
}

void LC_ActionPolylineTrim::doTriggerCompletion([[maybe_unused]]bool success) {
    m_segment1 = nullptr;
    m_segment2 = nullptr;
    setStatus(SetSegment1);
}

void LC_ActionPolylineTrim::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case ChooseEntity: {
            deleteSnapper();
            const RS_Entity *pl = catchAndDescribe(e, RS2::EntityPolyline);
            if (pl != nullptr){
                highlightHover(pl);
            }
            break;
        }
        case SetSegment1:{
            const RS_Entity* en = catchEntityByEvent(e, RS2::ResolveAll);
            if (en != nullptr){
                if (en->getParent() == m_polylineToModify){
                    highlightHover(en);
                    previewRefSelectablePoint(en->getStartpoint());
                    previewRefSelectablePoint(en->getEndpoint());
                }
            }
            break;
        }
        case SetSegment2:{
            highlightSelected(m_segment1);
            RS_Entity* en = catchEntityByEvent(e, RS2::ResolveAll);
            if (isAtomic(en) && en->getParent() == m_polylineToModify && en != m_segment1){
                const auto candidate = static_cast<RS_AtomicEntity *>(en);

                previewRefPoint(m_segment1->getStartpoint());
                previewRefPoint(m_segment1->getEndpoint());

                LC_DocumentModificationBatch ctx;
                const auto polyline = RS_Modification::polylineTrim(m_polylineToModify, *m_segment1, *candidate, ctx);
                if (polyline != nullptr){
                    highlightHover(en);
                    previewEntity(polyline);
                    previewRefSelectablePoint(candidate->getStartpoint());
                    previewRefSelectablePoint(candidate->getEndpoint());
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionPolylineTrim::setPolylineToModify(RS_Entity* en) {
    if (en == nullptr){
        commandMessage(tr("No Entity found."));
    } else if (en->rtti() != RS2::EntityPolyline){
        commandMessage(tr("Entity must be a polyline."));
    } else {
        m_polylineToModify = dynamic_cast<RS_Polyline *>(en);
        select(m_polylineToModify);
        redraw();
        setStatus(SetSegment1);
        redraw(); // fixme - why redraw twice??
    }
}

void LC_ActionPolylineTrim::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case ChooseEntity: {
            const auto en = catchEntityByEvent(e);
            setPolylineToModify(en);
            invalidateSnapSpot();
            break;
        }
        case SetSegment1:{
            RS_Entity *en = catchEntityByEvent(e, RS2::ResolveAll);
            if (isAtomic(en) &&  en->getParent() == m_polylineToModify){
                m_segment1 = dynamic_cast<RS_AtomicEntity *>(en);
                setStatus(SetSegment2);
            }
            else{
                commandMessage(tr("First segment should be on selected polyline."));
            }
            break;
        }
        case SetSegment2: {
            RS_Entity *en = catchEntityByEvent(e, RS2::ResolveAll);
            if (isAtomic(en) && en->getParent() == m_polylineToModify && en != m_segment1){
                m_segment2 = dynamic_cast<RS_AtomicEntity *>(en);
                deleteSnapper();
                trigger();
            }
            else{
                commandMessage(tr("Second segment should be on selected polyline and not equal to first one."));
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionPolylineTrim::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deleteSnapper();
    deletePreview();
    const int newStatus = status - 1;
    if (newStatus == ChooseEntity){
        if (m_polylineToModify != nullptr){
            unselect(m_polylineToModify);
            redraw();
        }
    }
    setStatus(newStatus);
}

void LC_ActionPolylineTrim::finish(){
    if (m_polylineToModify != nullptr){
        unselect(m_polylineToModify);
        redraw();
    }
    RS_PreviewActionInterface::finish();
}

void LC_ActionPolylineTrim::updateActionPrompt(){
    switch (getStatus()) {
        case ChooseEntity:
            updatePromptTRCancel(tr("Specify polyline to trim"));
            break;
        case SetSegment1:
            updatePromptTRBack(tr("Specify first segment"));
            break;
        case SetSegment2:
            updatePromptTRBack(tr("Specify second segment"));
            break;
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType LC_ActionPolylineTrim::doGetMouseCursor([[maybe_unused]] int status){
     return RS2::SelectCursor;
}
