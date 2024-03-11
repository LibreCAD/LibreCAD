#include <cmath>
#include "lc_actiondrawrectangle2points.h"
#include "rs_math.h"
#include "rs_graphicview.h"
#include "rs_coordinateevent.h"
#include "lc_rectangle2pointsoptions.h"
#include "rs_commandevent.h"
#include "rs_dialogfactory.h"
#include "lc_abstractactiondrawrectangle.h"
#include <QMouseEvent>

LC_ActionDrawRectangle2Points::LC_ActionDrawRectangle2Points(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionDrawRectangle("Draw rectangle 2 points",
                                    container, graphicView){
    actionType = RS2::ActionDrawRectangle2Points;
    angle = 0;
    init(SetPoint1);
}

LC_ActionDrawRectangle2Points::~LC_ActionDrawRectangle2Points() = default;

void LC_ActionDrawRectangle2Points::init(int status){
    RS_PreviewActionInterface::init(status);
    corner1Set = false;
}

RS_Polyline *LC_ActionDrawRectangle2Points::createPolyline(RS_Vector &snapPoint) const{

    RS_Vector c2 = snapPoint;
    RS_Vector c1 = corner1;


    double angleRad = RS_Math::deg2rad(angle);

    bool rotate = false;

    if (std::abs(angleRad) > RS_TOLERANCE_ANGLE){
        rotate = true;
    }

    if (rotate) {
        // rotate c2 around c1, as first we'll build rectangle parallel to axises
        c2 = c2.rotate(c1,-angleRad);
    }

    RS_Vector size = c2-c1;

    // do adjustments of first corner according to snap mode of insertion point
    switch (insertionPointSnapMode){
        case SNAP_CORNER:
            break;
        case SNAP_EDGE_VERT:
            c1.y = c1.y - size.y;
            break;
        case SNAP_EDGE_HOR:
            c1.x = c1.x - size.x;
            break;
        case SNAP_MIDDLE:
            c1.x = c1.x - size.x;
            c1.y = c1.y - size.y;
            break;
    }

//    size = c2-c1;

    // do adjustments for second corner based on snap mode of second point
    switch (secondPointSnapMode){
        case SNAP_CORNER:
            break;
        case SNAP_EDGE_VERT:
            c2.y = c2.y + size.y;
            break;
        case SNAP_MIDDLE:{
            c2.y = c2.y + size.y;
            c2.x = c2.x + size.x;
            break;
        case SNAP_EDGE_HOR:
            c2.x = c2.x + size.x;
            break;
        }
    }

    bool drawBulge = false;
    double radiusX;
    double radiusY;

    // is it just rectangle or more complex shape
    bool drawComplex = true;

    // should we draw rounded corner or just lines
    prepareCornersDrawMode(radiusX, radiusY, drawComplex, drawBulge);

    // square - adjust coordinate to draw square
    if (squareDrawRequested) {
        double w = c2.x - c1.x;
        double h = c2.y - c1.y;
        double s = std::max(std::abs(w), std::abs(h));

        if (w<0) {
            c2.x = c1.x - s;
        }
        else {
            c2.x = c1.x + s;
        }

        if (h<0) {
            c2.y = c1.y - s;
        }
        else {
            c2.y = c1.y + s;
        }
    }

    RS_Vector bottomLeftCorner = RS_Vector(c1.x , c1.y);
    RS_Vector bottomRightCorner = RS_Vector(c2.x, c1.y);
    RS_Vector topRightCorner = RS_Vector(c2.x, c2.y);
    RS_Vector topLeftCorner = RS_Vector(c1.x, c2.y);

    normalizeCorners(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner);

    if (drawBulge && snapToCornerArcCenter){

        // adjust corners coordinates, so we'll snap to arc centers

        RS_Vector radiusShiftX = RS_Vector(radiusX, 0);
        RS_Vector radiusShiftY = RS_Vector(0, radiusY);

        bottomLeftCorner = bottomLeftCorner - radiusShiftX - radiusShiftY;
        bottomRightCorner = bottomRightCorner + radiusShiftX - radiusShiftY;

        topLeftCorner = topLeftCorner - radiusShiftX + radiusShiftY;
        topRightCorner = topRightCorner + radiusShiftX + radiusShiftY;
    }

    RS_Polyline *polyline = createPolylineByVertexes(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner, drawBulge, drawComplex, radiusX, radiusY);

    if (rotate) {
        // rotate corners:
        // now we'll rotate shape on specific angle
        polyline->rotate(corner1, angleRad);
    }

    return polyline;
}

RS_Vector LC_ActionDrawRectangle2Points::createSecondCornerSnapForGivenRectSize(RS_Vector size){
    RS_Vector result;

    // take care of adjustment for second point snap accroding to current snap mode of insertion point

    switch (insertionPointSnapMode){
        case SNAP_CORNER:
            result = corner1 + size;
            break;
        case SNAP_EDGE_VERT:
            result = corner1;
            result.x = result.x + size.x;
            result.y = result.y + size.y/2;
        break;
        case SNAP_EDGE_HOR:
            result = corner1;
            result.x = result.x + size.x / 2;
            result.y = result.y + size.y;
        break;
        case SNAP_MIDDLE:
            result = corner1;
            result.x = result.x + size.x / 2;
            result.y = result.y + size.y / 2;
        break;
    }

    // here we ignore snap mode for second point in order to satisfy given size

    if (std::abs(angle) > RS_TOLERANCE_ANGLE){
        // rotate resulting point to given angle
        double angleRad = RS_Math::deg2rad(angle);
        result = result.rotate(corner1, angleRad);
    }
    return result;
}

void LC_ActionDrawRectangle2Points::doAfterTrigger(){
    setStatus(SetPoint1);
    corner1Set = false;
}

void LC_ActionDrawRectangle2Points::proceedMouseLeftButtonReleasedEvent(QMouseEvent *e){
    switch (getStatus()){
        case SetPoint1: {
            RS_Vector snap = snapPoint(e);
            graphicView->moveRelativeZero(snap);
            corner1 = snap;
            corner1Set = true;
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            RS_Vector snap = snapPoint(e);
            squareDrawRequested = e->modifiers() & Qt::ShiftModifier;
            resultingPolyline = createPolyline(snap);
            graphicView->moveRelativeZero(snap);
            trigger();
            break;
        }
    }
}

void LC_ActionDrawRectangle2Points::processCoordinateEvent(RS_CoordinateEvent *pEvent, RS_Vector coord, bool zero){
    switch (getStatus()){
        case SetPoint1:
            corner1Set = true;
            corner1 = coord;
            setStatus(SetPoint2);
            stateUpdated(false);
            break;
        case SetPoint2:
            resultingPolyline = createPolyline(coord);
            graphicView->moveRelativeZero(coord);
            trigger();
            break;
        case SetSize:
            RS_Vector newSnap = createSecondCornerSnapForGivenRectSize(coord);
            resultingPolyline = createPolyline(newSnap);
            graphicView->moveRelativeZero(coord);
            trigger();
            break;
    }
}

void LC_ActionDrawRectangle2Points::doUpdateMouseButtonHints(){

    switch (getStatus()) {
        case SetPoint2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second point"),
                                                tr("Back"));
            break;
        case SetPoint1Snap:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify corner one snap [corner|mid-vert|mid-hor|middle]"),
                                                tr("Back"));
            break;
        case SetPoint2Snap:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify corner one snap [corner|mid-vert|mid-hor|middle]"),
                                                tr("Back"));
            break;
        case SetSize:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify size (width, height)"),
                                                tr("Back"));
            break;

        default:
            LC_AbstractActionDrawRectangle::doUpdateMouseButtonHints();
            break;
    }
}

void LC_ActionDrawRectangle2Points::processCommandValue(double value){
    // no additional processing there
}

bool LC_ActionDrawRectangle2Points::processCustomCommand(RS_CommandEvent *e, const QString &c, bool &toMainStatus){
    bool result = true;
    if (checkCommand("snap1",c)){
        setStatus(SetPoint1Snap);
        toMainStatus = false;
    }
    else if (checkCommand("snap2",c)){
        setStatus(SetPoint2Snap);
        toMainStatus = false;
    }
    else if (checkCommand("corner",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_CORNER;
        }else if (getStatus() == SetPoint2Snap){
            secondPointSnapMode = SNAP_CORNER;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("mid-vert",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_EDGE_VERT;
        }else if (getStatus() == SetPoint2Snap){
            secondPointSnapMode = SNAP_EDGE_VERT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("mid-hor",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_EDGE_HOR;
        }else if (getStatus() == SetPoint2Snap){
            secondPointSnapMode = SNAP_EDGE_HOR;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("middle",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_MIDDLE;
        }else if (getStatus() == SetPoint2Snap){
            secondPointSnapMode = SNAP_MIDDLE;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("size",c)){
        if (corner1Set){
            toMainStatus = false;
            setStatus(SetSize);
        }
    }
    else if (checkCommand("point",c)){
        toMainStatus = false;
        setStatus(SetPoint1);
    }
    else if (checkCommand("snapcorner",c)){
        snapToCornerArcCenter = false;
    }
    else if (checkCommand("snapshift",c)){
        snapToCornerArcCenter = true;
    }
    else{
        result = false;
    }
    return result;
}

QStringList LC_ActionDrawRectangle2Points::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetPoint1Snap:
        case SetPoint2Snap:{
            cmd += command("corner");
            cmd += command("mid-vert");
            cmd += command("mid-hor");
            cmd += command("middle");
            break;
        }
        case SetCorners:{
            cmd += command("str");
            cmd += command("round");
            cmd += command("bevels");
            break;
        }
        case SetPoint1:
        case SetPoint2:
        case SetAngle:
        case SetBevels:
        case SetRadius:
        case SetSize:
            cmd += command("point");
            cmd += command("size");
            cmd += command("angle");
            cmd += command("corners");
            cmd += command("bevels");
            cmd += command("snap1");
            cmd += command("snap2");
            cmd += command("size");
            cmd += command("radius");
            cmd += command("usepoly");
            cmd += command("nopoly");
            cmd += command("snapcorner");
            cmd += command("snapshift");
            break;
        default:
            break;
    }

    return cmd;
}

void LC_ActionDrawRectangle2Points::processMouseEvent(QMouseEvent *e){
    squareDrawRequested = e->modifiers() & Qt::ShiftModifier;
}

void LC_ActionDrawRectangle2Points::setMainStatus(){
    if (corner1Set){
        setStatus(SetPoint2);
    }
    else {
        setStatus(SetPoint1);
    }
}

void LC_ActionDrawRectangle2Points::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_Rectangle2PointsOptions>(nullptr);
}

bool LC_ActionDrawRectangle2Points::mayDrawPreview(QMouseEvent *pEvent){
    return corner1Set;
}

int LC_ActionDrawRectangle2Points::getSecondPointSnapMode(){
    return secondPointSnapMode;
}

void LC_ActionDrawRectangle2Points::setSecondPointSnapMode(int value){
    secondPointSnapMode = value;
    drawPreviewForLastPoint();
}







