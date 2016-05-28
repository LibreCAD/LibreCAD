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

/**
 * This widget allows users to choose actions from a list
 * and save that list.
 *
 * @todo for 2.2.0 make this more generic, drop qt4 support, and then complete dox
 */
class WidgetCreator : public QFrame
{
    Q_OBJECT

public:

    /**
     * @brief WidgetCreator constructor.
     * @param parent - the parent widget (e.g. QDialog)
     * @param actions - a map with objectName keys and QAction* values
     * @param action_groups - a map with objectName keys and QActionGroup* values
     * @param assigner - when true: buttons are added for assignment and updating
     */
    explicit WidgetCreator(QWidget* parent,
                           QMap<QString, QAction*>& actions,
                           QMap<QString, QActionGroup*> action_groups,
                           bool assigner = false);
    ~WidgetCreator();


    QStringList getChosenActions();
    QString getWidgetName();
    /**
     * @brief adds the keys from a group to the combobox
     * @param group - a QSettings group that has keys paired with stringlists
     */
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
