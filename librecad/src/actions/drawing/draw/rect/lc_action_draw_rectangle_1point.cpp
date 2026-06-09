/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#include "lc_action_draw_rectangle_1point.h"

#include "lc_linemath.h"
#include "lc_rectangle_1_point_options_filler.h"
#include "lc_rectangle_1point_options_widget.h"
#include "rs_polyline.h"

LC_ActionDrawRectangle1Point::LC_ActionDrawRectangle1Point(LC_ActionContext *actionContext)
    :LC_ActionDrawRectangleAbstract("ActionDrawRectangle1Point",actionContext, RS2::ActionDrawRectangle1Point){
}

LC_ActionDrawRectangle1Point::~LC_ActionDrawRectangle1Point() = default;

// positions of snap points on the rectangle. Actually, this vector is used for
// coordinates transformation in createPolyline function
const std::vector<RS_Vector> LC_ActionDrawRectangle1Point::SNAP_POINTS {
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

void LC_ActionDrawRectangle1Point::doSaveOptions() {
    LC_ActionDrawRectangleAbstract::doSaveOptions();
    save("SnapMode", m_insertionPointSnapMode);
    save("Width", m_width);
    save("Height", m_height);
    save("BaseAngleIsFree", m_angleIsFree);
    save("SizeInner", m_sizeIsInner);
}

void LC_ActionDrawRectangle1Point::doLoadOptions() {
    LC_ActionDrawRectangleAbstract::doLoadOptions();

    m_insertionPointSnapMode = loadInt("SnapMode", SnapMode::SNAP_BOTTOM_LEFT);
    m_width = loadDouble("Width", 10.0);
    m_height = loadDouble("Height", 10.0);
    m_angleIsFree= loadBool("BaseAngleIsFree", false);
    m_sizeIsInner = loadBool("SizeInner", false);
}

/**
 * Central function used to draw a rectangle, that is positioned by one of its point (defined by snapMode) in provided snap point.
 * Rectangle may be rotated on specified angle, and corners of rectangle are drawn according to specified mode (straight, rounded corners or bevels
 * @param snapPoint primary point used for positioning of shape
 * @return positioned polyline
 */
LC_ActionDrawRectangleAbstract::ShapeData LC_ActionDrawRectangle1Point::createPolyline(const RS_Vector &snapPoint){

    ShapeData result;
    result.snapPoint = snapPoint;

    const bool inFreeAngleMode = getStatus() == SetAngleFree;

    const double x = inFreeAngleMode ? m_insertionPoint.x : snapPoint.x;
    const double y = inFreeAngleMode? m_insertionPoint.y : snapPoint.y;

    // calculate half size of required size
    double halfWidth  = m_width/2;
    double halfHeight = m_height/2;

    double radiusX;
    double radiusY;

    // is it just rectangle or more complex shape
    bool drawComplex = true;

    // should we draw rounded corner or just lines
    bool drawBulge = false;

    prepareCornersDrawMode(radiusX, radiusY, drawComplex, drawBulge);

    if (drawBulge){
        if (m_sizeIsInner){
            // increase the overall height and width to value of diameter (radius*2)
            halfWidth = halfWidth + m_radius;
            halfHeight = halfHeight + m_radius;
        }
    }

    // create centered rect first - it center is the same as provided snap point

    // here we calculate coordinates of corners
    const auto bottomLeftCorner = RS_Vector(x - halfWidth , y - halfHeight);
    const auto topRightCorner = RS_Vector(x + halfWidth, y + halfHeight);
    const auto bottomRightCorner = RS_Vector(x + halfWidth, y - halfHeight);
    const auto topLeftCorner = RS_Vector(x - halfWidth, y + halfHeight);

    result.height = bottomLeftCorner.distanceTo(topLeftCorner);
    result.width = bottomLeftCorner.distanceTo(bottomRightCorner);

    auto center = RS_Vector((bottomLeftCorner + topRightCorner)*0.5);

    RS_Polyline *polyline = createPolylineByVertexes(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner, drawBulge, drawComplex, radiusX, radiusY);

    // shape is built, so now we'll position it

    // vector to reference point that is used as snap
    const RS_Vector reference = SNAP_POINTS.at(static_cast<size_t>(m_insertionPointSnapMode));

    // utility vector (half size)
    const auto halfRect = RS_Vector(-halfWidth, -halfHeight);

    // prepare vector we'll use for moving shape
    RS_Vector moveVector = reference * halfRect;

    // additional move if corners are round and snap to center is needed
    if (drawBulge){
        if (m_snapToCornerArcCenter){
            moveVector = moveVector + reference*m_radius;
        }
    }
    // move shape so it's reference point will correspond to provided snap point
    polyline->move(moveVector);

    center.move(moveVector);

    double wcsActualBaseAngle = 0.0;
    if (m_baseAngleIsFixed){
        wcsActualBaseAngle = toWorldAngleFromUCSBasis(m_ucsBasisBaseAngleRad);
        if (m_angleIsFree){
            if (inFreeAngleMode){
                wcsActualBaseAngle = m_insertionPoint.angleTo(snapPoint);
                const double ucsBasisAngle = toUCSBasisAngle(wcsActualBaseAngle);
                doSetAngle(ucsBasisAngle);
                updateOptionsUI(LC_Rectangle1PointOptionsWidget::UPDATE_ANGLE);
            }
        }
    }
    else{
        wcsActualBaseAngle = toWorldAngleFromUCSBasis(0.0);
    }

    const double wcsBaseAngle = wcsActualBaseAngle;

    if (LC_LineMath::isMeaningfulAngle(wcsBaseAngle)){
        // now we'll rotate shape on specific angle
        if (inFreeAngleMode){
            polyline->rotate(m_insertionPoint, wcsBaseAngle);
            center.rotate(m_insertionPoint, wcsBaseAngle);
        }
        else {
            polyline->rotate(snapPoint, wcsBaseAngle);
            center.rotate(snapPoint, wcsBaseAngle);
        }
    }
    result.centerPoint = center;
    result.resultingPolyline = polyline;
    return result;
}

int LC_ActionDrawRectangle1Point::doGetStatusForInitialSnapToRelativeZero(){
    return SetPoint1;
}

// fixme - invalid code!!!! refactor to initial presnap trigger logic
void LC_ActionDrawRectangle1Point::doInitialSnapToRelativeZero(const RS_Vector& relZero){
    createShapeData(relZero);
    trigger();
    finishAction();
}

RS_Vector LC_ActionDrawRectangle1Point::doGetRelativeZeroAfterTrigger() {
    if (getStatus() == SetAngleFree){
        return m_insertionPoint;
    }
    return LC_ActionDrawRectangleAbstract::doGetRelativeZeroAfterTrigger();
}

void LC_ActionDrawRectangle1Point::doAfterTrigger() {
    LC_ActionDrawRectangleAbstract::doAfterTrigger();
    int newStatus = -1;
    switch (getStatus()){
        case SetPoint1:
            newStatus = m_controlPressedOnMouseRelease ? -1 : SetPoint1;
            break;
        case SetAngleFree:
            newStatus = m_controlPressedOnMouseRelease ? SetPoint1 : SetAngleFree;
            break;
        default:
            break;
    }
    m_controlPressedOnMouseRelease = false;
    setStatus(newStatus);
}

void LC_ActionDrawRectangle1Point::doOnLeftMouseButtonRelease([[maybe_unused]] const LC_MouseEvent* e, const int status, const RS_Vector &snap){
    switch (status) {
        case SetPoint1: {
            if (m_angleIsFree){
                m_insertionPoint = snap;
                setStatus(SetAngleFree);
            }
            else{
                m_controlPressedOnMouseRelease = e->isControl;
                createShapeData(snap);
                trigger();
            }
            break;
        }
        case SetAngleFree:{
            m_controlPressedOnMouseRelease = e->isControl;
            createShapeData(snap);
            trigger();
            break;
        }
        default:
            break;
    }
}

RS_Vector LC_ActionDrawRectangle1Point::doGetMouseSnapPoint(const LC_MouseEvent* e) {
   RS_Vector result = e->snapPoint;
   if (getStatus() == SetAngleFree){
       result = getSnapAngleAwarePoint(e, m_insertionPoint, result, isMouseMove(e));
   }
   return result;
}

void LC_ActionDrawRectangle1Point::setBaseAngleFree(const bool val) {
    m_angleIsFree = val;
    if (getStatus() == SetAngleFree){
        setStatus(SetPoint1);
    }
}

void LC_ActionDrawRectangle1Point::doBack([[maybe_unused]] const LC_MouseEvent* e, const int status) {
    switch (status){
        case SetAngleFree: {
            setStatus(SetPoint1);
            break;
        }
        case SetPoint1:{
            setStatus(-1);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawRectangle1Point::doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, const int status){
    LC_ActionDrawRectangleAbstract::doPreparePreviewEntities(e, snap, list, status);
    createRefSelectablePoint(snap, list);
    if (m_showRefEntitiesOnPreview) {
        if (status == SetAngleFree) {
            createRefPoint(m_insertionPoint, list);
            createRefLine(m_insertionPoint, snap, list);
        }
    }
}

void LC_ActionDrawRectangle1Point::doProcessCoordinateEvent(const RS_Vector &coord,[[maybe_unused]] bool isRelativeZero, const int status){
    switch (status) {
        case SetPoint1:
            if (m_angleIsFree){
                m_insertionPoint = coord;
                setStatus(SetAngleFree);
            }
            else {
                createShapeData(coord);
                trigger();
            }
            break;
        case SetSize: { // size is set from command widget
            const double w = LC_LineMath::getMeaningful(coord.x);
            const double h = LC_LineMath::getMeaningful(coord.y);
            m_width = w;
            m_height = h;
            updateOptions();
            setMainStatus(SetPoint1);
            break;
        }
        case SetWidth:
            m_width = 0.0;
            updateOptions();
            setMainStatus(SetPoint1);
            break;
        case SetHeight:
            m_height = 0.0;
            updateOptions();
            setMainStatus(SetPoint1);
            break;
        case SetAngleFree:
            createShapeData(coord);
            trigger();
            break;
        default:
            break;
    }
}

void LC_ActionDrawRectangle1Point::processCommandValue(const double value, [[maybe_unused]]bool &toMainStatus){
    switch (getStatus()) {
        case SetWidth: {
            m_width = LC_LineMath::getMeaningful(value);
            break;
        }
        case SetHeight: {
            m_height = LC_LineMath::getMeaningful(value);
            break;
        }
        default:
            break;
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

void LC_ActionDrawRectangle1Point::doUpdateMouseButtonHints(const int status){
    switch (status) {
        case SetHeight:
            updatePromptTRBack(tr("Specify height"));
            break;
        case SetWidth:
            updatePromptTRBack(tr("Specify width"));
            break;
        case SetSize:
            updatePromptTRBack(tr("Specify size (width, height))"));
            break;
        case SetPoint1Snap:
            updatePromptTRBack(tr("Specify reference point [topl|top|topr|left|middle|right|bottoml|bottom|bottomr]"));
            break;
        case SetAngleFree:
            updatePromptTRBack(tr("Specify point that defines base angle for rectangle"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Don't keep insertion point")));
            break;
        default:
            updatePrompt();
            break;
    }
}

bool LC_ActionDrawRectangle1Point::processCustomCommand([[maybe_unused]] const int status, const QString &command, bool &toMainStatus){
    bool result = true;
    if (checkCommand("width",command)){ //initiates entering width
        setStatus(SetWidth);
        toMainStatus = false;
    }
    else if (checkCommand("height",command)){ // initiates entering height
        setStatus(SetHeight);
        toMainStatus = false;
    }
    else if (checkCommand("size",command)){ // initiates entering size of rect
        setStatus(SetSize);
        toMainStatus = false;
    }
    else if (checkCommand("pos",command)){ // switches to entering insertion point state
        setStatus(SetPoint1);
        toMainStatus = false;
    }
    else if (checkCommand("snap1",command)){  // initiates entering snap mode for insertion point
        setStatus(SetPoint1Snap);
        toMainStatus = false;
    }
    else if (checkCommand("topl",command)){  // top-left position of snap
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_TOP_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("top",command)){ // middle of top edge position of snap
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_TOP;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("topr",command)){ // top-right corner snap
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_TOP_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("left",command)){ // middle of left edge snap position
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("middle",command)){ // center of rect snap position
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_MIDDLE;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("right",command)){ // middle of right edge snap position
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottoml",command)){ // bottom-left corner snap position
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_BOTTOM_LEFT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottom",command)){ // middle of bottom edge snap position
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_BOTTOM;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("bottomr",command)){ // bottom-right corner snap position
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_BOTTOM_RIGHT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("snapcorner",command)){ // switches to snap to corner mode
        m_snapToCornerArcCenter = false;
    }
    else if (checkCommand("snapshift",command)){ // switches to snap to rounding arc center mode
        m_snapToCornerArcCenter = true;
    }
    else if (checkCommand("sizeout",command)){ // switches sizes calculation relating to corners
        m_sizeIsInner = false;
    }
    else if (checkCommand("sizein",command)){ // switches sizes calculation relating to centers of rounding arcs
        m_sizeIsInner = true;
    }
    else{
        result = false;
    }
    return result;
}

bool LC_ActionDrawRectangle1Point::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* event, [[maybe_unused]]int status){
    return true;
}

void LC_ActionDrawRectangle1Point::setWidth(const double value){
    m_width = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangle1Point::setHeight(const double value){
    m_height = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangle1Point::setSizeInner(const bool value){
    m_sizeIsInner = value;
    drawPreviewForLastPoint();
}

bool LC_ActionDrawRectangle1Point::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setAngleRadians(angleRad);
        return true;
    }
    return false;
}

bool LC_ActionDrawRectangle1Point::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "width") {
        setWidth(distance);
        return true;
    }
    if (tag == "height") {
        setHeight(distance);
        return true;
    }
    if (tag == "radius") {
        setCornerRadius(distance);
        return true;
    }
    if (tag == "lengthX") {
        setCornerBevelLengthX(distance);
        return true;
    }
    if (tag == "lengthY") {
        setCornerBevelLengthY(distance);
        return true;
    }
    return false;
}

bool LC_ActionDrawRectangle1Point::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetAngleFree);
}

LC_ActionOptionsWidget* LC_ActionDrawRectangle1Point::createOptionsWidget(){
    return new LC_Rectangle1PointOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawRectangle1Point::createOptionsFiller() {
    return new LC_Rectangle1PointOptionsFiller();
}
