#include <QAction>
#include <QActionGroup>
#include "lc_actionfactorybase.h"
#include "qg_actionhandler.h"

LC_ActionFactoryBase::LC_ActionFactoryBase(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler):
    QObject(parent), main_window(parent),action_handler(a_handler){
}

QAction *LC_ActionFactoryBase::createAction_MW(const char *name, const char *slot, const char *text,
                                           const char *iconName, const char *themeIconName,
                                           QActionGroup *parent, QMap<QString, QAction *> &a_map, bool useToggled) const {
    QAction *action = doCreateActionTR(a_map, name, text, iconName, themeIconName, parent);
    if (slot != nullptr) {
        if (useToggled) {
            connect(action, SIGNAL(toggled(bool)), main_window, slot);
        } else {
            connect(action, SIGNAL(triggered(bool)), main_window, slot);
        }
    }
    return action;
}

QAction * LC_ActionFactoryBase::createAction_AH(const char* name, RS2::ActionType actionType, const char* text,
                                            const char *iconName, const char *themeIconName,
                                            QActionGroup *parent,
                                            QMap<QString, QAction *> &a_map) const{
    QAction *action = doCreateActionTR(a_map, name, text, iconName, themeIconName, parent);
    // LC_ERR <<  " ** original action handler" << this->action_handler;
    // well, a bit crazy hacky code to let the lambda properly capture action handler... without local var, class member is not captured
    QG_ActionHandler* capturedHandler = action_handler;
    QObject::connect(action, &QAction::triggered, capturedHandler, [ capturedHandler, actionType](bool){
        // LC_ERR << " ++ captured action handler "<<   capturedHandler;
        capturedHandler->setCurrentAction(actionType);
    });
    return action;
}

QAction *LC_ActionFactoryBase::doCreateActionTR(QMap<QString, QAction *> &a_map, const char* name,
                                                const char* text, const char *iconName, const char *themeIconName,
                                                QActionGroup *parent, const char* textDisambiguation) const {
    auto* action = new QAction( tr(text, textDisambiguation), parent);
    if (iconName != nullptr) {
        QIcon icon = QIcon(iconName);
        if (using_theme && themeIconName != nullptr)
            action->setIcon(QIcon::fromTheme(themeIconName, icon));
        else
            action->setIcon(icon);
    }
    action->setObjectName(name);
    a_map[name] = action;
    return action;
}

void LC_ActionFactoryBase::createActionHandlerActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList) const {
    for (const LC_ActionFactoryBase::ActionInfo a: actionList){
        createAction_AH(a.key, a.actionType, a.text, a.iconName,a.themeIconName, group, map);
    }
}

void LC_ActionFactoryBase::createMainWindowActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList, bool useToggled) const {
    for (const LC_ActionFactoryBase::ActionInfo a: actionList){
        createAction_MW(a.key, a.slot, a.text, a.iconName, a.themeIconName, group, map, useToggled);
    }
}

void LC_ActionFactoryBase::assignShortcutsToActions(
    const QMap<QString, QAction *> &map, std::vector<ActionShortCutInfo> &shortcutsList) const {
    for (const ActionShortCutInfo &a: shortcutsList){
        QAction* createdAction = map[a.key];
        if (createdAction != nullptr){
            if (!a.keySequencesList.isEmpty()){
                createdAction->setShortcuts(a.keySequencesList);
            }
            else if (a.keySequence != QKeySequence::UnknownKey){
                createdAction->setShortcut(a.keySequence);
            }
        }
    }
}


