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

#ifndef QC_APPLICATIONWINDOW_H
#define QC_APPLICATIONWINDOW_H

#include <QMainWindow>

#include "rs_pen.h"
#include "rs_snapper.h"

#ifdef RS_SCRIPTING
#include "qs_scripter.h"
#include <qsproject.h>
#endif

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
class QHelpEngine;
class QC_PluginInterface;
class QG_ActiveLayerName;
class LC_SimpleTests;
class LC_CustomToolbar;
class QG_ActionHandler;
class RS_GraphicView;
class RS_Document;


struct Sidebar
{
    QAction* view_action;
    QList<QWidget*> widgets;
};

/**
 * Main application window. Hold together document, view and controls.
 *
 * @author Andrew Mustun
 */
class QC_ApplicationWindow: public QMainWindow
{
    Q_OBJECT

public:
    QC_ApplicationWindow();
    ~QC_ApplicationWindow();

    void menus_and_toolbars();
    void add_action(QMenu* menu, QToolBar* toolbar, QAction* action);
    void set_icon_size();
    void initStatusBar();
    void initSettings();
    void restoreDocks();
    void storeSettings();
    void initMDI();
    void initView();

    bool queryExit(bool force);

    /** Catch hotkey for giving focus to command line. */
    virtual void keyPressEvent(QKeyEvent* e);
    void setRedoEnable(bool enable);
    void setUndoEnable(bool enable);

public slots:
    void slot_set_action(QAction* q_action);
    virtual void show();
    void finishSplashScreen();
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
    void slotScriptOpenIDE();
    void slotScriptRun();

    void slotRunStartScript();
    void slotRunScript();
    void slotRunScript(const QString& name);

    void slotInsertBlock();
    void slotInsertBlock(const QString& name);

    /** shows an about dlg*/
    void slotHelpAbout();
    void slotHelpManual();

    /**
     * @brief slotUpdateActiveLayer
     * update layer name when active layer changed
     */
    void slotUpdateActiveLayer();
	void execPlug();

    void goto_wiki();

    void slot_fullscreen(bool checked);

    void setPreviousZoomEnable(bool enable);

    void hide_options(QC_MDIWindow*);
    void slotToggleToolSidebar(bool checked);

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

    /**
     * Implementation from QG_MainWindowInterface.
     *
     * @return Pointer to this.
     */
	const QMainWindow* getMainWindow() const;
	QMainWindow* getMainWindow();

    /**
     * @return Pointer to action handler. Implementation from QG_MainWindowInterface.
     */
	QG_ActionHandler const* getActionHandler() const{
        return actionHandler;
    }
	QG_ActionHandler* getActionHandler(){
		return actionHandler;
	}

    /**
     * @return Pointer to the qsa object.
     */
    #ifdef RS_SCRIPTING
        QSProject* getQSAProject() {
                    if (scripter!=nullptr) {
                    return scripter->getQSAProject();
                    }
                    else {
                            return nullptr;
                    }
        }
    #endif

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
    void closeEvent(QCloseEvent*);
    //! \{ accept drop files to open
    virtual void dropEvent(QDropEvent* e);
    virtual void dragEnterEvent(QDragEnterEvent * event);
    //! \}

private:

    QMenu* createPopupMenu();

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

    #ifdef RS_SCRIPTING
        /** Scripting interface. */
        QS_Scripter* scripter;
    #endif

    #ifdef LC_DEBUGGING
        LC_SimpleTests* m_pSimpleTest;
    #endif

    //display "Draft Mode" in window title for draft mode
    const QString m_qDraftModeTitle;

    /** Pointer to the application window (this). */
    static QC_ApplicationWindow* appWindow;
    QTimer *autosaveTimer;

    /** MdiArea for MDI */
    QMdiArea* mdiAreaCAD{nullptr};
    QMdiSubWindow* activedMdiSubWindow;
    bool mdiAreaTab;

    /** Dialog factory */
    QC_DialogFactory* dialogFactory;

    /** Recent files list */
	QG_RecentFiles* recentFiles;

    /** Action handler. */
    QG_ActionHandler* actionHandler;

    // --- Dockwidgets ---
    Sidebar tool_sidebar; //!< a group of dockwidgets

    /** Layer list widget */
    QG_LayerWidget* layerWidget;
    /** Block list widget */
    QG_BlockWidget* blockWidget;
    /** Library browser widget */
    QG_LibraryWidget* libraryWidget;

    /** Layer list dock widget */
    QDockWidget* dock_layer;
    /** Block list dock widget */
    QDockWidget* dock_block;
    /** Library list dock widget */
    QDockWidget* dock_library;

    /** Command line */
    QG_CommandWidget* commandWidget;
    QDockWidget* dock_command;

    QHelpEngine* helpEngine{nullptr};
    QDockWidget* helpWindow{nullptr};

    // --- Statusbar ---
    /** Coordinate widget */
    QG_CoordinateWidget* coordinateWidget;
    /** Mouse widget */
    QG_MouseWidget* mouseWidget;
    /** Selection Status */
    QG_SelectionWidget* selectionWidget;
    QG_ActiveLayerName* m_pActiveLayerName;

    // --- Menus ---
    QMenu* windowsMenu;
    QMenu* scriptMenu;
    QMenu* helpMenu;
    QMenu* testMenu;
    QMenu* file_menu;

    // --- Toolbars ---
    QToolBar* dockwidgets_toolbar;
    QToolBar* circleToolBar;
    QToolBar* file_toolbar;
    QToolBar* edit_toolbar;
    QToolBar* view_toolbar;
    QG_SnapToolBar* snapToolBar;
    QG_PenToolBar* penToolBar; //!< for selecting the current pen
    QToolBar* optionWidget; //!< for individual tool options
    LC_CustomToolbar* custom_toolbar{nullptr};

    // --- Actions ---
    static QAction* previousZoom;
    static QAction* undoButton;
    static QAction* redoButton;

    QAction* scriptOpenIDE;
    QAction* scriptRun;
    QAction* helpAboutApp;
    QAction* helpManual;

    QAction* statusbar_view_action;

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
};


#endif

