/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_actiondimstyleapply.h"

#include "rs_dimension.h"
#include "rs_entity.h"

LC_ActionDimStyleApply::LC_ActionDimStyleApply(LC_ActionContext *actionContext):
    LC_UndoableDocumentModificationAction("DimStyleApply", actionContext, RS2::ActionDimStyleApply){
}

void LC_ActionDimStyleApply::init(const int status){
    RS_PreviewActionInterface::init(status);
    if (status < 0){
        m_srcEntity = nullptr;
    }
}

void LC_ActionDimStyleApply::doInitWithContextEntity(RS_Entity* contextEntity,[[maybe_unused]] const RS_Vector& clickPos) {
    setSourceEntity(contextEntity);
}

bool LC_ActionDimStyleApply::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const auto clone = m_entityToApply->clone();
    const auto clonedDimension = static_cast<RS_Dimension*>(clone);

    const RS2::EntityType dimensionType = m_entityToApply->rtti();

    if (m_srcEntityStyleType != dimensionType) {
        // types of entities are different, potentially there might be type-specific styles
        const auto dimStylesList = m_graphic->getDimStyleList();
        // try to find style with the same base name as for source entity, but with type of target one
        const auto exactDimStyle = dimStylesList->findByBaseNameAndType(m_srcEntityBaseStyleName, dimensionType);
        if (exactDimStyle != nullptr) {
            // here we use style name for the exact style for target dimension type
            clonedDimension->setStyle(exactDimStyle->getName());
        }
        else {
            // there is no type-specific style, use base name of style from source entity
            clonedDimension->setStyle(m_srcEntityBaseStyleName);
        }
    }
    else { // source dimension and target dimension have the same type, so we can just assign style name
        const QString originalStyle = m_srcEntity->getStyle();
        clonedDimension->setStyle(originalStyle);
    }

    if (m_applyStyleOverride) {
        const LC_DimStyle* overrideStyle = m_srcEntity->getDimStyleOverride();
        clonedDimension->setDimStyleOverride(overrideStyle);
    }

    ctx.replace(m_entityToApply,clonedDimension);
    clonedDimension->update();

    return true;
}

void LC_ActionDimStyleApply::doTriggerCompletion([[maybe_unused]]bool success) {
    m_entityToApply = nullptr;
    m_applyStyleOverride = false;
}

void LC_ActionDimStyleApply::onMouseMoveEvent(const int status, const LC_MouseEvent* event) {
    switch (status){
        case SelectEntity:
        case ApplyToEntity:{
            const RS_Entity* en = catchAndDescribe(event, RS2::ResolveNone);
            if (en != nullptr && en != m_srcEntity && RS2::isDimensionalEntity(en->rtti())){ // exclude entity we use as source, if any
                highlightHover(en);
            }
            break;
        }
        default:
            break;
    }
}

/**
 * this one is called if back by escape is involved, so perform cleanup there too
 */
void LC_ActionDimStyleApply::finish(){
    RS_PreviewActionInterface::finish();
    m_srcEntity = nullptr;
}

void LC_ActionDimStyleApply::setSourceEntity(RS_Entity* en) {
    const RS2::EntityType dimensionType = en->rtti();
    const bool dimensionalEntity = RS2::isDimensionalEntity(dimensionType);
    // selection of entity that will be used as source for pen
    if (dimensionalEntity) {
        m_srcEntity = static_cast<RS_Dimension*>(en);
        const QString styleName = m_srcEntity->getStyle();
        LC_DimStyle::parseStyleName(styleName, m_srcEntityBaseStyleName, m_srcEntityStyleType);
        setStatus(ApplyToEntity);
    }
}

void LC_ActionDimStyleApply::onMouseLeftButtonRelease([[maybe_unused]]int status, const LC_MouseEvent* e) {
    RS_Entity* en = catchEntityByEvent(e, RS2::ResolveNone);
    if (en != nullptr) {
        switch (getStatus()) {
            case SelectEntity: {
                setSourceEntity(en);
                break;
            }
            case ApplyToEntity: {
                const RS2::EntityType dimensionType = en->rtti();
                const bool dimensionalEntity = RS2::isDimensionalEntity(dimensionType);
                if (!en->isLocked() && en != m_srcEntity && dimensionalEntity) {
                    m_entityToApply = static_cast<RS_Dimension*>(en);
                    m_applyStyleOverride = !e->isShift;
                    trigger();
                }
                break;
            }
            default:
                break;
        }
    }
}

void LC_ActionDimStyleApply::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    switch (status){
        case SelectEntity:{
            init(-1);
            break;
        }
        case ApplyToEntity:{
            setStatus(SelectEntity);
            m_srcEntity = nullptr;
            break;
        }
        default:
            break;
    }
    redraw();
}

void LC_ActionDimStyleApply::updateActionPrompt(){
    switch (getStatus()) {
        case SelectEntity:
            updatePromptTRCancel(tr("Specify dimension to pick the style"));
            break;
        case ApplyToEntity:
            updatePromptTRCancel(tr("Specify dimension to apply style"), MOD_SHIFT_LC(tr("Do not apply style override")));
            break;
        default:
            RS_ActionInterface::updateActionPrompt();
            break;
    }
}
RS2::CursorType LC_ActionDimStyleApply::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
