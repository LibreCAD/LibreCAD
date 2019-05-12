/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 Simon Wells <simonrwells@gmail.com>
** Copyright (C) 2015-2016 ravas (github.com/r-a-v-a-s)
** Copyright (C) 2015-2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#include <QStatusBar>
#include <QMenuBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QSplitter>
#include <QMdiArea>
#include <QPluginLoader>
#include <QImageWriter>
#include <QtSvg>
#include <QStyleFactory>
#include <QPrintDialog>
#include <QPagedPaintDevice>
#include <QRegExp>
#include <QSysInfo>

#include "main.h"

#include "rs_actionprintpreview.h"
#include "rs_settings.h"
#include "rs_staticgraphicview.h"
#include "rs_system.h"
#include "rs_actionlibraryinsert.h"
#include "rs_painterqt.h"
#include "rs_selection.h"
#include "rs_document.h"

#include "lc_centralwidget.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"

#include "lc_actionfactory.h"
#include "qg_actionhandler.h"

#include "lc_widgetfactory.h"
#include "qg_snaptoolbar.h"
#include "qg_blockwidget.h"
#include "qg_layerwidget.h"
#include "qg_librarywidget.h"
#include "qg_commandwidget.h"
#include "qg_pentoolbar.h"

#include "qg_coordinatewidget.h"
#include "qg_selectionwidget.h"
#include "qg_activelayername.h"
#include "qg_mousewidget.h"
#include "twostackedlabels.h"

#include "qg_recentfiles.h"
#include "qg_dlgimageoptions.h"
#include "qg_filedialog.h"

#include "rs_dialogfactory.h"
#include "qc_dialogfactory.h"
#include "doc_plugin_interface.h"
#include "qc_plugininterface.h"
#include "rs_commands.h"

#include "lc_simpletests.h"
#include "rs_debug.h"

#include "lc_widgetoptionsdialog.h"
#include "comboboxoption.h"

#include "lc_printing.h"
#include "actionlist.h"
#include "widgetcreator.h"
#include "lc_actiongroupmanager.h"
#include "linklist.h"
#include "colorwizard.h"
#include "lc_penwizard.h"
#include "textfileviewer.h"
#include "lc_undosection.h"

#include <boost/version.hpp>

QC_ApplicationWindow* QC_ApplicationWindow::appWindow = nullptr;

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
    , autosaveTimer(nullptr)
    , actionHandler(new QG_ActionHandler(this))
    , current_subwindow(nullptr)
    , pen_wiz(new LC_PenWizard(QObject::tr("Pen Wizard"), this))
{
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow");

    //accept drop events to open files
    setAcceptDrops(true);

    // make the left and right dock areas dominant
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    appWindow = this;

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
    if (allow_statusbar_height)
    {
        int height = settings.value("StatusbarHeight", 28).toInt();
        status_bar->setMinimumHeight(height);
    }
    settings.endGroup();

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating LC_CentralWidget");

    auto central = new LC_CentralWidget(this);

    setCentralWidget(central);

    mdiAreaCAD = central->getMdiArea();
    mdiAreaCAD->setDocumentMode(true);

    settings.beginGroup("Startup");
    if (settings.value("TabMode", 0).toBool())
        mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
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
    if (enable_left_sidebar)
        widget_factory.createLeftSidebar(5, icon_size);
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
        foreach (auto action, list)
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
    blockWidget = widget_factory.block_widget;
    commandWidget = widget_factory.command_widget;

    file_menu = widget_factory.file_menu;
    windowsMenu = widget_factory.windows_menu;

    connect(a_map["FileClose"], SIGNAL(triggered(bool)),
            mdiAreaCAD, SLOT(closeActiveSubWindow()));

    connect(penToolBar, SIGNAL(penChanged(RS_Pen)),
            this, SLOT(slotPenChanged(RS_Pen)));

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
    if (settings.value("Defaults/AutoBackupDocument", 1).toBool())
    {
        autosaveTimer = new QTimer(this);
        autosaveTimer->setObjectName("autosave");
        connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(slotFileAutoSave()));
        int ms = 60000 * settings.value("Defaults/AutoSaveTime", 5).toInt();
        autosaveTimer->start(ms);
    }

    // Disable menu and toolbar items
    emit windowsChanged(false);

    RS_COMMANDS->updateAlias();
    //plugin load
    loadPlugins();

    statusBar()->showMessage(qApp->applicationName() + " Ready", 2000);
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
            #ifdef Q_OS_WIN32
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
                            QStringList treemenu = loc.menuEntryPoint.split('/', QString::SkipEmptyParts);
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
void QC_ApplicationWindow::slotWindowActivated(QMdiSubWindow* w) {

    RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated begin");

    if(w==nullptr) {
        emit windowsChanged(false);
        activedMdiSubWindow=w;
        return;
    }

    if(w==activedMdiSubWindow) return;
    activedMdiSubWindow=w;
    QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(w);

    if (m && m->getDocument()) {

        RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated: "
                        "document: %d", m->getDocument()->getId());

        bool showByBlock = m->getDocument()->rtti()==RS2::EntityBlock;

        layerWidget->setLayerList(m->getDocument()->getLayerList(),
                                  showByBlock);

        coordinateWidget->setGraphic(m->getGraphic());

        // Only graphics show blocks. (blocks don't)
        if (m->getDocument()->rtti()==RS2::EntityGraphic) {
            blockWidget->setBlockList(m->getDocument()->getBlockList());
        } else {
            blockWidget->setBlockList(nullptr);
        }

        // Update all inserts in this graphic (blocks might have changed):
        m->getDocument()->updateInserts();
        // whether to enable undo/redo buttons
        m->getDocument()->setGUIButtons();
        m->getGraphicView()->redraw();

        // set snapmode from snap toolbar
        //actionHandler->updateSnapMode();

        // set pen from pen toolbar
        slotPenChanged(penToolBar->getPen());

        pen_wiz->mdi_win = m;

        // update toggle button status:
        if (m->getGraphic()) {
            emit(gridChanged(m->getGraphic()->isGridOn()));
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

    windowsMenu->clear();

    if (mdiAreaCAD->viewMode() == QMdiArea::TabbedView) {
        windowsMenu->addAction( tr("Su&b-Window mode"), this, SLOT(slotToggleTab()));
    } else {
        windowsMenu->addAction( tr("Ta&b mode"), this, SLOT(slotToggleTab()));
        if (window_list.size() > 1) {
            windowsMenu->addAction( tr("&Cascade"), this, SLOT(slotCascade()));
            windowsMenu->addAction( tr("&Tile"), this, SLOT(slotTile()));
            windowsMenu->addAction( tr("Tile &Vertically"), this, SLOT(slotTileVertical()));
            windowsMenu->addAction( tr("Tile &Horizontally"), this, SLOT(slotTileHorizontal()));
        }
    }

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

        mdiAreaCAD->setActiveSubWindow(w);
        w->activateWindow();
        w->raise();
        w->showMaximized();
        w->setFocus();
        slotWindowActivated(w);
    }
}

/**
 * Cascade MDI windows
 */
void QC_ApplicationWindow::slotTile() {
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
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    switch(windows.size()){
    case 1:
        mdiAreaCAD->tileSubWindows();
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

    // primitive horizontal tiling
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    if (windows.count()<=1) {
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

    // primitive horizontal tiling
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    if (windows.count()<=1) {
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

/**
 * toggles between subwindow and tab mode for the MdiArea
 */
void QC_ApplicationWindow::slotToggleTab()
{
    if (mdiAreaCAD->viewMode() == QMdiArea::SubWindowView)
    {
        mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
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
            m->showMaximized();
            qobject_cast<QC_MDIWindow*>(m)->slotZoomAuto();
        }
    }
    else
    {
        mdiAreaCAD->setViewMode(QMdiArea::SubWindowView);
        slotCascade();
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

/**
 * Called when something changed in the snaps tool bar
 */
void QC_ApplicationWindow::slotSnapsChanged(RS_SnapMode snaps) {
    RS_DEBUG->print("QC_ApplicationWindow::slotSnapsChanged() begin");

    actionHandler->slotSetSnaps(snaps);
}



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

    QC_MDIWindow* w = new QC_MDIWindow(doc, mdiAreaCAD, 0);

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

    connect(view, SIGNAL(gridStatusChanged(const QString&)),
            this, SLOT(updateGridStatus(const QString&)));

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

    // only graphics offer block lists, blocks don't
    RS_DEBUG->print("  adding listeners");
    RS_Graphic* graphic = w->getDocument()->getGraphic();

    if(layerWidget) {
        layerWidget->setLayerList(w->getDocument()->getLayerList(), false);
    }

    if(blockWidget) {
        blockWidget->setBlockList(w->getDocument()->getBlockList());
    }
    if (graphic) {
        // Link the graphic's layer list to the pen tool bar
        graphic->addLayerListListener(penToolBar);
        // Link the layer list to the layer widget
        graphic->addLayerListListener(layerWidget);

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

    QMdiSubWindow* subWindow=mdiAreaCAD->addSubWindow(w);

    RS_DEBUG->print("  showing MDI window");
    w->show();
    w->slotZoomAuto();
    subWindow->showMaximized();
    subWindow->setFocus();
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
    layerWidget->setLayerList(w->getDocument()->getLayerList(), false);
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
    if(old ) {//save old geometry
        geo=activedMdiSubWindow->geometry();
        maximized=activedMdiSubWindow->isMaximized();
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
            w->setForceClosing(true);
            mdiAreaCAD->removeSubWindow(mdiAreaCAD->currentSubWindow());
            slotFilePrintPreview(false);
            w->closeMDI(true,false); //force closing, without asking user for confirmation
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
        /*	Calculate Window Title Bar Available Space.
         *	*/
    int	wtb_as = WTB_MAX_SIZE - ((int) strlen("LibreCAD") + WTB_EXTRA_SIZE);


        /*	- If string to display to window title bar is too long, truncate
         *	  it from the left.
         *	---------------------------------------------------------------- */
        if (qstring_in.length() > wtb_as)
        {
        return "..." + qstring_in.right(wtb_as - 3);
        }
        else
        return qstring_in;
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
        bool maximized=false;

        QC_MDIWindow* w = slotFileNew();
        // RVT_PORT qApp->processEvents(1000);
        qApp->processEvents(QEventLoop::AllEvents, 1000);

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: linking layer list");
        // link the layer widget to the new document:
        layerWidget->setLayerList(w->getDocument()->getLayerList(), false);
        // link the block widget to the new document:
        blockWidget->setBlockList(w->getDocument()->getBlockList());
        // link coordinate widget to graphic
        coordinateWidget->setGraphic(w->getGraphic());

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file");

        qApp->processEvents(QEventLoop::AllEvents, 1000);

        if(old) {//save old geometry
            geo=activedMdiSubWindow->geometry();
            maximized=activedMdiSubWindow->isMaximized();
        }

        // open the file in the new view:
        bool success=false;
        if(QFileInfo(fileName).exists())
            success=w->slotFileOpen(fileName, type);
        if (!success) {
               // error
               QApplication::restoreOverrideCursor();
               QString msg=tr("Cannot open the file\n%1\nPlease "
                              "check its existence and permissions.")
                       .arg(fileName);
               commandWidget->appendHistory(msg);
               QMessageBox::information(this, QMessageBox::tr("Warning"),
                                        msg,
                                        QMessageBox::Ok);
           //file opening failed, clean up QC_MDIWindow and QMdiSubWindow
               w->setForceClosing(true);
               mdiAreaCAD->removeSubWindow(mdiAreaCAD->currentSubWindow());
               slotFilePrintPreview(false);
               w->closeMDI(true,false); //force closing, without asking user for confirmation
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
                   qobject_cast<QC_MDIWindow*>(active)->slotZoomAuto();
               }
               return;
        }

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: 1");

        // update recent files menu:
        recentFiles->add(fileName);
        openedFiles.push_back(fileName);
        layerWidget->slotUpdateLayerList();
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

        recentFiles->updateRecentFilesMenu();

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: set caption");


                /*	Format and set caption.
                 *	----------------------- */
        w->setWindowTitle(format_filename_caption(fileName) + "[*]");
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

    statusBar()->showMessage(tr("Saving drawing..."));

    QC_MDIWindow* w = getMDIWindow();
    QString name;
    if (w) {
        if (w->getDocument()->getFilename().isEmpty()) {
            slotFileSaveAs();
        } else {
            bool cancelled;
            if (w->slotFileSave(cancelled)) {
                if (!cancelled) {
                    name = w->getDocument()->getFilename();
                    statusBar()->showMessage(tr("Saved drawing: %1").arg(name), 2000);
                }
            } else {
                QString message( tr("Cannot save the file ") +
                                 w->getDocument()->getFilename()
                                 + tr(" , please check the filename and permissions.")
                                 );
                statusBar()->showMessage(message, 2000);
                commandWidget->appendHistory(message);
                slotFileSaveAs();
                // error
                /*
                QMessageBox::information(this, QMessageBox::tr("Warning"),
                                         tr("Cannot save the file\n%1\nPlease "
                                            "check the permissions.")
                                         .arg(w->getDocument()->getFilename()),
                                         QMessageBox::Ok);
                                         */
            }
        }
    }
}



/**
 * Menu file -> save as.
 */
void QC_ApplicationWindow::slotFileSaveAs() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileSaveAs()");

    statusBar()->showMessage(tr("Saving drawing under new filename..."));

    QC_MDIWindow* w = getMDIWindow();
    QString name;
    if (w) {
        bool cancelled;
        if (w->slotFileSaveAs(cancelled)) {
            if (!cancelled) {
                name = w->getDocument()->getFilename();
                recentFiles->add(name);
                w->setWindowTitle(format_filename_caption(name) + "[*]");
                if(w->getGraphicView()->isDraftMode())
                    w->setWindowTitle(w->windowTitle() + " ["+tr("Draft Mode")+"]");

                if (autosaveTimer && !autosaveTimer->isActive())
                {
                    RS_SETTINGS->beginGroup("/Defaults");
                    autosaveTimer->start(RS_SETTINGS->readNumEntry("/AutoSaveTime", 5)*60*1000);
                    RS_SETTINGS->endGroup();
                }
            }
        } else {
            // error
            QMessageBox::information(this, QMessageBox::tr("Warning"),
                                     tr("Cannot save the file\n%1\nPlease "
                                        "check the permissions.")
                                     .arg(w->getDocument()->getFilename()),
                                     QMessageBox::Ok);
        }
    }
    recentFiles->updateRecentFilesMenu();

    QString message = tr("Saved drawing: %1").arg(name);
    statusBar()->showMessage(message, 2000);
    commandWidget->appendHistory(message);
}



/**
 * Autosave.
 */
void QC_ApplicationWindow::slotFileAutoSave() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileAutoSave()");

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
            autosaveTimer->stop();
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

    // black background:
    if (black) {
//RLZ        painter.setBackgroundColor(RS_Color(0,0,0));
		painter.setBackground(Qt::black);
    }
    // white background:
    else {
//RLZ        painter.setBackgroundColor(RS_Color(255,255,255));
		painter.setBackground(Qt::white);
    }

    // black/white:
    if (bw) {
        painter.setDrawingMode(RS2::ModeBW);
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
 * Called when a MDI window is actually about to close. Used to
 * detach widgets from the document.
 */
void QC_ApplicationWindow::slotFileClosing(QC_MDIWindow* win)
{
    RS_DEBUG->print("QC_ApplicationWindow::slotFileClosing()");

    window_list.removeOne(win);

    if (activedMdiSubWindow == win)
    {
        layerWidget->setLayerList(nullptr, false);
        blockWidget->setBlockList(nullptr);
        coordinateWidget->setGraphic(nullptr);
    }

    openedFiles.removeAll(win->getDocument()->getFilename());

    activedMdiSubWindow = nullptr;
    actionHandler->set_view(nullptr);
    actionHandler->set_document(nullptr);
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
        printer.setPaperSize(QSizeF(s.x,s.y),QPrinter::Millimeter);
        // RS_DEBUG->print(RS_Debug::D_ERROR, "set Custom paper size to (%g, %g)\n", s.x,s.y);
    }else{
        printer.setPaperSize(paperSizeName);
    }
    // qDebug()<<"paper size=("<<printer.paperSize(QPrinter::Millimeter).width()<<", "<<printer.paperSize(QPrinter::Millimeter).height()<<")";
    if (landscape) {
        printer.setOrientation(QPrinter::Landscape);
    } else {
        printer.setOrientation(QPrinter::Portrait);
    }
    QPagedPaintDevice::Margins paperMargins{graphic->getMarginLeft(),
                                            graphic->getMarginRight(),
                                            graphic->getMarginTop(),
                                            graphic->getMarginBottom()};
    printer.setMargins(paperMargins);

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
        QPagedPaintDevice::Margins printerMargins = printer.margins();
        RS_Vector printerSize(printer.widthMM(), printer.heightMM());
        if (bStartPrinting
                && (paperSize != printerSize
                    || paperMargins.left != printerMargins.left
                    || paperMargins.top != printerMargins.top
                    || paperMargins.right != printerMargins.right
                    || paperMargins.bottom != printerMargins.bottom)) {
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
                .arg(RS_Units::convert(paperMargins.left, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.top, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.right, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.bottom, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerSize.x, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerSize.y, RS2::Millimeter, graphic->getUnit()))
                .arg(printer.paperName())
                .arg(RS_Units::convert(printerMargins.left, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.top, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.right, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.bottom, RS2::Millimeter, graphic->getUnit()));
            msgBox.setDetailedText(detailedText);
            int answer = msgBox.exec();
            switch (answer) {
            case QMessageBox::Yes:
                graphic->setPaperSize(RS_Units::convert(printerSize, RS2::Millimeter, graphic->getUnit()));
                graphic->setMargins(printerMargins.left, printerMargins.top,
                                    printerMargins.right, printerMargins.bottom);
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

        QPagedPaintDevice::Margins margins = printer.margins();

        double printerFx = (double)printer.width() / printer.widthMM();
        double printerFy = (double)printer.height() / printer.heightMM();

        painter.setClipRect(margins.left * printerFx, margins.top * printerFy,
                            printer.width() - (margins.left + margins.right) * printerFx,
                            printer.height() - (margins.top + margins.bottom) * printerFy);

        RS_StaticGraphicView gv(printer.width(), printer.height(), &painter);
        gv.setPrinting(true);
        gv.setBorders(0,0,0,0);

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
                gv.drawEntity(&painter, graphic );
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
            mdiAreaCAD->closeActiveSubWindow();
            getMDIWindow()->showMaximized();
            emit(printPreviewChanged(false));
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

            ppv->showMaximized();
            mdiAreaCAD->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(ppv));
            emit(printPreviewChanged(true));
        }
        else
        {
            if (!parent->getGraphicView()->isPrintPreview())
            {
                QSettings settings;
                //generate a new print preview
                RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): create");

                QC_MDIWindow* w = new QC_MDIWindow(parent->getDocument(), mdiAreaCAD, 0);
                QMdiSubWindow* subWindow=mdiAreaCAD->addSubWindow(w);
                subWindow->showMaximized();
                parent->addChildWindow(w);
                connect(w, SIGNAL(signalClosing(QC_MDIWindow*)),
                        this, SLOT(hideOptions(QC_MDIWindow*)));

                w->setWindowTitle(tr("Print preview for %1").arg(parent->windowTitle()));
                w->setWindowIcon(QIcon(":/main/document.png"));
                w->slotZoomAuto();
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

                if (mdiAreaCAD->subWindowList().size() <= 1 ) {
                    w->showMaximized();
                } else {
                    w->show();
                }

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
                w->getGraphicView()->getDefaultAction()->showOptions();

                slotWindowActivated(subWindow);

                emit(printPreviewChanged(true));
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
    QColor background(RS_SETTINGS->readEntry("/background", Colors::background));
    QColor gridColor(RS_SETTINGS->readEntry("/grid", Colors::grid));
    QColor metaGridColor(RS_SETTINGS->readEntry("/meta_grid", Colors::meta_grid));
    QColor selectedColor(RS_SETTINGS->readEntry("/select", Colors::select));
    QColor highlightedColor(RS_SETTINGS->readEntry("/highlight", Colors::highlight));
    QColor startHandleColor(RS_SETTINGS->readEntry("/start_handle", Colors::start_handle));
    QColor handleColor(RS_SETTINGS->readEntry("/handle", Colors::handle));
	QColor endHandleColor(RS_SETTINGS->readEntry("/end_handle", Colors::end_handle));
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("/Appearance");
    int antialiasing = RS_SETTINGS->readNumEntry("/Antialiasing");
    RS_SETTINGS->endGroup();

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
                gv->setAntialiasing(antialiasing?true:false);
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

    QString links
    (
        QString("<a href=\"https://github.com/LibreCAD/LibreCAD/graphs/contributors\">%1</a>").arg(tr("Contributors"))
        + "<br/>" +
        QString("<a href=\"https://github.com/LibreCAD/LibreCAD/blob/master/LICENSE\">%1</a>").arg(tr("License"))
        + "<br/>" +
        QString("<a href=\"https://github.com/LibreCAD/LibreCAD/tree/%1\">%2</a>").arg(XSTR(LC_VERSION)).arg("The Code")
    );

    auto links_label = new QLabel(links);
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
bool QC_ApplicationWindow::queryExit(bool force) {
    RS_DEBUG->print("QC_ApplicationWindow::queryExit()");

    bool succ = true;

    QList<QMdiSubWindow*> list = mdiAreaCAD->subWindowList();

    while (!list.isEmpty())
    {
        QC_MDIWindow* tmp = qobject_cast<QC_MDIWindow*>(list.takeFirst());
        tmp->getGraphicView()->killAllActions();
        if (tmp)
        {
            slotFilePrintPreview(false);
            succ = tmp->closeMDI(force);
            if (!succ)
                break;
            else
                tmp->close();
        }
    }

    if (succ) {
        storeSettings();
    } else {
        QMdiSubWindow* subWindow=mdiAreaCAD->currentSubWindow();
        appWindow->slotWindowActivated(subWindow);
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
    if (fileName!=QString::null && getDocument()) {
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

void QC_ApplicationWindow::invokeLinkList()
{
    // author: ravas

    QDialog dlg;
    dlg.setWindowTitle(tr("Help Links"));
    auto layout = new QVBoxLayout;
    auto list = new LinkList(&dlg);
    list->addLink(QObject::tr("Wiki"), "http://wiki.librecad.org/");
    list->addLink(QObject::tr("User's Manual"), "http://wiki.librecad.org/index.php/LibreCAD_users_Manual");
    list->addLink(QObject::tr("Commands"), "http://wiki.librecad.org/index.php/Commands");
    list->addLink(QObject::tr("Style Sheets"), "https://github.com/LibreCAD/LibreCAD/wiki/Style-Sheets");
    list->addLink(QObject::tr("Widgets"), "https://github.com/LibreCAD/LibreCAD/wiki/Widgets");
    list->addLink(QObject::tr("Forum"), "http://forum.librecad.org/");
    list->addLink(QObject::tr("Release Information"), "https://github.com/LibreCAD/LibreCAD/releases");
    layout->addWidget(list);
    dlg.setLayout(layout);
    dlg.exec();
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

    QDockWidget* cmd_dockwidget = findChild<QDockWidget*>("command_dockwidget");

    if (area == Qt::BottomDockWidgetArea || area == Qt::TopDockWidgetArea)
    {
        cmd_dockwidget->setWindowTitle("Cmd");
        cmd_dockwidget->setFeatures(QDockWidget::DockWidgetClosable
                                   |QDockWidget::DockWidgetMovable
                                   |QDockWidget::DockWidgetFloatable
                                   |QDockWidget::DockWidgetVerticalTitleBar);
    }
    else
    {
        cmd_dockwidget->setWindowTitle(tr("Command line"));
        cmd_dockwidget->setFeatures(QDockWidget::DockWidgetClosable
                                   |QDockWidget::DockWidgetMovable
                                   |QDockWidget::DockWidgetFloatable);
    }
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
