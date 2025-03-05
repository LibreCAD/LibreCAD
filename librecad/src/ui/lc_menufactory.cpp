/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_menufactory.h"

#include <QDesktopServices>
#include <QMdiArea>
#include <QMenuBar>
#include <QToolBar>

#include "lc_actiongroupmanager.h"
#include "rs_settings.h"
class QToolBar;

LC_MenuFactory::LC_MenuFactory(QC_ApplicationWindow *main_win, LC_ActionGroupManager *agm)
    : QObject(nullptr)
    , main_window(main_win)
    , ag_manager(agm){
     allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
}


void LC_MenuFactory::recreateMenuIfNeeded(QMenuBar* menuBar){
    MenuOptions options;
    LC_GROUP("Startup"); {
        options.expandToolsMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        options.expandToolsTillEntity = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
    }
    LC_GROUP_END();
    if (m_menuOptions.isDifferent(options)) {
        m_menuOptions.apply(options);
        menuBar->clear();
        doCreateMenus(menuBar);
    }
}

void LC_MenuFactory::createMenus(QMenuBar* menuBar){
    LC_GROUP("Startup"); {
        m_menuOptions.expandToolsMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        m_menuOptions.expandToolsTillEntity = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
    }
    doCreateMenus(menuBar);
}

void LC_MenuFactory::doCreateMenus(QMenuBar* menu_bar){
    QList<QMenu *> topMenuMenus;
    createFileMenu(menu_bar, topMenuMenus);
    createSettingsMenu(menu_bar, topMenuMenus);
    createEditMenu(menu_bar, topMenuMenus);
    createViewMenu(menu_bar, topMenuMenus);
    createPluginsMenu(menu_bar, topMenuMenus);
    createToolsMenu(menu_bar, topMenuMenus);
    createWorkspaceMenu(menu_bar, topMenuMenus);
    prepareWorkspaceMenuComponents();
    createHelpMenu(menu_bar, topMenuMenus);

    for (auto m : topMenuMenus) {
        menu_bar->addMenu(m);
    }
}

void LC_MenuFactory::createHelpMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    auto help = menu(tr("&Help"), "help", menu_bar);

    subMenuWithActions(help, tr("On&line Docs"),"OnlineInfo", nullptr, {
                           urlActionTR(tr("&Wiki"), "https://dokuwiki.librecad.org/"),
                           urlActionTR(tr("User's &Manual"), "https://librecad.readthedocs.io/"),
                           urlActionTR(tr("&Commands"), "https://librecad.readthedocs.io/en/latest/ref/tools.html"),
                           urlActionTR(tr("&Style Sheets"), "https://librecad.readthedocs.io/en/latest/ref/customize.html#style-sheets"),
                           urlActionTR(tr("Wid&gets"), "https://librecad.readthedocs.io/en/latest/ref/menu.html#widgets")
                       });


    auto help_about = new QAction(QIcon(":/images/librecad.png"), tr("About"), main_window);
    connect(help_about, SIGNAL(triggered()), main_window, SLOT(showAboutWindow()));

    auto license = new QAction(QObject::tr("License"), main_window);
    connect(license, SIGNAL(triggered()), main_window, SLOT(invokeLicenseWindow()));

    help->addSeparator();
    help->QWidget::addAction(urlActionTR(tr("&Forum"), "https://forum.librecad.org/"));
    help->QWidget::addAction(urlActionTR(tr("Zulip &Chat"), "https://librecad.zulipchat.com/"));
    help->addSeparator();
    help->QWidget::addAction(urlActionTR(tr("&Submit Error"), "https://github.com/LibreCAD/LibreCAD/issues/new"));
    help->QWidget::addAction(urlActionTR(tr("&Request Feature"), "https://github.com/LibreCAD/LibreCAD/releases"));
    help->QWidget::addAction(urlActionTR(tr("&Releases Page"), "https://github.com/LibreCAD/LibreCAD/releases"));
    help->addSeparator();
    help->QWidget::addAction(help_about);
    help->QWidget::addAction(license);
    help->QWidget::addAction(urlActionTR(tr("&Donate"), "https://librecad.org/donate.html"));

    topMenuMenus << help;
}

void LC_MenuFactory::createToolsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    if (m_menuOptions.expandToolsMenu) {
        if (m_menuOptions.expandToolsTillEntity) {
            auto line = menu(tr("&Line"), "line", menu_bar);
            line->addActions(ag_manager->line_actions);
            topMenuMenus << line;

            auto point = menu(tr("Poin&t"), "point", menu_bar);
            point->addActions(ag_manager->point_actions);
            topMenuMenus << point;

            auto circle = menu(tr("&Circle"), "circle", menu_bar);
            circle->addActions(ag_manager->circle_actions);
            topMenuMenus << circle;

            auto arc = menu(tr("&Arc"), "arc", menu_bar);
            arc->addActions(ag_manager->curve_actions);
            topMenuMenus << arc;

            auto shape = menu(tr("Poly&gon"), "shape", menu_bar);
            shape->addActions(ag_manager->shape_actions);
            topMenuMenus << shape;

            auto spline = menu(tr("Splin&e"), "spline", menu_bar);
            spline->addActions(ag_manager->spline_actions);
            topMenuMenus << spline;

            auto ellipse = menu(tr("Ellipse&e"), "ellipse", menu_bar);
            ellipse->addActions(ag_manager->ellipse_actions);
            topMenuMenus << ellipse;

            auto polyline = menu(tr("&Polyline"), "polyline", menu_bar);
            polyline->addActions(ag_manager->polyline_actions);
            topMenuMenus << polyline;

            auto other = menu(tr("&Other"), "other", menu_bar);
            other->addActions(ag_manager->other_drawing_actions);
            topMenuMenus << other;
        }
        else {
            auto draw = menu(tr("&Draw"), "draw", menu_bar);
            subMenuWithActions(draw, tr("&Line"), "line", ":/icons/line.lci", ag_manager->line_actions);
            subMenuWithActions(draw, tr("Poin&t"), "point", ":/icons/points.lci", ag_manager->point_actions);
            subMenuWithActions(draw, tr("&Circle"), "circle", ":/icons/circle.lci", ag_manager->circle_actions);
            subMenuWithActions(draw, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci", ag_manager->curve_actions);
            subMenuWithActions(draw, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci", ag_manager->shape_actions);
            subMenuWithActions(draw, tr("Splin&e"), "spline", ":/icons/spline_points.lci", ag_manager->spline_actions);
            subMenuWithActions(draw, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci", ag_manager->ellipse_actions);
            subMenuWithActions(draw, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci", ag_manager->polyline_actions);
            subMenuWithActions(draw, tr("Ot&her"), "other", ":/icons/text.lci", ag_manager->other_drawing_actions);

            topMenuMenus << draw;
        }

        auto modify = menu(tr("&Modify"), "info", menu_bar);
        modify->addActions(ag_manager->modify_actions);
        subMenuWithActions(modify, tr("&Order"), "order", ":/icons/order.lci", ag_manager->order_actions);

        topMenuMenus << modify;

        auto dims = menu(tr("&Dimensions"), "dims", menu_bar);
        dims->addActions(ag_manager->dimension_actions);
        topMenuMenus << dims;

        auto info = menu(tr("&Info"), "info", menu_bar);
        info->addActions(ag_manager->info_actions);
        topMenuMenus << info;
    }
    else {
        auto tools = menu(tr("&Tools"), "tools", menu_bar);
        subMenuWithActions(tools, tr("&Line"), "line", ":/icons/line.lci", ag_manager->line_actions);
        subMenuWithActions(tools, tr("Poin&t"), "line", ":/icons/points.lci", ag_manager->point_actions);
        subMenuWithActions(tools, tr("&Circle"), "circle", ":/icons/circle.lci", ag_manager->circle_actions);
        subMenuWithActions(tools, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci", ag_manager->curve_actions);
        subMenuWithActions(tools, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci", ag_manager->shape_actions);
        subMenuWithActions(tools, tr("Splin&e"), "spline", ":/icons/spline_points.lci", ag_manager->spline_actions);
        subMenuWithActions(tools, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci", ag_manager->ellipse_actions);
        subMenuWithActions(tools, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci", ag_manager->polyline_actions);
        subMenuWithActions(tools, tr("&Select"), "select", ":/icons/select.lci", ag_manager->select_actions);
        subMenuWithActions(tools, tr("Dime&nsion"), "dimension", ":/icons/dim_horizontal.lci", ag_manager->dimension_actions);
        subMenuWithActions(tools, tr("Ot&her"), "other", ":/icons/text.lci", ag_manager->other_drawing_actions);
        subMenuWithActions(tools, tr("&Modify"), "modify", ":/icons/move_rotate.lci", ag_manager->modify_actions);
        subMenuWithActions(tools, tr("&Info"), "info", ":/icons/measure.lci", ag_manager->info_actions);
        subMenuWithActions(tools, tr("&Order"), "order", ":/icons/order.lci", ag_manager->order_actions);

        topMenuMenus << tools;
    }
}

void LC_MenuFactory::createFileMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    file_menu = menu(tr("&File"),"file", menu_bar, {
                         "FileNew",
                         "FileNewTemplate",
                         "FileOpen",
                         "",
                         "FileSave",
                         "FileSaveAs",
                         "FileSaveAll",
                         ""
                     });

    recentFilesMenu = new QMenu(tr("Recent Files"), file_menu);
    file_menu->addMenu(recentFilesMenu);

    subMenu(file_menu, tr("Import"),"import", ":/icons/import.lci", {
             "DrawImage",
             "BlocksImport"
         });

    subMenu(file_menu, tr("Export"),"export", ":/icons/export.lci", {
        "FileExportMakerCam",
        "FilePrintPDF",
        "FileExport"
    });

    addActions(file_menu, {
        "",
        "FilePrint",
        "FilePrintPreview",
        "",
        "FileClose",
        "FileCloseAll",
        "FileQuit",
        ""
    });


    topMenuMenus << file_menu;
}

void LC_MenuFactory::createSettingsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    auto settings = menu(tr("&Options"),"options", menu_bar, {
                             "OptionsGeneral",
                             "ShortcutsOptions",
                             "WidgetOptions",
                             "DeviceOptions",
                             "ReloadStyleSheet",
                             "",
                             "OptionsDrawing",
                         });

    topMenuMenus << settings;
}

void LC_MenuFactory::createEditMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    auto edit = menu(tr("&Edit"),"edit", menu_bar, {
                         "EditKillAllActions",
                         "",
                         "EditUndo",
                         "EditRedo",
                         "",
                         "EditCut",
                         "EditCopy",
                         "EditPaste",
                         "EditPasteTransform",
                         "PasteToPoints",
                         "",
                         "EditCutQuick",
                         "EditCopyQuick",
                         "ModifyDeleteQuick"
                     });

    topMenuMenus << edit;
}

void LC_MenuFactory::createViewMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    view_menu = menu(tr("&View"), "view", menu_bar, {
                         /*     "Fullscreen",
        "ViewStatusBar",*/
                         "ViewGrid",
                         "ViewDraft",
                         "ViewLinesDraft",
                         "ViewAntialiasing",
                         "",
                         "ViewGridOrtho",
                         "ViewGridIsoLeft",
                         "ViewGridIsoTop",
                         "ViewGridIsoRight",
                         "",
                         "ViewGridOrtho",
                         "ViewGridIsoLeft",
                         "ViewGridIsoTop",
                         "ViewGridIsoRight",
                         "",
                         "ZoomRedraw",
                         "ZoomIn",
                         "ZoomOut",
                         "ZoomAuto",
                         "ZoomPrevious",
                         "ZoomWindow",
                         "ZoomPan",
                         "",
                         "ZoomViewSave",
                     });

    subMenu(view_menu, tr("&Views Restore"), "view_restore", ":/icons/nview_visible.lci",
                                     {"ZoomViewRestore1",
                                      "ZoomViewRestore2",
                                      "ZoomViewRestore3",
                                      "ZoomViewRestore4",
                                      "ZoomViewRestore5"});

    topMenuMenus << view_menu;
}

void LC_MenuFactory::createPluginsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    auto plugins = menu(tr("Pl&ugins"),"plugins", menu_bar);
    topMenuMenus << plugins;
}

void LC_MenuFactory::createWorkspaceMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    workspaceMenu = menu(tr("&Workspace"),"workspaces", menu_bar, {
                             "Fullscreen" // temp way to show this menu on OS X
                         });

    connect(workspaceMenu, SIGNAL(aboutToShow()),
            main_window, SLOT(slotWindowsMenuAboutToShow()));

    topMenuMenus << workspaceMenu;
}


void LC_MenuFactory::prepareWorkspaceMenuComponents(){

    dockareas = subMenu(workspaceMenu, tr("Dock Areas"),"dockareas", nullptr, {
                            "LeftDockAreaToggle",
                            "RightDockAreaToggle",
                            "TopDockAreaToggle",
                            "BottomDockAreaToggle",
                            "FloatingDockwidgetsToggle"
                        });

    dockwidgets_menu = doCreateSubMenu(workspaceMenu, tr("Dock Wid&gets"), "dockwidgets", nullptr);

    dockwidgets_menu->addSeparator();

    QList<QDockWidget*> dockwidgetsList = main_window->findChildren<QDockWidget*>();
    main_window->sortWidgetsByTitle(dockwidgetsList);

    QAction* namedViewsToggleViewAction = nullptr;
    QAction* ucsToggleViewAction = nullptr;

    for (QDockWidget* dw: dockwidgetsList){
        if (main_window->dockWidgetArea(dw) == Qt::RightDockWidgetArea) {
            QAction *action = dw->toggleViewAction();
            dockwidgets_menu->QWidget::addAction(action);
            QString objectName = dw->objectName();
            if (objectName == "view_dockwidget"){
                namedViewsToggleViewAction = action;
            }
            else if (objectName == "ucs_dockwidget"){
                ucsToggleViewAction = action;
            }
        }
    }

    view_menu->QWidget::addAction(namedViewsToggleViewAction);
    view_menu->addSeparator();
    view_menu->QWidget::addAction(ag_manager->getActionByName("UCSCreate"));
    view_menu->QWidget::addAction(ag_manager->getActionByName("UCSSetWCS"));
    view_menu->QWidget::addAction(ucsToggleViewAction);

    dockwidgets_menu->addSeparator();

    for (QDockWidget* dw : dockwidgetsList){
        if (main_window->dockWidgetArea(dw) == Qt::LeftDockWidgetArea) {
            dockwidgets_menu->QWidget::addAction(dw->toggleViewAction());
        }
    }

    toolbars = doCreateSubMenu(workspaceMenu, tr("&Toolbars"),"toolbars", nullptr);

    QList<QToolBar*> toolbarsList = main_window->findChildren<QToolBar*>();

    main_window->sortWidgetsByGroupAndTitle(toolbarsList);

    int previousGroup = -100;

    for (QToolBar* tb: toolbarsList){
        const QVariant &variant = tb->property("_group");
        int group = variant.toInt();
        if (group != previousGroup ){
            if (previousGroup != -100) {
                toolbars->addSeparator();
            }
            previousGroup = group;
        }
        toolbars->QWidget::addAction(tb->toggleViewAction());
    }
}

void LC_MenuFactory::slotWindowsMenuAboutToShow(const QList<QC_MDIWindow*> &window_list){
    LC_GROUP_GUARD("WindowOptions");
    {
         QIcon wsIcon = QIcon(":/icons/workspace.lci");
         workspaceMenu->clear(); // this is a temporary menu; constructed on-demand
         allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
         QMenu *menu;

         workspaceMenu->addAction(ag_manager->getActionByName("Fullscreen"));
         workspaceMenu->addAction(ag_manager->getActionByName("ViewStatusBar"));

         workspaceMenu->addMenu(dockareas);
         workspaceMenu->addMenu(dockwidgets_menu);
         workspaceMenu->addMenu(toolbars);
         addAction(workspaceMenu, "RedockWidgets");
         workspaceMenu->addSeparator();

         workspaceMenu->addAction(ag_manager->getActionByName("WorkspaceCreate"));

         QList<QPair<int, QString>> workspacesList;
         main_window->fillWorkspacesList(workspacesList);
         if (!workspacesList.isEmpty()){
             auto workspaces = new QMenu(tr("&Workspaces"), workspaceMenu);
             workspaces->setTearOffEnabled(allowTearOffMenus);
             workspaces->setIcon(wsIcon);
             int workspacesCount = workspacesList.size();
             for (int i = 0; i < workspacesCount; i++){
                 auto w = workspacesList.at(i);
                 auto name = w.second;
                 auto* a = workspaces->addAction(wsIcon, name);
                 connect(a, &QAction::triggered, main_window, &QC_ApplicationWindow::restoreWorkspace);
                 a->setEnabled(true);
                 a->setCheckable(false);
                 a->setVisible(true);
                 a->setProperty("_WSPS_IDX", QVariant(w.first));
             }
             workspaceMenu->addMenu(workspaces);
             workspaceMenu->addAction(ag_manager->getActionByName("WorkspaceRemove"));
         }
         workspaceMenu->addSeparator();

         addAction(workspaceMenu, "InvokeMenuCreator");
         addAction(workspaceMenu, "InvokeToolbarCreator");
         workspaceMenu->addSeparator();

         auto drawings = new QMenu(tr("&Drawinsgs"), workspaceMenu);
         drawings->setTearOffEnabled(allowTearOffMenus);
         workspaceMenu->addMenu(drawings);
         QAction *menuItem;

         auto mdi_area = main_window->getMdiArea();
         auto mdiViewMode = mdi_area->viewMode();
         bool tabbed = mdiViewMode == QMdiArea::TabbedView;

         menuItem = drawings->addAction(tr("Ta&b mode"), main_window, &LC_MDIApplicationWindow::slotToggleTab);
         menuItem->setCheckable(true);
         menuItem->setChecked(tabbed);

         menuItem = drawings->addAction(tr("&Window mode"), main_window, &LC_MDIApplicationWindow::slotToggleTab);
         menuItem->setCheckable(true);
         menuItem->setChecked(!tabbed);


         if (tabbed) {
             menu = new QMenu(tr("&Layout"), workspaceMenu);
             menu->setTearOffEnabled(allowTearOffMenus);
             drawings->addMenu(menu);

             menuItem = menu->addAction(tr("Rounded"), main_window, &LC_MDIApplicationWindow::slotTabShapeRounded);
             menuItem->setCheckable(true);

             int tabShape = LC_GET_INT("TabShape");
             menuItem->setChecked(tabShape == RS2::Rounded);

             menuItem = menu->addAction(tr("Triangular"), main_window, &LC_MDIApplicationWindow::slotTabShapeTriangular);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabShape == RS2::Triangular);

             menu->addSeparator();
             int tabPosition = LC_GET_INT("TabPosition");

             menuItem = menu->addAction(tr("North"), main_window, &LC_MDIApplicationWindow::slotTabPositionNorth);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::North);

             menuItem = menu->addAction(tr("South"), main_window, &LC_MDIApplicationWindow::slotTabPositionSouth);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::South);

             menuItem = menu->addAction(tr("East"), main_window, &LC_MDIApplicationWindow::slotTabPositionEast);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::East);

             menuItem = menu->addAction(tr("West"), main_window, &LC_MDIApplicationWindow::slotTabPositionWest);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::West);

         } else {

             menu = new QMenu(tr("&Arrange"), workspaceMenu);
             menu->setTearOffEnabled(allowTearOffMenus);
             workspaceMenu->addMenu(menu);

             menuItem = menu->addAction(tr("&Maximized"), main_window, &LC_MDIApplicationWindow::slotSetMaximized);
             menuItem->setCheckable(true);
             menuItem->setChecked(LC_GET_INT("SubWindowMode") == RS2::Maximized);

             menu->addAction(tr("&Cascade"), main_window, &LC_MDIApplicationWindow::slotCascade);
             menu->addAction(tr("&Tile"), main_window, &LC_MDIApplicationWindow::slotTile);
             menu->addAction(tr("Tile &Vertically"), main_window, &LC_MDIApplicationWindow::slotTileVertical);
             menu->addAction(tr("Tile &Horizontally"), main_window, &LC_MDIApplicationWindow::slotTileHorizontal);
         }

         workspaceMenu->addSeparator();
         QMdiSubWindow *active = mdi_area->activeSubWindow();
         for (int i = 0; i < window_list.size(); ++i) {
             QString title = window_list.at(i)->windowTitle();
             if (title.contains("[*]")) { // modification mark placeholder
                 int idx = title.lastIndexOf("[*]");
                 if (window_list.at(i)->isWindowModified()) {
                     title.replace(idx, 3, "*");
                 } else {
                     title.remove(idx, 3);
                 }
             }
             QAction *id = workspaceMenu->addAction(title,main_window, SLOT(slotWindowsMenuActivated(bool)));
             id->setCheckable(true);
             id->setData(i);
             id->setChecked(window_list.at(i) == active);
         }
    }
}


QAction* LC_MenuFactory::urlActionTR(const QString& title, const char* url ){
    auto* result    = new QAction(  title, main_window);
    connect(result, &QAction::triggered, main_window, [=](){
        QDesktopServices::openUrl( QUrl( url));
    });
    return result;
}

QMenu*  LC_MenuFactory::subMenu(QMenu* parent, const QString& title, const QString& name, const char* icon, const std::vector<QString> &actionNames){
    QMenu *result = doCreateSubMenu(parent, title, name, icon);
    addActions(result, actionNames);
    return result;
}

QMenu*  LC_MenuFactory::subMenuWithActions(QMenu* parent, const QString& title, const QString& name, const char* icon, const QList<QAction*> &actions){
    QMenu *sub_menu = doCreateSubMenu(parent, title, name, icon);
    sub_menu->addActions(actions);
    return sub_menu;
}

QMenu *LC_MenuFactory::doCreateSubMenu(QMenu *parent, const QString& title, const QString& name, const char *icon) const {
    auto sub_menu = parent->addMenu(title);
    if (icon != nullptr) {
        sub_menu->setIcon(QIcon(icon));
    }
    sub_menu->setTearOffEnabled(allowTearOffMenus);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    const QString &objectName = nameCleared.toLower() + "_menu";
    sub_menu->setObjectName(objectName);
    return sub_menu;
}

QMenu*  LC_MenuFactory::menu(const QString& title, const QString& name, QMenuBar* parent){
    auto result =  new QMenu(title, parent);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    result->setObjectName(nameCleared.toLower() + "_menu");
    result->setTearOffEnabled(allowTearOffMenus);
    return result;
}

QMenu*  LC_MenuFactory::menu(const QString& title, const QString& name,  QMenuBar* parent, const std::vector<QString> &actionNames){
    QMenu* result = menu(title, name, parent);
    addActions(result, actionNames);
    return result;
}

void LC_MenuFactory::addAction(QMenu* menu, const char* actionName){
    QAction *action = ag_manager->getActionByName(actionName);
    if (action != nullptr) {
        menu->addAction(action);
    }
}

void LC_MenuFactory::addActions(QMenu *result, const std::vector<QString> &actionNames) {
    for (const QString& actionName : actionNames){
        if (actionName.isEmpty()){
            result->addSeparator();
        }
        else{
            QAction* action = ag_manager->getActionByName(actionName);
            if (action != nullptr){
                result->addAction(action);
            }
        }
    }
}
