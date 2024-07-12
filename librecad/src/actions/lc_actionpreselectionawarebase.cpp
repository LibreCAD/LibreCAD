#include "lc_actionpreselectionawarebase.h"
#include "rs_document.h"


LC_ActionPreSelectionAwareBase::LC_ActionPreSelectionAwareBase(
    const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
    const QList<RS2::EntityType> &entityTypeList)
    :RS_ActionSelectBase(name, container, graphicView, entityTypeList) {}

void LC_ActionPreSelectionAwareBase::init(int status) {
    RS_PreviewActionInterface::init(status);
    if (document->countSelected() > 0){
        selectionCompleted(false);
    }
}

void LC_ActionPreSelectionAwareBase::selectionFinishedByKey(QKeyEvent *e, bool escape) {
    if (escape){
        finish(false);
    }
    else{
        selectionComplete = true;
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

void LC_ActionPreSelectionAwareBase::selectionCompleted(bool singleEntity) {}

void LC_ActionPreSelectionAwareBase::updateMouseButtonHintsForSelected(int status) {
    updateMouseWidget();
}

void LC_ActionPreSelectionAwareBase::mouseLeftButtonReleaseEventSelected(int status, QMouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::mouseRightButtonReleaseEventSelected(int status, QMouseEvent *pEvent) {}

void LC_ActionPreSelectionAwareBase::mouseMoveEventSelected(QMouseEvent *e) {}
