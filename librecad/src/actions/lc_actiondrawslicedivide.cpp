#include <QMouseEvent>

#include "lc_actiondrawslicedivide.h"
#include "rs_dialogfactory.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_point.h"
#include "rs_debug.h"
#include "rs_math.h"
#include "math.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_modification.h"
#include "lc_linemath.h"
#include "lc_slicedivideoptions.h"


// fixme - initial highlight of entity to select
namespace {
    //list of entity types supported by current action
    const EntityTypeList sliceDivideEntityTypeList = {RS2::EntityLine, RS2::EntityArc, RS2::EntityCircle};
}

LC_ActionDrawSliceDivide::LC_ActionDrawSliceDivide(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview("Draw slice divide", container, graphicView){
    actionType = RS2::ActionDrawSliceDivide;
}

// fixme - remove
bool LC_ActionDrawSliceDivide::isCircleEntity(){
    return false;
}

void LC_ActionDrawSliceDivide::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
}

bool LC_ActionDrawSliceDivide::doCheckMayDrawPreview(QMouseEvent *event, int status){
    return status == SetEntity;
}

void LC_ActionDrawSliceDivide::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    ticksData.clear();
    RS_Entity *en = catchEntity(e, sliceDivideEntityTypeList, RS2::ResolveAll);
    if (en != nullptr){
        int rtti = en->rtti();
        switch (rtti) {
            case RS2::EntityLine: {
                RS_Line *lineEntity = dynamic_cast<RS_Line *>(en);
                prepareLineTicks(lineEntity);
                break;
            }
            case RS2::EntityArc: {
                RS_Arc *arcEntity = dynamic_cast<RS_Arc *>(en);
                prepareArcTicks(arcEntity);
                break;
            }
            case RS2::EntityCircle: {
                RS_Circle *circleEntity = dynamic_cast<RS_Circle *>(en);
                prepareCircleTicks(circleEntity);
                break;
            }
        }
        int createdTicksCount = ticksData.size();
        if (createdTicksCount > 0){
            bool hasTickLength = LC_LineMath::isMeaningful(tickLength);
            bool doDrawTicks = hasTickLength || doDivideEntity;

            if (doDrawTicks){
                for (int i = 0; i < createdTicksCount; i++) {
                    TickData tick = ticksData.at(i);
                    if (tick.isVisible){
                        RS_Entity *tickEntity;
                        if (hasTickLength){
                            tickEntity = new RS_Line(tick.tickLine.startpoint, tick.tickLine.endpoint);
                        } else { // just divide mode, without ticks
                            // just indicate that we may have divide points
                            tickEntity = new RS_Point(nullptr, RS_PointData(tick.snapPoint));
                        }
                        list << tickEntity;
                    }
                }
            }
        }
    }
}

bool LC_ActionDrawSliceDivide::doCheckMayTrigger(){
    return entity != nullptr && getStatus() == SetEntity;
}

bool LC_ActionDrawSliceDivide::isSetActivePenAndLayerOnTrigger(){
    return false;
}

void LC_ActionDrawSliceDivide::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    ticksData.clear();
    int rtti = entity->rtti();
    RS_Entity* entityToDelete = nullptr;
    switch (rtti) {
        case RS2::EntityLine: {
            RS_Line *lineEntity = dynamic_cast<RS_Line *>(entity);
            prepareLineTicks(lineEntity);
            bool mayDivide = checkShouldDivideEntity(lineEntity, tr("Line"));
            if (mayDivide){
                createLineSegments(lineEntity, list);
                entityToDelete = lineEntity;
            }
            break;
        }
        case RS2::EntityArc: {
            RS_Arc *arcEntity = dynamic_cast<RS_Arc *>(entity);
            prepareArcTicks(arcEntity);
            bool mayDivide = checkShouldDivideEntity(arcEntity, tr("Arc"));
            if (mayDivide){
                createArcSegments(arcEntity, list);
                entityToDelete = arcEntity;
            }
            break;
        }
        case RS2::EntityCircle: {
            RS_Circle *circleEntity = dynamic_cast<RS_Circle *>(entity);
            prepareCircleTicks(circleEntity);
            bool mayDivide = checkShouldDivideEntity(circleEntity, tr("Circle"));
            if (mayDivide){
                createCircleSegments(circleEntity, list);
                entityToDelete = circleEntity;
            }
            break;
        }
    }

    if (entityToDelete != nullptr){
        deleteEntityUndoable(entityToDelete);
    }

    bool hasTickLength = LC_LineMath::isMeaningful(tickLength);
    if (hasTickLength){
        int count = ticksData.size();
        for (int i = 0; i < count; i++) {
            TickData tick = ticksData.at(i);
            if (tick.isVisible){
                RS_Line *line = new RS_Line(container, tick.tickLine);
                line->setPenToActive();
                line->setLayerToActive();
                list<<line;
            }
        }
    }
}

void LC_ActionDrawSliceDivide::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed){
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, sliceDivideEntityTypeList, RS2::ResolveAll);
            if (en != nullptr){
                entity = en;
                alternateAngle = shiftPressed;
                trigger();
            }
            break;
        }
            /*case SetSnapDistance:
            {
                RS_CoordinateEvent ce(snapPoint(e));
                coordinateEvent(&ce);
            }
                break;
            case SetTickLength:{
                if (lengthIsFree){
                    trigger();
                    setStatus(SetLine);
                }
                break;
            }*/
        default:
            break;
    }
}


void LC_ActionDrawSliceDivide::updateMouseButtonHints(){
    // fixme - complete
    RS_ActionInterface::updateMouseButtonHints();
}

RS2::CursorType LC_ActionDrawSliceDivide::doGetMouseCursor(int status){
    return RS2::SelectCursor;
}


void LC_ActionDrawSliceDivide::commandEvent(RS_CommandEvent *e){
    RS_ActionInterface::commandEvent(e);
}

void LC_ActionDrawSliceDivide::coordinateEvent(RS_CoordinateEvent *e){
    RS_ActionInterface::coordinateEvent(e);
}

void LC_ActionDrawSliceDivide::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    entity = nullptr;
    ticksData.clear();
    alternateAngle = false;
    init(SetEntity);

}

// fixme
RS_Vector LC_ActionDrawSliceDivide::doGetRelativeZeroAfterTrigger(){
    return LC_AbstractActionWithPreview::doGetRelativeZeroAfterTrigger();
}

bool LC_ActionDrawSliceDivide::checkShouldDivideEntity(const RS_Entity *e, const QString &entityName) const{
    bool mayDivide = false;
    if (doDivideEntity){
        bool locked = e->isLocked();
        if (locked){
            commandMessage(entityName + tr(" is not divided as it is locked."));
        } else {
            RS_EntityContainer *pContainer = e->getParent();
            if (pContainer != nullptr){
                if (pContainer->rtti() == RS2::EntityPolyline){
                    mayDivide = false;
                    commandMessage(entityName + tr(" is not divided as it is part of polyline. Expand polyline first."));
                } else {
                    mayDivide = true;
                }
            } else {
                mayDivide = true;
            }
        }
    }
    return mayDivide;
}

void LC_ActionDrawSliceDivide::createLineSegments(RS_Line *pLine, QList<RS_Entity *> &list){
    int count = ticksData.size();
    if (count > 2){ // we always set 2 ticks for edges
        RS_Pen originalPen = pLine->getPen();
        RS_Layer *originalLayer = pLine->getLayer();

        for (int i = 1; i < count; i++) {
            TickData startTick = ticksData.at(i - 1);
            TickData endTick = ticksData.at(i);
            RS_Vector startPoint = startTick.snapPoint;
            RS_Vector endPoint = endTick.snapPoint;
            RS_Line *line = new RS_Line(container, startPoint, endPoint);
            line->setLayer(originalLayer);
            line->setPen(originalPen);
            list << line;
        }
    }
}


void LC_ActionDrawSliceDivide::createArcSegments(RS_Arc *pArc, QList<RS_Entity *> &list){
    RS_Vector center = pArc->getCenter();
    double radius = pArc->getRadius();
    bool reversed = pArc->isReversed();
    doCreateArcSegments(pArc, center, radius, reversed, list);
}

void LC_ActionDrawSliceDivide::doCreateArcSegments(RS_Entity *pArc, const RS_Vector &center, double radius, bool reversed, QList<RS_Entity *> &list){
    int count = ticksData.size();

    if (count > 2){ // we always set 2 ticks for edges
        RS_Pen originalPen = pArc->getPen();
        RS_Layer* originalLayer = pArc->getLayer();

        for (int i = 1; i < count; i++) {
            TickData startTick = ticksData.at(i - 1);
            TickData endTick = ticksData.at(i);
            double startAngle = startTick.arcAngle;
            double endAngle = endTick.arcAngle;
            RS_Arc *newArc = new RS_Arc(container, RS_ArcData(center, radius, startAngle, endAngle, reversed));
            newArc->setLayer(originalLayer);
            newArc->setPen(originalPen);
            list << newArc;
        }
    }
}

void LC_ActionDrawSliceDivide::createCircleSegments(RS_Circle *pCircle, QList<RS_Entity *> &list){
    RS_Vector center = pCircle->getCenter();
    double radius = pCircle->getRadius();
    doCreateArcSegments(pCircle, center, radius, false, list);
}

void LC_ActionDrawSliceDivide::doDrawTicks(){
    bool hasTickLength = (std::abs(tickLength) > RS_TOLERANCE);
    if (hasTickLength){
        int count = ticksData.size();
        for (int i = 0; i < count; i++) {
            TickData tick = ticksData.at(i);
            if (tick.isVisible){
                RS_Line *line = new RS_Line(container, tick.tickLine);
                line->setLayerToActive();
                line->setPenToActive();
                container->addEntity(line);
                document->addUndoable(line);
            }
        }
    }
}

void LC_ActionDrawSliceDivide::init(int status){
    RS_PreviewActionInterface::init(status);
}

void LC_ActionDrawSliceDivide::prepareArcTicks(RS_Arc *arc){
    double radius = arc->getRadius();
    RS_Vector center = arc->getCenter();
    double startPointAngle = arc->getAngle1();
    double arcLength = arc->getAngleLength();

    // create start edge tick, if any
    prepareStartTick(arc, arc->getStartpoint(), startPointAngle);
    prepareArcSegments(arc, radius, center, startPointAngle, arcLength);
    // create end edge tick, if any
    prepareEndTick(arc, arc->getEndpoint(), arc->getAngle2());

}

void LC_ActionDrawSliceDivide::prepareArcSegments(RS_Entity *e, double radius, RS_Vector &center, double startPointAngle, double arcLength){
    int segmentsCount = tickCount + 1;
    double segmentAngleLength = arcLength / segmentsCount;

    for (int i = 1; i < segmentsCount; i++) {
        double segmentAngle = (segmentAngleLength * i) + startPointAngle;
        RS_Vector tickSnapPosition = LC_LineMath::findPointOnCircle(radius, segmentAngle, center);
        createTickData(e, tickSnapPosition, segmentAngle, false);
    }
}


void LC_ActionDrawSliceDivide::prepareCircleTicks(RS_Circle *circle){
    double radius = circle->getRadius();
    RS_Vector center = circle->getCenter();
    double startPointAngle = RS_Math::deg2rad(getCircleStartAngle());

    RS_Vector startPoint = LC_LineMath::findPointOnCircle(radius, startPointAngle, center);

    prepareStartTick(circle, startPoint, startPointAngle);
    prepareArcSegments(circle, radius, center, startPointAngle, M_PI * 2);
    prepareEndTick(circle, startPoint, startPointAngle);
}

void LC_ActionDrawSliceDivide::prepareLineTicks(RS_Line *line){

    RS_Vector startPoint = line->getStartpoint();

    // create start edge tick, if any
    prepareStartTick(line, startPoint, 0);

    double lineLength = line->getLength();
    int segmentsCount = tickCount + 1;
    double segmentLength = lineLength / segmentsCount;
    const double lineAngle = line->getTangentDirection(startPoint).angle();

    for (int i = 1; i < segmentsCount; i++) {
        double distanceOnLine = segmentLength * i;
        const RS_Vector snapVector = RS_Vector::polar(distanceOnLine, lineAngle);
        RS_Vector tickSnapPosition = startPoint + snapVector;
        createTickData(line, tickSnapPosition, 0, false);
    }

    // create end edge tick, if any
    prepareEndTick(line, line->getEndpoint(), 0);
}

void LC_ActionDrawSliceDivide::prepareEndTick(RS_Entity *ent, const RS_Vector& endPoint, double arcAngle){
    bool visible = (tickEdgeDrawMode == DRAW_EDGE_BOTH) || (tickEdgeDrawMode == DRAW_EDGE_END);
    createTickData(ent, endPoint, arcAngle, true, visible);
}

void LC_ActionDrawSliceDivide::prepareStartTick(RS_Entity *ent, const RS_Vector& startPoint, double arcAngle){
    bool visible = (tickEdgeDrawMode == DRAW_EDGE_BOTH) || (tickEdgeDrawMode == DRAW_EDGE_START);
    createTickData(ent,  startPoint, arcAngle, true, visible);
}

void LC_ActionDrawSliceDivide::createTickData(RS_Entity *e, RS_Vector tickSnapPoint, double angle, bool edge, bool visible){
    RS_LineData lineData;
    prepareTickData(tickSnapPoint, e, lineData);
    addTick(tickSnapPoint, lineData, edge, visible, angle);
}

void LC_ActionDrawSliceDivide::addTick(RS_Vector &tickSnapPoint, RS_LineData &lineData, bool edge, bool visible, double angle){
    ticksData.push_back(TickData(edge, visible, tickSnapPoint, lineData, angle));
}

void LC_ActionDrawSliceDivide::prepareTickData(RS_Vector &tickSnapPosition, RS_Entity *ent, RS_LineData &tickLineData){

    double actualTickLength = tickLength;
    auto const vp = ent->getNearestPointOnEntity(tickSnapPosition, false);

    double tickAngleToUse = tickAngle;
    if (alternateAngle){
        tickAngleToUse = 180 - tickAngle;
    }
    double tickAngleRad = RS_Math::deg2rad(tickAngleToUse);
    double actualTickAngle = tickAngleRad;
    if (tickAngleIsRelative){
        actualTickAngle = actualTickAngle + ent->getTangentDirection(vp).angle();
    }

    RS_Vector vectorOffset(0, 0, 0);
    RS_Vector vectorOffsetCorrection(0, 0, 0);

    if (LC_LineMath::isMeaningful(tickOffset)){
        vectorOffset = RS_Vector::polar(tickOffset, actualTickAngle);
    }

    switch (tickSnapMode) {
        case SNAP_START:
            break;
        case SNAP_END:
            vectorOffsetCorrection = RS_Vector::polar(-actualTickLength, actualTickAngle);
            break;
        case SNAP_MIDDLE:
            vectorOffsetCorrection = RS_Vector::polar(-actualTickLength / 2, actualTickAngle);
            break;
    }

    tickLineData.startpoint = tickSnapPosition + vectorOffset + vectorOffsetCorrection;
    RS_Vector vectorTick = RS_Vector::polar(actualTickLength, actualTickAngle);
    tickLineData.endpoint = tickLineData.startpoint + vectorTick;
}

void LC_ActionDrawSliceDivide::doMouseMoveEnd(int status, QMouseEvent *e){
    alternateAngle = false;
}

void LC_ActionDrawSliceDivide::doMouseMoveStart(int status, QMouseEvent *pEvent, bool shiftPressed){
    if (shiftPressed && status == SetEntity){
        alternateAngle = true;
    }
}

int LC_ActionDrawSliceDivide::getTickSnapMode(){return tickSnapMode;}

int LC_ActionDrawSliceDivide::getTickCount(){return tickCount;}

int LC_ActionDrawSliceDivide::getDrawTickOnEdgeMode(){return tickEdgeDrawMode;}

double LC_ActionDrawSliceDivide::getTickAngle(){return tickAngle;}

double LC_ActionDrawSliceDivide::getTickLength(){return tickLength;}

double LC_ActionDrawSliceDivide::getTickOffset(){    return tickOffset;}

double LC_ActionDrawSliceDivide::getCircleStartAngle(){return circleStartTickAngle;}

bool LC_ActionDrawSliceDivide::isTickAngleRelative(){
    return tickAngleIsRelative;
}

bool LC_ActionDrawSliceDivide::isDivideEntity(){return doDivideEntity;}

void LC_ActionDrawSliceDivide::setTickLength(double len){
    tickLength = len;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setDrawTickOnEdgeMode(int i){
    tickEdgeDrawMode = i;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setTickAngle(double a){
    tickAngle = a;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setCircleStartTickAngle(double a){
    circleStartTickAngle = a;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setTickAngleRelative(bool b){
    tickAngleIsRelative = b;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setDivideEntity(bool value){
    doDivideEntity = value;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setTickCount(int c){
    tickCount = c;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setTickSnapMode(int m){
    tickSnapMode = m;
    updatePreview();
}

void LC_ActionDrawSliceDivide::setTickOffset(double offset){
    tickOffset = offset;
    updatePreview();
}

void LC_ActionDrawSliceDivide::updatePreview(){

}

void LC_ActionDrawSliceDivide::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_SliceDivideOptions>(nullptr);
}














