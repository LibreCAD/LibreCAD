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

#include "lc_widgetfactory.h"

#include <QStatusBar>
#include <QToolBar>

#include "lc_actiongroupmanager.h"
#include "lc_anglesbasiswidget.h"
#include "lc_caddockwidget.h"
#include "lc_dockwidget.h"
#include "lc_layertreewidget.h"
#include "lc_namedviewslistwidget.h"
#include "lc_penpalettewidget.h"
#include "lc_penwizard.h"
#include "lc_qtstatusbarmanager.h"
#include "lc_quickinfowidget.h"
#include "lc_relzerocoordinateswidget.h"
#include "lc_ucslistwidget.h"
#include "lc_ucsstatewidget.h"
#include "qc_applicationwindow.h"
#include "qg_activelayername.h"
#include "qg_blockwidget.h"
#include "qg_commandwidget.h"
#include "qg_coordinatewidget.h"
#include "qg_layerwidget.h"
#include "qg_librarywidget.h"
#include "qg_mousewidget.h"
#include "qg_selectionwidget.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "twostackedlabels.h"

LC_WidgetFactory::LC_WidgetFactory(QC_ApplicationWindow* main_win)
    : QObject(nullptr)
    , LC_AppWindowAware(main_win)
    , m_agm(main_win->m_actionGroupManager.get()),
    m_actionFactory{main_win->m_actionFactory.get()}{
}

void LC_WidgetFactory::updateDockOptions(QC_ApplicationWindow * mainWin, bool allowDockNesting, bool verticalTabs) {
    auto dockOptions = QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks;
    if (allowDockNesting) {
        dockOptions |=  QMainWindow::AllowNestedDocks;
    }
    if (verticalTabs) {
        dockOptions |=  QMainWindow::VerticalTabs;
    }

    mainWin->setDockOptions(dockOptions);
}

void LC_WidgetFactory::initWidgets(){
    initStatusBar();
    initLeftCADSidebar();
    createRightSidebar(m_appWin->m_actionHandler.get());
}

void LC_WidgetFactory::initLeftCADSidebar(){
    bool enable_left_sidebar = LC_GET_ONE_BOOL("Startup", "EnableLeftSidebar", true);
    if (enable_left_sidebar) {
        LC_GROUP("Widgets"); {
            int leftSidebarColumnsCount = LC_GET_INT("LeftToolbarColumnsCount", 5);
            int leftSidebarIconSize = LC_GET_INT("LeftToolbarIconSize", 24);
            bool flatIcons = LC_GET_BOOL("LeftToolbarFlatIcons", true);
            createCADSidebar(leftSidebarColumnsCount, leftSidebarIconSize, flatIcons);
        }
        LC_GROUP_END();
    }
}

void LC_WidgetFactory::createCADSidebar(int columns, int icon_size, bool flatButtons){
    auto* line = cadDockWidget(tr("Line"), "Line", m_actionFactory->line_actions, columns, icon_size, flatButtons);
    auto* point = cadDockWidget(tr("Point"), "Point", m_actionFactory->point_actions, columns, icon_size, flatButtons);
    auto* shape = cadDockWidget(tr("Polygon"), "Polygon", m_actionFactory->shape_actions, columns, icon_size, flatButtons);
    auto* circle = cadDockWidget(tr("Circle"), "Circle", m_actionFactory->circle_actions, columns, icon_size, flatButtons);
    auto* curve = cadDockWidget(tr("Arc"), "Curve", m_actionFactory->curve_actions, columns, icon_size, flatButtons);
    auto* spline = cadDockWidget(tr("Spline"), "Spline", m_actionFactory->spline_actions, columns, icon_size, flatButtons);
    auto* ellipse = cadDockWidget(tr("Ellipse"), "Ellipse", m_actionFactory->ellipse_actions, columns, icon_size, flatButtons);
    auto* polyline = cadDockWidget(tr("Polyline"), "Polyline", m_actionFactory->polyline_actions, columns, icon_size, flatButtons);
    auto* select = cadDockWidget(tr("Select"), "Select", m_actionFactory->select_actions, columns, icon_size, flatButtons);
    auto* dimension = cadDockWidget(tr("Dimension"), "Dimension", m_actionFactory->dimension_actions, columns, icon_size, flatButtons);
    auto* other = cadDockWidget(tr("Other"), "Other", m_actionFactory->other_drawing_actions, columns, icon_size, flatButtons);
    auto* modify = cadDockWidget(tr("Modify"), "Modify", m_actionFactory->modify_actions, columns, icon_size, flatButtons);
    auto* info = cadDockWidget(tr("Info"), "Info", m_actionFactory->info_actions, columns, icon_size, flatButtons);
    auto* order = cadDockWidget(tr("Order"), "Order", m_actionFactory->order_actions, columns, icon_size, flatButtons);

    m_appWin->addDockWidget(Qt::LeftDockWidgetArea, line);
    m_appWin->tabifyDockWidget(line, polyline);
    m_appWin->tabifyDockWidget(polyline, point);
    m_appWin->tabifyDockWidget(polyline, shape);
    line->raise();
    m_appWin->addDockWidget(Qt::LeftDockWidgetArea, circle);
    m_appWin->tabifyDockWidget(circle, curve);
    m_appWin->tabifyDockWidget(curve, spline);
    m_appWin->tabifyDockWidget(spline, ellipse);
    circle->raise();
    m_appWin->addDockWidget(Qt::LeftDockWidgetArea, dimension);
    m_appWin->tabifyDockWidget(dimension, other);
    m_appWin->tabifyDockWidget(other, info);
    m_appWin->tabifyDockWidget(info, select);
    dimension->raise();
    m_appWin->addDockWidget(Qt::LeftDockWidgetArea, modify);
    m_appWin->tabifyDockWidget(modify, order);
}

QDockWidget* LC_WidgetFactory::createDockWidget(const QString& horizontalTitle, const char *name, const QString& verticalTitle) const {
    auto result = new LC_DockWidget(m_appWin, horizontalTitle, verticalTitle);
    // auto result = new QDockWidget(horizontalTitle, m_appWin);
    result->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    result->setWindowTitle(horizontalTitle);
    result->setObjectName(name);
    result->setProperty("_lc_doc_widget", true);
    return result;
}

QDockWidget* LC_WidgetFactory::createPenPalletteWidget(){
    auto dock = createDockWidget(tr("Pens Palette"), "pen_palette_dockwidget", tr("Pens"));
    auto widget = new LC_PenPaletteWidget("PenPalette", dock);
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(widget, &LC_PenPaletteWidget::escape, m_appWin, &QC_ApplicationWindow::slotFocus);
    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_PenPaletteWidget::updateWidgetSettings);

    m_appWin->m_penPaletteWidget = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createLayerWidget(QG_ActionHandler *actionHandler){
    auto dock = createDockWidget(tr("Layers"), "layer_dockwidget", tr("Layers"));
    auto widget = new QG_LayerWidget(m_agm, actionHandler, dock, "Layer");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(widget, &QG_LayerWidget::escape, m_appWin, &QC_ApplicationWindow::slotFocus);
    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &QG_LayerWidget::updateWidgetSettings);

    m_appWin->m_layerWidget  = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createNamedViewsWidget(){
    auto dock   = createDockWidget(tr("Named Views"), "view_dockwidget", tr("Views"));
    auto widget = new LC_NamedViewsListWidget("View", dock);
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_NamedViewsListWidget::updateWidgetSettings);
    connect(m_appWin->m_ucsListWidget, &LC_UCSListWidget::ucsListChanged, widget, &LC_NamedViewsListWidget::onUcsListChanged);

    m_appWin->m_namedViewsWidget = widget;

    QC_ApplicationWindow *win = m_appWin;

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
    auto dock = createDockWidget(tr("User Coordinate Systems"), "ucs_dockwidget", tr("UCSs"));
    auto widget = new LC_UCSListWidget("UCS", dock);
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_UCSListWidget::updateWidgetSettings);

    m_appWin->m_ucsListWidget = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createLayerTreeWidget(QG_ActionHandler *action_handler){
    QDockWidget* dock = createDockWidget(tr("Layers Tree"), "layer_tree_dockwidget", tr("Layers Tree"));
    auto widget = new LC_LayerTreeWidget(action_handler, dock, "Layer Tree");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(widget, &LC_LayerTreeWidget::escape, m_appWin, &QC_ApplicationWindow::slotFocus);
    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_LayerTreeWidget::updateWidgetSettings);

    m_appWin->m_layerTreeWidget = widget;
    return dock;
}

QDockWidget* LC_WidgetFactory::createEntityInfoWidget(){
    QDockWidget* dock = createDockWidget(tr("Entity Info"), "quick_entity_info", tr("Info"));
    auto widget = new LC_QuickInfoWidget(dock, m_agm->getActionsMap());
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_QuickInfoWidget::updateWidgetSettings);

    m_appWin->m_quickInfoWidget = widget;
    return dock;
}

QDockWidget*  LC_WidgetFactory::createBlockListWidget(QG_ActionHandler *actionHandler){
    auto dock =  createDockWidget(tr("Blocks"), "block_dockwidget", tr("Blocks"));

    auto widget = new QG_BlockWidget(m_agm, actionHandler, dock, "Block");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    connect(widget, &QG_BlockWidget::escape, m_appWin, &QC_ApplicationWindow::slotFocus);
    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &QG_BlockWidget::updateWidgetSettings);

    m_appWin->m_blockWidget = widget;
    return dock;
}

QDockWidget*  LC_WidgetFactory::createLibraryWidget(QG_ActionHandler *action_handler){
    auto dock = createDockWidget(tr("Library Browser"), "library_dockwidget", tr("Library"));

    auto widget = new QG_LibraryWidget(action_handler, dock, "Library");
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    // result->resize(240, 400);

    connect(widget, &QG_LibraryWidget::escape, m_appWin, &QC_ApplicationWindow::slotFocus);
    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &QG_LibraryWidget::updateWidgetSettings);

    m_appWin->m_libraryWidget = widget;
    return dock;
}

QDockWidget * LC_WidgetFactory::createCmdWidget(QG_ActionHandler *action_handler){
    auto dock = createDockWidget(tr("Command Line"), "command_dockwidget", tr("Cmd"));

    auto widget = new QG_CommandWidget(action_handler, dock, "Command");
    widget->setActionHandler(action_handler);

    dock->setWidget(widget);
    widget->getDockingAction()->setText(dock->isFloating() ? tr("Dock") : tr("Float"));

    connect(widget->leCommand, &QG_CommandEdit::escape, m_appWin, &QC_ApplicationWindow::slotFocus);
    // fixme - sand - disable setting vertical caption so far as this is now controlled in uniform way by widget
    // setttings.
    // fixme - sand - remove this call and the slot later, if there will no request from the users to recover this
    // connect(dock, &QDockWidget::dockLocationChanged,m_appWin, &QC_ApplicationWindow::modifyCommandTitleBar);

    m_appWin->m_commandWidget = widget;
    return dock;
}

/**
 * This slot modifies the commandline's title bar
 * depending on the dock area it is moved to.
 */
// fixme - sand - files - remove later, just port from ApppWindow - use uniform way
void LC_WidgetFactory::modifyCommandTitleBar(Qt::DockWidgetArea area) const {
    auto *cmdDockWidget = findChild<QDockWidget *>("command_dockwidget");

    auto *commandWidget = static_cast<QG_CommandWidget *>(cmdDockWidget->widget());
    QAction *dockingAction = commandWidget->getDockingAction();
    bool docked = area & Qt::AllDockWidgetAreas;
    cmdDockWidget->setWindowTitle(docked ? tr("Cmd") : tr("Command Line"));
    dockingAction->setText(docked ? tr("Float") : tr("Dock", "Dock the command widget to the main window"));
    QDockWidget::DockWidgetFeatures features =
        QDockWidget::DockWidgetClosable
        | QDockWidget::DockWidgetMovable
        | QDockWidget::DockWidgetFloatable;

    if (docked) {
        features |= QDockWidget::DockWidgetVerticalTitleBar;
    }
    cmdDockWidget->setFeatures(features);
}

void LC_WidgetFactory::updateDockWidgetsTitleBarType(const QC_ApplicationWindow* mainWin, bool verticalTitle) {
    QList<QDockWidget*> dockwidgetsList = mainWin->findChildren<QDockWidget*>();
    for (QDockWidget* dw: dockwidgetsList) {
        if (dw->property("_lc_doc_widget").isValid()) {
            setDockWidgetTitleType(dw, verticalTitle);
        }
    }
}

void LC_WidgetFactory::createRightSidebar(QG_ActionHandler* action_handler){

    bool verticalTitle = LC_GET_ONE_BOOL("Widgets", "DockTitleBarVertical", false);
    QDockWidget *dock_pen_palette = createPenPalletteWidget();
    QDockWidget *dock_layer = createLayerWidget(action_handler);
    QDockWidget *dock_ucss = createUCSListWidget();
    QDockWidget *dock_views = createNamedViewsWidget();
    QDockWidget *dock_layer_tree = createLayerTreeWidget(action_handler);
    QDockWidget *dock_quick_info = createEntityInfoWidget();
    QDockWidget *dock_block = createBlockListWidget(action_handler);
    QDockWidget *dock_library = createLibraryWidget(action_handler);
    QDockWidget *dock_command = createCmdWidget(action_handler);
    QDockWidget *doc_pen_wiz = createPenWizardWidget();

    m_appWin->addDockWidget(Qt::RightDockWidgetArea, doc_pen_wiz);

    m_appWin->addDockWidget(Qt::RightDockWidgetArea, dock_library);
    m_appWin->tabifyDockWidget(dock_library, dock_block);
    m_appWin->tabifyDockWidget(dock_block, dock_layer);
    m_appWin->tabifyDockWidget(dock_block, dock_quick_info);
    m_appWin->tabifyDockWidget(dock_layer, dock_pen_palette);
    m_appWin->tabifyDockWidget(dock_pen_palette, dock_layer_tree);

    m_appWin->addDockWidget(Qt::RightDockWidgetArea, dock_views);
    m_appWin->tabifyDockWidget(dock_views, dock_ucss);
    m_appWin->addDockWidget(Qt::RightDockWidgetArea, dock_command);

    updateDockWidgetsTitleBarType(m_appWin, verticalTitle);
}

// fixme - sand - remove this method
void LC_WidgetFactory::makeActionsInvisible(const std::vector<QString> &actionNames) const {
    for (const QString& actionName: actionNames) {
        QAction *action = m_agm->getActionByName(actionName);
        if (action != nullptr) {
            action->setVisible(false);
        }
    }
}

// fixme - sand - remove this method
void LC_WidgetFactory::addAction(QToolBar* toolbar, const char* actionName) const {
    QAction *action = m_agm->getActionByName(actionName);
    if (action != nullptr) {
        toolbar->addAction(action);
    }
}

QDockWidget* LC_WidgetFactory::createPenWizardWidget(){
    auto dock = createDockWidget(tr("Pen Wizard"), "pen_wiz_dockwidget", tr("PenWiz"));
    auto widget = new LC_PenWizard(dock);
    widget->setFocusPolicy(Qt::NoFocus);
    dock->setWidget(widget);

    // connect(widget, &LC_PenPaletteWidget::escape, m_appWin, &QC_ApplicationWindow::slotFocus);
    // connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, widget, &LC_PenPaletteWidget::updateWidgetSettings);
    connect(m_appWin, &QC_ApplicationWindow::windowsChanged,widget, &LC_PenWizard::setEnabled);

    m_appWin->m_penWizard = widget;
    return dock;
}

void LC_WidgetFactory::setDockWidgetTitleType(QDockWidget *widget, bool verticalTitleBar){
    QDockWidget::DockWidgetFeatures features =
            QDockWidget::DockWidgetClosable
            | QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable;


    if (verticalTitleBar) {
        features |= QDockWidget::DockWidgetVerticalTitleBar;
    }
    widget->setFeatures(features);
    auto lcDocWidget = dynamic_cast<LC_DockWidget*>(widget);
    if (lcDocWidget != nullptr) {
        lcDocWidget->updateTitle();
    }

}

LC_CADDockWidget* LC_WidgetFactory::cadDockWidget(const QString& title, const char* name, const QList<QAction*> &actions, int columns, int iconSize, bool flatButtons){
    auto* result = new LC_CADDockWidget(m_appWin);
    result->setAllowedAreas(Qt::LeftDockWidgetArea |
                              Qt::RightDockWidgetArea);
    result->setObjectName("dock_" + QString(name).toLower());
    result->setWindowTitle(title);
    result->add_actions(actions, columns, iconSize, flatButtons);
    result->hide();
    connect(m_appWin, &QC_ApplicationWindow::widgetSettingsChanged, result, &LC_CADDockWidget::updateWidgetSettings);
    return result;
}

 QToolBar* LC_WidgetFactory::createStatusBarToolbar(QSizePolicy tbPolicy, QWidget *widget, const QString& title, const char *name) const {
    auto tb = new QToolBar(title, m_appWin);
    tb->setSizePolicy(tbPolicy);
    tb->addWidget(widget);
    tb->setObjectName(name);
    tb->setProperty("_group", 3);
    addToBottom(tb);
    return tb;
}

void LC_WidgetFactory::initStatusBar() {
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init status bar");
    QStatusBar* status_bar = m_appWin->statusBar();

    m_appWin->m_coordinateWidget = new QG_CoordinateWidget(status_bar, "coordinates");
    m_appWin->m_relativeZeroCoordinatesWidget = new LC_RelZeroCoordinatesWidget(status_bar, "relZeroCordinates");
    m_appWin->m_mouseWidget = new QG_MouseWidget(status_bar, "mouse info");
    m_appWin->m_selectionWidget = new QG_SelectionWidget(status_bar, "selections");
    m_appWin->m_activeLayerNameWidget = new QG_ActiveLayerName(status_bar);

    m_appWin->m_gridStatusWidget = new TwoStackedLabels(status_bar);
    m_appWin->m_gridStatusWidget->setTopLabel(tr("Grid Status"));

    auto* ucsStateWidget = new LC_UCSStateWidget(status_bar, "ucs");
    m_appWin->m_ucsStateWidget = ucsStateWidget;

    auto* anglesBasisWidget = new LC_AnglesBasisWidget(status_bar, "anglesBase");
    m_appWin->m_anglesBasisWidget = anglesBasisWidget;

    m_appWin->m_statusbarManager = new LC_QTStatusbarManager(status_bar);
    m_appWin->m_statusbarManager->loadSettings();

    bool useClassicalStatusBar = LC_GET_ONE_BOOL("Startup", "UseClassicStatusBar", false);
    if (useClassicalStatusBar) {
        status_bar->addWidget(m_appWin->m_coordinateWidget);
        status_bar->addWidget(m_appWin->m_mouseWidget);
        status_bar->addWidget(m_appWin->m_selectionWidget);
        status_bar->addWidget(m_appWin->m_activeLayerNameWidget);
        status_bar->addWidget(m_appWin->m_gridStatusWidget);
        status_bar->addWidget(m_appWin->m_relativeZeroCoordinatesWidget);
        status_bar->addWidget(m_appWin->m_ucsStateWidget);
        status_bar->addWidget(m_appWin->m_anglesBasisWidget);

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

        createStatusBarToolbar(tbPolicy, m_appWin->m_coordinateWidget, tr("Coordinates"), "TBCoordinates");
        createStatusBarToolbar(tbPolicy, m_appWin->m_relativeZeroCoordinatesWidget, tr("Relative Zero"), "TBRelZero");
        createStatusBarToolbar(tbPolicy, m_appWin->m_mouseWidget, tr("Mouse"), "TBMouse");
        createStatusBarToolbar(tbPolicy, m_appWin->m_selectionWidget, tr("Selection Info"), "TBSelection");
        createStatusBarToolbar(tbPolicy, m_appWin->m_activeLayerNameWidget, tr("Active Layer"), "TBActiveLayer");
        createStatusBarToolbar(tbPolicy, m_appWin->m_gridStatusWidget, tr("Grid Status"), "TBGridStatus");
        createStatusBarToolbar(tbPolicy, m_appWin->m_ucsStateWidget, tr("UCS Status"), "TBUCSStatus");
        createStatusBarToolbar(tbPolicy, m_appWin->m_anglesBasisWidget, tr("Angles Basis"), "TBAnglesBasis");

        m_appWin->m_statusbarManager->setup();

        m_appWin->m_gridStatusWidget->setToolTip(tr("Current size of Grid/MetaGrid. Click to change grid size."));
        connect(m_appWin->m_gridStatusWidget, &TwoStackedLabels::clicked, m_appWin, &QC_ApplicationWindow::slotShowDrawingOptions);

    }
    connect(m_appWin->m_anglesBasisWidget, &LC_AnglesBasisWidget::clicked, m_appWin, &QC_ApplicationWindow::slotShowDrawingOptionsUnits);

    connect(m_appWin, &QC_ApplicationWindow::iconsRefreshed, m_appWin->m_ucsStateWidget, &LC_UCSStateWidget::onIconsRefreshed);
    connect(m_appWin, &QC_ApplicationWindow::iconsRefreshed, m_appWin->m_anglesBasisWidget, &LC_AnglesBasisWidget::onIconsRefreshed);
    connect(m_appWin, &QC_ApplicationWindow::iconsRefreshed, m_appWin->m_mouseWidget, &QG_MouseWidget::onIconsRefreshed);

    connect(m_appWin, &QC_ApplicationWindow::currentActionIconChanged, m_appWin->m_mouseWidget, &QG_MouseWidget::setCurrentQAction);
    connect(m_appWin, &QC_ApplicationWindow::currentActionIconChanged, m_appWin->m_statusbarManager, &LC_QTStatusbarManager::setCurrentQAction);

    bool statusBarVisible = LC_GET_ONE_BOOL("Appearance", "StatusBarVisible", true);
    status_bar->setVisible(statusBarVisible);
}

void LC_WidgetFactory::addToBottom(QToolBar *toolbar) const { m_appWin->addToolBar(Qt::BottomToolBarArea, toolbar); }
