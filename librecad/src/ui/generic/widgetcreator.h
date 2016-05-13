#ifndef WIDGETCREATOR_H
#define WIDGETCREATOR_H

#include <QFrame>
#include <QList>
#include <QMap>

class QListWidgetItem;
class QActionGroup;
class QPushButton;

namespace Ui {
class WidgetCreator;
}

class WidgetCreator : public QFrame
{
    Q_OBJECT

public:
    explicit WidgetCreator(QWidget* parent,
                           QMap<QString, QAction*>& actions,
                           QMap<QString, QActionGroup*> action_groups,
                           bool assigner = false);
    ~WidgetCreator();

    QStringList getChosenActions();
    QString getWidgetName();
    void addCustomWidgets(const QString& group);
    void addButton(QPushButton* button);
    bool hasBeenCreated(const QString& widget_name);

private:
    Ui::WidgetCreator* ui;
    QMap<QString, QAction*>& a_map;
    QMap<QString, QActionGroup*> ag_map;
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
    void requestAssignment();
    void requestUpdate();

    void setCategory(QString);

signals:
    void widgetToCreate(QString);
    void widgetToDestroy(QString);
    void widgetToAssign(QString);
    void widgetToUpdate(QString);
};

#endif // WIDGETCREATOR_H
