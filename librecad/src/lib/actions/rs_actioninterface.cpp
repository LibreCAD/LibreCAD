/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "rs_actioninterface.h"

#include <QMouseEvent>

#include "lc_action_options_editor.h"
#include "lc_action_options_editor_typed.h"
#include "lc_action_options_widget.h"
#include "lc_actioncontext.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_visual_snap_manager.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_selection.h"
#include "rs_settings.h"

/**
 * Constructor.
 *
 * Sets the entity container on which the action class inherited
 * from this interface operates.
 *
 * @param actionName Action name. This can be used internally for
 *            action settings group name and for debugging.
 * @param actionContext
 * @param actionType
 */
RS_ActionInterface::RS_ActionInterface(const QString& actionName, LC_ActionContext* actionContext, const RS2::ActionType actionType)
    : RS_Snapper(actionContext), LC_ActionOptionsBase(actionName, ""), m_graphic{actionContext->getDocument()->getGraphic()},
      m_actionType{actionType} {
    updateSnapAngleStep();
}

RS_ActionInterface::~RS_ActionInterface() {
}

/**
 * Must be implemented to return the ID of this action.
*
* @todo no default implementation
 */
RS2::ActionType RS_ActionInterface::rtti() const {
    return m_actionType;
}

void RS_ActionInterface::select(const QList<RS_Entity*>& entitiesList) const {
    RS_Selection::selectEntitiesList(m_document, m_viewport, entitiesList, true);
}

void RS_ActionInterface::unselect(const QList<RS_Entity*>& entitiesList) const {
    RS_Selection::selectEntitiesList(m_document, m_viewport, entitiesList, false);
}

void RS_ActionInterface::unselectAll() const {
    RS_Selection::unselectAllInDocument(m_document, m_viewport);
}

void RS_ActionInterface::unselect(RS_Entity* e) const {
    m_document->unselect(e);
}

void RS_ActionInterface::clearVisualSnap() const {
    if (!isSupportsPredecessorAction()) {
        RS_Snapper::clearVisualSnap();
    }
}

void RS_ActionInterface::select(RS_Entity* e) const {
    const RS_Selection sel(m_document, m_viewport);
    sel.selectSingle(e);
}

/**
 * @return name of this action
 */
QString RS_ActionInterface::getName() {
    return LC_ActionOptionsBase::m_optionsSettingsGroupName;
}

bool RS_ActionInterface::mayInitWithContextEntity(const int status) {
    return status == InitialActionStatus;
}

/**
 * Called to initiate an action. This funtcion is often
 * overwritten by the implementing action.
 *
 * @param status The status on which to initiate this action.
 * default is 0 to begin the action.
 */
void RS_ActionInterface::init(const int status) {
    setStatus(status);
    if (status >= InitialActionStatus) {
        RS_Snapper::init();
        updateActionPrompt();
        updateMouseCursor();
        if (mayInitWithContextEntity(status)) {
            doInitialInit();
            const auto contextEntity = m_actionContext->getContextMenuActionContextEntity();
            if (contextEntity != nullptr) {
                doInitWithContextEntity(contextEntity, m_actionContext->getContextMenuActionClickPosition());
                m_actionContext->clearContextMenuActionContext();
            }
        }
    }
    else {
        //delete snapper when finished, bug#3416878
        deleteSnapper();
    }
}

void RS_ActionInterface::doInitWithContextEntity([[maybe_unused]] RS_Entity* contextEntity, [[maybe_unused]] const RS_Vector& clickPos) {
}

void RS_ActionInterface::doInitialInit() {
}

/**
 * Called when the mouse moves and this is the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation keeps track of the mouse position.
 */
void RS_ActionInterface::mouseMoveEvent(QMouseEvent*) {
}

/**
 * Called when the left mouse button is pressed and this is the
 * current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
// todo - add default implementation?
void RS_ActionInterface::mousePressEvent(QMouseEvent* e) {
    const Qt::MouseButton button = e->button();
    if (button == Qt::LeftButton) {
        if (m_graphicView->isInRelativePointInput()) {
            return;
        }
        onMouseLeftButtonPress(m_status, e);
    }
    else if (button == Qt::RightButton) {
        if (m_graphicView->isInRelativePointInput()) {
            return;
        }
        onMouseRightButtonPress(m_status, e);
    }
}

/**
 * Called when the left mouse button is released and this is
 * the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
void RS_ActionInterface::mouseReleaseEvent(QMouseEvent* e) {
    const Qt::MouseButton button = e->button();
    if (button == Qt::LeftButton) {
        if (m_graphicView->isInRelativePointInput()) {
            m_graphicView->hideRelativeInputWidget();
            return;
        }
        onMouseLeftButtonRelease(m_status, e);
    }
    else if (button == Qt::RightButton) {
        if (m_graphicView->isInRelativePointInput()) {
            m_graphicView->hideRelativeInputWidget();
            return;
        }
        if (hasVisualSnap() && isClearVisualSnapByRMB()) {
            stopVisualSnap();
            onVisualSnapSolutionRefresh();
            e->accept();
            return;
        }
        onMouseRightButtonRelease(m_status, e);
    }
    else if (button == Qt::MiddleButton) {
        if (hasVisualSnap()) {
            const bool control = isControl(e);
            if (control) {
                stopVisualSnap();
                onVisualSnapSolutionRefresh();
                e->accept();
            }
        }
    }
}

void RS_ActionInterface::onMouseLeftButtonRelease([[maybe_unused]] int status, [[maybe_unused]] QMouseEvent* e) {
}

void RS_ActionInterface::onMouseRightButtonRelease([[maybe_unused]] int status, [[maybe_unused]] QMouseEvent* e) {
}

void RS_ActionInterface::onMouseLeftButtonPress([[maybe_unused]] int status, [[maybe_unused]] QMouseEvent* e) {
}

void RS_ActionInterface::onMouseRightButtonPress([[maybe_unused]] int status, [[maybe_unused]] QMouseEvent* e) {
}

/**
 * Called when a key is pressed and this is the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
void RS_ActionInterface::keyPressEvent(QKeyEvent* e) {
    e->ignore();
}

/**
 * Called when a key is released and this is the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
void RS_ActionInterface::keyReleaseEvent(QKeyEvent* e) {
    e->ignore();
}

/**
 * Coordinate event. Triggered usually from a command line.
 * This function can be overwritten by the implementing action.
 * The default implementation just checks preconditions and delegates
 * actual processing to method that may be overwritten for specific
 * implementation
 */
void RS_ActionInterface::coordinateEvent(RS_CoordinateEvent* e) {
    m_graphicView->hideRelativeInputWidget();
    m_restoreRelativeInput = false;
    if (e == nullptr) {
        return;
    }

    // retrieve coordinates
    const RS_Vector wcsPos = e->getCoordinate();
    if (!wcsPos.valid) {
        return;
    }
    // check whether it's zero - so it might be from "0" shortcut
    // use it to handle "0" shortcut (it is passed as 0,0 vector)
    const bool isZero = e->isZero();

    // delegate further processing
    onCoordinateEvent(m_status, isZero, wcsPos);
}

/**
 * Expansion point for coordinate event processing.
 * @param status current status of the action
 * @param isZero true if coordinate is zero (so it's shortcut).
 * @param coord coordinate
 */
void RS_ActionInterface::onCoordinateEvent([[maybe_unused]] int status, [[maybe_unused]] bool isZero,
                                           [[maybe_unused]] const RS_Vector& coord) {
}

/**
 * Called when a command from the command line is launched.
 * and this is the current action.
 * This function can be overwritten by the implementing action.
* default implementation  simply prepares command and handle accepting event.
* Actual processing is delegated to inherited method
* @param e
*/
void RS_ActionInterface::commandEvent(RS_CommandEvent* e) {
    m_graphicView->hideRelativeInputWidget();
    m_restoreRelativeInput = false;
    const QString c = prepareCommand(e);
    if (!c.isEmpty()) {
        if (checkCommand("help", c)) {
            const QStringList& list = getAvailableCommands();
            if (!list.isEmpty()) {
                commandMessage(msgAvailableCommands() + " " + list.join(", ") + getAdditionalHelpMessage());
            }
            else {
                // fixme - need some indication that commands are not supported
            }
            e->accept();
        }
        else {
            const bool accept = doProcessCommand(getStatus(), c);
            if (accept) {
                e->accept();
            }
        }
    }
}

QString RS_ActionInterface::prepareCommand(RS_CommandEvent* e) const {
    const QString& c = e->getCommand().toLower().trimmed();
    return c;
}

QString RS_ActionInterface::getAdditionalHelpMessage() {
    return {};
}

bool RS_ActionInterface::doProcessCommand([[maybe_unused]] int status, [[maybe_unused]] const QString& command) {
    return false;
}

/**
 * Must be implemented to return the currently available commands
 *  for the command line.
 */
QStringList RS_ActionInterface::getAvailableCommands() {
    return {};
}

/**
 * Sets the current status (progress) of this action.
 * The default implementation sets the class variable 'status' to the
 * given value and finishes the action if 'status' is negative.
 *
 * @param status Status number. It's up to the action implementor
 *               what the action uses the status for. However, a
 *               negative status number finishes the action. Usually
 *               the status of an action increases for every step
 *               of progress and decreases when the user goes one
 *               step back (i.e. presses the right mouse button).
 */
void RS_ActionInterface::setStatus(const int status) {
    m_status = status;
    updateActionPrompt();
    updateMouseCursor();
    if (status < 0) {
        finish();
    }
}

/**
 * @return Current status of this action.
 */
int RS_ActionInterface::getStatus() const {
    return m_status;
}

/**
 * Triggers this action. This should be called after all
 * data needed for this action was collected / set.
 * The default implementation does nothing.
 */
void RS_ActionInterface::trigger() {
}

/**
 * Should be overwritten to update the mouse button hints
 * wherever they might needed.
 */
void RS_ActionInterface::updateActionPrompt() {
}

/**
 * Should be overwritten to set the mouse cursor for this action.
 * Default implementation for the base method. Simply ask appropriate method for cursor and sets it.
 */
void RS_ActionInterface::updateMouseCursor() {
    const int status = getStatus();
    const RS2::CursorType cursor = doGetMouseCursor(status);
    if (cursor != RS2::NoCursorChange) {
        setMouseCursor(cursor);
    }
}

/**
 * Returns cursor for the given state. Default implementation returns CadCursor, inherited actions may add more sophisticated processing.
 * @param status status of the action
 * @return cursor
 */
RS2::CursorType RS_ActionInterface::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::NoCursorChange;
}

/**
 * @return true, if the action is finished and can be deleted.
 */
bool RS_ActionInterface::isFinished() const {
    return m_finished;
}

/**
 * Forces a termination of the action without any cleanup.
 */
void RS_ActionInterface::setFinished() {
    m_status = -1;
}

/**
 * Finishes this action.
 */
void RS_ActionInterface::finish() {
    RS_DEBUG->print("RS_ActionInterface::finish");
    //refuse to quit the default action
    if (rtti() != RS2::ActionDefault) {
        m_status = -1;
        m_finished = true;
        hideOptions();
        RS_Snapper::finish();
    }
    RS_DEBUG->print("RS_ActionInterface::finish: OK");
}

/**
 * Called by the event handler to give this action a chance to
 * communicate with its predecessor.
 */
void RS_ActionInterface::setPredecessor(const std::shared_ptr<RS_ActionInterface> pre) {
    m_predecessor = pre;
}

std::shared_ptr<RS_ActionInterface> RS_ActionInterface::getPredecessor() const {
    return m_predecessor;
}

/**
 * Suspends this action while another action takes place.
 */
void RS_ActionInterface::suspend() {
    RS_Snapper::suspend();
}

/**
 * Resumes an action after it was suspended.
 */
void RS_ActionInterface::resume() {
    updateMouseCursor();
    updateActionPrompt();
    updateSnapAngleStep();
    RS_Snapper::resume();
}

/**
 * Hides the tool options.
 */
void RS_ActionInterface::hideOptions() {
    if (m_optionsEditor != nullptr) {
        saveOptions();
        m_optionsEditor->hideOptions();
    }
}

void RS_ActionInterface::updateOptions(const QString& tagToFocus) const {
    if (m_optionsEditor != nullptr) {
        m_optionsEditor->updateOptions(tagToFocus);
        /*QTimer::singleShot(5, [this, tagToFocus]() {
            if (m_optionsEditor != nullptr) {
                m_optionsEditor->updateOptions(tagToFocus);
            }
        });*/
    }
}

void RS_ActionInterface::createOptionsEditor() {
    m_optionsEditor.reset(new LC_ActionOptionsEditorTyped(this, [this] {
        return createOptionsWidget();
    }, [this] {
        return createOptionsFiller();
    }));
}

void RS_ActionInterface::postCreateInit() {
    loadOptions();
    createOptionsEditor();
    m_visualSnapManager->setClearSnapData(!isSupportsPredecessorAction());
}

void RS_ActionInterface::updateOptionsUI(const int mode, const QVariant *value) const {
    if (m_optionsEditor != nullptr) {
        m_optionsEditor->updateOptionsUI(mode, value);
    }
}

/**
 * Shows the tool options. Default implementation does nothing.
 */
void RS_ActionInterface::showOptions() const {
    if (m_optionsEditor != nullptr) {
        m_optionsEditor->showOptions();
    }
}

void RS_ActionInterface::onLateRequestCompleted(const bool shouldBeSkipped) {
    if (!shouldBeSkipped) {
        const auto inputInfo = m_actionContext->getInteractiveInputInfo();
        bool updated = false;
        const auto requestorTag = inputInfo->requestorTag;
        switch (inputInfo->inputType) {
            case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                updated = doUpdateAngleByInteractiveInput(requestorTag, inputInfo->angleRad);
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
                updated = doUpdateDistanceByInteractiveInput(requestorTag, inputInfo->distance);
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::POINT:
            case LC_ActionContext::InteractiveInputInfo::POINT_X:
            case LC_ActionContext::InteractiveInputInfo::POINT_Y: {
                updated = doUpdatePointByInteractiveInput(requestorTag, inputInfo->wcsPoint);
                break;
            }
            default:
                break;
        }
        if (updated) {
            updateOptions(requestorTag);
        }
    }
    m_actionContext->interactiveInputRequestCancel();
}

LC_ActionOptionsWidget* RS_ActionInterface::createOptionsWidget() {
    return nullptr;
}

LC_ActionOptionsPropertiesFiller* RS_ActionInterface::createOptionsFiller() {
    return nullptr;
}

void RS_ActionInterface::setActionType(const RS2::ActionType actionType) {
    this->m_actionType = actionType;
}

/**
 * Calls checkCommand() from the RS_COMMANDS module.
 */
// fixme - check for type and string literal
bool RS_ActionInterface::checkCommand(const QString& cmd, const QString& str, const RS2::ActionType action) {
    return (getAvailableCommands().contains(str) && cmd == str) || RS_COMMANDS->checkCommand(cmd, str, action);
}

/**
 * Calls command() from the RS_COMMANDS module.
 * Utility method to reduce dependencies in inherited actions
 * @param cmd command
 */
// fixme - check for type and string literal
QString RS_ActionInterface::command(const QString& cmd) {
    return RS_COMMANDS->command(cmd);
}

void RS_ActionInterface::switchToAction(const RS2::ActionType actionType, void* data) const {
    m_actionContext->setCurrentAction(actionType, data);
}

/**
 * Calls msgAvailableCommands() from the RS_COMMANDS module.
 */
QString RS_ActionInterface::msgAvailableCommands() {
    return RS_COMMANDS->msgAvailableCommands(); // fixme - sand - via m_actionContext
}

int RS_ActionInterface::getGraphicVariableInt(const QString& key, const int def) const {
    return (m_graphic != nullptr) ? m_graphic->getGraphicVariableInt(key, def) : def;
}

void RS_ActionInterface::setMouseCursor(const RS2::CursorType cursor) const {
    if (m_graphicView != nullptr) {
        m_graphicView->setMouseCursor(cursor);
    }
}

void RS_ActionInterface::preparePromptForInfoCursorOverlay(const QString& msg, const LC_ModifiersInfo& modifiers) const {
    QString prompt = "";
    const LC_InfoCursorOverlayPrefs* prefs = getInfoCursorOverlayPrefs();
    if (prefs->showCommandPrompt) {
        if (prefs->showCurrentActionName) {
            const QString actionName = m_graphicView->getCurrentActionName();
            if (!actionName.isEmpty()) {
                prompt = actionName + ": " + msg;
            }
            else {
                prompt = msg;
            }
        }
        else {
            prompt = msg;
        }
        QString modifiersStr = "";
        const QString& shiftMessage = modifiers.getShiftMessage();
        if (!shiftMessage.isEmpty()) {
            modifiersStr = modifiersStr + tr("SHIFT:") + shiftMessage;
        }
        const QString& ctrlMessage = modifiers.getCtrlMessage();
        if (!ctrlMessage.isEmpty()) {
            if (!modifiersStr.isEmpty()) {
                modifiersStr = modifiersStr + " | ";
            }
            modifiersStr = modifiersStr + tr("CTRL:") + ctrlMessage;
        }

        if (!modifiersStr.isEmpty()) {
            prompt = prompt + "\n" + modifiersStr;
        }
    }
    m_infoCursorOverlayData->setZone4(prompt);
}

/**
 * Just a shortcut for updating action prompt with message that should be translated
 * @param msg
 * @param modifiers
 */
void RS_ActionInterface::updatePromptTRBack(const QString& msg, const LC_ModifiersInfo& modifiers) const {
    updatePrompt(msg, tr("Back"), modifiers);
}

/**
 * Just a shortcut for updating action promopt with message that should be translated
 * @param msg
 * @param modifiers
 */
void RS_ActionInterface::updatePromptTRCancel(const QString& msg, const LC_ModifiersInfo& modifiers) const {
    updatePrompt(msg, tr("Cancel"), modifiers);
}

/**
 * Shortcut for updating mouse widget by given strings
 * @param left string
 * @param right string
 * @param modifiers
 */
void RS_ActionInterface::updatePrompt(const QString& left, const QString& right, const LC_ModifiersInfo& modifiers) const {
    if (m_infoCursorOverlayPrefs->enabled) {
        preparePromptForInfoCursorOverlay(left, modifiers);
    }
    m_actionContext->updateActionPrompt(left, right, modifiers);
}

void RS_ActionInterface::clearMouseWidgetIcon() const {
    m_infoCursorOverlayData->setZone4("");
}

/**
 * Shortcut for displaying command message string
 * @param msg string
 */
void RS_ActionInterface::commandMessage(const QString& msg) const {
    m_actionContext->commandMessage(msg);
}

void RS_ActionInterface::commandPrompt(const QString& msg) const {
    m_actionContext->commandPrompt(msg);
}



bool RS_ActionInterface::isControl(const QInputEvent* e) {
    return (e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) != 0u;
}

bool RS_ActionInterface::isShift(const QInputEvent* e) {
    return (e->modifiers() & Qt::ShiftModifier) != 0u;
}

bool RS_ActionInterface::isAlt(const QInputEvent* e) {
    return (e->modifiers() & Qt::AltModifier) != 0u;
}

void RS_ActionInterface::fireCoordinateEvent(const RS_Vector& coord) {
    auto ce = RS_CoordinateEvent(coord);
    coordinateEvent(&ce);
}

void RS_ActionInterface::initPrevious(const int status) {
    init(status - 1);
}

void RS_ActionInterface::undoCycleReplace(RS_Entity* entityToReplace, RS_Entity* entityReplacing) const {
    m_document->undoableModify(m_viewport, [entityToReplace, entityReplacing](LC_DocumentModificationBatch& ctx)-> bool {
        ctx += entityReplacing;
        ctx -= entityToReplace;
        return true;
    });
}

void RS_ActionInterface::setPenAndLayerToActive(RS_Entity* e) {
    e->setLayerToActive();
    e->setPenToActive();
}

void RS_ActionInterface::suspendRelativeInputWidget() {
    m_restoreRelativeInput = m_graphicView->isInRelativePointInput();
    m_graphicView->hideRelativeInputWidget();
}
