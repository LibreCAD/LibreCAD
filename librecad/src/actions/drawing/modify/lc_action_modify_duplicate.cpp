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

#include "lc_action_modify_duplicate.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_duplicate_options_filler.h"
#include "lc_duplicate_options_widget.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_entity.h"

// fixme - sand - cmd - add support of commands for entering offset(?) and setting direction (for interactive mode)!!
LC_ActionModifyDuplicate::LC_ActionModifyDuplicate(LC_ActionContext *actionContext):
    LC_AbstractActionWithPreview("ActionModifyDuplicate", actionContext, RS2::ActionModifyDuplicate){
}

LC_ActionModifyDuplicate::~LC_ActionModifyDuplicate() = default;

void LC_ActionModifyDuplicate::doSaveOptions() {
    save("OffsetX", m_offsetX);
    save("OffsetY", m_offsetY);
    save("InPlace", m_duplicateInplace);
    save("PenMode", m_penMode);
    save("LayerMode", m_layerMode);
}

void LC_ActionModifyDuplicate::doLoadOptions() {
    m_offsetX = loadDouble("OffsetX", 10.0);
    m_offsetY = loadDouble("OffsetY", 10.0);
    m_duplicateInplace = loadBool("InPlace", false);
    m_penMode = loadInt("PenMode", PenApplyMode::PEN_ACTIVE);
    m_layerMode = loadInt("LayerMode", LayerApplyMode::LAYER_ACTIVE);
}

bool LC_ActionModifyDuplicate::isInVisualSnapStatus(int status) {
    return (status == SetOffsetDirection);
}

// support of duplicating already selected entities on action invocation
bool LC_ActionModifyDuplicate::doCheckMayTriggerOnInit(const int status){
    return status == SelectEntity;
}

bool LC_ActionModifyDuplicate::isAcceptSelectedEntityToTriggerOnInit([[maybe_unused]]RS_Entity *pEntity){
    return true;
}

void LC_ActionModifyDuplicate::doInitWithContextEntity(RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
    m_selectedEntity = contextEntity;
    const RS_Vector center = getEntityCenterPoint(m_selectedEntity);
    addSnappedPointToVisualSnap(center);
    moveRelativeZero(center);
    setStatus(SetOffsetDirection);
}

// trigger support
bool LC_ActionModifyDuplicate::doCheckMayTrigger(){
    return m_selectedEntity != nullptr;
}

bool LC_ActionModifyDuplicate::isSetActivePenAndLayerOnTrigger(){
    return m_duplicateInplace; // if inplace, use settings from active layer and pen
}

bool LC_ActionModifyDuplicate::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx){
    doCreateEntitiesOnTrigger(m_selectedEntity, ctx.entitiesToAdd);
    return true;
}

void LC_ActionModifyDuplicate::doCreateEntitiesOnTrigger(RS_Entity *en, QList<RS_Entity *> &list){
    // create clone
    RS_Entity* clone = en->clone();
    if (clone != nullptr) {
        clone->setHighlighted(false);

        // move clone if needed to offset
        const RS_Vector offset = determineOffset(m_triggerPoint, getEntityCenterPoint(en));
        if (offset.valid){
            clone->move(offset);
        }

        // apply proper layer and pen attributes if we are not creating duplicate in place
        if (!m_duplicateInplace){
            applyPenAndLayerBySourceEntity(en, clone, m_penMode, m_layerMode);
        }
        // add duplicate to list of trigger entities
        list<<clone;
    }
}
namespace {
// todo - remove later
//  offset transformation vectors for directions of offset - for each 45 degrees.
    const std::vector<RS_Vector> OFFSET_DIRECTION_VECTORS{
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
RS_Vector LC_ActionModifyDuplicate::determineOffset(const RS_Vector& snapOfOffset, const RS_Vector& center) const{
    RS_Vector wcsOffset(false);
    if (!m_duplicateInplace){
        const bool moveX = LC_LineMath::isMeaningful(m_offsetX);
        const bool moveY = LC_LineMath::isMeaningful(m_offsetY);

        if (moveX || moveY){
            // create offset vector
            const double ucsMoveX = LC_LineMath::getMeaningful(m_offsetX);
            const double ucsMoveY = LC_LineMath::getMeaningful(m_offsetY);
            const auto ucsOffset = RS_Vector(ucsMoveX, ucsMoveY);
            wcsOffset = toWorldDelta(ucsOffset);
        }
        if (snapOfOffset.valid){
            const double wcsAngle = center.angleTo(snapOfOffset);
            const double correctedAngle = RS_Math::correctAngle(wcsAngle);
            const auto wcsOffsetDirection = RS_Vector(correctedAngle);
            // prepare vector we'll use for moving shape
            const auto resultingOffset = wcsOffsetDirection * wcsOffset;
            wcsOffset = resultingOffset;
        }
    }
    return wcsOffset;
}

void LC_ActionModifyDuplicate::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    m_triggerPoint = RS_Vector{false};
    const int status = getStatus();
    if (status == SelectEntity){
        m_selectedEntity = nullptr;
    }
    else if (status == SetOffsetDirection){
        // stay in the same status
//        setStatus(SelectEntity);
    }
    else{
        finishAction();
    }
}

void LC_ActionModifyDuplicate::doOnLeftMouseButtonRelease([[maybe_unused]] const LC_MouseEvent* e, const int status, [[maybe_unused]]const RS_Vector &snapPoint){
    switch (status) {
        case SelectEntity: {
            RS_Entity *en = catchEntityByEvent(e);
            if (en != nullptr){
                // just call trigger for duplicate creation
                m_selectedEntity = en;
                if (m_alternativeActionMode && !m_duplicateInplace){
                    const RS_Vector center = getEntityCenterPoint(m_selectedEntity);
                    addSnappedPointToVisualSnap(center);
                    moveRelativeZero(center);
                    setStatus(SetOffsetDirection);
                } else {
                    trigger();
                }
            }
            break;
        }
        case SetOffsetDirection:
            m_triggerPoint = snapPoint;
            addSnappedPointToVisualSnap(snapPoint);
            trigger();
            break;
        default:
            break;
    }
}

bool LC_ActionModifyDuplicate::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* event, const int status){
    return status ==  SelectEntity || status == SetOffsetDirection;
}

/**
 * create preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionModifyDuplicate::doPreparePreviewEntities(const LC_MouseEvent* e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]] const int status){
    switch (status){
        case SelectEntity:{
            const auto en = catchEntityByEvent(e);
            if (en != nullptr){
                // highlight original
                highlightHover(en);

                // handle offset - if it is present, create a clone of snapped entity and display it for preview
                const auto snapForOffset = RS_Vector(false);
                const auto offset = determineOffset(snapForOffset, getEntityCenterPoint(en));
                if (offset.valid){
                    const auto clone = en->clone();
                    clone->move(offset);
                    list << clone;
                    if (isInfoCursorForModificationEnabled()){
                        msg(tr("Duplicate Offset"))
                            .relative(offset)
                            .relativePolar(offset)
                            .toInfoCursorZone2(false);
                    }
                }
            }
            break;
        }
        case SetOffsetDirection:{
            if (m_selectedEntity != nullptr){
                highlightSelected(m_selectedEntity);
                auto snapOffset = RS_Vector(false);
//                if (alternativeActionMode){
                    snapOffset = snap;
//                }
                const RS_Vector &center = getEntityCenterPoint(m_selectedEntity);
                if (m_showRefEntitiesOnPreview) {
                    previewRefLine(center, snap);
                    previewRefPoint(center);
                }
                const RS_Vector offset = determineOffset(snapOffset, center);
                if (offset.valid){
                    const auto clone = m_selectedEntity->clone();
                    clone->move(offset);
                    list << clone;
                    if (m_showRefEntitiesOnPreview) {
                        const RS_Vector newCenter = getEntityCenterPoint(clone);
                        previewRefSelectablePoint(newCenter);
                        auto data = RS_EllipseData();
                        data.center = center;
                        data.majorP = toWorldDelta(RS_Vector(std::abs(m_offsetX), 0, 0));
                        data.ratio = std::abs(m_offsetY / m_offsetX);
                        previewRefEllipse(data);
                    }
                    if (isInfoCursorForModificationEnabled()){
                        msg(tr("Duplicate Offset"))
                          .relative(offset)
                          .relativePolar(offset)
                          .toInfoCursorZone2(false);
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

void LC_ActionModifyDuplicate::updateActionPrompt(){
    switch (getStatus()){
        case SelectEntity:
            updatePromptTRCancel(tr("Select entity to duplicate"), m_duplicateInplace ? MOD_NONE :  MOD_SHIFT_LC(tr("Interactive Offset")));
            break;
        case SetOffsetDirection:
            updatePromptTRCancel(tr("Select direction of offset"),MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            LC_AbstractActionWithPreview::updateActionPrompt();
            break;
    }
}

bool LC_ActionModifyDuplicate::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "offsetX") {
        setOffsetX(distance);
        return true;
    }
    if (tag == "offsetY") {
        setOffsetY(distance);
        return true;
    }
    return false;
}

LC_ActionOptionsWidget* LC_ActionModifyDuplicate::createOptionsWidget(){
    return new LC_DuplicateOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyDuplicate::createOptionsFiller() {
    return new LC_DuplicateOptionsFiller();
}

RS2::CursorType LC_ActionModifyDuplicate::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

RS_Vector LC_ActionModifyDuplicate::doGetMouseSnapPoint(const LC_MouseEvent* e){
    RS_Vector snapped = e->snapPoint;
    if (getStatus() == SetOffsetDirection){
        snapped = getSnapAngleAwarePoint(e, getEntityCenterPoint(m_selectedEntity), snapped, isMouseMove(e));
    }
    return snapped;
}
