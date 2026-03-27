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

#include "lc_action_modify_explode_text.h"

#include "rs_document.h"
#include "rs_modification.h"

/**
 * Constructor.
 */
LC_ActionModifyExplodeText::LC_ActionModifyExplodeText(LC_ActionContext *actionContext)
    :LC_ActionPreSelectionAwareBase("ActionModifyExplodeText", actionContext,RS2::ActionModifyExplodeText,
        {RS2::EntityMText, RS2::EntityText}) {
}

LC_ActionModifyExplodeText::~LC_ActionModifyExplodeText() = default;

bool LC_ActionModifyExplodeText::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    RS_Modification::explodeTextIntoLetters(m_selectedEntities, ctx);
    return true;
}

void LC_ActionModifyExplodeText::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyExplodeText::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select to explode text") + getSelectionCompletionHintMsg(), MOD_CTRL(tr("Explode immediately after selection")));
}
