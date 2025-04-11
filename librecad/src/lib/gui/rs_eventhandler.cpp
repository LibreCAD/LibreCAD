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

#include <QAction>
#include <QMouseEvent>
#include <QRegularExpression>

#include "lc_convert.h"
#include "rs_actioninterface.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_eventhandler.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_snapper.h"
#include "rs_units.h"

namespace {
    bool isActive(const std::shared_ptr<RS_ActionInterface>& action) {
        return action != nullptr && !action->isFinished();
    }

    bool isInactive(const std::shared_ptr<RS_ActionInterface>& action) {
        return ! isActive(action);
    }

    QString evaluateFraction(QString input, QRegularExpression rx, int index, int tailI) {

        QString copy = input;
        QString tail = QString{R"(\)"} + QString::number(tailI);
        QRegularExpressionMatch match = rx.match(copy);

        if (match.hasMatch()) {
            qsizetype pos = match.capturedStart();
            LC_ERR << "Evaluate: " << copy;
            QString formula = ((index != 2) ? match.captured(2) + "+" : QString{}) + match.captured(index) + "/" +
                              match.captured(index + 1);
            LC_ERR << "formula=" << formula;
            QString value = QString{}.setNum(RS_Math::eval(formula), 'g', 10);
            LC_ERR << "formula=" << formula << ": value=" << value;
            return input.left(pos)
                   + input.mid(pos, match.capturedLength()).replace(rx, R"( \1)" + value + tail)
                   + evaluateFraction(input.right(input.size() - pos - match.capturedLength()), rx, index, tailI);
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
        QRegularExpression rx{R"((\D*)([\d]+)\s*(['"])([\d]+)/([\d]+)\s*$)"};
        QRegularExpressionMatch match = rx.match(input);
        if (match.hasMatch()) {
            qsizetype pos = match.capturedStart();
            input = input.left(pos) + match.captured(1) + match.captured(2) + " " + match.captured(4) + "/" +
                    match.captured(5) + match.captured(3);
        }
        std::vector<std::tuple<QRegularExpression, int, int>> regexps{
            {{QRegularExpression{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*([\D$]))"},3, 5},
                {QRegularExpression{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*(['"]))"},3, 5},
                {QRegularExpression{R"((\D*)\s*([\d]+)/([\d]+)\s*([\D$]))"},2, 4},}};
        LC_LOG << "input=" << input;
        for (auto &[rx, index, tailI]: regexps)
            input = evaluateFraction(input, rx, index, tailI).replace(QRegularExpression(R"(\s+)"), QString{});
        LC_LOG << "eval: " << input;
        return input;
    }
}

/**
 * Constructor.
 */
RS_EventHandler::RS_EventHandler(RS_GraphicView *parent):QObject(parent), graphicView{parent} {
    connect(parent, &RS_GraphicView::relativeZeroChanged,this, &RS_EventHandler::setRelativeZero);
}

/**
 * Destructor.
 */
RS_EventHandler::~RS_EventHandler() {
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler");
    defaultAction.reset();

    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..");
    currentActions.clear();
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: Deleting all actions..: OK");
    RS_DEBUG->print("RS_EventHandler::~RS_EventHandler: OK");
}


/**
 * Go back in current action.
 */
void RS_EventHandler::back() {
    QMouseEvent e(QEvent::MouseButtonRelease, QPoint(0,0), QPoint{0, 0},
                  Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    mouseReleaseEvent(&e);
    if (!hasAction() && q_action) {
        q_action->setChecked(false);
        q_action = nullptr;
    }
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
        currentActions.last()->mousePressEvent(e);
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
    if(hasAction()){
        //    if (actionIndex>=0 && currentActions[actionIndex] &&
        //            !currentActions[actionIndex]->isFinished()) {
        std::shared_ptr<RS_ActionInterface> &lastAction = currentActions.last();
        LC_ERR<< "call action "<< lastAction->getName();

        lastAction->mouseReleaseEvent(e);

        // action may be completed by click. Check this and if it so, uncheck the action
        checkLastActionCompletedAndUncheckQAction(lastAction);
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

void RS_EventHandler::checkLastActionCompletedAndUncheckQAction(const std::shared_ptr<RS_ActionInterface> &lastAction) {
    int lastActionStatus = lastAction->getStatus();
    if (lastActionStatus < 0){
        if (!hasAction() && q_action){
            q_action->setChecked(false);
            q_action = nullptr;
        }
    }
}

/**
 * Called by QG_GraphicView
 */
void RS_EventHandler::mouseMoveEvent(QMouseEvent* e){
    if(hasAction()) {
        std::shared_ptr<RS_ActionInterface> &lastAction = currentActions.last();
        lastAction->mouseMoveEvent(e);
        checkLastActionCompletedAndUncheckQAction(lastAction);
        cleanUp();
        e->accept();
    }
    else if (defaultAction) {
        defaultAction->mouseMoveEvent(e);
    }
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
//        LC_ERR<<__func__<<"(): resume: "<<currentActions.last()->getName();
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
        std::shared_ptr<RS_ActionInterface> &lastAction = currentActions.last();
        lastAction->keyPressEvent(e);
        checkLastActionCompletedAndUncheckQAction(lastAction);
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
        currentActions.last()->keyReleaseEvent(e);
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

    if (coordinateInputEnabled) {
        if (!e->isAccepted()) {
            if(hasAction()){
                // handle quick shortcuts for absolute/current origins:
                if (cmd.length() == 1) {
                    switch (cmd[0].toLatin1()) {
                        case '0': {
                            RS_Vector ucs0 = RS_Vector(0,0,0);
                            RS_Vector wcs0 = toWCS(ucs0);
                            RS_CoordinateEvent ce(wcs0, true, false);
                            currentActions.last()->coordinateEvent(&ce);
                            e->accept();
                            break;
                        }
                        case '.':
                        case ',':{
                            RS_Vector wcs0 = relative_zero;
                            RS_CoordinateEvent ce(wcs0, false, true);
                            currentActions.last()->coordinateEvent(&ce);
                            e->accept();
                            break;
                        }
                        default: /* NO OP */
                            break;
                    }
                }

                bool wcsCoordinates = cmd.at(0) == '!';
                if (wcsCoordinates){ // proceed absolute wcs coordinates
                    bool isCartesian = cmd.contains(',');
                    cmd = cmd.mid(1);
                    if (isCartesian) {
                        int separatorPos = cmd.indexOf(',');
                        bool ok1, ok2;
                        double x = RS_Math::eval(updateForFraction(cmd.left(separatorPos)), &ok1);
                        double y = RS_Math::eval(updateForFraction(cmd.mid(separatorPos + 1)), &ok2);
                        if (ok1 && ok2) {
                            const RS_Vector &wcsPosition = RS_Vector(x, y);
                            RS_CoordinateEvent ce(wcsPosition);
                            currentActions.last()->coordinateEvent(&ce);
                        } else {
                            RS_DIALOGFACTORY->commandMessage("Expression Syntax Error");
                        }
                        e->accept();
                    }
                    else{
                        bool isPolar = cmd.contains('<');
                        if (isPolar) {  // proceed absolute polar coordinates
                            int separatorPos = cmd.indexOf('<');
                            bool ok1, ok2;
                            double r = RS_Math::eval(updateForFraction(cmd.left(separatorPos)), &ok1);
                            const QString &angleStr = cmd.mid(separatorPos + 1);
                            double angleDegrees = evalAngleValue(angleStr, ok2);
                            if (ok1 && ok2) {
                                double wcsAngle = RS_Math::deg2rad(angleDegrees);
                                RS_Vector wcsPos = RS_Vector(r, wcsAngle);
                                RS_CoordinateEvent ce(wcsPos);
                                currentActions.last()->coordinateEvent(&ce);
                            } else {
                                RS_DIALOGFACTORY->commandMessage("Expression Syntax Error");
                            }
                            e->accept();
                        }
                        else{
                            RS_DIALOGFACTORY->commandMessage("Expression Syntax Error");
                            e->accept();
                        }
                    }
                }
                else {
                    bool absoluteCoordinates = cmd.at(0) != '@';
                    if (!e->isAccepted()) {
                        bool isCartesian = cmd.contains(',');
                        if (isCartesian) {
                            int separatorPos = cmd.indexOf(',');
                            if (absoluteCoordinates) { // absolute cartesian coordinates
                                bool ok1, ok2;
                                double x = RS_Math::eval(updateForFraction(cmd.left(separatorPos)), &ok1);
                                double y = RS_Math::eval(updateForFraction(cmd.mid(separatorPos + 1)), &ok2);
                                if (ok1 && ok2) {
                                    const RS_Vector &ucsPosition = RS_Vector(x, y);
                                    RS_Vector wcsPosition = toWCS(ucsPosition);
                                    RS_CoordinateEvent ce(wcsPosition);
                                    currentActions.last()->coordinateEvent(&ce);
                                } else {
                                    RS_DIALOGFACTORY->commandMessage("Expression Syntax Error");
                                }
                                e->accept();
                            } else { // relative cartesian coordinates
                                bool ok1, ok2;
                                double x = RS_Math::eval(updateForFraction(cmd.mid(1, separatorPos - 1)), &ok1);
                                double y = RS_Math::eval(updateForFraction(cmd.mid(separatorPos + 1)), &ok2);

                                if (ok1 && ok2) {
                                    const RS_Vector &ucsOffset = RS_Vector(x, y);
                                    const RS_Vector ucsRelZero = toUCS(relative_zero);
                                    const RS_Vector ucsPosition = ucsOffset + ucsRelZero;
                                    const RS_Vector &wcsPosition = toWCS(ucsPosition);
                                    RS_CoordinateEvent ce(wcsPosition);
                                    currentActions.last()->coordinateEvent(&ce);
                                } else {
                                    RS_DIALOGFACTORY->commandMessage("Expression Syntax Error");
                                }
                                e->accept();
                            }
                        } else { // try to handle polar coordinate input:
                            bool isPolar = cmd.contains('<');
                            if (isPolar) {
                                int separatorPos = cmd.indexOf('<');
                                if (absoluteCoordinates) { // handle absolute polar coordinate input:
                                    bool ok1, ok2;
                                    double ucsR = RS_Math::eval(updateForFraction(cmd.left(separatorPos)), &ok1);
                                    const QString &angleStr = cmd.mid(separatorPos + 1);
                                    double ucsBasisAngleDegrees = evalAngleValue(angleStr, ok2);

                                    if (ok1 && ok2) {
                                        double ucsBasisAngleRad = RS_Math::deg2rad(ucsBasisAngleDegrees);
                                        double ucsAngle = toAbsUCSAngle(ucsBasisAngleRad);
                                        RS_Vector ucsPos{RS_Vector::polar(ucsR, ucsAngle)};
                                        RS_Vector wcsPos = toWCS(ucsPos);
                                        RS_CoordinateEvent ce(wcsPos);
                                        currentActions.last()->coordinateEvent(&ce);
                                    } else {
                                        RS_DIALOGFACTORY->commandMessage("Expression Syntax Error");
                                    }
                                    e->accept();
                                } else { // handle relative polar coordinate input:
                                    int commaPos = cmd.indexOf('<');
                                    bool ok1, ok2;
                                    double r = RS_Math::eval(updateForFraction(cmd.mid(1, commaPos - 1)), &ok1);
                                    const QString &angleStr = cmd.mid(commaPos + 1);
                                    double ucsBasisAngleDegrees = evalAngleValue(angleStr, ok2);

                                    if (ok1 && ok2) {
                                        double ucsBasisAngleRad = RS_Math::deg2rad(ucsBasisAngleDegrees);
                                        double ucsAngle = toAbsUCSAngle(ucsBasisAngleRad);
                                        RS_Vector ucsOffset = RS_Vector::polar(r, ucsAngle);
                                        const RS_Vector ucsRelZero = toUCS(relative_zero);
                                        const RS_Vector ucsPosition = ucsOffset + ucsRelZero;
                                        const RS_Vector &wcsPosition = toWCS(ucsPosition);
                                        RS_CoordinateEvent ce(wcsPosition);
                                        currentActions.last()->coordinateEvent(&ce);
                                    } else {
                                        RS_DIALOGFACTORY->commandMessage("Expression Syntax Error");
                                    }
                                    e->accept();
                                }
                            }
                        }
                    }
                }
                // send command event directly to current action:
                if (!e->isAccepted()) {
//                    std::cout<<"RS_EventHandler::commandEvent(RS_CommandEvent* e): sending cmd("<<qPrintable(e->getCommand()) <<") to action: "<<currentActions.last()->rtti()<<std::endl;
                    currentActions.last()->commandEvent(e);
                }
            }else{
                //send the command to default action
                if (defaultAction) {
                    defaultAction->commandEvent(e);
                }
            }
            // do not accept command here. Actions themselves should be responsible to accept commands
//            e->accept();
        }
    }

    RS_DEBUG->print("RS_EventHandler::commandEvent: OK");
}

double RS_EventHandler::evalAngleValue(const QString &angleStr, bool &ok2) const {
    double angleDegrees;
    ok2 = LC_Convert::parseToToDoubleAngleDegrees(angleStr, angleDegrees, 0.0, false);
    return angleDegrees;
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
void RS_EventHandler::setCurrentAction(std::shared_ptr<RS_ActionInterface> action) {
    RS_DEBUG->print("RS_EventHandler::setCurrentAction");
    if (action==nullptr) {
        return;
    }
    // Do not initialize action if it's already the last one.
    // This is attempt to fix crashes of dialogs (like properties) which are called from actions
    // todo - check again, either remove or uncomment
//    if (hasAction() && currentActions.last().get() == action){
//        return;
//    }

    LC_LOG<<"RS_EventHandler::setCurrentAction " << action->getName();
    // Predecessor of the new action or NULL:
    auto& predecessor = hasAction() ? currentActions.last() : defaultAction;
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
    currentActions.push_back(action);
    //    RS_DEBUG->print("RS_EventHandler::setCurrentAction: current action is: %s -> %s",
    //                    predecessor->getName().toLatin1().data(),
    //                    currentActions.last()->getName().toLatin1().data());

    // Initialisation of our new action:
    RS_DEBUG->print("RS_EventHandler::setCurrentAction: init current action");
    action->init(0);
    // ## new:
    if (!action->isFinished()) {
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: show options");
        action->showOptions();
        RS_DEBUG->print("RS_EventHandler::setCurrentAction: set predecessor");
        action->setPredecessor(predecessor.get());
    }

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: cleaning up..");
    cleanUp();

    RS_DEBUG->print("RS_EventHandler::setCurrentAction: debugging actions");
    debugActions();
    RS_DEBUG->print("RS_GraphicView::setCurrentAction: OK");
    // For some actions: action->init() may call finish() within init()
    // If so, the q_action shouldn't be checked
    if (q_action){
        bool hasActionToCheck = hasAction();
        q_action->setChecked(hasActionToCheck);
    }
}

/**
 * Kills all running selection actions. Called when a selection action
 * is launched to reduce confusion.
 */
void RS_EventHandler::killSelectActions() {

    for (auto it=currentActions.begin();it != currentActions.end();){
        RS2::ActionType rtti = (*it)->rtti();
        if (rtti == RS2::ActionSelectSingle ||
            rtti == RS2::ActionSelectContour ||
            rtti == RS2::ActionSelectWindow ||
            rtti == RS2::ActionSelectIntersected ||
            rtti == RS2::ActionSelectLayer) {
            if (isActive(*it)) {
                (*it)->finish();
            }
            it= currentActions.erase(it);
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

    if (q_action)    {
        q_action->setChecked(false);
        q_action = nullptr;
    }

    for(auto& p: currentActions){
        if (isActive(p))
        {
            p->finish();
        }
    }
    currentActions.clear();

    if (!defaultAction->isFinished()) {
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
bool RS_EventHandler::hasAction(){
    auto it = std::remove_if(currentActions.begin(), currentActions.end(), isInactive);
    currentActions.erase(it, currentActions.end());
    return !currentActions.empty();
}

/**
 * Garbage collector for actions.
 */
void RS_EventHandler::cleanUp() {
    RS_DEBUG->print("RS_EventHandler::cleanUp");

    if (hasAction()) {
        currentActions.last()->resume();
        currentActions.last()->showOptions();
    } else {
        if (defaultAction) {
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

QAction* RS_EventHandler::getQAction(){
  return q_action;
}

void RS_EventHandler::setQAction(QAction *action) {
//    LC_ERR << __func__ << "()";
    debugActions();
    if (q_action) {
        q_action->setChecked(false);
        killAllActions();
    }
    q_action = action;
}

void RS_EventHandler::setRelativeZero(const RS_Vector &point) {
    relative_zero = point;
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

RS_Vector RS_EventHandler::toWCS(const RS_Vector &ucs) {
    return graphicView->getViewPort()->toWorld(ucs);
}

RS_Vector RS_EventHandler::toUCS(const RS_Vector &wcs) {
    return graphicView->getViewPort()->toUCS(wcs);
}

double RS_EventHandler::toAbsUCSAngle(double ucsBasisAngle) {
    return graphicView->getViewPort()->toAbsUCSAngle(ucsBasisAngle);
}

double RS_EventHandler::toWCSAngle(double ucsAngle) {
    return graphicView->getViewPort()->toWorldAngle(ucsAngle);
}
