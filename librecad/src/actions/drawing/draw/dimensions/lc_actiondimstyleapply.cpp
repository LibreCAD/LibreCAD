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

#include "lc_graphicviewport.h"
#include "qg_pentoolbar.h"
#include "rs_dimension.h"
#include "rs_entity.h"
#include "rs_modification.h"
#include "rs_pen.h"

LC_ActionDimStyleApply::LC_ActionDimStyleApply(LC_ActionContext *actionContext):
    RS_PreviewActionInterface("DimStyleApply", actionContext, RS2::ActionDimStyleApply){
}

void LC_ActionDimStyleApply::init(int status){
    RS_PreviewActionInterface::init(status);
    if (status < 0){
        m_srcEntity = nullptr;
    }
}

void LC_ActionDimStyleApply::doInitWithContextEntity(RS_Entity* contextEntity,[[maybe_unused]] const RS_Vector& clickPos) {
    setSourceEntity(contextEntity);
}

void LC_ActionDimStyleApply::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    switch (status){
        case SelectEntity:
        case ApplyToEntity:{
            RS_Entity* en = catchAndDescribe(e, RS2::ResolveNone);
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
 * @param updateTB
 */
void LC_ActionDimStyleApply::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
    m_srcEntity = nullptr;
}

void LC_ActionDimStyleApply::setSourceEntity(RS_Entity* en) {
    RS2::EntityType dimensionType = en->rtti();
    bool dimensionalEntity = RS2::isDimensionalEntity(dimensionType);
    // selection of entity that will be used as source for pen
    if (dimensionalEntity) {
        m_srcEntity = static_cast<RS_Dimension*>(en);
        QString styleName = m_srcEntity->getStyle();
        LC_DimStyle::parseStyleName(styleName, m_srcEntityBaseStyleName, m_srcEntityStyleType);
        setStatus(ApplyToEntity);
    }
}

void LC_ActionDimStyleApply::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Entity* en = catchEntityByEvent(e, RS2::ResolveNone);
    if(en != nullptr){
        switch (getStatus()){
            case SelectEntity:{
                setSourceEntity(en);
                break;
            }
            case ApplyToEntity:{
                RS2::EntityType dimensionType = en->rtti();
                bool dimensionalEntity = RS2::isDimensionalEntity(dimensionType);
                if (!en->isLocked() && en != m_srcEntity && dimensionalEntity){

                    auto clone = en->clone();
                    auto clonedDimension = static_cast<RS_Dimension*>(clone);

                    QString originalStyle = m_srcEntity->getStyle();
                    LC_DimStyle* overrideStyle {nullptr};

                    if (m_srcEntityStyleType != dimensionType) {
                        // types of entities are different, potentially there might be type-specific styles
                        auto dimStylesList = m_graphic->getDimStyleList();
                        // try to find style with the same base name as for source entity, but with type of target one
                        auto exactDimStyle = dimStylesList->findByBaseNameAndType(m_srcEntityBaseStyleName, dimensionType);
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
                       clonedDimension->setStyle(originalStyle);
                    }

                    if (!e->isShift) {
                        overrideStyle = m_srcEntity->getDimStyleOverride();
                        clonedDimension->setDimStyleOverride(overrideStyle);
                    }

                    m_container->addEntity(clonedDimension);
                    undoCycleReplace(en, clonedDimension);
                    clonedDimension->update();
                    redraw();
                }
                break;
            }
            default:
                break;
        }
    }
}

void LC_ActionDimStyleApply::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
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

void LC_ActionDimStyleApply::updateMouseButtonHints(){
    switch (getStatus()) {
        case (SelectEntity):
            updateMouseWidgetTRCancel(tr("Specify dimension to pick the style"));
            break;
        case ApplyToEntity:
            updateMouseWidgetTRCancel(tr("Specify dimension to apply style"), MOD_SHIFT_LC(tr("Do not apply style override")));
            break;
        default:
            RS_ActionInterface::updateMouseButtonHints();
    }
}
RS2::CursorType LC_ActionDimStyleApply::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
