/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "rs_point.h"
#include "rs_math.h"
#include "rs_graphicview.h"
#include "lc_abstractactiondrawline.h"
#include "lc_abstractactionwithpreview.h"
#include "lc_linemath.h"

LC_AbstractActionDrawLine::LC_AbstractActionDrawLine(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView)
     :LC_AbstractActionWithPreview(name, container, graphicView){
}

LC_AbstractActionDrawLine::~LC_AbstractActionDrawLine()= default;


/**
 * Setting new line start point. Checks whether we're in state that allows to set new start point and changes the action state.
 */
void LC_AbstractActionDrawLine::setNewStartPointState(){
    if (mayStart()){
        setStatus(SetStartPoint);
    }
    else{
        commandMessage(tr("Start point may set in distance or point state only"));
    }
}

/**
 * Extension point that controls whether we may start new line with new start point
 * @return true if setting new start point is allowed
 */
bool LC_AbstractActionDrawLine::mayStart(){
    return true;
}
/**
 * We may pre-snap to relative zero if we're in SetStartPoint state
 * @return
 */
int LC_AbstractActionDrawLine::doGetStatusForInitialSnapToRelativeZero(){
    return SetStartPoint;
}

/**
 * Do pre-snap to relative zero
 * @param relZero
 */
void LC_AbstractActionDrawLine::doInitialSnapToRelativeZero(RS_Vector relZero){
    doSetStartPoint(relZero);
}

/**
 * Calculate snap point for mouse move. If SHIFT is pressed, we'll use snap to angle (15* degrees), otherwise it will be normal snap point
 * @param e event
 * @return point for snap
 */
RS_Vector LC_AbstractActionDrawLine::doGetMouseSnapPoint(QMouseEvent *e){
    RS_Vector snapped = snapPoint(e);
    if (direction == DIRECTION_POINT || direction == DIRECTION_NONE){
        // Snapping to angle(15*) if shift key is pressed
        snapped = getSnapAngleAwarePoint(e, getStartPointForAngleSnap(), snapped, isMouseMove(e));
    }
    return snapped;
}

/**
 * We may start drawing preview if there is at least start point set
 * @param pEvent
 * @param status
 * @return
 */
bool LC_AbstractActionDrawLine::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *pEvent, [[maybe_unused]]int status){
    return isStartPointValid();
}

/**
 * Common line-related commands processing
 * @param e
 * @param c
 * @return
 */
bool LC_AbstractActionDrawLine::doProcessCommand(int status, const QString &c){
    bool accept = true;

    // line by X coordinate
    if (checkCommand("x", c)){
       setSetXDirectionState();
    }
    // line by Y coordinate
    else if (checkCommand("y", c)){
       setSetYDirectionState();
    }
    // line to arbitrary point
    else if (checkCommand("p", c)){
       setSetPointDirectionState();
    }
    // line to angle
    else if (checkCommand("angle", c)){
        setSetAngleState(false);
    }
    else if (doProceedCommand(status, c)){ // delegate other commands to inherited actions
       // intentionally does nothing, processing is withing method call
    }
    else if (doProcessCommandValue(status, c)){ // if we're here, it means that this is some input value - delegate it to inherited action
        // intentionally does nothing
    }
    else{
        accept = false;
    }
    return accept;
}

/**
 * Extension points for inherited actions for processing specific commands
 * @param e event
 * @param c command string
 * @return true if command is processed, false if not and further processing is needed (or command invalid)
 */
bool LC_AbstractActionDrawLine::doProceedCommand([[maybe_unused]]int status, [[maybe_unused]]const QString &c){
    return false;
}

/**
 * Extension point for inherited actions for processing of some values (like length or so).
 * @param e event
 * @param c command input
 * @return true if value is processed, false if value in not processed (invalid input or so)
 */
bool LC_AbstractActionDrawLine::doProcessCommandValue([[maybe_unused]]int status, [[maybe_unused]]const QString &c){
    return false;
}

/**
 * Utility method for setting mode that angle is invalid
 * @param value
 */
void LC_AbstractActionDrawLine::setAngleIsRelative(bool value){
    angleIsRelative = value;
    updateOptions();
}

/**
 * Return whether specified angle is relative
 * @return
 */
bool LC_AbstractActionDrawLine::isAngleRelative() const{
    return angleIsRelative;
}

/**
 * Sets action drawing state to drawing with angle mode
 */
void LC_AbstractActionDrawLine::setSetAngleDirectionState(){
    direction = DIRECTION_ANGLE;
    setStatusForValidStartPoint(SetAngle);
    updateOptions();
}

/**
* Sets action drawing state to drawing to point mode
*/
void LC_AbstractActionDrawLine::setSetPointDirectionState(){
    direction = DIRECTION_POINT;
    setStatusForValidStartPoint(SetPoint);
    updateOptions();
}

/**
 * Set relative angle drawing mode
 * @param relative
 */
void LC_AbstractActionDrawLine::setSetAngleState(bool relative){
    direction = DIRECTION_ANGLE;
    angleIsRelative = relative;
    setStatusForValidStartPoint(SetAngle);
    updateOptions();
}

/**
 * Sets drawing mode to X state
 */
void LC_AbstractActionDrawLine::setSetXDirectionState(){
    direction = DIRECTION_X;
    setStatusForValidStartPoint(SetDistance);
    updateOptions();
}

/**
 * Sets drawing mode to Y state
 */
void LC_AbstractActionDrawLine::setSetYDirectionState(){
    direction = DIRECTION_Y;
    setStatusForValidStartPoint(SetDistance);
    updateOptions();
}

/**
 * Sets angle value and switch to SetDistance state
 */
void LC_AbstractActionDrawLine::setAngleValue(double value){
    angle = value;
    if (getStatus() == SetAngle){
        setStatusForValidStartPoint(SetDistance);
    }
    updateOptions();
}

void LC_AbstractActionDrawLine::setStatusForValidStartPoint(int newStatus){
    if (isStartPointValid()){
        setStatus(newStatus);
    }
    else{
        setStatus(SetStartPoint);
    }
}

double LC_AbstractActionDrawLine::getAngle() const{
    return angle;
}

bool LC_AbstractActionDrawLine::processAngleValueInput(const QString &c){
    bool ok = false;
    double value = RS_Math::eval(c, &ok);
    if (ok){
        value = LC_LineMath::getMeaningfulAngle(value);
        setAngleValue(value);
        // ask for distance after angle entering
        setStatus(SetDistance);
    }
    return ok;
}

void LC_AbstractActionDrawLine::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status, const RS_Vector &snapped){
    onCoordinateEvent(status,  false, snapped);
}

bool LC_AbstractActionDrawLine::isStartPointValid() const{
    return false;
}
