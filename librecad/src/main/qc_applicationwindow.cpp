/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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

#if QT_VERSION < 0x040400
#include <QtAssistant/QAssistantClient>
#include <QTime>
#include "emu_qt44.h"
#else
#include <QtHelp>
#include "helpbrowser.h"
#endif // QT_VERSION 0x040400

#include <QSplitter>
#include <QMdiArea>

#include "qc_applicationwindow.h"
// RVT_PORT added
#include <QImageWriter>
#if QT_VERSION >= 0x040300
#include <QtSvg>
#endif

#include <fstream>

#include <QPrinter>
#include <QPrintDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

//Plugin support
#include <QPluginLoader>

#include "rs_actionprintpreview.h"
#include "rs_dimaligned.h"
#include "rs_dimlinear.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_text.h"
#include "rs_settings.h"
#include "rs_staticgraphicview.h"
#include "rs_system.h"
#include "rs_actionlibraryinsert.h"
#include "rs_painterqt.h"
#include "rs_selection.h"

#include "qg_cadtoolbar.h"
#include "qg_snaptoolbar.h"
#include "qg_actionfactory.h"
#include "qg_blockwidget.h"
#include "qg_layerwidget.h"
#include "qg_librarywidget.h"
#include "qg_commandwidget.h"

#include "qg_coordinatewidget.h"
#include "qg_dlgimageoptions.h"
#include "qg_filedialog.h"
#include "qg_selectionwidget.h"
#include "qg_mousewidget.h"

#include "rs_dialogfactory.h"
#include "qc_dialogfactory.h"
#include "main.h"
#include "doc_plugin_interface.h"
#include "qc_plugininterface.h"
#include "rs_commands.h"


QC_ApplicationWindow* QC_ApplicationWindow::appWindow = NULL;

#ifndef QC_APP_ICON
# define QC_APP_ICON ":/main/librecad.png"
#endif
#ifndef QC_ABOUT_ICON
# define QC_ABOUT_ICON ":/main/intro_librecad.png"
#endif
#ifndef QC_APP_ICON16
# define QC_APP_ICON16 ":/main/librecad16.png"
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
        RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init toolbar");
    initToolBar();
        RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init actions");
    initActions();
        RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init menu bar");
    initMenuBar();
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
    emit windowsChanged(FALSE);

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
        i++;
    }
    return 0;
}

/**
 * Loads the found plugins.
 */
void QC_ApplicationWindow::loadPlugins() {

    loadedPlugins.clear();
    QStringList lst = RS_SYSTEM->getDirectoryList("plugins");

    for (int i = 0; i < lst.size(); ++i) {
        QDir pluginsDir(lst.at(i));
        foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
            QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(fileName));
            QObject *plugin = pluginLoader.instance();
            if (plugin) {
                QC_PluginInterface *pluginInterface = qobject_cast<QC_PluginInterface *>(plugin);
                if (pluginInterface) {
                    loadedPlugins.append(pluginInterface);
                    PluginCapabilities pluginCapabilities=pluginInterface->getCapabilities();
                    foreach (PluginMenuLocation loc,  pluginCapabilities.menuEntryPoints) {
                        QAction *actpl = new QAction(loc.menuEntryActionName, plugin);
                        actpl->setData(loc.menuEntryActionName);
                        connect(actpl, SIGNAL(triggered()), this, SLOT(execPlug()));
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
    RS_Graphic* currdoc = static_cast<RS_Graphic*>(w->getDocument());
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
    if (assistant != NULL) {
        delete assistant;
    }
#else
    if (helpEngine!=NULL) {
        delete helpEngine;
    }
    if (helpWindow!=NULL) {
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
    if (s!=NULL) {
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
        scriptList.append(RS_SYSTEM->getHomeDir() + "/." XSTR(QC_APPKEY) "/" + name);

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
    if (s!=NULL) {
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
        if (graphicView!=NULL && document!=NULL) {
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
void QC_ApplicationWindow::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton && cadToolBar!=NULL) {
        cadToolBar->showToolBarMain();
    }
    e->accept();
}



/**
 * Initializes the MDI mdiAreaCAD.
 */
void QC_ApplicationWindow::initMDI() {
    RS_DEBUG->print("QC_ApplicationWindow::initMDI() begin");

    QFrame *vb = new QFrame(this);
    vb->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
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

    RS_DEBUG->print("QC_ApplicationWindow::initMDI() end");
}
/**
 * @return Pointer to the currently active MDI Window or NULL if no
 * MDI Window is active.
 */
QC_MDIWindow* QC_ApplicationWindow::getMDIWindow() {
    if (mdiAreaCAD!=NULL) {
        QMdiSubWindow* w=mdiAreaCAD->currentSubWindow();
        if(w!=NULL) {

            return qobject_cast<QC_MDIWindow*>(w->widget());
        }
    }
        return NULL;
}

/*	*
 *	Description:	Initializes all QActions of the application.
 *	Author(s):		..., Claude Sylvain
 *	Created:
 *	Last modified:	16 July 2011
 *	Parameters:		void
 *	Returns:			void
 *	*/

void QC_ApplicationWindow::initActions(void)
{
    RS_DEBUG->print("QC_ApplicationWindow::initActions()");

    QG_ActionFactory actionFactory(actionHandler, this);
    QAction* action;
    QMenu* menu;
    QToolBar* tb;
    QMenu* subMenu;

    // File actions:
    //
    menu = menuBar()->addMenu(tr("&File"));
    menu->setObjectName("File");
    tb = fileToolBar;
    tb->setWindowTitle("File");

    action = actionFactory.createAction(RS2::ActionFileNew, this);
    menu->addAction(action);
    tb->addAction(action);
    action = actionFactory.createAction(RS2::ActionFileOpen, this);
    menu->addAction(action);
    tb->addAction(action);
    action = actionFactory.createAction(RS2::ActionFileSave, this);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionFileSaveAs, this);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionFileExport, this);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    subMenu = menu->addMenu(tr("Import"));
    subMenu->setObjectName("Import");

    //insert images
    // Image:
    action = actionFactory.createAction(RS2::ActionDrawImage,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    // Block:
    action = new QAction(QIcon(":/ui/blockinsert.png"), tr("&Block"), this);
    subMenu->addAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(slotImportBlock()));

    menu->addSeparator();
    action = actionFactory.createAction(RS2::ActionFileClose, this);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->addSeparator();
    action = actionFactory.createAction(RS2::ActionFilePrint, this);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionFilePrintPreview, this);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->addSeparator();
    action = actionFactory.createAction(RS2::ActionFileQuit, this);
    menu->addAction(action);
    menu->addSeparator();
    addToolBar(Qt::TopToolBarArea, tb); //tr("File");

    fileMenu = menu;

    // Editing actions:
    //
    menu = menuBar()->addMenu(tr("&Edit"));
    menu->setObjectName("Edit");
    tb = editToolBar;
    tb->setWindowTitle("Edit");

    action = actionFactory.createAction(RS2::ActionEditKillAllActions, actionHandler);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    tb->addSeparator();

    undoButton = actionFactory.createAction(RS2::ActionEditUndo, actionHandler);
    menu->addAction(undoButton);
    tb->addAction(undoButton);
    connect(this, SIGNAL(windowsChanged(bool)), this, SLOT(slotEnableActions(bool)));

    redoButton = actionFactory.createAction(RS2::ActionEditRedo, actionHandler);
    menu->addAction(redoButton);
    tb->addAction(redoButton);
    connect(this, SIGNAL(windowsChanged(bool)), this, SLOT(slotEnableActions(bool)));

    tb->addSeparator();
    menu->addSeparator();

    action = actionFactory.createAction(RS2::ActionEditCut, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionEditCopy, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionEditPaste, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    menu->addSeparator();
    // Draw order:
    subMenu= menu->addMenu(tr("Draw &Order"));
    subMenu->setObjectName("Order");
    action = actionFactory.createAction(RS2::ActionOrderBottom, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionOrderLower, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionOrderRaise, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionOrderTop, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionOptionsGeneral, this);
    menu->addAction(action);
    action = actionFactory.createAction(RS2::ActionOptionsDrawing, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    //addToolBar(tb, tr("Edit"));
        addToolBar(Qt::TopToolBarArea, tb); //tr("Edit");

    // Options menu:
    //
    //menu = new QPopupMenu(this);
    //menuBar()->insertItem(tr("&Options"), menu);


    // Viewing / Zooming actions:
    //
    menu = menuBar()->addMenu(tr("&View"));
    menu->setObjectName("View");
    tb = zoomToolBar;
    tb->setWindowTitle("View");

    action = actionFactory.createAction(RS2::ActionViewGrid, this);
    menu->addAction(action);
    tb->addAction(action);
    action->setChecked(true);
    connect(this, SIGNAL(gridChanged(bool)), action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    RS_SETTINGS->beginGroup("/Appearance");
    bool draftMode = (bool)RS_SETTINGS->readNumEntry("/DraftMode", 0);
    RS_SETTINGS->endGroup();

    action = actionFactory.createAction(RS2::ActionViewDraft, this);
    menu->addAction(action);
    tb->addAction(action);
    action->setChecked(draftMode);
    connect(this, SIGNAL(draftChanged(bool)), action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    /*
    action = new QAction(tr("Back"),
                        tr("&Back"), Key_Escape, this);
       connect(action, SIGNAL(activated()),
               this, SLOT(slotBack()));
       action->addTo(menu);
    */


    action = actionFactory.createAction(RS2::ActionZoomRedraw, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomIn, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomOut, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomAuto, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    previousZoom = actionFactory.createAction(RS2::ActionZoomPrevious, actionHandler);
    menu->addAction(previousZoom);
    tb->addAction(previousZoom);
//    connect(this, SIGNAL(windowsChanged(bool)), previousZoom, SLOT(setEnabled(bool)));
    previousZoom->setEnabled(false);
    connect(this, SIGNAL(windowsChanged(bool)), this, SLOT(slotEnableActions(bool)));
    action = actionFactory.createAction(RS2::ActionZoomWindow, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomPan, actionHandler);
    menu->addAction(action);
    tb->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    menu->addSeparator();

    action = actionFactory.createAction(RS2::ActionViewStatusBar, this);
    action->setChecked(true);
    menu->addAction(action);

    subMenu= menu->addMenu(tr("&Toolbars"));
    subMenu->setObjectName("Toolbars");

    action = actionFactory.createAction(RS2::ActionViewLayerList, this, this->layerWidget->parentWidget());
    subMenu->addAction(action);
    action = actionFactory.createAction(RS2::ActionViewBlockList, this, this->blockWidget->parentWidget());
    subMenu->addAction(action);
    action = actionFactory.createAction(RS2::ActionViewLibrary, this, this->libraryWidget->parentWidget());
    subMenu->addAction(action);
    action = actionFactory.createAction(RS2::ActionViewCommandLine, this, this->commandWidget->parentWidget());
    subMenu->addAction(action);

    subMenu->addSeparator();

    action = actionFactory.createAction(RS2::ActionViewPenToolbar, this, this->penToolBar);
    subMenu->addAction(action);
    action = actionFactory.createAction(RS2::ActionViewOptionToolbar, this, this->optionWidget);
    subMenu->addAction(action);
    //action = actionFactory.createAction(RS2::ActionViewCadToolbar, this, this->cadToolBar);
    //action->addTo(subMenu); // RVT CadToolbar is not a correct widget yet to beable to get toogled.
    action = actionFactory.createAction(RS2::ActionViewFileToolbar, this, this->fileToolBar);
    subMenu->addAction(action);
    action = actionFactory.createAction(RS2::ActionViewEditToolbar, this, this->editToolBar);
    subMenu->addAction(action);
    action = actionFactory.createAction(RS2::ActionViewSnapToolbar, this, this->snapToolBar);
    subMenu->addAction(action);

    // RVT_PORT menu->insertItem(tr("Vie&ws"), createDockWindowMenu(NoToolBars));
    // RVT_PORT menu->insertItem(tr("Tool&bars"), createDockWindowMenu(OnlyToolBars));


    // tr("Focus on Command Line")
    action = new QAction(tr("Focus on &Command Line"), this);
    action->setIcon(QIcon(":/main/editclear.png"));
    {//added commandline shortcuts, feature request# 3437106
        QList<QKeySequence> commandLineShortcuts;
        commandLineShortcuts<<QKeySequence(Qt::CTRL + Qt::Key_M)<<QKeySequence( Qt::Key_Colon)<<QKeySequence(Qt::Key_Space);
        action->setShortcuts(commandLineShortcuts);
    }
        //action->zetStatusTip(tr("Focus on Command Line"));

    connect(action, SIGNAL(triggered()),
            this, SLOT(slotFocusCommandLine()));
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    //addToolBar(tb, tr("View"));
        addToolBar(Qt::TopToolBarArea, tb); //tr("View");

    // Selecting actions:
    //
    menu = menuBar()->addMenu(tr("&Select"));
    menu->setObjectName("Select");
    action = actionFactory.createAction(RS2::ActionDeselectAll, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectAll, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectSingle, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectContour, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDeselectWindow, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectWindow, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectInvert, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectIntersected,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDeselectIntersected,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectLayer, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Drawing actions:
    //
    menu = menuBar()->addMenu(tr("&Draw"));
    menu->setObjectName("Draw");

    // Points:
//    subMenu= menu->addMenu(tr("&Point"));
//    subMenu->setObjectName("Point");
    action = actionFactory.createAction(RS2::ActionDrawPoint, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Lines:
    subMenu= menu->addMenu(tr("&Line"));
    subMenu->setObjectName("Line");
    action = actionFactory.createAction(RS2::ActionDrawLine,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineAngle,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineHorizontal,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineVertical,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineRectangle,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineParallel,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineParallelThrough,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineBisector,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineTangent1,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineTangent2,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineOrthTan,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineOrthogonal,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineRelAngle,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineFree,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLinePolygonCenCor,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLinePolygonCorCor,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionDrawPolyline,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Arcs:
    subMenu= menu->addMenu(tr("&Arc"));
    subMenu->setObjectName("Arc");
    action = actionFactory.createAction(RS2::ActionDrawArc, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawArc3P, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawArcParallel, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawArcTangential, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Circles:
    subMenu= menu->addMenu(tr("&Circle"));
    subMenu->setObjectName("Circle");
    action = actionFactory.createAction(RS2::ActionDrawCircle, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleCR, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircle2P, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircle3P, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleParallel, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleInscribe, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleTan2, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleTan3, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleTan1_2P, actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    // Ellipses:
    subMenu= menu->addMenu(tr("&Ellipse"));
    subMenu->setObjectName("Ellipse");
    action = actionFactory.createAction(RS2::ActionDrawEllipseAxis,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawEllipseArcAxis,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawEllipseFociPoint,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawEllipse4Points,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawEllipseCenter3Points,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawEllipseInscribe,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Splines:
//    subMenu= menu->addMenu(tr("&Spline"));
//    subMenu->setObjectName("Spline");
    action = actionFactory.createAction(RS2::ActionDrawSpline, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

        // Polylines:
    subMenu= menu->addMenu(tr("&Polyline"));
    subMenu->setObjectName("Polyline");
    action = actionFactory.createAction(RS2::ActionDrawPolyline,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionPolylineAdd,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionPolylineAppend,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionPolylineDel,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionPolylineDelBetween,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionPolylineTrim,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionPolylineEquidistant,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionPolylineSegment,
                                        actionHandler);
    subMenu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Text:
    action = actionFactory.createAction(RS2::ActionDrawMText,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionDrawText,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Hatch:
    action = actionFactory.createAction(RS2::ActionDrawHatch,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    // Image:
    action = actionFactory.createAction(RS2::ActionDrawImage,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Dimensioning actions:
    //
#ifdef __APPLE1__
    QMenu* m = menu;
    menu= m->addMenu(tr("&Dimension"));
#else
    menu = menuBar()->addMenu(tr("&Dimension"));
#endif
    menu->setObjectName("Dimension");
    action = actionFactory.createAction(RS2::ActionDimAligned, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLinear, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLinearHor, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLinearVer, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimRadial, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimDiametric, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimAngular, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLeader, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Modifying actions:
    //
    menu = menuBar()->addMenu(tr("&Modify"));
    menu->setObjectName("Modify");
    action = actionFactory.createAction(RS2::ActionModifyMove,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyRotate,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyScale,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyMirror,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyMoveRotate,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyRotate2,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyTrim,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyTrim2,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyTrimAmount,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyBevel,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyRound,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyCut,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyStretch,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyEntity,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyAttributes,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyDelete,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyDeleteQuick,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyExplodeText,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    //action = actionFactory.createAction(RS2::ActionModifyDeleteFree,
    //                                    actionHandler);
    //action->addTo(menu);
    action = actionFactory.createAction(RS2::ActionBlocksExplode, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Snapping actions:
    //
    menu = menuBar()->addMenu(tr("&Snap"));
    menu->setObjectName("Snap");
    if(snapToolBar!=NULL) {
        auto&& actions = snapToolBar->getActions();
        foreach(QAction* a, actions){
            menu->addAction(a);
            connect(this, SIGNAL(windowsChanged(bool)), a, SLOT(setEnabled(bool)));
        }
    }
    // Info actions:
    //
    menu = menuBar()->addMenu(tr("&Info"));
    menu->setObjectName("Info");
    //action = actionFactory.createAction(RS2::ActionInfoInside,
    //                                    actionHandler);
    //action->addTo(menu);
    action = actionFactory.createAction(RS2::ActionInfoDist,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionInfoDist2,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionInfoAngle,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionInfoTotalLength,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionInfoArea,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Layer actions:
    //
    menu = menuBar()->addMenu(tr("&Layer"));
    menu->setObjectName("Layer");
    action = actionFactory.createAction(RS2::ActionLayersDefreezeAll,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersFreezeAll,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersAdd, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersRemove,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersEdit, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersToggleLock,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersToggleView,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    // Block actions:
    //
    menu = menuBar()->addMenu(tr("&Block"));
    menu->setObjectName("Block");
    action = actionFactory.createAction(RS2::ActionBlocksDefreezeAll,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksFreezeAll,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksToggleView,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksAdd, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksRemove, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksAttributes,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksInsert,
                                        actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksEdit, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksSave, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksCreate, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksExplode, actionHandler);
    menu->addAction(action);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));


    QMainWindow::addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(Qt::TopToolBarArea, penToolBar);

    addToolBar(Qt::TopToolBarArea, optionWidget);


#ifdef RS_SCRIPTING
    // Scripts menu:
    //
    scriptMenu = new QMenu(tr("&Scripts"));
    scriptMenu->setObjectName("Scripts");
    scriptOpenIDE = actionFactory.createAction(RS2::ActionScriptOpenIDE, this);
    scriptOpenIDE->addTo(scriptMenu);
    scriptRun = actionFactory.createAction(RS2::ActionScriptRun, this);
    scriptMenu->addAction(scriptRun);
#else
    scriptMenu = 0;
    scriptOpenIDE = 0;
    scriptRun = 0;
#endif


    // Help menu:
    //
    /*RVT_PORThelpAboutApp = new QAction(tr("About"),
                                                           QC_APP_ICON16), tr("&About %1").arg(QC_APPNAME), 0, this); */
    helpAboutApp = new QAction(QIcon(QC_APP_ICON16), tr("About"), this);

    //helpAboutApp->zetStatusTip(tr("About the application"));
    //helpAboutApp->setWhatsThis(tr("About\n\nAbout the application"));
    connect(helpAboutApp, SIGNAL(triggered()),
            this, SLOT(slotHelpAbout()));

    helpManual = new QAction(QIcon(":/main/contents.png"), tr("&Manual"), this);
    //helpManual->zetStatusTip(tr("Launch the online manual"));
    connect(helpManual, SIGNAL(triggered()),
            this, SLOT(slotHelpManual()));

/* RVT_PORT    testDumpEntities = new QAction("Dump Entities",
                                   "Dump &Entities", 0, this); */
    testDumpEntities = new QAction("Dump Entities", this);
    connect(testDumpEntities, SIGNAL(triggered()),
            this, SLOT(slotTestDumpEntities()));

/* RVT_PORT	testDumpUndo = new QAction("Dump Undo Info",
                                                           "Undo Info", 0, this); */
        testDumpUndo = new QAction("Dump Undo Info", this);
    connect(testDumpUndo, SIGNAL(triggered()),
            this, SLOT(slotTestDumpUndo()));

/* RVT_PORT    testUpdateInserts = new QAction("Update Inserts",
                                    "&Update Inserts", 0, this); */
    testUpdateInserts = new QAction("Update Inserts", this);
    connect(testUpdateInserts, SIGNAL(triggered()),
            this, SLOT(slotTestUpdateInserts()));

/* RVT_PORT    testDrawFreehand = new QAction("Draw Freehand",
         "Draw Freehand", 0, this); */
         testDrawFreehand = new QAction("Draw Freehand", this);
    connect(testDrawFreehand, SIGNAL(triggered()),
            this, SLOT(slotTestDrawFreehand()));

/* RVT_PORT    testInsertBlock = new QAction("Insert Block",
                                  "Insert Block", 0, this); */
    testInsertBlock = new QAction("Insert Block", this);

    connect(testInsertBlock, SIGNAL(triggered()),
            this, SLOT(slotTestInsertBlock()));

/* RVT_PORT    testInsertText = new QAction("Insert Text",
                                 "Insert Text", 0, this); */
    testInsertMText = new QAction("Insert MText", this);
    connect(testInsertMText, SIGNAL(triggered()),
            this, SLOT(slotTestInsertMText()));
    testInsertText = new QAction("Insert Text", this);
    connect(testInsertText, SIGNAL(triggered()),
            this, SLOT(slotTestInsertText()));

/* RVT_PORT    testInsertImage = new QAction("Insert Image",
                                  "Insert Image", 0, this); */
        // "Insert Image",
    testInsertImage = new QAction(tr("Insert Image"), this);
    connect(testInsertImage, SIGNAL(triggered()),
            this, SLOT(slotTestInsertImage()));

/* RVT_PORT    testUnicode = new QAction("Unicode",
                              "Unicode", 0, this); */
    testUnicode = new QAction("Unicode", this);
    connect(testUnicode, SIGNAL(triggered()),
            this, SLOT(slotTestUnicode()));

/* RVT_PORT    testInsertEllipse = new QAction("Insert Ellipse",
                                    "Insert Ellipse", 0, this); */
    testInsertEllipse = new QAction("Insert Ellipse", this);
    connect(testInsertEllipse, SIGNAL(triggered()),
            this, SLOT(slotTestInsertEllipse()));

/*  RVT_PORT  testMath01 = new QAction("Math01",
                             "Math01", 0, this); */
    testMath01 = new QAction("Math01", this);
    connect(testMath01, SIGNAL(triggered()),
            this, SLOT(slotTestMath01()));

/* RVT_PORT    testResize640 = new QAction("Resize to 640x480",
                                "Resize 1", 0, this); */
    testResize640 = new QAction("Resize to 640x480", this);
    connect(testResize640, SIGNAL(triggered()),
            this, SLOT(slotTestResize640()));

/* RVT_PORT    testResize800 = new QAction("Resize to 800x600",
                                "Resize 2", 0, this); */
    testResize800 = new QAction("Resize to 800x600", this);
    connect(testResize800, SIGNAL(triggered()),
            this, SLOT(slotTestResize800()));

/* RVT_PORT    testResize1024 = new QAction("Resize to 1024x768",
                                 "Resize 3", 0, this); */
    testResize1024 = new QAction("Resize to 1024x768", this);
    connect(testResize1024, SIGNAL(triggered()),
            this, SLOT(slotTestResize1024()));

}

void QC_ApplicationWindow::setPreviousZoomEnable(bool enable){
    previousZoomEnable=enable;
    if(previousZoom != NULL){
        previousZoom->setEnabled(enable);
    }
}


void QC_ApplicationWindow::setUndoEnable(bool enable){
    undoEnable=enable;
    if(undoButton != NULL){
        undoButton->setEnabled(enable);
    }
}

void QC_ApplicationWindow::setRedoEnable(bool enable){
    redoEnable=enable;
    if(redoButton != NULL){
        redoButton->setEnabled(enable);
    }
}


void QC_ApplicationWindow::slotEnableActions(bool enable) {
    if(previousZoom != NULL){
        previousZoom->setEnabled(enable&& previousZoomEnable);
        undoButton->setEnabled(enable&& undoEnable);
        redoButton->setEnabled(enable&& redoEnable);
    }
}

/**
 * Initializes the menu bar.
 */
void QC_ApplicationWindow::initMenuBar() {
    RS_DEBUG->print("QC_ApplicationWindow::initMenuBar()");

    // menuBar entry scriptMenu
#ifdef RS_SCRIPTING
    menuBar()->addMenu(scriptMenu);
#endif
    //scriptOpenIDE->addTo(scriptMenu);
    //scriptRun->addTo(scriptMenu);
    //connect(scriptMenu, SIGNAL(aboutToShow()),
    //        this, SLOT(slotScriptMenuAboutToShow()));

    // menuBar entry windowsMenu
    windowsMenu = menuBar()->addMenu(tr("&Window"));
    windowsMenu->setObjectName("Window");
    connect(windowsMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotWindowsMenuAboutToShow()));

    menuBar()->addSeparator();
    // menuBar entry helpMenu
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->setObjectName("Help");
    helpMenu->addAction(helpManual);
    helpMenu->addSeparator();
    helpMenu->addAction(helpAboutApp);

    // menuBar entry test menu
    if (QC_DEBUGGING) {
        testMenu = menuBar()->addMenu(tr("De&bugging"));
        testMenu->setObjectName("Debugging");
        testMenu->addAction(testDumpEntities);
        testMenu->addAction(testDumpUndo);
        testMenu->addAction(testUpdateInserts);
        testMenu->addAction(testDrawFreehand);
        testMenu->addAction(testInsertBlock);
        testMenu->addAction(testInsertText);
        testMenu->addAction(testInsertImage);
        testMenu->addAction(testInsertEllipse);
        testMenu->addAction(testUnicode);
        testMenu->addAction(testMath01);
        testMenu->addAction(testResize640);
        testMenu->addAction(testResize800);
        testMenu->addAction(testResize1024);
    }

    // menuBar configuration
    recentFiles = new QG_RecentFiles(9);
    openedFiles.clear();
}



/**
 * Initializes the tool bars (file tool bar and pen tool bar).
 */
void QC_ApplicationWindow::initToolBar() {
    RS_DEBUG->print("QC_ApplicationWindow::initToolBar()");


        QSizePolicy toolBarPolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);

        fileToolBar = new QToolBar( "File Operations", this);
        fileToolBar->setSizePolicy(toolBarPolicy);
        fileToolBar->setObjectName ( "FileTB" );

    editToolBar = new QToolBar( "Edit Operations", this);
        editToolBar->setSizePolicy(toolBarPolicy);
        editToolBar->setObjectName ( "EditTB" );
    zoomToolBar = new QToolBar( "Zoom Operations", this);

        zoomToolBar->setSizePolicy(toolBarPolicy);
        zoomToolBar->setObjectName ( "ZoomTB" );

        penToolBar = new QG_PenToolBar("Pen Selection", this);
        penToolBar->setSizePolicy(toolBarPolicy);
        penToolBar->setObjectName ( "PenTB" );

    connect(penToolBar, SIGNAL(penChanged(RS_Pen)),
            this, SLOT(slotPenChanged(RS_Pen)));

    //Add snap toolbar
    snapToolBar = new QG_SnapToolBar("Snap Selection",actionHandler, this);
    snapToolBar->setSizePolicy(toolBarPolicy);
    snapToolBar->setObjectName ( "SnapTB" );

    connect(this, SIGNAL(windowsChanged(bool)), snapToolBar, SLOT(setEnabled(bool)));
    //connect(snapToolBar, SIGNAL(snapsChanged(RS_SnapMode)),
    //        this, SLOT(slotSnapsChanged(RS_SnapMode)));
    this->addToolBar(snapToolBar);


    optionWidget = new QToolBar("Tool Options", this);
        QSizePolicy optionWidgetBarPolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
//        optionWidget->setMinimumSize(440,30);
        optionWidget->setSizePolicy(optionWidgetBarPolicy);
        optionWidget->setObjectName ( "ToolTB" );

    //optionWidget->setFixedExtentHeight(26);
    //optionWidget->setHorizontallyStretchable(true);
    //addDockWindow(optionWidget, DockTop, true);

    // CAD toolbar left:
    QToolBar* t = new QToolBar("CAD Tools", this);

    t->setMinimumSize(66,400);
        QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
        t->setSizePolicy(policy);
        t->setObjectName ( "CADTB" );
    t->setFixedWidth(66);
    t->setFloatable(false);
    t->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea);
   // t->setVerticallyStretchable(true);
        addToolBar(Qt::LeftToolBarArea, t);

    cadToolBar = new QG_CadToolBar(t, "CAD Tools");
    cadToolBar->createSubToolBars(actionHandler);

    connect(cadToolBar, SIGNAL(signalBack()),
            this, SLOT(slotBack()));
    connect(this, SIGNAL(windowsChanged(bool)),
            cadToolBar, SLOT(setEnabled(bool)));

    //QG_CadToolBarMain* cadToolBarMain =
    //new QG_CadToolBarMain(cadToolBar);
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
}



/**
 * Initializes the global application settings from the
 * config file (unix, mac) or registry (windows).
 */
void QC_ApplicationWindow::initSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::initSettings()");

    //RS_Settings settings(QC_REGISTRY, QC_APPKEY);

    RS_SETTINGS->beginGroup("/RecentFiles");
    for (int i=0; i<recentFiles->getNumber(); ++i) {
        QString filename = RS_SETTINGS->readEntry(QString("/File") +
                           QString::number(i+1));
        if (!filename.isEmpty()) {
            recentFiles->add(filename);
        }
    }
    RS_SETTINGS->endGroup();
//    QList <QAction*> recentFilesAction;

    for (int i = 0; i < recentFiles->getNumber(); ++i) {
        recentFilesAction.insert(i, new QAction(this));
        recentFilesAction[i]->setVisible(false);
        connect(recentFilesAction[i], SIGNAL(triggered()),
                this, SLOT(slotFileOpenRecent()));
        fileMenu->addAction(recentFilesAction[i]);
    }
    if (recentFiles->count()>0) {
        updateRecentFilesMenu();
    }

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

    RS_SETTINGS->beginGroup("/RecentFiles");
    for (int i=0; i<recentFiles->count(); ++i) {
        RS_SETTINGS->writeEntry(QString("/File") +
                                QString::number(i+1), recentFiles->get(i));
    }
    RS_SETTINGS->endGroup();

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
    layerWidget = NULL;
    blockWidget = NULL;
    libraryWidget = NULL;
    commandWidget = NULL;



    RS_DEBUG->print("  layer widget..");
    dw = new QDockWidget( "Layer", this);
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
 * Creates a new toolbar.
 * Implementation from QG_MainWindowInterface.
 * Can be called from scripts to add individual GUI elements.
 */
/*QToolBar* QC_ApplicationWindow::createToolBar(const QString& name) {
    QToolBar* tb = new QToolBar(name, this);
        tb->setLabel(name);
        return tb;
}*/



/**
 * Creates a new button in the given tool bar for running a script.
 */
/*void QC_ApplicationWindow::addToolBarButton(QToolBar* tb) {
        if (tb!=NULL) {
        QAction* action = new QAction("Blah",
                        QIcon(":/actions/zoomwindow.png"),
            "&Blah", QKeySequence(), NULL);
        //action->zetStatusTip("Blah blah");
                action->addTo(tb);
        }
}*/



/**
 * Updates the recent file list in the file menu.
 */
void QC_ApplicationWindow::updateRecentFilesMenu() {
    RS_DEBUG->print("QC_ApplicationWindow::updateRecentFilesMenu()");

    RS_DEBUG->print("Updating recent file menu...");
    int numRecentFiles = qMin(recentFiles->count(), recentFiles->getNumber());

    for (int i = 0; i < numRecentFiles; ++i) {
        //oldest on top
//        QString text = tr("&%1 %2").arg(i + 1).arg(recentFiles->get(i));
        //newest on top
        QString text = tr("&%1 %2").arg(i + 1).arg(recentFiles->get(numRecentFiles-i-1));
        recentFilesAction[i]->setText(text);
        //newest on top
        recentFilesAction[i]->setData(recentFiles->get(numRecentFiles-i-1));
        recentFilesAction[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < recentFiles->getNumber(); ++j)
        recentFilesAction[j]->setVisible(false);

/*    for (int i=0; i<recentFiles->getNumber(); ++i) {
        QString label = QString( "&%1 %2" ).
                        arg(i+1).arg(recentFiles->get(i));

        if (fileMenu->findItem(i)) {
            RS_DEBUG->print("Changeing item %d", i);
            fileMenu->changeItem(i, label);
        } else if (i < int(recentFiles->count())) {
            RS_DEBUG->print("Adding item %d", i);
            fileMenu->insertItem(label,
                                 this, SLOT(slotFileOpenRecent(int)),
                                 0, i);
        }
    }*/
}



/**
 * Goes back to the previous menu or one step in the current action.
 */
void QC_ApplicationWindow::slotBack() {
    RS_GraphicView* graphicView = getGraphicView();
    if (graphicView!=NULL) {
        graphicView->back();
    } else {
        if (cadToolBar!=NULL) {
            cadToolBar->showToolBar(RS2::ToolBarMain);
        }
    }
}

void QC_ApplicationWindow::slotKillAllActions() {
    RS_GraphicView* gv = getGraphicView();
    QC_MDIWindow* m = getMDIWindow();
    if (gv!=NULL && m!=NULL && m->getDocument()!=NULL) {
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

    if (commandWidget==NULL || !commandWidget->checkFocus()) {
        if (cadToolBar!=NULL) {
            cadToolBar->forceNext();
        } else {
            RS_GraphicView* graphicView = getGraphicView();
            if (graphicView!=NULL) {
                graphicView->enter();
            }
        }
    }
}



/**
 * Sets the keyboard focus on the command line.
 */
void QC_ApplicationWindow::slotFocusCommandLine() {
    if (commandWidget->isVisible()) {
        commandWidget->setFocus();
    }
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
    //QG_GraphicView* graphicView = getGraphicView();
    /*if (graphicView!=NULL) {
        graphicView->setFocus();
}
    else {*/
    setFocus();
    //}
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
        if(w0!=NULL) slotWindowActivated(w0);
        return;
    }
    if(w==activedMdiSubWindow) return;
    activedMdiSubWindow=w;
    QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(w->widget());

//    QList<QMdiSubWindow*> windows=mdiAreaCAD->subWindowList();
//    int activeIndex=windows.indexOf(w);
//    std::cout<<"QC_ApplicationWindow::slotWindowActivated(QMdiSubWindow* w): activated "<< activeIndex <<std::endl;

    if (m!=NULL && m->getDocument()!=NULL) {

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
        if(snapToolBar != NULL ){
            actionHandler->slotSetSnaps(snapToolBar->getSnaps());
        }else {
            RS_DEBUG->print(RS_Debug::D_ERROR,"snapToolBar is NULL\n");
        }

        // set pen from pen toolbar
        slotPenChanged(penToolBar->getPen());

        // update toggle button status:
        if (m->getGraphic()!=NULL) {
            emit(gridChanged(m->getGraphic()->isGridOn()));
        }
        if (m->getGraphicView()!=NULL) {
//            std::cout<<"QC_ApplicationWindow::slotWindowActivated(): emit(printPreviewChanged("<<m->getGraphicView()->isPrintPreview()<<")"<<std::endl;

            emit(printPreviewChanged(m->getGraphicView()->isPrintPreview()));
        }
    }

    // Disable/Enable menu and toolbar items
    emit windowsChanged(m!=NULL && m->getDocument()!=NULL);
//    emit windowsChanged(true);
    RS_DEBUG->print("RVT_PORT emit windowsChanged(true);");

    RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated end");
}



/**
 * Called when the menu 'windows' is about to be shown.
 * This is used to update the window list in the menu.
 */
void QC_ApplicationWindow::slotWindowsMenuAboutToShow() {

    RS_DEBUG->print("QC_ApplicationWindow::slotWindowsMenuAboutToShow");

    windowsMenu->clear();
    //    while( windowsMenu.size() > 0 ){
//            delete windowsMenu->takeFirst();
//    }

    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    for (int i=0; i<windows.size(); ) {
        //clean up invalid sub-windows
        //fixme, this should be auto, by
        //setAttribute(Qt::WA_DeleteOnClose);

        if(windows.at(i) != NULL && windows.at(i)->widget() != NULL){
            i++;
        }else{
            mdiAreaCAD->removeSubWindow(windows.at(i));
            windows = mdiAreaCAD->subWindowList();
            if(windows.size() > 0){
                QMdiSubWindow* active= mdiAreaCAD->currentSubWindow();
                if(active != NULL) {
                   mdiAreaCAD->setActiveSubWindow(active);
                   active->raise();
                   active->setFocus();
                }

            }
            continue;
        }
    }
    if (mdiAreaCAD->subWindowList().size()>1) {
        if(mdiAreaTab) {
            windowsMenu->addAction(tr("Su&b-Window mode"),
                                             this, SLOT(slotToggleTab()));
        }else{
            windowsMenu->addAction(tr("&Cascade"), this, SLOT(slotCascade()));
//            windowsMenu->addAction(tr("&Tile"), mdiAreaCAD, SLOT(tileSubWindows()));
            windowsMenu->addAction(tr("&Tile"), this, SLOT(slotTile()));
            windowsMenu->addAction(tr("Tile &Vertically"), this, SLOT(slotTileVertical()));
            windowsMenu->addAction(tr("Tile &Horizontally"), this, SLOT(slotTileHorizontal()));
            windowsMenu->addAction(tr("Ta&b mode"), this, SLOT(slotToggleTab()));
        }
    }else{
        if(mdiAreaCAD->subWindowList().size() == 0) return; //no sub-window to show
    }
    windowsMenu->addSeparator();
    QMdiSubWindow* active= mdiAreaCAD->activeSubWindow();
//    int active=windows.indexOf(mdiAreaCAD->activeSubWindow());
//    std::cout<<" QC_ApplicationWindow::slotWindowsMenuAboutToShow(): has active: "<< (mdiAreaCAD->activeSubWindow() != NULL )<<" index="<<active<<std::endl;
//    if(active<0) active=windows.size()-1;
    for (int i=0; i<windows.size(); ++i) {
        QAction *id = windowsMenu->addAction(windows.at(i)->windowTitle(),
                                         this, SLOT(slotWindowsMenuActivated(bool)));
        id->setCheckable(true);
        id->setData(i);
        id->setChecked(windows.at(i)==active);
//    std::cout<<" QC_ApplicationWindow::slotWindowsMenuAboutToShow(): "<<i<<":windows.at(i)->isactiveSubWindow(): "<< windows.at(i)->isactiveSubWindow()<<std::endl;
////    std::cout<<" QC_ApplicationWindow::slotWindowsMenuAboutToShow(): "<<i<<":windows.at(i)->widget()->isactiveSubWindow(): "<< windows.at(i)->widget()->isactiveSubWindow()<<std::endl;
////    std::cout<<" QC_ApplicationWindow::slotWindowsMenuAboutToShow(): "<<i<<":windows.at(i)->hasFocus(): "<< windows.at(i)->hasFocus()<<std::endl;
//    std::cout<<" QC_ApplicationWindow::slotWindowsMenuAboutToShow(): "<<i<<":windows.at(i)->widget()->hasFocus(): "<< windows.at(i)->widget()->hasFocus()<<std::endl;
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
    if (w!=NULL) {
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

            qobject_cast<QC_MDIWindow*>(w->widget())->zoomAuto();
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
        qobject_cast<QC_MDIWindow*>(window->widget())->zoomAuto();
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
            qobject_cast<QC_MDIWindow*>(window->widget())->zoomAuto();
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
         qobject_cast<QC_MDIWindow*>(window->widget())->zoomAuto();
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
         qobject_cast<QC_MDIWindow*>(window->widget())->zoomAuto();
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
            qobject_cast<QC_MDIWindow*>(m->widget())->zoomAuto();
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
    if (m!=NULL) {
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
    if (graphic!=NULL) {
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
    if (cadToolBar!=NULL) {
        //set SnapFree to avoid orphaned snapOptions, bug#3407522
            /* setting snap option toolbar pointers to non-static fixes
             * bug#3407522
            if (snapToolBar != NULL && getGraphicView() != NULL && getDocument() != NULL ) {
                    //need to detect graphicView and Document for NULL
//bug#3408689
                RS_SnapMode s=snapToolBar->getSnaps();
                s.snapMiddle=false;
                s.snapDistance=false;
                snapToolBar->setSnaps(s);
                //cadToolBar->setSnapFree();
            }
            */
        cadToolBar->showToolBar(RS2::ToolBarMain);
        cadToolBar->resetToolBar();
        }

    QG_DIALOGFACTORY->setCadToolBar(cadToolBar);
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
        w->zoomAuto();
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
 * Called when a recently opened file is chosen from the list in the
 * file menu.
 */
void QC_ApplicationWindow::slotFileOpenRecent() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpenRecent()");

    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {

    statusBar()->showMessage(tr("Opening recent file..."));
    QString fileName = action->data().toString();

    slotFileOpen(fileName, RS2::FormatUnknown);
    }
}


/*	*
 *	Function name:
 *
 *	Description:	- Format a string that hold a file name path
 *						  such a way that it can displayed on the
 *						  windows title bar.
 *
 *	Author(s):		Claude Sylvain
 *	Created:			30 July 2011
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

    if (!fileName.isEmpty())
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
        if(old !=NULL) {//save old geometry
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
        if (w->slotFileOpen(fileName, type)==false) {
               // error
               QApplication::restoreOverrideCursor();
               QString msg=tr("Cannot open the file\n%1\nPlease "
                              "check the permissions.")
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
               if( active != NULL ){//restore old geometry
                   mdiAreaCAD->setActiveSubWindow(active);
                   active->raise();
                   active->setFocus();
                   if(old==NULL || maximized){
                       active->showMaximized();
                   }else{
                       active->setGeometry(geo);
                   }
                   qobject_cast<QC_MDIWindow*>(active->widget())->zoomAuto();
               }
               return;
        }

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: 1");

        // update recent files menu:
        recentFiles->add(fileName);
        openedFiles.append(fileName);
        layerWidget->slotUpdateLayerList();

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: 2");
        updateRecentFilesMenu();

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: set caption");


                /*	Format and set caption.
                 *	----------------------- */
        w->setWindowTitle(format_filename_caption(fileName));

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: set caption: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update coordinate widget");
        // update coordinate widget format:
        RS_DIALOGFACTORY->updateCoordinateWidget(RS_Vector(0.0,0.0),
                RS_Vector(0.0,0.0),
                true);
        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update coordinate widget: OK");

        // show output of filter (if any):
        commandWidget->processStderr();
        QString message=tr("Loaded document: ")+fileName;
        commandWidget->appendHistory(message);
        statusBar()->showMessage(message, 2000);

    }
         else
         {
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
    if (w!=NULL) {
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
    if (w!=NULL) {
        bool cancelled;
        if (w->slotFileSaveAs(cancelled)) {
            if (!cancelled) {
                name = w->getDocument()->getFilename();
                recentFiles->add(name);
                w->setWindowTitle(name);
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
    updateRecentFilesMenu();

    QString message = tr("Saved drawing: %1").arg(name);
    statusBar()->showMessage(message, 2000);
    commandWidget->appendHistory(message);
}



/**
 * Autosave.
 */
void QC_ApplicationWindow::slotFileAutoSave() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileAutoSave()");

    statusBar()->showMessage(tr("Auto-saving drawing..."));

    QC_MDIWindow* w = getMDIWindow();
    QString name;
    if (w!=NULL) {
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
    if (w!=NULL) {

        // read default settings:
        RS_SETTINGS->beginGroup("/Paths");
        QString defDir = RS_SETTINGS->readEntry("/ExportImage", RS_SYSTEM->getHomeDir());
        QString defFilter = RS_SETTINGS->readEntry("/ExportImageFilter",
                                                     QString("%1 (%2)(*.%2)").arg(QG_DialogFactory::extToFormat("png")).arg("png"));
        RS_SETTINGS->endGroup();

        bool cancel = false;

        QStringList filters;
        QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();
    #if QT_VERSION >= 0x040300
        supportedImageFormats.append("svg"); // add svg
    #endif
        foreach (QString format, supportedImageFormats) {
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
                filters.append(st);
        }
        // revise list of filters
        filters.removeDuplicates();
        filters.sort();

        // set dialog options: filters, mode, accept, directory, filename
        QFileDialog fileDlg(this, "Export as");
        fileDlg.setFilters(filters);
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
            RS_SETTINGS->beginGroup("/Paths");
            RS_SETTINGS->writeEntry("/ExportImage", QFileInfo(fn).absolutePath());
            RS_SETTINGS->writeEntry("/ExportImageFilter",
                                    fileDlg.selectedFilter());
            RS_SETTINGS->endGroup();

            // find out extension:
            QString filter = fileDlg.selectedFilter();
            QString format = "";
            int i = filter.indexOf("(*.");
            if (i!=-1) {
                int i2 = filter.indexOf(QRegExp("[) ]"), i);
                format = filter.mid(i+3, i2-(i+3));
                format = format.toUpper();
            }

            // append extension to file:
            if (!QFileInfo(fn).fileName().contains(".")) {
                fn.append("." + format.toLower());
            }

            // show options dialog:
            QG_ImageOptionsDialog dlg(this);
            dlg.setGraphicSize(w->getGraphic()->getSize());
            //dlg.setGraphicSize(w->getGraphic()->calculateBorders());
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
 * Exports the drawing as a bitmap.
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
            e!=NULL; e=graphic->nextEntity(RS2::ResolveAll)) {
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
        iio.setFormat(format.toAscii());
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

    if(w!=NULL){
        openedFiles.removeAll(w->getDocument()->getFilename());
        //        int pos=openedFiles.indexOf(w->getDocument()->getFilename());
        //        if(pos>=0) {
        //            openedFiles.erase(openedFiles.begin()+pos);
        //        }

        //properly close print preview if exists
        QC_MDIWindow *ppv = w->getPrintPreview();
        if (ppv!=NULL) {
            mdiAreaCAD->removeSubWindow(ppv->parentWidget());
        }
    }


    mdiAreaCAD->closeActiveSubWindow();
    activedMdiSubWindow=NULL;
    QMdiSubWindow* m=mdiAreaCAD->currentSubWindow();
    if(m!=NULL){
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
}



/**
 * Menu file -> print.
 */
void QC_ApplicationWindow::slotFilePrint() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFilePrint()");

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
    printer.setPaperSize(RS2::rsToQtPaperFormat(graphic->getPaperFormat(&landscape)));
#endif // QT_VERSION 0x040400
    if (landscape) {
        printer.setOrientation(QPrinter::Landscape);
    } else {
        printer.setOrientation(QPrinter::Portrait);
    }

    RS_SETTINGS->beginGroup("/Print");
    printer.setOutputFileName(RS_SETTINGS->readEntry("/FileName", ""));
    printer.setColorMode((QPrinter::ColorMode)RS_SETTINGS->readNumEntry("/ColorMode", (int)QPrinter::Color));
//RLZ: No more needed, if setOutputFileName == "" then setOutputToFile is false
/*    printer.setOutputToFile((bool)RS_SETTINGS->readNumEntry("/PrintToFile",
                             0));*/
    RS_SETTINGS->endGroup();

    // printer setup:
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        //printer.setOutputToFile(true);
        //printer.setOutputFileName(outputFile);

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

//        std::cout<<"printer.resolution()="<<printer.resolution()<<std::endl;
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

        double f = (fx+fy)/2.0;

        double scale = graphic->getPaperScale();

        gv.setOffset((int)(graphic->getPaperInsertionBase().x * f),
                     (int)(graphic->getPaperInsertionBase().y * f));
        gv.setFactor(f*scale);

        gv.setContainer(graphic);
//fixme, I don't understand the meaning of 'true' here
//        gv.drawEntity(&painter, graphic, true);

        gv.drawEntity(&painter, graphic );

        // GraphicView deletes painter
        painter.end();

        RS_SETTINGS->beginGroup("/Print");
        //RLZ: No more needed, if outputFileName == "" then PrintToFile is false
//        RS_SETTINGS->writeEntry("/PrintToFile", (int)printer.outputToFile());
        RS_SETTINGS->writeEntry("/ColorMode", (int)printer.colorMode());
        RS_SETTINGS->writeEntry("/FileName", printer.outputFileName());
        RS_SETTINGS->endGroup();
        QApplication::restoreOverrideCursor();
    }

    statusBar()->showMessage(tr("Printing complete"), 2000);
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
                if(w != NULL){
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
        if (ppv!=NULL) {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): show existing");

            /*
            QList<QMdiSubWindow*> windows=mdiAreaCAD->subWindowList();
            for(int i=0;i<windows.size();i++){
                if( windows.at(i)->widget() == ppv){
                    windows.at(i)->showMaximized();
                    mdiAreaCAD->setActiveSubWindow(windows.at(i));
                    break;
                }
            }*/

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
                w->zoomAuto();
                w->getGraphicView()->setPrintPreview(true);
                w->getGraphicView()->setBackground(RS_Color(255,255,255));
                w->getGraphicView()->setDefaultAction(
                    new RS_ActionPrintPreview(*w->getDocument(), *w->getGraphicView()));

                // only graphics offer block lists, blocks don't
                RS_DEBUG->print("  adding listeners");
                RS_Graphic* graphic = w->getDocument()->getGraphic();
                if (graphic!=NULL) {
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
                QG_DIALOGFACTORY->setCadToolBar(cadToolBar);
                // Link the graphic view to the command widget:
                QG_DIALOGFACTORY->setCommandWidget(commandWidget);

                RS_DEBUG->print("  showing MDI window");

                if (mdiAreaCAD->subWindowList().size() <= 1 ) {
                    w->showMaximized();
                } else {
                    w->show();
                }

                if(graphic!=NULL){
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
    if (m!=NULL) {
        RS_Graphic* g = m->getGraphic();
        if (g!=NULL) {
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

    redrawAll();
}



/**
 * Redraws all mdi windows.
 */
void QC_ApplicationWindow::redrawAll() {
    if (mdiAreaCAD!=NULL) {
        QList<QMdiSubWindow*> windows = mdiAreaCAD->subWindowList();
        for (int i = 0; i < windows.size(); ++i) {
            QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i)->widget());
            if (m!=NULL) {
                QG_GraphicView* gv = m->getGraphicView();
                if (gv!=NULL) {
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
    if (mdiAreaCAD!=NULL) {
        QList<QMdiSubWindow*> windows = mdiAreaCAD->subWindowList();
        for (int i = 0; i < windows.size(); ++i) {
            QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i)->widget());
            if (m!=NULL) {
                QG_GraphicView* gv = m->getGraphicView();
                if (gv!=NULL) {
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
 * Creates a new MDI window for editing the selected block.
 */
/*
void QC_ApplicationWindow::slotBlocksEdit() {
    RS_DEBUG->print("QC_ApplicationWindow::slotBlocksEdit()");

    QC_MDIWindow* parent = getMDIWindow();
    if (parent!=NULL) {
        RS_BlockList* blist = blockWidget->getBlockList();
        if (blist!=NULL) {
            RS_Block* blk = blist->getActiveBlock();
            if (blk!=NULL) {
                QC_MDIWindow* w = slotFileNew(blk);
                // the parent needs a pointer to the block window and
                //   vice versa
                parent->addChildWindow(w);
                w->getGraphicView()->zoomAuto();
            }
        }
    }
} */



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
    QColor highlightedColor(RS_SETTINGS->readEntry("/HighlightedColor",
                            "#739373"));
    RS_SETTINGS->endGroup();

    QList<QMdiSubWindow*> windows = mdiAreaCAD->subWindowList();
    for (int i = 0; i < windows.size(); ++i) {
        QC_MDIWindow* m = qobject_cast<QC_MDIWindow*>(windows.at(i)->widget());
        if (m!=NULL) {
            QG_GraphicView* gv = m->getGraphicView();
            if (gv!=NULL) {
                gv->setBackground(color);
                gv->setGridColor(gridColor);
                gv->setMetaGridColor(metaGridColor);
                gv->setSelectedColor(selectedColor);
                gv->setHighlightedColor(highlightedColor);
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
        if (actionHandler!=NULL) {
            RS_ActionInterface* a =
                actionHandler->setCurrentAction(RS2::ActionLibraryInsert);
            if (a!=NULL) {
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
    foreach (QC_PluginInterface *pluginInterface, loadedPlugins)
        modules.append(pluginInterface->name());

    QString modulesString=tr("None");
    if (modules.empty()==false) {
        modulesString = modules.join(", ");
    }

    QMessageBox box(this);
    box.setWindowTitle(tr("About..."));
    box.setText(       QString("<p><font size=\"2\">") +
                       "<h2>"+ XSTR(QC_APPNAME)+ "</h2>" +
                       tr("Version: %1").arg(XSTR(QC_VERSION)) + "<br>" +
#ifdef QC_SCMREVISION
                       tr("SCM Revision: %1").arg(XSTR(QC_SCMREVISION)) + "<br>" +
#endif
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
            QMessageBox::information(this, "Helpfiles not found", tr("Bugger, I couldn't find the helpfiles on the filesystem."));
        }

    }
    if (helpWindow) {
        helpWindow->show();
    }
#endif // QT_VERSION 0x040400
}

/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestDumpEntities(RS_EntityContainer* d) {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestDumpEntities()");
    static int level = 0;
    std::ofstream dumpFile;

    if (d==NULL) {
        d = getDocument();
        dumpFile.open("debug_entities.html");
        level = 0;
    } else {
        dumpFile.open("debug_entities.html", std::ios::app);
        level++;
    }

    if (d!=NULL) {
        if (level==0) {
            dumpFile << "<html>\n";
            dumpFile << "<body>\n";
        }

        for (RS_Entity* e=d->firstEntity();
                e!=NULL;
                e=d->nextEntity()) {

            dumpFile << "<table border=\"1\">\n";
            dumpFile << "<tr><td>Entity: " << e->getId()
            << "</td></tr>\n";

            dumpFile
            << "<tr><td><table><tr>"
            << "<td>VIS:" << e->isVisible() << "</td>"
            << "<td>UND:" << e->isUndone() << "</td>"
            << "<td>SEL:" << e->isSelected() << "</td>"
            << "<td>TMP:" << e->getFlag(RS2::FlagTemp) << "</td>";
            QString lay = "NULL";
            if (e->getLayer()!=NULL) {
                lay = e->getLayer()->getName();
            }
            dumpFile
            << "<td>Layer: " << lay.toAscii().data() << "</td>"
            << "<td>Width: " << (int)e->getPen(false).getWidth() << "</td>"
            << "<td>Parent: " << e->getParent()->getId() << "</td>"
            << "</tr></table>";

            dumpFile
            << "<tr><td>\n";

            switch (e->rtti()) {
            case RS2::EntityPoint: {
                    RS_Point* p = (RS_Point*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Point:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>"
                    << p->getPos()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityLine: {
                    RS_Line* l = (RS_Line*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Line:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>"
                    << l->getStartpoint()
                    << "</td>"
                    << "<td>"
                    << l->getEndpoint()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityArc: {
                    RS_Arc* a = (RS_Arc*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Arc:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>Center: "
                    << a->getCenter()
                    << "</td>"
                    << "<td>Radius: "
                    << a->getRadius()
                    << "</td>"
                    << "<td>Angle 1: "
                    << a->getAngle1()
                    << "</td>"
                    << "<td>Angle 2: "
                    << a->getAngle2()
                    << "</td>"
                    << "<td>Startpoint: "
                    << a->getStartpoint()
                    << "</td>"
                    << "<td>Endpoint: "
                    << a->getEndpoint()
                    << "</td>"
                    << "<td>reversed: "
                    << (int)a->isReversed()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityCircle: {
                    RS_Circle* c = (RS_Circle*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Circle:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>Center: "
                    << c->getCenter()
                    << "</td>"
                    << "<td>Radius: "
                    << c->getRadius()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityDimAligned: {
                    RS_DimAligned* d = (RS_DimAligned*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Dimension / Aligned:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>"
                    << d->getDefinitionPoint()
                    << "</td>"
                    << "<td>"
                    << d->getExtensionPoint1()
                    << "</td>"
                    << "<td>"
                    << d->getExtensionPoint2()
                    << "</td>"
                    << "<td>Text: "
                    << d->getText().toLatin1().data()
                    << "</td>"
                    << "<td>Label: "
                    << d->getLabel().toLatin1().data()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityDimLinear: {
                    RS_DimLinear* d = (RS_DimLinear*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Dimension / Linear:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>"
                    << d->getDefinitionPoint()
                    << "</td>"
                    << "<td>"
                    << d->getExtensionPoint1()
                    << "</td>"
                    << "<td>"
                    << d->getExtensionPoint2()
                    << "</td>"
                    << "<td>Text: "
                    << d->getText().toAscii().data()
                    << "</td>"
                    << "<td>Label: "
                    << d->getLabel().toAscii().data()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityInsert: {
                    RS_Insert* i = (RS_Insert*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Insert:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>Insertion point:"
                    << i->getInsertionPoint()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityMText: {
                    RS_MText* t = (RS_MText*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Text:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>Text:"
                    << t->getText().toLatin1().data()
                    << "</td>"
                    << "<td>Height:"
                    << t->getHeight()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityText: {
                    RS_Text* t = (RS_Text*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Text:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>Text:"
                    << t->getText().toLatin1().data()
                    << "</td>"
                    << "<td>Height:"
                    << t->getHeight()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            case RS2::EntityHatch: {
                    RS_Hatch* h = (RS_Hatch*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Hatch:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>Pattern:"
                    << h->getPattern().toLatin1().data()
                    << "</td>"
                    << "<td>Scale:"
                    << h->getScale()
                    << "</td>"
                    << "<td>Solid:"
                    << (int)h->isSolid()
                    << "</td>"
                    << "</tr></table>";
                }
                break;

            default:
                dumpFile
                << "<tr><td>"
                << "<b>Unknown Entity: " << e->rtti() << "</b>"
                << "</td></tr>";
                break;
            }

            if (e->isContainer() || e->rtti()==RS2::EntityHatch) {
                RS_EntityContainer* ec = (RS_EntityContainer*)e;
                dumpFile << "<table><tr><td valign=\"top\">&nbsp;&nbsp;&nbsp;&nbsp;Contents:</td><td>\n";
                dumpFile.close();
                slotTestDumpEntities(ec);
                dumpFile.open("debug_entities.html", std::ios::app);
                dumpFile << "</td></tr></table>\n";
            }

            dumpFile
            << "</td></tr>"
            << "</table>\n"
            << "<br><br>";
        }

        if (level==0) {
            dumpFile << "</body>\n";
            dumpFile << "</html>\n";
        } else {
            level--;
        }
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestDumpUndo() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestDumpUndo()");

    RS_Document* d = getDocument();
        if (d!=NULL) {
                std::cout << *(RS_Undo*)d;
                std::cout << std::endl;
        }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestUpdateInserts() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestUpdateInserts()");

    RS_Document* d = getDocument();
    if (d!=NULL) {
        d->updateInserts();
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestDrawFreehand() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestDrawFreehand()");


    //RS_Graphic* g = document->getMarking();
    /*

       RS_ActionDrawLineFree* action =
          new RS_ActionDrawLineFree(*document->getGraphic(),
                                    *graphicView);

       for (int i=0; i<100; ++i) {

           int posx = (random()%600);
           int posy = (random()%400);

           //QMouseEvent rsm1(posx, posy, LEFT);
        QMouseEvent rsm1(QEvent::MouseButtonPress,
                           QPoint(posx,posy),
                           RS2::LeftButton,
                           RS2::LeftButton);
           action->mousePressEvent(&rsm1);

           int speedx = 0;
           int speedy = 0;

           for (int k=0; k<100; ++k) {
               int accx = (random()%40)-20;
               int accy = (random()%40)-20;

               speedx+=accx;
               speedy+=accy;

               posx+=speedx;
               posy+=speedy;

               //QMouseEvent rsm2(posx, posy, LEFT);

            QMouseEvent rsm2(QEvent::MouseMove,
                           QPoint(posx,posy),
                           RS2::LeftButton,
                           RS2::LeftButton);
               action->mouseMoveEvent(&rsm2);
           }

           action->mouseReleaseEvent(NULL);

           slotFileSave();
       }

       delete action;
    */
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestInsertBlock() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestInsertBlock()");

    RS_Document* d = getDocument();
    if (d!=NULL && d->rtti()==RS2::EntityGraphic) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return;
        }

        graphic->addLayer(new RS_Layer("default"));
        RS_Block* block = new RS_Block(graphic, RS_BlockData("debugblock",
                                       RS_Vector(0.0,0.0), true));

        RS_Line* line;
        RS_Arc* arc;
        RS_Circle* circle;

        // Add one red line:
        line = new RS_Line(block,
                           RS_LineData(RS_Vector(0.0,0.0),
                                       RS_Vector(50.0,0.0)));
        line->setLayerToActive();
        line->setPen(RS_Pen(RS_Color(255, 0, 0),
                            RS2::Width01,
                            RS2::SolidLine));
        block->addEntity(line);

        // Add one line with attributes from block:
        line = new RS_Line(block,
                           RS_LineData(RS_Vector(50.0,0.0),
                                       RS_Vector(50.0,50.0)));
        line->setPen(RS_Pen(RS_Color(RS2::FlagByBlock),
                            RS2::WidthByBlock,
                            RS2::LineByBlock));
        block->addEntity(line);

        // Add one arc with attributes from block:
        RS_ArcData d(RS_Vector(50.0,0.0),
                     50.0, M_PI/2.0, M_PI,
                     false);
        arc = new RS_Arc(block, d);
        arc->setPen(RS_Pen(RS_Color(RS2::FlagByBlock),
                           RS2::WidthByBlock,
                           RS2::LineByBlock));
        block->addEntity(arc);

        // Add one blue circle:
        RS_CircleData circleData(RS_Vector(20.0,15.0),
                                 12.5);
        circle = new RS_Circle(block, circleData);
        circle->setLayerToActive();
        circle->setPen(RS_Pen(RS_Color(0, 0, 255),
                              RS2::Width01,
                              RS2::SolidLine));
        block->addEntity(circle);


        graphic->addBlock(block);



        RS_Insert* ins;
        RS_InsertData insData("debugblock",
                              RS_Vector(0.0,0.0),
                              RS_Vector(1.0,1.0), 0.0,
                              1, 1, RS_Vector(0.0, 0.0),
                              NULL, RS2::NoUpdate);

        // insert one magenta instance of the block (original):
        ins = new RS_Insert(graphic, insData);
        ins->setLayerToActive();
        ins->setPen(RS_Pen(RS_Color(255, 0, 255),
                           RS2::Width02,
                           RS2::SolidLine));
        ins->update();
        graphic->addEntity(ins);

        // insert one green instance of the block (rotate):
        insData = RS_InsertData("debugblock",
                                RS_Vector(-50.0,20.0),
                                RS_Vector(1.0,1.0), 30.0/ARAD,
                                1, 1, RS_Vector(0.0, 0.0),
                                NULL, RS2::NoUpdate);
        ins = new RS_Insert(graphic, insData);
        ins->setLayerToActive();
        ins->setPen(RS_Pen(RS_Color(0, 255, 0),
                           RS2::Width02,
                           RS2::SolidLine));
        ins->update();
        graphic->addEntity(ins);

        // insert one cyan instance of the block (move):
        insData = RS_InsertData("debugblock",
                                RS_Vector(10.0,20.0),
                                RS_Vector(1.0,1.0), 0.0,
                                1, 1, RS_Vector(0.0, 0.0),
                                NULL, RS2::NoUpdate);
        ins = new RS_Insert(graphic, insData);
        ins->setLayerToActive();
        ins->setPen(RS_Pen(RS_Color(0, 255, 255),
                           RS2::Width02,
                           RS2::SolidLine));
        ins->update();
        graphic->addEntity(ins);

        // insert one blue instance of the block:
        for (double a=0.0; a<360.0; a+=45.0) {
            insData = RS_InsertData("debugblock",
                                    RS_Vector(60.0,0.0),
                                    RS_Vector(2.0/5,2.0/5), a/ARAD,
                                    1, 1, RS_Vector(0.0, 0.0),
                                    NULL, RS2::NoUpdate);
            ins = new RS_Insert(graphic, insData);
            ins->setLayerToActive();
            ins->setPen(RS_Pen(RS_Color(0, 0, 255),
                               RS2::Width05,
                               RS2::SolidLine));
            ins->update();
            graphic->addEntity(ins);
        }

        // insert an array of yellow instances of the block:
        insData = RS_InsertData("debugblock",
                                RS_Vector(-100.0,-100.0),
                                RS_Vector(0.2,0.2), M_PI/6.0,
                                6, 4, RS_Vector(100.0, 100.0),
                                NULL, RS2::NoUpdate);
        ins = new RS_Insert(graphic, insData);
        ins->setLayerToActive();
        ins->setPen(RS_Pen(RS_Color(255, 255, 0),
                           RS2::Width01,
                           RS2::SolidLine));
        ins->update();
        graphic->addEntity(ins);


        RS_GraphicView* v = getGraphicView();
        if (v!=NULL) {
            v->redraw();
        }
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestInsertEllipse() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestInsertEllipse()");


    RS_Document* d = getDocument();
    if (d!=NULL) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return;
        }

        RS_Ellipse* ellipse;
        RS_Line* line;

        for (double a=0.0; a<2*M_PI; a+=0.1) {
            RS_Vector v;
            v.setPolar(50.0, a);
            double xp = 1000.0*a;

            RS_EllipseData ellipseData(RS_Vector(xp,0.0),
                                       v,
                                       0.5,
                                       0.0, 2*M_PI,
                                       false);
            ellipse = new RS_Ellipse(graphic, ellipseData);

            ellipse->setPen(RS_Pen(RS_Color(255, 0, 255),
                                   RS2::Width01,
                                   RS2::SolidLine));

            graphic->addEntity(ellipse);
            //graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
            //graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));

            line = new RS_Line(graphic,
                               RS_LineData(RS_Vector(xp,0.0),
                                           RS_Vector(xp,0.0)+v));
            line->setPen(RS_Pen(RS_Color(128, 128, 128),
                                RS2::Width01,
                                RS2::SolidLine));
            graphic->addEntity(line);


            /*
                     for (double mx=-60.0; mx<60.0; mx+=1.0) {
                         //for (double mx=0.0; mx<1.0; mx+=2.5) {
                         RS_VectorSolutions sol = ellipse->mapX(xp + mx);
                         //graphic->addEntity(new RS_Point(graphic,
                         //                   sol.vector2 + RS_Vector(a*500.0, 0.0)));
                         //graphic->addEntity(new RS_Point(graphic,
                         //                   sol.vector3 + RS_Vector(a*500.0, 0.0)));
                         //graphic->addEntity(new RS_Point(graphic,
                         //                   sol.vector4 + RS_Vector(a*500.0, 0.0)));

                         line = new RS_Line(graphic,
                                            RS_LineData(RS_Vector(xp+mx,-50.0),
                                                        RS_Vector(xp+mx,50.0)));
                         line->setPen(RS_Pen(RS_Color(60, 60, 60),
                                             RS2::Width01,
                                             RS2::SolidLine));
                         graphic->addEntity(line);

                         graphic->addEntity(new RS_Point(graphic,
                                                         sol.get(0)));
                     }
            */
        }


        // different minor/minor relations
        /*
              double x, y;
              for (y=-250.0; y<=250.0; y+=50.0) {
                  for (x=-250.0; x<=250.0; x+=50.0) {
                      RS_Vector v(x, y);

                      ellipse = new RS_Ellipse(graphic,
                                               v,
                                               RS_Vector((x/5+50.0)/2.0, 0.0),
                                         fabs(x/y),
                                               0.0, 2*M_PI,
                                               false);

                ellipse->setPen(RS_Pen(RS_Color(255, 255, 0),
                                       RS2::Width01,
                                       RS2::DashDotLine));

                      graphic->addEntity(ellipse);
                      graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
                      graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));

                ellipse = new RS_Ellipse(graphic,
                                               v + RS_Vector(750.0, 0.0),
                                               RS_Vector((x/5+50.0)/2.0, 0.0),
                                               fabs(x/y),
                                               2*M_PI, 0.0,
                                               true);

                      graphic->addEntity(ellipse);
                      graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
                      graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));
                  }
              }
        */

        /*
              // different rotation angles:
              double rot;
              for (rot=0.0; rot<=2*M_PI+0.1; rot+=(M_PI/8)) {
                  ellipse = new RS_Ellipse(graphic,
                                           RS_Vector(rot*200, 500.0),
                                           RS_Vector(50.0, 0.0).rotate(rot),
                                           0.3,
                                           0.0, 2*M_PI,
                                           false);
                  graphic->addEntity(ellipse);
                  graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
                  graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));
              }


              // different arc angles:
              double a1, a2;
              for (rot=0.0; rot<=2*M_PI+0.1; rot+=(M_PI/8)) {
                  for (a1=0.0; a1<=2*M_PI+0.1; a1+=(M_PI/8)) {
                      for (a2=a1+M_PI/8; a2<=2*M_PI+a1+0.1; a2+=(M_PI/8)) {
                          ellipse = new RS_Ellipse(graphic,
                                                   RS_Vector(-500.0-a1*200.0-5000.0*rot,
                                                             500.0-a2*200.0),
                                                   RS_Vector(50.0, 0.0).rotate(rot),
                                                   0.3,
                                                   a1, a2,
                                                   false);
                          graphic->addEntity(ellipse);
                          graphic->addEntity(new RS_Point(graphic, ellipse->getMax()));
                          graphic->addEntity(new RS_Point(graphic, ellipse->getMin()));
                      }
                  }
              }
        */

        RS_GraphicView* v = getGraphicView();
        if (v!=NULL) {
            v->redraw();
        }
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestInsertMText() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestInsertMText()");


    RS_Document* d = getDocument();
    if (d!=NULL) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return;
        }

        RS_MText* text;
        RS_MTextData textData;

        textData = RS_MTextData(RS_Vector(10.0,10.0),
                               10.0, 100.0,
                               RS2::VAlignTop,
                               RS2::HAlignLeft,
                               RS2::LeftToRight,
                               RS2::Exact,
                               1.0,
                               "Andrew",
                               "normal",
                               0.0);
        text = new RS_MText(graphic, textData);

        text->setLayerToActive();
        text->setPen(RS_Pen(RS_Color(255, 0, 0),
                            RS2::Width01,
                            RS2::SolidLine));
        graphic->addEntity(text);
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestInsertText() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestInsertMText()");


    RS_Document* d = getDocument();
    if (d!=NULL) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return;
        }

        RS_Text* text;
        RS_TextData textData;

        textData = RS_TextData(RS_Vector(10.0,10.0),
                               10.0, 100.0,
                               RS2::VAlignTop,
                               RS2::HAlignLeft,
                               RS2::LeftToRight,
                               RS2::Exact,
                               1.0,
                               "Andrew",
                               "normal",
                               0.0);
        text = new RS_Text(graphic, textData);

        text->setLayerToActive();
        text->setPen(RS_Pen(RS_Color(255, 0, 0),
                            RS2::Width01,
                            RS2::SolidLine));
        graphic->addEntity(text);

        /*
              double x, y;
              for (y=-250.0; y<=250.0; y+=50.0) {
                  for (x=-250.0; x<=250.0; x+=50.0) {
                      RS_Vector v(x, y);

                      textData = RS_TextData(v,
                                             10.0, 100.0,
                                             RS2::VAlignTop,
                                             RS2::HAlignLeft,
                                             RS2::LeftToRight,
                                             RS2::Exact,
                                             1.0,
                                             "Andrew",
                                             "normal",
                                             0.0);

                      text = new RS_Text(graphic, textData);

                      text->setLayerToActive();
                      text->setPen(RS_Pen(RS_Color(255, 0, 0),
                                          RS2::Width01,
                                          RS2::SolidLine));
                      graphic->addEntity(text);
                  }
              }

              RS_Line* line;
              for (x=0.0; x<M_PI*2.0; x+=0.2) {
                  RS_Vector v(600.0+cos(x)*50.0, 0.0+sin(x)*50.0);

                  line = new RS_Line(graphic,
                                     RS_LineData(RS_Vector(600.0,0.0),
                                                 v));
                  line->setLayerToActive();
                  line->setPenToActive();
                  graphic->addEntity(line);

                  textData = RS_TextData(v,
                                         5.0, 50.0,
                                         RS2::VAlignTop,
                                         RS2::HAlignLeft,
                                         RS2::LeftToRight,
                                         RS2::Exact,
                                         1.0,
                                         "Andrew",
                                         "normal",
                                         x);

                  text = new RS_Text(graphic, textData);

                  text->setLayerToActive();
                  text->setPen(RS_Pen(RS_Color(255, 0, 0),
                                      RS2::Width01,
                                      RS2::SolidLine));
                  graphic->addEntity(text);
              }

              RS_SolidData solidData = RS_SolidData(RS_Vector(5.0, 10.0),
                                                    RS_Vector(25.0, 15.0),
                                                    RS_Vector(15.0, 30.0));

              RS_Solid* s = new RS_Solid(graphic, solidData);

              s->setLayerToActive();
              s->setPen(RS_Pen(RS_Color(255, 255, 0),
                               RS2::Width01,
                               RS2::SolidLine));
              graphic->addEntity(s);

              RS_GraphicView* v = getGraphicView();
              if (v!=NULL) {
                  v->redraw();
              }
        */
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestInsertImage() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestInsertImage()");


    RS_Document* d = getDocument();
    if (d!=NULL) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return;
        }

        RS_Image* image;
        RS_ImageData imageData;

        imageData = RS_ImageData(0, RS_Vector(50.0,30.0),
                                 RS_Vector(0.5,0.5),
                                 RS_Vector(-0.5,0.5),
                                 RS_Vector(640,480),
                                 "/home/andrew/data/image.png",
                                 50, 50, 0);
        image = new RS_Image(graphic, imageData);

        image->setLayerToActive();
        image->setPen(RS_Pen(RS_Color(255, 0, 0),
                             RS2::Width01,
                             RS2::SolidLine));
        graphic->addEntity(image);
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestUnicode() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestUnicode()");

    slotFileOpen("./fonts/unicode.cxf", RS2::FormatCXF);
    RS_Document* d = getDocument();
    if (d!=NULL) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return;
        }

        RS_Insert* ins;

        int col;
        int row;
        QChar uCode;       // e.g. 65 (or 'A')
        QString strCode;   // unicde as string e.g. '[0041] A'

        graphic->setAutoUpdateBorders(false);

        for (col=0x0000; col<=0xFFF0; col+=0x10) {
            printf("col: %X\n", col);
            for (row=0x0; row<=0xF; row++) {
                //printf("  row: %X\n", row);

                uCode = QChar(col+row);
                //printf("  code: %X\n", uCode.unicode());

                strCode.setNum(uCode.unicode(), 16);
                while (strCode.length()<4) {
                    strCode="0"+strCode;
                }
                strCode = "[" + strCode + "] " + uCode;

                if (graphic->findBlock(strCode)!=NULL) {
                    RS_InsertData d(strCode,
                                    RS_Vector(col/0x10*20.0,row*20.0),
                                    RS_Vector(1.0,1.0), 0.0,
                                    1, 1, RS_Vector(0.0, 0.0),
                                    NULL, RS2::NoUpdate);
                    ins = new RS_Insert(graphic, d);
                    ins->setLayerToActive();
                    ins->setPen(RS_Pen(RS_Color(255, 255, 255),
                                       RS2::Width01,
                                       RS2::SolidLine));
                    ins->update();
                    graphic->addEntity(ins);
                }
            }
        }
        graphic->setAutoUpdateBorders(true);
        graphic->calculateBorders();
    }
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestMath01() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestMath01()");

    RS_Document* d = getDocument();
    if (d!=NULL) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return;
        }

        // axis
        graphic->addEntity(new RS_Line(graphic,
                                       RS_LineData(RS_Vector(0.0,0.0),
                                                   RS_Vector(2*M_PI,0.0))));
        graphic->addEntity(new RS_Line(graphic,
                                       RS_LineData(RS_Vector(0.0,-1.0),
                                                   RS_Vector(0.0,1.0))));

        // cos
        double a;
        double x = 59.0/ARAD;
        double x_0 = 60.0/ARAD;
        for (a=0.01; a<2*M_PI; a+=0.01) {
            // cos curve:
            RS_Line* line = new RS_Line(graphic,
                                        RS_LineData(RS_Vector(a-0.01, cos(a-0.01)),
                                                    RS_Vector(a, cos(a))));
            graphic->addEntity(line);

            // tangent:
            graphic->addEntity(new RS_Line(graphic,
                                           RS_LineData(RS_Vector(a-0.01,cos(x_0)-sin(x_0)*(a-0.01-x_0)),
                                                       RS_Vector(a,cos(x_0)-sin(x_0)*(a-x_0)))));
        }

        // 59.0 deg
        graphic->addEntity(new RS_Line(graphic,
                                       RS_LineData(RS_Vector(x,0.0),
                                                   RS_Vector(x,1.0))));

        // 60.0 deg
        graphic->addEntity(new RS_Line(graphic,
                                       RS_LineData(RS_Vector(x_0,0.0),
                                                   RS_Vector(x_0,1.0))));

        // tangent
        //graphic->addEntity(new RS_Line(graphic,
        //                   RS_Vector(0.0,cos(x_0)-sin(x_0)*(0.0-x_0)),
        //                   RS_Vector(6.0,cos(x_0)-sin(x_0)*(6.0-x_0))));


        RS_GraphicView* v = getGraphicView();
        if (v!=NULL) {
            v->redraw();
        }
    }
}




/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestResize640() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestResize640()");

    resize(640, 480);
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestResize800() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestResize800()");

    resize(800, 600);
}



/**
 * Testing function.
 */
void QC_ApplicationWindow::slotTestResize1024() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestResize1024()");

    resize(1024, 768);
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
             if( tmp != NULL){
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
    QTime now = QTime::currentTime();
    bool actionProcessed=false;
    doubleCharacters << e->key();
    if (doubleCharacters.size()>2)
        doubleCharacters=doubleCharacters.mid(doubleCharacters.size()-2,2);
    if (ts.msecsTo(now)<2000) {

        QString code="";
        QList<int>::iterator i;
        for (i = doubleCharacters.begin(); i != doubleCharacters.end(); ++i)
             code += QChar(*i);

        // Check against double keycode handler
        if (actionHandler->keycode(code)==true) {
            actionProcessed=true;
        }

        // Matches doublescape, since this is not a action, it's not done in actionHandler (is that logical??)
        if (doubleCharacters == (QList<int>() << Qt::Key_Escape << Qt::Key_Escape) ) {
            slotKillAllActions();
            actionProcessed=true;
            RS_DEBUG->print("QC_ApplicationWindow::Got double escape!");
        }

        if (actionProcessed) {
            doubleCharacters.clear();
        }
    }
    ts = now;

    if (actionProcessed==false) {
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
            if (graphicView!=NULL) {
                graphicView->keyPressEvent(e);
            }
            e->accept();
        }
            break;

        case Qt::Key_Return:
            slotEnter();
            e->accept();
            break;

        case Qt::Key_Plus:
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
            if (graphicView!=NULL) {
                graphicView->keyReleaseEvent(e);
            }
            e->accept();
        }
        break;
    }

    QMainWindow::keyPressEvent(e);
}



