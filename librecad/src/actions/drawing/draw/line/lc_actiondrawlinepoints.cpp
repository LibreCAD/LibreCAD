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

#include <QMouseEvent>
#include "rs_math.h"
#include "rs_point.h"
#include "lc_linemath.h"
#include "lc_linepointsoptions.h"
#include "lc_abstractactiondrawline.h"
#include "lc_actiondrawlinepoints.h"
#include "rs_previewactioninterface.h"

LC_ActionDrawLinePoints::LC_ActionDrawLinePoints(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :LC_AbstractActionDrawLine("LineDrawPoints",container, graphicView){
    actionType = RS2::ActionDrawLinePoints;
}

LC_ActionDrawLinePoints::~LC_ActionDrawLinePoints()= default;

/**
 * just cleanup if needed
 * @param status new status
 */
void LC_ActionDrawLinePoints::init(int status){
    LC_AbstractActionWithPreview::init(status);
    if (status == 0){
        point1Set = false;
        startpoint = RS_Vector(false);
        endpoint = RS_Vector(false);
    }
}

void LC_ActionDrawLinePoints::doSetStartPoint(RS_Vector vector){
    // pre-snap to relative zero
    startpoint = vector;
    point1Set = true;
    if (direction == DIRECTION_POINT || direction == DIRECTION_NONE){
        setStatus(SetPoint);
    }
    else{
        setStatus(SetDistance);
    }
}

void LC_ActionDrawLinePoints::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    // prepare points data
    createPoints(endpoint, list);
}

bool LC_ActionDrawLinePoints::doCheckMayTrigger(){
    bool result =  point1Set && startpoint.valid && endpoint.valid;
    return result;
}

RS_Vector LC_ActionDrawLinePoints::doGetRelativeZeroAfterTrigger(){
    return endpoint;
}

void LC_ActionDrawLinePoints::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
//    finishAction();
     if (direction == DIRECTION_X || direction == DIRECTION_Y){
         direction = DIRECTION_POINT;
     }
     init(SetStartPoint);
}

void LC_ActionDrawLinePoints::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    // determine candidate for possible end point
    RS_Vector possibleEndPoint;
    switch (status) {
        case SetStartPoint:
        case SetEdge:
        case SetFixDistance:
        case SetPointsCount:
            return;
        case SetDirection:
        case SetPoint:
            possibleEndPoint = snap;
            break;
        case SetAngle:
            // calculate point with given angle from start point to snap
            possibleEndPoint = getPossibleEndPointForAngle(snap);
            break;
        case SetDistance:
            switch (direction) {
                case DIRECTION_X: // use only x coordinate from snap
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.y = startpoint.y;
                    possibleEndPoint.x = snap.x;
                    break;
                case DIRECTION_Y: // use only y coordinate from snap
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.x = startpoint.x;
                    possibleEndPoint.y = snap.y;
                    break;
                case DIRECTION_POINT:
                    possibleEndPoint = snap;
                    break;
                case DIRECTION_ANGLE:
                    // calculate point with given angle from start point to snap
                    possibleEndPoint = getPossibleEndPointForAngle(snap);
                    break;
            }
            break;
        default:
            break;
    }
    // draw preview if this is non-zero line
    if (isNonZeroLine(possibleEndPoint)){
        createPoints(possibleEndPoint, list);
        if (showRefEntitiesOnPreview) {
            createRefSelectablePoint(possibleEndPoint, list);
            createRefPoint(startpoint, list);
        }
    }
}

RS_Vector LC_ActionDrawLinePoints::getPossibleEndPointForAngle(const RS_Vector &snap){
    // if shift is pressed, we'll use alternative mirrored angle for direction
    double angleToUse = angle;
    if (alternativeActionMode){
        angleToUse = 180-angle;
    }
    return LC_LineMath::calculateEndpointForAngleDirection(angleToUse,startpoint, snap);
}

/**
 * Calculate positions of points specified on line between start point and given point
 * @param potentialEndPoint coordinates of end point *
 */
void LC_ActionDrawLinePoints::createPoints(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList){

    // determine angle of line
    double segmentAngle = startpoint.angleTo(potentialEndPoint);

    // calculate distance of line
    double distanceAll = startpoint.distanceTo(potentialEndPoint);

    // calculate length of single segment between points
    double segmentLength;

    int numberOfPoints = pointsCount;

    if (fixedDistanceMode){
        segmentLength = fixedDistance;
        if (withinLineMode){
            // calculate required number of points dynamically based on length of line and distance
            numberOfPoints = std::ceil(distanceAll / segmentLength);
        }
    }
    else{
        segmentLength = distanceAll / (pointsCount + 1);
    }

    // handle point for start edge of line
    bool includeStartPoint = edgePointsMode == DRAW_EDGE_START || edgePointsMode == DRAW_EDGE_BOTH;
    if (includeStartPoint){
        createPoint(startpoint, entitiesList);
    }

    bool lineExceeds = false;

    // proceed with intermediate points
    for (int i = 1; i <= numberOfPoints; i++) {
        // calc distance from start point to intermediate point
        double distanceFromStart = segmentLength * i;

        if (fixedDistanceMode && withinLineMode){
            // check whether we are still within line if we in fixed distance mode and should fit points into the line
            lineExceeds = distanceFromStart > distanceAll;
            if (lineExceeds){
                break;
            }
        }

        // define point with needed distance and angle of line from start point
        RS_Vector point = startpoint.relative(distanceFromStart, segmentAngle);

        createPoint(point, entitiesList);
    }

    // handle point for end edge of line
    if (!lineExceeds){
        bool includeEndPoint = edgePointsMode == DRAW_EDGE_END || edgePointsMode == DRAW_EDGE_BOTH;
        if (includeEndPoint){
            RS_Vector actualEndPoint;
            if (fixedDistanceMode){
                double endDistance = (numberOfPoints + 1) * fixedDistance;
                actualEndPoint  = startpoint.relative(endDistance, segmentAngle);
            }
            else {
                actualEndPoint = potentialEndPoint;
            }
            createPoint(actualEndPoint, entitiesList);
            // endpoint field will be used for setting related zero after trigger, so
            // we'll correct it there
            endpoint = actualEndPoint;
        }
    }
}


/**
 * check whether start is set (and so it is valid)
 * @return
 */
bool LC_ActionDrawLinePoints::isStartPointValid() const{
    return startpoint.valid;
}

const RS_Vector& LC_ActionDrawLinePoints::getStartPointForAngleSnap() const {
    return startpoint;
}

void LC_ActionDrawLinePoints::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetDistance:
            switch (direction) {
                case DIRECTION_X: { // calculate  point on X axis
                    RS_Vector possiblePoint(mouse.x,startpoint.y);
                    if (isNonZeroLine(possiblePoint)){
                        endpoint = possiblePoint;
                        trigger();
                    }
                    break;
                }
                case DIRECTION_Y: {// calculate  point on y axis
                    RS_Vector possiblePoint(startpoint.x, mouse.y);
                    if (isNonZeroLine(possiblePoint)){
                        endpoint = possiblePoint;
                        trigger();
                    }
                    break;
                }
                case DIRECTION_ANGLE: { // calculate end point in given angle direction
                    RS_Vector possiblePoint = getPossibleEndPointForAngle(mouse);
                    if (isNonZeroLine(possiblePoint)) {
                        endpoint = possiblePoint;
                        trigger();
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        case SetAngle: {
            RS_Vector possiblePoint = getPossibleEndPointForAngle(mouse);
            if (isNonZeroLine(possiblePoint)){
                endpoint = possiblePoint;
                trigger();
            }
            break;
        }
        case SetDirection:
        case SetPoint: { // set end to provided point
            if (isNonZeroLine(mouse)) {
                // refuse zero length lines
                endpoint = mouse;
                trigger();
            }
            break;
        }
        case SetStartPoint:{ // setup start point of line
            startpoint = mouse;
            point1Set = true;
            if (direction != DIRECTION_POINT && direction != DIRECTION_NONE){
                setStatus(SetDistance);
            }
            else {
                setStatus(SetPoint);
            }
            moveRelativeZero(mouse);
            break;
        }
        case SetFixDistance:
            break;
        default:
            break;
    }
}

/**
 * Check whether distance between start point and given point is non-zero
 * @param possiblePoint second point
 * @return true if non-zero
 */
bool LC_ActionDrawLinePoints::isNonZeroLine(const RS_Vector &possiblePoint) const{
    return LC_LineMath::isNonZeroLineLength(startpoint, possiblePoint);
}

/**
 * Proceed commands from command widget
 * @param e event
 * @param c command
 * @return true if command is processed, false - if additional processing is needed
 */
bool LC_ActionDrawLinePoints::doProceedCommand([[maybe_unused]]int status, const QString &c){
    bool result = true;
    bool edgeStatus = status == SetEdge;
    if (checkCommand("edge_none", c)){        //specifies no points in line edges
        updateEdgePointsMode(DRAW_EDGE_NONE);
    }
    else if (checkCommand("edge_start", c)){  // point will be created in start point edge
        updateEdgePointsMode(DRAW_EDGE_START);
    }
    else if (checkCommand("edge_end", c)){  // point will be created in end point edge
        updateEdgePointsMode(DRAW_EDGE_END);
    }
    else if (checkCommand("edge_both", c)){  // points will be created in start and end points of line
        updateEdgePointsMode(DRAW_EDGE_BOTH);
    }
    else if (edgeStatus &&checkCommand("start",c)){ // point will be created in start point edge
        updateEdgePointsMode(DRAW_EDGE_START);
        edgePointsMode = DRAW_EDGE_START;
        setMajorStatus();
    }
    else if (edgeStatus &&checkCommand("end",c)){
        updateEdgePointsMode(DRAW_EDGE_END);
        edgePointsMode = DRAW_EDGE_END;
        setMajorStatus();
    }
    else if (edgeStatus &&checkCommand("none",c)){
        edgePointsMode = DRAW_EDGE_NONE;
        setMajorStatus();
    }
    else if (edgeStatus &&checkCommand("both",c)){
        edgePointsMode = DRAW_EDGE_BOTH;
        setMajorStatus();
    }
    else if (checkCommand("edges",c)){ // initiates edge entering mode
        setStatus(SetEdge);
    }
    else if (checkCommand("number", c)){ // initiates entering of points numbers (edges are not counted!)
        setStatus(SetPointsCount);
        updateOptions();
    }
    else if (checkCommand("dist_fixed", c)){  // switches to fixed distance mode
        fixedDistanceMode = true;
        updateOptions();
    }
    else if (checkCommand("dist_flex", c)){ // switches to flexible distance mode
        fixedDistanceMode = false;
        updateOptions();
    }
    else if (checkCommand("nofit", c)){ // for fixed distance mode, allows creation points outside of line
        withinLineMode = false;
        updateOptions();
    }
    else if (checkCommand("fit", c)){ //for fixed distance mode, ensures that all point are within the line
        withinLineMode = true;
        updateOptions();
    }
    else if (checkCommand("distance", c)){ // initiates entering distance between points (for fixed mode)
        setStatus(SetFixDistance);
    }
    else{
        result = false;
    }
    return result;
}

/**
 * processing of individual value
 * @param e
 * @param c
 * @return
 */
bool LC_ActionDrawLinePoints::doProcessCommandValue(int status, const QString &c){
    bool result = true;
    switch (status) {
        case SetDirection:
            // processed earlier
            break;
        case SetFixDistance:{
            bool ok = false;
            double distance = RS_Math::eval(c, &ok);
            if (ok && LC_LineMath::isMeaningful(distance)){ // non-zero distance is provided
                fixedDistance = distance;
                fixedDistanceMode = true;
                setMajorStatus();
            }
            else{
                result = false;
            }
            break;
        }
        case SetDistance: {
            bool ok = false;
            double distance = RS_Math::eval(c, &ok);
            if (ok && LC_LineMath::isMeaningful(distance)){ // non-zero distance is provided
                switch (direction) {
                    case DIRECTION_X: // calculate x on x axis
                        endpoint.x = startpoint.x + distance;
                        endpoint.y = startpoint.y;
                        endpoint.valid = true;
                        trigger();
                        break;
                    case DIRECTION_Y: // calculate y on y axis
                        endpoint.x = startpoint.x;
                        endpoint.y = startpoint.y + distance;
                        endpoint.valid = true;
                        trigger();
                        break;
                    case DIRECTION_ANGLE: { // calculate endpoint coordinate by previously set angle and distance
                        endpoint = LC_LineMath::getEndOfLineSegment(startpoint, angle, distance);
                        endpoint.valid = true;
                        trigger();
                        break;
                    }
                    default:
                        break;
                }
            } else {
                result = false; // invalid value
            }
            break;
        }
        case SetPointsCount: { // set amount of points
            bool ok = false;
            int count = RS_Math::eval(c, &ok);
            if (ok && count > 0){ // at least 1 point should be present
                updatePointsCount(count);
                setMajorStatus();
            } else {
                result = false; // in valid value
            }
            break;
        }
        case SetAngle: { // process angle value
            processAngleValueInput( c);
            break;
        }
        default:
            break;
    }
    return result;
}

QStringList LC_ActionDrawLinePoints::getAvailableCommands(){
    QStringList cmd;
    switch (getStatus()) {
        case SetEdge:
        case SetDistance:
        case SetDirection:
        case SetPointsCount:
        case SetPoint:
        case SetAngle:
            cmd += command("x");
            cmd += command("y");
            cmd += command("p");
            cmd += command("angle");
            cmd += command("number");
            cmd += command("edges");
            cmd += command("edges");
            cmd += command("distance");
            cmd += command("fit");
            cmd += command("nofit");
            cmd += command("fix");
            cmd += command("nofix");
//            cmd += command("edge_none");
//            cmd += command("edge_start");
//            cmd += command("edge_end");
//            cmd += command("edge_both");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawLinePoints::updateMouseButtonHints(){
    QString msg;
    switch (getStatus()) {
        case SetStartPoint:
            updateMouseWidgetTRCancel(tr("Specify First Point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint:
            updateMouseWidgetTRBack(tr("Specify Second Point\nor [number|x|y|angle|p|edges|distance]"),MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetDirection:
            updateMouseWidgetTRBack(tr("Specify line direction\n[x|y|angle|p|distance]"));
            break;
        case SetAngle:
            updateMouseWidgetTRBack(tr("Specify line direction angle\nor [x|y|p|number|edges|distance]"));
            break;
        case SetEdge:
            updateMouseWidgetTRBack(tr("Specify edge points mode\n[none|start|end|both|distance]"));
            break;
        case SetFixDistance:
            updateMouseWidgetTRBack(tr("Specify fixed distance between points\nor[x|y|p|number|edges]"));
            break;
        case SetDistance: {
            bool toX = direction == DIRECTION_X;
            bool toY = direction == DIRECTION_Y;
            msg += command("number")+"|";
            msg += command("angle")+"|";
            msg += command("p")+"|";
            msg += command("edges");
            if (toX){
                msg += "|" + command("y");
                updateMouseWidget(tr("Specify distance (%1)\nor [%2]").arg(tr("X"), msg),tr("Back"));
            } else if (toY){
                msg += "|" + command("x");
                updateMouseWidget(tr("Specify distance (%1)\nor [%2]").arg(tr("Y"), msg), tr("Back"));
            } else if (direction == DIRECTION_ANGLE){
                msg += "|" + command("x");
                msg += "|" + command("y");
                QString angleStr = RS_Math::doubleToString(angle, 1);
                updateMouseWidget(tr("Specify  distance (angle %1 deg)\nor [%2]").arg(angleStr, msg),tr("Back"), MOD_SHIFT_MIRROR_ANGLE);
            }
            break;
        }
        case SetPointsCount:
            updateMouseWidgetTRBack(tr("Specify points count"));
            break;
        default:
            LC_AbstractActionDrawLine::updateMouseButtonHints();
    }
}

/**
 * Right mouse button processing - doing back for specific state
 * @param e original event
 * @param status current status
 */
void LC_ActionDrawLinePoints::doBack([[maybe_unused]]QMouseEvent *e, int status){
    if (status == SetStartPoint){ // complete action
        finishAction();
    } else { // return to set start point state
        // restore relative point to start point
        moveRelativeZero(startpoint);
        init(SetStartPoint);
    }
}

/**
 * updates of edges mode
 * @param mode
 */
void LC_ActionDrawLinePoints::updateEdgePointsMode(int mode){
    edgePointsMode = mode;
    updateOptions();
}

void LC_ActionDrawLinePoints::setEdgePointsMode(int value){
   edgePointsMode = value;
}

void LC_ActionDrawLinePoints::updatePointsCount(int count){
    pointsCount = count;
    updateOptions();
}

void LC_ActionDrawLinePoints::setMajorStatus(){
    updateOptions();
    if (point1Set){
       setStatus(SetPoint);
    }
    else{
      setStatus(SetStartPoint);
    }
}

/**
 * options for the action
 */
LC_ActionOptionsWidget* LC_ActionDrawLinePoints::createOptionsWidget(){
    return new LC_LinePointsOptions();
}
