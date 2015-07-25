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

#include <QStatusBar>
#include <QMenuBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QSplitter>
#include <QMdiArea>
#include <QPluginLoader>

#if QT_VERSION < 0x040400
#include <QtAssistant/QAssistantClient>
#include <QTime>
#include "emu_qt44.h"
#else
#include <QtHelp>
#include "helpbrowser.h"
#endif // QT_VERSION 0x040400

#include "qc_applicationwindow.h"
// RVT_PORT added
#include <QImageWriter>
#if QT_VERSION >= 0x040300
#include <QtSvg>
#endif

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

//#include "qg_cadtoolbar.h"
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
#include "qc_graphicview.h"
#include "qg_pentoolbar.h"
#include "qg_recentfiles.h"

#include "rs_dialogfactory.h"
#include "qc_dialogfactory.h"
#include "main.h"
#include "doc_plugin_interface.h"
#include "qc_plugininterface.h"
#include "rs_commands.h"

#include "lc_simpletests.h"
#include "lc_actionfactory.h"


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

 QAction* QC_ApplicationWindow::previousZoom=NULL;
 QAction* QC_ApplicationWindow::undoButton=NULL;
 QAction* QC_ApplicationWindow::redoButton=NULL;

/**
 * Constructor. Initializes the app.
 */
QC_ApplicationWindow::QC_ApplicationWindow()
        : QMainWindow(0),
        QG_MainWindowInterface()
      ,m_qDraftModeTitle(" ["+tr("Draft Mode")+"]")
{
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow");

    setAttribute(Qt::WA_DeleteOnClose);
    appWindow = this;
#if QT_VERSION < 0x040400
    assistant = NULL;
#else
    helpEngine = NULL;
    helpWindow = NULL;
#endif // QT_VERSION 0x040400

    mdiAreaCAD = NULL;

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
        if (RS_DialogFactory::instance()==NULL) {
                RS_DEBUG->print("no RS_DialogFactory instance");
        }
        else {
                RS_DEBUG->print("got RS_DialogFactory instance");
        }
    RS_DialogFactory::instance()->setFactoryObject(dialogFactory);
        RS_DEBUG->print("setting dialog factory object: OK");

        RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init settings");
    initSettings();

        RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init MDI");
    initMDI();

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
    //setFocusPolicy(WheelFocus);
    previousZoomEnable=false;
    undoEnable=false;
    redoEnable=false;

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
                    "deleting action handler");
    delete actionHandler;

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting action handler: OK");
    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting dialog factory");

    delete dialogFactory;

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting dialog factory: OK");

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
        "deleting assistant..");
#if QT_VERSION < 0x040400
    if (assistant) {
        delete assistant;
    }
#else
    if (helpEngine) {
        delete helpEngine;
    }
    if (helpWindow) {
        delete helpWindow;
    }
#endif // QT_VERSION 0x040400

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting assistant: OK");

    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: OK");
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


        if (scripter==NULL) {
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
void QC_ApplicationWindow::closeEvent(QCloseEvent* ce) {
    RS_DEBUG->print("QC_ApplicationWindow::closeEvent()");

    if (!queryExit(false)) {
        ce->ignore();
    }
    else
    {
        if(mdiAreaCAD==NULL){
            ce->accept();
            return;
        }
        mdiAreaCAD->closeAllSubWindows();
        if (mdiAreaCAD->currentSubWindow()) {
            ce->ignore();
        } else {
            ce->accept();
        }
    }
//we shouldn't need this; saving should be done within ~QG_SnapToolBar()
    //snapToolBar->saveSnapMode();

    RS_DEBUG->print("QC_ApplicationWindow::closeEvent(): OK");
}



/**
 * Handles right-clicks for moving back to the last cad tool bar.
 */
//void QC_ApplicationWindow::mouseReleaseEvent(QMouseEvent* e) {
//	if (e->button()==Qt::RightButton && cadToolBar) {
//        cadToolBar->showToolBarMain();
//    }
//    e->accept();
//}

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
    activedMdiSubWindow=NULL;
    mdiAreaTab = false;
    layout->addWidget(mdiAreaCAD);
//    mdiAreaCAD->setScrollBarsEnabled(false);
    mdiAreaCAD->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiAreaCAD->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiAreaCAD->setFocusPolicy(Qt::ClickFocus);
    mdiAreaCAD->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
#if QT_VERSION >= 0x040800
    mdiAreaCAD->setTabsClosable(true);
#endif
    vb->setLayout(layout);
    setCentralWidget(vb);
    connect(mdiAreaCAD, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(slotWindowActivated(QMdiSubWindow*)));

    //this event filter allows sending key events to the command widget, therefore, no
    // need to activate the command widget before typing commands.
    // Since this nice feature causes a bug of lost key events when the command widget is on
    // a screen different from the main window, disabled for the time being
    //send key events for mdiAreaCAD to command widget by default
    mdiAreaCAD->installEventFilter(commandWidget);

    RS_DEBUG->print("QC_ApplicationWindow::initMDI() end");

}
/**
 * @return Pointer to the currently active MDI Window or NULL if no
 * MDI Window is active.
 */
QC_MDIWindow const* QC_ApplicationWindow::getMDIWindow() const{
    if (mdiAreaCAD) {
        QMdiSubWindow const* w=mdiAreaCAD->currentSubWindow();
        if(w) {
            return qobject_cast<QC_MDIWindow*>(w->widget());
        }
    }
    return nullptr;
}

QC_MDIWindow* QC_ApplicationWindow::getMDIWindow(){
    if (mdiAreaCAD) {
        QMdiSubWindow* w=mdiAreaCAD->currentSubWindow();
        if(w) {
            return qobject_cast<QC_MDIWindow*>(w->widget());
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
    recentFiles->initSettings();

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
    RS_SETTINGS->endGroup();
}


/**
 * Stores the global application settings to file or registry.
 */
void QC_ApplicationWindow::storeSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::storeSettings()");

    RS_SETTINGS->beginGroup("/Geometry");
    RS_SETTINGS->writeEntry("/WindowWidth", width());
    RS_SETTINGS->writeEntry("/WindowHeight", height());
    RS_SETTINGS->writeEntry("/WindowX", x());
    RS_SETTINGS->writeEntry("/WindowY", y());
    RS_SETTINGS->writeEntry("/DockWindows", QVariant (saveState()));
    RS_SETTINGS->endGroup();
    //save snapMode
    snapToolBar->saveSnapMode();
    RS_DEBUG->print("QC_ApplicationWindow::storeSettings(): OK");
}



/**
 * Initializes the view.
 */
void QC_ApplicationWindow::initView() {
    RS_DEBUG->print("QC_ApplicationWindow::initView()");

    RS_DEBUG->print("init view..");
    QDockWidget* dw;

    RS_DEBUG->print("  layer widget..");
    dw = new QDockWidget( "Layer", this);
    dw->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        dw->setObjectName ( "LayerDW" );
    layerWidget = new QG_LayerWidget(actionHandler, dw, "Layer");
    layerWidget->setFocusPolicy(Qt::NoFocus);
    connect(layerWidget, SIGNAL(escape()),
            this, SLOT(slotFocus()));
    connect(this, SIGNAL(windowsChanged(bool)),
            layerWidget, SLOT(setEnabled(bool)));
    //dw->boxLayout()->addWidget(layerWidget);
    dw->setWidget(layerWidget);
    //dw->setFixedExtentWidth(120);
    //dw->setFixedExtentHeight(400);
    //dw->setFixedHeight(400);
    // dw->setResizeEnabled(true);
    dw->setWindowTitle(tr("Layer List"));
    // dw->setCloseMode(QDockWidget::Always);
    //dw->resize(120,mdiAreaCAD->height()/2);
    addDockWidget (Qt::RightDockWidgetArea, dw );

    layerDockWindow = dw;

    RS_DEBUG->print("  block widget..");
    dw = new QDockWidget("Block", this);
    dw->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        dw->setObjectName ( "BlockDW" );
    // dw->setResizeEnabled(true);
    blockWidget = new QG_BlockWidget(actionHandler, dw, "Block");
    blockWidget->setFocusPolicy(Qt::NoFocus);
    connect(blockWidget, SIGNAL(escape()),
            this, SLOT(slotFocus()));
    connect(this, SIGNAL(windowsChanged(bool)),
            blockWidget, SLOT(setEnabled(bool)));
    //dw->boxLayout()->addWidget(blockWidget);
    dw->setWidget(blockWidget);
    // dw->setFixedExtentWidth(120);
    dw->setWindowTitle(tr("Block List"));
    // dw->setCloseMode(QDockWidget::Always);
    //dw->setFixedExtentHeight(400);
        addDockWidget(Qt::RightDockWidgetArea, dw);
    blockDockWindow = dw;

    RS_DEBUG->print("  library widget..");
    dw = new QDockWidget("Library", this);
    dw->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        dw->setObjectName ( "BlockDW" );
        dw->setObjectName ( "LibraryDW" );
    libraryWidget = new QG_LibraryWidget(dw, "Library");
    libraryWidget->setActionHandler(actionHandler);
    libraryWidget->setFocusPolicy(Qt::NoFocus);
    connect(libraryWidget, SIGNAL(escape()),
            this, SLOT(slotFocus()));
    connect(this, SIGNAL(windowsChanged(bool)),
            (QObject*)libraryWidget->bInsert, SLOT(setEnabled(bool)));
    dw->setWidget(libraryWidget);
    //dw->setFixedExtentWidth(240);
    //dw->setHeight(400);
    dw->resize(240, 400);
    // dw->setResizeEnabled(true);
    dw->setWindowTitle(tr("Library Browser"));
    // dw->setCloseMode(QDockWidget::Always);
    addDockWidget(Qt::LeftDockWidgetArea , dw);

    libraryDockWindow = dw;
    libraryDockWindow->hide();

    RS_DEBUG->print("  command widget..");
    dw = new QDockWidget(tr("Command line"), this);
    dw->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        dw->setObjectName ( "BlockDW" );
    dw->setFeatures(QDockWidget::DockWidgetVerticalTitleBar|QDockWidget::AllDockWidgetFeatures);
    dw->setObjectName ( "CommandDW" );
    // dw->setResizeEnabled(true);
    commandWidget = new QG_CommandWidget(dw, "Command");
    commandWidget->setActionHandler(actionHandler);
    //commandWidget->redirectStderr();
    //std::cerr << "Ready.\n";
    //commandWidget->processStderr();
    connect(this, SIGNAL(windowsChanged(bool)),
            commandWidget, SLOT(setEnabled(bool)));
    //connect(commandWidget, SIGNAL(escape()),
    //        this, SLOT(slotFocus()));
    //commandWidget->grabKeyboard();
    //dw->boxLayout()->addWidget(commandWidget);
    dw->setWidget(commandWidget);
    //dw->setFixedExtentWidth(120);
    //dw->setFixedExtentHeight(45);
//    dw->setWindowTitle();
    // dw->setCloseMode(QDockWidget::Always);
    commandDockWindow = dw;
        addDockWidget(Qt::BottomDockWidgetArea, dw);

    RS_DEBUG->print("  done");
}


/**
 * Goes back to the previous menu or one step in the current action.
 */
void QC_ApplicationWindow::slotBack() {
    RS_GraphicView* graphicView = getGraphicView();
    if (graphicView) {
        graphicView->back();
    }
//    else
//    {
//		if (cadToolBar) {
//            cadToolBar->showToolBar(RS2::ToolBarMain);
//        }
//    }
}

void QC_ApplicationWindow::slotKillAllActions() {
    RS_GraphicView* gv = getGraphicView();
    QC_MDIWindow* m = getMDIWindow();
    if (gv && m && m->getDocument()) {
        gv->killAllActions();
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);

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
void QC_ApplicationWindow::slotEnter() {
    RS_DEBUG->print("QC_ApplicationWindow::slotEnter(): begin\n");
//		if (cadToolBar) {
//            cadToolBar->forceNext();
//        } else {
            RS_GraphicView* graphicView = getGraphicView();
            if (graphicView) {
                graphicView->enter();
            }
//        }
//    }
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

    if(w==NULL) {
        emit windowsChanged(false);
        activedMdiSubWindow=w;
        return;
    }
    if(w->widget() == NULL) {
        mdiAreaCAD->removeSubWindow(w);

        mdiAreaCAD->activateNextSubWindow();
        auto w0=mdiAreaCAD->currentSubWindow();
        w0->showNormal();
        if(w0) slotWindowActivated(w0);
        return;
    }
    if(w==activedMdiSubWindow) return;
    activedMdiSubWindow=w;
    QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(w->widget());

//    QList<QMdiSubWindow*> windows=mdiAreaCAD->subWindowList();
//    int activeIndex=windows.indexOf(w);
//    std::cout<<"QC_ApplicationWindow::slotWindowActivated(QMdiSubWindow* w): activated "<< activeIndex <<std::endl;

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
            blockWidget->setBlockList(NULL);
        }

        // Update all inserts in this graphic (blocks might have changed):
        m->getDocument()->updateInserts();
        // whether to enable undo/redo buttons
        m->getDocument()->setGUIButtons();
//        m->zoomAuto();
        m->getGraphicView()->redraw();

        // set snapmode from snap toolbar
        //actionHandler->updateSnapMode();
        if(snapToolBar ){
            actionHandler->slotSetSnaps(snapToolBar->getSnaps());
        }else {
            RS_DEBUG->print(RS_Debug::D_ERROR,"snapToolBar is NULL\n");
        }

        // set pen from pen toolbar
        slotPenChanged(penToolBar->getPen());

        // update toggle button status:
        if (m->getGraphic()) {
            emit(gridChanged(m->getGraphic()->isGridOn()));
        }
        if (m->getGraphicView()) {
//            std::cout<<"QC_ApplicationWindow::slotWindowActivated(): emit(printPreviewChanged("<<m->getGraphicView()->isPrintPreview()<<")"<<std::endl;

            emit(printPreviewChanged(m->getGraphicView()->isPrintPreview()));
        }
    }

    // Disable/Enable menu and toolbar items
    emit windowsChanged(m && m->getDocument());
//    emit windowsChanged(true);
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

//            qobject_cast<QC_MDIWindow*>(w->widget())->slotZoomAuto();
            for(int i=0;i<mdiAreaCAD->subWindowList().size();i++){
                QMdiSubWindow* m=mdiAreaCAD->subWindowList().at(i);
                if( m != w){
                    m->hide();
                }
                //                qobject_cast<QC_MDIWindow*>(m)->zoomAuto();
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
        qobject_cast<QC_MDIWindow*>(window->widget())->slotZoomAuto();
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
//            qobject_cast<QC_MDIWindow*>(window)->zoomAuto();
            qobject_cast<QC_MDIWindow*>(window->widget())->slotZoomAuto();
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
         qobject_cast<QC_MDIWindow*>(window->widget())->slotZoomAuto();
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
         qobject_cast<QC_MDIWindow*>(window->widget())->slotZoomAuto();
        x+=actWidth;
    }
    mdiAreaCAD->activeSubWindow()->raise();
}

void QC_ApplicationWindow::slotToggleTab() {
    mdiAreaTab = ! mdiAreaTab;
    if(mdiAreaTab){
#if QT_VERSION >= 0x040400
        mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
#endif
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
            qobject_cast<QC_MDIWindow*>(m->widget())->slotZoomAuto();
        }

    }else{
#if QT_VERSION >= 0x040400
        mdiAreaCAD->setViewMode(QMdiArea::SubWindowView);
#endif
        slotCascade();
        //            mdiAreaCAD->setViewMode(QMdiArea::SubWindowView);
        //            QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
        //            QMdiSubWindow* active=mdiAreaCAD->activeSubWindow();
//            for(int i=0;i<windows.size();i++){
//                QMdiSubWindow* m=windows.at(i);
//                m->show();
//                if(m!=active){
//                    m->lower();
//                }else{
//                    m->showMaximized();
//                    m->raise();
//                }
//            }
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
 *  document if 'doc' is NULL.
 */

QC_MDIWindow* QC_ApplicationWindow::slotFileNew(RS_Document* doc) {

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNew() begin");

    static int id = 0;
    id++;

    statusBar()->showMessage(tr("Creating new file..."));

    RS_DEBUG->print("  creating MDI window");
    QC_MDIWindow* w = new QC_MDIWindow(doc, mdiAreaCAD, 0);
        //w->setWindowState(WindowMaximized);
    connect(w, SIGNAL(signalClosing()),
            this, SLOT(slotFileClosing()));

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
    // Link the dialog factory to the cad tool bar:
//	if (cadToolBar) {
//        //set SnapFree to avoid orphaned snapOptions, bug#3407522
//            /* setting snap option toolbar pointers to non-static fixes
//             * bug#3407522
//			if (snapToolBar && getGraphicView() && getDocument() ) {
//                    //need to detect graphicView and Document for NULL
////bug#3408689
//                RS_SnapMode s=snapToolBar->getSnaps();
//                s.snapMiddle=false;
//                s.snapDistance=false;
//                snapToolBar->setSnaps(s);
//                //cadToolBar->setSnapFree();
//            }
//            */
//        cadToolBar->showToolBar(RS2::ToolBarMain);
//        cadToolBar->resetToolBar();
//        }

//    QG_DIALOGFACTORY->setCadToolBar(cadToolBar);
    // Link the dialog factory to the command widget:
    QG_DIALOGFACTORY->setCommandWidget(commandWidget);
    // Link the dialog factory to the main app window:
    QG_DIALOGFACTORY->setMainWindow(this);

        QMdiSubWindow* subWindow=mdiAreaCAD->addSubWindow(w);

    RS_DEBUG->print("  showing MDI window");
//    if (mdiAreaCAD->subWindowList().isEmpty()) {
//        w->showMaximized();
//        w->setFocus();
//    } else {
        w->show();
        w->slotZoomAuto();
//        subWindow->showNormal();
        //show new open maximized
        subWindow->showMaximized();
        subWindow->setFocus();
        slotWindowActivated(subWindow);
//    }
//    slotWindowActivated(subWindow);
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

    if (slotFileNewHelper(fileName)==false) {
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
    QC_MDIWindow* w =NULL;
    if (slotFileNewHelper(fileName, w)==false) {
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
            w->closeMDI(true,false); //force closing, without asking user for confirmation
        }
        QMdiSubWindow* active=mdiAreaCAD->currentSubWindow();
        activedMdiSubWindow=NULL; //to allow reactivate the previous active
        if( active){//restore old geometry
            mdiAreaCAD->setActiveSubWindow(active);
            active->raise();
            active->setFocus();
            if(old==NULL || maximized){
                active->showMaximized();
            }else{
                active->setGeometry(geo);
            }
            //            qobject_cast<QC_MDIWindow*>(active->widget())->zoomAuto();
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
               w->closeMDI(true,false); //force closing, without asking user for confirmation
               QMdiSubWindow* active=mdiAreaCAD->currentSubWindow();
               activedMdiSubWindow=NULL; //to allow reactivate the previous active
               if( active){//restore old geometry
                   mdiAreaCAD->setActiveSubWindow(active);
                   active->raise();
                   active->setFocus();
                   if(old==NULL || maximized){
                       active->showMaximized();
                   }else{
                       active->setGeometry(geo);
                   }
                   qobject_cast<QC_MDIWindow*>(active->widget())->slotZoomAuto();
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
    #if QT_VERSION >= 0x040300
        supportedImageFormats.push_back("svg"); // add svg
    #endif
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
#if QT_VERSION < 0x040400
        emu_qt44_QFileDialog_setNameFilters(fileDlg, filters);
#else
        fileDlg.setNameFilters(filters);
#endif
        fileDlg.setFileMode(QFileDialog::AnyFile);
        fileDlg.selectNameFilter(defFilter);
        fileDlg.setAcceptMode(QFileDialog::AcceptSave);
        fileDlg.setDirectory(defDir);
        fn = QFileInfo(w->getDocument()->getFilename()).baseName();
        if(fn==NULL)
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
#if QT_VERSION < 0x040400
            RS_SETTINGS->writeEntry("/ExportImageFilter",
                                    emu_qt44_QFileDialog_selectedNameFilter(fileDlg) );
#else
            RS_SETTINGS->writeEntry("/ExportImageFilter",
                                    fileDlg.selectedNameFilter());
#endif
            RS_SETTINGS->endGroup();

            // find out extension:
#if QT_VERSION < 0x040400
            QString filter = emu_qt44_QFileDialog_selectedNameFilter(fileDlg);
#else
            QString filter = fileDlg.selectedNameFilter();
#endif
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
    if (w==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFileExport: "
                "no window opened");
        return false;
    }

    RS_Graphic* graphic = w->getDocument()->getGraphic();
    if (graphic==NULL) {
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
#if QT_VERSION >= 0x040300
    QSvgGenerator* vector = new QSvgGenerator();
#endif
    // set buffer var
    QPaintDevice* buffer;

#if QT_VERSION >= 0x040300
    if(format.toLower() != "svg") {
        buffer = picture;
    } else {
        vector->setSize(size);
        vector->setViewBox(QRectF(QPointF(0,0),size));
        vector->setFileName(name);
        buffer = vector;
    }
#else
    buffer = picture;
#endif

    // set painter with buffer
    RS_PainterQt painter(buffer);

    // black background:
    if (black) {
//RLZ        painter.setBackgroundColor(RS_Color(0,0,0));
        painter.setBackground(RS_Color(0,0,0));
    }
    // white background:
    else {
//RLZ        painter.setBackgroundColor(RS_Color(255,255,255));
        painter.setBackground(RS_Color(255,255,255));
    }

    // black/white:
    if (bw) {
        painter.setDrawingMode(RS2::ModeBW);
    }

    painter.eraseRect(0,0, size.width(), size.height());

    RS_StaticGraphicView gv(size.width(), size.height(), &painter, borders);
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
#if QT_VERSION >= 0x040300
    if(format.toLower() != "svg") {
#endif
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
#if QT_VERSION >= 0x040300
    }
#endif
    QApplication::restoreOverrideCursor();

    // GraphicView deletes painter
    painter.end();
    // delete vars
    delete picture;
#if QT_VERSION >= 0x040300
    delete vector;
#endif

    if (ret) {
        statusBar()->showMessage(tr("Export complete"), 2000);
    } else {
        statusBar()->showMessage(tr("Export failed!"), 2000);
    }

    return ret;
}


/**
 * Menu file -> close.
 */
void QC_ApplicationWindow::slotFileClose() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileClose(): begin");

    RS_DEBUG->print("QC_ApplicationWindow::slotFileClose(): detaching lists");
    QC_MDIWindow* w = getMDIWindow();

    if(w){
        openedFiles.removeAll(w->getDocument()->getFilename());
        //        int pos=openedFiles.indexOf(w->getDocument()->getFilename());
        //        if(pos>=0) {
        //            openedFiles.erase(openedFiles.begin()+pos);
        //        }

        //properly close print preview if exists
        QC_MDIWindow *ppv = w->getPrintPreview();
        if (ppv) {
            mdiAreaCAD->removeSubWindow(ppv->parentWidget());
        }
    }


    mdiAreaCAD->closeActiveSubWindow();
    activedMdiSubWindow=NULL;
    QMdiSubWindow* m=mdiAreaCAD->currentSubWindow();
    if(m){
        slotWindowActivated(m);
    }

}



/**
 * Called when a MDI window is actually about to close. Used to
 * detach widgets from the document.
 */
void QC_ApplicationWindow::slotFileClosing() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileClosing()");

    layerWidget->setLayerList(NULL, false);
    blockWidget->setBlockList(NULL);
    coordinateWidget->setGraphic(NULL);
    QC_MDIWindow* w = getMDIWindow();
    if(w)
        openedFiles.removeAll(w->getDocument()->getFilename());
}



/**
 * Menu file -> print.
 */
void QC_ApplicationWindow::slotFilePrint(bool printPDF) {
    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL,"QC_ApplicationWindow::slotFilePrint(%s)", printPDF ? "PDF" : "Native");

    QC_MDIWindow* w = getMDIWindow();
    if (w==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFilePrint: "
                "no window opened");
        return;
    }

    RS_Graphic* graphic = w->getDocument()->getGraphic();
    if (graphic==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFilePrint: "
                "no graphic");
        return;
    }

    statusBar()->showMessage(tr("Printing..."));
    QPrinter printer(QPrinter::HighResolution);

    bool landscape = false;
#if QT_VERSION < 0x040400
    emu_qt44_QPrinter_setPaperSize(printer, RS2::rsToQtPaperFormat(graphic->getPaperFormat(&landscape)));
#else
    QPrinter::PageSize paperSize=RS2::rsToQtPaperFormat(graphic->getPaperFormat(&landscape));
#endif // QT_VERSION 0x040400
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
        QString     strPdfFileName("");
        QFileDialog fileDlg(this, tr("Export as PDF"));
        QString     defFilter("PDF files (*.pdf)");
        QStringList filters;

        filters << defFilter
                << "Any files (*)";

#if QT_VERSION < 0x040400
        emu_qt44_QFileDialog_setNameFilters(fileDlg, filters);
#else
        fileDlg.setNameFilters(filters);
#endif
        fileDlg.setFileMode(QFileDialog::AnyFile);
        fileDlg.selectNameFilter(defFilter);
        fileDlg.setAcceptMode(QFileDialog::AcceptSave);
        fileDlg.setDefaultSuffix("pdf");
        fileDlg.setDirectory(infDefaultFile.dir().path());
        strPdfFileName = infDefaultFile.baseName();
        if( strPdfFileName.isEmpty())
            strPdfFileName = "unnamed";
        fileDlg.selectFile(strPdfFileName);

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

void QC_ApplicationWindow::slotFilePrintPreview(bool on) {
    RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview()");

    RS_DEBUG->print("  creating MDI window");
    QC_MDIWindow* parent = getMDIWindow();
    if (parent==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QC_ApplicationWindow::slotFilePrintPreview: "
                "no window opened");
        return;
    }

    // close print preview:
    if (on==false) {
        RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): off");
        if (parent->getGraphicView()->isPrintPreview()) {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): close");
            slotFileClose();
//            std::cout<<"QC_ApplicationWindow::slotFilePrintPreview(bool on): close"<<std::endl;
            emit(printPreviewChanged(false));
            if(mdiAreaCAD->subWindowList().size()>0){
                QMdiSubWindow* w=mdiAreaCAD->currentSubWindow();
                if(w){
                    mdiAreaCAD->setActiveSubWindow(w);
                }
            }
            return;
        }
    }

    // open print preview:
    else {
        // look for an existing print preview:
        QC_MDIWindow* ppv = parent->getPrintPreview();
        if (ppv) {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): show existing");

            //no need to search, casting parentWindow works like a charm
            ppv->parentWidget()->showMaximized();
            mdiAreaCAD->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(ppv->parentWidget()));
//            std::cout<<"QC_ApplicationWindow::slotFilePrintPreview(bool on): emit(printPreviewChanged(true))"<<std::endl;
            emit(printPreviewChanged(true));


        } else {
            if (!parent->getGraphicView()->isPrintPreview()) {
                //generate a new print preview
                RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): create");

                QC_MDIWindow* w = new QC_MDIWindow(parent->getDocument(), mdiAreaCAD, 0);
                                QMdiSubWindow* subWindow=mdiAreaCAD->addSubWindow(w);
                                subWindow->showMaximized();
//                                w->setWindowState(Qt::WindowMaximized);
                parent->addChildWindow(w);
                connect(w, SIGNAL(signalClosing()),
                         this, SLOT(slotFileClose()));

                w->setWindowTitle(tr("Print preview for %1").arg(parent->windowTitle()));
                w->setWindowIcon(QIcon(":/main/document.png"));
                w->slotZoomAuto();
                w->getGraphicView()->setPrintPreview(true);
                w->getGraphicView()->setBackground(RS_Color(255,255,255));
                w->getGraphicView()->setDefaultAction(
                    new RS_ActionPrintPreview(*w->getDocument(), *w->getGraphicView()));

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
                // Link the graphic view to the cad tool bar:
//                QG_DIALOGFACTORY->setCadToolBar(cadToolBar);
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
//                w->getGraphicView()->zoomPage();
//                setFocus();

                slotWindowActivated(subWindow);
//            std::cout<<"QC_ApplicationWindow::slotFilePrintPreview(bool on): new: emit(printPreviewChanged(true))"<<std::endl;
//            std::cout<<"QC_ApplicationWindow::slotFilePrintPreview(bool on): create"<<std::endl;
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
        qApp->exit(0);
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
    QList<QWidget *> windows;
    if(mdiAreaCAD)
        for(QMdiSubWindow* w: mdiAreaCAD->subWindowList())
            windows<<w;
    windows.push_back(this);

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
void QC_ApplicationWindow::redrawAll() {
    if (mdiAreaCAD) {
        QList<QMdiSubWindow*> windows = mdiAreaCAD->subWindowList();
        for (int i = 0; i < windows.size(); ++i) {
            QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i)->widget());
            if (m) {
                QG_GraphicView* gv = m->getGraphicView();
                if (gv) {
                    gv->redraw();
                }
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
            QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i)->widget());
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

    if (toggle==false) {
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

    // update background color of all open drawings:
    RS_SETTINGS->beginGroup("/Appearance");
    QColor color(RS_SETTINGS->readEntry("/BackgroundColor", "#000000"));
    QColor gridColor(RS_SETTINGS->readEntry("/GridColor", "Gray"));
    QColor metaGridColor(RS_SETTINGS->readEntry("/MetaGridColor", "Darkgray"));
    QColor selectedColor(RS_SETTINGS->readEntry("/SelectedColor", "#A54747"));
    QColor highlightedColor(RS_SETTINGS->readEntry("/HighlightedColor", "#739373"));
    QColor startHandleColor(RS_SETTINGS->readEntry("/StartHandleColor", "#00FFFF"));
    QColor handleColor(RS_SETTINGS->readEntry("/HandleColor", "#0000FF"));
    QColor endHandleColor(RS_SETTINGS->readEntry("/EndHandleColor", "#0000FF"));
    RS_SETTINGS->endGroup();

    set_icon_size();

    QList<QMdiSubWindow*> windows = mdiAreaCAD->subWindowList();
    for (int i = 0; i < windows.size(); ++i) {
        QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i)->widget());
        if (m) {
            QG_GraphicView* gv = m->getGraphicView();
            if (gv) {
                gv->setBackground(color);
                gv->setGridColor(gridColor);
                gv->setMetaGridColor(metaGridColor);
                gv->setSelectedColor(selectedColor);
                gv->setHighlightedColor(highlightedColor);
                gv->setStartHandleColor(startHandleColor);
                gv->setHandleColor(handleColor);
                gv->setEndHandleColor(endHandleColor);
//                gv->updateGrid();
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
        modules.push_back(pluginInterface->name());

    QString modulesString=tr("None");
    if (modules.empty()==false) {
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
void QC_ApplicationWindow::slotHelpManual() {
    RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual()");

#if QT_VERSION < 0x040400
    if (assistant == NULL) {
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().toLatin1().constData());
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().toLatin1().constData());
        assistant = new QAssistantClient(RS_SYSTEM->getAppDir(), this);
        connect(assistant, SIGNAL(error(const QString&)),
            this, SLOT(slotError(const QString&)));
        QStringList args;
        args << "-profile";
        args << QDir::convertSeparators(RS_SYSTEM->getDocPath() + "/qcaddoc.adp");
//        args << QString("doc") + QDir::separator() + QString("qcaddoc.adp");

#if QT_VERSION >= 0x030200
        assistant->setArguments(args);
#endif
    }
    assistant->openAssistant();
    //assistant->showPage("index.html");
#else // QT_VERSION 0x030200
    if (helpEngine==NULL) {
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().toLatin1().data());
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().toLatin1().data());

        if ((RS_SYSTEM->getDocPath().length()>0) && (QFile::exists(RS_SYSTEM->getDocPath()+ "/LibreCADdoc.qhc")==true)) {
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

            // Enable single clicking of the index
            connect(helpEngine->contentWidget(), SIGNAL(clicked(QModelIndex)), helpEngine->contentWidget(), SLOT(showLink(QModelIndex)));
            connect(helpEngine->contentWidget(), SIGNAL(linkActivated(const QUrl &)), helpBrowser, SLOT(setSource(const QUrl &)));
            addDockWidget(Qt::TopDockWidgetArea, helpWindow);
        } else {
            QMessageBox::information(this, tr("Help files not found"), tr("Bugger, I couldn't find the helpfiles on the filesystem."));
        }

    }
    if (helpWindow) {
        helpWindow->show();
    }
#endif // QT_VERSION 0x040400
}

/**
 * overloaded for Message box on last window exit.
 */
bool QC_ApplicationWindow::queryExit(bool force) {
    RS_DEBUG->print("QC_ApplicationWindow::queryExit()");

    bool succ = true;


         QList<QMdiSubWindow*> list = mdiAreaCAD->subWindowList();

         while (!list.isEmpty()) {
             QC_MDIWindow *tmp=qobject_cast<QC_MDIWindow*>(list.takeFirst()->widget());
             if( tmp){
                 succ = tmp->closeMDI(force);
                 if (!succ) {
                     break;
                 }
             }
        }

    if (succ) {
        storeSettings();
    }

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
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Meta:
        case Qt::Key_Alt:
        case Qt::Key_CapsLock: {
            QMainWindow::keyPressEvent(e);

            // forward to actions:
            RS_GraphicView* graphicView = getGraphicView();
            if (graphicView) {
                graphicView->keyPressEvent(e);
            }
            e->accept();
        }
            break;

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


void QC_ApplicationWindow::keyReleaseEvent(QKeyEvent* e) {

    switch (e->key()) {
    case Qt::Key_Shift:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Alt:
    case Qt::Key_CapsLock: {
            QMainWindow::keyReleaseEvent(e);

            // forward to actions:
            RS_GraphicView* graphicView = getGraphicView();
            if (graphicView) {
                graphicView->keyReleaseEvent(e);
            }
            e->accept();
        }
        break;
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
    getGraphicView()->set_action(q_action);
}

// github.com/r-a-v-a-s/LibreCAD.git
// ravas@outlook.com - 2015
void QC_ApplicationWindow::menus_and_toolbars()
{
    RS_DEBUG->print("QC_ApplicationWindow::menus_and_toolbars()");

    set_icon_size();

    QSizePolicy toolBarPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QAction* action;
    QMenu* menu;
    QMenu* sub_menu;
    QToolBar* toolbar;
    QToolButton* tool_button;

    std::vector<std::string> a_list;
    std::vector<QToolBar*> list_tb; // for creating the toolbar menu

    QActionGroup* tools = new QActionGroup(this);
    connect(tools, SIGNAL(triggered(QAction*)), this, SLOT(slot_set_action(QAction*)));
    LC_ActionFactory a_factory(actionHandler, this, tools);
    std::map<std::string, QAction*> a_map;
    a_map = a_factory.action_map();

    QToolBar* tb_categories = new QToolBar("Categories", this);
    tb_categories->setSizePolicy(toolBarPolicy);
    tb_categories->setObjectName("CategoriesTB");
    list_tb.push_back(tb_categories);


    // <[~ File ~]>

    menu = menuBar()->addMenu(tr("&File"));
    menu->setObjectName("File");

    tb_file = new QToolBar("File Operations", this);
    tb_file->setSizePolicy(toolBarPolicy);
    tb_file->setObjectName("FileTB");
    list_tb.push_back(tb_file);

    a_list = {"FileNew", "FileNewTemplate", "FileOpen", "FileSave", "FileSaveAs"};

    for (std::string a : a_list)
    {
        add_action(menu, tb_file, a_map[a]);
    }

    sub_menu = menu->addMenu(QIcon(":/actions/fileimport.png"), tr("Import"));
    sub_menu->setObjectName("Import");
    sub_menu->addAction(a_map["DrawImage"]);
    sub_menu->addAction(a_map["BlocksImport"]);

    sub_menu = menu->addMenu(QIcon(":/actions/fileexport.png"), tr("Export"));
    sub_menu->setObjectName("Export");
    sub_menu->addAction(a_map["FileExportMakerCam"]);
    sub_menu->addAction(a_map["FilePrintPDF"]);
    sub_menu->addAction(a_map["FileExport"]);

    menu->addSeparator();

    menu->addAction(a_map["FilePrint"]);
    menu->addAction(a_map["FilePrintPreview"]);
    tb_file->addAction(a_map["FilePrint"]);
    tb_file->addAction(a_map["FilePrintPreview"]);

    menu->addSeparator();

    menu->addAction(a_map["FileClose"]);
    menu->addAction(a_map["FileQuit"]);

    menu->addSeparator();

    addToolBar(Qt::TopToolBarArea, tb_file);
    fileMenu = menu;

    // <[~ Edit ~]>

    menu = menuBar()->addMenu(tr("&Edit"));
    menu->setObjectName("Edit");

    tb_edit = new QToolBar("Edit Operations", this);
    tb_edit->setSizePolicy(toolBarPolicy);
    tb_edit->setObjectName ("EditTB" );
    list_tb.push_back(tb_edit);

    add_action(menu, tb_edit, a_map["EditKillAllActions"]);

    tb_edit->addSeparator();
    menu->addSeparator();

    undoButton = a_map["EditUndo"];
    redoButton = a_map["EditRedo"];

    add_action(menu, tb_edit, undoButton);
    add_action(menu, tb_edit, redoButton);

    tb_edit->addSeparator();
    menu->addSeparator();

    add_action(menu, tb_edit, a_map["EditCut"]);
    add_action(menu, tb_edit, a_map["EditCopy"]);
    add_action(menu, tb_edit, a_map["EditPaste"]);

    menu->addSeparator();

    // <[~ Order ~]>

    sub_menu= menu->addMenu(tr("Draw &Order"));
    sub_menu->setObjectName("Order");
    sub_menu->addAction(a_map["OrderBottom"]);
    sub_menu->addAction(a_map["OrderTop"]);
    sub_menu->addAction(a_map["OrderLower"]);
    sub_menu->addAction(a_map["OrderRaise"]);

    // <[~ Options ~]>

    menu->addAction(a_map["OptionsGeneral"]);
    menu->addAction(a_map["OptionsDrawing"]);

    addToolBar(Qt::TopToolBarArea, tb_edit); //tr("Edit");

    // <[~ View ~]>

    menu = menuBar()->addMenu(tr("&View"));
    menu->setObjectName("View");

    tb_zoom = new QToolBar("Zoom Operations", this);
    tb_zoom->setSizePolicy(toolBarPolicy);
    tb_zoom->setObjectName("ZoomTB");
    list_tb.push_back(tb_zoom);

    add_action(menu, tb_zoom, a_map["ViewGrid"]);

    RS_SETTINGS->beginGroup("/Appearance");
    bool draftMode = (bool)RS_SETTINGS->readNumEntry("/DraftMode", 0);
    RS_SETTINGS->endGroup();
    add_action(menu, tb_zoom, a_map["ViewDraft"]);
    a_map["ViewDraft"]->setChecked(draftMode);

    menu->addSeparator();
    tb_zoom->addSeparator();

    add_action(menu, tb_zoom, a_map["ZoomRedraw"]);
    add_action(menu, tb_zoom, a_map["ZoomIn"]);
    add_action(menu, tb_zoom, a_map["ZoomOut"]);
    add_action(menu, tb_zoom, a_map["ZoomAuto"]);
    previousZoom = a_map["ZoomPrevious"];
    add_action(menu, tb_zoom, previousZoom);
    add_action(menu, tb_zoom, a_map["ZoomWindow"]);
    add_action(menu, tb_zoom, a_map["ZoomPan"]);

    addToolBar(Qt::TopToolBarArea, tb_zoom);

    // <[~ Select ~]>

    menu = menuBar()->addMenu(tr("&Select"));
    menu->setObjectName("Select");

    a_list = {"DeselectAll"
             ,"SelectAll"
             ,"SelectSingle"
             ,"SelectContour"
             ,"SelectWindow"
             ,"DeselectWindow"
             ,"SelectIntersected"
             ,"DeselectIntersected"
             ,"SelectLayer"
             ,"SelectInvert"
             };

    for (std::string a : a_list)
    {
        menu->addAction(a_map[a]);
    }

    // <[~ Draw ~]>

    menu = menuBar()->addMenu(tr("&Draw"));
    menu->setObjectName("Draw");

    // <[~ Lines ~]>

    sub_menu= menu->addMenu(tr("&Line"));
    sub_menu->setObjectName("Line");

    toolbar = new QToolBar("Line Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("LineTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuline.png"));
    tb_categories->addWidget(tool_button);

    a_list =  {"DrawLine"
              ,"DrawLineAngle"
              ,"DrawLineHorizontal"
              ,"DrawLineVertical"
              ,"DrawLineRectangle"
              ,"DrawLineParallel"
              ,"DrawLineParallelThrough"
              ,"DrawLineBisector"
              ,"DrawLineTangent1"
              ,"DrawLineTangent2"
              ,"DrawLineOrthTan"
              ,"DrawLineOrthogonal"
              ,"DrawLineRelAngle"
              ,"DrawLinePolygonCenCor"
              ,"DrawLinePolygonCorCor"
              ,"DrawLineFree"};


    for (std::string a : a_list)
    {
        add_action(sub_menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::TopToolBarArea, toolbar);

    // <[~ Arcs ~]>

    sub_menu= menu->addMenu(tr("&Arc"));
    sub_menu->setObjectName("Arc");

    toolbar = new QToolBar("Arc Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("ArcTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuarc.png"));
    tb_categories->addWidget(tool_button);

    a_list = {"DrawArc"
             ,"DrawArc3P"
             ,"DrawArcParallel"
             ,"DrawArcTangential"};


    for (std::string a : a_list)
    {
        add_action(sub_menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::BottomToolBarArea, toolbar);

    // <[~ Circles ~]>

    sub_menu= menu->addMenu(tr("&Circle"));
    sub_menu->setObjectName("Circle");

    toolbar = new QToolBar("Circle Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName ("CirclesTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menucircle.png"));
    tb_categories->addWidget(tool_button);

    a_list = {"DrawCircleTan3"
             ,"DrawCircleCR"
             ,"DrawCircle2P"
             ,"DrawCircle2PR"
             ,"DrawCircle3P"
             ,"DrawCircleParallel"
             ,"DrawCircleInscribe"
             ,"DrawCircleTan1_2P"
             ,"DrawCircleTan2"
             ,"DrawCircleTan2_1P"
             ,"DrawCircle"};

    for (std::string a : a_list)
    {
        add_action(sub_menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::TopToolBarArea, toolbar);

    // <[~ Ellipses ~]>

    sub_menu= menu->addMenu(tr("&Ellipse"));
    sub_menu->setObjectName("Ellipse");

    toolbar = new QToolBar("Ellipse Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("EllipseTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuellipse.png"));
    tb_categories->addWidget(tool_button);


    a_list = {"DrawEllipseAxis"
             ,"DrawEllipseArcAxis"
             ,"DrawEllipseFociPoint"
             ,"DrawEllipse4Points"
             ,"DrawEllipseCenter3Points"
             ,"DrawEllipseInscribe"};

    for (std::string a : a_list)
    {
        add_action(sub_menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::BottomToolBarArea, toolbar);

    // <[~ Splines ~]>

    sub_menu= menu->addMenu(tr("&Spline"));
    sub_menu->setObjectName("Spline");

    toolbar = new QToolBar("Spline Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("SplineTB");
    list_tb.push_back(toolbar);

    add_action(sub_menu, toolbar, a_map["DrawSpline"]);
    add_action(sub_menu, toolbar, a_map["DrawSplinePoints"]);

    addToolBar(Qt::BottomToolBarArea, toolbar);

    // <[~ Polylines ~]>

    sub_menu= menu->addMenu(tr("&Polyline"));
    sub_menu->setObjectName("Polyline");

    toolbar = new QToolBar("Polyline Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("PolylineTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menupolyline.png"));
    tb_categories->addWidget(tool_button);

    a_list = {"DrawPolyline"
              ,"PolylineAdd"
              ,"PolylineAppend"
              ,"PolylineDel"
              ,"PolylineDelBetween"
              ,"PolylineTrim"
              ,"PolylineEquidistant"
              ,"PolylineSegment"};


    for (std::string a : a_list)
    {
        add_action(sub_menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::BottomToolBarArea, toolbar);

    // Text:
    menu = menuBar()->addMenu(tr("&Misc"));
    sub_menu->setObjectName("MiscMenu");

    // <[~ Misc ~]>

    toolbar = new QToolBar("Misc Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("MiscTB");
    list_tb.push_back(toolbar);

    a_list = {"DrawMText",
                "DrawHatch",
                "DrawImage",
                "BlocksCreate",
                "DrawPoint"};

    for (std::string a : a_list)
    {
        add_action(menu, toolbar, a_map[a]);
    }

    addToolBar(Qt::LeftToolBarArea, toolbar);

    // <[~ Dimension ~]>

#ifdef __APPLE1__
    QMenu* m = menu;
    menu= m->addMenu(tr("&Dimension"));
#else
    menu = menuBar()->addMenu(tr("&Dimension"));
#endif

    menu->setObjectName("Dimension");

    toolbar = new QToolBar("Dimension Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("DimensionsTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/dimhor.png"));
    tb_categories->addWidget(tool_button);

    a_list = {"DimAligned"
              ,"DimLinear"
              ,"DimLinearHor"
              ,"DimLinearVer"
              ,"DimRadial"
              ,"DimDiametric"
              ,"DimAngular"
              ,"DimLeader"};


    for (std::string a : a_list)
    {
        add_action(menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::BottomToolBarArea, toolbar);

    // <[~ Modify ~]>

    menu = menuBar()->addMenu(tr("&Modify"));
    menu->setObjectName("Modify");

    toolbar = new QToolBar("Modify Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("ModifyTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menuedit.png"));
    tb_categories->addWidget(tool_button);


    a_list = {"ModifyMove"
                ,"ModifyRotate"
                ,"ModifyScale"
                ,"ModifyMirror"
                ,"ModifyMoveRotate"
                ,"ModifyRotate2"
                ,"ModifyRevertDirection"
                ,"ModifyTrim"
                ,"ModifyTrim2"
                ,"ModifyTrimAmount"
                ,"ModifyOffset"
                ,"ModifyBevel"
                ,"ModifyRound"
                ,"ModifyCut"
                ,"ModifyStretch"
                ,"ModifyEntity"
                ,"ModifyAttributes"
                ,"ModifyDelete"
                ,"ModifyDeleteQuick"
                ,"ModifyExplodeText"
                ,"BlocksExplode"};


    for (std::string a : a_list)
    {
        add_action(menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::RightToolBarArea, toolbar);

    // <[~ Snapping ~]>

    menu = menuBar()->addMenu(tr("&Snap"));
    menu->setObjectName("Snap");

    snapToolBar = new QG_SnapToolBar(tr("Snap Selection"),actionHandler, this);
    snapToolBar->setSizePolicy(toolBarPolicy);
    snapToolBar->setObjectName("SnapTB" );
    list_tb.push_back(snapToolBar);

    connect(this, SIGNAL(windowsChanged(bool)), snapToolBar, SLOT(setEnabled(bool)));
    this->addToolBar(snapToolBar);

    if(snapToolBar) {
        for(QAction* a : snapToolBar->getActions()){
            menu->addAction(a);
        }
    }

    addToolBar(Qt::LeftToolBarArea, snapToolBar);

    // <[~ Info ~]>

    menu = menuBar()->addMenu(tr("&Info"));
    menu->setObjectName("Info");

    toolbar = new QToolBar("Info Tools", this);
    toolbar->setSizePolicy(toolBarPolicy);
    toolbar->setObjectName("InfoTB");
    list_tb.push_back(toolbar);

    tool_button = new QToolButton;
    tool_button->setPopupMode(QToolButton::InstantPopup);
    tool_button->setIcon(QIcon(":/extui/menumeasure.png"));
    tb_categories->addWidget(tool_button);

    a_list = {"InfoDist"
               ,"InfoDist2"
               ,"InfoAngle"
               ,"InfoTotalLength"
               ,"InfoArea"};

    for (std::string a : a_list)
    {
        add_action(menu, toolbar, a_map[a]);
        tool_button->addAction(a_map[a]);
    }

    addToolBar(Qt::TopToolBarArea, toolbar);

    // <[~ Layer ~]>

    menu = menuBar()->addMenu(tr("&Layer"));
    menu->setObjectName("Layer");

    a_list =  {"LayersDefreezeAll"
               ,"LayersFreezeAll"
               ,"LayersAdd"
               ,"LayersRemove"
               ,"LayersEdit"
               ,"LayersToggleLock"
               ,"LayersToggleView"
               ,"LayersTogglePrint"
               ,"LayersToggleConstruction"};

    for (std::string a : a_list)
    {
        menu->addAction(a_map[a]);
    }

    // <[~ Block ~]>

    menu = menuBar()->addMenu(tr("&Block"));
    menu->setObjectName("Block");

    a_list = {"BlocksDefreezeAll"
               ,"BlocksFreezeAll"
               ,"BlocksToggleView"
               ,"BlocksAdd"
               ,"BlocksRemove"
               ,"BlocksAttributes"
               ,"BlocksInsert"
               ,"BlocksEdit"
               ,"BlocksSave"
               ,"BlocksCreate"
               ,"BlocksExplode"};

    for (std::string a : a_list)
    {
        menu->addAction(a_map[a]);
    }

    penToolBar = new QG_PenToolBar(tr("Pen Selection"), this);
    penToolBar->setSizePolicy(toolBarPolicy);
    penToolBar->setObjectName ( "PenTB" );
    list_tb.push_back(penToolBar);

    connect(penToolBar, SIGNAL(penChanged(RS_Pen)),
    this, SLOT(slotPenChanged(RS_Pen)));

    addToolBar(Qt::TopToolBarArea, penToolBar);

    // <[~ Tool Options ~]>

    optionWidget = new QToolBar(tr("Tool Options"), this);
    //        optionWidget->setMinimumSize(440,30);
    optionWidget->setSizePolicy(toolBarPolicy);
    optionWidget->setObjectName ("ToolTB");
    list_tb.push_back(optionWidget);


    // <[~ Toolbar Menu ~]>

    menu = menuBar()->addMenu(tr("&Toolbars"));
    menu->setObjectName("Toolbars");

    addToolBar(Qt::TopToolBarArea, optionWidget);

    for (QToolBar* tb : list_tb)
    {
        menu->addAction(tb->toggleViewAction());
    }


    // <[~ DockWidgets ~]>

    menu = menuBar()->addMenu(tr("&Dockwidgets"));
    menu->setObjectName("Dockwidgets");

    tb_wigets = new QToolBar( "DockWidgets", this);
    tb_wigets->setSizePolicy(toolBarPolicy);
    tb_wigets->setObjectName ( "DockWidgetsTB" );

    add_action(menu, tb_wigets, a_map["ViewStatusBar"]);
    add_action(menu, tb_wigets, blockDockWindow->toggleViewAction());
    add_action(menu, tb_wigets, libraryDockWindow->toggleViewAction());
    add_action(menu, tb_wigets, commandDockWindow->toggleViewAction());
    add_action(menu, tb_wigets, layerDockWindow->toggleViewAction());

    menu->addSeparator();

    // tr("Focus on Command Line")
    action = new QAction(tr("Focus on &Command Line"), this);
    action->setIcon(QIcon(":/main/editclear.png"));

    //added commandline shortcuts, feature request# 3437106
    QList<QKeySequence> commandLineShortcuts;
    commandLineShortcuts<<QKeySequence(Qt::CTRL + Qt::Key_M)<<QKeySequence(Qt::Key_Colon)<<QKeySequence(Qt::Key_Space);
    action->setShortcuts(commandLineShortcuts);

    connect(action, SIGNAL(triggered()), this, SLOT(slotFocusCommandLine()));
    menu->addAction(action);

    addToolBar(Qt::BottomToolBarArea, tb_wigets);

    windowsMenu = menuBar()->addMenu(tr("&Window"));
    windowsMenu->setObjectName("Window");
    connect(windowsMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotWindowsMenuAboutToShow()));

    // menuBar configuration
    recentFiles.reset(new QG_RecentFiles(9));
    openedFiles.clear();

    // <[~ Help ~]>

    helpAboutApp = new QAction( QIcon(QC_APP_ICON), tr("About"), this);

    //helpAboutApp->zetStatusTip(tr("About the application"));
    //helpAboutApp->setWhatsThis(tr("About\n\nAbout the application"));
    connect(helpAboutApp, SIGNAL(triggered()),
            this, SLOT(slotHelpAbout()));

    helpManual = new QAction( QIcon(":/main/manual.png"), tr("&Manual"), this);
    connect( helpManual, SIGNAL(triggered()), this, SLOT(slotHelpManual()));

    // menuBar entry helpMenu
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->setObjectName("Help");
    helpMenu->addAction(helpManual);
    helpMenu->addSeparator();
    helpMenu->addAction(helpAboutApp);

#ifdef RS_SCRIPTING
    // Scripts menu:
    //
    scriptMenu = new QMenu(tr("&Scripts"));
    scriptMenu->setObjectName("Scripts");
    scriptOpenIDE = AF.createAction("ScriptOpenIDE", this);
    scriptOpenIDE->addTo(scriptMenu);
    scriptRun = AF.createAction("ScriptRun", this);
    scriptMenu->addAction(scriptRun);
    menuBar()->addMenu(scriptMenu);
#else
    scriptMenu = 0;
    scriptOpenIDE = 0;
    scriptRun = 0;
#endif

#ifdef LC_DEBUGGING
    m_pSimpleTest=new LC_SimpleTests(this);
#endif


    QToolBar* tb_construct = new QToolBar("Construction", this);
    tb_construct->setSizePolicy(toolBarPolicy);
    tb_construct->setObjectName("ConstructionTB" );

    tb_construct->addAction(a_map["DrawCircle"]);
    tb_construct->addAction(a_map["DrawCircle2P"]);
    tb_construct->addAction(a_map["DrawCircle3P"]);
    tb_construct->addAction(a_map["DrawCircleTan3"]);
    tb_construct->addSeparator();
    tb_construct->addAction(a_map["DrawLine"]);

    addToolBar(Qt::BottomToolBarArea, tb_construct);

    addToolBar(Qt::LeftToolBarArea, tb_categories);


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

