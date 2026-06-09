/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#include "qg_actionhandler.h"

#include "lc_action.h"
#include "lc_actionhandlerfactory.h"
#include "lc_defaultactioncontext.h"
#include "lc_graphicviewport.h"
#include "lc_snapmanager.h"
#include "qc_applicationwindow.h"
#include "rs_actionlayerstogglelock.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

/**
 * Constructor
 */
QG_ActionHandler::QG_ActionHandler(QC_ApplicationWindow* parent)
    : QObject(parent) {
}

void QG_ActionHandler::killAllActions() const {
    if (m_view != nullptr) {
        m_view->killAllActions();
    }
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface* QG_ActionHandler::getCurrentAction() const {
    if (m_view != nullptr) {
        return m_view->getCurrentAction();
    }
    return nullptr;
}

// fixme - sand - initial implementation of command promotion. ADD: 1) support of command aliases 2) notification type? 3) Stats for future displaying the user?
void QG_ActionHandler::promoteCommandIfNeeded(const RS2::ActionType id) const {
    const auto sndr = sender();
    const auto action = dynamic_cast<LC_Action*>(sndr);
    if (action != nullptr) {
        const bool actionInvokedViaShortcut = action->isInvokedViaShortcut();
        if (actionInvokedViaShortcut) {
            // shortcut is assigned to action, nothing to do (yet later we may note this fact!)
        }
        else{
            // unefficient way, invocation from UI. Promote command, if it is allowed
            const bool promoteCommands = LC_GET_ONE_BOOL("CommandsPromotion", "PromoteCommands", true);
            if (promoteCommands) {
                // fixme - sand - more details are needed (like aliases)
                const QString command = RS_COMMANDS->getCommandForAction(id);
                if (!command.isEmpty()) {
                    const auto actionName = action->text().remove("&");
                    // fixme - probably the exact way of notification should be isolated from there (say, support notification banners too)
                    QC_ApplicationWindow::getAppWindow()->commandMessage( QObject::tr("%2 - command for \"%1\"").arg(actionName).arg(command));
                }
            }
        }
    }
    else {
        // action is invoked via command, nothing to do
        // LC_ERR << "FROM CMD";
    }
}

/**
 * Sets current action.
 *
 * @return Pointer to the created action or nullptr.
 */
std::shared_ptr<RS_ActionInterface> QG_ActionHandler::setCurrentAction(const RS2::ActionType id, void* data) const {
    RS_DEBUG->print("QG_ActionHandler::setCurrentAction()");
    RS_DEBUG->print("QC_ActionHandler::setCurrentAction: view = %p, document = %p", m_view, m_document);

    // only global options are allowed without a document:
    if (m_view == nullptr || m_document == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "QG_ActionHandler::setCurrentAction: graphic view or document is nullptr");
        return nullptr;
    }

    promoteCommandIfNeeded(id);

    std::shared_ptr<RS_ActionInterface> a = createActionInstance(id, data);

    if (a != nullptr) {
        m_view->setCurrentAction(a);
    }

    RS_DEBUG->print("QG_ActionHandler::setCurrentAction(): OK");
    return a;
}

void QG_ActionHandler::setSnapManager(LC_SnapManager* snapManager) {
    m_snapManager = snapManager;
}

std::shared_ptr<RS_ActionInterface> QG_ActionHandler::createActionInstance(const RS2::ActionType id, void* data) const {
    return LC_ActionsHandlerFactory::createActionInstance(id, m_actionContext, data);
}

/**
 * @return Available commands of the application or the current action.
 */
QStringList QG_ActionHandler::getAvailableCommands() const {
    RS_ActionInterface* currentAction = getCurrentAction();

    if (currentAction != nullptr) {
        return currentAction->getAvailableCommands();
    }
    QStringList cmd;
    cmd += "line";
    cmd += "rectangle";
    return cmd;
}

/**
 * Launches the command represented by the given keycode if possible.
 *
 * @return true: the command was recognized.
 *         false: the command is not known and was probably intended for a
 *            running action.
 */
bool QG_ActionHandler::keycode(const QString& code) const {
    RS_DEBUG->print("QG_ActionHandler::keycode()");

    // keycode for new action:
    const RS2::ActionType type = RS_COMMANDS->keycodeToAction(code);
    if (type != RS2::ActionNone) {
        const bool result = m_snapManager->tryToProcessSnapActions(type);
        if (!result) {
            setCurrentAction(type);
            return true;
        }
    }
    return false;
}

/**
 * Launches the given command if possible.
 *
 * @return true: the command was recognized.
 *         false: the command is not known and was probably intended for a
 *            running action.
 */
bool QG_ActionHandler::command(const QString& cmd) const {
    if (m_view == nullptr) {
        return false;
    }

    if (cmd.isEmpty()) {
        if (LC_GET_BOOL("Keyboard/ToggleFreeSnapOnSpace")) {
            RS_DEBUG->print("QG_ActionHandler::command: toggle Snap Free: begin");
            const bool isSnappingFree = m_snapManager->toggleTemporarySnapFree();
            RS_DIALOGFACTORY->commandMessage(isSnappingFree?
                                                 tr("Spacebar: restored snapping mode to normal")
                                               : tr("Spacebar: temporarily set snapping mode to free snapping"));

            RS_DEBUG->print("QG_ActionHandler::command: toggle Snap Free: OK");
        }
        return true;
    }

    RS_DEBUG->print("QG_ActionHandler::command: %s", cmd.toLatin1().data());
    const QString c = cmd.toLower().trimmed();
    if (c == tr("escape", "escape, go back from action steps")) {
        m_view->back(Qt::KeyboardModifier::NoModifier);
        RS_DEBUG->print("QG_ActionHandler::command: back");
        return true;
    }

    // fixme - visual snap - command to clean/remove last?

    // pass command on to running action:
    RS_CommandEvent commandEvent(cmd);

    RS_DEBUG->print("QG_ActionHandler::command: trigger command event in graphic view");
    m_view->commandEvent(&commandEvent);

    // if the current action can't deal with the command,
    //   it might be intended to launch a new command
    //    std::cout<<"QG_ActionHandler::command(): e.isAccepted()="<<e.isAccepted()<<std::endl;
    if (!commandEvent.isAccepted()) {
        RS_DEBUG->print("QG_ActionHandler::command: convert cmd to action type");
        // command for new action:
        const RS2::ActionType type = RS_COMMANDS->cmdToAction(cmd);
        if (type != RS2::ActionNone) {
            RS_DEBUG->print("QG_ActionHandler::command: setting current action");
            //special handling, currently needed for snap actions
            if (!m_snapManager->tryToProcessSnapActions(type)) {
                //not handled yet
                setCurrentAction(type);
            }
            RS_DEBUG->print("QG_ActionHandler::command: current action set");
            return true;
        }
    }
    else {
        return true;
    }

    RS_DEBUG->print("QG_ActionHandler::command: current action not set");
    return false;
}

void QG_ActionHandler::setSnaps(const RS_SnapMode& s) const {
    m_snapManager->setSnaps(s);
}

// void QG_ActionHandler::slotSnapIntersectionManual() {
//disableSnaps();
/*if (snapIntersectionManual) {
    snapIntersectionManual->setChecked(true);
}*/
/*if (snapToolBar) {
    snapToolBar->setSnapMode(RS2::SnapIntersectionManual);
}*/
//setCurrentAction(RS2::ActionSnapIntersectionManual);
// }

// fixme - sand - files - rework snap middle value, it's not consistent with other actions !!!!
void QG_ActionHandler::slotSnapMiddleManual() const {
    setCurrentAction(RS2::ActionSnapMiddleManual);
}

void QG_ActionHandler::slotSetRelativeZero() const {
    setCurrentAction(RS2::ActionSetRelativeZero);
}

void QG_ActionHandler::slotLockRelativeZero(const bool on) const {
    m_snapManager->setRelativeZeroLock(on);
    // calling view directly instead of action to ensure that button for action will not be unchecked after action init/finish
    m_view->getViewPort()->lockRelativeZero(on);
}

void QG_ActionHandler::setDocumentAndView(RS_Document* doc, RS_GraphicView* graphicView) {
    m_actionContext->setDocumentAndView(doc, graphicView);
    if (m_snapManager != nullptr) {
        m_snapManager->setGraphicView(graphicView);
    }
    m_view = graphicView;
    m_document = doc;
}
