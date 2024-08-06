/****************************************************************************
**
* Abstract base class for actions that draws a rectangle

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

#include <cmath>

#include "lc_abstractactiondrawrectangle.h"
#include "lc_linemath.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_polyline.h"


LC_AbstractActionDrawRectangle::LC_AbstractActionDrawRectangle(
    const char *name,
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview(name, container, graphicView){
    mainStatus = SetPoint1;
}

LC_AbstractActionDrawRectangle::~LC_AbstractActionDrawRectangle() = default;

/**
 * Creates shape data by creation of polyline for snap point and storing snap point
 * @param snapPoint point of snap
 */
void LC_AbstractActionDrawRectangle::createShapeData(const RS_Vector &snapPoint){
    shapeData = new ShapeData();
    RS_Polyline* polyline = createPolyline(snapPoint);
    shapeData->resultingPolyline = polyline;
    shapeData->snapPoint = snapPoint;
}

/**
 * Extract polyline from shape data and add it (or it's entities) to the resulting list of entities
 * @param list created entities list for trigger
 */
void LC_AbstractActionDrawRectangle::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    RS_Polyline *polyline = shapeData->resultingPolyline;
    // extract entities from polyline and insert them as result of action
    doAddPolylineToListOfEntities(polyline, list, false);
}

/**
 * Base on options, method either just adds given polyline to the list of entities that are created on trigger,
 * or extracts individual entities (lines and arcs) from polyline and insert them to the list individually.
 *
 * If corners draw mode is CORNER_STRAIGHT (so no rounding or bevels), appropriate edges are inspected and only
 * those who matches to the edges draw mode are inserted to the list.
 *
 * @param polyline
 * @param list
 * @param preview
 */
void LC_AbstractActionDrawRectangle::doAddPolylineToListOfEntities(RS_Polyline *polyline, QList<RS_Entity *> &list, bool preview){

    // we should additionally filter edge lines if we draw straight rectangle and not all edges should be drawn
    bool shouldInspectForNonCompleteShape = edgesDrawMode != EDGES_BOTH && cornersDrawMode == CORNER_STRAIGHT; // here we draw only side edges

    // flag that defines whether we'll insert polyline or individual elements
    bool addAtOnce;
    if (preview){
        addAtOnce = !shouldInspectForNonCompleteShape;
    }
    else{
        if (shouldInspectForNonCompleteShape){
            addAtOnce = false; // can't use polyline as we'll draw partial edges
        }
        else{
            addAtOnce = usePolyline; // rely on polyline settings
        }
    }
    if (addAtOnce){
        // just insert created polyline into drawing or preview
        list<<polyline;
    }
    else {
        // iterate over entities of polyline
        int index = -1;
        for (RS_Entity *entity = polyline->firstEntity(RS2::ResolveAll); entity;
            entity = polyline->nextEntity(RS2::ResolveAll)) {
            index++;
            if (entity != nullptr){
                if (shouldInspectForNonCompleteShape){
                    // check whether this edge should be added according to edges mode
                    bool sideEdgeLine = doCheckPolylineEntityAllowedInTrigger(index);
                    if (!sideEdgeLine){
                        continue; // skip this entity and go to next
                    }
                }
                // create clone of entity for safe deletion of original polyline
                RS_Entity *clone = entity->clone();
                clone->reparent(container);
                list << clone;
            }
        }
        delete polyline; //don't need it anymore
    }
}

/**
 * Performs filtering of rectangle edges according to edges draw node
 * @param index  index of edge in polyline
 * @return true if edge may be added to result, false if it should be skipped
 */
bool LC_AbstractActionDrawRectangle::doCheckPolylineEntityAllowedInTrigger(int index) const{
    if (edgesDrawMode == EDGES_VERT)
        return (index == 1) || (index == 3);
    else if (edgesDrawMode == EDGES_HOR){
        return (index == 0) || (index == 2);
    }
    else
        return true;
}

/**
 * Cleanup after trigger operation execution
 */
void LC_AbstractActionDrawRectangle::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    if (shapeData != nullptr){
        delete shapeData;
        shapeData = nullptr;
    }
    graphicView->redraw();
}

/**
 * After trigger, by default we'll set relative zero to last snap point (to which trigger was invoked)
 * @return
 */
RS_Vector LC_AbstractActionDrawRectangle::doGetRelativeZeroAfterTrigger(){
    return shapeData -> snapPoint;
}

/**
 * We may trigger if there is shape data
 * @return
 */
bool LC_AbstractActionDrawRectangle::doCheckMayTrigger(){
    return shapeData != nullptr;
}

/**
 * Creating preview entities. Calculates and creates polyline, and add it (or specific edges) to the list of preview entities.
 * @param e original event
 * @param snap snap point
 * @param list list of entities for preview
 * @param status current status of action
 */
void LC_AbstractActionDrawRectangle::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    RS_Polyline *polyline = createPolyline(snap);
    // todo - is it really necessary to set attributes there?
    polyline->setLayerToActive();
    polyline->setPenToActive();
    doAddPolylineToListOfEntities(polyline, list, true);
}

/**
 * process generic coordinates common for various actions and delegates processing to
 * inherited actions
 * @param coordinate event
 */


void LC_AbstractActionDrawRectangle::onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetBevels:
            // actually, we'll allow zero values there - together with drawing as individual lines, that may
            // potentially bring interesting effects - with separation of edge lines on several segments
            bevelX = LC_LineMath::getMeaningful(coord.x);
            bevelY = LC_LineMath::getMeaningful(coord.y);
            updateOptions();
            restoreMainStatus();
            break;
        case SetAngle: // zero angle value processing
            if (isZero){
                baseAngleIsFixed = false;
                drawPreviewForLastPoint();
                updateOptions();
                restoreMainStatus();
            } else {
                commandMessage(tr("Invalid Angle"));
                updateMouseButtonHints();
            }
            break;
        case SetRadius: // zero radius value processing
            if (isZero){
                radius = 0.0;
                updateOptions();
                restoreMainStatus();
            }
            else{
                commandMessage(tr("Invalid radius"));
                updateMouseButtonHints();
            }
            break;
        default:
            // delegate processing to inherited actions
            doProcessCoordinateEvent(coord, isZero, status); // do processing of other vars
            break;
    }
}

/**
 * Extension point for inherited actions for processing coordinate events
 * @param coord coordinates
 * @param isZero is zero coordinate
 * @param status current state of action
 */
void LC_AbstractActionDrawRectangle::doProcessCoordinateEvent([[maybe_unused]]const RS_Vector &coord,[[maybe_unused]] bool isZero, [[maybe_unused]]int status){}

/**
 * returns base angle in radians if base angle is fixed, or 0 otherwise.
 * @return
 */
double LC_AbstractActionDrawRectangle::getActualBaseAngle() const{
    double result = 0.0;
    if (baseAngleIsFixed){
        result = RS_Math::deg2rad(angle);
    }
    return result;
}


/**
 * Processing of common commands
 * @param e event
 * @param c command
 * @return true if command is processed, false if additional processing is needed
 */
bool LC_AbstractActionDrawRectangle::doProcessCommand(int status, const QString &c){
    bool processed = true;
    bool toMainStatus = true;

    if (checkCommand("angle",c)){ // initiates entering of base angle of rect (angle from corner1 to corner2)
        setStatus(SetAngle);
        toMainStatus = false;
    }
    else if (checkCommand("radius",c)){ // initiates entering rounding radius for corners
        setStatus(SetRadius);
        toMainStatus = false;
    }
    else if (checkCommand("bevels",c)){ // initiates entering of bevels or setting bevels corners mode
        if (status == SetCorners){
            cornersDrawMode = CORNER_BEVEL;
            updateOptions();
        }
        else {
            setStatus(SetBevels);
            toMainStatus = false;
        }
    }
    else if (checkCommand("nopoly",c)){  // disables drawing rect as polyline (so all elements are individual entities)
        usePolyline = false;
        updateOptions();
    }
    else if (checkCommand("usepoly",c)){ // enables drawing rect as polyline
        usePolyline = true;
        updateOptions();
    }
    else if (checkCommand("corners",c)){ // initiates entering corners mode
        setStatus(SetCorners);
        toMainStatus = false;
    }
    else if (checkCommand("str",c)){  // straight corners mode (no rounding, not bevel)
        if (status == SetCorners){
            cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
        }
    }
    else if (checkCommand("round",c)){ // rounded corners mode
        if (status == SetCorners){
            cornersDrawMode = CORNER_RADIUS;
            updateOptions();
        }
    }
    else if (checkCommand("edges", c)){ // initiates entering drawing edges modes for straight rect
        setStatus(SetEdges);
        toMainStatus = false;
    }
    else if (checkCommand("both", c)){ // all edges are drawn
        if (status == SetEdges){
            edgesDrawMode = EDGES_BOTH;
            cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
            restoreMainStatus();
        }
    }
    else if (checkCommand("hor", c)){  // only horizontal edges are drawn
        if (status == SetEdges){
            edgesDrawMode = EDGES_HOR;
            cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
            restoreMainStatus();
        }
    }
    else if (checkCommand("vert", c)){ // only vertical edges are drawn
        if (status == SetEdges){
            edgesDrawMode = EDGES_VERT;
            cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
            restoreMainStatus();
        }
    }
    else if (processCustomCommand(status,c, toMainStatus)){ // delegate processing to inherited class
        // intentionally do nothing
    }
    else{ // process entered value
        bool ok = false;
        double value = RS_Math::eval(c, &ok);
        if (ok){
            switch (status) {
                case SetAngle: {
                    angle = LC_LineMath::getMeaningfulAngle(value);
                    baseAngleIsFixed = true;
                    updateOptions();
                    toMainStatus = true;
                    break;
                }
                case SetRadius: {
                    radius = LC_LineMath::getMeaningful(value);
                    updateOptions();
                    toMainStatus = true;
                    break;
                }
                default:
                    // let inherited action process this
                    processCommandValue(value, toMainStatus);
            }
        }
        else {
            processed = false;
            commandMessage(tr("Invalid value"));
        }
    }
    if (processed){
        stateUpdated(toMainStatus);
    }
    return processed;
}

/**
 * Check current settings for corners and define whether we should draw simple rect or complex shape, and in later case
 * prepare corner-related values
 * @param radiusX  x length of corner's bevel or radius
 * @param radiusY  y length of corner's bevel or radius
 * @param drawComplex should it be complex shape or plain rect (if false)
 * @param drawBulge returns whether rounded corners should be drawn
 */
void LC_AbstractActionDrawRectangle::prepareCornersDrawMode(double &radiusX, double &radiusY, bool &drawComplex, bool &drawBulge) const{
    switch (cornersDrawMode){
        case CORNER_STRAIGHT:{
            drawComplex = false;
            break;
        }
        case CORNER_BEVEL:
        {
            drawComplex = true;
            radiusX = bevelX;
            if (LC_LineMath::isNotMeaningful(radiusX)){
                // just double check of user input - for invalid value, draw just rect
                drawComplex = false;
            }
            radiusY = bevelY;
            if (LC_LineMath::isNotMeaningful(radiusY)){
                // just double check of user input - for invalid value, draw just rect
                drawComplex = false;
            }
            break;
        }
        case CORNER_RADIUS:{
            if (LC_LineMath::isNotMeaningful(radius)){
                drawComplex = false;
            }
            else{
                radiusX = radius;
                radiusY = radius;
                drawBulge = true;
            }
            break;
        }
    }
}

void LC_AbstractActionDrawRectangle::updateMouseButtonHints() {
    int status = getStatus();
    switch (status) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Specify insertion point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetAngle:
            updateMouseWidgetTRBack(tr("Specify angle"));
            break;
        case SetSize:
            updateMouseWidgetTRBack(tr("Specify size (width, height)"));
            break;
        case SetCorners:
            updateMouseWidgetTRBack(tr("Specify corners type\n[str|round|bevels]"));
            break;
        case SetBevels:
            updateMouseWidgetTRBack(tr("Specify corner bevel length (x,y)"));
            break;
        case SetRadius:
            updateMouseWidgetTRBack(tr("Specify corner radius"));
            break;
        case SetEdges:
            updateMouseWidgetTRBack(tr("Specify edges mode\n[both|hor|vert]"));
            break;
        default:
            doUpdateMouseButtonHints(status); // delegate to inherited classes to process additional statuses
            break;
    }
}

/**
 * method for custom prompts in inherited actions
 */
void LC_AbstractActionDrawRectangle::doUpdateMouseButtonHints([[maybe_unused]]int status){
    LC_AbstractActionWithPreview::updateMouseButtonHints();
}

/**
 * Generic method for drawing rect shape with provided corners coordinates and corner values.
 * It is expected that shape is not rotated, so it's angle to x is 0
 *
 * @param bottomLeftCorner
 * @param bottomRightCorner
 * @param topRightCorner
 * @param topLeftCorner
 * @param drawBulge
 * @param drawComplex
 * @param radiusX
 * @param radiusY
 * @return
 */
RS_Polyline *LC_AbstractActionDrawRectangle::createPolylineByVertexes(RS_Vector bottomLeftCorner, RS_Vector bottomRightCorner,
                                                                      RS_Vector topRightCorner, RS_Vector topLeftCorner,
                                                                      bool drawBulge, bool drawComplex, double radiusX, double radiusY) const{
    auto *polyline = new RS_Polyline(container);

    if (drawComplex){ // we'll draw complex shape

        // vector used for corner size
        RS_Vector radiusShiftX = RS_Vector(radiusX, 0);
        RS_Vector radiusShiftY = RS_Vector(0, radiusY);

        // define left line
        RS_Vector leftBottom = bottomLeftCorner + radiusShiftY;
        RS_Vector leftTop = topLeftCorner - radiusShiftY;

        // define right line
        RS_Vector rightBottom = bottomRightCorner + radiusShiftY;
        RS_Vector rightTop = topRightCorner - radiusShiftY;

        // define top line
        RS_Vector topLeft = topLeftCorner + radiusShiftX;
        RS_Vector topRight = topRightCorner - radiusShiftX;

        // define bottom line

        RS_Vector bottomLeft = bottomLeftCorner + radiusShiftX;
        RS_Vector bottomRight = bottomRightCorner - radiusShiftX;

        // prepare bulge for 90 degrees curve, actually it might be just a constant
        double bulge = tan(M_PI / 2 / 4); // normal case

        // start to build the shape starting from bottom left corner, then we go right, up, left and down

        polyline->addVertex(bottomLeft);
        polyline->addVertex(bottomRight);
        if (drawBulge){
            polyline->setNextBulge(bulge);
        }
        polyline->addVertex(rightBottom);
        polyline->addVertex(rightTop);
        if (drawBulge){
            polyline->setNextBulge(bulge);
        }
        polyline->addVertex(topRight);
        polyline->addVertex(topLeft);
        if (drawBulge){
            polyline->setNextBulge(bulge);
        }
        polyline->addVertex(leftTop);
        polyline->addVertex(leftBottom);
        if (drawBulge){
            polyline->setNextBulge(bulge);
        }
        polyline->addVertex(bottomLeft);
        polyline->setClosed(true);

    } else { // here we just draw plain rectangle
        polyline->addVertex(bottomLeftCorner);
        polyline->addVertex(bottomRightCorner);
        polyline->addVertex(topRightCorner);
        polyline->addVertex(topLeftCorner);
        polyline->addVertex(bottomLeftCorner);
    }
    return polyline;
}

/**
 * Method checks provided corners and swap them if needed to ensure that they are properly named - so left corner top corner is indeed left top and so on.
 *
 * It is assumed that all corners are for non-rotated rect, so edges between corners are parallel to coordinates axis
 *
 * @param bottomLeftCorner
 * @param bottomRightCorner
 * @param topRightCorner
 * @param topLeftCorner
 */
void LC_AbstractActionDrawRectangle::normalizeCorners(
    RS_Vector &bottomLeftCorner, RS_Vector &bottomRightCorner, RS_Vector &topRightCorner,
    RS_Vector &topLeftCorner) {
    // normalize rect to ensure that passed corners are in right places in coordinates grid (i.e. - top is over bottom and left is before right)
    if (bottomLeftCorner.x > bottomRightCorner.x){ // check whether left and right top corners are correct or should be swapped
        std::swap(bottomRightCorner, bottomLeftCorner);
        std::swap(topRightCorner, topLeftCorner);
    }

    if (topLeftCorner.y < bottomLeftCorner.y){ // check whether bottom corners are correct
        std::swap(topLeftCorner, bottomLeftCorner);
        std::swap(topRightCorner, bottomRightCorner);
    }
}

/**
 * Setter for corners radius
 * @param value
 */
void LC_AbstractActionDrawRectangle::setRadius(double value){
    radius = value;
    drawPreviewForLastPoint();
}

/**
 * setter for bevel x length
 * @param value
 */
void LC_AbstractActionDrawRectangle::setLengthX(double value){
    bevelX = value;
    drawPreviewForLastPoint();
}

/**
 * setter for bevel y length
 * @param value
 */
void LC_AbstractActionDrawRectangle::setLengthY(double value){
    bevelY = value;
    drawPreviewForLastPoint();
}

/**
 * setter for corners draw mode
 * @param value
 */
void LC_AbstractActionDrawRectangle::setCornersMode(int value){
    cornersDrawMode = value;
    drawPreviewForLastPoint();
}

/**
 * Setter for base angle
 * @param value
 */
void LC_AbstractActionDrawRectangle::setAngle(double value){
    angle = value;
    setBaseAngleFixed(true);
    drawPreviewForLastPoint();
}

/**
 * Setter for snap mode used for insertion
 * @param value
 */
void LC_AbstractActionDrawRectangle::setInsertionPointSnapMode(int value){
    insertionPointSnapMode = value;
    drawPreviewForLastPoint();
}

/**
 * If true, snap point of rectangle will be center of rounding arc instead of corner point
 * @param value
 */
void LC_AbstractActionDrawRectangle::setSnapToCornerArcCenter(bool value){
    snapToCornerArcCenter = value;
    drawPreviewForLastPoint();
}

/**
 * utility method called as soon as state updated
 * @param toMainStatus
 */
void LC_AbstractActionDrawRectangle::stateUpdated(bool toMainStatus){
    updateOptions();
    if (toMainStatus){
        restoreMainStatus();
    }
    drawPreviewForLastPoint();
}

/**
 * Default implementation of back processing
 * @param pEvent
 * @param status
 */
void LC_AbstractActionDrawRectangle::doBack([[maybe_unused]]QMouseEvent *pEvent, int status){
    switch (status){
        case (SetPoint1):{
            finishAction();
            break;
        }
        default:
            init(SetPoint1);
            mainStatus = SetPoint1;
            break;
    }
}
