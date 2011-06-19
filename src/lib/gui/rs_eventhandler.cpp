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
#include "rs_coordinateevent.h"
//Added by qt3to4:
#include <QEvent>

/**
 * Constructor.
 */
RS_EventHandler::RS_EventHandler(RS_GraphicView* graphicView) {
    this->graphicView = graphicView;
    actionIndex=-1;
    for (int i=0; i<RS_MAXACTIONS; ++i) {
        currentActions[i] = NULL;
    }
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
    for (int i=0; i<RS_MAXACTIONS; ++i) {
        if (currentActions[i]!=NULL) {
            currentActions[i]->setFinished();
        }
    }
    cleanUp();
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..: OK");
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: OK");
}


/**
 * Go back in current action.
 */
void RS_EventHandler::back() {
    RS_MouseEvent e(QEvent::MouseButtonRelease, QPoint(0,0),
                    Qt::RightButton, Qt::RightButton);
    mouseReleaseEvent(&e);
}



/**
 * Go enter pressed event for current action.
 */
void RS_EventHandler::enter() {
    RS_KeyEvent e(QEvent::KeyPress, Qt::Key_Enter, '\n', 0);
    keyPressEvent(&e);
}


/**
 * Called by RS_GraphicView 
 */
void RS_EventHandler::mousePressEvent(RS_MouseEvent* e) {
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL) {
        currentActions[actionIndex]->mousePressEvent(e);
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
void RS_EventHandler::mouseReleaseEvent(RS_MouseEvent* e) {

    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {
        RS_DEBUG->print("call action %s",
                        currentActions[actionIndex]->getName().latin1());

        currentActions[actionIndex]->mouseReleaseEvent(e);

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
void RS_EventHandler::mouseMoveEvent(RS_MouseEvent* e) {
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {
        currentActions[actionIndex]->mouseMoveEvent(e);
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
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {
        currentActions[actionIndex]->suspend();
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
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {
        currentActions[actionIndex]->resume();
    } else {
        if (defaultAction!=NULL) {
            defaultAction->resume();
        }
    }
}



/**
 * Called by RS_GraphicView 
 */
void RS_EventHandler::keyPressEvent(RS_KeyEvent* e) {
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {
        currentActions[actionIndex]->keyPressEvent(e);
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
void RS_EventHandler::keyReleaseEvent(RS_KeyEvent* e) {
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {
        currentActions[actionIndex]->keyReleaseEvent(e);
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

    RS_String cmd = e->getCommand();

    if (coordinateInputEnabled) {
        if (!e->isAccepted()) {
            // handle absolute cartesian coordinate input:
            if (cmd.contains(',') && cmd.at(0)!='@') {
                if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
                        !currentActions[actionIndex]->isFinished()) {
                    int commaPos = cmd.find(',');
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
                        currentActions[actionIndex]->coordinateEvent(&ce);
                    } else {
                        if (RS_DIALOGFACTORY!=NULL) {
                            RS_DIALOGFACTORY->commandMessage(
                                "Expression Syntax Error");
                        }
                    }
                    e->accept();
                }
            }
        }

        // handle relative cartesian coordinate input:
        if (!e->isAccepted()) {
            if (cmd.contains(',') && cmd.at(0)=='@') {
                if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
                        !currentActions[actionIndex]->isFinished()) {
                    int commaPos = cmd.find(',');
                    bool ok1, ok2;
                    double x = RS_Math::eval(cmd.mid(1, commaPos-1), &ok1);
                    double y = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

                    if (ok1 && ok2) {
                        RS_CoordinateEvent ce(RS_Vector(x,y) +
                                              graphicView->getRelativeZero());
                        currentActions[actionIndex]->coordinateEvent(&ce);
                    } else {
                        if (RS_DIALOGFACTORY!=NULL) {
                            RS_DIALOGFACTORY->commandMessage(
                                "Expression Syntax Error");
                        }
                    }
                    e->accept();
                }
            }
        }

        // handle absolute polar coordinate input:
        if (!e->isAccepted()) {
            if (cmd.contains('<') && cmd.at(0)!='@') {
                if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
                        !currentActions[actionIndex]->isFinished()) {
                    int commaPos = cmd.find('<');
                    bool ok1, ok2;
                    double r = RS_Math::eval(cmd.left(commaPos), &ok1);
                    double a = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

                    if (ok1 && ok2) {
                        RS_Vector pos;
                        pos.setPolar(r,RS_Math::deg2rad(a));
                        RS_CoordinateEvent ce(pos);
                        currentActions[actionIndex]->coordinateEvent(&ce);
                    } else {
                        if (RS_DIALOGFACTORY!=NULL) {
                            RS_DIALOGFACTORY->commandMessage(
                                "Expression Syntax Error");
                        }
                    }
                    e->accept();
                }
            }
        }

        // handle relative polar coordinate input:
        if (!e->isAccepted()) {
            if (cmd.contains('<') && cmd.at(0)=='@') {
                if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
                        !currentActions[actionIndex]->isFinished()) {
                    int commaPos = cmd.find('<');
                    bool ok1, ok2;
                    double r = RS_Math::eval(cmd.mid(1, commaPos-1), &ok1);
                    double a = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

                    if (ok1 && ok2) {
                        RS_Vector pos;
                        pos.setPolar(r,RS_Math::deg2rad(a));
                        RS_CoordinateEvent ce(pos +
                                              graphicView->getRelativeZero());
                        currentActions[actionIndex]->coordinateEvent(&ce);
                    } else {
                        if (RS_DIALOGFACTORY!=NULL) {
                            RS_DIALOGFACTORY->commandMessage(
                                "Expression Syntax Error");
                        }
                    }
                    e->accept();
                }
            }
        }
    }

    // send command event directly to current action:
    if (!e->isAccepted()) {
        if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
                !currentActions[actionIndex]->isFinished()) {
            currentActions[actionIndex]->commandEvent(e);
            e->accept();
        } else {
            if (defaultAction!=NULL) {
                defaultAction->commandEvent(e);
                //e->accept();
            }
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
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {

        return currentActions[actionIndex];
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
        defaultAction = NULL;
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
    if (actionIndex>=0 && currentActions[actionIndex]!=NULL &&
            !currentActions[actionIndex]->isFinished()) {
        predecessor = currentActions[actionIndex];
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

    // Forget about the oldest action and make space for the new action:
    if (actionIndex==RS_MAXACTIONS-1) {
        // delete oldest action if necessary (usually never happens):
        if (currentActions[0]!=NULL) {
			currentActions[0]->finish();
            delete currentActions[0];
			currentActions[0] = NULL;
        }
        // Move up actionstack (optimize):
        for (int i=0; i<RS_MAXACTIONS-1; ++i) {
            currentActions[i] = currentActions[i+1];
        }
    } else if (actionIndex<RS_MAXACTIONS-1) {
        actionIndex++;
    }

    // Set current action:
    currentActions[actionIndex] = action;
    RS_DEBUG->print("RS_EventHandler::setCurrentAction: current action is: %s",
                    currentActions[actionIndex]->getName().latin1());

    // Initialisation of our new action:
    RS_DEBUG->print("RS_EventHandler::setCurrentAction: init current action");
    action->init();
    // ## new:
	if (action->isFinished()==false) {
    	RS_DEBUG->print("RS_EventHandler::setCurrentAction: show options");
    	currentActions[actionIndex]->showOptions();
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
    for (int c=0; c<RS_MAXACTIONS; ++c) {
        if (currentActions[c]!=NULL) {
            if (currentActions[c]->rtti()==RS2::ActionSelectSingle ||
                    currentActions[c]->rtti()==RS2::ActionSelectContour ||
                    currentActions[c]->rtti()==RS2::ActionSelectWindow ||
                    currentActions[c]->rtti()==RS2::ActionSelectIntersected ||
                    currentActions[c]->rtti()==RS2::ActionSelectLayer) {

                currentActions[c]->finish();
            }
        }
    }
}



/**
 * Kills all running actions. Called when a window is closed.
 */
void RS_EventHandler::killAllActions() {
    for (int c=0; c<RS_MAXACTIONS; ++c) {
        if (currentActions[c]!=NULL) {
            currentActions[c]->finish();
            delete currentActions[c];
            currentActions[c]=NULL;
        }
    }
    cleanUp();
}



/**
 * @return true if there is at least one action in the action stack.
 */
bool RS_EventHandler::hasAction() {
    if (actionIndex!=-1 || defaultAction!=NULL) {
        return true;
    } else {
        return false;
    }
}



/**
 * Garbage collector for actions.
 */
void RS_EventHandler::cleanUp() {
    RS_DEBUG->print("RS_EventHandler::cleanUp");
	
    int o=0;   // old index
    int n=0;   // new index
    int resume=0; // index of action to resume
    bool doResume=false; // do we need to resume an action
    actionIndex = -1;

    debugActions();
    do {
        // search first used action (o)
        while (currentActions[o]==NULL && o<RS_MAXACTIONS) {
            o++;
        }

        // delete action if it is finished
        if (o<RS_MAXACTIONS && currentActions[o]!=NULL &&
                currentActions[o]->isFinished()) {
            delete currentActions[o];
            currentActions[o] = NULL;

            doResume = true;
        }

        // move a running action up in the stack
        if (o<RS_MAXACTIONS && currentActions[o]!=NULL) {
            if (n!=o) {
                currentActions[n] = currentActions[o];
                resume = n;
                currentActions[o] = NULL;
            } else {
                if (o<RS_MAXACTIONS) {
                    o++;
                }
            }
            actionIndex = n;
            if (n<RS_MAXACTIONS-1) {
                n++;
            }
        }
    } while (o<RS_MAXACTIONS);

    debugActions();

    // Resume last used action:
    if (doResume) {
        if (currentActions[resume]!=NULL &&
                !currentActions[resume]->isFinished()) {

            currentActions[resume]->resume();
            currentActions[resume]->showOptions();
        } else {
            if (defaultAction!=NULL) {
                defaultAction->resume();
           		defaultAction->showOptions();
            }
        }
    }
    RS_DEBUG->print("RS_EventHandler::cleanUp: OK");
}



/**
 * Sets the snap mode for all currently active actions.
 */
void RS_EventHandler::setSnapMode(RS2::SnapMode sm) {
    for (int c=0; c<RS_MAXACTIONS; ++c) {
        if (currentActions[c]!=NULL) {
            currentActions[c]->setSnapMode(sm);
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
    for (int c=0; c<RS_MAXACTIONS; ++c) {
        if (currentActions[c]!=NULL) {
            currentActions[c]->setSnapRestriction(sr);
        }
    }

    if (defaultAction!=NULL) {
        defaultAction->setSnapRestriction(sr);
    }
}


void RS_EventHandler::debugActions() {
    RS_DEBUG->print("---");
    for (int c=0; c<RS_MAXACTIONS; ++c) {
        if (c==actionIndex) {
            RS_DEBUG->print("Current");
        }
        if (currentActions[c]!=NULL) {
            RS_DEBUG->print("Action %03d: %s [%s]",
                            c, currentActions[c]->getName().latin1(),
                            currentActions[c]->isFinished() ? "finished" : "active");
        } else {
            RS_DEBUG->print("Action %03d: NULL", c);
        }
    }
}

// EOF
