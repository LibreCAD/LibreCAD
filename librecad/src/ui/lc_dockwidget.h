#ifndef LC_DOCKWIDGET_H
#define LC_DOCKWIDGET_H

#include <QDockWidget>
#include <QGroupBox>
#include <QGridLayout>

class LC_DockWidget : public QDockWidget
{
    Q_OBJECT

public:
    LC_DockWidget(QWidget* parent = 0);
    ~LC_DockWidget()=default;

    QFrame* frame;
    QGridLayout* grid;
    void add_actions(QList<QAction*> list, int columns, int icon_size);

};

#endif // LC_DOCKWIDGET_H
