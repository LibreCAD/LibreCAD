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

#include "qc_applicationwindow.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QDockWidget>
#include <QMdiArea>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QStatusBar>
#include <QStyleHints>
#include <QTimer>

#include "lc_iconcolorsoptions.h"

#include "lc_action_block_library_insert.h"
#include "lc_action_options_manager.h"
#include "lc_actiongroupmanager.h"
#include "lc_actionsshortcutsdialog.h"
#include "lc_anglesbasiswidget.h"
#include "lc_applicationwindowinitializer.h"
#include "lc_appwindowdialogsinvoker.h"
#include "lc_creatorinvoker.h"
#include "lc_customstylehelper.h"
#include "lc_defaultactioncontext.h"
#include "lc_exporttoimageservice.h"
#include "lc_graphicviewport.h"
#include "lc_gridviewinvoker.h"
#include "lc_infocursorsettingsmanager.h"
#include "lc_lastopenfilesopener.h"
#include "lc_layertreewidget.h"
#include "lc_menufactory.h"
#include "lc_namedviewslistwidget.h"
#include "lc_penpalettewidget.h"
#include "lc_penwizard.h"
#include "lc_printing.h"
#include "lc_propertysheetwidget.h"
#include "lc_qtstatusbarmanager.h"
#include "lc_quickinfowidget.h"
#include "lc_releasechecker.h"
#include "lc_relzerocoordinateswidget.h"
#include "lc_snapmanager.h"
#include "lc_snapoptionswidgetsholder.h"
#include "lc_ucslistwidget.h"
#include "lc_ucsstatewidget.h"
#include "lc_workspacesinvoker.h"
#include "qc_dialogfactory.h"
#include "qc_mdiwindow.h"
#include "qg_actionhandler.h"
#include "qg_activelayername.h"
#include "qg_blockwidget.h"
#include "qg_commandwidget.h"
#include "qg_coordinatewidget.h"
#include "qg_exitdialog.h"
#include "qg_graphicview.h"
#include "qg_layerwidget.h"
#include "qg_librarywidget.h"
#include "qg_mousewidget.h"
#include "qg_pentoolbar.h"
#include "qg_recentfiles.h"
#include "qg_selectionwidget.h"
#include "qg_snaptoolbar.h"
#include "rs_actioninterface.h"
#include "rs_actionprintpreview.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "twostackedlabels.h"

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

class QSplashScreen;
/**
 * Constructor. Initializes the app.
 */
QC_ApplicationWindow::QC_ApplicationWindow() {
#ifdef _WINDOWS
    qt_ntfs_permission_lookup++; // turn checking on
#endif
    setWindowIcon(QIcon(QC_APP_ICON));

    LC_ApplicationWindowInitializer initializer(this);
    initializer.initApplication();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Re-apply icon styling defaults when the OS color scheme flips so the
    // toolbar icons don't go invisible on a newly-dark palette. Stored user
    // overrides are preserved because loadColor() returns the stored value
    // when present and falls back to the (now theme-aware) default only
    // when the key is absent.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, [this](Qt::ColorScheme) {
                LC_IconColorsOptions opts;
                opts.loadSettings();
                opts.applyOptions();
                fireIconsRefresh();
            });
#endif

    // Re-paint toolbars when the OS-level primary screen changes (e.g. the
    // user swaps which monitor is primary while LibreCAD is running). The
    // canvas's own m_isHiDpi state is computed once at LC_GraphicViewRenderer
    // construction and currently does NOT re-evaluate here — see the
    // // fixme - sand comment at lc_graphicviewrenderer.cpp:43. Tracking that
    // through to the per-MDI-window renderer is follow-up work documented in
    // Task D of the plan; this connect at least refreshes icons and any
    // listener wired to iconsRefreshed().
    connect(qApp, &QGuiApplication::primaryScreenChanged,
            this, [this](QScreen*) { fireIconsRefresh(); });
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
    delete m_actionContext;
}

void QC_ApplicationWindow::checkForNewVersion() const {
    m_releaseChecker->checkForNewVersion();
}

void QC_ApplicationWindow::forceCheckForNewVersion() const {
    m_releaseChecker->checkForNewVersion(true);
}

void QC_ApplicationWindow::onNewVersionAvailable() const {
    m_dlgHelpr->showNewVersionAvailableDialog(m_releaseChecker.get());
}

// fixme - should it be there or in persistence?
void QC_ApplicationWindow::startAutoSaveTimer(const bool startAutoBackup) {
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
                const int ms = 60000 * LC_GET_INT("AutoSaveTime", 5);
                m_autosaveTimer->start(ms);
            }
        }
    }
    else {
        if (m_autosaveTimer != nullptr) {
            m_autosaveTimer.reset();
        }
    }
}

void QC_ApplicationWindow::tryShowRelativeInput(RS2::RelativePointParam paramType) const {
    RS_ActionInterface* currentAction = m_actionContext->getCurrentAction();
    if (currentAction != nullptr) {
        currentAction->tryShowRelativeInput(paramType);
    }
}

/**
 * @brief QC_ApplicationWindow::getAppWindow() accessor for the application window singleton instance
 * @return QC_ApplicationWindow* the application window instance
 */
std::unique_ptr<QC_ApplicationWindow>& QC_ApplicationWindow::getAppWindow() {
    static auto instance = std::unique_ptr < QC_ApplicationWindow > (new QC_ApplicationWindow);
    // singleton could be reset: cannot be called after reseting
    Q_ASSERT(instance != nullptr);
    return instance;
}

void QC_ApplicationWindow::setupMDIWindowTitleByFile(QC_MDIWindow* w, const QString& drawingFileFullPath, const bool draftMode,
                                                     const bool forPreview) {
    const QString fileName = getFileNameFromFullPath(drawingFileFullPath);
    QString baseName;
    if (forPreview) {
        baseName = tr("Print preview for %1").arg(fileName);
    }
    else {
        baseName = fileName;
    }
    setupMDIWindowTitleByName(w, baseName, draftMode);
}

void QC_ApplicationWindow::setupMDIWindowTitleByName(QC_MDIWindow* w, const QString& baseTitleString, const bool draftMode) {
    if (draftMode) {
        const auto title = baseTitleString + "[*]" + " [" + tr("Draft Mode") + "]";
        w->setWindowTitle(title);
    }
    else {
        const auto title = baseTitleString + "[*]";
        w->setWindowTitle(title);
    }
}

/**
 * Force-Save(as) the content of the sub window.  Retry on failure.
 * @return true success (or window was not modified)
 * @return false user cancelled (or window was null)
 */
bool QC_ApplicationWindow::doSave(QC_MDIWindow* w, const bool forceSaveAs) {
    if (w == nullptr) {
        return false;
    }
    if (w->isModified() || forceSaveAs) {
        QString drawingFileFullPath = w->getFileName();
        if (drawingFileFullPath.isEmpty()) {
            doActivate(w); // show the user the drawing for save as
        }
        QString msg = drawingFileFullPath.isEmpty() ? tr("Saving drawing...") : tr("Saving drawing: %1").arg(drawingFileFullPath);
        showStatusMessage(msg);
        bool cancelled = false;
        const bool saved = forceSaveAs ? w->saveDocumentAs(cancelled) : w->saveDocument(cancelled);
        if (saved) {
            if (cancelled) {
                showStatusMessage(tr("Save cancelled"), 2000);
                return false;
            }

            drawingFileFullPath = w->getFileName();
            msg = tr("Saved drawing: %1").arg(drawingFileFullPath);
            notificationMessage(msg, 2000);

            m_recentFilesList->addIfAbsent(drawingFileFullPath);

            const auto graphicView = w->getGraphicView();
            const bool draftMode = graphicView->isDraftMode();
            setupMDIWindowTitleByFile(w, drawingFileFullPath, draftMode, graphicView->isPrintPreview());

            const bool autoBackup = LC_GET_ONE_BOOL("Defaults", "AutoBackupDocument", true);
            startAutoSaveTimer(autoBackup);
        }
        else {
            msg = tr("Cannot save the file ") + w->getFileName() + tr(" , please check the filename and permissions.");
            notificationMessage(msg, 2000);
            return doSave(w, true);
        }
    }
    return true;
}

void QC_ApplicationWindow::activeMDIWindowChanged(QC_MDIWindow* window) {
    m_activeMdiSubWindow = window;
}

/**
 * Force-Close this sub window.
 * @param w
 * @param activateNext also activate the next window in the window_list, if any
 */
void QC_ApplicationWindow::doClose(QC_MDIWindow* w, const bool activateNext) {
    w->getGraphicView()->killAllActions();

    QC_MDIWindow* parentWindow = w->getParentWindow();
    if (parentWindow != nullptr) {
        parentWindow->removeChildWindow(w);
    }

    const auto graphic = w->getDocument()->getGraphic();
    if (graphic != nullptr) {
        const auto view = w->getGraphicView();
        graphic->removeLayerListListener(view);
    }

    for (auto && child : std::as_const(w->getChildWindows())) {
        // block editors and print previews; just force these closed
        doClose(child, false); // they belong to the document (changes already saved there)
    }

    w->getChildWindows().clear();
    // m_mdiAreaCAD->removeSubWindow(w);
    if (w->getSaveOnClosePolicy() == QC_MDIWindow::SaveOnClosePolicy::CANCEL) {
        // support for cancelling of saving untitled new document (via close all and close event)
        return;
    }
    w->close();
    m_windowList.removeOne(w);

    if (m_activeMdiSubWindow == nullptr || m_activeMdiSubWindow == w) {
        setupWidgetsByWindow(nullptr);
    }
    m_openedFiles.removeAll(w->getFileName());

    activeMDIWindowChanged(nullptr);
    m_actionHandler->setDocumentAndView(nullptr, nullptr);

    if (activateNext && !m_windowList.empty()) {
        if (parentWindow != nullptr) {
            doActivate(parentWindow);
        }
        else {
            doActivate(m_windowList.back());
        }
    }
    enableFileActions();
}

void QC_ApplicationWindow::enableFileActions() {
    if (m_windowList.isEmpty()) {
        enableFileActions(nullptr);
    }
}

// fixme - sand - files - change to signals?
void QC_ApplicationWindow::setupWidgetsByWindow(const QC_MDIWindow* w) const {
    RS_GraphicView* gv = (w == nullptr) ? nullptr : w->getGraphicView();

    // fixme - sand - files - replace by updating list of instances, to simplify introduction of new widgets

    m_layerWidget->setGraphicView(gv);
    m_layerTreeWidget->setGraphicView(gv);
    m_namedViewsWidget->setGraphicView(gv);
    m_ucsListWidget->setGraphicView(gv);
    m_quickInfoWidget->setGraphicView(gv);
    m_anglesBasisWidget->setGraphicView(gv);
    m_penPaletteWidget->setGraphicView(gv);
    m_blockWidget->setGraphicView(gv);
    m_coordinateWidget->setGraphicView(gv);
    m_relativeZeroCoordinatesWidget->setGraphicView(gv);
    m_penToolBar->setGraphicView(gv);
    m_activeLayerNameWidget->setGraphicView(gv);
    m_selectionWidget->setGraphicView(gv);
    m_penWizard->setGraphicView(gv);
    m_propertySheetWidget->setGraphicView(gv);
}

/**
 * Force-Activate this sub window.
 */
void QC_ApplicationWindow::doActivate(QMdiSubWindow* w) {
    LC_MDIApplicationWindow::doActivate(w);
    enableFileActions(qobject_cast<QC_MDIWindow*>(w));
    // fixme - sand - potentially, there we may just fire signal to widgets...
}

int QC_ApplicationWindow::showCloseDialog(const QC_MDIWindow* w, const bool showSaveAll) const {
    return m_dlgHelpr->showCloseDialog(w, showSaveAll);
}

/**
 * Enable the available file actions for this sub-window.
 */
void QC_ApplicationWindow::enableFileActions(const QC_MDIWindow* w) {
    const bool hasWindow = w != nullptr;
    QString fileName;
    if (hasWindow) {
        fileName = w->getFileName();
    }

    if (!hasWindow || fileName.isEmpty()) {
        getAction("FileSave")->setText(tr("&Save"));
        getAction("FileSaveAs")->setText(tr("Save &as..."));
    }
    else {
        const QString name = getFileNameFromFullPath(fileName);
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
                  }, hasWindow);

    enableActions({"FileSaveAll", "FileCloseAll"}, hasWindow && m_windowList.count() > 1);
}

LC_ActionContext* QC_ApplicationWindow::getActionContext() const {
    return m_actionContext;
}

/**
 * Close Event. Called when the user tries to close the app.
 */
void QC_ApplicationWindow::closeEvent(QCloseEvent* ce) {
    tryCloseAllBeforeExist() ? ce->accept() : ce->ignore();
}

bool QC_ApplicationWindow::isAcceptableDragNDropFileName(const QString& fileName) {
    if (fileName.endsWith(R"(.dxf)", Qt::CaseInsensitive) || fileName.endsWith(R"(.cxf)", Qt::CaseInsensitive) || fileName.endsWith(
        R"(.lff)", Qt::CaseInsensitive)) {
        return QFileInfo::exists(fileName);
    }
    return false;
}

void QC_ApplicationWindow::dropEvent(QDropEvent* event) {
    event->acceptProposedAction();
    //limit maximum number of dropped files to be opened
    unsigned counts = 0;
    for (const QUrl& url : event->mimeData()->urls()) {
        const QString& fileName = url.toLocalFile();
        if (isAcceptableDragNDropFileName(fileName)) {
            openFile(fileName);
            if (++counts > 32) {
                return;
            }
        }
    }
}

void QC_ApplicationWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            const QString& fileName = url.toLocalFile();
            if (isAcceptableDragNDropFileName(fileName)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void QC_ApplicationWindow::setPreviousZoomEnable(const bool enable) {
    m_previousZoomEnable = enable;
    enableAction("ZoomPrevious", enable);
}

void QC_ApplicationWindow::setUndoEnable(const bool enable) {
    m_undoEnable = enable;
    enableAction("EditUndo", enable);
}

void QC_ApplicationWindow::setRedoEnable(const bool enable) {
    m_redoEnable = enable;
    enableAction("EditRedo", enable);
}

void QC_ApplicationWindow::setSaveEnable(const bool enable) const {
    enableAction("FileSave", enable);
}

void QC_ApplicationWindow::slotEnableActions(const bool enable) const {
    enableAction("ZoomPrevious", enable && m_previousZoomEnable);
    enableAction("EditUndo", enable && m_undoEnable);
    enableAction("EditRedo", enable && m_redoEnable);
}

// fixme - sand - rework, think about changed to signal from the widget?
void QC_ApplicationWindow::slotUpdateActiveLayer() const {
    if (m_layerWidget != nullptr && m_activeLayerNameWidget != nullptr) {
        m_activeLayerNameWidget->activeLayerChanged(m_layerWidget->getActiveName());
    }
}

/**
 * Initializes the global application settings from the
 * config file (unix, mac) or registry (windows).
 */
void QC_ApplicationWindow::initSettings() {
    RS_DEBUG->print("QC_ApplicationWindow::initSettings()");

    const bool first_load = LC_GET_ONE_BOOL("Startup", "FirstLoad", true);
    if (!first_load) {
        m_workspacesInvoker->init();
    }
    fireWorkspacesChanged();
    m_styleHelper->loadFromSettings();
    LC_GROUP("Appearance");
    {
        QAction* viewLinesDraftAction = getAction("ViewLinesDraft");
        viewLinesDraftAction->setChecked(LC_GET_BOOL("DraftLinesMode", false));

        const bool draftMode = LC_GET_BOOL("DraftMode", false);

        getAction("ViewDraft")->setChecked(draftMode);
        viewLinesDraftAction->setDisabled(draftMode);

        QAction* viewAntialiasing = getAction("ViewAntialiasing");
        const bool antialiasing = LC_GET_BOOL("Antialiasing", false);
        viewAntialiasing->setChecked(antialiasing);
    }
    LC_GROUP_END();
    m_infoCursorSettingsManager->loadFromSettings();
}

/**
 * Stores the global application settings to file or registry.
 */
void QC_ApplicationWindow::storeSettings() const {
    if (RS_Settings::saveIsAllowed) {
        m_workspacesInvoker->persist();
        m_penPaletteWidget->persist();
        // fixme - sand - decided whether shortcuts should be also saved... This may be necessary if
        // the path for settins was changed.
        // m_actionGroupManager->persist();

        m_snapToolBar->saveSnapMode();
    }
}

void QC_ApplicationWindow::slotKillAllActions() {
    const QC_MDIWindow* win = getCurrentMDIWindow();
    if (win != nullptr) {
        RS_GraphicView* gv = win->getGraphicView();
        if (gv != nullptr) {
            gv->switchToDefaultAction();
        }
    }
}

/**
 * Sets the keyboard focus on the command line.
 */
void QC_ApplicationWindow::slotFocusCommandLine() {
    // if command widget is not visible - show it first
    auto* cmd_dockwidget = findChild<QDockWidget*>("command_dockwidget");
    if (cmd_dockwidget->isHidden()) {
        cmd_dockwidget->show();
    }
    m_commandWidget->focusWidget();
}

void QC_ApplicationWindow::slotFocusOptionsWidget() {
    // fixme - sand - files - fix for mor reliable focus settings
    if (m_toolOptionsToolbar != nullptr) {
        m_toolOptionsToolbar->setFocus();
    }
}

/**
 * Shows the given error on the command line.
 */
void QC_ApplicationWindow::slotError(const QString& msg) const {
    m_commandWidget->appendHistory(msg);
}

void QC_ApplicationWindow::slotShowDrawingOptions() const {
    m_actionHandler->setCurrentAction(RS2::ActionOptionsDrawingGrid);
}

void QC_ApplicationWindow::slotShowDrawingOptionsUnits() const {
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

void QC_ApplicationWindow::disableUIForAbsentDrawing() {
    enableWidgets(false);
    enableWidgetList(false, {m_layerTreeWidget, m_layerWidget, m_commandWidget});
    m_snapToolBar->getSnapOptionsHolder()->hideSnapOptions();
    m_coordinateWidget->clearContent();
    m_relativeZeroCoordinatesWidget->clearContent();
}

/**
 * Called when a document window was activated.
 */
void QC_ApplicationWindow::doWindowActivated(QMdiSubWindow* w, const bool forced) {
    if (w == nullptr) {
        // when it may occur???
        disableUIForAbsentDrawing();
        // todo - check which other widgets in status bar or so should be cleared if no files..
        emit windowsChanged(false);
        activeMDIWindowChanged(nullptr);
        return;
    }

    if (w == m_activeMdiSubWindow) {
        // this may occur after file open, so additional update is needed :(
        RS_GraphicView* activatedGraphicView = getCurrentGraphicView();
        if (activatedGraphicView != nullptr) {
            const RS_Graphic* activatedGraphic = activatedGraphicView->getGraphic();
            if (activatedGraphic != nullptr) {
                const bool printPreview = activatedGraphicView->isPrintPreview();
                if (!printPreview) {
                    const bool isometricGrid = activatedGraphic->isIsometricGrid();
                    const RS2::IsoGridViewType isoViewType = activatedGraphic->getIsoView();
                    updateGridViewActions(isometricGrid, isoViewType);
                }
                activatedGraphicView->loadSettings();
                activatedGraphicView->redraw();
            }
        }
        return;
    }

    // kill active actions in previous windows.that will prevent the situation described by issue #1762 with
    // non-finished action started on previous window and action that is active with UI still checked  after window switch
    doForEachSubWindowGraphicView([](const QG_GraphicView* graphicView, [[maybe_unused]] QC_MDIWindow* sw) {
        RS_ActionInterface* ai = graphicView->getCurrentAction();
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
    });

    const auto windowActivated = static_cast<QC_MDIWindow*>(w);
    activeMDIWindowChanged(windowActivated);
    enableFileActions(windowActivated);

    bool hasDocumentInActivatedWindow = false;

    RS_Document* activatedDocument = windowActivated->getDocument();
    if (activatedDocument != nullptr) {
        hasDocumentInActivatedWindow = true;
        QG_GraphicView* activatedGraphicView = windowActivated->getGraphicView();
        activatedGraphicView->loadSettings();

        const RS_Graphic* activatedGraphic = windowActivated->getGraphic();
        RS_Units::setCurrentDrawingUnits(activatedDocument->getGraphic()->getUnit());

        m_actionHandler->setDocumentAndView(activatedDocument, activatedGraphicView);
        setupWidgetsByWindow(windowActivated);

        // Update all inserts in this graphic (blocks might have changed):
        activatedDocument->updateInserts();
        // whether to enable undo/redo buttons
        activatedDocument->updateUndoState();

        QAction* lockRelZeroAction = m_actionGroupManager->getActionByName("LockRelativeZero");
        if (lockRelZeroAction != nullptr) {
            const bool locked = activatedGraphicView->getViewPort()->isRelativeZeroLocked();
            lockRelZeroAction->setChecked(locked);
        }

        activatedGraphicView->redraw();

        // set pen from pen toolbar
        slotPenChanged(m_penToolBar->getPen());

        if (!forced) {
            // update toggle button status:
            emit gridChanged(activatedGraphic->isGridOn());
        }

        const bool printPreview = activatedGraphicView->isPrintPreview();
        if (!printPreview) {
            const bool isometricGrid = activatedGraphic->isIsometricGrid();
            const RS2::IsoGridViewType isoViewType = activatedGraphic->getIsoView();
            updateGridViewActions(isometricGrid, isoViewType);
            m_actionHandler->setSnaps(m_snapToolBar->getSnaps());
        }

        updateActionsAndWidgetsForPrintPreview(printPreview);

        const RS_ActionInterface* currentAction = activatedGraphicView->getCurrentAction();
        if (currentAction != nullptr) {
            currentAction->showOptions();
        }
    }

    // Disable/Enable menu and toolbar items
    emit windowsChanged(hasDocumentInActivatedWindow);
}

/**
 * Called when the menu 'workspaces' is about to be shown.
 * This is used to update the window list in the menu.
 */
void QC_ApplicationWindow::slotWorkspacesMenuAboutToShow() const {
    m_menuFactory->onWorkspaceMenuAboutToShow(m_windowList);
}

QMenu* QC_ApplicationWindow::createGraphicViewContentMenu(const QMouseEvent* event, QG_GraphicView* view, RS_Entity* entity,
                                                          const RS_Vector& pos) const {
    QStringList actions;
    const bool mayInvokeDefaultMenu = m_creatorInvoker->getMenuActionsForMouseEvent(event, entity, actions);
    return m_menuFactory->createGraphicViewPopupMenu(view, entity, pos, actions, mayInvokeDefaultMenu);
}

/**
 * Called when the user selects a document window from the
 * window list.
 */
void QC_ApplicationWindow::slotWindowsMenuActivated(bool /*id*/) {
    const int ii = qobject_cast<QAction*>(sender())->data().toInt();
    QMdiSubWindow* w = m_mdiAreaCAD->subWindowList().at(ii);
    if (w != nullptr && w != m_mdiAreaCAD->activeSubWindow()) {
        doActivate(w);
    }
}

/**
 * Called when something changed in the pen toolbar
 * (e.g. color, width, style).
 */
void QC_ApplicationWindow::slotPenChanged(const RS_Pen& pen) {
    const QC_MDIWindow* w = getCurrentMDIWindow();
    if (w != nullptr) {
        w->slotPenChanged(pen);
    }
}

QC_MDIWindow* QC_ApplicationWindow::createNewDrawingWindow(RS_Document* doc, const QString& expectedFileName) {
    static unsigned id = 0;
    id++;

    auto* w = new QC_MDIWindow(doc, m_mdiAreaCAD, false, m_actionContext);
    QG_GraphicView* view = setupNewGraphicView(w);

    m_actionHandler->setDocumentAndView(w->getDocument(), view);

    QString baseTitleString;
    if (w->getDocument()->rtti() == RS2::EntityBlock) {
        baseTitleString = tr("Block '%1'").arg(expectedFileName);
    }
    else {
        if (expectedFileName.isEmpty()) {
            baseTitleString = tr("unnamed document %1").arg(id);
        }
        else {
            baseTitleString = getFileNameFromFullPath(expectedFileName);
        }
    }

    const bool draftMode = LC_GET_ONE_BOOL("Appearance", "DraftMode", false);
    view->setDraftMode(draftMode);

    setupMDIWindowTitleByName(w, baseTitleString, draftMode);
    w->setWindowIcon(QIcon(":/icons/document.lci"));

    // fixme - sand- where that listeners are removed?
    RS_Graphic* graphic = w->getDocument()->getGraphic();
    if (graphic != nullptr) {
        graphic->addLayerListListener(view);
    }

    m_windowList << w;
    m_mdiAreaCAD->addSubWindow(w);
    return w;
}

void QC_ApplicationWindow::recreateToolbarsMenu() {
    m_menuFactory->recreateToolbarsMenu();
}

QG_GraphicView* QC_ApplicationWindow::setupNewGraphicView(const QC_MDIWindow* w) {
    QG_GraphicView* view = w->getGraphicView();
    LC_GROUP("Appearance");
    const bool antialiasing = LC_GET_BOOL("Antialiasing"); // fixme - sand - check whether its not loaded in loadSettings() later
    const bool showScrollbars = LC_GET_BOOL("ScrollBars", true);
    const bool cursor_hiding = LC_GET_BOOL("cursor_hiding");
    LC_GROUP_END();

    view->setAntialiasing(antialiasing);
    view->setCursorHiding(cursor_hiding);
    view->setDeviceName(LC_GET_ONE_STR("Hardware", "Device", "Mouse"));
    if (showScrollbars) {
        view->addScrollbars();
    }
     connect(view, &QG_GraphicView::gridStatusChanged, this, &QC_ApplicationWindow::updateGridStatus);
     connect(view, &RS_GraphicView::currentActionChanged, this, &QC_ApplicationWindow::onViewCurrentActionChanged);
    // ==========================================================================================================================
    // NOTE: this connect leads to quite a mystical HEAP CORRUPTION in destructor in RS_GraphicView desctructor (or, of view->disconnedAll()
    // is called. Dr.Memory dies earlier than it access this connect, so the reason is not too obvious.
    // However, such heap corruption is observed only under MSVC compiler (Win 10), and only if functions inlining is
    // enabled by compiler options (i.e with complier options used by default).
    // I'm still not sure what is the reason for heap corruption there, however, it's seems that actually that slot is not actually needed
    // as so far it seems it is not called.
    // Thus let this commented call be there for now, and most probably it should be removed later (or some other slot will be used).
    // ==========================================================================================================================
    // connect(view, &RS_GraphicView::previousZoomAvailable, this, &QC_ApplicationWindow::setPreviousZoomEnable);
    // ==========================================================================================================================

    return view;
}

bool QC_ApplicationWindow::newDrawingFromTemplate(const QString& fileName, QC_MDIWindow* w) {
    bool ret = false;
    constexpr RS2::FormatType type = RS2::FormatDXFRW;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    showStatusMessage(tr("Creating new file..."));
    w = createNewDrawingWindow(nullptr, "");
    qApp->processEvents(QEventLoop::AllEvents, 1000);

    const bool noFile = fileName.isEmpty();
    if (noFile) {
        ret = true;
    }
    else {
        // loads the template file in the new view:
        ret = w->loadDocumentFromTemplate(fileName, type);
    }

    if (ret) {
        updateCoordinateWidgetFormat();
        doActivate(w);
        doArrangeWindows(RS2::CurrentMode);
        autoZoomAfterLoad(w->getGraphicView());
        if (!noFile) {
            const QString message = tr("New document from template: ") + fileName;
            notificationMessage(message, 2000);
        }
        else {
            showStatusMessage(tr("New Drawing created."), 2000);
        }
        const auto graphic = w->getGraphic();
        if (graphic != nullptr) {
            if (noFile) {
                // indicate that loading is completed so we could update default dim style from vars
                graphic->onLoadingCompleted();
            }
            emit gridChanged(graphic->isGridOn());
        }
    }

    QApplication::restoreOverrideCursor();
    return ret;
}

/**
 * Menu file -> New (using a predefined Template).
 */
void QC_ApplicationWindow::slotFileNewFromDefaultTemplate() {
    //tried to load template file indicated in RS_Settings
    const QString templateFileName = LC_GET_ONE_STR("Paths", "Template", "");
    newDrawingFromTemplate(templateFileName);
}

void QC_ApplicationWindow::slotFileNewFromTemplate() {
    const QString fileName = m_dlgHelpr->requestDrawingFileName().first;
    if (fileName.isEmpty()) {
        showStatusMessage(tr("Select Template aborted"), 2000);
        return;
    }

    // Create new document window:
    const QMdiSubWindow* old = m_activeMdiSubWindow;
    QRect geo;
    bool maximized = false;
    if (old != nullptr) {
        //save old geometry
        geo = old->geometry();
        maximized = old->isMaximized();
    }
    QC_MDIWindow* w = nullptr;
    if (!newDrawingFromTemplate(fileName, w)) {
        // error
        const QString msg = tr("Cannot open the file\n%1\nPlease check the permissions.").arg(fileName);
        m_commandWidget->appendHistory(msg);
        QMessageBox::information(this, QMessageBox::tr("Warning"), msg, QMessageBox::Ok);
        //file opening failed, clean up QC_MDIWindow and QMdiSubWindow
        if (w != nullptr) {
            slotFilePrintPreview(false); // fixme - sand  why it's there?
            doClose(w); //force closing, without asking user for confirmation
        }
        QMdiSubWindow* activeWindow = m_mdiAreaCAD->currentSubWindow();

        // activeMDIWindowChanged(w);
        // m_activeMdiSubWindow = nullptr; //to allow reactivate the previous active

        if (activeWindow != nullptr) {
            //restore old geometry
            m_mdiAreaCAD->setActiveSubWindow(activeWindow);
            activeWindow->raise();
            activeWindow->setFocus();
            if (old == nullptr || maximized) {
                activeWindow->showMaximized();
            }
            else {
                activeWindow->setGeometry(geo);
            }
        }
        RS_DEBUG->print("QC_ApplicationWindow::slotFileNewTemplate: load Template failed");
    }
    else {
        RS_DEBUG->print("QC_ApplicationWindow::slotFileNewTemplate() OK");
    }
}

/**
 * Menu file -> open.
 */
void QC_ApplicationWindow::slotFileOpen() {
    const QPair<QString, RS2::FormatType> info = m_dlgHelpr->requestDrawingFileName(RS2::FormatUnknown);
    const QString fileName = info.first;
    if (!fileName.isEmpty()) {
        openFile(fileName, info.second);
    }
}

void QC_ApplicationWindow::slotEditActiveBlock() {
    QC_MDIWindow* parent = getCurrentMDIWindow();
    if (parent == nullptr) {
        return;
    }

    // If block is opened from another block the parent must be set
    // to graphic that contain all these blocks.
    if (parent->getDocument()->rtti() == RS2::EntityBlock) {
        parent = parent->getParentWindow();
    }

    //get blocklist from block widget, bug#3497154
    const RS_BlockList* blockList = m_blockWidget->getBlockList();

    if (blockList == nullptr) {
        return;
    }

    RS_Block* activeBlock = blockList->getActive();
    if (activeBlock == nullptr) {
        return;
    }

    QC_MDIWindow* blockWindow = getWindowWithDoc(activeBlock);
    if (blockWindow != nullptr) {
        m_mdiAreaCAD->setActiveSubWindow(blockWindow);
    }
    else {
        QC_MDIWindow* w = createNewDrawingWindow(activeBlock, activeBlock->getName());
        setupWidgetsByWindow(w);
        parent->addChildWindow(w);
        qApp->processEvents(QEventLoop::AllEvents, 1000);
        // the parent needs a pointer to the block window and vice versa
        updateCoordinateWidgetFormat();
        doActivate(w);
        doArrangeWindows(RS2::CurrentMode);

        const QG_GraphicView* graphicView = w->getGraphicView();
        graphicView->zoomAuto();
    }
}

QString QC_ApplicationWindow::getFileNameFromFullPath(const QString& path) {
    const QFileInfo info(path);
    return info.fileName();
}

void QC_ApplicationWindow::updateCoordinateWidgetFormat() const {
    m_coordinateWidget->setCoordinates({0.0, 0.0}, {0.0, 0.0}, true);
}

void QC_ApplicationWindow::updateWidgetsAsDocumentLoaded(const QC_MDIWindow* w) {
    m_layerWidget->slotUpdateLayerList(); // fixme - sand - rework to signals...?
    m_layerWidget->activateLayer(0);
    m_layerTreeWidget->slotFilteringMaskChanged();
    m_namedViewsWidget->reload();
    m_ucsListWidget->reload();
    m_quickInfoWidget->updateFormats();
    m_propertySheetWidget->updateFormats();

    const auto graphic = w->getGraphic();
    if (graphic != nullptr) {
        if (const int objects_removed = graphic->clean()) {
            const auto msg = QObject::tr("Invalid objects removed:");
            m_commandWidget->appendHistory(msg + " " + QString::number(objects_removed));
        }
        emit gridChanged(graphic->isGridOn());

        m_anglesBasisWidget->update(graphic);
    }

    // update coordinate widget format:
    updateCoordinateWidgetFormat();
}

void QC_ApplicationWindow::autoZoomAfterLoad(const QG_GraphicView* graphicView) {
    if (LC_GET_ONE_BOOL("CADPreferences", "AutoZoomDrawing", true)) {
        graphicView->zoomAuto(false);
    }
}

int QC_ApplicationWindow::maybeSurfaceBlocksDock(RS_Graphic *graphic) {
  if (graphic == nullptr || m_blockWidget == nullptr)
    return 0;
  if (graphic->countDeep() != 0)
    return 0;

  RS_BlockList *blockList = m_blockWidget->getBlockList();
  if (blockList == nullptr)
    return 0;

  int hits = 0;
  for (int i = 0; i < blockList->count(); ++i) {
    RS_Block *block = blockList->at(i);
    if (block == nullptr || block->isUndone())
      continue;
    // *Model_Space / *Paper_Space[N] are pseudo-blocks that mirror the
    // ENTITIES section; surfacing them is pointless.
    if (block->getName().startsWith('*'))
      continue;
    if (block->countDeep() > 0)
      ++hits;
  }
  if (hits == 0)
    return 0;

  if (auto *dock = qobject_cast<QDockWidget *>(m_blockWidget->parentWidget())) {
    dock->show();
    dock->raise();
    if (dock->isFloating())
      dock->activateWindow();
  }
  return hits;
}

void QC_ApplicationWindow::openFile(const QString& fileName, const RS2::FormatType type) {
    if (!QFileInfo::exists(fileName)) {
        m_commandWidget->appendHistory(tr("File '%1' does not exist. Opening aborted").arg(fileName));
        showStatusMessage(tr("Opening aborted"), 2000);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (m_openedFiles.indexOf(fileName) >= 0) {
        const QString message = tr("Warning: File already opened : ") + fileName;
        notificationMessage(message, 2000);
    }

    // Create new document window:
    const auto w = createNewDrawingWindow(nullptr, fileName);
    qApp->processEvents(QEventLoop::AllEvents, 1000);

    // open the file in the new view:
    bool success = false;
    if (QFileInfo::exists(fileName)) {
        success = w->loadDocument(fileName, type);
    }
    else {
        const QString msg = tr("Cannot open the file\n%1\nPlease check its existence and permissions.").arg(fileName);
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

    // update recent files menu:
    m_recentFilesList->add(fileName);
    m_openedFiles.push_back(fileName);

    if (m_mdiAreaCAD->viewMode() == QMdiArea::TabbedView) {
        const QList<QTabBar*> tabBarList = m_mdiAreaCAD->findChildren<QTabBar*>();
        QTabBar* tabBar = tabBarList.at(0);
        if (tabBar != nullptr) {
            tabBar->setExpanding(false);
            tabBar->setTabToolTip(tabBar->currentIndex(), fileName);
        }
    }
    else {
        doArrangeWindows(RS2::CurrentMode);
    }

    doActivate(w);

    updateWidgetsAsDocumentLoaded(w);

    const auto graphicView = w->getGraphicView();
    autoZoomAfterLoad(graphicView);
    const auto graphic = graphicView->getGraphic(true);
    if (graphic != nullptr) {
        graphic->setModified(false);
    }

    int blocksWithGeometry = maybeSurfaceBlocksDock(graphic);
    QString message;
    int messageTimeout = 0;
    if (blocksWithGeometry > 0) {
      message = tr("Loaded %1 — modelspace is empty; %n block(s) in the Blocks "
                   "dock contain geometry.",
                   "", blocksWithGeometry)
                    .arg(fileName);
      messageTimeout = 8000;
    } else {
      message = tr("Loaded document: ") + fileName;
      messageTimeout = 2000;
    }
    notificationMessage(message, messageTimeout);

    QApplication::restoreOverrideCursor();
}

void QC_ApplicationWindow::notifyCurrentDrawingOptionsChanged() {
    const auto graphicView = getCurrentGraphicView();
    RS_Graphic* graphic = graphicView->getGraphic(true);
    updateCoordinateWidgetFormat();
    m_quickInfoWidget->updateFormats();
    m_propertySheetWidget->updateFormats();
    m_anglesBasisWidget->update(graphic);
    m_relativeZeroCoordinatesWidget->updateFormats();
    m_ucsListWidget->reload();
    m_namedViewsWidget->reload();
    graphicView->loadSettings();
    graphic->update();
    graphicView->redraw();
    graphicView->repaint();
}

void QC_ApplicationWindow::changeDrawingOptions(const int tabToShowIndex) {
    const auto graphicView = getCurrentGraphicView();
    RS_Graphic* graphic = graphicView->getGraphic(true);

    const int dialogResult = m_dlgHelpr->requestOptionsDrawingDialog(*graphic, tabToShowIndex);
    if (dialogResult == QDialog::Accepted) {
        notifyCurrentDrawingOptionsChanged();
        // fixme - sand - emit signal?
    }
    else {
    }
}

void QC_ApplicationWindow::openFile(const QString& fileName) {
    openFile(fileName, RS2::FormatUnknown);
}

/**
 * Menu file -> save.
 */
void QC_ApplicationWindow::slotFileSave() {
    if (doSave(getCurrentMDIWindow())) {
        m_recentFilesList->updateRecentFilesMenu();
    }
}

/**
 * Menu file -> save as.
 */
void QC_ApplicationWindow::slotFileSaveAs() {
    if (doSave(getCurrentMDIWindow(), true)) {
        m_recentFilesList->updateRecentFilesMenu();
    }
}

bool QC_ApplicationWindow::doSaveAllFiles() {
    QC_MDIWindow* current = getCurrentMDIWindow();
    bool result{true};
    for (const auto w : std::as_const(m_windowList)) {
        if (w != nullptr && w->isModified()) {
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

void QC_ApplicationWindow::slotFileSaveAll() {
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

    const QC_MDIWindow* w = getCurrentMDIWindow();
    if (w != nullptr) {
        QString autosaveFileName;
        if (w->autoSaveDocument(autosaveFileName)) {
            showStatusMessage(tr("Auto-saved drawing"), 2000);
        }
        else {
            // error
            m_autosaveTimer->stop();
            QMessageBox::information(this, QMessageBox::tr("Warning"),
                                     tr("Cannot auto-save the file\n%1\nPlease check the permissions.\n" "Auto-save disabled.").arg(
                                         autosaveFileName), QMessageBox::Ok);
            showStatusMessage(tr("Auto-saving failed"), 2000);
        }
    }
}

void QC_ApplicationWindow::showStatusMessage(const QString& msg, const int timeout) const {
    statusBar()->showMessage(msg, timeout);
}

void QC_ApplicationWindow::notificationMessage(const QString& msg, const int timeout) const {
    statusBar()->showMessage(msg, timeout);
    const bool duplicateMessageInCmdWidget = true; // fixme - sand - complete - setting? Rework later with cmd
    if (duplicateMessageInCmdWidget) {
        m_commandWidget->appendHistory(msg);
    }
}

void QC_ApplicationWindow::initCompleted() {
    enableFileActions();
}

void QC_ApplicationWindow::slotFileExport() {
    const auto* w = getCurrentMDIWindow();
    if (w != nullptr) {
        const auto graphic = w->getGraphic();
        if (graphic != nullptr) {
            const QString currentDocumentFileName = w->getFileName();
            const LC_ExportToImageService exportService(this, m_dlgHelpr.get());
            exportService.exportGraphicsToImage(graphic, currentDocumentFileName);
        }
    }
}

/**
 * Called when a sub window is about to close.
 * If modified, show the Save/Close/Cancel dialog, then do the request.
 * If a save is needed but the user cancels, the window is not closed.
 */
void QC_ApplicationWindow::closeWindow(QC_MDIWindow* win) {
    if (win != nullptr) {
        bool cancel = false;
        const bool hasParent = win->getParentWindow() != nullptr;
        if (win->isModified() && !hasParent) {
            // fixme - sand - files - simplify the logic there
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
}

bool QC_ApplicationWindow::doCloseAllFiles() {
    bool hasParent(false);
    QC_MDIWindow::SaveOnClosePolicy policy = QC_MDIWindow::SaveOnClosePolicy::ASK;
    for (const auto w : std::as_const(m_windowList)) {
        if (w != nullptr) {
            hasParent = w->getParentWindow() != nullptr;
            if (w->isModified() && !hasParent && policy == QC_MDIWindow::SaveOnClosePolicy::ASK) {
                doActivate(w);
                switch (showCloseDialog(w, m_windowList.count() > 1)) {
                    case QG_ExitDialog::DontSaveAll:
                        policy = QC_MDIWindow::SaveOnClosePolicy::DONT_SAVE;
                        break;
                    case QG_ExitDialog::DontSave:
                        w->setSaveOnClosePolicy(QC_MDIWindow::SaveOnClosePolicy::DONT_SAVE);
                        break;
                    case QG_ExitDialog::SaveAll:
                        policy = QC_MDIWindow::SaveOnClosePolicy::SAVE;
                        break;
                    case QG_ExitDialog::Save: {
                        w->setSaveOnClosePolicy(QC_MDIWindow::SaveOnClosePolicy::SAVE);
                        if (!doSave(w)) {
                            showStatusMessage(tr("Close All cancelled"), 2000);
                            return true;
                        }
                        break;
                    }
                    case QG_ExitDialog::Cancel:
                        showStatusMessage(tr("Close All cancelled"), 2000);
                        return true;
                    default:
                        break;
                }
            }
            if (policy != QC_MDIWindow::SaveOnClosePolicy::ASK) {
                w->setSaveOnClosePolicy(policy);
            }
            doClose(w);
            if (w->getSaveOnClosePolicy() == QC_MDIWindow::SaveOnClosePolicy::CANCEL) {
                // this may occur if window contains new untitled document. In such case, the user will be
                // prompted to enter the name of the document via save dialog - and here it is possible to cancel
                // save - so cancelling save for new doc affects the process of all windows save.
                showStatusMessage(tr("Close All cancelled"), 2000);
                return true;
            }
            doArrangeWindows(RS2::CurrentMode);
        }
        qApp->processEvents(QEventLoop::AllEvents, 1000);
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

void QC_ApplicationWindow::slotFilePrintPDF() {
    slotFilePrint(true);
}

/**
 * Menu file -> print.
 */
void QC_ApplicationWindow::slotFilePrint(const bool printPDF) {
    QC_MDIWindow* w = getCurrentMDIWindow();
    if (w == nullptr) {
        return;
    }

    // Avoid printing without print preview
    if (!w->getGraphicView()->isPrintPreview()) {
        slotFilePrintPreview(true);
        return;
    }

    const RS_Graphic* graphic = w->getDocument()->getGraphic();
    if (graphic != nullptr) {
        showStatusMessage(tr("Printing..."));
        using namespace LC_Printing;
        const PrinterType type = printPDF ? PrinterType::PDF : PrinterType::Printer;
        print(*w, type);
        showStatusMessage(tr("Printing complete"), 2000);
    }
}

bool QC_ApplicationWindow::closePrintPreview(QC_MDIWindow* parent) {
    const QG_GraphicView* graphicView = parent->getGraphicView();
    if (graphicView->isPrintPreview()) {
        graphicView->hideOptions();
        RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): close");
        updateActionsAndWidgetsForPrintPreview(false);
        doClose(parent);
        doArrangeWindows(RS2::CurrentMode);
        return true;
    }
    return false;
}

void QC_ApplicationWindow::openPrintPreview(QC_MDIWindow* parent) {
    // look for an existing print preview:
    QC_MDIWindow* existingPrintPreview = parent->getPrintPreview();

    if (existingPrintPreview != nullptr) {
        doActivate(existingPrintPreview);
        doArrangeWindows(RS2::CurrentMode);
        updateActionsAndWidgetsForPrintPreview(true);
    }
    else {
        if (!parent->getGraphicView()->isPrintPreview()) {
            auto* w = new QC_MDIWindow(parent->getDocument(), m_mdiAreaCAD, true, m_actionContext);
            m_mdiAreaCAD->addSubWindow(w);
            parent->addChildWindow(w);

            const bool draftMode = LC_GET_ONE_BOOL("Appearance", "DraftMode");
            setupMDIWindowTitleByFile(w, parent->getFileName(), draftMode, true);

            w->setWindowIcon(QIcon(":/icons/document.lci"));
            QG_GraphicView* view = w->getGraphicView();
            view->setDeviceName(LC_GET_ONE_STR("Hardware", "Device", "Mouse"));
            const auto printPreviewAction = new RS_ActionPrintPreview(m_actionContext);
            printPreviewAction->postCreateInit();
            view->setDefaultAction(printPreviewAction); // fixme - sand - is it correct for preview?

            // fixme - view_connect_ok
             connect(view, &RS_GraphicView::currentActionChanged, this, &QC_ApplicationWindow::onViewCurrentActionChanged);

            // only graphics offer block lists, blocks don't
            /*RS_DEBUG->print("  adding listeners");*/

            doActivate(w);
            doArrangeWindows(RS2::CurrentMode);

            view->zoomAuto(false);
            RS_Graphic* graphic = w->getDocument()->getGraphic();
            if (graphic != nullptr) {
                const auto plotSettings = graphic->getPlotSettings();
                const bool bigger = plotSettings->isBiggerThanPaper(graphic->getSize());
                const bool fixed = plotSettings->isPaperScaleFixed();

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
                }
                else {
                    RS_DEBUG->print("%s: call zoomPage()", __func__);
                    view->getViewPort()->zoomPage();
                }
            }
            updateActionsAndWidgetsForPrintPreview(true);
        }
    }
}

void QC_ApplicationWindow::slotFilePrintPreview(const bool on) {
    QC_MDIWindow* parent = getCurrentMDIWindow();
    if (parent == nullptr) {
        return;
    }

    if (on) {
        openPrintPreview(parent);
    }
    else {
        RS_DEBUG->print("QC_ApplicationWindow::slotFilePrintPreview(): off");
        closePrintPreview(parent);
    }
}

/**
 * Menu file -> quit.
 */
void QC_ApplicationWindow::slotFileQuit() {
    showStatusMessage(tr("Exiting application..."));
    qApp->quit(); // signal handler closeEvent() will take care of modifications
}

/**
 * Shows / hides the grid.
 *
 * @param toggle true: show, false: hide.
 */
void QC_ApplicationWindow::slotViewGrid(const bool toggle) {
    const QC_MDIWindow* m = getCurrentMDIWindow();
    if (m != nullptr) {
        RS_Graphic* g = m->getGraphic();
        if (g != nullptr) {
            g->setGridOn(toggle);
            g->getSelection()->fireSelectionChanged();
        }
    }
    updateGrids();
    redrawAll();
}

/**
 * Enables / disables the draft mode.
 *
 * @param toggle true: enable, false: disable.
 */
void QC_ApplicationWindow::slotViewDraft(bool toggle) {
    LC_SET_ONE("Appearance", "DraftMode", toggle);
    // fixme - sand - files - probably just rely on signal??

    doForEachWindowGraphicView([toggle, this](QG_GraphicView* gv, QC_MDIWindow* w) {
        // fixme - sand - files - probably just rely on signal??
        gv->setDraftMode(toggle);
        const QString fileName = w->getFileName();
        setupMDIWindowTitleByFile(w, fileName, toggle, gv->isPrintPreview());
    });
    emit draftChanged(toggle);
    redrawAll();
}

void QC_ApplicationWindow::slotShowEntityDescriptionOnHover(bool toggle) {
    doForEachWindowGraphicView([toggle](QG_GraphicView* gv, [[maybe_unused]] QC_MDIWindow* w) {
        // fixme - sand - files - probably just rely on signal??
        gv->setShowEntityDescriptionOnHover(toggle);
    });
    emit showEntityDescriptionOnHoverChanged(toggle);
    redrawAll();
}

void QC_ApplicationWindow::slotViewDraftLines(bool toggle) {
    LC_SET_ONE("Appearance", "DraftLinesMode", toggle);
    doForEachWindowGraphicView([toggle](const QG_GraphicView* gv, [[maybe_unused]] QC_MDIWindow* w) {
        // fixme - sand - files - probably just rely on signal??
        gv->setDraftLinesMode(toggle);
    });
    emit draftLinesChanged(toggle);
    redrawAll();
}

void QC_ApplicationWindow::slotViewAntialiasing(bool toggle) {
    LC_SET_ONE("Appearance", "Antialiasing", toggle);

    doForEachSubWindowGraphicView([toggle](const QG_GraphicView* gv, [[maybe_unused]] QC_MDIWindow* w) {
        // fixme - sand - files - probably just rely on signal??
        gv->setAntialiasing(toggle);
    });
    emit antialiasingChanged(toggle);
    redrawAll();
}

/**
 * Updates all grids of all graphic views.
 */
void QC_ApplicationWindow::updateGrids() const {
    doForEachSubWindowGraphicView([](QG_GraphicView* gv, [[maybe_unused]] QC_MDIWindow* w) {
        gv->loadSettings();
        gv->redraw(RS2::RedrawGrid);
    });
}

/**
 * Shows / hides the status bar.
 *
 * @param toggle true: show, false: hide.
 */
void QC_ApplicationWindow::slotViewStatusBar(const bool toggle) {
    statusBar()->setVisible(toggle);
    LC_SET_ONE("Appearance", "StatusBarVisible", toggle);
}

void QC_ApplicationWindow::slotViewGridOrtho(const bool toggle) {
    m_gridViewInvoker->setGridView(toggle, false, RS2::IsoGridViewType::Ortho);
}

void QC_ApplicationWindow::slotViewGridIsoLeft(const bool toggle) {
    m_gridViewInvoker->setGridView(toggle, true, RS2::IsoGridViewType::IsoLeft);
}

void QC_ApplicationWindow::slotViewGridIsoRight(const bool toggle) {
    m_gridViewInvoker->setGridView(toggle, true, RS2::IsoGridViewType::IsoRight);
}

void QC_ApplicationWindow::slotViewGridIsoTop(const bool toggle) {
    m_gridViewInvoker->setGridView(toggle, true, RS2::IsoGridViewType::IsoTop);
}

void QC_ApplicationWindow::updateGridViewActions(const bool isometric, const RS2::IsoGridViewType type) const {
    m_gridViewInvoker->updateGridViewActions(isometric, type);
}

void QC_ApplicationWindow::slotOptionsShortcuts() {
    LC_ActionsShortcutsDialog dlg(this, m_actionGroupManager.get());
    dlg.exec();
}

void QC_ApplicationWindow::rebuildMenuIfNecessary() const {
    m_menuFactory->recreateMainMenuIfNeeded(menuBar());
}

/**
 * Shows the dialog for general application preferences.
 */
void QC_ApplicationWindow::slotOptionsGeneral() {
    const int dialogResult = m_dlgHelpr->showGeneralOptionsDialog();
    if (dialogResult == QDialog::Accepted) {
        m_actionOptionsManager->update();
        // fixme - check this signal, probably it's better to rely on settings change
        const bool hideRelativeZero = LC_GET_ONE_BOOL("Appearance", "hideRelativeZero");
        emit signalEnableRelativeZeroSnaps(!hideRelativeZero);

        const bool antialiasing = LC_GET_ONE_BOOL("Appearance", "Antialiasing", false);
        emit antialiasingChanged(antialiasing);

        m_statusbarManager->loadSettings();
        onCADTabBarIndexChanged(0); // force update if settings changed

        doForEachSubWindowGraphicView([this](QG_GraphicView* gv, const QC_MDIWindow* w) {
            gv->loadSettings();
            if (w == m_activeMdiSubWindow) {
                gv->redraw();
            }
        });

        // fixme - sand - consider emitting signal on properties change instead of processing changes there
        m_infoCursorSettingsManager->loadFromSettings();
        rebuildMenuIfNecessary();
    }
    fireCurrentActionIconChanged(nullptr);
}

void QC_ApplicationWindow::slotImportBlock() {
    if (getCurrentMDIWindow() == nullptr) {
        return;
    }

    const QString dxfPath = m_dlgHelpr->requestDrawingFileName().first;
    if (dxfPath.isEmpty()) {
        return;
    }

    // fixme - sand - files - rework - provide the user with error info?
    if (QFileInfo(dxfPath).isReadable()) {
        if (m_actionHandler != nullptr) {
            const std::shared_ptr<RS_ActionInterface> a = m_actionHandler->setCurrentAction(RS2::ActionLibraryInsert);
            if (a) {
                const auto action = static_cast<LC_ActionBlockLibraryInsert*>(a.get());
                action->setFile(dxfPath);
            }
            else {
                RS_DEBUG->print(RS_Debug::D_ERROR, "QC_ApplicationWindow::slotImportBlock:" "Cannot create action RS_ActionLibraryInsert");
            }
        }
    }
    else {
        RS_DEBUG->print(RS_Debug::D_ERROR, "QC_ApplicationWindow::slotImportBlock: Can't read file: '%s'", dxfPath.toLatin1().data());
    }
}

void QC_ApplicationWindow::showAboutWindow() const {
    m_dlgHelpr->showAboutWindow();
}

void QC_ApplicationWindow::openFilesOnStartup(QStringList& fileList, QSplashScreen* splash) const {
    m_lastFilesOpener->openLastOpenFiles(fileList, splash);
}

bool QC_ApplicationWindow::tryCloseAllBeforeExist() {
    m_lastFilesOpener->collectFilesList(m_windowList, m_activeMdiSubWindow);
    const bool mayExit = !doCloseAllFiles();

    if (mayExit) {
        m_lastFilesOpener->saveSettings();
        storeSettings();
    }
    return mayExit;
}

/**
 * Handle hotkeys. Don't let it to the default handler of Qt.
 * it will consume them also if a text field is active
 * which means it's impossible to enter a command.
 */
void QC_ApplicationWindow:: keyPressEvent(QKeyEvent* e) {
    const int key = e->key();
    switch (key) {
        case Qt::Key_Escape: {
            bool doDefaultProcessing = true;
            RS_GraphicView* graphicView = getCurrentGraphicView();
            if (graphicView != nullptr) {
                const auto currentAction = m_actionHandler->getCurrentAction();
                if (currentAction != nullptr) {
                    const RS2::ActionType actionType = currentAction->rtti();
                    if (RS2::isInteractiveInputAction(actionType)) {
                        graphicView->keyPressEvent(e);
                        e->accept();
                        doDefaultProcessing = false;
                    }
                    else if (currentAction->hasVisualSnap()) { // fixme - should we rely on hardcoded shortcut or it's better to use some action?
                        if (e->modifiers() && e->modifiers() & Qt::ShiftModifier) {
                            currentAction->removePrevioustVisualSnapAddition();
                            e->accept();
                        } else {
                            currentAction->stopVisualSnap();
                            e->accept();
                        }
                        doDefaultProcessing = false;
                    }
                }
            }
            if (doDefaultProcessing) {
                slotKillAllActions();
                e->accept();
            }
            break;
        }
        /* Fixme - keyboard
         * it might be better to to use backspace instead of SHIFT+ESC?
         * Yet using Backspace may clash with command widget - so let it be commented until Cms will be reworked
         *case Qt::Key_Backspace: {
            RS_GraphicView* graphicView = getCurrentGraphicView();
            if (graphicView != nullptr) {
                const auto currentAction = m_actionHandler->getCurrentAction();
                if (currentAction != nullptr) {
                    if (currentAction->hasVisualSnap()) {
                        currentAction->removePrevioustVisualSnapAddition();
                        e->accept();
                    }
                }
            }
            break;
        }*/
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // slotKillAllActions();
            onEnterKey();
            e->accept();
            break;

        case Qt::Key_Plus:
        case Qt::Key_Equal:
            m_actionHandler->setCurrentAction(RS2::ActionZoomIn);
            e->accept();
            break;

        case Qt::Key_Minus:
            m_actionHandler->setCurrentAction(RS2::ActionZoomOut);
            e->accept();
            break;
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Tab:{
            RS_GraphicView* graphicView = getCurrentGraphicView();
            if (graphicView != nullptr) {
                QWidget* focusWidget = QApplication::focusWidget();
                const bool focuseNotInLineEdit = dynamic_cast<QLineEdit*>(focusWidget) == nullptr;
                if (focuseNotInLineEdit || true) {
                    graphicView->keyPressEvent(e);
                    if (!e->isAccepted()) {
                        if (key == Qt::Key_Shift || key == Qt::Key_Control) {
                            e->accept();
                        }
                    }
                }
            }
            [[fallthrough]];
        }
        default:
            e->ignore();
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

void QC_ApplicationWindow::relayAction(QAction* q_action) {
    const auto view = getCurrentGraphicView();
    if (view == nullptr) {
        // this is possible if there are not open windows at all
        // when switching back to LibreCAD from another program
        // occasionally no drawings are activated
        qWarning("relayAction: graphicView is nullptr");
        return;
    }

    auto* graphicView = static_cast<QG_GraphicView*>(view);

    if (q_action == nullptr) {
        if (graphicView->isPrintPreview()) {
            q_action = getAction("FilePrintPreview");
        }
    }

    if (q_action != nullptr) {
        bool setAsCurrentActionInView = true;
        const auto property = q_action->property("_SetAsCurrentActionInView");
        if (property.isValid()) {
            setAsCurrentActionInView = property.toBool();
        }

        if (setAsCurrentActionInView) {
            graphicView->setCurrentQAction(q_action);
        }

        const QString commands(q_action->data().toString());
        if (!commands.isEmpty()) {
            const QString title(q_action->text().remove("&"));
            m_commandWidget->appendHistory(title + " : " + commands);
        }
    }

    fireCurrentActionIconChanged(q_action);
}

/**
 * Called by Qt after a toolbar or dockwidget right-click.
 * See QMainWindow::createPopupMenu() for more information.
 */
QMenu* QC_ApplicationWindow::createPopupMenu() {
    return m_menuFactory->createMainWindowPopupMenu();
}

void QC_ApplicationWindow::toggleFullscreen(const bool checked) {
    checked ? showFullScreen() : showMaximized();
    LC_SET_ONE("Appearance", "FullscreenMode", checked);
}

void QC_ApplicationWindow::toggleMainMenu(const bool toggle) {
    menuBar()->setVisible(toggle);
    LC_SET_ONE("Appearance", "MainMenuVisible", toggle);
}

void QC_ApplicationWindow::slotFileOpenRecent(const QAction* action) {
    const auto variant = action->data();
    if (variant.isValid()) {
        showStatusMessage(tr("Opening recent file..."));
        const QString fileName = variant.toString();
        openFile(fileName, RS2::FormatUnknown);
    }
}

/**
 * This slot manipulates the widget options dialog,
 * and reads / writes the associated settings.
 */
void QC_ApplicationWindow::widgetOptionsDialog() {
    if (m_dlgHelpr->showWidgetOptionsDialog()) {
        fireWidgetSettingsChanged();
    }
}

bool QC_ApplicationWindow::loadStyleSheet(const QString& path) const {
    return m_styleHelper->loadStyleSheet(path);
}

void QC_ApplicationWindow::reloadStyleSheet() {
    m_styleHelper->reloadStyleSheet();
}

bool QC_ApplicationWindow::eventFilter(QObject* obj, QEvent* event) {
    if (QEvent::FileOpen == event->type()) {
        const auto* openEvent = static_cast<QFileOpenEvent*>(event);
        openFile(openEvent->file(), RS2::FormatUnknown);
        return true;
    }
    return QObject::eventFilter(obj, event);
}

void QC_ApplicationWindow::onViewCurrentActionChanged(const RS2::ActionType actionType) {
    if (actionType != RS2::ActionNone && actionType != RS2::ActionDefault) {
        const auto qAction = m_actionGroupManager->getActionByType(actionType);
        relayAction(qAction);
    }
    else {
        fireCurrentActionIconChanged(nullptr);
    }
}

void QC_ApplicationWindow::updateGridStatus(const QString& status) const {
    m_gridStatusWidget->setBottomLabel(status);
}

void QC_ApplicationWindow::showDeviceOptions() {
    m_dlgHelpr->showDeviceOptions();
}

void QC_ApplicationWindow::updateDevice(const QString& device) {
    LC_SET_ONE("Hardware", "Device", device);
    for (const auto& win : std::as_const(m_windowList)) {
        win->getGraphicView()->setDeviceName(device);
    }
}

void QC_ApplicationWindow::saveNamedView() {
    m_namedViewsWidget->addNewView();
}

void QC_ApplicationWindow::saveWorkspace(const bool on) {
    m_workspacesInvoker->saveWorkspace(on);
}

void QC_ApplicationWindow::fillWorkspacesList(QList<QPair<int, QString>>& list) const {
    m_workspacesInvoker->fillWorkspacesList(list);
}

void QC_ApplicationWindow::applyWorkspaceById(const int id) const {
    m_workspacesInvoker->applyWorkspaceById(id);
}

void QC_ApplicationWindow::removeWorkspace(const bool on) {
    m_workspacesInvoker->removeWorkspace(on);
}

void QC_ApplicationWindow::restoreWorkspace(const bool on) {
    m_workspacesInvoker->restoreWorkspace(on);
}

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
    m_namedViewsWidget->restoreSelectedView();
}

void QC_ApplicationWindow::restoreNamedView5() {
    doRestoreNamedView(5);
}

void QC_ApplicationWindow::restoreNamedView(const QString& viewName) const {
    m_namedViewsWidget->restoreView(viewName);
}

void QC_ApplicationWindow::doRestoreNamedView(const int i) const {
    m_namedViewsWidget->restoreView(i);
}

void QC_ApplicationWindow::invokeToolbarCreator() {
    m_creatorInvoker->invokeToolbarCreator();
}

void QC_ApplicationWindow::invokeMenuCreator() {
    m_creatorInvoker->invokeMenuCreator();
}

void QC_ApplicationWindow::changeEvent([[maybe_unused]] QEvent* event) {
    // returning to LC via Command+Tab won't always activate a subwindow #821

#if defined(Q_OS_MACOS)
    if (event->type() == QEvent::ActivationChange) {
        if (isActiveWindow()) {
            if (m_currentSubWindow)
                m_mdiAreaCAD->setActiveSubWindow(m_currentSubWindow);
        }
        else {
            m_currentSubWindow = m_mdiAreaCAD->currentSubWindow();
        }
    }
#endif
}

void QC_ApplicationWindow::invokeLicenseWindow() const {
    m_dlgHelpr->showLicenseWindow();
}

void QC_ApplicationWindow::showBlockActivated(const RS_Block* block) const {
    if (block != nullptr) {
        m_blockWidget->activateBlock(const_cast<RS_Block*>(block));
    }
}

QAction* QC_ApplicationWindow::getAction(const QString& actionName) const {
    return m_actionGroupManager->getActionByName(actionName);
}

void QC_ApplicationWindow::commandMessage(const QString& msg) const {
   m_commandWidget->appendHistory(msg);
}

LC_ActionGroup* QC_ApplicationWindow::getActionGroup(const QString& groupName) const {
    return m_actionGroupManager->getActionGroup(groupName);
}

// todo - think later about staying with signal-slot approach... current one is too explicit
void QC_ApplicationWindow::updateActionsAndWidgetsForPrintPreview(const bool printPreviewOn) {
    const bool enable = !printPreviewOn;
    enableWidgets(enable);
    for (const auto a : std::as_const(m_actionsToDisableInPrintPreviewList)) {
        if (a->isEnabled() != enable) {
            a->setEnabled(enable);
        }
    }
    enableWidgetList(enable, {
                         m_coordinateWidget,
                         m_selectionWidget,
                         m_activeLayerNameWidget,
                         m_gridStatusWidget,
                         m_relativeZeroCoordinatesWidget
                     });

    if (printPreviewOn) {
        m_mouseWidget->setActionIcon(QIcon());
        m_propertySheetWidget->setEnabled(true);
        m_propertySheetWidget->setCurrentQAction(getAction("FilePrintPreview"));
    }
    else {
        m_propertySheetWidget->setCurrentQAction(nullptr);
    }

    emit printPreviewChanged(printPreviewOn);
}

void QC_ApplicationWindow::enableWidgets(const bool enable) {
    enableWidgetList(enable, {
                         m_penPaletteWidget,
                         m_quickInfoWidget,
                         m_propertySheetWidget,
                         m_blockWidget,
                         m_penToolBar,
                         m_penWizard,
                         m_ucsListWidget,
                         m_ucsStateWidget,
                         m_anglesBasisWidget,
                         m_libraryWidget->getInsertButton(),
                         m_snapToolBar
                     });
    //  enableWidget(namedViewsWidget,enable);

    if (enable) {
        enableWidgetList(enable, {m_layerTreeWidget, m_layerWidget});
        // command widget should be enabled for print preview as it supports commands...
        // fixme - command widget should be aware of print preview mode and do not support other commands...
        enableWidget(m_commandWidget, enable);
    }
    // fixme - disable widgets from status bar ??
}

void QC_ApplicationWindow::fireIconsRefresh() {
    emit iconsRefreshed();
}

void QC_ApplicationWindow::fireWidgetSettingsChanged() {
    emit widgetSettingsChanged();
}

void QC_ApplicationWindow::fireCurrentActionIconChanged(QAction* actionIcon) {
    emit currentActionIconChanged(actionIcon);
}

void QC_ApplicationWindow::fireWorkspacesChanged() {
    const bool hasWorkspaces = m_workspacesInvoker->hasWorkspaces();
    emit workspacesChanged(hasWorkspaces);
}
