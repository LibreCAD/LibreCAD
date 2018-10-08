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

#include "qc_applicationwindow.h"
#include "lc_widgetfactory.h"
#include "lc_actionfactory.h"
#include "lc_dockwidget.h"
#include "lc_actiongroupmanager.h"

#include "qg_actionhandler.h"
#include "qg_snaptoolbar.h"
#include "qg_blockwidget.h"
#include "qg_layerwidget.h"
#include "qg_librarywidget.h"
#include "qg_commandwidget.h"
#include "qg_selectionwidget.h"
#include "qg_activelayername.h"
#include "qg_mousewidget.h"
#include "qg_pentoolbar.h"

#include <QMenu>
#include <QFile>
#include <QMenuBar>
#include <QActionGroup>


LC_WidgetFactory::LC_WidgetFactory(QC_ApplicationWindow* main_win,
                                   QMap<QString, QAction*>& action_map,
                                   LC_ActionGroupManager* agm)
    : QObject(nullptr)
    , main_window(main_win)
    , a_map(action_map)
    , ag_manager(agm)
{
    file_actions
            << a_map["FileNew"]
            << a_map["FileNewTemplate"]
            << a_map["FileOpen"]
            << a_map["FileSave"]
            << a_map["FileSaveAs"];

    line_actions
            << a_map["DrawLine"]
            << a_map["DrawLineAngle"]
            << a_map["DrawLineHorizontal"]
            << a_map["DrawLineVertical"]
            << a_map["DrawLineRectangle"]
            << a_map["DrawLineParallelThrough"]
            << a_map["DrawLineParallel"]
            << a_map["DrawLineBisector"]
            << a_map["DrawLineTangent1"]
            << a_map["DrawLineTangent2"]
            << a_map["DrawLineOrthTan"]
            << a_map["DrawLineOrthogonal"]
            << a_map["DrawLineRelAngle"]
            << a_map["DrawLinePolygonCenCor"]
            << a_map["DrawLinePolygonCenTan"]   //20161226 added by txmy
            << a_map["DrawLinePolygonCorCor"];

    circle_actions
            << a_map["DrawCircle"]
            << a_map["DrawCircle2P"]
            << a_map["DrawCircle2PR"]
            << a_map["DrawCircle3P"]
            << a_map["DrawCircleCR"]
            << a_map["DrawCircleTan2_1P"]
            << a_map["DrawCircleTan1_2P"]
            << a_map["DrawCircleTan2"]
            << a_map["DrawCircleTan3"];

    curve_actions
            << a_map["DrawArc"]
            << a_map["DrawArc3P"]
            << a_map["DrawArcTangential"]
            << a_map["DrawSpline"]
            << a_map["DrawSplinePoints"]
            << a_map["DrawEllipseArcAxis"]
            << a_map["DrawLineFree"];

    ellipse_actions
            << a_map["DrawEllipseAxis"]
            << a_map["DrawEllipseFociPoint"]
            << a_map["DrawEllipse4Points"]
            << a_map["DrawEllipseCenter3Points"]
            << a_map["DrawEllipseInscribe"];

    polyline_actions
            << a_map["DrawPolyline"]
            << a_map["PolylineAdd"]
            << a_map["PolylineAppend"]
            << a_map["PolylineDel"]
            << a_map["PolylineDelBetween"]
            << a_map["PolylineTrim"]
            << a_map["PolylineEquidistant"]
            << a_map["PolylineSegment"];

    select_actions
            << a_map["DeselectAll"]
            << a_map["SelectAll"]
            << a_map["SelectSingle"]
            << a_map["SelectContour"]
            << a_map["SelectWindow"]
            << a_map["DeselectWindow"]
            << a_map["SelectIntersected"]
            << a_map["DeselectIntersected"]
            << a_map["SelectLayer"]
            << a_map["SelectInvert"];

    dimension_actions
            << a_map["DimAligned"]
            << a_map["DimLinear"]
            << a_map["DimLinearHor"]
            << a_map["DimLinearVer"]
            << a_map["DimRadial"]
            << a_map["DimDiametric"]
            << a_map["DimAngular"]
            << a_map["DimLeader"];

    modify_actions
            << a_map["ModifyMove"]
            << a_map["ModifyRotate"]
            << a_map["ModifyScale"]
            << a_map["ModifyMirror"]
            << a_map["ModifyMoveRotate"]
            << a_map["ModifyRotate2"]
            << a_map["ModifyRevertDirection"]
            << a_map["ModifyTrim"]
            << a_map["ModifyTrim2"]
            << a_map["ModifyTrimAmount"]
            << a_map["ModifyOffset"]
            << a_map["ModifyBevel"]
            << a_map["ModifyRound"]
            << a_map["ModifyCut"]
            << a_map["ModifyStretch"]
            << a_map["ModifyEntity"]
            << a_map["ModifyAttributes"]
            << a_map["ModifyExplodeText"]
            << a_map["BlocksExplode"]
            << a_map["ModifyDeleteQuick"];

    info_actions
            << a_map["InfoDist"]
            << a_map["InfoDist2"]
            << a_map["InfoAngle"]
            << a_map["InfoTotalLength"]
            << a_map["InfoArea"];

    layer_actions
            << a_map["LayersDefreezeAll"]
            << a_map["LayersFreezeAll"]
            << a_map["LayersUnlockAll"]
            << a_map["LayersLockAll"]
            << a_map["LayersAdd"]
            << a_map["LayersRemove"]
            << a_map["LayersEdit"]
            << a_map["LayersToggleLock"]
            << a_map["LayersToggleView"]
            << a_map["LayersTogglePrint"]
            << a_map["LayersToggleConstruction"];

    block_actions
            << a_map["BlocksDefreezeAll"]
            << a_map["BlocksFreezeAll"]
            << a_map["BlocksToggleView"]
            << a_map["BlocksAdd"]
            << a_map["BlocksRemove"]
            << a_map["BlocksAttributes"]
            << a_map["BlocksInsert"]
            << a_map["BlocksEdit"]
            << a_map["BlocksSave"]
            << a_map["BlocksCreate"]
            << a_map["BlocksExplode"];
}


void LC_WidgetFactory::createLeftSidebar(int columns, int icon_size)
{
    LC_DockWidget* dock_line = new LC_DockWidget(main_window);
    dock_line->setObjectName("dock_line");
    dock_line->setWindowTitle(QC_ApplicationWindow::tr("Line"));
    dock_line->add_actions(line_actions, columns, icon_size);

    LC_DockWidget* dock_circle = new LC_DockWidget(main_window);
    dock_circle->setObjectName("dock_circle");
    dock_circle->setWindowTitle(QC_ApplicationWindow::tr("Circle"));
    dock_circle->add_actions(circle_actions, columns, icon_size);

    LC_DockWidget* dock_curve = new LC_DockWidget(main_window);
    dock_curve->setObjectName("dock_curve");
    dock_curve->setWindowTitle(QC_ApplicationWindow::tr("Curve"));
    dock_curve->add_actions(curve_actions, columns, icon_size);

    LC_DockWidget* dock_ellipse = new LC_DockWidget(main_window);
    dock_ellipse->setObjectName("dock_ellipse");
    dock_ellipse->setWindowTitle(QC_ApplicationWindow::tr("Ellipse"));
    dock_ellipse->add_actions(ellipse_actions, columns, icon_size);

    LC_DockWidget* dock_polyline = new LC_DockWidget(main_window);
    dock_polyline->setObjectName("dock_polyline");
    dock_polyline->setWindowTitle(QC_ApplicationWindow::tr("Polyline"));
    dock_polyline->add_actions(polyline_actions, columns, icon_size);

    LC_DockWidget* dock_select = new LC_DockWidget(main_window);
    dock_select->setObjectName("dock_select");
    dock_select->setWindowTitle(QC_ApplicationWindow::tr("Select"));
    dock_select->add_actions(select_actions, columns, icon_size);

    LC_DockWidget* dock_dimension = new LC_DockWidget(main_window);
    dock_dimension->setObjectName("dock_dimension");
    dock_dimension->setWindowTitle(QC_ApplicationWindow::tr("Dimension"));
    dock_dimension->add_actions(dimension_actions, columns, icon_size);

    LC_DockWidget* dock_modify = new LC_DockWidget(main_window);
    dock_modify->setObjectName("dock_modify");
    dock_modify->setWindowTitle(QC_ApplicationWindow::tr("Modify"));
    dock_modify->add_actions(modify_actions, columns, icon_size);

    LC_DockWidget* dock_info = new LC_DockWidget(main_window);
    dock_info->setObjectName("dock_info");
    dock_info->setWindowTitle(QC_ApplicationWindow::tr("Info"));
    dock_info->add_actions(info_actions, columns, icon_size);

    main_window->addDockWidget(Qt::LeftDockWidgetArea, dock_line);
    main_window->tabifyDockWidget(dock_line, dock_polyline);
    dock_line->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, dock_circle);
    main_window->tabifyDockWidget(dock_circle, dock_curve);
    main_window->tabifyDockWidget(dock_curve, dock_ellipse);
    dock_circle->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, dock_dimension);
    main_window->tabifyDockWidget(dock_dimension, dock_info);
    main_window->tabifyDockWidget(dock_info, dock_select);
    dock_dimension->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, dock_modify);

    dock_line->hide();
    dock_polyline->hide();
    dock_circle->hide();
    dock_curve->hide();
    dock_ellipse->hide();
    dock_dimension->hide();
    dock_info->hide();
    dock_modify->hide();
    dock_select->hide();
}

void LC_WidgetFactory::createRightSidebar(QG_ActionHandler* action_handler)
{
    QDockWidget* dock_layer = new QDockWidget(main_window);
    dock_layer->setWindowTitle(QC_ApplicationWindow::tr("Layer List"));
    dock_layer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_layer->setObjectName("layer_dockwidget");
    layer_widget = new QG_LayerWidget(action_handler, dock_layer, "Layer");
    layer_widget->setFocusPolicy(Qt::NoFocus);
    connect(layer_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
    connect(main_window, SIGNAL(windowsChanged(bool)), layer_widget, SLOT(setEnabled(bool)));
    dock_layer->setWidget(layer_widget);

    QDockWidget* dock_block = new QDockWidget(main_window);
    dock_block->setWindowTitle(QC_ApplicationWindow::tr("Block List"));
    dock_block->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_block->setObjectName("block_dockwidget");
    block_widget = new QG_BlockWidget(action_handler, dock_block, "Block");
    block_widget->setFocusPolicy(Qt::NoFocus);
    connect(block_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
    connect(main_window, SIGNAL(windowsChanged(bool)), block_widget, SLOT(setEnabled(bool)));
    dock_block->setWidget(block_widget);

    QDockWidget* dock_library = new QDockWidget(main_window);
    dock_library->setWindowTitle(QC_ApplicationWindow::tr("Library Browser"));
    dock_library->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_library->setObjectName("library_dockwidget");
    library_widget = new QG_LibraryWidget(dock_library, "Library");
    library_widget->setActionHandler(action_handler);
    library_widget->setFocusPolicy(Qt::NoFocus);
    connect(library_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
    connect(main_window, SIGNAL(windowsChanged(bool)),
            (QObject*)library_widget->bInsert, SLOT(setEnabled(bool)));
    dock_library->setWidget(library_widget);
    dock_library->resize(240, 400);

    QDockWidget* dock_command = new QDockWidget(QC_ApplicationWindow::tr("Command line"), main_window);
    dock_command->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_command->setObjectName("command_dockwidget");
    command_widget = new QG_CommandWidget(dock_command, "Command");
    command_widget->setActionHandler(action_handler);
    connect(main_window, SIGNAL(windowsChanged(bool)), command_widget, SLOT(setEnabled(bool)));
    connect(command_widget->leCommand, SIGNAL(escape()), main_window, SLOT(setFocus()));
    dock_command->setWidget(command_widget);

    connect(dock_command, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
            main_window, SLOT(modifyCommandTitleBar(Qt::DockWidgetArea)));

    main_window->addDockWidget(Qt::RightDockWidgetArea, dock_library);
    main_window->tabifyDockWidget(dock_library, dock_block);
    main_window->tabifyDockWidget(dock_block, dock_layer);
    main_window->addDockWidget(Qt::RightDockWidgetArea, dock_command);
}

void LC_WidgetFactory::createStandardToolbars(QG_ActionHandler* action_handler)
{
    QSizePolicy toolBarPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QToolBar* file_toolbar = new QToolBar(QC_ApplicationWindow::tr("File"), main_window);
    file_toolbar->setSizePolicy(toolBarPolicy);
    file_toolbar->setObjectName("file_toolbar");
    file_toolbar->addActions(file_actions);
    file_toolbar->addAction(a_map["FilePrint"]);
    file_toolbar->addAction(a_map["FilePrintPreview"]);

    QToolBar* edit_toolbar = new QToolBar(QC_ApplicationWindow::tr("Edit"), main_window);
    edit_toolbar->setSizePolicy(toolBarPolicy);
    edit_toolbar->setObjectName("edit_toolbar");
    edit_toolbar->addAction(a_map["EditKillAllActions"]);
    edit_toolbar->addSeparator();
    edit_toolbar->addAction(a_map["EditUndo"]);
    edit_toolbar->addAction(a_map["EditRedo"]);
    edit_toolbar->addSeparator();
    edit_toolbar->addAction(a_map["EditCut"]);
    edit_toolbar->addAction(a_map["EditCopy"]);
    edit_toolbar->addAction(a_map["EditPaste"]);

    QToolBar* order_toolbar = new QToolBar(QC_ApplicationWindow::tr("Order"), main_window);
    order_toolbar->setSizePolicy(toolBarPolicy);
    order_toolbar->setObjectName("order_toolbar");
    order_toolbar->addAction(a_map["OrderTop"]);
    order_toolbar->addAction(a_map["OrderBottom"]);
    order_toolbar->addAction(a_map["OrderRaise"]);
    order_toolbar->addAction(a_map["OrderLower"]);
    order_toolbar->hide();

    QToolBar* view_toolbar = new QToolBar(QC_ApplicationWindow::tr("View"), main_window);
    view_toolbar->setObjectName("view_toolbar");
    view_toolbar->setSizePolicy(toolBarPolicy);
    view_toolbar->addAction(a_map["ViewGrid"]);
    view_toolbar->addAction(a_map["ViewDraft"]);
    view_toolbar->addSeparator();
    view_toolbar->addAction(a_map["ZoomRedraw"]);
    view_toolbar->addAction(a_map["ZoomIn"]);
    view_toolbar->addAction(a_map["ZoomOut"]);
    view_toolbar->addAction(a_map["ZoomAuto"]);
    view_toolbar->addAction(a_map["ZoomPrevious"]);
    view_toolbar->addAction(a_map["ZoomWindow"]);
    view_toolbar->addAction(a_map["ZoomPan"]);

    snap_toolbar = new QG_SnapToolBar(main_window, action_handler, ag_manager);
    snap_toolbar->setWindowTitle(QC_ApplicationWindow::tr("Snap Selection"));
    snap_toolbar->setSizePolicy(toolBarPolicy);
    snap_toolbar->setObjectName("snap_toolbar" );
    action_handler->set_snap_toolbar(snap_toolbar);

    pen_toolbar = new QG_PenToolBar(QC_ApplicationWindow::tr("Pen"), main_window);
    pen_toolbar->setSizePolicy(toolBarPolicy);
    pen_toolbar->setObjectName("pen_toolbar");

    options_toolbar = new QToolBar(QC_ApplicationWindow::tr("Tool Options"), main_window);
    options_toolbar->setSizePolicy(toolBarPolicy);
    options_toolbar->setObjectName("options_toolbar");

    // <[~ Dock Areas Toolbar ~]>

    QToolBar* dockareas_toolbar = new QToolBar(main_window);
    dockareas_toolbar->setWindowTitle(QC_ApplicationWindow::tr("Dock Areas"));
    dockareas_toolbar->setSizePolicy(toolBarPolicy);
    dockareas_toolbar->setObjectName("dockareas_toolbar");
    dockareas_toolbar->addAction(a_map["LeftDockAreaToggle"]);
    dockareas_toolbar->addAction(a_map["RightDockAreaToggle"]);
    dockareas_toolbar->addAction(a_map["TopDockAreaToggle"]);
    dockareas_toolbar->addAction(a_map["BottomDockAreaToggle"]);
    dockareas_toolbar->addAction(a_map["FloatingDockwidgetsToggle"]);

    // <[~ Creators ~]>

    auto creators_toolbar = new QToolBar(main_window);
    creators_toolbar->setWindowTitle(QObject::tr("Creators"));
    creators_toolbar->setObjectName("creators_toolbar");
    creators_toolbar->addAction(a_map["InvokeMenuCreator"]);
    creators_toolbar->addAction(a_map["InvokeToolbarCreator"]);

    // <[~ Toolbars Layout~]>

    main_window->addToolBar(Qt::TopToolBarArea, file_toolbar);
    main_window->addToolBar(Qt::TopToolBarArea, edit_toolbar);
    main_window->addToolBar(Qt::TopToolBarArea, view_toolbar);
    main_window->addToolBarBreak();
    main_window->addToolBar(Qt::TopToolBarArea, pen_toolbar);
    main_window->addToolBar(Qt::TopToolBarArea, options_toolbar);

    main_window->addToolBar(Qt::LeftToolBarArea, order_toolbar);

    main_window->addToolBar(Qt::BottomToolBarArea, snap_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, dockareas_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, creators_toolbar);
}

void LC_WidgetFactory::createCADToolbars()
{
    QSizePolicy toolBarPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QToolBar* line_toolbar = new QToolBar(QC_ApplicationWindow::tr("Line"), main_window);
    line_toolbar->setSizePolicy(toolBarPolicy);
    line_toolbar->setObjectName("line_toolbar");
    line_toolbar->addActions(line_actions);
    line_toolbar->hide();

    QToolBar* circle_toolbar = new QToolBar(QC_ApplicationWindow::tr("Circle"), main_window);
    circle_toolbar->setSizePolicy(toolBarPolicy);
    circle_toolbar->setObjectName ("circle_toolbar");
    circle_toolbar->addActions(circle_actions);
    circle_toolbar->hide();

    QToolBar* curve_toolbar = new QToolBar(QC_ApplicationWindow::tr("Curve"), main_window);
    curve_toolbar->setSizePolicy(toolBarPolicy);
    curve_toolbar->setObjectName("curve_toolbar");
    curve_toolbar->addActions(curve_actions);
    curve_toolbar->hide();

    QToolBar* ellipse_toolbar = new QToolBar(QC_ApplicationWindow::tr("Ellipse"), main_window);
    ellipse_toolbar->setSizePolicy(toolBarPolicy);
    ellipse_toolbar->setObjectName("ellipse_toolbar");
    ellipse_toolbar->addActions(ellipse_actions);
    ellipse_toolbar->hide();

    QToolBar* polyline_toolbar = new QToolBar(QC_ApplicationWindow::tr("Polyline"), main_window);
    polyline_toolbar->setSizePolicy(toolBarPolicy);
    polyline_toolbar->setObjectName("polyline_toolbar");
    polyline_toolbar->addActions(polyline_actions);
    polyline_toolbar->hide();

    QToolBar* select_toolbar = new QToolBar(QC_ApplicationWindow::tr("Select"), main_window);
    select_toolbar->setSizePolicy(toolBarPolicy);
    select_toolbar->setObjectName("select_toolbar");
    select_toolbar->addActions(select_actions);
    select_toolbar->hide();

    QToolBar* dimension_toolbar = new QToolBar(QC_ApplicationWindow::tr("Dimension"), main_window);
    dimension_toolbar->setSizePolicy(toolBarPolicy);
    dimension_toolbar->setObjectName("dimension_toolbar");
    dimension_toolbar->addActions(dimension_actions);
    dimension_toolbar->hide();

    QToolBar* modify_toolbar = new QToolBar(QC_ApplicationWindow::tr("Modify"), main_window);
    modify_toolbar->setSizePolicy(toolBarPolicy);
    modify_toolbar->setObjectName("modify_toolbar");
    modify_toolbar->addActions(modify_actions);
    modify_toolbar->hide();

    QToolBar* info_toolbar = new QToolBar(QC_ApplicationWindow::tr("Info"), main_window);
    info_toolbar->setSizePolicy(toolBarPolicy);
    info_toolbar->setObjectName("info_toolbar");
    info_toolbar->addActions(info_actions);
    info_toolbar->hide();

    main_window->addToolBar(Qt::BottomToolBarArea, line_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, circle_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, curve_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, ellipse_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, polyline_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, dimension_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, modify_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, info_toolbar);
    main_window->addToolBar(Qt::BottomToolBarArea, select_toolbar);
}

QToolBar* LC_WidgetFactory::createCategoriesToolbar()
{
    QSizePolicy toolBarPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QToolBar* categories_toolbar = new QToolBar(QC_ApplicationWindow::tr("Categories"), main_window);
    categories_toolbar->setSizePolicy(toolBarPolicy);
    categories_toolbar->setObjectName("categories_toolbar");

    QToolButton* tool_button;

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/line.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(line_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/circle.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(circle_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/line_freehand.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(curve_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/ellipses.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(ellipse_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/polylines.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(polyline_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/select.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(select_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/dim_horizontal.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(dimension_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/move_rotate.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(modify_actions);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/icons/measure.svg"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(info_actions);

    main_window->addToolBar(Qt::LeftToolBarArea, categories_toolbar);

    return categories_toolbar;
}

void LC_WidgetFactory::createMenus(QMenuBar* menu_bar)
{
    QMenu* sub_menu;
    // <[~ File ~]>

    file_menu = new QMenu(QC_ApplicationWindow::tr("&File"), menu_bar);
    file_menu->setObjectName("File");
    file_menu->setTearOffEnabled(true);
    file_menu->addActions(file_actions);
    sub_menu = file_menu->addMenu(QIcon(":/icons/import.svg"), QC_ApplicationWindow::tr("Import"));
    sub_menu->setObjectName("Import");
    sub_menu->addAction(a_map["DrawImage"]);
    sub_menu->addAction(a_map["BlocksImport"]);
    sub_menu = file_menu->addMenu(QIcon(":/icons/export.svg"), QC_ApplicationWindow::tr("Export"));
    sub_menu->setObjectName("Export");
    sub_menu->addAction(a_map["FileExportMakerCam"]);
    sub_menu->addAction(a_map["FilePrintPDF"]);
    sub_menu->addAction(a_map["FileExport"]);
    file_menu->addSeparator();
    file_menu->addAction(a_map["FilePrint"]);
    file_menu->addAction(a_map["FilePrintPreview"]);
    file_menu->addSeparator();
    file_menu->addAction(a_map["FileClose"]);
    file_menu->addAction(a_map["FileQuit"]);
    file_menu->addSeparator();

    // <[~ Options ~]>

    QMenu* settings_menu = new QMenu(QC_ApplicationWindow::tr("&Options"), menu_bar);
    settings_menu->setObjectName("options_menu");
    settings_menu->setTearOffEnabled(true);
    settings_menu->addAction(a_map["OptionsGeneral"]);
    settings_menu->addAction(a_map["OptionsDrawing"]);
    settings_menu->addAction(a_map["WidgetOptions"]);
    settings_menu->addAction(a_map["DeviceOptions"]);
    settings_menu->addAction(a_map["ReloadStyleSheet"]);

    // <[~ Edit ~]>

    QMenu* edit_menu = new QMenu(QC_ApplicationWindow::tr("&Edit"), menu_bar);
    edit_menu->setObjectName("Edit");
    edit_menu->setTearOffEnabled(true);
    edit_menu->addAction(a_map["EditKillAllActions"]);
    edit_menu->addSeparator();
    edit_menu->addAction(a_map["EditUndo"]);
    edit_menu->addAction(a_map["EditRedo"]);
    edit_menu->addSeparator();
    edit_menu->addAction(a_map["EditCut"]);
    edit_menu->addAction(a_map["EditCopy"]);
    edit_menu->addAction(a_map["EditPaste"]);
    edit_menu->addAction(a_map["ModifyDeleteQuick"]);

    // <[~ Plugins ~]>

    QMenu* plugins_menu = new QMenu(QC_ApplicationWindow::tr("Pl&ugins"), menu_bar);
    plugins_menu->setObjectName("plugins_menu");
    plugins_menu->setTearOffEnabled(true);

    // <[~ View ~]>

    QMenu* view_menu = new QMenu(QC_ApplicationWindow::tr("&View"), menu_bar);
    view_menu->setObjectName("view_menu");
    view_menu->setTearOffEnabled(true);
    view_menu->addAction(a_map["Fullscreen"]);
    view_menu->addAction(a_map["ViewStatusBar"]);
    view_menu->addAction(a_map["ViewGrid"]);
    view_menu->addAction(a_map["ViewDraft"]);
    view_menu->addSeparator();
    view_menu->addAction(a_map["ZoomRedraw"]);
    view_menu->addAction(a_map["ZoomIn"]);
    view_menu->addAction(a_map["ZoomOut"]);
    view_menu->addAction(a_map["ZoomAuto"]);
    view_menu->addAction(a_map["ZoomPrevious"]);
    view_menu->addAction(a_map["ZoomWindow"]);
    view_menu->addAction(a_map["ZoomPan"]);

    // <[~ Tools ~]>

    QMenu* tools_menu = new QMenu(QC_ApplicationWindow::tr("&Tools"), menu_bar);
    tools_menu->setObjectName("tools_menu");
    tools_menu->setTearOffEnabled(true);

    // <[~ Lines ~]>

    sub_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Line"));
    sub_menu->setIcon(QIcon(":/icons/line.svg"));
    sub_menu->setObjectName("Line");
    sub_menu->addActions(line_actions);

    // <[~ Circles ~]>

    sub_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Circle"));
    sub_menu->setIcon(QIcon(":/icons/circle.svg"));
    sub_menu->setObjectName("Circle");
    sub_menu->addActions(circle_actions);

    // <[~ Curves ~]>

    sub_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Curve"));
    sub_menu->setIcon(QIcon(":/icons/line_freehand.svg"));
    sub_menu->setObjectName("Curve");
    sub_menu->addActions(curve_actions);

    // <[~ Ellipses ~]>

    sub_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Ellipse"));
    sub_menu->setIcon(QIcon(":/icons/ellipses.svg"));
    sub_menu->setObjectName("Ellipse");
    sub_menu->addActions(ellipse_actions);

    // <[~ Polylines ~]>

    sub_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Polyline"));
    sub_menu->setIcon(QIcon(":/icons/polylines_polyline.svg"));
    sub_menu->setObjectName("Polyline");
    sub_menu->addActions(polyline_actions);

    // <[~ Select ~]>

    QMenu* select_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Select"));
    select_menu->setIcon(QIcon(":/icons/select.svg"));
    select_menu->setObjectName("Select");
    select_menu->setTearOffEnabled(true);
    select_menu->addActions(select_actions);

    // <[~ Dimension ~]>

    QMenu* dimension_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("Dime&nsion"));
    dimension_menu->setIcon(QIcon(":/icons/dim_horizontal.svg"));
    dimension_menu->setObjectName("dimension_menu");
    dimension_menu->setTearOffEnabled(true);
    dimension_menu->addActions(dimension_actions);

    // <[~ Order ~]>

    QMenu* order_menu = new QMenu(QC_ApplicationWindow::tr("&Order"), menu_bar);
    order_menu->setObjectName("order_menu");
    order_menu->setTearOffEnabled(true);
    order_menu->addAction(a_map["OrderTop"]);
    order_menu->addAction(a_map["OrderBottom"]);
    order_menu->addAction(a_map["OrderRaise"]);
    order_menu->addAction(a_map["OrderLower"]);

    // <[~ Modify ~]>

    QMenu* modify_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Modify"));
    modify_menu->setIcon(QIcon(":/icons/move_rotate.svg"));
    modify_menu->setObjectName("Modify");
    modify_menu->setTearOffEnabled(true);
    modify_menu->addMenu(order_menu);
    modify_menu->addActions(modify_actions);

    // <[~ Info ~]>

    QMenu* info_menu = tools_menu->addMenu(QC_ApplicationWindow::tr("&Info"));
    info_menu->setIcon(QIcon(":/icons/measure.svg"));
    info_menu->setObjectName("Info");
    info_menu->setTearOffEnabled(true);
    info_menu->addActions(info_actions);

    tools_menu->addAction(a_map["DrawMText"]);
    tools_menu->addAction(a_map["DrawText"]);
    tools_menu->addAction(a_map["DrawHatch"]);
    tools_menu->addAction(a_map["DrawPoint"]);

    // <[~ Layer ~]>

//    QMenu* layer_menu = new QMenu(QC_ApplicationWindow::tr("&Layer"), menu_bar);
//    layer_menu->setObjectName("layer_menu");
//    layer_menu->setTearOffEnabled(true);
//    layer_menu->addActions(layer_actions);

    // <[~ Block ~]>

//    QMenu* block_menu = new QMenu(QC_ApplicationWindow::tr("&Block"), menu_bar);
//    block_menu->setObjectName("block_menu");
//    block_menu->setTearOffEnabled(true);
//    block_menu->addActions(block_actions);

    // <[~ Snapping ~]>

//    QMenu* snap_menu = new QMenu(QC_ApplicationWindow::tr("Sna&p"), menu_bar);
//    snap_menu->setObjectName("snap_menu");
//    snap_menu->setTearOffEnabled(true);
//    // QToolBar* snap_tb = main_window->findChild<QToolBar*>("snap_toolbar");
//    snap_menu->addActions(snap_toolbar->actions());

    // <[~ Drawings ~]>

    windows_menu = new QMenu(QC_ApplicationWindow::tr("&Drawings"), menu_bar);
    windows_menu->setObjectName("drawings_menu");
    windows_menu->setTearOffEnabled(true);
    windows_menu->addAction(a_map["Fullscreen"]); // temp way to show this menu on OS X

    connect(windows_menu, SIGNAL(aboutToShow()),
            main_window, SLOT(slotWindowsMenuAboutToShow()));

    // <[~ Help ~]>

    QMenu* help_menu = new QMenu(QC_ApplicationWindow::tr("&Help"), menu_bar);
    help_menu->setObjectName("Help");
    help_menu->setTearOffEnabled(true);

    QAction* wiki_link = new QAction(QC_ApplicationWindow::tr("Online"), main_window);
    connect(wiki_link, SIGNAL(triggered()), main_window, SLOT(invokeLinkList()));
    help_menu->addAction(wiki_link);

    help_menu->addSeparator();

    QAction* help_about = new QAction(QIcon(":/main/librecad.png"), QC_ApplicationWindow::tr("About"), main_window);
    connect(help_about, SIGNAL(triggered()), main_window, SLOT(showAboutWindow()));
    help_menu->addAction(help_about);

    QAction* license = new QAction(QObject::tr("License"), main_window);
    connect(license, SIGNAL(triggered()), main_window, SLOT(invokeLicenseWindow()));
    help_menu->addAction(license);

    // <[~ Widgets Menu ~]>

    QMenu* widgets_menu = new QMenu("Widgets", menu_bar);
    widgets_menu->setTearOffEnabled(true);

    // <[~ Dock Areas Menu ~]>

    QMenu* dockareas_menu = new QMenu("Dock Areas", widgets_menu);

    dockareas_menu->addAction(a_map["LeftDockAreaToggle"]);
    dockareas_menu->addAction(a_map["RightDockAreaToggle"]);
    dockareas_menu->addAction(a_map["TopDockAreaToggle"]);
    dockareas_menu->addAction(a_map["BottomDockAreaToggle"]);
    dockareas_menu->addAction(a_map["FloatingDockwidgetsToggle"]);

    // <[~ Dock Widgets Menu ~]>

    QMenu* dockwidgets_menu = new QMenu(QC_ApplicationWindow::tr("Dock Wid&gets"), widgets_menu);
    dockwidgets_menu->setObjectName("dockwidgets_menu");
    dockwidgets_menu->setTearOffEnabled(true);

    dockwidgets_menu->addSeparator();

    QList<QDockWidget*> dockwidgets = main_window->findChildren<QDockWidget*>();

    main_window->sortWidgetsByTitle(dockwidgets);

    foreach (QDockWidget* dw, dockwidgets)
    {
        if (main_window->dockWidgetArea(dw) == Qt::RightDockWidgetArea)
            dockwidgets_menu->addAction(dw->toggleViewAction());
    }

    dockwidgets_menu->addSeparator();

    foreach (QDockWidget* dw, dockwidgets)
    {
        if (main_window->dockWidgetArea(dw) == Qt::LeftDockWidgetArea)
            dockwidgets_menu->addAction(dw->toggleViewAction());
    }

    // <[~ Toolbars Menu ~]>

    QMenu* toolbars_menu = new QMenu(QC_ApplicationWindow::tr("&Toolbars"), widgets_menu);
    toolbars_menu->setObjectName("toolbars_menu");
    toolbars_menu->setTearOffEnabled(true);

    QList<QToolBar*> toolbars = main_window->findChildren<QToolBar*>();

    main_window->sortWidgetsByTitle(toolbars);

    foreach (QToolBar* tb, toolbars)
    {
        toolbars_menu->addAction(tb->toggleViewAction());
    }

    widgets_menu->addMenu(dockareas_menu);
    widgets_menu->addMenu(dockwidgets_menu);
    widgets_menu->addMenu(toolbars_menu);
    widgets_menu->addAction(a_map["InvokeMenuCreator"]);
    widgets_menu->addAction(a_map["InvokeToolbarCreator"]);

    // <[~ MenuBar Layout~]>

    menu_bar->addMenu(file_menu);
    menu_bar->addMenu(settings_menu);
    menu_bar->addMenu(edit_menu);
    menu_bar->addMenu(view_menu);
    menu_bar->addMenu(plugins_menu);
//    menu_bar->addMenu(select_menu);
    menu_bar->addMenu(tools_menu);
//    menu_bar->addMenu(dimension_menu);
//    menu_bar->addMenu(modify_menu);
//    menu_bar->addMenu(snap_menu);
//    menu_bar->addMenu(info_menu);
//    menu_bar->addMenu(layer_menu);
//    menu_bar->addMenu(block_menu);
    menu_bar->addMenu(widgets_menu);
    menu_bar->addMenu(windows_menu);
    menu_bar->addMenu(help_menu);
}
