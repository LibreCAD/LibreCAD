/****************************************************************************
**
* Action that draws a line from given point to selected line.
* Created line may be either orthogonal to selected line, or be with
* some specified angle.

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
#include "rs_math.h"
#include "lc_linemath.h"
#include "rs_point.h"
#include "lc_abstractactionwithpreview.h"
#include "lc_actiondrawlinefrompointtoline.h"
#include "lc_linefrompointtolineoptions.h"
#include "rs_previewactioninterface.h"

LC_ActionDrawLineFromPointToLine::LC_ActionDrawLineFromPointToLine(
    QG_ActionHandler* a_handler,
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview("Draw Line To Line", container, graphicView){
    actionType = RS2::ActionDrawLineFromPointToLine;
    actionhandler = a_handler;
}


/*
 * check that we're fine to trigger
 */
bool LC_ActionDrawLineFromPointToLine::doCheckMayTrigger(){
    return targetLine != nullptr;
}

/**
 * just create line according to given parameters
 * @param list
 */
void LC_ActionDrawLineFromPointToLine::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    RS_Vector intersection;
    RS_Line* line = createLineFromPointToTarget(targetLine, intersection);
    list << line;
}

/*
 * do post trigger cleanup and go to point selection state
 */
void LC_ActionDrawLineFromPointToLine::doAfterTrigger(){
    targetLine = nullptr;
    startPoint = RS_Vector(false);
    restoreSnapMode();
    setStatus(SetPoint);
}

void LC_ActionDrawLineFromPointToLine::doBack(QMouseEvent *pEvent, int status){
    if (status == SelectLine){
        restoreSnapMode();
    }
    LC_AbstractActionWithPreview::doBack(pEvent, status);
}

void LC_ActionDrawLineFromPointToLine::doFinish([[maybe_unused]]bool updateTB){
    if (getStatus() == SelectLine){
        restoreSnapMode();
    }
}

/**
 * support of snapping to relative point on mouse move
 * @return
 */
int LC_ActionDrawLineFromPointToLine::doGetStatusForInitialSnapToRelativeZero(){
    return SetPoint;
}

/**
 * rely on relative zero for first point
 * @param zero
 */
void LC_ActionDrawLineFromPointToLine::doInitialSnapToRelativeZero(RS_Vector zero){
    // we'll use current relative point as starting point on pre-snap (mouse move + SHIFT)
    startPoint = zero;
    setStatus(SelectLine);
    setFreeSnap();
}

/**
 * left mouse clicks processing
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawLineFromPointToLine::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status, const RS_Vector &snapPoint){
    switch (status){
        case (SetPoint):{
            onCoordinateEvent(status, false, snapPoint);
            break;
        }
        case (SelectLine):{
            // try to select target line
            RS_Entity* en = catchModifiableEntity(e, RS2::EntityLine);
            if (en != nullptr){
                targetLine = dynamic_cast<RS_Line *>(en);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

/**
 * need preview if point was selected only
 * @param event
 * @param status
 * @return
 */
bool LC_ActionDrawLineFromPointToLine::doCheckMayDrawPreview([[maybe_unused]] QMouseEvent *event, int status){
    return status != SetPoint; // draw preview if we're selecting the line only
}

/**
 * snap entity, check whether it's line and build line from start point to that line
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionDrawLineFromPointToLine::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    if (status == SelectLine){
        RS_Entity* en = catchModifiableEntity(e, RS2::EntityLine);
        RS_Line* line;
        if (en != nullptr){
            auto potentialLine = dynamic_cast<RS_Line *>(en);
            highlightHover(potentialLine);
            auto intersectionPoint = RS_Vector(false);
            line = createLineFromPointToTarget(potentialLine, intersectionPoint);
            if (showRefEntitiesOnPreview) {
                createRefPoint(line->getEndpoint(), list);
                if (sizeMode == SIZE_INTERSECTION && LC_LineMath::isMeaningful(endOffset)) {
                    createRefPoint(intersectionPoint, list);
                }
            }
        }
        else{
            line = new RS_Line(startPoint, snap);
        }
        if (showRefEntitiesOnPreview) {
            createRefSelectablePoint(snap, list);
            createRefPoint(startPoint, list);
        }
        list << line;
    }
}

/**
 * processing of coordinates for start point via mouse click or command widget
 * @param coord
 * @param isZero
 * @param status
 */
void LC_ActionDrawLineFromPointToLine::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &coord) {
    if (status == SetPoint){
        startPoint = coord;
        // for simplicity of line selection, remove snap restrictions until we'll select a line
        setFreeSnap();
        setStatus(SelectLine);
        // relative zero will remain in starting point
        moveRelativeZero(coord);
    }
}

/**
 * Central method for building the line. First, for the simplicity of calculations, we'll rotate coordinates of target line around start point,
 * so the line will be parallel to x axis.
 * Than, calculate proper angles for the vector direction.
 * That direction vector is positioned to start point, and based on the size policy we build the line in the direction of the vector
 * either to the point of intersection of target line and ray defined by the direction vector, or just build the line of given length.
 * As soon as all calculations are performed, perform backward rotation.
 *
 * @param line
 * @return
 */
RS_Line *LC_ActionDrawLineFromPointToLine::createLineFromPointToTarget(RS_Line *line, RS_Vector& intersection){
    RS_Vector lineStart = line->getStartpoint();
    RS_Vector lineEnd = line->getEndpoint();

    double targetLineAngle = lineStart.angleTo(lineEnd);

    // rotate line coordinates around lineStart point, so they will be parallel to X axis

    lineStart.rotate(startPoint, -targetLineAngle);
    lineEnd.rotate(startPoint, -targetLineAngle);

    // define angle that should be used
    double vectorAngle;

    if (orthogonalMode){
        vectorAngle = RS_Math::deg2rad(90);
    }
    else{
        // determine which angle should be used - normal or alternate
        // alternative angle allows to simplify angle setting - so in ui the angle is within 0..90 degrees.
        double angleToUse = angle;
        if (alternativeActionMode){
            angleToUse = -angle;
        }
        double resultingAngle = RS_Math::deg2rad(angleToUse);
        vectorAngle = resultingAngle;
//        vectorAngle = RS_Math::correctAngle3(resultingAngle);
    }

    if (startPoint.y >= lineStart.y){ // lineStart point is above
    }
    else{
        vectorAngle = -vectorAngle;
    }

    // create direction vector

    RS_Vector directionVector = RS_Vector::polar(length, vectorAngle);

    RS_Vector ortLineStart;
    RS_Vector ortLineEnd;

    switch (sizeMode) {
        case SIZE_INTERSECTION: { // create line from point to intersection point
            ortLineStart = startPoint; // in this mode, we just build the line from start point

            // calculate end point of direction vector positioned in start point
            RS_Vector vectorEnd = startPoint + directionVector;

            // determine intersection point
            RS_Vector intersectionPoint = LC_LineMath::getIntersectionLineLine(startPoint, vectorEnd, lineStart, lineEnd);

            if (intersectionPoint.valid){
                // rotate intersection back to return to drawing coordinates
                RS_Vector restoredIntersection = intersectionPoint.rotate(startPoint, targetLineAngle);
                // process end offset from intersection point, if needed
                RS_Vector endPoint = restoredIntersection;
                if (LC_LineMath::isMeaningful(endOffset)){
                    RS_Vector offsetVector = RS_Vector::polar(endOffset, startPoint.angleTo(restoredIntersection));
                    endPoint = restoredIntersection + offsetVector;
                }

                intersection  = restoredIntersection;

                // end of the line to be build is intersection point
                ortLineEnd = endPoint;
            } else {
                // should not be there - if we're here, it means calculation error, since it is always should be possible to create a line from point to line
                ortLineEnd = ortLineStart;
            }
            break;
        }
        case SIZE_FIXED_LENGTH: { // create line from point in direction to intersection point yet with given length, taking into consideration snap mode
            switch (lineSnapMode) {
                case SNAP_START: // start point of line will be in initial point
                    // correct start point according to current snap mode
                    ortLineStart = startPoint;
                    // define end point of line
                    if (alternativeActionMode){
                        ortLineEnd = ortLineStart + directionVector;
                    }
                    else {
                        ortLineEnd = ortLineStart - directionVector;
                    }
                    break;
                case SNAP_END: // end point of the line will be in initial point
                    ortLineEnd = startPoint;
                    if (alternativeActionMode){
                        ortLineStart = ortLineEnd - directionVector;
                    }
                    else{
                        ortLineStart = ortLineEnd + directionVector;
                    }

                    break;
                case SNAP_MIDDLE: // middle point of line will be in initial point
                    RS_Vector vectorOffsetCorrection = RS_Vector::polar(length / 2, vectorAngle);
                    if (alternativeActionMode){
                        ortLineStart = startPoint - vectorOffsetCorrection;
                        ortLineEnd = startPoint + vectorOffsetCorrection;
                    }
                    else{
                        ortLineStart = startPoint + vectorOffsetCorrection;
                        ortLineEnd = startPoint - vectorOffsetCorrection;
                    }
                    break;
            }
            // restore rotated position back to drawing
            ortLineStart.rotate(startPoint, targetLineAngle);
            ortLineEnd.rotate(startPoint, targetLineAngle);
            break;
        }
    }
    // resulting line
    auto* result = new RS_Line(container,ortLineStart, ortLineEnd);
    return result;
}

void LC_ActionDrawLineFromPointToLine::updateMouseButtonHints(){
    switch (getStatus()){
        case SetPoint:
            updateMouseWidgetTRCancel(tr("Select Initial Point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SelectLine:
            updateMouseWidgetTRBack(tr("Select Line"), (orthogonalMode && (sizeMode == SIZE_INTERSECTION))? MOD_NONE : MOD_SHIFT_MIRROR_ANGLE);
            break;
        default:
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionDrawLineFromPointToLine::createOptionsWidget(){
    return new LC_LineFromPointToLineOptions();
}

RS2::CursorType LC_ActionDrawLineFromPointToLine::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}
