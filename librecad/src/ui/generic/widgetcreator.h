#ifndef WIDGETCREATOR_H
#define WIDGETCREATOR_H

#include <QFrame>
#include <QList>

class QListWidgetItem;
class LC_ActionGroupManager;

namespace Ui {
class WidgetCreator;
}

class WidgetCreator : public QFrame
{
    Q_OBJECT

public:
    explicit WidgetCreator(QWidget* parent,
                           QMap<QString, QAction*>& action_map,
                           LC_ActionGroupManager* agm);
    ~WidgetCreator();

    QStringList getChosenActions();
    void addCustomWidgets(const QString& group);

private:
    Ui::WidgetCreator* ui;
    QMap<QString, QAction*>& a_map;
    LC_ActionGroupManager* ag_manager;
    QString w_group;
    QString w_key;

private slots:
    void addChosenAction();
    void addChosenAction(QListWidgetItem* item);

    void removeChosenAction();
    void removeChosenAction(QListWidgetItem* item);

    void setLists(QString);

    void destroyWidget();
    void createWidget();

    void setCategory(QString);

signals:
    void widgetToCreate(QString);
    void widgetToDestroy(QString);
};

#endif // WIDGETCREATOR_H
