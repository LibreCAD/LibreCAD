#ifndef LC_WIDGETFACTORY_H
#define LC_WIDGETFACTORY_H

#include <QMap>
#include <QObject>
#include <QString>

class QMenu;
class QAction;
class QMenuBar;
class QToolBar;
class QActionGroup;

class QG_PenToolBar;
class QG_SnapToolBar;
class QG_LayerWidget;
class QG_BlockWidget;
class QG_ActionHandler;
class QG_LibraryWidget;
class QG_CommandWidget;
class LC_CustomToolbar;
class QC_ApplicationWindow;
class LC_ActionGroupManager;

/**
 * creates the widgets and adds them to the main window;
 * it also "tags" a few widgets that the main window uses
 */
class LC_WidgetFactory : public QObject
{
    Q_OBJECT

public:
    LC_WidgetFactory(QC_ApplicationWindow* main_win,
                     QMap<QString, QAction*>& action_map,
                     LC_ActionGroupManager* agm);

    void createStandardToolbars(QG_ActionHandler* action_handler);
    void createCADToolbars();
    void createMenus(QMenuBar* menu_bar);
    void createLeftSidebar(int columns, int icon_size);
    void createRightSidebar(QG_ActionHandler* action_handler);

    QToolBar* createCategoriesToolbar();

    // --- tagged widgets ---

    QG_SnapToolBar* snap_toolbar;
    QG_PenToolBar* pen_toolbar;
    QToolBar* options_toolbar;

    QG_LayerWidget* layer_widget;
    QG_BlockWidget* block_widget;
    QG_LibraryWidget* library_widget;
    QG_CommandWidget* command_widget;

    QMenu* file_menu;
    QMenu* windows_menu;


private:
    QC_ApplicationWindow* main_window;
    QMap<QString, QAction*>& a_map;
    LC_ActionGroupManager* ag_manager;

    QList<QAction*> file_actions;
    QList<QAction*> line_actions;
    QList<QAction*> circle_actions;
    QList<QAction*> curve_actions;
    QList<QAction*> ellipse_actions;
    QList<QAction*> polyline_actions;
    QList<QAction*> select_actions;
    QList<QAction*> dimension_actions;
    QList<QAction*> modify_actions;
    QList<QAction*> info_actions;
    QList<QAction*> layer_actions;
    QList<QAction*> block_actions;
};

#endif // LC_WIDGETFACTORY_H
