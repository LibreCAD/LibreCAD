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

// Changes: https://github.com/LibreCAD/LibreCAD/commits/master/librecad/src/main/qc_applicationwindow.h

#ifndef QC_APPLICATIONWINDOW_H
#define QC_APPLICATIONWINDOW_H

#include "lc_actioncontext.h"
#include "lc_actionfactory.h"
#include "lc_mdiapplicationwindow.h"
#include "lc_plugininvoker.h"

class LC_PropertySheetWidget;
class LC_ActionGroupManager;
class LC_ActionGroup;
class LC_ActionOptionsManager;
class LC_AnglesBasisWidget;
class LC_AppWindowDialogsInvoker;
class LC_CreatorInvoker;
class LC_CustomStyleHelper;
class LC_DefaultActionContext;
class LC_GridViewInvoker;
class LC_InfoCursorSettingsManager;
class LC_LastOpenFilesOpener;
class LC_LayerTreeWidget;
class LC_MenuFactory;
class LC_NamedViewsListWidget;
class LC_PenPaletteWidget;
class LC_PenWizard;
class LC_PluginInvoker;
class LC_QTStatusbarManager;
class LC_QuickInfoWidget;
class LC_RelZeroCoordinatesWidget;
class LC_ReleaseChecker;
class LC_SnapManager;
class LC_UCSListWidget;
class LC_UCSStateWidget;
class LC_WorkspacesInvoker;
class QC_DialogFactory;
class QG_ActionHandler;
class QG_ActiveLayerName;
class QG_BlockWidget;
class QG_CommandWidget;
class QG_CoordinateWidget;
class QG_LayerWidget;
class QG_LibraryWidget;
class QG_MouseWidget;
class QG_PenToolBar;
class QG_RecentFiles;
class QG_SelectionWidget;
class QG_SnapToolBar;
class QSplashScreen;
class RS_ActionInterface;
class RS_Block;
class RS_Pen;
class TwoStackedLabels;

struct AreasToggleActions {
    QAction* left{nullptr};
    QAction* right{nullptr};
    QAction* top{nullptr};
    QAction* bottom{nullptr};
    QAction* floating{nullptr};
};

/**
 * Main application window. Hold together document, view and controls.
 *
 * @author Andrew Mustun
 */
class QC_ApplicationWindow : public LC_MDIApplicationWindow {
    Q_OBJECT public:
    enum {
        DEFAULT_STATUS_BAR_MESSAGE_TIMEOUT = 2000
    };

    ~QC_ApplicationWindow() override;

    void initSettings();
    void storeSettings() const;

    /** Catch hotkey for giving focus to command line. */
    void keyPressEvent(QKeyEvent* e) override;
    void setRedoEnable(bool enable);

    void setUndoEnable(bool enable);
    void setSaveEnable(bool enable) const;
    bool loadStyleSheet(const QString& path) const;

    bool eventFilter(QObject* obj, QEvent* event) override;
    void onViewCurrentActionChanged(RS2::ActionType actionType);
    QAction* getAction(const QString& actionName) const override;

    void activateWindow(QMdiSubWindow* w) {
        if (w != nullptr) {
            doActivate(w);
        }
    }

    void fireIconsRefresh();
    void fireWidgetSettingsChanged();
    void fireWorkspacesChanged();
    void fireCurrentActionIconChanged(QAction* actionIcon);
    void showStatusMessage(const QString& msg, int timeout = 0) const;
    void notificationMessage(const QString& msg, int timeout) const;
    void initCompleted();
    void openFilesOnStartup(QStringList& fileList, QSplashScreen* splash) const;
    QMenu* createGraphicViewContentMenu(const QMouseEvent* event, QG_GraphicView* view, RS_Entity* entity, const RS_Vector& pos) const;
    void disableUIForAbsentDrawing();
    void relayAction(QAction* q_action);
    /** generates a new document for a graphic. */
    QC_MDIWindow* createNewDrawingWindow(RS_Document* doc, const QString& expectedFileName);
    void recreateToolbarsMenu();
public slots:
    void slotFocus();
    void slotKillAllActions();
    void slotFocusCommandLine();
    void slotFocusOptionsWidget();
    void slotError(const QString& msg) const;
    void slotShowDrawingOptions() const;
    void slotShowDrawingOptionsUnits() const;
    void slotWorkspacesMenuAboutToShow() const;
    void slotWindowsMenuActivated(bool);
    void slotPenChanged(const RS_Pen& pen);
    //void slotSnapsChanged(RS_SnapMode s);
    void slotEnableActions(bool enable) const;
    /** generates a new document based in predefined template */
    void slotFileNewFromDefaultTemplate();
    /** generates a new document based in selected template */
    void slotFileNewFromTemplate();
    /** opens a document */
    void slotFileOpen();
    void slotEditActiveBlock();
    void slotFileOpenRecent(const QAction* action);
    /** saves a document */
    void slotFileSave();
    /** saves a document under a different filename*/
    void slotFileSaveAs();
    /** saves all open documents; return false == operation cancelled **/
    void slotFileSaveAll();
    /** auto-save document */
    void autoSaveCurrentDrawing();
    /** exports the document as bitmap */
    void slotFileExport();
    /** close all files; return false == operation cancelled */
    void slotFileCloseAll();
    /** prints the current file */
    void slotFilePrint(bool printPDF = false);
    void slotFilePrintPDF();
    /** shows print preview of the current file */
    void slotFilePrintPreview(bool on);
    /** exits the application */
    void slotFileQuit();

    /** toggle the grid */
    void slotViewGrid(bool toggle);
    /** toggle the draft mode */
    void slotViewDraft(bool toggle);
    void slotViewDraftLines(bool toggle);
    /** toggle the statusbar */
    void slotViewStatusBar(bool toggle);
    void slotViewAntialiasing(bool toggle);
    void slotViewGridOrtho(bool toggle);
    void slotViewGridIsoLeft(bool toggle);
    void slotViewGridIsoRight(bool toggle);
    void slotViewGridIsoTop(bool toggle);
    void slotOptionsGeneral();
    void slotOptionsShortcuts();
    void slotImportBlock();
    /** shows an about dlg*/
    void showAboutWindow() const;
    /**
     * @brief slotUpdateActiveLayer
     * update layer name when active layer changed
     */
    void slotUpdateActiveLayer() const;
    void toggleFullscreen(bool checked);
    void toggleMainMenu(bool toggle);
    void setPreviousZoomEnable(bool enable);
    void widgetOptionsDialog();
    void reloadStyleSheet();
    void updateGridStatus(const QString&) const;
    void showDeviceOptions();
    void updateDevice(const QString&);
    void invokeMenuCreator();
    void invokeToolbarCreator();
    void saveNamedView();
    void saveWorkspace(bool on);
    void removeWorkspace(bool on);
    void restoreWorkspace(bool on);
    void restoreNamedView1();
    void restoreNamedView2();
    void restoreNamedView3();
    void restoreNamedView4();
    void restoreNamedView5();
    void restoreNamedViewCurrent();
    void restoreNamedView(const QString& viewName) const;
    void invokeLicenseWindow() const;
    void onNewVersionAvailable() const;
    void checkForNewVersion() const;
    void forceCheckForNewVersion() const;
    void slotShowEntityDescriptionOnHover(bool toggle);
    void tryShowRelativeInput(RS2::RelativePointParam paramType) const;
signals:
    void gridChanged(bool on);
    void draftChanged(bool on);
    void draftLinesChanged(bool on);
    void antialiasingChanged(bool on);
    void printPreviewChanged(bool on);
    void windowsChanged(bool windowsLeft);
    void signalEnableRelativeZeroSnaps(bool);
    void showEntityDescriptionOnHoverChanged(bool show);
    void iconsRefreshed();
    void widgetSettingsChanged();
    void workspacesChanged(bool hasWorkspaces);
    void currentActionIconChanged(QAction* actionIcon);
    void currentGraphicViewChanged(RS_GraphicView* graphicView);
public:
    /**
     * @return Pointer to application window.
     */
    static std::unique_ptr<QC_ApplicationWindow>& getAppWindow();

    QG_PenToolBar* getPenToolBar() const { // fixme - rework later, it's not good that the active pen is actually saved as state of UI components!
        return m_penToolBar;
    }

    /**
     * Updates all grids of all graphic views.
     */
    void updateGrids() const;

    QG_BlockWidget* getBlockWidget() const {
        return m_blockWidget;
    }

    QG_SnapToolBar* getSnapToolBar() {
        return m_snapToolBar;
    }

    const QG_SnapToolBar* getSnapToolBar() const {
        return m_snapToolBar;
    }

    LC_PenPaletteWidget* getPenPaletteWidget() const {
        return m_penPaletteWidget;
    }

    AreasToggleActions& getDockAreaToggleActions() {
        return m_dockAreasToggleActions;
    }

    AreasToggleActions& getToolbarAreaToggleActions() {
        return m_toolbarAreasToggleActions;
    }

    LC_QuickInfoWidget* getEntityInfoWidget() const {
        return m_quickInfoWidget;
    }

    LC_AnglesBasisWidget* getAnglesBasisWidget() const {
        return m_anglesBasisWidget;
    }

    // Highlight the active block in the block widget
    void showBlockActivated(const RS_Block* block) const;

    // Auto-save
    void startAutoSaveTimer(bool startAutoBackup);

    int showCloseDialog(const QC_MDIWindow* w, bool showSaveAll = false) const;
    bool doSave(QC_MDIWindow* w, bool forceSaveAs = false);
    void activeMDIWindowChanged(QC_MDIWindow* window);
    void doClose(QC_MDIWindow* w, bool activateNext = true);
    void setupWidgetsByWindow(const QC_MDIWindow* w) const;
    void updateActionsAndWidgetsForPrintPreview(bool printPreviewOn);
    void updateGridViewActions(bool isometric, RS2::IsoGridViewType type) const;
    void fillWorkspacesList(QList<QPair<int, QString>>& list) const;
    void applyWorkspaceById(int id) const;
    void rebuildMenuIfNecessary() const;
    void openFile(const QString& fileName); // Assume Unknown type
    /**
 * opens the given file.
 */
    void openFile(const QString& fileName, RS2::FormatType type);
    void notifyCurrentDrawingOptionsChanged();
    void changeDrawingOptions(int tabToShowIndex);
    void closeWindow(QC_MDIWindow* win) override;

    // todo - think about more generic way of obtainin widgets!
    QG_LibraryWidget* getLibraryWidget() const {
        return m_libraryWidget;
    }

    LC_PropertySheetWidget* getPropertySheetWidget() const {
        return m_propertySheetWidget;
    }

    LC_UCSListWidget* getUCSListWidget() const {
        return m_ucsListWidget;
    }

    LC_LayerTreeWidget* getLayerTreeWidget() const{
        return m_layerTreeWidget;
    }

    LC_NamedViewsListWidget* getNamedViewsListWidget() const{
        return m_namedViewsWidget;
    }

    void commandMessage(const QString& msg) const;
    LC_ActionGroup* getActionGroup(const QString& groupName) const;
    LC_ActionContext* getActionContext() const;

protected:
    bool closePrintPreview(QC_MDIWindow* parent);
    void openPrintPreview(QC_MDIWindow* parent);
    bool doSaveAllFiles();
    bool doCloseAllFiles();
    void closeEvent(QCloseEvent*) override;
    bool isAcceptableDragNDropFileName(const QString& fileName);
    //! \{ accept drop files to open
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void changeEvent(QEvent* event) override;
    //! \}

    QG_GraphicView* setupNewGraphicView(const QC_MDIWindow* w);
    QC_ApplicationWindow();
    QMenu* createPopupMenu() override;
    QString getFileNameFromFullPath(const QString& path);
    void updateCoordinateWidgetFormat() const;
    void updateWidgetsAsDocumentLoaded(const QC_MDIWindow* w);
    void autoZoomAfterLoad(const QG_GraphicView* graphicView);
    bool newDrawingFromTemplate(const QString& fileName, QC_MDIWindow* w = nullptr);
    void doActivate(QMdiSubWindow* w) override;
    void enableFileActions(const QC_MDIWindow* w);
    void enableFileActions();
    void doWindowActivated(QMdiSubWindow* w, bool forced) override;
    void setupMDIWindowTitleByName(QC_MDIWindow* w, const QString& baseTitleString, bool draftMode);
    void setupMDIWindowTitleByFile(QC_MDIWindow* w, const QString& drawingFileFullPath, bool draftMode, bool forPreview);

    bool tryCloseAllBeforeExist();

    void enableWidgets(bool enable);
    void doRestoreNamedView(int i) const;

#ifdef LC_DEBUGGING
    LC_SimpleTests* m_pSimpleTest{nullptr};
#endif

    std::unique_ptr<LC_ActionGroupManager> m_actionGroupManager;
    std::unique_ptr<LC_CreatorInvoker> m_creatorInvoker;
    std::unique_ptr<LC_PluginInvoker> m_pluginInvoker;
    std::unique_ptr<LC_AppWindowDialogsInvoker> m_dlgHelpr;
    std::unique_ptr<LC_WorkspacesInvoker> m_workspacesInvoker;
    std::unique_ptr<LC_ActionFactory> m_actionFactory;
    std::unique_ptr<LC_MenuFactory> m_menuFactory;
    std::unique_ptr<LC_ReleaseChecker> m_releaseChecker;
    std::unique_ptr<LC_LastOpenFilesOpener> m_lastFilesOpener;
    LC_DefaultActionContext* m_actionContext{nullptr};
    LC_ActionOptionsManager* m_actionOptionsManager{nullptr};

    std::unique_ptr<LC_GridViewInvoker> m_gridViewInvoker;
    std::unique_ptr<LC_InfoCursorSettingsManager> m_infoCursorSettingsManager;
    std::unique_ptr<LC_CustomStyleHelper> m_styleHelper;
    std::unique_ptr<LC_SnapManager> m_snapManager;

    /** Pointer to the application window (this). */
    static QC_ApplicationWindow* m_appWindow;
    std::unique_ptr<QTimer> m_autosaveTimer;

    std::unique_ptr<QG_ActionHandler> m_actionHandler;

    /** Dialog factory */
    // fixme - sand - files rework, merge to one factory
    QC_DialogFactory* m_dialogFactory{nullptr};

    /** Recent files list */
    QG_RecentFiles* m_recentFilesList{nullptr};

    // --- Dockwidgets ---
    //! toggle actions for the dock areas
    AreasToggleActions m_dockAreasToggleActions;
    AreasToggleActions m_toolbarAreasToggleActions;

    // --- Dock widgets ---
    QG_LayerWidget* m_layerWidget{nullptr};
    LC_LayerTreeWidget* m_layerTreeWidget{nullptr};
    LC_QuickInfoWidget* m_quickInfoWidget{nullptr};
    LC_PropertySheetWidget* m_propertySheetWidget{nullptr};
    QG_BlockWidget* m_blockWidget{nullptr};
    QG_LibraryWidget* m_libraryWidget{nullptr};
    QG_CommandWidget* m_commandWidget{nullptr};
    LC_PenWizard* m_penWizard{nullptr};
    LC_PenPaletteWidget* m_penPaletteWidget{nullptr};
    LC_NamedViewsListWidget* m_namedViewsWidget{nullptr};
    LC_UCSListWidget* m_ucsListWidget{nullptr};

    // --- Statusbar ---
    QG_CoordinateWidget* m_coordinateWidget{nullptr};
    LC_RelZeroCoordinatesWidget* m_relativeZeroCoordinatesWidget{nullptr};
    QG_MouseWidget* m_mouseWidget{nullptr};
    QG_SelectionWidget* m_selectionWidget{nullptr};
    QG_ActiveLayerName* m_activeLayerNameWidget{nullptr};
    TwoStackedLabels* m_gridStatusWidget{nullptr};
    LC_UCSStateWidget* m_ucsStateWidget{nullptr};
    LC_AnglesBasisWidget* m_anglesBasisWidget{nullptr};
    LC_QTStatusbarManager* m_statusbarManager{nullptr};

    // --- Toolbars ---
    QG_SnapToolBar* m_snapToolBar{nullptr};
    QG_PenToolBar* m_penToolBar{nullptr}; //!< for selecting the current pen
    QToolBar* m_toolOptionsToolbar{nullptr}; //!< for individual tool options

    // --- Actions ---
    QAction* scriptOpenIDE{nullptr};
    QAction* scriptRun{nullptr};
    QAction* m_helpAboutApp{nullptr};

    // --- Flags ---
    bool m_previousZoomEnable{false};
    bool m_undoEnable{false};
    bool m_redoEnable{false};

    // --- Lists ---
    QList<QAction*> m_toolbarViewActionList;
    QList<QAction*> m_dockWidgetViewActionList;
    QList<QAction*> m_recentFilesActionList;

    QStringList m_openedFiles;
    QList<QAction*> m_actionsToDisableInPrintPreviewList;

    friend class LC_WidgetFactory;
    friend class LC_ActionFactory;
    friend class LC_MenuFactory;
    friend class LC_ToolbarFactory;
    friend class LC_ApplicationWindowInitializer;
};

#ifdef _WINDOWS
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

#endif
