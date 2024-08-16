/****************************************************************************
**
* Base class for actions that adds some common workflows and utility methods

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

#include <QList>
#include <QMouseEvent>

#include "lc_abstractactionwithpreview.h"
#include "rs_commandevent.h"
#include "rs_commands.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_line.h"
#include "rs_point.h"
#include "lc_refpoint.h"
#include "lc_refline.h"
#include "rs_previewactioninterface.h"
#include "lc_refarc.h"
#include "rs_actioninterface.h"

/**
 * Utility base class for actions. It includes some basic logic and utilities, that simplifies creation of specific actions
 * and reduces repetitive code, as well as defines generic workflow for various action methods processing.
 * Major functions:
 * - basic control for entity highlighting
 * - generic common processing logic for common action events (like mouse up, mouse move)
 * - fine grained methods for specific events (left mouse, right mouse, back etc)
 * - support of  pre-snap during mouse move point (+SHIFT)
 * - support of alternative action mode (by default, invoked by SHIFT on mouse move and click)
 * - support of already selected entities processing on action's invocation
 * - uniform preview handling for mouse move and action triggers
 * - relative point control
 * - free snap enabling/disabling
 * - utilities
 * - defined extensions points

 * @param name
 * @param container
 * @param graphicView
 */
LC_AbstractActionWithPreview::LC_AbstractActionWithPreview(
    const char *name,
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface(name, container, graphicView),
    highlightedEntity{nullptr}{
}


/**
 * Init of action with build-in support of trigger on init invocation (if there are already selected entities
 * and specific action may handle them)
 * @param status status
 */
void LC_AbstractActionWithPreview::init(int status){
    RS_PreviewActionInterface::init(status);
    // check whether we may trigger with such status
    // fixme - refactor to separate methods
    if (doCheckMayTriggerOnInit(status)){
        // collect selected entities
        QList<RS_Entity*> selectedEntities;
        QList<RS_Entity*> entitiesForTrigger;
        for (RS_Entity *e: *container) {
            if (e->isSelected()){
                selectedEntities << e;
                // check whether specific entity is suitable for processing
                if (isAcceptSelectedEntityToTriggerOnInit(e)){
                    entitiesForTrigger << e;
                }
            }
        }
        if (!entitiesForTrigger.isEmpty()){
            showOptions(); // use this as simplest way to read settings for the action
            if (document){
                // take care of undo cycle
                if (isUndoableTrigger()){
                    document->startUndoCycle();
                    // invoke trigger
                    performTriggerOnInit(entitiesForTrigger);
                    document->endUndoCycle();
                } else {
                    performTriggerOnInit(entitiesForTrigger);
                }
            }
            entitiesForTrigger.clear();
            // as we've completed - finish the action
            finishAction();
            // if necessary - unselect currently selected entities
            if (isUnselectEntitiesOnInitTrigger()){
                unSelectEntities(selectedEntities);
            }
            graphicView->redraw(RS2::RedrawDrawing);
        }
        selectedEntities.clear();
    }
}

/**
 * Extension point - defines whether action may be interested on triggering against currently selected entities
 * @param status status on init
 * @return true if may trigger
 */
bool LC_AbstractActionWithPreview::doCheckMayTriggerOnInit([[maybe_unused]]int status){
    return false;
}

/**
 * Extension point - checks whether one of currently selected entities is suitable for triggering action on init
 * @param pEntity entity
 * @return true if may trigger for this entity
 */
bool LC_AbstractActionWithPreview::isAcceptSelectedEntityToTriggerOnInit([[maybe_unused]]RS_Entity *pEntity){
    return false;
}

/**
 * Extension point - will perform deletion of entities (if needed) by inherited actions as part of
 * trigger on init process (i.e - some original selected entities.
 * @param list list of entities
 */
void LC_AbstractActionWithPreview::doPerformOriginalEntitiesDeletionOnInitTrigger([[maybe_unused]]QList<RS_Entity *> &list){}


void LC_AbstractActionWithPreview::updateSnapperAndCoordinateWidget(QMouseEvent* e, [[maybe_unused]]int status){
    // todo - actually, this is a bit ugly to call snap point  - yet as side effect, it will draw snapper and update coordinates widget..
    snapPoint(e);
}

/**
 * Explicitly updates coordinate widget by current mouse position.
 * Method is useful for actions states that do not call snapPoint() method on mouse move (which, in turn, updates the widget)
 * @param e
 */
void LC_AbstractActionWithPreview::doUpdateCoordinateWidgetByMouse(QMouseEvent* e){
    RS_Vector mouse = toGraph(e);
    updateCoordinateWidgetByRelZero(mouse);
}


/**
 * Creation of entities on init trigger
 * @param entities selected entities to trigger
 */
void LC_AbstractActionWithPreview::performTriggerOnInit(QList<RS_Entity*>  entities){
    QList<RS_Entity*> createdEntities;
    // only valid entities are there, so we can create do trigger action for each of them
    for (auto e: entities){
        doCreateEntitiesOnTrigger(e, createdEntities);
    }
    doPerformOriginalEntitiesDeletionOnInitTrigger(entities);
    // do setup of layer and pen and add to drawing
    setupAndAddTriggerEntities(createdEntities);
    createdEntities.clear();
}

/**
 *  Extension point for creating entities on trigger() for provided original entity.
 * @param entity source entity
 * @param list list to which created entities should be added
 */
void LC_AbstractActionWithPreview::doCreateEntitiesOnTrigger([[maybe_unused]]RS_Entity *entity,[[maybe_unused]] QList<RS_Entity *> &list){}

/**
 * Returns true if entities selected on init should be unselected after init trigger
 * @return
 */
bool LC_AbstractActionWithPreview::isUnselectEntitiesOnInitTrigger(){
    return true;
}

/**
 * Default implementation of trigger method. First checks whether all conditions are fine for the trigger via doCheckMayTrigger().
 *
 * If trigger may be executed, checks whether trigger represents undoable operation and if it is so, wraps actual trigger processing into undo cycle.
 * if not - just delegate trigger processing to performTrigger method.
 *
 */
void LC_AbstractActionWithPreview::trigger(){
    RS_PreviewActionInterface::trigger();
    if (doCheckMayTrigger()){
        if (document){
            if (isUndoableTrigger()){
                document->startUndoCycle();
                performTrigger();
                document->endUndoCycle();
            } else {
                performTrigger();
            }
            graphicView->redraw(RS2::RedrawAll);
        }
    }

    // cleanup alternative mode after trigger
    clearAlternativeActionMode();
}

/**
 * Template method for performing trigger operation. Actual processing is delegated to corresponding methods.
 * First, methods executes insertion of entities, than performs deletion of entities.
 * Based on provided point, may set new relative zero.
 * The last step is potential cleanup of data not needed after trigger completion
 */
void LC_AbstractActionWithPreview::performTrigger(){
    performTriggerInsertions();
    performTriggerDeletions();
    RS_Vector newRelativeZeroPosition = doGetRelativeZeroAfterTrigger();
    if (newRelativeZeroPosition.valid){
        graphicView->setRelativeZero(newRelativeZeroPosition);
    }
    doAfterTrigger(); // inherited actions may do additional processing there
}

/**
 * Method that handles insertion entities, that are created during trigger operation.
 * Method collects entities that should be inserted (created in doPrepareTriggerEntities method) in the list,
 * and iterates over them but setting layer and pen (if needed) and adding entities to container and document.
 */
void LC_AbstractActionWithPreview::performTriggerInsertions(){
    QList<RS_Entity*> entities;
    // collect entities that should be created in the list
    doPrepareTriggerEntities(entities);
    setupAndAddTriggerEntities(entities);
    entities.clear();
}

/**
 * Perform setup of layer and pen for entity created for trigger (if needed) and adds entity to container and document
 * @param entities list of entities
 */
void LC_AbstractActionWithPreview::setupAndAddTriggerEntities(const QList<RS_Entity *> &entities){
    // check whether layer and pen should be set to active. If not  - it's up to doPrepareTriggerEntities to perform proper setup of pen and layer
    bool setActiveLayerAndPen = isSetActivePenAndLayerOnTrigger();
    bool undoableTrigger = isUndoableTrigger();
    for (auto ent: entities) {
        if (setActiveLayerAndPen){
            // do setup
            ent->setLayerToActive();
            ent->setPenToActive();
        }
        container->addEntity(ent);
        if (undoableTrigger){
            document->addUndoable(ent);
        }
    }
}

/**
 * Extension method that checks whether all data are collected and trigger may be invoked.
 * @return true if trigger() may be executed.
 */
bool LC_AbstractActionWithPreview::doCheckMayTrigger(){
    return true;
}

/**
 * Controls whether trigger should be within undoable cycle
 * @return true if trigger execution should be within undo cycle
 */
bool LC_AbstractActionWithPreview::isUndoableTrigger(){
    return true;
}

/**
 * Expansion method that controls whether layer and pen should be set to active for entities created as part of trigger.
 * If returns false, it's up to doPrepareTriggerEntities() implementation to perform proper setup.
 * @return true if pen and layer should be set to active, false if doPrepareTriggerEntities of specific action handles that.
 */
bool LC_AbstractActionWithPreview::isSetActivePenAndLayerOnTrigger(){
    return true;
}

/**
 * Extension method that allows to perform some data cleanup after trigger operation completion
 */
void LC_AbstractActionWithPreview::doAfterTrigger(){}

/**
 * Major expansion method that should create entities that should be inserted to the drawing as result of action's trigger.
 * Method should create entities and add to provided list.
 * @param list list of entities to which created entities should be added.
 */
void LC_AbstractActionWithPreview::doPrepareTriggerEntities([[maybe_unused]]QList<RS_Entity *> &list){}

/**
 * Extension method that returns position for setting relative zero after trigger operation execution
 * @return new position of relative zero, invalid position if relative zero should not be updated
 */
RS_Vector LC_AbstractActionWithPreview::doGetRelativeZeroAfterTrigger(){
    return RS_Vector(false);
}

/**
 * Extension method that might be used by inherited actions for performing deletions of some entities from drawing as result of trigger operation execution.
 * Called as part of trigger() execution workflow.
 */
void LC_AbstractActionWithPreview::performTriggerDeletions(){}

/**
 * Default implementation of finish. It removes highlight on entity (if any),
 * allows additional processing and cleanup in inherited doFinish() and calls super method.
 * @param updateTB
 */
void LC_AbstractActionWithPreview::finish(bool updateTB){
    unHighlightEntity();
    doFinish(updateTB);
    RS_PreviewActionInterface::finish(updateTB);
}

/**
 * Extension point for inherited actions for implementing cleanup logic that occurs during action finish()
 * @param updateTB
 */
void LC_AbstractActionWithPreview::doFinish([[maybe_unused]]bool updateTB){}

/**
 * Generic processing of mouse release for simplification of inherited classes
 * Method checks whether shift is pressed, and checks which button is released.
 * Actual processing is delegated to inherited methods.
 * Also, handles preview deletion and unenlightening of entity.
 * @param e mouse event
 */
void LC_AbstractActionWithPreview::mouseReleaseEvent(QMouseEvent *e){
    int status = getStatus();
    bool shiftPressed = isShift(e);
    checkAlternativeActionMode(e, status, shiftPressed);
    Qt::MouseButton button = e->button();
    deletePreview();
    if (button == Qt::LeftButton){
        RS_Vector snapped = doGetMouseSnapPoint(e);
        doOnLeftMouseButtonRelease(e, status, snapped);
        unHighlightEntity();
    } else if (button == Qt::RightButton){
        onRightMouseButtonRelease(e, status);
    }
    clearAlternativeActionMode();
}

/**
 * Alternative mode flags cleanup - at the end of event processing
 */

void LC_AbstractActionWithPreview::clearAlternativeActionMode(){
    alternativeActionMode = false;
}

/**
 * Method that checks whether mouse event triggers alternative mode of action.
 * By default, we consider SHIFT and mouse release as trigger for alternative
 * action mode (that depends on action). However, inherited actions may implement
 * different logic of invocation (say, CTRL etc.).
 * Also, this method may be overridden for more sophisticated modes and modifiers
 * support.
 * @param e mouse event
 */
void LC_AbstractActionWithPreview::checkAlternativeActionMode([[maybe_unused]]const QMouseEvent *e, [[maybe_unused]]int status, bool shiftPressed){
    alternativeActionMode = shiftPressed;
}

/**
 * Returns point that is used as a snap for given mouse event. By default, returns snap point, however, inherited actions may override this and
 * implement different snapping logic (based on status or so) - like angle snap.
 *
 * @param e original mouse event
 * @return point that should be used as snap for mouse even
 */
RS_Vector LC_AbstractActionWithPreview::doGetMouseSnapPoint(QMouseEvent *e){
    RS_Vector snap = snapPoint(e);
    return snap;
}

/**
 * Expansion point for handling left mouse button release event. Should be overridden by inherited actions for doing some meaningful processing.
 * @param e  original mouse event
 * @param status current status of the action
 * @param snapPoint snap point for mouse event (after  doGetMouseSnapPoint method)
 */
void LC_AbstractActionWithPreview::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, [[maybe_unused]]int status, [[maybe_unused]]const RS_Vector &snapPoint){}


/**
 * Default template method implementation for processing of right mouse button release event.
 * Method deletes preview, removes highlighting and delegates actual processing to inherited doBack method.
 * @param e original mouse event
 * @param status current status of action.
 */
void LC_AbstractActionWithPreview::onRightMouseButtonRelease(QMouseEvent *e, int status){
    deletePreview();
    unHighlightEntity();
    doBack(e, status);
}

/**
 * Default implementation for processing back/cancel operation. Simply returns to previous status of the action.
 * Inherited actions should override this method for additional logic.
 * @param pEvent original mouse event
 * @param status current status of the action
 */
void LC_AbstractActionWithPreview::doBack([[maybe_unused]]QMouseEvent *pEvent, int status){
    initPrevious(status);
}

bool LC_AbstractActionWithPreview::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, [[maybe_unused]]int status){
    return true;
}


/**
 * Utility method to highlighting provided entity. That entity will be also stored for further un-highlighting.
 * if more than one entity should be selected by the action, later entities should be highlighted/highlighted explicitly.
 * @param en
 */
void LC_AbstractActionWithPreview::highlightEntity(RS_Entity* en){
    unHighlightEntity();
    highlightedEntity = en;
    highlightedEntity->setHighlighted(true);
    graphicView->drawEntity(highlightedEntity);
}

/**
 * Un-highlights previously highlighted and saved entity. Method is pair for highlightEntity() method.
 */
void LC_AbstractActionWithPreview::unHighlightEntity(){
    if(highlightedEntity){
        highlightedEntity->setHighlighted(false);
        graphicView->drawEntity(highlightedEntity);
        highlightedEntity = nullptr;
    }
}

/**
 * Highlights/de-highlights specific entity - yet do not store it for later un-highlighting.
 * @param en entity to highlight
 * @param highlight true if the entity should be highlighted, false otherwise
 */
void LC_AbstractActionWithPreview::highlightEntityExplicit(RS_Entity* en, bool highlight){
    en->setHighlighted(highlight);
    graphicView->drawEntity(highlightedEntity);
}

/**
 * Default implementation of mouse move event.
 * Detects whether shift is pressed, delegates some preparations for other method.
 * Checks whether it is necessary to do auto pre-snap to current relative zero, and removes preview.
 * If drawing preview is possible for the action status, determines snap point for mouse event and checks whether previous should be drawn for
 * obtained snap point.
 * If it is decided that preview should be drawn, creates it for determined snap point.
 * At the end, performs post-processing/cleanup via delegation to doMouseMoveEnd method.
 *
 * @param e original mouse event
 */
void LC_AbstractActionWithPreview::mouseMoveEvent(QMouseEvent *e){
    int status = getStatus();
    bool shiftPressed = isShift(e);
    checkAlternativeActionMode(e, status, shiftPressed);
    doMouseMoveStart(status, e);
    checkPreSnapToRelativeZero(status, e);
    status = getStatus();
    deletePreview();
    deleteHighlights();
    if (doCheckMayDrawPreview(e, status)){ // check whether preview may be drawn according to state etc.
        RS_Vector snap = doGetMouseSnapPoint(e);
        bool shouldDrawPreview = onMouseMove(e, snap, status); // delegate processing to inherited actions
        if (shouldDrawPreview){
            unHighlightEntity();
            drawPreviewForPoint(e, snap);
            lastSnapPoint = snap; // store snap point for later use (like redraw preview on options change)
        }
        else{
            drawPreview(); // ensure that preview is refreshed if something (like angle snap mark) is there
        }
        graphicView->redraw();
    }
    else{
        updateSnapperAndCoordinateWidget(e, status);
    }
    drawHighlights();
    doMouseMoveEnd(status, e);
    clearAlternativeActionMode();
}

/**
 * Extension point for inherited actions. Called on start of mouse move event processing and lets the action to do some preparations.
 * @param status status of the action
 * @param pEvent mouse event
 */
void LC_AbstractActionWithPreview::doMouseMoveStart([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *pEvent){}

/**
 * Extension point for inherited actions. Called at the of mouse move event processing and lets the action to do some cleanup after mouse move event processing.
 * @param status status of the action
 * @param pEvent mouse event
 */
void LC_AbstractActionWithPreview::doMouseMoveEnd([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e){}


/**
 * Performs initial pre-snap to current relative zero point if shift is pressed.
 * This implementation allows to support nice snapping to relative point and use it, say, as a start point for line, or as corner of shape etc. using the mouse
 * without relying on command.
 * First checks shift state, and if one is pressed - determines whether current status of the action allows performing pre-snap.
 * If it is allowed, determines relative zero point and delegates actual processing of snap to the doInitialSnapToRelativeZero method.
 *
 * @param status current status of the action
 * @param e original mouse event
 */
void LC_AbstractActionWithPreview::checkPreSnapToRelativeZero(int status, QMouseEvent *e){
    if (doCheckMouseEventValidForInitialSnap(e)){ // do pre-snap if SHIFT Pressed
        // check whether status is valid for pre-snap
        if (status == doGetStatusForInitialSnapToRelativeZero()){
            RS_Vector relZero = graphicView->getRelativeZero();
            if (relZero.valid){
                // do actual pre-snap in delegate
                doInitialSnapToRelativeZero(relZero);
            }
        }
    }
}

/**
 * Method checks that mouse event is in general applicable for initial snap to relative zero.
 * By default, returns value of alternativeActionMode (which is set if SHIFT modified is part of
 * mouse event). However, subsequent actions may oeverride it and use different modifiers or
 * additional logic.
 * @param e mouse event
 * @return true if initial pre-snap to relative zero may be triggered by the mouse event
 */
bool LC_AbstractActionWithPreview::doCheckMouseEventValidForInitialSnap([[maybe_unused]]QMouseEvent *e){
    return alternativeActionMode;
}


/**
 * Method return status of the action in which pre-snap to relative zero is allowed. If there are more than one status is allowed - well, this function in inherited
 * action should be coded accordingly. By default, returns negative status (so do not allow pre-snap)
 * @return status in which pre-snap to relative point is allowed.
 */
int LC_AbstractActionWithPreview::doGetStatusForInitialSnapToRelativeZero(){
    return -1;
}

/**
 * Extension point for inherited actions. The method does actual processing of snapping to relative zero point.
 * In most scenarios within inherited actions the method will simply use relative zero as one of state-related points (i.e - start point of line) and
 * update the status of the action accordingly (to move to the next status).
 * @param relZero relative zero point.
 */
void LC_AbstractActionWithPreview::doInitialSnapToRelativeZero([[maybe_unused]]RS_Vector relZero){}


/**
 * Expansion point for inherited actions. Method is called as part of mouse move processing, and if returns true, preview will be created.
 * @param e mouse event
 * @param snap snap point for event
 * @param status status of the action
 * @return true if preview should be shown, false otherwise.
 */
bool LC_AbstractActionWithPreview::onMouseMove([[maybe_unused]]QMouseEvent *e, [[maybe_unused]]RS_Vector snap, [[maybe_unused]]int status){ return true;}

/**
 * Draws preview for given point.
 * Method delegates actual building of preview to doPreparePreviewEntities and collects entities created by that method in the list.
 *
 * Collected entities are added to the preview and draw preview is initiated.
 * @param e
 * @param snap base point (snap point) for preview displaying.
 */
void LC_AbstractActionWithPreview::drawPreviewForPoint(QMouseEvent *e, RS_Vector& snap){
    QList<RS_Entity*> entitiesForPreview; // here we'll collect the list of entities for preview
    // do actual creation of preview entities
    doPreparePreviewEntities(e, snap, entitiesForPreview, getStatus());
    // adding collected entities to preview
    for (auto ent: entitiesForPreview){
        previewEntity(ent);
    }
    // draw
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

/**
 * Major method that should be overridden by inherited actions for preparing preview.
 *
 * THe method relies in provided snap point and should perform necessary calculations of preview - and add created entities to provided list.
 *
 * It should not deal with preview directly, just add created and calculated entities to the list.
 *
 * @param e original mouse event
 * @param snap snap point calculated for the event
 * @param list list of entities to which preview entities should be added
 * @param status current status of the action
 */
void LC_AbstractActionWithPreview::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, [[maybe_unused]]RS_Vector &snap, [[maybe_unused]]QList<RS_Entity *> &list, [[maybe_unused]]int status){}




/**
 * Returns cursor for the given state. Default implementation returns CadCursor, inherited actions may add more sophisticated processing.
 * @param status status of the action
 * @return cursor
 */
RS2::CursorType LC_AbstractActionWithPreview::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::CadCursor;
}

/**
 * Utility method for finishing action.
 */
void LC_AbstractActionWithPreview::finishAction(){
    init(-1);
    updateMouseButtonHints();
    finish(true);
    graphicView->repaint();
}

/**
 * Utility method for restoring snap mode from saved state.
 * May be used by actions that would like to control snap mode during their operations (say, for simplicity of entities selection).
 */
void LC_AbstractActionWithPreview::restoreSnapMode(){
    RS_SnapMode restoredMode = RS_SnapMode::fromInt(savedSnapMode);
    setGlobalSnapMode(restoredMode);
}

/**
 * Utility method for setting snap mode for action and for action handler
 * @param mode new snap mode
 */
void LC_AbstractActionWithPreview::setGlobalSnapMode(const RS_SnapMode &mode){
    setSnapMode(mode);
    if (actionhandler != nullptr){
        actionhandler->slotSetSnaps(mode);
    }
}
/**
 * Utility method that sets free snap mode
 */
void LC_AbstractActionWithPreview::setFreeSnap(){
    RS_SnapMode* currentSnapMode = getSnapMode();
    savedSnapMode = RS_SnapMode::toInt(*currentSnapMode);
    currentSnapMode->clear();
    currentSnapMode->snapFree = true;
    setGlobalSnapMode(*currentSnapMode);
}

/**
 * Utility method for un-selecting list of provided entities
 * @param entities list of entities
 */
void LC_AbstractActionWithPreview::unSelectEntities(const QList<RS_Entity*>& entities){
    for (auto original: entities) {
        original ->setSelected(false);
    }
}

/**
 * Utility method that applies pen and layer to target entity based on provided source entity and provided modes
 * @param source source entity to pick attributes (if needed)
 * @param target entity to which attributes are applied
 * @param penMode value of pen mode enum
 * @param layerMode value of layer mode enum
 */
void LC_AbstractActionWithPreview::applyPenAndLayerBySourceEntity(const RS_Entity *source, RS_Entity *target, int penMode, int layerMode) const{
    switch (penMode) {
        case PEN_ACTIVE: {
            target->setPenToActive();
            break;
        }
        case PEN_ORIGINAL: {
            target->setPen(source->getPen());
            break;
        }
        case PEN_ORIGINAL_RESOLVED: {
            RS_Pen pen = source->getPen(true);
            target->setPen(pen);
            break;
        }
        default:
            break;
    }

    switch (layerMode) {
        case LAYER_ACTIVE: {
            target->setLayerToActive();
            break;
        }
        case LAYER_ORIGINAL: {
            RS_Layer *layer = source->getLayer(true);
            target->setLayer(layer);
            break;
        }
        default:
            break;
    }
}

void LC_AbstractActionWithPreview::updateMouseButtonHints(){
    updateMouseWidget();
}

/**
 * utility method that checks that provided entity is not locked and is not part of polyline.
 * Mostly used for operations that affects original entity and divide it to segments
 * @param e
 * @param entityName
 * @return
 */
bool LC_AbstractActionWithPreview::checkMayExpandEntity(const RS_Entity *e, const QString &entityName) const{
    bool mayDivide = false;
    bool locked = e->isLocked();
    if (locked){
        if (!entityName.isEmpty()){
            commandMessage(entityName + tr(" is not divided as it is locked."));
        }
    } else {
        RS_EntityContainer *pContainer = e->getParent();
        if (pContainer != nullptr){
            if (pContainer->is(RS2::EntityPolyline)){
                mayDivide = false;
                if (!entityName.isEmpty()){
                    commandMessage(entityName + tr(" is not divided as it is part of polyline. Expand polyline first."));
                }
            } else {
                mayDivide = true;
            }
        } else {
            mayDivide = true;
        }
    }
    return mayDivide;
}

/**
 * Utility method that created point for given coordinate and adds it to the list of entities
 * @param list list of entities to add
 * @param coord point coordinates
 * @return created point entity
 */
RS_Point* LC_AbstractActionWithPreview::createPoint(const RS_Vector &coord, QList<RS_Entity *> &list) const{
    auto *result = new RS_Point(container, coord);
    list << result;
    return result;
}

/**
 * utility method that created reference point
 * @param coord
 * @param list
 * @return
 */
void LC_AbstractActionWithPreview::createRefPoint(const RS_Vector &coord, QList<RS_Entity *> &list) const{
    auto *result = new LC_RefPoint(preview.get(), coord, refPointSize, refPointMode);
    list << result;
}

void LC_AbstractActionWithPreview::createRefSelectablePoint(const RS_Vector &coord, QList<RS_Entity *> &list) const{
        auto *result = new LC_RefPoint(preview.get(), coord, refPointSize, refPointMode);
        result->setHighlighted(true);
        list << result;
}

/**
 * Utility method that created line for given coordinate and adds it to the list of entities
 * @param startPoint line start point
 * @param endPoint  line end point
 * @param list list of entities to add line
 * @return created line
 */
RS_Line* LC_AbstractActionWithPreview::createLine(const RS_Vector &startPoint, const RS_Vector &endPoint, QList<RS_Entity *> &list) const{
    auto *result = new RS_Line(container, startPoint, endPoint);
    list << result;
    return result;
}

RS_Line* LC_AbstractActionWithPreview::createLine(const RS_LineData &lineData, QList<RS_Entity *> &list) const{
    auto *result = new RS_Line(container, lineData);
    list << result;
    return result;
}

void LC_AbstractActionWithPreview::createRefLine(const RS_Vector &startPoint, const RS_Vector &endPoint, QList<RS_Entity *> &list) const{
        auto *result = new LC_RefLine(preview.get(), startPoint, endPoint);
        list << result;
}

void LC_AbstractActionWithPreview::createRefArc(const RS_ArcData &data, QList<RS_Entity *> &list) const{
        auto *result = new LC_RefArc(preview.get(), data);
        list << result;
}

bool LC_AbstractActionWithPreview::isMouseMove(QMouseEvent *e){
    return e->type() == QMouseEvent::MouseMove;
}
