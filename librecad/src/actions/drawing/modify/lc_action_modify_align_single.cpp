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
#include "lc_action_modify_align_single.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_align_single_options_filler.h"
#include "lc_align_single_options_widget.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "rs_document.h"
#include "rs_entity.h"

LC_ActionModifyAlignSingle::LC_ActionModifyAlignSingle(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionModifyAlignOne", actionContext, RS2::ActionModifyAlignOne) {
}

void LC_ActionModifyAlignSingle::doSaveOptions() {
    save("HAlign", m_hAlign);
    save("VAlign", m_vAlign);
    save("AlignTo", m_alignType);
}

void LC_ActionModifyAlignSingle::doLoadOptions() {
    m_hAlign = loadInt("HAlign", 0);
    m_vAlign = loadInt("VAlign", 0);
    m_alignType = loadInt("AlignTo", 0);
}

bool LC_ActionModifyAlignSingle::isInVisualSnapStatus(int status) {
    return (status == SetRefPoint);
}

void LC_ActionModifyAlignSingle::init(const int status) {
    if (m_viewport->hasUCS()) {
        commandMessage(tr(
            "Align action at the moment supports only World Coordinates system, and may not be invoked if User Coordinate System is active."));
        finish();
    }
    else {
        RS_PreviewActionInterface::init(status);
    }
}

void LC_ActionModifyAlignSingle::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
    m_entityToAlign = contextEntity;
}

bool LC_ActionModifyAlignSingle::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (m_entityToAlign != nullptr) {
        const RS_Vector target = LC_Align::getReferencePoint(m_alignMin, m_alignMax, m_hAlign, m_vAlign);
        RS_Entity* clone = LC_Align::createCloneMovedToTarget(m_entityToAlign, target, true, m_hAlign, m_vAlign);
        if (clone != nullptr) {
            clone->clearSelectionFlag();
            ctx.replace(m_entityToAlign, clone);
            ctx.dontSetActiveLayerAndPen();
        }
    }
    return true;
}

void LC_ActionModifyAlignSingle::doTriggerCompletion([[maybe_unused]] bool success) {
    m_entityToAlign = nullptr;
    if (m_finishActionAfterTrigger) {
        setStatus(-1);
    }
    else {
        if (m_showRefEntitiesOnPreview) {
            previewAlignRefPoint(m_alignMin, m_alignMax);
        }
    }
    drawPreview();
}

void LC_ActionModifyAlignSingle::previewAlign(const RS_Entity* entity, const double verticalRef, const bool drawVertical,
                                              const double horizontalRef, const bool drawHorizontal, const RS_Vector& alignMin,
                                              const RS_Vector& alignMax) const {
    RS_Vector offset;
    if (entity != nullptr) {
        highlightHover(entity);
        const RS_Vector target = LC_Align::getReferencePoint(alignMin, alignMax, m_hAlign, m_vAlign);

        const RS_Vector entityRefPoint = LC_Align::getReferencePoint(entity->getMin(), entity->getMax(), m_hAlign, m_vAlign);
        offset = target - entityRefPoint;

        const RS_Entity* clone = LC_Align::createCloneMovedToOffset(entity, offset, false);
        if (clone != nullptr) {
            previewEntity(clone);
        }
    }

    if (isInfoCursorForModificationEnabled()) {
        const QString message = prepareInfoCursorMessage(verticalRef, drawVertical, horizontalRef, drawHorizontal);

        auto builder = msgStart().string(message);
        if (entity != nullptr) {
            builder.string(tr("Offset:")).relative(offset).relativePolar(offset);
        }
        builder.toInfoCursorZone2(false);
    }
}

void LC_ActionModifyAlignSingle::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetRefPoint: {
            RS_Vector min;
            RS_Vector max;
            bool showPreview = true;
            switch (m_alignType) {
                case LC_Align::ENTITY: {
                    const RS_Entity* entity = catchAndDescribe(snap);
                    if (entity != nullptr) {
                        min = entity->getMin();
                        max = entity->getMax();
                        highlightHover(entity);
                    }
                    else {
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
                    min = m_document->getMin();
                    max = m_document->getMax();
                    break;
                }
                default:
                    break;
            }
            if (showPreview) {
                double verticalRef;
                const bool drawVertical = LC_Align::getVerticalRefCoordinate(min, max, m_hAlign, verticalRef);

                double horizontalRef;
                const bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(min, max, m_vAlign, horizontalRef);

                previewRefLines(drawVertical, verticalRef, drawHorizontal, horizontalRef);

                // info cursor
                if (m_infoCursorOverlayPrefs->enabled) {
                    const QString msg = prepareInfoCursorMessage(verticalRef, drawVertical, horizontalRef, drawHorizontal);
                    appendInfoCursorZoneMessage(msg, 2, false);
                }

                if (m_entityToAlign != nullptr) {
                    previewAlign(m_entityToAlign, verticalRef, drawVertical, horizontalRef, drawHorizontal, min, max);
                }
            }
            break;
        }
        case SelectEntity: {
            double verticalRef;
            const bool drawVertical = LC_Align::getVerticalRefCoordinate(m_alignMin, m_alignMax, m_hAlign, verticalRef);

            double horizontalRef;
            const bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(m_alignMin, m_alignMax, m_vAlign, horizontalRef);

            if (m_showRefEntitiesOnPreview) {
                previewRefLines(drawVertical, verticalRef, drawHorizontal, horizontalRef);
            }
            if (m_baseAlignEntity != nullptr) {
                highlightSelected(m_baseAlignEntity);
            }

            const RS_Entity* entity = catchAndDescribe(e);
            previewAlign(entity, verticalRef, drawVertical, horizontalRef, drawHorizontal, m_alignMin, m_alignMax);
            break;
        }
        default:
            break;
    }
}

QString LC_ActionModifyAlignSingle::prepareInfoCursorMessage(const double verticalRef, const bool drawVertical, const double horizontalRef,
                                                             const bool drawHorizontal) const {
    QString msg = tr("Align to ");
    switch (m_alignType) {
        case LC_Align::ENTITY: {
            msg.append(tr("Entity"));
            break;
        }
        case LC_Align::POSITION: {
            msg.append(tr("Position"));
            break;
        }
        case LC_Align::DRAWING: {
            msg.append(tr("Drawing"));
            break;
        }
        default:
            break;
    }
    msg.append("\n");
    msg.append(tr("Reference: "));
    if (drawVertical) {
        msg.append("X: ");
        msg.append(formatLinear(verticalRef));
        msg.append(" ");
    }

    if (drawHorizontal) {
        msg.append("Y: ");
        msg.append(formatLinear(horizontalRef));
    }
    return msg;
}

void LC_ActionModifyAlignSingle::previewRefLines(const bool drawVertical, double verticalRef, const bool drawHorizontal,
                                                 double horizontalRef) const {
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

void LC_ActionModifyAlignSingle::previewAlignRefPoint(const RS_Vector& min, const RS_Vector& max) const {
    double verticalRef;
    const bool drawVertical = LC_Align::getVerticalRefCoordinate(min, max, m_hAlign, verticalRef);
    RS_Vector wcsLeftBottom = m_viewport->getUCSViewLeftBottom();
    RS_Vector wcsRightTop = m_viewport->getUCSViewRightTop();
    if (drawVertical) {
        // NOTE:: works properly for WCS only
        previewRefConstructionLine({verticalRef, wcsLeftBottom.y}, {verticalRef, wcsRightTop.y});
    }
    double horizontalRef;
    const bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(min, max, m_vAlign, horizontalRef);
    if (drawHorizontal) {
        // NOTE:: works properly for WCS only
        previewRefConstructionLine({wcsLeftBottom.x, horizontalRef}, {wcsRightTop.x, horizontalRef});
    }
}

RS2::CursorType LC_ActionModifyAlignSingle::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::CadCursor;
}

void LC_ActionModifyAlignSingle::updateActionPrompt() {
    switch (getStatus()) {
        case SetRefPoint: {
            switch (m_alignType) {
                case LC_Align::ENTITY:
                    updatePromptTRCancel(tr("Select base alignment entity"));
                    break;
                case LC_Align::POSITION:
                    updatePromptTRCancel(tr("Specify base alignment point"), MOD_SHIFT_RELATIVE_ZERO);
                    break;
                case LC_Align::DRAWING:
                    //                    updateMouseWidgetTRCancel(tr("Click or Enter to finish align"));
                    updatePrompt();
                    break;
                default:
                    updatePrompt();
                    break;
            }
            break;
        }
        case SelectEntity: {
            updatePromptTRBack(tr("Select entity to align"), MOD_CTRL(tr("Finish alignment after selection")));
            break;
        }
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionModifyAlignSingle::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetRefPoint: {
            switch (m_alignType) {
                case LC_Align::ENTITY: {
                    RS_Entity* entity = catchEntityByEvent(e);
                    if (entity != nullptr) {
                        m_baseAlignEntity = entity;
                        m_alignMin = entity->getMin();
                        m_alignMax = entity->getMax();
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
                    m_alignMin = m_document->getMin();
                    m_alignMax = m_document->getMax();
                    setStatus(SelectEntity);
                    break;
                default:
                    break;
            }

            if (m_entityToAlign != nullptr) {
                trigger();
            }
            else {
                setStatus(SelectEntity);
            }
            break;
        }
        case SelectEntity: {
            RS_Entity* entity = catchEntityByEvent(e);
            if (entity != nullptr) {
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

void LC_ActionModifyAlignSingle::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    switch (status) {
        case SetRefPoint: {
            setStatus(-1);
            break;
        }
        case SelectEntity: {
            m_entityToAlign = nullptr;
            if (m_alignType == LC_Align::AlignMode::DRAWING) {
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

void LC_ActionModifyAlignSingle::onCoordinateEvent(const int status, const bool isZero, const RS_Vector& pos) {
    if (status == SetRefPoint && !isZero && m_alignType == LC_Align::AlignMode::POSITION) {
        m_alignMin = pos;
        m_alignMax = pos;
        setStatus(SelectEntity);
    }
    else {
        commandMessage(tr("Coordinate is accepted only for Align to \"Position\""));
    }
}

LC_ActionOptionsWidget* LC_ActionModifyAlignSingle::createOptionsWidget() {
    return new LC_AlignSingleOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyAlignSingle::createOptionsFiller() {
    return new LC_AlignSingleOptionsFiller();
}

void LC_ActionModifyAlignSingle::setAlignType(const int a) {
    if (a != m_alignType) {
        LC_ActionModifyAlignData::setAlignType(a);
        if (a == LC_Align::AlignMode::DRAWING) {
            m_alignMin = m_document->getMin();
            m_alignMax = m_document->getMax();
            setStatus(SelectEntity);
        }
        else {
            setStatus(SetRefPoint);
        }
        updateActionPrompt();
    }
}
