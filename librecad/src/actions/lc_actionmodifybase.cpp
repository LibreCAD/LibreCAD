#include "lc_actionmodifybase.h"


LC_ActionModifyBase::LC_ActionModifyBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView,
    const QList<RS2::EntityType> &entityTypeList, bool countSelectionDeep)
    :LC_ActionPreSelectionAwareBase(name, container, graphicView, entityTypeList, countSelectionDeep){}

void LC_ActionModifyBase::selectionCompleted([[maybe_unused]] bool singleEntity) {
    setSelectionComplete(isAllowTriggerOnEmptySelection());
    updateMouseButtonHints();
    updateSelectionWidget();
}

bool LC_ActionModifyBase::isShowModifyActionDialog() {
    return true; // fixme - add options support
}

void LC_ActionModifyBase::setUseCurrentLayer(bool b) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->useCurrentLayer = b;
}

bool LC_ActionModifyBase::isUseCurrentLayer() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->useCurrentLayer;
}

void LC_ActionModifyBase::setUseCurrentAttributes(bool b) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->useCurrentAttributes = b;
}

bool LC_ActionModifyBase::isUseCurrentAttributes() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->useCurrentAttributes;
}

int LC_ActionModifyBase::getCopiesNumber() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->number;
}

void LC_ActionModifyBase::setCopiesNumber(int value) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->number = value;
}

void LC_ActionModifyBase::setKeepOriginals(bool b) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->keepOriginals = b;
}

void LC_ActionModifyBase::setUseMultipleCopies(bool val) {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    data->multipleCopies = val;
}

bool LC_ActionModifyBase::isUseMultipleCopies() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->multipleCopies;
}

bool LC_ActionModifyBase::isKeepOriginals() {
    LC_ModifyOperationFlags* data = getModifyOperationFlags();
    return data->keepOriginals;
}
