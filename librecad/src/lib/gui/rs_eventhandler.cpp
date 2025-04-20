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
#include <QMouseEvent>

#include "lc_coordinates_parser.h"
#include "rs_actioninterface.h"
#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphicview.h"

namespace {
    bool isActive(const std::shared_ptr<RS_ActionInterface>& action) {
        return action != nullptr && !action->isFinished();
    }

    bool isInactive(const std::shared_ptr<RS_ActionInterface>& action) {
        return ! isActive(action);
    }
}

/**
 * Constructor.
 */
RS_EventHandler::RS_EventHandler(RS_GraphicView *parent):QObject(parent), m_coordinatesParser{std::make_unique<LC_CoordinatesParser>(parent)},
    m_graphicView{parent}{
}
/**
 * Destructor.
 */
RS_EventHandler::~RS_EventHandler() {
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler");
    m_defaultAction.reset();

    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..");
    m_currentActions.clear();
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..: OK");
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: OK");
}

void RS_EventHandler::uncheckQAction(){
    if (!hasAction()){
        if (m_QAction != nullptr) {
            m_QAction->setChecked(false);
            m_QAction = nullptr;
        }
        m_graphicView->notifyNoActiveAction();
    }
}

/**
 * Go back in current action.
 */
void RS_EventHandler::back() {
    QMouseEvent e(QEvent::MouseButtonRelease, QPoint(0,0), QPoint{0, 0},
                  Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    mouseReleaseEvent(&e);
    uncheckQAction();
}

/**
 * Go enter pressed event for current action.
 */
void RS_EventHandler::enter() {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, {});
    keyPressEvent(&e);
}

/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mousePressEvent(QMouseEvent* e) {
    if(hasAction()){
        m_currentActions.last()->mousePressEvent(e);
        e->accept();
    } else {
        if (m_defaultAction) {
            m_defaultAction->mousePressEvent(e);
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
        std::shared_ptr<RS_ActionInterface> &lastAction = m_currentActions.last();
        LC_ERR<< "call action "<< lastAction->getName();

        lastAction->mouseReleaseEvent(e);

        // action may be completed by click. Check this and if it so, uncheck the action
        checkLastActionCompletedAndUncheckQAction(lastAction);
        // Clean up actions - one might be finished now
        cleanUp();
        e->accept();
    } else {
        if (m_defaultAction) {
            m_defaultAction->mouseReleaseEvent(e);
        } else {
            e->ignore();
        }
    }
}

void RS_EventHandler::checkLastActionCompletedAndUncheckQAction(const std::shared_ptr<RS_ActionInterface> &lastAction) {
    int lastActionStatus = lastAction->getStatus();
    if (lastActionStatus < 0){
        uncheckQAction();
    }
}

/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseMoveEvent(QMouseEvent* e){
    if(hasAction()) {
        std::shared_ptr<RS_ActionInterface> &lastAction = m_currentActions.last();
        lastAction->mouseMoveEvent(e);
        checkLastActionCompletedAndUncheckQAction(lastAction);
        cleanUp();
        e->accept();
    }
    else if (m_defaultAction) {
        m_defaultAction->mouseMoveEvent(e);
    }
}

/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseLeaveEvent() {
    if(hasAction()){
        m_currentActions.last()->suspend();
    } else {
        if (m_defaultAction) {
            m_defaultAction->suspend();
        }
        //RS_DEBUG->print("currently no action defined");
    }
}

/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseEnterEvent() {
    if(hasAction()){
        cleanUp();
        debugActions();
//        LC_ERR<<__func__<<"(): resume: "<<currentActions.last()->getName();
        m_currentActions.last()->resume();
    } else {
        if (m_defaultAction) {
            m_defaultAction->resume();
        }
    }
}

/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::keyPressEvent(QKeyEvent* e) {
    if(hasAction()){
        std::shared_ptr<RS_ActionInterface> &lastAction = m_currentActions.last();
        lastAction->keyPressEvent(e);
        checkLastActionCompletedAndUncheckQAction(lastAction);
    } else {
        if (m_defaultAction) {
            m_defaultAction->keyPressEvent(e);
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
        m_currentActions.last()->keyReleaseEvent(e);
    } else {
        if (m_defaultAction) {
            m_defaultAction->keyReleaseEvent(e);
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

    if (m_coordinateInputEnabled) {
        if (!e->isAccepted()) {
            if(hasAction()) {
                bool commandContainsCoordinate = false;
                QString command = e->getCommand();
                auto coordinateEvent = m_coordinatesParser->parseCoordinate(command, commandContainsCoordinate);
                if (commandContainsCoordinate) {
                    if (coordinateEvent.isValid()) {
                        m_currentActions.last()->coordinateEvent(&coordinateEvent);
                    }
                    else {
                        RS_DIALOGFACTORY->commandMessage("Expression Syntax Error"); // fixme - sand - remove static
                    }
                    e->accept();
                }
                else {
                    // send command event directly to current action:
                    std::shared_ptr<RS_ActionInterface> &lastAction = m_currentActions.last();
                    lastAction->commandEvent(e);
                    if (e->isAccepted()) {
                        checkLastActionCompletedAndUncheckQAction(lastAction);
                        cleanUp();
                    }
                }
            }else{
                //send the command to default action
                if (m_defaultAction) {
                    m_defaultAction->commandEvent(e);
                }
            }
            // do not accept command here. Actions themselves should be responsible to accept commands
            // e->accept();
        }
    }

    RS_DEBUG->print("RS_EventHandler::commandEvent: OK");
}
/**
 * Enables coordinate input in the command line.
 */
void RS_EventHandler::enableCoordinateInput() {
    m_coordinateInputEnabled = true;
}

/**
 * Enables coordinate input in the command line.
 */
void RS_EventHandler::disableCoordinateInput() {
    m_coordinateInputEnabled = false;
}

/**
 * @return Current action.
 */
RS_ActionInterface* RS_EventHandler::getCurrentAction(){
    if(hasAction()){
        return m_currentActions.last().get();
    } else {
        return m_defaultAction.get();
    }
}

/**
 * @return The current default action.
 */
RS_ActionInterface* RS_EventHandler::getDefaultAction() const{
    return m_defaultAction.get();
}

/**
 * Sets the default action.
 */
void RS_EventHandler::setDefaultAction(RS_ActionInterface* action) {
    if (m_defaultAction) {
        m_defaultAction->finish();
    }

    m_defaultAction.reset(action);
}

/**
 * Sets the current action.
 */
bool RS_EventHandler::setCurrentAction(std::shared_ptr<RS_ActionInterface> action) {
    RS_DEBUG->print("RS_EventHandler::setCurrentAction");
    if (action==nullptr) {
        return false;
    }
    // Do not initialize action if it's already the last one.
    // This is attempt to fix crashes of dialogs (like properties) which are called from actions
    // todo - check again, either remove or uncomment
//    if (hasAction() && currentActions.last().get() == action){
//        return;
//    }

    LC_LOG<<"RS_EventHandler::setCurrentAction " << action->getName();
    // Predecessor of the new action or NULL:
    auto& predecessor = hasAction() ? m_currentActions.last() : m_defaultAction;
    // Suspend current action:
    predecessor->suspend();
    predecessor->hideOptions();

    // Set current action:
    m_currentActions.push_back(action);
    //    RS_DEBUG->print("RS_EventHandler::setCurrentAction: current action is: %s -> %s",
    //                    predecessor->getName().toLatin1().data(),
    //                    currentActions.last()->getName().toLatin1().data());

    // Initialisation of our new action:
    RS_DEBUG->print("RS_EventHandler::setCurrentAction: init current action");
    action->init(0);
    // ## new:
    bool passedActionIsNotFinished = false;
    if (!action->isFinished()) {
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: show options");
        action->showOptions();
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: set predecessor");
        action->setPredecessor(predecessor.get());
        passedActionIsNotFinished = true;
    }

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: cleaning up..");
    cleanUp();

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: debugging actions");
    debugActions();
    RS_DEBUG->print("RS_GraphicView::setCurrentAction: OK");
    // For some actions: action->init() may call finish() within init()
    // If so, the q_action shouldn't be checked
    if (m_QAction){
        bool hasActionToCheck = hasAction();
        m_QAction->setChecked(hasActionToCheck);
        if (!hasActionToCheck) {
            m_graphicView->notifyNoActiveAction();
        }
    }
    return passedActionIsNotFinished;
}

/**
 * Kills all running selection actions. Called when a selection action
 * is launched to reduce confusion.
 */
void RS_EventHandler::killSelectActions() {

    for (auto it=m_currentActions.begin();it != m_currentActions.end();){
        RS2::ActionType rtti = (*it)->rtti();
        if (rtti == RS2::ActionSelectSingle ||
            rtti == RS2::ActionSelectContour ||
            rtti == RS2::ActionSelectWindow ||
            rtti == RS2::ActionSelectIntersected ||
            rtti == RS2::ActionSelectLayer) {
            if (isActive(*it)) {
                (*it)->finish();
            }
            it= m_currentActions.erase(it);
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

    if (m_QAction)    {
        m_QAction->setChecked(false);
        m_QAction = nullptr;
        m_graphicView->notifyNoActiveAction();
    }

    for(auto& p: m_currentActions){
        if (isActive(p)){
            p->finish();
        }
    }
    m_currentActions.clear();

    if (!m_defaultAction->isFinished()) {
        m_defaultAction->finish();
    }

    RS_DEBUG->print(__FILE__ ": %s: line %d: begin\n", __func__, __LINE__);
    m_defaultAction->init(0);
}

/**
 * @return true if the action is within currentActions
 */
bool RS_EventHandler::isValid(RS_ActionInterface* action) const{
    return action != nullptr && std::any_of(m_currentActions.cbegin(), m_currentActions.cend(),
                       [action](const std::shared_ptr<RS_ActionInterface>& entry){
        return entry.get() == action;});
}

/**
 * @return true if there is at least one action in the action stack.
 */
bool RS_EventHandler::hasAction(){
    auto it = std::remove_if(m_currentActions.begin(), m_currentActions.end(), isInactive);
    m_currentActions.erase(it, m_currentActions.end());
    return !m_currentActions.empty();
}

/**
 * Garbage collector for actions.
 */
void RS_EventHandler::cleanUp() {
    RS_DEBUG->print("RS_EventHandler::cleanUp");

    if (hasAction()) {
        m_currentActions.last()->resume();
        m_currentActions.last()->showOptions();
    } else {
        if (m_defaultAction) {
            m_defaultAction->resume();
            m_defaultAction->showOptions();
        }
    }
    RS_DEBUG->print("RS_EventHandler::cleanUp: OK");
}

/**
 * Sets the snap mode for all currently active actions.
 */
void RS_EventHandler::setSnapMode(RS_SnapMode sm) {
    for(auto& a: m_currentActions){
        if(isActive(a)) {
            a->setSnapMode(sm);
        }
    }

	if (m_defaultAction) {
        m_defaultAction->setSnapMode(sm);
    }
}

/**
 * Sets the snap restriction for all currently active actions.
 */
void RS_EventHandler::setSnapRestriction(RS2::SnapRestriction sr) {
    for(auto& a: m_currentActions){
        if(isActive(a)) {
            a->setSnapRestriction(sr);
        }
    }

    if (m_defaultAction) {
        m_defaultAction->setSnapRestriction(sr);
    }
}

void RS_EventHandler::debugActions() const{
    //        std::cout<<"action queue size=:"<<currentActions.size()<<std::endl;
    RS_DEBUG->print("---");
    for(int i=0;i<m_currentActions.size();++i){
        if (i == m_currentActions.size() - 1 ) {
            RS_DEBUG->print("Current");
        }
        RS_DEBUG->print("Action %03d: %s [%s]",
                        i, m_currentActions.at(i)->getName().toLatin1().data(),
                        m_currentActions.at(i)->isFinished() ? "finished" : "active");
    }
}

QAction* RS_EventHandler::getQAction(){
  return m_QAction;
}

void RS_EventHandler::setQAction(QAction *action) {
//    LC_ERR << __func__ << "()";
    debugActions();
    if (m_QAction != nullptr && m_QAction != action) {
        killAllActions();
    }
    m_QAction = action;
}

bool RS_EventHandler::inSelectionMode() {
    switch (getCurrentAction()->rtti()) {
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
