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

#include "rs_actionmodifyrevertdirection.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_modification.h"

RS_ActionModifyRevertDirection::RS_ActionModifyRevertDirection(RS_EntityContainer& container, RS_GraphicView& graphicView)
	:LC_ActionPreSelectionAwareBase("Revert direction", container, graphicView,{}){
    actionType = RS2::ActionModifyRevertDirection;
}

void RS_ActionModifyRevertDirection::trigger() {
    RS_DEBUG->print("RS_ActionModifyRevertDirection::trigger");

    RS_Modification m(*container, graphicView);
    m.revertDirection(selectedEntities);
}

bool RS_ActionModifyRevertDirection::isShowRefPointsOnHighlight() {
    return true;
}

void RS_ActionModifyRevertDirection::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select to revert direction"), MOD_CTRL(tr("Revert immediately after selection")));
}

bool RS_ActionModifyRevertDirection::isEntityAllowedToSelect(RS_Entity *ent) const {
    if (ent->isContainer()){ // todo - check this, it seems not all containers are properly supported
        return true;
    }
    else{
        int rtti = ent->rtti();
        switch (rtti){
            case RS2::EntityParabola:
            case RS2::EntityPolyline:
            case RS2::EntityLine:
            case  RS2::EntityContainer:
            case  RS2::EntityArc:
            case  RS2::EntitySplinePoints:
                return true;
            default:
                return false;
        }
    }
}
