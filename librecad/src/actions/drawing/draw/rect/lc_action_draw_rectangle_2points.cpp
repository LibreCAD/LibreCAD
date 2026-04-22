/****************************************************************************
**
* Action that creates a rectangle defined by 2 points
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

#include "lc_action_draw_rectangle_2points.h"

#include "lc_linemath.h"
#include "lc_rectangle_2points_options_filler.h"
#include "lc_rectangle_2points_options_widget.h"
#include "rs_polyline.h"

LC_ActionDrawRectangle2Points::LC_ActionDrawRectangle2Points(LC_ActionContext *actionContext)
    :LC_ActionDrawRectangleAbstract("ActionDrawRectangle2Points",actionContext, RS2::ActionDrawRectangle2Points){
    m_ucsBasisBaseAngleRad = 0;
    LC_ActionDrawRectangle2Points::init(SetPoint1);
    setMainStatus(SetPoint1);
}

LC_ActionDrawRectangle2Points::~LC_ActionDrawRectangle2Points() = default;

void LC_ActionDrawRectangle2Points::doSaveOptions() {
    LC_ActionDrawRectangleAbstract::doSaveOptions();
    save("SnapMode", m_insertionPointSnapMode);
    save("SecondPointSnapMode", m_secondPointSnapMode);
}

void LC_ActionDrawRectangle2Points::doLoadOptions() {
    LC_ActionDrawRectangleAbstract::doLoadOptions();
    m_insertionPointSnapMode = loadInt("SnapMode", SNAP_CORNER);
    m_secondPointSnapMode = loadInt("SecondPointSnapMode", SNAP_CORNER);
}

bool LC_ActionDrawRectangle2Points::isInVisualSnapStatus(int status) {
    return (status == SetPoint1) || (status == SetPoint2);
}

void LC_ActionDrawRectangle2Points::init(const int status){
    LC_ActionDrawRectangleAbstract::init(status);
    m_corner1Set = false;
}

int LC_ActionDrawRectangle2Points::doGetStatusForInitialSnapToRelativeZero(){
    return SetPoint1;
}

void LC_ActionDrawRectangle2Points::doInitialSnapToRelativeZero(const RS_Vector& zero){
    m_corner1 = zero;
    m_corner1Set = true;
    setMainStatus(SetPoint2);
}

LC_ActionDrawRectangleAbstract::ShapeData LC_ActionDrawRectangle2Points::createPolyline(const RS_Vector &snapPoint) {
    ShapeData result;

    result.snapPoint = snapPoint;

    RS_Vector c2 = snapPoint;
    RS_Vector c1 = m_corner1;

    double angleRad = getActualBaseAngle();
    // check whether we should do rotation for calculations
    bool rotate = LC_LineMath::isMeaningfulAngle(angleRad);

    if (rotate) {
        // rotate c2 around c1, as first we'll build rectangle parallel to axises
        c2 = c2.rotate(c1, -angleRad);
    }

    RS_Vector size = c2 - c1;

    // do adjustments of first corner according to snap mode of insertion point
    switch (m_insertionPointSnapMode) {
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
        default:
            break;
    }

    // do adjustments for second corner based on snap mode of second point
    switch (m_secondPointSnapMode) {
        case SNAP_CORNER: {
            break;
        }
        case SNAP_EDGE_VERT: {
            c2.y = c2.y + size.y;
            break;
        }
        case SNAP_MIDDLE: {
            c2.y = c2.y + size.y;
            c2.x = c2.x + size.x;
            break;
        }
        case SNAP_EDGE_HOR: {
            c2.x = c2.x + size.x;
            break;
        }
        default:
            break;
    }

    bool drawBulge = false;
    double radiusX;
    double radiusY;

    // is it just rectangle or more complex shape
    bool drawComplex = true;

    // should we draw rounded corner or just lines
    prepareCornersDrawMode(radiusX, radiusY, drawComplex, drawBulge);

    // square - adjust coordinate to draw square
    //
    // Draw of square is needed (SHIFT is pressed on second point selection)
    if (m_alternativeActionMode) {
        double w = c2.x - c1.x;
        double h = c2.y - c1.y;
        double s = std::max(std::abs(w), std::abs(h));

        if (w < 0) {
            c2.x = c1.x - s;
        }
        else {
            c2.x = c1.x + s;
        }

        if (h < 0) {
            c2.y = c1.y - s;
        }
        else {
            c2.y = c1.y + s;
        }
    }

    // define coordinates of corners
    auto bottomLeftCorner = RS_Vector(c1.x, c1.y);
    auto bottomRightCorner = RS_Vector(c2.x, c1.y);
    auto topRightCorner = RS_Vector(c2.x, c2.y);
    auto topLeftCorner = RS_Vector(c1.x, c2.y);

    // ensure proper order of corners
    normalizeCorners(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner);

    if (drawBulge && m_snapToCornerArcCenter) {
        // adjust corners coordinates, so we'll snap to arc centers

        auto radiusShiftX = RS_Vector(radiusX, 0);
        auto radiusShiftY = RS_Vector(0, radiusY);

        bottomLeftCorner = bottomLeftCorner - radiusShiftX - radiusShiftY;
        bottomRightCorner = bottomRightCorner + radiusShiftX - radiusShiftY;

        topLeftCorner = topLeftCorner - radiusShiftX + radiusShiftY;
        topRightCorner = topRightCorner + radiusShiftX + radiusShiftY;
    }

    result.height = bottomLeftCorner.distanceTo(topLeftCorner);
    result.width = bottomLeftCorner.distanceTo(bottomRightCorner);
    result.centerPoint = (bottomLeftCorner + topRightCorner) / 2;

    RS_Polyline* polyline = createPolylineByVertexes(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner, drawBulge,
                                                     drawComplex, radiusX, radiusY);

    if (rotate) {
        // rotate corners:
        // now we'll rotate shape on specific angle
        polyline->rotate(m_corner1, angleRad);
        result.centerPoint = result.centerPoint.rotate(m_corner1, angleRad);
    }

    result.resultingPolyline = polyline;
    return result;
}

void LC_ActionDrawRectangle2Points::doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, const int status){
    LC_ActionDrawRectangleAbstract::doPreparePreviewEntities(e, snap, list, status);
    if (m_showRefEntitiesOnPreview) {
        if (m_corner1Set) {
            createRefPoint(m_corner1, list);
        }
        createRefSelectablePoint(snap, list);
    }
}

RS_Vector LC_ActionDrawRectangle2Points::createSecondCornerSnapForGivenRectSize(const RS_Vector& size) const {
    RS_Vector result;

    // take care of adjustment for second point snap according to current snap mode of insertion point
    // this is necessary if size of rect is specified via command

    switch (m_insertionPointSnapMode){
        case SNAP_CORNER:
            result = m_corner1 + size;
            break;
        case SNAP_EDGE_VERT:
            result = m_corner1;
            result.x = result.x + size.x;
            result.y = result.y + size.y/2;
        break;
        case SNAP_EDGE_HOR:
            result = m_corner1;
            result.x = result.x + size.x / 2;
            result.y = result.y + size.y;
        break;
        case SNAP_MIDDLE:
            result = m_corner1;
            result.x = result.x + size.x / 2;
            result.y = result.y + size.y / 2;
        break;
        default:
            break;
    }

    // here we ignore snap mode for second point in order to satisfy given size
    const double angleRad = getActualBaseAngle();
    if (LC_LineMath::isMeaningfulAngle(angleRad)){
        // rotate resulting point to given angle
        result = result.rotate(m_corner1, angleRad);
    }
    return result;
}

void LC_ActionDrawRectangle2Points::doAfterTrigger(){
    LC_ActionDrawRectangleAbstract::doAfterTrigger();
    setMainStatus(SetPoint1);
    m_corner1Set = false;
}

void LC_ActionDrawRectangle2Points::doOnLeftMouseButtonRelease([[maybe_unused]] const LC_MouseEvent* e, const int status, const RS_Vector &snapPoint){
    switch (status){
        case SetPoint1: {
            moveRelativeZero(snapPoint);
            m_corner1 = snapPoint;
            addSnappedPointToVisualSnap(snapPoint);
            m_corner1Set = true;
            setMainStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            createShapeData(snapPoint);
            addSnappedPointToVisualSnap(snapPoint);
            trigger();
            break;
        }
        default:
          break;
    }
}

void LC_ActionDrawRectangle2Points::doProcessCoordinateEvent(const RS_Vector &coord, [[maybe_unused]]bool zero, const int status){
    switch (status){
        case SetPoint1:
            m_corner1Set = true;
            addSnappedPointToVisualSnap(coord);
            m_corner1 = coord;
            setMainStatus(SetPoint2);
            stateUpdated(false);
            break;
        case SetPoint2:
            createShapeData(coord);
            addSnappedPointToVisualSnap(coord);
            trigger();
            break;
        case SetSize: {
            const RS_Vector newSnap = createSecondCornerSnapForGivenRectSize(coord);
            createShapeData(newSnap);
            moveRelativeZero(coord);
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawRectangle2Points::doUpdateMouseButtonHints(const int status){
    switch (status) {
        case SetPoint2:
            updatePromptTRBack(tr("Specify second point"), MOD_SHIFT_LC(tr("Draw Square")));
            break;
        case SetPoint1Snap:
            updatePromptTRBack(tr("Specify point 1 snap [corner|mid-vert|mid-hor|middle]"));
            break;
        case SetPoint2Snap:
            updatePromptTRBack(tr("Specify point 2 snap [corner|mid-vert|mid-hor|middle]"));
            break;
        case SetSize:
            updatePromptTRBack(tr("Specify size (width, height)"));
            break;
        default:
            LC_ActionDrawRectangleAbstract::doUpdateMouseButtonHints(status);
            break;
    }
}

void LC_ActionDrawRectangle2Points::processCommandValue([[maybe_unused]]double value, [[maybe_unused]]bool &toMainStatus){
    // no additional processing there
}

bool LC_ActionDrawRectangle2Points::processCustomCommand([[maybe_unused]] const int status, const QString &command, bool &toMainStatus){
    bool result = true;
    if (checkCommand("snap1",command)){ // starts entering of snap mode for point 1
        setStatus(SetPoint1Snap);
        toMainStatus = false;
    }
    else if (checkCommand("snap2",command)){ // starts entering of snap mode for corner 2
        setStatus(SetPoint2Snap);
        toMainStatus = false;
    }
    else if (checkCommand("corner",command)){ // value for corner mode
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_CORNER;
        }else if (getStatus() == SetPoint2Snap){
            m_secondPointSnapMode = SNAP_CORNER;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("mid-vert",command)){ // value for corner mode
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_EDGE_VERT;
        }else if (getStatus() == SetPoint2Snap){
            m_secondPointSnapMode = SNAP_EDGE_VERT;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("mid-hor",command)){ // value for corner mode
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_EDGE_HOR;
        }else if (getStatus() == SetPoint2Snap){
            m_secondPointSnapMode = SNAP_EDGE_HOR;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("middle",command)){ // value for corner mode
        if (status == SetPoint1Snap){
            m_insertionPointSnapMode = SNAP_MIDDLE;
        }else if (getStatus() == SetPoint2Snap){
            m_secondPointSnapMode = SNAP_MIDDLE;
        }
        else {
            result = false;
        }
    }
    else if (checkCommand("size",command)){ // starts entering size
        if (m_corner1Set){
            toMainStatus = false;
            setStatus(SetSize);
        }
    }
    else if (checkCommand("pos",command)){  // switches to insertion point state
        toMainStatus = false;
        setMainStatus(SetPoint1);
    }
    else if (checkCommand("snapcorner",command)){ // enables snapping to corner
        m_snapToCornerArcCenter = false;
    }
    else if (checkCommand("snapshift",command)){ // enables snapping to center of rounding arc
        m_snapToCornerArcCenter = true;
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

LC_ActionOptionsWidget* LC_ActionDrawRectangle2Points::createOptionsWidget(){
    return new LC_Rectangle2PointsOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawRectangle2Points::createOptionsFiller() {
    return new LC_Rectangle2PointsOptionsFiller();
}

bool LC_ActionDrawRectangle2Points::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* e, [[maybe_unused]]int status){
    return m_corner1Set;
}

void LC_ActionDrawRectangle2Points::setSecondPointSnapMode(const int value){
    m_secondPointSnapMode = value;
    drawPreviewForLastPoint();
}

bool LC_ActionDrawRectangle2Points::doUpdateAngleByInteractiveInput(const QString& tag, const double angleRad) {
    if (tag == "angle") {
        setAngleRadians(angleRad);
        return true;
    }
    return false;
}

bool LC_ActionDrawRectangle2Points::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
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
