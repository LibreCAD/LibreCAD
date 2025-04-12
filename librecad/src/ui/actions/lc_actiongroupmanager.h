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

    LC_ActionGroup* block;
    LC_ActionGroup* circle;
    LC_ActionGroup* curve;
    LC_ActionGroup* spline;
    LC_ActionGroup* edit;
    LC_ActionGroup* ellipse;
    LC_ActionGroup* file;
    LC_ActionGroup* dimension;
    LC_ActionGroup* info;
    LC_ActionGroup* layer;
    LC_ActionGroup* line;
    LC_ActionGroup* point;
    LC_ActionGroup* shape;
    LC_ActionGroup* modify;
    LC_ActionGroup* options;
    LC_ActionGroup* other;
    LC_ActionGroup* relZero;
    LC_ActionGroup* polyline;
    LC_ActionGroup* restriction;
    LC_ActionGroup* select;
    LC_ActionGroup* snap;
    LC_ActionGroup* snap_extras;
    LC_ActionGroup* view;
    LC_ActionGroup* namedViews;
    LC_ActionGroup* workspaces;
    LC_ActionGroup* ucs;
    LC_ActionGroup* widgets;
    LC_ActionGroup* pen;
    LC_ActionGroup* infoCursor;

    QList<QAction*> file_actions;
    QList<QAction*> line_actions;
    QList<QAction*> point_actions;
    QList<QAction*> shape_actions;
    QList<QAction*> circle_actions;
    QList<QAction*> curve_actions;
    QList<QAction*> spline_actions;
    QList<QAction*> ellipse_actions;
    QList<QAction*> polyline_actions;
    QList<QAction*> select_actions;
    QList<QAction*> dimension_actions;
    QList<QAction*> other_drawing_actions;
    QList<QAction*> modify_actions;
    QList<QAction*> order_actions;
    QList<QAction*> info_actions;
    QList<QAction*> layer_actions;
    QList<QAction*> block_actions;
    QList<QAction*> pen_actions;

    QList<LC_ActionGroup*> toolGroups();
    QMap<QString, LC_ActionGroup*> allGroups();
    QList<LC_ActionGroup *> allGroupsList();
    void sortGroupsByName(QList<LC_ActionGroup*>& list);
    void assignShortcutsToActions(QMap<QString, QAction *> &map,  std::vector<LC_ShortcutInfo> &shortcutsList);
    int loadShortcuts(const QMap<QString, QAction *> &map);
    int loadShortcuts(const QString &fileName, QMap<QString, QKeySequence> *result);
    int saveShortcuts(QMap<QString, LC_ShortcutInfo *> map);
    int saveShortcuts(const QList<LC_ShortcutInfo *> &shortcutsList, const QString &fileName);
    const QString getShortcutsMappingsFolder();
    QMap<QString, QAction *> &getActionsMap();
    QAction *getActionByName(const QString &name);
    bool hasActionGroup(QString categoryName);
    LC_ActionGroup* getActionGroup(QString groupName);
    void fillActionsList(QList<QAction *> &list, const std::vector<const char *> &actionNames);
    bool isActionTypeSetsTheIcon(RS2::ActionType actionType);
    void completeInit();
    QAction* getActionByType(RS2::ActionType actionType);
    static void associateQActionWithActionType(QAction* action, RS2::ActionType actionType);
    void persist();
public slots:
    void toggleExclusiveSnapMode(bool state);
    void toggleTools(bool state);
    void onOptionsChanged();
private:
    QMap<QString, QAction*> m_actionsMap; // should be initialized by action factory by call of loadShortcuts()
    QMap<int, QAction*> m_actionsByTypes;
    std::unique_ptr<LC_ShortcutsManager> m_shortcutsManager;
    QList<bool> snap_memory;
};

#endif // LC_ACTIONGROUPMANAGER_H
