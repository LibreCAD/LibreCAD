/****************************************************************************
**
* Action that creates a set of lines, with support of angle and "snake" mode

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
**********************************************************************/

#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_math.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_actioneditundo.h"
#include "rs_commands.h"
#include "rs_actionpolylinesegment.h"
#include "lc_linemath.h"
#include "lc_abstractactiondrawline.h"
#include "lc_lineoptions.h"
#include "lc_actiondrawlinesnake.h"

LC_ActionDrawLineSnake::LC_ActionDrawLineSnake(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView,
    int initialDirection)
    :LC_AbstractActionDrawLine("Draw line snake",
                               container, graphicView)
    , pPoints(new Points{}){
    primaryDirection = initialDirection;
    direction = initialDirection;
    actionType = RS2::ActionDrawSnakeLine;
}

LC_ActionDrawLineSnake::~LC_ActionDrawLineSnake() = default;

size_t LC_ActionDrawLineSnake::Points::index(const int offset /*= 0*/){
    return static_cast<size_t>( std::max(0, historyIndex + offset));
}

void LC_ActionDrawLineSnake::init(int status){
    if (status >= 0){
        resetPoints();
        // restore primary direction of action
        direction = primaryDirection;
    }
    LC_AbstractActionWithPreview::init(status);
}

void LC_ActionDrawLineSnake::resetPoints(){
    pPoints.reset(new Points{});
}

/**
 * create line segment based on points data
 * @param list  list of entities to add created line
 */
void LC_ActionDrawLineSnake::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    auto *line = new RS_Line(container, pPoints->data);
    list << line;
}

void LC_ActionDrawLineSnake::doSetStartPoint(RS_Vector start){
    pPoints->startOffset = 0;
    pPoints->data.startpoint = start;
    addHistory(HA_SetStartpoint, start, start, pPoints->startOffset);

    //  adjust state and direction of action based on primary direction.
    // this is needef for horizontal/vertical line actions
    if (primaryDirection == DIRECTION_POINT){
        if (direction == DIRECTION_POINT){
            setStatus(SetPoint);
        }
        else{
            setStatus(SetDistance);
        }
    }
    else{
        direction = primaryDirection;
        setStatus(SetDistance);
    }
    moveRelativeZero(start);
    updateMouseButtonHints();
}

bool LC_ActionDrawLineSnake::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *pEvent, int status){
    return status != SetStartPoint; // can draw preview if at least start point is set
}

void LC_ActionDrawLineSnake::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    RS_Vector possibleEndPoint;

    switch (status) {
        case SetDirection:
        case SetPoint:
            possibleEndPoint = snap;
            break;
        case SetAngle: // draw line in direction specified by angle
            possibleEndPoint = calculateAngleEndpoint(snap);
            break;
        case SetDistance:
            switch (direction) {
                case DIRECTION_X: // draw horizontal line, y-axis is fixed
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.y = pPoints->data.startpoint.y;
                    possibleEndPoint.x = snap.x;
                    break;
                case DIRECTION_Y: // draw vertical line segment, x-axis is fixed
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.x = pPoints->data.startpoint.x;
                    possibleEndPoint.y = snap.y;
                    break;
                case DIRECTION_POINT: // free draw mode
                    possibleEndPoint = snap;
                    break;
                case DIRECTION_ANGLE: // draw segment in direction defined by angle
                    possibleEndPoint = calculateAngleEndpoint(snap);
                    break;
            }
            break;
        default:
            break;
    }
    createEntities(possibleEndPoint, list);
    if (showRefEntitiesOnPreview) {
        createRefPoint(pPoints->data.startpoint, list);
        createRefSelectablePoint(possibleEndPoint, list);
    }

}

RS_Vector LC_ActionDrawLineSnake::doGetRelativeZeroAfterTrigger(){
    return pPoints->history.at(pPoints->index()).currPt; // move relative end point to last point
}

void LC_ActionDrawLineSnake::createEntities(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList){
    auto *line = new RS_Line(pPoints->data.startpoint, potentialEndPoint);
    entitiesList << line;
}

bool LC_ActionDrawLineSnake::isStartPointValid() const{
    return pPoints->data.startpoint.valid;
}

const RS_Vector &LC_ActionDrawLineSnake::getStartPointForAngleSnap() const {
    return pPoints->data.startpoint;
}

void LC_ActionDrawLineSnake::doBack(QMouseEvent *e, int status){
    e->accept();
    switch (status) {
        case SetStartPoint:
            finishAction();
            break;
        case SetPoint:
        case SetDistance:
        case SetDirection:
        case SetAngle: // skip last operations
            setStatus(SetStartPoint);
            pPoints->data.startpoint = RS_Vector(false);
            updateOptions();
            break;
        default:
            finishAction();
    }
}
/*
 * Check whether line from start point to provided point is non-zero length
 */
bool LC_ActionDrawLineSnake::isNonZeroLine(const RS_Vector &possiblePoint) const{
    return LC_LineMath::isNonZeroLineLength( pPoints->data.startpoint, possiblePoint);
}

void LC_ActionDrawLineSnake::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetDistance:
            switch (direction) {
                case DIRECTION_X: {
                    // draw horizontal segment, fix start point y coordinate
                    RS_Vector possiblePoint(mouse.x, pPoints->data.startpoint.y);
                    if (isNonZeroLine(possiblePoint)){
                        pPoints->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                }
                    break;
                case DIRECTION_Y: {
                    // draw vertical segment, fix start point x coordinate
                    RS_Vector possiblePoint(pPoints->data.startpoint.x, mouse.y);
                    if (isNonZeroLine(possiblePoint)){
                        pPoints->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                }
                break;
                case DIRECTION_ANGLE:{
                    // draw segment in given angle direction
                    RS_Vector possiblePoint = calculateAngleEndpoint(mouse);
                    if (isNonZeroLine(possiblePoint)){
                        pPoints->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                }
                 break;
                default:
                    break;
            }
            break;
        case SetDirection:
        case SetPoint:
            if (isNonZeroLine(mouse)){
                // refuse zero length lines
                pPoints->data.endpoint = mouse;
                completeLineSegment(false);
            }
            break;
        case SetStartPoint:{
            doSetStartPoint(mouse);
            break;
        }
        case SetAngle: {
            // draw segment in direction specified by angle
            RS_Vector possiblePoint = calculateAngleEndpoint(mouse);
            if (isNonZeroLine(possiblePoint)){
                pPoints->data.endpoint = possiblePoint;
                completeLineSegment(false);
            }
            break;
        }
        default:
            break;
    }
}

/**
 * Completes current line segment, invokes trigger and based on direction set new status for action
 * @param close if true, line should be closed
 */
void LC_ActionDrawLineSnake::completeLineSegment(bool close){
    ++pPoints->startOffset;
    if (!close){
        addHistory(HA_SetEndpoint, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
    }
    trigger();
    pPoints->data.startpoint = pPoints->data.endpoint;

    switch (direction) {
        case DIRECTION_X: {
            // switch direction (snake)
            direction = DIRECTION_Y;
            setStatus(SetDistance);
            break;
        }
        case DIRECTION_Y: {
            // switch direction (snake)
            direction = DIRECTION_X;
            setStatus(SetDistance);
            break;
        }
        case DIRECTION_POINT:
            // stay in free direction
            direction = DIRECTION_POINT;
            setStatus(SetPoint);
            break;
        case DIRECTION_ANGLE:
            // stay in angle mode
            direction = DIRECTION_ANGLE;
            // by handle status differently for relative and absolute mode
            if (angleIsRelative){
                setStatus(SetDistance);
            }
            else {
                setStatus(SetAngle);
            }
            break;
        default:
            setStatus(SetDirection);
            break;
    }
    updateOptions();
    updateMouseButtonHints();
}

bool LC_ActionDrawLineSnake::doProceedCommand([[maybe_unused]]int status, const QString &c){
    bool result = true;
    if (checkCommand("close", c)){
        close();
        updateMouseButtonHints();
    } else if (checkCommand("undo", c)){
        undo();
        updateMouseButtonHints();
    } else if (checkCommand("polyline", c) ||
               checkCommand("pl", c)){
        polyline();
        updateMouseButtonHints();
    } else if (checkCommand("redo", c)){
        redo();
        updateMouseButtonHints();
    }
    else if (checkCommand("anglerel", c)){
        // line to angle related to previous segment
        setSetAngleState(true);
    } else if (checkCommand("start", c)){
        setNewStartPointState();
    } else {
        result = false;
    }
    return result;
}

bool LC_ActionDrawLineSnake::doProcessCommandValue(int status, const QString &c){
    bool result = true;
    switch (status) {
        case SetDirection:
            break;
        case SetDistance: {
            // processing entered distance value
            bool ok = false;
            double distance = RS_Math::eval(c, &ok);
            if (ok && LC_LineMath::isMeaningful(distance)){
                switch (direction) {
                    case DIRECTION_X: // the value is for x coordinate adjustment
                        pPoints->data.endpoint.x = pPoints->data.startpoint.x + distance;
                        pPoints->data.endpoint.y = pPoints->data.startpoint.y;
                        completeLineSegment(false);
                        break;
                    case DIRECTION_Y: // the value is for y coordinate adjustment
                        pPoints->data.endpoint.x = pPoints->data.startpoint.x;
                        pPoints->data.endpoint.y = pPoints->data.startpoint.y + distance;
                        completeLineSegment(false);
                        break;
                    case DIRECTION_ANGLE: { // the value is for coordinates adjustment in direction specified by angle
                        calculateAngleSegment(distance);
                        completeLineSegment(false);
                        break;
                    }
                    default:
                        break;
                }
            } else {
                result = false;
            }
            break;
        }
        case SetAngle: {
            // entering angle value
            result = processAngleValueInput(c);
            break;
        }
        default:
            break;
    }
    return result;
}


QStringList LC_ActionDrawLineSnake::getAvailableCommands(){
    QStringList cmd;
    cmd += command("pl");
    if (pPoints->index() + 1 < pPoints->history.size()){
        cmd += command("redo");
    }

    switch (getStatus()) {
        case SetDistance:
        case SetDirection:
            cmd += command("x");
            cmd += command("y");
            cmd += command("p");
            cmd += command("angle");
            cmd += command("anglerel");
            break;
        case SetPoint:
            cmd += command("x");
            cmd += command("y");
            cmd += command("angle");
            cmd += command("anglerel");

            if (pPoints->historyIndex >= 1){
                cmd += command("undo");
            }
            if (pPoints->startOffset >= 2){
                cmd += command("close");
            }
            break;
        default:
            break;
    }

    return cmd;
}

void LC_ActionDrawLineSnake::updateMouseButtonHints(){
    QString msg = "pl";

    if (pPoints->startOffset >= 2){
        msg += "/";
        msg += command("close");
    }
    if (pPoints->index() + 1 < pPoints->history.size()){
        if (msg.size() > 0){
            msg += "/";
        }
        msg += command("redo");
    }
    bool hasHistory = pPoints->historyIndex >= 1;
    if (hasHistory){
        if (msg.size() > 0){
            msg += "/";
        }
        msg += command("undo");
    }

    switch (getStatus()) {
        case SetStartPoint:
            updateMouseWidgetTRCancel(tr("Specify first point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetDirection:
            msg += "/";
            msg += command("p");
            msg += "/";
            msg += command("angle");
            msg += "/";
            msg += command("anglerel");
            updateMouseWidgetTRBack(tr("Specify direction (x or y) or [%1]").arg(msg));
            break;
        case SetDistance: {
            bool toX = direction == DIRECTION_X;
            bool toY = direction == DIRECTION_Y;
            msg += "/";
            msg += command("p");
            msg += "/";
            msg += command("angle");
            msg += "/";
            msg += command("anglerel");
            if (toX){
                msg += "/";
                msg += command("y");
                updateMouseWidgetTRBack(tr("Specify distance (%1) or [%2]").arg(tr("X"), msg));
            } else if (toY){
                msg += "/";
                msg += command("x");
                updateMouseWidgetTRBack(tr("Specify distance (%1) or [%2]").arg(tr("Y"), msg));
            }
            else if (direction == DIRECTION_ANGLE){
                msg += "/";
                msg += command("x");
                QString angleStr = RS_Math::doubleToString(angle, 1);
                updateMouseWidgetTRBack(tr("Specify distance (%1 deg) or [%2]").arg(angleStr, msg), MOD_SHIFT_MIRROR_ANGLE);
            }
            break;
        }
        case SetAngle:{
            msg += "/";
            msg += command("x");
            msg += "/";
            msg += command("y");
            msg += "/";
            msg += command("p");
            msg += "/";
            msg += command("angle");
            msg += "/";
            msg += command("anglerel");
            updateMouseWidgetTRBack(tr("Specify angle or [%2]").arg(msg));
            break;
        }
        case SetPoint: {
            msg += "/";
            msg += command("x");
            msg += "/";
            msg += command("y");
            msg += "/";
            msg += command("angle");
            msg += "/";
            msg += command("anglerel");
            updateMouseWidgetTRBack(tr("Specify point or [%1]").arg(msg), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        default:
            updateMouseWidget();
            break;
    }
}

void LC_ActionDrawLineSnake::next(){
    addHistory(HA_Next, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
    setStatus(SetDirection);
}

// in-action undo
void LC_ActionDrawLineSnake::undo(){
    if (mayUndo()){
        History h(pPoints->history.at(pPoints->index()));

        --pPoints->historyIndex;
        deletePreview();

        if (h.histAct != HA_Polyline){
            moveRelativeZero(h.prevPt);
        }

        switch (h.histAct) {
            case HA_SetStartpoint:
                setStatus(SetDirection);
                break;

            case HA_Polyline:
            case HA_SetEndpoint:
            case HA_Close:
                graphicView->setCurrentAction(new RS_ActionEditUndo(true, *container, *graphicView));
                pPoints->data.startpoint = h.prevPt;
                setStatus(SetDirection);
                break;

            case HA_Next:
                pPoints->data.startpoint = h.prevPt;
                setStatus(SetDirection);
                break;
        }

        // get index for close from new current history
        h = pPoints->history.at(pPoints->index());
        pPoints->startOffset = h.startOffset;
    } else {
        commandMessage(tr("Cannot undo: Begin of history reached"));
    }
}

bool LC_ActionDrawLineSnake::mayUndo() const{return 0 <= pPoints->historyIndex;}

void LC_ActionDrawLineSnake::redo(){
    if (mayRedo()){
        ++pPoints->historyIndex;
        History h(pPoints->history.at(pPoints->index()));
        deletePreview();
        if (h.histAct != HA_Polyline){
            moveRelativeZero(h.currPt);
            pPoints->data.startpoint = h.currPt;
            pPoints->startOffset = h.startOffset;
        }
        switch (h.histAct) {
            case HA_SetStartpoint:
                setStatus(SetDirection);
                break;

            case HA_Polyline:
            case HA_SetEndpoint:
                graphicView->setCurrentAction(new RS_ActionEditUndo(false, *container, *graphicView));
                setStatus(SetDirection);
                break;

            case HA_Close:
                graphicView->setCurrentAction(new RS_ActionEditUndo(false, *container, *graphicView));
                setStatus(SetDirection);
                break;

            case HA_Next:
                setStatus(SetDirection);
                break;
        }
    } else {
        commandMessage(tr("Cannot redo: End of history reached"));
    }
}

void LC_ActionDrawLineSnake::addHistory(LC_ActionDrawLineSnake::HistoryAction a, const RS_Vector &p, const RS_Vector &c, const int s){
    if (pPoints->historyIndex < -1){
        pPoints->historyIndex = -1;
    }
    pPoints->history.erase(pPoints->history.begin() + pPoints->historyIndex + 1, pPoints->history.end());
    pPoints->history.push_back(History(a, p, c, s));
    pPoints->historyIndex = static_cast<int>(pPoints->history.size() - 1);
}

// closing sequence of lines
void LC_ActionDrawLineSnake::close(){
    if (mayClose()){
        History h(pPoints->history.at(pPoints->index(-pPoints->startOffset)));
        if (LC_LineMath::isNonZeroLineLength(pPoints->data.startpoint, h.currPt)){
            pPoints->data.endpoint = h.currPt;
            addHistory(HA_Close, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
            completeLineSegment(true);
        }
    } else {
        commandMessage("Cannot close sequence of lines: Not enough entities defined yet, or already closed.");
    }
}

// creation of polyline. This will end line drawing sequence
void LC_ActionDrawLineSnake::polyline(){
    // fixme - add support of alternative way of polyline based on selected entities (so only drawn lines will be converted to polyline, without others found
    RS_Entity *en = catchEntity(pPoints->data.endpoint, RS2::EntityLine, RS2::ResolveAllButTextImage);
    if (en != nullptr){
        finishAction();
        addHistory(HA_Polyline, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
        auto *polylineSegmentAction = new RS_ActionPolylineSegment(*container, *graphicView, en);
        graphicView->setCurrentAction(polylineSegmentAction);
    }
}

bool LC_ActionDrawLineSnake::mayClose(){
    return 1 < pPoints->startOffset && 0 <= pPoints->historyIndex - pPoints->startOffset;
}

bool LC_ActionDrawLineSnake::mayRedo(){
    return pPoints->history.size() > (pPoints->index() + 1);
}

bool LC_ActionDrawLineSnake::mayStart(){
    return getStatus() == SetDistance || getStatus() == SetPoint;
}

// FIXME - check LC_LineMath
void LC_ActionDrawLineSnake::calculateAngleSegment(double distance){
    double angleRadians = RS_Math::deg2rad(angle);
    double realAngle = defineActualSegmentAngle(angleRadians);
    pPoints->data.endpoint = pPoints->data.startpoint.relative(distance, realAngle);
}

double LC_ActionDrawLineSnake::defineActualSegmentAngle(double realAngle){
    if (angleIsRelative){
        size_t currentIndex = pPoints->index();  // this should be start point of current line
        if (currentIndex > 0){
            History h(pPoints->history.at(currentIndex));

            if (h.histAct == HA_SetEndpoint) // this is start of previous line segment
            {
              RS_Vector previousSegmentStart = h.prevPt;
              RS_Vector previousSegmentEnd = h.currPt;

              RS_Vector line = previousSegmentEnd - previousSegmentStart;
              double previousSegmentAngle = line.angle();

              realAngle = realAngle + previousSegmentAngle;
            }
        }
    }
    return realAngle;
}

RS_Vector LC_ActionDrawLineSnake::calculateAngleEndpoint(const RS_Vector &snap){
    RS_Vector possibleEndPoint;
    double angleToUse = angle;
    if (alternativeActionMode){
        angleToUse = 180-angle;
    }
    double angleRadians = RS_Math::deg2rad(angleToUse);
    RS_Vector infiniteTickStartPoint = pPoints->data.startpoint;
    double realAngle = defineActualSegmentAngle(angleRadians);

    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, realAngle);
    RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick =  LC_LineMath::getNearestPointOnInfiniteLine(snap, infiniteTickStartPoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}

LC_ActionOptionsWidget* LC_ActionDrawLineSnake::createOptionsWidget(){
    return new LC_LineOptions();
}
