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

#include "rs_graphicview.h"
#include "rs_document.h"
#include "lc_actionmodifyalignsingle.h"
#include "lc_modifyalignoptions.h"

LC_ActionModifyAlignSingle::LC_ActionModifyAlignSingle( RS_EntityContainer &container,RS_GraphicView &graphicView)
  :RS_PreviewActionInterface("ModifyAlignSingle", container, graphicView){
    actionType = RS2::ActionModifyAlignOne;
}

void LC_ActionModifyAlignSingle::init(int status) {
    RS_PreviewActionInterface::init(status);
//    showOptions();
}

void LC_ActionModifyAlignSingle::trigger() {
    RS_PreviewActionInterface::trigger();
    if (entityToAlign != nullptr) {
        if (document != nullptr) {
            RS_Vector target = LC_Align::getReferencePoint(alignMin, alignMax, hAlign, vAlign);
            RS_Entity *clone = LC_Align::createCloneMovedToTarget(entityToAlign, target, true, hAlign, vAlign);
            if (clone != nullptr) {
                document->startUndoCycle();
                clone->setSelected(false);
                container->addEntity(clone);
                document->addUndoable(clone);
                deleteEntityUndoable(entityToAlign);
                document->endUndoCycle();
            }
        }
    }
    entityToAlign = nullptr;
    deletePreview();
    if (finishActionAfterTrigger){
        setStatus(-1);
    }
    else {
        if (showRefEntitiesOnPreview) {
            previewAlignRefPoint(alignMin, alignMax);
        }
    }
    drawPreview();
    graphicView->redraw();
}

void LC_ActionModifyAlignSingle::mouseMoveEvent(QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    deletePreview();
    deleteHighlights();
    switch (getStatus()){
        case SetRefPoint:{
            RS_Vector min;
            RS_Vector max;
            bool showPreview = true;
            switch (alignType) {
                case LC_Align::ENTITY: {
                    RS_Entity *entity = catchEntity(snap);
                    if (entity != nullptr) {
                        min = entity->getMin();
                        max = entity->getMax();
                        highlightHover(entity);
                    } else {
                        showPreview = false;
                    }
                    break;
                }
                case LC_Align::POSITION: {
                    snap = getRelZeroAwarePoint(e, snap);
                    min = snap;
                    max = snap;
                    break;
                }
                case LC_Align::DRAWING: {
                    min = container->getMin();
                    max = container->getMax();
                    break;
                }
                default:
                    break;
            }
            if (showPreview) {
                previewAlignRefPoint(min, max);
            }
            break;
        }
        case SelectEntity:{
            RS_Entity* entity = catchEntity(e);
            if (showRefEntitiesOnPreview){
                previewAlignRefPoint(alignMin, alignMax);
            }
            if (baseAlignEntity != nullptr) {
                highlightSelected(baseAlignEntity);
            }
            if (entity != nullptr){
                highlightHover(entity);
                RS_Vector target = LC_Align::getReferencePoint(alignMin, alignMax, hAlign, vAlign);
                RS_Entity* clone = LC_Align::createCloneMovedToTarget(entity, target, false, hAlign, vAlign);
                if (clone != nullptr){
                    previewEntity(clone);
                }
            }
            break;
        }
    }
    drawPreview();
    drawHighlights();
}

void LC_ActionModifyAlignSingle::previewAlignRefPoint(const RS_Vector &min, const RS_Vector &max) {
    double verticalRef;
    bool drawVertical  = LC_Align::getVerticalRefCoordinate(min, max, hAlign, verticalRef);
    if (drawVertical) {
        double g0 = this->graphicView->toGraphY(0);
        double gHeight = this->graphicView->toGraphY(this->graphicView->getHeight());
        this->previewRefConstructionLine({verticalRef, g0}, {verticalRef, gHeight});
    }
    double horizontalRef;
    bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(min, max, vAlign, horizontalRef);
    if (drawHorizontal) {
        double g0 = this->graphicView->toGraphX(0);
        double gWidth = this->graphicView->toGraphX(this->graphicView->getWidth());
        this->previewRefConstructionLine({g0, horizontalRef}, {gWidth, horizontalRef});
    }
}

RS2::CursorType LC_ActionModifyAlignSingle::doGetMouseCursor([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

void LC_ActionModifyAlignSingle::updateMouseButtonHints() {
    switch (getStatus()){
        case SetRefPoint:{
            switch (alignType) {
                case LC_Align::ENTITY:
                    updateMouseWidgetTRCancel(tr("Select base alignment entity"));
                    break;
                case LC_Align::POSITION:
                    updateMouseWidgetTRCancel(tr("Specify base alignment point"), MOD_SHIFT_RELATIVE_ZERO);
                    break;
                case LC_Align::DRAWING:
//                    updateMouseWidgetTRCancel(tr("Click or Enter to finish align"));
                    updateMouseWidget();
                    break;
                default:
                    updateMouseWidget();
            }
            break;
        }
        case SelectEntity:{
            updateMouseWidgetTRBack(tr("Select entity to align"), MOD_CTRL(tr("Finish alignment after selection")));
            break;
        }
        default:
            updateMouseWidget();
    }
}

void LC_ActionModifyAlignSingle::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    switch (status){
        case SetRefPoint:{
            switch (alignType) {
                case LC_Align::ENTITY:{
                    RS_Entity *entity = catchEntity(e);
                    if (entity != nullptr) {
                        baseAlignEntity = entity;
                        alignMin = entity->getMin();
                        alignMax = entity->getMax();
                        setStatus(SelectEntity);
                    }
                    break;
                }
                case LC_Align::POSITION: {
                    snap = getRelZeroAwarePoint(e, snap);
                    alignMin = snap;
                    alignMax = snap;
                    setStatus(SelectEntity);
                    break;
                }
                case LC_Align::DRAWING:
                    alignMin = container->getMin();
                    alignMax = container->getMax();
                    setStatus(SelectEntity);
                    break;
                default:
                    break;
            }
            break;
        }
        case SelectEntity:{
            RS_Entity* entity = catchEntity(e);
            if (entity != nullptr){
                entityToAlign = entity;
                finishActionAfterTrigger = isControl(e);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyAlignSingle::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    switch (status){
        case SetRefPoint:{
            setStatus(-1);
            break;
        }
        case SelectEntity:{
            entityToAlign = nullptr;
            if (alignType == LC_Align::AlignMode::DRAWING){
                setStatus(-1);
            }
            else {
                setStatus(SetRefPoint);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyAlignSingle::onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) {
    if (status == SetRefPoint && !isZero && alignType == LC_Align::AlignMode::POSITION) {
        alignMin = pos;
        alignMax = pos;
        setStatus(SelectEntity);
    } else {
        commandMessage(tr("Coordinate is accepted only for Align to \"Position\""));
    }
}

LC_ActionOptionsWidget *LC_ActionModifyAlignSingle::createOptionsWidget() {
    return new LC_ModifyAlignOptions();
}

void LC_ActionModifyAlignSingle::setAlignType(int a) {
    if (a != alignType) {
        LC_ActionModifyAlignData::setAlignType(a);
        if (a == LC_Align::AlignMode::DRAWING){
            alignMin = container->getMin();
            alignMax = container->getMax();
            setStatus(SelectEntity);
        }
        else {
            setStatus(SetRefPoint);
        }
        updateMouseButtonHints();
    }
}
