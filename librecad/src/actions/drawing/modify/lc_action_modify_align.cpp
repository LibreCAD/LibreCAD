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
#include "lc_action_modify_align.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_align_options_widget.h"
#include "lc_align_options_filler.h"
#include "rs_document.h"
#include "rs_entity.h"

LC_ActionModifyAlign::LC_ActionModifyAlign(LC_ActionContext *actionContext)
    :LC_ActionPreSelectionAwareBase("ActionModifyAlign", actionContext, RS2::ActionModifyAlign) {
}

void LC_ActionModifyAlign::doSaveOptions() {
    save("HAlign", m_hAlign);
    save("VAlign", m_vAlign);
    save("AlignTo", m_alignType);
    save("AsGroup", m_asGroup);
}

void LC_ActionModifyAlign::doLoadOptions() {
    m_hAlign = loadInt("HAlign", 0);
    m_vAlign = loadInt("VAlign", 0);
    m_alignType = loadInt("AlignTo", 0);
    m_asGroup = loadBool("AsGroup", false);
}

void LC_ActionModifyAlign::init(const int status) {
    if (m_viewport->hasUCS()){
        commandMessage(tr("Align action at the moment supports only World Coordinates system, and may not be invoked if User Coordinate System is active."));
        finish();
    }
    else {
        showOptions();
        LC_ActionPreSelectionAwareBase::init(status);
    }
}

bool LC_ActionModifyAlign::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    createAlignedEntities(ctx.entitiesToAdd, m_alignMin, m_alignMax, false);
    ctx.dontSetActiveLayerAndPen();
    ctx -= m_selectedEntities;
    return true;
}

void LC_ActionModifyAlign::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyAlign::onSelectionCompleted([[maybe_unused]]bool singleEntity, const bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateActionPrompt();
}

void LC_ActionModifyAlign::doTriggerCompletion([[maybe_unused]]bool success) {
    m_selectionComplete = false;
    m_selectedEntities.clear();
}

bool LC_ActionModifyAlign::isInVisualSnapStatus([[maybe_unused]]int status) {
    return m_alignType == LC_Align::POSITION;
}


void LC_ActionModifyAlign::onMouseMoveEventSelected([[maybe_unused]]int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;

    RS_Vector min;
    RS_Vector max;
    bool showPreview = true;

    // defining boundaries
    switch (m_alignType) {
        case LC_Align::ENTITY: {
            const RS2::ResolveLevel resolveLevel = e->isControl ? RS2::ResolveAll : RS2::ResolveNone;
            const RS_Entity *entity = catchEntity(snap, resolveLevel);
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
            min = m_document->getMin();
            max = m_document->getMax();
            break;
        }
        default:
            break;
    }

    // preview entities
    if (showPreview) {
        // reference points
        double horizontalRef;
        double verticalRef;
        const bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(min, max, m_vAlign, horizontalRef);
        const bool drawVertical  = LC_Align::getVerticalRefCoordinate(min, max, m_hAlign, verticalRef);

        QList<RS_Entity *> alignedEntitiesList;
        const RS_Vector groupOffset = createAlignedEntities(alignedEntitiesList, min, max, true);

        for (const auto ent: alignedEntitiesList) {
            previewEntity(ent);
        }

        if (m_showRefEntitiesOnPreview) {
            previewRefLines(drawVertical, verticalRef, drawHorizontal, horizontalRef);
        }

        // info cursor
        if (m_infoCursorOverlayPrefs->enabled){
            QString message = tr("Align to ");
            switch (m_alignType) {
                case LC_Align::ENTITY:{
                    message.append(tr("Entity"));
                    break;
                }
                case LC_Align::POSITION:{
                    message.append(tr("Position"));
                    break;
                }
                case LC_Align::DRAWING:{
                    message.append(tr("Drawing"));
                    break;
                }
                default:
                    break;
            }

            QString ref = tr("Reference: ");
            if (drawVertical){
                ref.append("X: ");
                ref.append(formatLinear(verticalRef));
                ref.append(" ");
            }

            if (drawHorizontal){
                ref.append("Y: ");
                ref.append(formatLinear(horizontalRef));
            }

            auto builder = msgStart()
                .string(message)
                .string(ref);
            if (groupOffset.valid) {
                builder.string(tr("Offset:"))
                       .relative(groupOffset)
                       .relativePolar(groupOffset);
            }
            builder.toInfoCursorZone2(false);
        }
    }
}

void LC_ActionModifyAlign::previewRefLines(const bool drawVertical, [[maybe_unused]]double verticalRef, const bool drawHorizontal, [[maybe_unused]]double horizontalRef) const {
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


void LC_ActionModifyAlign::onMouseLeftButtonReleaseSelected([[maybe_unused]]int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    bool mayTrigger = true;
    switch (m_alignType) {
        case LC_Align::ENTITY: {
            const RS2::ResolveLevel resolveLevel = e->isControl ? RS2::ResolveAll : RS2::ResolveNone;
            const RS_Entity *entity  = catchEntity(snap, resolveLevel);
            if (entity != nullptr) {
                m_alignMin = entity->getMin();
                m_alignMax = entity->getMax();
            } else {
                mayTrigger = false;
            }
            break;
        }
        case LC_Align::POSITION: {
            snap = getRelZeroAwarePoint(e, snap);
            m_alignMin = snap;
            m_alignMax = snap;
            break;
        }
        case LC_Align::DRAWING: {
            m_alignMin = m_document->getMin();
            m_alignMax = m_document->getMax();
            break;
        }
        default:
            break;
    }
    if (mayTrigger) {
        trigger();
    }
}

void LC_ActionModifyAlign::onCoordinateEvent([[maybe_unused]]int status, const bool isZero, const RS_Vector &pos) {
    if (m_alignType == LC_Align::POSITION && !isZero) {
        m_alignMin = pos;
        m_alignMax = pos;
        trigger();
    }
    else{
        commandMessage(tr("Coordinate is accepted only for Align to \"Position\""));
    }
}

void LC_ActionModifyAlign::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    deletePreview();
    if (m_selectionComplete) {
        m_selectionComplete = false;
    } else {
        initPrevious(status);
    }
}

void LC_ActionModifyAlign::updateActionPromptForSelected([[maybe_unused]]int status) {
    switch (m_alignType) {
        case LC_Align::ENTITY:
            updatePromptTRBack(tr("Select base alignment entity"), MOD_CTRL(tr("Select child entities of containers")));
            break;
        case LC_Align::POSITION:
            updatePromptTRBack(tr("Specify base alignment point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case LC_Align::DRAWING:
            updatePromptTRBack(tr("Click or Enter to finish align"));
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionModifyAlign::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select entities to align") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Select and align")));
}

RS2::CursorType LC_ActionModifyAlign::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

bool LC_ActionModifyAlign::isAllowTriggerOnEmptySelection() {
    return false;
}

void LC_ActionModifyAlign::setAlignType(const int t) {
    m_alignType = t;
    updateActionPrompt();
}

LC_ActionOptionsWidget *LC_ActionModifyAlign::createOptionsWidget() {
    return new LC_AlignOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyAlign::createOptionsFiller() {
    return new LC_AlignOptionsFiller();
}

RS_Vector LC_ActionModifyAlign::createAlignedEntities(QList<RS_Entity *> &clonesList, const RS_Vector& min, const RS_Vector& max, const bool previewOnly) {
    auto result =  RS_Vector(false);

    const RS_Vector targetPoint = getReferencePoint(min, max);
    const bool updateAttributes = !previewOnly;

    if (m_asGroup || m_selectedEntities.size() == 1) {
        RS_Vector selectionMin;
        RS_Vector selectionMax;

        LC_Align::collectSelectionBounds(m_selectedEntities, selectionMin, selectionMax);
        const RS_Vector selectionRefPoint = getReferencePoint(selectionMin, selectionMax);
        const RS_Vector offset = targetPoint - selectionRefPoint;

        result = offset;
        result.valid = true;

        for (const auto e: m_selectedEntities) {
            RS_Entity* clone = LC_Align::createCloneMovedToOffset(e, offset, updateAttributes);
            clonesList << clone;
        }
    } else {
        for (const auto e: m_selectedEntities) {
            RS_Entity *clone = LC_Align::createCloneMovedToTarget(e, targetPoint, updateAttributes, m_hAlign, m_vAlign);
            clonesList << clone;
        }
    }
    return result;
}

RS_Vector LC_ActionModifyAlign::getReferencePoint(const RS_Vector &min, const RS_Vector &max) const {
    return LC_Align::getReferencePoint(min, max, m_hAlign, m_vAlign);
}
