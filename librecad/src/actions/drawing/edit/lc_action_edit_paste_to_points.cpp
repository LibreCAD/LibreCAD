/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_action_edit_paste_to_points.h"

#include "lc_copyutils.h"
#include "lc_paste_to_points_options_filler.h"
#include "lc_paste_to_points_options_widget.h"
#include "rs_clipboard.h"
#include "rs_document.h"
#include "rs_entity.h"
#include "rs_selection.h"

LC_ActionPasteToPoints::LC_ActionPasteToPoints(LC_ActionContext *actionContext):
    LC_ActionPreSelectionAwareBase("ActionPasteToPoints", actionContext, RS2::ActionPasteToPoints, {RS2::EntityPoint}){
}

void LC_ActionPasteToPoints::doSaveOptions() {
    save("Angle", m_angleRad);
    save("ScaleFactor", m_scaleFactor);
    save("RemovePoints", m_removePointAfterPaste);
}

void LC_ActionPasteToPoints::doLoadOptions() {
    m_angleRad = loadDouble("Angle", 0);
    m_scaleFactor = loadDouble("ScaleFactor", 1.0);
    m_removePointAfterPaste = loadBool("RemovePoints", true);
}

void LC_ActionPasteToPoints::init(const int status) {
    if (RS_CLIPBOARD->count() == 0){
        commandMessage(tr("Clipboard is empty"));
        finish();
    }
    else{
        showOptions();
        LC_ActionPreSelectionAwareBase::init(status);
    }
}

bool LC_ActionPasteToPoints::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    for (const auto p : std::as_const(m_selectedEntities)) {
        const RS_Vector currentPoint = p->getCenter();
        const auto pasteData = LC_CopyUtils::RS_PasteData(currentPoint, m_scaleFactor , m_angleRad);
        LC_CopyUtils::paste(pasteData, m_graphic, ctx);
        ctx.dontSetActiveLayerAndPen();
        // fixme - some progress is needed there, ++++ speed improvement for paste operation!!
//        LC_ERR << "Paste: " << currentPoint;
    }
    if (m_removePointAfterPaste){
        ctx.remove(m_selectedEntities);
    }
    return true;
}

void LC_ActionPasteToPoints::doTriggerSelectionUpdate(const bool keepSelected, [[maybe_unused]] const LC_DocumentModificationBatch& ctx) {
    if (!m_removePointAfterPaste) {
        RS_Selection::selectEntitiesList(m_document, m_viewport, m_selectedEntities, keepSelected);
    }
}

void LC_ActionPasteToPoints::doTriggerCompletion(const bool success) {
    LC_ActionPreSelectionAwareBase::doTriggerCompletion(success);
}

bool LC_ActionPasteToPoints::doUpdateAngleByInteractiveInput(const QString& tag, const double angle) {
    if (tag == "angle") {
        setAngle(angle);
        return true;
    }
    return false;
}

bool LC_ActionPasteToPoints::isAllowTriggerOnEmptySelection() {
    return false;
}

LC_ActionOptionsWidget *LC_ActionPasteToPoints::createOptionsWidget() {
    return new LC_PasteToPointsOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionPasteToPoints::createOptionsFiller() {
    return new LC_PasteToPointsOptionsFiller();
}

double LC_ActionPasteToPoints::getAngle() const {
    return m_angleRad;
}

void LC_ActionPasteToPoints::setAngle(const double angle) {
    m_angleRad = angle;
}

double LC_ActionPasteToPoints::getScaleFactor() const {
    return m_scaleFactor;
}

void LC_ActionPasteToPoints::setScaleFactor(const double scaleFactor) {
    m_scaleFactor = scaleFactor;
}

bool LC_ActionPasteToPoints::isRemovePointAfterPaste() const {
    return m_removePointAfterPaste;
}

void LC_ActionPasteToPoints::setRemovePointAfterPaste(const bool removePointAfterPaste) {
    m_removePointAfterPaste = removePointAfterPaste;
}

bool LC_ActionPasteToPoints::isEntityAllowedToSelect(RS_Entity *ent) const {
    return ent->rtti() == RS2::EntityPoint;
}

void LC_ActionPasteToPoints::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select insertion points") + getSelectionCompletionHintMsg(), MOD_CTRL(tr("Select and paste")));
}
