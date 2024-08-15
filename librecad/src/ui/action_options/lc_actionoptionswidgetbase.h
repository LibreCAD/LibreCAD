#ifndef LC_ACTIONOPTIONSWIDGETBASE_H
#define LC_ACTIONOPTIONSWIDGETBASE_H

#include "lc_actionoptionswidget.h"

class LC_ActionOptionsWidgetBase:public LC_ActionOptionsWidget
{
public:
    LC_ActionOptionsWidgetBase(RS2::ActionType actionType, const QString &optionsGroupName, const QString &optionNamePrefix);
    ~LC_ActionOptionsWidgetBase() override;

protected:
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    QString getSettingsGroupName() override;
    QString getSettingsOptionNamePrefix() override;

private:
    RS2::ActionType supportedActionType;
    QString settingsGroupName;
    QString settingsOptionNamePrefix;
};

#endif // LC_ACTIONOPTIONSWIDGETBASE_H
