#include "lc_actionoptionswidget.h"
#include "rs_actioninterface.h"
#include "rs_debug.h"

LC_ActionOptionsWidget::LC_ActionOptionsWidget(QWidget *parent) :
    QWidget(parent)
{
}

LC_ActionOptionsWidget::~LC_ActionOptionsWidget(){}

void LC_ActionOptionsWidget::hideOptions(){
    hide();
    saveSettings();
}

void LC_ActionOptionsWidget::setAction(RS_ActionInterface *a, bool update){
    if (a != nullptr){
        RS2::ActionType actionType = a->rtti();
        if (checkActionRttiValid(actionType)){
            doSetAction(a, update);
        }
        else{
            // fixme - review
            RS_DEBUG->print(RS_Debug::D_ERROR, typeid(*this).name(), "::setAction: wrong action type");
            clearAction();
        }
    }
}

bool LC_ActionOptionsWidget::checkActionRttiValid(RS2::ActionType actionType){
    return false;
}
