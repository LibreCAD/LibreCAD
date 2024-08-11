/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
** 
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)
** Copyright (C) 2018 Simon Wells (simonrwells@gmail.com)
** Copyright (C) 2015-2016 ravas (github.com/r-a-v-a-s)
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

// Changes: https://github.com/LibreCAD/LibreCAD/commits/master/librecad/src/main/qc_applicationwindow.cpp

#include "qc_applicationwindow.h"

#include <QByteArray>
#include <QDockWidget>
#include <QFileDialog>
#include <QImageWriter>
#include <QMdiArea>
#include <QMenuBar>
#include <QMessageBox>
#include <QPagedPaintDevice>
#include <QPluginLoader>
#include <QPrinter>
#include <QPrintDialog>
#include <QRegExp>
#include <QSplitter>
#include <QStatusBar>
#include <QStyleFactory>
#include <QSysInfo>
#include <QTimer>
#include <QtSvg>

#include <boost/version.hpp>


#include "comboboxoption.h"
#include "doc_plugin_interface.h"
#include "main.h"
#include "textfileviewer.h"
#include "twostackedlabels.h"
#include "widgetcreator.h"

#include "rs_actionlibraryinsert.h"
#include "rs_actionprintpreview.h"
#include "rs_commands.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_document.h"
#include "rs_painterqt.h"
#include "rs_pen.h"
#include "rs_settings.h"
#include "rs_staticgraphicview.h"
#include "rs_system.h"
#include "rs_selection.h"
#include "rs_units.h"

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "lc_centralwidget.h"
#include "lc_penwizard.h"
#include "lc_printing.h"
#include "lc_widgetfactory.h"
#include "lc_widgetoptionsdialog.h"
#include "lc_undosection.h"

#include "qc_dialogfactory.h"
#include "qc_mdiwindow.h"
#include "qc_plugininterface.h"

#include "qg_actionhandler.h"
#include "qg_activelayername.h"
#include "qg_blockwidget.h"
#include "qg_commandwidget.h"
#include "qg_coordinatewidget.h"
#include "qg_dlgimageoptions.h"
#include "qg_exitdialog.h"
#include "qg_filedialog.h"
#include "qg_graphicview.h"
#include "qg_layerwidget.h"
#include "lc_layertreewidget.h"
#include "qg_pentoolbar.h"
#include "qg_selectionwidget.h"
#include "qg_snaptoolbar.h"
#include "qg_mousewidget.h"
#include "qg_recentfiles.h"

#ifndef QC_APP_ICON
# define QC_APP_ICON ":/main/librecad.png"
#endif
#ifndef QC_ABOUT_ICON
# define QC_ABOUT_ICON ":/main/intro_librecad.png"
#endif

/*	- Window Title Bar Extra (character) Size.
 *	- Notes: Extra characters appearing in the windows title bar
 *	  are " - [", ... "]" (5), and sometimes "Print preview of " (17).
 *	*/
#define WTB_EXTRA_SIZE        (5 + 17)

/*	Window Title Bar Maximum Size.
 *	Notes: On Windows XP, this is 79.
 *	*/
#define WTB_MAX_SIZE        79

/**
 * Constructor. Initializes the app.
 */
QC_ApplicationWindow::QC_ApplicationWindow()
    : ag_manager(new LC_ActionGroupManager(this))
    , actionHandler(new QG_ActionHandler(this))
    , current_subwindow(nullptr)
    , pen_wiz(new LC_PenWizard(QObject::tr("Pen Wizard"), this))
{
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow");

#ifdef _WINDOWS
	qt_ntfs_permission_lookup++; // turn checking on
#endif

    //accept drop events to open files
    setAcceptDrops(true);

    // make the left and right dock areas dominant
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    QSettings settings;

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: setting icon");
    setWindowIcon(QIcon(QC_APP_ICON));

    pen_wiz->setObjectName("pen_wiz");
    connect(this, &QC_ApplicationWindow::windowsChanged,
            pen_wiz, &LC_PenWizard::setEnabled);
    addDockWidget(Qt::RightDockWidgetArea, pen_wiz);

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init status bar");
    QStatusBar* status_bar = statusBar();
    coordinateWidget = new QG_CoordinateWidget(status_bar, "coordinates");
    status_bar->addWidget(coordinateWidget);
    mouseWidget = new QG_MouseWidget(status_bar, "mouse info");
    status_bar->addWidget(mouseWidget);
    selectionWidget = new QG_SelectionWidget(status_bar, "selections");
    status_bar->addWidget(selectionWidget);
    m_pActiveLayerName = new QG_ActiveLayerName(this);
    status_bar->addWidget(m_pActiveLayerName);
    grid_status = new TwoStackedLabels(status_bar);
    grid_status->setTopLabel(tr("Grid Status"));
    status_bar->addWidget(grid_status);

    settings.beginGroup("Widgets");
    int allow_statusbar_fontsize = settings.value("AllowStatusbarFontSize", 0).toInt();
    int allow_statusbar_height = settings.value("AllowStatusbarHeight", 0).toInt();

    if (allow_statusbar_fontsize)
    {
        int fontsize = settings.value("StatusbarFontSize", 12).toInt();
        QFont font;
        font.setPointSize(fontsize);
        status_bar->setFont(font);
    }
    int height {64};
    if (allow_statusbar_height) {
        height = settings.value( "StatusbarHeight", 64).toInt();
    }
    status_bar->setMinimumHeight( height);
    status_bar->setMaximumHeight( height);
    settings.endGroup();

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating LC_CentralWidget");

    auto central = new LC_CentralWidget(this);

    setCentralWidget(central);

    mdiAreaCAD = central->getMdiArea();
    mdiAreaCAD->setDocumentMode(true);

	RS_SETTINGS->beginGroup("/WindowOptions");
	setTabLayout(static_cast<RS2::TabShape>(RS_SETTINGS->readNumEntry("/TabShape", RS2::Triangular)),
		static_cast<RS2::TabPosition>(RS_SETTINGS->readNumEntry("/TabPosition", RS2::West)));
	RS_SETTINGS->endGroup();

    settings.beginGroup("Startup");
	if (settings.value("TabMode", 0).toBool()) {
		mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
		QList<QTabBar *> tabBarList = mdiAreaCAD->findChildren<QTabBar*>();
		QTabBar *tabBar = tabBarList.at(0);
		if (tabBar)
			tabBar->setExpanding(false);
	}
        
    bool enable_left_sidebar = settings.value("EnableLeftSidebar", 1).toBool();
    bool enable_cad_toolbars = settings.value("EnableCADToolbars", 1).toBool();
    settings.endGroup();

    connect(mdiAreaCAD, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(slotWindowActivated(QMdiSubWindow*)));

    settings.beginGroup("Widgets");
    bool custom_size = settings.value("AllowToolbarIconSize", 0).toBool();
    int icon_size = custom_size ? settings.value("ToolbarIconSize", 24).toInt() : 24;
    settings.endGroup();

    if (custom_size)
        setIconSize(QSize(icon_size, icon_size));

    LC_ActionFactory a_factory(this, actionHandler);
    a_factory.using_theme = settings.value("Widgets/AllowTheme", 0).toBool();
    a_factory.fillActionContainer(a_map, ag_manager);

    LC_WidgetFactory widget_factory(this, a_map, ag_manager);
    if (enable_left_sidebar){
        int leftSidebarColumnsCount = settings.value("Widgets/LeftToolbarColumnsCount", 5).toInt();
        widget_factory.createLeftSidebar(leftSidebarColumnsCount, icon_size);
    }
    if (enable_cad_toolbars)
        widget_factory.createCADToolbars();
    widget_factory.createRightSidebar(actionHandler);
    widget_factory.createCategoriesToolbar();
    widget_factory.createStandardToolbars(actionHandler);

    foreach(auto action, widget_factory.snap_toolbar->actions())
    {
        if(!action->objectName().isEmpty())
        {
            a_map[action->objectName()] = action;
        }
    }

    settings.beginGroup("CustomToolbars");
    foreach (auto key, settings.childKeys())
    {
        auto toolbar = new QToolBar(key, this);
        toolbar->setObjectName(key);
        foreach (auto action, settings.value(key).toStringList())
        {
            toolbar->addAction(a_map[action]);
        }
        addToolBar(toolbar);
    }
    settings.endGroup();

    if (settings.value("Startup/FirstLoad", 1).toBool())
    {
        QStringList list;
        list << "DrawMText"
             << "DrawHatch"
             << "DrawImage"
             << "BlocksCreate"
             << "DrawPoint";

        auto toolbar = new QToolBar("DefaultCustom", this);
        toolbar->setObjectName("DefaultCustom");
        foreach (auto& action, list)
        {
            toolbar->addAction(a_map[action]);
        }
        settings.setValue("CustomToolbars/DefaultCustom", list);
        addToolBar(Qt::LeftToolBarArea, toolbar);
    }

    widget_factory.createMenus(menuBar());

    undoButton = a_map["EditUndo"];
    redoButton = a_map["EditRedo"];
    previousZoom = a_map["ZoomPrevious"];

    dock_areas.left = a_map["LeftDockAreaToggle"];
    dock_areas.right = a_map["RightDockAreaToggle"];
    dock_areas.top = a_map["TopDockAreaToggle"];
    dock_areas.bottom = a_map["BottomDockAreaToggle"];
    dock_areas.floating = a_map["FloatingDockwidgetsToggle"];

    snapToolBar = widget_factory.snap_toolbar;
    penToolBar = widget_factory.pen_toolbar;
    optionWidget = widget_factory.options_toolbar;

    layerWidget = widget_factory.layer_widget;

    layerTreeWidget = widget_factory.layer_tree_widget;
    /**/
    blockWidget = widget_factory.block_widget;
    commandWidget = widget_factory.command_widget;

    penPaletteWidget = widget_factory.pen_palette;

    file_menu = widget_factory.file_menu;
    windowsMenu = widget_factory.windows_menu;

    connect(a_map["FileClose"], SIGNAL(triggered(bool)),
            mdiAreaCAD, SLOT(closeActiveSubWindow()));

    connect(penToolBar, SIGNAL(penChanged(RS_Pen)),
            this, SLOT(slotPenChanged(const RS_Pen&)));

    auto ctrl_l = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(ctrl_l, SIGNAL(activated()), actionHandler, SLOT(slotLayersAdd()));

    auto ctrl_m = new QShortcut(QKeySequence("Ctrl+M"), this);
    connect(ctrl_m, SIGNAL(activated()), this, SLOT(slotFocusCommandLine()));

    // This event filter allows sending key events to the command widget, therefore, no
    // need to activate the command widget before typing commands.
    // Since this nice feature causes a bug of lost key events when the command widget is on
    // a screen different from the main window, disabled for the time being
    // send key events for mdiAreaCAD to command widget by default
    mdiAreaCAD->installEventFilter(commandWidget);

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating dialogFactory");
    dialogFactory = new QC_DialogFactory(this, optionWidget);
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating dialogFactory: OK");

    RS_DEBUG->print("setting dialog factory object");
    if (RS_DialogFactory::instance()==nullptr) {
        RS_DEBUG->print("no RS_DialogFactory instance");
    } else {
        RS_DEBUG->print("got RS_DialogFactory instance");
    }
    RS_DialogFactory::instance()->setFactoryObject(dialogFactory);
    RS_DEBUG->print("setting dialog factory object: OK");

    recentFiles = new QG_RecentFiles(this, 9);
    auto recent_menu = new QMenu(tr("Recent Files"), file_menu);
    file_menu->addMenu(recent_menu);
    recentFiles->addFiles(recent_menu);

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init settings");
    initSettings();

    auto command_file = settings.value("Paths/VariableFile", "").toString();
    if (!command_file.isEmpty())
        commandWidget->leCommand->readCommandFile(command_file);

    // Activate autosave timer
    bool allowAutoSave = settings.value("Defaults/AutoBackupDocument", 1).toBool();
    startAutoSave(allowAutoSave);

    // Disable menu and toolbar items
    //emit windowsChanged(false);

    RS_COMMANDS->updateAlias();
    //plugin load
    loadPlugins();

    statusBar()->showMessage(qApp->applicationName() + " Ready", 2000);
}

void QC_ApplicationWindow::startAutoSave(bool startAutoBackup)
{
    if (startAutoBackup)
    {
        if (m_autosaveTimer == nullptr) {
            m_autosaveTimer = std::make_unique<QTimer>(this);
            m_autosaveTimer->setObjectName("autosave");
            connect(m_autosaveTimer.get(), SIGNAL(timeout()), this, SLOT(slotFileAutoSave()));
        }
        if (!m_autosaveTimer->isActive()) {
            // autosaving has been turned on. Make a backup immediately
            auto groupGuard = RS_SETTINGS->beginGroupGuard("/Defaults");
            RS_SETTINGS->writeEntry("/AutoBackupDocument", 1);
            slotFileAutoSave();
            int ms = 60000 * RS_SETTINGS->readNumEntry("AutoSaveTime", 5);
            m_autosaveTimer->start(ms);
        }
    } else {
        m_autosaveTimer.reset();
    }
}

/**
 * @brief QC_ApplicationWindow::getAppWindow() accessor for the application window singleton instance
 * @return QC_ApplicationWindow* the application window instance
 */
std::unique_ptr<QC_ApplicationWindow>& QC_ApplicationWindow::getAppWindow()
{
    static auto instance = std::unique_ptr<QC_ApplicationWindow>(new QC_ApplicationWindow);
    // singleton could be reset: cannot be called after reseting
    Q_ASSERT(instance != nullptr);
    return instance;
}

/**
  * Find a menu entry in the current menu list. This function will try to recursively find the menu
  * searchMenu for example foo/bar
  * thisMenuList list of Widgets
  * currentEntry only used internally during recursion
  * returns 0 when no menu was found
  */
QMenu *QC_ApplicationWindow::findMenu(const QString &searchMenu, const QObjectList thisMenuList, const QString& currentEntry) {
    if (searchMenu==currentEntry)
        return ( QMenu *)thisMenuList.at(0)->parent();

    QList<QObject*>::const_iterator i=thisMenuList.begin();
    while (i != thisMenuList.end()) {
        if ((*i)->inherits ("QMenu")) {
            QMenu *ii=(QMenu*)*i;
            if (QMenu *foundMenu=findMenu(searchMenu, ii->children(), currentEntry+"/"+ii->objectName().replace("&", ""))) {
                return foundMenu;
            }
        }
        ++i;
    }
    return 0;
}

/**
 * Arrange the sub-windows as specified, and set the setting.
 * Note: Tab mode always uses (and sets) the RS2::Maximized mode.
 * @param m the layout mode; if set to RS2::CurrentMode, read the current setting
 * @param actuallyDont just set the setting, don't actually do the arrangement
 */
void QC_ApplicationWindow::doArrangeWindows(RS2::SubWindowMode m, bool actuallyDont)
{
	RS_SETTINGS->beginGroup("/WindowOptions");
	int mode = m != RS2::CurrentMode ? m : RS_SETTINGS->readNumEntry("/SubWindowMode", RS2::Maximized);
	RS_SETTINGS->endGroup();
	if (!actuallyDont) switch (mode) {
	case RS2::Maximized:
		if (mdiAreaCAD->currentSubWindow())
			mdiAreaCAD->currentSubWindow()->showMaximized();
		break;
	case RS2::Cascade:
		slotCascade();
		break;
	case RS2::Tile:
		slotTile();
		break;
	case RS2::TileHorizontal:
		slotTileHorizontal();
		break;
	case RS2::TileVertical:
		slotTileVertical();
		break;
	}
	
	RS_SETTINGS->beginGroup("/WindowOptions");
	RS_SETTINGS->writeEntry("/SubWindowMode", mode);
	RS_SETTINGS->endGroup();
}

/**
 * Set the QTabWidget shape and position for the MDI area; also the settings.
 * Note: setting a Tab layout always sets the window arrangement to RS2::Maximized
 * Used by the Drawing > Layout menu.
 * @param s the tab shape; if RS2::AnyShape read the current setting
 * @param p the tab bar position; if RS2::AnyPosition read the current setting
 */
void QC_ApplicationWindow::setTabLayout(RS2::TabShape s, RS2::TabPosition p)
{
	RS_SETTINGS->beginGroup("/WindowOptions");
	int shape = s != RS2::AnyShape ? s : RS_SETTINGS->readNumEntry("/TabShape", RS2::Triangular);
	int position = p != RS2::AnyPosition ? p : RS_SETTINGS->readNumEntry("/TabPosition", RS2::West);
	RS_SETTINGS->endGroup();
	mdiAreaCAD->setTabShape(static_cast<QTabWidget::TabShape>(shape));
	mdiAreaCAD->setTabPosition(static_cast<QTabWidget::TabPosition>(position));
	doArrangeWindows(RS2::Maximized);
	RS_SETTINGS->beginGroup("/WindowOptions");
	RS_SETTINGS->writeEntry("/TabShape", shape);
	RS_SETTINGS->writeEntry("/TabPosition", position);
	RS_SETTINGS->endGroup();
}

/**
 * Force-Save(as) the content of the sub window.  Retry on failure.
 * @return true success (or window was not modified)
 * @return false user cancelled (or window was null)
 */
bool QC_ApplicationWindow::doSave(QC_MDIWindow * w, bool forceSaveAs)
{
	QString name, msg;
	bool cancelled;
	if (!w) return false;
    if (w->getDocument()->isModified() || forceSaveAs) {
		name = w->getDocument()->getFilename();
		if (name.isEmpty())
			doActivate(w); // show the user the drawing for save as
		msg = name.isEmpty() ? tr("Saving drawing...") : tr("Saving drawing: %1").arg(name);
		statusBar()->showMessage(msg);
		bool res = forceSaveAs ? w->slotFileSaveAs(cancelled) : w->slotFileSave(cancelled);
		if (res) {
			if (cancelled) {
				statusBar()->showMessage(tr("Save cancelled"), 2000);
				return false;
			}
			name = w->getDocument()->getFilename();			
			msg = tr("Saved drawing: %1").arg(name);
			statusBar()->showMessage(msg, 2000);
			commandWidget->appendHistory(msg);

            if (!recentFiles->contains(name)) {
                recentFiles->add(name);
            }

			w->setWindowTitle(format_filename_caption(name) + "[*]");
			if (w->getGraphicView()->isDraftMode())
				w->setWindowTitle(w->windowTitle() + " [" + tr("Draft Mode") + "]");

            auto groupGuard = RS_SETTINGS->beginGroupGuard("/Defaults");
            bool autoBackup = RS_SETTINGS->readNumEntry("/AutoBackupDocument", 1) == 1;
            startAutoSave(autoBackup);
		}
		else {
			msg = tr("Cannot save the file ") +
				w->getDocument()->getFilename()
				+ tr(" , please check the filename and permissions.");
			statusBar()->showMessage(msg, 2000);
			commandWidget->appendHistory(msg);
			return doSave(w, true);
		}
	}
	return true;
}

/**
 * Force-Close this sub window.
 * @param activateNext also activate the next window in the window_list, if any
 */
void QC_ApplicationWindow::doClose(QC_MDIWindow * w, bool activateNext)
{
	RS_DEBUG->print("QC_ApplicationWindow::doClose begin");
	w->getGraphicView()->killAllActions();
	QC_MDIWindow* parentWindow = w->getParentWindow();
	if (parentWindow)
	{
		RS_DEBUG->print("QC_ApplicationWindow::doClose closing block or print preview");
		parentWindow->removeChildWindow(w);
	}
	else
	{
		RS_DEBUG->print("QC_ApplicationWindow::doClose closing graphic");
	}
    foreach (auto&& child, w->getChildWindows()) // block editors and print previews; just force these closed
		doClose(child, false); // they belong to the document (changes already saved there)
	w->getChildWindows().clear();
	mdiAreaCAD->removeSubWindow(w);
	window_list.removeOne(w);

	if (!activedMdiSubWindow || activedMdiSubWindow == w)
	{
		layerWidget->setLayerList(nullptr, false);

        if (layerTreeWidget != nullptr) {
            layerTreeWidget->setLayerList(nullptr);
            layerTreeWidget->set_view(nullptr);
            layerTreeWidget->set_document(nullptr);
        }


		blockWidget->setBlockList(nullptr);
		coordinateWidget->setGraphic(nullptr);
	}

 if (penPaletteWidget != nullptr){
     penPaletteWidget->setLayerList(nullptr);
 }

	openedFiles.removeAll(w->getDocument()->getFilename());

	activedMdiSubWindow = nullptr;
	actionHandler->set_view(nullptr);
	actionHandler->set_document(nullptr);

    if (activateNext && !window_list.empty()) {
        if (nullptr != parentWindow) {
            doActivate(parentWindow);
        }
        else {
            doActivate(window_list.back());
        }
    }

    RS_DEBUG->print("QC_ApplicationWindow::doClose end");
}

/**
 * Force-Activate this sub window.
 */
void QC_ApplicationWindow::doActivate(QMdiSubWindow * w)
{
	RS_SETTINGS->beginGroup("/WindowOptions");
	bool maximized = RS_SETTINGS->readNumEntry("/Maximized");
	RS_SETTINGS->endGroup();
	if (w) {
		slotWindowActivated(w, true);
		w->activateWindow();
		w->raise();
		w->setFocus();
        if (maximized || QMdiArea::TabbedView == mdiAreaCAD->viewMode()) {
            w->showMaximized();
        }
        else {
            w->show();
        }
    }
	if (mdiAreaCAD->viewMode() == QMdiArea::SubWindowView)
		doArrangeWindows(RS2::CurrentMode);
	enableFileActions(qobject_cast<QC_MDIWindow*>(w));
}

/**
 * Show a Save/Close/Cancel(All) dialog for the content of this sub-window.
 * The window handle must not be null, and the document must actually have been modified.
 *
 * @param showSaveAll show a Save All button and rename Close -> Close All
 * @return QG_ExitDialog::ExitDialogResult the button that was pressed, or -1 if invoked in error
 * @see QG_ExitDialog
 */
int QC_ApplicationWindow::showCloseDialog(QC_MDIWindow * w, bool showSaveAll)
{
	QG_ExitDialog dlg(this);
	dlg.setShowSaveAll(showSaveAll);
	dlg.setTitle(tr("Closing Drawing"));
	if (w && w->getDocument()->isModified()) {
		QString fn = w->getDocument()->getFilename();
		if (fn.isEmpty())
			fn = w->windowTitle();
		else if (fn.length() > 50)
			fn = QString("%1...%2").arg(fn.left(24)).arg(fn.right(24));

		dlg.setText(tr("Save changes to the following item?\n%1").arg(fn));
		return dlg.exec();
	}
	return -1; // should never get here; please send only modified documents
}

/**
 * Enable the available file actions for this sub-window.
 */
void QC_ApplicationWindow::enableFileActions(QC_MDIWindow* w)
{
	if (!w || w->getDocument()->getFilename().isEmpty()) {
		a_map["FileSave"]->setText(tr("&Save"));
		a_map["FileSaveAs"]->setText(tr("Save &as..."));
	}
	else {
		QString name = format_filename_caption(w->getDocument()->getFilename());
		a_map["FileSave"]->setText(tr("&Save %1").arg(name));
		a_map["FileSaveAs"]->setText(tr("Save %1 &as...").arg(name));
	}
	a_map["FileSave"]->setEnabled(w);
	a_map["FileSaveAs"]->setEnabled(w);
	a_map["FileSaveAll"]->setEnabled(w && window_list.count() > 1);
	a_map["FileExportMakerCam"]->setEnabled(w);
	a_map["FilePrintPDF"]->setEnabled(w);
	a_map["FileExport"]->setEnabled(w);
	a_map["FilePrint"]->setEnabled(w);
	a_map["FilePrintPreview"]->setEnabled(w);
	a_map["FileClose"]->setEnabled(w);
	a_map["FileCloseAll"]->setEnabled(w && window_list.count() > 1);
}

/**
 * Loads the found plugins.
 */
void QC_ApplicationWindow::loadPlugins() {

    loadedPlugins.clear();
    QStringList lst = RS_SYSTEM->getDirectoryList("plugins");
    // Keep track of plugin filenames loaded to skip duplicate plugins.
    QStringList loadedPluginFileNames;

    for (int i = 0; i < lst.size(); ++i) {
        QDir pluginsDir(lst.at(i));
        for(const QString& fileName: pluginsDir.entryList(QDir::Files)) {
            // Skip loading a plugin if a plugin with the same
            // filename has already been loaded.
            #ifdef Q_OS_MAC
            if (!fileName.contains(".dylib"))
                continue;
            #endif
#if (defined (_WIN32) || defined (_WIN32) || defined (_WIN64))
            if (!fileName.contains(".dll"))
                continue;
#endif

            if (loadedPluginFileNames.contains(fileName)) {
                continue;
            }
            QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
            QObject *plugin = pluginLoader.instance();
            if (plugin) {
                QC_PluginInterface *pluginInterface = qobject_cast<QC_PluginInterface *>(plugin);
                if (pluginInterface) {
                    loadedPlugins.push_back(pluginInterface);
                    loadedPluginFileNames.push_back(fileName);
                    PluginCapabilities pluginCapabilities=pluginInterface->getCapabilities();
                    for(const PluginMenuLocation& loc: pluginCapabilities.menuEntryPoints) {
                        QAction *actpl = new QAction(loc.menuEntryActionName, plugin);
                        actpl->setData(loc.menuEntryActionName);
                        connect(actpl, SIGNAL(triggered()), this, SLOT(execPlug()));
                        connect(this, SIGNAL(windowsChanged(bool)), actpl, SLOT(setEnabled(bool)));
                        QMenu *atMenu = findMenu("/"+loc.menuEntryPoint, menuBar()->children(), "");
                        if (atMenu) {
                            atMenu->addAction(actpl);
                        } else {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                            QStringList treemenu = loc.menuEntryPoint.split('/', Qt::SkipEmptyParts);
#else
                            QStringList treemenu = loc.menuEntryPoint.split('/', QString::SkipEmptyParts);
#endif
                            QString currentLevel="";
                            QMenu *parentMenu=0;
                            do {
                                QString menuName=treemenu.at(0); treemenu.removeFirst();
                                currentLevel=currentLevel+"/"+menuName;
                                atMenu = findMenu(currentLevel, menuBar()->children(), "");
                                if (atMenu==0) {
                                    if (parentMenu==0) {
                                        parentMenu=menuBar()->addMenu(menuName);
                                    } else {
                                        parentMenu=parentMenu->addMenu(menuName);
                                    }
                                    parentMenu->setObjectName(menuName);
                                }
                            } while(treemenu.size()>0);
							if (parentMenu) parentMenu->addAction(actpl);
                        }
                    }
                }
            } else {
                QMessageBox::information(this, "Info", pluginLoader.errorString());
                RS_DEBUG->print("QC_ApplicationWindow::loadPlugin: %s", pluginLoader.errorString().toLatin1().data());
            }
        }
    }
}

/**
 * Execute the plugin.
 */
void QC_ApplicationWindow::execPlug() {
    QAction *action = qobject_cast<QAction *>(sender());
    QC_PluginInterface *plugin = qobject_cast<QC_PluginInterface *>(action->parent());
//get actual drawing
    QC_MDIWindow* w = getMDIWindow();
    RS_Document* currdoc = w->getDocument();
//create document interface instance
    Doc_plugin_interface pligundoc(currdoc, w->getGraphicView(), this);
//execute plugin
    LC_UndoSection undo(currdoc);
    plugin->execComm(&pligundoc, this, action->data().toString());
//TODO call update view
w->getGraphicView()->redraw();
}


/**
 * Destructor.
 */
QC_ApplicationWindow::~QC_ApplicationWindow() {
    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow");

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting dialog factory");

#ifdef _WINDOWS
	qt_ntfs_permission_lookup--; // turn it off again
#endif

    delete dialogFactory;

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting dialog factory: OK");
}


/**
 * Close Event. Called when the user tries to close the app.
 */
void QC_ApplicationWindow::closeEvent(QCloseEvent* ce)
{
    RS_DEBUG->print("QC_ApplicationWindow::closeEvent()");

    queryExit(false) ? ce->accept() : ce->ignore();

    RS_DEBUG->print("QC_ApplicationWindow::closeEvent(): OK");
}

void QC_ApplicationWindow::dropEvent(QDropEvent* event)
{
    event->acceptProposedAction();

    //limit maximum number of dropped files to be opened
    unsigned counts=0;
    for(QUrl const& url: event->mimeData()->urls()) {
        const QString &fileName = url.toLocalFile();
        if(QFileInfo(fileName).exists() && fileName.endsWith(R"(.dxf)", Qt::CaseInsensitive)){
            slotFileOpen(fileName);
            if(++counts>32) return;
        }
    }
}

void 	QC_ApplicationWindow::dragEnterEvent(QDragEnterEvent * event)
{
    if (event->mimeData()->hasUrls()){
        for(QUrl const& url: event->mimeData()->urls()) {
            const QString &fileName = url.toLocalFile();
            if(QFileInfo(fileName).exists() && fileName.endsWith(R"(.dxf)", Qt::CaseInsensitive)){
                event->acceptProposedAction();
                return;
            }
        }
    }
}

/**
 * @return Pointer to the currently active MDI Window or nullptr if no
 * MDI Window is active.
 */
QC_MDIWindow const* QC_ApplicationWindow::getMDIWindow() const{
    if (mdiAreaCAD) {
        QMdiSubWindow* w=mdiAreaCAD->currentSubWindow();
        if(w) {
            return qobject_cast<QC_MDIWindow*>(w);
        }
    }
    return nullptr;
}

QC_MDIWindow* QC_ApplicationWindow::getMDIWindow(){
    if (mdiAreaCAD) {
        QMdiSubWindow* w=mdiAreaCAD->currentSubWindow();
        if(w) {
            return qobject_cast<QC_MDIWindow*>(w);
        }
    }
    return nullptr;
}

void QC_ApplicationWindow::setPreviousZoomEnable(bool enable){
    previousZoomEnable=enable;
    if(previousZoom){
        previousZoom->setEnabled(enable);
    }
}

void QC_ApplicationWindow::setUndoEnable(bool enable){
    undoEnable=enable;
    if(undoButton){
        undoButton->setEnabled(enable);
    }
}

void QC_ApplicationWindow::setRedoEnable(bool enable){
    redoEnable=enable;
    if(redoButton){
        redoButton->setEnabled(enable);
    }
}

void QC_ApplicationWindow::slotEnableActions(bool enable) {
    if(previousZoom){
        previousZoom->setEnabled(enable&& previousZoomEnable);
        undoButton->setEnabled(enable&& undoEnable);
        redoButton->setEnabled(enable&& redoEnable);
    }
}

void QC_ApplicationWindow::slotUpdateActiveLayer()
{
    if(layerWidget&&m_pActiveLayerName)
        m_pActiveLayerName->activeLayerChanged(layerWidget->getActiveName());
}

/**
 * Initializes the global application settings from the
 * config file (unix, mac) or registry (windows).
 */
void QC_ApplicationWindow::initSettings()
{
    RS_DEBUG->print("QC_ApplicationWindow::initSettings()");

    QSettings settings;

    settings.beginGroup("Geometry");
    restoreState(settings.value("StateOfWidgets", "").toByteArray());
    dock_areas.left->setChecked(settings.value("LeftDockArea", 0).toBool());
    dock_areas.right->setChecked(settings.value("RightDockArea", 1).toBool());
    dock_areas.top->setChecked(settings.value("TopDockArea", 0).toBool());
    dock_areas.bottom->setChecked(settings.value("BottomDockArea", 0).toBool());
    dock_areas.floating->setChecked(settings.value("FloatingDockwidgets", 0).toBool());
    settings.endGroup();

    settings.beginGroup("Widgets");

    int allow_style = settings.value("AllowStyle", 0).toInt();
    if (allow_style)
    {
        QString style = settings.value("Style", "").toString();
        QApplication::setStyle(QStyleFactory::create(style));
    }

    QString sheet_path = settings.value("StyleSheet", "").toString();
    if (loadStyleSheet(sheet_path))
        style_sheet_path = sheet_path;
    settings.endGroup();

    a_map["ViewDraft"]->setChecked(settings.value("Appearance/DraftMode", 0).toBool());
}


/**
 * Stores the global application settings to file or registry.
 */
void QC_ApplicationWindow::storeSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::storeSettings()");

    if (RS_Settings::save_is_allowed)
    {
        RS_SETTINGS->beginGroup("/Geometry");
        RS_SETTINGS->writeEntry("/WindowWidth", width());
        RS_SETTINGS->writeEntry("/WindowHeight", height());
        RS_SETTINGS->writeEntry("/WindowX", x());
        RS_SETTINGS->writeEntry("/WindowY", y());
        QString geometry {saveGeometry().toBase64(QByteArray::Base64Encoding)};
        RS_SETTINGS->writeEntry("/WindowGeometry", geometry);
        RS_SETTINGS->writeEntry("/StateOfWidgets", QVariant (saveState()));
        RS_SETTINGS->writeEntry("/LeftDockArea", dock_areas.left->isChecked());
        RS_SETTINGS->writeEntry("/RightDockArea", dock_areas.right->isChecked());
        RS_SETTINGS->writeEntry("/TopDockArea", dock_areas.top->isChecked());
        RS_SETTINGS->writeEntry("/BottomDockArea", dock_areas.bottom->isChecked());
        RS_SETTINGS->writeEntry("/FloatingDockwidgets", dock_areas.floating->isChecked());
        RS_SETTINGS->endGroup();
        //save snapMode
        snapToolBar->saveSnapMode();
    }

    RS_DEBUG->print("QC_ApplicationWindow::storeSettings(): OK");
}

/**
 * Goes back to the previous menu or one step in the current action.
 */
void QC_ApplicationWindow::slotBack() {
    RS_GraphicView* graphicView = getGraphicView();
    if (graphicView) {
        graphicView->back();
    }
}

void QC_ApplicationWindow::slotKillAllActions() {
    RS_GraphicView* gv = getGraphicView();
    QC_MDIWindow* m = getMDIWindow();
    if (gv && m && m->getDocument()) {
        gv->killAllActions();

        RS_Selection s((RS_EntityContainer&)*m->getDocument(), gv);
        s.selectAll(false);
        RS_DIALOGFACTORY->updateSelectionWidget(
                    m->getDocument()->countSelected()
                    ,
                    m->getDocument()->totalSelectedLength()
                    );

        gv->redraw(RS2::RedrawAll);
    }
}


/**
 * Goes one step further in the current action.
 */
void QC_ApplicationWindow::slotEnter()
{
    RS_DEBUG->print("QC_ApplicationWindow::slotEnter(): begin\n");
    RS_GraphicView* graphicView = getGraphicView();
    if (graphicView)
    {
        graphicView->enter();
    }
    RS_DEBUG->print("QC_ApplicationWindow::slotEnter(): end\n");
}

/**
 * Sets the keyboard focus on the command line.
 */
void QC_ApplicationWindow::slotFocusCommandLine() {
//    if (commandWidget->isVisible()) {
        commandWidget->show();
        commandWidget->setFocus();
//    }
}


/**
 * Shows the given error on the command line.
 */
void QC_ApplicationWindow::slotError(const QString& msg) {
        commandWidget->appendHistory(msg);
}

/**
 * Hands focus back to the application window. In the rare event
 * of a escape press from the layer widget (e.g after switching desktops
 * in XP).
 */
void QC_ApplicationWindow::slotFocus() {
    setFocus();
}

void QC_ApplicationWindow::slotWindowActivated(int index){
    if(index < 0 || index >= mdiAreaCAD->subWindowList().size()) return;
    slotWindowActivated(mdiAreaCAD->subWindowList().at(index));
}

/**
 * Called when a document window was activated.
 */
void QC_ApplicationWindow::slotWindowActivated(QMdiSubWindow* w, bool forced)
{
    RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated begin");

    if(w==nullptr) {
        emit windowsChanged(false);
        activedMdiSubWindow=w;
        return;
    }

    if(w==activedMdiSubWindow) return;
    activedMdiSubWindow=w;

    QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(w);
    enableFileActions(m);

    if (m && m->getDocument()) {

        RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated: "
                        "document: %lu", m->getDocument()->getId());

        bool showByBlock = m->getDocument()->rtti()==RS2::EntityBlock;

        RS_LayerList *layerList = m->getDocument()->getLayerList();

        layerWidget->setLayerList(layerList,showByBlock);

        if (layerTreeWidget != nullptr) {
            layerTreeWidget->setLayerList(layerList);
            layerTreeWidget->set_view(m->getGraphicView());
            layerTreeWidget->set_document(m->getDocument());
        }

        if (penPaletteWidget != nullptr){
            penPaletteWidget->setLayerList(layerList);
        }


        coordinateWidget->setGraphic(m->getGraphic());

        blockWidget->setBlockList(m->getDocument()->getBlockList());

        // Update all inserts in this graphic (blocks might have changed):
        m->getDocument()->updateInserts();
        // whether to enable undo/redo buttons
        m->getDocument()->setGUIButtons();
        m->getGraphicView()->redraw();

        // set snapmode from snap toolbar
        //actionHandler->updateSnapMode();

        // set pen from pen toolbar
        slotPenChanged(penToolBar->getPen());

        pen_wiz->setMdiWindow(m);
        if (penPaletteWidget != nullptr){
         penPaletteWidget->setMdiWindow(m);
        }

        if (!forced)
        {
            // update toggle button status:
            if (m->getGraphic()) emit gridChanged(m->getGraphic()->isGridOn());
        }

        QG_GraphicView* view = m->getGraphicView();
        if (view)
        {
            actionHandler->set_view(view);
            actionHandler->set_document(m->getDocument());
            emit printPreviewChanged(view->isPrintPreview());
        }

        if(snapToolBar){
            actionHandler->slotSetSnaps(snapToolBar->getSnaps());
        }else {
            RS_DEBUG->print(RS_Debug::D_ERROR,"snapToolBar is nullptr\n");
        }
    }

    // show action options for active window only
    foreach (QMdiSubWindow* sw, mdiAreaCAD->subWindowList()) {
        QC_MDIWindow* sm = qobject_cast<QC_MDIWindow*>(sw);
        RS_ActionInterface* ai = sm->getGraphicView()->getCurrentAction();
        if (ai) {
            ai->hideOptions();
        }
    }
    if (m && m->getGraphicView()->getCurrentAction()) {
        m->getGraphicView()->getCurrentAction()->showOptions();
    }

    // Disable/Enable menu and toolbar items
    emit windowsChanged(m && m->getDocument());

    RS_DEBUG->print("RVT_PORT emit windowsChanged(true);");

    RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated end");
}

/**
 * Called when the menu 'windows' is about to be shown.
 * This is used to update the window list in the menu.
 */
void QC_ApplicationWindow::slotWindowsMenuAboutToShow() {

    RS_DEBUG->print( RS_Debug::D_NOTICE, "QC_ApplicationWindow::slotWindowsMenuAboutToShow");
	RS_SETTINGS->beginGroup("/WindowOptions");

	QMenu* menu;
	QAction* menuItem;
	bool tabbed = mdiAreaCAD->viewMode() == QMdiArea::TabbedView;
    windowsMenu->clear(); // this is a temporary menu; constructed on-demand

	menuItem = windowsMenu->addAction(tr("Ta&b mode"), this, SLOT(slotToggleTab()));
	menuItem->setCheckable(true);
	menuItem->setChecked(tabbed);

	menuItem = windowsMenu->addAction( tr("&Window mode"), this, SLOT(slotToggleTab()));
	menuItem->setCheckable(true);
	menuItem->setChecked(!tabbed);

	
	if (mdiAreaCAD->viewMode() == QMdiArea::TabbedView) {
		menu = new QMenu(tr("&Layout"), windowsMenu);
		windowsMenu->addMenu(menu);

		menuItem = menu->addAction(tr("Rounded"), this, SLOT(slotTabShapeRounded()));
		menuItem->setCheckable(true);
		menuItem->setChecked(RS_SETTINGS->readNumEntry("/TabShape") == RS2::Rounded);

		menuItem = menu->addAction(tr("Triangular"), this, SLOT(slotTabShapeTriangular()));
		menuItem->setCheckable(true);
		menuItem->setChecked(RS_SETTINGS->readNumEntry("/TabShape") == RS2::Triangular);

		menu->addSeparator();

		menuItem = menu->addAction(tr("North"), this, SLOT(slotTabPositionNorth()));
		menuItem->setCheckable(true);
		menuItem->setChecked(RS_SETTINGS->readNumEntry("/TabPosition") == RS2::North);

		menuItem = menu->addAction(tr("South"), this, SLOT(slotTabPositionSouth()));
		menuItem->setCheckable(true);
		menuItem->setChecked(RS_SETTINGS->readNumEntry("/TabPosition") == RS2::South);

		menuItem = menu->addAction(tr("East"), this, SLOT(slotTabPositionEast()));
		menuItem->setCheckable(true);
		menuItem->setChecked(RS_SETTINGS->readNumEntry("/TabPosition") == RS2::East);

		menuItem = menu->addAction(tr("West"), this, SLOT(slotTabPositionWest()));
		menuItem->setCheckable(true);
		menuItem->setChecked(RS_SETTINGS->readNumEntry("/TabPosition") == RS2::West);

	} else {
		menu = new QMenu(tr("&Arrange"), windowsMenu);
		windowsMenu->addMenu(menu);

		menuItem = menu->addAction(tr("&Maximized"), this, SLOT(slotSetMaximized()));
		menuItem->setCheckable(true);
		menuItem->setChecked(RS_SETTINGS->readNumEntry("/SubWindowMode") == RS2::Maximized);

        menu->addAction(tr("&Cascade"), this, SLOT(slotCascade()));
        menu->addAction(tr("&Tile"), this, SLOT(slotTile()));
        menu->addAction(tr("Tile &Vertically"), this, SLOT(slotTileVertical()));
        menu->addAction(tr("Tile &Horizontally"), this, SLOT(slotTileHorizontal()));
	}
	

	RS_SETTINGS->endGroup();
        
    windowsMenu->addSeparator();
    QMdiSubWindow* active= mdiAreaCAD->activeSubWindow();
    for (int i=0; i< window_list.size(); ++i) {
        QString title = window_list.at(i)->windowTitle();
        if (title.contains("[*]")) { // modification mark placeholder
            int idx = title.lastIndexOf("[*]");
            if (window_list.at(i)->isWindowModified()) {
                title.replace(idx, 3, "*");
            } else {
                title.remove(idx, 3);
            }
        }
        QAction *id = windowsMenu->addAction(title,
                                         this, SLOT(slotWindowsMenuActivated(bool)));
        id->setCheckable(true);
        id->setData(i);
        id->setChecked(window_list.at(i)==active);
    }
}

/**
 * Called when the user selects a document window from the
 * window list.
 */
void QC_ApplicationWindow::slotWindowsMenuActivated(bool /*id*/) {
    RS_DEBUG->print("QC_ApplicationWindow::slotWindowsMenuActivated");

    int ii = qobject_cast<QAction*>(sender())->data().toInt();
    QMdiSubWindow* w = mdiAreaCAD->subWindowList().at(ii);
    if (w) {
        if(w==mdiAreaCAD->activeSubWindow()) {
            return;
        }

		doActivate(w);
    }
}

/**
 * Cascade MDI windows
 */
void QC_ApplicationWindow::slotTile() {
	doArrangeWindows(RS2::Tile, true);
        mdiAreaCAD->tileSubWindows();
        slotZoomAuto();
}
//auto zoom the graphicView of sub-windows
void QC_ApplicationWindow::slotZoomAuto() {
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    for(int i=0;i<windows.size();i++){
        QMdiSubWindow *window = windows.at(i);
        qobject_cast<QC_MDIWindow*>(window)->slotZoomAuto();
    }
}

/**
 * Cascade MDI windows
 */
void QC_ApplicationWindow::slotCascade() {
//    mdiAreaCAD->cascadeSubWindows();
//return;
	doArrangeWindows(RS2::Cascade, true);
	QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    switch(windows.size()){
    case 1:
        //mdiAreaCAD->tileSubWindows();
		slotTile();
    case 0:
        return;
    default: {
        QMdiSubWindow* active=mdiAreaCAD->activeSubWindow();
        for (int i=0; i<windows.size(); ++i) {
            windows.at(i)->showNormal();
        }
        mdiAreaCAD->cascadeSubWindows();
        //find displacement by linear-regression
        double mi=0.,mi2=0.,mw=0.,miw=0.,mh=0.,mih=0.;
        for (int i=0; i<windows.size(); ++i) {
                mi += i;
                mi2 += i*i;
                double w=windows.at(i)->pos().x();
                mw += w;
                miw += i*w;
                double h=windows.at(i)->pos().y();
                mh += h;
                mih += i*h;
        }
        mi2 *= windows.size();
        miw *= windows.size();
        mih *= windows.size();
        double d=1./(mi2 - mi*mi);
        double disX=(miw-mi*mw)*d;
        double disY=(mih-mi*mh)*d;
        //End of Linear Regression
        //
        QMdiSubWindow *window = windows.first();
        QRect geo=window->geometry();
        QRect frame=window->frameGeometry();
//        std::cout<<"Frame=:"<<( frame.height() - geo.height())<<std::endl;
        int width= mdiAreaCAD->width() -( frame.width() - geo.width())- disX*(windows.size()-1);
        int height= mdiAreaCAD->height() -( frame.width() - geo.width())- disY*(windows.size()-1);
        if(width<=0 || height<=0) {
            return;
        }
        for (int i=0; i<windows.size(); ++i) {
            window = windows.at(i);
//            std::cout<<"window:("<<i<<"): pos()="<<(window->pos().x())<<" "<<(window->pos().y())<<std::endl;
            geo=window->geometry();
//            if(i==active) {
//                    window->setWindowState(Qt::WindowActive);
//            }else{
//                    window->setWindowState(Qt::WindowNoState);
//            }
            window->setGeometry(geo.x(),geo.y(),width,height);
            qobject_cast<QC_MDIWindow*>(window)->slotZoomAuto();
        }
        mdiAreaCAD->setActiveSubWindow(active);
//        windows.at(active)->activateWindow();
//        windows.at(active)->raise();
//        windows.at(active)->setFocus();
    }
    }
}


/**
 * Tiles MDI windows horizontally.
 */
void QC_ApplicationWindow::slotTileHorizontal() {

    RS_DEBUG->print("QC_ApplicationWindow::slotTileHorizontal");
	doArrangeWindows(RS2::TileHorizontal, true);
	
    // primitive horizontal tiling
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    if (windows.count()<=1) {
		slotTile();
        return;
    }
    for (int i=0; i<windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        window->lower();
        window->showNormal();
    }
    int heightForEach = mdiAreaCAD->height() / windows.count();
    int y = 0;
    for (int i=0; i<windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        int preferredHeight = window->minimumHeight()
                              + window->parentWidget()->baseSize().height();
        int actHeight = qMax(heightForEach, preferredHeight);

        window->setGeometry(0, y, mdiAreaCAD->width(), actHeight);
         qobject_cast<QC_MDIWindow*>(window)->slotZoomAuto();
        y+=actHeight;
    }
    mdiAreaCAD->activeSubWindow()->raise();
}


/**
 * Tiles MDI windows vertically.
 */
void QC_ApplicationWindow::slotTileVertical() {
	
    RS_DEBUG->print("QC_ApplicationWindow::slotTileVertical()");
	doArrangeWindows(RS2::TileVertical, true);
	
    // primitive horizontal tiling
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    if (windows.count()<=1) {
		slotTile();
        return;
    }
    for (int i=0; i<windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        window->lower();
        window->showNormal();
    }
    int widthForEach = mdiAreaCAD->width() / windows.count();
    int x = 0;
    for (int i=0; i<windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        int preferredWidth = window->minimumWidth()
                              + window->parentWidget()->baseSize().width();
        int actWidth = qMax(widthForEach, preferredWidth);

        window->setGeometry(x, 0, actWidth, mdiAreaCAD->height());
         qobject_cast<QC_MDIWindow*>(window)->slotZoomAuto();
        x+=actWidth;
    }
    mdiAreaCAD->activeSubWindow()->raise();
}

void QC_ApplicationWindow::slotSetMaximized()
{
	doArrangeWindows(RS2::Maximized);
}

void QC_ApplicationWindow::slotTabShapeRounded()
{
	setTabLayout(RS2::Rounded, RS2::AnyPosition);
}

void QC_ApplicationWindow::slotTabShapeTriangular()
{
	setTabLayout(RS2::Triangular, RS2::AnyPosition);
}

void QC_ApplicationWindow::slotTabPositionNorth()
{
	setTabLayout(RS2::AnyShape, RS2::North);
}

void QC_ApplicationWindow::slotTabPositionSouth()
{
	setTabLayout(RS2::AnyShape, RS2::South);
}

void QC_ApplicationWindow::slotTabPositionEast()
{
	setTabLayout(RS2::AnyShape, RS2::East);
}

void QC_ApplicationWindow::slotTabPositionWest()
{
	setTabLayout(RS2::AnyShape, RS2::West);
}

/**
 * toggles between subwindow and tab mode for the MdiArea
 */
void QC_ApplicationWindow::slotToggleTab()
{
    if (mdiAreaCAD->viewMode() == QMdiArea::SubWindowView)
    {
		RS_SETTINGS->beginGroup("Startup");
		RS_SETTINGS->writeEntry("/TabMode", 1);
		RS_SETTINGS->endGroup();
        mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
		QList<QTabBar *> tabBarList = mdiAreaCAD->findChildren<QTabBar*>();
		QTabBar *tabBar = tabBarList.at(0);
		if (tabBar) {
			tabBar->setExpanding(false);
		}
        QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
        QMdiSubWindow* active=mdiAreaCAD->activeSubWindow();
        for(int i=0;i<windows.size();i++){
            QMdiSubWindow* m=windows.at(i);
            m->hide();
            if(m!=active){
                m->lower();
            }else{
                m->raise();
            }
			slotSetMaximized();
            qobject_cast<QC_MDIWindow*>(m)->slotZoomAuto();
        }
    }
    else
    {
		RS_SETTINGS->beginGroup("Startup");
		RS_SETTINGS->writeEntry("/TabMode", 0);
		RS_SETTINGS->endGroup();
        mdiAreaCAD->setViewMode(QMdiArea::SubWindowView);
		doArrangeWindows(RS2::CurrentMode);
    }
}

/**
 * Called when something changed in the pen tool bar
 * (e.g. color, width, style).
 */
void QC_ApplicationWindow::slotPenChanged(RS_Pen pen) {
    RS_DEBUG->print("QC_ApplicationWindow::slotPenChanged() begin");

    RS_DEBUG->print("Setting active pen...");

    QC_MDIWindow* m = getMDIWindow();
    if (m) {
        m->slotPenChanged(pen);
    }

    RS_DEBUG->print("QC_ApplicationWindow::slotPenChanged() end");
}

///**
// * Called when something changed in the snaps tool bar
// */
//void QC_ApplicationWindow::slotSnapsChanged(const RS_SnapMode& snaps) {
//    RS_DEBUG->print("QC_ApplicationWindow::slotSnapsChanged() begin");

//    actionHandler->slotSetSnaps(snaps);
//}



/**
 * Creates a new MDI window with the given document or a new
 *  document if 'doc' is nullptr.
 */

QC_MDIWindow* QC_ApplicationWindow::slotFileNew(RS_Document* doc) {

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNew() begin");

    QSettings settings;
    static int id = 0;
    id++;

    statusBar()->showMessage(tr("Creating new file..."));

    RS_DEBUG->print("  creating MDI window");

    QC_MDIWindow *w = new QC_MDIWindow(doc, mdiAreaCAD, {});

    window_list << w;

    RS_SETTINGS->beginGroup("/Appearance");
    int aa = RS_SETTINGS->readNumEntry("/Antialiasing", 0);
    int scrollbars = RS_SETTINGS->readNumEntry("/ScrollBars", 1);
    int cursor_hiding = RS_SETTINGS->readNumEntry("/cursor_hiding", 0);
    RS_SETTINGS->endGroup();

    QG_GraphicView* view = w->getGraphicView();

    view->setAntialiasing(aa);
    view->setCursorHiding(cursor_hiding);
    view->device = settings.value("Hardware/Device", "Mouse").toString();
    if (scrollbars) view->addScrollbars();

    settings.beginGroup("Activators");
    auto activators = settings.childKeys();
    settings.endGroup();

    foreach (auto activator, activators)
    {
        auto menu_name = settings.value("Activators/" + activator).toString();
        auto path = QString("CustomMenus/%1").arg(menu_name);
        auto a_list = settings.value(path).toStringList();
        auto menu = new QMenu(activator, view);
        menu->setObjectName(menu_name);
        foreach (auto key, a_list)
        {
            menu->addAction(a_map[key]);
        }
        view->setMenu(activator, menu);
    }

    connect(view, SIGNAL(gridStatusChanged(QString)),
            this, SLOT(updateGridStatus(QString)));

    actionHandler->set_view(view);
    actionHandler->set_document(w->getDocument());

    connect(w, SIGNAL(signalClosing(QC_MDIWindow*)),
            this, SLOT(slotFileClosing(QC_MDIWindow*)));

    if (w->getDocument()->rtti()==RS2::EntityBlock) {
        w->setWindowTitle(tr("Block '%1'").arg(((RS_Block*)(w->getDocument()))->getName()) + "[*]");
    } else {
        w->setWindowTitle(tr("unnamed document %1").arg(id) + "[*]");
    }

    //check for draft mode

    if (settings.value("Appearance/DraftMode", 0).toBool())
    {
        QString draft_string = " ["+tr("Draft Mode")+"]";
        w->getGraphicView()->setDraftMode(true);
        QString title = w->windowTitle();
        w->setWindowTitle(title + draft_string);
    }

    w->setWindowIcon(QIcon(":/main/document.png"));

    RS_DEBUG->print("  adding listeners");
    RS_Graphic* graphic = w->getDocument()->getGraphic();

    RS_LayerList* layerList = w->getDocument()->getLayerList();

    if (layerWidget != nullptr) {
        layerWidget->setLayerList(layerList, false);
    }

    if (penPaletteWidget != nullptr){
        penPaletteWidget->setLayerList(layerList);
    }

    if(layerTreeWidget != nullptr) {
        layerTreeWidget->setLayerList(layerList);
        layerTreeWidget->set_view(view);
        layerTreeWidget->set_document(w->getDocument());
    }

    if(blockWidget) {
        blockWidget->setBlockList(w->getDocument()->getBlockList());
    }
    if (graphic) {
        // Link the graphic's layer list to the pen tool bar
        graphic->addLayerListListener(penToolBar);
        // Link the layer list to the layer widget
        graphic->addLayerListListener(layerWidget);

        if (layerTreeWidget != nullptr) {
            graphic->addLayerListListener(layerTreeWidget);
        }

        // Link the block list to the block widget
        graphic->addBlockListListener(blockWidget);
    }
	// Link the dialog factory to the coordinate widget:
	if( coordinateWidget){
		coordinateWidget->setGraphic(graphic );
	}
	// Link the dialog factory to the mouse widget:
	QG_DIALOGFACTORY->setMouseWidget(mouseWidget);
	QG_DIALOGFACTORY->setCoordinateWidget(coordinateWidget);
	QG_DIALOGFACTORY->setSelectionWidget(selectionWidget);
	// Link the dialog factory to the option widget:
	//QG_DIALOGFACTORY->setOptionWidget(optionWidget);
	// Link the dialog factory to the command widget:
	QG_DIALOGFACTORY->setCommandWidget(commandWidget);

    mdiAreaCAD->addSubWindow(w);

    RS_DEBUG->print("  showing MDI window");
	doActivate(w);
	doArrangeWindows(RS2::CurrentMode);
    statusBar()->showMessage(tr("New Drawing created."), 2000);

    layerWidget->activateLayer(0);

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNew() OK");

    return w;
}

/**
 * Helper function for Menu file -> New & New....
 */
bool QC_ApplicationWindow::slotFileNewHelper(QString fileName, QC_MDIWindow* w) {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper()");
    bool ret = false;
    RS2::FormatType type = RS2::FormatDXFRW;

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper: creating new doc window");
    /*QC_MDIWindow* */ w = slotFileNew();
    qApp->processEvents(QEventLoop::AllEvents, 1000);

    // link the layer widget to the new document:
    RS_LayerList *layerList = w->getDocument()->getLayerList();
    layerWidget->setLayerList(layerList, false);
    if (layerTreeWidget != nullptr)
        layerTreeWidget->setLayerList(layerList);

    if (penPaletteWidget != nullptr){
        penPaletteWidget->  setLayerList(layerList);
    }


    // link the block widget to the new document:
    blockWidget->setBlockList(w->getDocument()->getBlockList());
    // link coordinate widget to graphic
    coordinateWidget->setGraphic(w->getGraphic());

    qApp->processEvents(QEventLoop::AllEvents, 1000);

    // loads the template file in the new view:
    if (!fileName.isEmpty()) {
        ret = w->slotFileNewTemplate(fileName, type);
    } else
        //new without template is OK;
        ret = true;

    if (!ret) {
        // error loading template
        QApplication::restoreOverrideCursor();
        return ret;
    }

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper: load Template: OK");

    layerWidget->slotUpdateLayerList();
    if (layerTreeWidget != nullptr)
        layerTreeWidget->slotFilteringMaskChanged();

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper: update coordinate widget");
    // update coordinate widget format:
    RS_DIALOGFACTORY->updateCoordinateWidget(RS_Vector(0.0,0.0),
                                             RS_Vector(0.0,0.0), true);

    if (!fileName.isEmpty()) {
        QString message=tr("New document from template: ")+fileName;
        commandWidget->appendHistory(message);
        statusBar()->showMessage(message, 2000);
    }
    if (w->getGraphic()) {
        emit(gridChanged(w->getGraphic()->isGridOn()));
    }

    QApplication::restoreOverrideCursor();
    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper() OK");
    return ret;
}

/**
 * Menu file -> New (using a predefined Template).
 */
void QC_ApplicationWindow::slotFileNewNew() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewNew()");

//    RS2::FormatType type = RS2::FormatDXFRW;
    //tried to load template file indicated in RS_Settings
    RS_SETTINGS->beginGroup("/Paths");
    QString fileName = RS_SETTINGS->readEntry("/Template");
    RS_SETTINGS->endGroup();
/*    QFileInfo finfo(fileName);
    if (!fileName.isEmpty() && finfo.isReadable()) {
        slotFileNewTemplate(fileName, RS2::FormatDXFRW);
        return;
    }*/

	if (!slotFileNewHelper(fileName)) {
        // error opening template
        RS_DEBUG->print("QC_ApplicationWindow::slotFileNewNew: load Template failed");
    } else
        RS_DEBUG->print("QC_ApplicationWindow::slotFileNewNew() OK");
}

/**
 * Menu file -> New with Template.
 */
void QC_ApplicationWindow::slotFileNewTemplate() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewTemplate()");

    RS2::FormatType type = RS2::FormatDXFRW;
    QG_FileDialog dlg(this);
    QString fileName = dlg.getOpenFile(&type);

    if (fileName.isEmpty()) {
           statusBar()->showMessage(tr("Select Template aborted"), 2000);
           return;
       }

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewTemplate: creating new doc window");
    // Create new document window:
    QMdiSubWindow* old=activedMdiSubWindow;
    QRect geo;
    bool maximized=false;
    if(old != nullptr ) {//save old geometry
        geo=old->geometry();
        maximized=old->isMaximized();
    }
    QC_MDIWindow* w =nullptr;
	if (!slotFileNewHelper(fileName, w)) {
        // error
        QString msg=tr("Cannot open the file\n%1\nPlease "
                       "check the permissions.").arg(fileName);
        commandWidget->appendHistory(msg);
        QMessageBox::information(this, QMessageBox::tr("Warning"),
                                 msg,QMessageBox::Ok);
        //file opening failed, clean up QC_MDIWindow and QMdiSubWindow
        if (w) {
            slotFilePrintPreview(false);
            doClose(w); //force closing, without asking user for confirmation
        }
        QMdiSubWindow* active=mdiAreaCAD->currentSubWindow();
        activedMdiSubWindow=nullptr; //to allow reactivate the previous active
        if( active){//restore old geometry
            mdiAreaCAD->setActiveSubWindow(active);
            active->raise();
            active->setFocus();
            if(old==nullptr || maximized){
                active->showMaximized();
            }else{
                active->setGeometry(geo);
            }
        }
        RS_DEBUG->print("QC_ApplicationWindow::slotFileNewTemplate: load Template failed");
    } else
        RS_DEBUG->print("QC_ApplicationWindow::slotFileNewTemplate() OK");
}


/**
 * Menu file -> open.
 */
void QC_ApplicationWindow::slotFileOpen() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen()");

    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen() 001");
    RS2::FormatType type = RS2::FormatUnknown;
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen() 002");
    QG_FileDialog dlg(this);
    QString fileName = dlg.getOpenFile(&type);
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen() 003");
    slotFileOpen(fileName, type);
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(): OK");
}


/**
 *
 *	\brief	- Format a string that hold a file name path
 *						  such a way that it can displayed on the
 *						  windows title bar.
 *
 *	\author		Claude Sylvain
 *	\date			30 July 2011
 *	Last modified:
 *
 *	Parameters:		const QString &qstring_in:
 *							String to format (in).
 *
 *						QString &qstring_out:
 *							Formatted string (out).
 *
 *	Returns:			void
 *	*/

QString QC_ApplicationWindow::
    format_filename_caption(const QString &qstring_in)
{
	QFileInfo info = QFileInfo(qstring_in);
	return info.fileName(); // don't include the full path
}

/*	*
 *	Function name:
 *	Description:
 *	Author(s):		..., Claude Sylvain
 *	Created:			?
 *	Last modified:	30 July 2011
 *
 *	Parameters:		const QString& fileName:
 *							...
 *
 *						RS2::FormatType type:
 *							...
 *
 *	Returns:			void
 *	Notes:			Menu file -> open.
 *	*/
void QC_ApplicationWindow::
        slotFileOpen(const QString& fileName, RS2::FormatType type)
{
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(..)");

    QSettings settings;

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    if ( QFileInfo(fileName).exists())
         {
        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: creating new doc window");
        if (openedFiles.indexOf(fileName) >=0) {
            QString message=tr("Warning: File already opened : ")+fileName;
            commandWidget->appendHistory(message);
            statusBar()->showMessage(message, 2000);
        }
        // Create new document window:
		QMdiSubWindow* old=activedMdiSubWindow;
        QRect geo;
        //bool maximized=false;

        QC_MDIWindow* w = slotFileNew(nullptr);
        // RVT_PORT qApp->processEvents(1000);
        qApp->processEvents(QEventLoop::AllEvents, 1000);

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: linking layer list");
             RS_LayerList *layerList = w->getDocument()->getLayerList();
        // link the layer widget to the new document:

        layerWidget->setLayerList(layerList, false);
        if (layerTreeWidget)
            layerTreeWidget->setLayerList(layerList);
        if (penPaletteWidget != nullptr){
            penPaletteWidget->setLayerList(layerList);
         }
        // link the block widget to the new document:
        blockWidget->setBlockList(w->getDocument()->getBlockList());
        // link coordinate widget to graphic
        coordinateWidget->setGraphic(w->getGraphic());

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file");

        qApp->processEvents(QEventLoop::AllEvents, 1000);

        if(old != nullptr) {//save old geometry
            geo=old->geometry();
            //maximized=old->isMaximized();
        }

        // open the file in the new view:
        bool success=false;
        if (QFileInfo( fileName).exists()) {
            success = w->slotFileOpen( fileName, type);
        }
        else {
            QString msg=tr("Cannot open the file\n%1\nPlease "
                           "check its existence and permissions.")
                    .arg( fileName);
            commandWidget->appendHistory( msg);
            QMessageBox::information( this, QMessageBox::tr("Warning"), msg, QMessageBox::Ok);
        }
        if (!success) {
               // error
               QApplication::restoreOverrideCursor();

               //file opening failed, clean up QC_MDIWindow and QMdiSubWindow
               slotFilePrintPreview(false);
               doClose(w); //force closing, without asking user for confirmation
               return;
        }

		slotWindowActivated(w);

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: 1");

        // update recent files menu:
        recentFiles->add(fileName);
        openedFiles.push_back(fileName);
        layerWidget->slotUpdateLayerList();
        if (layerTreeWidget != nullptr)
            layerTreeWidget->slotFilteringMaskChanged();

        auto graphic = w->getGraphic();
        if (graphic)
        {
            if (int objects_removed = graphic->clean())
            {
                auto msg = QObject::tr("Invalid objects removed:");
                commandWidget->appendHistory(msg + " " + QString::number(objects_removed));
            }
            emit(gridChanged(graphic->isGridOn()));
        }

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: set caption");


                /*	Format and set caption.
                 *	----------------------- */
        w->setWindowTitle(format_filename_caption(fileName) + "[*]");

		if (mdiAreaCAD->viewMode() == QMdiArea::TabbedView) {
			QList<QTabBar *> tabBarList = mdiAreaCAD->findChildren<QTabBar*>();
			QTabBar *tabBar = tabBarList.at(0);
			if (tabBar) {
				tabBar->setExpanding(false);
				tabBar->setTabToolTip(tabBar->currentIndex(), fileName);
			}
		}
		else
			doArrangeWindows(RS2::CurrentMode);

		RS_SETTINGS->beginGroup("/CADPreferences");
		if (RS_SETTINGS->readNumEntry("/AutoZoomDrawing"))
			w->getGraphicView()->zoomAuto(false);
		RS_SETTINGS->endGroup();

        if (settings.value("Appearance/DraftMode", 0).toBool())
        {
            QString draft_string = " ["+tr("Draft Mode")+"]";
            w->getGraphicView()->setDraftMode(true);
            w->getGraphicView()->redraw();
            QString title = w->windowTitle();
            w->setWindowTitle(title + draft_string);
        }

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: set caption: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update coordinate widget");
        // update coordinate widget format:
        RS_DIALOGFACTORY->updateCoordinateWidget(RS_Vector(0.0,0.0),
                RS_Vector(0.0,0.0),
                true);
        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update coordinate widget: OK");

        QString message=tr("Loaded document: ")+fileName;
        commandWidget->appendHistory(message);
        statusBar()->showMessage(message, 2000);

	} else {
		QG_DIALOGFACTORY->commandMessage(tr("File '%1' does not exist. Opening aborted").arg(fileName));
        statusBar()->showMessage(tr("Opening aborted"), 2000);
    }

    QApplication::restoreOverrideCursor();
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(..) OK");
}

void QC_ApplicationWindow::slotFileOpen(const QString& fileName) {
    slotFileOpen(fileName, RS2::FormatUnknown);
}


/**
 * Menu file -> save.
 */
void QC_ApplicationWindow::slotFileSave() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileSave()");

	if (doSave(getMDIWindow()))
		recentFiles->updateRecentFilesMenu();
}



/**
 * Menu file -> save as.
 */
void QC_ApplicationWindow::slotFileSaveAs() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileSaveAs()");
	if (doSave(getMDIWindow(), true))
		recentFiles->updateRecentFilesMenu();
}

bool QC_ApplicationWindow::slotFileSaveAll()
{
	QC_MDIWindow* current = getMDIWindow();
    bool result {true};
	for (auto w : window_list) {
		if (w && w->getDocument()->isModified()) {
			result = doSave(w);
			if (!result) {
				statusBar()->showMessage(tr("Save All cancelled"), 2000);
				break;
			}
		}
	}
	doActivate(current);
	recentFiles->updateRecentFilesMenu();
	return result;
}



/**
 * Autosave.
 */
void QC_ApplicationWindow::slotFileAutoSave() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileAutoSave(): begin");

    auto groupGuard = RS_SETTINGS->beginGroupGuard("/Defaults");
    if (RS_SETTINGS->readNumEntry("/AutoBackupDocument", 1) == 0) {
        RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "QC_ApplicationWindow::%s: /Defaults/AutoBackupDocument is disabled\n", __func__);
        startAutoSave(false);
        return;
    }

    statusBar()->showMessage(tr("Auto-saving drawing..."), 2000);

    QC_MDIWindow* w = getMDIWindow();
    if (w) {
        bool cancelled;
        if (w->slotFileSave(cancelled, true)) {
            // auto-save cannot be cancelled by user, so the
            // "cancelled" parameter is a dummy
            statusBar()->showMessage(tr("Auto-saved drawing"), 2000);
        } else {
            // error
            m_autosaveTimer->stop();
            QMessageBox::information(this, QMessageBox::tr("Warning"),
                                     tr("Cannot auto-save the file\n%1\nPlease "
                                        "check the permissions.\n"
                                        "Auto-save disabled.")
                                     .arg(w->getDocument()->getAutoSaveFilename()),
                                     QMessageBox::Ok);
            statusBar()->showMessage(tr("Auto-saving failed"), 2000);
        }
    }
}



/**
 * Menu file -> export.
 */
void QC_ApplicationWindow::slotFileExport() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileExport()");

    statusBar()->showMessage(tr("Exporting drawing..."), 2000);

    QC_MDIWindow* w = getMDIWindow();
    QString fn;
    if (w) {

        // read default settings:
        RS_SETTINGS->beginGroup("/Export");
        QString defDir = RS_SETTINGS->readEntry("/ExportImage", RS_SYSTEM->getHomeDir());
		QString defFilter = RS_SETTINGS->readEntry("/ExportImageFilter",
													   QString("%1 (%2)(*.%2)").arg(QG_DialogFactory::extToFormat("png")).arg("png"));
        RS_SETTINGS->endGroup();

        bool cancel = false;

        QStringList filters;
        QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();
        supportedImageFormats.push_back("svg"); // add svg

        for (QString format: supportedImageFormats) {
            format = format.toLower();
            QString st;
            if (format=="jpeg" || format=="tiff") {
                // Don't add the aliases
			} else {
				st = QString("%1 (%2)(*.%2)")
						.arg(QG_DialogFactory::extToFormat(format))
						.arg(format);
            }
            if (st.length()>0)
                filters.push_back(st);
        }
        // revise list of filters
        filters.removeDuplicates();
        filters.sort();

        // set dialog options: filters, mode, accept, directory, filename
        QFileDialog fileDlg(this, tr("Export as"));

        fileDlg.setNameFilters(filters);
        fileDlg.setFileMode(QFileDialog::AnyFile);
        fileDlg.selectNameFilter(defFilter);
        fileDlg.setAcceptMode(QFileDialog::AcceptSave);
        fileDlg.setDirectory(defDir);
        fn = QFileInfo(w->getDocument()->getFilename()).baseName();
        if(fn==nullptr)
            fn = "unnamed";
        fileDlg.selectFile(fn);

        if (fileDlg.exec()==QDialog::Accepted) {
            QStringList files = fileDlg.selectedFiles();
            if (!files.isEmpty())
                fn = files[0];
            cancel = false;
        } else {
            cancel = true;
        }

        // store new default settings:
        if (!cancel) {
            RS_SETTINGS->beginGroup("/Export");
            RS_SETTINGS->writeEntry("/ExportImage", QFileInfo(fn).absolutePath());
            RS_SETTINGS->writeEntry("/ExportImageFilter",
                                    fileDlg.selectedNameFilter());
            RS_SETTINGS->endGroup();

            // find out extension:

            QString filter = fileDlg.selectedNameFilter();
            QString format = "";
            int i = filter.indexOf("(*.");
            if (i!=-1) {
                int i2 = filter.indexOf(QRegExp("[) ]"), i);
                format = filter.mid(i+3, i2-(i+3));
                format = format.toUpper();
            }

            // append extension to file:
            if (!QFileInfo(fn).fileName().contains(".")) {
                fn.push_back("." + format.toLower());
            }

            // show options dialog:
            QG_ImageOptionsDialog dlg(this);
            w->getGraphic()->calculateBorders();
            dlg.setGraphicSize(w->getGraphic()->getSize()*2.);
            if (dlg.exec()) {

                //QSize size = dlg.getSize();
                //QSize borders = dlg.getBorders();
                //bool black = dlg.isBackgroundBlack();
                //bool bw = dlg.isBlackWhite();

                bool ret = slotFileExport(fn, format, dlg.getSize(), dlg.getBorders(),
                            dlg.isBackgroundBlack(), dlg.isBlackWhite());
                if (ret) {
                    QString message = tr("Exported: %1").arg(fn);
                    statusBar()->showMessage(message, 2000);
                    commandWidget->appendHistory(message);
                }
            }
        }
    }

}



/**
 * Exports the drawing as a bitmap or another picture format.
 *
 * @param name File name.
 * @param format File format (e.g. "png")
 * @param size Size of the bitmap in pixel
 * @param black true: Black background, false: white
 * @param bw true: black/white export, false: color
 */
bool QC_ApplicationWindow::slotFileExport(const QString& name,
        const QString& format, QSize size, QSize borders, bool black, bool bw) {

    QC_MDIWindow* w = getMDIWindow();
    if (w==nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFileExport: "
                "no window opened");
        return false;
    }

    RS_Graphic* graphic = w->getDocument()->getGraphic();
    if (graphic==nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFileExport: "
                "no graphic");
        return false;
    }

    statusBar()->showMessage(tr("Exporting..."));
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    bool ret = false;
    // set vars for normal pictures and vectors (svg)
    QPixmap* picture = new QPixmap(size);

    QSvgGenerator* vector = new QSvgGenerator();

    // set buffer var
    QPaintDevice* buffer;

    if(format.toLower() != "svg") {
        buffer = picture;
    } else {
        vector->setSize(size);
        vector->setViewBox(QRectF(QPointF(0,0),size));
        vector->setFileName(name);
        buffer = vector;
    }

    // set painter with buffer
    RS_PainterQt painter(buffer);

    if (black) {
        painter.setBackground( Qt::black);
        if (bw) {
            painter.setDrawingMode( RS2::ModeWB);
        }
    }
    else {
        painter.setBackground(Qt::white);
        if (bw) {
            painter.setDrawingMode( RS2::ModeBW);
        }
    }

    painter.eraseRect(0,0, size.width(), size.height());

	RS_StaticGraphicView gv(size.width(), size.height(), &painter, &borders);
    if (black) {
		gv.setBackground(Qt::black);
    } else {
		gv.setBackground(Qt::white);
    }
    gv.setContainer(graphic);
    gv.zoomAuto(false);
	gv.drawEntity(&painter, gv.getContainer());

    // end the picture output
    if(format.toLower() != "svg")
    {
        // RVT_PORT QImageIO iio;
        QImageWriter iio;
        QImage img = picture->toImage();
        // RVT_PORT iio.setImage(img);
        iio.setFileName(name);
        iio.setFormat(format.toLatin1());
        // RVT_PORT if (iio.write()) {
        if (iio.write(img)) {
            ret = true;
        }
//        QString error=iio.errorString();
    }
    QApplication::restoreOverrideCursor();

    // GraphicView deletes painter
    painter.end();
    // delete vars
    delete picture;
    delete vector;

    if (ret) {
        statusBar()->showMessage(tr("Export complete"), 2000);
    } else {
        statusBar()->showMessage(tr("Export failed!"), 2000);
    }

    return ret;
}


/**
 * Called when a sub window is about to close. 
 * If modified, show the Save/Close/Cancel dialog, then do the request.
 * If a save is needed but the user cancels, the window is not closed.
 */
void QC_ApplicationWindow::slotFileClosing(QC_MDIWindow* win)
{
    RS_DEBUG->print("QC_ApplicationWindow::slotFileClosing()");
	bool cancel = false;
	bool hasParent = win->getParentWindow() != nullptr;
	if (win && win->getDocument()->isModified() && !hasParent) {
		switch (showCloseDialog(win)) {
		case QG_ExitDialog::Save:
			cancel = !doSave(win);
			break;
		case QG_ExitDialog::Cancel:
			cancel = true;
			break;
		}
	}
	if (!cancel)
	{
		doClose(win);
		doArrangeWindows(RS2::CurrentMode);
	}
}

/**
 * File > Close All - loop through all open windows, and close them.
 * Prompt user to save changes for modified documents.  If the user cancels
 * the remaining unsaved documents will not be closed.
 *
 * @return true success
 * @return false the user cancelled.
 */
bool QC_ApplicationWindow::slotFileCloseAll()
{
	bool cancel(false), closeAll(false), hasParent(false);
	for (auto w : window_list) if (w) {

		hasParent = w->getParentWindow() != nullptr;

		if (w->getDocument()->isModified() && !hasParent && !closeAll) {
			doActivate(w);
			switch (showCloseDialog(w, window_list.count() > 1)) {
            case QG_ExitDialog::Discard:
				closeAll = true;
				break;
			case QG_ExitDialog::SaveAll:
				closeAll = slotFileSaveAll();
				break;
			case QG_ExitDialog::Save:
				cancel = !doSave(w);
				break;
			case QG_ExitDialog::Cancel:
				cancel = true;
				break;
			}
		}
		if (cancel) {
			statusBar()->showMessage(tr("Close All cancelled"), 2000);
			return false;
		}

		doClose(w);
		doArrangeWindows(RS2::CurrentMode);
	}
	return true;
}


/**
 * Menu file -> print.
 */
void QC_ApplicationWindow::slotFilePrint(bool printPDF) {
    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL,"QC_ApplicationWindow::slotFilePrint(%s)", printPDF ? "PDF" : "Native");

    QC_MDIWindow* w = getMDIWindow();
    if (w==nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFilePrint: "
                "no window opened");
        return;
    }

    // Avoid printing without print preview
    if (!w->getGraphicView()->isPrintPreview())
    {
        slotFilePrintPreview(true);
        return;
    }

    RS_Graphic* graphic = w->getDocument()->getGraphic();
    if (graphic==nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFilePrint: "
                "no graphic");
        return;
    }

    statusBar()->showMessage(tr("Printing..."));
    QPrinter printer(QPrinter::HighResolution);

    bool landscape = false;
    RS2::PaperFormat pf = graphic->getPaperFormat(&landscape);
    QPrinter::PageSize paperSizeName = LC_Printing::rsToQtPaperFormat(pf);
    RS_Vector paperSize = graphic->getPaperSize();
    if(paperSizeName==QPrinter::Custom){
        RS_Vector&& s=RS_Units::convert(paperSize, graphic->getUnit(),RS2::Millimeter);
        if(landscape) s=s.flipXY();
        printer.setPageSize(QPageSize{QSizeF(s.x,s.y), QPageSize::Millimeter});
        // RS_DEBUG->print(RS_Debug::D_ERROR, "set Custom paper size to (%g, %g)\n", s.x,s.y);
    }else{
        printer.setPageSize(QPageSize{static_cast<QPageSize::PageSizeId>(paperSizeName)});
    }
    // qDebug()<<"paper size=("<<printer.paperSize(QPrinter::Millimeter).width()<<", "<<printer.paperSize(QPrinter::Millimeter).height()<<")";
    printer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    QMarginsF paperMargins{graphic->getMarginLeft(),
                                            graphic->getMarginRight(),
                                            graphic->getMarginTop(),
                                            graphic->getMarginBottom()};
    printer.setPageMargins(paperMargins);

    QString strDefaultFile("");
    RS_SETTINGS->beginGroup("/Print");
    strDefaultFile = RS_SETTINGS->readEntry("/FileName", "");
    printer.setOutputFileName(strDefaultFile);
    printer.setColorMode((QPrinter::ColorMode)RS_SETTINGS->readNumEntry("/ColorMode", (int)QPrinter::Color));
    RS_SETTINGS->endGroup();

    // printer setup:
    bool    bStartPrinting = false;
    if(printPDF) {
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setColorMode(QPrinter::Color);
        QFileInfo   infDefaultFile(strDefaultFile);
        QFileDialog fileDlg(this, tr("Export as PDF"));
        QString     defFilter("PDF files (*.pdf)");
        QStringList filters;

        filters << defFilter
                << "Any files (*)";

        fileDlg.setNameFilters(filters);
        fileDlg.setFileMode(QFileDialog::AnyFile);
        fileDlg.selectNameFilter(defFilter);
        fileDlg.setAcceptMode(QFileDialog::AcceptSave);
        fileDlg.setDefaultSuffix("pdf");
        fileDlg.setDirectory(infDefaultFile.dir().path());
		// bug#509 setting default file name restricts selection
//        strPdfFileName = infDefaultFile.baseName();
//        if( strPdfFileName.isEmpty())
//            strPdfFileName = "unnamed";
		//fileDlg.selectFile(strPdfFileName);

        if( QDialog::Accepted == fileDlg.exec()) {
            QStringList files = fileDlg.selectedFiles();
            if (!files.isEmpty()) {
                if(!files[0].endsWith(R"(.pdf)",Qt::CaseInsensitive)) files[0]=files[0]+".pdf";
                printer.setOutputFileName(files[0]);
                bStartPrinting = true;
            }
        }
    } else {
        printer.setOutputFileName(""); // uncheck 'Print to file' checkbox
        printer.setOutputFormat(QPrinter::NativeFormat);

        QPrintDialog printDialog(&printer, this);
        printDialog.setOption(QAbstractPrintDialog::PrintToFile);
        printDialog.setOption(QAbstractPrintDialog::PrintShowPageSize);
        bStartPrinting = (QDialog::Accepted == printDialog.exec());

        // fullPage must be set to true to get full width and height
        // (without counting margins).
        printer.setFullPage(true);
        auto equalPaperSize = [&printer](const RS_Vector& v0, const RS_Vector& v1) {
            // from DPI to pixel/mm
            auto resolution = RS_Units::convert(1., RS2::Millimeter, RS2::Inch) * printer.resolution();
            // ignore difference within two pixels
            return v0.distanceTo(v1) * resolution <= 2.;
        };
        auto equalMargins = [&printer](const QMarginsF& drawingMargins) {
            QMarginsF printerMarginsPixels = printer.pageLayout().marginsPixels(printer.resolution());
            // from DPI to pixel/mm
            auto resolution = RS_Units::convert(1., RS2::Millimeter, RS2::Inch) * printer.resolution();
            // assuming drawingMargins in mm
            QMarginsF drawingMarginsPixels = drawingMargins * resolution;
            QMarginsF diff = printerMarginsPixels - drawingMarginsPixels;
            // ignore difference within two pixels
            return std::max({std::abs(diff.left()), std::abs(diff.right()), std::abs(diff.top()), std::abs(diff.bottom())}) <= 2.;
        };

        RS_Vector paperSizeMm = RS_Units::convert(paperSize, graphic->getUnit(), RS2::Millimeter);
        QMarginsF printerMargins = printer.pageLayout().margins();
        QRectF paperRect = printer.paperRect(QPrinter::Millimeter);
        RS_Vector printerSizeMm{paperRect.width(), paperRect.height()};
        if (bStartPrinting
            && (!equalPaperSize(printerSizeMm, paperSizeMm) || !equalMargins(paperMargins))) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Paper settings");
            msgBox.setText("Paper size and/or margins have been changed!");
            msgBox.setInformativeText("Do you want to apply changes to current drawing?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            QString detailedText = QString("Drawing settings:\n"
                "\tsize: %1 x %2 (%3)\n"
                "\tmargins: %4, %5, %6, %7\n"
                "\n"
                "Printer settings:\n"
                "\tsize: %8 x %9 (%10)\n"
                "\tmargins: %11, %12, %13, %14\n")
                .arg(paperSize.x)
                .arg(paperSize.y)
                .arg(RS_Units::paperFormatToString(pf))
                .arg(RS_Units::convert(paperMargins.left(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.top(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.right(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.bottom(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerSizeMm.x, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerSizeMm.y, RS2::Millimeter, graphic->getUnit()))
                .arg(printer.pageLayout().pageSize().name())
                .arg(RS_Units::convert(printerMargins.left(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.top(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.right(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.bottom(), RS2::Millimeter, graphic->getUnit()));
            msgBox.setDetailedText(detailedText);
            int answer = msgBox.exec();
            switch (answer) {
            case QMessageBox::Yes:
                graphic->setPaperSize(RS_Units::convert(printerSizeMm, RS2::Millimeter, graphic->getUnit()));
                graphic->setMargins(printerMargins.left(), printerMargins.top(),
                                    printerMargins.right(), printerMargins.bottom());
                break;
            case QMessageBox::No:
                break;
            case QMessageBox::Cancel:
                bStartPrinting = false;
                break;
            }
        }
    }

    if (bStartPrinting) {
        // Try to set the printer to the highest resolution
        //todo: handler printer resolution better
        if(printer.outputFormat() == QPrinter::NativeFormat ){
            //bug#3448560
            //fixme: supportedResolutions() only reports resolution of 72dpi
            //this seems to be a Qt bug up to Qt-4.7.4
            //we might be ok to keep the default resolution

//            QList<int> res=printer.supportedResolutions ();
//            if (res.size()>0)
//                printer.setResolution(res.last());
            //        for(int i=0;i<res.size();i++){
            //        std::cout<<"res.at(i)="<<res.at(i)<<std::endl;
            //        }
        }else{//pdf or postscript format
            //fixme: user should be able to set resolution output to file
            printer.setResolution(1200);
        }

        RS_DEBUG->print(RS_Debug::D_INFORMATIONAL,"QC_ApplicationWindow::slotFilePrint: resolution is %d", printer.resolution());
        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        printer.setFullPage(true);

        RS_PainterQt painter(&printer);
        painter.setDrawingMode(w->getGraphicView()->getDrawingMode());

        QMarginsF margins = printer.pageLayout().margins();

        double printerFx = (double)printer.width() / printer.widthMM();
        double printerFy = (double)printer.height() / printer.heightMM();

        painter.setClipRect(margins.left() * printerFx, margins.top() * printerFy,
                            printer.width() - (margins.left() + margins.right()) * printerFx,
                            printer.height() - (margins.top() + margins.bottom()) * printerFy);

        RS_StaticGraphicView gv(printer.width(), printer.height(), &painter);
        gv.setPrinting(true);
        gv.setBorders(0,0,0,0);
        gv.setLineWidthScaling(w->getGraphicView()->getLineWidthScaling());

        double fx = printerFx * RS_Units::getFactorToMM(graphic->getUnit());
        double fy = printerFy * RS_Units::getFactorToMM(graphic->getUnit());
//RS_DEBUG->print(RS_Debug::D_ERROR, "paper size=(%d, %d)\n",
//                printer.widthMM(),printer.heightMM());

        double f = (fx+fy)/2.0;

        double scale = graphic->getPaperScale();

        gv.setFactor(f*scale);
//RS_DEBUG->print(RS_Debug::D_ERROR, "PaperSize=(%d, %d)\n",printer.widthMM(), printer.heightMM());
        gv.setContainer(graphic);

        double baseX = graphic->getPaperInsertionBase().x;
        double baseY = graphic->getPaperInsertionBase().y;
        int numX = graphic->getPagesNumHoriz();
        int numY = graphic->getPagesNumVert();
        RS_Vector printArea = graphic->getPrintAreaSize(false);

        for (int pY = 0; pY < numY; pY++) {
            double offsetY = printArea.y * pY;
            for (int pX = 0; pX < numX; pX++) {
                double offsetX = printArea.x * pX;
                // First page is created automatically.
                // Extra pages must be created manually.
                if (pX > 0 || pY > 0) printer.newPage();
                gv.setOffset((int)((baseX - offsetX) * f),
                             (int)((baseY - offsetY) * f));
//fixme, I don't understand the meaning of 'true' here
//        gv.drawEntity(&painter, graphic, true);
                painter.setDrawSelectedOnly(true);
                gv.drawEntity(&painter, graphic);
                painter.setDrawSelectedOnly(false);
                gv.drawEntity(&painter, graphic);
            }
        }

        // GraphicView deletes painter
        painter.end();

        RS_SETTINGS->beginGroup("/Print");
        RS_SETTINGS->writeEntry("/ColorMode", (int)printer.colorMode());
        RS_SETTINGS->writeEntry("/FileName", printer.outputFileName());
        RS_SETTINGS->endGroup();
        QApplication::restoreOverrideCursor();
    }

    statusBar()->showMessage(tr("Printing complete"), 2000);
}

void QC_ApplicationWindow::slotFilePrintPDF() {
    slotFilePrint(true);
}



/*	*
 *	Function name:
 *	Description:
 *	Author(s):		..., Claude Sylvain
 *	Created:			?
 *	Last modified:	30 July 2011
 *
 *	Parameters:		bool on:
 *							...
 *
 *	Returns:			void
 *	Notes:			Menu file -> print preview.
 *	*/

void QC_ApplicationWindow::slotFilePrintPreview(bool on)
{
    RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview()");

    QC_MDIWindow* parent = getMDIWindow();

	if (!parent)
    {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFilePrintPreview: "
                "no window opened");
        return;
    }

    // close print preview:
	if (!on)
    {
        RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): off");

        if (parent->getGraphicView()->isPrintPreview())
        {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): close");
            emit(printPreviewChanged(false));
            doClose(parent);
            doArrangeWindows(RS2::CurrentMode);
            return;
        }
    }

    // open print preview:
    else {
        // look for an existing print preview:
        QC_MDIWindow* ppv = parent->getPrintPreview();
        if (ppv)
        {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): show existing");

            doActivate(ppv);
            doArrangeWindows(RS2::CurrentMode);
            emit(printPreviewChanged(true));
        }
        else
        {
            if (!parent->getGraphicView()->isPrintPreview())
            {
                QSettings settings;
                //generate a new print preview
                RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): create");

                QC_MDIWindow* w = new QC_MDIWindow(parent->getDocument(), mdiAreaCAD, {});
                mdiAreaCAD->addSubWindow(w);
                parent->addChildWindow(w);

                w->setWindowTitle(tr("Print preview for %1").arg(parent->windowTitle()));
                w->setWindowIcon(QIcon(":/main/document.png"));
                QG_GraphicView* gv = w->getGraphicView();
                gv->device = settings.value("Hardware/Device", "Mouse").toString();
                gv->setPrintPreview(true);
                gv->setBackground(RS_Color(255,255,255));
                gv->setDefaultAction(new RS_ActionPrintPreview(*w->getDocument(), *w->getGraphicView()));

                // only graphics offer block lists, blocks don't
                RS_DEBUG->print("  adding listeners");
                RS_Graphic* graphic = w->getDocument()->getGraphic();
                if (graphic) {
                    // Link the layer list to the pen tool bar
                    graphic->addLayerListListener(penToolBar);
                    // Link the layer list to the layer widget
                    graphic->addLayerListListener(layerWidget);
                    // link the layer list ot the layer tree widget
                    graphic->addLayerListListener(layerTreeWidget);

                    // Link the block list to the block widget
                    graphic->addBlockListListener(blockWidget);

                }

				// Link the graphic view to the mouse widget:
				QG_DIALOGFACTORY->setMouseWidget(mouseWidget);
				// Link the graphic view to the coordinate widget:
				QG_DIALOGFACTORY->setCoordinateWidget(coordinateWidget);
				QG_DIALOGFACTORY->setSelectionWidget(selectionWidget);
				// Link the graphic view to the option widget:
				//QG_DIALOGFACTORY->setOptionWidget(optionWidget);
				// Link the graphic view to the command widget:
				QG_DIALOGFACTORY->setCommandWidget(commandWidget);

                RS_DEBUG->print("  showing MDI window");

                doActivate(w);
                doArrangeWindows(RS2::CurrentMode);

                gv->zoomAuto(false);

                if(graphic){
                    bool bigger = graphic->isBiggerThanPaper();
                    bool fixed = graphic->getPaperScaleFixed();

                    graphic->fitToPage();

                    // Calling zoomPage() after fitToPage() always fits
                    // preview paper in preview window. The only reason not
                    // to call zoomPage() is when drawing is bigger than paper,
                    // plus it is fixed. In that case, not calling zoomPage()
                    // prevents displaying empty paper (when drawing is actually
                    // outside the paper and the preview window) and displays
                    // full drawing and smaller paper inside it.
                    if (bigger && fixed) {
                        RS_DEBUG->print("%s: don't call zoomPage()", __func__);
                    } else {
                        RS_DEBUG->print("%s: call zoomPage()", __func__);
                        gv->zoomPage();
                    }
                }

                emit printPreviewChanged(true);
            }
        }
    }
}





/**
 * Menu file -> quit.
 */
void QC_ApplicationWindow::slotFileQuit() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileQuit()");

    statusBar()->showMessage(tr("Exiting application..."));

    if (queryExit(false)) {
        qApp->quit();
    }
}


/**
 * Shows / hides the grid.
 *
 * @param toggle true: show, false: hide.
 */
void QC_ApplicationWindow::slotViewGrid(bool toggle) {
    RS_DEBUG->print("QC_ApplicationWindow::slotViewGrid()");

    QC_MDIWindow* m = getMDIWindow();
    if (m) {
        RS_Graphic* g = m->getGraphic();
        if (g) {
            g->setGridOn(toggle);
        }
    }

    updateGrids();
    redrawAll();

    RS_DEBUG->print("QC_ApplicationWindow::slotViewGrid() OK");
}

/**
 * Enables / disables the draft mode.
 *
 * @param toggle true: enable, false: disable.
 */
void QC_ApplicationWindow::slotViewDraft(bool toggle)
{
    RS_DEBUG->print("QC_ApplicationWindow::slotViewDraft()");

    RS_SETTINGS->beginGroup("/Appearance");
    RS_SETTINGS->writeEntry("/DraftMode", (int)toggle);
    RS_SETTINGS->endGroup();

    //handle "Draft Mode" in window titles
    QString draft_string = " ["+tr("Draft Mode")+"]";

    foreach (QC_MDIWindow* win, window_list)
    {
        win->getGraphicView()->setDraftMode(toggle);
        QString title = win->windowTitle();

        if (toggle && !title.contains(draft_string))
        {
            win->setWindowTitle(title + draft_string);
        }
        else if (!toggle && title.contains(draft_string))
        {
            title.remove(draft_string);
            win->setWindowTitle(title);
        }
    }
    redrawAll();
}

/**
 * Redraws all mdi windows.
 */
void QC_ApplicationWindow::redrawAll()
{
    if (mdiAreaCAD)
    {
        foreach (const QC_MDIWindow* win, window_list)
        {
            if (win)
            {
                QG_GraphicView* gv = win->getGraphicView();
                if (gv) {gv->redraw();}
            }
        }
    }
}



/**
 * Updates all grids of all graphic views.
 */
void QC_ApplicationWindow::updateGrids() {
    if (mdiAreaCAD) {
        QList<QMdiSubWindow*> windows = mdiAreaCAD->subWindowList();
        for (int i = 0; i < windows.size(); ++i) {
            QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i));
            if (m) {
                QG_GraphicView* gv = m->getGraphicView();
                if (gv) {
                    // gv->updateGrid();
                    gv->redraw(RS2::RedrawGrid);
                }
            }
        }
    }
}



/**
 * Shows / hides the status bar.
 *
 * @param toggle true: show, false: hide.
 */
void QC_ApplicationWindow::slotViewStatusBar(bool toggle) {
    RS_DEBUG->print("QC_ApplicationWindow::slotViewStatusBar()");

    statusBar()->setVisible(toggle);
}

/**
 * Shows the dialog for general application preferences.
 */
void QC_ApplicationWindow::slotOptionsGeneral() {
    RS_DIALOGFACTORY->requestOptionsGeneralDialog();

    RS_SETTINGS->beginGroup("Colors");
    QColor background(RS_SETTINGS->readEntry("/background", RS_Settings::background));
    QColor gridColor(RS_SETTINGS->readEntry("/grid", RS_Settings::grid));
    QColor metaGridColor(RS_SETTINGS->readEntry("/meta_grid", RS_Settings::meta_grid));
    QColor selectedColor(RS_SETTINGS->readEntry("/select", RS_Settings::select));
    QColor highlightedColor(RS_SETTINGS->readEntry("/highlight", RS_Settings::highlight));
    QColor startHandleColor(RS_SETTINGS->readEntry("/start_handle", RS_Settings::start_handle));
    QColor handleColor(RS_SETTINGS->readEntry("/handle", RS_Settings::handle));
    QColor endHandleColor(RS_SETTINGS->readEntry("/end_handle", RS_Settings::end_handle));
    QColor relativeZeroColor(RS_SETTINGS->readEntry("/relativeZeroColor", RS_Settings::relativeZeroColor));
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Appearance");
    int antialiasing = RS_SETTINGS->readNumEntry("/Antialiasing");
    bool hideRelativeZero = RS_SETTINGS->readNumEntry("/hideRelativeZero", 0) == 1;
    RS_SETTINGS->endGroup();

    emit signalEnableRelativeZeroSnaps(!hideRelativeZero);

    QList<QMdiSubWindow*> windows = mdiAreaCAD->subWindowList();
    for (int i = 0; i < windows.size(); ++i) {
        QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i));
        if (m) {
            QG_GraphicView* gv = m->getGraphicView();
            if (gv) {
                gv->setBackground(background);
                gv->setGridColor(gridColor);
                gv->setMetaGridColor(metaGridColor);
                gv->setSelectedColor(selectedColor);
                gv->setHighlightedColor(highlightedColor);
                gv->setStartHandleColor(startHandleColor);
                gv->setHandleColor(handleColor);
                gv->setEndHandleColor(endHandleColor);
                gv->setRelativeZeroColor(relativeZeroColor);
                gv->setRelativeZeroHiddenState(hideRelativeZero);
                gv->setAntialiasing(antialiasing);
                gv->redraw(RS2::RedrawGrid);
            }
        }
    }
}


/**
 * Menu File -> import -> importBlock
 */
void QC_ApplicationWindow::slotImportBlock() {

    if (getMDIWindow() == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotImportBlock: "
                "no window opened");
        return;
    }

    QG_FileDialog dlg(this);
    RS2::FormatType type = RS2::FormatDXFRW;
    QString dxfPath = dlg.getOpenFile(&type);
    if (dxfPath.isEmpty()) {
        return;
    }

    if (QFileInfo(dxfPath).isReadable()) {
        if (actionHandler) {
            RS_ActionInterface* a =
                actionHandler->setCurrentAction(RS2::ActionLibraryInsert);
            if (a) {
                RS_ActionLibraryInsert* action = (RS_ActionLibraryInsert*)a;
                action->setFile(dxfPath);
            } else {
                RS_DEBUG->print(RS_Debug::D_ERROR,
                                "QC_ApplicationWindow::slotImportBlock:"
                                "Cannot create action RS_ActionLibraryInsert");
            }
        }
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QC_ApplicationWindow::slotImportBlock: Can't read file: '%s'", dxfPath.toLatin1().data());
    }
}


void QC_ApplicationWindow::showAboutWindow()
{
    // author: ravas

    QDialog dlg;
    dlg.setWindowTitle(tr("About"));

    auto layout = new QVBoxLayout;
    dlg.setLayout(layout);

    auto frame = new QGroupBox(qApp->applicationName());
    layout->addWidget(frame);

    auto f_layout = new QVBoxLayout;
    frame->setLayout(f_layout);

    // Compiler macro list in Qt source tree
    // Src/qtbase/src/corelib/global/qcompilerdetection.h

    QString info
    (
        tr("Version: %1").arg(XSTR(LC_VERSION)) + "\n" +
        #if defined(Q_CC_CLANG)
            tr("Compiler: Clang %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__) + "\n" +
        #elif defined(Q_CC_GNU)
            tr("Compiler: GNU GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__) + "\n" +
        #elif defined(Q_CC_MSVC)
            tr("Compiler: Microsoft Visual C++") + "\n" +
        #endif
        tr("Compiled on: %1").arg(__DATE__) + "\n" +
        tr("Qt Version: %1").arg(qVersion()) + "\n" +
        tr("Boost Version: %1.%2.%3").arg(BOOST_VERSION / 100000).arg(BOOST_VERSION / 100 % 1000).arg(BOOST_VERSION % 100)
    );

    auto app_info = new QLabel(info);
    app_info->setTextInteractionFlags(Qt::TextSelectableByMouse);
    f_layout->addWidget(app_info);

    auto copy_button = new QPushButton(tr("Copy"));
    // copy_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    f_layout->addWidget(copy_button);

    connect(copy_button, SIGNAL(released()), &dlg, SLOT(accept()));

    QLabel *links_label = new QLabel( QString( "<a href=\"https://github.com/LibreCAD/LibreCAD/graphs/contributors\">%1</a>"
                                               "<br/>"
                                               "<a href=\"https://github.com/LibreCAD/LibreCAD/blob/master/LICENSE\">%2</a>"
                                               "<br/>"
                                               "<a href=\"https://github.com/LibreCAD/LibreCAD/\">%3</a>")
                                      .arg( tr("Contributors"))
                                      .arg( tr("License"))
                                      .arg( tr("The Code")) );
    links_label->setOpenExternalLinks(true);
    links_label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    f_layout->addWidget(links_label);

    if (dlg.exec())
    {
        QClipboard* clipboard = QApplication::clipboard();
        #if QT_VERSION >= 0x050400
           info += "\n" + tr("System") + ": " + QSysInfo::prettyProductName();
        #endif
        clipboard->setText(info);
    }
}

/**
 * overloaded for Message box on last window exit.
 */
bool QC_ApplicationWindow::queryExit(bool force){
    RS_DEBUG->print("QC_ApplicationWindow::queryExit()");
    bool succ = true;
    if (force)
        for (auto w: window_list)
            doClose(w);
    else {

        RS_SETTINGS->beginGroup("/Startup");
        bool saveOpenedFiles = RS_SETTINGS->readNumEntry("/OpenLastOpenedFiles", 0) == 1;
        RS_SETTINGS->endGroup();

        QString openedFiles;
        QString activeFile = "";
        if (saveOpenedFiles){
            for (auto w: window_list) {
                QString fileName = w->getDocument() ->getFilename();
                if (activedMdiSubWindow != nullptr && activedMdiSubWindow == w){
                    activeFile = fileName;
                }
                openedFiles += fileName;
                openedFiles +=";";
            }
        }
        succ = slotFileCloseAll();

        if (succ){
            if (!openedFiles.isEmpty()){
                RS_SETTINGS->beginGroup("/Startup");
                RS_SETTINGS->writeEntry("/LastOpenFilesList", openedFiles);
                RS_SETTINGS->writeEntry("/LastOpenFilesActive", activeFile);
                RS_SETTINGS->endGroup();
            }
            storeSettings();
        }
    }

    RS_DEBUG->print("QC_ApplicationWindow::queryExit(): OK");

    return succ;
}

/**
 * Handle hotkeys. Don't let it to the default handler of Qt.
 * it will consume them also if a text field is active
 * which means it's impossible to enter a command.
 */
void QC_ApplicationWindow::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        slotKillAllActions();
        // fall-through
    case Qt::Key_Return:
    case Qt::Key_Enter:
        slotEnter();
        e->accept();
        break;

    case Qt::Key_Plus:
    case Qt::Key_Equal:
        actionHandler->slotZoomIn();
        e->accept();
        break;

    case Qt::Key_Minus:
        actionHandler->slotZoomOut();
        e->accept();
        break;

    default:
        e->ignore();
        RS_DEBUG->print("QC_ApplicationWindow::KeyPressEvent: IGNORED");
        break;
    }

    if (e->isAccepted()) {
        RS_DEBUG->print("QC_ApplicationWindow::KeyPressEvent: Accepted");
        return;
    }

    QMainWindow::keyPressEvent(e);
}


QMdiArea const* QC_ApplicationWindow::getMdiArea() const{
    return mdiAreaCAD;
}

QMdiArea* QC_ApplicationWindow::getMdiArea(){
    return mdiAreaCAD;
}

RS_GraphicView const* QC_ApplicationWindow::getGraphicView() const{
    QC_MDIWindow const* m = getMDIWindow();
    if (m) {
        return m->getGraphicView();
    }
    return nullptr;
}

RS_GraphicView * QC_ApplicationWindow::getGraphicView() {
    QC_MDIWindow* m = getMDIWindow();
    if (m) {
        return m->getGraphicView();
    }
    return nullptr;
}

RS_Document const* QC_ApplicationWindow::getDocument() const{
    QC_MDIWindow const* m = getMDIWindow();
    if (m) {
        return m->getDocument();
    }
    return nullptr;
}

RS_Document* QC_ApplicationWindow::getDocument(){
    QC_MDIWindow* m = getMDIWindow();
    if (m) {
        return m->getDocument();
    }
    return nullptr;
}

void QC_ApplicationWindow::createNewDocument(
        const QString& fileName, RS_Document* doc) {

    slotFileNew(doc);
    if (fileName!=QString() && getDocument()) {
        getDocument()->setFilename(fileName);
    }
}

void QC_ApplicationWindow::updateWindowTitle(QWidget *w)
{
    RS_DEBUG->print("QC_ApplicationWindow::slotViewDraft()");

    RS_SETTINGS->beginGroup("/Appearance");
    bool draftMode = RS_SETTINGS->readNumEntry("/DraftMode", 0);
    RS_SETTINGS->endGroup();

    if (draftMode)
    {
        QString draft_string = " ["+tr("Draft Mode")+"]";
        if (!w->windowTitle().contains(draft_string))
            w->setWindowTitle(w->windowTitle() + draft_string);
    }
}

void QC_ApplicationWindow::relayAction(QAction* q_action)
{
    // author: ravas

    auto view = getMDIWindow()->getGraphicView();
    if (!view)
    {   // when switching back to LibreCAD from another program
        // occasionally no drawings are activated
        qWarning("relayAction: graphicView is nullptr");
        return;
    }

    view->setCurrentQAction(q_action);

    const QString commands(q_action->data().toString());
    if (!commands.isEmpty())
    {
        const QString title(q_action->text().remove("&"));
        commandWidget->appendHistory(title + " : " + commands);
    }
}

/**
 * Called by Qt after a toolbar or dockwidget right-click.
 * See QMainWindow::createPopupMenu() for more information.
 */
QMenu* QC_ApplicationWindow::createPopupMenu()
{
    // author: ravas

    QMenu* context_menu = new QMenu("Context");
    context_menu->setAttribute(Qt::WA_DeleteOnClose);

    QMenu* tb_menu = menuBar()->findChild<QMenu*>("toolbars_menu");
    QMenu* temp_tb_menu = new QMenu(tr("Toolbars"), context_menu);
    temp_tb_menu->addActions(tb_menu->actions());
    context_menu->addMenu(temp_tb_menu);

    QMenu* dw_menu = menuBar()->findChild<QMenu*>("dockwidgets_menu");
    QMenu* temp_dw_menu = new QMenu(tr("Dockwidgets"), context_menu);
    temp_dw_menu->addActions(dw_menu->actions());
    context_menu->addMenu(temp_dw_menu);

    context_menu->addAction(a_map["ViewStatusBar"]);

    return context_menu;
}

void QC_ApplicationWindow::toggleFullscreen(bool checked)
{
    // author: ravas

    checked?showFullScreen():showMaximized();
}

void QC_ApplicationWindow::hideOptions(QC_MDIWindow* win)
{
    // author: ravas

    win->getGraphicView()->getDefaultAction()->hideOptions();
}

void QC_ApplicationWindow::slotFileOpenRecent(QAction* action)
{
	RS_DEBUG->print("QC_ApplicationWindow::slotFileOpenRecent()");

    statusBar()->showMessage(tr("Opening recent file..."));
    QString fileName = action->data().toString();
    slotFileOpen(fileName, RS2::FormatUnknown);
}

/**
 * This slot manipulates the widget options dialog,
 * and reads / writes the associated settings.
 */
void QC_ApplicationWindow::widgetOptionsDialog()
{
    // author: ravas

    LC_WidgetOptionsDialog dlg;

    QSettings settings;
    settings.beginGroup("Widgets");

    int allow_style = settings.value("AllowStyle", 0).toInt();
    dlg.style_checkbox->setChecked(allow_style);
    dlg.style_combobox->addItems(QStyleFactory::keys());
    if (allow_style)
    {
        QString a_style = settings.value("Style", "").toString();
        if (!a_style.isEmpty())
        {
            int index = dlg.style_combobox->findText(a_style);
            dlg.style_combobox->setCurrentIndex(index);
        }
    }

    QString sheet_path = settings.value("StyleSheet", "").toString();
    if (!sheet_path.isEmpty() && QFile::exists(sheet_path))
        dlg.stylesheet_field->setText(sheet_path);

    int allow_theme = settings.value("AllowTheme", 0).toInt();
    dlg.theme_checkbox->setChecked(allow_theme);

    int allow_toolbar_icon_size = settings.value("AllowToolbarIconSize", 0).toInt();
    dlg.toolbar_icon_size_checkbox->setChecked(allow_toolbar_icon_size);
    int toolbar_icon_size = settings.value("ToolbarIconSize", 24).toInt();
    dlg.toolbar_icon_size_spinbox->setValue(toolbar_icon_size);

    int allow_statusbar_height = settings.value("AllowStatusbarHeight", 0).toInt();
    dlg.statusbar_height_checkbox->setChecked(allow_statusbar_height);
    int statusbar_height = settings.value("StatusbarHeight", 32).toInt();
    dlg.statusbar_height_spinbox->setValue(statusbar_height);

    int allow_statusbar_fontsize = settings.value("AllowStatusbarFontSize", 0).toInt();
    dlg.statusbar_fontsize_checkbox->setChecked(allow_statusbar_fontsize);
    int statusbar_fontsize = settings.value("StatusbarFontSize", 12).toInt();
    dlg.statusbar_fontsize_spinbox->setValue(statusbar_fontsize);

    int leftToolbarColumnsCount = settings.value("LeftToolbarColumnsCount", 5).toInt();
    dlg.left_toobar_columns_spinbox->setValue(leftToolbarColumnsCount);

    if (dlg.exec())
    {
        int allow_style = dlg.style_checkbox->isChecked();
        settings.setValue("AllowStyle", allow_style);
        if (allow_style)
        {
            QString style = dlg.style_combobox->currentText();
            settings.setValue("Style", style);
            QApplication::setStyle(QStyleFactory::create(style));
        }

        QString sheet_path = dlg.stylesheet_field->text();
        settings.setValue("StyleSheet", sheet_path);
        if (loadStyleSheet(sheet_path))
            style_sheet_path = sheet_path;

        int allow_theme = dlg.theme_checkbox->isChecked();
        settings.setValue("AllowTheme", allow_theme);

        int allow_toolbar_icon_size = dlg.toolbar_icon_size_checkbox->isChecked();
        settings.setValue("AllowToolbarIconSize", allow_toolbar_icon_size);
        if (allow_toolbar_icon_size)
        {
            int toolbar_icon_size = dlg.toolbar_icon_size_spinbox->value();
            settings.setValue("ToolbarIconSize", toolbar_icon_size);
            setIconSize(QSize(toolbar_icon_size, toolbar_icon_size));
        }

        int allow_statusbar_fontsize = dlg.statusbar_fontsize_checkbox->isChecked();
        settings.setValue("AllowStatusbarFontSize", allow_statusbar_fontsize);
        if (allow_statusbar_fontsize)
        {
            int statusbar_fontsize = dlg.statusbar_fontsize_spinbox->value();
            settings.setValue("StatusbarFontSize", statusbar_fontsize);
            QFont font;
            font.setPointSize(statusbar_fontsize);
            statusBar()->setFont(font);
        }

        int allow_statusbar_height = dlg.statusbar_height_checkbox->isChecked();
        settings.setValue("AllowStatusbarHeight", allow_statusbar_height);
        if (allow_statusbar_height)
        {
            int statusbar_height = dlg.statusbar_height_spinbox->value();
            settings.setValue("StatusbarHeight", statusbar_height);
            statusBar()->setMinimumHeight(statusbar_height);
        }

        int columnCount = dlg.left_toobar_columns_spinbox->value();
        settings.setValue("LeftToolbarColumnsCount", columnCount);
    }
    settings.endGroup();
}

/**
 * This slot modifies the commandline's title bar
 * depending on the dock area it is moved to.
 */
void QC_ApplicationWindow::modifyCommandTitleBar(Qt::DockWidgetArea area)
{
    // author: ravas

    auto* cmdDockWidget = findChild<QDockWidget*>("command_dockwidget");

    auto* commandWidget = static_cast<QG_CommandWidget*>(cmdDockWidget->widget());
    QAction* dockingAction = commandWidget->getDockingAction();
    bool docked = area & Qt::AllDockWidgetAreas;
    cmdDockWidget->setWindowTitle(docked ? tr("Cmd") : tr("Command line"));
    dockingAction->setText(docked ? tr("Float") : tr("Dock", "Dock the command widget to the main window"));
    QDockWidget::DockWidgetFeatures features =
            QDockWidget::DockWidgetClosable
            | QDockWidget::DockWidgetMovable
            | QDockWidget::DockWidgetFloatable;

    if (docked) features |= QDockWidget::DockWidgetVerticalTitleBar;
    cmdDockWidget->setFeatures(features);
}

bool QC_ApplicationWindow::loadStyleSheet(QString path)
{
    // author: ravas

    if (!path.isEmpty() && QFile::exists(path))
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qApp->setStyleSheet(QString::fromLatin1(file.readAll()));
            return true;
        }
    }
    return false;
}

void QC_ApplicationWindow::reloadStyleSheet()
{
    // author: ravas

    loadStyleSheet(style_sheet_path);
}

bool QC_ApplicationWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (QEvent::FileOpen == event->type()) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        slotFileOpen(openEvent->file(), RS2::FormatUnknown);
        return true;
    }

    return QObject::eventFilter(obj, event);
}

void QC_ApplicationWindow::updateGridStatus(const QString & status)
{
    // author: ravas

   grid_status->setBottomLabel(status);
}

void QC_ApplicationWindow::showDeviceOptions()
{
    // author: ravas

    QSettings settings;

    QDialog dlg;
    dlg.setWindowTitle(tr("Device Options"));
    auto layout = new QVBoxLayout;
    auto device_combo = new ComboBoxOption(&dlg);
    device_combo->setTitle(tr("Device"));
    device_combo->setOptionsList(QStringList({"Mouse", "Tablet", "Trackpad", "Touchscreen"}));
    device_combo->setCurrentOption(settings.value("Hardware/Device", "Mouse").toString());
    layout->addWidget(device_combo);
    dlg.setLayout(layout);
    connect(device_combo, &ComboBoxOption::optionToSave,
            this, &QC_ApplicationWindow::updateDevice);
    dlg.exec();
}

void QC_ApplicationWindow::updateDevice(QString device)
{
    // author: ravas
    QSettings settings;
    settings.setValue("Hardware/Device", device);
    foreach (auto win, window_list)
    {
        win->getGraphicView()->device = device;
    }
}

void QC_ApplicationWindow::invokeToolbarCreator()
{
    // author: ravas

    auto tb_creator = findChild<QDialog*>("Toolbar Creator");
    if (tb_creator)
    {
        tb_creator->raise();
        tb_creator->activateWindow();
        return;
    }

    auto dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(tr("Toolbar Creator"));
    dlg->setObjectName("Toolbar Creator");

    auto toolbar_creator = new WidgetCreator(dlg, a_map, ag_manager->allGroups());
    toolbar_creator->addCustomWidgets("CustomToolbars");

    connect(toolbar_creator, SIGNAL(widgetToCreate(QString)),
            this, SLOT(createToolbar(QString)));
    connect(toolbar_creator, SIGNAL(widgetToDestroy(QString)),
            this, SLOT(destroyToolbar(QString)));

    auto layout = new QVBoxLayout;
    layout->addWidget(toolbar_creator);
    dlg->setLayout(layout);

    dlg->show();
}

void QC_ApplicationWindow::createToolbar(const QString& toolbar_name)
{
    // author: ravas

    QSettings settings;
    auto tb = QString("CustomToolbars/%1").arg(toolbar_name);
    auto a_list = settings.value(tb).toStringList();

    auto toolbar = findChild<QToolBar*>(toolbar_name);

    if (toolbar)
        toolbar->clear();
    else
    {
        toolbar = new QToolBar(toolbar_name, this);
        toolbar->setObjectName(toolbar_name);
        addToolBar(Qt::BottomToolBarArea, toolbar);
    }

    foreach (auto key, a_list)
    {
        toolbar->addAction(a_map[key]);
    }
}

void QC_ApplicationWindow::destroyToolbar(const QString& toolbar_name)
{
    // author: ravas

    auto toolbar = findChild<QToolBar*>(toolbar_name);
    if (toolbar) delete toolbar;
}


void QC_ApplicationWindow::invokeMenuCreator()
{
    // author: ravas

    auto menu_creator = findChild<QDialog*>("Menu Creator");
    if (menu_creator)
    {
        menu_creator->raise();
        menu_creator->activateWindow();
        return;
    }

    auto dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle(tr("Menu Creator"));
    auto layout = new QVBoxLayout;
    auto widget_creator = new WidgetCreator(dlg, a_map, ag_manager->allGroups(), true);
    widget_creator->addCustomWidgets("CustomMenus");

    connect(widget_creator, SIGNAL(widgetToDestroy(QString)),
            this, SLOT(destroyMenu(QString)));
    connect(widget_creator, SIGNAL(widgetToAssign(QString)),
            this, SLOT(invokeMenuAssigner(QString)));
    connect(widget_creator, SIGNAL(widgetToUpdate(QString)),
            this, SLOT(updateMenu(QString)));

    layout->addWidget(widget_creator);
    dlg->setLayout(layout);
    dlg->show();
}

void QC_ApplicationWindow::invokeMenuAssigner(const QString& menu_name)
{
    //author: ravas

    QSettings settings;
    settings.beginGroup("Activators");

    QDialog dlg;
    dlg.setWindowTitle(tr("Menu Assigner"));

    auto cb_1 = new QCheckBox("Double-Click");
    auto cb_2 = new QCheckBox("Right-Click");
    auto cb_3 = new QCheckBox("Ctrl+Right-Click");
    auto cb_4 = new QCheckBox("Shift+Right-Click");
    cb_1->setChecked(settings.value("Double-Click").toString() == menu_name);
    cb_2->setChecked(settings.value("Right-Click").toString() == menu_name);
    cb_3->setChecked(settings.value("Ctrl+Right-Click").toString() == menu_name);
    cb_4->setChecked(settings.value("Shift+Right-Click").toString() == menu_name);

    auto button_box = new QDialogButtonBox;
    button_box->setStandardButtons(QDialogButtonBox::Save|QDialogButtonBox::Cancel);

    connect(button_box, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(button_box, SIGNAL(rejected()), &dlg, SLOT(reject()));

    auto layout = new QVBoxLayout;
    dlg.setLayout(layout);

    auto frame = new QFrame;
    layout->addWidget(frame);

    auto f_layout = new QVBoxLayout;
    frame->setLayout(f_layout);

    f_layout->addWidget(cb_1);
    f_layout->addWidget(cb_2);
    f_layout->addWidget(cb_3);
    f_layout->addWidget(cb_4);
    f_layout->addWidget(button_box);

    if (dlg.exec())
    {
        if (cb_1->isChecked())
            assignMenu("Double-Click", menu_name);
        else
            unassignMenu("Double-Click", menu_name);

        if (cb_2->isChecked())
            assignMenu("Right-Click", menu_name);
        else
            unassignMenu("Right-Click", menu_name);

        if (cb_3->isChecked())
            assignMenu("Ctrl+Right-Click", menu_name);
        else
            unassignMenu("Ctrl+Right-Click", menu_name);

        if (cb_4->isChecked())
            assignMenu("Shift+Right-Click", menu_name);
        else
            unassignMenu("Shift+Right-Click", menu_name);
    }
    settings.endGroup();
}

void QC_ApplicationWindow::unassignMenu(const QString& activator, const QString& menu_name)
{
    // author: ravas

    QSettings settings;
    settings.beginGroup("Activators");

    if (settings.value(activator).toString() == menu_name)
    {
        settings.remove(activator);
    }
    settings.endGroup();

    foreach (auto win, window_list)
    {
        auto view = win->getGraphicView();
        view->destroyMenu(activator);
    }
}

void QC_ApplicationWindow::assignMenu(const QString& activator, const QString& menu_name)
{
    // author: ravas

    QSettings settings;

    settings.beginGroup("Activators");
    settings.setValue(activator, menu_name);
    settings.endGroup();

    auto menu_key = QString("CustomMenus/%1").arg(menu_name);
    auto a_list = settings.value(menu_key).toStringList();

    foreach (auto win, window_list)
    {
        auto view = win->getGraphicView();
        auto menu = new QMenu(activator, view);
        menu->setObjectName(menu_name);
        foreach (auto key, a_list)
        {
            menu->addAction(a_map[key]);
        }
        view->setMenu(activator, menu);
    }
}

void QC_ApplicationWindow::updateMenu(const QString& menu_name)
{
    // author: ravas

    QSettings settings;

    auto menu_key = QString("CustomMenus/%1").arg(menu_name);
    auto a_list = settings.value(menu_key).toStringList();

    settings.beginGroup("Activators");
    auto activators = settings.childKeys();

    foreach (auto activator, activators)
    {
        if (settings.value(activator).toString() == menu_name)
        {
            foreach (auto win, window_list)
            {
                auto view = win->getGraphicView();
                auto menu = new QMenu(activator, view);
                menu->setObjectName(menu_name);
                foreach (auto key, a_list)
                {
                    menu->addAction(a_map[key]);
                }
                view->setMenu(activator, menu);
            }
        }
    }
}

void QC_ApplicationWindow::destroyMenu(const QString& menu_name)
{
    //author: ravas

    QSettings settings;
    settings.beginGroup("Activators");
    auto activators = settings.childKeys();

    foreach (auto activator, activators)
    {
        if (settings.value(activator).toString() == menu_name)
        {
            settings.remove(activator);
            foreach (auto win, window_list)
            {
                auto view = win->getGraphicView();
                view->destroyMenu(activator);
            }
        }
    }
    settings.endGroup();
}

void QC_ApplicationWindow::changeEvent(QEvent* event)
{
    // author: ravas
    // returning to LC via Command+Tab won't always activate a subwindow #821

    #if defined(Q_OS_OSX)
        if (event->type() == QEvent::ActivationChange)
        {
            if (isActiveWindow())
            {
                if (current_subwindow)
                    mdiAreaCAD->setActiveSubWindow(current_subwindow);
            }
            else
            {
                current_subwindow = mdiAreaCAD->currentSubWindow();
            }
        }
    #else
    Q_UNUSED( event)
    #endif
}


void QC_ApplicationWindow::invokeLicenseWindow()
{
    // author: ravas

    QDialog dlg;

    dlg.setWindowTitle(QObject::tr("License"));

    auto viewer = new TextFileViewer(&dlg);
    auto layout = new QVBoxLayout;
    layout->addWidget(viewer);
    dlg.setLayout(layout);

    viewer->addFile("readme", ":/readme.md");
    viewer->addFile("GPLv2", ":/gpl-2.0.txt");

    viewer->setFile("readme");

    dlg.exec();
}


QC_MDIWindow* QC_ApplicationWindow::getWindowWithDoc(const RS_Document* doc)
{
    QC_MDIWindow* wwd = nullptr;

    if (doc) {
        foreach (QC_MDIWindow* w, window_list) {
            if (w && w->getDocument() == doc) {
                wwd = w;
                break;
            }
        }
    }
    return wwd;
}

void QC_ApplicationWindow::activateWindowWithFile(QString& fileName)
{
    if (!fileName.isEmpty()){
            foreach (QC_MDIWindow *w, window_list) {
                if (w != nullptr){}
                RS_Document *doc = w->getDocument();
                if (doc != nullptr){
                    const QString &docFileName = doc->getFilename();
                    if (fileName == docFileName){
                        doActivate(w);
                        break;
                    }
                }
            }
    }
}

void QC_ApplicationWindow::showBlockActivated(const RS_Block *block)
{
    if (blockWidget != nullptr && block != nullptr) {
        blockWidget->activateBlock(const_cast<RS_Block*>(block));
    }
}

QAction* QC_ApplicationWindow::getAction(const QString& actionName) const
{
    if (a_map.count(actionName) == 0)
        return nullptr;
    return a_map[actionName];
}
