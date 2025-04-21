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
#include "lc_applicationwindowinitializer.h"

#include <QCoreApplication>
#include <QMdiArea>
#include <QMdiSubWindow>

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "lc_actionoptionsmanager.h"
#include "lc_appwindowdialogsinvoker.h"
#include "lc_centralwidget.h"
#include "lc_customstylehelper.h"
#include "lc_defaultactioncontext.h"
#include "lc_gridviewinvoker.h"
#include "lc_infocursorsettingsmanager.h"
#include "lc_lastopenfilesopener.h"
#include "lc_menufactory.h"
#include "lc_optionswidgetsholder.h"
#include "lc_plugininvoker.h"
#include "lc_releasechecker.h"
#include "lc_snapmanager.h"
#include "lc_toolbarfactory.h"
#include "lc_widgetfactory.h"
#include "lc_workspacesinvoker.h"
#include "main.h"
#include "qc_applicationwindow.h"
#include "qc_dialogfactory.h"
#include "qg_actionhandler.h"
#include "qg_commandwidget.h"
#include "qg_recentfiles.h"
#include "qg_snaptoolbar.h"
#include "rs_commands.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_settings.h"

LC_ApplicationWindowInitializer::LC_ApplicationWindowInitializer(QC_ApplicationWindow *appWindow)
    : LC_AppWindowAware{appWindow}
{}

void LC_ApplicationWindowInitializer::initApplication(){
    m_appWin->m_actionHandler = std::make_unique<QG_ActionHandler>(m_appWin);
    m_appWin->m_dlgHelpr = std::make_unique<LC_AppWindowDialogsInvoker>(m_appWin);
    m_appWin->m_workspacesInvoker = std::make_unique<LC_WorkspacesInvoker>(m_appWin);
    m_appWin->m_gridViewInvoker = std::make_unique<LC_GridViewInvoker>(m_appWin);
    m_appWin->m_infoCursorSettingsManager = std::make_unique<LC_InfoCursorSettingsManager>(m_appWin);
    m_appWin->m_styleHelper = std::make_unique<LC_CustomStyleHelper>(m_appWin);
    m_appWin->m_lastFilesOpener = std::make_unique<LC_LastOpenFilesOpener>(m_appWin);

    initActionGroupManager();
    //accept drop events to open files
    m_appWin->setAcceptDrops(true);
    initDockCorners();
    initIconSize();

    initActionFactory();
    initWidgets();
    initToolbars();
    initSnapManager();

    initCentralWidget();
    initMainMenu();
    initDockAreasActions();
    initActionOptionsManager();
    initActionContext();
    initDialogFactory();
    initRecentFilesList();
    m_appWin->initSettings();
    loadCmdWidgetVariablesFile();
    initAutoSaveTimer();
    updateCommandsAlias();
    initPlugins();
    m_appWin->showStatusMessage(qApp->applicationName() + " Ready", 2000);
    initReleaseChecker();
}

void LC_ApplicationWindowInitializer::initSnapManager() {
    m_appWin->m_snapManager = std::make_unique<LC_SnapManager>(m_appWin->m_snapToolBar);
    m_appWin->m_actionHandler->setSnapManager(m_appWin->m_snapManager.get());
}

void LC_ApplicationWindowInitializer::initReleaseChecker(){
    const char *ownBuildVersion = XSTR(LC_VERSION);
    m_appWin->m_releaseChecker = std::make_unique<LC_ReleaseChecker>( ownBuildVersion,XSTR(LC_PRERELEASE));
    connect(m_appWin->m_releaseChecker.get(), &LC_ReleaseChecker::updatesAvailable, m_appWin, &QC_ApplicationWindow::onNewVersionAvailable);
}

void LC_ApplicationWindowInitializer::initActionGroupManager(){
    m_appWin->m_actionGroupManager = std::make_unique<LC_ActionGroupManager>(m_appWin);
    connect(RS_SETTINGS, &RS_Settings::optionsChanged, m_appWin->m_actionGroupManager.get(), &LC_ActionGroupManager::onOptionsChanged);
}

void LC_ApplicationWindowInitializer::initActionOptionsManager(){
    LC_SnapOptionsWidgetsHolder *snapOptionsHolder = m_appWin->m_snapToolBar->getSnapOptionsHolder();
    m_appWin->m_actionOptionsManager = new LC_ActionOptionsManager(m_appWin, m_appWin->m_toolOptionsToolbar, snapOptionsHolder);
    LC_OptionsWidgetsHolder* optionsWidgetsHolder = m_appWin->m_actionOptionsManager->getActionOptionWidgetHolder();
    connect(m_appWin, &QC_ApplicationWindow::currentActionIconChanged, optionsWidgetsHolder, &LC_OptionsWidgetsHolder::setCurrentQAction);
}

void LC_ApplicationWindowInitializer::initActionFactory() const {
    m_appWin->m_actionFactory = std::make_unique<LC_ActionFactory>(m_appWin, m_appWin->m_actionHandler.get());
    bool using_theme = LC_GET_ONE_BOOL("Widgets","AllowTheme", false);
    m_appWin->m_actionFactory->initActions(m_appWin->m_actionGroupManager.get(), using_theme);
}

void LC_ApplicationWindowInitializer::initDockCorners() const {
    LC_GROUP("Widgets");
    {
        bool allowDockNesting = LC_GET_BOOL("DockAllowNested", true);
        bool verticalTabs = LC_GET_BOOL("DockVerticalTabs", false);
        LC_WidgetFactory::updateDockOptions(m_appWin, allowDockNesting, verticalTabs);
    }
    LC_GROUP_END();

    // make the left and right dock areas dominant
    m_appWin->setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    m_appWin->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    m_appWin->setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    m_appWin->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
}


void LC_ApplicationWindowInitializer::initCentralWidget(){
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating LC_CentralWidget");

    auto central = new LC_CentralWidget(m_appWin);
    m_appWin->setCentralWidget(central);
    m_appWin->m_mdiAreaCAD = central->getMdiArea();
    m_appWin->m_mdiAreaCAD->setDocumentMode(true);

    LC_GROUP("WindowOptions");
    m_appWin->setTabLayout(static_cast<RS2::TabShape>(LC_GET_INT("TabShape", RS2::Triangular)),
                 static_cast<RS2::TabPosition>(LC_GET_INT("TabPosition", RS2::West)));
    LC_GROUP_END();

    bool tabMode = LC_GET_ONE_BOOL("Startup", "TabMode", false);
    if (tabMode) {
        m_appWin->setupCADAreaTabbar();
    }

    connect(m_appWin->m_mdiAreaCAD, &QMdiArea::subWindowActivated, m_appWin, &QC_ApplicationWindow::slotWindowActivated);

    // This event filter allows sending key events to the command widget, therefore, no
    // need to activate the command widget before typing commands.
    // Since this nice feature causes a bug of lost key events when the command widget is on
    // a screen different from the main window, disabled for the time being
    // send key events for mdiAreaCAD to command widget by default
    m_appWin->m_mdiAreaCAD->installEventFilter(m_appWin->m_commandWidget);

    connect(m_appWin->getAction("FileClose"), &QAction::triggered, m_appWin->m_mdiAreaCAD, &QMdiArea::closeActiveSubWindow);
}

void LC_ApplicationWindowInitializer::initIconSize() const {
    LC_GROUP("Widgets");
    {
        bool custom_size = LC_GET_BOOL("AllowToolbarIconSize", false);
        int icon_size = custom_size ? LC_GET_INT("ToolbarIconSize", 24) : 24;

        if (custom_size) {
            m_appWin->setIconSize(QSize(icon_size, icon_size));
        }
    }
    LC_GROUP_END();
}

void LC_ApplicationWindowInitializer::loadCmdWidgetVariablesFile() const {
    auto command_file = LC_GET_ONE_STR("Paths","VariableFile", "");
    if (!command_file.isEmpty()) {
        m_appWin->m_commandWidget->leCommand->readCommandFile(command_file);
    }
}

void LC_ApplicationWindowInitializer::initDockAreasActions() const {
    m_appWin->m_dockAreas.left = m_appWin->getAction("LeftDockAreaToggle");
    m_appWin->m_dockAreas.right = m_appWin->getAction("RightDockAreaToggle");
    m_appWin->m_dockAreas.top = m_appWin->getAction("TopDockAreaToggle");
    m_appWin->m_dockAreas.bottom = m_appWin->getAction("BottomDockAreaToggle");
    m_appWin->m_dockAreas.floating = m_appWin->getAction("FloatingDockwidgetsToggle");
}

void LC_ApplicationWindowInitializer::initMainMenu() const {
    m_appWin->m_menuFactory = std::make_unique<LC_MenuFactory>(m_appWin);
    m_appWin->m_menuFactory->createMainMenu(m_appWin->menuBar());
}

void LC_ApplicationWindowInitializer::updateCommandsAlias(){
    RS_COMMANDS->updateAlias();
}

void LC_ApplicationWindowInitializer::initRecentFilesList() const {
    m_appWin->m_recentFilesList = new QG_RecentFiles(m_appWin, 9);
    m_appWin->m_recentFilesList->addFiles(m_appWin->m_menuFactory->getRecentFilesMenu());
}

void LC_ApplicationWindowInitializer::initDialogFactory() const {
    LC_SnapOptionsWidgetsHolder *snapOptionsHolder = m_appWin->m_snapToolBar->getSnapOptionsHolder();
    auto factory = new QC_DialogFactory(m_appWin, m_appWin->m_toolOptionsToolbar, snapOptionsHolder);
    RS_DialogFactory::instance()->setFactoryObject(factory);
    m_appWin->m_dialogFactory = factory;

    // fixme - sand - temporary setters, remove later
    factory->set_command_widget(m_appWin->m_commandWidget);
    factory->set_rel_zero_coordinates_widget(m_appWin->m_relativeZeroCoordinatesWidget);
    factory->set_selection_widget(m_appWin->m_selectionWidget);
}

void LC_ApplicationWindowInitializer::initWidgets() const {
    LC_WidgetFactory widgetFactory(m_appWin);
    widgetFactory.initWidgets();
}

void LC_ApplicationWindowInitializer::initToolbars() const {
    LC_ToolbarFactory toolbarFactory(m_appWin);
    toolbarFactory.initToolBars();
}

void LC_ApplicationWindowInitializer::initPlugins(){
    m_appWin->m_pluginInvoker = std::make_unique<LC_PluginInvoker>(m_appWin, m_appWin->m_actionContext);
    m_appWin->m_pluginInvoker->loadPlugins();
}

void LC_ApplicationWindowInitializer::initAutoSaveTimer() const {
    bool allowAutoSave = LC_GET_ONE_BOOL("Defaults", "AutoBackupDocument", true);
    m_appWin->startAutoSaveTimer(allowAutoSave);
}

/**
 * NOTE: potentially, the main application window may represent implementation of ActionContext instead of
 * LC_DefaultActionContext. Thinks whether this is practical..
 */
void LC_ApplicationWindowInitializer::initActionContext() const {
    auto actionHandler = m_appWin->m_actionHandler.get();
    auto action_context = new LC_DefaultActionContext(actionHandler);

    action_context->setActionOptionsManager(m_appWin->m_actionOptionsManager);
    action_context->setCommandWidget(m_appWin->m_commandWidget);
    action_context->setCoordinateWidget(m_appWin->m_coordinateWidget);
    action_context->setMouseWidget(m_appWin->m_mouseWidget);
    action_context->setSelectionWidget(m_appWin->m_selectionWidget);
    action_context->setStatusBarManager(m_appWin->m_statusbarManager);

    m_appWin->m_actionContext = action_context;

    actionHandler->setActionContext(action_context);
}
