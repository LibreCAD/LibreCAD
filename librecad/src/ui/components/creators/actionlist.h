#ifndef ACTIONLIST_H
#define ACTIONLIST_H

#include <QListWidget>

class ActionList : public QListWidget
{
    Q_OBJECT

public:
    ActionList(QWidget* parent);
    void addActionItem(QAction* action);
    void fromActionList(const QList<QAction *>& a_list);
    void fromActionMap(QMap<QString, QAction*>& a_map);

public slots:
    void activateAction(QListWidgetItem*);

protected:
    QList<QAction*> action_list;
};

#endif // ACTIONLIST_H
