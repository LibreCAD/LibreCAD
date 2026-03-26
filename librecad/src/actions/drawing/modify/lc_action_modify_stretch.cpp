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

#include "lc_action_modify_stretch.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_stretch_options_filler.h"
#include "lc_stretch_options_widget.h"
#include "rs_document.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "rs_settings.h"

struct LC_ActionModifyStretch::StretchActionData {
    RS_Vector firstCorner;
    RS_Vector secondCorner;
    RS_Vector referencePoint;
    RS_Vector targetPoint;
};

// fixme - add for consistency support of "Use current Attributes" and "Use Current Layer" options

// fime - FIX: trigger is not consistent with preview!!!!

LC_ActionModifyStretch::LC_ActionModifyStretch(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionModifyStretch", actionContext, RS2::ActionModifyStretch),
    m_actionData(std::make_unique<StretchActionData>()){
}

void LC_ActionModifyStretch::doSaveOptions() {
    save("RemoveOriginals", isRemoveOriginals());
}

void LC_ActionModifyStretch::doLoadOptions() {
    bool removeOriginals = loadBool("RemoveOriginals", true);
    setRemoveOriginals(removeOriginals);
}

bool LC_ActionModifyStretch::isInVisualSnapStatus(int status) {
    return (status == SetReferencePoint) || (status == SetTargetPoint) || (status == SetFirstCorner) || (status == SetSecondCorner);
}

void LC_ActionModifyStretch::init(const int status) {
    RS_PreviewActionInterface::init(status);
}

LC_ActionModifyStretch::~LC_ActionModifyStretch() = default;

bool LC_ActionModifyStretch::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (m_entitiesList.empty()) {
        for (const auto e : *m_document) { // fixme - we don't rely on selection for stretch :(
            //
            // fixme - and check that e is editable.
            if (e->isVisible() && (e->rtti() != RS2::EntityHatch) &&
                (e->isInWindow(m_actionData->firstCorner, m_actionData->secondCorner) ||
                 e ->hasEndpointsWithinWindow(m_actionData->firstCorner, m_actionData->secondCorner))) {
                m_entitiesList.append(e);
            }
        }
    }
    RS_Modification::stretch(m_actionData->firstCorner, m_actionData->secondCorner, m_actionData->targetPoint - m_actionData->referencePoint, m_entitiesList,
                             m_removeOriginals, ctx);
    if (!m_removeOriginals) {
        unselect(m_entitiesList);
    }
    else {
        const bool keepSelected =  LC_GET_ONE_BOOL("Modify", "KeepModifiedSelected", true);
        if (keepSelected) {
            select(m_entitiesList);
        }
    }
    return true;
}

void LC_ActionModifyStretch::doTriggerCompletion([[maybe_unused]]bool success) {
    if (m_removeOriginals) {
        setStatus(SetFirstCorner);
    }
    else{
        setStatus(SetTargetPoint);
    }
}

void LC_ActionModifyStretch::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    RS_Vector mouse = e->snapPoint;
    switch (status) {
        case SetFirstCorner: {
            break;
        }
        case SetSecondCorner: {
            if (m_actionData->firstCorner.valid) {
                m_actionData->secondCorner = e->snapPoint;
                previewStretchRect(false);
                if (isInfoCursorForModificationEnabled()) {
                    msg(tr("Stretch")).vector(tr("Start Corner:"), m_actionData->firstCorner).vector(
                        tr("End Corner:"), m_actionData->secondCorner).toInfoCursorZone2(false);
                }
            }
            break;
        }
        case SetReferencePoint: {
            previewStretchRect(true);
            trySnapToRelZeroCoordinateEvent(e);
            if (isInfoCursorForModificationEnabled()) {
                msg(tr("Stretch")).vector(tr("Reference Point:"), mouse).toInfoCursorZone2(false);
            }
            break;
        }
        case SetTargetPoint: {
            if (m_actionData->referencePoint.valid) {
                mouse = getSnapAngleAwarePoint(e, m_actionData->referencePoint, mouse, true);
                m_actionData->targetPoint = mouse;
                // fixme - isn't it more reliable to rely on RS_Modification::stretch there?
                m_preview->addStretchablesFrom(*m_document, m_viewport, m_actionData->firstCorner, m_actionData->secondCorner);
                const RS_Vector& offset = m_actionData->targetPoint - m_actionData->referencePoint;
                m_preview->stretch(m_actionData->firstCorner, m_actionData->secondCorner, offset);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(m_actionData->referencePoint);
                    previewRefSelectablePoint(m_actionData->targetPoint);
                    previewRefLine(m_actionData->referencePoint, m_actionData->targetPoint);
                }
                if (isInfoCursorForModificationEnabled()) {
                    msg(tr("Stretch")).vector(tr("Reference Point:"), m_actionData->referencePoint).vector(tr("Target Point:"), mouse).
                                       string(tr("Offset:")).relative(mouse).relativePolar(mouse).toInfoCursorZone2(false);
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyStretch::previewStretchRect(const bool selected) const {
    if (m_showRefEntitiesOnPreview){
        RS_Vector v0 = m_actionData->firstCorner;
        RS_Vector v1 = m_actionData->secondCorner;
        previewRefLine(v0, {v1.x, v0.y});
        previewRefLine({v1.x, v0.y}, v1);
        previewRefLine(v1, {v0.x, v1.y});
        previewRefLine({v0.x, v1.y}, v0);
        previewRefPoint(v0);
        if (selected){
            previewRefPoint(v1);
        }
        else{
            previewRefSelectablePoint(v1);
        }
    }
    else{
        m_preview->addRectangle(m_actionData->firstCorner, m_actionData->secondCorner);
    }
}

void LC_ActionModifyStretch::onMouseLeftButtonRelease([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (status == SetTargetPoint){
        const RS_Vector mouse= getSnapAngleAwarePoint(e, m_actionData->referencePoint, e->snapPoint);
        fireCoordinateEvent(mouse);
    }
    else {
        fireCoordinateEventForSnap(e);
    }
}
// fixme - default back - remove as well as from other actions and rely to parent class
void LC_ActionModifyStretch::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionModifyStretch::onCoordinateEvent(const int status,  [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetFirstCorner: {
            m_actionData->firstCorner = coord;
            addSnappedPointToVisualSnap(coord);
            setStatus(SetSecondCorner);
            break;
        }
        case SetSecondCorner: {
            m_actionData->secondCorner = coord;
            addSnappedPointToVisualSnap(coord);
            deletePreview();
            setStatus(SetReferencePoint);
            break;
        }
        case SetReferencePoint: {
            m_actionData->referencePoint = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(m_actionData->referencePoint);
            setStatus(SetTargetPoint);
            break;
        }
        case SetTargetPoint: {
            m_actionData->targetPoint = coord;
            addSnappedPointToVisualSnap(coord);
            moveRelativeZero(m_actionData->targetPoint);
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyStretch::updateActionPrompt(){
    switch (getStatus()) {
        case SetFirstCorner:
            updatePromptTRCancel(tr("Specify first corner"));
            break;
        case SetSecondCorner:
            updatePromptTRBack(tr("Specify second corner"));
            break;
        case SetReferencePoint:
            updatePromptTRBack(tr("Specify reference point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetTargetPoint:
            updatePromptTRBack(tr("Specify target point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType LC_ActionModifyStretch::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* LC_ActionModifyStretch::createOptionsWidget() {
    return new LC_StretchOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyStretch::createOptionsFiller() {
    return new LC_StretchOptionsFiller();
}
