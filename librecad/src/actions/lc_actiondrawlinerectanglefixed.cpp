#include <cmath>
#include "lc_actiondrawlinerectanglefixed.h"
#include "rs_arc.h"
#include "rs_preview.h"
#include "QMouseEvent"
#include "rs_math.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "lc_linerectanglefixedoptions.h"

LC_ActionDrawLineRectangleFixed::LC_ActionDrawLineRectangleFixed(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Draw rectangles rel",
                               container, graphicView){
    actionType = RS2::ActionDrawLineRectangleFixed;
}

LC_ActionDrawLineRectangleFixed::~LC_ActionDrawLineRectangleFixed() = default;

void LC_ActionDrawLineRectangleFixed::init(int status){
    RS_PreviewActionInterface::init(status);
}

// positions of snap points on the rectangle. Actually, this vector is used for
// coordinates transformation in createPolyline function
const std::vector<RS_Vector> LC_ActionDrawLineRectangleFixed::snapPoints {
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
RS_Polyline *LC_ActionDrawLineRectangleFixed::createPolyline(RS_Vector &snapPoint) const{

    RS_Polyline* polyline = new RS_Polyline(this->container);

    double x = snapPoint.x;
    double y = snapPoint.y;

    // calculate half size of required size
    double halfWidth  = width/2;
    double halfHeight = height/2;

    // create centered rect first - it center is the same as provided snap point

    // here we calculate coordinates of corners
    RS_Vector bottomLeftCorner = RS_Vector(x - halfWidth , y - halfHeight);
    RS_Vector topRightCorner = RS_Vector(x + halfWidth, y + halfHeight);
    RS_Vector bottomRightCorner = RS_Vector(x + halfWidth, y - halfHeight);
    RS_Vector topLeftCorner = RS_Vector(x - halfWidth, y + halfHeight);

    double radiusX;
    double radiusY;

    // is it just rectangle or more complex shape
    bool drawComplex = true;

    // should we draw rounded corner or just lines
    bool drawBulge = false;

    switch (cornersDrawMode){
        case DRAW_STAIGHT:{
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

    if (drawComplex){ // we'll draw complex shape
        // vector used for corner size
        RS_Vector radiusShiftX = RS_Vector(radiusX,0);
        RS_Vector radiusShiftY = RS_Vector(0,radiusY);

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
        double bulge = std::tan(M_PI/2/4);

        // start to build the shape starting from bottom left corner, than go right, up, left and down

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

    }
    else{ // here we just draw plain rectangle
        polyline->addVertex(bottomLeftCorner);
        polyline->addVertex(bottomRightCorner);
        polyline->addVertex(topRightCorner);
        polyline->addVertex(topLeftCorner);
        polyline->addVertex(bottomLeftCorner);
    }

    // shape is built, so now we'll position it

    // vector to reference point that is used as snap
    RS_Vector reference = snapPoints.at(snapMode);

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

void LC_ActionDrawLineRectangleFixed::trigger(){
    RS_PreviewActionInterface::trigger();
    if (resultingPolyline != nullptr){
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
        resultingPolyline = nullptr;
    }
}

void LC_ActionDrawLineRectangleFixed::mouseMoveEvent(QMouseEvent *e){
    RS_Vector snap = snapPoint(e);
    drawPreviewForPoint(snap);
    lastSnapPoint = snap;
}

void LC_ActionDrawLineRectangleFixed::drawPreviewForLastPoint(){
    if (lastSnapPoint.valid){
        drawPreviewForPoint(lastSnapPoint);
    }
}

void LC_ActionDrawLineRectangleFixed::drawPreviewForPoint(RS_Vector &snapPoint){
    RS_Polyline *polyline = createPolyline(snapPoint);
    deletePreview();
    polyline->setLayerToActive();
    polyline->setPenToActive();
    preview->addEntity(polyline);
    drawPreview();
    graphicView->redraw(RS2::RedrawDrawing);
}

void LC_ActionDrawLineRectangleFixed::mouseReleaseEvent(QMouseEvent *e){
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
            case SetPosition: {
                RS_Vector snap = snapPoint(e);
                resultingPolyline = createPolyline(snap);
                graphicView->moveRelativeZero(snap);
                trigger();
                break;
            }
            default:
                break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void LC_ActionDrawLineRectangleFixed::coordinateEvent(RS_CoordinateEvent *e){
    if (!e) return;
    RS_Vector coord = e->getCoordinate();
    RS_Vector relativeZero = RS_Vector(0,0,0);
    bool isRelativeZero = coord == relativeZero; // use it to handle "0" shortcut (it is passed as 0,0 verctor)

    switch (getStatus()) {
        case SetPosition:
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
                    setStatus(SetPosition);
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
            setStatus(SetPosition);
            break;
        case SetAngle:
            if (isRelativeZero){
                angle = 0.0;
                updateOptions();
                setStatus(SetPosition);
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Invalid Angle"));
                updateMouseButtonHints();
            }
            break;
        case SetRadius:
            if (isRelativeZero){
               radius = 0.0;
               updateOptions();
               setStatus(SetPosition);
            }
            else{
                RS_DIALOGFACTORY->commandMessage(tr("Invalid radius"));
                updateMouseButtonHints();
            }
            break;
        case SetWidth:
            RS_DIALOGFACTORY->commandMessage(tr("Zero width is invalid"));
            break;
        case SetHeight:
            RS_DIALOGFACTORY->commandMessage(tr("Zero height is invalid"));
            break;
    }
}

void LC_ActionDrawLineRectangleFixed::commandEvent(RS_CommandEvent *e){
    QString const& c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        e->accept();
    }
    else if (checkCommand("angle",c)){
        e->accept();
        setStatus(SetAngle);
    }
    else if (checkCommand("radius",c)){
        e->accept();
        setStatus(SetRadius);
    }
    else if (checkCommand("width",c)){
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
        setStatus(SetPosition);
    }
    else if (checkCommand("bevels",c)){
        e->accept();
        if (getStatus() == SetCorners){
            cornersDrawMode = DRAW_BEVEL;
            updateOptions();
            setStatus(SetPosition);
        }
        else {
            setStatus(SetBevels);
        }
    }
    else if (checkCommand("nopoly",c)){
        e->accept();
        usePolyline = false;
        updateOptions();
        setStatus(SetPosition);
    }
    else if (checkCommand("usepoly",c)){
        e->accept();
        usePolyline = true;
        updateOptions();
        setStatus(SetPosition);
    }
    else if (checkCommand("refpoint",c)){
        e->accept();
        setStatus(SetReferencePoint);
    }
    else if (checkCommand("topl",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_TOP_LEFT;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("top",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_TOP;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("topr",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_TOP_RIGHT;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("left",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_LEFT;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("middle",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_MIDDLE;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("right",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_RIGHT;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("bottoml",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_BOTTOM_LEFT;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("bottom",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_BOTTOM;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("bottomr",c)){
        if (getStatus() == SetReferencePoint){
            e->accept();
            snapMode = SNAP_BOTTOM_RIGHT;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("corners",c)){
          e->accept();
          setStatus(SetCorners);
    }
    else if (checkCommand("str",c)){
        if (getStatus() == SetCorners){
            e->accept();
            cornersDrawMode = DRAW_STAIGHT;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("round",c)){
        if (getStatus() == SetCorners){
            e->accept();
            cornersDrawMode = DRAW_RADIUS;
            updateOptions();
            setStatus(SetPosition);
        }
    }
    else if (checkCommand("snapcorner",c)){
            e->accept();
            snapToCornerArcCenter = false;
            updateOptions();
            setStatus(SetPosition);
    }
    else if (checkCommand("snapshift",c)){
        e->accept();
        snapToCornerArcCenter = true;
        updateOptions();
        setStatus(SetPosition);
    }
    else{
        bool ok = false;
        double value = RS_Math::eval(c, &ok);
        e->accept();
        if (ok){
            switch (getStatus()) {
                case SetWidth: {
                    double w = std::abs(value);
                    if (w > RS_TOLERANCE){
                        width = w;
                        updateOptions();
                        setStatus(SetPosition);
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
                        setStatus(SetPosition);
                    } else {
                        RS_DIALOGFACTORY->commandMessage(tr("Invalid height, it should be non-zero positive"));
                        updateMouseButtonHints();
                    }
                    break;
                }
                case SetAngle: {
                    double a = value;
                    if (std::abs(value) < RS_TOLERANCE_ANGLE){
                        a = 0.0;
                    }
                    angle = a;
                    updateOptions();
                    setStatus(SetPosition);
                    break;
                }
                case SetRadius:
                    double r = std::abs(value);
                    if (std::abs(r) < RS_TOLERANCE){
                        r = 0.0;
                    }
                    radius = r;
                    updateOptions();
                    setStatus(SetPosition);
                    break;
            }
        }
        else {
            RS_DIALOGFACTORY->commandMessage(tr("Invalid value"));
        }
    }
}


QStringList LC_ActionDrawLineRectangleFixed::getAvailableCommands() {
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
        case SetPosition:
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

void LC_ActionDrawLineRectangleFixed::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPosition:
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



void LC_ActionDrawLineRectangleFixed::setSnapPointMode(int value){
    snapMode = value;
    drawPreviewForLastPoint();
}
void LC_ActionDrawLineRectangleFixed::setCornersMode(int value){
    cornersDrawMode = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::setAngle(double value){
    angle = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::setWidth(double value){
    width = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::setHeight(double value){
    height = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::setRadius(double value){
    radius = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::setLengthX(double value){
    bevelX = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::setLengthY(double value){
    bevelY = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::setSnapToCornerArcCenter(bool value){
    snapToCornerArcCenter = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawLineRectangleFixed::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_LineRectangleFixedOptions>(nullptr);
}

