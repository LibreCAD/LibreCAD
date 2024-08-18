#ifndef LC_ACTIONGROUPMANAGER_H
#define LC_ACTIONGROUPMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "lc_actiongroup.h"
#include "lc_shortcuts_manager.h"


class QAction;
class QC_ApplicationWindow;

class LC_ActionGroupManager : public QObject
{
    Q_OBJECT



public:
    explicit LC_ActionGroupManager(QC_ApplicationWindow *parent);

    LC_ActionGroup* block;
    LC_ActionGroup* circle;
    LC_ActionGroup* curve;
    LC_ActionGroup* edit;
    LC_ActionGroup* ellipse;
    LC_ActionGroup* file;
    LC_ActionGroup* dimension;
    LC_ActionGroup* info;
    LC_ActionGroup* layer;
    LC_ActionGroup* line;
    LC_ActionGroup* modify;
    LC_ActionGroup* options;
    LC_ActionGroup* other;
    LC_ActionGroup* polyline;
    LC_ActionGroup* restriction;
    LC_ActionGroup* select;
    LC_ActionGroup* snap;
    LC_ActionGroup* snap_extras;
    LC_ActionGroup* view;
    LC_ActionGroup* widgets;
    LC_ActionGroup* pen;

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
public slots:
    void toggleExclusiveSnapMode(bool state);
    void toggleTools(bool state);
    void onOptionsChanged();
private:
    QMap<QString, QAction*> a_map; // should be initialized by action factory by call of loadShortcuts()
    LC_ShortcutsManager shortcutsManager;
    QList<bool> snap_memory;


};

#endif // LC_ACTIONGROUPMANAGER_H
