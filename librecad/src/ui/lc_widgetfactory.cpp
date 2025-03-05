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
#include "qg_coordinatewidget.h"
#include "qg_mousewidget.h"
#include "qg_activelayername.h"
#include "qg_selectionwidget.h"
#include "lc_relzerocoordinateswidget.h"
#include "twostackedlabels.h"

#include "rs_debug.h"
#include "rs_settings.h"
#include "lc_namedviewslistwidget.h"
#include "lc_namedviewsbutton.h"
#include "lc_ucslistwidget.h"
#include "lc_workspacelistbutton.h"

namespace {
    // only enable the penpallet by settings
    bool usePenPallet() {
        return LC_GET_ONE_BOOL("CustomToolbars", "UsePenPallet", true);
    }
} // namespace

LC_WidgetFactory::LC_WidgetFactory(QC_ApplicationWindow* main_win,
                                   LC_ActionGroupManager* agm)
    : QObject(nullptr)
    , main_window(main_win)
    , ag_manager(agm){

   fillActionLists();
}

void LC_WidgetFactory::fillActionLists(){
        ag_manager->fillActionsList(ag_manager->file_actions,  {
        "FileNew",
        "FileNewTemplate",
        "FileOpen",
        "FileSave",
        "FileSaveAs",
        "FileSaveAll"
    });

    ag_manager->fillActionsList(ag_manager->shape_actions, {
        "DrawLineRectangle",
        "DrawLineRectangle1Point",
        "DrawLineRectangle2Points",
        "DrawLineRectangle3Points",
        "DrawLinePolygonCenCor",
        "DrawLinePolygonCenTan",   //20161226 added by txmy
        "DrawLinePolygonCorCor",
        "DrawLinePolygonSideSide",
        "DrawStar"
    });

    ag_manager->fillActionsList(ag_manager->line_actions, {
        "DrawLine",
        "DrawLineAngle",
        "DrawLineHorizontal",
        "DrawLineVertical",
        "DrawLineParallelThrough",
        "DrawLineParallel",
        "DrawLineBisector",
        "DrawLineTangent1",
        "DrawLineTangent2",
        "DrawLineOrthTan",
        "DrawLineOrthogonal",
        "DrawLineRelAngle",
        "DrawLineRel",
        "DrawLineRelX",
        "DrawLineRelY",
        "DrawLineAngleRel",
        "DrawLineOrthogonalRel",
        "DrawLineFromPointToLine",
        "DrawSliceDivideLine",
        "DrawSliceDivideCircle",
        "DrawCross",
        "DrawLineMiddle"
    });

    ag_manager->fillActionsList(ag_manager->point_actions , {
        "DrawPoint",
        "DrawLinePoints",
        "DrawPointsMiddle",
        "DrawPointLattice",
        "SelectPoints",
        "PasteToPoints"
    });

    ag_manager->fillActionsList(ag_manager->circle_actions, {
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

    ag_manager->fillActionsList(ag_manager->curve_actions, {
        "DrawArc",
        "DrawArcChord",
        "DrawArcAngleLen",
        "DrawArc3P",
        "DrawArc2PAngle",
        "DrawArc2PRadius",
        "DrawArc2PLength",
        "DrawArc2PHeight",
        "DrawArcTangential",
        "DrawEllipseArcAxis",
        "DrawEllipseArc1Point"
    });

    ag_manager->fillActionsList(ag_manager->spline_actions, {
        "DrawParabola4Points",
        "DrawParabolaFD",
        "DrawSpline",
        "DrawSplinePoints",
        "DrawSplineFromPolyline",
        "DrawSplinePointsAppend",
        "DrawSplinePointsAdd",
        "DrawSplinePointsRemove",
        "DrawSplinePointsDelTwo",
        "DrawSplineExplode",
        "DrawLineFree"
    });

    ag_manager->fillActionsList(ag_manager->ellipse_actions, {
        "DrawEllipse1Point",
        "DrawEllipseAxis",
        "DrawEllipseFociPoint",
        "DrawEllipse4Points",
        "DrawEllipseCenter3Points",
        "DrawEllipseInscribe"
    });

    ag_manager->fillActionsList(ag_manager->polyline_actions, {
        "DrawPolyline",
        "PolylineAdd",
        "PolylineAppend",
        "PolylineDel",
        "PolylineDelBetween",
        "PolylineTrim",
        "PolylineEquidistant",
        "PolylineSegment",
        "PolylineArcToLines",
        "PolylineSegmentType"
    });

    ag_manager->fillActionsList(ag_manager->select_actions, {
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

    ag_manager->fillActionsList(ag_manager->dimension_actions, {
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

    ag_manager->fillActionsList(ag_manager->other_drawing_actions, {
        "DrawText",
        "DrawMText",
        "DrawHatch",
        "DrawImage",
        "DrawBoundingBox"
    });

    ag_manager->fillActionsList(ag_manager->modify_actions, {
        "ModifyMove",
        "ModifyDuplicate",
        "ModifyAlign",
        "ModifyAlignOne",
        "ModifyAlignRef",
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

    ag_manager->fillActionsList(ag_manager->order_actions, {
        "OrderTop",
        "OrderBottom",
        "OrderRaise",
        "OrderLower"
    });

    ag_manager->fillActionsList(ag_manager->info_actions, {
        "InfoDist",
        "InfoDist2",
        "InfoDist3",
        "InfoAngle",
        "InfoAngle3Points",
        "InfoTotalLength",
        "InfoArea",
        "EntityInfo"
    });

    ag_manager->fillActionsList(ag_manager->layer_actions, {
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

    ag_manager->fillActionsList(ag_manager->block_actions, {
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

    ag_manager->fillActionsList(ag_manager->pen_actions, {
        "PenSyncFromLayer",
        "PenPick",
        "PenPickResolved",
        "PenApply",
        "PenCopy"
    });

    ag_manager->fillActionsList(actionsToDisableInPrintPreview, {
        "EditCut",
        "EditCutQuick",
        "EditCopy",
        "EditCopyQuick",
        "EditPaste",
        "EditPasteTransform",
        "ViewGrid",
        "ViewDraft",
        "ViewLinesDraft",
        "ViewAntialiasing",
        "ModifyDeleteQuick",
        "EditKillAllActions",
        "ZoomIn",
        "ZoomOut",
        "ZoomAuto",
        "ZoomPrevious",
        "ZoomWindow",
        "ZoomPan",
        "OptionsDrawing",
        "ViewGridOrtho",
        "ViewGridIsoLeft",
        "ViewGridIsoTop",
        "ViewGridIsoRight",
        "UCSSetWCS",
        "UCSCreate"
    });

    actionsToDisableInPrintPreview.append(ag_manager->line_actions);
    actionsToDisableInPrintPreview.append(ag_manager->point_actions);
    actionsToDisableInPrintPreview.append(ag_manager->circle_actions);
    actionsToDisableInPrintPreview.append(ag_manager->curve_actions);
    actionsToDisableInPrintPreview.append(ag_manager->spline_actions);
    actionsToDisableInPrintPreview.append(ag_manager->ellipse_actions);
    actionsToDisableInPrintPreview.append(ag_manager->polyline_actions);
    actionsToDisableInPrintPreview.append(ag_manager->select_actions);
    actionsToDisableInPrintPreview.append(ag_manager->dimension_actions);
    actionsToDisableInPrintPreview.append(ag_manager->other_drawing_actions);
    actionsToDisableInPrintPreview.append(ag_manager->modify_actions);
    actionsToDisableInPrintPreview.append(ag_manager->order_actions);
    actionsToDisableInPrintPreview.append(ag_manager->info_actions);
    actionsToDisableInPrintPreview.append(ag_manager->block_actions);
    actionsToDisableInPrintPreview.append(ag_manager->pen_actions);
}

void LC_WidgetFactory::createLeftSidebar(int columns, int icon_size, bool flatButtons){
    auto* line = leftDocWidget(tr("Line"), "Line", ag_manager->line_actions, columns, icon_size, flatButtons);
    auto* point = leftDocWidget(tr("Point"), "Point", ag_manager->point_actions, columns, icon_size, flatButtons);
    auto* shape = leftDocWidget(tr("Polygon"), "Polygon", ag_manager->shape_actions, columns, icon_size, flatButtons);
    auto* circle = leftDocWidget(tr("Circle"), "Circle", ag_manager->circle_actions, columns, icon_size, flatButtons);
    auto* curve = leftDocWidget(tr("Arc"), "Curve", ag_manager->curve_actions, columns, icon_size, flatButtons);
    auto* spline = leftDocWidget(tr("Spline"), "Spline", ag_manager->spline_actions, columns, icon_size, flatButtons);
    auto* ellipse = leftDocWidget(tr("Ellipse"), "Ellipse", ag_manager->ellipse_actions, columns, icon_size, flatButtons);
    auto* polyline = leftDocWidget(tr("Polyline"), "Polyline", ag_manager->polyline_actions, columns, icon_size, flatButtons);
    auto* select = leftDocWidget(tr("Select"), "Select", ag_manager->select_actions, columns, icon_size, flatButtons);
    auto* dimension = leftDocWidget(tr("Dimension"), "Dimension", ag_manager->dimension_actions, columns, icon_size, flatButtons);
    auto* other = leftDocWidget(tr("Other"), "Other", ag_manager->other_drawing_actions, columns, icon_size, flatButtons);
    auto* modify = leftDocWidget(tr("Modify"), "Modify", ag_manager->modify_actions, columns, icon_size, flatButtons);
    auto* info = leftDocWidget(tr("Info"), "Info", ag_manager->info_actions, columns, icon_size, flatButtons);
    auto* order = leftDocWidget(tr("Order"), "Order", ag_manager->order_actions, columns, icon_size, flatButtons);

    main_window->addDockWidget(Qt::LeftDockWidgetArea, line);
    main_window->tabifyDockWidget(line, polyline);
    main_window->tabifyDockWidget(polyline, point);
    main_window->tabifyDockWidget(polyline, shape);
    line->raise();
    main_window->addDockWidget(Qt::LeftDockWidgetArea, circle);
    main_window->tabifyDockWidget(circle, curve);
    main_window->tabifyDockWidget(curve, spline);
    main_window->tabifyDockWidget(spline, ellipse);
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
        connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, pen_palette, &LC_PenPaletteWidget::updateWidgetSettings);
        dock_pen_palette ->setWidget(pen_palette);
    }
    auto* dock_layer = new QDockWidget(main_window);
    dock_layer->setWindowTitle(tr("Layer List"));
    dock_layer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_layer->setObjectName("layer_dockwidget");
    layer_widget = new QG_LayerWidget(action_handler, dock_layer, "Layer");
    layer_widget->setFocusPolicy(Qt::NoFocus);
    connect(layer_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
    dock_layer->setWidget(layer_widget);

    connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, layer_widget, &QG_LayerWidget::updateWidgetSettings);

    auto* dock_views = new QDockWidget(main_window);
    dock_views->setWindowTitle(tr("Named Views"));
    dock_views->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_views->setObjectName("view_dockwidget");
    named_views_widget = new LC_NamedViewsListWidget("View", dock_views);
    named_views_widget->setFocusPolicy(Qt::NoFocus);
    dock_views->setWidget(named_views_widget);

    connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, named_views_widget, &LC_NamedViewsListWidget::updateWidgetSettings);

    auto* dock_ucss = new QDockWidget(main_window);
    dock_ucss->setWindowTitle(tr("UCSs"));
    dock_ucss->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_ucss->setObjectName("ucs_dockwidget");
    ucs_widget = new LC_UCSListWidget("UCS", dock_ucss);
    ucs_widget->setFocusPolicy(Qt::NoFocus);
    dock_ucss->setWidget(ucs_widget);

    connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, ucs_widget, &LC_UCSListWidget::updateWidgetSettings);
    connect(ucs_widget, &LC_UCSListWidget::ucsListChanged, named_views_widget, &LC_NamedViewsListWidget::onUcsListChanged);

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

        connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, layer_tree_widget, &LC_LayerTreeWidget::updateWidgetSettings);
    }

    QDockWidget* dock_quick_info = nullptr;
    dock_quick_info = new QDockWidget(main_window);
    dock_quick_info->setWindowTitle(tr("Entity Info"));
    dock_quick_info->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_quick_info->setObjectName("quick_entity_info");
    quick_info_widget = new LC_QuickInfoWidget(dock_quick_info, ag_manager->getActionsMap());
    quick_info_widget->setFocusPolicy(Qt::NoFocus);
    quick_info_widget->setVisible(false);
    dock_quick_info->setWidget(quick_info_widget);

    connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, quick_info_widget, &LC_QuickInfoWidget::updateWidgetSettings);

    auto* dock_block = new QDockWidget(main_window);
    dock_block->setWindowTitle(tr("Block List"));
    dock_block->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_block->setObjectName("block_dockwidget");
    block_widget = new QG_BlockWidget(action_handler, dock_block, "Block");
    block_widget->setFocusPolicy(Qt::NoFocus);
    connect(block_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
    connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, block_widget, &QG_BlockWidget::updateWidgetSettings);
    dock_block->setWidget(block_widget);

    auto* dock_library = new QDockWidget(main_window);
    dock_library->setWindowTitle(tr("Library Browser"));
    dock_library->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_library->setObjectName("library_dockwidget");
    library_widget = new QG_LibraryWidget(dock_library, "Library");
    library_widget->setActionHandler(action_handler);
    library_widget->setFocusPolicy(Qt::NoFocus);
    connect(library_widget, SIGNAL(escape()), main_window, SLOT(slotFocus()));
    connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, library_widget, &QG_LibraryWidget::updateWidgetSettings);
    dock_library->setWidget(library_widget);
    dock_library->resize(240, 400);

    auto* dock_command = new QDockWidget(tr("Command line"), main_window);
    // dock_command->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    dock_command->setObjectName("command_dockwidget");
    command_widget = new QG_CommandWidget(dock_command, "Command");
    command_widget->setActionHandler(action_handler);
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
    main_window->addDockWidget(Qt::RightDockWidgetArea, dock_views);
    main_window->tabifyDockWidget(dock_views, dock_ucss);
    main_window->addDockWidget(Qt::RightDockWidgetArea, dock_command);
    command_widget->getDockingAction()->setText(dock_command->isFloating() ? tr("Dock") : tr("Float"));
}

void LC_WidgetFactory::createStandardToolbars(QG_ActionHandler* action_handler){
    QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *file = createGenericToolbar(tr("File"), "file", tbPolicy, {});
    file->addActions(ag_manager->file_actions);
    file->addAction(ag_manager->getActionByName("FilePrint"));
    file->addAction(ag_manager->getActionByName("FilePrintPreview"));

    auto *edit = createGenericToolbar(tr("Edit"), "Edit", tbPolicy, {
        "EditKillAllActions", "EntityDescriptionInfo", "", "EditUndo", "EditRedo", "", "EditCut", "EditCopy", "EditPaste", "EditPasteTransform"
    });

    auto *order = createGenericToolbar(tr("Order"), "Order", tbPolicy, {
        "OrderTop", "OrderBottom", "OrderRaise", "OrderLower"
    });
    order->hide();

    auto *view = createGenericToolbar(tr("View"), "View", tbPolicy, {
        "ViewGrid", "ViewDraft", "ViewLinesDraft", "ViewAntialiasing", "", "ZoomRedraw", "ZoomIn",
        "ZoomOut", "ZoomAuto", "ZoomPrevious", "ZoomWindow", "ZoomPan"
    });

    auto *viewsList = createNamedViewsToolbar(tr("Named Views"), "Views", tbPolicy);

    auto *ucsList = createUCSToolbar(tr("UCS"), "UCS", tbPolicy);

    auto *perspectivesToolbar = createWorkspacesToolbar(tr("Workspaces"), "Workspaces", tbPolicy);

    snap_toolbar = new QG_SnapToolBar(main_window, action_handler, ag_manager,ag_manager->getActionsMap());
    snap_toolbar->setWindowTitle(tr("Snap Selection"));
    snap_toolbar->setSizePolicy(tbPolicy);
    snap_toolbar->setObjectName("snap_toolbar" );
    snap_toolbar->setProperty("_group", 3);

    action_handler->set_snap_toolbar(snap_toolbar);

    connect( main_window,  &QC_ApplicationWindow::signalEnableRelativeZeroSnaps,
             snap_toolbar, &QG_SnapToolBar::slotEnableRelativeZeroSnaps);


//    snap_toolbar = new QG_SnapToolBar(main_window, action_handler, ag_manager,ag_manager->getActionsMap());

    pen_toolbar = new QG_PenToolBar(tr("Pen"), main_window);
    pen_toolbar->setSizePolicy(tbPolicy);
    pen_toolbar->setObjectName("pen_toolbar");
    pen_toolbar->addActions(ag_manager->pen_actions);
    pen_toolbar->setProperty("_group", 0);

    options_toolbar = createGenericToolbar(tr("Tool Options"), "Tool Options", tbPolicy, {});

//    edit->addAction("InfoCursorEnable")

    createInfoCursorToolbar(tbPolicy);

    auto *dockareas = createGenericToolbar(tr("Dock Areas"), "Dock Areas", tbPolicy, {
        "LeftDockAreaToggle", "RightDockAreaToggle", "TopDockAreaToggle",
        "BottomDockAreaToggle", "FloatingDockwidgetsToggle"
    });

    auto *creators = createGenericToolbar(tr("Creators"), "Creators", tbPolicy, {
        "InvokeMenuCreator", "InvokeToolbarCreator"
    });

    auto *preferences = createGenericToolbar(tr("Preferences"), "Preferences", tbPolicy, {
        "OptionsGeneral", "OptionsDrawing"
    });

    addToTop(file);
    addToTop(edit);
    addToTop(view);
    addToTop(perspectivesToolbar);
    addToTop(viewsList);
    addToTop(ucsList);
    addToTop(preferences);
    main_window->addToolBarBreak();
    addToTop(pen_toolbar);
    addToTop(options_toolbar);

    addToLeft(order);

    addToBottom(snap_toolbar);
    addToBottom(dockareas);
    addToBottom(creators);
}

void LC_WidgetFactory::createInfoCursorToolbar(QSizePolicy &tbPolicy) {
    auto infoCursorTB = createGenericToolbar(tr("Info Cursor"), "Info Cursor", tbPolicy, {
        "InfoCursorEnable"
    });

    QAction* action = ag_manager->getActionByName("InfoCursorEnable");
    action->setProperty("InfoCursorActionTag", 0);
    connect(action, &QAction::triggered, main_window, &QC_ApplicationWindow::slotInfoCursorSetting);
    if (action != nullptr){
        QWidget* w = infoCursorTB->widgetForAction(action);
        if (w != nullptr){
            auto* btn = dynamic_cast<QToolButton *>(w);

            if (btn != nullptr){
                btn->setPopupMode(QToolButton::MenuButtonPopup);
                auto* menu = new QMenu();

                addInfoCursorOptionAction(menu, "InfoCursorAbs", 1);
                addInfoCursorOptionAction(menu, "InfoCursorSnap", 2);
                addInfoCursorOptionAction(menu, "InfoCursorRel", 3);
                addInfoCursorOptionAction(menu, "InfoCursorPrompt", 4);
                addInfoCursorOptionAction(menu, "InfoCursorCatchedEntity", 5);

                btn->setMenu(menu);
            }
        }
    }
    addToTop(infoCursorTB);
}

void LC_WidgetFactory::addInfoCursorOptionAction(QMenu *menu, const char *name, int tag) {
    QAction* action = ag_manager->getActionByName(name);
    action->setProperty("InfoCursorActionTag", tag);
    menu->addAction(action);
    connect(action, &QAction::triggered, main_window, &QC_ApplicationWindow::slotInfoCursorSetting);
}

void LC_WidgetFactory::createCADToolbars(){
    QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* line = toolbarWithActions(tr("Line"), "Line", tbPolicy, ag_manager->line_actions);
    auto* point = toolbarWithActions(tr("Point"), "Point", tbPolicy, ag_manager->point_actions);
    auto* shape = toolbarWithActions(tr("Polygon"), "Polygon", tbPolicy, ag_manager->shape_actions);
    auto* circle = toolbarWithActions(tr("Circle"), "Circle", tbPolicy, ag_manager->circle_actions);
    auto* curve = toolbarWithActions(tr("Arc"), "Curve", tbPolicy, ag_manager->curve_actions);
    auto* spline = toolbarWithActions(tr("Spline"), "Spline", tbPolicy, ag_manager->curve_actions);
    auto* ellipse = toolbarWithActions(tr("Ellipse"), "Ellipse", tbPolicy, ag_manager->ellipse_actions);
    auto* polyline = toolbarWithActions(tr("Polyline"), "Polyline", tbPolicy, ag_manager->polyline_actions);
    auto* select = toolbarWithActions(tr("Select"), "Select", tbPolicy, ag_manager->select_actions);
    auto* dimension = toolbarWithActions(tr("Dimension"), "Dimension", tbPolicy, ag_manager->dimension_actions);
    auto* other = toolbarWithActions(tr("Other"), "other_drawing", tbPolicy, ag_manager->other_drawing_actions);
    auto* modify = toolbarWithActions(tr("Modify"), "Modify", tbPolicy, ag_manager->modify_actions);
    auto* info = toolbarWithActions(tr("Info"), "Info", tbPolicy, ag_manager->info_actions);

    addToBottom(line);
    addToBottom(point);
    addToBottom(shape);
    addToBottom(circle);
    addToBottom(curve);
    addToBottom(spline);
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

    toolButton(toolbar, tr("Lines"), ":/icons/line.lci", ag_manager->line_actions);
    toolButton(toolbar, tr("Points"), ":/icons/points.lci", ag_manager->point_actions);
    toolButton(toolbar, tr("Polygons"), ":/icons/circle.lci", ag_manager->circle_actions);
    toolButton(toolbar, tr("Arcs"), ":/icons/arc_center_point_angle.lci", ag_manager->curve_actions);
    toolButton(toolbar, tr("Splines"), ":/icons/spline_points.lci", ag_manager->spline_actions);
    toolButton(toolbar, tr("Polygons"), ":/icons/rectangle_2_points.lci", ag_manager->shape_actions);
    toolButton(toolbar, tr("Ellipses"), ":/icons/ellipses.lci",ag_manager->ellipse_actions);
    toolButton(toolbar, tr("PolyLines"), ":/icons/polylines.lci", ag_manager->polyline_actions);
    toolButton(toolbar, tr("Select"), ":/icons/select.lci", ag_manager->select_actions);
    toolButton(toolbar, tr("Dimensions"), ":/icons/dim_horizontal.lci", ag_manager->dimension_actions);
    toolButton(toolbar, tr("Other"), ":/icons/text.lci", ag_manager->other_drawing_actions);
    toolButton(toolbar, tr("Modify"), ":/icons/move_rotate.lci", ag_manager->modify_actions);
    toolButton(toolbar, tr("Measure"), ":/icons/measure.lci", ag_manager->info_actions);
    toolButton(toolbar, tr("Order"), ":/icons/order.lci", ag_manager->order_actions);

    toolbar->setProperty("_group", 1);

    addToLeft(toolbar);
    return toolbar;
}
void LC_WidgetFactory::makeActionsInvisible(const std::vector<QString> &actionNames){
    for (const QString& actionName: actionNames) {
        QAction *action = ag_manager->getActionByName(actionName);
        if (action != nullptr) {
            action->setVisible(false);
        }
    }
}

void LC_WidgetFactory::addAction(QToolBar* toolbar, const char* actionName){
    QAction *action = ag_manager->getActionByName(actionName);
    if (action != nullptr) {
        toolbar->addAction(action);
    }
}

QToolBar* LC_WidgetFactory::createNamedViewsToolbar(const QString& title, const QString& name, QSizePolicy toolBarPolicy){
    QToolBar * result = doCreateToolBar(title, name, toolBarPolicy);

    QAction *saveViewAction = ag_manager->getActionByName("ZoomViewSave");
    result->addAction(saveViewAction);

    QAction *restoreCurrentViewAction = ag_manager->getActionByName("ZoomViewRestore");

    auto namedViewsSelectionWidget = named_views_widget->createSelectionWidget(saveViewAction, restoreCurrentViewAction);
    namedViewsSelectionWidget->setParent(result);
    result->addWidget(namedViewsSelectionWidget);

    return result;
}

QToolBar* LC_WidgetFactory::createUCSToolbar(const QString& title, const QString& name, QSizePolicy toolBarPolicy){
    QToolBar * result = doCreateToolBar(title, name, toolBarPolicy);

    QAction *ucsCreateAction = ag_manager->getActionByName("UCSCreate");
    result->addAction(ucsCreateAction);

    QAction *setWCSAction = ag_manager->getActionByName("UCSSetWCS");

    auto ucsSelectionWidget = ucs_widget->createSelectionWidget(ucsCreateAction, setWCSAction);
    ucsSelectionWidget->setParent(result);
    result->addWidget(ucsSelectionWidget);

    setWCSAction->setCheckable(false);
    connect(setWCSAction, &QAction::triggered, ucs_widget, &LC_UCSListWidget::setWCS);

    ucs_widget->setStateWidget(main_window->ucsStateWidget);

    return result;
}

QToolBar* LC_WidgetFactory::createWorkspacesToolbar(const QString& title, const QString& name, QSizePolicy toolBarPolicy){
    auto * result = doCreateToolBar(title, name, toolBarPolicy);

    auto* toolButton = new QToolButton(result);
    auto *createAction = ag_manager->getActionByName("WorkspaceCreate");

    toolButton->setDefaultAction(createAction);
    QAction *removeAction = ag_manager->getActionByName("WorkspaceRemove");
    toolButton->addAction(removeAction);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);

    result->addWidget(toolButton);

    auto* workspacesListButton = new LC_WorkspaceListButton(main_window);
    auto restoreAction = ag_manager->getActionByName("WorkspaceRestore");
    workspacesListButton->setDefaultAction(restoreAction);
    result->addWidget(workspacesListButton);
    return result;
}

QToolBar* LC_WidgetFactory::createGenericToolbar(const QString& title, const QString &name, QSizePolicy toolBarPolicy, const std::vector<QString> &actionNames){

    QToolBar * result = doCreateToolBar(title, name, toolBarPolicy);

    for (const QString& actionName: actionNames){
        if (actionName.isEmpty()){
            result->addSeparator();
        }
        else{
            result->addAction(ag_manager->getActionByName(actionName));
        }
    }
    result->setProperty("_group", 0);
    return result;
}

QToolBar *LC_WidgetFactory::doCreateToolBar(const QString &title, const QString &name, const QSizePolicy &toolBarPolicy) const {
    auto* result = new QToolBar(title, main_window);
    result->setSizePolicy(toolBarPolicy);
    QString nameCleared(name);
    nameCleared.remove(' ');
    const QString &objectName = nameCleared.toLower() + "_toolbar";
    result->setObjectName(objectName);
    return result;
}

QToolBar* LC_WidgetFactory::toolbarWithActions(const QString& title, const QString& name, QSizePolicy toolBarPolicy, const QList<QAction*> &actions){
    QToolBar * result = doCreateToolBar(title, name, toolBarPolicy);

    result->addActions(actions);
    result->hide();
    result->setProperty("_group", 1);
    return result;
}



LC_DockWidget* LC_WidgetFactory::leftDocWidget(const QString& title, const char* name, const QList<QAction*> &actions, int columns, int iconSize, bool flatButtons){
    auto* result = new LC_DockWidget(main_window);
    result->setObjectName("dock_" + QString(name).toLower());
    result->setWindowTitle(title);
    result->add_actions(actions, columns, iconSize, flatButtons);
    result->hide();

    connect(main_window, &QC_ApplicationWindow::widgetSettingsChanged, result, &LC_DockWidget::updateWidgetSettings);

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

void LC_WidgetFactory::initStatusBar() {
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init status bar");
    QStatusBar* status_bar = main_window->statusBar();
    main_window->coordinateWidget = new QG_CoordinateWidget(status_bar, "coordinates");
    main_window->relativeZeroCoordinatesWidget = new LC_RelZeroCoordinatesWidget(status_bar, "relZeroCordinates");
    main_window->mouseWidget = new QG_MouseWidget(status_bar, "mouse info");
    main_window->selectionWidget = new QG_SelectionWidget(status_bar, "selections");
    main_window->m_pActiveLayerName = new QG_ActiveLayerName(status_bar);

    main_window->grid_status = new TwoStackedLabels(status_bar);
    main_window->grid_status->setTopLabel(tr("Grid Status"));

    auto* ucsStateWidget = new LC_UCSStateWidget(status_bar, "ucs");
    main_window->ucsStateWidget = ucsStateWidget;

    auto* anglesBasisWidget = new LC_AnglesBasisWidget(status_bar, "anglesBase");
    main_window->anglesBasisWidget = anglesBasisWidget;

    main_window->statusbarManager = new LC_QTStatusbarManager(status_bar);
    main_window->statusbarManager->loadSettings();

    bool useClassicalStatusBar = LC_GET_ONE_BOOL("Startup", "UseClassicStatusBar", false);
    if (useClassicalStatusBar) {
        status_bar->addWidget(main_window->coordinateWidget);
        status_bar->addWidget(main_window->mouseWidget);
        status_bar->addWidget(main_window->selectionWidget);
        status_bar->addWidget(main_window->m_pActiveLayerName);
        status_bar->addWidget(main_window->grid_status);
        status_bar->addWidget(main_window->relativeZeroCoordinatesWidget);
        status_bar->addWidget(main_window->ucsStateWidget);
        status_bar->addWidget(main_window->anglesBasisWidget);

        LC_GROUP_GUARD("Widgets");{
            bool allow_statusbar_fontsize = LC_GET_BOOL("AllowStatusbarFontSize", false);
            bool allow_statusbar_height = LC_GET_BOOL("AllowStatusbarHeight", false);

            if (allow_statusbar_fontsize) {
                int fontsize = LC_GET_INT("StatusbarFontSize", 12);
                QFont font;
                font.setPointSize(fontsize);
                status_bar->setFont(font);
            }
            int height{64};
            if (allow_statusbar_height) {
                height = LC_GET_INT("StatusbarHeight", 64);
            }
            status_bar->setMinimumHeight(height);
            status_bar->setMaximumHeight(height);
        }
    }
    else {
        QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        auto *tb = new QToolBar(tr("Coordinates"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->addWidget(main_window->coordinateWidget);
        tb->setObjectName("TBCoordinates");
        tb->setProperty("_group", 3);
        addToBottom(tb);

        tb = new QToolBar(tr("Relative Zero"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->addWidget(main_window->relativeZeroCoordinatesWidget);
        tb->setObjectName("TBRelZero");
        tb->setProperty("_group", 3);
        addToBottom(tb);

        tb = new QToolBar(tr("Mouse"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->setObjectName("TBMouse");
        tb->addWidget(main_window->mouseWidget);
        tb->setProperty("_group", 3);
        addToBottom(tb);

        tb = new QToolBar(tr("Selection Info"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->setObjectName("TBSelection");
        tb->addWidget(main_window->selectionWidget);
        tb->setProperty("_group", 3);
        addToBottom(tb);

        tb = new QToolBar(tr("Active Layer"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->setObjectName("TBActiveLayer");
        tb->addWidget(main_window->m_pActiveLayerName);
        tb->setProperty("_group", 3);
        addToBottom(tb);

        tb = new QToolBar(tr("Grid Status"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->setObjectName("TBGridStatus");
        tb->addWidget(main_window->grid_status);
        tb->setProperty("_group", 3);
        addToBottom(tb);

        tb = new QToolBar(tr("UCS Status"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->setObjectName("TBUCSStatus");
        tb->addWidget(main_window->ucsStateWidget);
        tb->setProperty("_group", 3);
        addToBottom(tb);

        tb = new QToolBar(tr("Angles Basis"), main_window);
        tb->setSizePolicy(tbPolicy);
        tb->setObjectName("TBAnglesBasis");
        tb->addWidget(main_window->anglesBasisWidget);
        tb->setProperty("_group", 3);
        addToBottom(tb);

        main_window->statusbarManager->setup();

        main_window->grid_status->setToolTip(tr("Current size of Grid/MetaGrid. Click to change grid size."));
        connect(main_window->grid_status, &TwoStackedLabels::clicked, main_window, &QC_ApplicationWindow::slotShowDrawingOptions);

    }
    connect(main_window->anglesBasisWidget, &LC_AnglesBasisWidget::clicked, main_window, &QC_ApplicationWindow::slotShowDrawingOptionsUnits);

    connect(main_window, &QC_ApplicationWindow::iconsRefreshed, main_window->ucsStateWidget, &LC_UCSStateWidget::onIconsRefreshed);
    connect(main_window, &QC_ApplicationWindow::iconsRefreshed, main_window->anglesBasisWidget, &LC_AnglesBasisWidget::onIconsRefreshed);
    connect(main_window, &QC_ApplicationWindow::iconsRefreshed, main_window->mouseWidget, &QG_MouseWidget::onIconsRefreshed);
}


void LC_WidgetFactory::addToTop(QToolBar *toolbar) { main_window->addToolBar(Qt::TopToolBarArea, toolbar); }
void LC_WidgetFactory::addToBottom(QToolBar *toolbar) { main_window->addToolBar(Qt::BottomToolBarArea, toolbar); }
void LC_WidgetFactory::addToLeft(QToolBar *toolbar) { main_window->addToolBar(Qt::LeftToolBarArea, toolbar); }
