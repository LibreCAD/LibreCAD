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


#include "rs_eventhandler.h"

#include "rs_actioninterface.h"
#include "rs_dialogfactory.h"
#include "rs_commandevent.h"

/**
 * Constructor.
 */
RS_EventHandler::RS_EventHandler(RS_GraphicView* graphicView) {
    this->graphicView = graphicView;
//    actionIndex=-1;
    currentActions.clear();
    //    for (int i=0; i<RS_MAXACTIONS; ++i) {
    //        currentActions[i] = NULL;
    //    }
    coordinateInputEnabled = true;
    defaultAction = NULL;
}



/**
 * Destructor.
 */
RS_EventHandler::~RS_EventHandler() {
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler");
    if (defaultAction!=NULL) {
        defaultAction->finish();
        delete defaultAction;
        defaultAction = NULL;
    }

    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..");
    for(int i=0; i< currentActions.size();i++){
        //        currentActions[i]->finish(false);
        delete currentActions[i];
    }
    //    for (int i=0; i<RS_MAXACTIONS; ++i) {
    //        if (currentActions[i]!=NULL) {
    //            currentActions[i]->setFinished();
    //        }
    //    }
    //cleanUp();
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..: OK");
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: OK");
}


/**
 * Go back in current action.
 */
void RS_EventHandler::back() {
    QMouseEvent e(QEvent::MouseButtonRelease, QPoint(0,0),
                  Qt::RightButton, Qt::RightButton,Qt::NoModifier);
    mouseReleaseEvent(&e);
}



/**
 * Go enter pressed event for current action.
 */
void RS_EventHandler::enter() {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, 0);
    keyPressEvent(&e);
}


/**
 * Called by RS_GraphicView
 */
void RS_EventHandler::mousePressEvent(QMouseEvent* e) {
    if(hasAction()){
        currentActions.last()->mousePressEvent(e);
        e->accept();
    } else {
        if (defaultAction!=NULL) {
            defaultAction->mousePressEvent(e);
            e->accept();
        } else {
            RS_DEBUG->print("currently no action defined");
            e->ignore();
        }
    }
}



/**
 * Called by RS_GraphicView
 */
void RS_EventHandler::mouseReleaseEvent(QMouseEvent* e) {
    if(hasAction()){
        //    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
        //            !currentActions[actionIndex]->isFinished()) {
        RS_DEBUG->print("call action %s",
                        currentActions.last()->getName().toLatin1().data());

        currentActions.last()->mouseReleaseEvent(e);

        // Clean up actions - one might be finished now
        cleanUp();
        e->accept();
    } else {
        if (defaultAction!=NULL) {
            defaultAction->mouseReleaseEvent(e);
        } else {
            e->ignore();
        }
    }
}



/**
 * Called by RS_GraphicView
 */
void RS_EventHandler::mouseMoveEvent(QMouseEvent* e) {
    if(hasAction()){
        currentActions.last()->mouseMoveEvent(e);
        e->accept();
    } else {
        if (defaultAction!=NULL) {
            defaultAction->mouseMoveEvent(e);
            e->accept();
        } else {
            e->ignore();
        }
        //RS_DEBUG->print("currently no action defined");
    }
}



/**
 * Called by RS_GraphicView
 */
void RS_EventHandler::mouseLeaveEvent() {

    if(hasAction()){
        currentActions.last()->suspend();
    } else {
        if (defaultAction!=NULL) {
            defaultAction->suspend();
        }
        //RS_DEBUG->print("currently no action defined");
    }
}



/**
 * Called by RS_GraphicView
 */
void RS_EventHandler::mouseEnterEvent() {

    if(hasAction()){
        currentActions.last()->resume();
    } else {
        if (defaultAction!=NULL) {
            defaultAction->resume();
        }
    }
}



/**
 * Called by RS_GraphicView
 */
void RS_EventHandler::keyPressEvent(QKeyEvent* e) {

    if(hasAction()){
        currentActions.last()->keyPressEvent(e);
    } else {
        if (defaultAction!=NULL) {
            defaultAction->keyPressEvent(e);
        }
        else {
            e->ignore();
        }

        //RS_DEBUG->print("currently no action defined");
    }
}



/**
 * Called by RS_GraphicView
 */
void RS_EventHandler::keyReleaseEvent(QKeyEvent* e) {

    if(hasAction()){
        currentActions.last()->keyReleaseEvent(e);
    } else {
        if (defaultAction!=NULL) {
            defaultAction->keyReleaseEvent(e);
        }
        else {
            e->ignore();
        }
        //RS_DEBUG->print("currently no action defined");
    }
}



/**
 * Handles command line events.
 */
void RS_EventHandler::commandEvent(RS_CommandEvent* e) {
    RS_DEBUG->print("RS_EventHandler::commandEvent");

    QString cmd = e->getCommand();

    if (coordinateInputEnabled) {
        if (!e->isAccepted()) {

            if(hasAction()){
                // handle absolute cartesian coordinate input:
                if (cmd.contains(',') && cmd.at(0)!='@') {

                    int commaPos = cmd.indexOf(',');
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 001");
                    bool ok1, ok2;
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 002");
                    double x = RS_Math::eval(cmd.left(commaPos), &ok1);
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 003a");
                    double y = RS_Math::eval(cmd.mid(commaPos+1), &ok2);
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 004");

                    if (ok1 && ok2) {
                        RS_DEBUG->print("RS_EventHandler::commandEvent: 005");
                        RS_CoordinateEvent ce(RS_Vector(x,y));
                        RS_DEBUG->print("RS_EventHandler::commandEvent: 006");
                        currentActions.last()->coordinateEvent(&ce);
                    } else {
                        if (RS_DIALOGFACTORY!=NULL) {
                            RS_DIALOGFACTORY->commandMessage(
                                        "Expression Syntax Error");
                        }
                    }
                    e->accept();
                }

                // handle relative cartesian coordinate input:
                if (!e->isAccepted()) {
                    if (cmd.contains(',') && cmd.at(0)=='@') {
                        int commaPos = cmd.indexOf(',');
                        bool ok1, ok2;
                        double x = RS_Math::eval(cmd.mid(1, commaPos-1), &ok1);
                        double y = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

                        if (ok1 && ok2) {
                            RS_CoordinateEvent ce(RS_Vector(x,y) +
                                                  graphicView->getRelativeZero());

                            currentActions.last()->coordinateEvent(&ce);
                            //                            currentActions[actionIndex]->coordinateEvent(&ce);
                        } else {
                            if (RS_DIALOGFACTORY!=NULL) {
                                RS_DIALOGFACTORY->commandMessage(
                                            "Expression Syntax Error");
                            }
                        }
                        e->accept();
                    }
                }

                // handle absolute polar coordinate input:
                if (!e->isAccepted()) {
                    if (cmd.contains('<') && cmd.at(0)!='@') {
                        int commaPos = cmd.indexOf('<');
                        bool ok1, ok2;
                        double r = RS_Math::eval(cmd.left(commaPos), &ok1);
                        double a = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

                        if (ok1 && ok2) {
                            RS_Vector pos;
                            pos.setPolar(r,RS_Math::deg2rad(a));
                            RS_CoordinateEvent ce(pos);
                            currentActions.last()->coordinateEvent(&ce);
                        } else {
                            if (RS_DIALOGFACTORY!=NULL) {
                                RS_DIALOGFACTORY->commandMessage(
                                            "Expression Syntax Error");
                            }
                        }
                        e->accept();
                    }
                }

                // handle relative polar coordinate input:
                if (!e->isAccepted()) {
                    if (cmd.contains('<') && cmd.at(0)=='@') {
                        int commaPos = cmd.indexOf('<');
                        bool ok1, ok2;
                        double r = RS_Math::eval(cmd.mid(1, commaPos-1), &ok1);
                        double a = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

                        if (ok1 && ok2) {
                            RS_Vector pos;
                            pos.setPolar(r,RS_Math::deg2rad(a));
                            RS_CoordinateEvent ce(pos +
                                                  graphicView->getRelativeZero());
                            currentActions.last()->coordinateEvent(&ce);
                        } else {
                            if (RS_DIALOGFACTORY!=NULL) {
                                RS_DIALOGFACTORY->commandMessage(
                                            "Expression Syntax Error");
                            }
                        }
                        e->accept();
                    }
                }

                // send command event directly to current action:
                if (!e->isAccepted()) {
//                    std::cout<<"RS_EventHandler::commandEvent(RS_CommandEvent* e): sending cmd("<<qPrintable(e->getCommand()) <<") to action: "<<currentActions.last()->rtti()<<std::endl;
                    currentActions.last()->commandEvent(e);
                }
            }else{
            //send the command to default action
                if (defaultAction!=NULL) {
                    defaultAction->commandEvent(e);
                }
            }
            // do not accept command here. Actions themselves should be responsible to accept commands
//            e->accept();
        }
    }

    RS_DEBUG->print("RS_EventHandler::commandEvent: OK");
}



/**
 * Enables coordinate input in the command line.
 */
void RS_EventHandler::enableCoordinateInput() {
    coordinateInputEnabled = true;
}



/**
 * Enables coordinate input in the command line.
 */
void RS_EventHandler::disableCoordinateInput() {
    coordinateInputEnabled = false;
}



/**
 * @return Current action.
 */
RS_ActionInterface* RS_EventHandler::getCurrentAction() {
    if(hasAction()){
        return currentActions.last();
    } else {
        return defaultAction;
    }
}



/**
 * @return The current default action.
 */
RS_ActionInterface* RS_EventHandler::getDefaultAction() {
    return defaultAction;
}



/**
 * Sets the default action.
 */
void RS_EventHandler::setDefaultAction(RS_ActionInterface* action) {
    if (defaultAction!=NULL) {
        defaultAction->finish();
        delete defaultAction;
        //        defaultAction = NULL;
    }

    defaultAction = action;
}



/**
 * Sets the current action.
 */
void RS_EventHandler::setCurrentAction(RS_ActionInterface* action) {
    RS_DEBUG->print("RS_EventHandler::setCurrentAction");
    if (action==NULL) {
        return;
    }

    // Predecessor of the new action or NULL:
    RS_ActionInterface* predecessor = NULL;

    // Suspend current action:
    if(hasAction()){
        predecessor = currentActions.last();
        predecessor->suspend();
        predecessor->hideOptions();
    }
    else {
        if (defaultAction!=NULL) {
            predecessor = defaultAction;
            predecessor->suspend();
            predecessor->hideOptions();
        }
    }

    //    // Forget about the oldest action and make space for the new action:
    //    if (actionIndex==RS_MAXACTIONS-1) {
    //        // delete oldest action if necessary (usually never happens):
    //        if (currentActions[0]!=NULL) {
    //            currentActions[0]->finish();
    //            delete currentActions[0];
    //            currentActions[0] = NULL;
    //        }
    //        // Move up actionstack (optimize):
    //        for (int i=0; i<RS_MAXACTIONS-1; ++i) {
    //            currentActions[i] = currentActions[i+1];
    //        }
    //    } else if (actionIndex<RS_MAXACTIONS-1) {
    //        actionIndex++;
    //    }

    // Set current action:
    currentActions.push_back(action);
    RS_DEBUG->print("RS_EventHandler::setCurrentAction: current action is: %s",
                    currentActions.last()->getName().toLatin1().data());

    // Initialisation of our new action:
    RS_DEBUG->print("RS_EventHandler::setCurrentAction: init current action");
    action->init();
    // ## new:
    if (action->isFinished()==false) {
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: show options");
        currentActions.last()->showOptions();
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: set predecessor");
        action->setPredecessor(predecessor);
    }

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: cleaning up..");
    cleanUp();

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: debugging actions");
    debugActions();
    RS_DEBUG->print("RS_GraphicView::setCurrentAction: OK");
}



/**
 * Kills all running selection actions. Called when a selection action
 * is launched to reduce confusion.
 */
void RS_EventHandler::killSelectActions() {

    for (auto it=currentActions.begin();it != currentActions.end();){
        if ((*it)->rtti()==RS2::ActionSelectSingle ||
                (*it)->rtti()==RS2::ActionSelectContour ||
                (*it)->rtti()==RS2::ActionSelectWindow ||
                (*it)->rtti()==RS2::ActionSelectIntersected ||
                (*it)->rtti()==RS2::ActionSelectLayer) {
            if( ! (*it)->isFinished()){
                (*it)->finish();
            }
            delete *it;
            it= currentActions.erase(it);
        }else{
            it++;
        }
    }
}



/**
 * Kills all running actions. Called when a window is closed.
 */
void RS_EventHandler::killAllActions() {

    while(currentActions.size()>0){
        if ( ! currentActions.first()->isFinished() ){
            currentActions.first()->finish();
        }
        //need to check the size again after finish(), bug#3451525, 3451415
        if(currentActions.size()==0) return;
        delete currentActions.takeFirst();
    }
//    if(defaultAction->rtti() == RS2::ActionDefault){

//    }
    //cleanup default action, issue#285
    defaultAction->init(0);
}



/**
 * @return true if the action is within currentActions
 */
bool RS_EventHandler::isValid(RS_ActionInterface* action){
    return currentActions.indexOf(action) >= 0;
}

/**
 * @return true if there is at least one action in the action stack.
 */
bool RS_EventHandler::hasAction() {

    while(currentActions.size()>0 ) {
        if(! currentActions.last()->isFinished()){
            return true;
        }
        delete currentActions.last();
        currentActions.pop_back();
    }
    return false;
}



/**
 * Garbage collector for actions.
 */
void RS_EventHandler::cleanUp() {
    RS_DEBUG->print("RS_EventHandler::cleanUp");

    for (auto it=currentActions.begin();it != currentActions.end();){

        if( (*it)->isFinished()){
//            (*it)->finish();
            delete *it;
            it= currentActions.erase(it);
        }else{
            it++;
        }
    }
    if(hasAction()){
        currentActions.last()->resume();
        currentActions.last()->showOptions();
    } else {
        if (defaultAction!=NULL) {
            defaultAction->resume();
            defaultAction->showOptions();
        }
    }
    RS_DEBUG->print("RS_EventHandler::cleanUp: OK");
}



/**
 * Sets the snap mode for all currently active actions.
 */
void RS_EventHandler::setSnapMode(RS_SnapMode sm) {
    for (auto it=currentActions.begin();it != currentActions.end();it++){
        if(! (*it)->isFinished()){
            (*it)->setSnapMode(sm);
        }
    }

    if (defaultAction!=NULL) {
        defaultAction->setSnapMode(sm);
    }
}


/**
 * Sets the snap restriction for all currently active actions.
 */
void RS_EventHandler::setSnapRestriction(RS2::SnapRestriction sr) {
    for (auto it=currentActions.begin();it != currentActions.end();it++){
        if(! (*it)->isFinished()){
            (*it)->setSnapRestriction(sr);
        }
    }

    if (defaultAction!=NULL) {
        defaultAction->setSnapRestriction(sr);
    }
}


void RS_EventHandler::debugActions() {
    //        std::cout<<"action queue size=:"<<currentActions.size()<<std::endl;
    RS_DEBUG->print("---");
    for(int i=0;i<currentActions.size();i++){

        if (i == currentActions.size() - 1 ) {
            RS_DEBUG->print("Current");
        }
        RS_DEBUG->print("Action %03d: %s [%s]",
                        i, currentActions.at(i)->getName().toLatin1().data(),
                        currentActions.at(i)->isFinished() ? "finished" : "active");
    }
}

// EOF
