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


#include <QMouseEvent>

#include "rs_actionpolylinetrim.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_modification.h"
#include "rs_polyline.h"

RS_ActionPolylineTrim::RS_ActionPolylineTrim(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Trim segments",
						   container, graphicView) {
	actionType=RS2::ActionPolylineTrim;
}

void RS_ActionPolylineTrim::init(int status) {
    RS_PreviewActionInterface::init(status);
    polylineToModify = nullptr;
    Segment1 = Segment2 = nullptr;
}

void RS_ActionPolylineTrim::trigger(){

    RS_DEBUG->print("RS_ActionPolylineTrim::trigger()");

    polylineToModify->setSelected(false);
    graphicView->drawEntity(polylineToModify);

    RS_Modification m(*container, graphicView);
    auto newPolyline = m.polylineTrim((RS_Polyline &) *polylineToModify, *Segment1, *Segment2, false);
    if (newPolyline != nullptr){
        polylineToModify = newPolyline;
        Segment1 = Segment2 = nullptr;
        setStatus(SetSegment1);
    }

    updateSelectionWidget();

    graphicView->redraw();
}

void RS_ActionPolylineTrim::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionPolylineTrim::mouseMoveEvent begin");
    snapPoint(e);
    deleteHighlights();
    switch (getStatus()) {
        case ChooseEntity: {
            RS_Entity *pl = catchEntity(e, RS2::EntityPolyline);
            if (pl != nullptr){
                highlightHover(pl);
            }
            break;
        }
        case SetSegment1:{
            RS_Entity* en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr){
                if (en->getParent() == polylineToModify){
                    highlightHover(en);
                    deletePreview();
                    previewRefSelectablePoint(en->getStartpoint());
                    previewRefSelectablePoint(en->getEndpoint());
                    drawPreview();
                }
            }
            break;
        }
        case SetSegment2:{
            highlightSelected(Segment1);
            deletePreview();
            RS_Entity* en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr){
                if (en->getParent() == polylineToModify){
                    if (en != Segment1){
                        if (en->isAtomic()){
                            auto candidate = dynamic_cast<RS_AtomicEntity *>(en);

                            previewRefPoint(Segment1->getStartpoint());
                            previewRefPoint(Segment1->getEndpoint());

                            RS_Modification m(*preview, graphicView);
                            auto polyline = m.polylineTrim((RS_Polyline &) *polylineToModify, *Segment1, *candidate, true);
                            if (polyline != nullptr){
                                highlightHover(en);
                                previewRefSelectablePoint(candidate->getStartpoint());
                                previewRefSelectablePoint(candidate->getEndpoint());
                            }
                        }
                    }
                }
            }
            drawPreview();
            break;
        }
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionPolylineTrim::mouseMoveEvent end");
}

void RS_ActionPolylineTrim::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case ChooseEntity: {
            auto en = catchEntity(e);
            if (en == nullptr){
                commandMessage(tr("No Entity found."));
            } else if (en->rtti() != RS2::EntityPolyline){
                commandMessage(tr("Entity must be a polyline."));
            } else {
                polylineToModify = dynamic_cast<RS_Polyline *>(en);
                polylineToModify->setSelected(true);
                graphicView->drawEntity(polylineToModify);
                setStatus(SetSegment1);
                graphicView->redraw();
            }
            break;
        }
        case SetSegment1:{
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr &&  en->getParent() == polylineToModify && en->isAtomic()){
                Segment1 = dynamic_cast<RS_AtomicEntity *>(en);
                setStatus(SetSegment2);
            }
            else{
                commandMessage(tr("First segment should be on selected polyline."));
            }
            break;
        }
        case SetSegment2: {
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr &&  en->getParent() == polylineToModify && en->isAtomic() && en != Segment1){
                Segment2 = dynamic_cast<RS_AtomicEntity *>(en);
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

void RS_ActionPolylineTrim::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deleteSnapper();
    deletePreview();
    int newStatus = status - 1;
    if (newStatus == ChooseEntity){
        if (polylineToModify){
            polylineToModify->setSelected(false);
            graphicView->redraw();
        }
    }
    setStatus(newStatus);
}

void RS_ActionPolylineTrim::finish(bool updateTB){
    if (polylineToModify){
        polylineToModify->setSelected(false);
        graphicView->redraw();
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
