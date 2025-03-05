#ifndef LC_WIDGETFACTORY_H
#define LC_WIDGETFACTORY_H

#include <QMap>
#include <QObject>

#include "lc_penpalettewidget.h"
#include "lc_quickinfowidget.h"
#include "lc_namedviewslistwidget.h"
#include "lc_ucslistwidget.h"

class QMenu;
class QAction;
class QMenuBar;
class QToolBar;
class QActionGroup;

class QG_PenToolBar;
class QG_SnapToolBar;
class QG_LayerWidget;
class LC_LayerTreeWidget;
class LC_QuickInfoWidget;
class QG_BlockWidget;
class QG_ActionHandler;
class QG_LibraryWidget;
class QG_CommandWidget;
class LC_CustomToolbar;
class LC_DockWidget;
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
                     LC_ActionGroupManager* agm);
    void fillActionLists();
    void createStandardToolbars(QG_ActionHandler* action_handler);
    void createCADToolbars();
    void createLeftSidebar(int columns, int icon_size, bool flatButtons);
    void createRightSidebar(QG_ActionHandler* action_handler);
    void initStatusBar();

    QToolBar* createCategoriesToolbar();

    // --- tagged widgets ---

    QG_SnapToolBar* snap_toolbar = nullptr;
    QG_PenToolBar* pen_toolbar = nullptr;
    QToolBar* options_toolbar = nullptr;

    QG_LayerWidget* layer_widget = nullptr;
    LC_QuickInfoWidget* quick_info_widget = nullptr;
    LC_LayerTreeWidget* layer_tree_widget = nullptr;
    LC_PenPaletteWidget* pen_palette = nullptr;
    LC_NamedViewsListWidget* named_views_widget = nullptr;
    LC_UCSListWidget* ucs_widget = nullptr;
    QG_BlockWidget* block_widget = nullptr;
    QG_LibraryWidget* library_widget = nullptr;
    QG_CommandWidget* command_widget = nullptr;

    QList<QAction*> actionsToDisableInPrintPreview;

private:
    QC_ApplicationWindow* main_window = nullptr;
    LC_ActionGroupManager* ag_manager = nullptr;

    LC_DockWidget *leftDocWidget(const QString& title, const char* name, const QList<QAction *> &actions, int columns, int iconSize, bool flatButtons);
    QToolBar *createGenericToolbar(const QString& title, const QString &name, QSizePolicy toolBarPolicy, const std::vector<QString> &actionNames);
    void addToTop(QToolBar *toolbar);
    void addToBottom(QToolBar *toolbar);
    void addToLeft(QToolBar *toolbar);
    QToolButton *toolButton(QToolBar *toolbar, const QString &tooltip, const char *icon, const QList<QAction *>& actions);
    QToolBar *toolbarWithActions(const QString& title, const QString& name, QSizePolicy toolBarPolicy, const QList<QAction *> &actions);

    void addAction(QToolBar* toolbar, const char* actionName);
    void sortToolbarsByByGroupAndTitle(QList<QToolBar *> &list);
    QToolBar *createNamedViewsToolbar(const QString &title, const QString &name, QSizePolicy toolBarPolicy);
    QToolBar* createUCSToolbar(const QString& title, const QString& name, QSizePolicy toolBarPolicy);
    void makeActionsInvisible(const std::vector<QString> &actionNames);
    QToolBar *doCreateToolBar(const QString &title, const QString &name, const QSizePolicy &toolBarPolicy) const;
    void createInfoCursorToolbar(QSizePolicy &tbPolicy);
    void addInfoCursorOptionAction(QMenu *menu, const char *name, int tag);

    QToolBar *createWorkspacesToolbar(const QString &title, const QString &name, QSizePolicy toolBarPolicy);
};

#endif // LC_WIDGETFACTORY_H
