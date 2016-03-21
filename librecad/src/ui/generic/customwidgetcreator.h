#ifndef CUSTOMWIDGETCREATOR_H
#define CUSTOMWIDGETCREATOR_H

#include <QFrame>
#include <QList>

class QListWidgetItem;

namespace Ui {
class CustomWidgetCreator;
}

class CustomWidgetCreator : public QFrame
{
    Q_OBJECT

public:
    explicit CustomWidgetCreator(QWidget* parent, QMap<QString, QAction*>& a_map);
    ~CustomWidgetCreator();

    QStringList getChosenActions();


protected slots:
    void addChosenAction();
    void addChosenAction(QListWidgetItem* item);

    void removeChosenAction();
    void removeChosenAction(QListWidgetItem* item);

private:
    Ui::CustomWidgetCreator* ui;
};

#endif // CUSTOMWIDGETCREATOR_H
