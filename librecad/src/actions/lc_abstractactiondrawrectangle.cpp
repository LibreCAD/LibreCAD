
#include "rs_arc.h"
#include <cmath>
#include "QMouseEvent"
#include "lc_abstractactiondrawrectangle.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_entitycontainer.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"
#include "lc_abstractactionwithpreview.h"
#include "lc_linemath.h"

LC_AbstractActionDrawRectangle::LC_AbstractActionDrawRectangle(
    const char *name,
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview(name, container, graphicView),
    shapeData(nullptr){
}

LC_AbstractActionDrawRectangle::~LC_AbstractActionDrawRectangle() = default;

void LC_AbstractActionDrawRectangle::createShapeData(const RS_Vector &snapPoint){
    shapeData = new ShapeData();
    RS_Polyline* polyline = createPolyline(snapPoint);
    shapeData->resultingPolyline = polyline;
    shapeData->snapPoint = snapPoint;
}

void LC_AbstractActionDrawRectangle::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    RS_Polyline *polyline = shapeData->resultingPolyline;
        // extract entities from polyline and insert them as result of action
    doAddPolylineToListOfEntities(polyline, list, false);

}

void LC_AbstractActionDrawRectangle::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    if (shapeData != nullptr){
        delete shapeData;
        shapeData = nullptr;
    }
}

RS_Vector LC_AbstractActionDrawRectangle::doGetRelativeZeroAfterTrigger(){
    return shapeData -> snapPoint;
}

bool LC_AbstractActionDrawRectangle::doCheckMayTrigger(){
    return shapeData != nullptr;
}

void LC_AbstractActionDrawRectangle::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    RS_Polyline *polyline = createPolyline(snap);
    polyline->setLayerToActive();
    polyline->setPenToActive();
    doAddPolylineToListOfEntities(polyline, list, true);
}

void LC_AbstractActionDrawRectangle::doAddPolylineToListOfEntities(RS_Polyline *polyline, QList<RS_Entity *> &list, bool preview){

    bool shouldInspectForNonCompleteShape = edgesDrawMode != EDGES_BOTH && cornersDrawMode == CORNER_STRAIGHT; // here we draw only side edges

    bool addAtOnce = true;
    if (preview){
        addAtOnce = !shouldInspectForNonCompleteShape;
    }
    else{
        if (shouldInspectForNonCompleteShape){
            addAtOnce = false;
        }
        else{
            addAtOnce = usePolyline;
        }
    }
    if (addAtOnce){
        // just insert created polyline into drawing or preview
        list<<polyline;
    }
    else {
        int index = -1;
        for (RS_Entity *entity = polyline->firstEntity(RS2::ResolveAll); entity;
             entity = polyline->nextEntity(RS2::ResolveAll)) {
            index++;
            if (entity != nullptr){
                if (shouldInspectForNonCompleteShape){
                    bool sideEdgeLine = doCheckPolylineEntityAllowedInTrigger(entity, index);
                    if (!sideEdgeLine){
                        continue; // skip this entity and go to next
                    }
                }

                RS_Entity *clone = entity->clone(); // use clone for safe deletion of polyline
                clone->reparent(container);
                list << clone;
            }
        }
        delete polyline; //don't need it anymore
    }
}


bool LC_AbstractActionDrawRectangle::doCheckPolylineEntityAllowedInTrigger(RS_Entity *pEntity, int index){
    if (edgesDrawMode == EDGES_VERT)
        return (index == 1) || (index == 3);
    else if (edgesDrawMode == EDGES_HOR){
        return (index == 0) || (index == 2);
    }
    else
        return true;
}

/**
 * process generic coordinates common for various actions and delegates processing to
 * inherited actions
 * @param coordinate event
 */
void LC_AbstractActionDrawRectangle::onCoordinateEvent(const RS_Vector &coord, bool isZero, int status){
    switch (status) {
        case SetBevels:
            // actually, we'll allow zero values there - together with drawing as individual lines, that may
            // potentially bring interesting effects - with separation of edge lines on several segments
            bevelX = LC_LineMath::getMeaningful(coord.x);
            bevelY = LC_LineMath::getMeaningful(coord.y);
            updateOptions();
            setMainStatus();
            break;
        case SetAngle:
            if (isZero){
                angle = 0.0;
                updateOptions();
                setMainStatus();
            } else {
                commandMessageTR("Invalid Angle");
                updateMouseButtonHints();
            }
            break;
        case SetRadius:
            if (isZero){
                radius = 0.0;
                updateOptions();
                setMainStatus();
            }
            else{
                commandMessageTR("Invalid radius");
                updateMouseButtonHints();
            }
            break;
        default:
            doProcessCoordinateEvent(coord, isZero, status); // do processing of other vars
            break;
    }
}

void LC_AbstractActionDrawRectangle::doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status){

}

bool LC_AbstractActionDrawRectangle::doProcessCommand(RS_CommandEvent *e, const QString &c){
    bool processed = true;
    bool toMainStatus = true;

    if (checkCommand("help", c)) {
        commandMessage(msgAvailableCommands() + getAvailableCommands().join(", "));
    }
    else if (checkCommand("angle",c)){
        setStatus(SetAngle);
        toMainStatus = false;
    }
    else if (checkCommand("radius",c)){
        setStatus(SetRadius);
        toMainStatus = false;
    }
    else if (checkCommand("bevels",c)){
        if (getStatus() == SetCorners){
            cornersDrawMode = CORNER_BEVEL;
        }
        else {
            setStatus(SetBevels);
            toMainStatus = false;
        }
    }
    else if (checkCommand("nopoly",c)){
        usePolyline = false;
    }
    else if (checkCommand("usepoly",c)){
        usePolyline = true;
    }
    else if (checkCommand("corners",c)){
        setStatus(SetCorners);
        toMainStatus = false;
    }
    else if (checkCommand("str",c)){
        if (getStatus() == SetCorners){
            cornersDrawMode = CORNER_STRAIGHT;
        }
    }
    else if (checkCommand("round",c)){
        if (getStatus() == SetCorners){
            cornersDrawMode = CORNER_RADIUS;
        }
    }
    else if (processCustomCommand(e,c, toMainStatus)){ // delegate processing to inherited class
        // intentionally do nothing
    }
    else{ // process entered value
        bool ok = false;
        double value = RS_Math::eval(c, &ok);
        if (ok){
            switch (getStatus()) {
                case SetAngle: {
                    angle = LC_LineMath::getMeaningfulAngle(value);
                    break;
                }
                case SetRadius: {
                    radius = LC_LineMath::getMeaningful(value);
                    break;
                }
                default:
                    // let inherited action process this
                    processCommandValue(value);
            }
        }
        else {
            processed = false;
            commandMessageTR("Invalid value");
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
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTR("Specify insertion point","Cancel");
            break;
        case SetAngle:
            updateMouseWidgetTR("Specify angle","Back");
            break;
        case SetSize:
            updateMouseWidgetTR("Specify size (width, height)","Back");
            break;
        case SetCorners:
            updateMouseWidgetTR("Specify corners type [str|round|bevels]","Back");
            break;
        case SetBevels:
            updateMouseWidgetTR("Specify corner bevel length (x,y)","Back");
            break;
        case SetRadius:
            updateMouseWidgetTR("Specify corner radius","Back");
            break;
        default:
            doUpdateMouseButtonHints(); // delegate to inherited classes to process additional statuses
            break;
    }
}

/**
 * method for custom prompts in inherited actions
 */
void LC_AbstractActionDrawRectangle::doUpdateMouseButtonHints(){
    RS_DIALOGFACTORY->updateMouseWidget();
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
    RS_Polyline *polyline = new RS_Polyline(container);

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
    RS_Vector &topLeftCorner) const{
    // normalize rect to ensure that passed corners are in right places in coordinates grid (i.e. - top is over bottom and left is before right)
    RS_Vector tmp;
    if (bottomLeftCorner.x > bottomRightCorner.x){ // check whether left and right top corners are correct or should be swapped
        tmp = bottomRightCorner;
        bottomRightCorner = bottomLeftCorner;
        bottomLeftCorner = tmp;

        tmp = topRightCorner;
        topRightCorner = topLeftCorner;
        topLeftCorner = tmp;
    }

    if (topLeftCorner.y < bottomLeftCorner.y){ // check whether bottom corners are correct
        tmp = topLeftCorner;
        topLeftCorner = bottomLeftCorner;
        bottomLeftCorner = tmp;

        tmp = topRightCorner;
        topRightCorner = bottomRightCorner;
        bottomRightCorner = tmp;
    }
}

void LC_AbstractActionDrawRectangle::setRadius(double value){
    radius = value;
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::setLengthX(double value){
    bevelX = value;
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::setLengthY(double value){
    bevelY = value;
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::setCornersMode(int value){
    cornersDrawMode = value;
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::setAngle(double value){
    angle = value;
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::setInsertionPointSnapMode(int value){
    insertionPointSnapMode = value;
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::setSnapToCornerArcCenter(bool value){
    snapToCornerArcCenter = value;
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::stateUpdated(bool toMainStatus){
    updateOptions();
    if (toMainStatus){
        setMainStatus();
    }
    drawPreviewForLastPoint();
}

void LC_AbstractActionDrawRectangle::doBack(QMouseEvent *pEvent, int status){
    switch (status){
        case (SetPoint1):{
            finishAction();
            break;
        }
        default:
            init(SetPoint1);
            break;
    }
}



