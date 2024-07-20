#include "lc_actionpreselectionawarebase.h"
#include "rs_document.h"


LC_ActionPreSelectionAwareBase::LC_ActionPreSelectionAwareBase(
    const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
    const QList<RS2::EntityType> &entityTypeList, const bool countSelectionDeep)
    :RS_ActionSelectBase(name, container, graphicView, entityTypeList),
    countDeep(countSelectionDeep){}

void LC_ActionPreSelectionAwareBase::init(int status) {
    RS_PreviewActionInterface::init(status);
    unsigned int selectedCount = countSelectedEntities();
    if (selectedCount> 0){
        selectionCompleted(false);
    }
}

unsigned int LC_ActionPreSelectionAwareBase::countSelectedEntities() {
    unsigned int selectedCount = document->countSelected(countDeep, catchForSelectionEntityTypes);
    return selectedCount;
}

void LC_ActionPreSelectionAwareBase::selectionFinishedByKey([[maybe_unused]]QKeyEvent *e, bool escape) {
    if (escape){
        finish(false);
    }
    else{
        selectionCompleted(false);
    }
}

void LC_ActionPreSelectionAwareBase::mouseLeftButtonReleaseEvent(int status, QMouseEvent *e) {
    if (selectionComplete){
        mouseLeftButtonReleaseEventSelected(status, e);
    }
    else{
        entityToSelect = catchEntity(e, catchForSelectionEntityTypes);
        if (entityToSelect != nullptr){
            selectEntity();
            if (isControl(e)){
                selectionCompleted(true);
            }
        }
    }
}

void LC_ActionPreSelectionAwareBase::mouseRightButtonReleaseEvent(int status, QMouseEvent *e) {
    if (selectionComplete) {
        mouseRightButtonReleaseEventSelected(status, e);
        // fixme - return to selection mode before finish
    }
    else{
        finish(false);
    }
}

void LC_ActionPreSelectionAwareBase::mouseMoveEvent(QMouseEvent *event) {
    if (selectionComplete){
        mouseMoveEventSelected(event);
    }
    else{
        selectionMouseMove(event);
    }
}

void LC_ActionPreSelectionAwareBase::updateMouseButtonHints() {
    if (selectionComplete){
        updateMouseButtonHintsForSelected(getStatus());
    }
    else{
        updateMouseButtonHintsForSelection();
    }
}

void LC_ActionPreSelectionAwareBase::selectionCompleted([[maybe_unused]]bool singleEntity) {
    setSelectionComplete(isAllowTriggerOnEmptySelection());
    updateMouseButtonHints();
    if (selectionComplete) {
        trigger();
        if (singleEntity) {
            deselectAll();
            selectionComplete = false; // continue with selection, don't finish
        } else {
            setStatus(-1);
        }
        updateSelectionWidget();
    }
}

void LC_ActionPreSelectionAwareBase::setSelectionComplete(bool allowEmptySelection) {
        unsigned int selectedCount = countSelectedEntities();
    bool proceed = selectedCount > 0 || allowEmptySelection;
    if (proceed) {
        selectionComplete = true;
        updateMouseButtonHintsForSelected(getStatus());
    }
    else{
        commandMessageTR("No valid entities selected, select them first");
    }
}

void LC_ActionPreSelectionAwareBase::updateMouseButtonHintsForSelected([[maybe_unused]]int status) {
    updateMouseWidget();
}

void LC_ActionPreSelectionAwareBase::mouseLeftButtonReleaseEventSelected([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::mouseRightButtonReleaseEventSelected([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::mouseMoveEventSelected([[maybe_unused]]QMouseEvent *e) {}

RS2::CursorType LC_ActionPreSelectionAwareBase::doGetMouseCursor(int status) {
    if (selectionComplete){
        return doGetMouseCursorSelected(status);
    }
    else {
        return RS_ActionSelectBase::doGetMouseCursor(status);
    }
}

RS2::CursorType LC_ActionPreSelectionAwareBase::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}
