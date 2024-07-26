#ifndef LC_ACTIONFACTORYBASE_H
#define LC_ACTIONFACTORYBASE_H

#include "qc_applicationwindow.h"

class LC_ActionFactoryBase : public QObject{
Q_OBJECT
public:
    LC_ActionFactoryBase(QC_ApplicationWindow* parent, QG_ActionHandler* a_handler);
protected:

    struct ActionShortCutInfo{
        const char* key;
        QKeySequence keySequence = QKeySequence::UnknownKey;
        QList<QKeySequence> keySequencesList;
        ActionShortCutInfo(const char *key, const QKeySequence &shortCut):key(key), keySequence(shortCut) {}
        ActionShortCutInfo(const char *key, const QList<QKeySequence> &shortcutsList):key(key) {
            keySequencesList = shortcutsList;
        }
    };

    struct ActionInfo{
        const char* key;
        RS2::ActionType actionType;
        const char* text = nullptr;
        const char* iconName = nullptr;
        const char* themeIconName = nullptr;
        const char* slot = nullptr;

        ActionInfo(const char *actionKey, RS2::ActionType actionType,const char *text, const char *iconName = nullptr, const char *themeIcon = nullptr)
            :key(actionKey),actionType(actionType),text(text),iconName(iconName), themeIconName(themeIcon){}

        ActionInfo(const char *actionKey, const char* actionSlot, const char *text, const char *iconName=nullptr, const char *themeIcon = nullptr)
            :key(actionKey),actionType(RS2::ActionNone),text(text),iconName(iconName), themeIconName(themeIcon), slot(actionSlot){}
    };

    QC_ApplicationWindow* main_window = nullptr;
    QG_ActionHandler* action_handler = nullptr;
    bool using_theme = false;

    QAction* doCreateActionTR(QMap<QString, QAction *> &a_map, const char* name, const char* text, const char *iconName,
                              const char *themeIconName, QActionGroup *parent, const char* textDisambiguation = nullptr) const;

    QAction *createAction_AH(const char* name, RS2::ActionType actionType, const char* text,
                             const char *iconName, const char *themeIconName,
                             QActionGroup *parent,
                             QMap<QString, QAction *> &a_map) const;

    void createActionHandlerActions(QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList) const;
    void createMainWindowActions( QMap<QString, QAction *> &map, QActionGroup *group, const std::vector<ActionInfo> &actionList, bool useToggled = false) const;
    QAction *createAction_MW(const char *name, const char *slot, const char *text, const char *iconName,  const char *themeIconName,  QActionGroup *parent,
        QMap<QString, QAction *> &a_map, bool toggled = false) const;

    void assignShortcutsToActions(const QMap<QString, QAction *> &map,
                                 std::vector<ActionShortCutInfo> &shortcutsList) const;
};

#endif // LC_ACTIONFACTORYBASE_H
