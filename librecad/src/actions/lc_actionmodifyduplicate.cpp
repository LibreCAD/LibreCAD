#include "lc_actionmodifyduplicate.h"
#include "lc_linemath.h"
#include "lc_duplicateoptions.h"
#include <QList>

LC_ActionModifyDuplicate::LC_ActionModifyDuplicate(RS_EntityContainer &container, RS_GraphicView &graphicView):
    LC_AbstractActionWithPreview("ModifyDuplicate", container, graphicView),
    offsetX(0), offsetY(0){
    actionType = RS2::ActionModifyDuplicate;
}
LC_ActionModifyDuplicate::~LC_ActionModifyDuplicate() = default;

void LC_ActionModifyDuplicate::init(int status){
    LC_AbstractActionWithPreview::init(status);
    if (status == SelectEntity){

        for (RS_Entity *e: *container) {
            if (e->isSelected()){
                selectedEntities << e;
            }
        }

        if (!selectedEntities.isEmpty()){
            trigger();
            finishAction();
        }
    }
}

bool LC_ActionModifyDuplicate::doCheckMayTrigger(){
    return !selectedEntities.isEmpty();
}

void LC_ActionModifyDuplicate::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    for (int i =0; i < selectedEntities.count(); i++){
        RS_Entity* original = selectedEntities.at(i);
        RS_Vector offset = getOffset();
        RS_Entity* clone = original->clone();
        if (clone != nullptr){
            if (offset.valid){
                clone->move(offset);
            }
            list<<clone;
        }
    }
}

RS_Vector LC_ActionModifyDuplicate::getOffset() const{
    bool moveX = LC_LineMath::isMeaningful(offsetX);
    bool moveY = LC_LineMath::isMeaningful(offsetY);
    RS_Vector offset(false);

    if (moveX || moveY){
        double mx = LC_LineMath::getMeaningful(offsetX);
        double my = LC_LineMath::getMeaningful(offsetY);
        offset = RS_Vector(mx, my, 0.0);
    }
    return offset;
}

// todo - potentially, this is candidate for options
#define CLEAR_SELECTION_AFTER_TRIGGER true

void LC_ActionModifyDuplicate::doAfterTrigger(){
    LC_AbstractActionWithPreview::doAfterTrigger();
    if (CLEAR_SELECTION_AFTER_TRIGGER){
        for (int i = 0; i < selectedEntities.count(); i++) {
            RS_Entity *original = selectedEntities.at(i);
            original ->setSelected(false);
        }
    }
    selectedEntities.clear();
}

void LC_ActionModifyDuplicate::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint){
    if (status == SelectEntity){
        RS_Entity *en = catchEntity(e, RS2::ResolveNone);
        if (en != nullptr){ // can snap to line
            selectedEntities << en;
            trigger();
            unHighlightEntity();
            deletePreview();
        }
    }
}

void LC_ActionModifyDuplicate::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    RS_Entity *en = catchEntity(e, RS2::ResolveNone);

    switch (status) {
        case SelectEntity: {
            if (en != nullptr){ // can snap to line
                highlightEntity(en);
                RS_Vector offset = getOffset();
                if (offset.valid){
                    RS_Entity* clone = en->clone();
                    clone->move(offset);
                    list << clone;
                }
            }
            break;
        }
    }
}

void LC_ActionModifyDuplicate::updateMouseButtonHints(){
    switch (getStatus()){
        case SelectEntity:
            updateMouseWidgetTR("Select entity to duplicate", "Cancel");
            break;
        default:
            RS_ActionInterface::updateMouseButtonHints();
    }
}

void LC_ActionModifyDuplicate::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_DuplicateOptions>(nullptr);
}

RS2::CursorType LC_ActionModifyDuplicate::doGetMouseCursor(int status){
    return RS2::SelectCursor;
}


