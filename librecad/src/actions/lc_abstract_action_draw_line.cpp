//
// Created by sand1 on 14/03/2024.
//


#include <rs_point.h>
#include <QMouseEvent>
#include "rs_commands.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_coordinateevent.h"
#include "rs_commandevent.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "lc_abstract_action_draw_line.h"
#include "lc_abstractactionwithpreview.h"
#include "lc_linemath.h"

LC_AbstractActionDrawLine::LC_AbstractActionDrawLine(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView)
     :LC_AbstractActionWithPreview(name, container, graphicView){
}

LC_AbstractActionDrawLine::~LC_AbstractActionDrawLine(){
}

void LC_AbstractActionDrawLine::setNewStartPointState(){
    if (mayStart()){
        setStatus(SetStartPoint);
    }
    else{
        commandMessageTR("Start point may set in distance or point state only");
    }
}

int LC_AbstractActionDrawLine::doRelZeroInitialSnapState(){
    return SetStartPoint;
}

void LC_AbstractActionDrawLine::doRelZeroInitialSnap(RS_Vector relZero){
    doSetStartPoint(relZero);
}

RS_Vector LC_AbstractActionDrawLine::doGetMouseSnapPoint(QMouseEvent *e, bool shiftPressed){
    RS_Vector snapped = snapPoint(e);
    // Snapping to angle(15*) if shift key is pressed
    if (shiftPressed){
        snapped = snapToAngle(snapped, getStartPointForAngleSnap());
    }
    return snapped;
}

bool LC_AbstractActionDrawLine::doCheckMayDrawPreview(QMouseEvent *pEvent, int status){
    return isStartPointValid();
}


bool LC_AbstractActionDrawLine::doProcessCommand(RS_CommandEvent *e, const QString &c){
    bool accept = true;
    if (checkCommand("help", c)){
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
    }
    // line by X coordinate
    else if (checkCommand("x", c)){
        if (isStartPointValid()){
           setSetXDirectionState();
        }
        else{
            commandMessageTR("Select start point first to set an angle.");
        }
    }
        // line by Y coordinate
    else if (checkCommand("y", c)){
        if (isStartPointValid()){
           setSetYDirectionState();
        }
        else{
            commandMessageTR("Select start point first to set an angle.");
        }
    }
        // line to arbitrary point
    else if (checkCommand("p", c)){
        if (isStartPointValid()){
            setSetPointDirectionState();
        }
        else{
            commandMessageTR("Select start point first to set an angle.");
        }
    }
        // line to angle
    else if (checkCommand("angle", c)){
        if (isStartPointValid()){
           setSetAngleState(false);
        }
        else{
            commandMessageTR("Select start point first to set an angle.");
        }
    }
    else if (doProceedCommand(e, c)){
    }
    else if (doProcessCommandValue(e, c)){
    }
    else{
        accept = false;
    }
    return accept;
}


bool LC_AbstractActionDrawLine::doProceedCommand(RS_CommandEvent *e, const QString &c){
    return false;
}

bool LC_AbstractActionDrawLine::doProcessCommandValue(RS_CommandEvent *e, const QString &c){
    return false;
}

void LC_AbstractActionDrawLine::setAngleIsRelative(bool value){
    angleIsRelative = value;
    updateOptions();
}

bool LC_AbstractActionDrawLine::isAngleRelative(){
    return angleIsRelative;
}

void LC_AbstractActionDrawLine::setSetAngleDirectionState(){
    direction = DIRECTION_ANGLE;
    setStatus(SetAngle);
    updateOptions();
}

void LC_AbstractActionDrawLine::setSetPointDirectionState(){
    direction = DIRECTION_POINT;
    setStatus(SetPoint);
    updateOptions();
}

void LC_AbstractActionDrawLine::setSetAngleState(bool relative){
    direction = DIRECTION_ANGLE;
    angleIsRelative = relative;
    setStatus(SetAngle);
    updateOptions();
}

void LC_AbstractActionDrawLine::setSetXDirectionState(){
    direction = DIRECTION_X;
    setStatus(SetDistance);
    updateOptions();
}

void LC_AbstractActionDrawLine::setSetYDirectionState(){
    direction = DIRECTION_Y;
    setStatus(SetDistance);
    updateOptions();
}

void LC_AbstractActionDrawLine::setAngleValue(double value){
    angleValue = value;
    if (getStatus() == SetAngle){
        setStatus(SetDistance);
    }
    updateOptions();
}

double LC_AbstractActionDrawLine::getAngleValue(){
    return angleValue;
}

bool LC_AbstractActionDrawLine::mayStart(){
    return true;
}

bool LC_AbstractActionDrawLine::processAngleValueInput(RS_CommandEvent *e, const QString &c){
    bool ok = false;
    double value = RS_Math::eval(c, &ok);
    if (ok){
        setAngleValue(value);
        value = LC_LineMath::getMeaningfulAngle(value);
        // ask for distance after angle entering
        setStatus(SetDistance);
    }
    return ok;
}

void LC_AbstractActionDrawLine::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapped, bool shiftPressed){
    onCoordinateEvent(snapped, false, status);
}

bool LC_AbstractActionDrawLine::isStartPointValid() const{
    return false;
}



