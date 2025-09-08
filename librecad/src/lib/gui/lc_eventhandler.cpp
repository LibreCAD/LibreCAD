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



#include <QMouseEvent>

#include "lc_coordinates_parser.h"
#include "lc_eventhandler.h"
#include "rs_actioninterface.h"
#include "rs_commandevent.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphicview.h"

namespace {
    bool isActive(const std::shared_ptr<RS_ActionInterface>& action) {
        return action != nullptr && !action->isFinished();
    }
}

/**
 * Constructor.
 */
LC_EventHandler::LC_EventHandler(RS_GraphicView *parent):QObject(parent), m_coordinatesParser{std::make_unique<LC_CoordinatesParser>(parent)},
    m_graphicView{parent}{
}
/**
 * Destructor.
 */
LC_EventHandler::~LC_EventHandler() {
    m_defaultAction.reset();
    m_currentAction.reset();
}

/**
 * Go back in current action.
 */
void LC_EventHandler::back() {
    QMouseEvent e(QEvent::MouseButtonRelease, QPoint(0,0), QPoint{0, 0},
                  Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    mouseReleaseEvent(&e);
    uncheckQAction();
}

/**
 * Go enter pressed event for current action.
 */
void LC_EventHandler::enter() {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, {});
    keyPressEvent(&e);
}

void LC_EventHandler::mousePressEvent(QMouseEvent* e) {
    if (hasAction()) {
        m_currentAction->mousePressEvent(e);
        e->accept();
    }
    else {
        if (m_defaultAction) {
            m_defaultAction->mousePressEvent(e);
            e->accept();
        }
        else {
            e->ignore();
        }
    }
}

void LC_EventHandler::mouseReleaseEvent(QMouseEvent* e) {
    if (hasAction()) {
        m_currentAction->mouseReleaseEvent(e);
        // action may be completed by click. Check this and if it is so, uncheck the action
        checkLastActionFinishedAndUncheckQAction();
        e->accept();
    }
    else {
        if (m_defaultAction) {
            m_defaultAction->mouseReleaseEvent(e);
        }
        else {
            e->ignore();
        }
    }
}

void LC_EventHandler::mouseMoveEvent(QMouseEvent* e){
    if(hasAction()) {
        m_currentAction->mouseMoveEvent(e);
        checkLastActionFinishedAndUncheckQAction();
        e->accept();
    }
    else if (m_defaultAction) {
        m_defaultAction->mouseMoveEvent(e);
    }
}

void LC_EventHandler::mouseLeaveEvent() {
    if(hasAction()){
        m_currentAction->suspend();
    } else {
        if (m_defaultAction) {
            m_defaultAction->suspend();
        }
    }
}

void LC_EventHandler::mouseEnterEvent() {
    if(hasAction()){
        m_currentAction->resume();
    } else {
        if (m_defaultAction) {
            m_defaultAction->resume();
        }
    }
}

void LC_EventHandler::keyPressEvent(QKeyEvent* e) {
    if(hasAction()){
        m_currentAction->keyPressEvent(e);
        checkLastActionFinishedAndUncheckQAction();
    } else {
        if (m_defaultAction) {
            m_defaultAction->keyPressEvent(e);
        }
        else {
            e->ignore();
        }
    }
}

void LC_EventHandler::keyReleaseEvent(QKeyEvent* e) {
    if(hasAction()){
        m_currentAction->keyReleaseEvent(e);
        checkLastActionFinishedAndUncheckQAction();
    } else {
        if (m_defaultAction) {
            m_defaultAction->keyReleaseEvent(e);
        }
        else {
            e->ignore();
        }
    }
}

/**
 * Handles command line events.
 */
void LC_EventHandler::commandEvent(RS_CommandEvent* e) {
    if (m_coordinateInputEnabled) {
        if (!e->isAccepted()) {
            if (hasAction()) {
                bool commandContainsCoordinate = false;
                QString command = e->getCommand();
                auto coordinateEvent = m_coordinatesParser->parseCoordinate(command, commandContainsCoordinate);
                if (commandContainsCoordinate) {
                    if (coordinateEvent.isValid()) {
                        m_currentAction->coordinateEvent(&coordinateEvent);
                    }
                    else {
                        RS_DIALOGFACTORY->commandMessage("Expression Syntax Error"); // fixme - sand - remove static
                    }
                    e->accept();
                }
                else {
                    // send command event directly to current action:
                    m_currentAction->commandEvent(e);
                    if (e->isAccepted()) {
                        checkLastActionFinishedAndUncheckQAction();
                    }
                }
            }
            else {
                //send the command to default action
                if (m_defaultAction) {
                    m_defaultAction->commandEvent(e);
                }
            }
            // do not accept command here. Actions themselves should be responsible to accept commands
            // e->accept();
        }
    }
}


bool  LC_EventHandler::checkLastActionFinishedAndUncheckQAction() {
    int lastActionStatus = m_currentAction->getStatus();
    bool result = false;
    if (lastActionStatus < 0 || m_currentAction->isFinished()){
        if (m_QAction != nullptr) {
            m_QAction->setChecked(false);
            m_QAction = nullptr;
        }
        auto predecessor = m_currentAction->getPredecessor();
        if (predecessor != nullptr) {
            RS2::ActionType actionType = predecessor->rtti();
            m_currentAction = predecessor;
            m_graphicView->notifyCurrentActionChanged(actionType);
            resumeAction(m_currentAction);
        }
        else {
            switchToDefaultAction();
        }
        result = true;
    }
    return result;
}

void LC_EventHandler::switchToDefaultAction() {
    m_currentAction.reset();
    if (m_QAction != nullptr){
        m_QAction->setChecked(false);
        m_QAction = nullptr;
    }
    m_graphicView->notifyCurrentActionChanged(RS2::ActionNone);
    if (m_defaultAction != nullptr) {
        resumeAction(m_defaultAction);
    }
}

/**
 * Sets the current action.
 */
bool LC_EventHandler::setCurrentAction(std::shared_ptr<RS_ActionInterface> action) {
    if (action==nullptr) {
        return false;
    }
    // Do not initialize action if it's already the last one.
    // This is attempt to fix crashes of dialogs (like properties) which are called from actions
    if (isValid(action.get())) {
        return false;
    }

    bool hasNonDefaultAction = hasAction();
    // Predecessor of the new action or NULL:
    auto predecessor = hasNonDefaultAction ? m_currentAction : m_defaultAction;
    // Suspend current action:
    if (predecessor != nullptr) {
        predecessor->suspend();
        predecessor->hideOptions();
    }
    // Initialisation of our new action:
    bool passedActionIsNotFinished = false;

    uncheckQAction();

    RS2::ActionType actionType = action->rtti();
    m_graphicView->notifyCurrentActionChanged(actionType);

    action->init(RS_ActionInterface::InitialActionStatus);

    if (action->isFinished()) {
        // For some actions: action->init() may call finish() within init()
        // If so, the q_action shouldn't be checked
        if (action->isSupportsPredecessorAction()) { // we'll not change current action for one-shoot call
            if (hasNonDefaultAction) {
                actionType = m_currentAction->rtti();
                m_graphicView->notifyCurrentActionChanged(actionType);
                resumeAction(m_currentAction);
            }
            else {
                switchToDefaultAction();
            }
        }
        else { // one-shoot finished action, return to default
            switchToDefaultAction();
        }
    }
    else{ // this is multi-state action, so switch to it
        if (hasNonDefaultAction && action->isSupportsPredecessorAction()) {
            action->setPredecessor(predecessor);
        }
        // Set current action:
        m_currentAction = action;
        passedActionIsNotFinished = true;
        resumeAction(action);
    }
    return passedActionIsNotFinished;
}

void LC_EventHandler::resumeAction(const std::shared_ptr<RS_ActionInterface>& action) {
    action->resume();
    action->showOptions();
}

void LC_EventHandler::notifyLastActionFinished() {
    // fixme - sand check that action is not null!!!
    int lastActionStatus = m_currentAction->getStatus();
    if (lastActionStatus < 0 || m_currentAction->isFinished()){
        uncheckQAction();
    }
}

/**
 * Enables coordinate input in the command line.
 */
void LC_EventHandler::enableCoordinateInput() {
    m_coordinateInputEnabled = true;
}

/**
 * Enables coordinate input in the command line.
 */
void LC_EventHandler::disableCoordinateInput() {
    m_coordinateInputEnabled = false;
}

/**
 * @return Current action.
 */
RS_ActionInterface* LC_EventHandler::getCurrentAction(){
    if(hasAction()){
        return m_currentAction.get();
    } else {
        return m_defaultAction.get();
    }
}

/**
 * @return The current default action.
 */
RS_ActionInterface* LC_EventHandler::getDefaultAction() const{
    return m_defaultAction.get();
}

/**
 * Sets the default action.
 */
void LC_EventHandler::setDefaultAction(RS_ActionInterface* action) {
    if (m_defaultAction) {
        m_defaultAction->finish();
    }
    m_defaultAction.reset(action);
}

/**
 * Kills all running actions. Called when a window is closed.
 */
void LC_EventHandler::killAllActions(){
    bool mayTerminate = true;
    if (m_currentAction != nullptr) {
        if (!m_currentAction->isFinished()) {
            mayTerminate = m_currentAction->mayBeTerminatedExternally();
        }
    }
    if (mayTerminate) {
        if (isActive(m_currentAction)){
            m_currentAction->finish();
            m_currentAction.reset();
        }

        if (m_QAction)  {
            m_QAction->setChecked(false);
            m_QAction = nullptr;
            m_graphicView->notifyCurrentActionChanged(RS2::ActionNone);
        }

        if (!m_defaultAction->isFinished()) {
            m_defaultAction->finish();
        }
        m_defaultAction->init(0);
    }
}

/**
 * @return true if the action is within currentActions
 */
bool LC_EventHandler::isValid(RS_ActionInterface* action) const{
    return m_currentAction != nullptr && m_currentAction.get() == action;
}

/**
 * @return true if there is at least one action in the action stack.
 */
bool LC_EventHandler::hasAction(){
    return m_currentAction != nullptr;
}

/**
 * Sets the snap mode for all currently active actions.
 */
void LC_EventHandler::setSnapMode(RS_SnapMode sm) {
    if (isActive(m_currentAction)) {
        m_currentAction->setSnapMode(sm);
    }

	if (m_defaultAction) {
        m_defaultAction->setSnapMode(sm);
    }
}

/**
 * Sets the snap restriction for all currently active actions.
 */
void LC_EventHandler::setSnapRestriction(RS2::SnapRestriction sr) {
    if (isActive(m_currentAction)) {
        m_currentAction->setSnapRestriction(sr);
    }

    if (m_defaultAction) {
        m_defaultAction->setSnapRestriction(sr);
    }
}

QAction* LC_EventHandler::getQAction(){
  return m_QAction;
}

void LC_EventHandler::setQAction(QAction* action) {
    if (action->isCheckable()) {
        if (!action->isChecked()) {
            action->setChecked(true);
        }
    }
    m_QAction = action;
}

void LC_EventHandler::uncheckQAction(){
    if (m_QAction != nullptr) {
        m_QAction->setChecked(false);
        m_QAction = nullptr;
    }
}
