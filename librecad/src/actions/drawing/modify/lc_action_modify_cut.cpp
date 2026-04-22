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

#include "lc_action_modify_cut.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_linemath.h"
#include "rs_atomicentity.h"
#include "rs_document.h"
#include "rs_entity.h"
#include "rs_modification.h"

LC_ActionModifyCut::LC_ActionModifyCut(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionModifyCut",actionContext, RS2::ActionModifyCut), m_cutCoord(new RS_Vector{}){
}

LC_ActionModifyCut::~LC_ActionModifyCut() = default;

void LC_ActionModifyCut::init(const int status){
    RS_PreviewActionInterface::init(status);
}

void LC_ActionModifyCut::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (contextEntity->trimmable()) {
        m_cutEntity = contextEntity;
        setStatus(SetCutCoord);
    }
}

bool LC_ActionModifyCut::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (isAtomic(m_cutEntity) && m_cutCoord->valid && m_cutEntity->isPointOnEntity(*m_cutCoord)){
        m_cutEntity->setHighlighted(false);
        RS_Modification::cut(*m_cutCoord, static_cast<RS_AtomicEntity*>(m_cutEntity), ctx);
        const RS_Pen &originalPen = m_cutEntity->getPen(false);
        RS_Layer *originalLayer = m_cutEntity->getLayer(false);
        for (const auto e: std::as_const(ctx.entitiesToAdd)) {
            e->setPen(originalPen);
            e->setLayer(originalLayer);
        }
        ctx.dontSetActiveLayerAndPen();
        return true;
    }
    return false;
}

void LC_ActionModifyCut::doTriggerCompletion(const bool success) {
    if (success) {
        m_cutEntity = nullptr;
        *m_cutCoord = RS_Vector(false);
        setStatus(ChooseCutEntity);
    }
}

bool LC_ActionModifyCut::isInVisualSnapStatus(int status) {
    return (status == SetCutCoord);
}

void LC_ActionModifyCut::finish(){
    m_cutEntity = nullptr;
    RS_PreviewActionInterface::finish();
}

void LC_ActionModifyCut::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector snap = e->snapPoint;
    switch (status) {
        case ChooseCutEntity: {
            deleteSnapper();
            const auto en = catchAndDescribe(e);
            if (en != nullptr &&  en->trimmable()){
                highlightHover(en);
                const RS_Vector nearest = en->getNearestPointOnEntity(snap, true);
                previewRefSelectablePoint(nearest);
            }
            break;
        }
        case SetCutCoord: {
            highlightSelected(m_cutEntity);
            const RS_Vector nearest = m_cutEntity->getNearestPointOnEntity(snap, true);
            previewRefSelectablePoint(nearest);
            // todo - is description for selected entity necessary there?
            if (isInfoCursorForModificationEnabled()){
                msg(tr("Divide"))
                    .vector(tr("At:"), nearest)
                    .toInfoCursorZone2(false);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyCut::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case ChooseCutEntity: {
            m_cutEntity = catchEntityByEvent(e);
            if (m_cutEntity == nullptr){
                commandMessage(tr("No Entity found."));
            } else if (m_cutEntity->trimmable()){
                setStatus(SetCutCoord);
            } else {
                commandMessage(tr("Entity must be a line, arc, circle, ellipse or interpolation spline."));
            }
            break;
        }
        case SetCutCoord: {
            const RS_Vector snap = e->snapPoint;
            const RS_Vector nearest = m_cutEntity->getNearestPointOnEntity(snap, true);
            if (LC_LineMath::isNotMeaningfulDistance(m_cutEntity->getStartpoint(), nearest) ||
                LC_LineMath::isNotMeaningfulDistance(m_cutEntity->getEndpoint(), nearest)){
                commandMessage(tr("Cutting point may not be entity's endpoint."));
            } else {
                *m_cutCoord = nearest;
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyCut::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

void LC_ActionModifyCut::updateActionPrompt(){
    switch (getStatus()) {
        case ChooseCutEntity:
            updatePromptTRCancel(tr("Specify entity to cut"));
            break;
        case SetCutCoord:
            updatePromptTRBack(tr("Specify cutting point"));
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionModifyCut::doGetMouseCursor([[maybe_unused]] const int status){
    switch (status) {
        case ChooseCutEntity:
            return RS2::SelectCursor;
        case SetCutCoord:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}
