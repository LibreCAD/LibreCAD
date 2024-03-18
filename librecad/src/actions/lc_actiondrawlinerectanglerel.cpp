//
// Created by sand1 on 15/02/2024.
//

#include "lc_actiondrawlinerectanglerel.h"
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

struct LC_ActionDrawLineRectangleRel::Points {
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

LC_ActionDrawLineRectangleRel::LC_ActionDrawLineRectangleRel(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
    :RS_PreviewActionInterface("Draw rectangles rel",
                               container, graphicView)
    , pPoints(std::make_unique<Points>())
{
    actionType=RS2::ActionDrawLineRectangleRel;
}

LC_ActionDrawLineRectangleRel::~LC_ActionDrawLineRectangleRel() = default;


void LC_ActionDrawLineRectangleRel::trigger() {
    RS_PreviewActionInterface::trigger();


    RS_Polyline *polyline = createPolyline();

    container->addEntity(polyline);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(polyline);
        document->endUndoCycle();
    }

    // upd. view
    graphicView->redraw(RS2::RedrawDrawing);

    // define end relative point
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
    graphicView->moveRelativeZero(zeroCorner);
    resetPoints();
}



void LC_ActionDrawLineRectangleRel::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineRectangle::mouseMoveEvent begin");

    deletePreview();
    RS_Vector mouse = snapPoint(e);
    bool doDrawPreview = true;
    if (pPoints->corner1.valid) {
        double angleRad = RS_Math::deg2rad(angle);
        switch (getStatus()) {
            case SetWidth:
                pPoints->corner2 = calculatePossibleEndpointForAngle(mouse, pPoints->corner1, angleRad);
                pPoints->corner3 = pPoints->corner2;
                break;
            case SetHeight: {
                pPoints->corner3 = calculatePossibleEndpointForAngle(mouse, pPoints->corner2, angleRad + M_PI / 2);
                calculateCorner4();
                break;
            }
            case SetStart:
                doDrawPreview = false;
                break;

            default:
                break;
        }

        if (doDrawPreview){
            RS_Polyline *polyline = createPolyline();
            preview->addEntity(polyline);
            drawPreview();
        }
    }

    RS_DEBUG->print("RS_ActionDrawLineRectangle::mouseMoveEvent end");
}

RS_Polyline *LC_ActionDrawLineRectangleRel::createPolyline() const{
    RS_Polyline* polyline = new RS_Polyline(this->container);

    polyline->addVertex(this->pPoints->corner1);
    polyline->addVertex(this->pPoints->corner2);
    polyline->addVertex(this->pPoints->corner3);
    polyline->addVertex(this->pPoints->corner4);
    polyline->setClosed(true);
    polyline->endPolyline();

    polyline->setLayerToActive();
    polyline->setPenToActive();
    return polyline;
}

void LC_ActionDrawLineRectangleRel::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        switch (getStatus()) {
            case SetWidth:
                graphicView->moveRelativeZero(pPoints->corner1);
                break;
            case SetHeight:
                graphicView->moveRelativeZero(pPoints->corner1);
                break;
        }
        init(getStatus()-1);
    }
}

void LC_ActionDrawLineRectangleRel::init(int status){
    if (status >= 0){
        resetPoints();
    }
    RS_PreviewActionInterface::init(status);
}

void LC_ActionDrawLineRectangleRel::resetPoints(){
      RS_Vector zero = this->graphicView->getRelativeZero();
      doResetPoints(zero);
}

void LC_ActionDrawLineRectangleRel::doResetPoints(const RS_Vector &zero){
    this->pPoints->corner1 = zero;
    this->pPoints->corner2 = zero;
    this->pPoints->corner3 = zero;
    this->pPoints->corner4 = zero;

    widthIsSet = false;
}

RS_Vector LC_ActionDrawLineRectangleRel::calculateAngleEndpoint(const RS_Vector& startPoint, double angle, double length){
    RS_Vector line = RS_Vector::polar(length, angle);
    return startPoint + line;
}

RS_Vector LC_ActionDrawLineRectangleRel::calculatePossibleEndpointForAngle(const RS_Vector &snap, const RS_Vector lineStartPoint, double angle){
    RS_Vector possibleEndPoint;

    RS_Vector infiniteTickVector = RS_Vector::polar(10.0, angle);
    RS_Vector infiniteTickEndPoint = lineStartPoint + infiniteTickVector;
    RS_Vector pointOnInfiniteTick = LC_LineMath::getNearestPointOnInfiniteLine(snap, lineStartPoint, infiniteTickEndPoint);

    possibleEndPoint = pointOnInfiniteTick;
    return possibleEndPoint;
}

void LC_ActionDrawLineRectangleRel::toHeightExpectedState(){
    widthIsSet = true;
    setStatus(SetHeight);
}

void LC_ActionDrawLineRectangleRel::toWidthExpectedState(){
    widthIsSet = false;
    setStatus(SetWidth);
}

void LC_ActionDrawLineRectangleRel::coordinateEvent(RS_CoordinateEvent* e) {
    if (!e) return;

    RS_Vector mouse = e->getCoordinate();

    double angleRad = RS_Math::deg2rad(angle);
    switch (getStatus()) {
        case SetWidth: {
            pPoints->corner2 = calculatePossibleEndpointForAngle(mouse, pPoints->corner1, angleRad);
            pPoints->corner3 = pPoints->corner2;

            graphicView->moveRelativeZero(pPoints->corner2);

            deletePreview();
            RS_Polyline *polyline = createPolyline();
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
        case SetStart: {
            doResetPoints(mouse);
            graphicView->moveRelativeZero(mouse);
            toWidthExpectedState();
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

void LC_ActionDrawLineRectangleRel::setStatusAfterAngleValue(){
    if (widthIsSet){
        this->setStatus(SetHeight);
    }
    else{
        this->setStatus(SetWidth);
    }
}

void LC_ActionDrawLineRectangleRel::commandEvent(RS_CommandEvent* e) {
    QString const& c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        e->accept();
        return;
    }
    else if (checkCommand("start",c)){
        e->accept();
        setStatus(SetStart);
        return;
    }
    else if (checkCommand("angle",c)){
        e->accept();
        setStatus(SetAngle);
        return;
    }
    else{
        bool ok = false;
        double value = RS_Math::eval(updateForFraction(c), &ok);
        if (ok){
            e->accept();
            switch (getStatus()){
                case SetWidth: {
                    double angleRad = RS_Math::deg2rad(angle);
                    pPoints->corner2 = calculateAngleEndpoint(pPoints->corner1, angleRad, value);
                    pPoints->corner3 = pPoints->corner2;
                    graphicView->moveRelativeZero(pPoints->corner2);
                    deletePreview();
                    RS_Polyline *polyline = createPolyline();
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

void LC_ActionDrawLineRectangleRel::calculateCorner4(){
    RS_Vector tangentBase = this->pPoints->corner2 - this->pPoints->corner1;
    this->pPoints->corner4 = this->pPoints->corner3 - tangentBase;
}

void LC_ActionDrawLineRectangleRel::updateMouseButtonHints() {
    switch (getStatus()) {
        // fixme - restore
     /*   case SetWidth:
            updateMouseWidgetTR("Specify width","Cancel");
            break;
        case SetHeight:
            updateMouseWidgetTR("Specify height","Back");
            break;
        case SetStart:
            updateMouseWidgetTR("Specify start point","Back");
            break;
        case SetAngle:
            updateMouseWidgetTR("Specify angle","Back");
            break;*/
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

// fixme - add support of UI options for width and height

void LC_ActionDrawLineRectangleRel::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}


// fixme - refactor. it's copy from rs_handler
QString LC_ActionDrawLineRectangleRel::evaluateFraction(QString input, QRegExp rx, int index, int tailI)
{
    QString copy = input;
    QString tail =QString{R"(\)"} + QString::number(tailI);

    int pos = 0;
    if ((pos = rx.indexIn(copy, pos)) != -1) {
        LC_ERR<<"Evaluate: "<<copy;
        LC_ERR<<"pos="<<pos<<", rx.matchedLength()="<<rx.matchedLength();
        QString formula = ((index != 2) ? rx.cap(2) + "+" : QString{}) + rx.cap(index) + "/" + rx.cap(index + 1);
        LC_ERR<<"formula="<<formula;
        QString value = QString{}.setNum(RS_Math::eval(formula));
        LC_ERR<<"formula="<<formula<<": value="<<value;
        return input.left(pos)
                + input.mid(pos, rx.matchedLength()).replace(rx, R"( \1)" + value + tail)
                + evaluateFraction(input.right(input.size() - pos - rx.matchedLength()), rx, index, tailI);
    }
    return input;
}

/**
 * @{description}       Update a length string to support fraction
 *                      (1 1/2") to (1+1/2")
 *                      (1"1/2) to (1+1/2")
*/
QString LC_ActionDrawLineRectangleRel::updateForFraction(QString input) {
    // support fraction at the end: (1'1/2) => (1 1/2')
    QRegExp rx{R"((\D*)([\d]+)\s*(['"])([\d]+)/([\d]+)\s*$)"};
    int pos = 0;
    if ((pos = rx.indexIn(input, pos)) != -1) {
        input = input.left(pos) + rx.cap(1) + rx.cap(2) + " " + rx.cap(4) + "/" + rx.cap(5) + rx.cap(3);
    }
    std::vector<std::tuple<QRegExp, int, int>> regexps{{
            {QRegExp{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*([\D$]))"}, 3, 5},
            {QRegExp{R"((\D*)([\d]+)\s+([\d]+)/([\d]+)\s*(['"]))"}, 3, 5},
            {QRegExp{R"((\D*)\s*([\d]+)/([\d]+)\s*([\D$]))"}, 2, 4},
        }};
    LC_LOG<<"input="<<input;
    for(auto& [rx, index, tailI] : regexps)
        input = evaluateFraction(input, rx, index, tailI).replace(QRegExp(R"(\s+)"), QString{});
    LC_LOG<<"eval: "<<input;
    return input;
}

void LC_ActionDrawLineRectangleRel::setStartState(){
    setStatus(SetStart);
}
