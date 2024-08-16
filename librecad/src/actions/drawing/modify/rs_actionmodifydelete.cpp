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

#include "rs_actionmodifydelete.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_modification.h"

RS_ActionModifyDelete::RS_ActionModifyDelete(RS_EntityContainer &container,RS_GraphicView& graphicView)
    :LC_ActionPreSelectionAwareBase("Delete Entities",container, graphicView) {
    actionType=RS2::ActionModifyDelete;
}

void RS_ActionModifyDelete::trigger() {
    RS_DEBUG->print("RS_ActionModifyDelete::trigger()");
    RS_Modification m(*container, graphicView);
    m.remove(selectedEntities);
}

void RS_ActionModifyDelete::selectionCompleted(bool singleEntity,  [[maybe_unused]]bool fromInit) {
    trigger();
    if (singleEntity) {
        deselectAll();
    } else {
        finish(false);
    }
    updateSelectionWidget();
}

void RS_ActionModifyDelete::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to delete"),MOD_CTRL(tr("Delete immediately after selection")));
}

RS2::CursorType RS_ActionModifyDelete::doGetMouseCursorSelected([[maybe_unused]] int status){
    return RS2::DelCursor;
}
