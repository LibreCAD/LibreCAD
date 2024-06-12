#include "lc_actionoptionswidgetbase.h"

LC_ActionOptionsWidgetBase::LC_ActionOptionsWidgetBase(
    RS2::ActionType actionType, const QString &optionsGroupName, const QString &optionNamePrefix):
    LC_ActionOptionsWidget(nullptr){
    supportedActionType  = actionType;
    settingsGroupName = optionsGroupName;
    settingsOptionNamePrefix = optionNamePrefix;
}

LC_ActionOptionsWidgetBase::~LC_ActionOptionsWidgetBase()= default;

bool LC_ActionOptionsWidgetBase::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == supportedActionType;
}

QString LC_ActionOptionsWidgetBase::getSettingsGroupName(){
    return settingsGroupName;
}

QString LC_ActionOptionsWidgetBase::getSettingsOptionNamePrefix(){
    return settingsOptionNamePrefix;
}

