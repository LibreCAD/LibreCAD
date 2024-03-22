//
// Created by sand1 on 15/02/2024.
//

#include "lc_actiondrawrectangle3points.h"
#include <QAction>
#include <QMouseEvent>
#include <cmath>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"    
#include "rs_polyline.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "rs_eventhandler.h"
#include "lc_actiondrawlineanglerel.h"
#include "lc_linemath.h"
#include "lc_rectangle3pointsoptions.h"

struct LC_ActionDrawRectangle3Points::Points {
/**
 * 1st corner.
 */
    RS_Vector corner1;
/**
 * 2nd corner.
 */
    RS_Vector corner2;

    RS_Vector corner3;

    RS_Vector corner4;
};

LC_ActionDrawRectangle3Points::LC_ActionDrawRectangle3Points(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
    :LC_AbstractActionDrawRectangle("Draw rectangles rel",
                               container, graphicView)
    , pPoints(std::make_unique<Points>())
{
    actionType=RS2::ActionDrawRectangle3Points;
}

LC_ActionDrawRectangle3Points::~LC_ActionDrawRectangle3Points() = default;


void LC_ActionDrawRectangle3Points::doAfterTrigger(){
    resetPoints();
    setStatus(SetPoint1);
}

RS_Vector LC_ActionDrawRectangle3Points::doGetRelativeZeroAfterTrigger(){
    RS_Vector zeroCorner;
    switch (endRelativeZeroPointCorner){
        case (SNAP_CORNER1):
            zeroCorner = pPoints->corner1;
            break;
        case (SNAP_CORNER2):
            zeroCorner = pPoints->corner2;
            break;
        case (SNAP_CORNER3):
            zeroCorner = pPoints->corner3;
            break;
        case (SNAP_CORNER4):
            zeroCorner = pPoints->corner4;
            break;
        default:
            zeroCorner = pPoints->corner2;
    }
    return zeroCorner;
}



// fixme - complete refactoring
RS_Polyline *LC_ActionDrawRectangle3Points::createPolyline(const RS_Vector &snapPoint) const{
    RS_Polyline* polyline = new RS_Polyline(this->container);

    if (snapPoint.valid){


        double angleRad = RS_Math::deg2rad(angle);
        int status = getStatus();
        switch (status) {
            case SetPoint2:
                pPoints->corner2 = snapPoint;
                pPoints->corner3 = pPoints->corner2;
                break;
            case SetWidth:
                pPoints->corner2 = calculatePossibleEndpointForAngle(snapPoint, pPoints->corner1, angleRad);
                pPoints->corner3 = pPoints->corner2;
                break;
            case SetPoint3: {
                double baseAngle = pPoints->corner1.angleTo(pPoints->corner2);
                if (createQuadrangle){
                    if (innerAngleIsFixed){
                        double innerAngleRad = RS_Math::deg2rad(innerAngle);
                        double actualAngle = baseAngle + innerAngleRad;
                        pPoints->corner3 = calculatePossibleEndpointForAngle(snapPoint, pPoints->corner2, actualAngle + M_PI / 2);
                    } else {
                        pPoints->corner3 = snapPoint;
                    }
                } else {
                    pPoints->corner3 = calculatePossibleEndpointForAngle(snapPoint, pPoints->corner2, baseAngle + M_PI / 2);
                }
                calculateCorner4();
                break;
            }
            case SetHeight: {
                pPoints->corner3 = calculatePossibleEndpointForAngle(snapPoint, pPoints->corner2, angleRad + M_PI / 2);
                calculateCorner4();
                break;
            }
            default:
                break;
        }

        bool drawPrimitiveShape = true;

        switch (status) {
            case SetPoint3:
            case SetHeight: {
                drawPrimitiveShape = createQuadrangle;
                break;
            }
        }

        bool shouldInspectForNonCompleteShape = cornersDrawMode == CORNER_STRAIGHT && edgesDrawMode != EDGES_BOTH;

        if (!drawPrimitiveShape){
            drawPrimitiveShape = shouldInspectForNonCompleteShape;
        }

        if (drawPrimitiveShape){
            polyline->addVertex(this->pPoints->corner1);
            polyline->addVertex(this->pPoints->corner2);
            polyline->addVertex(this->pPoints->corner3);
            polyline->addVertex(this->pPoints->corner4);
            polyline->setClosed(true);
            polyline->endPolyline();
        }
        else{
            double  baseAngle = pPoints->corner1.angleTo(pPoints->corner2);

            bool rotate = false;

            if (LC_LineMath::isMeaningfulAngle(baseAngle)){
                rotate = true;
            }
        }
    }
    return polyline;
}

void LC_ActionDrawRectangle3Points::setMainStatus(){

}

void LC_ActionDrawRectangle3Points::processCommandValue(double value){

}

bool LC_ActionDrawRectangle3Points::processCustomCommand(RS_CommandEvent *e, const QString &command, bool &toMainStatus){
    return false;
}

bool LC_ActionDrawRectangle3Points::doCheckMayDrawPreview(QMouseEvent *event, int status){
    return status != SetPoint1 && pPoints->corner1.valid;
}

RS_Vector LC_ActionDrawRectangle3Points::doGetMouseSnapPoint(QMouseEvent *e, bool shiftPressed){
    RS_Vector snapped = snapPoint(e);
    // Snapping to angle(15*) if shift key is pressed
    if (shiftPressed){
        int status = getStatus();
        switch (status){
            case (SetPoint2):
               snapped = snapToAngle(snapped, pPoints->corner1);
               break;
            case (SetPoint3):{
                if (createQuadrangle && !innerAngleIsFixed){
                    double angle = pPoints->corner1.angleTo(pPoints->corner2);
                    snapped = snapToRelativeAngle(angle, snapped, pPoints->corner2);
                }
                else{ // draw square
                    // width of rect
                    double width = pPoints->corner2.distanceTo(pPoints->corner1);
                    // angle for base edge
                    double baseAngle = pPoints->corner1.angleTo(pPoints->corner2);

                    RS_Vector tmpPoint = pPoints->corner2;

                    // end point of base edge rotated to be parallel to x axis
                    tmpPoint.rotate(pPoints->corner1, -baseAngle);
                    // set height equal to width
                    if (snapped.y > pPoints->corner2.y){ // mouse above corner 2
                        tmpPoint.y = tmpPoint.y + width;
                    }
                    else{ // mouse below corner 2
                        tmpPoint.y = tmpPoint.y - width;
                    }

                    // rotate back to get snap point
                    tmpPoint.rotate(pPoints->corner1, baseAngle);

                    snapped = tmpPoint;
                }
                break;
            }
        }
    }
    return snapped;
}

int LC_ActionDrawRectangle3Points::doRelZeroInitialSnapState(){
    return SetPoint1;
}

void LC_ActionDrawRectangle3Points::doRelZeroInitialSnap(RS_Vector relZero){
    pPoints->corner1 = relZero;
    pPoints->corner2 = relZero;
    pPoints->corner3 = relZero;
    pPoints->corner4 = relZero;
    setStatus(SetPoint2);
}

void LC_ActionDrawRectangle3Points::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed){
    onCoordinateEvent(snapPoint, false, status);
}

void LC_ActionDrawRectangle3Points::doFinish(bool updateTB){
    resetPoints();
    widthIsSet = false;
}

void LC_ActionDrawRectangle3Points::doBack(QMouseEvent *pEvent, int status){
    switch (status){
        case (SetPoint1):{
            finishAction();
            break;
        }
        case (SetPoint3):{
            graphicView->moveRelativeZero(pPoints->corner1);
            pPoints->corner3 = pPoints->corner1;
            pPoints->corner4 = pPoints->corner1;
            setStatus(SetPoint2);
            widthIsSet = false;
            break;
        }
        case (SetPoint2):{
            setStatus(SetPoint1);
            widthIsSet = false;
            break;
        }
        default:
            init(SetPoint1);
            break;
    }
}

void LC_ActionDrawRectangle3Points::init(int status){
    if (status >= 0){
        resetPoints();
    }
    RS_PreviewActionInterface::init(status);
}

void LC_ActionDrawRectangle3Points::resetPoints(){
      RS_Vector zero = RS_Vector(false);
      doResetPoints(zero);
}

void LC_ActionDrawRectangle3Points::doResetPoints(const RS_Vector &zero){
    this->pPoints->corner1 = zero;
    this->pPoints->corner2 = zero;
    this->pPoints->corner3 = zero;
    this->pPoints->corner4 = zero;

    widthIsSet = false;
}

RS_Vector LC_ActionDrawRectangle3Points::calculateAngleEndpoint(const RS_Vector& startPoint, double angle, double length){
    RS_Vector line = RS_Vector::polar(length, angle);
    return startPoint + line;
}

RS_Vector LC_ActionDrawRectangle3Points::calculatePossibleEndpointForAngle(const RS_Vector &snap, const RS_Vector lineStartPoint, double angle) const{
    RS_Vector possibleEndPoint;

    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, angle);
    RS_Vector infiniteTickEndPoint = lineStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick = LC_LineMath::getNearestPointOnInfiniteLine(snap, lineStartPoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}

void LC_ActionDrawRectangle3Points::toHeightExpectedState(){
    widthIsSet = true;
    setStatus(SetHeight);
}

void LC_ActionDrawRectangle3Points::toWidthExpectedState(){
    widthIsSet = false;
    setStatus(SetWidth);
}

void LC_ActionDrawRectangle3Points::doProcessCoordinateEvent(const RS_Vector &mouse, bool zero, int status){


    double angleRad = RS_Math::deg2rad(angle);
    switch (getStatus()) {
        case SetPoint1: {
            doResetPoints(mouse);
            graphicView->moveRelativeZero(mouse);
//            toWidthExpectedState();
            setStatus(SetPoint2);
            widthIsSet = false;
            break;
        }
        case SetWidth: {
            pPoints->corner2 = calculatePossibleEndpointForAngle(mouse, pPoints->corner1, angleRad);
            pPoints->corner3 = pPoints->corner2;

            graphicView->moveRelativeZero(pPoints->corner2);

            deletePreview();
            RS_Polyline *polyline = createPolyline(RS_Vector(false));
            preview->addEntity(polyline);
            drawPreview();
            toHeightExpectedState();
            break;
        }
        case SetHeight: {
            pPoints->corner3 = calculatePossibleEndpointForAngle(mouse, pPoints->corner2, angleRad + M_PI / 2);
            calculateCorner4();

            trigger();
            toWidthExpectedState();
            break;
        }
        case SetPoint2: {
            graphicView->moveRelativeZero(mouse);
//            toWidthExpectedState();
            setStatus(SetPoint3);
            widthIsSet = true;
            break;
        }

        case SetAngle:{ // special handle for "0" value that denotes zero
            RS_Vector relativeZero = RS_Vector(0,0,0);
            bool isRelativeZero = mouse == relativeZero;
            if (isRelativeZero){
                angle = 0;
                setStatusAfterAngleValue();
                break;
            }
        }
        default:
            break;
    }
}

void LC_ActionDrawRectangle3Points::setStatusAfterAngleValue(){
    if (widthIsSet){
        this->setStatus(SetHeight);
    }
    else{
        this->setStatus(SetWidth);
    }
}

void LC_ActionDrawRectangle3Points::commandEvent(RS_CommandEvent* e) {
    QString const& c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        e->accept();
        return;
    }
    else if (checkCommand("start",c)){
        e->accept();
        setStatus(SetPoint1);
        return;
    }
    else if (checkCommand("angle",c)){
        e->accept();
        setStatus(SetAngle);
        return;
    }
    else{
        bool ok = false;
        double value = RS_Math::eval(c, &ok);
        if (ok){
            e->accept();
            switch (getStatus()){
                case SetWidth: {
                    double angleRad = RS_Math::deg2rad(angle);
                    pPoints->corner2 = calculateAngleEndpoint(pPoints->corner1, angleRad, value);
                    pPoints->corner3 = pPoints->corner2;
                    graphicView->moveRelativeZero(pPoints->corner2);
                    deletePreview();
                    RS_Polyline *polyline = createPolyline(RS_Vector(false));
                    preview->addEntity(polyline);
                    drawPreview();
                    toHeightExpectedState();
                    break;
                }
                case SetHeight: {
                    double angleRad = RS_Math::deg2rad(angle);
                    pPoints->corner3 = calculateAngleEndpoint(pPoints->corner2, angleRad  + M_PI / 2, value);
                    calculateCorner4();
                    trigger();
                    trigger();
                    toWidthExpectedState();
                    break;
                }
                case SetAngle: {
                    angle = value;
                    setStatusAfterAngleValue();
                    break;
                }
            }
        }
    }
}

void LC_ActionDrawRectangle3Points::calculateCorner4() const{
    RS_Vector tangentBase = this->pPoints->corner2 - this->pPoints->corner1;
    this->pPoints->corner4 = this->pPoints->corner3 - tangentBase;
}

void LC_ActionDrawRectangle3Points::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetWidth:
            updateMouseWidgetTR("Specify width","Cancel");
            break;
        case SetHeight:
            updateMouseWidgetTR("Specify height","Back");
            break;
        case SetPoint1:
            updateMouseWidgetTR("Specify start point","Back");
            break;
        case SetPoint2:
            updateMouseWidgetTR("Specify second point","Back");
            break;
        case SetPoint3:
            updateMouseWidgetTR("Specify third point","Back");
            break;
        case SetAngle:
            updateMouseWidgetTR("Specify angle","Back");
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

// fixme - add support of UI options for width and height

void LC_ActionDrawRectangle3Points::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

void LC_ActionDrawRectangle3Points::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_Rectangle3PointsOptions>(nullptr);
}

void LC_ActionDrawRectangle3Points::setStartState(){
    setStatus(SetPoint1);
}




