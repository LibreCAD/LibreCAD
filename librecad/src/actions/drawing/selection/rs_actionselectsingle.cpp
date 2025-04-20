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

#include "rs_actionselectsingle.h"

#include "rs_entity.h"
#include "rs_selection.h"

RS_ActionSelectSingle::RS_ActionSelectSingle(LC_ActionContext *actionContext,
                                             RS_ActionInterface* action_select,
                                             const QList<RS2::EntityType> &entityTypeList)
    :RS_ActionSelectBase("Select Entities", actionContext,RS2::ActionSelectSingle, entityTypeList)
    ,m_actionSelect(action_select){
}

RS_ActionSelectSingle::RS_ActionSelectSingle(enum RS2::EntityType selectType,
                                             LC_ActionContext *actionContext,
                                             RS_ActionInterface* action_select,
                                             const QList<RS2::EntityType> &entityTypeList)
    :RS_ActionSelectBase("Select Entities", actionContext, RS2::ActionSelectSingle, entityTypeList)
    ,m_actionSelect(action_select)
    ,m_typeToSelect(selectType){
}

void RS_ActionSelectSingle::trigger(){
    selectEntity(m_entityToSelect,m_selectContour);
    m_selectContour = false;
}

void RS_ActionSelectSingle::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *event) {
    selectionMouseMove(event);
}

void RS_ActionSelectSingle::selectionFinishedByKey(QKeyEvent *e, [[maybe_unused]]bool escape) {
    finish(false);
    m_actionSelect->keyPressEvent(e);
}

void RS_ActionSelectSingle::onMouseLeftButtonRelease([[maybe_unused]] int status, LC_MouseEvent *e) {
    m_entityToSelect = catchEntityByEvent(e, m_catchForSelectionEntityTypes);
    if (m_entityToSelect != nullptr){
       m_selectContour = e->isShift;
       trigger();
    }
}

void RS_ActionSelectSingle::doSelectEntity(RS_Entity *entityToSelect, bool selectContour) const {
    if (entityToSelect != nullptr){
        RS_Selection s(*m_container, m_viewport);
        // try to minimize selection clicks - and select contour based on selected entity. May be optional, but what for?
        if (entityToSelect->isAtomic() && selectContour) {
            s.selectContour(entityToSelect);
        }
        else{
            s.selectSingle(entityToSelect);
        }
    }
}

void RS_ActionSelectSingle::onMouseRightButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    finish();
    if (m_actionSelect->rtti() == RS2::ActionSelect)
        m_actionSelect->finish();
    else
        m_actionSelect->mouseReleaseEvent(e->originalEvent); // fixme - sand - review, rework
}

RS2::CursorType RS_ActionSelectSingle::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

enum RS2::EntityType RS_ActionSelectSingle::getTypeToSelect(){
    return m_typeToSelect;
}

bool RS_ActionSelectSingle::isEntityAllowedToSelect(RS_Entity *ent) const {
    if (m_typeToSelect == RS2::EntityType::EntityUnknown)
        return true;
    else
        return ent ->rtti() == m_typeToSelect;
}

void RS_ActionSelectSingle::updateMouseButtonHints() {
    updateMouseWidgetTRCancel(tr("Specify entity to select"), MOD_SHIFT_LC(tr("Select contour")));
}
