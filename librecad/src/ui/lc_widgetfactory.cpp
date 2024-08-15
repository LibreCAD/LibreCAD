/*
**********************************************************************************
**
** This file was created for the LibreCAD project (librecad.org), a 2D CAD program.
**
** Copyright (C) 2015 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
**********************************************************************************
*/

#include <QMenu>
#include <QFile>
#include <QMenuBar>
#include <QActionGroup>
#include <QDesktopServices>

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "lc_dockwidget.h"
#include "lc_layertreewidget.h"
#include "lc_widgetfactory.h"

#include "qc_applicationwindow.h"

#include "qg_actionhandler.h"
#include "qg_blockwidget.h"
#include "qg_commandwidget.h"
#include "qg_layerwidget.h"
#include "qg_librarywidget.h"
#include "qg_pentoolbar.h"
#include "qg_snaptoolbar.h"

#include "rs_debug.h"
#include "rs_settings.h"

namespace {
    // only enable the penpallet by settings
    bool usePenPallet() {
        auto guard= RS_SETTINGS->beginGroupGuard("/CustomToolbars");
        return RS_SETTINGS->readNumEntry("/UsePenPallet", 1) == 1;
    }
} // namespace

LC_WidgetFactory::LC_WidgetFactory(QC_ApplicationWindow* main_win,
                                   QMap<QString, QAction*>& action_map,
                                   LC_ActionGroupManager* agm)
    : QObject(nullptr)
    , main_window(main_win)
    , a_map(action_map)
    , ag_manager(agm){

    fillActionsList(file_actions,  {
        "FileNew",
        "FileNewTemplate",
        "FileOpen",
        "FileSave",
        "FileSaveAs",
        "FileSaveAll"
    });

    fillActionsList(line_actions, {
        "DrawLine",
        "DrawLineAngle",
        "DrawLineHorizontal",
        "DrawLineVertical",
        "DrawLineRectangle",
        "DrawLineParallelThrough",
        "DrawLineParallel",
        "DrawLineBisector",
        "DrawLineTangent1",
        "DrawLineTangent2",
        "DrawLineOrthTan",
        "DrawLineOrthogonal",
        "DrawLineRelAngle",
        "DrawLinePolygonCenCor",
        "DrawLinePolygonCenTan",   //20161226 added by txmy
        "DrawLinePolygonCorCor",
        "DrawStar",
        "DrawPoint",
        "DrawLinePoints",
        "DrawLineRectangle1Point",
        "DrawLineRectangle2Points",
        "DrawLineRectangle3Points",
        "DrawCross",
        "DrawLineRel",
        "DrawLineRelX",
        "DrawLineRelY",
        "DrawLineAngleRel",
        "DrawLineOrthogonalRel",
        "DrawLineFromPointToLine",
        "DrawSliceDivideLine",
        "DrawSliceDivideCircle"
    });

    fillActionsList(circle_actions, {
        "DrawCircle",
        "DrawCircle2P",
        "DrawCircle2PR",
        "DrawCircle3P",
        "DrawCircleCR",
        "DrawCircleTan2_1P",
        "DrawCircleTan1_2P",
        "DrawCircleTan2",
        "DrawCircleTan3",
        "DrawCircleInscribe",
        "DrawCircleParallel",
        "DrawCircleByArc"
    });

    fillActionsList(curve_actions, {
        "DrawArc",
        "DrawArc3P",
        "DrawArcTangential",
        "DrawParabola4Points",
        "DrawParabolaFD",
        "DrawSpline",
        "DrawSplinePoints",
        "DrawEllipseArcAxis",
        "DrawLineFree"
    });

    fillActionsList(ellipse_actions, {
        "DrawEllipseAxis",
        "DrawEllipseFociPoint",
        "DrawEllipse4Points",
        "DrawEllipseCenter3Points",
        "DrawEllipseInscribe"
    });

    fillActionsList(polyline_actions, {
        "DrawPolyline",
        "PolylineAdd",
        "PolylineAppend",
        "PolylineDel",
        "PolylineDelBetween",
        "PolylineTrim",
        "PolylineEquidistant",
        "PolylineSegment"
    });

    fillActionsList(select_actions, {
        "DeselectAll",
        "SelectAll",
        "SelectSingle",
        "SelectContour",
        "SelectWindow",
        "DeselectWindow",
        "SelectIntersected",
        "DeselectIntersected",
        "SelectLayer",
        "SelectInvert"
    });

    fillActionsList(dimension_actions, {
        "DimAligned",
        "DimLinear",
        "DimLinearHor",
        "DimLinearVer",
        "DimRadial",
        "DimDiametric",
        "DimAngular",
        "DimArc",
        "DimLeader",
        "DrawText",
        "DrawMText",
        "DrawHatch",
        "DrawImage"
    });

    fillActionsList(modify_actions, {
        "ModifyMove",
        "ModifyDuplicate",
        "ModifyRotate",
        "ModifyScale",
        "ModifyMirror",
        "ModifyMoveRotate",
        "ModifyRotate2",
        "ModifyRevertDirection",
        "ModifyTrim",
        "ModifyTrim2",
        "ModifyTrimAmount",
        "ModifyLineJoin",
        "ModifyBreakDivide",
        "ModifyLineGap",
        "ModifyOffset",
        "ModifyBevel",
        "ModifyRound",
        "ModifyCut",
        "ModifyStretch",
        "ModifyEntity",
        "ModifyAttributes",
        "ModifyExplodeText",
        "BlocksExplode",
        "ModifyDelete"
        //            "ModifyDeleteFree",
//            "ModifyDeleteQuick"});
    });

    fillActionsList(order_actions, {
        "OrderTop",
        "OrderBottom",
        "OrderRaise",
        "OrderLower"
    });

    fillActionsList(info_actions, {
        "InfoDist",
        "InfoDist2",
        "InfoDist3",
        "InfoAngle",
        "InfoAngle3Points",
        "InfoTotalLength",
        "InfoArea",
        "EntityInfo"
    });
    
    fillActionsList(layer_actions, {
        "LayersDefreezeAll",
        "LayersFreezeAll",
        "LayersUnlockAll",
        "LayersLockAll",
        "LayersAdd",
        "LayersRemove",
        "LayersEdit",
        "LayersToggleLock",
        "LayersToggleView",
        "LayersTogglePrint",
        "LayersToggleConstruction",
        "LayersExportSelected",
        "LayersExportVisible"
    });

    fillActionsList(block_actions, {
        "BlocksDefreezeAll",
        "BlocksFreezeAll",
        "BlocksToggleView",
        "BlocksAdd",
        "BlocksRemove",
        "BlocksAttributes",
        "BlocksInsert",
        "BlocksEdit",
        "BlocksSave",
        "BlocksCreate",
        "BlocksExplode"
    });

    fillActionsList(pen_actions, {
        "PenSyncFromLayer",
        "PenPick",
        "PenPickResolved",
        "PenApply",
        "PenCopy"
    });

    fillActionsList(actionsToDisableInPrintPreview, {
        "EditCut",
        "EditCutQuick",
        "EditCopy",
        "EditCopyQuick",
        "EditPaste",
        "EditPasteTransform",
        "ViewGrid",
        "ModifyDeleteQuick",
        "EditKillAllActions",
        "ZoomIn",
        "ZoomOut",
        "ZoomAuto",
        "ZoomPrevious",
        "ZoomWindow",
        "ZoomPan",
        "OptionsDrawing"
    });

    actionsToDisableInPrintPreview.append(line_actions);
    actionsToDisableInPrintPreview.append(circle_actions);
    actionsToDisableInPrintPreview.append(curve_actions);
    actionsToDisableInPrintPreview.append(ellipse_actions);
    actionsToDisableInPrintPreview.append(polyline_actions);
    actionsToDisableInPrintPreview.append(select_actions);
    actionsToDisableInPrintPreview.append(dimension_actions);
    actionsToDisableInPrintPreview.append(modify_actions);
    actionsToDisableInPrintPreview.append(order_actions);
    actionsToDisableInPrintPreview.append(info_actions);
    actionsToDisableInPrintPreview.append(block_actions);
    actionsToDisableInPrintPreview.append(pen_actions);
}

void LC_WidgetFactory::createLeftSidebar(int columns, int icon_size){
    auto* line = leftDocWidgetTR("Line", line_actions, columns, icon_size);
    auto* circle = leftDocWidgetTR("Circle", circle_actions, columns, icon_size);
    auto* curve = leftDocWidgetTR("Curve", curve_actions, columns, icon_size);
    auto* ellipse = leftDocWidgetTR("Ellipse", ellipse_actions, columns, icon_size);
    auto* polyline = leftDocWidgetTR("Polyline", polyline_actions, columns, icon_size);
    auto* select = leftDocWidgetTR("Select", select_actions, columns, icon_size);
    auto* dimension = leftDocWidgetTR("Dimension", dimension_actions, columns, icon_size);
    auto* modify = leftDocWidgetTR("Modify", modify_actions, columns, icon_size);
    auto* info = leftDocWidgetTR("Info", info_actions, columns, icon_size);
    auto* order = leftDocWidgetTR("Order", order_actions, columns, icon_size);

    main_window->addDockWidget(Qt::LeftDockWidgetArea, line);
    main_window->tabifyDockWidget(line, polyline);
    line->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, circle);
    main_window->tabifyDockWidget(circle, curve);
    main_window->tabifyDockWidget(curve, ellipse);
    circle->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, dimension);
    main_window->tabifyDockWidget(dimension, info);
    main_window->tabifyDockWidget(info, select);
    dimension->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, modify);
    main_window->tabifyDockWidget(modify, order);
}

void LC_WidgetFactory::createRightSidebar(QG_ActionHandler* action_handler){
    QDockWidget* dock_pen_palette = nullptr;
    if (usePenPallet()) {
        dock_pen_palette = new QDockWidget(main_window);
        dock_pen_palette->setWindowTitle(QC_ApplicationWindow::tr("Pen Palette"));
        dock_pen_palette->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        dock_pen_palette->setObjectName("pen_palette_dockwidget");
        pen_palette = new LC_PenPaletteWidget("PenPalette", dock_pen_palette);
        pen_palette->setFocusPolicy(Qt::NoFocus);
        connect(pen_palette, SIGNAL(escape()), main_window, SLOT(slotFocus()));
//        connect(main_window, SIGNAL(windowsChanged(bool)), pen_palette, SLOT(setEnabled(bool)));
        dock_pen_palette ->setWidget(pen_palette);
    }
    auto* dock_layer = new QDockWidget(main_window);
    dock_layer->setWindowTitle(QC_ApplicationWindow::tr("Layer List"));
    dock_layer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_layer->setObjectName("layer_dockwidget");
    layer_widget = new QG_LayerWidget(action_handler, dock_layer, "Layer");
    layer_widget->setFocusPolicy(Qt::NoFocus);
    connect(layer_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
//    connect(main_window, SIGNAL(windowsChanged(bool)), layer_widget, SLOT(setEnabled(bool)));
    dock_layer->setWidget(layer_widget);

    QDockWidget* dock_layer_tree = nullptr;
    if (usePenPallet()) {
        dock_layer_tree = new QDockWidget(main_window);
        dock_layer_tree->setWindowTitle(QC_ApplicationWindow::tr("Layer Tree"));
        dock_layer_tree->setObjectName("layer_tree_dockwidget");
        dock_layer_tree->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        layer_tree_widget = new LC_LayerTreeWidget(action_handler, dock_layer_tree, "Layer Tree");
        layer_tree_widget->setFocusPolicy(Qt::NoFocus);
        connect(layer_tree_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
//        connect(main_window, SIGNAL(windowsChanged(bool)), layer_tree_widget, SLOT(setEnabled(bool)));
        layer_tree_widget->setVisible(false);
        dock_layer_tree->setWidget(layer_tree_widget);
    }

    QDockWidget* dock_quick_info = nullptr;
    dock_quick_info = new QDockWidget(main_window);
    dock_quick_info->setWindowTitle(QC_ApplicationWindow::tr("Entity Info"));
    dock_quick_info->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_quick_info->setObjectName("quick_entity_info");
    quick_info_widget = new LC_QuickInfoWidget(dock_quick_info, a_map);
    quick_info_widget->setFocusPolicy(Qt::NoFocus);
//    quick_info_widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    connect(quick_info_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
//    connect(main_window, SIGNAL(windowsChanged(bool)), quick_info_widget, SLOT(setEnabled(bool)));
    quick_info_widget->setVisible(false);
    dock_quick_info->setWidget(quick_info_widget);


    auto* dock_block = new QDockWidget(main_window);
    dock_block->setWindowTitle(QC_ApplicationWindow::tr("Block List"));
    dock_block->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_block->setObjectName("block_dockwidget");
    block_widget = new QG_BlockWidget(action_handler, dock_block, "Block");
    block_widget->setFocusPolicy(Qt::NoFocus);
    connect(block_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
//    connect(main_window, SIGNAL(windowsChanged(bool)), block_widget, SLOT(setEnabled(bool)));
    dock_block->setWidget(block_widget);

    auto* dock_library = new QDockWidget(main_window);
    dock_library->setWindowTitle(tr("Library Browser"));
    dock_library->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_library->setObjectName("library_dockwidget");
    library_widget = new QG_LibraryWidget(dock_library, "Library");
    library_widget->setActionHandler(action_handler);
    library_widget->setFocusPolicy(Qt::NoFocus);
    connect(library_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
//    connect(main_window, SIGNAL(windowsChanged(bool)),
//            (QObject*) library_widget->getInsertButton(), SLOT(setEnabled(bool)));
    dock_library->setWidget(library_widget);
    dock_library->resize(240, 400);

    auto* dock_command = new QDockWidget(tr("Command line"), main_window);
    // dock_command->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    dock_command->setObjectName("command_dockwidget");
    command_widget = new QG_CommandWidget(dock_command, "Command");
    command_widget->setActionHandler(action_handler);
    // command_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
//    connect(main_window, SIGNAL(windowsChanged(bool)), command_widget, SLOT(setEnabled(bool)));
    connect(command_widget->leCommand, SIGNAL(escape()), main_window, SLOT(setFocus()));
    dock_command->setWidget(command_widget);

    connect(dock_command, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
            main_window, SLOT(modifyCommandTitleBar(Qt::DockWidgetArea)));

    main_window->setDockOptions(QMainWindow::AnimatedDocks
                                | QMainWindow::AllowTabbedDocks );

    main_window->addDockWidget(Qt::RightDockWidgetArea, dock_library);
    main_window->tabifyDockWidget(dock_library, dock_block);
    main_window->tabifyDockWidget(dock_block, dock_layer);
    main_window->tabifyDockWidget(dock_block, dock_quick_info);
    if (dock_pen_palette != nullptr && dock_layer_tree != nullptr) {
        main_window->tabifyDockWidget(dock_layer, dock_pen_palette);
        main_window->tabifyDockWidget(dock_pen_palette, dock_layer_tree);
    }
    main_window->addDockWidget(Qt::RightDockWidgetArea, dock_command);
    command_widget->getDockingAction()->setText(dock_command->isFloating() ? tr("Dock") : tr("Float"));
}

void LC_WidgetFactory::createStandardToolbars(QG_ActionHandler* action_handler){
    QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *file = createGenericToolbarTR("File", tbPolicy, {});
    file->addActions(file_actions);
    file->addAction(a_map["FilePrint"]);
    file->addAction(a_map["FilePrintPreview"]);

    auto *edit = createGenericToolbarTR("Edit", tbPolicy, {
        "EditKillAllActions", "", "EditUndo", "EditRedo", "", "EditCut", "EditCopy", "EditPaste","EditPasteTransform"
    });

    auto *order = createGenericToolbarTR("Order", tbPolicy, {
        "OrderTop", "OrderBottom", "OrderRaise", "OrderLower"
    });
    order->hide();

    auto *view = createGenericToolbarTR("View", tbPolicy, {
        "ViewGrid", "ViewDraft", "", "ZoomRedraw", "ZoomIn",
        "ZoomOut", "ZoomAuto", "ZoomPrevious", "ZoomWindow", "ZoomPan"
    });

    snap_toolbar = new QG_SnapToolBar(main_window, action_handler, ag_manager);
    snap_toolbar->setWindowTitle(QC_ApplicationWindow::tr("Snap Selection"));
    snap_toolbar->setSizePolicy(tbPolicy);
    snap_toolbar->setObjectName("snap_toolbar" );
    action_handler->set_snap_toolbar(snap_toolbar);

    connect( main_window,  &QC_ApplicationWindow::signalEnableRelativeZeroSnaps, 
             snap_toolbar, &QG_SnapToolBar::slotEnableRelativeZeroSnaps);

    pen_toolbar = new QG_PenToolBar(QC_ApplicationWindow::tr("Pen"), main_window);
    pen_toolbar->setSizePolicy(tbPolicy);
    pen_toolbar->setObjectName("pen_toolbar");
    pen_toolbar->addActions(pen_actions);

    options_toolbar = createGenericToolbarTR("Tool Options", tbPolicy, {});
    
    auto *dockareas = createGenericToolbarTR("Dock Areas", tbPolicy, {
        "LeftDockAreaToggle", "RightDockAreaToggle", "TopDockAreaToggle",
        "BottomDockAreaToggle", "FloatingDockwidgetsToggle"
    });
    
    auto *creators = createGenericToolbarTR("Creators", tbPolicy, {
        "InvokeMenuCreator", "InvokeToolbarCreator"
    });

    addToTop(file);
    addToTop(edit);
    addToTop(view);
    main_window->addToolBarBreak();
    addToTop(pen_toolbar);
    addToTop(options_toolbar);

    addToLeft(order);

    addToBottom(snap_toolbar);
    addToBottom(dockareas);
    addToBottom(creators);
}

void LC_WidgetFactory::createCADToolbars(){
    QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* line = toolbarWithActionsTR("Line", tbPolicy, line_actions);
    auto* circle = toolbarWithActionsTR("Circle", tbPolicy, circle_actions);
    auto* curve = toolbarWithActionsTR("Curve", tbPolicy, curve_actions);
    auto* ellipse = toolbarWithActionsTR("Ellipse", tbPolicy, ellipse_actions);
    auto* polyline = toolbarWithActionsTR("Polyline", tbPolicy, polyline_actions);
    auto* select = toolbarWithActionsTR("Select", tbPolicy, select_actions);
    auto* dimension = toolbarWithActionsTR("Dimension", tbPolicy, dimension_actions);
    auto* modify = toolbarWithActionsTR("Modify", tbPolicy, modify_actions);
    auto* info = toolbarWithActionsTR("Info", tbPolicy, info_actions);

    addToBottom(line);
    addToBottom(circle);
    addToBottom(curve);
    addToBottom(ellipse);
    addToBottom(polyline);
    addToBottom(dimension);
    addToBottom(modify);
    addToBottom(info);
    addToBottom(select);
}

QToolBar *LC_WidgetFactory::createCategoriesToolbar() {
    auto *toolbar = createGenericToolbarTR("Categories",
                                           QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding), {});

    toolButtonTR(toolbar, "Lines", ":/icons/line.svg", line_actions);
    toolButtonTR(toolbar, "Circles", ":/icons/circle.svg", circle_actions);
    toolButtonTR(toolbar, "Freehand", ":/icons/line_freehand.svg", curve_actions);
    toolButtonTR(toolbar, "Ellipses", ":/icons/ellipses.svg", ellipse_actions);
    toolButtonTR(toolbar, "PolyLines", ":/icons/polylines.svg", polyline_actions);
    toolButtonTR(toolbar, "Select", ":/icons/select.svg", select_actions);
    toolButtonTR(toolbar, "Dimensions", ":/icons/dim_horizontal.svg", dimension_actions);
    toolButtonTR(toolbar, "Modify", ":/icons/move_rotate.svg", modify_actions);
    toolButtonTR(toolbar, "Measure", ":/icons/measure.svg", info_actions);
    toolButtonTR(toolbar, "Order", ":/icons/order.svg", order_actions);

    addToLeft(toolbar);
    return toolbar;
}

void LC_WidgetFactory::createMenus(QMenuBar* menu_bar){
    file_menu = menu("&File", menu_bar, {
        "FileNew",
        "FileNewTemplate",
        "FileOpen",
        "",
        "FileSave",
        "FileSaveAs",
        "FileSaveAll",
        ""
    });

    subMenuTR(file_menu, "Import", ":/icons/import.svg", {
        "DrawImage",
        "BlocksImport"
    });

    subMenuTR(file_menu, "Export", ":/icons/export.svg", {
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

    auto settings = menu("&Options", menu_bar, {
        "OptionsGeneral",
        "ShortcutsOptions",
        "WidgetOptions",
        "DeviceOptions",
        "ReloadStyleSheet",
        "",
        "OptionsDrawing",
    });

    auto edit = menu("&Edit", menu_bar, {
        "EditKillAllActions",
        "",
        "EditUndo",
        "EditRedo",
        "",
        "EditCut",
        "EditCopy",
        "EditPaste",
        "EditPasteTransform",
        "",
        "EditCutQuick",
        "EditCopyQuick",
        "ModifyDeleteQuick"
    });

    auto plugins = menuTR("Pl&ugins", menu_bar);

    auto view = menu("&View", menu_bar, {
        "Fullscreen",
        "ViewStatusBar",
        "ViewGrid",
        "ViewDraft",
        "",
        "ZoomRedraw",
        "ZoomIn",
        "ZoomOut",
        "ZoomAuto",
        "ZoomPrevious",
        "ZoomWindow",
        "ZoomPan",
    });

    // fixme - MTesxt& text - remove from dimensions actions...
    auto tools = menuTR("&Tools", menu_bar);
    subMenuWithActionsTR(tools, "&Line", ":/icons/line.svg", line_actions);
    subMenuWithActionsTR(tools, "&Circle", ":/icons/circle.svg", circle_actions);
    subMenuWithActionsTR(tools, "&Curve", ":/icons/line_freehand.svg", curve_actions);
    subMenuWithActionsTR(tools, "&Ellipse", ":/icons/ellipses.svg", ellipse_actions);
    subMenuWithActionsTR(tools, "&Polyline", ":/icons/polylines_polyline.svg", polyline_actions);
    subMenuWithActionsTR(tools, "&Select", ":/icons/select.svg", select_actions);
    subMenuWithActionsTR(tools, "Dime&nsion", ":/icons/dim_horizontal.svg", dimension_actions);
    subMenuWithActionsTR(tools, "&Modify", ":/icons/move_rotate.svg", modify_actions);
    subMenuWithActionsTR(tools, "&Info", ":/icons/measure.svg", info_actions);
    subMenuWithActionsTR(tools, "&Order", ":/icons/order.svg", order_actions);

    windows_menu = menu("&Drawings", menu_bar, {
        "Fullscreen" // temp way to show this menu on OS X
    });

    connect(windows_menu, SIGNAL(aboutToShow()),
            main_window, SLOT(slotWindowsMenuAboutToShow()));

    auto help = menuTR("&Help", menu_bar);

    subMenuWithActionsTR(help, "On&line", nullptr, {
        urlActionTR("&Wiki", "https://dokuwiki.librecad.org/"),
        urlActionTR("User's &Manual", "https://librecad.readthedocs.io/"),
        urlActionTR("&Commands", "https://librecad.readthedocs.io/en/latest/ref/tools.html"),
        urlActionTR("&Style Sheets", "https://librecad.readthedocs.io/en/latest/ref/customize.html#style-sheets"),
        urlActionTR("Wid&gets", "https://librecad.readthedocs.io/en/latest/ref/menuTR.html#widgets"),
        urlActionTR("&Forum", "https://forum.librecad.org/"),
        urlActionTR("Zulip &Chat", "https://librecad.zulipchat.com/"),
        urlActionTR("&Release Information", "https://github.com/LibreCAD/LibreCAD/releases")
    });

    auto help_about = new QAction(QIcon(":/main/librecad.png"), QC_ApplicationWindow::tr("About"), main_window);
    connect(help_about, SIGNAL(triggered()), main_window, SLOT(showAboutWindow()));

    auto license = new QAction(QObject::tr("License"), main_window);
    connect(license, SIGNAL(triggered()), main_window, SLOT(invokeLicenseWindow()));

    help->addSeparator();
    help->addAction(help_about);
    help->addAction(license);
    help->addAction(urlActionTR("&Donate", "https://librecad.org/donate.html"));

    auto widgets = menuTR("Widgets", menu_bar);

    auto *dockareas = subMenuTR(widgets, "Dock Areas", nullptr, {
        "LeftDockAreaToggle",
        "RightDockAreaToggle",
        "TopDockAreaToggle",
        "BottomDockAreaToggle",
        "FloatingDockwidgetsToggle"
    });

    auto* dockwidgets_menu = doCreateSubMenuTR(widgets, "Dock Wid&gets", nullptr);

    dockwidgets_menu->addSeparator();

    QList<QDockWidget*> dockwidgetsList = main_window->findChildren<QDockWidget*>();
    main_window->sortWidgetsByTitle(dockwidgetsList);

    for (QDockWidget* dw: dockwidgetsList){
        if (main_window->dockWidgetArea(dw) == Qt::RightDockWidgetArea)
            dockwidgets_menu->addAction(dw->toggleViewAction());
    }

    addAction(dockwidgets_menu, "RedockWidgets");

    dockwidgets_menu->addSeparator();

    for (QDockWidget* dw : dockwidgetsList){
        if (main_window->dockWidgetArea(dw) == Qt::LeftDockWidgetArea)
            dockwidgets_menu->addAction(dw->toggleViewAction());
    }

    auto* toolbars = doCreateSubMenuTR(widgets, "&Toolbars", nullptr);

    QList<QToolBar*> toolbarsList = main_window->findChildren<QToolBar*>();

    main_window->sortWidgetsByTitle(toolbarsList);

    for (QToolBar* tb: toolbarsList){
        toolbars->addAction(tb->toggleViewAction());
    }

    widgets->addMenu(dockareas);
    widgets->addMenu(dockwidgets_menu);
    widgets->addMenu(toolbars);

    addAction(widgets, "InvokeMenuCreator");
    addAction(widgets, "InvokeToolbarCreator");

    menu_bar->addMenu(file_menu);
    menu_bar->addMenu(settings);
    menu_bar->addMenu(edit);
    menu_bar->addMenu(view);
    menu_bar->addMenu(plugins);
    menu_bar->addMenu(tools);
    menu_bar->addMenu(widgets);
    menu_bar->addMenu(windows_menu);
    menu_bar->addMenu(help);
}

void LC_WidgetFactory::addAction(QMenu* menu, const char* actionName){
    QAction *action = a_map[actionName];
    if (action != nullptr) {
        menu->addAction(action);
    }
}

QAction* LC_WidgetFactory::urlActionTR(const char* title, const char* url ){
    auto* result    = new QAction( QC_ApplicationWindow::tr( title), main_window);
    connect(result, &QAction::triggered, main_window, [=](){
        QDesktopServices::openUrl( QUrl( url));
    });
    return result;
}

QMenu*  LC_WidgetFactory::menu(const char* title, QMenuBar* parent, const std::vector<QString> &actionNames){
  QMenu* result = menuTR(title, parent);
    addActions(result, actionNames);
    return result;
}

void LC_WidgetFactory::addActions(QMenu *result, const std::vector<QString> &actionNames) {
    for (const QString& actionName : actionNames){
        if (actionName.isEmpty()){
            result->addSeparator();
        }
        else{
            QAction* action = a_map[actionName];
            if (action != nullptr){
                result->addAction(action);
            }
        }
    }
}

QMenu*  LC_WidgetFactory::menuTR(const char* title, QMenuBar* parent){
    auto result =  new QMenu(QC_ApplicationWindow::tr(title), parent);
    QString name(title);
    name.remove(' ');
    name.remove('&');
    result->setObjectName(name + "_menu");
    result->setTearOffEnabled(true);
    return result;
}

QMenu*  LC_WidgetFactory::subMenuTR(QMenu* parent, const char* title, const char* icon, const std::vector<QString> &actionNames){
    QMenu *result = doCreateSubMenuTR(parent, title, icon);
    addActions(result, actionNames);
    return result;
}

QMenu*  LC_WidgetFactory::subMenuWithActionsTR(QMenu* parent, const char* title, const char* icon, const QList<QAction*> &actions){
    QMenu *sub_menu = doCreateSubMenuTR(parent, title, icon);
    sub_menu->addActions(actions);
    return sub_menu;
}

QMenu *LC_WidgetFactory::doCreateSubMenuTR(QMenu *parent, const char *title, const char *icon) const {
    auto sub_menu = parent->addMenu(QC_ApplicationWindow::tr(title));
    if (icon != nullptr) {
        sub_menu->setIcon(QIcon(icon));
    }
    sub_menu->setTearOffEnabled(true);
    QString name(title);
    name.remove(' ');
    name.remove('&');
    sub_menu->setObjectName(title);
    return sub_menu;
}

QToolBar* LC_WidgetFactory::createGenericToolbarTR(const char* title, QSizePolicy toolBarPolicy, const std::vector<QString> &actionNames){
    auto* result = new QToolBar(QC_ApplicationWindow::tr(title), main_window);
    result->setSizePolicy(toolBarPolicy);
    QString name(title);
    name.remove(' ');
    result->setObjectName(name.toLower() + "_toolbar");
    for (const QString& actionName: actionNames){
        if (actionName.isEmpty()){
            result->addSeparator();
        }
        else{
            result->addAction(a_map[actionName]);
        }
    }
    return result;
}

QToolBar* LC_WidgetFactory::toolbarWithActionsTR(const char* title, QSizePolicy toolBarPolicy, const QList<QAction*> &actions){
    auto* result = new QToolBar(QC_ApplicationWindow::tr(title), main_window);
    result->setSizePolicy(toolBarPolicy);
    QString name(title);
    name.remove(' ');
    result->setObjectName(name.toLower() + "_toolbar");
    result->addActions(actions);
    result->hide();
    return result;
}

void  LC_WidgetFactory::fillActionsList(QList<QAction *> &list, const std::vector<const char *> &actionNames){
    for (const char* actionName: actionNames){
        list << a_map[actionName];
    }
}

LC_DockWidget* LC_WidgetFactory::leftDocWidgetTR(const char* title, const QList<QAction*> &actions, int columns, int iconSize){
    auto* result = new LC_DockWidget(main_window);
    QString name(title);
    result->setObjectName("dock_" + name.toLower());
    result->setWindowTitle(QC_ApplicationWindow::tr(title));
    result->add_actions(actions, columns, iconSize);
    result->hide();
    return result;
}

QToolButton*LC_WidgetFactory::toolButtonTR(QToolBar* toolbar, const char* tooltip, const char* icon, const QList<QAction*>& actions){
    auto * result = new QToolButton;
    result->setPopupMode(QToolButton::InstantPopup);
    result->setIcon(QIcon(icon));
    result->setToolTip(tr(tooltip));
    toolbar->addWidget(result);
    result->addActions(actions);
    return result;
}

void LC_WidgetFactory::addToTop(QToolBar *toolbar) { main_window->addToolBar(Qt::TopToolBarArea, toolbar); }
void LC_WidgetFactory::addToBottom(QToolBar *toolbar) { main_window->addToolBar(Qt::BottomToolBarArea, toolbar); }
void LC_WidgetFactory::addToLeft(QToolBar *toolbar) { main_window->addToolBar(Qt::LeftToolBarArea, toolbar); }
