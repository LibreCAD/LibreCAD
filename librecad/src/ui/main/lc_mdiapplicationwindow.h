#ifndef LC_MDIAPPLICATIONWINDOW_H
#define LC_MDIAPPLICATIONWINDOW_H

#include "lc_quickinfowidget.h"
#include "lc_penpalettewidget.h"
#include "rs_snapper.h"
#include "rs_pen.h"
#include "rs.h"
#include <QSettings>
#include <QMap>
#include <memory>
#include "mainwindowx.h"

class LC_MDIApplicationWindow:public MainWindowX{

protected:
    /** MdiArea for MDI */
    QMdiArea* mdiAreaCAD {nullptr};
    QMdiSubWindow* activedMdiSubWindow {nullptr};
    QMdiSubWindow* current_subwindow {nullptr};
    QList<QC_MDIWindow*> window_list;

    QMenu *findMenu(const QString &searchMenu, const QObjectList thisMenuList, const QString& currentEntry);
    void doArrangeWindows(RS2::SubWindowMode mode, bool actuallyDont = false);
    void setTabLayout(RS2::TabShape s, RS2::TabPosition p);
    virtual void doActivate(QMdiSubWindow* w);
    void setupCADAreaTabbar();

protected slots:
    void onCADTabBarIndexChanged(int index);
public:
    LC_MDIApplicationWindow();

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
     * Find opened window for specified document.
     */
    QC_MDIWindow* getWindowWithDoc(const RS_Document* doc);

    // activates window with given filename of drawing, if any
    void activateWindowWithFile(QString &fileName);

    void redrawAll();
    void enableWidget(QWidget* w, bool enable);

public slots:
    void slotCascade();
    void slotTileHorizontal();
    void slotTileVertical();
    void slotSetMaximized();

    void slotTabShapeRounded();
    void slotTabShapeTriangular();
    void slotTabPositionNorth();
    void slotTabPositionSouth();
    void slotTabPositionEast();
    void slotTabPositionWest();
    void slotToggleTab();

    void slotTile();
    void slotZoomAuto();
    virtual void slotWindowActivated(QMdiSubWindow* w, bool forced=false) = 0;
    void slotWindowActivated(int);

    friend class QC_MDIWindow;



};

#endif // LC_MDIAPPLICATIONWINDOW_H
