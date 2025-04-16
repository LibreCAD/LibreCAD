/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
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

#include "rs_actionselect.h"

#include "lc_actioncontext.h"
#include "../drawing/selection/rs_actionselectsingle.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"


RS_ActionSelect::RS_ActionSelect(LC_ActionContext *actionContext,
                                 RS2::ActionType nextAction,
                                 QList<RS2::EntityType> allowedEntityTypes)
	:RS_ActionInterface("Select Entities", actionContext, RS2::ActionSelect)
    ,nextAction(nextAction)
    , entityTypeList(std::move(allowedEntityTypes)){
}

void RS_ActionSelect::init(int status) {
    RS_ActionInterface::init(status);
    if(status >= 0 ) {
        // fixme - sand - files direct action creation!
        m_graphicView->setCurrentAction(std::make_shared<RS_ActionSelectSingle>(m_actionContext, this, entityTypeList));
    }
    deleteSnapper();
}

void RS_ActionSelect::resume(){
    RS_ActionInterface::resume();
    deleteSnapper();
}

void RS_ActionSelect::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e){
    initPrevious(status);
}

int RS_ActionSelect::countSelected() const {
    int ret = m_container->countSelected();
    if (ret == 0){
        commandMessage(tr("No entity selected!"));
    }
    return ret;
}

void RS_ActionSelect::updateMouseButtonHints() {
    switch(nextAction) {
        default:
            updateMouseWidget();
    }
}
RS2::CursorType RS_ActionSelect::doGetMouseCursor([[maybe_unused]] int status){
    return isFinished() ? RS2::ArrowCursor : RS2::SelectCursor;
}

void RS_ActionSelect::keyPressEvent(QKeyEvent* e){
    if (e->key()==Qt::Key_Enter && countSelected() > 0){
        finish();
        switchToAction(nextAction);
    }
}
