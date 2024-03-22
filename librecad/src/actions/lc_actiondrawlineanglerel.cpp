

#include <cmath>

#include <QAction>
#include <QMouseEvent>
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "lc_actiondrawlineanglerel.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_preview.h"
#include "lc_undosection.h"
#include "rs_constructionline.h"
#include "lc_linemath.h"
#include "lc_lineanglereloptions.h"

// fixme - take care of specified snap point on line - for preview and positioning of tick
// fixme - add divide option
namespace {

//list of entity types supported by current action
    const auto enTypeList = EntityTypeList{RS2::EntityLine/*, RS2::EntityArc, RS2::EntityCircle,RS2::EntityEllipse*/};
}


LC_ActionDrawLineAngleRel:: LC_ActionDrawLineAngleRel(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView,
    double angle,
    bool fixedAngle)
    :LC_AbstractActionWithPreview("Draw Lines with relative angles",
                               container, graphicView)
    , tickSnapPosition(RS_Vector())
    , tickAngle(angle)
    , fixedAngle(fixedAngle)
    , line(nullptr)

{
    if (fixedAngle){
        relativeAngle = true;
    }
}

LC_ActionDrawLineAngleRel::~LC_ActionDrawLineAngleRel() = default;

RS2::ActionType LC_ActionDrawLineAngleRel::rtti() const{
    if( fixedAngle &&
        RS_Math::getAngleDifference(RS_Math::deg2rad(tickAngle), M_PI_2) < RS_TOLERANCE_ANGLE)
        return RS2::ActionDrawLineOrthogonalRel;
    else
        return RS2::ActionDrawLineAngleRel;
}

void LC_ActionDrawLineAngleRel::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    prepareLineData();
    RS_Line* en = new RS_Line{container, tickLineData};
    list<<en;
}

RS_Vector LC_ActionDrawLineAngleRel::doGetRelativeZeroAfterTrigger(){
    return tickLineData.endpoint;
}

void LC_ActionDrawLineAngleRel::doAfterTrigger(){
    unHighlightEntity();
    line = nullptr;
}

void LC_ActionDrawLineAngleRel::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed){

        switch (status) {
            case SetLine:{
                RS_Entity* en = catchEntity(e, enTypeList, RS2::ResolveAll);
                if (en != nullptr) {
                    line = dynamic_cast<RS_Line *>(en);
                    highlightEntity(line);

                    setLineSnapMode(lineSnapMode);
                    setStatus(SetSnapDistance);
                    tickEndPosition.valid = false;
                }
                break;
            }
            case SetSnapDistance:{
                RS_CoordinateEvent ce(snapPoint);
                coordinateEvent(&ce);
                break;
            }
            case SetTickLength:{
                if (lengthIsFree){
                    trigger();
                    setStatus(SetLine);
                }
                break;
            }
            default:
                break;
        }
}

void LC_ActionDrawLineAngleRel::doBack(QMouseEvent *pEvent, int status){
    // fixme by status
    LC_AbstractActionWithPreview::doBack(pEvent, status);
}

bool LC_ActionDrawLineAngleRel::doCheckMayDrawPreview(QMouseEvent *event, int status){
    return status == SetSnapDistance || (status == SetTickLength && lengthIsFree);
}

void LC_ActionDrawLineAngleRel::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    switch (status) {
        case SetSnapDistance: {
            // fixme - probably snap should take care line snap point (say, with SHIFT) - think about this
//            RS_Vector snap = snapPoint(e);
            tickSnapPosition = detectNearestPointOnLine(snap, true);
            prepareLineData();
            RS_Line *previewLine = new RS_Line{container, tickLineData};
            list << previewLine;
            break;
        }
        case SetTickLength: {
            if (lengthIsFree){
//                RS_Vector snap = snapPoint(e);
                double tickAngleRad = RS_Math::deg2rad(tickAngle);
                auto const vp = line->getNearestPointOnEntity(tickSnapPosition, false);
                double actualTickAngle = tickAngleRad;
                if (relativeAngle){
                    actualTickAngle = actualTickAngle + line->getTangentDirection(vp).angle();
                }

                RS_Vector infiniteTickStartPoint = tickSnapPosition;
                /// just create vector of length 10 using angle of tick
                RS_Vector infiniteTickVector = RS_Vector::polar(10.0, actualTickAngle);
                RS_Vector infiniteTickEndPoint = infiniteTickStartPoint + infiniteTickVector;
                RS_Vector pointOnInfiniteTick = LC_LineMath::getNearestPointOnInfiniteLine(snap, infiniteTickStartPoint, infiniteTickEndPoint);

                tickEndPosition = pointOnInfiniteTick;

                prepareLineData();
                RS_Line *previewLine = new RS_Line{container, tickLineData};
                list<< previewLine;
            }
            break;
        }
        default:
            break;
    }
}



void LC_ActionDrawLineAngleRel::prepareLineData(){

    double actualTickLength = tickLength;
    if (lengthIsFree){
        if (tickEndPosition.valid){
            RS_Vector tickVector = tickEndPosition - tickSnapPosition;
            tickLength = tickVector.magnitude();

            tickLength = LC_LineMath::getMeaningful(tickLength, 1.0);

            // determine sign of length

            RS_Vector point1 = line->getStartpoint();
            RS_Vector point2 = line->getEndpoint();

            int pointPosition = LC_LineMath::getPointPosition(point1,point2, tickEndPosition);
            if (pointPosition == LC_LineMath::RIGHT){
                if (tickSnapMode != SNAP_END){
                    actualTickLength = -actualTickLength;
                }
            }
            else if (pointPosition == LC_LineMath::LEFT){
                if (tickSnapMode == SNAP_END){
                    actualTickLength = -actualTickLength;
                }
            }
            if (tickSnapMode == SNAP_MIDDLE){
                actualTickLength = actualTickLength * 2;
            }
        }
    }

    auto const vp = line->getNearestPointOnEntity(tickSnapPosition, false);

    double tickAngleRad = RS_Math::deg2rad(tickAngle);

    double actualTickAngle = tickAngleRad;
    if (relativeAngle){
        actualTickAngle = actualTickAngle + line->getTangentDirection(vp).angle();
    }

    RS_Vector vectorOffset(0,0,0);
    RS_Vector vectorOffsetCorrection(0,0,0);

    if (LC_LineMath::isMeaningful(tickOffset)){
        vectorOffset = RS_Vector::polar(tickOffset, actualTickAngle);
    }

    // fixme - complete - similar logic as for finding nearest line on infinite line, yet based on start point and angle
    switch (tickSnapMode){
        case SNAP_START:
            break;
        case SNAP_END:
            vectorOffsetCorrection =  RS_Vector::polar(-actualTickLength, actualTickAngle);
            break;
        case SNAP_MIDDLE:
            vectorOffsetCorrection =  RS_Vector::polar(-actualTickLength/2, actualTickAngle);
            break;
    }

    tickLineData.startpoint = tickSnapPosition + vectorOffset + vectorOffsetCorrection;

    RS_Vector vectorTick = RS_Vector::polar(actualTickLength, actualTickAngle);

    tickLineData.endpoint = tickLineData.startpoint + vectorTick;
}

void LC_ActionDrawLineAngleRel::updateTickSnapPosition(double distanceOnLine){
    RS_Vector lineSnapPoint = obtainLineSnapPointForMode();

    const double lineAngle =  line->getTangentDirection(lineSnapPoint).angle();
    const RS_Vector snapVector = RS_Vector::polar(distanceOnLine, lineAngle);
    tickSnapPosition = lineSnapPoint + snapVector;
}
RS_Vector LC_ActionDrawLineAngleRel::detectNearestPointOnLine(const RS_Vector& coord, bool infiniteLine){
    return LC_LineMath::getNearestPointOnLine(line, coord, infiniteLine);
}


void LC_ActionDrawLineAngleRel::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) {
        return;
    }

    RS_Vector coord = e->getCoordinate();
    RS_Vector zero = RS_Vector(0, 0, 0);
    bool isZero = coord == zero;

    switch (getStatus()) {
        // additional handling of zero value - "0" is treated as shortcut for relative zero, and 0 coordinates are passed. So here we just hangle 0 value offset
        case SetTickOffset:
            if (isZero){
                tickOffset = 0;
                setStatus(SetSnapDistance);
            }
            break;

        case SetSnapDistance:
            if (isZero){
                double distance = 0;
                updateTickSnapPosition(distance);
            }
            if (lengthIsFree){
                setStatus(SetTickLength);
            }
            else{
               trigger();
               setStatus(SetLine);
            }
            break;

        default:
            break;
    }
}


void LC_ActionDrawLineAngleRel::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower().trimmed();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
        case SetLine:
        case SetSnapDistance:
            if (!fixedAngle && checkCommand("angle", c)) {
                deletePreview();
                setStatus(SetTickAngle);
                e->accept();
            } else if (checkCommand("length", c)) {
                deletePreview();
                setStatus(SetTickLength);
                e->accept();
            }
            else if (checkCommand("offset", c)) {
                deletePreview();
                setStatus(SetTickOffset);
                e->accept();
            }
            else if (checkCommand("linesnap", c)) {
                deletePreview();
                setStatus(SetLineSnap);
                e->accept();
            }
            else if (checkCommand("ticksnap", c)) {
                deletePreview();
                setStatus(SetTickSnap);
                e->accept();
            }
            else // offset is entered
            {
                bool ok = false;
                double distance = RS_Math::eval(c, &ok);
                if (ok){
                    e->accept();
                    updateTickSnapPosition(distance);
                    trigger();
                    setStatus(SetLine);
                }
                else {
                    commandMessageTR("Not a valid expression");
                }
            }
            break;

        case SetTickAngle: {
            bool ok = false;
            double a = RS_Math::eval(c, &ok);
            if (ok) {
                e->accept();
                tickAngle =a;
            } else {
                commandMessageTR("Not a valid expression");
            }
            updateOptions();
            setStatus(SetSnapDistance);
            break;
        }
        case SetTickLength: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                tickLength = l;
                e->accept();
                updateOptions();
                if (lengthIsFree){
                    trigger();
                    setStatus(SetLine);
                }
                else{
                    setStatus(SetSnapDistance);
                }
            } else {
                commandMessageTR("Not a valid expression");
            }
            break;
        }
        case SetTickOffset: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                tickOffset = l;
                e->accept();
                RS_DIALOGFACTORY->requestOptions(this, true, true);
                setStatus(SetSnapDistance);
            } else {
                commandMessageTR("Not a valid expression");
            }
            break;
        }
        case SetLineSnap: {
            int lineSnap = getSnapModeFromCommand(c);
            if (lineSnap > 0){
                setLineSnapMode(lineSnap);
                e->accept();
                setStatus(SetSnapDistance);
                updateOptions();
            }
            else {
               commandMessageTR("Not a valid expression");
            }
            break;
        }
        case SetTickSnap: {
            int lineSnap = getSnapModeFromCommand(c);
            if (lineSnap > 0){
                setTickSnapMode(lineSnap);
                e->accept();
                setStatus(SetSnapDistance);
                updateOptions();
            }
            else {
                commandMessage("Not a valid expression");
            }
            break;
        }
        default:
            break;
    }
}

int LC_ActionDrawLineAngleRel::getSnapModeFromCommand(QString &command){
    int result = -1;
    if (command == "s"){
        result = SNAP_START;
    }
    else if (command == "m"){
        result = SNAP_MIDDLE;
    }
    else if (command == "e"){
        result = SNAP_END;
    }
    return result;
}

void LC_ActionDrawLineAngleRel::setLineSnapMode(int mode){
    lineSnapMode = mode;

    if (line != nullptr){
        RS_Vector snapPoint = obtainLineSnapPointForMode();

        graphicView->moveRelativeZero(snapPoint);
        graphicView->redraw(RS2::RedrawOverlay);
    }
}

RS_Vector LC_ActionDrawLineAngleRel::obtainLineSnapPointForMode() const{
    RS_Vector snapPoint;

    switch (lineSnapMode) {
        case SNAP_START:
            snapPoint = line->getStartpoint();
            break;
        case SNAP_END:
            snapPoint = line->getEndpoint();
            break;
        case SNAP_MIDDLE:
            snapPoint = line->getMiddlePoint();
            break;
    }
    return snapPoint;
};

void LC_ActionDrawLineAngleRel::setTickSnapMode(int mode){tickSnapMode = mode;};

void LC_ActionDrawLineAngleRel::setTickAngle(double a){tickAngle = a;};

void LC_ActionDrawLineAngleRel::setTickLength(double len){tickLength = len;};

void LC_ActionDrawLineAngleRel::setTickOffset(double o){tickOffset = o;};

void LC_ActionDrawLineAngleRel::setSnapDistance(double d){snapDistance = d;};

void LC_ActionDrawLineAngleRel::setSegmentsCount(int c){segmentsCount = c;};

RS2::CursorType LC_ActionDrawLineAngleRel::doGetMouseCursor(int status){
    switch (status){
        case SetLine:
            return RS2::SelectCursor;
        default:
            return RS2::CadCursor;
    }
}

void LC_ActionDrawLineAngleRel::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_LineAngleRelOptions>(nullptr);
}

QStringList LC_ActionDrawLineAngleRel::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetSnapDistance:
        case SetTickLength:
            if (!fixedAngle) {
                cmd += command("angle");
            }
            cmd += command("length");
            break;
        default:
            break;
    }

    return cmd;
}


void LC_ActionDrawLineAngleRel::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetLine:
            updateMouseWidgetTR("Select base line", "Cancel");
            break;
        case SetLineSnap:
            updateMouseWidgetTR("Specify snap point (s,e,m)", "Back");
            break;
        case SetSnapDistance:
            updateMouseWidgetTR("Specify snap distance", "Back");
            break;
        case SetTickAngle:
            updateMouseWidgetTR("Specify angle", "Back");
            break;
        case SetTickLength:
            updateMouseWidgetTR("Specify length", "Back");
            break;
        case SetTickOffset:
            updateMouseWidgetTR("Specify offset", "Back");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void LC_ActionDrawLineAngleRel::finish(bool updateTB) {
    if (line != nullptr) {
        line->setHighlighted(false);
        graphicView->drawEntity(line);
    }
    RS_PreviewActionInterface::finish(updateTB);
}
