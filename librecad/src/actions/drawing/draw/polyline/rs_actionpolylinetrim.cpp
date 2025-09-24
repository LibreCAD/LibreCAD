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

#include "rs_actionpolylinetrim.h"

#include "lc_actioncontext.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"

RS_ActionPolylineTrim::RS_ActionPolylineTrim(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Trim segments",actionContext, RS2::ActionPolylineTrim) {
}

void RS_ActionPolylineTrim::init(int status) {
    m_polylineToModify = nullptr;
    m_segment1 = m_segment2 = nullptr;
    RS_PreviewActionInterface::init(status);
}

void RS_ActionPolylineTrim::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    setPolylineToModify(contextEntity);
}

void RS_ActionPolylineTrim::doTrigger() {
    RS_DEBUG->print("RS_ActionPolylineTrim::trigger()");

    m_polylineToModify->setSelected(false);

    RS_Modification m(*m_container, m_viewport);
    auto newPolyline = m.polylineTrim((RS_Polyline &) *m_polylineToModify, *m_segment1, *m_segment2, false);
    if (newPolyline != nullptr){
        m_polylineToModify = newPolyline;
        m_segment1 = m_segment2 = nullptr;
        setStatus(SetSegment1);
    }
}

void RS_ActionPolylineTrim::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status) {
        case ChooseEntity: {
            deleteSnapper();
            RS_Entity *pl = catchAndDescribe(e, RS2::EntityPolyline);
            if (pl != nullptr){
                highlightHover(pl);
            }
            break;
        }
        case SetSegment1:{
            RS_Entity* en = catchEntityByEvent(e, RS2::ResolveAll);
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
                auto candidate = dynamic_cast<RS_AtomicEntity *>(en);

                previewRefPoint(m_segment1->getStartpoint());
                previewRefPoint(m_segment1->getEndpoint());

                RS_Modification m(*m_preview, m_viewport);
                auto polyline = m.polylineTrim((RS_Polyline &) *m_polylineToModify, *m_segment1, *candidate, true);
                if (polyline != nullptr){
                    highlightHover(en);
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

void RS_ActionPolylineTrim::setPolylineToModify(RS_Entity* en) {
    if (en == nullptr){
        commandMessage(tr("No Entity found."));
    } else if (en->rtti() != RS2::EntityPolyline){
        commandMessage(tr("Entity must be a polyline."));
    } else {
        m_polylineToModify = dynamic_cast<RS_Polyline *>(en);
        m_polylineToModify->setSelected(true);
        redraw();
        setStatus(SetSegment1);
        redraw(); // fixme - why redraw twice??
    }
}

void RS_ActionPolylineTrim::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case ChooseEntity: {
            auto en = catchEntityByEvent(e);
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

void RS_ActionPolylineTrim::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deleteSnapper();
    deletePreview();
    int newStatus = status - 1;
    if (newStatus == ChooseEntity){
        if (m_polylineToModify){
            m_polylineToModify->setSelected(false);
            redraw();
        }
    }
    setStatus(newStatus);
}

void RS_ActionPolylineTrim::finish(bool updateTB){
    if (m_polylineToModify){
        m_polylineToModify->setSelected(false);
        redraw();
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionPolylineTrim::updateMouseButtonHints(){
    switch (getStatus()) {
        case ChooseEntity:
            updateMouseWidgetTRCancel(tr("Specify polyline to trim"));
            break;
        case SetSegment1:
            updateMouseWidgetTRBack(tr("Specify first segment"));
            break;
        case SetSegment2:
            updateMouseWidgetTRBack(tr("Specify second segment"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionPolylineTrim::doGetMouseCursor([[maybe_unused]] int status){
     return RS2::SelectCursor;
}
