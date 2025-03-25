// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//

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
#include "lc_penwizard.h"
#include "lc_ucslistwidget.h"


LC_WidgetFactory::LC_WidgetFactory(QC_ApplicationWindow* main_win)
    : QObject(nullptr)
    , m_mainWin(main_win)
    , m_agm(main_win->m_actionGroupManager){
   fillActionLists();
}

void LC_WidgetFactory::fillActionLists(){
     m_agm->fillActionsList(actionsToDisableInPrintPreview, {
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

    actionsToDisableInPrintPreview.append(m_agm->line_actions);
    actionsToDisableInPrintPreview.append(m_agm->point_actions);
    actionsToDisableInPrintPreview.append(m_agm->circle_actions);
    actionsToDisableInPrintPreview.append(m_agm->curve_actions);
    actionsToDisableInPrintPreview.append(m_agm->spline_actions);
    actionsToDisableInPrintPreview.append(m_agm->ellipse_actions);
    actionsToDisableInPrintPreview.append(m_agm->polyline_actions);
    actionsToDisableInPrintPreview.append(m_agm->select_actions);
    actionsToDisableInPrintPreview.append(m_agm->dimension_actions);
    actionsToDisableInPrintPreview.append(m_agm->other_drawing_actions);
    actionsToDisableInPrintPreview.append(m_agm->modify_actions);
    actionsToDisableInPrintPreview.append(m_agm->order_actions);
    actionsToDisableInPrintPreview.append(m_agm->info_actions);
    actionsToDisableInPrintPreview.append(m_agm->block_actions);
    actionsToDisableInPrintPreview.append(m_agm->pen_actions);
}

void LC_WidgetFactory::initWidgets(){
    initStatusBar();
    initLeftCADSidebar();
    createRightSidebar(m_mainWin->m_actionHandler);
}

void LC_WidgetFactory::initLeftCADSidebar(){
    bool enable_left_sidebar = LC_GET_ONE_BOOL("Startup", "EnableLeftSidebar", true);
    if (enable_left_sidebar) {
        LC_GROUP("Widgets"); {
            int leftSidebarColumnsCount = LC_GET_INT("LeftToolbarColumnsCount", 5);
            int leftSidebarIconSize = LC_GET_INT("LeftToolbarIconSize", 24);
            bool flatIcons = LC_GET_BOOL("LeftToolbarFlatIcons", true);
            createLeftSidebar(leftSidebarColumnsCount, leftSidebarIconSize, flatIcons);
        }
        LC_GROUP_END();
    }
}

void LC_WidgetFactory::createLeftSidebar(int columns, int icon_size, bool flatButtons){
    auto* line = leftDockWidget(tr("Line"), "Line", m_agm->line_actions, columns, icon_size, flatButtons);
    auto* point = leftDockWidget(tr("Point"), "Point", m_agm->point_actions, columns, icon_size, flatButtons);
    auto* shape = leftDockWidget(tr("Polygon"), "Polygon", m_agm->shape_actions, columns, icon_size, flatButtons);
    auto* circle = leftDockWidget(tr("Circle"), "Circle", m_agm->circle_actions, columns, icon_size, flatButtons);
    auto* curve = leftDockWidget(tr("Arc"), "Curve", m_agm->curve_actions, columns, icon_size, flatButtons);
    auto* spline = leftDockWidget(tr("Spline"), "Spline", m_agm->spline_actions, columns, icon_size, flatButtons);
    auto* ellipse = leftDockWidget(tr("Ellipse"), "Ellipse", m_agm->ellipse_actions, columns, icon_size, flatButtons);
    auto* polyline = leftDockWidget(tr("Polyline"), "Polyline", m_agm->polyline_actions, columns, icon_size, flatButtons);
    auto* select = leftDockWidget(tr("Select"), "Select", m_agm->select_actions, columns, icon_size, flatButtons);
    auto* dimension = leftDockWidget(tr("Dimension"), "Dimension", m_agm->dimension_actions, columns, icon_size, flatButtons);
    auto* other = leftDockWidget(tr("Other"), "Other", m_agm->other_drawing_actions, columns, icon_size, flatButtons);
    auto* modify = leftDockWidget(tr("Modify"), "Modify", m_agm->modify_actions, columns, icon_size, flatButtons);
    auto* info = leftDockWidget(tr("Info"), "Info", m_agm->info_actions, columns, icon_size, flatButtons);
    auto* order = leftDockWidget(tr("Order"), "Order", m_agm->order_actions, columns, icon_size, flatButtons);

    m_mainWin->addDockWidget(Qt::LeftDockWidgetArea, line);
    m_mainWin->tabifyDockWidget(line, polyline);
    m_mainWin->tabifyDockWidget(polyline, point);
    m_mainWin->tabifyDockWidget(polyline, shape);
    line->raise();
    m_mainWin->addDockWidget(Qt::LeftDockWidgetArea, circle);
    m_mainWin->tabifyDockWidget(circle, curve);
    m_mainWin->tabifyDockWidget(curve, spline);
    m_mainWin->tabifyDockWidget(spline, ellipse);
    circle->raise();
    m_mainWin->addDockWidget(Qt::LeftDockWidgetArea, dimension);
    m_mainWin->tabifyDockWidget(dimension, other);
    m_mainWin->tabifyDockWidget(other, info);
    m_mainWin->tabifyDockWidget(info, select);
    dimension->raise();
    m_mainWin->addDockWidget(Qt::LeftDockWidgetArea, modify);
    m_mainWin->tabifyDockWidget(modify, order);
}

QDockWidget* LC_WidgetFactory::newDockWidget(const QString& title, const char *name){
    auto result = new QDockWidget(m_mainWin);
    result->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    result->setWindowTitle(title);
    result->setObjectName(name);
    return result;
}

QDockWidget* LC_WidgetFactory::createPenPalletteWidget(){
    auto dock = newDockWidget(tr("Pen Palette"), "pen_palette_dockwidget");
    auto widget = new LC_PenPaletteWidget("PenPalette", dock);
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(widget, &LC_PenPaletteWidget::escape, m_mainWin, &QC_ApplicationWindow::slotFocus);
    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_PenPaletteWidget::updateWidgetSettings);

    m_mainWin->m_penPaletteWidget = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createLayerWidget(QG_ActionHandler *action_handler){
    auto dock = newDockWidget(tr("Layer List"), "layer_dockwidget");
    auto widget = new QG_LayerWidget(action_handler, dock, "Layer");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(widget, &QG_LayerWidget::escape, m_mainWin, &QC_ApplicationWindow::slotFocus);
    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &QG_LayerWidget::updateWidgetSettings);

    m_mainWin->m_layerWidget  = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createNamedViewsWidget(){
    auto dock   = newDockWidget(tr("Named Views"), "view_dockwidget");
    auto widget = new LC_NamedViewsListWidget("View", dock);
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_NamedViewsListWidget::updateWidgetSettings);
    connect(m_mainWin->m_ucsListWidget, &LC_UCSListWidget::ucsListChanged, widget, &LC_NamedViewsListWidget::onUcsListChanged);

    m_mainWin->m_namedViewsWidget = widget;

    QC_ApplicationWindow *win = m_mainWin;

    connect(widget, &LC_NamedViewsListWidget::viewListChanged, [win](int itemsCount){
        win->enableAction("ZoomViewRestore1", itemsCount > 0);
        win->enableAction("ZoomViewRestore2", itemsCount > 1);
        win->enableAction("ZoomViewRestore3", itemsCount > 2);
        win->enableAction("ZoomViewRestore4", itemsCount > 3);
        win->enableAction("ZoomViewRestore5", itemsCount > 4);
    });
    return dock;
}

QDockWidget*  LC_WidgetFactory::createUCSListWidget(){
    auto dock = newDockWidget(tr("UCSs"), "ucs_dockwidget");
    auto widget = new LC_UCSListWidget("UCS", dock);
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_UCSListWidget::updateWidgetSettings);

    m_mainWin->m_ucsListWidget = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createLayerTreeWidget(QG_ActionHandler *action_handler){
    QDockWidget* dock = newDockWidget(tr("Layer Tree"), "layer_tree_dockwidget");
    auto widget = new LC_LayerTreeWidget(action_handler, dock, "Layer Tree");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    // layer_tree_widget->setVisible(false);

    connect(widget, &LC_LayerTreeWidget::escape, m_mainWin, &QC_ApplicationWindow::slotFocus);
    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_LayerTreeWidget::updateWidgetSettings);

    m_mainWin->m_layerTreeWidget = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createEntityInfoWidget(){
    QDockWidget* dock = newDockWidget(tr("Entity Info"), "quick_entity_info");
    auto widget = new LC_QuickInfoWidget(dock, m_agm->getActionsMap());
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    // quick_info_widget->setVisible(false);

    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_QuickInfoWidget::updateWidgetSettings);

    m_mainWin->m_quickInfoWidget = widget;
    return dock;
}

QDockWidget*  LC_WidgetFactory::createBlockListWidget(QG_ActionHandler *action_handler){
    auto dock =  newDockWidget(tr("Block List"), "block_dockwidget");

    auto widget = new QG_BlockWidget(action_handler, dock, "Block");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(widget, &QG_BlockWidget::escape, m_mainWin, &QC_ApplicationWindow::slotFocus);
    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &QG_BlockWidget::updateWidgetSettings);

    m_mainWin->m_blockWidget = widget;
    return dock;
}

QDockWidget*  LC_WidgetFactory::createLibraryWidget(QG_ActionHandler *action_handler){
    auto dock = newDockWidget(tr("Library Browser"), "library_dockwidget");

    auto widget = new QG_LibraryWidget(action_handler, dock, "Library");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    // result->resize(240, 400);

    connect(widget, QG_LibraryWidget::escape, m_mainWin, &QC_ApplicationWindow::slotFocus);
    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &QG_LibraryWidget::updateWidgetSettings);

    m_mainWin->m_libraryWidget = widget;
    return dock;
}

QDockWidget * LC_WidgetFactory::createCmdWidget(QG_ActionHandler *action_handler){
    auto dock = newDockWidget(tr("Command line"), "command_dockwidget");

    auto widget = new QG_CommandWidget(action_handler, dock, "Command");
    widget->setActionHandler(action_handler);

    dock->setWidget(widget);
    widget->getDockingAction()->setText(dock->isFloating() ? tr("Dock") : tr("Float"));

    connect(widget->leCommand, &QG_CommandEdit::escape, m_mainWin, &QC_ApplicationWindow::slotFocus);
    connect(dock, &QDockWidget::dockLocationChanged,m_mainWin, &QC_ApplicationWindow::modifyCommandTitleBar);

    m_mainWin->m_commandWidget = widget;
    return dock;
}

void LC_WidgetFactory::createRightSidebar(QG_ActionHandler* action_handler){
    createPenWizardWidget();
    QDockWidget *dock_pen_palette = createPenPalletteWidget();
    QDockWidget *dock_layer = createLayerWidget(action_handler);
    QDockWidget *dock_views = createNamedViewsWidget();
    QDockWidget *dock_ucss = createUCSListWidget();
    QDockWidget *dock_layer_tree = createLayerTreeWidget(action_handler);
    QDockWidget *dock_quick_info = createEntityInfoWidget();
    QDockWidget *dock_block = createBlockListWidget(action_handler);
    QDockWidget *dock_library = createLibraryWidget(action_handler);
    QDockWidget *dock_command = createCmdWidget(action_handler);

    m_mainWin->setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks );

    m_mainWin->addDockWidget(Qt::RightDockWidgetArea, dock_library);
    m_mainWin->tabifyDockWidget(dock_library, dock_block);
    m_mainWin->tabifyDockWidget(dock_block, dock_layer);
    m_mainWin->tabifyDockWidget(dock_block, dock_quick_info);
    m_mainWin->tabifyDockWidget(dock_layer, dock_pen_palette);
    m_mainWin->tabifyDockWidget(dock_pen_palette, dock_layer_tree);

    m_mainWin->addDockWidget(Qt::RightDockWidgetArea, dock_views);
    m_mainWin->tabifyDockWidget(dock_views, dock_ucss);
    m_mainWin->addDockWidget(Qt::RightDockWidgetArea, dock_command);
}


void LC_WidgetFactory::makeActionsInvisible(const std::vector<QString> &actionNames){
    for (const QString& actionName: actionNames) {
        QAction *action = m_agm->getActionByName(actionName);
        if (action != nullptr) {
            action->setVisible(false);
        }
    }
}

void LC_WidgetFactory::addAction(QToolBar* toolbar, const char* actionName){
    QAction *action = m_agm->getActionByName(actionName);
    if (action != nullptr) {
        toolbar->addAction(action);
    }
}

void LC_WidgetFactory::createPenWizardWidget(){
    auto penWizard = new LC_PenWizard(QObject::tr("Pen Wizard"), m_mainWin);
    penWizard->setObjectName("pen_wiz");
    connect(m_mainWin, &QC_ApplicationWindow::windowsChanged,penWizard, &LC_PenWizard::setEnabled);
    m_mainWin->addDockWidget(Qt::RightDockWidgetArea, penWizard);
    m_mainWin->m_penWizard = penWizard;
}

LC_DockWidget* LC_WidgetFactory::leftDockWidget(const QString& title, const char* name, const QList<QAction*> &actions, int columns, int iconSize, bool flatButtons){
    auto* result = new LC_DockWidget(m_mainWin);
    result->setObjectName("dock_" + QString(name).toLower());
    result->setWindowTitle(title);
    result->add_actions(actions, columns, iconSize, flatButtons);
    result->hide();

    connect(m_mainWin, &QC_ApplicationWindow::widgetSettingsChanged, result, &LC_DockWidget::updateWidgetSettings);
    return result;
}


 QToolBar* LC_WidgetFactory::createStatusBarToolbar(QSizePolicy tbPolicy, QWidget *widget, QString title, const char *name){
    auto tb = new QToolBar(title, m_mainWin);
    tb->setSizePolicy(tbPolicy);
    tb->addWidget(widget);
    tb->setObjectName(name);
    tb->setProperty("_group", 3);
    addToBottom(tb);
    return tb;
}

void LC_WidgetFactory::initStatusBar() {
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init status bar");
    QStatusBar* status_bar = m_mainWin->statusBar();

    m_mainWin->m_coordinateWidget = new QG_CoordinateWidget(status_bar, "coordinates");
    m_mainWin->m_relativeZeroCoordinatesWidget = new LC_RelZeroCoordinatesWidget(status_bar, "relZeroCordinates");
    m_mainWin->m_mouseWidget = new QG_MouseWidget(status_bar, "mouse info");
    m_mainWin->m_selectionWidget = new QG_SelectionWidget(status_bar, "selections");
    m_mainWin->m_activeLayerName = new QG_ActiveLayerName(status_bar);

    m_mainWin->m_gridStatusWidget = new TwoStackedLabels(status_bar);
    m_mainWin->m_gridStatusWidget->setTopLabel(tr("Grid Status"));

    auto* ucsStateWidget = new LC_UCSStateWidget(status_bar, "ucs");
    m_mainWin->m_ucsStateWidget = ucsStateWidget;

    auto* anglesBasisWidget = new LC_AnglesBasisWidget(status_bar, "anglesBase");
    m_mainWin->m_anglesBasisWidget = anglesBasisWidget;

    m_mainWin->m_statusbarManager = new LC_QTStatusbarManager(status_bar);
    m_mainWin->m_statusbarManager->loadSettings();

    bool useClassicalStatusBar = LC_GET_ONE_BOOL("Startup", "UseClassicStatusBar", false);
    if (useClassicalStatusBar) {
        status_bar->addWidget(m_mainWin->m_coordinateWidget);
        status_bar->addWidget(m_mainWin->m_mouseWidget);
        status_bar->addWidget(m_mainWin->m_selectionWidget);
        status_bar->addWidget(m_mainWin->m_activeLayerName);
        status_bar->addWidget(m_mainWin->m_gridStatusWidget);
        status_bar->addWidget(m_mainWin->m_relativeZeroCoordinatesWidget);
        status_bar->addWidget(m_mainWin->m_ucsStateWidget);
        status_bar->addWidget(m_mainWin->m_anglesBasisWidget);

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

        createStatusBarToolbar(tbPolicy, m_mainWin->m_coordinateWidget, tr("Coordinates"), "TBCoordinates");
        createStatusBarToolbar(tbPolicy, m_mainWin->m_relativeZeroCoordinatesWidget, tr("Relative Zero"), "TBRelZero");
        createStatusBarToolbar(tbPolicy, m_mainWin->m_mouseWidget, tr("Mouse"), "TBMouse");
        createStatusBarToolbar(tbPolicy, m_mainWin->m_selectionWidget, tr("Selection Info"), "TBSelection");
        createStatusBarToolbar(tbPolicy, m_mainWin->m_activeLayerName, tr("Active Layer"), "TBActiveLayer");
        createStatusBarToolbar(tbPolicy, m_mainWin->m_gridStatusWidget, tr("Grid Status"), "TBGridStatus");
        createStatusBarToolbar(tbPolicy, m_mainWin->m_ucsStateWidget, tr("UCS Status"), "TBUCSStatus");
        createStatusBarToolbar(tbPolicy, m_mainWin->m_anglesBasisWidget, tr("Angles Basis"), "TBAnglesBasis");

        m_mainWin->m_statusbarManager->setup();

        m_mainWin->m_gridStatusWidget->setToolTip(tr("Current size of Grid/MetaGrid. Click to change grid size."));
        connect(m_mainWin->m_gridStatusWidget, &TwoStackedLabels::clicked, m_mainWin, &QC_ApplicationWindow::slotShowDrawingOptions);

    }
    connect(m_mainWin->m_anglesBasisWidget, &LC_AnglesBasisWidget::clicked, m_mainWin, &QC_ApplicationWindow::slotShowDrawingOptionsUnits);

    connect(m_mainWin, &QC_ApplicationWindow::iconsRefreshed, m_mainWin->m_ucsStateWidget, &LC_UCSStateWidget::onIconsRefreshed);
    connect(m_mainWin, &QC_ApplicationWindow::iconsRefreshed, m_mainWin->m_anglesBasisWidget, &LC_AnglesBasisWidget::onIconsRefreshed);
    connect(m_mainWin, &QC_ApplicationWindow::iconsRefreshed, m_mainWin->m_mouseWidget, &QG_MouseWidget::onIconsRefreshed);

    connect(m_mainWin, &QC_ApplicationWindow::currentActionIconChanged, m_mainWin->m_mouseWidget, &QG_MouseWidget::setCurrentQAction);
    connect(m_mainWin, &QC_ApplicationWindow::currentActionIconChanged, m_mainWin->m_statusbarManager, &LC_QTStatusbarManager::setCurrentQAction);

    bool statusBarVisible = LC_GET_ONE_BOOL("Appearance", "StatusBarVisible", true);
    status_bar->setVisible(statusBarVisible);
}

void LC_WidgetFactory::addToBottom(QToolBar *toolbar) { m_mainWin->addToolBar(Qt::BottomToolBarArea, toolbar); }
