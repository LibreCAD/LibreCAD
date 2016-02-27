/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015-2016 ravas (ravas@outlook.com)
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
#include <QDesktopServices>
#include <QUrl>
#include <QtHelp>
#include <QImageWriter>
#include <QtSvg>
#include <QStyleFactory>
#include <QPrintDialog>

#include "main.h"
#include "helpbrowser.h"

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
#include "lc_options.h"

#include "lc_printing.h"

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
    : options(std::make_shared<LC_Options>())
{
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow");

    appWindow = this;

    QSettings settings;

    options->device = settings.value("Hardware/Device", "Mouse").toString();

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: setting icon");
    setWindowIcon(QIcon(QC_APP_ICON));

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

    if (allow_statusbar_fontsize)
    {
        int fontsize = settings.value("StatusbarFontSize", 12).toInt();
        QFont font;
        font.setPointSize(fontsize);
        status_bar->setFont(font);
    }
    settings.endGroup();

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating LC_CentralWidget");

    LC_CentralWidget* central = new LC_CentralWidget(this);

    setCentralWidget(central);

    mdiAreaCAD = central->getMdiArea();

    RS_SETTINGS->beginGroup("/Defaults");
    if (RS_SETTINGS->readNumEntry("/TabMode", 0))
        mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
    bool enable_left_sidebar = RS_SETTINGS->readNumEntry("/EnableLeftSidebar", 1);
    bool enable_cad_toolbars = RS_SETTINGS->readNumEntry("/EnableCADToolbars", 1);
    bool keycode_mode = RS_SETTINGS->readNumEntry("/KeycodeMode", 0);
    RS_SETTINGS->endGroup();

    connect(mdiAreaCAD, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(slotWindowActivated(QMdiSubWindow*)));

    RS_SETTINGS->beginGroup("Widgets");
    bool custom_size = RS_SETTINGS->readNumEntry("/AllowToolbarIconSize", 0);
    int icon_size = custom_size ? RS_SETTINGS->readNumEntry("/ToolbarIconSize", 24) : 24;
    RS_SETTINGS->endGroup();

    if (custom_size)
        setIconSize(QSize(icon_size, icon_size));

    actionHandler = new QG_ActionHandler(this);
    LC_ActionFactory a_factory(this, actionHandler);
    a_factory.fillActionContainer(a_map);
    LC_WidgetFactory widget_factory(this, a_map);
    if (enable_left_sidebar)
        widget_factory.createLeftSidebar(5, icon_size);
    if (enable_cad_toolbars)
        widget_factory.createCADToolbars();
    widget_factory.createRightSidebar(actionHandler);
    widget_factory.createCategoriesToolbar();
    widget_factory.createStandardToolbars(actionHandler);

    QString path = RS_SETTINGS->readEntry("/Paths/CustomToolbar");

    LC_CustomToolbar* custom_toolbar = nullptr;

    if (!path.isEmpty())
    {
        custom_toolbar = widget_factory.createCustomToolbar(path, a_factory.tool_group);

        if (custom_toolbar == nullptr)
        {
            RS_DEBUG->print("The custom toolbar file was not found.");
            RS_SETTINGS->writeEntry("/Paths/CustomToolbar", QString::null);
        }
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

    // This event filter allows sending key events to the command widget, therefore, no
    // need to activate the command widget before typing commands.
    // Since this nice feature causes a bug of lost key events when the command widget is on
    // a screen different from the main window, disabled for the time being
    // send key events for mdiAreaCAD to command widget by default
    if (!keycode_mode)
        mdiAreaCAD->installEventFilter(commandWidget);

    RS_SETTINGS->beginGroup("/Appearance");
    QString layer_select_color(RS_SETTINGS->readEntry("/LayerSelectColor", "#CCFFCC"));
    RS_SETTINGS->endGroup();

    layerWidget->setStyleSheet("selection-background-color: " + layer_select_color);
    blockWidget->setStyleSheet("selection-background-color: " + layer_select_color);

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

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init settings");
    initSettings();

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

    //accept drop events to open files
    setAcceptDrops(true);

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    statusBar()->showMessage(XSTR(QC_APPNAME) " Ready", 2000);
}

/**
  * Find a menu entry in the current menu list. This function will try to recursivly find the menu
  * searchMenu for example foo/bar
  * thisMenuList list of Widgets
  * currentEntry only used internally dueing recursion
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
                            parentMenu->addAction(actpl);
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
            slotFileOpen(fileName, RS2::FormatUnknown);
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
void QC_ApplicationWindow::initSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::initSettings()");
    recentFiles->addFiles(file_menu);

    RS_SETTINGS->beginGroup("/Geometry");
    restoreState(RS_SETTINGS->readByteArrayEntry("/DockWindows", ""));
    dock_areas.left->setChecked(RS_SETTINGS->readNumEntry("/LeftDockArea", 0));
    dock_areas.right->setChecked(RS_SETTINGS->readNumEntry("/RightDockArea", 1));
    dock_areas.top->setChecked(RS_SETTINGS->readNumEntry("/TopDockArea", 0));
    dock_areas.bottom->setChecked(RS_SETTINGS->readNumEntry("/BottomDockArea", 0));
    dock_areas.floating->setChecked(RS_SETTINGS->readNumEntry("/FloatingDockwidgets", 0));
    RS_SETTINGS->endGroup();

    RS_SETTINGS->beginGroup("Widgets");

    int allow_style = RS_SETTINGS->readNumEntry("/AllowStyle", 0);
    if (allow_style)
    {
        QString style = RS_SETTINGS->readEntry("/Style", "");
        QApplication::setStyle(QStyleFactory::create(style));
    }

    QString sheet_path = RS_SETTINGS->readEntry("/StyleSheet", "");
    if (loadStyleSheet(sheet_path))
        style_sheet_path = sheet_path;
    RS_SETTINGS->endGroup();
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
        RS_SETTINGS->writeEntry("/DockWindows", QVariant (saveState()));
        RS_SETTINGS->writeEntry("/LeftDockArea", dock_areas.left->isChecked());
        RS_SETTINGS->writeEntry("/RightDockArea", dock_areas.right->isChecked());
        RS_SETTINGS->writeEntry("/TopDockArea", dock_areas.top->isChecked());
        RS_SETTINGS->writeEntry("/BottomDockArea", dock_areas.bottom->isChecked());
        RS_SETTINGS->writeEntry("/FloatingDockwidgets", dock_areas.floating->isChecked());
        RS_SETTINGS->endGroup();
        //save snapMode
        snapToolBar->saveSnapMode();

        QSettings settings;
        settings.setValue("Hardware/Device", options->device);
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
    if(w->widget() == nullptr) {
        mdiAreaCAD->removeSubWindow(w);

        mdiAreaCAD->activateNextSubWindow();
        auto w0=mdiAreaCAD->currentSubWindow();
        w0->showNormal();
        if(w0) slotWindowActivated(w0);
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

    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    for (int i=0; i<windows.size(); ) {
        //clean up invalid sub-windows
        //fixme, this should be auto, by
        //setAttribute(Qt::WA_DeleteOnClose);

        if(windows.at(i) && windows.at(i)->widget()){
            i++;
        }else{
            mdiAreaCAD->removeSubWindow(windows.at(i));
            windows = mdiAreaCAD->subWindowList();
            if(windows.size() > 0){
                QMdiSubWindow* active= mdiAreaCAD->currentSubWindow();
                if(active) {
                   mdiAreaCAD->setActiveSubWindow(active);
                   active->raise();
                   active->setFocus();
                }

            }
            continue;
        }
    }

    if ( mdiAreaCAD->subWindowList().isEmpty()) {
        return; //no sub-window to show
    } else if (mdiAreaCAD->viewMode() == QMdiArea::TabbedView) {
        windowsMenu->addAction( tr("Su&b-Window mode"), this, SLOT(slotToggleTab()));
    } else {
        windowsMenu->addAction( tr("Ta&b mode"), this, SLOT(slotToggleTab()));
        if ( 1 < mdiAreaCAD->subWindowList().size()) {
            windowsMenu->addAction( tr("&Cascade"), this, SLOT(slotCascade()));
            windowsMenu->addAction( tr("&Tile"), this, SLOT(slotTile()));
            windowsMenu->addAction( tr("Tile &Vertically"), this, SLOT(slotTileVertical()));
            windowsMenu->addAction( tr("Tile &Horizontally"), this, SLOT(slotTileHorizontal()));
        }
    }

    windowsMenu->addSeparator();
    QMdiSubWindow* active= mdiAreaCAD->activeSubWindow();
//    int active=windows.indexOf(mdiAreaCAD->activeSubWindow());
//    std::cout<<" QC_ApplicationWindow::slotWindowsMenuAboutToShow(): has active: "<< (mdiAreaCAD->activeSubWindow() )<<" index="<<active<<std::endl;
//    if(active<0) active=windows.size()-1;
    for (int i=0; i<windows.size(); ++i) {
        QAction *id = windowsMenu->addAction(windows.at(i)->windowTitle(),
                                         this, SLOT(slotWindowsMenuActivated(bool)));
        id->setCheckable(true);
        id->setData(i);
        id->setChecked(windows.at(i)==active);
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
        // to avoid showing by tile(), bug#3418133
        // todo, is showNormal() indeed the proper way?
        //        w->showNormal();
        //        w->showMaximized();
        mdiAreaCAD->setActiveSubWindow(w);
        //                w->activateWindow();
        w->raise();
        w->showMaximized();
        w->setFocus();

        if (w->widget())
        {
            for(int i=0;i<mdiAreaCAD->subWindowList().size();i++){
                QMdiSubWindow* m=mdiAreaCAD->subWindowList().at(i);
                if( m != w){
                    m->hide();
                }
            }
        }
        // RVT_PORT need to reset/cleanup current menu here to avoid menu clutter
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
    view->options = options;
    if (scrollbars) view->addScrollbars();

    connect(view, SIGNAL(gridStatusChanged(const QString&)),
            this, SLOT(updateGridStatus(const QString&)));

    actionHandler->set_view(view);
    actionHandler->set_document(w->getDocument());

    connect(w, SIGNAL(signalClosing(QC_MDIWindow*)),
            this, SLOT(slotFileClosing(QC_MDIWindow*)));
    connect(w->getGraphicView(), SIGNAL(xbutton1_released()),
            commandWidget, SLOT(trigger()));

    if (w->getDocument()->rtti()==RS2::EntityBlock) {
        w->setWindowTitle(tr("Block '%1'").arg(((RS_Block*)(w->getDocument()))->getName()));
    } else {
        w->setWindowTitle(tr("unnamed document %1").arg(id));
    }

    //check for draft mode
    updateWindowTitle(w);
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
    // Link the dialog factory to the mouse widget:
    QG_DIALOGFACTORY->setMouseWidget(mouseWidget);
    // Link the dialog factory to the coordinate widget:
    if( coordinateWidget){
        coordinateWidget->setGraphic(graphic );
    }
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
    int	wtb_as = WTB_MAX_SIZE - ((int) strlen(XSTR(QC_APPNAME)) + WTB_EXTRA_SIZE);


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
        if (w->getGraphic()) {
            emit(gridChanged(w->getGraphic()->isGridOn()));
        }

        recentFiles->updateRecentFilesMenu();

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: set caption");


                /*	Format and set caption.
                 *	----------------------- */
        w->setWindowTitle(format_filename_caption(fileName));
        updateWindowTitle(w);

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

    }
         else
         {
        QG_DIALOGFACTORY->commandMessage(tr("File '%1' does not exist. Opening aborted").arg(fileName));
        statusBar()->showMessage(tr("Opening aborted"), 2000);
    }

    QApplication::restoreOverrideCursor();
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(..) OK");
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
                w->setWindowTitle(format_filename_caption(name));
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

    layerWidget->setLayerList(nullptr, false);
    blockWidget->setBlockList(nullptr);
    coordinateWidget->setGraphic(nullptr);

    openedFiles.removeAll(win->getDocument()->getFilename());

    activedMdiSubWindow = nullptr;
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
    QPrinter::PageSize paperSize = LC_Printing::rsToQtPaperFormat(graphic->getPaperFormat(&landscape));
    if(paperSize==QPrinter::Custom){
        RS_Vector&& s=graphic->getPaperSize();
        if(landscape) s=s.flipXY();
        printer.setPaperSize(QSizeF(s.x,s.y),QPrinter::Millimeter);
//        RS_DEBUG->print(RS_Debug::D_ERROR, "set paper size to (%g, %g)\n", s.x,s.y);
    }else
        printer.setPaperSize(paperSize);
//    qDebug()<<"paper size=("<<printer.paperSize(QPrinter::Millimeter).width()<<", "<<printer.paperSize(QPrinter::Millimeter).height()<<")";
    if (landscape) {
        printer.setOrientation(QPrinter::Landscape);
    } else {
        printer.setOrientation(QPrinter::Portrait);
    }
    QString     strDefaultFile("");
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
        printer.setOutputFormat(QPrinter::NativeFormat);

        QPrintDialog printDialog(&printer, this);
        printDialog.setOption(QAbstractPrintDialog::PrintToFile);
        printDialog.setOption(QAbstractPrintDialog::PrintShowPageSize);
        bStartPrinting = (QDialog::Accepted == printDialog.exec());
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

        RS_StaticGraphicView gv(printer.width(), printer.height(), &painter);
        gv.setPrinting(true);
        gv.setBorders(0,0,0,0);

        double fx = (double)printer.width() / printer.widthMM()
                    * RS_Units::getFactorToMM(graphic->getUnit());
        double fy = (double)printer.height() / printer.heightMM()
                    * RS_Units::getFactorToMM(graphic->getUnit());
//RS_DEBUG->print(RS_Debug::D_ERROR, "paper size=(%d, %d)\n",
//                printer.widthMM(),printer.heightMM());

        double f = (fx+fy)/2.0;

        double scale = graphic->getPaperScale();

        gv.setOffset((int)(graphic->getPaperInsertionBase().x * f),
                     (int)(graphic->getPaperInsertionBase().y * f));
        gv.setFactor(f*scale);
//RS_DEBUG->print(RS_Debug::D_ERROR, "PaperSize=(%d, %d)\n",printer.widthMM(), printer.heightMM());
        gv.setContainer(graphic);
//fixme, I don't understand the meaning of 'true' here
//        gv.drawEntity(&painter, graphic, true);

        gv.drawEntity(&painter, graphic );

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
                gv->options = options;
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
                    graphic->fitToPage();
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
	QString layer_select_color = RS_SETTINGS->readEntry("/layer_selection", Colors::layer_selection);
    RS_SETTINGS->endGroup();

    layerWidget->setStyleSheet("selection-background-color: " + layer_select_color);
    blockWidget->setStyleSheet("selection-background-color: " + layer_select_color);

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

/**
 * Menu help -> about.
 */
void QC_ApplicationWindow::slotHelpAbout() {
    RS_DEBUG->print("QC_ApplicationWindow::slotHelpAbout()");

    QStringList modules;

    /**
      * Show all plugin that has been loaded
      */
	for (QC_PluginInterface * const pluginInterface: loadedPlugins)
        modules.append(pluginInterface->name());

    QString modulesString=tr("None");
	if (!modules.empty()) {
        modulesString = modules.join(", ");
    }

    QMessageBox box(this);
    box.setWindowTitle(tr("About..."));
    /**
     * Compiler macro list in Qt source tree
     * Src/qtbase/src/corelib/global/qcompilerdetection.h
     */

    box.setText(       QString("<p><font size=\"2\">") +
                       "<h2>"+ XSTR(QC_APPNAME)+ "</h2>" +
                       tr("Version: %1").arg(XSTR(QC_VERSION)) + "<br>" +
#ifdef QC_SCMREVISION
                       tr("SCM Revision: %1").arg(XSTR(QC_SCMREVISION)) + "<br>" +
#endif
#if defined(Q_CC_CLANG)
                       tr("Compiler: Clang %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__) + "<br>" +
#elif defined(Q_CC_GNU)
                       tr("Compiler: GNU GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__) + "<br>" +
#elif defined(Q_CC_MSVC)
                       tr("Compiler: Microsoft Visual C++<br>") +
#endif
                       tr("Qt Version: %1").arg(qVersion()) + "<br>" +
                       tr("Compiled on: %1").arg(__DATE__) + "<br>" +
                       "Portions (c) 2011 by R. van Twisk" + "<br>" +
                       tr("Program Icons Supplied by") +"<br>&nbsp;&nbsp;&nbsp;Pablo: LibreCAD Argentine<br/>" +
                       tr("Splash and Logo supplied by") + "<br>&nbsp;&nbsp;&nbsp;Diego " + "<a href=\"http://daltom.2082studio.com/\">Daltom Designer</a>" + "<br/>" +
                       "<br />" +
                       tr("Modules: %1").arg(modulesString) + "<br>" +
                       "<br />" +
                       tr("Main Website : ") + "<a href=\"http://www.LibreCAD.org\">http://www.LibreCAD.org</a>"+"<br><br><br>"+
                       "<font size=\"1\">Portions (c) by RibbonSoft, Andrew Mustun</font>" +
                       "</font></p>" +
                       "<br>" +
                       "<center>" +
                       tr("Please consider donating to LibreCAD to help maintain the source code and website.") +
                       "<br>" +
                       "<br>" +
                       "<a href=\"http://librecad.org/donate.html\" alt=\"Donate to LibreCAD\">" +
                       "<img src=':/main/donate.png' />" +
                       "</a></center>"
                       );

    box.setIconPixmap( QPixmap(QC_ABOUT_ICON) );
    box.setMinimumSize(500,400);
    box.setBaseSize(500,400);
    box.setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    box.exec();
    box.resize(500,400);
}

/**
 * Menu help -> help.
 */
void QC_ApplicationWindow::slotHelpManual()
{
    RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual()");

    if (helpEngine==nullptr)
    {
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().toLatin1().data());
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().toLatin1().data());

        if ((RS_SYSTEM->getDocPath().length()>0) && (QFile::exists(RS_SYSTEM->getDocPath()+ "/LibreCADdoc.qhc")==true))
        {
            helpEngine = new QHelpEngine(RS_SYSTEM->getDocPath() + "/LibreCADdoc.qhc", this);

            helpEngine->setupData();

            QHelpContentModel *contentModel = helpEngine->contentModel();
            QHelpContentWidget *contentWidget = helpEngine->contentWidget();
            HelpBrowser *helpBrowser = new HelpBrowser(helpEngine);

            QSplitter* splitter = new QSplitter();
            splitter->addWidget(contentWidget);
            splitter->addWidget(helpBrowser);
            contentWidget->setModel(contentModel);

            helpWindow = new QDockWidget(tr("Help"), this);
            helpWindow->setWidget(splitter);
            helpWindow->setObjectName("HelpWindow");

            // Enable single clicking of the index
            connect(helpEngine->contentWidget(), SIGNAL(clicked(QModelIndex)), helpEngine->contentWidget(), SLOT(showLink(QModelIndex)));
            connect(helpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl &)), helpBrowser, SLOT(setSource(const QUrl &)));
            addDockWidget(Qt::TopDockWidgetArea, helpWindow);
        } else {
            QMessageBox::information(this, tr("Help files not found"), tr("The help files were not found."));
        }
    }
    if (helpWindow) {
        helpWindow->show();
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
            if (!succ) {break;}
        }
    }

    if (succ) {storeSettings();}

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

    if (e->modifiers() & Qt::ControlModifier)
    {
        if (e->key() == Qt::Key_M)
        {
            slotFocusCommandLine();
            e->accept();
            return;
        }
    }
    // multi key codes:
    static QTime ts = QTime();
    static QList<int> doubleCharacters;
    bool actionProcessed = false;
    QTime now = QTime::currentTime();

     // Handle "single" function keys and Alt- hotkeys.
     QString modCode = "";
     int fn_nr = 0;

     if(e->key() >= Qt::Key_F1 && e->key() <= Qt::Key_F35) {
         fn_nr = e->key() - Qt::Key_F1 + 1;
     }

     if(e->text().size() > 0) {
         if(e->modifiers() & Qt::AltModifier) {
             modCode += RS_Commands::AltPrefix;
             modCode += e->text();
         } else if(e->modifiers() & Qt::MetaModifier) {
             modCode += RS_Commands::MetaPrefix;
             modCode += e->text();
         }
     } else if(fn_nr > 0) {
         modCode += RS_Commands::FnPrefix;
         modCode += QString::number(fn_nr);
     }

     if(modCode.size() > 0) {
        // We found a single function key. Handle it.
        //std::cout << modCode.toStdString() << std::endl;
      actionHandler->keycode(modCode);
        ts = now;

        return;
     }

     // Handle double character keycodes.
    doubleCharacters << e->key();

    if (doubleCharacters.size() > 2)
        doubleCharacters = doubleCharacters.mid(doubleCharacters.size() - 2, 2);

    if (ts.msecsTo(now) < 2000 && doubleCharacters.size() == 2) {
        QString code = "";
        QList<int>::iterator i;

        for (i = doubleCharacters.begin(); i != doubleCharacters.end(); ++i)
             code += QChar(*i);

        // Check against double keycode handler
        if (actionHandler->keycode(code) == true) {
            actionProcessed = true;
        }

        // Matches doublescape, since this is not a action, it's not done in actionHandler (is that logical??)
        if (doubleCharacters == (QList<int>() << Qt::Key_Escape << Qt::Key_Escape) ) {
            slotKillAllActions();
            actionProcessed = true;
            RS_DEBUG->print("QC_ApplicationWindow::Got double escape!");
        }

        if (actionProcessed) {
            doubleCharacters.clear();
        }
    }
    ts = now;

    if (actionProcessed == false) {
        // single key codes:
        switch (e->key()) {
        //need to pass Escape to actions, issue#285
        case Qt::Key_Escape:
            slotBack();

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
    // SIGNAL = http://doc.qt.io/qt-5/qactiongroup.html#triggered

    getGraphicView()->set_action(q_action);

    const QString commands(q_action->data().toString());
    if (!commands.isEmpty())
    {
        const QString title(q_action->text().remove("&"));
        commandWidget->appendHistory(title + " : " + commands);
    }
}

void QC_ApplicationWindow::gotoWiki()
{
    QDesktopServices::openUrl(QUrl("http://wiki.librecad.org/"));
}

/**
 * Called by Qt after a toolbar or dockwidget right-click.
 * See QMainWindow::createPopupMenu() for more information.
 */
QMenu* QC_ApplicationWindow::createPopupMenu()
{
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
    // SIGNAL = http://doc.qt.io/qt-5/qaction.html#checked-prop

    checked?showFullScreen():showMaximized();
}

void QC_ApplicationWindow::hideOptions(QC_MDIWindow* win)
{
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
    // writers: ravas, ...

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
    loadStyleSheet(style_sheet_path);
}

void QC_ApplicationWindow::updateGridStatus(const QString & status)
{
    grid_status->setBottomLabel(status);
}

void QC_ApplicationWindow::showDeviceOptions()
{
    QDialog dlg;
    dlg.setWindowTitle(tr("Device Options"));
    auto layout = new QVBoxLayout;
    auto device_combo = new ComboBoxOption(&dlg);
    device_combo->setTitle(tr("Device"));
    device_combo->setOptionsList(QStringList({"Mouse", "Tablet", "Trackpad", "Touchscreen"}));
    device_combo->setCurrentOption(options->device);
    layout->addWidget(device_combo);
    dlg.setLayout(layout);
    connect(device_combo, SIGNAL(optionToSave(QString)), this, SLOT(updateDevice(QString)));
    dlg.exec();
}

void QC_ApplicationWindow::updateDevice(QString device)
{
    options->device = device;
}
