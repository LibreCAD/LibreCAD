/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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
#include "qc_mdiwindow.h"

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
#include "helpbrowser.h"
#include <QImageWriter>
#include <QtSvg>

#if QT_VERSION >= 0x050000
# include <QtPrintSupport/QPrinter>
# include <QtPrintSupport/QPrintDialog>
#else
# include <QPrinter>
# include <QPrintDialog>
#endif

#include "rs_actionprintpreview.h"
#include "rs_settings.h"
#include "rs_staticgraphicview.h"
#include "rs_system.h"
#include "rs_actionlibraryinsert.h"
#include "rs_painterqt.h"
#include "rs_selection.h"
#include "rs_document.h"

#include "qg_snaptoolbar.h"
#include "qg_blockwidget.h"
#include "qg_layerwidget.h"
#include "qg_librarywidget.h"
#include "qg_commandwidget.h"

#include "qg_coordinatewidget.h"
#include "qg_dlgimageoptions.h"
#include "qg_filedialog.h"
#include "qg_selectionwidget.h"
#include "qg_activelayername.h"
#include "qg_mousewidget.h"
#include "qg_pentoolbar.h"
#include "qg_recentfiles.h"
#include "qg_actionhandler.h"
#include "qg_graphicview.h"

#include "rs_dialogfactory.h"
#include "qc_dialogfactory.h"
#include "main.h"
#include "doc_plugin_interface.h"
#include "qc_plugininterface.h"
#include "rs_commands.h"

#include "lc_simpletests.h"
#include "lc_actionfactory.h"
#include "lc_dockwidget.h"
#include "lc_customtoolbar.h"


QC_ApplicationWindow* QC_ApplicationWindow::appWindow = nullptr;

#ifndef QC_APP_ICON
# define QC_APP_ICON ":/main/librecad.png"
#endif
#ifndef QC_ABOUT_ICON
# define QC_ABOUT_ICON ":/main/intro_librecad.png"
#endif

#include <QSplashScreen>
    extern QSplashScreen *splash;


/*	- Window Title Bar Extra (character) Size.
 *	- Notes: Extra characters appearing in the windows title bar
 *	  are " - [", ... "]" (5), and sometimes "Print preview of " (17).
 *	*/
#define WTB_EXTRA_SIZE        (5 + 17)

/*	Window Title Bar Maximum Size.
 *	Notes: On Windows XP, this is 79.
 *	*/
#define WTB_MAX_SIZE        79

 QAction* QC_ApplicationWindow::previousZoom=nullptr;
 QAction* QC_ApplicationWindow::undoButton=nullptr;
 QAction* QC_ApplicationWindow::redoButton=nullptr;
namespace {
/**
 * Wrapper for Qt.
 */
QPrinter::PageSize rsToQtPaperFormat(RS2::PaperFormat f) {
	switch (f) {
	default:
	case RS2::Custom:
		return QPrinter::Custom;
	case RS2::Letter:
		return QPrinter::Letter;
	case RS2::Legal:
		return QPrinter::Legal;
	case RS2::Executive:
		return QPrinter::Executive;
	case RS2::A0:
		return QPrinter::A0;
	case RS2::A1:
		return QPrinter::A1;
	case RS2::A2:
		return QPrinter::A2;
	case RS2::A3:
		return QPrinter::A3;
	case RS2::A4:
		return QPrinter::A4;
	case RS2::A5:
		return QPrinter::A5;
	case RS2::A6:
		return QPrinter::A6;
	case RS2::A7:
		return QPrinter::A7;
	case RS2::A8:
		return QPrinter::A8;
	case RS2::A9:
		return QPrinter::A9;
	case RS2::B0:
		return QPrinter::B0;
	case RS2::B1:
		return QPrinter::B1;
	case RS2::B2:
		return QPrinter::B2;
	case RS2::B3:
		return QPrinter::B3;
	case RS2::B4:
		return QPrinter::B4;
	case RS2::B5:
		return QPrinter::B5;
	case RS2::B6:
		return QPrinter::B6;
	case RS2::B7:
		return QPrinter::B7;
	case RS2::B8:
		return QPrinter::B8;
	case RS2::B9:
		return QPrinter::B9;
	case RS2::B10:
		return QPrinter::B10;
	case RS2::C5E:
		return QPrinter::C5E;
	case RS2::Comm10E:
		return QPrinter::Comm10E;
	case RS2::DLE:
		return QPrinter::DLE;
	case RS2::Folio:
		return QPrinter::Folio;
	case RS2::Ledger:
		return QPrinter::Ledger;
	case RS2::Tabloid:
		return QPrinter::Tabloid;
#if QT_MAJOR_VERSION >= 5
	case RS2::Arch_A:
		return QPrinter::ArchA;
	case RS2::Arch_B:
		return QPrinter::ArchB;
	case RS2::Arch_C:
		return QPrinter::ArchC;
	case RS2::Arch_D:
		return QPrinter::ArchD;
	case RS2::Arch_E:
		return QPrinter::ArchE;
#endif
	case RS2::NPageSize:
		return QPrinter::NPageSize;
	}
}
}
/**
 * Constructor. Initializes the app.
 */
QC_ApplicationWindow::QC_ApplicationWindow()
    : m_qDraftModeTitle(" ["+tr("Draft Mode")+"]")
{
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow");

    appWindow = this;

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: setting icon");
    setWindowIcon(QIcon(QC_APP_ICON));

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating action handler");
    actionHandler = new QG_ActionHandler(this);
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating action handler: OK");

    #ifdef RS_SCRIPTING
        RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating scripter");
        scripter = new QS_Scripter(this, this);
        RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating scripter: OK");
    #endif

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init MDI");
    initMDI();
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init view");
    initView();
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: menus_and_toolbars");
    menus_and_toolbars();
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init status bar");
    initStatusBar();

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

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init settings");
    initSettings();

    // Activate autosave timer
    autosaveTimer = new QTimer(this);
    autosaveTimer->setObjectName("autosave");
    connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(slotFileAutoSave()));
    RS_SETTINGS->beginGroup("/Defaults");
    autosaveTimer->start(RS_SETTINGS->readNumEntry("/AutoSaveTime", 5)*60*1000);
    RS_SETTINGS->endGroup();

    // Disable menu and toolbar items
    emit windowsChanged(false);

    RS_COMMANDS->updateAlias();
    //plugin load
    loadPlugins();
    QMenu *importMenu = findMenu("/File/Import", menuBar()->children(), "");
    if (importMenu && importMenu->isEmpty())
        importMenu->setDisabled(true);

    statusBar()->showMessage(XSTR(QC_APPNAME) " Ready", 2000);

    //accept drop events to open files
    setAcceptDrops(true);
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

const QMainWindow* QC_ApplicationWindow::getMainWindow() const{
    return this;
}

QMainWindow* QC_ApplicationWindow::getMainWindow() {
    return this;
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
    #ifdef RS_SCRIPTING

        RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                        "deleting scripter");

        delete scripter;

        RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                        "deleting scripter: OK");

    #endif

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting dialog factory");

    delete dialogFactory;

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting dialog factory: OK");
}



/**
 * Runs the start script if scripting is available.
 */
void QC_ApplicationWindow::slotRunStartScript() {
        slotRunScript("autostart.qs");
        restoreDocks();
}



/**
 * Runs a script. The action that triggers this slot has to carry the
 * name of the script file.
 */
void QC_ApplicationWindow::slotRunScript() {
    RS_DEBUG->print("QC_ApplicationWindow::slotRunScript");

    const QObject* s = sender();
    if (s) {
        QString script = ((QAction*)s)->text();
        RS_DEBUG->print("QC_ApplicationWindow::slotRunScript: %s",
                        script.toLatin1().data());
                slotRunScript(script);
    }
}



/**
 * Runs the script with the given name.
 */
void QC_ApplicationWindow::slotRunScript(const QString& name) {
    Q_UNUSED(name);
#ifdef RS_SCRIPTING
        RS_DEBUG->print("QC_ApplicationWindow::slotRunScript");


        if (scripter==nullptr) {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QC_ApplicationWindow::slotRunScript: "
                        "scripter not initialized");
                return;
        }

    statusBar()->showMessage(tr("Running script '%1'").arg(name), 2000);

        QStringList scriptList = RS_SYSTEM->getScriptList();
        scriptList.push_back(RS_SYSTEM->getHomeDir() + "/." XSTR(QC_APPKEY) "/" + name);

        for (QStringList::Iterator it = scriptList.begin(); it!=scriptList.end(); ++it) {
                RS_DEBUG->print("QC_ApplicationWindow::slotRunScript: "
                        "checking script '%s'", (*it).latin1());
                QFileInfo fi(*it);
                if (fi.exists() && fi.fileName()==name) {
                        RS_DEBUG->print("QC_ApplicationWindow::slotRunScript: running '%s'",
                                (*it).latin1());
                        scripter->runScript(*it, "main");
                }
        }
#endif
}



/**
 * Called from toolbar buttons that were added by scripts to
 * insert blocks.
 */
void QC_ApplicationWindow::slotInsertBlock() {
    const QObject* s = sender();
    if (s) {
        QString block = ((QAction*)s)->text();
        RS_DEBUG->print("QC_ApplicationWindow::slotInsertBlock: %s",
                        block.toLatin1().data());
                slotInsertBlock(block);
    }
}



/**
 * Called to insert blocks.
 */
void QC_ApplicationWindow::slotInsertBlock(const QString& name) {
        RS_DEBUG->print("QC_ApplicationWindow::slotInsertBlock: '%s'", name.toLatin1().data());

    statusBar()->showMessage(tr("Inserting block '%1'").arg(name), 2000);


        RS_GraphicView* graphicView = getGraphicView();
        RS_Document* document = getDocument();
        if (graphicView && document) {
                RS_ActionLibraryInsert* action =
                        new RS_ActionLibraryInsert(*document, *graphicView);
                action->setFile(name);
                graphicView->setCurrentAction(action);
        }
}



/**
 * Shows the main application window and a splash screen.
 */
void QC_ApplicationWindow::show() {
#ifdef QSPLASHSCREEN_H
    if (splash) {
        splash->raise();
        }
#endif

    QMainWindow::show();
#ifdef QSPLASHSCREEN_H
    if (splash) {
        splash->raise();
        qApp->processEvents();
        splash->clearMessage();
# ifdef QC_DELAYED_SPLASH_SCREEN
        QTimer::singleShot(1000*2, this, SLOT(finishSplashScreen()));
# else
        finishSplashScreen();
# endif
    }
#endif
}



/**
 * Called when the splash screen has to terminate.
 */
void QC_ApplicationWindow::finishSplashScreen() {
#ifdef QSPLASHSCREEN_H
    if (splash) {
        splash->finish(this);
        delete splash;
        splash = 0;
    }
#endif
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
 * Initializes the MDI mdiAreaCAD.
 */
void QC_ApplicationWindow::initMDI() {
    RS_DEBUG->print("QC_ApplicationWindow::initMDI() begin");

    QFrame *vb = new QFrame(this);
    vb->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    QVBoxLayout *layout = new QVBoxLayout;
    vb->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    layout->setContentsMargins ( 0, 0, 0, 0 );
    mdiAreaCAD = new QMdiArea(this);
    activedMdiSubWindow=nullptr;
    mdiAreaTab = false;
    layout->addWidget(mdiAreaCAD);
//    mdiAreaCAD->setScrollBarsEnabled(false);
    mdiAreaCAD->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiAreaCAD->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiAreaCAD->setFocusPolicy(Qt::ClickFocus);
    mdiAreaCAD->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    mdiAreaCAD->setActivationOrder(QMdiArea::ActivationHistoryOrder);
#if QT_VERSION >= 0x040800
    mdiAreaCAD->setTabsMovable(true);
    mdiAreaCAD->setTabsClosable(true);
#endif

    RS_SETTINGS->beginGroup("/Defaults");

    if (RS_SETTINGS->readNumEntry("/TabMode", 0))
    {
        mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
        mdiAreaTab = true;
    }
    RS_SETTINGS->endGroup();

    vb->setLayout(layout);
    setCentralWidget(vb);
    connect(mdiAreaCAD, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(slotWindowActivated(QMdiSubWindow*)));

    RS_DEBUG->print("QC_ApplicationWindow::initMDI() end");

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

/**
 * Initializes the status bar at the bottom.
 */
void QC_ApplicationWindow::initStatusBar() {
    RS_DEBUG->print("QC_ApplicationWindow::initStatusBar()");

    statusBar()->setMinimumHeight(32);
    coordinateWidget = new QG_CoordinateWidget(statusBar(), "coordinates");
    statusBar()->addWidget(coordinateWidget);
    mouseWidget = new QG_MouseWidget(statusBar(), "mouse info");
    statusBar()->addWidget(mouseWidget);
    selectionWidget = new QG_SelectionWidget(statusBar(), "selections");
    statusBar()->addWidget(selectionWidget);
    m_pActiveLayerName=new QG_ActiveLayerName(this);
    statusBar()->addWidget(m_pActiveLayerName);
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
    int windowWidth = RS_SETTINGS->readNumEntry("/WindowWidth", 950);
    int windowHeight = RS_SETTINGS->readNumEntry("/WindowHeight", 700);
    int windowX = RS_SETTINGS->readNumEntry("/WindowX", 0);
    int windowY = RS_SETTINGS->readNumEntry("/WindowY", 30);
    RS_SETTINGS->endGroup();

#ifdef __APPLE1__
    if (windowY<30) {
        windowY=30;
    }
#endif

    resize(windowWidth, windowHeight);
    move(windowX, windowY);

    restoreDocks();
}


/**
 * Restores the position of the dock windows.
 */
void QC_ApplicationWindow::restoreDocks() {
    RS_SETTINGS->beginGroup("/Geometry");
    restoreState ( RS_SETTINGS->readByteArrayEntry("/DockWindows", ""));
    tool_sidebar.view_action->setChecked(RS_SETTINGS->readNumEntry("/ToolSidebarState", 0));
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
        RS_SETTINGS->writeEntry("/ToolSidebarState", tool_sidebar.view_action->isChecked());
        RS_SETTINGS->endGroup();
        //save snapMode
        snapToolBar->saveSnapMode();
    }

    RS_DEBUG->print("QC_ApplicationWindow::storeSettings(): OK");
}


/**
 * Initializes the view.
 */
void QC_ApplicationWindow::initView()
{
    RS_DEBUG->print("QC_ApplicationWindow::initView()");
    RS_DEBUG->print("layer widget..");

    dock_layer = new QDockWidget("Layer", this);
    dock_layer->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_layer->setObjectName("LayerDW");
    layerWidget = new QG_LayerWidget(actionHandler, dock_layer, "Layer");
    layerWidget->setFocusPolicy(Qt::NoFocus);
    connect(layerWidget, SIGNAL(escape()), this, SLOT(slotFocus()));
    connect(this, SIGNAL(windowsChanged(bool)), layerWidget, SLOT(setEnabled(bool)));
    dock_layer->setWidget(layerWidget);
    dock_layer->setWindowTitle(tr("Layer List"));

    RS_DEBUG->print("block widget..");

    dock_block = new QDockWidget("Block", this);
    dock_block->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_block->setObjectName("BlockDW");
    blockWidget = new QG_BlockWidget(actionHandler, dock_block, "Block");
    blockWidget->setFocusPolicy(Qt::NoFocus);
    connect(blockWidget, SIGNAL(escape()), this, SLOT(slotFocus()));
    connect(this, SIGNAL(windowsChanged(bool)), blockWidget, SLOT(setEnabled(bool)));
    dock_block->setWidget(blockWidget);
    dock_block->setWindowTitle(tr("Block List"));

    RS_DEBUG->print("library widget..");

    dock_library = new QDockWidget("Library", this);
    dock_library->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_library->setObjectName("LibraryDW");
    libraryWidget = new QG_LibraryWidget(dock_library, "Library");
    libraryWidget->setActionHandler(actionHandler);
    libraryWidget->setFocusPolicy(Qt::NoFocus);
    connect(libraryWidget, SIGNAL(escape()), this, SLOT(slotFocus()));
    connect(this, SIGNAL(windowsChanged(bool)),
            (QObject*)libraryWidget->bInsert, SLOT(setEnabled(bool)));
    dock_library->setWidget(libraryWidget);
    dock_library->resize(240, 400);
    dock_library->setWindowTitle(tr("Library Browser"));
    addDockWidget(Qt::RightDockWidgetArea , dock_library);
    tabifyDockWidget(dock_library, dock_block);
    tabifyDockWidget(dock_block, dock_layer);

    RS_DEBUG->print("command widget..");

    dock_command = new QDockWidget(tr("Command line"), this);
    dock_command->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    dock_command->setObjectName("CommandDW");
    commandWidget = new QG_CommandWidget(dock_command, "Command");
    commandWidget->setActionHandler(actionHandler);
    connect(this, SIGNAL(windowsChanged(bool)), commandWidget, SLOT(setEnabled(bool)));
    connect(commandWidget->leCommand, SIGNAL(escape()), this, SLOT(setFocus()));
    dock_command->setWidget(commandWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock_command);

    //this event filter allows sending key events to the command widget, therefore, no
    // need to activate the command widget before typing commands.
    // Since this nice feature causes a bug of lost key events when the command widget is on
    // a screen different from the main window, disabled for the time being
    //send key events for mdiAreaCAD to command widget by default
    mdiAreaCAD->installEventFilter(commandWidget);

    RS_SETTINGS->beginGroup("/Appearance");
    QString layer_select_color(RS_SETTINGS->readEntry("/LayerSelectColor", "#CCFFCC"));
    RS_SETTINGS->endGroup();

    layerWidget->setStyleSheet("selection-background-color: " + layer_select_color);
    blockWidget->setStyleSheet("selection-background-color: " + layer_select_color);
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
    } else if( mdiAreaTab) {
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

void QC_ApplicationWindow::slotToggleTab() {
    mdiAreaTab = ! mdiAreaTab;
    if(mdiAreaTab)
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
    actionHandler->set_view(w->getGraphicView());
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
        if(old) {//save old geometry
            geo=activedMdiSubWindow->geometry();
            maximized=activedMdiSubWindow->isMaximized();
        }

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
                    w->setWindowTitle(w->windowTitle()+m_qDraftModeTitle);

                if (!autosaveTimer->isActive()) {
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
        painter.setBackground(RS_Color(0,0,0));
    }
    // white background:
    else {
        painter.setBackground(RS_Color(255,255,255));
    }

    // black/white:
    if (bw) {
        painter.setDrawingMode(RS2::ModeBW);
    }

    painter.eraseRect(0,0, size.width(), size.height());

	RS_StaticGraphicView gv(size.width(), size.height(), &painter, &borders);
    if (black) {
        gv.setBackground(RS_Color(0,0,0));
    } else {
        gv.setBackground(RS_Color(255,255,255));
    }
    gv.setContainer(graphic);
    gv.zoomAuto(false);
    for (RS_Entity* e=graphic->firstEntity(RS2::ResolveAll);
            e; e=graphic->nextEntity(RS2::ResolveAll)) {
        gv.drawEntity(&painter, e);
    }

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
        QString error=iio.errorString();
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
	QPrinter::PageSize paperSize=rsToQtPaperFormat(graphic->getPaperFormat(&landscape));
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

            //no need to search, casting parentWindow works like a charm
            ppv->parentWidget()->showMaximized();
            mdiAreaCAD->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(ppv->parentWidget()));
//            std::cout<<"QC_ApplicationWindow::slotFilePrintPreview(bool on): emit(printPreviewChanged(true))"<<std::endl;
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
                        this, SLOT(hide_options(QC_MDIWindow*)));

                w->setWindowTitle(tr("Print preview for %1").arg(parent->windowTitle()));
                w->setWindowIcon(QIcon(":/main/document.png"));
                w->slotZoomAuto();
                w->getGraphicView()->setPrintPreview(true);
                w->getGraphicView()->setBackground(RS_Color(255,255,255));
                w->getGraphicView()->setDefaultAction(new RS_ActionPrintPreview(*w->getDocument(), *w->getGraphicView()));

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
void QC_ApplicationWindow::slotViewDraft(bool toggle) {
    RS_DEBUG->print("QC_ApplicationWindow::slotViewDraft()");

    RS_SETTINGS->beginGroup("/Appearance");
    RS_SETTINGS->writeEntry("/DraftMode", (int)toggle);
    RS_SETTINGS->endGroup();

    for (auto const& win : window_list)
    {
        win->getGraphicView()->setDraftMode(toggle);
    }

    QList<QWidget *> windows;
    if(mdiAreaCAD)
        for(QMdiSubWindow* w: mdiAreaCAD->subWindowList())
            windows<<w;
    windows.append(this);

    //handle "Draft Mode" in window titles
    if(toggle){
        for(QWidget* w: windows){
            QString title=w->windowTitle();
//            qDebug()<<"position="<<w->windowTitle().lastIndexOf(m_qDraftModeTitle)<<" "<<m_qDraftModeTitle.size()<<" "<<w->windowTitle().size();
            //avoid duplicated "Draft Mode" string in window title
            if(title.size()>m_qDraftModeTitle.size() && title.size()-1 != title.lastIndexOf(m_qDraftModeTitle)+m_qDraftModeTitle.size())
                w->setWindowTitle(title+m_qDraftModeTitle);
        }
    } else {
        for(QWidget* w: windows){
            QString title=w->windowTitle();
            if(title.size()>m_qDraftModeTitle.size() && title.count(m_qDraftModeTitle)==1){
                title.remove(title.lastIndexOf(m_qDraftModeTitle),m_qDraftModeTitle.size());
                w->setWindowTitle(title);
            }
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

	if (!toggle) {
        statusBar()->hide();
    } else {
        statusBar()->show();
    }
}

/**
 * Shows the dialog for general application preferences.
 */
void QC_ApplicationWindow::slotOptionsGeneral() {
    RS_DIALOGFACTORY->requestOptionsGeneralDialog();

    set_icon_size();

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
                gv->setAntiAliasing(antialiasing?true:false);
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
 * Menu script -> show ide
 */
void QC_ApplicationWindow::slotScriptOpenIDE() {
#ifdef RS_SCRIPTING
    scripter->openIDE();
#endif
}



/**
 * Menu script -> run
 */
void QC_ApplicationWindow::slotScriptRun() {
#ifdef RS_SCRIPTING
    scripter->runScript();
#endif
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
void QC_ApplicationWindow::keyPressEvent(QKeyEvent* e) {

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
    //check for draft mode
    RS_DEBUG->print("QC_ApplicationWindow::slotViewDraft()");

    RS_SETTINGS->beginGroup("/Appearance");
    bool draftMode=RS_SETTINGS->readNumEntry("/DraftMode", 0);
    RS_SETTINGS->endGroup();
    if(draftMode){
//        qDebug()<<"position="<<w->windowTitle().lastIndexOf(m_qDraftModeTitle)<<" "<<m_qDraftModeTitle.size()<<" "<<w->windowTitle().size();
        if(w->windowTitle().lastIndexOf(m_qDraftModeTitle))
        w->setWindowTitle(w->windowTitle()+m_qDraftModeTitle);
    }
}

void QC_ApplicationWindow::slot_set_action(QAction* q_action)
{
    // SIGNAL = http://doc.qt.io/qt-5/qactiongroup.html#triggered

    getGraphicView()->set_action(q_action);
}

void QC_ApplicationWindow::goto_wiki()
{
    QDesktopServices::openUrl(QUrl("http://wiki.librecad.org/"));
}


// github.com/r-a-v-a-s/LibreCAD.git
// ravas@outlook.com - 2015
void QC_ApplicationWindow::menus_and_toolbars()
{
    RS_DEBUG->print("QC_ApplicationWindow::menus_and_toolbars()");

    set_icon_size();

    QSizePolicy toolBarPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QMenu* sub_menu;
    QToolButton* tool_button;

    QList<QAction*> list_a;

    RS_SETTINGS->beginGroup("/Appearance");
    bool custom_size = RS_SETTINGS->readNumEntry("/SetIconSize", 0);
    int icon_size = custom_size ? RS_SETTINGS->readNumEntry("/IconSize", 22) : 22;
    RS_SETTINGS->endGroup();

    int columns = 5; // sidebar dockwidgets

    QActionGroup* tools = new QActionGroup(this);
    connect(tools, SIGNAL(triggered(QAction*)), this, SLOT(slot_set_action(QAction*)));
    connect(this, SIGNAL(windowsChanged(bool)), tools, SLOT(setEnabled(bool)));

    QActionGroup* disable_group = new QActionGroup(this);
    disable_group->setExclusive(false);
    connect(this, SIGNAL(windowsChanged(bool)), disable_group, SLOT(setEnabled(bool)));

    LC_ActionFactory a_factory(this);
    QMap<QString, QAction*> map_a;
    map_a = a_factory.action_map(actionHandler, tools, disable_group);

    QMenuBar* menu_bar = menuBar();

    // <[~ Categories Toolbar ~]>

    QToolBar* categories_toolbar = new QToolBar(tr("Categories"), this);
    categories_toolbar->setSizePolicy(toolBarPolicy);
    categories_toolbar->setObjectName("categories_toolbar");

    // <[~ File ~]>

    file_menu = new QMenu(tr("&File"), menu_bar);
    file_menu->setObjectName("File");
    file_menu->setTearOffEnabled(true);

    file_toolbar = new QToolBar(tr("File"), this);
    file_toolbar->setSizePolicy(toolBarPolicy);
    file_toolbar->setObjectName("file_toolbar");

    list_a
            << map_a["FileNew"]
            << map_a["FileNewTemplate"]
            << map_a["FileOpen"]
            << map_a["FileSave"]
            << map_a["FileSaveAs"];

    file_menu->addActions(list_a);
    file_toolbar->addActions(list_a);

    sub_menu = file_menu->addMenu(QIcon(":/actions/fileimport.png"), tr("Import"));
    sub_menu->setObjectName("Import");
    sub_menu->addAction(map_a["DrawImage"]);
    sub_menu->addAction(map_a["BlocksImport"]);

    sub_menu = file_menu->addMenu(QIcon(":/actions/fileexport.png"), tr("Export"));
    sub_menu->setObjectName("Export");
    sub_menu->addAction(map_a["FileExportMakerCam"]);
    sub_menu->addAction(map_a["FilePrintPDF"]);
    sub_menu->addAction(map_a["FileExport"]);

    file_menu->addSeparator();

    file_menu->addAction(map_a["FilePrint"]);
    file_menu->addAction(map_a["FilePrintPreview"]);
    file_toolbar->addAction(map_a["FilePrint"]);
    file_toolbar->addAction(map_a["FilePrintPreview"]);

    file_menu->addSeparator();

    file_menu->addAction(map_a["FileClose"]);
    connect(map_a["FileClose"], SIGNAL(triggered(bool)),
            mdiAreaCAD, SLOT(closeActiveSubWindow()));

    file_menu->addAction(map_a["FileQuit"]);

    file_menu->addSeparator();

    // <[~ Settings ~]>

    QMenu* settings_menu = new QMenu(tr("Settings"), menu_bar);
    settings_menu->setObjectName("settings_menu");
    settings_menu->setTearOffEnabled(true);

    QToolBar* settings_toolbar = new QToolBar(tr("Settings"), this);
    settings_toolbar->setSizePolicy(toolBarPolicy);
    settings_toolbar->setObjectName("settings_toolbar");

    add_action(settings_menu, settings_toolbar, map_a["OptionsGeneral"]);
    add_action(settings_menu, settings_toolbar, map_a["OptionsDrawing"]);

    // <[~ Edit ~]>

    QMenu* edit_menu = new QMenu(tr("&Edit"), menu_bar);
    edit_menu->setObjectName("Edit");
    edit_menu->setTearOffEnabled(true);

    edit_toolbar = new QToolBar(tr("Edit"), this);
    edit_toolbar->setSizePolicy(toolBarPolicy);
    edit_toolbar->setObjectName("edit_toolbar");

    add_action(edit_menu, edit_toolbar, map_a["EditKillAllActions"]);

    edit_toolbar->addSeparator();
    edit_menu->addSeparator();

    undoButton = map_a["EditUndo"];
    redoButton = map_a["EditRedo"];

    add_action(edit_menu, edit_toolbar, undoButton);
    add_action(edit_menu, edit_toolbar, redoButton);

    edit_toolbar->addSeparator();
    edit_menu->addSeparator();

    add_action(edit_menu, edit_toolbar, map_a["EditCut"]);
    add_action(edit_menu, edit_toolbar, map_a["EditCopy"]);
    add_action(edit_menu, edit_toolbar, map_a["EditPaste"]);
    edit_menu->addAction(map_a["ModifyDeleteQuick"]);

    // <[~ Order ~]>

    QMenu* order_menu = new QMenu(tr("&Order"), menu_bar);
    order_menu->setObjectName("order_menu");
    order_menu->setTearOffEnabled(true);

    QToolBar* order_toolbar = new QToolBar(tr("Order"), this);
    order_toolbar->setSizePolicy(toolBarPolicy);
    order_toolbar->setObjectName("order_toolbar");

    add_action(order_menu, order_toolbar, map_a["OrderTop"]);
    add_action(order_menu, order_toolbar, map_a["OrderBottom"]);
    add_action(order_menu, order_toolbar, map_a["OrderRaise"]);
    add_action(order_menu, order_toolbar, map_a["OrderLower"]);

    // <[~ View ~]>

    QMenu* view_menu = new QMenu(tr("&View"), menu_bar);
    view_menu->setObjectName("view_menu");
    view_menu->setTearOffEnabled(true);

    view_toolbar = new QToolBar(tr("View"), this);
    view_toolbar->setSizePolicy(toolBarPolicy);
    view_toolbar->setObjectName("view_toolbar");

    view_menu->addAction(map_a["Fullscreen"]);
    statusbar_view_action = map_a["ViewStatusBar"];
    view_menu->addAction(statusbar_view_action);

    add_action(view_menu, view_toolbar, map_a["ViewGrid"]);

    RS_SETTINGS->beginGroup("/Appearance");
    bool draftMode = (bool)RS_SETTINGS->readNumEntry("/DraftMode", 0);
    RS_SETTINGS->endGroup();
    add_action(view_menu, view_toolbar, map_a["ViewDraft"]);
    map_a["ViewDraft"]->setChecked(draftMode);

    view_menu->addSeparator();
    view_toolbar->addSeparator();

    add_action(view_menu, view_toolbar, map_a["ZoomRedraw"]);
    add_action(view_menu, view_toolbar, map_a["ZoomIn"]);
    add_action(view_menu, view_toolbar, map_a["ZoomOut"]);
    add_action(view_menu, view_toolbar, map_a["ZoomAuto"]);
    previousZoom = map_a["ZoomPrevious"];
    add_action(view_menu, view_toolbar, previousZoom);
    add_action(view_menu, view_toolbar, map_a["ZoomWindow"]);
    add_action(view_menu, view_toolbar, map_a["ZoomPan"]);

    // <[~ Draw ~]>

    QMenu* draw_menu = new QMenu(tr("&Draw"), menu_bar);
    draw_menu->setObjectName("Draw");
    draw_menu->setTearOffEnabled(true);

    // <[~ Lines ~]>

    list_a.clear();

    list_a
            << map_a["DrawLine"]
            << map_a["DrawLineAngle"]
            << map_a["DrawLineHorizontal"]
            << map_a["DrawLineVertical"]
            << map_a["DrawLineRectangle"]
            << map_a["DrawLineParallelThrough"]
            << map_a["DrawLineParallel"]
            << map_a["DrawLineBisector"]
            << map_a["DrawLineTangent1"]
            << map_a["DrawLineTangent2"]
            << map_a["DrawLineOrthTan"]
            << map_a["DrawLineOrthogonal"]
            << map_a["DrawLineRelAngle"]
            << map_a["DrawLinePolygonCenCor"]
            << map_a["DrawLinePolygonCorCor"];

    sub_menu= draw_menu->addMenu(tr("&Line"));
    sub_menu->setIcon(QIcon(":/extui/menuline.png"));
    sub_menu->setObjectName("Line");
    sub_menu->addActions(list_a);

    QToolBar* line_toolbar = new QToolBar(tr("Line"), this);
    line_toolbar->setSizePolicy(toolBarPolicy);
    line_toolbar->setObjectName("line_toolbar");
    line_toolbar->addActions(list_a);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuline.png"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_line = new LC_DockWidget(this);
    dock_line->setObjectName("dock_line");
    dock_line->setWindowTitle(tr("Line"));
    dock_line->add_actions(list_a, columns, icon_size);

    line_toolbar->hide();

    // <[~ Circles ~]>

    sub_menu= draw_menu->addMenu(tr("&Circle"));
    sub_menu->setIcon(QIcon(":/extui/menucircle.png"));
    sub_menu->setObjectName("Circle");

    QToolBar* circle_toolbar = new QToolBar(tr("Circle"), this);
    circle_toolbar->setSizePolicy(toolBarPolicy);
    circle_toolbar->setObjectName ("circle_toolbar");

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menucircle.png"));
    categories_toolbar->addWidget(tool_button);

    list_a.clear();

    list_a
            << map_a["DrawCircle"]
            << map_a["DrawCircle2P"]
            << map_a["DrawCircle2PR"]
            << map_a["DrawCircle3P"]
            << map_a["DrawCircleCR"]
            << map_a["DrawCircleTan2_1P"]
            << map_a["DrawCircleTan1_2P"]
            << map_a["DrawCircleTan2"]
            << map_a["DrawCircleTan3"];

    sub_menu->addActions(list_a);
    circle_toolbar->addActions(list_a);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_circle = new LC_DockWidget(this);
    dock_circle->setObjectName("dock_circle");
    dock_circle->setWindowTitle(tr("Circle"));
    dock_circle->add_actions(list_a, columns, icon_size);

    circle_toolbar->hide();

    // <[~ Curves ~]>

    sub_menu = draw_menu->addMenu(tr("&Curve"));
    sub_menu->setIcon(QIcon(":/extui/linesfree.png"));
    sub_menu->setObjectName("Curve");

    QToolBar* curve_toolbar = new QToolBar(tr("Curve"), this);
    curve_toolbar->setSizePolicy(toolBarPolicy);
    curve_toolbar->setObjectName("curve_toolbar");

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/linesfree.png"));
    categories_toolbar->addWidget(tool_button);

    list_a.clear();

    list_a
            << map_a["DrawArc"]
            << map_a["DrawArc3P"]
            << map_a["DrawArcTangential"]
            << map_a["DrawSpline"]
            << map_a["DrawSplinePoints"]
            << map_a["DrawEllipseArcAxis"]
            << map_a["DrawLineFree"];

    sub_menu->addActions(list_a);
    curve_toolbar->addActions(list_a);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_curve = new LC_DockWidget(this);
    dock_curve->setObjectName("dock_curve");
    dock_curve->setWindowTitle(tr("Curve"));
    dock_curve->add_actions(list_a, columns, icon_size);

    curve_toolbar->hide();

    // <[~ Ellipses ~]>

    sub_menu= draw_menu->addMenu(tr("&Ellipse"));
    sub_menu->setIcon(QIcon(":/extui/menuellipse.png"));
    sub_menu->setObjectName("Ellipse");

    QToolBar* ellipse_toolbar = new QToolBar(tr("Ellipse"), this);
    ellipse_toolbar->setSizePolicy(toolBarPolicy);
    ellipse_toolbar->setObjectName("ellipse_toolbar");

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuellipse.png"));
    categories_toolbar->addWidget(tool_button);

    list_a.clear();

    list_a
            << map_a["DrawEllipseAxis"]
            << map_a["DrawEllipseFociPoint"]
            << map_a["DrawEllipse4Points"]
            << map_a["DrawEllipseCenter3Points"]
            << map_a["DrawEllipseInscribe"];

    sub_menu->addActions(list_a);
    ellipse_toolbar->addActions(list_a);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_ellipse = new LC_DockWidget(this);
    dock_ellipse->setObjectName("dock_ellipse");
    dock_ellipse->setWindowTitle(tr("Ellipse"));
    dock_ellipse->add_actions(list_a, columns, icon_size);

    ellipse_toolbar->hide();

    // <[~ Polylines ~]>

    sub_menu= draw_menu->addMenu(tr("&Polyline"));
    sub_menu->setIcon(QIcon(":/extui/menupolyline.png"));
    sub_menu->setObjectName("Polyline");

    QToolBar* polyline_toolbar = new QToolBar(tr("Polyline"), this);
    polyline_toolbar->setSizePolicy(toolBarPolicy);
    polyline_toolbar->setObjectName("polyline_toolbar");

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menupolyline.png"));
    categories_toolbar->addWidget(tool_button);

    list_a.clear();

    list_a
            << map_a["DrawPolyline"]
            << map_a["PolylineAdd"]
            << map_a["PolylineAppend"]
            << map_a["PolylineDel"]
            << map_a["PolylineDelBetween"]
            << map_a["PolylineTrim"]
            << map_a["PolylineEquidistant"]
            << map_a["PolylineSegment"];

    sub_menu->addActions(list_a);
    polyline_toolbar->addActions(list_a);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_polyline = new LC_DockWidget(this);
    dock_polyline->setObjectName("dock_polyline");
    dock_polyline->setWindowTitle(tr("Polyline"));
    dock_polyline->add_actions(list_a, columns, icon_size);

    polyline_toolbar->hide();

    // <[~ Misc ~]>

    QMenu* misc_menu = new QMenu(tr("&Misc"), menu_bar);
    misc_menu->setObjectName("misc_menu");
    misc_menu->setTearOffEnabled(true);

    QToolBar* misc_toolbar = new QToolBar(tr("Misc"), this);
    misc_toolbar->setSizePolicy(toolBarPolicy);
    misc_toolbar->setObjectName("misc_toolbar");

    list_a.clear();

    list_a
            << map_a["DrawMText"]
            << map_a["DrawHatch"]
            << map_a["DrawImage"]
            << map_a["BlocksCreate"]
            << map_a["DrawPoint"];

    misc_menu->addActions(list_a);
    misc_toolbar->addActions(list_a);

    // <[~ Select ~]>

    QMenu* select_menu = new QMenu(tr("&Select"), menu_bar);
    select_menu->setObjectName("Select");
    select_menu->setTearOffEnabled(true);

    list_a.clear();

    list_a
            << map_a["DeselectAll"]
            << map_a["SelectAll"]
            << map_a["SelectSingle"]
            << map_a["SelectContour"]
            << map_a["SelectWindow"]
            << map_a["DeselectWindow"]
            << map_a["SelectIntersected"]
            << map_a["DeselectIntersected"]
            << map_a["SelectLayer"]
            << map_a["SelectInvert"];

    select_menu->addActions(list_a);

    QToolBar* select_toolbar = new QToolBar(tr("Select"), this);
    select_toolbar->setSizePolicy(toolBarPolicy);
    select_toolbar->setObjectName("select_toolbar");
    select_toolbar->addActions(list_a);
    select_toolbar->hide();

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuselect.png"));
    categories_toolbar->addWidget(tool_button);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_select = new LC_DockWidget(this);
    dock_select->setObjectName("dock_select");
    dock_select->setWindowTitle(tr("Select"));
    dock_select->add_actions(list_a, columns, icon_size);

    // <[~ Dimension ~]>

    QMenu* dimension_menu = new QMenu(tr("&Dimension"), menu_bar);
    dimension_menu->setObjectName("dimension_menu");
    dimension_menu->setTearOffEnabled(true);

    QToolBar* dimension_toolbar = new QToolBar(tr("Dimension"), this);
    dimension_toolbar->setSizePolicy(toolBarPolicy);
    dimension_toolbar->setObjectName("dimension_toolbar");

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/dimhor.png"));
    categories_toolbar->addWidget(tool_button);

    list_a.clear();

    list_a
            << map_a["DimAligned"]
            << map_a["DimLinear"]
            << map_a["DimLinearHor"]
            << map_a["DimLinearVer"]
            << map_a["DimRadial"]
            << map_a["DimDiametric"]
            << map_a["DimAngular"]
            << map_a["DimLeader"];

    dimension_menu->addActions(list_a);
    dimension_toolbar->addActions(list_a);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_dimension = new LC_DockWidget(this);
    dock_dimension->setObjectName("dock_dimension");
    dock_dimension->setWindowTitle(tr("Dimension"));
    dock_dimension->add_actions(list_a, columns, icon_size);

    dimension_toolbar->hide();

    // <[~ Modify ~]>

    QMenu* modify_menu = new QMenu(tr("&Modify"), menu_bar);
    modify_menu->setObjectName("Modify");
    modify_menu->setTearOffEnabled(true);

    QToolBar* modify_toolbar = new QToolBar(tr("Modify"), this);
    modify_toolbar->setSizePolicy(toolBarPolicy);
    modify_toolbar->setObjectName("modify_toolbar");

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuedit.png"));
    categories_toolbar->addWidget(tool_button);

    list_a.clear();

    list_a
            << map_a["ModifyMove"]
            << map_a["ModifyRotate"]
            << map_a["ModifyScale"]
            << map_a["ModifyMirror"]
            << map_a["ModifyMoveRotate"]
            << map_a["ModifyRotate2"]
            << map_a["ModifyRevertDirection"]
            << map_a["ModifyTrim"]
            << map_a["ModifyTrim2"]
            << map_a["ModifyTrimAmount"]
            << map_a["ModifyOffset"]
            << map_a["ModifyBevel"]
            << map_a["ModifyRound"]
            << map_a["ModifyCut"]
            << map_a["ModifyStretch"]
            << map_a["ModifyEntity"]
            << map_a["ModifyAttributes"]
            << map_a["ModifyExplodeText"]
            << map_a["BlocksExplode"]
            << map_a["ModifyDeleteQuick"];

    modify_menu->addActions(list_a);
    modify_toolbar->addActions(list_a);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_modify = new LC_DockWidget(this);
    dock_modify->setObjectName("dock_modify");
    dock_modify->setWindowTitle(tr("Modify"));
    dock_modify->add_actions(list_a, columns, icon_size);

    modify_toolbar->hide();

    // <[~ Snapping ~]>

    QMenu* snap_menu = new QMenu(tr("&Snap"), menu_bar);
    snap_menu->setObjectName("snap_menu");
    snap_menu->setTearOffEnabled(true);

    snapToolBar = new QG_SnapToolBar(this, actionHandler, disable_group);
    snapToolBar->setWindowTitle(tr("Snap Selection"));
    snapToolBar->setSizePolicy(toolBarPolicy);
    snapToolBar->setObjectName("snap_toolbar" );
    actionHandler->set_snap_toolbar(snapToolBar);

    snap_menu->addActions(snapToolBar->actions());

    // <[~ Info ~]>

    QMenu* info_menu = new QMenu(tr("&Info"), menu_bar);
    info_menu->setObjectName("Info");
    info_menu->setTearOffEnabled(true);

    QToolBar* info_toolbar = new QToolBar(tr("Info"), this);
    info_toolbar->setSizePolicy(toolBarPolicy);
    info_toolbar->setObjectName("info_toolbar");

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menumeasure.png"));
    categories_toolbar->addWidget(tool_button);

    list_a.clear();

    list_a
            << map_a["InfoDist"]
            << map_a["InfoDist2"]
            << map_a["InfoAngle"]
            << map_a["InfoTotalLength"]
            << map_a["InfoArea"];

    info_menu->addActions(list_a);
    info_toolbar->addActions(list_a);
    tool_button->addActions(list_a);

    LC_DockWidget* dock_info = new LC_DockWidget(this);
    dock_info->setObjectName("dock_info");
    dock_info->setWindowTitle(tr("Info"));
    dock_info->add_actions(list_a, columns, icon_size);

    info_toolbar->hide();

    // <[~ Layer ~]>

    QMenu* layer_menu = new QMenu(tr("&Layer"), menu_bar);
    layer_menu->setObjectName("layer_menu");
    layer_menu->setTearOffEnabled(true);

    list_a.clear();

    list_a
            << map_a["LayersDefreezeAll"]
            << map_a["LayersFreezeAll"]
            << map_a["LayersAdd"]
            << map_a["LayersRemove"]
            << map_a["LayersEdit"]
            << map_a["LayersToggleLock"]
            << map_a["LayersToggleView"]
            << map_a["LayersTogglePrint"]
            << map_a["LayersToggleConstruction"];

    layer_menu->addActions(list_a);

    // <[~ Block ~]>

    QMenu* block_menu = new QMenu(tr("&Block"), menu_bar);
    block_menu->setObjectName("block_menu");
    block_menu->setTearOffEnabled(true);

    list_a.clear();

    list_a
            << map_a["BlocksDefreezeAll"]
            << map_a["BlocksFreezeAll"]
            << map_a["BlocksToggleView"]
            << map_a["BlocksAdd"]
            << map_a["BlocksRemove"]
            << map_a["BlocksAttributes"]
            << map_a["BlocksInsert"]
            << map_a["BlocksEdit"]
            << map_a["BlocksSave"]
            << map_a["BlocksCreate"]
            << map_a["BlocksExplode"];

    block_menu->addActions(list_a);

    penToolBar = new QG_PenToolBar(tr("Pen"), this);
    penToolBar->setSizePolicy(toolBarPolicy);
    penToolBar->setObjectName("pen_toolbar");

    connect(penToolBar, SIGNAL(penChanged(RS_Pen)),
    this, SLOT(slotPenChanged(RS_Pen)));

    // <[~ Tool Options ~]>

    optionWidget = new QToolBar(tr("Tool Options"), this);
    optionWidget->setSizePolicy(toolBarPolicy);
    optionWidget->setObjectName ("options_toolbar");

    // <[~ Custom Toolbar ~]>

    QString path = RS_SETTINGS->readEntry("/Paths/CustomToolbar");

    if (!path.isEmpty())
    {
        if (QFile::exists(path))
        {
            custom_toolbar = new LC_CustomToolbar(tr("Custom"), this);
            custom_toolbar->actions_from_file(path, map_a);
            custom_toolbar->setObjectName("custom_toolbar");
            custom_toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            connect(tools, SIGNAL(triggered(QAction*)), custom_toolbar, SLOT(slot_most_recent_action(QAction*)));
            addToolBar(Qt::TopToolBarArea, custom_toolbar);
        }
        else
        {
            qDebug() << "The custom toolbar file was not found.";
            RS_SETTINGS->writeEntry("/Paths/CustomToolbar", QString::null);
        }
    }

    // <[~ Windows ~]>

    windowsMenu = new QMenu(tr("&Window"), menu_bar);
    windowsMenu->setObjectName("Window");
    windowsMenu->setTearOffEnabled(true);

    connect(windowsMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotWindowsMenuAboutToShow()));

    // <[~ Help ~]>

    helpAboutApp = new QAction(QIcon(QC_APP_ICON), tr("About"), this);
    connect(helpAboutApp, SIGNAL(triggered()), this, SLOT(slotHelpAbout()));

    helpManual = new QAction(QIcon(":/main/manual.png"), tr("&Manual"), this);
    connect(helpManual, SIGNAL(triggered()), this, SLOT(slotHelpManual()));

    QAction* wiki_link = new QAction(tr("Online (Wiki)"), this);
    connect(wiki_link, SIGNAL(triggered()), this, SLOT(goto_wiki()));

    // menuBar entry helpMenu
    QMenu* help_menu = new QMenu(tr("&Help"), menu_bar);
    help_menu->setObjectName("Help");
    help_menu->setTearOffEnabled(true);

    help_menu->addAction(helpManual);
    help_menu->addAction(wiki_link);
    help_menu->addSeparator();
    help_menu->addAction(helpAboutApp);

    // <[~ Dockwidgets Toolbar ~]>

    dockwidgets_toolbar = new QToolBar(tr("DockWidgets"), this);
    dockwidgets_toolbar->setSizePolicy(toolBarPolicy);
    dockwidgets_toolbar->setObjectName("dockwidgets_toolbar");
    dockwidgets_toolbar->addAction(map_a["ViewStatusBar"]);
    dockwidgets_toolbar->addAction(map_a["ToggleToolSidebar"]);
    dockwidgets_toolbar->addAction(dock_block->toggleViewAction());
    dockwidgets_toolbar->addAction(dock_library->toggleViewAction());
    dockwidgets_toolbar->addAction(dock_command->toggleViewAction());
    dockwidgets_toolbar->addAction(dock_layer->toggleViewAction());

    // <[~ Scripting?! ~]>

#ifdef RS_SCRIPTING
    scriptMenu = new QMenu(tr("&Scripts"));
    scriptMenu->setObjectName("Scripts");
    scriptMenu->addAction(map_a["ScriptOpenIDE"]);
    scriptMenu->addAction(map_a["ScriptRun"]);
    menuBar()->addMenu(scriptMenu);
#else
    scriptMenu = 0;
    scriptOpenIDE = 0;
    scriptRun = 0;
#endif

    // <[~ SimpleTests?! ~]>

#ifdef LC_DEBUGGING
    m_pSimpleTest=new LC_SimpleTests(this);
#endif

    // <[~ Tool Sidebar ~]>

    addDockWidget(Qt::LeftDockWidgetArea, dock_line);
    tabifyDockWidget(dock_line, dock_polyline);
    dock_line->raise();
    addDockWidget(Qt::LeftDockWidgetArea, dock_circle);
    tabifyDockWidget(dock_circle, dock_curve);
    tabifyDockWidget(dock_curve, dock_ellipse);
    dock_circle->raise();
    addDockWidget(Qt::LeftDockWidgetArea, dock_dimension);
    tabifyDockWidget(dock_dimension, dock_info);
    tabifyDockWidget(dock_info, dock_select);
    dock_dimension->raise();
    addDockWidget(Qt::LeftDockWidgetArea, dock_modify);

    dock_line->hide();
    dock_polyline->hide();
    dock_circle->hide();
    dock_curve->hide();
    dock_ellipse->hide();
    dock_dimension->hide();
    dock_info->hide();
    dock_modify->hide();
    dock_select->hide();

    tool_sidebar.widgets
            << dock_line
            << dock_polyline
            << dock_circle
            << dock_curve
            << dock_ellipse
            << dock_dimension
            << dock_info
            << dock_modify
            << dock_select;

    tool_sidebar.view_action = map_a["ToggleToolSidebar"];

    // <[~ DockWidgets Menu ~]>

    QMenu* dockwidgets_menu = new QMenu(tr("&Dockwidgets"), menu_bar);
    dockwidgets_menu->setObjectName("Dockwidgets");
    dockwidgets_menu->setTearOffEnabled(true);

    dockwidget_view_actions
            << map_a["ToggleToolSidebar"]
            << dock_block->toggleViewAction()
            << dock_library->toggleViewAction()
            << dock_command->toggleViewAction()
            << dock_layer->toggleViewAction()
            << dock_line->toggleViewAction()
            << dock_polyline->toggleViewAction()
            << dock_circle->toggleViewAction()
            << dock_curve->toggleViewAction()
            << dock_ellipse->toggleViewAction()
            << dock_dimension->toggleViewAction()
            << dock_info->toggleViewAction()
            << dock_modify->toggleViewAction()
            << dock_select->toggleViewAction();

    dockwidgets_menu->addActions(dockwidget_view_actions);
    dockwidgets_menu->addAction(map_a["FocusCommand"]);

    // <[~ Toolbars Menu ~]>

    QMenu* toolbars_menu = new QMenu(tr("&Toolbars"), menu_bar);
    toolbars_menu->setObjectName("toolbars_menu");
    toolbars_menu->setTearOffEnabled(true);

    toolbar_view_actions
            << categories_toolbar->toggleViewAction()
            << file_toolbar->toggleViewAction()
            << settings_toolbar->toggleViewAction()
            << edit_toolbar->toggleViewAction()
            << view_toolbar->toggleViewAction()
            << line_toolbar->toggleViewAction()
            << circle_toolbar->toggleViewAction()
            << curve_toolbar->toggleViewAction()
            << ellipse_toolbar->toggleViewAction()
            << polyline_toolbar->toggleViewAction()
            << misc_toolbar->toggleViewAction()
            << dimension_toolbar->toggleViewAction()
            << modify_toolbar->toggleViewAction()
            << snapToolBar->toggleViewAction()
            << info_toolbar->toggleViewAction()
            << penToolBar->toggleViewAction()
            << dockwidgets_toolbar->toggleViewAction()
            << optionWidget->toggleViewAction()
            << order_toolbar->toggleViewAction()
            << select_toolbar->toggleViewAction();

    if (custom_toolbar)
    {
        toolbar_view_actions << custom_toolbar->toggleViewAction();
    }

    toolbars_menu->addActions(toolbar_view_actions);

    // <[~ Toolbars Layout~]>

    addToolBar(Qt::TopToolBarArea, file_toolbar);
    addToolBar(Qt::TopToolBarArea, edit_toolbar);
    addToolBar(Qt::TopToolBarArea, view_toolbar);
    addToolBar(Qt::TopToolBarArea, settings_toolbar);
    addToolBarBreak();
    addToolBar(Qt::TopToolBarArea, penToolBar);
    addToolBar(Qt::TopToolBarArea, optionWidget);

    addToolBar(Qt::LeftToolBarArea, categories_toolbar);
    addToolBar(Qt::LeftToolBarArea, order_toolbar);
    addToolBar(Qt::LeftToolBarArea, misc_toolbar);

    addToolBar(Qt::BottomToolBarArea, line_toolbar);
    addToolBar(Qt::BottomToolBarArea, circle_toolbar);
    addToolBar(Qt::BottomToolBarArea, curve_toolbar);
    addToolBar(Qt::BottomToolBarArea, ellipse_toolbar);
    addToolBar(Qt::BottomToolBarArea, polyline_toolbar);
    addToolBar(Qt::BottomToolBarArea, dimension_toolbar);
    addToolBar(Qt::BottomToolBarArea, modify_toolbar);
    addToolBar(Qt::BottomToolBarArea, snapToolBar);
    addToolBar(Qt::BottomToolBarArea, info_toolbar);
    addToolBar(Qt::BottomToolBarArea, dockwidgets_toolbar);
    addToolBar(Qt::BottomToolBarArea, select_toolbar);

    // <[~ MenuBar Layout~]>

    menu_bar->addMenu(file_menu);
    menu_bar->addMenu(settings_menu);
    menu_bar->addMenu(edit_menu);
    menu_bar->addMenu(view_menu);
    menu_bar->addMenu(order_menu);
    menu_bar->addMenu(select_menu);
    menu_bar->addMenu(draw_menu);
    menu_bar->addMenu(misc_menu);
    menu_bar->addMenu(dimension_menu);
    menu_bar->addMenu(modify_menu);
    menu_bar->addMenu(snap_menu);
    menu_bar->addMenu(info_menu);
    menu_bar->addMenu(layer_menu);
    menu_bar->addMenu(block_menu);
    menu_bar->addMenu(toolbars_menu);
    menu_bar->addMenu(dockwidgets_menu);
    menu_bar->addMenu(windowsMenu);
    menu_bar->addMenu(help_menu);

    // menuBar configuration
    recentFiles = new QG_RecentFiles(this, 9);
    openedFiles.clear();
}


void QC_ApplicationWindow::set_icon_size()
{
    RS_SETTINGS->beginGroup("/Appearance");
    bool custom_size = RS_SETTINGS->readNumEntry("/SetIconSize", 0);
    if (custom_size)
    {
        int icon_size = RS_SETTINGS->readNumEntry("/IconSize", 24);
        setIconSize(QSize(icon_size, icon_size));
    }
    RS_SETTINGS->endGroup();
}

void QC_ApplicationWindow::add_action(QMenu* menu, QToolBar* toolbar, QAction* action)
{
    menu->addAction(action);
    toolbar->addAction(action);
}

/**
 * Called by Qt after a toolbar or dockwidget right-click.
 * See QMainWindow::createPopupMenu() for more information.
 */
QMenu* QC_ApplicationWindow::createPopupMenu()
{
    QMenu* context_menu = new QMenu("Context");
    context_menu->setAttribute(Qt::WA_DeleteOnClose);

    QMenu* tb_menu = new QMenu("Toolbars", context_menu);
    tb_menu->addActions(toolbar_view_actions);
    context_menu->addMenu(tb_menu);

    QMenu* dw_menu = new QMenu("Dockwidgets", context_menu);
    dw_menu->addActions(dockwidget_view_actions);
    context_menu->addMenu(dw_menu);

    context_menu->addAction(statusbar_view_action);

    return context_menu;
}

void QC_ApplicationWindow::slot_fullscreen(bool checked)
{
    // SIGNAL = http://doc.qt.io/qt-5/qaction.html#checked-prop

    checked?showFullScreen():showMaximized();
}

void QC_ApplicationWindow::hide_options(QC_MDIWindow* win)
{
    win->getGraphicView()->getDefaultAction()->hideOptions();
}

void QC_ApplicationWindow::slotToggleToolSidebar(bool checked)
{
    foreach (QWidget* x, tool_sidebar.widgets)
    {
        x->setVisible(checked);
    }
}

void QC_ApplicationWindow::slotFileOpenRecent(QAction* action)
{
	RS_DEBUG->print("QC_ApplicationWindow::slotFileOpenRecent()");

    statusBar()->showMessage(tr("Opening recent file..."));
    QString fileName = action->data().toString();
    slotFileOpen(fileName, RS2::FormatUnknown);
}
