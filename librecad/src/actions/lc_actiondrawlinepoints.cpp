#include "lc_actiondrawlinepoints.h"
#include "lc_linepointsoptions.h"
#include "rs_math.h"
#include "lc_linemath.h"
#include "lc_abstract_action_draw_line.h"
#include "rs_commands.h"
#include <QMouseEvent>
#include <rs_coordinateevent.h>
#include <rs_commandevent.h>
#include <rs_graphicview.h>
#include <rs_document.h>
#include <rs_preview.h>
#include <rs_point.h>

LC_ActionDrawLinePoints::LC_ActionDrawLinePoints(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :LC_AbstractActionDrawLine("LineDrawPoints",container, graphicView){
    actionType = RS2::ActionDrawLinePoints;
}

LC_ActionDrawLinePoints::~LC_ActionDrawLinePoints(){}

/**
 * just cleanup if needed
 * @param status new status
 */
void LC_ActionDrawLinePoints::init(int status){
    RS_PreviewActionInterface::init(status);
    if (status == 0){
        point1Set = false;
        startpoint = RS_Vector(false);
        endpoint = RS_Vector(false);
    }
}

void LC_ActionDrawLinePoints::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    // prepare points data
    createEntities(endpoint, list);
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
    finishAction();
}

void LC_ActionDrawLinePoints::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    // determine candidate for possible end point
    RS_Vector possibleEndPoint;
    switch (status) {
        case SetStartPoint:
            return;
        case SetDirection:
        case SetPoint:
            possibleEndPoint = snap;
            break;
        case SetAngle:
            possibleEndPoint = snap;
            break;
        case SetDistance:
            switch (direction) {
                case DIRECTION_X:
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.y = startpoint.y;
                    possibleEndPoint.x = snap.x;
                    break;
                case DIRECTION_Y:
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.x = startpoint.x;
                    possibleEndPoint.y = snap.y;
                    break;
                case DIRECTION_POINT:
                    possibleEndPoint = snap;
                    break;
                case DIRECTION_ANGLE:
                    RS_Vector snap = snapPoint(e);
                    possibleEndPoint = LC_LineMath::calculateEndpointForAngleDirection(angleValue, startpoint, snap);
                    break;
            }
            break;
    }
    // draw preview if this is non-zero line
    if (isNonZeroLine(possibleEndPoint)){
        createEntities(possibleEndPoint, list);
    }
}

/**
 * Calculate positions of points specified on line betwen start point and given point
 * @param potentialEndPoint coordinates of end point *
 */
void LC_ActionDrawLinePoints::createEntities(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList){

    // determine angle of line
    double angle = startpoint.angleTo(potentialEndPoint);

    // calculate distance of line
    double distanceAll = startpoint.distanceTo(potentialEndPoint);

    // calculate length of single segment between points
    double segmentLength = distanceAll / (pointsCount + 1);

    // handle point for start edge of line
    bool includeStartPoint = edgePointsMode == DRAW_EDGE_START || edgePointsMode == DRAW_EDGE_BOTH;
    if (includeStartPoint){
        RS_Point* start = createPointEntity(startpoint);
        entitiesList << start;
    }

    // proceed with intermediate points
    for (int i = 1; i <= pointsCount; i++){
        // calc distance from start point to intermediate point
        double distanceFromStart = segmentLength * i;
        // define vector with needed distance and angle of line
        RS_Vector point = RS_Vector::polar(distanceFromStart, angle);
        // add that vector to start point
        point  = point + startpoint;
        RS_Point* inner = createPointEntity(point);
        entitiesList << inner;
    }

    // handle point for end edge of line
    bool includeEndPoint = edgePointsMode == DRAW_EDGE_END || edgePointsMode == DRAW_EDGE_BOTH;
    if (includeEndPoint){
        RS_Point* end = createPointEntity(potentialEndPoint);
        entitiesList << end;
    }
}

/**
 * options for the action
 */
void LC_ActionDrawLinePoints::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_LinePointsOptions>(nullptr);
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

void LC_ActionDrawLinePoints::onOnCoordinateEvent(const RS_Vector &mouse, bool isZero, int status){
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
                case DIRECTION_ANGLE:{ // calculate end point in given angle direction
                    RS_Vector possiblePoint = LC_LineMath::calculateEndpointForAngleDirection(angleValue, startpoint, mouse);
                    if (isNonZeroLine(possiblePoint)){
                        endpoint = possiblePoint;
                        trigger();
                    }
                    break;                }
                default:
                    break;
            }
            break;
        case SetDirection:
        case SetPoint: // set end to provided point
            if (isNonZeroLine(mouse)){
                // refuse zero length lines
                endpoint = mouse;
                trigger();
            }
            break;
        case SetStartPoint:{ // setup start point of line
            startpoint = mouse;
            point1Set = true;
            setStatus(SetPoint);
            graphicView->moveRelativeZero(mouse);
            break;
        }
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
 * @return true if comamnd is processed, false - if adidtional processing is needed
 */
bool LC_ActionDrawLinePoints::doProceedCommand(RS_CommandEvent *e, const QString &c){
    bool result = true;
    bool edgeStatus = getStatus() == SetEdge;
    if (checkCommand("edge_none", c)){
        updateEdgePointsMode(DRAW_EDGE_NONE);
    }
    else if (checkCommand("edge_start", c)){
        updateEdgePointsMode(DRAW_EDGE_START);
    }
    else if (checkCommand("edge_end", c)){
        updateEdgePointsMode(DRAW_EDGE_END);
    }
    else if (checkCommand("edge_both", c)){
        updateEdgePointsMode(DRAW_EDGE_BOTH);
    }
    else if (edgeStatus &&checkCommand("start",c)){
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
    else if (checkCommand("edges",c)){
        setStatus(SetEdge);
    }
    else if (checkCommand("number", c)){
        setSetNumberOfPointsState(false);
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
bool LC_ActionDrawLinePoints::doProcessCommandValue(RS_CommandEvent *e, const QString &c){
    bool result = true;
    switch (getStatus()) {
        case SetDirection:
            // processed earlier
            break;
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
                        endpoint = LC_LineMath::calculateAngleSegment(startpoint, angleValue, distance);
                        endpoint.valid = true;
                        trigger();
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
        case SetPointsCount: { // set amount of points
            bool ok = false;
            int count = RS_Math::eval(c, &ok);
            if (ok && count > 0){ // at least 1 point should be present
                updatePointsCount(count);
                setMajorStatus();
            } else {
                result = false;
            }
            break;
        }
        case SetAngle: { // process angle value
            processAngleValueInput(e, c);
            break;
        }
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
            updateMouseWidgetTR("Specify First Point","Cancel");
            break;
        case SetPoint:
            updateMouseWidgetTR("Specify Second Point or [number|x|y|angle|p|edges]","Back");
            break;
        case SetDirection:
            updateMouseWidgetTR("Specify line direction [x|y|angle|p]","Back");
            break;
        case SetAngle:
            updateMouseWidgetTR("Specify line direction angle or [x|y|p|number|edges]","Back");
            break;
        case SetEdge:
            updateMouseWidgetTR("Specify edge points mode [none|start|end|both]","Back");
            break;
        case SetDistance: {
            bool toX = direction == DIRECTION_X;
            bool toY = direction == DIRECTION_Y;
            msg += RS_COMMANDS->command("number")+"|";
            msg += RS_COMMANDS->command("angle")+"|";
            msg += RS_COMMANDS->command("p")+"|";
            msg += RS_COMMANDS->command("edges");
            if (toX){
                msg += "|" + RS_COMMANDS->command("y");
                updateMouseWidget(tr("Specify distance (%1) or [%2]").arg(tr("X"), msg),tr("Back"));
            } else if (toY){
                msg += "|" + RS_COMMANDS->command("x");
                updateMouseWidget(tr("Specify distance (%1) or [%2]").arg(tr("Y"), msg), tr("Back"));
            } else if (direction == DIRECTION_ANGLE){
                msg += "|" + RS_COMMANDS->command("x");
                msg += "|" + RS_COMMANDS->command("y");
                QString angleStr = RS_Math::doubleToString(angleValue, 1);
                updateMouseWidget(tr("Specify  distance (angle %1 deg) or [%2]").arg(angleStr, msg),tr("Back"));
            }
            break;
        }
        case SetPointsCount:
            updateMouseWidgetTR("Specify points count","Back");
            break;
        default:
            RS_ActionInterface::updateMouseButtonHints();
    }
}

/**
 * Right mouse button processing - doing back for specific state
 * @param e original event
 * @param status current status
 */
void LC_ActionDrawLinePoints::doBack(QMouseEvent *e, int status){
    switch (status) {
        case SetStartPoint: { // complete action
            finishAction();
            break;
        }
        default: { // return to set start point state
            // restore relative point to start point
            graphicView->moveRelativeZero(startpoint);
            init(SetStartPoint);
        }
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

RS_Point* LC_ActionDrawLinePoints::createPointEntity(const RS_Vector &point) const{
    RS_PointData pointEntityData(point);
    RS_Point* pointEntity = new RS_Point(container, pointEntityData);
    return pointEntity;
}

void LC_ActionDrawLinePoints::setEdgePointsMode(int value){
   edgePointsMode = value;
}


void LC_ActionDrawLinePoints::setSetNumberOfPointsState(bool b){
    setStatus(SetPointsCount);
    updateOptions();
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


void LC_ActionDrawLinePoints::setPointsCount(int value){
    pointsCount = value;
}

