/****************************************************************************
**
* Experimental direct-line action: click start, aim with mouse, type length,
* press Enter to place segment in aimed direction, auto-continue.
*
* Adapted from LC_ActionDrawLineSnake.
* Original copyright (C) 2024 LibreCAD.org / sand1024
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation; either version 2 of the License, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
****************************************************************************/

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
#include "lc_actiondrawlinedirect.h"
#include "lc_archparser.h"
#include "rs_settings.h"

LC_ActionDrawLineDirect::LC_ActionDrawLineDirect(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    : LC_AbstractActionDrawLine("Draw Direct Line", container, graphicView)
    , pPoints(new Points{})
{
    primaryDirection = DIRECTION_POINT;
    direction = DIRECTION_POINT;
    actionType = RS2::ActionDrawLineDirect;
}

LC_ActionDrawLineDirect::~LC_ActionDrawLineDirect() = default;

size_t LC_ActionDrawLineDirect::Points::index(const int offset /*= 0*/){
    return static_cast<size_t>(std::max(0, historyIndex + offset));
}

void LC_ActionDrawLineDirect::init(int status){
    if (status >= 0){
        resetPoints();
        direction = primaryDirection;
    }
    LC_AbstractActionWithPreview::init(status);
}

void LC_ActionDrawLineDirect::resetPoints(){
    pPoints.reset(new Points{});
}

void LC_ActionDrawLineDirect::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    auto *line = new RS_Line(container, pPoints->data);
    list << line;
}

void LC_ActionDrawLineDirect::doSetStartPoint(RS_Vector start){
    pPoints->startOffset = 0;
    pPoints->data.startpoint = start;
    addHistory(HA_SetStartpoint, start, start, pPoints->startOffset);
    direction = DIRECTION_POINT;
    setStatus(SetPoint);
    moveRelativeZero(start);
    updateMouseButtonHints();
}

bool LC_ActionDrawLineDirect::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *pEvent, int status){
    return status != SetStartPoint;
}

RS_Vector LC_ActionDrawLineDirect::doGetMouseSnapPoint(QMouseEvent *e) {
    RS_Vector snapped = snapPoint(e);
    // Only apply angle snap when there is no active object/grid snap result and a start point exists.
    // If snapPoint found a real snap point it will differ from the raw mouse position.
    RS_Vector rawMouse = graphicView->toGraph(e->x(), e->y());
    bool freeSnap = snapped.squaredTo(rawMouse) <= RS_TOLERANCE;
    if (freeSnap && isStartPointValid() && (snapMode.snapAngle || alternativeActionMode)) {
        double angle = 15.0;
        if (snapMode.snapAngle) {
            auto guard = RS_SETTINGS->beginGroupGuard("/Defaults");
            bool ok = false;
            double cfg = RS_SETTINGS->readEntry("/PolarSnapAngle", "15").toDouble(&ok);
            if (ok && cfg > 0.0) angle = cfg;
        }
        snapped = snapToAngle(snapped, getStartPointForAngleSnap(), angle);
    }
    return snapped;
}

void LC_ActionDrawLineDirect::doPreparePreviewEntities(
    [[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    RS_Vector possibleEndPoint;

    switch (status) {
        case SetDirection:
        case SetPoint:
            possibleEndPoint = snap;
            break;
        case SetAngle:
            possibleEndPoint = calculateAngleEndpoint(snap);
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
                    possibleEndPoint = calculateAngleEndpoint(snap);
                    break;
            }
            break;
        default:
            break;
    }
    createEntities(possibleEndPoint, list);
}

RS_Vector LC_ActionDrawLineDirect::doGetRelativeZeroAfterTrigger(){
    return pPoints->history.at(pPoints->index()).currPt;
}

void LC_ActionDrawLineDirect::createEntities(
    RS_Vector &potentialEndPoint, QList<RS_Entity *> &entitiesList){
    auto *line = new RS_Line(pPoints->data.startpoint, potentialEndPoint);
    entitiesList << line;
}

bool LC_ActionDrawLineDirect::isStartPointValid() const{
    return pPoints->data.startpoint.valid;
}

const RS_Vector &LC_ActionDrawLineDirect::getStartPointForAngleSnap() const {
    return pPoints->data.startpoint;
}

void LC_ActionDrawLineDirect::doBack(QMouseEvent *e, int status){
    e->accept();
    switch (status) {
        case SetStartPoint:
            finishAction();
            break;
        case SetPoint:
        case SetDistance:
        case SetDirection:
        case SetAngle:
            setStatus(SetStartPoint);
            pPoints->data.startpoint = RS_Vector(false);
            updateOptions();
            break;
        default:
            finishAction();
    }
}

bool LC_ActionDrawLineDirect::isNonZeroLine(const RS_Vector &possiblePoint) const{
    return LC_LineMath::isNonZeroLineLength(pPoints->data.startpoint, possiblePoint);
}

void LC_ActionDrawLineDirect::onCoordinateEvent(
    const RS_Vector &mouse, [[maybe_unused]]bool isZero, int status){
    switch (status) {
        case SetStartPoint:
            doSetStartPoint(mouse);
            break;
        case SetPoint:
            if (isNonZeroLine(mouse)){
                pPoints->data.endpoint = mouse;
                completeLineSegment(false);
            }
            break;
        case SetDistance:
            switch (direction) {
                case DIRECTION_X: {
                    RS_Vector possiblePoint(mouse.x, pPoints->data.startpoint.y);
                    if (isNonZeroLine(possiblePoint)){
                        pPoints->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                    break;
                }
                case DIRECTION_Y: {
                    RS_Vector possiblePoint(pPoints->data.startpoint.x, mouse.y);
                    if (isNonZeroLine(possiblePoint)){
                        pPoints->data.endpoint = possiblePoint;
                        completeLineSegment(false);
                    }
                    break;
                }
                case DIRECTION_ANGLE: {
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
            break;
        case SetAngle: {
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

void LC_ActionDrawLineDirect::completeLineSegment(bool close){
    ++pPoints->startOffset;
    if (!close){
        addHistory(HA_SetEndpoint, pPoints->data.startpoint, pPoints->data.endpoint,
                   pPoints->startOffset);
    }
    trigger();
    pPoints->data.startpoint = pPoints->data.endpoint;

    switch (direction) {
        case DIRECTION_X:
            direction = DIRECTION_Y;
            setStatus(SetDistance);
            break;
        case DIRECTION_Y:
            direction = DIRECTION_X;
            setStatus(SetDistance);
            break;
        case DIRECTION_POINT:
            direction = DIRECTION_POINT;
            setStatus(SetPoint);
            break;
        case DIRECTION_ANGLE:
            direction = DIRECTION_ANGLE;
            if (angleIsRelative){
                setStatus(SetDistance);
            } else {
                setStatus(SetAngle);
            }
            break;
        default:
            setStatus(SetPoint);
            break;
    }
    updateOptions();
    updateMouseButtonHints();
}

bool LC_ActionDrawLineDirect::doProceedCommand(
    [[maybe_unused]]RS_CommandEvent *e, const QString &c){
    bool result = true;
    if (checkCommand("close", c)){
        close();
        updateMouseButtonHints();
    } else if (checkCommand("undo", c)){
        undo();
        updateMouseButtonHints();
    } else if (checkCommand("polyline", c) || checkCommand("pl", c)){
        polyline();
        updateMouseButtonHints();
    } else if (checkCommand("redo", c)){
        redo();
        updateMouseButtonHints();
    } else if (checkCommand("start", c)){
        setNewStartPointState();
    } else {
        result = false;
    }
    return result;
}

bool LC_ActionDrawLineDirect::doProcessCommandValue(RS_CommandEvent *e, const QString &c){
    bool result = true;
    switch (getStatus()) {
        case SetDirection:
            break;
        case SetPoint: {
            // Typed distance in free-aim mode: compute endpoint along current mouse direction.
            // When architectural input is enabled, try the arch parser first; fall back to eval.
            bool ok = false;
            double distance = 0.0;
            {
                auto guard = RS_SETTINGS->beginGroupGuard("/Defaults");
                bool archEnabled = RS_SETTINGS->readNumEntry("/ArchitecturalInput", 0) != 0;
                if (archEnabled) {
                    distance = LC_ArchParser::parse(c, &ok);
                }
            }
            if (!ok) {
                distance = RS_Math::eval(c, &ok);
            }
            if (ok && LC_LineMath::isMeaningful(distance) && isNonZeroLine(lastSnapPoint)){
                double aimAngle = (lastSnapPoint - pPoints->data.startpoint).angle();
                pPoints->data.endpoint = pPoints->data.startpoint.relative(distance, aimAngle);
                completeLineSegment(false);
            } else {
                result = false;
            }
            break;
        }
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
                    case DIRECTION_ANGLE:
                        calculateAngleSegment(distance);
                        completeLineSegment(false);
                        break;
                    default:
                        break;
                }
            } else {
                result = false;
            }
            break;
        }
        case SetAngle:
            result = processAngleValueInput(e, c);
            break;
        default:
            result = false;
            break;
    }
    return result;
}

QStringList LC_ActionDrawLineDirect::getAvailableCommands(){
    QStringList cmd;
    cmd += command("pl");
    if (pPoints->index() + 1 < pPoints->history.size()){
        cmd += command("redo");
    }
    if (pPoints->historyIndex >= 1){
        cmd += command("undo");
    }
    if (pPoints->startOffset >= 2){
        cmd += command("close");
    }
    return cmd;
}

void LC_ActionDrawLineDirect::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetStartPoint:
            updateMouseWidgetTR("Specify first point", "Cancel");
            break;
        case SetPoint: {
            QString msg = "pl";
            if (pPoints->startOffset >= 2){
                msg += "/";
                msg += getCommand("close");
            }
            if (pPoints->index() + 1 < pPoints->history.size()){
                msg += "/";
                msg += getCommand("redo");
            }
            if (pPoints->historyIndex >= 1){
                msg += "/";
                msg += getCommand("undo");
            }
            updateMouseWidget(tr("Specify point or distance [%1]").arg(msg), tr("Back"));
            break;
        }
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void LC_ActionDrawLineDirect::next(){
    addHistory(HA_Next, pPoints->data.startpoint, pPoints->data.endpoint, pPoints->startOffset);
    setStatus(SetPoint);
}

void LC_ActionDrawLineDirect::undo(){
    if (mayUndo()){
        History h(pPoints->history.at(pPoints->index()));
        --pPoints->historyIndex;
        deletePreview();
        if (h.histAct != HA_Polyline){
            moveRelativeZero(h.prevPt);
        }
        switch (h.histAct) {
            case HA_SetStartpoint:
                setStatus(SetPoint);
                break;
            case HA_Polyline:
            case HA_SetEndpoint:
            case HA_Close:
                graphicView->setCurrentAction(new RS_ActionEditUndo(true, *container, *graphicView));
                pPoints->data.startpoint = h.prevPt;
                setStatus(SetPoint);
                break;
            case HA_Next:
                pPoints->data.startpoint = h.prevPt;
                setStatus(SetPoint);
                break;
        }
        h = pPoints->history.at(pPoints->index());
        pPoints->startOffset = h.startOffset;
    } else {
        commandMessageTR("Cannot undo: Begin of history reached");
    }
}

bool LC_ActionDrawLineDirect::mayUndo() const { return 0 <= pPoints->historyIndex; }

void LC_ActionDrawLineDirect::redo(){
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
                setStatus(SetPoint);
                break;
            case HA_Polyline:
            case HA_SetEndpoint:
                graphicView->setCurrentAction(new RS_ActionEditUndo(false, *container, *graphicView));
                setStatus(SetPoint);
                break;
            case HA_Close:
                graphicView->setCurrentAction(new RS_ActionEditUndo(false, *container, *graphicView));
                setStatus(SetPoint);
                break;
            case HA_Next:
                setStatus(SetPoint);
                break;
        }
    } else {
        commandMessageTR("Cannot redo: End of history reached");
    }
}

void LC_ActionDrawLineDirect::addHistory(
    LC_ActionDrawLineDirect::HistoryAction a,
    const RS_Vector &p,
    const RS_Vector &c,
    const int s){
    if (pPoints->historyIndex < -1){
        pPoints->historyIndex = -1;
    }
    pPoints->history.erase(
        pPoints->history.begin() + pPoints->historyIndex + 1, pPoints->history.end());
    pPoints->history.push_back(History(a, p, c, s));
    pPoints->historyIndex = static_cast<int>(pPoints->history.size() - 1);
}

void LC_ActionDrawLineDirect::close(){
    if (mayClose()){
        History h(pPoints->history.at(pPoints->index(-pPoints->startOffset)));
        if (LC_LineMath::isNonZeroLineLength(pPoints->data.startpoint, h.currPt)){
            pPoints->data.endpoint = h.currPt;
            addHistory(HA_Close, pPoints->data.startpoint, pPoints->data.endpoint,
                       pPoints->startOffset);
            completeLineSegment(true);
        }
    } else {
        commandMessage(
            "Cannot close sequence of lines: Not enough entities defined yet, or already closed.");
    }
}

void LC_ActionDrawLineDirect::polyline(){
    RS_Entity *en = catchEntity(
        pPoints->data.endpoint, RS2::EntityLine, RS2::ResolveAllButTextImage);
    if (en != nullptr){
        finishAction();
        addHistory(HA_Polyline, pPoints->data.startpoint, pPoints->data.endpoint,
                   pPoints->startOffset);
        RS_ActionPolylineSegment *polylineSegmentAction =
            new RS_ActionPolylineSegment(*container, *graphicView, en);
        graphicView->setCurrentAction(polylineSegmentAction);
    }
}

bool LC_ActionDrawLineDirect::mayClose(){
    return 1 < pPoints->startOffset && 0 <= pPoints->historyIndex - pPoints->startOffset;
}

bool LC_ActionDrawLineDirect::mayRedo(){
    return pPoints->history.size() > (pPoints->index() + 1);
}

bool LC_ActionDrawLineDirect::mayStart(){
    return getStatus() == SetPoint;
}

void LC_ActionDrawLineDirect::calculateAngleSegment(double distance){
    double angleRadians = RS_Math::deg2rad(angle);
    double realAngle = defineActualSegmentAngle(angleRadians);
    pPoints->data.endpoint = pPoints->data.startpoint.relative(distance, realAngle);
}

double LC_ActionDrawLineDirect::defineActualSegmentAngle(double realAngle){
    if (angleIsRelative){
        size_t currentIndex = pPoints->index();
        if (currentIndex > 0){
            History h(pPoints->history.at(currentIndex));
            if (h.histAct == HA_SetEndpoint){
                RS_Vector previousSegmentStart = h.prevPt;
                RS_Vector previousSegmentEnd   = h.currPt;
                RS_Vector line = previousSegmentEnd - previousSegmentStart;
                double previousSegmentAngle = line.angle();
                realAngle = realAngle + previousSegmentAngle;
            }
        }
    }
    return realAngle;
}

RS_Vector LC_ActionDrawLineDirect::calculateAngleEndpoint(const RS_Vector &snap){
    double angleToUse = angle;
    if (alternativeActionMode){
        angleToUse = 180.0 - angle;
    }
    double angleRadians = RS_Math::deg2rad(angleToUse);
    RS_Vector infiniteTickStartPoint = pPoints->data.startpoint;
    double realAngle = defineActualSegmentAngle(angleRadians);
    RS_Vector infiniteTickVector   = RS_Vector::polar(10.0, realAngle);
    RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
    return LC_LineMath::getNearestPointOnInfiniteLine(
        snap, infiniteTickStartPoint, infiniteTickEndPoint);
}
