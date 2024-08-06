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

#include "rs_actionselect.h"
#include "rs_actionselectsingle.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"


RS_ActionSelect::RS_ActionSelect(QG_ActionHandler* a_handler,
                                 RS_EntityContainer& container,
                                 RS_GraphicView& graphicView,
                                 RS2::ActionType nextAction,
                                 QList<RS2::EntityType> allowedEntityTypes)
	:RS_ActionInterface("Select Entities", container, graphicView)
    ,action_handler(a_handler)
    ,nextAction(nextAction)
    , entityTypeList(std::move(allowedEntityTypes)){
    setActionType(RS2::ActionSelect);
}

void RS_ActionSelect::init(int status) {
    RS_ActionInterface::init(status);
    if(status >= 0 ) {
        graphicView->setCurrentAction(
                    new RS_ActionSelectSingle(*container, *graphicView, this, entityTypeList));
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

int RS_ActionSelect::countSelected(){
    int ret = container->countSelected();
    if (ret == 0){
        commandMessage(tr("No entity selected!"));
    }
    return ret;
}

void RS_ActionSelect::updateMouseButtonHints() {
    switch(nextAction) {
        case RS2::ActionOrderNoSelect:
            updateMouseWidgetTRCancel(tr("Select entities to order"));
            break;
        case RS2::ActionModifyDeleteQuick:
            updateMouseWidgetTRCancel(tr("Select to delete immediately"));
            break;
        case RS2::ActionEditCopyNoSelect:
            updateMouseWidgetTRCancel(tr("Select to copy"));
            break;
        case RS2::ActionEditCutNoSelect:
            updateMouseWidgetTRCancel(tr("Select to cut"));
            break;
        case RS2::ActionBlocksCreateNoSelect:
            updateMouseWidgetTRCancel(tr("Select to create block"));
            break;
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
        action_handler->setCurrentAction(nextAction);
    }
}
