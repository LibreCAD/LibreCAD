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

#include "rs_actionmodifycut.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_linemath.h"
#include "rs_atomicentity.h"
#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_modification.h"

RS_ActionModifyCut::RS_ActionModifyCut(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Cut Entity",actionContext, RS2::ActionModifyCut)
    ,m_cutEntity(nullptr), m_cutCoord(new RS_Vector{}){
}

RS_ActionModifyCut::~RS_ActionModifyCut() = default;

void RS_ActionModifyCut::init(int status){
    RS_PreviewActionInterface::init(status);
}

void RS_ActionModifyCut::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    if (contextEntity->trimmable()) {
        m_cutEntity = contextEntity;
        setStatus(SetCutCoord);
    }
}

void RS_ActionModifyCut::doTrigger() {
    RS_DEBUG->print("RS_ActionModifyCut::trigger()");

    if (isAtomic(m_cutEntity) && m_cutCoord->valid && m_cutEntity->isPointOnEntity(*m_cutCoord)){
        m_cutEntity->setHighlighted(false);

        RS_Modification m(*m_container, m_viewport);
        m.cut(*m_cutCoord, static_cast<RS_AtomicEntity*>(m_cutEntity));

        m_cutEntity = nullptr;
        *m_cutCoord = RS_Vector(false);
        setStatus(ChooseCutEntity);
    }
}

void RS_ActionModifyCut::finish(bool updateTB){
    m_cutEntity = nullptr;
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionModifyCut::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case ChooseCutEntity: {
            deleteSnapper();
            auto en = catchAndDescribe(e);
            if (en != nullptr &&  en->trimmable()){
                highlightHover(en);
                RS_Vector nearest = en->getNearestPointOnEntity(snap, true);
                previewRefSelectablePoint(nearest);
            }
            break;
        }
        case SetCutCoord: {
            highlightSelected(m_cutEntity);
            RS_Vector nearest = m_cutEntity->getNearestPointOnEntity(snap, true);
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

void RS_ActionModifyCut::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case ChooseCutEntity: {
            m_cutEntity = catchEntityByEvent(e);
            if (m_cutEntity == nullptr){
                commandMessage(tr("No Entity found."));
            } else if (m_cutEntity->trimmable()){
                setStatus(SetCutCoord);
            } else
                commandMessage(tr("Entity must be a line, arc, circle, ellipse or interpolation spline."));
            break;
        }
        case SetCutCoord: {
            RS_Vector snap = e->snapPoint;
            RS_Vector nearest = m_cutEntity->getNearestPointOnEntity(snap, true);
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

void RS_ActionModifyCut::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    initPrevious(status);
}

void RS_ActionModifyCut::updateMouseButtonHints(){
    switch (getStatus()) {
        case ChooseCutEntity:
            updateMouseWidgetTRCancel(tr("Specify entity to cut"));
            break;
        case SetCutCoord:
            updateMouseWidgetTRBack(tr("Specify cutting point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionModifyCut::doGetMouseCursor([[maybe_unused]] int status){
    switch (status) {
        case ChooseCutEntity:
            return RS2::SelectCursor;
        case SetCutCoord:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}
