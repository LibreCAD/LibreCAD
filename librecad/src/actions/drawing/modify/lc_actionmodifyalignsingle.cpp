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
#include "lc_actionmodifyalignsingle.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_modifyalignoptions.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"

LC_ActionModifyAlignSingle::LC_ActionModifyAlignSingle(LC_ActionContext *actionContext)
  :RS_PreviewActionInterface("ModifyAlignSingle", actionContext, RS2::ActionModifyAlignOne){
}

void LC_ActionModifyAlignSingle::init(int status) {
    if (m_viewport->hasUCS()){
        commandMessage(tr("Align action at the moment supports only World Coordinates system, and may not be invoked if User Coordinate System is active."));
        finish();
    }
    else {
        RS_PreviewActionInterface::init(status);
    }
}

void LC_ActionModifyAlignSingle::doTrigger() {
    if (m_entityToAlign != nullptr) {
        if (m_document != nullptr) {
            RS_Vector target = LC_Align::getReferencePoint(m_alignMin, m_alignMax, hAlign, vAlign);
            RS_Entity *clone = LC_Align::createCloneMovedToTarget(m_entityToAlign, target, true, hAlign, vAlign);
            if (clone != nullptr) {
                clone->setSelected(false);
                m_container->addEntity(clone);

                undoCycleReplace(m_entityToAlign, clone);
            }
        }
    }
    m_entityToAlign = nullptr;
    if (m_finishActionAfterTrigger){
        setStatus(-1);
    }
    else {
        if (m_showRefEntitiesOnPreview) {
            previewAlignRefPoint(m_alignMin, m_alignMax);
        }
    }
    drawPreview();
}

void LC_ActionModifyAlignSingle::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetRefPoint:{
            RS_Vector min;
            RS_Vector max;
            bool showPreview = true;
            switch (alignType) {
                case LC_Align::ENTITY: {
                    RS_Entity *entity = catchAndDescribe(snap);
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
                    min = m_container->getMin();
                    max = m_container->getMax();
                    break;
                }
                default:
                    break;
            }
            if (showPreview) {
                double verticalRef;
                bool drawVertical  = LC_Align::getVerticalRefCoordinate(min, max, hAlign, verticalRef);

                double horizontalRef;
                bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(min, max, vAlign, horizontalRef);

                previewRefLines(drawVertical, verticalRef, drawHorizontal, horizontalRef);

                // info cursor
                if (m_infoCursorOverlayPrefs->enabled){
                    QString msg = prepareInfoCursorMessage(verticalRef, drawVertical, horizontalRef, drawHorizontal);
                    appendInfoCursorZoneMessage(msg, 2, false);
                }
            }
            break;
        }
        case SelectEntity:{

            double verticalRef;
            bool drawVertical  = LC_Align::getVerticalRefCoordinate(m_alignMin, m_alignMax, hAlign, verticalRef);

            double horizontalRef;
            bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(m_alignMin, m_alignMax, vAlign, horizontalRef);

            if (m_showRefEntitiesOnPreview){
                previewRefLines(drawVertical, verticalRef, drawHorizontal, horizontalRef);
            }
            if (m_baseAlignEntity != nullptr) {
                highlightSelected(m_baseAlignEntity);
            }

            RS_Entity* entity = catchAndDescribe(e);
            RS_Vector offset;
            if (entity != nullptr){
                highlightHover(entity);
                RS_Vector target = LC_Align::getReferencePoint(m_alignMin, m_alignMax, hAlign, vAlign);

                RS_Vector entityRefPoint = LC_Align::getReferencePoint(entity->getMin(), entity->getMax(), hAlign, vAlign);
                offset = target - entityRefPoint;

                RS_Entity* clone =  LC_Align::createCloneMovedToOffset(entity, offset, false);
                if (clone != nullptr){
                    previewEntity(clone);
                }
            }

            if (isInfoCursorForModificationEnabled()){
                QString message = prepareInfoCursorMessage(verticalRef, drawVertical, horizontalRef, drawHorizontal);

                auto builder = msg(message);
                if (entity != nullptr){
                    builder.string(tr("Offset:"))
                           .relative(offset)
                           .relativePolar(offset);
                }
                builder.toInfoCursorZone2(false);
            }
            break;
        }
    }
}

QString LC_ActionModifyAlignSingle::prepareInfoCursorMessage(double verticalRef, bool drawVertical, double horizontalRef, bool drawHorizontal) {
    QString msg = tr("Align to ");
    switch (alignType) {
        case LC_Align::ENTITY:{
            msg.append(tr("Entity"));
            break;
        }
        case LC_Align::POSITION:{
            msg.append(tr("Position"));
            break;
        }
        case LC_Align::DRAWING:{
            msg.append(tr("Drawing"));
            break;
        }
    }
    msg.append("\n");
    msg.append(tr("Reference: "));
    if (drawVertical){
        msg.append("X: ");
        msg.append(formatLinear(verticalRef));
        msg.append(" ");
    }

    if (drawHorizontal){
        msg.append("Y: ");
        msg.append(formatLinear(horizontalRef));
    }
    return msg;
}

void LC_ActionModifyAlignSingle::previewRefLines(bool drawVertical, double verticalRef, bool drawHorizontal, double horizontalRef) {
    // NOTE:
    // AS Action so far do not support UCS, coordinates below will be in WCS despite used methods.
    RS_Vector wcsLeftBottom = m_viewport->getUCSViewLeftBottom();
    RS_Vector wcsRightTop = m_viewport->getUCSViewRightTop();
    if (drawVertical) {
        previewRefConstructionLine({verticalRef, wcsLeftBottom.y}, {verticalRef, wcsRightTop.y});
    }
    if (drawHorizontal) {
        previewRefConstructionLine({wcsLeftBottom.x, horizontalRef}, {wcsRightTop.x, horizontalRef});
    }
}

void LC_ActionModifyAlignSingle::previewAlignRefPoint(const RS_Vector &min, const RS_Vector &max) {
    double verticalRef;
    bool drawVertical  = LC_Align::getVerticalRefCoordinate(min, max, hAlign, verticalRef);
    RS_Vector wcsLeftBottom = m_viewport->getUCSViewLeftBottom();
    RS_Vector wcsRightTop = m_viewport->getUCSViewRightTop();
    if (drawVertical) {
        // NOTE:: works properly for WCS only
        previewRefConstructionLine({verticalRef, wcsLeftBottom.y}, {verticalRef, wcsRightTop.y});
    }
    double horizontalRef;
    bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(min, max, vAlign, horizontalRef);
    if (drawHorizontal) {
        // NOTE:: works properly for WCS only
        previewRefConstructionLine({wcsLeftBottom.x, horizontalRef}, {wcsRightTop.x, horizontalRef});
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

void LC_ActionModifyAlignSingle::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status){
        case SetRefPoint:{
            switch (alignType) {
                case LC_Align::ENTITY:{
                    RS_Entity *entity = catchEntityByEvent(e);
                    if (entity != nullptr) {
                        m_baseAlignEntity = entity;
                        m_alignMin = entity->getMin();
                        m_alignMax = entity->getMax();
                        setStatus(SelectEntity);
                    }
                    break;
                }
                case LC_Align::POSITION: {
                    snap = getRelZeroAwarePoint(e, snap);
                    m_alignMin = snap;
                    m_alignMax = snap;
                    setStatus(SelectEntity);
                    break;
                }
                case LC_Align::DRAWING:
                    m_alignMin = m_container->getMin();
                    m_alignMax = m_container->getMax();
                    setStatus(SelectEntity);
                    break;
                default:
                    break;
            }
            break;
        }
        case SelectEntity:{
            RS_Entity* entity = catchEntityByEvent(e);
            if (entity != nullptr){
                m_entityToAlign = entity;
                m_finishActionAfterTrigger = e->isControl;
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyAlignSingle::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    switch (status){
        case SetRefPoint:{
            setStatus(-1);
            break;
        }
        case SelectEntity:{
            m_entityToAlign = nullptr;
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
        m_alignMin = pos;
        m_alignMax = pos;
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
            m_alignMin = m_container->getMin();
            m_alignMax = m_container->getMax();
            setStatus(SelectEntity);
        }
        else {
            setStatus(SetRefPoint);
        }
        updateMouseButtonHints();
    }
}
