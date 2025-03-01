#ifndef LC_DOCKWIDGET_H
#define LC_DOCKWIDGET_H

#include <QDockWidget>

class QFrame;
class QGridLayout;

class LC_DockWidget : public QDockWidget{
    Q_OBJECT
public slots:
    void updateWidgetSettings();
public:
    LC_DockWidget(QWidget* parent);

    QFrame* frame = nullptr;
    QGridLayout* grid = nullptr;

    void add_actions(const QList<QAction*>& list, int columns, int icon_size, bool flatButton);
};

#endif // LC_DOCKWIDGET_H
