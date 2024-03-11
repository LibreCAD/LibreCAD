
#include "lc_rectangle2pointsoptions.h"
#include "lc_actiondrawrectangle2points.h"
#include <cmath>
#include "QMouseEvent"
#include "lc_abstractactiondrawrectangle.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_document.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"

LC_AbstractActionDrawRectangle::LC_AbstractActionDrawRectangle(
    const char *name,
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface(name, container, graphicView){
}

LC_AbstractActionDrawRectangle::~LC_AbstractActionDrawRectangle() = default;

/**
 * trigger action and insertions of created rect into drawing
 */
void LC_AbstractActionDrawRectangle::trigger(){
    RS_PreviewActionInterface::trigger();
    if (resultingPolyline != nullptr){ // extract and proceed saved polyline
        if (document){
            document->startUndoCycle();
            if (usePolyline){
                // just insert created polyline into drawing
                resultingPolyline->setLayerToActive();
                resultingPolyline->setPenToActive();
                container->addEntity(resultingPolyline);
                document->addUndoable(resultingPolyline);
            } else {
                // extract entities from polyline and insert them as result of action
                for (RS_Entity *entity = resultingPolyline->firstEntity(RS2::ResolveAll); entity;
                     entity = resultingPolyline->nextEntity(RS2::ResolveAll)) {

                    if (entity){
                        RS_Entity* clone = entity->clone(); // use clone for safe deletion of polyline
                        clone->reparent(container);
                        clone->setLayerToActive();
                        clone->setPenToActive();
                        container->addEntity(clone);
                        document->addUndoable(clone);
                    }
                }
                delete resultingPolyline; //don't need it anymore
            }
            document->endUndoCycle();
        }
        graphicView->redraw(RS2::RedrawDrawing);
        resultingPolyline = nullptr; // do cleanup
        doAfterTrigger(); // inherited actions may do additional processing there
    }
}

void LC_AbstractActionDrawRectangle::mouseReleaseEvent(QMouseEvent *e){
    if (e->button()==Qt::LeftButton) {
        proceedMouseLeftButtonReleasedEvent(e); // delegate actual processing to inherited actions
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1); // fixme - check status for init
    }
}

void LC_AbstractActionDrawRectangle::mouseMoveEvent(QMouseEvent *e){
    if (mayDrawPreview(e)){ // check whether preview may be drawn according to state etc.
        RS_Vector snap = snapPoint(e);
        processMouseEvent(e); // delegate processing to inherited actions
        drawPreviewForPoint(snap);
        lastSnapPoint = snap; // store snap point for later use (like redraw preview on options change)
    }
}
/**
 * redraws preview in it's last point. This is used if the user would like to change some option
 * via options widget - and thus we'll reflect such change in preview without waiting for the next mouse move event
 */
void LC_AbstractActionDrawRectangle::drawPreviewForLastPoint(){
    if (lastSnapPoint.valid){
        if (mayDrawPreview(nullptr)){
            drawPreviewForPoint(lastSnapPoint);
        }
    }
}

/**
 * draws preview in provided snap point
 * @param snapPoint
 */
void LC_AbstractActionDrawRectangle::drawPreviewForPoint(RS_Vector &snapPoint){
    RS_Polyline *polyline = createPolyline(snapPoint);
    deletePreview();
    polyline->setLayerToActive();
    polyline->setPenToActive();
    preview->addEntity(polyline);
    drawPreview();
    graphicView->redraw(RS2::RedrawDrawing);
}

/**
 * process generic coordinates common for various actions and delegates processing to
 * inherited actions
 * @param e
 */
void LC_AbstractActionDrawRectangle::coordinateEvent(RS_CoordinateEvent *e){
    if (!e) return;
    RS_Vector coord = e->getCoordinate();
    RS_Vector zero = RS_Vector(0, 0, 0);
    bool isZero = coord == zero; // use it to handle "0" shortcut (it is passed as 0,0 vector)

    switch (getStatus()) {
        case SetBevels:
            // actually, we'll allow zero values there - together with drawing as individual lines, that may
            // potentially bring interesting effects - with separation of edge lines on several segments
            bevelX = std::abs(coord.x);
            if (bevelX < RS_TOLERANCE){
                bevelX = 0.0;
            }
            bevelY = std::abs(coord.y);
            if (bevelY < RS_TOLERANCE){
                bevelY = 0.0;
            }
            updateOptions();
            setMainStatus();
            break;
        case SetAngle:
            if (isZero){
                angle = 0.0;
                updateOptions();
                setMainStatus();
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Invalid Angle"));
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
                RS_DIALOGFACTORY->commandMessage(tr("Invalid radius"));
                updateMouseButtonHints();
            }
            break;
        default:
            processCoordinateEvent(e, coord, isZero); // do processing of other vars
            break;
    }
}

void LC_AbstractActionDrawRectangle::commandEvent(RS_CommandEvent *e){

    bool processed = true;
    bool toMainStatus = true;
    QString const& c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
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
            cornersDrawMode = DRAW_BEVEL;
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
            cornersDrawMode = DRAW_STRAIGHT;
        }
    }
    else if (checkCommand("round",c)){
        if (getStatus() == SetCorners){
            cornersDrawMode = DRAW_RADIUS;
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
                    double a = value;
                    if (std::abs(value) < RS_TOLERANCE_ANGLE){
                        a = 0.0;
                    }
                    angle = a;
                    break;
                }
                case SetRadius: {
                    double r = std::abs(value);
                    if (std::abs(r) < RS_TOLERANCE){
                        r = 0.0;
                    }
                    radius = r;
                    break;
                }
                default:
                    // let inherited action process this
                    processCommandValue(value);
            }
        }
        else {
            processed = false;
            RS_DIALOGFACTORY->commandMessage(tr("Invalid value"));
        }
    }
    if (processed){
        e->accept();
        stateUpdated(toMainStatus);
    }
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
        case DRAW_STRAIGHT:{
            drawComplex = false;
            break;
        }
        case DRAW_BEVEL:
        {
            drawComplex = true;
            radiusX = bevelX;
            if (radiusX < RS_TOLERANCE){
                // just double check of user input - for invalid value, draw just rect
                drawComplex = false;
            }
            radiusY = bevelY;
            if (radiusY < RS_TOLERANCE){
                // just double check of user input - for invalid value, draw just rect
                drawComplex = false;
            }
            break;
        }
        case DRAW_RADIUS:{
            if (radius < RS_TOLERANCE){
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
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify insertion point"),
                                                tr("Cancel"));
            break;
        case SetAngle:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify angle"),
                                                tr("Back"));
            break;
        case SetSize:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify size (width, height)"),
                                                tr("Back"));
            break;
        case SetCorners:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify corners type [str|round|bevels]"),
                                                tr("Back"));
            break;
        case SetBevels:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify corner bevel length (x,y)"),
                                                tr("Back"));
            break;
        case SetRadius:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify corner radius"),
                                                tr("Back"));
            break;
        default:
            doUpdateMouseButtonHints();
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


bool LC_AbstractActionDrawRectangle::mayDrawPreview(QMouseEvent *pEvent){
    return true;
}

void LC_AbstractActionDrawRectangle::doAfterTrigger(){

}

void LC_AbstractActionDrawRectangle::processMouseEvent(QMouseEvent *e){

}

void LC_AbstractActionDrawRectangle::stateUpdated(bool toMainStatus){
    updateOptions();
    if (toMainStatus){
        setMainStatus();
    }
    drawPreviewForLastPoint();
}