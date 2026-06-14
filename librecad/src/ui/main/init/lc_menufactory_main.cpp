/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_menufactory_main.h"

#include <QDesktopServices>
#include <QDockWidget>
#include <QMdiArea>
#include <QToolBar>
#include <QUrl>

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "lc_menufactory.h"
#include "main.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "rs_settings.h"
class QToolBar;


void LC_MenuFactoryMain::recreateMainMenuIfNeeded(QMenuBar* menuBar) {
    MenuOptions options;
    LC_GROUP("Startup");
    {
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

void LC_MenuFactoryMain::createMainMenu(QMenuBar* menuBar) {
    LC_GROUP("Startup");
    {
        m_menuOptions.expandToolsMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        m_menuOptions.expandToolsTillEntity = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
    }
    doCreateMenus(menuBar, true);
}

void LC_MenuFactoryMain::doCreateMenus(QMenuBar* menuBar, const bool firstCreation) {
    QList<QMenu*> topMenuMenus;
    if (firstCreation) {
        createFileMenu(menuBar, topMenuMenus);
        createSettingsMenu(menuBar, topMenuMenus);
        createEditMenu(menuBar, topMenuMenus);
        createViewMenu(menuBar, topMenuMenus);
        createPluginsMenu(menuBar, topMenuMenus);
        if (m_menuOptions.expandToolsMenu) {
            const auto select = menu(tr("&Select"), "select", menuBar);
            select->addActions(m_actionFactory->selectActions);
            select->addAction(m_actionGroupManager->getActionByName("SelectionModeToggle"));
            topMenuMenus << select;
        }
        createToolsMenu(menuBar, topMenuMenus);
        createWorkspaceMenu(menuBar, topMenuMenus);
        prepareWorkspaceMenuComponents();
        createHelpMenu(menuBar, topMenuMenus);
    }
    else {
        // we re-add previously created menus into the menu bar.
        // we can't recreate them fully, as recent files menu should survive the menu update, as it referenced by RS_RecentFiles instance.
        topMenuMenus << m_menusHolder->m_menuFile;
        topMenuMenus << m_menusHolder->m_menuSettings;
        topMenuMenus << m_menusHolder->m_menuEdit;
        topMenuMenus << m_menusHolder->m_menuView;
        topMenuMenus << m_menusHolder->m_menuPlugins;
        if (m_menuOptions.expandToolsMenu) {
            const auto select = menu(tr("&Select"), "select", menuBar);
            select->addActions(m_actionFactory->selectActions);
            select->addAction(m_actionGroupManager->getActionByName("SelectionModeToggle"));
            topMenuMenus << select;
            // fixme - sand - icons - potentially this may lead to the waste if menus are switched often (which is hardly the case but still).
            // this is due to the fact that menubar.clear() does not delete original actions/sub menus, but just removes them from the list.
            // From the other side, as menubar is the owner for them, they will be deleted as menu bar will be deleted.
            // potentially, we should create all menus just once.
            createToolsMenuExpanded(menuBar, topMenuMenus);
        }
        else {
            if (m_menusHolder->m_menuToolsCombined == nullptr) {
                createToolsMenuCombined(menuBar, topMenuMenus);
            }
            else {
                topMenuMenus << topMenuMenus;
            }
        }
        topMenuMenus << m_menusHolder->m_menuWorkspace;
        topMenuMenus << m_menusHolder->m_menuHelp;
    }

    for (const auto m : std::as_const(topMenuMenus)) {
        menuBar->addMenu(m);
    }
}

void LC_MenuFactoryMain::createHelpMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) {
    const auto menuHelp = menu(tr("&Help"), "help", menuBar);
    m_menusHolder->m_menuHelp = menuHelp;

    subMenuWithActions(menuHelp, tr("On&line Docs"), "OnlineInfo", nullptr, {
                           urlActionTR(tr("&Wiki"), "https://dokuwiki.librecad.org/"),
                           urlActionTR(tr("User's &Manual"), "https://librecad.readthedocs.io/"),
                           urlActionTR(tr("&Commands"), "https://librecad.readthedocs.io/en/latest/ref/tools.html"),
                           urlActionTR(tr("&Style Sheets"),
                                       "https://librecad.readthedocs.io/en/latest/ref/customize.html#style-sheets"),
                           urlActionTR(tr("Wid&gets"),
                                       "https://librecad.readthedocs.io/en/latest/ref/menu.html#widgets")
                       });

    const auto help_about = new QAction(QIcon(":/images/librecad.png"), tr("About"), m_appWin);
    connect(help_about, &QAction::triggered, m_appWin, &QC_ApplicationWindow::showAboutWindow);

    const auto license = new QAction(QObject::tr("License"), m_appWin);
    connect(license, &QAction::triggered, m_appWin, &QC_ApplicationWindow::invokeLicenseWindow);

    menuHelp->addSeparator();
    menuHelp->QWidget::addAction(urlActionTR(tr("&Forum"), "https://forum.librecad.org/"));
    menuHelp->QWidget::addAction(urlActionTR(tr("Zulip &Chat"), "https://librecad.zulipchat.com/"));
    menuHelp->addSeparator();
    menuHelp->QWidget::addAction(urlActionTR(tr("&Submit Error"), "https://github.com/LibreCAD/LibreCAD/issues/new"));
    menuHelp->QWidget::addAction(urlActionTR(tr("&Request Feature"), "https://github.com/LibreCAD/LibreCAD/issues"));
    menuHelp->QWidget::addAction(urlActionTR(tr("&Releases Page"), "https://github.com/LibreCAD/LibreCAD/releases"));
    menuHelp->addSeparator();

    if (XSTR(LC_PRERELEASE)) {
        // fixme - this is makes sense only for pre-release versions. Of course, the generation of tag included into URL should be more intelligent..
        menuHelp->QWidget::addAction(urlActionTR(tr("&Dev Snapshot Release"), " https://github.com/LibreCAD/LibreCAD/releases/tag/2.2.2_alpha-latest"));
        menuHelp->addSeparator();
    }

    menuHelp->QWidget::addAction(help_about);
    menuHelp->QWidget::addAction(license);
    menuHelp->QWidget::addAction(urlActionTR(tr("&Donate"), "https://librecad.org/donate.html"));

    topMenuMenus << menuHelp;
}

void LC_MenuFactoryMain::createToolsMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    if (m_menuOptions.expandToolsMenu) {
        createToolsMenuExpanded(menuBar, topMenuMenus);
    }
    else {
        createToolsMenuCombined(menuBar, topMenuMenus);
    }
}

void LC_MenuFactoryMain::createToolsMenuExpanded(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    if (m_menuOptions.expandToolsTillEntity) {
        const auto line = menu(tr("&Line"), "line", menuBar);
        line->addActions(m_actionFactory->lineActions);
        topMenuMenus << line;

        const auto point = menu(tr("Poin&t"), "point", menuBar);
        point->addActions(m_actionFactory->pointActions);
        topMenuMenus << point;

        const auto circle = menu(tr("&Circle"), "circle", menuBar);
        circle->addActions(m_actionFactory->circleActions);
        topMenuMenus << circle;

        const auto arc = menu(tr("&Arc"), "arc", menuBar);
        arc->addActions(m_actionFactory->curveActions);
        topMenuMenus << arc;

        const auto shape = menu(tr("Poly&gon"), "shape", menuBar);
        shape->addActions(m_actionFactory->shapeActions);
        topMenuMenus << shape;

        const auto spline = menu(tr("Splin&e"), "spline", menuBar);
        spline->addActions(m_actionFactory->splineActions);
        topMenuMenus << spline;

        const auto ellipse = menu(tr("Ellip&se"), "ellipse", menuBar);
        ellipse->addActions(m_actionFactory->ellipseActions);
        topMenuMenus << ellipse;

        const auto polyline = menu(tr("&Polyline"), "polyline", menuBar);
        polyline->addActions(m_actionFactory->polylineActions);
        topMenuMenus << polyline;

        const auto other = menu(tr("&Other"), "other", menuBar);
        other->addActions(m_actionFactory->otherDrawingActions);
        topMenuMenus << other;
    }
    else {
        const auto draw = menu(tr("&Draw"), "draw", menuBar);
        subMenuWithActions(draw, tr("&Line"), "line", ":/icons/line.lci", m_actionFactory->lineActions);
        subMenuWithActions(draw, tr("Poin&t"), "point", ":/icons/points.lci", m_actionFactory->pointActions);
        subMenuWithActions(draw, tr("&Circle"), "circle", ":/icons/circle.lci", m_actionFactory->circleActions);
        subMenuWithActions(draw, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci",
                           m_actionFactory->curveActions);
        subMenuWithActions(draw, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci",
                           m_actionFactory->shapeActions);
        subMenuWithActions(draw, tr("Splin&e"), "spline", ":/icons/spline_points.lci", m_actionFactory->splineActions);
        subMenuWithActions(draw, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci", m_actionFactory->ellipseActions);
        subMenuWithActions(draw, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci",
                           m_actionFactory->polylineActions);
        subMenuWithActions(draw, tr("Ot&her"), "other", ":/icons/text.lci", m_actionFactory->otherDrawingActions);

        topMenuMenus << draw;
    }

    const auto modify = menu(tr("&Modify"), "info", menuBar);
    modify->addActions(m_actionFactory->modifyActions);
    subMenuWithActions(modify, tr("&Order"), "order", ":/icons/order.lci", m_actionFactory->orderActions);

    topMenuMenus << modify;

    const auto dims = menu(tr("&Dimensions"), "dims", menuBar);
    dims->addActions(m_actionFactory->dimension_Actions);
    topMenuMenus << dims;

    const auto info = menu(tr("&Info"), "info", menuBar);
    info->addActions(m_actionFactory->infoActions);
    topMenuMenus << info;
}

void LC_MenuFactoryMain::createToolsMenuCombined(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    const auto menuToolsCombined = menu(tr("&Tools"), "tools", menuBar);
    m_menusHolder-> m_menuToolsCombined = menuToolsCombined;
    subMenuWithActions(menuToolsCombined, tr("&Line"), "line", ":/icons/line.lci", m_actionFactory->lineActions);
    subMenuWithActions(menuToolsCombined, tr("Poin&t"), "line", ":/icons/points.lci", m_actionFactory->pointActions);
    subMenuWithActions(menuToolsCombined, tr("&Circle"), "circle", ":/icons/circle.lci",
                       m_actionFactory->circleActions);
    subMenuWithActions(menuToolsCombined, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci",
                       m_actionFactory->curveActions);
    subMenuWithActions(menuToolsCombined, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci",
                       m_actionFactory->shapeActions);
    subMenuWithActions(menuToolsCombined, tr("Splin&e"), "spline", ":/icons/spline_points.lci",
                       m_actionFactory->splineActions);
    subMenuWithActions(menuToolsCombined, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci",
                       m_actionFactory->ellipseActions);
    subMenuWithActions(menuToolsCombined, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci",
                       m_actionFactory->polylineActions);
    const auto selectMenu = subMenuWithActions(menuToolsCombined, tr("&Select"), "select", ":/icons/select.lci",
                       m_actionFactory->selectActions);
    selectMenu->addAction(m_actionGroupManager->getActionByName("SelectionModeToggle"));
    subMenuWithActions(menuToolsCombined, tr("Dime&nsion"), "dimension", ":/icons/dim_horizontal.lci",
                       m_actionFactory->dimension_Actions);
    subMenuWithActions(menuToolsCombined, tr("Ot&her"), "other", ":/icons/text.lci",
                       m_actionFactory->otherDrawingActions);
    subMenuWithActions(menuToolsCombined, tr("&Modify"), "modify", ":/icons/move_rotate.lci",
                       m_actionFactory->modifyActions);
    subMenuWithActions(menuToolsCombined, tr("&Info"), "info", ":/icons/measure.lci", m_actionFactory->infoActions);
    subMenuWithActions(menuToolsCombined, tr("&Order"), "order", ":/icons/order.lci", m_actionFactory->orderActions);

    topMenuMenus << menuToolsCombined;
}

void LC_MenuFactoryMain::createFileMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    const auto menuFile = menu(tr("&File"), "file", menuBar, {
                          "FileNew",
                          "FileNewTemplate",
                          "FileOpen"
                      });

    m_menusHolder->m_menuFile = menuFile;

    m_menusHolder->m_menuRecentFiles = new QMenu(tr("Recent Files"), menuFile);

    menuFile->addMenu(m_menusHolder->m_menuRecentFiles);

    addActions(menuFile, {
                          "",
                          "FileSave",
                          "FileSaveAs",
                          "FileSaveAll",
                          ""
                      });

    subMenu(menuFile, tr("Import"), "import", ":/icons/import.lci", {
                "DrawImage",
                "BlocksImport"
            });

    subMenu(menuFile, tr("Export"), "export", ":/icons/export.lci", {
                "FileExportMakerCam",
                "FilePrintPDF",
                "FileExport"
            });

    addActions(menuFile, {
                   "",
                   "FilePrint",
                   "FilePrintPreview",
                   "",
                   "FileClose",
                   "FileCloseAll",
                   "FileQuit",
                   ""
               });

    topMenuMenus << menuFile;
}

void LC_MenuFactoryMain::createSettingsMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    m_menusHolder->m_menuSettings = menu(tr("&Options"), "options", menuBar, {
                              "OptionsGeneral",
                              "ShortcutsOptions",
                              "WidgetOptions",
                              "DeviceOptions",
                              "ReloadStyleSheet",
                              "",
                              "OptionsDrawing",
                          });

    topMenuMenus << m_menusHolder->m_menuSettings;
}

void LC_MenuFactoryMain::createEditMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    m_menusHolder->m_menuEdit = menu(tr("&Edit"), "edit", menuBar, {
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

    topMenuMenus << m_menusHolder->m_menuEdit;
}

void LC_MenuFactoryMain::createViewMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    m_menusHolder->m_menuView = menu(tr("&View"), "view", menuBar, {
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

    subMenu(m_menusHolder->m_menuView, tr("&Views Restore"), "view_restore", ":/icons/nview_visible.lci",
            {
                "ZoomViewRestore1",
                "ZoomViewRestore2",
                "ZoomViewRestore3",
                "ZoomViewRestore4",
                "ZoomViewRestore5"
            });

    topMenuMenus << m_menusHolder->m_menuView;
}

void LC_MenuFactoryMain::createPluginsMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) const {
    m_menusHolder->m_menuPlugins = menu(tr("Pl&ugins"), "plugins", menuBar);
    m_menusHolder->m_menuPlugins ->setToolTipsVisible(true);
    topMenuMenus << m_menusHolder->m_menuPlugins;
}

void LC_MenuFactoryMain::createWorkspaceMenu(QMenuBar* menuBar, QList<QMenu*>& topMenuMenus) {
    m_menusHolder->m_menuWorkspace = menu(tr("&Workspace"), "workspaces", menuBar, {
                               "Fullscreen" // temp way to show this menu on OS X
                           });

    connect(m_menusHolder->m_menuWorkspace, &QMenu::aboutToShow, m_appWin, &QC_ApplicationWindow::slotWorkspacesMenuAboutToShow);
    topMenuMenus << m_menusHolder->m_menuWorkspace;
}

void LC_MenuFactoryMain::prepareWorkspaceMenuComponents() const {
    const auto menuWorkspace = m_menusHolder->m_menuWorkspace;
    m_menusHolder->m_menuDockAreas = subMenu(menuWorkspace, tr("Dock Areas"), "dockareas", nullptr, {
                                                 "LeftDockAreaToggle",
                                                 "RightDockAreaToggle",
                                                 "TopDockAreaToggle",
                                                 "BottomDockAreaToggle",
                                                 "FloatingDockwidgetsToggle"
                                             }, m_allowTearOffMenus);

    m_menusHolder->m_menuToolBarAreas = subMenu(menuWorkspace, tr("Toolbar Areas"), "tbareas", nullptr, {
                                  "LeftTBAreaToggle",
                                  "RightTBAreaToggle",
                                  "TopTBAreaToggle",
                                  "BottomTBAreaToggle"
                              }, m_allowTearOffMenus);

    m_menusHolder->m_menuDockWidgets = doCreateSubMenu(menuWorkspace, tr("Wid&gets"), "dockwidgets", nullptr, m_allowTearOffMenus);

    m_menusHolder->m_menuDockWidgets->addSeparator();

    QList<QDockWidget*> dockwidgetsList = m_appWin->findChildren<QDockWidget*>();
    m_appWin->sortWidgetsByTitle(dockwidgetsList);

    QAction* namedViewsToggleViewAction = nullptr;
    QAction* ucsToggleViewAction = nullptr;

    findViewAndUCSToggleActions(dockwidgetsList, namedViewsToggleViewAction, ucsToggleViewAction);

    const auto menuView = m_menusHolder->m_menuView;

    menuView->addAction(namedViewsToggleViewAction);
    menuView->addSeparator();
    menuView->addAction(m_actionGroupManager->getActionByName("UCSCreate"));
    menuView->addAction(m_actionGroupManager->getActionByName("UCSSetWCS"));
    menuView->addAction(m_actionGroupManager->getActionByName("UCSSetByDimOrdinate"));
    menuView->addAction(ucsToggleViewAction);

    // fixme - sand - add menu to restore ucs!!!

    m_menusHolder->m_menuDockWidgets->addSeparator();

    const bool cadDocWidgetsAreEnabled = LC_GET_ONE_BOOL("Startup", "EnableLeftSidebar", true);

    if (cadDocWidgetsAreEnabled) {
        const auto menuCADDockWidgets = doCreateSubMenu(menuWorkspace, tr("CAD Wid&gets"), "caddockwidgets", nullptr, m_allowTearOffMenus);
        m_menusHolder->m_menuCADDockWidgets = menuCADDockWidgets;
        auto actions = QList<QAction*>();
        QAction* megaMenuAction = nullptr;
        for (QDockWidget* dw : std::as_const(dockwidgetsList)) {
            if (m_appWin->dockWidgetArea(dw) == Qt::LeftDockWidgetArea) {
                if (dw->objectName() == "dock_cad_mega") {
                    megaMenuAction = dw->toggleViewAction();
                }
                else {
                    actions.push_back(dw->toggleViewAction());
                }
            }
        }

        if (megaMenuAction != nullptr) {
            menuCADDockWidgets->QWidget::addAction(megaMenuAction);
            menuCADDockWidgets->addSeparator();
        }

        for (const auto action : std::as_const(actions)) {
            menuCADDockWidgets->QWidget::addAction(action);
        }
    }

    const auto menuToolbars = doCreateSubMenu(menuWorkspace, tr("&Toolbars"), "toolbars", nullptr, m_allowTearOffMenus);
    m_menusHolder->m_menuToolbars = menuToolbars;

    QList<QToolBar*> toolbarsList = m_appWin->findChildren<QToolBar*>();

    const bool cadToolbarsAreEnabled = LC_GET_ONE_BOOL("Startup", "EnableCADToolbars", true);
    if (cadToolbarsAreEnabled) {
        QList<QToolBar*> cadToolbarsList;
        QList<QToolBar*> otherToolbarsList;
        for (QToolBar* tb : std::as_const(toolbarsList)) {
            const QVariant& variant = tb->property("_group");
            const int group = variant.toInt();
            if (group == 2) {
                cadToolbarsList << tb;
            }
            else {
                otherToolbarsList << tb;
            }
        }

        m_appWin->sortWidgetsByGroupAndTitle(cadToolbarsList);
        m_menusHolder->m_menuCADToolbars = doCreateSubMenu(menuWorkspace, tr("&CAD Toolbars"), "cadtoolbars", nullptr, m_allowTearOffMenus);
        for (const QToolBar* tb : std::as_const(cadToolbarsList)) {
            m_menusHolder->m_menuCADToolbars->QWidget::addAction(tb->toggleViewAction());
        }
        toolbarsList = otherToolbarsList;
    }

    m_appWin->sortWidgetsByGroupAndTitle(toolbarsList);
    int previousGroup = -100;

    for (const QToolBar* tb : std::as_const(toolbarsList)) {
        const QVariant& variant = tb->property("_group");
        const int group = variant.toInt();
        if (group != previousGroup) {
            if (previousGroup != -100) {
                menuToolbars->addSeparator();
            }
            previousGroup = group;
        }
        menuToolbars->QWidget::addAction(tb->toggleViewAction());
    }
}

void LC_MenuFactoryMain::recreateToolbarsMenu() const {
    m_menusHolder->m_menuWorkspace->clear();
    prepareWorkspaceMenuComponents();
}


void LC_MenuFactoryMain::onWorkspaceMenuAboutToShow(const QList<QC_MDIWindow*>& windowList) {
    LC_GROUP_GUARD("WindowOptions");
    {
        const auto menuWorkspace = m_menusHolder->m_menuWorkspace;
        menuWorkspace->clear(); // this is a temporary menu; constructed on-demand
        m_allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
        QMenu* menu = nullptr;

        addActions(menuWorkspace, {
            "Fullscreen",
            "MainMenu",
            "ViewStatusBar"
        });

        menuWorkspace->addSeparator();
        menuWorkspace->addMenu(m_menusHolder->m_menuDockWidgets);
        menuWorkspace->addMenu(m_menusHolder->m_menuToolbars);
        addAction(menuWorkspace, "RedockWidgets");
        menuWorkspace->addSeparator();

        bool needSeparator = false;
        if (m_menusHolder->m_menuCADDockWidgets != nullptr) {
            menuWorkspace->addMenu(m_menusHolder->m_menuCADDockWidgets);
            needSeparator = true;
        }

        if (m_menusHolder->m_menuCADToolbars != nullptr) {
            menuWorkspace->addMenu(m_menusHolder->m_menuCADToolbars);
            needSeparator = true;
        }

        if (needSeparator) {
            menuWorkspace->addSeparator();
        }
        menuWorkspace->addMenu(m_menusHolder->m_menuToolBarAreas);
        menuWorkspace->addMenu(m_menusHolder->m_menuDockAreas);
        addAction(menuWorkspace, "RedockWidgets");
        menuWorkspace->addSeparator();
        menuWorkspace->addAction(m_actionGroupManager->getActionByName("WorkspaceCreate"));

        QMenu* workspacesListParentMenu = menuWorkspace;
        createWorkspacesListSubMenu(workspacesListParentMenu);
        menuWorkspace->addSeparator();

        addAction(menuWorkspace, "InvokeMenuCreator");
        addAction(menuWorkspace, "InvokeToolbarCreator");
        menuWorkspace->addSeparator();

        const auto drawings = new QMenu(tr("&Drawings"), menuWorkspace);
        drawings->setTearOffEnabled(m_allowTearOffMenus);
        menuWorkspace->addMenu(drawings);

        const auto mdi_area = m_appWin->getMdiArea();
        const auto mdiViewMode = mdi_area->viewMode();
        const bool tabbed = mdiViewMode == QMdiArea::TabbedView;

        QAction* menuItem = drawings->addAction(tr("Ta&b mode"), m_appWin, &LC_MDIApplicationWindow::slotToggleTab);
        menuItem->setCheckable(true);
        menuItem->setChecked(tabbed);

        menuItem = drawings->addAction(tr("&Window mode"), m_appWin, &LC_MDIApplicationWindow::slotToggleTab);
        menuItem->setCheckable(true);
        menuItem->setChecked(!tabbed);

        if (tabbed) {
            menu = new QMenu(tr("&Layout"), menuWorkspace);
            menu->setTearOffEnabled(m_allowTearOffMenus);
            drawings->addMenu(menu);

            menuItem = menu->addAction(tr("Rounded"), m_appWin, &LC_MDIApplicationWindow::slotTabShapeRounded);
            menuItem->setCheckable(true);

            const int tabShape = LC_GET_INT("TabShape");
            menuItem->setChecked(tabShape == RS2::Rounded);

            menuItem = menu->addAction(tr("Triangular"), m_appWin, &LC_MDIApplicationWindow::slotTabShapeTriangular);
            menuItem->setCheckable(true);
            menuItem->setChecked(tabShape == RS2::Triangular);

            menu->addSeparator();
            const int tabPosition = LC_GET_INT("TabPosition");

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
        }
        else {
            menu = new QMenu(tr("&Arrange"), menuWorkspace);
            menu->setTearOffEnabled(m_allowTearOffMenus);
            menuWorkspace->addMenu(menu);

            menuItem = menu->addAction(tr("&Maximized"), m_appWin, &LC_MDIApplicationWindow::slotSetMaximized);
            menuItem->setCheckable(true);
            menuItem->setChecked(LC_GET_INT("SubWindowMode") == RS2::Maximized);

            menu->addAction(tr("&Cascade"), m_appWin, &LC_MDIApplicationWindow::slotCascade);
            menu->addAction(tr("&Tile"), m_appWin, &LC_MDIApplicationWindow::slotTile);
            menu->addAction(tr("Tile &Vertically"), m_appWin, &LC_MDIApplicationWindow::slotTileVertical);
            menu->addAction(tr("Tile &Horizontally"), m_appWin, &LC_MDIApplicationWindow::slotTileHorizontal);
        }

        menuWorkspace->addSeparator();
        const QMdiSubWindow* active = mdi_area->activeSubWindow();
        for (int i = 0; i < windowList.size(); ++i) {
            QString title = windowList.at(i)->windowTitle();
            if (title.contains("[*]")) { // modification mark placeholder
                const qsizetype idx = title.lastIndexOf("[*]");
                if (windowList.at(i)->isWindowModified()) {
                    title.replace(idx, 3, "*");
                }
                else {
                    title.remove(idx, 3);
                }
            }
            // QAction *id = m_menuWorkspace->addAction(title, m_appWin, SLOT(slotWindowsMenuActivated(bool),
            QAction* id = menuWorkspace->addAction(title, m_appWin, &QC_ApplicationWindow::slotWindowsMenuActivated);
            id->setCheckable(true);
            id->setData(i);
            id->setChecked(windowList.at(i) == active);
        }
    }
}

QAction* LC_MenuFactoryMain::urlActionTR(const QString& title, const char* url) {
    auto* result = new QAction(title, m_appWin);
    connect(result, &QAction::triggered, m_appWin, [=] {
        QDesktopServices::openUrl(QUrl(url));
    });
    return result;
}

class QCDynamicMenu : public QMenu {
public:
    using QMenu::QMenu;

protected:
    void hideEvent(QHideEvent *event) override {
        QMenu::hideEvent(event);
        if (!isTearOffMenuVisible()) {
            this->deleteLater();
        }
    }
};

QMenu* LC_MenuFactoryMain::createMainWindowPopupMenu() const {
    auto* result = new QCDynamicMenu(tr("Context"), QC_ApplicationWindow::getAppWindow().get());
    // result->setAttribute(Qt::WA_DeleteOnClose);
    // result->setTearOffEnabled(m_allowTearOffMenus);

        addActions(result, {
           "Fullscreen",
           "MainMenu"
    });

    result->addMenu(m_menusHolder->m_menuToolBarAreas);
    result->addMenu(m_menusHolder->m_menuDockAreas);

    addActions(result, {
        "ViewStatusBar"
    });

    result->addSeparator();

    auto* tmpToolbarsMenu = new QMenu(tr("Toolbars"));
    tmpToolbarsMenu->addActions(m_menusHolder->m_menuToolbars->actions());
    tmpToolbarsMenu->setTearOffEnabled(m_allowTearOffMenus);
    result->addMenu(tmpToolbarsMenu);

    auto* tmpWidgetsMenu = new QMenu(tr("Widgets"));
    tmpWidgetsMenu->addActions(m_menusHolder->m_menuDockWidgets->actions());
    tmpWidgetsMenu->setTearOffEnabled(m_allowTearOffMenus);
    result->addMenu(tmpWidgetsMenu);

    result->addSeparator();

    bool needSeparator = false;
    if (m_menusHolder->m_menuCADDockWidgets != nullptr) {
        auto* tmpCADWidgetsMenu = new QMenu(tr("CAD Widgets"));
        tmpCADWidgetsMenu->setTearOffEnabled(m_allowTearOffMenus);
        tmpCADWidgetsMenu->addActions(m_menusHolder->m_menuCADDockWidgets->actions());
        result->addMenu(tmpCADWidgetsMenu);
        needSeparator = true;
    }
    if (m_menusHolder->m_menuCADToolbars != nullptr) {
        auto* tmpCADToolbarsMenu = new QMenu(tr("CAD Toolbars"));
        tmpCADToolbarsMenu->setTearOffEnabled(m_allowTearOffMenus);
        tmpCADToolbarsMenu->addActions(m_menusHolder->m_menuCADToolbars->actions());
        result->addMenu(tmpCADToolbarsMenu);
        needSeparator = true;
    }
    if (needSeparator) {
        result->addSeparator();
    }
    return result;
}

QMenu * LC_MenuFactoryMain::getRecentFilesMenu() const{
    return m_menusHolder->m_menuRecentFiles;
}
