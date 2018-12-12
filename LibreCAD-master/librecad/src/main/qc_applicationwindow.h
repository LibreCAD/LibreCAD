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

#include "mainwindowx.h"

#include "rs_pen.h"
#include "rs_snapper.h"
#include <QMap>

class QMdiArea;
class QMdiSubWindow;
class QC_MDIWindow;
class QG_LibraryWidget;
class QG_CadToolBar;
class QG_SnapToolBar;
class QC_DialogFactory;
class QG_LayerWidget;
class QG_BlockWidget;
class QG_CommandWidget;
class QG_CoordinateWidget;
class QG_MouseWidget;
class QG_SelectionWidget;
class QG_RecentFiles;
class QG_PenToolBar;
class QC_PluginInterface;
class QG_ActiveLayerName;
class LC_SimpleTests;
class LC_CustomToolbar;
class QG_ActionHandler;
class RS_GraphicView;
class RS_Document;
class TwoStackedLabels;
class LC_ActionGroupManager;
class LC_PenWizard;

struct DockAreas
{
    QAction* left;
    QAction* right;
    QAction* top;
    QAction* bottom;
    QAction* floating;
};

/**
 * Main application window. Hold together document, view and controls.
 *
 * @author Andrew Mustun
 */
class QC_ApplicationWindow: public MainWindowX
{
    Q_OBJECT

public:
    QC_ApplicationWindow();
    ~QC_ApplicationWindow();

    void initSettings();
    void storeSettings();

    bool queryExit(bool force);

    /** Catch hotkey for giving focus to command line. */
    virtual void keyPressEvent(QKeyEvent* e) override;
    void setRedoEnable(bool enable);
    void setUndoEnable(bool enable);
    bool loadStyleSheet(QString path);

    bool eventFilter(QObject *obj, QEvent *event) override;

    QMap<QString, QAction*> a_map;
    LC_ActionGroupManager* ag_manager;

public slots:
    void relayAction(QAction* q_action);
    void slotFocus();
    void slotBack();
    void slotKillAllActions();
    void slotEnter();
    void slotFocusCommandLine();
    void slotError(const QString& msg);

    void slotWindowActivated(int);
    void slotWindowActivated(QMdiSubWindow* w);
    void slotWindowsMenuAboutToShow();
    void slotWindowsMenuActivated(bool);
    void slotCascade();
    void slotTile();
    void slotTileHorizontal();
    void slotTileVertical();
    void slotToggleTab();
    void slotZoomAuto();

    void slotPenChanged(RS_Pen p);
    void slotSnapsChanged(RS_SnapMode s);
    void slotEnableActions(bool enable);

    /** generates a new document for a graphic. */
	QC_MDIWindow* slotFileNew(RS_Document* doc=nullptr);
    /** generates a new document based in predefined template */
    void slotFileNewNew();
    /** generates a new document based in selected template */
    void slotFileNewTemplate();
    /** opens a document */
    void slotFileOpen();

    /**
     * opens the given file.
     */
    void slotFileOpen(const QString& fileName, RS2::FormatType type);
    void slotFileOpen(const QString& fileName); // Assume Unknown type
    void slotFileOpenRecent(QAction* action);
    /** saves a document */
    void slotFileSave();
    /** saves a document under a different filename*/
    void slotFileSaveAs();
    /** auto-save document */
    void slotFileAutoSave();
    /** exports the document as bitmap */
    void slotFileExport();
    bool slotFileExport(const QString& name, const QString& format,
                QSize size, QSize borders, bool black, bool bw=true);
    /** closing the current file */
    void slotFileClosing(QC_MDIWindow*);
    /** prints the current file */
    void slotFilePrint(bool printPDF=false);
    void slotFilePrintPDF();
    /** shows print preview of the current file */
    void slotFilePrintPreview(bool on);
    /** exits the application */
    void slotFileQuit();

    /** toggle the grid */
    void slotViewGrid(bool toggle);
    /** toggle the draft mode */
    void slotViewDraft(bool toggle);
    /** toggle the statusbar */
    void slotViewStatusBar(bool toggle);

    void slotOptionsGeneral();

    void slotImportBlock();

    /** shows an about dlg*/
    void showAboutWindow();

    /**
     * @brief slotUpdateActiveLayer
     * update layer name when active layer changed
     */
    void slotUpdateActiveLayer();
	void execPlug();

    void invokeLinkList();

    void toggleFullscreen(bool checked);

    void setPreviousZoomEnable(bool enable);

    void hideOptions(QC_MDIWindow*);

    void widgetOptionsDialog();

    void modifyCommandTitleBar(Qt::DockWidgetArea area);
    void reloadStyleSheet();

    void updateGridStatus(const QString&);

    void showDeviceOptions();

    void updateDevice(QString);

    void invokeMenuCreator();
    void invokeToolbarCreator();
    void createToolbar(const QString& toolbar_name);
    void destroyToolbar(const QString& toolbar_name);
    void destroyMenu(const QString& activator);
    void unassignMenu(const QString& activator, const QString& menu_name);
    void assignMenu(const QString& activator, const QString& menu_name);
    void invokeMenuAssigner(const QString& menu_name);
    void updateMenu(const QString& menu_name);

    void invokeLicenseWindow();


signals:
    void gridChanged(bool on);
    void draftChanged(bool on);
    void printPreviewChanged(bool on);
    void windowsChanged(bool windowsLeft);

public:
    /**
     * @return Pointer to application window.
     */
    static QC_ApplicationWindow* getAppWindow() {
        return appWindow;
    }

    /**
     * @return Pointer to MdiArea.
     */
	QMdiArea const* getMdiArea() const;
	QMdiArea* getMdiArea();

    /**
	 * @return Pointer to the currently active MDI Window or nullptr if no
     * MDI Window is active.
     */
	const QC_MDIWindow* getMDIWindow() const;
	QC_MDIWindow* getMDIWindow();

    /**
     * Implementation from RS_MainWindowInterface (and QS_ScripterHostInterface).
     *
     * @return Pointer to the graphic view of the currently active document
	 * window or nullptr if no window is available.
     */
	const RS_GraphicView* getGraphicView() const;
	RS_GraphicView* getGraphicView();

    /**
     * Implementation from RS_MainWindowInterface (and QS_ScripterHostInterface).
     *
     * @return Pointer to the graphic document of the currently active document
	 * window or nullptr if no window is available.
     */
	const RS_Document* getDocument() const;
	RS_Document* getDocument();

    /**
     * Creates a new document. Implementation from RS_MainWindowInterface.
     */
	void createNewDocument(const QString& fileName = QString::null, RS_Document* doc=nullptr);

    void redrawAll();
    void updateGrids();

    QG_BlockWidget* getBlockWidget(void)
    {
        return blockWidget;
    }

    QG_SnapToolBar* getSnapToolBar(void)
    {
        return snapToolBar;
    }

    QG_SnapToolBar const* getSnapToolBar(void) const
    {
        return snapToolBar;
    }

protected:
    void closeEvent(QCloseEvent*) override;
    //! \{ accept drop files to open
    virtual void dropEvent(QDropEvent* e) override;
    virtual void dragEnterEvent(QDragEnterEvent * event) override;
    void changeEvent(QEvent* event) override;
    //! \}

private:

    QMenu* createPopupMenu() override;

    QString format_filename_caption(const QString &qstring_in);
    /** Helper function for Menu file -> New & New.... */
	bool slotFileNewHelper(QString fileName, QC_MDIWindow* w = nullptr);

    /**
     * @brief updateWindowTitle, for draft mode, add "Draft Mode" to window title
     * @param w, pointer to window widget
     */
    void updateWindowTitle(QWidget* w);

    //Plugin support
    void loadPlugins();
    QMenu *findMenu(const QString &searchMenu, const QObjectList thisMenuList, const QString& currentEntry);

    #ifdef LC_DEBUGGING
        LC_SimpleTests* m_pSimpleTest;
    #endif

    /** Pointer to the application window (this). */
    static QC_ApplicationWindow* appWindow;
    QTimer *autosaveTimer;

    QG_ActionHandler* actionHandler;

    /** MdiArea for MDI */
    QMdiArea* mdiAreaCAD{nullptr};
    QMdiSubWindow* activedMdiSubWindow;
    QMdiSubWindow* current_subwindow;


    /** Dialog factory */
    QC_DialogFactory* dialogFactory;

    /** Recent files list */
	QG_RecentFiles* recentFiles;

    // --- Dockwidgets ---
    //! toggle actions for the dock areas
    DockAreas dock_areas;

    /** Layer list widget */
    QG_LayerWidget* layerWidget;
    /** Block list widget */
    QG_BlockWidget* blockWidget;
    /** Library browser widget */
    QG_LibraryWidget* libraryWidget;
    /** Command line */
    QG_CommandWidget* commandWidget;

    LC_PenWizard* pen_wiz;

    // --- Statusbar ---
    /** Coordinate widget */
    QG_CoordinateWidget* coordinateWidget;
    /** Mouse widget */
    QG_MouseWidget* mouseWidget;
    /** Selection Status */
    QG_SelectionWidget* selectionWidget;
    QG_ActiveLayerName* m_pActiveLayerName;
    TwoStackedLabels* grid_status;

    // --- Menus ---
    QMenu* windowsMenu;
    QMenu* scriptMenu;
    QMenu* helpMenu;
    QMenu* testMenu;
    QMenu* file_menu;

    // --- Toolbars ---
    QG_SnapToolBar* snapToolBar;
    QG_PenToolBar* penToolBar; //!< for selecting the current pen
    QToolBar* optionWidget; //!< for individual tool options

    // --- Actions ---
    QAction* previousZoom;
    QAction* undoButton;
    QAction* redoButton;

    QAction* scriptOpenIDE;
    QAction* scriptRun;
    QAction* helpAboutApp;

    // --- Flags ---
    bool previousZoomEnable{false};
    bool undoEnable{false};
    bool redoEnable{false};

    // --- Lists ---
    QList<QC_PluginInterface*> loadedPlugins;
    QList<QAction*> toolbar_view_actions;
    QList<QAction*> dockwidget_view_actions;
    QList<QC_MDIWindow*> window_list;
    QList<QAction*> recentFilesAction;

    QStringList openedFiles;

    // --- Strings ---
    QString style_sheet_path;

};


#endif

