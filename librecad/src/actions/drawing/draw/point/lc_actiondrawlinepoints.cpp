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

#include "lc_actiondrawlinepoints.h"

#include "lc_linemath.h"
#include "lc_linepointsoptions.h"
#include "rs_point.h"

LC_ActionDrawLinePoints::LC_ActionDrawLinePoints(LC_ActionContext *actionContext, bool drawMiddle)
    :LC_AbstractActionDrawLine("LineDrawPoints", actionContext,
        drawMiddle ? RS2::ActionDrawPointsMiddle: RS2::ActionDrawLinePoints){
}
LC_ActionDrawLinePoints::~LC_ActionDrawLinePoints()= default;

/**
 * just cleanup if needed
 * @param status new status
 */
void LC_ActionDrawLinePoints::init(int status){
    LC_AbstractActionWithPreview::init(status);
    if (status == 0){
        m_point1Set = false;
        m_startpoint = RS_Vector(false);
        m_endpoint = RS_Vector(false);
    }
}

void LC_ActionDrawLinePoints::doSetStartPoint(RS_Vector vector){
    // pre-snap to relative zero
    m_startpoint = vector;
    m_point1Set = true;
    if (m_direction == DIRECTION_POINT || m_direction == DIRECTION_NONE){
        setStatus(SetPoint);
    }
    else{
        setStatus(SetDistance);
    }
}

void LC_ActionDrawLinePoints::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    // prepare points data
    createPoints(m_endpoint, list);
}

bool LC_ActionDrawLinePoints::doCheckMayTrigger(){
    bool result =  m_point1Set && m_startpoint.valid && m_endpoint.valid;
    return result;
}

RS_Vector LC_ActionDrawLinePoints::doGetRelativeZeroAfterTrigger(){
    return m_endpoint;
}

void LC_ActionDrawLinePoints::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
//    finishAction();
     if (m_direction == DIRECTION_X || m_direction == DIRECTION_Y){
         m_direction = DIRECTION_POINT;
     }
     init(SetStartPoint);
}

void LC_ActionDrawLinePoints::doPreparePreviewEntities([[maybe_unused]]LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
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
            switch (m_direction) {
                case DIRECTION_X: // use only x coordinate from snap
                    possibleEndPoint = restrictHorizontal(m_startpoint, snap);
                    break;
                case DIRECTION_Y: // use only y coordinate from snap
                    possibleEndPoint = restrictVertical(m_startpoint, snap);
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
        if (m_actionType == RS2::ActionDrawPointsMiddle && list.size() == 1){
            RS_Entity* ep = list.at(0);
            auto* point = dynamic_cast<RS_Point *>(ep);
            if (point != nullptr){
                previewEntityToCreate(point, false);
            }
        }
        if (m_showRefEntitiesOnPreview) {
            createRefSelectablePoint(possibleEndPoint, list);
            createRefPoint(m_startpoint, list);
//            if (actionType == RS2::ActionDrawPointsMiddle){
                createRefLine(m_startpoint, possibleEndPoint, list);
//            }
        }
    }
}

RS_Vector LC_ActionDrawLinePoints::getPossibleEndPointForAngle(const RS_Vector &snap){
    // if shift is pressed, we'll use alternative mirrored angle for direction
    double angleToUse = m_angleDegrees;
    if (m_alternativeActionMode){
        angleToUse = 180 - m_angleDegrees;
    }
    double wcsAngle = toWorldAngleFromUCSBasisDegrees(angleToUse);
    return LC_LineMath::calculateEndpointForAngleDirection(wcsAngle,m_startpoint, snap);
}

/**
 * Calculate positions of points specified on line between start point and given point
 * @param potentialEndPoint coordinates of end point *
 */
void LC_ActionDrawLinePoints::createPoints(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList){

    // determine angle of line
    double segmentAngle = m_startpoint.angleTo(potentialEndPoint);

    // calculate distance of line
    double distanceAll = m_startpoint.distanceTo(potentialEndPoint);

    // calculate length of single segment between points
    double segmentLength;

    int numberOfPoints = m_pointsCount;

    if (m_fixedDistanceMode){
        segmentLength = m_fixedDistance;
        if (m_withinLineMode){
            // calculate required number of points dynamically based on length of line and distance
            numberOfPoints = std::ceil(distanceAll / segmentLength);
        }
    }
    else{
        segmentLength = distanceAll / (m_pointsCount + 1);
    }

    // handle point for start edge of line
    bool includeStartPoint = m_edgePointsMode == DRAW_EDGE_START || m_edgePointsMode == DRAW_EDGE_BOTH;
    if (includeStartPoint){
        createPoint(m_startpoint, entitiesList);
    }

    bool lineExceeds = false;

    // proceed with intermediate points
    for (int i = 1; i <= numberOfPoints; i++) {
        // calc distance from start point to intermediate point
        double distanceFromStart = segmentLength * i;

        if (m_fixedDistanceMode && m_withinLineMode){
            // check whether we are still within line if we in fixed distance mode and should fit points into the line
            lineExceeds = distanceFromStart > distanceAll;
            if (lineExceeds){
                break;
            }
        }

        // define point with needed distance and angle of line from start point
        RS_Vector point = m_startpoint.relative(distanceFromStart, segmentAngle);

        createPoint(point, entitiesList);
    }

    // handle point for end edge of line
    if (!lineExceeds){
        bool includeEndPoint = m_edgePointsMode == DRAW_EDGE_END || m_edgePointsMode == DRAW_EDGE_BOTH;
        if (includeEndPoint){
            RS_Vector actualEndPoint;
            if (m_fixedDistanceMode){
                double endDistance = (numberOfPoints + 1) * m_fixedDistance;
                actualEndPoint  = m_startpoint.relative(endDistance, segmentAngle);
            }
            else {
                actualEndPoint = potentialEndPoint;
            }
            createPoint(actualEndPoint, entitiesList);
            // endpoint field will be used for setting related zero after trigger, so
            // we'll correct it there
            m_endpoint = actualEndPoint;
        }
    }
}


/**
 * check whether start is set (and so it is valid)
 * @return
 */
bool LC_ActionDrawLinePoints::isStartPointValid() const{
    return m_startpoint.valid;
}

const RS_Vector& LC_ActionDrawLinePoints::getStartPointForAngleSnap() const {
    return m_startpoint;
}

void LC_ActionDrawLinePoints::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetDistance:
            switch (m_direction) {
                case DIRECTION_X: { // calculate  point on X axis
                    RS_Vector possiblePoint = restrictHorizontal(m_startpoint, mouse);
                    if (isNonZeroLine(possiblePoint)){
                        m_endpoint = possiblePoint;
                        trigger();
                    }
                    break;
                }
                case DIRECTION_Y: {// calculate  point on y axis
                    RS_Vector possiblePoint = restrictVertical(m_startpoint, mouse);
                    if (isNonZeroLine(possiblePoint)){
                        m_endpoint = possiblePoint;
                        trigger();
                    }
                    break;
                }
                case DIRECTION_ANGLE: { // calculate end point in given angle direction
                    RS_Vector possiblePoint = getPossibleEndPointForAngle(mouse);
                    if (isNonZeroLine(possiblePoint)) {
                        m_endpoint = possiblePoint;
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
                m_endpoint = possiblePoint;
                trigger();
            }
            break;
        }
        case SetDirection:
        case SetPoint: { // set end to provided point
            if (isNonZeroLine(mouse)) {
                // refuse zero length lines
                m_endpoint = mouse;
                trigger();
            }
            break;
        }
        case SetStartPoint:{ // setup start point of line
            m_startpoint = mouse;
            m_point1Set = true;
            if (m_direction != DIRECTION_POINT && m_direction != DIRECTION_NONE){
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
    return LC_LineMath::isNonZeroLineLength(m_startpoint, possiblePoint);
}

/**
 * Proceed commands from command widget
 * @param e event
 * @param c command
 * @return true if command is processed, false - if additional processing is needed
 */
bool LC_ActionDrawLinePoints::doProceedCommand([[maybe_unused]]int status, const QString &c){
    bool result = true;
    bool shouldProcess = false;
    if (m_actionType == RS2::ActionDrawLinePoints) {
        bool edgeStatus = status == SetEdge;
        if (checkCommand("edge_none", c)) {        //specifies no points in line edges
            updateEdgePointsMode(DRAW_EDGE_NONE);
        } else if (checkCommand("edge_start", c)) {  // point will be created in start point edge
            updateEdgePointsMode(DRAW_EDGE_START);
        } else if (checkCommand("edge_end", c)) {  // point will be created in end point edge
            updateEdgePointsMode(DRAW_EDGE_END);
        } else if (checkCommand("edge_both", c)) {  // points will be created in start and end points of line
            updateEdgePointsMode(DRAW_EDGE_BOTH);
        } else if (edgeStatus && checkCommand("start", c)) { // point will be created in start point edge
            updateEdgePointsMode(DRAW_EDGE_START);
            m_edgePointsMode = DRAW_EDGE_START;
            setMajorStatus();
        } else if (edgeStatus && checkCommand("end", c)) {
            updateEdgePointsMode(DRAW_EDGE_END);
            m_edgePointsMode = DRAW_EDGE_END;
            setMajorStatus();
        } else if (edgeStatus && checkCommand("none", c)) {
            m_edgePointsMode = DRAW_EDGE_NONE;
            setMajorStatus();
        } else if (edgeStatus && checkCommand("both", c)) {
            m_edgePointsMode = DRAW_EDGE_BOTH;
            setMajorStatus();
        } else if (checkCommand("edges", c)) { // initiates edge entering mode
            setStatus(SetEdge);
        } else if (checkCommand("dist_fixed", c)) {  // switches to fixed distance mode
            m_fixedDistanceMode = true;
            updateOptions();
        } else if (checkCommand("dist_flex", c)) { // switches to flexible distance mode
            m_fixedDistanceMode = false;
            updateOptions();
        } else if (checkCommand("nofit", c)) { // for fixed distance mode, allows creation points outside of line
            m_withinLineMode = false;
            updateOptions();
        } else if (checkCommand("fit", c)) { //for fixed distance mode, ensures that all point are within the line
            m_withinLineMode = true;
            updateOptions();
        } else if (checkCommand("distance", c)) { // initiates entering distance between points (for fixed mode)
            setStatus(SetFixDistance);
        }
        else{
            shouldProcess = true;
        }
    }
    else {
        shouldProcess = true;
    }
    if (shouldProcess){
        if (checkCommand("number", c)){ // initiates entering of points numbers (edges are not counted!)
            setStatus(SetPointsCount);
            updateOptions();
        }
        else{
            result = false;
        }
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
                m_fixedDistance = distance;
                m_fixedDistanceMode = true;
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
                switch (m_direction) {
                    case DIRECTION_X: // calculate x on x axis
                        m_endpoint.x = m_startpoint.x + distance;
                        m_endpoint.y = m_startpoint.y;
                        m_endpoint.valid = true;
                        trigger();
                        break;
                    case DIRECTION_Y: // calculate y on y axis
                        m_endpoint.x = m_startpoint.x;
                        m_endpoint.y = m_startpoint.y + distance;
                        m_endpoint.valid = true;
                        trigger();
                        break;
                    case DIRECTION_ANGLE: { // calculate endpoint coordinate by previously set angle and distance
                        m_endpoint = LC_LineMath::getEndOfLineSegment(m_startpoint, m_angleDegrees, distance);
                        m_endpoint.valid = true;
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
            if (m_actionType == RS2::ActionDrawLinePoints) {
                cmd += command("x");
                cmd += command("y");
                cmd += command("p");
                cmd += command("angle");
                cmd += command("edges");
                cmd += command("distance");
                cmd += command("dist_fixed");
                cmd += command("dist_flex");
                cmd += command("fit");
                cmd += command("nofit");
                cmd += command("fix");
                cmd += command("nofix");
                cmd += command("edge_none");
                cmd += command("edge_start");
                cmd += command("edge_end");
                cmd += command("edge_both");
            }
            cmd += command("number");

            break;
        default:
            break;
    }
    return cmd;
}

// FIXME - SAND - REVIEW command PROMPTS!!!

void LC_ActionDrawLinePoints::updateMouseButtonHints(){
    QString msg;
    switch (getStatus()) {
        case SetStartPoint:
            updateMouseWidgetTRCancel(tr("Specify First Point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint:
            if (m_actionType == RS2::ActionDrawLinePoints) {
                updateMouseWidgetTRBack(tr("Specify Second Point\nor [number|x|y|angle|p|edges|distance]"), MOD_SHIFT_ANGLE_SNAP);
            }
            else{
                updateMouseWidgetTRBack(tr("Specify Second Point\nor [number]"), MOD_SHIFT_ANGLE_SNAP);
            }
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
            bool toX = m_direction == DIRECTION_X;
            bool toY = m_direction == DIRECTION_Y;
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
            } else if (m_direction == DIRECTION_ANGLE){
                msg += "|" + command("x");
                msg += "|" + command("y");
                QString angleStr = RS_Math::doubleToString(m_angleDegrees, 1);
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
void LC_ActionDrawLinePoints::doBack([[maybe_unused]]LC_MouseEvent *e, int status){
    if (status == SetStartPoint){ // complete action
        finishAction();
    } else { // return to set start point state
        // restore relative point to start point
        moveRelativeZero(m_startpoint);
        init(SetStartPoint);
    }
}

/**
 * updates of edges mode
 * @param mode
 */
void LC_ActionDrawLinePoints::updateEdgePointsMode(int mode){
    m_edgePointsMode = mode;
    updateOptions();
}

void LC_ActionDrawLinePoints::setEdgePointsMode(int value){
   m_edgePointsMode = value;
}

void LC_ActionDrawLinePoints::updatePointsCount(int count){
    m_pointsCount = count;
    updateOptions();
}

void LC_ActionDrawLinePoints::setMajorStatus(){
    updateOptions();
    if (m_point1Set){
       setStatus(SetPoint);
    }
    else{
      setStatus(SetStartPoint);
    }
}

bool LC_ActionDrawLinePoints::isAllowDirectionCommands() {
    return m_actionType == RS2::ActionDrawLinePoints;
}

bool LC_ActionDrawLinePoints::doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) {
    if (tag == "angle") {
        setAngleDegrees(RS_Math::rad2deg(angleRad));
        return true;
    }
    return false;
}

bool LC_ActionDrawLinePoints::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if (tag == "distance") {
        setPointsDistance(distance);
        return true;
    }
    return false;
}

/**
 * options for the action
 */
LC_ActionOptionsWidget* LC_ActionDrawLinePoints::createOptionsWidget(){
    return new LC_LinePointsOptions();
}
