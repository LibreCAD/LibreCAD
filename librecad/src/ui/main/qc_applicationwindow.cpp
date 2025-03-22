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
#include <QRegularExpression>
#include <QStatusBar>
#include <QStyleFactory>
#include <QSysInfo>
#include <QTimer>
#include <QtSvg>

#include <boost/version.hpp>

#include "doc_plugin_interface.h"
#include "main.h"
#include "twostackedlabels.h"
#include "widgetcreator.h"

#include "rs_actionlibraryinsert.h"
#include "rs_actionprintpreview.h"
#include "rs_commands.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_document.h"
#include "rs_painter.h"
#include "rs_pen.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_selection.h"
#include "rs_units.h"

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "lc_actionsshortcutsdialog.h"
#include "lc_applicationwindowdialogshelper.h"
#include "lc_centralwidget.h"
#include "lc_creatorinvoker.h"
#include "lc_penwizard.h"
#include "qg_librarywidget.h"
#include "lc_printing.h"
#include "lc_widgetfactory.h"
#include "lc_widgetoptionsdialog.h"

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
#include "lc_mdiapplicationwindow.h"
#include "lc_releasechecker.h"
#include "lc_inputtextdialog.h"
#include "lc_menufactory.h"
#include "lc_plugininvoker.h"
#include "lc_printviewportrenderer.h"

#ifndef QC_APP_ICON
# define QC_APP_ICON ":/images/librecad.png"
#endif
#ifndef QC_ABOUT_ICON
# define QC_ABOUT_ICON ":/images/intro_librecad.png"
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
QC_ApplicationWindow::QC_ApplicationWindow():
     m_actionHandler(new QG_ActionHandler(this))
    , m_penWizard(new LC_PenWizard(QObject::tr("Pen Wizard"), this))
{
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow");

    m_actionGroupManager = new LC_ActionGroupManager(this);
    connect(RS_SETTINGS, &RS_Settings::optionsChanged, m_actionGroupManager, &LC_ActionGroupManager::onOptionsChanged);

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

    m_penWizard->setObjectName("pen_wiz");
    connect(this, &QC_ApplicationWindow::windowsChanged,
            m_penWizard, &LC_PenWizard::setEnabled);
    addDockWidget(Qt::RightDockWidgetArea, m_penWizard);

    LC_ActionFactory a_factory(this, m_actionHandler);
    bool using_theme = settings.value("Widgets/AllowTheme", 0).toBool();
    a_factory.fillActionContainer(m_actionGroupManager, using_theme);

    LC_WidgetFactory widget_factory(this, m_actionGroupManager);

    widget_factory.initStatusBar();

    bool statusBarVisible = LC_GET_ONE_BOOL("Appearance", "StatusBarVisible", true);
    statusBar()->setVisible(statusBarVisible);


    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating LC_CentralWidget");

    auto central = new LC_CentralWidget(this);

    setCentralWidget(central);

    mdiAreaCAD = central->getMdiArea();
    mdiAreaCAD->setDocumentMode(true);

    LC_GROUP("WindowOptions");
    setTabLayout(static_cast<RS2::TabShape>(LC_GET_INT("TabShape", RS2::Triangular)),
                 static_cast<RS2::TabPosition>(LC_GET_INT("TabPosition", RS2::West)));
    LC_GROUP_END();

    settings.beginGroup("Startup");
    if (settings.value("TabMode", 0).toBool()) {
        setupCADAreaTabbar();
    }

    bool enable_left_sidebar = settings.value("EnableLeftSidebar", 1).toBool();
    bool enable_cad_toolbars = settings.value("EnableCADToolbars", 1).toBool();
    settings.endGroup();

    connect(mdiAreaCAD, &QMdiArea::subWindowActivated,this, &QC_ApplicationWindow::slotWindowActivated);

    LC_GROUP("Widgets");
    {
        bool custom_size = LC_GET_BOOL("AllowToolbarIconSize", false);
        int icon_size = custom_size ? LC_GET_INT("ToolbarIconSize", 24) : 24;

        if (custom_size) {
            setIconSize(QSize(icon_size, icon_size));
        }

        if (enable_left_sidebar){
            int leftSidebarColumnsCount = settings.value("Widgets/LeftToolbarColumnsCount", 5).toInt();
            int leftSidebarIconSize = settings.value("Widgets/LeftToolbarIconSize", 24).toInt();
            bool flatIcons = settings.value("Widgets/LeftToolbarFlatIcons", 24).toInt();
            widget_factory.createLeftSidebar(leftSidebarColumnsCount, leftSidebarIconSize, flatIcons);
        }
        if (enable_cad_toolbars) {
            widget_factory.createCADToolbars();
        }
        widget_factory.createRightSidebar(m_actionHandler);
        widget_factory.createCategoriesToolbar();
        widget_factory.createStandardToolbars(m_actionHandler);
    }
    LC_GROUP_END();

    m_creatorInvoker = new LC_CreatorInvoker(this, m_actionGroupManager);
    m_creatorInvoker->createCustomToolbars();

    m_dlgHelpr = new LC_ApplicationWindowDialogsHelper(this);

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
        foreach (auto& actionName, list){
            toolbar->addAction(getAction(actionName));
        }
        settings.setValue("CustomToolbars/DefaultCustom", list);
        addToolBar(Qt::LeftToolBarArea, toolbar);
    }

    m_menuFactory = new LC_MenuFactory(this, m_actionGroupManager);
    m_menuFactory->createMainMenu(menuBar());

    m_dockAreas.left = getAction("LeftDockAreaToggle");
    m_dockAreas.right = getAction("RightDockAreaToggle");
    m_dockAreas.top = getAction("TopDockAreaToggle");
    m_dockAreas.bottom = getAction("BottomDockAreaToggle");
    m_dockAreas.floating = getAction("FloatingDockwidgetsToggle");

    m_snapToolBar = widget_factory.snap_toolbar;
    m_penToolBar = widget_factory.pen_toolbar;
    m_optionWidget = widget_factory.options_toolbar;

    m_layerWidget = widget_factory.layer_widget;

    m_layerTreeWidget = widget_factory.layer_tree_widget;

    m_quickInfoWidget = widget_factory.quick_info_widget;

    m_libraryWidget = widget_factory.library_widget;
    /**/
    m_blockWidget = widget_factory.block_widget;
    m_commandWidget = widget_factory.command_widget;

    m_penPaletteWidget = widget_factory.pen_palette;
    m_namedViewsWidget = widget_factory.named_views_widget;
    m_ucsListWidget = widget_factory.ucs_widget;


    connect(m_namedViewsWidget, &LC_NamedViewsListWidget::viewListChanged, [this](int itemsCount){
        enableAction("ZoomViewRestore1",itemsCount > 0);
        enableAction("ZoomViewRestore2",itemsCount > 1);
        enableAction("ZoomViewRestore3",itemsCount > 2);
        enableAction("ZoomViewRestore4",itemsCount > 3);
        enableAction("ZoomViewRestore5",itemsCount > 4);
    });

    m_actionsToDisableInPrintPreviewList = widget_factory.actionsToDisableInPrintPreview;

    connect(getAction("FileClose"), &QAction::triggered, mdiAreaCAD, &QMdiArea::closeActiveSubWindow);

    connect(m_penToolBar, &QG_PenToolBar::penChanged, this, &QC_ApplicationWindow::slotPenChanged);

    // fixme - sand - remove hardcoded shortcuts!!!
    // fixme - review the entire keyboard support
    auto ctrl_l = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(ctrl_l, SIGNAL(activated()), m_actionHandler, SLOT(slotLayersAdd()));

    auto ctrl_m = new QShortcut(QKeySequence("Ctrl+M"), this);
    connect(ctrl_m, SIGNAL(activated()), this, SLOT(slotFocusCommandLine()));

    // This event filter allows sending key events to the command widget, therefore, no
    // need to activate the command widget before typing commands.
    // Since this nice feature causes a bug of lost key events when the command widget is on
    // a screen different from the main window, disabled for the time being
    // send key events for mdiAreaCAD to command widget by default
    mdiAreaCAD->installEventFilter(m_commandWidget);

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating dialogFactory");
    LC_SnapOptionsWidgetsHolder *snapOptionsHolder = nullptr;
    snapOptionsHolder = m_snapToolBar->getSnapOptionsHolder();
    m_dialogFactory = new QC_DialogFactory(this, m_optionWidget, snapOptionsHolder);
    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: creating dialogFactory: OK");

    RS_DEBUG->print("setting dialog factory object");
    if (RS_DialogFactory::instance()==nullptr) {
        RS_DEBUG->print("no RS_DialogFactory instance");
    } else {
        RS_DEBUG->print("got RS_DialogFactory instance");
    }
    RS_DialogFactory::instance()->setFactoryObject(m_dialogFactory);
    RS_DEBUG->print("setting dialog factory object: OK");

    m_recentFilesList = new QG_RecentFiles(this, 9);
    m_recentFilesList->addFiles(m_menuFactory->getRecentFilesMenu());

    RS_DEBUG->print("QC_ApplicationWindow::QC_ApplicationWindow: init settings");
    initSettings();

    auto command_file = settings.value("Paths/VariableFile", "").toString();
    if (!command_file.isEmpty())
        m_commandWidget->leCommand->readCommandFile(command_file);

    // Activate autosave timer
    bool allowAutoSave = settings.value("Defaults/AutoBackupDocument", 1).toBool();
    startAutoSaveTimer(allowAutoSave);

    // Disable menu and toolbar items
    //emit windowsChanged(false);

    RS_COMMANDS->updateAlias();
    //plugin load
    m_pluginInvoker = new LC_PluginInvoker(this);
    m_pluginInvoker->loadPlugins();

    showStatusMessage(qApp->applicationName() + " Ready", 2000);
    const char *ownBuildVersion = XSTR(LC_VERSION);
    m_releaseChecker = new LC_ReleaseChecker( ownBuildVersion,XSTR(LC_PRERELEASE));
    connect(m_releaseChecker, &LC_ReleaseChecker::updatesAvailable, this, &QC_ApplicationWindow::onNewVersionAvailable);
}

void QC_ApplicationWindow::checkForNewVersion() {
    m_releaseChecker->checkForNewVersion();
}

void QC_ApplicationWindow::forceCheckForNewVersion() {
    m_releaseChecker->checkForNewVersion(true);
}

void QC_ApplicationWindow::onNewVersionAvailable() {
   m_dlgHelpr->showNewVersionAvailableDialog(m_releaseChecker);
}

void QC_ApplicationWindow::startAutoSaveTimer(bool startAutoBackup) {
    if (startAutoBackup) {
        if (m_autosaveTimer == nullptr) {
            m_autosaveTimer = std::make_unique<QTimer>(this);
            m_autosaveTimer->setObjectName("autosave");
            connect(m_autosaveTimer.get(), &QTimer::timeout, this, &QC_ApplicationWindow::autoSaveCurrentDrawing);
        }
        if (!m_autosaveTimer->isActive()) {
            // autosaving has been turned on. Make a backup immediately
            LC_GROUP_GUARD("Defaults");
            {
                LC_SET("AutoBackupDocument", 1);
                autoSaveCurrentDrawing();
                int ms = 60000 * LC_GET_INT("AutoSaveTime", 5);
                m_autosaveTimer->start(ms);
            }
        }
    } else {
        if (m_autosaveTimer != nullptr) {
            m_autosaveTimer.reset();
        }
    }
}

/**
 * @brief QC_ApplicationWindow::getAppWindow() accessor for the application window singleton instance
 * @return QC_ApplicationWindow* the application window instance
 */
std::unique_ptr<QC_ApplicationWindow>& QC_ApplicationWindow::getAppWindow(){
    static auto instance = std::unique_ptr<QC_ApplicationWindow>(new QC_ApplicationWindow);
    // singleton could be reset: cannot be called after reseting
    Q_ASSERT(instance != nullptr);
    return instance;
}

void QC_ApplicationWindow::setupMDIWindowTitleByFile(QC_MDIWindow *w, QString drawingFileFullPath, bool draftMode){
    setupMDIWindowTitleByName(w, getFileNameFromFullPath(drawingFileFullPath), draftMode);
}

void QC_ApplicationWindow::setupMDIWindowTitleByName(QC_MDIWindow *w, QString baseTitleStr, bool draftMode){
    auto title = baseTitleStr + "[*]";
    if (draftMode) {
        title = title + " [" + tr("Draft Mode") + "]";
    }
    w->setWindowTitle(title);
}

/**
 * Force-Save(as) the content of the sub window.  Retry on failure.
 * @return true success (or window was not modified)
 * @return false user cancelled (or window was null)
 */
bool QC_ApplicationWindow::doSave(QC_MDIWindow * w, bool forceSaveAs) {
    QString drawingFileFullPath, msg;
    bool cancelled;
    if (!w) {
        return false;
    }
    if (w->getDocument()->isModified() || forceSaveAs) {
        drawingFileFullPath = w->getFileName();
        if (drawingFileFullPath.isEmpty()) {
            doActivate(w); // show the user the drawing for save as
        }
        msg = drawingFileFullPath.isEmpty() ? tr("Saving drawing...") : tr("Saving drawing: %1").arg(drawingFileFullPath);
        showStatusMessage(msg);
        bool res = forceSaveAs ? w->saveDocumentAs(cancelled) : w->saveDocument(cancelled);
        if (res) {
            if (cancelled) {
                showStatusMessage(tr("Save cancelled"), 2000);
                return false;
            }
            drawingFileFullPath = w->getFileName();
            msg = tr("Saved drawing: %1").arg(drawingFileFullPath);
            showStatusMessage(msg, 2000);
            m_commandWidget->appendHistory(msg);

            if (!m_recentFilesList->contains(drawingFileFullPath)) {
                m_recentFilesList->add(drawingFileFullPath);
            }

            bool draftMode = w->getGraphicView()->isDraftMode();
            setupMDIWindowTitleByFile(w, drawingFileFullPath, draftMode);

            bool autoBackup = LC_GET_ONE_BOOL("Defaults", "AutoBackupDocument", true);
            startAutoSaveTimer(autoBackup);
        } else {
            msg = tr("Cannot save the file ") +  w->getFileName()
                  + tr(" , please check the filename and permissions.");
            showStatusMessage(msg, 2000);
            m_commandWidget->appendHistory(msg);
            return doSave(w, true);
        }
    }
    return true;
}

/**
 * Force-Close this sub window.
 * @param activateNext also activate the next window in the window_list, if any
 */
void QC_ApplicationWindow::doClose(QC_MDIWindow *w, bool activateNext) {
    RS_DEBUG->print("QC_ApplicationWindow::doClose begin");
    w->getGraphicView()->killAllActions();
    QC_MDIWindow *parentWindow = w->getParentWindow();
    if (parentWindow) {
        RS_DEBUG->print("QC_ApplicationWindow::doClose closing block or print preview");
        parentWindow->removeChildWindow(w);
    } else {
        RS_DEBUG->print("QC_ApplicationWindow::doClose closing graphic");
    }
    for (auto &&child : std::as_const(w->getChildWindows())) {// block editors and print previews; just force these closed
        doClose(child, false); // they belong to the document (changes already saved there)
    }
    w->getChildWindows().clear();
    mdiAreaCAD->removeSubWindow(w);
    window_list.removeOne(w);

    if (!activedMdiSubWindow || activedMdiSubWindow == w) {
        setupWidgetsByWindow(nullptr);
    }

    openedFiles.removeAll(w->getFileName());

    activedMdiSubWindow = nullptr;
    m_actionHandler->setDocumentAndView(nullptr, nullptr);

    if (activateNext && !window_list.empty()) {
        if (nullptr != parentWindow) {
            doActivate(parentWindow);
        } else {
            doActivate(window_list.back());
        }
    }

    RS_DEBUG->print("QC_ApplicationWindow::doClose end");
}

void QC_ApplicationWindow::setupWidgetsByWindow(QC_MDIWindow *w){
    RS_Document* doc = nullptr;
    RS_GraphicView* gv = nullptr;

    if (w != nullptr) {
        doc = w->getDocument();
        gv = w->getGraphicView();
    }
    m_layerWidget->setDocumentAndView(doc, gv);

    if (m_layerTreeWidget != nullptr) {
        m_layerTreeWidget->setDocumentAndView(doc, gv);
    }
    if (m_namedViewsWidget != nullptr){
        m_namedViewsWidget->setGraphicView(gv, w);
    }
    if (m_ucsListWidget != nullptr){
        m_ucsListWidget->setGraphicView(gv, w);
    }
    if (m_quickInfoWidget != nullptr) {
        m_quickInfoWidget->setDocumentAndView(doc, gv);
    }
    if (m_anglesBasisWidget != nullptr){
        m_anglesBasisWidget->update(nullptr);
    }
    if (m_penPaletteWidget != nullptr) {
        m_penPaletteWidget->setDocumentAndView(doc, gv);
        m_penPaletteWidget->setMdiWindow(w);
    }
    m_blockWidget->setDocument(doc);
    m_coordinateWidget->setGraphic(nullptr, gv);
    m_relativeZeroCoordinatesWidget->setGraphicView(gv);
}

/**
 * Force-Activate this sub window.
 */
void QC_ApplicationWindow::doActivate(QMdiSubWindow *w) {
    LC_MDIApplicationWindow::doActivate(w);
    enableFileActions(qobject_cast<QC_MDIWindow *>(w));
}

/**
 * Show a Save/Close/Cancel(All) dialog for the content of this sub-window.
 * The window handle must not be null, and the document must actually have been modified.
 *
 * @param showSaveAll show a Save All button and rename Close -> Close All
 * @return QG_ExitDialog::ExitDialogResult the button that was pressed, or -1 if invoked in error
 * @see QG_ExitDialog
 */
int QC_ApplicationWindow::showCloseDialog(QC_MDIWindow *w, bool showSaveAll) {
    QG_ExitDialog dlg(this);
    dlg.setShowSaveAll(showSaveAll);
    dlg.setTitle(tr("Closing Drawing"));
    if (w != nullptr && w->getDocument()->isModified()) {
        QString fn = w->getFileName();
        if (fn.isEmpty()) {
            fn = w->windowTitle();
        }
        else if (fn.length() > 50) {
            fn = QString("%1...%2").arg(fn.left(24)).arg(fn.right(24));
        }

        dlg.setText(tr("Save changes to the following item?\n%1").arg(fn));
        return dlg.exec();
    }
    return -1; // should never get here; please send only modified documents
}

/**
 * Enable the available file actions for this sub-window.
 */
void QC_ApplicationWindow::enableFileActions(QC_MDIWindow *w) {
    bool hasWindow = w != nullptr;
    auto fileName = w->getFileName();
    if (!hasWindow || fileName.isEmpty()) {
        getAction("FileSave")->setText(tr("&Save"));
        getAction("FileSaveAs")->setText(tr("Save &as..."));
    } else {
        QString name = getFileNameFromFullPath(fileName);
        getAction("FileSave")->setText(tr("&Save %1").arg(name));
        getAction("FileSaveAs")->setText(tr("Save %1 &as...").arg(name));
    }

    enableActions({
            "FileSave",
            "FileSaveAs",
            "FileExportMakerCam",
            "FilePrintPDF",
            "FileExport",
            "FilePrint",
            "FilePrintPreview",
            "FileClose"
        },hasWindow);

    enableActions({
        "FileSaveAll",
        "FileCloseAll"
        },hasWindow && window_list.count() > 1);
}

/**
 * Loads the found plugins.
 */
void QC_ApplicationWindow::loadPlugins() {
   m_pluginInvoker->loadPlugins();
}

/**
 * Destructor.
 */
QC_ApplicationWindow::~QC_ApplicationWindow() {
    RS_DEBUG->print("QC_ApplicationWindow::~QC_ApplicationWindow");

#ifdef _WINDOWS
	qt_ntfs_permission_lookup--; // turn it off again
#endif

    delete m_dialogFactory;
    delete m_pluginInvoker;
    delete m_creatorInvoker;
    delete m_dlgHelpr;
}


/**
 * Close Event. Called when the user tries to close the app.
 */
void QC_ApplicationWindow::closeEvent(QCloseEvent *ce) {
    RS_DEBUG->print("QC_ApplicationWindow::closeEvent()");

    queryMayExit() ? ce->accept() : ce->ignore();

    RS_DEBUG->print("QC_ApplicationWindow::closeEvent(): OK");
}

void QC_ApplicationWindow::dropEvent(QDropEvent *event) {
    event->acceptProposedAction();

    //limit maximum number of dropped files to be opened
    unsigned counts = 0;
    for (QUrl const &url: event->mimeData()->urls()) {
        const QString &fileName = url.toLocalFile();
        if (QFileInfo(fileName).exists() && fileName.endsWith(R"(.dxf)", Qt::CaseInsensitive)) {
            slotFileOpen(fileName);
            if (++counts > 32) return;
        }
    }
}

void QC_ApplicationWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        for (QUrl const &url: event->mimeData()->urls()) {
            const QString &fileName = url.toLocalFile();
            if (QFileInfo(fileName).exists() && fileName.endsWith(R"(.dxf)", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void QC_ApplicationWindow::setPreviousZoomEnable(bool enable){
    m_previousZoomEnable=enable;
    enableAction("ZoomPrevious", enable);
}

void QC_ApplicationWindow::setUndoEnable(bool enable){
    m_undoEnable = enable;
    enableAction("EditUndo", enable);
}

void QC_ApplicationWindow::setRedoEnable(bool enable){
    m_redoEnable = enable;
    enableAction("EditRedo", enable);
}

QAction* QC_ApplicationWindow::enableAction(const QString& name, bool enable) const{
    QAction* action = getAction(name);
    if (action != nullptr) {
        action->setEnabled(enable);
    }
    return action;
}

QAction* QC_ApplicationWindow::checkAction(const QString& name, bool enable) const{
    QAction* action = getAction(name);
    if (action != nullptr) {
        action->setChecked(enable);
    }
    return action;
}

void QC_ApplicationWindow::checkActions(const std::vector<QString> &actionList, bool enable) const {
    for (const QString &a: actionList){
        checkAction(a, enable);
    }
}

void QC_ApplicationWindow::enableActions(const std::vector<QString> &actionList, bool enable) const {
    for (const QString &a: actionList){
        enableAction(a, enable);
    }
}

void QC_ApplicationWindow::setSaveEnable(bool enable){
    enableAction("FileSave", enable);
}


void QC_ApplicationWindow::slotEnableActions(bool enable) {
    enableAction("ZoomPrevious", enable && m_previousZoomEnable);
    enableAction("EditUndo", enable && m_undoEnable);
    enableAction("EditRedo", enable && m_redoEnable);
}

void QC_ApplicationWindow::slotUpdateActiveLayer() {
    if (m_layerWidget && m_activeLayerName) {
        m_activeLayerName->activeLayerChanged(m_layerWidget->getActiveName());
    }
}

/**
 * Initializes the global application settings from the
 * config file (unix, mac) or registry (windows).
 */
void QC_ApplicationWindow::initSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::initSettings()");
    QSettings settings;

    bool first_load = settings.value("Startup/FirstLoad", 1).toBool();
    if (!first_load) {
        m_workspacesManager.init(this);
    }
    fireWorkspacesChanged();


    LC_GROUP("Widgets");
    {
        bool allow_style = LC_GET_BOOL("AllowStyle", false);
        if (allow_style) {
            QString style = LC_GET_STR("Style", "");
            QApplication::setStyle(QStyleFactory::create(style));
        }

        QString sheet_path = LC_GET_STR("StyleSheet", "");
        if (loadStyleSheet(sheet_path)) {
            m_styleSheetPath = sheet_path;
        }
    }
    LC_GROUP("Appearance");
    {
        QAction *viewLinesDraftAction = getAction("ViewLinesDraft");
        viewLinesDraftAction->setChecked(LC_GET_BOOL("DraftLinesMode", false));

        bool draftMode = LC_GET_BOOL("DraftMode", false);

        getAction("ViewDraft")->setChecked(draftMode);
        viewLinesDraftAction->setDisabled(draftMode);

        QAction* viewAntialiasing = getAction("ViewAntialiasing");
        bool antialiasing = LC_GET_BOOL("Antialiasing", false);
        viewAntialiasing->setChecked(antialiasing);
    }
    LC_GROUP_END();
}


/**
 * Stores the global application settings to file or registry.
 */
void QC_ApplicationWindow::storeSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::storeSettings()");

    if (RS_Settings::save_is_allowed) {
       m_workspacesManager.persist();
        //save snapMode
       m_snapToolBar->saveSnapMode();
    }

    RS_DEBUG->print("QC_ApplicationWindow::storeSettings(): OK");
}

/**
 * Goes back to the previous menu or one step in the current action.
 */
void QC_ApplicationWindow::slotBack() {
    RS_GraphicView* graphicView = getCurrentGraphicView();
    if (graphicView) {
        graphicView->back();
    }
}

void QC_ApplicationWindow::slotKillAllActions() {
    RS_GraphicView* gv = getCurrentGraphicView();
    QC_MDIWindow* m = getCurrentMDIWindow();
    if (gv && m && m->getDocument()) {
        gv->killAllActions();

        RS_Selection s((RS_EntityContainer&)*m->getDocument(), gv->getViewPort());
        s.selectAll(false);
        RS_DIALOGFACTORY->updateSelectionWidget(m->getDocument()->countSelected(),m->getDocument()->totalSelectedLength());
        gv->redraw(RS2::RedrawAll);
    }
}

/**
 * Goes one step further in the current action.
 */
void QC_ApplicationWindow::slotEnter() {
    RS_DEBUG->print("QC_ApplicationWindow::slotEnter(): begin\n");
    RS_GraphicView *graphicView = getCurrentGraphicView();
    if (graphicView) {
        graphicView->enter();
    }
    RS_DEBUG->print("QC_ApplicationWindow::slotEnter(): end\n");
}

/**
 * Sets the keyboard focus on the command line.
 */
void QC_ApplicationWindow::slotFocusCommandLine() {
//    if (commandWidget->isVisible()) {
    m_commandWidget->show();
    m_commandWidget->setFocus();
//    }
}

void QC_ApplicationWindow::slotFocusOptionsWidget(){
    if (m_optionWidget != nullptr){
        m_optionWidget->setFocus();
    }
}

/**
 * Shows the given error on the command line.
 */
void QC_ApplicationWindow::slotError(const QString& msg) {
  m_commandWidget->appendHistory(msg);
}

void QC_ApplicationWindow::slotShowDrawingOptions() {
    m_actionHandler->setCurrentAction(RS2::ActionOptionsDrawingGrid);
}

void QC_ApplicationWindow::slotShowDrawingOptionsUnits() {
    m_actionHandler->setCurrentAction(RS2::ActionOptionsDrawingUnits);
}

/**
 * Hands focus back to the application window. In the rare event
 * of a escape press from the layer widget (e.g after switching desktops
 * in XP).
 */
void QC_ApplicationWindow::slotFocus() {
    setFocus();
}


/**
 * Called when a document window was activated.
 */



void QC_ApplicationWindow::doSlotWindowActivated(QMdiSubWindow *w, bool forced) {
    RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated begin");

    if (w == nullptr) {
        enableWidgets(false);
        enableWidget(m_layerTreeWidget, false);
        enableWidget(m_layerWidget, false);
        enableWidget(m_commandWidget, false);
        RS_DIALOGFACTORY->hideSnapOptions();
        m_coordinateWidget->clearContent();
        m_relativeZeroCoordinatesWidget->clearContent();
        // todo - check which other widgets in status bar or so should be cleared if no files..
        emit windowsChanged(false);
        activedMdiSubWindow = w;
        return;
    }

    if (w == activedMdiSubWindow) {
        // this may occur after file open, so additional update is needed :(
        RS_GraphicView* activatedGraphicView = getCurrentGraphicView();
        if (activatedGraphicView == nullptr || activatedGraphicView->getGraphic() == nullptr)
            return;

        RS_Graphic* activatedGraphic = activatedGraphicView->getGraphic();

        bool printPreview = activatedGraphicView->isPrintPreview();
        if (!printPreview){
            bool isometricGrid = activatedGraphic->isIsometricGrid();
            RS2::IsoGridViewType isoViewType = activatedGraphic->getIsoView();
            updateGridViewActions(isometricGrid, isoViewType);
        }
        activatedGraphicView->loadSettings();
        activatedGraphicView->redraw();
        return;
    }

// kill active actions in previous windows.that will prevent the situation described by issue #1762 with
// non-finished action started on previous window and action that is active with UI still checked  after window switch
        foreach (QMdiSubWindow *sw, mdiAreaCAD->subWindowList()) {
            auto *sm = dynamic_cast<QC_MDIWindow *>(sw);
            QG_GraphicView *graphicView = sm->getGraphicView();
            if (graphicView != nullptr) {
                RS_ActionInterface *ai = graphicView->getCurrentAction();
                if (ai != nullptr) {
                    ai->hideOptions();
                    ai->hideSnapOptions();
                    // actually, this is a "brute force" approach for now.
                    // todo - more intelligent approach is for sure to uncheck the action (for action in progress in  old
                    // todo window without killing action and restore the action toggle state on return to that view.
                    // todo hawever, it seems that will require some additional relation between action an QAction and
                    // todo support from ActionHandler
                    // fixme - return to this later
                    graphicView->killAllActions();
                }
            }
        }

    activedMdiSubWindow = w;

    auto *windowActivated = dynamic_cast<QC_MDIWindow *>(w);
    enableFileActions(windowActivated);

    QG_GraphicView *activatedGraphicView = nullptr;

    bool hasDocumentInActivatedWindow = false;

    RS_Document *activatedDocument = windowActivated->getDocument();
    if (activatedDocument != nullptr) {

        hasDocumentInActivatedWindow = true;

        activatedGraphicView = windowActivated->getGraphicView();

        activatedGraphicView->loadSettings();

        RS_Graphic *activatedGraphic = windowActivated->getGraphic();

        RS_Units::setCurrentDrawingUnits(activatedDocument->getGraphic()->getUnit());

        RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated: "
                        "document: %lu", activatedDocument->getId());

        setupWidgetsByWindow(windowActivated);

        // Update all inserts in this graphic (blocks might have changed):
        activatedDocument->updateInserts();
        // whether to enable undo/redo buttons
        activatedDocument->updateUndoState();

        QAction *lockRelZeroAction = m_actionGroupManager->getActionByName("LockRelativeZero");
        if (lockRelZeroAction != nullptr){
            bool locked = activatedGraphicView->getViewPort()->isRelativeZeroLocked();
            lockRelZeroAction->setChecked(locked);
        }

        if (activatedGraphicView != nullptr) {
            activatedGraphicView->redraw();
        }

        // set snapmode from snap toolbar
        //actionHandler->updateSnapMode();

        // set pen from pen toolbar
        slotPenChanged(m_penToolBar->getPen());
/// fixme - setup too in setupWidgetsByWindow???
        m_penWizard->setMdiWindow(windowActivated);


        if (!forced) {
            // update toggle button status:
            emit gridChanged(activatedGraphic->isGridOn());
        }
        bool printPreview = false;
        m_actionHandler->setDocumentAndView(activatedDocument, activatedGraphicView);
        if (activatedGraphicView != nullptr) {
            printPreview = activatedGraphicView->isPrintPreview();
            RS_ActionInterface *currentAction = activatedGraphicView->getCurrentAction();
            if (currentAction != nullptr) {
                currentAction->showOptions();
            }
        }

        if (!printPreview){
            bool isometricGrid = activatedGraphic->isIsometricGrid();
            RS2::IsoGridViewType isoViewType = activatedGraphic->getIsoView();
            updateGridViewActions(isometricGrid, isoViewType);
        }

        updateActionsAndWidgetsForPrintPreview(printPreview);

        if (m_snapToolBar) {
            if (!printPreview) {
                m_actionHandler->slotSetSnaps(m_snapToolBar->getSnaps());
            }
        } else {
            RS_DEBUG->print(RS_Debug::D_ERROR, "snapToolBar is nullptr\n");
        }
    }

    // Disable/Enable menu and toolbar items
    emit windowsChanged(hasDocumentInActivatedWindow);

    RS_DEBUG->print("RVT_PORT emit windowsChanged(true);");

    RS_DEBUG->print("QC_ApplicationWindow::slotWindowActivated end");
}

/**
 * Called when the menu 'windows' is about to be shown.
 * This is used to update the window list in the menu.
 */
void QC_ApplicationWindow::slotWorkspacesMenuAboutToShow() {
    RS_DEBUG->print(RS_Debug::D_NOTICE, "QC_ApplicationWindow::slotWorkspacesMenuAboutToShow");
    m_menuFactory->onWorkspaceMenuAboutToShow(window_list);
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
 * Called when something changed in the pen tool bar
 * (e.g. color, width, style).
 */
void QC_ApplicationWindow::slotPenChanged(RS_Pen pen) {
    RS_DEBUG->print("QC_ApplicationWindow::slotPenChanged() begin");
    RS_DEBUG->print("Setting active pen...");
    QC_MDIWindow *m = getCurrentMDIWindow();
    if (m) {
        m->slotPenChanged(pen);
    }
    RS_DEBUG->print("QC_ApplicationWindow::slotPenChanged() end");
}

/**
 * Creates a new MDI window with the given document or a new
 *  document if 'doc' is nullptr.
 */

QC_MDIWindow *QC_ApplicationWindow::slotFileNew(RS_Document *doc) {

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNew() begin");

    QSettings settings;
    static int id = 0;
    id++;

    showStatusMessage(tr("Creating new file..."));

    RS_DEBUG->print("  creating MDI window");

    auto *w = new QC_MDIWindow(doc, mdiAreaCAD, false);

    window_list << w;

    LC_GROUP("Appearance"); // fixme - settings
    bool aa = LC_GET_BOOL("Antialiasing");
    bool scrollbars = LC_GET_BOOL("ScrollBars", true);
    bool cursor_hiding = LC_GET_BOOL("cursor_hiding");
    LC_GROUP_END();

    QG_GraphicView *view = w->getGraphicView();

    view->setAntialiasing(aa);
    view->setCursorHiding(cursor_hiding);
    view->device = settings.value("Hardware/Device", "Mouse").toString();
    if (scrollbars) {
        view->addScrollbars();
    }

    settings.beginGroup("Activators");
    auto activators = settings.childKeys();
    settings.endGroup();
    // fixme - settings
        for (auto activator: activators) {
            auto menu_name = settings.value("Activators/" + activator).toString();
            auto path = QString("CustomMenus/%1").arg(menu_name);
            auto a_list = settings.value(path).toStringList();
            auto menu = new QMenu(activator, view);
            menu->setObjectName(menu_name);
                foreach (auto key, a_list) {
                    menu->addAction(getAction(key));
                }
            view->setMenu(activator, menu);
        }

    connect(view, &QG_GraphicView::gridStatusChanged, this, &QC_ApplicationWindow::updateGridStatus);

    m_actionHandler->setDocumentAndView(w->getDocument(), view);

    QString baseTitleString;
    if (w->getDocument()->rtti() == RS2::EntityBlock) {
        baseTitleString = tr("Block '%1'").arg(((RS_Block *) (w->getDocument()))->getName()) + "[*]";
    } else {
        baseTitleString = tr("unnamed document %1").arg(id) + "[*]";
    }

    bool draftMode = LC_GET_ONE_BOOL("Appearance", "DraftMode", false);
    setupMDIWindowTitleByName(w, baseTitleString, draftMode);

    w->setWindowIcon(QIcon(":/icons/document.lci"));

    RS_Graphic *graphic = w->getDocument()->getGraphic();

    setupWidgetsByWindow(w);

    // fixme - sand - review and complete initialization. check why we check for graphic, when it might be null, and how that affects init

    if (graphic) {
        // Link the graphic's layer list to the pen tool bar
        graphic->addLayerListListener(m_penToolBar);
        // Link the layer list to the layer widget
        graphic->addLayerListListener(m_layerWidget);

        if (m_layerTreeWidget != nullptr) {
            graphic->addLayerListListener(m_layerTreeWidget);
        }

        // Link the block list to the block widget
        graphic->addBlockListListener(m_blockWidget);

        if (m_namedViewsWidget != nullptr){
            m_namedViewsWidget->setGraphicView(view, w);
        }

        if (m_ucsListWidget != nullptr){
            m_ucsListWidget->setGraphicView(view, w);
        }

        if (m_anglesBasisWidget != nullptr){
            m_anglesBasisWidget->update(graphic);
        }
    }

// fixme - sand - is it really necessary to do this each time?
// Link the dialog factory to the mouse widget:
    QG_DIALOGFACTORY->setMouseWidget(m_mouseWidget);
    QG_DIALOGFACTORY->setCoordinateWidget(m_coordinateWidget);
    QG_DIALOGFACTORY->setRelativeZeroCoordinatesWidget(m_relativeZeroCoordinatesWidget);
    QG_DIALOGFACTORY->setSelectionWidget(m_selectionWidget);
// Link the dialog factory to the option widget:
//QG_DIALOGFACTORY->setOptionWidget(optionWidget);
// Link the dialog factory to the command widget:
    QG_DIALOGFACTORY->setCommandWidget(m_commandWidget);
    QG_DIALOGFACTORY->setStatusBarManager(m_statusbarManager);

    mdiAreaCAD->addSubWindow(w);

    RS_DEBUG->print("  showing MDI window");
    doActivate(w);
    doArrangeWindows(RS2::CurrentMode);
    showStatusMessage(tr("New Drawing created."), 2000);

    m_layerWidget->activateLayer(0);

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNew() OK");

    return w;
}

/**
 * Helper function for Menu file -> New & New....
 */
bool QC_ApplicationWindow::slotFileNewHelper(QString fileName, QC_MDIWindow *w) {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper()");
    bool ret = false;
    RS2::FormatType type = RS2::FormatDXFRW;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper: creating new doc window");
    /*QC_MDIWindow* */ w = slotFileNew();
    qApp->processEvents(QEventLoop::AllEvents, 1000);

    setupWidgetsByWindow(w);

    qApp->processEvents(QEventLoop::AllEvents, 1000);

    // loads the template file in the new view:
    if (!fileName.isEmpty()) {
        ret = w->loadDocumentFromTemplate(fileName, type);
    } else
        //new without template is OK;
        ret = true;

    if (!ret) {
        // error loading template
        QApplication::restoreOverrideCursor();
        return ret;
    }

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewHelper: load Template: OK");

    m_layerWidget->slotUpdateLayerList();
    if (m_layerTreeWidget != nullptr) {
        m_layerTreeWidget->slotFilteringMaskChanged();
    }
    if (m_namedViewsWidget != nullptr){
        m_namedViewsWidget->reload();
    }
    if (m_ucsListWidget != nullptr){
        m_ucsListWidget->reload();
    }

    updateCoordinateWidgetFormat();

    if (!fileName.isEmpty()) {
        QString message = tr("New document from template: ") + fileName;
        m_commandWidget->appendHistory(message);
        showStatusMessage(message, 2000);
    }
    auto graphic = w->getGraphic();
    if (graphic) {
        emit(gridChanged(graphic->isGridOn()));
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
    //tried to load template file indicated in RS_Settings
    QString fileName = LC_GET_ONE_STR("Paths","Template", "");
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
        showStatusMessage(tr("Select Template aborted"), 2000);
        return;
    }

    RS_DEBUG->print("QC_ApplicationWindow::slotFileNewTemplate: creating new doc window");
    // Create new document window:
    QMdiSubWindow *old = activedMdiSubWindow;
    QRect geo;
    bool maximized = false;
    if (old != nullptr) {//save old geometry
        geo = old->geometry();
        maximized = old->isMaximized();
    }
    QC_MDIWindow *w = nullptr;
    if (!slotFileNewHelper(fileName, w)) {
        // error
        QString msg = tr("Cannot open the file\n%1\nPlease "
                         "check the permissions.").arg(fileName);
        m_commandWidget->appendHistory(msg);
        QMessageBox::information(this, QMessageBox::tr("Warning"),
                                 msg, QMessageBox::Ok);
        //file opening failed, clean up QC_MDIWindow and QMdiSubWindow
        if (w) {
            slotFilePrintPreview(false);
            doClose(w); //force closing, without asking user for confirmation
        }
        QMdiSubWindow *active = mdiAreaCAD->currentSubWindow();
        activedMdiSubWindow = nullptr; //to allow reactivate the previous active
        if (active) {//restore old geometry
            mdiAreaCAD->setActiveSubWindow(active);
            active->raise();
            active->setFocus();
            if (old == nullptr || maximized) {
                active->showMaximized();
            } else {
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
    RS2::FormatType type = RS2::FormatUnknown;
    QG_FileDialog dlg(this);
    QString fileName = dlg.getOpenFile(&type);
    openFile(fileName, type);
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(): OK");
}

QString QC_ApplicationWindow::getFileNameFromFullPath(const QString &path) {
    QFileInfo info(path);
    return info.fileName();
}

void QC_ApplicationWindow::updateCoordinateWidgetFormat(){
    m_coordinateWidget->setCoordinates({0.0, 0.0}, {0.0, 0.0}, true);
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
void QC_ApplicationWindow::openFile(const QString &fileName, RS2::FormatType type) {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(..)");

    QSettings settings;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (QFileInfo(fileName).exists()) {
        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: creating new doc window");
        if (openedFiles.indexOf(fileName) >= 0) {
            QString message = tr("Warning: File already opened : ") + fileName;
            m_commandWidget->appendHistory(message);
            showStatusMessage(message, 2000);
        }
        // Create new document window:
        QMdiSubWindow *old = activedMdiSubWindow;


        auto w = slotFileNew(nullptr);
        qApp->processEvents(QEventLoop::AllEvents, 1000);

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: linking layer list");

        // link the layer widget to the new document:
// fixme - sand - it seems that setup below is duplicated, as it is called from slotFileNew already
        setupWidgetsByWindow(w);

        // link coordinate widget to graphic

        auto graphicView = w->getGraphicView();

        RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen: open file");

        qApp->processEvents(QEventLoop::AllEvents, 1000);

        // open the file in the new view:
        bool success = false;
        if (QFileInfo(fileName).exists()) {
            success = w->loadDocument(fileName, type);
        } else {
            QString msg = tr("Cannot open the file\n%1\nPlease "
                             "check its existence and permissions.").arg(fileName);
            m_commandWidget->appendHistory(msg);
            QMessageBox::information(this, QMessageBox::tr("Warning"), msg, QMessageBox::Ok);
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
        m_recentFilesList->add(fileName);
        openedFiles.push_back(fileName);
        m_layerWidget->slotUpdateLayerList();
        if (m_layerTreeWidget != nullptr) { // fixme - sand - rewrite widget updates in initialization - move to single codebase
            m_layerTreeWidget->slotFilteringMaskChanged();
        }
        if (m_namedViewsWidget != nullptr){
            m_namedViewsWidget->reload();
        }
        if (m_ucsListWidget != nullptr){
            m_ucsListWidget->reload();
        }
        // fixme - sand - the overall workflow of file opening is suxx with lots of redundancy. Review it.
        if (m_quickInfoWidget != nullptr) {
            m_quickInfoWidget->updateFormats();
        }

         auto graphic = w->getGraphic();
        if (graphic) {
            if (int objects_removed = graphic->clean()) {
                auto msg = QObject::tr("Invalid objects removed:");
                m_commandWidget->appendHistory(msg + " " + QString::number(objects_removed));
            }
            emit(gridChanged(graphic->isGridOn()));

            if (m_anglesBasisWidget != nullptr){
                m_anglesBasisWidget->update(graphic);
            }
        }

        if (mdiAreaCAD->viewMode() == QMdiArea::TabbedView) {
            QList<QTabBar *> tabBarList = mdiAreaCAD->findChildren<QTabBar *>();
            QTabBar *tabBar = tabBarList.at(0);
            if (tabBar) {
                tabBar->setExpanding(false);
                tabBar->setTabToolTip(tabBar->currentIndex(), fileName);
            }
        } else {
            doArrangeWindows(RS2::CurrentMode);
        }

        if (LC_GET_ONE_BOOL("CADPreferences", "AutoZoomDrawing")) {
            graphicView->zoomAuto(false);
        }

        bool draftMode = LC_GET_ONE_BOOL("Appearance", "DraftMode", false);
        setupMDIWindowTitleByName(w, fileName, draftMode);
        graphicView->setDraftMode(draftMode);

        // update coordinate widget format:

        updateCoordinateWidgetFormat();

        QString message = tr("Loaded document: ") + fileName;
        m_commandWidget->appendHistory(message);
        showStatusMessage(message, 2000);

    } else {
        QG_DIALOGFACTORY->commandMessage(tr("File '%1' does not exist. Opening aborted").arg(fileName));
        showStatusMessage(tr("Opening aborted"), 2000);
    }

    QApplication::restoreOverrideCursor();
    RS_DEBUG->print("QC_ApplicationWindow::slotFileOpen(..) OK");
}

void QC_ApplicationWindow::slotFileOpen(const QString &fileName) {
    openFile(fileName, RS2::FormatUnknown);
}

/**
 * Menu file -> save.
 */
void QC_ApplicationWindow::slotFileSave() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileSave()");

    if (doSave(getCurrentMDIWindow())) {
        m_recentFilesList->updateRecentFilesMenu();
    }
}

/**
 * Menu file -> save as.
 */
void QC_ApplicationWindow::slotFileSaveAs() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileSaveAs()");
    if (doSave(getCurrentMDIWindow(), true)) {
        m_recentFilesList->updateRecentFilesMenu();
    }
}

bool QC_ApplicationWindow::doSaveAllFiles(){
    QC_MDIWindow *current = getCurrentMDIWindow();
    bool result{true};
    for (auto w: window_list) {
        if (w && w->getDocument()->isModified()) {
            result = doSave(w);
            if (!result) {
                showStatusMessage(tr("Save All cancelled"), 2000);
                break;
            }
        }
    }
    doActivate(current);
    m_recentFilesList->updateRecentFilesMenu();
    return result;
}

void QC_ApplicationWindow::slotFileSaveAll(){
    doSaveAllFiles();
}

/**
 * Autosave.
 */
void QC_ApplicationWindow::autoSaveCurrentDrawing() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileAutoSave(): begin");

    if (!LC_GET_ONE_BOOL("Defaults", "AutoBackupDocument", true)) {
        startAutoSaveTimer(false);
        return;
    }

    showStatusMessage(tr("Auto-saving drawing..."), 2000);

    QC_MDIWindow *w = getCurrentMDIWindow();
    if (w) {
        QString autosaveFileName;
        if (w->autoSaveDocument(autosaveFileName)) {
            // auto-save cannot be cancelled by user, so the
            // "cancelled" parameter is a dummy
            showStatusMessage(tr("Auto-saved drawing"), 2000);
        } else {
            // error
            m_autosaveTimer->stop();
            QMessageBox::information(this, QMessageBox::tr("Warning"),
                                     tr("Cannot auto-save the file\n%1\nPlease check the permissions.\n"
                                        "Auto-save disabled.").arg(autosaveFileName),QMessageBox::Ok);
            showStatusMessage(tr("Auto-saving failed"), 2000);
        }
    }
}


void QC_ApplicationWindow::showStatusMessage(const QString& msg, int timeout){
    statusBar()->showMessage(msg, timeout);
}

/**
 * Menu file -> export.
 */
void QC_ApplicationWindow::slotFileExport() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileExport()");

    showStatusMessage(tr("Exporting drawing..."), 2000);

    QC_MDIWindow *w = getCurrentMDIWindow();
    QString fn;
    if (w) {

        // read default settings:
        LC_GROUP("Export");
        QString defDir = LC_GET_STR("ExportImage", RS_SYSTEM->getHomeDir());
        QString defFilter = LC_GET_STR("ExportImageFilter",
                                       QString("%1 (%2)(*.%2)").arg(QG_DialogFactory::extToFormat("png")).arg("png"));
        LC_GROUP_END();

        bool cancel = false;

        QStringList filters;
        QList<QByteArray> supportedImageFormats = QImageWriter::supportedImageFormats();
        supportedImageFormats.push_back("svg"); // add svg

        for (QString format: supportedImageFormats) {
            format = format.toLower();
            QString st;
            if (format == "jpeg" || format == "tiff") {
                // Don't add the aliases
            } else {
                st = QString("%1 (%2)(*.%2)")
                    .arg(QG_DialogFactory::extToFormat(format))
                    .arg(format);
            }
            if (st.length() > 0)
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
        fn = QFileInfo(w->getFileName()).baseName();
        if (fn == nullptr)
            fn = "unnamed";
        fileDlg.selectFile(fn);

        if (fileDlg.exec() == QDialog::Accepted) {
            QStringList files = fileDlg.selectedFiles();
            if (!files.isEmpty())
                fn = files[0];
            cancel = false;
        } else {
            cancel = true;
        }

        // store new default settings:
        if (!cancel) {
            LC_GROUP_GUARD("Export");{
                LC_SET("ExportImage", QFileInfo(fn).absolutePath());
                LC_SET("ExportImageFilter",
                       fileDlg.selectedNameFilter());
            }

            // find out extension:

            QString filter = fileDlg.selectedNameFilter();
            QString format = "";
            int i = filter.indexOf("(*.");
            if (i != -1) {
                int i2 = filter.indexOf(QRegularExpression("[) ]"), i);
                format = filter.mid(i + 3, i2 - (i + 3));
                format = format.toUpper();
            }

            // append extension to file:
            if (!QFileInfo(fn).fileName().contains(".")) {
                fn.push_back("." + format.toLower());
            }

            // show options dialog:
            QG_ImageOptionsDialog dlg(this);
            w->getGraphic()->calculateBorders();
            dlg.setGraphicSize(w->getGraphic()->getSize() * 2.);
            if (dlg.exec()) {

                //QSize size = dlg.getSize();
                //QSize borders = dlg.getBorders();
                //bool black = dlg.isBackgroundBlack();
                //bool bw = dlg.isBlackWhite();

                bool ret = slotFileExport(fn, format, dlg.getSize(), dlg.getBorders(),
                                          dlg.isBackgroundBlack(), dlg.isBlackWhite());
                if (ret) {
                    QString message = tr("Exported: %1").arg(fn);
                    showStatusMessage(message, 2000);
                    m_commandWidget->appendHistory(message);
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
bool QC_ApplicationWindow::slotFileExport(
    const QString &name,
    const QString &format, QSize size, QSize borders, bool black, bool bw) {

    QC_MDIWindow *w = getCurrentMDIWindow();
    if (w == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QC_ApplicationWindow::slotFileExport: "
                        "no window opened");
        return false;
    }

    RS_Graphic *graphic = w->getDocument()->getGraphic();
    if (graphic == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QC_ApplicationWindow::slotFileExport: "
                        "no graphic");
        return false;
    }

    showStatusMessage(tr("Exporting..."));
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    bool ret = false;
    // set vars for normal pictures and vectors (svg)
    auto picture = std::make_shared<QPixmap>(size);
    auto vector = std::make_shared<QSvgGenerator>();

    // set buffer var
    std::shared_ptr<QPaintDevice> buffer;

    if (format.toLower() != "svg") {
        buffer = picture;
    } else {
        vector->setSize(size);
        vector->setViewBox(QRectF(QPointF(0, 0), size));
        vector->setFileName(name);
        buffer = vector;
    }

    // set painter with buffer
    RS_Painter painter(buffer.get());

    painter.setBackground(black ? Qt::black : Qt::white);
    if (bw) {
        painter.setDrawingMode(black ? RS2::ModeWB : RS2::ModeBW);
    }

    painter.eraseRect(0, 0, size.width(), size.height());

    // fixme - sand - rework to more generic printing (add progress or confirmation ?)

    LC_GraphicViewport viewport = LC_GraphicViewport();
    viewport.setSize(size.width(), size.height());
    viewport.setBorders(borders.width(), borders.height(), borders.width(), borders.height());
    viewport.setContainer(graphic);
    viewport.zoomAuto(false);
    viewport.loadSettings();

    LC_PrintViewportRenderer renderer = LC_PrintViewportRenderer(&viewport, &painter);
    renderer.setBackground(black ? Qt::black : Qt::white);
    renderer.loadSettings();
    renderer.render();

    // end the picture output
    if (format.toLower() != "svg") {
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

    showStatusMessage(ret ? tr("Export complete") : tr("Export failed!"), 2000);

    return ret;
}


/**
 * Called when a sub window is about to close.
 * If modified, show the Save/Close/Cancel dialog, then do the request.
 * If a save is needed but the user cancels, the window is not closed.
 */
void QC_ApplicationWindow::slotFileClosing(QC_MDIWindow *win) {
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
            default:
                break;
        }
    }
    if (!cancel) {
        doClose(win);
        doArrangeWindows(RS2::CurrentMode);
    }
}

bool QC_ApplicationWindow::doCloseAllFiles(){
    bool cancel(false), closeAll(false), hasParent(false);
    for (auto w: window_list)
        if (w) {
            hasParent = w->getParentWindow() != nullptr;

            if (w->getDocument()->isModified() && !hasParent && !closeAll) {
                doActivate(w);
                switch (showCloseDialog(w, window_list.count() > 1)) {
                    case QG_ExitDialog::Discard:
                        closeAll = true;
                        break;
                    case QG_ExitDialog::SaveAll:
                        closeAll = doSaveAllFiles();
                        break;
                    case QG_ExitDialog::Save:
                        cancel = !doSave(w);
                        break;
                    case QG_ExitDialog::Cancel:
                        cancel = true;
                        break;
                    default:
                        break;
                }
            }
            if (cancel) {
                showStatusMessage(tr("Close All cancelled"), 2000);
                return true;
            }

            doClose(w);
            doArrangeWindows(RS2::CurrentMode);
        }
    return false;
}

/**
 * File > Close All - loop through all open windows, and close them.
 * Prompt user to save changes for modified documents.  If the user cancels
 * the remaining unsaved documents will not be closed.
 *
 * @return true success
 * @return false the user cancelled.
 */
void QC_ApplicationWindow::slotFileCloseAll() {
    doCloseAllFiles();
}


/**
 * Menu file -> print.
 */
void QC_ApplicationWindow::slotFilePrint(bool printPDF) {
    RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "QC_ApplicationWindow::slotFilePrint(%s)", printPDF ? "PDF" : "Native");

    QC_MDIWindow *w = getCurrentMDIWindow();
    if (w == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QC_ApplicationWindow::slotFilePrint: "
                        "no window opened");
        return;
    }

    // Avoid printing without print preview
    if (!w->getGraphicView()->isPrintPreview()) {
        slotFilePrintPreview(true);
        return;
    }

    RS_Graphic *graphic = w->getDocument()->getGraphic();
    if (graphic == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QC_ApplicationWindow::slotFilePrint: "
                        "no graphic");
        return;
    }

    showStatusMessage(tr("Printing..."));
    using namespace LC_Printing;
    PrinterType type = printPDF ? PrinterType::PDF : PrinterType::Printer;
    LC_Printing::Print(*w, type);
    showStatusMessage(tr("Printing complete"), 2000);
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

    QC_MDIWindow *parent = getCurrentMDIWindow();

    if (!parent) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QC_ApplicationWindow::slotFilePrintPreview: "
                        "no window opened");
        return;
    }

    // close print preview:
    if (!on) {
        RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): off");

        QG_GraphicView *graphicView = parent->getGraphicView();
        if (graphicView->isPrintPreview()) {
            RS_ActionInterface *currentAction = graphicView->getCurrentAction();
            if (currentAction != nullptr){
                currentAction->hideOptions();
            }
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): close");
            updateActionsAndWidgetsForPrintPreview(false);
            doClose(parent);
            doArrangeWindows(RS2::CurrentMode);
            return;
        }
    }

        // open print preview:
    else {
        // look for an existing print preview:
        QC_MDIWindow *ppv = parent->getPrintPreview();

        if (ppv) {
            RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): show existing");
            doActivate(ppv);
            doArrangeWindows(RS2::CurrentMode);
            updateActionsAndWidgetsForPrintPreview(true);
        } else {
            if (!parent->getGraphicView()->isPrintPreview()) {
                QSettings settings;
                //generate a new print preview
                RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): create");

                auto *w = new QC_MDIWindow(parent->getDocument(), mdiAreaCAD, true);
                mdiAreaCAD->addSubWindow(w);
                parent->addChildWindow(w);

                w->setWindowTitle(tr("Print preview for %1").arg(parent->windowTitle()));
                w->setWindowIcon(QIcon(":/icons/document.lci"));
                QG_GraphicView *gv = w->getGraphicView();
                gv->device = settings.value("Hardware/Device", "Mouse").toString();
//                gv->setBackground(RS_Color(255, 255, 255));
                gv->setDefaultAction(new RS_ActionPrintPreview(*w->getDocument(), *w->getGraphicView()));

                // only graphics offer block lists, blocks don't
                RS_DEBUG->print("  adding listeners");
                RS_Graphic *graphic = w->getDocument()->getGraphic();
                if (graphic) {
                    // Link the layer list to the pen tool bar
                    graphic->addLayerListListener(m_penToolBar);
                    // Link the layer list to the layer widget
                    graphic->addLayerListListener(m_layerWidget);
                    // link the layer list ot the layer tree widget
                    graphic->addLayerListListener(m_layerTreeWidget);

                    // Link the block list to the block widget
                    graphic->addBlockListListener(m_blockWidget);

                    // fixme - sand - check whether we should setup ViewListener for NamedViewsList widget?

                }

                // Link the graphic view to the mouse widget:
                QG_DIALOGFACTORY->setMouseWidget(m_mouseWidget);
                // fixme - sand - check whether coordinates, selection and relzero are really necessary for print preview!!!
                // Link the graphic view to the coordinate widget:
                QG_DIALOGFACTORY->setCoordinateWidget(m_coordinateWidget);
                QG_DIALOGFACTORY->setRelativeZeroCoordinatesWidget(m_relativeZeroCoordinatesWidget);
                QG_DIALOGFACTORY->setSelectionWidget(m_selectionWidget);
                // Link the graphic view to the option widget:
                //QG_DIALOGFACTORY->setOptionWidget(optionWidget);
                // Link the graphic view to the command widget:
                QG_DIALOGFACTORY->setCommandWidget(m_commandWidget);
                QG_DIALOGFACTORY->setStatusBarManager(m_statusbarManager);

                RS_DEBUG->print("  showing MDI window");

                doActivate(w);
                doArrangeWindows(RS2::CurrentMode);

                gv->zoomAuto(false);

                if (graphic) {
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
                        gv->getViewPort()->zoomPage();
                    }
                }
                updateActionsAndWidgetsForPrintPreview(true);
            }
        }
    }
}


/**
 * Menu file -> quit.
 */
void QC_ApplicationWindow::slotFileQuit() {
    RS_DEBUG->print("QC_ApplicationWindow::slotFileQuit()");

    showStatusMessage(tr("Exiting application..."));
    if (queryMayExit()) {
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

    QC_MDIWindow *m = getCurrentMDIWindow();
    if (m) {
        RS_Graphic *g = m->getGraphic();
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

    LC_SET_ONE("Appearance","DraftMode", toggle);

    //handle "Draft Mode" in window titles
    QString draft_string = " [" + tr("Draft Mode") + "]";

    for (QC_MDIWindow *win: window_list) {
        win->getGraphicView()->setDraftMode(toggle);

        QC_MDIWindow *ppv = win->getPrintPreview();
        if (ppv != nullptr){
            QG_GraphicView *printPreviewGraphicView = ppv->getGraphicView();
            printPreviewGraphicView->setDraftMode(toggle);
        }

        /*setupMDIWindowTitle(win, drawingFileFullPath, toggle);*/



        QString title = win->windowTitle();

        if (toggle && !title.contains(draft_string)) {
            win->setWindowTitle(title + draft_string);
        } else if (!toggle && title.contains(draft_string)) {
            title.remove(draft_string);
            win->setWindowTitle(title);
        }
    }
    emit draftChanged(toggle);
    redrawAll();
}

void QC_ApplicationWindow::slotShowEntityDescriptionOnHover(bool toggle) {

    for (QC_MDIWindow *win: window_list) {
        QG_GraphicView *graphicView = win->getGraphicView();
        graphicView->setShowEntityDescriptionOnHover(toggle);
    }
    emit showEntityDescriptionOnHoverChanged(toggle);
    redrawAll();
}

void QC_ApplicationWindow::slotInfoCursorSetting(bool toggle) {

    auto *action = qobject_cast<QAction*>(sender());
    if (action != nullptr) {
        QVariant tag = action->property("InfoCursorActionTag");
        if (tag.isValid()){
            bool ok;
            int tagValue = tag.toInt(&ok);
            if (ok){
                bool doUpdate = true;
                switch (tagValue){
                    case 0:{
                        LC_SET_ONE("InfoOverlayCursor","Enabled", toggle);
                        emit showInfoCursorSettingChanged(toggle);
                        break;
                    }
                    case 1:{
                        LC_SET_ONE("InfoOverlayCursor","ShowAbsolute", toggle);
                        break;
                    }
                    case 2:{
                        LC_SET_ONE("InfoOverlayCursor","ShowSnapInfo", toggle);
                        break;
                    }
                    case 3:{
                        LC_SET_ONE("InfoOverlayCursor","ShowRelativeDA", toggle);
                        break;
                    }
                    case 4:{
                        LC_SET_ONE("InfoOverlayCursor","ShowPrompt", toggle);
                        break;
                    }
                    case 5:{
                        LC_SET_ONE("InfoOverlayCursor","ShowPropertiesCatched", toggle);
                        break;
                    }
                    default:
                        doUpdate = false;
                        break;
                }

                if (doUpdate){
                    doForEachWindowGraphicView([](QG_GraphicView *gv){
                        gv->loadSettings();
                        gv->redraw();
                    });
                }
            }
        }
    }
}

void QC_ApplicationWindow::doForEachWindowGraphicView(std::function<void(QG_GraphicView*)> callback) const{
    for (QC_MDIWindow* value : window_list) {
        QG_GraphicView *graphicView = value->getGraphicView();
        callback(graphicView);
    }
}

void QC_ApplicationWindow::slotViewDraftLines(bool toggle) {
    LC_SET_ONE("Appearance","DraftLinesMode", toggle);

    doForEachWindowGraphicView([toggle](QG_GraphicView *gv){
        gv->setDraftLinesMode(toggle);
    });
    emit draftLinesChanged(toggle);
    redrawAll();
}

void QC_ApplicationWindow::slotViewAntialiasing(bool toggle) {
    LC_SET_ONE("Appearance","Antialiasing", toggle);

    for (QC_MDIWindow *win: window_list) {
        QG_GraphicView *graphicView = win->getGraphicView();
        graphicView->setAntialiasing(toggle);
    }
    emit antialiasingChanged(toggle);
    redrawAll();
}


/**
 * Updates all grids of all graphic views.
 */
void QC_ApplicationWindow::updateGrids() {
    if (mdiAreaCAD) {
        QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
        for (int i = 0; i < windows.size(); ++i) {
            auto *m = qobject_cast<QC_MDIWindow *>(windows.at(i));
            if (m) {
                QG_GraphicView *gv = m->getGraphicView();
                if (gv) {
                    gv->loadSettings();
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
    LC_SET_ONE("Appearance", "StatusBarVisible", toggle);
}


void QC_ApplicationWindow::slotViewGridOrtho(bool toggle) {
    setGridView(toggle, false, RS2::IsoGridViewType::IsoLeft);
}

void QC_ApplicationWindow::slotViewGridIsoLeft(bool toggle) {
    setGridView(toggle, true, RS2::IsoGridViewType::IsoLeft);
}

void QC_ApplicationWindow::slotViewGridIsoRight(bool toggle) {
    setGridView(toggle, true, RS2::IsoGridViewType::IsoRight);
}

void QC_ApplicationWindow::slotViewGridIsoTop(bool toggle) {
    setGridView(toggle, true, RS2::IsoGridViewType::IsoTop);
}

void QC_ApplicationWindow::setGridView(bool toggle, bool isometric, RS2::IsoGridViewType isoGridType) {
    if (toggle) {
        RS_GraphicView *view = getCurrentGraphicView();
        if (view != nullptr) {
            if (!view->isPrintPreview()) {
                RS_Graphic *graphic = view->getGraphic();
                graphic->setIsometricGrid(isometric);
                if (isometric) {
                    graphic->setIsoView(isoGridType);
                }
                LC_GraphicViewport* viewport = view->getViewPort();
                viewport->loadGridSettings();
                updateGridViewActions(isometric, isoGridType);
                view->redraw();
                view->update();
            }
        }
    }
}

void QC_ApplicationWindow::updateGridViewActions(bool isometric, RS2::IsoGridViewType type) {
    bool viewOrtho = false, viewIsoLeft = false, viewIsoRight = false, viewIsoTop = false;

    if (isometric){
        switch (type){
            case RS2::IsoLeft:{
                viewIsoLeft = true;
                break;
            }
            case RS2::IsoTop:{
                viewIsoTop = true;
                break;
            }
            case RS2::IsoRight:{
                viewIsoRight = true;
            }
            default:
                break;
        }
    }
    else{
        viewOrtho = true;
    }

    getAction("ViewGridOrtho")->setChecked(viewOrtho);
    getAction("ViewGridIsoLeft")->setChecked(viewIsoLeft);
    getAction("ViewGridIsoTop")->setChecked(viewIsoTop);
    getAction("ViewGridIsoRight")->setChecked(viewIsoRight);
}

void QC_ApplicationWindow::slotOptionsShortcuts() {
    LC_ActionsShortcutsDialog dlg(this, m_actionGroupManager);
    dlg.exec();
}

void QC_ApplicationWindow::rebuildMenuIfNecessary(){
    m_menuFactory->recreateMainMenuIfNeeded(menuBar());
}

/**
 * Shows the dialog for general application preferences.
 */
void QC_ApplicationWindow::slotOptionsGeneral() {

    int dialogResult = RS_DIALOGFACTORY->requestOptionsGeneralDialog();
    if (dialogResult == QDialog::Accepted){
        // fixme - check this signal, probably it's better to rely on settings change
        bool hideRelativeZero = LC_GET_ONE_BOOL("Appearance", "hideRelativeZero");
        emit signalEnableRelativeZeroSnaps(!hideRelativeZero);

        bool antialiasing = LC_GET_ONE_BOOL("Appearance", "Antialiasing", false);
        emit antialiasingChanged(antialiasing);

        m_statusbarManager->loadSettings();
        onCADTabBarIndexChanged(0); // force update if settings changed

        QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
        for (int i = 0; i < windows.size(); ++i) {
            auto *m = qobject_cast<QC_MDIWindow *>(windows.at(i));
            if (m) {
                QG_GraphicView *gv = m->getGraphicView();
                if (gv != nullptr) {
                    gv->loadSettings();
                    if (m == activedMdiSubWindow) {
                        gv->redraw();
                    }
                }
            }
        }

        // fixme - sand - consider emitting signal on properties change instead of processing changes there

        LC_GROUP("InfoOverlayCursor");
        {
            bool infoCursorEnabled = LC_GET_BOOL("Enabled", true);
            QAction *action = getAction("EntityDescriptionInfo");
            if (action != nullptr) {
                action->setVisible(infoCursorEnabled);
            }

            action = getAction("InfoCursorEnable");
            if (action != nullptr) {
                action->setChecked(infoCursorEnabled);
            }
            // todo - is necessary to check for null there?
            checkAction("InfoCursorAbs", LC_GET_BOOL("ShowAbsolute", true));
            checkAction("InfoCursorSnap",LC_GET_BOOL("ShowSnapInfo", true));
            checkAction("InfoCursorRel",LC_GET_BOOL("ShowRelativeDA", true));
            checkAction("InfoCursorPrompt",LC_GET_BOOL("ShowPrompt", true));
            checkAction("InfoCursorCatchedEntity",LC_GET_BOOL("ShowPropertiesCatched", true));
        }
        LC_GROUP_END();

        rebuildMenuIfNecessary();
    }
}

/**
 * Menu File -> import -> importBlock
 */
// fixme - sand - files - rework
void QC_ApplicationWindow::slotImportBlock() {

    if (getCurrentMDIWindow() == nullptr) {
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
        if (m_actionHandler) {
            RS_ActionInterface *a =
                m_actionHandler->setCurrentAction(RS2::ActionLibraryInsert);
            if (a) {
                auto *action = (RS_ActionLibraryInsert *) a;
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


void QC_ApplicationWindow::showAboutWindow() {
    m_dlgHelpr->showAboutWindow();
}

bool QC_ApplicationWindow::queryMayExit() {
    RS_DEBUG->print("QC_ApplicationWindow::queryExit()");

    bool saveOpenedFiles = LC_GET_ONE_BOOL("Startup", "OpenLastOpenedFiles");
    QString openedFiles;
    QString activeFile = "";
    if (saveOpenedFiles) {
        for (auto w: window_list) {
            QString fileName = w->getFileName();
            if (activedMdiSubWindow != nullptr && activedMdiSubWindow == w) {
                activeFile = fileName;
            }
            openedFiles += fileName;
            openedFiles += ";";
        }
    }
    bool mayExit = !doCloseAllFiles();

    if (mayExit) {
        LC_GROUP_GUARD("Startup"); {
            LC_SET("LastOpenFilesList", openedFiles);
            LC_SET("LastOpenFilesActive", activeFile);
        }
        storeSettings();
    }

    RS_DEBUG->print("QC_ApplicationWindow::queryExit(): OK");
    return mayExit;
}

/**
 * Handle hotkeys. Don't let it to the default handler of Qt.
 * it will consume them also if a text field is active
 * which means it's impossible to enter a command.
 */
void QC_ApplicationWindow::keyPressEvent(QKeyEvent *e) {
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
            m_actionHandler->slotZoomIn();
            e->accept();
            break;

        case Qt::Key_Minus:
            m_actionHandler->slotZoomOut();
            e->accept();
            break;

        default:
            e->ignore();
// fixme - sand  - temporary test code, check regressions and move to method
// fixme me - add proper support for keyboard in view (scroll, zoom) and actions (default action - move by keyboards)
// fixme - as well as focusing options widget if there is action
//            RS_GraphicView* graphicView = getGraphicView();
//            if (graphicView) {
//                graphicView->keyPressEvent(e);
//            }
            // fixme - tmp-end
            RS_DEBUG->print("QC_ApplicationWindow::KeyPressEvent: IGNORED");
            break;
    }

    if (e->isAccepted()) {
        RS_DEBUG->print("QC_ApplicationWindow::KeyPressEvent: Accepted");
        return;
    }

    QMainWindow::keyPressEvent(e);
}

void QC_ApplicationWindow::createNewDocument(
    const QString &fileName, RS_Document *doc) {
    slotFileNew(doc);
    if (!fileName.isEmpty()) {
        RS_Document* doc = getCurrentDocument();
        if (doc != nullptr) {
            RS_Graphic* g = doc->getGraphic();
            // fixme - sand - files - check for blocks and fonts case!!!
            g->setFilename(fileName);
        }
    }
}

void QC_ApplicationWindow::updateWindowTitle(QWidget *w) {
    bool draftMode = LC_GET_ONE_BOOL("Appearance","DraftMode");
    if (draftMode) {
        QString draft_string = " [" + tr("Draft Mode") + "]";
        if (!w->windowTitle().contains(draft_string)) {
            w->setWindowTitle(w->windowTitle() + draft_string);
        }
    }
}

void QC_ApplicationWindow::relayAction(QAction *q_action) {
    auto view = getCurrentMDIWindow()->getGraphicView();
    if (!view) {
        // when switching back to LibreCAD from another program
        // occasionally no drawings are activated
        qWarning("relayAction: graphicView is nullptr");
        return;
    }

    // fixme - ugly fix for #2012. Actually, if some action does not invoke setCurrentAction(*) - it should not set current qaction..
    // probably there could be the list of ignored actions in the future
    bool setAsCurrentActionInView = true;
    if (getAction("LockRelativeZero") == q_action){
        // other actions may be added later
        setAsCurrentActionInView = false;
    }
    if (setAsCurrentActionInView) {
        view->setCurrentQAction(q_action);
    }

    setCurrentQAction(q_action);

    const QString commands(q_action->data().toString());
    if (!commands.isEmpty()) {
        const QString title(q_action->text().remove("&"));
        m_commandWidget->appendHistory(title + " : " + commands);
    }
}

void QC_ApplicationWindow::setCurrentQAction(QAction* q_action) {
    if (m_mouseWidget != nullptr){
        m_mouseWidget->setCurrentQAction(q_action);
    }
    if (m_statusbarManager != nullptr){
        m_statusbarManager->setCurrentQAction(q_action);
    }
    // fixme - sand - files - restore
    /*if (optionWidgetHolder != nullptr){
        optionWidgetHolder->setCurrentQAction(q_action);
    }*/
}

/**
 * Called by Qt after a toolbar or dockwidget right-click.
 * See QMainWindow::createPopupMenu() for more information.
 */
QMenu *QC_ApplicationWindow::createPopupMenu() {
   return  m_menuFactory->createMainWindowPopupMenu();
}

void QC_ApplicationWindow::toggleFullscreen(bool checked) {
    checked ? showFullScreen() : showMaximized();
}

void QC_ApplicationWindow::hideOptions(QC_MDIWindow *win) {
    auto graphicView = win->getGraphicView();
    auto defaultAction = graphicView->getDefaultAction();
    defaultAction->hideOptions();
}

void QC_ApplicationWindow::slotFileOpenRecent(QAction *action){
    auto variant = action->data();
    if (variant.isValid()) {
        showStatusMessage(tr("Opening recent file..."));
        QString fileName = variant.toString();
        openFile(fileName, RS2::FormatUnknown);
    }
}

/**
 * This slot manipulates the widget options dialog,
 * and reads / writes the associated settings.
 */
void QC_ApplicationWindow::widgetOptionsDialog() {
    if (m_dlgHelpr->widgetOptionsDialog()) {
        fireWidgetSettingsChanged();
    }
}

/**
 * This slot modifies the commandline's title bar
 * depending on the dock area it is moved to.
 */
void QC_ApplicationWindow::modifyCommandTitleBar(Qt::DockWidgetArea area) {
    auto *cmdDockWidget = findChild<QDockWidget *>("command_dockwidget");

    auto *commandWidget = static_cast<QG_CommandWidget *>(cmdDockWidget->widget());
    QAction *dockingAction = commandWidget->getDockingAction();
    bool docked = area & Qt::AllDockWidgetAreas;
    cmdDockWidget->setWindowTitle(docked ? tr("Cmd") : tr("Command line"));
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

bool QC_ApplicationWindow::loadStyleSheet(QString path) {
    if (!path.isEmpty() && QFile::exists(path)) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qApp->setStyleSheet(QString::fromLatin1(file.readAll()));
            return true;
        }
    }
    return false;
}

void QC_ApplicationWindow::reloadStyleSheet() {
    loadStyleSheet(m_styleSheetPath);
}

bool QC_ApplicationWindow::eventFilter(QObject *obj, QEvent *event) {
    if (QEvent::FileOpen == event->type()) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        openFile(openEvent->file(), RS2::FormatUnknown);
        return true;
    }
    return QObject::eventFilter(obj, event);
}

void QC_ApplicationWindow::updateGridStatus(const QString &status) {
    m_gridStatusWidget->setBottomLabel(status);
}

void QC_ApplicationWindow::showDeviceOptions() {
   m_dlgHelpr->showDeviceOptions();
}

void QC_ApplicationWindow::updateDevice(QString device) {
    LC_SET_ONE("Hardware", "Device", device);
    for (const auto &win: window_list) {
        win->getGraphicView()->device = device;
    }
}

void QC_ApplicationWindow::saveNamedView() {
    if (m_namedViewsWidget != nullptr){
        m_namedViewsWidget->addNewView();
    }
}

void QC_ApplicationWindow::saveWorkspace(bool on) {
    bool ok;
    QStringList options;
    m_workspacesManager.getWorkspaceNames(options);
    auto name = LC_InputTextDialog::getText(this, tr("New Workspace"), tr("Name of workspace to save:"), options, true, "", &ok);
    if (ok) {
        m_workspacesManager.saveWorkspace(name, this);
        fireWorkspacesChanged();
    }
}

void  QC_ApplicationWindow::fillWorkspacesList(QList<QPair<int, QString>> &list){
    m_workspacesManager.getWorkspaces(list);
}

void QC_ApplicationWindow::applyWorkspaceById(int id){
    m_workspacesManager.activateWorkspace(id);
}

void QC_ApplicationWindow::removeWorkspace(bool on){
    bool ok;
    QList<QPair<int, QString>> options;
    m_workspacesManager.getWorkspaces(options);
    int workspaceId = LC_InputTextDialog::selectId(this, tr("Remove Workspace"), tr("Select workspace to remove:"), options, &ok);
    if (ok) {
       m_workspacesManager.deleteWorkspace(workspaceId);
       fireWorkspacesChanged();
    }
}

void QC_ApplicationWindow::restoreWorkspace(bool on){
    auto *action = qobject_cast<QAction*>(sender());
    if (action != nullptr) {
        QVariant variant = action->property("_WSPS_IDX");
        if (variant.isValid()){
            int id = variant.toInt();
            applyWorkspaceById(id);
        }
        else {
            m_workspacesManager.activateWorkspace(-1);
        }
    }
}

// methods needed for support of shortcuts for views restoring
void QC_ApplicationWindow::restoreNamedView1() {
    doRestoreNamedView(1);
}

void QC_ApplicationWindow::restoreNamedView2() {
    doRestoreNamedView(2);
}

void QC_ApplicationWindow::restoreNamedView3() {
    doRestoreNamedView(3);
}

void QC_ApplicationWindow::restoreNamedView4() {
    doRestoreNamedView(4);
}

void QC_ApplicationWindow::restoreNamedViewCurrent() {
    if (m_namedViewsWidget != nullptr){
        m_namedViewsWidget->restoreSelectedView();
    }
}

void QC_ApplicationWindow::restoreNamedView5() {
    doRestoreNamedView(5);
}

void QC_ApplicationWindow::restoreNamedView(const QString& viewName){
    if (m_namedViewsWidget != nullptr){
        m_namedViewsWidget->restoreView(viewName);
    }
}

void QC_ApplicationWindow::doRestoreNamedView(int i) const {
    if (m_namedViewsWidget != nullptr){
        m_namedViewsWidget->restoreView(i);
    }
}

void QC_ApplicationWindow::invokeToolbarCreator() {
    m_creatorInvoker->invokeToolbarCreator();
}

void QC_ApplicationWindow::invokeMenuCreator() {
    m_creatorInvoker->invokeMenuCreator();
}

void QC_ApplicationWindow::changeEvent([[maybe_unused]] QEvent *event) {
    // returning to LC via Command+Tab won't always activate a subwindow #821

#if defined(Q_OS_MACOS)
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
#endif
}

void QC_ApplicationWindow::invokeLicenseWindow() {
    m_dlgHelpr-> invokeLicenseWindow();
}

void QC_ApplicationWindow::showBlockActivated(const RS_Block *block) {
    if (m_blockWidget != nullptr && block != nullptr) {
        m_blockWidget->activateBlock(const_cast<RS_Block *>(block));
    }
}

QAction *QC_ApplicationWindow::getAction(const QString &actionName) const {
    return m_actionGroupManager->getActionByName(actionName);
}

// todo - think later about staying with signal-slot approach... current one is too explicit
void QC_ApplicationWindow::updateActionsAndWidgetsForPrintPreview(bool printPreviewOn) {
    bool enable = !printPreviewOn;
    enableWidgets(enable);
    for (auto a: m_actionsToDisableInPrintPreviewList) {
        if (a->isEnabled() != enable) {
            a->setEnabled(enable);
        }
    }

    m_coordinateWidget->setEnabled(!printPreviewOn);
    m_selectionWidget->setEnabled(!printPreviewOn);
    m_activeLayerName->setEnabled(!printPreviewOn);
    m_gridStatusWidget->setEnabled(!printPreviewOn);
    m_relativeZeroCoordinatesWidget->setEnabled(!printPreviewOn);
    if (printPreviewOn){
        m_mouseWidget->setActionIcon(QIcon());
    }

    emit printPreviewChanged(printPreviewOn);
}

void QC_ApplicationWindow::enableWidgets(bool enable) {
    enableWidget(m_penPaletteWidget, enable);
    enableWidget(m_quickInfoWidget, enable);
    enableWidget(m_blockWidget, enable);
    enableWidget(m_penToolBar, enable);
    enableWidget(m_penWizard, enable);
//    enableWidget(namedViewsWidget,enable);
    enableWidget(m_ucsListWidget, enable);
    enableWidget(m_ucsStateWidget, enable);
    enableWidget(m_anglesBasisWidget, enable);

    if (m_libraryWidget != nullptr) {
        enableWidget(m_libraryWidget->getInsertButton(), enable);
    }
    enableWidget(m_snapToolBar, enable);

    if (enable) {
        enableWidget(m_layerTreeWidget, enable);
        enableWidget(m_layerWidget, enable);
        // command widget should be enabled for print preview as it supports commands...
        // fixme - command widget should be aware of print preview mode and do not support other commands...
        enableWidget(m_commandWidget, enable);
    }

    // fixme - disable widgets from status bar
}

void QC_ApplicationWindow::slotRedockWidgets() {
    const QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    for (auto *dockwidget: dockwidgets) {
        dockwidget->setFloating(false);
    }
}

void QC_ApplicationWindow::fireIconsRefresh(){
    emit iconsRefreshed();
}

void QC_ApplicationWindow::fireWidgetSettingsChanged(){
    emit widgetSettingsChanged();
}

void QC_ApplicationWindow::fireWorkspacesChanged(){
    bool hasWorkspaces = m_workspacesManager.hasWorkspaces();
    emit workspacesChanged(hasWorkspaces);
}
