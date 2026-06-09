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

#include "lc_action_block_explode.h"

#include "rs_document.h"
#include "rs_entity.h"
#include "rs_modification.h"

/**
 * Constructor.
 */
LC_ActionBlocksExplode::LC_ActionBlocksExplode(LC_ActionContext *actionContext)
    :LC_ActionPreSelectionAwareBase("ActionBlocksExplode", actionContext,m_actionType=RS2::ActionBlocksExplode) {
}

bool LC_ActionBlocksExplode::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    RS_Modification::explode(m_selectedEntities, ctx);
    ctx.setActiveLayerAndPen(false, false);
    return true;
}

void LC_ActionBlocksExplode::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    unselect(m_selectedEntities); // deselect atomics, if any included into selection. They should not be included into selection, yet still...
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionBlocksExplode::doTriggerCompletion([[maybe_unused]]bool success) {
    m_document->updateInserts();
}

bool LC_ActionBlocksExplode::isEntityAllowedToSelect(RS_Entity *ent) const {
    return ent->isContainer();
}

void LC_ActionBlocksExplode::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to explode container")+getSelectionCompletionHintMsg(),
        MOD_CTRL(tr("Select and explode")));
}
