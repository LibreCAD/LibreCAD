/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
//! File: lc_actiondrawdual.cpp
#include "lc_actiondrawdual.h"

#include <QMouseEvent>

#include "lc_quadraticutils.h"
#include "lc_undosection.h"
#include "rs_entitycontainer.h"


LC_ActionDrawDual::LC_ActionDrawDual(LC_ActionContext* context)
    : LC_ActionPreSelectionAwareBase("Draw Dual", context){
}

void LC_ActionDrawDual::init(int status) {
    setStatus(ChooseCenter);
    LC_ActionPreSelectionAwareBase::init(status);
}

void LC_ActionDrawDual::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero,
                                           [[maybe_unused]] const RS_Vector& coord) {
    if (getStatus() == ChooseCenter) {
        m_center = coord;
        trigger();
    }
}

void LC_ActionDrawDual::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (keepSelected) {
        // unselect(m_selectedEntities);
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionDrawDual::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to create dual (Enter to complete)"), MOD_SHIFT_LC(tr("Select contour")));
}

void LC_ActionDrawDual::updateActionPromptForSelected(int status) {
    updatePrompt(tr("Select Center"));
}

bool LC_ActionDrawDual::doCheckMayTrigger() {
    if (m_selectedEntities.empty()) {
        commandMessage(tr("No entities selected. Dual creation cancelled."));
        return false;
    }
    return true;
}

bool LC_ActionDrawDual::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    return createDualForSelected(ctx);
}

void LC_ActionDrawDual::doTriggerCompletion([[maybe_unused]] bool success) {
    finish();
}

void LC_ActionDrawDual::onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* event) {
    const RS_Vector snap = event->snapPoint;
    if (getStatus() == ChooseCenter) {
        m_center = snap;
        trigger();  // Always proceed to creation after center is chosen
    }
}

void LC_ActionDrawDual::onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) {
    finish();
}

void LC_ActionDrawDual::onMouseMoveEventSelected(int status, const LC_MouseEvent* event) {
    // fixme - preview?
    LC_ActionPreSelectionAwareBase::onMouseMoveEventSelected(status, event);
}

bool LC_ActionDrawDual::createDualForSelected(LC_DocumentModificationBatch& ctx){
    int count = 0;
    // Create a group to hold all dual copies
    for (RS_Entity* e : std::as_const(m_selectedEntities)) {
        // Delegate dual creation to the utility function
        RS_Entity* dualCopy = LC_QuadraticUtils::createDualAroundCenter(e, m_center);

        if (dualCopy != nullptr) {
            if (dualCopy->isContainer()) {
                auto* dualContainer = static_cast<RS_EntityContainer*>(dualCopy);
                for (RS_Entity* dual: *dualContainer) {
                    ctx += dual;
                    count++;
                }
                delete dualCopy;
            } else {
                ctx += dualCopy;
                count++;
            }
        }
    }

    if (count == 0) {
        commandMessage(tr("No supported entities could be dualized."));
        return false;
    }

    commandMessage(tr("Dual created around center (%1 entities)").arg(count));
    return true;
}

RS_Vector LC_ActionDrawDual::symmetricOf(const RS_Vector& p) const {
    return m_center * 2.0 - p;
}
