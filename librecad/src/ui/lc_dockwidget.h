#ifndef LC_DOCKWIDGET_H
#define LC_DOCKWIDGET_H

#include <QDockWidget>

class QFrame;
class QGridLayout;

class LC_DockWidget : public QDockWidget
{
    Q_OBJECT

public:
    LC_DockWidget(QWidget* parent);

    QFrame* frame;
    QGridLayout* grid;

    void add_actions(const QList<QAction*>& list, int columns, int icon_size);
};

#endif // LC_DOCKWIDGET_H
