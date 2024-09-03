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
        return LC_GET_ONE_BOOL("CustomToolbars", "UsePenPallet", true);
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
        "DrawEllipseArc1Point",
        "DrawLineFree"
    });

    fillActionsList(ellipse_actions, {
        "DrawEllipse1Point",
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
        "DimBaseline",
        "DimContinue",
        "DimRadial",
        "DimDiametric",
        "DimAngular",
        "DimArc",
        "DimLeader"
    });

    fillActionsList(other_drawing_actions, {
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
    actionsToDisableInPrintPreview.append(other_drawing_actions);
    actionsToDisableInPrintPreview.append(modify_actions);
    actionsToDisableInPrintPreview.append(order_actions);
    actionsToDisableInPrintPreview.append(info_actions);
    actionsToDisableInPrintPreview.append(block_actions);
    actionsToDisableInPrintPreview.append(pen_actions);
}

void LC_WidgetFactory::createLeftSidebar(int columns, int icon_size){
    auto* line = leftDocWidget(tr("Line"), "Line", line_actions, columns, icon_size);
    auto* circle = leftDocWidget(tr("Circle"), "Circle", circle_actions, columns, icon_size);
    auto* curve = leftDocWidget(tr("Curve"), "Curve", curve_actions, columns, icon_size);
    auto* ellipse = leftDocWidget(tr("Ellipse"), "Ellipse", ellipse_actions, columns, icon_size);
    auto* polyline = leftDocWidget(tr("Polyline"), "Polyline", polyline_actions, columns, icon_size);
    auto* select = leftDocWidget(tr("Select"), "Select", select_actions, columns, icon_size);
    auto* dimension = leftDocWidget(tr("Dimension"), "Dimension", dimension_actions, columns, icon_size);
    auto* other = leftDocWidget(tr("Other"), "Other", other_drawing_actions, columns, icon_size);
    auto* modify = leftDocWidget(tr("Modify"), "Modify", modify_actions, columns, icon_size);
    auto* info = leftDocWidget(tr("Info"), "Info", info_actions, columns, icon_size);
    auto* order = leftDocWidget(tr("Order"), "Order", order_actions, columns, icon_size);

    main_window->addDockWidget(Qt::LeftDockWidgetArea, line);
    main_window->tabifyDockWidget(line, polyline);
    line->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, circle);
    main_window->tabifyDockWidget(circle, curve);
    main_window->tabifyDockWidget(curve, ellipse);
    circle->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, dimension);
    main_window->tabifyDockWidget(dimension, other);
    main_window->tabifyDockWidget(other, info);
    main_window->tabifyDockWidget(info, select);
    dimension->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, modify);
    main_window->tabifyDockWidget(modify, order);
}

void LC_WidgetFactory::createRightSidebar(QG_ActionHandler* action_handler){
    QDockWidget* dock_pen_palette = nullptr;
    if (usePenPallet()) {
        dock_pen_palette = new QDockWidget(main_window);
        dock_pen_palette->setWindowTitle(tr("Pen Palette"));
        dock_pen_palette->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        dock_pen_palette->setObjectName("pen_palette_dockwidget");
        pen_palette = new LC_PenPaletteWidget("PenPalette", dock_pen_palette);
        pen_palette->setFocusPolicy(Qt::NoFocus);
        connect(pen_palette, SIGNAL(escape()), main_window, SLOT(slotFocus()));
//        connect(main_window, SIGNAL(windowsChanged(bool)), pen_palette, SLOT(setEnabled(bool)));
        dock_pen_palette ->setWidget(pen_palette);
    }
    auto* dock_layer = new QDockWidget(main_window);
    dock_layer->setWindowTitle(tr("Layer List"));
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
        dock_layer_tree->setWindowTitle(tr("Layer Tree"));
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
    dock_quick_info->setWindowTitle(tr("Entity Info"));
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
    dock_block->setWindowTitle(tr("Block List"));
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

    auto *file = createGenericToolbar(tr("File"), "file", tbPolicy, {});
    file->addActions(file_actions);
    file->addAction(a_map["FilePrint"]);
    file->addAction(a_map["FilePrintPreview"]);

    auto *edit = createGenericToolbar(tr("Edit"), "Edit", tbPolicy, {
        "EditKillAllActions", "", "EditUndo", "EditRedo", "", "EditCut", "EditCopy", "EditPaste", "EditPasteTransform"
    });

    auto *order = createGenericToolbar(tr("Order"), "Order", tbPolicy, {
        "OrderTop", "OrderBottom", "OrderRaise", "OrderLower"
    });
    order->hide();

    auto *view = createGenericToolbar(tr("View"), "View", tbPolicy, {
        "ViewGrid", "ViewDraft", "", "ZoomRedraw", "ZoomIn",
        "ZoomOut", "ZoomAuto", "ZoomPrevious", "ZoomWindow", "ZoomPan"
    });

    snap_toolbar = new QG_SnapToolBar(main_window, action_handler, ag_manager,a_map);
    snap_toolbar->setWindowTitle(tr("Snap Selection"));
    snap_toolbar->setSizePolicy(tbPolicy);
    snap_toolbar->setObjectName("snap_toolbar" );
    action_handler->set_snap_toolbar(snap_toolbar);

    connect( main_window,  &QC_ApplicationWindow::signalEnableRelativeZeroSnaps, 
             snap_toolbar, &QG_SnapToolBar::slotEnableRelativeZeroSnaps);

    pen_toolbar = new QG_PenToolBar(tr("Pen"), main_window);
    pen_toolbar->setSizePolicy(tbPolicy);
    pen_toolbar->setObjectName("pen_toolbar");
    pen_toolbar->addActions(pen_actions);

    options_toolbar = createGenericToolbar(tr("Tool Options"), "Tool Options", tbPolicy, {});
    
    auto *dockareas = createGenericToolbar(tr("Dock Areas"), "Dock Areas", tbPolicy, {
        "LeftDockAreaToggle", "RightDockAreaToggle", "TopDockAreaToggle",
        "BottomDockAreaToggle", "FloatingDockwidgetsToggle"
    });
    
    auto *creators = createGenericToolbar(tr("Creators"), "Creators", tbPolicy, {
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

    auto* line = toolbarWithActions(tr("Line"), "Line", tbPolicy, line_actions);
    auto* circle = toolbarWithActions(tr("Circle"), "Circle", tbPolicy, circle_actions);
    auto* curve = toolbarWithActions(tr("Curve"), "Curve", tbPolicy, curve_actions);
    auto* ellipse = toolbarWithActions(tr("Ellipse"), "Ellipse", tbPolicy, ellipse_actions);
    auto* polyline = toolbarWithActions(tr("Polyline"), "Polyline", tbPolicy, polyline_actions);
    auto* select = toolbarWithActions(tr("Select"), "Select", tbPolicy, select_actions);
    auto* dimension = toolbarWithActions(tr("Dimension"), "Dimension", tbPolicy, dimension_actions);
    auto* other = toolbarWithActions(tr("Other"), "other_drawing", tbPolicy, other_drawing_actions);
    auto* modify = toolbarWithActions(tr("Modify"), "Modify", tbPolicy, modify_actions);
    auto* info = toolbarWithActions(tr("Info"), "Info", tbPolicy, info_actions);

    addToBottom(line);
    addToBottom(circle);
    addToBottom(curve);
    addToBottom(ellipse);
    addToBottom(polyline);
    addToBottom(dimension);
    addToBottom(other);
    addToBottom(modify);
    addToBottom(info);
    addToBottom(select);
}

QToolBar *LC_WidgetFactory::createCategoriesToolbar() {
    auto *toolbar = createGenericToolbar(tr("Categories"), "Categories",
                                         QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding), {});

    toolButton(toolbar, tr("Lines"), ":/icons/line.svg", line_actions);
    toolButton(toolbar, tr("Circles"), ":/icons/circle.svg", circle_actions);
    toolButton(toolbar, tr("Freehand"), ":/icons/line_freehand.svg", curve_actions);
    toolButton(toolbar, tr("Ellipses"), ":/icons/ellipses.svg", ellipse_actions);
    toolButton(toolbar, tr("PolyLines"), ":/icons/polylines.svg", polyline_actions);
    toolButton(toolbar, tr("Select"), ":/icons/select.svg", select_actions);
    toolButton(toolbar, tr("Dimensions"), ":/icons/dim_horizontal.svg", dimension_actions);
    toolButton(toolbar, tr("Other"), ":/icons/text.svg", other_drawing_actions);
    toolButton(toolbar, tr("Modify"), ":/icons/move_rotate.svg", modify_actions);
    toolButton(toolbar, tr("Measure"), ":/icons/measure.svg", info_actions);
    toolButton(toolbar, tr("Order"), ":/icons/order.svg", order_actions);

    addToLeft(toolbar);
    return toolbar;
}

void LC_WidgetFactory::createMenus(QMenuBar* menu_bar){
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

    subMenu(file_menu, tr("Import"),"import", ":/icons/import.svg", {
        "DrawImage",
        "BlocksImport"
    });

    subMenu(file_menu, tr("Export"),"export", ":/icons/export.svg", {
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

    auto settings = menu(tr("&Options"),"options", menu_bar, {
        "OptionsGeneral",
        "ShortcutsOptions",
        "WidgetOptions",
        "DeviceOptions",
        "ReloadStyleSheet",
        "",
        "OptionsDrawing",
    });

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
        "",
        "EditCutQuick",
        "EditCopyQuick",
        "ModifyDeleteQuick"
    });

    auto plugins = menu(tr("Pl&ugins"),"plugins", menu_bar);

    auto view = menu(tr("&View"),"view", menu_bar, {
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
    auto tools = menu(tr("&Tools"), "tools", menu_bar);
    subMenuWithActions(tools, tr("&Line"), "line", ":/icons/line.svg", line_actions);
    subMenuWithActions(tools, tr("&Circle"), "circle", ":/icons/circle.svg", circle_actions);
    subMenuWithActions(tools, tr("&Curve"), "curve", ":/icons/line_freehand.svg", curve_actions);
    subMenuWithActions(tools, tr("&Ellipse"), "ellipse", ":/icons/ellipses.svg", ellipse_actions);
    subMenuWithActions(tools, tr("&Polyline"), "polyline", ":/icons/polylines_polyline.svg", polyline_actions);
    subMenuWithActions(tools, tr("&Select"), "select", ":/icons/select.svg", select_actions);
    subMenuWithActions(tools, tr("Dime&nsion"), "dimension", ":/icons/dim_horizontal.svg", dimension_actions);
    subMenuWithActions(tools, tr("Ot&her"), "other", ":/icons/text.svg", other_drawing_actions);
    subMenuWithActions(tools, tr("&Modify"), "modify", ":/icons/move_rotate.svg", modify_actions);
    subMenuWithActions(tools, tr("&Info"), "info", ":/icons/measure.svg", info_actions);
    subMenuWithActions(tools, tr("&Order"), "order", ":/icons/order.svg", order_actions);

    windows_menu = menu(tr("&Drawings"),"drawings", menu_bar, {
        "Fullscreen" // temp way to show this menu on OS X
    });

    connect(windows_menu, SIGNAL(aboutToShow()),
            main_window, SLOT(slotWindowsMenuAboutToShow()));

    auto help = menu(tr("&Help"), "help", menu_bar);

    subMenuWithActions(help, tr("On&line"),"Online", nullptr, {
        urlActionTR(tr("&Wiki"), "https://dokuwiki.librecad.org/"),
        urlActionTR(tr("User's &Manual"), "https://librecad.readthedocs.io/"),
        urlActionTR(tr("&Commands"), "https://librecad.readthedocs.io/en/latest/ref/tools.html"),
        urlActionTR(tr("&Style Sheets"), "https://librecad.readthedocs.io/en/latest/ref/customize.html#style-sheets"),
        urlActionTR(tr("Wid&gets"), "https://librecad.readthedocs.io/en/latest/ref/menu.html#widgets"),
        urlActionTR(tr("&Forum"), "https://forum.librecad.org/"),
        urlActionTR(tr("Zulip &Chat"), "https://librecad.zulipchat.com/"),
        urlActionTR(tr("&Release Information"), "https://github.com/LibreCAD/LibreCAD/releases")
    });

    auto help_about = new QAction(QIcon(":/main/librecad.png"), tr("About"), main_window);
    connect(help_about, SIGNAL(triggered()), main_window, SLOT(showAboutWindow()));

    auto license = new QAction(QObject::tr("License"), main_window);
    connect(license, SIGNAL(triggered()), main_window, SLOT(invokeLicenseWindow()));

    help->addSeparator();
    help->addAction(help_about);
    help->addAction(license);
    help->addAction(urlActionTR(tr("&Donate"), "https://librecad.org/donate.html"));

    auto widgets = menu(tr("Widgets"),"widgets", menu_bar);

    auto *dockareas = subMenu(widgets, tr("Dock Areas"),"dockareas", nullptr, {
        "LeftDockAreaToggle",
        "RightDockAreaToggle",
        "TopDockAreaToggle",
        "BottomDockAreaToggle",
        "FloatingDockwidgetsToggle"
    });

    auto* dockwidgets_menu = doCreateSubMenu(widgets, tr("Dock Wid&gets"), "dockwidgets", nullptr);

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

    auto* toolbars = doCreateSubMenu(widgets, tr("&Toolbars"),"toolbars", nullptr);

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

QAction* LC_WidgetFactory::urlActionTR(const QString& title, const char* url ){
    auto* result    = new QAction(  title, main_window);
    connect(result, &QAction::triggered, main_window, [=](){
        QDesktopServices::openUrl( QUrl( url));
    });
    return result;
}

QMenu*  LC_WidgetFactory::menu(const QString& title, const QString& name,  QMenuBar* parent, const std::vector<QString> &actionNames){
    QMenu* result = menu(title, name, parent);
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

QMenu*  LC_WidgetFactory::menu(const QString& title, const QString& name, QMenuBar* parent){
    auto result =  new QMenu(title, parent);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    result->setObjectName(nameCleared.toLower() + "_menu");
    result->setTearOffEnabled(true);
    return result;
}

QMenu*  LC_WidgetFactory::subMenu(QMenu* parent, const QString& title, const QString& name, const char* icon, const std::vector<QString> &actionNames){
    QMenu *result = doCreateSubMenu(parent, title, name, icon);
    addActions(result, actionNames);
    return result;
}

QMenu*  LC_WidgetFactory::subMenuWithActions(QMenu* parent, const QString& title, const QString& name, const char* icon, const QList<QAction*> &actions){
    QMenu *sub_menu = doCreateSubMenu(parent, title, name, icon);
    sub_menu->addActions(actions);
    return sub_menu;
}

QMenu *LC_WidgetFactory::doCreateSubMenu(QMenu *parent, const QString& title, const QString& name, const char *icon) const {
    auto sub_menu = parent->addMenu(title);
    if (icon != nullptr) {
        sub_menu->setIcon(QIcon(icon));
    }
    sub_menu->setTearOffEnabled(true);
    QString nameCleared(name);
    nameCleared.remove(' ');
    nameCleared.remove('&');
    const QString &objectName = nameCleared.toLower() + "_menu";
    sub_menu->setObjectName(objectName);
    return sub_menu;
}

QToolBar* LC_WidgetFactory::createGenericToolbar(const QString& title, const QString &name, QSizePolicy toolBarPolicy, const std::vector<QString> &actionNames){
    auto* result = new QToolBar(title, main_window);
    result->setSizePolicy(toolBarPolicy);
    QString nameCleared(name);
    nameCleared.remove(' ');
    const QString &objectName = nameCleared.toLower() + "_toolbar";
    result->setObjectName(objectName);
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

QToolBar* LC_WidgetFactory::toolbarWithActions(const QString& title, const QString& name, QSizePolicy toolBarPolicy, const QList<QAction*> &actions){
    auto* result = new QToolBar(title, main_window);
    result->setSizePolicy(toolBarPolicy);
    QString nameCleaned = name;
    nameCleaned.remove(' ');
    result->setObjectName(nameCleaned.toLower() + "_toolbar");
    result->addActions(actions);
    result->hide();
    return result;
}

void  LC_WidgetFactory::fillActionsList(QList<QAction *> &list, const std::vector<const char *> &actionNames){
    for (const char* actionName: actionNames){
        list << a_map[actionName];
    }
}

LC_DockWidget* LC_WidgetFactory::leftDocWidget(const QString& title, const char* name, const QList<QAction*> &actions, int columns, int iconSize){
    auto* result = new LC_DockWidget(main_window);
    result->setObjectName("dock_" + QString(name).toLower());
    result->setWindowTitle(title);
    result->add_actions(actions, columns, iconSize);
    result->hide();
    return result;
}

QToolButton*LC_WidgetFactory::toolButton(QToolBar* toolbar, const QString &tooltip, const char* icon, const QList<QAction*>& actions){
    auto * result = new QToolButton(toolbar); // ignore memory warning leak, toolbar will delete button
    result->setPopupMode(QToolButton::InstantPopup);
    result->setIcon(QIcon(icon));
    result->setToolTip(tooltip);
    toolbar->addWidget(result);
    result->addActions(actions);
    return result;
}

void LC_WidgetFactory::addToTop(QToolBar *toolbar) { main_window->addToolBar(Qt::TopToolBarArea, toolbar); }
void LC_WidgetFactory::addToBottom(QToolBar *toolbar) { main_window->addToolBar(Qt::BottomToolBarArea, toolbar); }
void LC_WidgetFactory::addToLeft(QToolBar *toolbar) { main_window->addToolBar(Qt::LeftToolBarArea, toolbar); }
