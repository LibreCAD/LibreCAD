#include <cmath>
#include "lc_actiondrawrectangle1point.h"
#include "rs_arc.h"
#include "rs_preview.h"
#include "QMouseEvent"
#include "rs_math.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "lc_rectangle1pointoptions.h"

LC_ActionDrawRectangle1Point::LC_ActionDrawRectangle1Point(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionDrawRectangle("Draw rectangles rel",
                               container, graphicView){
    actionType = RS2::ActionDrawRectangle1Point;
}

LC_ActionDrawRectangle1Point::~LC_ActionDrawRectangle1Point() = default;

void LC_ActionDrawRectangle1Point::init(int status){
    RS_PreviewActionInterface::init(status);
}

// positions of snap points on the rectangle. Actually, this vector is used for
// coordinates transformation in createPolyline function
const std::vector<RS_Vector> LC_ActionDrawRectangle1Point::snapPoints {
    RS_Vector(-1,  1) , // top left
    RS_Vector( 0,  1) , // top
    RS_Vector( 1,  1) , // top right
    RS_Vector(-1,  0) , // left
    RS_Vector( 0,  0) , // middle
    RS_Vector( 1,  0) , // right
    RS_Vector(-1, -1) , // bottom left
    RS_Vector( 0, -1) , // bottom
    RS_Vector( 1, -1) , // bottom right
};

/**
 * Central function used to draw a rectangle, that is positioned by one of its point (defined by snapMode) in provided snap point.
 * Rectangle may be rotated on specified angle, and corners of rectangle are drawn according to specified mode (straight, rounded corners or bevels
 * @param snapPoint primary point used for positioning of shape
 * @return positioned polyline
 */
RS_Polyline *LC_ActionDrawRectangle1Point::createPolyline(RS_Vector &snapPoint) const{

    double x = snapPoint.x;
    double y = snapPoint.y;

    // calculate half size of required size
    double halfWidth  = width/2;
    double halfHeight = height/2;

    double radiusX;
    double radiusY;

    // is it just rectangle or more complex shape
    bool drawComplex = true;

    // should we draw rounded corner or just lines
    bool drawBulge = false;

    prepareCornersDrawMode(radiusX, radiusY, drawComplex, drawBulge);

    if (drawBulge){
        if (sizeIsInner){
            // increase the overall height and width to value of diameter (radius*2)
            halfWidth = halfWidth + radius;
            halfHeight = halfHeight + radius;
        }
    }

    // create centered rect first - it center is the same as provided snap point

    // here we calculate coordinates of corners
    RS_Vector bottomLeftCorner = RS_Vector(x - halfWidth , y - halfHeight);
    RS_Vector topRightCorner = RS_Vector(x + halfWidth, y + halfHeight);
    RS_Vector bottomRightCorner = RS_Vector(x + halfWidth, y - halfHeight);
    RS_Vector topLeftCorner = RS_Vector(x - halfWidth, y + halfHeight);


    RS_Polyline *polyline = createPolylineByVertexes(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner, drawBulge, drawComplex, radiusX, radiusY);

    // shape is built, so now we'll position it

    // vector to reference point that is used as snap
    RS_Vector reference = snapPoints.at(insertionPointSnapMode);

    // utility vector (half size)
    RS_Vector halfRect = RS_Vector(-halfWidth, -halfHeight);

    // prepare vector we'll use for moving shape
    RS_Vector moveVector = reference * halfRect;

    // additional move if corners are round and snap to center is needed
    if (drawBulge){
        if (snapToCornerArcCenter){
            moveVector = moveVector + reference*radius;
        }
    }

    // move shape so it's reference point will correspond to provided snap point
    polyline->move(moveVector);

    // now we'll rotate shape on specific angle
    double angleRad = RS_Math::deg2rad(angle);
    polyline->rotate(snapPoint, angleRad);

    return polyline;
}

void LC_ActionDrawRectangle1Point::proceedMouseLeftButtonReleasedEvent(QMouseEvent *e){
    switch (getStatus()) {
        case SetPoint1: {
            RS_Vector snap = snapPoint(e);
            resultingPolyline = createPolyline(snap);
            graphicView->moveRelativeZero(snap);
            trigger();
            break;
        }
        default:
            break;
    }
}


void LC_ActionDrawRectangle1Point::processCoordinateEvent(RS_CoordinateEvent *pEvent, RS_Vector coord, bool isRelativeZero){
    switch (getStatus()) {
        case SetPoint1:
            resultingPolyline = createPolyline(coord);
            graphicView->moveRelativeZero(coord);
            trigger();
            break;
        case SetSize: {
            double w = std::abs(coord.x);
            if (w > RS_TOLERANCE){
                double h = std::abs(coord.y);
                if (h > RS_TOLERANCE){
                    width = w;
                    height = h;
                    updateOptions();
                    setStatus(SetPoint1);
                } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Zero height is invalid"));
                    updateMouseButtonHints();
                }
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Zero width is invalid"));
                updateMouseButtonHints();
            }
            break;
        }
        case SetWidth:
            RS_DIALOGFACTORY->commandMessage(tr("Zero width is invalid"));
            break;
        case SetHeight:
            RS_DIALOGFACTORY->commandMessage(tr("Zero height is invalid"));
            break;
    }
}


void LC_ActionDrawRectangle1Point::setMainStatus(){
    setStatus(SetPoint1);
}

void LC_ActionDrawRectangle1Point::processCommandValue(double value){
    switch (getStatus()) {
        case SetWidth: {
            double w = std::abs(value);
            if (w > RS_TOLERANCE){
                width = w;
                updateOptions();
                setStatus(SetPoint1);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Invalid width, it should be non-zero positive"));
                updateMouseButtonHints();
            }
            break;
        }
        case SetHeight: {
            double h = std::abs(value);
            if (h > RS_TOLERANCE){
                height = h;
                updateOptions();
                setStatus(SetPoint1);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Invalid height, it should be non-zero positive"));
                updateMouseButtonHints();
            }
            break;
        }
    }
}


QStringList LC_ActionDrawRectangle1Point::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetReferencePoint:{
            cmd += command("topl");
            cmd += command("top");
            cmd += command("topr");
            cmd += command("left");
            cmd += command("middle");
            cmd += command("right");
            cmd += command("bottoml");
            cmd += command("bottom");
            cmd += command("bottomr");
            break;
        }
        case SetCorners:{
            cmd += command("str");
            cmd += command("round");
            cmd += command("bevels");
            break;
        }
        case SetPoint1:
        case SetAngle:
        case SetHeight:
        case SetWidth:
        case SetBevels:
        case SetRadius:
        case SetSize:
            cmd += command("point");
            cmd += command("width");
            cmd += command("height");
            cmd += command("size");
            cmd += command("angle");
            cmd += command("corners");
            cmd += command("bevels");
            cmd += command("refpoint");
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

void LC_ActionDrawRectangle1Point::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify insertion point"),
                                                tr("Cancel"));
            break;
        case SetHeight:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify height"),
                                                tr("Back"));
            break;
        case SetWidth:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify width"),
                                                tr("Back"));
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
        case SetReferencePoint:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point [topl|top|topr|left|middle|right|bottoml|bottom|bottomr]"),
                                                tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}


void LC_ActionDrawRectangle1Point::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_Rectangle1PointOptions>(nullptr);
}

bool LC_ActionDrawRectangle1Point::processCustomCommand(RS_CommandEvent *e, const QString &c){
    bool result = true;
    if (checkCommand("width",c)){
      e->accept();
        setStatus(SetWidth);
    }
    else if (checkCommand("height",c)){
        e->accept();
        setStatus(SetHeight);
    }
    else if (checkCommand("size",c)){
        e->accept();
        setStatus(SetSize);
    }
    else if (checkCommand("point",c)){
        e->accept();
        setStatus(SetPoint1);
    }
    else if (checkCommand("refpoint",c)){
        e->accept();
        setStatus(SetReferencePoint);
    }
    else if (checkCommand("topl",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_TOP_LEFT;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("top",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_TOP;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("topr",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_TOP_RIGHT;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("left",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_LEFT;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("middle",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_MIDDLE;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("right",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_RIGHT;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("bottoml",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_BOTTOM_LEFT;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("bottom",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_BOTTOM;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("bottomr",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            insertionPointSnapMode = SNAP_BOTTOM_RIGHT;
            updateOptions();
            setStatus(SetPoint1);
        }
    }
    else if (checkCommand("snapcorner",c)){
        e->accept();
        snapToCornerArcCenter = false;
        updateOptions();
        setStatus(SetPoint1);
    }
    else if (checkCommand("snapshift",c)){
        e->accept();
        snapToCornerArcCenter = true;
        updateOptions();
        setStatus(SetPoint1);
    }
    else{
        result = false;
    }
    return result;
}


void LC_ActionDrawRectangle1Point::setWidth(double value){
    width = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangle1Point::setHeight(double value){
    height = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangle1Point::setSizeInner(bool value){
    sizeIsInner = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawRectangle1Point::doAfterTrigger(){

}
