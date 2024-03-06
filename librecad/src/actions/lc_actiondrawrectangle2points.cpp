#include <cmath>
#include "lc_actiondrawrectangle2points.h"
#include "rs_math.h"
#include "rs_graphicview.h"
#include "lc_rectangle2pointsoptions.h"
#include <QMouseEvent>

LC_ActionDrawRectangle2Points::LC_ActionDrawRectangle2Points(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionDrawRectangle("Draw rectangle 2 points",
                                    container, graphicView){
    actionType = RS2::ActionDrawRectangle2Points;
    angle = 0;
    init(SetPoint1);
}

LC_ActionDrawRectangle2Points::~LC_ActionDrawRectangle2Points() = default;

void LC_ActionDrawRectangle2Points::init(int status){
    RS_PreviewActionInterface::init(status);
}

RS_Polyline *LC_ActionDrawRectangle2Points::createPolyline(RS_Vector &snapPoint) const{

    double x = snapPoint.x;
    double y = snapPoint.y;

    RS_Vector c2 = snapPoint;
    RS_Vector c1 = corner1;


    double angleRad = RS_Math::deg2rad(angle);

    bool rotate = false;

    if (std::abs(angleRad) > RS_TOLERANCE_ANGLE){
        rotate = true;
    }

    if (rotate) {
        // rotate c2 around c1:
        c2 = c2.rotate(c1,-angleRad);
    }

    RS_Vector size = c2-c1;

    switch (insertionPointSnapMode){
        case SNAP_CORNER:
            break;
        case SNAP_EDGE_VERT:
            c1.y = c1.y - size.y;
            break;
        case SNAP_EDGE_HOR:
            c1.x = c1.x - size.x;
            break;
        case SNAP_MIDDLE:
            c1.x = c1.x - size.x;
            c1.y = c1.y - size.y;
            break;
    }

    size = c2-c1;

    switch (secondPointSnapMode){
        case SNAP_CORNER:
            break;
        case SNAP_EDGE_VERT:
            c2.y = c2.y + size.y;
            break;
        case SNAP_MIDDLE:{
            c2.y = c2.y + size.y;
            c2.x = c2.x + size.x;
            break;
        case SNAP_EDGE_HOR:
            c2.x = c2.x + size.x;
            break;
        }
    }

    bool drawBulge = false;
    double radiusX;
    double radiusY;

    // is it just rectangle or more complex shape
    bool drawComplex = true;

    // should we draw rounded corner or just lines
    prepareCornersDrawMode(radiusX, radiusY, drawComplex, drawBulge);

    // square:

    if (squareDrawRequested) {
        double w = c2.x - c1.x;
        double h = c2.y - c1.y;
        double s = std::max(std::abs(w), std::abs(h));

        if (w<0) {
            c2.x = c1.x - s;
        }
        else {
            c2.x = c1.x + s;
        }

        if (h<0) {
            c2.y = c1.y - s;
        }
        else {
            c2.y = c1.y + s;
        }
    }

    RS_Vector bottomLeftCorner = RS_Vector(c1.x , c1.y);
    RS_Vector bottomRightCorner = RS_Vector(c2.x, c1.y);
    RS_Vector topRightCorner = RS_Vector(c2.x, c2.y);
    RS_Vector topLeftCorner = RS_Vector(c1.x, c2.y);

    normalizeCorners(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner);

    if (drawBulge && snapToCornerArcCenter){

        // adjust corners coordinates, so we'll snap to arc centers

        RS_Vector radiusShiftX = RS_Vector(radiusX, 0);
        RS_Vector radiusShiftY = RS_Vector(0, radiusY);

        bottomLeftCorner = bottomLeftCorner - radiusShiftX - radiusShiftY;
        bottomRightCorner = bottomRightCorner + radiusShiftX - radiusShiftY;

        topLeftCorner = topLeftCorner - radiusShiftX + radiusShiftY;
        topRightCorner = topRightCorner + radiusShiftX + radiusShiftY;
    }

    RS_Polyline *polyline = createPolylineByVertexes(bottomLeftCorner, bottomRightCorner, topRightCorner, topLeftCorner, drawBulge, drawComplex, radiusX, radiusY);

    if (rotate) {
        // rotate corners:
        // now we'll rotate shape on specific angle
        polyline->rotate(corner1, angleRad);
    }

    return polyline;
}

void LC_ActionDrawRectangle2Points::doAfterTrigger(){
    setStatus(SetPoint1);
}

void LC_ActionDrawRectangle2Points::proceedMouseLeftButtonReleasedEvent(QMouseEvent *e){
    switch (getStatus()){
        case SetPoint1: {
            RS_Vector snap = snapPoint(e);
            graphicView->moveRelativeZero(snap);
            corner1 = snap;
            setStatus(SetPoint2);
            break;
        }
        case SetPoint2: {
            RS_Vector snap = snapPoint(e);
            squareDrawRequested = e->modifiers() & Qt::ShiftModifier;
            resultingPolyline = createPolyline(snap);
            graphicView->moveRelativeZero(snap);
            trigger();
            break;
        }
    }
}

void LC_ActionDrawRectangle2Points::processMouseEvent(QMouseEvent *e){
    squareDrawRequested = e->modifiers() & Qt::ShiftModifier;
}

void LC_ActionDrawRectangle2Points::setMainStatus(){
    setStatus(SetPoint1);
}

void LC_ActionDrawRectangle2Points::processCommandValue(double value){

}

bool LC_ActionDrawRectangle2Points::processCustomCommand(RS_CommandEvent *e, const QString &command){
    return false;
}

void LC_ActionDrawRectangle2Points::processCoordinateEvent(RS_CoordinateEvent *pEvent, RS_Vector vector, bool zero){

}

void LC_ActionDrawRectangle2Points::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_Rectangle2PointsOptions>(nullptr);
}

bool LC_ActionDrawRectangle2Points::mayDrawPreview(QMouseEvent *pEvent){
    return getStatus() != SetPoint1;
}

int LC_ActionDrawRectangle2Points::getSecondPointSnapMode(){
    return secondPointSnapMode;
}

void LC_ActionDrawRectangle2Points::setSecondPointSnapMode(int value){
    secondPointSnapMode = value;
    drawPreviewForLastPoint();
}




