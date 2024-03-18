#include <cmath>
#include "lc_actiondrawrectangle1point.h"
#include "rs_arc.h"
#include "rs_preview.h"
#include "QMouseEvent"
#include "rs_math.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "lc_rectangle1pointoptions.h"
#include "lc_abstractactiondrawrectangle.h"
#include "lc_linemath.h"

LC_ActionDrawRectangle1Point::LC_ActionDrawRectangle1Point(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionDrawRectangle("Draw rectangles rel",
                               container, graphicView){
    actionType = RS2::ActionDrawRectangle1Point;
}

LC_ActionDrawRectangle1Point::~LC_ActionDrawRectangle1Point() = default;

// positions of snap points on the rectangle. Actually, this vector is used for
// coordinates transformation in createPolyline function
const std::vector<RS_Vector> LC_ActionDrawRectangle1Point::snapPoints {
    RS_Vector(-1,  1) , // top left
    RS_Vector( 0,  1) , // top
    RS_Vector( 1,  1) , // top right
    RS_Vector(-1,  0) , // left
    RS_Vector( 0,  0) , // middle
    RS_Vector( 1,  0) , // right
    RS_Vector(-1, -1) , // bottom left
    RS_Vector( 0, -1) , // bottom
    RS_Vector( 1, -1) , // bottom right
};

/**
 * Central function used to draw a rectangle, that is positioned by one of its point (defined by snapMode) in provided snap point.
 * Rectangle may be rotated on specified angle, and corners of rectangle are drawn according to specified mode (straight, rounded corners or bevels
 * @param snapPoint primary point used for positioning of shape
 * @return positioned polyline
 */
RS_Polyline *LC_ActionDrawRectangle1Point::createPolyline(const RS_Vector &snapPoint) const{

    double x = snapPoint.x;
    double y = snapPoint.y;

    // calculate half size of required size
    double halfWidth  = width/2;
    double halfHeight = height/2;

    double radiusX;
    double radiusY;

    // is it just rectangle or more complex shape
    bool drawComplex = true;

    // should we draw rounded corner or just lines
    bool drawBulge = false;

    prepareCornersDrawMode(radiusX, radiusY, drawComplex, drawBulge);

    if (drawBulge){
        if (sizeIsInner){
            // increase the overall height and width to value of diameter (radius*2)
            halfWidth = halfWidth + radius;
            halfHeight = halfHeight + radius;
        }
    }

    // create centered rect first - it center is the same as provided snap point

    // here we calculate coordinates of corners
    RS_Vector bottomLeftCorner = RS_Vector(x - halfWidth , y - halfHeight);
    RS_Vector topRightCorner = RS_Vector(x + halfWidth, y + halfHeight);
    RS_Vector bottomRightCorner = RS_Vector(x + halfWidth, y - halfHeight);
    RS_Vector topLeftCorner = RS_Vector(x - halfWidth, y + halfHeight);

    RS_Polyline *polyline = createPolylineByVertexes(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner, drawBulge, drawComplex, radiusX, radiusY);

    // shape is built, so now we'll position it

    // vector to reference point that is used as snap
    RS_Vector reference = snapPoints.at(insertionPointSnapMode);

    // utility vector (half size)
    RS_Vector halfRect = RS_Vector(-halfWidth, -halfHeight);

    // prepare vector we'll use for moving shape
    RS_Vector moveVector = reference * halfRect;

    // additional move if corners are round and snap to center is needed
    if (drawBulge){
        if (snapToCornerArcCenter){
            moveVector = moveVector + reference*radius;
        }
    }

    // move shape so it's reference point will correspond to provided snap point
    polyline->move(moveVector);

    // now we'll rotate shape on specific angle
    double angleRad = RS_Math::deg2rad(angle);
    polyline->rotate(snapPoint, angleRad);

    return polyline;
}

void LC_ActionDrawRectangle1Point::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snap){
    switch (status) {
        case SetPoint1: {
            createShapeData(snap);
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawRectangle1Point::doProcessCoordinateEvent(const RS_Vector &coord, bool isRelativeZero, int status){
    switch (status) {
        case SetPoint1:
            createShapeData(coord);
            trigger();
            break;
        case SetSize: { // size is set from command widget
            double w = LC_LineMath::getMeaningful(coord.x);
            double h = LC_LineMath::getMeaningful(coord.y);
            width = w;
            height = h;
            updateOptions();
            setStatus(SetPoint1);
            break;
        }
        case SetWidth:
            width = 0.0;
            updateOptions();
            setStatus(SetPoint1);
            break;
        case SetHeight:
            height = 0.0;
            updateOptions();
            setStatus(SetPoint1);
            break;
    }
}


void LC_ActionDrawRectangle1Point::setMainStatus(){
    setStatus(SetPoint1);
}

void LC_ActionDrawRectangle1Point::processCommandValue(double value){
    switch (getStatus()) {
        case SetWidth: {
            width = LC_LineMath::getMeaningful(value);
            break;
        }
        case SetHeight: {
            height = LC_LineMath::getMeaningful(value);
            break;
        }
    }
}

QStringList LC_ActionDrawRectangle1Point::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetPoint1Snap:{
            cmd += command("topl");
            cmd += command("top");
            cmd += command("topr");
            cmd += command("left");
            cmd += command("middle");
            cmd += command("right");
            cmd += command("bottoml");
            cmd += command("bottom");
            cmd += command("bottomr");
            break;
        }
        case SetCorners:{
            cmd += command("str");
            cmd += command("round");
            cmd += command("bevels");
            break;
        }
        case SetPoint1:
        case SetAngle:
        case SetHeight:
        case SetWidth:
        case SetBevels:
        case SetRadius:
        case SetSize:
            cmd += command("point");
            cmd += command("width");
            cmd += command("height");
            cmd += command("size");
            cmd += command("angle");
            cmd += command("corners");
            cmd += command("bevels");
            cmd += command("snap1");
            cmd += command("radius");
            cmd += command("usepoly");
            cmd += command("nopoly");
            cmd += command("snapcorner");
            cmd += command("snapshift");
            cmd += command("sizein");
            cmd += command("sizeout");
            break;
        default:
            break;
    }

    return cmd;
}

void LC_ActionDrawRectangle1Point::doUpdateMouseButtonHints(){
    switch (getStatus()) {
        case SetHeight:
            updateMouseWidgetTR("Specify height", "Back");
            break;
        case SetWidth:
            updateMouseWidgetTR("Specify width", "Back");
            break;
        case SetSize:
            updateMouseWidgetTR("Specify size (width, height)", "Back");
            break;
        case SetPoint1Snap:
            updateMouseWidgetTR("Specify reference point [topl|top|topr|left|middle|right|bottoml|bottom|bottomr]", "Back");
            break;
        default:
            updateMouseWidget();
            break;
    }
}


void LC_ActionDrawRectangle1Point::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_Rectangle1PointOptions>(nullptr);
}

bool LC_ActionDrawRectangle1Point::processCustomCommand(RS_CommandEvent *e, const QString &c, bool &toMainStatus){
    bool result = true;
    if (checkCommand("width",c)){
        setStatus(SetWidth);
        toMainStatus = false;
    }
    else if (checkCommand("height",c)){
        setStatus(SetHeight);
        toMainStatus = false;
    }
    else if (checkCommand("size",c)){
        setStatus(SetSize);
        toMainStatus = false;
    }
    else if (checkCommand("point",c)){
        setStatus(SetPoint1);
        toMainStatus = false;
    }
    else if (checkCommand("snap1",c)){
        setStatus(SetPoint1Snap);
        toMainStatus = false;
    }
    else if (checkCommand("topl",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_TOP_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("top",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_TOP;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("topr",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_TOP_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("left",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("middle",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_MIDDLE;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("right",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottoml",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_BOTTOM_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottom",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_BOTTOM;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottomr",c)){
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_BOTTOM_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("snapcorner",c)){
        snapToCornerArcCenter = false;
    }
    else if (checkCommand("snapshift",c)){
        snapToCornerArcCenter = true;
    }
    else if (checkCommand("sizeout",c)){
        sizeIsInner = false;
    }
    else if (checkCommand("sizein",c)){
        sizeIsInner = true;
    }
    else{
        result = false;
    }
    return result;
}

bool LC_ActionDrawRectangle1Point::doCheckMayDrawPreview(QMouseEvent *event, int status){
    return true;
}

void LC_ActionDrawRectangle1Point::setWidth(double value){
    width = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangle1Point::setHeight(double value){
    height = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangle1Point::setSizeInner(bool value){
    sizeIsInner = value;
    drawPreviewForLastPoint();
}