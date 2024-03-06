#include "lc_actiondrawcirclebyarc.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_entitycontainer.h"
#include "rs_document.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_preview.h"
#include "rs_dialogfactory.h"
#include "lc_circlebyarcoptions.h"
#include <QMouseEvent>

LC_ActionDrawCircleByArc::LC_ActionDrawCircleByArc(RS_EntityContainer& container,RS_GraphicView& graphicView):
    RS_PreviewActionInterface("Circle By Arc", container, graphicView)
{
}

LC_ActionDrawCircleByArc::~LC_ActionDrawCircleByArc() = default;

void LC_ActionDrawCircleByArc::trigger(){
    RS_PreviewActionInterface::trigger();

    if (arc != nullptr){ // check we have arc selected
        // prepare data for cycle based on arc
        RS_CircleData circleData = createCircleData();

        // setup new circle
        RS_Entity *circle = new RS_Circle(container, circleData);

        arc->setHighlighted(false);
        graphicView->drawEntity(arc);

        circle->setLayerToActive();
        circle->setPenToActive();
        container->addEntity(circle);

        if (document){
            document->startUndoCycle();

            // check whether we need to delete oridinal arc
            if (replaceArcByCircle){
                bool mayDelete = true;
                bool locked = arc->isLocked();
                if (locked){
                    RS_DIALOGFACTORY->commandMessage(tr("Arc is not replaced as it is locked."));
                    mayDelete = false;
                } else {
                    RS_EntityContainer *pContainer = arc->getParent();
                    if (pContainer != nullptr){
                        // todo - what if arc is part of block?
                        if (pContainer->rtti() == RS2::EntityPolyline){
                            // don't let deletion of arc which is part of polyline
                            mayDelete = false;
                            RS_DIALOGFACTORY->commandMessage(tr("Arc is not removed as it is part of polyline. Expand polyline first."));
                        }
                    }
                }
                if (mayDelete){ // simply delete source arc
                    deleteOriginalEntity(arc);
                }
            }

            document->addUndoable(circle);
            document->endUndoCycle();
        }

        graphicView->moveRelativeZero(circleData.center);

        graphicView->redraw(RS2::RedrawDrawing);

        setStatus(SetCircle);

        arc = nullptr;

    } else {
        RS_DEBUG->print("RS_ActionDrawCroww::trigger:"
                        " Circle is nullptr\n");
    }
}

void LC_ActionDrawCircleByArc::deleteOriginalEntity(RS_Entity *entity){
    // delete and add this into undo
    graphicView->deleteEntity(entity);
    entity->changeUndoState();
    document->addUndoable(entity);
}

RS_CircleData LC_ActionDrawCircleByArc::createCircleData(){

    RS_Vector center = arc->getCenter();
    double radius = arc->getRadius();

    RS_CircleData result = RS_CircleData(center, radius);
    return result;
}


void LC_ActionDrawCircleByArc::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("LC_ActionDrawCross::mouseMoveEvent begin");

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));

    switch (getStatus()) {
        case SetCircle: {
            RS_Entity* en = catchEntity(e, circleType, RS2::ResolveAll);
            if (en && (en->isArc() && en->rtti() == RS2::EntityArc)) {
                if(arc != nullptr){
                    arc->setHighlighted(false);
                    graphicView->drawEntity(en);
                }
                arc = dynamic_cast<RS_Arc *>(en);
                arc->setHighlighted(true);
                graphicView->drawEntity(en);

                deletePreview();

                RS_CircleData circleData = createCircleData();

                RS_Entity *circle = new RS_Circle(container, circleData);
                circle->setPenToActive();
                circle->setLayerToActive();

                preview->addEntity(circle);

                drawPreview();
            }
            else{
                deletePreview();
                if(arc){
                    arc->setHighlighted(false);
                    graphicView->drawEntity(en);
                }
                graphicView->redraw(RS2::RedrawOverlay);
                arc = nullptr;
            }
        }
        break;

        default:
            break;
    }

    RS_DEBUG->print("LC_ActionDrawCross::mouseMoveEvent end");
}

void LC_ActionDrawCircleByArc::mouseReleaseEvent(QMouseEvent *e){
    if (e->button()==Qt::RightButton) {
        deletePreview();
        if(arc){
            arc->setHighlighted(false);
            graphicView->drawEntity(arc);
        }
        init(getStatus()-1);
    } else {
        switch (getStatus()) {
            case SetCircle:
                trigger();
                break;
        }
    }
}

void LC_ActionDrawCircleByArc::coordinateEvent(RS_CoordinateEvent *e){
//    RS_ActionInterface::coordinateEvent(e);
// potentially, it's possible to use coordinates for selection of arc - yet it seems it is overkill,
// selection by mouse is more convenient so do nothing there
}

void LC_ActionDrawCircleByArc::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCircle:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select arc or circle"),
                                                tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void LC_ActionDrawCircleByArc::updateMouseCursor(){
    switch (getStatus())    {
        case SetCircle:
            graphicView->setMouseCursor(RS2::SelectCursor);
            break;
        default:
            graphicView->setMouseCursor(RS2::CadCursor);
    }
}

void LC_ActionDrawCircleByArc::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_CircleByArcOptions>(nullptr);
}

void LC_ActionDrawCircleByArc::setReplaceArcByCircle(bool value){
    replaceArcByCircle = value;
}

