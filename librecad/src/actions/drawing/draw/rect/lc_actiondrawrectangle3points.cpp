/****************************************************************************
**
* Abstract base class for actions that draws a rectangle (or quadrangle)
* based on 3 points

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

#include "lc_actiondrawrectangle3points.h"

#include "lc_linemath.h"
#include "lc_rectangle3pointsoptions.h"
#include "rs_polyline.h"

/**
 * data that holds corners of rectangle
 */
struct LC_ActionDrawRectangle3Points::ActionData {
    RS_Vector corner1;
    RS_Vector corner2;
    RS_Vector corner3;
    RS_Vector corner4;

    bool corner1Set = false;
    bool corner2Set = false;
};

// todo  - add support of UI options for width and height?
LC_ActionDrawRectangle3Points::LC_ActionDrawRectangle3Points(LC_ActionContext *actionContext)
    :LC_AbstractActionDrawRectangle("Draw rectangles rel",actionContext, RS2::ActionDrawRectangle3Points)
    , m_actionData(std::make_unique<ActionData>()){
}

LC_ActionDrawRectangle3Points::~LC_ActionDrawRectangle3Points() = default;

void LC_ActionDrawRectangle3Points::doAfterTrigger(){
    LC_AbstractActionDrawRectangle::doAfterTrigger();
    resetPoints();
    setMainStatus(SetPoint1);
}

// todo - potentially, we may move relative point to different corners...
// let it stays there for now, probably later it will be either moved
// to parent class or simplified
RS_Vector LC_ActionDrawRectangle3Points::doGetRelativeZeroAfterTrigger(){
    RS_Vector zeroCorner;
    switch (m_endRelativeZeroPointCorner){
        case (SNAP_CORNER1):
            zeroCorner = m_actionData->corner1;
            break;
        case (SNAP_CORNER2):
            zeroCorner = m_actionData->corner2;
            break;
        case (SNAP_CORNER3):
            zeroCorner = m_actionData->corner3;
            break;
        case (SNAP_CORNER4):
            zeroCorner = m_actionData->corner4;
            break;
        default:
            zeroCorner = m_actionData->corner3;
    }
    return zeroCorner;
}

void LC_ActionDrawRectangle3Points::doPreparePreviewEntities(LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    LC_AbstractActionDrawRectangle::doPreparePreviewEntities(e, snap, list, status);
    if (m_showRefEntitiesOnPreview) {
        if (m_actionData->corner1Set) {
            createRefPoint(m_actionData->corner1, list);

            if (m_actionData->corner2Set) {
                createRefPoint(m_actionData->corner2, list);
                createRefPoint(m_actionData->corner4, list);
                createRefSelectablePoint(snap, list);
            } else {
                createRefSelectablePoint(m_actionData->corner2, list);
            }
        }
    }
}

/***
 * Central function that calculates resulting shape based on the provided snap point,
 * settings and current state of the action.
 * @param snapPoint snap point
 * @return
 */
LC_AbstractActionDrawRectangle::ShapeData LC_ActionDrawRectangle3Points::createPolyline(const RS_Vector &snapPoint) {
    ShapeData result;
    result.snapPoint = snapPoint;
    RS_Polyline* polyline = nullptr;

    if (snapPoint.valid){
        int status = getStatus();

        // first, perform corners pre-calculation based on the current status
        switch (status) {
            case SetWidth:
            case SetPoint2: {
                double angleRad = getActualBaseAngle();
                calculateCorner2(snapPoint, angleRad, false);
                break;
            }
            case SetSize:
                // all corners should be calculated already
                break;
            case SetHeight:
                // point 3 is already calculated there
                calculateCorner4();
                break;
            case SetPoint3: {
                double baseAngle = m_actionData->corner1.angleTo(m_actionData->corner2);
                if (m_createQuadrangle){
                    if (m_innerAngleIsFixed){
                        double innerAngleRad = RS_Math::deg2rad(m_innerAngle);
                        double actualAngle = baseAngle + innerAngleRad;
                        m_actionData->corner3 = calculatePossibleEndpointForAngle(snapPoint, m_actionData->corner2, actualAngle);
                    } else {
                        m_actionData->corner3 = snapPoint;
                    }
                } else {
                    m_actionData->corner3 = calculatePossibleEndpointForAngle(snapPoint, m_actionData->corner2, baseAngle + M_PI / 2);
                }
                calculateCorner4();
                break;
            }
            default:
                break;
        }

        // should we draw just simple shape (by vertexes) or more complex (rounding, bevels)?
        bool drawPrimitiveShape = true;

        switch (status) {
            case SetPoint3:
            case SetHeight: {
                drawPrimitiveShape = m_createQuadrangle; // for quadrangle we just connect corners
                break;
            }
            default:
                break;
        }

        // check how we'll handle edges
        bool shouldInspectForNonCompleteShape = m_cornersDrawMode == CORNER_STRAIGHT && m_edgesDrawMode != EDGES_BOTH;

        if (!drawPrimitiveShape){
            drawPrimitiveShape = shouldInspectForNonCompleteShape;
        }

        if (drawPrimitiveShape){
            // simple mode - just create a polyline that connects calculated corner vertexes
            polyline = new RS_Polyline(m_container);
            polyline->addVertex(m_actionData->corner1);
            polyline->addVertex(m_actionData->corner2);
            polyline->addVertex(m_actionData->corner3);
            polyline->addVertex(m_actionData->corner4);
            polyline->setClosed(true);
            polyline->endPolyline();

            result.height = m_actionData->corner1.distanceTo(m_actionData->corner4);
            result.width = m_actionData->corner1.distanceTo(m_actionData->corner2);
            result.centerPoint = (m_actionData->corner1 + m_actionData->corner3) / 2;
        }
        else{
            // more complex case, draw corners or bevels
            double baseAngle = m_actionData->corner1.angleTo(m_actionData->corner2);

            bool rotate = false;

            if (LC_LineMath::isMeaningfulAngle(baseAngle)){
                rotate = true;
            }

            RS_Vector c1 = m_actionData->corner1;
            RS_Vector c2 = m_actionData->corner2;
            RS_Vector c3 = m_actionData->corner3;
            RS_Vector c4 = m_actionData->corner4;

            if (rotate) {
                // rotate c2 around c1, as first we'll build rectangle parallel to axises
                c2.rotate(c1,-baseAngle);
                c3.rotate(c1,-baseAngle);
                c4.rotate(c1,-baseAngle);
            }

            bool drawBulge = false;
            double radiusX;
            double radiusY;

            // is it just rectangle or more complex shape
            bool drawComplex = true;

            // should we draw rounded corner or just lines
            prepareCornersDrawMode(radiusX, radiusY, drawComplex, drawBulge);


            RS_Vector bottomLeftCorner = c1;
            RS_Vector bottomRightCorner = c2;
            RS_Vector topRightCorner = c3;
            RS_Vector topLeftCorner = c4;

            normalizeCorners(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner);

            if (drawBulge && m_snapToCornerArcCenter){

                // adjust corners coordinates, so we'll snap to arc centers

                RS_Vector radiusShiftX = RS_Vector(radiusX, 0);
                RS_Vector radiusShiftY = RS_Vector(0, radiusY);

                bottomLeftCorner = bottomLeftCorner - radiusShiftX - radiusShiftY;
                bottomRightCorner = bottomRightCorner + radiusShiftX - radiusShiftY;

                topLeftCorner = topLeftCorner - radiusShiftX + radiusShiftY;
                topRightCorner = topRightCorner + radiusShiftX + radiusShiftY;
            }

            polyline = createPolylineByVertexes(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner, drawBulge, drawComplex, radiusX, radiusY);

            result.height = bottomLeftCorner.distanceTo(topLeftCorner);
            result.width = bottomLeftCorner.distanceTo(bottomRightCorner);
            result.centerPoint = (bottomLeftCorner + topRightCorner) / 2;

            if (rotate) {
                // rotate corners:
                // now we'll rotate shape on specific angle
                polyline->rotate(m_actionData->corner1, baseAngle);
                result.centerPoint = result.centerPoint.rotate(m_actionData->corner1, baseAngle);
            }
        }
    }
    result.resultingPolyline = polyline;
    return result;
}

/**
 * calculation of second corner - takes into consideration that base angle may be set
 */
void LC_ActionDrawRectangle3Points::calculateCorner2(const RS_Vector &snapPoint, double angleRad, bool cornerSet) const{

    if (m_baseAngleIsFixed){
        m_actionData->corner2 =calculatePossibleEndpointForAngle(snapPoint, m_actionData->corner1, angleRad);
    } else {
        m_actionData->corner2 = snapPoint;
    }
    m_actionData->corner2Set = cornerSet;
    m_actionData->corner3 = m_actionData->corner2;
}

bool LC_ActionDrawRectangle3Points::doCheckMayDrawPreview([[maybe_unused]]LC_MouseEvent *event, int status){
    return status != SetPoint1 && m_actionData->corner1.valid;
}

/**
 * Performing snap on mouse move. Snap depends on state and setting. 
 * If shift is not pressed - ordinary snap is used. 
 * If SHIFT Is pressed - for setting point 2 angle snap use used. 
 * For setting point 3 - with SHIFT< a snap to relative angle is used for quadrangle if inner angle is not fixed. 
 * Otherwise, for rect a square figure is created
 * @param e
 * @return 
 */
RS_Vector LC_ActionDrawRectangle3Points::doGetMouseSnapPoint(LC_MouseEvent *e){
    RS_Vector snapped = e->snapPoint;
    // Snapping to angle(15*) if shift key is pressed
    if (m_alternativeActionMode){
        int status = getStatus();
        switch (status){
            case (SetPoint2):
                if (!m_baseAngleIsFixed){
                    // if base angle is not explicitly set, try to make snap to angle
                    snapped = getSnapAngleAwarePoint(e, m_actionData->corner1, snapped, isMouseMove(e));
                }
               break;
            case (SetPoint3):{
                if (m_createQuadrangle && !m_innerAngleIsFixed){
                    // we'll do angle snap, yet related ot the base edge, so we'll calculate an angle 
                    // from corner 1 to corner 2 first
                    double baseAngle = m_actionData->corner1.angleTo(m_actionData->corner2);
                    snapped = snapToRelativeAngle(baseAngle, snapped, m_actionData->corner2, isMouseMove(e));
                }
                else{ // draw square
                    // width of rect
                    double width = m_actionData->corner2.distanceTo(m_actionData->corner1);
                    // angle for base edge
                    double baseAngle = m_actionData->corner1.angleTo(m_actionData->corner2);

                    RS_Vector tmpPoint = m_actionData->corner2;

                    // end point of base edge rotated to be parallel to x-axis
                    tmpPoint.rotate(m_actionData->corner1, -baseAngle);
                    // set height equal to width
                    if (snapped.y > m_actionData->corner2.y){ // mouse above corner 2
                        tmpPoint.y = tmpPoint.y + width;
                    }
                    else{ // mouse below corner 2
                        tmpPoint.y = tmpPoint.y - width;
                    }

                    // rotate back to get snap point
                    tmpPoint.rotate(m_actionData->corner1, baseAngle);

                    snapped = tmpPoint;
                }
                break;
            }
            default:
              break;
        }
    }
    return snapped;
}

int LC_ActionDrawRectangle3Points::doGetStatusForInitialSnapToRelativeZero(){
    return SetPoint1;
}

void LC_ActionDrawRectangle3Points::doInitialSnapToRelativeZero(RS_Vector zero){
    doResetPoints(zero);
    m_actionData->corner1Set = true;
    setMainStatus(SetPoint2);
}

void LC_ActionDrawRectangle3Points::doOnLeftMouseButtonRelease([[maybe_unused]]LC_MouseEvent *e, int status, const RS_Vector &snapPoint){
    onCoordinateEvent(status, false, snapPoint);
    if (m_actionData->corner2Set){ // adjust relative zero for point 2 (for point 3 it will be set on trigger)
        moveRelativeZero(m_actionData->corner2);
    }
}

void LC_ActionDrawRectangle3Points::doFinish([[maybe_unused]]bool updateTB){
    resetPoints();
}

void LC_ActionDrawRectangle3Points::doBack([[maybe_unused]]LC_MouseEvent *pEvent, int status){
    switch (status){
        case (SetPoint1):{
            finishAction();
            break;
        }
        case (SetPoint3):{
            moveRelativeZero(m_actionData->corner1);
            doResetPoints(m_actionData->corner1);
            m_actionData->corner1Set = true;
            setMainStatus(SetPoint2);
            break;
        }
        case (SetPoint2):{
            setMainStatus(SetPoint1);
            m_actionData->corner1Set = false;
            break;
        }
        default:
            restoreMainStatus();
            break;
    }
}

void LC_ActionDrawRectangle3Points::init(int status){
    if (status >= 0){
        resetPoints();
    }
    LC_AbstractActionWithPreview::init(status);
}

void LC_ActionDrawRectangle3Points::resetPoints(){
      auto zero = RS_Vector(false);
      doResetPoints(zero);
}

void LC_ActionDrawRectangle3Points::doResetPoints(const RS_Vector &zero){
    m_actionData->corner1 = zero;
    m_actionData->corner2 = zero;
    m_actionData->corner3 = zero;
    m_actionData->corner4 = zero;
    m_actionData->corner1Set = false;
    m_actionData->corner2Set = false;
}

/**
 * Calculates possible endpoint as projection of snap point on infinite vector from given start point in given vector
 * @param snap snap point
 * @param lineStartPoint start of vector
 * @param angle direction of vector, in radians
 * @return projection of snap to infinite line
 */
RS_Vector LC_ActionDrawRectangle3Points::calculatePossibleEndpointForAngle(const RS_Vector &snap, const RS_Vector lineStartPoint, double angle) const{
    RS_Vector infiniteVectorEndPoint = lineStartPoint.relative(10.0, angle);
    RS_Vector pointOnInfiniteVector = LC_LineMath::getNearestPointOnInfiniteLine(snap, lineStartPoint, infiniteVectorEndPoint);
    return pointOnInfiniteVector;
}

/**
 * Calculates coordinates of rect/quadrangle (if inner angle is set) based on previously set corner1 and provided size
 * @param size width and height of rectangle
 */
void LC_ActionDrawRectangle3Points::calculateCornersBySize(RS_Vector size){
    double angleRad = getActualBaseAngle();
    RS_Vector result1 =m_actionData->corner1.relative(size.x, angleRad);
    m_actionData->corner2 = result1;
    m_actionData->corner2Set = true;
    double actualInnerAngle = getActualInnerAngle();
    RS_Vector result2 = m_actionData->corner2.relative(size.y, angleRad + actualInnerAngle);
    m_actionData->corner3 = result2;
    calculateCorner4();
}

void LC_ActionDrawRectangle3Points::doProcessCoordinateEvent(const RS_Vector &mouse, bool zero, [[maybe_unused]]int status){
switch (getStatus()) {
        case SetPoint1: {
            doResetPoints(mouse);
            m_actionData->corner1Set = true;
            moveRelativeZero(mouse);
            setMainStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            double angleRad = getActualBaseAngle();
            calculateCorner2(mouse, angleRad, true);
            moveRelativeZero(mouse);
            setMainStatus(SetPoint3);
            break;
        }
        case SetWidth: {
            double angleRad = getActualBaseAngle();
            calculateCorner2(mouse, angleRad, true);
            moveRelativeZero(mouse);
            setMainStatus(SetHeight);
            break;
        }
        case SetSize:{
            calculateCornersBySize(mouse);
            createShapeData(mouse);
            trigger();
            break;
        }
        case SetAngle:{
            if (zero){
                setBaseAngleFixed(false);
                restoreMainStatus();
            }
            break;
        }
        case SetInnerAngle:{
            if (zero){
                setInnerAngleFixed(false);
                restoreMainStatus();
            }
            break;
        }
        case SetPoint3:
        case SetHeight: {
            createShapeData(mouse);
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawRectangle3Points::processCommandValue(double value, bool &toMainStatus){
    switch (getStatus()){
        case SetInnerAngle:
            m_innerAngle = value;
            updateOptions();
            restoreMainStatus();
            break;
        case SetWidth: {
            double angleRad = getActualBaseAngle();
            RS_Vector result1 = m_actionData->corner1.relative(value, angleRad);
            m_actionData->corner2 = result1;
            m_actionData->corner2Set = true;
            m_actionData->corner3 = m_actionData->corner2;
            m_actionData->corner4 - m_actionData->corner2;
            moveRelativeZero(m_actionData->corner2);
            deletePreview();
            LC_AbstractActionDrawRectangle::ShapeData data = createPolyline(RS_Vector(false));
            previewEntity(data.resultingPolyline);
            drawPreview();
            setStatus(SetHeight);
            toMainStatus = false;
            break;
        }
        case SetHeight: {
            double baseAngle = m_actionData->corner1.angleTo(m_actionData->corner2);
            double innerAngleRad = getActualInnerAngle();
            RS_Vector result1 = m_actionData->corner2.relative(value, baseAngle + innerAngleRad);
            m_actionData->corner3 = result1;
            createShapeData(m_actionData->corner3);
            trigger();
            break;
        }
    }
}
/**
 * Returns inner angle for rectangle as PI/2 or, if quadrangle is needed and inner angle is specified - 
 * inner angle in radians, if not inner angle set - will return PI/2
 * @return calculated angle in radians
 */
double LC_ActionDrawRectangle3Points::getActualInnerAngle() const{
    double result = M_PI / 2;
    if (m_createQuadrangle && m_innerAngleIsFixed){
        result = RS_Math::deg2rad(m_innerAngle);
    }
    return result;
}

bool LC_ActionDrawRectangle3Points::processCustomCommand([[maybe_unused]]int status, const QString &command, bool &toMainStatus){
    bool result = true;
    if (checkCommand("pos",command)){ // setting start point
        resetPoints();
        setMainStatus(SetPoint1);
    }
    else if (checkCommand("quad",command)){ // sets quadrangle mode
        m_createQuadrangle = true;
        updateOptions();
    }
    if (checkCommand("noquad",command)){ // sets rectangle mode
        m_createQuadrangle = false;
        updateOptions();
    }
    else if (checkCommand("angle_inner",command)){ // starts entering of inner angle for quadrangle (and enables quadrangle mode)
        m_innerAngleIsFixed = true;
        setStatus(SetInnerAngle);
        toMainStatus = false;
    }
    else if (checkCommand("width", command)){ // starts entering width value
        if (m_actionData->corner1Set){
            setStatus(SetWidth);
            toMainStatus = false;
        }
        else{
            commandMessage(tr("Specify first point first"));
        }
    }
    else if (checkCommand("height", command)){ // starts entering height value
        if (m_actionData->corner2Set){
            setStatus(SetHeight);
            toMainStatus = false;
        }
        else{
            commandMessage(tr("Specify width or second point first"));
        }
    }
    else if (checkCommand("size", command)){ // starts entering size as (width, height)
        if (m_actionData->corner1Set){
            setStatus(SetSize);
            toMainStatus = false;
        }
        else{
            commandMessage(tr("Specify first point first"));
        }
    }
    else{
        result = false;
    }
    return result;
}

/**
 * simply calculates coordinates of corner4
 */
void LC_ActionDrawRectangle3Points::calculateCorner4() const{
    RS_Vector tangentBase = m_actionData->corner2 - m_actionData->corner1;
    m_actionData->corner4 = m_actionData->corner3 - tangentBase;
}

void LC_ActionDrawRectangle3Points::doUpdateMouseButtonHints(int status){
    switch (status) {
        case SetWidth:
            updateMouseWidgetTRBack(tr("Specify width"));
            break;
        case SetHeight:
            updateMouseWidgetTRBack(tr("Specify height"));
            break;
        case SetPoint1:
            updateMouseWidgetTRBack(tr("Specify start point)"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Specify second point"), m_baseAngleIsFixed ? MOD_NONE: MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetPoint3:
            updateMouseWidgetTRBack(tr("Specify third point"),MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetAngle:
            updateMouseWidgetTRBack(tr("Specify angle"));
            break;
        case SetInnerAngle:
            updateMouseWidgetTRBack(tr("Specify inner angle"));
            break;
        default:
            LC_AbstractActionDrawRectangle::doUpdateMouseButtonHints(status);
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionDrawRectangle3Points::createOptionsWidget(){
    return new LC_Rectangle3PointsOptions();
}
