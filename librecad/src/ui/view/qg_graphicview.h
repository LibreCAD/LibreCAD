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

#ifndef QG_GRAPHICVIEW_H
#define QG_GRAPHICVIEW_H

#include <mutex>
#include <cstdlib>

#include <QString>
#include <QWidget>

#include "lc_ucslist.h"
#include "rs.h"
#include "rs_blocklistlistener.h"
#include "rs_graphicview.h"
#include "rs_layerlistlistener.h"

struct LC_UCSMarkOptions;
class QEnterEvent;
class QG_ScrollBar;
class QGridLayout;
class QLabel;
class QMenu;
class QMouseEvent;
class LC_ActionContext;

/**
 * This is the Qt implementation of a widget which can view a 
 * graphic. 
 *
 * Instances of this class can be linked to layer lists using
 * addLayerListListener().
 */
class QG_GraphicView:   public RS_GraphicView,
                        public RS_LayerListListener,
                        public RS_BlockListListener,
                        public LC_UCSListListener{
    Q_OBJECT
public:
    // fixme - sand - files - restore - what if action context is null?? As for hatch dialog?
    explicit QG_GraphicView(QWidget *parent,  RS_Document *doc = nullptr, LC_ActionContext* actionContext = nullptr);
    ~QG_GraphicView() override;

    int getWidth() const override;
    int getHeight() const override;
    void redraw(RS2::RedrawMethod method=RS2::RedrawAll) override;
    void adjustOffsetControls() override;
    void adjustZoomControls() override;
    void setMouseCursor(RS2::CursorType c) override;
    void updateGridStatusWidget(QString text) override;
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
public:
    void loadSettings() override;

    // Methods from RS_LayerListListener Interface:
    void layerEdited(RS_Layer*) override{
        redraw(RS2::RedrawDrawing);
    }
    void layerRemoved(RS_Layer*) override{
        redraw(RS2::RedrawDrawing);
    }

    void layerToggled(RS_Layer*) override;
    void layerActivated(RS_Layer *) override;

    /**
     * @brief setOffset
     * @param ox, offset X
     * @param oy, offset Y
     */
    void setOffset(int ox, int oy);

    void setAntialiasing(bool state);
    bool isDraftMode() const;
    void setDraftMode(bool dm);
    void setDraftLinesMode(bool mode);

    void setCursorHiding(bool state);
    void addScrollbars();
    bool hasScrollbars();
    void setCurrentQAction(QAction* q_action);
    void destroyMenu(const QString& activator);
    void setMenu(const QString& activator, QMenu* menu);
    QString obtainEntityDescription(RS_Entity *entity, RS2::EntityDescriptionLevel shortDescription) override;
    virtual void initView();
    const QString& getDeviceName() const {
        return m_device;
    }
    void setDeviceName(QString deviceName) {
        m_device = std::move(deviceName);
    }

protected slots:
    void slotHScrolled(int value);
    void slotVScrolled(int value);
protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void tabletEvent(QTabletEvent* e) override;
    void leaveEvent(QEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    bool event(QEvent * e) override;
    void doZoom(RS2::ZoomDirection direction, RS_Vector& center, double zoom_factor);
    void paintEvent(QPaintEvent *)override;
    void resizeEvent(QResizeEvent* e) override;
    void switchToAction(RS2::ActionType actionType, void* data = nullptr) const;
    void autoPanStep();
    void highlightUCSLocation(LC_UCS *ucs) override;
    void ucsHighlightStep();

    virtual void createViewRenderer();
    void addEditEntityEntry(QMouseEvent* event, QMenu& menu);
    // For auto panning by the cursor close to the view border
    void startAutoPanTimer(QMouseEvent *e);
    bool isAutoPan(QMouseEvent* e) const;
signals:
    void xbutton1_released();
    void gridStatusChanged(QString);
private:
    QString m_device;
    QList<QAction*> m_recent_actions;

    //! Horizontal scrollbar.
    QG_ScrollBar* m_hScrollBar = nullptr;
    //! Vertical scrollbar.
    QG_ScrollBar* m_vScrollBar = nullptr;
    //! Layout used to fit in the view and the scrollbars.
    QGridLayout* m_layout = nullptr;
    //! CAD mouse cursor
    std::unique_ptr<QCursor> m_cursorCad;
    //! Delete mouse cursor
    std::unique_ptr<QCursor> m_cursorDel;
    //! Select mouse cursor
    std::unique_ptr<QCursor> m_cursorSelect;
    //! Magnifying glass mouse cursor
    std::unique_ptr<QCursor> m_cursorMagnifier;
    //! Hand mouse cursor
    std::unique_ptr<QCursor> m_cursorHand;

    double m_scrollZoomFactor = 1.137;

    //! Keep tracks of if we are currently doing a high-resolution scrolling
    bool m_isSmoothScrolling;

    std::unique_ptr<LC_UCSMarkOptions> m_ucsMarkOptions;

    QMap<QString, QMenu*> m_menus;

    bool m_scrollbars{false};
    bool m_cursor_hiding{false};
    bool m_selectCursor_hiding{false};
    bool m_invertZoomDirection{false};
    bool m_invertHorizontalScroll {false};
    bool m_invertVerticalScroll {false};

    struct AutoPanData;
    std::unique_ptr<AutoPanData> m_panData;
    struct UCSHighlightData;
    std::unique_ptr<UCSHighlightData> m_ucsHighlightData;

    LC_ActionContext* m_actionContext {nullptr};

    void showEntityPropertiesDialog(RS_Entity *entity);
    void launchEditProperty(RS_Entity *entity);
    void editAction(RS_Entity &entity);
    // for scroll bar adjustment
    std::mutex m_scrollbarMutex;

};

#endif
