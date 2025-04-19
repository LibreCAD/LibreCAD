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
#include "lc_actiondrawlinesnake.h"

#include <QMouseEvent>

#include "lc_linemath.h"
#include "lc_lineoptions.h"

LC_ActionDrawLineSnake::LC_ActionDrawLineSnake(LC_ActionContext *actionContext,RS2::ActionType actionType)
    :LC_AbstractActionDrawLine("Draw line snake",actionContext, actionType)
    , m_actionData(new ActionData{}){
    switch (actionType) {
        case (RS2::ActionDrawSnakeLine): {
            m_primaryDirection = LC_ActionDrawLineSnake::DIRECTION_POINT;
            break;
        }
        case (RS2::ActionDrawSnakeLineX): {
            m_primaryDirection = LC_ActionDrawLineSnake::DIRECTION_X;
            break;
        }
        case (RS2::ActionDrawSnakeLineY): {
            m_primaryDirection = LC_ActionDrawLineSnake::DIRECTION_Y;
            break;
        }
        default: {
            m_primaryDirection = LC_ActionDrawLineSnake::DIRECTION_POINT;
            break;
        }
    }
    m_direction = m_primaryDirection;
}

LC_ActionDrawLineSnake::~LC_ActionDrawLineSnake() = default;

size_t LC_ActionDrawLineSnake::ActionData::index(const int offset /*= 0*/){
    return static_cast<size_t>( std::max(0, historyIndex + offset));
}

void LC_ActionDrawLineSnake::init(int status){
    if (status >= 0){
        resetPoints();
        // restore primary direction of action
        m_direction = m_primaryDirection;
    }
    LC_AbstractActionWithPreview::init(status);
}

void LC_ActionDrawLineSnake::resetPoints(){
    m_actionData.reset(new ActionData{});
}

/**
 * create line segment based on points data
 * @param list  list of entities to add created line
 */
void LC_ActionDrawLineSnake::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    auto *line = new RS_Line(m_container, m_actionData->data);
    list << line;
}

void LC_ActionDrawLineSnake::doSetStartPoint(RS_Vector start){
    m_actionData->startOffset = 0;
    m_actionData->data.startpoint = start;
    addHistory(HA_SetStartpoint, start, start, m_actionData->startOffset);

    //  adjust state and direction of action based on primary direction.
    // this is needef for horizontal/vertical line actions
    if (m_primaryDirection == DIRECTION_POINT){
        if (m_direction == DIRECTION_POINT){
            setStatus(SetPoint);
        }
        else{
            setStatus(SetDistance);
        }
    }
    else{
        m_direction = m_primaryDirection;
        setStatus(SetDistance);
    }
    moveRelativeZero(start);
    updateMouseButtonHints();
}

bool LC_ActionDrawLineSnake::doCheckMayDrawPreview([[maybe_unused]]LC_MouseEvent *pEvent, int status){
    return status != SetStartPoint; // can draw preview if at least start point is set
}

void LC_ActionDrawLineSnake::doPreparePreviewEntities([[maybe_unused]]LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    RS_Vector possibleEndPoint;
    QString directionName = "";
    switch (status) {
        case SetDirection:
            possibleEndPoint = snap;
            break;
        case SetPoint:
            possibleEndPoint = snap;
            directionName = tr("Point");
            break;
        case SetAngle: // draw line in direction specified by angle
            possibleEndPoint = calculateAngleEndpoint(snap);
            directionName = tr("Angle");
            break;
        case SetDistance: {
            switch (m_direction) {
                case DIRECTION_Y: {
                    // draw horizontal line segment, y-axis is fixed
                    possibleEndPoint = restrictVertical(m_actionData->data.startpoint, snap);
                    directionName = tr("Y");
                    break;
                }
                case DIRECTION_X: {
                    // draw vertical line segment, x-axis is fixed
                    possibleEndPoint = restrictHorizontal(m_actionData->data.startpoint, snap);
                    directionName = tr("X");
                    break;
                }
                case DIRECTION_POINT: { // free draw mode
                    possibleEndPoint = snap;
                    break;
                }
                case DIRECTION_ANGLE: {// draw segment in direction defined by angle
                    possibleEndPoint = calculateAngleEndpoint(snap);
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
    createEntities(possibleEndPoint, list);
    if (!directionName.isEmpty()){
        appendInfoCursorEntityCreationMessage(tr("Direction:") + directionName);
    }
    if (m_showRefEntitiesOnPreview) {
        createRefPoint(m_actionData->data.startpoint, list);
        createRefSelectablePoint(possibleEndPoint, list);
    }
}

RS_Vector LC_ActionDrawLineSnake::doGetRelativeZeroAfterTrigger(){
    return m_actionData->history.at(m_actionData->index()).currPt; // move relative end point to last point
}

void LC_ActionDrawLineSnake::createEntities(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList){
    auto *line = new RS_Line(m_actionData->data.startpoint, potentialEndPoint);
    entitiesList << line;
}

bool LC_ActionDrawLineSnake::isStartPointValid() const{
    return m_actionData->data.startpoint.valid;
}

const RS_Vector &LC_ActionDrawLineSnake::getStartPointForAngleSnap() const {
    return m_actionData->data.startpoint;
}

void LC_ActionDrawLineSnake::doBack(LC_MouseEvent *e, int status){
    e->originalEvent->accept();
    switch (status) {
        case SetStartPoint:
            finishAction();
            break;
        case SetPoint:
        case SetDistance:
        case SetDirection:
        case SetAngle: // skip last operations
            setStatus(SetStartPoint);
            m_actionData->data.startpoint = RS_Vector(false);
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
    return LC_LineMath::isNonZeroLineLength( m_actionData->data.startpoint, possiblePoint);
}

void LC_ActionDrawLineSnake::onCoordinateEvent(int status, [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetDistance:{
            switch (m_direction) {
                case DIRECTION_Y: {
                    // draw horizontal segment, fix start point y coordinate
                    RS_Vector possiblePoint = restrictVertical(m_actionData->data.startpoint, mouse);
                    if (isNonZeroLine(possiblePoint)){
                        m_actionData->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                    break;
                }
                case DIRECTION_X: {
                    // draw vertical segment, fix start point x coordinate
                    RS_Vector possiblePoint = restrictHorizontal(m_actionData->data.startpoint, mouse);
                    if (isNonZeroLine(possiblePoint)){
                        m_actionData->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                    break;
                }
                case DIRECTION_ANGLE:{
                    // draw segment in given angle direction
                    RS_Vector possiblePoint = calculateAngleEndpoint(mouse);
                    if (isNonZeroLine(possiblePoint)){
                        m_actionData->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SetDirection:
        case SetPoint: {
            if (isNonZeroLine(mouse)) {
                // refuse zero length lines
                m_actionData->data.endpoint = mouse;
                completeLineSegment(false);
            }
            break;
        }
        case SetStartPoint:{
            doSetStartPoint(mouse);
            break;
        }
        case SetAngle: {
            if (isZero){
                setAngleValue(0);
                // ask for distance after angle entering
                setStatus(SetDistance);
            }
            else {
                // draw segment in direction specified by angle
                RS_Vector possiblePoint = calculateAngleEndpoint(mouse);
                if (isNonZeroLine(possiblePoint)) {
                    m_actionData->data.endpoint = possiblePoint;
                    completeLineSegment(false);
                }
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
    ++m_actionData->startOffset;
    if (!close){
        addHistory(HA_SetEndpoint, m_actionData->data.startpoint, m_actionData->data.endpoint, m_actionData->startOffset);
    }
    trigger();
    m_actionData->data.startpoint = m_actionData->data.endpoint;

    switch (m_direction) {
        case DIRECTION_X: {
            // switch direction (snake)
            m_direction = DIRECTION_Y;
            setStatus(SetDistance);
            break;
        }
        case DIRECTION_Y: {
            // switch direction (snake)
            m_direction = DIRECTION_X;
            setStatus(SetDistance);
            break;
        }
        case DIRECTION_POINT:
            // stay in free direction
            m_direction = DIRECTION_POINT;
            setStatus(SetPoint);
            break;
        case DIRECTION_ANGLE:
            // stay in angle mode
            m_direction = DIRECTION_ANGLE;
            // by handle status differently for relative and absolute mode
            if (m_angleIsRelative){
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
                switch (m_direction) {
                    case DIRECTION_X: {// the value is for x coordinate adjustment
                        RS_Vector ucsStart = toUCS(m_actionData->data.startpoint);
                        RS_Vector ucsEndPoint = RS_Vector(ucsStart.x + distance, ucsStart.y);
                        m_actionData->data.endpoint = toWorld(ucsEndPoint);
                        completeLineSegment(false);
                        break;
                    }
                    case DIRECTION_Y: {// the value is for y coordinate adjustment
                        RS_Vector ucsStart = toUCS(m_actionData->data.startpoint);
                        RS_Vector ucsEndPoint = RS_Vector(ucsStart.x, ucsStart.y + + distance);
                        m_actionData->data.endpoint = toWorld(ucsEndPoint);
//                        pPoints->data.endpoint.x = pPoints->data.startpoint.x;
//                        pPoints->data.endpoint.y = pPoints->data.startpoint.y + distance;
                        completeLineSegment(false);
                        break;
                    }
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
    if (m_actionData->index() + 1 < m_actionData->history.size()){
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

            if (m_actionData->historyIndex >= 1){
                cmd += command("undo");
            }
            if (m_actionData->startOffset >= 2){
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

    if (m_actionData->startOffset >= 2){
        msg += "/";
        msg += command("close");
    }
    if (m_actionData->index() + 1 < m_actionData->history.size()){
        if (msg.size() > 0){
            msg += "/";
        }
        msg += command("redo");
    }
    bool hasHistory = m_actionData->historyIndex >= 1;
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
            bool toX = m_direction == DIRECTION_X;
            bool toY = m_direction == DIRECTION_Y;
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
            else if (m_direction == DIRECTION_ANGLE){
                msg += "/";
                msg += command("x");
                QString angleStr = RS_Math::doubleToString(m_angleDegrees, 1);
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
            updateMouseWidgetTRBack(tr("Specify angle or [%1]").arg(msg));
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
    addHistory(HA_Next, m_actionData->data.startpoint, m_actionData->data.endpoint, m_actionData->startOffset);
    setStatus(SetDirection);
}

// in-action undo
void LC_ActionDrawLineSnake::undo(){
    if (mayUndo()){
        History h(m_actionData->history.at(m_actionData->index()));

        --m_actionData->historyIndex;
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
                switchToAction(RS2::ActionEditUndo);
                m_actionData->data.startpoint = h.prevPt;
                setStatus(SetDirection);
                break;

            case HA_Next:
                m_actionData->data.startpoint = h.prevPt;
                setStatus(SetDirection);
                break;
        }

        // get index for close from new current history
        h = m_actionData->history.at(m_actionData->index());
        m_actionData->startOffset = h.startOffset;
    } else {
        commandMessage(tr("Cannot undo: Begin of history reached"));
    }
}

bool LC_ActionDrawLineSnake::mayUndo() const{return 0 <= m_actionData->historyIndex;}

void LC_ActionDrawLineSnake::redo(){
    if (mayRedo()){
        ++m_actionData->historyIndex;
        History h(m_actionData->history.at(m_actionData->index()));
        deletePreview();
        if (h.histAct != HA_Polyline){
            moveRelativeZero(h.currPt);
            m_actionData->data.startpoint = h.currPt;
            m_actionData->startOffset = h.startOffset;
        }
        switch (h.histAct) {
            case HA_SetStartpoint:
                setStatus(SetDirection);
                break;

            case HA_Polyline:
            case HA_SetEndpoint:
                switchToAction(RS2::ActionEditRedo);
                setStatus(SetDirection);
                break;

            case HA_Close:
                switchToAction(RS2::ActionEditRedo);
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
    if (m_actionData->historyIndex < -1){
        m_actionData->historyIndex = -1;
    }
    auto offset =m_actionData->historyIndex + 1;
    if (offset == 0) { // MSVC-specific workaround
        m_actionData->history.erase(m_actionData->history.begin(), m_actionData->history.end());
    }
    else {
        m_actionData->history.erase(m_actionData->history.begin()+offset, m_actionData->history.end());
    }
    m_actionData->history.push_back(History(a, p, c, s));
    m_actionData->historyIndex = static_cast<int>(m_actionData->history.size() - 1);
}

// closing sequence of lines
void LC_ActionDrawLineSnake::close(){
    if (mayClose()){
        History h(m_actionData->history.at(m_actionData->index(-m_actionData->startOffset)));
        if (LC_LineMath::isNonZeroLineLength(m_actionData->data.startpoint, h.currPt)){
            m_actionData->data.endpoint = h.currPt;
            addHistory(HA_Close, m_actionData->data.startpoint, m_actionData->data.endpoint, m_actionData->startOffset);
            completeLineSegment(true);
        }
    } else {
        commandMessage("Cannot close sequence of lines: Not enough entities defined yet, or already closed.");
    }
}

// creation of polyline. This will end line drawing sequence
void LC_ActionDrawLineSnake::polyline(){
    // fixme - add support of alternative way of polyline based on selected entities (so only drawn lines will be converted to polyline, without others found
    RS_Entity *en = catchEntity(m_actionData->data.endpoint, RS2::EntityLine, RS2::ResolveAllButTextImage);
    if (en != nullptr){
        finishAction();
        addHistory(HA_Polyline, m_actionData->data.startpoint, m_actionData->data.endpoint, m_actionData->startOffset);
        // fixme - sand - files - direct action creation
        switchToAction(RS2::ActionPolylineSegment, en);
    }
}

bool LC_ActionDrawLineSnake::mayClose(){
    return 1 < m_actionData->startOffset && 0 <= m_actionData->historyIndex - m_actionData->startOffset;
}

bool LC_ActionDrawLineSnake::mayRedo(){
    return m_actionData->history.size() > (m_actionData->index() + 1);
}

bool LC_ActionDrawLineSnake::mayStart(){
    return getStatus() == SetDistance || getStatus() == SetPoint;
}

double LC_ActionDrawLineSnake::defineActualSegmentAngle(double relativeAngleRad){
    size_t currentIndex = m_actionData->index();  // this should be start point of current line
    double ucsBasisAngle = relativeAngleRad;
    if (currentIndex > 0){
        History h(m_actionData->history.at(currentIndex));

        if (h.histAct == HA_SetEndpoint){ // this is start of previous line segment
            RS_Vector previousSegmentStart = h.prevPt;
            RS_Vector previousSegmentEnd = h.currPt;

            RS_Vector line = previousSegmentEnd - previousSegmentStart;
            double previousSegmentAngle = line.angle();

            ucsBasisAngle = relativeAngleRad + toUCSBasisAngle(previousSegmentAngle);
        }
    }
    double result = toWorldAngleFromUCSBasis(ucsBasisAngle);
    return result;
}

RS_Vector LC_ActionDrawLineSnake::calculateAngleEndpoint(const RS_Vector &snap){
    double ucsBasisAngleToUse = m_angleDegrees;
    if (m_alternativeActionMode){
        ucsBasisAngleToUse = 180 - m_angleDegrees;
    }

    double wcsAngle;
    if (m_angleIsRelative){
        double angleRadians = RS_Math::deg2rad(ucsBasisAngleToUse);
        wcsAngle = defineActualSegmentAngle(angleRadians);
    }
    else{
        wcsAngle = toWorldAngleFromUCSBasisDegrees(ucsBasisAngleToUse);
    }
    RS_Vector possibleEndPoint = LC_LineMath::calculateEndpointForAngleDirection(wcsAngle,m_actionData->data.startpoint, snap);
    return possibleEndPoint;
}


void LC_ActionDrawLineSnake::calculateAngleSegment(double distance){
    double wcsAngle;
    if (m_angleIsRelative){
        double angleRadians = RS_Math::deg2rad(m_angleDegrees);
        wcsAngle = defineActualSegmentAngle(angleRadians);
    }
    else{
        wcsAngle = toWorldAngleFromUCSBasisDegrees(m_angleDegrees);
    }
    m_actionData->data.endpoint = m_actionData->data.startpoint.relative(distance, wcsAngle);
}


LC_ActionOptionsWidget* LC_ActionDrawLineSnake::createOptionsWidget(){
    return new LC_LineOptions();
}
