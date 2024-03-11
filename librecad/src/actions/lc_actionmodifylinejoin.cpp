#include "lc_actionmodifylinejoin.h"
#include "rs_dialogfactory.h"
#include "rs_information.h"
#include "lc_linejoinoptions.h"
#include "lc_actiondrawlineanglerel.h"
#include <QMouseEvent>
#include <rs_coordinateevent.h>
#include <rs_preview.h>
#include <rs_document.h>
#include <rs_polyline.h>
#include <rs_graphicview.h>
#include <rs_commandevent.h>
#include <cmath>

LC_ActionModifyLineJoin::LC_ActionModifyLineJoin(RS_EntityContainer &container, RS_GraphicView &graphicView):
    RS_PreviewActionInterface("ModifyLineJoin", container, graphicView),
    line1(nullptr), line2(nullptr){
    actionType = RS2::ActionModifyLineJoin;
}
LC_ActionModifyLineJoin::~LC_ActionModifyLineJoin() = default;

QStringList LC_ActionModifyLineJoin::getAvailableCommands(){
    return RS_ActionInterface::getAvailableCommands();
}

void LC_ActionModifyLineJoin::init(int status){
    RS_PreviewActionInterface::init(status);
    highlightedLine = nullptr;
    line1 = nullptr;
    line2 = nullptr;
}

void LC_ActionModifyLineJoin::updateMouseButtonHints(){
    switch (getStatus()) {
    case SetLine1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select first line"),
                                            tr("Back"));
        break;
    case SetLine2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select second line"),
                                            tr("Back"));
        break;
    case ResolveFirstLineTrim:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select part of first line that should remain after trim"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void LC_ActionModifyLineJoin::coordinateEvent(RS_CoordinateEvent *e){
    RS_ActionInterface::coordinateEvent(e);
}

void LC_ActionModifyLineJoin::commandEvent(RS_CommandEvent *e){
    RS_ActionInterface::commandEvent(e);
}

#define LINE1_UPDATE_BY_TRIM_ON_PREVIEW false

void LC_ActionModifyLineJoin::mouseMoveEvent(QMouseEvent *e){
    RS_Entity *en = catchEntity(e, lineType, RS2::ResolveAll);
    RS_Line* snappedLine = nullptr;
    if (en && en->rtti() == RS2::EntityLine){
        snappedLine = dynamic_cast<RS_Line *>(en);
    }
    switch (getStatus()) {
    case SetLine1: {
        if (snappedLine != nullptr){ // can snap to line
            if (highlightedLine != nullptr){
                highlightedLine->setHighlighted(false);
                graphicView->drawEntity(en);
            }
            highlightedLine = snappedLine;
            highlightedLine->setHighlighted(true);
            graphicView->drawEntity(en);
            graphicView->redraw(RS2::RedrawDrawing);

        } else { // no entity to snap
            deletePreview();
            if (highlightedLine != nullptr){
                highlightedLine->setHighlighted(false);
                graphicView->drawEntity(en);
            }
            graphicView->redraw(RS2::RedrawDrawing);
            highlightedLine = nullptr;
        }
        break;
    }
    case SetLine2: {
        if (snappedLine == line1){
            snappedLine = nullptr;
        }
        if (snappedLine != nullptr) {
            if(highlightedLine != nullptr){
                highlightedLine->setHighlighted(false);
                graphicView->drawEntity(en);
            }
            line1->setHighlighted(false);
            graphicView->drawEntity(line1);

            deletePreview();

            RS_Vector snap =  graphicView->toGraph(e->x(), e->y());
            LC_LineJoinData* lineJoinData = createLineJoinData(snappedLine, snap);

            RS_Polyline* polyline = lineJoinData->polyline;
            if (polyline != nullptr){
                preview->addEntity(polyline);
                drawPreview();
            }
            delete lineJoinData;
            graphicView->redraw(); // fixme - check mode
        }
        else{
            deletePreview();
            if(highlightedLine != nullptr){
                highlightedLine->setHighlighted(false);
                graphicView->drawEntity(en);
            }
            line1->setHighlighted(true);
            graphicView->drawEntity(line1);
            graphicView->redraw();// fixme - check mode
        }
        break;
    }
    case ResolveFirstLineTrim:
        if (snappedLine != line1){
            snappedLine = nullptr;
        }
        deletePreview();
        if (snappedLine != nullptr) {
            RS_Vector snap = graphicView->toGraph(e->x(), e->y());
            updateLine1TrimData(snap);
            line1->setHighlighted(false);

            RS_Polyline* polyline = linesJoinData->polyline;
            if (polyline != nullptr){
                preview->addEntity(polyline->clone());
                drawPreview();
            }
            // potentially, on preview we can adjust line 1 according to trim point. Yet this affects drawing any may be not safe until trigger
            if (LINE1_UPDATE_BY_TRIM_ON_PREVIEW){
                RS_Vector newStart = getTrimStartPoint(linesJoinData->line1Disposition, linesJoinData->intersectPoint);
                RS_Vector newEnd = getTrimEndPoint(linesJoinData->line1Disposition, linesJoinData->intersectPoint);
                line1->setStartpoint(newStart);
                line1->setEndpoint(newEnd);
            }
        }
        else{
            if (LINE1_UPDATE_BY_TRIM_ON_PREVIEW){
                line1->setStartpoint(linesJoinData->line1Disposition.startPoint);
                line1->setEndpoint(linesJoinData->line1Disposition.endPoint);
            }
            line1->setHighlighted(true);
        }
        graphicView->drawEntity(line1);

        graphicView->redraw(); // fixme - check mode
        break;
    }
}

void LC_ActionModifyLineJoin::mouseReleaseEvent(QMouseEvent *e){
    if (e->button()==Qt::LeftButton) {
        RS_Entity *en = catchEntity(e, lineType, RS2::ResolveAll);
        RS_Line* snappedLine = nullptr;
        if (en && en->rtti() == RS2::EntityLine){
            snappedLine = dynamic_cast<RS_Line *>(en);
        }
        switch (getStatus()){
        case SetLine1:
            if (snappedLine != nullptr){
                line1 = snappedLine;
                highlightedLine->setHighlighted(false);
                graphicView->drawEntity(highlightedLine);
                highlightedLine = nullptr;
                line1->setHighlighted(true);
                graphicView->drawEntity(line1);
                graphicView->redraw(); // fixme - check mode
                setStatus(SetLine2);
            }
            else{
                RS_DIALOGFACTORY->commandMessage(tr("No line selected"));
            }
            break;
        case SetLine2:
            if (snappedLine != nullptr){
                line2 = snappedLine;
                RS_Vector snap = graphicView->toGraph(e->x(), e->y());
                LC_LineJoinData* joinData = createLineJoinData(snappedLine, snap);
                if (joinData->parallelLines){
                    if (joinData->straightLinesConnection){
                        linesJoinData = joinData;
                        doTrigger();
                    }
                    else{
                        RS_DIALOGFACTORY->commandMessage(tr("Lines are parallel, can't merge"));
                    }
                }
                else{ // lines merge with angle
                    if (joinData->areLinesAlreadyIntersected()){
                        // both lines are already crossed, do nothing
                        RS_DIALOGFACTORY->commandMessage(tr("Lines already intersects, can't merge"));
                    } else {
                        bool firstLineTrimShouldBeSpecified = joinData->isIntersectionOnLine1();
                        if (line1EdgeMode != EDGE_EXTEND_TRIM){
                            firstLineTrimShouldBeSpecified = false;
                        }
                        linesJoinData = joinData;
                        if (firstLineTrimShouldBeSpecified){
                            setStatus(ResolveFirstLineTrim);
                            line1->setHighlighted(true);
                            graphicView->drawEntity(line1);
                            line2->setHighlighted(false);
                            graphicView->drawEntity(line2);
                            graphicView->redraw();

                        } else {
                            doTrigger();
                        }
                    }
                }
            }
            else{
                RS_DIALOGFACTORY->commandMessage(tr("No line selected"));
            }
            break;

        case ResolveFirstLineTrim:
            // fixme - complete

            break;
        }

    } else if (e->button()==Qt::RightButton) {
        processRightButtonClick();
    }
}

void LC_ActionModifyLineJoin::processRightButtonClick(){
    deletePreview();
    switch (getStatus()){
    case SetLine1:
        if (line1 != nullptr){
            line1->setHighlighted(false);
            highlightedLine = nullptr;
            graphicView->drawEntity(line1);
            graphicView->redraw(RS2::RedrawOverlay);
        }
        init(SetLine1 - 1);
        break;
    case SetLine2: {
        if (line2 != nullptr){
            line2->setHighlighted(false);
            graphicView->drawEntity(line2);
        }
        if (line1 != nullptr){
            line1->setHighlighted(false);
            graphicView->drawEntity(line1);
        }
        highlightedLine = nullptr;
        graphicView->redraw(RS2::RedrawDrawing);
        setStatus(SetLine1);
        break;
    }
    case ResolveFirstLineTrim:{
        if (LINE1_UPDATE_BY_TRIM_ON_PREVIEW){
            line1->setStartpoint(linesJoinData->line1Disposition.startPoint);
            line1->setEndpoint(linesJoinData->line1Disposition.endPoint);
        }
        if (line2 != nullptr){
            line2->setHighlighted(false);
            graphicView->drawEntity(line2);
        }
        highlightedLine = nullptr;
        graphicView->redraw(RS2::RedrawDrawing);
        setStatus(SetLine2);
        break;
    }
    default:
        init(getStatus() - 1);
    }
}

void LC_ActionModifyLineJoin::doTrigger(){
    deletePreview();
    line1->setHighlighted(false);
    graphicView->drawEntity(line1);
    line2->setHighlighted(false);
    graphicView->drawEntity(line2);
    trigger();

    delete linesJoinData;

    graphicView->redraw();
    init(SetLine1);
}

void LC_ActionModifyLineJoin::updateMouseCursor(){
    graphicView->setMouseCursor(RS2::SelectCursor);
}

void LC_ActionModifyLineJoin::trigger(){
    RS_PreviewActionInterface::trigger();
    if (document){

        document->startUndoCycle();
        if (linesJoinData->parallelLines && linesJoinData->straightLinesConnection){
            RS_Line *line = new RS_Line(container, linesJoinData->majorPointLine1, linesJoinData->majorPointLine2);
            applyAttributes(line, true);
            container->addEntity(line);
            document->addUndoable(line);

        } else {
            RS_Vector &intersectionPoint = linesJoinData->intersectPoint;
            RS_Vector &major1 = linesJoinData->majorPointLine1;
            RS_Vector &major2 = linesJoinData->majorPointLine2;
            RS_Line *l1;
            RS_Line *l2;

            if (createPolyline && major1.valid && major2.valid){
                RS_Polyline *poly = new RS_Polyline(container);
                poly->addVertex(major1);
                poly->addVertex(intersectionPoint);
                poly->addVertex(major2);
                applyAttributes(poly, true);
                container->addEntity(poly);
                document->addUndoable(poly);

            } else {
                if (major1.valid){
                    l1 = new RS_Line(container, major1, intersectionPoint);
                    applyAttributes(l1, true);
                    container->addEntity(l1);
                    document->addUndoable(l1);
                }
                if (major2.valid){
                    l2 = new RS_Line(container, intersectionPoint, major2);
                    applyAttributes(l2, false);
                    container->addEntity(l2);
                    document->addUndoable(l2);
                }
            }
        }
        if (removeOriginalLines){
            deleteOriginalEntity(line1);
            deleteOriginalEntity(line2);
        }
        document->endUndoCycle();
        graphicView->redraw(RS2::RedrawDrawing);
    }
}

void LC_ActionModifyLineJoin::deleteOriginalEntity(RS_Entity *entity){
    // delete and add this into undo
    graphicView->deleteEntity(entity);
    entity->changeUndoState();
    document->addUndoable(entity);
}

void LC_ActionModifyLineJoin::applyAttributes(RS_Entity *e, bool forLine1){
    RS_Pen pen;
    RS_Layer *layer;
    switch (attributesSource) {
    case ATTRIBUTES_LINE_1: {
        pen = line1->getPen(false);
        e->setPen(pen);
        layer = line1->getLayer(true);
        e->setLayer(layer);
        break;
    }
    case ATTRIBUTES_BOTH_LINES:
        if (forLine1){
            pen = line1->getPen(false);
            e->setPen(pen);
            layer = line1->getLayer(true);
            e->setLayer(layer);
        } else {
            pen = line2->getPen(false);
            e->setPen(pen);
            layer = line2->getLayer(true);
            e->setLayer(layer);
        }
        break;
    case ATTRIBUTES_LINE_2:
        pen = line2->getPen(false);
        e->setPen(pen);
        layer = line2->getLayer(true);
        e->setLayer(layer);
        break;
    case ATTRIBUTES_ACTIVE_PEN_LAYER:
        e->setPenToActive();
        e->setLayerToActive();
        break;
    }
}

LC_ActionModifyLineJoin::LC_LineJoinData* LC_ActionModifyLineJoin::createLineJoinData(RS_Line *secondLine, RS_Vector &snapPoint){
    LC_LineJoinData* result = nullptr;

    if (line1 != nullptr && secondLine != nullptr){
        RS_VectorSolutions sol = RS_Information::getIntersectionLineLine(line1, secondLine);
        const RS_Vector &line1Start = line1->getStartpoint();
        const RS_Vector &line1End = line1->getEndpoint();
        const RS_Vector &line2Start = secondLine->getStartpoint();
        const RS_Vector &line2End = secondLine->getEndpoint();

        if (sol.empty()){ // parallel lines, needs special handling
            result = proceedParallelLinesJoin(line1Start, line1End, line2Start, line2End);

        } else {
            result = proceedNonParallelLines(snapPoint, sol, line1Start, line1End, line2Start, line2End);
        }
    }
    return result;
}

LC_ActionModifyLineJoin::LC_LineJoinData* LC_ActionModifyLineJoin::proceedNonParallelLines(RS_Vector &snapPoint,
                                                                                           const RS_VectorSolutions &sol,
                                                                                           const RS_Vector &line1Start, const RS_Vector &line1End,
                                                                                           const RS_Vector &line2Start, const RS_Vector &line2End){
    LC_LineJoinData* result = new LC_LineJoinData();
    RS_Vector intersectPoint = sol.get(0);
    result->intersectPoint = intersectPoint;
    result->parallelLines = false;
    RS_Polyline *polyline = new RS_Polyline(container);

    // processing of line 1
    LC_PointsDisposition line1Disposition = determine3PointsDisposition(line1Start, line1End, intersectPoint, snapPoint);
    RS_Vector pointFromLine1 = getMajorPointFromLine(line1EdgeMode, line1Start, line1End, line1Disposition);
    result->majorPointLine1 = pointFromLine1;
    if (pointFromLine1.valid){
        polyline->addVertex(pointFromLine1);
    }

    // add intersection
    polyline->addVertex(intersectPoint);

    // processing of line 2

    LC_PointsDisposition line2Disposition = determine3PointsDisposition(line2Start, line2End, intersectPoint, snapPoint);
    RS_Vector pointFromLine2 = getMajorPointFromLine(line2EdgeMode, line2Start, line2End, line2Disposition);
    result->majorPointLine2 = pointFromLine2;
    if (pointFromLine2.valid){
        polyline->addVertex(pointFromLine2);
    }

    result->polyline = polyline;
    result->line1Disposition = line1Disposition;
    result->line2Disposition = line2Disposition;
    result->intersectPoint = intersectPoint;
    return result;
}

void LC_ActionModifyLineJoin::updateLine1TrimData(RS_Vector snap){

    const RS_Vector &line1Start = line1->getStartpoint();
    const RS_Vector &line1End = line1->getEndpoint();

    RS_Polyline *polyline = linesJoinData->polyline;
    if (polyline != nullptr) {
        delete polyline;
    }
    polyline = new RS_Polyline(container);
    linesJoinData->polyline = polyline;

    RS_Vector &intersection = linesJoinData->intersectPoint;
    LC_ActionModifyLineJoin::LC_PointsDisposition line1Disposition = determine3PointsDisposition(line1Start, line1End, intersection, snap);

    RS_Vector pointFromLine1 = getMajorPointFromLine(line1EdgeMode, line1Start, line1End, line1Disposition);
    linesJoinData->majorPointLine1 = pointFromLine1;

    if (line1Start.valid){
        polyline->addVertex(pointFromLine1);
    }

    polyline->addVertex(intersection);

    RS_Vector pointFromLine2 = getMajorPointFromLine(line2EdgeMode, line2->getStartpoint(), line2->getEndpoint(), linesJoinData->line2Disposition);
    linesJoinData->majorPointLine2 = pointFromLine2;

    if (pointFromLine2.valid){
        polyline->addVertex(pointFromLine2);
    }
}

RS_Vector LC_ActionModifyLineJoin::getMajorPointFromLine(const int edgeMode,
                                                         const RS_Vector &lineStart,
                                                         const RS_Vector &lineEnd,
                                                         const LC_ActionModifyLineJoin::LC_PointsDisposition &lineDisposition) const{
    RS_Vector result = RS_Vector(false);
    switch (edgeMode){

    case EDGE_EXTEND_TRIM:
        switch (lineDisposition.dispositionMode) {
        case LC_PointsDisposition::BOTH_POINTS_ON_RIGHT:
        case LC_PointsDisposition::BOTH_POINTS_ON_LEFT: {
            result= lineDisposition.farPoint;
            break;
        }
        case LC_PointsDisposition::MIDDLE_END_LEFT:{
            if (lineDisposition.snapSelectionOnLeft){
                result = lineEnd;
            }
            else {
                result = lineStart;
            }
            break;
        }
        case LC_PointsDisposition::MIDDLE_START_LEFT: {
            if (lineDisposition.snapSelectionOnLeft){
                result = lineStart;
            }
            else {
                result = lineEnd;
            }
            break;
        }
        }
        break;
    case EDGE_ADD_SEGMENT:
        switch (lineDisposition.dispositionMode)
        {
        case LC_PointsDisposition::BOTH_POINTS_ON_RIGHT:
        case LC_PointsDisposition::BOTH_POINTS_ON_LEFT: {
            result = lineDisposition.closePoint;
            break;
        }
        case LC_PointsDisposition::MIDDLE_END_LEFT:{
            //                    if (lineDisposition.snapSelectionOnLeft){
            //                        result = lineEnd;
            //                    }
            //                    else {
            //                        result = lineStart;
            //                    }
            break;
        }
        case LC_PointsDisposition::MIDDLE_START_LEFT: {
            //                    if (lineDisposition.snapSelectionOnLeft){
            //                        result = lineStart;
            //                    }
            //                    else {
            //                        result = lineEnd;
            //                    }
            break;
        }
        }

        break;
    case EDGE_NO_MODIFICATION:
        break;
    }
    return result;
}

LC_ActionModifyLineJoin::LC_LineJoinData* LC_ActionModifyLineJoin::proceedParallelLinesJoin(const RS_Vector &line1Start, const RS_Vector &line1End,
                                                                                            const RS_Vector &line2Start, const RS_Vector &line2End) const{

    LC_LineJoinData * result = new LC_LineJoinData();
    result->parallelLines = true;
    // check whether these lines are on the same vector
    bool sameRay = areLinesOnSameRay(line1Start, line1End, line2Start, line2End);

    if (sameRay){
        result->straightLinesConnection = true;

        double angle = line1Start.angleTo(line1End);
        RS_Vector startOnX1 = RS_Vector(line1Start);
        RS_Vector endOnX1 = RS_Vector(line1End);

        RS_Vector startOnX2 = RS_Vector(line2Start);
        RS_Vector endOnX2 = RS_Vector(line2End);

        // rotate all points around start line 1 do the will be on horizontal line
        endOnX1.rotate(startOnX1, -angle);
        startOnX2.rotate(startOnX1, -angle);
        endOnX2.rotate(startOnX1, -angle);

        double sx1 = startOnX1.x;
        double ex1 = endOnX1.x;
        double sx2 = startOnX2.x;
        double ex2 = endOnX2.x;

        // orient all lines in one direction, from left to right

        RS_Vector s1 = line1Start;
        RS_Vector e1 = line2End;
        RS_Vector s2 = line2Start;
        RS_Vector e2 = line2End;

        RS_Vector tmp;

        if (ex1 < sx1){
            std::swap(sx1, ex1);
            std::swap(s1, e1);
        }

        if (ex2 < sx2){
            std::swap(sx2, ex2);
            std::swap(s2, e2);
        }

        RS_Vector leftPoint;
        RS_Vector rightPoint;
        RS_Vector middleLeftPoint;
        RS_Vector middleRightPoint;

        // check we have intersection of two lines
        bool hasIntersection = false;

        if (sx1 < sx2){
            hasIntersection = sx2 < ex1; // check whether start of line 2 between start and end of line 1
            leftPoint = s1;
            if (hasIntersection){
                if (ex1 > ex2){
                    rightPoint = e1;
                }
                else{
                    rightPoint = e2;
                }
            }
            else{
                rightPoint = e2;
                middleLeftPoint = e1;
                middleRightPoint = s2;
            }
        }
        else {
            hasIntersection = sx1 < ex2; // check whether start of line 2 between start and end of line 1
            leftPoint = s2;
            if (hasIntersection){
                if (ex1 > ex2){
                    rightPoint = e1;
                }
                else{
                    rightPoint = e2;
                }
            }
            else{
                rightPoint = e1;
                middleRightPoint = s1;
                middleLeftPoint = e2;
            }
        }

        if (line1EdgeMode == EDGE_EXTEND_TRIM || line2EdgeMode == EDGE_EXTEND_TRIM){
            RS_Polyline *polyline = new RS_Polyline(container);
            polyline->addVertex(leftPoint);
            polyline->addVertex(rightPoint);
            result->majorPointLine1 = leftPoint;
            result->majorPointLine2 = rightPoint;
            result->polyline = polyline;
        }
        else if (line1EdgeMode == EDGE_ADD_SEGMENT && line2EdgeMode == EDGE_ADD_SEGMENT){
            if (!hasIntersection){
                RS_Polyline *polyline = new RS_Polyline(container);
                polyline->addVertex(middleLeftPoint);
                polyline->addVertex(middleRightPoint);
                result->majorPointLine1 = middleLeftPoint;
                result->majorPointLine2 = middleRightPoint;
                result->polyline = polyline;
            }
        }
    }
    else{
        result->straightLinesConnection = false;
    }
    return result;
}

bool LC_ActionModifyLineJoin::areLinesOnSameRay(
        const RS_Vector &line1Start, const RS_Vector &line1End, const RS_Vector &line2Start, const RS_Vector &line2End) const{
    double angle1 = line1Start.angleTo(line1End);
    if (std::abs(angle1 - M_PI) < RS_TOLERANCE_ANGLE){
        angle1 = angle1 - M_PI;
    }
    double angle2 = line1Start.angleTo(line2End);
    if (std::abs(angle2 - M_PI) < RS_TOLERANCE_ANGLE){
        angle2 = angle2 - M_PI;
    }

    double angle3 = line1Start.angleTo(line2Start);
    if (std::abs(angle3 - M_PI) < RS_TOLERANCE_ANGLE){
        angle3 = angle3 - M_PI;
    }

    bool sameLine = false;
    if (std::abs(angle1 - angle2) < RS_TOLERANCE_ANGLE && std::abs(angle1 - angle3) < RS_TOLERANCE_ANGLE){
        sameLine = true;
    }
    return sameLine;
}


LC_ActionModifyLineJoin::LC_PointsDisposition LC_ActionModifyLineJoin::determine3PointsDisposition(const RS_Vector start,
                                                                                                   const RS_Vector end,
                                                                                                   const RS_Vector intersection,
                                                                                                   const RS_Vector &snapPoint) const{

    // align all 3 points horizontally, rotate them as needed

    double angle = start.angleTo(end);
    RS_Vector startOnX = RS_Vector(start);
    RS_Vector endOnX = RS_Vector(end);

    startOnX.rotate(intersection, -angle);
    endOnX.rotate(intersection, -angle);

    RS_Vector snapProjection = LC_ActionDrawLineAngleRel::getNearestPointOnInfiniteLine(snapPoint, start, end);
    RS_Vector snapOnX = snapProjection.rotate(intersection, -angle);


    // move coordinates that intersection point is in 0
    double ix = intersection.x;
    double delta = ix;

    if (ix < 0){
        delta = -ix;
    }

    double startX = startOnX.x;
    double endX = endOnX.x;
    double snapX = snapOnX.x;

    startX -= delta;
    endX -= delta;
    ix -= delta;
    snapX -= delta;

    LC_PointsDisposition result;
    if (startX > 0){
        if (endX > 0){
            result.dispositionMode = LC_PointsDisposition::BOTH_POINTS_ON_RIGHT;
        } else {
            result.dispositionMode = LC_PointsDisposition::MIDDLE_END_LEFT;
        }
    } else {
        if (endX > 0){
            result.dispositionMode = LC_PointsDisposition::MIDDLE_START_LEFT;
        } else {
            result.dispositionMode = LC_PointsDisposition::BOTH_POINTS_ON_RIGHT;
        }
    }

    if (std::abs(endX) > std::abs(startX)){
        result.closePoint = start;
        result.farPoint = end;
        result.isStartPointClosest = true;
    } else {
        result.closePoint = end;
        result.farPoint = start;
        result.isStartPointClosest = false;
    }

    // determine position of snap point relating to interseption point, if any
    result.snapSelectionOnLeft = snapX < 0;
    result.startPoint = start;
    result.endPoint = end;
    return result;
}



void LC_ActionModifyLineJoin::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_LineJoinOptions>(nullptr);
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

RS_Vector LC_ActionModifyLineJoin::getTrimStartPoint(LC_ActionModifyLineJoin::LC_PointsDisposition disposition, RS_Vector & intersectionPoint){
    if (disposition.dispositionMode == LC_PointsDisposition::MIDDLE_START_LEFT){
        if (disposition.snapSelectionOnLeft){
            return disposition.startPoint;
        }
        else{
            return intersectionPoint;
        }
    }
    else {
        if (disposition.snapSelectionOnLeft){
            return intersectionPoint;
        }
        else {
            return disposition.startPoint;
        }
    }
}

RS_Vector LC_ActionModifyLineJoin::getTrimEndPoint(LC_ActionModifyLineJoin::LC_PointsDisposition disposition, RS_Vector & intersectionPoint){
    if (disposition.dispositionMode == LC_PointsDisposition::MIDDLE_END_LEFT){
        if (disposition.snapSelectionOnLeft){
            return disposition.endPoint;
        }
        else{
            return intersectionPoint;
        }
    }
    else {
        if (disposition.snapSelectionOnLeft){
            return intersectionPoint;
        }
        else {
            return disposition.endPoint;
        }
    }
}

void LC_ActionModifyLineJoin::setAttributesSource(int value){
    attributesSource = value;
}





