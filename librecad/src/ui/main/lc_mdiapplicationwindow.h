#ifndef LC_MDIAPPLICATIONWINDOW_H
#define LC_MDIAPPLICATIONWINDOW_H

#include <memory>

#include <QList>

#include "lc_penpalettewidget.h"
#include "lc_quickinfowidget.h"
#include "mainwindowx.h"
#include "rs.h"
#include "rs_pen.h"
#include "rs_snapper.h"

class QMdiArea;
class QMdiSubWindow;

class LC_MDIApplicationWindow:public MainWindowX{
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
    const QC_MDIWindow* getCurrentMDIWindow() const;
    QC_MDIWindow* getCurrentMDIWindow();

    /**
   * Implementation from RS_MainWindowInterface (and QS_ScripterHostInterface).
   *
   * @return Pointer to the graphic view of the currently active document
   * window or nullptr if no window is available.
   */
    const RS_GraphicView* getCurrentGraphicView() const;
    RS_GraphicView* getCurrentGraphicView();

    /**
     * Implementation from RS_MainWindowInterface (and QS_ScripterHostInterface).
     *
     * @return Pointer to the graphic document of the currently active document
	    * window or nullptr if no window is available.
     */
    const RS_Document* getCurrentDocument() const;
    RS_Document* getCurrentDocument();
    QString getCurrentDocumentFileName() const;
    /**
     * Find opened window for specified document.
     */
    QC_MDIWindow* getWindowWithDoc(const RS_Document* doc);


    // activates window with given filename of drawing, if any
    void activateWindowWithFile(QString &fileName);

    void redrawAll();
    void enableWidgetList(bool enable, const std::vector<QWidget *> &widgeList);
    void enableWidget(QWidget* win, bool enable);

    void doForEachWindow(std::function<void(QC_MDIWindow*)> callback) const;
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
    void slotWindowActivated(QMdiSubWindow *w);
    void slotWindowActivatedByIndex(int);
    friend class QC_MDIWindow;
    QMenu *findMenu(const QString &searchMenu, const QObjectList thisMenuList, const QString& currentEntry);
protected slots:
    void onCADTabBarIndexChanged(int index);
protected:
    /** MdiArea for MDI */
    QMdiArea* m_mdiAreaCAD {nullptr};
    QMdiSubWindow* m_activeMdiSubWindow {nullptr};
    QMdiSubWindow* m_currentSubWindow {nullptr};
    QList<QC_MDIWindow*> m_windowList;
    void doArrangeWindows(RS2::SubWindowMode mode, bool actuallyDont = false);
    void setTabLayout(RS2::TabShape s, RS2::TabPosition p);
    virtual void doActivate(QMdiSubWindow* win);
    void setupCADAreaTabbar();
    void slotWindowActivatedForced(QMdiSubWindow *w);
    virtual void doSlotWindowActivated(QMdiSubWindow *w, bool forced) = 0;
    void doForEachWindowGraphicView(std::function<void(QG_GraphicView *, QC_MDIWindow *)> callback) const;
    void doForEachSubWindowGraphicView(std::function<void(QG_GraphicView *, QC_MDIWindow *)> callback) const;
};

#endif // LC_MDIAPPLICATIONWINDOW_H
