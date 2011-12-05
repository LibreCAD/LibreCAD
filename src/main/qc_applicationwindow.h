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


#undef QT_NO_WORKSPACE
#include <qworkspace.h>

#include "qc_mdiwindow.h"
#include "qg_mainwindowinterface.h"

#ifdef RS_SCRIPTING
#include "qs_scripter.h"
#include <qsproject.h>
#endif

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

/**
 * Main application window. Hold together document, view and controls.
 *
 * @author Andrew Mustun
 */
class QC_ApplicationWindow: public QMainWindow,
    public QG_MainWindowInterface
        {
    Q_OBJECT

public:
    QC_ApplicationWindow();
    ~QC_ApplicationWindow();

    void initActions();
    void initMenuBar();
    void initToolBar();
    void initStatusBar();

    void initSettings();
        void restoreDocks();
    void storeSettings();

    void updateRecentFilesMenu();

    void initMDI();
    void initView();

    bool queryExit(bool force);

        /** Catch hotkey for giving focus to command line. */
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void keyReleaseEvent(QKeyEvent* e);

public slots:
    virtual void show();
    void finishSplashScreen();
        void slotFocus();
    void slotBack();
    void slotKillAllActions();
    //void slotNext();
    void slotEnter();
    void slotFocusCommandLine();
        void slotError(const QString& msg);

    void slotWindowActivated(QWidget* w);
    void slotWindowsMenuAboutToShow();
    void slotWindowsMenuActivated(bool);
    void slotCascadeSubWindows();
    void slotTileHorizontal();
    void slotTileVertical();

    void slotPenChanged(RS_Pen p);
    void slotSnapsChanged(RS_SnapMode s);

    /** generates a new document for a graphic. */
    QC_MDIWindow* slotFileNew(RS_Document* doc=NULL);
    /** opens a document */
    void slotFileOpen();
    /**
    * opens a recent file document
    * @param id File Menu id of the file
    */
    void slotFileOpenRecent();
    /**
     * opens the given file.
     */
    void slotFileOpen(const QString& fileName, RS2::FormatType type);
    /** saves a document */
    void slotFileSave();
    /** saves a document under a different filename*/
    void slotFileSaveAs();
    /** auto-save document */
    void slotFileAutoSave();
        /** exports the document as bitmap */
        void slotFileExport();
        bool slotFileExport(const QString& name, const QString& format,
                QSize size, bool black, bool bw=false);
    /** closes the current file */
    void slotFileClose();
    /** closing the current file */
    void slotFileClosing();
    /** prints the current file */
    void slotFilePrint();
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

    // void slotBlocksEdit();
    void slotOptionsGeneral();

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

    /** dumps entities to file */
    void slotTestDumpEntities(RS_EntityContainer* d=NULL);
    /** dumps undo info to stdout */
    void slotTestDumpUndo();
    /** updates all inserts */
    void slotTestUpdateInserts();
    /** draws some random lines */
    void slotTestDrawFreehand();
    /** inserts a test block */
    void slotTestInsertBlock();
    /** inserts a test ellipse */
    void slotTestInsertEllipse();
    /** inserts a test text */
    void slotTestInsertText();
    /** inserts a test image */
    void slotTestInsertImage();
    /** unicode table */
    void slotTestUnicode();
    /** math experimental */
    void slotTestMath01();
    /** resizes window to 640x480 for screen shots */
    void slotTestResize640();
    /** resizes window to 640x480 for screen shots */
    void slotTestResize800();
    /** resizes window to 640x480 for screen shots */
    void slotTestResize1024();

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
     * @return Pointer to workspace.
     */
    QWorkspace* getWorkspace() {
        return workspace;
    }

    /**
     * @return Pointer to the currently active MDI Window or NULL if no
     * MDI Window is active.
     */
    QC_MDIWindow* getMDIWindow() {
                if (workspace!=NULL) {
                return (QC_MDIWindow*)workspace->activeWindow();
                }
                else {
                        return NULL;
                }
    }

    /**
     * Implementation from RS_MainWindowInterface (and QS_ScripterHostInterface).
     *
     * @return Pointer to the graphic view of the currently active document
     * window or NULL if no window is available.
     */
    virtual RS_GraphicView* getGraphicView() {
        QC_MDIWindow* m = getMDIWindow();
        if (m!=NULL) {
            return m->getGraphicView();
        }
        return NULL;
    }

    /**
     * Implementation from RS_MainWindowInterface (and QS_ScripterHostInterface).
     *
     * @return Pointer to the graphic document of the currently active document
     * window or NULL if no window is available.
     */
    virtual RS_Document* getDocument() {
        QC_MDIWindow* m = getMDIWindow();
        if (m!=NULL) {
            return m->getDocument();
        }
        return NULL;
    }

        /**
         * Creates a new document. Implementation from RS_MainWindowInterface.
         */
    virtual void createNewDocument(
                const QString& fileName = QString::null, RS_Document* doc=NULL) {

                slotFileNew(doc);
                if (fileName!=QString::null && getDocument()!=NULL) {
                        getDocument()->setFilename(fileName);
                }
        }

    /**
     * Implementation from QG_MainWindowInterface.
     *
     * @return Pointer to this.
     */
    virtual QMainWindow* getMainWindow() {
        return this;
    }

    /**
     * @return Pointer to action handler. Implementation from QG_MainWindowInterface.
     */
    virtual QG_ActionHandler* getActionHandler() {
        return actionHandler;
    }



        //virtual QToolBar* createToolBar(const QString& name);
        //virtual void addToolBarButton(QToolBar* tb);

    /**
     * @return Pointer to the qsa object.
     */
#ifdef RS_SCRIPTING
    QSProject* getQSAProject() {
                if (scripter!=NULL) {
                return scripter->getQSAProject();
                }
                else {
                        return NULL;
                }
    }
#endif

        void redrawAll();
        void updateGrids();

        /**
         * Implementation from QG_MainWindowInterface.
         */
        virtual void setFocus2() {
                setFocus();
        }

protected:
    void closeEvent(QCloseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent* e);


private:

    QString format_filename_caption(const QString &qstring_in);

    /** Pointer to the application window (this). */
    static QC_ApplicationWindow* appWindow;
    QTimer *autosaveTimer;

    /** Workspace for MDI */
    QWorkspace* workspace;

        /** Dialog factory */
        QC_DialogFactory* dialogFactory;

    /** Layer list widget */
    QG_LayerWidget* layerWidget;
    /** Block list widget */
    QG_BlockWidget* blockWidget;
    /** Library browser widget */
    QG_LibraryWidget* libraryWidget;

    /** Layer list dock widget */
    QDockWidget* layerDockWindow;
    /** Block list dock widget */
    QDockWidget* blockDockWindow;
    /** Library list dock widget */
    QDockWidget* libraryDockWindow;


        /** Command line */
        QG_CommandWidget* commandWidget;
        QDockWidget* commandDockWindow;

    /** Coordinate widget */
    QG_CoordinateWidget* coordinateWidget;
    /** Mouse widget */
    QG_MouseWidget* mouseWidget;
    /** Selection Status */
    QG_SelectionWidget* selectionWidget;

    /** Option widget for individual tool options */
    QToolBar* optionWidget;

    /** Recent files list */
    QG_RecentFiles* recentFiles;
    QStringList openedFiles;

    /** Action handler. */
        QG_ActionHandler* actionHandler;

#ifdef RS_SCRIPTING
        /** Scripting interface. */
        QS_Scripter* scripter;
#endif

    QMenu* fileMenu;
    QMenu* windowsMenu;
    QMenu* scriptMenu;
    QMenu* helpMenu;
    QMenu* testMenu;

    QList <QAction*> recentFilesAction;
    /** the main toolbars */
    QToolBar* fileToolBar;
    QToolBar* editToolBar;
    QToolBar* zoomToolBar;

    QG_SnapToolBar* snapToolBar;

    // Toolbar for selecting the current pen
    QG_PenToolBar* penToolBar;
    // Toolbar for CAD tools
    QG_CadToolBar* cadToolBar;

    QHelpEngine* helpEngine;
    QDockWidget *helpWindow;

    QAction* scriptOpenIDE;
    QAction* scriptRun;

    QAction* helpAboutApp;
    QAction* helpManual;

    QAction *testDumpEntities;
    QAction *testDumpUndo;
    QAction *testUpdateInserts;
    QAction *testDrawFreehand;
    QAction *testInsertBlock;
    QAction *testInsertText;
    QAction *testInsertImage;
    QAction *testUnicode;
    QAction *testInsertEllipse;

    QAction *testMath01;

    QAction *testResize640;
    QAction *testResize800;
    QAction *testResize1024;

//Plugin support
private:
    void loadPlugins();
    QMenu *findMenu(const QString &searchMenu, const QObjectList thisMenuList, const QString& currentEntry);
    QList<QC_PluginInterface*> loadedPlugins;
public slots:
    void execPlug();

};


#endif

