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

#include "lc_action_draw_slice_divide.h"

#include "lc_linemath.h"
#include "lc_slice_divide_options_filler.h"
#include "lc_slice_divide_options_widget.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_line.h"
#include "rs_pen.h"

namespace {
    //list of entity types supported by current action
    const EntityTypeList sliceDivideLineEntityTypeList = {RS2::EntityLine};
    const EntityTypeList sliceDivideCircleEntityTypeList = {RS2::EntityArc, RS2::EntityCircle};
}
// todo - think about free mode for selection of tick length... not clear how to do this in convenient way yet
// todo - think whether dividing arc/circle for fixed angle (similar to fixed length for lines is needed

void LC_ActionDrawSliceDivide::doSaveOptions() {
    save("Count", m_tickCount);
    save("Length", m_tickLength);
    save("Offset", m_tickOffset);
    save("Angle", m_tickAngleDegrees);
    save("TickSnap", m_tickSnapMode);
    save("TickEdgeMode", m_tickEdgeDrawMode);
    save("CircleStartAngle", m_circleStartTickAngleDegrees);
    save("LengthTickAngleRel", m_tickAngleIsRelative);
    save("DoDivide", m_doDivideEntity);
    save("Distance", m_distance);
    save("FixedDistance", m_fixedDistance);
}

void LC_ActionDrawSliceDivide::doLoadOptions() {
    m_tickCount = loadInt("Count", 1);
    m_tickLength = loadDouble("Length", 1.0);
    m_tickOffset = loadDouble("Offset",0.0);
    m_tickAngleDegrees = loadDouble("Angle", 90.0);
    m_tickSnapMode = loadInt("TickSnap", 0);
    m_tickEdgeDrawMode = loadInt("TickEdgeMode", 0);
    m_circleStartTickAngleDegrees = loadDouble("CircleStartAngle", 0);
    m_tickAngleIsRelative = loadBool("LengthTickAngleRel", true);
    m_doDivideEntity = loadBool("DoDivide", false);
    m_distance = loadDouble("Distance", 0);
    m_fixedDistance = loadBool("FixedDistance", true);
}

/**
 * Structure that describes single tick
 */
struct LC_ActionDrawSliceDivide::TickData {
    explicit TickData(const bool e, const bool v,
        const RS_Vector &p,
        const RS_LineData &l, const double ang):
        isVisible(v),
        edge(e),
        snapPoint(p),
        tickLine(l),
        arcAngle(ang){}

    ~TickData() = default;

    bool isVisible{true}; // visible or not
    bool edge{false}; // is for edge?
    RS_Vector snapPoint; // point on entity where tick is snapped
    RS_LineData tickLine; // line data for tick
    double arcAngle; // angle for snapping tick
};
LC_ActionDrawSliceDivide::LC_ActionDrawSliceDivide(LC_ActionContext *actionContext, const bool forCircle)
    :LC_AbstractActionWithPreview(forCircle ? "ActionDrawSliceDivideCircle":"ActionDrawSliceDivideLine", actionContext){
    if (forCircle){
        m_actionType = RS2::ActionDrawSliceDivideCircle;
    }
    else {
        m_actionType = RS2::ActionDrawSliceDivideLine;
    }
}

bool LC_ActionDrawSliceDivide::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* event, const int status){
    return status == SetEntity;
}
/**
 * determines which types of entities should be selectable for the action
 * @return
 */
EntityTypeList LC_ActionDrawSliceDivide::getCatchEntityTypeList() const{
    if (m_actionType == RS2::ActionDrawSliceDivideLine){
        return sliceDivideLineEntityTypeList;
    }
    return sliceDivideCircleEntityTypeList;
}
/**
 * creating preview ticks lines
 * @param e  event
 * @param snap  snap point
 * @param list  list to create preview entities
 * @param status current status of the action
 */
void LC_ActionDrawSliceDivide::doPreparePreviewEntities([[maybe_unused]] const LC_MouseEvent* e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    m_ticksData.clear();
    deleteSnapper();
    const EntityTypeList catchEntityTypes = getCatchEntityTypeList();
    RS_Entity *en = catchModifiableAndDescribe(e, catchEntityTypes);
    int optionsMode = SELECTION_NONE;
    if (en != nullptr){
        const int rtti = en->rtti();

        // proceed suitable entities and calculate ticks data for it
        switch (rtti) {
            case RS2::EntityLine: {
                const auto *lineEntity = dynamic_cast<RS_Line *>(en);
                highlightHover(en);
                prepareLineTicks(lineEntity);
                break;
            }
            case RS2::EntityArc: {
                const auto *arcEntity = dynamic_cast<RS_Arc *>(en);
                highlightHover(en);
                prepareArcTicks(arcEntity);
                optionsMode = SELECTION_ARC;
                break;
            }
            case RS2::EntityCircle: {
                const auto *circleEntity = dynamic_cast<RS_Circle *>(en);
                highlightHover(en);
                prepareCircleTicks(circleEntity);
                optionsMode = SELECTION_CIRCLE;
                break;
            }
            default:
                break;
        }

        // create lines for calculated ticks data
        const size_t createdTicksCount = m_ticksData.size();
        if (createdTicksCount > 0){
            const bool hasTickLength = LC_LineMath::isMeaningful(m_tickLength);
            const bool doDivideEntity = m_doDivideEntity;
            const bool doDrawTicks = hasTickLength || doDivideEntity;

            if (doDrawTicks){
                for (size_t i = 0; i < createdTicksCount; i++) {
                    const TickData* tick = m_ticksData.at(i);
                    if (tick->isVisible){
                        if (hasTickLength){ // create preview line for tick with non-zero length
                            const auto tickLine = new RS_Line(tick->tickLine.startpoint, tick->tickLine.endpoint);
                            list << tickLine;
                        }
                        if (doDivideEntity) { // if tick length is zero - it is just divide mode, without ticks
                            // so on preview, we just indicate that we may have divide points
                            // even if tick is present - we'd better highlight division points
                            if (m_showRefEntitiesOnPreview) {
                                createRefPoint(tick->snapPoint, list);
                            }
                        }
                    }
                }
            }
        }
    }
    if (m_actionType == RS2::ActionDrawSliceDivideCircle){
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
        if (m_entity != nullptr) {
            const int entityRtti = m_entity->rtti();
            switch (entityRtti) {
                case RS2::EntityLine:
                    result = m_actionType == RS2::ActionDrawSliceDivideLine;
                    break;
                case RS2::EntityArc:
                case RS2::EntityCircle: {
                    result = m_actionType == RS2::ActionDrawSliceDivideCircle;
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

void LC_ActionDrawSliceDivide::clearTickData() {
    for (const auto t: m_ticksData) {
        delete t;
    }
    m_ticksData.clear();
}


bool LC_ActionDrawSliceDivide::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx){
    clearTickData();
    const int rtti = m_entity->rtti();
    RS_Entity *entityToDelete = nullptr;
    switch (rtti) {
        // handle selected entity, preparing ticks and entities that are result of dividing original entities
        case RS2::EntityLine: {
            auto *lineEntity = dynamic_cast<RS_Line *>(m_entity);
            prepareLineTicks(lineEntity);
            const bool mayDivide = checkShouldDivideEntity(lineEntity, tr("Line"));
            if (mayDivide){
                createLineSegments(lineEntity, ctx.entitiesToAdd);
                entityToDelete = lineEntity;
            }
            break;
        }
        case RS2::EntityArc: {
            auto *arcEntity = dynamic_cast<RS_Arc *>(m_entity);
            prepareArcTicks(arcEntity);
            const bool mayDivide = checkShouldDivideEntity(arcEntity, tr("Arc"));
            if (mayDivide){
                createArcSegments(arcEntity, ctx.entitiesToAdd);
                entityToDelete = arcEntity;
            }
            break;
        }
        case RS2::EntityCircle: {
            auto *circleEntity = dynamic_cast<RS_Circle *>(m_entity);
            prepareCircleTicks(circleEntity);
            const bool mayDivide = checkShouldDivideEntity(circleEntity, tr("Circle"));
            if (mayDivide){
                createCircleSegments(circleEntity, ctx.entitiesToAdd);
                entityToDelete = circleEntity;
            }
            break;
        }
        default:
            break;
    }

    // delete original entity, if necessary
    if (entityToDelete != nullptr){
        ctx -= entityToDelete;
    }

    const bool hasTickLength = LC_LineMath::isMeaningful(m_tickLength);
    if (hasTickLength){
        // ticks are non-zero, so we'll need to create lines for them
        const size_t count = m_ticksData.size();
        for (size_t i = 0; i < count; i++) {
            const TickData* tick = m_ticksData.at(i);
            if (tick->isVisible){
                auto *line = new RS_Line(m_document, tick->tickLine);
                // for ticks, we'll always use current pen and layer
                setPenAndLayerToActive(line);
                ctx += line;
            }
        }
    }
    return true;
}

void LC_ActionDrawSliceDivide::doOnLeftMouseButtonRelease(const LC_MouseEvent* e, const int status, [[maybe_unused]]const RS_Vector &snapPoint){
    switch (status) {
        case SetEntity: {
            const EntityTypeList catchEntityTypes = getCatchEntityTypeList();
            RS_Entity *en = catchModifiableEntity(e, catchEntityTypes);
            if (en != nullptr && !en->isParentIgnoredOnModifications()){
                // if we have selected entity, just perform the action
                m_entity = en;
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
    m_entity = nullptr;
    clearTickData();
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
    if (m_doDivideEntity){
        mayDivide = checkMayExpandEntity(e, entityName);
    }
    return mayDivide;
}

/**
 * For tick data that is already calculated for line, method creates a set of lines that represents segments between ticks
 * Created segments will have the same attributes as original line
 * @param line original line
 * @param list list of entities to which created segments are added
 */
void LC_ActionDrawSliceDivide::createLineSegments(const RS_Line *line, QList<RS_Entity *> &list) const {
    const size_t count = m_ticksData.size();
    if (count > 2){ // we always set 2 ticks for edges
        const RS_Pen originalPen = line->getPen(false);
        RS_Layer *originalLayer = line->getLayer(false);

        for (size_t i = 1; i < count; i++) {
            const TickData* startTick = m_ticksData.at(i - 1);
            const TickData* endTick = m_ticksData.at(i);
            RS_Vector startPoint = startTick->snapPoint;
            RS_Vector endPoint = endTick->snapPoint;
            auto *createdLine = createLine(startPoint, endPoint,list);
            createdLine->setLayer(originalLayer);
            createdLine->setPen(originalPen);
        }
    }
}

/**
 * For tick data that is already calculated for arc, method creates a set of arcs that represents segments between ticks
 * Created segments will have the same attributes as original arc
 * @param pArc original arc
 * @param list list of entities to add created segments
 */
void LC_ActionDrawSliceDivide::createArcSegments(const RS_Arc *pArc, QList<RS_Entity *> &list) const {
    const RS_Vector center = pArc->getCenter();
    const double radius = pArc->getRadius();
    const bool reversed = pArc->isReversed();
    doCreateArcSegments(pArc, center, radius, reversed, list);
}

/**
 * For tick data that is already calculated for circle, method creates a set of arcs that represents segments between ticks
 * Created segments will have the same attributes as original circle
 * @param pCircle
 * @param list list of entities to add created segments
 */
void LC_ActionDrawSliceDivide::createCircleSegments(const RS_Circle *pCircle, QList<RS_Entity *> &list) const {
    const RS_Vector center = pCircle->getCenter();
    const double radius = pCircle->getRadius();
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
void LC_ActionDrawSliceDivide::doCreateArcSegments(const RS_Entity *pArc, const RS_Vector &center, const double radius, const bool reversed, QList<RS_Entity *> &list) const {
    const size_t count = m_ticksData.size();

    if (count > 2){ // we always set 2 ticks for edges
        const RS_Pen originalPen = pArc->getPen(false);
        RS_Layer* originalLayer = pArc->getLayer();


        for (size_t i = 1; i < count; i++) {
            const TickData* startTick = m_ticksData.at(i - 1);
            const TickData* endTick = m_ticksData.at(i);
            double startAngle = startTick->arcAngle;
            double endAngle = endTick->arcAngle;
            if (reversed){
                std::swap(startAngle, endAngle);
            }
            auto *newArc = new RS_Arc(m_document, RS_ArcData(center, radius, startAngle, endAngle, reversed));
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
void LC_ActionDrawSliceDivide::prepareArcTicks(const RS_Arc* arc){
    const double radius = arc->getRadius();
    const RS_Vector center = arc->getCenter();
    double startPointAngle = arc->getAngle1();
    double endPointAngle = arc->getAngle2();
    RS_Vector startPoint = arc->getStartpoint();
    RS_Vector endPoint = arc->getEndpoint();
    if (arc->isReversed()){
        // handle angles properly
         std::swap(startPointAngle, endPointAngle);
         std::swap(startPoint, endPoint);
    }
    const double arcLength = arc->getAngleLength();

    // create start edge tick, if any
    prepareStartTick(arc, startPoint, startPointAngle);
    // create intermediate ticks
    prepareArcSegments(arc, radius, center, startPointAngle, arcLength);
    // create end edge tick, if any
    prepareEndTick(arc, endPoint, endPointAngle);
}

/**
 * prepares ticks for circle
 * @param circle original circle
 */
void LC_ActionDrawSliceDivide::prepareCircleTicks(const RS_Circle *circle){
    const double radius = circle->getRadius();
    const RS_Vector center = circle->getCenter();
    const double startPointAngle = toWorldAngleFromUCSBasisDegrees(m_circleStartTickAngleDegrees);

    const RS_Vector startPoint = LC_LineMath::findPointOnCircle(radius, startPointAngle, center);

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
void LC_ActionDrawSliceDivide::prepareArcSegments(const RS_Entity *e, const double radius, const RS_Vector &center, const double startPointAngle, const double arcLength){
    const int segmentsCount = m_tickCount + 1;
    const double segmentAngleLength = arcLength / segmentsCount;

    for (int i = 1; i < segmentsCount; i++) {
        const double segmentAngle = (segmentAngleLength * i) + startPointAngle;
        RS_Vector tickSnapPosition = LC_LineMath::findPointOnCircle(radius, segmentAngle, center);
        createTickData(e, tickSnapPosition, segmentAngle, false);
    }
}
/**
 * Calculates ticks for line
 * @param line original line
 */
void LC_ActionDrawSliceDivide::prepareLineTicks(const RS_Line *line){
    const RS_Vector startPoint = line->getStartpoint();
    // create start edge tick, if any
    prepareStartTick(line, startPoint, 0);

    // calculate intermediate ticks
    const double lineLength = line->getLength();
    int segmentsCount = m_tickCount + 1;
    double segmentLength;

    // todo - handle remaining part of line for fixed distance mode (and relate it to edges mode ticks somehow?)
//    double remainingPartOfLine = 0.0;

    if (m_fixedDistance){
        // for fixed distance between ticks, adjust length and ticks count
        segmentLength = m_distance;
        segmentsCount = std::ceil(lineLength / segmentLength + 1);
    }
    else {
        segmentLength = lineLength / segmentsCount;
    }
    const double lineAngle = line->getTangentDirection(startPoint).angle();

    for (int i = 1; i < segmentsCount; i++) {
        const double distanceOnLine = segmentLength * i;
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
 * @param entity entity
 * @param tickSnapPoint tick snap point
 * @param arcAngle angle on arc, if any
 */
void LC_ActionDrawSliceDivide::prepareStartTick(const RS_Entity *entity, const RS_Vector& tickSnapPoint, const double arcAngle){
    const int tickEdgeDrawMode = m_tickEdgeDrawMode;
    const bool visible = (tickEdgeDrawMode == DRAW_EDGE_BOTH) || (tickEdgeDrawMode == DRAW_EDGE_START);
    createTickData(entity, tickSnapPoint, arcAngle, true, visible);
}

/**
 * Creates end tick taking into consideration edge options
 * @param entity entity
 * @param tickSnapPoint tick snap point
 * @param arcAngle angle on arc, if any
 */
void LC_ActionDrawSliceDivide::prepareEndTick(const RS_Entity *entity, const RS_Vector& tickSnapPoint, const double arcAngle){
    const int tickEdgeDrawMode = m_tickEdgeDrawMode;
    const bool visible = (tickEdgeDrawMode == DRAW_EDGE_BOTH) || (tickEdgeDrawMode == DRAW_EDGE_END);
    createTickData(entity, tickSnapPoint, arcAngle, true, visible);
}

/**
 * Calculates tick data for given snap point and adds tick to tick data
 * @param e entity
 * @param tickSnapPoint tick snap point
 * @param arcAngle angle on arc, if any
 * @param edge true if this is edge tick
 * @param visible if true, tick is visible and line for it should be created and added to drawing
 */
void LC_ActionDrawSliceDivide::createTickData(const RS_Entity *e, const RS_Vector& tickSnapPoint, const double arcAngle, const bool edge, const bool visible){
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
void LC_ActionDrawSliceDivide::addTick(const RS_Vector &tickSnapPoint, const RS_LineData &lineData, const bool edge, const bool visible, const double angle){
    m_ticksData.push_back(new TickData(edge, visible, tickSnapPoint, lineData, angle));
}

/**
 * Calculates the line of single tick for given tick snap position
 * @param tickSnapPosition tick snap position
 * @param entity original entiy
 * @param tickLineData tick line data
 */
void LC_ActionDrawSliceDivide::prepareTickData(const RS_Vector &tickSnapPosition, const RS_Entity *entity, RS_LineData &tickLineData) const {
    const double actualTickLength = m_tickLength;
    const double tickAngleDegrees = m_tickAngleDegrees;
    double tickAngleToUse = tickAngleDegrees;
    if (m_alternativeActionMode){
        // if SHIFT is pressed, we'll mirror angle specified in options
        tickAngleToUse = 180 - tickAngleDegrees;
    }

    double actualTickAngle;

    // if angle should be related, take into consideration own angle of entity
    if (m_tickAngleIsRelative){
        const double tickAngleRad = RS_Math::deg2rad(tickAngleToUse);
        const auto vp = entity->getNearestPointOnEntity(tickSnapPosition, false);
        actualTickAngle = tickAngleRad + toUCSAngle(entity->getTangentDirection(vp).angle());
    }
    else{
        actualTickAngle = toWorldAngleFromUCSBasisDegrees(tickAngleToUse);
    }

    // proceed offset of tick specified by options
    RS_Vector vectorOffset(0, 0, 0);
    const double tickOffset = m_tickOffset;
    if (LC_LineMath::isMeaningful(tickOffset)){
        vectorOffset = RS_Vector::polar(tickOffset, actualTickAngle);
    }

    // prepare vector that will correct tick positions based on specified snap mode option for ticks
    RS_Vector vectorOffsetCorrection(0, 0, 0);
    switch (m_tickSnapMode) {
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
        default:
            break;
    }

    // determine tick line start point
    tickLineData.startpoint = tickSnapPosition + vectorOffset + vectorOffsetCorrection;

    // determine tick line end point
    tickLineData.endpoint = tickLineData.startpoint.relative(actualTickLength, actualTickAngle);
}

void LC_ActionDrawSliceDivide::updateActionPrompt(){
    // todo - actually , if tick angle is 90 degrees, alternative mode is not meaningful, so it's better to adjust more
    // fine grained shift status there
    if (m_actionType == RS2::ActionDrawSliceDivideLine){
        updatePromptTRCancel(tr("Select line"), MOD_SHIFT_MIRROR_ANGLE);
    }
    else{
        updatePromptTRCancel(tr("Select circle or arc"), MOD_SHIFT_MIRROR_ANGLE);
    }
}

RS2::CursorType LC_ActionDrawSliceDivide::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::SelectCursor;
}

bool LC_ActionDrawSliceDivide::doUpdateAngleByInteractiveInput(const QString& tag, const double angle) {
    if (tag == "angle") {
        setTickAngleDegrees(RS_Math::rad2deg(angle));
        return true;
    }
    if (tag == "angleCircle") {
        setCircleStartTickAngleDegrees(RS_Math::rad2deg(angle));
        return true;
    }
    return false;
}

bool LC_ActionDrawSliceDivide::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "length") {
        setTickLength(distance);
        return true;
    }
    if (tag == "offset") {
        setTickOffset(distance);
        return true;
    }
    if (tag == "distance") {
        setTickOffset(distance);
        return true;
    }
    return false;
}

LC_ActionOptionsWidget* LC_ActionDrawSliceDivide::createOptionsWidget() {
    return  new LC_SliceDivideOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawSliceDivide::createOptionsFiller() {
    return new LC_SliceDivideOptionsFiller();
}
