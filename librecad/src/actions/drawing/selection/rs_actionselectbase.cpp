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


#include <QMouseEvent>

#include "rs_actionselectbase.h"
#include "rs_graphicview.h"
#include "rs_selection.h"
#include "rs_debug.h"


RS_ActionSelectBase::RS_ActionSelectBase(const char* name,
        RS_EntityContainer& container,
        RS_GraphicView& graphicView,QList<RS2::EntityType> entityTypeList)
        :RS_PreviewActionInterface(name, container, graphicView),
        catchForSelectionEntityTypes(std::move(entityTypeList)){
}

/**
 * Default behaviour of this method is triggering the predecesing
 * action and finishing this one when the enter key is pressed.
 */
void RS_ActionSelectBase::keyReleaseEvent(QKeyEvent* e) {
    if (e->key()==Qt::Key_Return && predecessor) {
        finish(false);
    }
}

void RS_ActionSelectBase::keyPressEvent(QKeyEvent *e){
    int key = e->key();
    switch (key){
        case Qt::Key_Escape:{
            selectionFinishedByKey(e, true);
            break;
        }
        case Qt::Key_Enter:{
            if (container->countSelected() > 0){
                selectionFinishedByKey(e, false);
            }
            break;
        }
        default:
            break;
    }
}

RS2::CursorType RS_ActionSelectBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void RS_ActionSelectBase::selectEntity() {
    if (entityToSelect != nullptr){
        bool selectionAllowed = isEntityAllowedToSelect(entityToSelect);
        if (selectionAllowed){
            RS_Selection s(*container, graphicView);
            s.selectSingle(entityToSelect);
        } else {
            return;
        }
        updateSelectionWidget();
    }
    else {
        RS_DEBUG->print("RS_ActionSelectSingle::trigger: Entity is NULL\n");
    }
    entityToSelect = nullptr;
}

RS_Entity* RS_ActionSelectBase::selectionMouseMove(QMouseEvent *event) {
    RS_Entity* result = nullptr;
    snapPoint(event);
    deleteHighlights();
    auto ent = catchEntity(event, catchForSelectionEntityTypes);
    if (ent != nullptr){
        bool selectionAllowed = isEntityAllowedToSelect(ent);
        if (selectionAllowed){
            bool showRefPoints = isShowRefPointsOnHighlight();
            highlightHoverWithRefPoints(ent, showRefPoints);
            result = ent;
        }
    }
    drawHighlights();
    return result;
}

bool RS_ActionSelectBase::isShowRefPointsOnHighlight() {
    return highlightEntitiesRefPointsOnHover;
}

void RS_ActionSelectBase::deselectAll(){
    RS_Selection s(*container, graphicView);
    s.selectAll(false);
}

