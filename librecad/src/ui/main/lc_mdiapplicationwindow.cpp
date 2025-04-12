

#include "lc_mdiapplicationwindow.h"

#include <QDockWidget>
#include <QMdiArea>
#include <QMenu>
#include <QStyle>
#include <qtabbar.h>

#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_settings.h"

LC_MDIApplicationWindow::LC_MDIApplicationWindow():
    m_currentSubWindow(nullptr){}

/**
 * Goes back to the previous menu or one step in the current action.
 */
void LC_MDIApplicationWindow::slotBack() {
    RS_GraphicView* graphicView = getCurrentGraphicView();
    if (graphicView != nullptr) {
        graphicView->back();
    }
}

/**
 * Goes one step further in the current action.
 */
void LC_MDIApplicationWindow::onEnterKey() {
    RS_GraphicView *graphicView = getCurrentGraphicView();
    if (graphicView != nullptr) {
        graphicView->processEnterKey();
    }
}

RS_GraphicView const *LC_MDIApplicationWindow::getCurrentGraphicView() const {
    QC_MDIWindow const *win = getCurrentMDIWindow();
    if (win != nullptr) {
        return win->getGraphicView();
    }
    return nullptr;
}

RS_GraphicView *LC_MDIApplicationWindow::getCurrentGraphicView() {
    QC_MDIWindow *win = getCurrentMDIWindow();
    if (win != nullptr) {
        return win->getGraphicView();
    }
    return nullptr;
}

RS_Document const *LC_MDIApplicationWindow::getCurrentDocument() const {
    QC_MDIWindow const *win = getCurrentMDIWindow();
    if (win != nullptr) {
        return win->getDocument();
    }
    return nullptr;
}

RS_Document *LC_MDIApplicationWindow::getCurrentDocument() {
    QC_MDIWindow *win = getCurrentMDIWindow();
    if (win != nullptr) {
        return win->getDocument();
    }
    return nullptr;
}

QString LC_MDIApplicationWindow::getCurrentDocumentFileName() const{
    const RS_Document* doc = getCurrentDocument();
    if (doc == nullptr) {
        return "";
    }
    else {
        return doc->getGraphic()->getFilename();
    }
}

/**
 * @return Pointer to the currently active MDI Window or nullptr if no
 * MDI Window is active.
 */
QC_MDIWindow const* LC_MDIApplicationWindow::getCurrentMDIWindow() const{
    QMdiSubWindow *win = m_mdiAreaCAD->currentSubWindow();
    if (win != nullptr) {
        return qobject_cast<QC_MDIWindow *>(win);
    }
    return nullptr;
}

QC_MDIWindow *LC_MDIApplicationWindow::getCurrentMDIWindow(){
    QMdiSubWindow *win = m_mdiAreaCAD->currentSubWindow();
    if (win != nullptr) {
        return qobject_cast<QC_MDIWindow *>(win);
    }
    return nullptr;
}

QMdiArea const *LC_MDIApplicationWindow::getMdiArea() const {
    return m_mdiAreaCAD;
}

QMdiArea *LC_MDIApplicationWindow::getMdiArea() {
    return m_mdiAreaCAD;
}

/**
  * Find a menu entry in the current menu list. This function will try to recursively find the menu
  * searchMenu for example foo/bar
  * thisMenuList list of Widgets
  * currentEntry only used internally during recursion
  * returns 0 when no menu was found
  */
QMenu *LC_MDIApplicationWindow::findMenu(const QString &searchMenu, const QObjectList& thisMenuList, const QString& currentEntry) {
    if (searchMenu==currentEntry) {
        return static_cast<QMenu*>(thisMenuList.at(0)->parent());
    }

    QList<QObject*>::const_iterator i=thisMenuList.begin();
    while (i != thisMenuList.end()) {
        if ((*i)->inherits ("QMenu")) {
            auto *ii=static_cast<QMenu*>(*i);
            if (QMenu *foundMenu=findMenu(searchMenu, ii->children(), currentEntry+"/"+ii->objectName().replace("&", ""))) {
                return foundMenu;
            }
        }
        ++i;
    }
    return nullptr;
}

QC_MDIWindow *LC_MDIApplicationWindow::getWindowWithDoc(const RS_Document *doc) {
    QC_MDIWindow *wwd = nullptr;
    if (doc != nullptr) {
        foreach(QC_MDIWindow *w, m_windowList) {
            if (w && w->getDocument() == doc) {
                wwd = w;
                break;
            }
        }
    }
    return wwd;
}

void LC_MDIApplicationWindow::closeAllWindowsWithDoc(const RS_Document *doc){
    if (doc != nullptr) {
        for (auto*w :m_windowList) {
            if (w != nullptr && w->getDocument() == doc) {
                closeWindow(w);
                break;
            }
        }
    }
}

void LC_MDIApplicationWindow::activateWindowWithFile(const QString &fileName) {
    if (!fileName.isEmpty()) {
        foreach(QC_MDIWindow *w, m_windowList) {
            if (w != nullptr) {
                const QString &docFileName = w->getFileName();
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
                if (m_mdiAreaCAD->currentSubWindow())
                    m_mdiAreaCAD->currentSubWindow()->showMaximized();
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
            default:
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
    m_mdiAreaCAD->setTabShape(static_cast<QTabWidget::TabShape>(shape));
    m_mdiAreaCAD->setTabPosition(static_cast<QTabWidget::TabPosition>(position));
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
    doArrangeWindows(RS2::Cascade, true);
    QList<QMdiSubWindow *> windows = m_mdiAreaCAD->subWindowList();
    switch (windows.size()) {
        case 1:
            //mdiAreaCAD->tileSubWindows();
            slotTile();
        case 0:
            return;
        default: {
            QMdiSubWindow *active = m_mdiAreaCAD->activeSubWindow();
            for (int i = 0; i < windows.size(); ++i) {
                windows.at(i)->showNormal();
            }
            m_mdiAreaCAD->cascadeSubWindows();
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
            int width = m_mdiAreaCAD->width() - (frame.width() - geo.width()) - disX * (windows.size() - 1);
            int height = m_mdiAreaCAD->height() - (frame.width() - geo.width()) - disY * (windows.size() - 1);
            if (width <= 0 || height <= 0) {
                return;
            }
            for (int i = 0; i < windows.size(); ++i) {
                window = windows.at(i);
//            std::cout<<"window:("<<i<<"): pos()="<<(window->pos().x())<<" "<<(window->pos().y())<<std::endl;
                geo = window->geometry();
                window->setGeometry(geo.x(), geo.y(), width, height);
                qobject_cast<QC_MDIWindow *>(window)->zoomAuto();
            }
            m_mdiAreaCAD->setActiveSubWindow(active);
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
    QList<QMdiSubWindow *> windows = m_mdiAreaCAD->subWindowList();
    if (windows.count() <= 1) {
        slotTile();
        return;
    }
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        window->lower();
        window->showNormal();
    }
    int heightForEach = m_mdiAreaCAD->height() / windows.count();
    int y = 0;
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        int preferredHeight = window->minimumHeight()
                              + window->parentWidget()->baseSize().height();
        int actHeight = qMax(heightForEach, preferredHeight);

        window->setGeometry(0, y, m_mdiAreaCAD->width(), actHeight);
        qobject_cast<QC_MDIWindow *>(window)->zoomAuto();
        y += actHeight;
    }
    m_mdiAreaCAD->activeSubWindow()->raise();
}


/**
 * Tiles MDI windows vertically.
 */
void LC_MDIApplicationWindow::slotTileVertical() {

    RS_DEBUG->print("QC_ApplicationWindow::slotTileVertical()");
    doArrangeWindows(RS2::TileVertical, true);

    // primitive horizontal tiling
    QList<QMdiSubWindow *> windows = m_mdiAreaCAD->subWindowList();
    if (windows.count() <= 1) {
        slotTile();
        return;
    }
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        window->lower();
        window->showNormal();
    }
    int widthForEach = m_mdiAreaCAD->width() / windows.count();
    int x = 0;
    for (int i = 0; i < windows.count(); ++i) {
        QMdiSubWindow *window = windows.at(i);
        int preferredWidth = window->minimumWidth()
                             + window->parentWidget()->baseSize().width();
        int actWidth = qMax(widthForEach, preferredWidth);

        window->setGeometry(x, 0, actWidth, m_mdiAreaCAD->height());
        qobject_cast<QC_MDIWindow *>(window)->zoomAuto();
        x += actWidth;
    }
    m_mdiAreaCAD->activeSubWindow()->raise();
}

/**
 * Cascade MDI windows
 */
void LC_MDIApplicationWindow::slotTile() {
    doArrangeWindows(RS2::Tile, true);
    m_mdiAreaCAD->tileSubWindows();
    slotZoomAuto();
}

//auto zoom the graphicView of sub-windows
void LC_MDIApplicationWindow::slotZoomAuto() const {
    doForEachSubWindowGraphicView([]([[maybe_unused]]QG_GraphicView *gv, QC_MDIWindow* win){
        win->zoomAuto();
    });
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
    if (m_mdiAreaCAD->viewMode() == QMdiArea::SubWindowView) {
        LC_SET_ONE("Startup", "TabMode", 1);
        setupCADAreaTabbar();
        QMdiSubWindow *active = m_mdiAreaCAD->activeSubWindow();
        doForEachSubWindowGraphicView([active, this]([[maybe_unused]]QG_GraphicView *gv, QC_MDIWindow* win){
            win->hide();
            if (win != active) {
                win->lower();
            } else {
                win->raise();
            }
            slotSetMaximized(); // fixme - should it really be there and do maximize for each window?
            win->zoomAuto();
        });
    } else {
        LC_SET_ONE("Startup", "TabMode", 0);
        m_mdiAreaCAD->setViewMode(QMdiArea::SubWindowView);
        doArrangeWindows(RS2::CurrentMode);
    }
}

void LC_MDIApplicationWindow::doForEachWindow(const std::function<void(QC_MDIWindow*)>& callback) const{
    for (QC_MDIWindow* value : m_windowList) {
        callback(value);
    }
}
void LC_MDIApplicationWindow::setupCADAreaTabbar() {
    m_mdiAreaCAD->setViewMode(QMdiArea::TabbedView);
    QList<QTabBar *> tabBarList = m_mdiAreaCAD->findChildren<QTabBar *>();
    QTabBar *tabBar = tabBarList.at(0);
    if (tabBar != nullptr) {
        tabBar->setExpanding(false);
        connect(tabBar, &QTabBar::currentChanged, this, &LC_MDIApplicationWindow::onCADTabBarIndexChanged);
    }
}

void LC_MDIApplicationWindow::onCADTabBarIndexChanged([[maybe_unused]]int index) const {
    LC_GROUP("Appearance");
    {
        QList<QTabBar *> tabBarList = m_mdiAreaCAD->findChildren<QTabBar *>();
        if (tabBarList.isEmpty()){
            return;
        }
        bool showCloseButtons = LC_GET_BOOL("ShowCloseButton", true);
        bool showActive = LC_GET_BOOL("ShowCloseButtonActiveOnly", true);
        // setup close button in window tab for tabbed mode
        QTabBar *tabBar = tabBarList.at(0);
        if (tabBar != nullptr) {
            auto closeSide = static_cast<QTabBar::ButtonPosition>(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, nullptr, this));

            for (int i = 0; i < tabBar->count(); ++i) {
                QWidget *closeButtonWidget = tabBar->tabButton(i, closeSide);
                tabBar->setTabEnabled(i, closeButtonWidget != nullptr);
                if (closeButtonWidget != nullptr) {
                    if (showCloseButtons){
                        if (showActive) {
                            if (i != tabBar->currentIndex()) {
                                closeButtonWidget->hide();
                            } else {
                                closeButtonWidget->show();
                            }
                        }
                        else{
                            closeButtonWidget->show();
                        }
                    }
                    else {
                        closeButtonWidget->hide();
                    }
                }
            }
        }
    }
}

/**
 * Redraws all mdi windows.
 */
void LC_MDIApplicationWindow::redrawAll(){
    if (m_mdiAreaCAD) { // fixme - sand - redraw only if the window is visible, for tabbed view - redraw only current view
        for (const QC_MDIWindow *win: m_windowList) {
            if (win != nullptr) {
                QG_GraphicView *gv = win->getGraphicView();
                if (gv != nullptr) {
                    gv->redraw();
                }
            }
        }
    }
}

void LC_MDIApplicationWindow::enableWidgetList(bool enable, const std::vector<QWidget*> &widgeList){
    for (const auto w: widgeList){
        enableWidget(w, enable);
    }
}

void LC_MDIApplicationWindow::enableWidget(QWidget *win, bool enable) {
    if (win != nullptr) {
        if (win->isEnabled() != enable) {
            win->setEnabled(enable);
        }
    }
}

/**
 * Force-Activate this sub window.
 */
void LC_MDIApplicationWindow::doActivate(QMdiSubWindow *win) {
    bool maximized = LC_GET_ONE_BOOL("WindowOptions","Maximized");
    if (win != nullptr) {
        doWindowActivated(win, true);
        win->activateWindow();
        win->raise();
        win->setFocus();
        if (maximized || QMdiArea::TabbedView == m_mdiAreaCAD->viewMode()) {
            win->showMaximized();
        } else {
            win->show();
        }
    }
    if (m_mdiAreaCAD->viewMode() == QMdiArea::SubWindowView) {
        doArrangeWindows(RS2::CurrentMode);
    }
}

void LC_MDIApplicationWindow::slotWindowActivatedByIndex(int index){
    if (index < 0 || index >= m_mdiAreaCAD->subWindowList().size()) {
        return;
    }
    slotWindowActivated(m_mdiAreaCAD->subWindowList().at(index));
}

void LC_MDIApplicationWindow::slotRedockWidgets()  {
    const QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    for (auto *dockwidget: dockwidgets) {
        dockwidget->setFloating(false);
    }
}

/*
void LC_MDIApplicationWindow::slotWindowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState){

}
*/

void LC_MDIApplicationWindow::slotWindowActivated(QMdiSubWindow *w){
    doWindowActivated(w, false);
}

void LC_MDIApplicationWindow::slotWindowActivatedForced(QMdiSubWindow *w){
    doWindowActivated(w, true);
}

void LC_MDIApplicationWindow::doForEachWindowGraphicView(const std::function<void(QG_GraphicView *, QC_MDIWindow *)>& callback) const{
    for (QC_MDIWindow *win: m_windowList) {
        QG_GraphicView *graphicView = win->getGraphicView();
        if (graphicView != nullptr) {
            callback(graphicView, win);
        }
    }
}

void LC_MDIApplicationWindow::doForEachSubWindowGraphicView(const std::function<void(QG_GraphicView *, QC_MDIWindow *)>& callback) const{
    QList<QMdiSubWindow *> windows = m_mdiAreaCAD->subWindowList();
    for (int i = 0; i < windows.size(); ++i) {
        auto *win = qobject_cast<QC_MDIWindow *>(windows.at(i));
        if (win != nullptr) {
            QG_GraphicView *gv = win->getGraphicView();
            if (gv != nullptr) {
                callback(gv, win);
            }
        }
    }
}

QAction* LC_MDIApplicationWindow::enableAction(const QString& name, bool enable) const{
    QAction* action = getAction(name);
    if (action != nullptr) {
        action->setEnabled(enable);
    }
    return action;
}

QAction* LC_MDIApplicationWindow::checkAction(const QString& name, bool enable) const{
    QAction* action = getAction(name);
    if (action != nullptr) {
        action->setChecked(enable);
    }
    return action;
}

void LC_MDIApplicationWindow::checkActions(const std::vector<QString> &actionList, bool enable) const {
    for (const QString &a: actionList){
        checkAction(a, enable);
    }
}

void LC_MDIApplicationWindow::enableActions(const std::vector<QString> &actionList, bool enable) const {
    for (const QString &a: actionList){
        enableAction(a, enable);
    }
}
