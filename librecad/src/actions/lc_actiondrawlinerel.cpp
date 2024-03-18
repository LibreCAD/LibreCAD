#include "lc_actiondrawlinerel.h"
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
#include "lc_lineoptions.h"
#include "lc_linemath.h"
#include "lc_abstract_action_draw_line.h"
#include <QMouseEvent>

LC_ActionDrawLineRel::LC_ActionDrawLineRel(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView,
    int initialDirection)
    :LC_AbstractActionDrawLine("Draw lines rel",
                               container, graphicView)
//    , pPoints(std::make_unique<Points>())
    , pPoints(new Points{}){
    direction = initialDirection;
    actionType = RS2::ActionDrawLineRel;
}

size_t LC_ActionDrawLineRel::Points::index(const int offset /*= 0*/){
    return static_cast<size_t>( std::max(0, historyIndex + offset));
}

LC_ActionDrawLineRel::~LC_ActionDrawLineRel() = default;

void LC_ActionDrawLineRel::init(int status){
    if (status >= 0){
        resetPoints();
        if (direction != DIRECTION_NONE){
            status = SetDistance;
        }
    } else {
        direction = DIRECTION_NONE;
    }
    RS_PreviewActionInterface::init(status);
}

void LC_ActionDrawLineRel::resetPoints(){
    pPoints.reset(new Points{});
    RS_Vector zero = this->graphicView->getRelativeZero();
    pPoints->data.startpoint = zero;
    pPoints->startOffset = 0;
    addHistory(HA_SetStartpoint, zero, zero, pPoints->startOffset);
}

void LC_ActionDrawLineRel::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    RS_Line *line = new RS_Line(container, pPoints->data);
    list << line;
}

void LC_ActionDrawLineRel::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    RS_Vector possibleEndPoint;

    switch (status) {
        case SetDirection:
        case SetPoint:
            possibleEndPoint = snap;
            break;
            // fixme -think how to handle preview
        case SetAngle:
            possibleEndPoint = snap;
            break;
        case SetDistance:
            switch (direction) {
                case DIRECTION_X:
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.y = pPoints->data.startpoint.y;
                    possibleEndPoint.x = snap.x;
                    break;
                case DIRECTION_Y:
                    possibleEndPoint = RS_Vector(snap);
                    possibleEndPoint.x = pPoints->data.startpoint.x;
                    possibleEndPoint.y = snap.y;
                    break;
                case DIRECTION_POINT:
                    possibleEndPoint = snap;
                    break;
                case DIRECTION_ANGLE:
                    RS_Vector snap = snapPoint(e);
                    possibleEndPoint = calculateAngleEndpoint(snap);
                    break;
            }
            break;
    }
    createEntities(possibleEndPoint, list);
}

RS_Vector LC_ActionDrawLineRel::doGetRelativeZeroAfterTrigger(){
    return pPoints->history.at(pPoints->index()).currPt;
}

void LC_ActionDrawLineRel::createEntities(RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList){
    RS_Line *line = new RS_Line(pPoints->data.startpoint, potentialEndPoint);
    entitiesList << line;
}

bool LC_ActionDrawLineRel::isStartPointValid() const{return pPoints->data.startpoint.valid;}

const RS_Vector &LC_ActionDrawLineRel::getStartPointForAngleSnap() const { return pPoints->data.startpoint;}

void LC_ActionDrawLineRel::doBack(QMouseEvent *e, int status){
    e->accept();
    switch (status) {
        default:
        case SetDirection:
            init(getStatus() - 1);
            break;
        case SetPoint:
        case SetDistance:
            next();
            break;
    }
}

bool LC_ActionDrawLineRel::isNonZeroLine(const RS_Vector &possiblePoint) const{
    return LC_LineMath::isNonZeroLineLength( pPoints->data.startpoint, possiblePoint);
}

void LC_ActionDrawLineRel::onOnCoordinateEvent(const RS_Vector &mouse, bool isZero, int status){
    switch (status) {
        case SetDistance:
            switch (direction) {
                case DIRECTION_X: {
                    RS_Vector possiblePoint(mouse.x, pPoints->data.startpoint.y);
                    if (isNonZeroLine(possiblePoint)){
                        pPoints->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                }
                    break;
                case DIRECTION_Y: {
                    RS_Vector possiblePoint(pPoints->data.startpoint.x, mouse.y);
                    if (isNonZeroLine(possiblePoint)){
                        pPoints->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                }
                break;
                case DIRECTION_ANGLE:{
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
            pPoints->startOffset = 0;
            pPoints->data.startpoint = mouse;
            addHistory(HA_SetStartpoint, graphicView->getRelativeZero(), mouse, pPoints->startOffset);
            if (direction == DIRECTION_NONE){
               setStatus(SetDirection);
            }
            else{
               setStatus(SetDistance);
            }
            graphicView->moveRelativeZero(mouse);
            updateMouseButtonHints();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineRel::completeLineSegment(bool close){
    ++pPoints->startOffset;
    if (!close){
        addHistory(HA_SetEndpoint, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
    }
    trigger();
    pPoints->data.startpoint = pPoints->data.endpoint;

    switch (direction) {
        case DIRECTION_X: {
            direction = DIRECTION_Y;
            setStatus(SetDistance);
            break;
        }
        case DIRECTION_Y: {
            direction = DIRECTION_X;
            setStatus(SetDistance);
            break;
        }
        case DIRECTION_POINT:
            direction = DIRECTION_POINT;
            setStatus(SetPoint);
            break;
        case DIRECTION_ANGLE:
            direction = DIRECTION_ANGLE;
            setStatus(SetAngle);
            break;
        default:
            setStatus(SetDirection);
            break;
    }
    updateMouseButtonHints();
}

bool LC_ActionDrawLineRel::doProceedCommand(RS_CommandEvent *e, const QString &c){
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

bool LC_ActionDrawLineRel::doProcessCommandValue(RS_CommandEvent *e, const QString &c){
    bool result = true;
    switch (getStatus()) {
        case SetDirection:
            break;
        case SetDistance: {
            bool ok = false;
            double distance = RS_Math::eval(c, &ok);
            if (ok && LC_LineMath::isMeaningful(distance)){
                switch (direction) {
                    case DIRECTION_X:
                        pPoints->data.endpoint.x = pPoints->data.startpoint.x + distance;
                        pPoints->data.endpoint.y = pPoints->data.startpoint.y;
                        completeLineSegment(false);
                        break;
                    case DIRECTION_Y:
                        pPoints->data.endpoint.x = pPoints->data.startpoint.x;
                        pPoints->data.endpoint.y = pPoints->data.startpoint.y + distance;
                        completeLineSegment(false);
                        break;
                    case DIRECTION_ANGLE: {
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
            result = processAngleValueInput(e, c);
            break;
        }
    }
    return result;
}


QStringList LC_ActionDrawLineRel::getAvailableCommands(){
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

void LC_ActionDrawLineRel::updateMouseButtonHints(){
    QString msg = "pl";

    if (pPoints->startOffset >= 2){
        msg += "/";
        msg += RS_COMMANDS->command("close");
    }
    if (pPoints->index() + 1 < pPoints->history.size()){
        if (msg.size() > 0){
            msg += "/";
        }
        msg += RS_COMMANDS->command("redo");
    }
    bool hasHistory = pPoints->historyIndex >= 1;
    if (hasHistory){
        if (msg.size() > 0){
            msg += "/";
        }
        msg += RS_COMMANDS->command("undo");
    }

    switch (getStatus()) {
        case SetStartPoint:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                                tr("Cancel"));
            break;
        case SetDirection:
            msg += "/";
            msg += RS_COMMANDS->command("p");
            msg += "/";
            msg += RS_COMMANDS->command("angle");
            msg += "/";
            msg += RS_COMMANDS->command("anglerel");
            updateMouseWidget(tr("Specify direction (x or y) or [%1]").arg(msg),tr("Back"));
            break;
        case SetDistance: {
            bool toX = direction == DIRECTION_X;
            bool toY = direction == DIRECTION_Y;
            msg += "/";
            msg += RS_COMMANDS->command("p");
            msg += "/";
            msg += RS_COMMANDS->command("angle");
            msg += "/";
            msg += RS_COMMANDS->command("anglerel");
            if (toX){
                msg += "/";
                msg += RS_COMMANDS->command("y");
                RS_DIALOGFACTORY->updateMouseWidget(tr("Specify distance (%1) or [%2]").arg(tr("X"), msg),
                                                    tr("Back"));
            } else if (toY){
                msg += "/";
                msg += RS_COMMANDS->command("x");
                updateMouseWidget(tr("Specify distance (%1) or [%2]").arg(tr("Y"), msg),tr("Back"));
            }
            else if (direction == DIRECTION_ANGLE){
                msg += "/";
                msg += RS_COMMANDS->command("x");
                QString angleStr = RS_Math::doubleToString(angleValue, 1);
                updateMouseWidget(tr("Specify distance (%1 deg) or [%2]").arg(angleStr, msg),tr("Back"));
            }
            break;
        }
        case SetAngle:
        {
            msg += "/";
            msg += RS_COMMANDS->command("x");
            msg += "/";
            msg += RS_COMMANDS->command("y");
            msg += "/";
            msg += RS_COMMANDS->command("p");
            msg += "/";
            msg += RS_COMMANDS->command("angle");
            msg += "/";
            msg += RS_COMMANDS->command("anglerel");
            updateMouseWidget(tr("Specify angle or [%2]").arg(msg),tr("Back"));
            break;
        }
        case SetPoint: {
            msg += "/";
            msg += RS_COMMANDS->command("x");
            msg += "/";
            msg += RS_COMMANDS->command("y");
            msg += "/";
            msg += RS_COMMANDS->command("angle");
            msg += "/";
            msg += RS_COMMANDS->command("anglerel");
            updateMouseWidget(tr("Specify point or [%1]").arg(msg),tr("Back"));

            break;
        }
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void LC_ActionDrawLineRel::next(){
    addHistory(HA_Next, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
    setStatus(SetDirection);
}

void LC_ActionDrawLineRel::undo(){
    if (mayUndo()){
        History h(pPoints->history.at(pPoints->index()));

        --pPoints->historyIndex;
        deletePreview();

        if (h.histAct != HA_Polyline){
            graphicView->moveRelativeZero(h.prevPt);
        }

        switch (h.histAct) {
            case HA_SetStartpoint:
                setStatus(SetDirection);
                break;

            case HA_Polyline    :
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
        commandMessageTR("Cannot undo: Begin of history reached");
    }
}

bool LC_ActionDrawLineRel::mayUndo() const{return 0 <= pPoints->historyIndex;}

void LC_ActionDrawLineRel::redo(){
    if (mayRedo()){
        ++pPoints->historyIndex;
        History h(pPoints->history.at(pPoints->index()));
        deletePreview();
        if (h.histAct != HA_Polyline){
            graphicView->moveRelativeZero(h.currPt);
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
        commandMessageTR("Cannot redo: End of history reached");
    }
}

void LC_ActionDrawLineRel::addHistory(LC_ActionDrawLineRel::HistoryAction a, const RS_Vector &p, const RS_Vector &c, const int s){
    if (pPoints->historyIndex < -1){
        pPoints->historyIndex = -1;
    }

    pPoints->history.erase(pPoints->history.begin() + pPoints->historyIndex + 1, pPoints->history.end());
    pPoints->history.push_back(History(a, p, c, s));
    pPoints->historyIndex = static_cast<int>(pPoints->history.size() - 1);
}

void LC_ActionDrawLineRel::close(){
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

void LC_ActionDrawLineRel::polyline(){
    // fixme - add support of alternative way of polyline based on selected entities (so only drawn lines will be converted to polyline, without others found
    RS_Entity *en = catchEntity(pPoints->data.endpoint, RS2::EntityLine, RS2::ResolveAllButTextImage);
    if (en != nullptr){
        // fixme - should the action be deleted??
        RS_ActionPolylineSegment *polylineSegmentAction = new RS_ActionPolylineSegment(*container, *graphicView, en);
        graphicView->setCurrentAction(polylineSegmentAction);
        addHistory(HA_Polyline, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
        setStatus(-1);
    }
}

bool LC_ActionDrawLineRel::mayClose(){
    return 1 < pPoints->startOffset && 0 <= pPoints->historyIndex - pPoints->startOffset;
}

bool LC_ActionDrawLineRel::mayRedo(){
    return pPoints->history.size() > (pPoints->index() + 1);
}

bool LC_ActionDrawLineRel::mayStart(){
    return getStatus() == SetDistance || getStatus() == SetPoint;
}

void LC_ActionDrawLineRel::calculateAngleSegment(double distance){

    double angle = RS_Math::deg2rad(angleValue);
    double realAngle = defineActualSegmentAngle(angle);
    RS_Vector line = RS_Vector::polar(distance, realAngle);
    pPoints->data.endpoint = pPoints->data.startpoint + line;
}

double LC_ActionDrawLineRel::defineActualSegmentAngle(double realAngle){
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

RS_Vector LC_ActionDrawLineRel::calculateAngleEndpoint(const RS_Vector &snap){
    RS_Vector possibleEndPoint;
    double angle = RS_Math::deg2rad(angleValue);
    RS_Vector infiniteTickStartPoint = pPoints->data.startpoint;
    double realAngle = defineActualSegmentAngle(angle);

    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, realAngle);
    RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick =  LC_LineMath::getNearestPointOnInfiniteLine(snap, infiniteTickStartPoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}



void LC_ActionDrawLineRel::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_LineOptions>(nullptr);
}


