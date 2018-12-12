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

#include <QKeyEvent>
#include "rs_actioninterface.h"
#include "rs_graphicview.h"
#include "rs_commands.h"
#include "rs_dialogfactory.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"

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
RS_ActionInterface::RS_ActionInterface(const char* name,
                                       RS_EntityContainer& container,
                                       RS_GraphicView& graphicView) :
RS_Snapper(container, graphicView) {

    RS_DEBUG->print("RS_ActionInterface::RS_ActionInterface: Setting up action: \"%s\"", name);

    this->name = name;
    status = 0;
    finished = false;
    //triggerOnResume = false;

    // graphic provides a pointer to the graphic if the
    // entity container is a graphic (i.e. can also hold
    // layers).
    graphic = container.getGraphic();

    // document pointer will be used for undo / redo
    document = container.getDocument();

    //this->cursor = cursor;
    //setSnapMode(graphicView.getDefaultSnapMode());
    actionType=RS2::ActionNone;

    RS_DEBUG->print("RS_ActionInterface::RS_ActionInterface: Setting up action: \"%s\": OK", name);

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
void RS_ActionInterface::init(int status)
{
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
void RS_ActionInterface::mousePressEvent(QMouseEvent*) {}

/**
 * Called when the left mouse button is released and this is
 * the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
void RS_ActionInterface::mouseReleaseEvent(QMouseEvent*) {}

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
 * The default implementation does nothing.
 */
void RS_ActionInterface::coordinateEvent(RS_CoordinateEvent*) {}

/**
 * Called when a command from the command line is launched.
 * and this is the current action.
 * This function can be overwritten by the implementing action.
 * The default implementation does nothing.
 */
void RS_ActionInterface::commandEvent(RS_CommandEvent*) {
}

/**
 * Must be implemented to return the currently available commands
 *  for the command line.
 */
QStringList RS_ActionInterface::getAvailableCommands() {
	return QStringList{};
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
int RS_ActionInterface::getStatus() {
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
 */
void RS_ActionInterface::updateMouseCursor() {}

/**
 * @return true, if the action is finished and can be deleted.
 */
bool RS_ActionInterface::isFinished() {
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
	if(!(rtti() == RS2::ActionDefault || rtti()==RS2::ActionFilePrintPreview) ) {
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
    RS_Snapper::resume();
}

/**
 * Hides the tool options. Default implementation does nothing.
 */
void RS_ActionInterface::hideOptions() {
    RS_Snapper::hideOptions();
}

/**
 * Shows the tool options. Default implementation does nothing.
 */
void RS_ActionInterface::showOptions() {
    RS_Snapper::showOptions();
}

void RS_ActionInterface::setActionType(RS2::ActionType actionType){
	this->actionType=actionType;
}

/**
 * Calls checkCommand() from the RS_COMMANDS module.
 */
bool RS_ActionInterface::checkCommand(const QString& cmd, const QString& str,
                                      RS2::ActionType action) {
    return RS_COMMANDS->checkCommand(cmd, str, action);
}

/**
 * Calls command() from the RS_COMMANDS module.
 */
QString RS_ActionInterface::command(const QString& cmd) {
    return RS_COMMANDS->command(cmd);
}

/**
 * Calls msgAvailableCommands() from the RS_COMMANDS module.
 */
QString RS_ActionInterface::msgAvailableCommands() {
    return RS_COMMANDS->msgAvailableCommands();
}

