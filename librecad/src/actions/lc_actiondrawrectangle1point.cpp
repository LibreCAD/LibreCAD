/****************************************************************************
**
* Action that creates a rectangle defined by fixed width and height and snapped
* in one point

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
#include <QMouseEvent>
#include "rs_arc.h"
#include "rs_math.h"
#include "lc_linemath.h"
#include "lc_actiondrawrectangle1point.h"
#include "lc_rectangle1pointoptions.h"

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

    double actualBaseAngle = getActualBaseAngle();
    if (LC_LineMath::isMeaningfulAngle(actualBaseAngle)){
        // now we'll rotate shape on specific angle
        polyline->rotate(snapPoint, actualBaseAngle);
    }
    return polyline;
}

int LC_ActionDrawRectangle1Point::doGetStatusForInitialSnapToRelativeZero(){
    return SetPoint1;
}

void LC_ActionDrawRectangle1Point::doInitialSnapToRelativeZero(RS_Vector relZero){
    createShapeData(relZero);
    trigger();
}

void LC_ActionDrawRectangle1Point::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status, const RS_Vector &snap){
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

void LC_ActionDrawRectangle1Point::doProcessCoordinateEvent(const RS_Vector &coord,[[maybe_unused]] bool isRelativeZero, int status){
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
            setMainStatus(SetPoint1);
            break;
        }
        case SetWidth:
            width = 0.0;
            updateOptions();
            setMainStatus(SetPoint1);
            break;
        case SetHeight:
            height = 0.0;
            updateOptions();
            setMainStatus(SetPoint1);
            break;
        default:
            break;
    }
}

void LC_ActionDrawRectangle1Point::processCommandValue(double value, [[maybe_unused]]bool &toMainStatus){
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

void LC_ActionDrawRectangle1Point::doUpdateMouseButtonHints(int status){
    switch (status) {
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

bool LC_ActionDrawRectangle1Point::processCustomCommand([[maybe_unused]]RS_CommandEvent *e, const QString &c, bool &toMainStatus){
    bool result = true;
    if (checkCommand("width",c)){ //initiates entering width
        setStatus(SetWidth);
        toMainStatus = false;
    }
    else if (checkCommand("height",c)){ // initiates entering height
        setStatus(SetHeight);
        toMainStatus = false;
    }
    else if (checkCommand("size",c)){ // initiates entering size of rect
        setStatus(SetSize);
        toMainStatus = false;
    }
    else if (checkCommand("pos",c)){ // switches to entering insertion point state
        setStatus(SetPoint1);
        toMainStatus = false;
    }
    else if (checkCommand("snap1",c)){  // initiates entering snap mode for insertion point
        setStatus(SetPoint1Snap);
        toMainStatus = false;
    }
    else if (checkCommand("topl",c)){  // top-left position of snap
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_TOP_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("top",c)){ // middle of top edge position of snap
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_TOP;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("topr",c)){ // top-right corner snap
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_TOP_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("left",c)){ // middle of left edge snap position
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("middle",c)){ // center of rect snap position
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_MIDDLE;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("right",c)){ // middle of right edge snap position
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottoml",c)){ // bottom-left corner snap position
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_BOTTOM_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottom",c)){ // middle of bottom edge snap position
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_BOTTOM;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottomr",c)){ // bottom-right corner snap position
        if (getStatus() == SetPoint1Snap){
            insertionPointSnapMode = SNAP_BOTTOM_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("snapcorner",c)){ // switches to snap to corner mode
        snapToCornerArcCenter = false;
    }
    else if (checkCommand("snapshift",c)){ // switches to snap to rounding arc center mode
        snapToCornerArcCenter = true;
    }
    else if (checkCommand("sizeout",c)){ // switches sizes calculation relating to corners
        sizeIsInner = false;
    }
    else if (checkCommand("sizein",c)){ // switches sizes calculation relating to centers of rounding arcs
        sizeIsInner = true;
    }
    else{
        result = false;
    }
    return result;
}

bool LC_ActionDrawRectangle1Point::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, [[maybe_unused]]int status){
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

void LC_ActionDrawRectangle1Point::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_Rectangle1PointOptions>(nullptr);
}
