#ifndef LC_ACTIONGROUPMANAGER_H
#define LC_ACTIONGROUPMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>

#include "rs.h"

class LC_ShortcutsManager;
class LC_ShortcutInfo;
class LC_ActionGroup;
class QAction;
class QC_ApplicationWindow;

class LC_ActionGroupManager : public QObject{
    Q_OBJECT
public:
    explicit LC_ActionGroupManager(QC_ApplicationWindow *parent);
    ~LC_ActionGroupManager() override;


    QList<LC_ActionGroup*> toolGroups() const;
    QMap<QString, LC_ActionGroup*> allGroups();
    QList<LC_ActionGroup *> allGroupsList();
    void sortGroupsByName(QList<LC_ActionGroup*>& list);
    void assignShortcutsToActions(QMap<QString, QAction *> &map,  std::vector<LC_ShortcutInfo> &shortcutsList) const;
    int loadShortcuts(const QMap<QString, QAction *> &map);
    int loadShortcuts(const QString &fileName, QMap<QString, QKeySequence> *result) const;
    int saveShortcuts(QMap<QString, LC_ShortcutInfo *> map);
    int saveShortcuts(const QList<LC_ShortcutInfo *> &shortcutsList, const QString &fileName) const;
    const QString getShortcutsMappingsFolder() const;
    QMap<QString, QAction *> &getActionsMap();
    QAction *getActionByName(const QString &name) const;
    bool hasActionGroup(const QString& categoryName) const;
    LC_ActionGroup* getActionGroup(const QString& groupName);
    bool isActionTypeSetsTheIcon(RS2::ActionType actionType);
    void completeInit();
    QAction* getActionByType(RS2::ActionType actionType);
    static void associateQActionWithActionType(QAction* action, RS2::ActionType actionType);
    void persist();
    LC_ActionGroup* getGroupByName(const QString &name) const;
    void addActionGroup(const QString &name, LC_ActionGroup *actionGroup, bool isToolsGroup);
public slots:
    void toggleExclusiveSnapMode(bool state); // fixme - sand - refactor later!!! Should be out of generic AGM?
    void toggleTools(bool state);
    void onOptionsChanged() const;
private:
    QList<LC_ActionGroup *> m_toolsGroups;
    QMap<QString, LC_ActionGroup*> m_actionGroups;
    QMap<QString, QAction*> m_actionsMap; // should be initialized by action factory by call of loadShortcuts()
    QMap<int, QAction*> m_actionsByTypes;
    std::unique_ptr<LC_ShortcutsManager> m_shortcutsManager;
    QList<bool> snap_memory;
};

#endif // LC_ACTIONGROUPMANAGER_H
