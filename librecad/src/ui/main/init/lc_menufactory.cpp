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
#include <QMouseEvent>
#include <QUrl>

#include "lc_actiongroupmanager.h"
#include "lc_graphicviewport.h"
#include "muParserDef.h"
#include "qc_applicationwindow.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_insert.h"
#include "rs_polyline.h"
#include "rs_settings.h"

class QToolBar;

namespace {
    // Issue #1765: set default cursor size: 32x32
    constexpr int g_cursorSize = 32; // fixme - sand - move to common public place! Duplicate from QG_GraphicView
    // maximum length for displayed block name in context menu
    constexpr int g_MaxBlockNameLength = 40;
    // fixme - sand - move to common public place! Duplicate from QG_GraphicView
}

LC_MenuFactory::LC_MenuFactory(QC_ApplicationWindow* main_win)
    : QObject(nullptr)
      , LC_AppWindowAware(main_win)
      , m_actionGroupManager{main_win->m_actionGroupManager.get()}
      , m_actionFactory{main_win->m_actionFactory.get()} {
    m_allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
}

void LC_MenuFactory::recreateMainMenuIfNeeded(QMenuBar* menuBar) {
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

void LC_MenuFactory::createMainMenu(QMenuBar* menuBar) {
    LC_GROUP("Startup");
    {
        m_menuOptions.expandToolsMenu = LC_GET_BOOL("ExpandedToolsMenu", false);
        m_menuOptions.expandToolsTillEntity = LC_GET_BOOL("ExpandedToolsMenuTillEntity", false);
    }
    doCreateMenus(menuBar, true);
}

void LC_MenuFactory::doCreateMenus(QMenuBar* menu_bar, bool firstCreation) {
    QList<QMenu*> topMenuMenus;
    if (firstCreation) {
        createFileMenu(menu_bar, topMenuMenus);
        createSettingsMenu(menu_bar, topMenuMenus);
        createEditMenu(menu_bar, topMenuMenus);
        createViewMenu(menu_bar, topMenuMenus);
        createPluginsMenu(menu_bar, topMenuMenus);
        if (m_menuOptions.expandToolsMenu) {
            auto select = menu(tr("&Select"), "select", menu_bar);
            select->addActions(m_actionFactory->select_actions);
            topMenuMenus << select;
        }
        createToolsMenu(menu_bar, topMenuMenus);
        createWorkspaceMenu(menu_bar, topMenuMenus);
        prepareWorkspaceMenuComponents();
        createHelpMenu(menu_bar, topMenuMenus);
    }
    else {
        // we re-add previously created menus into the menu bar.
        // we can't recreate them fully, as recent files menu should survive the menu update, as it referenced by RS_RecentFiles instance.
        topMenuMenus << m_menuFile;
        topMenuMenus << m_menuSettings;
        topMenuMenus << m_menuEdit;
        topMenuMenus << m_menuView;
        topMenuMenus << m_menuPlugins;
        if (m_menuOptions.expandToolsMenu) {
            auto select = menu(tr("&Select"), "select", menu_bar);
            select->addActions(m_actionFactory->select_actions);
            topMenuMenus << select;
            // fixme - sand - icons - potentially this may lead to the waste if menus are switched often (which is hardly the case but still).
            // this is due to the fact that menubar.clear() does not delete original actions/sub menus, but just removes them from the list.
            // From the other side, as menubar is the owner for them, they will be deleted as menu bar will be deleted.
            // potentially, we should create all menus just once.
            createToolsMenuExpanded(menu_bar, topMenuMenus);
        }
        else {
            if (m_menuToolsCombined == nullptr) {
                createToolsMenuCombined(menu_bar, topMenuMenus);
            }
            else {
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

void LC_MenuFactory::createHelpMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
    m_menuHelp = menu(tr("&Help"), "help", menu_bar);

    subMenuWithActions(m_menuHelp, tr("On&line Docs"), "OnlineInfo", nullptr, {
                           urlActionTR(tr("&Wiki"), "https://dokuwiki.librecad.org/"),
                           urlActionTR(tr("User's &Manual"), "https://librecad.readthedocs.io/"),
                           urlActionTR(tr("&Commands"), "https://librecad.readthedocs.io/en/latest/ref/tools.html"),
                           urlActionTR(tr("&Style Sheets"),
                                       "https://librecad.readthedocs.io/en/latest/ref/customize.html#style-sheets"),
                           urlActionTR(tr("Wid&gets"),
                                       "https://librecad.readthedocs.io/en/latest/ref/menu.html#widgets")
                       });

    auto help_about = new QAction(QIcon(":/images/librecad.png"), tr("About"), m_appWin);
    connect(help_about, &QAction::triggered, m_appWin, &QC_ApplicationWindow::showAboutWindow);

    auto license = new QAction(QObject::tr("License"), m_appWin);
    connect(license, &QAction::triggered, m_appWin, &QC_ApplicationWindow::invokeLicenseWindow);

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

void LC_MenuFactory::createToolsMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
    if (m_menuOptions.expandToolsMenu) {
        createToolsMenuExpanded(menu_bar, topMenuMenus);
    }
    else {
        createToolsMenuCombined(menu_bar, topMenuMenus);
    }
}

void LC_MenuFactory::createToolsMenuExpanded(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) const {
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
        subMenuWithActions(draw, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci",
                           m_actionFactory->curve_actions);
        subMenuWithActions(draw, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci",
                           m_actionFactory->shape_actions);
        subMenuWithActions(draw, tr("Splin&e"), "spline", ":/icons/spline_points.lci", m_actionFactory->spline_actions);
        subMenuWithActions(draw, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci", m_actionFactory->ellipse_actions);
        subMenuWithActions(draw, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci",
                           m_actionFactory->polyline_actions);
        subMenuWithActions(draw, tr("Ot&her"), "other", ":/icons/text.lci", m_actionFactory->other_drawing_actions);

        topMenuMenus << draw;
    }

    auto modify = menu(tr("&Modify"), "info", menu_bar);
    modify->addActions(m_actionFactory->modify_actions);
    subMenuWithActions(modify, tr("&Order"), "order", ":/icons/order.lci", m_actionFactory->order_actions);

    topMenuMenus << modify;

    auto dims = menu(tr("&Dimensions"), "dims", menu_bar);
    dims->addActions(m_actionFactory->dimension_actions);
    topMenuMenus << dims;

    auto info = menu(tr("&Info"), "info", menu_bar);
    info->addActions(m_actionFactory->info_actions);
    topMenuMenus << info;
}

void LC_MenuFactory::createToolsMenuCombined(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
    m_menuToolsCombined = menu(tr("&Tools"), "tools", menu_bar);
    subMenuWithActions(m_menuToolsCombined, tr("&Line"), "line", ":/icons/line.lci", m_actionFactory->line_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Poin&t"), "line", ":/icons/points.lci", m_actionFactory->point_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Circle"), "circle", ":/icons/circle.lci",
                       m_actionFactory->circle_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Arc"), "curve", ":/icons/arc_center_point_angle.lci",
                       m_actionFactory->curve_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Poly&gon"), "polygon", ":/icons/rectangle_1_point.lci",
                       m_actionFactory->shape_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Splin&e"), "spline", ":/icons/spline_points.lci",
                       m_actionFactory->spline_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Ellipse"), "ellipse", ":/icons/ellipses.lci",
                       m_actionFactory->ellipse_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.lci",
                       m_actionFactory->polyline_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Select"), "select", ":/icons/select.lci",
                       m_actionFactory->select_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Dime&nsion"), "dimension", ":/icons/dim_horizontal.lci",
                       m_actionFactory->dimension_actions);
    subMenuWithActions(m_menuToolsCombined, tr("Ot&her"), "other", ":/icons/text.lci",
                       m_actionFactory->other_drawing_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Modify"), "modify", ":/icons/move_rotate.lci",
                       m_actionFactory->modify_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Info"), "info", ":/icons/measure.lci", m_actionFactory->info_actions);
    subMenuWithActions(m_menuToolsCombined, tr("&Order"), "order", ":/icons/order.lci", m_actionFactory->order_actions);

    topMenuMenus << m_menuToolsCombined;
}

void LC_MenuFactory::createFileMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
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

void LC_MenuFactory::createSettingsMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
    m_menuSettings = menu(tr("&Options"), "options", menu_bar, {
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

void LC_MenuFactory::createEditMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
    m_menuEdit = menu(tr("&Edit"), "edit", menu_bar, {
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

void LC_MenuFactory::createViewMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
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
            {
                "ZoomViewRestore1",
                "ZoomViewRestore2",
                "ZoomViewRestore3",
                "ZoomViewRestore4",
                "ZoomViewRestore5"
            });

    topMenuMenus << m_menuView;
}

void LC_MenuFactory::createPluginsMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
    m_menuPlugins = menu(tr("Pl&ugins"), "plugins", menu_bar);
    topMenuMenus << m_menuPlugins;
}

void LC_MenuFactory::createWorkspaceMenu(QMenuBar* menu_bar, QList<QMenu*>& topMenuMenus) {
    m_menuWorkspace = menu(tr("&Workspace"), "workspaces", menu_bar, {
                               "Fullscreen" // temp way to show this menu on OS X
                           });

    connect(m_menuWorkspace, &QMenu::aboutToShow, m_appWin, &QC_ApplicationWindow::slotWorkspacesMenuAboutToShow);
    topMenuMenus << m_menuWorkspace;
}

void LC_MenuFactory::findViewAndUCSToggleActions(QList<QDockWidget*> dockwidgetsList,
                                                 QAction*& namedViewsToggleViewAction, QAction*& ucsToggleViewAction) {
    for (QDockWidget* dw : dockwidgetsList) {
        if (m_appWin->dockWidgetArea(dw) == Qt::RightDockWidgetArea) {
            QAction* action = dw->toggleViewAction();
            m_menuDockWidgets->QWidget::addAction(action);
            QString objectName = dw->objectName();
            if (objectName == "view_dockwidget") {
                namedViewsToggleViewAction = action;
            }
            else if (objectName == "ucs_dockwidget") {
                ucsToggleViewAction = action;
            }
        }
    }
}

void LC_MenuFactory::prepareWorkspaceMenuComponents() {
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

    findViewAndUCSToggleActions(dockwidgetsList, namedViewsToggleViewAction, ucsToggleViewAction);

    m_menuView->addAction(namedViewsToggleViewAction);
    m_menuView->addSeparator();
    m_menuView->addAction(m_actionGroupManager->getActionByName("UCSCreate"));
    m_menuView->addAction(m_actionGroupManager->getActionByName("UCSSetWCS"));
    m_menuView->addAction(m_actionGroupManager->getActionByName("UCSSetByDimOrdinate"));
    m_menuView->addAction(ucsToggleViewAction);

    // fixme - sand - add menu to restore ucs!!!

    m_menuDockWidgets->addSeparator();

    bool cadDocWidgetsAreEnabled = LC_GET_ONE_BOOL("Startup", "EnableLeftSidebar", true);
    if (cadDocWidgetsAreEnabled) {
        m_menuCADDockWidgets = doCreateSubMenu(m_menuWorkspace, tr("CAD Wid&gets"), "caddockwidgets", nullptr);
        for (QDockWidget* dw : dockwidgetsList) {
            if (m_appWin->dockWidgetArea(dw) == Qt::LeftDockWidgetArea) {
                m_menuCADDockWidgets->QWidget::addAction(dw->toggleViewAction());
            }
        }
    }

    m_menuToolbars = doCreateSubMenu(m_menuWorkspace, tr("&Toolbars"), "toolbars", nullptr);

    QList<QToolBar*> toolbarsList = m_appWin->findChildren<QToolBar*>();

    bool cadToolbarsAreEnabled = LC_GET_ONE_BOOL("Startup", "EnableLeftSidebar", true);
    if (cadToolbarsAreEnabled) {
        QList<QToolBar*> cadToolbarsList;
        QList<QToolBar*> otherToolbarsList;
        for (QToolBar* tb : toolbarsList) {
            const QVariant& variant = tb->property("_group");
            int group = variant.toInt();
            if (group == 2) {
                cadToolbarsList << tb;
            }
            else {
                otherToolbarsList << tb;
            }
        }
        m_appWin->sortWidgetsByGroupAndTitle(cadToolbarsList);

        m_menuCADToolbars = doCreateSubMenu(m_menuWorkspace, tr("&CAD Toolbars"), "cadtoolbars", nullptr);
        for (QToolBar* tb : cadToolbarsList) {
            m_menuCADToolbars->QWidget::addAction(tb->toggleViewAction());
        }
        toolbarsList = otherToolbarsList;
    }

    m_appWin->sortWidgetsByGroupAndTitle(toolbarsList);
    int previousGroup = -100;

    for (QToolBar* tb : toolbarsList) {
        const QVariant& variant = tb->property("_group");
        int group = variant.toInt();
        if (group != previousGroup) {
            if (previousGroup != -100) {
                m_menuToolbars->addSeparator();
            }
            previousGroup = group;
        }
        m_menuToolbars->QWidget::addAction(tb->toggleViewAction());
    }
}

void LC_MenuFactory::onWorkspaceMenuAboutToShow(const QList<QC_MDIWindow*>& window_list) {
    LC_GROUP_GUARD("WindowOptions");
    {
        QIcon wsIcon = QIcon(":/icons/workspace.lci");
        m_menuWorkspace->clear(); // this is a temporary menu; constructed on-demand
        m_allowTearOffMenus = LC_GET_ONE_BOOL("Appearance", "AllowMenusTearOff", true);
        QMenu* menu;

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
        if (!workspacesList.isEmpty()) {
            auto workspaces = new QMenu(tr("&Workspaces"), m_menuWorkspace);
            workspaces->setTearOffEnabled(m_allowTearOffMenus);
            workspaces->setIcon(wsIcon);
            qsizetype workspacesCount = workspacesList.size();
            for (int i = 0; i < workspacesCount; i++) {
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
        }
        else {
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
        QMdiSubWindow* active = mdi_area->activeSubWindow();
        for (int i = 0; i < window_list.size(); ++i) {
            QString title = window_list.at(i)->windowTitle();
            if (title.contains("[*]")) { // modification mark placeholder
                qsizetype idx = title.lastIndexOf("[*]");
                if (window_list.at(i)->isWindowModified()) {
                    title.replace(idx, 3, "*");
                }
                else {
                    title.remove(idx, 3);
                }
            }
            // QAction *id = m_menuWorkspace->addAction(title, m_appWin, SLOT(slotWindowsMenuActivated(bool),
            QAction* id = m_menuWorkspace->addAction(title, m_appWin, &QC_ApplicationWindow::slotWindowsMenuActivated);
            id->setCheckable(true);
            id->setData(i);
            id->setChecked(window_list.at(i) == active);
        }
    }
}

QAction* LC_MenuFactory::urlActionTR(const QString& title, const char* url) {
    auto* result = new QAction(title, m_appWin);
    connect(result, &QAction::triggered, m_appWin, [=]() {
        QDesktopServices::openUrl(QUrl(url));
    });
    return result;
}

QMenu* LC_MenuFactory::subMenu(QMenu* parent, const QString& title, const QString& name, const char* icon,
                               const std::vector<QString>& actionNames, bool supportTearOff) const {
    QMenu* result = doCreateSubMenu(parent, title, name, icon, supportTearOff);
    addActions(result, actionNames);
    return result;
}

QMenu* LC_MenuFactory::subMenuWithActions(QMenu* parent, const QString& title, const QString& name, const char* icon,
                                          const QList<QAction*>& actions) const {
    QMenu* sub_menu = doCreateSubMenu(parent, title, name, icon);
    sub_menu->addActions(actions);
    return sub_menu;
}

QMenu* LC_MenuFactory::doCreateSubMenu(QMenu* parent, const QString& title, const QString& name,
                                       const char* icon, bool supportTearOff) const {
    auto sub_menu = parent->addMenu(title);
    if (icon != nullptr) {
        sub_menu->setIcon(QIcon(icon));
    }
    if (supportTearOff) {
        sub_menu->setTearOffEnabled(m_allowTearOffMenus);
    }
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    const QString& objectName = nameCleared.toLower() + "_menu";
    sub_menu->setObjectName(objectName);
    return sub_menu;
}

QMenu* LC_MenuFactory::menu(const QString& title, const QString& name, QMenuBar* parent) const {
    auto result = new QMenu(title, parent);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    result->setObjectName(nameCleared.toLower() + "_menu");
    result->setTearOffEnabled(m_allowTearOffMenus);
    return result;
}

QMenu* LC_MenuFactory::menu(const QString& title, const QString& name, QMenuBar* parent,
                            const std::vector<QString>& actionNames) const {
    QMenu* result = menu(title, name, parent);
    addActions(result, actionNames);
    return result;
}

void LC_MenuFactory::addAction(QMenu* menu, const char* actionName) const {
    QAction* action = m_actionGroupManager->getActionByName(actionName);
    if (action != nullptr) {
        menu->addAction(action);
    }
}

void LC_MenuFactory::addActions(QMenu* result, const std::vector<QString>& actionNames) const {
    for (const QString& actionName : actionNames) {
        if (actionName.isEmpty()) {
            result->addSeparator();
        }
        else {
            QAction* action = m_actionGroupManager->getActionByName(actionName);
            if (action != nullptr) {
                result->addAction(action);
            }
        }
    }
}

QMenu* LC_MenuFactory::createMainWindowPopupMenu() const {
    auto* result = new QMenu(tr("Context"));
    result->setAttribute(Qt::WA_DeleteOnClose);

    auto* tmpToolbarsMenu = new QMenu(tr("Toolbars"), result);
    tmpToolbarsMenu->addActions(m_menuToolbars->actions());
    result->addMenu(tmpToolbarsMenu);

    auto* tmpWidgetsMenu = new QMenu(tr("Widgets"), result);
    tmpWidgetsMenu->addActions(m_menuDockWidgets->actions());
    result->addMenu(tmpWidgetsMenu);

    result->addSeparator();

    bool needSeparator = false;
    if (m_menuCADDockWidgets != nullptr) {
        auto* tmpCADWidgetsMenu = new QMenu(tr("CAD Widgets"), result);
        tmpCADWidgetsMenu->addActions(m_menuCADDockWidgets->actions());
        result->addMenu(tmpCADWidgetsMenu);
        needSeparator = true;
    }
    if (m_menuCADToolbars != nullptr) {
        auto* tmpCADToolbarsMenu = new QMenu(tr("CAD Toolbars"), result);
        tmpCADToolbarsMenu->addActions(m_menuCADToolbars->actions());
        result->addMenu(tmpCADToolbarsMenu);
        needSeparator = true;
    }
    if (needSeparator) {
        result->addSeparator();
    }

    QAction* viewStatusBarAction = m_actionGroupManager->getActionByName("ViewStatusBar");
    if (viewStatusBarAction != nullptr) {
        result->addAction(viewStatusBarAction);
    }
    return result;
}

void LC_MenuFactory::createGVMenuSelect(QMenu* ctxMenu, RS_Entity* contextEntity,const RS_Vector &contextPosition,
                                        LC_ActionContext* actionContext, int selectionCount) {
    auto selectGroup = m_actionGroupManager->getActionGroup("select");
    auto selectMenu = ctxMenu->addMenu(selectGroup->getIcon(), tr("Select"));

    if (contextEntity == nullptr) {
        addActions(selectMenu, {
                       "SelectSingle",
                       "SelectContour",
                       "SelectIntersected",
                       "DeselectIntersected",
                       "SelectLayer",
                   });
    }
    else {
        addProxyActions(selectMenu, contextEntity, contextPosition, actionContext, {
                            "SelectSingle",
                            "SelectContour",
                            "SelectIntersected",
                            "DeselectIntersected",
                            "SelectLayer",
                        });
    }

    addAction(selectMenu, "SelectPoints");
    addActions(selectMenu, {
                   "SelectWindow"
                   "DeselectWindow"
                   "SelectAll"
               });

    if (selectionCount > 0) {
        addActions(selectMenu, {
                       "DeselectAll"
                   });
    }

    addActions(selectMenu, {
                   "SelectInvert"
               });
}

void LC_MenuFactory::createGVMenuRecent(QG_GraphicView* graphicView, QMenu* ctxMenu, LC_ActionContext* actionContext,
                                        RS_Entity* contextEntity, const RS_Vector &contextPosition, bool hasEntity) {
    auto recentActions = graphicView->getRecentActions();
    if (!recentActions.empty()) {
        if (hasEntity) {
            addActionProxy(ctxMenu, recentActions.first(), contextEntity, contextPosition, actionContext);
        }
        else {
            ctxMenu->QWidget::addAction(recentActions.first());
        }
        auto recentAction = ctxMenu->addMenu(tr("Recent"));
        if (hasEntity) {
            for (QAction* action : recentActions) {
                addActionProxy(recentAction, action, contextEntity, contextPosition, actionContext);
            }
        }
        else {
            recentAction->addActions(recentActions);
        }
        ctxMenu->addSeparator();
    }
}

QMenu* LC_MenuFactory::createGraphicViewPopupMenu(QG_GraphicView* graphicView,
                                                  RS_Entity* contextEntity, const RS_Vector& contextPosition,
                                                  QStringList& actionNames, bool mayInvokeDefaultMenu) {
    QMenu* contextMenu{nullptr};

    if (actionNames.isEmpty() && mayInvokeDefaultMenu) {
        contextMenu = createGraphicViewDefaultPopupMenu(graphicView, contextEntity, contextPosition);
    }
    else {
        contextMenu = createGraphicViewCustomPopupMenu(graphicView, contextEntity, contextPosition, actionNames);
    }

    if (contextEntity != nullptr && contextMenu != nullptr && !contextMenu->isEmpty()) {
        const bool clearEntitySelection = !contextEntity->isSelected();
        if (clearEntitySelection) {
            contextEntity->setSelected(true);
            graphicView->redraw(RS2::RedrawDrawing);
        }

        connect(contextMenu, &QMenu::aboutToHide, this, [graphicView, contextEntity, clearEntitySelection]() {
            // LC_ERR << "MENU_CLOSED";
            if (clearEntitySelection) {
                contextEntity->setSelected(false);
                graphicView->redraw();
            }
        });
    }

    return contextMenu;
}


QMenu* LC_MenuFactory::createGraphicViewCustomPopupMenu(QG_GraphicView* graphicView,
    RS_Entity* contextEntity, const  RS_Vector& contextPosition, QStringList& actionNames) {
    if (actionNames.isEmpty()) {
        return nullptr;
    }
    auto* ctxMenu = new QMenu(graphicView);
    ctxMenu->setAttribute(Qt::WA_DeleteOnClose);
    auto actionContext = graphicView->getActionContext();

    if (contextEntity == nullptr) {
        for (const auto& actionName: actionNames) {
            QAction* a = m_actionGroupManager->getActionByName(actionName);
            if (a != nullptr) {
                ctxMenu->addAction(a);
            }
        }
    }
    else {
        for (const auto &actionName: actionNames) {
            addActionProxy(ctxMenu, actionName, contextEntity, contextPosition, actionContext);
        }
    }

    return ctxMenu;
}

QMenu* LC_MenuFactory::createGraphicViewDefaultPopupMenu(QG_GraphicView* graphicView,
                                                         RS_Entity* contextEntity, const RS_Vector &contextPosition) {
    auto actionContext = graphicView->getActionContext();

    auto* ctxMenu = new QMenu(graphicView);
    ctxMenu->setAttribute(Qt::WA_DeleteOnClose);

    int selectionCount = actionContext->getSelectedEntitiesCount();

    bool hasEntity = contextEntity != nullptr;
    bool hasSelection = selectionCount > 0;

    createGVMenuRecent(graphicView, ctxMenu, actionContext, contextEntity, contextPosition, hasEntity);

    if (hasSelection) {
        addAction(ctxMenu, "EditKillAllActions");
    }

    createGVMenuSelect(ctxMenu, contextEntity, contextPosition, actionContext, selectionCount);
    createGVMenuEdit(ctxMenu, actionContext, contextEntity, contextPosition);

    if (hasEntity) {
        createGVMenuEntitySpecific(ctxMenu, graphicView, contextEntity, contextPosition);
    }
    else {
        if (hasSelection) {
            [[maybe_unused]]auto m_sel = subMenu(ctxMenu, tr("Modify"), "modify", ":/icons/move_copy.lci", {
                                     "ModifyAttributes",
                                     "ModifyAlign",
                                     "ModifyAlignRef",
                                     "ModifyMirror",
                                     "ModifyMove",
                                     "ModifyDuplicate",
                                     "ModifyMoveRotate",
                                     "ModifyOffset",
                                     "ModifyRotate",
                                     "ModifyRotate2",
                                     "ModifyScale",
                                     "ModifyStretch",
                                     "BlocksExplode",
                                     "ModifyDelete"
                                 }, false);

            [[maybe_unused]]auto m_other = subMenu(ctxMenu, tr("Modify More"), "modify_o", ":/icons/fillet.lci", {
                                       "ModifyRevertDirection",
                                       "ModifyEntity",
                                       "ModifyTrim",
                                       "ModifyTrim2",
                                       "ModifyTrimAmount",
                                       "ModifyCut",
                                       "ModifyBevel",
                                       "ModifyRound",
                                       "ModifyExplodeText",
                                       "ModifyBreakDivide",
                                       "ModifyLineGap",
                                       "ModifyLineJoin",
                                   }, false);
        }

        auto addMenu = ctxMenu->addMenu(QIcon(":/icons/line_2p.lci"), tr("Draw"));

        auto lineGroup = m_actionGroupManager->getActionGroup("line");
        auto lineMenu = addMenu->addMenu(lineGroup->getIcon(), tr("Line"));
        lineMenu->addActions(lineGroup->actions());

        auto polylineGroup = m_actionGroupManager->getActionGroup("polyline");
        auto polylineMenu = addMenu->addMenu(polylineGroup->getIcon(), tr("Polyline"));
        polylineMenu->addActions(polylineGroup->actions());

        auto pointGroup = m_actionGroupManager->getActionGroup("point");
        auto pointMenu = addMenu->addMenu(pointGroup->getIcon(), tr("Point"));
        pointMenu->addActions(pointGroup->actions());

        auto circleGroup = m_actionGroupManager->getActionGroup("circle");
        auto circleMenu = addMenu->addMenu(circleGroup->getIcon(), tr("Circle"));
        circleMenu->addActions(circleGroup->actions());

        auto arcGroup = m_actionGroupManager->getActionGroup("curve");
        auto arcMenu = addMenu->addMenu(arcGroup->getIcon(), tr("Arc"));
        arcMenu->addActions(arcGroup->actions());

        auto shapeGroup = m_actionGroupManager->getActionGroup("shape");
        auto shapeMenu = addMenu->addMenu(shapeGroup->getIcon(), tr("Polygon"));
        shapeMenu->addActions(shapeGroup->actions());

        auto splineMenu = addMenu->addMenu(QIcon(":/icons/polylines_polyline.lci"), tr("Polyline/Spline"));
        addActions(splineMenu, {
                       "DrawPolyline",
                       "DrawSpline",
                       "DrawSplinePoints",
                       "DrawParabola4Points",
                       "DrawParabolaFD"
                   });

        auto ellipseGroup = m_actionGroupManager->getActionGroup("ellipse");
        auto ellipseMenu = addMenu->addMenu(ellipseGroup->getIcon(), tr("Ellipse"));
        ellipseMenu->addActions(ellipseGroup->actions());

        auto otherGroup = m_actionGroupManager->getActionGroup("other");
        auto otherMenu = addMenu->addMenu(ellipseGroup->getIcon(), tr("Other"));
        otherMenu->addActions(otherGroup->actions());

        if (!hasSelection) {
            subMenu(ctxMenu, tr("Modify"), "modify", ":/icons/attributes.lci", {
                        "ModifyAttributes",
                        "ModifyDelete",
                        "ModifyMove",
                        "ModifyRevertDirection",
                        "ModifyRotate",
                        "ModifyScale",
                        "ModifyMirror",
                        "ModifyMoveRotate",
                        "ModifyRotate2",
                        "ModifyEntity",
                        "ModifyTrim",
                        "ModifyTrim2",
                        "ModifyTrimAmount",
                        "ModifyOffset",
                        "ModifyCut",
                        "ModifyStretch",
                        "ModifyBevel",
                        "ModifyRound",
                        "ModifyExplodeText",
                        "BlocksExplode",
                        "ModifyBreakDivide",
                        "ModifyLineGap",
                        "ModifyLineJoin",
                        "ModifyDuplicate"
                    }, false);
        }

        auto dimsGroup = m_actionGroupManager->getActionGroup("dimension");
        auto dimsMenu = ctxMenu->addMenu(dimsGroup->getIcon(), tr("Add Dimensions"));
        dimsMenu->addActions(dimsGroup->actions());

        subMenu(ctxMenu, tr("Align"), "align",
                ":/icons/align_one.lci", {
                    "ModifyAlign",
                    "ModifyAlignOne",
                    "ModifyAlignRef"
                }, false);

        subMenu(ctxMenu, tr("Draw Order"), "order", ":/icons/order.lci", {
                    "OrderBottom",
                    "OrderLower",
                    "",
                    "OrderRaise",
                    "OrderTop"
                }, false);

        subMenu(ctxMenu, tr("Layers"), "layers", ":/icons/layer_list.lci", {
                    "LayersDefreezeAll"
                }, false);

        auto infoGroup = m_actionGroupManager->getActionGroup("info");
        auto infoMenu = ctxMenu->addMenu(infoGroup->getIcon(), tr("Info"));
        infoMenu->addActions(infoGroup->actions());
        addActions(infoMenu, {"EntityDescriptionInfo"});
    }

    ctxMenu->addSeparator();
    createGVMenuView(ctxMenu);
    createGVMenuFiles(ctxMenu);
    createGVMenuOptions(ctxMenu);

    return ctxMenu;
}

void LC_MenuFactory::createGVMenuEdit(QMenu* ctxMenu, LC_ActionContext* actionContext,
                                     RS_Entity* contextEntity, const RS_Vector &contextPosition) {
    auto edit = ctxMenu->addMenu(tr("Edit"));
    bool undoAvailable{false}, redoAvailable{false};

    auto container = actionContext->getEntityContainer();
    auto document = container->getDocument();

    if (document != nullptr) {
        document->collectUndoState(undoAvailable, redoAvailable);
    }
    if (undoAvailable) {
        addAction(edit, "EditUndo");
    }
    if (redoAvailable) {
        addAction(edit, "EditRedo");
    }
    if (redoAvailable || undoAvailable) {
        edit->addSeparator();
    }
    addActions(edit, {
                   "EditCopy",
                   "EditCopyQuick",
                   "",
                   "EditPaste",
                   "EditPasteTransform",
                   "PasteToPoints",
                   "",
                   "EditCut",
                   "EditCutQuick"
               });

    if (contextEntity == nullptr) {
        addActions(edit, {
                       "",
                       "PenPick",
                       "PenPickResolved",
                       "PenApply",
                       "PenCopy"
                   });
    }
    else {
        addProxyActions(edit, contextEntity, contextPosition, actionContext, {
                            "",
                            "PenPick",
                            "PenPickResolved",
                            "PenApply",
                            "PenCopy"
                        });
    }
}

void LC_MenuFactory::createGVMenuOptions(QMenu* ctxMenu) {
    subMenu(ctxMenu, tr("Options"), "sub_options", ":/icons/settings.lci", {
                "OptionsDrawing",
                "OptionsGeneral",
                "WidgetOptions",
                "ShortcutsOptions"
            }, false);
}

void LC_MenuFactory::createGVMenuFiles(QMenu* ctxMenu) {
    auto menuFile = subMenu(ctxMenu, tr("&File"), "file", ":/icons/save.lci", {
                                "FileNew",
                                "FileNewTemplate",
                                "FileOpen",
                                "",
                                "FileSave",
                                "FileSaveAs",
                                "FileSaveAll",
                                ""
                            }, false);

    // m_menuRecentFiles = new QMenu(tr("Recent Files"), m_menuFile);

    // menuFile->addMenu(m_menuRecentFiles);

    subMenu(menuFile, tr("Import"), "import", ":/icons/import.lci", {
                "DrawImage",
                "BlocksImport"
            }, false);

    subMenu(menuFile, tr("Export"), "export", ":/icons/export.lci", {
                "FileExportMakerCam",
                "FilePrintPDF",
                "FileExport"
            }, false);

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
}

void LC_MenuFactory::createGVMenuView(QMenu* ctxMenu) {
    auto viewMenu = subMenu(ctxMenu, tr("&View"), "sub_view", ":/icons/zoom_in.lci", {
                                "Fullscreen",
                                /* "ViewStatusBar",*/
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
                            }, false);

    [[maybe_unused]]auto viewsMenu = subMenu(viewMenu, tr("&Named Views"), "view_restore", ":/icons/nview_visible.lci",
                             {
                                 "ZoomViewRestore1",
                                 "ZoomViewRestore2",
                                 "ZoomViewRestore3",
                                 "ZoomViewRestore4",
                                 "ZoomViewRestore5"
                             }, false);

    QList<QDockWidget*> dockwidgetsList = m_appWin->findChildren<QDockWidget*>();

    QAction* namedViewsToggleViewAction = nullptr;
    QAction* ucsToggleViewAction = nullptr;

    findViewAndUCSToggleActions(dockwidgetsList, namedViewsToggleViewAction, ucsToggleViewAction);

    viewMenu->QWidget::addAction(namedViewsToggleViewAction);

    addActions(viewMenu, {
                   "",
                   "UCSCreate",
                   "UCSSetWCS",
                   "UCSSetByDimOrdinate"
               });

    viewMenu->QWidget::addAction(ucsToggleViewAction);
}


void LC_MenuFactory::createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity) {
    QString actionName = tr("Edit Properties");
    createGVEditPropertiesAction(menu, graphicView, entity, actionName);
}

void LC_MenuFactory::createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity, const QString &actionText) {
    QAction* propertiesAction = menu->QWidget::addAction(QIcon(":/icons/properties.lci"), actionText);
    connect(propertiesAction, &QAction::triggered, this, [entity, graphicView]() {
            graphicView->launchEditProperty(entity);
    });
}

void LC_MenuFactory::createGVMenuModifyGeneral(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity, const RS_Vector& pos, LC_ActionContext* actionContext) {
    auto modifyGenericMenu = addProxyActionsSubMenu(contextMenu,
                                                    tr("Modify Generic"),
                                                    ":/icons/move_copy.lci",
                                                    entity, pos, actionContext, {

                                                        "ModifyMove",
                                                        "ModifyDuplicate",
                                                        "ModifyRotate",
                                                        "ModifyMirror",
                                                        "ModifyScale",
                                                        "ModifyStretch",
                                                        "ModifyMoveRotate",
                                                        "ModifyRotate2",
                                                        "",
                                                        "ModifyDelete",
                                                        ""
                                                    });

    createGVEditPropertiesAction(modifyGenericMenu, graphicView, entity);
}

void LC_MenuFactory::createGVMenuEntitySpecific(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity,
                                                const RS_Vector& pos) {


    auto entityType = entity->rtti();
 /*   auto resolvedEntity = entity;
    if (entityType == RS2::EntityPolyline) { // fixme - pass it to actions to eliminate resolving there??
        auto polyline = static_cast<RS_Polyline*>(entity);
        resolvedEntity = polyline->getNearestEntity(pos);
    }
*/

    LC_ActionContext* actionContext = graphicView->getActionContext();
    // int selectionCount = actionContext->getSelectedEntitiesCount();
    /*if (selectionCount == 1)*/
    {

        switch (entityType) {
            case RS2::EntityLine: {
                addProxyActionsSubMenu(contextMenu,
                                       tr("Modify Line"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSliceDivideLine",
                                           "ModifyCut",
                                           "ModifyBreakDivide",
                                           "ModifyTrimAmount",
                                           "ModifyLineJoin",
                                           "ModifyTrim",
                                           "ModifyTrim2",
                                           "ModifyLineGap",
                                           "ModifyOffset",
                                           "ModifyRevertDirection",
                                           "",
                                           "ModifyRound",
                                           "ModifyBevel"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineParallelThrough",
                                           "DrawLineOrthogonalRel",
                                           "DrawLineOrthogonal",
                                           "DrawLineParallel",
                                           "DrawLineRel",
                                           "DrawLineRelAngle",
                                           "DrawLineAngleRel",
                                           "DrawLineOrthTan",
                                           "DrawLineBisector",
                                           "DrawLineFree",
                                           "DrawLineMiddle",
                                           "DrawLineFromPointToLine"
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Circle"), ":/icons/circle_center_point.lci",
                                       entity, pos, actionContext, {
                                           "DrawCircleTan1_2P",
                                           "DrawCircleTan2",
                                           "DrawCircleTan2_1P",
                                           "DrawCircleTan3"
                                           "DrawCircleInscribe",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawArcTangential",
                                           "DrawEllipseInscribe",
                                           "DrawBoundingBox",
                                           "PolylineSegment",
                                       });

                auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci",
                                                entity, pos, actionContext, {
                                                    "DimAligned",
                                                    "DimLinear",
                                                    "DimLinearHor",
                                                    "DimLinearVer",
                                                    "DimAngular",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntityCircle: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Circle"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSliceDivideCircle",
                                           "ModifyBreakDivide",
                                           "ModifyCut",
                                           "ModifyTrim",
                                           "ModifyTrim2",
                                           "ModifyOffset",
                                           "",
                                           "ModifyRound",
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Circle"), ":/icons/circle_center_point.lci",
                                       entity, pos, actionContext, {
                                           "DrawCircleTan1_2P",
                                           "DrawCircleTan2",
                                           "DrawCircleTan2_1P",
                                           "DrawCircleTan3",
                                           "DrawCircleParallel",
                                           "DrawLineParallelThrough",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineOrthTan",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineRelAngle",
                                           "DrawLineOrthogonal",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawCross",
                                           "DrawBoundingBox"
                                       });

                auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci", entity,
                                                pos,
                                                actionContext, {
                                                    "DimRadial",
                                                    "DimDiametric",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntityArc: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Arc"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSliceDivideCircle",
                                           "ModifyBreakDivide",
                                           "ModifyCut",
                                           "ModifyTrimAmount",
                                           "ModifyTrim",
                                           "ModifyTrim2",
                                           "ModifyOffset",
                                           "ModifyRevertDirection",
                                           "",
                                           "ModifyRound",
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Circle"), ":/icons/circle_center_point.lci",
                                       entity, pos, actionContext, {
                                           "DrawCircleByArc",
                                           "DrawCircleTan1_2P",
                                           "DrawCircleTan2",
                                           "DrawCircleTan2_1P",
                                           "DrawCircleTan3",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineOrthTan",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineOrthogonal",
                                           "DrawLineRelAngle"
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawArcTangential",
                                           "DrawCross",
                                           "DrawCircleParallel",
                                           "DrawLineParallelThrough",
                                           "DrawBoundingBox",
                                           "PolylineSegment",
                                       });

                auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci",
                                                entity, pos, actionContext, {
                                                    "DimRadial",
                                                    "DimDiametric",
                                                    "DimArc",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntityPolyline: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Polyline"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "PolylineAdd",
                                           "PolylineAppend",
                                           "PolylineDel",
                                           "PolylineDelBetween",
                                           "PolylineTrim",
                                           "PolylineSegmentType",
                                           "PolylineArcToLines",
                                           "PolylineSegment",
                                           "PolylineEquidistant",
                                           "BlocksExplode",
                                           "ModifyRevertDirection",
                                           "ModifyOffset"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineBisector",
                                           "DrawLineOrthTan",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineOrthogonal",
                                           "DrawLineRelAngle"
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "PolylineEquidistant",
                                           "DrawLineParallelThrough",
                                           "DrawSplineFromPolyline",
                                           "DrawBoundingBox"
                                       });

                auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci",
                                                entity, pos, actionContext, {
                                                    "DimAligned",
                                                    "DimLinear",
                                                    "DimLinearHor",
                                                    "DimLinearVer",
                                                    "DimAngular",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntitySpline: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Spline"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSplinePointsAdd",
                                           "DrawSplinePointsAppend",
                                           "DrawSplinePointsRemove",
                                           "DrawSplineExplode",
                                           "DrawSplinePointsDelTwo",
                                           "BlocksExplode",
                                           "ModifyRevertDirection"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntitySplinePoints: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Spline Points"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSplinePointsAdd",
                                           "DrawSplinePointsAppend",
                                           "DrawSplinePointsRemove",
                                           "DrawSplineExplode",
                                           "DrawSplinePointsDelTwo",
                                           "ModifyCut",
                                           "ModifyRevertDirection"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineTangent1",
                                           "DrawBoundingBox"
                                       });
                break;
            }
            case RS2::EntityText: {
                // fixme - sand - add additional actions (align, style etc.)?
                addProxyActionsSubMenu(contextMenu, tr("Modify Text"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "ModifyExplodeText",
                                           "BlocksExplode",
                                           "DrawBoundingBox"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityMText: {
                // fixme - sand - add additional actions (align, style etc.)?
                addProxyActionsSubMenu(contextMenu, tr("Modify MText"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "ModifyExplodeText",
                                           "BlocksExplode",
                                           "DrawBoundingBox"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityEllipse: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "ModifyCut"
                                    "",
                                    "ModifyRound",
                                });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineOrthTan",
                                           "DrawLineOrthogonal",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineRelAngle"
                                       });

                auto m = addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                                entity, pos, actionContext, {
                                                    "DrawCross",
                                                    "DrawBoundingBox"
                                                });

                auto ellipse = static_cast<RS_Ellipse*>(entity);
                if (ellipse->isEllipticArc()) {
                    addAction(m, "DrawArcTangential");
                    addAction(m, "ModifyRevertDirection");
                }
                break;
            }
            case RS2::EntityParabola: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DrawLineOrthTan",
                                    "DrawLineTangent1",
                                    "DrawLineTangent2",
                                    "ModifyCut",
                                    "DrawBoundingBox"
                                });
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityDimAligned: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimBaseline",
                                    "DimContinue",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimLinear: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimBaseline",
                                    "DimContinue",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimAngular: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimBaseline",
                                    "DimContinue",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimRadial: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimDiametric: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimArc: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimOrdinate: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimOrdinateForBase",
                                    "DimOrdinateReBase",
                                    "UCSSetByDimOrdinate",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimLeader: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate"
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityInsert: {
                auto insert = static_cast<RS_Insert*>(entity);
                // For an insert, show the menu entry to edit the block instead
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                auto editActionText = QString{"%1: %2"}.arg(tr("Edit Block")).arg(
                    insert->getName().left(g_MaxBlockNameLength));

                createGVEditPropertiesAction(contextMenu, graphicView, entity, editActionText);


                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "BlocksExplode"
                                });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                break;
            }
            case RS2::EntityPoint: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "SelectPoints",
                                    "PasteToPoints",
                                    "DrawPointsMiddle",
                                    "DrawLinePoints",
                                    "DrawPointLattice"
                                });
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityImage: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityHatch: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityHyperbola: {
                // addAction(contextMenu, "DrawArcTangential");
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityConstructionLine: {
                // addAction(contextMenu, "DrawArcTangential");
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntitySolid: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityTolerance: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            default:
                break;
        }
    }

    // ":/icons/halign_left.lci"
    addProxyActionsSubMenu(contextMenu, tr("Align"),
                           ":/icons/align_one.lci",
                           entity, pos, actionContext, {
                               "ModifyAlign",
                               "ModifyAlignOne",
                               "ModifyAlignRef"
                           });

    addProxyActionsSubMenu(contextMenu, tr("Order"), ":/icons/order.lci",
                           entity, pos, actionContext, {
                               "OrderBottom",
                               "OrderLower",
                               "",
                               "OrderTop",
                               "OrderRaise"
                           });

    auto layerMenu = contextMenu->addMenu(QIcon(":/icons/layer_list.lci"), tr("Layers"));

    RS_Graphic* graphic = graphicView->getGraphic(false);
    auto entityLayer = entity->getLayer();
    if (graphic != nullptr && entityLayer != nullptr) {
        if (graphic->getActiveLayer() != entityLayer) {
            addProxyActions(layerMenu, entity, pos, actionContext, {
                                "EntityLayerActivate"
                            });
        }
    }
    addProxyActions(layerMenu, entity, pos, actionContext, {
                        "EntityLayerView",
                        "EntityLayerLock",
                        "EntityLayerPrint",
                        "EntityLayerConstruction",
                        "EntityLayerHideOthers",
                        "LayersDefreezeAll"
                    });

    addProxyActionsSubMenu(contextMenu, tr("Info"), ":/icons/measure.lci", entity, pos, actionContext, {
                               "EntityInfo",
                               "InfoPoint",
                               "InfoDist2",
                               "InfoDist",
                               "InfoDist3",
                               "InfoAngle",
                               "InfoAngle3Points",
                               "InfoTotalLength",
                               "InfoArea",
                               "PickCoordinates",
                               "",
                               "EntityDescriptionInfo"
                           });
}

QMenu* LC_MenuFactory::addProxyActionsSubMenu(QMenu* menu, const QString &subMenuName, const char* subMenuIconName,
                                              RS_Entity* entity, const RS_Vector& pos, LC_ActionContext* actionContext,
                                              const std::vector<QString>& actionNames) const {
    auto dimMenu = menu->addMenu(QIcon(subMenuIconName), subMenuName);
    addProxyActions(dimMenu, entity, pos, actionContext, actionNames);
    return dimMenu;
}

void LC_MenuFactory::addProxyActions(QMenu* menu, RS_Entity* entity, const RS_Vector& pos,
                                     LC_ActionContext* actionContext,
                                     const std::vector<QString>& actionNames) const {
    for (const QString& actionName : actionNames) {
        if (actionName.isEmpty()) {
            menu->addSeparator();
        }
        else {
            addActionProxy(menu, actionName, entity, pos, actionContext);
        }
    }
}

void LC_MenuFactory::addActionProxy(QMenu* menu, QAction* srcAction, RS_Entity* entity, const RS_Vector& pos,
                                    LC_ActionContext* actionContext) const {
    if (srcAction != nullptr && srcAction->isEnabled()) {
        auto actionProxy = new QAction(srcAction->icon(), srcAction->iconText());
        actionProxy->setToolTip(srcAction->toolTip());
        connect(actionProxy, &QAction::triggered, this,
                [this, entity, pos, srcAction, actionContext]([[maybe_unused]]bool checked) {
                    // LC_ERR << "MENU_TRIGGERED - 1";
                    actionContext->saveContextMenuActionContext(entity, pos, false);
                    srcAction->trigger();
                });
        menu->QWidget::addAction(actionProxy);
    }
}

void LC_MenuFactory::addActionProxy(QMenu* menu, const QString& actionName, RS_Entity* entity, const RS_Vector& pos,
                                    LC_ActionContext* actionContext) const {
    auto srcAction = m_actionGroupManager->getActionByName(actionName);
    if (srcAction != nullptr) {
        addActionProxy(menu, srcAction, entity, pos, actionContext);
    }
}
