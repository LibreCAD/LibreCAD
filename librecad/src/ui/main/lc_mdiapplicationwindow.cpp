#include "qg_recentfiles.h"
#include "qg_graphicview.h"
#include "qc_mdiwindow.h"
#include "lc_undosection.h"
#include "lc_widgetfactory.h"
#include "lc_printing.h"
#include "lc_penwizard.h"
#include "lc_centralwidget.h"
#include "rs_units.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_painter.h"
#include "rs_document.h"
#include "rs_debug.h"
#include "rs_commands.h"
#include "textfileviewer.h"
#include <boost/version.hpp>
#include <QtSvg>
#include <QTimer>
#include <QSysInfo>
#include <QStyleFactory>
#include <QStatusBar>
#include <QRegularExpression>
#include <QPluginLoader>
#include <QPagedPaintDevice>
#include <QMessageBox>
#include <QMenuBar>
#include <QMdiArea>
#include <QImageWriter>
#include <QFileDialog>
#include <QDockWidget>
#include <QByteArray>
#include "qc_applicationwindow.h"
#include "lc_mdiapplicationwindow.h"

LC_MDIApplicationWindow::LC_MDIApplicationWindow():
    current_subwindow(nullptr){}



RS_GraphicView const *LC_MDIApplicationWindow::getGraphicView() const {
    QC_MDIWindow const *m = getMDIWindow();
    if (m) {
        return m->getGraphicView();
    }
    return nullptr;
}

RS_GraphicView *LC_MDIApplicationWindow::getGraphicView() {
    QC_MDIWindow *m = getMDIWindow();
    if (m) {
        return m->getGraphicView();
    }
    return nullptr;
}

RS_Document const *LC_MDIApplicationWindow::getDocument() const {
    QC_MDIWindow const *m = getMDIWindow();
    if (m) {
        return m->getDocument();
    }
    return nullptr;
}

RS_Document *LC_MDIApplicationWindow::getDocument() {
    QC_MDIWindow *m = getMDIWindow();
    if (m) {
        return m->getDocument();
    }
    return nullptr;
}

/**
 * @return Pointer to the currently active MDI Window or nullptr if no
 * MDI Window is active.
 */
QC_MDIWindow const* LC_MDIApplicationWindow::getMDIWindow() const{
    if (mdiAreaCAD) {
        QMdiSubWindow* w=mdiAreaCAD->currentSubWindow();
        if(w) {
            return qobject_cast<QC_MDIWindow*>(w);
        }
    }
    return nullptr;
}

QC_MDIWindow* LC_MDIApplicationWindow::getMDIWindow(){
    if (mdiAreaCAD) {
        QMdiSubWindow* w=mdiAreaCAD->currentSubWindow();
        if(w) {
            return qobject_cast<QC_MDIWindow*>(w);
        }
    }
    return nullptr;
}

QMdiArea const *LC_MDIApplicationWindow::getMdiArea() const {
    return mdiAreaCAD;
}

QMdiArea *LC_MDIApplicationWindow::getMdiArea() {
    return mdiAreaCAD;
}

/**
  * Find a menu entry in the current menu list. This function will try to recursively find the menu
  * searchMenu for example foo/bar
  * thisMenuList list of Widgets
  * currentEntry only used internally during recursion
  * returns 0 when no menu was found
  */
QMenu *LC_MDIApplicationWindow::findMenu(const QString &searchMenu, const QObjectList thisMenuList, const QString& currentEntry) {
    if (searchMenu==currentEntry) {
        return (QMenu *) thisMenuList.at(0)->parent();
    }

    QList<QObject*>::const_iterator i=thisMenuList.begin();
    while (i != thisMenuList.end()) {
        if ((*i)->inherits ("QMenu")) {
            auto *ii=(QMenu*)*i;
            if (QMenu *foundMenu=findMenu(searchMenu, ii->children(), currentEntry+"/"+ii->objectName().replace("&", ""))) {
                return foundMenu;
            }
        }
        ++i;
    }
    return 0;
}

QC_MDIWindow *LC_MDIApplicationWindow::getWindowWithDoc(const RS_Document *doc) {
    QC_MDIWindow *wwd = nullptr;

    if (doc) {
            foreach (QC_MDIWindow *w, window_list) {
                if (w && w->getDocument() == doc) {
                    wwd = w;
                    break;
                }
            }
    }
    return wwd;
}

void LC_MDIApplicationWindow::activateWindowWithFile(QString &fileName) {
    if (!fileName.isEmpty()) {
            foreach (QC_MDIWindow *w, window_list) {
                if (w != nullptr) {}
                RS_Document *doc = w->getDocument();
                if (doc != nullptr) {
                    const QString &docFileName = doc->getFilename();
                    if (fileName == docFileName) {
                        doActivate(w);
                        break;
                    }
                }
            }
    }
}

/**
 * Arrange the sub-windows as specified, and set the setting.
 * Note: Tab mode always uses (and sets) the RS2::Maximized mode.
 * @param m the layout mode; if set to RS2::CurrentMode, read the current setting
 * @param actuallyDont just set the setting, don't actually do the arrangement
 */
void LC_MDIApplicationWindow::doArrangeWindows(RS2::SubWindowMode m, bool actuallyDont) {

    int mode = m != RS2::CurrentMode ? m : LC_GET_ONE_INT("WindowOptions", "SubWindowMode", RS2::Maximized);

    if (!actuallyDont) {
        switch (mode) {
            case RS2::Maximized:
                if (mdiAreaCAD->currentSubWindow())
                    mdiAreaCAD->currentSubWindow()->showMaximized();
                break;
            case RS2::Cascade:
                slotCascade();
                break;
            case RS2::Tile:
                slotTile();
                break;
            case RS2::TileHorizontal:
                slotTileHorizontal();
                break;
            case RS2::TileVertical:
                slotTileVertical();
                break;
        }
    }
    LC_SET_ONE("WindowOptions", "SubWindowMode", mode);
}

/**
 * Set the QTabWidget shape and position for the MDI area; also the settings.
 * Note: setting a Tab layout always sets the window arrangement to RS2::Maximized
 * Used by the Drawing > Layout menu.
 * @param s the tab shape; if RS2::AnyShape read the current setting
 * @param p the tab bar position; if RS2::AnyPosition read the current setting
 */
void LC_MDIApplicationWindow::setTabLayout(RS2::TabShape s, RS2::TabPosition p) {
    LC_GROUP("WindowOptions");
    int shape = (s == RS2::AnyShape) ? LC_GET_INT("TabShape", RS2::Triangular) : s;
    int position = (p == RS2::AnyPosition) ? LC_GET_INT("TabPosition", RS2::West) : p;
    LC_GROUP_END();
    mdiAreaCAD->setTabShape(static_cast<QTabWidget::TabShape>(shape));
    mdiAreaCAD->setTabPosition(static_cast<QTabWidget::TabPosition>(position));
    doArrangeWindows(RS2::Maximized);
    LC_GROUP_GUARD("WindowOptions");
    {
        LC_SET("TabShape", shape);
        LC_SET("TabPosition", position);
    }
}

/**
 * Cascade MDI windows
 */
void LC_MDIApplicationWindow::slotCascade() {
//    mdiAreaCAD->cascadeSubWindows();
//return;
    doArrangeWindows(RS2::Cascade, true);
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    switch (windows.size()) {
        case 1:
            //mdiAreaCAD->tileSubWindows();
            slotTile();
        case 0:
            return;
        default: {
            QMdiSubWindow *active = mdiAreaCAD->activeSubWindow();
            for (int i = 0; i < windows.size(); ++i) {
                windows.at(i)->showNormal();
            }
            mdiAreaCAD->cascadeSubWindows();
            //find displacement by linear-regression
            double mi = 0., mi2 = 0., mw = 0., miw = 0., mh = 0., mih = 0.;
            for (int i = 0; i < windows.size(); ++i) {
                mi += i;
                mi2 += i * i;
                double w = windows.at(i)->pos().x();
                mw += w;
                miw += i * w;
                double h = windows.at(i)->pos().y();
                mh += h;
                mih += i * h;
            }
            mi2 *= windows.size();
            miw *= windows.size();
            mih *= windows.size();
            double d = 1. / (mi2 - mi * mi);
            double disX = (miw - mi * mw) * d;
            double disY = (mih - mi * mh) * d;
            //End of Linear Regression
            //
            QMdiSubWindow *window = windows.first();
            QRect geo = window->geometry();
            QRect frame = window->frameGeometry();
//        std::cout<<"Frame=:"<<( frame.height() - geo.height())<<std::endl;
            int width = mdiAreaCAD->width() - (frame.width() - geo.width()) - disX * (windows.size() - 1);
            int height = mdiAreaCAD->height() - (frame.width() - geo.width()) - disY * (windows.size() - 1);
            if (width <= 0 || height <= 0) {
                return;
            }
            for (int i = 0; i < windows.size(); ++i) {
                window = windows.at(i);
//            std::cout<<"window:("<<i<<"): pos()="<<(window->pos().x())<<" "<<(window->pos().y())<<std::endl;
                geo = window->geometry();
//            if(i==active) {
//                    window->setWindowState(Qt::WindowActive);
//            }else{
//                    window->setWindowState(Qt::WindowNoState);
//            }
                window->setGeometry(geo.x(), geo.y(), width, height);
                qobject_cast<QC_MDIWindow *>(window)->slotZoomAuto();
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
void LC_MDIApplicationWindow::slotTileHorizontal() {

    RS_DEBUG->print("QC_ApplicationWindow::slotTileHorizontal");
    doArrangeWindows(RS2::TileHorizontal, true);

    // primitive horizontal tiling
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    if (windows.count() <= 1) {
        slotTile();
        return;
    }
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        window->lower();
        window->showNormal();
    }
    int heightForEach = mdiAreaCAD->height() / windows.count();
    int y = 0;
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        int preferredHeight = window->minimumHeight()
                              + window->parentWidget()->baseSize().height();
        int actHeight = qMax(heightForEach, preferredHeight);

        window->setGeometry(0, y, mdiAreaCAD->width(), actHeight);
        qobject_cast<QC_MDIWindow *>(window)->slotZoomAuto();
        y += actHeight;
    }
    mdiAreaCAD->activeSubWindow()->raise();
}


/**
 * Tiles MDI windows vertically.
 */
void LC_MDIApplicationWindow::slotTileVertical() {

    RS_DEBUG->print("QC_ApplicationWindow::slotTileVertical()");
    doArrangeWindows(RS2::TileVertical, true);

    // primitive horizontal tiling
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    if (windows.count() <= 1) {
        slotTile();
        return;
    }
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        window->lower();
        window->showNormal();
    }
    int widthForEach = mdiAreaCAD->width() / windows.count();
    int x = 0;
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        int preferredWidth = window->minimumWidth()
                             + window->parentWidget()->baseSize().width();
        int actWidth = qMax(widthForEach, preferredWidth);

        window->setGeometry(x, 0, actWidth, mdiAreaCAD->height());
        qobject_cast<QC_MDIWindow *>(window)->slotZoomAuto();
        x += actWidth;
    }
    mdiAreaCAD->activeSubWindow()->raise();
}


/**
 * Cascade MDI windows
 */
void LC_MDIApplicationWindow::slotTile() {
    doArrangeWindows(RS2::Tile, true);
    mdiAreaCAD->tileSubWindows();
    slotZoomAuto();
}

//auto zoom the graphicView of sub-windows
void LC_MDIApplicationWindow::slotZoomAuto() {
    QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
    for (int i = 0; i < windows.size(); i++) {
        QMdiSubWindow *window = windows.at(i);
        qobject_cast<QC_MDIWindow *>(window)->slotZoomAuto();
    }
}

void LC_MDIApplicationWindow::slotSetMaximized() {
    doArrangeWindows(RS2::Maximized);
}

void LC_MDIApplicationWindow::slotTabShapeRounded() {
    setTabLayout(RS2::Rounded, RS2::AnyPosition);
}

void LC_MDIApplicationWindow::slotTabShapeTriangular() {
    setTabLayout(RS2::Triangular, RS2::AnyPosition);
}

void LC_MDIApplicationWindow::slotTabPositionNorth() {
    setTabLayout(RS2::AnyShape, RS2::North);
}

void LC_MDIApplicationWindow::slotTabPositionSouth() {
    setTabLayout(RS2::AnyShape, RS2::South);
}

void LC_MDIApplicationWindow::slotTabPositionEast() {
    setTabLayout(RS2::AnyShape, RS2::East);
}

void LC_MDIApplicationWindow::slotTabPositionWest() {
    setTabLayout(RS2::AnyShape, RS2::West);
}

/**
 * toggles between subwindow and tab mode for the MdiArea
 */
void LC_MDIApplicationWindow::slotToggleTab() {
    if (mdiAreaCAD->viewMode() == QMdiArea::SubWindowView) {
        LC_SET_ONE("Startup", "TabMode", 1);
        setupCADAreaTabbar();
        QList<QMdiSubWindow *> windows = mdiAreaCAD->subWindowList();
        QMdiSubWindow *active = mdiAreaCAD->activeSubWindow();
        for (int i = 0; i < windows.size(); i++) {
            QMdiSubWindow *m = windows.at(i);
            m->hide();
            if (m != active) {
                m->lower();
            } else {
                m->raise();
            }
            slotSetMaximized();
            qobject_cast<QC_MDIWindow *>(m)->slotZoomAuto();
        }
    } else {
        LC_SET_ONE("Startup", "TabMode", 0);
        mdiAreaCAD->setViewMode(QMdiArea::SubWindowView);
        doArrangeWindows(RS2::CurrentMode);
    }
}

void LC_MDIApplicationWindow::setupCADAreaTabbar() {
    mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
    QList<QTabBar *> tabBarList = mdiAreaCAD->findChildren<QTabBar *>();
    QTabBar *tabBar = tabBarList.at(0);
    if (tabBar != nullptr) {
        tabBar->setExpanding(false);
        connect(tabBar, &QTabBar::currentChanged, this, &LC_MDIApplicationWindow::onCADTabBarIndexChanged);
    }
}

void LC_MDIApplicationWindow::onCADTabBarIndexChanged([[maybe_unused]]int index) {
    LC_GROUP("Appearance");
    {
        QList<QTabBar *> tabBarList = mdiAreaCAD->findChildren<QTabBar *>();
        if (tabBarList.isEmpty()){
            return;
        }
        bool showCloseButtons = LC_GET_BOOL("ShowCloseButton", true);
        bool showActive = LC_GET_BOOL("ShowCloseButtonActiveOnly", true);

        QTabBar *tabBar = tabBarList.at(0);
        if (tabBar != nullptr) {
            QTabBar::ButtonPosition closeSide =
                (QTabBar::ButtonPosition) style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, this);

            for (int i = 0; i < tabBar->count(); ++i) {
                QWidget *w = tabBar->tabButton(i, closeSide);
                tabBar->setTabEnabled(i, w != nullptr);
                if (w != nullptr) {
                    if (showCloseButtons){
                        if (showActive) {
                            if (i != tabBar->currentIndex()) {
//                                tabBar->tabButton(0, QTabBar::RightSide)->deleteLater();
//                                tabBar->setTabButton(0, QTabBar::RightSide, 0);
                                w->hide();
                            } else {
                                w->show();
                            }
                        }
                        else{
                            w->show();
                        }
                    }
                    else {
                        w->hide();
                    }
                }
            }
        }
    }
}

/**
 * Redraws all mdi windows.
 */
void LC_MDIApplicationWindow::redrawAll() {
    if (mdiAreaCAD) {
        for (const QC_MDIWindow *win: window_list) {
            if (win != nullptr) {
                QG_GraphicView *gv = win->getGraphicView();
                if (gv != nullptr) { gv->redraw(); }
            }
        }
    }
}

void LC_MDIApplicationWindow::enableWidget(QWidget *w, bool enable) {
    if (w != nullptr) {
        if (w->isEnabled() != enable) {
            w->setEnabled(enable);
        }
    }
}

/**
 * Force-Activate this sub window.
 */
void LC_MDIApplicationWindow::doActivate(QMdiSubWindow *w) {
    bool maximized = LC_GET_ONE_BOOL("WindowOptions","Maximized");
    if (w) {
        slotWindowActivated(w, true);
        w->activateWindow();
        w->raise();
        w->setFocus();
        if (maximized || QMdiArea::TabbedView == mdiAreaCAD->viewMode()) {
            w->showMaximized();
        } else {
            w->show();
        }
    }
    if (mdiAreaCAD->viewMode() == QMdiArea::SubWindowView) {
        doArrangeWindows(RS2::CurrentMode);
    }
}

void LC_MDIApplicationWindow::slotWindowActivated(int index){
    if(index < 0 || index >= mdiAreaCAD->subWindowList().size()) return;
    slotWindowActivated(mdiAreaCAD->subWindowList().at(index));
}
