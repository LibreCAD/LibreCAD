/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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
//Added by qt3to4:
#include <Q3StrList>
#include <QPixmap>
#include <QMouseEvent>
#include <QCloseEvent>
#include <q3mimefactory.h>
#include <QKeyEvent>
#include <Q3Frame>
#include <Q3PopupMenu>
// RVT_PORT added
#include <QImageReader>
// RVT_PORT added
#include <QImageWriter>


#include <fstream>

#include <q3accel.h>
#include <qaction.h>
#include <qapplication.h>
#include <qdatetime.h>
#include <qfile.h>
#include <q3filedialog.h>
#include <qnamespace.h>
#include <qmessagebox.h>
#include <q3paintdevicemetrics.h>
#include <qpainter.h>
#include <qprinter.h>
#include <qtimer.h>
#include <q3textstream.h>
#include <q3vbox.h>

#include <q3dockwindow.h>
#include <qeventloop.h>
#include <q3textedit.h>

#include "rs_application.h"
#include "rs_actiondrawlinefree.h"
#include "rs_actionprintpreview.h"
#include "rs_creation.h"
#include "rs_dialogfactory.h"
#include "rs_dimaligned.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_ellipse.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_fileio.h"
#include "rs_insert.h"
#include "rs_text.h"
#include "rs_settings.h"
#include "rs_script.h"
#include "rs_scriptlist.h"
#include "rs_solid.h"
#include "rs_staticgraphicview.h"
#include "rs_system.h"
#include "rs_actionlibraryinsert.h"
#include "rs_painterqt.h"

#include "qg_cadtoolbarmain.h"
#include "qg_colorbox.h"
#include "qg_coordinatewidget.h"
#include "qg_dlgimageoptions.h"
#include "qg_filedialog.h"
#include "qg_mousewidget.h"
#include "qg_pentoolbar.h"
#include "qg_selectionwidget.h"
#include "qg_cadtoolbarmain.h"
#include "qg_dlgimageoptions.h"
#include "qg_mousewidget.h"

#include "qc_mdiwindow.h"
#include "qc_dialogfactory.h"
#include "main.h"

QC_ApplicationWindow* QC_ApplicationWindow::appWindow = NULL;

#ifndef QC_APP_ICON
# define QC_APP_ICON "caduntu.png"
#endif
#ifndef QC_APP_ICON16
# define QC_APP_ICON16 "caduntu16.png"
#endif

# include <qsplashscreen.h>
    extern QSplashScreen *splash;

/**
 * Constructor. Initializes the app.
 */
QC_ApplicationWindow::QC_ApplicationWindow()
        : QMainWindow(0, "", Qt::WDestructiveClose),
        QG_MainWindowInterface()
{
	RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow");
    appWindow = this;
    // RVT_PORT assistant = NULL;
    workspace = NULL;
    
	RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: setting icon");
     setIcon(qPixmapFromMimeSource(QC_APP_ICON));

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

    // Disable menu and toolbar items
    emit windowsChanged(FALSE);

	
    statusBar()->showMessage(XSTR(QC_APPNAME) " Ready", 2000);
    //setFocusPolicy(WheelFocus);
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

	/*RVT_PORT     RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: " 
	"deleting assistant.."); 
    if (assistant!=NULL) {
        delete assistant;
    }
    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow: "
                    "deleting assistant: OK");
	*/
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
			script.latin1());
		slotRunScript(script);
    }
}



/**
 * Runs the script with the given name.
 */
void QC_ApplicationWindow::slotRunScript(const QString& name) {
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
			block.latin1());
		slotInsertBlock(block);
    }
}



/**
 * Called to insert blocks.
 */
void QC_ApplicationWindow::slotInsertBlock(const QString& name) {
	RS_DEBUG->print("QC_ApplicationWindow::slotInsertBlock: '%s'", name.latin1());

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
        splash->clear();
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
void QC_ApplicationWindow::closeEvent(QCloseEvent* /*ce*/) {
    RS_DEBUG->print("QC_ApplicationWindow::closeEvent()");

    slotFileQuit();

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
 * Initializes the MDI workspace.
 */
void QC_ApplicationWindow::initMDI() {
    RS_DEBUG->print("QC_ApplicationWindow::initMDI() begin");

    Q3VBox* vb = new Q3VBox(this);
    vb->setFrameStyle(Q3Frame::StyledPanel | Q3Frame::Sunken);
    workspace = new QWorkspace(vb);
    workspace->setScrollBarsEnabled(true);
    setCentralWidget(vb);

    connect(workspace, SIGNAL(windowActivated(QWidget*)),
            this, SLOT(slotWindowActivated(QWidget*)));

    RS_DEBUG->print("QC_ApplicationWindow::initMDI() end");
}



/**
 * Initializes all QActions of the application.
 */
void QC_ApplicationWindow::initActions() {
    RS_DEBUG->print("QC_ApplicationWindow::initActions()");

    QG_ActionFactory actionFactory(actionHandler, this);
    QAction* action;
    Q3PopupMenu* menu;
    QToolBar* tb;
    Q3PopupMenu* subMenu;


    // File actions:
    //
    menu = new Q3PopupMenu(this);
    tb = fileToolBar;
    tb->setCaption("File");

    action = actionFactory.createAction(RS2::ActionFileNew, this);
    action->addTo(menu);
    action->addTo(tb);
    action = actionFactory.createAction(RS2::ActionFileOpen, this);
    action->addTo(menu);
    action->addTo(tb);
    action = actionFactory.createAction(RS2::ActionFileSave, this);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionFileSaveAs, this);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionFileExport, this);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertSeparator();
    action = actionFactory.createAction(RS2::ActionFileClose, this);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertSeparator();
    action = actionFactory.createAction(RS2::ActionFilePrint, this);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionFilePrintPreview, this);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(printPreviewChanged(bool)), action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertSeparator();
    action = actionFactory.createAction(RS2::ActionFileQuit, this);
    action->addTo(menu);
    menu->insertSeparator();
    menuBar()->insertItem(tr("&File"), menu);
    addToolBar(Qt::TopToolBarArea, tb); //tr("File");

    fileMenu = menu;

    // Editing actions:
    //
    menu=new Q3PopupMenu(this);
    tb = editToolBar;
    tb->setCaption("Edit");

    action = actionFactory.createAction(RS2::ActionEditUndo, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionEditRedo, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    tb->addSeparator();
    menu->insertSeparator();

    action = actionFactory.createAction(RS2::ActionEditCut, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionEditCopy, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionEditPaste, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    menu->insertSeparator();

    action = actionFactory.createAction(RS2::ActionOptionsGeneral, this);
    action->addTo(menu);
    action = actionFactory.createAction(RS2::ActionOptionsDrawing, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    menuBar()->insertItem(tr("&Edit"), menu);
    //addToolBar(tb, tr("Edit"));
	addToolBar(Qt::TopToolBarArea, tb); //tr("Edit");

    // Options menu:
    //
    //menu = new QPopupMenu(this);
    //menuBar()->insertItem(tr("&Options"), menu);
	

    // Viewing / Zooming actions:
    //
    menu = new Q3PopupMenu(this);
    menu->setCheckable(true);
    tb = zoomToolBar;
    tb->setCaption("View");

    action = actionFactory.createAction(RS2::ActionViewGrid, this);
    action->addTo(menu);
    action->addTo(tb);
    action->setChecked(true);
    connect(this, SIGNAL(gridChanged(bool)), action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    RS_SETTINGS->beginGroup("/Appearance");
    bool draftMode = (bool)RS_SETTINGS->readNumEntry("/DraftMode", 0);
    RS_SETTINGS->endGroup();

    action = actionFactory.createAction(RS2::ActionViewDraft, this);
    action->addTo(menu);
    action->addTo(tb);
    action->setChecked(draftMode);
    connect(this, SIGNAL(draftChanged(bool)), action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    /*
       action = actionFactory.createAction(RS2::ActionViewLayerList, this);
       action->addTo(menu);
       action->setChecked(true);
       action = actionFactory.createAction(RS2::ActionViewBlockList, this);
       action->addTo(menu);
       action->setChecked(true);
       action = actionFactory.createAction(RS2::ActionViewOptionToolbar, this);
       action->addTo(menu);
       action->setChecked(true);
       action = actionFactory.createAction(RS2::ActionViewCommandLine, this);
       action->addTo(menu);
       action->setChecked(true);*/

    /*
    action = new QAction(tr("Back"),
                        tr("&Back"), Key_Escape, this);
       connect(action, SIGNAL(activated()),
               this, SLOT(slotBack()));
       action->addTo(menu);
    */


    action = actionFactory.createAction(RS2::ActionZoomRedraw, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomIn, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomOut, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomAuto, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomPrevious, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomWindow, actionHandler);
    action->addTo(menu);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionZoomPan, actionHandler);
	menu->addAction(action);
    action->addTo(tb);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    menu->insertSeparator();

    action = actionFactory.createAction(RS2::ActionViewStatusBar, this);
    action->addTo(menu);
    action->setChecked(true);

    // RVT_PORT menu->insertItem(tr("Vie&ws"), createDockWindowMenu(NoToolBars));
    // RVT_PORT menu->insertItem(tr("Tool&bars"), createDockWindowMenu(OnlyToolBars));

	// tr("Focus on Command Line")
	action = new QAction(tr("Focus on &Command Line"), this);
	action->setIcon(QIcon(":/main/editclear.png"));
	action->setShortcut(tr("CTRL+M"));
	//action->zetStatusTip(tr("Focus on Command Line"));
		
    connect(action, SIGNAL(activated()),
            this, SLOT(slotFocusCommandLine()));
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&View"), menu);
    //addToolBar(tb, tr("View"));
	addToolBar(Qt::TopToolBarArea, tb); //tr("View");
    // Selecting actions:
    //
    menu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDeselectAll, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectAll, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectSingle, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectContour, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDeselectWindow, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectWindow, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectInvert, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectIntersected,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDeselectIntersected,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSelectLayer, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&Select"), menu);

    // Drawing actions:
    //
    menu=new Q3PopupMenu(this);

    // Points:
    subMenu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDrawPoint, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertItem(tr("&Point"), subMenu);

    // Lines:
    subMenu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDrawLine,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineAngle,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineHorizontal,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineVertical,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineRectangle,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineParallel,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineParallelThrough,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineBisector,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineTangent1,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineTangent2,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineOrthogonal,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineRelAngle,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLineFree,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    //action = actionFactory.createAction(RS2::ActionDrawLineHorVert,
    //                                    actionHandler);
    //action->addTo(subMenu);
    action = actionFactory.createAction(RS2::ActionDrawLinePolygon,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawLinePolygon2,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    action = actionFactory.createAction(RS2::ActionDrawPolyline,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));

    menu->insertItem(tr("&Line"), subMenu);

    // Arcs:
    subMenu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDrawArc, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawArc3P, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawArcParallel, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertItem(tr("&Arc"), subMenu);
    // Circles:
    subMenu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDrawCircle, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleCR, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircle2P, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircle3P, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawCircleParallel, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertItem(tr("&Circle"), subMenu);
	
    // Ellipses:
    subMenu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDrawEllipseAxis,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDrawEllipseArcAxis,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertItem(tr("&Ellipse"), subMenu);

    // Splines:
    subMenu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDrawSpline, actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertItem(tr("&Spline"), subMenu);
    
	// Polylines:
    subMenu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDrawPolyline,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertItem(tr("&Polyline"), subMenu);

    action = actionFactory.createAction(RS2::ActionPolylineAdd,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionPolylineDel,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionPolylineDelBetween,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionPolylineTrim,
                                        actionHandler);
    action->addTo(subMenu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
	
    // Text:
    action = actionFactory.createAction(RS2::ActionDrawText,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    // Hatch:
    action = actionFactory.createAction(RS2::ActionDrawHatch,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    // Image:
    action = actionFactory.createAction(RS2::ActionDrawImage,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&Draw"), menu);

    // Dimensioning actions:
    //
#ifdef __APPLE1__

    Q3PopupMenu* m = menu;
#endif

    menu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionDimAligned, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLinear, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLinearHor, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLinearVer, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimRadial, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimDiametric, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimAngular, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionDimLeader, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
#ifdef __APPLE1__

    m->insertItem(tr("&Dimension"), menu);
#else

    menuBar()->insertItem(tr("&Dimension"), menu);
#endif

    // Modifying actions:
    //
    menu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionModifyMove,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyRotate,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyScale,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyMirror,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyMoveRotate,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyRotate2,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyTrim,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyTrim2,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyTrimAmount,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyBevel,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyRound,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyCut,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyStretch,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyEntity,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyAttributes,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyDelete,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyDeleteQuick,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionModifyExplodeText,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    //action = actionFactory.createAction(RS2::ActionModifyDeleteFree,
    //                                    actionHandler);
    //action->addTo(menu);
    action = actionFactory.createAction(RS2::ActionBlocksExplode, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&Modify"), menu);

    // Snapping actions:
    //
    menu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionSnapFree, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action->setChecked(true);
    action = actionFactory.createAction(RS2::ActionSnapGrid, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSnapEndpoint,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSnapOnEntity,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSnapCenter, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSnapMiddle, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSnapDist, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSnapIntersection,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionSnapIntersectionManual,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertSeparator();
    action = actionFactory.createAction(RS2::ActionRestrictNothing,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionRestrictOrthogonal,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionRestrictHorizontal,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionRestrictVertical,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menu->insertSeparator();
    action = actionFactory.createAction(RS2::ActionSetRelativeZero,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLockRelativeZero,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&Snap"), menu);

    // Info actions:
    //
    menu=new Q3PopupMenu(this);
    //action = actionFactory.createAction(RS2::ActionInfoInside,
    //                                    actionHandler);
    //action->addTo(menu);
    action = actionFactory.createAction(RS2::ActionInfoDist,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionInfoDist2,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionInfoAngle,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionInfoTotalLength,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&Info"), menu);

    // Layer actions:
    //
    menu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionLayersDefreezeAll,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersFreezeAll,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersAdd, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersRemove,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersEdit, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionLayersToggleView,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&Layer"), menu);

    // Block actions:
    //
    menu=new Q3PopupMenu(this);
    action = actionFactory.createAction(RS2::ActionBlocksDefreezeAll,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksFreezeAll,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksAdd, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksRemove, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksAttributes,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksInsert,
                                        actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksEdit, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksCreate, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    action = actionFactory.createAction(RS2::ActionBlocksExplode, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    menuBar()->insertItem(tr("&Block"), menu);
	
	QMainWindow::addToolBarBreak(Qt::TopToolBarArea);
	

	addToolBar(Qt::TopToolBarArea, penToolBar);                                                                                                               
    addToolBar(Qt::TopToolBarArea, optionWidget); 
	
	
#ifdef RS_SCRIPTING
    // Scripts menu:
    //
    scriptMenu = new Q3PopupMenu(this);
    scriptOpenIDE = actionFactory.createAction(RS2::ActionScriptOpenIDE, this);
    scriptOpenIDE->addTo(scriptMenu);
    scriptRun = actionFactory.createAction(RS2::ActionScriptRun, this);
    scriptRun->addTo(scriptMenu);
#else
    scriptMenu = 0;
    scriptOpenIDE = 0;
    scriptRun = 0;
#endif

#ifdef RVT_CAM
    menu=new Q3PopupMenu(this);

    action = actionFactory.createAction(RS2::ActionCamMakeProfile, actionHandler);
    action->addTo(menu);
    connect(this, SIGNAL(windowsChanged(bool)), action, SLOT(setEnabled(bool)));
    
    menuBar()->insertItem(tr("&CAM"), menu);
#endif

    // Help menu:
    //
    /*RVT_PORThelpAboutApp = new QAction(tr("About"), 
							   qPixmapFromMimeSource(QC_APP_ICON16), 
							   tr("&About %1").arg(QC_APPNAME), 0, this); */
    helpAboutApp = new QAction(qPixmapFromMimeSource(QC_APP_ICON16), tr("About"), this);

    //helpAboutApp->zetStatusTip(tr("About the application"));
    //helpAboutApp->setWhatsThis(tr("About\n\nAbout the application"));
    connect(helpAboutApp, SIGNAL(activated()),
            this, SLOT(slotHelpAbout()));

    helpManual = new QAction(qPixmapFromMimeSource("contents.png"), tr("&Manual"), this);
    //helpManual->zetStatusTip(tr("Launch the online manual"));
    connect(helpManual, SIGNAL(activated()),
            this, SLOT(slotHelpManual()));

/* RVT_PORT    testDumpEntities = new QAction("Dump Entities",
                                   "Dump &Entities", 0, this); */
    testDumpEntities = new QAction("Dump Entities", this);
    connect(testDumpEntities, SIGNAL(activated()),
            this, SLOT(slotTestDumpEntities()));
    
/* RVT_PORT	testDumpUndo = new QAction("Dump Undo Info",
							   "Undo Info", 0, this); */
	testDumpUndo = new QAction("Dump Undo Info", this);
    connect(testDumpUndo, SIGNAL(activated()),
            this, SLOT(slotTestDumpUndo()));

/* RVT_PORT    testUpdateInserts = new QAction("Update Inserts",
                                    "&Update Inserts", 0, this); */
    testUpdateInserts = new QAction("Update Inserts", this);
    connect(testUpdateInserts, SIGNAL(activated()),
            this, SLOT(slotTestUpdateInserts()));

/* RVT_PORT    testDrawFreehand = new QAction("Draw Freehand",
	 "Draw Freehand", 0, this); */
	 testDrawFreehand = new QAction("Draw Freehand", this); 
    connect(testDrawFreehand, SIGNAL(activated()),
            this, SLOT(slotTestDrawFreehand()));

/* RVT_PORT    testInsertBlock = new QAction("Insert Block",
                                  "Insert Block", 0, this); */
    testInsertBlock = new QAction("Insert Block", this);

    connect(testInsertBlock, SIGNAL(activated()),
            this, SLOT(slotTestInsertBlock()));

/* RVT_PORT    testInsertText = new QAction("Insert Text",
                                 "Insert Text", 0, this); */
    testInsertText = new QAction("Insert Text", this);
    connect(testInsertText, SIGNAL(activated()),
            this, SLOT(slotTestInsertText()));

/* RVT_PORT    testInsertImage = new QAction("Insert Image",
                                  "Insert Image", 0, this); */
	// "Insert Image",
    testInsertImage = new QAction(tr("Insert Image"), this);
    connect(testInsertImage, SIGNAL(activated()),
            this, SLOT(slotTestInsertImage()));

/* RVT_PORT    testUnicode = new QAction("Unicode",
                              "Unicode", 0, this); */
    testUnicode = new QAction("Unicode", this);
    connect(testUnicode, SIGNAL(activated()),
            this, SLOT(slotTestUnicode()));

/* RVT_PORT    testInsertEllipse = new QAction("Insert Ellipse",
                                    "Insert Ellipse", 0, this); */
    testInsertEllipse = new QAction("Insert Ellipse", this);
    connect(testInsertEllipse, SIGNAL(activated()),
            this, SLOT(slotTestInsertEllipse()));

/*  RVT_PORT  testMath01 = new QAction("Math01",
                             "Math01", 0, this); */
    testMath01 = new QAction("Math01", this);
    connect(testMath01, SIGNAL(activated()),
            this, SLOT(slotTestMath01()));

/* RVT_PORT    testResize640 = new QAction("Resize to 640x480",
                                "Resize 1", 0, this); */
    testResize640 = new QAction("Resize to 640x480", this);
    connect(testResize640, SIGNAL(activated()),
            this, SLOT(slotTestResize640()));

/* RVT_PORT    testResize800 = new QAction("Resize to 800x600",
                                "Resize 2", 0, this); */
    testResize800 = new QAction("Resize to 800x600", this);
    connect(testResize800, SIGNAL(activated()),
            this, SLOT(slotTestResize800()));

/* RVT_PORT    testResize1024 = new QAction("Resize to 1024x768",
                                 "Resize 3", 0, this); */
    testResize1024 = new QAction("Resize to 1024x768", this);
    connect(testResize1024, SIGNAL(activated()),
            this, SLOT(slotTestResize1024()));

}


/**
 * Initializes the menu bar.
 */
void QC_ApplicationWindow::initMenuBar() {
    RS_DEBUG->print("QC_ApplicationWindow::initMenuBar()");

    // menuBar entry windowsMenu
    windowsMenu = new Q3PopupMenu(this);
    windowsMenu->setCheckable(true);
    connect(windowsMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotWindowsMenuAboutToShow()));

    // menuBar entry scriptMenu
    //scriptMenu = new QPopupMenu(this);
    //scriptMenu->setCheckable(true);
    //scriptOpenIDE->addTo(scriptMenu);
    //scriptRun->addTo(scriptMenu);
    //connect(scriptMenu, SIGNAL(aboutToShow()),
    //        this, SLOT(slotScriptMenuAboutToShow()));

    // menuBar entry helpMenu
    helpMenu=new Q3PopupMenu();
    helpManual->addTo(helpMenu);
    helpMenu->insertSeparator();
    helpAboutApp->addTo(helpMenu);

    // menuBar entry test menu
    testMenu=new Q3PopupMenu();
    testDumpEntities->addTo(testMenu);
    testDumpUndo->addTo(testMenu);
    testUpdateInserts->addTo(testMenu);
    testDrawFreehand->addTo(testMenu);
    testInsertBlock->addTo(testMenu);
    testInsertText->addTo(testMenu);
    testInsertImage->addTo(testMenu);
    testInsertEllipse->addTo(testMenu);
    testUnicode->addTo(testMenu);
    testMath01->addTo(testMenu);
    testResize640->addTo(testMenu);
    testResize800->addTo(testMenu);
    testResize1024->addTo(testMenu);

    // menuBar configuration
#ifdef RS_SCRIPTING
    menuBar()->insertItem(tr("&Scripts"), scriptMenu);
#endif    
    menuBar()->insertItem(tr("&Window"), windowsMenu);
    menuBar()->insertSeparator();
    menuBar()->insertItem(tr("&Help"), helpMenu);
    if (QC_DEBUGGING) {
        menuBar()->insertItem(tr("De&bugging"), testMenu);
    }

    recentFiles = new QG_RecentFiles(9);
}



/**
 * Initializes the tool bars (file tool bar and pen tool bar).
 */
void QC_ApplicationWindow::initToolBar() {
    RS_DEBUG->print("QC_ApplicationWindow::initToolBar()");

	QSizePolicy toolBarPolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed); 

    fileToolBar = new QToolBar( "File Operations", this);	
	fileToolBar->setSizePolicy(toolBarPolicy);
	
    editToolBar = new QToolBar( "Edit Operations", this);
	editToolBar->setSizePolicy(toolBarPolicy);
    zoomToolBar = new QToolBar( "Zoom Operations", this);
	zoomToolBar->setSizePolicy(toolBarPolicy);
	    
	penToolBar = new QG_PenToolBar("Pen Selection", this);
	penToolBar->setSizePolicy(toolBarPolicy);
	
    connect(penToolBar, SIGNAL(penChanged(RS_Pen)),
            this, SLOT(slotPenChanged(RS_Pen))); 

    optionWidget = new QToolBar("Tool Options", this);
	QSizePolicy optionWidgetBarPolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed); 
	optionWidget->setMinimumSize(440,30);
	optionWidget->setSizePolicy(optionWidgetBarPolicy);	
	
    //optionWidget->setFixedExtentHeight(26);
    //optionWidget->setHorizontallyStretchable(true);
    //addDockWindow(optionWidget, DockTop, true);

    // CAD toolbar left:
    QToolBar* t = new QToolBar("CAD Tools", this);
	t->setMinimumSize(59,250);
	QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding); 
	t->setSizePolicy(policy);
   // t->setFixedExtentWidth(59);
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
    QString docks = RS_SETTINGS->readEntry("/DockWindows", "");
    RS_SETTINGS->endGroup();

    Q3TextStream ts(&docks, QIODevice::ReadOnly);
// RVT_PORT    ts >> *this;
}


/**
 * Stores the global application settings to file or registry.
 */
void QC_ApplicationWindow::storeSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::storeSettings()");
    QString docks;
    Q3TextStream ts(&docks, QIODevice::WriteOnly);

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
// RVT_PORT    ts << *this;
    RS_SETTINGS->writeEntry("/DockWindows", docks);
    RS_SETTINGS->endGroup();

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
    dw->setCaption(tr("Layer List"));
    // dw->setCloseMode(QDockWidget::Always);
    //dw->resize(120,workspace->height()/2);
    addDockWidget (Qt::RightDockWidgetArea, dw );

	
    layerDockWindow = dw;

    RS_DEBUG->print("  block widget..");
    dw = new QDockWidget("Block", this);
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
    dw->setCaption(tr("Block List"));
    // dw->setCloseMode(QDockWidget::Always);
    //dw->setFixedExtentHeight(400);
	addDockWidget(Qt::RightDockWidgetArea, dw); 
    blockDockWindow = dw;

    RS_DEBUG->print("  library widget..");
    dw = new QDockWidget("Library", this);
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
    dw->setCaption(tr("Library Browser"));
    // dw->setCloseMode(QDockWidget::Always);
    addDockWidget(Qt::LeftDockWidgetArea , dw);

    libraryDockWindow = dw;
    libraryDockWindow->hide();
	

    RS_DEBUG->print("  command widget..");
    dw = new QDockWidget("Command", this);
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
    dw->setCaption(tr("Command line"));
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
/*QToolBar* QC_ApplicationWindow::createToolBar(const RS_String& name) {
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
			QPixmap::fromMimeSource("zoomwindow.png"),
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
    for (int i=0; i<recentFiles->getNumber(); ++i) {
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
    }
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


/**
 * Called when a document window was activated.
 */
void QC_ApplicationWindow::slotWindowActivated(QWidget*) {

    RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated begin");

    QC_MDIWindow* m = getMDIWindow();

    if (m!=NULL && m->getDocument()!=NULL) {
		m->setWindowState(Qt::WindowMaximized);

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
        m->getGraphicView()->redraw();

        // set snapmode from snapping menu
        actionHandler->updateSnapMode();

        // set pen from pen toolbar
        slotPenChanged(penToolBar->getPen());

        // update toggle button status:
        if (m->getGraphic()!=NULL) {
            emit(gridChanged(m->getGraphic()->isGridOn()));
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
    int cascadeId = windowsMenu->insertItem(tr("&Cascade"),
                                            workspace, SLOT(cascade()));
    int tileId = windowsMenu->insertItem(tr("&Tile"),
                                         this, SLOT(slotTileVertical()));
    int horTileId = windowsMenu->insertItem(tr("Tile &Horizontally"),
                                            this, SLOT(slotTileHorizontal()));
    if (workspace->windowList().isEmpty()) {
        windowsMenu->setItemEnabled(cascadeId, false);
        windowsMenu->setItemEnabled(tileId, false);
        windowsMenu->setItemEnabled(horTileId, false);
    }
    windowsMenu->insertSeparator();
    QWidgetList windows = workspace->windowList();
    for (int i=0; i<int(windows.count()); ++i) {
        int id = windowsMenu->insertItem(windows.at(i)->caption(),
                                         this, SLOT(slotWindowsMenuActivated(int)));
        windowsMenu->setItemParameter(id, i);
        windowsMenu->setItemChecked(id, workspace->activeWindow()==windows.at(i));
    }
}



/**
 * Called when the user selects a document window from the
 * window list.
 */
void QC_ApplicationWindow::slotWindowsMenuActivated(int id) {
    RS_DEBUG->print("QC_ApplicationWindow::slotWindowsMenuActivated");

    QWidget* w = workspace->windowList().at(id);
    if (w!=NULL) {
        w->showNormal();
        w->setFocus();
        // RVT_PORT need to reset/cleanup current menu here to avoid menu clutter
    }
}



/**
 * Tiles MDI windows horizontally.
 */
void QC_ApplicationWindow::slotTileHorizontal() {

    RS_DEBUG->print("QC_ApplicationWindow::slotTileHorizontal");

    // primitive horizontal tiling
    QWidgetList windows = workspace->windowList();
    if (windows.count()==0) {
        return;
    }

    int heightForEach = workspace->height() / windows.count();
    int y = 0;
    for (int i=0; i<int(windows.count()); ++i) {
        QWidget *window = windows.at(i);
/* RVT_PORT 
		if (window->testWState(WState_Maximized)) {
            // prevent flicker
            window->hide();
            window->showNormal();
        } */
        int preferredHeight = window->minimumHeight()
                              + window->parentWidget()->baseSize().height();
        int actHeight = QMAX(heightForEach, preferredHeight);

        //window->parentWidget()->resize(workspace->width(), actHeight);
        window->parentWidget()->setGeometry(0, y,
                                            workspace->width(), actHeight);
        y+=actHeight;
    }
}



/**
 * Tiles MDI windows vertically.
 */
void QC_ApplicationWindow::slotTileVertical() {
    workspace->tile();

    /*
       QWidgetList windows = workspace->windowList();
       if (windows.count()==0) {
           return;
       }

       //int heightForEach = workspace->height() / windows.count();
       //int y = 0;
       for (int i=0; i<int(windows.count()); ++i) {
           QWidget *window = windows.at(i);
        if (window->testWState(WState_Maximized)) {
               // prevent flicker
               window->hide();
               window->showNormal();
           }
           //int preferredHeight = window->minimumHeight()
           //                      + window->parentWidget()->baseSize().height();
           //int actHeight = QMAX(heightForEach, preferredHeight);

           //window->parentWidget()->setGeometry(0, y,
           //                                    workspace->width(), actHeight);
           //window->parentWidget()->resize(window->parentWidget()->width(), 
        //        window->parentWidget()->height());
           //window->resize(window->width(), window->height());
           //y+=actHeight;
       }
    */
}



/**
 * CAM
 */
/*
#ifdef RS_CAM
void QC_ApplicationWindow::slotCamExportAuto() {
    printf("CAM export..\n");
    
    RS_Document* d = getDocument();
    if (d!=NULL) {
        RS_Graphic* graphic = (RS_Graphic*)d;
 
        RS_CamDialog dlg(graphic, this);
        dlg.exec();
    }
}
#endif
*/



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
 * Creates a new MDI window with the given document or a new
 *  document if 'doc' is NULL.
 */
QC_MDIWindow* QC_ApplicationWindow::slotFileNew(RS_Document* doc) {

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNew() begin");

    static int id = 0;
    id++;

    statusBar()->showMessage(tr("Creating new file..."));

    RS_DEBUG->print("  creating MDI window");
    QC_MDIWindow* w = new QC_MDIWindow(doc, workspace,
                                       0, Qt::WDestructiveClose);
	//w->setWindowState(WindowMaximized);
    connect(w, SIGNAL(signalClosing()),
            this, SLOT(slotFileClosing()));

    if (w->getDocument()->rtti()==RS2::EntityBlock) {
        w->setCaption(tr("Block '%1'").arg(((RS_Block*)(w->getDocument()))->getName()));
    } else {
        w->setCaption(tr("unnamed document %1").arg(id));
    }
    w->setIcon(qPixmapFromMimeSource("document.png"));

    // only graphics offer block lists, blocks don't
    RS_DEBUG->print("  adding listeners");
    RS_Graphic* graphic = w->getDocument()->getGraphic();
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
    QG_DIALOGFACTORY->setCoordinateWidget(coordinateWidget);
    QG_DIALOGFACTORY->setSelectionWidget(selectionWidget);
    // Link the dialog factory to the option widget:
    //QG_DIALOGFACTORY->setOptionWidget(optionWidget);
    // Link the dialog factory to the cad tool bar:
    QG_DIALOGFACTORY->setCadToolBar(cadToolBar);
    // Link the dialog factory to the command widget:
    QG_DIALOGFACTORY->setCommandWidget(commandWidget);
    // Link the dialog factory to the main app window:
    QG_DIALOGFACTORY->setMainWindow(this);
	
	workspace->addWindow(w);

    RS_DEBUG->print("  showing MDI window");
    if (workspace->windowList().isEmpty()) {
        w->showMaximized();
    } else {
        w->show();
    }
    slotWindowActivated(w);
    statusBar()->showMessage(tr("New Drawing created."), 2000);

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNew() OK");
    setFocus();
	
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
    QString fileName = QG_FileDialog::getOpenFileName(this, &type);
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen() 003");
    slotFileOpen(fileName, type);
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(): OK");
}



/**
 * Called when a recently opened file is chosen from the list in the
 * file menu.
 */
void QC_ApplicationWindow::slotFileOpenRecent(int id) {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpenRecent()");

    statusBar()->showMessage(tr("Opening recent file..."));
    QString fileName = recentFiles->get(id);

    if (fileName.endsWith(" (DXF 1)")) {
        slotFileOpen(fileName.left(fileName.length()-8), RS2::FormatDXF1);
    } else {
        slotFileOpen(fileName, RS2::FormatUnknown);
    }
}



/**
 * Menu file -> open.
 */
void QC_ApplicationWindow::slotFileOpen(const QString& fileName,
                                        RS2::FormatType type) {

    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(..)");

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    if (!fileName.isEmpty()) {
        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: creating new doc window");
        // Create new document window:
        QC_MDIWindow* w = slotFileNew();
        // RVT_PORT RS_APP->processEvents(1000);
        RS_APP->processEvents(QEventLoop::AllEvents, 1000);

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: linking layer list");
        // link the layer widget to the new document:
        layerWidget->setLayerList(w->getDocument()->getLayerList(), false);
        // link the block widget to the new document:
        blockWidget->setBlockList(w->getDocument()->getBlockList());
        // link coordinate widget to graphic
        coordinateWidget->setGraphic(w->getGraphic());

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file");
        
        // RVT_PORT RS_APP->processEvents(1000);
        RS_APP->processEvents(QEventLoop::AllEvents, 1000);

        // open the file in the new view:
        if (w->slotFileOpen(fileName, type)==false) {
            // error
            QApplication::restoreOverrideCursor();
            QMessageBox::information(this, QMessageBox::tr("Warning"),
                                     tr("Cannot open the file\n%1\nPlease "
                                        "check the permissions.")
                                     .arg(fileName),
                                     QMessageBox::Ok);
            w->setForceClosing(true);
            w->close();
            return;
        }

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: 1");

        // update recent files menu:
        if (type==RS2::FormatDXF1) {
            recentFiles->add(fileName + " (DXF 1)");
        } else {
            recentFiles->add(fileName);
        }
        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: 2");
        updateRecentFilesMenu();

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: update recent file menu: OK");

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: set caption");
        // update caption:
        w->setCaption(fileName);
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
    } else {
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
                // error
                QMessageBox::information(this, QMessageBox::tr("Warning"),
                                         tr("Cannot save the file\n%1\nPlease "
                                            "check the permissions.")
                                         .arg(w->getDocument()->getFilename()),
                                         QMessageBox::Ok);
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
            	w->setCaption(name);
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
 * Menu file -> export.
 */
void QC_ApplicationWindow::slotFileExport() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileExport()");

    statusBar()->showMessage(tr("Exporting drawing..."));

    QC_MDIWindow* w = getMDIWindow();
    QString fn;
    if (w!=NULL) {

        // read default settings:
        RS_SETTINGS->beginGroup("/Paths");
        RS_String defDir = RS_SETTINGS->readEntry("/ExportImage",
                           RS_SYSTEM->getHomeDir());
        RS_String defFilter = RS_SETTINGS->readEntry("/ExportImageFilter",
                              "Portable Network Graphic (*.png)");
        RS_SETTINGS->endGroup();

        bool cancel = false;

        Q3FileDialog fileDlg(NULL, "", true);

        //RVT_PORT Q3StrList f = QImageIO::outputFormats();
        //QStringList formats = QStringList::fromStrList(f);
        QStringList filters;
        //QString all = "";

		/* RVT_PORT		for (QStringList::Iterator it = formats.begin();
		 it!=formats.end(); ++it) { */

        foreach (QByteArray format, QImageReader::supportedImageFormats()) {    
			filters.append(format);  
			/* RVT_PORT
            QString st;
            if ((*it)=="JPEG") {
                st = QString("%1 (*.%2 *.jpg)")
                     .arg(QG_DialogFactory::extToFormat(*it))
                     .arg(QString(*it).lower());
            } else {
                st = QString("%1 (*.%2)")
                     .arg(QG_DialogFactory::extToFormat(*it))
                     .arg(QString(*it).lower());
            }
            filters.append(st); */

            //if (!all.isEmpty()) {
            //    all += " ";
            //}
            //all += QString("*.%1").arg(QString(*it).lower());
        }

        fileDlg.setFilters(filters);
        fileDlg.setMode(Q3FileDialog::AnyFile);
        fileDlg.setCaption(QObject::tr("Export Image"));
        fileDlg.setDir(defDir);
        fileDlg.setSelectedFilter(defFilter);

        if (fileDlg.exec()==QDialog::Accepted) {
            fn = fileDlg.selectedFile();
            cancel = false;
        } else {
            cancel = true;
        }

        // store new default settings:
        if (!cancel) {
            RS_SETTINGS->beginGroup("/Paths");
            RS_SETTINGS->writeEntry("/ExportImage", QFileInfo(fn).dirPath(true));
            RS_SETTINGS->writeEntry("/ExportImageFilter",
                                    fileDlg.selectedFilter());
            RS_SETTINGS->endGroup();

            // find out extension:
            QString filter = fileDlg.selectedFilter();
            QString format = "";
            int i = filter.find("(*.");
            if (i!=-1) {
                int i2 = filter.find(QRegExp("[) ]"), i);
                format = filter.mid(i+3, i2-(i+3));
                format = format.upper();
            }

            // append extension to file:
            if (!QFileInfo(fn).fileName().contains(".")) {
                fn.append("." + format.lower());
            }

            // show options dialog:
            QG_ImageOptionsDialog dlg(this);
            dlg.setGraphicSize(w->getGraphic()->getSize());
            if (dlg.exec()) {
                bool ret = slotFileExport(fn, format, dlg.getSize(),
                                          dlg.isBackgroundBlack());
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
        const QString& format, QSize size, bool black, bool bw) {

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
    QPixmap* buffer = new QPixmap(size);
    RS_PainterQt* painter = new RS_PainterQt(buffer);

    // black background:
    if (black) {
        painter->setBackgroundColor(RS_Color(0,0,0));
    }
    // white background:
    else {
        painter->setBackgroundColor(RS_Color(255,255,255));
    }

    // black/white:
    if (bw) {
        painter->setDrawingMode(RS2::ModeBW);
    }

    painter->eraseRect(0,0, size.width(), size.height());

    RS_StaticGraphicView gv(size.width(), size.height(), painter);
    if (black) {
        gv.setBackground(RS_Color(0,0,0));
    } else {
        gv.setBackground(RS_Color(255,255,255));
    }
    gv.setContainer(graphic);
    gv.zoomAuto(false);
    gv.drawEntity(graphic, true);

    // RVT_PORT QImageIO iio;
    QImageWriter iio;
    QImage img;
    img = *buffer;
    // RVT_PORT iio.setImage(img);
    iio.setFileName(name);
    iio.setFormat(format.ascii());
    // RVT_PORT if (iio.write()) {
	if (iio.write(img)) {
        ret = true;
    }
    QApplication::restoreOverrideCursor();

    // GraphicView deletes painter
    painter->end();
    delete buffer;

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
    RS_DEBUG->print("QC_ApplicationWindow::slotFileClose()");

    QC_MDIWindow* m = getMDIWindow();
    if (m!=NULL) {
        m->close(true);
    }
   
   	/*
	m = getMDIWindow();
    if (m!=NULL) {
		//m->showMaximized();
		m->setWindowState(WindowMaximized);
	}
	*/
}



/**
 * Called when a MDI window is actually about to close. Used to 
 * detach widgets from the document.
 */
void QC_ApplicationWindow::slotFileClosing() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileClosing()");

    RS_DEBUG->print("detaching lists");
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
    QPrinter* printer;
    printer = new QPrinter(QPrinter::HighResolution);
    bool landscape = false;
    printer->setPageSize(RS2::rsToQtPaperFormat(graphic->getPaperFormat(&landscape)));
    if (landscape) {
        printer->setOrientation(QPrinter::Landscape);
    } else {
        printer->setOrientation(QPrinter::Portrait);
    }

    RS_SETTINGS->beginGroup("/Print");
    printer->setOutputFileName(RS_SETTINGS->readEntry("/FileName", ""));
    printer->setColorMode((QPrinter::ColorMode)RS_SETTINGS->readNumEntry("/ColorMode", (int)QPrinter::Color));
    printer->setOutputToFile((bool)RS_SETTINGS->readNumEntry("/PrintToFile",
                             0));
    RS_SETTINGS->endGroup();

    // printer setup:
    if (printer->setup(this)) {
        //printer->setOutputToFile(true);
        //printer->setOutputFileName(outputFile);

        QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
        printer->setFullPage(true);
        Q3PaintDeviceMetrics metr(printer);

        RS_PainterQt* painter = new RS_PainterQt(printer);
        painter->setDrawingMode(w->getGraphicView()->getDrawingMode());

        RS_StaticGraphicView gv(metr.width(), metr.height(), painter);
        gv.setPrinting(true);
        gv.setBorders(0,0,0,0);

        double fx = (double)metr.width() / metr.widthMM()
                    * RS_Units::getFactorToMM(graphic->getUnit());
        double fy = (double)metr.height() / metr.heightMM()
                    * RS_Units::getFactorToMM(graphic->getUnit());

        double f = (fx+fy)/2;

        double scale = graphic->getPaperScale();

        gv.setOffset((int)(graphic->getPaperInsertionBase().x * f),
                     (int)(graphic->getPaperInsertionBase().y * f));
        gv.setFactor(f*scale);

        gv.setContainer(graphic);

        gv.drawEntity(painter, graphic, true);

        // GraphicView deletes painter
        painter->end();

        RS_SETTINGS->beginGroup("/Print");
        RS_SETTINGS->writeEntry("/PrintToFile", (int)printer->outputToFile());
        RS_SETTINGS->writeEntry("/ColorMode", (int)printer->colorMode());
        RS_SETTINGS->writeEntry("/FileName", printer->outputFileName());
        RS_SETTINGS->endGroup();
        QApplication::restoreOverrideCursor();
    }

    delete printer;

    statusBar()->showMessage(tr("Printing complete"), 2000);
}



/**
 * Menu file -> print preview.
 */
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
    if (!on) {
        RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): off");
        if (parent->getGraphicView()->isPrintPreview()) {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): close");
            slotFileClose();
        }
    }

    // open print preview:
    else {
        // look for an existing print preview:
        QC_MDIWindow* ppv = parent->getPrintPreview();
        if (ppv!=NULL) {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): show existing");
            slotWindowActivated(ppv);
            ppv->showNormal();
        } else {
            if (!parent->getGraphicView()->isPrintPreview()) {
                RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): create");

                QC_MDIWindow* w = new QC_MDIWindow(parent->getDocument(), workspace,
                                                   0, Qt::WDestructiveClose);
				workspace->addWindow(w);
				w->setWindowState(Qt::WindowMaximized);
                parent->addChildWindow(w);
                //connect(w, SIGNAL(signalClosing()),
                //        this, SLOT(slotFileClosing()));

                w->setCaption(tr("Print preview for %1").arg(parent->caption()));
                w->setIcon(qPixmapFromMimeSource("document.png"));
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

                    // Center by default:
                    graphic->centerToPage();
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

                if (workspace->windowList().isEmpty()) {
                    w->showMaximized();
                } else {
                    w->show();
                }
                w->getGraphicView()->zoomPage();
                setFocus();

                slotWindowActivated(w);
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
    if (workspace!=NULL) {
        QWidgetList windows = workspace->windowList();
        for (int i = 0; i < int(windows.count()); ++i) {
            QC_MDIWindow* m = (QC_MDIWindow*)windows.at(i);
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
    if (workspace!=NULL) {
        QWidgetList windows = workspace->windowList();
        for (int i = 0; i < int(windows.count()); ++i) {
            QC_MDIWindow* m = (QC_MDIWindow*)windows.at(i);
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
 * Shows / hides the layer list.
 *
 * @param toggle true: show, false: hide.
 */
/*void QC_ApplicationWindow::slotViewLayerList(bool toggle) {
    RS_DEBUG->print("QC_ApplicationWindow::slotViewLayerList()");
 
    if (toggle==false) {
        layerDockWindow->hide();
    } else {
        layerDockWindow->show();
    }
}
*/


/**
 * Shows / hides the block list.
 *
 * @param toggle true: show, false: hide.
 */
/*
void QC_ApplicationWindow::slotViewBlockList(bool toggle) {
    RS_DEBUG->print("QC_ApplicationWindow::slotViewBlockList()");
 
    if (toggle==false) {
        blockDockWindow->hide();
    } else {
        blockDockWindow->show();
    }
}
*/



/**
 * Shows / hides the command line.
 *
 * @param toggle true: show, false: hide.
 */
/*
void QC_ApplicationWindow::slotViewCommandLine(bool toggle) {
    RS_DEBUG->print("QC_ApplicationWindow::slotViewCommandLine()");
 
    if (toggle==false) {
        commandDockWindow->hide();
        //QG_GraphicView* graphicView = getGraphicView();
        //if (graphicView!=NULL) {
        //graphicView->setFocus();
        //}
        setFocus();
    } else {
        commandDockWindow->show();
    }
}
*/



/**
 * Shows / hides the option toolbar.
 *
 * @param toggle true: show, false: hide.
 */
/*
void QC_ApplicationWindow::slotViewOptionToolbar(bool toggle) {
    RS_DEBUG->print("QC_ApplicationWindow::slotViewOptionToolbar()");
 
    if (toggle==false) {
        optionWidget->hide();
    } else {
        optionWidget->show();
    }
}
*/


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
}
*/



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

    QWidgetList windows = workspace->windowList();
    for (int i = 0; i < int(windows.count()); ++i) {
        QC_MDIWindow* m = (QC_MDIWindow*)windows.at(i);
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

	QString edition;
    edition = "";

    QStringList modules;

#ifdef RS_SCRIPTING
    modules += "Scripting";
#endif

    QString modulesString;

    if (modules.empty()==false) {
        modulesString = modules.join(", ");
    } else {
        modulesString = tr("None");
    }

    QMessageBox box(this);
    box.setCaption(tr("About..."));
    box.setText(       QString("<qt>") +  // no center for main stream CADuntu
#ifdef QC_ABOUT_HEADER
                       QString("<center>") + 
                       QString(XSTR(QC_ABOUT_HEADER)) +
#else
                       "<h2>"+ XSTR(QC_APPNAME)+ "</h2>" +
#endif
				tr("Version: %1 %2").arg(XSTR(QC_VERSION)).arg(edition) + "<br>" +
#ifdef QC_SVNREVISION
				tr("SVN Revision: %1").arg(XSTR(QC_SVNREVISION)) + "<br>" +
#endif
                       tr("Date: %1").arg(__DATE__) + "<br>" +
				"(c) 2010 by R. van Twisk<br>"
                       "<br>" +
                       tr("Modules: %1").arg(modulesString) + "<br>" +
                       QString("<a href=\"http://www.caduntu.org\">http://www.caduntu.org</a>")+"<br><br><br>"+
				"<font size=\"1\">Portions (c) by RibbonSoft, Andrew Mustun</font>" 

#ifdef QC_ABOUT_ADD_COMPANY
                       + QString("<br>") + QC_ABOUT_ADD_COMPANY
                       + QString("</center>")
#endif
                       );
#ifndef QC_ABOUT_HEADER
    //RVT_PORT box.setIcon( qPixmapFromMimeSource(QC_APP_ICON) );
    //RVT_PORT box.setFixedWidth(340);
    //RVT_PORT box.setFixedHeight(250);
#endif
    box.exec();
}



/**
 * Menu help -> help.
 */
void QC_ApplicationWindow::slotHelpManual() {
    RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual()");
/*
    if (assistant==NULL) {
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().latin1());
        RS_DEBUG->print("QC_ApplicationWindow::slotHelpManual(): appdir: %s",
                        RS_SYSTEM->getAppDir().latin1());
        assistant = new QAssistantClient(RS_SYSTEM->getAppDir()+"/bin", this);
		connect(assistant, SIGNAL(error(const QString&)), 
			this, SLOT(slotError(const QString&)));
        QStringList args;
        args << "-profile";
        args << QDir::convertSeparators(RS_SYSTEM->getDocPath() + "/caduntudoc.adp");
//        args << QString("doc") + QDir::separator() + QString("caduntudoc.adp");

        assistant->setArguments(args);
    }
    assistant->openAssistant();
    //assistant->showPage("index.html");
 */
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
            RS_String lay = "NULL";
            if (e->getLayer()!=NULL) {
                lay = e->getLayer()->getName();
            }
            dumpFile
            << "<td>Layer: " << lay.ascii() << "</td>"
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
                    << d->getText().latin1()
                    << "</td>"
                    << "<td>Label: "
                    << d->getLabel().latin1()
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
                    << d->getText().ascii()
                    << "</td>"
                    << "<td>Label: "
                    << d->getLabel().ascii()
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

            case RS2::EntityText: {
                    RS_Text* t = (RS_Text*)e;
                    dumpFile
                    << "<table><tr><td>"
                    << "<b>Text:</b>"
                    << "</td></tr>";
                    dumpFile
                    << "<tr>"
                    << "<td>Text:"
                    << t->getText().latin1()
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
                    << h->getPattern().latin1()
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

           //RS_MouseEvent rsm1(posx, posy, LEFT);
        RS_MouseEvent rsm1(QEvent::MouseButtonPress, 
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

               //RS_MouseEvent rsm2(posx, posy, LEFT);
            
            RS_MouseEvent rsm2(QEvent::MouseMove, 
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
void QC_ApplicationWindow::slotTestInsertText() {
    RS_DEBUG->print("QC_ApplicationWindow::slotTestInsertText()");


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
        RS_Char uCode;       // e.g. 65 (or 'A')
        RS_String strCode;   // unicde as string e.g. '[0041] A'

        graphic->setAutoUpdateBorders(false);

        for (col=0x0000; col<=0xFFF0; col+=0x10) {
            printf("col: %X\n", col);
            for (row=0x0; row<=0xF; row++) {
                //printf("  row: %X\n", row);

                uCode = RS_Char(col+row);
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

	 QWidgetList list = workspace->windowList();
	
	while (!list.isEmpty()) {
		QC_MDIWindow *tmp=(QC_MDIWindow*)list.takeFirst();
		succ = tmp->closeMDI(force);
		if (!succ) {
            break;
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

    // timer
    static QTime ts = QTime();
    static QString firstKey = "";

    // single key codes:
    switch (e->key()) {
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

    case Qt::Key_Escape:
        firstKey = "";
        slotBack();
        e->accept();
        break;

    case Qt::Key_Return:
        if (firstKey.isEmpty()) {
            slotEnter();
            e->accept();
        }
        break;

    case Qt::Key_Plus:
        if (firstKey.isEmpty()) {
            actionHandler->slotZoomIn();
            e->accept();
        }
        break;

    case Qt::Key_Minus:
        if (firstKey.isEmpty()) {
            actionHandler->slotZoomOut();
            e->accept();
        }
        break;

    default:
        e->ignore();
        break;
    }

    if (e->isAccepted()) {
        return;
    }

    QTime now = QTime::currentTime();

    // multi key codes:
    if (ts.msecsTo(now)<2000) {
        QString code =
            QString("%1%2").arg(firstKey).arg(QChar(e->key())).lower();

        if (actionHandler->keycode(code)==false) {
            ts = now;
            if (QChar(e->key()).isPrint()) {
                firstKey += e->key();
            }
        }
        else {
            firstKey="";
        }
    }
    else {
        ts = now;
        if (QChar(e->key()).isPrint()) {
            firstKey = e->key();
        }
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


