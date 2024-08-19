#include <QKeyEvent>
#include "lc_actionmodifyselectionbase.h"


LC_ActionModifySelectionBase::LC_ActionModifySelectionBase(const char *name,
                                                           RS_EntityContainer &container,
                                                           RS_GraphicView &graphicView, RS2::ActionType actionType)
                                                           :RS_PreviewActionInterface(name, container, graphicView,actionType){}

void LC_ActionModifySelectionBase::mouseMoveEvent(QMouseEvent *event){
    RS_ActionInterface::mouseMoveEvent(event);
}

void LC_ActionModifySelectionBase::mouseReleaseEvent(QMouseEvent *event){
    RS_ActionInterface::mouseReleaseEvent(event);
}

int LC_ActionModifySelectionBase::countSelected() {
    unsigned int ret=container->countSelected();
    // fixme - ensure that this is correct place for method
    if (ret==0) {
        commandMessage(tr("No entity selected!"));
    }
    return ret;
}

void LC_ActionModifySelectionBase::keyPressEvent(QKeyEvent *e){
    if (e->key()==Qt::Key_Enter && countSelected() > 0)
    {
        selectionFinished = true;
    }
}
