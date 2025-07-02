/* ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_menufactory.h"

#include <QDesktopServices>
#include <QDockWidget>
#include <QMdiArea>
#include <QMenuBar>
#include <QToolBar>
#include <QUrl>

#include "lc_actiongroupmanager.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "rs_settings.h"

class QToolBar;

LC_MenuFactory::LC_MenuFactory(QC_ApplicationWindow *main_win)
    : QObject(nullptr)
    , LC_AppWindowAware(main_win)
    , m_actionGroupManager{main_win->m_actionGroupManager.get()}
    , m_actionFactory{main_win->m_actionFactory.get()}{
     m_allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
}

void LC_MenuFactory::recreateMainMenuIfNeeded(QMenuBar* menuBar){
    MenuOptions options;
    LC_GROUP("Startup"); {
        options.expandToolsMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        options.expandToolsTillEntity = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
    }
    LC_GROUP_END();
    if (m_menuOptions.isDifferent(options)) {
        m_menuOptions.apply(options);
        menuBar->clear();
        doCreateMenus(menuBar, false);
    }
}

void LC_MenuFactory::createMainMenu(QMenuBar* menuBar){
    LC_GROUP("Startup"); {
        m_menuOptions.expandToolsMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        m_menuOptions.expandToolsTillEntity = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
    }
    doCreateMenus(menuBar, true);
}

void LC_MenuFactory::doCreateMenus(QMenuBar *menu_bar, bool firstCreation) {
    QList<QMenu *> topMenuMenus;
    if (firstCreation) {
        createFileMenu(menu_bar, topMenuMenus);
        createSettingsMenu(menu_bar, topMenuMenus);
        createEditMenu(menu_bar, topMenuMenus);
        createViewMenu(menu_bar, topMenuMenus);
        createPluginsMenu(menu_bar, topMenuMenus);
        createToolsMenu(menu_bar, topMenuMenus);
        createWorkspaceMenu(menu_bar, topMenuMenus);
        prepareWorkspaceMenuComponents();
        createHelpMenu(menu_bar, topMenuMenus);
    }
    else{
        // we re-add previously created menus into the menu bar.
        // we can't recreate them fully, as recent files menu should survive the menu update, as it referenced by RS_RecentFiles instance.
        topMenuMenus << m_menuFile;
        topMenuMenus << m_menuSettings;
        topMenuMenus << m_menuEdit;
        topMenuMenus << m_menuView;
        topMenuMenus << m_menuPlugins;
        if (m_menuOptions.expandToolsMenu) {
            // fixme - sand - icons - potentially this may lead to the waste if menus are switched often (which is hardly the case but still).
            // this is due to the fact that menubar.clear() does not delete original actions/sub menus, but just removes them from the list.
            // From the other side, as menubar is the owner for them, they will be deleted as menu bar will be deleted.
            // potentially, we should create all menus just once.
            createToolsMenuExpanded(menu_bar, topMenuMenus);
        }
        else{
            if (m_menuToolsCombined == nullptr){
                createToolsMenuCombined(menu_bar, topMenuMenus);
            }
            else{
                topMenuMenus << topMenuMenus;
            }
        }
        topMenuMenus << m_menuWorkspace;
        topMenuMenus << m_menuHelp;
    }

    for (auto m : topMenuMenus) {
        menu_bar->addMenu(m);
    }
}

void LC_MenuFactory::createHelpMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    m_menuHelp = menu(tr("&Help"), "help", menu_bar);

    subMenuWithActions(m_menuHelp, tr("On&line Docs"),"OnlineInfo", nullptr, {
                           urlActionTR(tr("&Wiki"), "https://dokuwiki.librecad.org/"),
                           urlActionTR(tr("User's &Manual"), "https://librecad.readthedocs.io/"),
                           urlActionTR(tr("&Commands"), "https://librecad.readthedocs.io/en/latest/ref/tools.html"),
                           urlActionTR(tr("&Style Sheets"), "https://librecad.readthedocs.io/en/latest/ref/customize.html#style-sheets"),
                           urlActionTR(tr("Wid&gets"), "https://librecad.readthedocs.io/en/latest/ref/menu.html#widgets")
                       });


    auto help_about = new QAction(QIcon(":/images/librecad.png"), tr("About"), m_appWin);
    connect(help_about, &QAction::triggered, m_appWin, &QC_ApplicationWindow::showAboutWindow);

    auto license = new QAction(QObject::tr("License"), m_appWin);
    connect(license,  &QAction::triggered, m_appWin, &QC_ApplicationWindow::invokeLicenseWindow);

    m_menuHelp->addSeparator();
    m_menuHelp->QWidget::addAction(urlActionTR(tr("&Forum"), "https://forum.librecad.org/"));
    m_menuHelp->QWidget::addAction(urlActionTR(tr("Zulip &Chat"), "https://librecad.zulipchat.com/"));
    m_menuHelp->addSeparator();
    m_menuHelp->QWidget::addAction(urlActionTR(tr("&Submit Error"), "https://github.com/LibreCAD/LibreCAD/issues/new"));
    m_menuHelp->QWidget::addAction(urlActionTR(tr("&Request Feature"), "https://github.com/LibreCAD/LibreCAD/issues"));
    m_menuHelp->QWidget::addAction(urlActionTR(tr("&Releases Page"), "https://github.com/LibreCAD/LibreCAD/releases"));
    m_menuHelp->addSeparator();
    m_menuHelp->QWidget::addAction(help_about);
    m_menuHelp->QWidget::addAction(license);
    m_menuHelp->QWidget::addAction(urlActionTR(tr("&Donate"), "https://librecad.org/donate.html"));

    topMenuMenus << m_menuHelp;
}

void LC_MenuFactory::createToolsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    if (m_menuOptions.expandToolsMenu) {
        createToolsMenuExpanded(menu_bar, topMenuMenus);
    }
    else {
        createToolsMenuCombined(menu_bar, topMenuMenus);
    }
}

void LC_MenuFactory::createToolsMenuExpanded(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus) const {
    if (m_menuOptions.expandToolsTillEntity) {
        auto line = menu(tr("&Line"), "line", menu_bar);
        line->addActions(m_actionFactory->line_actions);
        topMenuMenus << line;

        auto point = menu(tr("Poin&t"), "point", menu_bar);
        point->addActions(m_actionFactory->point_actions);
        topMenuMenus << point;

        auto circle = menu(tr("&Circle"), "circle", menu_bar);
        circle->addActions(m_actionFactory->circle_actions);
        topMenuMenus << circle;

        auto arc = menu(tr("&Arc"), "arc", menu_bar);
        arc->addActions(m_actionFactory->curve_actions);
        topMenuMenus << arc;

        auto shape = menu(tr("Poly&gon"), "shape", menu_bar);
        shape->addActions(m_actionFactory->shape_actions);
        topMenuMenus << shape;

        auto spline = menu(tr("Splin&e"), "spline", menu_bar);
        spline->addActions(m_actionFactory->spline_actions);
        topMenuMenus << spline;

        auto ellipse = menu(tr("Ellip&se"), "ellipse", menu_bar);
        ellipse->addActions(m_actionFactory->ellipse_actions);
        topMenuMenus << ellipse;

        auto polyline = menu(tr("&Polyline"), "polyline", menu_bar);
        polyline->addActions(m_actionFactory->polyline_actions);
        topMenuMenus << polyline;

        auto other = menu(tr("&Other"), "other", menu_bar);
        other->addActions(m_actionFactory->other_drawing_actions);
        topMenuMenus << other;
    }
    else {
        auto draw = menu(tr("&Draw"), "draw", menu_bar);
        subMenuWithActions(draw, tr("&Line"), "line", ":/icons/line.lci", m_actionFactory->line_actions);
        subMenuWithActions(draw, tr("Poin&t"), "point", ":/icons/points.lci", m_actionFactory->point_actions);
        subMenuWithActions(draw, tr("&Circle"), "circle", ":/icons/circle.lci", m_actionFactory->circle_actions);
        subMenuWithActions(draw, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci", m_actionFactory->curve_actions);
        subMenuWithActions(draw, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci", m_actionFactory->shape_actions);
        subMenuWithActions(draw, tr("Splin&e"), "spline", ":/icons/spline_points.lci", m_actionFactory->spline_actions);
        subMenuWithActions(draw, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci", m_actionFactory->ellipse_actions);
        subMenuWithActions(draw, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci", m_actionFactory->polyline_actions);
        subMenuWithActions(draw, tr("Ot&her"), "other", ":/icons/text.lci", m_actionFactory->other_drawing_actions);

        topMenuMenus << draw;
    }

    auto modify = this->menu(tr("&Modify"), "info", menu_bar);
    modify->addActions(this->m_actionFactory->modify_actions);
    this->subMenuWithActions(modify, tr("&Order"), "order", ":/icons/order.lci", this->m_actionFactory->order_actions);

    topMenuMenus << modify;

    auto dims = this->menu(tr("&Dimensions"), "dims", menu_bar);
    dims->addActions(this->m_actionFactory->dimension_actions);
    topMenuMenus << dims;

    auto info = this->menu(tr("&Info"), "info", menu_bar);
    info->addActions(this->m_actionFactory->info_actions);
    topMenuMenus << info;
}

void LC_MenuFactory::createToolsMenuCombined(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus) {
    m_menuToolsCombined = menu(tr("&Tools"), "tools", menu_bar);
    subMenuWithActions(m_menuToolsCombined, tr("&Line"), "line", ":/icons/line.lci", m_actionFactory->line_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Poin&t"), "line", ":/icons/points.lci", m_actionFactory->point_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Circle"), "circle", ":/icons/circle.lci", m_actionFactory->circle_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci", m_actionFactory->curve_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci", m_actionFactory->shape_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Splin&e"), "spline", ":/icons/spline_points.lci", m_actionFactory->spline_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci", m_actionFactory->ellipse_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci", m_actionFactory->polyline_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Select"), "select", ":/icons/select.lci", m_actionFactory->select_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Dime&nsion"), "dimension", ":/icons/dim_horizontal.lci", m_actionFactory->dimension_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Ot&her"), "other", ":/icons/text.lci", m_actionFactory->other_drawing_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Modify"), "modify", ":/icons/move_rotate.lci", m_actionFactory->modify_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Info"), "info", ":/icons/measure.lci", m_actionFactory->info_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Order"), "order", ":/icons/order.lci", m_actionFactory->order_actions);

    topMenuMenus << m_menuToolsCombined;
}

void LC_MenuFactory::createFileMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    m_menuFile = menu(tr("&File"), "file", menu_bar, {
                         "FileNew",
                         "FileNewTemplate",
                         "FileOpen",
                         "",
                         "FileSave",
                         "FileSaveAs",
                         "FileSaveAll",
                         ""
                     });

    m_menuRecentFiles = new QMenu(tr("Recent Files"), m_menuFile);

    m_menuFile->addMenu(m_menuRecentFiles);

    subMenu(m_menuFile, tr("Import"), "import", ":/icons/import.lci", {
             "DrawImage",
             "BlocksImport"
         });

    subMenu(m_menuFile, tr("Export"), "export", ":/icons/export.lci", {
        "FileExportMakerCam",
        "FilePrintPDF",
        "FileExport"
    });

    addActions(m_menuFile, {
        "",
        "FilePrint",
        "FilePrintPreview",
        "",
        "FileClose",
        "FileCloseAll",
        "FileQuit",
        ""
    });

    topMenuMenus << m_menuFile;
}

void LC_MenuFactory::createSettingsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    m_menuSettings = menu(tr("&Options"),"options", menu_bar, {
                             "OptionsGeneral",
                             "ShortcutsOptions",
                             "WidgetOptions",
                             "DeviceOptions",
                             "ReloadStyleSheet",
                             "",
                             "OptionsDrawing",
                         });

    topMenuMenus << m_menuSettings;
}

void LC_MenuFactory::createEditMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    m_menuEdit = menu(tr("&Edit"),"edit", menu_bar, {
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

    topMenuMenus << m_menuEdit;
}

void LC_MenuFactory::createViewMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    m_menuView = menu(tr("&View"), "view", menu_bar, {
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

    subMenu(m_menuView, tr("&Views Restore"), "view_restore", ":/icons/nview_visible.lci",
                                     {"ZoomViewRestore1",
                                      "ZoomViewRestore2",
                                      "ZoomViewRestore3",
                                      "ZoomViewRestore4",
                                      "ZoomViewRestore5"});

    topMenuMenus << m_menuView;
}

void LC_MenuFactory::createPluginsMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    m_menuPlugins = menu(tr("Pl&ugins"),"plugins", menu_bar);
    topMenuMenus << m_menuPlugins;
}

void LC_MenuFactory::createWorkspaceMenu(QMenuBar *menu_bar, QList<QMenu *> &topMenuMenus){
    m_menuWorkspace = menu(tr("&Workspace"), "workspaces", menu_bar, {
                             "Fullscreen" // temp way to show this menu on OS X
                         });

    connect(m_menuWorkspace, &QMenu::aboutToShow, m_appWin, &QC_ApplicationWindow::slotWorkspacesMenuAboutToShow);
    topMenuMenus << m_menuWorkspace;
}

void LC_MenuFactory::prepareWorkspaceMenuComponents(){

    m_menuDockareas = subMenu(m_menuWorkspace, tr("Dock Areas"), "dockareas", nullptr, {
                            "LeftDockAreaToggle",
                            "RightDockAreaToggle",
                            "TopDockAreaToggle",
                            "BottomDockAreaToggle",
                            "FloatingDockwidgetsToggle"
                        });

    m_menuDockWidgets = doCreateSubMenu(m_menuWorkspace, tr("Wid&gets"), "dockwidgets", nullptr);

    m_menuDockWidgets->addSeparator();

    QList<QDockWidget*> dockwidgetsList = m_appWin->findChildren<QDockWidget*>();
    m_appWin->sortWidgetsByTitle(dockwidgetsList);

    QAction* namedViewsToggleViewAction = nullptr;
    QAction* ucsToggleViewAction = nullptr;

    for (QDockWidget* dw: dockwidgetsList){
        if (m_appWin->dockWidgetArea(dw) == Qt::RightDockWidgetArea) {
            QAction *action = dw->toggleViewAction();
            m_menuDockWidgets->QWidget::addAction(action);
            QString objectName = dw->objectName();
            if (objectName == "view_dockwidget"){
                namedViewsToggleViewAction = action;
            }
            else if (objectName == "ucs_dockwidget"){
                ucsToggleViewAction = action;
            }
        }
    }

    m_menuView->QWidget::addAction(namedViewsToggleViewAction);
    m_menuView->addSeparator();
    m_menuView->QWidget::addAction(m_actionGroupManager->getActionByName("UCSCreate"));
    m_menuView->QWidget::addAction(m_actionGroupManager->getActionByName("UCSSetWCS"));
    m_menuView->QWidget::addAction(m_actionGroupManager->getActionByName("UCSSetByDimOrdinate"));
    m_menuView->QWidget::addAction(ucsToggleViewAction);

    m_menuDockWidgets->addSeparator();

    bool cadDocWidgetsAreEnabled = LC_GET_ONE_BOOL("Startup","EnableLeftSidebar", true);
    if (cadDocWidgetsAreEnabled) {
        m_menuCADDockWidgets = doCreateSubMenu(m_menuWorkspace, tr("CAD Wid&gets"), "caddockwidgets", nullptr);
        for (QDockWidget* dw : dockwidgetsList){
            if (m_appWin->dockWidgetArea(dw) == Qt::LeftDockWidgetArea) {
                m_menuCADDockWidgets->QWidget::addAction(dw->toggleViewAction());
            }
        }
    }

    m_menuToolbars = doCreateSubMenu(m_menuWorkspace, tr("&Toolbars"), "toolbars", nullptr);

    QList<QToolBar*> toolbarsList = m_appWin->findChildren<QToolBar*>();

    bool cadToolbarsAreEnabled = LC_GET_ONE_BOOL("Startup","EnableLeftSidebar", true);
    if (cadToolbarsAreEnabled) {
        QList<QToolBar*> cadToolbarsList;
        QList<QToolBar*> otherToolbarsList;
        for (QToolBar* tb: toolbarsList){
            const QVariant &variant = tb->property("_group");
            int group = variant.toInt();
            if (group == 2){
                cadToolbarsList << tb;
            }
            else {
                otherToolbarsList << tb;
            }
        }
        m_appWin->sortWidgetsByGroupAndTitle(cadToolbarsList);

        m_menuCADToolbars = doCreateSubMenu(m_menuWorkspace, tr("&CAD Toolbars"), "cadtoolbars", nullptr);
        for (QToolBar* tb: cadToolbarsList) {
            m_menuCADToolbars->QWidget::addAction(tb->toggleViewAction());
        }
        toolbarsList = otherToolbarsList;
    }

    m_appWin->sortWidgetsByGroupAndTitle(toolbarsList);
    int previousGroup = -100;

    for (QToolBar* tb: toolbarsList){
        const QVariant &variant = tb->property("_group");
        int group = variant.toInt();
        if (group != previousGroup ){
            if (previousGroup != -100) {
                m_menuToolbars->addSeparator();
            }
            previousGroup = group;
        }
        m_menuToolbars->QWidget::addAction(tb->toggleViewAction());
    }
}

void LC_MenuFactory::onWorkspaceMenuAboutToShow(const QList<QC_MDIWindow*> &window_list){
    LC_GROUP_GUARD("WindowOptions");
    {
        QIcon wsIcon = QIcon(":/icons/workspace.lci");
        m_menuWorkspace->clear(); // this is a temporary menu; constructed on-demand
        m_allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
        QMenu *menu;

        m_menuWorkspace->addAction(m_actionGroupManager->getActionByName("Fullscreen"));
        m_menuWorkspace->addAction(m_actionGroupManager->getActionByName("ViewStatusBar"));

        m_menuWorkspace->addSeparator();
        m_menuWorkspace->addMenu(m_menuDockWidgets);
        m_menuWorkspace->addMenu(m_menuToolbars);
        addAction(m_menuWorkspace, "RedockWidgets");
        m_menuWorkspace->addSeparator();

        bool needSeparator = false;
        if (m_menuCADDockWidgets != nullptr) {
            m_menuWorkspace->addMenu(m_menuCADDockWidgets);
            needSeparator = true;
        }

        if (m_menuCADToolbars != nullptr) {
            m_menuWorkspace->addMenu(m_menuCADToolbars);
            needSeparator = true;
        }

        if (needSeparator) {
            m_menuWorkspace->addSeparator();
        }

        m_menuWorkspace->addMenu(m_menuDockareas);
        addAction(m_menuWorkspace, "RedockWidgets");
        m_menuWorkspace->addSeparator();
        m_menuWorkspace->addAction(m_actionGroupManager->getActionByName("WorkspaceCreate"));

         QList<QPair<int, QString>> workspacesList;
         m_appWin->fillWorkspacesList(workspacesList);
         if (!workspacesList.isEmpty()){
             auto workspaces = new QMenu(tr("&Workspaces"), m_menuWorkspace);
             workspaces->setTearOffEnabled(m_allowTearOffMenus);
             workspaces->setIcon(wsIcon);
             qsizetype workspacesCount = workspacesList.size();
             for (int i = 0; i < workspacesCount; i++){
                 const auto w = workspacesList.at(i);
                 auto name = w.second;
                 auto* a = workspaces->addAction(wsIcon, name);
                 connect(a, &QAction::triggered, m_appWin, &QC_ApplicationWindow::restoreWorkspace);
                 a->setEnabled(true);
                 a->setCheckable(false);
                 a->setVisible(true);
                 a->setProperty("_WSPS_IDX", QVariant(w.first));
             }
             m_menuWorkspace->addMenu(workspaces);
             m_menuWorkspace->addAction(m_actionGroupManager->getActionByName("WorkspaceRemove"));
         }
         m_menuWorkspace->addSeparator();

         addAction(m_menuWorkspace, "InvokeMenuCreator");
         addAction(m_menuWorkspace, "InvokeToolbarCreator");
         m_menuWorkspace->addSeparator();

         auto drawings = new QMenu(tr("&Drawings"), m_menuWorkspace);
         drawings->setTearOffEnabled(m_allowTearOffMenus);
         m_menuWorkspace->addMenu(drawings);

         auto mdi_area = m_appWin->getMdiArea();
         auto mdiViewMode = mdi_area->viewMode();
         bool tabbed = mdiViewMode == QMdiArea::TabbedView;

         QAction* menuItem = drawings->addAction(tr("Ta&b mode"), m_appWin, &LC_MDIApplicationWindow::slotToggleTab);
         menuItem->setCheckable(true);
         menuItem->setChecked(tabbed);

         menuItem = drawings->addAction(tr("&Window mode"), m_appWin, &LC_MDIApplicationWindow::slotToggleTab);
         menuItem->setCheckable(true);
         menuItem->setChecked(!tabbed);


         if (tabbed) {
             menu = new QMenu(tr("&Layout"), m_menuWorkspace);
             menu->setTearOffEnabled(m_allowTearOffMenus);
             drawings->addMenu(menu);

             menuItem = menu->addAction(tr("Rounded"), m_appWin, &LC_MDIApplicationWindow::slotTabShapeRounded);
             menuItem->setCheckable(true);

             int tabShape = LC_GET_INT("TabShape");
             menuItem->setChecked(tabShape == RS2::Rounded);

             menuItem = menu->addAction(tr("Triangular"), m_appWin, &LC_MDIApplicationWindow::slotTabShapeTriangular);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabShape == RS2::Triangular);

             menu->addSeparator();
             int tabPosition = LC_GET_INT("TabPosition");

             menuItem = menu->addAction(tr("North"), m_appWin, &LC_MDIApplicationWindow::slotTabPositionNorth);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::North);

             menuItem = menu->addAction(tr("South"), m_appWin, &LC_MDIApplicationWindow::slotTabPositionSouth);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::South);

             menuItem = menu->addAction(tr("East"), m_appWin, &LC_MDIApplicationWindow::slotTabPositionEast);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::East);

             menuItem = menu->addAction(tr("West"), m_appWin, &LC_MDIApplicationWindow::slotTabPositionWest);
             menuItem->setCheckable(true);
             menuItem->setChecked(tabPosition == RS2::West);

         } else {

             menu = new QMenu(tr("&Arrange"), m_menuWorkspace);
             menu->setTearOffEnabled(m_allowTearOffMenus);
             m_menuWorkspace->addMenu(menu);

             menuItem = menu->addAction(tr("&Maximized"), m_appWin, &LC_MDIApplicationWindow::slotSetMaximized);
             menuItem->setCheckable(true);
             menuItem->setChecked(LC_GET_INT("SubWindowMode") == RS2::Maximized);

             menu->addAction(tr("&Cascade"), m_appWin, &LC_MDIApplicationWindow::slotCascade);
             menu->addAction(tr("&Tile"), m_appWin, &LC_MDIApplicationWindow::slotTile);
             menu->addAction(tr("Tile &Vertically"), m_appWin, &LC_MDIApplicationWindow::slotTileVertical);
             menu->addAction(tr("Tile &Horizontally"), m_appWin, &LC_MDIApplicationWindow::slotTileHorizontal);
         }

         m_menuWorkspace->addSeparator();
         QMdiSubWindow *active = mdi_area->activeSubWindow();
         for (int i = 0; i < window_list.size(); ++i) {
             QString title = window_list.at(i)->windowTitle();
             if (title.contains("[*]")) { // modification mark placeholder
                 qsizetype idx = title.lastIndexOf("[*]");
                 if (window_list.at(i)->isWindowModified()) {
                     title.replace(idx, 3, "*");
                 } else {
                     title.remove(idx, 3);
                 }
             }
             // QAction *id = m_menuWorkspace->addAction(title, m_appWin, SLOT(slotWindowsMenuActivated(bool)));
             QAction *id = m_menuWorkspace->addAction(title, m_appWin, &QC_ApplicationWindow::slotWindowsMenuActivated);
             id->setCheckable(true);
             id->setData(i);
             id->setChecked(window_list.at(i) == active);
         }
    }
}

QAction* LC_MenuFactory::urlActionTR(const QString& title, const char* url ){
    auto* result    = new QAction(  title, m_appWin);
    connect(result, &QAction::triggered, m_appWin, [=](){
        QDesktopServices::openUrl( QUrl( url));
    });
    return result;
}

QMenu*  LC_MenuFactory::subMenu(QMenu* parent, const QString& title, const QString& name, const char* icon, const std::vector<QString> &actionNames) const {
    QMenu *result = doCreateSubMenu(parent, title, name, icon);
    addActions(result, actionNames);
    return result;
}

QMenu*  LC_MenuFactory::subMenuWithActions(QMenu* parent, const QString& title, const QString& name, const char* icon, const QList<QAction*> &actions) const {
    QMenu *sub_menu = doCreateSubMenu(parent, title, name, icon);
    sub_menu->addActions(actions);
    return sub_menu;
}

QMenu *LC_MenuFactory::doCreateSubMenu(QMenu *parent, const QString& title, const QString& name, const char *icon) const {
    auto sub_menu = parent->addMenu(title);
    if (icon != nullptr) {
        sub_menu->setIcon(QIcon(icon));
    }
    sub_menu->setTearOffEnabled(m_allowTearOffMenus);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    const QString &objectName = nameCleared.toLower() + "_menu";
    sub_menu->setObjectName(objectName);
    return sub_menu;
}

QMenu*  LC_MenuFactory::menu(const QString& title, const QString& name, QMenuBar* parent) const {
    auto result =  new QMenu(title, parent);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    result->setObjectName(nameCleared.toLower() + "_menu");
    result->setTearOffEnabled(m_allowTearOffMenus);
    return result;
}

QMenu*  LC_MenuFactory::menu(const QString& title, const QString& name,  QMenuBar* parent, const std::vector<QString> &actionNames) const {
    QMenu* result = menu(title, name, parent);
    addActions(result, actionNames);
    return result;
}

void LC_MenuFactory::addAction(QMenu* menu, const char* actionName) const {
    QAction *action = m_actionGroupManager->getActionByName(actionName);
    if (action != nullptr) {
        menu->addAction(action);
    }
}

void LC_MenuFactory::addActions(QMenu *result, const std::vector<QString> &actionNames) const {
    for (const QString& actionName : actionNames){
        if (actionName.isEmpty()){
            result->addSeparator();
        }
        else{
            QAction* action = m_actionGroupManager->getActionByName(actionName);
            if (action != nullptr){
                result->addAction(action);
            }
        }
    }
}

QMenu* LC_MenuFactory::createMainWindowPopupMenu() const {
    auto *result = new QMenu(tr("Context"));
    result->setAttribute(Qt::WA_DeleteOnClose);

    auto *tmpToolbarsMenu = new QMenu(tr("Toolbars"), result);
    tmpToolbarsMenu->addActions(m_menuToolbars->actions());
    result->addMenu(tmpToolbarsMenu);

    auto *tmpWidgetsMenu = new QMenu(tr("Widgets"), result);
    tmpWidgetsMenu->addActions(m_menuDockWidgets->actions());
    result->addMenu(tmpWidgetsMenu);

    result->addSeparator();

    bool needSeparator = false;
    if (m_menuCADDockWidgets != nullptr) {
        auto *tmpCADWidgetsMenu = new QMenu(tr("CAD Widgets"), result);
        tmpCADWidgetsMenu->addActions(m_menuCADDockWidgets->actions());
        result->addMenu(tmpCADWidgetsMenu);
        needSeparator = true;
    }
    if (m_menuCADToolbars != nullptr) {
        auto *tmpCADToolbarsMenu = new QMenu(tr("CAD Toolbars"), result);
        tmpCADToolbarsMenu->addActions(m_menuCADToolbars->actions());
        result->addMenu(tmpCADToolbarsMenu);
        needSeparator = true;
    }
    if (needSeparator) {
        result->addSeparator();
    }

    QAction *viewStatusBarAction = m_actionGroupManager->getActionByName("ViewStatusBar");
    if (viewStatusBarAction != nullptr) {
        result->addAction(viewStatusBarAction);
    }

    return result;
}
