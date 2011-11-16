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

#include "rs_actionselect.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_actionselectsingle.h"


RS_ActionSelect::RS_ActionSelect(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView,
                                 RS2::ActionType nextAction,
                                 QVector<RS2::EntityType>* entityTypeList)

    :RS_ActionInterface("Select Entities", container, graphicView) {
    this->entityTypeList=entityTypeList;

    this->nextAction = nextAction;
    selectSingle=true;
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
    if(selectSingle==false){
        finish();
    }else{
        deleteSnapper();
    }
}

void RS_ActionSelect::requestFinish(){
    selectSingle=false;
}


void RS_ActionSelect::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    }
}



void RS_ActionSelect::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (selectSingle&& !isFinished()){
            RS_DIALOGFACTORY->requestToolBarSelect(this, nextAction);
        }else{
            if(container->countSelected()==0){
                //some nextAction segfault with empty selection
                //todo: make actions safe with empty selection
                RS_DIALOGFACTORY->commandMessage(tr("No entity selected!"));
                RS_DIALOGFACTORY->requestToolBarSelect(this, nextAction);
            } else{
                if ( entityTypeList != NULL &&  entityTypeList->size() >= 1 ){
                    //only select entity types from the given list
                    //fixme, need to handle resolution level

                    for (RS_Entity* e=container->firstEntity();
                         e!=NULL;
                         e=container->nextEntity()) {
                        if (e!=NULL && e->isSelected()) {
                            if ( entityTypeList->contains( e->rtti() ) == false ){
                                e->setSelected(false);
                            }
                        }
                    }
                }
                RS_DIALOGFACTORY->requestPreviousToolBar();
            }

            RS_DIALOGFACTORY->requestToolBarSelect(this, nextAction);
        }
    }
}

void RS_ActionSelect::updateMouseButtonHints() {
    switch(nextAction) {
    case RS2::ActionModifyAttributesNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to modify attributes"), tr("Cancel"));
        break;
    case RS2::ActionModifyDeleteNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to delete"), tr("Cancel"));
        break;
    case RS2::ActionModifyDeleteQuick:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to delete immediately"), tr("Cancel"));
        break;
    case RS2::ActionModifyMoveNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to move"), tr("Cancel"));
        break;
    case RS2::ActionEditCopyNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to copy"), tr("Cancel"));
        break;
    case RS2::ActionEditCutNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to cut"), tr("Cancel"));
        break;
    case RS2::ActionModifyRotateNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to rotate"), tr("Cancel"));
        break;
    case RS2::ActionModifyScaleNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to scale"), tr("Cancel"));
        break;
    case RS2::ActionModifyMirrorNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to mirror"), tr("Cancel"));
        break;
    case RS2::ActionModifyMoveRotateNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to move and rotate"), tr("Cancel"));
        break;
    case RS2::ActionModifyOffsetNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to create offset"), tr("Cancel"));
        break;
    case RS2::ActionModifyRotate2NoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select for two axis rotation"), tr("Cancel"));
        break;
    case RS2::ActionModifyExplodeTextNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to explode text"), tr("Cancel"));
        break;
    case RS2::ActionBlocksCreateNoSelect:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select to create block"), tr("Cancel"));
        break;

    default:
        RS_DIALOGFACTORY->updateMouseWidget(tr(""), tr(""));
    }
}


void RS_ActionSelect::updateMouseCursor() {
    if(graphicView!=NULL){
        if(isFinished()){
            graphicView->setMouseCursor(RS2::ArrowCursor);
        }else{
            graphicView->setMouseCursor(RS2::SelectCursor);
        }
    }
}
// EOF
