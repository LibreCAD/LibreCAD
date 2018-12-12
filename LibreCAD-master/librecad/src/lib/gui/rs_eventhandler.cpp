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

#include <QRegExp>
#include <QAction>
#include <QMouseEvent>
#include "rs_eventhandler.h"
#include "rs_actioninterface.h"
#include "rs_dialogfactory.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_commands.h"
#include "rs_math.h"
#include "rs_snapper.h"
#include "rs_debug.h"

/**
 * Constructor.
 */
RS_EventHandler::RS_EventHandler(QObject* parent) : QObject(parent)
{
    connect(parent, SIGNAL(relative_zero_changed(const RS_Vector&)),
            this, SLOT(setRelativeZero(const RS_Vector&)));
}

/**
 * Destructor.
 */
RS_EventHandler::~RS_EventHandler() {
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler");
	delete defaultAction;
	defaultAction = nullptr;

    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..");
    for(auto a: currentActions){
        delete a;
    }
    currentActions.clear();
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
    if (!hasAction() && q_action)
    {
        q_action->setChecked(false);
        q_action = nullptr;
    }
}



/**
 * Go enter pressed event for current action.
 */
void RS_EventHandler::enter() {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, 0);
    keyPressEvent(&e);
}


/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mousePressEvent(QMouseEvent* e) {
    if(hasAction()){
        currentActions.last()->mousePressEvent(e);
        e->accept();
    } else {
        if (defaultAction) {
            defaultAction->mousePressEvent(e);
            e->accept();
        } else {
            RS_DEBUG->print("currently no action defined");
            e->ignore();
        }
    }
}



/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseReleaseEvent(QMouseEvent* e) {
    if(hasAction()){
        //    if (actionIndex>=0 && currentActions[actionIndex] &&
        //            !currentActions[actionIndex]->isFinished()) {
        RS_DEBUG->print("call action %s",
                        currentActions.last()->getName().toLatin1().data());

        currentActions.last()->mouseReleaseEvent(e);

        // Clean up actions - one might be finished now
        cleanUp();
        e->accept();
    } else {
        if (defaultAction) {
            defaultAction->mouseReleaseEvent(e);
        } else {
            e->ignore();
        }
    }
}



/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseMoveEvent(QMouseEvent* e)
{
    if(hasAction())
        currentActions.last()->mouseMoveEvent(e);

    else if (defaultAction)
        defaultAction->mouseMoveEvent(e);
}

/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseLeaveEvent() {

    if(hasAction()){
        currentActions.last()->suspend();
    } else {
        if (defaultAction) {
            defaultAction->suspend();
        }
        //RS_DEBUG->print("currently no action defined");
    }
}



/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseEnterEvent() {

    if(hasAction()){
        currentActions.last()->resume();
    } else {
        if (defaultAction) {
            defaultAction->resume();
        }
    }
}



/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::keyPressEvent(QKeyEvent* e) {

    if(hasAction()){
        currentActions.last()->keyPressEvent(e);
    } else {
        if (defaultAction) {
            defaultAction->keyPressEvent(e);
        }
        else {
            e->ignore();
        }

        //RS_DEBUG->print("currently no action defined");
    }
}



/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::keyReleaseEvent(QKeyEvent* e) {

    if(hasAction()){
        currentActions.last()->keyReleaseEvent(e);
    } else {
        if (defaultAction) {
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
					} else
						RS_DIALOGFACTORY->commandMessage(
									"Expression Syntax Error");
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
                            RS_CoordinateEvent ce(RS_Vector(x,y) + relative_zero);

                            currentActions.last()->coordinateEvent(&ce);
                            //                            currentActions[actionIndex]->coordinateEvent(&ce);
						} else
							RS_DIALOGFACTORY->commandMessage(
										"Expression Syntax Error");
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
							RS_Vector pos{
								RS_Vector::polar(r,RS_Math::deg2rad(a))};
                            RS_CoordinateEvent ce(pos);
                            currentActions.last()->coordinateEvent(&ce);
						} else
                                RS_DIALOGFACTORY->commandMessage(
                                            "Expression Syntax Error");
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
							RS_Vector pos = RS_Vector::polar(r,RS_Math::deg2rad(a));
                            RS_CoordinateEvent ce(pos + relative_zero);
                            currentActions.last()->coordinateEvent(&ce);
						} else
                                RS_DIALOGFACTORY->commandMessage(
                                            "Expression Syntax Error");
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
                if (defaultAction) {
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
RS_ActionInterface* RS_EventHandler::getCurrentAction(){
    if(hasAction()){
        return currentActions.last();
    } else {
        return defaultAction;
    }
}



/**
 * @return The current default action.
 */
RS_ActionInterface* RS_EventHandler::getDefaultAction() const{
    return defaultAction;
}



/**
 * Sets the default action.
 */
void RS_EventHandler::setDefaultAction(RS_ActionInterface* action) {
    if (defaultAction) {
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
        if (defaultAction) {
            predecessor = defaultAction;
            predecessor->suspend();
            predecessor->hideOptions();
        }
    }

    //    // Forget about the oldest action and make space for the new action:
    //    if (actionIndex==RS_MAXACTIONS-1) {
    //        // delete oldest action if necessary (usually never happens):
    //        if (currentActions[0]) {
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
    if (q_action)
        q_action->setChecked(true);
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
void RS_EventHandler::killAllActions()
{
	RS_DEBUG->print(__FILE__ ": %s: line %d: begin\n", __func__, __LINE__);

    if (q_action)
    {
        q_action->setChecked(false);
        q_action = nullptr;
    }

	for(auto p: currentActions)
    {
		if (!p->isFinished())
        {
			p->finish();
		}
	}

	RS_DEBUG->print(__FILE__ ": %s: line %d: begin\n", __func__, __LINE__);
	defaultAction->init(0);
}



/**
 * @return true if the action is within currentActions
 */
bool RS_EventHandler::isValid(RS_ActionInterface* action) const{
    return currentActions.indexOf(action) >= 0;
}

/**
 * @return true if there is at least one action in the action stack.
 */
bool RS_EventHandler::hasAction()
{
    foreach (RS_ActionInterface* a, currentActions)
    {
        if(!a->isFinished())
            return true;
    }
    return false;
}



/**
 * Garbage collector for actions.
 */
void RS_EventHandler::cleanUp() {
    RS_DEBUG->print("RS_EventHandler::cleanUp");

    for (auto it=currentActions.begin(); it != currentActions.end();)
    {
        if( (*it)->isFinished())
        {
            delete *it;
            it= currentActions.erase(it);
        }else{
            ++it;
        }
    }
    if(hasAction()){
        currentActions.last()->resume();
        currentActions.last()->showOptions();
    } else {
		if (defaultAction) {
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
    for(auto a: currentActions){
        if( ! a->isFinished()){
            a->setSnapMode(sm);
        }
    }

	if (defaultAction) {
        defaultAction->setSnapMode(sm);
    }
}


/**
 * Sets the snap restriction for all currently active actions.
 */
void RS_EventHandler::setSnapRestriction(RS2::SnapRestriction sr) {

    for(auto a: currentActions){
        if( ! a->isFinished()){
            a->setSnapRestriction(sr);
        }
    }

    if (defaultAction) {
        defaultAction->setSnapRestriction(sr);
    }
}


void RS_EventHandler::debugActions() const{
    //        std::cout<<"action queue size=:"<<currentActions.size()<<std::endl;
    RS_DEBUG->print("---");
    for(int i=0;i<currentActions.size();++i){

        if (i == currentActions.size() - 1 ) {
            RS_DEBUG->print("Current");
        }
        RS_DEBUG->print("Action %03d: %s [%s]",
                        i, currentActions.at(i)->getName().toLatin1().data(),
                        currentActions.at(i)->isFinished() ? "finished" : "active");
    }
}

void RS_EventHandler::setQAction(QAction* action)
{
    if (q_action)
    {
        q_action->setChecked(false);
        killAllActions();
    }
    q_action = action;
}

void RS_EventHandler::setRelativeZero(const RS_Vector& point)
{
    relative_zero = point;
}

bool RS_EventHandler::inSelectionMode()
{
    switch (getCurrentAction()->rtti())
    {
        case RS2::ActionDefault:
        case RS2::ActionSelectSingle:
        case RS2::ActionSelectWindow:
        case RS2::ActionDeselectWindow:
        case RS2::ActionSelectContour:
        case RS2::ActionSelectIntersected:
        case RS2::ActionDeselectIntersected:
        case RS2::ActionSelectLayer:
            return true;
        default:
            return false;
    }
}

// EOF
