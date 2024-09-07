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
#include "rs_math.h"
#include "lc_linemath.h"
#include "lc_duplicateoptions.h"
#include "lc_actionmodifyduplicate.h"
#include "lc_abstractactionwithpreview.h"
#include "rs_previewactioninterface.h"

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
        RS_Vector offset = determineOffset(triggerPoint, getEntityCenterPoint(en));
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
namespace {
// todo - remove later
//  offset transformation verctors for directions of offset - for each 45 degress. 
    const std::vector<RS_Vector> offsetDirectionVectors{
        RS_Vector(1, 0), // 0
        RS_Vector(1, 1), // 45
        RS_Vector(0, 1), // 90
        RS_Vector(-1, 1), // 135    
        RS_Vector(-1, 0), // 180
        RS_Vector(-1, -1), // 225
        RS_Vector(0, -1), // 270
        RS_Vector(1, -1), // 315    
    };
}

/**
 * Calculate vector that will be used for moving entity duplicate
 * @return
 */
RS_Vector LC_ActionModifyDuplicate::determineOffset(RS_Vector& snapOfOffset, const RS_Vector& center) const{
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
        if (snapOfOffset.valid){
            double angle = center.angleTo(snapOfOffset);
            double correctedAngle = RS_Math::correctAngle(angle);
            /**
            int mode = correctedAngle / M_PI_4;
            
//            if (mode >= 0 && mode << 8){
             */
//                RS_Vector offsetDirection = offsetDirectionVectors.at(mode);
                RS_Vector offsetDirection = RS_Vector(std::cos(correctedAngle), std::sin(correctedAngle), 0);
                // prepare vector we'll use for moving shape
                RS_Vector resultingOffset = offsetDirection * offset;
                offset = resultingOffset;
//            }
        }
    }
    return offset;
}

void LC_ActionModifyDuplicate::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    triggerPoint = RS_Vector{false};
    int status = getStatus();
    if (status == SelectEntity){
        selectedEntity = nullptr;
    }
    else if (status == SetOffsetDirection){
        // stay in the same status
//        setStatus(SelectEntity);
    }
    else{
        finishAction();
    }
}

void LC_ActionModifyDuplicate::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status, [[maybe_unused]]const RS_Vector &snapPoint){
    switch (status) {
        case SelectEntity: {
            RS_Entity *en = catchEntity(e, RS2::ResolveNone);
            if (en != nullptr){
                // just call trigger for duplicate creation
                selectedEntity = en;
                if (alternativeActionMode && !duplicateInplace){
                    RS_Vector center = getEntityCenterPoint(selectedEntity);
                    moveRelativeZero(center);
                    setStatus(SetOffsetDirection);
                } else {
                    trigger();
                }
            }
            break;
        }
        case SetOffsetDirection:
//            if (alternativeActionMode){
                triggerPoint = snapPoint;
//            }
            trigger();
            break;
        default:
            break;
    }
}

bool LC_ActionModifyDuplicate::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status){
    return status ==  SelectEntity || SetOffsetDirection;
}

/**
 * create preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionModifyDuplicate::doPreparePreviewEntities(QMouseEvent *e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    switch (status){
        case SelectEntity:{
            RS_Entity *en = catchEntity(e, RS2::ResolveNone);
            if (en != nullptr){
                // highlight original
                highlightHover(en);

                // handle offset - if it is present, create a clone of snapped entity and display it for preview
                auto snapForOffset = RS_Vector(false);
                RS_Vector offset = determineOffset(snapForOffset, getEntityCenterPoint(en));
                if (offset.valid){
                    RS_Entity *clone = en->clone();
                    clone->move(offset);
                    list << clone;
                }
            }
            break;
        }
        case SetOffsetDirection:{
            if (selectedEntity != nullptr){
                highlightSelected(selectedEntity);
                auto snapOffset = RS_Vector(false);
//                if (alternativeActionMode){
                    snapOffset = snap;
//                }
                const RS_Vector &center = getEntityCenterPoint(selectedEntity);
                if (showRefEntitiesOnPreview) {
                    previewRefLine(center, snap);
                    previewRefPoint(center);
                }
                RS_Vector offset = determineOffset(snapOffset, center);
                if (offset.valid){
                    RS_Entity *clone = selectedEntity->clone();
                    clone->move(offset);
                    list << clone;
                    if (showRefEntitiesOnPreview) {
                        const RS_Vector newCenter = getEntityCenterPoint(clone);
                        previewRefSelectablePoint(newCenter);
                        RS_EllipseData data = RS_EllipseData();
                        data.center = center;
                        data.majorP = RS_Vector(std::abs(offsetX), 0, 0);
                        data.ratio = std::abs(offsetY / offsetX);
                        previewRefEllipse(data);
                    }
                }
            }
            break;
        }
        default:
          break;
    }
}

RS_Vector LC_ActionModifyDuplicate::getEntityCenterPoint(const RS_Entity *en) const{
    RS_Vector result =  en->getCenter();
    if (!result.valid){
        result = (en->getMin() + en->getMax())/2;
    }
    return result;
}

void LC_ActionModifyDuplicate::updateMouseButtonHints(){
    switch (getStatus()){
        case SelectEntity:
            updateMouseWidgetTRCancel(tr("Select entity to duplicate"), duplicateInplace ? MOD_NONE :  MOD_SHIFT_LC(tr("Interactive Offset")));
            break;
        case SetOffsetDirection:
            updateMouseWidgetTRCancel(tr("Select direction of offset"),MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            LC_AbstractActionWithPreview::updateMouseButtonHints();
    }
}

LC_ActionOptionsWidget* LC_ActionModifyDuplicate::createOptionsWidget(){
    return new LC_DuplicateOptions();
}

RS2::CursorType LC_ActionModifyDuplicate::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

RS_Vector LC_ActionModifyDuplicate::doGetMouseSnapPoint(QMouseEvent *e){
    RS_Vector snapped = snapPoint(e);
    if (getStatus() == SetOffsetDirection){
        snapped = getSnapAngleAwarePoint(e, getEntityCenterPoint(selectedEntity), snapped, isMouseMove(e));
    }
    return snapped;
}
