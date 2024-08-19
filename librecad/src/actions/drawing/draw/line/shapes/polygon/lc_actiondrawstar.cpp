/****************************************************************************
**
* Action that draws a star with specified amount of rays and optional rounding
* of rays

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

#include "lc_actiondrawstar.h"
#include "lc_linemath.h"
#include "lc_staroptions.h"
#include "rs_math.h"
#include "rs_polyline.h"
#include "rs_previewactioninterface.h"

LC_ActionDrawStar::LC_ActionDrawStar(RS_EntityContainer &container, RS_GraphicView &graphicView)
   :LC_AbstractActionWithPreview("Draw Star", container, graphicView){
   actionType = RS2::ActionDrawStar;
}

int LC_ActionDrawStar::doGetStatusForInitialSnapToRelativeZero(){
    return SetCenter; // we'll snap center to relative zero if center is not set
}

void LC_ActionDrawStar::doInitialSnapToRelativeZero(RS_Vector vector){
    // snap center to relative zero and change status
    centerPoint = vector;
    setMainStatus(SetOuterPoint);
}

bool LC_ActionDrawStar::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status){
    return status != SetCenter;
}

RS_Vector LC_ActionDrawStar::doGetMouseSnapPoint(QMouseEvent *e){
    int status = getStatus();
    RS_Vector snap = LC_AbstractActionWithPreview::doGetMouseSnapPoint(e);
    if (status == SetOuterPoint || status == SetInnerPoint){
        snap = getSnapAngleAwarePoint(e, centerPoint, snap, isMouseMove(e));
    }
    return snap;
}

/**
 * preview generation
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionDrawStar::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    RS_Polyline* polyline = createShapePolyline(snap, list, status, true);
    addPolylineToEntitiesList(polyline,  list, true);
}
/**
 * back processing
 * @param pEvent
 * @param status
 */
void LC_ActionDrawStar::doBack(QMouseEvent *pEvent, int status){
    if (status >SetInnerPoint){
        restoreMainStatus();
    }
    else{
        LC_AbstractActionWithPreview::doBack(pEvent, status);
    }
}
/**
 * left mouse click processing
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawStar::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status, const RS_Vector &snapPoint){
    onCoordinateEvent(status, false, snapPoint);
}

void LC_ActionDrawStar::onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) {
    switch (status){
        case SetCenter: // setting center of star
           centerPoint = coord;
           moveRelativeZero(coord);
            setMainStatus(SetOuterPoint);
           break;
        case SetOuterPoint: // setting outer point
           outerPoint = coord;
            setMainStatus(SetInnerPoint);
           break;
        case SetInnerPoint: // setting inner point
           innerPoint = coord;
           trigger();
            setMainStatus(SetCenter);
           break;
        case SetRadiuses: // setting rounding radius's
            if (isZero){
                innerRadiusRounded = false;
                outerRadiusRounded = false;
            }
            else {
                double innerR = coord.y;
                if (LC_LineMath::isMeaningful(innerR)){
                    innerRadius = innerR;
                    innerRadiusRounded = true;
                }
                else{
                    innerRadiusRounded = false;
                }
                double outerR = coord.x;
                if (LC_LineMath::isMeaningful(outerR)){
                    outerRadius = outerR;
                    outerRadiusRounded = true;
                }
                else{
                    outerRadiusRounded = false;
                }
            }
            updateOptions();
            restoreMainStatus();
            break;
        default:
            break;
    }
}

/**
 * Commands processing
 * @param e
 * @param c
 * @return
 */
bool LC_ActionDrawStar::doProcessCommand([[maybe_unused]]int status, const QString &c){
    bool processed = true;
    if (checkCommand("nopoly",c)){ // don't create polyline
        createPolyline = false;
        updateOptions();
    }
    else if (checkCommand("usepoly",c)){ // draw star by single polyline
        createPolyline = true;
        updateOptions();
    }
    else if (checkCommand("number",c)){ // set number of rays
        setStatus(SetRays);
    }
    else if (checkCommand("radius",c)){ // setting rounding radiuses
        setStatus(SetRadiuses);
    }
    else if (checkCommand("sym",c)){ // draw symmetric star
        symmetric = true;
        updateOptions();
    }
    else if (checkCommand("nosym",c)){ // draw asymmetric star
        symmetric = false;
        updateOptions();
    }
    else{
        // process entered value
        bool ok = false;
        int value = RS_Math::eval(c, &ok);
        if (ok){
            switch (getStatus()) {
                case SetRays:{ // handling number of rays
                    if ((value >= STAR_MIN_RAYS) && (value <= STAR_MAX_RAYS)){
                        raysNumber  = value;
                        updateOptions();
                        restoreMainStatus();
                    }
                    else{
                        commandMessage(tr("Invalid rays number, should be in range [3..99]"));
                    }
                    break;
                }
            }
        }
        else {
            processed = false;
            commandMessage(tr("Invalid value"));
        }
    }
    return processed;
}

bool LC_ActionDrawStar::doCheckMayTrigger(){
    return getStatus() == SetInnerPoint; // may trigger if inner point set
}

/**
 * entities created for trigger
 * @param list
 */
void LC_ActionDrawStar::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    RS_Polyline* polyline = createShapePolyline(innerPoint, list, SetInnerPoint, false);
    addPolylineToEntitiesList(polyline, list, false);
}

/**
 * either adds polyline at once to list of entities, or, if necessary, "expand" it to individual entities
 * and adds them to the list
 * @param polyline
 * @param list
 * @param preview
 */
void LC_ActionDrawStar::addPolylineToEntitiesList(RS_Polyline *polyline, QList<RS_Entity *> &list, bool preview){
    if (polyline != nullptr){
        if (preview){
            list << polyline;
        }
        else{
            if (createPolyline){
                list << polyline;
            }
            else{
                for (RS_Entity *entity = polyline->firstEntity(RS2::ResolveAll); entity;
                     entity = polyline->nextEntity(RS2::ResolveAll)) {
                    if (entity != nullptr){
                        RS_Entity *clone = entity->clone(); // use clone for safe deletion of polyline
                        clone->reparent(container);
                        list << clone;
                    }
                }
                delete polyline; //don't need it anymore
            }
        }
    }
}

// defines whether debug visualization of rounding calculation logic will be included into preview.(DEBUG_OUTPUT_DRAW_OUTER)
// THESE SHOULD NOT BE DEFINED IN PRODUCTION MODE!!
#define DEBUG_OUTPUT_DRAW_OUTER_NO
#define DEBUG_OUTPUT_DRAW_INNER_NO

// basically, that's convenient for debug purposes, but also provides the user more clear indication
// why, for example, roundings disappears - so probably it's better to have this mode enabled
#define DRAW_JOIN_POINTS_ON_PREVIEW true


/**
 * Major function that creates start shape
 * @param snap snap point
 * @param list list to which start will be added
 * @param status current status
 * @param preview are we in preview mode or not
 * @return polyline that describes star
 */

RS_Polyline *LC_ActionDrawStar::createShapePolyline(RS_Vector &snap, QList<RS_Entity *> &list, int status, bool preview){

    // distance to outer vertex if outer is not rounded, or distance to rounding joint point if rounded
    double outerDistance = 0.0;
    // angle to outer vertex
    double outerAngle = 0.0;

    // distance to inner vertex if no inner rounding, or distance to inner joint point of rounding
    double innerDistance = 0.0;

    // angle to inner vertex
    double innerAngle= 0.0;

    // angle on circle that takes one segment (from one outer vertex to another outer vertex)
    double segmentAngle = 2 * M_PI / raysNumber;

    // angle on circle between outer vertex and inner vertex
    double angleBetweenOuterAndInner = 0.0;

    // initial calculation of points - will be fine for non-rounded star
    switch (status){
        case SetCenter:
            return nullptr;
        case SetOuterPoint:
            // outer distance is distance to snap
            outerDistance = centerPoint.distanceTo(snap);
            // outer angle is angle from center point to snap position
            outerAngle = centerPoint.angleTo(snap);

            // just a half of segment
            angleBetweenOuterAndInner = segmentAngle / 2;

            // we don't know yet where the inner vertex will be located, so set it to arbitrary initial position
            innerDistance = outerDistance / 2;
            innerAngle = outerAngle + angleBetweenOuterAndInner;
            break;
        case SetInnerPoint:
            // we know the outer point fixed already, so rely on it
            outerDistance = centerPoint.distanceTo(outerPoint);
            outerAngle = centerPoint.angleTo(outerPoint);

            // determine distance to snap
            innerDistance = centerPoint.distanceTo(snap);
            if (symmetric){
                // for symmetric star, we just calculate based on outer position
                angleBetweenOuterAndInner = segmentAngle / 2;
                // angle to inner is actually defined by the position and angle to outer
                innerAngle = outerAngle + angleBetweenOuterAndInner;
            }
            else {
                // for non-symmetric start, we rely on snap point
                innerAngle = centerPoint.angleTo(snap);
                angleBetweenOuterAndInner = RS_Math::getAngleDifference(outerAngle, innerAngle, false);
            }
            break;
        default:
            break;
    }


    // bulge that should be used for outer rounding
    double outerBulge = 0.0;

    // if there is rounding, 2 additional vertexes is added for rounding joints. This is the angle that corrects outer angle for proper position of
    // rounding join vertexes
    double outerAngleCorrection = 0.0;

    // do calculation for case when outer edge is rounded
    // determining rounding bulge and correction for outer rounding

    bool mayDrawOuterRound = outerRadiusRounded;
    if (outerRadiusRounded){

        // here we do calculation of one segment of start. For simplicity of calculations and debugging, that segment is positioned in
        // such way that the line from center to outer vertex is parallel to x-axis

        // calculate initial positions of one outer and 2 adjusent inner vertexes
        RS_Vector inner = centerPoint.relative(innerDistance, angleBetweenOuterAndInner);
        RS_Vector outer = centerPoint.relative( outerDistance, 0);
        RS_Vector inner1 = centerPoint.relative( innerDistance, -angleBetweenOuterAndInner);

        // create parallel lines that we'll use for defining position of rounding join points
        // these lines are within star ray
        RS_LineData par1data = LC_LineMath::createParallel(inner, outer, -outerRadius);
        RS_LineData par2data = LC_LineMath::createParallel(inner1, outer, outerRadius);

        // find intersection of parallel lines
        RS_Vector outerIntersection = LC_LineMath::getIntersectionLineLine(par1data.startpoint, par1data.endpoint, par2data.startpoint, par2data.endpoint);

        // rounding joint point will be intersection of line from inner vertex to outer vertex, and perpendicular from intersection point to that line
        RS_Vector outerJoint1 = LC_LineMath::getNearestPointOnInfiniteLine(outerIntersection, inner, outer);
        RS_Vector outerJoin2 = LC_LineMath::getNearestPointOnInfiniteLine(outerIntersection, inner1, outer);

        // here we'll do a correction for making rightmost point of radius rounding to be at snap point - so we'll move everything to right as we're on x-axis
        // without such correction, end of radius will be on the left side from current snap point, and it's actual position will be not obvious as
        // it will depend on ray size, angles, radius and so on.


        // we may draw rounding if intersection of rounding is not before center point (as otherwise, it creates funny yet hardly usable shape artefacts
        double distanceToIntersection = outerIntersection.x - centerPoint.x;
        double outerShift;
        if (outerIntersection.x > 0){
            outerShift = outerDistance - distanceToIntersection - outerRadius;
        }
        else{
            outerShift = outerDistance - outerRadius + distanceToIntersection * 2;
        }

        outerIntersection.x = outerIntersection.x + outerShift; // just move to right

        // we can correct x only since here outer point is on x-axis
        outerJoint1.x = outerJoint1.x + outerShift;
        outerJoin2.x = outerJoin2.x + outerShift;

        // this is the distance from center point to rounding join point

        double distanceToJoin1 = centerPoint.distanceTo(outerJoint1);

        // correction of inner distance if outer is rounded - in order to stay with initial angle of line between outer and inner point
        RS_Vector outerP3 = LC_LineMath::getNearestPointOnInfiniteLine(outerJoint1, inner, outer);
        double d = outerP3.distanceTo(outerJoint1);

        // this is hypotenuse of corresponding triangle, and angle is defined by solving intersections and triangle angles
        double innerDistanceCorrection = d / std::cos(M_PI_2 - angleBetweenOuterAndInner - (M_PI - outer.angleTo(inner)));
        double correctedInnerDistance = innerDistance + innerDistanceCorrection;

        mayDrawOuterRound = distanceToJoin1 > correctedInnerDistance;

        if (mayDrawOuterRound){

            innerDistance = correctedInnerDistance;

            // now we define angles from parallels intersection point to rounding join points
            double outerAng1 = outerIntersection.angleTo(outerJoint1);
            double outerAng2 = outerIntersection.angleTo(outerJoin2);
            bool outerReversed = (RS_Math::getAngleDifference(outerAng2, outerAng1) > M_PI);

            // define angle for arc that is used for rounding  - the angle between rounding points
            if (outerReversed) std::swap(outerAng2, outerAng1);
            double arcAngleLength = RS_Math::correctAngle(outerAng1 - outerAng2);
            // full circle:
            if (std::abs(std::remainder(arcAngleLength, 2. * M_PI)) < RS_TOLERANCE_ANGLE){
                arcAngleLength = 2 * M_PI;
            }

            // define bulge for polyline that will be used for outer rounding
            double bulge = std::tan(std::abs(arcAngleLength) / 4.0);
            outerBulge = (outerReversed ? -bulge : bulge);

            if (outerAng1 > outerAng2){
                outerBulge = -outerBulge;
            }

            outerDistance = distanceToJoin1;

            // save the angle from center to out join point for later drawing. Here we don't consider the angle from center to outer, as it is zero
            outerAngleCorrection = centerPoint.angleTo(outerJoint1);

#ifdef DEBUG_OUTPUT_DRAW_OUTER
            if (preview){
                // this is internal drawing used for debugging that illustrates calculation logic for outer ray rounding
                // we visualize all entities used in calculations during preview mode
                RS_Line *line1 = new RS_Line(inner, outer);
                RS_Line *line2 = new RS_Line(inner1, outer);
                RS_Entity *par1 = new RS_Line(par1data.startpoint, par1data.endpoint);
                RS_Entity *par2 = new RS_Line(par2data.startpoint, par2data.endpoint);
                RS_Arc *outerArc = new RS_Arc(nullptr,
                                              RS_ArcData(outerIntersection,
                                                         outerRadius,
                                                         outerAng2, outerAng1,
                                                         outerReversed));
                RS_Line *centerToInner = new RS_Line(centerPoint, inner);
                list << centerToInner;

                list << new RS_Line(outerIntersection, outerJoint1);
                list << new RS_Line(outerIntersection, outerJoin2);

                list << line1;
                list << line2;
                list << par1;
                list << par2;
                list << outerArc;

                RS_Point *intersection = new RS_Point(nullptr, outerIntersection);
                list << intersection;

                RS_Point *intersectionOriginal = new RS_Point(nullptr, outerIntersectionOriginal);
                list << intersectionOriginal;

                RS_Point *p3 = new RS_Point(nullptr, outerP3);
                list << p3;
            }
#endif
        }
    }

    // now we need to perform calculation of rounding  for inner vertex
    double innerBulge = 0.0; // bulge for inner vertex rounding

    // non-symmetric star may require different distance and angles for rounding points

    double innerAngleCorrection1 = 0.0; // correction of angle for first joint point
    double innerAngleCorrection2 = 0.0; // correction of angle for second join point
    double innerDistance2 = innerDistance; // inner distance for second join point

    bool mayDrawInnerRound = innerRadiusRounded;
    if (innerRadiusRounded){

        // prepare calculation ray (one inner vertex and 2 nearest outer vertexes

        RS_Vector outer = centerPoint.relative(outerDistance, outerAngleCorrection);
        RS_Vector inner = centerPoint.relative( innerDistance, angleBetweenOuterAndInner);
        RS_Vector outer1 = centerPoint.relative(outerDistance, segmentAngle - outerAngleCorrection);

        // parallel lines for inner rounding - they are above of ray lines
        RS_LineData par1data = LC_LineMath::createParallel(inner, outer, innerRadius);
        RS_LineData par2data = LC_LineMath::createParallel(inner, outer1, -innerRadius);

        // determining rounding bulge and correction for inner angle

        // first define where parallel lines intersects - intersection point will be the center of rounding arc
        RS_Vector innerIntersection = LC_LineMath::getIntersectionLineLine(par1data.startpoint, par1data.endpoint, par2data.startpoint, par2data.endpoint);

        // these are joint points for rounding
        RS_Vector innerJoint1 = LC_LineMath::getNearestPointOnInfiniteLine(innerIntersection, inner, outer);
        RS_Vector innerJoint2 = LC_LineMath::getNearestPointOnInfiniteLine(innerIntersection, inner, outer1);

        // define angles from intersection point to rounding joins
        double innerAng1 = innerIntersection.angleTo(innerJoint1);
        double innerAng2 = innerIntersection.angleTo(innerJoint2);

        // calculate angle of rounding arc
        bool innerReversed = (RS_Math::getAngleDifference(innerAng2, innerAng1) > M_PI);
        if (innerReversed) std::swap(innerAng2,innerAng1);
        double arcAngleLength = RS_Math::correctAngle(innerAng1-innerAng2);
        // full circle:
        if (std::abs(std::remainder(arcAngleLength, 2.*M_PI))<RS_TOLERANCE_ANGLE) {
            arcAngleLength = 2*M_PI;
        }

        // distance from center to rounding point
        double distanceToInnerJoint1 = centerPoint.distanceTo(innerJoint1);

        // we need to ensure that center of rounding point is within outer distance, so we actually can do rounding.
        // without such check - on larger radiuses there will be drawing artefacts
        mayDrawInnerRound = distanceToInnerJoint1 < outerDistance;

        if (mayDrawInnerRound){
            // determine bulde for rounding arc
            double bulge = std::tan(std::abs(arcAngleLength) / 4.0);
            innerBulge = (innerReversed ? -bulge : bulge);

            if (innerAng1 > innerAng2){
                innerBulge = -innerBulge;
            }

            // determine distances and agles for rounding joint points
            innerDistance = distanceToInnerJoint1;
            innerDistance2 = centerPoint.distanceTo(innerJoint2);
            innerAngleCorrection1 = centerPoint.angleTo(innerJoint1) - centerPoint.angleTo(inner);
            innerAngleCorrection2 = centerPoint.angleTo(innerJoint2) - centerPoint.angleTo(inner);

#ifdef DEBUG_OUTPUT_DRAW_INNER
            if (preview){
                // this is internal drawing used for debugging that illustrates calculation logic for inner ray rounding
                // we visualize all entities used in calculations during preview mode
                RS_Line *line1 = new RS_Line(inner, outer);
                RS_Line *line3 = new RS_Line(inner, outer1);
                RS_Entity *par1 = new RS_Line(par1data.startpoint, par1data.endpoint);
                RS_Entity *par2 = new RS_Line(par2data.startpoint, par2data.endpoint);

                list << new RS_Line(innerIntersection, innerJoint1);
                list << new RS_Line(innerIntersection, innerJoint2);

                list << line1;
                list << line3;
                list << par1;
                list << par2;
                RS_Line *centerToInner = new RS_Line(centerPoint, inner);
                list << centerToInner;

                RS_Arc *innerArc = new RS_Arc(nullptr,
                                              RS_ArcData(innerIntersection,
                                                         innerRadius,
                                                         innerAng2, innerAng1,
                                                         innerReversed));

                list << innerArc;
            }
#endif
        }
    }

    // now all preparations completed, and we'll create shape
    auto* polyline = new RS_Polyline(container);

    // the very first vertex from which shape creation begins.
    RS_Vector startingVertex;

    // create all necessary segments one by one. Here we'll draw 2 edges - from outer point to inner and from inner point to next outer point of rays
    for (int i=0; i < raysNumber; ++i) {

        double baseInnerAngle = innerAngle + i * segmentAngle;
        double baseOuterAngle = outerAngle + i * (segmentAngle);

        // definition of vertexes. Vertexes will be either edge points or rounding join points
        RS_Vector outer1 = centerPoint.relative(outerDistance, baseOuterAngle + outerAngleCorrection);
        RS_Vector inner1 = centerPoint.relative(innerDistance, baseInnerAngle + innerAngleCorrection1);
        RS_Vector inner2 = centerPoint.relative( innerDistance2, baseInnerAngle + innerAngleCorrection2);
        RS_Vector outer2 = centerPoint.relative( outerDistance, baseOuterAngle + segmentAngle - outerAngleCorrection);

        if (i == 0){ // store starting vertex for later use
            startingVertex = outer1;
        }

        polyline->addVertex(outer1);
        polyline->addVertex(inner1);
        if (mayDrawInnerRound){ // add bulge and second rounding join
            polyline->setNextBulge(innerBulge);
            polyline->addVertex(inner2);
        }
        if (preview && DRAW_JOIN_POINTS_ON_PREVIEW && showRefEntitiesOnPreview){
            // potential visualization of rounding point
            createRefPoint(inner1, list);
            createRefPoint(inner2, list);
            createRefPoint(outer1, list);
            createRefPoint(outer2, list);
        }
        if (mayDrawOuterRound){ // handle outer rounding
            polyline->addVertex(outer2);
            polyline->setNextBulge(outerBulge);
        }
    }

    // complete polyline and close it to starting vertex
    polyline->addVertex(startingVertex);

    return polyline;
}

/**
 * available commands list *
 * @return
 */
QStringList LC_ActionDrawStar::getAvailableCommands(){
    QStringList cmd;
    cmd += command("usepoly");
    cmd += command("nopoly");
    cmd += command("radius");
    cmd += command("number");
    cmd += command("sym");
    cmd += command("nosym");
    return cmd;
}

void LC_ActionDrawStar::updateMouseButtonHints(){
    switch (getStatus()){
        case SetCenter:
            updateMouseWidgetTRCancel(tr("Set center point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetOuterPoint:
            updateMouseWidgetTRBack(tr("Set outer point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetInnerPoint:
            updateMouseWidgetTRBack(tr("Set inner point"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetRadiuses:
            updateMouseWidgetTRBack(tr("Set rounding radiuses (outer, inner)"));
            break;
        case SetRays:
            updateMouseWidgetTRBack(tr("Set rays number"));
            break;
        default:
            LC_AbstractActionWithPreview::updateMouseButtonHints();
            break;
    }
}

void LC_ActionDrawStar::setRadiusOuter(double d){
   outerRadius = d;
   drawPreviewForLastPoint();
}

void LC_ActionDrawStar::setRadiusInner(double d){
  innerRadius = d;
   drawPreviewForLastPoint();
}

void LC_ActionDrawStar::setRaysNumber(int i){
    raysNumber = i;
    drawPreviewForLastPoint();
}

void LC_ActionDrawStar::setPolyline(bool value){
  createPolyline = value;
}

void LC_ActionDrawStar::setOuterRounded(bool value){
    outerRadiusRounded = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawStar::setInnerRounded(bool value){
    innerRadiusRounded = value;
    drawPreviewForLastPoint();
}

void LC_ActionDrawStar::setSymmetric(bool value){
   symmetric = value;
   drawPreviewForLastPoint();
}

LC_ActionOptionsWidget* LC_ActionDrawStar::createOptionsWidget(){
    return new LC_StarOptions();
}
