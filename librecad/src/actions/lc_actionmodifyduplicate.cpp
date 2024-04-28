/****************************************************************************
**
* Action that duplicates entities

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
**********************************************************************/
#include <QList>
#include "lc_linemath.h"
#include "lc_duplicateoptions.h"
#include "lc_actionmodifyduplicate.h"
#include "lc_abstractactionwithpreview.h"

LC_ActionModifyDuplicate::LC_ActionModifyDuplicate(RS_EntityContainer &container, RS_GraphicView &graphicView):
    LC_AbstractActionWithPreview("ModifyDuplicate", container, graphicView),
    selectedEntity(nullptr),
    offsetX(0), offsetY(0){
    actionType = RS2::ActionModifyDuplicate;
}
LC_ActionModifyDuplicate::~LC_ActionModifyDuplicate() = default;

// support of duplicating already selected entities on action invocation

bool LC_ActionModifyDuplicate::doCheckMayTriggerOnInit(int status){
    return status == SelectEntity;
}

bool LC_ActionModifyDuplicate::isAcceptSelectedEntityToTriggerOnInit([[maybe_unused]]RS_Entity *pEntity){
    return true;
}

// trigger support
bool LC_ActionModifyDuplicate::doCheckMayTrigger(){
    return selectedEntity != nullptr;
}

bool LC_ActionModifyDuplicate::isSetActivePenAndLayerOnTrigger(){
    return duplicateInplace; // if inplace, use settings from active layer and pen
}

void LC_ActionModifyDuplicate::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    doCreateEntitiesOnTrigger(selectedEntity, list);
}

void LC_ActionModifyDuplicate::doCreateEntitiesOnTrigger(RS_Entity *en, QList<RS_Entity *> &list){
    // create clone
    RS_Entity* clone = en->clone();
    if (clone != nullptr){
        clone->setHighlighted(false);

        // move clone if needed to offset
        RS_Vector offset = determineOffset();
        if (offset.valid){
            clone->move(offset);
        }

        // apply proper layer and pen attributes if we are not creating duplicate in place
        if (!duplicateInplace){
            applyPenAndLayerBySourceEntity(en, clone, penMode, layerMode);
        }
        // add duplicate to list of trigger entities
        list<<clone;
    }
}

/**
 * Calculate vector that will be used for moving entity duplicate
 * @return
 */
RS_Vector LC_ActionModifyDuplicate::determineOffset() const{
    RS_Vector offset(false);
    if (!duplicateInplace){
        bool moveX = LC_LineMath::isMeaningful(offsetX);
        bool moveY = LC_LineMath::isMeaningful(offsetY);

        if (moveX || moveY){
            // create offset vector
            double mx = LC_LineMath::getMeaningful(offsetX);
            double my = LC_LineMath::getMeaningful(offsetY);
            offset = RS_Vector(mx, my, 0.0);
        }
    }
    return offset;
}

void LC_ActionModifyDuplicate::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    selectedEntity = nullptr;
}

void LC_ActionModifyDuplicate::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status, [[maybe_unused]]const RS_Vector &snapPoint){
    if (status == SelectEntity){
        RS_Entity *en = catchEntity(e, RS2::ResolveNone);
        if (en != nullptr){
            // just call trigger for duplicate creation
            selectedEntity = en;
            trigger();
        }
    }
}

bool LC_ActionModifyDuplicate::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status){
    return status ==  SelectEntity;
}

/**
 * create preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionModifyDuplicate::doPreparePreviewEntities(QMouseEvent *e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    RS_Entity *en = catchEntity(e, RS2::ResolveNone);
    if (en != nullptr){
        // highlight original
        highlightEntity(en);

        // handle offset - if it is present, create a clone of snapped entity and display it for preview
        RS_Vector offset = determineOffset();
        if (offset.valid){
            RS_Entity *clone = en->clone();
            clone->move(offset);
            list << clone;
        }
    }
}

void LC_ActionModifyDuplicate::updateMouseButtonHints(){
    switch (getStatus()){
        case SelectEntity:
            updateMouseWidgetTR("Select entity to duplicate", "Cancel");
            break;
        default:
            LC_AbstractActionWithPreview::updateMouseButtonHints();
    }
}

void LC_ActionModifyDuplicate::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_DuplicateOptions>(nullptr);
}

RS2::CursorType LC_ActionModifyDuplicate::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}



