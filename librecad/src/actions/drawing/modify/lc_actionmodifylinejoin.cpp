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
#include <QMouseEvent>
#include <cmath>
#include "rs_polyline.h"
#include "rs_graphicview.h"
#include "lc_linemath.h"
#include "lc_linejoinoptions.h"
#include "lc_actionmodifylinejoin.h"

LC_ActionModifyLineJoin::LC_ActionModifyLineJoin(RS_EntityContainer &container, RS_GraphicView &graphicView):
    LC_AbstractActionWithPreview("ModifyLineJoin", container, graphicView),
    line1(nullptr), line2(nullptr){
    actionType = RS2::ActionModifyLineJoin;
}

LC_ActionModifyLineJoin::~LC_ActionModifyLineJoin() = default;

void LC_ActionModifyLineJoin::init(int status){
    LC_AbstractActionWithPreview::init(status);
    line1 = nullptr;
    line2 = nullptr;
}

/*
 * utility method that catches line based on mouse event
 */
RS_Line *LC_ActionModifyLineJoin::catchLine(QMouseEvent *e){
    RS_Entity *en = catchModifiableEntity(e, lineType);
    RS_Line *snappedLine = nullptr;
    if (isLine(en)){
        snappedLine = dynamic_cast<RS_Line *>(en);
    }
    return snappedLine;
}

void LC_ActionModifyLineJoin::doPreparePreviewEntities(QMouseEvent *e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, int status){

    RS_Line *snappedLine = catchLine(e);
    switch (status) {
        case SetLine1: {
            if (snappedLine != nullptr){ // can snap to line
                highlightHover(snappedLine); // just highlight line 1
            }
            break;
        }
        case SetLine2: {
            if (snappedLine == line1){ // don't let to snap to the same line again
                snappedLine = nullptr;
            }
            highlightSelected(line1);
            if (snappedLine != nullptr){
                highlightHover(snappedLine);
                // here we do not rely on snap point, simply get coordinates from event
                RS_Vector coord = toGraph(e);
                LC_LineJoinData *lineJoinData = createLineJoinData(snappedLine, coord);
                if (lineJoinData != nullptr){
                    RS_Polyline *polyline = lineJoinData->polyline;
                    if (polyline != nullptr){
                        list << polyline;
                    }

                    if (showRefEntitiesOnPreview) {
                        if (!lineJoinData->parallelLines) {
                            RS_Vector &intersectionPoint = lineJoinData->intersectPoint;
                            createRefPoint(intersectionPoint, list);
                        }

                        RS_Vector &major1 = lineJoinData->majorPointLine1;
                        if (major1.valid) {
                            createRefPoint(major1, list);
                        }

                        RS_Vector &major2 = lineJoinData->majorPointLine2;
                        if (major2.valid) {
                            createRefPoint(major2, list);
                        }
                    }

                    // we don't need line joint data so far
                    delete lineJoinData;
                }
            }
            break;
        }
        case ResolveFirstLineTrim: {
            if (snappedLine != line1){
                // don't let to snap on the other line except line 1 (as we need a point on the line 1 to determine which side from intersection
                // should survive trim
                snappedLine = nullptr;
            }
            highlightSelected(line1);
            if (snappedLine != nullptr){
                // retrieve current mouse position and recalculate line join data considering that mose position denotes part of line 1 that
                // will survive trim operation
                RS_Vector coord = toGraph(e);
                updateLine1TrimData(coord);

                RS_Polyline *polyline = linesJoinData->polyline;
                if (polyline != nullptr){
                    list << polyline->clone();
                    if (showRefEntitiesOnPreview) {
                        if (!linesJoinData->parallelLines) {
                            RS_Vector &intersectionPoint = linesJoinData->intersectPoint;
                            createRefPoint(intersectionPoint, list);
                        }
                        createRefPoint(linesJoinData->majorPointLine1, list);
                        createRefPoint(linesJoinData->majorPointLine2, list);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyLineJoin::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, [[maybe_unused]]const RS_Vector &snapPoint){
    RS_Line *snappedLine = catchLine(e);
    switch (status) {
        case SetLine1:
            if (snappedLine != nullptr){ // just store first line and proceed to selection of second line
                line1 = snappedLine;
                line1ClickPosition = snapPoint;
                setStatus(SetLine2);
            } else {
                commandMessage(tr("No line selected"));
            }
            break;
        case SetLine2:
            if (snappedLine == line1){ // don't let to snap to the same line again
                snappedLine = nullptr;
            }
            if (snappedLine != nullptr){
                line2 = snappedLine;
                RS_Vector snap = toGraph(e);
                LC_LineJoinData *joinData = createLineJoinData(snappedLine, snap);
                if (joinData != nullptr){
                    // check whether parallel lines were selected
                    if (joinData->parallelLines){
                        // check whether lines are on the same ray
                        if (joinData->straightLinesConnection){
                            // ok, let's merge them
                            linesJoinData = joinData;
                            trigger();
                        } else { // truly parallel lines, can't merge them
                            commandMessage(tr("Lines are parallel, can't merge"));
                        }
                    } else { // lines may merge with angle
                        if (joinData->areLinesAlreadyIntersected()){
                            // both lines are already crossed, do nothing
                            commandMessage(tr("Lines already intersects, can't merge"));
                        } else {
                            // check whether intersection is on the first line.
                            // if it so, and mode for line is EXTEND/TRIM, we need to ask the user
                            // which part of the line 1 should be trimmed and which one would
                            // survive trim.
                            // For such case, we need addition selection
                            bool firstLineTrimShouldBeSpecified = joinData->isIntersectionOnLine1();
                            if (line1EdgeMode != EDGE_EXTEND_TRIM){
                                firstLineTrimShouldBeSpecified = false;
                            }
                            linesJoinData = joinData;
                            if (firstLineTrimShouldBeSpecified){
                                // we need to addition hint from the user for proper trim, so go to the corresponding state
                                setStatus(ResolveFirstLineTrim);
                                highlightEntity(line1);
                                graphicView->redraw();
                            } else {
                                // we are find for joining/trimming lines, just invoke trigger
                                trigger();
                            }
                        }
                    }
                } else {
                    commandMessage(tr("No line selected"));
                }
            }
            break;

        case ResolveFirstLineTrim:
            if (snappedLine == line1){ // we need trim hint on the first line
                RS_Vector snap = toGraph(e);
                // update trim data according to selected part of line 1
                updateLine1TrimData(snap);
                // check if polyline is built and if it so - trigger action
                RS_Polyline *polyline = linesJoinData->polyline;
                if (polyline != nullptr){
                    trigger();
                }
            } else {
                highlightEntity(line1);
            }

            break;
        default:
            break;
    }
}

void LC_ActionModifyLineJoin::doBack(QMouseEvent *pEvent, int status){
    LC_AbstractActionWithPreview::doBack(pEvent, status);
    switch (status) {
        case SetLine1:
            init(SetLine1 - 1);
            break;
        case SetLine2: {
            setStatus(SetLine1);
            break;
        }
        case ResolveFirstLineTrim: {
            setStatus(SetLine2);
            break;
        }
        default:
            initPrevious(status);
    }
}

void LC_ActionModifyLineJoin::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    delete linesJoinData; // cleanup
    graphicView->redraw();
    // return to selection first line mode
    init(SetLine1);
}

void LC_ActionModifyLineJoin::performTriggerDeletions(){
    // check whether original lines should be deleted
    if (removeOriginalLines){
        // proceed line 1
        if (line1EdgeMode == EDGE_EXTEND_TRIM){
            deleteEntityUndoable(line1);
        }
        // proceed line 2
        if (line2EdgeMode == EDGE_EXTEND_TRIM){
            deleteEntityUndoable(line2);
        }
    }
}

bool LC_ActionModifyLineJoin::doCheckMayTrigger(){
    return linesJoinData != nullptr && document != nullptr;
}

bool LC_ActionModifyLineJoin::isSetActivePenAndLayerOnTrigger(){
    // the action will handle pen and layer based on ui settings
    return false;
}

void LC_ActionModifyLineJoin::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    if (linesJoinData->parallelLines && linesJoinData->straightLinesConnection){ // process straight lines
        // simply create straight line
        auto *line = createLine(linesJoinData->majorPointLine1, linesJoinData->majorPointLine2, list);
        // apply attributes according to settings
        applyAttributes(line, true);
    } else { // process lines that are not on the same ray, and are not parallel

        RS_Vector &intersectionPoint = linesJoinData->intersectPoint;

        // during calculation of line data, we've already processed various modes for each line,
        // therefore we rely on major points there and draw lines from them to intersection point
        RS_Vector &major1 = linesJoinData->majorPointLine1;
        RS_Vector &major2 = linesJoinData->majorPointLine2;
        RS_Line *l1;
        RS_Line *l2;

        if (createPolyline && major1.valid && major2.valid){ // handle polyline mode
            auto *poly = new RS_Polyline(container);
            poly->addVertex(major1);
            poly->addVertex(intersectionPoint);
            poly->addVertex(major2);
            applyAttributes(poly, true);
            list << poly;

        } else { // handle individual lines
            if (major1.valid){  // proceed line 1 (extend or segment)
                l1 = createLine(major1, intersectionPoint, list);
                applyAttributes(l1, true);
            }
            if (major2.valid){ // proceed line 2
                l2 = createLine(intersectionPoint, major2, list);
                applyAttributes(l2, false);
            }
        }
    }
}

/**
 * Apply pen and layer attributes to created lines
 * @param e created entity
 * @param forLine1 if true, we'll apply attributes to line 1, false - to line 2
 */
void LC_ActionModifyLineJoin::applyAttributes(RS_Entity *e, bool forLine1){
    RS_Pen pen;
    RS_Layer *layer;
    switch (attributesSource) {
        case ATTRIBUTES_LINE_1: { // pick attributes from line 1
            pen = line1->getPen(false);
            e->setPen(pen);
            layer = line1->getLayer(true);
            e->setLayer(layer);
            break;
        }
        case ATTRIBUTES_BOTH_LINES:   // pick attributes from each original line individually
            if (forLine1){ // proceed line 1
                pen = line1->getPen(false);
                e->setPen(pen);
                layer = line1->getLayer(true);
                e->setLayer(layer);
            } else { // proceed line 2
                pen = line2->getPen(false);
                e->setPen(pen);
                layer = line2->getLayer(true);
                e->setLayer(layer);
            }
            break;
        case ATTRIBUTES_LINE_2:  // pick attributes from line 2
            pen = line2->getPen(false);
            e->setPen(pen);
            layer = line2->getLayer(true);
            e->setLayer(layer);
            break;
        case ATTRIBUTES_ACTIVE_PEN_LAYER: // just set for active pen and layer
            e->setPenToActive();
            e->setLayerToActive();
            break;
    }
}

/**
 * Create line join data
 * @param secondLine second line
 * @param snapPoint snap point
 * @return created line join data
 */
LC_ActionModifyLineJoin::LC_LineJoinData *LC_ActionModifyLineJoin::createLineJoinData(RS_Line *secondLine, RS_Vector &snapPoint){
    LC_LineJoinData *result = nullptr;

    if (line1 != nullptr && secondLine != nullptr){

        // prepare endpoints vectors
        RS_Vector line1Start = line1->getStartpoint();
        RS_Vector line1End = line1->getEndpoint();
        RS_Vector line2Start = secondLine->getStartpoint();
        RS_Vector line2End = secondLine->getEndpoint();

        RS_Vector intersection = LC_LineMath::getIntersectionLineLine(line1Start, line1End, line2Start, line2End);

        // determine intersection point for line 1 and given line 2
        if (intersection.valid){// has intersection between lines, proceed them
            result = proceedNonParallelLines(line1ClickPosition, snapPoint, intersection, line1Start, line1End, line2Start, line2End);

        } else {// has intersection between lines, proceed them
            result = proceedParallelLinesJoin(line1Start, line1End, line2Start, line2End);
        }
    }
    return result;
}

/**
 * Determine line data for intersecting (non-parallel) lines
 * @param snapPoint snap point
 * @param sol solution for intersection
 * @param line1Start
 * @param line1End
 * @param line2Start
 * @param line2End
 * @return
 */
LC_ActionModifyLineJoin::LC_LineJoinData *LC_ActionModifyLineJoin::proceedNonParallelLines(
     RS_Vector& line1ClickPoint, RS_Vector &snapPoint,
    const RS_Vector &intersectPoint,
    const RS_Vector &line1Start, const RS_Vector &line1End,
    const RS_Vector &line2Start, const RS_Vector &line2End){
    auto *result = new LC_LineJoinData();

    result->intersectPoint = intersectPoint;
    result->parallelLines = false;

    // resulting polyline
    auto *polyline = new RS_Polyline(container);

    // processing of line 1
    // determining how intersection and snap points are located relating to line endpoints
    LC_PointsDisposition line1Disposition = determine3PointsDisposition(line1Start, line1End, intersectPoint, /*snapPoint*/line1ClickPoint);

    // determine major point that will be used for drawing of resulting entities.
    // Based on options, major point may be either one of line endpoints or intersection point

    RS_Vector pointFromLine1 = getMajorPointFromLine(line1EdgeMode, line1Start, line1End, line1Disposition);
    result->majorPointLine1 = pointFromLine1;

    // add major point for line 1 to polyline, if needed
    if (pointFromLine1.valid){
        polyline->addVertex(pointFromLine1);
    }

    // add intersection
    polyline->addVertex(intersectPoint);

    // processing of line 2
    // again, determine how endpoints of line are located relating to intersection and snap points
    LC_PointsDisposition line2Disposition = determine3PointsDisposition(line2Start, line2End, intersectPoint, snapPoint);

    // determine major point for line 2
    RS_Vector pointFromLine2 = getMajorPointFromLine(line2EdgeMode, line2Start, line2End, line2Disposition);
    result->majorPointLine2 = pointFromLine2;

    // add major point from line 2 to polyline, if needed
    if (pointFromLine2.valid){
        polyline->addVertex(pointFromLine2);
    }

    // finalise line point data fields
    result->polyline = polyline;
    result->line1Disposition = line1Disposition;
    result->line2Disposition = line2Disposition;
    result->intersectPoint = intersectPoint;
    return result;
}

/**
 * Updates previously calculated line join data for resolving trim of line 1 (if intersection of lines belongs to line2).
 * Snap point specified by the user indicates part of line 1 that should survive trim operation.
 * @param snap snap point
 */
void LC_ActionModifyLineJoin::updateLine1TrimData(RS_Vector snap){

    const RS_Vector &line1Start = line1->getStartpoint();
    const RS_Vector &line1End = line1->getEndpoint();
    RS_Polyline *polyline = linesJoinData->polyline;
    if (polyline != nullptr){ // we'll rebuild polyline, so delete original one
        delete polyline;
    }
    polyline = new RS_Polyline(container);
    linesJoinData->polyline = polyline;

    RS_Vector &intersection = linesJoinData->intersectPoint;

    // recalculate disposition of line 1 taking into consideration updated snap point
    LC_ActionModifyLineJoin::LC_PointsDisposition line1Disposition = determine3PointsDisposition(line1Start, line1End, intersection, snap);

    // update major point for line 1 based updated disposition
    RS_Vector pointFromLine1 = getMajorPointFromLine(line1EdgeMode, line1Start, line1End, line1Disposition);
    linesJoinData->majorPointLine1 = pointFromLine1;

    // add major point from line 1 to polyline
    if (line1Start.valid){
        polyline->addVertex(pointFromLine1);
    }

    // add intersection
    polyline->addVertex(intersection);

    // get major point for line 2
    RS_Vector pointFromLine2 = getMajorPointFromLine(line2EdgeMode, line2->getStartpoint(), line2->getEndpoint(), linesJoinData->line2Disposition);
    linesJoinData->majorPointLine2 = pointFromLine2;

    // add major point form line 2, if needed
    if (pointFromLine2.valid){
        polyline->addVertex(pointFromLine2);
    }
}

/**
 * Determining major point for the line. Major point depends on edges mode set for specific line and may represent either one of endpoints or
 * intersection point. Major point for the line is the point which will be used for drawing a line from this point to intersection point.
 * @param edgeMode edge mode for the line
 * @param lineStart start point of the line
 * @param lineEnd  end point of the line
 * @param lineDisposition disposition data
 * @return major point of line
 */
RS_Vector LC_ActionModifyLineJoin::getMajorPointFromLine(
    const int edgeMode,
    const RS_Vector &lineStart,
    const RS_Vector &lineEnd,
    const LC_ActionModifyLineJoin::LC_PointsDisposition &lineDisposition) const{
    auto result = RS_Vector(false);
    switch (edgeMode) {
        case EDGE_EXTEND_TRIM:
            switch (lineDisposition.dispositionMode) {
                case LC_PointsDisposition::BOTH_POINTS_ON_RIGHT:
                case LC_PointsDisposition::BOTH_POINTS_ON_LEFT: {
                    result = lineDisposition.farPoint; // we'll use endpoint of line that is more distant (not closer) from intersection
                    break;
                }
                case LC_PointsDisposition::MIDDLE_END_LEFT: {
                    if (lineDisposition.snapSelectionOnLeft){ // determine whether it should be start point or endpoint
                        result = lineEnd;
                    } else {
                        result = lineStart;
                    }
                    break;
                }
                case LC_PointsDisposition::MIDDLE_START_LEFT: {
                    if (lineDisposition.snapSelectionOnLeft){
                        result = lineStart;
                    } else {
                        result = lineEnd;
                    }
                    break;
                }
            }
            break;
        case EDGE_ADD_SEGMENT:
            switch (lineDisposition.dispositionMode) {
                case LC_PointsDisposition::BOTH_POINTS_ON_RIGHT:
                case LC_PointsDisposition::BOTH_POINTS_ON_LEFT: {
                    // for adding segment, we'll select endpoint that is closest to the intersection
                    // as intersection point is outside of line
                    result = lineDisposition.closestPoint;
                    break;
                }
                case LC_PointsDisposition::MIDDLE_END_LEFT:
                case LC_PointsDisposition::MIDDLE_START_LEFT: {
                    // we don't need major point for adding segment mode if intersection point is
                    // within line endpoints
                    break;
                }
            }
            break;
        case EDGE_NO_MODIFICATION:
            break;
        default:
            break;
    }
    return result;
}

/**
 * Resolving parallel lines - check whether both lines lies on the same ray so they may be joined
 * @param line1Start
 * @param line1End
 * @param line2Start
 * @param line2End
 * @return line join data
 */
LC_ActionModifyLineJoin::LC_LineJoinData *LC_ActionModifyLineJoin::proceedParallelLinesJoin(
    const RS_Vector &line1Start, const RS_Vector &line1End,
    const RS_Vector &line2Start, const RS_Vector &line2End) const{

    auto result = new LC_LineJoinData();
    result->parallelLines = true;

    // check whether these lines are on the same ray vector
    bool sameRay = LC_LineMath::areLinesOnSameRay(line1Start, line1End, line2Start, line2End);

    if (sameRay){
        result->straightLinesConnection = true;

        // determine angle from start point to end point

        double angle = line1Start.angleTo(line1End);

        // copy on endpoints
        auto startOnX1 = RS_Vector(line1Start);
        auto endOnX1 = RS_Vector(line1End);
        auto startOnX2 = RS_Vector(line2Start);
        auto endOnX2 = RS_Vector(line2End);

        // rotate all points around start point of  line 1 do they will be on horizontal line (parallel to x-axis)
        endOnX1.rotate(startOnX1, -angle);
        startOnX2.rotate(startOnX1, -angle);
        endOnX2.rotate(startOnX1, -angle);

        // determine positions of each point on x
        double sx1 = startOnX1.x;
        double ex1 = endOnX1.x;
        double sx2 = startOnX2.x;
        double ex2 = endOnX2.x;

        // orient all lines in one direction, from left to right

        RS_Vector s1 = line1Start;
        RS_Vector e1 = line1End;
        RS_Vector s2 = line2Start;
        RS_Vector e2 = line2End;

        // determine who two lines are positioned relating each other.
        // there may be either gap between lines or overlapping of lines

        // position of all endpoints in the same direction,
        // so start points are on the left side of end points

        if (ex1 < sx1){
            std::swap(sx1, ex1);
            std::swap(s1, e1);
        }

        if (ex2 < sx2){
            std::swap(sx2, ex2);
            std::swap(s2, e2);
        }

        RS_Vector leftPoint; // most left point
        RS_Vector rightPoint; // most right point
        RS_Vector middleLeftPoint; // left point in the middle
        RS_Vector middleRightPoint;  // right point in the middle

        // check we have intersection of two lines
        bool hasIntersection;

        if (sx1 < sx2){ // start of line 1 is on left from start of line 2
            hasIntersection = sx2 < ex1; // check whether start of line 2 between start and end of line 1
            leftPoint = s1;
            if (hasIntersection){
                if (ex1 > ex2){
                    rightPoint = e1;
                } else {
                    rightPoint = e2;
                }
            } else {
                rightPoint = e2;
                middleLeftPoint = e1;
                middleRightPoint = s2;
            }
        } else {
            hasIntersection = sx1 < ex2; // check whether start of line 2 between start and end of line 1
            leftPoint = s2;
            if (hasIntersection){
                if (ex1 > ex2){
                    rightPoint = e1;
                } else {
                    rightPoint = e2;
                }
            } else {
                rightPoint = e1;
                middleRightPoint = s1;
                middleLeftPoint = e2;
            }
        }

        // if edge modes for both lines are set to Extend/Trim, we'll
        // merge to lines into single line. Here we don't care whether lines are overlapping
        // or not - just use external endpoints for the new line
        if (line1EdgeMode == EDGE_EXTEND_TRIM || line2EdgeMode == EDGE_EXTEND_TRIM){
            auto *polyline = new RS_Polyline(container);

            // we just use most left and most right point as vertexes
            polyline->addVertex(leftPoint);
            polyline->addVertex(rightPoint);

            // and consider them as major points. Potentially, left and right might be from different lines and so
            // major point may be set for wrong line - yet it seems that's ok for the overall logic of joining
            // for merging lines, it's hard to properly determine right direction of resulting line and, most probably,
            // it's direction is not important at all.

            result->majorPointLine1 = leftPoint;
            result->majorPointLine2 = rightPoint;

            result->polyline = polyline;
        } else if (line1EdgeMode == EDGE_ADD_SEGMENT && line2EdgeMode == EDGE_ADD_SEGMENT){
            // if edge mode is adding segment for both lines, we'll create a single line that
            // fills a gap between lines if there is no intersection
            if (!hasIntersection){
                // we can do this only if lines are not overlapped
                auto *polyline = new RS_Polyline(container);

                // add gap points
                polyline->addVertex(middleLeftPoint);
                polyline->addVertex(middleRightPoint);

                // use them as major points
                result->majorPointLine1 = middleLeftPoint;
                result->majorPointLine2 = middleRightPoint;
                result->polyline = polyline;
            }
        }
    } else {
        result->straightLinesConnection = false;
    }
    return result;
}

/**
 * Function that determines how endpoints of line are located relating to provided intersection point, and on which side from
 * intersection point provided snap point is located.
 * @param start line start point
 * @param end lint end point
 * @param intersection intersection point
 * @param snapPoint snap point
 * @return points disposition
 */
LC_ActionModifyLineJoin::LC_PointsDisposition LC_ActionModifyLineJoin::determine3PointsDisposition(
    const RS_Vector start,
    const RS_Vector end,
    const RS_Vector intersection,
    const RS_Vector &snapPoint) const{

    // align all 3 points horizontally for simplicity of calculations, rotate them as needed

    // angle of line
    double angle = start.angleTo(end);

    // copy of endpoints for rotation
    auto startOnX = RS_Vector(start);
    auto endOnX = RS_Vector(end);

    startOnX.rotate(intersection, -angle);
    endOnX.rotate(intersection, -angle);

    // projection of snap point to line
    RS_Vector snapProjection = LC_LineMath::getNearestPointOnInfiniteLine(snapPoint, start, end);

    // rotated snap point projection
    RS_Vector snapOnX = snapProjection.rotate(intersection, -angle);

    // move coordinates so that the intersection point is in 0

    double ix = intersection.x;
    double delta = ix;


    double startX = startOnX.x;
    double endX = endOnX.x;
    double snapX = snapOnX.x;

    startX -= delta;
    endX -= delta;
    snapX -= delta;

    LC_PointsDisposition result;

    // determine where all points are located relating to intersection point
    if (startX > 0){
        if (endX > 0){
            result.dispositionMode = LC_PointsDisposition::BOTH_POINTS_ON_RIGHT;  // intersection is on the left side of line
        } else {
            result.dispositionMode = LC_PointsDisposition::MIDDLE_END_LEFT; // intersection is between endpoints, end point is on the left
        }
    } else {
        if (endX > 0){
            result.dispositionMode = LC_PointsDisposition::MIDDLE_START_LEFT; // intersection is between endpoints, start point is on the left
        } else {
            result.dispositionMode = LC_PointsDisposition::BOTH_POINTS_ON_LEFT; // both points are on the right side from intersection
        }
    }

    // determine which endpoint of the line is closest to the intersection point
    if (std::abs(endX) > std::abs(startX)){
        result.closestPoint = start;
        result.farPoint = end;
    } else {
        result.closestPoint = end;
        result.farPoint = start;
    }

    // determine position of snap point relating to intersection point, if any
    // check whether snap point is on the left side of line and intersection point is on the right
    result.snapSelectionOnLeft = snapX < 0;
    result.startPoint = start;
    result.endPoint = end;
    return result;
}

void LC_ActionModifyLineJoin::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetLine1:
            updateMouseWidgetTRCancel(tr("Select first line"));
            break;
        case SetLine2:
            updateMouseWidgetTRBack(tr("Select second line"));
            break;
        case ResolveFirstLineTrim:
            updateMouseWidgetTRBack(tr("Select part of first line that should remain after trim"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionModifyLineJoin::createOptionsWidget(){
    return new LC_LineJoinOptions();
}

RS2::CursorType LC_ActionModifyLineJoin::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

void LC_ActionModifyLineJoin::setAttributesSource(int value){
    attributesSource = value;
}

void LC_ActionModifyLineJoin::setCreatePolyline(bool value){
    createPolyline = value;
}

void LC_ActionModifyLineJoin::setRemoveOriginalLines(bool value){
    removeOriginalLines = value;
}

void LC_ActionModifyLineJoin::setLine1EdgeMode(int value){
    line1EdgeMode = value;
}

void LC_ActionModifyLineJoin::setLine2EdgeMode(int value){
    line2EdgeMode = value;
}
