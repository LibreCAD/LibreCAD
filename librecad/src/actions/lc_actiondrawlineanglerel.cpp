

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

namespace {

//list of entity types supported by current action
    const auto enTypeList = EntityTypeList{RS2::EntityLine/*, RS2::EntityArc, RS2::EntityCircle,RS2::EntityEllipse*/};
}


LC_ActionDrawLineAngleRel:: LC_ActionDrawLineAngleRel(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView,
    double angle,
    bool fixedAngle)
    :RS_PreviewActionInterface("Draw Lines with relative angles",
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


void LC_ActionDrawLineAngleRel::trigger() {
    RS_PreviewActionInterface::trigger();

    deletePreview();

    prepareLineData();

    RS_Line* en = new RS_Line{container, tickLineData};
    en->setLayerToActive();
    en->setPenToActive();

    if (container) {
        container->addEntity(en);
    }

    if (document) {
        document->startUndoCycle();
        document->addUndoable(en);
        document->endUndoCycle();
    }

    if (graphicView) {
        graphicView->moveRelativeZero(tickLineData.endpoint);
        line->setHighlighted(false);
        graphicView->drawEntity(line);
        graphicView->drawEntity(en);
    }
    line = nullptr;
}

enum {LEFT,  RIGHT,  BEYOND,  BEHIND, BETWEEN, ORIGIN, DESTINATION};


int LC_ActionDrawLineAngleRel::getPointPosition(RS_Vector &startPos, RS_Vector &endPos, RS_Vector &point)
{
    RS_Vector a = endPos - startPos; // 1
    RS_Vector b = point - startPos; // 2
    double sa = a. x * b.y - b.x * a.y; // 3
    if (sa > 0.0)
        return LEFT;
    if (sa < 0.0)
        return RIGHT;
    if ((a.x * b.x < 0.0) || (a.y * b.y < 0.0))
        return BEHIND;
    if (a.magnitude() < b.magnitude())
        return BEYOND;
    if (startPos == point)
        return ORIGIN;
    if (endPos == point)
        return DESTINATION;
    return BETWEEN;
}


void LC_ActionDrawLineAngleRel::prepareLineData(){

    double actualTickLength = tickLength;
    if (lengthIsFree){
        if (tickEndPosition.valid){
            RS_Vector tickVector = tickEndPosition - tickSnapPosition;
            tickLength = tickVector.magnitude();

            if (std::abs(tickLength) < RS_TOLERANCE) tickLength = 1.0;

            // determine sign of length

            RS_Vector point1 = line->getStartpoint();
            RS_Vector point2 = line->getEndpoint();

            int pointPosition = getPointPosition(point1,point2, tickEndPosition);
            if (pointPosition == RIGHT){
                if (tickSnapMode != SNAP_END){
                    actualTickLength = -actualTickLength;
                }
            }
            else if (pointPosition == LEFT){
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

    if (std::abs(tickOffset) > RS_TOLERANCE){
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

void LC_ActionDrawLineAngleRel::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
            case SetLine:
            {
                RS_Entity* en = catchEntity(e, enTypeList, RS2::ResolveAll);
                if (en != nullptr) {
                    line = dynamic_cast<RS_Line *>(en);
                    line->setHighlighted(true);
                    graphicView->drawEntity(line);

                    setLineSnapMode(lineSnapMode);
                    setStatus(SetSnapDistance);
                    tickEndPosition.valid = false;
                }
            }
            break;
            case SetSnapDistance:
            {
                RS_CoordinateEvent ce(snapPoint(e));
                coordinateEvent(&ce);
            }
            break;
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
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        if (line) {
            line->setHighlighted(false);
            graphicView->drawEntity(line);
        }
        init(getStatus()-1);
    }
}

void LC_ActionDrawLineAngleRel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineRelAngle::mouseMoveEvent begin");

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));


    switch (getStatus()) {
        case SetLine: {
            RS_Entity *entity = catchEntity(e, enTypeList, RS2::ResolveAll);
            if (entity != nullptr){
                if (entity->rtti() == RS2::EntityLine){
                    line = dynamic_cast<RS_Line *>(entity);
                }
            }
        }
        break;
        case SetSnapDistance: {

            RS_Vector snap = snapPoint(e);

            tickSnapPosition = detectNearestPointOnLine(snap, true);

            deletePreview();

            prepareLineData();
            RS_Line* previewLine = new RS_Line{container, tickLineData};
            preview->addEntity(previewLine);

            drawPreview();
        }
        break;
        case SetTickLength:{
            if (lengthIsFree){
                RS_Vector snap = snapPoint(e);
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

                RS_Vector pointOnInfiniteTick = getNearestPointOnInfiniteLine(snap, infiniteTickStartPoint, infiniteTickEndPoint);

                tickEndPosition = pointOnInfiniteTick;

                deletePreview();

                prepareLineData();
                RS_Line* previewLine = new RS_Line{container, tickLineData};
                preview->addEntity(previewLine);

                drawPreview();
            }
            break;

        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDrawLineRelAngle::mouseMoveEvent end");
}
RS_Vector LC_ActionDrawLineAngleRel::detectNearestPointOnLine(const RS_Vector& coord, bool infiniteLine){
    return getNearestPointOnLine(line, coord, infiniteLine);
}


// todo - actually this function is good candidate for some generic math utils class
RS_Vector LC_ActionDrawLineAngleRel::getNearestPointOnLine(RS_Line* baseLIne, const RS_Vector& coord, bool infiniteLine){
    if (infiniteLine){

        RS_Vector point1 = baseLIne->getStartpoint();
        RS_Vector point2 = baseLIne->getEndpoint();

        return getNearestPointOnInfiniteLine(coord, point1, point2);

    }
    else{
       return baseLIne->getNearestPointOnEntity(coord, true, nullptr);
    }
}

RS_Vector LC_ActionDrawLineAngleRel::getNearestPointOnInfiniteLine(const RS_Vector &coord, const RS_Vector &lineStartPoint, const RS_Vector &lineEndPoint){
    RS_Vector ae = lineEndPoint - lineStartPoint;
    RS_Vector ea = lineStartPoint-lineEndPoint;
    RS_Vector ap = coord-lineStartPoint;

    if (ae.magnitude()<RS_TOLERANCE|| ea.magnitude()<RS_TOLERANCE) {
           return RS_Vector(false);
     }

    // Orthogonal projection from both sides:
    RS_Vector ba = ae * RS_Vector::dotP(ae, ap)
                   / (ae.magnitude()*ae.magnitude());

    return lineStartPoint+ba;
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
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
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
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(SetSnapDistance);
        }
            break;

        case SetTickLength: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                tickLength = l;
                e->accept();
                RS_DIALOGFACTORY->requestOptions(this, true, true);
                if (lengthIsFree){
                    trigger();
                    setStatus(SetLine);
                }
                else{
                    setStatus(SetSnapDistance);
                }
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
            break;

        case SetTickOffset: {
            bool ok = false;
            double l = RS_Math::eval(c, &ok);
            if (ok) {
                tickOffset = l;
                e->accept();
                RS_DIALOGFACTORY->requestOptions(this, true, true);
                setStatus(SetSnapDistance);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
            break;

        case SetLineSnap: {
            int lineSnap = getSnapModeFromCommand(c);
            if (lineSnap > 0){
                setLineSnapMode(lineSnap);
                e->accept();
                setStatus(SetSnapDistance);
                RS_DIALOGFACTORY->requestOptions(this, true, true);
            }
            else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            }
            break;

        case SetTickSnap: {
            int lineSnap = getSnapModeFromCommand(c);
            if (lineSnap > 0){
                setTickSnapMode(lineSnap);
                e->accept();
                setStatus(SetSnapDistance);
                RS_DIALOGFACTORY->requestOptions(this, true, true);
            }
            else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
        }
            break;


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


void LC_ActionDrawLineAngleRel::updateMouseCursor(){
    switch (getStatus())
    {
        case SetLine:
            graphicView->setMouseCursor(RS2::SelectCursor);
            break;
        default:
            graphicView->setMouseCursor(RS2::CadCursor);
            break;
    }
}

void LC_ActionDrawLineAngleRel::showOptions(){
    RS_DEBUG->print("LC_ActionDrawLineAngleRel::showOptions");
    RS_ActionInterface::showOptions();
    // fixme - update widget factory for options
    RS_DIALOGFACTORY->requestOptions(this, true);
}

void LC_ActionDrawLineAngleRel::hideOptions(){
    RS_ActionInterface::hideOptions();
    RS_DIALOGFACTORY->requestOptions(this, false);
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
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select base line"),
                                                tr("Cancel"));
            break;
        case SetLineSnap:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify snap point (s,e,m)"),
                                                tr("Back"));
            break;
        case SetSnapDistance:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify snap distance"),
                                                tr("Back"));
            break;
        case SetTickAngle:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify angle"),
                                                tr("Back"));
            break;
        case SetTickLength:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify length"),
                                                tr("Back"));
            break;
        case SetTickOffset:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify offset"),
                                                tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
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
