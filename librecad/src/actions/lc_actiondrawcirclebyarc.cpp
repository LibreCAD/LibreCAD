#include "lc_actiondrawcirclebyarc.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_entitycontainer.h"
#include "rs_document.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_preview.h"
#include "lc_circlebyarcoptions.h"
#include <QMouseEvent>

// fixme - check whether we need to add more options for attributes source

LC_ActionDrawCircleByArc::LC_ActionDrawCircleByArc(RS_EntityContainer& container,RS_GraphicView& graphicView):
    LC_AbstractActionWithPreview("Circle By Arc", container, graphicView){
    actionType = RS2::ActionDrawCircleByArc;
}

LC_ActionDrawCircleByArc::~LC_ActionDrawCircleByArc() = default;

bool LC_ActionDrawCircleByArc::doCheckMayTrigger(){
    return arc != nullptr;
}

RS_Vector LC_ActionDrawCircleByArc::doGetRelativeZeroAfterTrigger(){
    return arc->getCenter();
}

void LC_ActionDrawCircleByArc::doAfterTrigger(){
    arc = nullptr;
    setStatus(SetArc);
}

void LC_ActionDrawCircleByArc::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    // prepare data for cycle based on arc
    RS_CircleData circleData = createCircleData();

    // setup new circle
    RS_Entity *circle = new RS_Circle(container, circleData);
    list << circle;
}

void LC_ActionDrawCircleByArc::performTriggerDeletions(){
    // check whether we need to delete original arc
    if (replaceArcByCircle){
        bool mayDelete = true;
        bool locked = arc->isLocked();
        if (locked){
            commandMessageTR("Arc is not replaced as it is locked.");
            mayDelete = false;
        } else {
            RS_EntityContainer *pContainer = arc->getParent();
            if (pContainer != nullptr){
                // todo - what if arc is part of block?
                if (pContainer->rtti() == RS2::EntityPolyline){
                    // don't let deletion of arc which is part of polyline
                    mayDelete = false;
                    commandMessageTR("Arc is not removed as it is part of polyline. Expand polyline first.");
                }
            }
        }
        if (mayDelete){ // simply delete source arc
            deleteEntityUndoable(arc);
        }
    }
}

RS_CircleData LC_ActionDrawCircleByArc::createCircleData(){
    RS_Vector center = arc->getCenter();
    double radius = arc->getRadius();
    RS_CircleData result = RS_CircleData(center, radius);
    return result;
}

#define HIGHLIGHT_ORIGINAL_ARC false

void LC_ActionDrawCircleByArc::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    if (status == SetArc){
        RS_Entity *en = catchEntity(e, circleType, RS2::ResolveAll);
        if (en && (en->isArc() && en->rtti() == RS2::EntityArc)){

            arc = dynamic_cast<RS_Arc *>(en);
            if (HIGHLIGHT_ORIGINAL_ARC){
                highlightEntity(arc);
            }
            RS_CircleData circleData = createCircleData();
            RS_Entity *circle = new RS_Circle(container, circleData);
            list << circle;
        } else {
            arc = nullptr;
        }
    }
}

void LC_ActionDrawCircleByArc::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint){
    if (status == SetArc){
        trigger();
    }
}

void LC_ActionDrawCircleByArc::coordinateEvent(RS_CoordinateEvent *e){
//    RS_ActionInterface::coordinateEvent(e);
// potentially, it's possible to use coordinates for selection of arc - yet it seems it is overkill,
// selection by mouse is more convenient so do nothing there
}

void LC_ActionDrawCircleByArc::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetArc:
            updateMouseWidgetTR("Select arc or circle","Back");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType LC_ActionDrawCircleByArc::doGetMouseCursor(int status){
    switch (status)    {
        case SetArc:
            return RS2::SelectCursor;
        default:
            return RS2::CadCursor;
    }
}

void LC_ActionDrawCircleByArc::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_CircleByArcOptions>(nullptr);
}

void LC_ActionDrawCircleByArc::setReplaceArcByCircle(bool value){
    replaceArcByCircle = value;
}

