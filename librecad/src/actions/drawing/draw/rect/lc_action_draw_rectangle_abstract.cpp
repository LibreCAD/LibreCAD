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
#include "lc_action_draw_rectangle_abstract.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_containertraverser.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_linemath.h"
#include "rs_document.h"
#include "rs_polyline.h"

LC_ActionDrawRectangleAbstract::LC_ActionDrawRectangleAbstract(const char *name,LC_ActionContext *actionContext, const RS2::ActionType actionType)
    : LC_AbstractActionWithPreview(name, actionContext, actionType), m_shapeData() {
    m_mainStatus = SetPoint1;
}

LC_ActionDrawRectangleAbstract::~LC_ActionDrawRectangleAbstract() = default;

void LC_ActionDrawRectangleAbstract::doSaveOptions() {
    save("Angle", m_ucsBasisBaseAngleRad);
    save("Corners", m_cornersDrawMode);
    save("Radius", m_radius);
    save("LengthX", m_bevelX);
    save("LengthY", m_bevelY);
    save("Polyline", m_usePolyline);
    save("RadiusSnap", m_snapToCornerArcCenter);
    save("Edges", m_edgesDrawMode);
    save("BaseAngleIsFixed", m_baseAngleIsFixed);
}

void LC_ActionDrawRectangleAbstract::doLoadOptions() {
    m_ucsBasisBaseAngleRad = loadDouble("Angle", 0.0);
    m_cornersDrawMode = loadInt("Corners", CornersMode::CORNER_STRAIGHT);
    m_radius = loadDouble("Radius", 1.0);
    m_bevelX = loadDouble("LengthX", 1.0);
    m_bevelY = loadDouble("LengthY", 1.0);
    m_usePolyline = loadBool("Polyline", false);
    m_snapToCornerArcCenter = loadBool("RadiusSnap", false);
    m_edgesDrawMode = loadInt("Edges", EDGES_BOTH);
    m_baseAngleIsFixed = loadBool("BaseAngleIsFixed", false);
}

/**
 * Creates shape data by creation of polyline for snap point and storing snap point
 * @param snapPoint point of snap
 */
void LC_ActionDrawRectangleAbstract::createShapeData(const RS_Vector &snapPoint){
    m_shapeData = createPolyline(snapPoint);
}

/**
 * Extract polyline from shape data and add it (or it's entities) to the resulting list of entities
 */
bool LC_ActionDrawRectangleAbstract::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) {
    RS_Polyline *polyline = m_shapeData.resultingPolyline;
    // extract entities from polyline and insert them as result of action
    doAddPolylineToListOfEntities(polyline, ctx.entitiesToAdd, false);
    return true;
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
void LC_ActionDrawRectangleAbstract::doAddPolylineToListOfEntities(RS_Polyline *polyline, QList<RS_Entity *> &list, const bool preview){

    // we should additionally filter edge lines if we draw straight rectangle and not all edges should be drawn
    const bool shouldInspectForNonCompleteShape = m_edgesDrawMode != EDGES_BOTH && m_cornersDrawMode == CORNER_STRAIGHT; // here we draw only side edges

    // flag that defines whether we'll insert polyline or individual elements
    bool addAtOnce = false;
    if (preview){
        addAtOnce = !shouldInspectForNonCompleteShape;
    }
    else{
        if (shouldInspectForNonCompleteShape){
            addAtOnce = false; // can't use polyline as we'll draw partial edges
        }
        else{
            addAtOnce = m_usePolyline; // rely on polyline settings
        }
    }
    if (addAtOnce){
        // just insert created polyline into drawing or preview
        list<<polyline;
    }
    else {
        // iterate over entities of polyline
        int index = -1;
        for(const RS_Entity* entity: lc::LC_ContainerTraverser{*polyline, RS2::ResolveAll}.entities()) {
            index++;
            if (entity != nullptr){
                if (shouldInspectForNonCompleteShape){
                    // check whether this edge should be added according to edges mode
                    const bool sideEdgeLine = doCheckPolylineEntityAllowedInTrigger(index);
                    if (!sideEdgeLine){
                        continue; // skip this entity and go to next
                    }
                }
                // create clone of entity for safe deletion of original polyline
                RS_Entity *clone = entity->clone();
                clone->reparent(m_document);
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
bool LC_ActionDrawRectangleAbstract::doCheckPolylineEntityAllowedInTrigger(const int index) const{
    if (m_edgesDrawMode == EDGES_VERT) {
        return (index == 1) || (index == 3);
    }
    if (m_edgesDrawMode == EDGES_HOR){
        return (index == 0) || (index == 2);
    }
    return true;
}

/**
 * Cleanup after trigger operation execution
 */
void LC_ActionDrawRectangleAbstract::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    m_shapeData.resultingPolyline = nullptr;
    redraw();
}

/**
 * After trigger, by default we'll set relative zero to last snap point (to which trigger was invoked)
 * @return
 */
RS_Vector LC_ActionDrawRectangleAbstract::doGetRelativeZeroAfterTrigger(){
    return m_shapeData.snapPoint;
}

/**
 * We may trigger if there is shape data
 * @return
 */
bool LC_ActionDrawRectangleAbstract::doCheckMayTrigger(){
    return m_shapeData.resultingPolyline != nullptr;
}

/**
 * Creating preview entities. Calculates and creates polyline, and add it (or specific edges) to the list of preview entities.
 * @param e original event
 * @param snap snap point
 * @param list list of entities for preview
 * @param status current status of action
 */
void LC_ActionDrawRectangleAbstract::doPreparePreviewEntities([[maybe_unused]] const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    const ShapeData data = createPolyline(snap);
    const auto polyline = data.resultingPolyline;
    doAddPolylineToListOfEntities(polyline, list, true);
    if (m_showRefEntitiesOnPreview) {
        createRefPoint(data.centerPoint, list);
    }

    if (m_infoCursorOverlayPrefs->enabled && m_infoCursorOverlayPrefs->showEntityInfoOnCreation) {
        msg(tr("To be created:"), tr("Rectangle"))
            .linear(tr("Width:"), data.width)
            .linear(tr("Height:"), data.height)
            .vector(tr("Center:"), data.centerPoint)
            .toInfoCursorZone2(true);
    }
}

/**
 * process generic coordinates common for various actions and delegates processing to
 * inherited actions
 * @param status
 * @param isZero
 * @param pos
 */
void LC_ActionDrawRectangleAbstract::onCoordinateEvent(const int status, const bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetBevels:
            // actually, we'll allow zero values there - together with drawing as individual lines, that may
            // potentially bring interesting effects - with separation of edge lines on several segments
            m_bevelX = LC_LineMath::getMeaningful(pos.x);
            m_bevelY = LC_LineMath::getMeaningful(pos.y);
            updateOptions();
            restoreMainStatus();
            break;
        case SetAngle: // zero angle value processing
            if (isZero){
                m_baseAngleIsFixed = false;
                drawPreviewForLastPoint();
                updateOptions();
                restoreMainStatus();
            } else {
                commandMessage(tr("Invalid Angle"));
                updateActionPrompt();
            }
            break;
        case SetRadius: // zero radius value processing
            if (isZero){
                m_radius = 0.0;
                updateOptions();
                restoreMainStatus();
            }
            else{
                commandMessage(tr("Invalid radius"));
                updateActionPrompt();
            }
            break;
        default:
            // delegate processing to inherited actions
            doProcessCoordinateEvent(pos, isZero, status); // do processing of other vars
            break;
    }
}

/**
 * Extension point for inherited actions for processing coordinate events
 * @param coord coordinates
 * @param isZero is zero coordinate
 * @param status current state of action
 */
void LC_ActionDrawRectangleAbstract::doProcessCoordinateEvent([[maybe_unused]]const RS_Vector &coord,[[maybe_unused]] bool isZero, [[maybe_unused]]int status){}

/**
 * returns base angle in radians if base angle is fixed, or 0 otherwise.
 * @return
 */
double LC_ActionDrawRectangleAbstract::getActualBaseAngle() const{
    double result;
    if (m_baseAngleIsFixed){
        result = toWorldAngleFromUCSBasis(m_ucsBasisBaseAngleRad);
    }
    else{
        result = toWorldAngleFromUCSBasis(0.0);
    }
    return result;
}


/**
 * Processing of common commands
 * @param status
 * @param command command
 * @return true if command is processed, false if additional processing is needed
 */
bool LC_ActionDrawRectangleAbstract::doProcessCommand(const int status, const QString &command){
    bool processed = true;
    bool toMainStatus = true;

    if (checkCommand("angle",command)){ // initiates entering of base angle of rect (angle from corner1 to corner2)
        setStatus(SetAngle);
        toMainStatus = false;
    }
    else if (checkCommand("radius",command)){ // initiates entering rounding radius for corners
        setStatus(SetRadius);
        toMainStatus = false;
    }
    else if (checkCommand("bevels",command)){ // initiates entering of bevels or setting bevels corners mode
        if (status == SetCorners){
            m_cornersDrawMode = CORNER_BEVEL;
            updateOptions();
        }
        else {
            setStatus(SetBevels);
            toMainStatus = false;
        }
    }
    else if (checkCommand("nopoly",command)){  // disables drawing rect as polyline (so all elements are individual entities)
        m_usePolyline = false;
        updateOptions();
    }
    else if (checkCommand("usepoly",command)){ // enables drawing rect as polyline
        m_usePolyline = true;
        updateOptions();
    }
    else if (checkCommand("corners",command)){ // initiates entering corners mode
        setStatus(SetCorners);
        toMainStatus = false;
    }
    else if (checkCommand("str",command)){  // straight corners mode (no rounding, not bevel)
        if (status == SetCorners){
            m_cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
        }
    }
    else if (checkCommand("round",command)){ // rounded corners mode
        if (status == SetCorners){
            m_cornersDrawMode = CORNER_RADIUS;
            updateOptions();
        }
    }
    else if (checkCommand("edges", command)){ // initiates entering drawing edges modes for straight rect
        setStatus(SetEdges);
        toMainStatus = false;
    }
    else if (checkCommand("both", command)){ // all edges are drawn
        if (status == SetEdges){
            m_edgesDrawMode = EDGES_BOTH;
            m_cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
            restoreMainStatus();
        }
    }
    else if (checkCommand("hor", command)){  // only horizontal edges are drawn
        if (status == SetEdges){
            m_edgesDrawMode = EDGES_HOR;
            m_cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
            restoreMainStatus();
        }
    }
    else if (checkCommand("vert", command)){ // only vertical edges are drawn
        if (status == SetEdges){
            m_edgesDrawMode = EDGES_VERT;
            m_cornersDrawMode = CORNER_STRAIGHT;
            updateOptions();
            restoreMainStatus();
        }
    }
    else if (processCustomCommand(status,command, toMainStatus)){ // delegate processing to inherited class
        // intentionally do nothing
    }
    else{ // process entered value
        bool ok = false;
        const double value = RS_Math::eval(command, &ok);
        if (ok){
            switch (status) {
                case SetAngle: {
                    double ucsBasisAngleRad;
                    ok = parseToUCSBasisAngle(command, ucsBasisAngleRad);
                    m_ucsBasisBaseAngleRad = LC_LineMath::getMeaningfulAngle(ucsBasisAngleRad);
                    m_baseAngleIsFixed = true;
                    updateOptions();
                    toMainStatus = true;
                    break;
                }
                case SetRadius: {
                    m_radius = LC_LineMath::getMeaningful(value);
                    updateOptions();
                    toMainStatus = true;
                    break;
                }
                default: {
                    // let inherited action process this
                    processCommandValue(value, toMainStatus);
                    break;
                }
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
void LC_ActionDrawRectangleAbstract::prepareCornersDrawMode(double &radiusX, double &radiusY, bool &drawComplex, bool &drawBulge) const{
    switch (m_cornersDrawMode){
        case CORNER_STRAIGHT:{
            drawComplex = false;
            break;
        }
        case CORNER_BEVEL:
        {
            drawComplex = true;
            radiusX = m_bevelX;
            if (LC_LineMath::isNotMeaningful(radiusX)){
                // just double check of user input - for invalid value, draw just rect
                drawComplex = false;
            }
            radiusY = m_bevelY;
            if (LC_LineMath::isNotMeaningful(radiusY)){
                // just double check of user input - for invalid value, draw just rect
                drawComplex = false;
            }
            break;
        }
        case CORNER_RADIUS:{
            if (LC_LineMath::isNotMeaningful(m_radius)){
                drawComplex = false;
            }
            else{
                radiusX = m_radius;
                radiusY = m_radius;
                drawBulge = true;
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawRectangleAbstract::updateActionPrompt() {
    const int status = getStatus();
    switch (status) {
        case SetPoint1:
            updatePromptTRCancel(tr("Specify insertion point"),MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetAngle:
            updatePromptTRBack(tr("Specify angle"));
            break;
        case SetSize:
            updatePromptTRBack(tr("Specify size (width, height)"));
            break;
        case SetCorners:
            updatePromptTRBack(tr("Specify corners type\n[str|round|bevels]"));
            break;
        case SetBevels:
            updatePromptTRBack(tr("Specify corner bevel length (x,y)"));
            break;
        case SetRadius:
            updatePromptTRBack(tr("Specify corner radius"));
            break;
        case SetEdges:
            updatePromptTRBack(tr("Specify edges mode\n[both|hor|vert]"));
            break;
        default:
            doUpdateMouseButtonHints(status); // delegate to inherited classes to process additional statuses
            break;
    }
}

/**
 * method for custom prompts in inherited actions
 */
void LC_ActionDrawRectangleAbstract::doUpdateMouseButtonHints([[maybe_unused]]int status){
    LC_AbstractActionWithPreview::updateActionPrompt();
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
RS_Polyline* LC_ActionDrawRectangleAbstract::createPolylineByVertexes(const RS_Vector& bottomLeftCorner, const RS_Vector& bottomRightCorner,
                                                                      const RS_Vector& topRightCorner, const RS_Vector& topLeftCorner,
                                                                      const bool drawBulge, const bool drawComplex, const double radiusX,
                                                                      const double radiusY) const {
    auto *polyline = new RS_Polyline(m_document);

    if (drawComplex){ // we'll draw complex shape

        // vector used for corner size
        const auto radiusShiftX = RS_Vector(radiusX, 0);
        const auto radiusShiftY = RS_Vector(0, radiusY);

        // define left line
        const RS_Vector leftBottom = bottomLeftCorner + radiusShiftY;
        const RS_Vector leftTop = topLeftCorner - radiusShiftY;

        // define right line
        const RS_Vector rightBottom = bottomRightCorner + radiusShiftY;
        const RS_Vector rightTop = topRightCorner - radiusShiftY;

        // define top line
        const RS_Vector topLeft = topLeftCorner + radiusShiftX;
        const RS_Vector topRight = topRightCorner - radiusShiftX;

        // define bottom line

        const RS_Vector bottomLeft = bottomLeftCorner + radiusShiftX;
        const RS_Vector bottomRight = bottomRightCorner - radiusShiftX;

        // prepare bulge for 90 degrees curve, actually it might be just a constant
        const double bulge = tan(M_PI / 2 / 4); // normal case

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
void LC_ActionDrawRectangleAbstract::normalizeCorners(
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
 * @param radius
 */
void LC_ActionDrawRectangleAbstract::setCornerRadius(const double radius){
    m_radius = radius;
    drawPreviewForLastPoint();
}

/**
 * setter for bevel x length
 * @param value
 */
void LC_ActionDrawRectangleAbstract::setCornerBevelLengthX(const double value){
    m_bevelX = value;
    drawPreviewForLastPoint();
}

/**
 * setter for bevel y length
 * @param value
 */
void LC_ActionDrawRectangleAbstract::setCornerBevelLengthY(const double value){
    m_bevelY = value;
    drawPreviewForLastPoint();
}

/**
 * setter for corners draw mode
 * @param value
 */
void LC_ActionDrawRectangleAbstract::setCornersMode(const int value){
    m_cornersDrawMode = value;
    drawPreviewForLastPoint();
}

/**
 * Setter for base angle
 * @param angle
 */
void LC_ActionDrawRectangleAbstract::setUcsAngleDegrees(const double angle){
    const double value = RS_Math::deg2rad(angle);
    doSetAngle(value);
    setBaseAngleFixed(true);
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangleAbstract::setAngleRadians(const double angleRad) {
    doSetAngle(angleRad);
    setBaseAngleFixed(true);
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangleAbstract::doSetAngle(const double value) {
    m_ucsBasisBaseAngleRad = value;
}

double LC_ActionDrawRectangleAbstract::getUcsAngleDegrees() const {
    return RS_Math::rad2deg(m_ucsBasisBaseAngleRad);
}

/**
 * Setter for snap mode used for insertion
 * @param value
 */
void LC_ActionDrawRectangleAbstract::setInsertionPointSnapMode(const int value){
    m_insertionPointSnapMode = value;
    drawPreviewForLastPoint();
}

/**
 * If true, snap point of rectangle will be center of rounding arc instead of corner point
 * @param value
 */
void LC_ActionDrawRectangleAbstract::setSnapToCornerArcCenter(const bool value){
    m_snapToCornerArcCenter = value;
    drawPreviewForLastPoint();
}

/**
 * utility method called as soon as state updated
 * @param toMainStatus
 */
void LC_ActionDrawRectangleAbstract::stateUpdated(const bool toMainStatus){
    updateOptions();
    if (toMainStatus){
        restoreMainStatus();
    }
    drawPreviewForLastPoint();
}

/**
 * Default implementation of back processing
 * @param e
 * @param status
 */
void LC_ActionDrawRectangleAbstract::doBack([[maybe_unused]] const LC_MouseEvent* e, const int status){
    switch (status){
        case SetPoint1:{
            finishAction();
            break;
        }
        default:
            init(SetPoint1);
            m_mainStatus = SetPoint1;
            break;
    }
}
