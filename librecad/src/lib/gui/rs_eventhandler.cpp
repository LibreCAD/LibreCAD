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

#include <algorithm>
#include <QRegExp>
#include <QAction>
#include <QMouseEvent>
#include "rs_eventhandler.h"
#include "rs_actioninterface.h"
#include "rs_dialogfactory.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_snapper.h"
#include "rs_debug.h"

namespace {
    bool isActive(const std::shared_ptr<RS_ActionInterface>& action) {
        return action != nullptr && !action->isFinished();
    }

    bool isInactive(const std::shared_ptr<RS_ActionInterface>& action) {
        return action == nullptr || action->isFinished();
    }

    QString evaluateFraction(QString input, QRegExp rx, int index, int tailI)
    {
        QString copy = input;
        QString tail =QString{R"(\)"} + QString::number(tailI);

        int pos = 0;
        if ((pos = rx.indexIn(copy, pos)) != -1) {
            LC_ERR<<"Evaluate: "<<copy;
            LC_ERR<<"pos="<<pos<<", rx.matchedLength()="<<rx.matchedLength();
            QString formula = ((index != 2) ? rx.cap(2) + "+" : QString{}) + rx.cap(index) + "/" + rx.cap(index + 1);
            LC_ERR<<"formula="<<formula;
            QString value = QString{}.setNum(RS_Math::eval(formula), 'g', 10);
            LC_ERR<<"formula="<<formula<<": value="<<value;
            return input.left(pos)
                    + input.mid(pos, rx.matchedLength()).replace(rx, R"( \1)" + value + tail)
                    + evaluateFraction(input.right(input.size() - pos - rx.matchedLength()), rx, index, tailI);
        }
        return input;
    }

    /**
     * @{description}       Update a length string to support fraction
     *                      (1 1/2") to (1+1/2")
     *                      (1"1/2) to (1+1/2")
    */
    QString updateForFraction(QString input) {
        // if the expression is already valid, bypass fraction processing
        bool okay = false;
        double value = RS_Math::eval(input, &okay);
        if (okay)
            return QString{}.setNum(value, 'g', 10);

        // support fraction at the end: (1'1/2) => (1 1/2')
        QRegExp rx{R"((\D*)([\d]+)\s*(['"])([\d]+)/([\d]+)\s*$)"};
        int pos = 0;
        if ((pos = rx.indexIn(input, pos)) != -1) {
            input = input.left(pos) + rx.cap(1) + rx.cap(2) + " " + rx.cap(4) + "/" + rx.cap(5) + rx.cap(3);
        }
        std::vector<std::tuple<QRegExp, int, int>> regexps{{
                {QRegExp{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*([\D$]))"}, 3, 5},
                {QRegExp{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*(['"]))"}, 3, 5},
                {QRegExp{R"((\D*)\s*([\d]+)/([\d]+)\s*([\D$]))"}, 2, 4},
            }};
        LC_LOG<<"input="<<input;
        for(auto& [rx, index, tailI] : regexps)
            input = evaluateFraction(input, rx, index, tailI).replace(QRegExp(R"(\s+)"), QString{});
        LC_LOG<<"eval: "<<input;
        return input;
    }
}

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
    setQActionStateActive(false);
    defaultAction.reset();

    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..");
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
        const auto actions = currentActions;
        actions.last()->mousePressEvent(e);
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
    if (hasAction()) {
        const auto actions = currentActions;
        const auto& current = actions.last();
        LC_LOG<< "call action "<< current->getName();

        current->mouseReleaseEvent(e);

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
    if(hasAction()) {
        const auto actions = currentActions;
        actions.last()->mouseMoveEvent(e);
        cleanUp();
    }

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
        cleanUp();
        debugActions();
        LC_LOG<<__func__<<"(): resume: "<<currentActions.last()->getName();
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
        const auto actions = currentActions;
        actions.last()->keyPressEvent(e);
        cleanUp();
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
        const auto actions = currentActions;
        actions.last()->keyReleaseEvent(e);
        cleanUp();
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
    // Actions may replace themselves while handling a command. Keep the old
    // stack alive until all nested callbacks have returned.
    const auto actionGuards = currentActions;
    Q_UNUSED(actionGuards)

    if (coordinateInputEnabled) {
        if (!e->isAccepted()) {

                // send command event directly to current action:
                if (hasAction()) {
                    LC_LOG<<"RS_EventHandler::commandEvent(RS_CommandEvent* e): sending cmd("<<qPrintable(e->getCommand()) <<") to action: "<<currentActions.last()->rtti();
                    const auto current = currentActions.last();
                    current->commandEvent(e);
                }

            if(hasAction() && !e->isAccepted()){
                // handle quick shortcuts for absolute/current origins:
                if (cmd.length() == 1) {
                    RS_Vector at = relative_zero;
                    switch (cmd[0].toLatin1()) {
                        case '0':
                        at.set(0,0);
                        [[fallthrough]];
                        case '.':
                        case ',':
                        {
                            RS_CoordinateEvent ce(at);
                            const auto current = currentActions.last();
                            current->coordinateEvent(&ce);
                            e->accept();
                            break;
                        }
                        default: /* NO OP */
                            break;
                    }
                }

                // handle absolute cartesian coordinate input:
                if (!e->isAccepted() && cmd.contains(',') && cmd.at(0)!='@') {
                    int commaPos = cmd.indexOf(',');
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 001");
                    bool ok1 = false, ok2 = false;
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 002");
                    double x = RS_Math::eval(updateForFraction(cmd.left(commaPos)), &ok1);
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 003a");
                    double y = RS_Math::eval(updateForFraction(cmd.mid(commaPos+1)), &ok2);
                    RS_DEBUG->print("RS_EventHandler::commandEvent: 004");

                    if (ok1 && ok2) {
                        RS_DEBUG->print("RS_EventHandler::commandEvent: 005");
                        RS_CoordinateEvent ce(RS_Vector(x,y));
                        RS_DEBUG->print("RS_EventHandler::commandEvent: 006");
						const auto current = currentActions.last();
						current->coordinateEvent(&ce);
					} else
						RS_DIALOGFACTORY->commandMessage(
									"Expression Syntax Error");
					e->accept();
                }

                // handle relative cartesian coordinate input:
                if (!e->isAccepted()) {
                    if (cmd.contains(',') && cmd.at(0)=='@') {
                        int commaPos = cmd.indexOf(',');
                        bool ok1 = false, ok2 = false;
                        double x = RS_Math::eval(updateForFraction(cmd.mid(1, commaPos-1)), &ok1);
                        double y = RS_Math::eval(updateForFraction(cmd.mid(commaPos+1)), &ok2);

                        if (ok1 && ok2) {
                            RS_CoordinateEvent ce(RS_Vector(x,y) + relative_zero);

                            const auto current = currentActions.last();
                            current->coordinateEvent(&ce);
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
                        bool ok1 = false, ok2 = false;
                        double r = RS_Math::eval(updateForFraction(cmd.left(commaPos)), &ok1);
                        double a = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

                        if (ok1 && ok2) {
							RS_Vector pos{
								RS_Vector::polar(r,RS_Math::deg2rad(a))};
                            RS_CoordinateEvent ce(pos);
                            const auto current = currentActions.last();
                            current->coordinateEvent(&ce);
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
                        bool ok1 = false, ok2 = false;
                        double r = RS_Math::eval(updateForFraction(cmd.mid(1, commaPos-1)), &ok1);
                        double a = RS_Math::eval(cmd.mid(commaPos+1), &ok2);

						if (ok1 && ok2) {
							RS_Vector pos = RS_Vector::polar(r,RS_Math::deg2rad(a));
                            RS_CoordinateEvent ce(pos + relative_zero);
                            const auto current = currentActions.last();
                            current->coordinateEvent(&ce);
						} else
                                RS_DIALOGFACTORY->commandMessage(
                                            "Expression Syntax Error");
                        e->accept();
                    }
                }

            }else{
            //send the command to default action
                if (defaultAction) {
                    defaultAction->commandEvent(e);
                }
            }
        }
    }

    cleanUp();

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
        return currentActions.last().get();
    } else {
        return defaultAction.get();
    }
}



/**
 * @return The current default action.
 */
RS_ActionInterface* RS_EventHandler::getDefaultAction() const{

    return defaultAction.get();
}



/**
 * Sets the default action.
 */
void RS_EventHandler::setDefaultAction(RS_ActionInterface* action) {
    if (defaultAction) {
        defaultAction->finish();
        //        defaultAction = NULL;
    }

    defaultAction.reset(action);
}



/**
 * Sets the current action.
 */
void RS_EventHandler::setCurrentAction(RS_ActionInterface* action) {
    RS_DEBUG->print("RS_EventHandler::setCurrentAction");
    if (action==nullptr) {
        return;
    }

    RS_DEBUG->print("RS_EventHandler::setCurrentAction %s", action->getName().toLatin1().data());

    // RS_ActionSelect finishes and starts the modify action once the selection is
    // done, with its finished helper RS_ActionSelectSingle still on top of the
    // stack: the modify action inherits the QAction, so the button stays checked
    // until the whole job is done.
    if (m_pendingQAction == nullptr) {
        for (auto it = currentActions.rbegin(); it != currentActions.rend() && (*it)->isFinished(); ++it) {
            auto found = m_toQAction.find(it->get());
            if (found != m_toQAction.end() && (*it)->rtti() == RS2::ActionSelect) {
                m_pendingQAction = found->second;
                m_toQAction.erase(found);
                break;
            }
        }
    }

    // Predecessor of the new action or NULL:
    const std::shared_ptr<RS_ActionInterface> predecessor =
            hasAction() ? currentActions.last() : defaultAction;
    // Suspend current action:
    predecessor->suspend();
    predecessor->hideOptions();

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
    const auto currentAction = std::shared_ptr<RS_ActionInterface>(action);
    currentActions.push_back(currentAction);
    if (m_pendingQAction != nullptr) {
        m_toQAction[action] = m_pendingQAction;
        m_pendingQAction = nullptr;
    }
    //    RS_DEBUG->print("RS_EventHandler::setCurrentAction: current action is: %s -> %s",
    //                    predecessor->getName().toLatin1().data(),
    //                    currentActions.last()->getName().toLatin1().data());

    // Initialisation of our new action:
    RS_DEBUG->print("RS_EventHandler::setCurrentAction: init current action");
    currentAction->init();
    // ## new:
    if (!currentAction->isFinished()) {
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: show options");
        currentAction->showOptions();
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: set predecessor");
        currentAction->setPredecessor(predecessor.get());
    }

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: cleaning up..");
    cleanUp();

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: debugging actions");
    debugActions();
    RS_DEBUG->print("RS_GraphicView::setCurrentAction: OK");
    // For some actions: action->init() may call finish() within init()
    // If so, the QAction is released by cleanUp(), which also checks the
    // QAction of the action which is current now.
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
            if (isActive(*it)) {
                (*it)->finish();
            }
            unlinkQAction(it->get());
            it= currentActions.erase(it);
        }else{
            it++;
        }
    }
    updateQActions();
}



/**
 * Kills all running actions. Called when a window is closed.
 */
void RS_EventHandler::killAllActions()
{
    RS_DEBUG->print(__FILE__ ": %s: line %d: begin\n", __func__, __LINE__);

    for(auto& p: currentActions)
    {
        if (isActive(p))
        {
			p->finish();
		}
	}
    currentActions.clear();
    if (m_qActionStateActive) {
        for (const auto& link : m_toQAction)
            link.second->setChecked(false);
        if (m_pendingQAction != nullptr)
            m_pendingQAction->setChecked(false);
    }

    m_toQAction.clear();
    m_pendingQAction = nullptr;

    if (!defaultAction->isFinished())
    {
        defaultAction->finish();
    }

	RS_DEBUG->print(__FILE__ ": %s: line %d: begin\n", __func__, __LINE__);
	defaultAction->init(0);
}



/**
 * @return true if the action is within currentActions
 */
bool RS_EventHandler::isValid(RS_ActionInterface* action) const{
    return action != nullptr && std::any_of(currentActions.cbegin(), currentActions.cend(),
                       [action](const std::shared_ptr<RS_ActionInterface>& entry){
        return entry.get() == action;});
}

/**
 * @return true if there is at least one action in the action stack.
 */
bool RS_EventHandler::hasAction()
{
    for (const auto& action : currentActions) {
        if (isInactive(action)) {
            unlinkQAction(action.get());
        }
    }
    auto it = std::remove_if(currentActions.begin(), currentActions.end(), isInactive);
    currentActions.erase(it, currentActions.end());
    updateQActions();
    return !currentActions.empty();
}


/**
 * Garbage collector for actions.
 */
void RS_EventHandler::cleanUp() {
    RS_DEBUG->print("RS_EventHandler::cleanUp");

    const std::shared_ptr<RS_ActionInterface> previous = currentActions.empty()
            ? nullptr : currentActions.last();
    RS_ActionInterface* current = hasAction()
            ? currentActions.last().get() : nullptr;

    if (current != previous.get()) {
        if (current != nullptr) {
            current->resume();
            current->showOptions();
        } else if (defaultAction) {
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
    for(auto& a: currentActions){
        if(isActive(a)) {
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
    for(auto& a: currentActions){
        if(isActive(a)) {
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
    // The QAction is triggered before the action handler creates the action,
    // so the link is established by setCurrentAction()
    QAction* dropped = m_pendingQAction;
    m_pendingQAction = action;
    if (m_qActionStateActive && dropped != nullptr && dropped != action) {
        // no action was started for it: Qt toggled it when it was triggered,
        // give it and the QAction of the current action the state of the invariant back
        dropped->setChecked(dropped == currentQAction());
        updateQActions();
    }
}

QAction* RS_EventHandler::currentQAction() const
{
    if (m_pendingQAction != nullptr) {
        // about to be linked to the action started next
        return m_pendingQAction;
    }
    for (auto it = currentActions.rbegin(); it != currentActions.rend(); ++it) {
        auto found = m_toQAction.find(it->get());
        if (found != m_toQAction.end()) {
            return found->second;
        }
    }
    return nullptr;
}

void RS_EventHandler::updateQActions()
{
    if (!m_qActionStateActive) {
        return;
    }

    QAction* current = currentQAction();
    if (current != nullptr) {
        current->setChecked(true);
    }
    for (const auto& link : m_toQAction) {
        link.second->setChecked(link.second == current);
    }
}

void RS_EventHandler::setQActionStateActive(bool active)
{
    if (!active && m_qActionStateActive) {
        for (const auto& link : m_toQAction) {
            link.second->setChecked(false);
        }
        if (m_pendingQAction != nullptr) {
            m_pendingQAction->setChecked(false);
        }
    }

    m_qActionStateActive = active;
    updateQActions();
}

void RS_EventHandler::unlinkQAction(const RS_ActionInterface* action)
{
    auto it = m_toQAction.find(action);
    if (it == m_toQAction.end()) {
        return;
    }
    QAction* qAction = it->second;
    m_toQAction.erase(it);
    // the QAction stays checked if it is linked to another action, or if the user
    // triggered it again and it is about to be linked to the action started next
    const bool stillLinked = std::any_of(m_toQAction.begin(), m_toQAction.end(),
                                         [qAction](const auto& link) { return link.second == qAction; });
    if (m_qActionStateActive && !stillLinked && qAction != m_pendingQAction) {
        qAction->setChecked(false);
    }
    updateQActions();
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
