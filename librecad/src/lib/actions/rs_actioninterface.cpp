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

#include "rs_actioninterface.h"

#include <QMouseEvent>

#include "lc_actioncontext.h"
#include "lc_actionoptionswidget.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

/**
 * Constructor.
 *
 * Sets the entity container on which the action class inherited
 * from this interface operates.
 *
 * @param name Action name. This can be used internally for
 *             debugging mainly.
 * @param container Entity container this action operates on.
 * @param graphicView Graphic view instance this action operates on.
 *                    Please note that an action belongs to this
 *                    view.
 * @param cursor Default mouse cursor for this action. If the action
 *               is suspended and resumed again the cursor will always
 *               be reset to the one given here.
 */
RS_ActionInterface::RS_ActionInterface(const char *name,
                                       LC_ActionContext *actionContext,
                                       RS2::ActionType actionType)
    :RS_Snapper(actionContext)
    , m_status{0}
    , m_name{name}
    , m_finished{false}
    , m_graphic{m_container->getGraphic()}
    , m_document{m_container->getDocument()}
    , m_actionType{actionType}{

    RS_DEBUG->print("RS_ActionInterface::RS_ActionInterface: Setting up action: \"%s\"", name);

    //triggerOnResume = false;

    // graphic provides a pointer to the graphic if the
    // entity container is a graphic (i.e. can also hold
    // layers).
    // graphic = container.getGraphic();

    // document pointer will be used for undo / redo
    // document = container.getDocument();

    updateSnapAngleStep();

    RS_DEBUG->print("RS_ActionInterface::RS_ActionInterface: Setting up action: \"%s\": OK", name);
}

RS_ActionInterface::~RS_ActionInterface(){
        if (m_optionWidget != nullptr){
        m_optionWidget->deleteLater();
        m_optionWidget.release();
        m_optionWidget.reset();
    }
}

/**
 * Must be implemented to return the ID of this action.
*
* @todo no default implementation
 */
RS2::ActionType RS_ActionInterface::rtti() const{
	return m_actionType;
}

/**
 * @return name of this action
 */
QString RS_ActionInterface::getName() {
    return m_name;
}

void RS_ActionInterface::setName(const char* _name) {
    m_name=_name;
}

bool RS_ActionInterface::mayInitWithContextEntity(int status) {
    return status == InitialActionStatus;
}

/**
 * Called to initiate an action. This funtcion is often
 * overwritten by the implementing action.
 *
 * @param status The status on which to initiate this action.
 * default is 0 to begin the action.
 */
void RS_ActionInterface::init(int status){
    setStatus(status);
    if (status >= InitialActionStatus) {
        RS_Snapper::init();
        updateMouseButtonHints();
        updateMouseCursor();
        if (mayInitWithContextEntity(status)) {
            doInitialInit();
            auto contextEntity = m_actionContext->getContextMenuActionContextEntity();
            if (contextEntity != nullptr) {
                // LC_ERR << "Action CTX INIT " << m_name;
                doInitWithContextEntity(contextEntity, m_actionContext->getContextMenuActionClickPosition());
                m_actionContext->clearContextMenuActionContext();
            }
        }
    } else{
        //delete snapper when finished, bug#3416878
        deleteSnapper();
    }
}

void RS_ActionInterface::doInitWithContextEntity([[maybe_unused]]RS_Entity* contextEntity, [[maybe_unused]]const RS_Vector& clickPos) {
}

void RS_ActionInterface::doInitialInit() {
}


/**
 * Called when the mouse moves and this is the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation keeps track of the mouse position.
 */
void RS_ActionInterface::mouseMoveEvent(QMouseEvent*) {}

/**
 * Called when the left mouse button is pressed and this is the
 * current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
// todo - add default implementation?
void RS_ActionInterface::mousePressEvent(QMouseEvent* e) {
    Qt::MouseButton button = e->button();
    if (button == Qt::LeftButton){
        onMouseLeftButtonPress(m_status, e);
    } else if (button == Qt::RightButton){
        onMouseRightButtonPress(m_status, e);
    }
}

/**
 * Called when the left mouse button is released and this is
 * the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
void RS_ActionInterface::mouseReleaseEvent(QMouseEvent* e){
    Qt::MouseButton button;
    button = e->button();
    if (button == Qt::LeftButton){
        onMouseLeftButtonRelease(m_status, e);
    } else if (button == Qt::RightButton){
        onMouseRightButtonRelease(m_status, e);
    }
}

void RS_ActionInterface::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent* e){}
void RS_ActionInterface::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent* e){}
void RS_ActionInterface::onMouseLeftButtonPress([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent* e){}
void RS_ActionInterface::onMouseRightButtonPress([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent* e){}

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
    if (e == nullptr){
        return;
    }

    // retrieve coordinates
    RS_Vector wcsPos = e->getCoordinate();
    if (!wcsPos.valid){
        return;
    }
    // check whether it's zero - so it might be from "0" shortcut
    // use it to handle "0" shortcut (it is passed as 0,0 vector)
    bool isZero = e->isZero();

    // delegate further processing
    onCoordinateEvent(m_status, isZero, wcsPos);
}

/**
 * Expansion point for coordinate event processing.
 * @param status current status of the action
 * @param isZero true if coordinate is zero (so it's shortcut).
 * @param pos coordinate
 */
void RS_ActionInterface::onCoordinateEvent([[maybe_unused]]int status, [[maybe_unused]]bool isZero, [[maybe_unused]]const RS_Vector &pos) {}

/**
 * Called when a command from the command line is launched.
 * and this is the current action.
 * This function can be overwritten by the implementing action.
* default implementation  simply prepares command and handle accepting event.
* Actual processing is delegated to inherited method
* @param e
*/
void RS_ActionInterface::commandEvent(RS_CommandEvent* e) {
    QString c = prepareCommand(e);
    if (!c.isEmpty()) {
        if (checkCommand("help", c)) {
            const QStringList &list = getAvailableCommands();
            if (!list.isEmpty()) {
                commandMessage(msgAvailableCommands() + list.join(", ") + getAdditionalHelpMessage());
            } else {
                // fixme - need some indication that commands are not supported
            }
            e->accept();
        } else {
            bool accept = doProcessCommand(getStatus(), c);
            if (accept) {
                e->accept();
            }
        }
    }
}

QString RS_ActionInterface::prepareCommand(RS_CommandEvent *e) const {
    QString const &c = e->getCommand().toLower().trimmed();
    return c;
}

QString RS_ActionInterface::getAdditionalHelpMessage() {return {};}

bool RS_ActionInterface::doProcessCommand([[maybe_unused]]int status, [[maybe_unused]]const QString &command) {return false;}

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
void RS_ActionInterface::setStatus(int status) {
    m_status = status;
    updateMouseButtonHints();
    updateMouseCursor();
    if(status<0) {
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
void RS_ActionInterface::trigger() {}

/**
 * Should be overwritten to update the mouse button hints
 * wherever they might needed.
 */
void RS_ActionInterface::updateMouseButtonHints() {}

/**
 * Should be overwritten to set the mouse cursor for this action.
 * Default implementation for the base method. Simply ask appropriate method for cursor and sets it.
 */
void RS_ActionInterface::updateMouseCursor(){
    int status = getStatus();
    RS2::CursorType cursor = doGetMouseCursor(status);
    if (cursor != RS2::NoCursorChange){
        setMouseCursor(cursor);
    }
}

/**
 * Returns cursor for the given state. Default implementation returns CadCursor, inherited actions may add more sophisticated processing.
 * @param status status of the action
 * @return cursor
 */
RS2::CursorType RS_ActionInterface::doGetMouseCursor([[maybe_unused]]int status){
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
void RS_ActionInterface::finish(bool /*updateTB*/){
	RS_DEBUG->print("RS_ActionInterface::finish");
	//refuse to quit the default action
	if(rtti() != RS2::ActionDefault) {
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
void RS_ActionInterface::setPredecessor(std::shared_ptr<RS_ActionInterface> pre) {
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
    updateMouseButtonHints();
    updateSnapAngleStep();
    RS_Snapper::resume();
}

/**
 * Hides the tool options.
 */
void RS_ActionInterface::hideOptions() {
    if (m_optionWidget != nullptr){
        m_optionWidget->hideOptions();
    }
}

void RS_ActionInterface::updateOptions(const QString &tagToFocus){
    if (m_optionWidget == nullptr){
        LC_ActionOptionsWidget* widget = createOptionsWidget();
        if (widget != nullptr){
            m_optionWidget.reset(widget);
        }
    }
    if (m_optionWidget != nullptr){
        if (!m_optionWidget->isVisible()){
            if (m_optionWidget->parent() == nullptr){ // first time created
                m_actionContext->addOptionsWidget(m_optionWidget.get());
                m_optionWidget->setAction(this, true);
            } else {
                m_optionWidget->setAction(this, true);
                m_optionWidget->show();
            }
        }
        else{
            m_optionWidget->setAction(this, true);
        }
        if (!tagToFocus.isEmpty()) {
            m_optionWidget->requestFocusForTag(tagToFocus);
        }
    }
}

void RS_ActionInterface::updateOptionsUI(int mode){
    if (m_optionWidget != nullptr){
        m_optionWidget->updateUI(mode);
    }
}

/**
 * Shows the tool options. Default implementation does nothing.
 */
void RS_ActionInterface::showOptions() {
    if (m_optionWidget == nullptr){
        m_optionWidget.reset(createOptionsWidget());
    }
    if (m_optionWidget != nullptr){
        if (!m_optionWidget->isVisible()){
            if (m_optionWidget->parent() == nullptr){ // first time created
                m_actionContext->addOptionsWidget(m_optionWidget.get());
                m_optionWidget->setAction(this);
            } else {
                m_optionWidget->show();
            }
        }
    }
}

void RS_ActionInterface::onLateRequestCompleted(bool shouldBeSkipped) {
    if (!shouldBeSkipped) {
        auto inputInfo = m_actionContext->getInteractiveInputInfo();
        bool updated = false;
        auto requestorTag = inputInfo->m_requestorTag;
        switch (inputInfo->m_inputType) {
            case LC_ActionContext::InteractiveInputInfo::ANGLE: {
                updated = doUpdateAngleByInteractiveInput(requestorTag, inputInfo->m_angleRad);
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::DISTANCE: {
                updated = doUpdateDistanceByInteractiveInput(requestorTag, inputInfo->m_distance);
                break;
            }
            case LC_ActionContext::InteractiveInputInfo::POINT: {
                updated = doUpdatePointByInteractiveInput(requestorTag, inputInfo->m_wcsPoint);
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

LC_ActionOptionsWidget* RS_ActionInterface::createOptionsWidget(){
    return nullptr;
}

void RS_ActionInterface::setActionType(RS2::ActionType actionType){
    this->m_actionType=actionType;
}

/**
 * Calls checkCommand() from the RS_COMMANDS module.
 */
// fixme - check for type and string literal
bool RS_ActionInterface::checkCommand(const QString& cmd, const QString& str,
                                      RS2::ActionType action) {
    return (getAvailableCommands().contains(str) && cmd == str)
        || RS_COMMANDS->checkCommand(cmd, str, action);
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

void RS_ActionInterface::switchToAction(RS2::ActionType actionType, void* data) {
    m_actionContext->setCurrentAction(actionType, data);
}

/**
 * Calls msgAvailableCommands() from the RS_COMMANDS module.
 */
QString RS_ActionInterface::msgAvailableCommands() {
    return RS_COMMANDS->msgAvailableCommands(); // fixme - sand - via m_actionContext
}

int RS_ActionInterface::getGraphicVariableInt(const QString& key, int def) const{
    return (m_graphic != nullptr) ? m_graphic->getGraphicVariableInt(key, def) : def;
}

void RS_ActionInterface::updateSelectionWidget() const{
    const RS_EntityContainer::LC_SelectionInfo &info = m_container->getSelectionInfo();
    updateSelectionWidget(info.count, info.length);
}

void RS_ActionInterface::updateSelectionWidget(int countSelected, double selectedLength) const{
    m_actionContext->updateSelectionWidget(countSelected,selectedLength);
}

void RS_ActionInterface::setMouseCursor(const RS2::CursorType &cursor){
    if (m_graphicView != nullptr) {
        m_graphicView->setMouseCursor(cursor);
    }
}

/**
 * Just a shortcut for updating mouse widgets with message that should be translated
 * @param left left string (key for tr())
 * @param right right string (key for tr())
 */
void RS_ActionInterface::updateMouseWidgetTRBack(const QString &msg, const LC_ModifiersInfo& modifiers){
    if  (m_infoCursorOverlayPrefs->enabled) {
        preparePromptForInfoCursorOverlay(msg, modifiers);
    }
    m_actionContext->updateMouseWidget(msg,tr("Back"), modifiers);
}

void RS_ActionInterface::preparePromptForInfoCursorOverlay(const QString &msg, const LC_ModifiersInfo &modifiers) {
    QString prompt = "";
    LC_InfoCursorOverlayPrefs* prefs = getInfoCursorOverlayPrefs();
    if (prefs->showCommandPrompt){
        if (prefs->showCurrentActionName) {
            QString actionName = m_graphicView->getCurrentActionName();
            if (!actionName.isEmpty()){
                prompt = actionName + ": " + msg;
            }
            else {
                prompt = msg;
            }
        }
        else{
            prompt = msg;
        }
        QString modifiersStr = "";
        const QString &shiftMessage = modifiers.getShiftMessage();
        if (!shiftMessage.isEmpty()){
            modifiersStr = modifiersStr + tr("SHIFT:") + shiftMessage;
        }
        const QString &ctrlMessage = modifiers.getCtrlMessage();
        if (!ctrlMessage.isEmpty()){
            if (!modifiersStr.isEmpty()){
                modifiersStr = modifiersStr + " | ";
            }
            modifiersStr = modifiersStr + tr("CTRL:") + ctrlMessage;
        }

        if (!modifiersStr.isEmpty()){
            prompt = prompt + "\n" + modifiersStr;
        }
    }
    m_infoCursorOverlayData->setZone4(prompt);
}

/**
 * Just a shortcut for updating mouse widgets with message that should be translated
 * @param left left string (key for tr())
 * @param right right string (key for tr())
 */
void RS_ActionInterface::updateMouseWidgetTRCancel(const QString &msg, const LC_ModifiersInfo& modifiers){
    if (m_infoCursorOverlayPrefs->enabled) {
        preparePromptForInfoCursorOverlay(msg, modifiers);
    }
    m_actionContext->updateMouseWidget(msg,tr("Cancel"), modifiers);
}

/**
 * Shortcut for updating mouse widget by given strings
 * @param left string
 * @param right string
 */
void RS_ActionInterface::updateMouseWidget(const QString& left,const QString& right, const LC_ModifiersInfo& modifiers){
    if (m_infoCursorOverlayPrefs->enabled) {
        preparePromptForInfoCursorOverlay(left, modifiers);
    }
    m_actionContext->updateMouseWidget(left, right, modifiers);
}

void RS_ActionInterface::clearMouseWidgetIcon(){
    m_infoCursorOverlayData->setZone4("");
}

/**
 * Shortcut for displaying command message string
 * @param msg string
 */
void RS_ActionInterface::commandMessage(const QString &msg) const{
    m_actionContext->commandMessage(msg);
}

void RS_ActionInterface::commandPrompt(const QString &msg) const{
    m_actionContext->commandPrompt(msg);
}

void RS_ActionInterface::updateSnapAngleStep() {
    int stepType = LC_GET_ONE_INT("Defaults", "AngleSnapStep", 3);
    double snapStepDegrees;
    switch (stepType){
        case 0:
            snapStepDegrees = 1.0;
            break;
        case 1:
            snapStepDegrees = 3.0;
            break;
        case 2:
            snapStepDegrees = 5.0;
            break;
        case 3:
            snapStepDegrees = 10.0;
            break;
        case 4:
            snapStepDegrees = 15.0;
            break;
        case 5:
            snapStepDegrees = 18.0;
            break;
        case 6:
            snapStepDegrees = 22.5;
            break;
        case 7:
            snapStepDegrees = 30.0;
            break;
        case 8:
            snapStepDegrees = 45.0;
            break;
        case 9:
            snapStepDegrees = 90.0;
            break;
        default:
            snapStepDegrees = 15.0;
    }
    m_snapToAngleStep = RS_Math::deg2rad(snapStepDegrees);
}

bool RS_ActionInterface::isControl(const QInputEvent *e){
    return  e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier);
}

bool RS_ActionInterface::isShift(const QInputEvent *e){
    return  e->modifiers() & Qt::ShiftModifier;
}

bool RS_ActionInterface::isAlt(const QInputEvent *e){
    return  e->modifiers() & Qt::AltModifier;
}


void RS_ActionInterface::fireCoordinateEvent(const RS_Vector &coord){
    auto ce = RS_CoordinateEvent(coord);
    coordinateEvent(&ce);
}

void RS_ActionInterface::initPrevious(int stat) {
    init(stat - 1);
}

bool RS_ActionInterface::undoCycleAdd(RS_Entity *e, bool addToContainer) const{
    // upd. undo list:
    if (addToContainer){
        m_container->addEntity(e);
    }
    if (m_document){
        undoCycleStart();
        undoableAdd(e);
        undoCycleEnd();
        return true;
    }
    return false;
}

/**
 * Just utility method for deleting given entity from drawing - should be called within undo cycle
 * @param entity entity to delete
 */
void RS_ActionInterface::undoableDeleteEntity(RS_Entity *entity){
    entity->changeUndoState();
    undoableAdd(entity);
}

void RS_ActionInterface::undoCycleReplace(RS_Entity *entityToReplace, RS_Entity *entityReplacing) {
    if (m_document != nullptr) {
        undoCycleStart();
        undoableDeleteEntity(entityToReplace);
        undoableAdd(entityReplacing);
        undoCycleEnd();
    }
}

void RS_ActionInterface::undoCycleEnd() const {
    RS_Undoable* relZeroUndoable = m_viewport->getRelativeZeroUndoable();
    if (relZeroUndoable != nullptr) {
        m_document->addUndoable(relZeroUndoable);
    }
    m_document->endUndoCycle();
}

void RS_ActionInterface::undoCycleStart() const { m_document->startUndoCycle(); }
void RS_ActionInterface::undoableAdd(RS_Undoable *e) const { m_document->addUndoable(e); }

void RS_ActionInterface::setPenAndLayerToActive(RS_Entity *e) {
    e->setLayerToActive();
    e->setPenToActive();
}
