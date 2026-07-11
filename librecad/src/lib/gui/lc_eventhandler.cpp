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

#include "lc_eventhandler.h"

#include <QAction>
#include <QMouseEvent>

#include "lc_coordinates_parser.h"
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
LC_EventHandler::LC_EventHandler(RS_GraphicView* parent) : QObject(parent),
                                                           m_coordinatesParser{std::make_unique<LC_CoordinatesParser>(parent)},
                                                           m_graphicView{parent} {
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
void LC_EventHandler::back(Qt::KeyboardModifiers modifiers) {
    QMouseEvent e(QEvent::MouseButtonRelease, QPoint(0, 0), QPoint{0, 0}, Qt::RightButton, Qt::RightButton, modifiers);
    mouseReleaseEvent(&e);
    // if (modifiers == Qt::NoModifier) {
    //     uncheckQAction();
    // }
}

/**
 * Go enter pressed event for current action.
 */
void LC_EventHandler::enter() {
    QKeyEvent e(QEvent::KeyPress, Qt::Key_Enter, {});
    keyPressEvent(&e);
}

void LC_EventHandler::mousePressEvent(QMouseEvent* e) const {
    if (hasAction()) {
        // Issue #2608: hold a strong ref for the duration of the dispatch. The
        // action may switchToAction() and thereby reset/replace m_currentAction
        // (the sole owner), which would otherwise destroy it while still on the
        // stack (use-after-free). The methods below that follow the dispatch with
        // checkLastActionFinishedAndUncheckQAction() release this ref first so a
        // normally-finished action is still torn down at its original point.
        auto current = m_currentAction;
        current->mousePressEvent(e);
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
        auto current = m_currentAction; // keep alive in case it self-switches mid-dispatch
        current->mouseReleaseEvent(e);
        current.reset(); // release before the finished-check to preserve teardown order
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

void LC_EventHandler::mouseMoveEvent(QMouseEvent* e) {
    if (hasAction()) {
        auto current = m_currentAction; // keep alive in case it self-switches mid-dispatch
        current->mouseMoveEvent(e);
        current.reset(); // release before the finished-check to preserve teardown order
        checkLastActionFinishedAndUncheckQAction();
        e->accept();
    }
    else if (m_defaultAction) {
        m_defaultAction->mouseMoveEvent(e);
    }
}

void LC_EventHandler::mouseLeaveEvent() const {
    if (hasAction()) {
        m_currentAction->suspend();
    }
    else {
        if (m_defaultAction) {
            m_defaultAction->suspend();
        }
    }
}

void LC_EventHandler::mouseEnterEvent() const {
    if (hasAction()) {
        m_currentAction->resume();
    }
    else {
        if (m_defaultAction) {
            m_defaultAction->resume();
        }
    }
}

void LC_EventHandler::keyPressEvent(QKeyEvent* e) {
    if (hasAction()) {
        auto current = m_currentAction; // keep alive in case it self-switches mid-dispatch
        current->keyPressEvent(e);
        current.reset(); // release before the finished-check to preserve teardown order
        checkLastActionFinishedAndUncheckQAction();
    }
    else {
        if (m_defaultAction) {
            m_defaultAction->keyPressEvent(e);
        }
        else {
            e->ignore();
        }
    }
}

void LC_EventHandler::keyReleaseEvent(QKeyEvent* e) {
    if (hasAction()) {
        auto current = m_currentAction; // keep alive in case it self-switches mid-dispatch
        current->keyReleaseEvent(e);
        current.reset(); // release before the finished-check to preserve teardown order
        checkLastActionFinishedAndUncheckQAction();
    }
    else {
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
                // keep the action alive across the dispatch: a command (e.g.
                // "polyline") may switchToAction and reset m_currentAction.
                auto current = m_currentAction;
                bool commandContainsCoordinate = false;
                const QString command = e->getCommand();
                auto coordinateEvent = m_coordinatesParser->parseCoordinate(command, commandContainsCoordinate);
                if (commandContainsCoordinate) {
                    if (coordinateEvent.isValid()) {
                        current->coordinateEvent(&coordinateEvent);
                    }
                    else {
                        RS_DIALOGFACTORY->commandMessage("Expression Syntax Error"); // fixme - sand - remove static
                    }
                    e->accept();
                }
                else {
                    // send command event directly to current action:
                    current->commandEvent(e);
                    current.reset(); // release before the finished-check to preserve teardown order
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

bool LC_EventHandler::checkLastActionFinishedAndUncheckQAction() {
    // Issue #2608: the action that was just dispatched may have switched away
    // (e.g. via switchToAction) and reset m_currentAction to null. Nothing to
    // finish then.
    if (m_currentAction == nullptr) {
        // fixme - sand - merge - my code
        /*// action may be null due to switch to default action.
        return true;
        */
        return false;
    }
    int lastActionStatus = m_currentAction->getStatus();
    bool result = false;
    if (lastActionStatus < 0 || m_currentAction->isFinished()) {
        if (m_QAction != nullptr) {
            m_QAction->setChecked(false);
            m_QAction = nullptr;
        }
        const auto predecessor = m_currentAction->getPredecessor();
        if (predecessor != nullptr) {
            const RS2::ActionType prevActionRtti = predecessor->rtti();
            m_currentAction = predecessor;
            m_graphicView->notifyCurrentActionChanged(prevActionRtti);
            m_graphicView->onSwitchToDefaultAction(m_currentAction == m_defaultAction, m_currentAction->rtti(), prevActionRtti);
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
    const auto prevRtti = m_currentAction != nullptr ? m_currentAction->rtti() : RS2::ActionNone;
    m_currentAction.reset();
    if (m_QAction != nullptr) {
        m_QAction->setChecked(false);
        m_QAction = nullptr;
    }

    if (m_defaultAction != nullptr) {
        const RS2::ActionType defaultActionRtti = m_defaultAction->rtti();
        m_graphicView->notifyCurrentActionChanged(defaultActionRtti);
        m_graphicView->onSwitchToDefaultAction(true, defaultActionRtti, prevRtti);
        resumeAction(m_defaultAction);
    }
    else {
        m_graphicView->notifyCurrentActionChanged(RS2::ActionNone);
    }
}

/**
 * Sets the current action.
 */
bool LC_EventHandler::setCurrentAction(std::shared_ptr<RS_ActionInterface> action) {
    if (action == nullptr) {
        return false;
    }
    // Do not initialize action if it's already the last one.
    // This is attempt to fix crashes of dialogs (like properties) which are called from actions
    if (isValid(action.get())) {
        return false;
    }

    const bool hasNonDefaultAction = hasAction();
    // Predecessor of the new action or NULL:
    const auto predecessor = hasNonDefaultAction ? m_currentAction : m_defaultAction;
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
        if (action->isSupportsPredecessorAction()) {
            // we'll not change current action for one-shoot call
            if (hasNonDefaultAction) {
                actionType = m_currentAction->rtti();
                m_graphicView->notifyCurrentActionChanged(actionType);
                const auto defaultActionRtti = (m_defaultAction != nullptr) ? m_defaultAction->rtti() : RS2::ActionNone;
                m_graphicView->onSwitchToDefaultAction(m_currentAction == m_defaultAction, defaultActionRtti, actionType);
                resumeAction(m_currentAction);
            }
            else {
                switchToDefaultAction();
            }
        }
        else {
            // one-shoot finished action, return to default
            switchToDefaultAction();
        }
    }
    else {
        // this is multi-state action, so switch to it
        if (hasNonDefaultAction && action->isSupportsPredecessorAction()) {
            action->setPredecessor(predecessor);
        }
        // Set current action:
        m_currentAction = action;
        passedActionIsNotFinished = true;
        const auto actionRtti = m_currentAction != nullptr ? m_currentAction->rtti() : RS2::ActionNone;
        const auto defaultActionRtti = (m_defaultAction != nullptr) ? m_defaultAction->rtti() : RS2::ActionNone;
        m_graphicView->onSwitchToDefaultAction(m_currentAction == m_defaultAction,defaultActionRtti, actionRtti);
        resumeAction(action);
    }
    return passedActionIsNotFinished;
}

void LC_EventHandler::resumeAction(const std::shared_ptr<RS_ActionInterface>& action) {
    action->resume();
    action->showOptions();
}

void LC_EventHandler::notifyLastActionFinished() {
    // Issue #2608: m_currentAction may be null after an action switched away.
    if (m_currentAction == nullptr) {
        return;
    }
    int lastActionStatus = m_currentAction->getStatus();
    if (lastActionStatus < 0 || m_currentAction->isFinished()) {
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
RS_ActionInterface* LC_EventHandler::getCurrentAction() const {
    if (hasAction()) {
        return m_currentAction.get();
    }
    return m_defaultAction.get();
}

/**
 * @return The current default action.
 */
RS_ActionInterface* LC_EventHandler::getDefaultAction() const {
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
bool LC_EventHandler::killAllActions() {
    bool mayTerminate = true;
    RS2::ActionType prevActionRtti = RS2::ActionNone;
    if (m_currentAction != nullptr) {
        prevActionRtti = m_currentAction->rtti();
        if (!m_currentAction->isFinished()) {
            mayTerminate = m_currentAction->mayBeTerminatedExternally();
        }
    }
    if (mayTerminate) {
        if (m_currentAction != nullptr) {
            if (isActive(m_currentAction)) {
                m_currentAction->finish();
                m_currentAction.reset();
            }
        }

        const auto defaultActionRtti = m_defaultAction->rtti();

        if (m_QAction != nullptr) {
            m_QAction->setChecked(false);
            m_QAction = nullptr;
            m_graphicView->notifyCurrentActionChanged(defaultActionRtti);
        }

        if (!m_defaultAction->isFinished()) {
            m_defaultAction->finish();
        }
        m_defaultAction->init(0);
        m_graphicView->onSwitchToDefaultAction(true, defaultActionRtti, prevActionRtti);
    }
    return mayTerminate;
}

/**
 * @return true if the action is within currentActions
 */
bool LC_EventHandler::isValid(const RS_ActionInterface* action) const {
    return m_currentAction != nullptr && m_currentAction.get() == action;
}

/**
 * @return true if there is at least one action in the action stack.
 */
bool LC_EventHandler::hasAction() const {
    return m_currentAction != nullptr;
}

/**
 * Sets the snap mode for all currently active actions.
 */
void LC_EventHandler::setSnapMode(const RS_SnapMode sm) const {
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
void LC_EventHandler::setSnapRestriction(const RS2::SnapRestriction sr) const {
    if (isActive(m_currentAction)) {
        m_currentAction->setSnapRestriction(sr);
    }

    if (m_defaultAction) {
        m_defaultAction->setSnapRestriction(sr);
    }
}

QAction* LC_EventHandler::getQAction() const {
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

void LC_EventHandler::uncheckQAction() {
    if (m_QAction != nullptr) {
        m_QAction->setChecked(false);
        m_QAction = nullptr;
    }
}
