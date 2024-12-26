#include "rs_math.h"
#include "rs_actiondrawlinepolygon2.h"
#include "rs_actioninterface.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_coordinateevent.h"
#include "rs_point.h"
#include "rs_creation.h"
#include "rs_graphicview.h"
#include "rs_dialogfactory.h"
#include "rs_polyline.h"
#include <QMouseEvent>
#include "lc_actiondrawlinepolygonbase.h"
#include "qg_linepolygonoptions.h"
#include "rs_commandevent.h"
#include "lc_linemath.h"

LC_ActionDrawLinePolygonBase::LC_ActionDrawLinePolygonBase( const char *name, RS_EntityContainer &container,
                                                            RS_GraphicView &graphicView,
                                                            RS2::ActionType actionType)
    :RS_PreviewActionInterface(name, container,graphicView,actionType),number(3), pPoints(std::make_unique<Points>()), lastStatus(SetPoint1){}

LC_ActionDrawLinePolygonBase::~LC_ActionDrawLinePolygonBase() = default;

void LC_ActionDrawLinePolygonBase::doTrigger() {
    if (document != nullptr) {
        PolygonInfo polygonInfo;
        preparePolygonInfo(polygonInfo, pPoints->point2);
        RS_Polyline *polyline = createShapePolyline(polygonInfo, false);
        if (polyline != nullptr) {
            undoCycleStart();
            RS_Graphic* graphic = graphicView->getGraphic();
            RS_Layer* layer;
            RS_Pen pen = document->getActivePen();
            layer = graphic->getActiveLayer();

            if (createPolyline) {
                polyline->setLayer(layer);
                polyline->setPen(pen);
                polyline->reparent(container);
                container->addEntity(polyline);
                undoableAdd(polyline);
            }
            else{
                for (RS_Entity *entity = polyline->firstEntity(RS2::ResolveAll); entity;
                     entity = polyline->nextEntity(RS2::ResolveAll)) {
                    if (entity != nullptr){
                        auto *clone = entity->clone(); // use clone for safe deletion of polyline
                        clone->setPen(pen);
                        clone->setLayer(layer);
                        clone->reparent(container);
                        container->addEntity(clone);
                        undoableAdd(clone);
                    }
                }
                delete polyline; //don't need it anymore
            }
            undoCycleEnd();
        }

        if (completeActionOnTrigger) {
            setStatus(-1);
        }
    }
}

void LC_ActionDrawLinePolygonBase::mouseMoveEvent(QMouseEvent* e) {
    deletePreview();
    RS_DEBUG->print("RS_ActionDrawLinePolygon2::mouseMoveEvent begin");
    RS_Vector mouse = snapPoint(e);
    switch (getStatus()) {
        case SetPoint1: {
            trySnapToRelZeroCoordinateEvent(e);
            break;
        }
        case SetPoint2: {
            if (pPoints->point1.valid){
                mouse = getSnapAngleAwarePoint(e, pPoints->point1, mouse, true);
                createPolygonPreview(mouse);
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(pPoints->point1);
                    previewRefLine(pPoints->point1,mouse);
                    previewRefSelectablePoint(mouse);
                    previewAdditionalReferences(mouse);
                }
            }

            break;
        }
        default:
            break;
    }
    drawPreview();
}

void LC_ActionDrawLinePolygonBase::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector coord = snapPoint(e);
    if (status == SetPoint2){
        coord = getSnapAngleAwarePoint(e, pPoints->point1, coord);
        completeActionOnTrigger = isControl(e);
    }
    fireCoordinateEvent(coord);
}

void LC_ActionDrawLinePolygonBase::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDrawLinePolygonBase::onCoordinateEvent(int status,  [[maybe_unused]]bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPoint1: {
            pPoints->point1 = mouse;
            setStatus(SetPoint2);
            moveRelativeZero(mouse);
            break;
        }
        case SetPoint2: {
            pPoints->point2 = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawLinePolygonBase::doProcessCommand(int status, const QString &c) {
    bool accept = false;
    if (checkCommand("number", c)){
        deletePreview();
        lastStatus = (Status) status;
        setStatus(SetNumber);
        accept = true;
    }
    else if (checkCommand("radius", c)){
        deletePreview();
        roundedCorners = true;
        lastStatus = (Status) status;
        setStatus(SetRadius);
        accept = true;
    }
    else if (checkCommand("str", c)){
        roundedCorners = false;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("usepoly", c)){
        createPolyline = true;
        updateOptions();
        accept = true;
    }
    else if (checkCommand("nopoly", c)){
        createPolyline = false;
        updateOptions();
        accept = true;
    }
    else{
        // process entered value
        bool ok = false;
        double value = RS_Math::eval(c, &ok);
        if (ok){
            switch (getStatus()) {
                case SetNumber:{ // handling number of rays
                    // fixme - check range to conform to UI
                    if ((value >= 3) && (value <= 10000)){
                        number = value;
                        updateOptions();
                        accept = true;
                        setStatus(lastStatus);
                    }
                    else{
                        commandMessage(tr("Not a valid number. Try 1..9999"));
                    }
                    break;
                }
                case SetRadius: {
                    if (value > 0 && LC_LineMath::isMeaningful(value)) {
                        roundingRadius = value;
                        updateOptions();
                        setStatus(lastStatus);
                        accept = true;
                    }
                    else{
                        commandMessage(tr("Invalid value of rounding radius"));
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else {
            accept = false;
            commandMessage(tr("Not a valid expression"));
        }
    }
    return accept;
}

QStringList LC_ActionDrawLinePolygonBase::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetPoint1:
        case SetPoint2:
            cmd += command("number");
            cmd += command("radius");
            cmd += command("str");
            cmd += command("usepoly");
            cmd += command("nopoly");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionDrawLinePolygonBase::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(getPoint1Hint(), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(getPoint2Hint(), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetNumber:
            updateMouseWidgetTRBack(tr("Enter number:"));
            break;
        case SetRadius:
            updateMouseWidgetTRBack(tr("Enter rounding radius:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

QString LC_ActionDrawLinePolygonBase::getPoint1Hint() const { return tr("Specify center"); }

RS2::CursorType LC_ActionDrawLinePolygonBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawLinePolygonBase::createOptionsWidget(){
    return new QG_LinePolygonOptions();
}

#define DRAW_JOIN_POINTS_ON_PREVIEW true

void LC_ActionDrawLinePolygonBase::createPolygonPreview(const RS_Vector &mouse) {
    PolygonInfo polygonInfo;
    preparePolygonInfo(polygonInfo, mouse);
    RS_Polyline* polyline = createShapePolyline(polygonInfo, true);
    if (polyline != nullptr){
        previewEntity(polyline);
        if (infoCursorOverlayPrefs->enabled && infoCursorOverlayPrefs->showEntityInfoOnCreation) {
            LC_InfoMessageBuilder msg{};
            msg.add(tr("To be created:"), tr("Polygon"));
            msg.add(tr("Center:"), formatVector(polygonInfo.centerPoint));
            msg.add(tr("Start angle:"), formatAngle(polygonInfo.startingAngle));
            msg.add(tr("Radius:"), formatLinear(polygonInfo.vertexRadius));
            msg.add(tr("Radius Inner:"), formatLinear(polygonInfo.innerRadius));
            appendInfoCursorZoneMessage(msg.toString(), 2, false);
        }
    }
}

#define DEBUG_OUTPUT_DRAW_ROUNDING_NO

RS_Polyline *LC_ActionDrawLinePolygonBase::createShapePolyline(PolygonInfo &polygonInfo, bool preview) {
    auto *result = new RS_Polyline();
    // bulge that should be used for corner rounding
    double bulge = 0.0;

    // angle on circle that takes one segment (from one vertex to another vertex)
    double segmentAngle = 2. * M_PI / number;

    // distance to vertex if is not rounded, or distance to rounding joint point if rounded
    double vertexDistance = polygonInfo.vertexRadius;

    // angle to outer vertex
    double vertexStartAngle = polygonInfo.startingAngle;

    RS_Vector centerPoint = polygonInfo.centerPoint;

    bool drawRounded = roundedCorners;
    if (roundedCorners && LC_LineMath::isMeaningful(roundingRadius)) {

        // if there is rounding, 2 additional vertexes is added for rounding joints. This is the angle that corrects outer angle for proper position of
        // rounding join vertexes
        double vertexAngleCorrection = 0.0;
        // here we do calculation of one segment of start. For simplicity of calculations and debugging, that segment is positioned in
        // such way that the line from center to vertex is parallel to x-axis

        // calculate initial positions of one base and 2 adjacent  vertexes
        RS_Vector plusVertex = centerPoint.relative(vertexDistance, segmentAngle);
        RS_Vector horizontalVertex = centerPoint.relative(vertexDistance, 0);
        RS_Vector minusVertex = centerPoint.relative(vertexDistance, -segmentAngle);

        // create parallel lines that we'll use for defining position of rounding join points
        // these lines are within star ray
        RS_LineData par1data = LC_LineMath::createParallel(plusVertex, horizontalVertex, -roundingRadius);
        RS_LineData par2data = LC_LineMath::createParallel(minusVertex, horizontalVertex, roundingRadius);

        // find intersection of parallel lines
        RS_Vector parallelsIntersection = LC_LineMath::getIntersectionLineLine(par1data.startpoint, par1data.endpoint, par2data.startpoint, par2data.endpoint);

        // rounding joint point will be intersection of line from one vertex to nearest vertex, and perpendicular from intersection point to that line
        RS_Vector jointPoint1 = LC_LineMath::getNearestPointOnInfiniteLine(parallelsIntersection, plusVertex, horizontalVertex);
        RS_Vector joinPoint2 = LC_LineMath::getNearestPointOnInfiniteLine(parallelsIntersection, minusVertex, horizontalVertex);

        bool doRoundedCornerCorrection = false;
        if (doRoundedCornerCorrection) {

            // here we'll do a correction for making rightmost point of radius rounding to be at snap point - so we'll move everything to right as we're on x-axis
            // without such correction, end of radius will be on the left side from current snap point, and it's actual position will be not obvious as
            // it will depend on ray size, angles, radius and so on.


            // we may draw rounding if intersection of rounding is not before center point (as otherwise, it creates funny yet hardly usable shape artefacts
            double distanceToIntersection = parallelsIntersection.x - centerPoint.x;
            double outerShift;
            if (parallelsIntersection.x > 0) {
                outerShift = vertexDistance - distanceToIntersection - roundingRadius;
            } else {
                outerShift = vertexDistance - roundingRadius + distanceToIntersection * 2;
            }

            parallelsIntersection.x = parallelsIntersection.x + outerShift; // just move to right

            // we can correct x only since here outer point is on x-axis
            jointPoint1.x = jointPoint1.x + outerShift;
            joinPoint2.x = joinPoint2.x + outerShift;

        }

        // this is the distance from center point to rounding join point

        double joinPointDistance = centerPoint.distanceTo(jointPoint1);

        // save the angle from center to our join point for later drawing. Here we don't consider the angle from center to vertex, as it is zero so far
        vertexAngleCorrection = centerPoint.angleTo(jointPoint1);

        drawRounded = vertexAngleCorrection < (segmentAngle * 0.5);

        if (drawRounded) {
            // now we define angles from parallels intersection point to rounding join points
            double jointAngle1 = parallelsIntersection.angleTo(jointPoint1);
            double jointAngle2 = parallelsIntersection.angleTo(joinPoint2);
            bool outerReversed = (RS_Math::getAngleDifference(jointAngle2, jointAngle1) > M_PI);

            // define angle for arc that is used for rounding  - the angle between rounding points
            if (outerReversed) std::swap(jointAngle2, jointAngle1);
            double arcAngleLength = RS_Math::correctAngle(jointAngle1 - jointAngle2);
            // full circle:
            if (std::abs(std::remainder(arcAngleLength, 2. * M_PI)) < RS_TOLERANCE_ANGLE) {
                arcAngleLength = 2 * M_PI;
            }

            // define bulge for polyline that will be used for outer rounding
            bulge = std::tan(std::abs(arcAngleLength) / 4.0);
            bulge = (outerReversed ? -bulge : bulge);

            if (jointAngle1 > jointAngle2) {
                bulge = -bulge;
            }

#ifdef DEBUG_OUTPUT_DRAW_ROUNDING
            if (preview) {
                // this is internal drawing used for debugging that illustrates calculation logic for outer ray rounding
                // we visualize all entities used in calculations during preview mode
                previewLine(plusVertex, horizontalVertex);
                previewLine(minusVertex, horizontalVertex);
                previewLine(par1data.startpoint, par1data.endpoint);
                previewLine(par2data.startpoint, par2data.endpoint);
                previewArc(RS_ArcData(parallelsIntersection,
                                      roundingRadius,
                                      jointAngle2, jointAngle1,
                                      outerReversed));
                auto *centerToInner = new RS_Line(centerPoint, plusVertex);
                previewEntity(centerToInner);

                previewLine(parallelsIntersection, jointPoint1);
                previewLine(parallelsIntersection, joinPoint2);

                previewPoint(parallelsIntersection);
                previewPoint(jointPoint1);
                previewPoint(joinPoint2);
            }
#endif

            // now all preparations completed, and we'll create shape

            // the very first vertex from which shape creation begins.
            RS_Vector startingVertex;

            // create all necessary segments one by one. Here we'll draw 2 edges - from outer point to inner and from inner point to next outer point of rays
              for (int i=0; i < number; ++i) {

                   double baseVertexAngle = vertexStartAngle + i * segmentAngle;

                   // definition of vertexes. Vertexes rounding join points
                   RS_Vector start = centerPoint.relative(joinPointDistance, baseVertexAngle + vertexAngleCorrection);
                   RS_Vector end = centerPoint.relative(joinPointDistance, baseVertexAngle + segmentAngle - vertexAngleCorrection);

                   if (i == 0){ // store starting vertex for later use
                       startingVertex = start;
                   }

                   result->addVertex(start);
                   result->addVertex(end);
                   result->setNextBulge(bulge);
                   if (preview && DRAW_JOIN_POINTS_ON_PREVIEW && showRefEntitiesOnPreview){
                       // potential visualization of rounding point
                       previewRefPoint(end);
                       previewRefPoint(start);
                       // todo - think whether vertex point should be shown there. It's not consistent to star drawing..
                       previewRefPoint(centerPoint.relative(vertexDistance, baseVertexAngle));
                   }
               }
               // complete polyline and close it to starting vertex
               result->addVertex(startingVertex);
        }
    }
    if (!drawRounded){
        for (int i = 0; i <= number; ++i) {
            RS_Vector const &vertex = centerPoint +
                                      RS_Vector::polar(vertexDistance, vertexStartAngle + i * segmentAngle);

            result->addVertex(vertex, bulge, false);
        }
    }

    // update inner radius
    RS_Vector const vertex0 = centerPoint.relative(vertexDistance, vertexStartAngle);
    RS_Vector const vertex1 = centerPoint.relative(vertexDistance, vertexStartAngle + segmentAngle);

    RS_Vector midPoint = (vertex0+vertex1)*0.5;
    double innerRadius = centerPoint.distanceTo(midPoint);

    polygonInfo.innerRadius = innerRadius;

    return result;
}
