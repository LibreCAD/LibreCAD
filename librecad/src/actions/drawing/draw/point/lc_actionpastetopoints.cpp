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

#include "lc_actionpastetopoints.h"

#include "lc_pastetopointsoptions.h"
#include "rs_clipboard.h"
#include "rs_entity.h"
#include "rs_modification.h"

LC_ActionPasteToPoints::LC_ActionPasteToPoints(LC_ActionContext *actionContext):
    LC_ActionPreSelectionAwareBase("PasteToPoints", actionContext, RS2::ActionPasteToPoints, {RS2::EntityPoint}){
}

void LC_ActionPasteToPoints::init(int status) {
    if (RS_CLIPBOARD->count() == 0){
        commandMessage(tr("Clipboard is empty"));
        finish(false);
    }
    else{
        showOptions();
        LC_ActionPreSelectionAwareBase::init(status);
    }
}

void LC_ActionPasteToPoints::doTrigger(bool keepSelected) {
    undoCycleStart();
    RS_Modification m(*m_container, m_viewport, false);
    for (auto p: m_selectedEntities){
        RS_Vector currentPoint = p->getCenter();
        const RS_PasteData &pasteData = RS_PasteData(currentPoint, m_scaleFactor , m_angleRad, false, "");
        m.paste(pasteData);
        // fixme - some progress is needed there, ++++ speed improvement for paste operation!!
//        LC_ERR << "Paste: " << currentPoint;

        if (m_removePointAfterPaste){
            undoableDeleteEntity(p);
        }
        else{
            p->setSelected(keepSelected);
        }
    }
    undoCycleEnd();
}

bool LC_ActionPasteToPoints::doUpdateAngleByInteractiveInput(const QString& tag, double angle) {
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
    return new LC_PasteToPointsOptions();
}

double LC_ActionPasteToPoints::getAngle() const {
    return m_angleRad;
}

void LC_ActionPasteToPoints::setAngle(double a) {
    m_angleRad = a;
}

double LC_ActionPasteToPoints::getScaleFactor() const {
    return m_scaleFactor;
}

void LC_ActionPasteToPoints::setScaleFactor(double f) {
    m_scaleFactor = f;
}

bool LC_ActionPasteToPoints::isRemovePointAfterPaste() const {
    return m_removePointAfterPaste;
}

void LC_ActionPasteToPoints::setRemovePointAfterPaste(bool val) {
    m_removePointAfterPaste = val;
}

bool LC_ActionPasteToPoints::isEntityAllowedToSelect(RS_Entity *ent) const {
    return ent->rtti() == RS2::EntityPoint;
}

void LC_ActionPasteToPoints::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel("Select insertion points (Enter to complete)"), MOD_CTRL(tr("Select and paste"));
}
