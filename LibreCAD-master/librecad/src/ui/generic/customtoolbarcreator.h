#ifndef CUSTOMTOOLBARCREATOR_H
#define CUSTOMTOOLBARCREATOR_H

#include <QFrame>
#include <QList>

class QListWidgetItem;
class LC_ActionGroupManager;

namespace Ui {
class CustomToolbarCreator;
}

class CustomToolbarCreator : public QFrame
{
    Q_OBJECT

public:
    explicit CustomToolbarCreator(QWidget* parent,
                                  QMap<QString, QAction*>& action_map,
                                  LC_ActionGroupManager* agm);
    ~CustomToolbarCreator();

    QStringList getChosenActions();
    void addCustomWidgets(const QString& group);
    QString getToolbarName();

private:
    Ui::CustomToolbarCreator* ui;
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

    void addWidget();
    void removeWidget();
    void create();

    void setCategory(QString);

signals:
    void widgetToCreate(QString);
    void widgetToDestroy(QString);
};

#endif // CUSTOMTOOLBARCREATOR_H
