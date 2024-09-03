/****************************************************************************
**
* Action that draws specified amount of ticks for line or circle or arc
* with specified distance, angle and size and may divide original entity by
* ticks if necessary

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

#include <QMouseEvent>
#include <cmath>

#include "rs_line.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_point.h"
#include "rs_math.h"
#include "lc_linemath.h"
#include "lc_actiondrawslicedivide.h"
#include "lc_slicedivideoptions.h"
#include "lc_abstractactionwithpreview.h"

namespace {
    //list of entity types supported by current action
    const EntityTypeList sliceDivideLineEntityTypeList = {RS2::EntityLine};
    const EntityTypeList sliceDivideCircleEntityTypeList = {RS2::EntityArc, RS2::EntityCircle};
}
// todo - think about free mode for selection of tick length... not clear how to do this in convenient way yet
// todo - think whether dividing arc/circle for fixed angle (similar to fixed length for lines is needed

LC_ActionDrawSliceDivide::LC_ActionDrawSliceDivide(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView,
    bool forCircle)
    :LC_AbstractActionWithPreview("Draw slice divide", container, graphicView){
    if (forCircle){
        actionType = RS2::ActionDrawSliceDivideCircle;
    }
    else {
        actionType = RS2::ActionDrawSliceDivideLine;
    }
}

bool LC_ActionDrawSliceDivide::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status){
    return status == SetEntity;
}
/**
 * determines which types of entities should be selectable for the action
 * @return
 */
EntityTypeList LC_ActionDrawSliceDivide::getCatchEntityTypeList() const{
    if (actionType == RS2::ActionDrawSliceDivideLine){
        return sliceDivideLineEntityTypeList;
    }
    else{
        return sliceDivideCircleEntityTypeList;
    }
}
/**
 * creating preview ticks lines
 * @param e  event
 * @param snap  snap point
 * @param list  list to create preview entities
 * @param status current status of the action
 */
void LC_ActionDrawSliceDivide::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    ticksData.clear();
    EntityTypeList catchEntityTypes = getCatchEntityTypeList();
    RS_Entity *en = catchModifiableEntity(e, catchEntityTypes);
    int optionsMode = SELECTION_NONE;
    if (en != nullptr){
        int rtti = en->rtti();

        // proceed suitable entities and calculate ticks data for it
        switch (rtti) {
            case RS2::EntityLine: {
                auto *lineEntity = dynamic_cast<RS_Line *>(en);
                highlightHover(en);
                prepareLineTicks(lineEntity);
                break;
            }
            case RS2::EntityArc: {
                auto *arcEntity = dynamic_cast<RS_Arc *>(en);
                highlightHover(en);
                prepareArcTicks(arcEntity);
                optionsMode = SELECTION_ARC;
                break;
            }
            case RS2::EntityCircle: {
                auto *circleEntity = dynamic_cast<RS_Circle *>(en);
                highlightHover(en);
                prepareCircleTicks(circleEntity);
                optionsMode = SELECTION_CIRCLE;
                break;
            }
            default:
                break;
        }

        // create lines for calculated ticks data
        uint createdTicksCount = ticksData.size();
        if (createdTicksCount > 0){
            bool hasTickLength = LC_LineMath::isMeaningful(tickLength);
            bool doDrawTicks = hasTickLength || doDivideEntity;

            if (doDrawTicks){
                for (uint i = 0; i < createdTicksCount; i++) {
                    TickData tick = ticksData.at(i);
                    if (tick.isVisible){
                        if (hasTickLength){ // create preview line for tick with non-zero length
                            auto tickLine = new RS_Line(tick.tickLine.startpoint, tick.tickLine.endpoint);
                            list << tickLine;
                        }
                        if (doDivideEntity) { // if tick length is zero - it is just divide mode, without ticks
                            // so on preview, we just indicate that we may have divide points
                            // even if tick is present - we'd better highlight division points
                            if (showRefEntitiesOnPreview) {
                                createRefPoint(tick.snapPoint, list);
                            }
                        }
                    }
                }
            }
        }
    }
    if (actionType == RS2::ActionDrawSliceDivideCircle){
        // update options widget for
        updateOptionsUI(optionsMode);
    }
}

/**
 * Conditions for triggering action
 * @return
 */
bool LC_ActionDrawSliceDivide::doCheckMayTrigger(){
    bool result = false;
    if (getStatus() == SetEntity){
        if (entity != nullptr) {
            int entityRtti = entity->rtti();
            switch (entityRtti) {
                case RS2::EntityLine:
                    result = actionType == RS2::ActionDrawSliceDivideLine;
                    break;
                case RS2::EntityArc:
                case RS2::EntityCircle: {
                    result = actionType == RS2::ActionDrawSliceDivideCircle;
                    break;
                }
                default:
                    break;
            }
        }
    }
    return result;
}

bool LC_ActionDrawSliceDivide::isSetActivePenAndLayerOnTrigger(){
    return false; // this action will handle attributes (as if there is divide, we'll use original attributes for created entities)
}

void LC_ActionDrawSliceDivide::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    ticksData.clear();
    int rtti = entity->rtti();
    RS_Entity *entityToDelete = nullptr;
    switch (rtti) {
        // handle selected entity, preparing ticks and entities that are result of dividing original entities
        case RS2::EntityLine: {
            auto *lineEntity = dynamic_cast<RS_Line *>(entity);
            prepareLineTicks(lineEntity);
            bool mayDivide = checkShouldDivideEntity(lineEntity, tr("Line"));
            if (mayDivide){
                createLineSegments(lineEntity, list);
                entityToDelete = lineEntity;
            }
            break;
        }
        case RS2::EntityArc: {
            auto *arcEntity = dynamic_cast<RS_Arc *>(entity);
            prepareArcTicks(arcEntity);
            bool mayDivide = checkShouldDivideEntity(arcEntity, tr("Arc"));
            if (mayDivide){
                createArcSegments(arcEntity, list);
                entityToDelete = arcEntity;
            }
            break;
        }
        case RS2::EntityCircle: {
            auto *circleEntity = dynamic_cast<RS_Circle *>(entity);
            prepareCircleTicks(circleEntity);
            bool mayDivide = checkShouldDivideEntity(circleEntity, tr("Circle"));
            if (mayDivide){
                createCircleSegments(circleEntity, list);
                entityToDelete = circleEntity;
            }
            break;
        }
        default:
            break;
    }

    // delete original entity, if necessary
    if (entityToDelete != nullptr){
        deleteEntityUndoable(entityToDelete);
    }

    bool hasTickLength = LC_LineMath::isMeaningful(tickLength);
    if (hasTickLength){
        // ticks are non-zero, so we'll need to create lines for them
        uint count = ticksData.size();
        for (uint i = 0; i < count; i++) {
            TickData tick = ticksData.at(i);
            if (tick.isVisible){
                auto *line = new RS_Line(container, tick.tickLine);
                // for ticks, we'll always use current pen and layer
                line->setPenToActive();
                line->setLayerToActive();
                list<<line;
            }
        }
    }
}

void LC_ActionDrawSliceDivide::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, [[maybe_unused]]const RS_Vector &snapPoint){
    switch (status) {
        case SetEntity: {
            EntityTypeList catchEntityTypes = getCatchEntityTypeList();
            RS_Entity *en = catchModifiableEntity(e, catchEntityTypes);
            if (en != nullptr && !en->isParentIgnoredOnModifications()){
                // if we have selected entity, just perform the action
                entity = en;
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawSliceDivide::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    // just perform a cleanup
    entity = nullptr;
    ticksData.clear();
    init(SetEntity);
}

/**
 * Function check whether we may divide selected entity (i.e it is not part of polyline)
 * @param e entity
 * @param entityName name of entity for the message
 * @return
 */
bool LC_ActionDrawSliceDivide::checkShouldDivideEntity(const RS_Entity *e, const QString &entityName) const{
    bool mayDivide = false;
    if (doDivideEntity){
        mayDivide = checkMayExpandEntity(e, entityName);
    }
    return mayDivide;
}

/**
 * For tick data that is already calculated for line, method creates a set of lines that represents segments between ticks
 * Created segments will have the same attributes as original line
 * @param pLine original line
 * @param list list of entities to which created segments are added
 */
void LC_ActionDrawSliceDivide::createLineSegments(RS_Line *pLine, QList<RS_Entity *> &list){
    uint count = ticksData.size();
    if (count > 2){ // we always set 2 ticks for edges
        RS_Pen originalPen = pLine->getPen();
        RS_Layer *originalLayer = pLine->getLayer();

        for (uint i = 1; i < count; i++) {
            TickData startTick = ticksData.at(i - 1);
            TickData endTick = ticksData.at(i);
            RS_Vector startPoint = startTick.snapPoint;
            RS_Vector endPoint = endTick.snapPoint;
            auto *line = createLine(startPoint, endPoint,list);
            line->setLayer(originalLayer);
            line->setPen(originalPen);
        }
    }
}

/**
 * For tick data that is already calculated for arc, method creates a set of arcs that represents segments between ticks
 * Created segments will have the same attributes as original arc
 * @param pArc original arc
 * @param list list of entities to add created segments
 */
void LC_ActionDrawSliceDivide::createArcSegments(RS_Arc *pArc, QList<RS_Entity *> &list){
    RS_Vector center = pArc->getCenter();
    double radius = pArc->getRadius();
    bool reversed = pArc->isReversed();
    doCreateArcSegments(pArc, center, radius, reversed, list);
}

/**
 * For tick data that is already calculated for circle, method creates a set of arcs that represents segments between ticks
 * Created segments will have the same attributes as original circle
 * @param pArc original circle
 * @param list list of entities to add created segments
 */
void LC_ActionDrawSliceDivide::createCircleSegments(RS_Circle *pCircle, QList<RS_Entity *> &list){
    RS_Vector center = pCircle->getCenter();
    double radius = pCircle->getRadius();
    doCreateArcSegments(pCircle, center, radius, false, list);
}

/**
 * method that creates arc segments for arc or circle
 * @param pArc arc or circle
 * @param center center point of arc or circle
 * @param radius radius
 * @param reversed is reversed (for arc)
 * @param list list of entities to add segments
 */
void LC_ActionDrawSliceDivide::doCreateArcSegments(RS_Entity *pArc, const RS_Vector &center, double radius, bool reversed, QList<RS_Entity *> &list){
    size_t count = ticksData.size();

    if (count > 2){ // we always set 2 ticks for edges
        RS_Pen originalPen = pArc->getPen();
        RS_Layer* originalLayer = pArc->getLayer();


        for (size_t i = 1; i < count; i++) {
            TickData startTick = ticksData.at(i - 1);
            TickData endTick = ticksData.at(i);
            double startAngle = startTick.arcAngle;
            double endAngle = endTick.arcAngle;
            if (reversed){
                std::swap(startAngle, endAngle);
            }
            auto *newArc = new RS_Arc(container, RS_ArcData(center, radius, startAngle, endAngle, reversed));
            newArc->setLayer(originalLayer);
            newArc->setPen(originalPen);
            list << newArc;
        }
    }
}

/**
 * prepares ticks for arc
 * @param arc original arc
 */
void LC_ActionDrawSliceDivide::prepareArcTicks(RS_Arc *arc){
    double radius = arc->getRadius();
    RS_Vector center = arc->getCenter();
    double startPointAngle = arc->getAngle1();
    double endPointAngle = arc->getAngle2();
    RS_Vector startPoint = arc->getStartpoint();
    RS_Vector endPoint = arc->getEndpoint();
    if (arc->isReversed()){
        // handle angles properly
         std::swap(startPointAngle, endPointAngle);
         std::swap(startPoint, endPoint);
    }
    double arcLength = arc->getAngleLength();

    // create start edge tick, if any
    prepareStartTick(arc, startPoint, startPointAngle);
    // create intermediate ticks
    prepareArcSegments(arc, radius, center, startPointAngle, arcLength);
    // create end edge tick, if any
    prepareEndTick(arc, endPoint, endPointAngle);
}

/**
 * prepares ticks for circle
 * @param arc original circle
 */
void LC_ActionDrawSliceDivide::prepareCircleTicks(RS_Circle *circle){
    double radius = circle->getRadius();
    RS_Vector center = circle->getCenter();
    double startPointAngle = RS_Math::deg2rad(getCircleStartAngle());

    RS_Vector startPoint = LC_LineMath::findPointOnCircle(radius, startPointAngle, center);

    // for circle, we always have a start tick
    createTickData(circle, startPoint, startPointAngle, true, true);
    // calculate arc segment ticks for intermediate points
    prepareArcSegments(circle, radius, center, startPointAngle, M_PI * 2);
    // and invisible end tick that will be the same as start tick
    createTickData(circle, startPoint, startPointAngle, true, false);
}

/**
 * Calculates non-edge ticks for arc or circle
 * @param e arc
 * @param radius radius
 * @param center center point
 * @param startPointAngle starting angle
 * @param arcLength angle length of arc
 */
void LC_ActionDrawSliceDivide::prepareArcSegments(RS_Entity *e, double radius, RS_Vector &center, double startPointAngle, double arcLength){
    int segmentsCount = tickCount + 1;
    double segmentAngleLength = arcLength / segmentsCount;

    for (int i = 1; i < segmentsCount; i++) {
        double segmentAngle = (segmentAngleLength * i) + startPointAngle;
        RS_Vector tickSnapPosition = LC_LineMath::findPointOnCircle(radius, segmentAngle, center);
        createTickData(e, tickSnapPosition, segmentAngle, false);
    }
}
/**
 * Calculates ticks for line
 * @param line original line
 */
void LC_ActionDrawSliceDivide::prepareLineTicks(RS_Line *line){
    RS_Vector startPoint = line->getStartpoint();
    // create start edge tick, if any
    prepareStartTick(line, startPoint, 0);

    // calculate intermediate ticks
    double lineLength = line->getLength();
    int segmentsCount = tickCount + 1;
    double segmentLength;

    // todo - handle remaining part of line for fixed distance mode (and relate it to edges mode ticks somehow?)
//    double remainingPartOfLine = 0.0;

    if (fixedDistance){
        // for fixed distance between ticks, adjust length and ticks count
        segmentLength = distance;
        segmentsCount = std::ceil(lineLength / segmentLength + 1);
//        remainingPartOfLine = lineLength - (segmentLength + 1)* segmentsCount;
    }
    else {
        segmentLength = lineLength / segmentsCount;
    }
    const double lineAngle = line->getTangentDirection(startPoint).angle();

    for (int i = 1; i < segmentsCount; i++) {
        double distanceOnLine = segmentLength * i;
        if (distanceOnLine < lineLength){
            // if we're still within line, calculate snap point for tick on the line
            RS_Vector tickSnapPosition = startPoint.relative(distanceOnLine, lineAngle);

            // calculate tick for determined snap point
            createTickData(line, tickSnapPosition, 0, false);
        }
    }
    // create end edge tick, if any
    prepareEndTick(line, line->getEndpoint(), 0);
}

/**
 * Created start tick taking into consideration edge options
 * @param ent entity
 * @param tickSnapPoint tick snap point
 * @param arcAngle angle on arc, if any
 */
void LC_ActionDrawSliceDivide::prepareStartTick(RS_Entity *ent, const RS_Vector& tickSnapPoint, double arcAngle){
    bool visible = (tickEdgeDrawMode == DRAW_EDGE_BOTH) || (tickEdgeDrawMode == DRAW_EDGE_START);
    createTickData(ent, tickSnapPoint, arcAngle, true, visible);
}

/**
 * Creates end tick taking into consideration edge options
 * @param ent entity
 * @param tickSnapPoint tick snap point
 * @param arcAngle angle on arc, if any
 */
void LC_ActionDrawSliceDivide::prepareEndTick(RS_Entity *ent, const RS_Vector& tickSnapPoint, double arcAngle){
    bool visible = (tickEdgeDrawMode == DRAW_EDGE_BOTH) || (tickEdgeDrawMode == DRAW_EDGE_END);
    createTickData(ent, tickSnapPoint, arcAngle, true, visible);
}

/**
 * Calculates tick data for given snap point and adds tick to tick data
 * @param e entity
 * @param tickSnapPoint tick snap point
 * @param arcAngle angle on arc, if any
 * @param edge true if this is edge tick
 * @param visible if true, tick is visible and line for it should be created and added to drawing
 */
void LC_ActionDrawSliceDivide::createTickData(RS_Entity *e, RS_Vector tickSnapPoint, double arcAngle, bool edge, bool visible){
    RS_LineData lineData;
    prepareTickData(tickSnapPoint, e, lineData);
    addTick(tickSnapPoint, lineData, edge, visible, arcAngle);
}

/**
 * adds tick to tick data
 * @param tickSnapPoint
 * @param lineData
 * @param edge
 * @param visible
 * @param angle
 */
void LC_ActionDrawSliceDivide::addTick(RS_Vector &tickSnapPoint, RS_LineData &lineData, bool edge, bool visible, double angle){
    ticksData.push_back(TickData(edge, visible, tickSnapPoint, lineData, angle));
}

/**
 * Calculates the line of single tick for given tick snap position
 * @param tickSnapPosition tick snap position
 * @param ent original entiy
 * @param tickLineData tick line data
 */
void LC_ActionDrawSliceDivide::prepareTickData(RS_Vector &tickSnapPosition, RS_Entity *ent, RS_LineData &tickLineData){

    double actualTickLength = tickLength;
    auto const vp = ent->getNearestPointOnEntity(tickSnapPosition, false);

    double tickAngleToUse = tickAngle;
    if (alternativeActionMode){
        // if SHIFT is pressed, we'll mirror angle specified in options
        tickAngleToUse = 180 - tickAngle;
    }
    double tickAngleRad = RS_Math::deg2rad(tickAngleToUse);
    double actualTickAngle = tickAngleRad;

    // if angle should be related, take into consideration own angle of entity
    if (tickAngleIsRelative){
        actualTickAngle = actualTickAngle + ent->getTangentDirection(vp).angle();
    }

    // proceed offset of tick specified by options
    RS_Vector vectorOffset(0, 0, 0);
    if (LC_LineMath::isMeaningful(tickOffset)){
        vectorOffset = RS_Vector::polar(tickOffset, actualTickAngle);
    }

    // prepare vector that will correct tick positions based on specified snap mode option for ticks
    RS_Vector vectorOffsetCorrection(0, 0, 0);
    switch (tickSnapMode) {
        case SNAP_START:
            // start point of tick should be in tick's snap point
            break;
        case SNAP_END:
            // end point of tick should be in tick's snap point
            vectorOffsetCorrection = RS_Vector::polar(-actualTickLength, actualTickAngle);
            break;
        case SNAP_MIDDLE:
            // middle point of tick should be in tick's snap point
            vectorOffsetCorrection = RS_Vector::polar(-actualTickLength / 2, actualTickAngle);
            break;
    }

    // determine tick line start point
    tickLineData.startpoint = tickSnapPosition + vectorOffset + vectorOffsetCorrection;

    // determine tick line end point
    tickLineData.endpoint = tickLineData.startpoint.relative(actualTickLength, actualTickAngle);
}

void LC_ActionDrawSliceDivide::updateMouseButtonHints(){
    // todo - actually , if tick angle is 90 degrees, alternative mode is not meaningful, so it's better to adjust more
    // fine grained shift status there
    if (actionType == RS2::ActionDrawSliceDivideLine){
        updateMouseWidgetTRCancel(tr("Select line"), MOD_SHIFT_MIRROR_ANGLE);
    }
    else{
        updateMouseWidgetTRCancel(tr("Select circle or arc"), MOD_SHIFT_MIRROR_ANGLE);
    }
}

RS2::CursorType LC_ActionDrawSliceDivide::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawSliceDivide::createOptionsWidget(){
    return new LC_SliceDivideOptions();
}
