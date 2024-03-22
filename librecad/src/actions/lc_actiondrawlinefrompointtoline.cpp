#include <cmath>
#include "lc_actiondrawlinefrompointtoline.h"
#include "lc_linefrompointtolineoptions.h"
#include "rs_math.h"
#include "lc_linemath.h"
#include "rs_graphicview.h"
#include <QMouseEvent>

LC_ActionDrawLineFromPointToLine::LC_ActionDrawLineFromPointToLine(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview("Draw Orth To Line", container, graphicView),
    targetLine(nullptr),
    startPoint(false) {
    actionType = RS2::ActionDrawLineFromPointToLine;
}

void LC_ActionDrawLineFromPointToLine::doMouseMoveStart(int status, QMouseEvent *pEvent, bool shiftPressed){
    if (status == SelectLine && shiftPressed){
        alternateAngle = true;
    }
}

void LC_ActionDrawLineFromPointToLine::doMouseMoveEnd(int status, QMouseEvent *e){
    alternateAngle = false;
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
    RS_Line* line = createOrtLine(targetLine);
    list << line;
}

/*
 * do post trigger cleanup and go to point selection state
 */
void LC_ActionDrawLineFromPointToLine::doAfterTrigger(){
    targetLine = nullptr;
    startPoint = RS_Vector(false);
    alternateAngle = false;
    setStatus(SetPoint);
}

/**
 * support of snapping to relative point on mouse move
 * @return
 */
int LC_ActionDrawLineFromPointToLine::doRelZeroInitialSnapState(){
    return SetPoint;
}

/**
 * rely on relative zero for first point
 * @param zero
 */
void LC_ActionDrawLineFromPointToLine::doRelZeroInitialSnap(RS_Vector zero){
    startPoint = zero;
    setStatus(SelectLine);
}

/**
 * left mouse clicks processing
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawLineFromPointToLine::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed){
    switch (status){
        case (SetPoint):{
            onCoordinateEvent(snapPoint, false, status);
            break;
        }
        case (SelectLine):{
            RS_Entity* en = catchEntity(snapPoint, RS2::EntityLine, RS2::ResolveAll);
            if (en != nullptr){
                targetLine = dynamic_cast<RS_Line *>(en);
                alternateAngle = shiftPressed;
                trigger();
            }
            break;
        }
    }
}

/**
 * need preview if point was selected only
 * @param event
 * @param status
 * @return
 */
bool LC_ActionDrawLineFromPointToLine::doCheckMayDrawPreview(QMouseEvent *event, int status){
    return status != SetPoint;
}

/**
 * snap entity, check whether it's line and build line from start point to that line
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionDrawLineFromPointToLine::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    if (status == SelectLine){
        RS_Entity* en = catchEntity(snap, RS2::EntityLine, RS2::ResolveAll);
        if (en != nullptr){
            RS_Line* potentialLine = dynamic_cast<RS_Line *>(en);
            highlightEntity(potentialLine);
            RS_Line* line = createOrtLine(potentialLine);
            if (line != nullptr){
                list << line;
            }
        }
    }
}

/**
 * processing of coordinates for start point via mouse click or command widget
 * @param coord
 * @param isZero
 * @param status
 */
void LC_ActionDrawLineFromPointToLine::onCoordinateEvent(const RS_Vector &coord, bool isZero, int status){
    if (status == SetPoint){
        startPoint = coord;
        setStatus(SelectLine);
        graphicView->moveRelativeZero(coord);
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
RS_Line *LC_ActionDrawLineFromPointToLine::createOrtLine(RS_Line *line){
    RS_Vector lineStart = line->getStartpoint();
    RS_Vector lineEnd = line->getEndpoint();

    double targetLineAngle = lineStart.angleTo(lineEnd);

    // rotate line coordinates around lineStart point so they will be parallel to X axis

    lineStart.rotate(startPoint, -targetLineAngle);
    lineEnd.rotate(startPoint, -targetLineAngle);

    // define angle that should be used
    double vectorAngle;

    if (orthogonalMode){
        vectorAngle = RS_Math::deg2rad(90);
    }
    else{
        double angleToUse = angle;
        if (alternateAngle){
            angleToUse = -angle;
        }
        double resultingAngle = RS_Math::deg2rad(/*180-*/angleToUse);
        vectorAngle = RS_Math::correctAngle3(resultingAngle);
    }

    if (startPoint.y > lineStart.y){ // lineStart point is above
    }
    else{
        vectorAngle = -vectorAngle;
    }

    // create direction vector

    RS_Vector directionVector = RS_Vector::polar(length, vectorAngle);

    RS_Vector ortLineStart;
    RS_Vector ortLineEnd;

    switch (sizeMode) {
        case SIZE_INTERSECTION: { //
            ortLineStart = startPoint; // in this mode, we just build the line from start point

            // calculate end point of direction vector positioned in start point
            RS_Vector vectorEnd = startPoint + directionVector;

            // determine intersection point
            RS_Vector intersectionPoint = LC_LineMath::getIntersectionLineLine(startPoint, vectorEnd, lineStart, lineEnd);

            if (intersectionPoint.valid){
                // rotate intersection back to return to drawing coordinates
                RS_Vector restoredIntersection = intersectionPoint.rotate(startPoint, targetLineAngle);

                // end of the line to be build is intersection point
                ortLineEnd = restoredIntersection;
            } else {
                // should not be there - if we're here, it means calculation error, since it is always should be possible to create a line from point to line
                ortLineEnd = ortLineStart;
            }
            break;
        }
        case SIZE_FIXED_LENGTH: {
            RS_Vector vectorOffsetCorrection(0, 0, 0); // vector user to handle different snap points (start, end, middle) for the line

            switch (lineSnapMode) {
                case SNAP_START:
                    if (vectorAngle > 0){
                        vectorOffsetCorrection = RS_Vector::polar(-length, vectorAngle);
                    }
                    break;
                case SNAP_END:
                    if (vectorAngle < 0){
                        vectorOffsetCorrection = RS_Vector::polar(-length, vectorAngle);
                    }
                    break;
                case SNAP_MIDDLE:
                    vectorOffsetCorrection = RS_Vector::polar(-length / 2, vectorAngle);
                    break;
            }

            // correct start point according to current snap mode
            ortLineStart = startPoint + vectorOffsetCorrection;

            // define end point of line
            ortLineEnd = ortLineStart + directionVector;

            // restore rotated position back to drawing
            ortLineStart.rotate(startPoint, targetLineAngle);
            ortLineEnd.rotate(startPoint, targetLineAngle);
            break;
        }
    }
    // resulting line
    RS_Line* result = new RS_Line(container,ortLineStart, ortLineEnd);
    return result;
}

void LC_ActionDrawLineFromPointToLine::updateMouseButtonHints(){
    switch (getStatus()){
        case SetPoint:
            updateMouseWidgetTR("Select Initial Point", "Cancel");
            break;
        case SelectLine:
            updateMouseWidgetTR("Select Line", "Back");
            break;
        default:
            break;
    }
}

void LC_ActionDrawLineFromPointToLine::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_LineFromPointToLineOptions>(nullptr);
}

RS2::CursorType LC_ActionDrawLineFromPointToLine::doGetMouseCursor(int status){
    return RS2::SelectCursor;
}
