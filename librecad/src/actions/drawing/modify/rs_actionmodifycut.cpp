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

#include "rs_actionmodifycut.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "lc_linemath.h"

RS_ActionModifyCut::RS_ActionModifyCut(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Cut Entity",
                               container, graphicView), cutEntity(nullptr), cutCoord(new RS_Vector{}){
    actionType = RS2::ActionModifyCut;
}

RS_ActionModifyCut::~RS_ActionModifyCut() = default;

void RS_ActionModifyCut::init(int status){
    RS_PreviewActionInterface::init(status);
}

void RS_ActionModifyCut::trigger(){

    RS_DEBUG->print("RS_ActionModifyCut::trigger()");

    if (cutEntity && cutEntity->isAtomic() && cutCoord->valid &&
        cutEntity->isPointOnEntity(*cutCoord)){

        cutEntity->setHighlighted(false);
        graphicView->drawEntity(cutEntity);

        RS_Modification m(*container, graphicView);
        m.cut(*cutCoord, (RS_AtomicEntity *) cutEntity);

        deleteSnapper();
        cutEntity = nullptr;
        *cutCoord = RS_Vector(false);
        setStatus(ChooseCutEntity);
        updateSelectionWidget();
    }
}

void RS_ActionModifyCut::finish(bool updateTB){
    cutEntity = nullptr;
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionModifyCut::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionModifyCut::mouseMoveEvent begin");
    RS_Vector snap = snapPoint(e);
    deleteHighlights();
    deletePreview();
    switch (getStatus()) {
        case ChooseCutEntity: {
            deleteSnapper();
            auto en = catchEntity(e);
            if (en != nullptr &&  en->trimmable()){
                highlightHover(en);
                RS_Vector nearest = en->getNearestPointOnEntity(snap, true);
                previewRefSelectablePoint(nearest);
            }
            break;
        }
        case SetCutCoord: {
            highlightSelected(cutEntity);
            RS_Vector nearest = cutEntity->getNearestPointOnEntity(snap, true);
            previewRefSelectablePoint(nearest);
            break;
        }
        default:
            break;
    }
    drawPreview();
    drawHighlights();
    RS_DEBUG->print("RS_ActionModifyTrim::mouseMoveEvent end");
}

void RS_ActionModifyCut::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case ChooseCutEntity: {
            cutEntity = catchEntity(e);
            if (cutEntity == nullptr){
                commandMessage(tr("No Entity found."));
            } else if (cutEntity->trimmable()){
                setStatus(SetCutCoord);
            } else
                commandMessage(tr("Entity must be a line, arc, circle, ellipse or interpolation spline."));
            break;
        }
        case SetCutCoord: {
            RS_Vector snap = snapPoint(e);
            RS_Vector nearest = cutEntity->getNearestPointOnEntity(snap, true);
            if (LC_LineMath::isNotMeaningfulDistance(cutEntity->getStartpoint(), nearest) ||
                LC_LineMath::isNotMeaningfulDistance(cutEntity->getEndpoint(), nearest)){
                commandMessage(tr("Cutting point may not be entity's endpoint."));
            } else {
                *cutCoord = nearest;
                trigger();
                deleteSnapper();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyCut::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
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
