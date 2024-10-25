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


#include "rs.h"
#include "rs_actioninterface.h"
#include "rs_commands.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_settings.h"
#include "lc_actionoptionswidget.h"

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
                                       RS_EntityContainer &container,
                                       RS_GraphicView &graphicView,
                                       RS2::ActionType actionType)
    :
    RS_Snapper(container, graphicView)
    , status{0}
    , name{name}
    , finished{false}
    , graphic{container.getGraphic()}
    , document{container.getDocument()}
    , actionType{actionType}{

    RS_DEBUG->print("RS_ActionInterface::RS_ActionInterface: Setting up action: \"%s\"", name);

    //triggerOnResume = false;

    // graphic provides a pointer to the graphic if the
    // entity container is a graphic (i.e. can also hold
    // layers).
    graphic = container.getGraphic();

    // document pointer will be used for undo / redo
    document = container.getDocument();

    updateSnapAngleStep();

    RS_DEBUG->print("RS_ActionInterface::RS_ActionInterface: Setting up action: \"%s\": OK", name);
}

RS_ActionInterface::~RS_ActionInterface(){
    if (m_optionWidget != nullptr){
        m_optionWidget->deleteLater();
        m_optionWidget.release();
    }
}

/**
 * Must be implemented to return the ID of this action.
*
* @todo no default implementation
 */
RS2::ActionType RS_ActionInterface::rtti() const{
	return actionType;
}

/**
 * @return name of this action
 */
QString RS_ActionInterface::getName() {
    return name;
}

void RS_ActionInterface::setName(const char* _name) {
    this->name=_name;
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
    if (status>=0) {
        RS_Snapper::init();
        updateMouseButtonHints();
        updateMouseCursor();
    }else{
        //delete snapper when finished, bug#3416878
        deleteSnapper();
    }
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
void RS_ActionInterface::mousePressEvent(QMouseEvent*) {}

/**
 * Called when the left mouse button is released and this is
 * the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
void RS_ActionInterface::mouseReleaseEvent(QMouseEvent* e){
    Qt::MouseButton button = e->button();
    if (button == Qt::LeftButton){
        onMouseLeftButtonRelease(status, e);
    } else if (button == Qt::RightButton){
        onMouseRightButtonRelease(status, e);
    }
}

void RS_ActionInterface::onMouseLeftButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent* e){}
void RS_ActionInterface::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent* e){}

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
    RS_Vector pos = e->getCoordinate();
    if (!pos.valid){
        return;
    }
    // check whether it's zero - so it might be from "0" shortcut
    RS_Vector zero = RS_Vector(0, 0, 0);
    bool isZero = pos == zero; // use it to handle "0" shortcut (it is passed as 0,0 vector)

    // delegate further processing
    onCoordinateEvent(status, isZero, pos);
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
    this->status = status;
    updateMouseButtonHints();
    updateMouseCursor();
    if(status<0) finish();
}

/**
 * @return Current status of this action.
 */
int RS_ActionInterface::getStatus() const {
    return status;
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
    return finished;
}


/**
 * Forces a termination of the action without any cleanup.
 */
void RS_ActionInterface::setFinished() {
        status = -1;
}


/**
 * Finishes this action.
 */
void RS_ActionInterface::finish(bool /*updateTB*/)
{
	RS_DEBUG->print("RS_ActionInterface::finish");
	//refuse to quit the default action
	if(rtti() != RS2::ActionDefault) {
		status = -1;
		finished = true;
		hideOptions();
		RS_Snapper::finish();
	}
	RS_DEBUG->print("RS_ActionInterface::finish: OK");
}

/**
 * Called by the event handler to give this action a chance to
 * communicate with its predecessor.
 */
void RS_ActionInterface::setPredecessor(RS_ActionInterface* pre) {
    predecessor = pre;
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
        RS_DIALOGFACTORY->removeOptionsWidget(m_optionWidget.get());
        m_optionWidget.release();
    }
}

void RS_ActionInterface::updateOptions(){
    if (m_optionWidget == nullptr){
        LC_ActionOptionsWidget* widget = createOptionsWidget();
        if (widget != nullptr){
            m_optionWidget.reset(widget);
        }
    }
    if (m_optionWidget != nullptr){
        if (!m_optionWidget->isVisible()){
            if (m_optionWidget->parent() == nullptr){ // first time created
                RS_DIALOGFACTORY->addOptionsWidget(m_optionWidget.get());
                m_optionWidget->setAction(this, true);
            } else {
                m_optionWidget->setAction(this, true);
                m_optionWidget->show();
            }
        }
        else{
            m_optionWidget->setAction(this, true);
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
        LC_ActionOptionsWidget* widget = createOptionsWidget();
        if (widget != nullptr){
            m_optionWidget.reset(widget);
        }
    }
    if (m_optionWidget != nullptr){
        if (!m_optionWidget->isVisible()){
            if (m_optionWidget->parent() == nullptr){ // first time created
                RS_DIALOGFACTORY->addOptionsWidget(m_optionWidget.get());
                m_optionWidget->setAction(this);
            } else {
                m_optionWidget->show();
            }
        }
    }
}

LC_ActionOptionsWidget* RS_ActionInterface::createOptionsWidget(){
    return nullptr;
}

void RS_ActionInterface::setActionType(RS2::ActionType actionType){
    this->actionType=actionType;
}

/**
 * Calls checkCommand() from the RS_COMMANDS module.
 */
// fixme - check for type and string literal
bool RS_ActionInterface::checkCommand(const QString& cmd, const QString& str,
                                      RS2::ActionType action) {
    return RS_COMMANDS->checkCommand(cmd, str, action);
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

/**
 * Calls msgAvailableCommands() from the RS_COMMANDS module.
 */
QString RS_ActionInterface::msgAvailableCommands() {
    return RS_COMMANDS->msgAvailableCommands();
}

int RS_ActionInterface::getGraphicVariableInt(const QString& key, int def) const
{
    return (graphic != nullptr) ? graphic->getGraphicVariableInt(key, def) : def;
}

void RS_ActionInterface::updateSelectionWidget() const{
    const RS_EntityContainer::LC_SelectionInfo &info = container->getSelectionInfo();
    updateSelectionWidget(info.count, info.length);
//    updateSelectionWidget(container->countSelected(), container->totalSelectedLength());
}

void RS_ActionInterface::updateSelectionWidget(int countSelected, double selectedLength) const{
    RS_DIALOGFACTORY->updateSelectionWidget(countSelected,selectedLength);
}

void RS_ActionInterface::setMouseCursor(const RS2::CursorType &cursor){
    if (graphicView != nullptr) {
        graphicView->setMouseCursor(cursor);
    }
}

/**
 * Just a shortcut for updating mouse widgets with message that should be translated
 * @param left left string (key for tr())
 * @param right right string (key for tr())
 */
void RS_ActionInterface::updateMouseWidgetTRBack(const QString &msg, const LC_ModifiersInfo& modifiers){
    RS_DIALOGFACTORY->updateMouseWidget(msg,tr("Back"), modifiers);
}

/**
 * Just a shortcut for updating mouse widgets with message that should be translated
 * @param left left string (key for tr())
 * @param right right string (key for tr())
 */
void RS_ActionInterface::updateMouseWidgetTRCancel(const QString &msg, const LC_ModifiersInfo& modifiers){
    RS_DIALOGFACTORY->updateMouseWidget(msg,tr("Cancel"), modifiers);
}

/**
 * Shortcut for updating mouse widget by given strings
 * @param left string
 * @param right string
 */
void RS_ActionInterface::updateMouseWidget(const QString& left,const QString& right, const LC_ModifiersInfo& modifiers){
    RS_DIALOGFACTORY->updateMouseWidget(left, right, modifiers);
}

void RS_ActionInterface::clearMouseWidgetIcon(){
    RS_DIALOGFACTORY->clearMouseWidgetIcon();
}


/**
 * Shortcut for displaying command message string
 * @param msg string
 */
void RS_ActionInterface::commandMessage(const QString &msg) const{
    RS_DIALOGFACTORY->commandMessage(msg);
}

void RS_ActionInterface::updateSnapAngleStep() {
    int stepType = LC_GET_ONE_INT("Defaults", "AngleSnapStep", 3);
    switch (stepType){
        case 0:
            snapToAngleStep = 1.0;
            break;
        case 1:
            snapToAngleStep = 3.0;
            break;
        case 2:
            snapToAngleStep = 5.0;
            break;
        case 3:
            snapToAngleStep = 15.0;
            break;
        case 4:
            snapToAngleStep = 30.0;
            break;
        case 5:
            snapToAngleStep = 45.0;
            break;
        case 6:
            snapToAngleStep = 90.0;
            break;
        default:
            snapToAngleStep = 15.0;
    }
}

bool RS_ActionInterface::isControl(const QInputEvent *e){
    return  e->modifiers() & (Qt::ControlModifier | Qt::MetaModifier);
}

bool RS_ActionInterface::isShift(const QInputEvent *e){
    return  e->modifiers() & Qt::ShiftModifier;
}

void RS_ActionInterface::fireCoordinateEvent(const RS_Vector &coord){
    auto ce = RS_CoordinateEvent(coord);
    coordinateEvent(&ce);
}

void RS_ActionInterface::fireCoordinateEventForSnap(QMouseEvent *e){
    fireCoordinateEvent(snapPoint(e));
}

void RS_ActionInterface::initPrevious(int stat) {
    init(stat - 1);
}
