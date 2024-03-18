#include "lc_abstract_action_draw_line.h"
#include "lc_actiondrawlinerel.h"
#include "rs_debug.h"
#include "rs_actioneditundo.h"
#include "rs_commands.h"
#include "rs_actionpolylinesegment.h"
#include "lc_lineoptions.h"
#include "lc_linemath.h"
#include "lc_actiondrawlinepoints.h"
#include "lc_linepointsoptions.h"
#include <rs_point.h>
#include "rs_math.h"
#include "rs_dialogfactory.h"
#include "rs_coordinateevent.h"
#include "rs_commandevent.h"
#include "rs_document.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_polyline.h"
#include "lc_abstractactiondrawrectangle.h"
#include "QMouseEvent"
#include <cmath>
#include "lc_actiondrawrectangle2points.h"
#include "lc_rectangle2pointsoptions.h"
#include "lc_abstractactionwithpreview.h"

LC_AbstractActionWithPreview::LC_AbstractActionWithPreview(
    const char *name,
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface(name, container, graphicView),
    highlightedEntity{nullptr}{
}


bool LC_AbstractActionWithPreview::doCheckMayDrawPreview(QMouseEvent *event, int status){
    return true;
}


void LC_AbstractActionWithPreview::unHighlightEntity(){
    if(highlightedEntity){
        highlightedEntity->setHighlighted(false);
        graphicView->drawEntity(highlightedEntity);
        highlightedEntity = nullptr;
    }
}

void LC_AbstractActionWithPreview::deleteEntityUndoable(RS_Entity *entity){
    // delete and add this into undo
    graphicView->deleteEntity(entity);
    entity->changeUndoState();
    document->addUndoable(entity);
}

void LC_AbstractActionWithPreview::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
    unHighlightEntity();
    doFinish(updateTB);
}

void LC_AbstractActionWithPreview::highlightEntity(RS_Entity* en){
    unHighlightEntity();
    highlightedEntity = en;
    highlightedEntity->setHighlighted(true);
    graphicView->drawEntity(highlightedEntity);
}

void LC_AbstractActionWithPreview::highlightEntityExplicit(RS_Entity* en, bool highlight){
    en->setHighlighted(highlight);
    graphicView->drawEntity(highlightedEntity);
}

void LC_AbstractActionWithPreview::commandEvent(RS_CommandEvent *e){
    QString const &c = e->getCommand().toLower().trimmed();
    bool accept = doProcessCommand(e, c);
    if (accept){
        e->accept();
    }
}
bool LC_AbstractActionWithPreview::doProcessCommand(RS_CommandEvent *e, const QString &c){
    return false;
}

/**
 * Generic processing of mouse release for simplification of inherited classes
 * @param e
 */
void LC_AbstractActionWithPreview::mouseReleaseEvent(QMouseEvent *e){
    int status = getStatus();
    Qt::MouseButton button = e->button();
    deletePreview();
    if (button == Qt::LeftButton){
        RS_Vector snapped = doGetMouseSnapPoint(e);
        doOnLeftMouseButtonRelease(e, status, snapped);
    } else if (button == Qt::RightButton){
        onRightMouseButtonRelease(e, status);
    }
}

void LC_AbstractActionWithPreview::onRightMouseButtonRelease(QMouseEvent *e, int status){
    deletePreview();
    unHighlightEntity();
    doBack(e, status);
}

void LC_AbstractActionWithPreview::updateMouseCursor(){
    int status = getStatus();
    RS2::CursorType cursor = doGetMouseCursor(status);
    if (cursor > 0){
        graphicView->setMouseCursor(cursor);
    }
}

void LC_AbstractActionWithPreview::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint){

}

void LC_AbstractActionWithPreview::mouseMoveEvent(QMouseEvent *e){
    int status = getStatus();
    deletePreview();
    if (doCheckMayDrawPreview(e, status)){ // check whether preview may be drawn according to state etc.
        RS_Vector snap = doGetMouseSnapPoint(e);
        bool drawPreview = onMouseMove(e, snap, status); // delegate processing to inherited actions
        if (drawPreview){
            unHighlightEntity();
            drawPreviewForPoint(e, snap);
            lastSnapPoint = snap; // store snap point for later use (like redraw preview on options change)
        }
        graphicView->redraw();
    }
}

void LC_AbstractActionWithPreview::drawPreviewForPoint(QMouseEvent *e, RS_Vector& snap){
    QList<RS_Entity*> entitiesForPreview;
    doPreparePreviewEntities(e, snap, entitiesForPreview, getStatus());
    for (int i =0; i < entitiesForPreview.count(); i++){
        RS_Entity* ent = entitiesForPreview.at(i);
        preview->addEntity(ent);
    }
    drawPreview();
}

/**
 * redraws preview in it's last point. This is used if the user would like to change some option
 * via options widget - and thus we'll reflect such change in preview without waiting for the next mouse move event
 */
void LC_AbstractActionWithPreview::drawPreviewForLastPoint(){
    deletePreview();
    if (lastSnapPoint.valid){
        if (doCheckMayDrawPreview(nullptr, getStatus())){
            drawPreviewForPoint(nullptr, lastSnapPoint);
            graphicView->redraw();
        }
    }
}

RS_Vector LC_AbstractActionWithPreview::doGetMouseSnapPoint(QMouseEvent *e){
    RS_Vector snap = snapPoint(e);
    return snap;
}

void LC_AbstractActionWithPreview::doFinish(bool updateTB){
}

void LC_AbstractActionWithPreview::doBack(QMouseEvent *pEvent, int status){
     init(status - 1);
}

bool LC_AbstractActionWithPreview::onMouseMove(QMouseEvent *e, RS_Vector snap, int status){
    return true;
}


void LC_AbstractActionWithPreview::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
}

void LC_AbstractActionWithPreview::coordinateEvent(RS_CoordinateEvent *e){
    if (!e) return;
    RS_Vector coord = e->getCoordinate();
    RS_Vector zero = RS_Vector(0, 0, 0);
    bool isZero = coord == zero; // use it to handle "0" shortcut (it is passed as 0,0 vector)
    int status = getStatus();
    onOnCoordinateEvent(coord, isZero, status);
}

void LC_AbstractActionWithPreview::onOnCoordinateEvent(const RS_Vector &coord, bool isZero, int status){
}

void LC_AbstractActionWithPreview::trigger(){
    RS_PreviewActionInterface::trigger();
    if (doCheckMayTrigger()){ // extract and proceed saved polyline
        if (document){
            if (isUndoableTrigger()){
                document->startUndoCycle();
                performTrigger();
                document->endUndoCycle();
            } else {
                performTrigger();
            }
            graphicView->redraw(RS2::RedrawDrawing);
        }
    }
}

void LC_AbstractActionWithPreview::performTrigger(){
    performTriggerInsertions();
    performTriggerDeletions();
    RS_Vector newRelativeZeroPosition = doGetRelativeZeroAfterTrigger();
    if (newRelativeZeroPosition.valid){
        graphicView->setRelativeZero(newRelativeZeroPosition);
    }
    doAfterTrigger(); // inherited actions may do additional processing there
}

void LC_AbstractActionWithPreview::performTriggerInsertions(){
    QList<RS_Entity*> entities;
    doPrepareTriggerEntities(entities);
    bool setActiveLayerAndPen = isSetActivePenAndLayerOnTrigger();

    for (int i = 0; i < entities.count(); i++) {
        RS_Entity *ent = entities.at(i);
        if (setActiveLayerAndPen){
            ent->setLayerToActive();
            ent->setPenToActive();
        }
        container->addEntity(ent);
        document->addUndoable(ent);
    }
    entities.clear();
}

bool LC_AbstractActionWithPreview::doCheckMayTrigger(){
    return true;
}
bool LC_AbstractActionWithPreview::isUndoableTrigger(){
    return true;
}

bool LC_AbstractActionWithPreview::isSetActivePenAndLayerOnTrigger(){
    return true;
}

void LC_AbstractActionWithPreview::doAfterTrigger(){
}

void LC_AbstractActionWithPreview::doPrepareTriggerEntities(QList<RS_Entity *> &list){
}

RS_Vector LC_AbstractActionWithPreview::doGetRelativeZeroAfterTrigger(){
    return RS_Vector(false);
}

void LC_AbstractActionWithPreview::finishAction(){
    init(-1);
    updateMouseButtonHints();
    finish(true);
}

RS2::CursorType LC_AbstractActionWithPreview::doGetMouseCursor(int status){
    return RS2::CadCursor;
}

void LC_AbstractActionWithPreview::performTriggerDeletions(){

}

void LC_AbstractActionWithPreview::updateMouseWidgetTR(const char* left, const char* right){
    RS_DIALOGFACTORY->updateMouseWidget(tr(left),tr(right));
}

void LC_AbstractActionWithPreview::updateMouseWidget(const QString& left,
                                                     const QString& right){
    RS_DIALOGFACTORY->updateMouseWidget(left, right);
}

void LC_AbstractActionWithPreview::commandMessageTR(const char * msg){
    RS_DIALOGFACTORY->commandMessage(msg);
}

void LC_AbstractActionWithPreview::commandMessage(QString msg) const{
    RS_DIALOGFACTORY->commandMessage(msg);
}
